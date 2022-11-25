/* $Header: warn.c,v 1.2 85/03/25 12:36:56 nicklin HP_UXRel1 $ */

/*
 * Author: Peter J. Nicklin
 */

/*
 * warn() places an error message on the standard error output stream
 * stderr.
 */
#include <stdio.h>
#include "null.h"

extern char *PGN;			/* program name */

warn(m)
	char *m;			/* warning message */
{
	if (PGN != NULL && *PGN != '\0')
		fprintf(stderr, "%s: ", PGN);
	fprintf(stderr, "%s\n", m);
}
