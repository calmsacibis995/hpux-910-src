
/*	@(#) $Revision: 56.1 $	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	/sccs/src/cmd/uucp/s.uucpname.c
	uucpname.c	1.1	7/29/85 16:33:52
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS
#include "uucp.h"
#if defined(DUX) || defined(DISKLESS)
#include <sys/types.h>
#include <cluster.h>
#endif


/*
 * get the uucp name
 * return:
 *	none
 */
void
uucpname(name)
register char *name;
{
	char *s;
#if defined(DUX) || defined(DISKLESS)
	char servername[24];
#endif

#ifdef BSD4_2
	int nlen;
	char	NameBuf[MAXBASENAME + 1];

	/* This code is slightly wrong, at least if you believe what the */
	/* 4.1c manual says.  It claims that gethostname's second parameter */
	/* should be a pointer to an int that has the size of the buffer. */
	/* The code in the kernel says otherwise.  The manual also says that */
	/* the string returned is null-terminated; this, too, appears to be */
	/* contrary to fact.  Finally, the variable containing the length */
	/* is supposed to be modified to have the actual length passed back; */
	/* this, too, doesn't happen.  So I'm zeroing the buffer first, and */
	/* passing an int, not a pointer to one.  *sigh*

	/*		--Steve Bellovin	*/
	bzero(NameBuf, sizeof NameBuf);
	nlen = sizeof NameBuf;
	gethostname(NameBuf, nlen);
	s = NameBuf;
	s[nlen] = '\0';
#else !BSD4_2
#ifdef UNAME
	struct utsname utsn;

	uname(&utsn);
	s = utsn.nodename;
#else !UNAME
	char	NameBuf[MAXBASENAME + 1], *strchr();
	FILE	*NameFile;

	s = MYNAME;
	NameBuf[0] = '\0';

	if ((NameFile = fopen("/etc/whoami", "r")) != NULL) {
		/* etc/whoami wins */
		(void) fgets(NameBuf, MAXBASENAME + 1, NameFile);
		(void) fclose(NameFile);
		NameBuf[MAXBASENAME] = '\0';
		if (NameBuf[0] != '\0') {
			if ((s = strchr(NameBuf, '\n')) != NULL)
				*s = '\0';
			s = NameBuf;
		}
	}
#endif UNAME
#endif BSD4_2

#if defined(DUX) || defined(DISKLESS)
    if (getservername(servername) == 0)
	s = servername;
#endif
	(void) strncpy(name, s, MAXBASENAME);
	name[MAXBASENAME] = '\0';
	return;
}

#if defined(DUX) || defined(DISKLESS)
int getservername(servername)
char *servername;
{
    struct cct_entry *cctptr;

    while ((cctptr = getccent()) != (struct cct_entry *)0) {
	if (cctptr->cnode_type == 'r') {
	    strcpy(servername,cctptr->cnode_name);
	    endccent();
	    return(0);
	}
    }

    return(1);
}
#endif
