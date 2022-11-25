/*
 * $Header: uipc_syscall.c,v 1.9.83.5 93/11/04 11:13:34 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_syscall.c,v $
 * $Revision: 1.9.83.5 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/04 11:13:34 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_syscall.c $Revision: 1.9.83.5 $ $Date: 93/11/04 11:13:34 $";
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
 *	@(#)uipc_syscalls.c	7.4 (Berkeley) 1/20/88
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#ifdef AUDIT
#include "../h/audit.h"
#include "../h/dgram_aud.h"
#include "../netinet/in.h"
#endif /* AUDIT */

#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */

/*
 * System call interface to the socket abstraction.
 */

struct	file *getsock();
struct  file *getsockns();                 /* HP INDaa14177 */
extern	struct fileops socketops;

socket()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
	} *uap = (struct a *)u.u_ap;
	struct socket *so;
	register struct file *fp;

	if ((fp = falloc()) == NULL)
		return;
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &socketops;
	u.u_error = socreate(uap->domain, &so, uap->type, uap->protocol, 0);
	if (u.u_error)
		goto bad;
	fp->f_data = (caddr_t)so;
	return;
bad:
	uffree(u.u_r.r_val1);		/* u.u_ofile[u.u_r.r_val1] = 0; */
	crfree(fp->f_cred);
	/* MPNET: Rule #1A */
	SPINLOCK(file_table_lock);
	FPENTRYFREE(fp);                /* from UKL for sar(1) */
	SPINUNLOCK(file_table_lock);
}

bind()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *nam;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	u.u_error = sockargs(&nam, uap->name, uap->namelen, MT_SONAME);
	if (u.u_error)
		return;
	u.u_error = sobind((struct socket *)fp->f_data, nam);
	m_freem(nam);
}

listen()
{
	register struct a {
		int	s;
		int	backlog;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	u.u_error = solisten((struct socket *)fp->f_data, uap->backlog);
}

accept()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	*anamelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct file *fp2;
	struct mbuf *nam;
	int namelen;
	int s;
	int error;
	register struct socket *so;
	sv_sema_t savestate;		/* MPNET: MP save state */

	if (uap->name == 0)
		goto noname;
	u.u_error = copyin((caddr_t)uap->anamelen, (caddr_t)&namelen,
		sizeof (namelen));
	if (u.u_error)
		return;
	if (useracc((caddr_t)uap->name, (u_int)namelen, B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
noname:
	fp2 = fp = getsock(uap->s);
	if (fp == 0)
		return;
	NETMP_GO_EXCLUSIVE(s, savestate);	/*MPNET: MP prot on */
	so = (struct socket *)fp->f_data;
	if ((so->so_options & SO_ACCEPTCONN) == 0) {
		u.u_error = EINVAL;
		goto unlock_sema;
	}
	if ((so->so_state & SS_NBIO) && so->so_qlen == 0) {
		u.u_error = EWOULDBLOCK;
		goto unlock_sema;
	}
	if ((fp->f_flag & O_NONBLOCK) && so->so_qlen == 0) {
		u.u_error = EAGAIN;
		goto unlock_sema;
	}
	if ((fp->f_flag & O_NDELAY) && so->so_qlen == 0) {
		u.u_error = EWOULDBLOCK;
		goto unlock_sema;
	}

	while (so->so_qlen == 0 && so->so_error == 0) {
		if (so->so_state & SS_CANTRCVMORE) {
			so->so_error = ECONNABORTED;
			break;
		}
		error = sleep((caddr_t)&so->so_timeo, PZERO+1 | PCATCH);

		/* If we get interrupted, return to caller */
		if (error) {
			u.u_error = EINTR;
			goto unlock_sema;
		}
	}
	if (so->so_error) {
		u.u_error = so->so_error;
		so->so_error = 0;
		goto unlock_sema;
	}
	if (ufalloc(0) < 0)
		goto unlock_sema;

	fp = falloc();
	if (fp == 0) {
		uffree(u.u_r.r_val1);	/* u.u_ofile[u.u_r.r_val1] = 0; */
		goto unlock_sema;
	}
	{ struct socket *aso = so->so_q;
	  if (soqremque(aso, 1) == 0)
		panic("accept");
	  so = aso;
	}
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_ops = fp2->f_ops;
	fp->f_data = (caddr_t)so;
	nam = m_get(M_WAIT, MT_SONAME);
	(void) soaccept(so, nam);
#ifdef AUDIT
	if (AUDITEVERON())
		save_sockaddr(mtod(nam, caddr_t), (u_int)nam->m_len);
#endif /* AUDIT */
	if (uap->name) {
		if (namelen > nam->m_len)
			namelen = nam->m_len;
		/* SHOULD COPY OUT A CHAIN HERE */
		(void) copyout(mtod(nam, caddr_t), (caddr_t)uap->name,
		    (u_int)namelen);
		(void) copyout((caddr_t)&namelen, (caddr_t)uap->anamelen,
		    sizeof (*uap->anamelen));
	}
	m_freem(nam);

unlock_sema:

	NETMP_GO_UNEXCLUSIVE(s, savestate);/*MPNET: prot off */
}

connect()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *nam;
	int oldlevel, oldlevel2;
	sv_sema_t savestate;		/* MPNET: MP save state */

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	so = (struct socket *)fp->f_data;
	if ((so->so_state & SS_NBIO) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EALREADY;
		return;
	}
	if ((fp->f_flag & O_NONBLOCK) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EALREADY;
		return;
	}
	if ((fp->f_flag & O_NDELAY) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EALREADY;
		return;
	}
	u.u_error = sockargs(&nam, uap->name, uap->namelen, MT_SONAME);
	if (u.u_error)
		return;
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);	/*MPNET: MP prot on */
	/* if interrupt occured then don't call soconnect() again */
	if ((so->so_state & SS_INTERRUPTED) && (so->so_state &
		(SS_ISCONNECTED | SS_ISCONNECTING)))
		goto inprogress;

	u.u_error = soconnect(so, nam);
	if (u.u_error)
		goto bad3;
inprogress:
	if ((so->so_state & SS_NBIO) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EINPROGRESS;
		m_freem(nam);
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return;
	}
	if ((fp->f_flag & O_NONBLOCK) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EINPROGRESS;
		m_freem(nam);
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return;
	}
	if ((fp->f_flag & O_NDELAY) &&
	    (so->so_state & SS_ISCONNECTING)) {
		u.u_error = EINPROGRESS;
		m_freem(nam);
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return;
	}
	so->so_state &= ~SS_INTERRUPTED; /* clear 'interrupted' flag */
	if (setjmp(&u.u_qsave)) {
		NETMP_GO_EXCLUSIVE(oldlevel2, savestate); /*MPNET: MP prot on */
		if (u.u_error == 0)
			u.u_error = EINTR;
		so->so_state |= SS_INTERRUPTED; /* remember interrupt occured */
		goto bad2;
	}
	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0)
		sleep((caddr_t)&so->so_timeo, PZERO+1);
	u.u_error = so->so_error;
	so->so_error = 0;
bad2:
	if (so->so_state & SS_ISCONNECTED)
		u.u_error = 0;
	so->so_state &= ~SS_ISCONNECTING;
bad3:
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);	/*MPNET: MP prot off */
	m_freem(nam);
}

socketpair()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
		int	*rsv;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp1, *fp2;
	struct socket *so1, *so2;
	int sv[2], oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	if (useracc((caddr_t)uap->rsv, 2 * sizeof (int), B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);	/*MPNET: MP prot on */
	u.u_error = socreate(uap->domain, &so1, uap->type, uap->protocol, 0);
	if (u.u_error) {
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
		return;
	}
	u.u_error = socreate(uap->domain, &so2, uap->type, uap->protocol, 0);
	if (u.u_error)
		goto free;
	fp1 = falloc();
	if (fp1 == NULL)
		goto free2;
	sv[0] = u.u_r.r_val1;
	fp1->f_flag = FREAD|FWRITE;
	fp1->f_type = DTYPE_SOCKET;
	fp1->f_ops = &socketops;
	fp1->f_data = (caddr_t)so1;
	fp2 = falloc();
	if (fp2 == NULL)
		goto free3;
	fp2->f_flag = FREAD|FWRITE;
	fp2->f_type = DTYPE_SOCKET;
	fp2->f_ops = &socketops;
	fp2->f_data = (caddr_t)so2;
	sv[1] = u.u_r.r_val1;
	u.u_error = soconnect2(so1, so2);
	if (u.u_error)
		goto free4;
	if (uap->type == SOCK_DGRAM) {
		/*
		 * Datagram socket connection is asymmetric.
		 */
		 u.u_error = soconnect2(so2, so1);
		 if (u.u_error)
			goto free4;
	}
	u.u_r.r_val1 = 0;
	(void) copyout((caddr_t)sv, (caddr_t)uap->rsv, 2 * sizeof (int));
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
	return;
free4:
	uffree(sv[1]);			/* u.u_ofile[sv[1]] = 0; */
	crfree(fp2->f_cred);
	/* MPNET: Rule #1A */
	SPINLOCK(file_table_lock);
	FPENTRYFREE(fp2);               /* from UKL for sar(1) */
	SPINUNLOCK(file_table_lock);
free3:
	uffree(sv[0]);			/* u.u_ofile[sv[0]] = 0; */
	crfree(fp1->f_cred);
	/* MPNET: Rule #1A */
	SPINLOCK(file_table_lock);
	FPENTRYFREE(fp1);               /* from UKL for sar(1) */
	SPINUNLOCK(file_table_lock);
free2:
	(void)soclose(so2);
free:
	(void)soclose(so1);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: prot off */
}

sendto()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	to;
		int	tolen;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = uap->to;
	msg.msg_namelen = uap->tolen;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
}

send()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
}

sendmsg()
{
	register struct a {
		int	s;
		caddr_t	msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin(uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen > sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
	sendit(uap->s, &msg, uap->flags);
}

sendit(s, mp, flags)
	int s;
	register struct msghdr *mp;
	int flags;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *to, *rights;
	int len;
	struct socket *so;
	int (*prsend)();
	int   sosend();
	
	fp = getsock(s);
	if (fp == 0)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	auio.uio_fpflags = fp->f_flag;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		if (iov->iov_len == 0)
			continue;
		if (useracc(iov->iov_base, (u_int)iov->iov_len, B_READ) == 0) {
			u.u_error = EFAULT;
			return;
		}
		auio.uio_resid += iov->iov_len;
	}
	if (mp->msg_name) {
		u.u_error =
		    sockargs(&to, mp->msg_name, mp->msg_namelen, MT_SONAME);
		if (u.u_error)
			return;
	} else
		to = 0;
	if (mp->msg_accrights) {
		u.u_error =
		    sockargs(&rights, mp->msg_accrights, mp->msg_accrightslen,
		    MT_RIGHTS);
		if (u.u_error)
			goto bad;
	} else
		rights = 0;
	len = auio.uio_resid;
	so = (struct socket *)fp->f_data;
	if (((prsend = so->so_proto->pr_usrsend) == 0) ||
	    (so->so_options & SO_DEBUG))
		prsend = sosend;
	u.u_error = (*prsend)(so, to, &auio, flags, rights, (struct mbuf*)0);
#ifdef AUDIT
	if (AUDITDGRAM(u.u_error)) {
		if(so->so_type == SOCK_DGRAM)		/* datagram socket */
			audit_send_dgram(mp->msg_name, mp->msg_namelen,
			   u.u_error);
	} 
#endif /* AUDIT */
	u.u_r.r_val1 = len - auio.uio_resid;
	if (rights)
		m_freem(rights);
bad:
	if (to)
		m_freem(to);
}

recvfrom()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	from;
		int	*fromlenaddr;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;
	int len;

	u.u_error = copyin((caddr_t)uap->fromlenaddr, (caddr_t)&len,
	   sizeof (len));
	if (u.u_error)
		return;
	msg.msg_name = uap->from;
	msg.msg_namelen = len;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)uap->fromlenaddr, (caddr_t)0);
}

recv()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)0, (caddr_t)0);
}

recvmsg()
{
	register struct a {
		int	s;
		struct	msghdr *msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin((caddr_t)uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen > sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
	if (msg.msg_accrights)
		if (useracc((caddr_t)msg.msg_accrights,
		    (unsigned)msg.msg_accrightslen, B_WRITE) == 0) {
			u.u_error = EFAULT;
			return;
		}
	recvit(uap->s, &msg, uap->flags,
	    (caddr_t)&uap->msg->msg_namelen,
	    (caddr_t)&uap->msg->msg_accrightslen);
}

recvit(s, mp, flags, namelenp, rightslenp)
	int s;
	register struct msghdr *mp;
	int flags;
	caddr_t namelenp, rightslenp;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *from, *rights;
	int len;
#ifdef AUDIT
	struct socket *so;
	struct sockaddr_in *sin;
#endif /* AUDIT */
	
	fp = getsock(s);
	if (fp == 0)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	auio.uio_fpflags = fp->f_flag;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		if (iov->iov_len == 0)
			continue;
		if (useracc(iov->iov_base, (u_int)iov->iov_len, B_WRITE) == 0) {
			u.u_error = EFAULT;
			return;
		}
		auio.uio_resid += iov->iov_len;
	}
	len = auio.uio_resid;
	u.u_error =
	    soreceive((struct socket *)fp->f_data, &from, &auio,
		&flags, &rights, (struct mbuf*)0);
	u.u_r.r_val1 = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
			if (len > from->m_len)
				len = from->m_len;
			(void) copyout((caddr_t)mtod(from, caddr_t),
			    (caddr_t)mp->msg_name, (unsigned)len);
		}
		(void) copyout((caddr_t)&len, namelenp, sizeof (int));
	}
#ifdef AUDIT
	so = (struct socket *)fp->f_data;
	if(AUDITDGRAM(u.u_error) && (so->so_type == SOCK_DGRAM)) {
		if (from) {
			sin = mtod(from, struct sockaddr_in *);
			audit_recv_dgram(sin->sin_port, &sin->sin_addr, 
					u.u_error);
		}
	}
#endif /* AUDIT */
	if (mp->msg_accrights) {
		len = mp->msg_accrightslen;
		if (len <= 0 || rights == 0)
			len = 0;
		else {
			if (len > rights->m_len)
				len = rights->m_len;
			(void) copyout((caddr_t)mtod(rights, caddr_t),
			    (caddr_t)mp->msg_accrights, (unsigned)len);
		}
		(void) copyout((caddr_t)&len, rightslenp, sizeof (int));
	}
	if (rights)
		m_freem(rights);
	if (from)
		m_freem(from);
}

shutdown()
{
	struct a {
		int	s;
		int	how;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	u.u_error = soshutdown((struct socket *)fp->f_data, uap->how);
}

setsockopt()
{
	struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	valsize;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct mbuf *m = NULL;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	if (uap->valsize > MLEN) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->val) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == NULL) {
			u.u_error = ENOBUFS;
			return;
		}
		u.u_error =
		    copyin(uap->val, mtod(m, caddr_t), (u_int)uap->valsize);
		if (u.u_error) {
			(void) m_free(m);
			return;
		}
		m->m_len = uap->valsize;
	}
	u.u_error =
	    sosetopt((struct socket *)fp->f_data, uap->level, uap->name, m);
}

getsockopt()
{
	struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	*avalsize;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct mbuf *m = NULL;
	int valsize;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	if (uap->val) {
		u.u_error = copyin((caddr_t)uap->avalsize, (caddr_t)&valsize,
			sizeof (valsize));
		if (u.u_error)
			return;
	} else
		valsize = 0;
	u.u_error =
	    sogetopt((struct socket *)fp->f_data, uap->level, uap->name, &m);
	if (u.u_error)
		goto bad;
	if (uap->val && valsize && m != NULL) {
		if (valsize > m->m_len)
			valsize = m->m_len;
		u.u_error = copyout(mtod(m, caddr_t), uap->val, (u_int)valsize);
		if (u.u_error)
			goto bad;
		u.u_error = copyout((caddr_t)&valsize, (caddr_t)uap->avalsize,
		    sizeof (valsize));
	}
bad:
	if (m != NULL)
		(void) m_free(m);
}

/*
 * Get socket name.
 */
getsockname()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len, oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	fp = getsock(uap->fdes);
	if (fp == 0)
		return;
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return;
	so = (struct socket *)fp->f_data;
	m = m_getclr(M_WAIT, MT_SONAME);
	if (m == NULL) {
		u.u_error = ENOBUFS;
		return;
	}
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot on */
	u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, 0, m, 0);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off */
	if (u.u_error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		goto bad;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
bad:
	m_freem(m);
}

/*
 * Get name of peer for connected socket.
 */
getpeername()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len, oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	fp = getsockns(uap->fdes);      /* HP INDaa14177 */
	if (fp == 0)
		return;
	so = (struct socket *)fp->f_data;
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		u.u_error = ENOTCONN;
		return;
	}
	m = m_getclr(M_WAIT, MT_SONAME);
	if (m == NULL) {
		u.u_error = ENOBUFS;
		return;
	}
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return;
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot on */
	u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR, 0, m, 0);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off */
	if (u.u_error)
		goto bad;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		goto bad;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
bad:
	m_freem(m);
}

sockargs(aname, name, namelen, type)
	struct mbuf **aname;
	caddr_t name;
	int namelen, type;
{
	register struct mbuf *m;
	int error;

	if ((u_int)namelen > MLEN)
		return (EINVAL);
	m = m_get(M_WAIT, type);
	if (m == NULL)
		return (ENOBUFS);
	m->m_len = namelen;
	error = copyin(name, mtod(m, caddr_t), (u_int)namelen);
	if (error)
		(void) m_free(m);
	else {
		*aname = m;
#ifdef AUDIT
		if(AUDITEVERON())
		 	save_sockaddr(mtod(m, caddr_t), (u_int)namelen);
#endif /* AUDIT */
	}
	return (error);
}


struct file *
getsock(fdes)
	int fdes;
{
	register struct file *fp;

	fp = getf(fdes);
	if (fp == NULL)
		return (0);
	if (fp->f_type != DTYPE_SOCKET ||
		(struct socket *)fp->f_data == 0 ||
		((struct socket *)fp->f_data)->so_ipc != SO_BSDIPC) {
		u.u_error = ENOTSOCK;
		return (0);
	}
	return (fp);
}

struct file *
getsockns(fdes)                           /* HP INDaa14177 */
	int fdes;
{
	register struct file *fp;

	fp = getf(fdes);
	if (fp == NULL)
		return (0);
	if (fp->f_type != DTYPE_SOCKET ||
		(struct socket *)fp->f_data == 0 ||
	        !( ((struct socket *)fp->f_data)->so_ipc == SO_BSDIPC  ||
		   ((struct socket *)fp->f_data)->so_ipc == SO_NETIPC) ) {
		u.u_error = ENOTSOCK;
		return (0);
	}
	return (fp);
}
