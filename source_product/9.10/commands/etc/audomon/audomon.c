#ifndef lint
    static char *HPUX_ID = "@(#) $Revision: 70.3 $";
#endif

#include <sys/types.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/audit.h>
#include <unistd.h>
#include <ndir.h>
/* #include <dirent.h> XXX -- clashes with <ndir.h>, what should we do here? */
#include <sys/errno.h>
#include <sys/signal.h>

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else /* NLS */
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif /* NLS */

#define E_AUDOMON_USE (catgets(nlmsg_fd,NL_SETN,1, "usage: audomon [-p fss] [-t sp_freq] [-w warning] [-v] [-o output]\n"))
#define E_AUDOMON_P (catgets(nlmsg_fd,NL_SETN,2, "audomon: fss is an integer > 0 and < 100\n"))
#define E_AUDOMON_T (catgets(nlmsg_fd,NL_SETN,3, "audomon: sp_freq (minutes) is a positive real number\n"))
#define E_AUDOMON_W (catgets(nlmsg_fd,NL_SETN,4, "audomon: warning is positive integer <= 100\n"))
#define E_AUDOMON_BADO (catgets(nlmsg_fd,NL_SETN,26, "audomon: cannot write to output file.\n"))
#define AUDOMON_FS1 (catgets(nlmsg_fd,NL_SETN,5, "file system of audit file has %d%% free space left !\n"))
#define AUDOMON_FS2 (catgets(nlmsg_fd,NL_SETN,6, "an attempt to switch to the backup file failed.\n"))
#define AUDOMON_BK (catgets(nlmsg_fd,NL_SETN,7, "Must specify a backup file now !\n"))
#define AUDOMON_AF1 (catgets(nlmsg_fd,NL_SETN,8, "current audit file size is %ld kilobytes!!!\n"))
#define AUDOMON_AF2 (catgets(nlmsg_fd,NL_SETN,9, "an attempt to switch to the backup file failed.\n"))
#define AUDOMON_FSW1 (catgets(nlmsg_fd,NL_SETN,10, "audit system approaching FreeSpaceSwitch point.\n"))
#define AUDOMON_FSW2 (catgets(nlmsg_fd,NL_SETN,11, "free space left on file system = %d%%.\n"))
#define AUDOMON_AFW1 (catgets(nlmsg_fd,NL_SETN,12, "audit system approaching AuditFileSwitch point.\n"))
#define AUDOMON_AFW2 (catgets(nlmsg_fd,NL_SETN,13, "current audit file size = %ld kilobytes.\n"))
#define AUDOMON_CNTL1 (catgets(nlmsg_fd,NL_SETN,14, "audit system corruption: could not open/read\n"))
#define AUDOMON_CNTL2 (catgets(nlmsg_fd,NL_SETN,15, "audit control file - %s !\n"))
#define AUDOMON_CAF1 (catgets(nlmsg_fd,NL_SETN,16, "audit system corruption: couldn't find current\n"))
#define AUDOMON_CAF2 (catgets(nlmsg_fd,NL_SETN,17, "audit file, %s, in the control file, %s.\n"))
#define AUDOMON_AS (catgets(nlmsg_fd,NL_SETN,18, "cannot get current/next audit files -\n"))
#define AUDOMON_SU (catgets(nlmsg_fd,NL_SETN,20, "audomon: you do not have access to the auditing system.\n"))
#define AUDOMON_BADFS (catgets(nlmsg_fd,NL_SETN,27, "audomon: cannot stat the current audit file system.\n"))
#define AUDOMON_BADAF (catgets(nlmsg_fd,NL_SETN,24, "audomon: cannot stat the current audit file.\n"))

#define CNTL_FILE   "/.secure/etc/audnames"
#define NL_CNTL_FILE   (catgets(nlmsg_fd,NL_SETN,21, CNTL_FILE))
#define KBYTES 1024

#define SECS_PER_MIN	 60	/* # of seconds in one minute */

#define	MAX_PCT		100	/* kinda like saying ONE = 1... :-) */
#define	FMAX_PCT	100.0

/*
 * Do not change the following values because they are stated in the manual
 * page.
 */
#define FSS_DEFAULT	 20
#define MAX_WARNINGS	100

/* max length of the control file - 2 records, each with
 * a pathname an a number
 */
#define MAXCNTLFILE ((MAXPATHLEN+12)*2)

/* don't sleep longer than MAXFREQ seconds (= 30 minutes) */
#define MAXFREQ 30*SECS_PER_MIN

/* don't wake up sooner than MINFREQ seconds */
#define MINFREQ 1
#define suser()   (geteuid() ? 0:1)

char *progname;
void my_perror();

/**************************************************************************
 * AUDOMON- Auditing Overflow MONitoring daemon
 *
 * EXIT CODES:
 *   0  - complete successfully.
 *   1  - wrong usage or option.
 *   2  - not previleged user.
 *   3  - cannot write to output file.
 *************************************************************************/

main(argc, argv)
int argc;
char *argv[];

{

#ifdef NLS
	nl_catd nlmsg_fd = catopen("audomon",0);
#endif

   extern int optind;

   /* flags for parsing params */
   int pflg=0, tflg=0, wflg=0, errflg=0, oflg=0, verbose=0;

   int c;   /* free space left in % of total file system */
   int s;   /* size of the current audit file */

   double sp_freq;   /* wakeup frequency at switch points */

   int afs;   /* audit file switch point */
   int fss;   /* free space switch point */

   int warning;   /* % of switch pt when warning begins */
   float fss_warn, afs_warn;   /* warning translated */

   char curr_audfile[MAXPATHLEN];   /* pathname of curr audit file */
   char next_audfile[MAXPATHLEN];   /* pathname of next audit file */
   char fn1[MAXPATHLEN], fn2[MAXPATHLEN];
   int afs1, afs2;
   int cntl_fd;   /* file descriptor to the audit control file */
   char cntl_buf[MAXCNTLFILE];   /* buffer for reading the control file */
   int cntl_buflen;

   struct statfs fsbuf;   /* buffer for statfs */
   struct stat fbuf;   /* buffer for stat */

   int found;
   unsigned checkagain, nextwakeup();	/* the next wake-up time */
   unsigned something_wrong;   		/* some inconsistency is discovered */

   char output[MAXPATHLEN];
   char outputfpn[MAXPATHLEN];
   int fd;   /* dummy file descriptor */

   FILE *out_stream;

   /* Verify if the user is privileged */
   if (!suser()) {
      error(AUDOMON_SU);
      exit(2);
   }

   progname = argv[0];

   while ((c=getopt(argc, argv, "p:t:w:o:v")) != EOF)
      switch(c) {
      case 'p':
         pflg++;
         fss = atoi(argv[optind-1]);
         break;
      case 't':
         tflg++;
         sp_freq = atof(argv[optind-1]);
         break;
      case 'w':
         wflg++;
         warning = atoi(argv[optind-1]);
         break;
      case 'o':
         oflg++;
         (void) strcpy(output, argv[optind-1]);
         break;
      case 'v':
	 verbose = 1;
	 break;
      default:
         errflg++;
         break;
      }

   if (errflg || (optind < argc)) {
      error(E_AUDOMON_USE);
      exit(1);
   }

   if (pflg) {
      if ((fss <= 0) || (fss >= MAX_PCT)) {
         error(E_AUDOMON_P);
         exit(1);
      }
   } else {
      fss = FSS_DEFAULT;
   }

   if (tflg) {
      if (sp_freq <= 0) {
         error(E_AUDOMON_T);
         exit(1);
      }
   } else {
      sp_freq = 1;
   }

   if (wflg) {
      if ((warning <= 0) || (warning > MAX_WARNINGS)) {
         error(E_AUDOMON_W);
         exit(1);
      }
   } else {
      warning = 90;
   }

   if (!oflg) {
      /* if output destination is not specified,
       * direct outputs to the console
       */
      (void) strcpy(output, "/dev/console");
   } else if (access (output, W_OK) != 0) {
      /* cannot access the output file for write */
      fprintf (stdout, E_AUDOMON_BADO);
      (void) strcpy(output, "/dev/console");
   }

   if (output[0] != '/') {
      /* convert to absolute path name
       * because we are going to change directory
       */
      getcwd (outputfpn, MAXPATHLEN);
      (void) strcat(outputfpn, "/");
      (void) strcat(outputfpn, output);
      (void) strcpy(output, outputfpn);
   }

   fss_warn = FMAX_PCT - (float) (warning *  (MAX_PCT - fss)) / FMAX_PCT;

   if (fork() != 0)
      exit(0);   /* parent */

   (void) setpgrp();   /* detach itself from the process group and
			* controlling terminal.
			*/
   (void) signal(SIGHUP, SIG_IGN);   /* Immune from pgrp leader death */
   if (fork() != 0)
      exit(0);   /* child */

   /* Now that we are running as the grandchild,
    * we have both gotten rid of association with the
    * process group and controlling terminal and avoided
    * the possibility of reacquiring a controlling terminal
    * when opening a tty.
    */

   (void) signal(SIGINT, SIG_IGN);   /* The following signal ignoring */
   (void) signal(SIGQUIT, SIG_IGN);  /* shouldn't be necessary after setpgrp */
   (void) signal(SIGTTOU, SIG_IGN);  /* but done just for safety and clarity */
   (void) signal(SIGTTIN, SIG_IGN);
   (void) signal(SIGTSTP, SIG_IGN);

   for (fd=getnumfds()-1;fd>=0;fd--)
      (void) close(fd);				/* close all open files */

   /* cd to root directory so we don't hold any mounted file system */
   chdir ("/");

   while (1) {   /* infinite loop */
      something_wrong = 0;

      /* open the output file */
      if ((out_stream = fopen (output, "w")) == NULL) {
         something_wrong++;
         goto take_a_nap;   /* try again later */
      }

      /* open the control file for read */
      if ((cntl_fd = (open(CNTL_FILE, O_RDONLY))) == -1) {
	 int open_error = errno;    /* errno might get clobbered by audctl() */
         something_wrong++;
         if ((audctl(AUD_GET, curr_audfile, next_audfile, 0) != 0) &&
	     (errno == EALREADY))
	    /* audit system off; don't bother printing warnings */
	    goto take_a_nap;
         fprintf(out_stream, AUDOMON_CNTL1);
         fprintf(out_stream, AUDOMON_CNTL2, NL_CNTL_FILE);
	 fprintf(out_stream, "%s\n", strerror(open_error));
         goto take_a_nap;   /* try again later */
      }

      /* get the pathname of the current audit file */
      if ((audctl(AUD_GET, curr_audfile, next_audfile, 0) != 0) &&
          (errno != ENOENT)) {
         if (errno != EALREADY) {
            /* audit system is on but couldn't
             * get the current audit file.
             */
            fprintf (out_stream, AUDOMON_AS);
         }
         something_wrong++;
         goto take_a_nap;
      }
      /* check the file system */
      if (statfs(curr_audfile, &fsbuf) == -1) {
         fprintf (out_stream, AUDOMON_BADFS);
         something_wrong++;
         goto take_a_nap;
      }
      c = fsbuf.f_bfree * 100 / fsbuf.f_blocks;
      if (c <= fss) {   /* past fss */
         /* try to switch to the backup */
         if (audctl(AUD_SWITCH,(char *)NULL,(char *)NULL,0) != 0) {
            /* switch unsuccessful */
            fprintf(out_stream, AUDOMON_FS1, c);
            fprintf(out_stream, AUDOMON_FS2);
            fprintf(out_stream, AUDOMON_BK);
         } else {
            /* a switch just took place; check
             *   again to make sure things are ok.
             */
	     if (close(cntl_fd) < 0)
	        my_perror(out_stream, CNTL_FILE, "close", errno);
	     (void) fclose(out_stream);
             continue;
	 }
      } else if (c <= fss_warn) {   /* warning time */
         fprintf(out_stream, AUDOMON_FSW1);
         fprintf(out_stream, AUDOMON_FSW2, c);
      }
      /* check the audit file */
      /* search for the current audit file record
       * in the control file
       */
      if ((cntl_buflen = read(cntl_fd, (void *)cntl_buf, MAXCNTLFILE)) <= 0) {
         fprintf(out_stream, AUDOMON_CNTL1);
         fprintf(out_stream, AUDOMON_CNTL2, NL_CNTL_FILE);
	 fprintf(out_stream, "%s\n", strerror(errno));
         something_wrong++;
         goto take_a_nap;
      }
      cntl_buf[cntl_buflen] = '\0';
      fn1[0] = '\0';
      fn2[0] = '\0';
      afs1=0;
      afs2=0;

      /* XXX should check return value here */
      sscanf(cntl_buf, "%[^,],%d\n%[^,],%d\n", fn1, &afs1, fn2, &afs2);
      found = 0;
      if (strcmp(fn1, curr_audfile) == 0) {
         afs = afs1;
         found++;
      } else if (strcmp(fn2, curr_audfile) == 0) {
         afs = afs2;
         found++;
      }
      if (!found) {
         fprintf(out_stream, AUDOMON_CAF1);
         fprintf(out_stream, AUDOMON_CAF2, curr_audfile, NL_CNTL_FILE);
         something_wrong++;
         goto take_a_nap;
      }
      if (stat(curr_audfile, &fbuf) == -1) {
         fprintf (out_stream, AUDOMON_BADAF);
         something_wrong++;
         goto take_a_nap;
      }
      /* determine the size of the audit file and store in s */
      s = af_size (curr_audfile, &fbuf);

      s /= KBYTES; /* audit file size is in kbytes */
      if (s >= afs) { /* exceeded AFS */
         /* try to switch to the backup */
         if (audctl(AUD_SWITCH,(char *)NULL,(char *)NULL,0) != 0) {
            /* switch unsuccessful */
            fprintf(out_stream, AUDOMON_AF1, s);
            fprintf(out_stream, AUDOMON_AF2);
            fprintf(out_stream, AUDOMON_BK);
         } else {
            /* a switch just took place; check
             * again to make sure things are ok.
             */
	     if (close(cntl_fd) < 0)
		my_perror(out_stream, CNTL_FILE, "close", errno);
	     (void) fclose(out_stream);
             continue;
	 }
      } else {   /* do we need to warn ? */
         afs_warn = (float) warning * afs / 100.0;
         if (s >= afs_warn) { /* warning time */
            fprintf(out_stream, AUDOMON_AFW1);
            fprintf(out_stream, AUDOMON_AFW2, s);
         }
      }
take_a_nap:
      /* if some inconsistency is discovered, try again in 1 minute */
      checkagain = ((something_wrong) ?
                     SECS_PER_MIN : nextwakeup(afs, fss, s, c, sp_freq));
      if (verbose) {
		fprintf(out_stream, catgets(nlmsg_fd, NL_SETN, 25,
			"audomon: periodic check = %d seconds.\n"),
			checkagain);
      }
      if (close(cntl_fd) < 0)
	 my_perror(out_stream, CNTL_FILE, "close", errno);
      (void) fclose(out_stream);
      (void) sleep(checkagain);
   }   /* while (1) */
}   /* main */

/*******************************************************************************
 * nextwakeup - calculate the next wake-up time in seconds for audomon
 *              based on the parameters.
 ******************************************************************************/
unsigned
nextwakeup(afs, fss, s, c, sp_freq)
int afs, fss, s, c;
double sp_freq;
{
   float min, next, f;

   min = (float)c / fss;
   if (s != 0)   /* don't divide by zero */
      if ((f = (float)afs / s) < min)
         min = f;
   next = sp_freq * min * SECS_PER_MIN;
   if (next > MAXFREQ) {   /* don't sleep too long */
      next = MAXFREQ;
   } else if (next < MINFREQ) {   /* don't wakeup too soon */
      next = MINFREQ;
   }
   return(next);   /* the returned value is truncated to an integer */
}

/*******************************************************************************
 * af_size - determines the size of the audit file. If the audit file is a
 *           CDF, the size is the total of the CDF elements.
 ******************************************************************************/

#define ISDIR(mode) ( ((mode) & S_IFMT) == S_IFDIR )
#define ISCDF(mode) ( ISDIR(mode) && ( (mode) & S_ISUID ) )

af_size (af, afbuf)
char *af;
struct stat *afbuf;
{
   char af_plus[MAXPATHLEN+1];
   char element[2*MAXPATHLEN+1];
   int s, offset;
   DIR *af_dirp;
   struct direct *dp;
   struct stat buf;

   (void) strcpy(af_plus, af);
   (void) strcat(af_plus, "+");

   if (!stat(af_plus, &buf) && ISCDF(buf.st_mode)) {
      /* audit file is CDF */
      (void) strcpy(element, af_plus);
      (void) strcat(element, "/");
      offset = strlen(element);
      af_dirp = opendir(af_plus);
      s = 0;
      for (dp=readdir(af_dirp); dp != NULL; dp=readdir(af_dirp)) {
         (void) strcpy(element+offset, dp->d_name);
         if (strcmp(dp->d_name, "." ) == 0 ||
             strcmp(dp->d_name, "..") == 0)
         continue;
         stat(element, &buf);
         s += buf.st_size;
      }
      (void) closedir(af_dirp);
   } else   /* audit file is regular file */
      s = afbuf->st_size;
   return (s);
} /* af_size */

/*******************************************************************************
 * error - a routine to flush stdout before printing to stderr.
 ******************************************************************************/

#include <varargs.h>

/*VARARGS*/
int
error(format, va_alist)
char *format;
va_dcl
{
   va_list ap;

   va_start(ap);
	(void) fflush(stdout);
   _doprnt(format, ap, stderr);
   va_end(ap);
   return(ferror(stderr) ? (EOF) : (0));
}

void
my_perror(stream, desc1, desc2, err_no)
    char *desc1, *desc2;
    FILE *stream;
{
    char desc[256];

    if (desc2)
	 (void) sprintf(desc, "(%s:%s)", desc1, desc2);
    else (void) sprintf(desc, "(%s)", desc1);

    fprintf(stream, "%s%s: %s\n", progname, desc, strerror(err_no));
}
