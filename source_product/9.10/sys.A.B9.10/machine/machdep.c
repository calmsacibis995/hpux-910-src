/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/machdep.c,v $
 * $Revision: 1.12.84.16 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 95/01/16 16:49:13 $
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


#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/cpu.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/callout.h"
#include "../dux/dux_hooks.h"
#include "../dux/rmswap.h"
#include "../h/vfs.h"
#include "../dux/dux_dev.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#include "../ufs/quota.h"
#include "../h/utsname.h"
#include "../ufs/fs.h"

#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../h/netfunc.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../wsio/hpibio.h"
#include "../wsio/intrpt.h"
#include "../s200io/bootrom.h"
#include "../wsio/iobuf.h"
#include "../wsio/dilio.h"
#include "../machine/sendsig.h"

#include "../machine/trap.h"
#include "../h/reboot.h"
#include "../dux/duxparam.h"

#include "../h/uio.h"
#include "../dux/cct.h"

#include "../h/swap.h"
#include "../h/pfdat.h"

extern site_t my_site;
int restart_writebacks();

#ifdef OSDEBUG
#include "../machine/vmparam.h"
#endif OSDEBUG

/*
 * Declare these as initialized data so we can patch them.
 */
extern  int dskless_fsbufs;
extern 	int parity_option, trans_parity;
int	nswbuf = 0;
int	structprocsz = sizeof(struct proc);
extern	int	dos_mem_byte;
extern	uint	dos_mem_start;

extern int swapmem_on;
extern int swapmem_max;
extern int swapmem_cnt;
extern int sys_mem;

/* variable so RMB-UX can tell what type floating point coprocessor we have */
int fp_coprocessor_type = 0;

/* for software trigger stuff --- needs to be somewhere */
int	sw_level = 0;

int	hil_pbase;	/* Physical page # for base memory pool */
caddr_t highest_kvaddr;	/* Higest vaddr used in KERNELSPACE */

/* temporary place holder for BANANAJR mm */
struct	eightbytes {
	int	firstword;
	int	secondword;
} eightbytes;

struct map *validsegmap;
struct map *blocktablemap;
struct map *pagetablemap;

#define	pfntopc(x)	((x) - physmembase)

#define PROC_OVHD   (8 * (UPAGES + KSTACK_PAGES))

int	CB_kvalloc = 1;
int	CB_fsbcache = 1;
/*
 * Machine-dependent startup code
 */
extern int meminit();
startup(firstaddr)
int firstaddr;
{
	register int unixsize;
	register unsigned i, j, k;
	register int mapaddr;
	register caddr_t v, vsave;
	int maxbufs, base, residual;
	int lmaxmem, lfreemem;
	extern char mc68881;
	extern struct ste Syssegtab[];
	extern unsigned int kgenmap_ptes;
	extern unsigned int high_addr;
	extern unsigned int highpages_alloced;
	extern unsigned int Buffermap_ptes;
	extern vm_lock_t   bcvirt_lock;
	extern vm_lock_t   bcphys_lock;
	extern int first_paddr;
	extern int three_level_tables;
	unsigned int pt_offset;
	struct pte *smap;
	extern char Proc0pt[];	
	extern int dragon_present;
	extern int dragon_hw_present;
	extern int (*driver_link[])();
	register char *p, *q;
	int curr_highpages_alloced;
	int	pte_bits;
	int	sysmapsize;


	/* setup lmaxmem */
	if (dos_mem_byte)
		lmaxmem = maxmem + btoc(dos_mem_byte);
	else
		lmaxmem = maxmem;

	curr_highpages_alloced = btorp(0 - high_addr);
	physmem -= curr_highpages_alloced;

	purge_tlb();

	/* Initialize 68881 here because we use it to determine */
	/* difference between model 360 and model 375           */

	mc68881 = initialize_68881();

	/*
	 * Initialize utsname.machine structure
	 * don't fill in serial at all (it's zero now)
	 */
	getmodeltype();
	

	dbc_init ();

	if (nswbuf == 0) {
		nswbuf = 64;		/* HP REVISIT sanity */
	}

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * As pages of memory are allocated, "firstaddr" is incremented.
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */
	VASSERT(firstaddr == first_paddr);
	mapaddr = 0;
	v = (caddr_t)(GENMAPSPACE + ((Buffermap_ptes - kgenmap_ptes)*1024));

#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))

#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))

	/* Flag where we left off for first allocation pass */
	vsave = v;

	valloc(swbuf, struct buf, nswbuf);
	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
        /* We want to reserve some slots of the file table for the case
          where the file table is full and commands are unusable.
          file_reserve defines the begining of this part of the file table */
        file_reserve = fileNFILE - file_pad;
	valloclim(proc, struct proc, nproc, procNPROC);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);

	/* Allocate kernel virtual address resource map.  The buffer cache
	** uses this map starting with 9.0.  Since the buffer cache is 
	** dynamic in size, we need a larger map on larger memory systems.
	** The size of the increase is open to debate.  
	*/
	sysmapsize = NSYSMAP;
	if (physmem > 4096)
		sysmapsize += (physmem-4096)/4;
	valloc(sysmap, struct map, sysmapsize);

	/* the system will use at most ? validsegmap entries */
	valloc(validsegmap, struct map, 200);

	if (three_level_tables) {
		/* the system will use at most ? block tables */
		valloc(blocktablemap, struct map, 400);

		/* the system will use at most ? page tables */
		valloc(pagetablemap, struct map, 400);
	}


	/*
	 * NOW do the page allocation structures. These should be last
	 * so that as few "pfdat"s as possible are wasted on staticly
	 * allocated structures.  Note that no space has yet been allocated
	 * to buffers.
	 */

	/* calculate number of pageable pages and calculate
	 * hash mask.  Allocate space for hash table which
	 * is used to find buffers.
	 */
	{
	    unsigned int
		total,		/* Total pages left */
		pool,		/* Pages in pfdat[] pool */
		hashsiz,	/* Size of pfhash[] table for mask */
		ovhd,		/* Overhead of pfdat[] */
		out,		/* Outstanding space from above valloc()s */
		x;		/* Temp */
	    extern int
		phashmask;	/* Mask for hashing into pfdat[] */

	    out = btorp(v-vsave);
	    total = physmem-pfntopc(firstaddr)-out;

	    /*
	     * First allocate hash mask based on # bits in pfd indices
	     */
	    phashmask = 0;
	    for( x = total; x; x >>= 1 ){
		phashmask = (phashmask << 1) | 1;
	    }
	    hashsiz = phashmask+1;
	    valloc(phash, struct pfdat *, hashsiz);

	    /*
	     * Allocate kmeminfo[] array to keep per-page size information
	     */
	    valloc(kmeminfo, caddr_t, total);

	    /* align cmap on a long word boundry for lots of reasons... */
	    v = (caddr_t)(((unsigned int)v + 3) & 0xfffffffc);

	    /* Are we longword aligned? */
	    if ((((int) &(((pfd_t  *) 0) -> pf_hdl.pf_ro_pte)) & 3) != 0)
		panic("pf_ro_pte must be longword aligned");

	    /*
	     * Allocate pfdat[] array to keep page information
	     */
	    valloc(pfdat, struct pfdat, total);
	}

	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */

	/* page align */
	v = (caddr_t)ptob(btorp(v));

        unixsize = btorp(v - vsave + 1) + mapaddr;
	firstaddr += unixsize - mapaddr;
	if (firstaddr >= pctopfn(physmem) - PROC_OVHD)
		panic("startup: not enough memory to powerup");

	/*
	 * Make sure we're page aligned
	 */
	v = (caddr_t)ptob(btorp(v));

	/*
	 * Reserve enough virtual addresses to map ALL of the
	 *  free page pool.
	 */
	hil_pbase = pfntopc(firstaddr);

	/* 
	** XXX for now...
	** The kernel sysmap area is 1 GB in size and is
	** attached at virtual address 0x40000000 (1 GB)
	*/
#define KERN_MAP_PAGES 		(0x40000)
#define KERN_MAP_ATTACH_PFN	(0x40000)

	/* add some padding for paranoia sake */
	v += ptob(2);
	highest_kvaddr = v;

#define KVM_PAGES (100)		/* How many virtual pages to offer */
#define KVM_GAP (5)		/* # pages gap from kernel mem to KVM */
	/*
	** XXX for now...	(nuke the constants)
	** The kernel kvm area is KVM_PAGES in size and is
	** attached just after the internal I/O segment.
	*/

	/* kernel virtual attach point */
#define KVM_ATTACH_POINT (LOGICAL_IO_BASE + ptob(INTIOPAGES))


	kmap_init(0, GENMAPSPACE,0,highest_kvaddr-GENMAPSPACE,0);
	highpages_alloced = btorp(0 - high_addr);
	physmem -= highpages_alloced - curr_highpages_alloced;

	if (three_level_tables)
		pt_offset = ((GENMAPSPACE & SG3_PMASK) >> SG_PSHIFT) << 2;
	else
		pt_offset = ((GENMAPSPACE & SG_PMASK) >> SG_PSHIFT) << 2;

	kgenmap_ptes = high_addr + pt_offset;
	Buffermap_ptes += kgenmap_ptes;

        mapaddr = 0;
	firstaddr = first_paddr;
	smap = (struct pte *)Buffermap_ptes;


	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */
	pte_bits = PG_V|PG_RW;
	if (CB_kvalloc) 
	{ 
		pte_bits |= PG_CB; 
	} else		
	{ 
		pte_bits |= PG_CI; 
	}
	for (i = mapaddr; i < unixsize; i++) {
		*(int *)(&smap[i]) = pte_bits | (firstaddr<<PGSHIFT);

		if (firstaddr >= pctopfn(physmem) - PROC_OVHD)
			panic("startup: not enough memory to powerup");
		clearseg((unsigned)firstaddr);
		firstaddr++;
	}
	if (firstaddr >= pctopfn(physmem) - PROC_OVHD)
		panic("startup: not enough memory to powerup 4");

	purge_tlb();


	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize the spinlock mechanism before initializing individual
	 * spinlocks/semaphores.
	 */
	init_spinlocks();
	hdl_initlocks();
	vm_initlock(rswap_lock, RSWAP_LOCK_LOCK_ORDER, "rswap_lock lock");
	vm_initlock(pfdat_lock, PFDAT_LOCK_LOCK_ORDER, "pfdat_lock lock");
	vm_initlock(pfdat_hash, PFDAT_HASH_LOCK_ORDER, "pfdat_hash lock");
        vm_initlock(bcvirt_lock, BUF_VIRT_LOCK_ORDER, "bcvirt_lock lock");
        vm_initlock(bcphys_lock, BUF_PHYS_LOCK_ORDER, "bcphys_lock lock");

	/*
	 * Initialize HIL portions of VM system
	 *
	 * Note that meminit() is told about physical page numbers from
	 *  a ZERO base (so that pfdat[] can be indexed by them), so we
	 *  had to stash away the physical base for use by the HDL
	 *  procedures later.
	 */
	meminit( 0, physmem-hil_pbase, KERN_MAP_ATTACH_PFN - 1);
	vasinit();
	reginit();
	preginit();
	lfreemem = maxmem = freemem;

        /* Initialize "available" memory for swap */
	if (swapmem_on) {
		sys_mem = (maxmem / LOTSFREEFRACT) + BUFFERZONE;
       		swapmem_max = maxmem - sys_mem;
        	swapmem_cnt =  swapmem_max;
	}
	else {
		sys_mem = maxmem;
       		swapmem_max = 0;
        	swapmem_cnt = 0;
	}

	/* Initialize sysmap for rest of kernel virtual addresses */
	rminit(sysmap, 0, 0, "sysmap", sysmapsize);
	rmfree(sysmap, KERN_MAP_PAGES, KERN_MAP_ATTACH_PFN);

	/* start with an empty map */
	rminit(validsegmap, 0, 0, "validsegmap", 200);

	if (three_level_tables) {
		/* start with empty maps */
		rminit(blocktablemap, 0, 0, "blocktablemap", 400);
		rminit(pagetablemap, 0, 0, "pagetablemap", 400);
	}

	/*
	 * Now that we know how big we're going to be,
	 *  initialize the HDL stuff.
	 */
	hdl_meminit( 0, physmem-hil_pbase, btop(v) );

	/* Initialize kernel virtual memory */
	config_kvm(KVM_ATTACH_POINT, KVM_PAGES);

	/*
	 * Configure devswap here so that devswap_vp will be valid when the
	 * graphics driver creates its lock page
	 */
	devswap_init();

	/*
	 * Configure the I/O subsystem.
	 */
	link_drivers();

	/* There used to be a call to MBINIT here -- moved it to after
	 * p0init so we're off the ICS.  A previous comment indicated
	 * some need for sequencing, on DUXs behalf.  It has yet to be
	 * determined exactly what that condition is...
	 */

	snoozeinit();
	hidden_mode_initialize();
	internal_hil_init();		/* initialize internal HIL interface */
	tty_init();
#ifndef QUIETBOOT
	printf("\nI/O System Configuration:\n");

	switch(processor) {
	case 1:
		printf("    MC68020 processor\n");
		break;
	case 2:
		printf("    MC68030 processor\n");
		break;
	case 3:
		printf("    MC68040 processor\n");
		break;
	default:
		panic("startup: Unknown processor type");
	}
	if (mc68881 && (processor != 3)) {
		if (is_mc68882()) {
			fp_coprocessor_type = 2;
			printf("    MC68882 coprocessor\n");
		} else {
			fp_coprocessor_type = 1;
			printf("    MC68881 coprocessor\n");
		}
	}
#endif /* ! QUIETBOOT */
	parity_init();
	kernel_initialize();
	if (dragon_hw_present) {
#ifndef QUIETBOOT
		printf("    HP98248A Floating Point Accelerator at 0x5c000 ");
		if (!dragon_present)
			printf("Ignored: interface driver not present.");
		printf("\n");	
#endif /* ! QUIETBOOT */
	} else
		float_init();

	if (dos_mem_byte)
		msg_printf("memory reserved for dos  = %d\n", dos_mem_byte);

}

/*
 * Fill in utsname.machine field with appropriate string.
 */
getmodeltype()
{
	extern short machine_model; /* declared as short in locore.s */

	switch(machine_model) {
		case MACH_MODEL_310:
			bcopy( "9000/310", utsname.machine, 8);
			break;
		case MACH_MODEL_320:
			bcopy( "9000/320", utsname.machine, 8);
			break;
		case MACH_MODEL_330:
			bcopy( "9000/330", utsname.machine, 8);
			break;
		case MACH_MODEL_332:
			/* Otter is S332 */
			bcopy( "9000/332", utsname.machine, 8);
			break;
		case MACH_MODEL_340:
			/* Ferret is S340 */
			bcopy( "9000/340", utsname.machine, 8);
			break;
		case MACH_MODEL_345:
			bcopy( "9000/345", utsname.machine, 8);
			break;
		case MACH_MODEL_350:
			bcopy( "9000/350", utsname.machine, 8);
			break;
		case MACH_MODEL_360:
			/* Weasel is S360 */
			bcopy( "9000/360", utsname.machine, 8);
			break;
		case MACH_MODEL_370:
			/* Wolverine is S370 */
			bcopy( "9000/370", utsname.machine, 8);
			break;
		case MACH_MODEL_375:
			/* Summit is S375 */
			bcopy( "9000/375", utsname.machine, 8);
			break;
		case MACH_MODEL_380:
			bcopy( "9000/380", utsname.machine, 8);
			break;
		case MACH_MODEL_385:
			bcopy( "9000/385", utsname.machine, 8);
			break;
		case MACH_MODEL_40T:
			bcopy( "9000/40T", utsname.machine, 8);
			break;
		case MACH_MODEL_42T:
			bcopy( "9000/42T", utsname.machine, 8);
			break;
		case MACH_MODEL_43T:
			bcopy( "9000/43T", utsname.machine, 8);
			break;
		case MACH_MODEL_40S:
			bcopy( "9000/40S", utsname.machine, 8);
			break;
		case MACH_MODEL_42S:
			bcopy( "9000/42S", utsname.machine, 8);
			break;
		case MACH_MODEL_43S:
			bcopy( "9000/43S", utsname.machine, 8);
			break;
		case MACH_MODEL_WOODY25:
			bcopy( "9000/42E", utsname.machine, 8);
			break;
		case MACH_MODEL_WOODY33:
			bcopy( "9000/43E", utsname.machine, 8);
			break;
		case MACH_MODEL_MACE25:
			bcopy( "9000/382", utsname.machine, 8);
			break;
		case MACH_MODEL_MACE33:
			bcopy( "9000/387", utsname.machine, 8);
			break;
		case MACH_MODEL_UNKNOWN:
			bcopy( "9000/300", utsname.machine, 8);
			break;
		default:
			bcopy( "9000/300", utsname.machine, 8);
			break;
	}
}

link_drivers()
{
	int (**pp)() = driver_link;

	while ( *pp != (int (*)())0 ) 
		(*pp++)();
}

/*
** procedure chain which drivers can insert themselves into so that they
** will be called at rootinit time
*/
last_init()
{
}

int (*dev_init)() = last_init;


rootinit()
{
/*
**  Find the root device.  Call find_root to determine (from the bus address)
**  whether the root & swap a local and/or remote, and if the user can change.
*/
	extern site_t root_site;
	int remoteroot;
	extern int (*rootfsmount)();	/* pointer to root mounting routine */
	extern int dux_mountroot();
	extern int dskless_initialized;
	extern int alloc_pidtable();
	extern struct msus msus;

	(*dev_init)();

	if (rootdev == NODEV
			&& (isc_table[msus.sc] == NULL
				|| isc_table[msus.sc]->card_type != HP98643))
		panic("rootinit: no driver for boot/root device");

	DUXCALL(FIND_ROOT)(&remoteroot);

	if (remoteroot != 0)
	{
		/*
		** Ok, we have a remote root.
		*/
		DUXCALL(DUX_LANIFT_INIT)();
		DUXCALL(WS_CLUSTER)();

		if (rootdev < 0)
		{
			panic("rootinit: root device driver not present");
		}
		rootfsmount = dux_mountroot;
	} else {
		if (dskless_initialized) {
			/*
			 * We have dskless configured and we are not
			 * remote root, so we will need a pidtable.
			 * Allocate its memory.
			 */
			alloc_pidtable();
		}
	}
	
#ifndef QUIETBOOT
	printf("    Root device major is %d, minor is 0x%x, root site is %d\n",
				major(rootdev), minor(rootdev), root_site);
#endif /* ! QUIETBOOT */
}


#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	return(time.tv_sec-otime);  /* time resolution is 20 msec !!!!! */
}
#endif

#define	CALL2	0x4E90		/* jsr (a0)    */
#define	CALL6	0x4EB9		/* jsr address */
#define	BSR	0x6100		/* bsr address */

#define	ADDQL	0x500F		/* addq.l #xxx,sp */
#define	ADDL	0xDEFC		/* add.l  #xxx,sp */
#define	LEA 	0x4fef		/* lea    xxx(sp),sp */

int *bt_link;			/* to get a6--doesn't depend much on compiler */
int *panic_stack;

extern int boot_code[];
extern int szboot_code;

int	waittime = -1;
short	reboot_after_panic = 0;	/* may make user configurable */

extern int console_width, console_height;

boot(arghowto, panic_stack_ptr)
	int arghowto;
	union {
		ushort	*s;
		char	*c;
	} panic_stack_ptr;
{
	register int howto;		/* how to boot */
	register int devtype;		/* major of root dev */
	register int i, j;		/* loop counter */
	int nbusy;			/* number of busy buffers */
	register struct proc *p;
	register struct file *fp;
	int numprocs;			/* number of procs in proc table */
	register struct mount *mp;	/* pointer to mount table entry */
	register struct mounthead *mhp; /* pointer to mount hash table entry */
	int flag;			/* whether file system mounted ronly */
	dev_t dev;			/* dev_t value for mounted file sys */
	int rtn, *pp; 
	short inst;
	int calladr, entadr;
	struct symentry *s;
	int args_size;
	register struct buf *bp;
	int iter;

	int lines_remaining;
	int err;
	int max_safe_procs;

#ifdef lint
	howto = 0; devtype = 0;
	printf("howto %d, devtype %d\n", arghowto, devtype);
#endif
	(void) spl0();
	howto = arghowto;

#ifdef 	CLEANUP_DEBUG
	if (howto & RB_CRASH) {
		spl6();
		goto crash;
	}
#endif 	CLEANUP_DEBUG

	lines_remaining = console_height;
	if (lines_remaining == 0)		/* not an ite? */
		lines_remaining = 48;		/* usual terminal */

	lines_remaining -= 6;			/* overhead before us */

	if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		/* kill the limited nsp */
		if ((my_site_status & (CCT_CLUSTERED | CCT_ROOT)) == 
						(CCT_CLUSTERED | CCT_ROOT)) {
			if ((err = DUXCALL(KILL_LIMITED_NSP)()) != 0) {
				printf("Couldn't kill limited CSP\n");
			}
		}
		
		if (howto & RB_PANIC)
                        update(0,1,0);
		else
			boot_clean_fs();

		/*
		 *  Close all mounted file systems including
		 *  the root file system.  Close is synchronous
		 *  so all buffers should be finished to disc 
		 *  before close finishes as long as no other 
		 *  file system activity is going on.
		 */
		printf("syncing disks...");
		for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
		{
			for (mp = mhp->m_hforw; mp != (struct mount *)mhp;
			     mp = mp->m_hforw)
			{
				if (((mp->m_flag&MINUSE)==0) || mp->m_dev==NODEV)
					continue;
				dev = mp->m_dev;
				if ((!(bdevrmt(dev))) &&
				    (mp->m_vfsp->vfs_mtype == MOUNT_UFS)) {
					flag = !(mp->m_bufp->b_un.b_fs->fs_ronly);
					(*bdevsw[major(dev)].d_close)(dev, flag);
				}
			}
		}
		for (iter = 0; iter < 5; iter++) {
			nbusy = busybufs();
			if (nbusy <= 1)
				break;
			printf("%d ", nbusy);
			snooze(2000000); /* sleep 2 seconds.   */
					 /* retries currently 
					 /* range from 2 seconds */
					 /* to 5 seconds. */
		}
		printf("done\n"); lines_remaining--;
		if( my_site_status & CCT_CLUSTERED )
		{
			spl6();
			DUXCALL(BROADCAST_FAILURE)();
		}
	}
	spl6();			/* extreme priority */
	devtype = major(rootdev);
	if (howto & RB_PANIC) {
		register int	line, word, end_of_stack;

		/* Dump a bit of the stack - note "word" is 16 bits. */
		/* We dump 16 of these words per line, which fits    */
		/* well on any 80-column or wider console.  This     */
		/* does not look very good on a 50-column 9826, but  */
		/* (1) there is no easy way to tell we have one, and */
		/* (2) they are not officially supported for HP-UX.  */
		for (end_of_stack = 0, line = 0;
		     !end_of_stack && line < 12;
		     line++) {
			printf ("%x:", &panic_stack_ptr.s[0]);
			for (word = 0; !end_of_stack && word < 16; word++) {
				if (panic_stack_ptr.c < (char *)KSTACKADDR) {
					if ((word & 1) == 0)
						putchar (' ', 0);
					printf ("%04x", *panic_stack_ptr.s++);
				} else
					end_of_stack = 1;
			}
			putchar ('\n', 0); lines_remaining--;
		}

		asm("   mov.l %a6,_bt_link");
		asm("   mov.l %sp,_panic_stack");

		pp = bt_link; 
		while (lines_remaining >= 1
		    && pp >= panic_stack
		    && pp < (int *) KSTACKADDR ) {

			pp = bt_link; 

			if (!testr(*pp, 8) || ((int) pp & 01))
				break;

			bt_link = (int *) *pp;
			rtn = *++pp;

			calladr = NULL;
			entadr = NULL;

			if (!testr(rtn-6, 12) || (rtn & 01))
				break;

			if (*((short *) (rtn-6)) == CALL6) {
				entadr = *((int *) (rtn-4));
				calladr = rtn - 6;
			} else if ((*((short *) (rtn-4)) & ~0xFFL) == BSR) {
				calladr = rtn - 4;
				entadr = (short) *((short *) (rtn-2)) + rtn-2;
			} else if ((*((short *) (rtn-2)) & ~0xFFL) == BSR) {
				calladr = rtn - 2;
				entadr = (char) *((short *) (rtn-2)) + rtn;
			} else if (*((short *) (rtn-2)) == CALL2)
				calladr = rtn - 2;

			inst = *((short *) rtn);

			if ((inst & 0xF13F) == ADDQL) {
				args_size = (inst>>9) & 07;
				if (args_size == 0) args_size = 8;
			}
			else if ((inst & 0xFEFF) == ADDL) {
				if (inst & 0x100)
					args_size = *((int *) (rtn + 2));
				else
					args_size = *((short *) (rtn + 2));
			}
			else if ((inst & 0xFFFF) == LEA)
				args_size = *((short *) (rtn + 2));
			else
				args_size = 0;	   /* no adjustment, no args! */

			printf("%08x: ", calladr);
			print_addr(entadr); printf(" (");

			while (args_size>0) {
				pp++;
				printf(*pp>=0 && *pp<=9 ? "%d" : "0x%x", *pp);
				args_size-=4;
				if (args_size)
					printf(", ");
			}

			printf(")\n"); lines_remaining--;
		}

		/* print current time */
		print_gmt_time();

		/* print out utsname info */
		printf(" %s %s %s %s %s %s\n",
			utsname.sysname, utsname.nodename, utsname.release,
			utsname.version, utsname.machine, utsname.idnumber);
		lines_remaining--;

		if (reboot_after_panic)
			howto &= ~RB_HALT;
		else
			howto |= RB_HALT;
	}

	if (!(howto&RB_HALT)) {
		register int addr;	/* avoid compiler bug */
#ifdef	CLEANUP_DEBUG
crash:
#endif	CLEANUP_DEBUG
		addr=0xfffff800;
		bcopy((caddr_t) boot_code, (caddr_t) addr, szboot_code);
		((int (*) ()) addr)(); 
	}

	printf("\nHalted, you may now cycle power.\n");
	for (;;)
		asm("	stop	&0x2700 ");
}

/* A small symbol table of common entry points for panic backtraces. */

extern int fault(), trap(), update(), buserror();

struct panic_addrs {
	char *panic_name;
	int (*panic_addr)();
} panic_addrs[] = {
	{"   fault", fault},
	{"buserror", buserror},
	{"  update", update},
	{"   panic", panic},
	{"    trap", trap},
	{"    boot", boot},
	{"", 0}
};

print_addr(a)
register char  *a;
{
	register struct panic_addrs *p;

	/* Try to translate the address to a name */
	for (p=panic_addrs; p->panic_addr; p++) {
		if ((char *) p->panic_addr == a) {
			printf("%s", p->panic_name);
			return;
		}
	}

	if (a == NULL)
		printf("     ???");
	else
		printf("%08x", a);
}


char *dow[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *mon[] = {	"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

#define	dysize(A) (((A)%4)? 365: 366)

static short dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static short dmleap[12]={31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

print_gmt_time()
{
	register int d0, d1;
	register long hms, day;
	int tm_sec, tm_min, tm_hour, tm_wday, tm_year, tm_mday, tm_mon;
	struct timeval atv;

	get_precise_time (&atv);

#define SECONDS_PER_DAY (24L * 60L * 60L)
	/*
	 * break initial number into days
	 */
	hms = atv.tv_sec % SECONDS_PER_DAY;
	day = atv.tv_sec / SECONDS_PER_DAY;
	if (hms < 0) {
		hms += SECONDS_PER_DAY;
		day -= 1;
	}
	/*
	 * generate hours:minutes:seconds
	 */
	tm_sec = hms % 60;
	d1 = hms / 60;
	tm_min = d1 % 60;
	d1 /= 60;
	tm_hour = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	tm_wday = (day + 7340036L) % 7;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d1=70; day >= dysize(d1); d1++)
			day -= dysize(d1);
	else
		for(d1=70; day < 0; d1--)
			day += dysize(d1-1);
	tm_year = d1;
	d0 = day;

	/*
	 * generate month
	 */

	if(dysize(d1) == 366)
		for(d1=0; d0 >= dmleap[d1]; d1++)
			d0 -= dmleap[d1];
	else
		for(d1=0; d0 >= dmsize[d1]; d1++)
			d0 -= dmsize[d1];
	tm_mday = d0+1;
	tm_mon = d1;

	printf("%s %s %d %02d:%02d:%02d GMT %d",
		dow[tm_wday],
		mon[tm_mon],
		tm_mday,
		tm_hour, tm_min, tm_sec,
		tm_year+1900);
}


extern caddr_t romprf_code[];
extern int szromprf;

rom_printf(s)
caddr_t s;
{
	register int addr=0xfffff800;	/* avoid compiler bug */

	bcopy(romprf_code, (caddr_t) addr, szromprf);
	bcopy(s, (unsigned int)addr + szromprf, strlen(s)+1);
	((int (*) ()) addr)(); 
}

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by trap
 * to sigcleanup routine below.  Sigcleanup
 * restores the signal mask and user registers,
 * checks whether the user wants to restart or
 * return an error from an interrupted system call,
 * and returns to user via syscall() (in trap.c).
 *
 * Warning - user_fp is a pointer into the user
 *	     It is NOT directly dereferenceable
 *	     from the kernel (as it is on a vax).
 */

sendsig(p, sig, returnmask)
	void (*p)(); 
	int sig, returnmask;
{
	register struct exception_stack *regs;
 	register struct sigframe *user_fp;
	struct sigframe temp_frame;
	int oonstack;
	extern int dragon_present;
	register struct buf *bp;
	register struct dil_info *dil_info;
	int x;

	regs = (struct exception_stack *)u.u_ar0;
	oonstack = u.u_onstack;
#define	mask(s)	(1<<((s)-1))
	if (!u.u_onstack && (u.u_sigonstack & mask(sig))) {
		user_fp = (struct sigframe *)u.u_sigsp - 1;
		u.u_onstack = 1;
	} else
		user_fp = (struct sigframe *)regs->e_regs[SP] - 1;

	/* Set up the four fields in the sigframe struct    */
	/* which are used by the trampoline code to invoke  */
	/* the user signal handler.			    */
	temp_frame.sf_signum = sig;
#if defined(PLOCKSIGNAL)
	if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV ) {
#else
	if (sig == SIGILL || sig == SIGFPE) {
#endif
		temp_frame.sf_code = u.u_code;
		u.u_code = 0;
	} else
		temp_frame.sf_code = 0;

	if (sig == u.u_procp->p_dil_signal) {
		/*
		**  remove head of event list and deliver event to him.
		**  if the list is not empty then call psignal to get
		**  the next one going.
		*/
		x = spl6();
		bp = u.u_procp->p_dil_event_f;

		/* if bp is NULL then this must be SIGIO so just ignore it */
		if (bp != NULL) {
			dil_info = (struct dil_info *)bp->dil_packet;

			/* take buf off list of pending interrupts 
			   for this process */
			if (dil_info->event_back != NULL)
				((struct dil_info *)
				  dil_info->event_back->dil_packet)->event_forw = 
							  dil_info->event_forw;
			else
				u.u_procp->p_dil_event_f = dil_info->event_forw;
			if (dil_info->event_forw != NULL)
				((struct dil_info *)
				  dil_info->event_forw->dil_packet)->event_back = 
							  dil_info->event_back;
			else
				u.u_procp->p_dil_event_l = dil_info->event_back;

			/* save params for this event */
			temp_frame.sf_code = dil_info->event;
			temp_frame.sf_full.fs_context.sc_pad1 = dil_info->eid;
			temp_frame.sf_full.fs_context.sc_pad2 = dil_info->ppl_mask;

			/* clear out event mask */
			dil_info->event = 0;

			/* if there are still pending events send SIGDIL */
			if (u.u_procp->p_dil_event_f != NULL)
				psignal(u.u_procp, u.u_procp->p_dil_signal);
		}
		splx(x);
	}
	temp_frame.sf_scp = &user_fp->sf_full.fs_context;
	temp_frame.sf_handler = p;

	temp_frame.sf_full.fs_context.sc_onstack = oonstack;
	temp_frame.sf_full.fs_context.sc_mask = returnmask;
	temp_frame.sf_full.fs_context.sc_syscall_action = SIG_RETURN;

	if ((u.u_eosys == EOSYS_NORMAL && u.u_error == EINTR) ||
	     u.u_eosys == EOSYS_INTERRUPTED ||
	     u.u_eosys == EOSYS_RESTART) {
		/* the only cases where a system call may be restarted */
		temp_frame.sf_full.fs_context.sc_syscall = u.u_syscall;
		temp_frame.sf_full.fs_eosys = EOSYS_INTERRUPTED;
	} else {
		/* This includes u.u_eosys values EOSYS_NORMAL, */
		/* EOSYS_NOTSYSCALL, and EOSYS_NORESTART        */
		temp_frame.sf_full.fs_context.sc_syscall = SYS_NOTSYSCALL;
		temp_frame.sf_full.fs_eosys = u.u_eosys;

		/* The following is only necessary for EOSYS_NORMAL */
		/* but is harmless otherwise.                       */

		/* Since there are now errno's > 0xff, and the field
		 * fs_error is defined as a u_char, we use the larger
		 * (32 bit) u_rval1 to hold it.  We undo this wunderkludge
		 * in sigcleanup().  We don't simply change fs_error
		 * to be a ushort or uint as this changes the offsets
		 * within the full_sigcontext and sigframe structures,
		 * and various things (lisp, ADA) depend on knowing where
		 * the registers live -- another offset.
		 */
		if (u.u_error >= 0xff) {
			temp_frame.sf_full.fs_error = 0xff;
			temp_frame.sf_full.fs_rval1 = u.u_error;
		} else {
			temp_frame.sf_full.fs_error = u.u_error;
			temp_frame.sf_full.fs_rval1 = u.u_r.r_val1;
		}
		temp_frame.sf_full.fs_rval2 = u.u_r.r_val2;
	}

	/* Set up registers for rte after sigcleanup.    */
	/* Other registers are saved by trampoline code. */
	if ((processor == M68040) && (u.u_pcb.pcb_flags & M68040_WB_MASK)) {
		u.u_pcb.pcb_flags &= ~ M68040_WB_MASK ;
		temp_frame.sf_full.fs_context.sc_sp = (u_int) &u ;
		temp_frame.sf_full.fs_context.sc_pc = (u_int) restart_writebacks ;
		temp_frame.sf_full.fs_context.sc_ps = 0x2000 ;
	} else {
		temp_frame.sf_full.fs_context.sc_sp = regs->e_regs[SP];
		temp_frame.sf_full.fs_context.sc_pc = regs->e_PC;
		temp_frame.sf_full.fs_context.sc_ps = regs->e_PS;
	}

	/* Set up rte to trampoline code. */
	regs->e_regs[SP] = (int)&user_fp->sf_handler;
	regs->e_PC = (int)((struct user *)user_area)->u_sigcode;

	/* If the kernel was entered via syscall(), set fields in u so   */
	/* syscall() in trap.c will return to trampoline code without    */
	/* disturbing user registers.  The trampoline code can then save */
	/* the registers in the sf_regs array.  If the kernel was not    */
	/* entered via syscall() (ie. was entered via buserror() or      */
	/* trap()) these routines will not disturb the registers before  */
	/* returning, and we must leave u_eosys as EOSYS_NOTSYSCALL.     */

	if (u.u_eosys != EOSYS_NOTSYSCALL) {
		u.u_error = 0;
		u.u_eosys = EOSYS_INTERRUPTED;
	}

	save_floating_point(&temp_frame.sf_full.fs_regs[GPR_REGS]);
	if (dragon_present && (u.u_pcb.pcb_dragon_bank != -1))
		dragon_save_regs(&(temp_frame.sf_full.fs_regs[DRAGON_START]));

	/* save the fault address and fpiar register */
	temp_frame.sf_full.fs_regs[GPR_REGS] = u.u_pcb.pcb_fault_addr;
	temp_frame.sf_full.fs_regs[GPR_REGS+3] = u.u_pcb.pcb_fpiar;

	/*
	** copy out the sigframe, except the general purpose registers,
	** which will be initialized by the trampoline code.
	*/
	if (copyout((caddr_t)&temp_frame, (caddr_t)user_fp, sizeof(temp_frame))) {
/*
	 	* Process has overflowed or trashed its stack;
		* give it a segmentation violation signal.
		* Unless it is set up to catch that signal on a
		* different stack, this could cause an infinite loop;
		* so we must insure that the signal is fatal.
	 	*/
		register struct proc *pp = u.u_procp;
		register int bit;

		bit = mask(SIGSEGV);
		if (((pp->p_sigcatch & bit) &&
		      !(pp->p_sigmask & bit) &&
		      (u.u_sigonstack & bit)  &&
		      !u.u_onstack)) {
			/*
			* Process is set up to catch SIGSEGV on different stack.
			* We first re-instate the signal we are catching,
			* leaving it masked until SIGSEGV handler returns.
			*/
			pp->p_sig |= mask(sig);
			pp->p_sigcatch |= mask(sig);
			u.u_signal[sig-1] = p;
			if (sig == SIGILL || sig == SIGFPE)
				u.u_code = temp_frame.sf_code;
			u.u_oldmask = returnmask;
			pp->p_flag |= SOMASK;
		} else {
			/*
			* make sure the signal is fatal
			*/
			u.u_signal[SIGSEGV-1] = SIG_DFL;
			pp->p_sigignore &= ~bit;
			pp->p_sigcatch &= ~bit;
			pp->p_sigmask &= ~bit;
		}
		/*
		* Circumvent the usual psignal/issig in order
		* to insure this signal is delivered immediately.
		*/
		pp->p_sig &= ~bit;	/* in case already pending */
		pp->p_cursig = SIGSEGV;
		psig();			/* recursive call */
		return;
	}

	/*
	** If this signal was caused by an exception which created a
	** "long" exception frame, set the flag to pop it off before
	** returning to user code.  We want to avoid continuing the
	** current instruction.
	** "long" exception frames include:
	**	0x8000 -- 68010 bus error frame
	**	0x9000 -- 68020 mid-instruction exception frame
	**	0xa000 -- 68020 short bus error frame
	**	0xb000 -- 68020 long bus error frame
	**	0x7000 -- 68040 access error exception frame
	*/
	if ((regs->e_offset & 0x8000) || ((processor == M68040) &&
	          ((regs->e_offset & FORMAT_BITS) == MC68040_ACCESS_EXCEPTION)))
		u.u_pcb.pcb_flags |= POP_STACK_MASK;
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Take care of restart or error return for
 * interrupted system calls.  Restore all
 * registers for return to user code.
 * Warning - user_fp is a pointer into the
 *	     user address space.  It is NOT
 *	     directly dereferenceable from
 *	     the kernel (as it is on a vax).
 */
sigcleanup()
{
	struct		full_sigcontext		context;
	register	struct	sigframe	*user_fp;
	register struct exception_stack *regs;
	extern		int			dragon_present;

	regs = (struct exception_stack *)u.u_ar0;

	/* The trampoline code leaves its SP pointing one field past   */
	/* the start of the sigframe structure - back up to the start. */
	user_fp = (struct sigframe *)(regs->e_regs[SP] - sizeof(int(*)()));

	if (copyin((caddr_t)&user_fp->sf_full, (caddr_t)&context,
	    sizeof(context)))
		return;

	regs->e_PC = context.fs_context.sc_pc;
	regs->e_regs[SP] = context.fs_context.sc_sp;
	/*
	 * Restart 68040 writebacks if necessary.
	 */
	if ((processor == M68040) &&
	    (context.fs_context.sc_ps == 0x2000) &&
	    (context.fs_context.sc_pc == (u_int)restart_writebacks) &&
	    (context.fs_context.sc_sp == (u_int)&u)) {
		regs->e_PS = context.fs_context.sc_ps ;
	} else {
		regs->e_PS = (context.fs_context.sc_ps|PS_USERSET)&~PS_USERCLR;
	}

	/* Handle interrupted system calls properly - syscall() in trap.c  */
	/* will finish up, depending on the values of u_error and u_eosys. */
	/* Note, u_error and u_syscall are already initialized to 0 and    */
	/* SYS_NOTSYSCALL, respectively, by syscall().                     */
	/* syscall now sets this differently */
	u.u_syscall = SYS_NOTSYSCALL;
	switch (u.u_eosys = context.fs_eosys) {

	    case EOSYS_INTERRUPTED:
		if (context.fs_context.sc_syscall_action != SIG_RETURN) {
			/* Tell syscall to back up PC. If sendsig is invoked */
			/* before returning to user, this will be postponed. */
			u.u_eosys = EOSYS_RESTART;
			u.u_syscall = context.fs_context.sc_syscall;
			break;
		}
		/* else fall through to next case */
		u.u_eosys = EOSYS_NORESTART;

	    case EOSYS_NORESTART:
		u.u_error = EINTR;
		break;

	    default:
		/* The user has apparently trashed this field.    */
		/* We'll let it go - perhaps should kill process. */
	    	u.u_eosys = EOSYS_NORMAL;
		/* and fall through to that case */

	    case EOSYS_NORMAL:
		/*
		 * if the errno was >= 255, we stuck it in the rval1
		 * which has 32 bits of room rather than fs_error
		 * which has only 8.  This happened in sendsig(),
		 * now we undo it.
		 */
		if (context.fs_error == 0xff) {
			u.u_error = context.fs_rval1;
		} else {
			u.u_error = context.fs_error;
		}
		u.u_r.r_val1 = context.fs_rval1;
		u.u_r.r_val2 = context.fs_rval2;
		break;

	    case EOSYS_NOTSYSCALL:
		break;
	}

	/* restore the bulk (d0-d7, a0-a6) of the user registers */
	bcopy(&context.fs_regs[GPR_START], &regs->e_regs[GPR_START], GPR_REGS*sizeof(int));

	if (setjmp(&u.u_psave) == 0)
		restore_floating_point(&context);

	/* if dragon  causes  exception  for some  reasone(user  trashes
	   stack or hardware  goes crazy) we wont get control  back here
	   cause it uses external  interrupt as opposed to trap/buserror
	   and we don't do longjmp  from isr, unless we can afford to be
	   extremely selective.
	*/

	if (dragon_present && (u.u_pcb.pcb_dragon_bank != -1))
		dragon_restore_regs(&(context.fs_regs[DRAGON_START]));

	u.u_onstack = context.fs_context.sc_onstack & 01;
	u.u_procp->p_sigmask = context.fs_context.sc_mask &~
				 (mask(SIGKILL)|mask(SIGSTOP));

}
#undef mask

addupc(pc, p, incr)
unsigned pc;
register struct
{
	short	*pr_base;
	unsigned pr_size;
	unsigned pr_off;
	unsigned pr_scale;
} *p;
{
	union
	{
		int w_form;		/* this is 32 bits on 68000 */
		short s_form[2];
	} word;
	register short *slot;
	int pc_offset;

/*	This only works for PC values < 64K.  A more precise version follows.
	slot = &p->pr_base[((((pc - p->pr_off) * p->pr_scale) >> 16) + 1)>>1];
*/
	if (p->pr_scale == 2)
		slot = &p->pr_base[0];
	else
	{
		if ((pc_offset = pc - p->pr_off) < 0)
			return;
		slot = &p->pr_base[(mulpc(pc_offset, p->pr_scale) + 1) >> 1];
	}

	if ((caddr_t)slot >= (caddr_t)(p->pr_base) &&
		(caddr_t)slot < ((caddr_t)p->pr_base) + p->pr_size)
		{
			if ((word.w_form = fuword((int *)slot)) == -1)
				u.u_prof.pr_scale = 0;	/* turn off */
			else
			{
				word.s_form[0] += (short)incr;
				suword((int *)slot, word.w_form);
			}
		}
}

isrlink(isr,level,regaddr,mask,value,misc,tmp)
int (*isr)(), level;
char *regaddr, mask, value, misc;
int tmp;
{
	register struct interrupt **p, *q;

	if ((level < 1) || (level > 6))
		panic("isrlink: interrupt level out of range");

	p = &rupttable[level];
	while ((q = *p) != NULL)
		p = &q->next;

	q = (struct interrupt *) calloc(sizeof(struct interrupt));
	q->next = 0;
	q->regaddr = regaddr;
	q->mask = mask;
	q->value = value;
	q->misc = misc;
	q->isr = isr;
	q->temp = tmp;

	*p = q;
}

#ifdef NEVER_CALLED
isrunlink(isr, level)
int (*isr)();
int level;
{
	struct interrupt *p, *q;

	if ((level < 1) || (level > 6))
		panic("isrlink: interrupt level out of range");

	p = rupttable[level];
	q = p;
	while ((p != NULL) && (p->isr != isr)) {
		q = p;
		p = q->next;
	}
	if (p == NULL) 
		printf("isr not on rupttable list\n");
	if (p == rupttable[level])
		rupttable[level] = q->next;
	else
		q->next = p->next;
}
#endif /* NEVER_CALLED */

struct sw_intloc *sw_queuehead;

sw_trigger(intloc,proc,arg,level,sublevel)
register struct sw_intloc *intloc;
caddr_t arg;
int (*proc)();
{
	register struct sw_intloc **p, *q;
	register x;
	/* on the assumption that there will seldom be many entries,
	   we simply keep a sorted list */
	
	x = spl6();
	if (intloc->proc > (int(*)()) 1) {
		/* already triggered; don't mess up the linked list */
		splx(x);
		return;
	}
	for (p = &sw_queuehead, q = *p; q != NULL; p = &q->link, q = *p)
		if (q->priority < level ||
		    q->priority == level && q->sub_priority < sublevel)
				break;
	intloc->link = q;
	intloc->arg = arg;
	intloc->proc = proc;
	intloc->priority = level;
	intloc->sub_priority = sublevel;
	*p = intloc;
	splx(x);
}

#ifdef notdef
dorti()
{
	struct frame frame;
	register int sp;
	register int reg, mask;
	extern int ipcreg[];

	(void) copyin((caddr_t)u.u_ar0[FP], (caddr_t)&frame, sizeof (frame));
	sp = u.u_ar0[FP] + sizeof (frame);
	u.u_ar0[PC] = frame.fr_savpc;
	u.u_ar0[FP] = frame.fr_savfp;
	u.u_ar0[AP] = frame.fr_savap;
	mask = frame.fr_mask;
	for (reg = 0; reg <= 11; reg++) {
		if (mask&1) {
			u.u_ar0[ipcreg[reg]] = fuword((caddr_t)sp);
			sp += 4;
		}
		mask >>= 1;
	}
	sp += frame.fr_spa;
	u.u_ar0[PS] = (u.u_ar0[PS] & 0xffff0000) | frame.fr_psw;
	if (frame.fr_s)
		sp += 4 + 4 * (fuword((caddr_t)sp) & 0xff);
	/* phew, now the rei */
	u.u_ar0[PC] = fuword((caddr_t)sp);
	sp += 4;
	u.u_ar0[PS] = fuword((caddr_t)sp);
	sp += 4;
	u.u_ar0[PS] |= PSL_USERSET;
	u.u_ar0[PS] &= ~PSL_USERCLR;
	u.u_ar0[SP] = (int)sp;
}
#endif

physstrat(bp, strat, prio)
	struct buf *bp;
	int (*strat)(), prio;
{
	int s;

	(*strat)(bp);
	s = spl6();
	if (bp->b_flags & B_DIL) {
		if (setjmp(&u.u_qsave)) {
			register struct dil_info *dil_info;
			register struct iobuf *iob = bp->b_queue;

			dil_info = (struct dil_info *) bp->dil_packet;
			/* 
			** Request has recieved a signal cleanup and return. 
			*/
			if (!(bp->b_flags & B_DONE)) {
				(*dil_info->dil_timeout_proc)(bp);
			}
			/* make sure any software trigger of timeout proc is complete */
			splx(s);

			/* return partial count if we got some of the I/O done */
			if (iob->b_xcount != bp->b_resid) {
				bp->b_flags &= ~B_ERROR;
				bp->b_error = 0;
			} else {
				u.u_eosys = EOSYS_NORESTART;
				bp->b_error = EINTR;
			}
			return;
		} else {
			while ((bp->b_flags & B_DONE) == 0)
				sleep((caddr_t) &bp->b_flags, DILPRI);
		}
	} else {
		while ((bp->b_flags & B_DONE) == 0)
			sleep((caddr_t)bp, prio);
	}
	splx(s);
}

/* 
** Enable parity dectection for 98257A type memory boards.
** See dmachain.s for low level parity handling.
** The routine parity_error in this file determines what
** should be done (panic, kill user process, etc.) about
** the parity error.
*/ 

parity_init()
{
	/*
	** If a buserror is not generated when location 0x5b0001
	** is written then there is a least one 98257A type memory
	** board present.
	** Setting 0x5b0001 to a 1 enables parity detection.
	*/
	if (testw(0x5b0001 + LOG_IO_OFFSET,1))
		*((short *) (0x5b0000 + LOG_IO_OFFSET)) = 1;
}

/*
** parity_option 0: just report the parity error
** parity_option 1: report the error 
**		    if user kill current process; if supervisor panic
** parity_option 2: report the error
**		    always panic
*/

parity_error(interrupt_frame, address)
struct { u_short ps; u_int pc; u_short vector_offset; } *interrupt_frame;
caddr_t address;
{
	if (trans_parity) {
		trans_parity = 0;
		printf("MEMORY ERROR (parity or ECC) AT UNKNOWN LOCATION \n");
	} else
		printf("MEMORY ERROR (parity or ECC) AT 0x%x\n", address);

	switch(parity_option) {
		case 0:
		default:
			/* Don't support option zero
			break;
			*/
			msg_printf("Parity option 0 is no longer supported\n");
			msg_printf("Using parity option 1 instead\n");
		case 1:
			if (USERMODE(interrupt_frame->ps))
				swkill(u.u_procp, "killed on memory error (parity or ECC)");
			else
				panic("parity_error: memory error (parity or ECC) in supervisor state");
			break;
		case 2:
			panic("parity_error: memory error (parity or ECC)");
			break;
	}
}

caddr_t map_copy_addr = NULL;
struct pte *map_copy_pte = NULL;

map_copy_init()
{
	extern struct pte *vm_alloc_tables();
        /* 
	 * Allocate space in the resource map. The map cant give
	 * out 0, so there is an "off by one" to account for.
	 */
        if ((map_copy_addr = (caddr_t)ptob(rmalloc(sysmap, 1))) == NULL) {
		panic("map_copy_init: cant allocate kernel virtual space");
        }
        map_copy_addr -= ptob(1);		
	map_copy_pte = vm_alloc_tables(KERNVAS, map_copy_addr);
}

map_copyout(source, dest, numb)
register char *dest; 
register char *source;
register numb;
{
	register int length;
	register int pgrep;
	register struct pte *pte;

	/* get a kernel address if necessary */
	if (map_copy_addr == NULL) map_copy_init();

	/* do not allow more than 2Gb copys XXX */
	if ((int)numb < 0) return(EINVAL);

	/* calculate user land page offset */
	pgrep = (int)dest & (NBPG-1);

	/* calculate bytes left in page */
	length = MIN(NBPG-pgrep, numb);

	/* Copy no more than a page per bcopy interation */
	for (;;) 
	{
		register i;
		/* 
		** Make sure page exists
		** Write a 0 because we will re-write it later.
		*/
		for (i = 0; i < 10; i++)
		{
			if (subyte(dest, 0)) {return(EFAULT);}
			pte = vastopte(u.u_procp->p_vas, dest);
		/****	if (pte != NULL && pte->pg_v) {break;} XXX ****/
			if (pte != NULL && (*(int *)pte & PG_V)) {break;}
			printf("Concern!! --  '%s' map_copyout pte = 0x%x, *pte = 0x%x\n",
				u.u_comm, pte, *pte);
			if (i > 5) purge_dcache_physical();
		}
		/* get users pte */
		*map_copy_pte = *pte;
		purge_tlb_select_super(map_copy_addr);

		/* purge users data cache */
		purge_dcache_u();

		/* copyout the buffer */
		if ((length == NBPG) && (((int)source & (16-1)) == 0)) 
		{
			/* This is a full page aligned in user land */
			pg_copy4096(source, map_copy_addr);
		} else
		{
			bcopy(source, map_copy_addr+pgrep, length);
		}
		/* calculate numb byte remaining */
		if ((numb -= length) <= 0) {break;}

		/* calculate length of next transfer */
		source += length;
		dest += length;
		length = MIN(NBPG, numb);
		pgrep = 0;
	}
	u.u_probe_addr = 0;
	return(0);
}

map_copyin(source, dest, numb)
register char *source;
register char *dest; 
register numb;
{
	register int length;
	register int pgrep;
	register struct pte *pte;

	/* get a kernel address if necessary */
	if (map_copy_addr == NULL) map_copy_init();

	/* do not allow more than 2Gb copys XXX */
	if ((int)numb < 0) return(EINVAL);

	/* calculate user land page offset */
	pgrep = (int)source & (NBPG-1);

	/* calculate bytes left in page */
	length = MIN(NBPG-pgrep, numb);

	/* Copy no more than a page per bcopy interation */
	for (;;) {
		register i;
		/* 
		** Make sure page exists
		** It does not matter what we read because we will read it again.
		*/
		for (i = 0; i < 10; i++)
		{
			if (fubyte(source) == -1) {return(EFAULT);}
			pte = vastopte(u.u_procp->p_vas, source);
		/****	if (pte != NULL && pte->pg_v) {break;} XXX ****/
			if (pte != NULL && (*(int *)pte & PG_V)) {break;}
			printf("Concern!! --  '%s' map_copyin pte = 0x%x, *pte = 0x%x\n",
				u.u_comm, pte, *pte);
			if (i > 5) purge_dcache_physical();
		}
		/* get users pte */
		*map_copy_pte = *pte;
		purge_tlb_select_super(map_copy_addr);
		if (fubyte(source) == -1) {return(EFAULT);}

		/* purge systems data cache */
		purge_dcache_s();

		/* copyout the buffer */
		if ((length == NBPG) && (((int)dest & (16-1)) == 0)) 
		{
			/* This is a full page aligned in user land */
			pg_copy4096(map_copy_addr, dest);
		} else
		{
			bcopy(map_copy_addr+pgrep, dest, length);
		}
		/* calculate numb byte remaining */
		if ((numb -= length) <= 0)
			break;

		/* calculate length of next transfer */
		source += length;
		dest += length;
		length = MIN(NBPG, numb);
		pgrep = 0;
	}
	return(0);
}

#ifdef CRDEBUG
/*
 * The following are 300 equivalents of the S800 panic_stktrc()
 * and getrp() functions.  They can be used for debugging.
 * panic_stktrc() is merely the stack backtrace code lifted boot().
 */
panic_stktrc()
{
/*	register int i, j; */		/* loop counter */
	int rtn, *pp; 
	short inst;
	int calladr, entadr;
/*	struct symentry *s; */
	int args_size;

	int lines_remaining;

	lines_remaining = console_height;
	if (lines_remaining == 0)		/* not an ite? */
		lines_remaining = 48;		/* usual terminal */

	lines_remaining -= 6;			/* overhead before us */

	asm("   mov.l %a6,_bt_link");
	asm("   mov.l %sp,_panic_stack");

	pp = bt_link; 
	while (lines_remaining >= 1
	    && pp >= panic_stack
	    && pp < (int *) KSTACKADDR ) {

		pp = bt_link; 

		if (!testr(*pp, 8) || ((int) pp & 01))
				break;
		bt_link = (int *) *pp;
		rtn = *++pp;

		calladr = NULL;
		entadr = NULL;

		if (!testr(rtn-6, 12) || (rtn & 01))
			break;
		if (*((short *) (rtn-6)) == CALL6) {
			entadr = *((int *) (rtn-4));
			calladr = rtn - 6;
		} else if ((*((short *) (rtn-4)) & ~0xFFL) == BSR) {
			calladr = rtn - 4;
			entadr = (short) *((short *) (rtn-2)) + rtn-2;
		} else if ((*((short *) (rtn-2)) & ~0xFFL) == BSR) {
			calladr = rtn - 2;
			entadr = (char) *((short *) (rtn-2)) + rtn;
		} else if (*((short *) (rtn-2)) == CALL2)
			calladr = rtn - 2;
		
		inst = *((short *) rtn);
		
		if ((inst & 0xF13F) == ADDQL) {
			args_size = (inst>>9) & 07;
			if (args_size == 0) args_size = 8;
		}
		else if ((inst & 0xFEFF) == ADDL) {
			if (inst & 0x100)
				args_size = *((int *) (rtn + 2));
			else
				args_size = *((short *) (rtn + 2));
		}
		else if ((inst & 0xFFFF) == LEA)
			args_size = *((short *) (rtn + 2));
		else
			args_size = 0;	   /* no adjustment, no args! */
		
		printf("%08x: ", calladr);
		print_addr(entadr); printf(" (");
		
		while (args_size>0) {
			pp++;
			printf(*pp>=0 && *pp<=9 ? "%d" : "0x%x", *pp);
			args_size-=4;
			if (args_size)
				printf(", ");
		}
		
		printf(")\n"); lines_remaining--;
	}

}

int **cr_link;

int *
getrp()
{
	int *rp;

	asm("	mov.l	(%a6), _cr_link");
	rp = cr_link;
	return(*++rp);
}
#endif /* CRDEBUG */
#ifdef	SEMAPHORE_DEBUG

static	void **sd_link;

void *
prevpc()
{
	void **rp;

	asm("	mov.l	(%a6), _sd_link");
	rp = sd_link;
	return(*++rp);
}
#endif	/* SEMAPHORE_DEBUG */

