/* @(#) $Revision: 70.9 $ */
#ifndef _FTW_INCLUDED
#define _FTW_INCLUDED /* allow multiple inclusions */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Codes for the third argument to the user-supplied function
 *	which is passed as the second argument to ftw
 */

#ifdef _INCLUDE_XOPEN_SOURCE

#  include <sys/stat.h>

#  define	FTW_F	0	/* file */
#  define	FTW_D	1	/* directory */
#  define	FTW_DNR	2	/* directory without read permission */
#  define	FTW_NS	3	/* unknown type, stat failed */

#  ifdef _PROTOTYPES 
     extern int ftw(const char *, int(*)(const char *, const struct stat *, int), int);
#  else /* _PROTOTYPES */
     extern int ftw();
#  endif /* _PROTOTYPES */
#endif /* _INCLUDE_XOPEN_SOURCE */


#ifdef _INCLUDE_HPUX_SOURCE

#  define	FTW_DP	4	/* directory, all desendants have been visited */

/* Codes for nftw and nftwh option flags 
*/
#  define	FTW_PHYS	1	/* do a physical walk */
#  define	FTW_MOUNT	2	/* do not cross a mount point */
#  define	FTW_DEPTH	4	/* do a depth first search */
#  define	FTW_CHDIR	8	/* chdir to each directory before reading it */
#  define	FTW_CDF		16	/* make cdf files visible */
#  define	FTW_SERR	32	/* ignore stat errors */

   struct FTW {
     int base;      /* offset to basename of file */
     int level;     /* depth of walk, where start = 0 */
     };

#  ifdef _PROTOTYPES
     extern int nftw(const char *, int(*)(const char *, const struct stat *,
                    int, struct FTW ), int, int);
     extern int nftwh(const char *, int(*)(const char *, const struct stat *,
                    int, struct FTW ), int, int);
#  else  /* _PROTOTYPES */
     extern int nftw();
     extern int nftwh();
#  endif /* _PROTOTYPES */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef _INCLUDE_AES_SOURCE
#  define	FTW_SL	5	/* symbolic link */
#endif /* _INCLUDE_AES_SOURCE */

#ifdef __cplusplus
}
#endif

#endif  /* _FTW_INCLUDED */
