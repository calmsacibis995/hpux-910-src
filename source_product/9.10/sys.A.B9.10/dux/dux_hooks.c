/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_hooks.c,v $
 * $Revision: 1.11.83.3 $        $Author: root $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 16:41:01 $
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
#include "../h/systm.h"
#include "../h/syscall.h"
#ifdef __hp9000s300
#include "../s200io/lnatypes.h"
#endif
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_hooks.h"

#include "../h/socket.h"
#ifdef __hp9000s300
#include "../wsio/timeout.h"
#include "../wsio/hpibio.h"
#include "../s200io/drvhw.h"
#endif
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

#if defined(__hp9000s700) || defined(__hp9000s800)
#include "../sio/llio.h"
#endif
#include "../sio/lanc.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../dux/duxparam.h"
#include "../dux/cct.h"
#include "../h/vnode.h"
#include "../h/netfunc.h"

extern struct vnodeops dux_vnodeops;

/*
** These functions are defined external here since
** they are not directly referenced anywhere except
** when the DUX code is configured into the kernel.
*/
extern int (*duxproc[])();
extern	int dm1_alloc();
extern	int dm1_release();
extern	int dm1_send();
extern	int dm1_reply();
extern	int dm1_quick_reply();
extern	int dm1_wait();
extern	int dm1_recv_reply();
extern	int dm1_recv_request();
extern	int senddmsig1();

#if defined(__hp9000s700) || defined(__hp9000s800)
extern	int mpduxintr();
extern	int netisr_priority;
int dux_netisr_priority;
#endif
extern	int dskless_initialized;

/*
** This is the driver (pseudo) link routine as defined in
** conf.c for DUX. There really isn't a driver that we are
** configuring out, but rather we are using the link routine
** all by itself to serve as a means of configuring out portions
** of the DUX code. The idea is that all of the pertinent portions
** of DUX are stubbed out in the kernel resident code by indirect
** procedure calls to nop functions.(see funcentry and duxproc
** arrays of pointers). If DUX is configured in, those
** nop calls are replaced by the actual functions to be called
** at boot time. - daveg
**
*/
#if defined(_WSIO) || defined(__hp9000s300)
dskless_link() /* s300, s700 */
#else
dskless_init() /* s800 */
#endif
{

/*
** DUX functions that are actually in the LAN driver. The configurable
** diskless code fills in the lanproc[] array of pointers. This way
** dskless is configurable separate from LAN.
*/

#ifdef __hp9000s300
	int skip_frame();			/* 16 */
	int hw_send_dux();			/* 17 */
	int dux_copyin_frame();			/* 18 */
	int lla_open();				/* 20 */
#endif

	int dux_recv_routines();
	int dux_hw_send();
	int dux_lanift_init();

/*
** Stuff in dm.c (distribution manager interface)
** Additional functions are defined above.
*/
	int find_root();


/*
** Stuff that is scattered around the kernel that tries to
** make use of the DUX transport. They need to be stubbed out for
** a standalone system.
*/
	int broadcast_failure();
	int ws_cluster();
	int req_settimeofday();
	int limited_nsp();
	int dux_strategy();
	int brelse_to_net();
	int dux_lan_check();
	int root_cluster();
	int kill_limited_nsp();
#ifdef AUDIT
	int dux_setevent();
	int dux_audctl();
	int dux_audoff();
	int dux_getaudstuff();
	int dux_swaudfile();
	int cluster_swaudfile();
#endif
	int close_send();
	int fsctl_serve();


/*
** Stuff in funcentry.c (serving side functions specific to DUX)
**
**		There are additional entries that are specific
**		to NFS and DUX interactions that are handled separately.
*/
	int signalnsp();
	int serve_clusterreq();
	int serve_add_member();
	int invoke_cleanup();
	int rds_serve();
	int dux_pathrecv();
	int servestratread();
	int servestratwrite();
	int servestratsync();
	int close_serve();
	int dux_getattr_serve();
	int dux_setattr_serve();
	int servesync();
	int serve_ref_update();
	int openp_wait_recv();
	int fifo_flush_recv();
	int pipe_recv();
	int send_mount_table();
	int unpack_vfs();
	int serve_ufs_mount();
	int umount_dev_serve();
	int mount_commit();
	int global_umount_serve();
	int servesyncdisc();
	int record_failure();
	int serve_settimeofday();
	int serve_sync_req();
	int serve_getpids();
	int servelsync();
	int servefsync();
	int serve_chunkalloc();
	int serve_chunkfree();
	int serve_text_change();
	int xumount_recv();
	int xrele_recv();
	int dux_ustat_serve();
	int register_alive();
	int dux_symlink_recv();
	int dux_rename_recv();
	int dux_fstatfs_serve();
	int dux_lockf();
	int dux_procstat();
	int dux_unlockf();
	int dux_lockwait();
	int dux_ino_update();
	int lockmount_serve();
#ifdef ACLS
	int dux_setacl_serve();
	int dux_getacl_serve();
#endif
#ifdef	POSIX
	int dux_fpathconf_serve();
#endif
#ifdef AUDIT
	int dux_setevent_serve();
	int dux_audctl_serve();
	int dux_audoff_serve();
	int dux_getaudstuff_serve();
	int dux_swaudfile_serve();
	int cluster_swaudfile_serve();
#endif /* AUDIT */
	int fsctl_serve();
	int dux_fpathconf();
	int dux_getattr();
	int dux_pathsend();

/*
** Stuff in init_sent.c (system calls specific to DUX)
** If DUX isn't configured in, then the DUX specific
** system calls should return as an invalid sys call and
** dump core.
*/


#ifdef SETCONTEXT_SUPPORTED
	int	setcontext();
#endif
#ifdef FULLDUX
	int     mkrnod();
	int 	pipenode();
	int 	bigio();
	int 	rmtprocess();
#endif /* FULLDUX */
	int 	nsp_init();
	int 	unsp_open();
	int	sitels();
	int	swap_clients();
#ifdef FULLDUX
	int	remount();
#endif
	int 	dskless_stats();

#if defined(__hp9000s300) && defined(OSDEBUG)
	int 	test();
#endif

/*
** Routines for Disk Quotas.
*/

#ifdef QUOTA
	int	serve_quotactl();
        int     quota_mnt_update();
#endif

/*
** Set flag indicating that system has Diskless configured in.
*/

	dskless_initialized = 1;

/*
** 1st fill in the system entry table slots.
** These are DUX specific system calls.
*/

	sysent_assign( SYS_NSP_INIT,	2,	nsp_init);	/* 168 */
	sysent_assign( SYS_UNSP_OPEN,	0,	 unsp_open);	/* 172 */
	sysent_assign( SYS_SITELS,	1,	 sitels);	/* 181 */
	sysent_assign( SYS_SWAPCLIENTS,	1,	 swap_clients);	/* 182 */
	sysent_assign( SYS_DSKLESS_STATS,4,	 dskless_stats);/* 184 */

#if defined(__hp9000s300) && defined(OSDEBUG)
	sysent_assign( SYS_TEST,	2,	 test);		/* 171 */
#endif

#ifdef SETCONTEXT_SUPPORTED
	sysent_assign( SYS_SETCONTEXT,	1,	 setcontext);	/* 175 */
#endif
#ifdef FULLDUX
	sysent_assign( SYS_BIGIO,	6,	 bigio);	/* 176 */
	sysent_assign( SYS_PIPENODE,	1,	 pipenode);	/* 177 */
	sysent_assign( SYS_RMTPROCESS,	6,	 rmtprocess);	/* 183 */
#endif /* FULLDUX */

/*
** 2nd Fill in the dm layer functions.
** These are the interface routines to the DUX dm layer.
*/

#ifdef __hp9000s300
	duxproc[HW_SEND_DUX]	    = hw_send_dux;		/* 17 */
	duxproc[DUX_COPYIN_FRAME]   = dux_copyin_frame;		/* 18 */
	duxproc[LLA_OPEN]           = lla_open;			/* 20 */
#endif /* hp9000s300 */

	duxproc[DM_ALLOC]		= dm1_alloc;
	duxproc[DM_RELEASE]		= dm1_release;
	duxproc[DM_SEND]		= dm1_send;
	duxproc[DM_REPLY]		= dm1_reply;
	duxproc[DM_QUICK_REPLY]		= dm1_quick_reply;
	duxproc[DM_WAIT]		= dm1_wait;
	duxproc[DM_RECV_REPLY]		= dm1_recv_reply;
	duxproc[DM_RECV_REQUEST]	= dm1_recv_request;
	duxproc[SENDDMSIG]		= senddmsig1;
	duxproc[FIND_ROOT]		= find_root;


/*
** 3rd fill in the misc. functions used by DUX.
** These are function calls that are generally scattered
** through the DUX kernel.
*/
	duxproc[BROADCAST_FAILURE]	= broadcast_failure;
	duxproc[WS_CLUSTER]		= ws_cluster;
	duxproc[REQ_SETTIMEOFDAY]	= req_settimeofday;
	duxproc[LIMITED_NSP]		= limited_nsp;
	duxproc[DUX_STRATEGY]		= dux_strategy;
	duxproc[BRELSE_TO_NET]		= brelse_to_net;
	duxproc[DUX_LAN_ID_CHECK]	= dux_lan_check;
	duxproc[ROOT_CLUSTER]		= root_cluster;
	duxproc[KILL_LIMITED_NSP]	= kill_limited_nsp;
#ifdef AUDIT
	duxproc[SETEVENT]		= dux_setevent;
	duxproc[AUDCTL]			= dux_audctl;
	duxproc[AUDOFF]			= dux_audoff;
	duxproc[GETAUDSTUFF]		= dux_getaudstuff;
	duxproc[SWAUDFILE]		= dux_swaudfile;
	duxproc[CL_SWAUDFILE]		= cluster_swaudfile;
#endif
	duxproc[DUX_CLOSE_SEND]		= close_send;
	duxproc[DUX_FPATHCONF] 		= dux_fpathconf;
	duxproc[DUX_GETATTR]		= dux_getattr;
	duxproc[DUX_PATHSEND]		= dux_pathsend;
	/* moved here from what used to be lanproc initialization */
	duxproc[DUX_LANIFT_INIT]	= dux_lanift_init;
#ifdef __hp9000s300
	duxproc[DUX_RECV_ROUTINES]	= dux_recv_routines;
	duxproc[DUX_HW_SEND]		= dux_hw_send;
#endif

/*
** 4th fill in the dux_strategy routine into the vnodeops
** structure of pointers to functions.
*/
	dux_vnodeops.vn_strategy  = dux_strategy;


/*
** 5th fill in the funcentry serving side functions that
** are specific	to DUX (Discless) functionality.
**
** NOTE: Most of these functions are not actually configured out.
**	 The ROI didn't seem to be sufficient for the scattered
**	 functions. However, they are all treated as if they are
**	 configurable, and it time allows towards the end of the
**	 project, they will be removed. - daveg
*/


	dm_funcentry_assign(0,
	            DM_UNUSED, NULL);		 /*entry 0 is unused*/
	dm_funcentry_assign(DMSIGNAL,
	            DM_INTERRUPT, signalnsp);		  	/*01*/
	dm_funcentry_assign(DM_CLUSTER,
		    DM_KERNEL, serve_clusterreq); 		/*02*/
	dm_funcentry_assign(DM_ADD_MEMBER,
	            DM_KERNEL|DM_LIMITED,serve_add_member);	/*03*/
	dm_funcentry_assign(DM_READCONF,
		    DM_USER, NULL);				/*04*/
	dm_funcentry_assign(DM_CLEANUP,
		    DM_KERNEL, invoke_cleanup);			/*05*/
	dm_funcentry_assign(DMNDR_READ,
		    DM_KERNEL,rds_serve);			/*06*/
	dm_funcentry_assign(DMNDR_WRITE,
		    DM_KERNEL,rds_serve);			/*07*/
	dm_funcentry_assign(DMNDR_OPEND,
		    DM_KERNEL,rds_serve);			/*08*/
	dm_funcentry_assign(DMNDR_CLOSE,
		    DM_KERNEL,rds_serve);			/*09*/
	dm_funcentry_assign(DMNDR_IOCTL,
		    DM_KERNEL,rds_serve);			/*10*/
	dm_funcentry_assign(DMNDR_SELECT,
		    DM_KERNEL,rds_serve);			/*11*/
	dm_funcentry_assign(DMNDR_STRAT,
		    DM_KERNEL,rds_serve);			/*12*/
	dm_funcentry_assign(DMNDR_BIGREAD,
		    DM_USER, NULL);				/*13*/
	dm_funcentry_assign(DMNDR_BIGWRITE,
		    DM_USER, NULL);				/*14*/
	dm_funcentry_assign(DMNDR_BIGIOFAIL,
		    DM_USER, NULL);				/*15*/
	dm_funcentry_assign(DM_LOOKUP,
		    DM_KERNEL,dux_pathrecv);			/*16*/
	dm_funcentry_assign(DMNETSTRAT_READ,
		    DM_KERNEL,servestratread);			/*17*/
	dm_funcentry_assign(DMNETSTRAT_WRITE,
		    DM_KERNEL,servestratwrite);			/*18*/
	dm_funcentry_assign(DMSYNCSTRAT_READ,
		    DM_KERNEL,servestratsync);			/*19*/
	dm_funcentry_assign(DMSYNCSTRAT_WRITE,
		    DM_KERNEL,servestratsync);			/*20*/
	dm_funcentry_assign(DM_CLOSE,
		    DM_KERNEL,close_serve);			/*21*/
	dm_funcentry_assign(DM_GETATTR,
		    DM_KERNEL,dux_getattr_serve);		/*22*/
	dm_funcentry_assign(DM_SETATTR,
		    DM_KERNEL,dux_setattr_serve);		/*23*/
	dm_funcentry_assign(0,
		    DM_UNUSED,NULL);				/*24*/
	dm_funcentry_assign(DM_SYNC,
		    DM_KERNEL, servesync);			/*25*/
	dm_funcentry_assign(DM_REF_UPDATE,
		    DM_KERNEL,serve_ref_update);		/*26*/
	dm_funcentry_assign(DM_OPENPW,
		    DM_KERNEL, openp_wait_recv);		/*27*/
	dm_funcentry_assign(DM_FIFO_FLUSH,
		    DM_KERNEL, fifo_flush_recv);		/*28*/
	dm_funcentry_assign(DM_PIPE,
		    DM_KERNEL, pipe_recv);			/*29*/
	dm_funcentry_assign(DM_GETMOUNT,
		    DM_KERNEL, send_mount_table);		/*30*/
	dm_funcentry_assign(DM_INITIAL_MOUNT_ENTRY,
		    DM_INTERRUPT, unpack_vfs);			/*31*/
	dm_funcentry_assign(DM_MOUNT_ENTRY,
		    DM_KERNEL|DM_LIMITED, unpack_vfs);		/*32*/
	dm_funcentry_assign(DM_UFS_MOUNT,
		    DM_KERNEL, serve_ufs_mount);		/*33*/
	dm_funcentry_assign(DM_COMMIT_MOUNT,
		    DM_KERNEL|DM_LIMITED, mount_commit);	/*34*/
	dm_funcentry_assign(DM_ABORT_MOUNT,
		    DM_KERNEL|DM_LIMITED, mount_commit);	/*35*/
	dm_funcentry_assign(DM_UMOUNT_DEV,
		    DM_KERNEL|DM_LIMITED, umount_dev_serve);	/*36*/
	dm_funcentry_assign(DM_UMOUNT,
		    DM_KERNEL|DM_LIMITED, global_umount_serve);	/*37*/
	dm_funcentry_assign(DM_SYNCDISC,
		    DM_KERNEL|DM_LIMITED, servesyncdisc);	/*38*/
	dm_funcentry_assign(DM_FAILURE,
		    DM_INTERRUPT, record_failure);		/*39*/
	dm_funcentry_assign(DM_SERSETTIME,
		    DM_KERNEL|DM_LIMITED, serve_settimeofday);	/*40*/
	dm_funcentry_assign(DM_SERSYNCREQ,
		    DM_INTERRUPT, serve_sync_req);		/*41*/
	dm_funcentry_assign(0,
	            DM_UNUSED, NULL);		/*entry 42 is unused*/
	dm_funcentry_assign(DM_GETPIDS,
		    DM_KERNEL|DM_LIMITED, serve_getpids);	/*43*/
	dm_funcentry_assign(DM_RELEASEPIDS,
		    DM_KERNEL|DM_LIMITED, serve_getpids);	/*44*/
	dm_funcentry_assign(DM_LSYNC,
		    DM_KERNEL|DM_LIMITED, servelsync);		/*45*/
	dm_funcentry_assign(DM_FSYNC,
		    DM_KERNEL|DM_LIMITED, servefsync);		/*46*/
	dm_funcentry_assign(DM_CHUNKALLOC,
		    DM_KERNEL, serve_chunkalloc);		/*47*/
	dm_funcentry_assign(DM_CHUNKFREE,
		    DM_KERNEL, serve_chunkfree);		/*48*/
	dm_funcentry_assign(DM_TEXT_CHANGE,
		    DM_KERNEL, serve_text_change);		/*49*/
	dm_funcentry_assign(DM_XUMOUNT,
		    DM_KERNEL|DM_LIMITED, xumount_recv);	/*50*/
	dm_funcentry_assign(DM_XRELE,
		    DM_KERNEL|DM_LIMITED, xrele_recv);		/*51*/
	dm_funcentry_assign(DM_USTAT,
		    DM_KERNEL, dux_ustat_serve);		/*52*/
	dm_funcentry_assign(DM_RMTCMD,
		    DM_USER, NULL);				/*53*/
	dm_funcentry_assign(DM_ALIVE,
		    DM_INTERRUPT, register_alive);		/*54*/
	dm_funcentry_assign(0,
		    DM_UNUSED, NULL);				/*55*/
	dm_funcentry_assign(DM_SYMLINK,
		    DM_KERNEL, dux_symlink_recv);		/*56*/
	dm_funcentry_assign(DM_RENAME,
		    DM_KERNEL, dux_rename_recv);		/*57*/
	dm_funcentry_assign(DM_FSTATFS,
		    DM_KERNEL, dux_fstatfs_serve);		/*58*/
	dm_funcentry_assign(DM_LOCKF,
		    DM_KERNEL, dux_lockf);			/*59*/
	dm_funcentry_assign(DM_PROCLOCKF,
		    DM_KERNEL|DM_LIMITED, dux_procstat);	/*60*/
	dm_funcentry_assign(DM_LOCKWAIT,
		    DM_KERNEL, dux_lockwait);			/*61*/
	dm_funcentry_assign(DM_INOUPDATE,
		    DM_KERNEL|DM_LIMITED, dux_ino_update);	/*62*/
	dm_funcentry_assign(DM_UNLOCKF,
		    DM_KERNEL|DM_LIMITED, dux_unlockf);		/*63*/
	dm_funcentry_assign(DM_MARK_FAILED,
		    DM_INTERRUPT, record_failure);		/*67*/
	dm_funcentry_assign(DM_LOCK_MOUNT,
		    DM_KERNEL, lockmount_serve);		/*68*/
	dm_funcentry_assign(DM_UNLOCK_MOUNT,
		    DM_INTERRUPT, lockmount_serve);		/*69*/
#ifdef ACLS
	dm_funcentry_assign(DM_SETACL,
		    DM_KERNEL, dux_setacl_serve);		/*70*/
	dm_funcentry_assign(DM_GETACL,
		    DM_KERNEL, dux_getacl_serve);		/*71*/
#endif
#ifdef	POSIX
	dm_funcentry_assign(DM_FPATHCONF,
		    DM_KERNEL, dux_fpathconf_serve);		/*72*/
#endif
#ifdef AUDIT
	dm_funcentry_assign(DM_SETEVENT,
		    DM_KERNEL, dux_setevent_serve);		/*73*/
	dm_funcentry_assign(DM_AUDCTL,
		    DM_KERNEL, dux_audctl_serve);		/*74*/
	dm_funcentry_assign(DM_AUDOFF,
		    DM_KERNEL, dux_audoff_serve);		/*75*/
	dm_funcentry_assign(DM_GETAUDSTUFF,
		    DM_KERNEL, dux_getaudstuff_serve); 		/*76*/
	dm_funcentry_assign(DM_SWAUDFILE,
		    DM_KERNEL, dux_swaudfile_serve);		/*77*/
	dm_funcentry_assign(DM_CL_SWAUDFILE,
		    DM_KERNEL, cluster_swaudfile_serve);	/*78*/
#endif /* AUDIT */

	dm_funcentry_assign(DM_FSCTL, DM_KERNEL, fsctl_serve);	/*79*/
#ifdef	QUOTA
	dm_funcentry_assign(DM_QUOTACTL, DM_KERNEL, serve_quotactl); /*80*/
	dm_funcentry_assign(DM_QUOTAONOFF,
                    DM_KERNEL|DM_LIMITED, quota_mnt_update);  /*81*/
#endif
#if defined(__hp9000s700) || defined(__hp9000s800)
	/*
	 * Series 700 and Series 800 use netisr for some diskless
	 * traffic.
	 */
	netproc_assign(NET_DUXINTR, mpduxintr);
	dux_netisr_priority = -1;
	/* netisr_priority = dux_netisr_priority; */
#endif /* s700 || s800 */
}
