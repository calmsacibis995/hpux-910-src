/* @(#)automount:	$Revision: 1.4.109.1 $	$Date: 91/11/19 14:14:51 $
*/

#ifndef lint
static char sccsid[] = 	"(#)strdup.c	1.2 90/07/24 4.1NFSSRC Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#define NULL 0

char *strdup(s1)
char *s1;
{
    char *s2;
    extern char *malloc(), *strcpy();

    s2 = malloc(strlen(s1)+1);
    if (s2 != NULL)
        s2 = strcpy(s2, s1);
    return(s2);
}
