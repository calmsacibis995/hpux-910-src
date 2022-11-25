/* @(#) $Revision: 66.1 $ */    

/****************************************************************************

	DEBUGGER - expression evaluation

****************************************************************************/
#include "defs.h"

MSG		BADSYM;
MSG		BADVAR;
MSG		BADKET;
MSG		BADSYN;
MSG		NOCFN;
MSG		NOADR;
MSG		BADLOC;
MSG		BADREAL;

extern FILE 	*infile;		/* file for commands MFM */
REGLIST		reglist[];
int		fcor;
SYMPTR		symbol;

CHAR		*lp;
STRING		errflg;
CHAR		isymbol[NCHNAM];		/* MFM */

CHAR		lastc;
POS		*endhdr;

long		dot;
long		ditto;
int		dotinc;
long		var[];
long		expv;
int		ibase=16;
short		real_num;		/* flag to indicate real number */
int		digit(),hexdigit(),octdigit();

expr(a)					/* term | term dyadic expr |  */
{
	int	rc;
	long	lhs;

	rdc(); lp--; rc = term(a);
	while (rc)
	{
		lhs = expv;
		switch (readchar())
		{
		    case '+':
			term(a|1); expv += lhs; break;

		    case '-':
			term(a|1); expv = lhs - expv; break;

		    case '#':
			term(a|1); expv = round(lhs,expv); break;

		    case '*':
			term(a|1); expv *= lhs; break;

		    case '%':
			term(a|1); expv = lhs/expv; break;

		    case '&':
			term(a|1); expv &= lhs; break;

		    case '|':
			term(a|1); expv |= lhs; break;

		    case ')':
			if ((a&2) == 0) error(BADKET);

		    default:
			lp--;
			return(rc);
		}
	}
	return(rc);
}

term(a)					/* item | monadic item | (expr) | */
{
	switch (readchar())
	{
		case '*':
			term(a|1); 
			expv = getword(expv, DSP);
			return(1);

		case '@':
			term(a|1);
			expv = getword(expv, ISP);
			return(1);

		case '-':
			term(a|1); 
			expv = (real_num ? fneg(expv) : -expv );
			return(1);

		case '~':
			term(a|1); expv = ~expv; return(1);

		case '(':
			expr(2);
			if (*lp != ')') error(BADSYN);
			else {lp++; return(1);}

		default:
			lp--;
			return(item(a));
	}
}

/* name [ . local ] | number | . | ^ | <var | <register | 'x | | */
item(a)
{
	int	base, d, frpt, regptr;
	CHAR	savc;
	long	frame;
	SYMPTR	symp;
	int (*gooddigit)();
	extern int dtof();
	extern double atof();
	register char savlc;
	register char *savlp;

	real_num = 0;
	readchar();
	if (symchar(0))
	{
		readsym();
		if (lastc == '.') error(BADSYM);
		symp = lookupsym(isymbol);
		if (!symp && ibase==16 && hexdigit(isymbol[0])) expv = nval();
		else if (symp) expv = symp->vals;
		else error(BADSYM);
		lp--;
	}
	else if (digit(lastc))
	{
		savlc = lastc;
		savlp = lp;
		while (digit(lastc)) readchar();
		if (lastc == '.')
		{	readchar();
			while (digit(lastc)) readchar();
			expv = dtof(atof(savlp-1));
			real_num = 1;
			lp--;
			return 1;
		}
		lp = savlp;
		lastc = savlc;

		if (lastc == '0')
		{	readchar();
			if (lastc == 'x' || lastc == 'X')
			{	base = 16;
				readchar();
			}
			else if (lastc == 'd' || lastc == 'D')
			{	base = 10;
				readchar();
			}
			else
			{	base = 8;
			}
		}
		else base = ibase;
		switch (base)
		{	case  8: gooddigit = octdigit;
				break;
			case 10: gooddigit = digit;
				break;
			case 16: gooddigit = hexdigit;
				break;
		}

		expv = 0;
		while ((*gooddigit)(lastc))
		{
			expv *= base;
			if ((d = convdig(lastc)) >= base) error(BADSYN);
			expv += d; readchar();
		}
		lp--;
	}
	else if (lastc == '.')
	{
		readchar();
		if (symchar(0)) error(BADSYM);
		else expv = dot;
		lp--;
	}
	else if (lastc == '"') expv = ditto;
	else if (lastc == '+') expv = inkdot(dotinc);
	else if (lastc == '^') expv = inkdot(-dotinc);
	else if (lastc == '<')
	{
		savc = rdc();
		if ((regptr = getroffs(savc)) != -1)
			expv = reglist[regptr].rval;
		else if ((base=varchk(savc)) != -1) expv = var[base];
		else error(BADVAR);
	}
	else if (lastc == '\'')
	{
		d = 4; expv = 0;
		while (quotchar())
		if (d--)
		{
			expv <<= 8;
			expv |= lastc;
		}
		else error(BADSYN);
	}
	else if (a) error(NOADR);
	else
	{
		lp--; return(0);
	}
	return(1);
}


/* nval builds a hex number. assumes the symbol cannot be an identifier */
nval()
{
	register short i;			/* MFM */
	register int val = 0;			/* MFM */
	int d;

	for (i = 0; i < MAXHDIGITS; i++) if (isymbol[i])	/* MFM */
	{
		val *= 16;
		if ((d = convdig(isymbol[i])) >= 16) error(BADSYN);
		val += d;
	}
	else break;
	return(val);
}

readsym()
{
	char	*p = isymbol;

	do
	{
		if (p < &isymbol[NCHNAM]) *p++ = lastc;
		readchar();
	}
	while (symchar(1));
	while (p < &isymbol[NCHNAM]) *p++ = 0;
}

SYMPTR
lookupsym(symstr)
char	*symstr;
{
	register SYMPTR	symp;		/* MFM */
	char tsym[NCHNAM];

	symset();
	while((symp = symget()) != NULL)
		if (((symp->symf & SYMCHK) == symp->symf) &&
		eqsym(symp->symc, symstr, '.')) return(symp);
	strcpy(tsym,"_");
	strcat(tsym,symstr);
	symset();
	while((symp = symget()) != NULL)
		if (((symp->symf & SYMCHK) == symp->symf) &&
		eqsym(symp->symc, tsym, '.')) return(symp);
	return(NULL);
}

hexdigit(c)
register char	c;			/* MFM */
{
	return((c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F'));
}

convdig(c)
register char	c;			/* MFM */
{
	if (digit(c)) return(c-'0');
	else if (hexdigit(c))
	{
		if (c >= 'a') return(c-'a'+10);
		else return(c-'A'+10);
	}
	else return(16);
}

digit(c)
char	c;
{
	return(c>='0' && c<='9');
}

octdigit(c)
char	c;
{
	return(c>='0' && c<='7');
}

letter(c)
register char	c;			/* MFM */
{
	return((c>='a' && c<='z') || (c>='A' && c<='Z'));
}

symchar(dig)
{
	if (lastc=='\\')
	{ readchar();
	  return(TRUE);
	}
	return( letter(lastc) || lastc=='_' || dig && digit(lastc) );
}

varchk(name)
	register name;			/* MFM */
{
	if (digit(name)) return(name-'0');
	if (letter(name)) return((name&037)-1+10);
	return(-1);
}

eqsym(s1, s2, c)
register STRING	s1, s2;
CHAR		c;
{
	if (strcmp(s1, s2) == 0) return(TRUE);
	else if (*s1==c)
	{	CHAR		s3[NCHNAM];
		register short	i;		/* MFM */
		s3[0]=c;
		for (i=1; i<NCHNAM; i++) s3[i] = *s2++;
		return(strcmp(s1,s3)==0);
	}
	else return(FALSE);
}


real_bad()
{
	error(BADREAL);
}

