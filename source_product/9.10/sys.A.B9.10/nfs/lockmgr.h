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
#ifdef hpux
	/*
	 * Use a real file handle instead of faking it so that we are
	 * guaranteed to have the same structure and don't have to worry
	 * about keeping it in sync with other changes we may make to the fh.
	 */
	fhandle_t lh_id;
#else /* not hpux */
	struct {				/* fhandle (sort of) */
		struct lh_ufsid {
			fsid_t		lh_fsid;
			struct fid	lh_fid;
		} lh_ufs;
#define KLM_LHPAD	(KLM_LHSIZE - sizeof (struct lh_ufsid))
		char	lh_pad[KLM_LHPAD];
	} lh_id;
#endif /* hpux */
} lockhandle_t;

#ifndef hpux	/* No longer needed because we use real file handle */

#define lh_fsid	lh_id._lh_ufs._lh_fsid
#define lh_fid	lh_id._lh_ufs._lh_fid

#endif /* hpux */


/* define 'well-known' information */
#define KLM_PROTO	IPPROTO_UDP

/* define public routines */
int  klm_lockctl();
