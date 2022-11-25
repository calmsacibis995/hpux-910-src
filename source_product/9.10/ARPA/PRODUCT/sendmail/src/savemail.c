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
static char rcsid[] = "$Header: savemail.c,v 1.15.109.8 95/02/21 16:08:34 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)savemail.c	5.14 (Berkeley) 8/29/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: savemail.o $Revision: 1.15.109.8 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include <sys/types.h>
# include <pwd.h>
# include "sendmail.h"

/*
**  SAVEMAIL -- Save mail on error
**
**	If mailing back errors, mail it back to the originator
**	together with an error message; otherwise, just put it in
**	dead.letter in the user's home directory (if he exists on
**	this machine).
**
**	Parameters:
**		e -- the envelope containing the message in error.
**
**	Returns:
**		none
**
**	Side Effects:
**		Saves the letter, by writing or mailing it back to the
**		sender, or by putting it in dead.letter in her home
**		directory.
*/

/* defines for state machine */
# define ESM_REPORT	0	/* report to sender's terminal */
# define ESM_MAIL	1	/* mail back to sender */
# define ESM_QUIET	2	/* messages have already been returned */
# define ESM_DEADLETTER	3	/* save in ~/dead.letter */
# define ESM_POSTMASTER	4	/* return to postmaster */
# define ESM_USRTMP	5	/* save in /usr/tmp/dead.letter */
# define ESM_PANIC	6	/* leave the locked queue/transcript files */
# define ESM_DONE	7	/* the message is successfully delivered */


void savemail(e)
	register ENVELOPE *e;
{
	register struct passwd *pw;
	register FILE *fp;
	int state;
	auto ADDRESS *q;
	char buf[MAXLINE+1];
	extern struct passwd *getpwnam();
	register char *p;
	extern char *ttypath();
	typedef int (*fnptr)();
	int status;

# ifdef DEBUG
	if (tTd(6, 1))
	{
		printf("\nsavemail, errormode = %c, id = %s\n  e_from=",
			e->e_errormode, e->e_id == NULL ? "NONE" : e->e_id);
		printaddr(&e->e_from, FALSE);
	}
#  endif /* DEBUG */

	if (bitset(EF_RESPONSE, e->e_flags))
		return;
	if (e->e_class < 0)
	{
		message("Dumping junk mail");
		return;
	}
	e->e_flags &= ~EF_FATALERRS;

	e->e_to = NULL;

	/*
	**  Basic state machine.
	**
	**	This machine runs through the following states:
	**
	**	ESM_QUIET	Errors have already been printed iff the
	**			sender is local.
	**	ESM_REPORT	Report directly to the sender's terminal.
	**	ESM_MAIL	Mail response to the sender.
	**	ESM_DEADLETTER	Save response in ~/dead.letter.
	**	ESM_POSTMASTER	Mail response to the postmaster.
	**	ESM_PANIC	Save response anywhere possible.
	*/

	/* determine starting state */
	switch (e->e_errormode)
	{
	  case EM_WRITE:
		state = ESM_REPORT;
		break;

	  case EM_BERKNET:
		/* mail back, but return o.k. exit status */
		ExitStat = EX_OK;

		/* fall through.... */

	  case EM_MAIL:
		state = ESM_MAIL;
		break;

	  case EM_PRINT:
	  case '\0':
		state = ESM_QUIET;
		break;

	  case EM_QUIET:
		/* no need to return anything at all */
		return;

	  default:
		syserr("554 savemail: bogus errormode x%x\n", e->e_errormode);
		state = ESM_MAIL;
		break;
	}

	while (state != ESM_DONE)
	{
# ifdef DEBUG
		if (tTd(6, 5))
			printf("  state %d\n", state);
#  endif /* DEBUG */
		switch (state)
		{
		  case ESM_QUIET:
			if (e->e_from.q_mailer == LocalMailer)
				state = ESM_DEADLETTER;
			else
				state = ESM_MAIL;
			break;

		  case ESM_REPORT:

			/*
			**  If the user is still logged in on the same terminal,
			**  then write the error messages back to hir (sic).
			*/

			p = ttypath();
			if (p == NULL || freopen(p, "w", stdout) == NULL)
			{
				state = ESM_MAIL;
				break;
			}

			expand("\001n", buf, &buf[sizeof buf - 1], e);
			printf("\r\nMessage from %s...\r\n", buf);
			printf("Errors occurred while sending mail.\r\n");
			if (e->e_xfp != NULL)
			{
				(void) fflush(e->e_xfp);
				fp = fopen(queuename(e, 'x'), "r");
			}
			else
				fp = NULL;
			if (fp == NULL)
			{
				syserr("Cannot open %s", queuename(e, 'x'));
				printf("Transcript of session is unavailable.\r\n");
			}
			else
			{
				printf("Transcript follows:\r\n");
				while (fgets(buf, sizeof buf, fp) != NULL &&
				       !ferror(stdout))
					fputs(buf, stdout);
				(void) fclose(fp);
			}
			printf("Original message will be saved in dead.letter.\r\n");
			state = ESM_DEADLETTER;
			break;

		  case ESM_MAIL:
		  case ESM_POSTMASTER:
			/*
			**  If mailing back, do it.
			**	Throw away all further output.  Don't alias,
			**	since this could cause loops, e.g., if joe
			**	mails to joe@x, and for some reason the network
			**	for @x is down, then the response gets sent to
			**	joe@x, which gives a response, etc.  Also force
			**	the mail to be delivered even if a version of
			**	it has already been sent to the sender.
			*/

			if (state == ESM_MAIL)
			{
				if (e->e_errorqueue == NULL)
				{
					if (e->e_from.q_paddr != NULL)
					{
						sendtolist(e->e_from.q_paddr, (ADDRESS *) NULL,
							   &e->e_errorqueue, e);
					}
					else if (PostMasterCopy == NULL)
					{
						state = ESM_POSTMASTER;
						break;
					}
				}

				/* Set up return message subject */
				sprintf(buf, "Returned mail: %s", e->e_message != NULL ?
					e->e_message : "Unable to deliver mail");
				q = e->e_errorqueue;
			}
			else
			{
				/* 
				** Since we couldn't mail it back, we'll send
				** it to the local Postmaster.  Note that the
				** postmaster may have already received a cc:
				** of the message header, if he/she was the
				** PostMasterCopy address.
				*/
				q = NULL;
				sendtolist("Postmaster", (ADDRESS *) NULL, &q, e);
				sprintf(buf, "Unable to return to sender (Returned Mail: %s)", 
					e->e_message != NULL ? 	e->e_message 
					                     : "Unable to deliver mail");
			}
			
			status = returntosender(buf, q, TRUE, e);

			/* deliver a cc: of the header to the postmaster if desired */
			if (state == ESM_MAIL && PostMasterCopy != NULL)
			{
				ADDRESS *p = NULL;
				sendtolist(PostMasterCopy, (ADDRESS *) NULL, &p, e);
				if (p != NULL)
					returntosender(buf, p, FALSE, e);
			}

			/* determine next state */
			if (status == 0)
				state = ESM_DONE;
			else
				state = state == ESM_MAIL ? ESM_POSTMASTER : ESM_USRTMP;
			break;

		  case ESM_DEADLETTER:
			/*
			**  Save the message in dead.letter.
			**	If we weren't mailing back, and the user is
			**	local, we should save the message in
			**	~/dead.letter so that the poor person doesn't
			**	have to type it over again -- and we all know
			**	what poor typists UNIX users are.
			*/

			p = NULL;
			if (e->e_from.q_mailer == LocalMailer)
			{
				if (e->e_from.q_home != NULL)
					p = e->e_from.q_home;
				else if ((pw = getpwnam(e->e_from.q_user)) != NULL)
					p = pw->pw_dir;
			}
			if (p == NULL)
			{
				syserr("Can't return mail to %s", e->e_from.q_paddr);
				state = ESM_MAIL;
				break;
			}
			if (e->e_dfp != NULL)
			{
				auto ADDRESS *q;
				bool oldverb = Verbose;

				/* we have a home directory; open dead.letter */
				define('z', p, e);
				expand("\001z/dead.letter", buf, &buf[sizeof buf - 1], e);
				Verbose = TRUE;
				message("Saving message in %s", buf);
				Verbose = oldverb;
				e->e_to = buf;
				q = NULL;
				ForceMail = TRUE;
				sendtolist(buf, (ADDRESS *) NULL, &q, e);
				ForceMail = FALSE;
				if (deliver(e, q) == 0)
					state = ESM_DONE;
				else
					state = ESM_MAIL;
			}
			else
			{
				/* no data file -- try mailing back */
				state = ESM_MAIL;
			}
			break;

		  case ESM_USRTMP:
			/*
			**  Log the mail in /usr/tmp/dead.letter.
			*/
# ifdef V4FS
			e->e_to = "/var/tmp/dead.letter";
# else	/* ! V4FS */
			e->e_to = "/usr/tmp/dead.letter";
# endif	/* V4FS */
			logdelivery("Can't return mail to anyone", e);
			fp = dfopen(e->e_to, "a");
			if (fp == NULL)
			{
				state = ESM_PANIC;
				break;
			}

			putfromline(fp, ProgMailer, e);
			(*e->e_puthdr)(fp, ProgMailer, e);
			putline("\n", fp, ProgMailer);
			(*e->e_putbody)(fp, ProgMailer, e);
			putline("\n", fp, ProgMailer);
			(void) fflush(fp);
			state = ferror(fp) ? ESM_PANIC : ESM_DONE;
			(void) fclose(fp);
			(void) chmod(e->e_to, FileMode);
			break;

		  default:
			syserr("554 savemail: unknown state %d", state);

			/* fall through ... */

		  case ESM_PANIC:
			syserr("savemail: HELP!!!!");
# ifdef LOG
			if (LogLevel >= 1)
				syslog(LOG_ALERT, "savemail: HELP!!!!");
#  endif /* LOG */

			/* leave the locked queue & transcript files around */
			exit(EX_SOFTWARE);
		}
	}
}
/*
**  RETURNTOSENDER -- return a message to the sender with an error.
**
**	Parameters:
**		msg -- the explanatory message.
**		returnq -- the queue of people to send the message to.
**		sendbody -- if TRUE, also send back the body of the
**			message; otherwise just send the header.
**		e -- the current envelope.
**
**	Returns:
**		zero -- if everything went ok.
**		else -- some error.
**
**	Side Effects:
**		Returns the current message to the sender via
**		mail.
*/

bool	SendBody;

# define MAXRETURNS	6	/* max depth of returning messages */

returntosender(msg, returnq, sendbody, e)
	char *msg;
	ADDRESS *returnq;
	bool sendbody;
	register ENVELOPE *e;
{
	char buf[MAXLINE];
	extern putheader(), errbody();
	register ENVELOPE *ee;
	ENVELOPE *oldcur = CurEnv;
	ENVELOPE errenvelope;
	static int returndepth;
	register ADDRESS *q;
	bool first_time = TRUE;

# ifdef DEBUG
	if (tTd(6, 1))
	{
		printf("Return To Sender: msg=\"%s\", depth=%d, e=%x, returnq=",
		       msg, returndepth, e);
		printaddr(returnq, TRUE);
	}
#  endif /* DEBUG */

	if (++returndepth >= MAXRETURNS)
	{
		if (returndepth != MAXRETURNS)
			syserr("554 returntosender: infinite recursion on %s", returnq->q_paddr);
		/* don't "unrecurse" and fake a clean exit */
		/* returndepth--; */
		return (0);
	}

	SendBody = sendbody;
	define('g', "\001f", e);
	ee = newenvelope(&errenvelope, e);
	ee->e_puthdr = putheader;
	ee->e_putbody = errbody;
	ee->e_flags |= EF_RESPONSE;
	ee->e_flags &= ~EF_FATALERRS;
	if (!bitset(EF_OLDSTYLE, e->e_flags))
		ee->e_flags &= ~EF_OLDSTYLE;
	ee->e_sendqueue = returnq;
	openxscript(ee);
	/*
	**	form a To: header from the return queue
	*/
	bzero(buf, sizeof(buf));
	for (q = returnq; q != NULL; q = q->q_next)
	{
		if (bitset(QBADADDR, q->q_flags))
			continue;

		if (!bitset(QDONTSEND, q->q_flags))
			ee->e_nrcpts++;

		if (q->q_alias != NULL)
			continue;
		if (strlen(q->q_paddr) + 1 >= sizeof(buf))
		{
			syserr("savemail: error recipient name too long");
			continue;
		}
		if ((strlen(buf) + strlen(q->q_paddr)
			    + (first_time ? 1 : 3)) <= sizeof(buf))
		{
			/*
			**	if the address fits, add it to the To: line
			*/
			if (!first_time)
				strcat(buf, ", ");
			strcat(buf, q->q_paddr);
			first_time = FALSE;
		}
		else
		{
			/*
			**	if it doesn't fit, store the old one and
			**	start a new To: line
			*/
			addheader("to", buf, ee);
			bzero(buf, sizeof(buf));
			strcat(buf, q->q_paddr);
			first_time = FALSE;
		}
	}
	if (*buf != NULL)
	{
		addheader("to", buf, ee);
		bzero(buf, sizeof(buf));
	}
	addheader("subject", msg, ee);

	/* fake up an address header for the from person */
	expand("\001n", buf, &buf[sizeof buf - 1], e);
	if (parseaddr(buf, &ee->e_from, -1, '\0', e) == NULL)
	{
		syserr("553 Can't parse myself!");
		ExitStat = EX_SOFTWARE;
		returndepth--;
		return (-1);
	}
	loweraddr(&ee->e_from);
	ee->e_sender = ee->e_from.q_paddr;

	/* push state into submessage */
	CurEnv = ee;
	define('f', "\001n", ee);
	define('x', "Mail Delivery Subsystem", ee);
	eatheader(ee, TRUE);

	/* actually deliver the error message */
	sendall(ee, SM_DEFAULT);

	/* restore state */
	dropenvelope(ee);
	CurEnv = oldcur;
	returndepth--;

	/* check for delivery errors here */
	return(bitset(EF_FATALERRS|EF_TIMEOUT, ee->e_flags));
}

/*
 * uniqprf - generate a unique prefix for MIME encapsulations
 */

# define	INITIAL_PREFIX	"----- =_aaaaaaaaaa"
static char    prefix[sizeof INITIAL_PREFIX] = "";

uniqprf (e)
ENVELOPE *e;
{
    char   *cp;

    (void) fflush(stdout);
    if (e->e_xfp)
	(void) fflush(e->e_xfp);

    (void) strcpy(prefix, INITIAL_PREFIX);
    cp = strchr(prefix, 'a');

    while (!uniqtry(e))
	if (*cp < 'z')
	    (*cp)++;
        else
	    if (*++cp == 0) {
		syserr("unable to determine unique delimiter string?!?");
		(void) strcpy(prefix, INITIAL_PREFIX);
		break;
	    } else
		(*cp)++;
}
/* 
 * uniqtry - make one pass through it all
 */

uniqtry (e)
ENVELOPE *e;
{
    int	    len = strlen (prefix);
    register char *cp;
    char   *qf,
	    buffer[MAXLINE];
    FILE   *qp;

    if (qp = fopen(qf = queuename(e->e_parent, 'x'), "r")) {
	while (fgets(buffer, sizeof buffer, qp))
	    if ((buffer[0] == '-') && (buffer[1] == '-')) {
		for (cp = buffer + strlen (buffer) - 1; cp >= buffer; cp--)
		    if (!isspace(*cp))
			break;
		*++cp = NULL;

		if (strncmp(buffer + 2, prefix, len) == 0) {
		    (void) fclose(qp);
		    return FALSE;
		}
	    }

	(void) fclose(qp);
    } else
	syserr("unable to fopen %s for reading", qf);

    if (NoReturn || !(qp = e->e_parent->e_dfp) || !SendBody)
	return TRUE;

    rewind (qp);
    while (fgets(buffer, sizeof buffer, qp))
	if ((buffer[0] == '-') && (buffer[1] == '-')) {
	    for (cp = buffer + strlen (buffer) - 1; cp >= buffer; cp--)
		if (!isspace(*cp))
		    break;
	    *++cp = NULL;

	    if (strncmp(buffer + 2, prefix, len) == 0)
		return FALSE;
	}

    return TRUE;
}
/*
**  ERRBODY -- output the body of an error message.
**
**	Typically this is a copy of the transcript plus a copy of the
**	original offending message.
**
**	Parameters:
**		fp -- the output file.
**		m -- the mailer to output to.
**		e -- the envelope we are working in.
**
**	Returns:
**		none
**
**	Side Effects:
**		Outputs the body of an error message.
*/

errbody(fp, m, e)
	register FILE *fp;
	register struct mailer *m;
	register ENVELOPE *e;
{
	register FILE *xfile;
	char buf[MAXLINE];
	char *p;
	char type[MAXNAME];

	if (prefix[0] == NULL)
	    uniqprf(e);

	(void) sprintf(type, "multipart/mixed; boundary=\"%s\"", prefix);

	addheader("mime-version", "1.0", e);
	addheader("content-type", type, e);

	/*
	**  Output transcript of errors
	*/

	(void) fflush(stdout);
	p = queuename(e->e_parent, 'x');
	fprintf (fp, "--%s\nContent-Type: text/plain; charset=\"us-ascii\"\n", prefix);
	if ((xfile = fopen(p, "r")) == NULL)
	{
		syserr("Cannot open %s", p);
		fprintf(fp, "\nTranscript of session is unavailable\n");
	}
	else
	{
	        fprintf(fp, "Content-Description: Session Transcript\n\n");
		if (e->e_xfp != NULL)
			(void) fflush(e->e_xfp);
		while (fgets(buf, sizeof buf, xfile) != NULL)
			putline(buf, fp, m);
		(void) fclose(xfile);
	}
	errno = 0;

	/*
	**  Output text of original message
	*/

	if (NoReturn){
		fprintf(fp, "\n--%s\nContent-Type: text/plain; charset=\"us-ascii\"\n\n", prefix);
		fprintf(fp, "Returned message suppressed\n");
	} else if (e->e_parent->e_dfp != NULL){
	        fprintf(fp, "\n--%s\nContent-Type: message/rfc822\n", prefix);
		fprintf(fp, "Content-Description: Returned Content%s\n\n",
			SendBody ? "" : " (headers only)");
		(void) fflush(fp);
		putheader(fp, m, e->e_parent);

		if (SendBody){
			putline("\n", fp, m);
			putbody(fp, m, e->e_parent);
		}
	}
	else {
		fprintf(fp, "\n--%s\nContent-Type: text/plain; charset=\"us-ascii\"\n\n", prefix);
		fprintf(fp, "No message was collected\n");
	}

	fprintf(fp,"\n--%s--\n", prefix);

	/*
	**  Cleanup and exit
	*/

	if (errno != 0)
		syserr("errbody: I/O error");
}
