#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	
#include <nl_types.h>
#endif NLS
/*	@(#) $Revision: 51.4 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	/sccs/src/cmd/uucp/s.gnamef.c
	gnamef.c	1.1	7/29/85 16:33:04
*/
#include "uucp.h"


/*
 * get next file name from directory
 *	p	 -> file description of directory file to read
 *	filename -> address of buffer to return filename in
 *		    must be of size DIRSIZ+1
 * returns:
 *	FALSE	-> end of directory read
 *	TRUE	-> returned name
 */
gnamef(p, filename)
register char *filename;
DIR *p;
{
	struct direct dentry;
	register struct direct *dp = &dentry;

	while (1) {
/* Do this because we have readdir too */
/*#ifdef	BSD4_2 */
		if ((dp = readdir(p)) == NULL)
/*#else	!BSD4_2*/
		/*if (fread((char *)dp,  sizeof(dentry), 1, p) != 1) */
/*#endif	BSD4_2*/
			return(FALSE);
		if (dp->d_ino != 0 && dp->d_name[0] != '.')
			break;
	}

	(void) strncpy(filename, dp->d_name, MAXBASENAME);
	filename[MAXBASENAME] = '\0';
	return(TRUE);
}

/*
 * get next directory name from directory
 *	p	 -> file description of directory file to read
 *	filename -> address of buffer to return filename in
 *		    must be of size DIRSIZ+1
 * returns:
 *	FALSE	-> end of directory read
 *	TRUE	-> returned dir
 */
gdirf(p, filename, dir)
register char *filename;
DIR *p;
char *dir;
{
	char statname[MAXNAMESIZE];

	while (1) {
		if(gnamef(p, filename) == FALSE)
			return(FALSE);
		(void) sprintf(statname, "%s/%s", dir, filename);
		DEBUG(4, (catgets(nlmsg_fd,NL_SETN,517, "stat %s\n")), statname);
		if (DIRECTORY(statname))
		    break;
	}

	return(TRUE);
}
