/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/cluster.c,v $
 * $Revision: 1.11.83.4 $        $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/09 13:34:57 $
 */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#ifndef _SYS_STDSYMS_INCLUDED
#    include "../h/stdsyms.h"
#endif   /* _SYS_STDSYMS_INCLUDED  */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/errno.h"
#ifdef __hp9000s300
#include "../machine/cpu.h"
#include "../wsio/timeout.h"
#include "../wsio/intrpt.h"
#include "../s200io/lnatypes.h"
#include "../s200io/drvhw.h" /* changed to s200io for prabha */
#endif

/* next 7 includes added because they are needed by the lan driver.  jtg */
#include "../h/socket.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../net/route.h"
#include "../net/raw_cb.h"
#include "../sio/netio.h"

#ifdef __hp9000s300
#include "../sio/lanc.h"      /* 300/800 convergence janke 4/24/89 */
#include "../s200io/drvhw_ift.h"
#include "../dux/duxlaninit.h"
#include "../s200io/drvmac.h"
#else /* s700 || s800 */
#include "../sio/llio.h"
#include "../sio/lanc.h"      /* 300/800 convergence janke 4/24/89 */
#include "../ufs/fsdir.h"
#endif /* s700 || s800 */
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
/* User.h needs to be after lanc.h, because lanc.h has unions named u,
 * and user.h has #define u (*uptr), thus the typedefs get messed up.
 */
#include "../h/user.h"
#include "../h/mount.h"
#include "../h/utsname.h"
#include "../h/tuneable.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dm.h"
#include "../dux/unsp.h"
#include "../dux/protocol.h"
#define IS_CLUSTER /*cause message structures to be included in cct.h*/
#include "../dux/cct.h"
#undef IS_CLUSTER
#include "../dux/nsp.h"
#if defined(__hp9000s800) && !defined(__hp9000s700)
#include "../h/vnode.h"         /* for IFBLK */
#include "../ufs/inode.h"
#endif
#include "../h/kern_sem.h"
#include "../net/netmp.h"
#include "../dux/duxparam.h"

/*
** Make sure we get the correct timeout routine.
*/
#undef timeout

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

extern dev_t rootdev;			/*root device id */
extern int failed_sites; 		/* number of failed sites */
extern int cleanup_running; /*flag indicating cleanup operation in progress*/
extern struct privgrp_map priv_global;
extern struct privgrp_map privgrp_map[EFFECTIVE_MAXGRPS];
#if defined(__hp9000s700) || defined(__hp9000s800)
#ifdef _WSIO
extern dev_t duxlan_dev[];
#else
extern int duxlan_major[];
extern int duxlan_mgr_index[];
#endif /* _WSIO vs. non _WSIO */
#endif /* __hp9000s800 */

extern struct ifnet *dux_lan_cards[];
extern int num_dux_lan_cards;
extern u_char clustcast_addr[];
extern u_char duxcast_addr[];
extern u_char rootlink[];
extern u_int my_site_status; 	/* site status flags  */
#ifdef NET_DEBUG
extern int net_debug;
#endif NET_DEBUG

/*
 *global variables:
 */
extern site_t my_site;		/*this site's id, defined in /etc/clusterconf*/
extern char my_sitename[];	/*site-name for this site*/
extern site_t swap_site;   		/*swap server site for this client*/
extern site_t root_site;		/*root server's site id*/
extern struct dux_context dux_context;
struct dux_context *build_context();
extern int retry_alive();
int using_debugger = 0;		/* don't call check_alive if set */
struct cct clustab[MAXSITE]; 	/* incore cluster configuration table */

#if defined(__hp9000s700) || defined(__hp9000s800)
u_char *get_lan_addr();
#define get_my_addr()	get_lan_addr(0)
#else
#define get_my_addr()	(dux_lan_glob->local_link_address)
#endif

#if defined(__hp9000s700) || defined(__hp9000s800)
extern int cputype;
#endif

extern int prepage, maxpageout;

/*
 * Initialize out dux flags value for non configurable subsystems.
 * Configurable subsystems (like NFS) are done in the routine
 * init_my_dux_flags().
 */
static int my_dux_flags = 0
#ifdef POSIX
	| DUX_HAS_POSIX
#endif
#ifdef ACLS
	| DUX_HAS_ACLS
#endif
#ifdef AUDIT
	| DUX_HAS_AUDIT
#endif
	;

#ifndef DEBUG
#define DUX_RELEASE "Rel8.0   "
#endif

char dux_release[UTSLEN+1];


/*
 * This configurable routine is called by the always-resident portion of
 * cluster(OS).  This routine may be replaced by dux_nop.  If the lan_id
 * matches any of the lan cards, the first byte is replaced with 0x09.
 * If diskless is not configured, this routine is replaced by dux_nop(),
 * which always returns 0.
 */
dux_lan_check(uap_lan_id)
char *uap_lan_id;
{
    register int i;			/* performance */
    register lan_ift *lp;
    register struct ifnet *ifp;	/* ifnet pointer */
    unsigned char lan_id[ADDRESS_SIZE];
    static unsigned char lan_id_zero[6]; /* zeroed by the compiler */

    if ((i = copyin(uap_lan_id, lan_id, ADDRESS_SIZE)) != 0)
	return(i);

    if (bcmp(lan_id, lan_id_zero, ADDRESS_SIZE) == 0)
	return(EINPROGRESS);	/* KLUDGE - magic for /etc/init */

    for (ifp = ifnet; ifp; ifp = ifp->if_next)
    {
	if (bcmp(ifp->if_name, "lan", 3) == 0) /* lan associated */
	{
	    /*
	     * check hw address and station address.
	     * S800 processor switch over uses station address
	     */
	    lp = (lan_ift *)ifp;  /* cast to lan_ift */
	    if (bcmp(lan_id,
#ifdef __hp9000s300
			((landrv_ift *)lp)->lan_vars.local_link_address,
#else
			lp->is_addr,
#endif
			ADDRESS_SIZE) == 0 ||

		  bcmp(lan_id, lp->station_addr, ADDRESS_SIZE) == 0)
		break; /* got the right lan struct */
	}
    }

    /*
     * Return ENXIO if we couldn't find the correct lan device.
     */
    if (ifp == (struct ifnet *)0)
	return(ENXIO);

    /*
     * The first byte of the clustercast address is 0x09.  The
     * remaining bytes are the same as the root server's LAN ID.
     * This ensures a unique clustercast address.  clustcast_addr
     * is used by the second call to cluster().
     */
    lan_id[0] = (unsigned char)0x09;
    bcopy(lan_id, clustcast_addr, ADDRESS_SIZE);
    if (
#ifdef __hp9000s300
	((landrv_ift *)lp)->lan_vars.hwvars.card_state != HARDWARE_UP ||
#endif
        (lp->hdw_state & LAN_DEAD))
	return(EIO);

    dux_lan_cards[0] = ifp;
    num_dux_lan_cards++;

#ifdef __hp9000s300
    dux_lan_glob = &(((landrv_ift *)lp)->lan_vars);
    dux_hwvars   = &(((landrv_ift *)lp)->lan_vars.hwvars);
#else
#ifdef _WSIO
    /*
     * The s700 lan driver stores the high 12 bits of the
     * minor number in hdw_path[0].  The low order 12 bits
     * are always 0.  Unfortunately, the driver does not
     * (yet) put its major number into the lp->mjr_num field,
     * so we must hard code it!
     */
    if (lp->mjr_num == 0)
	duxlan_dev[0] = makedev(52, (lp->hdw_path[0] << 12));
    else
	duxlan_dev[0] = makedev(lp->mjr_num, (lp->hdw_path[0] << 12));
#else
    duxlan_major[0] = lp->mjr_num;
    duxlan_mgr_index[0] = lp->mgr_index;
#endif /* _WSIO vs non _WSIO */
    dux_lanift_init();
#endif /* s300 vs. s700, s800 */
    return(0);
}

/*
 * Init_my_dux_flags() will initialize the proper value of
 * "my_dux_flags" for the kernel running as it was configured.
 * It also initializes the variable "dux_release" to the appropriate
 * value used to check whether or not a client is compatible with
 * its server.
 */
init_my_dux_flags()
{
#ifdef __hp9000s300
	extern int (*nfsproc[])();
	extern int dux_nop();
#else
	extern int nfs_initialized;
#endif
	extern int cdfs_initialized;

	my_dux_flags |=
#ifdef __hp9000s300
	(nfsproc[0] == dux_nop)?0:DUX_HAS_NFS;
#else
	(nfs_initialized)?DUX_HAS_NFS:0;
#endif

	if (my_dux_flags & DUX_HAS_NFS)
	    my_dux_flags |= DUX_HAS_NFS3_2;

	my_dux_flags |= (cdfs_initialized)?DUX_HAS_CDFS:0;

#ifdef DEBUG
	bcopy(utsname.release, dux_release, UTSLEN);
#else
	bcopy(DUX_RELEASE, dux_release, UTSLEN);
#endif
}

/*
 * Clustering operation for the root server.  It is called by cluster(OS),
 * usually when /etc/cluster is invoked.  It cannot be called more than once.
 * If diskless is not configured, this routine is replaced by dux_nop(),
 * which always returns 0.
 * Note that in the following code, error is declared static.  The only likely
 * error in clustering is running out of memory.  Since clustering only
 * occurs during initialization time, it is unlikely that a subsequent
 * invocation of cluster will work without reconfiguring the kernel.
 * Thus, running cluster twice would most likely give the same error each time.
 * However, since cluster does not clean up after an error, it is dangerous.
 * So, if we have had an error before, we return the same error immediately.
 */

root_cluster()
{
	static int error = 0;
#if defined(__hp9000s800) && !defined(__hp9000s700)
	int map_mi_to_lu();
	struct mount *mp, *getmount();
#endif
	sv_sema_t ss;

	if (error)
		return (error);
	init_my_dux_flags();

#if defined(__hp9000s800) && !defined(__hp9000s700)
	/*
	 * This is a hack.  When we mounted the root device,
	 * we didn't have the LU information for the root
	 * device.  However, m_rdev for the root mount entry
	 * must correspond to the lu number before we pass
	 * it on to the clients because they rely on this information
	 * to pass it back to stat() calls on the client.
	 * At this point we assume that we have the lu dev number
	 * for rootdev since /etc/ioinit should already have run,
	 * so we can stuff that value into the root mount entry.
	 * We can just change the m_rdev value and not worry about
	 * about rehashing the entry because mount entries are
	 * not hashed on rdev on the root server.
	 */
	mp = getmount(rootdev);
	(void) map_mi_to_lu(&mp->m_rdev, IFBLK);
#endif /* s800 only, not for s700 */

	/* Add DUXcast and Clustercast Address */
#if defined(__hp9000s700) || defined(__hp9000s800)
	bcopy (get_my_addr(), clustcast_addr, ADDRESS_SIZE);
	*clustcast_addr = (u_char) 0x09;
#endif

	dux_add_multicast(duxcast_addr);
	dux_add_multicast(clustcast_addr);

	/*
	** Start up the network buffers
	*/
	initialize_netbufs();
	initialize_dux_protocol();

	PXSEMA(&filesys_sema, &ss);
	/*fork limited network server processes*/
	if ((error = fork_nsp(1)) != 0) {
		VXSEMA(&filesys_sema, &ss);
		return(error); 	/*cannot create process*/
	}

	init_clustab(); /*initialize cluster table*/

	/* adding server's address into clustab[]*/
	(void) add_site (my_site, get_my_addr(), bufpages * NBPG,
#ifdef __hp9000s300
		CCT_CPU_ARCH_300, machine_model,
#else
#ifdef __hp9000s700
		CCT_CPU_ARCH_700, (short)cputype,
#else /* __hp9000s800 */
		CCT_CPU_ARCH_800, (short)cputype,
#endif /* 700, 800 */
#endif /* 300 */
		my_site, 0);
	clustab[my_site].status = CL_ACTIVE;

	/* start crash detection */
        check_alive();
	retry_alive();

	/* start clock synchronization */
	clocksync();
	VXSEMA(&filesys_sema, &ss);
	return(0);
}

ws_cluster_retry(print_msg)
{
	if (print_msg)
		printf("Server not responding...\n");
	timeout(ws_cluster_retry, 1, HZ*2);
}

/*
 * Clustering operation for client machines. It is called by
 * rootinit() during kernel initialization operation.
 */
ws_cluster()
{
	extern void wakeup_nsp_forker();
	extern int nsps_started;
	extern volatile int nsps_to_invoke;
	register dm_message clust_msg;
	register struct clusterreq *clustreq;		/*cluster request*/
        register struct buf *respbuf;
	register struct clusterresp *clustresp; 	/* cluster reply */
	register struct cluster_error_resp *clust_err_resp;
	register struct cct *cctp; 	/* cluster configuration table entry */
	u_char *my_addr; 		/* this machine's ETHERNET address */
	struct buf *geteblk();
	register int flags;
	int s;
#ifdef __hp9000s300
	lan_gloptr myglobal; /*Lan Card Driver Interface Structure */
#endif

	init_my_dux_flags();

#ifdef NET_DEBUG
	if( net_debug > 3 )
	{
	  	uprintf("*** ws_cluster ***\n");
	}
#endif NET_DEBUG
	initialize_dux_protocol();
	init_clustab();

	msg_printf("Server's Link Address = 0x%08x%04x\n",
		((unsigned long  *) rootlink)[0],
		((unsigned short *) rootlink)[2]);

	/* Prepare a cluster request*/
retry_clusterreq:
	clust_msg = dm_alloc (sizeof(struct clusterreq), WAIT);
	clustreq = DM_CONVERT (clust_msg, struct clusterreq);
#ifdef __hp9000s300
	myglobal = Myglob_dux_lan_gloptr;
	my_addr =  myglobal->local_link_address;
#else
	my_addr = get_my_addr();
#endif
	/* For now, the net_address is used as machine id */

	bcopy(my_addr, clustreq->machine_id, ADDRESS_SIZE);
	bcopy(my_addr, clustreq->net_addr,sizeof(clustreq->net_addr));

	bcopy(dux_release,clustreq->release,UTSLEN);
	clustreq->total_buffers = bufpages * NBPG;
#ifdef __hp9000s300
	clustreq->cpu_arch = CCT_CPU_ARCH_300;
	clustreq->cpu_model = machine_model;
#else /* hp9000s800 */
#ifdef __hp9000s700
	clustreq->cpu_arch = CCT_CPU_ARCH_700;
#else
	clustreq->cpu_arch = CCT_CPU_ARCH_800;
#endif
	clustreq->cpu_model = (short)cputype;
#endif /* s700, s800 */
	clustreq->client_flags = my_dux_flags;
	respbuf = geteblk (sizeof(clustab));
	flags = DM_SLEEP|DM_RELEASE_REQUEST|DM_REPLY_BUF;

	ws_cluster_retry(0);			/* start retry message */
	clust_msg = dm_send(clust_msg,
		flags,
		DM_CLUSTER,
		DM_DIRECTEDCAST,
 		sizeof (struct clusterresp),
		NULL,
 		respbuf,
		sizeof(clustab),
		 0, NULL, NULL, NULL);
	untimeout(ws_cluster_retry,1);		/* stop retry message */

	if ( DM_RETURN_CODE (clust_msg) == 0)
	{
		clustresp = DM_CONVERT(clust_msg, struct clusterresp);
		root_site 	= clustresp->root_site;
		my_site         = clustresp->site_id;
		u.u_site 	= my_site;
		swap_site 	= clustresp->swap_site;

#if defined(__hp9000s800) && !defined(__hp9000s700)
		/*
		 * This is a work around to the rootinit() panic
		 * if rootdev is unitialized after a cluster.
		 * The 800 client does not use rootdev at all, and the
		 * the sanity check in rootinit() was inherited
		 * from the Series 300.  Rootinit() should be
		 * fixed to not require this, but doing the
		 * following is harmless.
		 */
		rootdev = clustresp->root_dev;
#endif /* s800 only, not for s700 */

		settimeofday1(&clustresp->time, &clustresp->tz);

		if (swap_site != my_site) {
			my_site_status |= CCT_SLWS;
			prepage = NETMAXPG;
			maxpageout = NETMAXPG;
		}
		printf("Swap site is %d\n",swap_site);

		bcopy(clustresp->multiaddr, clustcast_addr, ADDRESS_SIZE);
		bcopy(clustresp->site_name, my_sitename, DIRSIZ+1);
		bcopy(respbuf->b_un.b_addr, clustab, sizeof(clustab));
		{
			int i, lan_card;
			struct cct *cctptr;

			cctptr = clustab;
			lan_card = clustab[my_site].lan_card;
			for (i = 0; i < MAXSITE; i++, cctptr++) {
				if (i == 0) continue;  /* Dummy entry */
				if (i == 1) continue;  /* Root server */
				if (i == my_site) continue;  /* Our site */
				if (cctptr->status & CL_IS_MEMBER) {
					cctptr->lan_card =
					  lan_card == cctptr->lan_card ? 0 : 1;
				}
			}
		}
		dux_add_multicast(duxcast_addr);
		dux_add_multicast(clustcast_addr);

		initialize_netbufs();	/* start up network buffer */

		/*
		** Before we allow any significant activity to occur,
		** clear nak_time (to allow sending), the protocol window
		** request count and request queue pointers, and make sure
		** we can communicate with all sites in the cluster.
		*/
		for (cctp = clustab+1; cctp < clustab+MAXSITE;  cctp++)
		{
			cctp->nak_time = 0;
			cctp->req_count = 0;
			cctp->req_waiting_Qhead = cctp->req_waiting_Qtail = 0;

			if ((cctp->status & CCT_STATUS_MASK) == CL_IS_MEMBER)
			{
				/* mark all member nodes to retry state */
				cctp->status = CL_RETRY;
			}
			else
			{
					cctp->status = CL_INACTIVE;
			}
		}

		/* mark my status to CL_ACTIVE */
		clustab[my_site].status = CL_ACTIVE;

	  	/* setup status and context before requesting alive packets! */
		/* status must be CCT_CLUSTERED before we can recv requests */
		/* context must be set before status */
		u.u_cntxp = (char **) build_context(&dux_context);
		my_site_status |= (CCT_CLUSTERED|CCT_STARTED);
		/* cluster cast an alive request--every one should respond */
		dux_send_alive(TRUE,DM_CLUSTERCAST);

		/* turn on crash detection function */
		check_alive();
		retry_alive(); 	/* kick off retries */

		dm_release (clust_msg, 1);

		/*
		 * Ensure that we have at least one gcsp process
		 * running.  If not, get one going.
		 */
		s = spl6();
		if (nsps_started <= 0) {
		    nsps_started = 1;
		    nsps_to_invoke++;
		    wakeup_nsp_forker();
		}
		splx(s);

		return ;		/*cluster successful*/
	}
	else
	{
		register char *s;
		register errno;

		clust_err_resp = DM_CONVERT(clust_msg, struct cluster_error_resp);
		errno = DM_RETURN_CODE(clust_msg);
		switch (errno) {
	case EPERM:
			bcopy(clust_err_resp->rootlink, rootlink, ADDRESS_SIZE);
			msg_printf("Root Server's Link Address = 0x%08x%04x\n",
			       ((long *)   rootlink)[0],
			       ((ushort *) rootlink)[2] );
			s = "Redirecting to real root server";
			break;
	case EACCES:
			s = "CSP shortage on Server";
			break;
	case EAGAIN:
			s = "Server busy";
			break;
	case EBADF:
	case ENOEXEC:
			s = "cannot exec /etc/read_cct";
			break;
	case EIDRM:
			s = "/etc/read_cct aborted";
			break;
	case EEXIST:
			s = "/etc/clusterconf: duplicate cnode id";
			break;
	case EINVAL:
			s = "/etc/clusterconf: cnode id out of bounds";
			break;
        case ECONNREFUSED:
			s = "Client's kernel is incompatible with server's kernel.";

			if (bcmp(dux_release, clust_err_resp->release, UTSLEN) != 0)
			     printf("Diskless protocol versions don't match:\n    client version \"%s\", server version \"%s\".\n", dux_release, clust_err_resp->release);

			if (my_dux_flags != clust_err_resp->server_flags)
			{
			    char *server_has = "Server has %s configured in, client doesn't.\n";
			    char *client_has = "Server doesn't have %s configured in, client does.\n";
			    int xor_flags = my_dux_flags ^ clust_err_resp->server_flags;

			    if (xor_flags & DUX_HAS_NFS)
				printf((my_dux_flags & DUX_HAS_NFS)?client_has:server_has, "NFS");

			    if (xor_flags & DUX_HAS_NFS3_2)
				printf((my_dux_flags & DUX_HAS_NFS3_2)?client_has:server_has, "NFS 3.2");

			    if (xor_flags & DUX_HAS_POSIX)
				printf((my_dux_flags & DUX_HAS_POSIX)?client_has:server_has, "POSIX");

			    if (xor_flags & DUX_HAS_ACLS)
				printf((my_dux_flags & DUX_HAS_ACLS)?client_has:server_has, "Access Control Lists");

			    if (xor_flags & DUX_HAS_AUDIT)
				printf((my_dux_flags & DUX_HAS_AUDIT)?client_has:server_has, "Auditing");

			    if (xor_flags & DUX_HAS_CDFS)
				printf((my_dux_flags & DUX_HAS_CDFS)?client_has:server_has, "CD Filesystem");
			}
			goto panic;
	case 65535:
			s = "this cnode not in /etc/clusterconf";
			break;
	default:
			printf("errno %d\n",errno);
			s = "unknown cluster error from Server";
			break;
		}
		printf("%s, retrying...\n",s);

		for (errno = 0; errno < 2000; errno++)	/* wait a little bit */
#ifdef __hp9000s300
			snooze(1000);
#else
			busywait(1000);
#endif

		dm_release(clust_msg, 1);
		goto retry_clusterreq;
panic:
		printf("panic: %s\n",s);
		for (;;);	/* loop forever */
	}
}

/* Server's code for handling workstation's cluster call.  This code will
 * be executed by a network server process upon the arrival of a DM_CLUSTER
 * request.
 */

int serving_clusterreq = 0;	/* ensures serial cluster request processing */
u_char serving_clusterreq_addr[ADDRESS_SIZE];

serve_clusterreq(reqp)
dm_message reqp;
{
	extern void wakeup_nsp_forker();
	extern int nsps_started;
	extern volatile int nsps_to_invoke;
	dm_message respp, reqstp;
	dm_message u_request, u_reply; /*UNSP request, reply*/
 	register struct clusterreq *clustreq;
	register struct clusterresp *clustresp;
	register struct cluster_error_resp *clust_err_resp;
 	register struct addmember *addmemberp;
	struct buf *bp, *geteblk();
	site_t new_site, save_site, swap_site;
	char new_site_addr[ADDRESS_SIZE];
	register err;	/*error code*/
	struct unsp_message  *u_reqp, *u_repp;
	struct cct_entry *cct_entry;
	char new_site_name[DIRSIZ+1];
	int total_buffers;
	extern struct timezone tz;
	int s;
	int lan_card;

#ifdef NET_DEBUG
	if (net_debug > 3)
		uprintf("*** serve_clusterreq ***\n");
#endif NET_DEBUG

	/*
	 * Ensure that we only serve one cluster request at a time.
	 */
	{
	    int x = spl6();

	    if (serving_clusterreq) {		/* test semaphore */
		splx(x);			/* restore orig spl */
		dm_quick_reply(EAGAIN);		/* deallocates mbuf */
		return;
	    }
	    serving_clusterreq = 1;		/* set semaphore */
	    splx(x);				/* restore orig spl */
	}

	clustreq = DM_CONVERT(reqp, struct clusterreq);

	/*
	 * Save this cnode address for easier debugging.  Although this
	 * is unnecessary, this code is not part of the performance
	 * path, so we can do it all the time.  The data in the global
	 * "serving_clusterreq_addr" is only valid while the semaphore,
	 * "serving_clusterreq" is 1.
	 */
	bcopy(clustreq->net_addr, serving_clusterreq_addr, ADDRESS_SIZE);

	/*
	 * client thought we were the root server.
	 * tell him the truth.
	 */
	if (!IM_SERVER) {
		err = EPERM;
		respp = dm_alloc (sizeof(struct cluster_error_resp), WAIT);
		clust_err_resp = DM_CONVERT (respp, struct cluster_error_resp);
		bcopy(rootlink, clust_err_resp->rootlink, ADDRESS_SIZE);
		dm_reply (respp, 0, err, NULL, NULL, NULL );
		goto done;
	}

	if ((bcmp(clustreq->release, dux_release, UTSLEN)) ||
	    (clustreq->client_flags != my_dux_flags))
	{
		err = ECONNREFUSED;
		respp = dm_alloc (sizeof(struct cluster_error_resp), WAIT);
		clust_err_resp = DM_CONVERT (respp, struct cluster_error_resp);
		clust_err_resp->server_flags = my_dux_flags;
		bcopy(dux_release, clust_err_resp->release, UTSLEN);
		dm_reply (respp, 0, err, NULL, NULL, NULL );
		goto done;
	}

	/* Figure out which lan card the client came in on */
	lan_card = get_lan_card(DM_HEADER(reqp)->dm_ph.iheader.destaddr);

/*
 * invoke user nsp to read /etc/clusterconf file
 */
	/*
	** First alocate space for the request and reply.
	*/
	u_request =  dm_alloc (sizeof (struct unsp_message), WAIT);
	u_reply   =  dm_alloc (sizeof (struct unsp_message), WAIT);
	/*
	** Take the request and reply pointers to the dux_mbuf
	** and obtain new pointers into the mbuf, so we can easily
	** access structure aligned data.
	*/
	u_reqp = DM_CONVERT (u_request, struct unsp_message);
	u_repp = DM_CONVERT (u_reply, struct unsp_message);
	/*
	** Signify that this is a new request and assign the op-code.
	*/
	u_reqp->um_id = -1;
	DM_OP_CODE (u_request) = DM_READCONF;

	cct_entry = (struct cct_entry *) (u_reqp->um_msg);
	bcopy (clustreq->net_addr, cct_entry->net_addr, ADDRESS_SIZE);
	cct_entry->site_id = 0xffff;
	if ((err = invoke_unsp (u_request, u_reply)) != 0) {
		dm_release (u_request, 1);
		err = ENOENT;
		goto err_return_release;
	}
	dm_wait (u_reply); /* wait for user level server process return*/

	/*
	** Type cast the reply message of the user level server process
	*/
	cct_entry = (struct cct_entry *) (u_repp->um_msg);
	new_site = cct_entry->site_id;
	if ((err = DM_RETURN_CODE(u_reply)) != 0)
		goto err_return_release;
	if ((new_site >= MAXSITE) || (new_site < 1)) {	/* validate site id */
		err = EINVAL;
		goto err_return_release;
	}

	bcopy (cct_entry->site_name, new_site_name, sizeof (new_site_name));
	bcopy (clustreq->net_addr, new_site_addr, ADDRESS_SIZE);
	total_buffers = clustreq->total_buffers;

	respp = dm_alloc (sizeof(struct clusterresp), WAIT);
	clustresp = DM_CONVERT (respp, struct clusterresp);
	clustresp->root_site = my_site;
	clustresp->root_dev = rootdev;

#if defined(FULLDUX) || defined (LOCAL_DISC)  /* Multiple swap servers really */
	swap_site = cct_entry->swap_site;
#else
	/* allow only local swap or swap on root */
	if ( new_site == cct_entry->swap_site )
		swap_site = cct_entry->swap_site;
	else
	{	if (cct_entry->swap_site != my_site)
		{	msg_printf("Err in /etc/clusterconf--Non-root swap servers not supported\n");
			msg_printf("  swap for cnode #%d will be moved to root\n",new_site);
		}
		swap_site= my_site;
	}
#endif
	clustresp->swap_site = swap_site;

	/* set up new site in root server's cluster table, but don't add yet */
	if ((err = add_site (new_site,clustreq->net_addr,total_buffers,
		clustreq->cpu_arch, clustreq->cpu_model, swap_site,
		lan_card)) != 0) {
		dm_release(respp, 1);
		goto err_return_release;
	}

	/*
	 * Indicate our intention to lock the mount semaphore for this
	 * client.  However, we still want to allow M_RECOVERY_LOCK to
	 * occur.
	 */
	save_site = u.u_site;
	u.u_site = new_site;
	lock_mount(M_PREVENT_LOCK);
	u.u_site = save_site;

	clustresp->site_id = new_site;

	bcopy (clustcast_addr, clustresp->multiaddr, ADDRESS_SIZE);
	bcopy (new_site_name, clustresp->site_name, sizeof (new_site_name));

	dm_release (u_reply, 1);	/* request was released by nsp routine*/

	/* inform other cluster members that a new site has joined the cluster*/
	reqstp = dm_alloc (sizeof(struct addmember), WAIT);
	addmemberp = DM_CONVERT (reqstp, struct addmember);
	addmemberp->site_id = new_site;
	addmemberp->total_buffers = total_buffers;
	addmemberp->swap_site = swap_site;
	addmemberp->lan_card = lan_card;
	bcopy (new_site_addr, addmemberp->net_addr, ADDRESS_SIZE);
	dm_send (reqstp,
		 DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
		 DM_ADD_MEMBER,
		 DM_CLUSTERCAST,
		 DM_EMPTY,
		 NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	/*
	 * Now that everyone has performed any necessary cleanup for
	 * this node, restore the mount lock to normal operation (but
	 * we still have it locked).
	 */
	save_site = u.u_site;
	u.u_site = new_site;
	lock_mount(M_ALLOW_LOCK);
	u.u_site = save_site;

	/*
	 * Send a copy of the new clustab[] back to
	 * the requestor in the data buffer.
	 */
	bp = geteblk (sizeof(clustab));
	s=splimp();
	bcopy (clustab, bp->b_un.b_addr, sizeof (clustab));

#if defined(__hp9000s700) || defined(__hp9000s800)
	{
		struct cct *cctptr;

		cctptr = (struct cct *)(bp->b_un.b_addr);
		cctptr++; /* First entry is garbage */

		/* Make sure the cnode gets the lan card address for its
		 * subnet on the server
		 */
		DUX_COPY_ADDRESS(get_lan_addr(lan_card), cctptr->net_addr);
	}
#endif
	clustab[new_site].status = CL_ACTIVE;
	splx(s);

	clustresp->tz = tz;
	dux_gettimeofday(&clustresp->time); /* must be close to reply */
	dm_reply (respp, DM_REPLY_BUF, 0, bp, sizeof (clustab), 0);

done:
	serving_clusterreq = 0;			/* clear semaphore */
	return;

err_return_release:
	dm_release (u_reply, 1);
	dm_quick_reply(err);			/* deallocates mbuf */
	goto done;
}

/*
** Add cluster member.  This routine will be invoked when a DM_ADD_MEMBER
** multicast message is received.
*/
serve_add_member (reqp)
dm_message reqp;
{
	register struct addmember *msgp = DM_CONVERT (reqp, struct addmember);
 	/*read new site's id and network address; make sure its cleaned up*/
	(void) add_site (msgp->site_id, msgp->net_addr, msgp->total_buffers,
			msgp->cpu_arch, msgp->cpu_model, msgp->swap_site,
			(int)msgp->lan_card);
	/*mark site active*/
	clustab[msgp->site_id].status = CL_ACTIVE;
	dm_reply (NULL, 0, 0, NULL, NULL, NULL );
#ifdef NET_DEBUG
/*
	uprintf ("Inside of serve_add_member: add cluster member\n");
	print_cct();
*/
#endif NET_DEBUG
}

/*initialize cluster control table*/
init_clustab()
{
	register struct cct *cctp;
	for (cctp = clustab; cctp < clustab+MAXSITE; cctp++) {
		cctp->status = CL_INACTIVE; /*mark the empty slot*/
		cctp->nak_time = 0;
	}
}


/* prepare a new site to join cluster */

add_site (site, net_addr, total_buffers, cpu_arch, cpu_model, swap_site,
					lan_card)
site_t  site;
char *net_addr;
int total_buffers;
short cpu_arch, cpu_model;
site_t swap_site;
int lan_card;
{
	struct cct *p;
	int s;
	p = &clustab[site];
	if (p->status != CL_INACTIVE) {
	    if ((p->status & CCT_STATUS_MASK) == CL_IS_MEMBER) {
		/*
		 * if net_addr doesn't match what is already in the table,
		 * we have a collision of cnode numbers in /etc/clusterconf.
		 * This should only be seen on root server.
		 */
		if ( IM_SERVER && (bcmp(p->net_addr,net_addr,ADDRESS_SIZE)))
			return(EEXIST);

		/* This site must have gone down and came back before the
		 * crash detection mechanism detects it. Cleanup the
		 * resources used by this site but don`t kill the nsp.
		 * In contrast, nsp for the crashed site (CL_FAILEDs state)
		 * must be killed during cleanup.  NOTE that this ONLY
		 * applies to the root site.  The client site should have
		 * already received a DM_MARK_FAILED from the root site
		 * before the DM_ADD_MEMBER message was sent, so if we're
		 * in this code and we're a client, print a debug message
		 * that we had a problem and try to clean the mess up.
		 */
		if (!IM_SERVER)
#ifdef CLEANUP_DEBUG
		    printf("Active node %d rejoining cluster!\n",site);
#else  CLEANUP_DEBUG
		    msg_printf("Active node %d rejoining cluster!\n",site);
#endif CLEANUP_DEBUG
		s = splimp();
		p->status = CL_FAILED; /*indicate this site needs cleanup*/
		failed_sites++;
		if (!cleanup_running) {
			splx(s);
			/* use this nsp to perform cleanup */
			invoke_cleanup((dm_message)NULL);
		}
		else splx(s);
		/* Either we cleaned it up, or sleep til someone else does */
		while ((p->status == CL_FAILED) || (p->status == CL_CLEANUP))
		    sleep ((caddr_t)&clustab[site], P_CLEANUP);
	}
	else	/* status must be CL_FAILED or CL_CLEANUP */
	{
	    if ((p->status == CL_FAILED) || (p->status == CL_CLEANUP)) {
		/* verify that cleanup is being run */
		s=splimp();
		/* cleanup should be running -- if not start it up */
		if (!cleanup_running) {
			if (failed_sites == 0)
			{
			    count_failed_sites();
#ifdef CLEANUP_DEBUG
			    printf("WARNING: Cleanup inconsistency.  Trying to correct.\n");
#else  CLEANUP_DEBUG
			    msg_printf("WARNING: Cleanup inconsistency.  Trying to correct.\n");
#endif CLEANUP_DEBUG
			}
			splx(s);
			invoke_cleanup((dm_message)NULL);
		}
		else splx(s);
		/* Either we cleaned it up, or sleep til someone else does */
		while ((p->status == CL_FAILED) || (p->status == CL_CLEANUP))
		    sleep ((caddr_t)&clustab[site], P_CLEANUP);
	    }
	}
    }
    p->nak_time = 0;
    bcopy (net_addr, p->net_addr, ADDRESS_SIZE);
    if (IM_SERVER || (site == my_site)) {
	/* cnodes need to keep track of their own lan cards for below tests
	 * and server always keeps track of actual cards */
    	p->lan_card = lan_card;
    }
    else {
	/* Mark to forward if we are not on the same lan card */
	p->lan_card = lan_card == clustab[my_site].lan_card ? 0 : 1;
    }
    p->total_buffers = total_buffers;
    p->cpu_arch = cpu_arch;
    p->cpu_model = cpu_model;
    p->swap_site = swap_site;
    p->site_seqno = 0;
    p->req_count = 0;
    p->req_waiting_Qhead = p->req_waiting_Qtail = 0;
    /* if adding a site other than myself and I am the swap site
     * then I must be a swap server.
     */
    if ((site != my_site) && (swap_site == my_site))
	    my_site_status |= CCT_SWPSERVER;

    return(0);
}
/* count the number of failed sites */
count_failed_sites()
{
	register struct cct *cctp;

	/* must be run at splimp() -- called by add_site at level 5 */
	failed_sites=0;
	for (cctp = clustab+1; cctp < clustab+MAXSITE; cctp++)
		if ((cctp->status == CL_FAILED) || (cctp->status == CL_CLEANUP))
			failed_sites++;
}

/*
 * prepare clustercast destination address (called by dm_send)
 */
clusterlist (list)
struct dm_multisite *list;
{
	register site_t site;
	register struct cct *cctp;
	register struct dm_site *stp;
	register int maxsite;

	bzero (list, sizeof(struct dm_multisite));
	cctp = &(clustab[1]);
	stp = &(list->dm_sites[1]);
	for (site = 1; site < MAXSITE; site++, cctp++, stp++) {
		if ((cctp->status & CCT_STATUS_MASK) == CL_IS_MEMBER) {
			stp->site_is_valid = 1;
			maxsite = site;
		}
		else
			stp->site_is_valid = 0;
	}
	list->maxsite = maxsite;
}

#if defined(__hp9000s700) || defined(__hp9000s800)
u_char *
get_lan_addr(lan_card)
	int lan_card;
{
        struct fis data;
        int error;
        int cmd;
	int oldlevel;
	sv_sema_t ss;
	static u_char address[ADDRESS_SIZE];

        data.reqtype = LOCAL_ADDRESS;
        data.vtype = 6;
        cmd = NETSTAT;
	NETMP_GO_EXCLUSIVE(oldlevel, ss);
        error = (*dux_lan_cards[lan_card]->if_ioctl)
				(dux_lan_cards[lan_card], cmd, &data);
	NETMP_GO_UNEXCLUSIVE(oldlevel, ss);
        if (error != 0)
		printf("Error: ioctl to get my address returned: %d\n",error);
	bcopy(data.value.s, address, ADDRESS_SIZE);
	return(address);
}
#endif /* s700 || s800 */

dux_add_multicast(address)
u_char *address;
{
        struct fis data;
        int error;
        int cmd;
	int oldlevel;
	sv_sema_t ss;
	int i;

	if (dux_lan_cards[0] == (struct ifnet *)0)
		panic("dux_add_multicast: dux_lan_cards[0] is NULL");

	data.reqtype = ADD_MULTICAST;
        data.vtype = 6;
        bcopy(address, data.value.s, ADDRESS_SIZE);
        cmd = NETCTRL;
	for (i = 0; i < num_dux_lan_cards; i++) {
		NETMP_GO_EXCLUSIVE(oldlevel, ss);
        	error = (*dux_lan_cards[i]->if_ioctl)
					(dux_lan_cards[i], cmd, &data);
		NETMP_GO_UNEXCLUSIVE(oldlevel, ss);
		if (error != 0) {
                	printf("Error: ioctl to add-multicast returned: %d for lan card %d\n",error, i);
			num_dux_lan_cards = i;
			break;
		}
	}
}
