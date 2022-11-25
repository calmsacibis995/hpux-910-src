/*
 * $Header: uipc_socket2.c,v 1.8.83.3 93/09/17 20:14:33 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_socket2.c,v $
 * $Revision: 1.8.83.3 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 20:14:33 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_socket2.c $Revision: 1.8.83.3 $ $Date: 93/09/17 20:14:33 $";
#endif

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)uipc_socket2.c	7.5 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/un.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/domain.h"                                                 /*HP*/
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/netfunc.h"
#include "../net/netmp.h"
#include "../h/sem_alpha.h"

/*
 * Primitive routines for operating on sockets and socket buffers
 */

/*
 * Procedures to manipulate state flags of socket
 * and do appropriate wakeups.  Normal sequence from the
 * active (originating) side is that soisconnecting() is
 * called during processing of connect() call,
 * resulting in an eventual call to soisconnected() if/when the
 * connection is established.  When the connection is torn down
 * soisdisconnecting() is called during processing of disconnect() call,
 * and soisdisconnected() is called when the connection to the peer
 * is totally severed.  The semantics of these routines are such that
 * connectionless protocols can call soisconnected() and soisdisconnected()
 * only, bypassing the in-progress calls when setting up a ``connection''
 * takes no time.
 *
 * From the passive side, a socket is created with
 * two queues of sockets: so_q0 for connections in progress
 * and so_q for connections already made and awaiting user acceptance.
 * As a protocol is preparing incoming connections, it creates a socket
 * structure queued on so_q0 by calling sonewconn().  When the connection
 * is established, soisconnected() is called, and transfers the
 * socket structure to so_q, making it available to accept().
 * 
 * If a socket is closed with sockets on either
 * so_q0 or so_q, these sockets are dropped.
 *
 * If higher level protocols are implemented in
 * the kernel, the wakeups done here will sometimes
 * cause software-interrupt process scheduling.
 */

#ifdef SEMAPHORE_DEBUG
extern int netmp_current_pmodel;
#define NETSKT_ASSERT_SEMA(invariant, message)	\
	if (netmp_current_pmodel == NETMP_THO) \
		MP_ASSERT(invariant,message)
#else	/* ! SEMAPHORE_DEBUG */
#define NETSKT_ASSERT_SEMA(invariant, message)
#endif /* SEMAPHORE_DEBUG */

soisconnecting(so)
	register struct socket *so;
{
	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soisconnecting: not holding networking sema when should be");
	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
	wakeup((caddr_t)&so->so_timeo);
}

soisconnected(so)
	register struct socket *so;
{
	register struct socket *head = so->so_head;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soisconnected: not holding networking sema when should be");
	if (head) {
		if (soqremque(so, 0) == 0)
			panic("soisconnected");
		soqinsque(head, so, 1);
		sorwakeup(head);
		wakeup((caddr_t)&head->so_timeo);
	}
	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTED;
	wakeup((caddr_t)&so->so_timeo);
	sorwakeup(so);
	sowwakeup(so);
}

soisdisconnecting(so)
	register struct socket *so;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soisdisconnecting: not holding networking sema when should be");
	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

soisdisconnected(so)
	register struct socket *so;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soisdisconnected: not holding networking sema when should be");
	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

/*
 * When a connection indication is received by a socket which is capable
  * of accepting connections and uses IPC switch, soconnindication is called.
  * If the connection is possible (subject to space constraints, etc.)
  * then we allocate a new structure, propoerly linked into the
  * data structure of the original socket, return it, and call
  * the protocol with PRU_ACCEPTCONN.
  */
struct socket *
soconnindication(head)
	register struct socket *head;
{
	register struct socket *so;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soconnindication: not holding networking sema when should be");
	if (!(so = sonewconn(head)))
		goto bad;
	(void) (*so->so_proto->pr_usrreq)(so, PRU_ACCEPTCONN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	return (so);
bad:
	return ((struct socket *)0);
}

/*
 * When an attempt at a new connection is noted on a socket
 * which accepts connections, sonewconn is called.  If the
 * connection is possible (subject to space constraints, etc.)
 * then we allocate a new structure, propoerly linked into the
 * data structure of the original socket, and return this.
 */
struct socket *
sonewconn(head)
	register struct socket *head;
{
	register struct socket *so;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sonewconn: not holding networking sema when should be");
	if (head->so_qlen + head->so_q0len > 3 * head->so_qlimit / 2)
		goto bad;
	MALLOC(so, struct socket *, sizeof (struct socket), M_SOCKET, M_NOWAIT);
	if (so == NULL)
		goto bad;
	bzero((caddr_t)so, sizeof (struct socket));
	mbstat.m_mtypes[MT_SOCKET]++;
	so->so_rcv.sb_so = so;
	so->so_snd.sb_so = so;
	so->so_type = head->so_type;
	so->so_options = head->so_options & ~SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;
	so->so_ipc = head->so_ipc;
	so->so_connindication = head->so_connindication;
 	(void) soreserve(so, head->so_snd.sb_hiwat, head->so_rcv.sb_hiwat);
	soqinsque(head, so, 0);
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0)) {
		(void) soqremque(so, 0);
		FREE(so, M_SOCKET);
		mbstat.m_mtypes[MT_SOCKET]--;
		goto bad;
	}
	return (so);
bad:
	return ((struct socket *)0);
}

soqinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soqinsque: not holding networking sema when should be");
	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_q0 = head->so_q0;
		head->so_q0 = so;
	} else {
		head->so_qlen++;
		so->so_q = head->so_q;
		head->so_q = so;
	}
}

soqremque(so, q)
	register struct socket *so;
	int q;
{
	register struct socket *head, *prev, *next;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soqremque: not holding networking sema when should be");
	head = so->so_head;
	prev = head;
	for (;;) {
		next = q ? prev->so_q : prev->so_q0;
		if (next == so)
			break;
		if (next == head)
			return (0);
		prev = next;
	}
	if (q == 0) {
		prev->so_q0 = next->so_q0;
		head->so_q0len--;
	} else {
		prev->so_q = next->so_q;
		head->so_qlen--;
	}
	next->so_q0 = next->so_q = 0;
	next->so_head = 0;
	return (1);
}

/*
 * Socantsendmore indicates that no more data will be sent on the
 * socket; it would normally be applied to a socket when the user
 * informs the system that no more data is to be sent, by the protocol
 * code (in case PRU_SHUTDOWN).  Socantrcvmore indicates that no more data
 * will be received, and will normally be applied to the socket by a
 * protocol when it detects that the peer will send no more data.
 * Data queued for reading in the socket may yet be read.
 */

socantsendmore(so)
	struct socket *so;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"socantsendmore: not holding networking sema when should be");
	so->so_state |= SS_CANTSENDMORE;
	sowwakeup(so);
}

socantrcvmore(so)
	struct socket *so;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"socantrcvmore: not holding networking sema when should be");
	so->so_state |= SS_CANTRCVMORE;
	sorwakeup(so);
}


int
sbmaylock(sb)                  /*HP*/
	struct sockbuf *sb;
{
   /* prevent sleep in sblock if SS_NOWAIT */

   int s;

   VASSERT((sb)->sb_so);

   s = splimp();                  /* splnet??  see sorflush */
   if (sb->sb_flags & SB_LOCK) {
      /* locked: if SS_NOWAIT, just return */
      splx(s);
      if (sb->sb_so->so_state & SS_NOWAIT) {
         /* combination of EWOULDBLOCK and SB_WANT indicates
          * sbmaylock failure, distinguished from other EWOULDBLOCK
	  * errors.  also, setting SB_WANT ensures that sb_wakeup
	  * is called from sbunlock.
          */
         sb->sb_flags |= SB_WANT;
         return (EWOULDBLOCK);
         }
      sblock(sb);    /* wait for unlock if necessary */
      }
   else {
      /* not locked: lock it */
      sb->sb_flags |= SB_LOCK;
      splx(s);
      }

   return (0);
}


/*
 * Socket select/wakeup routines.
 */

/*
 * Queue a process for a select on a socket buffer.
 */
sbselqueue(sb)
	struct sockbuf *sb;
{
	register struct proc *p;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbselque: not holding networking sema when should be");

	if ((p = sb->sb_sel) && p->p_wchan == (caddr_t)&selwait)
		sb->sb_flags |= SB_COLL;
	else
		sb->sb_sel = u.u_procp;
}


/*
 * Wait for data to arrive at/drain from a socket buffer.
 */
sbwait(sb)
	struct sockbuf *sb;
{
	int error;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbwait: not holding networking sema when should be");

	sb->sb_flags |= SB_WAIT;
	error = sleep((caddr_t)&sb->sb_cc, PZERO+1 | PCATCH );

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbwait: not holding networking sema after wakeup");

	return (error);
}


/*
 * Wakeup processes waiting on a socket buffer.
 */
sbwakeup(sb)
	register struct sockbuf *sb;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbwakeup: not holding networking sema when should be");

	if (sb->sb_sel) {
		selwakeup(sb->sb_sel, sb->sb_flags & SB_COLL);
		sb->sb_sel = 0;
		sb->sb_flags &= ~(SB_SEL | SB_COLL);  /*HP* was SB_COLL */
	}

	if (sb->sb_flags & SB_WAIT) {
		sb->sb_flags &= ~SB_WAIT;
        	if (sb->sb_wakeup)
                	(void) sewakeup(sb->sb_so, sb, 1);
		else
			wakeup((caddr_t)&sb->sb_cc);
	}
}


/*
 * Wakeup processes waiting on a socket buffer.
 *
 * Notify alternate wakeup routine of new state. The bits are
 * for XTI at the moment and encode the following:
 *	disconn ordrel conn connconfirm data oobdata
 * When any of these are valid the high bit is set, if the
 * word is all 0, a previous failed lock attempt may be retried.
 *
 * Note: while the socket and sockbuf may be accessed from this
 * upcall (e.g. for determining sbspace, state, etc), it is not
 * possible to perform an action such as sending or receiving,
 * or to modify socket values. That must be done later from a
 * safe context.
 */
int
sewakeup(so, sb, what)
	struct socket *so;
	struct sockbuf *sb;
	int what;
{
	int state;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbwakeup: not holding networking sema when should be");

	/* Encode state */
	if (so) {
		state = SE_STATUS;
		if (what == 0)
			state |= SE_POLL;
		if (so->so_error)
			state |= SE_ERROR;
		if (sb->sb_cc)
			state |= SE_HAVEDATA;
		if (so->so_oobmark)
			state |= SE_HAVEOOB;
		if (sbspace(sb) <= 0)
			state |= SE_DATAFULL;
		if (so->so_state & (SS_ISCONNECTED)) /* |SS_ISCONFIRMING)) */
			state |= SE_CONNOUT;
		if (so->so_qlen)
			state |= SE_CONNIN;
		if (so->so_state & SS_ISCONNECTED) {
			if (!(so->so_state & SS_CANTSENDMORE))
				state |= SE_SENDCONN;
			if (!(so->so_state & SS_CANTRCVMORE))
				state |= SE_RECVCONN;
		}
	} else
		state = 0;

	if (sb->sb_wakeup)
		(*sb->sb_wakeup)(sb->sb_wakearg, state);

	return state;
}


/*
 * Wakeup socket readers and writers.
 * Do asynchronous notification via SIGIO
 * if the socket has the SS_ASYNC flag set.
 */
sowakeup(so, sb)
	register struct socket *so;
	struct sockbuf *sb;
{
	register struct proc *p;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sowakeup: not holding networking sema when should be");

	if (sb->sb_flags & SB_NVS) {		/* kernel telnet */
		NETCALL(NET_NVSINPUT)(so->so_nvs_index);
		return;
	}

	sbwakeup(sb);

	if (so->so_state & SS_ASYNC) {
		if (so->so_pgrp < 0)
			gsignal(-so->so_pgrp, SIGIO);
		else if (so->so_pgrp > 0 && (p = pfind(so->so_pgrp)) != 0)
			psignal(p, SIGIO);
	}
}


int
sbpoll(so, sb)
	struct socket *so;
	struct sockbuf *sb;
{
	/* there are historical reasons why this is a separate function */

	int state;

	state = sewakeup(so, sb, 0);
	return (state);
}


/*
 * Socket buffer (struct sockbuf) utility routines.
 *
 * Each socket contains two socket buffers: one for sending data and
 * one for receiving data.  Each buffer contains a queue of mbufs,
 * information about the number of mbufs and amount of data in the
 * queue, and other fields allowing select() statements and notification
 * on data availability to be implemented.
 *
 * Data stored in a socket buffer is maintained as a list of records.
 * Each record is a list of mbufs chained together with the m_next
 * field.  Records are chained together with the m_act field. The upper
 * level routine soreceive() expects the following conventions to be
 * observed when placing information in the receive buffer:
 *
 * 1. If the protocol requires each message be preceded by the sender's
 *    name, then a record containing that name must be present before
 *    any associated data (mbuf's must be of type MT_SONAME).
 * 2. If the protocol supports the exchange of ``access rights'' (really
 *    just additional data associated with the message), and there are
 *    ``rights'' to be received, then a record containing this data
 *    should be present (mbuf's must be of type MT_RIGHTS).
 * 3. If a name or rights record exists, then it must be followed by
 *    a data record, perhaps of zero length.
 *
 * Before using a new socket structure it is first necessary to reserve
 * buffer space to the socket, by calling sbreserve().  This should commit
 * some of the available buffer space in the system buffer pool for the
 * socket (currently, it does nothing but enforce limits).  The space
 * should be released by calling sbrelease() when the socket is destroyed.
 */

soreserve(so, sndcc, rcvcc)
	register struct socket *so;
	u_long sndcc, rcvcc;
{
	/* \*HP*\ 8.0
	 * Use sbreserve_sw to decide whether to call sbreserve or sbreserve2,
	 * depending on socket characteristics.
	 */

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"soreserve: not holding networking sema when should be");
	if (sbreserve_sw(so, &so->so_snd, sndcc) == 0)                   /*HP*/
		goto bad;
	if (sbreserve_sw(so, &so->so_rcv, rcvcc) == 0)                   /*HP*/
		goto bad2;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	return (ENOBUFS);
}

/* \*HP*\ 8.0
 * Choose sbreserve (4.3BSD compatible) or sbreserve2 (7.0HP compatible,
 * somewhat) based on socket characteristics.
 */

int
sbreserve_sw(so, sb, cc)                                                 /*HP*/
	register struct socket *so;
	struct sockbuf *sb;
	u_long cc;
{
	if (so->so_proto->pr_domain->dom_family == AF_CCITT)
	   return (sbreserve2(sb, cc));
	else
	   return (sbreserve(sb, cc));
}

/*
 * Allot mbufs to a sockbuf.
 * Attempt to scale cc so that mbcnt doesn't become limiting
 * if buffering efficiency is near the normal case.
 */
/* ANY CHANGES TO sbreserve MIGHT ALSO APPLY TO sbreserve2 */            /*HP*/

sbreserve(sb, cc)
	struct sockbuf *sb;
	u_long cc;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbreserve: not holding networking sema when should be");
	if (cc > (u_long)SB_MAX * MCLBYTES / (2 * MSIZE + MCLBYTES))
		return (0);
	sb->sb_hiwat = cc;
	sb->sb_mbmax = MAX(MSIZE * 2, MIN(cc * 2, SB_MAX));
	return (1);
}

/* \*HP*\ 8.0
 * sbreserve2 added primarily to accomodate AF_CCITT.  Allows "cc" to be
 * 65535, needed for 7.0 compatibility for AF_CCITT.  Technically, we also
 * need 7.0 compat for UDP (well, up to 65534!), but we guess that no
 * program would send a UDP message that large because of the inherent
 * unreliability.
 *
 * In any cases, the choice between calling sbreserve and sbreserve2 is
 * really up to the protocol.  See soreserve and sosetopt.
 */
/* ANY CHANGES TO sbreserve2 MIGHT ALSO APPLY TO sbreserve */

int
sbreserve2(sb, cc)                                                       /*HP*/
	struct sockbuf *sb;
	u_long cc;
{
	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbreserve2: not holding networking sema when should be");
	if (cc >= (u_long)SB_MAX)
		return (0);
	sb->sb_hiwat = cc;
	sb->sb_mbmax = MAX(MSIZE * 2, MIN(cc * 2, SB_MAX));
	return (1);
}

/* \*HP*\ 8.07 & 9.0
 * sbreserve_big added primarily to accomodate NFS.  Allows "cc" to be
 * very big (at least 256 * 1024) for NFS performance reason.
 *
 * This routine is internally called by nfs - svckudp_create().
 */
/* ANY CHANGES TO sbreserve_big MIGHT ALSO APPLY TO sbreserve */

int
sbreserve_big(sb, cc)                    /*HP*/
	struct sockbuf *sb;
	u_long cc;
{
	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbreserve_big: not holding networking sema when should be");
	sb->sb_hiwat = cc;
	sb->sb_mbmax = 4 * cc;
	return (1);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
sbrelease(sb)
	struct sockbuf *sb;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbrelease: not holding networking sema when should be");
	sbflush(sb);
	sb->sb_hiwat = sb->sb_mbmax = 0;
}

/*
 * Routines to add and remove
 * data from an mbuf queue.
 *
 * The routines sbappend() or sbappendrecord() are normally called to
 * append new mbufs to a socket buffer, after checking that adequate
 * space is available, comparing the function sbspace() with the amount
 * of data to be added.  sbappendrecord() differs from sbappend() in
 * that data supplied is treated as the beginning of a new record.
 * To place a sender's address, optional access rights, and data in a
 * socket receive buffer, sbappendaddr() should be used.  To place
 * access rights and data in a socket receive buffer, sbappendrights()
 * should be used.  In either case, the new data begins a new record.
 * Note that unlike sbappend() and sbappendrecord(), these routines check
 * for the caller that there will be enough space to store the data.
 * Each fails if there is not enough space, or if it cannot find mbufs
 * to store additional information in.
 *
 * Reliable protocols may use the socket send buffer to hold data
 * awaiting acknowledgement.  Data is normally copied from a socket
 * send buffer in a protocol with m_copy for output to a peer,
 * and then removing the data from the socket buffer with sbdrop()
 * or sbdroprecord() when the data is acknowledged by the peer.
 */

/*
 * Append mbuf chain m to the last record in the
 * socket buffer sb.  The additional space associated
 * the mbuf chain is recorded in sb.  Empty mbufs are
 * discarded and mbufs are compacted where possible.
 */
sbappend(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	register struct mbuf *n;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbappend: not holding networking sema when should be");
	if (m == 0)
		return;
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		while (n->m_next)
			n = n->m_next;
	}
	sbcompress(sb, m, n);
}

/*
 * As above, except the mbuf chain
 * begins a new record.
 */
sbappendrecord(sb, m0)
	register struct sockbuf *sb;
	register struct mbuf *m0;
{
	register struct mbuf *m;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbappendrecord: not holding networking sema when should be");
	if (m0 == 0)
		return;
	if (m = sb->sb_mb)
		while (m->m_act)
			m = m->m_act;
	/*
	 * Put the first mbuf on the queue.
	 * Note this permits zero length records.
	 */
	sballoc(sb, m0);
	if (m)
		m->m_act = m0;
	else
		sb->sb_mb = m0;
	m = m0->m_next;
	m0->m_next = 0;
	sbcompress(sb, m, m0);
}

/*
 * Append address and data, and optionally, rights
 * to the receive queue of a socket.  Return 0 if
 * no space in sockbuf or insufficient mbufs.
 */
sbappendaddr(sb, asa, m0, rights0)
	register struct sockbuf *sb;
	struct sockaddr *asa;
	struct mbuf *rights0, *m0;
{
	register struct mbuf *m, *n;
	int space = sizeof (*asa);

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbappendaddr: not holding networking sema when should be");
	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	if (rights0)
		space += rights0->m_len;
	if (space > sbspace(sb))
		return (0);
	MGET(m, M_DONTWAIT, MT_SONAME);
	if (m == 0)
		return (-1);
	*mtod(m, struct sockaddr *) = *asa;
	m->m_len = sizeof (*asa);
	if (rights0 && rights0->m_len) {
		m->m_next = m_copy(rights0, 0, rights0->m_len);
		if (m->m_next == 0) {
			m_freem(m);
			return (0);
		}
		sballoc(sb, m->m_next);
	}
	sballoc(sb, m);
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m->m_next)
		m = m->m_next;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

/*
 * Append UNIX domain address and data, and optionally, rights
 * to the receive queue of a socket.  Return 0 if
 * no space in sockbuf or insufficient mbufs.
 */
sbappendaddr_un(sb, asa, m0, rights0)
	register struct sockbuf *sb;
	struct sockaddr_un *asa;
	struct mbuf *rights0, *m0;
{
	register struct mbuf *m, *n;
	int space = sizeof (*asa);

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbappendaddr: not holding networking sema when should be");
	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	if (rights0)
		space += rights0->m_len;
	if (space > sbspace(sb))
		return (0);
	MGET(m, M_DONTWAIT, MT_SONAME);
	if (m == 0)
		return (0);
	*mtod(m, struct sockaddr_un *) = *asa;
	m->m_len = sizeof (*asa);
	if (rights0 && rights0->m_len) {
		m->m_next = m_copy(rights0, 0, rights0->m_len);
		if (m->m_next == 0) {
			m_freem(m);
			return (0);
		}
		sballoc(sb, m->m_next);
	}
	sballoc(sb, m);
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m->m_next)
		m = m->m_next;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

sbappendrights(sb, m0, rights)
	struct sockbuf *sb;
	struct mbuf *rights, *m0;
{
	register struct mbuf *m, *n;
	int space = 0;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbappendaddr: not holding networking sema when should be");
	if (rights == 0)
		panic("sbappendrights");
	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	space += rights->m_len;
	if (space > sbspace(sb))
		return (0);
	m = m_copy(rights, 0, rights->m_len);
	if (m == 0)
		return (0);
	sballoc(sb, m);
	if (n = sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

/*
 * Compress mbuf chain m into the socket
 * buffer sb following mbuf n.  If n
 * is null, the buffer is presumed empty.
 */
sbcompress(sb, m, n)
	register struct sockbuf *sb;
	register struct mbuf *m, *n;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbcompress: not holding networking sema when should be");
	while (m) {
		if (m->m_len == 0) {
			m = m_free(m);
			continue;
		}
		if (n && n->m_off <= MMAXOFF && m->m_off <= MMAXOFF &&
		    (n->m_off + n->m_len + m->m_len) <= MMAXOFF &&
		    n->m_type == m->m_type) {
			bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
			    (unsigned)m->m_len);
			n->m_len += m->m_len;
			sb->sb_cc += m->m_len;
			m = m_free(m);
			continue;
		}
		sballoc(sb, m);
		if (n)
			n->m_next = m;
		else
			sb->sb_mb = m;
		n = m;
		m = m->m_next;
		n->m_next = 0;
	}
}

/*
 * Free all mbufs in a sockbuf.
 * Check that all resources are reclaimed.
 */
sbflush(sb)
	register struct sockbuf *sb;
{

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbflush: not holding networking sema when should be");
	if (sb->sb_flags & SB_LOCK)
		panic("sbflush");
	while (sb->sb_mbcnt)
		sbdrop(sb, (int)sb->sb_cc);
	if (sb->sb_cc || sb->sb_mbcnt || sb->sb_mb)
		panic("sbflush 2");
}

/*
 * Drop data from (the front of) a sockbuf.
 */
sbdrop(sb, len)
	register struct sockbuf *sb;
	register int len;
{
	register struct mbuf *m, *mn;
	struct mbuf *next;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbdrop: not holding networking sema when should be");
	next = (m = sb->sb_mb) ? m->m_act : 0;
	while (len > 0) {
		if (m == 0) {
			if (next == 0)
				panic("sbdrop");
			m = next;
			next = m->m_act;
			continue;
		}
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_off += len;
			sb->sb_cc -= len;
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	while (m && m->m_len == 0) {
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	if (m) {
		sb->sb_mb = m;
		m->m_act = next;
	} else
		sb->sb_mb = next;
}

/*
 * Drop a record off the front of a sockbuf
 * and move the next record to the front.
 */
sbdroprecord(sb)
	register struct sockbuf *sb;
{
	register struct mbuf *m, *mn;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"sbdroprecord: not holding networking sema when should be");
	m = sb->sb_mb;
	if (m) {
		sb->sb_mb = m->m_act;
		do {
			sbfree(sb, m);
			MFREE(m, mn);
		} while (m = mn);
	}
}

#ifdef	__hp9000s800
/*
 * Build an mbuf chain of outbound data, attempting to use copy avoidance
 * techniques if the user has requested copy avoidance and if the protocol
 * stack supports copy avoidance.
 *
 * This routine is called by (*pr_usrsend)() routines (eg. udp_usrsend).
 */

#define SOLinkMBUF(M,TAIL) {					\
	MGET(M, M_WAIT, MT_DATA);				\
	*(TAIL) = M;						\
	TAIL = &(M)->m_next;					\
}

#define SOAvoidCopyToCLUSTER(M,MAXLEN,UIO,ERROR) {		\
	caddr_t addr; int len = MAXLEN;				\
	(M)->m_ops_arg = MCLARGADDR(M,caddr_t);			\
	ERROR = uiowrite_avoid_copy( &addr, &len, 0, UIO, 	\
	    &(M)->m_ops_ops, &(M)->m_ops_arg, MCLARGSIZE);	\
	MCLADD(M,addr,len,MCLBYTES,MCL_OPS);				\
}

#define SOCopyToCLUSTER(M,LEN,UIO,ERROR) {			\
	MCLGETVAR_WAIT(M,LEN);					\
	(M)->m_len = LEN;					\
	ERROR = uiomove( mtod(M,caddr_t), LEN, UIO_WRITE, UIO );\
}

#define SOCopyToMBUF(M,LEN,UIO,ERROR) {				\
	(M)->m_len = LEN;					\
	ERROR = uiomove( mtod(M,caddr_t), LEN, UIO_WRITE, UIO );\
}

int
sobuildsnd( mp, uio, space, flags )
struct mbuf **mp;
struct uio *uio;
int space;
int flags;
{
	struct mbuf *m;
	int error,datalen;

	*mp = 0;

	if (space < 0)
		goto send;

	if (uio->uio_resid >= MCLBYTES) {
		/* 
		 *  Determine if COW can be used for first cluster worth of
		 *  user data: if COW can be used for the first batch of 
		 *  data, it should work for the remaining data until there 
		 *  is less than a cluster worth of user data left or until
		 *  the socket buffer is full.
		 */
		SOLinkMBUF( m, mp );
		if ((flags & SB_COPYAVOID_REQ)  &&
		    (flags & SB_COPYAVOID_SUPP)) {
			SOAvoidCopyToCLUSTER( m, MCLBYTES, uio, error );
			if (error == EAGAIN) /*eg, user data not page aligned */
				goto copy;
			if (error)
				goto exit;
			if ((space -= m->m_len) <= 0)
				goto send;
			while (uio->uio_resid >= MCLBYTES) {
				SOLinkMBUF( m, mp );
				SOAvoidCopyToCLUSTER( m, MCLBYTES, uio, error );
				if (error) 
					goto exit;
				if ((space -= m->m_len) <= 0)
					goto send;
			}
		}
		else {
		copy:	SOCopyToCLUSTER( m, MCLBYTES, uio, error );
			if (error)
				goto exit;
			if ((space -= m->m_len) <= 0)
				goto send;
			while (uio->uio_resid >= MCLBYTES) {
				SOLinkMBUF( m, mp );
				SOCopyToCLUSTER( m, MCLBYTES, uio, error );
				if (error)
					goto exit;
				if ((space -= m->m_len) <= 0)
					goto send;
			}
		}
	}

	/*
	 *  Less than a cluster worth of data: if at least half a cluster
	 *  worth of data then place it all in a cluster, else place it in
	 *  a variable length cluster which is no greater than 2x the data.
	 *  However, if the remaining data fits into an mbuf use that
	 *  instead of a variable length cluster.
	 */
	datalen = uio->uio_resid;
	if (datalen <= MLEN) {
		SOLinkMBUF( m, mp );
		SOCopyToMBUF( m, datalen, uio, error );
		if (error)
			goto exit;
		goto send;
	} 
	SOLinkMBUF( m, mp );
	SOCopyToCLUSTER( m, datalen, uio, error );
	if (error)
		goto exit;
send:	error = 0;

exit:	return (error);
}

#undef SOLinkMBUF
#undef SOAvoidCopyToCLUSTER
#undef SOCopyToCLUSTER
#undef SOCopyToMBUF
#endif	/* __hp9000s800 */

/*
 * NIPC 
 */
void
nipc_sbappendmsg(sb, m, eom)
struct sockbuf	*sb;
struct mbuf	*m;
int		eom;
{
	struct mbuf	*mptr;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"nipc_sbappendmsg: not holding networking sema when should be");
	/* UPDATE SB_CC ETC. */
	mptr = m;
	while (mptr) {
		sballoc(sb, mptr);
		mptr = mptr->m_next;
	}

	/* IF NO MBUFS ARE CURRENTLY IN THE SOCKBUF THEN THIS IS THE FIRST */
	if (mptr = sb->sb_mb) {
		/* find last message */
		while (mptr->m_act)
			mptr = mptr->m_act;

		if (!(sb->sb_flags & SB_MSGCOMPLETE)) {
			/* APPEND DATA TO END OF MBUF CHAIN */
			/* goto end of chain */
			while (mptr->m_next)
				mptr = mptr->m_next;
			mptr->m_next = m;
		} else 
			/* CREATE NEW MESSAGE */
			mptr->m_act = m;
	} else 
		/* SOCKET BUFFER IS EMPTY */
		sb->sb_mb = m;

	/* UPDATE THE MSG_COMPLETE FLAG */ 
	if (eom)
		sb->sb_flags |= SB_MSGCOMPLETE; /* set */
	else
		sb->sb_flags &= ~SB_MSGCOMPLETE; /* clear */
}


void 
nipc_sbmsgsize(sb, off, ilen, olen, eom)
struct sockbuf	*sb;
int		off;
int		ilen;
int		*olen;	
int		*eom;
{
	struct mbuf	*m = sb->sb_mb;
	struct mbuf	*first_mbuf = sb->sb_mb;

	NETSKT_ASSERT_SEMA(OWN_NET_SEMA,
		"nipc_sbmsgsize: not holding networking sema when should be");
	/* IF NO DATA IS TO BE SENT THEN DONE */
	if (ilen == 0)
		return;

	/* INITIALIZE MESSAGE LENGTH TO ZERO */
	*olen = 0;

	/* SKIP PART OF A MESSAGE(S) */
	while (off > 0) {
		if (m == 0)
			panic("nipc_sbmsg2");

		/* IF DATA STARTS IN THIS MBUF THEN BREAK */
		if (off < m->m_len)
			break;

		/* GOTO NEXT MBUF */
		off -= m->m_len;
		if (m->m_next)
			m = m->m_next;
		else 
			m = first_mbuf = first_mbuf->m_act;
		
	}

	/* count bytes until end of data or end of requested length */
	while (m) {
		if (off +ilen < m->m_len) {
			/* bytes requested is less than whats in this mbuf */
			*olen += ilen;
			*eom = 0;
			return;
		} 
		else if (off + ilen == m->m_len) {
			/* bytes requested equals whats in this mbuf */
			*olen += ilen;
			if (m->m_next != NULL) 
				*eom = 0;
			else if (first_mbuf->m_act != NULL) 
				*eom = 1;
			else 
				*eom = sb->sb_flags & SB_MSGCOMPLETE;
			return;
		}
		else {
			/* continue to next mbuf in this message */
			ilen -= m->m_len - off;
			*olen += m->m_len - off;
			off = 0;
			m = m->m_next;
		}
	}

	/* MESSAGE IS SHORTER THAN BYTES REQUESTED  */
	if (first_mbuf->m_act)
		*eom = 1;
	else 
		*eom = sb->sb_flags & SB_MSGCOMPLETE;
}
