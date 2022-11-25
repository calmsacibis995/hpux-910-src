/* @(#) $Revision: 1.18.83.7 $ */      
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/opt.h,v $
 * $Revision: 1.18.83.7 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 11:49:01 $
 */
#ifndef _SYS_OPT_INCLUDED /* allows multiple inclusion */
#define _SYS_OPT_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _WSIO

/*
 *  Dummy entry routines for sysent entries dependent on user specifiable
 *  parameters are declared in this file.
 */

#if MESG==0
msgget(){nosys();}
msgctl(){nosys();}
omsgctl(){nosys();}
msgsnd(){nosys();}
msgrcv(){nosys();}
msginit(){return(0);}
#endif

#if SEMA==0
semget(){nosys();}
semctl(){nosys();}
osemctl(){nosys();}
semop(){nosys();}
seminit(){}
semexit(){}
#endif

#if SHMEM==0
shm_fork(){}
shm_fork_backout(){}
shmcnattinc(){}
shmat(){nosys();}
shmctl(){nosys();}
oshmctl(){nosys();}
shmdt(){nosys();}
shmget(){nosys();}
shminit(){}
shmconv(){}
shmexec(){}
shmexit(){}
shmfork(){}
shmfree(){}
shmreset(){}
struct shmid_ds *isashmsv(){return(0);}
vtoshmp(){}
shmptod(){}
struct proc *shmloadproc(){return(0);}
shmptovp(){}
shmdecacatt_p(){}
shmcnattdec(){}
shmrssize_p(){}
shmrelease(){}
shmislock_p(){return(0);}
shmispoip_p(){return(0);}
shmrelp(){}
shmdistpte(){}
shm_anyrefb(){}
shm_cacheoff(){}

detachshm(vas, prp)
vas_t *vas;
preg_t *prp;
{
	detachreg(vas, prp);
}
#endif
#endif /* _WSIO */

#ifdef __hp9000s300
#if FPA==0
dragon_bank_free(){}
dragon_detach(){}
dragon_save_regs(){}
dragon_restore_regs(){}
dragon_read_ureg(){}
dragon_write_ureg(){}
dragon_buserror(){}
dragon_init(){dragon_present = 0;}
#endif

#if MC68040==0
mc68040_warn()
{
	uprintf("WARNING: Support for MC68040 floating point math\n");
	uprintf("         is NOT configured into your kernel.\n");
	uprintf("         Remove the \"mc68040 0\" line from your dfile,\n");
	uprintf("         rebuild a new kernel, and reboot on it.\n");
	asm("	add.l	&12,%sp");
	asm("	jmp	_fault");
}

bsun() { mc68040_warn(); }
inex() { mc68040_warn(); }
dz() { mc68040_warn(); }
unfl() { mc68040_warn(); }
operr() { mc68040_warn(); }
ovfl() { mc68040_warn(); }
snan() { mc68040_warn(); }
fline() { mc68040_warn(); }
unsupp() { mc68040_warn(); }
soft_emulation_beg() { mc68040_warn(); }
soft_emulation_end() { mc68040_warn(); }
fpsr_save() { mc68040_warn(); }
#endif

#if PRB==0
prb_warn()
{
	uprintf("WARNING: Support for networking probe proxy\n");
	uprintf("         is NOT configured into your kernel.\n");
	uprintf("         Remove the \"prb 0\" line from your dfile,\n");
	uprintf("         rebuild a new kernel, and reboot on it.\n");
}

prb_ninput() { prb_warn(); }
prb_pxyioctl() { prb_warn(); }
prb_buildpr() { prb_warn(); }
prb_getcache() { prb_warn(); }
prb_freecache() { prb_warn(); }
prb_init() { prb_warn(); }
prb_unsol() { prb_warn(); }
#endif

#endif /* __hp9000s300 */

#if !defined(DSKLESS) && !defined(RDU)
rdu_strategy()		{ }
#endif
#ifndef DSKLESS
#ifdef __hp9000s800
dux_lan_init() 		{ return(0); }
#endif
find_root() 		{ return(0); }
dux_swaprequest() 	{ }
dux_mountroot() 	{ }
#ifdef __hp9000s800
mysite()		{ uptr->u_rval1=1;}
#else
mysite()		{ u.u_r.r_val1=1; }
#endif /* __hp9000s800 */
pathisremote()		{ return(0); }
lock_mount() 		{ return(0); }
release_mount() 	{ }
broadcast_mount()	{ return(0); }
update_duxref() 	{ }
checksync()		{ return(0); }
setsync()		{ }
isynclock()		{ }
isyncunlock()		{ }
syncdisccheck()		{ }
update_text_change() 	{ }
xrele_send() 		{ }
dux_ustat()		{ }
dux_close()		{ }
dux_xumount() 		{ }
global_sync() 		{ }
syncdisc() 		{ }
dux_fifo_invalidate()	{ }
dux_rwip() 		{ }
notifysync() 		{ }
dux_syncip() 		{ }
send_ufs_mount()	{ }
global_unmount()	{ }
send_umount_dev()	{ }
dux_pseudo_root()	{ }
dux_fstatfs() 		{ }
ino_update() 		{ }
openp_wait_send()       { }
openp_wait_recv()	{ }
pipe_send() 		{ }
pipe_recv() 		{ }
broadcast_failure() 	{ }
dd_nspstat()		{ return(0); }
Rdd_procstat()		{ return(0); }
Rdd_nspstat()		{ return(0); }
dux_getnewpid()		{ }
alloc_pidtable()	{ }
dux_swapcontract()      { return(0); }
dux_locked()		{ return(0); }
dux_buf_check()		{ return(0); }
dux_buf_fail()		{ }
dux_buf_size()		{ }
stprmi()		{ return(0); }
pkrmi()			{ }

/*
 * For standalone system, use a simplified version of updatesitecount
 * which uses one_site sitemap as a simple reference count.
 * Putting code in opt.h breaks all the rules, but I would like
 * to avoid indirect procedure calls as much as possible.
 */
int
updatesitecount(mapp,site,value)
register struct sitemap *mapp;
site_t site;
int value;
{
	SPINLOCK(v_count_lock);
	mapp->s_onesite.s_count += value;
	SPINUNLOCK(v_count_lock);
	if (mapp->s_onesite.s_count < 0)
		panic("updatesitecount: count < 0");
	mapp->s_count = (mapp->s_onesite.s_count) ? 1 : 0;
	return(mapp->s_onesite.s_count);
}

/*
 * reduced form of getsitecount for standalone operation
 */
int
getsitecount(mapp,site)
struct sitemap *mapp;
site_t site;
{
	return(mapp->s_onesite.s_count);
}

#endif /* not DSKLESS */

/*
 * EISA Configurability Linker References
 *
 * Bushmasters and many Cobra's do not need EISA installed.
 * However, many core drivers are both core and EISA drivers 
 * and will bring the whole EISA system in regardless of 
 * whether an EISA adapter even exists.
 */
#if defined(__hp9000s700) && defined(_WSIO)
#if !defined(EISA)
	/*
	 * driver link routines will want to store
	 * attach routine pointers here, but the chain
	 * will never get called.
	 */
	int (*eisa_attach)() = 0;

	/* 
	 * to be used by VM powerup code. I simply say
	 * all slots are empty. Therefore no extra reserved
	 * space.
	 */
	read_eisa_cardid() 	{return(0);}
	eisa_cards_present()    {return(0);}
	eisa_configured = 0;

	/*
	 * resolve external references, and tell
	 * drivers nothing is present.
	 */
	int eisa_reinit = 0;
	int eisa_exists = 0;
	int eeprom_opened  = 0;

	/* eeprom.c */
	read_eisa_slot_data ()  {}
	read_eisa_function_data() {}


	/* eisa.c */
	cvt_eisa_id_to_ascii() 	{}
	map_isa_address() 	{}
	eisa_refresh_on() 	{}
	set_liowait() 		{}
	read_eisa_bus_lock() 	{}
	lock_eisa_bus() 	{}
	unlock_eisa_bus() 	{}
	eisa_get_dma() 		{}
	eisa_free_dma() 	{}
	eisa_dma_setup() 	{}
	eisa_dma_cleanup() 	{}
	lock_isa_channel() 	{}
	unlock_isa_channel() 	{}
#else
	/*
	 * can save 4K pages worth of hash table entries
	 */
	eisa_configured = 1;
#endif
#endif

/*
 * VME Configurability Linker References
 */
#if !defined(VME2)
#if defined(__hp9000s700) && defined(_WSIO)
	vme_init() {}
	vme_bus_error_check() {}
	vme_map_mem_to_host() {}
	vme_unmap_mem_from_host() {}
	vme_map_mem_to_bus() {}
	vme_unmap_mem_from_bus() {}
	vme_testr() {}
	vme_testw() {}
	vme_isrunlink() {}

	/* return false when vme not configured into kernel */
	vme_present() {return(0);}
#endif
	vme_isrlink() {}
	vme_dma_setup() {}
	vme_dma_cleanup() {}
#endif


#if	defined(INET) && !defined(NIPC)
prb_ninput(p1, m, p3, p4)
struct mbuf *m;
{
	m_freem(m);
}
#endif

#if __hp9000s800
#if	defined(LAN) || defined(LAN01)
lan_init() {}
#else
struct fileops lancops;
lanc_if_unit_init() {}
#endif /* LAN || LAN01 */
#endif /* __hp9000s800 */

#if __hp9000s300
#if	!defined(LAN01)
struct fileops lancops;
#endif /* !defined(LAN01) */
#endif /* __hp9000s300 */

#if	defined(SWITCH) && !defined(LABEL)
#define	LABEL
#endif	/* defined(SWITCH) && !defined(LABEL) */

#ifndef	LABEL		/* stub routines for disk labelling */
label_read_bp(x) {}
label_ok(x)  {}
label_read(u,v,w,x,y,z) {}
label_in_mem(x) {}
label_clear(x) {}
label_lu_init(x) {}
#endif /* !LABEL */

/*
 * We can have tcp/ip running over x.25, with no lan configured in the
 * subsystem, in which case lanc_output() called in if_ether.c will be
 * an unsatisfied symbol without the following #ifdefs
 */
#ifdef __hp9000s700
#if !defined(LAN01) && defined(INET)
lanc_output(ifp, m, sa)
struct mbuf *m;
{
        m_freem(m);
}
#endif /* no lan */
#else /* ! __hp9000s700 */
#if defined(__hp9000s800) && !defined(LAN) && defined(INET)
lanc_output(ifp, m, sa)
struct mbuf *m;
{
	m_freem(m);
}
#endif /* no lan */
#endif /* __hp9000s700 */

#if defined(__hp9000s300) && !defined(LAN01) && defined(INET)
lanc_output(ifp, m, sa)
struct mbuf *m;
{
	m_freem(m);
}
#endif /* no lan */

#if defined(__hp9000s800) && !defined(TAPE1_INCLUDED) && !defined(TAPE2_INCLUDED)
tape0_open() {}
#endif /* __hp9000s800 && !TAPE1_INCLUDED */

#endif /* _SYS_OPT_INCLUDED */

