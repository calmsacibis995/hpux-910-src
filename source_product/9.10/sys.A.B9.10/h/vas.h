/*
 * @(#)vas.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 18:38:18 $
 * $Locker:  $
 */

#ifndef _VAS_INCLUDED
#define _VAS_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/file.h"
#include "../h/pregion.h"
#include "../machine/hdl_vas.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/file.h>
#include <sys/pregion.h>
#include <machine/hdl_vas.h>
#endif /* _KERNEL_BUILD */

#define	VA_CACHE_SIZE	1

struct vas {
	struct p_lle va_ll;	/* Doubly linked list of pregions */
#define va_next va_ll.lle_next
#define va_prev va_ll.lle_prev
	preg_t *va_cache[VA_CACHE_SIZE];
	int	va_refcnt;	/* Number of pointers to this vas */
	vm_sema_t va_lock;	/* Lock structure */
	u_int va_rss;		/* Cached approx. of shared res. set size */
	u_int va_prss;		/* Cached approx. of private RSS (in mem) */
	u_int va_swprss;	/* Cached approx. of private RSS (on swap) */
	u_long va_flags;	/* various flags */
	struct file *va_fp;	/* file table entry for MMFs psuedo-vas */
	u_long va_wcount;	/* count of writable MMFs sharing psuedo-vas */
	struct proc *va_proc;	/* pointer to process, if there is one */
	struct hdlvas va_hdl;	/* HW Dependent info for vas */
};

typedef struct vas vas_t;  /* this needs to be visible to compile proc.h */

/*
 * Values for va_flags
 */
#define VA_HOLES     0x00000001	/* vas may have holes within pregions */
#define VA_IOMAP     0x00000002	/* there may be an iomap pregion in the vas */
#define VA_NOTEXT    0x00000004	/* No text region in vas (EXEC_MAGIC a.out) */

#ifdef _KERNEL

#define	VA_INS_CACHE(vas, prp)						\
{									\
	(vas)->va_cache[0] = prp;					\
}

#define	VA_KILL_CACHE(vas)						\
{									\
	(vas)->va_cache[0] = (preg_t *)vas;					\
}


void		vasinit();	/* Initialize the vas table */
void		hdl_vasinit();	/*  HDL initialization */
vas_t		*allocvas();	/* Allocate a vas */
vas_t		*dupvas();	/* Duplicate a vas */
void		freevas();	/* Free a vas */
void		insertpreg();	/* Insert a pregion into a vas */
preg_t		*findpreg();	/* Find pregion from va */
reg_t		*searchreg();	/* Find region from va, rtn unlocked */
reg_t		*findreg();	/* Find region from va, rtn rp locked. */
preg_t		*findpregtype();/* Find pregion of given type, rtn unlocked */
void		kissofdeath();	/* Remove all of proc pregions */
int		dispreg();	/* Remove most of a proc pregions */
int		mlockpregtype();/* lock a particular type of pregion */
void		munlockpregtype();/* Unlock pregion type */
int		textsize();	/* rtn text size of process */
int		ustacksize();	/* rtn size of U stack */
int		datasize();	/* rtn size of data */

#define NOPROCATTACH 0
#define PROCATTACHED 1


#define	vaslock(VAS)	vm_psema(&(VAS)->va_lock, PZERO)
#define	cvaslock(VAS)	vm_cpsema(&(VAS)->va_lock, PZERO)
#define	vasunlock(VAS)	{VASSERT(vm_valusema(&(VAS)->va_lock) <= 0); \
				 vm_vsema(&(VAS)->va_lock, 0);}

#endif /* _KERNEL */
#endif /* _VAS_INCLUDED */
