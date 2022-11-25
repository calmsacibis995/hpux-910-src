#ifdef MODULE_ID
/*
 * @(#)lockmgr.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 19:07:33 $
 * $Locker:  $
 */
#endif /* MODULE_ID */

/*
 * REVISION: @(#)10.3
 */

/*
 * (c) Copyright 1988 Hewlett-Packard Company
 * (c) Copyright 1986 Sun Microsystems, Inc.
 */

/*
 * Header file for Kernel<->Network Lock-Manager implementation
 */

/* NOTE: size of a lockhandle-id should track the size of an fhandle */
#define KLM_LHSIZE	32

/* the lockhandle uniquely describes any file in a domain */
typedef struct {
	struct vnode *lh_vp;			/* vnode of file */
	char *lh_servername;			/* file server machine name */
	/*
	 * Use a real file handle instead of faking it so that we are
	 * guaranteed to have the same structure and don't have to worry
	 * about keeping it in sync with other changes we may make to the fh.
	 */
	fhandle_t lh_id;
} lockhandle_t;



/* define 'well-known' information */
#define KLM_PROTO	IPPROTO_UDP

/* define public routines */
int  klm_lockctl();
