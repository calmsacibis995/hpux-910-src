/*	@(#) $Revision: 64.1 $	*/
/*
 *	Wait for a DATAKIT splice to take place.
 *	This routine should be called from the
 *	target system, not the splice initiator.
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

dksplwait(fd)
int fd;			/* the file descriptor being spliced */
{
	if (ioctl(fd, DIOCSWAIT, 0) < 0) {
		return(-1);
	}

	return(0);
}
