/* @(#) $Revision: 70.2 $
 */
/**********************************************************************
 *
 * audisp - display the audit information as requested by the parameters
 *
 * syntax:
 *
 *  audisp [ -u username ] [ -e eventname ] [ -c syscall ] [ -p ] [ -f ]
 *         [ -l ttyid ] [ -t start_time ] [ -s stop_time ] audit_filename
 *
 * EXIT CODES:
 *
 *  0  - complete successfully.
 *  1  - wrong usage or option.
 *  2  - not previleged user or cannot do self-auditing.
 *  3  - cannot read audit_file
 *  4  - cannot open or change to /dev directory.
 *  5  - audit record is bad.
 *  6  - not enough room for malloc.
 *  7  - cannot locate pir info for the current audit record.
 *  8  - too many audit files given.
 *
 *
 **********************************************************************/

#include <sys/types.h>
#include <ndir.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/audit.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <pwd.h>
#include <unistd.h>
#include "audparamC.h"
#include "eventmap.h"
#include <netinet/in.h>
#include <sys/pstat.h>

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1 /* set number */
#include <nl_types.h>
#endif NLS

#ifdef NLS
   nl_catd nlmsg_fd;
#endif

#define MAXLOGIN      8      /* longest possible login name length */
#define MAXTTYNAME   14      /* longest possible tty name length */
#define MAXEVNAME 20         /* longest possible event name length */
#define USR_SZ   50
#define EVENT_SZ      50
#define TTY_SZ         50
/* system call record body should be less than 2 max pathnames ? */
#define MAX_SCALL_BODY 2048
#define E_SUSER (catgets(nlmsg_fd,NL_SETN,1, "audisp: you do not have the appropriate privileges.\n"))
#define E_AUDISP_NOAF (catgets(nlmsg_fd,NL_SETN,2, "audisp: can't read audit file %s.\n"))
#define E_AUDISP_BOTHPF (catgets(nlmsg_fd,NL_SETN,3, "audisp: can't use both -p and -f options.\n"))
#define E_AUDISP_USE1 (catgets(nlmsg_fd,NL_SETN,4, "audisp [ -u username ] [ -e eventname ] [ -c syscall ]\n"))
#define E_AUDISP_USE2 (catgets(nlmsg_fd,NL_SETN,5, "       [ -p ] [ -f ] [ -l ttyid ] [ -t mmddhhmm[yy] ]\n"))
#define E_AUDISP_USE3 (catgets(nlmsg_fd,NL_SETN,6, "       [ -s mmddhhmm[yy] ] audit_filename ...\n"))
#define E_AUDISP_START (catgets(nlmsg_fd,NL_SETN,7, "audisp: bad start time in -t option.\n"))
#define E_AUDISP_STOP (catgets(nlmsg_fd,NL_SETN,8, "audisp: bad stop time in -s option.\n"))
#define E_AUDISP_TIMES (catgets(nlmsg_fd,NL_SETN,9, "audisp: start time is greater than stop time.\n"))
#define E_AUDISP_UNAME (catgets(nlmsg_fd,NL_SETN,10, "audisp: all specified users names are invalid.\n"))
#define E_AUDISP_TTY (catgets(nlmsg_fd,NL_SETN,11, "audisp: all specified tty names are invalid.\n"))
#define E_AUDISP_ENAME (catgets(nlmsg_fd,NL_SETN,12, "audisp: all specified event names are invalid.\n"))
#define E_AUDISP_CHDEV (catgets(nlmsg_fd,NL_SETN,13, "audisp: cannot change to /dev directory.\n"))
#define E_AUDISP_NODEV (catgets(nlmsg_fd,NL_SETN,14, "audisp: cannot open /dev directory.\n"))
#define E_AUDISP_BADAR (catgets(nlmsg_fd,NL_SETN,15, "\naudisp: bad audit record.\n"))
#define E_AUDISP_BADBD (catgets(nlmsg_fd,NL_SETN,16, "\naudisp: bad audit record body.\n"))
#define E_AUDISP_ALLOC (catgets(nlmsg_fd,NL_SETN,17, "\naudisp: malloc failed.\n"))
#define E_AUDISP_AFLG (catgets(nlmsg_fd,NL_SETN,18, "\naudisp: bad aud_flag struct.\n"))
#define E_AUDISP_NOF (catgets(nlmsg_fd,NL_SETN,19, "Note: # files opened successfully = %d.\n"))
#define E_AUDISP_PT (catgets(nlmsg_fd,NL_SETN,20, "audisp: cannot back-reference pid ident. information.\n"))
#define E_AUDISP_SELF (catgets(nlmsg_fd,NL_SETN,21, "audisp: attempt to self-audit failed.\n"))

#define SUSER()   (!geteuid())

#define OUTPUTHEAD (catgets(nlmsg_fd,NL_SETN,22, "TIME              PID E  EVENT   PPID    AID   RUID   RGID   EUID   EGID TTY\n\n"))
#define LINE (catgets(nlmsg_fd,NL_SETN,23, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"))
#define E_AUDISP_FNUM (catgets(nlmsg_fd,NL_SETN,87, "audisp: cannot handle so many audit files. (Some may be CDF elements.)\n"))

#define INDENT() printf((catgets(nlmsg_fd,NL_SETN,24, "     ")))
#define CR() printf("\n")

#define SELF_AUD(x) audrec.aud_head.ah_error = (x); audwrite(audrec); audswitch(AUD_RESUME)

#define done goto end

/* macros for handling CDF files */
#define ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define ISCDF(mode) (ISDIR(mode) && ((mode) & S_ISUID))

/* self auditing record for audisp itself */
struct self_audit_rec audrec;

/* define for extracting lower order bits. Used for fomating internet addr */
#define UC(b)   (((int)b)&0xff)

union mixed_body {
   char *scbody;
   char txbody[MAX_AUD_TEXT];
   struct audit_str_data stbody;
   struct pir_body pibody;
};

struct file_cn {
  FILE *fp;
  char *cname;
};

/* array of audit file pointers and corresponding
 * cnode names (if the file is a CDF element).
 * Note that the array size is limited
 * by the max open files per process.
 */
struct file_cn *aud_files;

int af_cnt;                /* audit file counter */
short curr_afind;          /* the index into the current audit file */
struct audit_hdr **aud_heads;

struct pir_node {      /* the pir's form a binary search tree */
   u_short pid;
   struct pir_body info;
   char *ttyname;
   struct pir_node *left, *right;
} *curr_pir_node;

struct pir_node **pir_trees;

short given_start = 0;
short given_stop = 0;
short allusers = 0;
short allevents = 0;
short allttys = 0;
short pf = 0;

long aids[USR_SZ];
short aid_cnt = 0;
int events[EVENT_SZ];
short event_cnt = 0;
dev_t ttydevs[TTY_SZ];
short ttyd_cnt = 0;
struct tm start, stop;

char *tmtoasc();

extern char *strcpy(), *strncpy(), *strcat();
extern void *malloc(), *calloc(), free(), exit();

struct aud_flag vfsmount800 = {1,0,4,1,8,1,8,0,0,0,0,0,0};


/* tty cache description
 * The tty cache is shared by the tc_insert() and the tc_lookup() routines. 
 */
typedef struct tc_entry {
    dev_t dev;
    char ttyname[MAXTTYNAME];
    struct tc_entry *nextp;
} tc_entry_t; 

/* tty_cache_head is the start of the tty cache.
 */
static tc_entry_t *tty_cache_head = (tc_entry_t *)0;

/* console device related variables
 */
struct pst_static *pst_ptr; /* pstat structure- contains console device */
dev_t console_dev;          /* console device */

main(argc, argv)
int argc;
char *argv[];

{
   short i, c;
   extern char *optarg;
   extern int optind;

   short uflg = 0, eflg = 0, pflg = 0, fflg = 0;
   short lflg = 0, tflg = 0, sflg = 0;

   char *users[USR_SZ];       /* array to hold user names */
   char *ttys[TTY_SZ];         /* array to hold tty names */
   char *evnames[EVENT_SZ];   /* array to hold event names */
   char *scnames[EVENT_SZ];   /* array to hold sys call names */

   short tty_cnt = 0;
   short usr_cnt = 0;
   short evname_cnt = 0;
   short scname_cnt = 0;
   char *e_ptr;
   char *c_ptr;
   char *u_ptr;
   char *t_ptr;
   struct tm *tp;

   struct audit_hdr ar_head;
   union mixed_body ar_body;

   int ret_val;            /* generic dummy return value */
   short is800vfsmount;    /* the record is vfsmount syscall on s800 */
   char  *dateformat;      /* NLSized date format */

   int fds;


#ifdef NLS
   nlmsg_fd = catopen("audisp",0);
#endif

   /* set up arrays that depend on number of available file descriptors */

   fds=sysconf(_SC_OPEN_MAX);

   aud_files=(struct file_cn *)calloc(sizeof(struct file_cn),fds);
   if ((int)aud_files == 0) error(E_AUDISP_ALLOC);

   aud_heads=(struct audit_hdr **)calloc(sizeof(struct audit_hdr *),fds);
   if ((int)aud_heads == 0) error(E_AUDISP_ALLOC);

   pir_trees=(struct pir_node **)calloc(sizeof(struct pir_node *),fds);
   if ((int)pir_trees == 0) error(E_AUDISP_ALLOC);

   /* Verify if the user is privileged */
   if (!SUSER()) {
      error(E_SUSER);
      SELF_AUD(2);
      exit(2);
   }

   /* self audit */
   if (audswitch(AUD_SUSPEND) == -1) {
      error(E_AUDISP_SELF);
      exit(2);
   }
   audrec.aud_head.ah_event = EN_AUDISP;
   for (i=0; i < argc; i++) {
      strcat (audrec.aud_body.text, argv[i]);
      strcat (audrec.aud_body.text, (catgets(nlmsg_fd,NL_SETN,25, " ")));
   }
   audrec.aud_head.ah_len = strlen (audrec.aud_body.text);


   /* Determine the system console from the kernel */

   pst_ptr=(struct pst_static *)malloc(sizeof(struct pst_static));
   if ((int)pst_ptr == 0) error(E_AUDISP_ALLOC);

   pstat(PSTAT_STATIC, pst_ptr, sizeof(struct pst_static),0,0);
   console_dev=makedev(pst_ptr->console_device.psd_major,
	pst_ptr->console_device.psd_minor);

   free(pst_ptr);

   /* parse command line */

   if (argc <= 1) {
      error(E_AUDISP_USE1);
      error(E_AUDISP_USE2);
      error(E_AUDISP_USE3);
      SELF_AUD(1);
      exit(1);
   };

   /* get the other arguments */
   while ((c=getopt(argc, argv, "u:c:e:pfl:t:s:")) != EOF)
      switch (c) {
      case 'u':
         uflg++;
         optind--;
         u_ptr = malloc (MAXLOGIN);
         strncpy(u_ptr, argv[optind++], MAXLOGIN);
         users[usr_cnt++] = u_ptr;
         break;

      case 'c':
         eflg++;
         optind--;
         c_ptr = malloc (MAXEVNAME);
         strncpy(c_ptr, argv[optind++], MAXEVNAME);
         scnames[scname_cnt++] = c_ptr;
         break;

      case 'e':
         eflg++;
         optind--;
         e_ptr = malloc (MAXEVNAME);
         strncpy(e_ptr, argv[optind++], MAXEVNAME);
         evnames[evname_cnt++] = e_ptr;
         break;

      case 'p':
         pflg++;
         break;

      case 'f':
         fflg++;
         break;

      case 'l':
         lflg++;
         optind--;
         t_ptr = malloc(MAXTTYNAME);
         strncpy(t_ptr, argv[optind++], MAXTTYNAME);
         ttys[tty_cnt++] = t_ptr;
         break;

      case 't':
         tflg++;
         optind--;
         if ( ct (argv[optind++], &start) < 0 ) {
            error(E_AUDISP_START);
            SELF_AUD(1);
            exit(1);
         }
         break;

      case 's':
         sflg++;
         optind--;
         if ( ct (argv[optind++], &stop) < 0 ) {
            error(E_AUDISP_STOP);
            SELF_AUD(1);
            exit(1);
         }
         break;

      default:
         error(E_AUDISP_USE1);
         error(E_AUDISP_USE2);
         error(E_AUDISP_USE3);
         SELF_AUD(1);
         exit(1);
      }

   /* open and store the audit files */
   af_cnt = 0;
   while ((optind < argc) && (af_cnt < fds)) {
      if ((ret_val = get_cnode (argv[optind], aud_files,
                                &af_cnt, fds)) > 0) {
         /* too many audit files given */
         error (E_AUDISP_FNUM);
         SELF_AUD (8);
         exit (8);
      } else if (ret_val < 0) {
         /* argv[optind] is not a CDF file or element */
         if ((aud_files[af_cnt].fp =
              fopen (argv[optind], "r")) == NULL) {
            error(E_AUDISP_NOAF, argv[optind]);
            error(E_AUDISP_USE1);
            error(E_AUDISP_USE2);
            error(E_AUDISP_USE3);
            error(E_AUDISP_NOF, af_cnt);
            SELF_AUD(3);
            exit(3);
         }
         aud_files[af_cnt++].cname = NULL; /* regular file- no cname */
      }
      optind++;
   } /* end of while */

   if (pflg && fflg) {
      error(E_AUDISP_BOTHPF);
      SELF_AUD(1);
      exit(1);
   }

   if (uflg) {
      printf((catgets(nlmsg_fd,NL_SETN,26, "users and aids:\n")));
      convu (users, aids, usr_cnt, &aid_cnt);
      for (i = 0; i < usr_cnt; i++)
         printf("%s\n", users[i]);
      for (i = 0; i < aid_cnt; i++)
         printf("%d\n", aids[i]);
   } else {
      allusers++;
      printf((catgets(nlmsg_fd,NL_SETN,27, "All users are selected.\n")));
   }

   if (eflg) {
      printf((catgets(nlmsg_fd,NL_SETN,28, "Selected the following events:\n")));
      conve (evnames, scnames, events, evname_cnt, scname_cnt, &event_cnt);
      for (i = 0; i < evname_cnt; i++)
         printf("%s\n", evnames[i]);
      for (i = 0; i < event_cnt; printf("%ld ", events[i++]))
         ;
      CR();
   } else {
      allevents++;
      printf((catgets(nlmsg_fd,NL_SETN,29, "All events are selected.\n")));
   }

   if (lflg) {
      printf((catgets(nlmsg_fd,NL_SETN,30, "ttys and ttydevs:\n")));
      convt (ttys, ttydevs, tty_cnt, &ttyd_cnt);
      for (i = 0; i < tty_cnt; i++)
         printf("%s\n", ttys[i]);
      for (i = 0; i < ttyd_cnt; i++)
         printf("%d\t\t%x\n", major(ttydevs[i]),
         minor(ttydevs[i]));
   } else {
      allttys++;
      printf((catgets(nlmsg_fd,NL_SETN,31, "All ttys are selected.\n")));
   }

   if (pflg) {
      pf = 1;
      printf((catgets(nlmsg_fd,NL_SETN,32, "Selecting only successful events.\n")));
   } else if (fflg) {
      pf = 2;
      printf((catgets(nlmsg_fd,NL_SETN,33, "Selecting only failed events.\n")));
   } else {
      pf = 3;
      printf((catgets(nlmsg_fd,NL_SETN,34, "Selecting successful & failed events.\n")));
   }

   if (tflg && sflg) {
      if ( compare_tm (start, stop) < 0 ) {
         error(E_AUDISP_TIMES);
         SELF_AUD(1);
         exit(1);
      }
   };

   if (tflg) {
      printf((catgets(nlmsg_fd,NL_SETN,35, "start time :\n")));
      printf("%s\n", tmtoasc(start));
      given_start++;
   }

   if (sflg) {
      printf((catgets(nlmsg_fd,NL_SETN,36, "stop time :\n")));
      printf("%s\n", tmtoasc(stop));
      given_stop++;
   }

   printf(OUTPUTHEAD);
   init_aheads();
   printf(LINE);


/*
 * Translate and save date format string once, before entering loop:
 */

   dateformat = catgets(nlmsg_fd, NL_SETN, 86, "%02d%02d%02d %02d:%02d:%02d");

   dateformat = strcpy ((char *) malloc (strlen (dateformat) + 1), dateformat);

   while (next_ar (&ar_head, &ar_body) > 0) {
      if (ar_head.ah_event == EN_PIDWRITE) {
         continue;      /* no need to display pid identification record */
      } else {
         tp = localtime(&(ar_head.ah_time));
         printf(dateformat,
      tp->tm_year, tp->tm_mon + 1, tp->tm_mday,
      tp->tm_hour, tp->tm_min,     tp->tm_sec);
      }
      printf("%6d ", ar_head.ah_pid); 
      if (ar_head.ah_error) {
         printf((catgets(nlmsg_fd,NL_SETN,37, "F ")));
      } else {
         printf((catgets(nlmsg_fd,NL_SETN,38, "S ")));
      }
      printf("%6d ", ar_head.ah_event);

      /* print pir info */
      printf("%6d %6ld %6d %6d %6d %6d %s\n",
               curr_pir_node->info.ppid, curr_pir_node->info.aid,
               curr_pir_node->info.ruid, curr_pir_node->info.rgid,
               curr_pir_node->info.euid, curr_pir_node->info.egid,
               curr_pir_node->ttyname);

      is800vfsmount = ch800vfsmount(ar_head.ah_event, ar_head.ah_len);
      if (is800vfsmount)
	 ar_head.ah_event = VFSMOUNT800;

      /* print translated names */
      printf((catgets(nlmsg_fd,NL_SETN,88, "[ ")));
      printf((catgets(nlmsg_fd,NL_SETN,89, "Event=%s; ")),
               geten(ar_head.ah_event));
      printf((catgets(nlmsg_fd,NL_SETN,90, "User=%s; ")),
	       getun(curr_pir_node->info.aid));
      printf((catgets(nlmsg_fd,NL_SETN,91, "Real Grp=%s; ")),
	       getgn(curr_pir_node->info.rgid));
      printf((catgets(nlmsg_fd,NL_SETN,92, "Eff.Grp=%s; ")),
	       getgn(curr_pir_node->info.egid));
      /* print cnode if necessary */
      if (aud_files[curr_afind].cname[0] != 0)
         printf((catgets(nlmsg_fd,NL_SETN,93, "Origin=%s; ")),
		  aud_files[curr_afind].cname);
      printf((catgets(nlmsg_fd,NL_SETN,94, " ]")));
      CR();
      CR();
      if (ar_head.ah_event < EN_CREATE) {   /* system calls */
	 if (is800vfsmount) {
            syscall_info (vfsmount800, ar_body.scbody);
	 } else {
            syscall_info (audparam[ar_head.ah_event], ar_body.scbody);
	 }
      } else if ( (ar_head.ah_event == EN_IPC_GETMSG) ||
                  (ar_head.ah_event == EN_IPC_PUTMSG) ) {
         stream_info (ar_head.ah_len, ar_body.stbody);
      } else { /* other self-auditing records */
         printf((catgets(nlmsg_fd,NL_SETN,39, "SELF-AUDITING TEXT: ")));
         for (i=0; i < ar_head.ah_len; i++)
            printf("%c", ar_body.txbody[i]);
         CR();
      }
      printf(LINE);
   } /* while */

      SELF_AUD(0);
      return 0;
} /* audisp main */

/*

*/
/**********************************************************************
 * syscall_info prints the audit record body that contains system
 *              call parameters. af is the aud_flag structure which
 *              specifies what parameters are saved when this system
 *              call is audited. sc is the audit record body which
 *              contains the parameters and needs to be decoded with
 *              af.
 *********************************************************************/
syscall_info (af, sc)
struct aud_flag af;
char *sc;
{
   short i, j;
   int len;
   unsigned a_strlen;
   struct audit_sock asock;

   struct aud_fn {
      u_long apn_cnode;
      u_long apn_dev;
      u_long apn_inode;
      u_short apn_len;
   } afn;
   /*
    * long parameters are either a string(#4), socket address(#7)
    * or filename(#8).
    */
   char *long_params;
   int  pathnamelength;
   /*
    * short parameters are any other kinds of parameters
    */
   union ab_data *short_params, *lenptr;

   INDENT();
   short_params = (union ab_data *) sc;

   if (af.rtn_val1)
      printf((catgets(nlmsg_fd,NL_SETN,40, "RETURN_VALUE 1 = %d; ")), (short_params++)->ival);
   if (af.rtn_val2)
      printf((catgets(nlmsg_fd,NL_SETN,41, "RETURN_VALUE 2 = %d")), (short_params++)->ival);
   CR();

   /* position long_params pointer */
   long_params = (char *) short_params;
   for (i = 0; i < af.param_count; i++)
      switch (af.param[i]) {
      case 0:
         /* not saved */
         break;
      case 1:
      case 2:
      case 3:
      case 5:
      case 12:
      case 13:
         long_params += sizeof (union ab_data);
         break;
      case 6:
         /* integer array - length is stored in the first element
     * of the arry. Length doesn't include itself.
     */
         lenptr = (union ab_data *) long_params;
         len = lenptr->ival;
    /* skip len+1 integers */
         for (j=0; j < len+1; j++)
            long_params += sizeof (union ab_data);
         break;
      case 9:
         long_params += 3 * sizeof (union ab_data);
         break;
      case 10:
      case 11:
         long_params += 2 * sizeof (union ab_data);
         break;
      case 14:
         /* access control list */
         /* length given as the previous parameter */
         lenptr = (union ab_data *) (long_params - sizeof (union ab_data));
         len = lenptr->ival;
         for (j=0; j < len; j++)
            long_params += 3 * sizeof (union ab_data);
         break;
      case 15:
         /* 2 file descriptors (in return values) */
         long_params += 6 * sizeof (union ab_data);
         break;
      default:
         break;
      } /* position the long_params pointer */
      
   /* print the sys call parameters */
   for (i = 0; i < af.param_count; i++) {
      INDENT();
      switch (af.param[i]) {
      case 0:
         /* not saved */
         break;
      case 1:
         printf((catgets(nlmsg_fd,NL_SETN,42, "PARAM #%1d (int) = %ld\n")),
                  i+1, (short_params++)->ival);
         break;
      case 2:
         printf((catgets(nlmsg_fd,NL_SETN,43, "PARAM #%1d (unsigned long) = %lu\n")),
                  i+1, (short_params++)->ulval);
         break;
      case 3:
         printf((catgets(nlmsg_fd,NL_SETN,44, "PARAM #%1d (long) = %ld\n")),
                  i+1, (short_params++)->lval);
         break;
      case 4:
         printf((catgets(nlmsg_fd,NL_SETN,45, "PARAM #%1d (char string) = ")), i+1);
         memcpy ( (char *) &a_strlen, long_params, sizeof (unsigned) ); 
         long_params += sizeof (unsigned);
         for (j=0; j < a_strlen; j++)
            printf("%c", *(long_params++));
         CR();
         break;
      case 5:
         printf((catgets(nlmsg_fd,NL_SETN,46, "PARAM #%1d (int) = %lu\n")),
                  i+1, (short_params++)->ival);
         break;
      case 6:
         printf((catgets(nlmsg_fd,NL_SETN,47, "PARAM #%1d (int array) = ")), i+1);
         len = short_params->ival;
    short_params++;
         for (j=0; j < len; j++) {
            printf("%d ", short_params->ival);
            short_params++;
         }
         CR();
         break;
      case 7:
	{
		struct sockaddr_in *sock;
		char *ptr;

         	memcpy ( (char *) &asock, long_params, 
			sizeof (struct audit_sock) );
         	printf((catgets(nlmsg_fd,NL_SETN,48, 
			"PARAM #%1d (socket addr) = ")), i+1);

	 	if (asock.a_sock.sa_family == 2) {
			sock = (struct sockaddr_in *) &asock.a_sock;
			ptr = (char *)&(sock->sin_addr);
		printf((catgets(nlmsg_fd,NL_SETN,49, "%d (family); %d (port); %d.%d.%d.%d (addr)\n")),
			sock->sin_family,
			sock->sin_port,
			UC(ptr[0]), UC(ptr[1]), UC(ptr[2]), UC(ptr[3]));
		}
	 	else
			printf((catgets(nlmsg_fd,NL_SETN,49, "%d (addr family); %s\n")),
			asock.a_sock.sa_family, asock.a_sock.sa_data);
         	long_params += sizeof (struct audit_sock);
         	break;
	}
      case 8:
         memcpy ( (char *) &afn, long_params,
                  3 * sizeof (u_long) + sizeof (u_short) );
         printf((catgets(nlmsg_fd,NL_SETN,50, "PARAM #%1d (file path) = %lu (cnode);")),
                  i+1, afn.apn_cnode);
         CR();
         INDENT();
         printf((catgets(nlmsg_fd,NL_SETN,51, "                       0x%08.8x (dev);")),afn.apn_dev);
         CR();
         INDENT();
         printf((catgets(nlmsg_fd,NL_SETN,52, "                       %lu (inode);")),afn.apn_inode);
         CR();
         INDENT();
         printf((catgets(nlmsg_fd,NL_SETN,53, "         (path) = ")));
         long_params += 3 * sizeof (u_long) + sizeof (u_short);
    if ( (afn.apn_len > 0) &&
	 (*(long_params+afn.apn_len-1) == NULL) ) {
      pathnamelength = afn.apn_len-1;
    }
    else {
      pathnamelength = afn.apn_len;
    }
         for (j=0; j < pathnamelength; j++)
            printf("%c", *(long_params++));
         long_params += afn.apn_len - pathnamelength;
         CR();
         break;
      case 9:
         short_params++; /* skip cnode - not saved */
         printf((catgets(nlmsg_fd,NL_SETN,54, "PARAM #%1d (file desc) = %lu (idev);\n")),
                  i+1, (short_params++)->ulval);
         INDENT();
         printf((catgets(nlmsg_fd,NL_SETN,55, "                       %lu (inum)\n")),
                  (short_params++)->ulval);
         break;
      case 10:
         printf((catgets(nlmsg_fd,NL_SETN,56, "PARAM #%1d (time struct) = %lu (seconds); %lu (micro sec)\n")),
                  i+1, short_params->ulval, short_params->lval);
	 short_params++;
         break;
      case 11:
         printf((catgets(nlmsg_fd,NL_SETN,57, "PARAM #%1d (timezone) = %d (min west); %d (dst time)\n")),
                  i+1, short_params->ival, short_params->ival);
	 short_params++;
         break;
      case 12:
         printf((catgets(nlmsg_fd,NL_SETN,58, "PARAM #%1d (addr of char) = %lu\n")),
                  i+1, (short_params++)->ulval);
         break;
      case 13:
         printf((catgets(nlmsg_fd,NL_SETN,59, "PARAM #%1d (cast long ptr) = %lu\n")),
                  i+1, (short_params++)->ulval);
         break;
      case 14:
         printf((catgets(nlmsg_fd,NL_SETN,60, "PARAM #%1d (acl's) = ")), i+1);
         len = (short_params-1)->ival;
         if (len == 0) {
            printf((catgets(nlmsg_fd,NL_SETN,61, "no acl's recorded\n")));
            break;
         }
	 CR();
         for (j=0; j < len; j++) {
	    INDENT();
            INDENT();
            printf((catgets(nlmsg_fd,NL_SETN,62, "uid=%lu, ")), (short_params++)->lval);
            printf((catgets(nlmsg_fd,NL_SETN,63, "gid=%lu, ")), (short_params++)->lval);
            printf((catgets(nlmsg_fd,NL_SETN,64, "mode=%#o")), (short_params++)->ucval);
            CR();
         }
         break;
      case 15:
         printf((catgets(nlmsg_fd,NL_SETN,65, "PARAM #%1d (file desc's) =\n")), i+1);
         for (j=0; j < 2; j++) { /* 2 file descriptors */
            short_params++; /* skip cnode - not saved */
            INDENT();
            printf((catgets(nlmsg_fd,NL_SETN,66, "fd#%1d:   %lu (idev); %lu (inum)\n")), j+1,
                     short_params->ulval, short_params->ulval);
	    short_params++;
         }
         break;
      default:
         error(E_AUDISP_AFLG);
         break;
      } /* switch */
   } /* for */
} /* syscall_info */

/**********************************************************************
 * stream_info prints out the information in the record body which
 *             is caused by streams related operations.
 *********************************************************************/
stream_info (len, strm)
u_short len;
struct audit_str_data strm;
{
   u_short bdylen, i;

   bdylen = len - sizeof (struct audit_str_hdr);
   printf((catgets(nlmsg_fd,NL_SETN,67, "Stream Header {\n")));
   printf((catgets(nlmsg_fd,NL_SETN,68, "   pid = %d\n")), strm.str_hdr.pid);
   printf((catgets(nlmsg_fd,NL_SETN,69, "   ppid = %d\n")), strm.str_hdr.ppid);
   printf((catgets(nlmsg_fd,NL_SETN,70, "   aid = %ld\n")), strm.str_hdr.aid);
   printf((catgets(nlmsg_fd,NL_SETN,71, "   uid = %d\n")), strm.str_hdr.uid);
   printf((catgets(nlmsg_fd,NL_SETN,72, "   gid = %d\n")), strm.str_hdr.gid);
   printf((catgets(nlmsg_fd,NL_SETN,73, "   euid = %d\n")), strm.str_hdr.euid);
   printf((catgets(nlmsg_fd,NL_SETN,74, "   egid = %d\n")), strm.str_hdr.egid);
   printf((catgets(nlmsg_fd,NL_SETN,75, "   tty = %ld\n")), strm.str_hdr.tty);
   printf((catgets(nlmsg_fd,NL_SETN,76, "Stream Body =\n")));
   for (i=0; i < bdylen; i += sizeof (union ab_data))
      printf("%ld", strm.bdy_data.data[i]);
}

/**********************************************************************
 * tmtoasc takes a time buf in tm format (defined in time.h) and returns
 *         an ascii time string without day of week.
 *********************************************************************/

char *tmtoasc (tmbuf)
struct tm tmbuf;
{
   char *str;

   str = asctime(&tmbuf);
   str += 4;
   return (str);
}

/**********************************************************************
 * compare_tm takes 2 time bufs in tm format (defined in time.h) and
 *            returns:
 *
 *              0  - if they are equal.
 *              1  - if the first time buf is less than the second.
 *              -1 - if the first time buf is greater than the second.
 *********************************************************************/

int compare_tm (tm1, tm2)
struct tm tm1, tm2;
{
   if (tm1.tm_year < tm2.tm_year) {
      return(1);
   } else if (tm1.tm_year > tm2.tm_year) {
      return(-1);
   };

   if (tm1.tm_mon < tm2.tm_mon) {
      return(1);
   } else if (tm1.tm_mon > tm2.tm_mon) {
      return(-1);
   };

   if (tm1.tm_mday < tm2.tm_mday) {
      return(1);
   } else if (tm1.tm_mday > tm2.tm_mday) {
      return(-1);
   };

   if (tm1.tm_hour < tm2.tm_hour) {
      return(1);
   } else if (tm1.tm_hour > tm2.tm_hour) {
      return(-1);
   };

   if (tm1.tm_min < tm2.tm_min) {
      return(1);
   } else if (tm1.tm_min > tm2.tm_min) {
      return(-1);
   };

   return(0);
} /* compare_tm */

/**********************************************************************
 * ct converts a string of the form mmddhhmm[yy] (month,
 *    day, hour, minute, optional year) to the struct tm as
 *    defined in time.h. The year field in the string is
 *    optional. If not specified, it will be set to the year
 *    returned by gettimeofday() system call. Since the day of
 *    the week is not included in the string, the returned time
 *    buf does not contain a valid day of week field.
 *    If any of the fields in the string are not valid, for example
 *    the hh (hour) filed contains 25, ct returns -1; otherwise it
 *    returns 0.
 *    Note: the month field in the tm struct has the range 0 - 11.
 *          That is, Jan is 0, Feb is 1, so on. The input string
 *          is expected to have a more conventional representation.
 *          That is, Jan is 01, Feb is 02, so on. Thus, ct must do
 *          the adjustment.
 *********************************************************************/

int ct (time_str, time_buf_ptr)
char *time_str;
struct tm *time_buf_ptr;
{
   struct tm *curr_time_ptr;
   struct timeval t;
   struct timezone tz;
   char two_char[2];

   gettimeofday(&t, &tz);
   curr_time_ptr = localtime(&(t.tv_sec)); 
   strncpy(two_char, &time_str[0], 2);
   time_buf_ptr->tm_mon = atoi(two_char) - 1; /* the month is adjusted */
   strncpy(two_char, &time_str[2], 2);
   time_buf_ptr->tm_mday = atoi(two_char);
   strncpy(two_char, &time_str[4], 2);
   time_buf_ptr->tm_hour = atoi(two_char);
   strncpy(two_char, &time_str[6], 2);
   time_buf_ptr->tm_min = atoi(two_char);
   time_buf_ptr->tm_sec = 0;
   if (time_str[8] != '\0') {
      strncpy(two_char, &time_str[8], 2);
      time_buf_ptr->tm_year = atoi(two_char);
   } else {
      time_buf_ptr->tm_year = curr_time_ptr->tm_year;
   }
   time_buf_ptr->tm_isdst = curr_time_ptr->tm_isdst;
   /* check validity of the fields */
   if (time_buf_ptr->tm_mon < 0   ||
       time_buf_ptr->tm_mon > 11  ||
       time_buf_ptr->tm_mday < 1  ||
       time_buf_ptr->tm_mday > 31 ||
       time_buf_ptr->tm_hour < 0  ||
       time_buf_ptr->tm_hour > 23 ||
       time_buf_ptr->tm_min < 0   ||
       time_buf_ptr->tm_min > 59) {
       return(-1);
   } else {
       return(0);
   }
} /* ct */

/**********************************************************************
 * convu converts users, an array of user names, to the
 *       corresponding array aids. If some of the user names
 *       are not valid, they are ignored. But if none of the
 *       names are valid, the program exits.
 *********************************************************************/       
convu (users, aids, usr_cnt, aid_cnt)
char *users[];
long aids[];
short usr_cnt, *aid_cnt; /* number of users */
{
   struct s_passwd *spw;
   struct s_passwd *getspwent();
   int i = 0;
   int j = 0;
   int found = 0;

   while ( (spw = getspwent()) != NULL ) {
      for (i = 0; i < usr_cnt; i++) {
         if (strncmp(users[i], spw->pw_name, MAXLOGIN) == 0 ) {
            found++;
            aids[j++] = spw->pw_audid;
         }
      }
   };
   *aid_cnt = j;
   if (found == 0) {
      error(E_AUDISP_UNAME);
      SELF_AUD(1);
      exit(1);
   }
} /* convu */

/**********************************************************************
 * convt converts ttys, an array of ascii ttys names, to their
 *       device numbers (in an array of dev_t, ttydevs). If some
 *       of the ttys are not valid, they are ignored. But if
 *       none of the ttys are valid, the program exits.
 *********************************************************************/
convt (ttys, ttydevs, tty_cnt, ttyd_cnt_ptr)
char *ttys[];
dev_t *ttydevs;
short tty_cnt, *ttyd_cnt_ptr; /* number of ttys */
{

   struct stat sbuf1;
   struct direct *dp;
   DIR *dirp;
   int i;
   char temp[MAXTTYNAME];

   if ((dirp = opendir((catgets(nlmsg_fd,NL_SETN,77, "/dev")))) == NULL ) {
      error(E_AUDISP_NODEV);
      SELF_AUD(4);
      exit(4);
   }

   if (chdir((catgets(nlmsg_fd,NL_SETN,78, "/dev"))) < 0) {
      error(E_AUDISP_CHDEV);
      SELF_AUD(4);
      exit(4);
   }

   *ttyd_cnt_ptr = 0;
   for (dp = readdir(dirp); (dp != NULL) && (*ttyd_cnt_ptr < tty_cnt);
   dp=readdir(dirp)) {
      if (stat(dp->d_name, &sbuf1) < 0)
         continue;
      if (((sbuf1.st_mode&S_IFMT) != S_IFCHR)
            || (!strcmp(dp->d_name,(catgets(nlmsg_fd,NL_SETN,79, "syscon"))))
            || (!strcmp(dp->d_name, (catgets(nlmsg_fd,NL_SETN,80, "systty")))))
              continue;
      for (i = 0; i < tty_cnt; i++) {
         if ((strcmp(dp->d_name, ttys[i])) == 0) {
            ttydevs[*ttyd_cnt_ptr] = sbuf1.st_rdev;
            if (!tc_lookup(sbuf1.st_rdev,temp))
	    	tc_insert(sbuf1.st_rdev,dp->d_name);
            (*ttyd_cnt_ptr)++;
            break;
         }
      }
   }
   closedir(dirp);

   /* if we have already found all the ttys - leave */
   if (*ttyd_cnt_ptr >= tty_cnt) 
	goto convt_leave;

   /* Search the /dev/pty directory for any ttys that haven't already been 
    * found.
    */

   if ((dirp = opendir((catgets(nlmsg_fd,NL_SETN,97, "/dev/pty")))) == NULL ) {
      error(E_AUDISP_NODEV);
      SELF_AUD(4);
      exit(4);
   }

   if (chdir((catgets(nlmsg_fd,NL_SETN,98, "/dev/pty"))) < 0) {
      error(E_AUDISP_CHDEV);
      SELF_AUD(4);
      exit(4);
   }

   for (dp = readdir(dirp); (dp != NULL) && (*ttyd_cnt_ptr < tty_cnt);
   dp=readdir(dirp)) {
      if (stat(dp->d_name, &sbuf1) < 0)
         continue;
      if ((sbuf1.st_mode&S_IFMT) != S_IFCHR)
              continue;
      for (i = 0; i < tty_cnt; i++) {
         if ((strcmp(dp->d_name, ttys[i]) == 0)
	   && !tc_lookup(sbuf1.st_rdev, temp)) {
            ttydevs[*ttyd_cnt_ptr] = sbuf1.st_rdev;
            (*ttyd_cnt_ptr)++;
	    tc_insert(sbuf1.st_rdev,dp->d_name);
            break;
         }
      }
   }
   closedir(dirp);

   if (*ttyd_cnt_ptr == 0) {
      error(E_AUDISP_TTY);
      SELF_AUD(1);
      exit(1);
   }

convt_leave:

   /* search ttydevs and replace any references to the generic console
    * major 0  minor 0x00000 with the real system console value.
    */
   for (i=0; i < *ttyd_cnt_ptr; i++) { 
      if (ttydevs[i] == 0) 
         ttydevs[i]=console_dev;
   }

} /* convt */

/**********************************************************************
 * conve converts an array of event names to the corresponding
 *       array of event numbers. If some of the event names are
 *       not valid, they are ignored. But if none of the names are
 *       valid, the program exits.
 *
 * Note: for this routine to work, the inculde file "eventmap.h"
 *       must be present.
 *********************************************************************/

conve (enames, cnames, enums, ename_cnt, cname_cnt, enum_cnt)
char *enames[];
char *cnames[];
int enums[];
short ename_cnt;
short cname_cnt;
short *enum_cnt;
{
   register int i, j;

   /* go through the entire system call map
    * and event map. Do a translation whenever
    * there is a match of names.
    */

   *enum_cnt = 0;

   if (ename_cnt > 0)
      for (i=0; (i < EVMAPSIZE); i++) {
         for (j=0; j < ename_cnt; j++) {
            if (strcmp(enames[j], ev_map[i].ev_name) == 0)
               enums[(*enum_cnt)++] = ev_map[i].ev_num;
         }
      }

   if (cname_cnt > 0)
      for (i=0;   (i < SCMAPSIZE); i++) {
         for (j=0; j < cname_cnt; j++) {
            if (strcmp(cnames[j], sc_map[i].sc_name) == 0)
               enums[(*enum_cnt)++] = sc_map[i].sc_num;
         }
      }
   
   if (*enum_cnt <= 0) {
      error(E_AUDISP_ENAME);
      SELF_AUD(1);
      exit(1);
   } /* none of the event names are valid */

} /* conve */


/**********************************************************************
 * add_pn adds a pir (a pid identification record) node to the
 *        appropriate pir_tree;
 *********************************************************************/
add_pn (newpid, newpir)
u_short newpid;
struct pir_body *newpir;
{
   struct pir_node *curr_pnp;
   struct pir_node **trace;

   trace = &(pir_trees[curr_afind]);
   curr_pnp = pir_trees[curr_afind];

   while (curr_pnp != NULL) {
      if (curr_pnp->pid == newpid)
         break;
      if (curr_pnp->pid > newpid) {
         trace = &(curr_pnp->left);
         curr_pnp = curr_pnp->left;
      } else {
         trace = &(curr_pnp->right);
         curr_pnp = curr_pnp->right;
      }
   }   /* while */
   if (curr_pnp == NULL) {
      /* a new pir node; needs to be added */
      *trace = (struct pir_node *) malloc (sizeof (struct pir_node));
      (*trace)->pid = newpid;
      (*trace)->left = NULL;
      (*trace)->right = NULL;
      (*trace)->ttyname = malloc (MAXTTYNAME);
   }

   /* note that an existing pir will be overwritten. */
   (*trace)->info.ppid = newpir->ppid;
   (*trace)->info.aid = newpir->aid;
   (*trace)->info.ruid = newpir->ruid;
   (*trace)->info.rgid = newpir->rgid;
   (*trace)->info.euid = newpir->euid;
   (*trace)->info.egid = newpir->egid;
   (*trace)->info.tty = newpir->tty;
   dev_to_name (newpir->tty, (*trace)->ttyname);

} /* add_pn */

/**********************************************************************
 * get_pn gets the pir node for the pid from the appropriate pir tree.
 *        and points curr_pir_node to it. Note that if the search is
 *        unsuccessful, curr_pir_node is set to NULL.
 **********************************************************************/
get_pn (the_pid)
u_short the_pid;
{

   curr_pir_node = pir_trees[curr_afind];
   while (curr_pir_node != NULL) {
      if (curr_pir_node->pid == the_pid)
         break;
      else
         curr_pir_node = (curr_pir_node->pid > the_pid) ?
                          curr_pir_node->left : curr_pir_node->right;
   }   /* while */
}

/**********************************************************************
 * next_ar searches for the next record to be displayed. If the search
 *         is successful, it returns a 1 (and the header is returned in
 *         hp and the body in bp). If no more records are available,
 *         it returns a 0.
 **********************************************************************/
next_ar (hp, bp)
struct audit_hdr *hp;
union mixed_body *bp;
{
   short errflg;
   short gotone = 0;
   FILE *aud_fp;

   while (next_ah (hp) > 0) { /* get the next audit header */
      errflg = 0;
      aud_fp = aud_files[curr_afind].fp;
      if (hp->ah_event < EN_CREATE) { /* system calls */
         if (hp->ah_len <= MAX_SCALL_BODY) {
            if ((bp->scbody = malloc (hp->ah_len)) != NULL) {
               fread ((void *) (bp->scbody), hp->ah_len, 1, aud_fp);
            } else {
               error(E_AUDISP_ALLOC);
               SELF_AUD(6);
               exit(6);
            }
         } else {
            errflg++;
         }
      } else if (hp->ah_event == EN_PIDWRITE) {
         if (hp->ah_len == sizeof(bp->pibody)) {
            /* s800 records */
            fread ((void *) &(bp->pibody), hp->ah_len, 1, aud_fp);
            add_pn (hp->ah_pid, &(bp->pibody));
         } else if (hp->ah_len ==
                   (sizeof(bp->pibody) - sizeof(short))) {
            /* s300 records */
            fread ((void *) &(bp->pibody), sizeof(short), 1, aud_fp);
            fread ((void *) &(bp->pibody.aid), sizeof(aid_t) +
                  4 * sizeof(u_short) + sizeof(dev_t), 1, aud_fp);
            add_pn (hp->ah_pid, &(bp->pibody));
         } else {
            errflg++;
         }
      } else if ( (hp->ah_event == EN_IPC_GETMSG) ||
                  (hp->ah_event == EN_IPC_PUTMSG) ) {
         if (hp->ah_len <= sizeof(bp->stbody)) {
            fread ((void *) &(bp->stbody), hp->ah_len, 1, aud_fp);
         } else {
            errflg++;
         }
      } else {   /* other self-auditing records */
         if (hp->ah_len <= sizeof(bp->txbody)) {
            fread ((void *) (bp->txbody), hp->ah_len, 1, aud_fp);
         } else {
            errflg++;
         }
      };
      if (errflg) {
         printf(E_AUDISP_BADBD);
         SELF_AUD(5);
         exit(5);
      }
      /* fill the audit header that got displayed */
      if (fread ((void *) aud_heads[curr_afind], sizeof(*hp), 1, aud_fp) <= 0) {
         /* that file is ended */
         fclose (aud_fp);
         free ((char *) aud_heads[curr_afind]);
         aud_heads[curr_afind] = NULL;
         aud_files[curr_afind].fp = NULL;
      }
      /* see if this record should be displayed or not */
      if (should_display(hp)) {
         gotone++;
         break;
      }
   } /* while next_ah > 0 */
   return (gotone);

} /* next_ar */

/**********************************************************************
 * init_aheads initializes the aud_heads array by reading a header
 *             from each specified audit file.
 **********************************************************************/
init_aheads()
{
   short i;

   for (i=0; i < af_cnt; i++) {
      aud_heads[i] = (struct audit_hdr *) malloc (sizeof (struct audit_hdr));
      if (fread ((void *) aud_heads[i], sizeof (struct audit_hdr),
         1, aud_files[i].fp) <= 0) {
         free ((char *) aud_heads[i]);
         aud_heads[i] = NULL;
      } /* if fread <= 0 */
   } /* for */
} /* init_aheads */

/**********************************************************************
 * next_ah compares the audit headers in aud_heads, picks one that is
 *         the earliest in time, returns that audit header in hp. It
 *         also sets the curr_afind, the index to the current audit
 *           file.
 *         If any header is returned, next_ah returns 1; else if no
 *         records are available from any of the audit files it
 *         returns a 0.
 **********************************************************************/
next_ah (hp)
struct audit_hdr *hp;
{
   u_long mintime;
   short minind;
   short i;

   mintime = ~0; /* fill mintime with all 1's */
   for (i=0; i < af_cnt; i++) {
      if (aud_heads[i] != NULL) {
         if (aud_heads[i]->ah_event == EN_PIDWRITE) {
            /* any pir is returned right away;
             *   no sense comparing time.
             */
            mintime = 0;
            minind = i;
            break;
         } else if (aud_heads[i]->ah_time < mintime) {
            mintime = aud_heads[i]->ah_time;
            minind = i;
         }
      }
   }

   if (mintime != ~0) {
      hp->ah_time = aud_heads[minind]->ah_time;
      hp->ah_pid = aud_heads[minind]->ah_pid;
      hp->ah_error = aud_heads[minind]->ah_error;
      hp->ah_event = aud_heads[minind]->ah_event;
      hp->ah_len = aud_heads[minind]->ah_len;
      curr_afind = minind;
      return (1);
   } else
      return (0);
} /* next_ah */

/**********************************************************************
 * should_display returns 1 if the audit record should be displayed;
 *                returns 0 otherwise. The decision is based on the
 *                the user, time, event, success/failure of the event,
 *                and tty id of the record and the corresponding
 *                parameters specified by the user.
 *                A search through the appropriate pir tree, which
 *                is a binary search tree, is required to find the
 *                corresponding pir node.
 *                If a search of the pir node is successful, curr_pir_node
 *                is set to point to that node. Else, there is some in-
 *                consistency in the audit file or the pir tree, and the
 *                program aborts.
 **********************************************************************/
should_display (hp)
struct audit_hdr *hp;
{
   struct tm *tp;
   short i;
   int ec; /* event class */

   /* Compare with any specified parameters to see if we should
    * display the record. As soon as we find any paramters not
    * matching the specified values (IF the parameters are specified),
    * don't display the record.
    *
    * The comparisons are done in such an order that the ones easily
    * comparable and more likely to be specified are checked first.
    */

   /* is it within specified time range ? */
   tp = localtime( &(hp->ah_time) );
   if (given_start)
      /* check against start time */
      if ( compare_tm (start, *tp) < 0 )
         goto Dont_Display;

   if (given_stop) {
      /* check against stop time */
      if ( compare_tm (stop, *tp) > 0 )
         goto Dont_Display;
   }

   if (pf == 1)
      /* only want successful events */
      if ( hp->ah_error != 0 )
         goto Dont_Display;

   if (pf == 2)
      /* only want failed events */
      if ( hp->ah_error == 0 )
         goto Dont_Display;

   if (!allevents ) {
      /* check if we want to display info about this event */
      if (ch800vfsmount(hp->ah_event, hp->ah_len)) {
         /* special case for vfsmount on s800 */
         for (i=0; i < event_cnt; i++)
	    if ((events[i] == VFSMOUNT800) ||
		(events[i] == EN_REMOVABLE))
               break;
      } else {
         ec = event_class(hp->ah_event);
         for (i=0; i < event_cnt; i++)
            if ((hp->ah_event == events[i]) || (ec == events[i]))
               break;
      }
      if (i >= event_cnt)
         goto Dont_Display;
   }

   get_pn (hp->ah_pid); /* points curr_pir_node to the right pir node. */
   if (curr_pir_node == NULL) {
      error(E_AUDISP_PT);
      SELF_AUD(7);
      exit (7);
   }

   if (!allttys) {
      /* check if we want to display info about this tty */
      for (i=0; i < ttyd_cnt; i++)
         if (curr_pir_node->info.tty == ttydevs[i])
            break;
      if (i >= ttyd_cnt)
         goto Dont_Display;
   }

   if (!allusers) {
      /* check if we want to display info about this user */
      for (i=0; i < aid_cnt; i++)
         if (curr_pir_node->info.aid == aids[i])
            break;
      if (i >= aid_cnt)
         goto Dont_Display;
   }

   return (1);

Dont_Display:
   return (0);
}

/**********************************************************************
 * event_class returns the event class of an event. For self-auditing
 *             events, the event class is simply the 6 leftmost bits;
 *             For system calls, we have to search through the sc_map
 *             table, which is in eventmap.h, to find the corresponding
 *             event class.
 *********************************************************************/

event_class (ev)
unsigned short ev;
{
   int index, low, high;   /* for doing binary search */
   int ec;

   ec = ev & ~01777;   /* get the 6 leftmost bits */
   if (ec == 0) {   /* it is a system call */
      low = 0;
      high = scmapsize - 1;
      index = high / 2;
      while (low < high) {
         if (sc_map[index].sc_num == ev) {
            break;
         } else if (sc_map[index].sc_num < ev) {
            low = index + 1;
         } else {
            high = index - 1;
         }
         index = (high + low) / 2;
      }
      if (sc_map[index].sc_num == ev)
         ec = sc_map[index].ev_class;
   }
   return (ec);
} /* event_class */

/**********************************************************************
 * dev_to_name takes a tty device number and returns the tty name if
 *             /dev has that tty. If the search is not successful, it
 *             returns NULL.
 *********************************************************************/
dev_to_name (dev, ttyname)
dev_t dev;
char *ttyname;
{
   struct stat sbuf1;
   struct direct *dp;
   DIR *dirp;
   char *tmp_ttyname;

   /* Search the tty cache first */

   if (tc_lookup(dev, ttyname)) 
      return; 


   /* Search /dev for ttys */

   if ((dirp = opendir((catgets(nlmsg_fd,NL_SETN,81, "/dev")))) == NULL) {
      error(E_AUDISP_NODEV);
      SELF_AUD(4);
      exit(4);
   }

   if (chdir((catgets(nlmsg_fd,NL_SETN,82, "/dev"))) < 0) {
      error(E_AUDISP_CHDEV);
      SELF_AUD(4);
      exit(4);
   }

   for (dp = readdir(dirp); (dp != NULL); dp=readdir(dirp)) {
      if (stat(dp->d_name, &sbuf1) < 0)
         continue;
      if (((sbuf1.st_mode&S_IFMT) != S_IFCHR)
            || (!strcmp(dp->d_name,(catgets(nlmsg_fd,NL_SETN,83, "syscon"))))
            || (!strcmp(dp->d_name, (catgets(nlmsg_fd,NL_SETN,84, "systty")))))
               continue;
      if (dev == sbuf1.st_rdev) {
         closedir(dirp);
         strcpy (ttyname, dp->d_name);
	 tc_insert(dev,ttyname);
         return;
      }
   }
   closedir(dirp);


   /* Search /dev/pty for ptys */

   if ((dirp = opendir((catgets(nlmsg_fd,NL_SETN,95, "/dev/pty")))) == NULL) {
      error(E_AUDISP_NODEV);
      SELF_AUD(4);
      exit(4);
   }

   if (chdir((catgets(nlmsg_fd,NL_SETN,96, "/dev/pty"))) < 0) {
      error(E_AUDISP_CHDEV);
      SELF_AUD(4);
      exit(4);
   }

   for (dp = readdir(dirp); (dp != NULL); dp=readdir(dirp)) {
      if (stat(dp->d_name, &sbuf1) < 0)
         continue;
      if ((sbuf1.st_mode&S_IFMT) != S_IFCHR)
               continue;
      if (dev == sbuf1.st_rdev) {
         closedir(dirp);
         strcpy (ttyname, dp->d_name);
	 tc_insert(dev,ttyname);
         return;
      }
   }
   closedir(dirp);

   /* if no matching device file is found output...
    * console       if the device is the system console
    * ?????         if the device is unknown (-1)
    *  major:minor  if the device is known, but doesn't match anything
    */
   if (dev == console_dev) 
   	strcpy (ttyname, (catgets(nlmsg_fd,NL_SETN,85, "console")));
   else	 {
	if (dev == -1) 
	   strcpy (ttyname, (catgets(nlmsg_fd,NL_SETN,85, "?????")));	
	else {
	   char console_str_dev[13];
	   sprintf(console_str_dev,"%3.3d:0x%06.6x",major(dev),minor(dev)); 
   	   strcpy (ttyname, (catgets(nlmsg_fd,NL_SETN,85, console_str_dev)));
	}
   }	
}


/**********************************************************************
 * tc_insert - insert an entry in the tty cache.
 * the cache is a LIFO linklist starting with the entry at tty_cach_head
 * and ending with an entry with a nextp == 0.
 **********************************************************************/

tc_insert(dev,ttyname)
dev_t dev;
char *ttyname;
{

    tc_entry_t *new_ptr;

    if ((new_ptr = (tc_entry_t *)malloc(sizeof(tc_entry_t)))==(tc_entry_t*) 0){
      error(E_AUDISP_ALLOC);
      SELF_AUD(6);
      exit(6);
    }

    new_ptr->dev     = dev;
    strcpy(new_ptr->ttyname, ttyname);
    new_ptr->nextp   = tty_cache_head;

    tty_cache_head   = new_ptr;

}


/**********************************************************************
 * tc_lookup - find the dev device in the tty cache
 * since the only ordering is LIFO, we just look through all
 * entries until we find one that matches.
 *
 * if a match is found tc_lookup returns 1 and updates ttyname
 * otherwise tclookup() returns 0 and leaves ttyname unchanged.
 **********************************************************************/

tc_lookup(dev,ttyname)
dev_t dev;
char *ttyname;
{

    tc_entry_t *curr_entry;

    for (curr_entry=tty_cache_head; curr_entry != 0;
      curr_entry=curr_entry->nextp)
        if (dev == curr_entry->dev)
	    break;

    if (curr_entry == (tc_entry_t *) 0)
        return 0; 

    strcpy(ttyname, curr_entry->ttyname);
    return 1;

}

/*******************************************************************************
 * error - a routine to flush stdout before printing to stdout.
 ******************************************************************************/

#include <varargs.h>

int error(format, va_alist)
char *format;
va_dcl
{
        va_list ap;

        va_start(ap);
        fflush(stdout);
        _doprnt(format, ap, stderr);
        va_end(ap);
        return(ferror(stderr) ? (EOF) : (0));
}


/*******************************************************************************
 *                           G E T _ C N O D E
 *
 * Given a CDF filename, returns the file pointers and the names
 * of the cnodes in cn_array,
 * that is, in cn_array[*ind], cn_array[*ind+1], ...
 *
 * The number of file pointers found is indicated by the difference of the
 * values of ind before and after the call.
 *
 * If fname is a CDF element already, its file pointer is put in cn_array
 * and ind is incremented by one.
 *
 * Ind should not be greater than maxind.
 *
 * Return values:
 *   0  if everything is ok;
 *   1  if ind exceeds maxind;
 *   -1 if the filename is not a CDF directory or element.
 ******************************************************************************/
get_cnode (fname, cn_array, ind, maxind)
char *fname;
struct file_cn cn_array[];
int *ind;
int maxind;
{
  char fn_plus[MAXPATHLEN+1];
  char dir[MAXPATHLEN];
  struct stat buf;
  DIR *opendir();
  DIR *dirp;
  struct direct *dp;
  char base[20];
  char cn_fullpn[MAXPATHLEN];

  if (*ind >= maxind)
    return (1);

  strcpy (fn_plus, fname);
  strcat (fn_plus, "+");

  dirp = NULL;
  if (stat(fn_plus, &buf) == 0) {
    dirp = opendir (fn_plus);
    strcpy (dir, fn_plus);
  } else {
    stat (fname, &buf);
    if (ISCDF(buf.st_mode)) {
      dirp = opendir (fname);
      strcpy (dir, fname);
    }
  }
  if (dirp != NULL) {
    /*
     * get all the CDF elements.
     */
    dp = readdir (dirp); /* skip . and .. */
    dp = readdir (dirp);

    for (dp = readdir(dirp);
         (dp != NULL) && (*ind < maxind);
         dp = readdir(dirp)) {
      strcpy (cn_fullpn, dir);
      strcat (cn_fullpn, "/");
      strcat (cn_fullpn, dp->d_name);
      if ((cn_array[*ind].fp = fopen (cn_fullpn, "r")) == NULL) {
         error (E_AUDISP_NOAF, cn_fullpn);
         error (E_AUDISP_USE1);
         error (E_AUDISP_USE2);
         error (E_AUDISP_USE3);
         SELF_AUD(3);
         exit(3);
      }
      cn_array[*ind].cname =
        strcpy ((char *) malloc (strlen (dp->d_name) + 1), dp->d_name);
      (*ind)++;
    }
    if (*ind >= maxind) {
      return (1);
    } else {
      return (0);
    }
  }

  strcpy (cn_fullpn, fname);
  dir_base (cn_fullpn, base); /* cn_fullpn becomes the prefix (parent) */
  stat (cn_fullpn, &buf);
  if (ISCDF(buf.st_mode)) {
    /*
     * if the directory it is in is a CDF directory,
     * fname is an element.
     * store the base name as the cnode.
     */
    if ((cn_array[*ind].fp = fopen (fname, "r")) == NULL) {
       error (E_AUDISP_NOAF, cn_fullpn);
       error (E_AUDISP_USE1);
       error (E_AUDISP_USE2);
       error (E_AUDISP_USE3);
       SELF_AUD(3);
       exit(3);
    }
    cn_array[*ind].cname =
       strcpy ((char *) malloc (strlen (base) + 1), base);
    (*ind)++;
    return (0);
  }
  /* NOT a CDF directory or element */
  return(-1);
}

/*******************************************************************************
 *                           D I R _ B A S E
 *
 * Given a pathname in pn, separates the last level from the prefix.
 * Note that pn becomes the prefix as a side-effect.
 ******************************************************************************/
dir_base(pn, base)
char *pn;
char *base;
{
  register char *ptr, *dir, *savep, *nextc;

  nextc = savep = ptr = dir = pn;
  while (*ptr) {
    nextc = (ptr + 1);
    if(((*ptr == '/') && (*nextc != '/')) && (*nextc != '\0'))
      savep = ptr;
    ptr = nextc;  
  }
  if (savep == dir) {
    if (*savep == '/') {
      strcpy (base, savep+1);
    } else {
      strcpy (base, savep);
      *savep = '.';
    }
    savep++;
  } else {
    strcpy (base, savep+1);
  }

  *savep = '\0';
}

/*******************************************************************************
 *                C H 8 0 0 V F S M O U N T
 * -checks to see if the syscall is vfs_mount on 800;
 * -if so, returns 1; else 0.
 * -this is done to resolve 300/800 syscall difference.
 *  for detailed explanation, refer to header comments in eventmap.h
 ******************************************************************************/
ch800vfsmount(scnum, body_len)
u_short scnum;
u_short body_len;
{
   if ( ( scnum == 198 ) && ( body_len > 8 ) ) {
      /* this is a vfs_mount from 800 */
      return (1);
   } else {
      return (0);
   }
}
