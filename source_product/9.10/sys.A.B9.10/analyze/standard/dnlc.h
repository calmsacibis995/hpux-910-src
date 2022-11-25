#ifdef VNODE

#define NC_SIZE                 356     /* size of name cache */
#define	NC_HASH_SIZE		8	/* size of hash table */

#define	NC_HASH(namep, namlen, vp)	\
	((namep[0] + namep[namlen-1] + namlen + (int) vp) & (NC_HASH_SIZE-1))

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

/*
 * LRU list of cache entries for aging.
 */
struct	nc_lru	{
	struct	ncache	*hash_next, *hash_prev;	/* hash chain, unused */
	struct 	ncache	*lru_next, *lru_prev;	/* LRU chain */
};

#endif VNODE
