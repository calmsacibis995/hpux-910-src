/*
 * $Header: uipc_compat.c,v 1.6.83.4 93/12/15 12:19:14 marshall Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_compat.c,v $
 * $Revision: 1.6.83.4 $		$Author: marshall $
 * $State: Exp $		$Locker:  $
 * $Date: 93/12/15 12:19:14 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_compat.c $Revision: 1.6.83.4 $";
#endif

#include "../h/types.h"
#include "../h/param.h"
#include "../h/file.h"
#include "../h/ioctl.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/signal.h"
#include "../h/syscall.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/un.h"
#include "../h/protosw.h"
#include "../h/domain.h"

#include "../net/if.h"
#include "../net/route.h"
#include "../net/netmp.h"	/* MPNET */


int compat_accept();
int compat_bind();
int compat_connect();
int compat_getpeername();
int compat_getsockname();
int compat_getsockopt();
int compat_listen();
int compat_recv();
int compat_recvfrom();
int compat_recvmsg();
int compat_send();
int compat_sendmsg();
int compat_sendto();
int compat_setsockopt();
int compat_shutdown();
int compat_socket();
int compat_socketpair();
#ifdef	__hp9000s300
extern int netioctl();
#endif

uipc_init_compat()
{
#ifdef	__hp9000s300
	sysent_assign(SYS_NETIOCTL,	2, netioctl);
#endif
#ifdef __hp9000s800
	sysent_assign(99,  3, compat_accept);
	sysent_assign(104, 3, compat_bind);
	sysent_assign(98,  3, compat_connect);
	sysent_assign(141, 3, compat_getpeername);
	sysent_assign(150, 3, compat_getsockname);
	sysent_assign(118, 5, compat_getsockopt);
	sysent_assign(106, 2, compat_listen);
	sysent_assign(102, 4, compat_recv);
	sysent_assign(125, 6, compat_recvfrom);
	sysent_assign(113, 3, compat_recvmsg);
	sysent_assign(101, 4, compat_send);
	sysent_assign(114, 3, compat_sendmsg);
	sysent_assign(133, 6, compat_sendto);
	sysent_assign(105, 5, compat_setsockopt);
	sysent_assign(134, 2, compat_shutdown);
	sysent_assign(97,  3, compat_socket);
	sysent_assign(135, 4, compat_socketpair);
#endif
}

int compat_soo_rw();
int compat_soo_ioctl();
int compat_soo_select();
int compat_soo_close();

static struct fileops oldsocketops = {
	compat_soo_rw, compat_soo_ioctl, compat_soo_select, compat_soo_close
};

#define	COMPAT_SO_DONTLINGER	(~SO_LINGER)
#define COMPAT_SO_BURST_IN	0x1003
#define COMPAT_SO_BURST_OUT	0x1004

extern struct file *getsock();

compat_socket()
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
	fp->f_ops = &oldsocketops;
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

compat_bind()
{
	/*
	 * 8.0 Hack --
	 * Since the size of the sockaddr_un has decreased, we allow
	 * those operations to succeed who use pathnames less than
	 * the actual limit (but passed in the sizeof sockaddr_un
	 * as argument).  This code should be yanked if the
	 * sizeof(sockaddr_un) ever increases
	 */
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;

	if (validate_sockaddr_un(uap->s, uap->name, &uap->namelen) == -1)
		return;
	bind();
}

compat_listen()
{
	listen();
}

compat_accept()
{
	accept();
}

compat_connect()
{
	/*
	 * 8.0 Hack --
	 * Since the size of the sockaddr_un has decreased, we allow
	 * those operations to succeed who use pathnames less than
	 * the actual limit (but passed in the sizeof sockaddr_un
	 * as argument).  This code should be yanked if the
	 * sizeof(sockaddr_un) ever increases
	 */
	register struct a {
		int	s;
		caddr_t	name;
		int	namelen;
	} *uap = (struct a *)u.u_ap;

	if (validate_sockaddr_un(uap->s, uap->name, &uap->namelen) == -1)
		return;
	connect();
}

compat_socketpair()
{
	socketpair();
}

compat_sendto()
{
	sendto();
}

compat_send()
{
	send();
}

compat_sendmsg()
{
	sendmsg();
}

compat_recvfrom()
{
	recvfrom();
}

compat_recv()
{
	recv();
}

compat_recvmsg()
{
	recvmsg();
}

compat_shutdown()
{
	shutdown();
}

compat_setsockopt()
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
	struct linger *linger;

	fp = getsock(uap->s);
	if (fp == 0)
		return;
	if (uap->valsize > MLEN) {
		u.u_error = EINVAL;
		return;
	}
	switch (uap->name) {
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_REUSEADDR:
		case SO_BROADCAST:
			m = m_get(M_WAIT, MT_SOOPTS);
			m->m_len = sizeof(int);
			*mtod(m, int *) = 1;
			break;

		case SO_LINGER:
			if (uap->val == NULL || uap->valsize != sizeof(int)) {
				u.u_error = EINVAL;
				return;
			}
			m = m_get(M_WAIT, MT_SOOPTS);
			m->m_len = sizeof(struct linger);
			linger = mtod(m, struct linger *);
			linger->l_onoff = 1;
			u.u_error = copyin(uap->val, &linger->l_linger,
					uap->valsize);
			if (u.u_error) {
				(void)m_freem(m);
				return;
			}
			break;

		case COMPAT_SO_DONTLINGER:
			uap->name = SO_LINGER;
			m = m_get(M_WAIT, MT_SOOPTS);
			m->m_len = sizeof(struct linger);
			linger = mtod(m, struct linger *);
			linger->l_onoff = 1;
			linger->l_linger = 0;
			break;

		case COMPAT_SO_BURST_IN:
		case COMPAT_SO_BURST_OUT:
			/* improved performance in 8.0 :-) */
			return;

		default:
			if (uap->val) {
				m = m_get(M_WAIT, MT_SOOPTS);
				u.u_error = copyin(uap->val, mtod(m, caddr_t),
					(u_int)uap->valsize);
				if (u.u_error) {
					(void) m_free(m);
					return;
				}
				m->m_len = uap->valsize;
			}
			break;
	}
	u.u_error =
	    sosetopt((struct socket *)fp->f_data, uap->level, uap->name, m);
}

compat_getsockopt()
{
	getsockopt();
}

compat_getsockname()
{
	getsockname();
}

compat_getpeername()
{
	getpeername();
}

compat_soo_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	return(soo_rw(fp, rw, uio));
}

#define COMPAT_SIOCSPGRP	_IOW('s',  8, int)
#define COMPAT_SIOCGPGRP	_IOR('s',  9, int)
#define	COMPAT_SIOCADDRT	_IOW('r', 10, struct compat_rtentry)
#define	COMPAT_SIOCDELRT	_IOW('r', 11, struct compat_rtentry)
#define	COMPAT_SIOCPROXYON	_IOW('s', 28, struct compat_prxentry)
#define COMPAT_SIOCPROXYOFF	_IOW('s', 29, struct compat_prxentry)
#define COMPAT_SIOCPROXYADD	_IOW('s', 30, struct compat_prxentry)
#define COMPAT_SIOCPROXYDELETE	_IOW('s', 31, struct compat_prxentry)
#define COMPAT_SIOCPROXYSHOW	_IOW('s', 32, struct compat_prxentry)
#define COMPAT_SIOCPROXYLIST	_IOW('s', 33, struct compat_prxentry)
#define COMPAT_SIOCPROXYFLUSH	_IOW('s', 34, struct compat_prxentry)
#define COMPAT_SIOCPROXYAPPEND	_IOW('s', 35, struct compat_prxentry)
#define	COMPAT_SIOCGIFMETRIC	_IOWR('i', 39, struct ifreq)
#define	COMPAT_SIOCSIFMETRIC	_IOR('i', 40, struct ifreq)
#define	COMPAT_SIOCGIFBRDADDR	_IOWR('i', 38, struct ifreq)
#define COMPAT_SIOCJNVS		_IOW('s', 71, int)

struct	compat_rtentry {
	u_long		rt_hash;
	struct sockaddr	rt_dst;
	struct sockaddr	rt_gateway;
	short		rt_flags;
	short		rt_refcnt;
	u_long		rt_use;
	struct ifnet *	rt_ifp;
};

compat_soo_ioctl(fp, cmd, data)
	struct file *fp;
	int cmd;
	register caddr_t data;
{
	struct sockaddr dst;
	struct sockaddr gateway;
	short flags;
	int error, oldlevel;
	struct socket *so = (struct socket *)fp->f_data;

	switch (cmd) {
		/*
		 * The sign convention has changed for getting SIGIO and
		 * SIGURG signals, between 7.0 and previous releases and 8.0
		 * In 8.0, we do things the same way 4.3 BSD does. This code
		 * makes us backward object (but not backward source) 
		 * compatible.
		 */
		case COMPAT_SIOCSPGRP:
			so->so_pgrp = -(*(int *)data);
			return(0);
		case COMPAT_SIOCGPGRP:
			*(int *)data = -(so->so_pgrp);
			return(0);
		case COMPAT_SIOCADDRT:
		case COMPAT_SIOCDELRT:
			dst = ((struct compat_rtentry *)data)->rt_dst;
			gateway = ((struct compat_rtentry *)data)->rt_gateway;
			flags = ((struct compat_rtentry *)data)->rt_flags;
			bzero(data, sizeof(struct rtentry));
			((struct rtentry *)data)->rt_dst = dst;
			((struct rtentry *)data)->rt_gateway = gateway;
			((struct rtentry *)data)->rt_flags = flags;
			cmd = (cmd == COMPAT_SIOCADDRT)
					? SIOCADDRT
					: SIOCDELRT;
			break;

		case COMPAT_SIOCSIFMETRIC:
			cmd = SIOCSIFMETRIC;
			break;

		case COMPAT_SIOCGIFMETRIC:
			cmd = SIOCGIFMETRIC;
			break;

		case COMPAT_SIOCGIFBRDADDR:
			cmd = SIOCGIFBRDADDR;
			break;
		case COMPAT_SIOCJNVS:
			cmd = SIOCJNVS;
			break;
	}
	error = (soo_ioctl(fp, cmd, data));
	return(error);
}

compat_soo_select(fp, which)
	struct file *fp;
	int which;
{
	return(soo_select(fp, which));
}

compat_soo_close(fp)
	struct file *fp;
{
	return(soo_close(fp));
}

/*
 * 8.0 Hack --
 * Since the size of the sockaddr_un has decreased, we allow
 * those operations to succeed who use pathnames less than
 * the actual limit (but passed in the sizeof sockaddr_un
 * as argument).  This code should be yanked if the
 * sizeof(sockaddr_un) ever increases.
 */

validate_sockaddr_un(s, name, nlp)
	int s;
	caddr_t name;
	int *nlp;
{
	struct file *fp;
	struct socket *so;
	struct sockaddr_un sun;
	int size;
	char c, *cp;

	fp = getsock(s);
	if (fp == 0)
		return(-1);
	so = (struct socket *)fp->f_data;
	if (so->so_proto->pr_domain->dom_family == AF_UNIX) {
		if (*nlp > sizeof(struct sockaddr_un)) {
			cp = name + sizeof(sun.sun_family);
			size = sizeof(sun.sun_family); 
			while (size < sizeof(struct sockaddr_un)) {
				c = fubyte(cp++);
				if (c == -1) {
					u.u_error = EFAULT;
					return(-1);
				}
				if (c == 0) {
					*nlp = size;
					return(0);
				}
				size++;
			}
			u.u_error = EINVAL;
			return(-1);
		}
	}
	return(0);
}
