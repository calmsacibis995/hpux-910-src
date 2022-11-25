/* $Header: sys_socket.c,v 1.37.83.4 93/09/18 06:44:34 root Exp $ */

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
 *	@(#)sys_socket.c	7.2 (Berkeley) 3/31/88
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/stat.h"
#include "../h/netfunc.h"
#ifdef _WSIO      
#include "../h/poll.h"
#endif

#include "../net/if.h"
#include "../net/route.h"

#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */

int	soo_rw(), soo_ioctl(), soo_select(), soo_close();
struct	fileops socketops =
    { soo_rw, soo_ioctl, soo_select, soo_close };

soo_rw(fp, rw, uio)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uio;
{
	int soreceive(), sosend();

	if (rw == UIO_READ) {
		int flags=0;
		return(soreceive((struct socket *)fp->f_data, 0, uio, &flags, 0,
		       (struct mbuf*)0));
	}
	else {
		return(sosend((struct socket *)fp->f_data, 0, uio, 0, 0,
		       (struct mbuf*)0));
	}
}


soo_ioctl(fp, cmd, data)
	struct file *fp;
	int cmd;
	register caddr_t data;
{
	register struct socket *so = (struct socket *)fp->f_data;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int result, oldlevel;

	switch (cmd) {

#ifdef hp9000s800
        case FIOSNBIO:
#endif
	case FIONBIO:
		/*MPNET: turn MP protection on */
		NETMP_GO_EXCLUSIVE(oldlevel, savestate);
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);

		/*MPNET: MP protection is now off. */
		return (0);

	case FIOASYNC:
		/*MPNET: turn MP protection on */
		NETMP_GO_EXCLUSIVE(oldlevel, savestate);
		if (*(int *)data)
			so->so_state |= SS_ASYNC;
		else
			so->so_state &= ~SS_ASYNC;
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);

		/*MPNET: MP protection is now off. */
		return (0);

	case FIONREAD:
		*(int *)data = so->so_rcv.sb_cc;/* No MPNET protection */
		return (0);

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;/* No MPNET protection */
		return (0);

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;/* No MPNET protection */
		return (0);

	case SIOCATMARK:/* No MPNET protection */
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		return (0);

	case SIOCJNVS:
		/*MPNET: MP protection supplied in nvs_ioc_join() */
		result = NETCALL(NET_NVSJOIN)(so, *(int *)data);
		return(result);
	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff)

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);
	if (cmdbyte(cmd) == 'i')
	    result = NETCALL(NET_IFIOCTL)(so, cmd, data);
	else if (cmdbyte(cmd) == 'r') 
	    result = NETCALL(NET_RTIOCTL)(cmd, data);
	else result = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL, 
	    (struct mbuf *)cmd, (struct mbuf *)data, (struct mbuf *)0));
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	return(result);
}

soo_select(fp, which)
	struct file *fp;
	int which;
{
	register struct socket *so = (struct socket *)fp->f_data;
	int oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	switch (which) {

	case FREAD:
		if (soreadable(so)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;

	case FWRITE:
		if (sowriteable(so)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			return (1);
		}
		sbselqueue(&so->so_snd);
		break;

#ifdef _WSIO      
	case POLLERR:
		if (so->so_error) {
                        NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
                        /*MPNET: MP protection is now off. */
                        return (1);
                }
                sbselqueue(&so->so_rcv);
                break;

	case POLLHUP:
		if (so->so_state & SS_CANTRCVMORE) {
                        NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
                        /*MPNET: MP protection is now off. */
                        return (1);
                }
                sbselqueue(&so->so_rcv);
                break;

	case POLLPRI:
#endif
	case 0 :
		if (so->so_oobmark ||
		    (so->so_state & SS_RCVATMARK)) {
			NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
			/*MPNET: MP protection is now off. */
			return (1);
		}
		sbselqueue(&so->so_rcv);
		break;
	}
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
	return (0);
}

/*ARGSUSED*/
soo_stat(so, ub)
	register struct socket *so;
	register struct stat *ub;
{
	int result, oldlevel;
	sv_sema_t savestate;		/* MPNET: MP save state */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate); /*MPNET: MP prot on */

	bzero((caddr_t)ub, sizeof (*ub));
	result = (*so->so_proto->pr_usrreq)(so, PRU_SENSE,
	    (struct mbuf *)ub, (struct mbuf *)0, 
	    (struct mbuf *)0);
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);/*MPNET: MP prot off*/
	return (result);
}

soo_close(fp)
	struct file *fp;
{
	int error = 0;

	if (fp->f_data) {
		error = soclose((struct socket *)fp->f_data);
	    /* Lock File Table. */
	    /* MPNET Rule #1D */
	    SPINLOCK(file_table_lock);
	fp->f_data = 0;
	    SPINUNLOCK(file_table_lock);
	}
	return (error);
}
