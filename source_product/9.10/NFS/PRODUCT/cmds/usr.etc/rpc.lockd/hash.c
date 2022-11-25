/* @(#)rpc.lockd:	$Revision: 1.9.109.2 $	$Date: 95/03/21 12:12:41 $
*/
/* (#)hash.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)hash.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)hash.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

#ifdef PATCH_STRING
static char *patch_5380="@(#) PATCH_9.X: hash.o $Revision: 1.9.109.2 $ 95/03/21 PHNE_5380"
;
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* hash.c
	 * rotuines handle insertion, deletion of hashed monitor, file entries
	 */

#ifndef NULL
#define NULL 0
#endif

#include "prot_lock.h"

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS


char *malloc();
char *xmalloc();
extern int debug;
extern int HASH_SIZE;
extern struct fs_rlck *rel_fe;

typedef struct fs_rlck cache_fp;
typedef struct fs_rlck cache_me;

cache_fp *table_fp[MAX_HASHSIZE];
cache_me *table_me[MAX_HASHSIZE];
int cache_fp_len = sizeof(struct fs_rlck);
int cache_me_len = sizeof(struct fs_rlck);

/*
 * find_fe returns the cached entry;
 * it returns NULL if not found;
 */
struct fs_rlck *
find_fe(a)
reclock *a;
{
	cache_fp *cp;

	/* tjs 12/93 use file handle hash function */
	cp = table_fp[fh_hash(a->lck.fh_bytes, a->lck.fh_len)];
	while( cp != NULL) {
		if(strcmp(cp->svr, a->lck.svr) == 0 &&
		obj_cmp(&cp->fs.fh, &a->lck.fh)) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

/*
 * find_me returns the cached entry;
 * it returns NULL if not found;
 */
struct fs_rlck *
find_me(svr, proc)
char *svr;
int proc;
{
	cache_me *cp;

	cp = table_me[hash(svr, proc)];
	while( cp != NULL) {
		if(strcmp(cp->svr, svr) == 0 &&
		cp->fs.procedure == proc) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

void
insert_fe(fp)
struct fs_rlck *fp;
{
	int h;

	/* tjs 12/93 use file handle hash function */
	h = fh_hash(fp->fs.fh_bytes, fp->fs.fh_len);
	fp->nxt = table_fp[h];
	table_fp[h] = fp;
}

void
insert_me(mp)
struct fs_rlck *mp;
{
	int h;

	h = hash(mp->svr);
	mp->nxt = table_me[h];
	table_me[h] = mp;
}

void
release_fe()
{
	cache_fp *cp, *fp;
	cache_fp *cp_prev = NULL;
	cache_fp *next;
	int h;

	if(rel_fe == NULL) 
		return;
	fp = rel_fe;
	if(fp->rlckp == NULL) {
		/* tjs 12/94 use file handle hash function */
		h = fh_hash(fp->fs.fh_bytes, fp->fs.fh_len );
		next = table_fp[h];
		while((cp = next) != NULL) {
			next = cp->nxt;
			if(strcmp(cp->svr, fp->svr) == 0 &&
			obj_cmp(&cp->fs.fh, &fp->fs.fh)) {
				if(cp_prev == NULL) {
					table_fp[h] = cp->nxt;
				}
				else {
					cp_prev->nxt = cp->nxt;
				}
				free_fe(cp);
				rel_fe = NULL;
				return;
			}
			else {
				cp_prev = cp;
			}
		}
	}
}

release_me()
{
	/* we never free up monitor entry, the knowledge of contacting
	 * status monitor accumulates
	 */
}


/*
 * zap_all_locks_for(client) zips throughthe table_fp looking for
 * all fs_rlck which reference reclock's from client, and delete them.
 * this is used by prot_freeall to implement the PC-NFS cleanup...
 */
void
zap_all_locks_for(client)
	char *client;
{
	cache_fp *cp;
	cache_fp *nextcp;
	reclock *le;
	reclock *nextle;

	int i;

	if (debug)
	    logmsg((catgets(nlmsg_fd,NL_SETN,320,"zap_all_locks_for %s\n")),
		 client);
	for (i = 0; i< MAX_HASHSIZE; i++) {
		cp = table_fp[i];
		while (cp != NULL) {
			nextcp = cp->nxt;	/*  cp might change */
			le = cp->rlckp;
			while (le) {
				nextle = le->nxt;
				if (strcmp(le->lock.clnt_name, client) == 0) {
	if (debug)
		logmsg((catgets(nlmsg_fd,NL_SETN,321,"...zapping: cp@0x%x, le@0x%x\n")), cp, le);

					delete_le(cp, le);
					le->rel = 1;
					release_le(le);
				}
				le = nextle;
			}
			cp = nextcp;
		}
	}
	if (debug)
	    logmsg((catgets(nlmsg_fd,NL_SETN,322,"DONE zap_all_locks_for %s\n")), client);
}

/* tjs 12/94 file handle hash function - fixed SR#5003-169565  */
/*           similar to file handle hash in prot_shar.c        */
int
fh_hash(bytes, len)
char *bytes;
int len;
{
	int i;
        int hv;
        register char *c;
	
        hv = 0;
        for (i = len, c = bytes; --i; )
		hv += *c++;
        hv %= HASH_SIZE;
        hv = abs(hv);

        return(hv);
}
