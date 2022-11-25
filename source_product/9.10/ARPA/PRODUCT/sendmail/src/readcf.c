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
static char rcsid[] = "$Header: readcf.c,v 1.23.109.8 95/03/22 17:39:41 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)readcf.c	5.21 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: readcf.o $Revision: 1.23.109.8 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# include "sendmail.h"
# include <pwd.h>
# include <grp.h>
# include <arpa/nameser.h>
# include <netdb.h>

static void toomany();
static void fileclass();
static void makemailer();

/*
**  READCF -- read control file.
**
**	This routine reads the control file and builds the internal
**	form.
**
**	The file is formatted as a sequence of lines, each taken
**	atomically.  The first character of each line describes how
**	the line is to be interpreted.  The lines are:
**		Dxval		Define macro x to have value val.
**		Cxword		Put word into class x.
**		Fxfile [fmt]	Read file for lines to put into
**				class x.  Use scanf string 'fmt'
**				or "%s" if not present.  Fmt should
**				only produce one string-valued result.
**		Hname: value	Define header with field-name 'name'
**				and value as specified; this will be
**				macro expanded immediately before
**				use.
**		Sn		Use rewriting set n.
**		Rlhs rhs	Rewrite addresses that match lhs to
**				be rhs.
**		Mn p f s r a	Define mailer.  n - internal name,
**				p - pathname, f - flags, s - rewriting
**				ruleset for sender, s - rewriting ruleset
**				for recipients, a - argument vector.
**		Oxvalue		Set option x to value.
**		Pname=value	Set precedence name to value.
**
**	Parameters:
**		cfname -- control file name.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Builds several internal tables.
*/

void readcf(cfname, e)
	char *cfname;
	register ENVELOPE *e;
{
	FILE *cf;
	int i, ruleset = -1;
	char *q;
	char **pv;
	struct rewrite *rwp = NULL;
	char buf[MAXLINE];
	register char *p;
	char exbuf[MAXLINE];
	char pvpbuf[PSBUFSIZE];
	extern char *fgetfolded();
	extern char *munchstring();

	cf = fopen(cfname, "r");
	if (cf == NULL)
	{
		syserr("Cannot open %s", cfname);
		exit(EX_OSFILE);
	}

	for (i = 0; i < MAXRWSETS; i++)
		RewriteRules[i] = NULL;

	FileName = cfname;
	LineNumber = 0;
	while (fgetfolded(buf, sizeof buf, cf) != NULL)
	{
		/* map $ into \001 (ASCII SOH) for macro expansion */
		for (p = buf; *p != '\0'; p++)
		{
			if (*p != '$')
				continue;

			if (p[1] == '$')
			{
				/* actual dollar sign.... */
				(void) strcpy(p, p + 1);
				continue;
			}

			/* convert to macro expansion character */
			*p = '\001';
		}

		/* interpret this line */
		switch (buf[0])
		{
		  case '\0':
		  case '#':		/* comment */
			break;

		  case 'R':		/* rewriting rule */
			if (ruleset == -1)
			{
				syserr("No ruleset currently defined");
				break;
			}
			for (p = &buf[1]; *p != '\0' && *p != '\t'; p++)
				continue;

			if (*p == '\0')
			{
				syserr("Invalid rewrite line \"%s\"", buf);
				break;
			}

			/* allocate space for the rule header */
			if (rwp == NULL)
			{
				RewriteRules[ruleset] = rwp =
					(struct rewrite *) xalloc(sizeof *rwp);
			}
			else
			{
				rwp->r_next = (struct rewrite *) xalloc(sizeof *rwp);
				rwp = rwp->r_next;
			}
			rwp->r_next = NULL;

			/* expand and save the LHS */
			*p = '\0';
			expand(&buf[1], exbuf, &exbuf[sizeof exbuf], e);
			rwp->r_lhs = prescan(exbuf, '\t', pvpbuf);
			if (rwp->r_lhs != NULL)
				rwp->r_lhs = copyplist(rwp->r_lhs, TRUE);

			/* expand and save the RHS */
			while (*++p == '\t')
				continue;
			q = p;
			while (*p != '\0' && *p != '\t')
				p++;
			*p = '\0';
			expand(q, exbuf, &exbuf[sizeof exbuf], e);
			rwp->r_rhs = prescan(exbuf, '\t', pvpbuf);
			if (rwp->r_rhs != NULL)
				rwp->r_rhs = copyplist(rwp->r_rhs, TRUE);
			break;

		  case 'S':		/* select rewriting set */
			ruleset = atoi(&buf[1]);
			if (ruleset >= MAXRWSETS || ruleset < 0)
			{
				syserr("Invalid ruleset %d (%d max)", ruleset, MAXRWSETS);
				ruleset = -1;
			}
			rwp = NULL;
			break;

		  case 'D':		/* macro definition */
			/* only if not set on command line previously */
			if (!bitnset(buf[1], e->e_sticky))
			{
				/* canonicalize hostname before defining */
				if (buf[1] == 'w')
				{
					char wbuf[MAXDNAME];

					expand(munchstring(&buf[2]), wbuf, &wbuf[sizeof wbuf], e);
					(void) definehostname(wbuf, e);
				}
				else
				{
					char *w = munchstring(&buf[2]);

					define(buf[1], newstr(w), e);
				}
			}
			break;

		  case 'H':		/* required header line */
			(void) chompheader(&buf[1], TRUE, e);
			break;

		  case 'C':		/* word class */
		  case 'F':		/* word class from file or pipe */
			if (buf[0] == 'F')
			{
				/* read from file or command */
				for (p = &buf[2]; *p != '\0' && !isspace(*p); p++)
					continue;
				if (*p != '\0')
				{
					*p = '\0';
					while (isspace(*++p))
						continue;
				}
				fileclass(buf[1], &buf[2], *p == '\0' ? "%s" : p);
				break;
			}

			/* scan the list of words and set class for all */
			for (p = &buf[2]; *p != '\0'; )
			{
				register char *wd;
				char delim;

				while (*p != '\0' && isascii(*p) && isspace(*p))
					p++;
				wd = p;
				while (*p != '\0' && !(isascii(*p) && isspace(*p)))
					p++;
				delim = *p;
				*p = '\0';
				if (wd[0] != '\0')
					setclass(buf[1], wd);
				*p = delim;
			}
			break;

		  case 'M':		/* define mailer */
			makemailer(&buf[1]);
			break;

		  case 'O':		/* set option */
			setoption(buf[1], &buf[2], TRUE, FALSE, e);
			break;

		  case 'P':		/* set precedence */
			if (NumPriorities >= MAXPRIORITIES)
			{
				toomany('P', MAXPRIORITIES);
				break;
			}
			for (p = &buf[1]; *p != '\0' && *p != '=' && *p != '\t'; p++)
				continue;
			if (*p == '\0')
				goto badline;
			*p = '\0';
			Priorities[NumPriorities].pri_name = newstr(&buf[1]);
			Priorities[NumPriorities].pri_val = atoi(++p);
			NumPriorities++;
			break;

		  case 'T':		/* trusted user(s) */
			p = &buf[1];
			while (*p != '\0')
			{
				while (isspace(*p))
					p++;
				q = p;
				while (*p != '\0' && !isspace(*p))
					p++;
				if (*p != '\0')
					*p++ = '\0';
				if (*q == '\0')
					continue;
				for (pv = TrustedUsers; *pv != NULL; pv++)
					continue;
				if (pv >= &TrustedUsers[MAXTRUST])
				{
					toomany('T', MAXTRUST);
					break;
				}
				*pv = newstr(q);
			}
			break;

		  default:
		  badline:
			syserr("Unknown control line \"%s\"", buf);
		}
	}
	FileName = NULL;
}
/*
**  TOOMANY -- signal too many of some option
**
**	Parameters:
**		id -- the id of the error line
**		maxcnt -- the maximum possible values
**
**	Returns:
**		none.
**
**	Side Effects:
**		gives a syserr.
*/

static void toomany(id, maxcnt)
	char id;
	int maxcnt;
{
	syserr("too many %c lines, %d max", id, maxcnt);
}
/*
**  FILECLASS -- read members of a class from a file or command
**
**	Parameters:
**		class -- class to define.
**		filename -- name of file or command to read.
**		fmt -- scanf string to use for match.
**
**	Returns:
**		none
**
**	Side Effects:
**
**		puts all lines in filename or command that match
**			a scanf into the named class.
*/

static void fileclass(class, filename, fmt)
	int class;
	char *filename, *fmt;
{
	char buf[MAXLINE];
	char fcmd[MAXLINE];		/* fileclass command name */
	char farg[MAXLINE];		/* fileclass command argument */
	char fdata[MAXLINE];		/* fileclass output */
	char ferrmsg[MAXLINE];		/* fileclass error message */
	FILE *fout, *ferr;
	int fpid, fstatus;
	bool frompipe, outdone = FALSE, errdone = FALSE, did_syserr = FALSE;

	frompipe = filename[0] == '|';
	if (frompipe)
	{
		if ((fpid = p2open(&filename[1], NULL, &fout, &ferr)) < 0)
			return;
	}
	else
	{
		if ((fout = fopen(filename, "r")) == NULL)
		{
			syserr("fileclass %c: %s", class, filename);
			return;
		}
	}

	do
	{
		register STAB *s;
		char wordbuf[MAXNAME+1];

		if (fgets(buf, sizeof buf, fout) != NULL)
		{
			if (sscanf(buf, fmt, wordbuf) != 1)
				continue;
			s = stab(wordbuf, ST_CLASS, ST_ENTER);
			setbitn(class, s->s_class);
		}
		if (frompipe && fgets(buf, sizeof buf, ferr) != NULL)
		{
			fixcrlf(buf, TRUE);
			syserr("fileclass %c: %s", class, buf);
			did_syserr = TRUE;
		}
		outdone = (feof(fout) || ferror(fout));
		errdone = frompipe? (feof(ferr) || ferror(ferr)) : TRUE;
	}
	while (!(outdone && errdone));

	if (frompipe)
	{
		/*
		**	clean up after the child
		*/
		fstatus = waitfor(fpid);
		if (fstatus != 0 && !did_syserr)
		{
			errno = 0;
			if (fstatus & 0377 != 0)
			{
				syserr("fileclass %c: %s: signal %d",
				    class, &filename[1], fstatus & 0377);
			}
			else
			{
				syserr("fileclass %c: %s: exit %d",
				    class, &filename[1], (fstatus >> 8) & 0377);
			}
		}

		/*
		**	close the pipes
		*/
		(void) fclose(fout);
		(void) fclose(ferr);
	}
	else
		(void) fclose(fout);

	return;
}
/*
**  MAKEMAILER -- define a new mailer.
**
**	Parameters:
**		line -- description of mailer.  This is in labeled
**			fields.  The fields are:
**			   P -- the path to the mailer
**			   F -- the flags associated with the mailer
**			   A -- the argv for this mailer
**			   S -- the sender rewriting set
**			   R -- the recipient rewriting set
**			   E -- the eol string
**			The first word is the canonical name of the mailer.
**
**	Returns:
**		none.
**
**	Side Effects:
**		enters the mailer into the mailer table.
*/

static void makemailer(line)
	char *line;
{
	register char *p;
	register struct mailer *m;
	register STAB *s;
	int i;
	char fcode;
	extern int NextMailer;
	extern char **makeargv();
	extern char *munchstring();
	extern char *DelimChar;
	extern long atol();

	/* allocate a mailer and set up defaults */
	m = (struct mailer *) xalloc(sizeof *m);
	bzero((char *) m, sizeof *m);
	m->m_mno = NextMailer;
	m->m_eol = newstr("\n");

	/* collect the mailer name */
	for (p = line; *p != '\0' && *p != ',' && !(isascii(*p) && isspace(*p)); p++)
		continue;
	if (*p != '\0')
		*p++ = '\0';
	m->m_name = newstr(line);

	/* now scan through and assign info from the fields */
	while (*p != '\0')
	{
		while (*p != '\0' && (*p == ',' || (isascii(*p) && isspace(*p))))
			p++;

		/* p now points to field code */
		fcode = *p;
		while (*p != '\0' && *p != '=' && *p != ',')
			p++;
		if (*p++ != '=')
		{
			syserr("No `=' in %c field of %s mailer definition",
			fcode, m->m_name);
			return;
		}
		while (isascii(*p) && isspace(*p))
			p++;

		/* p now points to the field body */
		p = munchstring(p);

		/* install the field into the mailer struct */
		switch (fcode)
		{
		  case 'P':		/* pathname */
			m->m_mailer = newstr(p);
			break;

		  case 'F':		/* flags */
			for (; *p != '\0'; p++)
				setbitn(*p, m->m_flags);
			break;

		  case 'S':		/* sender rewriting ruleset */
		  case 'R':		/* recipient rewriting ruleset */
			i = atoi(p);
			if (i < 0 || i >= MAXRWSETS)
			{
				syserr("Invalid %c rewrite set in %s mailer definition",
					fcode, m->m_name);
				return;
			}
			if (fcode == 'S')
				m->m_s_rwset = i;
			else
				m->m_r_rwset = i;
			break;

		  case 'E':		/* end of line string */
			m->m_eol = newstr(p);
			break;

		  case 'A':		/* argument vector */
			m->m_argv = makeargv(p);
			break;

		  case 'M':		/* maximum message size */
			m->m_maxsize = atol(p);
			break;
		}

		p = DelimChar;
	}

	/* now store the mailer away */
	if (NextMailer >= MAXMAILERS)
	{
		toomany('M', MAXMAILERS);
		return;
	}
	Mailer[NextMailer++] = m;
	s = stab(m->m_name, ST_MAILER, ST_ENTER);
	s->s_mailer = m;
}
/*
**  MUNCHSTRING -- translate a string into internal form.
**
**	Parameters:
**		p -- the string to munch.
**
**	Returns:
**		the munched string.
**
**	Side Effects:
**		Sets "DelimChar" to point to the string that caused us
**		to stop.
*/

char *
munchstring(p)
	register char *p;
{
	register char *q;
	bool backslash = FALSE;
	bool quotemode = FALSE;
	static char buf[MAXLINE];
	extern char *DelimChar;

	for (q = buf; *p != '\0'; p++)
	{
		if (backslash)
		{
			/* everything is roughly literal */
			backslash = FALSE;
			switch (*p)
			{
			  case 'r':		/* carriage return */
				*q++ = '\r';
				continue;

			  case 'n':		/* newline */
				*q++ = '\n';
				continue;

			  case 'f':		/* form feed */
				*q++ = '\f';
				continue;

			  case 'b':		/* backspace */
				*q++ = '\b';
				continue;
			}
			*q++ = *p;
		}
		else
		{
			if (*p == '\\')
				backslash = TRUE;
			else if (*p == '"')
				quotemode = !quotemode;
			else if (quotemode || *p != ',')
				*q++ = *p;
			else
				break;
		}
	}

	DelimChar = p;
	*q++ = '\0';
	return (buf);
}
/*
**  MAKEARGV -- break up a string into words
**
**	Parameters:
**		p -- the string to break up.
**
**	Returns:
**		a char **argv (dynamically allocated)
**
**	Side Effects:
**		munges p.
*/

char **
makeargv(p)
	register char *p;
{
	char *q;
	int i;
	char **avp;
	char *argv[MAXPV + 1];

	/* take apart the words */
	i = 0;
	while (*p != '\0' && i < MAXPV)
	{
		q = p;
		while (*p != '\0' && !(isascii(*p) && isspace(*p)))
			p++;
		while (isascii(*p) && isspace(*p))
			*p++ = '\0';
		argv[i++] = newstr(q);
	}
	argv[i++] = NULL;

	/* now make a copy of the argv */
	avp = (char **) xalloc(sizeof *avp * i);
	bcopy((char *) argv, (char *) avp, sizeof *avp * i);

	return (avp);
}
/*
**  PRINTRULES -- print rewrite rules (for debugging)
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		prints rewrite rules.
*/

# ifdef DEBUG

void printrules()
{
	register struct rewrite *rwp;
	register int ruleset;

	for (ruleset = 0; ruleset < MAXRWSETS; ruleset++)
	{
		if (RewriteRules[ruleset] == NULL)
			continue;
		printf("\n----Rule Set %d:", ruleset);

		for (rwp = RewriteRules[ruleset]; rwp != NULL; rwp = rwp->r_next)
		{
			printf("\nLHS:");
			printav(rwp->r_lhs);
			printf("RHS:");
			printav(rwp->r_rhs);
		}
	}
}

# endif /* DEBUG */
/*
**  SETOPTION -- set global processing option
**
**	Parameters:
**		opt -- option name.
**		val -- option value (as a text string).
**		safe -- set if this came from a configuration file.
**			Some options (if set from the command line) will
**			reset the user id to avoid security problems.
**		sticky -- if set, don't let other setoptions override
**			this value.
**		e -- the current envelope.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Sets options as implied by the arguments.
*/

static BITMAP	StickyOpt;		/* set if option is stuck */
extern char	*NetName;		/* name of home (local) network */

void setoption(opt, val, safe, sticky, e)
	char opt;
	char *val;
	bool safe;
	bool sticky;
	register ENVELOPE *e;
{
	register char *p;
	extern bool atobool();
	extern time_t convtime();
	extern int QueueLA;
	extern int RefuseLA;
	extern char *AliasMap; /* NIS mail map*/
	 

# ifdef DEBUG
	if (tTd(37, 1))
		printf("setoption %c=%s", opt, val);
# endif /* DEBUG */

	/*
	**  See if this option is preset for us.
	*/

	if (bitnset(opt, StickyOpt) ||
	    (opt == 'M' && bitnset(val[0], e->e_sticky)))
	{
# ifdef DEBUG
		if (tTd(37, 1))
			printf(" (ignored)\n");
# endif /* DEBUG */
		return;
	}

	/*
	**  Check to see if this option can be specified by this user.
	*/

	if (!safe && getuid() == 0)
	        safe = TRUE;

# ifdef HP_NFS
        /* add option 'N' (as 'Y' in sun) for NIS alias map */
	if (!safe && strchr("CdeiLmNoprsv", opt) == NULL)
# else	/* ! HP_NFS */
	if (!safe && strchr("CdeiLmoprsv", opt) == NULL)
# endif /* HP_NFS */
	{
		if (opt != 'M' || (val[0] != 'r' && val[0] != 's'))
		{
# ifdef DEBUG
				if (tTd(37, 1))
					printf(" (unsafe)");
# endif /* DEBUG */
				{
					if (getuid() != geteuid())
					{
						printf("(Resetting uid)\n");
						(void) setgid(getgid());
						(void) setuid(getuid());
					}
				}
		}
	}
# ifdef DEBUG
	else if (tTd(37, 1))
		printf("\n");
# endif /* DEBUG */

	switch (opt)
	{
	  case 'A':		/* set default alias file */
		if (val[0] == '\0')
			AliasFile = "aliases";
		else
			AliasFile = newstr(val);
		break;

	  case 'a':		/* look N minutes for "@:@" in alias file */
		if (val[0] == '\0')
			SafeAlias = 5;
		else
			SafeAlias = atoi(val);
		break;

	  case 'B':		/* substitution for blank character */
		SpaceSub = val[0];
		if (SpaceSub == '\0')
			SpaceSub = ' ';
		break;

	  case 'c':		/* don't connect to "expensive" mailers */
		NoConnect = atobool(val);
		break;

	  case 'C':		/* checkpoint every N addresses */
		CheckpointInterval = atoi(val);
		break;

	  case 'd':		/* delivery mode */
		switch (*val)
		{
		  case '\0':
			e->e_sendmode = SM_DELIVER;
			break;

		  case SM_QUEUE:	/* queue only */
# ifndef QUEUE
			syserr("need QUEUE to set -odqueue");
# endif /* QUEUE */
			/* fall through..... */

		  case SM_DELIVER:	/* do everything */
		  case SM_FORK:		/* fork after verification */
			e->e_sendmode = *val;
			break;

		  default:
			syserr("Unknown delivery mode %c", *val);
			exit(EX_USAGE);
		}
		break;

	  case 'D':		/* rebuild alias database as needed */
		AutoRebuild = atobool(val);
		break;

	  case 'E':		/* set default encoding option */
		if (val[0] == '\0')
			Bit8Mode = B8_ENCODE;
		else
			Bit8Mode = *val;
		break;

	  case 'e':		/* set error processing mode */
		switch (*val)
		{
		  case EM_QUIET:	/* be silent about it */
		  case EM_MAIL:		/* mail back */
		  case EM_BERKNET:	/* do berknet error processing */
		  case EM_WRITE:	/* write back (or mail) */
			HoldErrs = TRUE;
			/* fall through... */

		  case EM_PRINT:	/* print errors normally (default) */
			e->e_errormode = *val;
			break;

		  default:
			syserr("Unknown error return mode %c", *val);
			e->e_errormode = EM_PRINT;
			break;
			
		}
		break;

	  case 'F':		/* file mode */
		FileMode = atooct(val) & 0777;
		break;

	  case 'f':		/* save Unix-style From lines on front */
		SaveFrom = atobool(val);
		break;

	  case 'g':		/* default gid */
		if (isascii(*val) && isdigit(*val))
			DefGid = atoi(val);
		else
		{
			register struct group *gr;

			DefGid = -1;
			gr = getgrnam(val);
			if (gr == NULL)
				syserr("readcf: option g: unknown group %s", val);
			else
				DefGid = gr->gr_gid;
		}
		if (getgrgid(DefGid) == NULL)
			syserr("readcf: option g: unknown default group id %d", DefGid);
		break;

	  case 'H':		/* help file */
		if (val[0] == '\0')
			HelpFile = "sendmail.hf";
		else
			HelpFile = newstr(val);
		break;

	  case 'I':		/* use internet domain name server */
		UseNameServer = atobool(val);
		break;

	  case 'i':		/* ignore dot lines in message */
		IgnrDot = atobool(val);
		break;

	  case 'K':		/* max allowable msg size (in bytes) */
		MaxMessageSize = atol(val);
		break;

	  case 'L':		/* log level */
		LogLevel = atoi(val);
		break;

	  case 'M':		/* define macro */
		/* canonicalize hostname before defining */
		if (val[0] == 'w')
		{
			char wbuf[MAXDNAME];

			expand(denlstring(munchstring(&val[1]), TRUE, FALSE),
				wbuf, &wbuf[sizeof wbuf], e);
			(void) definehostname(wbuf, e);
		}
		else
		{
			char *w = munchstring(&val[1]);

			p = newstr(w);
			if (!safe)
				cleanstrcpy(p, p, MAXNAME);
			define(val[0], p, e);
		}
		if (sticky)
		{
			setbitn(val[0], e->e_sticky);
		}
		sticky = FALSE;
		break;

	  case 'm':		/* send to me too */
		MeToo = atobool(val);
		break;

# ifdef HP_NFS
	  case 'N':		/*** set NIS alias map ***/
		AliasMap = newstr(val);
		break;
# endif /* HP_NFS */

	  case 'n':		/* validate RHS in newaliases */
		CheckAliases = atobool(val);
		break;

# ifdef DAEMON			/*** changed from 'N' to 'O' ***/
	  case 'O':		/* home (local?) network name */
		NetName = newstr(val);
		break;
# endif /* DAEMON */

	  case 'o':		/* assume old style headers */
		if (atobool(val))
			e->e_flags |= EF_OLDSTYLE;
		else
			e->e_flags &= ~EF_OLDSTYLE;
		break;

	  case 'p':		/* select privacy level */
		p = val;
		for (;;)
		{
			register struct prival *pv;
			extern struct prival PrivacyValues[];

			while (isascii(*p) && (isspace(*p) || ispunct(*p)))
				p++;
			if (*p == '\0')
				break;
			val = p;
			while (isascii(*p) && isalnum(*p))
				p++;
			if (*p != '\0')
				*p++ = '\0';

			for (pv = PrivacyValues; pv->pv_name != NULL; pv++)
			{
				if (strcasecmp(val, pv->pv_name) == 0)
					break;
			}
			if (pv->pv_name == NULL)
				syserr("readcf: Op line: %s unrecognized", val);
			PrivacyFlags |= pv->pv_flag;
		}
		break;

	  case 'P':		/* postmaster copy address for returned mail */
		PostMasterCopy = newstr(val);
		break;

	  case 'q':		/* slope of queue only function */
		QueueFactor = atoi(val);
		break;

	  case 'Q':		/* queue directory */
		if (val[0] == '\0')
			QueueDir = "mqueue";
		else
			QueueDir = newstr(val);
		break;

	  case 'r':		/* read timeout */
		ReadTimeout = convtime(val);
		break;

	  case 'R':		/* remote mail delivery */
	  	RemoteServer = newstr(val);
	  	break;

	  case 'S':		/* status file */
		if (val[0] == '\0')
			StatFile = "sendmail.st";
		else
			StatFile = newstr(val);
		break;

	  case 's':		/* be super safe, even if expensive */
		SuperSafe = atobool(val);
		break;

	  case 'T':		/* queue timeout */
		TimeOut = convtime(val);
		/*FALLTHROUGH*/

	  case 't':		/* time zone name */
		break;

	  case 'u':		/* set default uid */
		if (isascii(*val) && isdigit(*val))
			DefUid = atoi(val);
		else
		{
			register struct passwd *pw;

			DefUid = -1;
			pw = getpwnam(val);
			if (pw == NULL)
				syserr("readcf: option u: unknown user %s", val);
			else
				DefUid = pw->pw_uid;
		}
		if (getpwuid(DefUid) == NULL)
			syserr("readcf: option u: unknown default user id %d", DefUid);
		setdefuser();
		break;

	  case 'v':		/* run in verbose mode */
		Verbose = atobool(val);
		break;

	  case 'V':		/* set default reverse-alias file */
		if (val[0] == '\0')
			ReverseAliasFile = "rev-aliases";
		else
			ReverseAliasFile = newstr(val);
		break;

	  case 'x':		/* load avg at which to auto-queue msgs */
		QueueLA = atoi(val);
		break;

	  case 'X':		/* load avg at which to auto-reject connections */
		RefuseLA = atoi(val);
		break;

	  case 'y':		/* work recipient factor */
		WkRecipFact = atoi(val);
		break;

	  case 'Y':		/* fork jobs during queue runs */
		ForkQueueRuns = atobool(val);
		break;

	  case 'z':		/* work message class factor */
		WkClassFact = atoi(val);
		break;

	  case 'Z':		/* work time factor */
		WkTimeFact = atoi(val);
		break;

	  default:
		break;
	}
	if (sticky)
		setbitn(opt, StickyOpt);
	return;
}
/*
**  SETCLASS -- set a word into a class
**
**	Parameters:
**		class -- the class to put the word in.
**		word -- the word to enter
**
**	Returns:
**		none.
**
**	Side Effects:
**		puts the word into the symbol table.
*/

void setclass(class, word)
	int class;
	char *word;
{
	register STAB *s;

	s = stab(word, ST_CLASS, ST_ENTER);
	setbitn(class, s->s_class);
}
/*
**  DEFINEHOSTNAME -- canonicalize hostname, define macro 'w' as the
**		canonicalized hostname, and add it and its aliases,
**		if any, to class 'w'.
**
**	Parameters:
**		hostname -- the hostname to define; if hostname is "",
**			myhostname will fill it in by calling gethostname.
**		e -- the main envelope.
**
**	Returns:
**		normally 0; -1 if myhostname() fails.
**
**	Side Effects:
**              either fills in hostname or canonicalizes it in place
**		(it must be at least MAXDNAME); defines the macro 'w';
**		adds a name to class 'w'.
**		syserr if myhostname() fails, which only happens if
**		gethostname() fails.
*/

int
definehostname(hostname, e)
	char *hostname;
	register ENVELOPE *e;
{
	char **hav, *p;
	extern char **myhostname();

	hav = myhostname(hostname, MAXDNAME);
	if (hostname == NULL || hostname[0] == '\0')
	{
		syserr("Cannot get hostname");
		return(-1);
	}
	else
	{
# ifdef DEBUG
		if (tTd(0, 4))
			printf("canonical name: %s\n", hostname);
# endif /* DEBUG */
		p = newstr(hostname);
		define('w', p, e);
		setclass('w', p);
	}
	/*
	**	If the nameserver is running, gethostbyname
	**	will not have returned any aliases.
	*/
	while (hav != NULL && *hav != NULL)
	{
# ifdef DEBUG
		if (tTd(0, 4))
			printf("\ta.k.a.: %s\n", *hav);
# endif /* DEBUG */
		setclass('w', *hav++);
	}

	return(0);
}
