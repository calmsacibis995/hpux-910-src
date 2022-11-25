/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/init_main.c,v $
 * $Revision: 1.12.84.8 $       $Author: rpc $
 * $State: Exp $        $Locker:  $
 * $Date: 94/11/03 09:43:44 $
 */

/* HPUX_ID: @(#)init_main.c	55.1		88/12/23 */

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
*/


#include "../h/debug.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/tty.h"
#include "../h/protosw.h"
#include "../ufs/quota.h"
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../h/netfunc.h"
#include "../h/pregion.h"
#include "../h/core.h"

#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/rmswap.h"
#include "../dux/cct.h"
#include "../dux/dux_hooks.h"
#include "../ufs/inode.h"
#include "../h/swap.h"
#ifdef  AUDIT
#include "../h/audit.h"
#endif
#include "../cdfs/cdfs_hooks.h"
#include "../h/ki_calls.h"

#define WAITOK 1
extern	struct user u;		/* have to declare it somewhere! */
extern  int    maxdsiz, maxssiz;

extern int noswap;
extern int freemem;
extern long pmmu_exist;
extern int sys_mem;
extern int sysmem_max;
extern int swapmem_on;
extern int swapmem_max;
extern int swapmem_cnt;
extern int vdma_present;

int	lockmem_init = 0;

/*
** Several of the following definations are done here because 
** the variables are used extensively in functions that must be
** configurable. Since it is not likely that this routine will
** be configured out, it seemed like a good place. (?). If it
** is deemed as an undesirable location to have these scattered
** definations, then they can be easily moved elsewhere. - daveg
*/

/*
** Necessary for clustering operations and basis cnode info.
*/
extern struct cct clustab[MAXSITE];
char my_sitename[DIRSIZ+1];
site_t my_site = 0;
u_int my_site_status = 0;
site_t root_site = 0;
site_t swap_site = 0;
extern struct dux_context dux_context;
extern struct dux_context *build_context();
extern struct swaptab *swapMAXSWAPTAB;

int old_tick = 1000000/HZ;	/* for distributed clock sync */
/*
** Necessary for booting over the lan.
** Must be initialized to force into data space.
*/
u_char bootrom_f_area[6] = "";

/* 
** Necessary for DUX selftest code.
*/
int selftest_passed = 0;

/*
** Others 
*/
char *my_cputype;
extern char my_sitename[];
extern int float_present;
extern site_t root_site;
extern int SetClockFlag;
int clockspl;
int mainentered = 0;
int kstack_reserve = 0;
unsigned int kstack_rptes[KSTACK_RESERVE + KSTACK_PAGES - 2];
extern unsigned int Kstack_toppage[];

/*
 * Create the physical address space for proc 0.
 * init_proc0() is entered on a temporary stack, and u is not mapped.
 * We create a U-area and kernel stack for process 0 here and initialize
 * it. We also attach this U-area and kernel stack to the kernel vas.
 */

p0init()
{
	register struct user *useg;
	register struct user *ruseg;
	register int i;
	register unsigned int *umap_pte;
	reg_t *rp;
	preg_t *prp;
	vfd_t *vfd;
	int sp, pid;
	unsigned int addr;
	struct proc *p;
	unsigned int *ktsmap_pte;

	/*
	 * Hand craft a P0, P1 and U block for the Scheduler.  We allocate
	 * a uarea here since attachreg() expects one.
	 */
	p = &proc[0];
	if ((p->p_vas = allocvas()) == (vas_t *)NULL)
	    panic("p0init: can not allocate vas");
	p->p_vas->va_proc = p;
	vasunlock(p->p_vas);

	/*
	 * attach a stack region and place the u area where it belongs.
	 */
	/* XXX swapdev_vp hasn't been assigned yet, so the second */
	/*  argument is pro forma only ... Andy */
	/* Of course, we probably won't be paging proc 0... */
	if ((rp = allocreg(NULL, swapdev_vp, RT_PRIVATE)) == NULL)
		panic("p0init: allocreg failed");
	if ((grow_p0init(rp, KSTACK_PAGES + UPAGES, DBD_DZERO)) == -1)
		panic("p0init: growreg failed");

	/* PF_NOPAGE will keep vhand from paging us out */

	if ((prp = attachreg(p->p_vas, rp,
			PROT_KERNEL|PROT_READ|PROT_WRITE, PT_UAREA,
			PF_EXACT | PF_NOPAGE, UAREA - (KSTACK_PAGES * NBPG),
			0, KSTACK_PAGES + UPAGES)) == NULL)
		panic("p0init: attachreg failed");

	/* Map u and kernel stack using above allocated pregion */

	for (i = 0; i < (KSTACK_PAGES + UPAGES); i++) {
	    umap_pte = (unsigned int *)itablewalk(Syssegtab, 
						((char *) KSTACKBASE) + i*NBPG);
	    if ((umap_pte = (unsigned int *)vm_alloc_tables(&kernvas, 
			((char *) KSTACKBASE) + i*NBPG)) == NULL)
		panic("Cannot allocate proc 0 page tables");

	    if (i == 0) {

		/* Map Kstack_toppage at top page of kernel stack */

		ktsmap_pte = (unsigned int *)vastopte(&kernvas,Kstack_toppage);
		VASSERT(ktsmap_pte != (unsigned int *)0);
		VASSERT(*ktsmap_pte != 0);
		*umap_pte = *ktsmap_pte;
	    }
	    else {
		if (i < (KSTACK_PAGES - 1)) {

		    /* Allocate common stack pages and map them */
		    /* into kernel.                             */

		    addr = alloc_page(DONT_ZERO_MEM);
		    *umap_pte = addr | PG_RW | PG_CB | PG_V;
		}
		else {

		    /* Allocate private pages to user pregion */
		    /* and then map into kernel.              */

		    fillpreg(prp, i);
		    vfd = findvfd(prp->p_reg, i);
		    *umap_pte = (hiltopfn(vfd->pgm.pg_pfnum) << PGSHIFT)
				| PG_RW | PG_CB | PG_V;
		}
	    }
	}

	/* u and stack are now now mapped! */

	/* Allocate reserve stack pages */

	for (i = 0; i < KSTACK_RESERVE; i++) {
	    addr = alloc_page(DONT_ZERO_MEM);
	    kstack_rptes[i] = addr | PG_RW | PG_CB | PG_V;
	}
	kstack_reserve = KSTACK_RESERVE;

	u.u_procp = p;
	hdl_procattach(&u, p->p_vas, prp);

	p->p_upreg = prp;

	/* 
	 * A Uarea was allocated by us above.  Point our proc structure
	 *  to its PTEs so locore.s can map it.
	 */
	p->p_addr = vtoipte(prp, ptob(KSTACK_PAGES));

	regrele(rp);

	/* Set # of kernel stack pages */

	p->p_stackpages = 1;

	/*
	 * The active list on proc[0] threads the active proc table
	 * entries. The same index is also used to thread the free
	 * proc table entries.
	 */
	p->p_fandx = 0;
	p->p_pandx = 0;
	p->p_stat = SRUN;
	p->p_flag = SLOAD|SSYS;
	p->p_nice = NZERO;
	p->p_memresv = -1;
        p->p_swpresv = -1;
	p->p_maxrss = RLIM_INFINITY/NBPG;
	p->p_coreflags = DEFAULT_CORE_OPTS;
	p->p_ttyp = NULL;
	p->p_ttyd = NODEV;
	p->p_vforkbuf = (struct vforkinfo *)0;
	p->p_msem_info = (struct msem_procinfo *)0;

	/* 
	 * setup start time for process 0, 1, and 2
	 */
	p->p_start = time.tv_sec;


	/*
	 * Fix U area entries.
	 */
#ifdef AUDIT
	u.u_audpath = NULL;
	u.u_audstr = NULL;
	u.u_audsock = NULL;
	u.u_audxparam = NULL;			
	u.u_aid = -1;	
	audit_mode = -1;
#endif
	u.u_cmask = CMASK;

	/* There is an upper limit of MAXFUPLIM open files */
	/* Even if they configure more than that in their
	   gen file, limit them to MAXFUPLIM open files per
	   process. */
	if (maxfiles > MAXFUPLIM)
	  maxfiles = MAXFUPLIM;
	if (maxfiles_lim > MAXFUPLIM)
	  maxfiles_lim = MAXFUPLIM;

	u.u_maxof = maxfiles;	
	u.u_highestfd = -1;	/* no files open */

	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max = 
		    RLIM_INFINITY;
	u.u_rlimit[RLIMIT_STACK].rlim_cur = 
		u.u_rlimit[RLIMIT_STACK].rlim_max = ptob(maxssiz);
	u.u_rlimit[RLIMIT_DATA].rlim_max =
		u.u_rlimit[RLIMIT_DATA].rlim_cur = ptob(maxdsiz);
	u.u_rlimit[RLIMIT_FSIZE].rlim_max =
		u.u_rlimit[RLIMIT_FSIZE].rlim_cur =
		RLIM_INFINITY >> 9;	/* units are 512 byte blocks */
	/* Set up the rlimit current and maximum for RLIMIT_NOFILE */
	u.u_rlimit[RLIMIT_NOFILE].rlim_cur = maxfiles;
	u.u_rlimit[RLIMIT_NOFILE].rlim_max = maxfiles_lim;
	u.u_pcb.pcb_dragon_bank = -1;
	u.u_site = my_site;
	bcopy("standalone",my_sitename,DIRSIZ+1);
	/* set up initial context */
	u.u_cntxp = (char **)build_context(&dux_context);
	/*
	 * Setup credentials
	 */
	u.u_cred = crget();
	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	u.u_rdir = NULL;

	/*
	 * Set up system call information
	 */

	u.u_pcb.pcb_sysent_ptr = sysent;
	u.u_pcb.pcb_nsysent = nsysent;

	return;
}

/*
 * Kernel stack and process 0 initialization code.
 * Called from cold start routine as
 * soon as segmentation has been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *      initialize and link drivers
 *      hand craft process 0
 *      Attach u-area and stack
 *
 *
 * Note: we return to cold start so
 *       that the stack will be fully unwound
 *       before switching to new stack. We
 *       can't just copy the stack since it
 *       might have links on it.
 */

init_proc0(firstaddr)
	int firstaddr;
{
	p1pages = 1048576;	/* 1048576 4k pages = 4 gigabytes */
	usrstack = USRSTACK;
	user_area = UAREA;
	float_area = FLOAT_AREA;
	highpages  = HIGHPAGES;

	rqinit();

	/*
	 * initialize kernel memory allocator
	 *  (really it's just the spinlock, and it's needed before startup() )
	 */
	kmem_init();

	startup(firstaddr);

	/* If we have vdma we must not use indirect pte's. The
	 * indirect_ptes flag must be properly set before we
	 * create any user processes.
	 */

	if (vdma_present)
	    indirect_ptes = 0;

	/* Currently u is not mapped, and we are
	 * running on a temporary stack. p0init
	 * hand crafts a U area for process 0 and
	 * maps it into the kernel virtual address
	 * for u.
	 *
	 * Note: startup initialized devices, however
	 * we must still be at spl6 since the ki code
	 * references the U area upon entering
	 * INTERRUPT_1to6. If we get an interrupt,
	 * trap or non bus-trial fault before this time
	 * the ki code will panic due to u not being
	 * mapped.
	 */

	p0init();

	return;
}

/*
 * Main initialization code.
 * Called from cold start routine
 * after drivers have been linked
 * and process 0 has been initialized.
 * We are now running on the normal
 * kernel stack.
 * Functions:
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 2 to page out
 *	     - process 1 execute bootstrap
 *
 * loop at loc 13 (0xd) in user mode -- /etc/init
 *	cannot be executed.
 */

int skipfsck = 0;		/* can be used to come up faster */
int forkdirs = 0;

main()
{
	register int i;
	register struct proc *p;
	register struct proc *pp;
	extern int freemem_cnt;
	int remoteroot;
        pid_t newpid;
        extern unsigned int highpages_alloced;
        unsigned int temp_physmem;
        

	mainentered = 1;
#ifdef XTERMINAL
	noswap = 1 ;
#endif

	init_freeproc_list();
	freemem_cnt = 0;

	/* Allocate the space for our u.u_ofilep.  This should go into p0init,
	   but since kmem_init() is not called until now...  Maybe we can 
	   move kmem_init() into p0init too (like it is on the 800), until
	   then, my junk has to go here. */
	u.u_ofilep = (struct ofile_t **)kmem_alloc((sizeof(struct ofile_t *)) * NFDCHUNKS(u.u_maxof));
	/* allocate space for our u.u_ofilep[0] */

	u.u_ofilep[0] = (struct ofile_t *)kmem_alloc(sizeof(struct ofile_t));
	bzero((caddr_t)u.u_ofilep[0], sizeof(struct ofile_t));

	for (i=1; i<NFDCHUNKS(u.u_maxof); i++)
	  u.u_ofilep[i] = NULL;
	/*
	 * get vnodes for swapdev
	 */
	swapdev_vp = devtovp(swapdev);


#ifdef CRFIFO
	crinit();
#endif
#ifdef FSD_KI
        ki_init_clocks();
#endif  /* FSD_KI */

	startrtclock();
#include "../h/kg.h"


	/*
	 * Initialize tables, and set up well-known inodes.
	 */
	cinit();			/* needed by dmc-11 driver? */

	/* Note:  mbufs should only be initialized once.  The netfunc entry
	 * is filled in by uipc_init (providing its configured), which
	 * gets called as a result of link_drivers.  We call it here so
	 * we're off the ICS.
	 */
	NETCALL(NET_MBINIT)();	    /* really necessary?? */

	/* init_ntimo MUST be before ifinit and domaininit.  Since it
	 * uses the same memory allocator as MBINIT, they probably should
	 * be kept together (i.e. both expect to be on the kernel stack)
	 */
	NETCALL(NET_NTIMO_INIT)();  

	/*
	 * No need to block interrupts since we are still
	 * at level 6 (We don't drop to 0 until after call
	 * to rootinit().
	 */

	/*
	 *initialize the interface send queues and interface slow timers
	 */
	NETCALL(NET_IFINIT)();

	/*
	 * initialize the domains list, all of the protocols, and the
	 * fast and slow protocol timers
	 */
	NETCALL(NET_DOMAININIT)();

	ihinit();
	CDFSCALL(CDHINIT)();
	bhinit();
	binit();

#ifndef QUIETBOOT
	printf("\nDisk Information:\n");
#endif
 	/*  initialize the root device here in case we try to connect
 	 *  up with a remote root server (need dux_mbufs, etc.).
 	 */
	rootinit();

	/* drop the interrupt level to 0 so timeouts will work */

	spl0();

	bswinit();
	dnlc_init();
	msginit();
	seminit();
	shminit();
	privgrp_init();
#ifdef GPROF
	kmstartup();
#endif

	/*
	 * Initialize the mount hash array
	 */
	mounthashinit();

	/*
	 * for standalone systems, we must look like site 1
	 * so that cnode-specific devices with id 1 will work
	 */
        if (!(my_site_status & CCT_CLUSTERED )) 
                u.u_site = my_site = root_site = swap_site = 1;

	find_root(&remoteroot);
	if (remoteroot)
		skipfsck = 1;
	/*
	 * mount the root, gets rootdir
	 */
	vfs_mountroot(skipfsck ? 0 : VFS_RDONLY);
	swapinit();
	dumpconf();
	/*
	 * Activate kernel virtual memory
	 */
	init_kvm();
	boottime = time;

/* kick off timeout driven events by calling first time */
	if (timeslice == 0)
		timeslice = HZ / 10;	/* default to 1/10 seconds */
	if (timeslice > 0)
	roundrobin();
	sendlbolt();
	schedpaging();
        schedunhash();
        sendsync();

/* set up the root file system */

	/*
	 * Set the memory thresholds.
	 */
	setmemthresholds();

	/*
	 * mpid is used by nodux_getpid() to generate a unique pid
	 * for each process when dux is not configured in the system.
	 * It must be initialized to PID_MAXSYS before the first 
	 * call to getnewpid() so that no process will use any of the
	 * 0 - PID_MAXSYS pids, which are reserved for system processes.
	 */
	mpid = PID_MAXSYS;

	u.u_syscall = KI_SWAPPER;

	/*
	 * make page-out daemon
	 */
	forkdirs++;
	pp = allocproc(S_PAGEOUT);
	proc_hash(pp, 0, PID_PAGEOUT, PGID_NOT_SET, SID_NOT_SET);
	if (newproc(FORK_DAEMON, pp)) {
		/* release root so that it can be unmounted below */
		VN_RELE(u.u_cdir);
		u.u_cdir = NULL;
		u.u_syscall = KI_VHAND;
		pstat_cmd(u.u_procp, "vhand", 1, "vhand");
		forkdirs--;
		wakeup(&forkdirs);
                proc[S_PAGEOUT].p_flag2 |=SANYPAGE;
		vhand();
		/*NOTREACHED*/
	}

	/*
	 * make schedcpu and measurement process
	 */
	forkdirs++;
	pp = allocproc(S_STAT);
	proc_hash(pp, 0, PID_STAT, PGID_NOT_SET, SID_NOT_SET);
	if (newproc(FORK_DAEMON, pp)) {
		/* release root so that it can be unmounted below */
		VN_RELE(u.u_cdir);
		u.u_cdir = NULL;
		u.u_syscall = KI_STATDAEMON;
		pstat_cmd(u.u_procp, "statdaemon", 1, "statdaemon");
		forkdirs--;
		wakeup(&forkdirs);
                proc[S_STAT].p_flag2 |=SANYPAGE;
		schedcpu();
		/*NOTREACHED*/
	}

        /*
         * make unhashpage daemon
         */
        forkdirs++;
        newpid = getnewpid();
        pp = allocproc(S_DONTCARE);
        proc_hash(pp, 0, newpid, PGID_NOT_SET, SID_NOT_SET);
        if (newproc(FORK_DAEMON, pp)) {
                char *unhash_name = "unhashdaemon";

                /* release root so that it can be unmounted below */
                VN_RELE(u.u_cdir);
                u.u_cdir = NULL;
		u.u_syscall = KI_UNHASHDAEMON;
                forkdirs--;
                wakeup(&forkdirs);
                pp->p_flag2 |=SANYPAGE;
                pstat_cmd(u.u_procp, unhash_name, 1, unhash_name);
                unhashdaemon();
                /*NOTREACHED*/
        }

	/* run fsck before starting init */
	if (!skipfsck) {
		extern int fcode[];
		extern int szfcode;
		pid_t pid;
		u_int save_my_site_status;

		pid = getnewpid();
		pp = allocproc(S_DONTCARE);
		proc_hash(pp, 0, pid, pid, pid);
		if (newproc(FORK_PROCESS, pp)) {
			/* Temporarily include localroot context to help*/
			/* both standalone and server to find console.	*/
			/* Server's /dev context must be localroot for	*/
			/* this to work. The fcode as is will _not_ work*/
			/* on a cnode (remoteroot) as we wouldn't be	*/
			/* able to figure it's context for console.	*/
			save_my_site_status = my_site_status;
			my_site_status |= CCT_ROOT;
			u.u_cntxp = (char **) build_context(&dux_context);

			/* start up fsck process to check root */
			proc_start((caddr_t)fcode, szfcode, pp);
			return;
		}
		/* Wait for fsck to finish... */
		if (wait1(pid, 0, (struct rusage *)0))
			panic("init_main: ECHILD"); 

		/* Restore previous context */
		my_site_status = save_my_site_status;
		u.u_cntxp = (char **) build_context(&dux_context);

		/* release root working directory to allow unmount */
		while (forkdirs)
			sleep((caddr_t)&forkdirs, PRIBIO);
		/* this is a little bit bogus, because I'm not clearing
		 * the pointers associate with these holds.  It's OK,
		 * though, because vfs_rootmount will reset the pointers
		 * when it remounts the root.
		 */
		VN_RELE(rootdir);
		VN_RELE(rootdir);
		/* this clear is a little easier to justify */
		VN_RELE(u.u_cdir);
		u.u_cdir = NULL;

		while (i = umount_dev(rootdev)) {
			printf("unmount1: err = %d rootdir %x\n", i, rootdir);
			sleep((caddr_t)&lbolt, PRIBIO);
		}
		rootdir = NULL;

		vfs_mountroot(0);
	}

	/*
	 * make init
	 */
	
	pp = allocproc(S_INIT);
	proc_hash(pp, 0, PID_INIT, PGID_NOT_SET, SID_NOT_SET);
	if (newproc(FORK_PROCESS, pp)) {
		/* start up init */
		proc_start(icode, szicode, pp);
		return;
	}

	/*
	 * Initialize Netipc.
	 */
	NETCALL(NET_NIPCINIT)();

	/*
	 * Initialize the netisr process.  Must happen after domaininit!
	 * We don't care what proc table entry we get, so if others
	 * do, newproc before us!
	 */
	NETCALL(NET_NETISR_DAEMON)();

	/*
	 * Initialize streams scheduler, memory, and weld processes.
	 * We don't care what proc table entry we get either.
	 */
	NETCALL(NETSTR_SCHED)();
	NETCALL(NETSTR_MEM)();
        NETCALL(NETSTR_WELD)();

	/*
	 *if we have joined a cluster, create a limited nsp
	 */
	if (my_site_status & CCT_CLUSTERED) {
		pp = allocproc(S_DONTCARE);
		proc_hash(pp, 0, PID_LCSP, PGID_NOT_SET, SID_NOT_SET);
		if (newproc(FORK_DAEMON, pp)) {
			u.u_syscall = KI_LCSP;
			pstat_cmd(u.u_procp, "lcsp", 1, "lcsp");
			DUXCALL(LIMITED_NSP)();
			/*NOTREACHED*/
		}
	}
	/*
	 *if we have a GENESIS vdma card, start the vdma daemon
	 */
	if (vdma_present) {
		pp = allocproc(S_DONTCARE);
		proc_hash(pp, 0, PID_VDMAD, PGID_NOT_SET, SID_NOT_SET);
		if (newproc(FORK_DAEMON, pp)) {
 			/*
			 * Put vdmad on the correct hash chains.
 			 */
			u.u_syscall = KI_VDMAD;
			vdmad();
			/*NOTREACHED*/
		}
	}
        syncd_fork();

#ifdef AUDIT
	if (my_site != root_site)
		DUXCALL(GETAUDSTUFF)();
#endif

#ifndef QUIETBOOT
	printf("\nMemory Information:\n");
#endif
	if (ptob(freemem) <= 102400)
		printf("WARNING: available memory too low, may deadlock\n");

	/* 
	 * lockable_mem (like most other variables) is in units of pages
	 * for convenience/efficiency inside the kernel.  unlockable_mem is
	 * in units of bytes for user convenience, since it is configurable.
	 */
	total_lockable_mem = freemem - btop(unlockable_mem);
	if ((swapmem_on) && (total_lockable_mem > swapmem_cnt))
                total_lockable_mem = swapmem_cnt;
	if (total_lockable_mem < 0)
		total_lockable_mem = 0;
	lockable_mem = total_lockable_mem;
	lockmem_init = 1;
	/*
	 *  The bootrom takes 2 pages (top page + a few bytes from the
	 *  bottom page for driver data or something), and we've also
	 *  subtracted the kernel page tables from physmem (is that good?)
	 */
	temp_physmem = physmem + highpages_alloced + 2;	
	if (temp_physmem & 0xff)	/*  something else took memory  */
		temp_physmem = physmem;
#ifndef QUIETBOOT
	printf("    Physical: %d Kbytes", ptob(temp_physmem) >>10 );
        printf(", lockable: %d Kbytes",ptob(lockable_mem) >>10);
        printf(", available: %d Kbytes\n\n", ptob(freemem) >>10);
#endif

        if (orignbuf || origbufpages)
          msg_printf("    Using %d buffers containing %d Kbytes of memory.\n",
                nbuf, (bufpages * NBPG) >> 10);

/*
 * Disk Quotas initialization
 */
        qtinit();

	pstat_cmd(u.u_procp, "swapper", 1, "swapper");
	sched();
}

proc_start(addr, size, pp)
	caddr_t addr;
	int size;
	struct proc * pp;
{
	register reg_t *rp;
	register preg_t *prp;

	rp = allocreg(NULL, swapdev_vp, RT_PRIVATE);
	prp = attachreg(u.u_procp->p_vas, rp, PROT_URWX,
		PT_DATA, PF_EXACT, 0, 0, 0);
	hdl_procattach(&u, u.u_procp->p_vas, prp);
	if (growpreg(prp, btorp(size), btorp(size),
			DBD_DZERO, ADJ_REG) < 0)
		panic("main - init growpreg");
	regrele(rp);
	if (pmmu_exist || (processor == M68040))
		set_crp(pp->p_segptr);
	else
		*((u_long *) USEGPTR) = 
		(u_long) (((u_long) pp->p_segptr) >> PGSHIFT);
	(void) copyout(addr, (caddr_t)0, (unsigned)size);

	if (processor == M68040)
		/* copyout put the text in the data cache, better push it out */
		purge_dcache_physical();

#ifdef  FSD_KI
        VASSERT(KI_CLK_TOS_STACK_PTR == KI_CLK_BEGINNING_STACK_ADDRS);

        /* need to fix stack in this special case */
        *KI_CLK_TOS_STACK_PTR = KT_USR_CLOCK;
#endif /* FSD_KI */
	/*
	 * Return goes to loc. 0 of user init
	 * code just copied out.
	 */
}


/*
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
bswinit()
{
	register int i;
	register struct buf *sp = swbuf;

	bswlist.av_forw = sp;
	for (i=0; i<nswbuf-1; i++, sp++)
		sp->av_forw = sp+1;
	sp->av_forw = NULL;
}

/*
 * Initialize clist by freeing all character blocks.
 */

cinit()
{
	register n;
	register struct cblock *cp;

	for(n = 0, cp = &cfree[0]; n < nclist; n++, cp++) {
		cp->c_next = cfreelist.c_next;
		cfreelist.c_next = cp;
	}
	cfreelist.c_size = CBSIZE;
}

swapinit()
{
	register int i, retried;
        register dm_message reqp, resp;
        register int * pdmmax;
        struct swaptab *st;
        swpdbd_t swappg;

	if (noswap)
		return;

	vm_initsema(&swap_lock, 1, SWAP_LOCK_ORDER, "swap sema");
	if (!(my_site_status & CCT_SLWS)) {
                swapconf();

		/* If the first (primary) entry of the swdevt has no
		 * blocks associated with it      OR
		 * if a call to enable swap using swfree() fails,
		 * then set noswap (nothing to swap to at this time).
		 */
                if (!swdevt[0].sw_nblks || !swfree(0,0,"Primary Swap")) {
                        noswap = 1;
			dbc_set_ceiling ();
		}
        }
        else {
		res_mbuf_init();
                swdevt[0].sw_head = -1;
        }

        devswap_init();
	swapMAXSWAPTAB = &swaptab[maxswapchunks];
        if (minswapchunks > maxswapchunks)
                minswapchunks = maxswapchunks;

        /* get the first chunks */
        retried = 0;
        for(i=0; i<minswapchunks; i++) {
		if (my_site_status & CCT_SLWS) {
                        if (!dux_swaprequest()) {
                                printf("System does not have MINSWAPCHUNKS of swap space available--continuing anyway\n");
                        }
                }
		else {
                        if (noswap)
                                break;

                        while(!get_swap(NPGCHUNK, &swappg, 0)) {
                                if (retried == 0) {
                                   printf("Cannot get mininum swap space from swap server\n");
                                   printf("System will retry until swap is available\n");
                                   retried = 1;
				retried = 1;

                                }
                        }
                        st = &swaptab[swappg.dbd_swptb];
                        st->st_free = 0;
		}
        }

	/*
	 * now that we have some real swap space, give
	 * up some system memory.
	 */

	if (swapmem_cnt - sysmem_max > 0) {
		swapmem_cnt -= sysmem_max;
		swapmem_max = swapmem_cnt;
		sys_mem += sysmem_max;
	}
}
/*
 *cluster system call.
 *
 * It is used for two purposes:
 * 1. Set site_id and site_name for the standalone machines without making
 *    them clustered. This operation is invoked in the /etc/init program. 
 * 2. Make a standalone machine clustered. This operation is done through
 *    the cluster command. 
 */ 
cluster()
{
	register struct a {
		u_int id;
		char *name;
		char *lan_id;
		} *uap = (struct a *) u.u_ap;

	extern site_t my_site;
	extern site_t root_site;
	extern site_t swap_site;
	int dux_nop();
	static int lan_card_error = 0;	/* should be a bit in my_site_status */
#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_str(uap->name);
	}
#endif /* AUDIT */

	if (!suser())
		return;

	if (lan_card_error) {
		u.u_error = EINVAL;
		return;
	}

	if ((my_site_status & CCT_STARTED) == 0) {	/* /etc/init */
		/*
		 * The following check will cause /etc/cluster to fail
		 * If an "old" (pre-cluster) /etc/init is used.
		 */
                if (uap->id == 0) {             /* could be /etc/cluster */
                        u.u_error = EINVAL;
                        return;
                }

		my_site_status |= CCT_ROOT;	/* for build_context */

		/* get cnode name */
		if ((u.u_error = copyin(uap->name,my_sitename,DIRSIZ+1)) != 0)
			strcpy(my_sitename,"standalone");

		u.u_cntxp = (char **) build_context(&dux_context);

		if (!((0 < uap->id) && (uap->id <= MAXSITE))) {
			uap->id = 1;
			u.u_error = EINVAL;
		}

		my_site = uap->id;			/* get cnode id */
		u.u_site = uap->id;
		root_site = uap->id;
		swap_site = uap->id;

		if ((u.u_error = DUXCALL(DUX_LAN_ID_CHECK)(uap->lan_id)) != 0)
			lan_card_error = 1;
		else
			my_site_status |= CCT_STARTED;
		return;
	}
	if ((my_site_status & CCT_CLUSTERED) == 0) {	/* /etc/cluster */
		if (duxproc[ROOT_CLUSTER] == dux_nop) {
			u.u_error = ENODEV;
			return;
		}
		if ((u.u_error = DUXCALL(ROOT_CLUSTER)()) != 0)
			return;
		my_site_status |= CCT_CLUSTERED;
		return;
	}
	u.u_error = EINPROGRESS;
	return;
}
/*
 * can't call growreg() since no swap space available yet
 */
int
grow_p0init(rp, amount, type)
        reg_t *rp;
        int amount;
        int type;
{
        dbd_t *dbd;
        struct broot *br;

        /* Initial growth--allocate a btree */
        VASSERT(rp->r_root == NULL);
        br = rp->r_root = bt_init(rp);
        dbd = (dbd_t *)default_dbd(rp, 0);
        dbd->dbd_type = type;
        dbd->dbd_data = DBD_DINVAL;
	rp->r_pgsz += amount;
        return(1);
}
