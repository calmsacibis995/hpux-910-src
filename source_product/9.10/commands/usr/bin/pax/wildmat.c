/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/wildmat.c,v $
 *
 * $Revision: 66.1 $
 *
 * wildmat.c - simple regular expression pattern matching routines 
 *
 * DESCRIPTION 
 *
 * 	These routines provide simple UNIX style regular expression matching.  
 *	They were originally written by Rich Salz, the comp.sources.unix 
 *	moderator for inclusion in some of his software.  These routines 
 *	were released into the public domain and used by John Gilmore in 
 *	USTAR. 
 *
 * AUTHORS 
 *
 * 	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com) 
 * 	John Gilmore (gnu@hoptoad) 
 * 	Rich Salz (rs@uunet.uu.net) 
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
 * $Log:	wildmat.c,v $
 * Revision 66.1  90/05/11  10:50:53  10:50:53  michas
 * inital checkin
 * 
 * Revision 2.0.0.5  89/12/16  10:36:17  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.4  89/10/13  02:36:04  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: wildmat.c,v 2.0.0.5 89/12/16 10:36:17 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Function Prototypes */

#ifdef __STDC__
#define P(x)    x
#else
#define P(x)	()
#endif

static int	    star P((char *, char *));

#undef P


/*
 * star - handle trailing * in a regular expression 
 *
 * DESCRIPTION
 *
 *	Star is used to match filename expansions containing a trailing
 *	asterisk ('*').  Star calls wildmat() to determine if the substring
 *	passed to it is matches the regular expression.
 *
 * PARAMETERS 
 *
 * 	char *source 	- The source string which is to be compared to the 
 *			  regular expression pattern. 
 * 	char *pattern 	- The regular expression which we are supposed to 
 *			  match to. 
 *
 * RETURNS 
 *
 * 	Returns non-zero if the entire source string is completely matched by 
 *	the regular expression pattern, returns 0 otherwise. This is used to 
 *	see if *'s in a pattern matched the entire source string. 
 *
 */

static int
star(source, pattern)
    char               *source;		/* source operand */
    char               *pattern;	/* regular expression to match */
{
    DBUG_ENTER("star");
    while (!wildmat(pattern, source)) {
	if (*++source == '\0') {
	    DBUG_RETURN(0);
	}
    }
    DBUG_RETURN(1);
}


/*
 * wildmat - match a regular expression 
 *
 * DESCRIPTION
 *
 *	Wildmat attempts to match the string pointed to by source to the 
 *	regular expression pointed to by pattern.  The subset of regular 
 *	expression syntax which is supported is defined by POSIX P1003.2 
 *	FILENAME EXPANSION rules.
 *
 * PARAMETERS 
 *
 * 	char *pattern 	- The regular expression which we are supposed to 
 *			  match to. 
 * 	char *source 	- The source string which is to be compared to the 
 *			  regular expression pattern. 
 *
 * RETURNS 
 *
 * 	Returns non-zero if the source string matches the regular expression 
 *	pattern specified, returns 0 otherwise. 
 *
 */

int
wildmat(pattern, source)
    char               *pattern;	/* regular expression to match */
    char               *source;		/* source operand */
{
    int                 last;		/* last character matched */
    int                 matched;	/* !0 if a match occurred */
    int                 reverse;	/* !0 if sense of match is reversed */

    DBUG_ENTER("wildmat");
    for (; *pattern; source++, pattern++) {
	switch (*pattern) {

	case '\\':
	    /* Literal match with following character */
	    pattern++;
	    /* FALLTHRU */

	default:
	    if (*source != *pattern) {
		DBUG_RETURN(0);
	    }
	    continue;

	case '?':
	    /* Match anything. */
	    if (*source == '\0') {
		DBUG_RETURN(0);
	    }
	    continue;

	case '*':
	    /* Trailing star matches everything. */
	    DBUG_RETURN(*++pattern ? star(source, pattern) : 1);

	case '[':
	    /* [^....] means inverse character class. */
	    if (reverse = pattern[1] == '^') {
		pattern++;
	    }
	    for (last = 0400, matched = 0;
		 *++pattern && *pattern != ']'; last = *pattern) {
		/* This next line requires a good C compiler. */
		if (*pattern == '-'
		    ? *source <= *++pattern && *source >= last
		    : *source == *pattern) {
		    matched = 1;
		}
	    }
	    if (matched == reverse) {
		DBUG_RETURN(0);
	    }
	    continue;
	}
    }

    /*
     * For "tar" use, matches that end at a slash also work. --hoptoad!gnu 
     */
    DBUG_RETURN(*source == '\0' || *source == '/');
}
