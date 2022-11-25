/*
 * @(#)pregion.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 18:32:13 $
 * $Locker:  $
 */

#ifndef _PREGION_INCLUDED
#define _PREGION_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/sema.h"
#include "../h/mman.h"
#include "../h/region.h"
#include "../machine/hdl_preg.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sema.h>
#include <sys/mman.h>
#include <sys/region.h>
#include <machine/hdl_preg.h>
#endif /* _KERNEL_BUILD */

/*	Each process has a number of pregions which describe the
 *	regions which are attached to the process.
 *	%%% For maximum efficency, pregions should be on a doubly
 *	    linked list.  This is avoided because of the extra
 *	    memory overhead (trading off CPU for memory).
 */

/*
 * Virtual address space structure
 */
struct p_lle {
	struct pregion *lle_next;	/* First pregion in list */
	struct pregion *lle_prev;	/* Last pregion in list */
};

typedef struct pregion {
	struct p_lle p_ll;	/* Linked list of pregions in vas */
#define p_next p_ll.lle_next
#define p_prev p_ll.lle_prev
	short	p_flags;	 
	short	p_type;		
	reg_t	*p_reg;		/* Pointer to the region. */
	space_t	p_space;	/* virtual space for region */
	caddr_t	p_vaddr;	/* virtual offset for region */
	size_t	p_off;		/* offset in region */
	size_t	p_count;	/* number of pages mapped by pregion */
	short	p_prot;		/* protection ID of region */
	ushort	p_ageremain;	/* remaining number of pages to age */
	size_t	p_agescan;	/* index of next scan for vhand's age hand */
	size_t	p_stealscan;	/* index of next scan for vhand's steal hand */
	struct vas *p_vas;	/* Pointer to vas we're under */
	struct pregion *p_forw; /* Active chain of pregions */
	struct pregion *p_back; 
	struct pregion *p_prpnext; /* list of pregions off region */
	struct pregion *p_prpprev; /* list of pregions off region */
	size_t p_lastfault;	/* last page faulted by this pregion */
	size_t p_lastpagein;	/* last page-in scheduled for this pregion */
	short p_trend_diff;	/* difference between last two page faults */
	ushort p_trend_strength;/* number of times p_trend_diff was the same */
	struct hdlpregion p_hdl;/* HDL specific info for pregion */
} preg_t;

/*	Pregion flags.
 */

#define PF_ALLOC	0x0001		/* Pregion allocated 		*/
#define	PF_MLOCK	0x0002		/* region is memory locked	*/
#define PF_EXACT	0x0004		/* map pregion exactly 		*/
#define PF_ACTIVE	0x0008		/* Pregion on active chain	*/
#define	PF_NOPAGE	0x0010		/* Pregion locked against paging */
					/* either another pregion is    */
					/* responsible for paging this  */
					/* region or we don't want it   */
					/* paged (UAREA and NULLDREF)   */
#define PF_NOMAP	0x0020		/* Translations should not be	*/
					/* resolved through this preg	*/
					/* by HIL code (for priveleged	*/
					/* shared libraries).		*/
#define PF_PUBLIC	0x0040		/* May be public (for shared	*/
					/* libraries)			*/
#define PF_DAEMON	0x0080          /* pregion is for kernel daemon */
#define PF_WRITABLE	0x0100		/* May grant write access to	*/
					/* pages.			*/
#define PF_INHERIT	0x0200		/* Inherit across exec()	*/
#define PF_VTEXT	0x0400		/* vnode was marked as VTEXT	*/
#define PF_MMFATTACH    0x0800		/* MMF pregion is being attached*/

#define PREGMLOCKED(PRP)	(PRP->p_flags & PF_MLOCK)

/*	Pregion types.
 */

#define	PT_UNUSED	0		/* Unused pregion.		 */
#define PT_UAREA	1		/* U area			 */
#define	PT_TEXT		2		/* Text region.			 */
#define	PT_DATA		3		/* Data region.			 */
#define	PT_STACK	4		/* Stack region.		 */
#define	PT_SHMEM	5		/* Shared memory region.	 */
#define PT_NULLDREF	6		/* Null pointer dereference page */
#define PT_LIBTXT	7		/* shared library text region    */
#define PT_LIBDAT	8		/* shared library data region    */
#define PT_SIGSTACK	9		/* signal stack			 */
#define PT_IO		10		/* I/O region 			 */
#define PT_MMAP		11		/* Memory mapped file		 */
#define PT_GRAFLOCKPG	12		/* Framebuffer lock page	 */
#define PT_NTYPES	13		/* Total # pregion types defined */


/*
 * Given the pregion and an address determine the offset in the region.
 */
#define regindx(PRP, ADDR) \
	(btop((ptob(btop((u_int)ADDR)) - (u_int)(PRP)->p_vaddr))+(PRP)->p_off)


#ifdef _KERNEL

/*
 * Flag parameters for calling VOP_PAGEOUT.
 */
#define PAGEOUT_HARD	0x01 /* Take all the pages you can, wait for locks */
#define PAGEOUT_FREE	0x02 /* Free the pages once the I/O is complete */
#define PAGEOUT_WAIT	0x04 /* Wait for the I/O to complete */
#define PAGEOUT_VHAND	0x08 /* Pageout request by vhand */
#define PAGEOUT_SWAP	0x10 /* Pageout request by swapper */
#define PAGEOUT_PURGE	0x20 /* purge pages from the page cache */

/* Flags for growpreg() */
#define ADJ_REG	1		/* Adjust region size to match pregion */

extern vm_lock_t plistlock;	/* pregion list lock */

void		preginit();	/* Init pregions */
void		hdl_preginit();	/* HDL version of preginit() */
preg_t		*allocpreg();	/* Allocate a pregion */
void		freepreg();	/* Free a pregion from a process. */
void		activate_preg();/* Activate a pregion */
void		deactivate_preg(); /* Deactivate a pregion */
preg_t		*duppregion();	/* Duplicate pregion (fork). */
int		chkattach();	/* Legal attach ? */
int		mlockpreg();	/* Lock preg in memory */
void		munlockpreg();	/* Unlock preg from memory */
preg_t		*attachreg();	/* Attach region to process. */
void		detachreg();	/* Detach region from process. */
void		add_pregion_to_region(); /* does just what it says */
extern u_long 	hdl_page_mprot();     /* MPROT_* for a single page */
extern void	hdl_mprotect();	      /* mprotect() a page range */
extern int	hdl_range_mapped();   /* is a page range mapped? */
extern int	hdl_range_unmapped(); /* is a page range unmapped? */
extern void	hdl_mprot_dup();      /* duplicate mprotect() data */

/*
 * Constants to store page-range protections for mprotect()
 */
#define MPROT_UNMAPPED	0x0	/* page-range is no longer mapped */
#define MPROT_NONE	0x1	/* page-range is not readable or writable */
#define MPROT_RO	0x2	/* page-range is read-only */
#define MPROT_RW	0x3	/* page-range is read-write */

# define	plstlock()	{vm_spinlock(plistlock);}
# define	plstunlock()	{VASSERT(vm_valulock(plistlock) <= 0); \
				 vm_spinunlock(plistlock);}
/* Some utility macros */
#define prpcontains(prp, space, vaddr) \
	(((space == prp->p_space) || (space == (u_int)-1)) && \
	 ((vaddr >= prp->p_vaddr) &&  \
	  (vaddr <= (prp->p_vaddr+ptob(prp->p_count) - 1))))

#define prpgreaterthan(prp, space, vaddr) \
	 ((prp->p_space > space) || \
	 ((prp->p_space == space)&&(prp->p_vaddr > vaddr)))

#define prpcontains_exact(prp, space, vaddr, match_minus1) (	\
	  (vaddr >= prp->p_vaddr) &&				\
	  (vaddr < prp->p_vaddr + ptob(prp->p_count)) &&	\
	  ((match_minus1) ? 					\
		  ((space == prp->p_space) || (space == (space_t)-1)) : \
		  ((space == prp->p_space))			\
	  )							\
	)

#define	FPRP_LOCK	0x1
#define	FPRP_COND	0x2
#define	FPRP_MINUS1	0x4

#define findprp_exact(vas, space, vaddr)	\
	findpreg(vas, space, vaddr, FPRP_LOCK)

#define findprp(vas, space, vaddr)	\
	findpreg(vas, space, vaddr, FPRP_LOCK|FPRP_MINUS1)

#define searchprp_exact(vas, space, vaddr)	\
	findpreg(vas, space, vaddr, 0)

#define searchprp(vas, space, vaddr)	\
	findpreg(vas, space, vaddr, FPRP_MINUS1)

#define csearchprp_exact(vas, space, vaddr)	\
	findpreg(vas, space, vaddr, FPRP_COND)

#define csearchprp(vas, space, vaddr)	\
	findpreg(vas, space, vaddr, FPRP_COND|FPRP_MINUS1)

#ifdef	LATER

# define	plstlock()	{u.u_procp->p_flag |= SLOCK; \
				 vm_spinlock(plistlock);}
# define	plstunlock()	{VASSERT(vm_valulock(plistlock) <= 0); \
				 vm_spinunlock(plistlock); \
				 u.u_procp->p_flag &= ~SLOCK;}
#endif	/* LATER */
#endif /* _KERNEL */
#endif /* _PREGION_INCLUDED */
