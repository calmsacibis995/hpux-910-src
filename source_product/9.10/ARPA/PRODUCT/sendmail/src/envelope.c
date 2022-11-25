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
 */

# ifndef lint
static char rcsid[] = "$Header: envelope.c,v 1.27.109.9 95/03/22 17:16:10 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)envelope.c	5.22 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: envelope.o $Revision: 1.27.109.9 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# include <sys/types.h>
# include <sys/time.h>
# include <sys/stat.h>
# include <pwd.h>
# include <sys/file.h>
# include "sendmail.h"

static void closexscript();

/*
**  NEWENVELOPE -- allocate a new envelope
**
**	Supports inheritance.
**
**	Parameters:
**		e -- the new envelope to fill in.
**		parent -- the envelope to be the parent of e.
**
**	Returns:
**		e.
**
**	Side Effects:
**		none.
*/

ENVELOPE *
newenvelope(e, parent)
	register ENVELOPE *e;
	register ENVELOPE *parent;
{
	extern ENVELOPE BlankEnvelope;

	if (e == parent && e->e_parent != NULL)
		parent = e->e_parent;
	clearenvelope(e, TRUE);
	if (e == CurEnv)
		bcopy((char *) &NullAddress, (char *) &e->e_from, sizeof e->e_from);
	else
		bcopy((char *) &CurEnv->e_from, (char *) &e->e_from, sizeof e->e_from);
	e->e_parent = parent;
	e->e_ctime = curtime();
	if (parent != NULL)
		e->e_msgpriority = parent->e_msgsize;
	e->e_puthdr = putheader;
	e->e_putbody = putbody;
	if (CurEnv->e_xfp != NULL)
		(void) fflush(CurEnv->e_xfp);

	return (e);
}
/*
**  DROPENVELOPE -- deallocate an envelope.
**
**	Parameters:
**		e -- the envelope to deallocate.
**
**	Returns:
**		none.
**
**	Side Effects:
**		housekeeping necessary to dispose of an envelope.
**		Unlocks this queue file.
*/

void dropenvelope(e)
	register ENVELOPE *e;
{
	bool queueit = FALSE;
	register ADDRESS *q;
	char *id = e->e_id;

# ifdef DEBUG
	if (tTd(50, 1))
	{
		printf("dropenvelope %x: id=", e);
		xputs(e->e_id);
		printf(", flags=0x%x\n", e->e_flags);
		if (tTd(50, 10))
		{
			printf("sendq=");
			printaddr(e->e_sendqueue, TRUE);
		}
	}

	/* we must have an id to remove disk files */
	if (id == NULL)
		return;

# endif /* DEBUG */
# ifdef LOG
	if (LogLevel > 4 && bitset(EF_LOGSENDER, e->e_flags))
		logsender(e, NULL);
	if (LogLevel > 10)
		syslog(LOG_DEBUG, "dropenvelope, id=%s, flags=0x%x, pid=%d",
				  id, e->e_flags, getpid());
# endif /* LOG */
	e->e_flags &= ~EF_LOGSENDER;

	QueueRun = FALSE;

	/*
	**  Extract state information from dregs of send list.
	*/

	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		if (bitset(QQUEUEUP, q->q_flags))
			queueit = TRUE;
		if (!bitset(QDONTSEND, q->q_flags) &&
		    bitset(QBADADDR, q->q_flags))
		{
			if (strcmp(e->e_from.q_paddr, "<>") != 0)
				sendtolist(e->e_from.q_paddr, NULL,
						  &e->e_errorqueue, e);
		}
	}

	/*
	**  Send back return receipts as requested.
	*/

	if (e->e_receiptto != NULL && bitset(EF_SENDRECEIPT, e->e_flags))
	{
		auto ADDRESS *rlist = NULL;

		sendtolist(e->e_receiptto, NULLADDR, &rlist, e);
		(void) returntosender("Return receipt", rlist, FALSE, e);
	}

	/*
	**  Arrange to send error messages if there are fatal errors.
	*/

	if (e->e_sendmode != SM_VERIFY &&
	    (bitset(EF_FATALERRS|EF_TIMEOUT, e->e_flags) && e->e_errormode != EM_QUIET))
		savemail(e);

	/*
	**  Instantiate or deinstantiate the queue.
	*/

	if ((!queueit && !bitset(EF_KEEPQUEUE, e->e_flags)) ||
	    bitset(EF_CLRQUEUE, e->e_flags))
	{
		if (e->e_df != NULL)
			xunlink(e->e_df);
		xunlink(queuename(e, 'q'));
	}
	else if (queueit || !bitset(EF_INQUEUE, e->e_flags))
	{
# ifdef QUEUE
		FILE *lockfp, *queueup();
		lockfp = queueup(e, FALSE, FALSE, TRUE);
		if (lockfp != NULL)
			(void) fclose(lockfp);
# else /* QUEUE */
		syserr("554 dropenvelope: queueup");
# endif /* QUEUE */
	}

	/* now unlock the job */
	closexscript(e);
	unlockqueue(e);

	/* make sure that this envelope is marked unused */
	e->e_id = e->e_df = NULL;
	if (e->e_dfp != NULL)
		(void) fclose(e->e_dfp);
	e->e_dfp = NULL;
}
/*
**  CLEARENVELOPE -- clear an envelope without unlocking
**
**	This is normally used by a child process to get a clean
**	envelope without disturbing the parent.
**
**	Parameters:
**		e -- the envelope to clear.
**		fullclear - if set, the current envelope is total
**			garbage and should be ignored; otherwise,
**			release any resources it may indicate.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Closes files associated with the envelope.
**		Marks the envelope as unallocated.
*/

void clearenvelope(e, fullclear)
	register ENVELOPE *e;
	bool fullclear;
{
	register HDR *bh;
	register HDR **nhp;
	extern ENVELOPE BlankEnvelope;

	if (!fullclear)
	{
		/* clear out any file information */
		if (e->e_xfp != NULL)
			(void) fclose(e->e_xfp);
		if (e->e_dfp != NULL)
			(void) fclose(e->e_dfp);
	}

	/* now clear out the data */
	STRUCTCOPY(BlankEnvelope, *e);
	if (Verbose)
		e->e_sendmode = SM_DELIVER;
	bh = BlankEnvelope.e_header;
	nhp = &e->e_header;
	while (bh != NULL)
	{
		*nhp = (HDR *) xalloc(sizeof *bh);
		bcopy((char *) bh, (char *) *nhp, sizeof *bh);
		bh = bh->h_link;
		nhp = &(*nhp)->h_link;
	}
}
/*
**  INITSYS -- initialize instantiation of system
**
**	In Daemon mode, this is done in the child.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Initializes the system macros, some global variables,
**		etc.  In particular, the current time in various
**		forms is set.
*/

void initsys(e)
	register ENVELOPE *e;
{
	char cbuf[5];				/* holds hop count */
	char pbuf[10];				/* holds pid */
# ifdef TTYNAME
	static char ybuf[60];			/* holds tty id */
	register char *p;
# endif /* TTYNAME */
	extern char *ttyname();
	extern void settime();
	extern char Version[];

	/*
	**  Give this envelope a reality.
	**	I.e., an id, a transcript, and a creation time.
	*/

	openxscript(e);
	e->e_ctime = curtime();

	/*
	**  Set OutChannel to something useful if stdout isn't it.
	**	This arranges that any extra stuff the mailer produces
	**	gets sent back to the user on error (because it is
	**	tucked away in the transcript).
	*/

	if (OpMode == MD_DAEMON && QueueRun)
		OutChannel = e->e_xfp;

	/*
	**  Set up some basic system macros.
	*/

	/* process id */
	(void) sprintf(pbuf, "%d", getpid());
	define('p', newstr(pbuf), e);

	/* hop count */
	(void) sprintf(cbuf, "%d", e->e_hopcount);
	define('c', newstr(cbuf), e);

	/* time as integer, unix time, arpa time */
	settime(e);

# ifdef TTYNAME
	/* tty name */
	if (macvalue('y', e) == NULL)
	{
		p = ttyname(2);
		if (p != NULL)
		{
			if (strrchr(p, '/') != NULL)
				p = strrchr(p, '/') + 1;
			(void) strcpy(ybuf, p);
			define('y', ybuf, e);
		}
	}
# endif /* TTYNAME */
}
/*
**  SETTIME -- set the current time.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets the various time macros -- $a, $b, $d, $t.
*/

void
settime(e)
	register ENVELOPE *e;
{
	register char *p;
	auto time_t now;
	char tbuf[20];				/* holds "current" time */
	char dbuf[30];				/* holds ctime(tbuf) */
	register struct tm *tm;
	extern char *arpadate();
	extern struct tm *gmtime();

	now = curtime();
	tm = gmtime(&now);
	(void) sprintf(tbuf, "%04d%02d%02d%02d%02d", tm->tm_year + 1900,
			tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);
	define('t', newstr(tbuf), e);
	(void) strcpy(dbuf, ctime(&now));
	p = strchr(dbuf, '\n');
	if (p != NULL)
		*p = '\0';
	define('d', newstr(dbuf), e);
	p = arpadate(dbuf);
	p = newstr(p);
	if (macvalue('a', e) == NULL)
		define('a', p, e);
	define('b', p, e);
}
/*
**  OPENXSCRIPT -- Open transcript file
**
**	Creates a transcript file for possible eventual mailing or
**	sending back.
**
**	Parameters:
**		e -- the envelope to create the transcript in/for.
**
**	Returns:
**		none
**
**	Side Effects:
**		Creates the transcript file.
*/

void openxscript(e)
	register ENVELOPE *e;
{
	register char *p;
	int fd;

# ifdef LOG
	if (LogLevel > 19)
		syslog(LOG_DEBUG, "%s: openx%s", e->e_id, e->e_xfp == NULL ? "" : " (no)");
# endif /* LOG */
	if (e->e_xfp != NULL)
		return;
	p = queuename(e, 'x');
	fd = open(p, O_WRONLY|O_CREAT|O_APPEND, 0644);
	if (fd < 0)
		syserr("Can't create %s", p);
	else
		e->e_xfp = fdopen(fd, "w");
}
/*
**  CLOSEXSCRIPT -- close the transcript file.
**
**	Parameters:
**		e -- the envelope containing the transcript to close.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

static void closexscript(e)
	register ENVELOPE *e;
{
	if (e->e_xfp == NULL)
		return;
	(void) fclose(e->e_xfp);
	e->e_xfp = NULL;
}
/*
**  SETSENDER -- set the person who this message is from
**
**	Under certain circumstances allow the user to say who
**	s/he is (using -f or -r).  These are:
**	1.  The user's uid is zero (root).
**	2.  The user's login name is in an approved list (typically
**	    from a network server).
**	3.  The address the user is trying to claim has a
**	    "!" character in it (since #2 doesn't do it for
**	    us if we are dialing out for UUCP).
**	A better check to replace #3 would be if the
**	effective uid is "UUCP" -- this would require me
**	to rewrite getpwent to "grab" uucp as it went by,
**	make getname more nasty, do another passwd file
**	scan, or compile the UID of "UUCP" into the code,
**	all of which are reprehensible.
**
**	Assuming all of these fail, we figure out something
**	ourselves.
**
**	Parameters:
**		from -- the person we would like to believe this message
**			is from, as specified on the command line.
**		e -- the envelope in which we would like the sender set.
**
**	Returns:
**		none.
**
**	Side Effects:
**		sets sendmail's notion of who the from person is.
*/

void setsender(from, e)
	char *from;
	register ENVELOPE *e;
{
	register char **pvp;
	char *realname = NULL;
	register struct passwd *pw;
	char buf[MAXNAME + 2];
	char pvpbuf[PSBUFSIZE];
	extern struct passwd *getpwnam();
	extern char *FullName;

# ifdef DEBUG
	if (tTd(45, 1))
		printf("setsender(%s)\n", from == NULL ? "" : from);
# endif /* DEBUG */

	/*
	**  Figure out the real user executing us.
	**	Username can return errno != 0 on non-errors.
	*/

	if (QueueRun || OpMode == MD_SMTP || OpMode == MD_ARPAFTP)
	{
		if (from[0] == '\0')
			from = newstr("<>");
                realname = from;
	}
	if (realname == NULL || realname[0] == '\0')
		realname = username();

	/*
	**  Determine if this real person is allowed to alias themselves.
	*/

	if (from != NULL)
	{
		extern bool trusteduser();

		if (!trusteduser(realname) && getuid() != geteuid() &&
		    strchr(from, '!') == NULL && getuid() != 0)
		{
			/* network sends -r regardless (why why why?) */
			/* syserr("%s, you cannot use the -f flag", realname); */
			from = NULL;
		}
	}

	SuprErrs = TRUE;
	if (from == NULL || parseaddr(from, &e->e_from, 1, '\0', e) == NULL)
	{
		/* log garbage addresses for traceback */
		if (from != NULL)
		{
# ifdef LOG
			if (LogLevel >= 1)
			    if (realname == from && RealHostName != NULL)
				syslog(LOG_NOTICE,
				    "from=%s unparseable, received from %s",
				    from, RealHostName ? RealHostName : "local host");
			    else
				syslog(LOG_NOTICE,
				    "Unparseable username %s wants from=%s",
				    realname, from);
# endif /* LOG */
		}
		from = newstr(realname);
		if (parseaddr(from, &e->e_from, 1, '\0', e) == NULL)
		{
			/*
			**  parseaddr() was unable to fill in e->e_from;
			**  we allocate a (blank) mailer, so indirection through
			**  e->e_from.q_mailer doesn't dump core
			*/
			int size = sizeof(struct mailer);

# ifdef LOG
			if (from == NULL || (from != NULL && *from != '\0'))
				if (LogLevel >= 1 && strcasecmp(from, realname))
					syslog(LOG_NOTICE, "%s: from=%s also unparsable",
					       e->e_id, from);
# endif /* LOG */
			e->e_from.q_mailer = (struct mailer *) xalloc(size);
			memset(e->e_from.q_mailer, 0, size);
		}
	}
	else
		FromFlag = TRUE;
	e->e_from.q_flags |= QDONTSEND;
	loweraddr(&e->e_from);
	SuprErrs = FALSE;

	pvp = NULL;
	if (e->e_from.q_mailer == LocalMailer &&
	    (pw = getpwnam(e->e_from.q_user)) != NULL)
	{
		register char *p;
		extern char *aliaslookup();

		if (ReverseAliasDB){
			p = aliaslookup(ReverseAliasDB, e->e_from.q_user);
			if (p != NULL){
				/*
				** We have an alternate address for the sender.
				*/
# ifdef DEBUG
				if (tTd(45, 2))
					printf("%s reverse-aliased to %s\n", e->e_from.q_user, p);
# endif /* DEBUG */
				pvp = prescan(p, '\0', pvpbuf);
			}
		}

		/*
		**  Process passwd file entry.
		*/

		/* extract home directory */
		e->e_from.q_home = newstr(pw->pw_dir);
		define('z', e->e_from.q_home, e);

		/* extract user and group id */
		e->e_from.q_uid = pw->pw_uid;
		e->e_from.q_gid = pw->pw_gid;

		/* if the user has given fullname already, don't redefine */
		if (FullName == NULL)
			FullName = macvalue('x', e);
		if (FullName != NULL && FullName[0] == '\0')
			FullName = NULL;

		/* extract full name from passwd file */
		if (FullName == NULL && pw->pw_gecos != NULL &&
		    strcmp(pw->pw_name, e->e_from.q_user) == 0)
		{
			buildfname(pw->pw_gecos, e->e_from.q_user, buf);
			if (buf[0] != '\0')
				FullName = newstr(buf);
		}
		if (FullName != NULL)
			define('x', FullName, e);
	}
	else
	{
		if (e->e_from.q_home == NULL)
			e->e_from.q_home = getenv("HOME");
		e->e_from.q_uid = getuid();
		e->e_from.q_gid = getgid();
	}

	/*
	**  Rewrite the from person to dispose of possible implicit
	**	links in the net.
	*/

	if (pvp == NULL)
		pvp = prescan(from, '\0', pvpbuf);
	if (pvp == NULL)
	{
# ifdef LOG
		if (LogLevel >= 1)
			syslog(LOG_NOTICE, "Cannot prescan from (%s)", from);
# endif	/* LOG */
		usrerr("Cannot prescan from (%s)", from);
		finis();
	}
	rewrite(pvp, 3, e);
	rewrite(pvp, 1, e);
	rewrite(pvp, 4, e);
	cataddr(pvp, buf, sizeof buf);
	e->e_sender = newstr(buf);
	define('f', e->e_sender, e);

	/* save the domain spec if this mailer wants it */
	if (e->e_from.q_mailer != NULL &&
	    bitnset(M_CANONICAL, e->e_from.q_mailer->m_flags))
	{
		char **npvp, **p, **q;

		/* Copy the address, eliminating angle brackets */
		npvp = copyplist(pvp, FALSE);
		for (p = pvp, q = npvp;  *p != NULL;  p++)
			if (strcmp(*p, "<") && strcmp(*p, ">"))
				*q++ = *p;
		*q = NULL;

		/*
		**  Find the last occurrance of @, perhaps following
		**  a colon.
		*/
		for (p = npvp, q = NULL;  *p != NULL;  p++) {
			if (strcmp(*p, "@") == 0)
				q = p;
			else if (strcmp(*p, ":") == 0)
				q = NULL;
		}

		/* If there was a domain, save it */
		if (q != NULL && *q != NULL)
			e->e_fromdomain = copyplist(q, TRUE);
	}
}
/*
**  TRUSTEDUSER -- tell us if this user is to be trusted.
**
**	Parameters:
**		user -- the user to be checked.
**
**	Returns:
**		TRUE if the user is in an approved list.
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
trusteduser(user)
	char *user;
{
	register char **ulist;
	extern char *TrustedUsers[];

	for (ulist = TrustedUsers; *ulist != NULL; ulist++)
		if (strcmp(*ulist, user) == 0)
			return (TRUE);
	return (FALSE);
}
