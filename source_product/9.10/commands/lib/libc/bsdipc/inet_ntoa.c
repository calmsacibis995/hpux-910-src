/*	@(#)$Header: inet_ntoa.c,v 66.1 90/01/10 13:20:16 jmc Exp $	*/
/*
 * Copyright (c) 1983 Regents of the University of California.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)inet_ntoa.c 5.4 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */

/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */

#ifdef _NAMESPACE_CLEAN
#define ultoa _ultoa
#define strcat _strcat
#define strcpy _strcpy
#endif

#include <sys/types.h>
#include <netinet/in.h>

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _inet_ntoa inet_ntoa
#define inet_ntoa _inet_ntoa
#endif

char *
inet_ntoa(in)
	struct in_addr in;
{
	extern char *ultoa();
	static char b[18];
	register char *p;

	p = (char *)&in;
#define	UC(b)	(((int)b)&0xff)
	/* Changed from sprintf() */
	strcpy(b, ultoa((unsigned)UC(p[0])));
	strcat(b, ".");
	strcat(b, ultoa((unsigned)UC(p[1])));
	strcat(b, ".");
	strcat(b, ultoa((unsigned)UC(p[2])));
	strcat(b, ".");
	strcat(b, ultoa((unsigned)UC(p[3])));
	return (b);
}
