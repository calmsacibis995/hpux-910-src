/* yp_cache.h: $Revision: 1.2.109.1 $       $Date: 91/11/19 14:23:37 $ */
#ifndef YP_CACHE_INCLUDED
#define YP_CACHE_INCLUDED

#ifdef YP_CACHE
/* when lookups>MAXLOOKUPS, right shift it & cache_miss to prevent overflow */
#define MAXLOOKUPS	1000000

#define MAXKEYLEN	64
#define MAXVALLEN	256

/* define default size of cache */
#define MAXENTRIES 128

#define NO_SUCH_MAP	-1

/* define entry types. */
#define NON_STICKY	0
#define STICKY		1

/* map types */
#define LOOKUP_TYPE_MAP 	0
#define AUTOXFER_TYPE_MAP	1

/* map states */
#define BOOTING		0
#define STEADY_STATE	1
#define BAD_STATE	2

struct entrylst {
	long	seq_id;
	struct ypresp_key_val key_val;
	struct entrylst *next;
};
typedef struct entrylst entrylst;
bool_t xdr_entrylst();

struct maplst {
	int map_state;	/* booting, steady, bad */
	int map_type;   /* LOOKUP, AUTOXFR */
	int min_entries;/* num of entries reqd for 93% cache hit */
	int max_entries;/* max num of entries allowed, default = MAXENTRIES */
	int ns_entries;	/* non-sticky entries */
	int s_entries;	/* sticky entries */
	u_int cache_miss;
	u_int lookups;
	struct entrylst *firstentry;
	char *mapname;
	struct maplst *next;
};
typedef struct maplst maplst;
bool_t xdr_maplst();

struct domainlst {
	char *domainname;
	struct maplst *firstmap;
	struct domainlst *next;
};
typedef struct domainlst domainlst;
bool_t xdr_domainlst();

struct update_entry {
	struct ypreq_nokey *dm;
	struct entrylst *ep;
};
typedef struct update_entry update_entry;
bool_t xdr_update_entry();

#endif /* YP_CACHE */
#endif /* YP_CACHE_INCLUDED */
