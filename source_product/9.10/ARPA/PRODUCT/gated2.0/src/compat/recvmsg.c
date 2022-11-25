/*
 *  $Header: recvmsg.c,v 1.1.109.4 92/01/20 18:05:13 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


#include "include.h"

#ifdef	NEED_RECVMSG

static caddr_t buf_base;
static int buf_size;

int
recvmsg(s, msg, flags)
int s;
struct msghdr *msg;
int flags;
{
    register int i, cc, acc;
    extern int errno;
    extern char etext;
    extern char *sbrk();
    char *buf_ptr;

    if ((msg->msg_iovlen < 1) || (msg->msg_iovlen > MSG_MAXIOVLEN)) {
	errno = EINVAL;
	return (-1);
    }
#ifdef	SCM_RIGHTS
    if (msg->msg_control) {
	*msg->msg_control = (char) 0;
    }
#else				/* SCM_RIGHTS */
    if (msg->msg_accrights) {
	*msg->msg_accrights = (char) 0;
    }
#endif				/* SCM_RIGHTS */

    /* only 1 buffer - receive the data directly */
    if (msg->msg_iovlen == 1) {
	acc = recvfrom(s,
		       msg->msg_iov->iov_base,
		       msg->msg_iov->iov_len,
		       flags,
		       msg->msg_name,
		       &msg->msg_namelen);
	return (acc);
    }
    /* Scan through the iovec's to check lengths and figure out */
    /* maximum buffer size */
    for (acc = i = 0; i < msg->msg_iovlen; i++) {
	register int len = msg->msg_iov[i].iov_len;
	register char *base = msg->msg_iov[i].iov_base;

	if ((len < 0) || (base < &etext) || ((base + len) > sbrk(0))) {
	    errno = EINVAL;
	    return (-1);
	}
	acc += msg->msg_iov[i].iov_len;
    }

    /* Allocate a receive buffer */
    if (acc > buf_size) {
	if (buf_base != NULL) {
	    (void) free(buf_base);
	}
	buf_base = (caddr_t) malloc(acc);
	buf_size = acc;
    }
    if (!buf_base) {
	buf_size = 0;
	errno = ENOMEM;
	return (-1);
    }
    acc = cc = recvfrom(s,
			buf_base,
			acc,
			flags,
			msg->msg_name,
			&msg->msg_namelen);

    /* Return if error */
    if (cc < 0) {
	return (cc);
    }
    /* Distribute the data as specified in the iovec */
    for (i = 0, buf_ptr = buf_base; acc && (i < msg->msg_iovlen); i++) {
	register int len = msg->msg_iov[i].iov_len;

	(void) memcpy(msg->msg_iov[i].iov_base, buf_ptr, len);
	buf_ptr += len;
	acc -= len;
    }

    /* Return the length read */
    return (cc);
}

#endif	/* NEED_RECVMSG */
