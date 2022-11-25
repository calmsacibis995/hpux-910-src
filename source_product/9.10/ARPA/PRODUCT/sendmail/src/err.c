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
static char rcsid[] = "$Header: err.c,v 1.12.109.6 95/02/21 16:07:51 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)err.c	5.10 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: err.o $Revision: 1.12.109.6 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# include "sendmail.h"
# include <errno.h>
# include <netdb.h>

static void putmsg();
static void puterrmsg();

/*
**  SYSERR -- Print error message.
**
**	Prints an error message via printf to the diagnostic
**	output.  If LOG is defined, it logs it also.
**
**	If the first character of the syserr message is `!' it will
**	log this as an ALERT message and exit immediately.  This can
**	leave queue files in an indeterminate state, so it should not
**	be used lightly.
**
**	Parameters:
**		f -- the format string
**		a, b, c, d, e -- parameters
**
**	Returns:
**		none
**		Through TopFrame if QuickAbort is set.
**
**	Side Effects:
**		increments Errors.
**		sets ExitStat.
*/

# ifdef lint
int	sys_nerr;
char	*sys_errlist[];
#  endif /* lint */
char	MsgBuf[BUFSIZ+2];	/* text of most recent message */

static void	fmtmsg();

void
/*VARARGS1*/
syserr(va_alist)
	va_dcl
{
	register char *p;
	int olderrno = errno;
	bool panic;
	va_list args;
	char *fmt, msg1[BUFSIZ], *msg2;

	/* format and output the error message */
	if (olderrno == 0)
		p = "554";
	else
		p = "451";
	va_start(args);
	fmt = va_arg(args, char *);
	(void) vsprintf(msg1, fmt, args);

	msg2 = &msg1[0];
	panic = *msg2 == '!';
	if (panic)
		msg2++;

	fmtmsg(MsgBuf, (char *) NULL, p, olderrno, msg2);
	va_end(args);
	puterrmsg(MsgBuf);

	/* determine exit status if not already set */
	if (ExitStat == EX_OK)
	{
		if (olderrno == 0)
			ExitStat = EX_SOFTWARE;
		else
			ExitStat = EX_OSERR;
	}

# ifdef LOG
	if (LogLevel > 0)
		syslog(panic ? LOG_ALERT : LOG_CRIT, "%s: SYSERR: %s",
			CurEnv->e_id == NULL ? "NOQUEUE" : CurEnv->e_id,
			&MsgBuf[4]);
#  endif /* LOG */
	errno = 0;
	if (QuickAbort)
		longjmp(TopFrame, 2);
}
/*
**  USRERR -- Signal user error.
**
**	This is much like syserr except it is for user errors.
**
**	Parameters:
**		fmt, a, b, c, d, e -- printf strings
**
**	Returns:
**		none
**		Through TopFrame if QuickAbort is set.
**
**	Side Effects:
**		increments Errors.
*/

/*VARARGS1*/
void
usrerr(va_alist)
	va_dcl
{
	extern int errno;
	va_list args;
	char *fmt, msg[BUFSIZ];

	if (SuprErrs)
		return;

	va_start(args);
	fmt = va_arg(args, char *);
	(void) vsprintf(msg, fmt, args);
	fmtmsg(MsgBuf, CurEnv->e_to, "501", errno, msg);
	va_end(args);
	puterrmsg(MsgBuf);

	if (QuickAbort)
		longjmp(TopFrame, 1);
}
/*
**  MESSAGE -- print message (not necessarily an error)
**
**	Parameters:
**		msg -- the message (printf fmt) -- it can begin with
**			an SMTP reply code.  If not, 050 is assumed.
**		a, b, c, d, e -- printf arguments
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
*/

/*VARARGS2*/
void message(va_alist)
	va_dcl
{
	va_list args;
	char *fmt, msg[BUFSIZ];

	errno = 0;
	va_start(args);
	fmt = va_arg(args, char *);
	(void) vsprintf(msg, fmt, args);
	fmtmsg(MsgBuf, CurEnv->e_to, "050", 0, msg);
	va_end(args);
	putmsg(MsgBuf, FALSE);
}
/*
**  NMESSAGE -- print message (not necessarily an error)
**
**	Just like "message" except it never puts the to... tag on.
**
**	Parameters:
**		num -- the default ARPANET error number (in ascii)
**		msg -- the message (printf fmt) -- if it begins
**			with three digits, this number overrides num.
**		a, b, c, d, e -- printf arguments
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
*/

/*VARARGS2*/
void nmessage(va_alist)
	va_dcl
{
	va_list args;
	char *fmt, msg[BUFSIZ];

	errno = 0;
	va_start(args);
	fmt = va_arg(args, char *);
	(void) vsprintf(msg, fmt, args);
	fmtmsg(MsgBuf, (char *) NULL, "050", 0, msg);
	va_end(args);
	putmsg(MsgBuf, FALSE);
}
/*
**  PUTMSG -- output error message to transcript and channel
**
**	Parameters:
**		msg -- message to output (in SMTP format).
**		holdmsg -- if TRUE, don't output a copy of the message to
**			our output channel.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Outputs msg to the transcript.
**		If appropriate, outputs it to the channel.
**		Deletes SMTP reply code number as appropriate.
*/

static void putmsg(msg, holdmsg)
	char *msg;
	bool holdmsg;
{
	/* output to transcript if serious */
	if (CurEnv->e_xfp != NULL && (msg[0] == '4' || msg[0] == '5'))
		fprintf(CurEnv->e_xfp, "%s\n", msg);

	/* output to channel if appropriate */
	if (!holdmsg && (Verbose || msg[0] != '0'))
	{
		(void) fflush(stdout);
		if (OpMode == MD_SMTP || OpMode == MD_ARPAFTP)
			fprintf(OutChannel, "%s\r\n", msg);
		else
			fprintf(OutChannel, "%s\n", &msg[4]);
		(void) fflush(OutChannel);
	}
}
/*
**  PUTERRMSG -- like putmsg, but does special processing for error messages
**
**	Parameters:
**		msg -- the message to output.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets the fatal error bit in the envelope as appropriate.
*/

static void puterrmsg(msg)
	char *msg;
{
	/* output the message as usual */
	putmsg(msg, HoldErrs);

	/* signal the error */
	Errors++;
	if (msg[0] == '5')
		CurEnv->e_flags |= EF_FATALERRS;
}
/*
**  FMTMSG -- format a message into buffer.
**
**	Parameters:
**		eb -- error buffer to get result.
**		to -- the recipient tag for this message.
**		num -- arpanet error number.
**		eno -- the error number to display.
**		fmt -- format of string.
**		a, b, c, d, e -- arguments.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

/*VARARGS5*/
static void
fmtmsg(eb, to, num, eno, msg)
	register char *eb;
	char *to;
	char *num;
	int eno;
	char *msg;
{
	char del;
	char *meb;

	/* output the reply code */
	if (isdigit(msg[0]) && isdigit(msg[1]) && isdigit(msg[2]))
	{
		num = msg;
		msg += 4;
	}
	if (num[3] == '-')
		del = '-';
	else
		del = ' ';
	(void) sprintf(eb, "%3.3s%c", num, del);
	eb += 4;

	/* output the file name and line number */
	if (FileName != NULL)
	{
		(void) sprintf(eb, "%s: line %d: ", FileName, LineNumber);
		eb += strlen(eb);
	}

	/* output the "to" person */
	if (to != NULL && to[0] != '\0')
	{
		(void) sprintf(eb, "%s... ", to);
		while (*eb != '\0')
			*eb++ &= 0177;
	}

	meb = eb;

	/* output the message */
	(void) strcpy(eb, msg);
	while (*eb != '\0')
		*eb++ &= 0177;

	/* output the error code, if any */
	if (eno != 0)
	{
		(void) sprintf(eb, ": %s", errstring(eno));
		eb += strlen(eb);
	}

	if (num[0] == '5' || (CurEnv->e_message == NULL && num[0] == '4'))
	{
		if (CurEnv->e_message != NULL)
			free(CurEnv->e_message);
		CurEnv->e_message = newstr(meb);
	}
}
/*
**  ERRSTRING -- return string description of error code
**
**	Parameters:
**		errno -- the error number to translate
**
**	Returns:
**		A string description of errno.
**
**	Side Effects:
**		none.
*/

char *
errstring(errno)
	int errno;
{
	extern char *sys_errlist[];
	extern int sys_nerr;
	static char buf[100];
# ifdef SMTP
	extern char *SmtpPhase;
# endif /* SMTP */

# ifdef DAEMON
	/*
	**  Handle special network error codes.
	**
	**	These are 4.2/4.3bsd specific; they should be in daemon.c.
	*/

	switch (errno)
	{
	  case ETIMEDOUT:
	  case ECONNRESET:
		(void) strcpy(buf, sys_errlist[errno]);
		if (SmtpPhase != NULL)
		{
			(void) strcat(buf, " during ");
			(void) strcat(buf, SmtpPhase);
		}
		if (CurHostName != NULL)
		{
			(void) strcat(buf, " with ");
			(void) strcat(buf, CurHostName);
		}
		return (buf);

	  case EHOSTDOWN:
		if (CurHostName == NULL)
			break;
		(void) sprintf(buf, "Host %s is down", CurHostName);
		return (buf);

	  case ECONNREFUSED:
		if (CurHostName == NULL)
			break;
		(void) sprintf(buf, "Connection refused by %s", CurHostName);
		return (buf);

	  case (TRY_AGAIN+MAX_ERRNO):
		(void) sprintf(buf, "Host Name Lookup Failure");
		return (buf);
	}
#  endif /* DAEMON */

	if (errno > 0 && errno < sys_nerr)
		return (sys_errlist[errno]);

	(void) sprintf(buf, "Error %d", errno);
	return (buf);
}
