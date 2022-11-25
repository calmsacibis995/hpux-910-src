/*
 * $Header: nm_gen.c,v 1.5.83.4 93/09/17 19:04:31 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/nm_gen.c,v $
 * $Revision: 1.5.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:04:31 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_gen.c $Revision: 1.5.83.4 $";
#endif

#include "../h/types.h"
#include "../h/mib.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/malloc.h"
#include "../h/user.h"
#include "../h/file.h"		/* for FREAD def'n	*/
#include "../h/systm.h"		/* for selwait def'n	*/
#include "../h/proc.h"
#include "../h/uio.h"

#if defined(TRUX) && defined(B1)
#include "../h/security.h"
#endif

#include "../netinet/mib_kern.h"

int	mib_unsupp();
/*
 *	Switch Table for Net Mgmt groups
 */
struct  nmsw	nmsw[] = {		/* Net Mgmt Switch table */
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
	{ mib_unsupp,	mib_unsupp, mib_unsupp,   mib_unsupp,	},
};
/*
 *	Global variables for managing events in kernel
 */
struct nm_evcb {
	struct	evrec	*evfirst;	/* first unread event */
	struct	evrec	*evlast;	/* last recorded event */
	struct  proc	*selproc;
	int	evcount;		/* # unread events	*/
	int	evdrops;		/* # events droppped	*/
	short	pgrp;			/* ASYNC process group	*/
	u_short	flags;			/* flags	*/
};
struct nm_evcb	evcb;

#define	NMF_COLL	0x1
#define	NMF_NBIO	0x2
#define	NMF_ASYNC	0x4
#define	NMF_WAIT	0x8
/*
 *	routine to handle invalid calls
 */
mib_unsupp()
{
	return(EOPNOTSUPP);
}
/*
 *	routine used by subsystem to generate events
 */
nmevenq(evr)
	struct	evrec	*evr;
{
	int	s;

	s = splimp();
	if (evcb.evcount >= MAXEVNTS) {
		evcb.evdrops++;
		FREE (evr,M_TEMP);
		splx(s);
		return (ENOBUFS);
	}
	evr->ev.time.tv_sec = time.tv_sec;
	evr->evnext = NULL;
	if (evcb.evfirst == NULL)	/* first event */
		evcb.evfirst = evcb.evlast = evr;
	else	{
		evcb.evlast->evnext = evr;
		evcb.evlast = evr;
	}
	evcb.evcount ++;
	nm_wakeup();
	splx(s);
	return (NULL);

}
/*
 *	This routine wakes up those processes which might be waiting for 
 *	events to occur and send SIGIO signal if ASYNC IO is enabled.
 */
nm_wakeup()
{
	struct proc	*p;
	int		s;

	s = splimp();
	if (evcb.flags & NMF_WAIT) {
		wakeup(&evcb.evfirst);
		evcb.flags &= ~NMF_WAIT;
	}
	if (evcb.selproc) {
		selwakeup(evcb.selproc,NMF_COLL);
		evcb.selproc = 0;
		evcb.flags &= ~NMF_COLL;
	}
	if (evcb.flags & NMF_ASYNC) {
		if (evcb.pgrp > 0)
			gsignal (evcb.pgrp, SIGIO);
		else if (evcb.pgrp < 0 && (p=pfind(-evcb.pgrp)) != 0)
			psignal (p, SIGIO);
	}
	splx(s);
}
/*
 *	Open/Close routines for network mgmt character device driver
 */
nm_open(dev,flag)
	dev_t	dev;
	int	flag;
{
	return(NULL);
}
nm_close(dev,flag)
	dev_t	dev;
	int	flag;
{
	return(NULL);
}
/*
 *	This routine reads network management events
 */
nm_read(dev, uio)
	dev_t		dev;
	struct uio	*uio;
{
	struct	evrec	*save;
	int	evlen=0;
	int	error=0;
	int	s;

	if (uio->uio_resid < sizeof(struct event))
		return (EMSGSIZE);

	s = splimp();
	if (evcb.evfirst == NULL) {
		if (uio->uio_fpflags & FNDELAY) {
			splx(s);
			return (0);
		}

		if (evcb.flags & NMF_NBIO) {
			splx(s);
			return (EWOULDBLOCK);
		}

		evcb.flags |= NMF_WAIT;
		while (evcb.evfirst == NULL) {
			if (sleep (&evcb.evfirst, PCATCH|PZERO+1)) {
				evcb.flags &= ~NMF_WAIT;
				splx(s);
				return (EINTR);
			}
		}
	}
	/* 	Copy first unread event into evbuf	*/
	evlen = evcb.evfirst->ev.len + NMEVHDRLEN;
	error = uiomove (evcb.evfirst, evlen, UIO_READ, uio);
	if (error) {
		splx(s);
		return(error);
	}

	/* 	Update link for event queue and free event record */
	save 	= evcb.evfirst;
	evcb.evfirst	= save->evnext;
	FREE (save, M_TEMP);
	evcb.evcount --;
	if (evcb.evfirst==NULL)		/* read last event */
		evcb.evlast = NULL;
	splx(s);
	return(error);
}
/*
 *	Network Management ioctl's
 */
nm_ioctl(dev, cmd, arg, flag)
	dev_t	dev;
	int	cmd;
	struct nmparms	*arg;
	int	flag;
{
	int	status=0;
	int	*data=(int *)arg;


	switch (cmd) {

	case	NMIOGET :
		status = nmget(arg->objid,arg->buffer,arg->len);
		break;

	case	NMIOSET :
		status = nmset(arg->objid,arg->buffer,arg->len);
		break;

	case	NMIOCRE :
		status = nmcreate(arg->objid,arg->buffer,arg->len);
		break;

	case	NMIODEL :
		status = nmdelete(arg->objid,arg->buffer,arg->len);
		break;

	case	FIONBIO :
		if (*data)
			evcb.flags |= NMF_NBIO;
		else
			evcb.flags &= ~NMF_NBIO;
		status = NULL;
		break;

	case	FIOASYNC :
		if (*data)
			evcb.flags |= NMF_ASYNC;
		else
			evcb.flags &= ~NMF_ASYNC;
		status = NULL;
		break;

	case	SIOCSPGRP :
		evcb.pgrp = (short) (*data);
		status = NULL;
		break;

	case	SIOCGPGRP :
		*data = evcb.pgrp;
		status = NULL;
		break;

	default :
		return (EOPNOTSUPP);
	}
	return (status);
}
/*
 *	select routine for network mgmt driver
 */
nm_select(dev,which)
	dev_t	dev;
	int	which;
{
	int	s;
	switch (which) {

	case	FREAD :
		s = splimp();
		if (evcb.evfirst != NULL) {
			splx(s);
			return (1);
		}

		if (evcb.selproc && evcb.selproc->p_wchan == (caddr_t) &selwait)
			evcb.flags |= NMF_COLL;
		else
			evcb.selproc = u.u_procp;
		splx(s);
		return (0);
		break;

	default :	return(0);
	}
}
#define	GROUP(x) (x)>>16 & 0xFFF;
/*
 *	Generic, high-level nmget() system call
 */
nmget(objid, buffer, len)
	int	objid;
	char	*buffer;
	int	*len;
{

	int	group,klen=0;
	int	status=0;

	/*  validate group encoded in object identifier	 */
	group = GROUP(objid);
	if (group < 1 || group > GP_MAX) {
		u.u_error = EINVAL;
		return(EINVAL);
	}

	/* check for privileged user	*/
#if defined(TRUX) && defined(B1)
	if (!privileged(SEC_REMOTE,EPERM)) 
#else
	if (!suser())
#endif
		return(EPERM);

	if (status = copyin(len, &klen, sizeof(int)))
		return(status);

	/* switch to subsystem get procedure	*/
	status = (*nmsw[group].pr_get) (objid, buffer, &klen);
	if (status==NULL)
		status = copyout(&klen, len, sizeof(int));
	u.u_error = status;
	return (status);
}
/*
 *	Generic, high-level nmset() system call
 */
nmset(objid, buffer, len)
	int	objid;
	char	*buffer;
	int	*len;
{
	int	group,klen=0;
	int	status=0;

	group = GROUP(objid);
	if (group < 1 || group > GP_MAX) {
		u.u_error = EINVAL;
		return(EINVAL);
	}

#if defined(TRUX) && defined(B1)
	if (!privileged(SEC_REMOTE,EPERM)) 
#else
	if (!suser())
#endif
		return(EPERM);

	if (status = copyin(len, &klen, sizeof(int)))
		return(status);

	/* switch to subsystem get procedure	*/

	status = (*nmsw[group].pr_set) (objid, buffer, &klen);
	u.u_error = status;
	return (status);
}
nmcreate(objid, buffer, len)
	int	objid;
	char	*buffer;
	int	*len;
{
	int	group,klen=0;
	int	status=0;


	group = GROUP(objid);
	if (group != GP_ip) {
		u.u_error = EINVAL;
		return(EINVAL);
	}

#if defined(TRUX) && defined(B1)
	if (!privileged(SEC_REMOTE,EPERM)) 
#else
	if (!suser())
#endif
		return(EPERM);

	if (status = copyin(len, &klen, sizeof(int)))
		return(status);

        /* switch to subsystem create procedure    */

        status = (*nmsw[group].pr_create) (objid, buffer, &klen);
        u.u_error = status;
        return (status);

}

nmdelete(objid, buffer, len)
	int	objid;
	char	*buffer;
	int	*len;
{
	int	group,klen=0;
	int	status=0;

	group = GROUP(objid);
	if (group != GP_ip) {
		u.u_error = EINVAL;
		return(EINVAL);
	}

#if defined(TRUX) && defined(B1)
	if (!privileged(SEC_REMOTE,EPERM)) 
#else
	if (!suser())
#endif
		return(EPERM);

	if (status = copyin(len, &klen, sizeof(int)))
		return(status);

        /* switch to subsystem delete procedure    */

        status = (*nmsw[group].pr_delete) (objid, buffer, &klen);
        u.u_error = status;
        return (status);

}
