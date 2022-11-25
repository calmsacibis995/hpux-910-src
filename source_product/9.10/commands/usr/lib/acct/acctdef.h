/* @(#) $Revision: 66.2 $ */     
/*
 *	defines, typedefs, etc. used by acct programs
 */


/*
 *	acct only typedefs
 */
#if hp9000s200
#define WOPR
#endif hp9000s200

#ifndef HZ
#include <sys/param.h>
#endif

#define LSZ	12	/* size of line name */
#define NSZ	8	/* sizeof login name */
#define P	0	/* prime time */
#define NP	1	/* nonprime time */

/*
 *	limits which may have to be increased if systems get larger
 */
#define A_SSIZE	1000	/* max number of sessions in 1 acct run */
#define A_TSIZE	500	/* max number of line names in 1 acct run */
#define A_USIZE	500	/* max number of distinct login names in 1 acct run */

#define EQN(s1, s2)	(strncmp(s1, s2, sizeof(s1)) == 0)
#define CPYN(s1, s2)	strncpy(s1, s2, sizeof(s1))

#define SECSINDAY	86400L
#define SECS(tics)	((double) tics)/HZ
#define MINS(secs)	((double) secs)/60
#define MINT(tics)	((double) tics)/(60*HZ)

#if NBPG>1024
#define KCORE(clicks)   ((double) clicks*(NBPG/1024))
#else
#define KCORE(clicks)   ((double) clicks/(1024/NBPG))
#endif

/* The block size reported by diskusg is 512-bytes now for all the 	  */
/* s300, s800 and s700 machines. Ming Chen 11/5/90                 	  */

#define BLOCKSIZE		512	/* Block size for reporting 	  */


/* blks * DEV_BSIZE % BLOCKSIZE ? 					  */
/* (blks * DEV_BSIZE /BLOCKSIZE) + 1L: blks * DEV_BSIZE / BLOCKSIZE );	  */
#define BLOCKS(blks)	\
	howmany(dbtob(blks), BLOCKSIZE)

