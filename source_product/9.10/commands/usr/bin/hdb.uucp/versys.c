
/*	@(#) $Revision: 51.3 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	/sccs/src/cmd/uucp/s.versys.c
	versys.c	1.1	7/29/85 16:34:26
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#include "uucp.h"

/*
 * verify system name
 * input:
 *	name	-> system name
 * returns:  
 *	0	-> success
 *	FAIL	-> failure
 */
versys(name)
char *name;
{
	register FILE *fp;
	register char *iptr;
	char line[300];

	if (EQUALS(name, Myname))
		return(0);

	fp = fopen(SYSFILE, "r");
	if (fp == NULL)
		return(FAIL);
	
	while (fgets(line, sizeof(line) ,fp) != NULL) {
		if((line[0] == '#') || (line[0] == ' ') || (line[0] == '\t') || 
			(line[0] == '\n'))
			continue;

		if ((iptr=strpbrk(line, " \t")) == NULL)
		    continue;	/* why? */
		*iptr = '\0';
		if (EQUALS(name, line)) {
			(void) fclose(fp);
			return(0);
		}
	}
	fclose(fp);
	return(FAIL);
}
