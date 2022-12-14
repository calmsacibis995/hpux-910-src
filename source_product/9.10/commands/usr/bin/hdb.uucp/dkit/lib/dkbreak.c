/*	@(#) $Revision: 64.2 $	*/

/*	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */
#include	<dk.h>

	static short	sendbreak[3] = {
		72, 0, 0
	};	/* Asynch Break Level-D code */

dkbreak(fd)
{
	char	nothing[1];

	ioctl(fd, DIOCXCTL, sendbreak);

	write(fd, nothing, 0);
}
