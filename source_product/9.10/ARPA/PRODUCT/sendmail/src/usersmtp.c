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
static char rcsid[] = "$Header: usersmtp.c,v 1.22.109.8 95/02/21 16:09:06 mike Exp $";
# 	ifndef hpux
# 		ifdef SMTP
static char sccsid[] = "@(#)usersmtp.c	5.15 (Berkeley) 6/1/90 (with SMTP)";
# 		else	/* ! SMTP */
static char sccsid[] = "@(#)usersmtp.c	5.15 (Berkeley) 6/1/90 (without SMTP)";
# 		endif	/* SMTP */
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: usersmtp.o $Revision: 1.22.109.8 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include "sendmail.h"
# include <sysexits.h>
# include <errno.h>

# ifdef SMTP

/*
**  USERSMTP -- run SMTP protocol from the user end.
**
**	This protocol is described in RFC821.
*/

# 	define REPLYTYPE(r)	((r) / 100)		/* first digit of reply code */
# 	define REPLYCLASS(r)	(((r) / 10) % 10)	/* second digit of reply code */
# 	define SMTPCLOSING	421			/* "Service Shutting Down" */

char	SmtpMsgBuffer[MAXLINE];		/* buffer for commands */
char	SmtpReplyBuffer[MAXLINE];	/* buffer for replies */
char	SmtpError[MAXLINE] = "";	/* save failure error messages */
bool	SmtpNeedIntro;			/* set before first error */
FILE	*SmtpOut;			/* output file */
FILE	*SmtpIn;			/* input file */
int	SmtpPid;			/* pid of mailer */

/* following represents the state of the SMTP connection */
int	SmtpState;			/* connection state, see below */

# 	define SMTP_CLOSED	0		/* connection is closed */
# 	define SMTP_OPEN	1		/* connection is open for business */
# 	define SMTP_SSD	2		/* service shutting down */

int	doingEHLO;			/* EHLO in progress */

# 	define	MAXEHLO	10			/* EHLO keywords supported */
char   *EHLOkeys[MAXEHLO + 1];

/*
**  SMTPINIT -- initialize SMTP.
**
**	Opens the connection and sends the initial protocol.
**
**	Parameters:
**		m -- mailer to create connection to.
**		pvp -- pointer to parameter vector to pass to
**			the mailer.
**		e -- the envelope for this message.
**
**	Returns:
**		appropriate exit status -- EX_OK on success.
**		If not EX_OK, it should close the connection.
**
**	Side Effects:
**		creates connection and sends initial protocol.
*/

jmp_buf	CtxGreeting;

smtpinit(m, pvp, e)
	struct mailer *m;
	char **pvp;
	ENVELOPE *e;
{
	register int r;
	EVENT *gte;
	char buf[MAXNAME], size[MAXNAME], bit8[MAXNAME];
	extern greettimeout();
	int losing_ehlo = 0;
	extern char *EHLOset();
	extern mimefy();

	/*
	**  Open the connection to the mailer.
	*/

# 	ifdef DEBUG
	if (SmtpState == SMTP_OPEN)
		syserr("smtpinit: already open");
# 	endif /* DEBUG */

try_again: ;
	SmtpIn = SmtpOut = NULL;
	SmtpState = SMTP_CLOSED;
	SmtpError[0] = '\0';
	SmtpNeedIntro = TRUE;
	SmtpPhase = "user open";
	setproctitle("%s %s: %s", e->e_id, pvp[1], SmtpPhase);
	SmtpPid = openmailer(m, pvp, (ADDRESS *) NULL, TRUE, &SmtpOut, &SmtpIn, e);
	if (SmtpPid < 0)
	{
# 	ifdef DEBUG
		if (tTd(18, 1))
			printf("smtpinit: Cannot open %s: stat %d errno %d\n",
			   pvp[0], ExitStat, errno);
# 	endif /* DEBUG */
		if (e->e_xfp != NULL)
		{
			register char *p;
			extern char *errstring();
			extern char *statstring();

			if (errno == 0)
			{
				p = statstring(ExitStat);
				fprintf(e->e_xfp, "%.3s %s (%s)... %s\n",
					p, CurHostName, m->m_name, &p[4]);
			}
			else
			{
				r = errno;
				fprintf(e->e_xfp, "421 %s (%s)... Deferred",
					pvp[1], m->m_name);
				fprintf(e->e_xfp, ": %s\n", errstring(r));
				errno = r;
			}
		}
		return (ExitStat);
	}
	SmtpState = SMTP_OPEN;

	/*
	**  Get the greeting message.
	**	This should appear spontaneously.  
	**	If we have a ReadTimeout, the read will timeout in sfgets.
	**	Otherwise, we arrange to timeout here.
	*/

	SmtpPhase = "greeting wait";
	setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	if (ReadTimeout == 0) {
		if (setjmp(CtxGreeting) != 0)
			goto tempfail;
		gte = setevent((time_t) 300, greettimeout, 0);
		r = reply(m, e);
		clrevent(gte);
	}
	else {
		r = reply(m, e);
	}

	if (r < 0 || REPLYTYPE(r) != 2)
		goto tempfail;

	/*
	**  Send the HELO command.
	**	My mother taught me always to introduce myself.
	*/

	if (losing_ehlo)
	    goto lost_ehlo;
	SmtpPhase = "EHLO wait";
	smtpmessage("EHLO %s", m, MyHostName);
	setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	doingEHLO = 1;
	r = reply(m, e);
	doingEHLO = 0;
	if (r < 0) {
undo_ehlo: ;
	    losing_ehlo = 1;
	    smtpquit(m, e);
	    sleep(5);
	    goto try_again;
	}

	if (REPLYTYPE (r) == 5) {
lost_ehlo: ;
	    SmtpPhase = "HELO wait";
	    smtpmessage("HELO %s", m, MyHostName);
	    setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	    r = reply(m, e);
	    if (r < 0) {
		if (losing_ehlo)
		    goto tempfail;
		else
		    goto undo_ehlo;
	    } else if (REPLYTYPE(r) == 5)
	       goto unavailable;
	}
	if (REPLYTYPE(r) != 2)
	    goto tempfail;

	if (EHLOset("SIZE")) {
	    long   octets;
	    extern long hdrsize (), bodysize ();

	    octets = hdrsize(m, e);
	    if (e->e_putbody == putbody) {
		if (e->e_msgsize == 0)
		    e->e_msgsize = bodysize(e);
		octets += e->e_msgsize;
	    } else {
		extern bool SendBody;

		octets += 500;	/* not worth calculating the real amount... */
		if (!NoReturn && (e->e_parent->e_dfp)) {
		    octets += hdrsize (m, e->e_parent);
		    if (SendBody)
			octets += bodysize (e->e_parent);
		}
	    }

	    (void) sprintf(size, " SIZE=%ld", octets);
	} else
	    size[0] = '\0';

	if (Saw8Bits || (strcasecmp(e->e_bodytype, "8BITMIME") == 0)){
	    if (EHLOset("8BITMIME")){
		(void) strcpy(bit8, " BODY=8BITMIME");
		e->e_bodytype = "8BITMIME";
	    } else {
		if (Bit8Mode == B8_REJECT)
		    goto unavailable;
		if ((Bit8Mode == B8_ENCODE) && (!bitset(EF_RESPONSE, e->e_flags))){
		    /*
		     * We should not encode an error response: it must have had
		     * an 8-bit path to get here, so assume it can get back.
		     * Anyway, in this case we want the sender to get the
		     * message back in its original form.
		     */
		    char *tmp_file_out_name, *tmp_file_in_name, *value;
		    FILE *tmp_file_out;
		    HDR *h;

		    tmp_file_out_name = tempnam(QueueDir, "sendm");
		    tmp_file_out = fopen(tmp_file_out_name, "w");
		    /*
		     * Add a MIME-Version: header if one is not already present.
		     */
		    if (hvalue("mime-version", e) == NULL)
			addheader("mime-version", "1.0", e);
		    /*
		     * Put the Content-Type: header to the temp file;
		     * it will be needed to parse multi-part messages.
		     */
		    if (value = hvalue("content-type", e))
			putheaderline(tmp_file_out, m, "content-type", value);
		    /*
		     * Content-Transfer-Encoding: is difficult.  mimefy() must
		     * write the new value for this header.  It uses chompheader(),
		     * which only over-writes an old value if the H_DEFAULT
		     * flag is set.  We can't count on the flag being set by
		     * declaring it in the config-file, so we must set the
		     * H_DEFAULT flag the hard way!
		     */
		    for (h = e->e_header; h != NULL; h = h->h_link)
		        if (strcmp(h->h_field, "content-transfer-encoding") == 0){
			    h->h_flags |= H_DEFAULT;
			    putheaderline(tmp_file_out, m, "content-transfer-encoding", h->h_value);
			    break;
			}
		    /*
		     * Now put the header/body separator, then the body.
		     */
		    putline("\n", tmp_file_out, m);
		    (*e->e_putbody)(tmp_file_out, m, e);
		    fclose(tmp_file_out);
		    tmp_file_in_name = tempnam(QueueDir, "sendm");
		    if (mimefy(tmp_file_out_name, tmp_file_in_name, e)){
			/*
			 * Non-zero return => failure: clean up and continue.
			 */
			xunlink(tmp_file_in_name);
			free(tmp_file_in_name);
			xunlink(tmp_file_out_name);
			free(tmp_file_out_name);
		    } else {
		        /*
			 * Zero return => success: clean up the temp files,
			 * and set the envelope fields to the new values.
			 */
			xunlink(tmp_file_out_name);
			free(tmp_file_out_name);
			if (e->e_dfp)
			    fclose(e->e_dfp);
			if (rename(tmp_file_in_name, e->e_df) < 0){
			    char action[255];
			    int ret_val;

			    sprintf(action, "cp %s %s", tmp_file_in_name, e->e_df);
			    ret_val = system(action);
			    xunlink(tmp_file_in_name);
			    if (ret_val)
			        syserr("Cannot rename(%s, %s) or cp, uid=%d",
				       tmp_file_in_name, e->e_df, geteuid());
			}
			if ((e->e_dfp = fopen(e->e_df, "r")) == NULL)
			    syserr("Cannot reopen %s", e->e_df);
			free(tmp_file_in_name);
			e->e_bodytype = "7BIT";
		    }
		}
		/*
		 * B8_SEND_ANYWAY needs no action...
		 */
		bit8[0] = '\0';
	    }
	} else
	    bit8[0] = '\0';

	/*
	**  If this is expected to be another sendmail, send some internal
	**  commands.
	*/

	if (Verbose && ((EHLOset("XVRB")) || (EHLOset("VERB")))){
	    smtpmessage("VERB", m);
	    if ((r = reply(m, e)) < 0)
		goto tempfail;
	}
	if ((EHLOset("XONE")) || (EHLOset("ONEX"))){
	    smtpmessage("ONEX", m);
	    if ((r = reply(m, e)) < 0)
		goto tempfail;
	}

	if (!EHLOkeys[0] && bitnset(M_INTERNAL, m->m_flags)){
		/* tell it to be verbose */
		smtpmessage("VERB", m);
		r = reply(m, e);
		if (r < 0)
			goto tempfail;

		/* tell it we will be sending one transaction only */
		smtpmessage("ONEX", m);
		r = reply(m, e);
		if (r < 0)
			goto tempfail;
	}

	/*
	**  Send the MAIL command.
	**	Designates the sender.
	*/

one_more_time: ;
	/*
	 * If this is a response, set the sender to <> (i.e., NULL), to avoid
	 * bounce explosions, unless the mailer flag is set saying not to.
	 */
	if (bitset(EF_RESPONSE, e->e_flags) && !bitnset(M_NO_NULL_FROM, m->m_flags))
		(void) strcpy(buf, "");
	else
		expand("\001g", buf, &buf[sizeof buf - 1], e);
	if (e->e_from.q_mailer == LocalMailer || !bitnset(M_FROMPATH, m->m_flags))
	{
		if (buf[0] == '<')
			smtpmessage("MAIL From:%s%s%s", m, buf, size, bit8);
		else
			smtpmessage("MAIL From:<%s>%s%s", m, buf, size, bit8);
	}
	else
	{
		smtpmessage("MAIL From:<@%s%c%s>%s%s", m, MyHostName,
			buf[0] == '@' ? ',' : ':', buf, size, bit8);
	}
	SmtpPhase = "MAIL wait";
	setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	r = reply(m, e);
	if (r < 0 || REPLYTYPE(r) == 4)
		goto tempfail;
	else if (r == 250)
		return (EX_OK);
	else if (r == 552){
	        if (size) {
		    extern bool Sendbody;

		    if (e->e_putbody != putbody) {
			/* ugh!  don't return the body... */
			SendBody = FALSE;
			size[0] = '\0';
			goto one_more_time;
		    }
		}
		goto unavailable;
	}
	/* protocol error -- close up */
	smtpquit(m, e);
	return (EX_PROTOCOL);

	/* signal a temporary failure */
  tempfail:
	smtpquit(m, e);
	return (EX_TEMPFAIL);

	/* signal service unavailable */
  unavailable:
	smtpquit(m, e);
	return (EX_UNAVAILABLE);
}


static 
greettimeout()
{
	/* timeout reading the greeting message */
	longjmp(CtxGreeting, 1);
}
/*
**  SMTPRCPT -- designate recipient.
**
**	Parameters:
**		to -- address of recipient.
**		m -- the mailer we are sending to.
**		e -- the envelope for this transaction.
**
**	Returns:
**		exit status corresponding to recipient status.
**
**	Side Effects:
**		Sends the mail via SMTP.
*/

smtprcpt(to, m, e)
	ADDRESS *to;
	register MAILER *m;
	ENVELOPE *e;
{
	register int r;
	char *rcpt;
	extern char *remotename();

	rcpt = remotename(to->q_user, m, FALSE, TRUE, e);
	if (rcpt && *rcpt == '<')
		smtpmessage("RCPT To:%s", m, rcpt);
	else
		smtpmessage("RCPT To:<%s>", m, rcpt);

	SmtpPhase = "RCPT wait";
	setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	r = reply(m, e);
	if (r < 0 || REPLYTYPE(r) == 4)
		return (EX_TEMPFAIL);
	else if (REPLYTYPE(r) == 2)
		return (EX_OK);
	else if (r == 550 || r == 551 || r == 553)
		return (EX_NOUSER);
	else if (r == 552 || r == 554)
		return (EX_UNAVAILABLE);
	return (EX_PROTOCOL);
}
/*
**  SMTPDATA -- send the data and clean up the transaction.
**
**	Parameters:
**		m -- mailer being sent to.
**		e -- the envelope for this message.
**
**	Returns:
**		exit status corresponding to DATA command.
**
**	Side Effects:
**		none.
*/

smtpdata(m, e)
	struct mailer *m;
	register ENVELOPE *e;
{
	register int r;

	/*
	**  Send the data.
	**	First send the command and check that it is ok.
	**	Then send the data.
	**	Follow it up with a dot to terminate.
	**	Finally get the results of the transaction.
	*/

	/* send the command and check ok to proceed */
	smtpmessage("DATA", m);
	SmtpPhase = "client DATA 354";
	setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	r = reply(m, e);
	if (r < 0 || REPLYTYPE(r) == 4)
		return (EX_TEMPFAIL);
	else if (r == 554)
		return (EX_UNAVAILABLE);
	else if (r != 354)
	{
# 	ifdef LOG
		if (LogLevel > 1)
		{
			syslog(LOG_CRIT, "%s: SMTP DATA-1 protocol error: %s",
				e->e_id, SmtpReplyBuffer);
		}
# 	endif	/* LOG */
		return (EX_PROTOCOL);
	}

	/* now output the actual message */
	(*e->e_puthdr)(SmtpOut, m, e);
	putline("\n", SmtpOut, m);
	(*e->e_putbody)(SmtpOut, m, e);

	if (ferror(SmtpOut))
	{
		/* error during processing -- don't send the dot */
		ExitStat = EX_IOERR;
		return (EX_IOERR);
	}

	/* terminate the message */
	fprintf(SmtpOut, ".%s", m->m_eol);
	if (Verbose && !HoldErrs)
		nmessage(">>> .");

	/* check for the results of the transaction */
	SmtpPhase = "client DATA 250";
	setproctitle("%s %s: %s", e->e_id, CurHostName, SmtpPhase);
	r = reply(m, e);
	if (r < 0 || REPLYTYPE(r) == 4)
		return (EX_TEMPFAIL);
	else if (r == 250)
		return (EX_OK);
	else if (r == 552 || r == 554)
		return (EX_UNAVAILABLE);
# 	ifdef LOG
	if (LogLevel > 1)
	{
		syslog(LOG_CRIT, "%s: SMTP DATA-2 protocol error: %s",
			e->e_id, SmtpReplyBuffer);
	}
# 	endif	/* LOG */
	return (EX_PROTOCOL);
}
/*
**  SMTPQUIT -- close the SMTP connection.
**
**	Parameters:
**		m -- a pointer to the mailer.
**
**	Returns:
**		none.
**
**	Side Effects:
**		sends the final protocol and closes the connection.
*/

void smtpquit(m, e)
	register MAILER *m;
	ENVELOPE *e;
{
	int i;
	char *s;

	/* if the connection is already closed, don't bother */
	if (SmtpIn == NULL)
		return;

	/* send the quit message if not a forced quit */
	if (SmtpState == SMTP_OPEN || SmtpState == SMTP_SSD)
	{
		smtpmessage("QUIT", m);
		(void) reply(m, e);
		if (SmtpState == SMTP_CLOSED)
			return;
	}

	/* now actually close the connection */
	(void) fclose(SmtpIn);
	(void) fclose(SmtpOut);
	SmtpIn = SmtpOut = NULL;
	SmtpState = SMTP_CLOSED;

	/* and pick up the zombie */
	i = endmailer(SmtpPid, m->m_argv[0]);
	if (i != EX_OK) 
	{
		s = statstring(i);
		message(s, "%s: %s", m->m_mailer, s+4);
	}

	/* emit a blank line in the transcript to separate this session */
	if (!SmtpNeedIntro && e->e_xfp != NULL)
		fputc('\n', e->e_xfp);
}
/*
**  REPLY -- read arpanet reply
**
**	Parameters:
**		m -- the mailer we are reading the reply from.
**		e -- the current envelope.
**
**	Returns:
**		reply code it reads.
**
**	Side Effects:
**		flushes the mail file.
*/

reply(m, e)
	MAILER *m;
	ENVELOPE *e;
{
	char  **ehlo;

	(void) fflush(SmtpOut);

# 	ifdef DEBUG
	if (tTd(18, 1))
		printf("reply\n");
# 	endif /* DEBUG */

	if (doingEHLO) {
	    static int at_least_once = 0;

	    if (at_least_once) {
		char   *ep;

		for (ehlo = EHLOkeys; ep = *ehlo; ehlo++)
		    free (ep);
	    } else
		at_least_once = 1;

	    *(ehlo = EHLOkeys) = NULL;
	}

	/*
	**  Read the input line, being careful not to hang.
	*/

	for (;;)
	{
		register int r;
		register char *p;

		/* actually do the read */
		if (e->e_xfp != NULL)
			(void) fflush(e->e_xfp);	/* for debugging */

		/* if we are in the process of closing just give the code */
		if (SmtpState == SMTP_CLOSED)
			return (SMTPCLOSING);

		/* get the line from the other side */
		p = sfgets(SmtpReplyBuffer, sizeof SmtpReplyBuffer, SmtpIn);
		if (p == NULL)
		{
			extern char MsgBuf[];		/* err.c */
			extern int sys_nerr;
			extern char *sys_errlist[];
			int r;

			/* save the errno */
			r = errno;
			/* no reply -- log the previous command */
			if (e->e_xfp != NULL)
			{
				if (SmtpMsgBuffer[0] != '\0')
					fprintf(e->e_xfp, ">>> %s\n", SmtpMsgBuffer);
				SmtpMsgBuffer[0] = '\0';
			}
			/* Report that connection ended prematurely */
			usrerr("451 %s while awaiting SMTP reply from %s",
				(r > 0 && r < sys_nerr) ? sys_errlist[r] : "Unexpected close",
				CurHostName);
# 	ifdef LOG
			if (LogLevel > 3)
				syslog(LOG_ERR, "%s: %s",
					e->e_id == NULL ? "NOQUEUE" : e->e_id,
					&MsgBuf[4]);
# 	endif /* LOG */
# 	ifdef DEBUG
			/* if debugging, pause so we can see state */
			if (tTd(18, 100))
				pause();
# 	endif /* DEBUG */
			SmtpState = SMTP_CLOSED;
			smtpquit(m, e);
			/* Restore the errno */
			errno = r;
			return (-1);
		}
		fixcrlf(SmtpReplyBuffer, TRUE);

		/* EHLO failure is not a real error */
		if (e->e_xfp != NULL &&
		    (SmtpReplyBuffer[0] == '4' ||
		     (SmtpReplyBuffer[0] == '5' && strncmp(SmtpMsgBuffer, "EHLO", 4) != 0)))
		{
			/* serious error -- log the previous command */
 			/* also record who we were talking before first error */
 			if (SmtpNeedIntro)
			{
				char SmtpServer[MAXLINE];

				/* Construct a meaningful introduction */
				strcpy(SmtpServer, CurHostName);
				if (ServerAddress != NULL && CurHostName[0] != '[')
				{
					(void) strcat(SmtpServer, " [");
					(void) strcat(SmtpServer, ServerAddress);
					(void) strcat(SmtpServer, "]");
				}
 				fprintf(e->e_xfp, "\nWhile connected to %s (%s):\n",
					SmtpServer, m->m_name);
				SmtpNeedIntro = FALSE;
			}
			if (SmtpMsgBuffer[0] != '\0')
				fprintf(e->e_xfp, ">>> %s\n", SmtpMsgBuffer);
			SmtpMsgBuffer[0] = '\0';

			/* now log the message as from the other side */
			fprintf(e->e_xfp, "<<< %s\n", SmtpReplyBuffer);
		}

		/* display the input for verbose mode */
		if (Verbose && !HoldErrs)
			nmessage("050 %s", SmtpReplyBuffer);

		if ((doingEHLO && (strncmp(SmtpReplyBuffer, "250", 3) == 0)
		        && ((SmtpReplyBuffer[3] == '-') || (doingEHLO == 2))
		        && SmtpReplyBuffer[4])) {
		    if (doingEHLO == 2) {
			*ehlo++ = newstr(SmtpReplyBuffer + 4);
			*ehlo = NULL;
			if (ehlo >= EHLOkeys + MAXEHLO)
			    doingEHLO = 0;
		    } else
			doingEHLO = 2;
		}

		/* if continuation is required, we can go on */
		if (SmtpReplyBuffer[3] == '-' || !isdigit(SmtpReplyBuffer[0]))
			continue;

		/* decode the reply code */
		r = atoi(SmtpReplyBuffer);

		/* extra semantics: 0xx codes are "informational" */
		if (r < 100)
			continue;

		/* reply code 421 is "Service Shutting Down" */
		if (r == SMTPCLOSING && SmtpState != SMTP_SSD)
		{
			/* send the quit protocol */
			SmtpState = SMTP_SSD;
			smtpquit(m, e);
		}

		/* save temporary failure messages for posterity */
		if (SmtpReplyBuffer[0] == '4' && SmtpError[0] == '\0')
			(void) strcpy(SmtpError, &SmtpReplyBuffer[4]);

		return (r);
	}
}
/*
**  SMTPMESSAGE -- send message to server
**
**	Parameters:
**		f -- format
**		m -- the mailer to control formatting.
**		a, b, c, d, e -- parameters
**
**	Returns:
**		none.
**
**	Side Effects:
**		writes message to SmtpOut.
*/

/*VARARGS1*/
void smtpmessage(f, m, a, b, c, d, e)
	char *f;
	MAILER *m;
{
	if (SmtpOut != NULL)
	{
		(void) sprintf(SmtpMsgBuffer, f, a, b, c, d, e);
		if (tTd(18, 1) || (Verbose && !HoldErrs))
			nmessage(">>> %s", SmtpMsgBuffer);
		fprintf(SmtpOut, "%s%s", SmtpMsgBuffer, m->m_eol);
	}
}
/* 
 * EHLOset - determines if an particular EHLO keyword is present
 */
char *EHLOset(s)
   char *s;
{
    int	    len = strlen (s);
    register char  *ep,
		 **ehlo;

    for (ehlo = EHLOkeys; ep = *ehlo; ehlo++)
	if (strncmp(ep, s, len) == 0) {
	    for (ep += len; *ep == ' '; ep++)
		continue;
	    return ep;
	}

    return NULL;
}

/* 
 * hdrsize - predicts the SMTP size of the headers of a message
 */
long hdrsize(m, e)
   struct mailer *m;
   ENVELOPE *e;
{
    int	    c;
    long    size = 0;
    char   *zf;
    FILE   *zp;

    if (!(zp = fopen(zf = queuename(e, 'z'), "w+"))) {
	syserr("unable to fopen %s for writing and reading", zf);
	return 0L;
    }

    (*e->e_puthdr)(zp, m, e);

    (void) fflush(zp);

    (void) fseek(zp, 0L, 0);
    for (size = 2L; (c = getc(zp)) != EOF; size++)
	if (c == '\n')
	    size++;

    (void) fclose(zp);
    (void) unlink(zf);

    return size;
}

/* 
 * bodysize - predicts the SMTP size of the body of a message
 */
long bodysize(e)
   ENVELOPE *e;
{
    int	    c;
    long    size = 0;

    if ((e->e_dfp == NULL) && (e->e_df))
	e->e_dfp = fopen(e->e_df, "r");

    if (e->e_dfp) {
	for (; (c = getc(e->e_dfp)) != EOF; size++)
	    if (c == '\n')
		size++;
	(void) fseek(e->e_dfp, 0L, 0);
    }

    return size;
  }

# endif /* SMTP */
