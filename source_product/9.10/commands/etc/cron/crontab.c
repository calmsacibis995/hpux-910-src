/* @(#) $Revision: 72.3 $ */      
/**************************************************************************
 ***			C r o n t a b . c				***
 **************************************************************************

	date:	7/2/82
	description:	This program implements crontab (see cron(1)).
			This program should be set-uid to root.
	files:
		/usr/lib/cron drwxr-xr-x root sys
		/usr/lib/cron/cron.allow -rw-r--r-- root sys
		/usr/lib/cron/cron.deny -rw-r--r-- root sys

 **************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <nl_types.h>
#include "cron.h"

#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#endif

#ifdef AUDIT 
#include <sys/audit.h>
#include <pwd.h>
#endif

#define TMPFILE		"_cron"		/* prefix for tmp file */
#define CRMODE		0444	/* mode for creating crontabs */
#define NL_SETN		1

#define BADCREATE	(catgets(nlmsg_fd,NL_SETN,1, "can't create your crontab file in the crontab directory."))
#define BADOPEN		(catgets(nlmsg_fd,NL_SETN,2, "can't open your crontab file."))
#define BADSHELL	(catgets(nlmsg_fd,NL_SETN,3, "because your login shell isn't /bin/sh, you can't use cron."))
#define WARNSHELL	(catgets(nlmsg_fd,NL_SETN,4, "warning: commands will be executed using /bin/sh\n"))
#define BADUSAGE	(catgets(nlmsg_fd,NL_SETN,5, "proper usage is: \n	crontab [file]\n	crontab [-r]\n	crontab [-l]"))
#define INVALIDUSER	(catgets(nlmsg_fd,NL_SETN,6, "you are not a valid user (no entry in /etc/passwd)."))
#define NOTALLOWED	(catgets(nlmsg_fd,NL_SETN,7, "you are not authorized to use cron.  Sorry."))
#define EOLN		(catgets(nlmsg_fd,NL_SETN,8, "unexpected end of line."))
#define UNEXPECT	(catgets(nlmsg_fd,NL_SETN,9, "unexpected character found in line."))
#define OUTOFBOUND	(catgets(nlmsg_fd,NL_SETN,10, "number out of bounds."))

extern int per_errno;
int err,cursor,catch();
char *cf,*tnam,line[CTLINESIZE];
extern char *xmalloc();
nl_catd nlmsg_fd;


/* structure used for passing messages from the
   at and crontab commands to the cron			*/
struct message msgbuf;
main(argc,argv,environ)
int argc;
char **argv;
char **environ;
{
	char login[UNAMESIZE],*getuser(),*strcat(),*strcpy();
	char *pp;
	FILE *fp;

#ifdef SecureWare
	if(ISSECURE)
            set_auth_parameters(argc, argv);
#endif
	nlmsg_fd=catopen("crontab",0);
	if (argc>2) crabort(BADUSAGE);
#ifdef SecureWare
	if(ISSECURE)
            pp = crontab_get_user();
	else
	    pp = getuser(getuid());
#else
	pp = getuser(getuid());
#endif
	if(pp == NULL) {
		if (per_errno==2)
			crabort(BADSHELL);
		else
			crabort(INVALIDUSER); 
	}
	strcpy(login,pp);
	if (!allowed(login,CRONALLOW,CRONDENY))
        {
#ifdef SecureWare
		if (ISSECURE)
                	audit_subsystem("verify user allowed to use cron",
                        	"not allowed, crontab aborted", ET_RES_DENIAL);
#endif
                crabort(NOTALLOWED);
        }

	cf = xmalloc(strlen(CRONDIR)+strlen(login)+2);
	strcat(strcat(strcpy(cf,CRONDIR),"/"),login);
	if ((argc==2) && (strcmp(argv[1],"-r")==0)) {
		unlink(cf);

#ifdef AUDIT
		/*
		 * get rid of audit ID file
		 */
	    {
		auto	char *		aidfname = (char *)0;

		aidfname = xmalloc(strlen(CRONAIDS) + strlen(login) + 2);
		(void)sprintf(aidfname, "%s/%s", CRONAIDS, login);
		(void)unlink(aidfname);
	    }
#endif /* AUDIT */

		sendmsg(DELETE,login);
		exit(0); 
	}
	if ((argc==2) && (strcmp(argv[1], "-l") ==0)) {
		if((fp = fopen(cf,"r")) == NULL)
			crabort(BADOPEN);
		while(fgets(line,CTLINESIZE,fp) != NULL)
			fputs(line,stdout);
		fclose(fp);
		exit(0);
	}

	if (argc==1) copycron(stdin);
	else {
		if (access(argv[1], 04) != 0)       /* this program runs as */
			crabort(BADOPEN);           /* root, so must check */
		if ((fp=fopen(argv[1],"r"))==NULL)  /* permissions!! */
			crabort(BADOPEN);
		else
			copycron(fp);
	}

#ifdef AUDIT
    {
	auto	FILE *		wfp = (FILE *)0;
	auto	char *		jobname = (char *)0;
	auto	int		wfd = 0;
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
		 */
		jobname = xmalloc(strlen(CRONAIDS) + strlen(login) + 2);
		(void)sprintf(jobname, "%s/%s", CRONAIDS, login);

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
				(void)fprintf(wfp, "%d\n", aid);
			}
			(void)fclose(wfp);
			(void)close(wfd);	/* necessary if fdopen() failed */
		}
	}
    }
#endif /* AUDIT */

	if (!err) {
		sendmsg(ADD,login);
		if (per_errno == 2)
			fprintf(stderr,WARNSHELL);
	}
	catclose(nlmsg_fd);
	exit(0);
}


/******************/
copycron(fp)
/******************/
FILE *fp;
{
	FILE *tfp,*fdopen();
	char pid[6],*strcat(),*strcpy();
	int t;

	sprintf(pid,"%-5d",getpid());
	tnam=xmalloc(strlen(CRONDIR)+strlen(TMPFILE)+7);
	strcat(strcat(strcat(strcpy(tnam,CRONDIR),"/"),TMPFILE),pid);
	/* catch SIGINT, SIGHUP, SIGQUIT signals */
	if (signal(SIGINT,catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP,catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
#ifdef SecureWare
	if(ISSECURE)
            t= crontab_secure_create(tnam, BADCREATE);
	else{
	    if ((t=creat(tnam,CRMODE))==-1) crabort(BADCREATE);
	}
#else	
	if ((t=creat(tnam,CRMODE))==-1) crabort(BADCREATE);
#endif
	if ((tfp=fdopen(t,"w"))==NULL) {
		unlink(tnam);
		crabort(BADCREATE); 
	}
	err=0;	/* if errors found, err set to 1 */
	while (fgets(line,CTLINESIZE,fp) != NULL) {
		cursor=0;
		while(line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if ((line[cursor] == '#') || (line[cursor] == '\0')
			|| (line[cursor] == '\n'))
			goto cont;
		if (next_field(0,59)) continue;
		if (next_field(0,23)) continue;
		if (next_field(1,31)) continue;
		if (next_field(1,12)) continue;
		if (next_field(0,06)) continue;
		if (line[++cursor] == '\0') {
			cerror(EOLN);
			continue; 
		}
cont:
		if (fputs(line,tfp) == EOF) {
			unlink(tnam);
			crabort(BADCREATE); 
		}
	}
	fclose(fp);
	fclose(tfp);
	if (!err) {
		/* make file tfp the new crontab */
		unlink(cf);
		if (link(tnam,cf)==-1) {
			unlink(tnam);
			crabort(BADCREATE); 
		} 
	}
	unlink(tnam);
}


/*****************/
next_field(lower,upper)
/*****************/
int lower,upper;
{
	int num,num2;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	if (line[cursor] == '\0') {
		cerror(EOLN);
		return(1); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			cerror(UNEXPECT);
			return(1); 
		}
		return(0); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			cerror(UNEXPECT);
			return(1); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			cerror(OUTOFBOUND);
			return(1); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				cerror(UNEXPECT);
				return(1); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				cerror(OUTOFBOUND);
				return(1); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			cerror(EOLN);
			return(1); 
		}
		if (line[cursor++]!=',') {
			cerror(UNEXPECT);
			return(1); 
		}
	}
	return(0);
}


/**********/
cerror(msg)
/**********/
char *msg;
{
	char savemsg[128];

	strcpy(savemsg,msg);
	fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,11, "%scrontab: error on previous line; %s\n")),line,savemsg);
	err=1;
}


/**********/
catch()
/**********/
{
	unlink(tnam);
	exit(1);
}


/**********/
crabort(msg)
/**********/
char *msg;
{
	fprintf(stderr,"crontab: %s\n",msg);
	exit(1);
}

/***********/
sendmsg(action,fname)
/****************/
char action;
char *fname;
{

	static	int	msgfd = -2;
	struct	message	*pmsg;

#if defined(SecureWare) && defined(B1)
	if(ISB1){
            pmsg = (struct message *) crontab_set_message((char *) &msgbuf,
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
		if((msgfd = open(FIFO,O_WRONLY|O_NDELAY)) < 0) {
			if(errno == ENXIO || errno == ENOENT)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,12, "cron may not be running - call your system administrator\n")));
			else
				fprintf(stderr,catgets(nlmsg_fd,NL_SETN,13, "crontab: error in message queue open, errno=%d\n"),errno);
			return;
		}
	pmsg->etype = CRON;
	pmsg->action = action;
	strncpy(pmsg->fname,fname,FLEN);
#if defined(SecureWare) && defined(B1)
        if ((ISB1) ? (!crontab_write_message(msgfd, (char *) pmsg,
                                   sizeof(*pmsg) - sizeof(pmsg->mlevel))) :
	             (write(msgfd,pmsg,sizeof(struct message)) !=
						sizeof(struct message)))
#else
	if(write(msgfd,pmsg,sizeof(struct message)) != sizeof(struct message))
#endif
#ifdef SecureWare
	{
        	if (ISSECURE) {
                	audit_subsystem("send message to cron daemon",
                          "message not sent", ET_SUBSYSTEM);
		}
                fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,14, "crontab: error in message send\n")));
	}
#else
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,14, "crontab: error in message send\n")));
#endif
}
