/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_vfd.c,v $
 * $Revision: 1.12.83.6 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/09/16 16:58:10 $
 */

/*
 * BEGIN_DESC 
 *
 *   File:
 *	vm_vfd.c
 *
 *   Purpose:
 *	This file implements the abstractions associated with
 *	storing the virtual frame descriptors and the disk
 *	block descriptors.
 *
 *   Interface:
 *	findvfd(rp, i)
 *	finddbd(rp, i)
 *	findentry(rp, i, vfdp, dbdp)
 *	growvfd(rp, amount, type)
 *	shrinkvfd(rp, amount)
 *	dupvfds(srp, drp, off, count)
 *	foreach_chunk(rp, start, cnt, fn, arg)
 *	foreach_entry(rp, start, cnt, fn, arg)
 *	foreach_valid(rp, start, cnt, fn, arg)
 *	vfdswapo(rp)
 *	vfdswapi(rp)
 *	vfd_setcw(rp, idx, cnt)
 *
 *   Theory:
 *
 *	The current implementation of storing vfd/dbd is a B-tree.
 *	Most of the system does not know about this complex data
 *	structure.  Only code in this file understands the B-tree
 *	data structure.  Each region either contains a single 
 *	array of vfd/dbd (called a chunk) or it contains a pointer
 *	to a B-tree.
 * END_DESC
 */

#define _VFD_INTERNALS
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/malloc.h"
#include "../h/vfd.h"
#include "../h/dbd.h"
#include "../h/region.h"
#include "../h/buf.h"
#include "../machine/vmparam.h"
#include "../h/swap.h"
#include "../h/vmmac.h"
#undef _VFD_INTERNALS

/*
 * Macros for inserting and killing the vfd/dbd 
 * cache.
 */
#define	BT_INS_CACHE(br, key, val)			\
{							\
   int 	i;						\
   for (i=B_CACHE_SIZE-1; i>0; i--) {			\
	(br)->b_key_cache[i] = (br)->b_key_cache[i-1];	\
	(br)->b_val_cache[i] = (br)->b_val_cache[i-1];	\
   }							\
   (br)->b_key_cache[0] = (key);			\
   (br)->b_val_cache[0] = (caddr_t)(val);		\
}

#define	BT_KILL_CACHE(br)				\
{							\
   int 	i;						\
   for (i=0; i<B_CACHE_SIZE; i++) {			\
	(br)->b_key_cache[i] = (unsigned long) UNUSED_IDX;	\
   }							\
}

/*
 * Macros for finding overlapping ranges for storing the vfd
 * values.
 */
#define CONTAINS(start, end, idx) (((idx) >= (start)) && ((idx) <= (end)))
#define OVERLAPS(x, y, m, n) (((n) >= ((x)-1)) && ((y) >= ((m)-1)))


/*
 * Arguments for for_val3 (scan types). 
 */
enum scan {SCAN_VALID, SCAN_ALL};

struct fv3_arg {
	int 	(*fn)();	/* Function to call */
	caddr_t	arg;		/* Argumnet to pass to called function */
	enum scan flags;	/* Type of scan */
};

/*
 * The prototype VFDs for the copy-on-write case and not
 */
struct vfd vfd_cw =   {0, 1, 0, 0, 0};
struct vfd vfd_nocw = {0, 0, 0, 0, 0};

/*
 * Debug variables that exist even when the system is compiled without
 * OSDEBUG.  When vfddebug is set to a postiive value, vfdswapo writes out
 * each page of the B-tree and then reads each page back into the system.  The
 * results are then compared.  If a mismatch occurs, the system panics.  This
 * technique has proven very useful in tracing down certain swapping subsystem
 * issues.  The cost of the runtime check is minimal, thus we left this debug
 * information in.  globalbp1 and globalbp2 are the two global variables that
 * point to the buffer headers used to write and then read the page that
 * caused the panic.
 */
int vfddebug = 0;
struct buf *globalbp1, *globalbp2;

void foreach_entry();

/*
 * BEGIN_DESC 
 *
 * balloc()
 *
 * Return Value:
 *	A pointer to an area of memory to be used as a
 *	bnode.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This routine call alloc_chunk to get a chunk of memory.
 *	It then zeros out the memory and returns it to the caller.
 *
 * Algorithm:
 *	This routine will call alloc_chunk to get memory.
 *	Note: this routine can sleep forever.
 *
 * END_DESC
 */
struct bnode *
balloc(br)
	struct broot *br;
{
	struct bnode *bt = (struct bnode *)alloc_chunk(br->b_rp);
	bzero(bt, sizeof(struct bnode));
	return(bt);
}

#ifdef	hp9000s800
/*
 * BEGIN_DESC 
 *
 * vfd_copy()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function copies an array of bytes from a source
 *	address to a destination address.
 *
 * Algorithm:
 *	Because the pointers might overlap this function
 *	must avoid what is known as a ripple effect.
 *	It does this by comparing the values of the
 *	source and destination pointers as it copies.
 *
 * END_DESC
 */
static void
vfd_copy(src, dest, cnt)
	char *src; 	/* Pointer to source */
	char *dest; 	/* Pointer to destination */
	int cnt;	/* Count in bytes */
{
	/* Avoid "ripple" effect */
	if (src > dest) {
		while (cnt--)
			*dest++ = *src++;
		return;
	}
	dest += cnt;
	src += cnt;
	while (cnt--)
		*--dest = *--src;
}
#endif	/* hp9000s800 */

#ifdef	__hp9000s300
#define	vfd_copy(src,dest,cnt)	overlap_bcopy((src),(dest),(cnt))
#endif	/* __hp9000s300 */

/*
 * BEGIN_DESC 
 *
 * function_name()
 *
 * Return Value:
 *	A pointer to a prototype vfd is returned.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function performs two functions.  One it answers
 *	the question, what is the prototype value for this vfd
 *	and two it returns the index at which the returned 
 *	vfd is no longer the correct prototype.  
 *
 * Algorithm:
 *	This routine (along with vfd_setcw) allows
 *	us to set a range of copy-on-write information
 *	without having to allocate the actual B-tree
 *	entires.  We keep an array of starting
 *	and ending indicies for vfd/dbd paris.
 *
 * In/Out conditions:
 *	The region must be locked on entry.
 *
 * END_DESC
 */
vfd_t *
default_vfd(rp, idx, endidx)
	reg_t *rp;	/* Pointer to region */
	int idx;	/* Offset of vfd in region */
	int *endidx;	/* OUT - ending offset in region */
{
	register int x, start;
	register struct vfdcw *v = &(rp->r_root->b_vproto);
	int save = UNUSED_IDX;

	for (x = 0; x < MAXVPROTO; ++x) {
		if ((start = v->v_start[x]) == -1)
			continue;
		if (CONTAINS(start, v->v_end[x], idx)) {
			*endidx = v->v_end[x];
			return((vfd_t *)&vfd_cw);
		}
		if ((start > idx) && (start < save))
			save = start; 
	}
	*endidx = save - 1;
	return((vfd_t *)&vfd_nocw);
}


/*
 * BEGIN_DESC 
 *
 * default_dbd(rp, idx)
 *
 * Return Value:
 *	A pointer to a prototype dbd.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function return a pointer to a prototype
 *	dbd.  Each region is allowed to prototypes.
 *
 * Algorithm:
 *	Given the index return either the first
 *	prototype value of the second.  Each region is 
 *	allowed two.  Currently only the data region uses
 *	both (initialized data == DBD_FSTORE) and
 *	(bss == DBD_DZERO).
 *
 * END_DESC
 */
dbd_t *
default_dbd(rp, idx)
	reg_t *rp;	/* Region pointer */
	int idx;	/* Index in region */
{
	register struct broot *br = rp->r_root;

	if (idx >= br->b_protoidx)
		return(&br->b_proto2);
	else
		return(&br->b_proto1);
}

/*
 * BEGIN_DESC 
 *
 * bt_init()
 *
 * Return Value:
 *	A pointer to an initialized root of a B-tree.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	bt_init gets memory for the B-tree and initializes it.
 *
 * Algorithm:
 *	Call MALLOC (with sleep) to get memory.
 *	Initialize fields on return.
 *
 * END_DESC
 */
struct broot *
bt_init(rp)
	reg_t *rp;
{
	register struct broot *br;
	register int x;
	struct vfdcw *v;

	/* Allocate memory dynamically */
	MALLOC(br, struct broot *, sizeof(struct broot), M_VFD, M_WAITOK);

	/* Initialize fields */
	br->b_root = NULL;
	br->b_depth = 0;
	br->b_npages = 0;
	br->b_rpages = 0;
#ifdef MALLOC_VFDS	
	br->b_list = (struct chunk_list *)NULL;
#else
	br->b_list = (chunk_t *)NULL;
        br->b_nfrag = 0
#endif
	br->b_rp = rp;
	br->b_protoidx = UNUSED_IDX;
	BT_KILL_CACHE(br);
	v = &(br->b_vproto);
	for (x = 0; x < MAXVPROTO; ++x)
		v->v_start[x] = -1;
	return(br);
}

/*
 * BEGIN_DESC 
 *
 * bt_free()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function free the root of the B-Tree associated
 *	with the region.
 *
 * Algorithm:
 *	Call free_vfdpages to free all the pages 
 *		associated with the tree.
 *	Call FREE to return the space.
 *
 * END_DESC
 */
void
bt_free(rp)
	reg_t *rp; /* A pointer to the region */
{
	/* Throw away each allocated page */
        free_vfdpages(rp->r_root);

        /* Free root of B-tree */
        FREE(rp->r_root, M_VFD);
        rp->r_root = NULL;
        rp->r_pgsz = 0;
}

/*
 * BEGIN_DESC 
 *
 * bt_ins()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Insert the node val with the specified key at the
 *	given level of the B-tree.
 *
 * Algorithm:
 *	First insert the new node in the cache.
 *	If the current tree depth is 0, create the
 *		first bnode and insert the first chunk.
 *	If not, walk the B-tree to find the correct
 *		place to insert the new chunk.
 *
 * END_DESC
 */
void
bt_ins(br, key, val, bstack, depth)
	struct broot *br;		/* Root of tree */
	unsigned long key;		/* New key inserting */
	char *val;			/* Value at this key */
	struct bnode *bstack[];		/* Nodes under which  leaf  exists */
	int depth;			/* Length of bstack[] */
{
	register struct bnode *bt, *bt2;
	register int x, lim;

	BT_INS_CACHE(br, key, val);
	/* Special case: new tree */
	if (br->b_depth == 0) {
		bt = br->b_root = balloc(br);
		bt->b_key[0] = key;
		bt->b_down[1] = (struct bnode *)val;
		bt->b_nelem = 1;
		br->b_depth = 1;
		return;
	}

	/* Repeat while insertions need to be done */
	for (;;) {

		/* Insert key & value into their place */
		bt = bstack[--depth];

		/* Find slot to insert before */
		lim = bt->b_nelem;
		for (x = 0; x < lim; ++x) {
			if (bt->b_key[x] > key)
				break;
		}

		/* Move up elements to insert a new entry */
		if (x < lim) {
			vfd_copy((char *)(bt->b_key+x), 
				 (char *)(bt->b_key+(x+1)),
			    	 (int)(sizeof(unsigned long)*(lim-x)));
			vfd_copy((char *)(bt->b_down+(x+1)), 
				 (char *)(bt->b_down+(x+2)),
				 (int)(sizeof(struct bnode *)*(lim-x)));
		}

		/* Insert the elements */
		bt->b_key[x] = key;
		bt->b_down[x+1] = (struct bnode *)val;

		/* Update count, done if we don't need to split the node */
		if (++(bt->b_nelem) <= B_ORD)
			break;

		/* Need to split the node */

		/* Get a new node, copy the upper half of the old to it */
		bt2 = balloc(br);
		vfd_copy((char *)(bt->b_key+B_SIZE/2), (char *)bt2->b_key,
		    (int)(B_SIZE/2 * sizeof(unsigned long)));
		vfd_copy((char *)(bt->b_down+(B_SIZE/2+1)), 
			 (char *)(bt2->b_down+1),
		    	 B_SIZE/2 * sizeof(struct bnode *));
		bt->b_nelem = bt2->b_nelem = B_SIZE/2;

		/*
		 * If we just split the root, raise the size
		 *  of the tree & finish
		 */
		if (depth == 0) {
			struct bnode *bt3;

			br->b_depth += 1;
			bt3 = br->b_root = balloc(br);
			bt3->b_key[0] = bt2->b_key[0];
			bt3->b_down[1] = bt2;
			bt3->b_down[0] = bt;
			bt3->b_nelem = 1;
			break;
		}

		/* Continue loop at upper level, inserting into parent */
		key = bt2->b_key[0];
		val = (char *)bt2;
	}
}

/*
 * BEGIN_DESC 
 *
 * bt_insert()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function inserts the given chunk into the B-tree.
 *
 * Algorithm:
 *	Search the B-tree for the given key.
 *	It should not be found!  We will then insert
 *	the new chunk with this key at the proper place
 *	in the B-tree.
 *
 *	It seems to me that if the entry is not supposed to be found 
 *	we should panic or return an error.
 *	HP_REVISIT (sol 10/21/90).
 *
 * END_DESC
 */
void
bt_insert(br, key, nval)
	struct broot *br;	/* Root of B-tree */
	unsigned long key;	/* Key for this chunk */
	char *nval;		/* Chunk to insert */
{
	register struct bnode *bt = br->b_root;
	register unsigned long *lp, val;
	register int x;
	int depth = 0;
	struct bnode *bstack[B_DEPTH];

	/* While we have a node to consider, search for key */
	while ((depth < br->b_depth) && bt) {

		/* Record search path down so we can insert if needed */
		bstack[depth++] = bt;

		/* Linear search across */
		lp = bt->b_key;
		for (x = 0; x < bt->b_nelem; ++x) {

			/* If found/passed key value, go down */
			if ((val = *lp++) >= key) {

				/* If leaf, we are at end of search */
				if (depth == br->b_depth) {

					/*
					 * Get exact hit, or it isn't
					 *  in the tree
					 */
					if (val == key) {
						bt->b_down[x+1] =
						    (struct bnode *)nval;
						return;
					}
					goto new;
				}

				/* Descend a level */
				if (val == key)
					x += 1;
				break;
			}

			/* Otherwise, keep going */
		}

		/* Fell off the end, so pick up the last down pointer */
		bt = bt->b_down[x];
	}

new:
	/* Didn't find it, so create a new node into the specified place */
	bt_ins(br, key, nval, bstack, depth);
}

/*
 * BEGIN_DESC 
 *
 * bt_search()
 *
 * Return Value:
 *	A pointer to the desired chunk or NULL if the chunk is not found.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Search the B-tree and return the associated chunk.
 *
 * Algorithm:
 *	Walk each level of the B-tree searching for the key provided.
 *	If found, return a pointer to the chunk.  If not, return NULL.
 *
 *	HP_REVISIT (sol 10/21/90)
 *	Currently this routine is check the cache to see if it contains 
 *	the desired key.  This operation is supposed to be done by macros
 *	that call this rotuine only on failure.  Because not all of the code
 *	was converted to use the macros, this routine is still searching the
 *	cache.  We should remove the cache once everyone is using the new 
 *	macros.
 *
 * END_DESC
 */
char *
bt_search(br, key)
	struct broot *br;	/* Root pointer of B-tree */
	unsigned long key;	/* Key of interest */
{
	register struct bnode *bt = br->b_root;
	register unsigned long *lp, val;
	register int x, depth = br->b_depth;

	/*
	 * See if the key has already been found.
	 */
	if (br->b_key_cache[0] == key)
		return(br->b_val_cache[0]);

	/* 
	 * While we have a node to consider, search for key 
	 */
	while ((depth-- > 0) && bt) {

		/* Linear search across */
		lp = bt->b_key;
		for (x = 0; x < bt->b_nelem; ++x) {

			/* If found/passed key value, go down */
			if ((val = *lp++) >= key) {

				/* If leaf, we have our value */
				if (!depth) {

					/*
					 * Get exact hit, or it
					 *  isn't in the tree
					 */
				   if (val == key) {
				       BT_INS_CACHE(br, key, bt->b_down[x+1]);
				       return((char *)(bt->b_down[x+1]));
				   }
				   return(NULL);
				}

				/* Descend a level */
				if (val == key)
					x += 1;
				break;
			}

			/* Otherwise, keep going */
		}

		/* Fell off the end, so pick up the last down pointer */
		bt = bt->b_down[x];
	}
	return(NULL);
}

/*
 * BEGIN_DESC 
 *
 * fill_chunk()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function initialized the chunk of vfd/dbd to the correct 
 *	values for the given key.
 *
 * Algorithm:
 *	We maintain to default values for dbds and an array of ranges for
 *	vfds.  The array of ranges for vfd is an array of areas that has
 *	been set to copy-on-write.  This function loops through the chunk
 *	initializing the vfd/dbd pairs be calling the routine default_vfd
 *	and default_dbd.  Note: default_vfd provides a performance 
 *	enhancement by returning the ending address that the default_vfd is
 *	no longer valid at.
 *
 * END_DESC
 */
void
fillchunk(chunk, rp, idx)
	chunk_t *chunk;	/* Pointer to the chunk of memory to fill */
	reg_t *rp;	/* Pointer to the region this chunk belongs to */
	int idx;	/* Index at which this chunk will be inserted */
{
	register int x;
	register struct vfddbd *vd;
	int brk, end, lim;
	int endidx;
	dbd_t *dbd = default_dbd(rp, idx);
	vfd_t *vfd;

	VASSERT(idx == CHUNKROUND(idx));


	/* Calculate where we switch from first to second proto */
	brk = rp->r_root->b_protoidx;
	end = idx + CHUNKENT;
	lim = (brk < end) ? brk : end;
	vd = &((*chunk)[0]);

	/* 
	 * Loop over first default dbd values
	 */
	vfd = default_vfd(rp, idx, &endidx);
	for (x = idx; x < lim; ++x, ++vd) {
		if (x > endidx)
			vfd = default_vfd(rp, x, &endidx);
		vd->c_vfd = *vfd;
		vd->c_dbd = *dbd;
	}

	/*
	 * Loop over second range of dbds.
	 * Only envoke default_dbd if we must!
	 */
	if (x < end) {
		dbd_t *dbd = default_dbd(rp, x);
		for ( ; x < end; ++x, ++vd) {
			if (x > endidx)
				vfd = default_vfd(rp, x, &endidx);
			vd->c_vfd = *vfd;
			vd->c_dbd = *dbd;
		}
	}
}

/*
 * BEGIN_DESC 
 *
 * alloc_chunk()
 *
 * Return Value:
 *	A pointer to a new chunk. 
 *	Note: the chunk is not zeroed out!
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Allocate a new chunk for the B-tree.
 *
 * Algorithm:
 *	Each broot points to a list of pages.  The last chunk of each
 *	page is wasted.  It is used as a pointer to the next page of
 *	chunks.  This is very expensive.  If no more chunks exist in
 *	the current page, a new page is allocated.  It is linked into
 *	the current list of pages and then a pointer to the next
 *	available chunk is returned.  Note: the variable b_nfrag
 *	is decremented each time a chunk is allocated; so the
 *	chunks are actually allocated from the end of the page 
 *	toward the beginning of the page.
 *	
 *	HP_REVISIT (sol 10/21/90)
 *	The current approach of using the last chunk as a next pointer for
 *	the linked list of pages is VERY expensive.  In future release we
 *	should re-think the approach.
 *
 * END_DESC
 */
 
#ifdef MALLOC_VFDS
int kmalloced_vfds = 0;
#else
int kalloced_vfds = 0;
#endif

chunk_t *
alloc_chunk(rp)
	reg_t *rp;	/* A pointer the region to allocate a new chunk */
{
#ifdef MALLOC_VFDS	
	register struct broot *br = rp->r_root;
	register struct chunk_list *vpp;
	
	vpp = (struct chunk_list *) kmalloc(sizeof(struct chunk_list), M_VFD2, M_WAITOK);
	vpp->cp = (chunk_t *) kmalloc(sizeof(chunk_t), M_VFD2, M_WAITOK);
	bzero(vpp->cp, sizeof(chunk_t));
	kmalloced_vfds++;
	vpp->clp = br->b_list;
	br->b_list = vpp;
	return(vpp->cp);
#else
	register struct broot *br = rp->r_root;
	/* If we need a new page, allocate it & attach it to the free list */
	if (!br->b_nfrag) {
		caddr_t new_page;
		caddr_t va;

		/* Allocate a new page */
		va = kalloc(1, KALLOC_NOZERO);
		kalloced_vfds++;
		new_page = va;
		br->b_npages += 1;

		/* 
		 * Point to last entry in new chunk 
		 * We use the last entry as a linked list for the
		 * pages.  Store current head in this page.
		 */
		va = (caddr_t)((chunk_t *)va + (NFRAG-1));
		*(caddr_t *)va = (caddr_t)br->b_list;

		/* 
		 * Link the new page on to the head of the linked list.
		 */
		br->b_list = (chunk_t *)new_page;

		/* Update b_nfrag since we used the last entry */
		br->b_nfrag = NFRAG-1;
	}

	/* Take a chunk out of the page, update the count */
	br->b_nfrag -= 1;
	return(br->b_list + br->b_nfrag);
#endif	
}

/*
 * BEGIN_DESC 
 *
 * findvfd()
 *
 * Return Value:
 *	A pointer to the vfd requested
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Returns a pointer to the vfd associated at the given
 *	index.  Currently, we require that the index requested
 *	exist. 
 *
 * Algorithm:
 *	Call findentry.
 *
 * In/Out conditions: 
 *	The region MUST be locked on entry.
 *
 * END_DESC
 */
vfd_t *
findvfd(rp, i)
	reg_t *rp;	/* A pointer to the region */
	int i;		/* Index in the region */
{
	vfd_t *vfd;
	(void)findentry(rp, i, &vfd, (dbd_t **)0);
	return(vfd);
}

/*
 * BEGIN_DESC 
 *
 * findbd()
 *
 * Return Value:
 *	A pointer to the dbd requested
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Returns a pointer to the dbd associated at the given
 *	index.  Currently, we require that the index requested
 *	exist. 
 *
 * Algorithm:
 *	Call findentry.
 *
 * In/Out conditions: 
 *	The region MUST be locked on entry.
 *
 * END_DESC
 */
dbd_t *
finddbd(rp, i)
	reg_t *rp;	/* A pointer to the region */
	int i;		/* Desired index */
{
	dbd_t *dbd;
	(void)findentry(rp, i, (vfd_t **)0, &dbd);
	return(dbd);
}

/*
 * BEGIN_DESC 
 *
 * findentry()
 *
 * Return Value:
 *	0 - Failure (currently panics)
 *	1 - Success
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	None.
 *
 * Algorithm:
 *	Each region has a single chunk and key inside it.  The search 
 *	routines (findvfd, finddbd, findentry, and foreach_*) will use the
 *	chunk in the region before using the B-tree.  As long as, the 
 *	active vfd/dbd remain within this one chunk, the B-tree will never
 *	be used.  Once the region overflows this chunk, the B-tree is used.
 *	For simplicity, once the overflow happens, we copy the regions chunk
 *	into the B-tree and then mark the region; so that its chunk will not
 *	be used.
 *
 * In/Out conditions: 
 *	The region MUST be locked on entry.
 *
 * END_DESC
 */
int
findentry(rp, i, vfd, dbd)
	reg_t *rp;	/* Pointer to region */
	int i;		/* Desired index */
	vfd_t **vfd;	/* OUT - Pointer to a pointer to a vfd */
	dbd_t **dbd;	/* OUT - Pointer to a pointer to a vfd */
{							
	register chunk_t *chunk;
	register struct vfddbd *vd;

	VASSERT(rp->r_root);
	VASSERT( i < rp->r_pgsz);
	
	/*
	 * Check that index is less than region size.
	 */
	if (i >= rp->r_pgsz) {
		panic("findentry: idx beyond region size");
		/*NOTREACHED*/
	}

	if (rp->r_key == CINDEX(i)) {
		chunk = rp->r_chunk;
	} else {
		if (rp->r_key == UNUSED_IDX) {
			MALLOC(rp->r_chunk, chunk_t *, 
			       sizeof(chunk_t), M_VFD, M_WAITOK);
			chunk = rp->r_chunk;
			fillchunk(chunk, rp, CHUNKROUND(i));
			rp->r_key = CINDEX(i);
		} else {
			/*
			 * If r_key is DONTUSE_IDX, data could
			 * be in the B-tree.  Search for it.
			 */
			if (rp->r_key == DONTUSE_IDX) {
				/* 
				 * Search existing entry 
				 */
				chunk = (chunk_t *)bt_search(rp->r_root, 
						(unsigned long)CINDEX(i));
			} else {
				/*
				 * First time to use B_tree, copy
				 * the region chunk into the B-tree and
				 * keep going.
				 */
				BT_KILL_CACHE(rp->r_root);
				chunk = alloc_chunk(rp);
				bcopy((chunk_t *)rp->r_chunk, chunk,
				      sizeof(chunk_t));
				bt_insert(rp->r_root, rp->r_key, 
					  (caddr_t)chunk);
				rp->r_key = DONTUSE_IDX;
				FREE(rp->r_chunk, M_VFD);
				rp->r_chunk = (chunk_t *)NULL;
				chunk = (chunk_t *)NULL;
			}
			/*
			 * First reference of new key.  Allocate a chunk
			 * and insert it in the tree.
			 */
			if (!chunk) {
				chunk = alloc_chunk(rp);
				fillchunk(chunk, rp, CHUNKROUND(i));
				bt_insert(rp->r_root, (unsigned long)CINDEX(i), 
					  (caddr_t)chunk);
			}
		}
	}

	/* Return pointers */
	vd = &((*chunk)[COFFSET(i)]);
	if (vfd) 
		*vfd = &(vd->c_vfd);
	if (dbd)
		*dbd = &(vd->c_dbd);
	return(1);
}


/*	XXX   MALLOC_VFDS since we don't swap, next two functions are bogus  */

/*
 * BEGIN_DESC 
 *
 * vfdpgs()
 *
 * Return Value:
 *	Returns an upper bound on the number of pages required
 *	for a B-tree to represent the given number of real 
 *	pages.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Answer the question: How many pages of B-tree stuff does
 *	it take to represent npages worth of data.
 *
 * Algorithm:
 *	Compute the number of chunks needed. 
 *	Add to this the number of inner nodes needed.
 *	Now convert this many chunks into pages.
 *
 * END_DESC
 */
int
vfdpgs(npages)
	int npages; 	/* Number of pages that the B-tree would represent */
{
	int x;

        if (!npages)
		 return(0);

        /* 
	 * First calculate the number of chunks needed to
         * account for the number of pages in the region.
	 * This could be done by:
	 * 	x = npages/ CHUNKENT	
	 * however for speed we will just do a shift.
	 */
        x = (npages + (CHUNKENT-1)) >> CHUNKSHIFT;

	/*
	 * Next, take into account the overhead for the
	 * inner nodes.
	 * This could be done by using the numerical
	 * progression:
	 *				  2              3
	 *	x + (x/B_ORD) + (x/(B_ORD) ) + (x/(B_ORD) ) + .... 
	 * however for speed, we will do a shift by the closest
	 * power of two for B_ORD as an upper bound approximation. 
	 */
	x += (x + B_ORD2) >> B_ORDSHIFT;

	/*
 	 * Convert to pages count
	 */
#ifdef MALLOC_VFDS
        x = howmany(x, NBPG/sizeof(chunk_t));
#else
        x = howmany(x, CHUNKS_PER_PAGE);
#endif        
        return(x);
}

/*
 * BEGIN_DESC 
 *
 * grow_vfdpgs()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Reserve enough swap space to hold the pages needed
 *	to represent the new vfd/dbd + the B-tree overhead.
 *
 * Algorithm:
 *
 * END_DESC
 */
grow_vfdpgs(rp, size)
	reg_t *rp;		/* Pointer to region */
	int size;		/* New size in pages */
{
	int curr_pgs;
	int request_pgs;
	int extra_pgs;
	struct broot *br = rp->r_root;

	request_pgs = vfdpgs(size);

	/* we have already reserved pages */
	if (br->b_rpages != 0) {

		/* make sure we have reserved at least enough */
		curr_pgs = vfdpgs((int)rp->r_pgsz);
		VASSERT(br->b_rpages >= curr_pgs);

		/* only reserve as much as needed */
		extra_pgs = br->b_rpages - curr_pgs;
		if (extra_pgs >= request_pgs)
			request_pgs = 0; 
		else
			request_pgs -= extra_pgs;
	}

	if (!reserve_swap(rp, request_pgs, SWAP_NOWAIT)) 
                return(0);
	else {
		br->b_rpages += request_pgs;
		return(1);
	}
}

/*
 * BEGIN_DESC 
 *
 * do_filldbd()
 *
 * Return Value:
 *	This function always returns 0.  It is called 
 *	by foreach_entry, in a loop.  Zero means keep
 *	going.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Initialize the dbd and vfd values.
 *
 * Algorithm:
 *	Assign the dbd it new value and clean the
 *	vfd.
 *
 * END_DESC
 */
/*ARGSUSED*/
int
do_filldbd(idx, vfd, dbd, proto_dbd)
	int idx;		/* Index of vfd/dbd */
	vfd_t *vfd;		/* pointer to vfd */
	dbd_t *dbd;		/* pointer to dbd */
	dbd_t *proto_dbd;	/* prototype dbd pointer */
{
	VASSERT(vfd->pgm.pg_v == 0);
	*dbd = *proto_dbd;

	/*
	 * This is only called from growvfd, thus the
	 * vfd should be initialized to not have 
	 * copy-on-write set.
	 */
	*vfd = *((vfd_t *)&vfd_nocw);
	return(0);
}

/*
 * BEGIN_DESC 
 *
 * dupvfds()
 *
 * Return Value:
 *	0 - vfds have been duplicated.
 *	1 - not enough swap space to duplicate.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Duplicate the prototype dbd and vfd information
 *	during a fork.
 *
 * Algorithm:
 *	Allocate a root pointer.
 *	Reserve the correct amount of swap space.
 *	Copy the prototype information over.
 *
 * In/Out conditions: 
 *	Destination must be zero in size.
 *	Destination must not have a root pointer already.
 *
 * END_DESC
 */
int
dupvfds(srp, drp, off, count)
	reg_t *srp;		/* Source region */
	reg_t *drp;		/* Destination region */
	size_t off;		/* Page offset in source */
	size_t count;		/* Number of pages to copy */
{
	struct broot *dbr, *sbr;
	dbd_t *sdbd, *ddbd;


	VASSERT(drp->r_pgsz == 0);
	VASSERT(drp->r_root == NULL);


	/*
	 * Allocate a b_root
	 */
	sbr = srp->r_root;
	dbr = drp->r_root = bt_init(drp);

	/* Check if we have enough swap space to cover growth */
	if(!grow_vfdpgs(drp, (int)count)) {
		bt_free(drp);
                return(1);
	}

	/*
	 * Figure out what portion of the new region lies inside of 
	 * what portion of the old regions prototype dbds.
	 */
	if ((off < sbr->b_protoidx) && ((off + count) < sbr->b_protoidx)) {
		/*
		 * Totally within index of first proto.
		 */
		dbr->b_protoidx = UNUSED_IDX;

		/*
		 * will get you the address of the first proto.
		 */
		sdbd = default_dbd(srp, 0);
		ddbd = default_dbd(drp, 0);
		*ddbd = *sdbd;
	} else if (off >= sbr->b_protoidx) {
		/*
		 * Totally within index of second proto.
		 */
		dbr->b_protoidx = UNUSED_IDX;

		/*
		 * will get you the address of the second proto.
		 */
		sdbd = default_dbd(srp, sbr->b_protoidx);
		ddbd = default_dbd(drp, 0);
		*ddbd = *sdbd;
	} else {
		/*
		 * New region crosses boundry at a potentially
		 * different offset.
		 */
		dbr->b_protoidx = sbr->b_protoidx - off;

		/*
		 * Copy both protos.
		 */
		sdbd = default_dbd(srp, 0);
		ddbd = default_dbd(drp, 0);
		*ddbd = *sdbd;
		sdbd = default_dbd(srp, sbr->b_protoidx);
		ddbd = default_dbd(drp, dbr->b_protoidx);
		*ddbd = *sdbd;
	}
	drp->r_pgsz += count;
	return(0);
}
	
/*
 * BEGIN_DESC 
 *
 * growvfd()
 *
 * Return Value:
 *	0 - Region sucessfully grown to the new size.
 *	1 _ Growth of region failed.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Grow the region by the specified amount.  The new
 *	pages will be filled in with a dbd of type.
 *
 * Algorithm:
 *	If the region is currently zero length,
 *		allocate a root pointer.
 *		reserve swap space.
 *		set the default dbd value.
 *	If not,
 *		reserve swap space
 *		set the default dbd value or if this is a new
 *		value set, the second default value.
 *	If this is a third default value, allocate the
 *	actual dbds and fill them in.
 *
 * In/Out conditions: Locks held on entry...
 *		      Side effects (locks released...)
 *                    This routine runs on the ISR
 *
 * END_DESC
 */
int
growvfd(rp, amount, type)
	reg_t *rp;	/* Pointer to region */
	int amount;	/* Amount of pages to grow by */
	int type;	/* DBD value */
{
	int oldidx;
	int k;
	dbd_t *dbd;
	struct broot *br;

	VASSERT(amount >= 0);
	if (amount == 0)
		return(0);


	/* Initial growth--allocate a btree */
	if (rp->r_pgsz == 0) {

		VASSERT(rp->r_root == NULL);
		br = rp->r_root = bt_init(rp);
		if (!grow_vfdpgs(rp, amount)) {
			bt_free(rp);
                	return(1);
		}

		dbd = default_dbd(rp, 0);
		dbd->dbd_type = type;
		dbd->dbd_data = DBD_DINVAL;
		goto out;
	}

	/*
	 * Incremental growth--if the new stuff matches our original
	 * prototype DBD, then bump the region size and make sure that
	 * if a current chunk exists, it contains the appropriate default
	 * values.
	 */
	if (!grow_vfdpgs(rp, amount)) 
		return(1);

	dbd = default_dbd(rp, (int)rp->r_pgsz-1);
	if (dbd->dbd_type == type)
		goto out2;

	/*
	 * If we have a different DBD prototype, see if we can use the
	 * second prototype slot.  If we can, we must make sure that 
	 * any existing chunk contains the new prototype value.
	 */
	br = rp->r_root;
	if (br->b_protoidx == UNUSED_IDX) {
		br->b_protoidx = rp->r_pgsz;
		dbd = &br->b_proto2;
		dbd->dbd_type = type;
		dbd->dbd_data = DBD_DINVAL;
		goto out2;
	}

	/*
	 * Court of last resort: fill in the DBDs now
	 */
	oldidx = rp->r_pgsz;
	for (k = 0; k < amount; ++k) {
		dbd = FINDDBD(rp, oldidx+k);
		dbd->dbd_type = type;
		dbd->dbd_data = DBD_DINVAL;
	}
	goto out;
out2:
	foreach_entry(rp, (int)rp->r_pgsz, amount, do_filldbd, (caddr_t)dbd);
out:
	rp->r_pgsz += amount;
	return(0);
}

/*
 * BEGIN_DESC 
 *
 * free_vfdpages()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Free all of the underlying pages for the B-tree.
 *
 * Algorithm:
 *	Walked the linked list of pages.  Note the last
 *	chunk in each page is a next pointer and free
 *	the page by calling kdfree().
 *
 * END_DESC
 */
free_vfdpages(br)
	struct broot *br;	/* Pointer to root of tree */
{
#ifdef MALLOC_VFDS
	register struct chunk_list *vpp = br->b_list;
	register struct chunk_list *next;
	
	while (vpp) {
		next = vpp->clp;
		kfree(vpp->cp, M_VFD2);
		kfree(vpp, M_VFD2);		
		vpp = next;
		kmalloced_vfds--;
	}
#else
	caddr_t cp = (caddr_t)br->b_list;
	int count = 0;
	while (cp) {
		caddr_t next;
		caddr_t vaddr;
		count++;
		/* 
		 * The pointer to the next page of vfd's 
		 * is stored in the last entry of the current page
		 * to be freed.  We must save this value now, because
		 * we free the page before the end of this loop.
		 */
		next = *(caddr_t *)((chunk_t *)cp + (NFRAG-1));

		/*
		 * Reset vaddr to the beginning of the page.
		 */
		vaddr = (caddr_t)((long)cp & ~(NBPG-1));
		kdfree(vaddr, 1);
		kalloced_vfds--;
		cp = next;
	}
	VASSERT(count == br->b_npages);
#endif	
}

/*
 * BEGIN_DESC 
 *
 * shrinkvfd()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Shrink the region by <amount> pages.
 *
 * Algorithm:
 *	The region is shrunk by npages.  The most typical case is to shrink
 *	the entire region to 0.  The only case for partial shrinking is
 *	a negative sbreak.
 *
 * In/Out conditions: 
 *	The region must be locked.
 *
 * END_DESC
 */
void
shrinkvfd(rp, amount)
	reg_t *rp;	/* A pointer to the region */
	int amount;	/* Amount of pages to shrink region by */
{
	int newendidx;
	register struct broot *br;

	VASSERT(rp);
	VASSERT(rp->r_root);
	VASSERT(amount > 0);

	newendidx = rp->r_pgsz - amount - 1;
	br = rp->r_root;


	/*
	 * If we're deleting everything under the region,
	 * then don't bother readjusting vfd and dbd info
	 */
	if (newendidx == -1) {	/* i.e. amount == rp->r_pgsz */
		/* 
		 * Release the swap space used by vfd's no longer needed.
		 * Swap space is reserved in grow_vfdpgs().
		 */
		release_swap(rp->r_root->b_rpages);
		bt_free(rp);
	} else {
		register struct vfdcw *v = &(br->b_vproto);
		int i;

		/*
		 * Blow away the cache.
		 */
		BT_KILL_CACHE(br);

		/*
		 * Since the caller is shrinking, we must adjust the 
		 * copy-on-write vfd information.
		 */
		for (i = 0; i < MAXVPROTO; i++) {
			if (v->v_start[i] == -1)
				continue;
			if (v->v_end[i] <= newendidx)
				continue;
			/*
			 * Is the whole entry bogus?
			 */
			if (v->v_start[i] > newendidx)
				v->v_start[i] = -1;
			else	
				v->v_end[i] = newendidx;
		}

		/*
		 * Since the caller is shrinking, we must adjust the 
		 * prototype dbd information.
		 */
		if ((br->b_protoidx != UNUSED_IDX) &&
		    (br->b_protoidx > newendidx))
			br->b_protoidx = UNUSED_IDX;
		/*
		 * adjust count of remaining pages under the region
		 */
		rp->r_pgsz -= amount;
	}
}

#ifdef OSDEBUG
/*
 * BEGIN_DESC 
 *
 * vfdcheck()
 *
 * Return Value:
 *	This function is called by foreach_entry.  It always
 *	return 0 to indicate that it was to be called until
 *	the end of the region.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Check to make sure that the value of the dbd is acceptable.
 *	For swap we check to make sure that the swap information
 *	is in a specified set of ranges.
 *
 * Algorithm:
 *	Check that the dbd values are of known type.
 *
 * END_DESC
 */
/*ARGSUSED*/
int
vfdcheck(rindex, vfd, dbd, rp)
        int rindex;
        vfd_t *vfd;
        swpdbd_t *dbd;
        reg_t *rp;
{

	switch (dbd->dbd_type) {
        case DBD_BSTORE: 
		if (rp->r_bstore == swapdev_vp) {
                	VASSERT(dbd->dbd_swptb <= (uint) maxswapchunks);
                	VASSERT(dbd->dbd_swpmp <= (uint) NPGCHUNK);	
		}
        case DBD_FSTORE: 
        case DBD_NONE  :  
        case DBD_DZERO : 
        case DBD_DFILL : break;
	default        : panic("vfdcheck: invalid dbd type");
		
	}
	return(0);
}
#endif

#ifndef MALLOC_VFDS
/*
 * BEGIN_DESC 
 *
 * vfdpanic()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Check to make sure that the value of the first node
 *	in the B-tree is a reasonable value.
 *
 * Algorithm:
 *	It turned out that when ever we saw the B-tree get
 *	corrupted that the first down pointer contained a 
 *	bogus value.  This is not a routine that makes an
 *	exhaustive check of the B-tree.  Rather it makes
 *	a simple check that most often hit the correuption
 *	we were looking at.
 *
 * END_DESC
 */
void
vfdpanic(rp, s) 
	reg_t *rp;	/* Region pointer */
	char *s;	/* Character String */
{
	if (rp->r_root->b_npages == 1) {
		caddr_t treelink = (caddr_t)rp->r_root->b_root->b_down[1];
		caddr_t page = (caddr_t)(rp->r_root->b_list);
		if ((treelink < page) || (treelink > (page+NBPG))){
			printf(s);
			panic(" : b_down[1] is garbage");
		}
	}
}

#endif

/*
 * BEGIN_DESC 
 *
 * debugpageio()
 *
 * Return Value:
 *	This function returns the error value stored in the
 *	buffer header that was used for the I/O.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Launch the desired I/O request.
 *
 * Algorithm:
 *	Allocate a buffer header.
 *	Call asyncpageio to perform the desired I/O
 *	Wait for the I/O to complete
 *	place a pointer to the buffer header in bpp
 *	return the error value for the I/O
 *
 * In/Out conditions:
 *	The caller MUST free the buffer header!
 *
 * END_DESC
 */
int
debugpageio(dblkno, sp, addr, nbytes, rdflg, vp, bpp)
	swblk_t dblkno; 	/* Block number for I/O */
	space_t	sp;		/* Space value */
	caddr_t addr;		/* 32 bit virtual address */
	int nbytes;		/* length in bytes */
	int rdflg;		/* Read or Write */
	struct vnode *vp;	/* Vnode for I/O */
	struct buf **bpp;	/* OUT - buffer pointer */
{
	struct buf *bp;
	int error;

	bp = bswalloc();
	bp->b_flags |= B_BUSY;
	asyncpageio(bp, dblkno, sp, addr, nbytes, rdflg, vp);
	error = waitforpageio(bp);
	*bpp = bp;
	return(error);
}

/*
 * BEGIN_DESC 
 *
 * vfdswapo()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Swapout the vfd/dbd and B-tree information associated
 *	with the given region.
 *
 * Algorithm:
 *	Allocate contigous swap blocks for the vfd/dbd and B-tree.  Note
 *	the space was already resrved at growvfd time.  Walk
 *	through the B-tree page list and push each page
 *	synchronously to disk.  Note: while the pages are freed, their 
 *	virtual addresses are not.  It is because of this feature that the
 *	kernel must maintain a larger pool of virtual addresses than it 
 *	has physical addresses.
 *
 *	r_dbd is set to the value of the first swap block allocated.
 *
 * In/Out conditions: 
 *	The regon is locked.
 *	The region has not alreay been swapped.
 *	The region is not memory locked.
 *
 * END_DESC
 */
void
vfdswapo(rp)
	reg_t *rp;	/* A pointer to the region */
{
        register struct broot *br = rp->r_root;
        int npages;
        dbd_t dbd;
	caddr_t vaddr;
	caddr_t next_page;
	int error;
	int count = 0;

#ifdef MALLOC_VFDS
	return;		/*  don't bother swapping for now XXX  */
#else

        VASSERT(vm_valusema(&rp->r_lock) <= 0);
        VASSERT(rp->r_dbd == 0);
        VASSERT(rp->r_mlockcnt == 0);
	VASSERT(!(rp->r_flags&RF_NOWSWP));
#ifdef OSDEBUG
	foreach_entry(rp, 0, (int)rp->r_pgsz, vfdcheck, (caddr_t)rp);
#endif
	if ((npages = br->b_npages) == 0)
		return;
	/*
         * Calculate swap storage needed, don't swap out if we can't get it
         */
        if (!swcontig(&dbd, npages))
                return;

        rp->r_dbd = *(int *)&dbd;

	vaddr = (caddr_t)rp->r_root->b_list;
	while (vaddr) {
		count++;
		VASSERT(count <= br->b_npages);
		/* 
		 * The pointer to the next page of vfd's 
		 * is stored in the last entry of the current page
		 * to be swapped.  We must save this value now, because
		 * we free the page before the end of this loop.
		 */
		next_page = *(caddr_t *)((chunk_t *)vaddr + (NFRAG-1));

		vaddr = (caddr_t)((long)vaddr & ~(NBPG-1));
		if (vfddebug == 0) {
#ifdef FSD_KI
			{
			struct buf *bp;
			bp = bswalloc();
			bp->b_bptype = 	B_vfdswapo|B_swpbf;
			bp->b_rp = rp;
			asyncpageio(bp, (swblk_t)dbd.dbd_data, KERNELSPACE,
					vaddr, NBPG, B_WRITE, swapdev_vp);
			error = waitforpageio(bp);
			bswfree(bp);
			}
#else  /* ! FSD_KI */
			error = syncpageio((swblk_t)dbd.dbd_data, KERNELSPACE,
					vaddr, NBPG, B_WRITE, swapdev_vp);
#endif /* ! FSD_KI */
			if (error)
				panic("vfdswapo: syncpageio detected  error");
		} else {
#ifndef MALLOC_VFDS
			vfdpanic(rp, "vfdswapo start");
#endif			
			error = debugpageio((swblk_t)dbd.dbd_data, KERNELSPACE,
				vaddr, NBPG, B_WRITE, swapdev_vp, &globalbp1);
			if (error)
				panic("vfdswapo: syncpageio detected error");

			error = debugpageio((swblk_t)dbd.dbd_data, KERNELSPACE,
				vaddr, NBPG, B_READ, swapdev_vp, &globalbp2);
			if (error)
				panic("vfdswapo: read after write");
#ifndef MALLOC_VFDS
			vfdpanic(rp, "vfdswapo end");
#endif
			bswfree(globalbp1);
			bswfree(globalbp2);
		}

		/* Free its memory */
		kdrele(vaddr, 1);		/*  XXX do NBPVC, not pg  */

		/* Advance to next block in contiguous swap area */
		dbd.dbd_data++;

		/* Swap out next page */
		vaddr = (caddr_t)next_page;
	}
	rp->r_flags |= RF_NOWSWP|RF_EVERSWP;
#endif	
}

/*
 * BEGIN_DESC 
 *
 * vfdswapi()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function restores the swapped out B-tree structure associated
 *	with the region.
 *
 * Algorithm:
 *	Loop through (beginning at r_dbd) and read each block off the
 *	swap system synchronously.  After each block is read the virtual
 *	address of the next page is found in the last chunk of the page
 *	just read.  Allocate a page at that address and then read that
 *	page off disk.
 *
 * In/Out conditions:
 *	The region must be locked.
 *	The region must be swapped.
 *
 * END_DESC
 */
void
vfdswapi(rp)
	reg_t *rp;	/* A pointer to the region */
{
	register struct broot *br = rp->r_root;
        dbd_t dbd;
	int error;
	int count = 0;
	caddr_t vaddr;

#ifdef MALLOC_VFDS
	return;			/*  don't bother for now XXX  */
#else

        VASSERT(vm_valusema(&rp->r_lock) <= 0);
        VASSERT(rp->r_dbd != 0);
	VASSERT(rp->r_flags&RF_EVERSWP);
	VASSERT(rp->r_flags&RF_NOWSWP);

        dbd = *(dbd_t *)&rp->r_dbd;
	vaddr = (caddr_t)rp->r_root->b_list;

	while (vaddr) {
		
		count++;
		VASSERT(count <= br->b_npages);
		vaddr = (caddr_t)((long)vaddr & ~(NBPG-1));
		/* 
		 * Allocate memory and attach it to the given 
		 * virtual address.
		 */
		kdget(vaddr, 1);

		/* Read the page in */
#ifdef FSD_KI
		{
		struct buf *bp;
		bp = bswalloc();
		bp->b_bptype = B_vfdswapi|B_swpbf;
		bp->b_rp = rp;
		asyncpageio(bp, (swblk_t)dbd.dbd_data, KERNELSPACE, vaddr, 
					NBPG, B_READ, swapdev_vp);
		error = waitforpageio(bp);
		bswfree(bp);
		}
#else  /* ! FSD_KI */
		error = syncpageio((swblk_t)dbd.dbd_data, KERNELSPACE, vaddr, 
					NBPG, B_READ, swapdev_vp);
#endif /* ! FSD_KI */
		/*
		 * If we can not bring back in the VFD's we can not kill
		 * the process, thus we must panic.
		 */
		if (error)
			panic("vfdswapi: syncpageio detected an error");

		/* Free its swap resource */
		(void)swfree1(&dbd);
	
		/* Advance to next block in contiguous swap area */
		dbd.dbd_data++;
	
		/* 
		 * The pointer to the next page of vfd's 
		 * is stored in the last entry of the page just read.
		 */
		vaddr = *(caddr_t *)((chunk_t *)vaddr + (NFRAG-1));
	}
#ifdef OSDEBUG
         foreach_entry(rp, 0, (int)rp->r_pgsz, vfdcheck, (caddr_t)rp);
#endif
        rp->r_dbd = 0;
	rp->r_flags &= ~RF_NOWSWP;
#endif	
}

/*
 * BEGIN_DESC 
 *
 * countvfd()
 *
 * Return Value:
 *	Returns the number of pages currently be used to
 *	hold the vfd/dbd for a region.  This is the amount
 *	of space the system would get back by swapping out
 *	this region.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Returns b_npages.
 *
 * Algorithm:
 *	return b_npages.
 *
 * In/Out conditions: 
 *	The region must have a root pointer.
 *
 * END_DESC
 */
int
countvfd(rp)
	reg_t *rp;	 /* A pointer to the region */
{
	VASSERT(rp);
	VASSERT(rp->r_root);

	return(rp->r_root->b_npages);
}

/*
 * BEGIN_DESC 
 *
 * vfd_setcw()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Store a range of vfds that have copy-on-write 
 *	set on them.
 *
 * Algorithm:
 *	We support an array of ranges.  If the user exceeds
 *	MAXVPROTO worth of ranges we will go ahead and allocate
 *	the vfd and fill them in with the copy-on-write 
 *	information.  If the range is less than MAXVPROTO,
 *	we see if the new range lies with in (or contains)
 *	an existing range.  If so, the old range is replaced
 *	with the new range.  If no overlaps existed, a blank
 *	entry is found and initialized.
 *
 * END_DESC
 */
vfd_setcw(rp, idx, cnt)
	reg_t *rp;	/* Pointer to region */
	int idx;	/* Index of cow range */
	int cnt;	/* Lenght of range in pages */
{
	extern unsigned int min();
	extern unsigned int max();
	register int x = -1;
	register struct vfdcw *v = &(rp->r_root->b_vproto);
	int newendidx = idx+cnt-1;	
	int slot = -1;

	/*
	 * search for an identical range; otherwise use
	 * new range.
	 */
	for (x=0; x < MAXVPROTO; ++x) {
		if (v->v_start[x] == -1) {
			if (slot == -1)
				slot =x;
			continue;
		}
		if (OVERLAPS(v->v_start[x], v->v_end[x], idx, newendidx)) {
			v->v_start[x] = min((u_int)v->v_start[x], (u_int)idx);
			v->v_end[x] = max((u_int)v->v_end[x], (u_int)newendidx);
			return;
		}
	}

	/*
	 * If we found an empty slot use it.
	 */
	if (slot != -1) {
		v->v_start[slot] = idx;
		v->v_end[slot] = newendidx;
		return;
	}

	/*
	 * We have run out of slots to hold sparse 
	 * values for cw.
	 */
#ifdef OSDEBUG
	printf("vfd_setcw: no sparseness\n");
#endif
	for (x = idx; x < idx+cnt; ++x) {
		vfd_t *vfd = FINDVFD(rp, x);
		vfd->pgm.pg_cw = 1;
	}
}


/*
 * BEGIN_DESC 
 *
 * for_val2()
 *
 * Return Value:
 *	0 - Keep going
 *	1 - Stop
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	This function is recursive; although it does not often
 *	recurse.  The function walks the B-tree to find the desired
 *	starting point and then envokes for_val3 to walk the chunk
 *	and call the desired routine.
 *
 * Algorithm:
 *	If the B-tree has not been allocated the function looks
 *	to see if the chunk in the region is being used.  If it
 *	is and the chunk in the region lies within the specified
 *	bounds, for_val3 is called with the in-region chunk.
 *
 *	If the B-tree has been allocated, this routine walks
 *	the B-tree and envokes for_val3 for the specified 
 *	chunk.  Walking the B-tree might cause this routine
 *	to recurse down the tree.  It is rare that a B-tree
 *	will need to recurse.  This happens when the access
 *	patern of the user is very sparse and extremely random.
 *
 * END_DESC
 */
int
for_val2(br, bt, start, cnt, fn, arg, depth)
	struct broot *br;	/* Pointer to root of tree */
	struct bnode *bt;	/* Pointer to current node */
	int start;		/* Starting index in region */
	int cnt;		/* Number to examine */
	int (*fn)();		/* Pointer to function */
	caddr_t arg;		/* Argument to function */
	int depth;		/* Depth of tree */
{
	register int x;
	register int key_val;
	register int start_idx = CINDEX(start), end_idx = CINDEX(start+cnt-1);
	int interior = (depth < br->b_depth);

	/*
	 * If this is a NULL tree see if the chunk in the region
	 * should be used.  Note: Only one chunk exists in the region
	 * so we either use it or not, but no looping needs to happen.
	 */
	if (!bt) {
		reg_t *rp = br->b_rp;
		if (rp->r_key != UNUSED_IDX) {
			key_val = rp->r_key;
			if ((key_val >= start_idx) && (key_val <= end_idx)) {
				int	c_cnt, c_idx, r_idx, remainder;
				struct vfddbd *vfdp;

				r_idx = REGIDX(key_val);
				c_idx = 0;
				c_cnt = CHUNKENT;
				/*
				 *  If we start in this chunk...
				 */
				if (start > r_idx) {
					r_idx = start;
					c_idx = COFFSET(r_idx);
					/*
					 * Count is at most to the end of 
					 * this chunk
					 */
					c_cnt -= c_idx;
				} 
				/*
				 * If more in this chunk than remains,
				 * then only set count for remainder
				 */
				if (c_cnt > (remainder = start + cnt - r_idx))
					c_cnt = remainder;

				vfdp = &(((struct vfddbd *)
					  	rp->r_chunk)[c_idx]);
				(void)(*fn)(br->b_rp, r_idx, vfdp,
					    c_cnt, arg);
			}
		}
		return(0);
	}
	VASSERT(depth <= B_DEPTH);
	VASSERT(br->b_rp->r_key == DONTUSE_IDX);

	/* Look at each slot */
	for (x = 0; x < bt->b_nelem; ++x) {

		key_val = bt->b_key[x];

		/*
		 * For interior nodes we may need to drop into the 
		 *  "before key 0" slot.
		 */
		if (!x && interior && (start_idx < key_val)) {
			if (for_val2(br, bt->b_down[0], start,
					cnt, fn, arg, depth+1))
				return(1);
		}

		/* Process the appropriate node */
		if (interior) {
			if (for_val2(br, bt->b_down[x+1], start,
			    cnt, fn, arg, depth+1))
				return(1);
		} else {
			if ((key_val >= start_idx) && (key_val <= end_idx)) {
				int	c_cnt, c_idx, r_idx, remainder;
				struct vfddbd *vfdp;

				r_idx = REGIDX(key_val);
				c_idx = 0;
				c_cnt = CHUNKENT;
				/*
				 *  If we start in this chunk...
				 */
				if (start > r_idx) {
					r_idx = start;
					c_idx = COFFSET(r_idx);
					/*
					 * Count is at most to the end of 
					 * this chunk
					 */
					c_cnt -= c_idx;
				} 
				/*
				 * If more in this chunk than remains,
				 * then only set count for remainder
				 */
				if (c_cnt > (remainder = start + cnt - r_idx))
					c_cnt = remainder;

				vfdp = &(((struct vfddbd *)
					  	bt->b_down[x+1])[c_idx]);
				if ((*fn)(br->b_rp, r_idx, vfdp,
					  c_cnt, arg))
					return(1);
			}
		}

		/*
		 * If we've passed beyond the end of possible nodes for the
		 *  specified range, we're done.
		 */
		if (key_val > end_idx)
			return(1);
	}
	return(0);
}

/*
 * BEGIN_DESC 
 *
 * foreach_chunk()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Walk the region and envoke the specified routine
 *	for each chunk allocated.  This is the best means
 *	to examine all of the vfd/dbd pairs of a region.
 *	It is MUCH faster than using foreach_vald or
 *	foreach_entry.
 *
 * Algorithm:
 *	Call for_val2.
 *	Note: the function fn cause the walker to 
 *	continue or stop based on its return value
 *	(0 Keep going, 1 Stop).
 *
 * In/Out conditions:
 *	The region must be locked.
 *
 * END_DESC
 */
void
foreach_chunk(rp, start, cnt, fn, arg)
	reg_t *rp;	/* A pointer to the region */
	int start;	/* Starting index */
	int cnt;	/* Count to look at */
	int (*fn)();	/* Function to call */
	caddr_t arg;	/* Argument to pass to function */
{
	register struct broot *br = rp->r_root;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	if (!br)
		return;
	(void)for_val2(br, br->b_root, start, cnt, fn, arg, 1);
}

/*
 * BEGIN_DESC 
 *
 * for_val3()
 *
 * Return Value:
 *	Returns the value returned by (*fn)
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Envoke the given function for either all valid vfds
 *	in the chunk or all entries in the chunk.
 *
 * Algorithm:
 *	Walk the chunk and envoke the specified function.
 *
 * END_DESC
 */

/*ARGSUSED*/
for_val3(rp, idx, vd, cnt, arg)
        struct region *rp;      /* region pointer */
	int idx;		/* region index of start entry */
	struct vfddbd *vd;	/* pointer to 1st vfddbd pair of interest */
	int cnt;		/* Number of entries in range in this chunk */
	struct fv3_arg *arg;	/* Argument passed in to us */
{
	enum scan flags = arg->flags;	/*  scan all or only valid */

	VASSERT((flags == SCAN_ALL) || (flags == SCAN_VALID));
		
	for (; cnt--; idx++, vd++) {
		if ((flags == SCAN_ALL) || (vd->c_vfd.pgm.pg_v)) {
			if ((*arg->fn)(idx, &vd->c_vfd, &vd->c_dbd, 
			       arg->arg)) {
				return(1);
			}
		}
	}
	return(0);
}


/*
 * BEGIN_DESC 
 *
 * foreach_valid()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Walk the region and envoke the specified routine
 *	for each valid vfd.  Note: This is an expensive 
 *	method to walk the vfd list.  You should use 
 *	the chunk walker foreach_chunk.
 *
 * Algorithm:
 *	Call for_val2.
 *	Note: the function fn cause the walker to 
 *	continue or stop based on its return value
 *	(0 Keep going, 1 Stop).
 *
 * In/Out conditions:
 *	The region must be locked.
 *
 * END_DESC
 */
void
foreach_valid(rp, start, cnt, fn, arg)
	reg_t *rp;	/* Pointer to region */
	int start;	/* Starting index in region */
	int cnt;	/* Count to look at */
	int (*fn)();	/* Function to call */
	caddr_t arg;	/* Argument to pass */
{
	register struct broot *br = rp->r_root;
	struct	fv3_arg	fv3_arg;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	/* Save lots of work for this simple case */
        if (!br || (rp->r_nvalid == 0))
                return;

	fv3_arg.fn = fn;
	fv3_arg.arg = arg;
	fv3_arg.flags = SCAN_VALID;
	(void)for_val2(br, br->b_root, start, cnt, for_val3, &fv3_arg, 1);
}

/*
 * BEGIN_DESC 
 *
 * foreach_valid()
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	None.
 *
 * Description:
 *	Walk the region and envoke the specified routine
 *	for each allocated entry.  Note: This is an expensive 
 *	method to walk the vfd list.  You should use 
 *	the chunk walker foreach_chunk.
 *
 * Algorithm:
 *	Call for_val2.
 *	Note: the function fn cause the walker to 
 *	continue or stop based on its return value
 *	(0 Keep going, 1 Stop).
 *
 * In/Out conditions:
 *	The region must be locked.
 *
 * END_DESC
 */
void
foreach_entry(rp, start, cnt, fn, arg)
	reg_t *rp;	/* A pointer to the region */
	int start;	/* Starting index */
	int cnt;	/* Count to look at */
	int (*fn)();	/* Function to call */
	caddr_t arg;	/* Argument to pass to function */
{
	register struct broot *br = rp->r_root;
	struct	fv3_arg	fv3_arg;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
        if (!br)
                return;

	fv3_arg.fn = fn;
	fv3_arg.arg = arg;
	fv3_arg.flags = SCAN_ALL;
	(void)for_val2(br, br->b_root, start, cnt, for_val3, &fv3_arg, 1);
}
