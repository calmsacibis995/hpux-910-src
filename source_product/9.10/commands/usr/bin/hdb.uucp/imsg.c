/*	@(#) $Revision: 62.1 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	/sccs/src/cmd/uucp/s.imsg.c
	imsg.c	1.1	7/29/85 16:33:11
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#include "uucp.h"

#define MSYNC	'\020'
/* maximum likely message - make sure you don't get run away input */
#define MAXIMSG	256

/*
 * read message routine used before a
 * protocol is agreed upon.
 *	msg	-> address of input buffer
 *	fn	-> input file descriptor 
 * returns:
 *	EOF	-> no more messages
 *	0	-> message returned
 */
imsg(msg, fn)
register char *msg;
register int fn;
{
	register char c;
	short fndsync;
	char *bmsg;
        struct termio tmp_io;
        int savevmin;

	fndsync = 0;
	bmsg = msg;
	CDEBUG(7, (catgets(nlmsg_fd,NL_SETN,522, "imsg %s>")), "");
        ioctl(fn,TCGETA,&tmp_io);
        savevmin = tmp_io.c_cc[VMIN];
        tmp_io.c_cc[VMIN] = 1;
        ioctl(fn,TCSETAW,&tmp_io);
	while (read(fn, msg, sizeof(char)) == sizeof(char)) {
		*msg &= 0177;
		c = *msg;
		CDEBUG(7, "%s", c < 040 ? "^" : "");
		CDEBUG(7, "%c", c < 040 ? c | 0100 : c);
		if (c == MSYNC) { /* look for sync character */
			msg = bmsg;
			fndsync = 1;
			continue;
		}
		if (!fndsync)
			continue;

		if (c == '\0' || c == '\n') {
			*msg = '\0';
                        ioctl(fn,TCGETA,&tmp_io);
        	        tmp_io.c_cc[VMIN] =  savevmin;
                       	ioctl(fn,TCSETAW,&tmp_io);
			return(0);
		}
		else
			msg++;

		if (msg - bmsg > MAXIMSG)	/* unlikely */
			return(FAIL);
	}
	/* have not found sync or end of message */
	*msg = '\0';
        ioctl(fn,TCGETA,&tmp_io);
        tmp_io.c_cc[VMIN] =  savevmin;
        ioctl(fn,TCSETAW,&tmp_io);
	return(EOF);
}

/*
 * initial write message routine -
 * used before a protocol is agreed upon.
 *	type	-> message type
 *	msg	-> message body address
 *	fn	-> file descriptor
 * return: 
 *	Must always return 0 - wmesg (WMESG) looks for zero
 */
omsg(type, msg, fn)
register char *msg;
register char type;
int fn;
{
	char buf[BUFSIZ];

	(void) sprintf(buf, "%c%c%s", MSYNC, type, msg);
	write(fn, buf, strlen(buf) + 1);
	return(0);
}
