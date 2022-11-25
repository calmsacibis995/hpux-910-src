#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef AUDIT
#include <sys/audit.h>
#endif /* AUDIT */
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <nl_types.h>
#include "cron.h"
#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#endif
#ifdef DEBUG
#include <stdio.h>
#endif

#ifdef	FSS
#include <sys/fss.h>
#endif

/*added for att patch */
#ifdef AUDIT
#define SECUREPASS      "/.secure/etc/passwd"  /* shadow password file */
int secure;          /* flag to denote the existance of secure passwd file */
struct stat s_pfile;
#endif /* AUDIT */

#include <sys/wait.h>

static char *CRON_ID = "@(#) $Revision: 70.8.2.1 $";

#define MAIL		"/bin/mail"	/* mail program to use */
#define CONSOLE		"/dev/console"	/* where to write error messages when cron dies	*/

#define TMPINFILE	"/tmp/crinXXXXXX"  /* file to put stdin in for cmd  */
#define	TMPDIR		"/tmp"
#define	PFX		"crout"
#define TMPOUTFILE	"/tmp/croutXXXXXX" /* file to place stdout, stderr */
#define CHILDLOG        "/usr/lib/cron/childlog."

#define INMODE		00400		/* mode for stdin file	*/
#define OUTMODE		00600		/* mode for stdout file */
#define ISUID		06000		/* mode for verifing at jobs */

#define INFINITY	2147483647L	/* upper bound on time	*/

#ifdef FSS
#  define CUSHION	600L            /* allow a 5 minute cushion */
#else
#  define CUSHION	120L
#endif

#define	MAXRUN		40		/* max total jobs allowed in system */
#define ZOMB		100		/* proc slot used for mailing output */

#define	JOBF		'j'
#define	NICEF		'n'
#define	USERF		'u'
#define WAITF		'w'

#define BCHAR		'>'
#define	ECHAR		'<'

#define	DEFAULT		0
#define	LOAD		1
#define NL_SETN		1
#define BADCD		(catgets(nlmsg_fd,NL_SETN,1, "can't change directory to the crontab directory."))
#define NOREADDIR	(catgets(nlmsg_fd,NL_SETN,2, "can't read the crontab directory."))

#define BADJOBOPEN	(catgets(nlmsg_fd,NL_SETN,3, "unable to read your at job."))
#define BADSHELL	(catgets(nlmsg_fd,NL_SETN,4, "because your login shell isn't /bin/sh, you can't use cron."))
#define BADSTAT		(catgets(nlmsg_fd,NL_SETN,5, "can't access your crontab file.  Resubmit it."))
#define CANTCDHOME	(catgets(nlmsg_fd,NL_SETN,6, "can't change directory to your home directory.\nYour commands will not be executed."))
#define CANTEXECSH	(catgets(nlmsg_fd,NL_SETN,7, "unable to exec the shell for one of your commands."))
#define EOLN		(catgets(nlmsg_fd,NL_SETN,8, "unexpected end of line"))
#define NOREAD		(catgets(nlmsg_fd,NL_SETN,9, "can't read your crontab file.  Resubmit it."))
#define NOSTDIN		(catgets(nlmsg_fd,NL_SETN,10, "unable to create a standard input file for one of your crontab commands.\nThat command was not executed."))
#define OUTOFBOUND	(catgets(nlmsg_fd,NL_SETN,11, "number too large or too small for field"))
#define STDERRMSG	(catgets(nlmsg_fd,NL_SETN,12, "\n\n*************************************************\nCron: The previous message is the standard output\n      and standard error of one of your cron commands.\n\n"))
#define STDERRMSG2	(catgets(nlmsg_fd,NL_SETN,13, "\n\n*************************************************\nCron: The previous message is the standard output\n      and standard error of one of your crontab commands:\n\n"))
#define STDERRMSG3	(catgets(nlmsg_fd,NL_SETN,14, "\n\n*************************************************\nCron: The previous message is the standard output\n      and standard error of one of your at commands.\n\n"))
#define STDOUTERR	(catgets(nlmsg_fd,NL_SETN,15, "one of your commands generated output or errors, but cron was unable to mail you this output.\nRemember to redirect standard output and standard error for each of your commands."))
#define UNEXPECT	(catgets(nlmsg_fd,NL_SETN,16, "unexpected symbol found"))

#ifdef AUDIT
#define	NOAUDID		(catgets(nlmsg_fd,NL_SETN,69, "Your job did not contain a valid audit ID.\nSee your system administrator.\n"))
#endif /* AUDIT */

struct event 
{	
	time_t time;	/* time of the event	*/
	short  etype;	/* what type of event; 0=cron, 1=at	*/
	char   *cmd;	/* command for cron, job name for at	*/
	struct usr *u;	/* ptr to the owner (usr) of this event	*/
	struct event *link; /* ptr to another event for this user */
	char   notify_user; /* 'y' send mail to user when job completes */
	union { 
		struct { /* for crontab events */
			char *minute;	/*  (these	*/
			char *hour;	/*   fields	*/
			char *daymon;	/*   are	*/
			char *month;	/*   from	*/
			char *dayweek;	/*   crontab)	*/
			char *input;	/* ptr to stdin	*/
		} ct;
		struct { /* for at events */
			short exists;	/* for revising at events	*/
			int eventid;	/* for el_remove-ing at events	*/
		} at;
	} of; 
};

struct usr 
{	
	char *name;	/* name of user (e.g. "kew")	*/
	char *home;	/* home directory for user	*/
	int uid;	/* user id	*/
	int gid;	/* group id	*/
#ifdef AUDIT
	aid_t audid;	/* audit id */
	short audproc;	/* audit process flag */
#endif /* AUDIT */
#ifdef ATLIMIT
	int aruncnt;	/* counter for running jobs per uid */
#endif
#ifdef CRONLIMIT
	int cruncnt;	/* counter for running cron jobs per uid */
#endif
	int ctid;	/* for el_remove-ing crontab events */
	short ctexists;	/* for revising crontab events	*/
	struct event *ctevents;	/* list of this usr's crontab events */
	struct event *atevents;	/* list of this usr's at events */
	struct usr *nextusr; 
#if defined(SecureWare) && defined(B1)
        char *seclevel_ir; /* sensitivity level of job */
#endif
};	/* ptr to next user	*/

struct	queue
{
	int njob;	/* limit */
	int nice;	/* nice for execution */
	int nwait;	/* wait time to next execution attempt */
	int nrun;	/* number running */
}	
	qd = {100, 2, 60},		/* default values for queue defs */
	qt[NQUEUE];

struct	queue	qq;
int	wait_time = 60;
int	lognum=0;

struct	runinfo
{
	int	pid;
	short	que;
	struct  usr *rusr;	/* pointer to usr struct */
	char	*rcmd;		/* running command	 */
	char 	*outfile;	/* file where stdout & stderr are trapped */
	char    notify_user; /* 'y' send mail to user when job completes */
}	rt[MAXRUN];

int msgfd;		/* file descriptor for fifo queue */
int ecid=1;		/* for giving event classes distinguishable id names 
			   for el_remove'ing them.  MUST be initialized to 1 */
short jobtype;		/* at or batch job */
int delayed;		/* is job being rescheduled or did it run first time */
int notexpired;		/* time for next job has not come */
int cwd;		/* current working directory */
int running;		/* zero when no jobs are executing */
struct event *next_event;	/* the next event to execute	*/
struct usr *uhead;	/* ptr to the list of users	*/
struct usr *ulast;	/* ptr to last usr table entry */
time_t init_time,num(),time();
char *strcpy(),*strncpy(),*strcat();
extern char *xmalloc();

nl_catd	nlmsg_fd;

#ifdef SecureWare
char auditbuf[CTLINESIZE+100];
static int last_was_del; /* fixes delay bug in original code */
#endif

/* user's default environment for the shell */
char homedir[100]="HOME=";
char log_name[50]="LOGNAME=";
char timez[50]="TZ=";
char *envinit[]={
	homedir,log_name,timez,
	"PATH=/bin:/usr/bin:.",
	"SHELL=/bin/sh",0};
extern char **environ;

/* structure used for passing messages from the
   at and crontab commands to the cron			*/
struct message msgbuf;

/*
 * fulltime() -- print the full time, including timezone string in
 *               the format identical to that of the "date" command.
 */
/****************/
char *
fulltime(pt)
/****************/
time_t *pt;
{
    extern char *ctime();
    extern struct tm *localtime();

    static char buf[40];
    char year[6];
    struct tm *tm;

    (void *)strcpy(buf, ctime(pt));
    strncpy(year+1, buf+20, 4);
    year[0] = ' ';
    year[5] = '\0';

    tm = localtime(pt);
    (void *)strcpy(buf+20, tzname[tm->tm_isdst]);
    (void *)strcat(buf+20, year);
    return buf;
}

/*************/
main(argc,argv)
/*************/
char **argv;
{
	time_t t,t_old;
	time_t last_time;
	time_t ne_time;		/* amt of time until next event execution */
	time_t next_time();
	time_t lastmtime = 0L;
	struct usr *u,*u2;
	struct event *e= ((struct event *)~NULL);
	struct event *e2,*eprev;
	struct stat buf;
	long seconds;
	int rfork;
	struct runinfo *rp;

#ifdef SecureWare
	if (ISSECURE)
            set_auth_parameters(argc, argv);

#ifdef B1
	if (ISB1) {
            initprivs();
            (void) forcepriv(SEC_LIMIT);
            (void) forcepriv(SEC_SUID);
            (void) forcepriv(SEC_ALLOWMACACCESS);
            (void) forcepriv(SEC_ALLOWDACACCESS);
            (void) forcepriv(SEC_OWNER);
            (void) forcepriv(SEC_MULTILEVELDIR);
	}
#endif /* B1 */
#endif /* SecureWare */
begin:
#ifndef DEBUG
	if (rfork=fork()) {
		if (rfork==-1) {
			sleep(30);
			goto begin; 
		}
		exit(0); 
	}
#else
	{
	    time_t now = time((long *) 0);

	    printf("cron starting at %s\n", fulltime(&now));
	    fflush(stdout);
	}
#endif

#ifdef SecureWare
	if (ISSECURE)
            (void) cron_secure_mask();
	else
	    umask(022);
#else
	umask(022);
#endif
	setpgrp();	/* detach cron from console */
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	initialize();
	quedefs(DEFAULT);	/* load default queue definitions */
	msg1((catgets(nlmsg_fd,NL_SETN,17, "*** cron started ***   pid = %d")),getpid());
	timeout();	/* set up alarm clock trap */
	t_old = time((long *) 0);
	last_time = t_old;
	while (TRUE) {			/* MAIN LOOP	*/
		t = time((long *) 0);
		if ((t_old > t) || (t-last_time > CUSHION)) 
		{
			/* the time was set backwards or forward */
			el_delete();
			for (rp = rt; rp < rt + MAXRUN; rp++)
			    rp->rusr = NULL;
			u = uhead;
			while (u!=NULL) {
				rm_ctevents(u);
				e = u->atevents;
				while (e!=NULL) {
					free(e->cmd);
					e2 = e->link;
					free(e);
					e = e2; 
				}
				u2 = u->nextusr;
                                free(u->name);
                                free(u->home);
                                free(u);
				u = u2; 
			}
			close(msgfd);
			initialize();
			t = time((long *) 0); 
		}
		t_old = t;
		if (next_event == NULL)
			if (el_empty()) ne_time = INFINITY;
			else {	
				next_event = (struct event *) el_first();
				ne_time = next_event->time - t; 
			}
		else ne_time = next_event->time - t;
#ifdef DEBUG
		if (next_event != NULL)
			printf("next_event = %s\tnext_time = %s\n",
                                next_event->cmd,
				(next_event->etype ? "at" : "cron"),
                                fulltime(&next_event->time));
		fflush(stdout);
#endif
		if (ne_time > (long) 0)
			idle();
		if (notexpired) {
			notexpired = 0;
			last_time = INFINITY;
			continue;
		}
		if (stat(QUEDEFS,&buf))
			msg0((catgets(nlmsg_fd,NL_SETN,18, "cannot stat QUEDEFS file")));
		else
			if (lastmtime != buf.st_mtime) {
				quedefs(LOAD);
				lastmtime = buf.st_mtime;
			}
		last_time = next_event->time;	/* save execution time */
		ex(next_event);
		switch (next_event->etype) {
		/* add cronevent back into the main event list */
		case CRONEVENT:
			if (delayed) {
				delayed = 0;
				break;
			}
			next_event->time = next_time(next_event);
			el_add( next_event,next_event->time,
			    (next_event->u)->ctid ); 
			break;
		/* remove at or batch job from system */
		default:
			eprev=NULL;
			/*
			 *   I do not understand why these next two lines are
			 *   in the code.  They were added at revision 38.1.
			 *   "e" is a stack variable and one the first "at"
			 *   job isn't yet set up.  I have initialized "e"
			 *   to ~NULL so this will not result in the break
			 *   being issued the first time around.
			 */
			if (e == NULL)
				break;
			e=(next_event->u)->atevents;
			while (e != NULL)
				if (e == next_event) {
					e->notify_user=NULL;
					if (eprev == NULL)
						(e->u)->atevents = e->link;
					else	eprev->link = e->link;
					free(e->cmd);
					free(e);
					break;	
				}
				else {	
					eprev = e;
					e = e->link; 
				}
			break;
		}
		next_event = NULL; 
	}
}



/**********/
initialize()
/**********/
{

	static int flag = 0;
	struct stat stat_buf;

#ifdef DEBUG
	printf("in initialize\n");
	fflush(stdout);
#endif
	nlmsg_fd=catopen("cron",0);
	init_time = time((long *) 0);
	el_init(8,init_time,(long)(60*60*24),10);
#ifdef SecureWare
	if (ISSECURE)
            (void) cron_set_communications(FIFO, 0);
	else {
	    if ( stat(FIFO, &stat_buf) == 0) { 
	       if (! (stat_buf.st_mode & S_IFIFO))
	          unlink(FIFO);
	    }
	    if (access(FIFO,04)==-1 && mknod(FIFO,S_IFIFO|0600,0)!=0)
		crabort((catgets(nlmsg_fd,NL_SETN,19, "cannot access fifo queue")));
	}
#else	
	/*make sure that if the file exists, it is a FIFO before other checks*/
	if ( stat(FIFO, &stat_buf) == 0) { 
	   if (! (stat_buf.st_mode & S_IFIFO)) /* delete if not a FIFO file */
	      unlink(FIFO);
	}

	if (access(FIFO,04)==-1 && mknod(FIFO,S_IFIFO|0600,0)!=0)
		crabort((catgets(nlmsg_fd,NL_SETN,19, "cannot access fifo queue")));
#endif
	if ((msgfd = open(FIFO, O_RDWR)) < 0) {
		perror("! open");
		crabort((catgets(nlmsg_fd,NL_SETN,20, "cannot create fifo queue")));
	}

	if (lockf(msgfd, F_TLOCK, 0) != 0)  /* if fifo locked, then a cron */
	    crabort((catgets(nlmsg_fd,NL_SETN,21, "cron is already running")));      /* is already running */

	/* read directories, create users list,
	   and add events to the main event list	*/
	uhead = NULL;
	read_dirs();
	next_event = NULL;
	if (flag)
		return;
#ifdef SecureWare
#ifdef B1
	if (ISB1)
            (void) forcepriv(SEC_SETOWNER);
#endif /* B1 */
#ifndef DEBUG
	if (ISSECURE)
            create_file_securely(ACCTFILE, AUTH_SILENT,
                             "keep track of cron events");
#endif
#endif /* SecureWare */
#ifndef DEBUG
	if (freopen(ACCTFILE,"a",stdout) == NULL)
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,22, "cannot open %s\n")),ACCTFILE);
	close(fileno(stderr));
	dup(1);
#endif
	/* this must be done to make popen work....i dont know why */
	(void *)freopen("/dev/null","r",stdin);
	flag = 1;
	return;
}


/******************/
read_dirs()
/******************/
{
	DIR 	*dir;
	int mod_ctab(), mod_atjob();

#ifdef DEBUG
	printf("Read Directories\n");
	fflush(stdout);
#endif
	/*
	 *   scan the /usr/spool/cron/crontabs directory looking for jobs.
	 */
	if (chdir(CRONDIR) == -1) crabort(BADCD);
	cwd = CRON;
#if defined(SecureWare) && defined(B1)
	if (ISB1)
            cron_existing_jobs(NOREADDIR, mod_ctab, 0);
	else {
	    if ((dir = opendir(".")) == (DIR *) NULL)
		crabort(NOREADDIR);
	    dscan(dir,mod_ctab);
	    closedir(dir);
	}
#else
	if ((dir = opendir(".")) == (DIR *) NULL)
		crabort(NOREADDIR);
	dscan(dir,mod_ctab);
	closedir(dir);
#endif
	/*
	 *  scan the /usr/spool/cron/at_jobs directory looking for jobs.
	 */
	if (chdir(ATDIR) == -1) {
		msg0((catgets(nlmsg_fd,NL_SETN,23, "cannot chdir to at directory")));
		return;
	}
	cwd = AT;
#if defined(SecureWare) && defined(B1)
	if (ISB1)
            cron_existing_jobs("cannot read at directory", mod_atjob, 1);
	else {
	    if ((dir = opendir(".")) == (DIR *) NULL) {
		msg0((catgets(nlmsg_fd,NL_SETN,24, "cannot read at directory")));
		return; 
	    }
	    dscan(dir,mod_atjob);
	    closedir(dir);
	}
#else
	if ((dir = opendir(".")) == (DIR *) NULL) {
		msg0((catgets(nlmsg_fd,NL_SETN,24, "cannot read at directory")));
		return; 
	}
	dscan(dir,mod_atjob);
	closedir(dir);
#endif
        return;
}

/****************/
dscan(df,fp)
/****************/
DIR  	*df;
int	(*fp)();
{

	register	i;
	register	struct	dirent	*dp;
	char		name[MAXNAMLEN+1];


	for (dp = readdir(df); dp != NULL; dp=readdir(df)) {
		if (dp->d_ino == 0)
			continue;
	        /*
	         *   Neither "." nor ".." are jobs.  So ignore them.
	         */
		if (strcmp(dp->d_name,".") == 0)
			continue;
		if (strcmp(dp->d_name,"..") == 0)
			continue;

		for (i=0; i<MAXNAMLEN; ++i)
			name[i] = dp->d_name[i];
		name[i] = '\0';
		(*fp) (name,'\0');
	}
	return;
}

/****************/
mod_ctab(name,action)
/****************/
char	*name, action;
{

	struct	passwd	*pw,*getpwnam();
	struct	stat	buf;
	struct	usr	*u,*find_usr();
	char	namebuf[MAXPATHLEN];
	char	*pname;

#ifdef DEBUG
	printf("mod_ctab: action =\'%c\' (%#x)\n",action,action);
	fflush(stdout);
#endif
	if ((pw=getpwnam(name)) == NULL)
		return;
#if defined(SecureWare) && defined(B1)
	if (ISB1)
            pname = cron_jobname(cwd, CRON, CRONDIR, name, namebuf);
	else {
	    if (cwd != CRON) {
		(void *)strcat(strcat(strcpy(namebuf,CRONDIR),"/"),name);
		pname = namebuf;
	    } else
		pname = name;
	}
#else
	if (cwd != CRON) {
		(void *)strcat(strcat(strcpy(namebuf,CRONDIR),"/"),name);
		pname = namebuf;
	} else
		pname = name;
#endif
	/*
	 * a warning message is given by the crontab command so there is
	 * no need to give one here......use this code if you only want
	 * users with a login shell of /bin/sh to use cron
	*/
#ifdef RESTRICT_SHELL
	if (strcmp(pw->pw_shell,"") !=0 &&
	    strcmp(pw->pw_shell,SHELL) !=0) {
			mail(name,BADSHELL,2);
			unlink(pname);
			return;
	}
#endif /* RESTRICT_SHELL */
	if (stat(pname,&buf)) {
		mail(name,BADSTAT,2);
		unlink(pname);
		return;
	}
	if ((u=find_usr(name)) == NULL) {
#ifdef DEBUG
		printf("new user (%s) with a crontab\n",name);
		fflush(stdout);
#endif
		u = (struct usr *) xmalloc(sizeof(struct usr));
		u->name = xmalloc(strlen(name)+1);
		(void *)strcpy(u->name,name);
		u->home = xmalloc(strlen(pw->pw_dir)+1);
		(void *)strcpy(u->home,pw->pw_dir);
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                    u->seclevel_ir = cron_getlevel();
#endif
		u->ctexists = TRUE;
		u->ctid = ecid++;
		u->ctevents = NULL;
		u->atevents = NULL;
#ifdef ATLIMIT
		u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
		u->cruncnt = 0;
#endif
		u->nextusr = uhead;
		uhead = u;
		readcron(u);
	} else {
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                    u->seclevel_ir = cron_getlevel();
#endif
		if (strcmp(u->home,pw->pw_dir) != 0) {
			free(u->home);
			u->home = xmalloc(strlen(pw->pw_dir)+1);
			(void *)strcpy(u->home,pw->pw_dir);
		}
		u->ctexists = TRUE;
		if (u->ctid == 0) {
#ifdef DEBUG
			printf("%s now has a crontab\n",u->name);
			fflush(stdout);
#endif
			/* user didnt have a crontab last time */
			u->ctid = ecid++;
			u->ctevents = NULL;
			readcron(u);
			return;
		}
#ifdef DEBUG
		printf("%s has revised his crontab\n",u->name);
		fflush(stdout);
#endif
		rm_ctevents(u);
		el_remove(u->ctid,0);
		readcron(u);
	}
	return;
}


/****************/
mod_atjob(name,action)
/****************/
char	*name, action;
{

	char	*ptr;
	time_t	tim;
	struct	passwd	*pw,*getpwnam(),*getpwuid();
	struct	stat	buf;
	struct	usr	*u,*find_usr();
	struct	event	*e;
	char	namebuf[MAXPATHLEN];
	char	*pname;

#ifdef DEBUG
	printf("mod_atjob: action =\'%c\' (%#x)\n",action,action);
	fflush(stdout);
#endif
	/*
	 *    Extract the start time, the queue and at the same
	 *    time verify the file name is a valid job.
	 *    If it is not a valid job, we ignore it.
	 *    The format of the file name is "<starttime>.<queue>".
	 */
        ptr = name;
        if (((tim=num(&ptr)) == 0) || (*ptr != '.'))
           return;
        ptr++;
        if (!isalpha(*ptr))
           return;
        jobtype = *ptr - 'a';

#if defined(SecureWare) && defined(B1)
	if (ISB1)
            pname = cron_jobname(cwd, AT, ATDIR, name, namebuf);
	else {
	    if (cwd != AT) {
		(void *)strcat(strcat(strcpy(namebuf,ATDIR),"/"),name);
		pname = namebuf;
	    } else
		pname = name;
	}
#else	
	if (cwd != AT) {
		(void *)strcat(strcat(strcpy(namebuf,ATDIR),"/"),name);
		pname = namebuf;
	} else
		pname = name;
#endif
	if (stat(pname,&buf) || jobtype >= NQUEUE-1) {
		unlink(pname);
		return;
	}
	if (!(buf.st_mode & ISUID)) {
		unlink(pname);
		return;
	}
	if ((pw=getpwuid(buf.st_uid)) == NULL)
		return;
	/* a warning message is given by the at command so there is no
	 * need to give one here......use this code if you only want
	 * users with a login shell of /bin/sh to use cron
	 */
#ifdef RESTRICT_SHELL
	if (strcmp(pw->pw_shell,"") !=0 &&
	    strcmp(pw->pw_shell,SHELL)!=0) {
			mail(pw->pw_name,BADSHELL,2);
			unlink(pname);
			return;
	}
#endif /* RESTRICT_SHELL */
	if ((u=find_usr(pw->pw_name)) == NULL) {
#ifdef DEBUG
		printf("new user (%s) with an at job = %s\n",pw->pw_name,name);
		fflush(stdout);
#endif
		u = (struct usr *) xmalloc(sizeof(struct usr));
		u->name = xmalloc(strlen(pw->pw_name)+1);
		(void *)strcpy(u->name,pw->pw_name);
		u->home = xmalloc(strlen(pw->pw_dir)+1);
		(void *)strcpy(u->home,pw->pw_dir);
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                    u->seclevel_ir = cron_getlevel();
#endif
		u->ctexists = FALSE;
		u->ctid = 0;
		u->ctevents = NULL;
		u->atevents = NULL;
#ifdef ATLIMIT
		u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
		u->cruncnt = 0;
#endif
		u->nextusr = uhead;
		uhead = u;
		add_atevent(u,name,tim,action);
	} else {
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                    u->seclevel_ir = cron_getlevel();
#endif
		if (strcmp(u->home,pw->pw_dir) != 0) {
			free(u->home);
			u->home = xmalloc(strlen(pw->pw_dir)+1);
			(void *)strcpy(u->home,pw->pw_dir);
		}
		e = u->atevents;
		while (e != NULL)
			if (strcmp(e->cmd,name) == 0) {
				e->of.at.exists = TRUE;
				break;
			} else
				e = e->link;
		if (e == NULL) {
#ifdef DEBUG
			printf("%s has a new at job = %s\n",u->name,name);
			fflush(stdout);
#endif
		        add_atevent(u,name,tim,action);
		}
	}
	return;
}



/****************/
add_atevent(u,job,tim,action)
/****************/
struct usr *u;
char *job;
time_t tim;
char action;
{
	struct event *e;
	short queue;
	char	*ptr;

#ifdef DEBUG
        time_t dbgtime;

        dbgtime = time((long *) 0);
	printf("add_atevent: action =\'%c\' (%#x) %s\n",action,action,
               fulltime(&dbgtime));
	fflush(stdout);
#endif

	/*
	 *    Extract the queue and at the same time verify the file 
	 *    name is a valid job.  If it is not a valid job, we ignore it.
	 *    In this routine we ignore the "<start time>" part of the file
	 *    name since the time is passed in through the parameter "tim".
	 */
        ptr = job;
        if (((num(&ptr)) == 0) || (*ptr != '.'))
           return;
        ptr++;
        if (!isalpha(*ptr))
           return;
        queue = *ptr - 'a';

	e=(struct event *) xmalloc(sizeof(struct event));
	e->etype = queue;
	e->cmd = xmalloc(strlen(job)+1);
	(void *)strcpy(e->cmd,job);
	e->u = u;
#ifdef DEBUG
	printf("add_atevent: user=%s, job=\'%s\', queue=\'%c\', time=\'%s\'\n",
		u->name, e->cmd, (char)(queue+'a'), fulltime(&tim));
	fflush(stdout);
#endif
	e->link = u->atevents;
	u->atevents = e;
	e->notify_user=action; 
	e->of.at.exists = TRUE;
	e->of.at.eventid = ecid++;
	if (tim < init_time)		/* old job */
		e->time = init_time;
	else
		e->time = tim;
	el_add(e, e->time, e->of.at.eventid); 
	return;
}


char line[CTLINESIZE];		/* holds a line from a crontab file	*/
int cursor;			/* cursor for the above line	*/

/******************/
readcron(u)
/******************/
struct usr *u;
{
	/* readcron reads in a crontab file for a user (u).
	   The list of events for user u is built, and 
	   u->events is made to point to this list.
	   Each event is also entered into the main event list. */

	FILE *fopen(),*cf;		/* cf will be a user's crontab file */
	time_t next_time();
	struct event *e;
	int start,i;
	char *next_field();
	char namebuf[MAXPATHLEN];
	char *pname, *src, *dest;

	/* read the crontab file */
#if defined(SecureWare) && defined(B1)
	if (ISB1)
            pname = cron_jobname(cwd, CRON, CRONDIR, u->name, namebuf);
	else {
	    if (cwd != CRON) {
		(void *)strcat(strcat(strcpy(namebuf,CRONDIR),"/"),u->name);
		pname = namebuf;
	    } else
		pname = u->name;
	}
#else	
	if (cwd != CRON) {
		(void *)strcat(strcat(strcpy(namebuf,CRONDIR),"/"),u->name);
		pname = namebuf;
	} else
		pname = u->name;
#endif
	if ((cf=fopen(pname,"r")) == NULL) {
		mail(u->name,NOREAD,2);
		return; 
	}

#ifdef AUDIT
    {
	auto	FILE *		rfp = (FILE *)0;
	auto	char *		aidfname = (char *)0;


	/*
	 * construct audit ID file name
	 */
	aidfname = xmalloc(strlen(CRONAIDS) + strlen(u->name) + 2);
	(void)sprintf(aidfname, "%s/%s", CRONAIDS, u->name);

	/*
	 * get audit ID
	 */
	u->audid = (aid_t)-1;
	if ((rfp = fopen(aidfname, "r")) != (FILE *)0) {
		(void)fscanf(rfp, "%ld", &(u->audid));
		(void)fclose(rfp);
	}

	(void)free(aidfname);
    }
#endif /* AUDIT */

	while (fgets(line,CTLINESIZE,cf) != NULL) {
		/* process a line of a crontab file */
		cursor = 0;
		while (line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if ((line[cursor] == '#') || (line[cursor] == '\0')
			|| (line[cursor] == '\n'))
			continue;
		e = (struct event *) xmalloc(sizeof(struct event));
		e->etype = CRONEVENT;
		if ((e->of.ct.minute=next_field(0,59,u)) == NULL) goto badline;
		if ((e->of.ct.hour=next_field(0,23,u)) == NULL) goto badline;
		if ((e->of.ct.daymon=next_field(1,31,u)) == NULL) goto badline;
		if ((e->of.ct.month=next_field(1,12,u)) == NULL) goto badline;
		if ((e->of.ct.dayweek=next_field(0,6,u)) == NULL) goto badline;
		if (line[++cursor] == '\0') {
			mail(u->name,EOLN,1);
			goto badline; 
		}
		/* get the command to execute	*/
		start = cursor;
again:
		while ((line[cursor]!='%')&&(line[cursor]!='\n')
		    &&(line[cursor]!='\0') && (line[cursor]!='\\')) cursor++;
		if (line[cursor] == '\\') {
			cursor += 2;
			goto again;
		}
		e->cmd = xmalloc(cursor-start+1);

		/* Copy the command, converting \% to % */
		dest=e->cmd;
		for (src=line+start; src<line+cursor; src++) {
			if (*src=='\\' && src[1]=='%')
				src++;		/* skip the backslash */
			*dest++ = *src;
		}
		*dest = '\0';

		/* see if there is any standard input	*/
		if (line[cursor] == '%') {
			e->of.ct.input = xmalloc(strlen(line)-cursor+1);
			(void *)strcpy(e->of.ct.input,line+cursor+1);
			for (i=0; i<strlen(e->of.ct.input); i++)
				if (e->of.ct.input[i] == '%') e->of.ct.input[i] = '\n'; 
		}
		else e->of.ct.input = NULL;
		/* have the event point to its owner	*/
		e->u = u;
		/* insert this event at the front of this user's event list   */
		e->link = u->ctevents;
		u->ctevents = e;
		/* set the time for the first occurance of this event	*/
		e->time = next_time(e);
		/* Do not notify the users when this is executed	*/
		e->notify_user = '\0';
		/* finally, add this event to the main event list	*/
		el_add(e,e->time,u->ctid);
#ifdef SecureWare
		if (ISSECURE) {
                	sprintf(auditbuf,
                            "add_cronevent:user=%s, job=\'%s\', time=\'%s\'",
                            u->name, e->cmd, fulltime(&e->time));
                	audit_subsystem(auditbuf, "added 'cron' event", ET_SUBSYSTEM);
		}
#endif
#ifdef DEBUG
		printf("inserting cron event \'%s\' at %s\n",
			e->cmd, fulltime(&e->time));
		fflush(stdout);
#endif
		continue;

badline: 
		free(e); 
	}

	fclose(cf);
	return;
}


/*
 *  This code used "popen" in the past.  However, this does not work.
 *  For correct execution popen MUST be paired with a pclose.  Unfortunately
 *  pclose waits for the "open" process to complete.  Cron can not afford to 
 *  "wait" for mail to complete (this could take a long time on a busy system). 
 *  The following is an attempt to describe the failure which occurs
 *  when using popen and fclose.
 *
 *  close all pipes from other popen's 
 *       for (poptr2 = popen_table; poptr2 != NULL; poptr2=poptr2->next)
 *           if (poptr2->filep != (FILE *)NULL)
 *               (void)close(fileno(poptr2->filep));
 ***
 ***when a pclose is NOT issued this table is left with the results of 
 ***all previous popens.  Because of the way the "fd's" are being closed
 ***this popen will end up with the same "fd" as the previous popen.
 ***The close ends up closing the "fd".  This results in the fcntl below
 ***failing.  Therefore the shell ends up with its stdin and stdout
 ***attached together.
 *
 *       stdio = tst(0, 1);
 *       (void)close(myside);
 *       if (yourside != stdio)
 *       {
 * only if stdio not previously closed... 
 *           (void)close(stdio);
 *           (void)fcntl(yourside, F_DUPFD, stdio);
 *           (void)close(yourside);
 *       }
 *       (void)execl("/bin/sh", "sh", "-c", cmd, 0);
 *
 */
/*****************/
mail(usrname,msg,format)
/*****************/
char *usrname,*msg;
int format;
{
	/* mail mails a user a message.	*/

	FILE *pipe,*cron_popen();
	char *temp,*i,*strrchr();
	char *printbuf, *xmalloc();
#ifdef SecureWare
        int child_pid;
        struct runinfo *rp;
#endif
#ifdef TESTING
	return;
#endif
#ifdef SecureWare
	if (ISSECURE) {
            child_pid = cron_mail_setup(0);

            if (child_pid < 0)
                return;

            if (child_pid > 0)  {
                for (rp = rt; rp < rt + MAXRUN; rp++)  {
                        if (rp->pid == 0)
                                break;
                }
                if (rp >= rt + MAXRUN) {
                    /*
                        msg2("reached process limit of %d, mail to %s not sent",
                            MAXRUN, usrname);
                    */
                        return;
                }
                rp->pid = child_pid;
                rp->que = ZOMB;
                rp->rusr = (struct usr *) 0;
                rp->outfile = xmalloc(2);
                rp->outfile[0] = (char) ~0;
                rp->outfile[1] = '\0';

                /* decremented in idle() */
                running++;

                return;
            }
	}
#endif
	temp = xmalloc(strlen(MAIL)+strlen(usrname)+2);
	strcat(strcat(strcpy(temp,MAIL)," "),usrname);
#ifdef DEBUG
	printf("mail sched string: %s\n",temp);
	printf("format=%d\n",format);
#endif
	/*
	 * pipe = popen(temp,"w");  see discussion above for change
	 */
	pipe = cron_popen(temp,"w");
	if (pipe!=NULL) {
		printbuf=xmalloc(200);
		if (format == 1) {
			sprintf(printbuf,(catgets(nlmsg_fd,NL_SETN,25, "Your crontab file has an error in it.\n")));
#ifdef DEBUG
			printf("msg: %s\n",printbuf);
#endif
			fprintf(pipe,printbuf);
			i = strrchr(line,'\n');
			if (i != NULL) *i = ' ';
			sprintf(printbuf, "\t%s\n\t%s\n",line,msg);
#ifdef DEBUG
			printf("msg: %s\n",printbuf);
#endif
			fprintf(pipe,printbuf);
			sprintf(printbuf, (catgets(nlmsg_fd,NL_SETN,26, "This entry has been ignored.\n"))); 
#ifdef DEBUG
			printf("msg: %s\n",printbuf);
#endif
			fprintf(pipe,printbuf);
		}
		else 
		{
		        sprintf(printbuf, "\nCron: %s\n",msg);
#ifdef DEBUG
			printf("msg: %s\n",printbuf);
#endif
			fprintf(pipe,printbuf);
		}
		free(printbuf);
		fflush(pipe);
		fclose(pipe); 

		/* decremented in idle() */
		running++;
	}
	free(temp);
#ifdef SecureWare
	if (ISSECURE)
            cron_mail_finish();
#endif
        return;
}


/*****************/
char *next_field(lower,upper,u)
/*****************/
int lower,upper;
struct usr *u;
{
	/* next_field returns a pointer to a string which holds 
	   the next field of a line of a crontab file.
	   if (numbers in this field are out of range (lower..upper),
	       or there is a syntax error) then
			NULL is returned, and a mail message is sent to
			the user telling him which line the error was in.     */

	char *s;
	int num,num2,start;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	start = cursor;
	if (line[cursor] == '\0') {
		mail(u->name,EOLN,1);
		return(NULL); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			mail(u->name,UNEXPECT,1);
			return(NULL); 
		}
		s = xmalloc(2);
		(void *)strcpy(s,"*");
		return(s); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			mail(u->name,UNEXPECT,1);
			return(NULL); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			mail(u->name,OUTOFBOUND,1);
			return(NULL); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				mail(u->name,UNEXPECT,1);
				return(NULL); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				mail(u->name,OUTOFBOUND,1);
				return(NULL); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			mail(u->name,EOLN,1);
			return(NULL); 
		}
		if (line[cursor++]!=',') {
			mail(u->name,UNEXPECT,1);
			return(NULL); 
		}
	}
	s = xmalloc(cursor-start+1);
	strncpy(s,line+start,cursor-start);
	s[cursor-start] = '\0';
	return(s);
}


/*****************/
time_t next_time(e)
/*****************/
struct event *e;
{
	/*
	 * Returns the integer time for the next occurance of event e.
	 * the following fields have ranges as indicated:
	 *
	 *  PRGM  | min     hour  day of month    mon     day of week
	 *  ------|-----------------------------------------------------
	 *  cron  | 0-59    0-23      1-31        1-12    0-6 (0=sunday)
	 *  time  | 0-59    0-23      1-31        0-11    0-6 (0=sunday)
	 *     NOTE: this routine is hard to understand.
	 *     
	 * No kidding!
	 *  
	 * I've added code at the end of this routine to adjust for
	 * Daylight savings time.
	 *
	 * What we do is determine if the time for the next event is in
	 * a different DST mode than the current time.  If it is, we
	 * must adjust the time by + or - 1 hour.  We only do this if
	 * the hour specification for this event is NOT "*".
	 */

	struct tm *tm,*localtime();
	int tm_mon,tm_mday,tm_wday,wday,m,min,h,hr,carry,day,days,
	d1,day1,carry1,d2,day2,carry2,daysahead,mon,yr,db,wd,today;
	time_t t;

#ifdef DEBUG
	printf("\nnext_time: BEGIN\n");
	fflush(stdout);
#endif

	t = time((long *) 0);
	tm = localtime(&t);

	tm_mon = next_ge(tm->tm_mon+1,e->of.ct.month) - 1; /* 0-11 */
	tm_mday = next_ge(tm->tm_mday,e->of.ct.daymon);	   /* 1-31 */
	tm_wday = next_ge(tm->tm_wday,e->of.ct.dayweek);   /* 0-6  */
	today = TRUE;
	if ( (strcmp(e->of.ct.daymon,"*")==0 && tm->tm_wday!=tm_wday)
	    || (strcmp(e->of.ct.dayweek,"*")==0 && tm->tm_mday!=tm_mday)
	    || (tm->tm_mday!=tm_mday && tm->tm_wday!=tm_wday)
	    || (tm->tm_mon!=tm_mon))
	    today = FALSE;

	m = tm->tm_min+1;
	min = next_ge(m%60,e->of.ct.minute);
	carry = (min < m) ? 1:0;
	h = tm->tm_hour+carry;
	hr = next_ge(h%24,e->of.ct.hour);
	carry = (hr < h) ? 1:0;
	if ((!carry) && today) {
		/*
		 * This event must occur today, but if it isn't
		 * to be run in this hour, min(utes) must be reset
		 * (this is only relevant for crontab initialization
		 * and shouldn't affect regular cron job scheduling).
		 */
		if (hr != tm->tm_hour)
		    min = next_ge(0,e->of.ct.minute);

		/*
		 * This just adds the number of seconds to 't' to get
		 * to the desired hr:min (removing any extraneous
		 * seconds so that the job starts on the minute).
		 */
		if (tm->tm_min>min)
			t +=(time_t)(hr-tm->tm_hour-1)*HOUR + 
			    (time_t)(60-tm->tm_min+min)*MINUTE;
		else t += (time_t)(hr-tm->tm_hour)*HOUR +
			(time_t)(min-tm->tm_min)*MINUTE;
		t -= tm->tm_sec;
		day = tm->tm_mday; /* day of month we expect */
		goto DST_fix;
	}

	min = next_ge(0,e->of.ct.minute);
	hr = next_ge(0,e->of.ct.hour);

	/*
	 * calculate the date of the next occurance of this event,
	 * which will be on a different day than the current day.
	 */

	/* check monthly day specification	*/
	d1 = tm->tm_mday+1;
	day1 = next_ge((d1-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1,e->of.ct.daymon);
	carry1 = (day1 < d1) ? 1:0;

	/* check weekly day specification	*/
	d2 = tm->tm_wday+1;
	wday = next_ge(d2%7,e->of.ct.dayweek);
	if (wday < d2)
	    daysahead = 7 - d2 + wday;
	else
	    daysahead = wday - d2;
	day2 = (d1+daysahead-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1;
	carry2 = (day2 < d1) ? 1 : 0;

	/*
	 * based on their respective specifications,
	 * day1, and day2 give the day of the month
	 * for the next occurance of this event.
	 */
	if (strcmp(e->of.ct.daymon,"*") == 0 &&
	    strcmp(e->of.ct.dayweek,"*") != 0) {
		day1 = day2;
		carry1 = carry2; 
	}
	if (strcmp(e->of.ct.daymon,"*") != 0 &&
	    strcmp(e->of.ct.dayweek,"*") == 0) {
		day2 = day1;
		carry2 = carry1; 
	}

	yr = tm->tm_year;
	if ((carry1 && carry2) || (tm->tm_mon != tm_mon)) {
		/* event does not occur in this month	*/
		m = tm->tm_mon+1;
		mon = next_ge(m%12+1,e->of.ct.month)-1;	/* 0..11 */
		carry = (mon < m) ? 1:0;
		yr += carry;
		/* recompute day1 and day2	*/
		day1 = next_ge(1,e->of.ct.daymon);
		db = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,1,yr) + 1;
		wd = (tm->tm_wday+db)%7;
		/* wd is the day of the week of the first of month mon*/
		wday = next_ge(wd,e->of.ct.dayweek);
		if (wday < wd)
		    day2 = 1 + 7 - wd + wday;
		else
		    day2 = 1 + wday - wd;
		if (strcmp(e->of.ct.daymon,"*") != 0 &&
		    strcmp(e->of.ct.dayweek,"*") == 0)
			day2 = day1;
		if (strcmp(e->of.ct.daymon,"*") == 0 &&
		    strcmp(e->of.ct.dayweek,"*") != 0)
			day1 = day2;
		day = (day1 < day2) ? day1:day2; 
	}
	else { /* event occurs in this month */
		mon = tm->tm_mon;
		if (!carry1 && !carry2)
		    day = (day1 < day2) ? day1 : day2;
		else if (!carry1)
		    day = day1;
		else
		    day = day2;
	}

	/*
	 * now that we have the min,hr,day,mon,yr of the next
	 * event, figure out what time that turns out to be.
	 */
	days = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,day,yr);
	t += (time_t)(23-tm->tm_hour)*HOUR +
	     (time_t)(60-tm->tm_min)*MINUTE +
	     (time_t)hr*HOUR + (time_t)min*MINUTE + (time_t)days*DAY;
	t -= (time_t)tm->tm_sec;

DST_fix:
	/*
	 * Now, see if we need to adjust because of Daylight Savings
	 * Time.  We only do this if they didn't have "*" in the hour
	 * field.
	 */
	if (strcmp(e->of.ct.hour, "*") != 0)
	{
	    tm = localtime(&t);
	    /*
	     * If the hour and minute of our new time is not what we
	     * expected, we must have gone past a time where either
	     * REG->DST or DST->REG.  Try to adjust the time to what we
	     * expect.
	     *
	     * We can't simply use an hour, since the adjustment for
	     * DST can be in hours and minutes (although this would
	     * be *very* strange).
	     */
	    if (day != tm->tm_mday ||
		hr  != tm->tm_hour || min != tm->tm_min)
	    {
		time_t unadjusted = t; /* in case we just can't do it */

#ifdef DEBUG
	        printf("next_time: Wanted an event at %02d %02d:%02d\n",
		    day, hr, min);
		printf("next_time:        instead got %02d %02d:%02d\n",
		    tm->tm_mday, tm->tm_hour, tm->tm_min);
		fflush(stdout);
#endif
		t += ((hr*60 + min)-(tm->tm_hour*60 + tm->tm_min)) * 60;

		/*
		 * Since the DST shift is always less than a day...
		 *
		 * The day we got is yesterday if it is less than the
		 * day we expected OR it is greater than the day we
		 * expected by more than 1 day.
		 *
		 * The day we got is tomorrow if it is greater than the
		 * day we expected or it is less than the one day
		 * before the day we expected.
		 */
		if (tm->tm_mday < day || tm->tm_mday-1 > day)
		    t += DAY;
		else
		    if (day < tm->tm_mday || day-1 > tm->tm_mday)
			t -= DAY;

		/*
		 * Now that we have adjusted the time, make sure that
		 * it is what we wanted.  If not, we don't do any
		 * adjustment.
		 *
		 * We will only get this condition when the next time
		 * falls in the 1 hour window around the change in
		 * daylight savings time.
		 */
                tm = localtime(&t);
#ifdef DEBUG
		printf("next_time:  adjusting yielded %02d %02d:%02d\n",
		    tm->tm_mday, tm->tm_hour, tm->tm_min);
		fflush(stdout);
#endif
		if (hr != tm->tm_hour)
		    t = unadjusted;
	    }
	}
#ifdef DEBUG
	printf("next_time: END returning %s\n", fulltime(&t));
	fflush(stdout);
#endif
	return t;
}

#define	DUMMY	100
/****************/
next_ge(current,list)
/****************/
int current;
char *list;
{
	/* list is a character field as in a crontab file;
	   	for example: "40,20,50-10"
	   next_ge returns the next number in the list that is
	   greater than or equal to current.
	   if no numbers of list are >= current, the smallest
	   element of list is returned.
	   NOTE: current must be in the appropriate range.	*/

	char *ptr;
	int n,n2,min,min_gt;

	if (strcmp(list,"*") == 0) return(current);
	ptr = list;
	min = DUMMY; 
	min_gt = DUMMY;
	while (TRUE) {
		if ((n=(int)num(&ptr))==current) return(current);
		if (n<min) min=n;
		if ((n>current)&&(n<min_gt)) min_gt=n;
		if (*ptr=='-') {
			ptr++;
			if ((n2=(int)num(&ptr))>n) {
				if ((current>n)&&(current<=n2))
					return(current); 
			}
			else {	/* range that wraps around */
				if (current>n) return(current);
				if (current<=n2) return(current); 
			}
		}
		if (*ptr=='\0') break;
		ptr += 1; 
	}
	if (min_gt!=DUMMY) return(min_gt);
	else return(min);
}

/****************/
del_atjob(name,usrname)
/****************/
char	*name;
char	*usrname;
{

	struct	event	*e, *eprev;
	struct	usr	*u;

	if ((u = find_usr(usrname)) == NULL)
		return;
	e = u->atevents;
	eprev = NULL;
#ifdef SecureWare
	if (ISSECURE) {
                sprintf(auditbuf, "remove at event \'%s\' from %s\'s events",
                        name, usrname);
                audit_subsystem(auditbuf, "'at' event deleted", ET_SUBSYSTEM);
	}
#endif
	while (e != NULL)
		if (strcmp(name,e->cmd) == 0) {
			if (next_event == e)
				next_event = NULL;
			if (eprev == NULL)
				u->atevents = e->link;
			else
				eprev->link = e->link;
			el_remove(e->of.at.eventid, 1);
			free(e->cmd);
			free(e);
			break;
		} else {
			eprev = e;
			e = e->link;
		}
	if (!u->ctexists && u->atevents == NULL) {
#ifdef DEBUG
		printf("%s removed from usr list\n",usrname);
		fflush(stdout);
#endif
		if (ulast == NULL)
			uhead = u->nextusr;
		else
			ulast->nextusr = u->nextusr;
		free(u->name);
		free(u->home);
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                    cron_release_ir(u->seclevel_ir);
#endif	
		free(u);
	}
	return;
}

/*****************/
del_ctab(name)
/*****************/
char	*name;
{

	struct	usr	*u;
	struct  runinfo *runp;

	if ((u = find_usr(name)) == NULL)
		return;
	rm_ctevents(u);
#ifdef SecureWare
	if (ISSECURE) {
        	sprintf(auditbuf, "delete cron events for user %s", u->name);
        	audit_subsystem(auditbuf, "cron events deleted", ET_SUBSYSTEM);
	}
#endif
	el_remove(u->ctid, 0);
	u->ctid = 0;
	u->ctexists = 0;

	/* Determine if usr still has a process running.  If he does, do not free u until after the process exits. */
        for (runp = rt;runp < rt+MAXRUN; runp++)
            if (runp->pid != 0 && runp->rusr == u )
               return;

	if (u->atevents == NULL) {
#ifdef DEBUG
		printf("%s removed from usr list\n",name);
		fflush(stdout);
#endif
		if (ulast == NULL)
			uhead = u->nextusr;
		else
			ulast->nextusr = u->nextusr;
		free(u->name);
		free(u->home);
#if defined(SecureWare) && defined(B1)
		if (ISB1)
                    cron_release_ir(u->seclevel_ir);
#endif	
		free(u);
	}
	return;
}


/*****************/
rm_ctevents(u)
/*****************/
struct usr *u;
{
	struct event *e2,*e3;

	/* see if the next event (to be run by cron)
	   is a cronevent owned by this user.		*/
	if ( (next_event!=NULL) && 
	    (next_event->etype==CRONEVENT) &&
	    (next_event->u==u) )
		next_event = NULL;
	e2 = u->ctevents;
	while (e2 != NULL) {
		free(e2->cmd);
		free(e2->of.ct.minute);
		free(e2->of.ct.hour);
		free(e2->of.ct.daymon);
		free(e2->of.ct.month);
		free(e2->of.ct.dayweek);
		if (e2->of.ct.input != NULL) 
		{
		    free(e2->of.ct.input);
		}
		e3 = e2->link;
		free(e2);
		e2 = e3; 
	}
	u->ctevents = NULL;
	return;
}


/****************/
struct usr *find_usr(uname)
/****************/
char *uname;
{
	struct usr *u;

	u = uhead;
	ulast = NULL;
	while (u != NULL) {
#if defined(SecureWare) && defined(B1)
            if ((ISB1) ? (cron_id_match(u->name, uname, u->seclevel_ir)) :
	                 (strcmp(u->name,uname) == 0))
                        return(u);
#else		
	    if (strcmp(u->name,uname) == 0) return(u);
#endif
		ulast = u;
		u = u->nextusr; 
	}
	return(NULL);
}

#ifdef DEBUG
childsighandler(sig)
  int sig;
{
  exit (sig);
}
#endif


/*****************/
ex(e)
/*****************/
struct event *e;
{

	register i;
	short sp_flag;
	int fd,rfork;
	char *at_cmdfile, *cron_infile;
	char *mktemp();
	char *tempnam();
	struct stat buf;
	struct queue *qp;
	struct runinfo *rp;
	char	fileext[6];
	char	logfile [50];

#ifdef DEBUG
	FILE *dbgoutfp;
	int dbgoutfd;
#endif


	if (e == NULL)
		return;
	qp = &qt[e->etype];	/* set pointer to queue defs */
	if (qp->nrun >= qp->njob) {
		msg1((catgets(nlmsg_fd,NL_SETN,27, "%c queue max run limit reached")),e->etype+'a');
		resched(qp->nwait);
		return;
	}
	for (rp=rt; rp < rt+MAXRUN; rp++) {
		if (rp->pid == 0)
			break;
	}
	if (rp >= rt+MAXRUN) {
		msg1((catgets(nlmsg_fd,NL_SETN,28, "MAXRUN (%d) procs reached")),MAXRUN);
		resched(qp->nwait);
		return;
	}
#ifdef ATLIMIT
	if ((e->u)->uid != 0 && (e->u)->aruncnt >= ATLIMIT) {
		msg2((catgets(nlmsg_fd,NL_SETN,29, "ATLIMIT (%d) reached for uid %d")),ATLIMIT,(e->u)->uid);
		resched(qp->nwait);
		return;
	}
#endif
#ifdef CRONLIMIT
	if ((e->u)->uid != 0 && (e->u)->cruncnt >= CRONLIMIT) {
		msg2((catgets(nlmsg_fd,NL_SETN,30, "CRONLIMIT (%d) reached for uid %d")),CRONLIMIT,(e->u)->uid);
		resched(qp->nwait);
		return;
	}
#endif

#if defined(SecureWare) && defined(B1)
	if (ISB1)
            rp->outfile = cron_tempnam(TMPDIR,PFX, (e->u)->seclevel_ir);
	else
	    rp->outfile = tempnam(TMPDIR,PFX);
#else	
	rp->outfile = tempnam(TMPDIR,PFX);
#endif
	if ((rfork = fork()) == -1) {
		msg0((catgets(nlmsg_fd,NL_SETN,31, "cannot fork")));
		resched(wait_time);
		sleep(30);
		return;
	}
	if (rfork) {	/* parent process */
		++qp->nrun;
		++running;
		++lognum;
		if (lognum > 999)
			lognum = 0;
		rp->pid = rfork;
		rp->que = e->etype;
		rp->notify_user = e->notify_user;
#ifdef ATLIMIT
		if (e->etype != CRONEVENT)
			(e->u)->aruncnt++;
#endif
#if ATLIMIT && CRONLIMIT
		else
			(e->u)->cruncnt++;
#else
#ifdef CRONLIMIT
		if (e->etype == CRONEVENT)
			(e->u)->cruncnt++;
#endif
#endif
		rp->rusr = (e->u);
		rp->rcmd = (char *) strdup(e->cmd);
		logit((char)BCHAR,rp,0);
		return;
	}
        /*
	 *  Child process
	 */
#ifdef DEBUG
	/* Set all signals to cause exits */
        for (i=1; i<=32; i++) signal (i, childsighandler);

	/*  Reopen stdout for debug messages */
	sprintf (fileext, "%d", lognum);
	strcat (strcpy (logfile, CHILDLOG), fileext);
	dbgoutfp = fopen (logfile, "a");
	dbgoutfd = fileno (dbgoutfp);

	for (i=getnumfds()-1;i>=0;i--) 
		if (i!=dbgoutfd) close(i);
#else
	for (i=getnumfds()-1;i>=0;i--) close(i);
#endif


#ifdef EXWAITDEBUG
        for(;;);  /* wait to do adopted */
#endif

	if (e->etype != CRONEVENT ) {
		/* open jobfile as stdin to shell */
#if defined(SecureWare) && defined(B1)
		if (ISB1) {
                    at_cmdfile = xmalloc(strlen(ATDIR)+MAXNAMLEN+strlen(e->cmd)+3);
                    (void) cron_jobname(cwd, "", ATDIR, e->cmd, at_cmdfile);
		}
		else {
		    at_cmdfile = xmalloc(strlen(ATDIR)+strlen(e->cmd)+2);
		    (void *)strcat(strcat(strcpy(at_cmdfile,ATDIR),"/"),e->cmd);
		}
#else
		at_cmdfile = xmalloc(strlen(ATDIR)+strlen(e->cmd)+2);
		if (at_cmdfile == NULL)
			exit(101);
		(void *)strcat(strcat(strcpy(at_cmdfile,ATDIR),"/"),e->cmd);
#endif

#ifdef DEBUG
		fprintf (dbgoutfp,"at_cmdfile (0%#x): %s\n", at_cmdfile,
			 at_cmdfile);
		fflush (dbgoutfp);
#endif

		if (stat(at_cmdfile,&buf)) exit(102);
		if (!(buf.st_mode&ISUID)) {
			/* if setuid bit off, original owner has 
			   given this file to someone else	*/
			unlink(at_cmdfile);
			exit(103); 
		}
		if (open(at_cmdfile,O_RDONLY) == -1) {
			mail((e->u)->name,BADJOBOPEN,2);
			unlink(at_cmdfile);
			exit(104); 
		}

#ifdef AUDIT
		/*
		 * it is appropriate to get the audit ID now
		 * because the job is run only once.
		 */
	    {
		auto	FILE *		rfp = (FILE *)0;
		auto	char *		aidfname = (char *)0;


		/*
		 * construct audit ID file name
		 */
		aidfname = xmalloc(strlen(ATAIDS) + strlen(e->cmd) + 2);
		(void)sprintf(aidfname, "%s/%s", ATAIDS, e->cmd);

		/*
		 * get audit ID
		 */
		(e->u)->audid = (aid_t)-1;
		if ((rfp = fopen(aidfname, "r")) != (FILE *)0) {
			(void)fscanf(rfp, "%ld", &((e->u)->audid));
			(void)fclose(rfp);
		}

		/*
		 * audit ID file is no longer needed
		 */
		(void)unlink(aidfname);

		(void)free(aidfname);
	    }
#endif /* AUDIT */

		unlink(at_cmdfile); 
	}

#ifdef SecureWare
#ifdef B1
	if (ISB1)
            cron_set_user_environment((e->u)->seclevel_ir, (e->u)->name,
                                  (e->u)->uid);
	else
	    if (ISSECURE)
            	cron_set_user_environment((e->u)->name, (e->u)->uid);
#else
	if (ISSECURE)
            cron_set_user_environment((e->u)->name, (e->u)->uid);
#endif
#endif
#ifdef DEBUG
	fprintf(dbgoutfp,
                "set user environment done for cron child, user name: %s \n",
		(e->u)->name);
	fflush(dbgoutfp);
#endif

#ifdef AUDIT
    {
	auto	struct passwd *	pw = (struct passwd *)0;
	auto	char		cpath[MAXPATHLEN];
	auto	char		npath[MAXPATHLEN];
	auto	int		state = 0;

/* additions for att patch */
        auto    struct s_passwd * spw = (struct s_passwd *)0;

	/*
	 * first check if auditing is on.
	 */
	cpath[0] = (char)0;
	npath[0] = (char)0;
	errno = 0;
	state = audctl(AUD_GET, cpath, npath, 0);
	if ((state == -1) && (errno == ENOENT)) {
		/* cough */
		state = 0;
	}
	if (state == 0) {
		/*
		 * the current policy regarding invalid audit ID's
		 * is that the job will be ignored and the submitter
		 * will be notified.
		 */
		if ((e->u)->audid == (aid_t)-1) {
			/*
			 * tell submitter and exit
			 */
			mail((e->u)->name, NOAUDID, 2);
			exit(105); 
		}
	}


	/*
	 * turn audproc flag on by default
	 */
	(e->u)->audproc = AUD_PROC;

	/*
	 * at the start of every job, we must check the audproc
	 * flag in the secure password file because the audproc
	 * flag can be turned on or off at any time.
	 */
	
/* added for att patch */
        /* check existance of shadow password file */
        secure = (stat(SECUREPASS, &s_pfile) < 0) ? 0:1;

        if (secure) {

        /* search through shadow password file */
           setspwent();
           while ((spw = getspwent()) != (struct s_passwd *)0) {
                   if (spw->pw_audid == (e->u)->audid) {
                           (e->u)->audproc = spw->pw_audflg;
                           break;
                   }
           }
           endspwent();
        }
        else {
        /* search through password file */
           setpwent();
           while ((pw = getpwent()) != (struct passwd *)0) {
                   if (pw->pw_audid == (e->u)->audid) {
                           (e->u)->audproc = pw->pw_audflg;
                           break;
                   }
           }
           endpwent();
        }

	/*
	 * set audit ID and audproc flag
	 */
	(void)setaudid((e->u)->audid);
	(void)setaudproc((e->u)->audproc);
    }
#endif /* AUDIT */

	/* set correct user and group identification */

#ifdef	FSS
	/*
	 * Change to the appropriate fair share group.  Do this before
	 * changing uid, since only the superuser is allowed to change
	 * fair share groups.
	 */
	if (chfsg((e->u)->uid))
		exit(1);
#endif
	if ((setgid((e->u)->gid)<0) || (initgroups((e->u)->name,(e->u)->gid)<0) || (setuid((e->u)->uid)<0)) {

#ifdef DEBUG
        fprintf (dbgoutfp,"Couldn't set group / user id's exiting\n");
        fflush (dbgoutfp);
#endif
		exit(106);
	}

#ifdef SecureWare
	if (ISSECURE)
            cron_adjust_privileges();
#endif
	sp_flag = FALSE;
	if (e->etype == CRONEVENT) {

#ifdef DEBUG
	fprintf(dbgoutfp,
                "Checking for std input to command of child process\n");
	fflush(dbgoutfp);
#endif

		/* check for standard input to command	*/
		if (e->of.ct.input != NULL) {
			cron_infile = mktemp(TMPINFILE);
			if ((fd=creat(cron_infile,INMODE)) == -1) {
				mail((e->u)->name,NOSTDIN,2);
				exit(107); 
			}
			if (write(fd,e->of.ct.input,strlen(e->of.ct.input))
			    != strlen(e->of.ct.input)) {
				mail((e->u)->name,NOSTDIN,2);
				unlink(cron_infile);
				exit(108); 
			}
			close(fd);
			/* open tmp file as stdin input to sh	*/
			if (open(cron_infile,O_RDONLY)==-1) {
				mail((e->u)->name,NOSTDIN,2);
				unlink(cron_infile);
				exit(109); 
			}
			unlink(cron_infile); 
		}
		else if (open("/dev/null",O_RDONLY)==-1) {
			(void *)open("/",O_RDONLY);
			sp_flag = TRUE; 
		}
        }

	/* redirect stdout and stderr for the shell	*/
#if defined(SecureWare) && defined(B1)
	if (ISB1)
            (void) forcepriv(SEC_MULTILEVELDIR);
#endif
	if (creat(rp->outfile,OUTMODE)!=-1) dup(1);
	else if (open("/dev/null",O_WRONLY)!=-1) dup(1);
#ifdef DEBUG
	else fprintf (dbgoutfp,"couldn't open output file or /dev/null\n");
#endif
	if (sp_flag) close(0);
	(void *)strcat(homedir,(e->u)->home);
	(void *)strcat(log_name,(e->u)->name);
	(void *)strcat(timez, getenv(timez));
	environ = envinit;
	if (chdir((e->u)->home)==-1) {
		mail((e->u)->name,CANTCDHOME,2);
		exit(110); 
	}
#ifdef TESTING
	exit(111);
#endif
	if ((e->u)->uid != 0)
		nice(qp->nice);
	if (e->etype == CRONEVENT)
		execl(SHELL,"sh","-c",e->cmd,0);
	else /* type == ATEVENT */
		execl(SHELL,"sh",0);
	mail((e->u)->name,CANTEXECSH,2);
	exit(112);
}

/* This code is to handle the death of a child processes, and was originally
 * only called synchronously after issuing the wait() call.  It has been
 * moved here to a seperate function so that it may be called asynchronously
 * from a signal handler.   -- Dan Dickerman 7/15/93
 */

void
child_handler ()
{
	struct	runinfo	*rp;
	int pid, prc;

#ifdef DEBUG
		printf("entering child handler.  running = %d\n", running);
		fflush(stdout);
#endif

	/* Loop in case of multiple children */
	while ((pid = waitpid (-1, &prc, WNOHANG)) != 0 && pid != -1) { 

#ifdef DEBUG
		printf("found child pid = %ld\n", pid);
		fflush(stdout);
#endif
		for (rp=rt;rp < rt+MAXRUN; rp++)
			if (rp->pid == pid)
				break;
		if (rp >= rt+MAXRUN) {
			msg1((catgets(nlmsg_fd,NL_SETN,32, "unexpected pid returned %d (ignored)")),pid);

#ifdef DEBUG
			printf("wait returned %O\n",prc);
#endif
			/* incremented in mail() */
			running--;
		}
		else {
			if (rp->que == ZOMB) {
				running--;
				rp->pid = 0;
				free(rp->outfile);
				unlink(rp->outfile);
	                	free(rp->rcmd);

	/* If rp->rusr->ctevents is zero this indicates that del_ctab was   */  
	/* called previously on this usr but the usr had a process running. */
	/* Free the usr space now that this process is complete.            */

				if ((rp->rusr != NULL) 
		    		&& (rp->rusr->atevents == NULL)
		    		&& (rp->rusr->ctevents == NULL))
					del_ctab(rp->rusr->name);
			}
			else
				cleanup(rp,prc);
		}
	}

	/* Reset the signal handler if necessary.  Although using system V
         * signals for this does run a risk of tail recursion, hopefully the
         * while loop will leave only a very small window for this recursion. 
         * Since cron can only have a fixed number of children anyway, this 
         * is not a large risk to take in terms of interrupt stack space.
         */

	if (running)
		signal (SIGCLD, child_handler);

#ifdef DEBUG
		printf("exiting child handler.   running = %d\n", running);
		fflush(stdout);
#endif

}


/***************/
idle()
/***************/
{
	time_t	now;
	int	pid;
	long	alm;

#ifdef SecureWare
	/* fixes bug in original code -- if the last action was a delete,
	   don't delay a whole minute on this pass.  Old code delayed for
	   a minute if there is no pending action, then delays a minute in
	   msg_wait().  We only need one of those delays or user requests
	   can easily get ahead of cron. */

        /* Code involving last_was_del removed due to the fact that under
           this new scheme, we don't delay at all */
#endif
	do {

		if (running) { /* Set up trap handler in case of children */
#ifdef DEBUG
		printf ("Entered init - turning on SIGCLD\n");
#endif
			signal (SIGCLD, child_handler);
		} 
#ifdef DEBUG
		else
			printf ("Entered init - SIGCLD off\n");
#endif

		if (msg_wait())
			return;  /* message has modified the event list */
                                 /* Otherwise just loop, and go to sleep */

		now = time((char *) 0);

	} while (next_event == NULL || next_event->time > now);

	return;
}


/*****************/
cleanup(pr,rc)
/*****************/
struct	runinfo	*pr;
{

	int	fd;
	char	line[5+UNAMESIZE+CTLINESIZE];
	struct	usr	*p;
	struct	stat	buf;

	logit((char)ECHAR,pr,rc);
	--qt[pr->que].nrun;
	pr->pid = 0;
	--running;
	p = pr->rusr;
#if defined(SecureWare) && defined(B1)
	if (ISB1)
            (void) cron_setlevel(p->seclevel_ir);
#endif
#ifdef ATLIMIT
	if (pr->que != CRONEVENT)
		--p->aruncnt;
#endif
#if ATLIMIT && CRONLIMIT
	else
		--p->cruncnt;
#else
#ifdef CRONLIMIT
	if (pr->que == CRONEVENT)
		--p->cruncnt;
#endif
#endif
	if (!stat(pr->outfile,&buf)) {
		if (buf.st_size > 0) {
			if ((fd=open(pr->outfile,O_WRONLY|O_APPEND)) == -1)
				mail(p->name,STDOUTERR,2);
			else {
				if (pr->que == CRONEVENT) {
					(void *)write(fd,STDERRMSG2,strlen(STDERRMSG2));
					(void *)write(fd,pr->rcmd,strlen(pr->rcmd));
					(void *)write(fd, "\n", 1);

				}
				else if (pr->que == ATEVENT)
					(void *)write(fd,STDERRMSG3,strlen(STDERRMSG3));
				else

					(void *)write(fd,STDERRMSG,strlen(STDERRMSG));
				close(fd);
				/* mail user stdout and stderr */
#if defined(SecureWare) && defined(B1)
				if (ISB1)
                                    cron_mail_line(line, MAIL, p->name,
                                               pr->outfile);
				else {
				    sprintf(line,"/bin/echo 'Subject: %s\n' | /bin/cat - \"%s\" | %s %s\n",
					pr->que == CRONEVENT ? "cron" : "at",
					pr->outfile,MAIL,p->name);
				}
#else	
				sprintf(line,"/bin/echo 'Subject: %s\n' | /bin/cat - \"%s\" | %s %s\n",
					pr->que == CRONEVENT ? "cron" : "at",
					pr->outfile,MAIL,p->name);
#endif
				if ((pr->pid = fork()) == 0) {
#ifdef SecureWare
					if (ISSECURE)
                                            (void) cron_mail_setup(1);
#endif
					execl("/bin/sh","sh","-c",line,0);
					exit(127);
				}
				pr->que = ZOMB;
				running++;

			}
		} else {
			free(pr->outfile);
			unlink(pr->outfile);
			free(pr->rcmd);

         /* If pr->rusr->ctevents is zero this indicates that del_ctab was   */
         /* called previously on this usr but the usr had a process running. */
         /* Free the usr space now that this process is complete.            */

                        if ((pr->rusr != NULL) && (pr->rusr->atevents == NULL) 
			   && (pr->rusr->ctevents == NULL))
                           del_ctab(pr->rusr->name);
		}
	}
	return;
}

#define	MSGSIZE	sizeof(struct message)

/*****************/
msg_wait()
/*****************/
{

	long	t;
	time_t	now;
	struct	message	*pmsg;
	int	cnt;

	if (next_event == NULL)
		t = INFINITY;
	else {
		now = time((long *) 0);
		t = next_event->time - now;
		if (t <= 0L)
			t = 1L;
	}
#ifdef DEBUG
	printf("in msg_wait - setting alarm for %ld sec\n", t);
	fflush(stdout);
#endif
	alarm((unsigned) t);
	pmsg = &msgbuf;
#if defined(SecureWare) && defined(B1)
        if ((ISB1) && ((pmsg = (struct message *) cron_set_message((char *) &msgbuf,
                        sizeof(*pmsg) - sizeof(pmsg->mlevel),
                        &notexpired)) == (struct message *) 0))
                return(1);
        if ((ISB1) ? (!cron_read_message(msgfd, (char *) pmsg,
                               sizeof(*pmsg) - sizeof(pmsg->mlevel), &cnt)) :
	             ((cnt=read(msgfd,pmsg,MSGSIZE)) != MSGSIZE))
#else	
	if ((cnt=read(msgfd,pmsg,MSGSIZE)) != MSGSIZE) 
#endif
	{
		notexpired = 1;  /* put cron back to sleep when finished here */

		if ((cnt = -1) && (errno != EINTR)) {
			perror("! read");
		}

		return(0);
	}

	alarm(0);
	signal (SIGCLD, SIG_DFL); /* Prevent async event list modification */

#ifdef SecureWare
	/* if this action is a DELETE, note it so that we wait only a short
	   time for the next message */
	last_was_del = (pmsg->action == DELETE);
#endif
	if (pmsg->etype != NULL) {
		switch (pmsg->etype) {
		case AT:
			if (pmsg->action == DELETE)
				del_atjob(pmsg->fname,pmsg->logname);
			else
				mod_atjob(pmsg->fname,pmsg->action);
			break;
		case CRON:
			if (pmsg->action == DELETE)
				del_ctab(pmsg->fname);
			else
				mod_ctab(pmsg->fname,pmsg->action);
			break;
		default:
			msg0((catgets(nlmsg_fd,NL_SETN,34, "message received - bad format")));
			break;
		}
		if (next_event != NULL) {
			if (next_event->etype == CRONEVENT) {
#ifdef SecureWare
				sprintf(auditbuf, "add cron job=%s, time=%s",
                                  next_event->cmd, fulltime(&next_event->time));
                        	audit_subsystem(auditbuf, "cron event added",
                                  ET_SUBSYSTEM);
#endif
				el_add(next_event,next_event->time,(next_event->u)->ctid);
			} else { /* etype == ATEVENT */
#ifdef SecureWare
                        	sprintf(auditbuf, "add at job=%s, time=%s",
                                  next_event->cmd, fulltime(&next_event->time));
                        	audit_subsystem(auditbuf, "at event added",
                                  ET_SUBSYSTEM);
#endif
				el_add(next_event,next_event->time,next_event->of.at.eventid);
			}
			next_event = NULL;
		}
		fflush(stdout);
		pmsg->etype = NULL;
		notexpired = 1;
		return(1);
	}
}


/*****************/
timeout()
/*****************/
{
	signal(SIGALRM, timeout);
}

/*****************/
crabort(mssg)
/*****************/
char *mssg;
{
	/* crabort handles exits out of cron */
	int c;

	/* write error msg to console */
	if ((c=open(CONSOLE,O_WRONLY))>=0) {
		(void *)write(c,(catgets(nlmsg_fd,NL_SETN,35, "cron aborted: ")),14);
		(void *)write(c,mssg,strlen(mssg));
		(void *)write(c,"\n",1);
		close(c); 
	}
	msg0(mssg);
	msg0((catgets(nlmsg_fd,NL_SETN,36, "******* CRON ABORTED ********")));
	exit(1);
}

/*****************/
msg0(fmt)
/*****************/
char *fmt;
{
	time_t	t;

	t = time((long *) 0);
	printf("! ");
	printf(fmt);
	printf(" %s\n", fulltime(&t));
	fflush(stdout);
	return;
}

/****************/
msg1(fmt,a)
/*****************/
char *fmt;
{
	time_t	t;

	t = time((long *) 0);
	printf("! ");
	printf(fmt,a);
	printf(" %s\n", fulltime(&t));
	fflush(stdout);
	return;
}

/*****************/
msg2(fmt,a,b)
/*****************/
char *fmt;
{
	time_t	t;

	t = time((long *) 0);
	printf("! ");
	printf(fmt,a,b);
	printf(" %s\n", fulltime(&t));
	fflush(stdout);
	return;
}


/*****************/
logit(cc,rp,rc)
/*****************/
char	cc;
struct	runinfo	*rp;
{
	time_t t;
	int    ret;
	char *printbuf, *xmalloc();

	t = time((long *) 0);
	if (cc == BCHAR)
		printf("%c  CMD: %s\n",cc, next_event->cmd);
	printf("%c  %.8s %u %c %s",
		cc, (rp->rusr)->name, rp->pid, QUE(rp->que),
		fulltime(&t));
	if ((ret=TSTAT(rc)) != 0)
		printf(" ts=%d",ret);
	if ((ret=RCODE(rc)) != 0)
		printf(" rc=%d",ret);
	putchar('\n');
	fflush(stdout);
	/*
	 *   Did the user requested notification of job completion?
	 */
	if ((cc != BCHAR) && (rp->notify_user == ADDNOTIFY))
	{
	    printbuf=xmalloc(200);
	    sprintf(printbuf,"cron|at job completed: %s %u %c %s",
		    rp->rcmd, rp->pid, QUE(rp->que), fulltime(&t));
	    mail((rp->rusr)->name,printbuf,2);
	    free(printbuf);
	}
	return;
}

/***************/
resched(delay)
/***************/
int	delay;
{
	time_t	nt;

	/* run job at a later time */
	nt = next_event->time + delay;
	if (next_event->etype == CRONEVENT) {
		next_event->time = next_time(next_event);
		if (nt < next_event->time)
			next_event->time = nt;
		el_add(next_event,next_event->time,(next_event->u)->ctid);
		delayed = 1;
		msg0((catgets(nlmsg_fd,NL_SETN,37, "rescheduling a cron job")));
		return;
	}
	add_atevent(next_event->u, next_event->cmd,nt,next_event->notify_user);
	msg0((catgets(nlmsg_fd,NL_SETN,38, "rescheduling at job")));
	return;
}

#define	QBUFSIZ		80

/**********/
quedefs(action)
/**********/
int	action;
{
	register i;
	char	qbuf[QBUFSIZ];
	FILE	*fd;

	/* set up default queue definitions */
	for (i=0;i<NQUEUE;i++) {
		qt[i].njob = qd.njob;
		qt[i].nice = qd.nice;
		qt[i].nwait = qd.nwait;
	}
	if (action == DEFAULT)
		return;
	if ((fd = fopen(QUEDEFS,"r")) == NULL) {
		msg0((catgets(nlmsg_fd,NL_SETN,39, "cannot open quedefs file")));
		msg0((catgets(nlmsg_fd,NL_SETN,40, "using default queue definitions")));
		return;
	}
	while (fgets(qbuf, QBUFSIZ, fd) != NULL) {
		if ((i=qbuf[0]-'a') < 0 || i >= NQUEUE || qbuf[1] != '.')
			continue;
		parsqdef(&qbuf[2]);
		qt[i].njob = qq.njob;
		qt[i].nice = qq.nice;
		qt[i].nwait = qq.nwait;
	}
	fclose(fd);
	return;
}

/**********/
parsqdef(name)
/**********/
char *name;
{
	register i;

	qq = qd;
	while (*name) {
		i = 0;
		while (isdigit(*name)) {
			i *= 10;
			i += *name++ - '0';
		}
		switch (*name++) {
		case JOBF:
			qq.njob = i;
			break;
		case NICEF:
			qq.nice = i;
			break;
		case WAITF:
			qq.nwait = i;
			break;
		}
	}
	return;
}

#ifdef	FSS

int nofss = 0;		/* assume there is a fair share scheduler */
/*
 * handler() is invoked iff an fss system calls fails because there
 * is no such system call in the running kernel.  Since we can handle
 * this without lying or dying, we do so.
 */
/****************/
handler() {
/****************/
	nofss = 1;	/* no fair share scheduler */
}

/*
 * Change fair share group to the default group for the user identified
 * by uid.  This group can be found in /etc/fsg by looking at the comma-
 * separated list of fair share groups at the end of the line that starts
 * with the name of the user associated with the given uid.  Only lines
 * with empty fsid and shares fields are considered in the search.
 */
/****************/
chfsg(uid)
/****************/
int uid;
{
	int oldsig;
	struct passwd *pw;
	struct passwd *getpwuid();
	int *getfsguser();
	int fsid;

	oldsig = signal(SIGSYS, handler);
	fss(FS_STATE);			/* try the system call */
	signal(SIGSYS, oldsig);		/* restore the old value */
	if (nofss)			/* no fair share scheduler gen'd in? */
		return 0;		/* fine */
	pw = getpwuid(uid);
	if (pw == NULL)			/* no such user in /etc/passwd? */
		return 0;		/* fine */
	fsid = getfsguser(pw->pw_name);
	if (fsid != NULL)			/* user appears in /etc/fsg? */
		fss(FS_CHPID, fsid, getpid());	/* change to the new fs group */
	return 0;			/* ok, did it */
}

#endif	/* FSS */

extern FILE *fdopen();

/****************/
FILE *
cron_popen(cmd, mode)
/****************/
char *cmd;
char *mode;
{
    int p[2], fd;
    register pid_t pid;
    FILE *filefd;

    if (pipe(p) < 0)
	return (FILE *)NULL;
#ifdef DEBUG
    printf("cron_popen: pipe[0]=%d  pipe[1]=%d\n",p[0],p[1]);
#endif

    if ((pid = fork()) == (pid_t)0)
    {
#ifdef WAITDEBUG
	for (;;);
#endif
        close(p[1]); /*close down write side of pipe */
	if (p[0] != 0)
	{
	      (void)close(0); /* close stdin */

              /* dup read side of pipe into stdin */
              if ((fd = dup(p[0])) < 0) 
	      {
	           _exit(2);
	      }
	}
#ifdef DEBUG
        if ((p[0] !=0) && (fd != 0))
	   _exit(3);
#endif
	(void)execl("/bin/sh", "sh", "-c", cmd, 0);
	_exit(1);
    }
#ifdef DEBUG
    printf("pid of child=%d\n",pid);
#endif

    if (pid == -1)
    {
	/*
	 * If the fork() failed, we must close the two file descriptors
	 * of the pipe.  Save the errno from the fork(), so that the
	 * close() calls don't mess it up.
	 */
	int errno_save = errno;
	(void)close(p[0]);
	(void)close(p[1]);
	errno = errno_save;

	return (FILE *)NULL;
    }

    /*
     * Close the read side of pipe, turn our pipe into a FILE
     */
    (void)close(p[0]); 
    filefd = fdopen(p[1], mode);
    return (filefd);
}
