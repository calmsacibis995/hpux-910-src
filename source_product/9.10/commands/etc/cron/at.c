#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <nl_types.h>
#include <locale.h>
#include <unistd.h>
#include "cron.h"

#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#endif

static char *AT_ID = "@(#) $Revision: 72.6 $";

#define NL_SETN	1


#define TMPFILE		"_at"	/* prefix for temporary files	*/

#define ATMODE		06444	/* Mode for creating files in ATDIR.
         Setuid bit on so that if an owner of a file gives that file
         away to someone else, the setuid bit will no longer be set.
         If this happens, atrun will not execute the file	*/

#define ROOT		0	/* user-id of super-user */
#define BUFSIZE		512	/* for copying files */
#define LINESIZE	130	/* for listing jobs */
#define	MAXTRYS		100	/* max trys to create at job file */

#ifdef V4FS
#	define BADSHELL	(catgets(nlmsg_fd,NL_SETN,1, "because your login shell isn't /usr/bin/sh, you can't use at"))
#	define WARNSHELL	(catgets(nlmsg_fd,NL_SETN,2, "warning: commands will be executed using /usr/bin/sh\n"))
#	define MAIL		"/usr/bin/mail"	/* mail program to use */
#else
#	define BADSHELL	(catgets(nlmsg_fd,NL_SETN,1, "because your login shell isn't /bin/sh, you can't use at"))
#	define WARNSHELL	(catgets(nlmsg_fd,NL_SETN,2, "warning: commands will be executed using /bin/sh\n"))
#	define MAIL		"/bin/mail"	/* mail program to use */
#endif /* V4FS */

#define CANTCD		(catgets(nlmsg_fd,NL_SETN,3, "can't change directory to the at directory"))
#define CANTCHOWN	(catgets(nlmsg_fd,NL_SETN,4, "can't change the owner of your job to you"))
#define CANTCREATE	(catgets(nlmsg_fd,NL_SETN,5, "can't create a job for you"))
#define INVALIDUSER	(catgets(nlmsg_fd,NL_SETN,6, "you are not a valid user (no entry in /etc/passwd)"))
#define	NONUMBER	(catgets(nlmsg_fd,NL_SETN,7, "proper syntax is:\n\tat -ln\nwhere n is a number"))
#define NOREADDIR	(catgets(nlmsg_fd,NL_SETN,8, "can't read the at directory"))
#define NOTALLOWED	(catgets(nlmsg_fd,NL_SETN,9, "you are not authorized to use at.  Sorry."))
#define NOTHING		(catgets(nlmsg_fd,NL_SETN,10, "nothing specified"))
#define PAST		(catgets(nlmsg_fd,NL_SETN,11, "it's past that time"))

#define USAGE           (catgets(nlmsg_fd,NL_SETN,26, "Usage:\nat [-m][-f file][-qqueue] time [date][[next| +increment] time_designation] job\nat -r job ...\nat -l [job ...]\n"))

/*  Use when ALL POSIX options are supported
#define USAGE           (catgets(nlmsg_fd,NL_SETN,26, "Usage:\nat [-m][-f file][-qqueue] time [date][[next| +increment] time_designation] job(s)\nat -r job(s)\nat -l -q queuename\nat -l [job(s)]\n"))
*/

#define CANTOPENPIPE    (catgets(nlmsg_fd,NL_SETN,29,"Unable to open pipe to mail\n"))

char identif[]="at: $Revision: 72.6 $, $Date: 94/12/19 13:43:21 $";

/*
	this data is used for parsing time
*/
#define	dysize(A)	(((A)%4) ? 365 : 366)
int	gmtflag = 0;
extern	time_t	timezone;
extern	char	*argp;
char	login[UNAMESIZE];
char	argpbuf[80];
char	pname[MAXPATHLEN];
char	pname1[MAXPATHLEN];
#ifdef SecureWare
char auditbuf[80];
#endif
time_t	when, now, gtime(), mktime();
struct	tm	*tp, at, rt, *localtime();
int	mday[12] =
{
	31,28,31,
	30,31,30,
	31,31,30,
	31,30,31,
};
int	mtab[12] =
{
	0,   31,  59,
	90,  120, 151,
	181, 212, 243,
	273, 304, 334,
};
int     dmsize[12] = {
	31,28,31,30,31,30,31,31,30,31,30,31};

/* end of time parser */

short	jobtype = ATEVENT;		/* set to 1 if batch job */
char	*tfname = (char *) NULL;
extern	char *xmalloc();
extern int per_errno;
time_t num();
nl_catd nlmsg_fd;
nl_catd nltime_fd;

/* structure used for passing messages from the
   at and crontab commands to the cron			*/
struct message msgbuf;

/*  getopt externs */
extern char *optarg;
extern int optind, opterr;

/*
 *    several routines need a buffer into whic to build error messages.
 *    This buffer will either be printed to stderr or mailed to the user
 *    depending on the presence of the "-m" run string option.
 */
static char error_msg_buf[200];

/*
 *   This option flag is global because the error print routine(s) need to be
 *   able to access it and I do not know all the call chains to pass it 
 *   through.
 */
static int m_flg=0;
/*
 *    To enable the sending of multi-line mail messages we will open pipe
 *    to "/usr/bin/mail" if the -m option is specified in the run string.
 *    We save the FILE descriptor in this global.  The routine notifyuser
 *    will determine is the I/O should be written to the passed in FILE
 *    descriptor (the default) or to "pipe" (if -m specified).
 */
static FILE *mail_pipe;

/*
 *   m_flg is a global flag set if the user specified the "-m" run string
 *    option.
 *
 *   login is the user name string saved at the begining of main
 */
void
notifyuser(fd,msg)
FILE *fd;
char *msg;
{
    if ((m_flg == 0) || (mail_pipe == (FILE *)NULL))
    {
       fprintf(fd,"at: %s\n",msg);
       fflush(fd);
    }
    else
    {
       fprintf(mail_pipe, "at: %s\n",msg);
    }
    return;
}

/*
 *    remove:  scan the /usr/spool/cron/atjobs directory and remove any 
 *             files which exist.
 *
 */
static int removejobs(user,argc,argv)
int user,argc;
char **argv;
{
    struct stat buf;
    int err=0;

    /* remove jobs that are specified */
    if (chdir(ATDIR)==-1) atabort(CANTCD);
    for (; optind<argc; optind++)
    {
        if (stat(argv[optind],&buf)) 
        {
             sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,12, "%s does not exist\n")),argv[optind]);
             notifyuser(stderr,error_msg_buf);
             err=1;
        }
        else if ((user!=buf.st_uid) && (user!=ROOT)) 
        {
#ifdef SecureWare
             if (ISSECURE) 
	     {
                sprintf(auditbuf, "remove job %s", argv[optind]);
                audit_subsystem(auditbuf,
                     "not removed because user does not own job", ET_SUBSYSTEM);
             }
#endif
             sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,13, "you don't own %s\n")),argv[optind]);
	     notifyuser(stderr,error_msg_buf);
             err=1;
             return(err);
        }
    
        /*
         *    send a message to cron to delete the job from its job lists
         */
        atsendmsg(DELETE,argv[optind]);
        unlink(argv[optind]);
    
#ifdef AUDIT
        /*
         * chdir() back and forth because
         * it's easier and probably cheaper
         * than xmalloc() and sprintf().
         */
        if (chdir(ATAIDS) == 0) 
        {
            (void)unlink(argv[optind]);
        }
        if (chdir(ATDIR) != 0) 
        {
            atabort(CANTCD);
        }
#endif /* AUDIT */

    }
    return(err);
}

/*
 *   report: Report all jobs scheduled for the invoking user if no job_list
 *           was specified in the command line.  If a job_list was specified,
 *           report only information for those jobs.  If a queue was
 *           specified, limit the search to that queue only.
 *
 ***** Code to limit to given queue needs to be added
 */
static void reportjobs(user,queue,argc,argv)
int user,queue,argc;
char **argv;
{
    struct dirent *dentry;
    struct stat buf;
    DIR	*dir;
    char *ptr;
    time_t timebuf;
    struct passwd *pw, *getpwuid();

    /* list jobs for user */
    if (chdir(ATDIR)==-1) atabort(CANTCD);
    if (optind == argc)
    {
	/*
	 ******* this still needs code to limit to a specific queue
	 */
        /* list all jobs for a user */
        if ((dir=opendir("."))==NULL) atabort(NOREADDIR);
/*
Machine dependent:      lseek(dir,(long)2*sizeof(dentry),0);
*/
        for (;;) 
        {
            if ((dentry=readdir(dir))==NULL)
                break;    /* end of directory */
            if (dentry->d_ino==0) continue;
            if (stat(dentry->d_name,&buf)) 
	    {
                unlink(dentry->d_name);
                continue;
            }
            if ((user!=ROOT) && (buf.st_uid!=user))
                continue;
            ptr = dentry->d_name;
            if (((timebuf=num(&ptr))==0) || (*ptr!='.'))
                continue;
            if ((user==ROOT) && ((pw=getpwuid(buf.st_uid))!=NULL))
                printf((catgets(nlmsg_fd,NL_SETN,32,"user = %s\t%s\t%s\n")),pw->pw_name,dentry->d_name,nl_ascxtime(localtime(&timebuf),""));
            else    printf("%s\t%s\n",dentry->d_name,nl_ascxtime(localtime(&timebuf),""));
        }
        closedir(dir);
    }
    else    /* list particular jobs for user */
    {
        for (; optind<argc; optind++) 
        {
            ptr = argv[optind];
            if (((timebuf=num(&ptr))==0) || (*ptr!='.'))
	    {
                sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,14, "invalid job name %s\n")),argv[optind]);
	        notifyuser(stderr,error_msg_buf);
	    }
            else if (stat(argv[optind],&buf))
	    {
                sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,15, "%s does not exist\n")),argv[optind]);
	        notifyuser(stderr,error_msg_buf);
	    }
            else if ((user!=buf.st_uid) && (user!=ROOT))
	    {
                sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,16, "you don't own %s\n")),argv[optind]);
	        notifyuser(stderr,error_msg_buf);
	    }
            else
	    {
                sprintf(error_msg_buf,"%s\t%s\n",argv[optind],nl_ascxtime(localtime(&timebuf),""));
	        notifyuser(stdout,error_msg_buf);
	    }
        }
    }
    return;
}

main(argc,argv,environ)
int argc;
char **argv;
char **environ;
{
	int user,i,fd,atcatch();
	char *job,pid[6];
	char *pp, *temp, *xmalloc();
	char *nl_ascxtime(),*strcat(),*strcpy(),*strrchr();
	char *mkjobname(),*getuser();
	char *lang;
	time_t time();
	struct locale_data *ld;
	char tmplang[SL_NAME_SIZE+1];
	int  st = 1;
	int errflg=0;     /* cmnd line parsing error counter/flag */
	int f_flg=0;      /* none of the options have been seen yet */
	int q_flg=0;
	int t_flg=0;
	int r_flg=0;
	int l_flg=0;
	char *infile=(char *)NULL;     /* pointer to input file name */

	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale(),stderr);
		nlmsg_fd=(nl_catd)-1;
	}
	else {
	      nlmsg_fd=catopen("at",0);
	      ld=getlocale(LOCALE_STATUS);

		/* if $LC_TIME is different then $LANG */

	      if (strcmp(ld->LC_ALL_D,ld->LC_TIME_D)) {
		   sprintf(tmplang,"LANG=%s",ld->LC_TIME_D);
		   putenv(tmplang);
		   nltime_fd=catopen("at",0);
		   sprintf(tmplang,"LANG=%s",ld->LC_ALL_D);
		   putenv(tmplang);
	      }
	      else
		  nltime_fd=nlmsg_fd;
	}
#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
            at_check_and_set_user(&pp, &user);
	}
	else
	    pp = getuser((user=getuid()));
#else
	pp = getuser((user=getuid()));
#endif
	if(pp == NULL) {
		if(per_errno == 2)
			atabort(BADSHELL);
		else {
#ifdef SecureWare
			if (ISSECURE)
                        	audit_subsystem(
				    "get user entry from /etc/passwd",
                                    "no entry, abort at", ET_SUBSYSTEM);
#endif
			atabort(INVALIDUSER);
		}
	}
	strcpy(login,pp);
#ifdef SecureWare
	if (!allowed(login,ATALLOW,ATDENY)) {
		if (ISSECURE)
                	audit_subsystem("verify user allowed to use at",
                            "not allowed, abort at", ET_RES_DENIAL);
                atabort(NOTALLOWED);
	}
#else
	if (!allowed(login,ATALLOW,ATDENY)) atabort(NOTALLOWED);
#endif

	/* 
	 *   options m,t  to be implmented for POSIX draft 7
	 *     the draft is confusing on what -m does.
	 *     one place says if -m specified mail ALL responces to the user 
	 *       even if stdout and stderr are redirected.
	 *     another place says unless stdout and stderr are redirected.
	 *     I have implmented the first version and comment out the
	 *       setting of m_flg in the getopt "while" loop.  This has 
	 *       the effect of disabling the -m option.
	 *
	 *   options t and r may have multiple arguments.
	 *   the POSIX standard shows that they MUST be the last option on 
	 *     the line.  
	 */
	while ((i = getopt(argc,argv,"mf:q:trl")) != EOF)
	  switch(i) {
                case '?':
		    errflg++;  /* unknown option */
		    break;

		case 'm':
		    if (m_flg != 0)  /* have we already had a -m option?  */
		    {
			errflg++;
			break;
		    }
		    m_flg++;
	            /*
	             *    if the pipe call fails to open a pipe the 
	             *    messages will end up on stdout/stderr.
	             */
	            temp = xmalloc(strlen(MAIL)+strlen(login)+2);
	            strcat(strcat(strcpy(temp,MAIL)," "),login);
	            mail_pipe = popen(temp,"w");
	            if (mail_pipe == (FILE *)NULL) 
	               notifyuser(CANTOPENPIPE);
                    free(temp);
		    break;

		case 'f':
		    if (f_flg != 0)  /* have we already had a -f option?  */
		    {
			errflg++;
			break;
		    }
		    f_flg++;

		    infile=optarg;  /* save pointer to argv of file name */
		    break;

	        case 'q': 
		    if (q_flg != 0)  /* have we already had a -q option?  */
		    {
			errflg++;
			break;
		    }
		    q_flg++;

		    /*
		     * Fix for DSDe417677. Check if the queue specified by the
		     * user falls within the legal range ie i between 'a' and
		     *  'y'.
		     */

		    if ((*optarg < 'a') || (*optarg > 'y'))
			atabort((catgets(nlmsg_fd,NL_SETN,31, "queue should be in the range 'a' through 'y'")));
		    if(*optarg == 'c')
			atabort((catgets(nlmsg_fd,NL_SETN,25, "can't use c queue ")));
		    jobtype = *optarg - 'a';
	            break;

                case 'l': 
		    if (l_flg != 0)  /* have we already had a -l option?  */
		    {
			errflg++;
			break;
		    }
		    l_flg++;
                    break;

                case 'r': 
		    if (r_flg != 0)  /* have we already had a -r option?  */
		    {
			errflg++;
			break;
		    }
		    r_flg++;

                    break;
            
            
	}
	if (errflg) atabort(USAGE);
	/*
	 *   check for illegal option combinations
	 *     'l' can only be used with 'q'
	 *     'r' must be the only option
	 */
        if ((l_flg != 0) && 
	   ((m_flg !=0) || (f_flg !=0) || (t_flg !=0) || (r_flg !=0)))
	    atabort(USAGE);
	/*  we checked 'r' and 'l' above  */
        if ((r_flg != 0) && 
	   ((m_flg !=0) || (f_flg !=0) || (t_flg !=0) || (q_flg !=0)))
	    atabort(USAGE);
	    
	/*
	 *   go remove the jobs and then exit
	 */
        if (r_flg !=0) 
	    exit(removejobs(user,argc,argv));
        
	/*
	 *  report on scheduled jobs
	 */
        if (l_flg !=0)
	{
	    if (q_flg != 0) 
	       reportjobs(user,jobtype,argc,argv);
	    else
	       reportjobs(user,-1,argc,argv);
	    exit(0);
	}

	/* figure out what time to run the job */

	if(argc == 1 && jobtype != BATCHEVENT)
		atabort(NOTHING);
	time(&now);
	if(jobtype != BATCHEVENT) {	/* at job */
		argp = argpbuf;
		while(optind < argc) {
			strcat(argp,argv[optind]);
			strcat(argp, " ");
			optind++;
		}
		tp = localtime(&now);
		mday[1] = 28 + leap(tp->tm_year);
#ifdef NLS
		parsedatetime();
#else
		yyparse();
#endif
		atime(&at, &rt);
		/*
		 *   is the time GMT or local?
		 */
		if(!gmtflag) {
		   at.tm_isdst = -1; /* Ask mktime to determine if day light
					savings is effective. */
		   when = mktime(&at);
		} else
		   when = gtime(&at);
	} else		/* batch job */
		when = now;
	if(when < now)	/* time has already past */
		atabort((catgets(nlmsg_fd,NL_SETN,18, "too late")));

	sprintf(pid,"%-5d",getpid());
	tfname=xmalloc(strlen(ATDIR)+strlen(TMPFILE)+10);
	strcat(strcat(strcat(strcpy(tfname,ATDIR),"/../"),TMPFILE),pid);
	/* catch SIGINT, HUP, and QUIT signals */
	if (signal(SIGINT, atcatch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP, atcatch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,atcatch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,atcatch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
#ifdef SecureWare
	if(ISSECURE)
            fd = at_create_file(tfname, user, ATMODE, CANTCREATE, CANTCHOWN);
	else{
	    if((fd = open(tfname,O_CREAT|O_EXCL|O_WRONLY,ATMODE)) < 0)
	    {
		sprintf(error_msg_buf,catgets(nlmsg_fd,NL_SETN,17,"open \'%s\' failed, errno=%d\n"),tfname,errno);
	        notifyuser(stderr,error_msg_buf);
		atabort(CANTCREATE);
	    }
	    if (chown(tfname,user,getgid())==-1) {
		unlink(tfname);
		atabort(CANTCHOWN);
	    }
	    chmod(tfname, ATMODE);  /* above chown might reset setuid bit */
	}
#else
	if((fd = open(tfname,O_CREAT|O_EXCL|O_WRONLY,ATMODE)) < 0)
	{
		sprintf(error_msg_buf,catgets(nlmsg_fd,NL_SETN,17,"open \'%s\' failed, errno=%d\n"),tfname,errno);
	        notifyuser(stderr,error_msg_buf);
		atabort(CANTCREATE);
	}
	if (chown(tfname,user,getgid())==-1) {
		unlink(tfname);
		atabort(CANTCHOWN);
	}
	chmod(tfname, ATMODE);  /* above chown might reset setuid bit */
#endif
	close(1);
	dup(fd);	/* note stdout(fd=1) now points to tfname */
	close(fd);
	sprintf(pname,"%s",PROTO);
	sprintf(pname1,"%s.%c",PROTO,'a'+jobtype);
	copy(infile);
	fflush(stdout);
#ifndef hp9000s500
	fchmod(1,ATMODE);
#endif
#ifdef SecureWare
	if(ISSECURE)
            at_set_file_protection(stdout, tfname);
#endif
	close(1);
	job=mkjobname(tfname,when);
	unlink(tfname);

#ifdef AUDIT
    {
	auto	FILE *		wfp = (FILE *)0;
	auto	char *		jobname = (char *)0;
	auto	char *		jobptr = (char *)0;
	auto	int		wfd = 0;
	auto	int		jobnamelen = 0;
	auto	aid_t		aid = (aid_t)-1;
	auto    struct passwd * pw = (struct passwd *)0;


	/*
	 * if we can't get the audit ID, don't bother
	 * doing any more work -- a missing audit ID
	 * file is equivalent to an invalid (-1) aid.
	 */

	pw = getpwnam(login);
	aid = pw->pw_audid;

	if (aid != (aid_t)-1) {
		/*
		 * construct a file name to contain the audit ID.
		 * jobptr points to the basename of the job.  we
		 * do this so we don't have to keep doing strrchr()
		 * all over the place.  jobnamelen saves the length
		 * so we know how much to xmalloc().  jobname
		 * contains the file name.  2 is added to xmalloc()
		 * for the '/' and NULL terminator.
		 */
		jobptr = strrchr(job, '/') + 1;
		jobnamelen = strlen(jobptr);
		jobname = xmalloc(strlen(ATAIDS) + jobnamelen + 2);
		(void)sprintf(jobname, "%s/%s", ATAIDS, jobptr);

		/*
		 * create file, fix up modes, fix up owner
		 */
		if ((wfd = open(jobname, O_CREAT|O_EXCL|O_WRONLY, 0)) != -1) {
			(void)fchmod(wfd, S_IRUSR);
			(void)fchown(wfd, 0, -1);

			/*
			 * get a file pointer so we don't have to
			 * figure out how much space to xmalloc()
			 * for the audit ID (also so we can make it
			 * text rather than binary -- may as well
			 * make it easier to modify with an editor
			 * if the need ever arises).
			 */
			if ((wfp = fdopen(wfd, "w")) != (FILE *)0) {
				(void)fprintf(wfp, "%ld\n", aid);
			}
			(void)fclose(wfp);
			(void)close(wfd);	/* necessary if fdopen() failed */
		}
	}
    }
#endif /* AUDIT */

	/*
	 *   has user asked to have stdout/stderr mailed to them??
	 */
	if ((m_flg == 0) || (mail_pipe == (FILE *)NULL))
	{
	    /*
	     *   NO, dump it where ever the user pointed it.
	     */
	    atsendmsg(ADD,strrchr(job,'/')+1);

	    if(per_errno == 2)
		fprintf(stderr,WARNSHELL);

	    fprintf(stderr,catgets(nlmsg_fd,NL_SETN,30, "job %s at %.44s\n"),strrchr(job,'/')+1,nl_cxtime(&when,""));
#ifdef BADCODE
	    /*
	     *    the following "if" statement was inherited from AT&T.
	     *    "t" never contains a value we get here.  In the AT&T code
	     *    "t" was only used in the "list jobs" portion of the code.
	     *    That code did an exit before it reached this point.
	     *    Therefore, the print statement never occurred.  I have
	     *    #ifdef'ed it out so the code will compile.
	     */
	    if (when-now-MINUTE < HOUR) fprintf(stderr,
	        (catgets(nlmsg_fd,NL_SETN,19, "this job may not be executed at the proper time.\n")));
#endif
	}
	else
	{
	    /*
	     *   They asked to have it mailed to them.
	     *
	     *    This section of code should needs code to correctly function
	     *    under the #if defined(SecureWare) && defined(B1).
	     */
	    char *i,*strrchr();

	    atsendmsg(ADDNOTIFY,strrchr(job,'/')+1);

	    if(per_errno == 2)
	        fprintf(mail_pipe, "\nAt: %s",WARNSHELL);

	    fprintf(mail_pipe,catgets(nlmsg_fd,NL_SETN,30, "At: job %s at %.44s\n"),strrchr(job,'/')+1,nl_cxtime(&when,""));

#ifdef BADCODE
	    /*
	     *    the following "if" statement was inherited from AT&T.
	     *    "t" never contains a value we get here.  In the AT&T code
	     *    "t" was only used in the "list jobs" portion of the code.
	     *    That code did an exit before it reached this point.
	     *    Therefore, the print statement never occurred.  I have
	     *    #ifdef'ed it out so the code will compile.
	     */
	    if (when-t-MINUTE < HOUR) 
	        fprintf(mail_pipe,(catgets(nlmsg_fd,NL_SETN,19,"At: this job may not be executed at the proper time.\n")));
#endif
	}
        if ((m_flg != 0) && (mail_pipe != NULL)) pclose(mail_pipe);
	catclose(nlmsg_fd);
	exit(0);
}



/*
 *   This routine creates the name for the job.  On a multi-processor system
 *   and on a uni-processor system with a lot of at requests happening at the 
 *   same time doing a stat and later trying to do a link puts a "large" 
 *   window between the stat and the link request.  Another copy of "at" can 
 *   get in during that window and do the link first.  This routine is being 
 *   changed to do the link instead of a stat followed by a link request in 
 *   the caller.
 */
/****************/
char *mkjobname(tfname,timebuf)
char *tfname;
/****************/
time_t timebuf;
{
	int i;
	char *name;
	struct  stat buf;
	name=xmalloc(200);
	for (i=0;i < MAXTRYS;i++) {
		sprintf(name,"%s/%ld.%c",ATDIR,timebuf,'a'+jobtype);
		if (link(tfname,name)==0)
			return(name);
		timebuf += 1;
	}
	unlink(tfname);
	atabort((catgets(nlmsg_fd,NL_SETN,20, "queue full")));
}


/****************/
atcatch()
/****************/
{
	unlink(tfname);
        if ((m_flg != 0) && (mail_pipe != (FILE *)NULL)) pclose(mail_pipe);
	exit(1);
}


/****************/
atabort(msg)
/****************/
char *msg;
{
        notifyuser(stderr,msg);
        if ((m_flg != 0) && (mail_pipe != (FILE *)NULL)) pclose(mail_pipe);
	exit(1);
}

/*
 * add time structures logically
 */
atime(a, b)
register
struct tm *a, *b;
{
	if ((a->tm_sec += b->tm_sec) >= 60) {
		b->tm_min += a->tm_sec / 60;
		a->tm_sec %= 60;
	}
	if ((a->tm_min += b->tm_min) >= 60) {
		b->tm_hour += a->tm_min / 60;
		a->tm_min %= 60;
	}
	if ((a->tm_hour += b->tm_hour) >= 24) {
		b->tm_mday += a->tm_hour / 24;
		a->tm_hour %= 24;
	}
	a->tm_year += b->tm_year;
	if ((a->tm_mon += b->tm_mon) >= 12) {
		a->tm_year += a->tm_mon / 12;
		a->tm_mon %= 12;
	}
	a->tm_mday += b->tm_mday;
	while (a->tm_mday > mday[a->tm_mon]) {
		a->tm_mday -= mday[a->tm_mon++];
		if (a->tm_mon > 11) {
			a->tm_mon = 0;
			mday[1] = 28 + leap(++a->tm_year);
		}
	}
}

leap(year)
{
	return year % 4 == 0;
}

/*
 * return time from time structure
 */
time_t
gtime(tp)
register
struct	tm *tp;
{
	register i;
	long	tv;
	extern int dmsize[];

	tv = 0;
	for (i = 1970; i < tp->tm_year+1900; i++)
		tv += dysize(i);
	if (dysize(tp->tm_year) == 366 && tp->tm_mon >= 2)
		++tv;
	for (i = 0; i < tp->tm_mon; ++i)
		tv += dmsize[i];
	tv += tp->tm_mday - 1;
	tv = 24 * tv + tp->tm_hour;
	tv = 60 * tv + tp->tm_min;
	tv = 60 * tv + tp->tm_sec;
	return tv;
}

/*
 * make job file from proto + stdin
 */
copy(infile)
char *infile;
{
	register c;
	register FILE *pfp;
	register FILE *xfp;
	char    dirbuf[MAXPATHLEN];
	register char **ep;
	char *strchr();
	long ulimit();
	mode_t umask();
	mode_t um;
	char *val;
	extern char **environ;

	printf(": %s job\n",jobtype ? "batch" : "at");
	for (ep=environ; *ep; ep++) {
		if ((val=strchr(*ep,'='))==NULL)
			continue;
		*val++ = '\0';
		printf("export %s; %s=", *ep, *ep);
		print_quoted(val);	/* print value with backslashes */
		printf("\n");
		*--val = '=';
	}
	if((pfp = fopen(pname1,"r")) == NULL && (pfp=fopen(pname,"r"))==NULL) {
		unlink(tfname);
		atabort((catgets(nlmsg_fd,NL_SETN,21, "no prototype")));
	}

#ifdef SecureWare
	if(ISSECURE)
            um = at_secure_mask();
	else
	    um = umask(0);
#else
	um = umask(0);
#endif
	while ((c = getc(pfp)) != EOF) {
		if (c != '$')
			putchar(c);
		else switch (c = getc(pfp)) {
		case EOF:
			goto out;
		case 'd':
			dirbuf[0] = NULL;
			/*
			 * thwart attempts to fool popen with bogus IFS
			 */
			(void)putenv("IFS= \t\n");
#		ifdef V4FS
			if((xfp=popen("/usr/bin/pwd","r")) != NULL) 
#		else
			if((xfp=popen("/bin/pwd","r")) != NULL) 
#		endif /* V4FS */
				{
				fscanf(xfp,"%s",dirbuf);
				pclose(xfp);
			}
			printf("%s", dirbuf);
			break;
		case 'l':
			printf("%ld",ulimit(1,-1L));
			break;
		case 'm':
			printf("%o", um);
			break;
		case '<':
			if (infile == (char *)NULL)
			{
			    while ((c = getchar()) != EOF) {
				putchar(c);
			    }
			}
			else
			{
			   register FILE *infp;

                           if (access(infile,R_OK)) {
                               sprintf(error_msg_buf,"%s - %s",
                                                        strerror(errno),infile);
			       unlink(tfname);
                               atabort(error_msg_buf);
                           }

		           if((infp = fopen(infile,"r")) == (FILE *)NULL )
			   {
			       sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,28,"unable to open \'%s\'\n"),infile));
			       unlink(tfname);
			       atabort(error_msg_buf);
			   }
			   while ((c = getc(infp)) != EOF) {
				putchar(c);
			   }
			}
			break;
		case 't':
			printf(":%lu", when);
			break;
		default:
			putchar(c);
		}
	}
out:
	fclose(pfp);
}


/*
 * Print a string with adequate quoting.
 */
print_quoted(s)
char *s;
{
	char c;
	while (c = *s++) {
		if (c=='\n')
			printf("'\n'");
		else if (isalnum(c) || strchr("./,_-:%", c)!=NULL)
			putchar(c);
		else
			printf("\\%c", c);
	}
}



/****************/
atsendmsg(action,fname)
/****************/
char action;
char *fname;
{

	static	int	msgfd = -2;
	struct	message	*pmsg;

#if defined(SecureWare) && defined(B1)
	if(ISB1){
            pmsg = (struct message *) at_set_message((char *) &msgbuf,
                        sizeof(*pmsg) - sizeof(pmsg->mlevel));
            if (pmsg == (struct message *) 0)
                return;
	}
	else
	    pmsg = &msgbuf;
#else
	pmsg = &msgbuf;
#endif
	if(msgfd == -2)
#if defined(SecureWare) && defined(B1)
        {
		if (ISB1)
                	disablepriv(SEC_SUSPEND_AUDIT);
#endif
		/* Change it back to NDELAY to detect if cron isn't running 
		   and then use fcntl later to turn it off. */
		if((msgfd = open(FIFO,O_WRONLY|O_NDELAY)) < 0) {
			if(errno == ENXIO || errno == ENOENT)
			{
				sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,22, "cron may not be running - call your system administrator\n")));
		                notifyuser(stderr,error_msg_buf);
			}
			else
			{
				sprintf(error_msg_buf,catgets(nlmsg_fd,NL_SETN,
23, "error in message queue open, errno=%d\n"),errno);
		                notifyuser(stderr,error_msg_buf);
			}
			return;
		}
#if defined(SecureWare) && defined(B1)
                if (ISB1)
			enablepriv(SEC_SUSPEND_AUDIT);
        }
#endif
	/* Change the FIFO pipe to turn off NDELAY */
	(void) fcntl(msgfd, F_SETFL, fcntl(msgfd, F_GETFL, 0) & ~O_NDELAY); 
	pmsg->etype = AT;
	pmsg->action = action;
	strncpy(pmsg->fname,fname,FLEN);
	strncpy(pmsg->logname,login,LLEN);
#if defined(SecureWare) && defined(B1)
	if((ISB1) ? (!at_write_message(msgfd, (char *) pmsg,
                                   sizeof(*pmsg) - sizeof(pmsg->mlevel))) :
	   (write(msgfd,pmsg,
			sizeof(struct message)) != sizeof(struct message)))
#else
	if(write(msgfd,pmsg,sizeof(struct message)) != sizeof(struct message))
#endif
        {
#ifdef SecureWare
                if (ISSECURE)
			audit_subsystem("send message to cron daemon",
                            "no message sent to cron daemon", ET_SUBSYSTEM);
#endif
		sprintf(error_msg_buf,(catgets(nlmsg_fd,NL_SETN,24, "error in message send\n")));
		notifyuser(stderr,error_msg_buf);
	}

}
