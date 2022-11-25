#ifndef lint
static char rcsid[] = "@(#)pcnfsd:	$Revision: 1.38.109.1 $	$Date: 91/11/19 14:01:59 $";
#endif

/*  static char     sccsid[] = "pcnfsd.c	1.4";  */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

/*
 * pcnfsd.c 
 *
 * pcnfsd is intended to remedy the lack of certain critical generic network
 * services by providing an simple, customizable set of RPC-based
 * mechanisms. For this reason, Sun Microsystems Inc. is distributing it
 * in source form as part of the PC-NFS release. 
 *
 * Background: The first NFS networks were composed of systems running
 * derivatives of the 4.2BSD release of Unix (Sun's, VAXes, Goulds and
 * Pyramids). The immediate utility of the resulting networks was derived
 * not only from NFS but also from the availability of a number of TCP/IP
 * based network services derived from 4.2BSD. Furthermore the thorny
 * question of network-wide user authentication, while remaining a
 * security hole, was solved at least in terms of a convenient usage model
 * by the Network Information Service distributed data base facility, which
 * allows multiple Unix systems to refer to common password and group files. 
 *
 * The PC-NFS Dilemma: When Sun Microsystems Inc. ported NFS to PC's, two
 * things became apparent. First, the memory constraints of the typical PC
 * meant that it would be impossible to incorporate the pervasive TCP/IP
 * based service suite in a resident fashion. Indeed it was not at all
 * clear that the 4.2BSD services would prove sufficient: with the advent
 * of Unix System V and (experimental) VAX-VMS NFS implementations, we had
 * to consider the existence of networks with no BSD-derived Unix hosts.
 * The two key types of functionality we needed to provide were remote
 * login and print spooling. The second critical issue  was that of user
 * authentication. Traditional time-sharing systems such as Unix and VMS
 * have well- established user authentication mechanisms based upon user
 * id's and passwords: by defining appropriate mappings, these could
 * suffice for network-wide authentication provided that appropriate
 * administrative procedures were enforced. The PC, however, is typically
 * a single-user system, and the standard DOS operating environment
 * provides no user authentication mechanisms. While this is acceptable
 * within a single PC, it causes problems when attempting to connect to a
 * heterogeneous network of systems in which access control, file space
 * allocation, and print job accounting and routing may all be based upon
 * a user's identity. The initial (and default) approach is to use the
 * pseudo-identity 'nobody' defined as part of NFS to handle problems such
 * as this. However, taking ease of use into consideration, it became
 * necessary to provide a mechanism for establishing a user's identity. 
 *
 * Initially we felt that we needed to implement two types of functionality:
 * user authentication and print spooling. (Remote login is addressed by
 * the Telnet module.) Since no network services were defined within the
 * NFS architecture to support these, it was decided to implement them in
 * a fairly portable fashion using Sun's Remote Procedure Call protocol.
 * Since these mechanisms will need to be re-implemented ion a variety of
 * software environments, we have tried to define a very general model. 
 *
 * Authentication: NFS adopts the Unix model of using a pair of integers
 * (uid, gid) to define a user's identity. This happens to map tolerably
 * well onto the VMS system. 'pcnfsd' implements a Remote Procedure which
 * is required to map a username and password into a (uid, gid) pair.
 * Since we cannot predict what mapping is to be performed, and since we
 * do not wish to pass clear-text passwords over the net, both the
 * username and the password are mildly scrambled using a simple XOR
 * operation. The intent is not to be secure (the present NFS architecture
 * is inherently insecure) but to defeat "browsers". 
 *
 * The authentication RPC will be invoked when the user enters the PC-NFS
 * command: 
 *
 * NET NAME user [password|*] 
 *
 *
 * Printing: The availability of NFS file operations simplifies the print
 * spooling mechanisms. There are two services which 'pcnfsd' has to
 * provide:
 *   pr_init:	given the name of the client system, return the
 * name of a directory which is exported via NFS and in which the client
 * may create spool files.
 *  pr_start: given a file name, a user name, the printer name, the client
 * system name and an option string, initiate printing of the file
 * on the named printer. The file name is relative to the directory
 * returned by pr_init. pr_start is to be "idempotent": a request to print
 * a file which is already being printed has no effect. 
 *
 * Intent: The first versions of these procedures are implementations for Sun
 * 2.0/3.0 software, which will also run on VAX 4.2BSD systems. The intent
 * is to build up a set of implementations for different architectures
 * (Unix System V, VMS, etc.). Users are encouraged to submit their own
 * variations for redistribution. If you need a particular variation which
 * you don't see here, either code it yourself (and, hopefully, send it to
 * us at Sun) or contact your Customer Support representative. 
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include <sys/types.h>
#include <stdio.h>
#include <rpc/rpc.h>
#include <pwd.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/stat.h>

#include <audnetd.h>


/*	HPNFS
**	location of default spool file: /usr/spool/lp is too dangerous
**	to use because files/directories there have special meaning to lp;
**	file used for logging messages later by the daemon;
**	name of the line printer spooler, used by system();
**	name of the file remove command, used by system();
*/
# define	SPOOL_DEFAULT	"/usr/tmp"
# define	CONSOLE		"/dev/console"
# define	LPR		"/usr/bin/lp"
# define	RM		"/bin/rm"

/*	HPNFS
**	definitions for O_RDONLY and O_WRONLY are in here, not sys/file.h
*/
#include	<fcntl.h>
/*	HPNFS
**	include tracing information so we can debug this beast
*/
#include	<errno.h>
extern int errno;
int traceon = 0;
#include	<arpa/trace.h>

#ifdef SecureWare
#include <sys/types.h>
#include <sys/security.h>
#include <prot.h>
#endif /* SecureWare */

#if defined(SecureWare) && defined(B1)

#define ENABLEPRIV(priv) \
	 { \
          if (ISB1)  \
             if (nfs_enablepriv(priv)) { \
		fprintf(stderr, \
(catgets(nlmsg_fd,NL_SETN,13, "pcnfsd:  needs to be executed with SEC_ALLOWDACACCESS SEC_WRITE_AUDIT, SEC_SETPROCIDENT and\n          SEC_ALLOWMACACCESS privileges\n")) ); \
		exit(EACCES); \
	     } \
	 }

#define DISABLEPRIV(priv)       if (ISB1)  nfs_disablepriv(priv);

#else

#define ENABLEPRIV(priv)	{}
#define DISABLEPRIV(priv)	{}

#endif /* SecureWare && B1*/

/*
 * New addition of auditing for the situation where the PC client gives
 * a user name to be validated by this daemon.  
 */ 


#ifdef SecureWare
u_short aud_luid = (uid_t) -1;

#ifdef B1

#define ENABLE_PRIV_FOR_AUDIT() \
                if (ISB1) { \
                     ENABLEPRIV(SEC_ALLOWDACACCESS); \
                     ENABLEPRIV(SEC_ALLOWMACACCESS); \
                     ENABLEPRIV(SEC_WRITE_AUDIT); \
                }
#define DISABLE_PRIV_FOR_AUDIT() \
                if (ISB1) { \
                     DISABLEPRIV(SEC_ALLOWDACACCESS); \
                     DISABLEPRIV(SEC_ALLOWMACACCESS); \
                     DISABLEPRIV(SEC_WRITE_AUDIT); \
                }
#else /* not B1 */

#define ENABLE_PRIV_FOR_AUDIT()   {}
#define DISABLE_PRIV_FOR_AUDIT()  {}
#endif /* not B1 */

#define NFS_AUDIT_DAEMON(stat,str,succ_fail,raddr,rhost,ruid,laddr,aud_luid) \
     	if (ISSECURE) { \
		ENABLE_PRIV_FOR_AUDIT(); \
		audit_daemon(succ_fail, NA_VALI_OTHER, \
		raddr, rhost, ruid, (char *)0, aud_luid, (char *)0, stat, \
		   (ruid == (uid_t)-1 ? NA_MASK_RUID : 0)| \
		   (aud_luid == (uid_t)-1 ? NA_MASK_LUID : 0)| \
		   (raddr == (u_long)-1 ? NA_MASK_RADDR : 0)| \
		   (rhost == (char *)0 ? NA_MASK_RHOST : 0)| \
 		   (NA_MASK_RUSRNAME)|(NA_MASK_LUSR), (str) ); \
		DISABLE_PRIV_FOR_AUDIT(); \
	}	
#endif  /* SecureWare */

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * This is here because the audit subsystem is hosed for now.
 * This cannot be released as is or there will be NO auditing!!!
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */

#define NO_AUDIT 1

#ifdef NO_AUDIT
#undef  NFS_AUDIT_DAEMON
#define NFS_AUDIT_DAEMON(stat,str,succ_fail,raddr,rhost,ruid,laddr,aud_luid) {}
#endif



/*	HPNFS
**	used later; redefine them here instead of having #ifdef hpux
**	all over the code (it was getting ugly fast)
*/
#define	random		lrand48


/*
 * *************** RPC parameters ******************** 
 */
#define	PCNFSDPROG	(long)150001
#define	PCNFSDVERS	(long)1
#define	PCNFSD_AUTH	(long)1
#define	PCNFSD_PR_INIT	(long)2
#define	PCNFSD_PR_START	(long)3

/*
 * ************* Other #define's ********************** 
 */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#define	zchar		0x5b

/*
 * *********** XDR structures, etc. ******************** 
 */
enum arstat {
	AUTH_RES_OK, AUTH_RES_FAKE, AUTH_RES_FAIL
};
enum pirstat {
	PI_RES_OK, PI_RES_NO_SUCH_PRINTER, PI_RES_FAIL
};
enum psrstat {
	PS_RES_OK, PS_RES_ALREADY, PS_RES_NULL, PS_RES_NO_FILE,
	PS_RES_FAIL
};

struct auth_args {
	char           *aa_ident;
	char           *aa_password;
};

struct auth_results {
	enum arstat     ar_stat;
	long            ar_uid;
	long            ar_gid;
};

struct pr_init_args {
	char           *pia_client;
	char           *pia_printername;
};

struct pr_init_results {
	enum pirstat    pir_stat;
	char           *pir_spooldir;
};

struct pr_start_args {
	char           *psa_client;
	char           *psa_printername;
	char           *psa_username;
	char           *psa_filename;	/* within the spooldir */
	char           *psa_options;
};

struct pr_start_results {
	enum psrstat    psr_stat;
};


/*
 * ****************** Misc. ************************ 
 */

char           *authproc();
char           *pr_start();
char           *pr_init();
struct stat     statbuf;

char            pathname[MAXPATHLEN];
char            new_pathname[MAXPATHLEN];
char            spoolname[MAXPATHLEN];

int		dispatch();

/*
 * ************** Support procedures *********************** 
 */
scramble(s1, s2)
	char           *s1;
	char           *s2;
{
	TRACE2("scramble %s", s1);
	while (*s1) {
		*s2++ = (*s1 ^ zchar) & 0x7f;
		s1++;
	}
	*s2 = 0;
	TRACE2("scramble returns %s", s2);
}

/*
 * *************** XDR procedures ***************** 
 */
bool_t
xdr_auth_args(xdrs, aap)
	XDR            *xdrs;
	struct auth_args *aap;
{
	TRACE("xdr_auth_args");
	return (xdr_string(xdrs, &aap->aa_ident, 32) &&
		xdr_string(xdrs, &aap->aa_password, 64));
}

bool_t
xdr_auth_results(xdrs, arp)
	XDR            *xdrs;
	struct auth_results *arp;
{
	TRACE("xdr_auth_results");
	return (xdr_enum(xdrs, &arp->ar_stat) &&
		xdr_long(xdrs, &arp->ar_uid) &&
		xdr_long(xdrs, &arp->ar_gid));
}

bool_t
xdr_pr_init_args(xdrs, aap)
	XDR            *xdrs;
	struct pr_init_args *aap;
{
	TRACE("xdr_pr_init_args");
	return (xdr_string(xdrs, &aap->pia_client, 64) &&
		xdr_string(xdrs, &aap->pia_printername, 64));
}

bool_t
xdr_pr_init_results(xdrs, arp)
	XDR            *xdrs;
	struct pr_init_results *arp;
{
	TRACE("xdr_pr_init_results");
	return (xdr_enum(xdrs, &arp->pir_stat) &&
		xdr_string(xdrs, &arp->pir_spooldir, 255));
}

bool_t
xdr_pr_start_args(xdrs, aap)
	XDR            *xdrs;
	struct pr_start_args *aap;
{
	TRACE("xdr_pr_start_args");
	return (xdr_string(xdrs, &aap->psa_client, 64) &&
		xdr_string(xdrs, &aap->psa_printername, 64) &&
		xdr_string(xdrs, &aap->psa_username, 64) &&
		xdr_string(xdrs, &aap->psa_filename, 64) &&
		xdr_string(xdrs, &aap->psa_options, 64));
}

bool_t
xdr_pr_start_results(xdrs, arp)
	XDR            *xdrs;
	struct pr_start_results *arp;
{
	TRACE("xdr_pr_start_results");
	return (xdr_enum(xdrs, &arp->psr_stat));
}



usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,11, "Usage: pcnfsd [ -l log_file ] [ spool-dir ]\r\n")));
	exit(1);
}


# define	streql(s1,s2)	!strcmp(s1,s2)
/*
 *********************** main ********************* 
 */

#ifdef BFA

write_BFAdbase()
{
        _UpdateBFA();
}
#endif /* BFA */

main(argc, argv)
	int             argc;
	char          **argv;
{
	int             devnull, num_fds;
	extern          xdr_string_array();
	char		*log_file=CONSOLE, *invo_name = *argv;

	/*
	**	if tracing is defined in the compile options, then open
	**	the trace file and start writing to it!
	*/
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("pcnfsd",0);
#endif NLS
	STARTTRACE("/tmp/pcnfsd");

#ifdef SecureWare
/*
 * Changes have been made to let pcnfsd access the proctected password
 * database so it can do user authentication.  Changes have also been
 * made to insure that least privilege is maintained.
 */
      if (ISSECURE) {
              /*
               * set_auth_parameters() will change the umask, so keep
               * a copy and restore it.
	       * A call to this routine is needed before accessing
	       * the protected password database.
               */
              mode_t cmask = umask(0);
              set_auth_parameters(argc, argv);
              umask(cmask);
	      aud_luid = getluid();
      }
#ifdef B1
/*
 * This program assumes it will be started by the "epa" command and that it
 * will have the default privileges plus SEC_ALLOWDACACCESS and SEC_SETPROCIDENT.
 * An ENABLEPRIV will be done on the privileges as there is no sense
 * in proceeding if they are not available.  They will then be disabled
 * until they are needed later where it will be raised and lowered.
 */
	if (ISB1) {
              /*
	       * Only allow a user who is a member of the "network"
	       * protected subsystem to execute and utilize the potential
	       * privileges associated with this program.
	       */
	      if (authorized_user("") == 0) {
			TRACE("Not authorized for network subsystem");
			logmsg(catgets(nlmsg_fd,NL_SETN,14, "pcnfsd: Not authorized for network subsystem:  Permission denied\n"));
			exit(1);
		}

		nfs_initpriv();
		ENABLEPRIV(SEC_ALLOWDACACCESS);
		ENABLEPRIV(SEC_SETPROCIDENT);
		ENABLEPRIV(SEC_WRITE_AUDIT);
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		DISABLEPRIV(SEC_SETPROCIDENT);
		DISABLEPRIV(SEC_WRITE_AUDIT);
	}

#endif /* B1 */
#endif /* SecureWare */

	strcpy(spoolname, SPOOL_DEFAULT);
	for (--argc, ++argv; argc > 0; --argc, ++argv) {
		TRACE2("on argument '%s'", *argv);
		if (**argv == '-') {
			TRACE2("argument is an option: '%s'", *argv);
			if (streql(*argv, "-l")) {
				TRACE("matched '-l'");
				--argc, ++argv;
				TRACE2("log_file set to '%s'", *argv);
				log_file = *argv;
			} else {
				TRACE("illegal option, usage&exit");
				usage();
			}
		} else {
			TRACE2("assuming spool file is '%s'", *argv);
			strcpy(spoolname, *argv);
		}
	}
	TRACE2("main invoc is '%s'\n", invo_name);
	TRACE2("main Logfile is '%s'\n", log_file);
	TRACE2("main spoolname is '%s'\n", spoolname);

	if (stat(spoolname, &statbuf) || !(statbuf.st_mode & S_IFDIR)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "pcnfsd: invalid spool directory %s\n")), spoolname);
		usage();
	}
	if ((devnull = open("/dev/null", O_RDONLY)) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "pcnfsd: couldn't open /dev/null\n")));
		usage();
	}
	TRACE2("main got /dev/null on fd %d", devnull);
	/*
	 * Since it is possible to have /dev/console as the default log
	 * file, SEC_ALLOWDACACCESS is needed for the open of the file
	 */
	ENABLEPRIV(SEC_ALLOWDACACCESS);
	if (log_file && *log_file && startlog(invo_name, log_file) < 0) {
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "pcnfsd: couldn't open log file '%s'\n")), log_file);
		usage();
	}
	DISABLEPRIV(SEC_ALLOWDACACCESS);

	/*
	**	This function will put us into the background, ignore
	**	SIGHUP signals, free us from our controlling terminal, and
	**	prevent us from acquiring another one on a later open ...
	*/
	Become_daemon();

	TRACE("main child, about to dup over 0,1,2");
	/*
	**	The child used to dup2 /dev/null to stdin, and the
	**	LOGFILE to stdout, stderr.  Instead dup everything
	**	to /dev/null!  Use logmsg() to write any messages.
	*/
	dup2(devnull, 0);
	dup2(devnull, 1);
	dup2(devnull, 2);
	(void) close(devnull);
	/*
	**	register the programs and go off and wait for "action"
	*/

	TRACE("main registerrpc(PCNFSDPROG,PCNFSDVERS,PCNFSD_AUTH)");
#ifdef SecureWare
	/*
	 * This call to secure_svc_register() is made in order to use
	 * the lower level rpc calls instead of the middle level rpc
	 * calls because the middle level does not pass along the 
	 * SVCXPRT struct which contains the socket which contains the
	 * remote client node's address that is needed when doing auditing
	 */
	secure_svc_register(PCNFSDPROG, PCNFSDVERS, dispatch);
	TRACE("Back from secure_svc_register");
#else  /* SecureWare */
	registerrpc(PCNFSDPROG, PCNFSDVERS, PCNFSD_AUTH, authproc,
		xdr_auth_args, xdr_auth_results);
	TRACE("main registerrpc(PCNFSDPROG,PCNFSDVERS,PCNFSD_PR_INIT)");
	registerrpc(PCNFSDPROG, PCNFSDVERS, PCNFSD_PR_INIT, pr_init,
		xdr_pr_init_args, xdr_pr_init_results);
	TRACE("main registerrpc(PCNFSDPROG,PCNFSDVERS,PCNFSD_PR_START)");
	registerrpc(PCNFSDPROG, PCNFSDVERS, PCNFSD_PR_START, pr_start,
		xdr_pr_start_args, xdr_pr_start_results);
#endif /* SecureWare */
#ifdef NEW_SVC_RUN
	/* num_fds = 32 is fine because this is an all UDP service.
	 * If TCP sockets are registered, num_fds would have to account for the
	 * sockets that accept() on a tcp socket will generate or we should
	 * use svc_run, which by default allows FDSET_SIZE file descriptors */

	num_fds = 32;

	TRACE("main calling svc_run()");
	svc_run_ms(num_fds);
#else
	TRACE("main calling svc_run()");
	svc_run();
#endif /* NEW_SVC_RUN */

	TRACE("main svc_run() returned!  Mucho bad");
	logmsg(catgets(nlmsg_fd,NL_SETN,4, "pcnfsd: error: svc_run returned"));
	exit(1);
}

/*
 * ******************* RPC procedures ************** 
 */
 /*
  * Since authproc() is called like a normal low level rpc server routine
  * when it is compiled with SecureWare defined, there is a separate
  * function declaration here
  */

#ifdef SecureWare
char	       *
authproc(a, rqstp, transp)
	struct auth_args *a;
	struct svc_req   *rqstp;
	SVCXPRT		 *transp;
#else

char           *
authproc(a)
	struct auth_args *a;

#endif /* SecureWare */
{
	static struct auth_results r;
	char            username[32];
	char            password[64];
	int             c1, c2;
	struct passwd  *p, *getpwnam();

#ifdef SecureWare
	struct sockaddr_in remoteaddr, myaddr;
	char *machine = (char *) NULL;
	struct pr_passwd *ppr;
	char *ciphertext;
#endif /* SecureWare */

	char audit_msg[512];

#ifdef SecureWare
	machine = ((struct authunix_parms *)rqstp->rq_clntcred)->aup_machname;
	remoteaddr = *svc_getcaller(transp);
	get_myaddress(&myaddr);

#endif /* SecureWare */

	TRACE("authproc()");
	r.ar_stat = AUTH_RES_FAIL;	/* assume failure */
	scramble(a->aa_ident, username);
	scramble(a->aa_password, password);
	p = getpwnam(username);
	TRACE2("authproc getpwnam returns 0x%x", p);
	if (p == NULL)
		goto bottom;	/*  HPNFS  jad  87.04.27  */

#ifdef SecureWare
	if (ISSECURE) {
		/*
	 	 * This code is what is needed to interface with the protected
	 	 * password database of secure HP-UX.  You can't just
		 * look in /etc/passwd anymore to do authentication.
 	 	 * Check the protected password entry for a user. 
 	 	 */

		ENABLEPRIV(SEC_ALLOWDACACCESS);
		ppr = getprpwnam(username);
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		sprintf(audit_msg, catgets(nlmsg_fd, NL_SETN, 15,
			 "pcnfsd: passwords do not match for the user %s"),
			  username);
		if (ppr == (struct pr_passwd *) 0) {
			TRACE("authproc passwords do not match:  ppr == 0");           
			goto bottom;
		}

		/*
	 	 * Check fg_name and fg_uid flags to make sure that the 
	 	 * fd_name and fd_uid fields are valid
	 	 * Also check for either the password from the user or
	 	 * the password in the database being NULL
	 	 */
	
		c1 = strlen(password);
		c2 = strlen(ppr->ufld.fd_encrypt);
		if (ppr->uflg.fg_name == 0 || ppr->uflg.fg_uid == 0 ||
	    	    ppr->ufld.fd_uid != p->pw_uid ||
	    	    (c1 && !c2) || (c2 && !c1) ||
	    		strcmp(ppr->ufld.fd_name, p->pw_name) != 0) {
			TRACE("authproc passwords do not match:  ppr flags incorrect");           
			goto bottom;
		}
		/*
  	 	 * check that the password sent over is correct. 
 	 	 * Compare with the one in the protected password database.
 	 	 * return 0 on success, else nonzero.
 	 	 */
		if (!ppr->uflg.fg_encrypt) {
			TRACE("authproc passwords do not match:  bad encryption flag");           
			goto bottom;
		}
		ciphertext = bigcrypt(password, ppr->ufld.fd_encrypt);
		if (strcmp(ciphertext, ppr->ufld.fd_encrypt) != 0) {
				TRACE("authproc passwords do not match");
				goto bottom;
		}
		if (locked_out(ppr)) {
			sprintf(audit_msg, catgets(nlmsg_fd, NL_SETN, 16,
			    "pcnfsd: account is disabled for the user %s"),
			     username);
			TRACE("authproc account is disabled");           
			goto bottom;
		}
		if (ppr->uflg.fg_type &&
	    		ISBITSET(ppr->ufld.fd_type, AUTH_RETIRED_TYPE)) {
			sprintf(audit_msg, catgets(nlmsg_fd, NL_SETN, 17,
			    "pcnfsd: account has been retired for the user %s"),
			     username);
			TRACE("authproc account has been retired ");           
			goto bottom;
		}
		r.ar_uid = ppr->ufld.fd_uid;
	} else   /* if (ISSECURE) */

#endif /* SecureWare */

	{  /* This brace is here just to make the ifdef/endif's cleaner */
		c1 = strlen(password);
		c2 = strlen(p->pw_passwd);
		if ((c1 && !c2) || (c2 && !c1) ||
			(strcmp(p->pw_passwd, crypt(password, p->pw_passwd)))) {
			TRACE("authproc passwords do not match");
			goto bottom;	/*  HPNFS  jad  87.04.27  */
		}
		r.ar_uid = p->pw_uid;
	}
	r.ar_gid = p->pw_gid;
	r.ar_stat = AUTH_RES_OK;
	TRACE3("authproc passwords MATCH, uid=%d, gid=%d", r.ar_uid, r.ar_gid);
	sprintf(audit_msg, catgets(nlmsg_fd, NL_SETN, 18,
	    "pcnfsd: account is valid for the user %s"), username);
        NFS_AUDIT_DAEMON(NA_STAT_START, audit_msg, NA_RSLT_SUCCESS,
                     remoteaddr.sin_addr.s_addr, machine,
                     ((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid,
                     myaddr.sin_addr.s_addr, aud_luid);
	return ((char *) &r);

	/*	HPNFS	jad	87.04.27
	**	label bottom added to get BFA coverage of this routine;
	**	all "return" statements were changed to "goto bottom"
	**	which is where we return from ... (distasteful)
	*/
bottom:
#ifdef	BFA
_UpdateBFA();
#endif	BFA
        NFS_AUDIT_DAEMON(NA_STAT_START, audit_msg, NA_RSLT_FAILURE,
                     remoteaddr.sin_addr.s_addr, machine,
                     ((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid,
                     myaddr.sin_addr.s_addr, aud_luid);

	return ((char *) &r);

}


char           *
pr_init(pi_arg)
	struct pr_init_args *pi_arg;
{
	int             dir_mode = 0777;
	static struct pr_init_results pi_res;

	TRACE("pr_init()");
	/* get pathname of current directory and return to client */
	strcpy(pathname, spoolname);	/* first the spool area */
	strcat(pathname, "/");	/* append a slash */
	strcat(pathname, pi_arg->pia_client);
	/* now the host name */
	TRACE2("pr_init try to make directory %s", pathname);
	mkdir(pathname);	/* ignore the return code */
	
	/*
	 * This probably does not need to have any special privileges,
	 * since the spool directory is in "/usr/tmp",
	 * but if there is ever a problem with the spooler, then this
	 * is where the ENABLE/DISABLE of SEC_ALLOWDACACCESS needs to
	 * be done
	 */
	if (stat(pathname, &statbuf) || !(statbuf.st_mode & S_IFDIR)) {
		TRACE("pr_init can't create spool directory, PI_RES_FAIL");
		logmsg((catgets(nlmsg_fd,NL_SETN,12, "pcnfsd: unable to create spool directory %s")),
			pathname);
		pathname[0] = 0;/* null to tell client bad vibes */
		pi_res.pir_stat = PI_RES_FAIL;
	} else {
		TRACE("pr_init spool directory OK, PI_RES_OK");
		pi_res.pir_stat = PI_RES_OK;
	}
	pi_res.pir_spooldir = &pathname[0];
	chmod(pathname, dir_mode);

#ifdef	BFA
_UpdateBFA();
#endif	BFA
	return ((char *) &pi_res);
}

char           *
pr_start(ps_arg)
	struct pr_start_args *ps_arg;
{
	static struct pr_start_results ps_res;
	int             pid;
	char            printer_opt[64];
	char            username_opt[64];
	char            clientname_opt[64];
	struct passwd  *p;
	long		rnum;
	char		snum[20];

	TRACE("pr_start()");
#ifdef	hpux
	/*	HPNFS
	**	use default options for the printer, whatever they are.
	**	also, print the user name on the banner page since hp-ux
	**	does not have a username option; no clientname option either.
	**	Also print "pcnfsd" on the banner page.
	*/
	strcpy(printer_opt, "-d");	/* device name */
	strcpy(username_opt, "-t");	/* print name on banner page, since */
	strcpy(clientname_opt, "-C");
#else	hpux
	strcpy(printer_opt, "-P");
	strcpy(username_opt, "-J");
	strcpy(clientname_opt, "-C");
#endif	hpux

	/*
	**	if we ignore SIGCLD then we don't need to free_child
	**	Sun used a signal handler to free the zombie child
	*/
	signal(SIGCLD, SIG_IGN);
	strcpy(pathname, spoolname);	/* build filename */
	strcat(pathname, "/");
	strcat(pathname, ps_arg->psa_client);	/* /spool/host */
	strcat(pathname, "/");	/* /spool/host/ */
	strcat(pathname, ps_arg->psa_filename);	/* /spool/host/file */

	TRACE2("pr_start pathname =%s\n", pathname);
	TRACE2("pr_start username = %s\n", ps_arg->psa_username);
	TRACE2("pr_start client = %s\n", ps_arg->psa_client);

	/*
	**	concatenate the printer name after the option letter
	*/
	strcat(printer_opt, ps_arg->psa_printername);
	/*
	**	prints out the following on two lines of the banner page,
	**	followed by the name of the user (workstation) spooling
	**	the print request.
	*/
	strcat(username_opt, "'pcnfs ");
	strcat(username_opt, ps_arg->psa_username);
	strcat(username_opt, "'");
	strcat(clientname_opt, ps_arg->psa_client);
	TRACE2("pr_start printer_opt = %s\n", printer_opt);
	TRACE2("pr_start username_opt = %s\n", username_opt);
	TRACE2("pr_start clientname_opt = %s\n", clientname_opt);

	if (stat(pathname, &statbuf)) {
		/*
		 * We can't stat the file. Let's try appending '.spl' and
		 * see if it's already in progress. 
		 */
		TRACE("pr_start stat failed");
		strcat(pathname, ".spl");
		if (stat(pathname, &statbuf)) {
			/*
			 * It really doesn't exist. 
			 */
			TRACE("pr_start stat .spl failed, PS_RES_NO_FILE");
			ps_res.psr_stat = PS_RES_NO_FILE;
			goto bottom;	/*  HPNFS  jad  87.04.27  */
		}

		/*
		 * It is already on the way. 
		 */
		TRACE("pr_start stat succeeded, PS_RES_ALREADY");
		ps_res.psr_stat = PS_RES_ALREADY;
		goto bottom;	/*  HPNFS  jad  87.04.27  */
	}
	if (statbuf.st_size == 0) {
		/*
		 * Null file - don't print it, just kill it. 
		 */
		(void) unlink(pathname);
		TRACE("pr_start stat found 0 byte file, PS_RES_NULL");
		ps_res.psr_stat = PS_RES_NULL;
		goto bottom;	/*  HPNFS  jad  87.04.27  */
	}
	/*
	 * The file is real, has some data, and is not already going out.
	 * We rename it by appending '.spl' and exec "lpr" to do the
	 * actual work. 
	**	HPNFS
	**	we call system("lp FILE ; rm FILE"); to simulate the
	**	Berkeley "lpr -r" option.
	 */
	strcpy(new_pathname, pathname);
	strcat(new_pathname, ".spl");
	TRACE3("pr_start found %s, renamed %s", pathname, new_pathname);

	/*
	 * See if the new filename exists so as not to overwrite it.
	 */
	if (!stat(new_pathname, &statbuf)) {
		/*
		**	rebuild the file name using random number generator
		**	HPNFS	should watch out for going over 14 character
		**		filename limit!  maybe use mktemp instead?
		*/
		strcpy(new_pathname, pathname);
		sprintf(snum,"%ld",random());
		strncat(new_pathname, snum, 3);
		strcat(new_pathname, ".spl");
		TRACE2("pr_start made new name %s", new_pathname);
	}

	if (rename(pathname, new_pathname)) {
		/*
		 * CAVEAT: Microsoft changed rename for Microsoft C V3.0.
		 * Check this if porting to Xenix. 
		 */
		/*
		 * Should never happen. 
		 */
		TRACE3("pcnfsd: spool file rename (%s->%s) failed, PR_RES_FAIL",
			pathname, new_pathname);
		logmsg(catgets(nlmsg_fd,NL_SETN,5, "pcnfsd: spool file rename (%1$s->%2$s) failed."), pathname, new_pathname);
		ps_res.psr_stat = PS_RES_FAIL;
		goto bottom;	/*  HPNFS  jad  87.04.27  */
	}
	pid = fork();
	TRACE2("pr_start after fork, pid = %d", pid);
	if (pid == 0) {
		char	command[256];
		/*
		**	the child exec's the print spooler;
		**	command is the buffer to hold the system command
		*/
		if (ps_arg->psa_options[1] == 'd') {
			/*
			 * This is a Diablo print stream. Apply the ps630
			 * filter with the appropriate arguments. 
			**	HPNFS
			**	silently ignore -d option!  No ps_630
			**	filter ability on HP-UX ... see the
			**	original code for the filter
			*/
			TRACE("would try to run_ps630");
		}
		if ((p=getpwnam(ps_arg->psa_username)) || (p=getpwnam("lp"))) {
		    int lpgid = p->pw_gid;
		    int lpuid = p->pw_uid;
		    /*
		    **	either we've found the userid
		    **	we have found the user "lp", setuid() to it
		    **	so listings don't come out owned by "root".
		    */
		    ENABLEPRIV(SEC_IDENTITY)
		    TRACE2("pr_start trying to setgid to lp (%d)", lpgid);
		    (void) setgid(lpgid);
		    TRACE2("pr_start trying to setuid to lp (%d)", lpuid);
		    (void) setuid(lpuid);
		    DISABLEPRIV(SEC_IDENTITY)
		}
#ifdef	hpux
		/*	HPNFS
		**	-s suppress messages when the command is queued,
		**	-c forces a copy into the spool directory, so it
		**	is safe to remove the file immediately after the
		**	lp command returns.
		*/
		sprintf(command,"%s -s -c %s %s %s ; %s -f %s",
			LPR,			/* path to lp	*/
			printer_opt,		/* -d<PRINTER>	*/
			username_opt,		/* -t<USER>	*/
			new_pathname,		/* file.spl	*/
			RM,			/* path to rm	*/
			new_pathname);		/* file.spl	*/
		TRACE2("pr_start about to call system(%s)", command);
		if (system(command) < 0) {
		    /*	jad	Mon Apr 13 16:06:03 MDT 1987
		    **	Commented the perror statement out because
		    **	for some reason it always printed "No child procs"
		    **	on the console; we didn't run out of procs, so I
		    **	have no clue why system returned -1 w/errno = 10
		    ****
		    perror((catgets(nlmsg_fd,NL_SETN,6, "system()")));
		    ****
		    */
		    TRACE2("system command failed, errno = %d",errno);
		}
		TRACE("pr_start done with system command, child exits");
		ENDTRACE();
#else	hpux
		/*
		**	why use execlp with a full path name???
		**	Berkeley options -s and -r mean:
		**	-s: symbolic link the file into the spool directory
		**	-r: remove the file after it has been printed!!
		**	HP-UX lp does not have this facility, see above
		*/
		execlp("/usr/ucb/lpr",
			"lpr",
			"-s",
			"-r",
			printer_opt,
			username_opt,
			clientname_opt,
			new_pathname,
			0);
		perror((catgets(nlmsg_fd,NL_SETN,7, "pcnfsd: exec lpr failed")));
		logmsg((catgets(nlmsg_fd,NL_SETN,7, "pcnfsd: exec lpr failed")));
#endif	hpux
		_exit(1);	/* end of child process */
	} else if (pid == -1) {
		perror((catgets(nlmsg_fd,NL_SETN,8, "pcnfsd: fork failed")));
		logmsg((catgets(nlmsg_fd,NL_SETN,8, "pcnfsd: fork failed")));
		TRACE("pr_start fork failed, PR_RES_FAIL");
		ps_res.psr_stat = PS_RES_FAIL;
		goto bottom;	/*  HPNFS  jad  87.04.27  */
	} else {
		TRACE2("pr_start forked child %d, PR_RES_OK", pid);
		ps_res.psr_stat = PS_RES_OK;
		goto bottom;	/*  HPNFS  jad  87.04.27  */
	}
	/*	HPNFS	jad	87.04.27
	**	label bottom added to get BFA coverage of this routine;
	**	all "return" statements were changed to "goto bottom"
	**	which is where we return from ... (distasteful)
	*/
bottom:
#ifdef	BFA
_UpdateBFA();
#endif	BFA
	return ((char *) &ps_res);
}



/*
**	This function will put us into the background, ignore
**	SIGHUP signals, free us from our controlling terminal, and
**	prevent us from acquiring another one on a later open ...
*/
Become_daemon()
{
    int f;
    /*
    **	make sure the fork()s return OK ... if not, exit with an error
    **	use _exit so that we don't double-flush buffers (MUST MAKE SURE
    **	ALL BUFFERS HAVE BEEN FLUSHED!), but we don't expect pending
    **	output at this point in the process ...
    */
    TRACE("daemon()");
    if (f=fork())
	if (f > 0)	_exit(0);
	else {
	    perror((catgets(nlmsg_fd,NL_SETN,9, "fork")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,9, "fork")));
	    exit(1);
	}
    TRACE("daemon after fork");
    /*
    **	setpgrp() will break terminal affiliations and make us the pgrp leader;
    **	we ignore SIGHUP so our parent's exit does not cause the child to exit
    */
    (void) setpgrp();
    signal(SIGHUP, SIG_IGN);

#ifdef BFA
        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);
#endif /* BFA */

    TRACE("daemon after setpgrp and signal");
    /*
    **	fork again to prevent us from acquiring a controlling terminal
    **	when we do an open() (like /dev/console ??)
    **	use _exit() to prevent double flush; only affects Become_daemon
    */
    if (f=fork())
	if (f > 0)	_exit(0);
	else {
	    perror((catgets(nlmsg_fd,NL_SETN,10, "fork")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,10, "fork")));
	    exit(1);
	}
    /*
    **	at this point we are a grand-child of the command line process,
    **	we are a process group leader, we are ignoring the HUP signal,
    **	we have no controlling terminal, and we won't get one when we
    **	do an open() on a tty device
    */
}

/*
 * This code is here to replace the middle-level RPC call of registerrpc()
 * that is normally used to register this RPC program.  This is just
 * the low-level RPC equivalent.
 */

#ifdef SecureWare
secure_svc_register(prognum, versnum, procname)
        u_long prognum, versnum;
        char *(*procname)();
{
	SVCXPRT 	*transp;

        if ((transp = svcudp_create(RPC_ANYSOCK)) == NULL) {
                TRACE("secure_svc_register svcudp_create failed, exit");
                logmsg(catgets(nlmsg_fd,NL_SETN,19, "pcnfsd: couldn't create udp transport\n"));
                exit(1);
        }
	pmap_unset(prognum, versnum);
        if (!svc_register(transp, prognum, versnum, procname, IPPROTO_UDP)) {
                TRACE("secure_svc_register svc_register failed, exit");
                logmsg(catgets(nlmsg_fd,NL_SETN,20, "pcnfsd: couldn't register PCNFSDPROG"));
                exit(1);
 	}

}

/* 
 * This code is here so that low-level RPC calls can be used.
 * This code is called when an RPC request comes in for the daemon.
 * The proper procedure is determined, the request decoded, the
 * proper procedure called (in the fashion that middle-level
 * RPC procedure expect to be called), the results encoded and
 * then sent back.
 */
dispatch(rqstp, transp)
        struct svc_req *rqstp;
        SVCXPRT *transp;
{
	char xdrbuf[UDPMSGSIZE], *results;

        switch(rqstp->rq_proc) {
                case NULLPROC:
                        TRACE("mnt case NULLPROC");
                        if (!svc_sendreply(transp, xdr_void, 0)) {
                                TRACE("mnt svc_sendreply failed");
                                logmsg(catgets(nlmsg_fd,NL_SETN,21, "pcnfsd: couldn't reply to rpc call\n"));
                        }
                        break;
		case PCNFSD_AUTH:
			TRACE("dispatch case PCNFSD_AUTH");
			if (!svc_getargs(transp, xdr_auth_args, xdrbuf)) {
                		TRACE("pcnfsd unable to svc_getargs");
                		svcerr_decode(transp);
                		return;
			}
			results = authproc(xdrbuf, rqstp, transp);
                	if (!svc_sendreply(transp, xdr_auth_results, results)) {
				TRACE("dispatch  couldn't reply to rpc call");
                        	logmsg(catgets(nlmsg_fd,NL_SETN,21, "pcnfsd: couldn't reply to rpc call\n"));
			}
                        /* free the decoded arguments */
                        svc_freeargs(transp, xdr_auth_args, xdrbuf);
			break;
		case PCNFSD_PR_INIT:
			TRACE("dispatch case PCNFSD_PR_INIT");
			if (!svc_getargs(transp, xdr_pr_init_args, xdrbuf)) {
                		TRACE("pcnfsd unable to svc_getargs");
                		svcerr_decode(transp);
                		return;
			}
			results = pr_init(xdrbuf);
                	if (!svc_sendreply(transp, xdr_pr_init_results, results)) {
				TRACE("dispatch  couldn't reply to rpc call");
                        	logmsg(catgets(nlmsg_fd,NL_SETN,21, "pcnfsd: couldn't reply to rpc call\n"));
			}
                        /* free the decoded arguments */
                        svc_freeargs(transp, xdr_pr_init_args, xdrbuf);
			break;
		case PCNFSD_PR_START:
			TRACE("dispatch case PCNFSD_PR_START");
			if (!svc_getargs(transp, xdr_pr_start_args, xdrbuf)) {
                		TRACE("pcnfsd unable to svc_getargs");
                		svcerr_decode(transp);
                		return;
			}
			results = pr_start(xdrbuf);
                	if (!svc_sendreply(transp, xdr_auth_results, results)) {
				TRACE("dispatch  couldn't reply to rpc call");
                        	logmsg(catgets(nlmsg_fd,NL_SETN,21, "pcnfsd: couldn't reply to rpc call\n"));
			}
                        /* free the decoded arguments */
                        svc_freeargs(transp, xdr_pr_start_args, xdrbuf);
			break;
                default:
                        TRACE("dispatch case default");
                        svcerr_noproc(transp);
                        break;
        }

}
#endif /* SecureWare */
