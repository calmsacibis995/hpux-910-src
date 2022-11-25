/* @(#) $Revision: 70.4.1.1 $ */
/* LP Scheduler */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 9	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_types.h>
nl_catd nlmsg_fd;
nl_catd nlmsg_tfd;
#endif NLS

#ifdef AUDIT
#include <sys/audit.h>
#define SECUREPASS	"/.secure/etc/passwd"
#endif

#include	"lp.h"
#include	"lpsched.h"
#include	<errno.h>
#ifdef TRUX
#include <sys/security.h>
#endif
extern int errno;

int pgrp;			/* process group and process id of scheduler */
FILE *rfifo = NULL;
FILE *wfifo = NULL;
char errmsg[200];
short wrt;		/* TRUE ==> write to user instead of mailing */
short mail;		/* TRUE ==> user requested mail, FALSE ==> no mail */

/*
	The following global variables are the primary means of passing
	values among the functions of children of the scheduler.
*/

short sigterm = FALSE;		/* TRUE => received SIGTERM from scheduler */
FILE *rfile;			/* Used ONLY for reading request file */
char *pr;			/* Name of printer currently printing */
char *dev;			/* Device to which pr is printing */
char *dst;			/* Destination of current print request */
char *log_name;			/* Requestor of current request */
int seqno;			/* Sequence # of current print request */
#ifdef REMOTE
int hpuxbsd;			/* Indicates request type (3 or 4 digits */
char *ohostname;		/* Name or originating host */
#endif REMOTE
int pid;			/* process id  of child */
char *cmd[ARGMAX];		/* printer interface command line */
int nargs;			/* # of args in cmd array */
char rname[RNAMEMAX];		/* name of request file being processed */

#ifdef REMOTE
char rem_sending[FILEMAX];	/* path of the semaphore being processed */
FILE	*send_file;		/* pointer to FILE structure of semaphore */ 
#endif REMOTE

int vflag;			/* flags verbose logging */
int aflag;			/* flags analysis logging */

main(argc, argv)
int argc;
char *argv[];
{

#ifdef NLS || NLS16
	unsigned char lctime[5+4*MAXLNAME+4], *pc;
	unsigned char savelang[5+MAXLNAME+1];
	nlmsg_fd = catopen("lp");
	strcpy(lctime, "LANG=");	/* $LC_TIME affects some msgs */
	strcat(lctime, getenv("LC_TIME"));
	if (lctime[5] != '\0') {	/* if $LC_TIME is set */
		strcpy(savelang, "LANG=");	/* save $LANG */
		strcat(savelang, getenv("LANG"));
		if ((pc = strchr(lctime, '@')) != NULL) /*if modifier*/
			*pc = '\0';	/* remove modifer part */
		putenv(lctime);		/* use $LC_TIME for some msgs */
		nlmsg_tfd = catopen("lp", 0);
		putenv(savelang);	/* reset $LANG */
	} else				/* $LC_TIME is not set */
		nlmsg_tfd = nlmsg_fd;	/* use $LANG messages */
#endif NLS || NLS16
#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
#ifdef B1
	    if(ISB1){
        	initprivs();
        	(void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	}
#endif 
	options(argc, argv);

	startup(argv[0]);

	pinit();		/* initialize printers from MEMBER directory */
	cinit();		/* initialize classes from CLASS directory */
	openfifo();		/* open fifo to receive requests */
	enqueue(F_NEWLOG, "");	/* Make new error log */
	cleanoutq();		/* clean up output queue and initialize
				   printer and class output lists */
	cleanfiles();		/* remove orphan request and data files */
	psetup();		/* Clean up printer status file,
				   initialize printer status info,
				   and write "printer ready" messages
				   on fifo for all enabled printers */
	schedule();		/* schedule user requests for printing */
/* NOTREACHED */
} /* main */


/*
 * buildcmd -- builds command line to be given to printer interface program.
 *		 The result is left in cmd.
 *		 Returns:  0 for no errors.
 *		 Returns: -1 for errors, issue error messages.
 *		 Returns: -2 for errors, do not issue error messages.
 */

int
buildcmd()
{
	char arg[FILEMAX], type, file[NAMEMAX];
	struct	pstat	p;
	struct	outq	o;
	extern	FILE	*lockf_open();
#ifdef REMOTE
	if(getpdest(&p, dst) != EOF && strcmp(p.p_remoteprinter, "")){
		sprintf(rem_sending, "%s/%s/%s", REQUEST, dst, REMOTESENDING);
		if( (send_file = lockf_open(rem_sending, "w", TRUE)) == NULL){
		    endpent();
		    return(-1);
		}
	}
	endpent();
#endif REMOTE

#ifdef REMOTE
	if (hpuxbsd){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, dst, seqno, ohostname);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, dst, seqno, ohostname);
	}
	if(getoidhost(&o, dst, seqno, ohostname) == EOF){
		endoent();
		if (vflag){
			sprintf(errmsg,(catgets(nlmsg_fd,NL_SETN,1, "attenmpted to open a file that had been removed. seqno is %d\n")), seqno);
			fatal(errmsg,0);
		}
		return(-2);
	}
#else REMOTE
	sprintf(rname, "%s/%s/r-%d", REQUEST, dst, seqno);
	if(getoid(&o, dst, seqno) == EOF){
		endoent();
		if (vflag){
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,1, "attenmpted to open a file that had been removed. seqno is %d\n")), seqno);
			fatal(errmsg,0);
		}
		return(-2);
	}

#endif REMOTE

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,rname,0640,ADMIN,
		"request file to build commands from");
	else
	    chmod(rname, 0640);
#else
	chmod(rname, 0640);
#endif

	o.o_flags |= O_PROCESSED;
	time(&o.o_pdate);
	putoent(&o);
	endoent();

	if((rfile = lockf_open(rname, "r", FALSE)) == NULL){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,2, "Unable to open and lock \"%s\"\n")), rname);
		fatal(errmsg, 0);
		return(-1);
	}

	wrt = mail = FALSE;
	nargs = 0;
	sprintf(arg, "%s/%s", INTERFACE, pr);
	if(enter(arg, cmd, &nargs, ARGMAX) == -1)
	    return(-1);
	sprintf(arg, "%s-%d", dst, seqno);
	if(enter(arg, cmd, &nargs, ARGMAX) == -1)
	    return(-1);
	if(enter(log_name, cmd, &nargs, ARGMAX) == -1)
	    return(-1);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		if(enter(dev, cmd, &nargs, ARGMAX) == -1)
			return(-1);
	}
#endif

	while(getrent(&type, arg, rfile) != EOF) {
		switch(type) {
		case R_FILE:
			if(arg[0] != '/') {
				/* file under SPOOL/REQUEST/dst */
				strcpy(file, arg);
				sprintf(arg, "%s/%s/%s/%s", SPOOL, REQUEST,
					dst, file);
			}
#ifdef REMOTE
		case R_HEADERTITLE:
#else
		case R_TITLE:
#endif REMOTE
		case R_COPIES:
		case R_OPTIONS:
			if(enter(arg, cmd, &nargs, ARGMAX) == -1)
				return(-1);
			break;
		case R_WRITE:		/* if a remote machine or */
			wrt = TRUE;	/* a remote printer is */
			break;		/* specified, then mail and */
		case R_MAIL:		/* wrt are reset to false. */
			mail = TRUE;	/* this is so mail is not */
			break;		/* sent except when the */
					/* is really printed. */
		default:
			break;
		}
	}

	/* unlock the request file for other program */

	if(unlock_file(rfile) != 0){
		fclose(rfile);
		return(-1);
	}

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,rname,0440,ADMIN,
		"unlock request file after command built");
	else
	    chmod(rname, 0440);
#endif

	if(enter((char *) NULL, cmd, &nargs, ARGMAX) == -1)
		return(-1);
	return(0);
} /* buildcmd */


/*
 * cinit -- initialize destination structure from entries in CLASS directory
 */

cinit()
{
	DIR  *cd;
	FILE *cf;
	char member[DESTMAX + 1], cfile[sizeof(CLASS) + DESTMAX + 1];
	struct dirent *dirbuf;
	struct dest *d, *m, *getp(), *newdest();
	char *c;
	extern struct dest class;

	if((cd = opendir(CLASS)) == (DIR *) NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,3, "can't open CLASS directory")), 1);

	while((dirbuf = readdir(cd)) != (struct dirent *) NULL) {
		if(dirbuf->d_ino != 0 && dirbuf->d_name[0] != '.') {
			d = newdest(dirbuf->d_name);
			d->d_status = D_CLASS;
			insert(&class, d);
			sprintf(cfile, "%s/%s", CLASS, dirbuf->d_name);
			if((cf = fopen(cfile, "r")) == NULL) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,4, "can't open %s file in CLASS directory")), dirbuf->d_name);
				fatal(errmsg, 0);
			}
			else {
				while(fgets(member, FILEMAX, cf) != NULL) {
				   if(*(c=member+strlen(member)-1) == '\n')
					*c = '\0';
				   if((m = getp(member)) == NULL) {
#ifndef NLS
					sprintf(errmsg, "non-existent printer %s in class %s", member, dirbuf->d_name);
#else NLS
					sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,5, "non-existent printer %s in class %s")), member, dirbuf->d_name);
#endif NLS
					fatal(errmsg, 0);
				   }
				   else
					newmem(d, m);
				}
				fclose(cf);
			}
		}
	}
	closedir(cd);
} /* cinit */


/*
 * cleanfiles -- remove request and data files which have no entry in
 *	the output queue, temporary request files and mysterious files
 */

cleanfiles()
{
	struct dest *d;
	extern struct dest dest;
	DIR *f;
	struct dirent *entry;
#ifdef REMOTE
	char fullfile[RNAMEMAX], *file, *file1, *file2, *format34;
	/* char *strchr(), *strcpy(); */
#else
	char fullfile[RNAMEMAX], *file, *seq, *strchr(), *strcpy();
#endif REMOTE
	struct outq o;
	int s;

	struct stat statbuf;

	FORALLD(d) {
		sprintf(fullfile, "%s/%s/", REQUEST, d->d_dname);
		if((f = opendir(fullfile)) == (DIR *) NULL) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,6, "can't open request directory %s")), fullfile);
			fatal(errmsg, 1);
		}

		/* remove any request and data files that are not
		   mentioned in the output queue */

		file = &fullfile[strlen(fullfile)];
		while((entry = readdir(f)) != (struct dirent *) NULL) {
			if(entry->d_ino != 0 && entry->d_name[0] != '.') {
				strcpy(file, entry->d_name);
#ifdef REMOTE
				 /* check access time of file */
                                /* do not remove if younger than */
                                /* MAXFILEAGE seconds (defined in lpsched.h) */

                                stat(fullfile,&statbuf);
                                if (statbuf.st_atime > (time((time_t)NULL) - MAXFILEAGE))
                                {
                                        continue; /* file too young, may be */
                                                  /* still being created */
                                }

#if defined(SecureWare) && defined(B1)
                                if (((ISB1) ? ((*file < 'c' || *file > 's') &&
                                     *file != LP_FILTER_PREFIX) :
					((*file < 'c') || (*file > 's'))) ||
#else
				if ((*file < 'c') || (*file > 's') ||
#endif
				    (strlen(file) < 7)){
					unlink(fullfile);
				}else{
				    file1 = file + 1;
				    if (*(file1) == 'f'){
					file1 = file1 + 2;
					file2 = file1 + 3;
					format34 = "%03d";
				    }else{
					file1++;
					file2 = file1 + 4;
					format34 = "%04d";
				    }
				    
				    switch(*file){
					    case 'c' :	/* request file */
					    case 'd' :  /* data file */
						if ((sscanf(file1,format34,&s) != 1) ||
						    getoidhost(&o,d->d_dname,s,file2)==EOF)
						    unlink(fullfile);
						break;
					    default:
						break;
				    }
				}
#else
				switch(*file) {
				case 'd': /* data file */
				case 'r': /* request file */
#if defined(SecureWare) && defined(B1)
                                case LP_FILTER_PREFIX: /* filter file */
					if(!ISB1)
					    break;
#endif
					if((seq = strchr(file, '-')) == NULL ||
					   (s = atoi(++seq)) <= 0 ||
					   getoid(&o,d->d_dname,s)==EOF)
						unlink(fullfile);
					break;
				default:
					unlink(fullfile);
					break;
				}
#endif REMOTE
			}
		}
		closedir(f);
	}
	endoent();
} /* cleanfiles */


/*
 * cleanoutq -- clean up output queue :
 *		remove deleted records
 *		mark printing requests as not printing
 *	Initialize printer and class output lists.
 */

cleanoutq()
{
	struct outq o;
	FILE *otmp = NULL;
	struct dest *d, *getd();

	if((otmp = fopen(TOUTPUTQ, "w")) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,7, "can't open temporary output queue")), 1);

#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, TOUTPUTQ, 0644, ADMIN,
                       "temporary printer output queue");
	else
		chmod(TOUTPUTQ, 0644);
#else	
	chmod(TOUTPUTQ, 0644);
#endif

	while(getoent(&o) != EOF) {
		if(o.o_flags & O_PRINT) {
			o.o_flags &= ~O_PRINT;
			sprintf(o.o_dev, "-");
			time(&o.o_date);
		}

		wrtoent(&o, otmp);
		if((d = getd(o.o_dest)) != NULL)
#ifdef REMOTE
			inserto(d, o.o_priority, o.o_seqno, o.o_logname, o.o_host, (o.o_rflags & O_OB3));
#else
			inserto(d, o.o_priority, o.o_seqno, o.o_logname);
#endif REMOTE
	}

	fclose(otmp);
	do {
	if(unlink(OUTPUTQ) == -1 && errno != ENOENT)
	    fatal((catgets(nlmsg_fd,NL_SETN,8, "can't unlink old output queue")), 1);
	} while(link(TOUTPUTQ, OUTPUTQ) == -1 && errno == EEXIST);
	endoent();

	if(unlink(TOUTPUTQ) == -1)
	    fatal((catgets(nlmsg_fd,NL_SETN,8, "can't unlink old output queue")), 1);
} /* cleanoutq */


/*
 * cleanup -- called by fatal
 */

cleanup()
{
	struct dest *p;
	extern struct dest printer;

	endpent();
	endoent();
	unlink(SCHEDLOCK);

	unlink(FIFO);

	if(vflag)
		dump();

	FORALLP(p)
		if(p->d_status & D_BUSY)
			waitfor(p, -1);
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "***** STOPPED  %s *****\n")), date(time((time_t *)0)));
	exit(0);
} /* cleanup */

/*
 * disable(reason) -- disable printer pr because of specified reason
 */

disable(reason)
char *reason;
{
	char xqt[ARGMAX];

#ifndef NLS
	sprintf(xqt, "%s/%s -r\"disabled by scheduler: %s\" %s", USRDIR, DISABLE, reason, pr);
#else NLS
	sprintf(xqt, "%s/%s -r\"%s%s\" %s", USRDIR, DISABLE, 
		(catgets(nlmsg_fd,NL_SETN,11, "disabled by scheduler: ")), reason, pr);
#endif NLS
	system(xqt);
} /* disable */


/*
 * findprinter(d) -- find an available printer which can print a request
 *	that has been queued for destination d.  If such a printer p is found,
 *	then give it a request to print.
 */

findprinter(d)
struct dest *d;
{
	struct destlist *head, *dl;
	struct dest *findp = NULL;
	struct pstat pr;
	short fence = MAXPRI;
	int status;

	if(d->d_status & D_PRINTER) {
		if((d->d_status & D_ENABLED) && !(d->d_status & D_BUSY))
			makebusy(d);
	}
	else {
		for(head = d->d_class, dl = head->dl_next;
		    dl != head; dl = dl->dl_next) {

			status = (dl->dl_dest)->d_status;
			if(!(status & D_PRINTER) ||
			     !(status & D_ENABLED) || (status & D_BUSY))
			    continue;
			getpdest(&pr,(dl->dl_dest)->d_dname);
			if(pr.p_fence <= fence){
				findp = dl->dl_dest;
				fence = pr.p_fence;
			}
			endpent();
		}
		if(findp != NULL)
			makebusy(findp);
	}
} /* findprinter */


/*
 * makebusy(p) -- if there is a request to print on printer p, then print it
 */

makebusy(p)
struct dest *p;
{
	struct outlist *o, *nextreq();
	int ret, sig15(), killchild(), status;
	extern int (*f_clean)();

	if((o = nextreq(p)) == NULL)	/* No requests to print on printer p */
		return;

	fprintf(stderr, "%s-%d\t%s\t%s\t%s\n", (o->ol_dest)->d_dname,
	   o->ol_seqno, o->ol_name, p->d_dname, date(time((time_t *)0)));
	fflush(stderr);		/* update log file */

	/*
	 *	Fork to do the printing --
	 *	the parent will continue the scheduling
	 *	child #1 will fork and exec interface program
	 *	child #2 will wait for interface program to complete
	 */

	signal(SIGTERM, sig15);		/* Delay the handling of SIGTERM */
	f_clean = NULL;
	while((pid = fork()) == -1)	/* so the son does not call cleanup */
		;
	if(pid != 0) {	/* FATHER */		/* set in-memory status */
		f_clean = cleanup;
		signal(SIGTERM, cleanup);	/* reset to call cleanup */

		if(sigterm)		/* if a SIGTERM came in before the */
			cleanup();	/* signal handler was set back to */
					/* cleanup, go to cleanup anyway. */
		p->d_pid = pid;
		o->ol_print = p;
		p->d_status |= D_BUSY;
		p->d_print = o;
		return;		/* back to scheduling */
	}
	/* SON */

	/* Establish values for key global variables */

	pid = getpid();
	pr = p->d_dname;
	dev = p->d_device;
	dst = (o->ol_dest)->d_dname;
	log_name = o->ol_name;
	seqno = o->ol_seqno;
#ifdef REMOTE
	ohostname = o->ol_host;
	hpuxbsd	= o->ol_ob3;
#endif REMOTE

	preprint();		/* prepare for printing */

	while((pid = fork()) == -1)	/* 2nd level child does printing */
		if(sigterm)
			exit(0);
	if(pid == 0) {
		pid = setpgrp();
		if((ret = setstatus()) != 0)
			exit((ret < 0) ? EX_SYS : (EX_SYS | EX_READY));
		if(sigterm)
			exit(EX_SYS | EX_TERM);
		execvp(cmd[0], cmd);	/* execute interface program */
                close(1);
                if(fopen("/dev/null", "r+", stdout) == NULL) {
                    fprintf(stderr,"INTERFACE:Problem  opening /dev/null as stdout");
                    fflush(stderr);
                }
		sprintf(errmsg,(catgets(nlmsg_fd,NL_SETN,12, "Can't execute printer interface program (errno=%d)\n")),errno);
		wrtmail(log_name,errmsg);
		signal(SIGTERM, SIG_IGN);
		sprintf(errmsg,"cancel %s-%d\n",dst,seqno);
		system(errmsg);
		exit(EX_SYS);
	}
	signal(SIGTERM, killchild);	/* kill interface on SIGTERM */
	while(wait(&status) != pid)	/* 1st child waits for 2nd child */
		;
	postprint(status);		/* printing done -- clean up */
	if(vflag) {
		if(freopen(ERRLOG, "a", stderr) != NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "\tCHILD: %s  %s %d %.4x\n")), date(time((time_t *)0)), dst, seqno, status);
			fflush(stderr);
		}
	}
	exit(0);
} /* makebusy */

/*
 * newdev(p, d) -- associates device d with printer p
 */

newdev(p, d)
struct dest *p;
char *d;
{
	char *c;
        int d_length;           /* length of device file name */

        if(p->d_device != NULL)
                free(p->d_device);

        d_length = strlen(d) + 1;
        if (d_length < strlen("/dev/null")+1)
            d_length = strlen("/dev/null")+1;  /* must hold "/dev/null" */
        if((c = malloc((unsigned)d_length)) == NULL)
                fatal(CORMSG, 1);
        strcpy(c, d);
        p->d_device = c;

} /* newdev */


/*
 * nextreq(p) -- returns a pointer to the oldest output request that may
 *	be printed on printer p.
 *	This will be either a request queued specifically for p or
 *	one queued for one of the classes which p belongs to.
 *	If there are no requests for p, then nextreq returns NULL.
 */

struct outlist *
nextreq(p)
struct dest *p;
{
	int	t;
	struct pstat pr;
	struct outlist *o, *ohead, *oldest;
	struct prilist *pl, *plhead;
	struct destlist *dl, *dlhead;
	short found, priority;

	t = 0;
	oldest = NULL;
	priority = 0;

	/* search printer's output list */

	getpdest(&pr,p->d_dname);
	for( plhead = p->d_priority, pl = plhead->pl_next ;
	    t == 0 && pl != plhead && pl->priority >= pr.p_fence ; pl = pl->pl_next){
		for( ohead = pl->pl_output, o = ohead->ol_next ; t == 0 && o != ohead ;
		    o = o->ol_next){
			if( o->ol_print == NULL){
				oldest = o;
				t = o->ol_time;
				priority = o->ol_priority;
			}
		}
	}
	endpent();

	/* search output lists of classes that p belongs to */
	
	for( dlhead = p->d_class, dl = dlhead->dl_next ;
	    dl != dlhead ; dl = dl->dl_next){
		
		for( found = FALSE, plhead = (dl->dl_dest)->d_priority, 
		    pl = plhead->pl_next ; !found && pl != plhead 
		    && pl->priority >= pr.p_fence && pl->priority >= priority
		    ; pl = pl->pl_next){

			for( ohead = pl->pl_output, o = ohead->ol_next ;
			    !found && o != ohead ; o = o->ol_next){
				
				if(o->ol_print == NULL){
					found = TRUE;
					if(o->ol_time < t || t == 0){
						oldest = o;
						t = o->ol_time;
						priority = o->ol_priority;
					}
				}
			}
		}
	}
	return(oldest);
} /* nextreq */


/*
 * opendev -- reopen stdin, stdout and stderr and make sure that interface
 *	is executable.
 */

opendev()
{
	char xqt[sizeof(INTERFACE) + DESTMAX + 1];
	int sig14(), fd1, fd2;
	char mode[3];
	struct pstat pstatus;
	FILE *f;

	sprintf(xqt, "%s/%s", INTERFACE, pr);
	errmsg[0] = '\0';

	getpdest(&pstatus, pr);
	endpent();

	if ( pstatus.p_flags & P_CNODE )
	    strcpy( dev, "/dev/null" );

	if(GET_ACCESS(dev, ACC_W) != 0)
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,14, "can't write to %s")), dev);
	else if(GET_ACCESS(xqt, ACC_X) != 0)
		strcpy(errmsg, EXECMSG);
	else {
		freopen("/dev/null", "r+", stdin);
		mode[0] = 'a';
		mode[1] = mode[2] = '\0';
#ifdef SecureWare
		if(ISSECURE)
                	lp_set_printer_device(dev);
#endif 
		if(GET_ACCESS(dev, ACC_R) == 0)	/* open for r+w, if possible */
			mode[1] = '+';
		signal(SIGALRM, sig14);
		f = NULL;
		alarm(OPENTIME);
		f = freopen(dev, mode, stdout);
		alarm(0);
		signal(SIGALRM, SIG_DFL);
		if(f == NULL)
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,15, "can't open %s")), dev);
		else {
			fclose(stderr);
			fd1 = fileno(stdout);
			if((fd2 = dup(fd1)) < 0 || fdopen(fd2, mode) != stderr)
				strcpy(errmsg, (catgets(nlmsg_fd,NL_SETN,16, "can't reopen stderr")));
		}
	}

	if (sigterm)
	   exit (0);
	if(errmsg[0] != '\0') {
		disable(errmsg);
		exit(0);
	}
} /* opendev */


sig14()
{
}


/*
 * openfifo -- open fifo to queue printer requests for data
 */

openfifo()
{
	if(GET_ACCESS(FIFO,ACC_R|ACC_W) == -1 && mknod(FIFO,S_IFIFO|0600,0) != 0)
		fatal((catgets(nlmsg_fd,NL_SETN,17, "Can't access FIFO")), 1);

#ifdef SecureWare
	if(ISSECURE)
        	lp_change_mode(SPOOL, FIFO, 0644, ADMIN,
                       "printer communications file");
#endif
	if((wfifo=fopen(FIFO, "w+"))==NULL || (rfifo=fopen(FIFO, "r"))==NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,18, "can't open FIFO")), 1);
} /* openfifo */


/*
 * pinit -- initialize destination structure from entries in MEMBER directory
 */

pinit()
{
	DIR  *md;
	FILE *m;
	char device[FILEMAX], memfile[sizeof(MEMBER) + DESTMAX + 1];
	struct dirent *dirbuf;
	struct dest *d, *newdest();
	char *c;
	extern struct dest printer;

	if((md = opendir(MEMBER)) == (DIR *) NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,19, "can't open MEMBER directory")), 1);

	while((dirbuf = readdir(md)) != (struct dirent *) NULL){
		if(dirbuf->d_ino != 0 && dirbuf->d_name[0] != '.') {
			d = newdest(dirbuf->d_name);
			d->d_status = D_PRINTER;
			insert(&printer, d);

			sprintf(memfile, "%s/%s", MEMBER, dirbuf->d_name);
			if((m = fopen(memfile, "r")) == NULL) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,20, "can't open %s file in MEMBER directory")), dirbuf->d_name);
				fatal(errmsg, 0);
				d->d_device = NULL;
			}
			else {
				if(fgets(device, FILEMAX, m) == NULL)
					d->d_device = NULL;
				else {
				   if(*(c=device+strlen(device)-1) == '\n')
					*c = '\0';
				   newdev(d, device);
				}
				fclose(m);
			}
		}
	}
	closedir(md);
} /* pinit */


/*
 * postprint(status) -- clean up after printing a request
 *	status is the return code from interface program
 */

postprint(status)
int status;
{
	int term, excode;
	char	work[BUFSIZ];
#ifdef AUDIT
	char arg[FILEMAX], type;
	char audname[FILEMAX];
	char *txtptr;
	struct self_audit_rec audrec;
	int saveuid;
        int fcount=0;

	audname[0]='\0';
#endif

	excode = status >> 8;
	term = status & 0177;

#ifdef REMOTE	
	/* unlock and close the remote semaphore */
	fclose(send_file);
#endif REMOTE

/*
	Note: d0 or 0320.  The shell returns this
	when it receives a SIGTERM and is executing
	a internal command.
*/
        close(1);
        if(fopen("/dev/null", "r+", stdout) == NULL) {
           fprintf(stderr,"CHILD:Problem  opening /dev/null as stdout");
                        fflush(stderr);
        }
	if(term == SIGTERM || excode == (EX_SYS | EX_TERM) || excode == (0320) || excode == 3 || excode == 1)
		/* interface was killed */
		resetstatus(0, 1);
	else if(excode & EX_SYS) {	/* system error exit */
		if(excode & EX_RESET)
			resetstatus(0, 1);
		if(excode & EX_READY){
			sprintf(work, "%s %d", pr, (getp(pr)->d_pid));
		        enqueue(F_MORE, work);
		}

	}
	else {			/* printer interface exited */
		fclose(rfile);	/* unlock the request file */
#ifdef AUDIT
		/* save file name for audit record */
                /* use checking to make sure that we do not go out
                   of array bounds. This was only while auditing
                   was set                                     */
		chmod( rname, 0640 );
		 if((rfile = lockf_open(rname, "r", FALSE)) != NULL) {
			while (getrent(&type, arg, rfile) != EOF) {
				switch(type) {
				case R_FILENAME:
                                        fcount += strlen(arg);
                                        if(fcount < FILEMAX -1)
				           strcat (audname, arg);
					break;
				default:
					break;
				}
			}
		} 
		fclose(rfile);	/* re-close the request file */
#endif
		if((excode ==0) && aflag)  /* if no errors from interface */
#ifdef REMOTE
		    analysis_out(dst, seqno, ohostname);
#else REMOTE
		    analysis_out(dst, seqno);
#endif REMOTE

		resetstatus(1, 1);
		if(excode != 0) {
#ifndef NLS
			sprintf(errmsg, "error code %d in request %s-%d on printer %s", excode, dst, seqno, pr);
#else NLS
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,21, "error code %d in request %s-%d on printer %s")), excode, dst, seqno, pr);
#endif NLS
			wrtmail(log_name, errmsg);
#ifdef AUDIT
			/* need to be root to write a record */
			saveuid = getuid();
			setresuid(0,0,0);
			
			/* we have not suspended low-level auditing
			   because we don't feel comfortable enough
			   with this code to do so -- still write a
			   high level record */

			/* Construct a self audit record for the event */
			txtptr = (char *)audrec.aud_body.text;
			strcpy (txtptr, "File ");
			strcat (txtptr, audname);
			strcat (txtptr, " was not printed for user ");
			strcat (txtptr, log_name);
			strcat (txtptr, " on printer ");
			strcat (txtptr, pr);
			strcat (txtptr, " due to an error");
			audrec.aud_head.ah_pid = getpid();
			audrec.aud_head.ah_error = excode;
			audrec.aud_head.ah_event = EN_LP;
			audrec.aud_head.ah_len = strlen (txtptr);
			audwrite(&audrec);

			/* restore LP uid */
			setresuid(saveuid, saveuid, 0);
#endif AUDIT
		}
		else {		/* no errors detected by interface pgm */
#ifdef AUDIT
			/* need to be root to write a record */
			saveuid = getuid();
			setresuid(0,0,0);
			
			/* we have not suspended low-level auditing
			   because we don't feel comfortable enough
			   with this code to do so -- still write a
			   high level record */

			/* Construct a self audit record for the event */
			txtptr = (char *)audrec.aud_body.text;
			strcpy (txtptr, "File ");
			strcat (txtptr, audname);
			strcat (txtptr, " was printed for user ");
			strcat (txtptr, log_name);
			strcat (txtptr, " on printer ");
			strcat (txtptr, pr);
			audrec.aud_head.ah_pid = getpid();
			audrec.aud_head.ah_error = excode;
			audrec.aud_head.ah_event = EN_LP;
			audrec.aud_head.ah_len = strlen (txtptr);
			audwrite(&audrec);

			/* restore LP uid */
			setresuid(saveuid, saveuid, 0);
#endif AUDIT
			fclose(rfile);
			unlink(rname);  

/* associate stdout and stderr with the log file.
   This allows messages from mail to be logged.
   If for some reason the log file cannot be opened
   open stdout and stderr as /dev/null */

			if(freopen(ERRLOG, "a", stdout) == NULL) {
				freopen("/dev/null", "a", stdout);
			}
			if(freopen(ERRLOG, "a", stderr) == NULL) {
				freopen("/dev/null", "a", stderr);
			}

#ifndef NLS
			sprintf(errmsg, "printer request %s-%d has been printed on printer %s", dst, seqno, pr);
#else NLS
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,22, "printer request %s-%d has been printed on printer %s")), dst, seqno, pr);
#endif NLS

			if(mail)
#ifdef REMOTE
				sendmail(log_name, ohostname, errmsg);
#else
				sendmail(log_name, errmsg);
#endif REMOTE
			if(wrt && !mail)
				wrtmail(log_name, errmsg);
		}
		sprintf(work, "%s %d", pr, -1);/* pr ready for more requests */
		enqueue(F_MORE, work);
	}
} /* postprint */


/*
 * preprint -- prepare for printing:
 *	build interface program command line
 *	make sure interface program is executable
 *	open device for writing and reading (if possible)
 */

preprint()
{
	int	temp;
	char	work[BUFSIZ];

	if(sigterm)
		exit(0);
	temp = buildcmd();	/* Format interface pgm command line */
	if(temp != 0) {
		if (temp == -2){
			exit(0);
		}
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,23, "error in printer request %s-%d")), dst, seqno);
		wrtmail(log_name, errmsg);
		resetstatus(1, 0);
		sprintf(work, "%s %d", pr, (getp(pr)->d_pid));
		enqueue(F_MORE, work);
		exit(0);
	}
	if(sigterm)
		exit(0);
	opendev();	/* open printer's device */
	if(sigterm)
		exit(0);
} /* preprint */


/*
 * psetup -- set up printer status file and place names of enabled printers
 *	     on fifo
 */

psetup()
{
	struct pstat p;
	struct dest *d, *getp();
	char	work[BUFSIZ];

	while(getpent(&p) != EOF) {
		if(p.p_flags & P_ENAB) {
			if(p.p_flags & P_AUTO) {
				/* automatic disable */
				p.p_flags &= ~(P_BUSY | P_ENAB);
#ifdef REMOTE
				p.p_pid = 0;
				p.p_seqno = -1;
#else
				p.p_pid = p.p_seqno = 0;
#endif REMOTE
				sprintf(p.p_rdest, "-");
				sprintf(p.p_reason, (catgets(nlmsg_fd,NL_SETN,24, "disabled by scheduler: login terminal"))); time(&p.p_date);
				putpent(&p);
			}
			else if(p.p_flags & P_BUSY) {
				p.p_flags &= ~P_BUSY;
#ifdef REMOTE
				p.p_pid = 0;
				p.p_seqno = -1;
#else
				p.p_pid = p.p_seqno = 0;
#endif REMOTE
				sprintf(p.p_rdest, "-");
				putpent(&p);
			}
		}

		if((d = getp(p.p_dest)) == NULL) {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,25, "non-existent printer %s in PSTATUS")), p.p_dest);
			fatal(errmsg, 0);
		}
		else if(p.p_flags & P_ENAB) {
			d->d_status |= D_ENABLED;
			sprintf(work, "%s %d", p.p_dest, -1);
			enqueue(F_MORE, work);
		}
	}
	endpent();
} /* psetup */


/*
 * resetstatus(oflag, pflag) -- reset entries in outputq and pstatus to show
 *	that printer pr is no longer printing dst-seqno.
 *
 *	if oflag != 0 then delete outputq entry and remove associated data
 *		and request files.
 *	if pflag != 0 then reset pstatus entry for printer pr.
 */

resetstatus(oflag, pflag)
int oflag, pflag;
{
	struct outq o;
	struct pstat p;

	if(getoid(&o, dst, seqno) != EOF) {
		if(oflag != 0) {
			o.o_flags |= O_DEL;
#ifdef REMOTE
			rmreq(dst, seqno, o.o_host, (o.o_rflags & O_OB3));
#else
			rmreq(dst, seqno);
#endif REMOTE
		}
		else {
			o.o_flags &= ~O_PRINT;
			strcpy(o.o_dev, "-");
		}
		putoent(&o);
	}
	if(pflag != 0) {
		if(getpdest(&p, pr) != EOF) {
			p.p_flags &= ~P_BUSY;
#ifdef REMOTE
			p.p_pid = 0;
			p.p_seqno = -1;
			if ((p.p_rflags & (P_OMA|P_OPR)) != 0){
				mail	= FALSE;
				wrt	= FALSE;
			}
#else
			p.p_pid = p.p_seqno = 0;
#endif REMOTE
			strcpy(p.p_rdest, "-");
			putpent(&p);
		}
		endpent();
	}
	endoent();
} /* resetstatus */

/*
 * schedule() --
 *	This routine reads a fifo (FIFO) forever.  The input on the fifo
 *	is a mix of the following messages and commands:
 *	F_ENABLE	enable printer
 *	F_DISABLE	disable printer
 *	F_ZAP		disable printer and cancel the request which
 *			it is currently printing
 *	F_MORE		printer ready for more input
 *	F_REQUEST	an output request has been received
 *	F_DEST		alteration destination of output request
 *	F_PRIORITY	alteration priority of output request
 *	F_CANCEL	cancel output request
 *	F_STATUS	status dump
 *	F_QUIT		shut down the scheduler
 *	F_NOOP		check if scheduler is running
 *	F_NEWLOG	create new error log
 */

schedule()
{
	struct dest *d, *p;
	struct dest *newd;
#ifdef NLS
	char xqt[ARGMAX];
#endif NLS
	char msg[MSGMAX], dst_local[DESTMAX + 1], name[LOGMAX+1];
	char newdst_local[DESTMAX + 1];
#ifdef REMOTE
	char hostname_cmd[SP_MAXHOSTNAMELEN];
#endif REMOTE
	char *hname, *dest_x,*c, *arg, cmd_local, dev_local[FILEMAX];
        /*hname temporary storage for hostname*/
	int seqno_local, newseqno_local;
	int number;
        int seqnum;  /* request sequence */
	int priority;
	int process_id;
	struct outlist *o, *ocheck,*geto(), *nextreq();
        /* ocheck: temporary storage for next requst on queue */
	struct outq out,otemp;  /*otemp temporary outq struct */         

	while(TRUE) {
		/* get next message from fifo */
		fgets(msg, MSGMAX-1, rfifo);
		if(*(c = msg + strlen(msg) -1) == '\n')
			*c = '\0';
		cmd_local = msg[0];
		arg = &msg[2];

		if(vflag) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,26, "\tFIFO: %s  %s\n")), date(time((time_t *)0)), msg);
			fflush(stderr);
		}

		switch(cmd_local) {
		case F_ENABLE: /* enable printer.  arg1 = printer */
                   if((d=getp(arg)) != NULL && !(d->d_status & D_ENABLED)){
                        endpent();
                        d->d_status |= D_ENABLED;
                        if((ocheck = nextreq(d)) == NULL) {
                          /* this should not happen except on startup */
                                    endpent();
                                    break;
                         } 
                         seqnum = ocheck->ol_seqno;
                         hname = ocheck->ol_host;
                         dest_x = (ocheck->ol_dest)->d_dname;
                         if(getoidhost(&otemp,dest_x,seqnum,hname) == EOF)  {
                             deleteo(ocheck);
                         } 
                         endoent();
                         makebusy(d);
                      } else
                             endpent();
                             break;

		case F_DISABLE: /* disable printer.  arg1 = printer */
		case F_ZAP:	/* disable printer and cancel what's 
				   printing */
			if((p=getp(arg)) != NULL 
			   && (p->d_status & D_ENABLED)) {
				o = NULL;
				if(p->d_status & D_BUSY) {
					waitfor(p, -1);
					p->d_status &= ~D_BUSY;
					o = p->d_print;
					o->ol_print = NULL;
					p->d_print = NULL;
				}
				p->d_status &= ~D_ENABLED;
				if(o != NULL) {
					if(cmd_local == F_DISABLE)
						findprinter(o->ol_dest);
					else
						deleteo(o);
				}
			}
			break;
		case F_MORE: /* printer ready for next request.  
				arg1 = printer , arg2 = pid */
			sscanf(arg, "%s %d", dst_local, &process_id);
			if((d=getp(dst_local)) != NULL
			   && (d->d_status & D_ENABLED)) {
				if(d->d_status & D_BUSY) {
					waitfor(d, process_id);
					d->d_status &= ~D_BUSY;
					deleteo(d->d_print);
					d->d_print = NULL;
				}
				makebusy(d);
		}
			break;
		case F_REQUEST: /* output request received.  arg1 = destination,
			  	   arg2 = sequence #, arg3 = logname */
				/* arg4 = hostname, HP-UX or BSD request */
#ifdef REMOTE
			if(sscanf(arg, "%s %d %s %s %d", dst_local, &seqno_local, name, hostname_cmd, &hpuxbsd ) == 5 && 
#else
			if(sscanf(arg, "%s %d %s", dst_local, &seqno_local, name) == 3 && 
#endif REMOTE
			   (d=getd(dst_local)) != NULL) {
#ifdef REMOTE
				getoidhost(&out, dst_local, seqno_local, hostname_cmd);
				inserto(d, out.o_priority, seqno_local, name, hostname_cmd, (hpuxbsd & O_OB3));
#else
				getoid(&out, dst_local, seqno_local);
				inserto(d, out.o_priority, seqno_local, name);
#endif REMOTE
				endoent();
				findprinter(d);
			}
			break;
	       case F_DEST :
#ifdef REMOTE
		if( (number=sscanf(arg, "%s %d %s %d %s %d",
			dst_local, &seqno_local, newdst_local, &newseqno_local, hostname_cmd, &priority)) == 5 ){
#else REMOTE
		if( (number=sscanf(arg, "%s %d %s %d %d",
			dst_local, &seqno_local, newdst_local, &newseqno_local, &priority)) == 4 ){
#endif REMOTE
			priority = -1;
		}
#ifdef REMOTE
		if ( (number == 5 || number == 6)
		    && (d = getd(dst_local)) != NULL
		    && (newd = getd(newdst_local)) != NULL
		    && (o = geto(d, seqno_local, hostname_cmd)) != NULL){
#else REMOTE
		if ( (number == 4 || number == 5)
		    && (d = getd(dst_local)) != NULL
		    && (newd = getd(newdst_local)) != NULL
		    && (o = geto(d, seqno_local)) != NULL){
#endif REMOTE
			if( o->ol_print != NULL){
				break;
			}else{
				/* alteration request file */
#ifdef REMOTE
				if(mvrequest(dst_local, seqno_local, newdst_local, newseqno_local, (short)priority, hostname_cmd))
#else REMOTE
				if(mvrequest(dst_local, seqno_local, newdst_local, newseqno_local, (short)priority))
#endif REMOTE
				    break;
				/* alteration order of printing */

				mvoutq(o, newd, newseqno_local, (short)priority);
				findprinter(newd);
			}
		}
		break;

		case F_PRIORITY :
#ifdef REMOTE
		if( sscanf(arg, "%s %d %d %s", dst_local, &seqno_local, &priority, hostname_cmd) == 4
		   && (d = getd(dst_local)) != NULL
		   && (o = geto(d, seqno_local, hostname_cmd)) != NULL){
#else REMOTE
		if( sscanf(arg, "%s %d %d", dst_local, &seqno_local, &priority) == 3
		   && (d = getd(dst_local)) != NULL
		   && (o = geto(d, seqno_local)) != NULL){
#endif REMOTE
			if( o->ol_print != NULL){
				break;
			}else{
				/* alteration request file */
#ifdef REMOTE
				if(altrequest(dst_local, seqno_local, (short)priority, hostname_cmd))
#else REMOTE
				if(altrequest(dst_local, seqno_local, (short)priority))
#endif REMOTE
				    break;
				/* alteration order of printing */

				mvoutq(o, d, seqno_local, (short)priority);
				findprinter(d);
			}
		}
		break;

		case F_CANCEL: /* cancel output request.  arg1 = destination,
				  arg2 = sequence #  */
				/* arg3 = hostname */
#ifdef REMOTE
			if(sscanf(arg, "%s %d %s", dst_local, &seqno_local, hostname_cmd) == 3 &&
		   	(d = getd(dst_local)) != NULL &&
		   	(o = geto(d, seqno_local, hostname_cmd)) != NULL) {
#else
			if(sscanf(arg, "%s %d", dst_local, &seqno_local) == 2 &&
		   	(d = getd(dst_local)) != NULL &&
		   	(o = geto(d, seqno_local)) != NULL) {
#endif REMOTE
				if(o->ol_print != NULL) {
					deleteo(o);
					p = o->ol_print;
					waitfor(p, -1);
					p->d_status &= ~D_BUSY;
					p->d_print = NULL;
					makebusy(p);
				}
				else
					deleteo(o);
			}
			break;
		case F_DEV: /* change device for printer.  arg1 = printer,
				arg2 = new device pathname  */
			if(sscanf(arg, "%s %s", dst_local, dev_local) == 2 &&
		   	(d = getp(dst_local)) != NULL)
				newdev(d, dev_local);
			break;
		case F_QUIT: /* shut down scheduler */
			kill(pgrp, SIGTERM);
			break;
		case F_STATUS: /* status dump to error log */
			dump();
			break;
		case F_NOOP: /* no-op */
			break;
		case F_NEWLOG: /* new error log */
			unlink(OLDLOG);
			link(ERRLOG, OLDLOG);
#ifdef SecureWare
			if(ISSECURE)
                        	lp_change_mode(SPOOL, OLDLOG, 0644, ADMIN,
                                       "previous printer log");
			else
				chmod(OLDLOG, 0644);
#else
			chmod(OLDLOG, 0644);
#endif
			unlink(ERRLOG);
			if(freopen(ERRLOG, "a", stderr) == NULL) {
#ifndef NLS
				system("echo lpsched: cannot create log>>log");
#else NLS
				sprintf(xqt, "echo %s >>log", (catgets(nlmsg_fd,NL_SETN,27, "lpsched: cannot create log")));
				system(xqt);
#endif NLS
				exit(1);
			}

#ifdef SecureWare
			if(ISSECURE)
                        	lp_change_mode(SPOOL, ERRLOG, 0644, ADMIN,
                                       "error log for printer");
			else
				chmod(ERRLOG, 0644);
#else
			chmod(ERRLOG, 0644);
#endif
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,28, "***** LP LOG: %s *****\n")), date(time((time_t *)0)));
			fflush(stderr);
			break;
		default:
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,29, "FIFO: '%s' ?")), msg);
			fatal(errmsg, 0);
			break;
		}
	}
} /* schedule */

/* mvrequest -- move request file from REQUEST/dest to REQUEST/newdest */

#ifdef REMOTE
mvrequest(dst, seqno, newdst, newseqno, priority, host)
char *host;
#else REMOTE
mvrequest(dst, seqno, newdst, newseqno, priority)
#endif REMOTE
char *dst, *newdst;
int seqno, newseqno;
short priority;
{
	struct outq o;
#ifdef REMOTE
	struct qstat q;
#endif REMOTE

	FILE *sfile, *tfile, *rfile;
	char sname[RNAMEMAX], tname[RNAMEMAX], rname[RNAMEMAX];
	char req_id[IDSIZE+1], string[2];
	char type, arg[FILEMAX];
	extern	FILE	*lockf_open();

	int namechanged;
	char newname[RNAMEMAX];
	
#ifdef REMOTE
	getoidhost(&o, dst, seqno, host);
#else REMOTE
	getoid(&o, dst, seqno);
#endif REMOTE

	o.o_flags |= O_DEL;
	putoent(&o);
	endoent();

#ifdef REMOTE	
	getqdest(&q, dst);

	if (q.q_ob3){
		sprintf(tname, "%s/%s/tfA%03d%s", REQUEST, newdst, newseqno, host);
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, newdst, newseqno, host);
		sprintf(sname, "%s/%s/cfA%03d%s", REQUEST, dst, seqno, host);
	}else{
		sprintf(tname, "%s/%s/tA%04d%s", REQUEST, newdst, newseqno, host);
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, newdst, newseqno, host);
		sprintf(sname, "%s/%s/cA%04d%s", REQUEST, dst, seqno, host);
	}
	endqent();
#else REMOTE
	sprintf(tname, "%s/%s/t-%d", REQUEST, newdst, newseqno);
	sprintf(rname, "%s/%s/r-%d", REQUEST, newdst, newseqno);
	sprintf(sname, "%s/%s/r-%d", REQUEST, dst, seqno);
#endif REMOTE

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,sname,0640,ADMIN,"request file being moved");
	else
	    chmod(sname, 0640);
#else
	chmod(sname, 0640);
#endif

	if((sfile = lockf_open(sname, "r+", FALSE)) == NULL){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,30, "can't open request file \"%s\"")), sname);
		fatal(errmsg ,0);
		resetstatus(1,0);
#ifdef REMOTE
		deleteo(geto(getd(dst), seqno, host));
#else REMOTE
		deleteo(geto(getd(dst), seqno));
#endif REMOTE
		return(1);
	}

	if(GET_ACCESS(tname, 0) == -1 && GET_ACCESS(rname, 0) == -1 &&
	   (tfile = fopen(tname, "w")) != NULL){
		chmod(tname, 0440);
	}else{
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,31, "can't create temporary request file \"%s\"")), tname);
		fatal(errmsg, 1);
	}

	sprintf(req_id, "%s-%d", newdst, newseqno);
	newname[0] = '\0';
	namechanged = 0;

	/* copy from source-request-file to destination-request-file */

	while( getrent(&type, arg, sfile) != EOF){

		switch(type){
		    case R_JOBNAME :
			putrent(type, req_id, tfile);
			break;
		    case R_PRIORITY :
			if(priority < 0){
				putrent(type, arg, tfile);
			}else{
				sprintf(string, "%d", (int)priority);
				putrent(type, string, tfile);
			}
			break;

		    case R_FILE :
			strcpy (newname, arg);
			/*
			 * check if data file name clashes with some existing
			 * request on new dest
			 */
			if (check_name (newdst, seqno, newseqno, newname, namechanged) == -1) {
			      sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,33,
				   "can't rename datafile \"%s\"")), newname);
			      fatal(errmsg, 0);
			}
			/*
			 * If a request has more than 1 data file, and name of
			 * first data file is changed, names of all data files
			 * must also be changed, even if there is no clash.
			 * i.e all data files of a request should use same seq
			 * id (makes life easier while cleaning up the request).
			 * namechanged variable remembers if first was changed.
			 */
			namechanged = strcmp (newname, arg);

			if(mvdatafile(dst, newdst, arg, newname)){
				resetstatus(1,0);
#ifdef REMOTE
				deleteo(geto(getd(dst), seqno, host));
#else REMOTE
				deleteo(geto(getd(dst), seqno));
#endif REMOTE
			}
			putrent(type, newname, tfile);
			break;

		    case R_FORMATTEDFILE :
		    case R_UNLINKFILE :

			strcpy (arg, newname);
			putrent(type, arg, tfile);
			break;

		    default:
			putrent(type, arg, tfile);
			break;
		}
	}

	/* link new-path to source files and unlink old-path */

	fclose(sfile);
	fclose(tfile);

	if(link(tname, rname) == -1){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,32, "can't create request file \"%s\"")), rname);
		fatal(errmsg, 1);
	}

	unlink(tname);
#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,rname,0400,ADMIN,"new (moved) request file");
	else
	    chmod(rname, 0400);
#else
	chmod(rname, 0400);
#endif

#ifdef REMOTE
	rmreq(dst, seqno, host, (o.o_rflags & O_OB3));
#else REMOTE
	rmreq(dst, seqno);
#endif REMOTE

	/* make new outputq entry */

	if(priority < 0)
	    priority = o.o_priority;

	strcpy(o.o_dest, newdst);
	o.o_seqno = newseqno;
	o.o_flags = 0;
	o.o_priority = priority ;

	addoent(&o);
	endoent();

	return(0);
} /* mvrequest */

/* mvdatafile -- move data file from REQUEST/dest to REQUEST/newdest */

mvdatafile(dst, newdst, datafilename, newname)
char	*dst, *newdst, *datafilename, *newname;
{
	char sname[FILEMAX], dname[FILEMAX];

	sprintf(sname, "%s/%s/%s", REQUEST, dst, datafilename);
	sprintf(dname, "%s/%s/%s", REQUEST, newdst, newname);

#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL,sname,0640,ADMIN,"data file to move");
	else
		chmod(sname, 0640);
#else
	chmod(sname, 0640);
#endif

	if(link(sname, dname) == -1){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,33, "can't move datafile \"%s\"")), sname);
		fatal(errmsg, 0);
		return(1);
	}

	unlink(sname);
#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL,dname,0440,ADMIN,"data file moved");
	else
		chmod(dname, 0440);
#else
	chmod(dname, 0440);
#endif
	return(0);
} /* mvdatafile */

/* altrequest -- alter the contents of request file */

#ifdef REMOTE
altrequest(dst, seqno, priority, host)
char *host;
#else REMOTE
altrequest(dst, seqno, priority)
#endif REMOTE
char *dst;
int seqno;
short priority;
{
	FILE	*rfile, *tfile;
	char	rname[RNAMEMAX], tname[RNAMEMAX];
	char	string[2], type, arg[FILEMAX];
	struct outq o;
#ifdef REMOTE
	struct qstat q;
	extern	FILE	*lockf_open();

	getqdest(&q, dst);

	if(q.q_ob3){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, dst, seqno, host);
		sprintf(tname, "%s/%s/tfA%03d%s", REQUEST, dst, seqno, host);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, dst, seqno, host);
		sprintf(tname, "%s/%s/tA%04d%s", REQUEST, dst, seqno, host);
	}

	endqent();
#else REMOTE
	sprintf(rname, "%s/%s/r-%d", REQUEST, dst, seqno);
	sprintf(tname, "%s/%s/t-%d", REQUEST, dst, seqno);
#endif REMOTE

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,rname,0640,ADMIN,"request file to alter");
	else
	    chmod(rname, 0640);
#else
	chmod(rname, 0640);
#endif

	if((rfile = lockf_open(rname, "r+", FALSE)) == NULL){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,30, "can't open request file \"%s\"")), rname);
		fatal(errmsg, 0);
		resetstatus(1,0);
#ifdef REMOTE
		deleteo(geto(getd(dst), seqno, host));
#else REMOTE
		deleteo(geto(getd(dst), seqno));
#endif REMOTE
		return(1);
	}

	if(GET_ACCESS(tname, 0) == -1 && (tfile = fopen(tname, "w")) != NULL){
#ifdef SecureWare
		if(ISSECURE)
			lp_change_mode(SPOOL,tname,0640,ADMIN,
			"new (altered) request file");
		else
			chmod(tname, 0640);
#else
		chmod(tname, 0640);
#endif
	}else{
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,35, "can't open temporary request file \"%s\"")), tname);
		fatal(errmsg, 1);
	}

	sprintf(string, "%d", (int)priority);

	while( getrent(&type, arg, rfile) != EOF){
		switch(type){
		    case R_PRIORITY :
			putrent(type, string, tfile);
			break;
		    default:
			putrent(type, arg, tfile);
			break;
		}
	}

	/* link tmp-file to real-file */

	fclose(rfile);
	fclose(tfile);

	unlink(rname);

	if(link(tname, rname) == -1){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,32, "can't create request file \"%s\"")), rname);
		fatal(errmsg, 1);
	}
	unlink(tname);

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,rname,0400,ADMIN,"new (altered) request file");
	else
	    chmod(rname, 0400);
#else
	chmod(rname, 0400);
#endif

	/* update outputq entry */

#ifdef REMOTE	
	getoidhost(&o, dst, seqno, host);
#else REMOTE
	getoid(&o, dst, seqno);
#endif REMOTE

	o.o_priority = priority;

	putoent(&o);
	endoent();

	return(0);
} /* altrequest */

      
/* mvoutq -- move outlist */

mvoutq(o, d, seqno, priority)
struct outlist *o;
struct dest *d;
int	seqno;
short	priority;
{
	struct prilist *pri, *getpri();
	struct outlist *head, *next, *ol;

	short found = FALSE;

	/* unbind request from old list */

	(o->ol_next)->ol_prev = o->ol_prev;
	(o->ol_prev)->ol_next = o->ol_next;

	/* alteration members of outlist */

	if(priority < 0){
		priority = o->ol_priority;
	}

	o->ol_dest = d;
	o->ol_seqno = seqno;
	o->ol_priority = priority;

	/* bind request to another list */

	if( (pri=getpri(d, priority)) == NULL ){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,36, "can't get the entry for structure priority \"%d\"")), priority);
		fatal(errmsg, 1);
	}

	head = pri->pl_output;

	for( found = FALSE, ol = head->ol_next; ol != head && found == FALSE
	    ; ol = ol->ol_next){

		if ( ol->ol_time > o->ol_time){
			next = ol;
			found = TRUE;
		}
	}
	
	if( found == FALSE )
	    next = head;

	o->ol_next = next;
	o->ol_prev = next->ol_prev;
	next->ol_prev = o;
	(o->ol_prev)->ol_next = o;
} /* mvoutq */


/*
 * setstatus -- update outputq and pstatus entries to reflect the printing
 *	of the current request
 */

int
setstatus()
{
	struct outq q;
	struct pstat ps;

	if(getoid(&q, dst, seqno) == EOF) {
		endoent();
		return(1);
	}
	if(getpdest(&ps, pr) == EOF) {
		endpent();
		endoent();
		disable((catgets(nlmsg_fd,NL_SETN,37, "entry gone from printer status file")));
		return(-1);
	}
	ps.p_flags |= P_BUSY;
	ps.p_pid = pid;
	strcpy(ps.p_rdest, dst);
	ps.p_seqno = seqno;
#ifdef REMOTE
	strncpy(ps.p_host, ohostname, sizeof(ps.p_host));
	ps.p_host[sizeof(ps.p_host)-1]='\0';
	ps.p_remob3 = q.o_rflags;
#endif REMOTE

	q.o_flags |= O_PRINT;
	strcpy(q.o_dev, pr);
	putpent(&ps);
	putoent(&q);
	endpent();
	endoent();

	return(0);
} /* setstatus */


/* sig15 -- catch SIGTERM */

sig15()
{
	signal(SIGTERM, SIG_IGN);
	sigterm = TRUE;
} /* sig15 */



killchild()
{
	signal(SIGTERM, SIG_IGN);
	kill(-pid, SIGTERM);
} /* killchild */


/* startup -- Initialize */

startup(name)
char *name;
{
	extern char *f_name;
	extern int (*f_clean)();
	int cleanup();
	struct passwd *adm, *getpwnam();
	char readbuf;
	char writebuf;
	int readsize;		/* pipe read buffer */
	int writesize;		/* pipe write buffer */
	int running;		/* status of onelock call */
	int pfd[2];		/* pipe file descriptors */
	pid_t i;		/* proper type for fork return value */

#ifdef AUDIT
	struct s_passwd *s_adm,	*getspwnam();
	struct stat s_pfile;
#endif 

	writebuf = SCHED_INIT;		/* initialize writebuffer */
	f_name = name;

	if(chdir(SPOOL) == -1)
		fatal((catgets(nlmsg_fd,NL_SETN,38, "spool directory non-existent")), 1);

	/* Make sure that the user is an LP Administrator */

	if(! ISADMIN)
		fatal(ADMINMSG, 1);
	if((adm = getpwnam(ADMIN)) == NULL)
		fatal((catgets(nlmsg_fd,NL_SETN,39, "LP Administrator not in password file")), 1);
#ifdef AUDIT
	if (stat(SECUREPASS, &s_pfile) == 0) {	/* file exists */
		if((s_adm = getspwnam(ADMIN)) == NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,40, "LP Administrator not in secure password file")), 1);
		if(setaudproc(s_adm->pw_audflg) == -1) 
			fatal((catgets(nlmsg_fd,NL_SETN,41, "can't set LP Administrator's audit flag")), 1);
		/* We are currently ignoring the error if it can't set the
		   audit id to the LP Administrator.  The audit id will be
		   set to the audit id of the calling process */
		(void) setaudid(s_adm->pw_audid);
	}
#endif
#ifdef SecureWare
        if((ISSECURE) ? (lp_set_ids(adm->pw_uid,adm->pw_gid) == -1) :
			(setresgid(adm->pw_gid,adm->pw_gid,0) == -1
				|| setresuid(adm->pw_uid,adm->pw_uid,0) == -1))
#else
	if(setresgid(adm->pw_gid,adm->pw_gid,0) == -1
		|| setresuid(adm->pw_uid,adm->pw_uid,0) == -1)
#endif
		fatal((catgets(nlmsg_fd,NL_SETN,42, "can't set user id to LP Administrator's user id")), 1);

	/* set up pipe for child -> parent communication */
	if (pipe(pfd) == -1)
	    fatal((catgets(nlmsg_fd,NL_SETN,70,"pipe failed to connect")),1);
		
	/* Fork here so that the parent can return to free
	   the calling process */

	if((i = fork()) < (pid_t)0)
		fatal((catgets(nlmsg_fd,NL_SETN,43, "can't fork")), 1);
	else if(i > (pid_t)0)
	{	/* parent */
		if (close(pfd[1]) < 0)		/* parent closes write pipe */
	    		fatal((catgets(nlmsg_fd,NL_SETN,71,"close of write pipe failed")),1);

		if ((readsize = read(pfd[0],&readbuf, sizeof(readbuf))) <= 0)
			/* will be 0 if child dies, -1 if other error */
	    		fatal((catgets(nlmsg_fd,NL_SETN,72,"scheduler could not be started")),1);

		close(pfd[0]);		/* close read pipe */

		switch(readbuf){
		case SCHED_ACTIVE: fatal((catgets(nlmsg_fd,NL_SETN,73,"scheduler was already active")),1);
		case SCHED_RUNNING: fprintf(stdout,"%s\n",catgets(nlmsg_fd,NL_SETN,74,"scheduler is running"));
			  exit(0);
		default:  fatal((catgets(nlmsg_fd,NL_SETN,72,"scheduler could not be started")),1);
		} /* switch */
	}

	/* child continues */

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, SIG_IGN);

	f_clean = cleanup;
	pgrp = setpgrp();
#ifdef SecureWare
	if(ISSECURE)
            lp_set_mask(000);
	else
	    umask(000);
#else
	umask(000);
#endif

	if (close(pfd[0]) == -1)	/* child closes read pipe */
		/* this exit will unblock the parent's read by sending EOF  */
		/* through the write pipe        */
	    	fatal((catgets(nlmsg_fd,NL_SETN,71,"close of write pipe failed")),1);

	/* Lock the scheduler lock in case lpsched is invoked again */

	if((running = onelock(getpid(), "tmplock", SCHEDLOCK)) > 0)
	{	/* Scheduler already active */
		writebuf = SCHED_ACTIVE;
		writesize = write(pfd[1],&writebuf,sizeof(writebuf));
		exit(0);
	}
	else if (running < 0)
	{	/* Scheduler could not be started */
		writebuf = SCHED_FAULT;
		writesize = write(pfd[1],&writebuf,sizeof(writebuf));
		exit(1);
	}
	/* child becomes the master scheduler */
	writebuf = SCHED_RUNNING;
	writesize = write(pfd[1],&writebuf,sizeof(writebuf));
	close(pfd[1]);		/* close write pipe */

	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen(ERRLOG, "a", stderr);
#ifdef SecureWare
	if(ISSECURE)
            lp_change_mode(SPOOL, ERRLOG, 0644, ADMIN, "error log for printer");
	else
	    chmod(ERRLOG, 0644);
#else
	chmod(ERRLOG, 0644);
#endif
} /* startup */


/*
 * waitfor(p, process_id) -- wait for termination of the process
 *	associated with printer p.
 *	In case it has not exited, it will be killed with SIGTERM.
 *	While waiting for this specific pid, the scheduler may learn of the
 *	deaths of processes associated with other printers.  In such cases,
 *	the process id field of the appropriate printer structure will be
 *	zeroed so that the scheduler doesn't make the mistake of trying to wait
 *	for a process more than once.
 */

waitfor(p, process_id)
struct dest *p;
int	process_id;
{
	int ppid, status;
	struct dest *p1;
	extern struct dest printer;
	int ret;

	if(process_id == -1 || p->d_pid == process_id){
		if((ppid = p->d_pid) == 0) return 0;

		if(kill(ppid, SIGTERM)== -1) {
			if(errno == ESRCH) 
				p->d_pid = 0;
#ifndef NLS
			fprintf(stderr,"kill: invalid pid %d using %s\n", p->d_pid,p->d_dname);
#else NLS
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,44, "kill: invalid pid %d using %s\n")), p->d_pid,p->d_dname);
#endif NLS
			fflush(stderr);
			ppid = -1;
		}
		while((ret=wait(&status)) != ppid) {
			if(ret == -1 && errno == ECHILD) {
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,45, "unexpected wait return\n")));
				fflush(stderr);
				return;
			}
			for(p1 = printer.d_tnext; p1 != &printer;
			     p1 = p1->d_tnext)
				if(p1->d_pid == ret) {
					p1->d_pid = 0;
					break;
				}
		}
		p->d_pid = 0;
	}
} /* waitfor */


/*
 * wrtmail(user, msg) -- write msg to user's tty if logged in.
 *	If not, and user hasn't explicitly requested mail, then
 *	send msg to user's mailbox.
 */

wrtmail(user, msg)
char *user;
char *msg;
{
#ifdef REMOTE
	char	localhost[32];
#endif REMOTE

	if(wrt) {
#ifdef REMOTE
		gethostname(localhost, sizeof(localhost));
		if (! strcmp(localhost, ohostname)){
			if(! wrtmsg(user, "local", msg))
				sendmail(user, ohostname, msg);
		}else{
			if(! wrtremote(user, ohostname, msg))
				sendmail(user, ohostname, msg);
		}
#else REMOTE
		if(! wrtmsg(user, msg))
			sendmail(user, msg);
#endif REMOTE
	}
	else
#ifdef REMOTE
		sendmail(user, ohostname, msg);
#else REMOTE
		sendmail(user, msg);
#endif REMOTE
} /* wrtmail */

/* Our own system...gleaned and modified from libc */

int
system(s)
char	*s;
{
	int	status, local_pid, w;
	extern struct dest printer;

	if((local_pid = fork()) == 0) {
		(void) execl("/bin/sh", "sh", "-c", s, 0);
		_exit(127);
	}
	while((w = wait(&status)) != local_pid && w != -1) {
		struct dest *p1;
		for(p1 = printer.d_tnext; p1 != &printer;
		     p1 = p1->d_tnext)
			if(p1->d_pid == w) {
				p1->d_pid = 0;
				break;
			}
	}
	return((w == -1)? w: status);
} /* system */


int
dump()
{
	struct dest *d;
	struct destlist *l;
	struct outlist *o;
	struct prilist *pl;
	char p[30];

	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,46, "\tFIFO: Destinations --\n")));
	FORALLD(d) {
#ifndef NLS
		fprintf(stderr, "\tFIFO: %s: %c %s %s %s\n",
			d->d_dname,
			(d->d_status & D_PRINTER) ? 'p' : 'c',
			(d->d_status & D_ENABLED) ? "enabled" : "!enabled",
			(d->d_status & D_BUSY) ? "busy" : "!busy",
			d->d_device);
#else NLS
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,47, "\tFIFO: %s: %s %s %s %s\n")),
			d->d_dname,
			(d->d_status & D_PRINTER) ?
				(catgets(nlmsg_fd,NL_SETN,48, "p" ))
				: (catgets(nlmsg_fd,NL_SETN,49, "c")),
			(d->d_status & D_ENABLED) ?
				(catgets(nlmsg_fd,NL_SETN,50, "enabled")) 
				: (catgets(nlmsg_fd,NL_SETN,51, "not enabled")),
			(d->d_status & D_BUSY) ?
				(catgets(nlmsg_fd,NL_SETN,52, "busy")) 
				: (catgets(nlmsg_fd,NL_SETN,53, "not busy")),
			d->d_device);
#endif NLS
		for(pl=(d->d_priority)->pl_next; pl != d->d_priority; pl=pl->pl_next){
			for(o=(pl->pl_output)->ol_next; o != pl->pl_output; o=o->ol_next) {
				if(o->ol_print == NULL)
				    sprintf(p, (catgets(nlmsg_fd,NL_SETN,54, "not printing")));
				else
				    sprintf(p, (catgets(nlmsg_fd,NL_SETN,55, "printing on %s")), (o->ol_print)->d_dname);
#ifdef REMOTE
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,56, "\tFIFO: \t%d: %d %s %s time:%d %s host %s\n")), 
				pl->priority,
				o->ol_seqno,
				o->ol_name,
				p,
				o->ol_time,
				(o->ol_ob3) ?
				(catgets(nlmsg_fd,NL_SETN,57, "seqno(3)"))
				: (catgets(nlmsg_fd,NL_SETN,58, "seqno(4)")),
				o->ol_host);
#else REMOTE
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,59, "\tFIFO:\t%d: %d %s %s time:%d\n")),
				pl->priority, o->ol_seqno, o->ol_name, p, o->ol_time);
#endif REMOTE
			}
		}
	}
	fflush(stderr);
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,60, "\tFIFO: Printers --\n")));
	FORALLP(d) {
		if(d->d_print == NULL)
			sprintf(p, (catgets(nlmsg_fd,NL_SETN,61, "pid and id unknown")));
		else
#ifndef NLS
			sprintf(p, "pid=%d, id=%s-%d", d->d_pid,
			((d->d_print)->ol_dest)->d_dname, (d->d_print)->ol_seqno);
		fprintf(stderr, "\tFIFO: \t%s %s\n", d->d_dname, p);
#else NLS
			sprintf(p, (catgets(nlmsg_fd,NL_SETN,62, "pid=%d, id=%s-%d")), d->d_pid,
			((d->d_print)->ol_dest)->d_dname, (d->d_print)->ol_seqno);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,63, "\tFIFO: \t%s %s\n")), d->d_dname, p);
#endif NLS
		for(l=(d->d_class)->dl_next; l != d->d_class; l=l->dl_next) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,64, "\tFIFO: \t\t%s\n")), (l->dl_dest)->d_dname);
		}
	}
	fflush(stderr);
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,65, "\tFIFO: Classes --\n")));
	FORALLC(d) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,66, "\tFIFO: \t%s\n")), d->d_dname);
		for(l=(d->d_class)->dl_next; l != d->d_class; l=l->dl_next) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,64, "\tFIFO: \t\t%s\n")), (l->dl_dest)->d_dname);
		}
	}
	fflush(stderr);
} /* dump */

/*	onelock(pid,tempfile,name) makes lock a name
	on behalf of pid.  Tempfile must be in the same
	file system as name.  This is not a generic 
	routine.  It is specific for creating SCHEDLOCK */

onelock(local_pid, tempfile, name)
int local_pid;
char *tempfile;
char *name;
{
	int fd;

	if((fd = open(tempfile, O_WRONLY|O_CREAT|O_EXCL, 0666)) < 0)
		return(-1);	/* fatal, could not create tempfile */
	write(fd, (char *) &local_pid, sizeof(int));
	close(fd);
	if(link(tempfile , name) < 0) 	/* scheduler already running or dead */
	{
        	if(enqueue(F_NOOP, "") == 0)  /* scheduler is already running */
		{
			unlink(tempfile);
			return(1);
		}
        	else 		/* scheduler is not running */
		{		/* but housecleaning is required */
				/* in order to restart */
			(void) unlink(SCHEDLOCK);
			(void) unlink(FIFO);
			if(link(tempfile , name) < 0)	/* retry */
			{
				unlink(tmpfile);
				fatal((catgets(nlmsg_fd,NL_SETN,75, "Can't access SCHEDLOCK")), 1);
			}
		}
	}
	unlink(tempfile);
	return(0);	/* this child becomes the master scheduler */
} /* onelock */


int
options(argc, argv)
int argc;
char *argv[];
{
	int j;

	if(argc > 3) {
		printf((catgets(nlmsg_fd,NL_SETN,68, "usage: %s [-v] [-a]\n")), argv[0]);
		exit(1);
	}

	for(j = 1; j < argc; j++) {
		if(argv[j][0] == '-'){
			switch(argv[j][1]){
			    case 'v':
				vflag = TRUE;
				break;
			    case 'a':
				aflag = TRUE;
				break;
			    default:
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,69, "unknown option \"%s\"")), argv[j]);
				fatal(errmsg,1);
			}
		}else{
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,69, "unknown option \"%s\"")), argv[j]);
			fatal(errmsg,1);
		}

	}
} /* options */

int
analysis_out(dest, seqno, host)
char *dest;
int seqno;
char *host;
{
	int errcode;
	struct outq o;
	char file_name[sizeof(SPOOL)+sizeof(LPANA)+2];
	FILE *fp_ana;
	time_t date_now;

	time(&date_now);
#ifdef REMOTE
	errcode = getoidhost(&o, dest, seqno, host);
#else REMOTE
	errcode = getoid(&o, dest, seqno);
#endif REMOTE

	if (errcode == 0)  /* do not log data if EOF is encountered */
	{
	    sprintf(file_name, "%s/%s", SPOOL, LPANA);
	    fp_ana = lockf_open(file_name, "r+", TRUE);

	    fseek(fp_ana, 0L, 2);
	    fprintf(fp_ana, "%s-%d %ld %ld %ld %ld\n", dest, seqno, o.o_size,
			o.o_date ,o.o_pdate, date_now);
	    fclose(fp_ana);
	}
} /* analysis_out */

/*
 * This function checks if the file 'datafilename' exists in REQUEST/newdst
 * directory. If it exists, this function modifies datafilename to use 
 * newseqno, rather than (old)seqno. Thus the file datafilename will then be
 * unique in REQUEST/newdst even in case of a 'wraparound' of seq no.
 * 
 * 'namechanged' forces file name to be changed even if it is not present.
 * 
 * Note that this function operates only on the file NAME and NOT on
 * the file.
 * 
 * Input parameters         : newdst, seqno, newseqno, namechanged
 * Input/Output parameters  : datafilename
 * Return values            : -1 on failure, 0 on success
 *                          : (note that not having to change name is a
 *                          : success)
 */


int
check_name (newdst, seqno, newseqno, datafilename, namechanged)
char *newdst;
int  seqno, newseqno;
char *datafilename;
int  namechanged;
{
	char dname[FILEMAX];
	int  change_name = 0;

	sprintf(dname, "%s/%s/%s", REQUEST, newdst, datafilename);

	if (namechanged)
	   change_name = 1;
	else if (GET_ACCESS (dname, 0) != -1) 
	   change_name = 1;

	if (change_name) {
	   if (get_new_name (seqno, newseqno, datafilename) == -1) {
		return(-1);
	   }
	}
	return 0;
}

/*
 * This function expects arg <filename> to point to a string of the form
 *      <p><n><s> where 
 *                <p> & <s> are (sub) strings of chars
 *                <n> is a string of digits representing arg <seqno>
 * For such a string, this function replaces the substring <n> with the
 * string of digits representing the arg <nseqno>
 *
 * Eg : if <seqno> = 9876, <filename> is expected to be of the form
 *      xxx9876yyyyyyy.  Now if <nseqno> is 9877, at the end of this
 *      function, <filename> contains xxx9877yyyyyyy.
 *
 * Note : 1. <seqno> and <nseqno> need not have same number of digits
 *        2. Only first occurence of substring <n> in <filename> is 
 *           considered.
 */

int
get_new_name (seqno, nseqno, filename)
int seqno, nseqno;
char *filename;
{
    char buf[8], nbuf[8];
    char oldname[FILEMAX];
    int i, len;
    int j, nlen, offset;
    char *tp;

    strcpy (oldname, filename);
    sprintf (buf, "%d", seqno);
    sprintf (nbuf, "%d", nseqno);

    /* 
     * get # of digits in seqno and nseqno
     */
    len = strlen (buf); 
    nlen = strlen (nbuf);

    /*
     * Get offset of 'seqno' in oldname
     */

    tp = strstr (oldname, buf);
    if (len == 0 || tp == NULL) {
       return (-1);			/* filename not in reqd format ? */
    }
    offset = tp - oldname;

    /*
     * Get prefix part from oldname
     */
    for (i = 0; i < offset; i++) 
	filename[i] = oldname[i];

    /*
     * Append new string of digits
     */
    for (i = 0; i < nlen; i++)
	filename[offset + i] = nbuf[i];

    /*
     * Get suffix part from oldname
     */
    for (i = (offset + len), j = (offset + nlen); oldname[i]; i++, j++)
	filename[j] = oldname[i];
    filename[j] = '\0';

    return (0);
}
