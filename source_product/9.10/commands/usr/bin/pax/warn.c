/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/warn.c,v $
 *
 * $Revision: 66.1 $
 *
 * warn.c - miscellaneous user warning routines 
 *
 * DESCRIPTION
 *
 *	These routines provide the user with various forms of warning
 *	and informational messages.
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
 * $Log:	warn.c,v $
 * Revision 66.1  90/05/11  10:46:27  10:46:27  michas
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:36:15  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:36:02  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: warn.c,v 2.0.0.4 89/12/16 10:36:15 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

#ifdef __STDC__
#define P(x)	x
#else
#define P(x)	()
#endif

static void	    prsize P((FILE *, OFFSET));

#undef P


/* warnarch - print an archive-related warning message and offset
 *
 * DESCRIPTION
 *
 *	Present the user with an error message and an archive offset at
 *	which the error occured.   This can be useful for diagnosing or
 *	fixing damaged archives.
 *
 * PARAMETERS
 *
 *	char 	*msg	- A message string to be printed for the user.
 *	OFFSET 	adjust	- An adjustment which is added to the current 
 *			  archive position to tell the user exactly where 
 *			  the error occurred.
 */

void
warnarch(msg, adjust)
    char               *msg;
    OFFSET              adjust;
{
    DBUG_ENTER("warnarch");
    fprintf(stderr, "%s: [offset ", myname);
    prsize(stderr, total - adjust);
    fprintf(stderr, "]: %s\n", msg);
    DBUG_VOID_RETURN;
}


/* prsize - print a file offset on a file stream
 *
 * DESCRIPTION
 *
 *	Prints a file offset to a specific file stream.  The file offset is
 *	of the form "%dm+%dk+%d", where the number preceeding the "m" and
 *	the "k" stand for the number of Megabytes and the number of
 *	Kilobytes, respectivley, which have been processed so far.
 *
 * PARAMETERS
 *
 *	FILE  *stream	- Stream which is to be used for output 
 *	OFFSET size	- Current archive position to be printed on the output 
 *			  stream in the form: "%dm+%dk+%d".
 *
 */

static void
prsize(stream, size)
    FILE               *stream;	/* stream which is used for output */
    OFFSET              size;	/* current archive position to be printed */
{
    OFFSET              n;

    DBUG_ENTER("prsize");
    if (n = (size / (1024L * 1024L))) {
	fprintf(stream, "%ldm+", n);
	size -= n * 1024L * 1024L;
    }
    if (n = (size / 1024L)) {
	fprintf(stream, "%ldk+", n);
	size -= n * 1024L;
    }
    fprintf(stream, "%ld", size);
    DBUG_VOID_RETURN;
}


/* fatal - print fatal message and exit
 *
 * DESCRIPTION
 *
 *	Fatal prints the program's name along with an error message, then
 *	exits the program with a non-zero return code.
 *
 * PARAMETERS
 *
 *	char 	*why	- description of reason for termination 
 *		
 * RETURNS
 *
 *	Returns an exit code of 1 to the parent process.
 */

void
fatal(why)
    char               *why;	/* description of reason for termination */
{
    DBUG_ENTER("fatal");
    fprintf(stderr, "%s: %s\n", myname, why);
    exit(1);
}


/* warn - print a warning message
 *
 * DESCRIPTION
 *
 *	Print an error message listing the program name, the actual error
 *	which occurred and an informational message as to why the error
 *	occurred on the standard error device.  The standard error is
 *	flushed after the error is printed to assure that the user gets
 *	the message in a timely fasion.
 *
 * PARAMETERS
 *
 *	char *what	- Pointer to string describing what failed.
 *	char *why	- Pointer to string describing why did it failed.
 */

void
warn(what, why)
    char               *what;	/* message as to what the error was */
    char               *why;	/* explanation why the error occurred */
{
    DBUG_ENTER("warn");
    fprintf(stderr, "%s: %s : %s\n", myname, what, why);
    fflush(stderr);
    DBUG_VOID_RETURN;
}
