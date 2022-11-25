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

	char *
dknamer(chan)
{
	static char	dkname[22];

	sprintf(dkname, "/dev/dk/dk%.3d", chan);

	return(dkname);
}
