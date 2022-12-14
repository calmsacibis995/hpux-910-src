/*	@(#) $Revision: 64.1 $	*/
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */
/*
   Return minor device number for a given Datakit device.
   The channel number is used since using the minor device returned
   by fstat gives wrong results for duplex systems.
*/

#include <dk.h>
dkminor(fd)
{
	struct diocreq iocb;


	if (ioctl(fd, DIOCINFO, &iocb) < 0)
		return(-1);
	return(iocb.req_chmin); /* req_chmin contains channel number */
}
