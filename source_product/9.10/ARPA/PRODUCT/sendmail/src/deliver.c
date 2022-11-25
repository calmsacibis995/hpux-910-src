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
static char rcsid[] = "$Header: deliver.c,v 1.38.109.11 95/02/21 16:07:41 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)deliver.c	5.38 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: deliver.o $Revision: 1.38.109.11 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include "sendmail.h"
# include <sys/signal.h>
# include <sys/stat.h>
# include <sys/unistd.h>
# include <netdb.h>
# include <fcntl.h>
# include <errno.h>
# ifdef NAMED_BIND
# 	include <arpa/nameser.h>
# 	include <resolv.h>
# endif	/* NAMED_BIND */

# ifdef SMTP
extern char	SmtpError[];
#  endif /* SMTP */

static void markfailure();

int Deliveries;  /* Count the number of messages we attempted to deliver */

# define temporary(s) (((s)==EX_TEMPFAIL)||((s)==EX_IOERR)||((s)==EX_OSERR))

/*
**  DELIVER -- Deliver a message to a list of addresses.
**
**	This routine delivers to everyone on the same host as the
**	user on the head of the list.  It is clever about mailers
**	that don't handle multiple users.  It is NOT guaranteed
**	that it will deliver to all these addresses however -- so
**	deliver should be called once for each address on the
**	list.
**
**	Parameters:
**		e -- the envelope to deliver.
**		firstto -- head of the address list to deliver to.
**
**	Returns:
**		zero -- successfully delivered.
**		else -- some failure, see ExitStat for more info.
**
**	Side Effects:
**		The standard input is passed off to someone.
*/

deliver(e, firstto)
	register ENVELOPE *e;
	ADDRESS *firstto;
{
	char *host;			/* host being sent to */
	char *user;			/* user being sent to */
	char **pvp;
	register char **mvp;
	register char *p;
	register MAILER *m;		/* mailer for this recipient */
	ADDRESS *ctladdr;
	register ADDRESS *to = firstto;
	bool clever = FALSE;		/* running user smtp to this mailer */
	ADDRESS *tochain = NULL;	/* chain of users in this mailer call */
	int rcode;			/* response code */
	char *firstsig;			/* signature of firstto */
	char *curhost;
	char *pv[MAXPV+1];
	char tobuf[TOBUFSIZE];		/* text line of to people */
	char buf[MAXNAME];
	char tfrombuf[MAXNAME];		/* translated from person */
	extern int checkcompat();

	ServerAddress = NULL;
	errno = 0;
	if (bitset(QDONTSEND|QBADADDR|QQUEUEUP, to->q_flags))
		return (0);

# ifdef NAMED_BIND
	/* unless interactive, try twice, over a minute */
	if (OpMode == MD_DAEMON || OpMode == MD_SMTP)
	{
		_res.retrans = 30;
		_res.retry = 2;
	}
# endif 	/* NAMED_BIND */

	m = to->q_mailer;
	host = to->q_host;

# ifdef DEBUG
	if (tTd(10, 1))
		printf("\n--deliver, mailer=%d, host=`%s', first user=`%s'\n",
			m->m_mno, host, doublepercent(to->q_user));
#  endif /* DEBUG */

	/*
	**  If this mailer is expensive, and if we don't want to make
	**  connections now, just mark these addresses and return.
	**	This is useful if we want to batch connections to
	**	reduce load.  This will cause the messages to be
	**	queued up, and a daemon will come along to send the
	**	messages later.
	**		This should be on a per-mailer basis.
	*/

	if (NoConnect && !QueueRun && bitnset(M_EXPENSIVE, m->m_flags) &&
	    !Verbose)
	{
		for (; to != NULL; to = to->q_next)
		{
			if (bitset(QDONTSEND|QBADADDR|QQUEUEUP, to->q_flags) ||
			    to->q_mailer != m)
				continue;
			to->q_flags |= QQUEUEUP;
			Deliveries++;
			e->e_to = to->q_paddr;
			message("queued");
			if (LogLevel > 8)
				logdelivery("queued", e);
		}
		e->e_to = NULL;
		return (0);
	}

	/*
	**  Do initial argv setup.
	**	Insert the mailer name.  Notice that $x expansion is
	**	NOT done on the mailer name.  Then, if the mailer has
	**	a picky -f flag, we insert it as appropriate.  This
	**	code does not check for 'pv' overflow; this places a
	**	manifest lower limit of 4 for MAXPV.
	**		The from address rewrite is expected to make
	**		the address relative to the other end.
	*/

	/* rewrite from address, using rewriting rules */
	expand("\001f", buf, &buf[sizeof buf - 1], e);
	(void) strcpy(tfrombuf, remotename(buf, m, TRUE, TRUE, e));

	define('g', tfrombuf, e);		/* translated sender address */
	define('h', host, e);			/* to host */
	Errors = 0;
	pvp = pv;
	*pvp++ = m->m_argv[0];

	/* insert -f or -r flag as appropriate */
	if (FromFlag && (bitnset(M_FOPT, m->m_flags) || bitnset(M_ROPT, m->m_flags)))
	{
		if (bitnset(M_FOPT, m->m_flags))
			*pvp++ = "-f";
		else
			*pvp++ = "-r";
		expand("\001g", buf, &buf[sizeof buf - 1], e);
		*pvp++ = newstr(buf);
	}

	/*
	**  Append the other fixed parts of the argv.  These run
	**  up to the first entry containing "$u".  There can only
	**  be one of these, and there are only a few more slots
	**  in the pv after it.
	*/

	for (mvp = m->m_argv; (p = *++mvp) != NULL; )
	{
		while ((p = strchr(p, '\001')) != NULL)
			if (*++p == 'u')
				break;
		if (p != NULL)
			break;

		/* this entry is safe -- go ahead and process it */
		expand(*mvp, buf, &buf[sizeof buf - 1], e);
		*pvp++ = newstr(buf);
		if (pvp >= &pv[MAXPV - 3])
		{
			syserr("554 Too many parameters to %s before $u", pv[0]);
			return (-1);
		}
	}

	/*
	**  If we have no substitution for the user name in the argument
	**  list, we know that we must supply the names otherwise -- and
	**  SMTP is the answer!!
	*/

	if (*mvp == NULL)
	{
		/* running SMTP */
# ifdef SMTP
		clever = TRUE;
		*pvp = NULL;
# else /* SMTP */
		/* oops!  we don't implement SMTP */
		syserr("554 SMTP style mailer not implemented");
		return (EX_SOFTWARE);
# endif /* SMTP */
	}

	/*
	**  At this point *mvp points to the argument with $u.  We
	**  run through our address list and append all the addresses
	**  we can.  If we run out of space, do not fret!  We can
	**  always send another copy later.
	*/

	tobuf[0] = '\0';
	e->e_to = tobuf;
	ctladdr = NULL;
	firstsig = hostsignature(firstto->q_mailer, firstto->q_host, e);
	for (; to != NULL; to = to->q_next)
	{
		/* avoid sending multiple recipients to dumb mailers */
		if (tobuf[0] != '\0' && !bitnset(M_MUSER, m->m_flags))
			break;

		/* if already sent or not for this host, don't send */
		if (bitset(QDONTSEND|QBADADDR|QQUEUEUP, to->q_flags) ||
		    to->q_mailer != firstto->q_mailer ||
		    strcmp(hostsignature(to->q_mailer, to->q_host, e), firstsig) != 0)
			continue;

		/* avoid overflowing tobuf */
		if (sizeof tobuf < (strlen(to->q_paddr) + strlen(tobuf) + 2))
			break;

# ifdef DEBUG
		if (tTd(10, 1))
		{
			printf("\nsend to ");
			printaddr(to, FALSE);
		}
#  endif /* DEBUG */

		/* compute effective uid/gid when sending */
		if (to->q_mailer == ProgMailer)
			ctladdr = getctladdr(to);

		user = to->q_user;
		e->e_to = to->q_paddr;
		to->q_flags |= QDONTSEND;

		/*
		**  Check to see that these people are allowed to
		**  talk to each other.
		*/

		if (m->m_maxsize != 0 && e->e_msgsize > m->m_maxsize)
		{
			NoReturn = TRUE;
			usrerr("552 Message is too large; %ld bytes max", m->m_maxsize);
			giveresponse(EX_UNAVAILABLE, m, e);
			continue;
		}
		rcode = checkcompat(to, e);
		if (rcode != EX_OK)
		{
			markfailure(e, to, rcode);
			giveresponse(EX_UNAVAILABLE, m, e);
			continue;
		}

		/*
		**  Strip quote bits from names if the mailer is dumb
		**	about them.
		*/

		if (bitnset(M_STRIPQ, m->m_flags))
		{
			stripquotes(user);
			stripquotes(host);
		}

		/* hack attack -- delivermail compatibility */
		if (m == ProgMailer && *user == '|')
			user++;

		/*
		**  If an error message has already been given, don't
		**	bother to send to this address.
		**
		**	>>>>>>>>>> This clause assumes that the local mailer
		**	>> NOTE >> cannot do any further aliasing; that
		**	>>>>>>>>>> function is subsumed by sendmail.
		*/

		if (bitset(QBADADDR|QQUEUEUP, to->q_flags))
			continue;

		/* save statistics.... */
		markstats(e, to);

		/* Count the message as a good delivery */
		Deliveries++;

		/*
		**  See if this user name is "special".
		**	If the user name has a slash in it, assume that this
		**	is a file -- send it off without further ado.  Note
		**	that this type of addresses is not processed along
		**	with the others, so we fudge on the To person.
		*/

		if (m == LocalMailer)
		{
			if (user[0] == '/')
			{
				rcode = mailfile(user, getctladdr(to), e);
				giveresponse(rcode, m, e);
				if (rcode == EX_OK)
					to->q_flags |= QSENT;
				continue;
			}
		}

		/*
		**  Address is verified -- add this user to mailer
		**  argv, and add it to the print list of recipients.
		*/

		/* link together the chain of recipients */
		to->q_tchain = tochain;
		tochain = to;

		/* create list of users for error messages */
		(void) strcat(tobuf, ",");
		(void) strcat(tobuf, to->q_paddr);
		define('u', user, e);		/* to user */
		p = to->q_home;
		if (p == NULL && ctladdr != NULL)
			p = ctladdr->q_home;
		define('z', p, e);	/* user's home */

		/*
		**  Expand out this user into argument list.
		*/

		if (!clever)
		{
			expand(*mvp, buf, &buf[sizeof buf - 1], e);
			*pvp++ = newstr(buf);
			if (pvp >= &pv[MAXPV - 2])
			{
				/* allow some space for trailing parms */
				break;
			}
		}
	}

	/* see if any addresses still exist */
	if (tobuf[0] == '\0')
	{
		define('g', (char *) NULL, e);
		return (0);
	}

	/* print out messages as full list */
	e->e_to = tobuf + 1;

	/*
	**  Fill out any parameters after the $u parameter.
	*/

	while (!clever && *++mvp != NULL)
	{
		expand(*mvp, buf, &buf[sizeof buf - 1], e);
		*pvp++ = newstr(buf);
		if (pvp >= &pv[MAXPV])
			syserr("554 deliver: pv overflow after $u for %s", pv[0]);
	}
	*pvp++ = NULL;

	/*
	**  Call the mailer.
	**	The argument vector gets built, pipes
	**	are created as necessary, and we fork & exec as
	**	appropriate.
	**	If we are running SMTP, we just need to clean up.
	*/

	if (ctladdr == NULL)
		ctladdr = &e->e_from;

# ifdef NAMED_BIND
	/*
	**  We do this as needed in getmxrr() and makeconnection().
	*/
	/* _res.options &= ~(RES_DEFNAMES | RES_DNSRCH);           /* XXX */
# endif	/* NAMED_BIND */

	curhost = NULL;

# ifdef SMTP
	if (clever)
	{
		rcode = EX_OK;
		if ((rcode = smtpinit(m, pv, e)) == EX_OK) {
			register char *t = tobuf;
			register int i;

			/* send the recipient list */
			tobuf[0] = '\0';
			for (to = tochain; to; to = to->q_tchain) {
				e->e_to = to->q_paddr;
				if ((i = smtprcpt(to, m, e)) != EX_OK) {
					markfailure(e, to, i);
					giveresponse(i, m, e);
				}
				else {
					*t++ = ',';
					for (p = to->q_paddr; *p; *t++ = *p++);
				}
			}

			/* now send the data */
			if (tobuf[0] == '\0')
				e->e_to = NULL;
			else {
				e->e_to = tobuf + 1;
				rcode = smtpdata(m, e);
			}

			/* now close the connection */
			smtpquit(m, e);
		}
	}
	else
# endif /* SMTP */
        {
                message("Connecting to %s (%s)...", 
			(host == NULL || !strlen(host)) ? "local host" : host, 
			m->m_name);
                rcode = sendoff(e, m, pv, ctladdr);
        }
# ifdef NAMED_BIND
	/* _res.options |= RES_DEFNAMES | RES_DNSRCH;	/* XXX */
# endif	/* NAMED_BIND */

	/*
	**  Do final status disposal.
	**	We check for something in tobuf for the SMTP case.
	**	If we got a temporary failure, arrange to queue the
	**		addressees.
	*/

	if (tobuf[0] != '\0')
		giveresponse(rcode, m, e);
	for (to = tochain; to != NULL; to = to->q_tchain)
	{
		if (rcode != EX_OK)
			markfailure(e, to, rcode);
		else
		{
			to->q_flags |= QSENT;
			e->e_nsent++;
		}
	}

	/*
	**  Restore state and return.
	*/

	errno = 0;
	SmtpError[0] = '\0';
	define('g', (char *) NULL, e);
	return (rcode);
}
/*
**  MARKFAILURE -- mark a failure on a specific address.
**
**	Parameters:
**		e -- the envelope we are sending.
**		q -- the address to mark.
**		rcode -- the code signifying the particular failure.
**
**	Returns:
**		none.
**
**	Side Effects:
**		marks the address (and possibly the envelope) with the
**			failure so that an error will be returned or
**			the message will be queued, as appropriate.
*/

static void markfailure(e, q, rcode)
	register ENVELOPE *e;
	register ADDRESS *q;
	int rcode;
{
	if (rcode == EX_OK)
		return;
	else if (!temporary(rcode))
		q->q_flags |= QBADADDR;
	else if (curtime() > e->e_ctime + TimeOut)
	{
		extern char *pintvl();
		char buf[MAXLINE];

		if (!bitset(EF_TIMEOUT, e->e_flags))
		{
			(void) sprintf(buf, "Cannot send message for %s",
				pintvl(TimeOut, FALSE));
			if (e->e_message != NULL)
				free(e->e_message);
			e->e_message = newstr(buf);
			message(buf);
		}
		q->q_flags |= QBADADDR;
		e->e_flags |= EF_TIMEOUT;
	}
	else
		q->q_flags |= QQUEUEUP;
}
/*
**  DOFORK -- do a fork, retrying a couple of times on failure.
**
**	This MUST be a macro, since after a vfork we are running
**	two processes on the same stack!!!
**
**	Parameters:
**		none.
**
**	Returns:
**		From a macro???  You've got to be kidding!
**
**	Side Effects:
**		Modifies the ==> LOCAL <== variable 'pid', leaving:
**			pid of child in parent, zero in child.
**			-1 on unrecoverable error.
**
**	Notes:
**		I'm awfully sorry this looks so awful.  That's
**		vfork for you.....
*/

# define NFORKTRIES	5
# ifdef VMUNIX
# 	define XFORK	vfork
#  else /* ! VMUNIX */
# 	define XFORK	fork
#  endif /* VMUNIX */

# define DOFORK(fORKfN) \
{\
	register int i;\
\
	for (i = NFORKTRIES; --i >= 0; )\
	{\
		pid = fORKfN();\
		if (pid >= 0)\
			break;\
		if (i > 0)\
			sleep((unsigned) NFORKTRIES - i);\
	}\
}
/*
**  DOFORK -- simple fork interface to DOFORK.
**
**	Parameters:
**		none.
**
**	Returns:
**		pid of child in parent.
**		zero in child.
**		-1 on error.
**
**	Side Effects:
**		returns twice, once in parent and once in child.
*/

dofork()
{
	register int pid;

	DOFORK(fork);

	return (pid);
}
/*
**  SENDOFF -- send off call to mailer & collect response.
**
**	Parameters:
**		e -- the envelope to mail.
**		m -- mailer descriptor.
**		pvp -- parameter vector to send to it.
**		ctladdr -- an address pointer controlling the
**			user/groupid etc. of the mailer.
**
**	Returns:
**		exit status of mailer.
**
**	Side Effects:
**		none.
*/
static
sendoff(e, m, pvp, ctladdr)
	register ENVELOPE *e;
	MAILER *m;
	char **pvp;
	ADDRESS *ctladdr;
{
	auto FILE *mfile;
	auto FILE *rfile;
	register int i;
	int pid;

	/*
	**  Create connection to mailer.
	*/

	pid = openmailer(m, pvp, ctladdr, FALSE, &mfile, &rfile, e);
	if (pid < 0)
		return (ExitStat);

	/*
	**  Format and send message.
	*/

	putfromline(mfile, m, e);
	(*e->e_puthdr)(mfile, m, e);
	putline("\n", mfile, m);
	(*e->e_putbody)(mfile, m, e);
	(void) fclose(mfile);
	if (rfile != NULL)
		(void) fclose(rfile);

	i = endmailer(pid, pvp[0]);

	/* arrange a return receipt if requested */
	if (e->e_receiptto != NULL && bitnset(M_LOCAL, m->m_flags))
	{
		e->e_flags |= EF_SENDRECEIPT;
		/* do we want to send back more info? */
	}

	return (i);
}
/*
**  ENDMAILER -- Wait for mailer to terminate.
**
**	We should never get fatal errors (e.g., segmentation
**	violation), so we report those specially.  For other
**	errors, we choose a status message (into statmsg),
**	and if it represents an error, we print it.
**
**	Parameters:
**		pid -- pid of mailer.
**		name -- name of mailer (for error messages).
**
**	Returns:
**		exit code of mailer.
**
**	Side Effects:
**		none.
*/

endmailer(pid, name)
	int pid;
	char *name;
{
	int st;

	/* in the IPC case there is nothing to wait for */
	if (pid == 0)
		return (EX_OK);

	/* wait for the mailer process to die and collect status */
	st = waitfor(pid);
	if (st == -1)
	{
		syserr("endmailer %s: wait", name);
		return (EX_SOFTWARE);
	}

	/* see if it died a horrid death */
	if ((st & 0377) != 0)
	{
		syserr("mailer %s died with signal %d", name, st & 0377);
		ExitStat = EX_TEMPFAIL;
		return (EX_TEMPFAIL);
	}

	/* normal death -- return status */
	st = (st >> 8) & 0377;
	return (st);
}
/*
**  OPENMAILER -- open connection to mailer.
**
**	Parameters:
**		m -- mailer descriptor.
**		pvp -- parameter vector to pass to mailer.
**		ctladdr -- controlling address for user.
**		clever -- create a full duplex connection.
**		pmfile -- pointer to mfile (to mailer) connection.
**		prfile -- pointer to rfile (from mailer) connection.
**		e -- the current envelope.
**
**	Returns:
**		pid of mailer ( > 0 ).
**		-1 on error.
**		zero on an IPC connection.
**
**	Side Effects:
**		creates a mailer in a subprocess.
*/

openmailer(m, pvp, ctladdr, clever, pmfile, prfile, e)
	MAILER *m;
	char **pvp;
	ADDRESS *ctladdr;
	bool clever;
	FILE **pmfile;
	FILE **prfile;
	ENVELOPE *e;
{
	int pid;
	int mpvect[2];
	int rpvect[2];
	FILE *mfile = NULL;
	FILE *rfile = NULL;
	extern FILE *fdopen();
	char *curhost;

# ifdef DEBUG
	if (tTd(11, 1))
	{
		printf("openmailer:");
		printav(pvp);
	}
#  endif /* DEBUG */
	errno = 0;

	CurHostName = m->m_mailer;

	/*
	**  Deal with the special case of mail handled through an IPC
	**  connection.
	**	In this case we don't actually fork.  We must be
	**	running SMTP for this to work.  We will return a
	**	zero pid to indicate that we are running IPC.
	**  We also handle a debug version that just talks to stdin/out.
	*/

# ifdef DEBUG
	/* check for Local Person Communication -- not for mortals!!! */
	if (strcmp(m->m_mailer, "[LPC]") == 0)
	{
		*pmfile = stdout;
		*prfile = stdin;
		return (0);
	}
#  endif /* DEBUG */

	if (strcmp(m->m_mailer, "[IPC]") == 0)
	{
# ifdef HOSTINFO
		register STAB *st;
		extern STAB *stab();
#  endif /* HOSTINFO */
# ifdef DAEMON
		register int i;
		register u_short port;

		if (pvp[0] == NULL || pvp[1] == NULL || pvp[1][0] == '\0')
		{
			syserr("null host name for %s mailer", m->m_mailer);
			ExitStat = EX_CONFIG;
			return (-1);
		}

		CurHostName = pvp[1];
		curhost = hostsignature(m, pvp[1], e);

		if (curhost == NULL || curhost[0] == '\0')
		{
			syserr("null host signature for %s", pvp[1]);
			ExitStat = EX_CONFIG;
			return (-1);
		}

		if (!clever)
		{
			syserr("554 non-clever IPC");
			ExitStat = EX_CONFIG;
			return (-1);
		}
		if (pvp[2] != NULL)
			port = atoi(pvp[2]);
		else
			port = 0;

		while (*curhost != '\0')
		{
			register char *p;
			static char hostbuf[MAXNAME];

			/* pull the next host from the signature */
			p = strchr(curhost, ':');
			if (p == NULL)
				p = &curhost[strlen(curhost)];
			if (p == curhost)
			{
				syserr("deliver: null host name in signature");
				curhost++;
				continue;
			}
			strncpy(hostbuf, curhost, p - curhost);
			hostbuf[p - curhost] = '\0';
			if (*p != '\0')
				p++;
			curhost = p;

			/* see if we already know that this host is fried */
			CurHostName = hostbuf;
# 	ifdef HOSTINFO
                        st = stab(hostbuf, ST_HOST, ST_FIND);
                        if (st == NULL || st->s_hostinfo.ho_exitstat == EX_OK) {
				message("Connecting to %s (%s)...",
					hostbuf, m->m_name);
                                i = makeconnection(hostbuf, port, pmfile, prfile);
                        }
                        else
                        {
                                i = st->s_hostinfo.ho_exitstat;
                                errno = st->s_hostinfo.ho_errno;
                        }
# 	 else /* ! HOSTINFO */
                        i = makeconnection(hostbuf, port, pmfile, prfile);
# 	 endif /* HOSTINFO */
			if (i != EX_OK)
			{
# 	ifdef HOSTINFO
                                /* enter status of this host */
                                if (st == NULL)
					st = stab(hostbuf, ST_HOST, ST_ENTER);
                                st->s_hostinfo.ho_exitstat = i;
                                st->s_hostinfo.ho_errno = errno;
				if (st->s_hostinfo.ho_name == NULL)
					st->s_hostinfo.ho_name = newstr(hostbuf);
# 	 endif /* HOSTINFO */
				ExitStat = i;
				continue;
			}
			else
				return (0);
		}
		return (-1);
#  else /* ! DAEMON */
		syserr("openmailer: no IPC");
		return (-1);
#  endif /* DAEMON */
	}

	/* create a pipe to shove the mail through */
	if (pipe(mpvect) < 0)
	{
		syserr("openmailer: pipe (to mailer)");
		return (-1);
	}

# ifdef SMTP
	/* if this mailer speaks smtp, create a return pipe */
	if (clever && pipe(rpvect) < 0)
	{
		syserr("openmailer: pipe (from mailer)");
		(void) close(mpvect[0]);
		(void) close(mpvect[1]);
		return (-1);
	}
#  endif /* SMTP */

	/*
	**  Create a message with the mailer we will
	**  exec and its arguments.
	*/
	if (Verbose)
	{
		char **p = pvp;
		char buf[MAXLINE];

		strcpy(buf, m->m_mailer);
		p++;
		while (*p != NULL)
		{
			strcat(buf, " ");
			strcat(buf, *p++);
		}
		message("Executing \"%s\"", buf);
	}

	/*
	**  Actually fork the mailer process.
	**	DOFORK is clever about retrying.
	**
	**	Dispose of SIGCHLD signal catchers that may be laying
	**	around so that endmail will get it.
	*/

	if (e->e_xfp != NULL)
		(void) fflush(e->e_xfp);		/* for debugging */
	(void) fflush(stdout);
	(void) signal(SIGCLD, SIG_DFL);

	DOFORK(XFORK);

	/* pid is set by DOFORK */
	if (pid < 0)
	{
		/* failure */
		syserr("openmailer: Cannot fork");
		(void) close(mpvect[0]);
		(void) close(mpvect[1]);
# ifdef SMTP
		if (clever)
		{
			(void) close(rpvect[0]);
			(void) close(rpvect[1]);
		}
#  endif /* SMTP */
		ExitStat = EX_OSERR;
		return (-1);
	}
	else if (pid == 0)
	{
		int i;
		extern int DtableSize;

		/* child -- set up input & exec mailer */
		/* make diagnostic output be standard output */
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGTERM, SIG_DFL);

		/* arrange to filter standard & diag output of command */
		if (clever)
		{
			(void) close(rpvect[0]);
			(void) close(1);
			(void) dup(rpvect[1]);
			(void) close(rpvect[1]);
		}
		else if (OpMode == MD_SMTP || HoldErrs)
		{
			/* put mailer output in transcript */
			(void) close(1);
			(void) dup(fileno(e->e_xfp));
		}
		(void) close(2);
		(void) dup(1);

		/* arrange to get standard input */
		(void) close(mpvect[1]);
		(void) close(0);
		if (dup(mpvect[0]) < 0)
		{
			syserr("Cannot dup to zero!");
			_exit(EX_OSERR);
		}
		(void) close(mpvect[0]);
		if (!bitnset(M_RESTR, m->m_flags))
		{
			if (ctladdr == NULL || ctladdr->q_uid == 0)
			{
				(void) setgid(DefGid);
				(void) initgroups(DefUser, DefGid);
				(void) setuid(DefUid);
			}
			else
			{
				(void) setgid(ctladdr->q_gid);
				(void) initgroups(ctladdr->q_ruser?
					ctladdr->q_ruser: ctladdr->q_user,
					ctladdr->q_gid);
				(void) setuid(ctladdr->q_uid);
			}
		}

		/* arrange for all the files to be closed */
		for (i = 3; i < getnumfds(); i++) {
			register int j;
			if ((j = fcntl(i, F_GETFD, 0)) != -1)
				(void)fcntl(i, F_SETFD, j|1);
		}

		/* try to execute the mailer */
		execve(m->m_mailer, pvp, UserEnviron);
		syserr("Cannot exec %s", m->m_mailer);
		if (m == LocalMailer || errno == EIO || errno == EAGAIN ||
		    errno == ENOMEM)
			_exit(EX_TEMPFAIL);
		else
			_exit(EX_UNAVAILABLE);
	}

	/*
	**  Set up return value.
	*/

	(void) close(mpvect[0]);
	mfile = fdopen(mpvect[1], "w");
	if (clever)
	{
		(void) close(rpvect[1]);
		rfile = fdopen(rpvect[0], "r");
	} else
		rfile = NULL;

	*pmfile = mfile;
	*prfile = rfile;

	return (pid);
}
/*
**  GIVERESPONSE -- Interpret an error response from a mailer
**
**	Parameters:
**		stat -- the status code from the mailer (high byte
**			only; core dumps must have been taken care of
**			already).
**		m -- the mailer descriptor for this mailer.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Errors may be incremented.
**		ExitStat may be set.
*/

void giveresponse(stat, m, e)
	int stat;
	register MAILER *m;
	ENVELOPE *e;
{
	register char *statmsg = NULL;
	extern char *SysExMsg[];
	register int i;
	extern int N_SysEx;
# ifdef NAMED_BIND
	extern int h_errno;
# endif	/* NAMED_BIND */
	char buf[MAXLINE];
	char logbuf[MAXLINE];
	extern char *errstring();

# ifdef lint
	if (m == NULL)
		return;
#  endif /* lint */

	/*
	**  Compute status message from code.
	*/

	i = stat - EX__BASE;
	if (stat == 0) {
		sprintf(buf, "250 Sent");
	}
	else if (i < 0 || i >= N_SysEx)
	{
		(void) sprintf(buf, "554 unknown mailer error %d", stat);
		stat = EX_UNAVAILABLE;
	}
	else if (temporary(stat))
	{
		if (stat == EX_TEMPFAIL)
		{
			(void) strcpy(buf, SysExMsg[i]);
		}
		else
		{
			sprintf(buf, "%.3s Deferred: %s", SysExMsg[i], &SysExMsg[i][4]);
		}
# ifdef NAMED_BIND
		if (h_errno == TRY_AGAIN)
		{
			extern char *errstring();

			statmsg = errstring(h_errno+MAX_ERRNO);
		}
		else
# endif	/* NAMED_BIND */
			if (errno != 0)
			{
				extern char *errstring();

				statmsg = errstring(errno);
			}
			else
			{
# ifdef SMTP
				extern char SmtpError[];

				statmsg = SmtpError;
# else /* SMTP */
				statmsg = NULL;
# endif /* SMTP */
			}
		if (statmsg != NULL && statmsg[0] != '\0')
		{
			(void) strcat(buf, ": ");
			(void) strcat(buf, statmsg);
		}
	}
	else
	{
		sprintf(buf, SysExMsg[i]);
	}
	statmsg = buf;
	strcpy(logbuf, buf);

# ifdef LOG
	if (LogLevel > 9)
	{
		strcat(logbuf, ", mailer=");
		strcat(logbuf, m->m_name);
		if (strcmp(m->m_mailer, "[IPC]") == 0)
		{
			if (CurMxHost != NULL)
			{
				(void) strcat(logbuf, ", MX host=");
				(void) strcat(logbuf, CurMxHost);
			}
			if (ServerAddress != NULL)
			{
				(void) strcat(logbuf, ", address=[");
				(void) strcat(logbuf, ServerAddress);
				(void) strcat(logbuf, "]");
			}
		}
	}
#  endif /* LOG */

	/*
	**  Print the message as appropriate
	*/

	if (stat == EX_OK)
	{
	   	message(&statmsg[4]);
	}
	else if (temporary(stat))
	{
		message(statmsg, &statmsg[4]);
	}
	else
	{
		Errors++;
		usrerr(statmsg);
	}

	/*
	**  Final cleanup.
	**	Log a record of the transaction.  Compute the new
	**	ExitStat -- if we already had an error, stick with
	**	that.
	*/

	if (e->e_sendmode != SM_VERIFY &&
	    LogLevel > ((stat == 0 || temporary(stat)) ? 3 : 2))
		logdelivery(&logbuf[4], e);

	if (! temporary(stat))
		setstat(stat);
	if (stat != EX_OK)
	{
		if (e->e_message != NULL)
			free(e->e_message);
		e->e_message = newstr(&statmsg[4]);
	}
	errno = 0;
# ifdef NAMED_BIND
	h_errno = 0;
# endif	/* NAMED_BIND */
}
/*
**  LOGDELIVERY -- log the delivery in the system log
**
**	Parameters:
**		stat -- the message to print for the status
**		e -- the current envelope.
**
**	Returns:
**		none
**
**	Side Effects:
**		none
*/

void logdelivery(stat, e)
	char *stat;
	register ENVELOPE *e;
{
	extern char *pintvl();

# ifdef LOG
	syslog(LOG_INFO, "%s: to=%s, delay=%s, stat=%s", e->e_id,
	       e->e_to, pintvl(curtime() - e->e_ctime, TRUE), stat);
#  endif /* LOG */
}
/*
**  PUTFROMLINE -- output a UNIX-style from line (or whatever)
**
**	This can be made an arbitrary message separator by changing $l
**
**	One of the ugliest hacks seen by human eyes is contained herein:
**	UUCP wants those stupid "remote from <host>" lines.  Why oh why
**	does a well-meaning programmer such as myself have to deal with
**	this kind of antique garbage????
**
**	Parameters:
**		fp -- the file to output to.
**		m -- the mailer describing this entry.
**
**	Returns:
**		none
**
**	Side Effects:
**		outputs some text to fp.
*/

void putfromline(fp, m, e)
	register FILE *fp;
	register MAILER *m;
	ENVELOPE *e;
{
	char *template = "\001l\n";
	char buf[MAXLINE];

	if (bitnset(M_NHDR, m->m_flags))
		return;

# ifdef UGLYUUCP
	if (bitnset(M_UGLYUUCP, m->m_flags))
	{
		char *bang;
		char xbuf[MAXLINE];

		expand("\001g", buf, &buf[sizeof buf - 1], e);
		bang = strchr(buf, '!');
		if (bang == NULL)
		{
			if (bitnset(M_NO_BANGS_OK, m->m_flags))
			{
				if (LogLevel > 0)
					syslog("No ! in UUCP! (%s)", buf);
				(void) sprintf(xbuf, "From %s  \001d remote from <unknown>\n", buf);
				template = xbuf;
			}
			else
				syserr("No ! in UUCP! (%s)", buf);
		}
		else
		{
			*bang++ = '\0';
			(void) sprintf(xbuf, "From %s  \001d remote from %s\n", bang, buf);
			template = xbuf;
		}
	}
# endif /* UGLYUUCP */
	expand(template, buf, &buf[sizeof buf - 1], e);
	putline(buf, fp, m);
}
/*
**  PUTBODY -- put the body of a message.
**
**	Parameters:
**		fp -- file to output onto.
**		m -- a mailer descriptor to control output format.
**		e -- the envelope to put out.
**
**	Returns:
**		none.
**
**	Side Effects:
**		The message is written onto fp.
*/

putbody(fp, m, e)
	FILE *fp;
	MAILER *m;
	register ENVELOPE *e;
{
	char buf[MAXLINE];

	/*
	**  Output the body of the message
	*/

	if (e->e_dfp == NULL)
	{
		if (e->e_df != NULL)
		{
			e->e_dfp = fopen(e->e_df, "r");
			if (e->e_dfp == NULL)
				syserr("putbody: Cannot open %s for %s from %s",
				e->e_df, e->e_to, e->e_from);
		}
		else
			putline("<<< No Message Collected >>>", fp, m);
	}
	if (e->e_dfp != NULL)
	{
		rewind(e->e_dfp);
		while (!ferror(fp) && fgets(buf, sizeof buf, e->e_dfp) != NULL)
		{
			if (buf[0] == 'F' && bitnset(M_ESCFROM, m->m_flags) &&
			    strncmp(buf, "From ", 5) == 0)
				(void) putc('>', fp);
			putline(buf, fp, m);
		}

		if (ferror(e->e_dfp))
		{
			syserr("putbody: %s read error", e->e_df);
			ExitStat = EX_IOERR;
		}
	}

	(void) fflush(fp);
	if (ferror(fp) && errno != EPIPE && errno != ESPIPE)
	{
		syserr("putbody: write error");
		ExitStat = EX_IOERR;
	}
	errno = 0;
}
/*
**  MAILFILE -- Send a message to a file.
**
**	If the file has the setuid/setgid bits set, but NO execute
**	bits, sendmail will try to become the owner of that file
**	rather than the real user.  Obviously, this only works if
**	sendmail runs as root.
**
**	This could be done as a subordinate mailer, except that it
**	is used implicitly to save messages in ~/dead.letter.  We
**	view this as being sufficiently important as to include it
**	here.  For example, if the system is dying, we shouldn't have
**	to create another process plus some pipes to save the message.
**
**	Parameters:
**		filename -- the name of the file to send to.
**		ctladdr -- the controlling address header -- includes
**			the userid/groupid to be when sending.
**
**	Returns:
**		The exit code associated with the operation.
**
**	Side Effects:
**		none.
*/

mailfile(filename, ctladdr, e)
	char *filename;
	ADDRESS *ctladdr;
	register ENVELOPE *e;
{
	register FILE *f;
	register int pid;

	/*
	**  Fork so we can change permissions here.
	**	Note that we MUST use fork, not vfork, because of
	**	the complications of calling subroutines, etc.
	*/

	DOFORK(fork);

	if (pid < 0)
		return (EX_OSERR);
	else if (pid == 0)
	{
		/* child -- actually write to file */
		struct stat stb;

		(void) signal(SIGINT, SIG_DFL);
		(void) signal(SIGHUP, SIG_DFL);
		(void) signal(SIGTERM, SIG_DFL);
		(void) umask(OldUmask);

		if (stat(filename, &stb) < 0)
		{
			errno = 0;
			stb.st_mode = 0666;
		}
		if (bitset(0111, stb.st_mode))
			exit(EX_CANTCREAT);
		if (ctladdr == NULL)
			ctladdr = &e->e_from;
		/* we have to open the dfile BEFORE setuid */
		if (e->e_dfp == NULL && e->e_df != NULL)
		{
			e->e_dfp = fopen(e->e_df, "r");
			if (e->e_dfp == NULL)
			{
				syserr("mailfile: Cannot open %s for %s from %s",
				        e->e_df, e->e_to, e->e_from);
			}
		}

		if (!bitset(S_ISGID, stb.st_mode) || setgid(stb.st_gid) < 0)
		{
			if (ctladdr->q_uid == 0) {
				(void) setgid(DefGid);
				(void) initgroups(DefUser, DefGid);
			}
			else
			{
				(void) setgid(ctladdr->q_gid);
				(void) initgroups(ctladdr->q_ruser?
					ctladdr->q_ruser: ctladdr->q_user,
					ctladdr->q_gid);
			}
		}
		if (!bitset(S_ISUID, stb.st_mode) || setuid(stb.st_uid) < 0)
		{
			if (ctladdr == NULL || ctladdr->q_uid == 0)
				(void) setuid(DefUid);
			else
				(void) setuid(ctladdr->q_uid);
		}
		f = dfopen(filename, "a");
		if (f == NULL)
			exit(EX_CANTCREAT);

		putfromline(f, ProgMailer, e);
		(*e->e_puthdr)(f, ProgMailer, e);
		putline("\n", f, ProgMailer);
		(*e->e_putbody)(f, ProgMailer, e);
		putline("\n", f, ProgMailer);
		(void) fclose(f);
		(void) fflush(stdout);

		/* reset ISUID & ISGID bits for paranoid systems */
		(void) chmod(filename, (int) stb.st_mode);
		exit(EX_OK);
		/*NOTREACHED*/
	}
	else
	{
		/* parent -- wait for exit status */
		int st;

		st = waitfor(pid);
		if ((st & 0377) != 0)
			return (EX_UNAVAILABLE);
		else
			return ((st >> 8) & 0377);
		/*NOTREACHED*/
	}
}
/*
**  HOSTSIGNATURE -- return the "signature" for a host.
**
**	The signature describes how we are going to send this -- it
**	can be just the hostname (for non-Internet hosts) or can be
**	an ordered list of MX hosts.
**
**	Parameters:
**		m -- the mailer describing this host.
**		host -- the host name.
**		e -- the current envelope.
**
**	Returns:
**		The signature for this host.
**
**	Side Effects:
**		Can tweak the symbol table.
*/

char *
hostsignature(m, host, e)
	register MAILER *m;
	char *host;
	ENVELOPE *e;
{
	register char *p;
	register STAB *s;
	int i;
	int len;
# ifdef NAMED_BIND
	int nmx;
	auto int rcode;
	char *hp;
	char *endp;
	char *mxhosts[MAXMXHOSTS + 1];
# endif	/* NAMED_BIND */

	/*
	**  Check to see if this uses IPC -- if not, it can't have MX records.
	*/

	p = m->m_mailer;
	if (strcmp(p, "[IPC]") != 0 && strcmp(p, "[TCP]") != 0)
	{
		/* just an ordinary mailer */
		return host;
	}

	/*
	**  Look it up in the symbol table.
	*/

	s = stab(host, ST_HOSTSIG, ST_ENTER);
	if (s->s_hostsig != NULL)
		return s->s_hostsig;

	/*
	**  Not already there -- create a signature.
	*/

# ifdef NAMED_BIND
	for (hp = host; hp != NULL; hp = endp)
	{
		endp = strchr(hp, ':');
		if (endp != NULL)
			*endp = '\0';

		nmx = getmxrr(hp, mxhosts, &rcode, e);

		if (nmx <= 0)
		{
			/* return the original host name as the signature */
			nmx = 1;
			mxhosts[0] = hp;
		}

		len = 0;
		for (i = 0; i < nmx; i++)
			len += strlen(mxhosts[i]) + 1;
		if (s->s_hostsig != NULL)
			len += strlen(s->s_hostsig) + 1;
		p = xalloc(len);
		if (s->s_hostsig != NULL)
		{
			(void) strcpy(p, s->s_hostsig);
			free(s->s_hostsig);
			s->s_hostsig = p;
			p += strlen(p);
			*p++ = ':';
		}
		else
			s->s_hostsig = p;
		for (i = 0; i < nmx; i++)
		{
			if (i != 0)
				*p++ = ':';
			strcpy(p, mxhosts[i]);
			p += strlen(p);
		}
		if (endp != NULL)
			*endp++ = ':';
	}
	makelower(s->s_hostsig);
# else	/* ! NAMED_BIND */
	/* not using BIND -- the signature is just the host name */
	s->s_hostsig = host;
# endif	/* NAMED_BIND */
	if (tTd(17, 1))
		printf("hostsignature(%s) = %s\n", host, s->s_hostsig);
	return s->s_hostsig;
}
/*
**  SENDALL -- actually send all the messages.
**
**	Parameters:
**		e -- the envelope to send.
**		mode -- the delivery mode to use.  If SM_DEFAULT, use
**			the current e->e_sendmode.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Scans the send lists and sends everything it finds.
**		Delivers any appropriate error messages.
**		If we are running in a non-interactive mode, takes the
**			appropriate action.
*/

void sendall(e, mode)
	ENVELOPE *e;
	char mode;
{
	register ADDRESS *q;
	bool oldverbose;
	int pid;
	FILE *lockfp = NULL, *queueup();
	char buf[MAXLINE];

	/* determine actual delivery mode */
	CurrentLA = getla();
	if (mode == SM_DEFAULT)
	{
		mode = e->e_sendmode;
		if (mode != SM_VERIFY &&
		    shouldqueue(e->e_msgpriority))
		{
# ifdef LOG
			/*
			**	log that we are queuing because of load
			*/
			if (LogLevel > 11)
				syslog(LOG_DEBUG, "%s: load average = %d, queuing message",
				    e->e_id, CurrentLA);
#  endif /* LOG */
			mode = SM_QUEUE;
		}
	}

# ifdef DEBUG
	if (tTd(13, 1))
	{
		printf("\nSENDALL: mode %c, sendqueue:\n", mode);
		printaddr(e->e_sendqueue, TRUE);
	}
#  endif /* DEBUG */

	/*
	**  Do any preprocessing necessary for the mode we are running.
	**	Check to make sure the hop count is reasonable.
	**	Delete sends to the sender in mailing lists.
	*/

	CurEnv = e;

	if (e->e_hopcount > MAXHOP)
	{
		errno = 0;
		syserr("Too many hops %d (%d max): from %s, to %s",
			e->e_hopcount, MAXHOP, e->e_from, e->e_to);
		return;
	}

	if (!MeToo)
	{
		extern ADDRESS *recipient();

		e->e_from.q_flags |= QDONTSEND;
		(void) recipient(&e->e_from, &e->e_sendqueue, e);
	}

# ifdef QUEUE
	if ((mode == SM_QUEUE || mode == SM_FORK ||
	     (mode != SM_VERIFY && SuperSafe)) &&
	    !bitset(EF_INQUEUE, e->e_flags))
		lockfp = queueup(e, TRUE, mode == SM_QUEUE, mode != SM_FORK);
#  endif /* QUEUE */

	oldverbose = Verbose;
	switch (mode)
	{
	  case SM_VERIFY:
		Verbose = TRUE;
		break;

	  case SM_QUEUE:
		e->e_flags |= EF_INQUEUE|EF_KEEPQUEUE;
		return;

	  case SM_FORK:
		if (e->e_xfp != NULL)
			(void) fflush(e->e_xfp);
		pid = fork();
		if (pid > 0)
		{
			/* ignoring SIGCLD obviates zombies */
			signal(SIGCLD, SIG_IGN);
			/* be sure we leave the temp files to our child */
			e->e_id = e->e_df = NULL;
			if (lockfp != NULL)
				(void) fclose(lockfp);
			e->e_lockfp = lockfp;
			return;
		}
		if (lockfp != NULL)
		{
			/*
			** We need this code because SysV locks are gone
			** after the first close, but BSD flock assumes
			** that the child inherits the lock.  Here we'll
			** get the temp file lock and do the rename that
			** was skipped in queueup, therby emulating
			** the lock inheritance.
			*/

			register char *qf;
			char tf[MAXLINE];

			(void) strcpy(tf, queuename(e, 't'));
			qf = queuename(e, 'q');

			/* lock the temp file and rename it */
			if (lockf(fileno(lockfp), F_LOCK, 0) < 0)
				syserr("Cannot lockf(%s)", tf);

    			if (rename(tf, qf) < 0)
				syserr("Cannot rename(%s, %s), df=%s",
				       tf, qf, e->e_df);

			/* close and unlock old (locked) qf */
			if (e->e_lockfp != NULL)
				(void) fclose(e->e_lockfp);
			e->e_lockfp = lockfp;
		}
		if (pid < 0)
		{
			mode = SM_DELIVER;
			break;
		}

		/* be sure we are immune from the terminal */
		disconnect(FALSE, e);

		break;
	}

	/*
	**  Reset our count of attempted deliveries.
	*/
	Deliveries = 0;
	e->e_nsent = 0;

	/*
	**  Run through the list and send everything.
	*/

	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		if (mode == SM_VERIFY)
		{
			e->e_to = q->q_paddr;
			if (!bitset(QDONTSEND|QBADADDR, q->q_flags))
# ifdef LOG
			    if (LogLevel > 9)
			    {
				(void) sprintf (buf,
				    "deliverable, mailer=%s, host=%s, user=%s",
					    q->q_mailer->m_name, q->q_host,
					    doublepercent(q->q_user));
				message(buf);
			    }
			    else
#  endif /* LOG */
				message("deliverable");
		}
		else if (!bitset(QDONTSEND|QBADADDR, q->q_flags))
		{
# ifdef QUEUE
			/*
			**  Checkpoint the send list every few addresses
			*/

			if (e->e_nsent >= CheckpointInterval)
			{
				queueup(e, TRUE, FALSE, TRUE);
				e->e_nsent = 0;
			}
# endif /* QUEUE */
			(void) deliver(e, q);
		}

	}
	Verbose = oldverbose;

	/*
	**  Now run through and check for errors.
	*/

	if (mode == SM_VERIFY) {
		if (lockfp != NULL)
			(void) fclose(lockfp);
		return;
	}

	/*
	** Flag silent delivery failures.
	*/
	if (Deliveries == 0 && !bitset(EF_FATALERRS, e->e_flags))
	{
		e->e_flags |= EF_FATALERRS;
		if (e->e_message == NULL)
			e->e_message = newstr("All recipients suppressed");
	}
	if (bitset(EF_RESPONSE, e->e_flags))
		goto done;

	QueueRun = FALSE;
	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		register ADDRESS *qq;

# ifdef DEBUG
		if (tTd(13, 3))
		{
			printf("Checking ");
			printaddr(q, FALSE);
		}
#  endif /* DEBUG */

		/* only send errors if the message failed */
		if (!bitset(QBADADDR, q->q_flags))
			continue;

		/* we have an address that failed -- find the parent */
		for (qq = q; qq != NULL; qq = qq->q_alias)
		{
			char obuf[MAXNAME + 6];
			extern char *aliaslookup();

			/* we can only have owners for local addresses */
			if (!bitnset(M_LOCAL, qq->q_mailer->m_flags))
				continue;

			/* see if the owner list exists */
			(void) strcpy(obuf, "owner-");
			if (strncmp(qq->q_user, "owner-", 6) == 0)
				(void) strcat(obuf, "owner");
			else
				(void) strcat(obuf, qq->q_user);
			makelower(obuf);
			if (aliaslookup(AliasDB, obuf) == NULL)
				continue;

# ifdef DEBUG
			if (tTd(13, 4))
				printf("Errors to %s\n", obuf);
#  endif /* DEBUG */

			/* owner list exists -- add it to the error queue */
			sendtolist(obuf, (ADDRESS *) NULL, &e->e_errorqueue, e);
			e->e_errormode = EM_MAIL;
			break;
		}

		/* if we did not find an owner, send to the sender */
		if (qq == NULL && bitset(QBADADDR, q->q_flags) && e->e_errorqueue == NULL)
			sendtolist(e->e_from.q_paddr, qq, &e->e_errorqueue, e);
	}

done:
	/* this removes the lock on the file */
	if (lockfp != NULL)
		(void) fclose(lockfp);

	if (mode == SM_FORK)
		finis();
}
