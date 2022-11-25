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

#include	<dk.h>

	static short	endoffile[3] = {
		106, 0, 0
	};	/* End of File Level-D code */

dkeof(fd)
{
	char	nothing[1];

	ioctl(fd, DIOCXCTL, endoffile);

	write(fd, nothing, 0);
}
