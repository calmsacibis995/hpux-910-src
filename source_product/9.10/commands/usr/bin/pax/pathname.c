/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/pathname.c,v $
 *
 * $Revision: 66.2 $
 *
 * pathname.c - directory/pathname support functions 
 *
 * DESCRIPTION
 *
 *	These functions provide directory/pathname support for PAX
 *
 * AUTHOR
 *
 *	Mark H. Colburn, Open Systems Architects, Inc. (mark@osa.com)
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
 * $Log:	pathname.c,v $
 * Revision 66.2  90/07/10  08:34:46  08:34:46  kawo
 * Changes for CDF-file handling in dirneed() and dirmake().
 * 
 * Revision 66.1  90/05/11  09:10:19  09:10:19  michas (#Michael Sieber)
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:35:44  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:35:31  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: pathname.c,v 2.0.0.4 89/12/16 10:35:44 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* dirneed  - checks for the existance of directories and possibly create
 *
 * DESCRIPTION
 *
 *	Dirneed checks to see if a directory of the name pointed to by name
 *	exists.  If the directory does exist, then dirneed returns 0.  If
 *	the directory does not exist and the f_dir_create flag is set,
 *	then dirneed will create the needed directory, recursively creating
 *	any needed intermediate directory.
 *
 *	If f_dir_create is not set, then no directories will be created
 *	and a value of -1 will be returned if the directory does not
 *	exist.
 *
 * PARAMETERS
 *
 *	name		- name of the directory to create
 *
 * RETURNS
 *
 *	Returns a 0 if the creation of the directory succeeded or if the
 *	directory already existed.  If the f_dir_create flag was not set
 *	and the named directory does not exist, or the directory creation 
 *	failed, a -1 will be returned to the calling routine.
 */

int
dirneed(name)
    char               *name;
{
    char               *cp;
    char               *last;
    int			cdfflag;
    int                 ok;
    static Stat         sb;

    cdfflag = 0;

    DBUG_ENTER("dirneed");
    last = (char *) NULL;
    for (cp = name; *cp;) {
	if (*cp++ == '/') {
	    last = cp;
	}
    }
    if (last == (char *) NULL) {
	DBUG_RETURN(STAT(".", &sb));
    }
    /* check if cdf, they are marked with name+// in the path */
    if ( ( (*(last-2) == '+') && (*last == '/') ) ||
         ( (*(last-2) == '+') && (*last == '\0') )    ) {
	cdfflag++;
    }
    *--last = '\0';
    ok = STAT(*name ? name : ".", &sb) == 0
	? ((sb.sb_mode & S_IFMT) == S_IFDIR)
	: (f_dir_create && dirneed(name) == 0 &&
	   dirmake(name, &sb, cdfflag) == 0);
    *last = '/';
    DBUG_RETURN(ok ? 0 : -1);
}


/* nameopt - optimize a pathname
 *
 * DESCRIPTION
 *
 * 	Confused by "<symlink>/.." twistiness. Returns the number of final 
 * 	pathname elements (zero for "/" or ".") or -1 if unsuccessful. 
 *
 * PARAMETERS
 *
 *	char	*begin	- name of the path to optimize
 *
 * RETURNS
 *
 *	Returns 0 if successful, non-zero otherwise.
 *
 */

int
nameopt(begin)
    char               *begin;
{
    char               *name;
    char               *item;
    int                 idx;
    int                 namecnt;
    int                 absolute;
    char               *element[PATHELEM];

    DBUG_ENTER("nameopt");
    absolute = (*(name = begin) == '/');
    idx = 0;
    for (;;) {
	if (idx == PATHELEM) {
	    warn(begin, "Too many elements");
	    DBUG_RETURN(-1);
	}
	while (*name == '/') {
	    ++name;
	}
	if (*name == '\0') {
	    break;
	}
	element[idx] = item = name;
	namecnt = 0;
	while (*name && *name != '/') {
	    if (++namecnt > NAME_MAX) {
		warn(begin, "intermediate file name too long");
		DBUG_RETURN(-1);
	    }
	    ++name;
	}
	if (*name) {
	    *name++ = '\0';
	}
	if (strcmp(item, "..") == 0) {
	    if (idx == 0) {
		if (!absolute) {
		    ++idx;
		}
	    } else if (strcmp(element[idx - 1], "..") == 0) {
		++idx;
	    } else {
		--idx;
	    }
	} else if (strcmp(item, ".") != 0) {
	    ++idx;
	}
    }
    if (idx == 0) {
	element[idx++] = absolute ? "" : ".";
    }
    element[idx] = (char *) NULL;
    name = begin;
    if (absolute) {
	*name++ = '/';
    }
    for (idx = 0; item = element[idx]; ++idx, *name++ = '/') {
	while (*item) {
	    *name++ = *item++;
	}
    }
    *--name = '\0';
    DBUG_RETURN(idx);
}


/* dirmake - make a directory  
 *
 * DESCRIPTION
 *
 *	Dirmake makes a directory with the appropritate permissions.
 *
 * PARAMETERS
 *
 *	char 	*name	- Name of directory make
 *	Stat	*asb	- Stat structure of directory to make
 *	int	cdfflag - tells to make a cdf    
 *
 * RETURNS
 *
 * 	Returns zero if successful, -1 otherwise. 
 *
 */

int
dirmake(name, asb, cdfflag)
    char               *name;
    Stat               *asb;
    int			cdfflag; /* set if name+// was found in dirneed */
{
    char		*to_plus;

    DBUG_ENTER("dirmake");
    if ( cdfflag ) {
	asb->sb_mode |= S_CDF;
    }
    /* delete plus from end of name otherwise directory name will be wrong */
    if ( S_ISCDF(asb->sb_mode) ) {
	to_plus = name + strlen(name) - 1;
	for (; *to_plus != '+'; to_plus--)
	    ;
	*to_plus = '\0';
    }
    if (mkdir(name, (int) (asb->sb_mode & S_IPOPN)) < 0) {
	DBUG_RETURN(-1);
    }
    if (asb->sb_mode & S_IPEXE) {
	chmod(name, (int) (asb->sb_mode & S_IPERM));
    }
    if (f_owner) {
	chown(name, (int) asb->sb_uid, (GIDTYPE) asb->sb_gid);
    }
    /* change name as it was before */
    if ( S_ISCDF(asb->sb_mode) ) {
	*to_plus = '+';
    }
    DBUG_RETURN(0);
}
