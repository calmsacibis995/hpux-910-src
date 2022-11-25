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
static char rcsid[] = "$Header: headers.c,v 1.16.109.7 95/02/21 16:07:58 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)headers.c	5.15 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_3997="@(#) PATCH_9.03: headers.o $Revision: 1.16.109.7 $ 94/03/24 PHNE_3997";
# endif	/* PATCH_STRING */

# ifndef hpux
# 	include <sys/param.h>
# endif	/* not hpux */
# include <errno.h>
# include "sendmail.h"

/*
**  CHOMPHEADER -- process and save a header line.
**
**	Called by collect and by readcf to deal with header lines.
**
**	Parameters:
**		line -- header as a text line.
**		def -- if set, this is a default value.
**		e -- the envelope including this header.
**
**	Returns:
**		flags for this header.
**
**	Side Effects:
**		The header is saved on the header list.
**		Contents of 'line' are destroyed.
*/

chompheader(line, def, e)
	char *line;
	bool def;
	register ENVELOPE *e;
{
	register char *p;
	register HDR *h;
	HDR **hp;
	char *fname;
	char *fvalue;
	struct hdrinfo *hi;
	bool cond = FALSE;
	BITMAP mopts;
	extern char *crackaddr();

# ifdef DEBUG
	if (tTd(31, 6))
		printf("chompheader: %s\n", line);
#  endif /* DEBUG */

	/* strip off options */
	clrbitmap(mopts);
	p = line;
	if (*p == '?')
	{
		/* have some */
		register char *q = strchr(p + 1, *p);
		
		if (q != NULL)
		{
			*q++ = '\0';
			while (*++p != '\0')
				setbitn(*p, mopts);
			p = q;
		}
		else
			usrerr("553 header syntax error, line \"%s\"", line);
		cond = TRUE;
	}

	/* find canonical name */
	fname = p;
	p = strchr(p, ':');
	if (p == NULL)
	{
		syserr("553 header syntax error, line \"%s\"", line);
		return (0);
	}
	fvalue = &p[1];
	while (isspace(*--p))
		continue;
	*++p = '\0';
	makelower(fname);

	/* strip field value on front */
	if (*fvalue == ' ')
		fvalue++;

	/* see if it is a known type */
	for (hi = HdrInfo; hi->hi_field != NULL; hi++)
	{
		if (strcmp(hi->hi_field, fname) == 0)
			break;
	}

	/* see if this is a resent message */
	if (!def && bitset(H_RESENT, hi->hi_flags))
		e->e_flags |= EF_RESENT;

	/* if this means "end of header" quit now */
	if (bitset(H_EOH, hi->hi_flags))
		return (hi->hi_flags);

	/*
	**  Drop explicit From: if same as what we would generate.
	**  This is to make MH (which doesn't always give a full name)
	**  insert the full name information in all circumstances.
	*/

	p = "resent-from";
	if (!bitset(EF_RESENT, e->e_flags))
		p += 7;
	if (!def && !QueueRun && strcmp(fname, p) == 0)
	{
		if (e->e_from.q_paddr != NULL &&
		    strcmp(fvalue, e->e_from.q_paddr) == 0)
			return (hi->hi_flags);
	}

	/* delete default value for this header */
	for (hp = &e->e_header; (h = *hp) != NULL; hp = &h->h_link)
	{
		if (strcmp(fname, h->h_field) == 0 &&
		    bitset(H_DEFAULT, h->h_flags) &&
		    !bitset(H_FORCE, h->h_flags))
		{
			h->h_value = NULL;

			/* header in input gets this header's default mflags */
			bcopy((char *) h->h_mflags, (char *) mopts, sizeof mopts);
		}
	}

	/* create a new node */
	h = (HDR *) xalloc(sizeof *h);
	h->h_field = newstr(fname);
	h->h_value = NULL;
	h->h_link = NULL;
	bcopy((char *) mopts, (char *) h->h_mflags, sizeof mopts);
	*hp = h;
	h->h_flags = hi->hi_flags;
	if (def)
		h->h_flags |= H_DEFAULT;
	if (cond)
		h->h_flags |= H_CHECK;
	if (h->h_value != NULL)
		free((char *) h->h_value);
	h->h_value = newstr(fvalue);

	/* hack to see if this is a new format message */
	if (!def && bitset(H_RCPT|H_FROM, h->h_flags) &&
	    (strchr(fvalue, ',') != NULL || strchr(fvalue, '(') != NULL ||
	     strchr(fvalue, '<') != NULL || strchr(fvalue, ';') != NULL))
	{
		e->e_flags &= ~EF_OLDSTYLE;
	}

	return (h->h_flags);
}
/*
**  ADDHEADER -- add a header entry to the end of the queue.
**
**	This bypasses the special checking of chompheader.
**
**	Parameters:
**		field -- the name of the header field.
**		value -- the value of the field.  It must be lower-cased.
**		e -- the envelope to add them to.
**
**	Returns:
**		none.
**
**	Side Effects:
**		adds the field on the list of headers for this envelope.
*/

void addheader(field, value, e)
	char *field;
	char *value;
	ENVELOPE *e;
{
	register HDR *h;
	register struct hdrinfo *hi;
	HDR **hp;

	/* find info struct */
	for (hi = HdrInfo; hi->hi_field != NULL; hi++)
	{
		if (strcmp(field, hi->hi_field) == 0)
			break;
	}

	/* find current place in list -- keep back pointer? */
	for (hp = &e->e_header; (h = *hp) != NULL; hp = &h->h_link)
	{
		if (strcmp(field, h->h_field) == 0)
			break;
	}

	/* allocate space for new header */
	h = (HDR *) xalloc(sizeof *h);
	h->h_field = field;
	h->h_value = newstr(value);
	h->h_link = *hp;
	h->h_flags = hi->hi_flags | H_DEFAULT;
	clrbitmap(h->h_mflags);
	*hp = h;
}
/*
**  HVALUE -- return value of a header.
**
**	Only "real" fields (i.e., ones that have not been supplied
**	as a default) are used.
**
**	Parameters:
**		field -- the field name.
**		e -- the envelope containing the header.
**
**	Returns:
**		pointer to the value part.
**		NULL if not found.
**
**	Side Effects:
**		none.
*/

char *
hvalue(field, e)
	char *field;
	register ENVELOPE *e;
{
	register HDR *h;

	for (h = e->e_header; h != NULL; h = h->h_link)
	{
		if (!bitset(H_DEFAULT, h->h_flags) &&
		    strcmp(h->h_field, field) == 0)
			return (h->h_value);
	}
	return (NULL);
}
/*
**  ISHEADER -- predicate telling if argument is a header.
**
**	A line is a header if it has a single word followed by
**	optional white space followed by a colon.
**
**	Parameters:
**		s -- string to check for possible headerness.
**
**	Returns:
**		TRUE if s is a header.
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
isheader(s)
	register char *s;
{
	while (*s > ' ' && *s != ':' && *s != '\0')
		s++;

	/* following technically violates RFC822 */
	while (isascii(*s) && isspace(*s))
		s++;

	return (*s == ':');
}
/*
**  EATHEADER -- run through the stored header and extract info.
**
**	Parameters:
**		e -- the envelope to process.
**		full -- if set, do full processing (e.g., compute
**			message priority).
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets a bunch of global variables from information
**			in the collected header.
**		Aborts the message if the hop count is exceeded.
*/

void eatheader(e, full)
	register ENVELOPE *e;
	bool full;
{
	register HDR *h;
	register char *p;
	int hopcnt = 0;
	char *msgid;
	char buf[MAXLINE];

	/*
	**  Set up macros for possible expansion in headers.
	*/

	define('f', e->e_sender, e);
	define('g', e->e_sender, e);

	/* full name of from person */
	p = hvalue("full-name", e);
	if (p != NULL)
		define('x', p, e);

# ifdef DEBUG
	if (tTd(32, 1))
		printf("----- collected header -----\n");
#  endif /* DEBUG */
	msgid = "<none>";
	for (h = e->e_header; h != NULL; h = h->h_link)
	{
# ifdef DEBUG
		extern char *capitalize();

		if (h->h_value == NULL)
		{
			if (tTd(32, 1))
				printf("%s: <NULL>\n", capitalize(h->h_field));
			continue;
		}
		if (tTd(32, 1))
			printf("%s: %s\n", capitalize(h->h_field), h->h_value);
#  endif /* DEBUG */
		/* do early binding */
		if (bitset(H_DEFAULT, h->h_flags))
		{
			expand(h->h_value, buf, &buf[sizeof buf], e);
			if (buf[0] != '\0')
			{
				h->h_value = newstr(buf);
				h->h_flags &= ~H_DEFAULT;
			}
		}

# ifdef DEBUG
		if (tTd(32, 1))
		{
			printf("%s: ", h->h_field);
			xputs(h->h_value);
			printf("\n");
		}
#  endif /* DEBUG */

		/* count the number of times it has been processed */
		if (bitset(H_TRACE, h->h_flags))
			hopcnt++;

		/* send to this person if we so desire */
		if (GrabTo && bitset(H_RCPT, h->h_flags) &&
		    !bitset(H_DEFAULT, h->h_flags) && !QueueRun &&
		    (!bitset(EF_RESENT, e->e_flags) || bitset(H_RESENT, h->h_flags)))
		{
			sendtolist(h->h_value, (ADDRESS *) NULL, &e->e_sendqueue, e);
		}

		/* save the message-id for logging */
		if (full && strcmp(h->h_field, "message-id") == 0)
		{
			msgid = h->h_value;
			while (isascii(*msgid) && isspace(*msgid))
				msgid++;
		}

	}
# ifdef DEBUG
	if (tTd(32, 1))
		printf("----------------------------\n");
#  endif /* DEBUG */

	/* store hop count */
	if (hopcnt > e->e_hopcount)
		e->e_hopcount = hopcnt;

	/* message priority */
	p = hvalue("precedence", e);
	if (p != NULL)
		e->e_class = priencode(p);
	if (!QueueRun)
		e->e_msgpriority = e->e_msgsize
				 - e->e_class * WkClassFact
				 + e->e_nrcpts * WkRecipFact;

	/* return receipt to */
	p = hvalue("return-receipt-to", e);
	if (p != NULL)
		e->e_receiptto = p;

	/* errors to */
	p = hvalue("errors-to", e);
	if ((p != NULL) && !QueueRun)
		sendtolist(p, (ADDRESS *) NULL, &e->e_errorqueue, e);

	/* from person */
	if (OpMode == MD_ARPAFTP)
	{
		register struct hdrinfo *hi = HdrInfo;

		for (p = NULL; p == NULL && hi->hi_field != NULL; hi++)
		{
			if (bitset(H_FROM, hi->hi_flags))
				p = hvalue(hi->hi_field, e);
		}
		if (p != NULL)
			setsender(p, e);
	}

	/* full name of from person */
	p = hvalue("full-name", e);
	if (p != NULL)
		define('x', p, e);

	/* date message originated */
	p = hvalue("posted-date", e);
	if (p == NULL)
		p = hvalue("date", e);
	if (p != NULL)
	{
		define('a', p, e);
		/* we don't have a good way to do canonical conversion ....
		define('d', newstr(arpatounix(p)), e);
		.... so we will ignore the problem for the time being */
	}

	/*
	**  Log collection information.
	*/

# ifdef LOG
	if (full && LogLevel > 4)
		logsender(e, msgid);
# endif /* LOG */
	e->e_flags &= ~EF_LOGSENDER;
}
/*
**  LOGSENDER -- log sender information
**
**	Parameters:
**		e -- the envelope to log
**		msgid -- the message id
**
**	Returns:
**		none
*/

void logsender(e, msgid)
	register ENVELOPE *e;
	char *msgid;
{
	char *name;
	register char *sbp;
	register char *p;
	char hbuf[MAXNAME];
	char sbuf[MAXLINE];

	if (bitset(EF_RESPONSE, e->e_flags))
		name = "[RESPONSE]";
	else if (RealHostName == NULL)
		name = "local host";
	else if (RealHostName[0] == '[')
		name = RealHostName;
	else
	{
		name = hbuf;
		(void)sprintf(hbuf, "%.80s [%s]", RealHostName,
			      inet_ntoa(RealHostAddr.sin_addr));
	}

	syslog(LOG_INFO, "%s: from=%s",
		e->e_id, shortenstring(e->e_from.q_paddr, 83));
	syslog(LOG_INFO, "%s: size=%ld, class=%ld, pri=%ld, nrcpts=%d",
		e->e_id, e->e_msgsize, e->e_class,
		e->e_msgpriority, e->e_nrcpts);
	if (msgid != NULL)
		syslog(LOG_INFO, "%s: msgid=%s", e->e_id, msgid);
	sbp = sbuf;
	sprintf(sbp, "%s:", e->e_id);
	sbp += strlen(sbp);
	if (e->e_bodytype != NULL)
	{
		sprintf(sbp, " bodytype=%s,", e->e_bodytype);
		sbp += strlen(sbp);
	}
	p = macvalue('r', e);
	if (p != NULL)
	{
		sprintf(sbp, " proto=%s,", p);
		sbp += strlen(sbp);
	}
	syslog(LOG_INFO, "%s relay=%s", sbuf, name);
}
/*
**  PRIENCODE -- encode external priority names into internal values.
**
**	Parameters:
**		p -- priority in ascii.
**
**	Returns:
**		priority as a numeric level.
**
**	Side Effects:
**		none.
*/

priencode(p)
	char *p;
{
	register int i;

	for (i = 0; i < NumPriorities; i++)
	{
		if (!strcasecmp(p, Priorities[i].pri_name))
			return (Priorities[i].pri_val);
	}

	/* unknown priority */
	return (0);
}
/*
**  CRACKADDR -- parse an address and turn it into a macro
**
**	This doesn't actually parse the address -- it just extracts
**	it and replaces it with "$g".  The parse is totally ad hoc
**	and isn't even guaranteed to leave something syntactically
**	identical to what it started with.  However, it does leave
**	something semantically identical.
**
**	The process is kind of strange.  There are a number of
**	interesting cases:
**		1.  comment <address> comment	==> comment <$g> comment
**		2.  address			==> address
**		3.  address (comment)		==> $g (comment)
**		4.  (comment) address		==> (comment) $g
**	And then there are the hard cases....
**		5.  add (comment) ress		==> $g (comment)
**		6.  comment <address (comment)>	==> comment <$g (comment)>
**		7.    .... etc ....
**
**	Parameters:
**		addr -- the address to be cracked.
**
**	Returns:
**		a pointer to the new version.
**
**	Side Effects:
**		none.
**
**	Warning:
**		The return value is saved in local storage and should
**		be copied if it is to be reused.
*/

char *
crackaddr(addr)
	register char *addr;
{
	register char *p;
	register int i;
	static char buf[MAXNAME];
	char *rhs;
	bool gotaddr;
	register char *bp;

# ifdef DEBUG
	if (tTd(33, 1))
		printf("crackaddr(%s)\n", addr);
#  endif /* DEBUG */

	(void) strcpy(buf, "");
	rhs = (char *) -1;

	/* strip leading spaces */
	while (*addr != '\0' && isascii(*addr) && isspace(*addr))
		addr++;

	/*
	**  See if we have anything in angle brackets.  If so, that is
	**  the address part, and the rest is the comment.
	*/

	p = strchr(addr, '<');
	if (p != NULL)
	{
		/* copy the beginning of the addr field to the buffer */
		*p = '\0';
		strcpy(buf, addr);

		/*  do not copy angle brackets to the buffer */
		/* strcat(buf, "<"); */
		*p++ = '<';

		/* skip spaces */
		while (isspace(*p))
			p++;

		/* find the matching right angle bracket */
		addr = p;
		for (i = 0; *p != '\0'; p++)
		{
			switch (*p)
			{
			  case '<':
				i++;
				break;

			  case '>':
				i--;
				break;
			}
			if (i < 0)
				break;
		}

		/* p now points to the closing '>' or null */
		if (*p != '\0')
		{
			/* make rhs point to the '>' and the extra stuff at the end */
			rhs = p;
			*p = '\0';
		}
	}

	/*
	**  Now parse the real address part.  "addr" points to the (null
	**  terminated) version of what we are interested in; rhs points
	**  to the extra stuff at the end of the line, if any.
	*/

	p = addr;

	/* now strip out comments */
	bp = &buf[strlen(buf)];
	gotaddr = FALSE;
	for (; *p != '\0'; p++)
	{
		if (*p == '(')
		{
			/* copy to matching close paren */
			*bp++ = *p++;
			for (i = 0; *p != '\0'; p++)
			{
				*bp++ = *p;
				switch (*p)
				{
				  case '(':
					i++;
					break;

				  case ')':
					i--;
					break;
				}
				if (i < 0)
					break;
			}
			continue;
		}

		/*
		**  If this is the first "real" character we have seen,
		**  then we put the "$g" in the buffer now.
		*/

		if (isspace(*p))
			*bp++ = *p;
		else if (!gotaddr)
		{
			(void) strcpy(bp, "\001g");
			bp += 2;
			gotaddr = TRUE;
		}
	}

	/* null addr case */
	if (!gotaddr)
	{
		strcpy(bp, "\001g");
		bp += 2;
		gotaddr = TRUE;
	}

	/* hack, hack.... strip trailing blanks */
	do
	{
		*bp-- = '\0';
	} while (isspace(*bp));
	bp++;

	/* put any right hand side back on */
	if (rhs != (char *) -1)
	{
		/*  restore right angle bracket to addr but NOT to buffer */
		/* *rhs = '>'; */
		*rhs++ = '>';
		strcpy(bp, rhs);
	}

# ifdef DEBUG
	if (tTd(33, 1))
		printf("crackaddr=>`%s'\n", buf);
#  endif /* DEBUG */

	return (buf);
}
/*
**  PUTHEADERLINE -- put an individual header line
**
**	Parameters:
**		fp -- file to put it on.
**		m -- mailer to use.
**		field -- header field
**		value -- value of that header field
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

void putheaderline(fp, m, field, value)
	register FILE *fp;
	register MAILER *m;
	register char *field;
	register char *value;
{
	extern char *capitalize();
	char obuf[MAX(MAXFIELD,MAXLINE)];
	register char *nlp;

	(void) sprintf(obuf, "%s: ", capitalize(field));
	while ((nlp = strchr(value, '\n')) != NULL)
	{
		*nlp = '\0';
		(void) strcat(obuf, value);
		*nlp = '\n';
		putline(obuf, fp, m);
		value = ++nlp;
		obuf[0] = '\0';
	}
	(void) strcat(obuf, value);
	putline(obuf, fp, m);
}
/*
**  PUTHEADER -- put the header part of a message from the in-core copy
**
**	Parameters:
**		fp -- file to put it on.
**		m -- mailer to use.
**		e -- envelope to use.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

putheader(fp, m, e)
	register FILE *fp;
	register MAILER *m;
	register ENVELOPE *e;
{
	char buf[MAX(MAXFIELD,BUFSIZ)];
	register HDR *h;
	extern char *arpadate();
	extern char *capitalize();
	char obuf[MAX(MAXFIELD,MAXLINE)];

	for (h = e->e_header; h != NULL; h = h->h_link)
	{
		register char *p;
		extern bool bitintersect();

		if (bitset(H_CHECK|H_ACHECK, h->h_flags) &&
		    !bitintersect(h->h_mflags, m->m_flags))
			continue;

		/* handle Resent-... headers specially */
		if (bitset(H_RESENT, h->h_flags) && !bitset(EF_RESENT, e->e_flags))
			continue;

		p = h->h_value;
		if (bitset(H_DEFAULT, h->h_flags))
		{
			/* macro expand value if generated internally */
			expand(p, buf, &buf[sizeof buf], e);
			p = buf;
			if (p == NULL || *p == '\0')
				continue;
		}

		if (bitset(H_FROM|H_RCPT, h->h_flags))
		{
			/* address field */
			bool oldstyle = bitset(EF_OLDSTYLE, e->e_flags);

			if (bitset(H_FROM, h->h_flags))
				oldstyle = FALSE;
			commaize(h, p, fp, oldstyle, m, e);
		}
		else
		{
			/* vanilla header line */
			putheaderline(fp, m, h->h_field, p);
		}
	}
}
/*
**  COMMAIZE -- output a header field, making a comma-translated list.
**
**	Parameters:
**		h -- the header field to output.
**		p -- the value to put in it.
**		fp -- file to put it to.
**		oldstyle -- TRUE if this is an old style header.
**		m -- a pointer to the mailer descriptor.  If NULL,
**			don't transform the name at all.
**		e -- the envelope containing the message.
**
**	Returns:
**		none.
**
**	Side Effects:
**		outputs "p" to file "fp".
*/

void commaize(h, p, fp, oldstyle, m, e)
	register HDR *h;
	register char *p;
	FILE *fp;
	bool oldstyle;
	register MAILER *m;
	register ENVELOPE *e;
{
	register char *obp;
	int opos;
	bool firstone = TRUE;
	char obuf[MAXLINE + 3];

	/*
	**  Output the address list translated by the
	**  mailer and with commas.
	*/

# ifdef DEBUG
	if (tTd(14, 2))
		printf("commaize(%s: %s)\n", h->h_field, p);
#  endif /* DEBUG */

	obp = obuf;
	(void) sprintf(obp, "%s: ", capitalize(h->h_field));
	opos = strlen(h->h_field) + 2;
	obp += opos;

	/*
	**  Run through the list of values.
	*/

	while (*p != '\0')
	{
		register char *name;
		char savechar;
		extern char *remotename();
		extern char *DelimChar;		/* defined in prescan */

		/*
		**  Find the end of the name.  New style names
		**  end with a comma, old style names end with
		**  a space character.  However, spaces do not
		**  necessarily delimit an old-style name -- at
		**  signs mean keep going.
		*/

		/* find end of name */
		while ((isascii(*p) && isspace(*p)) || *p == ',')
			p++;
		name = p;
		for (;;)
		{
			auto char *oldp;
			char pvpbuf[PSBUFSIZE];

			(void) prescan(p, oldstyle ? ' ' : ',', pvpbuf);
			p = DelimChar;

			/* look to see if we have an at sign */
			oldp = p;
			while (*p != '\0' && isascii(*p) && isspace(*p))
				p++;

			if (*p != '@')
			{
				p = oldp;
				break;
			}
			p += *p == '@' ? 1 : 2;
			while (*p != '\0' && isascii(*p) && isspace(*p))
				p++;
		}
		/* at the end of one complete name */

		/* strip off trailing white space */
		while (p >= name &&
		       ((isascii(*p) && isspace(*p)) || *p == ',' || *p == '\0'))
			p--;
		if (++p == name)
			continue;
		savechar = *p;
		*p = '\0';

		/* translate the name to be relative */
		name = remotename(name, m, bitset(H_FROM, h->h_flags), FALSE, e);
		if (*name == '\0')
		{
			*p = savechar;
			continue;
		}

		/* output the name with nice formatting */
		opos += strlen(name);
		if (!firstone)
			opos += 2;
		if (opos > 78 && !firstone)
		{
			(void) strcpy(obp, ",\n");
			putline(obuf, fp, m);
			obp = obuf;
			(void) sprintf(obp, "        ");
			opos = strlen(obp);
			obp += opos;
			opos += strlen(name);
		}
		else if (!firstone)
		{
			(void) sprintf(obp, ", ");
			obp += 2;
		}

		/* strip off quote bits as we output */
		while (*name != '\0' && obp < &obuf[MAXLINE])
		{
			if (bitset(0200, *name))
				*obp++ = '\\';
			*obp++ = *name++ & ~0200;
		}
		firstone = FALSE;
		*p = savechar;
	}
	(void) strcpy(obp, "\n");
	putline(obuf, fp, m);
}
