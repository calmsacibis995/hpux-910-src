/*
 * dnlc.h: $Revision: 1.3.83.4 $ $Date: 93/09/17 18:25:30 $
 * $Locker:  $
 */

#ifndef _SYS_DNLC_INCLUDED
#define _SYS_DNLC_INCLUDED

/*
 * Copyright (c) 1984 Sun Microsystems Inc.
 */

/*
 * This structure describes the elements in the cache of recent
 * names looked up.
 */

#define	NC_NAMLEN	15	/* maximum name segment length we bother with*/

struct	ncache {
	struct	ncache	*hash_next, *hash_prev;	/* hash chain, MUST BE FIRST */
	struct 	ncache	*lru_next, *lru_prev;	/* LRU chain */
	struct	vnode	*vp;			/* vnode the name refers to */
	struct	vnode	*dp;			/* vno of parent of name */
	char		namlen;			/* length of name */
	char		name[NC_NAMLEN];	/* segment name */
	struct	ucred	*cred;			/* credentials */
};

#define	ANYCRED	((struct ucred *) -1)
#define	NOCRED	((struct ucred *) 0)
/*
int	ncsize;
struct	ncache *ncache;
*/


#define	NC_HASH_SIZE		256	/* size of hash table */

/*
 * Stats on usefulness of name cache.
 */
struct	ncstats {
	int	hits;		/* hits that we can really use */
	int	misses;		/* cache misses */
	int	long_enter;	/* long names tried to enter */
	int	long_look;	/* long names tried to look up */
	int	lru_empty;	/* LRU list empty */
	int	purges;		/* number of purges of cache */
};

/*
 * Hash list of name cache entries for fast lookup.
 */
struct	nc_hash	{
	struct	ncache	*hash_next, *hash_prev;
};

struct	nc_lru	{
	struct	ncache	*hash_next, *hash_prev;	/* hash chain, unused */
	struct 	ncache	*lru_next, *lru_prev;	/* LRU chain */
};

#endif /* _SYS_DNLC_INCLUDED */
