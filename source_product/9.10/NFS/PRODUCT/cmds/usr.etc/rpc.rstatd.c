#ifndef lint
static  char rcsid[] = "@(#)rpc.rstatd:	$Revision: 1.38.109.2 $	$Date: 93/10/26 16:18:14 $  ";
#endif
/* rpc.rstatd.c	2.2 86/05/15 NFSSRC */ 
/*static  char sccsid[] = "rpc.rstatd.c 1.1 86/02/05 Copyr 1984 Sun Micro";*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/* 
 * rstat demon:  called from inet
 *
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <nlist.h>
#include <sys/errno.h>
#include <sys/vmmeter.h>
#include <sys/pstat.h>
#include <net/if.h>
#include <time.h>
#include <rpcsvc/rstat.h>
#include <signal.h>
/*	HPNFS
**	include the tracing macros ...
*/
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/rstatd.trace"
/*
**	define KERNEL instead of hard-coding "/vmunix" !!!
**	also define KMEM since it's used multiple times...
*/
# define	KERNEL		"/hp-ux"
# define	KMEM		"/dev/kmem"

/*
**	define XDR_DRIVES to be the number of drives the silly XDR protocol
**	sends across the net; SHOULD *REALLY* use a counted array and
**	xdr_array instead of blindly assuming 4 drives ... (sigh).
**	define MAX_DRIVES to be the largest possible number of drives on a
**	s800 system (the maximum value of dk_ndrive in the kernel)
****
##	NOTE: we're now pulling in a fixed value for DK_NDRIVE from rstat.h
*/
# define	MAX_DRIVES	64

#ifdef hp9000s800
struct nlist nl[] = {
#define	X_CPTIME	0
	{ "cp_time" },
#define	X_SUM		1
	{ "sum" },
#define	X_IFNET		2
	{ "ifnet" },
#define	X_DKXFER	3
	{ "dk_xfer" },
#define	X_BOOTTIME	4
	{ "boottime" },
#define	X_AVENRUN	5
	{ "avenrun" },
#define X_HZ		6
	{ "hz" },
#define X_DK_NDRIVE	7
	{ "dk_ndrive" },
	"",
};
#else not hp9000s800

struct nlist nl[] = {
#define	X_CPTIME	0
	{ "_cp_time" },
#define	X_SUM		1
	{ "_sum" },
#define	X_IFNET		2
	{ "_ifnet" },
#define	X_DKXFER	3
	{ "_dk_xfer" },
#define	X_BOOTTIME	4
	{ "_boottime" },
#define	X_AVENRUN	5
	{ "_avenrun" },
#define X_HZ		6
	{ "_hz" },
	"",
};
#endif hp9000s800


int kmem;
int firstifnet, numintfs;	/* chain of ethernet interfaces */
int stats_service();

union {
    struct stats s1;
    struct statsswtch s2;
    struct statstime s3;
} stats;

extern int errno;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

#ifndef FSCALE
#define FSCALE (1 << 8)
#endif

char	LogFile[64];		/* log file: use it instead of console	*/
long	Exitopt=600l;		/* # of seconds to wait between checks	*/
int	My_prog=RSTATPROG,		/* the program number	*/
	My_vers=RSTATVERS_ORIG,		/* the version number	*/
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

main(argc, argv)
int	argc;
char	**argv;
{
	SVCXPRT *transp;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int num_fds, port;

	STARTTRACE(TRACEFILE);
#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rpc.rstatd",0);
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

        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);

	TRACE("main signals set up for HUP, INT, QUIT, TERM");
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
		TRACE2("main error with getsockname, errno = %d", errno);
		log_perror(catgets(nlmsg_fd,NL_SETN,1, "rstatd: getsockname"));
		perror((catgets(nlmsg_fd,NL_SETN,1, "rstatd: getsockname")));
		exit(1);
	} else {
		My_port = addr.sin_port;
		TRACE2("main found My_port = %d", My_port);
	}
	TRACE("main after getsockname, about to call svcudp_bufcreate");
	if ((transp = svcudp_bufcreate(0, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE))
	    == NULL) {
		TRACE2("main error with svcudp_bufcreate, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,2, "svc_rpc_udp_create: error\n"));
		exit(1);
	}
	TRACE("main about to call svc_register(RSTATPROG,RSTATVERS_ORIG)");
	if (!svc_register(transp,RSTATPROG,RSTATVERS_ORIG,stats_service,0)) {
		TRACE2("main error with svc_register ORIG, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,3, "svc_rpc_register: error\n"));
		exit(1);
	}
	TRACE("main about to call svc_register(RSTATPROG,RSTATVERS_SWTCH)");
	if (!svc_register(transp,RSTATPROG,RSTATVERS_SWTCH,stats_service,0)) {
		TRACE2("main error with svc_register SWTCH, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,4, "svc_rpc_register: error\n"));
		exit(1);
	}
	TRACE("main about to call svc_register(RSTATPROG,RSTATVERS_TIME)");
	if (!svc_register(transp,RSTATPROG,RSTATVERS_TIME,stats_service,0)) {
		TRACE2("main error with svc_register TIME, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,5, "svc_rpc_register: error\n"));
		exit(1);
	}

	TRACE("main about to call setup()");
	setup();

	num_fds = getnumfds();

#ifdef NEW_SVC_RUN
	TRACE2("main about to call svc_run_ms(), numfds = %d",num_fds);
	svc_run_ms(num_fds);
#else /* NEW_SVC_RUN */
	TRACE("main about to call svc_run()");
	svc_run();
#endif /* NEW_SVC_RUN */

	TRACE("main svc_run returned!  We now die!");
	logmsg(catgets(nlmsg_fd,NL_SETN,6, "svc_run should never return\n"));
	exit(1);
}

static int
stats_service(reqp, transp)
	 struct svc_req  *reqp;
	 SVCXPRT  *transp;
{
	int have;
	
	TRACE("stats_service SOP, switch on reqp->rq_proc");
	switch (reqp->rq_proc) {

		case NULLPROC:
			TRACE("stats_service case 0");
			if (svc_sendreply(transp, xdr_void, 0, TRUE)
			    == FALSE) {
				logmsg(catgets(nlmsg_fd,NL_SETN,11, "err: svc_rpc_send_results"));
				TRACE("stats_service: svc_sendreply failed, exit");
				exit(1);
			    }
			TRACE("stats_service sendreply OK, break");
			break;

		case RSTATPROC_STATS:
			TRACE("stats_service case RSTATPROC_STATS");
			/*
			**	update the kernel statistics before
			**	I try to svc_sendreply ...
			*/
			updatestat();
			if (reqp->rq_vers == RSTATVERS_ORIG) {
				TRACE("stats_service RSTATVERS_ORIG");
				if (svc_sendreply(transp, xdr_stats,
				    &stats.s1, TRUE) == FALSE) {
					logmsg(catgets(nlmsg_fd,NL_SETN,7, "err: svc_rpc_send_results"));
		    			TRACE("stats_service: svc_sendreply failed, exit");
					exit(1);
				}
				TRACE("stats_service sendreply OK, break");
				break;
			}
			if (reqp->rq_vers == RSTATVERS_SWTCH) {
				TRACE("stats_service RSTATVERS_SWTCH");
				if (svc_sendreply(transp, xdr_statsswtch,
				    &stats.s2, TRUE) == FALSE) {
					logmsg(catgets(nlmsg_fd,NL_SETN,8, "err: svc_rpc_send_results"));
		    			TRACE("stats_service: svc_sendreply failed, exit");
					exit(1);
				    }
				TRACE("stats_service sendreply OK, break");
				break;
			}
			if (reqp->rq_vers == RSTATVERS_TIME) {
				TRACE("stats_service RSTATVERS_TIME");
				if (svc_sendreply(transp, xdr_statstime,
				    &stats.s3, TRUE) == FALSE) {
					logmsg(catgets(nlmsg_fd,NL_SETN,9, "err: svc_rpc_send_results"));
		    			TRACE("stats_service: svc_sendreply failed, exit");
					exit(1);
				    }
				TRACE("stats_service sendreply OK, break");
				break;
			}
			svcerr_progvers(transp);
			break;

		case RSTATPROC_HAVEDISK:
			TRACE("stats_service case RSTATPROC_HAVEDISK");
			have = havedisk();
			TRACE2("stats_service have = %d", have);
			if (svc_sendreply(transp,xdr_long, &have, TRUE) == 0){
			    logmsg(catgets(nlmsg_fd,NL_SETN,10, "err: svc_sendreply"));
			    TRACE("stats_service: svc_sendreply failed, exit");
			    exit(1);
			}
			TRACE("stats_service sendreply OK, break");
			break;

		default: 
			TRACE("stats_service case default");
			svcerr_noproc(transp);
			TRACE("stats_service svcerr_noproc, break");
			break;
		}
	/*
	**	see if I should exit now, or hang around until remapped
	*/
	check_exit();
	TRACE("stats_service check_exit returned");
}


/*
**	updatestat()	--	read kernel data structures, place them
**				in struct stats for return to the caller
*/
updatestat()
{
	int off, i, hz;
	struct vmmeter sum;
	struct ifnet ifnet;
	struct timeval tm, btm;
	struct pst_static stat_info; 	/*rb - 8-15-91 */
	struct pst_dynamic dyn_info;

	
	TRACE("updatestat SOP");
	/*
	**	looks up the current time (for s3 info)
	*/
	(void) gettimeofday(&stats.s3.curtime, 0);
	TRACE2("updatestat gettimeofday says time is %ld",stats.s3.curtime);

	/*
	**	look up the load average and etc. information for s2
	**	RB - changed this mechanism from kmem to pstat 8-15-91        
	*/
         
	TRACE("calling pstat for dyn_info- load stats");
	if (pstat(PSTAT_DYNAMIC, &dyn_info, sizeof(dyn_info), 0, 0) < 0)
        { TRACE2("pstat call for bootime failed, errno= %d",errno);
            logmsg(catgets(nlmsg_fd,NL_SETN,38,"pstat call failed\n"));
            exit(1);
        }

	stats.s2.avenrun[0] = dyn_info.psd_avg_1_min * (double)FSCALE;
	stats.s2.avenrun[1] = dyn_info.psd_avg_5_min * (double)FSCALE;
	stats.s2.avenrun[2] = dyn_info.psd_avg_15_min * (double)FSCALE;
	TRACE4("updatestat load averages are (%0.3f,%0.3f,%0.3f)",
	   		dyn_info.psd_avg_1_min,dyn_info.psd_avg_5_min,
					dyn_info.psd_avg_15_min);								
         
         /* RB - calling pstat for boottime rather than dev kme 8-15-91 */

	 TRACE("calling pstat for stat_info -bootime");
	 if (pstat(PSTAT_STATIC, &stat_info, sizeof(stat_info), 0, 0) <0)
         { TRACE2("pstat call for bootime failed, errno= %d",errno);
            logmsg(catgets(nlmsg_fd,NL_SETN,38,"pstat call failed\n"));
            exit(1);
         }
         stats.s2.boottime.tv_sec = stat_info.boot_time;
         stats.s2.boottime.tv_usec = 0;


	/*
	**	now grab the SUM information; this code is duplicated later
	*/
	if (lseek(kmem, (long)nl[X_SUM].n_value, 0) ==-1) {
		TRACE2("updatestat can't lseek SUM kmem, errno = %d",errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,16, "can't seek in kmem\n"));
		exit(1);
	}
	if (read(kmem, &sum, sizeof sum) != sizeof sum) {
		TRACE2("updatestat can't read SUM kmem, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,17, "can't read sum from kmem\n"));
		exit(1);
	}
	stats.s2.v_swtch = sum.v_swtch;
	TRACE2("updatestat stats.s2.v_swtch = %d", stats.s2.v_swtch);

	/*
	**	look up lots of different data in the kernel, for s1
	*/
	if (lseek(kmem, (long)nl[X_HZ].n_value, 0) == -1) {
		TRACE2("updatestat can't lseek X_HZ kmem, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,18, "can't seek in kmem\n"));
		exit(1);
	}
	if (read(kmem, &hz, sizeof hz) != sizeof hz) {
		TRACE2("updatestat can't read hz kmem, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,19, "can't read hz from kmem\n"));
		exit(1);
	}
      /* manipulate dyn_info.psd_cpu_time stats to conform to NFS standard */
      /* of 4 CPUSTATES. Copy pstat data to s1 data area */

	  stats.s1.cp_time[0] = dyn_info.psd_cpu_time[0];
	  stats.s1.cp_time[1] = dyn_info.psd_cpu_time[1];
	  stats.s1.cp_time[2] = dyn_info.psd_cpu_time[2];
	  stats.s1.cp_time[3] = dyn_info.psd_cpu_time[3];
	  stats.s1.cp_time[3] += dyn_info.psd_cpu_time[4];
	  stats.s1.cp_time[2] += dyn_info.psd_cpu_time[5];
	  stats.s1.cp_time[2] += dyn_info.psd_cpu_time[6];
	  stats.s1.cp_time[2] += dyn_info.psd_cpu_time[7];
	  stats.s1.cp_time[2] += dyn_info.psd_cpu_time[8];

#define	SCTIME	stats.s1.cp_time
	TRACE5("updatestat stats.s1.cp_time=(%d,%d,%d,%d)",
			    SCTIME[0], SCTIME[1], SCTIME[2], SCTIME[3]);
	if (lseek(kmem, (long)nl[X_AVENRUN].n_value, 0) ==-1) {
		TRACE2("updatestat can't lseek AVENRUN kmem, errno = %d",errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,22, "can't seek in kmem\n"));
		exit(1);
	}
	/*
	**	now grab the SUM information; this code is duplicated earlier
	*/
	if (lseek(kmem, (long)nl[X_SUM].n_value, 0) ==-1) {
		TRACE2("updatestat can't lseek SUM kmem, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,23, "can't seek in kmem\n"));
		exit(1);
	}
 	if (read(kmem, &sum, sizeof sum) != sizeof sum) {
		TRACE2("updatestat can't read SUM kmem, errno = %d", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,24, "can't read sum from kmem\n"));
		exit(1);
	}
	/*
	**	stuff all the SUM info into the stats.s1 data area ...
	*/
	stats.s1.v_pgpgin = sum.v_pgpgin;
	stats.s1.v_pgpgout = sum.v_pgpgout;
	stats.s1.v_pswpin = sum.v_pswpin;
	stats.s1.v_pswpout = sum.v_pswpout;
	stats.s1.v_intr = sum.v_intr;
	gettimeofday(&tm);
	stats.s1.v_intr -= hz*(tm.tv_sec - btm.tv_sec) +
	    hz*(tm.tv_usec - btm.tv_usec)/1000000;
	TRACE2("updatestat stats.s1.v_pgpgin = %d", stats.s1.v_pgpgin);
	TRACE2("updatestat stats.s1.v_pgpgout = %d", stats.s1.v_pgpgout);
	TRACE2("updatestat stats.s1.v_pswpin = %d", stats.s1.v_pswpin);
	TRACE2("updatestat stats.s1.v_pswpout = %d", stats.s1.v_pswpout);
	TRACE2("updatestat stats.s1.v_intr = %d", stats.s1.v_intr);
	/*
	**	get the disk transfer data; need to use get_dk_xfer() since
	**	the s800 handles dk_xfer differently (with dk_ndrive).
	*/
 	get_dk_xfer(stats.s1.dk_xfer);
	TRACE2("updatestat stats.s1.dk_xfer[0] = %d", stats.s1.dk_xfer[0]);
	TRACE2("updatestat stats.s1.dk_xfer[1] = %d", stats.s1.dk_xfer[1]);
	TRACE2("updatestat stats.s1.dk_xfer[2] = %d", stats.s1.dk_xfer[2]);
	TRACE2("updatestat stats.s1.dk_xfer[3] = %d", stats.s1.dk_xfer[3]);
	/*
	**	initialize stats.s1 network interface statistics to zero
	*/
	stats.s1.if_ipackets = 0;
	stats.s1.if_opackets = 0;
	stats.s1.if_ierrors = 0;
	stats.s1.if_oerrors = 0;
	stats.s1.if_collisions = 0;
	/*
	**	run through all interfaces configured into the kernel and
	**	add their totals into the stats.s1 structure
	*/
	for (off = firstifnet, i = 0; off && i < numintfs; i++) {
		TRACE3("updatestat checking interface %d, off = 0x%x", i, off);
		if (lseek(kmem, off, 0) == -1) {
			TRACE3("updatestat can't lseek off=%d, errno = %d",
							off, errno);
			logmsg(catgets(nlmsg_fd,NL_SETN,27, "can't seek in kmem\n"));
			exit(1);
		}
		if (read(kmem, &ifnet, sizeof ifnet) != sizeof ifnet) {
			TRACE2("updatestat can't read ifnet kmem, errno = %d",
								errno);
			logmsg(catgets(nlmsg_fd,NL_SETN,28, "can't read ifnet from kmem\n"));
			exit(1);
		}
		stats.s1.if_ipackets += ifnet.if_ipackets;
		stats.s1.if_opackets += ifnet.if_opackets;
		stats.s1.if_ierrors += ifnet.if_ierrors;
		stats.s1.if_oerrors += ifnet.if_oerrors;
		stats.s1.if_collisions += ifnet.if_collisions;
		TRACE2("updatestat if_ipackets = %d",	ifnet.if_ipackets);
		TRACE2("updatestat if_opackets = %d",	ifnet.if_opackets);
		TRACE2("updatestat if_ierrors = %d",	ifnet.if_ierrors);
		TRACE2("updatestat if_oerrors = %d",	ifnet.if_oerrors);
		TRACE2("updatestat if_collisions = %d",	ifnet.if_collisions);
		TRACE2("updatestat if_next = 0x%x",	ifnet.if_next);
		off = (int) ifnet.if_next;
	}
	TRACE("all done setting up data structures, return");
}

static 
setup()
{
	struct ifnet ifnet;
	int off, *ip;
	
	TRACE("setup SOP, about to call nlist");
	(void) nlist(KERNEL, nl);
	TRACE2("setup nlist finally returned, n_value = %d", nl[0].n_value);
	if (nl[0].n_value == 0) {
		TRACE("setup nlist n_value = 0, vars missing, exit!");
		logmsg(catgets(nlmsg_fd,NL_SETN,29, "Variables missing from namelist\n"));
		exit (1);
	}
	if ((kmem = open(KMEM, 0)) < 0) {
		TRACE2("setup can't open kmem, errno = %d, exit!", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,30, "can't open kmem\n"));
		exit(1);
	}

	off = nl[X_IFNET].n_value;
	TRACE2("setup got IFNET offset = 0x%x, trying to lseek", off);
	if (lseek(kmem, off, 0) == -1) {
		TRACE2("setup can't seek to ifnet, errno = %d, exit!", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,31, "can't seek in kmem\n"));
		exit(1);
	}
	if (read(kmem, &firstifnet, sizeof(int)) != sizeof (int)) {
		TRACE2("setup can't read ifnet, errno = %d, exit!", errno);
		logmsg(catgets(nlmsg_fd,NL_SETN,32, "can't read firstifnet from kmem\n"));
		exit(1);
	}
	numintfs = 0;
	for (off = firstifnet; off;) {
		TRACE2("setup trying to read ifnet at offset 0x%x", off);
		if (lseek(kmem, off, 0) == -1) {
			TRACE2("setup could not seek ifnet, errno = %d, exit!", errno);
			logmsg(catgets(nlmsg_fd,NL_SETN,33, "can't seek in kmem\n"));
			exit(1);
		}
		if (read(kmem, &ifnet, sizeof ifnet) != sizeof ifnet) {
			TRACE2("setup could not read ifnet, errno = %d, exit!", errno);
			logmsg(catgets(nlmsg_fd,NL_SETN,34, "can't read ifnet from kmem\n"));
			exit(1);
		}
		numintfs++;
		off = (int) ifnet.if_next;
	}
	TRACE2("setup numintfs = %d, now returning", numintfs);
}

/* 
 * returns true if have a disk
 */
static
havedisk()
{
	int i, cnt;
	long  xfer[DK_NDRIVE];

	TRACE("havedisk SOP");
	TRACE("havedisk calling get_dk_xfer");
	get_dk_xfer(xfer);
	cnt = 0;
	for (i=0; i < DK_NDRIVE; i++)
		cnt += xfer[i];
	TRACE2("havedisk returning cnt = %d", cnt);
	return (cnt);
}


/*
**	get_dk_xfer()	--	code to get the number of disk transfers
**			reads kernel data structures and fills in 4 array
**			positions passed in to it (defined by rstat XDR).
##	NOTE:	This function localizes all the s800 arch. differences.
*/
get_dk_xfer(xfer)
long	xfer[];
{
    int	i, j;

#if defined(hp9000s300) || defined(hp9000s700)
    static	int	dk_ndrive=DK_NDRIVE, len=DK_NDRIVE*sizeof(long);
    long		dk_xfer[DK_NDRIVE];
#else	hp9000s800
    /*
    **	the s800 added variable dk_ndrive to the kernel; all other systems
    **	have this value fixed at 4 (DK_NDRIVE) (value of DK_NDRIVE).
    **	the array dk_xfer is as large as the kernel data could ever be ...
    */
    static	int	dk_ndrive=0, len=0;
    long		dk_xfer[MAX_DRIVES];

    /*
    **	this stuff has to be in the ifdef, too, since X_DK_NDRIVE
    **	is only defined in #ifde hp9000s800 code above ...
    */
    if (dk_ndrive == 0) {
	if (lseek(kmem, (long)nl[X_DK_NDRIVE].n_value, 0) < 0) {
	    TRACE2("get_dk_xfer can't seek DK_NDRIVE kmem, errno = %d",errno);
	    logmsg("can't seek in kmem\n");
	    exit(1);
	}
	len = sizeof(dk_ndrive);
	if (read(kmem, (char *)&dk_ndrive, len) != len) {
	    TRACE2("get_dk_xfer can't read DK_NDRIVE kmem, errno = %d", errno);
	    logmsg("can't read dk_ndrive from kmem\n");
	    exit(1);
	}
	/*
	**	the number of bytes we need to read is determined by dk_ndrive
	*/
	len = dk_ndrive * sizeof(long);
    }
#endif hp9000s800

    if (lseek(kmem, (long)nl[X_DKXFER].n_value, 0) < 0) {
	TRACE2("get_dk_xfer can't seek DKXFER kmem, errno = %d",errno);
	logmsg("can't seek in kmem\n");
	exit(1);
    }
    if (read(kmem, (char *)dk_xfer, len) != len) {
	TRACE2("get_dk_xfer can't read DKXFER kmem, errno = %d", errno);
	logmsg("can't read dk_xfer from kmem\n");
	exit(1);
    }

    /*
    **	transfer data from kernel data structure to xfer array ...
    */
    for (i=0, j=0; i < dk_ndrive; i++) {
	if (dk_xfer[i])
	    xfer[j++] = dk_xfer[i];
	if (j == DK_NDRIVE)
	    break;
    }
    /*
    **	clear out xfer array -- in case not enough data is available
    */
    while (j < DK_NDRIVE)
	xfer[j++] = 0l;
}
