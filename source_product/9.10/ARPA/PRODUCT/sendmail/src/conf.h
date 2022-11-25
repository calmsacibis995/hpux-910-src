/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *      $Header: conf.h,v 1.14.109.6 95/02/21 16:07:31 mike Exp $
 *	@(#)conf.h	5.17 (Berkeley) 6/1/90
 */
# ifdef PATCH_STRING
/* static char *patch_3997="@(#) PATCH_9.03: conf.h $Revision: 1.14.109.6 $ 94/03/24 PHNE_3997"; */
# endif	/* PATCH_STRING */

/*
**  CONF.H -- All user-configurable parameters for sendmail
*/

# include <sys/param.h>

/*
**  Table sizes, etc....
**	There shouldn't be much need to change these....
*/

# define MAXLINE	1024		/* max line length */
# define MAXNAME	512		/* max length of a name */
# define MAXFIELD	8192		/* max total length of a hdr field */
# define MAXPV		40		/* max # of parms to mailers */
# define MAXHOP		30		/* max value of HopCount */
# define MAXATOM	100		/* max atoms per address */
# define MAXMAILERS	25		/* maximum mailers known to system */
# define MAXRWSETS	30		/* max # of sets of rewriting rules */
# define MAXPRIORITIES	25		/* max values for Precedence: field */
# define MAXTRUST	30		/* maximum number of trusted users */
# define MAXUSERENVIRON	40		/* max # of items in user environ */
# define QUEUESIZE	600		/* max # of jobs per queue run */
# define MAXMXHOSTS	10		/* max # of MX records */

/*
**  Size of tobuf (deliver.c)
**	Tweak this to match your syslog implementation.  It will have to
**	allow for the extra information printed.
*/

# define TOBUFSIZE	(BUFSIZ - 256)

/*
**  Compilation options.
**
**	#define these if they are available; comment them out otherwise.
*/

# define NDBM		1	/* new DBM library available (requires DBM) */
# define LOG		1	/* enable logging */
# define SMTP		1	/* enable user and server SMTP */
# define QUEUE		1	/* enable queueing */
# define UGLYUUCP	1	/* output ugly UUCP From lines */
# define DAEMON		1	/* include the daemon (requires IPC & SMTP) */
# define SETPROCTITLE	1	/* munge argv to display current status */
# define NAMED_BIND	1	/* use Berkeley Internet Domain Server */

# include <varargs.h>

	/*
	 * Use query type of ANY if possible (NO_WILDCARD_MX), which will
	 * find types CNAME, A, and MX, and will cause all existing records
	 * to be cached by our local server.  If there is (might be) a
	 * wildcard MX record in the local domain or its parents that are
	 * searched, we can't use ANY; it would cause fully-qualified names
	 * to match as names in a local domain.
	 */
# define NO_WILDCARD_MX	1


/*
**  HP Sendmail Configuration Values
*/

# define DEBUG		1	/* Include BSD debug code */
# define BIGBUFSIZ	32766	/* Socket && write buffer size */
# define PSTAT		1	/* Use pstat() for setproctitle and getla */

/* Supports NIS Aliases *****/
# define ALIAS_MAP      "mail.aliases"  /* default NIS map for aliases */

# define GIDSET_T	gid_t
