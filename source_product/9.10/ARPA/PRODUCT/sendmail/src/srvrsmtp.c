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
static char rcsid[] = "$Header: srvrsmtp.c,v 1.18.109.9 95/02/21 16:08:46 mike Exp $";
# 	ifndef hpux
# 		ifdef SMTP
static char sccsid[] = "@(#)srvrsmtp.c	5.28 (Berkeley) 6/1/90 (with SMTP)";
# 		else	/* ! SMTP */
static char sccsid[] = "@(#)srvrsmtp.c	5.28 (Berkeley) 6/1/90 (without SMTP)";
# 		endif	/* SMTP */
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: srvrsmtp.o $Revision: 1.18.109.9 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include "sendmail.h"
# include <errno.h>
# include <signal.h>

static void printvrfyaddr();
static void help();

# ifdef SMTP
# 	include <sys/vfs.h>
# 	include <values.h>

/*
**  SMTP -- run the SMTP protocol.
**
**	Parameters:
**		none.
**
**	Returns:
**		never.
**
**	Side Effects:
**		Reads commands from the input channel and processes
**			them.
*/

struct cmd
{
	char	*cmdname;	/* command name */
	int	cmdcode;	/* internal code, see below */
};

/* values for cmdcode */
# 	define CMDERROR	0	/* bad command */
# 	define CMDMAIL	1	/* mail -- designate sender */
# 	define CMDRCPT	2	/* rcpt -- designate recipient */
# 	define CMDDATA	3	/* data -- send message text */
# 	define CMDRSET	4	/* rset -- reset state */
# 	define CMDVRFY	5	/* vrfy -- verify address */
# 	define CMDEXPN	6	/* expn -- expand address */
# 	define CMDNOOP	7	/* noop -- do nothing */
# 	define CMDQUIT	8	/* quit -- close connection and die */
# 	define CMDHELO	9	/* helo -- be polite */
# 	define CMDHELP	10	/* help -- give usage info */
# 	define CMDEHLO	11	/* ehlo -- ESMTP version of helo */
/* non-standard commands */
# 	define CMDONEX	16	/* onex -- sending one transaction only */
# 	define CMDVERB	17	/* verb -- go into verbose mode */
# 	define CMDQUED	18	/* qued -- queue it for later delivery */
/* debugging-only commands, only enabled if SMTPDEBUG is defined */
# 	define CMDDBGQSHOW	24	/* showq -- show send queue */
# 	define CMDDBGDEBUG	25	/* debug -- set debug mode */

static struct cmd	CmdTab[] =
{
	"mail",		CMDMAIL,
	"rcpt",		CMDRCPT,
	"data",		CMDDATA,
	"rset",		CMDRSET,
	"vrfy",		CMDVRFY,
	"expn",		CMDEXPN,
	"help",		CMDHELP,
	"noop",		CMDNOOP,
	"quit",		CMDQUIT,
	"helo",		CMDHELO,
	"ehlo",		CMDEHLO,
	"verb",		CMDVERB,
	"qued",		CMDQUED,
	"onex",		CMDONEX,
	/*
	 * remaining commands are here only
	 * to trap and log attempts to use them
	 */
	"showq",	CMDDBGQSHOW,
	"debug",	CMDDBGDEBUG,
	NULL,		CMDERROR,
};

bool	InChild = FALSE;		/* true if running in a subprocess */
bool	OneXact = FALSE;		/* one xaction only this run */
u_long	SizeD = 0;			/* message size declaration */

# 	define EX_QUIT		22		/* special code for QUIT command */

void smtp(e)
	register ENVELOPE *e;
{
	register char *p;
	register struct cmd *c;
	char *cmd;
	extern char *skipword();
	auto ADDRESS *vrfyqueue;
	ADDRESS *a;
	bool gotmail;			/* mail command received */
	bool gothello;			/* helo command received */
	bool vrfy;			/* set if this is a vrfy command */
	char *protocol;			/* sending protocol */
	char *sendinghost;		/* sending hostname */
	char *id;
	int nrcpts;			/* number of RCPT commands */
	bool doublequeue;
	char inp[MAXLINE];
	char cmdbuf[MAXLINE];
	extern char Version[];
	extern ENVELOPE BlankEnvelope;
	struct statfs fs;		/* used to determine available space */

	/*
	**      unbuffer InChannel if it is a file or a pipe; otherwise,
	**	when the child exits, the parent's copy of InChannel
	**	will contain input already read by the child, which the
	**	parent will not want to reread.
	*/
	if (!isatty(fileno(InChannel)) && !isasocket(fileno(InChannel)))
	{
# 	ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG,"unbuffering input");
# 	 endif /* LOG */
		setbuf(InChannel, NULL);
	}
	else
	/*
	**	InChannel is a socket (or a tty): use big read buffer
	*/
	{
		setvbuf(InChannel, NULL, _IOFBF, BIGBUFSIZ);
	}

	if (OutChannel != stdout)
	{
		/* arrange for debugging output to go to remote host */
		(void) close(1);
		(void) dup(fileno(OutChannel));
	}
	settime(e);
	if (RealHostName != NULL)
	{
		CurHostName = RealHostName;
		setproctitle("srvrsmtp %s", CurHostName);
	}
	else
	{
		/*
		**	if RealHostName wasn't set in getrequests,
		**	this must be us.
		*/
		CurHostName = MyHostName;
	}
	expand("\001e", inp, &inp[sizeof inp], e);
	message("220-%s", inp);
	message("220 ESMTP spoken here");
	SmtpPhase = "startup";
	protocol = NULL;
	sendinghost = NULL;
	gothello = FALSE;
	gotmail = FALSE;
	for (;;)
	{
		/* arrange for backout */
		if (setjmp(TopFrame) > 0 && InChild)
			finis();
		QuickAbort = FALSE;
		HoldErrs = FALSE;
		FileName = NULL;
		e->e_flags &= ~(EF_VRFYONLY);

		/* setup for the read */
		e->e_to = NULL;
		Errors = 0;
		(void) fflush(stdout);

		/* read the input line */
		p = sfgets(inp, sizeof inp, InChannel);

		/* handle errors */
		if (p == NULL)
		{
			/* end of file, just die */
			message("421 %s Lost input channel from %s",
				MyHostName, CurHostName);
# 	ifdef LOG
			if (LogLevel > (gotmail ? 1 : 19))
				syslog(LOG_NOTICE, "lost input channel from %s",
					CurHostName);
# 	endif	/* LOG */
			finis();
		}

		/* clean up end of line */
		fixcrlf(inp, TRUE);

		/* echo command to transcript */
		if (e->e_xfp != NULL)
			fprintf(e->e_xfp, "<<< %s\n", inp);

		/* break off command */
		for (p = inp; isascii(*p) && isspace(*p); p++)
			continue;
		cmd = p;
		for (cmd = cmdbuf; *p != '\0' && !isspace(*p); )
			*cmd++ = *p++;
		*cmd = '\0';

		/* throw away leading whitespace */
		while (isascii(*p) && isspace(*p))
			p++;

		/* decode command */
		for (c = CmdTab; c->cmdname != NULL; c++)
		{
			if (!strcasecmp(c->cmdname, cmdbuf))
				break;
		}

		/* process command */
		switch (c->cmdcode)
		{
		  case CMDHELO:		/* hello -- introduce yourself */
		  case CMDEHLO:		/* extended hello */
			if (c->cmdcode == CMDEHLO)
			{
				protocol = "ESMTP";
				SmtpPhase = "server EHLO";
			}
			else
			{
				protocol = "SMTP";
				SmtpPhase = "server HELO";
			}
			setproctitle("%s: %s", CurHostName, inp);
			if (!strcasecmp(p, MyHostName))
			{
				/*
				 * didn't know about alias,
				 * or connected to an echo server
				 */
				message("553 Local configuration error, hostname not recognized as local");
				break;
			}
			if (RealHostName != NULL && strcasecmp(p, RealHostName))
			{
				char hostbuf[MAXNAME];

				(void) sprintf(hostbuf, "%s (%s)", p, RealHostName);
				sendinghost = newstr(hostbuf);
			}
			else
				sendinghost = newstr(p);
			gothello = TRUE;
			if (c->cmdcode != CMDEHLO)
			{
				/* print old message and be done with it */
				message("250 %s Hello %s, pleased to meet you",
					MyHostName, sendinghost);
				break;
			}
			
			/* print extended message and brag */
			message("250-%s Hello %s, pleased to meet you",
				MyHostName, sendinghost);
			if (HelpFile)
				message("250-HELP");
			if (!bitset(PRIV_NOEXPN, PrivacyFlags))
				message("250-EXPN");
			if (MaxMessageSize > 0)
				message("250-SIZE %ld", MaxMessageSize);
			else if (statfs(QueueDir, &fs) == 0){
				if (fs.f_bavail >= (MAXLONG / fs.f_bsize))
					message("250-SIZE %ld", MAXLONG);
				else
					message("250-SIZE %ld", (u_long)(fs.f_bsize * fs.f_bavail));
			} else
				message("250-SIZE");
			message("250-8BITMIME");
# 	ifdef	SMTPDEBUG
			message("250-XSHQ");	/* SHOWQ */
			message("250-XDBG");	/* DEBUG */
# 	endif	/* SMTPDEBUG */
			message("250-XONE");
			if (!bitset(PRIV_NOEXPN, PrivacyFlags))
				message("250-XVRB");
			message("250 XQUE");
			break;

		  case CMDMAIL:		/* mail -- designate sender */
			SmtpPhase = "server MAIL";

			/* check for validity of this command */
			if (!gothello)
			{
				/* set sending host to our known value */
				if (sendinghost == NULL)
					sendinghost = RealHostName;

				if (bitset(PRIV_NEEDMAILHELO, PrivacyFlags))
				{
					message("503 Polite people say HELO first");
					break;
				}
			}
			if (gotmail)
			{
				message("503 Sender already specified");
				break;
			}
			if (InChild)
			{
				errno = 0;
				syserr("503 Nested MAIL command: MAIL %s", p);
				finis();
			}

			/* fork a subprocess to process this command */
			if (runinchild("SMTP-MAIL", e) > 0)
				break;
			if (protocol == NULL)
				protocol = "SMTP";
			define('r', protocol, e);
			define('s', sendinghost, e);
			initsys(e);
			nrcpts = 0;
			e->e_flags |= EF_LOGSENDER;
			setproctitle("%s %s: %.80s", e->e_id, CurHostName, inp);

			/* child -- go do the processing */
			SizeD = 0;
			{
			    register char *s, *t;

			    if ((s = strrchr(p, '>')) && isspace(*++s))
				for (*s++ = '\0';; s = t){
				    while (isspace(*s))
					*s++;
				    if (!*s)
					break;

				    for (t = s + 1; *t > ' '; t++)
					continue;

				    if (strncasecmp(s, "size=", 5) == 0){
				       (void) sscanf(s += 5, "%ld", &SizeD);
				       continue;
				    }
				    if (strncasecmp(s, "body=8bitmime", 13) == 0){
				       e->e_bodytype = "8BITMIME";
				       continue;
				    } else if (strncasecmp(s, "body=7bit", 9) == 0){
				       e->e_bodytype = "7BIT";
				       continue;
				    }
				}
			}
			p = skipword(p, "from");
			if (p == NULL)
				break;
			setsender(p, e);
			if (Errors == 0)
			{
				if (MaxMessageSize > 0 && SizeD > MaxMessageSize)
				{
					usrerr("552 Message size exceeds fixed maximum message size (%ld)",
					       MaxMessageSize);
					if (InChild)
						finis();
					break;
				}
				if ((SizeD > 0) && ((statfs(QueueDir, &fs) == 0)) &&
				    ((SizeD / fs.f_bsize) >= fs.f_bavail)){
					message("452 Not enough disk space; try again later");
					if (InChild)
						finis();
					break;
				}

				if (e->e_bodytype)
					message("250 Sender and %s ok", e->e_bodytype);
				else
					message("250 Sender ok");
				gotmail = TRUE;
			}
			else if (InChild)
				finis();
			break;

		  case CMDRCPT:		/* rcpt -- designate recipient */
			if (!gotmail)
			{
				usrerr("503 Need MAIL before RCPT");
				break;
			}
			SmtpPhase = "server RCPT";
			setproctitle("%s %s: %s", e->e_id, CurHostName, inp);
			if (setjmp(TopFrame) > 0)
			{
				e->e_flags &= ~EF_FATALERRS;
				break;
			}
			QuickAbort = TRUE;

			if (e->e_sendmode != SM_DELIVER)
				e->e_flags |= EF_VRFYONLY;

			p = skipword(p, "to");
			if (p == NULL)
				break;
			a = parseaddr(p, (ADDRESS *) NULL, 1, '\0', e);
			if (a == NULL)
				break;
			a->q_flags |= QPRIMARY;
			a = recipient(a, &e->e_sendqueue, e);
			if (Errors != 0)
				break;

			if (SizeD && (a->q_mailer) && (a->q_mailer->m_maxsize != 0)
			          && (SizeD > a->q_mailer->m_maxsize)) {
				char size[MAXNAME];

				(void) sprintf(size, "%ld octets maximum for recipient",
					       a->q_mailer->m_maxsize);
				message ("552", size);
				a->q_flags |= QDONTSEND;
				e->e_to = NULL;
				break;
			}

			/* no errors during parsing, but might be a duplicate */
			e->e_to = p;
			if (!bitset(QBADADDR, a->q_flags))
			{
				message("250 Recipient ok%s",
					bitset(QQUEUEUP, a->q_flags) ?
						" (will queue)" : "");
				nrcpts++;
			}
			else
			{
				/* punt -- should keep message in ADDRESS.... */
				message("550 Addressee unknown");
			}
			e->e_to = NULL;
			break;

		  case CMDDATA:		/* data -- text of mail */
			SmtpPhase = "server DATA";
			if (!gotmail)
			{
				message("503 Need MAIL command");
				break;
			}
			else if (nrcpts <= 0)
			{
				message("503 Need RCPT (recipient)");
				break;
			}

			/* check to see if we need to re-expand aliases */
			/* also reset QBADADDR on already-diagnosted addrs */
			doublequeue = FALSE;
			for (a = e->e_sendqueue; a != NULL; a = a->q_next)
			{
				if (bitset(QVERIFIED, a->q_flags))
				{
					/* need to re-expand aliases */
					doublequeue = TRUE;
				}
				if (bitset(QBADADDR, a->q_flags))
				{
					/* make this "go away" */
					a->q_flags |= QDONTSEND;
					a->q_flags &= ~QBADADDR;
				}
			}

			/* collect the text of the message */
			SmtpPhase = "collect";
			setproctitle("%s %s: %s", e->e_id, CurHostName, inp);
			Saw8Bits = FALSE;
			collect(TRUE, doublequeue, e);
			if (Errors != 0)
				break;
			/*
			 * We haven't hit any errors so far, so any after this must be
			 * if we pass this message on to another MTA.  Hold onto those
			 * errors, instead of passing them back to the source MTA.
			 */
			HoldErrs = TRUE;

			/*
			**  Arrange to send to everyone.
			**	If sending to multiple people, mail back
			**		errors rather than reporting directly.
			**	In any case, don't mail back errors for
			**		anything that has happened up to
			**		now (the other end will do this).
			**	Truncate our transcript -- the mail has gotten
			**		to us successfully, and if we have
			**		to mail this back, it will be easier
			**		on the reader.
			**	Then send to everyone.
			**	Finally give a reply code.  If an error has
			**		already been given, don't mail a
			**		message back.
			**	We goose error returns by clearing error bit.
			*/

			SmtpPhase = "delivery";
			if (nrcpts != 1 && !doublequeue)
			{
				/* from now on, we have to operate silently */
				HoldErrs = TRUE;
				e->e_errormode = EM_MAIL;
			}
			e->e_xfp = freopen(queuename(e, 'x'), "w", e->e_xfp);
			id = e->e_id;

			/* send to all recipients */
			sendall(e, doublequeue ? SM_QUEUE : SM_DEFAULT);
			e->e_to = NULL;

			/* issue success if appropriate and reset */
			if (Errors == 0 || HoldErrs)
				message("250 %s Message accepted for delivery", id);

			if (bitset(EF_FATALERRS, e->e_flags) && !HoldErrs)
			{
				/* avoid sending back an extra message */
				e->e_flags &= ~EF_FATALERRS;
				e->e_flags |= EF_CLRQUEUE;
			}
			else
			{
				/* from now on, we have to operate silently */
				HoldErrs = TRUE;
				e->e_errormode = EM_MAIL;

				/* if we just queued, poke it */
				if (doublequeue && e->e_sendmode != SM_QUEUE)
				{
					extern pid_t dowork();

					unlockqueue(e);
					(void) dowork(id, FALSE, TRUE, e);
				}
			}

			/* save statistics */
			markstats(e, (ADDRESS *) NULL);

			/* if in a child, pop back to our parent */
			if (InChild)
				finis();

			/* clean up a bit */
			gotmail = FALSE;
			dropenvelope(e);
			CurEnv = e = newenvelope(e, CurEnv);
			e->e_flags = BlankEnvelope.e_flags;
			break;

		  case CMDRSET:		/* rset -- reset state */
			message("250 Reset state");
			if (InChild)
				finis();

			/* clean up a bit */
			gotmail = FALSE;
			dropenvelope(e);
			CurEnv = e = newenvelope(e, CurEnv);
			break;

		  case CMDVRFY:		/* vrfy -- verify address */
		  case CMDEXPN:		/* expn -- expand address */
			vrfy = c->cmdcode == CMDVRFY;
			if (bitset(vrfy ? PRIV_NOVRFY : PRIV_NOEXPN,
						PrivacyFlags))
			{
				if (vrfy)
					message("252 Who's to say?");
				else
					message("502 Sorry, we do not allow this operation");
# 	ifdef LOG
				if (LogLevel > 5)
					syslog(LOG_INFO, "%s: %s [rejected]",
						CurHostName, inp);
# 	endif	/* LOG */
				break;
			}
			else if (!gothello &&
				 bitset(vrfy ? PRIV_NEEDVRFYHELO : PRIV_NEEDEXPNHELO,
						PrivacyFlags))
			{
				message("503 I demand that you introduce yourself first");
				break;
			}
			if (runinchild(vrfy ? "SMTP-VRFY" : "SMTP-EXPN", e) > 0)
				break;
			setproctitle("%s: %s", CurHostName, inp);
			vrfyqueue = NULL;
			QuickAbort = TRUE;
			if (vrfy)
				e->e_flags |= EF_VRFYONLY;
			while (*p != '\0' && isascii(*p) && isspace(*p))
				*p++;
			if (*p == '\0')
			{
				message("501 Argument required");
				Errors++;
			}
			else
			{
				sendtolist(p, NULLADDR, &vrfyqueue, e);
			}
			if (Errors != 0)
			{
				if (InChild)
					finis();
				break;
			}
			if (vrfyqueue == NULL)
			{
				message("554 Nothing to %s", vrfy ? "VRFY" : "EXPN");
			}
			while (vrfyqueue != NULL)
			{
				a = vrfyqueue;
				while ((a = a->q_next) != NULL &&
				       bitset(QDONTSEND|QBADADDR, a->q_flags))
					continue;
				if (!bitset(QDONTSEND|QBADADDR, vrfyqueue->q_flags))
					printvrfyaddr(vrfyqueue, a == NULL);
				vrfyqueue = vrfyqueue->q_next;
			}
			if (InChild)
				finis();
			break;

		  case CMDHELP:		/* help -- give user info */
			help(p);
			break;

		  case CMDNOOP:		/* noop -- do nothing */
			message("250 OK");
			break;

		  case CMDQUIT:		/* quit -- leave mail */
			message("221 %s closing connection", MyHostName);

			/* avoid future 050 messages */
			Verbose = FALSE;

			if (InChild)
				ExitStat = EX_QUIT;
			finis();

		  case CMDVERB:		/* set verbose mode */
			if (bitset(PRIV_NOEXPN, PrivacyFlags))
			{
				/* this would give out the same info */
				message("502 Verbose unavailable");
				break;
			}
			Verbose = TRUE;
			e->e_sendmode = SM_DELIVER;
			message("250 Verbose mode");
			break;

		  case CMDQUED:		/* set queued delivery */
			e->e_sendmode = SM_QUEUE;
			message("250 Queued delivery");
  			break;

		  case CMDONEX:		/* doing one transaction only */
			OneXact = TRUE;
			message("250 Only one transaction");
			break;

# 	ifdef SMTPDEBUG
		  case CMDDBGQSHOW:	/* show queues */
			printf("Send Queue=");
			printaddr(e->e_sendqueue, TRUE);
			break;

		  case CMDDBGDEBUG:	/* set debug mode */
			tTsetup(tTdvect, sizeof tTdvect, "0-99.1");
			tTflag(p);
			message("200 Debug set");
			break;

# 	else /* not SMTPDEBUG */
		  case CMDDBGQSHOW:	/* show queues */
		  case CMDDBGDEBUG:	/* set debug mode */
# 		ifdef LOG
			if (RealHostName != NULL && LogLevel > 0)
				syslog(LOG_NOTICE,
				    "\"%s\" command from %s (%s)\n",
				    c->cmdname, RealHostName,
				    inet_ntoa(RealHostAddr.sin_addr));
# 		endif	/* LOG */
			/* FALL THROUGH */
# 	endif /* SMTPDEBUG */

		  case CMDERROR:	/* unknown command */
			message("500 Command unrecognized");
			break;

		  default:
			syserr("500 smtp: unknown code %d", c->cmdcode);
			break;
		}
	}
}
/*
**  SKIPWORD -- skip a fixed word.
**
**	Parameters:
**		p -- place to start looking.
**		w -- word to skip.
**
**	Returns:
**		p following w.
**		NULL on error.
**
**	Side Effects:
**		clobbers the p data area.
*/

static char *
skipword(p, w)
	register char *p;
	char *w;
{
	register char *q;

	/* find beginning of word */
	while (isascii(*p) && isspace(*p))
		p++;
	q = p;

	/* find end of word */
	while (*p != '\0' && *p != ':' && !(isascii(*p) && isspace(*p)))
		p++;
	while (isascii(*p) && isspace(*p))
		*p++ = '\0';
	if (*p != ':')
	{
	  syntax:
		message("501 Syntax error in parameters");
		Errors++;
		return (NULL);
	}
	*p++ = '\0';
	while (isascii(*p) && isspace(*p))
		p++;

	/* see if the input word matches desired word */
	if (strcasecmp(q, w))
		goto syntax;

	return (p);
}
/*
**  PRINTVRFYADDR -- print an entry in the verify queue
**
**	Parameters:
**		a -- the address to print
**		last -- set if this is the last one.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Prints the appropriate 250 codes.
*/

static void printvrfyaddr(a, last)
	register ADDRESS *a;
	bool last;
{
	char fmtbuf[20];

	strcpy(fmtbuf, "250");
	fmtbuf[3] = last ? ' ' : '-';

	if (a->q_fullname == NULL)
	{
		if (strchr(a->q_user, '@') == NULL)
			strcpy(&fmtbuf[4], "<%s@%s>");
		else
			strcpy(&fmtbuf[4], "<%s>");
		message(fmtbuf, a->q_user, MyHostName);
	}
	else
	{
		if (strchr(a->q_user, '@') == NULL)
			strcpy(&fmtbuf[4], "%s <%s@%s>");
		else
			strcpy(&fmtbuf[4], "%s <%s>");
		message(fmtbuf, a->q_fullname, a->q_user, MyHostName);
	}
}
/*
**  HELP -- implement the HELP command.
**
**	Parameters:
**		topic -- the topic we want help for.
**
**	Returns:
**		none.
**
**	Side Effects:
**		outputs the help file to message output.
*/

static void help(topic)
	char *topic;
{
	register FILE *hf;
	int len;
	char buf[MAXLINE];
	bool noinfo;

	if (HelpFile == NULL || (hf = fopen(HelpFile, "r")) == NULL)
	{
		/* no help */
		errno = 0;
		message("502 HELP not implemented");
		return;
	}

	if (topic == NULL || *topic == '\0')
		topic = "smtp";
	else
		makelower(topic);

	len = strlen(topic);
	noinfo = TRUE;

	while (fgets(buf, sizeof buf, hf) != NULL)
	{
		if (strncmp(buf, topic, len) == 0)
		{
			register char *p;

			p = strchr(buf, '\t');
			if (p == NULL)
				p = buf;
			else
				p++;
			fixcrlf(p, TRUE);
			message("214-%s", p);
			noinfo = FALSE;
		}
	}

	if (noinfo)
		message("504 HELP topic unknown");
	else
		message("214 End of HELP info");
	(void) fclose(hf);
}
/*
**  RUNINCHILD -- return twice -- once in the child, then in the parent again
**
**	Parameters:
**		label -- a string used in error messages
**
**	Returns:
**		zero in the child
**		one in the parent
**
**	Side Effects:
**		none.
*/

runinchild(label, e)
	char *label;
	register ENVELOPE *e;
{
	int childpid;

	if (!OneXact)
	{
		childpid = dofork();
		if (childpid < 0)
		{
			syserr("%s: cannot fork", label);
			return (1);
		}
		if (childpid > 0)
		{
			auto int st;

			/* parent -- wait for child to complete */
			st = waitfor(childpid);
			if (st == -1)
				syserr("%s: wait", label);

			/* if we exited on a QUIT command, complete the process */
			if (st == (EX_QUIT << 8))
				finis();

			return (1);
		}
		else
		{
			/* child */
			InChild = TRUE;
			QuickAbort = FALSE;
			clearenvelope(e, FALSE);
		}
	}

	/* open alias databases */
	if (AliasDB == NULL)
		AliasDB = initaliases(AliasFile, FALSE, e);
	if (ReverseAliasFile && (ReverseAliasDB == NULL))
		ReverseAliasDB = initaliases(ReverseAliasFile, FALSE, e);

	return (0);
}

# endif /* SMTP */
