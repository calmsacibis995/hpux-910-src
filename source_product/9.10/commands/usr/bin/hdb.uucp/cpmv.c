#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	
#include <nl_types.h>
#endif NLS
/*	@(#) $Revision: 72.1 $	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	/sccs/src/cmd/uucp/s.cpmv.c
	cpmv.c	1.4	7/29/85 16:32:50
*/
#include "uucp.h"

/*
 * copy f1 to f2 locally
 *	f1	-> source file name
 *	f2	-> destination file name
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 */

xcp(f1, f2)
char *f1, *f2;
{
	register FILE *fp1, *fp2;
	register int n;
	char buf[BUFSIZ];
	char full[MAXFULLNAME];

	if ((fp1 = fopen(f1, "r")) == NULL)
		return(FAIL);
	(void) strcpy(full, f2);
	if (DIRECTORY(f2)) {
	    (void) strcat(full, "/");
	    (void) strcat(full, BASENAME(f1, '/'));
	    (void) strcpy(f2, full);
	}

	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,458, "full %s\n")), full);
	if ((fp2 = fopen(full, "w")) == NULL) {
	    (void) fclose(fp1);
	    return(FAIL);
	}
	(void) chmod(full, 0666);

	/* copy -- check errors later */
	while ( (n = fread(buf, sizeof (char), sizeof buf, fp1)) != 0)
	    (void) fwrite(buf, sizeof (char), n, fp2);

	/* check for any errors */
	n = ferror(fp1) | ferror(fp2);
	n |= fclose(fp1) | fclose(fp2);

	if (n)
	    return(FAIL);
	return(0);
}

/*
 * copy f1 to f2 locally after by toggling the effective user id.
 *	f1	-> source file name
 *	f2	-> destination file name
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 */

/*
 *	Similar to xcp with the following chages - 1-15-1993  -ABI
 *	a. The effective user id set to that of the user before 
 *	   the file f1 (user's file) is opened/read. This is
 *	   to allow reading a file which has permission
 *	   only for the owner of the file.
 *	b. The effective use id was reset back to that of uucp 
 *	   before accessing file(s) (in) f2 because, the .Workspace
 *	   directory has search/write permission only for uucp.
 *	Originally, the effective user id was switched to that of the user
 *	before this routine was called. 
*/

seid_xcp(f1, f2)
char *f1, *f2;
{
	register FILE *fp1, *fp2;
	register int n;
	char buf[BUFSIZ];
	char full[MAXFULLNAME];

 	(void) setuid(Uid); /* Switch effective user id to user's to enable
				opening user's file. */

	if ((fp1 = fopen(f1, "r")) == NULL)
		return(FAIL);
	(void) strcpy(full, f2);
 	setresuid(Uid, Euid, -1); /* Switch effective user id to uucp's 
				     to enable opening files in .Workspace 
				     directory */
	if (DIRECTORY(f2)) {
	    (void) strcat(full, "/");
	    (void) strcat(full, BASENAME(f1, '/'));
	    (void) strcpy(f2, full);
	}

	DEBUG(4, (catgets(nlmsg_fd,NL_SETN,458, "full %s\n")), full);
	if ((fp2 = fopen(full, "w")) == NULL) {
	    (void) fclose(fp1);
	    return(FAIL);
	}
	(void) chmod(full, 0666);

	/* copy the user's file to the .Workspace directory */

 	while (1)	{
		/*	Make eid to that of the user.	*/
 		(void) setuid(Uid);
 		n = fread(buf, sizeof (char), sizeof buf, fp1);
		/*	Make eid to that of uucp.	*/
 		setresuid(Uid, Euid, -1);
 		if (n <= 0)
 			break;
 		(void) fwrite(buf, sizeof (char), n, fp2);
 	}

	/* check for any errors */
	n = ferror(fp1) | ferror(fp2);
	n |= fclose(fp1) | fclose(fp2);

	if (n)
	    return(FAIL);
	return(0);
}

/*
 * move f1 to f2 locally
 * returns:
 *	0	-> ok
 *	FAIL	-> failed
 */

xmv(f1, f2)
register char *f1, *f2;
{
	register int ret;

	(void) unlink(f2);   /* i'm convinced this is the right thing to do */
	if ( (ret = link(f1, f2)) < 0) {
	    /* copy file */
	    ret = xcp(f1, f2);
	}

	if (ret == 0)
	    (void) unlink(f1);
	return(ret);
}


/* toCorrupt - move file to CORRUPTDIR
 * return - none
 */

void
toCorrupt(file)
char *file;
{
	char corrupt[MAXFULLNAME];

	(void) sprintf(corrupt, "%s/%s", CORRUPTDIR, BASENAME(file, '/'));
	(void) link(file, corrupt);
	ASSERT(unlink(file) == 0, Ct_UNLINK, file, errno);
	return;
}

/*
 * append f1 to f2
 *	f1	-> source FILE pointer
 *	f2	-> destination FILE pointer
 * return:
 *	SUCCESS	-> ok
 *	FAIL	-> failed
 */
xfappend(fp1, fp2)
register FILE	*fp1, *fp2;
{
	register int nc;
	char	buf[BUFSIZ];

	while ((nc = fread(buf, sizeof (char), BUFSIZ, fp1)) > 0)
		(void) fwrite(buf, sizeof (char), nc, fp2);

	return(ferror(fp1) || ferror(fp2) ? FAIL : SUCCESS);
}


/*
 * copy f1 to f2 locally under uid of uid argument
 *	f1	-> source file name
 *	f2	-> destination file name
 *	Uid and Euid are global
 * return:
 *	0	-> ok
 *	FAIL	-> failed
 * NOTES:
 *  for V7 systems, flip-flop between real and effective uid is
 *  not allowed, so fork must be done.  This code will not
 *  work correctly when realuid is root on System 5 because of
 *  a bug in setuid.
 */

uidxcp(f1, f2)
char *f1, *f2;
{
	int status;
	char full[MAXFULLNAME];

	(void) strcpy(full, f2);
	if (DIRECTORY(f2)) {
	    (void) strcat(full, "/");
	    (void) strcat(full, BASENAME(f1, '/'));
	}

	/* create full owned by uucp */
	(void) close(creat(full, 0666));
	(void) chmod(full, 0666);

	/* do file copy as read uid */
#ifndef V7	/* Removed setuid(Uid). Toggling between effective user ids
		   is done in seid_xcp now.
		*/
	status = seid_xcp(f1, full);
	return(status);

#else V7
	/* This may not work. No longer supported. */
	if (vfork() == 0) {
	    setuid(Uid);
	    _exit (xcp(f1, full));
	}
	wait(&status);
	return(status);
#endif
}
