/*	@(#) $Revision: 64.1 $	*/
/*
 *	splice two DATAKIT channels together
 */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include <dk.h>

dksplice(fdout, fdin)
int fdout, fdin;
{
	struct diocspl iocb;

	iocb.spl_fdin = fdin;
	if (ioctl(fdout, DKIOCSPL, &iocb) < 0) {
		return(-1);
	}
	close(fdin);
	close(fdout);
	return(0);
}
