#ifndef lint
static  char rcsid[] = "@(#)yp_cache.c:	$Revision: 1.3.109.1 $	$Date: 91/11/19 14:23:30 $ ";
#endif

#ifdef YP_CACHE
#include "ypsym.h"
#include "yp_cache.h"

/*
 **************************************************************************
 *
 *	This file contains the routines required for the new YP_CACHE'ing
 *	from HP. All data is organised into a linked list of entries under
 *	a linked list of maps, under a linked of domains. The global 'head'
 *	(defined below) points to the very first domain pointer.
 *
 *	The calls most often used by other services using YP_CACHE are
 *	'add_to_ypcachemap' and 'search_ypcachemap'. 'add_to_ypcachemap' adds
 *	an entry into a map under the specified domain. Memory is 'malloc'ed
 *	for the entry. A new map is made if it the first entry. A pointer to
 *	the entry is returned. 'search_ypcachemap' searches for an entry based
 *	on a key under the map in the specified doamin. If found, it is moved
 *	to the front of the linked list and a pointer to the entry is returned.
 * 	
 *	Some other calls of intertest are:
 *        detachentry		- detaches an entry from a map
 *        free_entry		- frees up an entry in a map
 *        get_map_state		- returns the state of a map
 *        num_entries		- returns the total num of entries ina map
 *        print_all_maps	- prints app maps, most for debugging
 *        remove_ypcachemap	- removes a map from cache
 *        set_cache_ptrs	- sets domptr and mapptr for a domain/map pair
 *        set_map_state		- sets map state to STEADY, BOOTING etc
 *        set_max_mapsize	- sets the max allowable cache entries per map.
 *	  set_max_cachesize	- sets the global max for all caches
 *
 **************************************************************************
 */


/*
 * NOTE: These two pointers are set by set_cache_ptrs only. Be careful
 *	 when you use them. DO NOT fiddle with these wildly.
 */

static domainlst	*head   = (domainlst *)0; /* head of the domainlst */
static domainlst	*domptr = (domainlst *)0; /* current domain ptr */
static maplst		*mapptr = (maplst *)0;	/* current map pointer */
static int		max_cache_size = -1;

void
detachdom(prev, d)
register domainlst *prev, *d;
{
	if (prev == (domainlst *)0) {	
		head = d->next;
	} else {
		prev->next = d->next;
	}
	d->next = 0;
} 

/* the global variable domptr should be set before this call */
void
detachmap(prev, m)
register  maplst	    *prev, *m;
{
	if (prev == (maplst *)0) {	
		domptr->firstmap = m->next;
	} else {
		prev->next = m->next;
	}
	m->next = 0;
} 

/* the global variable mapptr should be set before this call */
void
detachentry(prev, ep)
register entrylst *prev, *ep;
{
	if (prev == (entrylst *)0) {	
		mapptr->firstentry = ep->next;
	} else {
		prev->next = ep->next;
	}
	ep->next = 0;
	/* decrement ns_entries iff entry is non_sticky */
	if (ep->key_val.status == NON_STICKY)	/* decrement count */
		mapptr->ns_entries -= 1;
	else
		mapptr->s_entries -= 1;
} 

void
free_entry(epp)
register entrylst **epp;
{
register entrylst *ep = (*epp);

	if (ep->key_val.valdat.dptr != 0)
	    free(ep->key_val.valdat.dptr);
	if (ep->key_val.keydat.dptr != 0)
	    free(ep->key_val.keydat.dptr);
	memset((char *) ep, 0, sizeof(*ep));
	free((entrylst *) ep);
	ep = (entrylst *)0;
} 

void
free_entrylst (pp)
entrylst       **pp;
{
entrylst       *t, *ep;

        for (t = *pp; t != 0; ) {
                ep = t;
                t = t->next;
                (void) free_entry (&ep);
        }
	*pp = (entrylst *)0;
} 

void
free_map(mpp)
register maplst **mpp;
{
register maplst *mp = (*mpp);

	(void) free_entrylst (&(mp->firstentry));
	memset((char *)mp, 0, sizeof(*mp));
	free((maplst *) mp);
	mp = (maplst *)0;
} 

void
free_maplst (pp)
maplst       **pp;
{
maplst       *t, *mp;

        for (t = *pp; t != 0; ) {
                mp = t;
                t = t->next;
                (void) free_map (&mp);
        }
	*pp = (maplst *)0;
} 

void
free_dom(dpp)
register domainlst **dpp;
{
register domainlst *dp = (*dpp);

	(void) free_maplst (&(dp->firstmap));
	memset((char *)dp, 0, sizeof(*dp));
	free((domainlst *) dp);
	dp = (domainlst *)0;
} 

void
free_domlist (pp)
domainlst       **pp;
{
domainlst       *t, *dp;

        for (t = *pp; t != 0; ) {
                dp = t;
                t = t->next;
                (void) free_dom (&dp);
        }
	*pp = (domainlst *)0;
} 

domainlst *
makedomain(domainname) 
char	*domainname;
{
	int len = 0;
	register domainlst *dp =
	    (domainlst *) malloc(sizeof(domainlst));
	char	**dnp;

	if (dp == (domainlst *)0)
	    return ((domainlst *)0);
	
	dnp = &(dp->domainname);
	memset((char *) dp, 0, sizeof(*dp));
	len = strlen (domainname);
	if ((*dnp = malloc(len + 1)) == 0) {
		free((char *) dp);
		return (0);
	} else
		memcpy (*dnp, domainname, len);
	(*dnp)[len] = '\0';	/* null terminate domainname */
	return (dp);
} 

void
init_map (mp)
maplst *mp;
{
   if (mp) {
	mp->map_type	= LOOKUP_TYPE_MAP; /* default */
	mp->map_state	= BOOTING; /* default */
	mp->lookups	= 0;
	mp->cache_miss	= 0;
	mp->min_entries	= 0;
	mp->max_entries	= ((max_cache_size == -1) ? MAXENTRIES : max_cache_size);
	mp->ns_entries	= 0;
	mp->s_entries	= 0;
   }
} 

maplst *
makemap(mapname)
char	*mapname;
{
	int len = 0;
	register maplst *mp =
	    (maplst *) malloc(sizeof(maplst));
	char	**mnp;

	if (mp == (maplst *)0)
	    return ((maplst *)0);
	
	mnp	= &(mp->mapname);
	memset((char *) mp, 0, sizeof(*mp));
	len = strlen (mapname);
	if ((*mnp = malloc(len + 1)) == 0) {
		free((char *) mp);
		return (0);
	} else
		memcpy (*mnp, mapname, len);

	(*mnp)[len] = '\0';	/* null terminate mapname */
	init_map (mp);
	return (mp);
} 

entrylst *
makeentry(key, keylen, val, vallen)
register char	*key;
register int 	 keylen;
register char	*val;
register int 	 vallen;
{
register char	**kp, **vp;
entrylst *ep;

	if ((keylen > MAXKEYLEN) || (vallen > MAXVALLEN))
	    return ((entrylst *)0);
	ep = (entrylst *) malloc(sizeof(entrylst));
	if (ep == (entrylst *)0)
	    return ((entrylst *)0);
	
	/* initialises the struct with seq_id = 0 */
	memset((char *) ep, 0, sizeof(entrylst));
	kp = &(ep->key_val.keydat.dptr);
	if ((*kp = malloc(keylen + 1)) == 0)
		goto fail;
	else {
		ep->key_val.keydat.dsize = keylen;
		memcpy(*kp, key, keylen);
		(*kp)[keylen] = '\0';	/* null terminate key */
	}
	
	vp = &(ep->key_val.valdat.dptr);
	if ((*vp = malloc(vallen + 1)) == 0)
		goto fail;
	else {
		ep->key_val.valdat.dsize = vallen;
		memcpy (*vp, val, vallen);
		(*vp)[vallen] = '\0';	/* null terminate val */
	}
	return (ep);
fail: /* failure if here */
	(void) free_entry(&ep);
	return ((entrylst *)0);
} 

void
get_domptr(domainname, domptrp)
char	*domainname;
domainlst	**domptrp;
{
domainlst	*dp, *prev;
int	dlen;

	*domptrp = (domainlst *)0;
	prev	 = (domainlst *)0;
	dlen = strlen (domainname);
        for (dp = head; dp != 0; dp = dp->next) {
	     if ((strlen(dp->domainname) == dlen) &&
		 (memcmp (dp->domainname, domainname, dlen) == 0)) {
		  *domptrp = dp;
		   break;
	     } else {
		  prev = dp;
	     }
	}
	if (*domptrp) /* if the domain is there and */
	   if (prev) {/* it is not the first, move it to the front */
		prev->next = (*domptrp)->next;
		(*domptrp)->next = head->next;
		head = *domptrp;
	}
} 

void
get_mapptr(mapname, mapptrp)
char	*mapname;
maplst	**mapptrp;
{
maplst	*mp, *prev;
int	mlen;

	*mapptrp= (maplst *)0;
	prev  = (maplst *)0;
	mlen 	= strlen(mapname);
        for (mp = domptr->firstmap; mp != 0; mp = mp->next) {
	     if ((strlen(mp->mapname) == mlen) && 
		 (memcmp (mp->mapname, mapname, mlen) == 0)) {
		 *mapptrp = mp; 
		  break;
	     } else {
		 prev = mp;
	     }
	}
	if (*mapptrp) /* if the map is there and */
	   if (prev) {/*if it is not the 1st, move it to the front*/
		prev->next = (*mapptrp)->next;
		(*mapptrp)->next = domptr->firstmap;
		domptr->firstmap = *mapptrp;
	}
} 

/* This sets the global pointers domptr and mapptr */
void
set_cache_ptrs (domainname, mapname)
char	*domainname;
char	*mapname;
{
	(void) get_domptr (domainname, &domptr);
	if (domptr) 	/* domptr != NULL, so set mapptr */
		(void) get_mapptr (mapname, &mapptr);
	else 	/* domptr is NULL; set mapptr also to NULL */
		mapptr = (maplst *)0;
} 

void
prune_map(mp)
maplst *mp;
{
entrylst *ep, *c, *prev;
int min_entries = mp->min_entries, ns_entries = mp->ns_entries, cnt = 0;

	/* delete the least-recently-used non-sticky entries */
	prev = (entrylst *)0;
	for (c = mp->firstentry; c != (entrylst *)0;) {
	     if (c->key_val.status == NON_STICKY) { /* non-sticy entry ? */
	         cnt++; /* bad lookups not counting the sticky ones*/
	         if (cnt > min_entries) {/* remove c from the list */
		     if (prev)
			     prev->next = c->next;
		     else
			     mapptr->firstentry = c->next;
		     /* do not change the order of the following two lines */
		     ep  = c;
		     c = c->next;

		     ns_entries--;
		     ep->next = (entrylst *)0;
		     (void) free_entry(&ep);
	         } else {
		     prev  = c;
		     c = c->next;
	         }
	     } else {
		  prev  = c;
		  c = c->next;
	     }
	}
	mp->ns_entries = ns_entries;
} 

/* add_ro_cachemap adds the entry to the cache */
entrylst *
add_to_ypcachemap (domainname, mapname, keydat, valdat, entry_type)
char	*domainname, *mapname;
datum	keydat;
datum	valdat;
unsigned long	entry_type;
{
entrylst *ep, *c, *prev;
int	err = 0, cnt = 0, min_entries, max_entries;
char	**kp, **vp;
unsigned int miss, lookups;

   	(void) set_cache_ptrs(domainname, mapname);
	if (domptr == (domainlst *)0) {	/* make a new one */
		domptr = makedomain (domainname);
		if (domptr) {
			if (head) {
				domptr->next = head;
			}
			head = domptr; /* add new domain always at the head */
		} else
			return (entrylst *)0;
	}
	if (mapptr == (maplst *)0) { /* make a new maplst */
		mapptr = makemap(mapname);
		if (mapptr) {	/* attach it */
			mapptr->next = domptr->firstmap;
			domptr->firstmap = mapptr;
		} else
			return (entrylst *)0;
	}
	ep = makeentry (keydat.dptr, keydat.dsize, valdat.dptr, valdat.dsize);
	if (ep != 0) { /* we have an initialised struct */
		/* attach the entry to the map */
		ep->next	   = mapptr->firstentry;
		mapptr->firstentry = ep;
		ep->key_val.status = entry_type;
		/* increment the lookups AND the miss_count AND
		 * if the cache miss is > 1/32 * lookups, increment
		 * the cache size too, so more gets to stay.*/
		if (entry_type == NON_STICKY) {
			mapptr->ns_entries++;
			lookups = ++mapptr->lookups;
			miss  = ++mapptr->cache_miss;
			if ((miss << 5) > lookups) {
				min_entries = mapptr->min_entries;
				max_entries = mapptr->max_entries;
				if (min_entries < max_entries)
					mapptr->min_entries++;
			}
			/* shift to prevent overflow */
			if (lookups > MAXLOOKUPS) {
				mapptr->lookups    = (lookups >> 1);
				mapptr->cache_miss = (miss  >> 1);
			}
		} else { /* incr the count of sticky entries */
			mapptr->s_entries++;
		}
		return (ep);
	} else
		return (entrylst *)0;	/* no memory */
} 

/* add_n_prune_list adds the entry to the cache & prunes the list */
entrylst *
add_n_prune_list (domainname, mapname, keydat, valdat, entry_type)
char	*domainname, *mapname;
datum	keydat;
datum	valdat;
unsigned long	entry_type;
{
entrylst *e;

	e = add_to_ypcachemap (domainname, mapname, keydat, valdat, entry_type);
	if (entry_type == NON_STICKY) {
		/* prune the list. Is it gonna mess up ep ? */
		(void) prune_map(mapptr);
	}
	return (e);
} 

/*
 * copy_entry allocates ep->key_val.valdat.dsize bytes of memory for
 * *valp and copies the values from ep. Returns error if malloc fails.
 */
int copy_entry (ep, valp, vallenp)
entrylst	*ep;
char    **valp;             /* ptr to value array returned */
int     *vallenp;           /* ptr to len of val array in bytes */
{
char	*val = (*valp);
int	vallen = (*vallenp);

	val = malloc(ep->key_val.valdat.dsize + 1);
	if (val == 0) {
		return (YPERR_RESRC);
	}
	vallen = ep->key_val.valdat.dsize;
	/* copy all including the null byte */
	memcpy(val, ep->key_val.valdat.dptr, vallen + 1);
	return (0);
} 

/*
 * search_ypcachemap will return a pointer to
 * the entry being looked for if found.
 */
entrylst *
search_ypcachemap (domainname, mapname, keydat)
char	*domainname;
char	*mapname;
datum	keydat;
{
entrylst *prev;		/* the one previous to entry if found */
register entrylst *c, *ep;
int	  keylen;
char	 *key;

	(void) set_cache_ptrs(domainname, mapname);
	if (mapptr != 0) {
    	    prev = (entrylst *)0;
	    key = keydat.dptr;
	    keylen = keydat.dsize;
    	    for (c=mapptr->firstentry; c != 0;) {
		if ((c->key_val.keydat.dsize == keylen) &&
		    (memcmp(key, c->key_val.keydat.dptr, keylen) == 0)) {
			/*
			 * increment lookups iff entry is non-sticky */
			if (c->key_val.status == NON_STICKY)
				mapptr->lookups++;
			/*
			 * found! move the entry to the front. 
			 */
			if (prev != (entrylst *)0) {/* c is not the 1st entry */
				/* detach c from the list */
				prev->next = c->next;
				/* move c to the beginning */
				c->next = mapptr->firstentry;
				mapptr->firstentry = c;
			}
			return (c);
		}
		 else {
		    prev  = c;
		    c = c->next;
		}
    	    }
	    return ((entrylst *)0);	/* NO SUCH ENTRY */ 
	}
	return ((entrylst *)0);	/* NO SUCH MAP OR DOMAIN */
} 

void
print_entry(ep)
register entrylst *ep;
{
	fprintf(stderr,"\t\tkey = %s, keylen = %d, val = %s, vallen = %d\n",ep->key_val.keydat.dptr, ep->key_val.keydat.dsize, ep->key_val.valdat.dptr, ep->key_val.valdat.dsize);
} 

void
print_map(mp)
register maplst *mp;
{
entrylst	*ep;

	fprintf(stderr,"\tmapname = %s, map_type = %d, map_state = %d, ns_entries = %d, s_entries = %d, min_entries = %d, cache_miss = %d, lookups = %d\n",mp->mapname, mp->map_type, mp->map_state, mp->ns_entries, mp->s_entries, mp->min_entries, mp->cache_miss, mp->lookups);
	for (ep = mp->firstentry; ep != 0; ep = ep->next) {
		print_entry(ep);
  }
} 

void
print_all_maps()
{
domainlst	*dp;
maplst		*mp;

    if (head != 0)
	for (dp = head; dp != 0; dp=dp->next) {
		fprintf(stderr, "domain = %s\n",dp->domainname);
		for (mp = dp->firstmap; mp != 0; mp=mp->next) {
			print_map(mp);
		}
	}
} 

int
get_s_entries(domainname, mapname)
char	*domainname, *mapname;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		return(mapptr->s_entries);
	else
		return(-1);
} 

int
get_ns_entries(domainname, mapname)
char	*domainname, *mapname;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		return(mapptr->ns_entries);
	else
		return(-1);
} 

int
get_entries(domainname, mapname)
char	*domainname, *mapname;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		return((mapptr->ns_entries) + (mapptr->s_entries));
	else
		return(-1);
} 

void
set_max_cachesize(max_entries)
int     max_entries;
{
        max_cache_size = max_entries;
}

int
set_max_mapsize(domainname, mapname, max_entries)
char	*domainname, *mapname;
int	max_entries;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr) {
		mapptr->max_entries = max_entries;
		return (0);
	} else
		return(-1);
} 

void
set_map_state(domainname, mapname, map_state)
char	*domainname, *mapname;
int	map_state;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		mapptr->map_state = map_state;
} 

int
get_map_state(domainname, mapname)
char	*domainname, *mapname;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		return(mapptr->map_state);
	else
		return(NO_SUCH_MAP);
} 

void
set_map_type(domainname, mapname, map_type)
char	*domainname, *mapname;
int	map_type;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		mapptr->map_type = map_type;
} 

int
get_map_type(domainname, mapname)
char	*domainname, *mapname;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr)
		return(mapptr->map_type);
	else
		return(NO_SUCH_MAP);
} 

/* delete all entries, init map and set its type & state */
void
reset_ypcachemap (domainname, mapname)
char	*domainname, *mapname;
{
int	cache_size;

	set_cache_ptrs(domainname, mapname);
	if (mapptr) {
		free_entrylst(&(mapptr->firstentry));
		cache_size = mapptr->max_entries;
		init_map (mapptr);
		mapptr->max_entries = cache_size;
	}
} 

void
remove_ypcachemap (domainname, mapname)
char	*domainname, *mapname;
{
	set_cache_ptrs(domainname, mapname);
	if (mapptr) {
		detachmap((maplst *)0, mapptr);
		free_map(&mapptr);
		mapptr = (maplst *)0;
	}
} 

bool_t
xdr_entrylst(xdrs, objp)
	XDR *xdrs;
	entrylst *objp;
{
	if (!xdr_long(xdrs, &objp->seq_id)) {
		return (FALSE);
	}
	if (!xdr_ypresp_key_val(xdrs, &objp->key_val)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next, sizeof(entrylst), xdr_entrylst)) {
		return (FALSE);
	}
	return (TRUE);
} 

bool_t
xdr_maplst(xdrs, objp)
	XDR *xdrs;
	maplst *objp;
{
	if (!xdr_int(xdrs, &objp->map_state)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->map_type)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->min_entries)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->max_entries)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->ns_entries)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->s_entries)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->cache_miss)) {
		return (FALSE);
	}
	if (!xdr_u_int(xdrs, &objp->lookups)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->firstentry, sizeof(entrylst), xdr_entrylst)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->mapname, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next, sizeof(maplst), xdr_maplst)) {
		return (FALSE);
	}
	return (TRUE);
} 

bool_t
xdr_domainlst(xdrs, objp)
	XDR *xdrs;
	domainlst *objp;
{
	if (!xdr_pointer(xdrs, (char **)&objp->domainname, sizeof(char), xdr_char)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->firstmap, sizeof(maplst), xdr_maplst)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->next, sizeof(domainlst), xdr_domainlst)) {
		return (FALSE);
	}
	return (TRUE);
} 

bool_t
xdr_update_entry(xdrs, objp)
	XDR *xdrs;
	update_entry *objp;
{
	if (!xdr_pointer(xdrs, (char **)&objp->dm, sizeof(struct ypreq_nokey), xdr_ypreq_nokey)) {
		return (FALSE);
	}
	if (!xdr_pointer(xdrs, (char **)&objp->ep, sizeof(entrylst), xdr_entrylst)) {
		return (FALSE);
	}
	return (TRUE);
} 
#endif YP_CACHE
