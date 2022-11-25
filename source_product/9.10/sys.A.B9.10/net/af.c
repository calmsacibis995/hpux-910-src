/*
 * $Header: af.c,v 1.11.83.4 93/09/17 18:59:42 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/af.c,v $
 * $Revision: 1.11.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 18:59:42 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) af.c $Revision: 1.11.83.4 $";
#endif

/*
 * Copyright (c) 1983, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)af.c	7.4 (Berkeley) 6/27/88
 */

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../net/af.h"

/*
 * Address family support routines
 */
int	null_hash(), null_netmatch();
#define	AFNULL \
	{ null_hash,	null_netmatch }

struct afswitch afswitch[AF_MAX] = {
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,
	AFNULL,	AFNULL,	AFNULL,	AFNULL,	AFNULL,
	AFNULL, AFNULL, AFNULL, AFNULL, AFNULL,
	AFNULL, AFNULL,					/* through 16 */
};

null_init()
{
	register struct afswitch *af;

	for (af = afswitch; af < &afswitch[AF_MAX]; af++)
		if (af->af_hash == (int (*)())NULL) {
			af->af_hash = null_hash;
			af->af_netmatch = null_netmatch;
		}
}

/*ARGSUSED*/
null_hash(addr, hp)
	struct sockaddr *addr;
	struct afhash *hp;
{

	hp->afh_nethash = hp->afh_hosthash = 0;
}

/*ARGSUSED*/
null_netmatch(a1, a2)
	struct sockaddr *a1, *a2;
{

	return (0);
}
