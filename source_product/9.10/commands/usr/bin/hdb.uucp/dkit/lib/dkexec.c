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

#include <dk.h>
#include <stdio.h>
#include <string.h>

	char		*getenv();

dkexec(hname, cmd, arglen)
	char	*hname, *cmd;
	int	arglen;
{
	int	rem, exitcode;
	int	env, minusc;
	short	psize;
	char	*p, *system;
	char	ca[3];

	system = maphost(hname, 'x', "rx", "aevx", "");

	env = 1;
	if(p = miscfield('x', 'v'))
		env = (*p == 'y');
	minusc = (miscfield('x', 's') != NULL);

	rem = dkdial(system);
	if (rem < 0)
		return(rem);

	exitcode = 0;

	if(env)
		exitcode = dkxenviron(rem);

	psize = arglen;
	if(minusc)
		psize += 3;
	dktcanon("s", &psize, ca);

	if(exitcode >= 0)
		exitcode = dkxwrite(rem, ca, 2);

	if(exitcode >= 0)
		exitcode = dkxpwrite(rem, psize);

	if(exitcode >= 0){
		if(minusc)
			write(rem, "-c", 3);
		exitcode = write(rem, cmd, arglen);
	}

	if(exitcode >= 0)
		exitcode = dkxstdio(rem);

	close(rem);
	return(exitcode);
}
