/*
 * $Header: nipc_syscall.c,v 1.6.83.6 94/04/21 14:08:08 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_syscall.c,v $
 * $Revision: 1.6.83.6 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 94/04/21 14:08:08 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_syscall.c $Revision: 1.6.83.6 $";
#endif

/* 
 * Netipc system calls.
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/time.h"
#include "../h/uio.h"
#include "../h/malloc.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/ns_ipc.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_err.h"
#include "../nipc/nipc_name.h"
#include "../nipc/nipc_hpdsn.h"
#if defined (TRUX) && defined (B1)
#include "../h/security.h"
#endif  /* TRUX && B1 */
#ifdef AUDIT
#include "../h/audit.h"
#endif /* AUDIT */
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */


#ifndef	TRUE
#define  TRUE  1
#define  FALSE 0
#endif

struct socket *sou_create();

int soo_rw(), fop_ioctl(), fop_select(), fop_close(), fop_nosupp();

struct fileops nipc_socketops =
	{soo_rw, fop_ioctl, fop_select, fop_close};

struct fileops nipc_ddops =
	{fop_nosupp, fop_nosupp, fop_nosupp, fop_close};

extern char nipc_nodename[];
extern int  nipc_nodename_len;
short kludge_protoaddr;

ipcconnect()
{
	 struct a {
		int	calldesc;	/* CALL SOCKET IS IGNORED */
		int	destdesc;	/* POINTER TO A 'DEST DESC' */
		caddr_t flags;
		caddr_t opt;
		caddr_t vcdesc;
	} *uap = (struct a *)u.u_ap;

	struct socket	*so;		/* POINTER TO SOCKET */
	struct socket	*dd;		/* POINTER TO DESTINATION DESC.  */
	struct file	*fp;		/* POINTER TO A FILE */
	struct mbuf	*nam;		/* POINTER TO MBUF WITH AN ADDR */
	int		recv_size=NIPC_DEFAULT_MAX_RECV_SIZE;
	int		send_size=NIPC_DEFAULT_MAX_SEND_SIZE;
	struct nipccb	*socb;
	struct nipccb	*ddcb;
	int		error=0;
	struct nipc_protosw *proto=0;
	struct k_opt	kopt_space;
	struct k_opt	*kopt= &kopt_space;
	int		flags=0;
	int		protocol=NSP_TCP;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* IF CALL SOCKET IS OFFERED VERIFY IT'S VALID */
	if (uap->calldesc != NS_NULL_DESC) {
 		if (error = sou_getsock(uap->calldesc, &so)) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(error);
 		}

		socb = (struct nipccb *) so->so_ipccb;
 		if (socb->n_type != NS_CALL)  {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(E_NOTCALLSOCKET);
 		}
		/* GET THE PROTOCOL TO USE CREATING THE VC SOCKET */
		protocol = socb->n_protosw->np_protocol;
	}

	/* FIND THE NIPC_PROTOSW FOR THE DESIRED PROTOCOL */
 	if(error = mi_findproto(HPDSN_DOM, &proto, protocol, NS_VC)) {
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
 	}

	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
 		if (copyin(uap->flags, (caddr_t) &flags, sizeof(flags))) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(EFAULT);
 		}

	/* VERIFY FLAGS AND OPTIONS */
	if ((flags & NSF_MESSAGE_MODE) && 
#if defined (TRUX) && defined (B1)
	    (!priviledged(SEC_REMOTE, EPERM))
#else
	    (!suser() && u.u_ruid != 0))
#endif
 	{
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(EACCES);
 	}

	/* VERIFY NO FLAGS WERE OFFERED WHICH WEREN'T USED */
#define LEGAL_FLAGS (NSF_MESSAGE_MODE)
 	if ((flags | LEGAL_FLAGS) != LEGAL_FLAGS) {
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(E_FLAGS);
 	}
#undef LEGAL_FLAGS

	/* COPY OPTIONS INTO THE KERNEL */
 	if (error = mi_getoptions(uap->opt, kopt)) {
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(error);
 	}

	if (KOPT_DEFINED(kopt, KO_MAX_RECV_SIZE)) {
		recv_size = KOPT_FETCH(kopt, KO_MAX_RECV_SIZE);
 		if (recv_size < 0) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(EMSGSIZE);
 		}
		KOPT_UNDEFINE(kopt, KO_MAX_RECV_SIZE);
	}

	if (KOPT_DEFINED(kopt, KO_MAX_SEND_SIZE)) {
		send_size = KOPT_FETCH(kopt, KO_MAX_SEND_SIZE);
 		if (send_size < 0) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(EMSGSIZE);
 		}
		KOPT_UNDEFINE(kopt, KO_MAX_SEND_SIZE);
	}

	/* VERIFY NO OPTIONS WERE OFFERED WHICH WEREN'T USED */
 	if (KOPT_ANY_DEFINED(kopt)) {
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(E_OPTOPTION);
 	}

	/* CONVERT THE DESTINATION DESCRIPTOR TO A SOCKET POINTER */
 	if (error = sou_getsock(uap->destdesc, &dd)) {
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(error);
 	}

	/* THE SOCKET MUST BE A DESTINATION DESCRIPTOR */
	ddcb = (struct nipccb *) dd->so_ipccb;
 	if (ddcb->n_type != NS_DEST) {
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(E_DEST);
 	}

	/* CREATE THE SOCKET */
	so = sou_create(proto);
	
	/* CALL THE PROTOCOL TO ATTACH */
	if (error = (*so->so_proto->pr_usrreq)(so, PRU_ATTACH, 
			(struct mbuf *) 0, (struct mbuf *) 0,
			(struct mbuf *) 0)) {
		/* free dd resources      */
		(void) sou_delete(so, (struct file *) 0);	
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* SET THE BUFFER SIZES TO NETIPC VALUES */
	(void) soreserve(so, (u_long) send_size, (u_long) recv_size);

	if (flags & NSF_MESSAGE_MODE) {

		/* SET FLAG FOR USE WHEN SENDING AND RECEIVING */
		((struct nipccb *) so->so_ipccb)->n_flags |= NF_MESSAGE_MODE;

		/* INFORM THE PROTOCOL TO USE MESSAGE MODE */
		if (error = (*so->so_proto->pr_usrreq)(so,
				PRU_MESSAGE_MODE, (struct mbuf *) 0,
				(struct mbuf *) 0, (struct mbuf *) 0)) {
			(void) sou_delete(so, (struct file *) 0);	
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(error);
		}
	}

	/* GET AN MBUF FOR THE PROTOCOL ADDRESS */
	nam = m_get(M_WAIT, MT_SONAME);
	nam->m_len = sizeof(struct sockaddr_in);

	/* EXTRACT THE ADDRESS FROM THE CONN. SITE. PATH REPORT */
	if (error = path_getcsiteaddr(ddcb->n_cspr, proto, 
		mtod(nam, struct sockaddr *))) {
		m_free(nam);
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* CALL THE PROTOCOL TO CONNECT */
	if(error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
		(struct mbuf *) 0, nam, (struct mbuf *) 0)) {
		 
		/* FREE THE MBUF FOR THE ADDRESS */
		m_free(nam);

		/* DELETE THE SOCKET */
		(void) sou_delete(so, (struct file *) 0);	
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* FREE THE MBUF FOR THE ADDRESS */
	m_free(nam);

	if ((fp = falloc()) == 0) {
		/* DELETE THE SOCKET */
		(void) sou_delete(so, (struct file *) 0);	

 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(u.u_error);
		/* u.u_error is either ENFILE or EMFILE */
	}

	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_ops = &nipc_socketops;
	fp->f_data = (caddr_t)so;

	/* COPYOUT THE RESULTING FILE DESCRIPTOR */
	/* FALLOC PUT THE FILE DESCRIPTOR IN U.U_R.R_VAL1 */
	if (error = copyout((caddr_t) &u.u_r.r_val1,
			(caddr_t)uap->vcdesc, sizeof(uap->vcdesc))) {
		
		/* PUT THE FD IN THE UARGS AREA FOR "CLOSE" */
		u.u_arg[0] = u.u_r.r_val1; 

		/* UNDO ALL THE GOOD WORK WE'VE DONE */
		close();
	}
	
 	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	/*MPNET: MP protection is now off. */
 
	NETIPC_RETURN(error);
} /* ipcconnect */
 

ipccontrol()
{
	struct a {
		int	descriptor;
		int	request;
		caddr_t	wrtdata;
		int	wlen;
		caddr_t	readdata;
		caddr_t	rlen;
		caddr_t	flags;
	} *uap = (struct a *) u.u_ap;

	int		error=0;
	int		read=FALSE;
	struct nipccb	*socb;
	short		wrtdata;	/* DOC. SPECIFIES SIGNED SHORTS */
	caddr_t		rdata;		/* DOC. SPECIFIES SIGNED SHORTS */
	struct socket	*so;
	int		flags=0;
	int		rlen=0;
	int		*rlenptr = &rlen;
 	struct mbuf	*nam=0;		/* mbuf for sockaddr */
  	sv_sema_t savestate;		/* MPNET: MP save state */
  	int oldlevel;

	/* COPYIN THE READ LENGTH */
	if (uap->rlen)
		if (copyin(uap->rlen, (caddr_t) rlenptr, sizeof(rlen)))
			NETIPC_RETURN(EFAULT);

 	/*MPNET: turn MP protection on */
 	NETMP_GO_EXCLUSIVE(oldlevel, savestate);
 
	/* IF REQUEST IS TO GET NODE NAME THEN DO IT */
	if (uap->request == NSC_GET_NODE_NAME) {
		/* DONT COPY OUT A PARTIAL NAME */
 		if (rlen < nipc_nodename_len) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(E_NLEN);
 		}

		/* COPYOUT THE NAME */
 	 	if (copyout(nipc_nodename, uap->readdata, nipc_nodename_len)) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
 		}

		/* COPYOUT THE LENGTH */
		if (copyout((caddr_t) &nipc_nodename_len,uap->rlen,
 			sizeof(nipc_nodename_len))) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
 		}
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		
		NETIPC_RETURN(0);
	}

	/* FOLLOW THE FILE DESCRIPTOR TO A SOCKET POINTER */
 	if (error = sou_getsock(uap->descriptor, &so)) {
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
 	}

	/* THE SOCKET MUST NOT BE A DESTINDATION DESCRIPTOR */
	socb = (struct nipccb *) so->so_ipccb;

 /*	if (socb->n_type == NS_DEST) {
 /*		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 /*		/*MPNET: MP protection is now off. */
 /*		NETIPC_RETURN(E_REQUEST);
 /*	}
 */
	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
 		if (copyin(uap->flags, (caddr_t) &flags, sizeof(int))) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(EFAULT);
 		}

	/* VERIFY NO FLAGS WERE OFFERED WHICH WEREN'T USED */
 	if (flags){
 	    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	    /*MPNET: MP protection is now off. */
 	    NETIPC_RETURN(E_FLAGS);
 	}

	/* COPYIN THE WRITE DATA IF ANY */
	/* validate offered writeable data length */
 	if (uap->wlen && uap->wlen != NIPC_CONTROL_WLEN) {
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_DLEN);
 	}

	if (uap->wlen) 
 		if (copyin(uap->wrtdata,(caddr_t)&wrtdata,sizeof(wrtdata))){
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(EFAULT);
 		}

	/* EXECUTE THE REQUEST */
	switch (uap->request) {
	case NSC_SOCKADDR:
 		if (rlen != sizeof(struct sockaddr)) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_DLEN);
 		}
		/* get an mbuf for the protocol address */
		nam = m_get(M_WAIT, MT_SONAME);
		nam->m_len = sizeof(struct sockaddr_in);

		socb = (struct nipccb *) so->so_ipccb;

		switch (socb->n_type) {
		case NS_DEST: /* get remote addr */
			if (error = path_getcsiteaddr( socb->n_cspr, 
				socb->n_protosw, 
				mtod(nam, struct sockaddr *))) {
				m_free(nam);
 				NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 				/*MPNET: MP protection is now off. */
				NETIPC_RETURN(error);
			}
			break;
		case NS_VC: /* get remote addr */
			if(error = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR,
				(struct mbuf *) 0, nam, (struct mbuf *) 0)) {
				/* FREE THE MBUF FOR THE ADDRESS */
				m_free(nam);
 				NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 				/*MPNET: MP protection is now off. */
				NETIPC_RETURN(error);
			}
			break;
		case NS_CALL: /* get local addr */
			if(error = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR,
				(struct mbuf *) 0, nam, (struct mbuf *) 0)) {
				/* FREE THE MBUF FOR THE ADDRESS */
				m_free(nam);
 				NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 				/*MPNET: MP protection is now off. */
				NETIPC_RETURN(error);
			}
			break;
		}
		rdata = mtod(nam, caddr_t);
		read = TRUE;
		break;
	case NSC_NBIO_ENABLE:
		so->so_state |= SS_NBIO;
		break;
	case NSC_NBIO_DISABLE:
		so->so_state &= ~SS_NBIO;
		break;
	case NSC_TIMEOUT_RESET:
		/* DUPLICATE SANITY CHECKS DONE IN itimerfix */
 		if (wrtdata < 0 || wrtdata > NIPC_MAX_TIMEO) {
 		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		    /*MPNET: MP protection is now off. */
 		    NETIPC_RETURN(E_TIMEOUTVALUE);
 		}
		so->so_timeo = wrtdata;
		break;
	case NSC_TIMEOUT_GET:
 		if (rlen != NIPC_CONTROL_RLEN){
 		    	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_DLEN);
 		}
		read = TRUE;
		rdata = (caddr_t) &so->so_timeo;
		break;
	case NSC_RECV_THRESH_RESET:
 		if (socb->n_type != NS_VC) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_REQUEST);
		}
 		if (wrtdata < 0) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_THRESHVALUE);
 		}
		socb->n_recv_thresh = wrtdata;
		break;
	case NSC_RECV_THRESH_GET:
 		if (rlen != NIPC_CONTROL_RLEN) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_DLEN);
 		}
 		if (socb->n_type != NS_VC) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_REQUEST);
 		}
		read = TRUE;
		rdata = (caddr_t) &socb->n_recv_thresh;
		break;
	case NSC_SEND_THRESH_RESET:
 		if (socb->n_type != NS_VC) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_REQUEST);
 		}
 		if (wrtdata < 0) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_THRESHVALUE);
 		}
		socb->n_send_thresh = wrtdata;
		break;
	case NSC_SEND_THRESH_GET:
 		if (rlen != NIPC_CONTROL_RLEN) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_DLEN);
 		}
 		if (socb->n_type != NS_VC) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_REQUEST);
 		}
		read = TRUE;
		rdata = (caddr_t) &socb->n_send_thresh;
		break;
	default:
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_REQUEST);
	}

	/* IF IT WAS A READ REQUEST THEN COPYOUT THE REQUESTED DATA */
	if (read)
		error = copyout((caddr_t) rdata, uap->readdata, rlen);

	if (nam)
		m_free(nam);

 	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 	/*MPNET: MP protection is now off. */
	NETIPC_RETURN(error);
}

 

ipccreate()
{
	struct a {
		int	socketkind;
		int	protocol;	
		caddr_t	flags;	
		caddr_t	opt;
		caddr_t	calldesc;		/* OUTPUT */
	} *uap = (struct a *) u.u_ap;

	struct k_opt		kopt_space;
	struct k_opt		*kopt= &kopt_space;
	int			flags=0;
	struct mbuf		*nam=0;
	short			qlimit=NIPC_DEFAULT_CONN_REQ_BACK;
	struct nipc_protosw	*proto=0;
	struct nipc_protoaddr	*proto_addr=0;
	struct file		*fp;
	int			error=0;
	struct socket		*so;
	int			mi_timerpopped();
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* if protocol == 0 choose the best... tcp */
	if (!uap->protocol)
		uap->protocol = NSP_TCP;

	/* LET THE SOCKETKIND AND PROTOCOL BE CHECKED BY PFFINDPROTO */
	if(error = mi_findproto(HPDSN_DOM, 	
			&proto,uap->protocol, uap->socketkind)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}
	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
		if (copyin(uap->flags, (caddr_t) &flags, sizeof(flags))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
	}

	if (flags != 0)  {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_FLAGS); 
	}

	/* COPY OPTIONS INTO THE KERNEL */
	if (error = mi_getoptions(uap->opt, kopt)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	if (KOPT_DEFINED(kopt, KO_MAX_CONN_REQ_BACK)) {
		/* FETCH THE QLIMIT NUMBER */
		qlimit = KOPT_FETCH(kopt, KO_MAX_CONN_REQ_BACK);

		/* VALIDATE THE PROPOSED LIMIT */
		if (qlimit > KO_CONN_REQ_BACK_MAX ||
		    qlimit < KO_CONN_REQ_BACK_MIN) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_MAXCONNECTQ);
		}
		
		KOPT_UNDEFINE(kopt, KO_MAX_CONN_REQ_BACK);
	}

	/* GET THE PORT IF ONE IS GIVEN AND BUILD ADDRESS */
	if (KOPT_DEFINED(kopt, KO_PROTOCOL_ADDRESS)) {
		proto_addr = KOPT_FETCH(kopt, KO_PROTOCOL_ADDRESS);
		KOPT_UNDEFINE(kopt, KO_PROTOCOL_ADDRESS);
		/* SUPER USER STATUS IS CHECKED IN IN_PCBBIND */
	}

	/* VERIFY NO OPTIONS WERE OFFERED WHICH WEREN'T USED */
	if (KOPT_ANY_DEFINED(kopt))  {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_OPTOPTION);
	}

	/* CREATE A SOCKET */
	so = sou_create(proto);

	/* SET THE QUEUE LIMIT */
	so->so_qlimit = qlimit;

	/* CALL THE PROTOCOL TO ATTACH */
	if (error = (*so->so_proto->pr_usrreq)(so, PRU_ATTACH, 
			(struct mbuf *) 0, /* no message */
			(struct mbuf *) 0, /* no address */
			(struct mbuf *) 0)) /* no rights */ {
		(void) sou_delete(so, (struct file *) 0);	
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* GET A BUFFER FOR THE PROTOCOL ADDRESS */
	nam = m_get(M_WAIT, MT_SONAME);

	/* CALL A PROTOCOL SPECIFIC ROUTINE TO BUILD ADDR */
	if (error = proto->np_proto_addr(proto_addr, nam)){
		m_free(nam);
		(void) sou_delete(so, (struct file *) 0);	
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* CALL PROTOCOL TO BIND */
	if (error = (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *) 0, 
		nam,	/* may be null */ (struct mbuf *) 0)) {
		m_free(nam);
		(void) sou_delete(so, (struct file *) 0);	
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* FREE THE MBUF FOR THE ADDRESS */
	m_free(nam);

	
	/* CALL PROTOCOL TO LISTEN */
	(void)(*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *) 0, 
		(struct mbuf *) 0, 
		(struct mbuf *) 0);

	if ((fp = falloc()) == 0) {
		/* FREE THE SOCKET */
		(void) sou_delete(so, (struct file *) 0);	

		/* u.u_error is set to ENFILE or EMFILE by falloc */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(u.u_error);
	}

	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_ops = &nipc_socketops;
	fp->f_data = (caddr_t)so;

	/* COPYOUT THE RESULTING FILE DESCRIPTOR */
	/* FALLOC PUT THE FILE DESCRIPTOR IN U.U_R.R_VAL1 */
	if (error = copyout((caddr_t) &u.u_r.r_val1,
			uap->calldesc,
			sizeof(uap->calldesc))) {
		
		/* PUT THE FD IN THE UARGS AREA */
		u.u_arg[0] = u.u_r.r_val1; 

		/* UNDO ALL THE GOOD WORK WE'VE DONE */
		close();
	}

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	NETIPC_RETURN(error);
} /* ipccreate */
		
 


ipcdest()
{
	/* set up pointers to caller's parameters */
	 struct a {
		int	socketkind;	/* nipc socket kind of dest socket    */
		caddr_t nodename;	/* nodename where dest socket located */
		int	nodelen;	/* byte length of nodename            */
		int	protocol;       /* L4 protocol attached to dest socket*/
		caddr_t protoaddr;	/* protocol port address for dest sock*/
		int	protolen;	/* byte length of protoaddr           */
		caddr_t flags;		/* flags parm (no flags defined)      */
		caddr_t opt;		/* options parm (no options defined   */
		caddr_t destdesc;	/* location for output dd parm        */
	} *uap = (struct a *) u.u_ap;

	char   		knodename[NS_MAX_NODE_NAME];    /* kernel nodename    */
	char   		kprotoaddr[NIPC_MAX_PROTOADDR]; /* kernel protoaddr   */
	struct k_opt	kopt_space;			/* kernel options     */
	struct k_opt	*kopt = &kopt_space;		/* kernel options ptr */
	int		kflags;				/* kernel flags       */
	struct nipc_protosw	*proto;       		/* ptr to proto info  */
	struct socket	*dd;         			/* destdesc socket ptr*/
	struct file	*fp;				/* file table pointer */
	u_short		*npr;				/* nodal path report  */
	struct mbuf	*cspr;				/* cspr for dd        */
	struct nipccb	*ncb;				/* nipccb of dd	      */
	int		error=0;			/* local error        */
	sv_sema_t savestate;			/* MPNET: MP save state */
	int oldlevel;


	/* verify that socketkind is NS_CALL */
	if (uap->socketkind != NS_CALL)
		NETIPC_RETURN(E_NOTCALLSOCKET);

	/* verify that no flags are set */
	if (copyin(uap->flags, (caddr_t) &kflags, sizeof(int)))
		NETIPC_RETURN(EFAULT);
	if (kflags != 0)
		NETIPC_RETURN(E_FLAGS);

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* verify that no options were passed */
	if (uap->opt != (caddr_t)0) {
 		if (error = mi_getoptions(uap->opt, kopt)) {
 			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(error);
 		}
		if (KOPT_ANY_DEFINED(kopt)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_OPTOPTION);
		}
	}

	/* find the nipc_protosw for the protocol */
	if(error=mi_findproto(HPDSN_DOM,&proto,uap->protocol,uap->socketkind)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* verify the protoaddr length and copy the protoaddr into the kernel */
	if (uap->protolen != proto->np_addr_len) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_ADDROPT);
	}
	if (copyin(uap->protoaddr, (caddr_t) kprotoaddr, uap->protolen)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
	}

	/* if a nodename was specified, verify the length, copy it in, */
	/* and qualify it to ensure proper syntax		       */
	if (uap->nodelen != 0) {
		if (uap->nodelen > NS_MAX_NODE_NAME || uap->nodelen < 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_NLEN);
		}
		if (copyin(uap->nodename, (caddr_t) knodename, uap->nodelen)){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
		if (error = nm_qualify(knodename, &(uap->nodelen))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(error);
		}
	}

#ifdef AUDIT
	if(AUDITEVERON()) {
		(void)save_str(uap->nodename);
	}
#endif /* AUDIT */

	/* get a nodal path report for nodename */
 	if (error = nm_getnpr(knodename, uap->nodelen, &npr)) {
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
 	}

	/* map the nodal path report to a connect site path report for the dd */
	error = path_nodetocsite(npr, proto, kprotoaddr, &cspr);
	FREE(npr, M_NIPCPR);  /* free the nodal path report */
	if (error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* allocate and initialize socket and nipccb structures for the dd */
	dd = sou_create(proto);

	/* set values in the nipccb for the destination descriptor */
	ncb = (struct nipccb *)(dd->so_ipccb);
	ncb->n_type      = NS_DEST;
	ncb->n_dest_type = uap->socketkind;
	ncb->n_cspr      = cspr;

	/* allocate a file descriptor and file table entry for the dd */
	if ((fp = falloc()) == 0) {
		/* free dd resources      */
		(void) sou_delete(dd, (struct file *) 0);	
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(u.u_error);	/* falloc puts error here */
	}

	/* initialize the file table entry for the dd */
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = 0;			/* can't read or write on a dd */
	fp->f_data = (caddr_t)dd;
	fp->f_ops  = &nipc_ddops;

	/* copyout the file descriptor to the caller's destdesc             */
	/* note that the file descriptor was left in u.u_r.r_val1 by falloc */
 	error = copyout((caddr_t) &u.u_r.r_val1, uap->destdesc, sizeof(int));
 	if (error) { 
		/* close the file descriptor to get rid of all resources */
		u.u_arg[0] = u.u_r.r_val1;
		close();
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	NETIPC_RETURN(0);
}  /* ipcdest */



ipcgetnodename()

{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t nodename;	/* buffer to copy nodename to     */
		caddr_t	size;		/* byte length of nodename buffer */
	} *uap = (struct a *) u.u_ap;

	int	ksize;	/* kernel space for buffer size */
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* verify that the size of the caller's buffer is large enough */
	if (copyin(uap->size, (caddr_t) &ksize, sizeof(ksize)))  {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
	} else {
		if (ksize < nipc_nodename_len) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_NLEN);
		}
	}

	/* copyout the nodename length */
	if (copyout((caddr_t) &nipc_nodename_len, uap->size,
			sizeof(nipc_nodename_len))) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
	}

	/* copyout the nipc_nodename */
	if (nipc_nodename_len) {
		if (copyout((caddr_t) nipc_nodename, uap->nodename,
				nipc_nodename_len)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
	}
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(0);

}  /* getnodename */



ipclookup()
{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t	socketname;	/* name of socket to lookup           */
		int	nlen;		/* length of socketname		      */
		caddr_t nodename;	/* nodename where named socket located*/
		int	nodelen;	/* byte length of nodename            */
		caddr_t flags;		/* flags parm (no flags defined)      */
		caddr_t destdesc;	/* (output) destination descriptor    */
		caddr_t	protocol;       /* (output) protocol of named socket  */
		caddr_t socketkind;	/* (output) nipc type of named socket */
	} *uap = (struct a *) u.u_ap;

	char		ksoname[NS_MAX_SOCKET_NAME];	/* kernel socketname  */
	char   		knodename[NS_MAX_NODE_NAME];    /* kernel nodename    */
	int		kflags;				/* kernel flags       */
	struct socket	*dd;         			/* destdesc socket    */
	struct file	*fp;				/* file table pointer */
	int		protocol;			/* protocol of socket */
	int		sockkind;			/* socket kind for dd */
	struct mbuf	*cspr;				/* cspr for dd        */
	struct nipc_protosw	*proto;			/* protosw for dd     */
	struct nipccb	*ncb;				/* nipccb for dd      */
	int		error=0;			/* local error        */
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* verify that no flags are set */
	if (uap->flags) {
		if (copyin(uap->flags, (caddr_t) &kflags, sizeof(int))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
		if (kflags != 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_FLAGS);
		}
	}


	/* verify the socketname length and copy in the socketname */
	if (uap->nlen <= 0 || uap->nlen > NS_MAX_SOCKET_NAME) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_NLEN);
	} else  {
		if (copyin(uap->socketname, (caddr_t) ksoname, uap->nlen)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		} else
			nm_downshift(ksoname, uap->nlen);  /* put in in lower case */
	}

	/* if a nodename was specified, verify the length, copy it in, */
	/* and qualify it to ensure proper syntax		       */
	if (uap->nodelen != 0) {
		if (uap->nodelen > NS_MAX_NODE_NAME || uap->nodelen < 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_NLEN);
		}
		if (copyin(uap->nodename, (caddr_t) knodename, uap->nodelen)){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
		if (error = nm_qualify(knodename, &(uap->nodelen))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(error);
		}
	}

#ifdef AUDIT
	if(AUDITEVERON()) {
		(void)save_str(uap->socketname);
		(void)save_str(uap->nodename);
	}
#endif /* AUDIT */

	/* depending on the nodename, call the appropriate lookup routine */
	/* to get the socketkind, protocol, and cspr for the dd           */
	if (nm_islocal(knodename, uap->nodelen))
		error = nm_locallookup(ksoname, uap->nlen, PATH_LOOPOK,
				 &protocol, &sockkind, &cspr);
	else
		error = nm_remotelookup(knodename, uap->nodelen, ksoname,
					uap->nlen, &protocol, &sockkind, &cspr);
	if (error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);

	}
	/* find the nipc_protosw for the protocol/sockkind */
	if (error = mi_findproto(HPDSN_DOM,&proto,protocol,sockkind)) {
		m_freem(cspr);	/* free the connect site path report */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* allocate and initialize a socket and nipccp for the dd */
	dd = sou_create(proto);

	/* set values in the nipccb for the destination descriptor */
	ncb = (struct nipccb *)(dd->so_ipccb);
	ncb->n_type      = NS_DEST;
	ncb->n_dest_type = sockkind;
	ncb->n_cspr      = cspr;

	/* allocate a file descriptor and file table entry for the dd */
	if ((fp = falloc()) == 0) {
		/* free dd resources      */
		(void) sou_delete(dd, (struct file *) 0);	
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(u.u_error);	/* falloc puts error here */
	}

	/* initialize the file table entry for the dd */
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = 0;			/* can't read or write on a dd */
	fp->f_data = (caddr_t)dd;
	fp->f_ops  = &nipc_ddops;

	/* copyout the file descriptor, protocol, and socketkind;           */
	/* note that the file descriptor was left in u.u_r.r_val1 by falloc */
 	error = copyout((caddr_t) &u.u_r.r_val1, uap->destdesc, sizeof(int));
 	if (error) 
		goto ERROR_RETURN;
	protocol = 0;  /* the man page states that we always return zero */
	if (error = copyout((caddr_t) &protocol, uap->protocol, sizeof(int))) 
		goto ERROR_RETURN;
	if (error = copyout((caddr_t) &sockkind, uap->socketkind, sizeof(int))) 
		goto ERROR_RETURN;

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	NETIPC_RETURN(0);

ERROR_RETURN:
	/* close the file descriptor to get rid of all resources */
	u.u_arg[0] = u.u_r.r_val1;
	close();
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	NETIPC_RETURN(error);

}  /* ipclookup */




ipcname()
{
	/* set up pointers to caller's parameters */
	 struct a {
		int 	descriptor;	/* file descriptor to be named */
		caddr_t socketname;	/* name to bind to descriptor  */
		int	nlen;		/* byte length of socketname   */
	} *uap = (struct a *) u.u_ap;

	char   		kname[NS_MAX_SOCKET_NAME]; /* kernel socketname   */
	struct socket	*so;            	   /* descriptor target   */
	struct nipccb	*ncb;			   /* nipccb for socket   */
	caddr_t 	nr;		   	   /* sockreg name record */
	int		error=0;		   /* local error         */
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* get the "socket" associated with the caller's descriptor */
	if (error = sou_getsock(uap->descriptor, &so)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* verify that the socket type is not NS_VC */
	ncb = (struct nipccb *) so->so_ipccb;
	if (ncb->n_type == NS_VC) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_CANTNAMEVC);
	}

	/* get a socketname for the descriptor either by copying in the */
	/* caller-specified name if there is one, or creating a random  */
	/* name and copying it out to the caller                        */
	if (uap->nlen != 0) {
		if (uap->nlen > NS_MAX_SOCKET_NAME || uap->nlen < 0){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_NLEN);
		}
		if (copyin(uap->socketname, (caddr_t) kname, uap->nlen)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
		nm_downshift(kname, uap->nlen);  /* put it in lower case */
	} else {
		nm_random(uap->socketname, kname);
		uap->nlen = NM_SOCKLEN;
		if (copyout((caddr_t) kname, uap->socketname, uap->nlen)){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
	}

	/* bind the name to the socket within the socket registry */
	if (error = sr_add(kname, uap->nlen, getf(uap->descriptor), &nr)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* record the name binding within the socket */
	if (ncb->n_name) {
		/* there's already a name bound to this socket      */
		/* so we clear it and set the name collision flag   */
		ncb->n_name = (caddr_t)0;
		ncb->n_flags = ncb->n_flags | NF_COLLISION;
	} else  /* record the name IF no collisions have occurred */
		if (!(ncb->n_flags & NF_COLLISION))
			ncb->n_name = nr;

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(0);

}  /* ipcname */



ipcnamerase()
{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t socketname;	/* name to bind to descriptor  */
		int	nlen;		/* byte length of socketname   */
	} *uap = (struct a *) u.u_ap;

	char   		kname[NS_MAX_SOCKET_NAME]; /* kernel socketname  */
	struct file	*fp;			   /* file table pointer */
	int		error=0;		   /* local error        */
	int		i, found;		   /* loop control vars  */
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* verify the socket name length is in bounds */
	if (uap->nlen <= 0 || uap->nlen > NS_MAX_SOCKET_NAME){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_NLEN);
	}

	/* copyin the socket name */
	if (copyin(uap->socketname, (caddr_t) kname, uap->nlen)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
	}

	/* put the socket name in lower case */
	nm_downshift(kname, uap->nlen);

	/* check if the name exists in the socket registry */
	if (error = sr_lookup(kname, uap->nlen, &fp)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* verify that the caller is the owner of the name by checking   */
	/* to see if he has a descriptor for the associated ftab pointer */
	for (i = found = 0; i <= u.u_highestfd; i++)
		if (fp == getf(i)) {
			found++;
			break;
		}
	if (!found) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_NOOWNERSHIP);
	}

	/* delete the name-to-socket binding from the socket registry */
	if (sr_delete(kname, uap->nlen))
		panic("ipcnamerase");

	/* erase the name-to-socket binding within the socket */
	((struct nipccb *)((struct socket *)fp->f_data)->so_ipccb)->n_name = 0;

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(0);

}  /* ipcnamerase */


ipcrecv()
{
	struct a {
		int	vcdesc;
		caddr_t	data;
		int	dlen;
		caddr_t	flags;
		caddr_t	opt;
	} *uap = (struct a *) u.u_ap;

	struct socket	*so;		/* POINTER TO SOCKET */
	int		error=0;	/* RESULT OF THIS ROUTINE */
	int		s;	/* RETURN FROM SPLNET */
	struct timeval	*atv=0;		/* IF NULL THEN NO TIMER SET */
	struct timeval	timeout;	/* HOLDS EXPIRATION TIME */
	extern struct timeval	time;
	int		eom;
	int		flags=0;	
	struct iovec	iov_space[MAXIOV];
	struct iovec	*aiov = iov_space;
	struct uio	auio;
	int		i;
	struct k_opt	kopt_space;
	struct k_opt	*kopt= &kopt_space;
	struct nipccb	*socb;
	int		mi_timerpopped();
	int		len;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* CONVERT FD TO SOCKET */
	if (error = sou_getsock(uap->vcdesc, &so)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* VERIFY ITS A VC SOCKET */
	socb = (struct nipccb *) so->so_ipccb;
	if (socb->n_type != NS_VC) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(ENOTCONN);
	}

	/* COPY OPTIONS INTO THE KERNEL */
	if (error = mi_getoptions(uap->opt, kopt)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
		if (copyin(uap->flags, (caddr_t) &flags, sizeof(int))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}

	/* GOTO THE 'RECEIVE DATA' SECTION IF ESTABLISHED */
	if (socb->n_flags & NF_ISCONNECTED)
		goto recv_data;

	/* IF STILL CONNECTING AND NON-BLOCKING MODE THEN RETURN */
	if (so->so_state & SS_ISCONNECTING && so->so_state & SS_NBIO) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EWOULDBLOCK);
	}

	/* START A TIMER */
	if (so->so_timeo && !(so->so_state & SS_NBIO)) {

		/* BUILD A TIMEVAL STRUCT WITH THE SO_TIMEO DELTA */
		atv = &timeout;
		timeout.tv_sec = so->so_timeo/10;
		timeout.tv_usec = (so->so_timeo - (timeout.tv_sec*10))*100000;
		
		/* ADD THE CURRENT TIME TO THE DELTA */
		s = spl6();
		timevaladd(atv, &time);
		splx(s);

		/* START THE TIMER */
		net_timeout(mi_timerpopped, (caddr_t)&so->so_timeo,hzto(atv));
	}

	/* SLEEP UNTIL NO LONGER CONNECTING */
	/* IF PROTOCOL DETECTS ERROR THEN SS_ISCONNECTING IS CLEARED */
	while (so->so_state & SS_ISCONNECTING && !error)
		error = sleep((caddr_t)&so->so_timeo, PZERO+1 | PCATCH);

	/* IF TIMER SET THEN CANCEL TIMER */
	if (atv)
		net_untimeout(mi_timerpopped, (caddr_t)&so->so_timeo);
	
	/* IF ERROR THEN SIGNAL OCCURED */
	if (error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EINTR);
	}
		
	/* CHECK IF TIMER EXPIRED */			
	s = spl6();	
	if (atv && (time.tv_sec > atv->tv_sec || 
	    time.tv_sec == atv->tv_sec && time.tv_usec >= atv->tv_usec)) {
		splx(s); 
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EIO);
	}
	splx(s); 

	/* CHECK IF THE PROTOCOL DETECTED AN ERROR */
	if (so->so_error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(so->so_error);
	}

	/* MARK THE SOCKET AS 'NETIPC' CONNECTED */
	socb->n_flags |= NF_ISCONNECTED;

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	NETIPC_RETURN(0);
		
recv_data:
	/* BY THIS POINT WE HAVE 
	 * 	A POINTER TO A SOCKET   (SO)
	 *	A POINTER TO THE NICPCB (NCB)
	 *	THE OPTIONS ARE IN THE KERNEL (KOPT)
	 *	THE FLAGS ARE IN THE KERNEL (FLAGS)
	 *	THE NF_ISCONNECTED FLAG IS SET.
	 */

	 /* BUILD AN IOVEC */
	 if (flags & NSF_VECTORED) {

		/* DETECT NAUGHTY PROGRAMMERS */
		if (KOPT_DEFINED(kopt, KO_DATA_OFFSET)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_OPTOPTION);
		}

		/* SET THE UIO IOVCNT FIELD */
		/* FIRST, VERIFY THE dlen IS A MULTIPLE OF A struct iovec */
		if ((uap->dlen % sizeof(struct iovec)) != 0 )  {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_DLEN);
		}
		auio.uio_iovcnt = uap->dlen / sizeof(struct iovec);
		/* NOW VERIFY THE UIO IOVCNT IS NOT TOO LARGE */
		if (auio.uio_iovcnt > MAXIOV) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_TOOMANYVECTS);
		}

		/* COPYIN IN THE USER SPECIFIED IOVEC */
		if (copyin((caddr_t) uap->data, (caddr_t) aiov,
				(unsigned) auio.uio_iovcnt * sizeof(aiov[0]))){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
		/* SET THE UIO UIO_IOV FIELD */
		auio.uio_iov = aiov;
	}
	else {	/* NOT VECTORED */
		int offset = 0;
		
		/* SET THE UIO_OFFSET FIELD */
		if (KOPT_DEFINED(kopt, KO_DATA_OFFSET)) {
			offset = KOPT_FETCH(kopt, KO_DATA_OFFSET);
			KOPT_UNDEFINE(kopt, KO_DATA_OFFSET);
		}
		
		/* SET THE IOV IOVCNT FIELD */ 
		auio.uio_iovcnt = 1;

		/* BUILD AN IOVEC */
		aiov->iov_len = uap->dlen;
		aiov->iov_base = uap->data + offset;

		/* SET THE UIO UIO_IOV FIELD */
		auio.uio_iov = aiov;

	}

	/* FINISH SETTING THE UIO  */
	/* SET THE UIO UIO_SET AND UIO_RESID FIELDS */
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = 0;
	auio.uio_offset = 0;

	/* VERIFY NO UNSUPPORTED OPTIONS ARE SET */
	if (KOPT_ANY_DEFINED(kopt)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_OPTOPTION);
	}

	/* VERIFY NO ILLEGAL FLAGS ARE SET */
#define LEGAL_FLAGS (NSF_DATA_WAIT|NSF_PREVIEW|NSF_VECTORED)
	if ((flags | LEGAL_FLAGS) != LEGAL_FLAGS) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_FLAGS);
	}
#undef LEGAL_FLAGS
	
	/* FOREACH IOV VERIFY ADDRESS IS WRITEABLE AND INCREMENT RESID */
	for (i=0; i < auio.uio_iovcnt ; i++ , aiov++) {
		if (aiov->iov_len < 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_VECTCOUNT);
		}

		/* ALLOW EMPTY IO VECTORS */
		if (aiov->iov_len == 0)
			continue;

		/* INSURE THE INDICATED SPACE IS WRITABLE */
		if(useracc(aiov->iov_base, (u_int)aiov->iov_len,B_WRITE) == 0){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
			}
		
		/* INCREMENT THE TOTAL BYTES TO RECEIVABLE */
		auio.uio_resid += aiov->iov_len;
	}

	/* RECEIVE BYTES FROM THE SOCKET */
	len = auio.uio_resid;
	if (error = sou_receive(so, &auio, flags, &eom)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* SET THE OUTPUT FLAGS */
	if (!eom)
		flags = NSF_MORE_DATA;
	else
		flags = 0;

	/* RETURN THE DLEN */
	u.u_rval1 = len - auio.uio_resid;

	/* CALL COPYOUT TO RETURN THE FLAGS */
	error = copyout((caddr_t) &flags, uap->flags, sizeof(flags));
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(error);
}

 

ipcrecvcn()
{
	struct a {
		int	calldesc;
		caddr_t	vcdesc;
		caddr_t	flags;
		caddr_t	opt;
	} *uap = (struct a *) u.u_ap;

	int		recv_size=NIPC_DEFAULT_MAX_RECV_SIZE;
	int		send_size=NIPC_DEFAULT_MAX_SEND_SIZE;
	struct file	*fp;
	int		error=0;
	struct timeval	*atv=0;		/* IF NULL THEN NO TIMER SET */
	struct timeval	timeout;	/* HOLDS EXPIRATION TIME */
	extern struct timeval	time;
	struct k_opt	kopt_space;
	struct k_opt	*kopt= &kopt_space;
	struct nipccb	*socb;
	struct socket 	*so;
	int		s;
	int		mi_timerpopped();
	int		flags=0;	
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* CONVERT FD TO SOCKET */
	if (error = sou_getsock(uap->calldesc, &so)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* VERIFY IT IS A CALL SOCKET */
	socb = (struct nipccb *) (so->so_ipccb);
	if (socb->n_type != NS_CALL){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_NOTCALLSOCKET);
	}

	/* COPY OPTIONS INTO THE KERNEL */
	if (error = mi_getoptions(uap->opt, kopt)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* CHECK IF RECEIVE SIZE OPTION IS SET */
	if (KOPT_DEFINED(kopt, KO_MAX_RECV_SIZE)) {
		recv_size = KOPT_FETCH(kopt, KO_MAX_RECV_SIZE);
		if (recv_size < 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EMSGSIZE);
	    	}
		KOPT_UNDEFINE(kopt, KO_MAX_RECV_SIZE);
	}

	/* CHECK IF SEND SIZE OPTION IS SET */
	if (KOPT_DEFINED(kopt, KO_MAX_SEND_SIZE)) {
		send_size = KOPT_FETCH(kopt, KO_MAX_SEND_SIZE);
		if (send_size < 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EMSGSIZE);
	    	}
		KOPT_UNDEFINE(kopt, KO_MAX_SEND_SIZE);
	}

	/* VERIFY NO OPTIONS WERE OFFERED WHICH WEREN'T USED */
	if (KOPT_ANY_DEFINED(kopt)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_OPTOPTION);
	}

	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
		if (copyin(uap->flags, (caddr_t) &flags, sizeof(int))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
	    	}

	/* VERIFY FLAGS AND OPTIONS */
	if ((flags & NSF_MESSAGE_MODE) && 
#if defined (TRUX) && defined (B1)
		    (!priviledged(SEC_REMOTE, EPERM))
#else
		    (!suser() && u.u_ruid != 0))
#endif
	    {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EACCES);
	    }

	/* VERIFY NO FLAGS WERE OFFERED WHICH WEREN'T USED */
#define LEGAL_FLAGS (NSF_MESSAGE_MODE)
	if ((flags | LEGAL_FLAGS) != LEGAL_FLAGS)
	    {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_FLAGS);
	    }
#undef LEGAL_FLAGS

	/* IF NON-BLOCKING AND NO CONNECTIONS EXIST THEN RETURN */
	if ((so->so_state & SS_NBIO) && (so->so_qlen == 0)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EWOULDBLOCK);
	}

	/* IF A CONNECTION INDICATION EXISTS THEN SKIP TIMER STUFF */
	if (so->so_qlen)
		goto conn_indication;

	/* THE SOCKET IS SYNCHRONOUS 
	 * WAIT FOR A CONNECTION INDICATION, A SIGNAL, OR TIMEOUT 
	 */

	/* START A TIMER */
	if (so->so_timeo) {
		/* BUILD A TIMEVAL STRUCT USING THE SO_TIMEO DELTA */
		atv = &timeout;
		timeout.tv_sec = so->so_timeo/10;
		timeout.tv_usec = (so->so_timeo - (timeout.tv_sec*10))*100000;
		
		/* ADD THE DELTA AND THE CURRENT TIME */
		s = spl6();
		timevaladd(atv, &time);
		splx(s);

		/* START THE TIMER */
		net_timeout(mi_timerpopped, (caddr_t)&so->so_timeo,
			hzto(atv));
	}

	/* SLEEP UNTIL CONNECTION EXISTS OR ERROR.*/
	/* MPNET: we lose our exclusion at the sleep, but it is auto-
	/* magically re-acquired for us on wakeup.
	*/
	while (so->so_qlen == 0 && !error && !so->so_error) {
		error = sleep((caddr_t)&so->so_timeo, PZERO+1 | PCATCH);

		/* CHECK IF TIMER EXPIRED... */			
		s = spl6();
		if (atv && (time.tv_sec > atv->tv_sec || 
		    time.tv_sec == atv->tv_sec && time.tv_usec >= atv->tv_usec)) {
			splx(s);
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EIO);
		}
		splx(s);

	}

	/* IF TIMER SET THEN CANCEL TIMER */
	if (atv)
		net_untimeout(mi_timerpopped, (caddr_t)&so->so_timeo);
	
	/* CHECK IF SIGNAL OCCURED */
	if (error) { 
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EINTR);
	}

	/* CHECK IF PROTOCOL DETECTED AN ERROR */
	if (so->so_error) {
		error = so->so_error;
		so->so_error = 0; /* because BSD does */
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}


conn_indication:

	/* REMOVE THE CONNECTION FROM SO_Q */
	so = so->so_q;
	if (soqremque(so, 1) ==0)
		panic("ipcrecvcn");

	if (flags & NSF_MESSAGE_MODE)  {
		/* MARK THE SOCKET FOR MESSAGE MODE */
		((struct nipccb *) so->so_ipccb)->n_flags |= NF_MESSAGE_MODE;

		/* INFORM THE PROTOCOL TO USE MESSAGE MODE */
		if (error = (*so->so_proto->pr_usrreq)(so,
				PRU_MESSAGE_MODE, (struct mbuf *) 0,
				(struct mbuf *) 0, (struct mbuf *) 0)){
			(void) sou_delete(so, (struct file *) 0);	
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(error);
		}
	}

	/* SET THE BUFFER SIZES TO NETIPC VALUES */
	soreserve(so, (u_long) send_size, (u_long) recv_size);

	/* inform the protocol about the new window sizes */
	if (so->so_proto->pr_flags & PR_WANTRCVD && so->so_pcb) 
		(*so->so_proto->pr_usrreq)(so,PRU_RCVD, 
			(struct mbuf *)0, (struct mbuf *)0, 
			(struct mbuf *)0);


	/* MARK THE SOCKET AS 'NETIPC' CONNECTED */
	((struct nipccb *) (so->so_ipccb))->n_flags |= NF_ISCONNECTED;

	/* CREATE A FILE TABLE ENTRY FOR THE SOCKET */
	if ((fp = falloc()) == 0) {
		(void) sou_delete(so, (struct file *) 0);	
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(u.u_error);
	}

	/* SET THE FIELDS OF THE FILE STRUCTURE */
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_ops = &nipc_socketops;
	fp->f_data = (caddr_t)so;

	/* RETURN A FD TO THE USER */
	if (error = copyout((caddr_t) &u.u_r.r_val1,
			(caddr_t)uap->vcdesc,sizeof(int))) {

		/* PUT THE FD IN THE UARGS AREA */
		u.u_arg[0] = u.u_r.r_val1; 

		/* UNDO ALL THE GOOD WORK WE'VE DONE */
		close();
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */

		NETIPC_RETURN(error);
	}

	/* MARK THE SOCKET AS HAVING A FILE STRUCT. REFERENCING IT */
	so->so_state &= ~SS_NOFDREF; 
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(error);
} /* ipcrecvcn */

 
/*************** beginning of ipcselect stuff *******************************/
/*
 * in memory of jlb..., from billg
 *	byte_o_bits[x] contains 8 bits which are the mirror image of
 *	x (if you place the mirror between 0..3 and 4..7.  This array
 *	is used to speed up bit swapping of ENTIRE, thats right, 32 bit,
 *	integers.  (jlb)
 */
unsigned char byte_o_bits[] = {

0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};
/*
 *	TAKES AN UNSIGNED INTEGER AND RETURNS AN UNSIGNED
 *	INTEGER WITH THE BITS SWAPPED.  THAT IS,  THE MOST
 *	SIGNIFICANT BIT BECOMES THE LEAST SIGNIFICANT BIT.
 */
#define	INTSWAP(in) (byte_o_bits[((in) >> 24)&0xFF] | \
		     (byte_o_bits[((in) >> 16)&0xFF]) << 8 | \
		     (byte_o_bits[((in) >>  8)&0xFF]) <<16 | \
		     (byte_o_bits[((in) >>  0)&0xFF]) <<24)
 
/* PERFORMS BIT SWAPPING ON THE FIRST (NUM / 32) + 1 INTEGERS IN  MAP.  */
#define MAPSWAP(num, map) {					\
	int     	i;						\
	unsigned	*temp= (unsigned *) map;			\
	for (i=num;i>=0; i -= NFDBITS) {				\
		*temp = INTSWAP(*temp);				\
		temp++;						\
	}							\
}

/*************** end of ipcselect macros *******************************/
 
/* NOTE: ipcselect macros are immediately above ^^^^^^^^^^^^^^^^ */

#define SELECT_PERF_SIZE 2	/* >= 64 bits --> use performance path */

struct uap {
	int	*sdbound;
	fd_set	*rmap;
	fd_set	*wmap;
	fd_set	*emap;
	int	timeout;
};

ipcselect()
{
	struct uap *uap= (struct uap *)u.u_ap;

	/* This structure was reduced significantly in size to deal with
	 * stack overflow (UKL experienced this with the select() system
	 * call, and we merely leveraged the fix here).  This array used
	 * to be almost 2k - 1/4 of the kernel stack).  The new scheme is
	 * to handle the performance case here (<= 64 fds) and use another
	 * routine (ipcselect_many) for the cases with many fds.  sf 3/30/92)
	 */
	struct {
		fd_mask fd_bits[SELECT_PERF_SIZE];
	} ibits[3], obits[3];

	struct timeval	*atv=0;
	struct timeval	timer;
	extern struct timeval	time;
	int		mayblock;
	int		ncoll;
	int		unselect();
	int		sdbound;
	int		ni;
	extern int	nselcoll;
	extern int	selwait;
	int		bits_set;
	int		s;
	int		error=0;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* COPYIN THE BOUNDS */
	if (copyin((caddr_t)uap->sdbound, (caddr_t)&sdbound, sizeof(sdbound))){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
	}

	if (sdbound > u.u_maxof)
		sdbound = u.u_maxof;	/* forgiving, if slightly wrong */

	if (sdbound < 0 ) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EBADF);
	}

	if (sdbound > SELECT_PERF_SIZE*NFDBITS) {
		error = ipcselect_many(sdbound, uap);
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* INITIALIZE THE BIT MAPS TO ZEROS */
	bzero((caddr_t)ibits, sizeof(ibits));
	bzero((caddr_t)obits, sizeof(obits));

	/* FIGURE HOW MANY 4 BYTE INTS CAN HOLD THE FD MAPS */
	ni = howmany(sdbound, NFDBITS);

	/* COPYIN THE BIT MAPS */
#define	getbits(name, x)					\
	if (uap->name) {					\
		if(copyin((caddr_t)uap->name,			\
			(caddr_t)&ibits[x],			\
		    (unsigned)(ni*sizeof (fd_mask)))){		\
		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);  \
		    NETIPC_RETURN(EFAULT);			\
		}						\
	}
	getbits(rmap, 0);
	getbits(wmap, 1);
	getbits(emap, 2);
#undef getbits

	/* SWAP THE BITS FROM NETIPC ORDER TO HPUX ORDER */
	MAPSWAP(sdbound,  &ibits[0]);
	MAPSWAP(sdbound,  &ibits[1]);
	MAPSWAP(sdbound,  &ibits[2]);

	/* POSSIBLY CREATE A TIMEVAL STRUCTURE FOR TIMERS */
	if (uap->timeout == -1) {
		/* -1 == NO TIMEOUT, MAY BLOCK i.e. sleep forever */
		mayblock = 1;
	} else if (uap->timeout == 0) {
		/* NO TIMEOUT, MAY NOT BLOCK */
		mayblock = 0;
	} else if (uap->timeout < 0) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_TIMEOUTVALUE);
	} else { 
		/* (UAP->TIMEOUT > 0) */
		mayblock = 1;
		/* BUILD A TIMEVAL STRUCT WITH THE DESIRED EXPIRATION TIME */
		atv = &timer;
		timer.tv_sec = uap->timeout/10;
		timer.tv_usec = (uap->timeout - (timer.tv_sec*10))*100000;
		if (itimerfix(atv)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_TIMEOUTVALUE);
		}
		s = spl6();
		timevaladd(atv, &time);
		splx(s);
	}

retry:
	/* need to check if number of collisions increased */
	ncoll = nselcoll;
	
	/* indicate this process is selecting */
	SPINLOCK(sched_lock);
	u.u_procp->p_flag |= SSEL;
	SPINUNLOCK(sched_lock);

	/* scan the fd bits to see if any are selectable */
	bits_set = selscan((fd_mask *)ibits, (fd_mask *)obits, sdbound);

	/* found some */
	if (bits_set) 
		goto done;

	/* check if selscan found an error */
	if (u.u_error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(u.u_error);
	}

	/* CHECK IF TIMER EXPIRED */			
	s = spl6();
	if (atv && (time.tv_sec > atv->tv_sec || 
	    time.tv_sec == atv->tv_sec && time.tv_usec >= atv->tv_usec)) {
		splx(s);
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EIO);
	}
	splx(s);

	/* if collision occurred or ?? (what clears SSEL??)
         * then retry the select
         */
	SPINLOCK(sched_lock);
	if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~SSEL;
		SPINUNLOCK(sched_lock);
		goto retry;
	}

	 /* clear the SSEL flag, no longer selecting */
	u.u_procp->p_flag &= ~SSEL;

	SPINUNLOCK(sched_lock);

	/* Netipc specifies that 0 timeout == do not sleep */
	if (!mayblock)
		goto done;

	/* POSSIBLY START A TIMER */
	if (atv)
    net_timeout(unselect, (caddr_t)u.u_procp, hzto(atv));


	/* sleep and catch signals */
	error = sleep((caddr_t)&selwait, PZERO+1 | PCATCH);

	/* IF TIMER SET THEN CANCEL TIMER */
	if (atv)
      net_untimeout(unselect, (caddr_t)u.u_procp);

	/* SIGNAL OCCURED */
	if (error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EINTR);
	}

	/* no error from sleep, see if select works now */
	goto retry;


done:
	/* FIGURE THE HIGHEST BIT SET */
	sdbound = mi_gethighbit(ni, &obits[0], &obits[1], &obits[2])+1;

	/* COPYOUT THE HIGHEST BIT */
 	if(copyout((caddr_t)&sdbound, (caddr_t)uap->sdbound, sizeof(sdbound))){
 		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
 		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
 	}

	sdbound--;

	/* SWAP THE BITS FROM HPUX BITORDER TO NETIPC ORDER */
	MAPSWAP(sdbound,  &obits[0]);
	MAPSWAP(sdbound,  &obits[1]);
	MAPSWAP(sdbound,  &obits[2]);

	/* COPYOUT THE BIT MAPS */
#define	putbits(name, x)					\
	if (uap->name) {					\
		if(copyout((caddr_t)&obits[x],			\
			(caddr_t)uap->name,			\
		    (unsigned)(ni*sizeof (fd_mask)))){		\
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);\
			NETIPC_RETURN(EFAULT);			\
		}						\
	}
	if (u.u_error ==0) {
	  putbits(rmap, 0);
	  putbits(wmap, 1);
	  putbits(emap, 2);
	}
#undef putbits

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(error);
}

 
ipcselect_many(sdbound, uap)
int	sdbound;
struct	uap *uap;
{
	fd_set		*ibits, *obits;
	struct timeval	*atv=0;
	struct timeval	timer;
	extern struct timeval	time;
	int		mayblock;
	int		ncoll;
	int		unselect();
	int		ni;
	extern int	nselcoll;
	extern int	selwait;
	int		bits_set;
	int		s;
	int		error=0;

	/* There is a possibility that we may sleep here until memory
	 * is available -- which differs from the original implementation
	 * of ipcselect(2).  However this is ok since there is a possiblity
	 * of the user process being context switched at exit of the
	 * system call anyway.  
	 */

	MALLOC(ibits, fd_set *, 6*sizeof(fd_set), M_IOSYS, M_WAITOK);
	bzero((caddr_t)ibits, 6*sizeof(fd_set));
	obits = &ibits[3];
	
	/* FIGURE HOW MANY 4 BYTE INTS CAN HOLD THE FD MAPS */
	ni = howmany(sdbound, NFDBITS);

	/* COPYIN THE BIT MAPS */
#define	getbits(name, x)					\
	if (uap->name) {					\
		if(copyin((caddr_t)uap->name,			\
			(caddr_t)&ibits[x],			\
		    (unsigned)(ni*sizeof (fd_mask)))){		\
		    error = EFAULT;				\
		    goto error_return;				\
		}						\
	}
	getbits(rmap, 0);
	getbits(wmap, 1);
	getbits(emap, 2);
#undef getbits

	/* SWAP THE BITS FROM NETIPC ORDER TO HPUX ORDER */
	MAPSWAP(sdbound,  &ibits[0]);
	MAPSWAP(sdbound,  &ibits[1]);
	MAPSWAP(sdbound,  &ibits[2]);

	/* POSSIBLY CREATE A TIMEVAL STRUCTURE FOR TIMERS */
	if (uap->timeout == -1) {
		/* -1 == NO TIMEOUT, MAY BLOCK i.e. sleep forever */
		mayblock = 1;
	} else if (uap->timeout == 0) {
		/* NO TIMEOUT, MAY NOT BLOCK */
		mayblock = 0;
	} else if (uap->timeout < 0) {
		error = E_TIMEOUTVALUE;
		goto error_return;
	} else { 
		/* (UAP->TIMEOUT > 0) */
		mayblock = 1;
		/* BUILD A TIMEVAL STRUCT WITH THE DESIRED EXPIRATION TIME */
		atv = &timer;
		timer.tv_sec = uap->timeout/10;
		timer.tv_usec = (uap->timeout - (timer.tv_sec*10))*100000;
		if (itimerfix(atv)) {
			error = E_TIMEOUTVALUE;
			goto error_return;
		}
		s = spl6();
		timevaladd(atv, &time);
		splx(s);
	}

retry:
	/* need to check if number of collisions increased */
	ncoll = nselcoll;
	
	/* indicate this process is selecting */
	SPINLOCK(sched_lock);
	u.u_procp->p_flag |= SSEL;
	SPINUNLOCK(sched_lock);

	/* scan the fd bits to see if any are selectable */
	bits_set = selscan((fd_mask *)ibits, (fd_mask *)obits, sdbound);

	/* found some */
	if (bits_set) 
		goto done;

	/* check if selscan found an error */
	if (u.u_error) {
		error = u.u_error;
		goto error_return;
	}

	/* CHECK IF TIMER EXPIRED */			
	s = spl6();
	if (atv && (time.tv_sec > atv->tv_sec || 
	    time.tv_sec == atv->tv_sec && time.tv_usec >= atv->tv_usec)) {
		splx(s);
		error = EIO;
		goto error_return;
	}
	splx(s);

	/* if collision occurred or ?? (what clears SSEL??)
         * then retry the select
         */
	SPINLOCK(sched_lock);
	if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
		u.u_procp->p_flag &= ~SSEL;
		SPINUNLOCK(sched_lock);
		goto retry;
	}

	 /* clear the SSEL flag, no longer selecting */
	u.u_procp->p_flag &= ~SSEL;

	SPINUNLOCK(sched_lock);

	/* Netipc specifies that 0 timeout == do not sleep */
	if (!mayblock)
		goto done;

	/* POSSIBLY START A TIMER */
	if (atv)
    net_timeout(unselect, (caddr_t)u.u_procp, hzto(atv));


	/* sleep and catch signals */
	error = sleep((caddr_t)&selwait, PZERO+1 | PCATCH);

	/* IF TIMER SET THEN CANCEL TIMER */
	if (atv)
      net_untimeout(unselect, (caddr_t)u.u_procp);

	/* SIGNAL OCCURED */
	if (error) {
		error = EINTR;
		goto error_return;
	}

	/* no error from sleep, see if select works now */
	goto retry;


done:
	/* FIGURE THE HIGHEST BIT SET */
	sdbound = mi_gethighbit(ni, &obits[0], &obits[1], &obits[2])+1;

	/* COPYOUT THE HIGHEST BIT */
 	if(copyout((caddr_t)&sdbound, (caddr_t)uap->sdbound, sizeof(sdbound))){
		error = EFAULT;
		goto error_return;
 	}

	sdbound--;

	/* SWAP THE BITS FROM HPUX BITORDER TO NETIPC ORDER */
	MAPSWAP(sdbound,  &obits[0]);
	MAPSWAP(sdbound,  &obits[1]);
	MAPSWAP(sdbound,  &obits[2]);

	/* COPYOUT THE BIT MAPS */
#define	putbits(name, x)					\
	if (uap->name) {					\
		if(copyout((caddr_t)&obits[x],			\
			(caddr_t)uap->name,			\
		    (unsigned)(ni*sizeof (fd_mask)))){		\
			error = EFAULT;				\
			goto error_return;			\
		}						\
	}
	if (u.u_error ==0) {
	  putbits(rmap, 0);
	  putbits(wmap, 1);
	  putbits(emap, 2);
	}
error_return:
	return(error);
}

ipcsend()
{
	struct a {
		int	vcdesc;
		caddr_t data;
		int	dlen;
		caddr_t flags;
		caddr_t	opt;
	} *uap = (struct a *) u.u_ap;

	int		error=0;
	struct socket	*so;
	int		flags=0;
	struct k_opt	kopt_space;
	struct k_opt	*kopt= &kopt_space;
	struct uio	auio;
	struct iovec	iov_space[MAXIOV];
	struct iovec	*aiov= iov_space;
	int		mi_timerpopped();
	struct nipccb	*socb;
	int		i;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* CONVERT FD TO SOCKET */
	if (error = sou_getsock(uap->vcdesc, &so)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* VERIFY ITS A VC SOCKET */
	socb = (struct nipccb *) so->so_ipccb;
	if (socb->n_type != NS_VC) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(ENOTCONN);
	}

	/* COPY OPTIONS INTO THE KERNEL */
	if (error = mi_getoptions(uap->opt, kopt)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
		if (copyin(uap->flags, (caddr_t) &flags, sizeof(flags))){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
	    	}

	/* CHECI IF UNSUPPORTED FLAGS ARE SET */
#define LEGAL_FLAGS (NSF_MORE_DATA | NSF_VECTORED)
	if ((flags | LEGAL_FLAGS) != LEGAL_FLAGS) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_FLAGS);
	    }
#undef LEGAL_FLAGS

	/* VERIFY THE USER FOLLOWED NETIPC PROTOCOL TO ESTAB. A CONNECTION */
	if ((socb->n_flags & NF_ISCONNECTED) == 0) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_CNCTPENDING);
	}

	if (flags & NSF_VECTORED) {
		/* SET THE UIO OFFSET FIELD */
		if (KOPT_DEFINED(kopt, KO_DATA_OFFSET)) {
		    NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		    /*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_OPTOPTION);
	    	}

		/* SET THE UIO IOVCNT FIELD */
		/* FIRST, VERIFY THE dlen IS A MULTIPLE OF A struct iovec */
		if ((uap->dlen % sizeof(struct iovec)) != 0 ) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_DLEN);
		}
		auio.uio_iovcnt = uap->dlen / sizeof(struct iovec);
		/* NOW VERIFY THE IOVCNT IS NOT TOO LARGE */
		if (auio.uio_iovcnt > MAXIOV) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_TOOMANYVECTS);
		}

		/* COPY IN THE IOV VECTORS */
		if (copyin((caddr_t) uap->data, (caddr_t) aiov,
			    (unsigned)auio.uio_iovcnt*sizeof(iov_space[0]))){
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
		}
		
		/* SET THE UIO IOV POINTER FIELD */
		auio.uio_iov = aiov;
	}
	else { /* NOT VECTORED */
		int offset = 0;
		/* SET THE UIO OFFSET FIELD */
		if (KOPT_DEFINED(kopt, KO_DATA_OFFSET)) {
			offset = KOPT_FETCH(kopt, KO_DATA_OFFSET);
			KOPT_UNDEFINE(kopt, KO_DATA_OFFSET);
		}
		else 
			auio.uio_offset = 0;

		/* SET THE UIO IOVCNT FIELD */
		auio.uio_iovcnt = 1;

		/* BUILD AN IOVEC */
		aiov->iov_base = uap->data + offset;
		aiov->iov_len = uap->dlen;

		/* SET THE UIO IOV POINTER FIELD */
		auio.uio_iov = aiov;
	}

	/* FINISH BUILDING THE UIO  */
	/* SET THE UIO UIO_SET AND UIO_RESID FIELDS */
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = 0;
	auio.uio_offset = 0;

	if (KOPT_ANY_DEFINED(kopt))  {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_OPTOPTION);
	}

	/* VALIDATE THE IOVEC AND COUNT BYTES TO SEND */
	for (i=0; i < auio.uio_iovcnt ; i++ , aiov++) {
		/* NEGATIVE LENGTHS ARE ILLEGAL */
		if (aiov->iov_len < 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(E_VECTCOUNT);
	    	}
		
		/* ZERO LENGTH VECTORS ARE OK */
		if (aiov->iov_len == 0)
			continue;

		/* VERIFY THE SPECIFIED BYTES ARE READABLE */
		if(useracc(aiov->iov_base, (u_int)aiov->iov_len, B_READ) == 0) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
	    	}
		
		/* INCREMENT THE TOTAL BYTE COUNT */
		auio.uio_resid += aiov->iov_len;
	}

	/* SEND THE DATA */
	error = sou_send(so, &auio, flags);

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(error);
}



ipcsetnodename()

{
	/* set up pointers to caller's parameters */
	 struct a {
		caddr_t nodename;	/* nodename to be assigned */
		int	nodelen;	/* byte length of nodename */
	} *uap = (struct a *) u.u_ap;

	char   	knodename[NS_MAX_NODE_NAME];    /* kernel nodename */
	char	*name = knodename;		/* pointer to name */
	int	parts;				/* # of parts in name */
	int	error=0;			/* local error     */
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/* verify that the caller is super-user */
#if defined (TRUX) && defined (B1)
	if (!priviledged(SEC_SYSATTR, EPERM)
#else
	if (!suser() && u.u_ruid != 0)
#endif
		NETIPC_RETURN(EPERM);

	/* verify the node name length is in bounds */
	if (uap->nodelen > NS_MAX_NODE_NAME || uap->nodelen <= 0) {
		NETIPC_RETURN(E_NLEN);
	}
 
 	/*MPNET: turn MP protection on */
 	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* copy the nodename parameter into the kernel */
	if (copyin(uap->nodename, (caddr_t) knodename, uap->nodelen)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(EFAULT);
	}

	/* ensure the name has proper syntax */
	if (nm_isinvalid(knodename, (uap->nodelen))) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_NODENAMESYNTAX);
	}

	/* verify the name is fully qualified; i.e. it has 3 parts */
	for ( parts = 1; name < &knodename[uap->nodelen]; name++)
		if (*name == '.')
			parts++;
	if (parts != 3) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_NODENAMESYNTAX);
	}

	/* if this is really a name change then store the new nodename in  */
	/* the global nodename variable and send a probe unsolicited reply */
	if (!nm_islocal(knodename, uap->nodelen)) {
		bcopy(knodename, nipc_nodename, uap->nodelen);
		nipc_nodename_len = uap->nodelen;
		(void) prb_unsol((struct ifnet *)0);
	}

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(0);

}  /* setnodename */

 

ipcshutdown()
{
	struct a {
		int	descriptor;
		caddr_t	flags;
		caddr_t	opt;
	} *uap = (struct a *) u.u_ap;

	struct socket	*so;		/* pointer to socket */
	struct file	*fp;		/* pointer to a file */
	int		flags=0;
	int		error=0;
	int		s;
	struct k_opt	kopt_space;
	struct k_opt	*kopt= &kopt_space;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	/* COPY FLAGS INTO THE KERNEL */
	if (uap->flags) 
		if (copyin(uap->flags, (caddr_t) &flags, sizeof(flags))) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			NETIPC_RETURN(EFAULT);
	    	}

	/* CHECK FOR UNSUPPORTED FLAGS */
#define LEGAL_FLAGS (NSF_GRACEFUL_RELEASE)
	if ((flags | LEGAL_FLAGS) != LEGAL_FLAGS) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_FLAGS);
	    }
#undef LEGAL_FLAGS

	/* COPY OPTIONS INTO THE KERNEL */
	if (error = mi_getoptions(uap->opt, kopt)){
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* CHECK FOR UNSUPPORTED OPTIONS */
	if (KOPT_ANY_DEFINED(kopt))  {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(E_OPTOPTION);
	}


	/* INSURE THE DESCRIPTOR REFERS TO A NETIPC SOCKET */
	if (error = sou_getsock(uap->descriptor, &so)) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		NETIPC_RETURN(error);
	}

	/* FOLLOW THE FILE DESCRIPTOR AND GET A FILE POINTER */
	fp = getf(uap->descriptor);

	/* FREE THE FILE DESCRIPTOR AND DECREMENT THE F_COUNT */
	/* MPNET Rule # 1A */
	uffree(uap->descriptor);

	/* IF NO OTHER REFERENCES TO THIS FILE THEN DELETE IT */
	if (fp->f_count == 1) {

		if (flags & NSF_GRACEFUL_RELEASE) {

			/* IF NOT A VC SOCKET THEN ABORT IT*/
			if (((struct nipccb *)so->so_ipccb)->n_type != NS_VC){
				sou_delete(so, fp);
	    		}
			else {

				/* REMOVE NETIPC MEMORY FROM THIS SOCKET */
				sou_clean(so,fp);

				/* INSURE THE SOCKET MEMORY IS FREED */
				so->so_state |= SS_NOFDREF;

				/* INITIATE DISCONNECTION */
				soisdisconnecting(so);

				/* INFORM THE PROTOCOL */
				/* PROTOCOL WILL INITIATE FINAL DEALLOCATION */
				(void) (*so->so_proto->pr_usrreq)(so,
					PRU_DETACH,
					(struct mbuf *) 0, /* no xxx */
					(struct mbuf *) 0, /* no nam */
					(struct mbuf *) 0);  /* no rights */ 
			}
		}
		else {
			/* NOT GRACEFUL, ABORT THE SOCKET */
			sou_delete(so, fp);
		}

		/* now free the credentials structure */
		crfree(fp->f_cred);
		fp->f_cred = NULL;

#if defined(hp9000s800) && !defined(_WSIO)
		/* Decrement # of active file table entries; sar(1) support */
		MPCNTRS.activefiles--;          /* from UKL for sar(1) */
#endif	/* hp900s800 && ! _WSIO */

	}

	SPINLOCK(file_table_lock);
	fp->f_count--;
	SPINUNLOCK(file_table_lock);

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */

	NETIPC_RETURN(0);
}
