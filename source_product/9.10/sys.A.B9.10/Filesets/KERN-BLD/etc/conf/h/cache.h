/*
 * @(#)cache.h: $Revision: 1.4.83.5 $ $Date: 93/10/11 16:46:54 $
 * $Locker:  $
 */

/*
 * cache.h--interface for control the the HP-9000 cache
 */
#ifndef __CACHE_INCLUDED
#define __CACHE_INCLUDED

/*
 * Explanation of purge options:
 *  CC_PURGE requests a cache controller purge.  Cache lines which are "dirty"
 *	(i.e., hold valid data which is not currently in the corresponding memory)
 *	may be discarded without being written to RAM.
 *  CC_FLUSH requests a cache controller flush.  Dirty lines are written out to
 *	RAM before the cache line is cleared.
 *  CC_IPURGE flushes any dirty data cache entries, then purges any instruction
 *	cache which may hold stale contents.  This operation is useful for
 *	self-modifying code.
 *  CC_EXTPURGE purges the external cache (if any).
 *      This operation is useful for user started dma requests.
 *
 */

#define CC_PURGE    0x00000001
#define CC_FLUSH    0x00000002
#define CC_IPURGE   0x00000004
#define CC_EXTPURGE 0x80000000

/*
 * How to call cachectl():
 *
 *	cachectl(function, address, length)
 *		int function;
 *		char *address;
 *		int length;
 *
 *	"function" is one of the three operations above.
 *	"address" is NULL for an operation on ALL cache lines.
 *	"address" is not NULL for an operation on a specific
 *		range of addresses (i.e., selective flushing).
 *	"length" is only used if "address" is not NULL; it
 *		indicates how many bytes should be operated
 *		on starting at the given address.
 */

#ifdef _PROTOTYPES
      extern int cachectl(int, const char *, int);
#else /* not _PROTOTYPES */
      extern int cachectl();
#endif /* not _PROTOTYPES */

#endif /* __CACHE_INCLUDED */
