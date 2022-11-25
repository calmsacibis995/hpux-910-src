#ifndef lint
static  char rcsid[] = "@(#)nfsstat:	$Revision: 1.33.109.1 $	$Date: 91/11/19 14:10:53 $  ";
#endif
/* nfsstat.c	2.2 86/05/15 NFSSRC */ 
/*static  char sccsid[] = "nfsstat.c 1.1 86/02/05 Copyr 1983 Sun Micro";*/
/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* 
 * nfsstat: Network File System statistics
 *
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
char *catgets();
#endif NLS

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <nlist.h>

#if defined(SecureWare) && defined(B1)
/*
 * The security strategy for this command is this:
 *   1) To read /dev/kmem, the command will
 *      ENABLE/DISABLEPRIV(SEC_ALLOWDACACCESS) around the open()
 *   2) If the "-z" flag is used (used to zero out nfs statistics), the
 *      user must be a member of the "network" protected subsystem.
 *	If the user is not a member, the code wil exit so as not to
 *	allow the open("/dev/kmem", O_RDWR) to be done with
 *	SEC_ALLOWDACACCESS enabled.
 */
#include <sys/security.h>
#include <errno.h>
#include <prot.h>

#define ENABLEPRIV(priv)  \
	{ \
	  if (ISB1) { \
	     if (nfs_enablepriv(priv)) { \
	         fprintf(stderr, \
(catgets(nlmsg_fd,NL_SETN,57, "nfsstat: this command must have the SEC_ALLOWDACACCESS potential privilege\n")) ); \
	         exit(EACCES); \
	     } \
	  } \
	}


#define DISABLEPRIV(priv)	if (ISB1)  nfs_disablepriv(priv);

#else

#define ENABLEPRIV(priv)	{}
#define DISABLEPRIV(priv)	{}
#endif /* SecureWare && B1 */

#ifdef hp9000s800 
struct nlist nl[] = {
#define	X_RCSTAT	0
	{ "rcstat" },
#define	X_CLSTAT	1
	{ "clstat" },
#define	X_RSSTAT	2
	{ "rsstat" },
#define	X_SVSTAT	3
	{ "svstat" },
	"",
};

#else not hp9000s800

struct nlist nl[] = {
#define	X_RCSTAT	0
	{ "_rcstat" },
#define	X_CLSTAT	1
	{ "_clstat" },
#define	X_RSSTAT	2
	{ "_rsstat" },
#define	X_SVSTAT	3
	{ "_svstat" },
	"",
};
#endif hp9000s800


int kmem;			/* file descriptor for /dev/kmem */

char *vmunix = "/hp-ux";	/* HPNFS /vmunix is SUN's equivalent to */
				/* HPNFS our /hp-ux			*/

char *core = "/dev/kmem";	/* name for /dev/kmem */

#if defined(SecureWare) && defined(B1)
extern 	mask_t eff_privs[SEC_SPRIVVEC_SIZE];
#endif /* SecureWare && B1 */

/*
 * client side rpc statistics
 */
struct {
        int     rccalls;
        int     rcbadcalls;
        int     rcretrans;
        int     rcbadxids;
        int     rctimeouts;
        int     rcwaits;
        int     rcnewcreds;
} rcstat;

/*
 * client side nfs statistics
 */
struct {
        int     nclsleeps;              /* client handle waits */
        int     nclgets;                /* client handle gets */
        int     ncalls;                 /* client requests */
        int     nbadcalls;              /* rpc failures */
        int     reqs[32];               /* count of each request */
} clstat;

/*
 * Server side rpc statistics
 */
struct {
        int     rscalls;
        int     rsbadcalls;
        int     rsnullrecv;
        int     rsbadlen;
        int     rsxdrcall;

#ifdef hp9000s700
	int     rsruns;
#endif hp9000s700
} rsstat;

/*
 * server side nfs statistics
 */
struct {
        int     ncalls;         /* number of calls received */
        int     nbadcalls;      /* calls that failed */
        int     reqs[32];       /* count for each request */
} svstat;

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS
extern char **environ;

main(argc, argv)
	char *argv[];
{
	char *options;
	int	cflag = 0;		/* client stats */
	int	dflag = 0;		/* network disk stats */
	int	nflag = 0;		/* nfs stats */
	int	rflag = 0;		/* rpc stats */
	int	sflag = 0;		/* server stats */
	int	zflag = 0;		/* zero stats after printing */


	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0);

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("nfsstat",0);
#endif NLS

#if defined(SecureWare) && defined(B1)
        /*
         * Initialize the authentication database so that a check can be
         * of the invoking user to insure that it is a member of the
         * "network" protected subsystem.
         */

	if (ISB1) {
              /*
               * set_auth_parameters() will change the umask, so keep
               * a copy and restore it.
               */ 
                mode_t cmask = umask(0);
                set_auth_parameters(argc, argv);
                umask(cmask);
		nfs_initpriv();
        }
#endif /* defined(SecureWare) && defined(B1) */


	if (argc >= 2 && *argv[1] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'c':
				cflag++;
				break;
			case 'n':
				nflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 's':
				sflag++;
				break;
			case 'z':
#if defined(SecureWare) && defined(B1)
				if (ISB1) {
                                        /*
					 * See if the user is a member of the
					 * "network" protected subsystem
					 * before allowing the use of the -z
					 * flag to zero out nfs statistics.
					 */
					if (authorized_user("") == 0) {
                        	 	   logmsg(catgets(nlmsg_fd,NL_SETN,58,
                                            "nfsstat: Not authorized for network subsystem:  use of -z flag denied\n"));
                                           exit(1);
                                        }
				} else
#endif /* SecureWare && B1 */
				{
				    if (getuid()) {
					fprintf(stderr,
					  (catgets(nlmsg_fd,NL_SETN,1, "nfsstat: Must be root for z flag\n")));
					exit(1);
				    }
				}
				zflag++;
				break;
			default:
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}
	if (argc >= 2) {
		vmunix = argv[1];
		argv++;
		argc--;
	}
	if (argc != 1) {
		usage();
	}


	setup(zflag);
	getstats();
	if (sflag || (!sflag && !cflag)) {
		if (rflag || (!rflag && !nflag)) {
			sr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			sn_print(zflag);
		}
	}
	if (cflag || (!sflag && !cflag)) {
		if (rflag || (!rflag && !nflag)) {
			cr_print(zflag);
		}
		if (nflag || (!rflag && !nflag)) {
			cn_print(zflag);
		}
	}
	if (zflag) {
		putstats();
	}
	exit(0);
}

getstats()
{
	int size;

	if (lseek(kmem, (long)nl[X_RCSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
	if (read(kmem, &rcstat, sizeof rcstat) != sizeof rcstat) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "nfsstat: can't read rcstat from kmem\n")));
		exit(1);
	}

	if (lseek(kmem, (long)nl[X_CLSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
 	if (read(kmem, &clstat, sizeof(clstat)) != sizeof (clstat)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "nfsstat: can't read clstat from kmem\n")));
		exit(1);
	}

	if (lseek(kmem, (long)nl[X_RSSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,6, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
 	if (read(kmem, &rsstat, sizeof(rsstat)) != sizeof (rsstat)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "nfsstat: can't read rsstat from kmem\n")));
		exit(1);
	}

	if (lseek(kmem, (long)nl[X_SVSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
 	if (read(kmem, &svstat, sizeof(svstat)) != sizeof (svstat)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "nfsstat: can't read svstat from kmem\n")));
		exit(1);
	}
}

putstats()
{
	if (lseek(kmem, (long)nl[X_RCSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
	if (write(kmem, &rcstat, sizeof rcstat) != sizeof rcstat) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,11, "nfsstat: can't write rcstat to kmem\n")));
		exit(1);
	}

	if (lseek(kmem, (long)nl[X_CLSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
 	if (write(kmem, &clstat, sizeof(clstat)) != sizeof (clstat)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "nfsstat: can't write clstat to kmem\n")));
		exit(1);
	}

	if (lseek(kmem, (long)nl[X_RSSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
 	if (write(kmem, &rsstat, sizeof(rsstat)) != sizeof (rsstat)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,15, "nfsstat: can't write rsstat to kmem\n")));
		exit(1);
	}

	if (lseek(kmem, (long)nl[X_SVSTAT].n_value, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,16, "nfsstat: can't seek in kmem\n")));
		exit(1);
	}
 	if (write(kmem, &svstat, sizeof(svstat)) != sizeof (svstat)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,17, "nfsstat: can't write svstat to kmem\n")));
		exit(1);
	}
}

setup(zflag)
	int zflag;
{
	register struct nlist *nlp;

	if (nlist(vmunix, nl) < 0) {
		perror((catgets(nlmsg_fd,NL_SETN,56, "nfsstat: nlist")));
		exit(1);
	}
	if (nl[0].n_value == 0) {
		fprintf (stderr, (catgets(nlmsg_fd,NL_SETN,18, "nfsstat: Variables missing from namelist\n")));
		exit (1);
	}
	/*
	 * If the zflag is set and the program flow reached here,
	 * then the user is a member of the * "network" protected subsystem.
	 * (assuming the system was running as B1).  Do the 
	 * enable/disable privilege tango dance around the open of /dev/kmem
	 * so the open will succeed.
	 */
	ENABLEPRIV(SEC_ALLOWDACACCESS);
	if ((kmem = open(core, zflag ? 2 : 0)) < 0) {
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		perror(core);
		exit(1);
	}
	DISABLEPRIV(SEC_ALLOWDACACCESS);
}

cr_print(zflag)
	int zflag;
{
	fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,19, "\nClient rpc:\n")));
	fprintf(stdout,
	 (catgets(nlmsg_fd,NL_SETN,20, "calls      badcalls   retrans    badxid     timeout    wait       newcred\n")));
	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,21, "%-11d%-11d%-11d%-11d%-11d%-11d%-11d\n")),
	    rcstat.rccalls,
            rcstat.rcbadcalls,
            rcstat.rcretrans,
            rcstat.rcbadxids,
            rcstat.rctimeouts,
            rcstat.rcwaits,
            rcstat.rcnewcreds);
	if (zflag) {
		memset(&rcstat, 0, sizeof rcstat);
	}
}

sr_print(zflag)
	int zflag;
{
	fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,22, "\nServer rpc:\n")));
#ifdef hp9000s700
	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,23, "calls      badcalls   nullrecv   badlen     xdrcall    nfsdrun\n")));
	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,24, "%-11d%-11d%-11d%-11d%-11d%-11d\n")),
           rsstat.rscalls,
           rsstat.rsbadcalls,
           rsstat.rsnullrecv,
           rsstat.rsbadlen,
           rsstat.rsxdrcall,
	   rsstat.rsruns);
#else not hp9000s700

	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,23, "calls      badcalls   nullrecv   badlen     xdrcall\n")));
	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,24, "%-11d%-11d%-11d%-11d%-11d\n")),
           rsstat.rscalls,
           rsstat.rsbadcalls,
           rsstat.rsnullrecv,
           rsstat.rsbadlen,
           rsstat.rsxdrcall);
#endif hp9000s700
	if (zflag) {
		memset(&rsstat, 0, sizeof rsstat);
	}
}

/* HPNFS  Changed an array of strings to an array of structures */
/* HPNFS  which contain the string and the NLS message number   */
/* HPNFS  inside the message catalog.  If any messages change   */
/* HPNFS  or if the message numbers change this has to be fixed */
/* HPNFS  by hand.						*/

#define RFS_NPROC       18

struct nfsstr {
	char *string;
	int nlsmes;
};

struct nfsstr nfsstrings[RFS_NPROC] = {
	{ "null", 25},
	{ "getattr", 26},
	{ "setattr", 27},
	{ "root", 28},
	{ "lookup", 29},
	{ "readlink", 30},
	{ "read", 31},
	{ "wrcache", 32},
	{ "write", 33},
	{ "create", 34},
	{ "remove", 35},
	{ "rename", 36},
	{ "link", 37},
	{ "symlink", 38},
	{ "mkdir", 39},
	{ "rmdir", 40},
	{ "readdir", 41},
	{ "fsstat", 42} };

cn_print(zflag)
	int zflag;
{
	int i;

	fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,43, "\nClient nfs:\n")));
	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,44, "calls      badcalls   nclget     nclsleep\n")));
	fprintf(stdout,
	    (catgets(nlmsg_fd,NL_SETN,45, "%-11d%-11d%-11d%-11d\n")),
            clstat.ncalls,
            clstat.nbadcalls,
            clstat.nclgets,
            clstat.nclsleeps);
	req_print((int *)clstat.reqs, clstat.ncalls);
	if (zflag) {
		memset(&clstat, 0, sizeof clstat);
	}
}

sn_print(zflag)
	int zflag;
{
	fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,46, "\nServer nfs:\n")));
	fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,47, "calls      badcalls\n")));
	fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,48, "%-11d%-11d\n")), svstat.ncalls, svstat.nbadcalls);
	req_print((int *)svstat.reqs, svstat.ncalls);
	if (zflag) {
		memset(&svstat, 0, sizeof svstat);
	}
}

req_print(req, tot)
	int	*req;
	int	tot;
{
	int	i, j;
	char	fixlen[128];

	for (i=0; i<=RFS_NPROC / 7; i++) {
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,49, "%-11s")),
 (catgets(nlmsg_fd,NL_SETN,nfsstrings[j].nlsmes,nfsstrings[j].string)));
		}
		fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,50, "\n")));
		for (j=i*7; j<min(i*7+7, RFS_NPROC); j++) {
			if (tot) {
				sprintf(fixlen,
				    (catgets(nlmsg_fd,NL_SETN,51, "%d %2d%% ")), req[j], (req[j]*100)/tot);
			} else {
				sprintf(fixlen, (catgets(nlmsg_fd,NL_SETN,52, "%d 0%% ")), req[j]);
			}
			fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,53, "%-11s")), fixlen);
		}
		fprintf(stdout, (catgets(nlmsg_fd,NL_SETN,54, "\n")));
	}
}

usage()
{
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,55, "nfsstat [-cnrsz] [namelist]\n")));
	exit(1);
}

min(a,b)
	int a,b;
{
	if (a<b) {
		return(a);
	}
	return(b);
}
