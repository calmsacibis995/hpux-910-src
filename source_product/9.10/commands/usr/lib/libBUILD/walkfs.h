/*
 * walkfs.h -- include file for walkfs.c
 *
 *  by  Byron Jenings & R. Scott Holbrook
 */

#ifndef _WALKFS_INCLUDED
#define _WALKFS_INCLUDED

#include <values.h>

/*
 * Values returned by user functions to ftw or walkfs
 */
#define WALKFS_OK        0
#define WALKFS_RETRY     (HIBITI)
#define WALKFS_SKIP      (HIBITI>>1)
#define WALKFS_NOCHDIR   (HIBITI>>2) /* chdir failed, also passed
					  to user function */

/*
 * Bit flags for walkfsctl()
 */
#define WALKFS_LSTAT      0x0001 /* use lstat() instead of stat()     */
#define WALKFS_DOCDF      0x0004 /* Search for CDFs                   */
#define WALKFS_SHOWCDF    0x0008 /* Expand CDFs in fullpath           */
#define WALKFS_TELLPOPDIR 0x0020 /* Call user fn when done with dir   */
#define WALKFS_LEAVEDOT   0x0040 /* leave leading ./ on file names    */
#define WALKFS_SLOW       0x0080 /* Don't use chdir during traversal  */

/*
 * Extra codes passed to user function.  Note that WALKFS_NOCHDIR may
 * be passed to the user function.
 */
#define WALKFS_ERROR        0 /* An error condition exists          */
#define WALKFS_NONDIR       1 /* non-directory (file, symlink, etc) */
#define WALKFS_DIR          2 /* directory                          */
#define WALKFS_POPDIR       3 /* walkfs() did a chdir("..")         */
#define WALKFS_NOSTAT    0x10 /* stat failed                        */
#define WALKFS_NOREAD    0x20 /* can't read this dir                */
#define WALKFS_NOSEARCH  0x40 /* can't search this dir              */
#define WALKFS_CYCLE	 0x80 /* a directory creates a cycle	    */
/* Can also be passed WALKFS_NOCHDIR */

#ifdef __cplusplus
extern "C" {
extern int walkfs(char *in_path, int (*in_fn)(), int depth, int cntl);
}
#else /* C */
extern int walkfs();
#endif /* C */

struct walkfs_info
{
        struct walkfs_info *parent; /* parent directory's info        */
        char *relpath;          /* path name relative to start        */
        char *fullpath;         /* absolute path name                 */
        char *shortpath;        /* short but correct path to file     */
        char *basep;            /* start of basename                  */
        char *endp;             /* where NULL char should be in names */
	int buflen;		/* length of new buffer allocated     */
        struct stat st;         /* stat result struct                 */
        int ismountpoint;       /* TRUE if mount point                */
};

#endif /* _WALKFS_INCLUDED */
