/*
 * $Header: nipc_sou.c,v 1.8.83.5 93/10/27 13:19:59 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_sou.c,v $
 * $Revision: 1.8.83.5 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/27 13:19:59 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_sou.c $Revision: 1.8.83.5 $";
#endif

/* 
 * Netipc routines called by system calls (mostly).
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
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../nipc/nipc_cb.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_protosw.h"


/* REMOVE ALL NETIPC RELATED MEMORY FROM A SOCKET */
sou_clean(so, fp)
struct socket *so;
struct file	*fp;
{
	struct nipccb	*socb;
	
	if (!so->so_ipccb)
		return;

	/* POINT TO THE NETIPC CONTROL BLOCK */
	socb = (struct nipccb *) so->so_ipccb;

	/* FREE THE CONNECT SITE PATH REPORT */
	m_freem(socb->n_cspr);

	/* FREE ANY NAMES FOR THIS SOCKET */
	if (socb->n_name || socb->n_flags & NF_COLLISION) {
		if (!fp)
			panic("sou_clean");
		sr_sodelete(socb->n_name, fp);
	}


	/* FREE THE NIPCCB */
	FREE(so->so_ipccb, M_NIPCCB);
	mbstat.m_mtypes[MT_IPCCB]--;

	/* BE TIDY */
	so->so_ipccb = 0; 
}
 
/*
 * WHEN AN ATTEMPT AT A NEW CONNECTION IS NOTED ON A SOCKET
 * WHICH ACCEPTS CONNECTIONS, SONEWCONN IS CALLED.  iF THE
 * CONNECTION IS POSSIBLE (SUBJECT TO SPACE CONSTRAINTS, ETC.)
 * THEN WE ALLOCATE A NEW STRUCTURE, PROPOERLY LINKED INTO THE
 * DATA STRUCTURE OF THE ORIGINAL SOCKET, AND RETURN THIS.
 */
struct socket *
sou_connind(head)
	struct socket *head;
{
	struct socket	*so;
	struct nipccb		*socb;

	if (head->so_qlen >= head->so_qlimit)
		return((struct socket *)0);

	/* ALLOCATE A NIPCCB */
	MALLOC(socb, struct nipccb *, sizeof(struct nipccb), M_NIPCCB,M_NOWAIT);
	if (!socb) 
		return((struct socket *)0);
	bzero((caddr_t)socb, sizeof(struct nipccb));
	mbstat.m_mtypes[MT_IPCCB]++;

	/* INITIALIZE THE NIPCCB */
	socb->n_type = NS_VC;
	socb->n_protosw = ((struct nipccb *) head->so_ipccb)->n_protosw;
	socb->n_recv_thresh = NIPC_DEFAULT_RECV_THRESHOLD;
	socb->n_send_thresh = NIPC_DEFAULT_SEND_THRESHOLD;

	/* ALLOCATE A SOCKET */
	MALLOC(so, struct socket *, sizeof(struct socket),M_SOCKET,M_NOWAIT);
	if (!so) {
		FREE(socb, M_NIPCCB);
		mbstat.m_mtypes[MT_IPCCB]--;
		return((struct socket *)0);
	}
	bzero((caddr_t)so, sizeof(struct socket));
	mbstat.m_mtypes[MT_SOCKET]++;

	/* INITIALIZE THE SOCKET */
        so->so_rcv.sb_so = so;
        so->so_snd.sb_so = so;
	so->so_ipc	= SO_NETIPC;
	so->so_ipccb	= (caddr_t) socb;
	so->so_connindication = (int (*)()) sou_connind;
	so->so_type = head->so_type;
	so->so_rcv.sb_mbmax = NIPC_DEFAULT_MAX_RECV_SIZE;
	so->so_snd.sb_mbmax = NIPC_DEFAULT_MAX_SEND_SIZE;
	so->so_options = head->so_options & ~SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;

	/* ADD TO SO_Q0 */
	sou_qinsque(head, so, 0);

	/* CALL THE PROTOCOL TO ATTACH TO THE NEW SOCKET */
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0)) {
		sou_delete(so, (struct file *) 0);
		return((struct socket *)0);
	}

	/* CALL THE PROTOCOL TO ACCEPT THE CONNECTION INDICATION */
	if ((*so->so_proto->pr_usrreq)(so, PRU_ACCEPTCONN,
				(struct mbuf *) 0,
				(struct mbuf *) 0,
				(struct mbuf *) 0)) {
		/* protocol error accepting socket */
		sou_delete(so, (struct file *) 0);
		return((struct socket *)0);
	}

	/* set the buffer sizes to 0 until ipcrecvcn */
	(void) soreserve(so, 0, 0);

	return (so);
}

 
struct socket *
sou_create(proto)
struct nipc_protosw	*proto;
{
	struct nipccb	*socb;
	struct socket	*so;
	struct socket	*sou_connind();

	/* ALLOCATE A NIPCCB */
	MALLOC(socb, struct nipccb *, sizeof(struct nipccb), M_NIPCCB,M_WAITOK);
	bzero((caddr_t)socb, sizeof(struct nipccb));
	mbstat.m_mtypes[MT_IPCCB]++;

	/* INITIALIZE THE NIPCCB */
	socb->n_type = proto->np_type;
	socb->n_protosw = proto;
	socb->n_recv_thresh = NIPC_DEFAULT_RECV_THRESHOLD;
	socb->n_send_thresh = NIPC_DEFAULT_SEND_THRESHOLD;

	/* ALLOCATE A SOCKET */
	MALLOC(so, struct socket *, sizeof(struct socket),M_SOCKET,M_WAITOK);
	bzero((caddr_t)so, sizeof(struct socket));
	mbstat.m_mtypes[MT_SOCKET]++;

	/* INITIALIZE GENERIC SOCKET FIELDS 
	 * Netipc default max send and receive sizes must be set
	 * after the protocol has been called to PRU_ATTACH or they will
	 * be overwritten by the protocol.
	 */
        so->so_rcv.sb_so = so;
        so->so_snd.sb_so = so;
	so->so_ipc	= SO_NETIPC;
	so->so_ipccb	= (caddr_t) socb;
	so->so_connindication = (int (*)()) sou_connind;
	so->so_timeo	= NIPC_DEFAULT_TIMEOUT;
	so->so_type	= proto->np_bsd_protosw->pr_type;
	so->so_options	= SO_KEEPALIVE;
	so->so_qlimit	= NIPC_DEFAULT_CONN_REQ_BACK;
	so->so_proto	= proto->np_bsd_protosw;

	/* DO SOCKET_TYPE SPECIFIC INITIALIZATION */
	switch (proto->np_type) {
	case NS_CALL:
		so->so_options |= SO_ACCEPTCONN;
		so->so_q = so;
		so->so_q0 = so;
		break;
	case NS_REQUEST:
	case NS_REPLY:
	case NS_VC: 
		/* leave as 0 */
		break;
	default:
		panic("sou_create: np_type is unknown");
	}

	/* RETURN A POINTER TO THE NEW SOCKET */
	return(so);
}

sou_delete(so, fp)
struct socket	*so;
struct file 	*fp;
{

	/* IF CALL SOCKET DELETE ALL SOCKETS IN ITS QUEUES */
	if (so->so_options & SO_ACCEPTCONN) {

		/* CLEAN THE LIST OF PARTIAL CONNECTIONS */
		while (so->so_q0 != so) {
			/* FREE NETIPC RELATED STUFF */
			sou_clean(so->so_q0, (struct file *) 0);

			/* INFORM THE PROTOCOL TO ABORT */
			so->so_proto->pr_usrreq(so->so_q0, PRU_ABORT,
				(struct mbuf *) 0, /* no message */
				(struct mbuf *) 0, /* no address */
				(struct mbuf *) 0); /* no rights */
		}

		/* CLEAN THE LIST OF ESTABLISHED CONNECTIONS */
		while (so->so_q != so) {
			/* FREE NETIPC RELATED STUFF */
			sou_clean(so->so_q, (struct file *) 0);

			/* INFORM THE PROTOCOL TO ABORT */
			so->so_proto->pr_usrreq(so->so_q, PRU_ABORT,
				(struct mbuf *) 0, /* no message */
				(struct mbuf *) 0, /* no address */
				(struct mbuf *) 0); /* no rights */
		}
	}

	/* FREE NETIPC RELATED STUFF FROM THE SOCKET */
	sou_clean(so, fp);

	/* INSURE THE SOCKET MEMORY IS FREED */
	so->so_state |= SS_NOFDREF;

	/* FREE SOCKET */
	/* CHECK FOR PCB BECAUSE PRU_ATTACH MAY HAVE FAILED */
	if (so->so_pcb)
		(void) so->so_proto->pr_usrreq(so, PRU_ABORT,
			(struct mbuf *) 0, /* no message */
			(struct mbuf *) 0, /* no address */
			(struct mbuf *) 0); /* no rights */
	else
		sofree(so);

}



struct mbuf *
sou_sbdqmsg(sb, msgid)
struct sockbuf *sb;	/* socket buffer to dequeue message from  */
int msgid;			/* msgid of message to dequeue;  if zero, */
				/* then dequeue the first message	  */

{
	struct mbuf *m, *mn, *mprev = 0;
	int	*msg;

	m = sb->sb_mb;
	if (msgid == 0) {
		/* dequeue the first message */
		if (m) 
			sb->sb_mb = m->m_act;
	} else {
		/* find the message w/msgid */
		for ( ; (m); m = m->m_act) {
			msg = mtod(m, int *);
			if (*msg == msgid)
				break;
			mprev = m;
		}
		/* dequeue the message */
		if (m) 
			if (mprev)
				mprev->m_act = m->m_act;
			else
				sb->sb_mb = m->m_act;
	}

	/* adjust sockbuf resource count for dequeued message */
	for (mn = m; (mn); mn = mn->m_next)
		sbfree(sb, mn);

	return (m);
}
 
/* FOLLOW FILE DESCRIPTOR TO A NETIPC SOCKET */
sou_getsock(fd, sop)
int		fd;
struct socket	**sop;
{
	struct file *fp;
	
	/* HAVE THE FILE SYSTEM DO THE INITIAL FOLLOW */
	fp = getf(fd);

	/* CHECK FOR FAILURE DUE TO BAD FILE DESCRIPTOR */
	if (fp == NULL)
		return(EBADF);

	/* INSURE THE FILE REALLY REFERS TO A SOCKET */
	if (fp->f_type != DTYPE_SOCKET)
		return(ENOTSOCK);

	/* INSURE ITS A NETIPC SOCKET */
	*sop = (struct socket *) fp->f_data;
	if ((*sop)->so_ipc != SO_NETIPC)
		return(ENOTSOCK);

	/* NO ERROR */
	return(0);
}


/* append a socket at the TAIL of a queue */
sou_qinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{
	struct socket *temp;

	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;

		/* goto last socket in list */
		temp = head;
		while (temp->so_q0 != head)
			temp = temp->so_q0;


		so->so_q0 = head->so_q0;
		head->so_q0 = so;
	} else {
		head->so_qlen++;

		/* goto last socket in list */
		temp = head;
		while (temp->so_q != head)
			temp = temp->so_q;

		/* have last socket in the queue point to this socket */
		so->so_q = head->so_q;
		head->so_q = so;
	}
}

 
/* COPY DATA FROM A SOCKBUF INTO USER SPACE */
sou_receive(so,auio,flags,eom)
struct socket	*so;
struct uio	*auio;
int		flags;
int		*eom;
{
	struct mbuf	*m;
	int		len;
	struct mbuf	*nextrecord=0;
	int		error=0;
	struct timeval	timeout;
	extern struct timeval	time;
	struct timeval	*atv=0;
	int		s;
	int		mi_timerpopped();
	void		nipc_sbmsgsize();

	/* DON'T ALLOW A REQUEST BIGGER THAN THE BUFFER WILL HOLD */
	if (auio->uio_resid > so->so_rcv.sb_hiwat)  {
		return(EMSGSIZE);
	}

	/* IF NO DATA IS ASKED FOR THEN DONE */
	if (auio->uio_resid == 0) {
		return(0);
	}

	/* 
	 * BUILD A TIMEVAL STRUCT WITH THE DESIRED EXPIRATION TIME 
	 */
	if (so->so_timeo && !(so->so_state & SS_NBIO)) {
		atv = &timeout;
		timeout.tv_sec = so->so_timeo/10;
		timeout.tv_usec = (so->so_timeo - (timeout.tv_sec*10))*100000;

		/* ADD THE DELTAL AND THE CURRENT TIME */
		s = spl7();
		timevaladd(atv, &time);
		splx(s);
	}

	/* LOOP UNTIL AN ERROR OCCURS OR ENOUGH DATA ARRIVES */
restart:

	/* LOCK THE SOCKBUF */
	if ((so->so_rcv.sb_flags & SB_LOCK) == 0) {
		so->so_rcv.sb_flags |= SB_LOCK;
	}
	else {
		/* IF NON-BLOCKING THEN NEVER SLEEP */
		if (so->so_state & SS_NBIO) {
			/* RETURN (INSTEAD OF GOTO DONE) TO AVOID
			 * THE SBUNLOCK WHICH IS THERE.
			 */
			return(EWOULDBLOCK);
		}

		/* NOT NBIO , OK TO SLEEP */

		/* START A TIMER SO WE DON'T SLEEP TOO LONG */
		if (atv)
			net_timeout(mi_timerpopped,
				(caddr_t)&so->so_rcv.sb_flags,
				hzto(atv));

		/* SLEEP UNTIL THE SOCKBUF CAN BE LOCKED OR ERROR */
		/* LIKE SBLOCK, SLEEP ON SB_FLAGS */
		while (so->so_rcv.sb_flags & SB_LOCK && !error) {
			so->so_rcv.sb_flags |= SB_WANT;
			error =sleep((caddr_t)&so->so_rcv.sb_flags,
					PZERO+1|PCATCH);
		}

		/* IF TIMER SET THEN CANCEL TIMER */
		if (atv)
			net_untimeout(mi_timerpopped,
			    (caddr_t)&so->so_rcv.sb_flags);

		/* CHECK IF SIGNAL OCCURED */
		if (error) return(EINTR);

		so->so_snd.sb_flags |= SB_LOCK;
	}

	/* CHECK IF PROTOCOL DETECTED AN ERROR */
	if (so->so_error) {
		error = so->so_error;
		goto done;
	}

	/* check if peer gracefully closed i.e. sent fin instead of reset */
	if (so->so_state & SS_CANTRCVMORE) {
		error = EMLINK;
		goto done;
	}

	/* CHECK IF TIMER EXPIRED */			
	s = spl7();	
	if (atv && (time.tv_sec > atv->tv_sec || 
	    time.tv_sec == atv->tv_sec && time.tv_usec >= atv->tv_usec)) {
		error = EIO;
		splx(s);
		goto done;
	}
	splx(s);

	/* IMPLEMENT THE WAITING FOR DATA TABLE IN THE DESIGN DOCUMENT */
	/* IF NON-BLOCKING THEN EITHER DATA ARRIVED OR QUIT */
	if (so->so_state & SS_NBIO) {
		/* IF (NBIO AND NOT NSF_DATA_WAIT AND SB_CC == 0) OR
		 * IF (NBIO AND NSF_DATA_WAIT AND SB_CC < RESID) THEN
		 * RETURN EWOULDBLOCK 
		 * ELSE ENOUGH DATA HAS ARRIVED 
		 */
		if (!(flags & NSF_DATA_WAIT)) {
			if (so->so_rcv.sb_cc == 0) {
				error = EWOULDBLOCK;
				goto done;
			}
			/* data_arrived; */
		} else {
			if (so->so_rcv.sb_cc < auio->uio_resid) {
				error = EWOULDBLOCK;
				goto done;
			}
			/* data_arrived; */
		}
	} else if (so->so_rcv.sb_cc && so->so_rcv.sb_flags & SB_MSGCOMPLETE) {
	     /* do nothing here, fall out of loop */
	} else  /* IT'S OK TO BLOCK SO EITHER DATA ARRIVED OR SLEEP */
		/* IF (NOT NSF_DATA_WAIT AND NO DATA HAS ARRIVED) OR
		 * IF (NSF_DATA_WAIT AND DATA IS LESS THAN REQUESTED) THEN
		 * WAIT FOR (more) DATA TO ARRIVE
		 * ELSE ENOUGH DATA HAS ARRIVED 
		 */
	    if ((((flags & NSF_DATA_WAIT)==0) && (so->so_rcv.sb_cc == 0))
				 ||
	      ((flags&NSF_DATA_WAIT)&&(so->so_rcv.sb_cc<auio->uio_resid))){

			/* WAIT FOR DATA */
			/* LIKE SBWAIT, SLEEP ON SB_CC */
			sbunlock(&so->so_rcv);
			so->so_rcv.sb_flags |= SB_WAIT;
			if (atv) { /* START TIMER */
				net_timeout(mi_timerpopped, 
					(caddr_t)&so->so_rcv.sb_cc, 
					 hzto(atv));
				/* SLEEP */
				error =sleep((caddr_t)&so->so_rcv.sb_cc, 
						PZERO+1 | PCATCH);
				/* CANCEL TIMER */
				net_untimeout(mi_timerpopped,
					(caddr_t)&so->so_rcv.sb_cc);
			}
			else 	/* SLEEP WITHOUT A TIMER */
				error =sleep((caddr_t)&so->so_rcv.sb_cc, 
						PZERO+1 | PCATCH);

			if (error) return(EINTR);

			goto restart;
	}

	/* GET EOM FLAG */
	nipc_sbmsgsize(&so->so_rcv, 0, auio->uio_resid,
		&len, eom);

	/* update accounting information */
#if defined(__hp9000s800) && !defined(_WSIO)
	u.u_procp->p_rusagep->ru_msgrcv++;      /* Series 800 only */
#else /* !Series 800 */
	u.u_ru.ru_msgrcv++;
#endif /* Series 800 */

	/* COPY DATA INTO USER SPACE, START WITH THE FIRST MBUF  */
	m = so->so_rcv.sb_mb;

	/* KEEP POINTER TO NEXT MESSAGE */
	nextrecord = m->m_act; 

	while (!error && m && auio->uio_resid) {

		/* DONT MOVE MORE THAN AN MBUF AT A TIME */
		len = MIN(auio->uio_resid, m->m_len);

		/* MOVE THE DATA */
		error = uiomove(mtod(m, caddr_t), (int) len, UIO_READ, auio);

		/* MAYBE GO TO NEXT MBUF */
		if (len == m->m_len) {
			/* ALL OF MBUF WAS COPIED TO USER SPACE */
			if (flags & MSG_PEEK) {
				m = m->m_next;
			} else {
				nextrecord = m->m_act;
				sbfree(&so->so_rcv, m);
				so->so_rcv.sb_mb = m_free(m);
				m = so->so_rcv.sb_mb;
				if (m)
					m->m_act = nextrecord;
			}
		} else {
			/* DATA STILL LEFT IN MBUF */
			if (!(flags & MSG_PEEK)) {
				m->m_off += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
	} /* end while */

	/* FINISHED MOVING DATA, SET THE END OF MESSAGE OUTPUT FLAG */
	/* POSSIBLY ADJUST POINTER TO POINT TO NEXT MESSAGE */
	/* POSSIBLY INFORM THE PROTOCOL DATA WAS REMOVED */
	if (!(flags & MSG_PEEK)) {
		/* UPDATE SOCKBUF POINTER */
		if (m == 0) 
			so->so_rcv.sb_mb = nextrecord;

		/* POSSIBLY INFORM THE PROTOCOL DATA WAS REMOVED */
		if (so->so_proto->pr_flags & PR_WANTRCVD && so->so_pcb) 
			(*so->so_proto->pr_usrreq)(so,PRU_RCVD, 
				(struct mbuf *)0, (struct mbuf *)0, 
				(struct mbuf *)0);
	}
done:
	sbunlock(&so->so_rcv);
	return(error);
}

 
sou_send(so, auio, flags)
struct socket 	*so;
struct uio	*auio;
int		flags;
{
	
	struct mbuf	*m;
	int		len;
	struct mbuf	*top=0;
	struct mbuf	**mp;
	int		error=0;
	struct timeval	timeout;
	extern struct timeval	time;
	struct timeval	*atv=0;
	int		s;
	int		space;
	void		nipc_sbappendmsg();

	/* IF RESID > MAX BUFFERABLE SIZE THEN ERROR */
	if (auio->uio_resid > so->so_snd.sb_hiwat)  {
		return(EMSGSIZE);
	}

	/* IF NO DATA TO SEND THEN DONE */
	if (auio->uio_resid == 0) {
		return(0);
	}

	/* IF A TIME VALUE EXISTS THEN
	 * BUILD A TIMEVAL STRUCT WITH THE DESIRED EXPIRATION TIME
	 */
	if (so->so_timeo) {
		atv = &timeout;
		timeout.tv_sec = so->so_timeo/10;
		timeout.tv_usec = (so->so_timeo - (timeout.tv_sec*10))*100000;
		if (itimerfix(atv)) {
			panic("sou_send: bad so_timeo");
		}
		s = spl7();
		timevaladd(atv, &time);
		splx(s);
	}

	/* LOOP UNTIL AN ERROR OCCURS OR ENOUGH SPACE EXISTS */
restart:

	/* LOCK THE SOCKETBUF */
	if ((so->so_snd.sb_flags & SB_LOCK) == 0) {
		so->so_snd.sb_flags |= SB_LOCK;
	}
	else {
		if (so->so_state & SS_NBIO) {
			return(EWOULDBLOCK);
		}

		/* START A TIMER SO WE DON'T SLEEP TOO LONG */
		if (atv)
			net_timeout(mi_timerpopped,
				(caddr_t)&so->so_snd.sb_flags, 
				hzto(atv));

		/* SLEEP UNTIL THE SOCKBUF CAN BE LOCKED */
		while (so->so_snd.sb_flags & SB_LOCK && !error) {
			so->so_snd.sb_flags |= SB_WANT;
			error = sleep((caddr_t)&so->so_snd.sb_flags, 
					PZERO+1 | PCATCH);
		}

		/* IF TIMER SET THEN CANCEL TIMER */
		if (atv)
			net_untimeout(mi_timerpopped,
				(caddr_t)&so->so_snd.sb_flags);

		/* CHECK IF SIGNAL OCCURED */
		if (error) return(EINTR);

		so->so_snd.sb_flags |= SB_LOCK;
	}

	/* CHECK IF PROTOCOL DETECTED AN ERROR */
	if (so->so_error) {
		error = so->so_error;
		goto done;
	}

	/* CHECK IF TIMER EXPIRED */			
	s = spl7();	
	if (atv && (time.tv_sec > atv->tv_sec || 
	    time.tv_sec == atv->tv_sec && time.tv_usec >= atv->tv_usec)) {
		error = EIO;
		splx(s); 
		goto done;
	}
	splx(s); 

	/* GET THE CURRENT AMOUNT OF SPACE IN THE SOCKBUF */
	space = sbspace(&so->so_snd);

	/* CHECK IF NEEDED SPACE EXISTS YET */
	if (space < auio->uio_resid) {
		if (so->so_state & SS_NBIO) {
			error = EWOULDBLOCK;
			goto done;
		}

		/* WAIT FOR SPACE */
		sbunlock(&so->so_snd);
		so->so_snd.sb_flags |= SB_WAIT;
		if (atv) { /* START TIMER */
			net_timeout(mi_timerpopped, 
				(caddr_t)&so->so_snd.sb_cc, 
				hzto(atv));
			/* SLEEP */
			error = sleep((caddr_t)&so->so_snd.sb_cc, 
					PZERO+1 | PCATCH);
			/* CANCEL TIMER */
			net_untimeout(mi_timerpopped,
				(caddr_t)&so->so_snd.sb_cc);
		}
		else 	/* SLEEP WITHOUT A TIMER */
			error = sleep((caddr_t)&so->so_snd.sb_cc, 
					PZERO+1 | PCATCH);

		if (error) return(EINTR);

		goto restart;
	}

	/* POINT TO THE FIRST "m_next" POINTER OF THE MBUF CHAIN */
	mp = &top;

	/* update accounting information */
#if defined(__hp9000s800) && !defined(_WSIO)
	u.u_procp->p_rusagep->ru_msgsnd++;      /* Series 800 only */
#else /* !Series 800 */
	u.u_ru.ru_msgsnd++;
#endif /* Series 800 */


	/* COPY DATA FROM USER SPACE */
	while (auio->uio_resid) {
		/* GET AN MBUF OR CLUSTER */
		m = m_get(M_WAIT, MT_DATA);
		if (auio->uio_resid >= MCLBYTES / 2 && space >= MCLBYTES) {
			MCLGET(m);
			if (m->m_len != MCLBYTES)
				goto nopages; /* because BSD does :-) */
			len = MIN(MCLBYTES, auio->uio_resid);
		} else {
nopages:
			len = MIN(MIN(MLEN, auio->uio_resid), space);
		}

		/* MOVE THE DATA */
		error = uiomove(mtod(m, caddr_t), len, UIO_WRITE, auio);
		m->m_len = len;

		/* SET THE LAST m_next FIELD IN THE MBUF CHAIN TO THIS MBUF */
		*mp = m;

		/* IF UIOMOVE FAILED THEN STOP MOVING DATA */
		if (error)
			goto done;

		/* SET mp TO POINT TO THE LAST m_next FIELD IN THE MBUF CHAIN */
		mp = &m->m_next;
	}

	/* FINISHED MOVING DATA FROM USER SPACE */
	/* APPEND IT TO THE SEND SOCKBUF AND CALL THE PROTOCOL */

	if (((struct nipccb *)(so->so_ipccb))->n_flags & NF_MESSAGE_MODE)
		nipc_sbappendmsg(&so->so_snd, top, !(flags & NSF_MORE_DATA));
	else
		sbappend(&so->so_snd, top);
	error = (*so->so_proto->pr_usrreq)(so, PRU_SEND,
			(caddr_t) 0, (caddr_t) 0, (caddr_t) 0);
	/* CALLED PROTOCOL AND PASSED MBUF CHAIN SO THEY MUST FREE IT */
	top = 0;	/* DON'T FREE THE MBUF CHAIN */

done:
	sbunlock(&so->so_snd);
	if (top)
		m_freem(top);
	return(error);
}
