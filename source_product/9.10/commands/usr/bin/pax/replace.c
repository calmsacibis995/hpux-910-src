/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/replace.c,v $
 *
 * $Revision: 66.1 $
 *
 * replace.c - regular expression pattern replacement functions
 *
 * DESCRIPTION
 *
 *	These routines provide for regular expression file name replacement
 *	as required by pax.
 *
 * AUTHORS
 *
 *	Mark H. Colburn, Open Systems Architects, Inc.  (mark@osa.com)
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
 * $Log:	replace.c,v $
 * Revision 66.1  90/05/11  10:36:38  10:36:38  michas
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:36:07  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:35:54  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: replace.c,v 2.0.0.4 89/12/16 10:36:07 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif				/* not lint */

/* Headers */

#include "pax.h"


/* add_replstr - add a replacement string to the replacement string list
 *
 * DESCRIPTION
 *
 *	Add_replstr adds a replacement string to the replacement string
 *	list which is applied each time a file is about to be processed.
 *
 * PARAMETERS
 *
 *	char	*pattern	- A regular expression which is to be parsed
 */

void
add_replstr(pattern)
    char               *pattern;
{
    char               *p;
    char                sep;
    Replstr            *rptr;
    int                 len;

    DBUG_ENTER("add_replstr");
    if ((len = strlen(pattern)) < 4) {
	warn("Replacement string not added",
	     "Malformed substitution syntax");
	DBUG_VOID_RETURN;
    }
    if ((rptr = (Replstr *) malloc(sizeof(Replstr))) == (Replstr *) NULL) {
	warn("Replacement string not added", "No space");
	DBUG_VOID_RETURN;
    }
    /* First character is the delimeter... */
    sep = *pattern;

    /* Get trailing g and/or p */
    p = pattern + len - 1;
    while (*p != sep) {
	if (*p == 'g') {
	    rptr->global = 1;
	} else if (*p == 'p') {
	    rptr->print = 1;
	} else {
	    warn(p, "Invalid RE modifier");
	}
	p--;
    }

    if (*p != sep) {
	warn("Replacement string not added", "Bad delimeters");
	free(rptr);
	DBUG_VOID_RETURN;
    }
    /* strip off leading and trailing delimeter */
    *p = '\0';
    pattern++;

    /* find the separating '/' in the pattern */
    p = pattern;
    while (*p) {
	if (*p == sep) {
	    break;
	}
	if (*p == '\\' && *(p + 1) != '\0') {
	    p++;
	}
	p++;
    }
    if (*p != sep) {
	warn("Replacement string not added", "Bad delimeters");
	free(rptr);
	DBUG_VOID_RETURN;
    }
    *p++ = '\0';

    /*
     * Now pattern points to 'old' and p points to 'new' and both are '\0'
     * terminated 
     */
    if ((rptr->comp = regcomp(pattern)) == (regexp *) NULL) {
	warn("Replacement string not added", "Invalid RE");
	free(rptr);
	DBUG_VOID_RETURN;
    }
    rptr->replace = p;
    rptr->next = (Replstr *) NULL;
    if (rplhead == (Replstr *) NULL) {
	rplhead = rptr;
	rpltail = rptr;
    } else {
	rpltail->next = rptr;
	rpltail = rptr;
    }
    DBUG_VOID_RETURN;
}



/* rpl_name - possibly replace a name with a regular expression
 *
 * DESCRIPTION
 *
 *	The string name is searched for in the list of regular expression
 *	substituions.  If the string matches any of the regular expressions
 *	then the string is modified as specified by the user.
 *
 * PARAMETERS
 *
 *	char	*name	- name to search for and possibly modify
 */

void
rpl_name(name)
    char               *name;
{
    int                 found = 0;
    int                 ret;
    Replstr            *rptr;
    char                buff[PATH_MAX + 1];
    char                buff1[PATH_MAX + 1];
    char                buff2[PATH_MAX + 1];
    char               *p;
    char               *b;

    DBUG_ENTER("rpl_name");
    strcpy(buff, name);
    for (rptr = rplhead; !found && rptr != (Replstr *) NULL; rptr = rptr->next) {
	do {
	    if ((ret = regexec(rptr->comp, buff)) != 0) {
		p = buff;
		b = buff1;
		while (p < rptr->comp->startp[0]) {
		    *b++ = *p++;
		}
		p = rptr->replace;
		while (*p) {
		    *b++ = *p++;
		}
		strcpy(b, rptr->comp->endp[0]);
		found = 1;
		regsub(rptr->comp, buff1, buff2);
		strcpy(buff, buff2);
	    }
	} while (ret && rptr->global);
	if (found) {
	    if (rptr->print) {
		fprintf(stderr, "%s >> %s\n", name, buff);
	    }
	    strcpy(name, buff);
	}
    }
    DBUG_VOID_RETURN;
}


/* get_disposition - get a file disposition
 *
 * DESCRIPTION
 *
 *	Get a file disposition from the user.  If the user enters 'y'
 *	the the file is processed, anything else and the file is ignored.
 *	If the user enters EOF, then the PAX exits with a non-zero return
 *	status.
 *
 * PARAMETERS
 *
 *	char	*mode	- string signifying the action to be taken on file
 *	char	*name	- the name of the file
 *
 * RETURNS
 *
 *	Returns 1 if the file should be processed, 0 if it should not.
 */

int
get_disposition(mode, name)
    char               *mode;
    char               *name;
{
    char                ans[2];
    char                buf[PATH_MAX + 10];

    DBUG_ENTER("get_disposition");
    if (f_disposition) {
	sprintf(buf, "%s %s? ", mode, name);
	if (nextask(buf, ans, sizeof(ans)) == -1 || ans[0] == 'q') {
	    exit(0);
	}
	if (strlen(ans) == 0 || ans[0] != 'y') {
	    DBUG_RETURN(1);
	}
    }
    DBUG_RETURN(0);
}


/* get_newname - prompt the user for a new filename
 *
 * DESCRIPTION
 *
 *	The user is prompted with the name of the file which is currently
 *	being processed.  The user may choose to rename the file by
 *	entering the new file name after the prompt; the user may press
 *	carriage-return/newline, which will skip the file or the user may
 *	type an 'EOF' character, which will cause the program to stop.
 *
 * PARAMETERS
 *
 *	char	*name		- filename, possibly modified by user
 *	int	size		- size of allowable new filename
 *
 * RETURNS
 *
 *	Returns 0 if successfull, or -1 if an error occurred.
 *
 */

int
get_newname(name, size)
    char               *name;
    int                 size;
{
    char                buf[PATH_MAX + 10];

    DBUG_ENTER("get_newname");
    if (f_interactive) {
	sprintf(buf, "rename %s? ", name);
	if (nextask(buf, name, size) == -1) {
	    exit(0);
	}
	if (strlen(name) == 0) {
	    DBUG_RETURN(1);
	}
    }
    DBUG_RETURN(0);
}
