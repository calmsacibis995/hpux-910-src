/* $Header: pperror.c,v 1.1 84/09/14 17:00:12 bob HP_UXRel1 $ */

/*
 * Author: Peter J. Nicklin
 */

/*
 * pperror() writes a system error message to standard error output,
 * preceded by the name of the program and message.
 */
#include <stdio.h>

pperror(message)
	char *message;			/* error message */
{
	extern char *PGN;		/* program name */

	fprintf(stderr, "%s: ", PGN);
	perror(message);
}
