/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfs_xdr.c,v $
 * $Revision: 1.9.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:08:54 $
 */

/* BEGIN_IMS_MOD  
******************************************************************************
****								
**** 	nfs_xdr - XDR routines for NFS data structures
****								
******************************************************************************
* Description
* 	These are the XDR routines used to serialize and deserialize the
*       various structures passed as parameters accross the network between
*       NFS clients and servers.
*
* Externally Callable Routines
*	xdr_fhandle - encode/decode the file handle structure
*       wafree - free the structures associated with a write request
*       wawakeup - wakeup the process waiting to free structures after
*               the last mbuf in a write request has been reached
*	xdr_writeargs - encode/decode the arguments for a remote write
*	xdr_fattr - encode/decode the file attributes structure
*	xdr_readargs - encode/decode the arguments for a remote read
*	rrokfree - free the structures associated with a read reply
*	rrokwakeup - wakeup the process waiting to free structures after
*		the last mbuf in a read reply has been reached
*	xdr_rrok - encode/decode the status OK portion of remote read reply
*	xdr_rdresult - encode/decode the nfs read result structure
*	xdr_sattr - encode/decode the file attributes that can be set
*	xdr_attrstat - encode/decode the reply status with file attributes
*	xdr_srok - encode/decode the OK part of read sym link reply union
*	xdr_rdlnres - encode/decode the result of reading a symbolic link
*	xdr_rddirargs - encode/decode the arguments to readdir
*	xdr_putrddirres - encode the read directory results
*	xdr_getrddirres - decode the read directory results
*	xdr_diropargs - encode/decode the arguments for directory operations
*	xdr_drok - encode/decode the OK part of directory operation result
*	xdr_diropres - encode/decode the results from directory operation
*	xdr_timeval - encode/decode the timeval structure
*	xdr_saargs - encode/decode the arguments to setattr
*	xdr_creatargs - encode/decode the arguments to create and mkdir
*	xdr_linkargs - encode/decode the arguments to link
*	xdr_rnmargs - encode/decode the arguments to rename
*	xdr_slargs - encode/decode the arguments to symlink
*	xdr_fsok - encode/decode the OK part of statfs operation
*	xdr_statfs - encode/decode the results of the statfs operation
*
* Test Module
*	$SCAFFOLD/nfs/*
*
* To Do List
*
* Notes
*
* Modification History
*     02/03/92          kls     Add wafree, wawakeup and modified
*                               xdr_writeargs.  Actually done by cwb.
*
******************************************************************************
* END_IMS_MOD */

/*
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1987. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/mbuf.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../netinet/in.h"
#ifdef NTRIGGER
#include "../h/trigdef.h"
#endif NTRIGGER

#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

#ifdef NFSDEBUG
extern int nfsdebug;

char *xdropnames[] = {"encode", "decode", "free"};
#endif



/* BEGIN_IMS xdr_fhandle *
 ********************************************************************
 ****
 ****		xdr_fhandle(xdrs, fh)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	fh		pointer to the file handle
 *
 * Output Parameters
 *	*fh		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	NFS_FHSIZE	size of the file handle
 *
 * Description
 *	This serializes/deserializes the file handle structure which is
 *	passed as a parameter across the network between NFS clients and
 *	servers. The file handle is essentially treated as a sequence of
 *	bytes.
 *
 * Algorithm
 *	{ call xdr_opaque to take care of the file handle as a sequence
 *		of bytes
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. is NFSDEBUG necessary?
 *	. the XDR_FREE option can be handled first to speed up the 
 *	  opration
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_opaque
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_fhandle */
bool_t
xdr_fhandle(xdrs, fh)
	XDR *xdrs;
	fhandle_t *fh;
{

	if (xdr_opaque(xdrs, (caddr_t)fh, NFS_FHSIZE)) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_fhandle: %s %o %d\n",
		    xdropnames[(int)xdrs->x_op], fh->fh_fsid, fh->fh_fno);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_fhandle %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}



/* BEGIN_IMS wainfo *
 ********************************************************************
 ****
 ****		wainfo
 ****
 ********************************************************************
 * Fields
 *	wi_func		routine (wafree) that waits for lower levels
 *			    to free file system buffer; frees wainfo
 *			    struct before exiting.
 *	wi_done		boolean indicating whether or not lower levels
 *			    have freed file system buffer.
 *	wi_want		boolean indicating whether or not nfs code is
 *			    waiting for the lower level to free the
 *			    the file system buffer; under normal
 *			    conditions, this should not be true!
 *	wi_data		address of data (for info only)
 *	wi_count	number of bytes of data
 *
 ********************************************************************
 * END_IMS wainfo */

struct wainfo {
	int	(*wi_func)();
	int	wi_done;
	int	wi_want;
	caddr_t	wi_data;
	int	wi_count;
};
	

/* BEGIN_IMS wafree *
 ********************************************************************
 ****
 ****		wafree(wip)
 ****
 ********************************************************************
 * Input Parameters
 *	wip		pointer to wainfo structure
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	This routinefree the structure associated with the write request.
 *      This procedure is called by clntkudp_callit via (xdr->x_public)
 *      after the reply has been received by the server. The request
 *      should be freed.
 *
 * Called By 
 *	clntkudp_callit (via xdrs->x_public)
 *
 ********************************************************************
 * END_IMS wafree */
static
wafree(wip)
	struct wainfo *wip;
{
	int s;
	int old;

	/*
	 * Under normal conditions this routine should not sleep
	 * because the client callit code does not call this routine
	 * (via xdrs->x_public) until after the reply has been 
	 * received and the request should be freed long before the
	 * reply is received.  
	 */
        /*
	 * Add ifdef MP to protect from MP lan driver
	 */
#ifdef MP
	old = sleep_lock();
	while (wip->wi_done == 0 ){
	        wip->wi_want = 1;
		sleep_then_unlock((caddr_t)wip, PZERO-1, old);
		if (wip->wi_done !=0) { 
		    goto loop_done;
		    }
                old=sleep_lock();
        }
	sleep_unlock(old);
loop_done:

#else  /* not MP */

	s = splimp();
	while (wip->wi_done == 0) {
		wip->wi_want = 1;
		sleep((caddr_t)wip, PZERO-1);
	}
	(void) splx(s);

#endif /* MP */

	kmem_free(wip,sizeof(struct wainfo));
}


/* BEGIN_IMS wawakeup *
 ********************************************************************
 ****
 ****		wawakeup(wip)
 ****
 ********************************************************************
 * Input Parameters
 *	wip		pointer to wainfo structure
 *
 * Output Parameters
 *	none
 *
 * Return Value
 * 	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 *      This routine is added with changes in xdr_writearg. wainfo is
 *      added. In xdr_writearg routine, via xdrmbuf_putbuf, wawakeup is
 *      set in func field of mbuf via mclgetx function. The mbuf
 *      allocated by xdrmbuf_putbuf is for write request. 
 *      When the mbuf is finaaly freed by the driver. The driver(or
 *      netisr) calls wawakeup thru M_CLFUN as part of the process
 *      of freeing the mbuf. wawakeup sets the wi_done flag to true.
 *      And wakeup the process( which had called wafree) sleeping
 *      on this structure. Through wafree, the buffer associated 
 *      with the writearg large buffer is freed.
 * 
 * Called By 
 *	M_CLFUN (via m->m_clfun field of MF_LARGEBUF type mbuf)
 *
 ********************************************************************
 * END_IMS wawakeup */
static
wawakeup(wip)
	struct wainfo *wip;
{
        int old;
	/* expects to be called at splimp */
#ifdef MP
        old = sleep_lock();
	wip->wi_done = 1;
	if (wip->wi_want) {
		wakeup((caddr_t)wip);
		}
        sleep_unlock(old);
#else / * not MP */

	wip->wi_done = 1;
	if (wip->wi_want) {
		wakeup((caddr_t)wip);
	}
#endif
}





/* BEGIN_IMS xdr_writeargs *
 ********************************************************************
 ****
 ****		xdr_writeargs(xdrs, wa)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	wa		pointer to nfswriteargs struct, which has arguments
 *			for remote write
 *
 * Output Parameters
 *	wa->		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	NFS_MAXDATA	maximum amount of data that may be sent on a remote
 *			write at any one time
 *
 *      NFS_MAXIOVEC    maximum number of the iovec array in the
 *                      nfs_writeargs
 *
 * Description
 *	This serializes/deserializes the nfswriteargs structure that 
 *	represents the arguments to the remote write and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers.  We call routines which handle each component of
 *	the nfswriteargs structure and return the resulting value.
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the file handle
 *	       xdr_long to handle the beginning offset
 *	       xdr_long to handle the current offset
 *	       xdr_long to handle the total write count so far
 *
 *             select (xdrs->x_op){
 *
 *              case XDR_ENCODE :
 *                   allocate wainfo structure,
 *                   set wi_func to wafree,
 *                   init  wi_done and wi_want to 0,
 *                   set xdrs->x_public to wip,
 *                   call xdrmbuf_putbuf to append the buufer
 *                        to append the mbuf chain
 *                   return;
 *
 *              case XDR_DECODE:
 *
 *                   call xdrmbuf_getvec to decode the data
 *                   from the xdr stream to wa.
 *                   if iovcnt > MAXIOVEC {
 *                       copy all data to one buffer,
 *                       allocate memory and call xdr_opaque
 *                       return;
 *                   }
 *              case XDR_FREE:
 *                    free the data buffer,
 *                    return;
 *              }  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. the XDR_FREE option could be handled first to speed up the 
 *	  opration?
 *	. instead of doing a byte copy from the input buffer to the xdr_stream
 *	  it might be advantageous to just append the buffer to the mbuf
 *	  chain by calling xdrmbuf_putbuf. This will result in a performance
 *	  improvement if the memory to memory copy takes a long time and there
 *	  is no overlap with any i/o. Warning: One will have to make sure that
 *	  the only routines to call xdr_writeargs use a xdr mbuf stream and
 *	  that they do not return the buffer before the stuff is sent out on
 *	  the wire. Also note that this is only for encode.
 *
 * Notes
 *
 * Modification History
 *
 *    02/03/92          kls     Changed the encoding with wainfo
 *                              structure, and use xdrmbuf_putbuf.
 *                              Changed the decoding with xdrmbuf_getvec
 *                              avoid copy to xdr temporary buffer.  Actually
 *				done by cwb.
 * External Calls
 *	xdr_fhandle
 *	xdr_long
 *	xdr_bytes
 *	dprint
 *      xdrmbuf_putbuf
 *      xdrmbuf_getvec
 *
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_writeargs */
bool_t
xdr_writeargs(xdrs, wa)
	XDR *xdrs;
	struct nfswriteargs *wa;
{
        caddr_t data;
        u_int  count;
        struct wainfo *wip;

	if (xdr_fhandle(xdrs, &wa->wa_fhandle) &&
	    xdr_long(xdrs, (long *)&wa->wa_begoff) &&
	    xdr_long(xdrs, (long *)&wa->wa_offset) &&
	    xdr_long(xdrs, (long *)&wa->wa_totcount)) {

                switch(xdrs->x_op){

                  case XDR_ENCODE:
                        /* client side */

                        /* kmem_alloc will not return until memory is alloc */

                        wip = (struct wainfo*)kmem_alloc(sizeof(struct wainfo));
                        wip->wi_func  = wafree;
                        wip->wi_done  = 0;
                        wip->wi_want  = 0;
                        wip->wi_data  = wa->wa_data  /* informational only */;
                        wip->wi_count = wa->wa_count /* informational only */;
                        xdrs->x_public = (caddr_t)wip;
                        if (xdrmbuf_putbuf(xdrs, wa->wa_data,
                            (u_int)wa->wa_count, wawakeup, (int)wip)) {
#ifdef NFSDEBUG
                                dprint(nfsdebug, 6,
                                    "xdr_writeargs: %s off %d cnt %d wip %x\n",
                                    xdropnames[(int)xdrs->x_op],
                                    wa->wa_offset, wa->wa_totcount, (int)wip);
#endif
                                return (TRUE);
                        } else {
                                wip->wi_done = 1;
                        }
                        break;

                case XDR_DECODE:


                        /* server side */

                        if (xdrmbuf_getvec(xdrs, wa->wa_iov,
                            (u_int *)&wa->wa_iovcnt, (u_int)NFS_MAXIOVEC,
                            (u_int *)&wa->wa_count, (u_int)NFS_MAXDATA) ) {
#ifdef NFSDEBUG
                                dprint(nfsdebug, 6,
                                    "xdr_writeargs: %s off %d cnt %d iov %d\n",
                                    xdropnames[(int)xdrs->x_op],
                                    wa->wa_offset, wa->wa_totcount,
                                    wa->wa_iovcnt);
#endif
                                return (TRUE);
                        }
                        if (wa->wa_iovcnt >= NFS_MAXIOVEC) {
                                /* too many fragments - copy all into one buf */
                                if ((data = wa->wa_data) == NULL) {
                                        data = kmem_alloc(wa->wa_count);
                                        wa->wa_data = data;
                                }
                                wa->wa_iov[0].iov_base = data;
                                wa->wa_iov[0].iov_len = count = wa->wa_count;
                                wa->wa_iovcnt = 1;
                                if (xdr_opaque(xdrs, data, count)) {
#ifdef NFSDEBUG
                                        dprint(nfsdebug, 6,
                                     "xdr_writeargs: %s off %d cnt %d buf %d\n",
                                            xdropnames[(int)xdrs->x_op],
                                            wa->wa_offset, wa->wa_totcount,
                                            (int)data);
#endif
                                        return (TRUE);
                                }
                        }

                    break;

                case  XDR_FREE:

                        /* server side */
                        if (data = wa->wa_data) {
                                kmem_free(data, wa->wa_count);
                                wa->wa_data = 0;
                        }
#ifdef NFSDEBUG
                        dprint(nfsdebug, 6,
                            "xdr_writeargs: %s off %d cnt %d buf %x\n",
                            xdropnames[(int)xdrs->x_op],
                            wa->wa_offset, wa->wa_totcount, (int)data);
#endif
                        return (TRUE);
                      break;

                 default:
                       return(FALSE);
                }
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_writeargs: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);

}


/* BEGIN_IMS xdr_fattr *
 ********************************************************************
 ****
 ****		xdr_fattr(xdrs, na)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	na		pointer to the file attributes structure
 *
 * Output Parameters
 *	na->		may change depending upon the value of xdr->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the file attributes structure which is
 *	passed as a parameter across the network between NFS clients and
 *	servers. This represents the handling of the NFS_OK portion of
 *	the request for file attributes. Put/Get the file attribute structure
 *	on/from the xdr stream. If xdrs->x_op is XDR_FREE then call the 
 *	appropriate routines to free the area.
 *
 * Algorithm
 *	{ if (xdrs->x_op == XDR_ENCODE) {
 *		get appropriate number of bytes in line on the stream
 *		if (ok) {
 *			put each attribute on the stream
 *			return(TRUE)
 *			}
 *	  } else {
 *		get appropriate number of bytes in line on the stream
 *		if (ok) {
 *			get each attribute from the stream
 *			return(TRUE)
 *			}
 *	  }
 *	  if (operations on the file attributes)
 *		return(TRUE)
 *	  return(FALSE)
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. there should be a check before the GETs for XDR_DECODE
 *	. why handle the different case seperately? Why not do a single
 *	  xdr_enum(),xdr_long() etc.
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	XDR_INLINE
 *	IXDR_PUT_ENUM
 *	IXDR_PUT_LONG
 *	IXDR_GET_ENUM
 *	IXDR_GET_LONG
 *	xdr_enum
 *	xdr_u_long
 *	xdr_timeval
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_fattr */
bool_t
xdr_fattr(xdrs, na)
	XDR *xdrs;
	register struct nfsfattr *na;
{
	register long *ptr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_fattr: %s\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	if (xdrs->x_op == XDR_ENCODE) {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			IXDR_PUT_ENUM(ptr, na->na_type);
			IXDR_PUT_LONG(ptr, na->na_mode);
			IXDR_PUT_LONG(ptr, na->na_nlink);
			IXDR_PUT_LONG(ptr, na->na_uid);
			IXDR_PUT_LONG(ptr, na->na_gid);
			IXDR_PUT_LONG(ptr, na->na_size);
			IXDR_PUT_LONG(ptr, na->na_blocksize);
			IXDR_PUT_LONG(ptr, na->na_rdev);
			IXDR_PUT_LONG(ptr, na->na_blocks);
			IXDR_PUT_LONG(ptr, na->na_fsid);
			IXDR_PUT_LONG(ptr, na->na_nodeid);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_atime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_mtime.tv_usec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_sec);
			IXDR_PUT_LONG(ptr, na->na_ctime.tv_usec);
			return (TRUE);
		}
	} else {
		ptr = XDR_INLINE(xdrs, 17 * BYTES_PER_XDR_UNIT);
		if (ptr != NULL) {
			na->na_type = IXDR_GET_ENUM(ptr, enum nfsftype);
			na->na_mode = IXDR_GET_LONG(ptr);
			na->na_nlink = IXDR_GET_LONG(ptr);
			na->na_uid = IXDR_GET_LONG(ptr);
			na->na_gid = IXDR_GET_LONG(ptr);
			na->na_size = IXDR_GET_LONG(ptr);
			na->na_blocksize = IXDR_GET_LONG(ptr);
			na->na_rdev = IXDR_GET_LONG(ptr);
			na->na_blocks = IXDR_GET_LONG(ptr);
			na->na_fsid = IXDR_GET_LONG(ptr);
			na->na_nodeid = IXDR_GET_LONG(ptr);
			na->na_atime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_atime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_mtime.tv_usec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_sec = IXDR_GET_LONG(ptr);
			na->na_ctime.tv_usec = IXDR_GET_LONG(ptr);
			return (TRUE);
		}
	}
	if (xdr_enum(xdrs, (enum_t *)&na->na_type) &&
	    xdr_u_long(xdrs, &na->na_mode) &&
	    xdr_u_long(xdrs, &na->na_nlink) &&
	    xdr_u_long(xdrs, &na->na_uid) &&
	    xdr_u_long(xdrs, &na->na_gid) &&
	    xdr_u_long(xdrs, &na->na_size) &&
	    xdr_u_long(xdrs, &na->na_blocksize) &&
	    xdr_u_long(xdrs, &na->na_rdev) &&
	    xdr_u_long(xdrs, &na->na_blocks) &&
	    xdr_u_long(xdrs, &na->na_fsid) &&
	    xdr_u_long(xdrs, &na->na_nodeid) &&
	    xdr_timeval(xdrs, &na->na_atime) &&
	    xdr_timeval(xdrs, &na->na_mtime) &&
	    xdr_timeval(xdrs, &na->na_ctime) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_fattr: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_readargs *
 ********************************************************************
 ****
 ****		xdr_readargs(xdrs, ra)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	ra		points to the nfsreadargs structure
 *
 * Output Parameters
 *	ra->		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfsreadargs structure that 
 *	represents the arguments for the remote read and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers.
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the file handle
 *	       xdr_long to handle the offset
 *	       xdr_long to handle the read count
 *	       xdr_long to handle the total read count so far
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_fhandle
 *	xdr_long
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_readargs */
bool_t
xdr_readargs(xdrs, ra)
	XDR *xdrs;
	struct nfsreadargs *ra;
{

	if (xdr_fhandle(xdrs, &ra->ra_fhandle) &&
	    xdr_long(xdrs, (long *)&ra->ra_offset) &&
	    xdr_long(xdrs, (long *)&ra->ra_count) &&
	    xdr_long(xdrs, (long *)&ra->ra_totcount) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_readargs: %s off %d ct %d\n",
		    xdropnames[(int)xdrs->x_op],
		    ra->ra_offset, ra->ra_totcount);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_raedargs: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Info necessary to free the bp which is also an mbuf
 */
struct rrokinfo {
	int	(*func)();
	int	done;
	struct vnode *vp;
	struct buf *bp;
};
	

/* BEGIN_IMS rrokfree *
 ********************************************************************
 ****
 ****		rrokfree(rip)
 ****
 ********************************************************************
 * Input Parameters
 *	rip		pointer to rrokinfo structure
 *
 * Output Parameters
 *	none
 *
 * Return Value
 *	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	This routine is called to release certain data structures after
 *	the data in an mbuf chain associated with an NFS read reply has
 *	been sent. The information necessary to release these structures
 *	is kept in an rrokinfo structure (rip), which is stored in an mbuf.
 *	In xdr_rrok, an buffer is allocated to hold the rrokinfo structure:
 *
 *	struct rrokinfo {
 *		int (*func)();		< the rrokfree routine >
 *		int done;		< whether the data has been sent >
 *		struct vnode *vp;	< the vnode of the file being read >
 *		struct buf *bp; 	< the buffer containing the read data >
 *	}
 *
 *	The rrokinfo structure is then initialized and a pointer to it
 *	is put in the x_public field of the XDR structure associated with
 *	the read reply. In svckudp_send, after ku_sendto_mbuf has been
 *	called to send an mbuf chain, a check is made to see if 
 *	xdrs->x_public is not null. If it is not null, then it points to 
 *	an rrokinfo structure whose first field is the routine to call
 *	to release the necessary data structures (rrokfree). In this case,
 *	rrokfree is called from svckudp_send. rrokfree sleeps until the
 *	the mbuf chain has been successfully written (more specifically
 *	until the data in the MF_LARGEBUF mbuf holding the data for the
 *	NFS read reply has been written and the mbuf is being freed).
 *	This event is signaled by the routine rrokwakeup, which is called
 *	when the mbuf is freed.
 *
 * Algorithm
 *	while (not done sending mbuf chain)
 *		sleep(rrokinfo structure)
 *	release the file system buffer 
 *	release the vnode
 *	free the buffer containing rrokinfo structure
 *
 * Concurrency
 *
 * To Do List
 *
 * Notes
 *	See the rrokwakeup routine.
 *
 * Modification History
 *    RB from IND  11/27  ifdef MP to protect from MP lan driver. (actually cwb)
 *
 * External Calls
 *	splimp
 *	sleep
 *	splx
 *	VOP_BRELSE
 *	VN_RELE
 *	MFREE
 *
 * Called By 
 *	svckudp_send (via xdrs->x_public)
 *
 ********************************************************************
 * END_IMS rrokfree */
static
rrokfree(rip)
	struct rrokinfo *rip;
{
#ifdef MP
	int old;
#else
	int s;
#endif /* MP */
	struct mbuf *n;

#ifdef MP
        old = sleep_lock();
        while (rip->done == 0) {
                sleep_then_unlock((caddr_t)rip, PZERO-1, old);
                if (rip->done != 0) { goto loop_done; }
                old = sleep_lock();
        }
        sleep_unlock(old);
loop_done:
#else /* not MP*/
	s = splimp();
	while (rip->done == 0) {
		sleep((caddr_t)rip, PZERO-1);
	}
	(void) splx(s);
#endif /* MP */

	VOP_BRELSE(rip->vp, rip->bp);
	VN_RELE(rip->vp);
        /* Change MFREE to kmem_free
         * MFREE(dtom(rip), n);
         */

         kmem_free(rip,sizeof(struct rrokinfo));
#ifdef lint
	n = n;
#endif lint
}


/* BEGIN_IMS rrokwakeup *
 ********************************************************************
 ****
 ****		rrokwakeup(rip)
 ****
 ********************************************************************
 * Input Parameters
 *	rip		pointer to rrokinfo structure
 *
 * Output Parameters
 *	none
 *
 * Return Value
 * 	none
 *
 * Globals Referenced
 *	none
 *
 * Description
 * 	NFS uses a special large buffer (MF_LARGEBUF) mbuf for pointing
 *	to data stored in file system buffers. In this type of mbuf, the
 *	"data" field of the mbuf is used to hold the name of a function
 *	to call to free the mbuf (mun_clfun) and an argument to pass to
 *	this function (mun_clarg). In some cases, this function is "buffree",
 *	in others it is "rrokwakeup". xdr_rrok calls xdrmbuf_putbuf with
 *	"rrokwakeup" specified as the function to use to free the LARGEBUF
 *	mbuf that will be allocated with mclgetx. This mbuf allocated by
 *	xdrmbuf_putbuf holds the NFS read reply data stored in a file system
 *	buffer. This avoids the overhead of data copy into regular mbufs or
 *	mbuf clusters. When the mbuf is finally freed by the driver, the
 *	driver (or netisr) calls "rrokwakeup" through M_CLFUN as part of the
 *	process of freeing the mbuf. rrokwakeup then sets to true the done
 *	flag of the rrokinfo structure it is passed as an argument and 
 *	wakes up the process (which had called rrokfree) sleeping on this
 *	structure. Through rrokfree, the buffer associated with the LARGEBUF
 *	mbuf is finally freed.
 *
 * Algorithm
 *	set done flag in rrokinfo structure
 *	wakeup process sleeping on rrokinfo structure
 *
 * Concurrency
 *
 * To Do List
 *
 * Notes
 *	See the rrokfree routine.
 *
 * Modification History
 *    RB from IND  11/27  ifdef MP to protect from MP lan driver (actually cwb)
 *
 * External Calls
 *	wakeup
 *
 * Called By 
 *	M_CLFUN (via m->m_clfun field of MF_LARGEBUF type mbuf)
 *
 ********************************************************************
 * END_IMS rrokwakeup */
static
rrokwakeup(rip)
	struct rrokinfo *rip;
{
#ifdef MP
        int old;

        old= sleep_lock();
        rip->done = 1;
        wakeup((caddr_t)rip);
        sleep_unlock(old);
#else /* not MP */
	rip->done = 1;
	wakeup((caddr_t)rip);
#endif
}


/* BEGIN_IMS xdr_rrok *
 ********************************************************************
 ****
 ****		xdr_rrok(xdrs, rrok)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	rrok		pointer to nfsrrok structure
 *
 * Output Parameters
 *	*rrok		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This serializes/deserializes the nfsrrok structure that 
 *	represents the NFS_OK portion of remote read reply which
 *	is passed as a parameter across the network between NFS 
 *	clients and servers.
 *
 * Algorithm
 *
 * Concurrency
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	11/19/87	lmb		added trigger
 *	11/25/87	lmb		added code to free buffer, vnode 
 *
 * External Calls
 *	xdr_fattr
 *	MGET
 *	NS_LOG
 *	mtod
 *	xdrmbuf_putbuf
 *	dprint
 *	xdr_bytes
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_rrok */
bool_t
xdr_rrok(xdrs, rrok)
	XDR *xdrs;
	struct nfsrrok *rrok;
{

	if (xdr_fattr(xdrs, &rrok->rrok_attr)) {
		if (xdrs->x_op == XDR_ENCODE && rrok->rrok_bp) {
			/* server side */
			struct rrokinfo *rip;

                        /* kmem_alloc will not return until memory is
                         * alloc
                         */

                        rip = (struct rrokinfo *)
                                  kmem_alloc(sizeof(struct rrokinfo));

			rip->func = rrokfree;
			rip->done = 0;
			rip->vp = rrok->rrok_vp;
			rip->bp = rrok->rrok_bp;
			xdrs->x_public = (caddr_t)rip;
			if (xdrmbuf_putbuf(xdrs, rrok->rrok_data,
			    (u_int)rrok->rrok_count, rrokwakeup, (int)rip)) {
#ifdef NFSDEBUG
				dprint(nfsdebug, 6, "xdr_rrok: %s %d addr %x\n",
				    xdropnames[(int)xdrs->x_op],
				    rrok->rrok_count, rrok->rrok_data);
#endif
				return (TRUE);
			} else {
				rip->done = 1;
			}
		} else {			/* client side */
			if (xdr_bytes(xdrs, &rrok->rrok_data,
			    (u_int *)&rrok->rrok_count, NFS_MAXDATA) ) {
#ifdef NFSDEBUG
				dprint(nfsdebug, 6, "xdr_rrok: %s %d addr %x\n",
				    xdropnames[(int)xdrs->x_op],
				    rrok->rrok_count, rrok->rrok_data);
#endif
				return (TRUE);
			}
		}
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rrok: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim rdres_discrim[2] = {
	{ (int)NFS_OK, xdr_rrok },
	{ __dontcare__, NULL_xdrproc_t }
};


/* BEGIN_IMS xdr_rdresult *
 ********************************************************************
 ****
 ****		xdr_rdresult(xdrs, rr)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	rr		pointer to nfsrdresult structure
 *
 * Output Parameters
 *	*rr		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This serializes/deserializes the nfsrdresult structure that 
 *	represents the reply status from the remote read and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers. A call is made to xdr_union to handle the different
 *	cases possible. However, note that at the moment the only 
 *	possible option is that of NFS_OK, as represented by the
 *	nfsrrok structure.
 *
 * Algorithm
 *	{ call xdr_union passing as parameters the read status (discriminant),
 *		the nfsrrok structure, the array of xdrdiscrims, and the
 *		default xdr_routine (in this case xdr_void)
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. in the call to xdr_union the nfsrrok structure is passed, instead
 *	  of the union itself. It doesn't seem to matter in this case,
 *	  but....
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_union
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_rdresult */
bool_t
xdr_rdresult(xdrs, rr)
	XDR *xdrs;
	struct nfsrdresult *rr;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_rdresult: %s\n", xdropnames[(int)xdrs->x_op]);
#endif
	if (xdr_union(xdrs, (enum_t *)&(rr->rr_status),
	      (caddr_t)&(rr->rr_ok), rdres_discrim, xdr_void) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rdresult: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_sattr *
 ********************************************************************
 ****
 ****		xdr_sattr(xdrs, sa)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	sa		pointer to nfssattr structure
 *
 * Output Parameters
 *	*sa		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This serializes/deserializes the nfssattr structure that 
 *	represents the file attributes that can be set and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers.
 *
 * Algorithm
 *	{ call xdr_u_long to handle the protection mode bits
 *	       xdr_u_long to handle the owner user id
 *	       xdr_u_long to handle the owner group id
 *	       xdr_u_long to handle the file size in bytes
 *	       xdr_timeval to handle the last access time
 *	       xdr_timeval to handle the last modification time
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_u_long
 *	xdr_timeval
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_sattr */
bool_t
xdr_sattr(xdrs, sa)
	XDR *xdrs;
	struct nfssattr *sa;
{

	if (xdr_u_long(xdrs, &sa->sa_mode) &&
	    xdr_u_long(xdrs, &sa->sa_uid) &&
	    xdr_u_long(xdrs, &sa->sa_gid) &&
	    xdr_u_long(xdrs, &sa->sa_size) &&
	    xdr_timeval(xdrs, &sa->sa_atime) &&
	    xdr_timeval(xdrs, &sa->sa_mtime) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6,
		    "xdr_sattr: %s mode %o uid %d gid %d size %d\n",
		    xdropnames[(int)xdrs->x_op], sa->sa_mode, sa->sa_uid,
		    sa->sa_gid, sa->sa_size);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_sattr: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim attrstat_discrim[2] = {
	{ (int)NFS_OK, xdr_fattr },
	{ __dontcare__, NULL_xdrproc_t }
};


/* BEGIN_IMS xdr_attrstat *
 ********************************************************************
 ****
 ****		xdr_attrstat(xdrs, ns)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	ns		pointer to nfsattrstat structure
 *
 * Output Parameters
 *	*ns		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfsattrstat structure that 
 *	represents the reply status with file attributes and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers. A call is made to xdr_union to handle the different
 *	cases possible. However, note that at the moment the only 
 *	possible option is that of NFS_OK, as represented by the
 *	nfsfattr structure.
 *
 * Algorithm
 *	{ call xdr_union passing as parameters the reply status (discriminant),
 *		the nfsfattr structure, the array of xdrdiscrims, and the
 *		default xdr_routine (in this case xdr_void)
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. in the call to xdr_union the nfsfattr structure is passed, instead
 *	  of the union itself. It doesn't seem to matter in this case,
 *	  but....
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_union
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_attrstat */
bool_t
xdr_attrstat(xdrs, ns)
	XDR *xdrs;
	struct nfsattrstat *ns;
{

	if (xdr_union(xdrs, (enum_t *)&(ns->ns_status),
	      (caddr_t)&(ns->ns_attr), attrstat_discrim, xdr_void) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_attrstat: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], ns->ns_status);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_attrstat: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_srok *
 ********************************************************************
 ****
 ****		xdr_srok(xdrs, srok)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	srok		pointer to nfssrok structure
 *
 * Output Parameters
 *	*srok		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	NFS_MAXPATHLEN
 *
 * Description
 *	This serializes/deserializes the nfssrok structure that 
 *	represents the NFS_OK part of read sym link reply union and
 *	which is passed as a parameter across the network between 
 *	NFS clients and servers.
 *
 * Algorithm
 *	{ call xdr_bytes to handle the string passing as parameters
 *		the string length and the string containing the
 *		symbolic link
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_bytes
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_srok */
bool_t
xdr_srok(xdrs, srok)
	XDR *xdrs;
	struct nfssrok *srok;
{

	if (xdr_bytes(xdrs, &srok->srok_data, (u_int *)&srok->srok_count,
	    NFS_MAXPATHLEN) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_srok: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

struct xdr_discrim rdlnres_discrim[2] = {
	{ (int)NFS_OK, xdr_srok },
	{ __dontcare__, NULL_xdrproc_t }
};


/* BEGIN_IMS xdr_rdlnres *
 ********************************************************************
 ****
 ****		xdr_rdlnres(xdrs, ns)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	ns		pointer to nfsrdlnres structure
 *
 * Output Parameters
 *	*ns		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfsrdlnres structure that
 *	represents the result of reading a symbolic link and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers. A call is made to xdr_union to handle the different
 *	cases possible. However, note that at the moment the only 
 *	possible option is that of NFS_OK, as represented by the
 *	nfssrok structure.
 *
 * Algorithm
 *	{ call xdr_union passing as parameters the reply status (discriminant),
 *		the nfssrok structure, the array of xdrdiscrims, and the
 *		default xdr_routine (in this case xdr_void)
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. in the call to xdr_union the nfssrok structure is passed, instead
 *	  of the union itself. It doesn't seem to matter in this case,
 *	  but....
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_union
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_rdlnres */
bool_t
xdr_rdlnres(xdrs, rl)
	XDR *xdrs;
	struct nfsrdlnres *rl;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_rdlnres: %s\n", xdropnames[(int)xdrs->x_op]);
#endif
	if (xdr_union(xdrs, (enum_t *)&(rl->rl_status),
	      (caddr_t)&(rl->rl_srok), rdlnres_discrim, xdr_void) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rdlnres: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_rddirargs *
 ********************************************************************
 ****
 ****		xdr_rddirargs(xdrs, rda)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	rda		pointer to nfsrddirargs structure
 *
 * Output Parameters
 *	*rda		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This serializes/deserializes the nfsrddirargs structure that 
 *	represents the arguments to readdir and which is passed as a
 *	parameter across the network between NFS clients and servers.
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the directory handle
 *	       xdr_u_long to handle the offset in directory
 *	       xdr_u_long to handle the number of directory bytes to
 *		be read 
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_fhandle
 *	xdr_u_long
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_rddirargs */
bool_t
xdr_rddirargs(xdrs, rda)
	XDR *xdrs;
	struct nfsrddirargs *rda;
{

	if (xdr_fhandle(xdrs, &rda->rda_fh) &&
	    xdr_u_long(xdrs, &rda->rda_offset) &&
	    xdr_u_long(xdrs, &rda->rda_count) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6,
		    "xdr_rddirargs: %s fh %o %d, off %d, cnt %d\n",
		    xdropnames[(int)xdrs->x_op],
		    rda->rda_fh.fh_fsid, rda->rda_fh.fh_fno, rda->rda_offset,
		    rda->rda_count);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rddirargs: %s FAILED\n",
	    xdropnames[(int)xdrs->x_op]);
#endif
	return (FALSE);
}

/*
 * Directory read reply:
 * union (enum status) {
 *	NFS_OK: entlist;
 *		boolean eof;
 *	default:
 * }
 *
 * Directory entries
 *	struct  direct {
 *		u_long  d_fileno;	       * inode number of entry *
 *		u_short d_reclen;	       * length of this record *
 *		u_short d_namlen;	       * length of string in d_name *
 *		char    d_name[MAXNAMLEN + 1];  * name no longer than this *
 *	};
 * are on the wire as:
 * union entlist (boolean valid) {
 * 	TRUE: struct dirent;
 *	      u_long nxtoffset;
 * 	      union entlist;
 *	FALSE:
 * }
 * where dirent is:
 * 	struct dirent {
 *		u_long	de_fid;
 *		string	de_name<NFS_MAXNAMELEN>;
 *	}
 */

#define	nextdp(dp)	((struct direct *)((int)(dp) + (dp)->d_reclen))

/*
 * HPNFS
 * put define of DIRSIZ back when we went to long file names in
 * directory structure.
 * HPNFS
 */
#undef DIRSIZ
#define DIRSIZ(dp)	(sizeof(struct direct) - MAXNAMLEN + (dp)->d_namlen)


/* BEGIN_IMS xdr_putrddirres *
 ********************************************************************
 ****
 ****		xdr_putrddirres(xdrs, rd)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	rd		pointer to nfsrddirres structure
 *
 * Output Parameters
 *	*rd		some fields may change depending upon the value
 *			of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	For some unknown reason, we depart from the normal convention of
 *	having the same routine do serialization/deserialization and
 *	instead have xdr_putrddirres do only the encoding of the read 
 *	directory results onto the wire. Another wierd mechanism utilised
 *	is the method of putting the actual structure on the wire. Instead
 *	of putting each field as it is, we put things in a different 
 *	order, and we do not put all the fields on the wire.
 *	The actual structure is a follows:
 *
 *    	struct nfsrddirres {
 *		enum nfsstat rd_status;
 *		u_long	     rd_bufsize; <<size of client request (not xdr'ed)>>
 *		union (rd_status) {
 *			NFS_OK:	struct nfsrdok rd_rdok_u;
 *			default:
 *		} rd_u;
 *	};
 *
 *	struct nfsrdok {
 *		u_long	rdok_offset;	<< next offset (opaque) >>
 *		u_long	rdok_size;	<< size in bytes of entries >>
 *		bool_t	rdok_eof;	<< true if last entry is in result >>
 *		struct direct *rdok_entries;	<< variable number of entries >>
 *	};
 *
 *	struct  direct {
 *		u_long  d_fileno;	        << inode number of entry >>
 *		u_short d_reclen;	        << length of this record >>
 *		u_short d_namlen;	        << length of string in d_name >>
 *		char    d_name[MAXNAMLEN + 1];  << name no longer than this >>
 *	};
 *
 *	This is put on the wire as follows:
 *
 *	rd_status
 *	union entlist (rd_status) {
 *		TRUE: struct dirent;
 *      	      u_long nxtoffset;
 *	              union entlist;
 *		FALSE:
 *	}
 * 
 * 	where dirent is:
 *	
 *	struct dirent {
 * 		u_long	d_fileno;
 * 		string	de_name(NFS_MAXNAMELEN)	< includes d_namelen >
 *	}
 *
 * Algorithm
 *	<< rd_offset 	- tells the current offset at the beginning of the 
 *		   	  direct entries. We calculate the new value  by 
 *			  adding the d_reclen values of each entry. The 
 *			  final value is picked up and used on the client 
 *			  side.
 *	   rd_size 	- this gives us the total size of all the direct
 *			  entries. Used to tell us if any are still left
 *			  to be put on the stream.
 *	   rd_bufsize	- this gives the size of the client request. If
 *			  we put more bytes on to the stream than requested
 *			  by the client then rd_eof is set to FALSE, since
 *			  the end has not been reached.
 *	   rd_eof	- indicates (when FALSE) that there were more bytes
 *	   		  that might have been sent if rd_bufsize had been
 *			  larger.
 *	>>
 *
 *	{	if (xdrs->x_op != XDR_ENCODE)
 *			return(FALSE)
 *		put rd_status on the xdr_stream
 *		if (rd_status != NFS_OK)
 *			return(TRUE)  << since read_dir failed nothing else
 *					 has to be put on the stream >>
 *		calculate current position on the xdr stream
 *		point to the current direct entry
 *		obtain size from rd_size and
 *		       offset from rd_offset
 *		while ( there still remains some stuff to put on
 *			the xdr stream (as determined by size > 0) ) {
 *			make sure value of d_reclen is ok
 *			calculate new offset
 *			put direct entry onto the xdr stream in the above
 *				mentioned format << see description >>
 *			put offset onto the stream
 *			if the put operations fail
 *				return(FALSE)
 *			if ((current position on stream - original postion)
 *				> rd_bufsize) {
 *				set rd_eof to FALSE
 *				jump out of while loop
 *			}
 *			calculate (size)number of bytes left
 *			calculate position of new direct entry
 *		  }
 *		  put FALSE on the stream to indicate that no more direct
 *			entries are left
 *		  put rd_eof value on the stream
 *		  if these two operations succeed
 *			return(TRUE)
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. there should be only one procedure that does both serialization/
 *	  and deserialization of the directory structure.
 *	. condition should be (DIRSIZ(dp) != dp->d_reclen) ?
 *	. there is no need to put the offset on each time with the direct
 *	  entry. This may be done only once, at the very end, along with
 *	  rd_eof. Changes will also have to be made in xdr_getrddirres().
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_enum
 *	XDR_GETPOS
 *	dprint
 *	xdr_bool
 *	xdr_u_long
 *	xdr_bytes
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_putrddirres */
bool_t
xdr_putrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsrddirres *rd;
{
	struct direct *dp;
	char *name;
	int size;
	int xdrpos;
	u_int namlen;
	u_long offset;
	bool_t true = TRUE;
	bool_t false = FALSE;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_putrddirres: %s size %d offset %d\n",
	    xdropnames[(int)xdrs->x_op], rd->rd_size, rd->rd_offset);
#endif
	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}
	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

	xdrpos = XDR_GETPOS(xdrs);
	for (offset = rd->rd_offset, size = rd->rd_size, dp = rd->rd_entries;
	     size > 0;
	     size -= dp->d_reclen, dp = nextdp(dp) ) {
		if (dp->d_reclen == 0 || DIRSIZ(dp) > dp->d_reclen) {
#ifdef NFSDEBUG
			dprint(nfsdebug, 2, "xdr_putrddirres: bad directory\n");
#endif
			return (FALSE);
		}
		offset += dp->d_reclen;
#ifdef NFSDEBUG
		dprint(nfsdebug, 10,
		    "xdr_putrddirres: entry %d %s(%d) %d %d %d %d\n",
		    dp->d_fileno, dp->d_name, dp->d_namlen, offset,
		    dp->d_reclen, XDR_GETPOS(xdrs), size);
#endif
		if (dp->d_fileno == 0) {
			continue;
		}
		name = dp->d_name;
		namlen = dp->d_namlen;
		if (!xdr_bool(xdrs, &true) ||
		    !xdr_u_long(xdrs, &dp->d_fileno) ||
		    !xdr_bytes(xdrs, &name, &namlen, NFS_MAXNAMLEN) ||
		    !xdr_u_long(xdrs, &offset) ) {
			return (FALSE);
		}
		if (XDR_GETPOS(xdrs) - xdrpos >= rd->rd_bufsize) {
			rd->rd_eof = FALSE;
			break;
		}
	}
	if (!xdr_bool(xdrs, &false)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	return (TRUE);
}

#define	roundtoint(x)	(((x) + (sizeof(int) - 1)) & ~(sizeof(int) - 1))
#define	reclen(dp)	roundtoint(((dp)->d_namlen + 1 + sizeof(u_long) +\
				2 * sizeof(u_short)))


/* BEGIN_IMS xdr_getrddirres *
 ********************************************************************
 ****
 ****		xdr_getrddirres(xdrs, rd)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	rd		pointer to nfsrddirres structure
 *
 * Output Parameters
 *	rd->		values are taken out of the xdr stream
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	For some unknown reason, we depart from the normal convention of
 *	having the same routine do serialization/deserialization and
 *	instead have xdr_getrddirres do only the decoding of the read 
 *	directory results from the wire. We get the fields off the stream
 *	in the same sequence as they were put on the stream. See the 
 *	xdr_putrddirres routine for the actual format and the original
 *	structure. The rd_bufsize field is not altered. During encoding,
 *	this reflected the size of the client request. rd_size contains
 *	of the total size of the direct entries that are to be on the wire.
 *	So we keep decoding till size becomes lte to zero (an error) or
 *	we get a FALSE value indicating the end of the direct entries.
 *
 * Algorithm
 *	{ get rd_status off the stream
 *	  if (rd_status != NFS_OK)	<< read dir failed on server >>
 *	  	return(TRUE)
 *	  get size from rd_size		<< maximum size of direct entries >>
 *	  do forever {
 *		read bool value from the stream
 *		if ( value == TRUE ) {	<< direct entry follows >>
 *			get direct entry from the stream
 *			get offset 
 *			calculate remaining direct entries
 *		}
 *		else			<< no more direct entries >>
 *			jump out of while loop
 *		calculate new value of size
 *		if ( size <= 0 )	<< error >>
 *			return(FALSE)
 *	  }
 *	  get rd_eof from the stream
 *	  calculate actual size of the read dir result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. there should be only one procedure that does both serialization/
 *	  and deserialization of the directory structure.
 *	. offset is read from the stream each time a direct entry is read,
 *	  but only the last one is used. So why not put it on and read it
 *	  off the stream only once, at the very end ? Changes will also have
 *	  to be made in xdr_putrddirres().
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_bool
 *	xdr_enum
 *	dprint
 *	xdr_u_long
 *	xdr_u_short
 *	xdr_opaque
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_getrddirres */
bool_t
xdr_getrddirres(xdrs, rd)
	XDR *xdrs;
	struct nfsrddirres *rd;
{
	struct direct *dp;
	int size;
	bool_t valid;
	u_long offset = (u_long)-1;

	if (!xdr_enum(xdrs, (enum_t *)&rd->rd_status)) {
		return (FALSE);
	}
	if (rd->rd_status != NFS_OK) {
		return (TRUE);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "xdr_getrddirres: %s size %d\n",
	    xdropnames[(int)xdrs->x_op], rd->rd_size);
#endif
	size = rd->rd_size;
	dp = rd->rd_entries;
	for (;;) {
		if (!xdr_bool(xdrs, &valid)) {
			return (FALSE);
		}
		if (valid) {
			if (!xdr_u_long(xdrs, &dp->d_fileno) ||
			    !xdr_u_short(xdrs, &dp->d_namlen) ||
			    (reclen(dp) > size) ||
			    !xdr_opaque(xdrs, dp->d_name, (u_int)dp->d_namlen)||
			    !xdr_u_long(xdrs, &offset) ) {
#ifdef NFSDEBUG
				dprint(nfsdebug, 2,
				    "xdr_getrddirres: entry error\n");
#endif
				return (FALSE);
			}
			dp->d_reclen = reclen(dp);
			dp->d_name[dp->d_namlen] = '\0';
#ifdef NFSDEBUG
			dprint(nfsdebug, 10,
			    "xdr_getrddirres: entry %d %s(%d) %d %d\n",
			    dp->d_fileno, dp->d_name, dp->d_namlen,
			    dp->d_reclen, offset);
#endif
		} else {
			break;
		}
		size -= dp->d_reclen;
		if (size <= 0) {
			return (FALSE);
		}
		dp = nextdp(dp);
	}
	if (!xdr_bool(xdrs, &rd->rd_eof)) {
		return (FALSE);
	}
	rd->rd_size = (int)dp - (int)(rd->rd_entries);
	rd->rd_offset = offset;
#ifdef NFSDEBUG
	dprint(nfsdebug, 6,
	    "xdr_getrddirres: returning size %d offset %d eof %d\n",
	    rd->rd_size, rd->rd_offset, rd->rd_eof);
#endif
	return (TRUE);
}


/* BEGIN_IMS xdr_diropargs *
 ********************************************************************
 ****
 ****		xdr_diropargs(xdrs, da)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	da		pointer to nfsdiropargs structure
 *
 * Output Parameters
 *	*da		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	NFS_MAXNAMELEN
 *
 * Description
 *	This serializes/deserializes the nfsdiropargs structure that 
 *	represents the arguments for directory operations which is
 *	passed as a parameter across the network between NFS clients 
 *	and servers. To do this, it calls routines to handle the
 *	directory file handle and the name.
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the directory file handle
 *	       xdr_string to handle the name
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_string
 *	xdr_fhandle
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_diropargs */
bool_t
xdr_diropargs(xdrs, da)
	XDR *xdrs;
	struct nfsdiropargs *da;
{

	if (xdr_fhandle(xdrs, &da->da_fhandle) &&
	    xdr_string(xdrs, &da->da_name, NFS_MAXNAMLEN) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_diropargs: %s %o %d '%s'\n",
		    xdropnames[(int)xdrs->x_op], da->da_fhandle.fh_fsid,
		    da->da_fhandle.fh_fno, da->da_name);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_diropargs: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_drok *
 ********************************************************************
 ****
 ****		xdr_drok(xdrs, drok)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	drok		pointer to nfsdrok structure
 *
 * Output Parameters
 *	*drok		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This serializes/deserializes the nfsdrok structure that 
 *	represents the NFS_OK part of the directory operation result
 *	which is passed as a parameter across the network between 
 *	NFS clients and servers. To do this, it calls routines to 
 *	handle the result file handle and the result file attributes.
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the result file handle
 *	       xdr_fattr to handle the result file attributes
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_fattr
 *	xdr_fhandle
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_drok */
bool_t
xdr_drok(xdrs, drok)
	XDR *xdrs;
	struct nfsdrok *drok;
{

	if (xdr_fhandle(xdrs, &drok->drok_fhandle) &&
	    xdr_fattr(xdrs, &drok->drok_attr) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_drok: FAILED\n");
#endif
	return (FALSE);
}

struct xdr_discrim diropres_discrim[2] = {
	{ (int)NFS_OK, xdr_drok },
	{ __dontcare__, NULL_xdrproc_t }
};


/* BEGIN_IMS xdr_diropres *
 ********************************************************************
 ****
 ****		xdr_diropres(xdrs, dr)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	dr		pointer to the nfsdiropres structure
 *
 * Output Parameters
 *	*dr		may changes depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *
 * Description
 *	This serializes/deserializes the nfsdiropres structure that
 *	represents the result of a directory operation and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers. A call is made to xdr_union to handle the different
 *	cases possible. However, note that at the moment the only 
 *	possible option is that of NFS_OK, as represented by the
 *	nfsdrok structure.
 *
 * Algorithm
 *	{ call xdr_union passing as parameters the reply status (discriminant),
 *		the nfsdrok structure, the array of xdrdiscrims, and the
 *		default xdr_routine (in this case xdr_void)
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. in the call to xdr_union the nfsdrok structure is passed, instead
 *	  of the union itself. It doesn't seem to matter in this case,
 *	  but....
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_union
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_diropres */
bool_t
xdr_diropres(xdrs, dr)
	XDR *xdrs;
	struct nfsdiropres *dr;
{

	if (xdr_union(xdrs, (enum_t *)&(dr->dr_status),
	      (caddr_t)&(dr->dr_drok), diropres_discrim, xdr_void) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_diropres: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], dr->dr_status);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_diropres: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_timeval *
 ********************************************************************
 ****
 ****		xdr_timeval(xdrs, tv)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	tv		pointer to the timeval structure
 *
 * Output Parameters
 *	*tv		may changes depeding upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the timeval structure that 
 *	is part of the file attributes structure, which is
 *	passed as a parameter across the network between NFS clients 
 *	and servers. 
 *
 * Algorithm
 *	{ call xdr_long to handle the seconds
 *	       xdr_long to handle the microseconds
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_long
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_timeval */
bool_t
xdr_timeval(xdrs, tv)
	XDR *xdrs;
	struct timeval *tv;
{

	if (xdr_long(xdrs, (long *)&tv->tv_sec) &&
	    xdr_long(xdrs, (long *)&tv->tv_usec) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_timeval: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_saargs *
 ********************************************************************
 ****
 ****		xdr_saargs(xdrs, argp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	argp		pointer to the nfssaargs structure
 *
 * Output Parameters
 *	*argp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfssaargs structure that 
 *	represents the file handle of the file to be set and the 
 *	new attributes, which is passed as a parameter across the 
 *	network between NFS clients and servers. To do this, it
 *	calls routines to handle the file handle and the new attributes.
 *
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the file handle of the file to be set
 *	       xdr_sattr to handle the file attributes which can be set
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_fhandle
 *	xdr_sattr
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_saargs */
bool_t
xdr_saargs(xdrs, argp)
	XDR *xdrs;
	struct nfssaargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->saa_fh) &&
	    xdr_sattr(xdrs, &argp->saa_sa) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_saargs: %s %o %d\n",
		    xdropnames[(int)xdrs->x_op], argp->saa_fh.fh_fsid,
		    argp->saa_fh.fh_fno);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_saargs: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_creatargs *
 ********************************************************************
 ****
 ****		xdr_creatargs(xdrs, argp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	argp		pointer to the nfscreatargs
 *
 * Output Parameters
 *	*argp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfscreatargs structure that 
 *	represents the the filename to create and parent directory and
 *	the initial attributes which is passed as a parameter across 
 *	the network between NFS clients and servers. To do this, it
 *	calls routines to handle the nfsdiropargs and nfssattr structures.
 *
 * Algorithm
 *	{ call xdr_diropargs to handle the directory file handle
 *	       xdr_sattr to handle the initial attributes
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_diropargs
 *	xdr_sattr
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_creatargs */
bool_t
xdr_creatargs(xdrs, argp)
	XDR *xdrs;
	struct nfscreatargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->ca_da) &&
	    xdr_sattr(xdrs, &argp->ca_sa) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_creatargs: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_linkargs *
 ********************************************************************
 ****
 ****		xdr_linkargs(xdrs, argp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	argp		pointer to the nfslinkargs structure
 *
 * Output Parameters
 *	*argp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfslinkargs structure that 
 *	represents the old file and the new file and parent directory
 *	which is passed as a parameter across the network between NFS 
 *	clients and servers. To do this, it calls routines to handle the
 *	fhandle_t and the nfsdiropargs structures.
 *
 *
 * Algorithm
 *	{ call xdr_fhandle to handle the old file handle
 *	       xdr_diropargs to handle the new file and parent directory
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_fhandle
 *	xdr_diropargs
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_linkargs */
bool_t
xdr_linkargs(xdrs, argp)
	XDR *xdrs;
	struct nfslinkargs *argp;
{

	if (xdr_fhandle(xdrs, &argp->la_from) &&
	    xdr_diropargs(xdrs, &argp->la_to) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_linkargs: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_rnmargs *
 ********************************************************************
 ****
 ****		xdr_rnmargs(xdrs, argp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	argp		pointer to the nfsrnmargs structure
 *
 * Output Parameters
 *	*argp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfsrnmargs structure that 
 *	represents the old file and parent directory and the new file
 *	and parent directory, which is passed as a parameter across 
 *	the network between NFS clients and servers. To do this, it 
 *	calls routines to handle the nfsdiropargs structures.
 *
 * Algorithm
 *	{ call xdr_diropargs to handle the old nfsdiropargs structure
 *	       xdr_diropargs to handle the new nfsdiropargs structure
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_diropargs
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_rnmargs */
bool_t
xdr_rnmargs(xdrs, argp)
	XDR *xdrs;
	struct nfsrnmargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->rna_from) &&
	    xdr_diropargs(xdrs, &argp->rna_to) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_rnmargs: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_slargs *
 ********************************************************************
 ****
 ****		xdr_slargs(xdrs, argp)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	argp		pointer to the nfsslargs structure
 *
 * Output Parameters
 *	*argp		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	MAXPATHLEN
 *
 * Description
 *	This serializes/deserializes the nfsslargs structure that 
 *	represents the old file and parent directory, the new name
 *	and attributes, which is passed as a parameter across the 
 *	network between NFS clients and servers. To do this, it calls 
 *	routines to handle the nfsdiropargs structure, the new name
 *	and the nfssattr structure.
 *
 * Algorithm
 *	{ call xdr_diropargs to handle the nfsdiropargs structure
 *	       xdr_string to handle the new name
 *	       xdr_sattr to handle the nfssattr structure
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_diropargs
 *	xdr_string
 *	xdr_sattr
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_slargs */
bool_t
xdr_slargs(xdrs, argp)
	XDR *xdrs;
	struct nfsslargs *argp;
{

	if (xdr_diropargs(xdrs, &argp->sla_from) &&
	    xdr_string(xdrs, &argp->sla_tnm, (u_int)MAXPATHLEN) &&
	    xdr_sattr(xdrs, &argp->sla_sa) ) {
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_slargs: FAILED\n");
#endif
	return (FALSE);
}


/* BEGIN_IMS xdr_fsok *
 ********************************************************************
 ****
 ****		xdr_fsok(xdrs, fsok)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	fsok		pointer to the nfsstatfsok structure
 *
 * Output Parameters
 *	*fsok		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfsstatfsok structure that 
 *	represents the NFS_OK part of the statfs operation, which is
 *	passed as a parameter across the network between NFS clients 
 *	and servers. To do this, it calls routines to handle the
 *	various fields like the preferred transfer size in bytes,
 *	the file system block size, number of blocks, number of free
 *	blocks and the number of free blocks available to the 
 *	non-superuser.
 *
 * Algorithm
 *	{ call xdr_long to handle the transfer size in bytes
 *	       xdr_long to handle the file system block size
 *	       xdr_long to handle the number of blocks
 *	       xdr_long to handle the number of free blocks
 *	       xdr_long to handle the number of free blocks available to
 *			the non-superuser
 *	  return result
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *	xdr_long
 *	dprint
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_fsok */
xdr_fsok(xdrs, fsok)
	XDR *xdrs;
	struct nfsstatfsok *fsok;
{

	if (xdr_long(xdrs, (long *)&fsok->fsok_tsize) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bsize) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_blocks) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bfree) &&
	    xdr_long(xdrs, (long *)&fsok->fsok_bavail) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6,
		    "xdr_fsok: %s tsz %d bsz %d blks %d bfree %d bavail %d\n",
		    xdropnames[(int)xdrs->x_op], fsok->fsok_tsize,
		    fsok->fsok_bsize, fsok->fsok_blocks, fsok->fsok_bfree,
		    fsok->fsok_bavail);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_fsok: FAILED\n");
#endif
	return (FALSE);
}

struct xdr_discrim statfs_discrim[2] = {
	{ (int)NFS_OK, xdr_fsok },
	{ __dontcare__, NULL_xdrproc_t }
};


/* BEGIN_IMS xdr_statfs *
 ********************************************************************
 ****
 ****		xdr_statfs(xdrs, fs)
 ****
 ********************************************************************
 * Input Parameters
 *	xdrs		handle passed to the routines
 *	fs		pointer to the nfsstatfs structure
 *
 * Output Parameters
 *	*fs		may change depending upon the value of xdrs->x_op
 *
 * Return Value
 *	TRUE/FALSE	depending upon the sucess of the operation
 *
 * Globals Referenced
 *	none
 *
 * Description
 *	This serializes/deserializes the nfsstatfs structure that
 *	represents the result of a statfs operation and which is
 *	passed as a parameter across the network between NFS clients
 *	and servers. A call is made to xdr_union to handle the different
 *	cases possible. However, note that at the moment the only 
 *	possible option is that of NFS_OK, as represented by the
 *	nfsstatfsok structure.
 *
 * Algorithm
 *	{ call xdr_union passing as parameters the reply status (discriminant),
 *		the nfsstatfsok structure, the array of xdrdiscrims, and the
 *		default xdr_routine (in this case xdr_void)
 *	  return value
 *	}
 *
 * Concurrency
 *	none
 *
 * To Do List
 *	. in the call to xdr_union the nfsstatfsok structure is passed, instead
 *	  of the union itself. It doesn't seem to matter in this case,
 *	  but....
 *
 * Notes
 *
 * Modification History
 *
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS xdr_statfs */
xdr_statfs(xdrs, fs)
	XDR *xdrs;
	struct nfsstatfs *fs;
{

	if (xdr_union(xdrs, (enum_t *)&(fs->fs_status),
	      (caddr_t)&(fs->fs_fsok), statfs_discrim, xdr_void) ) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 6, "xdr_statfs: %s stat %d\n",
		    xdropnames[(int)xdrs->x_op], fs->fs_status);
#endif
		return (TRUE);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 2, "xdr_statfs: FAILED\n");
#endif
	return (FALSE);
}
