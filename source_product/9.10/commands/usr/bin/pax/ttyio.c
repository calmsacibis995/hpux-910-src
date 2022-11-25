/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/ttyio.c,v $
 *
 * $Revision: 66.1 $
 *
 * ttyio.c - Terminal/Console I/O functions for all archive interfaces
 *
 * DESCRIPTION
 *
 *	These routines provide a consistent, general purpose interface to
 *	the user via the users terminal, if it is available to the
 *	process.
 *
 * AUTHOR
 *
 *     Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
 *
 * COPYRIGHT
 *
 *	Copyright (c) 1989 Mark H. Colburn.  All rights reserved.
 *
 *	Redistribution and use in source and binary forms are permitted
 *	provided that the above copyright notice and this paragraph are
 *	duplicated in all such forms and that any documentation,
 *	advertising materials, and other materials related to such
 *	distribution and use acknowledge that the software was developed
 *	by Mark H. Colburn.
 *
 *	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Log:	ttyio.c,v $
 * Revision 66.1  90/05/11  10:45:43  10:45:43  michas
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:36:13  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:36:00  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: ttyio.c,v 2.0.0.4 89/12/16 10:36:13 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* open_tty - open the terminal for interactive queries
 *
 * DESCRIPTION
 *
 * 	Assumes that background processes ignore interrupts and that the
 *	open() or the isatty() will fail for processes which are not
 *	attached to terminals. Returns a file descriptor or -1 if
 *	unsuccessful. 
 *
 * RETURNS
 *
 *	Returns a file descriptor which can be used to read and write
 *	directly to the user's terminal, or -1 on failure.  
 *
 * ERRORS
 *
 *	If SIGINT cannot be ignored, or the open fails, or the newly opened
 *	terminal device is not a tty, then open_tty will return a -1 to the
 *	caller.
 */

int
open_tty()
{
    int                 fd;	/* file descriptor for terminal */

#ifdef VOIDSIG
    void                (*intr) ();	/* used to restore interupts */
#else
    int                 (*intr) ();
#endif

    DBUG_ENTER("open_tty");
    if ((intr = signal(SIGINT, SIG_IGN)) == SIG_IGN) {
	DBUG_RETURN(-1);
    }
    signal(SIGINT, intr);
    if ((fd = open(TTY, O_RDWR)) < 0) {
	DBUG_RETURN(-1);
    }
    
#ifdef MSDOS
    setmode(fd, O_TEXT);
#endif /* MSDOS */
    
    if (isatty(fd)) {
	DBUG_RETURN(fd);
    }
    /* FIXME: do error checking here */
    close(fd);
    DBUG_RETURN(-1);
}


/* nextask - ask a question and get a response
 *
 * DESCRIPTION
 *
 *	Give the user a prompt and wait for their response.  The prompt,
 *	located in "msg" is printed, then the user is allowed to type
 *	a response to the message.  The first "limit" characters of the
 *	user response is stored in "answer".
 *
 *	Nextask ignores spaces and tabs. 
 *
 * PARAMETERS
 *
 *	char *msg	- Message to display for user 
 *	char *answer	- Pointer to user's response to question 
 *	int limit	- Limit of length for user's response
 *
 * RETURNS
 *
 *	Returns the number of characters in the user response to the 
 *	calling function.  If an EOF was encountered, a -1 is returned to
 *	the calling function.  If an error occured which causes the read
 *	to return with a value of -1, then the function will return a
 *	non-zero return status to the calling process, and abort
 *	execution.
 */

int
nextask(msg, answer, limit)
    char               *msg;	/* message to display for user */
    char               *answer;	/* pointer to user's response to question */
    int                 limit;	/* limit of length for user's response */
{
    int                 idx;	/* index into answer for character input */
    int                 got;	/* number of characters read */
    char                c;	/* character read */

    DBUG_ENTER("nextask");
    if (ttyf < 0) {
	fatal("/dev/tty Unavailable");
    }
    write(ttyf, msg, (uint) strlen(msg));
    idx = 0;
    while ((got = read(ttyf, &c, 1)) == 1) {
	if (c == '\n') {
	    break;
	} else if (c == ' ' || c == '\t') {
	    continue;
	} else if (idx < limit - 1) {
	    answer[idx++] = c;
	}
    }
    if (got == 0) {		/* got an EOF */
	DBUG_RETURN(-1);
    }
    if (got < 0) {
	fatal(strerror(errno));
    }
    answer[idx] = '\0';
    DBUG_RETURN(0);
}


/* lineget - get a line from a given stream
 *
 * DESCRIPTION
 * 
 *	Get a line of input for the stream named by "stream".  The data on
 *	the stream is put into the buffer "buf".
 *
 * PARAMETERS
 *
 *	FILE *stream		- Stream to get input from 
 *	char *buf		- Buffer to put input into
 *
 * RETURNS
 *
 * 	Returns 0 if successful, -1 at EOF. 
 */

int
lineget(stream, buf)
    FILE               *stream;	/* stream to get input from */
    char               *buf;	/* buffer to put input into */
{
    int                 c;

    DBUG_ENTER("lineget");
    for (;;) {
	if ((c = getc(stream)) == EOF) {
	    DBUG_RETURN(-1);
	}
	if (c == '\n') {
	    break;
	}
	*buf++ = c;
    }
    *buf = '\0';
    DBUG_RETURN(0);
}


/* next - Advance to the next archive volume. 
 *
 * DESCRIPTION
 *
 *	Prompts the user to replace the backup medium with a new volume
 *	when the old one is full.  There are some cases, such as when
 *	archiving to a file on a hard disk, that the message can be a
 *	little surprising.  Assumes that background processes ignore
 *	interrupts and that the open() or the isatty() will fail for
 *	processes which are not attached to terminals. Returns a file
 *	descriptor or -1 if unsuccessful. 
 *
 * PARAMETERS
 *
 *	int mode	- mode of archive (READ, WRITE, PASS) 
 */

void
next(mode)
    int                 mode;	/* mode of archive (READ, WRITE, PASS) */
{
    char                msg[200];	/* buffer for message display */
    char                answer[20];	/* buffer for user's answer */
    int                 ret;

    DBUG_ENTER("next");
    close_archive();

    sprintf(msg, "%s: Ready for volume %u\n%s: Type \"go\" when ready to proceed (or \"quit\" to abort): \007",
	    myname, arvolume + 1, myname);
    for (;;) {
	ret = nextask(msg, answer, sizeof(answer));
	if (ret == -1 || strcmp(answer, "quit") == 0) {
	    fatal("Aborted");
	}
	if (strcmp(answer, "go") == 0 && open_archive(mode) == 0) {
	    break;
	}
    }
    warnarch("Continuing", (OFFSET) 0);
    DBUG_VOID_RETURN;
}
