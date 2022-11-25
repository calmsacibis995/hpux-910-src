#ifndef lint
static  char rcsid[] = "@(#)rpc.rusersd:	$Revision: 1.34.109.1 $	$Date: 91/11/19 14:11:34 $  ";
#endif

/* rpc.rusersd.c	2.1 86/04/17 NFSSRC */ 
/*static  char sccsid[] = "rpc.rusersd.c 1.1 86/02/05 Copyr 1984 Sun Micro";*/
/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <rpc/rpc.h>
#include <utmp.h>
#include <rpcsvc/rusers.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
/*	HPNFS
**	include the tracing stuff
*/
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/ruserd.trace"

/*
**	utmp file, instead of hard-coding it later ...
*/
# define	UTMP	"/etc/utmp"

#define	DIV60(t)	((t+30)/60)    /* x/60 rounded */
#define MAXINT 0x7fffffff
#define min(a,b) ((a) < (b) ? (a) : (b))

struct utmparr utmparr;
struct utmpidlearr utmpidlearr;
int cnt;
int rusers_service();
extern int errno;

#define NLNTH 8			/* sizeof ut_name */

#ifdef	hpux
/*	HPNFS
**	Sun (Berkeley) defines the macro nonuser in utmp.h -- it
**	determines whether or not a utmp entry is a login user or
**	a window; we don't currently have a way to determine that,
**	so we just test to see if the utmp entry corresponds to a
**	USER_PROCESS.  This makes RUSERSPROC_ALLNAMES worthless.
**	Actually, with ALLNAMES, the user will get non-USER_PROCESS
**	utmp entries -- which could be strange (or not desired).
*/
# define	nonuser(x)	(x.ut_type != USER_PROCESS)
#endif	hpux
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

/*
**	Declare Globals for check_exit code
*/
char	LogFile[64];		/* log file: use it instead of console	*/
long	Exitopt=600l;		/* # of seconds to wait between checks	*/
int	My_prog=RUSERSPROG,		/* the program number	*/
	My_vers=RUSERSVERS_ORIG,	/* the version number	*/
	My_prot=IPPROTO_UDP,		/* the protocol number	*/
	My_port;		/* the port number, filled in later	*/

#ifdef	BFA
/*	HPNFS	jad	87.07.02
**	handle	--	added to get BFA coverage after killing server;
**		the explicit exit() will be modified by BFA to write the
**		BFA data and close the BFA database file.
*/
handle(sig)
int sig;
{
	exit(sig);
}

write_BFAdbase()
{
        _UpdateBFA();
}
#endif	BFA

main(argc,argv)
int	argc;
char	**argv;
{
	register SVCXPRT * transp;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in), num_fds;

	STARTTRACE(TRACEFILE);
#ifdef	TRACEON
	(void) freopen(TRACEFILE, "a+", stderr);
#endif	TRACEON
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rpc.ruserd",0);
#endif NLS
#ifdef	BFA
	/*	HPNFS	jad	87.07.02
	**	catch all fatal signals and exit explicitly, so the BFA
	**	numbers get updated.  Otherwise we get no BFA coverage!
	*/
	(void) signal(SIGHUP, handle);
	(void) signal(SIGINT, handle);
	(void) signal(SIGQUIT, handle);
	(void) signal(SIGTERM, handle);
	TRACE("main signals set up for HUP, INT, QUIT, TERM");

        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);
#endif	BFA
	/*
	**	call argparse to parse the options and set Exitopt
	*/
	LogFile[0] = '\0';
	TRACE("main calling argparse ...");
	argparse(argc,argv);
	TRACE2("main LogFile = %s", LogFile);
	startlog(*argv,LogFile);
	TRACE2("main Exitopt = %d", Exitopt);

	if (getsockname(0, &addr, &len) != 0) {
		TRACE("main getsockname failed");
		log_perror(catgets(nlmsg_fd,NL_SETN,1, "rusersd: getsockname"));
		perror((catgets(nlmsg_fd,NL_SETN,1, "rusersd: getsockname")));
		exit(1);
	} else {
		My_port = addr.sin_port;
		TRACE2("main found My_port = %d", My_port);
	}
	if ((transp = svcudp_create(0)) == NULL) {
		TRACE("main svcudp_create failed");
		logmsg(catgets(nlmsg_fd,NL_SETN,2, "svc_rpc_udp_create: error\n"));
		exit(1);
	}
	if (!svc_register(transp, RUSERSPROG, RUSERSVERS_ORIG,
	    rusers_service,0)) {
		TRACE("main svc_register(ORIG) failed");
		logmsg(catgets(nlmsg_fd,NL_SETN,3, "svc_rpc_register: error\n"));
		exit(1);
	}
	if (!svc_register(transp, RUSERSPROG, RUSERSVERS_IDLE,
	    rusers_service,0)) {
		TRACE("main svc_register(IDLE) failed");
		logmsg(catgets(nlmsg_fd,NL_SETN,4, "svc_rpc_register: error\n"));
		exit(1);
	}

#ifdef NEW_SVC_RUN
	num_fds = getnumfds();
	TRACE2("main about to call svc_run_ms, num_fds = %d",num_fds);
	svc_run_ms(num_fds);		/* never returns */
#else /* NEW_SVC_RUN */
	TRACE("main about to call svc_run");
	svc_run();			/* never returns */
#endif /* NEW_SVC_RUN */

	TRACE("main svc_run returned!  Die!");
	logmsg(catgets(nlmsg_fd,NL_SETN,5, "run_svc_rpc should never return\n"));
	exit(1);
}

getutmp(all, idle)
	int all;		/* give all listings? */
	int idle;		/* get idle time? */
{
	struct utmp buf, **p;
	struct utmpidle **q, *console = NULL;
	int minidle;
	FILE *fp;
	char name[NLNTH];
	
	TRACE3("getutmp SOP, all = %d, idle = %d", all, idle);
	cnt = 0;
	if ((fp = fopen(UTMP, "r")) == NULL) {
		TRACE("getutmp can't open UTMP file, abort");
		log_perror(UTMP);
		exit(1);
	}
	p = utmparr.uta_arr;
	q = utmpidlearr.uia_arr;
	while (fread(&buf, sizeof(buf), 1, fp) == 1) {
		TRACE("getutmp got a buf ...");
		if (buf.ut_line[0] == 0 || buf.ut_name[0] == 0)
			continue;
		TRACE("getutmp with a line and a name ...");
		/* 
		 * if tty[pqr]? and not remote, then skip it
		 */
		if (!all && nonuser(buf))
			continue;
		TRACE("getutmp not a nonuser ...");
		/* 
		 * need to free this
		 * oh sure, just say that and it's all OK.  idiots.
		**	see free_*() functions
		 */
		if (idle) {
			*q = (struct utmpidle *)
			    malloc(sizeof(struct utmpidle));
			TRACE2("getutmp malloc'ed *q = 0x%x", *q);
			/*	HPNFS	jad	87.10.20
			**	added test for NULL instead of blindly
			**	scribbling over memory ...
			*/
			if (*q == NULL) {
				logmsg("rusersd: out of memory\n");
				exit(1);
			}
			if (!strcmp(buf.ut_line, "console")) {
				console = *q;
				TRACE2("getutmp found console entry, 0x%x", console);
				strncpy(name, buf.ut_name, NLNTH);
			    }
			memcpy(&((*q)->ui_utmp), &buf, sizeof(buf));
			(*q)->ui_idle = findidle(&buf);
			TRACE5("getutmp buf: %-10s %-10s %.16s; idle %d",
				    buf.ut_line, buf.ut_name,
				    ctime(&buf.ut_time), (*q)->ui_idle);
			q++;
			TRACE2("getutmp q = 0x%x", q);
		}
		else {
			*p = (struct utmp *)malloc(sizeof(struct utmp));
			TRACE2("getutmp malloc'ed *p = 0x%x", *p);
			/*	HPNFS	jad	87.10.20
			**	added test for NULL instead of blindly
			**	scribbling over memory ...
			*/
			if (*p == NULL) {
				logmsg("rusersd: out of memory\n");
				exit(1);
			}
			memcpy(*p, &buf, sizeof(buf));
			TRACE3("getutmp %-10s %-10s", buf.ut_line, buf.ut_name);
			p++;
			TRACE2("getutmp p = 0x%x", p);
		}
		cnt++;
		TRACE2("getutmp cnt = %d", cnt);
	}
	TRACE("getutmp done freading UTMP");
	/* 
	 * if the console and the window pty's are owned by the same
	 * user, take the min of the idle times and associate
	 * it with the console
	 */
	/*
	**	This special case is based upon the assumption that the
	**	use logged in on the console is likely using windows.
	**	It does not take into account that other users may be
	**	using windows, nor that the user logged in to the
	**	console may in fact be logged in separately on another
	**	terminal (it will consider all pty's owned by the same
	**	user to be running on the console!)
	*/
	TRACE3("getutmp before idle&&console with (%d && 0x%x)", idle, console);
	if (idle && console) {
		TRACE("getutmp idle && console condition");
		minidle = MAXINT;
		rewind(fp);
		TRACE("getutmp rewound UTMP, now reading again");
		while (fread(&buf, sizeof(buf), 1, fp) == 1) {
			if (nonuser(buf)
			    && strncmp(buf.ut_name, name, NLNTH) == 0)
				minidle = min(findidle(&buf), minidle);
			TRACE2("getutmp minidle = %d", minidle);
		}
		TRACE2("getutmp out of while fread loop, console->ui_idle 0x%x", &(console->ui_idle));
		console->ui_idle = min(minidle, console->ui_idle);
		TRACE2("getutmp console->ui_idle = %d", console->ui_idle);
	}
	TRACE("getutmp returns normally");
	/*	HPNFS	jad	87.10.20
	**	Sun returns without closing the utmp file pointer.
	**	incredibly silly oversight not caught until now...
	*/
	(void) fclose(fp);
}

rusers_service(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	TRACE("rusers_service SOP");
	switch (rqstp->rq_proc) {
	case 0:
		TRACE("rusers_service case 0");
		if (svc_sendreply(transp, xdr_void, 0)  == FALSE) {
			TRACE("rusers_service can't svc_sendreply");
			logmsg(catgets(nlmsg_fd,NL_SETN,6, "err: rusersd"));
			exit(1);
		    }
		TRACE("rusers_service all is OK, exiting normally");
		break;
	case RUSERSPROC_NUM:
		TRACE("rusers_service case RUSERSPROC_NUM");
		utmparr.uta_arr = (struct utmp **)
		    malloc(MAXUSERS*sizeof(struct utmp *));
		/*	HPNFS	jad	87.10.20
		**	added test for NULL instead of blindly
		**	scribbling over memory ...
		*/
		if (utmparr.uta_arr == NULL) {
			logmsg("rusersd: out of memory\n");
			exit(1);
		}
		getutmp(0, 0);
		utmparr.uta_cnt = cnt;
		TRACE("rusers_service getutmp(0,0) returned, sending reply");
		if (!svc_sendreply(transp, xdr_u_long, &cnt)) {
			TRACE("rusers_service can't svc_sendreply");
			log_perror(catgets(nlmsg_fd,NL_SETN,7, "svc_rpc_send_results"));
		}
		/*	HPNFS	jad	87.10.20
		**	let's free all the malloc'ed junk ...
		*/
		free_utmparr();
		TRACE("rusers_service exiting normally");
		break;
	case RUSERSPROC_NAMES:
	case RUSERSPROC_ALLNAMES:
		TRACE("rusers_service case RUSERSPROC_{NAMES,ALLNAMES}");
		if (rqstp->rq_vers == RUSERSVERS_ORIG) {
			TRACE("rusers_service is RUSERSVERS_ORIG");
			utmparr.uta_arr = (struct utmp **)
			    malloc(MAXUSERS*sizeof(struct utmp *));
			/*	HPNFS	jad	87.10.20
			**	added test for NULL instead of blindly
			**	scribbling over memory ...
			*/
			if (utmparr.uta_arr == NULL) {
				logmsg("rusersd: out of memory\n");
				exit(1);
			}
			getutmp(rqstp->rq_proc == RUSERSPROC_ALLNAMES, 0);
			utmparr.uta_cnt = cnt;
			TRACE("rusers_service getutmp returned, sending reply");
			if (!svc_sendreply(transp, xdr_utmparr, &utmparr)) {
				TRACE("rusers_service can't svc_sendreply");
				log_perror(catgets(nlmsg_fd,NL_SETN,8, "svc_rpc_send_results"));
			}
			/*	HPNFS	jad	87.10.20
			**	let's free all the malloc'ed junk ...
			*/
			free_utmparr();
			TRACE("rusers_service exiting normally");
			break;
		}
		else {
			TRACE("rusers_service not ORIG version");
			utmpidlearr.uia_arr = (struct utmpidle **)
			    malloc(MAXUSERS*sizeof(struct utmpidle *));
			/*	HPNFS	jad	87.10.20
			**	added test for NULL instead of blindly
			**	scribbling over memory ...
			*/
			if (utmpidlearr.uia_arr == NULL) {
				logmsg("rusersd: out of memory\n");
				exit(1);
			}
			getutmp(rqstp->rq_proc == RUSERSPROC_ALLNAMES, 1);
			utmpidlearr.uia_cnt = cnt;
			TRACE("rusers_service getutmp returned, sending reply");
			if (!svc_sendreply(transp, xdr_utmpidlearr,
					    &utmpidlearr)) {
				TRACE("rusers_service can't svc_sendreply");
				log_perror(catgets(nlmsg_fd,NL_SETN,9, "svc_rpc_send_results"));
			}
			/*	HPNFS	jad	87.10.20
			**	let's free all the malloc'ed junk ...
			*/
			free_utmpidlearr();
			TRACE("rusers_service exiting normally");
			break;
		}
	default: 
		TRACE("rusers_service case default");
		svcerr_noproc(transp);
		break;
	}
	/*
	**	see if I should exit now, or hang around until remapped
	*/
	check_exit();
	TRACE("rusers_service check_exit returned");
}

/* find & return number of minutes current tty has been idle */
findidle(up)
	struct utmp *up;
{
	time_t	now;
	struct stat stbuf;
	long lastaction, diff;
	char ttyname[20];

	TRACE("findidle SOP");
	strcpy(ttyname, "/dev/");
	strncat(ttyname, up->ut_line, sizeof(up->ut_line));
	TRACE2("findidle stat(%s)", ttyname);
	(void) stat(ttyname, &stbuf);
	time(&now);
	lastaction = stbuf.st_atime;
	TRACE3("findidle lastaction = %d, now = %d", lastaction, now);
	diff = now - lastaction;
	diff = DIV60(diff);
	if (diff < 0) diff = 0;
	TRACE2("findidle computed diff = %d, returning", diff);
	return(diff);
}


/*	HPNFS	jad	87.10.20
**	free_utmparr	--	free all the malloc'ed data in utmparr
*/
free_utmparr()
{
	int	i;
	struct	utmp	**ut;
	/*
	**	free each of the utmp's in utmparr.uta_arr
	*/
	for (i=0, ut=utmparr.uta_arr; i < utmparr.uta_cnt; i++, ut++) {
		TRACE2("free_utmparr freeing 0x%x", *ut);
		if (*ut)
			(void) free(*ut);
	}
	(void) free(utmparr.uta_arr);
	utmparr.uta_arr = (struct utmp **) NULL;
}


/*	HPNFS	jad	87.10.20
**	free_utmpidlearr--	free all the malloc'ed data in utmpidlearr
*/
free_utmpidlearr()
{
	int	i;
	struct	utmpidle	**ui;
	/*
	**	free each of the utmpidle's in utmpidlearr.uia_arr
	*/
	for (i=0, ui=utmpidlearr.uia_arr; i < utmpidlearr.uia_cnt; i++, ui++) {
		TRACE2("free_utmpidlearr freeing 0x%x", *ui);
		if (*ui)
			(void) free(*ui);
	}
	(void) free(utmpidlearr.uia_arr);
	utmpidlearr.uia_arr = (struct utmpidle **) NULL;
}
