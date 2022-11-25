/* @(#) $Revision: 1.17.83.4 $ */      
#ifndef _SYS_MAP_INCLUDED /* allows multiple inclusion */
#define _SYS_MAP_INCLUDED

/*
 * Resource Allocation Maps.
 *
 * Associated routines manage sub-allocation of an address space using
 * an array of segment descriptors.  The first element of this array
 * is a map structure, describing the arrays extent and the name
 * of the controlled object.  Each additional structure represents
 * a free segment of the address space.
 *
 * A call to rminit initializes a resource map and may also be used
 * to free some address space for the map.  Subsequent calls to rmalloc
 * and rmfree allocate and free space in the resource map.  If the resource
 * map becomes too fragmented to be described in the available space,
 * then some of the resource is discarded.  This may lead to critical
 * shortages, but is better than not checking (as the previous versions
 * of these routines did) or giving up and calling panic().  The routines
 * could use linked lists and call a memory allocator when they run
 * out of space, but that would not solve the out of space problem when
 * called at interrupt time.
 *
 * N.B.: The address 0 in the resource address space is not available
 * as it is used internally by the resource map routines.
 */
struct map {
	struct	mapent *m_limit;	/* address of last slot in map */
	char	*m_name;		/* name of resource we use m_name */
					/* when the map overflows, in warning */
					/* messages */
#ifdef OSDEBUG
	/* Used to track the number of entries in the map. */
	unsigned int m_ents;
#endif /* OSDEBUG */
};
struct mapent
{
	unsigned long m_size;	/* size of this segment of the map */
	unsigned long m_addr;	/* resource-space addr of start of segment */
};

#ifdef _KERNEL
#define	NSYSMAP	(nproc > 800 ? nproc : 800)    
extern	struct	map *sysmap;
#ifdef	__hp9000s800
#define NMMAP   200
extern  struct  map *quad34map;
#endif /* __hp9000s800 */

extern  struct  map *mbmap;

#endif /* _KERNEL */
#endif /* _SYS_MAP_INCLUDED */
