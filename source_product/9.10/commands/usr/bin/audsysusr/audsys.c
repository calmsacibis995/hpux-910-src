static char *HPUX_ID = "@(#) $Revision: 64.15 $";
/*******************************************************************************


       audsys - turn the auditing system on or off

       SYNOPSIS

            audsys [ -n ] [ -c file -s kbytes ] [ -x file -z kbytes ]

	    audsys [ -f ]

       DESCRIPTION

            Audsys turns the auditing system on or off.
            Without arguments, audsys displays the status of the
            auditing system; status information includes if the
            auditing is on or off, the current and next audit file
            names and their file limits.  This command is
	    restricted to users with appropriate privilege.

            Audsys also allows for the specification of which file
            the auditing system should write.  The "current file"
            is the file to which the system writes the auditing
            information.  The "next file" is maintained as backup
            to the current file.  When the current file is full,
            the auditing system switches to write to the next file.
            The next file should be on another file system.

              -n        Turn on the auditing system.  The system
                        uses the  existing current and next audit
                        files unless specified.  Audsys uses the
                        existing current file and next file.

              -f        Turn off the auditing system.  The -f and
                        -n options are mutually exclusive; they
                        cannot both be specified.

              -c        Specify a current file.  If one already
                        exists, the file specified with the -c
                        option replaces the existing current file;
                        the auditing system immediately switches to
                        write to the specified file. The specified
			file must be empty or non-existent.

              -s        Specify the the switch size for the current file in
                        kbytes.  When the audit system fills
			the current file to this size, it will
			switch to the next file, if one is available.

              -x        Specify the next file.  If one already
                        exists, the file specified with the -x
                        option replaces the existing next file.
			The specified file must be empty or
			non-existent.

              -z        Specify the switch size for next file in
                        kbytes.  When the audit system fills
			the next file to this size, it will issue
			messages to the console indicating this
			condition.

                        If -c but not -x option is specified, only
                        the current audit file is switched to the
                        indicated one; the existing next audit file
                        remains.  If -x but not -c option is
                        specified, only the next audit file is
                        switched to the indicated one; the existing
                        current audit file remains.

			If -c and/or -x options are specified which
			cause the same current and next file to be
			specified.  The system will switch to the
			specified current file with no backup or
			switch to the next file with no backup 
			depending upon whether -c or -x were selected.




	EXIT CODES:

	0  - complete successfully.
	1  - wrong usage or option.
	2  - not previleged user.
	3  - 
	4  - cannot lock control file /.secure/etc/audnames.
	5  - cannot suspend auditing.
	6  - bad control file- either inaccessable or bad format.
	8  - user attempt to turn on auditing when it is already on.
	9  - user attempt to change current or next file while turning
	     off auditing.
	10 - user attempt to turn off auditing when it is already off. 
	11 - user supplied audit file is not an empty regular file.
	12 - the system is using an unknown audit file.
	13 - cannot create user specified audit file.
	14 - unknown internal error.
	15 - audit file size has grown to greater than afs
	16 - insufficient space on the filesystem for the audit file specified.
	17 - audit file specified is on a partition which exceeds minfree.
	18 - audit file doesn't exist

******************************************************************************/

#include <stdio.h>
#include <sys/audit.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ndir.h>
#include <signal.h>
#include <string.h>
#include "aud_def.h"

#ifdef NLS
#include <nl_types.h>
#define NL_SETN=1
#include "nl_aud_err.h"
#include "nl_aud_msg.h"
#else
#include "aud_err.h"
#include "aud_msg.h"
#endif

#ifdef NLS
    nl_catd catd;
#endif

#define LEADING_SLASH(s)	( *(s) == '/' ) 
#define CVAST(s)		( strcmp( s ,AST) ? (s) : 0 ) 
#define PVAST(s)		( strcmp( s ,AST) ? (s) : "<null>" ) 
#define ISREG(mode)	        ( ((mode) & S_IFMT) == S_IFREG )
#define ISDIR(mode)             ( ((mode) & S_IFMT) == S_IFDIR )
#define ISCDF(mode)             ( ISDIR(mode) && ( (mode) & S_ISUID ) )
#define BTOKB(s)		( (int)((double)(s)/(double)1024 + (double).5))
#define AF_PCENT(afsz,afs)	(\
    (int)((((double)1-(double)(afsz)/(double)(afs))*(double)100) + (double).5))

#define MODE			0600	/* mode to assign audfiles */
#define MAXCSZ			(2*MAXPATHLEN+24)
#define VAFS			1
#define NVAFS			0

struct sigvec vec_ignore = {SIG_IGN, 0, 0};
extern int errno;

/* control file descriptor - no_cntl flag set cntl file initially did not exist
 */
int fd, no_cntl=0;

typedef enum {GOOD, U_NEXT, RGOOD, RU_NEXT, U_CURR, U_BOTH} aud_state_type; 
typedef enum {EMPTY, USED} mk_audfile_type; 

static int get_cntl();
static void mk_audfile();
static void disp_stats();
static void write_audrec();
static void aud_exit();

main(argc, argv)
int argc;
char *argv[];

{
    /* info from command line entry */
    extern char *optarg;
    extern int optind;
    int c, errflg=0;
    int nflg=0, fflg=0, cflg=0, xflg=0, sflg=0, zflg=0;
    int cmax, xmax; 
    char *acmax, *axmax;
    char cfile[MAXPATHLEN], xfile[MAXPATHLEN];
    char com_line_msg[9+2*(MAXPATHLEN+14)];

    /* info from and to the control file */
    char cntl_cbuff[MAXPATHLEN], cntl_xbuff[MAXPATHLEN];
    char *cntl_cfile=cntl_cbuff, *cntl_xfile=cntl_xbuff;
    int cntl_cmax, cntl_xmax;
    extern int fd; /* cntl file descriptor */
    extern int no_cntl; /* set no_cntl indicates initially no_cntl existed */

    /* info from and to the kernel */
    char sys_cbuff[MAXPATHLEN], sys_xbuff[MAXPATHLEN]; /* for audctl to fill */
    char *sys_cfile=sys_cbuff, *sys_xfile=sys_xbuff;   /* to audctl */
    char audbody[MAX_AUD_TEXT];

    /* audsys states */
    aud_state_type aud_state;

    /* special flags to modify and over-ride states
     * aud_on indicates auditing is on
     * bad_cntl specifies bad control file over-ride
     * ceqx specifies the current and next files are the same over-ride
     * ceqc specifies the current is reset to the same file with different AFS
     * xeqx specifies the current is reset to the same file with different AFS
     */
    int aud_on=0, bad_cntl=0, ceqx=0, ceqc=0, xeqx=0, creat_c=0, creat_x=0;



#ifdef NLS

    /* open message catalog */
    catd = catopen("audsysusr",0);

#endif



    /* Verify access and do self auditing */

    if (audswitch(AUD_SUSPEND) == -1) {
	fprintf(stderr, E_SUSER);
    	aud_exit(2);
    }


    /* get arguments */

    while ((c=getopt(argc, argv, "nfc:s:x:z:")) != EOF)
        switch (c) {
        case 'n':
            nflg++;
            break;
        case 'f':
            fflg++;
            break;
        case 'c':
            cflg++;
            if (LEADING_SLASH(optarg))
                strcpy (cfile, optarg);
            else {
                 /* if the path is not fully specified - generate it */
                getcwd (cfile, MAXPATHLEN);
		strcat (cfile, "/");
                strcat (cfile, optarg);
            }
            break;
        case 's':
            sflg++;
            cmax=atoi(optarg);
	    acmax=optarg;
            break;
        case 'x':
            xflg++;
            if (LEADING_SLASH(optarg))
                strcpy (xfile, optarg);
            else {
                 /* if the path is not fully specified - generate it */
                getcwd(xfile, MAXPATHLEN);
		strcat (xfile, "/");
                strcat (xfile, optarg);
            }
            break;
        case 'z':
            zflg++;
            xmax=atoi(optarg);
	    axmax=optarg;
            break;
        default:
            errflg++;
        }

	if (errflg || optind < argc) {
	    if (optind < argc) 
		fprintf(stderr,"%s: %s -- %s\n",argv[0],
		  E_ILLEGAL,argv[optind]);
	    fprintf (stderr, E_AUDSYS_USE);
            aud_exit(1);
	    
        }

    /* validate the input options */

    if (nflg && fflg) {
        fprintf (stderr, E_NFFLG);
        aud_exit(1);
    }

    if (cflg&&(!sflg)) {
        fprintf (stderr, E_SWDFLG);
        aud_exit(1);
    }

    if (xflg&&(!zflg)) {
        fprintf (stderr, E_ZGRFLG);
        aud_exit(1);
    }

    /* assemble command line message com_line_msg for display */
    strcpy(com_line_msg,"audsys");
    if (argc==1)
        strcat(com_line_msg," <noargs>\n");
    else {
        if (nflg)
            strcat(com_line_msg," -n");
        if (fflg)
            strcat(com_line_msg," -f");
        if (cflg) {
	    strcat(com_line_msg," -c ");
	    strcat(com_line_msg,cfile);
	    strcat(com_line_msg," -s ");
	    strcat(com_line_msg,acmax);
        }
        if (xflg) {
	    strcat(com_line_msg," -x ");
	    strcat(com_line_msg,xfile);
	    strcat(com_line_msg," -z ");
	    strcat(com_line_msg,axmax);
        }
        if (sflg && (!cflg)) {
	    strcat(com_line_msg," -s ");
	    strcat(com_line_msg,acmax);
        }
        if (zflg && (!xflg)) {
	    strcat(com_line_msg," -z ");
	    strcat(com_line_msg,axmax);
        }
        strcat(com_line_msg,"\n");
    }

    strcpy(audbody,com_line_msg);

    /* ignore signals which may interfere
     * once we read from the control file we are committed to 
     * continuing
     */
    sigvector(SIGHUP, &vec_ignore, 0);
    sigvector(SIGINT, &vec_ignore, 0);
    sigvector(SIGQUIT, &vec_ignore, 0);
    sigvector(SIGTERM, &vec_ignore, 0);

    /* open the control file with enforced locking: 
     * If the file doesnt exist go ahead... we will create it later.
     *
     * It is intentional that the file is locked before the kernel is
     * updated; the file is used as a semaphore to let anybody else who
     * cares know that we are in the process of modifying the file and
     * the kernel.
     */
     
    if ((((fd = open(CNTL_FILE, O_RDWR, 02600)) < 0 ) ||
	  lockf(fd,F_LOCK,0)) && errno!=ENOENT) {
          fprintf (stderr, E_CNTLF_LK);
	  strcat(audbody,E_CNTLF_MSG);
	  write_audrec(audbody,1);
          aud_exit(4);
    }


    /* if audit system is on - get the kernel current and next file.
     * and set aud_on accordingly
     */
    if (audctl(AUD_GET,sys_cbuff,sys_xbuff,0)==0) {
	aud_on=1;
    } else {
	switch (errno) {
	case ENOENT:
	    aud_on=1;
	    sys_xfile=AST;
	    break;
        case EALREADY:
            aud_on=0; 
	    *sys_cfile=0;
	    *sys_xfile=0;
	    break;
        default:
	    fprintf (stderr, E_INTNAL);
	    strcat(audbody,E_INTNAL_MSG);
	    write_audrec(audbody,1);
	    aud_exit(14);
        }
    }

    /* get the control file current and next files 
     *
     * if somehow we cannot read the control file set aud_state to
     * U_BOTH and GOTO and BYPASS the state determining logic; all of which
     * relies on accurate data from the control file
     */

    if (fd < 0) { 
	/* if the control file didn't exist before- create it now */
        /* !create! the control file with enforced locking */
        if (((fd = open(CNTL_FILE, O_RDWR|O_CREAT, 02600)) < 0 ) ||
	      lockf(fd,F_LOCK,0)) {
            fprintf (stderr, E_CNTLF_LK);
            aud_exit(4);
        }
	bad_cntl=1;
	no_cntl=1;
        fprintf (stderr,E_CNTLF_EXIST);
	strcat(audbody,E_CNTL_EXMSG);
	aud_state=U_BOTH;
	goto BYPASS;
    }

    if (bad_cntl=get_cntl(cntl_cbuff, &cntl_cmax,
                                cntl_xbuff, &cntl_xmax)) {
	/* bad control file */
	*cntl_cbuff=0;
	*cntl_xbuff=0;
	cntl_cmax=0;
	cntl_xmax=0;
        fprintf (stderr, E_CNTLF_FMT);
	strcat(audbody,E_CNTL_FMTMSG);
	aud_state=U_BOTH;
	goto BYPASS;
    }


    /* The code that follows tries to reconcile what is in the control file
     * with the files the kernel is using (if auditing is on) or with the
     * apparent use of the  audit files (if auditing is off).
     *
     * Once the following tests are done; the values in the cntl_*
     * are left consistent with what was found in the kernel or the state
     * of the audit files.
     *
     * aud_state is set to one of the following:
     * GOOD control file and kernel/audfiles is consistent
     * U_NEXT kernel contains a reference to an unknown next file
     * RGOOD control file and kernel/audfiles are consistent with a roll-over
     * RU_NEXT kernel contains a reference to an unknown next after rollover
     * U_CURR kernel current file is unknown
     * U_BOTH kernel current and next files are unknown
     */ 
    if (aud_on)
	/* if auditing is on, the kernel specified current and next files
	 * are checked against those specified by the control file.
	 * if both match aud_state=GOOD. if the next audit file specified
	 * in the kernel doesn't match the one in the control file 
	 * aud_stat=U_NEXT. if the current file specified by the kernel
	 * matches the next file specified by the control file and the
	 * next file specified by the kernel is NULL aud_state=RGOOD.
	 * if the next file specified by the kernel in this situation is
	 * not NULL aud_state=RU_NEXT.  if the current file specified
	 * by the kernel matches neither file specified by the control
	 * file aud_state=U_CURR.  if the control file next file doesn't
	 * match the kernel next file in this situation aud_state=U_BOTH.
	 */
        if (strcmp(sys_cfile,cntl_cfile)==0) 
	    if (strcmp(sys_xfile,cntl_xfile)==0)
		aud_state=GOOD;
	    else {
                aud_state=U_NEXT;
            }
        else 
	    if (strcmp(sys_cfile,cntl_xfile)==0)
		if (strcmp(sys_xfile,AST)==0) {
		    aud_state=RGOOD;
		    cntl_cfile=cntl_xfile;
		    cntl_cmax=cntl_xmax;
		    cntl_xfile=AST;
		    cntl_xmax=0;
                } else {
		    aud_state=RU_NEXT;
		    cntl_cfile=cntl_xfile;
		    cntl_cmax=cntl_xmax;
		    cntl_xfile=AST;
		    cntl_xmax=0;
                }
            else
		if (strcmp(sys_xfile,cntl_xfile)==0)
		    aud_state=U_CURR;
                else
		    aud_state=U_BOTH;
                
    else 
	/* if auditing is off, all we have to go on is size of the
	 * next file (or CDF); if it hasn't been used then we haven't rolled,
	 * otherwise we have.  Or if there is no next file we must use
	 * the current one specified in the control file.
	 */
	if (strcmp(cntl_xfile,AST)==0)
	    aud_state=GOOD;
	else
	    if (af_empty(cntl_xfile))
	        aud_state=GOOD;
            else {
	        aud_state=RGOOD;
		cntl_cfile=cntl_xfile;
		cntl_cmax=cntl_xmax;
		cntl_xfile=AST;
		cntl_xmax=0;
            }

    /* Last command line checks c=c, x=x  -set ceqc and/or xeqx overide
     * These get set if the current or next files are set to the same value
     * they were set to already.
     * The filenames will not be reloaded into the kernel but the AFS
     * sizes may be changed in the control file.
     */
    ceqc=cflg && !strcmp(cntl_cfile,cfile);
    xeqx=xflg && !strcmp(cntl_xfile,xfile);

    /* special case - if -s or -z is specified without -c or -x
     * pretend that the cfile or xfile already in use was specified
     * on the command line
     */
    if (sflg && (!cflg)) {
	strcpy(cfile,cntl_cfile);
	cflg=1;
	ceqc=1;
    }
    if (zflg && (!xflg)) {
	if (strcmp(cntl_xfile,AST)) {
	    strcpy(xfile,cntl_xfile);
	    xflg=1;
	    xeqx=1;
        }
    }

    /* BYPASS - come here if unable to read the control file */
    BYPASS:


    /* take care of the simple cases: these do not require updating the kernel
     * or control file.
     * <no args> with auditing on - display audit system stats and exit
     * <no args> with auditing off - display auditing off message and exit
     * -n specified when audit system is already on - specify error and exit
     * -f specified to turn off audit system - shutdown audit sytem and exit
     * -f specified when audit system already off specify error and exit
     */

    /* auditing on/off  with no arguments */
	if (argc==1) {

            if (aud_on)
                fprintf (stdout,ON_MSG);
	    else
		fprintf (stdout, OFF_MSG);

	    switch (aud_state) {
	      case GOOD:
	      case RGOOD:
		  disp_stats(aud_state,cntl_cfile,cntl_cmax,
				       cntl_xfile,cntl_xmax);
	          aud_exit(0);
              case U_CURR:
		  disp_stats(aud_state,sys_cfile,0,
				       cntl_xfile,cntl_xmax);
		  aud_exit(12);
              case U_NEXT:
              case RU_NEXT:
		  disp_stats(aud_state,cntl_cfile,cntl_cmax,
				       sys_xfile,0);
		  aud_exit(12);
              case U_BOTH:
		  disp_stats(aud_state,sys_cfile,0,sys_xfile,0);
                  if (bad_cntl) 
		      aud_exit(6);
                  else
		      aud_exit(12);
              default:
		  fprintf(stderr, E_INTNAL);
	          strcat(audbody,E_INTNAL_MSG);
	          write_audrec(audbody,1);
		  aud_exit(14);
            }
        }

    if (aud_on) {
	if (nflg) {
	    /* trying to turn auditing on when it is already on */
	    fprintf(stderr, E_ON_MSG);
	    aud_exit(8); 
	} else if (fflg) {
	    /* turning off the audit system */

	    /* write an audit record */
	    strcat(audbody, TOFF_MSG);
            write_audrec(audbody,0);

	    /* turning the system off */
	    if(audctl(AUD_OFF,0,0,0)) {
	        fprintf(stderr, E_INTNAL);
		strcat(audbody,TOFF_INTN_MSG);
                write_audrec(audbody,1);
		aud_exit(14);
            }

	    /* print a message telling the user what we did */
	    fprintf(stdout,TOFF_OUT);
	    if (cflg || xflg) {
	        fprintf(stderr, IGNORED);
            }
	        aud_exit(0);
	}
    } else {
        if (!nflg || fflg) {
	    /* trying to turn auditing off when it is already off */
	    fprintf (stderr, OFF_MSG);
	    fprintf (stderr, E_OFFOPT);
	    aud_exit(10);
	}	
    }


    /* determine whether c eq x over-ride applies
     * if the user purposely or accidentally sets the current file equal to
     * the next; set the ceqx flag.
     */

     if (cflg&&xflg)
         ceqx= !strcmp(cfile,xfile);
     else {
	 if (!cflg&&!xflg&&!bad_cntl)
             ceqx= !strcmp(cntl_cfile,cntl_xfile);

         if (cflg&&!bad_cntl) 
             ceqx= !strcmp(cfile,cntl_xfile);

         if (xflg&&!bad_cntl) 
             ceqx= !strcmp(xfile,cntl_cfile);
     }
      

    /* Take care of the more complicated cases where the control file
     * and/or kernel has to be updated or corrected.
     * (-n) to turn on the system using default audit files
     * (-n, -c) turn on the system and switch current file
     * (-n, -x) turn on the system and switch next file
     * (-n, -c, -x) turn on the system and switch both current and next file
     * (-c, -x) switch both the current and next files
     * (-c) switch current file only
     * (-x) switch next file only
     *
     * prior to this point the cntl_* variables have been set
     * to make what them consistent with the state of the audfiles
     * and/or the state of the kernel. aud_state indicates whether
     * all is well (GOOD RGOOD) or whether the current and/or next file
     * is inconsistent.
     *
     * Files specified with -c or -x over-ride the cntl_* values.
     * For example, if the next file was inconsistent specifying a
     * next file with -x will set things right (without causing an
     * error message to be emitted). Using this same example, if
     * a valid next file is not specified an error message will be
     * emitted.
     */

    if (cflg) {
	/* verify the specified current file and make it if necessary */
	if (!ceqc && !ceqx && !af_empty(cfile)) {
	    /* audit file not empty */
	    fprintf(stderr, E_NEMPT, cfile);
	    fprintf(stderr, E_NOACT);
	    aud_exit(11);
	}
	af_full(cfile,cmax*1024,VAFS,E_CURRENT);
	mk_audfile(cfile,audbody,0,&creat_c);

        /* update the current file data to be written to the control file */
	cntl_cfile=cfile;
	cntl_cmax =cmax;

	/* update the audit record */
        strcat(audbody, C_MSG);
        strcat(audbody, " ");
        strcat(audbody, cfile);
        strcat(audbody, "\n");
    } else { 
	/* if the current file isn't switched- verify it is good */
	/* an audit record is written if it is bad */
	switch (aud_state) {
	  case U_CURR:
	      /* if the current file is bad */
	      fprintf(stderr,"%s %s\n", E_UNKNOWN_CUR, sys_cfile);
	      fprintf(stderr,E_UKCFIX);
	      fprintf(stderr, E_NOACT);

	      /* write the audit-record body */
	      strcat (audbody, UNKNOWNC_MSG);
	      strcat (audbody, " ");
	      strcat (audbody, sys_cfile);
	      strcat (audbody, "\n");
	      strcat (audbody, NOACT_MSG);
	      write_audrec(audbody,1);
	      aud_exit(12);
	      break;

	  case U_BOTH:
	      /* if both files are bad */

	      /* tell the user what we're using if we know */
	      if (aud_on) {
		  fprintf(stderr,"%s %s\n", E_UNKNOWN_CUR, sys_cfile);
		  fprintf(stderr,"%s %s\n", E_UNKNOWN_NXT, sys_xfile);
	      }
	      fprintf(stderr,E_UKCFIX);
	      fprintf(stderr, E_NOACT);

	      if (bad_cntl) {
	          /* write the audit-record body */
		  strcat (audbody, NOACT_MSG);
		  write_audrec(audbody,1);
		  aud_exit(6);
	      } else {
		  strcat (audbody, UNKNOWNC_MSG);
		  strcat (audbody, " ");
		  strcat (audbody, sys_cfile);
		  strcat (audbody, "\n");
		  strcat (audbody, UNKNOWNN_MSG);
		  strcat (audbody, " ");
		  strcat (audbody, sys_xfile);
		  strcat (audbody, "\n");
		  strcat (audbody, NOACT_MSG);
		  write_audrec(audbody,1);
		  aud_exit(12);
	      }
	      break;
	    }

	/* if the aud_state isn't bad make sure the file exists-
	 * create it if necessary.
	 */
	af_full(cntl_cfile,cntl_cmax*1024,NVAFS,E_CURRENT);
        mk_audfile(cntl_cfile,audbody,1,&creat_c);
    }


    /* if current file = next file; clear next file and skip the work
     * on the next files
     */
    if (ceqx) {
	/* User doesn't want a next file */
        cntl_xfile=AST;
        cntl_xmax=0;

	/* Tell the user what we did */
	fprintf (stderr, CEQX_OUT);
        fprintf (stderr, RESET);

	/* update audit record */
        strcat(audbody, CEQX_MSG);
        strcat(audbody, "\n");
    }
    

    /* skip this if the ceqx over-ride is active */
    /* othewise take care of -x option */
    if (xflg&&!ceqx) {
	/* verify the specified next file and make it if necessary */
	if (!xeqx && !af_empty(xfile)) { 
	    /* audit file not empty */
	    fprintf(stderr, E_NEMPT, xfile);
	    fprintf(stderr, E_NOACT);
	    aud_exit(11);
	}
	af_full(xfile,xmax*1024,VAFS,E_NEXT);
        mk_audfile(xfile,audbody,0,&creat_x);

        /* update the next file data to be written to the control file */
        cntl_xfile=xfile;
        cntl_xmax=xmax;

        /* update the audit record */
        strcat(audbody, X_MSG);
        strcat(audbody, " ");
        strcat(audbody, xfile);
        strcat(audbody, "\n");
} else
  if (!ceqx) { 
        /* if the next file isn't switched - make certain it is good */
        if ((aud_state==U_NEXT)||(aud_state==RU_NEXT)||(aud_state==U_BOTH)){
	    /* if it is bad */

	    if (aud_on) {
		/* if auditing is on tell them what we were using 
	         * and what we are going to do about it -nothing
	         */
                fprintf(stderr, "%s %s\n", E_UNKNOWN_ONXT,
		  (strcmp(sys_xfile,AST) ? sys_xfile : "none") );
		fprintf(stderr, E_UKNFIX);
                fprintf(stderr, E_NOACT);

                /* update the audit record
		 * if -c to set current file was specified earlier
		 * backtrack over last message - a kludge but its the
		 * easiest to do.
		 */
                if (cflg) {
                    *(strrchr(audbody, (int)'\n'))='\0';
		    strcpy(strrchr(audbody, (int)'\n')+1, NOACT_MSG);
                } else 
		    strcat(audbody,NOACT_MSG);

                /* write the audit record and exit */
                write_audrec(audbody,1);
		if (bad_cntl) aud_exit(6);
		else aud_exit(12);
	    } else {
                /* if we are starting-up we can help more */
	        /* repair the situation */
	        /* write the control file */
		fprintf (stderr,RESET);

                /* update the audit record */
                strcat(audbody, RESET_MSG);

		/* write the control file */
	        if (write_cntl(cntl_cfile,cntl_cmax,AST,0)){
	            fprintf(stderr, E_INTNAL);
	            strcat(audbody, CNT_WRCTL_MSG);
	            write_audrec(audbody,1);
		    aud_exit(14);
                }
		no_cntl=0;

		if (bad_cntl) {
                    fprintf(stderr,E_NEWCNTL);
	            strcat(audbody, FIXCNTL_MSG);
                }

                /* set the kernel curr/next file to match the control file */
		if (audctl(AUD_ON,cntl_cfile,0,MODE)){
	            fprintf (stderr, E_INTNAL);
		    aud_exit(14);
		} 

		/* update the audit record - explain the situation */
	        if (!bad_cntl) {
	            strcat (audbody, UNKNOWNN_MSG);
                    strcat (audbody, " ");
	            strcat (audbody, sys_xfile);
                    strcat (audbody, "\n");
                }

                /* write the audit record and exit */
                write_audrec(audbody,1);
		aud_exit(6);
            }
        } else { 
	    /* if the aud_state isn't bad make sure the file exists-
	     * create it if necessary.
	     */
            if (strcmp(cntl_xfile,AST)!=0) {
		af_full(cntl_xfile,cntl_xmax*1024,NVAFS,E_NEXT);
                mk_audfile(cntl_xfile,audbody,1,&creat_x);
	    }
        }
    }


    /* switch the kernel files as required:
     *
     * if -n was not specified the audit system is on
     * and we could not have gotten this far if the current and
     * next files were not consistent with the control file (before
     * applying any specified -c or -x option)
     *
     * The kernel is only updated with the information provided on the
     * command line -c or -x and only if the request is to change the
     * files (eg. if old current = new current, the current file isn't updated)
     * (ceqc or xeqx override active) OR if one or both of the files was
     * created because audsys noticed they didn't exist (creat_c or created_x
     * active). 
     *
     * The reason we go to all this trouble to do the minimum possible
     * in the kernel is to avoid confusing situations.
     *
     * if -n was specified the audit system is off
     * we must do an audctl with AUD_ON to get it started
     * we load the (adjusted for overflow)  filenames from the
     * control file into the kernel.
     */
    if (!nflg) {
	int retval=0;
	cflg = cflg && !ceqc;
	xflg = xflg && !xeqx;
        if (ceqx) {
            if (cflg&& !xflg) retval=audctl(AUD_SET,cfile,0,MODE);
	    if (xflg&& !cflg) retval=audctl(AUD_SET,xfile,0,MODE);
	    /* warning xflg and !cflg may result in a glitch if the
	     * audit system switches between the time audsys GETs and
	     * updates the kernel.
	     */
            if (cflg&& xflg)  retval=audctl(AUD_SET,cfile,0,MODE);
        } else {
	    /* Only what is requested on the command line is set
	     * except when the creat_x or creat_c override flags are
	     * on. These flags indicate that the current or next file
	     * was created; in these instances the corresponding file
	     * is reset in the kernel in addition to the file specified
	     * on the command line
             */
            if(cflg&& !xflg)
              if(creat_x) 
		retval=audctl(AUD_SET,cfile,CVAST(cntl_xfile),MODE);
              else
		retval=audctl(AUD_SETCURR,cfile,0,MODE);

	    if(xflg&& !cflg)
              if(creat_c)
		retval=audctl(AUD_SET,cntl_cfile,xfile,MODE);
              else 
		retval=audctl(AUD_SETNEXT,0,xfile,MODE);

	    if(cflg&&xflg)  retval=audctl(AUD_SET,cfile,xfile,MODE);

	    if(!cflg&&!xflg) {
	      if(creat_c && !creat_x)
		retval=audctl(AUD_SETCURR,cntl_cfile,0,MODE);
              if(creat_x && !creat_c)
		retval=audctl(AUD_SETNEXT,0,CVAST(cntl_xfile),MODE);
              if(creat_x && creat_c)
		retval=audctl(AUD_SET,cntl_cfile,CVAST(cntl_xfile),MODE);
            }
        }
	if (retval) {
	    fprintf (stderr, E_INTNAL);

	    /* write an audit record */
	    strcat(audbody,SET_INTN_MSG);
            write_audrec(audbody,1);

	    aud_exit(14);
        }
    } else { /* audit system is off -n with or without -x or -c */
	if (audctl(AUD_ON,cntl_cfile,CVAST(cntl_xfile),MODE)){
	    fprintf (stderr, E_INTNAL);

	    /* write an audit record */
	    strcat(audbody,SET_INTN_MSG);
            write_audrec(audbody,1);

	    aud_exit(14);
        }
        strcat(audbody, TON_MSG);
    }


    /* write the control file */
    if (write_cntl(cntl_cfile,cntl_cmax,cntl_xfile,cntl_xmax)) {
	fprintf(E_INTNAL);
	strcat(audbody, CNT_WRCTL_MSG);
	write_audrec(audbody,1);
	aud_exit(14);
    }

    no_cntl=0;
 
    if (bad_cntl) {
        /* and mention that we corrected the situation */
        fprintf(stderr,E_NEWCNTL);
        strcat(audbody, FIXCNTL_MSG);
    }
        
    /* close the control file */
    close(fd);

    /* write audit record body */
    if (nflg) {
	if (!cflg && !xflg) {
	    strcat(audbody, CURR_MSG);
	    strcat(audbody, cntl_cfile);
	    strcat(audbody, "\n");
	    strcat(audbody, BACKUP_MSG);
	    strcat(audbody, PVAST(cntl_xfile));
	    strcat(audbody, "\n");
        }
    }

    /* write the audit record we have been updating all along */
    write_audrec(audbody,0);

    aud_exit(0);
}



/* get_cntl() - routine to open and lock the control file and get the
 * control file parameters.
 *
 *  cntl_cfile, cntl_cmax - returned control current file params
 *  cntl_xfile, cntl_xmax - returned control next file params
 *
 *  get_cntl if it fails, prints an error message and exits
 *           if it succeeds, returns the assigned parameters
 */

static int
get_cntl(cntl_cfile,pcntl_cmax,cntl_xfile,pcntl_xmax)

char *cntl_cfile,*cntl_xfile;
int  *pcntl_cmax, *pcntl_xmax; 

{

    extern int fd;
    extern int errno;

    int nbytes;
    char tmpstr[MAXCSZ+1];


    /* Read the file */
    nbytes=read(fd,tmpstr,MAXCSZ);
    tmpstr[nbytes]='\0';

    /* get the records records */
    if (sscanf(tmpstr,"%[^,],%d %[^,],%d",
	cntl_cfile,pcntl_cmax,cntl_xfile,pcntl_xmax)!= 4) return (-1);

    return(0);
}


/* disp_stats() - routine to display audit system statistics for a given
 * audit file
 *
 *  situation - display matches aud_state requirements
 *  cfile - is the name of current file (or CDF)
 *  cs - is the switch size for the current file
 *  bfile - is the name of the backup file (or CDF)
 *  bs - is the switch size for the backup
 * 
 *  This routine doesn't return anything, it either works or it doesn't.
 *  It is a kludge, but the existance of the audit known audit files specified
 *  is checked here... this can cause an exit from the entire program
 *  with status == 18
 */

static void
disp_stats(situation,cfile,cs,bfile,bs)

aud_state_type situation;
char *cfile, *bfile;
int cs,bs;

{

    int cfs,bfs,cfps,cffps,bfps,bffps;
    int csize,csizeb,bsize,bsizeb,cfree,bfree,err18;
    struct stat buf;

    /* if current or next file is "" (unset) set them to "unknown" */
    if (*cfile==0) cfile=DUNKNOWN;
    if (*bfile==0) bfile=DUNKNOWN;

    /* if there is a current file: calculate the current file statistics */
    if (situation!=U_CURR && situation!=U_BOTH) {
      csize=BTOKB(csizeb=af_size(cfile));
      cfps=( csizeb== -1 ? 0 : AF_PCENT(csizeb,cs*1024));
      cffps=fs_percent(cfile,&cfs,&cfree);
    }

    /* print the current file name */
    printf("%s %s\n", DCFILE_OUT, cfile);

    /* if there is a next file: calculate the next file statistics */
    if ( situation!=U_NEXT && situation!=RU_NEXT && situation!=U_BOTH
      && strcmp(bfile,AST)!=0) {
        bsize=BTOKB(bsizeb=af_size(bfile));
	bfps=( bsizeb== -1 ? 0 : AF_PCENT(bsizeb,bs*1024));
        bffps=fs_percent(bfile,&bfs,&bfree);
    }

    /* if there is no next file: print "none"
     * otherwise, print the next file name
     */
    if (strcmp(bfile,AST)==0)
      printf(DNXFILE_MSG);
    else
      printf("%s %s\n", DBFILE_OUT, bfile);

    /* print the "statistics" header */
    printf(DSTAT_HDR_OUT);

    /* if there is a current file: print the current file statistics
     * otherwise, print "no data available"
     */
    if (situation!=U_CURR && situation!=U_BOTH) {
      printf("%s %8d %8d %8d %8d %8d %8d\n",
        DCFILE_OUT,cs,csize,cfps,BTOKB(cfs),BTOKB(cfs-cfree),cffps);
    } else {
      printf(DCDAU_OUT);
    }

    /* if there is a next file: print the next file statistics
     * otherwise, print "no data available"
     */
    if (situation!=U_NEXT && situation!=U_BOTH) {
      if (strcmp(bfile,AST)==0)
        printf (DNXFILE_MSG);
      else
        printf("%s %8d %8d %8d %8d %8d %8d\n",
          DBFILE_OUT,bs,bsize,bfps,BTOKB(bfs),BTOKB(bfs-bfree),bffps);
    } else {
        printf(DBDAU_OUT);
    }

    /* if everything is ok: 
     * check for the unlikely event that one of the files
     * doesn't exist-- if sombody has deleted one or both of the audit
     * files, it is detected here
     */
    if (situation==GOOD || situation==RGOOD) {
      if (stat(cfile,&buf)) {
        /* print error message */
        fprintf(stderr, E_WCREAT,cfile);
        err18=1;
      } else 
        err18=0;

      if ( strcmp(bfile,AST) && stat(bfile,&buf)) {
        /* print error message */
        fprintf(stderr, E_WCREAT,bfile);
        err18=1;
      }

      if (err18)
        aud_exit(18);
    }
}


static void
write_audrec(audbody,err)

char *audbody;
int err;

{
    extern int audswitch();
    struct self_audit_rec audrec;

    /* copy the audit body into the audrec struct */
    strcpy (audrec.aud_body.text, audbody);

    /* write audit record header */
    audrec.aud_head.ah_error = err;
    audrec.aud_head.ah_event = EN_AUDSYS;
    audrec.aud_head.ah_len = strlen(audbody);

    /* write the audit record */
    (void)audswitch(AUD_RESUME);
    audwrite(&audrec);
    (void)audswitch(AUD_SUSPEND);

}


/*  fs_percent - a routine to determine the percentage of
 *  the filesystem containing filename - available to the audit system
 *  the results are +/- 1 percent
 *
 *  If the statfs() fails this routine returns 100% and 0 for the file
 *  system size.
 */
int
fs_percent(filename,fsyss,fsfree)
char *filename;
int *fsyss;
int *fsfree;
{
    char filename_plus[MAXPATHLEN+1];
    struct statfs buf;

    if (statfs(filename, &buf)) {

        strcpy(filename_plus, filename);
        strcat(filename_plus,"+");

	if (statfs(filename_plus, &buf)) {
	    *fsyss=0;
	    *fsfree=0;
	    return(100);
        }
    }

    *fsyss=(double)buf.f_blocks * (double)buf.f_bsize;
    *fsfree=(double)buf.f_bfree * (double)buf.f_bsize;
    return((((double)buf.f_bfree/(double)buf.f_blocks)*(double)100)+.5);
}


/*  get_context - returns a limited context in the char* passed
 *  it returns 1 for a standalone or diskless system 
 *  0 otherwise.
 */

int
get_context(context)
    char *context;
{

    struct stat buf;
    FILE *cd, *popen();

    /* if we cannot get to /bin/getcontext then we probably are not on
     * a system capable of running diskless
     */
    if (stat("/bin/getcontext", &buf)) return(0);

    if ((cd=popen("/bin/getcontext","r"))==NULL) return(0);

    fscanf(cd,"%s",context);

    if (pclose(cd)!=0) return (0);

    return(1);
}

/* mk_audfile - routine to validate audit files or create them as necessary
 * if all goes well it returns otherwise it exits and writes an appropriate
 * audit record. If warn is set provide a warning when files or CDFs
 * are created.
 */

static void
mk_audfile(filename,audbody,warn,create)

    char *filename, *audbody;
    int warn, *create;
{

    struct stat buf;
    int diskless;
    char context[MAXPATHLEN];
    char invo_makecdf[MAXPATHLEN+20];
    char filename_plus[MAXPATHLEN+1];
    int chmod();

    diskless=get_context(context)&&strcmp("standalone",context);

    if (diskless) {
	strcpy (invo_makecdf,"/usr/bin/makecdf -c ");
	strcat (invo_makecdf,context);
	strcat (invo_makecdf," ");
	strcat (invo_makecdf,filename);
	strcat (invo_makecdf," ");

	strcpy (filename_plus,filename);
	strcat (filename_plus,"+");
    }


    if (stat(filename,&buf)) {
	/* if we can't stat the file- create it somehow */

	/* set the create flag
	 * - indicating that we've created the audit file
	 */
	*create=1;
	
	/* if mk_audfile is called in a situation where an audit
	 * file should exist (warn==WARN) a warning is provided.
	 */
        if (warn)
	    fprintf(stderr, E_WCREAT, filename);

        if (diskless && stat(filename_plus,&buf)) {
            /* if we are on a diskless system and there is no CDF dir */
            if (system(invo_makecdf) == 0)
	        printf("%s %s\n", F_CREAT, filename);
            else {
	        fprintf(stderr,E_CREAT,filename);
	        fprintf(stderr, E_NOACT);
                if (warn)
	            strcat(audbody, F_WCREAT);
                strcat(audbody,CREAT_ERR);
		strcat(audbody," ");
		strcat(audbody,filename);
		strcat(audbody,"\n");
		strcat (audbody, NOACT_MSG);
		write_audrec(audbody,1);
		aud_exit(11);
            }
        } else {
	    /* not a diskless system or diskless with CDF dir */
            if (creat(filename,MODE) > 0)
		printf("%s %s\n", F_CREAT, filename);
            else {
		fprintf(stderr,E_CREAT,filename);
	        fprintf(stderr, E_NOACT);
                if (warn)
	            strcat(audbody, F_WCREAT);
                strcat(audbody,CREAT_ERR);
		strcat(audbody," ");
		strcat(audbody,filename);
		strcat(audbody,"\n");
		strcat (audbody, NOACT_MSG);
		write_audrec(audbody,1);
		aud_exit(11);
            }
        }

    } else
	/* if the file exists */
        if (diskless){
            /* and if we are on a diskless system */

	    /* if it is not a regular file -getout */
	    if (!ISREG(buf.st_mode)) {
	        fprintf(stderr, E_FTYPE, filename);
	        fprintf(stderr, E_NOACT);
		strcat(audbody,FILE_ERR);
		strcat (audbody, NOACT_MSG);
		write_audrec(audbody,1);
		aud_exit(11);
            }

            /* if file+ is not stat-able (file not a CDF) convert it to a CDF */
	    if (stat(filename_plus,&buf)) {

	        /* set the create flag
	         * - indicating that we've created the audit file
	         */
	        *create=1;

                if (system(invo_makecdf) == 0)
	           printf("%s %s\n", F_CREAT, filename);
                else {
		    fprintf(stderr,E_CREAT,filename);
                    strcat(audbody,CREAT_ERR);
	            strcat(audbody," ");
		    strcat(audbody,filename);
		    strcat(audbody,"\n");
		    strcat(audbody, NOACT_MSG);
		    write_audrec(audbody,1);
                }
            }

             /* if already a CDF do nothing more */

        } else {
	    /*  if not diskless - verify the file is a regular file */
            if (!ISREG(buf.st_mode)) {
		fprintf(stderr, E_FTYPE, filename);
	        fprintf(stderr, E_NOACT);
		strcat(audbody,FILE_ERR);
		strcat(audbody, NOACT_MSG);
		write_audrec(audbody,1);
		aud_exit(11);
            }
        }

    /* guarantee that the modes are set right
     * cannot fail- we are root, and the file exists
     * and if it does we can catch it another time
     */
    (void)chmod(filename,MODE);

}


/* af_full() routine to verify that the audit file has not grown past its
 * switch size and that there is room on the filesystem for a full size
 * audit file.  Type is either VAFS or NVAFS to verify afs or not verify afs. 
 *
 * The objectives of the tests within this routine are:
 * 
 * 1. Insure that the auditing system does not allow someone to
 * accidentally set the current file and AFS to values that would
 * cause the auditing system to immediately rollover to the next file.
 * This is accomplished by verifying that the file has not already grown
 * past the AFS size specified. 
 *
 * 2. Insure that there is sufficient space on the partition 
 * that contains the audit file to allow it to grow to its AFS size.
 * This is accomplished by verifying that the available space on
 * the partition containing the audit file is greater than the possible
 * "growth" of the audit file.
 *
 * 3. Insure that the auditing system doesn't go into "minfree" behavior
 * (logging off everybody but root at console etc.) immediately after the
 * auditing system is started.  This is accomplished by preventing the
 * auditing system from being started when either the current
 * or next files are on a partition that is filled past minfree.
 * 
 * A "type" of NVAFS specifies that only objective 3 is insured. 
 * This "type" is used generally when use of the audit file specified is
 * resumed (it is not a new file).  However, when the auditing system
 * resumes using a current file when there isn't a next file. af_full()
 * is called with type VAFS instead.
 *
 * A "type" of VAFS specifies that all 3 objectives are verified.
 * This "type" is used generally when the audit file is specified using the -c
 * or -x option, and in addition, in the special case mentioned earlier. 
 *
 * This routine exits if the auditfile filesystem is full.  It returns -1
 * if the filename file or directory doesn't exist (for statfs).  It returns
 * 0 otherwise.
 */

int
af_full(filename,afs,type,which)
char *filename, *which;
int afs,type;
{
    char tmp_strbuff[MAXPATHLEN], *name_ptr;
    int filesize, growth;
    char *strrchr();
    struct stat buf;

    name_ptr=filename;

    /* get the filesize using af_size; if the filesize is <= zero
     * provide a truncated version of the filename for use by fs_avail;
     * fs_avail must be given a valid filename (directory).
     */ 
    if ((filesize=af_size(filename)) <= 0 ){

        (void)strcpy(tmp_strbuff,filename);
	*(strrchr(tmp_strbuff,'/'))='\0';
	if (strlen(tmp_strbuff))
	  filename=tmp_strbuff;
        else
	  filename="/";

        if (filesize == -1) {
	  if (stat(filename, &buf)) {
	    fprintf(stderr, E_DIRNEXIST,filename);
	    return(-1);
          }
	  filesize=0;
        }
    }

    if (type==VAFS) { 
        /* if type is VAFS - verify that the filesize is less than the afs */
	/* doing the <= check insures afs is > 0 at the same time */
        if ((growth = afs - filesize) <= 0 ){
    	    fprintf(stderr, E_AFTBG, which, name_ptr);
	    fprintf(stderr, E_NOACT);
	    aud_exit(15);
        }
        /* verify that there is room on the filesystem for the auditfile */
        if ((fs_avail(filename) - growth) <= 0 ){
    	    fprintf(stderr, E_AFSTBG, which, name_ptr);
	    fprintf(stderr, E_NOACT);
	    aud_exit(16);
        }
    } else {
	/* just verify that we are not already in minfree */
	/* we can allow audomon to switch the file if it is past afs */
   	if (fs_avail(filename) <= 0 ){ 
    	    fprintf(stderr, E_AFPFULL, which, name_ptr);
	    fprintf(stderr, E_NOACT);
	    aud_exit(17);
	}
    }

    return(0);
}


/*  af_empy - a routine to determine whether an audit file is empty
 *  
 *  returns 0 if the file exists and is not empty.
 *          1 otherwise.
 *
 */
int
af_empty(filename)

    char *filename;
{
    struct stat buf;
    DIR *dirp;
    struct direct *dp;
    char filename_plus[MAXPATHLEN+1];
    char element[2*MAXPATHLEN+1];
    int not_cdf;
    int offset;

    strcpy(filename_plus,filename);
    strcat(filename_plus,"+");

    /* if stat of filename+ doesnt work we know for sure it isnt a CDF */
    not_cdf=stat(filename_plus, &buf); 

    /* if the file doesnt exist return true */
    if (not_cdf && stat(filename, &buf) < 0) return(1);

    /* if it is a CDF */
    if ((!not_cdf)&&ISCDF(buf.st_mode)) {

	strcpy(element,filename_plus);
	strcat(element,"/");
	offset=strlen(element);

        dirp=opendir(filename_plus);

        for (dp=readdir(dirp); dp != NULL; dp=readdir(dirp)){
	    if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0)
                continue;
	    strcpy(element+offset,dp->d_name);
            if (stat(element, &buf) == 0)
                if (buf.st_size) {
	            closedir(dirp);
	            return(0);
                }
        }
	return(1);
    } else
        /* otherwise check the size of the file */
	return (!buf.st_size);

}


/*  af_size - a routine to determine the size of an  audit file (CDF)
 *  This routine returns zero if it cannot stat the file: if it doesn't
 *  exist or cannot be accessed.
 */
int
af_size(filename)

char *filename;

{
    struct stat buf;
    DIR *dirp;
    struct direct *dp;
    int offset, size;
    char filename_plus[MAXPATHLEN+1];
    char element[2*MAXPATHLEN+1];

    strcpy(filename_plus,filename);
    strcat(filename_plus,"+");
    

    if (!stat(filename_plus, &buf)&&ISCDF(buf.st_mode)) {
        /* if the file is a CDF */
	strcpy(element,filename_plus);
	strcat(element,"/");
	offset=strlen(element);

	size=0;

        dirp=opendir(filename_plus);

        for (dp=readdir(dirp); dp != NULL; dp=readdir(dirp)) {
	    if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0)
                continue;
	    strcpy(element+offset,dp->d_name);
            if (stat(element, &buf) ==0 )
                size+=buf.st_size;
        }

        closedir(dirp);
    } else {
        /* stat the file */
        if (stat(filename, &buf)) {
	    return(-1);
        }
	size=buf.st_size;
    }

    return(size);
}



/*  fs_avail - a routine to report the available space on the 
 *  filesystem containing filename (less minfree) in bytes
 *
 *  If the statfs() of the filename fails this routine returns a size of zero.
 */
int
fs_avail(filename)
char *filename;
{
    char filename_plus[MAXPATHLEN+1];
    struct statfs buf;

    if (statfs(filename, &buf)) {

        strcpy(filename_plus, filename);
        strcat(filename_plus,"+");

	if (statfs(filename_plus, &buf))
	    return(0);
    }

    return(buf.f_bavail*buf.f_bsize);
}



/* write_cntl() routine to write the control file
 */
int
write_cntl(cfile,cmax,xfile,xmax)
char *cfile, *xfile;
int cmax, xmax;

{

    extern int fd;
    char tmpstr[MAXCSZ+1];
    int tmpstr_len;

    (void)ftruncate(fd,0);
    (void)lseek(fd,0,0);

    if ((tmpstr_len=
      sprintf(tmpstr,"%s,%d\n%s,%d\n",cfile,cmax,xfile,xmax)) <= 0 ||
      write(fd,tmpstr,tmpstr_len) < 0) 
        return(-1);
    else
        return(0);  
}

/* aud_exit() removes the control file if it was properly fixed and
 * it didn't exist initially - we don't want to leave a bad one that
 * we have created around.
 */
static void 
aud_exit(retval)
int retval;
{
    extern int no_cntl, fd;

    if (no_cntl && fd > 0 ) unlink(CNTL_FILE);

#ifdef NLS
    (void)catclose(catd);
#endif

    exit(retval);
}
