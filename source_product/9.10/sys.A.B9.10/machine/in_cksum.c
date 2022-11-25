/*
 * $Header: in_cksum.c,v 1.7.84.4 93/10/20 11:08:51 rpc Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/in_cksum.c,v $
 * $Revision: 1.7.84.4 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/20 11:08:51 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) in_cksum.c $Revision: 1.7.84.4 $";
#endif

/*
 * Copyright (c) 1988 Regents of the University of California.
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
 *	@(#)in_cksum.c	7.2 (Berkeley) 6/29/88
 */

#include "../h/types.h"
#include "../h/mbuf.h"
/*
 * Checksum routine for Internet Protocol family headers (Tuned Version for 68K).
 */

in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register u_short *w;
	register int sum = 0;
	register int mlen = 0;
	char 	temp_char;
	union {
		char c[2];
		u_short s;
	} temp_sum;
	u_short prev_odd = 0;

	for (;m && len; m = m->m_next) {
		if (m->m_len == 0)
			continue;

		/* get next buffer to sum */
		w = mtod(m, u_short *);
		mlen = m->m_len;

		/* if too long, shorten it */
		if (len < mlen)
			mlen = len;

		/* subtract this buffer from total count */
		len -= mlen;

		/* sum in next mbuf */
		temp_sum.s = scksum(w, mlen);

		/* if odd number of bytes, swap bytes in checksum */
		if (prev_odd) {
			temp_char     = temp_sum.c[0];
			temp_sum.c[0] = temp_sum.c[1];
			temp_sum.c[1] = temp_char;
		}
		sum += temp_sum.s;
		/* wrap around carry */
		if (sum > 65535) sum -= 65535;

		/* odd number for next sum ?                */
		prev_odd = (prev_odd == (1 & mlen)) ? 0 : 1;
	}
	if (len)
		printf("cksum: out of data\n");
	return (~sum & 0xffff);
}

#ifdef NEVER_CALLED
/* HP : stubs for checksum offload.
 *	checksum offload supported only on S800.
 */
ip_cksum()
{
	panic( "ip_cksum" );
	return(0);
}
#endif /* NEVER_CALLED */

in_2sum()
{
	panic( "in_2sum" );
	return(0);
}

in_3sum()
{
	panic( "in_3sum" );
	return(0);
}

