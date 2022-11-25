/* @(#) $Revision: 70.1 $ */

/*
 *	cache.c
 *
 *	Series 300 dynamic loader
 *
 *	code to implement a cache of recently resolved symbols
 *
 */


#if	CACHE_SIZE

#include	"dld.h"


struct cache_entry
{
	int flags;
	import_t import;
	dynamic_t target_DYNAMIC;
	export_t target_export;
};

static struct cache_entry cache[CACHE_SIZE];


int search_cache (import_t import, int mode, dynamic_t *target_DYNAMIC, export_t *target_export)
/*
 * on entry:
 *	import = import entry to locate
 *	mode = options
 * returns:
 *	non-zero normally
 *	zero if symbol not found
 * on exit:
 *	*target_DYNAMIC = DYNAMIC structure of file containing definition
 *	*target_export = export table entry for symbol
 * effects:
 *	searches cache of recently bound symbols for name
 */
{
	struct cache_entry *entry;

	STACK_CHECK
	/* look in cache */
#if	DEBUG & 16
	printf("cache search...\n");
#endif
	entry = cache + (dld_last_hash & (CACHE_SIZE -1));
	if (import != entry->import)
		return(0);
	if (mode != entry->flags)
		return(0);
#if	DEBUG & 16
	printf("cache hit\n");
#endif
	*target_DYNAMIC = entry->target_DYNAMIC;
	*target_export = entry->target_export;
	return(-1);
}


void enter_cache (import_t import, dynamic_t target_DYNAMIC, export_t target_export, int mode)
/*
 * on entry:
 *	import = import entry of reference
 *	target_DYNAMIC = DYNAMIC structure of file containing definition
 *	target_export = export entry of definition
 *	mode = options
 * effects:
 *	enters name into cache of recently bound symbols
 */
{
	struct cache_entry *entry;

	STACK_CHECK
	entry = cache + (dld_last_hash & (CACHE_SIZE -1));
	entry->import = import;
	entry->flags = mode;
	entry->target_DYNAMIC = target_DYNAMIC;
	entry->target_export = target_export;
}


void invalidate_cache (dynamic_t target_DYNAMIC)
/*
 * on entry:
 *	target_DYNAMIC = file containing invalidated entries
 *	                 NULL to invalidate the whole cache
 * effects:
 *	purges all symbol definitions associated with target_DYNAMIC
 */
{
	int i;

	STACK_CHECK
	for (i = 0; i < CACHE_SIZE; ++i)
	{
		if (target_DYNAMIC == NULL || cache[i].target_DYNAMIC == target_DYNAMIC)
			cache[i].import = NULL;
	}
}

#endif	/* CACHE_SIZE */
