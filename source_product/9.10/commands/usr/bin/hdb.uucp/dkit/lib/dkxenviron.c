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

/*
 * Transmit environment variables to a remote host
 *
 * The environment variables to be sent are listed, separated by commas,
 * in the environment variable DKEXPORT.  They are all sent as a series of
 * null-terminated strings using the dkxqt protocol.
 *
 *	netfd	= file descriptor of an open connection to the remote host
 *	return	= >0 : write successful
 *		| <0 : write failed
 */

dkxenviron(netfd)
{
	register char	*envlist, *envnext, *ap, *ep;
	short		len;
	char		envarray[2048];

	if(!(envlist = getenv("DKEXPORT")))
		envlist = "";

	ap = envarray;

	do{
		if(envnext = strchr(envlist, ','))
			*envnext = '\0';

		if(ep = getenv(envlist)){
			sprintf(ap, "%s=%s", envlist, ep);

			ap += strlen(ap) + 1;
		}

		if(envnext)
			*envnext++ = ',';
	}while(envlist = envnext);

	len = ap - envarray;

	return(dkxlwrite(netfd, envarray, len));
}
