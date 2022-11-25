/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sysV_msg.c,v $
 * $Revision: 1.32.83.5 $       $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/09 14:33:10 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

/*
**	Inter-Process Communication Message Facility.
*/

#include "../h/param.h"
#ifdef __hp9000s800
#include "../h/sysmacros.h"
#include "../h/dir.h"
#endif
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../h/ipc.h"
#include "../h/msg.h"
#include "../h/systm.h"
#include "../h/vmmac.h"
#ifdef SAR
#include "../h/sar.h"
#endif
#ifdef MP
#include "../machine/reg.h"
#include "../machine/inline.h"
#endif /* MP */

extern struct ipcmap	msgmap[];	/* msg allocation map */
extern struct msqid_ds	msgque[];	/* msg queue headers */
extern struct msg	msgh[];		/* message headers */
extern struct msginfo	msginfo;	/* message parameters */
struct msg		*msgfp;	/* ptr to head of free header list */
caddr_t			msg;		/* base address of message buffer */
extern time_t		time;		/* system idea of date */

struct msqid_ds		*ipcgetperm(),
			*msgconv();

/* Convert bytes to msg segments. */
#define	btoq(X)	((X + msginfo.msgssz - 1) / msginfo.msgssz)

/* Choose appropriate message copy routine. */

#define	MOVE	iomove

/* flags indicating whether old or new msqid_ds is being used */
#define OLD_MSQID_DS	1
#define NEW_MSQID_DS	2

union msgunion {
	struct  msqid_ds        ds;     /* msqid_ds */
	struct  omsqid_ds       ods;    /* old msqid_ds structure
					 * supported for object code
					 * compatibility
					 */
};

/*
**	msgconv - Convert a user supplied message queue id into a ptr to a
**		msqid_ds structure.
*/

struct msqid_ds *
msgconv(id)
register int	id;
{
	register struct msqid_ds	*qp;	/* ptr to associated q slot */

	qp = &msgque[id % msginfo.msgmni];
	if((qp->msg_perm.mode & IPC_ALLOC) == 0 ||
#ifdef __hp9000s300
		(id < 0) ||
#endif
		id / msginfo.msgmni != qp->msg_perm.seq) {
		u.u_error = EINVAL;
		return(NULL);
	}
	return(qp);
}

/*
 *      omsgctl - Old msgctl system call.
 *                Retained for object code compatibility.
*/
omsgctl()
{
        msgctl1(OLD_MSQID_DS);
}

/*
**      msgctl - Msgctl system call.
*/
msgctl()
{
        msgctl1(NEW_MSQID_DS);
}

/*
**	msgctl1 - Main body of msgctl system call.
*/

msgctl1(msqid_ds_type)
int msqid_ds_type;	/* Is old or new msqid_ds being used? */
{
	register struct a {
		int		msgid,
				cmd;
		union msgunion	*arg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	union msgunion msgtmp;

	if((qp = msgconv(uap->msgid)) == NULL)
		return;
	if (ipc_lock((struct ipc_perm *)qp)) {
		/* The id was removed while sleeping on lock.         */
		/* u_error is already EINVAL, but we'll be consistant */
		/* and always set it explicitly in this file.         */
		/* (EIDRM is not appropriate for msgctl.)	      */
		u.u_error = EINVAL;
		return;
	}
	u.u_rval1 = 0;
	switch(uap->cmd) {
	case IPC_RMID:
		if(u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			&& !suser())
			break;
		while(qp->msg_first)
			msgfree(qp, (struct msg *)NULL, qp->msg_first);
		qp->msg_cbytes = 0;
		if(uap->msgid + msginfo.msgmni < 0)
			qp->msg_perm.seq = 0;
		else
			qp->msg_perm.seq++;
		if(qp->msg_perm.mode & MSG_RWAIT)
			wakeup( (caddr_t) &qp->msg_qnum);
		if(qp->msg_perm.mode & MSG_WWAIT) {
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup( (caddr_t) qp);
			wakeup( (caddr_t) &msgfp);
			wakeup( (caddr_t) msgmap);
		}
#ifdef __hp9000s800
		if( qp->msg_perm.mode & IPC_WANTED )
			wakeup(  (caddr_t) &(qp->msg_perm.mode) );
#endif
		ipc_unlock((struct ipc_perm *)qp);
		qp->msg_perm.mode = 0;
		return;  /* be sure not to unlock again */
	case IPC_SET:
		if(u.u_uid != qp->msg_perm.uid && u.u_uid != qp->msg_perm.cuid
			 && !suser())
			break;

		if (msqid_ds_type == NEW_MSQID_DS) {
			if(copyin((caddr_t) uap->arg, &msgtmp.ds,
				sizeof(msgtmp.ds))) {
				u.u_error = EFAULT;
				break;
			}
			if(msgtmp.ds.msg_qbytes > qp->msg_qbytes) {
				if(!suser())
					break;
				else if(qp->msg_perm.mode & MSG_WWAIT) {
					qp->msg_perm.mode &= ~MSG_WWAIT;
					wakeup( (caddr_t) qp);
					wakeup( (caddr_t) &msgfp);
					wakeup( (caddr_t) msgmap);
				}
			}
			qp->msg_perm.uid = msgtmp.ds.msg_perm.uid;
			qp->msg_perm.gid = msgtmp.ds.msg_perm.gid;
			qp->msg_perm.mode = (qp->msg_perm.mode & ~0777) |
				(msgtmp.ds.msg_perm.mode & 0777);
			qp->msg_qbytes = msgtmp.ds.msg_qbytes;
		}
		else {
			if(copyin((caddr_t) uap->arg, (caddr_t) &msgtmp.ods,
				sizeof(msgtmp.ods))) {
				u.u_error = EFAULT;
				break;
			}
			if(msgtmp.ods.msg_qbytes > qp->msg_qbytes) {
				if(!suser())
					break;
				else if(qp->msg_perm.mode & MSG_WWAIT) {
					qp->msg_perm.mode &= ~MSG_WWAIT;
					wakeup( (caddr_t) qp);
					wakeup( (caddr_t) &msgfp);
					wakeup( (caddr_t) msgmap);
				}
			}
			qp->msg_perm.uid = (uid_t) msgtmp.ods.msg_perm.uid;
			qp->msg_perm.gid = (gid_t) msgtmp.ods.msg_perm.gid;
			qp->msg_perm.mode = (qp->msg_perm.mode & ~0777) |
				(msgtmp.ods.msg_perm.mode & 0777);
			qp->msg_qbytes = msgtmp.ods.msg_qbytes;
		}
		qp->msg_ctime = time;
		break;
	case IPC_STAT:
		if(ipcaccess(&qp->msg_perm, MSG_R))
			break;
		if (msqid_ds_type == NEW_MSQID_DS) {
			if(copyout((caddr_t) qp, (caddr_t) uap->arg,
				sizeof(*qp))) {
				u.u_error = EFAULT;
				break;
			}
		}
		else {
                        /* Copy fields in the new msqid_ds structure to
                         * the old msqid_ds structure.  Note that there are
                         * old and new versions of struct ipc_perm as well.
                         */
                        msgtmp.ods.msg_perm.uid = (__ushort) qp->msg_perm.uid;
                        msgtmp.ods.msg_perm.gid = (__ushort) qp->msg_perm.gid;
                        msgtmp.ods.msg_perm.cuid = 
				(__ushort) qp->msg_perm.cuid;
                        msgtmp.ods.msg_perm.cgid = 
				(__ushort) qp->msg_perm.cgid;
                        msgtmp.ods.msg_perm.mode = qp->msg_perm.mode;
                        msgtmp.ods.msg_perm.seq = qp->msg_perm.seq;
                        msgtmp.ods.msg_perm.key = qp->msg_perm.key;
#ifdef __hp9000s800
                        msgtmp.ods.msg_perm.ndx = qp->msg_perm.ndx;
                        msgtmp.ods.msg_perm.wait = qp->msg_perm.wait;
#endif /* __hp9000s800 */
                        msgtmp.ods.msg_first = qp->msg_first;
                        msgtmp.ods.msg_last = qp->msg_last;
                        msgtmp.ods.msg_cbytes = qp->msg_cbytes;
                        msgtmp.ods.msg_qnum = qp->msg_qnum;
                        msgtmp.ods.msg_qbytes = qp->msg_qbytes;
                        msgtmp.ods.msg_lspid = (__ushort) qp->msg_lspid;
                        msgtmp.ods.msg_lrpid = (__ushort) qp->msg_lrpid;
                        msgtmp.ods.msg_stime = qp->msg_stime;
                        msgtmp.ods.msg_rtime = qp->msg_rtime;
                        msgtmp.ods.msg_ctime = qp->msg_ctime;

			if(copyout((caddr_t) &msgtmp.ods, (caddr_t) uap->arg,
				sizeof(msgtmp.ods))) {
				u.u_error = EFAULT;
				break;
			}
		}
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
	ipc_unlock ((struct ipc_perm *)qp);
}

/*
**	msgfree - Free up space and message header, relink pointers on q,
**	and wakeup anyone waiting for resources.
*/

msgfree(qp, pmp, mp)
register struct msqid_ds	*qp;	/* ptr to q of mesg being freed */
register struct msg		*mp,	/* ptr to msg being freed */
				*pmp;	/* ptr to mp's predecessor */
{
	/* Unlink message from the q. */
	if(pmp == NULL)
		qp->msg_first = mp->msg_next;
	else
		pmp->msg_next = mp->msg_next;
	if(mp->msg_next == NULL)
		qp->msg_last = pmp;
	qp->msg_qnum--;
	if(qp->msg_perm.mode & MSG_WWAIT) {
		qp->msg_perm.mode &= ~MSG_WWAIT;
		wakeup( (caddr_t) qp);
		wakeup( (caddr_t) &msgfp);
		wakeup( (caddr_t) msgmap);
	}

	/* Free up message text. */
	if(mp->msg_ts)
		ipcmfree(msgmap, btoq(mp->msg_ts), 
					(unsigned int)mp->msg_spot + 1);

	/* Free up header */
	mp->msg_next = msgfp;
	if(msgfp == NULL) {
		qp->msg_perm.mode &= ~MSG_WWAIT;
		wakeup( (caddr_t) qp);
		wakeup( (caddr_t) &msgfp);
		wakeup( (caddr_t) msgmap);
	}
	msgfp = mp;
}

/*
**	msgget - Msgget system call.
*/

msgget()
{
	register struct a {
		key_t	key;
		int	msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				s;	/* ipcgetperm status return */

	if((qp = ipcgetperm(uap->key, uap->msgflg, (struct ipc_perm *)msgque,
				msginfo.msgmni, sizeof(*qp), &s)) == NULL)
		return;

	if(s) {
		/* This is a new queue.  Finish initialization. */
		qp->msg_first = qp->msg_last = NULL;
		qp->msg_qnum = 0;
		qp->msg_qbytes = msginfo.msgmnb;
		qp->msg_lspid = qp->msg_lrpid = 0;
		qp->msg_stime = qp->msg_rtime = 0;
		qp->msg_ctime = time;
	}
	u.u_rval1 = qp->msg_perm.seq * msginfo.msgmni + (qp - msgque);
}

/*
**	msginit - Called by main(main.c) to initialize message queues.
*/

msginit()
{
	register int		i;	/* loop control */
	register struct msg	*mp;	/* ptr to msg begin linked */
	register int		bs;	/* message buffer size */

	/* Sanity check against SVID limitation (they use shorts) */
	if (((unsigned)msginfo.msgmax) > 65535) {
		printf("WARNING: max message size > 65535, adjusting.\n");
		msginfo.msgmax = 65535;
	}
	if (((unsigned)msginfo.msgmnb) > 65535) {
		printf("WARNING: max bytes on msg q > 65535, adjusting.\n");
		msginfo.msgmnb = 65535;
	}

	/* Allocate physical memory for message buffer. */
	bs=btop(msginfo.msgseg * msginfo.msgssz);
	if((msg = (caddr_t)wmemall(memall,
		msginfo.msgseg * msginfo.msgssz)) == NULL) {
		printf("Can't allocate message buffer.\n");
		msginfo.msgseg = 0;
	}
 	ipcmapinit(msgmap, msginfo.msgmap);
 	ipcmfree(msgmap, msginfo.msgseg, 1);
	for(i = 0, mp = msgfp = msgh;++i < msginfo.msgtql;mp++)
		mp->msg_next = mp + 1;
}

/*
**	msgrcv - Msgrcv system call.
*/

msgrcv()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msg		*mp,	/* ptr to msg on q */
					*pmp,	/* ptr to mp's predecessor */
					*smp,	/* ptr to best msg on q */
					*spmp;	/* ptr to smp's predecessor */
	register struct msqid_ds	*qp;	/* ptr to associated q */
	int				sz;	/* transfer byte count */
	register u_int			reg_temp;

#ifdef SAR
	msgcnt++;	/* sar */
#endif

	if((qp = msgconv(uap->msqid)) == NULL)
		return;
	if(ipcaccess(&qp->msg_perm, MSG_R))
		return;
	if(uap->msgsz < 0) {
		u.u_error = EINVAL;
		return;
	}
	smp = spmp = NULL;
findmsg:
	if (ipc_lock((struct ipc_perm *)qp)) {
		/* The id was removed while sleeping on lock, */
		/* but EINVAL is not appropriate.             */
		u.u_error = EIDRM;
		return;
	}

	pmp = NULL;
	mp = qp->msg_first;
	if(uap->msgtyp == 0)
		smp = mp;
	else
		for(;mp;pmp = mp, mp = mp->msg_next) {
			if(uap->msgtyp > 0) {
				if(uap->msgtyp != mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
				break;
			}
			if(mp->msg_type <= -uap->msgtyp) {
				if(smp && smp->msg_type <= mp->msg_type)
					continue;
				smp = mp;
				spmp = pmp;
			}
		}
	if(smp) {
		if(uap->msgsz < smp->msg_ts)
			if(!(uap->msgflg & MSG_NOERROR)) {
				u.u_error = E2BIG;
				ipc_unlock ((struct ipc_perm *)qp);
				return;
			} else
				sz = uap->msgsz;
		else
			sz = smp->msg_ts;
		if(u.u_error =
		   copyout(&smp->msg_type, uap->msgp, sizeof(smp->msg_type))) {
			ipc_unlock ((struct ipc_perm *)qp);
			return;
		}
		if(sz) {
			if (u.u_error = 
				copyout(msg + msginfo.msgssz * smp->msg_spot,
				     (caddr_t)uap->msgp + sizeof(smp->msg_type),
				     sz)) {
				ipc_unlock ((struct ipc_perm *)qp);
				return;
			}
		}
		u.u_rval1 = sz;
		qp->msg_cbytes -= smp->msg_ts;
		qp->msg_lrpid = u.u_procp->p_pid;
		qp->msg_rtime = time;
#ifdef MP
		SETCURPRI(PMSG, reg_temp);
#else
		curpri = PMSG;
#endif
		msgfree(qp, spmp, smp);
		ipc_unlock ((struct ipc_perm *)qp);
		return;
	}
	ipc_unlock ((struct ipc_perm *)qp);
	if(uap->msgflg & IPC_NOWAIT) {
		u.u_error = ENOMSG;
		return;
	}
	qp->msg_perm.mode |= MSG_RWAIT;
	if(sleep(&qp->msg_qnum, PMSG | PCATCH)) {
		u.u_error = EINTR;
		qp->msg_perm.mode &= ~MSG_RWAIT;
		wakeup( (caddr_t) &qp->msg_qnum);
		return;
	}
	if(msgconv(uap->msqid) == NULL) {
		u.u_error = EIDRM;
		return;
	}
	goto findmsg;
}

/*
**	msgsnd - Msgsnd system call.
*/

msgsnd()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;
	register struct msqid_ds	*qp;	/* ptr to associated q */
	register struct msg		*mp;	/* ptr to allocated msg hdr */
	register int			cnt,	/* byte count */
					spot;	/* msg pool allocation spot */
	long				type;	/* msg type */
	register u_int			reg_temp;

	if(u.u_error = copyin(uap->msgp, &type, sizeof(type)))
		return;
	if(type < 1) {
		u.u_error = EINVAL;
		return;
	}
	if((qp = msgconv(uap->msqid)) == NULL)
		return;
	if(ipcaccess(&qp->msg_perm, MSG_W))
		return;
	if((cnt = uap->msgsz) < 0 || cnt > msginfo.msgmax) {
		u.u_error = EINVAL;
		return;
	}
getres:
	/* Be sure that q has not been removed. */
	if(msgconv(uap->msqid) == NULL) {
		u.u_error = EIDRM;
		return;
	}

	if (ipc_lock((struct ipc_perm *)qp)) {
		/* The id was removed while sleeping on lock, */
		/* but EINVAL is not appropriate.             */
		u.u_error = EIDRM;
		return;
	}

	/* Allocate space on q, message header, & buffer space. */
	if(cnt + qp->msg_cbytes > qp->msg_qbytes) {
		ipc_unlock((struct ipc_perm *)qp);
		if(uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			return;
		}
		qp->msg_perm.mode |= MSG_WWAIT;
		if(sleep(qp, PMSG | PCATCH)) {
			u.u_error = EINTR;
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup( (caddr_t) qp);
			wakeup( (caddr_t) &msgfp);
			wakeup( (caddr_t) msgmap);
			return;
		}
		goto getres;
	}
	if(msgfp == NULL) {
		ipc_unlock((struct ipc_perm *)qp);
		if(uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			return;
		}
		qp->msg_perm.mode |= MSG_WWAIT;
		if(sleep(&msgfp, PMSG | PCATCH)) {
			u.u_error = EINTR;
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup( (caddr_t) qp);
			wakeup( (caddr_t) &msgfp);
			wakeup( (caddr_t) msgmap);
			return;
		}
		goto getres;
	}
	/*
	 * Must actually allocate before copyin
	 * (which can sleep or page fault).
	 */
	mp = msgfp;
	msgfp = mp->msg_next;
	mp->msg_next = NULL;

	if(cnt && (spot = ipcmalloc(msgmap, btoq(cnt))) == NULL) {
		mp->msg_next = msgfp;
		if(msgfp == NULL) {
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup( (caddr_t) qp);
			wakeup( (caddr_t) &msgfp);
			wakeup( (caddr_t) msgmap);
		}
		msgfp = mp;
		ipc_unlock((struct ipc_perm *)qp);
		if(uap->msgflg & IPC_NOWAIT) {
			u.u_error = EAGAIN;
			return;
		}
		ipcmapwant(msgmap)++;
		qp->msg_perm.mode |= MSG_WWAIT;
		if(sleep(msgmap, PMSG | PCATCH)) {
			u.u_error = EINTR;
			qp->msg_perm.mode &= ~MSG_WWAIT;
			wakeup( (caddr_t) qp);
			wakeup( (caddr_t) &msgfp);
			wakeup( (caddr_t) msgmap);
			return;
		}
		goto getres;
	}

	/* Everything is available, copy in text and put msg on q. */
	if(cnt) {
		u.u_error = copyin((caddr_t)uap->msgp + sizeof(type),
				   msg + msginfo.msgssz * --spot, cnt);
		if(u.u_error) {
			/* Return msg header to free list */
			mp->msg_next = msgfp;
			if(msgfp == NULL) {
				qp->msg_perm.mode &= ~MSG_WWAIT;
				wakeup( (caddr_t) qp);
				wakeup( (caddr_t) &msgfp);
				wakeup( (caddr_t) msgmap);
			}
			msgfp = mp;

			ipcmfree(msgmap, btoq(cnt), (unsigned int)spot + 1);
			ipc_unlock((struct ipc_perm *)qp);
			return;
		}
	}
	qp->msg_qnum++;
	qp->msg_cbytes += cnt;
	qp->msg_lspid = u.u_procp->p_pid;
	qp->msg_stime = time;
#ifdef __hp9000s300
	mp->msg_next = NULL;
#endif
	mp->msg_type = type;
	mp->msg_ts = cnt;
	mp->msg_spot = cnt ? spot : -1;
	if(qp->msg_last == NULL)
		qp->msg_first = qp->msg_last = mp;
	else {
		qp->msg_last->msg_next = mp;
		qp->msg_last = mp;
	}
	if(qp->msg_perm.mode & MSG_RWAIT) {
		qp->msg_perm.mode &= ~MSG_RWAIT;
#ifdef MP
		SETCURPRI(PMSG, reg_temp);
#else
		curpri = PMSG;
#endif
		wakeup( (caddr_t) &qp->msg_qnum);
	}
	u.u_rval1 = 0;
	ipc_unlock((struct ipc_perm *)qp);
}
