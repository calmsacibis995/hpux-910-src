/*
 * @(#)bcache.h: $Revision: 1.2.83.5 $ $Date: 93/10/21 13:07:33 $
 * $Locker:  $
 */

#ifndef _MACHINE_BCACHE_INCLUDED
#define _MACHINE_BCACHE_INCLUDED

#define DYNAMIC_BC

#define POINT9_PGS (((1024*1024*1024)*.9)/NBPG)
#define POINT8_PGS (((1024*1024*1024)*.8)/NBPG)

#define BUFFER_VIRTUAL_HIGH_MARK POINT9_PGS
#define BUFFER_VIRTUAL_LOW_MARK POINT8_PGS

extern vm_lock_t bcphys_lock;
extern int binvalcount;
extern u_int bc_adaptive;
extern int gpgslim;
extern int dbc_ceiling;
extern int dbc_cushion;

#ifdef OSDEBUG
extern u_int bcalloc_malloc;
extern u_int bcalloc_inval;
extern u_int bcalloc_unused;
extern u_int bcfree_unused; 
extern u_int bcalloc_number_free;	

#define BCALLOC_MALLOC bcalloc_malloc++;
#define BCALLOC_INVAL bcalloc_inval++;
#define BCALLOC_UNUSED bcalloc_unused++;
#define BCFREE_UNUSED bcfree_unused++; 
#define BCALLOC_NUMBER_FREE_INC bcalloc_number_free++;	
#define BCALLOC_NUMBER_FREE_DEC bcalloc_number_free--;	

#else

#define BCALLOC_MALLOC
#define BCALLOC_INVAL
#define BCALLOC_UNUSED
#define BCFREE_UNUSED
#define BCALLOC_NUMBER_FREE_INC
#define BCALLOC_NUMBER_FREE_DEC

#endif /* OSDEBUG */


/***************************************************
 * Buffer cache Virtual memory allocation routines.
 **************************************************/
extern struct map *bufmap;

/******************************************
 * Buffer cache Physical Page Pool Macros
 ******************************************/
/* bcalloc can return NULL (in pfd) if it fails to allocate a page of memory.
 * bcfree always succeeds.  Could optimize these by returning/getting more 
 * than one page of memory at a time.
*/

#define PAGE_POOL_EMPTY	(bhead.pf_next == &bhead)

#ifdef DYNAMIC_BC

#define MEMORY_PRESSURE ((dbc_vhandcredit<dbc_stealavg) && (freemem<gpgslim))
#define BUFFER_PAGES_AVAILABLE (dbc_invalcount() > 0)

/* Make use of idle pages first.  These can be found on a linked list of pfdat
 * entries, or in buffers on the free list which are marked B_INVAL.  Note
 * that race conditions could exist with binvalcount.  Its okay if we make 
 * the wrong decsision once in a while; not optimal, but not fatal.  Note that
 * dbc_kdget won't wait for memory and can return NULL.
*/
#define dux_adjust_bufpages(size)  {       		\
    register u_int context;                    		\
    SPINLOCK_USAV(bcphys_lock, context);   		\
    bufpages += size;		 			\
    SPINUNLOCK_USAV(bcphys_lock, context);  		\
}

#define bcalloc_init(pfd)  {                   		\
    register u_int context;                    		\
    if ((pfd = dbc_kdget()) != (pfd_t *)NULL) {		\
        BCALLOC_MALLOC					\
        SPINLOCK_USAV(bcphys_lock, context);   		\
	bufpages++;			 		\
        SPINUNLOCK_USAV(bcphys_lock, context);  	\
   }							\
}

	
#define bcalloc(pfd)  {                     		\
    register u_int context;                    		\
						 	\
    SPINLOCK_USAV(bcphys_lock, context);             	\
    if (bhead.pf_next != &bhead) {                   	\
    	VASSERT(bhead.pf_next != &bhead);        	\
    	(pfd) = bhead.pf_next;                   	\
    	bhead.pf_next = (pfd)->pf_next;          	\
	BCALLOC_NUMBER_FREE_DEC				\
	BCALLOC_UNUSED					\
        SPINUNLOCK_USAV(bcphys_lock, context);          \
    } else {                                         	\
	SPINUNLOCK_USAV(bcphys_lock, context);          \
        if (   (bufpages >= dbc_ceiling)		\
            || ((freemem - lotsfree) < dbc_cushion)	\
	    || BUFFER_PAGES_AVAILABLE 			\
	    || MEMORY_PRESSURE) {			\
	    BCALLOC_INVAL				\
    	    pfd = (pfd_t *)NULL;			\
        } else {					\
           if ((pfd = dbc_kdget()) != (pfd_t *)NULL) {	\
	        BCALLOC_MALLOC				\
                SPINLOCK_USAV(bcphys_lock, context);   	\
    		bufpages++;			 	\
	        SPINUNLOCK_USAV(bcphys_lock, context);  \
	   }						\
        } 						\
    }							\
}

#else

#define bcalloc(pfd)  {                                  \
	register u_int context;                          \
							 \
	SPINLOCK_USAV(bcphys_lock, context);             \
	if (bhead.pf_next == &bhead) {                   \
		(pfd) = (pfd_t *)NULL;                   \
	} else {                                         \
		VASSERT(bhead.pf_next != &bhead);        \
		(pfd) = bhead.pf_next;                   \
		bhead.pf_next = (pfd)->pf_next;          \
	}                                                \
	SPINUNLOCK_USAV(bcphys_lock, context);           \
     }
#endif /* DYNAMIC_BC */

#define bcfree(pfd) {                                    \
        register u_int context;                          \
                                                         \
        SPINLOCK_USAV(bcphys_lock, context);             \
        VASSERT((pfd) != NULL);                          \
	BCALLOC_NUMBER_FREE_INC				 \
	BCFREE_UNUSED 					 \
        (pfd)->pf_next = bhead.pf_next;                  \
        bhead.pf_next = (pfd);                           \
        SPINUNLOCK_USAV(bcphys_lock, context);           \
        }

#define BC_MAPPAGE(vaddr,pfn) \
	hdl_addtrans(KERNPREG, KERNELSPACE, vaddr, 0, pfn);

#endif /* ! _MACHINE_BCACHE_INCLUDED */
