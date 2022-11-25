/*
 * @(#)dbd.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 18:24:35 $
 * $Locker:  $
 */

#ifndef _DBD_INCLUDED
#define _DBD_INCLUDED

/*
 * The dbd (disk block descriptor) is a flag followed by information
 * that is vnode type (nfs, ufs, dux) specific.
 */

typedef struct dbd {
	u_int	dbd_type:4;
	u_int	dbd_data:28;
} dbd_t;

union idbd {
	dbd_t idbd;
	u_int idbd_int;
};

#define	DBD_NONE	0	/* There is no copy of this page on 	*/
				/* disk.				*/
#define	DBD_FSTORE	1	/* This page matches a block on the     */
				/* front store.				*/
#define DBD_BSTORE	2	/* This page matches a block on the	*/
				/* back store				*/
#define	DBD_DZERO	3	/* This is a demand zero page.  No	*/
				/* space is allocated now.  When a 	*/
				/* fault occurs, allocate a page and	*/
				/* initialize it to all zeros.		*/
#define	DBD_DFILL	4	/* This is a demand fill page.  No	*/
				/* space is allocated now.  When a	*/
				/* fault occurs, allocate a page and	*/
				/* do not initialize it at all.  It	*/
				/* will be initialized by reading in 	*/
				/* data from disk.			*/
#define DBD_HOLE	5	/* A hole in a memory mapped file.	*/
				/* Corresponding pages are marked	*/
				/* read-only, space will be allocated	*/
				/* when a write occurs.			*/

/*
 * An unitialized DBD has DBD_DINVAL in the dbd_data field.
 */
#define DBD_DINVAL	((u_int)0x0fffffff)

/*
 * Useful macros.
 */
#ifdef _KERNEL
dbd_t		*finddbd();
#endif /* _KERNEL */

#endif /* not _DBD_INCLUDED */
