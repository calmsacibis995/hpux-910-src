/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sysV_ipc.c,v $
 * $Revision: 1.17.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:11:27 $
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

#include "../h/param.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/signal.h"
#include "../h/ipc.h"

/*
**	Common IPC routines.
*/

/*
**	Check message, semaphore, or shared memory access permissions.
**
**	This routine verifies the requested access permission for the current
**	process.  Super-user is always given permission.  Otherwise, the
**	appropriate bits are checked corresponding to owner, group, or
**	everyone.  Zero is returned on success.  On failure, u.u_error is
**	set to EACCES and one is returned.
**	The arguments must be set up as follows:
**		p - Pointer to permission structure to verify
**		mode - Desired access permissions
*/

ipcaccess(p, mode)
register struct ipc_perm	*p;
register ushort			mode;
{
	register gid_t *gp;

	if(u.u_uid == 0)
		return(0);
	if(u.u_uid != p->uid && u.u_uid != p->cuid) {
		mode >>= 3;
		if(u.u_gid == p->gid || u.u_gid == p->cgid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (p->gid == *gp || p->cgid == *gp)
				goto found;
		mode >>= 3;
found:
		;
	}
	if(mode & p->mode)
		return(0);
	u.u_error = EACCES;
	return(1);
}

/*
**	Get message, semaphore, or shared memory structure.
**
**	This routine searches for a matching key based on the given flags
**	and returns a pointer to the appropriate entry.  A structure is
**	allocated if the key doesn't exist and the flags call for it.
**	The arguments must be set up as follows:
**		key - Key to be used
**		flag - Creation flags and access modes
**		base - Base address of appropriate facility structure array
**		cnt - # of entries in facility structure array
**		size - sizeof(facility structure)
**		status - Pointer to status word: set on successful completion
**			only:	0 => existing entry found
**				1 => new entry created
**	Ipcgetperm returns NULL with u.u_error set to an appropriate value if
**	it fails, or a pointer to the initialized entry if it succeeds.
*/

struct ipc_perm *
ipcgetperm(key, flag, base, cnt, size, status)
register struct ipc_perm	*base;
int				cnt,
				flag,
				size,
				*status;
key_t				key;
{
	register struct ipc_perm	*a;	/* ptr to available entry */
	register int			i;	/* loop control */

	if(key == IPC_PRIVATE) {
		for(i = 0;i++ < cnt;
			base = (struct ipc_perm *)(((char *)base) + size)) {
			if(base->mode & IPC_ALLOC)
				continue;
			goto init;
		}
		u.u_error = ENOSPC;
		return(NULL);
	} else {
		for(i = 0, a = NULL;i++ < cnt;
			base = (struct ipc_perm *)(((char *)base) + size)) {
			if(base->mode & IPC_ALLOC) {
				if(base->key == key) {
					if((flag & (IPC_CREAT | IPC_EXCL)) ==
						(IPC_CREAT | IPC_EXCL)) {
						u.u_error = EEXIST;
						return(NULL);
					}
					if((flag & 0777) & ~base->mode) {
						u.u_error = EACCES;
						return(NULL);
					}
					*status = 0;
					return(base);
				}
				continue;
			}
			if(a == NULL)
				a = base;
		}
		if(!(flag & IPC_CREAT)) {
			u.u_error = ENOENT;
			return(NULL);
		}
		if(a == NULL) {
			u.u_error = ENOSPC;
			return(NULL);
		}
		base = a;
	}
init:
	*status = 1;
	base->mode = IPC_ALLOC | (flag & 0777);
	base->key = key;
	base->cuid = base->uid = u.u_uid;
	base->cgid = base->gid = u.u_gid;
	return(base);
}

/*
**	ipc_lock - locks the ipc structure.  Returns zero for success,
**		non-zero (setting u_error) for failure.
**
**		The only error case is where the ID has been removed while
**		blocked waiting for the lock.  The intuitive error here is
**		EIDRM.  However, since that error is only specified by the
**		manual when a system call needs to block for user-perceived
**		reasons (ie. semop and msgop), in other cases it is a small
**		race condition where the most appropriate error is EINVAL
**		(the same as if the ID were removed before the call).  Since
**		there are currently more calls to this routine where EINVAL
**		is appropriate, that is the error we return.  The callers
**		for whom EIDRM is more appropriate must then set u_error
**		themselves.
*/

int ipc_lock (base)
register struct ipc_perm	*base;
{
	ushort	orig_seq;

	orig_seq = base->seq;
	while (base->mode & IPC_LOCKED) {
		base->mode |= IPC_WANTED;
		sleep (&base->mode, PINOD);
		if ( !(base->mode & IPC_ALLOC) || base->seq != orig_seq) {
			u.u_error = EINVAL;
			return (1);
		}
	} 
#ifdef hp9000s800
	/* (for debugging purposes) record who has lock */
	base->ndx = pindx(u.u_procp);
#endif
	base->mode |= IPC_LOCKED;
	return (0);
}

/*
**	ipc_unlock - releases the lock from ipc_lock
*/

void ipc_unlock (base)
register struct ipc_perm	*base;
{
	base->mode &= ~IPC_LOCKED;
#ifdef hp9000s800
	base->ndx = 0;
#endif
	if (base->mode & IPC_WANTED) {
		base->mode &= ~IPC_WANTED;
		wakeup ((caddr_t)&base->mode);
	}
}
