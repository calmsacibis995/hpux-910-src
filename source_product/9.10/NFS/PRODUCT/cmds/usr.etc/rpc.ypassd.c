#ifndef lint
static char	rcsid[] = "@(#)rpc.yppasswdd: $Revision: 1.48.109.4 $	$Date: 93/11/23 09:45:56 $  ";
#endif
/* rpc.yppasswdd.c	2.1 86/04/17 NFSSRC */
/*static char sccsid[]="rpc.yppasswdd.c 1.1 86/02/05 Copyr 1985 Sun Micro";*/
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* YP_CACHE needs to be defined for  YP_UPDATE stuff to work */
#ifndef YP_CACHE
#undef  YP_UPDATE
#endif	/* YP_CACHE */

/* For info on YP_UPDATE, see usr.etc/yp_update.c - prabha */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <pwd.h>
#include <rpcsvc/yppasswd.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/wait.h>
#include <audnetd.h>

#ifdef  YP_UPDATE
#include <netdb.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <sys/time.h>
#include "yp_cache.h"
#include "yp_update.h"
#endif /*  YP_UPDATE */
#define WEEK (24L * 7 * 60 * 60)


#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif /* NLS */

char	*passwd_file = NULL;	/* file in which passwords are found */
char	lockfile[] = "/etc/ptmp";/* lockfile for modifying passwd_file */
int	mflag_index = 0;	/* Execute /usr/etc/yp/ypmake if non-zero  */
bool_t run_ypmake_later = FALSE;/* Start a new ypmake after the last one dies */
int	ypmake_running = 0;	/* flag that a yp_make is running */
int	pid;			/* pid of the child/parent */
char	pwbuf[BUFSIZ];
time_t  when;
time_t  now;
time_t  maxweeks;
time_t  minweeks;
long    a64l();
char    *l64a();
long    time();


/* uflag and deltafiles have to be defined outside of  YP_UPDATE because we
 * need to check for the presence of no-zero deltafiles during normal startup.*/

char	*delta_byname = "/etc/pwd_delta.byna";
bool_t	uflag = FALSE;	 /* new passwd operation */

#ifdef	 YP_UPDATE
#define MAX_CACHE	200
int	cache_count = 0, max_cache = MAX_CACHE;
char	*map1 = "passwd.byname";
char	*map2 = "passwd.byuid";
FILE	*delta_fp;
char	*dflt_servdb = "ypservers";
char	*servdb = NULL;
char	*domain;

server_list	 upd_sl;
ad_file_struct	 adf;
merge_req	 mrg_msg;
yp_update_struct upd_struct;

extern char	*get_default_domain_name();
extern int	update_db();
extern int	send_entry();
extern int	save_n_send_entry();
extern entrylst *make_cache_entry();
extern entrylst *search_ypcachemap();
#endif	/* YP_UPDATE */

extern int	errno;
extern char	*strchr();
extern int	startlog();		/*  The logging routines  */
extern int	logmsg();
struct passwd *make_passwd();

int	boilerplate();
int	Argc;
char	**Argv;
int	silent = TRUE;
char	*invo_name;		/*  The name by which this program was invoked*/
struct sockaddr_in myaddr;	/*  IP addr of this machine */
char	audit_msg[1024];		/*  Place to format auditing info */
void usage();
void catch_sigchild();		/*  signal handler for SIGCLD */
void start_ypmake();		/*  This function is used to start ypmake if
				    changes have been made since the last
				    ypmake */
void unreg_it();		/*  This function is used to handle signals,
				    unregistering rpc.yppasswdd from portmap's
				    list  */
int	logging = FALSE;	/*  TRUE if messages are to be logged to a
				    file  */
#ifdef	BFA
/*	HPNFS	jad	87.07.02
**	handle	--	added to get BFA coverage after killing server;
**		the explicit exit() will be modified by BFA to write the
**		BFA data and close the BFA database file.
*/
handle(sig)
int	sig;
{
	exit(sig);
}

write_BFAdbase()
{
	_UpdateBFA();
}
#endif	/* BFA */

main(argc, argv)
int	argc;
char	**argv;
{
	int	err = 0;
	struct stat stb;
	char	*log_file_name = NULL;	/*  The file for logging error msgs  */
	SVCXPRT * transp;
	int	s, nfds;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rpc.ypassd", 0);
#endif /* NLS */

#ifdef	BFA
	/*	HPNFS	jad	87.07.02
	**	catch all fatal signals and exit explicitly, so the BFA
	**	numbers get updated.  Otherwise we get no BFA coverage!
	**	NOTE:	handlers for SIGHUP, SIGINT and SIGQUIT are
	**		set and reset by changepasswd; only SIGTERM is
	**		guaranteed to hit the handle()r ...
	*/
	(void) signal(SIGHUP,  handle);
	(void) signal(SIGINT,  handle);
	(void) signal(SIGQUIT, handle);
	(void) signal(SIGTERM, handle);

	/* added to get BFA data even as the daemon is running */
	(void) signal(SIGUSR2, write_BFAdbase);
#else	BFA
	/*  Use unreg_it to catch these signals.  We want to unregister
	    rpc.yppasswdd from portmap's list of registered programs if
	    rpc.yppasswdd is killed in any way.  */

	(void) signal(SIGHUP,  SIG_IGN);
	(void) signal(SIGINT,  unreg_it);
	(void) signal(SIGQUIT, unreg_it);
	(void) signal(SIGTERM, unreg_it);
#endif	/* BFA */

	Argc = argc;
	Argv = argv;
	invo_name = *argv;		/*  Save the name of the program  */

	/*  Parse the arguments passed in to rpc.yppasswdd  */
	while (--argc) {
		if (mflag_index)
			break;
		++argv;
		if ((*argv)[0] == '-') {
			switch ((*argv)[1]) {
			case 'l':
				if ((*argv)[2] != '\0'){
				   log_file_name = &((*argv)[2]);
				}else{
				   if ((argc > 1) && (argv[1][0] != '-')){
					log_file_name = argv[1];
					argc--;
					argv++;
				   }
				 }
				break;
			case 'm':
				mflag_index = Argc + 1 - argc;
				break;
#ifdef  YP_UPDATE
			case 's':	/* where to get server names from */
				if ((*argv)[2] != '\0') {
					servdb = *argv;
				} else {
					--argc; 
					++argv;
					servdb = *argv;
				}
				break;

			case 'u':	/* up_date operation ON */
				uflag = TRUE;
				if ((*argv)[2]) { /* get the number */
					max_cache = atoi(&((*argv)[2]));
					if (max_cache < 0) {
						max_cache = 0;
						fprintf(stderr,"Bad Option Specification: %s\n",(*argv));
					}	
					if (max_cache > MAX_CACHE)
						max_cache = MAX_CACHE;
				} else
					max_cache = MAX_CACHE;
				break;
#endif /*  YP_UPDATE */
			case 'v':
				if ((*argv)[2] != '\0') {
					fprintf(stderr,"Option Specification Error: %s\n",(*argv));
					err++;
					break;
				}
				silent = FALSE;
				break;

			default:
				fprintf(stderr,"Unknown option: %s\n",(*argv));
				err++;
				break;
			}
		} else {
			if (passwd_file)
				usage();
			passwd_file = *argv;
		}
	}

	if (err)
		usage();

	/* The delta file is used only with the  YP_UPDATE process. But, if a
	 * non-zero delta file is present, that means that the last new update
	 * type rpc.yppasswdd had quit without making some of the updates. We
	 * must check if a delta file is present. If it's size is > 0 and uflag
	 * is not ON, write error message and exit.
	 */

	err = stat(delta_byname, &stb);
	if (err == 0) { /* a non-zero delta_byname present & no uflag */
		if ((stb.st_size > 0) && (!uflag)) {
			fprintf(stderr, "\n%s is not empty. Try the following:\n\n1) Restart with -u option\n\n\t\tOR\n\n2) Merge changes in by hand, truncate %s &\n\t\trestart rpc.yppasswdd\n\n",
			     			     delta_byname, delta_byname);
			exit (1);
		}
	}
#ifdef YP_UPDATE
#ifdef DEBUG
	else
		fprintf(stderr,"error %d from stat of %s\n",errno, delta_byname);
#endif /* DEBUG */
#endif /* YP_UPDATE */
	if (!passwd_file)
		usage();

	if (log_file_name) {
		if (startlog(invo_name, log_file_name)) {
			fprintf(stderr,
			    (catgets(nlmsg_fd, NL_SETN, 16, "The log file \"%1$s\" cannot be opened.\n%2$s aborted.\n")),
			    log_file_name, invo_name);
			exit(EINVAL);
		}
		logging = TRUE;
	}

	if (access(passwd_file, R_OK) < 0) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 17, 
			    "%1$s:  can't read %2$s; it must be readable.\n"),
			    invo_name, passwd_file);
		else
			fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 17,
			    "%1$s:  can't read %2$s; it must be readable.\n")),
			    invo_name, passwd_file);
		exit(1);
	}

	if (access(passwd_file, W_OK) < 0) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 2,
			    "%1$s:  can't write %2$s; it must be writable.\n"),
			    invo_name, passwd_file);
		else
			fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 2,
			    "%1$s:  can't write %2$s; it must be writable.\n")),
			    invo_name, passwd_file);
		exit(1);
	}

	if ((s = rresvport()) < 0) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 3,
			    "%s:  can't bind to a privileged socket\n"),
			    invo_name);
		else
			fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 3,
			    "%s:  can't bind to a privileged socket\n")),
			    invo_name);
		exit(1);
	}

	transp = svcudp_create(s);
	if (transp == NULL) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 4,
			    "%s:  couldn't create an RPC server\n"),
			    invo_name);
		else
			fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 4,
			    "%s:  couldn't create an RPC server\n")),
			    invo_name);
		exit(1);
	}

	pmap_unset(YPPASSWDPROG, YPPASSWDVERS);

	if (!svc_register(transp, YPPASSWDPROG, YPPASSWDVERS,
	    boilerplate, IPPROTO_UDP)) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 5,
			    "Couldn't register %s with portmap.\n\tIs portmap running?\n"),
			    invo_name);
		else
			fprintf(stderr, (catgets(nlmsg_fd, NL_SETN, 5,
			    "Couldn't register %s with portmap.\n\tIs portmap running?\n")),
			    invo_name);
		exit(1);
	}

	get_myaddress(&myaddr);

	if (silent) {
		if (fork())
			exit(0);

/* HPNFS
 *  Getdtablesize returns the size of the file descriptor table.
 *  In HP-UX we can have up to getnumfds() file descriptors so I'm
 *  assuming that since the intent here is to close all file descriptors
 *  except the socket s, it is OK to close the getnumfds() descriptors.  CHM
 * HPNFS
 */
		 { 
			int	t;
			for (t = getnumfds() - 1; t >= 0; t--)
				if (t != s)
					(void) close(t);
		}
		(void) open("/", O_RDONLY);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		setpgrp();

	}	/*  If silent is TRUE  */

#ifdef  YP_UPDATE
	if (uflag) {
		strcpy((mrg_msg.adf).ascii_file, passwd_file);
		strcpy((mrg_msg.adf).delta_file, delta_byname);
		(mrg_msg.adf).separator = ':';
		strcpy(mrg_msg.domain, domain);
		strcpy(mrg_msg.map, map1);
		mrg_msg.opt_pull = TRUE;	/* default */

		upd_sl.mtime = 0;
		domain = get_default_domain_name();
		upd_sl.domain = domain;

		if (servdb == NULL)
			servdb = dflt_servdb;
		upd_sl.db_name = servdb;

		upd_sl.client  = (CLIENT * )0;
		upd_sl.prognum = YPPROG;	/* ypserv prog */
		upd_sl.version = YPVERS;	/* current version */
		upd_sl.procnum = YPPROC_UPDATE_MAPENTRY;
		upd_sl.se_list = (server_entry * )0;

		upd_struct.dfp = delta_fp = fopen (delta_byname, "r+");
		strcpy(upd_struct.lock_file, lockfile);
		upd_struct.mrg_msgp = &mrg_msg;
		upd_struct.slp = &upd_sl;

		if (delta_fp != NULL) {
			if ((err = init_update(&upd_struct)) != 0) {
				exit (err);
			}
		} else {
			logmsg("cant open %s\n", delta_byname);
			exit(1);
		}
	}
#endif /*  YP_UPDATE */


	/* Handle the death of the child that does ypmake of the passwd maps. */
	(void) signal(SIGCLD, catch_sigchild);

#ifdef NEW_SVC_RUN
	/* wait on the udp socket we have open. Note that if TCP sockets
	 * had been registered, nfds would have to be more because accept
	 * generates more sockets on which you may need to select.
	 */
	svc_run_ms(s+1); /* nfds = socket fd + 1 */
#else
	svc_run();
#endif /* NEW_SVC_RUN */
	if (logging)
		(void) logmsg(catgets(nlmsg_fd, NL_SETN, 6, "svc_run shouldn't have returned"));
}


boilerplate(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT *transp;
{
	int	err = 0;
#ifdef YP_UPDATE
	merge_req	new_mrg_msg;
#endif	/* YP_UPDATE */

	switch (rqstp->rq_proc) {

	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, 0)  &&  logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 7, "NULLPROC couldn't reply to RPC call"));
		break;

	case YPPASSWDPROC_UPDATE:
		changepasswd(rqstp, transp);
		break;

#ifdef  YP_UPDATE

	/*
	 * NOTE: It is common practice to register new proc #s with SUN and
	 *	also up the version number with that. This proc num has not
	 *	been registered with SUN, mainly due to time constraints and
	 *	also because this proc num would be used only for messages
	 *	from local programs on the same HP master. If/When it is
	 *	decided that we need to sell/support this code, we should get
	 *	a new version also for this PROC and move this section into
	 *	another routine, which would be registered as a separate
	 *	routine (other than this routine, boiler_plate) - prabha.
	 */

	case YPUPDATEPROC_MERGE:

		if (!svc_getargs(transp, xdr_merge_req, &new_mrg_msg))
			svcerr_decode(transp);

		/* call update on map1 */
		if (uflag) {
			if (strcmp((new_mrg_msg.adf).ascii_file, passwd_file)) {
				if (strcmp((new_mrg_msg.adf).delta_file, delta_byname)) {
					/* ignore map & domain for now */
					err = update_db(&upd_struct);
					cache_count = 0;
				} else {
					err = YPUPDATE_DELTAFILE; /* wrong delta file name */
				}
			} else {
				err = YPUPDATE_ASCIIFILE; /* wrong ascii file name */
			}

		} else {
			err = YPUPDATE_UNDEFOP;
		}
		if (!svc_sendreply(transp, xdr_int, &err) && logging ) {
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 7, "YPPASSWDPROC_UPDATE couldn't reply to RPC call"));
			}
		break;
#endif /*  YP_UPDATE */

	}
}

changepasswd(rqstp, transp)
struct svc_req *rqstp;
SVCXPRT *transp;
{
	static int	ans = 0;
	int	err, bad_pw = 0, stat_loc, new_pw_len;
	char	*p, *q, 	*pw_name;
	struct passwd *oldpw, *newpw, *getpwnam();
	struct yppasswd yppasswd;
	struct sockaddr_in remoteaddr;
	FILE	 * pwd_fp;

#ifdef  YP_UPDATE
	char	pw_uid[15];
	entrylst * e, *ep;
#endif /*  YP_UPDATE */

	remoteaddr = *svc_getcaller(transp);

	memset(&yppasswd, 0, sizeof(yppasswd));
	if (!svc_getargs(transp, xdr_yppasswd, &yppasswd)) {
		svcerr_decode(transp);
		return;
	}
	/* 
	 * Clean up from previous changepasswd() call
	 */

	while (wait3(&stat_loc, WNOHANG, 0) > 0)
		continue;

	newpw = &yppasswd.newpw;
	pw_name = newpw->pw_name;
	new_pw_len = strlen(newpw->pw_name);

	ans = 1;

	/* prevent people calling yppasswd() from replacing old pwd with
	 * a new one that contains ':0:...\n' giving themselves root
	 * privilages. Also  limit the encrypted pw to  MAXPWLEN  chars.
	 * MAXPWLEN is set to 22 as  follows: (13 for encrypted pw, 9
	 * for pw aging info (1 for a comma, 1 for M, one for m and 5
	 * for weeks since 1970 - this would allow this passwd version
	 * to exist for a few centuries! - see passwd man page- prabha*/

#define MAXPWLEN 22

	q = newpw->pw_passwd;

	if (new_pw_len > MAXPWLEN) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 24, "Encrypted passwd for %s too long. Truncated to MAXPWLEN (22) chars\n"),
			     			     pw_name);
		q[MAXPWLEN] = '\0';
	}

	for ( p = newpw->pw_passwd; (*p != '\0'); p++)
		if ((*p == ':') || (*p == '\n')) {
			bad_pw = 1; 	/* the passwd field should
								   not contain a ':' in it*/
			break;
		}

	if (bad_pw == 1) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 20, "Passwd for %s contains a colon or newline"), newpw->pw_name);
		sprintf(audit_msg, catgets(nlmsg_fd, NL_SETN, 20, "Passwd for %s contains a colon or newline"),
		    newpw->pw_name);
		audit_daemon( NA_SERV_YPPASSWDD, NA_RSLT_FAILURE, 
		    NA_VALI_PASSWD, remoteaddr.sin_addr.s_addr,
		    -1, myaddr.sin_addr.s_addr, -1, NA_STAT_START,
		    NA_MASK_RUSR | NA_MASK_LUSR, audit_msg);
		goto done;
	}

#ifdef YP_UPDATE
	/* value field doesnt matter */
	e = make_cache_entry (pw_name, 0, STICKY);

	/* save only the latest entry in the cache */
	ep = search_ypcachemap(domain, map1, e->key_val.keydat);
	if (ep) {
		oldpw = make_passwd(ep->key_val.valdat.dptr);
	} else {
#endif /* YP_UPDATE */

		if ((pwd_fp = fopen(passwd_file, "r")) == NULL) {
			if (logging)
				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 12, "fopen of %s failed"), passwd_file);
			exit(1);
		} else {
			oldpw = getpwnam(pw_name, pwd_fp);
			close (pwd_fp);
		}
#ifdef YP_UPDATE
	}
#endif /* YP_UPDATE */

	if (oldpw == NULL) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 8, "No passwd for %s"), newpw->pw_name);
		sprintf(audit_msg,
		    catgets(nlmsg_fd, NL_SETN, 21, "No passwd entry for %s"),
		    newpw->pw_name );
		audit_daemon( NA_SERV_YPPASSWDD, NA_RSLT_FAILURE, 
		    NA_VALI_PASSWD, remoteaddr.sin_addr.s_addr, -1,
		    myaddr.sin_addr.s_addr, -1, NA_STAT_START,
		    NA_MASK_RUSR | NA_MASK_LUSR, audit_msg );
		goto done;
	}
	if (oldpw->pw_passwd && *oldpw->pw_passwd && 
	    strcmp(crypt(yppasswd.oldpass, oldpw->pw_passwd),
	    oldpw->pw_passwd) != 0) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 9, "%s:  bad passwd"), newpw->pw_name);
		sprintf(audit_msg,
		    catgets(nlmsg_fd, NL_SETN, 22, "Bad passwd for %s"),
		    newpw->pw_name );
		audit_daemon( NA_SERV_YPPASSWDD, NA_RSLT_FAILURE, 
		    NA_VALI_PASSWD, remoteaddr.sin_addr.s_addr, -1,
		    myaddr.sin_addr.s_addr, -1, NA_STAT_START,
		    NA_MASK_RUSR | NA_MASK_LUSR, audit_msg );
		goto done;
	}

	if (oldpw->pw_age != (char *)NULL)
		sprintf (pwbuf, "%s:%s,%s:%d:%d:%s:%s:%s",
		    oldpw->pw_name,
		    newpw->pw_passwd,
		    oldpw->pw_age,
		    oldpw->pw_uid,
		    oldpw->pw_gid,
		    oldpw->pw_gecos,
		    oldpw->pw_dir,
		    oldpw->pw_shell);
	else
		sprintf (pwbuf, "%s:%s:%d:%d:%s:%s:%s",
		    oldpw->pw_name,
		    newpw->pw_passwd,
		    oldpw->pw_uid,
		    oldpw->pw_gid,
		    oldpw->pw_gecos,
		    oldpw->pw_dir,
		    oldpw->pw_shell);

#ifdef  YP_UPDATE
	if (uflag) {	/* add entry to map(s) & delta file */
		sprintf(pw_uid, "%s", newpw->pw_uid);
		e = make_cache_entry (pw_uid, pwbuf, STICKY);
		strcpy(mrg_msg.map, map2);
		(void) send_entry(&upd_struct, e);

		e = make_cache_entry (pw_name, pwbuf, STICKY);
		strcpy(mrg_msg.map, map1);
		ans = save_n_send_entry (&upd_struct, e);
		if (ans == 0) {
			if (++cache_count >= max_cache) {
				cache_count = 0;
				err = update_db(&upd_struct);
				if (err == 0) {
					call_ypmake(TRUE);
				}
			}
		}
	}

#else /* not  YP_UPDATE */
	strcat(pwbuf,"\n");
	ans = write_one_to_db(pw_name, pwbuf);
	if (ans == 0)
		(void) call_ypmake();

#endif	/*  YP_UPDATE */


done:
	if (!svc_sendreply(transp, xdr_int, &ans)  &&  logging)
		(void) logmsg(catgets(nlmsg_fd, NL_SETN, 13, "Couldn't reply to RPC call\n"));

	/* ADDED BY PRABHA so yppasswd gets freed 04/12/90 */

	if (!svc_freeargs(transp, xdr_yppasswd, &yppasswd) ) {
		/* TRACE("ypfirst:  svc_freeargs error"); */
		(void) logmsg(catgets(nlmsg_fd, NL_SETN, 13, "Couldn't svc_free args.\n"));
	}
}

#ifdef YP_UPDATE
call_ypmake(opt_pull)
bool_t	opt_pull;
#else
call_ypmake()
#endif /* YP_UPDATE */
{
	/*
	 * if the yppasswd daemon is set up with a -m option, do a ypmake on
	 * the passwd file with all options on the command line after the -m.
	 * Handle the death of the child that does ypmake in the signal handler
 	 * catch_sigchild. Pass opt_pull to start_ypmake as is.
	 */
	if (mflag_index) {
		if (!ypmake_running) {
#ifdef YP_UPDATE
			(void) start_ypmake(opt_pull);
#else
			(void) start_ypmake();
#endif /* YP_UPDATE */
		} else {
			run_ypmake_later = TRUE; /* so we can try it later */
			if (logging)
				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 25, "A ypmake is running now. Flag set to run ypmake after the current ypmake dies.\n"));
		}
	}
}


/* The following are defined in yp_update.c too for general use. The following
 * version will be used when YP_UPDATE is not defined - prabha */

#ifndef YP_UPDATE

FILE *
open_lockfile (lock_file)
char	*lock_file;
{
	int	tempfd;
	static FILE    *tempfp;

	(void) umask(0);

	tempfd = open(lock_file, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (tempfd < 0) {
		if (errno == EEXIST) {
			if (logging)
				(void) logmsg("open_lockfile: password file busy - try again");
		} else {
			if (logging)
				(void) logmsg("open_lockfile: error opening %s: errno = %d", lock_file, errno);
		}
		return((FILE * )0);
	}

	if ((tempfp = fdopen(tempfd, "w")) == NULL) {
		if (logging)
			(void) logmsg("open_lockfile: fdopen of %s failed", lock_file);
		close (tempfd);
		return((FILE * )0);
	}
	return (tempfp);
}


replace_asciifile(lock_file, asciifile)
char	*lock_file, *asciifile;
{
	char	cmdbuf[BUFSIZ];
	void (*f1)(), (*f2)(), (*f3)(), (*f4)();
	int	status,cpid,err;

	f1 = signal(SIGHUP,  SIG_IGN);
	f2 = signal(SIGINT,  SIG_IGN);
	f3 = signal(SIGQUIT, SIG_IGN);
	f4 = signal(SIGCLD,  SIG_IGN);

	if ((err = rename(lock_file, asciifile)) < 0) {
		/* the rename may have failed if the two files are
                   on different systems; if so, try a move operation */

		if ((cpid = fork()) < 0) 
			return -1;
 		if (cpid == 0)
			execl("/bin/mv","mv",lock_file,asciifile,0);
			
		waitpid(cpid,&status,0);
		if(status != 0){ 
			if (logging)
				(void) logmsg(
				    (catgets(nlmsg_fd, NL_SETN, 19, "Error renaming %1s to %2s:  errno = %d")), lock_file, asciifile, errno);
			unlink(lock_file);	/* since mv did not succeed */
			err = 1;
			}else{
                         err = 0;
			}
		} else
			err = 0;


	signal(SIGHUP,  f1);
	signal(SIGINT,  f2);
	signal(SIGQUIT, f3);
	signal(SIGCLD,  f4);

	return (err);
}


write_one_to_db (pw_name, pw_buf)
char	*pw_name;
char	*pw_buf;
{
	int	pw_len;
	FILE	*pwd_fp = (FILE * )0;
	FILE	*tempfp = (FILE * )0;
	char	buf[BUFSIZ], *p;

	(void) umask(0);

	tempfp = open_lockfile(lockfile);
	if (tempfp == NULL)
		return(1);

	/* open pw file to read from */
	if ((pwd_fp = fopen(passwd_file, "r")) == NULL) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 12, "fopen of %s failed"), passwd_file);
		fclose(tempfp);
		return(1);
	}

	while (fgets(buf, BUFSIZ, pwd_fp)) {
		p = strchr(buf, ':');

		/* look for a match with pw_name in the
		 * pw_list and replace the coresponding line */

		pw_len = strlen(pw_name);
		/* find & modify the line that has matching login name*/
		if (p && ((p - buf) == pw_len) && 
		    (memcmp(pw_name, buf, pw_len) == 0))
			fprintf(tempfp, "%s", pw_buf);
		else
			fputs(buf, tempfp);
	}

	fclose(pwd_fp);
	fclose(tempfp);

	return(replace_asciifile(lockfile, passwd_file));
}


#endif /* YP_UPDATE */

rresvport()
{
	struct sockaddr_in sin;
	int	s, alport = IPPORT_RESERVED -1;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_DGRAM, 0, 0);
	if (s < 0)
		return (-1);
	for (; ; ) {
		sin.sin_port = htons((u_short)alport);
		if (bind(s, (caddr_t) & sin, sizeof (sin), 0) >= 0)
			return (s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
			log_perror(catgets(nlmsg_fd, NL_SETN, 14, "socket"));
			perror((catgets(nlmsg_fd, NL_SETN, 14, "socket")));
			return (-1);
		}
		(alport)--;
		if (alport == IPPORT_RESERVED / 2) {
			if (logging)
				(void) logmsg(catgets(nlmsg_fd, NL_SETN, 15, "Socket:  all ports in use"));
			return (-1);
		}
	}
}


static char	*
pwskip(p)
register char	*p;
{
	while ( *p && *p != ':' && *p != '\n' )
		++p;
	if ( *p ) 
		*p++ = 0;
	return(p);
}


struct passwd *
make_passwd (p)
register char	*p;
{
	static char	passwd_copy[BUFSIZ];
	char	*q, *pcopy = passwd_copy;
	static struct passwd passwd;

	if (p == NULL)
		return ((struct passwd *)0);

	strcpy(passwd_copy, p, strlen(p));

	if (pcopy) {
		passwd.pw_name = pcopy;
		pcopy = pwskip(pcopy);
		passwd.pw_passwd = pcopy;
		pcopy = pwskip(pcopy);
/* HPNFS
 *  For HPUX, we have to separate password info from aging info.
 *  Also, HPUX doesn't have quotas, hence the passwd struct doesn't
 *  have a quota member as it does on a Sun.
 * HPNFS
 */
		if (q = strchr(passwd.pw_passwd, ',')) {
			*q++ = '\0';
			passwd.pw_age = q;
			if (passwd.pw_age != NULL)
			{
			   when = (long) a64l(passwd.pw_age);
			   maxweeks = when & 077;
			   minweeks = (when >> 6) & 077;
			   when >>= 12;
			   now = time((long *) 0)/WEEK;
			   if (maxweeks == 0 && minweeks <= maxweeks)
			      { 
				passwd.pw_age = (char *) NULL;
			      }
			    else
			     { 
				when = maxweeks + (minweeks << 6) + (now <<12);
				passwd.pw_age = l64a(when);
			     }
			}

		} else {
			passwd.pw_age = (char *)NULL;
		}
		passwd.pw_uid = atoi(pcopy);
		pcopy = pwskip(pcopy);
		passwd.pw_gid = atoi(pcopy);
		passwd.pw_comment = "";
		pcopy = pwskip(pcopy);
		passwd.pw_gecos = pcopy;
		pcopy = pwskip(pcopy);
		passwd.pw_dir = pcopy;
		pcopy = pwskip(pcopy);
		passwd.pw_shell = pcopy;
		while (*pcopy && *pcopy != '\n')
			pcopy++;
		*pcopy = '\0';

		return (&passwd);
	} else
		return((struct passwd *)0);
}


/* get the pw entry for name from the already open stream pwf */
struct passwd *
getpwnam(name, pwf)
char	*name;
FILE *pwf;
{
	int	cnt;
	char	*p;
	static char	line[BUFSIZ+1];

	if (pwf == NULL) {
		if (logging)
			(void) logmsg(catgets(nlmsg_fd, NL_SETN, 12, "getpwnam: NULL pwf"));
		return (NULL);
	}

	cnt = strlen(name);
	while (fgets(line, BUFSIZ + 1, pwf)) {
		p = strchr(line, ':');
		if (p  &&  p - line == cnt  && 
		    strncmp(name, line, cnt) == 0) {
			p = line;
			break;
		}
		p = NULL;
	}
	fclose(pwf);
	return(make_passwd(p));
}


void catch_sigchild (sig)
int	sig;
{
	int	stat_loc;

	ypmake_running = FALSE;

/*
 * wait for the child to cleanup. It may be a good idea to
 * use waitpid instead of wait. To do this, we would have to
 * save the pid of the child and use it in the waitpid call.
 * - prabha
 */

	(void) wait(&stat_loc);

	/* If a ypmake request is pending, do it now. */

	if (run_ypmake_later) {
		utime ("/etc/passwd", 0);
#ifdef  YP_UPDATE
		/* call start_ypmake with opt_pull TRUE. Used iff uflag */
		(void) start_ypmake(TRUE);
#else
		(void) start_ypmake();
#endif /*  YP_UPDATE */
	}

	(void) signal(SIGCLD, catch_sigchild);
}


/* The start_ypmake function is called to start a new ypmake.
 * opt_pull is used iff  YP_UPDATE if defined. */

void
#ifdef YP_UPDATE
start_ypmake(opt_pull)
bool_t opt_pull;
#else
start_ypmake()
#endif /* YP_UPDATE */
{
	char	cmdbuf[BUFSIZ];
	int	i;

	/* start child to do the ypmake */
	pid = fork();

	if ( pid > 0) {	/* still in the parent */
		ypmake_running = TRUE;
		run_ypmake_later = FALSE;
	} else {
		if (pid == 0) { /* in the child, set up default
				 * handlers for SIGCLD & SIGTERM */

			(void) signal(SIGCLD,  SIG_IGN);
			(void) signal(SIGTERM, SIG_DFL);

			strcpy(cmdbuf, "/usr/etc/yp/ypmake");

			for (i = mflag_index; i < Argc; i++) {
				strcat(cmdbuf, " ");
				strcat(cmdbuf, Argv[i]);
			}
#ifdef  YP_UPDATE
			if ((uflag) && (opt_pull))
				strcat (cmdbuf, " OPTPULL=1");
#endif /*  YP_UPDATE */
			(void) system(cmdbuf);
			exit(0);
		} else {	/* pid < 0 - in parent, error from fork */
			ypmake_running = FALSE;
		}
	}
}


/*  The unreg_it function is used to handle signals, unregistering
    rpc.yppasswdd from portmap's list of registered programs  */
void
unreg_it (sig)
int	sig;
{
	(void) pmap_unset (YPPASSWDPROG, YPPASSWDVERS);
#ifdef  YP_UPDATE
	print_all_maps();
#endif /*  YP_UPDATE */
	exit (sig);
}


void
usage ()
{
#ifndef YP_UPDATE
	fprintf (stderr, (catgets (nlmsg_fd, NL_SETN, 1, "Usage:\t%s passwd_file [-l log_file] [-m [arg1 arg2 ...]]\n\n where\tpasswd_file is the full pathname of the ASCII passwd file from which\n\t   the passwd map is built\n\t-m specifies that after file is updated, ypmake should be run with\n\t   the optional arguments; all arguments after -m are passed to ypmake\n")),
	    invo_name);
#else
	(void) logmsg (catgets (nlmsg_fd, NL_SETN, 1, "Usage:\t%s passwd_file -u[num] [-s server_dbname] [-l log_file] [-m [arg1 arg2 ...]]\n\n where\tpasswd_file is the full pathname of the ASCII passwd file from which\n\t   the passwd map is built\n\t-u[num] indicates new passwd operation with automatic merge after num entries are received\n\t-s server_map\tspecifies the map in the domain directory containing the list of servers\n\t-m specifies that after file is updated, ypmake should be run with\n\t   the optional arguments; all arguments after -m are passed to ypmake\n"),
	    invo_name);
#endif /* YP_UPDATE */

	exit (EINVAL);
}
