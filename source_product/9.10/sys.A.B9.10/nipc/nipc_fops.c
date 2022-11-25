/*
 * $Header: nipc_fops.c,v 1.4.83.4 93/09/17 19:10:07 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_fops.c,v $
 * $Revision: 1.4.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:10:07 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_fops.c $Revision: 1.4.83.4 $";
#endif

/* 
 * Netipc routines called by the file system 
 * via the file structure and f_ops ptrs.
 */
#include "../h/types.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_cb.h"
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */

fop_nosupp()
{
	return (EOPNOTSUPP);
}

/* Remove all resources used by a socket given a file pointer.
 * This routine is only called if f_count == 0.
 * Deleting the file descriptor and file credentials
 * is done before this is called.
 */
fop_close(fp)
struct file *fp;
{
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	if (fp->f_data) {
		/*MPNET: turn MP protection on */
		NETMP_GO_EXCLUSIVE(oldlevel, savestate);
		sou_delete((struct socket *) fp->f_data, fp);
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */

		SPINLOCK(file_table_lock);
		fp->f_data = 0;
		SPINUNLOCK(file_table_lock);
	}
}
 
fop_ioctl(fp, cmd, data)
struct file 		*fp;
int			cmd;
 caddr_t	data;
{
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel, result;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);
	
	result = prb_pxyioctl(cmd, data);

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	return(result);
}

 
fop_select(fp, which)
struct file	*fp;
int		which;
{
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel, result;
	 struct socket *so  = (struct socket *)fp->f_data;
	 struct nipccb *ncb = (struct nipccb *)so->so_ipccb;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	switch (which) {
	case FREAD:
		/* true if connected AND
		 * (if bytes present >= receive threshold 
		 * OR if (in message mode and complete message is present))
		 */
		if (ncb->n_flags & NF_ISCONNECTED &&
    		    (so->so_rcv.sb_cc >= ncb->n_recv_thresh    ||
		     (ncb->n_flags & NF_MESSAGE_MODE &&
			 so->so_rcv.sb_mb != 0 &&
			 (so->so_rcv.sb_mb->m_act != 0 ||
			  so->so_rcv.sb_flags & SB_MSGCOMPLETE)
		      )
		     ) 
		    ) {
			/*MPNET: turn MP protection off */
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			return(1);
		}
		sbselqueue(&so->so_rcv);
		/*MPNET: turn MP protection off */
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return(0);

	case FWRITE:
		   /* true if connected AND
		    * space present >= send threshold
		    */
		   if (ncb->n_flags & NF_ISCONNECTED	&&
		       sbspace(&so->so_snd) >= ncb->n_send_thresh) {
			/*MPNET: turn MP protection off */
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			return(1);
		}
		sbselqueue(&so->so_snd);
		/*MPNET: turn MP protection off */
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return(0);

	default:
		/*
		 * if VC socket then
		 * 	true if BSD connected but not Netipc connected
		 *		or socket is not connected
		 * else (Call socket) 
		 * 	true if connection indication queued
		 */
		if(ncb->n_type == NS_VC) {
			if ((so->so_state & SS_ISCONNECTED 
				&& ((!(ncb->n_flags & NF_ISCONNECTED)) ||
				    so->so_state & SS_CANTRCVMORE)) ||
			    !(so->so_state&SS_ISCONNECTED)) {
				/*MPNET: turn MP protection off */
 				NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
				return(1);
		}
		} else /* a call socket */
			if(so->so_qlen) {
				/*MPNET: turn MP protection off */
 				NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
				return(1);
		}
		sbselqueue(&so->so_rcv);
		/*MPNET: turn MP protection off */
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		return(0);

	}
}			/***** fop_select *****/
