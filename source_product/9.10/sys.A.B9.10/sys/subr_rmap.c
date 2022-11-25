/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/subr_rmap.c,v $
 * $Revision: 1.14.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:44:57 $
 */


/*	subr_rmap.c	6.1	83/07/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/sema.h"
#include "../h/debug.h"

/*
 * Allocate the lock for ALL resource maps.  If we see high contention, we
 * may want to move the rmap lock into each rmap structure.  But since I
 * suspect sysmap is most of the traffic, it might not help.  Andy
 */
vm_lock_t rmap_lock;	/* Spinlock for resource map operations */

/*
 * Resource map handling routines.
 *
 * A resource map is an array of structures each
 * of which describes a segment of the address space of an available
 * resource.  The segments are described by their base address and
 * length, and sorted in address order.  Each resource map has a fixed
 * maximum number of segments allowed.  Resources are allocated
 * by taking part or all of one of the segments of the map.
 *
 * Returning of resources will require another segment if
 * the returned resources are not adjacent in the address
 * space to an existing segment.  If the return of a segment
 * would require a slot which is not available, then one of
 * the resource map segments is discarded after a warning is printed.
 * Returning of resources may also cause the map to collapse
 * by coalescing two existing segments and the returned space
 * into a single segment.  In this case the resource map is
 * made smaller by copying together to fill the resultant gap.
 *
 * N.B.: the current implementation uses a dense array and does
 * not admit the value ``0'' as a legal address, since that is used
 * as a delimiter.
 */

/*
 * Initialize map mp to have (mapsize-2) segments
 * and to be called ``name'', which we print if
 * the slots become so fragmented that we lose space.
 * The map itself is initialized with size elements free
 * starting at addr.
 */
rminit(mp, size, addr, name, mapsize)
	register struct map *mp;
	unsigned long size, addr;
	char *name;
	int mapsize;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	static int lock_set = 0;

	/* Set up resource map spinlock once */
	if (!lock_set) {
		vm_initlock(rmap_lock, RMAP_LOCK_ORDER, "resource map lock");
		lock_set = 1;
	}

	mp->m_name = name;
	/*
	 * One of the mapsize slots is taken by the map structure,
	 * and mapsize - 1 slots are used for mapent structures.
	 * We never use segments past mp->m_limit; instead
	 * when excess segments occur we discard some resources.
	 * And we insure that mp->m_limit is within the space allocated
	 * by the caller.
	 */
	mp->m_limit = &ep[mapsize - 1];
	if (mp->m_limit > &mp[mapsize])
		panic("rminit: sizeof(struct map) < sizeof(struct mapent)");

	/*
	 * Size and addr must both be zero or they must both be non-zero.
	 */
	if ((size == 0) != (addr == 0))
		panic("rminit: size and addr inconsistent");

	/*
	 * Simulate an rmfree if size != 0 and addr != 0.
	 */
	ep->m_size = size;
	ep->m_addr = addr;
#ifdef OSDEBUG
	mp->m_ents = size ? 1 : 0;
#endif

	/*
	 * Add the delimiter (unnecessary, but not harmful if size = 0). 
	 */
	ep++;
	ep->m_size = ep->m_addr = 0;
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 */
long
rmalloc(mp, size)
	register struct map *mp;
	unsigned long size;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register int addr;
	register struct mapent *bp;
	register u_int context;


        VASSERT (size != 0);

	SPINLOCK_USAV(rmap_lock, context);

	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	for (bp = ep; bp->m_size; bp++) {
		if (bp->m_size >= size) {
			/*
			 * Allocate from the map.
			 * If there is no space left of the piece
			 * we allocated from, move the rest of
			 * the pieces to the left.
			 */
			addr = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while ((bp-1)->m_size = bp->m_size);
#ifdef OSDEBUG
                                mp->m_ents--;
#endif
			}
			SPINUNLOCK_USAV(rmap_lock, context);
			return (addr);
		}
	}
	SPINUNLOCK_USAV(rmap_lock, context);
	return (0);
}

/*
 * Free the previously allocated space at addr
 * of size units into the specified map.
 * Sort addr into map and combine on
 * one or both ends if possible.
 *
 * rmfree returns the size of the segment created by the free.
 */
u_int
rmfree(mp, size, addr)
	struct map *mp;
	unsigned long size, addr;
{
	struct mapent *firstbp;
	register struct mapent *bp;
	register int t;
	register u_int context;
        register u_int segment_length = size;

        VASSERT (size != 0);
	/*
	 * Locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
	firstbp = bp = (struct mapent *)(mp + 1);
	SPINLOCK_USAV(rmap_lock, context);
	for (; bp->m_addr <= addr && bp->m_size != 0; bp++)
		continue;
	/*
	 * If the piece on the left abuts us,
	 * then we should combine with it.
	 */
	if (bp > firstbp && (bp-1)->m_addr+(bp-1)->m_size >= addr) {
		/*
		 * Check no overlap (internal error).
		 */
		if ((bp-1)->m_addr+(bp-1)->m_size > addr) {
			SPINUNLOCK_USAV(rmap_lock, context);
			panic("rmfree: overlap");
		}
		/*
		 * Add into piece on the left by increasing its size.
		 */
		(bp-1)->m_size += size;
		/*
		 * If the combined piece abuts the piece on
		 * the right now, compress it in also,
		 * by shifting the remaining pieces of the map over.
		 */
		if (bp->m_addr && addr+size >= bp->m_addr) {
			if (addr+size > bp->m_addr) {
				SPINUNLOCK_USAV(rmap_lock, context);
				panic("rmfree: abut to right");
			}
			(bp-1)->m_size += bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
#ifdef OSDEBUG
                        mp->m_ents--;
#endif
		}
                segment_length = (bp-1)->m_size;
		goto done;
	}
	/*
	 * Don't abut on the left, check for abutting on
	 * the right.
	 */
	if (addr+size >= bp->m_addr && bp->m_size) {
		if (addr+size > bp->m_addr) {
			SPINUNLOCK_USAV(rmap_lock, context);
			panic("rmfree: abut to left");
		}
		bp->m_addr -= size;
		bp->m_size += size;
		segment_length = bp->m_size;
		goto done;
	}
	/*
	 * Don't abut at all.  Make a new entry
	 * and check for map overflow.
	 */
	do {
		t = bp->m_addr;
		bp->m_addr = addr;
		addr = t;
		t = bp->m_size;
		bp->m_size = size;
		bp++;
	} while (size = t);
#ifdef OSDEBUG
        mp->m_ents++;
#endif

	/*
	 * Segment at bp is to be the delimiter;
	 * If there is not room for it 
	 * then the table is too full
	 * and we must discard something.
	 */
	if (bp+1 > mp->m_limit) {
		/*
		 * Back bp up to last available segment.
		 * which contains a segment already and must
		 * be made into the delimiter.
		 * Discard second to last entry,
		 * since it is presumably smaller than the last
		 * and move the last entry back one.
		 */
		bp--;
		printf("%s: rmap ovflo, lost [%d,%d)\n", mp->m_name,
		    (bp-1)->m_addr, (bp-1)->m_addr+(bp-1)->m_size);
		bp[-1] = bp[0];
	}
	bp->m_size = bp->m_addr = 0;	/* add the delimiter */
done:
	SPINUNLOCK_USAV(rmap_lock, context);
	return(segment_length);
}

/*
 * Allocate 'size' units from the given map, starting at address 'addr'.
 * Return 'addr' if successful, 0 if not.
 * This may cause the creation or destruction of a resource map segment.
 *
 * This routine will return failure status if there is not enough room
 * for a required additional map segment.
 */
rmget(mp, size, addr)
	struct map *mp;
{
	/*
	 * Perform an exact allocation
	 */
	return do_rmget(mp, size, addr, 1);
}

#ifdef NEVER_CALLED
/*
 * Allocate 'size' units from the given map, at or above address 'addr'.
 * Return something >= 'addr' if successful, 0 if not.
 * This may cause the creation or destruction of a resource map segment.
 *
 * This routine will return failure status if there is not enough room
 * for a required additional map segment.
 */
rmget_above(mp, size, addr)
	struct map *mp;
{
    /*
     * Perform a non-exact allocation.  This will allocate from an
     * address >= addr.
     */
    return do_rmget(mp, size, addr, 0);
}

#endif /* NEVER_CALLED */

do_rmget(mp, size, addr, exact)
	register struct map *mp;
{
	register struct mapent *ep = (struct mapent *)(mp+1);
	register struct mapent *bp, *bp2;
	register u_int context;

	/*
	 * Look for a map segment containing the requested address.
	 * If none found, return failure.
	 */
	SPINLOCK_USAV(rmap_lock, context);
	for (bp = ep; bp->m_size; bp++) {
		if (bp->m_addr <= addr && bp->m_addr + bp->m_size > addr)
			break;

		/*
		 * Stop searching when bp->m_addr is greater than the
		 * requested address.
		 */
		if (bp->m_addr > addr) {
			/*
			 * If an exact allocation was requested, we
			 * cannot satisfy the request, so return.
			 */
			if (exact) {
				SPINUNLOCK_USAV(rmap_lock, context);
				return (0);
			}

			/*
			 * An exact allocation is not required, continue
			 * searching for a free slot that is large
			 * enough.
			 */
			for (; bp->m_size; bp++) {
				if (bp->m_size >= size) {
					addr = bp->m_addr;
					break;
				}
			}
			break;
		}
	}

	if (bp->m_size == 0) {
		SPINUNLOCK_USAV(rmap_lock, context);
		return (0);
	}

	/*
	 * If segment is too small, return failure.
	 * If big enough, allocate the block, compressing or expanding
	 * the map as necessary.
	 */
	if (bp->m_addr + bp->m_size < addr + size) {
		SPINUNLOCK_USAV(rmap_lock, context);
		return (0);
	}

	if (bp->m_addr == addr)
		if (bp->m_addr + bp->m_size == addr + size) {
			/*
			 * Allocate entire segment and compress map
			 */
			bp2 = bp;
			while (bp2->m_size) {
				bp2++;
				(bp2-1)->m_addr = bp2->m_addr;
				(bp2-1)->m_size = bp2->m_size;
			}
#ifdef OSDEBUG
                        mp->m_ents--;
#endif
		} else {
			/*
			 * Allocate first part of segment
			 */
			bp->m_addr += size;
			bp->m_size -= size;
		}
	else
		if (bp->m_addr + bp->m_size == addr + size) {
			/*
			 * Allocate last part of segment
			 */
			bp->m_size -= size;
		} else {
			/*
			 * Allocate from middle of segment, but only
			 * if table can be expanded.
			 */
			for (bp2=bp; bp2->m_size; bp2++)
				;
			if (bp2+1 >= mp->m_limit) {
				SPINUNLOCK_USAV(rmap_lock, context);
				return (0);
			}
			while (bp2 > bp) {
				(bp2+1)->m_addr = bp2->m_addr;
				(bp2+1)->m_size = bp2->m_size;
				bp2--;
			}
			(bp+1)->m_addr = addr + size;
			(bp+1)->m_size =
			    bp->m_addr + bp->m_size - (addr + size);
			bp->m_size = addr - bp->m_addr;
#ifdef OSDEBUG
                        mp->m_ents++;
#endif
		}
	SPINUNLOCK_USAV(rmap_lock, context);
	return (addr);
}
