/* @(#) $Revision: 70.1 $ */     
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"


static tchar	quote;	/* used locally */
static tchar	quoted;	/* used locally */


static char *
copyto(endch)
register tchar	endch;
{
	register tchar	c;

	while ((c = getch(endch)) != endch && c)
	{
		pushstak(c | quote);
	}
	zerostak();
	if (c != endch)
		error(nl_msg(610,badsub));
}

static
skipto(endch)
register tchar	endch;
{
	/*
	 * skip chars up to }
	 */
	register tchar	c;

	while ((c = readc()) && c != endch)
	{
		switch (c)
		{
		case SQUOTE:
			skipto(SQUOTE);
			break;

		case DQUOTE:
			skipto(DQUOTE);
			break;

		case DOLLAR:
			if (readc() == BRACE)
				skipto('}');
		}
	}
	if (c != endch)
		error(nl_msg(610,badsub));
}

static
getch(endch)
tchar	endch;		
{
	register tchar	d;

retry:
	d = readc();
	if (!subchar(d))
		return(d);
	if (d == DOLLAR)
	{
		register int	c;

		if ((c = readc(), dolchar(c)))
		{
			struct tnamnod *n = (struct tnamnod *)NIL; 
			int		dolg = 0;
			BOOL		bra;
			BOOL		nulflg;
			register tchar	*argp, *v;	
			tchar		idb[2];	
			tchar		*id = idb;

			if (bra = (c == BRACE))
				c = readc();
			if (letter(c))
			{
				argp = (tchar *)relstak();	
				while (alphanum(c))
				{
					pushstak(c);
					c = readc();
				}
				zerostak();
				n = lookup(absstak(argp));
				setstak(argp);
				if (n->namflg & N_FUNCTN)
					error(nl_msg(610,badsub));
				v = n->namval;
				id = n->namid;
				peekc = c | MARK;
			}
			else if (digchar(c))
			{
				*id = c;
				idb[1] = 0;
				if (astchar(c))
				{
					dolg = 1;
					c = '1';
				}
				c -= '0';
				v = ((c == 0) ? cmdadr : (c <= dolc) ? dolv[c] : (tchar *)(dolg = 0));
			}
			else if (c == '$')
				v = pidadr;
			else if (c == '!')
				v = pcsadr;
			else if (c == '#')
			{
				itos(dolc);
				v = numbuf;
			}
			else if (c == '?')
			{
				itos(retval);
				v = numbuf;
			}
			else if (c == '-')
			{
#ifndef NLS
			v = flagadr;
#else NLS
			tchar l_v[1024];		
				v = sto_tchar(flagadr,l_v);
#endif NLS
			}
			else if (bra)
				error(nl_msg(610,badsub));
			else
				goto retry;
			c = readc();
			if (c == ':' && bra)	/* null and unset fix */
			{
				nulflg = 1;
				c = readc();
			}
			else
				nulflg = 0;
			if (!defchar(c) && bra)
				error(nl_msg(610,badsub));
			argp = 0;
			if (bra)
			{
				if (c != '}')
				{
					argp = (tchar *)relstak();
					if ((v == 0 || (nulflg && *v == 0)) ^ (setchar(c)))
						copyto('}');
					else
						skipto('}');
					argp = absstak(argp);
				}
			}
			else
			{
				peekc = c | MARK;
				c = 0;
			}
			if (v && (!nulflg || *v))
			{
				tchar tmp = (*id == '*' ? SP | quote : SP);

				if (c != '+')
				{
					for (;;)
					{
						if (*v == 0 && quote)
						{
							pushstak(QUOTE);
						}
						else
						{
							while (c = *v++)
							{
								pushstak(c | quote);
							}
						}

						if (dolg == 0 || (++dolg > dolc))
							break;
						else
						{
							v = dolv[dolg];
							pushstak(tmp);
						}
					}
				}
			}
			else if (argp)
			{
				if (c == '?')
				{
					trim(argp);	/* DSDe600140 */
					tfailed(id, *argp ? to_char(argp) : (char *)nl_msg(608,badparam));
				}
				else if (c == '=')
				{
					if (n)
					{
						trim(argp);
						assign(n, argp);
					}
					else
						error(nl_msg(610,badsub));
				}
			}
			else if (flags & setflg)
				tfailed(id, nl_msg(609,unset));
			goto retry;
		}
		else
			peekc = c | MARK;
	}
	else if (d == endch)
		return(d);
	else if (d == SQUOTE)
	{
		comsubst();
		goto retry;
	}
	else if (d == DQUOTE)
	{
		quoted++;
		quote ^= QUOTE;
		goto retry;
	}
	return(d);
}

tchar *
macro(as)
tchar	*as;	
{
	/*
	 * Strip "" and do $ substitution
	 * Leaves result on top of stack
	 */
	register BOOL	savqu = quoted;
#ifndef NLS
	register char	savq = quote;
#else NLS
	register int	savq = quote;
#endif NLS
	struct filehdr	fb;

	push(&fb);
	estabf(as);
	usestak();
	quote = 0;
	quoted = 0;
	copyto(0);
	pop();
	if (quoted && (stakbot == staktop))
	{
		pushstak(QUOTE);
	}
/*
 * above is the fix for *'.c' bug
 */
	quote = savq;
	quoted = savqu;
	return(fixstak());
}

static
comsubst()
{
	/*
	 * command substn
	 */
	struct fileblk	cb;
	register tchar	d;		
	register tchar *savptr = fixstak();
	int (*oldsigcld)();

	usestak();
	while ((d = readc()) != SQUOTE && d)
	{
		pushstak(d);
	}
	{
		register tchar	*argc;

		trim(argc = fixstak());
		push(&cb);
		estabf(argc);
	}
	oldsigcld = (int(*)())signal(SIGCLD, SIG_DFL);
	{
		register struct trenod *t = makefork(FPOU, cmd(EOFSYM, MTFLG | NLFLG));
		int		pv[2];

		/*
		 * this is done like this so that the pipe
		 * is open only when needed
		 */
		chkpipe(pv);
		initf(pv[INPIPE]);
		execute(t, 0, (int)(flags & errflg), 0, pv);
		close(pv[OTPIPE]);
	}
	tdystak(savptr);
	staktop = tmovstr(savptr, stakbot);
	while (d = readc())
	{
		pushstak(d | quote);
	}
	await(0, 0);
	signal(SIGCLD, oldsigcld);
	while (stakbot != staktop)
	{
		if ((*--staktop & STRIP) != NL)
		{
			++staktop;
			break;
		}
	}
	pop();
}

#define CPYSIZ	512

subst(in, ot)
int	in, ot;
{
	register tchar	c;	
	struct fileblk	fb;
	register int	count = CPYSIZ;

	push(&fb);
	initf(in);
	/*
	 * DQUOTE used to stop it from quoting
	 */
	while (c = (getch(DQUOTE) & STRIP))
	{
		pushstak(c);
		if (--count == 0)
		{
			flush(ot);
			count = CPYSIZ;
		}
	}
	flush(ot);
	pop();
}

static
flush(ot)
{
#ifndef NLS
#define sav_stakbot stakbot
	write(ot, stakbot, staktop - stakbot);
	if (flags & execpr)
		write(output, stakbot, staktop - stakbot);
#else NLS
/*
 * As we can be called with a very long ponter, we need to write several
 * times, if to_charm can't translate everything at once.
 */

	extern char *to_charm(); 
	char *c_stakbot; 	/* local varible for non-tchar copy NLS */
	tchar	*leftoff = 0;
	int	startlen = -1;
	tchar *sav_stakbot = stakbot;
	tchar *start_ptr = stakbot;
	zerostak();	
	while (stakbot && startlen < 0) {
		c_stakbot = to_charm(start_ptr, &leftoff, &startlen);
		write(ot, c_stakbot, abs(startlen));
		if (flags & execpr)
			write(output, c_stakbot, abs(startlen));
		if (leftoff != 0)
			start_ptr = leftoff;
	}
#endif NLS
	staktop = sav_stakbot;
}
