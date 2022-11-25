/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char    copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif				/* not lint */

#ifndef lint
static char     sccsid[] = "@(#)praliases.c	5.5 (Berkeley) 6/1/90";
static char     rcsid[] = "@(#)$Header: praliases.c,v 1.11.109.2 94/08/30 14:17:09 mike Exp $";
#endif				/* not lint */

#include <sendmail.h>
#include "pathnames.h"
#include <sys/file.h>

main (argc, argv)
char  **argv;
{
    extern char    *optarg;
    extern int  optind;
    datum   content,
            key;
    static char    *filename = _PATH_MAILALIASES;
    int     ch;
    DBM    *alias_db;

    while ((ch = getopt (argc, argv, "f:r")) != EOF)
	switch ((char)ch)
	    {
	    case 'f':
		filename = optarg;
		break;
	    case 'r':
		filename = _PATH_MAILREVALIASES;
		break;
	    case '?':
	    default:
		fputs ("usage: praliases [-f file][-r] [key]\n", stderr);
		exit (EX_USAGE);
	    }
    argc -= optind;
    argv += optind;

    if ((alias_db = dbm_open (filename, O_RDONLY, 0)) == NULL)
        {
	perror (filename);
	exit (EX_OSFILE);
        }
    if (!argc)
	for (key = dbm_firstkey (alias_db); key.dptr != NULL; key = dbm_nextkey (alias_db))
	    {
	    content = dbm_fetch (alias_db, key);
	    printf ("%s:%s\n", key.dptr, content.dptr);
	    }
    else
	for (; *argv; ++argv)
	    {
	    key.dptr = *argv;
	    key.dsize = strlen (*argv) + 1;
	    content = dbm_fetch (alias_db, key);
	    if (!content.dptr)
		printf ("%s: No such key\n", key.dptr);
	    else
		printf ("%s:%s\n", key.dptr, content.dptr);
	    }
    exit (EX_OK);
}
