/* @(#) $Revision: 67.1 $ */    
static char *HPUX_ID = "@(#) $Revision: 67.1 $";


/*
 * For use with:
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 *	This "library command" removes an empty file with mode
 *	0660 from a directory modifiable and accessable by
 *	the effective userID of the invoker or by group mail,
 *	provided that the effective userID matches the file's
 *	owner or the real groupID of the invoker is mail (and
 *	the file's group is mail).
 *
 *	In particular, this "command"  is used by mailx to remove
 *	mailboxes (presumably from the /usr/mail directory).
 *	It is assumed that mailboxes can be removed from
 *	the /usr/mail directory by running under the effective
 *	group ID mail; thus this program must run setgid mail.
 *	The permission checks herein are to prevent users from
 *	removing other peoples mailboxes by invoking this "command"
 *	directly.
 *
 *	Mailboxes should be successfully removed when all of the
 *	following conditions are met:
 *
 *		1) The file is empty (mailx should assure this).
 *		2) The mode is 0660.
 *		3) The effective uid matches the owner's uid
 *		   or the real gid is mail (as well as the
 *		   file's group).
 *		4) There are no optional ACL entries on the file.
 *
 *	NOTE: This program would not be needed if mailx ran
 *	setgid mail.  If mailx ran setgid mail, however, it
 *	would have to go to great pains to ensure that the user
 *	did not acquire group mail privileges for any of its
 *	other functions.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#endif

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 20	/* set number */
#include <nl_types.h>
nl_catd nl_fn;
#endif NLS

#define	DONE(x)	done(x,argv[1])

main(argc,argv)
int argc;
char *argv[];
{
	struct stat stbuf;

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
        	set_auth_parameters(argc,argv);
        	initprivs();
        	disablesysauth(SEC_MULTILEVELDIR);
	}
#endif

#ifdef NLS
	nl_fn=catopen("mailx",0);	 /* share same catalog with mailx */
#endif

	if (argc != 2)
		DONE(1);
	if (stat(argv[1],&stbuf) != 0)
		DONE(2);
	if (stbuf.st_size != 0)
		DONE(3);
	if ((stbuf.st_mode & 0777) != 0660)
		DONE(4);
#ifdef ACLS
	/* 
	 * If the file has optional ACL entries, treat it as not 0660:
	 */
	if (stbuf.st_acl)
		DONE(4);
#endif /* ACLS */

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
        	if (geteuid()==stbuf.st_uid || hassysauth(SEC_ALLOWDACACCESS)){
                	enablepriv(SEC_ALLOWMACACCESS);
                	enablepriv(SEC_ALLOWDACACCESS);
                	enablepriv(SEC_WRITEUPSYSHI);
                	enablepriv(SEC_WRITEUPCLEARANCE);
                	enablepriv(SEC_OWNER);
                	DONE(unlink(argv[1]));
        	}
	} else {
		if (geteuid() == stbuf.st_uid)
			DONE(unlink(argv[1]));
	}
#else
	if (geteuid() == stbuf.st_uid)
		DONE(unlink(argv[1]));
#endif
	if (getgid() == stbuf.st_gid)
		DONE(unlink(argv[1]));
	DONE(5);
}

done(status, file)
int status;
char *file;
{
	switch(status) {
	case -1:
		fprintf(stderr,(catgets(nl_fn,NL_SETN,1, "rmmail: cannot unlink %s\n")),file);
		break;
	case 0:
		break;
	case 1:
		fprintf(stderr,(catgets(nl_fn,NL_SETN,2, "rmmail: invalid arguments\n")));
		break;
	case 2:
		fprintf(stderr,(catgets(nl_fn,NL_SETN,3, "rmmail: cannot stat %s\n")),file);
		break;
	case 3:
		fprintf(stderr,(catgets(nl_fn,NL_SETN,4, "rmmail: %s not empty\n")),file);
		break;
	case 4:
		break;
	case 5:
		fprintf(stderr,(catgets(nl_fn,NL_SETN,5, "rmmail: cannot remove %s - permission denied\n")),file);
		break;
	default:
		fprintf(stderr,(catgets(nl_fn,NL_SETN,6, "rmmail: bad status\n")));
		break;
	}
	exit(status);
}
