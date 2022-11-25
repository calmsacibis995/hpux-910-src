/* $Revision: 66.2.1.1 $ */
/* cancel id ... printer ...  --  cancel output requests */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 2	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include	"lp.h"

int interrupt = FALSE;
char errmsg[512];
#ifdef REMOTE
int	iflag=FALSE;	/* inhibit remote cancel */
char	syscommand[TMPLEN], option[TMPLEN];
char	destprinter[TMPLEN];
struct rem_queue_struct {
	char printer[DESTMAX];
	int seqno;
} rem_requests[MAXREQUESTS];
int rem_index;
#endif REMOTE

main(argc, argv)
int argc;
char *argv[];
{
	char dest[DESTMAX + 1], *arg;
	int seqno, i;
#ifdef REMOTE
	int	temp, aeuoption;
	char	*optionprinter;
	char	*username;
#endif REMOTE
    /* Ensure SIGCLD is always SIG_DFL. This is an NTT hotsite
       fix. One should also remove the while loops for the wait().
       Due to time constraints this will have to be done at a 
       later time. */

        signal(SIGCLD, SIG_DFL);

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
#ifdef NLS
	nlmsg_fd = catopen("lp");
#endif NLS

	startup(argv[0]);

	if(argc == 1) {
#ifdef REMOTE
		usage();
#else
		printf((catgets(nlmsg_fd,NL_SETN,1, "usage: cancel id ... printer ...\n")));
#endif REMOTE
		exit(0);
	}
#ifdef REMOTE
	username = getname();
#endif REMOTE

#ifdef REMOTE
/*
	Check to see if the a, e, u or i option was specified.
	If a, e, or u was specified, it means that the printer
	specification is used by the a,e or u option, not to
	cancel a request on that printer.

	If a, e, or u was specified, the printer specification
	is also used by the a,e or u option.

	If i was specified, it means cancel operations will not
	go to remote machines.
*/
	aeuoption = 0;
	optionprinter = "";
	for(i = 1; i < argc; i++) {
		arg = argv[i];
		if (*arg == '-'){
			temp = *(++arg);
			switch (temp) {
			case 'a':	/* check for the a option */
			case 'e':	/* check for the e option */
			case 'u':	/* check for the u option */
				aeuoption = 1;
				break;
			case 'i':	/* inhibit remote status */
				iflag = TRUE;
				break;
			}
		}else{
			if ( isprinter(arg) || isclass(arg) )
				optionprinter = arg;
		}
	}
#endif REMOTE
	for(i = 1; i < argc; i++) {
		arg = argv[i];
#ifdef REMOTE
		if (*arg == '-'){
			temp = *(++arg);
			switch (temp) {
			case 'a':	/* remove everything a user has */
				aoption(optionprinter, username);
				break;
			case 'e':	/* empty the print queue */
				arg++;
				eoption(optionprinter);
				break;
			case 'u':	/* remove everything the specified */
					/* user has spoolled */
				arg++;
				uoption(optionprinter, arg);
				break;
			default:	/* bad option */
				break;
				}
			}
			else if(isprinter(arg)){
				if (aeuoption == 0)
					restart(arg);
			}else if(isrequest(arg, dest, &seqno))
				cancel(dest, seqno);
			else if(isclass(arg)){
				if (aeuoption == 0) {
					sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,2, "cannot restart class")));
					fatal(errmsg, 0);
				}

			}else{
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,3, "\"%s\" is not a request id or a printer")), arg);
				fatal(errmsg, 0);
		}
#else
			
		if(isprinter(arg))
			restart(arg);
		else if(isrequest(arg, dest, &seqno))
			cancel(dest, seqno);
		else {
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,3, "\"%s\" is not a request id or a printer")), arg);
			fatal(errmsg, 0);
		}
#endif REMOTE
	}
#ifdef REMOTE
	if (rem_index && !iflag)
		do_remote();
#endif REMOTE
	exit(0);
/* NOTREACHED */
}

restart(printer)
char *printer;
{
	struct outq o;
	struct pstat p;
	char *l;

	setoent();
	if(getpdest(&p, printer) == EOF) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,4, "printer \"%s\" has disappeared!")), printer);
		fatal(errmsg, 0);
	}
	else if(! (p.p_flags & P_BUSY)) {
#ifdef REMOTE
		if (p.p_rflags & (P_OCI | P_OCM))
			storeup(printer, -1);
		else
		{
#endif 
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,5, "printer \"%s\" was not busy")), printer);
			fatal(errmsg, 0);
#ifdef REMOTE
		}
#endif
	}
	else {
		setsigs();
		if(getoid(&o, p.p_rdest, p.p_seqno) != EOF) {
			o.o_flags |= O_DEL;
#ifdef SecureWare
			if(ISSECURE){
#ifdef REMOTE
                            lp_verify_cancel(SPOOL, REQUEST, o.o_logname,
                                         o.o_dest, o.o_seqno, o.o_host,
                                        (o.o_rflags&O_OB3));
#else
                            lp_verify_cancel(SPOOL, REQUEST, o.o_logname,
                                         o.o_dest, o.o_seqno);
#endif
			}
#endif		
			putoent(&o);
#ifdef REMOTE
			rmreq(p.p_rdest, p.p_seqno, o.o_host, (o.o_rflags & O_OB3));
#else
			rmreq(p.p_rdest, p.p_seqno);
#endif REMOTE
		}
		killit(&p);
#ifdef REMOTE
		sprintf(errmsg, "%s %d %s", o.o_dest, o.o_seqno, o.o_host);
#else
		sprintf(errmsg, "%s %d", o.o_dest, o.o_seqno);
#endif REMOTE
		enqueue(F_CANCEL, errmsg);
		printf((catgets(nlmsg_fd,NL_SETN,6, "request \"%s-%d\" cancelled\n")), o.o_dest, o.o_seqno);
#ifdef SecureWare
		if(ISSECURE)
                    l = (char *) lp_getlname();
                if (((ISSECURE) && (strcmp(l, o.o_logname) != 0)) ||
 		    ((!ISSECURE) && (strcmp((l=getname()), o.o_logname) != 0)))
#else
 		if(strcmp((l=getname()), o.o_logname) != 0)
#endif
                { 
#ifndef NLS
			sprintf(errmsg, "your printer request %s-%d was cancelled by %s.", o.o_dest, o.o_seqno, l);
#else NLS
			sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,7, "your printer request %1$s-%2$d was cancelled by %3$s.")), o.o_dest, o.o_seqno, l);
#endif NLS
#ifdef REMOTE
			sendmail(o.o_logname, o.o_host, errmsg);
#else
			sendmail(o.o_logname, errmsg);
#endif REMOTE
		}
		reset();
	}
	endpent();
	endoent();
}

cancel(dest, seqno)
char *dest;
int seqno;
{
	struct outq o;
	struct pstat p;
	char *l;

	if(getoid(&o, dest, seqno) == EOF) {
#ifdef REMOTE
		if (getpdest(&p, dest) != EOF && (p.p_rflags & (P_OCI | P_OCM)))
			storeup(dest,seqno);
		else
		{
#endif

			sprintf(errmsg,(catgets(nlmsg_fd,NL_SETN,8, "request \"%s-%d\" non-existent")), dest, seqno);
			fatal(errmsg, 0);
#ifdef REMOTE
		}
		endpent();	/* unlock the pstatus file */
#endif
	}
	else {
#ifndef REMOTE
		setsigs();
#ifdef SecureWare
                /*
                 * PORT NOTE: why is an ifdef REMOTE (below) inside
                 * an ifndef REMOTE?
                 */
		if(ISSECURE)
                    lp_verify_cancel(SPOOL, REQUEST, o.o_logname,
					o.o_dest, o.o_seqno);
#endif
		o.o_flags |= O_DEL;
		putoent(&o);
#ifdef REMOTE
		rmreq(dest, seqno, o.o_host, (o.o_rflags & O_OB3));
#else
		rmreq(dest, seqno);
#endif REMOTE
		if(o.o_flags & O_PRINT) {
			if(getpdest(&p, o.o_dev) != EOF)
				killit(&p);
			endpent();
		}
		sprintf(errmsg, "%s %d", dest, seqno);
		enqueue(F_CANCEL, errmsg);
		printf((catgets(nlmsg_fd,NL_SETN,9, "request \"%s-%d\" cancelled\n")), dest, seqno);
#ifdef SecureWare
		if(((ISSECURE) &&
                    (strcmp((l=(char *) lp_getlname()), o.o_logname) != 0))||
		   ((!ISSECURE) &&
		    (strcmp((l=getname()), o.o_logname) != 0))) 
#else
		if(strcmp((l=getname()), o.o_logname) != 0) 
#endif
		{
#ifndef NLS
			sprintf(errmsg, "your printer request %s-%d was cancelled by %s.", o.o_dest, o.o_seqno, l);
#else NLS
			sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,10, "your printer request %1$s-%2$d was cancelled by %3$s.")), o.o_dest, o.o_seqno, l);
#endif NLS
			sendmail(o.o_logname, errmsg);
		}
		reset();
#else REMOTE
		setsigs();
		if (getpdest(&p, o.o_dev) == EOF) {
			p.p_rflags = 0;
		}
		if (p.p_rflags & P_ORC && (!strcmp(getname(),o.o_logname) ||
		    !strcmp(getname(), "root")) || !(p.p_rflags & P_ORC)) {
#if defined(SecureWare) && defined(B1)
			if(ISB1){
                            lp_verify_cancel(SPOOL, REQUEST, o.o_logname,
                              o.o_dest, o.o_seqno, o.o_host, o.o_rflags&O_OB3);
			}
#endif		
			o.o_flags |= O_DEL;
			putoent(&o);
			rmreq(dest, seqno, o.o_host, o.o_rflags & O_OB3);
			if (o.o_flags & O_PRINT) {
				killit(&p);
			}
			endpent();
			sprintf(errmsg, "%s %d %s", dest, seqno, o.o_host);
			enqueue(F_CANCEL, errmsg);
			printf((catgets(nlmsg_fd,NL_SETN,8, "request \"%s-%d\" cancelled\n")), dest,seqno);
			if (strcmp((l=getname()), o.o_logname) != 0) {
#ifndef NLS
				sprintf(errmsg, "your printer request %s-%d was cancelled by %s.", o.o_dest, o.o_seqno, l);
#else NLS
				sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,7, "your printer request %1$s-%2$d was cancelled by %3$s.")), o.o_dest, o.o_seqno, l);
#endif
				sendmail(o.o_logname, o.o_host, errmsg);
			}
			reset();
		}
		else {
#ifdef NLS
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "request \"%s-%d\" not cancelled: not owner\n")), o.o_dest, o.o_seqno);
#else
			fprintf(stderr, "request \"%s-%d\" not cancelled: not owner", o.o_dest, o.o_seqno);
#endif
		}
		endpent();
#endif REMOTE
	}
	endoent();
}

killit(p)
struct pstat *p;
{
	char *strcpy();

	if(p->p_pid != 0)
		kill(-(p->p_pid), SIGTERM);
	p->p_flags &= ~P_BUSY;
#ifdef REMOTE
	p->p_pid = 0;
	p->p_seqno = -1;
#else
	p->p_pid = p->p_seqno = 0;
#endif REMOTE
	strcpy(p->p_rdest, "-");
	putpent(p);
}

setsigs()
{
	int saveint();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, saveint);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, saveint);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, saveint);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, saveint);
}

reset()
{
	int catch();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);
	if(interrupt) {
		cleanup();
		exit(1);
	}
}

startup(name)
char *name;
{
	int catch(), cleanup();
	extern char *f_name;
	extern int (*f_clean)();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

	f_name = name;
	f_clean = cleanup;
	if(chdir(SPOOL) == -1)
		fatal((catgets(nlmsg_fd,NL_SETN,11, "spool directory non-existent")), 1);
}

/* catch -- catch signals */

catch()
{
	int cleanup();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}

saveint()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	interrupt = TRUE;
}

cleanup()
{
	endpent();
	endoent();
}

#ifdef REMOTE
int
aoption(dest, username)
char	*dest;
char	*username;
{
	struct	outq o;
	struct	pstat p;
	int	pid, seqno, status;
	extern	int errno;

	if(!isprinter(dest)){
		if(!isclass(dest)){
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,15, "\"%s\" is not a printer or a class")), dest);
			fatal(errmsg, 0);
			return(1);
		}
	}

	while (getodestuser(&o, dest, username) != EOF) {
		setsigs();
		o.o_flags |= O_DEL;
		putoent(&o);
		endoent();
		seqno = o.o_seqno;
		rmreq(dest, seqno, o.o_host, (o.o_rflags & O_OB3));
		if(o.o_flags & O_PRINT) {
			if(getpdest(&p, o.o_dev) != EOF)
				killit(&p);
			endpent();
		}
		sprintf(errmsg, "%s %d %s", dest, seqno, o.o_host);
		enqueue(F_CANCEL, errmsg);
		printf((catgets(nlmsg_fd,NL_SETN,9, "request \"%s-%d\" cancelled\n")), dest, seqno);
		reset();
	}
	endoent();
	if (iflag)
		return;
	getmpdest(&p, dest);
	endmpent();	/* unlock before execution of remote command */
	if ((p.p_rflags & (P_OCI|P_OCM)) != 0) {
		sprintf(syscommand , "%s/%s/%s",SPOOL,CINTERFACE,dest);
		sprintf(destprinter, "%s",dest);
		sprintf(option     , "-a");
		if( (pid = fork()) == 0) {
			(void) execlp(syscommand, dest, destprinter, option, 0);
		}
		if (pid == -1){
			printf((catgets(nlmsg_fd,NL_SETN,13, "Unable to execute the remote cancel command %s.\n")),syscommand);
			printf((catgets(nlmsg_fd,NL_SETN,14, "Error %d occured.\n")), errno);
		}else{
			while((wait(&status)) != pid)
				;
		}
	}
}

int
eoption(arg)
char	*arg;
{
	struct	outq o;
	struct	pstat p;
	char	*l, *dest;
	int	pid, seqno, status;
	extern	int errno;

	if(!isprinter(arg)){
		if(!isclass(arg)){
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,15, "\"%s\" is not a printer or a class")), arg);
			fatal(errmsg, 0);
			return(1);
		}
	}

	if (((int) getuid()) != 0){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,16, "You must have root capability to use this option\n")));
		fatal(errmsg, 0);
		return(1);
	}

	while (getodest(&o, arg) != EOF) {
		setsigs();
		o.o_flags |= O_DEL;
		putoent(&o);
		endoent();
		seqno = o.o_seqno;
		dest  = o.o_dest;
		rmreq(dest, seqno, o.o_host, (o.o_rflags & O_OB3));
		if(o.o_flags & O_PRINT) {
			if(getpdest(&p, o.o_dev) != EOF)
				killit(&p);
			endpent();
		}
		sprintf(errmsg, "%s %d %s", dest, seqno, o.o_host);
		enqueue(F_CANCEL, errmsg);
		printf((catgets(nlmsg_fd,NL_SETN,9, "request \"%s-%d\" cancelled\n")), dest, seqno);
		if(strcmp((l=getname()), o.o_logname) != 0) {
#ifndef NLS
			sprintf(errmsg, "your printer request %s-%d was cancelled by %s.", o.o_dest, o.o_seqno, l);
#else NLS
			sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,10, "your printer request %1$s-%2$d was cancelled by %3$s.")), o.o_dest, o.o_seqno, l);
#endif NLS
			sendmail(o.o_logname, o.o_host, errmsg);
		}
		reset();
	}
	endoent();
	if (iflag)
		return(0);
	dest = arg;
	getmpdest(&p, dest);
	endmpent();	/* unlock before execution of remote command */
	if ((p.p_rflags & (P_OCI|P_OCM)) != 0) {
		dest  = arg;
		sprintf(syscommand, "%s/%s/%s",SPOOL,CINTERFACE,dest);
		sprintf(destprinter, "%s",dest);
		sprintf(option    , "-e");
		if( (pid = fork()) == 0) {
			(void) execlp(syscommand, dest, destprinter, option, 0);
		}
		if (pid == -1){
			printf((catgets(nlmsg_fd,NL_SETN,13, "Unable to execute the remote cancel command %s.\n")),syscommand);
			printf((catgets(nlmsg_fd,NL_SETN,14, "Error %d occured.\n")), errno);
		}else{
			while((wait(&status)) != pid)
				;
		}
	}
	return(0);
}

uoption(dest, arg)
char	*dest;
char	*arg;
{
	struct	outq o;
	struct	pstat p;
	char	*l;
	int	pid, seqno, status;
	extern	int errno;

	if (((int) getuid()) != 0){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,16, "You must have root capability to use this option\n")));
		fatal(errmsg, 0);
		return(1);
	}

	if(!isprinter(dest)){
		if(!isclass(dest)){
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,15, "\"%s\" is not a printer or a class")), dest);
			fatal(errmsg, 0);
			return(1);
		}
	}

	while (getodestuser(&o, dest, arg) != EOF) {
		setsigs();
		o.o_flags |= O_DEL;
		putoent(&o);
		endoent();
		seqno = o.o_seqno;
		rmreq(dest, seqno, o.o_host, (o.o_rflags & O_OB3));
		if(o.o_flags & O_PRINT) {
			if(getpdest(&p, o.o_dev) != EOF)
				killit(&p);
			endpent();
		}
		sprintf(errmsg, "%s %d %s", dest, seqno, o.o_host);
		enqueue(F_CANCEL, errmsg);
		printf((catgets(nlmsg_fd,NL_SETN,9, "request \"%s-%d\" cancelled\n")), dest, seqno);
		if(strcmp((l=getname()), o.o_logname) != 0) {
#ifndef NLS
			sprintf(errmsg, "your printer request %s-%d was cancelled by %s.", o.o_dest, o.o_seqno, l);
#else NLS
			sprintmsg(errmsg, (catgets(nlmsg_fd,NL_SETN,10, "your printer request %1$s-%2$d was cancelled by %3$s.")), o.o_dest, o.o_seqno, l);
#endif NLS
			sendmail(o.o_logname, o.o_host, errmsg);
		}
		reset();
	}
	endoent();
	if (iflag)
		return(0);
	getmpdest(&p, dest);
	endmpent();	/* unlock before execution of remote command */
	if ((p.p_rflags & (P_OCI|P_OCM)) != 0) {
		sprintf(syscommand, "%s/%s/%s",SPOOL,CINTERFACE,dest);
		sprintf(destprinter, "%s",dest);
		sprintf(option    , "-u%s",arg);
		if( (pid = fork()) == 0) {
			(void) execlp(syscommand, dest, destprinter, option, 0);
		}
		if (pid == -1){
			printf((catgets(nlmsg_fd,NL_SETN,13, "Unable to execute the remote cancel command %s.\n")),syscommand);
			printf((catgets(nlmsg_fd,NL_SETN,14, "Error %d occured.\n")), errno);
		}else{
			while((wait(&status)) != pid)
				;
		}
	}
	return(0);
}

usage()
{
	printf((catgets(nlmsg_fd,NL_SETN,22, "usage: cancel id ... printer ... -a -e -uuser\n")));
	printf("\n");
	printf((catgets(nlmsg_fd,NL_SETN,23, "requests may be canceled based on the following options\n")));
	printf((catgets(nlmsg_fd,NL_SETN,24, "\tid      - request id\n")));
	printf((catgets(nlmsg_fd,NL_SETN,25, "\tprinter - printer to cancel request on\n")));
	printf((catgets(nlmsg_fd,NL_SETN,26, "\t-a      - all requests for the user on the specified printer\n")));
	printf((catgets(nlmsg_fd,NL_SETN,27, "\t-e      - everything in the printer queue\n")));
	printf((catgets(nlmsg_fd,NL_SETN,31, "\t-i      - cancel only local requests\n")));
	printf((catgets(nlmsg_fd,NL_SETN,28, "\t-uuser  - the specified user(s) on the specified printer\n")));
}

storeup(printer, seqno)
char *printer;
int seqno;
{
	if (rem_index < MAXREQUESTS) {
		strncpy(rem_requests[rem_index].printer, printer, DESTMAX);
		rem_requests[rem_index].printer[DESTMAX-1] = 0;
		rem_requests[rem_index++].seqno = seqno;
	}
	else if (seqno != -1)
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,29, "Maximum number of remote requests exceeded. Request \"%s-%d\" not cancelled.\n")), printer, seqno);
	else
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,30, "Maximum number of remote requests exceeded. Request \"%s\" not cancelled.\n")), printer);
}

do_remote()
{
	int i, j, start, end, pid, status;
	char *name;
	char *args[MAXREQUESTS+1];
	int arg_index;
	int comp();
	extern	char	*malloc();

	qsort(rem_requests, rem_index, sizeof(rem_requests[0]), comp);
	i = 0;
	while (i < rem_index) {
		start = i;
		name = rem_requests[i].printer;
		for (i++ ;i < rem_index && !strcmp(name, rem_requests[i].printer); i++);
		end = i - 1;
		arg_index = 1;
		for (j=start; j<=end; j++) {
			if (rem_requests[j].seqno != -1) {
				if ((args[arg_index] = malloc(DESTMAX + 10)) != NULL )
					sprintf(args[arg_index++], "%s-%d",
					 	rem_requests[j].printer,
					 	rem_requests[j].seqno);
			}
			else
				args[arg_index++] = rem_requests[j].printer;
		}
		sprintf(syscommand, "%s/%s/%s", SPOOL, CINTERFACE, 
			rem_requests[start].printer);
		args[0] = rem_requests[start].printer;
		args[arg_index] = NULL;
		if ((pid = fork()) == 0) {
			execvp(syscommand, args);
			exit(1);
		}
		if (pid == -1) {
			printf((catgets(nlmsg_fd,NL_SETN,13, "Unable to execute the remote cancel command %s.\n")), syscommand);
			printf((catgets(nlmsg_fd,NL_SETN,14, "Error %d occured.\n")), errno);
		}
		else {
			while ((wait(&status)) != pid)
				;
		}
	}
}

comp(a,b)
struct rem_queue_struct *a, *b;
{
	int i;

	i = strcmp(a->printer, b->printer);
	if (i != 0)
		return (i);

	if (a->seqno == b->seqno)
		return 0;
	else if ((unsigned) a->seqno > (unsigned) b->seqno)
		return 1;
	else
		return -1;
}
#endif REMOTE
