/* $Source: /misc/source_product/9.10/commands.rcs/usr/bin/pax/names.c,v $
 *
 * $Revision: 66.1 $
 *
 * names.c - Look up user and/or group names. 
 *
 * DESCRIPTION
 *
 *	These functions support UID and GID name lookup.  The results are
 *	cached to improve performance.
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
 * $Log:	names.c,v $
 * Revision 66.1  90/05/11  09:03:16  09:03:16  michas
 * inital checkin
 * 
 * Revision 2.0.0.4  89/12/16  10:35:41  mark
 * Changed the handling of function prototypes and declarations so that they
 * are easier to maintain.  The old ones were a royal pain.
 * 
 * Revision 2.0.0.3  89/10/13  02:35:24  mark
 * Beta Test Freeze
 * 
 */

#ifndef lint
static char        *ident = "$Id: names.c,v 2.0.0.4 89/12/16 10:35:41 mark Exp Locker: mark $";
static char        *copyright = "Copyright (c) 1989 Mark H. Colburn.\nAll rights reserved.\n";
#endif /* ! lint */


/* Headers */

#include "pax.h"


/* Defines */

#define myuid	( my_uid < 0? (my_uid = getuid()): my_uid )
#define	mygid	( my_gid < 0? (my_gid = getgid()): my_gid )


/* Internal Identifiers */

#ifndef MSDOS
static UIDTYPE      saveuid = -993;
static char         saveuname[TUNMLEN];
static UIDTYPE      my_uid = -993;

static GIDTYPE      savegid = -993;
static char         savegname[TGNMLEN];
static GIDTYPE      my_gid = -993;
#else /* MSDOS */
static int      saveuid = 0;
static char     saveuname[TUNMLEN] = "";
static int      my_uid = 0;
  
static int      savegid = 0;
static char     savegname[TGNMLEN] = "";
static int      my_gid = 0;
#endif /* MSDOS */

/* finduname - find a user or group name from a uid or gid
 *
 * DESCRIPTION
 *
 * 	Look up a user name from a uid/gid, maintaining a cache. 
 *
 * PARAMETERS
 *
 *	char	uname[]		- name (to be returned to user)
 *	int	uuid		- id of name to find
 *
 *
 * RETURNS
 *
 *	Returns a name which is associated with the user id given.  If there
 *	is not name which corresponds to the user-id given, then a pointer
 *	to a string of zero length is returned.
 *	
 * FIXME
 *
 * 	1. for now it's a one-entry cache. 
 *	2. The "-993" is to reduce the chance of a hit on the first lookup. 
 */

char *
finduname(uuid)
    UIDTYPE             uuid;
{
    struct passwd      *pw;

    DBUG_ENTER("finduname");
    if (uuid != saveuid) {
	saveuid = uuid;
	saveuname[0] = '\0';
	pw = getpwuid(uuid);
	if (pw) {
	    strncpy(saveuname, pw->pw_name, TUNMLEN);
	}
    }
    DBUG_RETURN(saveuname);
}


/* finduid - get the uid for a given user name
 *
 * DESCRIPTION
 *
 *	This does just the opposit of finduname.  Given a user name it
 *	finds the corresponding UID for that name.
 *
 * PARAMETERS
 *
 *	char	uname[]		- username to find a UID for
 *
 * RETURNS
 *
 *	The UID which corresponds to the uname given, if any.  If no UID
 *	could be found, then the UID which corrsponds the user running the
 *	program is returned.
 *
 */

UIDTYPE
finduid(uname)
    char               *uname;
{
    struct passwd      *pw;
    extern struct passwd *getpwnam();

    DBUG_ENTER("finduid");
    if (uname[0] != saveuname[0]/* Quick test w/o proc call */
	||0 != strncmp(uname, saveuname, TUNMLEN)) {
	strncpy(saveuname, uname, TUNMLEN);
	pw = getpwnam(uname);
	if (pw) {
	    saveuid = pw->pw_uid;
	} else {
	    saveuid = myuid;
	}
    }
    DBUG_RETURN(saveuid);
}


/* findgname - look up a group name from a gid
 *
 * DESCRIPTION
 *
 * 	Look up a group name from a gid, maintaining a cache.
 *
 * PARAMETERS
 *
 *	int	ggid		- goupid of group to find
 *
 * RETURNS
 *
 *	A string which is associated with the group ID given.  If no name
 *	can be found, a string of zero length is returned.
 */

char *
findgname(ggid)
    GIDTYPE             ggid;
{
    struct group       *gr;

    DBUG_ENTER("findgname");
    if (ggid != savegid) {
	savegid = ggid;
	savegname[0] = '\0';
	setgrent();
	gr = getgrgid(ggid);
	if (gr) {
	    strncpy(savegname, gr->gr_name, TGNMLEN);
	}
    }
    DBUG_RETURN(savegname);
}



/* findgid - get the gid for a given group name
 *
 * DESCRIPTION
 *
 *	This does just the opposit of finduname.  Given a group name it
 *	finds the corresponding GID for that name.
 *
 * PARAMETERS
 *
 *	char	uname[]		- groupname to find a GID for
 *
 * RETURNS
 *
 *	The GID which corresponds to the uname given, if any.  If no GID
 *	could be found, then the GID which corrsponds the group running the
 *	program is returned.
 *
 */

GIDTYPE
findgid(gname)
    char               *gname;
{
    struct group       *gr;

    DBUG_ENTER("findgid");
    /* Quick test w/o proc call */
    if (gname[0] != savegname[0] || strncmp(gname, savegname, TUNMLEN) != 0) {
	strncpy(savegname, gname, TUNMLEN);
	gr = getgrnam(gname);
	if (gr) {
	    savegid = gr->gr_gid;
	} else {
	    savegid = mygid;
	}
    }
    DBUG_RETURN(savegid);
}
