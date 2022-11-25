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
#include <sys/types.h>
#include <sys/remfio.h>
#include <stdio.h>
#include <sysexits.h>

	extern int	dk_verbose;

/*
 * Write a length-preceeded block over a dkxqt protocol channel
 *
 *	netfd	= file descriptor to use
 *	addr	= character pointer to beginning of block
 *	len	= number of bytes in block
 *	return	= >0 : write successful
 *		| <0 : write failed
 */

dkxlwrite(netfd, addr, len)
	char			*addr;
{
	int			n;
	short			slen;
	char			clen[3];

	slen = len;

	dktcanon("s", &slen, clen);

	if((n = dkxwrite(netfd, clen, 2)) < 0)
		return(n);

	if(len > 0)
		n = dkxwrite(netfd, addr, len);

	return(n);
}

/*
 * Write a block over a dkxqt protocol channel
 *
 *	netfd	= file descriptor to use
 *	addr	= character pointer to beginning of block
 *	len	= number of bytes in block
 *	return	= >0 : write successful
 *		| <0 : write failed
 */

dkxwrite(netfd, addr, len)
	char			*addr;
{
	int			n;

	if((n = dkxpwrite(netfd, len)) < 0)
		return(n);

	return(write(netfd, addr, len));
}

/*
 * Prepare to write a block over a dkxqt protocol channel
 *
 * Write the protocol envelope necessary before sending the block itself
 *
 *	netfd	= file descriptor to use
 *	len	= number of bytes in block
 *	return	= >0 : write successful
 *		| <0 : write failed
 */

dkxpwrite(netfd, len)
{
	int			n;
	struct rem_req		r;
	struct rem_reply	s;
	char			sb[REMSIZE], rb[REMSIZE];

	n = read(netfd, rb, REMSIZE);
	dkfcanon(F_REMREQ, rb, &r);

	if(n != REMSIZE || r.r_type != RREAD || r.r_var.rread.r_count != len){
		if(dk_verbose)
			fprintf(stderr, "dkxpwrite: REMREQ wrong\n");
		return(-EX_PROTOCOL);
	}

	s.s_type = RREAD;
	s.s_length = len;
	s.s_error = 0;
	s.s_resid = 0;
	dktcanon(F_REMREPLY, &s, sb) ;

	return(write(netfd, sb, REMSIZE));
}
