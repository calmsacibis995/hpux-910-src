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
static char rcsid[] = "$Header: parseaddr.c,v 1.27.109.8 95/03/22 17:36:36 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)parseaddr.c	5.13 (Berkeley) 6/1/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: parseaddr.o $Revision: 1.27.109.8 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# include "sendmail.h"

/*
**  PARSEADDR -- Parse an address
**
**	Parses an address and breaks it up into three parts: a
**	net to transmit the message on, the host to transmit it
**	to, and a user on that host.  These are loaded into an
**	ADDRESS header with the values squirreled away if necessary.
**	The "user" part may not be a real user; the process may
**	just reoccur on that machine.  For example, on a machine
**	with an arpanet connection, the address
**		csvax.bill@berkeley
**	will break up to a "user" of 'csvax.bill' and a host
**	of 'berkeley' -- to be transmitted over the arpanet.
**
**	Parameters:
**		addr -- the address to parse.
**		a -- a pointer to the address descriptor buffer.
**			If NULL, a header will be created.
**		copyf -- determines what shall be copied:
**			-1 -- don't copy anything.  The printname
**				(q_paddr) is just addr, and the
**				user & host are allocated internally
**				to parse.
**			0 -- copy out the parsed user & host, but
**				don't copy the printname.
**			+1 -- copy everything.
**		delim -- the character to terminate the address, passed
**			to prescan.
**		e -- the envelope that will contain this address.
**
**	Returns:
**		A pointer to the address descriptor header (`a' if
**			`a' is non-NULL).
**		NULL on error.
**
**	Side Effects:
**		none
*/

/* following delimiters are inherent to the internal algorithms */
# define DELIMCHARS	"\001()<>,;\\\"\r\n"	/* word delimiters */

ADDRESS *
parseaddr(addr, a, copyf, delim, e)
	char *addr;
	register ADDRESS *a;
	int copyf;
	char delim;
	register ENVELOPE *e;
{
	register char **pvp;
	register struct mailer *m;
	char pvpbuf[PSBUFSIZE];
	extern ADDRESS *buildaddr();

	/*
	**  Initialize and prescan address.
	*/

	e->e_to = addr;
# ifdef DEBUG
	if (tTd(20, 1))
		printf("\n--parseaddr(%s)\n", addr);
# endif /* DEBUG */

	pvp = prescan(addr, delim, pvpbuf);
	if (pvp == NULL)
		return (NULL);

	/*
	**  Apply rewriting rules.
	**	Ruleset 0 does basic parsing.  It must resolve.
	*/

	rewrite(pvp, 3, e);
	rewrite(pvp, 0, e);

	/*
	**  See if we resolved to a real mailer.
	*/

	if (pvp[0][0] != CANONNET)
	{
		setstat(EX_USAGE);
		usrerr("Cannot resolve name");
		return (NULL);
	}

	/*
	**  Build canonical address from pvp.
	*/

	a = buildaddr(pvp, a, e);
	if (a == NULL)
		return (NULL);
	m = a->q_mailer;

	/*
	**  Make local copies of the host & user and then
	**  transport them out.
	*/

	if (copyf > 0)
	{
		extern char *DelimChar;
		char savec = *DelimChar;

		*DelimChar = '\0';
		a->q_paddr = newstr(addr);
		*DelimChar = savec;
	}
	else
		a->q_paddr = addr;

	if (a->q_user == NULL)
		a->q_user = "";
	if (a->q_host == NULL)
		a->q_host = "";

	if (copyf >= 0)
	{
		a->q_host = newstr(a->q_host);
		if (a->q_user != a->q_paddr)
			a->q_user = newstr(a->q_user);
	}

	/*
	**  Convert host name to lower case if requested.
	**	User name will be done later.
	*/

	if (!bitnset(M_HST_UPPER, m->m_flags))
		makelower(a->q_host);

	/*
	**  Compute return value.
	*/

# ifdef DEBUG
	if (tTd(20, 1))
	{
		printf("parseaddr-->");
		printaddr(a, FALSE);
	}
# endif /* DEBUG */

	return (a);
}
/*
**  LOWERADDR -- map UPPER->lower case on addresses as requested.
**
**	Parameters:
**		a -- address to be mapped.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

void loweraddr(a)
	register ADDRESS *a;
{
	register MAILER *m = a->q_mailer;

	if (!bitnset(M_USR_UPPER, m->m_flags))
		makelower(a->q_user);
}
/*
**  PRESCAN -- Prescan name and make it canonical
**
**	Scans a name and turns it into a set of tokens.  This process
**	deletes blanks and comments (in parentheses).
**
**	This routine knows about quoted strings and angle brackets.
**
**	There are certain subtleties to this routine.  The one that
**	comes to mind now is that backslashes on the ends of names
**	are silently stripped off; this is intentional.  The problem
**	is that some versions of sndmsg (like at LBL) set the kill
**	character to something other than @ when reading addresses;
**	so people type "csvax.eric\@berkeley" -- which screws up the
**	berknet mailer.
**
**	Parameters:
**		addr -- the name to chomp.
**		delim -- the delimiter for the address, normally
**			'\0' or ','; \0 is accepted in any case.
**			If '\t' then we are reading the .cf file.
**		pvpbuf -- place to put the saved text -- note that
**			the pointers are static.
**
**	Returns:
**		A pointer to a vector of tokens.
**		NULL on error.
**
**	Side Effects:
**		sets global DelimChar to point to the instance of
**		delim (or NULL) terminating addr.
*/

/* states and character types */
# define OPR		0	/* operator */
# define ATM		1	/* atom */
# define QST		2	/* in quoted string */
# define SPC		3	/* chewing up spaces */
# define ONE		4	/* pick up one character */

# define NSTATES	5	/* number of states */
# define TYPE		017	/* mask to select state type */

/* meta bits for table */
# define M		020	/* meta character; don't pass through */
# define B		040	/* cause a break */
# define MB		M|B	/* meta-break */

static short StateTab[NSTATES][NSTATES] =
{
   /*	oldst	chtype>	OPR	ATM	QST	SPC	ONE	*/
	/*OPR*/		OPR|B,	ATM|B,	QST|B,	SPC|MB,	ONE|B,
	/*ATM*/		OPR|B,	ATM,	QST|B,	SPC|MB,	ONE|B,
	/*QST*/		QST,	QST,	OPR,	QST,	QST,
	/*SPC*/		OPR,	ATM,	QST,	SPC|M,	ONE,
	/*ONE*/		OPR,	OPR,	OPR,	OPR,	OPR,
};

# define NOCHAR		-1	/* signal nothing in lookahead token */

char	*DelimChar;		/* set to point to the delimiter */

char **
prescan(addr, delim, pvpbuf)
	char *addr;
	char delim;
	char pvpbuf[];
{
	register char *p;
	register char *q;
	register int c;
	char **avp;
	bool bslashmode;
	int cmntcnt;
	int anglecnt;
	char *tok;
	int state;
	int newstate;
	static char *av[MAXATOM+1];
	extern int errno;

	/* make sure error messages don't have garbage on them */
	errno = 0;

	q = pvpbuf;
	bslashmode = FALSE;
	cmntcnt = 0;
	anglecnt = 0;
	avp = av;
	state = OPR;
	c = NOCHAR;
	p = addr;
# ifdef DEBUG
	if (tTd(22, 45))
	{
		printf("prescan: ");
		xputs(p);
		(void) putchar('\n');
	}
# endif /* DEBUG */

	do
	{
		/* read a token */
		tok = q;
		for (;;)
		{
			/* store away any old lookahead character */
			if (c != NOCHAR && !bslashmode)
			{
				/* see if there is room */
				if (q >= &pvpbuf[PSBUFSIZE - 5])
				{
					usrerr("553 Address too long");
					while ((*p != NULL) && (*p != delim))
						p++;
					DelimChar = p;
					return (NULL);
				}

				/* squirrel it away */
				*q++ = c;
			}

			/* read a new input character */
			c = *p++;
			if (c == '\0')
				break;
			c &= ~0200;

# ifdef DEBUG
			if (tTd(22, 101))
				printf("c=%c, s=%d; ", c, state);
# endif /* DEBUG */

			/* chew up special characters */
			*q = '\0';
			if (bslashmode)
			{
				bslashmode = FALSE;

				/* kludge \! for naive users */
				if (cmntcnt > 0)
				{
					c = NOCHAR;
					continue;
				}
				else if (c != '!' || state == QST)
				{
					*q++ = '\\';
					continue;
				}
			}
			
			if (c == '\\')
			{
				bslashmode = TRUE;
			}
			else if (state == QST)
			{
				/* do nothing, just avoid next clauses */
			}
			else if (c == '(')
			{
				cmntcnt++;
				c = NOCHAR;
			}
			else if (c == ')')
			{
				if (cmntcnt <= 0)
				{
					usrerr("Unbalanced ')'");
					while ((*p != NULL) && (*p != delim))
						p++;
					DelimChar = p;
					return (NULL);
				}
				else
					cmntcnt--;
			}
			else if (cmntcnt > 0)
				c = NOCHAR;
			else if (c == '<')
				anglecnt++;
			else if (c == '>')
			{
				if (anglecnt <= 0)
				{
					usrerr("Unbalanced '>'");
					while ((*p != NULL) && (*p != delim))
						p++;
					DelimChar = p;
					return (NULL);
				}
				anglecnt--;
			}
			else if (delim == ' ' && isascii(c) && isspace(c))
				c = ' ';

			if (c == NOCHAR)
				continue;

			/* see if this is end of input */
			if (c == delim && anglecnt <= 0 && state != QST)
				break;

			newstate = StateTab[state][toktype(c)];
# ifdef DEBUG
			if (tTd(22, 101))
				printf("ns=%02o\n", newstate);
# endif /* DEBUG */
			state = newstate & TYPE;
			if (bitset(M, newstate))
				c = NOCHAR;
			if (bitset(B, newstate))
				break;
		}

		/* new token */
		if (tok != q)
		{
			*q++ = '\0';
# ifdef DEBUG
			if (tTd(22, 36))
			{
				printf("tok=");
				xputs(tok);
				(void) putchar('\n');
			}
# endif /* DEBUG */
			if (avp >= &av[MAXATOM])
			{
				syserr("553 prescan: too many tokens");
				while ((*p != NULL) && (*p != delim))
					p++;
				DelimChar = p;
				return (NULL);
			}
			*avp++ = tok;
		}
	} while (c != '\0' && (c != delim || anglecnt > 0));
	*avp = NULL;
	DelimChar = --p;
	if (tTd(22, 12))
		{
		printf("prescan==>");
		printav(av);
		}
	if (cmntcnt > 0)
		usrerr("Unbalanced '('");
	else if (anglecnt > 0)
		usrerr("Unbalanced '<'");
	else if (state == QST)
		usrerr("Unbalanced '\"'");
	else if (av[0] != NULL)
		return (av);
	return (NULL);
}
/*
**  TOKTYPE -- return token type
**
**	Parameters:
**		c -- the character in question.
**
**	Returns:
**		Its type.
**
**	Side Effects:
**		none.
*/

toktype(c)
	register char c;
{
	static char buf[50];
	static bool firstime = TRUE;

	if (firstime)
	{
		firstime = FALSE;
		expand("\001o", buf, &buf[sizeof buf - 1], CurEnv);
		(void) strcat(buf, DELIMCHARS);
	}
	if (c == MATCHCLASS || c == MATCHREPL || c == MATCHNCLASS)
		return (ONE);
	if (c == '"')
		return (QST);
	if (!isascii(c))
		return (ATM);
	if (isspace(c) || c == ')')
		return (SPC);
	if (iscntrl(c) || strchr(buf, c) != NULL)
		return (OPR);
	return (ATM);
}
/*
**  REWRITE -- apply rewrite rules to token vector.
**
**	This routine is an ordered production system.  Each rewrite
**	rule has a LHS (called the pattern) and a RHS (called the
**	rewrite); 'rwr' points the the current rewrite rule.
**
**	For each rewrite rule, 'avp' points the address vector we
**	are trying to match against, and 'pvp' points to the pattern.
**	If pvp points to a special match value (MATCHZANY, MATCHANY,
**	MATCHONE, MATCHCLASS, MATCHNCLASS) then the address in avp
**	matched is saved away in the match vector (pointed to by 'mvp').
**
**	When a match between avp & pvp does not match, we try to
**	back out.  If we back up over MATCHONE, MATCHCLASS, or MATCHNCLASS
**	we must also back out the match in mvp.  If we reach a
**	MATCHANY or MATCHZANY we just extend the match and start
**	over again.
**
**	When we finally match, we rewrite the address vector
**	and try over again.
**
**	Parameters:
**		pvp -- pointer to token vector.
**		ruleset -- the ruleset to use for rewriting.
**		e -- the current envelope.
**
**	Returns:
**		none.
**
**	Side Effects:
**		pvp is modified.
*/

struct match
{
	char	**first;	/* first token matched */
	char	**last;		/* last token matched */
};

# define MAXMATCH	9	/* max params per rewrite */


void rewrite(pvp, ruleset, e)
	char **pvp;
	int ruleset;
	register ENVELOPE *e;
{
	register char *ap;		/* address pointer */
	register char *rp;		/* rewrite pointer */
	register char **avp;		/* address vector pointer */
	register char **rvp;		/* rewrite vector pointer */
	register struct match *mlp;	/* cur ptr into mlist */
	register struct rewrite *rwr;	/* pointer to current rewrite rule */
	struct match mlist[MAXMATCH];	/* stores match on LHS */
	char *npvp[MAXATOM+1];		/* temporary space for rebuild */
 	char tokbuf[MAXNAME+1];		/* for concatenated class tokens */
	register STAB *s;		/* for looking up class members */
	char **oldavp;			/* temp in case no class match */
	register char *tbp;		/* for traversing tokbuf */
	register int tlen;		/* for range checking on tokbuf */
	bool nosuchruleset = FALSE;	/* error checking on parameter */
	extern char *macvalue();	/* nameserver defined in macro */

	nosuchruleset = (ruleset < 0 || ruleset >= MAXRWSETS);
	if (OpMode == MD_TEST || tTd(21, 2))
	{
		if (nosuchruleset)
		{
			printf("rewrite: ruleset %d not defined\n", ruleset);
		}
		else
		{
			printf("rewrite: ruleset %2d   input:", ruleset);
			printav(pvp);
		}
	}
	if (pvp == NULL || nosuchruleset)
		return;

	/*
	**  Run through the list of rewrite rules, applying
	**	any that match.
	*/

	for (rwr = RewriteRules[ruleset]; rwr != NULL; )
	{
# ifdef DEBUG
		if (tTd(21, 12))
		{
			printf("-----trying rule:");
			printav(rwr->r_lhs);
		}
# endif /* DEBUG */

		/* try to match on this rule */
		mlp = mlist;
		rvp = rwr->r_lhs;
		avp = pvp;
		while ((ap = *avp) != NULL || *rvp != NULL)
		{
			rp = *rvp;
# ifdef DEBUG
			if (tTd(21, 35))
			{
				printf("ap=");
				xputs(ap);
				printf(", rp=");
				xputs(rp);
				printf("\n");
			}
# endif /* DEBUG */
			if (rp == NULL)
			{
				/* end-of-pattern before end-of-address */
				goto backup;
			}
			if (ap == NULL && *rp != MATCHZANY)
			{
				/* end-of-input */
				break;
			}

			switch (*rp & 0377)
			{
			  case MATCHNCLASS:
				/* match any token not in a class */
				s = stab(ap, ST_CLASS, ST_FIND);
				if (s != NULL && bitnset(rp[1], s->s_class))
				{
					goto backup;
				}
				mlp->first = avp;
				mlp->last = avp++;
				mlp++;
				break;

 			  case MATCHCLASS:
 				/* match token or set of tokens in a class */
 				oldavp = avp;
				tbp = tokbuf;
 				*tbp = NULL;
 				do
				{
					if (*avp == NULL ||
					    (tbp + (tlen = strlen(*avp) + 1)) >=
					    (tokbuf + sizeof tokbuf))
					{
						avp = oldavp;
						goto backup;
					}
					strcpy(tbp, *avp++);
					tbp += tlen - 1;
					s = stab(tokbuf, ST_CLASS, ST_FIND);
 				}
				while (s == NULL ||
				    !bitnset(rp[1], s->s_class));
 
 				mlp->first = oldavp;
 				mlp->last = avp - 1;
 				mlp++;
 				break;
 
			  case MATCHONE:
			  case MATCHANY:
				/* match exactly one token */
				mlp->first = avp;
				mlp->last = avp++;
				mlp++;
				break;

			  case MATCHZANY:
				/* match zero or more tokens */
				mlp->first = avp;
				mlp->last = avp - 1;
				mlp++;
				break;

			  default:
				/* must have exact match */
				if (strcasecmp(rp, ap))
					goto backup;
				avp++;
				break;
			}

			/* successful match on this token */
			rvp++;
			continue;

		  backup:
			/* match failed -- back up */
			while (--rvp >= rwr->r_lhs)
			{
				rp = *rvp;
				if (*rp == MATCHANY || *rp == MATCHZANY)
				{
					/* extend binding and continue */
					avp = ++mlp[-1].last;
					avp++;
					rvp++;
					break;
				}
				if ((*rp & 0377) == MATCHCLASS)
				{
					/* extend binding, if possible */
					oldavp = avp;
					*tokbuf = '\0';
					for (avp = mlp[-1].first;
					     avp <= mlp[-1].last;
					     avp++)
					{
						strcat(tokbuf, *avp);
					}
					tbp = strchr(tokbuf, '\0');
					s = NULL;
					do
					{
						if (*avp == NULL ||
						    (tbp +
						    (tlen = strlen(*avp) + 1))
						    >=
						    (tokbuf + sizeof tokbuf))
						{
							s = NULL;
							break;
						}
						strcpy(tbp, *avp++);
						tbp += tlen - 1;
						s = stab(tokbuf,
						    ST_CLASS, ST_FIND);
					}
					while (s == NULL ||
					    !bitnset(rp[1], s->s_class));
					if (s == NULL)
					{
						/* back out binding */
						avp = oldavp;
						mlp--;
						continue;
					}
					else
					{
						/*
						** matched a longer string--
						** extend binding and continue
						*/
						mlp[-1].last = avp - 1;
						rvp++;
						break;
					}
				}
				avp--;
				if (*rp == MATCHONE || *rp == MATCHNCLASS)
				{
					/* back out binding */
					mlp--;
				}
			}

			if (rvp < rwr->r_lhs)
			{
				/* total failure to match */
				break;
			}
		}

		/*
		**  See if we successfully matched
		*/

		if (rvp < rwr->r_lhs || *rvp != NULL)
		{
# ifdef DEBUG
			if (tTd(21, 10))
				printf("----- rule fails\n");
# endif /* DEBUG */
			rwr = rwr->r_next;
			continue;
		}

		rvp = rwr->r_rhs;
# ifdef DEBUG
		if (tTd(21, 12))
		{
			printf("-----rule matches:");
			printav(rvp);
		}
# endif /* DEBUG */

		rp = *rvp;
		if ((*rp & 0377) == CANONUSER)
		{
			/* rewrite just once */
			rvp++;
			rwr = rwr->r_next;
		}
		else if ((*rp & 0377) == CANONHOST)
		{
			/* return result of this rewrite */
			rvp++;
			rwr = NULL;
		}
		else if ((*rp & 0377) == CANONNET)
			rwr = NULL;	/* resolve to {mailer,host,user} */

		/* substitute */
		for (avp = npvp; *rvp != NULL; rvp++)
		{
			register struct match *m;
			register char **pp;

			rp = *rvp;
			if (*rp == MATCHREPL)
			{
				/* substitute from LHS */
				m = &mlist[rp[1] - '1'];
				if (m >= mlp)
				{
					syserr("554 rewrite: ruleset %d: replacement $%c out of bounds",
						ruleset, rp[1]);
					return;
				}
# ifdef DEBUG
				if (tTd(21, 15))
				{
					printf("$%c:", rp[1]);
					pp = m->first;
					while (pp <= m->last)
					{
						printf(" %x=\"", *pp);
						(void) fflush(stdout);
						printf("%s\"", *pp++);
					}
					printf("\n");
				}
# endif /* DEBUG */
				pp = m->first;
				while (pp <= m->last)
				{
					if (avp >= &npvp[MAXATOM])
					{
						syserr("554 rewrite: expansion too long");
						return;
					}
					*avp++ = *pp++;
				}
			}
			else
			{
				/* vanilla replacement */
				if (avp >= &npvp[MAXATOM])
				{
	toolong:
					syserr("554 rewrite: expansion too long");
					return;
				}
				*avp++ = rp;
			}
		}
		/* avp now marks the end of the address vector */
		*avp++ = NULL;

		/*
		**  Check for any subroutine calls.
		*/

		for (rvp = npvp; *rvp != NULL; rvp++)
		{
			char **hbrvp;	/* vector segment containing hostname */
					/* to look up                         */
			char **xpvp;	/* for traversing prescanned hostname */
					/* and "trailing information" vectors */
			int trsize;
			char *olddelimchar;
			char buf[MAXNAME + 1];	/* hostname in string form    */
			char *pvpb1[MAXATOM + 1];
					/* segment of vector after hostname   */
			char pvpbuf[PSBUFSIZE]; /* for prescanning hostname   */
			extern char *DelimChar;

			if ((**rvp & 0377) != HOSTBEGIN)
				continue;

			/*
			**  Got a hostname lookup.
			**
			**	This could be optimized fairly easily.
			*/

			hbrvp = rvp;

			/* extract the match part */
			while (*++rvp != NULL && **rvp != HOSTEND)
				continue;
			if (*rvp != NULL)
				*rvp++ = NULL;

			/* save the remainder of the input vector */
			trsize = (int) (avp - rvp + 1) * sizeof *rvp;
			bcopy((char *) rvp, (char *) pvpb1, trsize);

			/* look it up */
			cataddr(++hbrvp, buf, sizeof buf);
			maphostname(buf, sizeof buf);

			/* scan the new host name */
			olddelimchar = DelimChar;
			xpvp = prescan(buf, '\0', pvpbuf);
			DelimChar = olddelimchar;
			if (xpvp == NULL)
			{
				syserr("rewrite: Cannot prescan canonical hostname: %s", buf);
				return;
			}

			/* append it to the token list */
			for (avp = --hbrvp; *xpvp != NULL; xpvp++)
			{
				*avp++ = newstr(*xpvp);
				if (avp >= &npvp[MAXATOM])
					goto toolong;
			}

			/*
			** make rvp point to the spot following the
			** canonicalized hostname, so the for loop works
			*/
			rvp = avp;

			/* restore the old trailing information */
			for (xpvp = pvpb1; (*avp++ = *xpvp++) != NULL; )
				if (avp >= &npvp[MAXATOM])
					goto toolong;
		}

		/*
		**  Check for subroutine calls.
		*/

		if (**npvp == CALLSUBR)
		{
			bcopy((char *) &npvp[2], (char *) pvp,
				(int) (avp - npvp - 2) * sizeof *avp);
# ifdef DEBUG
			if (tTd(21, 3))
				printf("-----callsubr %s\n", npvp[1]);
# endif /* DEBUG */
			rewrite(pvp, atoi(npvp[1]), e);
		}

		/*
		**  Check for external nameserver calls
		*/

		else if (**npvp == CALLNAME)
		{
			char ncmd[MAXLINE];	/* nameserver command name */
			char narg[MAXLINE];	/* nameserver command argument */
			char ndata[MAXLINE];	/* nameserver output */
			char nerrmsg[MAXLINE];	/* nameserver error message */
			FILE *nout, *nerr;
			int npid, nstatus;
			char *olddelimchar;
			char pvpbuf[PSBUFSIZE];
			extern char *DelimChar;
			char **xpvp;		/* for traversing output of prescan */

			ncmd[0] = narg[0] = ndata[0] = nerrmsg[0] = '\0';

			/* copy nameserver input vector to pvp in case of failure */
			for (avp = pvp, xpvp = &npvp[2]; *xpvp != NULL; )
			{
				*avp++ = *xpvp++;
			}
			*avp = NULL;

			/* set up nameserver argument */
			for (avp = &npvp[2]; *avp != NULL; avp++)
			{
				(void) strcat(narg, *avp);

			}
			(void) strcpy(ncmd, macvalue(*npvp[1], e));
# ifdef DEBUG
			if (tTd(21, 3))
				printf("-----external nameserver: %s %s\n", ncmd, narg);
# endif /* DEBUG */

			npid = (OpMode != MD_INITALIAS) ?
					p2open(ncmd, narg, &nout, &nerr) : 0;
			if (npid > 0)
			{
				char *s;

				s = fgets(ndata, MAXLINE, nout);
				nstatus = waitfor(npid);

				if (s != NULL && nstatus == 0)
				{
					/* success -- copy output data to pvp */
					olddelimchar = DelimChar;
					xpvp = prescan(ndata, '\n', pvpbuf);
					DelimChar = olddelimchar;
					
					for (avp = pvp; *xpvp != NULL; xpvp++)
					{
						*avp++ = newstr(*xpvp);
					}
					*avp = NULL;
				}

				/*
				**	output error message if any
				*/
				if (fgets(nerrmsg, MAXLINE, nerr) != NULL)
				{
					fixcrlf(nerrmsg, TRUE);
# ifdef LOG
					if (nstatus == 0 && LogLevel > 8)
					{
						/* returned zero -- treat as warning message */
						syslog(LOG_NOTICE, "external nameserver %c: %s",
							*npvp[1], nerrmsg);
					}
					else
# endif /* LOG */
					if (nstatus != 0)
					{
						/* returned nonzero -- treat as error message */
						syserr("external nameserver %c: %s",
							*npvp[1], nerrmsg);
					}
				}
				/*
				**	close the pipes
				*/
				(void) fclose(nout);
				(void) fclose(nerr);
# ifdef DEBUG
				if (tTd(21, 3))
				{
					printf("external nameserver return:");
					printav(pvp);
				}
# endif /* DEBUG */
			}
		}
		else
		{
			bcopy((char *) npvp, (char *) pvp,
				(int) (avp - npvp) * sizeof *avp);
		}
# ifdef DEBUG
		if (tTd(21, 4))
		{
			printf("rewritten as:");
			printav(pvp);
		}
# endif /* DEBUG */
	}

	if (OpMode == MD_TEST || tTd(21, 2))
	{
		printf("rewrite: ruleset %2d returns:", ruleset);
		printav(pvp);
	}
}
/*
**  BUILDADDR -- build address from token vector.
**
**	Parameters:
**		tv -- token vector.
**		a -- pointer to address descriptor to fill.
**			If NULL, one will be allocated.
**		e -- the current envelope.
**
**	Returns:
**		NULL if there was an error.
**		'a' otherwise.
**
**	Side Effects:
**		fills in 'a'
*/

ADDRESS *
buildaddr(tv, a, e)
	register char **tv;
	register ADDRESS *a;
	register ENVELOPE *e;
{
	static char buf[MAXNAME+MAXATOM];
	struct mailer **mp;
	register struct mailer *m;

	if (a == NULL)
		a = (ADDRESS *) xalloc(sizeof *a);
	bzero((char *) a, sizeof *a);

	/* figure out what net/mailer to use */
	if (*tv == NULL || (**tv & 0377) != CANONNET)
	{
		syserr("554 buildaddr: no mailer");
		return (NULL);
	}
	tv++;
	if (strcasecmp(*tv, "error") == 0)
	{
		if ((**++tv & 0377) == CANONHOST)
		{
			setstat(atoi(*++tv));
			tv++;
		}
		if ((**tv & 0377) != CANONUSER)
			syserr("buildaddr: no user");
		buf[0] = '\0';
		while (*++tv != NULL)
		{
			if (buf[0] != '\0')
				(void) strcat(buf, " ");
			(void) strcat(buf, *tv);
		}
		usrerr(buf);
		return (NULL);
	}
	for (mp = Mailer; (m = *mp++) != NULL; )
	{
		if (strcasecmp(m->m_name, *tv) == 0)
			break;
	}
	if (m == NULL)
	{
		syserr("554 buildaddr: unknown mailer %s", *tv);
		return (NULL);
	}
	a->q_mailer = m;

	/* figure out what host (if any) */
	tv++;
	if (!bitnset(M_LOCAL, m->m_flags))
	{
		if ((**tv++ & 0377) != CANONHOST)
		{
			syserr("554 buildaddr: no host");
			return (NULL);
		}
		buf[0] = '\0';
		while (*tv != NULL && **tv != CANONUSER)
			(void) strcat(buf, *tv++);
		a->q_host = newstr(buf);
	}
	else
		a->q_host = NULL;

	/* figure out the user */
	if (*tv == NULL || (**tv & 0377) != CANONUSER)
	{
		syserr("554 buildaddr: no user");
		return (NULL);
	}
	tv++;


	/* do special mapping for local mailer */
	if (m == LocalMailer && *tv != NULL)
	{
		register char *p = *tv;

		if (*p == '"')
			p++;
		if (*p == '|')
			a->q_mailer = m = ProgMailer;
	}

	/* externalize the address */
	rewrite(tv, 4, e);

	/* save the result for the command line/RCPT argument */
	cataddr(tv, buf, sizeof buf);
	a->q_user = buf;

	return (a);
}
/*
**  CATADDR -- concatenate pieces of addresses (putting in <LWSP> subs)
**
**	Parameters:
**		pvp -- parameter vector to rebuild.
**		buf -- buffer to build the string into.
**		sz -- size of buf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Destroys buf.
*/

void cataddr(pvp, buf, sz)
	char **pvp;
	char *buf;
	register int sz;
{
	bool oatomtok = FALSE;
	bool natomtok = FALSE;
	register int i;
	register char *p;

	if (pvp == NULL)
	{
		(void) strcpy(buf, "");
		return;
	}
	p = buf;
	sz -= 2;
	while (*pvp != NULL && (i = strlen(*pvp)) < sz)
	{
		natomtok = (toktype(**pvp) == ATM);
		if (oatomtok && natomtok)
			*p++ = SpaceSub;
		(void) strcpy(p, *pvp);
		oatomtok = natomtok;
		p += i;
		sz -= i + 1;
		pvp++;
	}
	*p = '\0';
}
/*
**  SAMEADDR -- Determine if two addresses are the same
**
**	This is not just a straight comparison -- if the mailer doesn't
**	care about the host we just ignore it, etc.
**
**	Parameters:
**		a, b -- pointers to the internal forms to compare.
**
**	Returns:
**		TRUE -- they represent the same mailbox.
**		FALSE -- they don't.
**
**	Side Effects:
**		none.
*/

bool
sameaddr(a, b)
	register ADDRESS *a;
	register ADDRESS *b;
{
	register ADDRESS *ca, *cb;

	/* if they don't have the same mailer, forget it */
	if (a->q_mailer != b->q_mailer)
		return (FALSE);

	/* if the user isn't the same, we can drop out */
	if (strcmp(a->q_user, b->q_user) != 0)
		return (FALSE);

	/* if we have good uids for both but they differ, these are different */
	if (a->q_mailer == ProgMailer)
	{
		ca = getctladdr(a);
		cb = getctladdr(b);
		if (ca != NULL && cb != NULL &&
		    bitset(QGOODUID, ca->q_flags & cb->q_flags) &&
		    ca->q_uid != cb->q_uid)
			return (FALSE);
	}

	/* otherwise compare hosts (but be careful for NULL ptrs) */
	if (a->q_host == b->q_host)
	{
		/* probably both null pointers */
		return (TRUE);
	}
	if (a->q_host == NULL || b->q_host == NULL)
	{
		/* only one is a null pointer */
		return (FALSE);
	}
	if (strcmp(a->q_host, b->q_host) != 0)
		return (FALSE);

	return (TRUE);
}
/*
**  PRINTADDR -- print address (for debugging)
**
**	Parameters:
**		a -- the address to print
**		follow -- follow the q_next chain.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

# ifdef DEBUG

void printaddr(a, follow)
	register ADDRESS *a;
	bool follow;
{
	bool first = TRUE;

	while (a != NULL)
	{
		first = FALSE;
		printf("%x=", a);
		(void) fflush(stdout);
		printf("%s: mailer %d (%s), host `%s', user `%s', ruser `%s'\n",
		       a->q_paddr, a->q_mailer->m_mno, a->q_mailer->m_name,
		       a->q_host, a->q_user, a->q_ruser? a->q_ruser: "<null>");
		printf("\tnext=%x, flags=%o, alias %x\n", a->q_next, a->q_flags,
		       a->q_alias);
		printf("\thome=\"%s\", fullname=\"%s\"\n", a->q_home,
		       a->q_fullname);

		if (!follow)
			return;
		a = a->q_next;
	}
	if (first)
		printf("[NULL]\n");
}

# endif /* DEBUG */
/*
**  REMOTENAME -- return the name relative to the current mailer
**
**	Parameters:
**		name -- the name to translate.
**		m -- the mailer that we want to do rewriting relative
**			to.
**		senderaddress -- if set, uses the sender rewriting rules
**			rather than the recipient rewriting rules.
**		canonical -- if set, strip out any comment information, etc.
**		e -- the current envelope.
**
**	Returns:
**		the text string representing this address relative to
**			the receiving mailer.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The text string returned is tucked away locally;
**			copy it if you intend to save it.
*/

char *
remotename(name, m, senderaddress, canonical, e)
	char *name;
	struct mailer *m;
	bool senderaddress;
	bool canonical;
	register ENVELOPE *e;
{
	register char **pvp;
	char *fancy;
	char *oldg = macvalue('g', e);
	register char *p;
	static char buf[MAXNAME + MAXATOM];
	char lbuf[MAXNAME + MAXATOM];
	char pvpbuf[PSBUFSIZE];
	extern char *crackaddr();
	bool hadangles = FALSE;

# ifdef DEBUG
	if (tTd(12, 1))
		printf("remotename(%s)\n", name);
# endif /* DEBUG */

	/* don't do anything if we are tagging it as special */
	if ((senderaddress ? m->m_s_rwset : m->m_r_rwset) < 0)
		return (name);

	/*
	**  Do a heuristic crack of this name to extract any comment info.
	**	This will leave the name as a comment and a $g macro.
	*/

	if (canonical)
		fancy = "\001g";
	else
	{
		fancy = crackaddr(name);
		hadangles = (strchr(name,'<') != NULL && strchr(name,'>') != NULL);
	}

	/*
	**  Turn the name into canonical form.
	**	Normally this will be RFC 822 style, i.e., "user@domain".
	**	If this only resolves to "user", and the "C" flag is
	**	specified in the sending mailer, then the sender's
	**	domain will be appended.
	*/

	pvp = prescan(name, '\0', pvpbuf);
	if (pvp == NULL)
		return (name);
	rewrite(pvp, 3, e);
	if (e->e_fromdomain != NULL)
	{
		/* append from domain to this address */
		register char **pxp = pvp;

		/* see if there is an "@domain" in the current name */
		while (*pxp != NULL && strcmp(*pxp, "@") != 0)
			pxp++;
		if (*pxp == NULL)
		{
			/* no.... append the "@domain" from the sender */
			register char **qxq = e->e_fromdomain;

			while ((*pxp++ = *qxq++) != NULL)
				continue;
			rewrite(pvp, 3, e);
		}
	}

	/*
	**  Do more specific rewriting.
	**	Rewrite using ruleset 1 or 2 depending on whether this is
	**		a sender address or not.
	**	Then run it through any receiving-mailer-specific rulesets.
	*/

	if (senderaddress)
	{
		rewrite(pvp, 1, e);
		if (m->m_s_rwset > 0)
			rewrite(pvp, m->m_s_rwset, e);
	}
	else
	{
		rewrite(pvp, 2, e);
		if (m->m_r_rwset > 0)
			rewrite(pvp, m->m_r_rwset, e);
	}

	/*
	**  Do any final sanitation the address may require.
	**	This will normally be used to turn internal forms
	**	(e.g., user@host.LOCAL) into external form.  This
	**	may be used as a default to the above rules.
	*/

	rewrite(pvp, 4, e);

	/*
	**  Now restore the comment information we had at the beginning.
	*/

	cataddr(pvp, buf, sizeof buf - 2);

	/*
	**  Enclose $g with angle brackets iff:
	**	the address had angle brackets in the first place;
	**	the rewriting did not add angle brackets.
	*/

	if (!canonical && hadangles &&
		(strchr(buf,'<') == NULL || strchr(buf,'>') == NULL))
	{
		strcpy(lbuf, "<");
		strcat(lbuf, buf);
		strcat(lbuf, ">");
	}
	else
	{
		strcpy(lbuf, buf);
	}
	define('g', lbuf, e);
	expand(fancy, buf, &buf[sizeof buf - 1], e);
	define('g', oldg, e);

# ifdef DEBUG
	if (tTd(12, 1))
		printf("remotename => `%s'\n", buf);
# endif /* DEBUG */
	return (buf);
}

/*
**	P2OPEN -- return streams for stdout and stderr of command;
**		command and args are concatenated and passed to sh -c.
**
**	Parameters:
**		command - command to exec
**		args - string containing args, if any, to command
**		out - pointer to FILE * for stdout of command
**		err - pointer to FILE * for stderr of command
**		
**	Returns:
**		pid of child command
**
**	Side Effects:
**		creates two pipes, forks child, returns in parent,
**		sets out and err value return parameters.
**
**	Warnings:
**		it is the caller's responsibility to fclose out and err,
**		and to clean up after the child, either by ignoring SIGCLD
**		or by waiting for the pid returned by p2open.
*/

int
p2open(command, args, out, err)
	char *command, *args;
	FILE **out, **err;
{
	int pid, status, sav_errno;
	char *cmdbuf;
	int fd[2], errfd[2];	/* command stdout & stderr pipes */

	if (pipe(fd) < 0 || pipe(errfd) < 0)
	{
		syserr("p2open: pipe");
		return(-1);
	}
	if ((pid = dofork()) < 0)
	{
 		syserr("p2open: fork");
		return(-1);
	}
	if (pid == 0)		/* child */
	{
                (void) close(fd[0]);
		(void) dup2(fd[1], 1);
		(void) close(fd[1]);
		(void) close(errfd[0]);
		(void) dup2(errfd[1], 2);
		(void) close(errfd[1]);

		if (args == NULL || args[0] == '\0')
			args = NULL;

		/*
		**      reset uid and gid to avoid running
		**      command with promoted privilege or authorization.
		*/
		(void) setuid(getuid());
		(void) setgid(getgid());

		(void) execl(command, command, args, 0);
		/*
		**	exec failed: output error message to stderr
		**	for calling function to deal with, and exit 1
		**	so it will know something went wrong.
		*/
		if (args == NULL)
		{
			fprintf(stderr, "%s: %s", command, errstring(errno));
		}
		else
		{
			fprintf(stderr, "%s %s: %s", command, args, errstring(errno));
		}
		exit(1);
	}
	if (pid > 0)		/* parent */
	{
		(void) close(fd[1]);
		(void) close(errfd[1]);
		if ((*out = fdopen(fd[0],"r")) == NULL ||
		    (*err = fdopen(errfd[0],"r")) == NULL)
		{
			sav_errno = errno;
			status = waitfor(pid);
			(void) close(fd[0]);
			(void) close(errfd[0]);
			errno = sav_errno;
			syserr("p2open: fdopen");
			return (-1);
		}
		else
		{
			return(pid);
		}
	}
}
