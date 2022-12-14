/* HPUX_ID: @(#) $Revision: 66.1 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */

/*
 *   NAM_HASH (NAME)
 *
 *        char *NAME;
 *
 *	Return a hash value of the string given by name.
 *      Trial and error has shown this hash function to perform well
 *
 */

#include	"sh_config.h"

int nam_hash(name)
register const char *name;
{
	register int h = *name;
	register int c;
	while(c= *++name)
	{
		if((h = (h>>2) ^ (h<<3) ^ c) < 0)
			h = ~h;
	}
	return (h);
}

