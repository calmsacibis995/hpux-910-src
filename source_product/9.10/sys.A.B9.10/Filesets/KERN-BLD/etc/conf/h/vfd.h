/*
 * @(#)vfd.h: $Revision: 1.10.83.6 $ $Date: 94/09/14 19:34:51 $
 * $Locker:  $
 */

#ifndef _VFD_INCLUDED
#define _VFD_INCLUDED

/* The chunk of VFD/DBDs requires the DBD type to be defined */
#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/dbd.h"
#include "../machine/vmparam.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/dbd.h>
#include <machine/vmparam.h>
#endif /* _KERNEL_BUILD */

extern int firstfree;
#ifdef OSDEBUG
extern int pfdatmax;
#endif

/*
 * Virtual Frame descriptor definition.
 */
struct vfd {
	unsigned pg_v : 1,	/* Valid */
		pg_cw : 1,	/* copy on write page */
		pg_lock : 1,	/* page locked	*/
		pg_fill : 8,	/* padding */
		pg_pfnum : 21;	/* Page frame number */
#define	pg_pfn	pg_pfnum
};

/*
 * A not so good C coding method for clearing the entire vfd in 
 * one operation.
 */
typedef union svfd {
	struct vfd	pgm;
	struct {
		int	pg_vfd;
	} pgi;
} vfd_t;

/*
 * Calling parameters for the hdl_[get|set|unset] routines.
 */
#define VPG_MOD 1
#define VPG_REF 2

/*
 * External declarations for routines
 */
#ifdef _KERNEL
caddr_t		kdvmap();
caddr_t		kdalloc();
vfd_t		*findvfd();
extern caddr_t	kmembase;
extern caddr_t  *kmeminfo;

#ifdef OSDEBUG
#define PAGE_INFO(x) ((caddr_t *)page_info((x))) 
#else
#define PAGE_INFO(x)  (&kmeminfo[(hdl_vpfn(KERNELSPACE,(x))) - firstfree])
#endif
#endif /* _KERNEL */

/*
 * These structures and definitions are private to vm_vfd.c.  However,
 * they are provided here so that diagnostic tools can be built with
 * knowledge of them.
 */
#define B_ORD (29)		/* Order of btree 
				 * must be ODD!!! 
				 */

#define B_ORDSHIFT  (3)		/* Shift value for the closest power of 2 
				 * to B_ORD/2 (nodes probably only half
				 * full) that is that is still smaller 
				 * than B_ORD. 
				 */	

#define B_ORD2 (31)		/* 
				 * Closet power of 2 to B_ORD plus
				 * closet power of 2 to B_ORD  minus 1.
				 * eg. for 29, closest power of 2 is
				 * 16.  B_ORD2 = 16+15
				 */

#define B_SIZE (B_ORD+1)	/* Allocate with extra element to make 
				 * insertions simpler & more regular.
				 */

#define B_DEPTH (10)		/* Max depth of tree. 
				 * Tree can hold roughly (B_ORD/2)^B_DEPTH 
				 * chunks.
				 */


/* A cluster of vfd/dbd's */
struct vfddbd {
	vfd_t c_vfd;
	dbd_t c_dbd;
};

/* Chunks of vfd/dbd's allocated together */
#define CHUNKENT (32) 

/* Power of CHUNKENT */
#define CHUNKSHIFT (5)


#define CHUNKROUND(x) ((x) & ~(CHUNKENT-1))
typedef struct vfddbd chunk_t[CHUNKENT];
#define COFFSET(x) ((x) & (CHUNKENT-1))
#define CINDEX(x) ((x) >> 5)
#define REGIDX(x) ((x) << 5)

/* r_protoidx value indicating that it isn't used */
#define UNUSED_IDX (0x7FFFFFFF)
#define DONTUSE_IDX (0x7FFFFFFE)

struct chunk_list {
	chunk_t *cp;
	struct chunk_list *clp;
};

#ifdef _KERNEL
chunk_t *alloc_chunk();
#endif

/*
 * Data structures for implementing sparse record of areas which are
 * to be set copy-on-write
 */
#define MAXVPROTO (5)
struct vfdcw {
	int v_start[MAXVPROTO];
	int v_end[MAXVPROTO];
};

/*
 * B_CACHE_SIZE hardcoded in FINDENTRY for performance reasons.
 */
#define	B_CACHE_SIZE	2

/*
 * Root node of a btree
 */
struct broot {
	struct bnode	/* Pointer to root node */
		*b_root;
	int b_depth;	/* How far down to leaf nodes */
	int b_npages;	/* # pages under this btree */
	int b_rpages;	/* # swap pages reserved for this btree */
	struct chunk_list *b_list;  /* list of allocated chunks */
	int b_nfrag;	/* # free fragments left in this page */
	struct region
		*b_rp;	/* Region this tree is for */
	int b_protoidx;	/* Index at which we switch from proto1 to proto2 */
	dbd_t		/* Prototype DBD values */
		b_proto1,
		b_proto2;
	struct vfdcw	/* Prototype copy-on-write values for VFD */
		b_vproto;
	unsigned long b_key_cache[B_CACHE_SIZE]; /* see FINDENTRY */
	caddr_t	b_val_cache[B_CACHE_SIZE];
};

/* Interior nodes of btree */
/*
 * Be careful about the size:
 *	- It must be the same size as a chunk_t
 *	- It must be a power of two
 */
struct bnode {
	unsigned long b_key[B_SIZE];	/* Keys for each index */
	int b_nelem;		/* # keys stored */
	struct bnode		/* Pointers "between" each key */
	*b_down[B_SIZE+1];
	int b_scr1, b_scr2;	/* Trailers to make it come to a power of 2 */
	/* For B_ORD==29, this is 256 bytes */
};


#define DEFAULT_DBD(RP, IDX)  \
	(((IDX) >= ((RP)->r_root->b_protoidx))? \
	 ((RP)->r_root->b_proto2): \
	 ((RP)->r_root->b_proto1)) 

struct broot	*bt_init();	/* Create root of btree */


/*
 * Macros for findvfd and friends
 *
 */

/*
 * helper macros
 */
#define BTKEY(rp, cacheline)				\
	((rp)->r_root->b_key_cache[cacheline])

#define BTCACHECHUNK(rp, cacheline) 			\
(*(chunk_t *)((rp)->r_root->b_val_cache[cacheline]))

#define BTCACHEVFDP(rp, idx, cacheline) 		\
(&(BTCACHECHUNK(rp, cacheline)[COFFSET(idx)].c_vfd))

#define BTCACHEDBDP(rp, idx, cacheline) 		\
(&(BTCACHECHUNK(rp, cacheline)[COFFSET(idx)].c_dbd))


#define FINDVFD(rp, idx) (				\
    (BTKEY(rp, 0) == CINDEX(idx)) ?			\
        BTCACHEVFDP(rp, idx, 0) :			\
	(						\
	    (BTKEY(rp, 1) == CINDEX(idx)) ?		\
	        BTCACHEVFDP(rp, idx, 1) :		\
		    findvfd(rp, idx)			\
	)						\
)

#define FINDDBD(rp, idx) (				\
    (BTKEY(rp, 0) == CINDEX(idx)) ?			\
        BTCACHEDBDP(rp, idx, 0) :			\
        (						\
	    (BTKEY(rp, 1) == CINDEX(idx)) ?		\
	        BTCACHEDBDP(rp, idx, 1) :		\
		    finddbd(rp, idx)			\
	)						\
)

#define FINDENTRY(rp, idx, vfdp, dbdp) (		\
(BTKEY(rp, 0) == CINDEX(idx)) ?				\
    (							\
        (*(vfdp) = BTCACHEVFDP(rp, idx, 0)),		\
        (*(dbdp) = BTCACHEDBDP(rp, idx, 0)),		\
        (1)						\
    ) :							\
    (							\
	(BTKEY(rp, 1) == CINDEX(idx)) ?			\
            (						\
                (*(vfdp) = BTCACHEVFDP(rp, idx, 1)),	\
                (*(dbdp) = BTCACHEDBDP(rp, idx, 1)),	\
                (1)					\
            ) :						\
		(findentry(rp, idx, vfdp, dbdp))	\
    )							\
)


#define VFDFILL_ONE(prp, rp, space, vaddr, vfd)				\
{									\
	pfd_t *pfd = (pfd_t *)allocpfd();				\
	(vfd)->pgm.pg_pfn = pfd - pfdat;				\
	(vfd)->pgm.pg_v = 1;						\
	(rp)->r_nvalid++;						\
	/* removed hdl_addtrans() -jkr */				\
}


#define VFDMEM_ONE(prp, rp, space, vaddr, pgindx, vfd, restart)		\
{									\
	VASSERT(vaddr==(prp->p_vaddr + ptob(pgindx - prp->p_off)));	\
	if (cmemreserve(1)) {						\
		VFDFILL_ONE(prp, rp, space, vaddr, vfd);		\
		restart = 0;						\
   	} else {							\
		restart = vfdmemall(prp, pgindx, 1);			\
	}								\
}
#endif /* _VFD_INCLUDED */
