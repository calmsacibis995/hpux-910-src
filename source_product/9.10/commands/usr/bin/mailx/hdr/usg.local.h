/* @(#) $Revision: 66.1 $ */     
/*
 * Declarations and constants specific to an installation.
 */
 
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 */

#define	GETHOST				/* Have gethostname syscall */ 
/* #define	UNAME				/* System has uname syscall */
#define	LOCAL		EMPTYID		/* Dynamically determined local host */

#define MYDOMAIN	".uucp"		/* Appended to local host name */

#define	MAIL		"/bin/mail"	/* Name of mail sender */
#define DELIVERMAIL     "/usr/lib/sendmail"
					/* Name of classy mail deliverer */
#define	EDITOR		"ed"		/* Name of text editor */
#define	VISUAL		"vi"		/* Name of display editor */
#define	PG		(value("PAGER") ? value("PAGER") : "pg")
					/* Standard output pager */
#define	LS		(value("LISTER") ? value("LISTER") : "ls")
					/* Name of directory listing prog*/
#define	SHELL		"/bin/sh"	/* Standard shell */
#define TMPDIR		"/tmp"
#define HELPFNAME	"mailx.help"
#define	HELPFILE	libpath(HELPFNAME)
					/* Name of casual help file */
#define	NL_HELPFILE	nlspath(HELPFNAME)
					/* Name of localized help file */
#define THELPFNAME	"mailx.help.~"
#define	THELPFILE	libpath(THELPFNAME)
					/* Name of casual tilde help */
#define	NL_THELPFILE	nlspath(THELPFNAME)
					/* Name of localized help file */
#define	UIDMASK		0177777		/* Significant uid bits */
#define	MASTER		libpath("mailx.rc")
#define	APPEND				/* New mail goes to end of mailbox */
#define CANLOCK				/* Locking protocol actually works */
#define	UTIME				/* System implements utime(2) */

#define MAILNAME	"/usr/mail/"	/* home of mail files */

#define LOGNAME		"LOGNAME"	/* login name (USER on UCB) */

#define FORWARD		"Forward to "	/* Bell style forwarding */
#define FRWRDLEN	11		/* len above */

#ifndef VMUNIX
#include "sigretro.h"			/* Retrofit signal defs */
#endif VMUNIX

#ifdef USG
#define	index		strchr
#define rindex		strrchr
#endif USG
