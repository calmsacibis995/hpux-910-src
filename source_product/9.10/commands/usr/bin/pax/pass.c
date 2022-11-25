/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/pass.c,v $
 *
 * $Revision: 66.1 $
 *
 * pass.c - handle the pass option of cpio
 *
 * DESCRIPTION
 *
 *	These functions implement the pass options in PAX.  The pass option
 *	copies files from one directory hierarchy to another.
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
 * $Log:	pass.c,v $
 * Revision 66.1  90/05/11  09:06:02  09:06:02  michas
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:35:43  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:35:27  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: pass.c,v 2.0.0.4 89/12/16 10:35:43 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* pass - copy within the filesystem
 *
 * DESCRIPTION
 *
 *	Pass copies the named files from the current directory hierarchy to
 *	the directory pointed to by dirname.
 *
 * PARAMETERS
 *
 *	char	*dirname	- name of directory to copy named files to.
 *
 */

int
pass(dirname)
    char               *dirname;
{
    char                name[PATH_MAX + 1];
    int                 fd;
    Stat                sb;

    DBUG_ENTER("pass");
    while (name_next(name, &sb) >= 0 && (fd = openin(name, &sb)) >= 0) {

	if (rplhead != (Replstr *) NULL) {
	    rpl_name(name);
	}
	if (get_disposition("pass", name) || get_newname(name, sizeof(name))) {
	    /* skip file... */
	    if (fd) {
		/* FIXME: do error checking here */
		close(fd);
	    }
	    continue;
	}
	if (passitem(name, &sb, fd, dirname)) {
	    /* FIXME: do error checking here */
	    close(fd);
	}
	if (f_verbose) {
	    fprintf(stderr, "%s/%s\n", dirname, name);
	}
    }
}


/* passitem - copy one file
 *
 * DESCRIPTION
 *
 *	Passitem copies a specific file to the named directory
 *
 * PARAMETERS
 *
 *	char   *from	- the name of the file to open
 *	Stat   *asb	- the stat block associated with the file to copy
 *	int	ifd	- the input file descriptor for the file to copy
 *	char   *dir	- the directory to copy it to
 *
 * RETURNS
 *
 * 	Returns given input file descriptor or -1 if an error occurs.
 *
 * ERRORS
 */

int
passitem(from, asb, ifd, dir)
    char               *from;
    Stat               *asb;
    int                 ifd;
    char               *dir;
{
    int                 ofd;
    time_t              tstamp[2];
    char                to[PATH_MAX + 1];

    DBUG_ENTER("passitem");
    if (nameopt(strcat(strcat(strcpy(to, dir), "/"), from)) < 0) {
	DBUG_RETURN(-1);
    }
    if (asb->sb_nlink > 1) {
	linkto(to, asb);
    }
    if (f_link && islink(from, asb) == (Link *) NULL) {
	linkto(from, asb);
    }
    if ((ofd = openout(to, asb, islink(to, asb), 1)) < 0) {
	DBUG_RETURN(-1);
    }
    if (ofd > 0) {
	passdata(from, ifd, to, ofd);
    }
    tstamp[0] = asb->sb_atime;
    tstamp[1] = f_mtime ? asb->sb_mtime : time((time_t *) 0);
    utime(to, tstamp);
    DBUG_RETURN(ifd);
}
