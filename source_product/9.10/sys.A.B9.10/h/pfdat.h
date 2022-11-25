/*
 * @@(#)pfdat.h: $Revision: 1.9.83.5 $ $Date: 94/05/05 15:29:01 $
 * $Locker:  $
 */

#ifdef _KERNEL_BUILD
#include "../h/sema.h"
#include "../machine/hdl_pfdat.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sema.h>
#include <machine/hdl_pfdat.h>
#endif /* _KERNEL_BUILD */


typedef struct pfdat {
	struct pfdat	*pf_hchain;	/* Hash chain link (MUST BE FIRST) */
#ifndef PFDAT32
	unsigned	pf_flags : 8,	/* Flags portion.	*/
			pf_fill : 3,
			pf_pfn : 21;	/* phys page #, saves divides */
	cnt_t		pf_use;		/* share use count	*/
#endif
	struct vnode	*pf_devvp;	/* Vnode for device.	*/
	unsigned	pf_data;	/* Disk block nummber.	*/
	struct pfdat	*pf_next;	/* Next free pfdat.	*/
	struct pfdat	*pf_prev;	/* Previous free pfdat.	*/
#ifdef PFDAT32
	unsigned	pf_locked : 1,
			pf_wanted : 1,
			pf_use : 14;		/* share use count	*/
#else
	vm_sema_t	pf_lock;	/* lock pfdat entry	*/
#endif	
	/* alignment of following field is critical to s300, see hdl_pfdat.h */
	struct hdlpfdat pf_hdl;		/* HDL specific info for pfdat */
} pfd_t;

#ifdef PFDAT32

#define pf_flags	pf_hdl.pf_bits

#define	P_QUEUE		0x0100	/* Page on free queue		*/
#define	P_BAD		0x0200	/* Bad page (ECC error, etc.)	*/
#define	P_HASH		0x0400	/* Page on hash queue		*/
#define P_SYS		0x1000	/* Page being used by kernel	*/
#define P_HDL		0x2000	/* Page being used by HDL	*/

#else

#define	P_QUEUE		0x01	/* Page on free queue		*/
#define	P_BAD		0x02	/* Bad page (ECC error, etc.)	*/
#define	P_HASH		0x04	/* Page on hash queue		*/
#define P_SYS		0x10	/* Page being used by kernel	*/
#define P_HDL		0x20	/* Page being used by HDL	*/
#define P_DMEM          0x40    /* Page begin used by DMEM      */
#define P_LCOW          0x80    /* Page being remapped by lcow  */

#endif

#define PF_HASH(data)		((data >> 5) + data)

#ifdef _KERNEL
extern vm_lock_t	pfdat_lock;
extern vm_lock_t	pfdat_hash;
/* extern vm_lock_t	mem_wait; */
extern pfd_t phead;
extern pfd_t pbad;
extern pfd_t *pfdat;
extern pfd_t **phash;
extern pfd_t ptfree;
extern int phashmask;
extern int phead_cnt;
pfd_t	*hash_insert();
pfd_t	*hash_peek();
pfd_t	*addtocache();
pfd_t	*pageincache();
pfd_t	*allocpfd();

#define BLKNULL		0	/* pf_blkno null value		*/

/*
 * Lock for free list.
 */
#define pfdatlstlock()		vm_spinlock(pfdat_lock)
#define pfdatlstunlock()	vm_spinunlock(pfdat_lock)
/*
 * Lock for hash list.
 */
#define hashlock()		vm_spinlock(pfdat_hash)
#define hashunlock()		vm_spinunlock(pfdat_hash)

/*
 * Pfdat entry lock.  
 * Two macros exist for locking the entry.  The first takes a pointer to 
 * the pfdat structure and the second takes the page frame number.
 * They are the same semaphore with different ways to reference them.
 */
#ifndef	PFDAT32

#define pfnlock(PFN)		vm_psema(&pfdat[(PFN)].pf_lock, PMEM)
#define pfdatlock(PFD)		vm_psema(&(PFD)->pf_lock, PMEM)
#define cpfnlock(PFN)		(vm_cpsema(&pfdat[(PFN)].pf_lock))
#define cpfdatlock(PFD)		(vm_cpsema(&(PFD)->pf_lock))

#define pfnunlock(PFN)		{ \
			 	vm_vsema(&pfdat[(PFN)].pf_lock, 0); \
			 	if ((phead_cnt) && (pfdat[(PFN)].pf_flags & P_QUEUE)) \
			 		wakeup( (caddr_t)&phead); \
}

#define pfdatunlock(PFD) 	{ \
			 	vm_vsema(&(PFD)->pf_lock, 0); \
			 	if ((phead_cnt) && ((PFD)->pf_flags & P_QUEUE)) \
			 		wakeup( (caddr_t)&phead); \
}

#define pfndisown(PFN) 		vm_disownsema(&pfdat[(PFN)].pf_lock);
#define pfdatdisown(PFD) 	vm_disownsema(&(PFD)->pf_lock);

#else

#define pfnlock(PFN)		pfd_lock(&pfdat[(PFN)])
#define pfdatlock(PFD)		pfd_lock(PFD)
#define cpfnlock(PFN)		pfd_test_lock(&pfdat[(PFN)])
#define cpfdatlock(PFD)		pfd_test_lock(PFD)
#define pfnunlock(PFN)		pfd_unlock(&pfdat[(PFN)])
#define pfdatunlock(PFD)	pfd_unlock(PFD)
#define pfdatdisown(PFD)	
#define pfd_is_locked(PFD)	((PFD)->pf_locked)
#endif	

/*
 * pageinhash return true if a given page is in the hash
 * and flase if it is not.
 */
#ifndef OSDEBUG
#define PAGEINHASH(PFD) ( PFD->pf_flags&P_HASH )
#define PAGEONFREE(PFD) ( PFD->pf_flags&P_QUEUE )
#else
#define PAGEINHASH(PFD) pageinhash(PFD)
#define PAGEONFREE(PFD) pageonfree(PFD)
#endif

#define	FREEPAGES()	(phead.pf_next != &phead)

#ifdef FDDI_VM

/*
 * lost page list data structure contains a field for the phsical page it
 * is keeping track of, and a forward and backward pointer so we can quickly
 * add new entries to the end of the list and easily take entries off of any
 * part of the list.  (Also, the actual size we will get from MALLOC will
 * be at least 16 bytes and most likely 32 or even larger, so not really
 * wasting space here.) 
 */

typedef struct lost_page {
	struct lost_page *next;
	struct lost_page *prev;
	int page;
} lost_page_t;

#endif /* FDDI_VM */

#endif /* _KERNEL */
