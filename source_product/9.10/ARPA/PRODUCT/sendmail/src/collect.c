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
static char rcsid[] = "$Header: collect.c,v 1.14.109.8 95/03/22 16:59:51 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)collect.c	5.9 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: collect.o $Revision: 1.14.109.8 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# include <errno.h>
# include "sendmail.h"

static void tferror();
static void eatfrom();
static bool flusheol();

/*
**  COLLECT -- read & parse message header & make temp file.
**
**	Creates a temporary file name and copies the standard
**	input to that file.  Leading UNIX-style "From" lines are
**	stripped off (after important information is extracted).
**
**	Parameters:
**		smtpmode -- if set, we are running SMTP: give an RFC821
**			style message to say we are ready to collect
**			input, and never ignore a single dot to mean
**			end of message.
**		requeueflag -- this message will be requeued later, so
**			don't do final processing on it.
**		e -- the current envelope.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Temp file is created and filled.
**		The from person may be set.
*/

void collect(smtpmode, requeueflag, e)
	bool smtpmode;
	bool requeueflag;
	register ENVELOPE *e;
{
	register FILE *tf;
	char buf[MAXFIELD], buf2[MAXFIELD];
	register char *workbuf, *freebuf;
	register int workbuflen;
	bool inputerr = FALSE;
	extern bool isheader();

	/*
	**  Create the temp file name and create the file.
	*/

	e->e_df = queuename(e, 'd');
	e->e_df = newstr(e->e_df);
	if ((tf = dfopen(e->e_df, "w")) == NULL)
	{
		syserr("Cannot create %s", e->e_df);
		NoReturn = TRUE;
		finis();
	}
	(void) chmod(e->e_df, FileMode);

	/*
	**  Tell ARPANET to go ahead.
	*/

	if (smtpmode)
		message("354 Enter mail, end with \".\" on a line by itself");

	/*
	**  Try to read a UNIX-style From line
	*/

	if (sfgets(buf, MAXFIELD, InChannel) == NULL)
		goto readerr;
	fixcrlf(buf, FALSE);
# ifndef NOTUNIX
	if (!SaveFrom && strncmp(buf, "From ", 5) == 0)
	{
		if (!flusheol(buf, InChannel))
			goto readerr;
		eatfrom(buf, e);
		if (sfgets(buf, MAXFIELD, InChannel) == NULL)
			goto readerr;
		fixcrlf(buf, FALSE);
	}
# endif /* NOTUNIX */

	/*
	**  Copy InChannel to temp file & do message editing.
	**	To keep certain mailers from getting confused,
	**	and to keep the output clean, lines that look
	**	like UNIX "From" lines are deleted in the header.
	*/

	workbuf = buf;		/* `workbuf' contains a header field */
	freebuf = buf2;		/* `freebuf' can be used for read-ahead */
	for (;;)
	{
		/* first, see if the header is over */
		if (!isheader(workbuf))
		{
			fixcrlf(workbuf, TRUE);
			break;
		}

		/* if the line is too long, throw the rest away */
		if (!flusheol(workbuf, InChannel))
			goto readerr;

		/* it's okay to toss '\n' now (flusheol() needed it) */
		fixcrlf(workbuf, TRUE);

		workbuflen = strlen(workbuf);

		/* get the rest of this field */
		for (;;)
		{
			if (sfgets(freebuf, MAXFIELD, InChannel) == NULL)
				goto readerr;

			/* is this a continuation line? */
			if (*freebuf != ' ' && *freebuf != '\t')
				break;

			if (!flusheol(freebuf, InChannel))
				goto readerr;

			/* yes; append line to `workbuf' if there's room */
			if (workbuflen < MAXFIELD-3)
			{
				register char *p = workbuf + workbuflen;
				register char *q = freebuf;

				/* we have room for more of this field */
				fixcrlf(freebuf, TRUE);
				*p++ = '\n'; workbuflen++;
				while(*q != '\0' && workbuflen < MAXFIELD-1)
				{
					*p++ = *q++;
					workbuflen++;
				}
				*p = '\0';
			}
		}

		e->e_msgsize += workbuflen;

		/*
		**  The working buffer now becomes the free buffer, since
		**  the free buffer contains a new header field.
		**
		**  This is premature, since we still havent called
		**  chompheader() to process the field we just created
		**  (so the call to chompheader() will use `freebuf').
		**  This convolution is necessary so that if we break out
		**  of the loop due to H_EOH, `workbuf' will always be
		**  the next unprocessed buffer.
		*/

		{
			register char *tmp = workbuf;
			workbuf = freebuf;
			freebuf = tmp;
		}

		/*
		**  Snarf header away.
		*/

		if (bitset(H_EOH, chompheader(freebuf, FALSE, e)))
			break;
	}

# ifdef DEBUG
	if (tTd(30, 1))
		printf("EOH\n");
# endif DEBUG

	if (*workbuf == '\0')
	{
		/* throw away a blank line */
		if (sfgets(buf, MAXFIELD, InChannel) == NULL)
			goto readerr;
	}
	else if (workbuf == buf2)	/* guarantee `buf' contains data */
		(void) strcpy(buf, buf2);

	/*
	**  Collect the body of the message.
	*/

	for (;;)
	{
		register char *bp = buf;

		fixcrlf(buf, TRUE);

		/* check for end-of-message */
		if (!IgnrDot && buf[0] == '.' && (buf[1] == '\n' || buf[1] == '\0'))
			break;

		/* check for transparent dot */
		if (OpMode == MD_SMTP && !IgnrDot && bp[0] == '.' && bp[1] == '.')
			bp++;

		/*
		**  Figure message length, output the line to the temp
		**  file, and insert a newline if missing.
		*/

		e->e_msgsize += strlen(bp) + 1;
		fputs(bp, tf);
		fputs("\n", tf);
		if (ferror(tf))
			tferror(tf, e);
		if (sfgets(buf, MAXFIELD, InChannel) == NULL)
			goto readerr;
	}

	if (feof(InChannel) || ferror(InChannel))
	{
readerr:
		if (tTd(30, 1))
			printf("collect: read error\n");
		inputerr = TRUE;
	}

	if (fflush(tf) != 0)
		tferror(tf, e);
	if (fsync(fileno(tf)) < 0 || fclose(tf) < 0)
	{
		tferror(tf, e);
		finis();
	}

	/* An EOF when running SMTP is an error */
	if (inputerr && OpMode == MD_SMTP)
	{
# ifdef LOG
		if (RealHostName != NULL && LogLevel > 0)
			syslog(LOG_NOTICE,
			    "collect: unexpected close on connection from %s: %m\n",
			    CurHostName);
# endif
		(feof(InChannel) ? usrerr : syserr)
			("Unexpected close during %s with %s, from=%s",
			 SmtpPhase, CurHostName, e->e_from.q_paddr);

		/* don't return an error indication */
		e->e_to = NULL;
		e->e_flags &= ~EF_FATALERRS;

		/* and don't try to deliver the partial message either */
		finis();
	}

	/*
	**  Find out some information from the headers.
	**	Examples are who is the from person & the date.
	*/

	eatheader(e, !requeueflag);

	/*
	**  Add an Apparently-To: line if we have no recipient lines.
	*/

	if (hvalue("to", e) == NULL && hvalue("cc", e) == NULL &&
	    hvalue("bcc", e) == NULL && hvalue("apparently-to", e) == NULL)
	{
		register ADDRESS *q;
		char ap_to_buf[MAXLINE];
		bool first_time = TRUE;

		bzero(ap_to_buf, sizeof(ap_to_buf));
		/* create an Apparently-To: field */
		/*    that or reject the message.... */
		for (q = e->e_sendqueue; q != NULL; q = q->q_next)
		{
			if (q->q_alias != NULL)
				continue;
			if (strlen(q->q_paddr) + 1 >= sizeof(ap_to_buf))
			{
				syserr("collect: recipient name too long");
				continue;
			}
			if ((strlen(ap_to_buf) + strlen(q->q_paddr)
				    + (first_time ? 1 : 3)) <= sizeof(ap_to_buf))
			{
				/*
				**	if the address fits, add it to the
				**	Apparently-To: line
				*/
				if (!first_time)
					strcat(ap_to_buf, ", ");
				strcat(ap_to_buf, q->q_paddr);
				first_time = FALSE;
			}
			else
			{
				/*
				**	if it doesn't fit, store the old one and
				**	start a new Apparently-To: line
				*/
# ifdef DEBUG
				if (tTd(30, 3))
					printf("Adding Apparently-To: %s\n", ap_to_buf);
# endif DEBUG
				addheader("apparently-to", ap_to_buf, e);
				bzero(ap_to_buf, sizeof(ap_to_buf));
				strcat(ap_to_buf, q->q_paddr);
				first_time = FALSE;
			}
		}
		if (*ap_to_buf != NULL)
		{
# ifdef DEBUG
			if (tTd(30, 3))
				printf("Adding Apparently-To: %s\n", ap_to_buf);
# endif DEBUG
			addheader("apparently-to", ap_to_buf, e);
		}
	}

	/* check for message too large */
	if (MaxMessageSize > 0 && e->e_msgsize > MaxMessageSize)
	{
		usrerr("552 Message exceeds maximum fixed size (%ld)",
			MaxMessageSize);
	}

	if ((e->e_dfp = fopen(e->e_df, "r")) == NULL)
	{
		/* we haven't acked receipt yet, so just chuck this */
		syserr("Cannot reopen %s", e->e_df);
		finis();
	}
}
/*
**  FLUSHEOL -- if not at EOL, throw away rest of input line.
**
**	Parameters:
**		buf -- last line read in (checked for '\n'),
**		fp -- file to be read from.
**
**	Returns:
**		FALSE on error from sfgets(), TRUE otherwise.
**
**	Side Effects:
**		none.
*/

static bool
flusheol(buf, fp)
	char *buf;
	FILE *fp;
{
	char junkbuf[MAXLINE], *sfgets();
	register char *p = buf;

	while (strchr(p, '\n') == NULL) {
		if (sfgets(junkbuf,MAXLINE,fp) == NULL)
			return(FALSE);
		p = junkbuf;
	}

	return (TRUE);
}
/*
**  TFERROR -- signal error on writing the temporary file.
**
**	Parameters:
**		tf -- the file pointer for the temporary file.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Gives an error message.
**		Arranges for following output to go elsewhere.
*/

static void tferror(tf, e)
	FILE *tf;
	register ENVELOPE *e;
{
	if (errno == ENOSPC)
	{
		(void) freopen(e->e_df, "w", tf);
		fputs("\nMAIL DELETED BECAUSE OF LACK OF DISK SPACE\n\n", tf);
		usrerr("452 Out of disk space for temp file");
	}
	else
		syserr("collect: Cannot write %s", e->e_df);
	(void) freopen("/dev/null", "w", tf);
}
/*
**  EATFROM -- chew up a UNIX style from line and process
**
**	This does indeed make some assumptions about the format
**	of UNIX messages.
**
**	Parameters:
**		fm -- the from line.
**
**	Returns:
**		none.
**
**	Side Effects:
**		extracts what information it can from the header,
**		such as the date.
*/

# ifndef NOTUNIX

char	*DowList[] =
{
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", NULL
};

char	*MonthList[] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
	NULL
};

static void eatfrom(fm, e)
	char *fm;
	register ENVELOPE *e;
{
	register char *p;
	register char **dt;

# 	ifdef DEBUG
	if (tTd(30, 2))
		printf("eatfrom(%s)\n", fm);
# 	 endif /* DEBUG */

	/* find the date part */
	p = fm;
	while (*p != '\0')
	{
		/* skip a word */
		while (*p != '\0' && *p != ' ')
			p++;
		while (*p == ' ')
			p++;
		if (!(isascii(*p) && isupper(*p)) ||
		    p[3] != ' ' || p[13] != ':' || p[16] != ':')
			continue;

		/* we have a possible date */
		for (dt = DowList; *dt != NULL; dt++)
			if (strncmp(*dt, p, 3) == 0)
				break;
		if (*dt == NULL)
			continue;

		for (dt = MonthList; *dt != NULL; dt++)
			if (strncmp(*dt, &p[4], 3) == 0)
				break;
		if (*dt != NULL)
			break;
	}

	if (*p != '\0')
	{
		char *q;
		extern char *arpadate();

		/* we have found a date */
		q = xalloc(25);
		(void) strncpy(q, p, 25);
		q[24] = '\0';
		define('d', q, e);
		q = arpadate(q);
		define('a', newstr(q), e);
	}
}

# endif /* NOTUNIX */
