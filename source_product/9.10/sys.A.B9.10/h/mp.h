/*
 * @(#)mp.h: $Revision: 1.9.83.6 $ $Date: 94/04/21 14:12:18 $
 * $Locker:  $
 */

#ifndef	_SYS_MP_INCLUDED	/* allows multiple inclusion */
#define	_SYS_MP_INCLUDED

#ifndef _SYS_TYPES_INCLUDED
#ifdef _KERNEL_BUILD
#    include	"../h/types.h"
#else  /* ! _KERNEL_BUILD */
#    include	<sys/types.h>
#endif /* _KERNEL_BUILD */
#endif	/* _SYS_TYPES_INCLUDED */

#ifdef _KERNEL_BUILD
#include "../machine/mp.h"
#include "../h/assert.h"
#include "../h/vmmeter.h"
#include "../h/spinlock.h"
#else /* ! _KERNEL_BUILD */
#include <machine/mp.h>
#endif	/* _KERNEL_BUILD */

#ifdef	MP
#define	MP_VM
#endif

#ifdef _KERNEL
#if defined(__hp9000s800) || defined(__hp9000s700)
/*
 * Definitions of locations off of the iva to hold per-processor info.
 * The rest of the per-processor info is held in the mpinfo table.
 * ispare holds processor footprint info on startup and panic.
 * Prochpa is in mpproc_info[], which is indexed by getprocindex().
 */

/*
 * This structure must contain only ints (or word length items) so that
 * the alignment before the IVA works out
 */
struct mp_iva {
	int sdta_iir;
	int sdta_regpid;
	int sdta_rctr;
	int sdta_ipsw;
	int sdta_isr;
	int sdta_ior;
	int sdta_pcoq;
	int sdta_flags;
	int pfail_savestate;	/* this is really a (label_t) pointer */
	int pdce_proc;		/* this is really a ptr to a function */
	int assist_config;
	int em_fpu;		/* this is really a pointer */
	int ipsw_savearea;
	int profiling;		/* Per-processor GPROF flag word */
	int crashstack;
	int tmpsavestate;
	int procindex;
	int iva_curpri;
	int iva_noproc;
	int timeinval;
	int globalpsw;
	int topics;
	int ispare;
	int rpb_offset;
	struct mpinfo 	*mpinfo;
	int 		iva_lock_depth;		/* processor_lock */
	unsigned int	iva_entry_psw_mask;	/* processor_lock */
	int		iva_spinlock_depth; 	/* spinlock */
	unsigned int	iva_entry_spl_level;	/* spinlock */
	int ibase;
	struct proc * procp;			/* pre-regions this was uptr */
	int ics;
	int ipc;
};

struct cache_info {
   unsigned int size;
   unsigned int conf;
   unsigned int base;
   unsigned int stride;
   unsigned int count;
   unsigned int loop;
} ;

struct tlb_info {
   unsigned int size;
   unsigned int conf;
   unsigned int sp_base;
   unsigned int sp_stride;
   unsigned int sp_count;
   unsigned int off_base;
   unsigned int off_stride;
   unsigned int off_count;
   unsigned int loop;
} ;

struct cache_tlb_info {
/*
 * Currently, these are kept in a single global (versus per processor).
 * This works only as long as we have (strictly) homogeneous MP systems.
 * struct cache_info	I_cache;
 * struct cache_info	D_cache;
 * struct tlb_info	I_tlb;
 * struct tlb_info	D_tlb;
 */
   unsigned int dummy; /* remove when real fields exist */
} ;

struct coproc_info {
   unsigned int ccr_enable;
   unsigned int ccr_present;
} ;

struct model_info {
   unsigned int hversion;
   unsigned int sversion;
   unsigned int hw_id;
   unsigned int boot_id;
   unsigned int sw_id;
   unsigned int sw_cap;
   unsigned int arch_rev;
   unsigned int potential_key;
   unsigned int current_key;
} ;

struct tod_info {
   unsigned int  offset_correction;
   unsigned int  itmr_sync_value;
   double itmr_freq;
   /* Panther Clock Synchronization */
   double freq_ratio; /* itmr_freq of proc/itmr_freq of monarch */
   unsigned int  tod_acc;
   unsigned int  itmr_acc;
} ;

struct pf_info {
   unsigned int pf_state;
   unsigned int itics_remaining;
} ;

#ifdef MP
/*
 * This structure is used in fast_resolvepid() and resume() (both in
 * Assembly language), so you cannot just arbitrarily change it!
 */
#define FAST_PROT_ENTRIES       128

#ifndef LOCORE
struct fast_protid_list {
	space_t sid;
	int protids[FAST_PROT_ENTRIES];
};
#endif	/* not LOCORE */
#endif	/* MP */
#endif	/* s700 || s800 */

/*
/* This structure has per processor information */
struct mpinfo {
	struct processorhpa 	*prochpa;
	int		cpustate;
	unsigned int	blktime;
	struct mp_iva	*iva;
	int		rctr_bit;	/* recovery counter PSW flag */
	int		lock_depth;	/* processor_lock */
	unsigned int	entry_psw_mask;	/* processor_lock */
	int		spinlock_depth; /* spinlock */
	unsigned int	entry_spl_level;/* spinlock */
	void		*held_spinlock;	/* held spinlocks */
	char		*dux_int_pkt;	/* (gag) see dux/dm.c */

#if defined(__hp9000s800) || defined(__hp9000s700)
	struct coproc_info	coproc_info;
	struct model_info	model_info;
	struct tod_info		tod_info;
	struct cache_tlb_info	cache_tlb_info;
	struct pf_info		pf_info;
	unsigned		idlestack_ptr;
#ifdef MP
	struct fast_protid_list fast_protid_list;
#endif	/* MP */
#endif	/* s700 || s800 */
};

/*
 * Extern variable and function declarations.  Must be in
 * ifdef _KERNEL so this file may be included in a user
 * application.
 */
extern	struct	mpinfo	mpproc_info[];

#ifdef	MP
extern	struct	mp_iva	mp_iva0[];
extern	struct	mp_iva	mp_iva1[];
extern	struct	mp_iva	mp_iva2[];
extern	struct	mp_iva	mp_iva3[];

#define SETCURPRI(pri, reg_temp)				\
	{_MFCTL(CR_IVA, reg_temp);				\
	 (((struct mp_iva *)reg_temp)-1)->iva_curpri=pri; }

#define GETCURPRI(reg_temp)				\
	(_MFCTL(CR_IVA, reg_temp),			\
	 (((struct mp_iva *)reg_temp)-1)->iva_curpri)

#define GETPROCINDEX(reg_temp) 		\
	(_MFCTL(CR_IVA, reg_temp), (((struct mp_iva *)reg_temp)-1)->procindex)

#define GETNOPROC(reg_temp) 		\
	(_MFCTL(CR_IVA, reg_temp), (((struct mp_iva *)reg_temp)-1)->iva_noproc)

#define SETNOPROC(proc, reg_temp)				\
	{_MFCTL(CR_IVA, reg_temp);				\
	 (((struct mp_iva *)reg_temp)-1)->iva_noproc=proc; }

#define	ISTACKPTR	getistackptr()
#define	GETMP_ID()	(int)GETIVA()
#define	getmp_id()	getiva()
#define	GETIVA()	(uniprocessor ? mp_iva0 : getiva())
extern	struct	mpinfo	*getproc_info();
extern	struct	mp_iva	*getiva();

#define getiva_data()	(getiva()-1)

#ifdef __hp9000s700
#define	uniprocessor 1
#else
extern	int	uniprocessor;		/* are we a uniprocessor? */
#endif	/* s700 */
#else	/* ! MP */
/*
 *  Here are definitions for when we are NOT MP
 */
#define	ISTACKPTR	istackptr
#define	GETNOPROC(X)	noproc
#define	GETMP_ID()	getmp_id()
#define	getmp_id()	(getprocindex()+1)
#define	getprocindex()	0
#define	getproc_info()	(&mpproc_info[getprocindex()])
#ifdef	uniprocessor
/*
 *  Make uniprocessor variable a define for non-MP cases for efficiency
 */
	XXX Error XXX uniprocessor defined
#else	/* ! uniprocessor */
#define	uniprocessor	1		/* we are uniprocessor de-facto */
#endif	/* ! uniprocessor */
#endif	/* ! MP */

/*
 *  Here are definitions for either MP or non-MP
 *    usually they depend on definitions above
 */
#define MP_MONARCH_INDEX	0
#define get_monarch_hpa()	(mpproc_info[MP_MONARCH_INDEX].prochpa)
#define get_my_hpa()		(getproc_info()->prochpa)

#if defined(__hp9000s800) || defined(__hp9000s700)
extern int	*ISTACKPTR;
#define	ON_ICS	(ISTACKPTR == 0)
#endif	/* s700 || s800 */

#ifdef	__hp9000s300
extern int interrupt;
extern int mainentered;
#define ON_ICS (!mainentered || (interrupt != 0))
#endif	/* __hp9000s300 */

/*
 *  Some macros to TRY to make reading ifdef'd code easier
 */
#if defined(__hp9000s800) || defined(__hp9000s700)
#define	S800(X)	X
#else	/* ! s700 || s800 */
#define	S800(X)
#endif	/* ! s700 || s800 */

#ifdef	__hp9000s300
#define	S300(X)	X
#else	/* ! __hp9000s300 */
#define	S300(X)
#endif	/* ! __hp9000s300 */
#endif	/* _KERNEL */

/*
 * mpctl commands (temporary and subject to change!)
 */

#define	MPC_GETNUMSPUS		1
#define	MPC_GETFIRSTSPU		2
#define	MPC_GETNEXTSPU		3
#define	MPC_GETCURRENTSPU	4
#define	MPC_SETPROCESS		7

#ifndef	_KERNEL
/*
 * mpctl convenience macros (temporary and subject to change!)
 */

#define	GETNUMSPUS()	\
	syscall(SYS_MPCTL, MPC_GETNUMSPUS)

#define	GETFIRSTSPU()	\
	syscall(SYS_MPCTL, MPC_GETFIRSTSPU)

#define	GETNEXTSPU(spu)	\
	syscall(SYS_MPCTL, MPC_GETNEXTSPU, spu)

#define	GETCURRENTSPU()	\
	syscall(SYS_MPCTL, MPC_GETCURRENTSPU)

#define	SETPROCESS(sgid, pid)	\
	syscall(SYS_MPCTL, MPC_SETPROCESS, sgid, pid)

#endif /* not _KERNEL */

#endif	/* ! _SYS_MP_INCLUDED */
