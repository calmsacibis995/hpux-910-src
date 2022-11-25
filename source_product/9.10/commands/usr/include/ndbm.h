/* @(#) $Revision: 66.2 $ */

/*
 * Hashed key data base library.
 */
#ifndef _NDBM_INCLUDED /* allow multiple inclusions */
#define _NDBM_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_HPUX_SOURCE

#define PBLKSIZ 1024
#define DBLKSIZ 4096

typedef struct {
    int     dbm_dirf;               /* open directory file */
    int     dbm_pagf;               /* open page file */
    int     dbm_flags;              /* flags, see below */
    long    dbm_maxbno;             /* last ``bit'' in dir file */
    long    dbm_bitno;              /* current bit number */
    long    dbm_hmask;              /* hash mask */
    long    dbm_blkptr;             /* current block for dbm_nextkey */
    int     dbm_keyptr;             /* current key for dbm_nextkey */
    long    dbm_blkno;              /* current page to read/write */
    long    dbm_pagbno;             /* current page in pagbuf */
    char    dbm_pagbuf[PBLKSIZ];    /* page file block buffer */
    long    dbm_dirbno;             /* current block in dirbuf */
    char    dbm_dirbuf[DBLKSIZ];    /* directory file block buffer */
} DBM;

#define _DBM_RDONLY	0x1	/* data base open read-only */
#define _DBM_IOERR	0x2	/* data base I/O error */

#define dbm_rdonly(db)	((db)->dbm_flags & _DBM_RDONLY)

#define dbm_error(db)	((db)->dbm_flags & _DBM_IOERR)
	/* use this one at your own risk! */
#define dbm_clearerr(db)	((db)->dbm_flags &= ~_DBM_IOERR)

/* for flock(2) and fstat(2) */
#define dbm_dirfno(db)	((db)->dbm_dirf)
#define dbm_pagfno(db)	((db)->dbm_pagf)

typedef struct {
	char	*dptr;
	int	dsize;
} datum;

/*
 * flags to dbm_store()
 */
#define DBM_INSERT	0
#define DBM_REPLACE	1

#if defined(__STDC__) || defined(__cplusplus)
   extern DBM *dbm_open(const char *, int, int);
   extern void dbm_close(DBM *);
   extern datum dbm_fetch(DBM *, datum);
   extern datum dbm_firstkey(DBM *);
   extern datum dbm_nextkey(DBM *);
   extern long dbm_forder(DBM *, datum);
   extern int dbm_delete(DBM *, datum);
   extern int dbm_store(DBM *, datum, datum, int);
#else /* not __STDC__ || __cplusplus */
   extern DBM *dbm_open();
   extern void dbm_close();
   extern datum dbm_fetch();
   extern datum dbm_firstkey();
   extern datum dbm_nextkey();
   extern long dbm_forder();
   extern int dbm_delete();
   extern int dbm_store();
#endif /* else not __STDC__ || __cplusplus */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _NDBM_INCLUDED */
