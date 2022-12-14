%Start A str sc reg comment

%{
/* $Revision: 70.1 $ */
/*
Copyright (c) 1984, 19885, 1986, 1987 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#include	"awk.h"
#include	"y.tab.h"

#undef	input	/* defeat lex */
#undef	unput

extern YYSTYPE	yylval;
extern int	infunc;
extern int	asgn_op;

int	lineno	= 1;
int	bracecnt = 0;
int	brackcnt  = 0;
int	parencnt = 0;
#define DEBUG
#ifdef	DEBUG
#	define	RET(x)	{if(dbg)printf("lex %s [%s]\n", tokname(x), yytext); return(x); }
#else
#	define	RET(x)	return(x)
#endif

#define	CADD	cbuf[clen++] = yytext[0]; \
		if (clen >= CBUFLEN-1) { \
			error(SYNTAX,ERR77, cbuf); \
			BEGIN A; \
		}

uchar	cbuf[CBUFLEN];
uchar	*s;
int	clen, cflag;
%}

A	[a-zA-Z_]
B	[a-zA-Z0-9_]
D	[0-9]
O	[0-7]
H	[0-9a-fA-F]
WS	[ \t]

%%
	switch (yybgin-yysvec-1) {	/* witchcraft */
	case 0:
		BEGIN A;
		break;
	case sc:
		BEGIN A;
		RET('}');
	}

<A>\n		{ lineno++; RET(NL); }
<A>#.*		{ lineno++; RET(NL); }	/* strip comment lines */
<A>{WS}+	{ ; }
<A>;		{ RET(';'); }

<A>"\\"\n	{ lineno++; }
<A>BEGIN	{ RET(XBEGIN); }
<A>END		{ RET(XEND); }
<A>func(tion)?	{ if (infunc) error(SYNTAX,ERR49); RET(FUNC); }
<A>return	{ if (!infunc) error(SYNTAX,ERR74); RET(RETURN); }
<A>"&&"		{ RET(AND); }
<A>"||"		{ RET(BOR); }
<A>"!"		{ RET(NOT); }
<A>"!="		{ yylval.i = NE; RET(NE); }
<A>"~"		{ yylval.i = MATCH; RET(MATCHOP); }
<A>"!~"		{ yylval.i = NOTMATCH; RET(MATCHOP); }
<A>"<"		{ yylval.i = LT; RET(LT); }
<A>"<="		{ yylval.i = LE; RET(LE); }
<A>"=="		{ yylval.i = EQ; RET(EQ); }
<A>">="		{ yylval.i = GE; RET(GE); }
<A>">"		{ yylval.i = GT; RET(GT); }
<A>">>"		{ yylval.i = APPEND; RET(APPEND); }
<A>"++"		{ yylval.i = INCR; RET(INCR); }
<A>"--"		{ yylval.i = DECR; RET(DECR); }
<A>"+="		{ yylval.i = ADDEQ; RET(ASGNOP); }
<A>"-="		{ yylval.i = SUBEQ; RET(ASGNOP); }
<A>"*="		{ yylval.i = MULTEQ; RET(ASGNOP); }
<A>"/="		{
			if (asgn_op==0) {
				unput(yytext[1]);
				unput(yytext[0]);
				RET(MAYBEDIVEQ);
			} else {
				asgn_op=0;
				yylval.i = DIVEQ;
				RET(ASGNOP);
			}
		}
<A>"%="		{ yylval.i = MODEQ; RET(ASGNOP); }
<A>"^="		{ yylval.i = POWEQ; RET(ASGNOP); }
<A>"**="	{ yylval.i = POWEQ; RET(ASGNOP); }
<A>"="		{ yylval.i = ASSIGN; RET(ASGNOP); }
<A>"**"		{ RET(POWER); }
<A>"^"		{ RET(POWER); }

<A>"$"{D}+	{ yylval.cp = fieldadr(atoi(yytext+1)); RET(FIELD); }
<A>"$NF"	{ unputstr("(NF)"); return(INDIRECT); }
<A>"$"{A}{B}*	{ int c, n;
		  c = input(); unput(c);
		  if (c == '(' || c == '[' || infunc && (n=isarg(yytext+1)) >= 0) {
			unputstr(yytext+1);
			return(INDIRECT);
		  } else {
			yylval.cp = setsymtab(yytext+1,"",0.0,STR|NUM,symtab);
			RET(IVAR);
		  }
		}
<A>"$"		{ RET(INDIRECT); }
<A>NF		{ yylval.cp = setsymtab(yytext, "", 0.0, NUM, symtab); RET(VARNF); }

<A>({D}+("."?){D}*|"."{D}+)((e|E)("+"|-)?{D}+)?	{
		  yylval.cp = setsymtab(yytext, tostring(yytext), atof(yytext), NUMCON|CON|NUM, symtab);
		/* should this also have STR set? */
		  RET(NUMBER); }

<A>while	{ RET(WHILE); }
<A>for		{ RET(FOR); }
<A>do		{ RET(DO); }
<A>if		{ RET(IF); }
<A>else		{ RET(ELSE); }
<A>next		{ RET(NEXT); }
<A>exit		{ RET(EXIT); }
<A>break	{ RET(BREAK); }
<A>continue	{ RET(CONTINUE); }
<A>print	{ yylval.i = PRINT; RET(PRINT); }
<A>printf	{ yylval.i = PRINTF; RET(PRINTF); }
<A>sprintf	{ yylval.i = SPRINTF; RET(SPRINTF); }
<A>split	{ yylval.i = SPLIT; RET(SPLIT); }
<A>substr	{ RET(SUBSTR); }
<A>sub		{ yylval.i = SUB; RET(SUB); }
<A>gsub		{ yylval.i = GSUB; RET(GSUB); }
<A>index	{ RET(INDEX); }
<A>match	{ RET(MATCHFCN); }
<A>in		{ RET(IN); }
<A>getline	{ RET(GETLINE); }
<A>close	{ RET(CLOSE); }
<A>delete	{ RET(DELETE); }
<A>length	{ yylval.i = FLENGTH; RET(BLTIN); }
<A>log		{ yylval.i = FLOG; RET(BLTIN); }
<A>int		{ yylval.i = FINT; RET(BLTIN); }
<A>exp		{ yylval.i = FEXP; RET(BLTIN); }
<A>sqrt		{ yylval.i = FSQRT; RET(BLTIN); }
<A>sin		{ yylval.i = FSIN; RET(BLTIN); }
<A>cos		{ yylval.i = FCOS; RET(BLTIN); }
<A>atan2	{ yylval.i = FATAN; RET(BLTIN); }
<A>system	{ yylval.i = FSYSTEM; RET(BLTIN); }
<A>rand		{ yylval.i = FRAND; RET(BLTIN); }
<A>srand	{ yylval.i = FSRAND; RET(BLTIN); }
<A>toupper      { yylval.i = FTOUPPER; RET(BLTIN); }
<A>tolower      { yylval.i = FTOLOWER; RET(BLTIN); }


<A>{A}{B}*	{ int n, c;
		  c = input(); unput(c);	/* look for '(' */
		  if (c != '(' && infunc && (n=isarg(yytext)) >= 0) {
			yylval.i = n;
			RET(ARG);
		  } else {
			yylval.cp = setsymtab(yytext,"",0.0,STR|NUM,symtab);
			if (c == '(') {
				RET(CALL);
			} else {
				RET(VAR);
			}
		  }
		}
<A>\"		{ if (do_str() == 0) RET(STRING); } /* NLS16 - see do_str() below */

<A>"}"		{ if (--bracecnt < 0) error(SYNTAX,ERR38); BEGIN sc; RET(';'); }
<A>"]"		{ if (--brackcnt < 0) error(SYNTAX,ERR37); RET(']'); }
<A>")"		{ if (--parencnt < 0) error(SYNTAX,ERR36); RET(')'); }

<A>.		{ if (yytext[0] == '{') bracecnt++;
		  else if (yytext[0] == '[') brackcnt++;
		  else if (yytext[0] == '(') parencnt++;
		  RET(yylval.i = yytext[0]); /* everything else */ }

<reg>\\.        {
#ifdef NLS16
                cbuf[clen++] = '\\';
                if (clen>=2 && FIRSTof2(cbuf[clen-2]) && SECof2('\\'))
                        unput(yytext[1]);
                else
                        cbuf[clen++] = yytext[1];
#else /* NLS16 */
                cbuf[clen++] = '\\'; cbuf[clen++] = yytext[1];
#endif /* NLS16 */
		}
<reg>\n		{ error(SYNTAX,ERR58, cbuf); lineno++; BEGIN A; }
<reg>"/"	{ BEGIN A;
		  cbuf[clen] = 0;
		  yylval.s = tostring(cbuf);
		  unput('/');
		  RET(REGEXPR); }
<reg>.		{ CADD; }
%%

startreg()
{
	BEGIN reg;
	clen = 0;
}

startreg2()
{
	BEGIN reg;
	cbuf[0] = '=';
	clen = 1;
	(void)input(); /* get '/' */
	(void)input(); /* get '=' */
}

/* input() and unput() are transcriptions of the standard lex
   macros for input and output with additions for error message
   printing.  God help us all if someone changes how lex works.
*/

uchar	ebuf[300];
uchar	*ep = ebuf;

input()
{
	register c;
	extern uchar *lexprog;

	if (yysptr > yysbuf)
		c = U(*--yysptr);
	else if (lexprog != NULL) {	/* awk '...' */
		if (c = *lexprog)
			lexprog++;
	} else				/* awk -f ... */
		c = pgetc();
	if (c == '\n')
		yylineno++;
	else if (c == EOF)
		c = 0;
	if (ep >= ebuf + sizeof ebuf)
		ep = ebuf;
	return *ep++ = c;
}

unput(c)
{
	yytchar = c;
	if (yytchar == '\n')
		yylineno--;
	*yysptr++ = yytchar;
	if (--ep < ebuf)
		ep = ebuf + sizeof(ebuf) - 1;
}


unputstr(s)
	char *s;
{
	int i;

	for (i = strlen(s)-1; i >= 0; i--)
		unput(s[i]);
}



static char from[] =  "\"antbrfv\\";
static char to[]   =  "\"\007\n\t\b\r\f\v\\";

#define XCADD(c)	{cbuf[clen++]=(c); if(clen>=CBUFLEN-1) {error(SYNTAX,ERR76, cbuf); }}

/* NLS16 - string processing moved here to simplify handling of kanji */
do_str()
{
	uchar *s;
	register int c;
	int n,old;
	char *strchr();

	clen = 0;
	while ((c = input()) != '"'){
		if (c == '\n' || c == '\0') {
		nlerror:
			error(SYNTAX,ERR59, cbuf);
			lineno++; 
			return(1);
		}
		else if (c == '\\') {
			c = input();
			if (c == '\n' || c == '\0')
				goto nlerror;
			if (member(c, from)) {
				XCADD(*(strchr(from,c) - from + to))
			} else if (c=='x') {
				old = n = 0;	
				while (1) {
					c=input();
					if (c>=0 && c<=9)
						n=n*16+c-'0';
					else if (c>='a'&&c<='f')
						n=n*16+c-'a'+10;
					else if (c>='A'&&c<='F')
						n=n*16+c-'A'+10;
					else {
						unput(c);
						if (old) {
							XCADD(n);
						}
						else {
							unput('x');
							XCADD('\\');
						}
						break;
					}
				}
			} else if (c>='0' && c<='7') {
				n=c-'0';
				c=input();
				if (c>='0' && c<='7') {
					n=n*8+(c-'0');
					c=input();
					if (c>='0' && c<='7')
						n=n*8+(c-'0');
					else
						unput(c);
					XCADD(n);
				} else {
					unput(c);
					XCADD(n);
				}
			}
			else {	/* Unlike C, backslash didn't disappear */
				/* "Feature" preserved for compatibility */
				XCADD('\\');
				unput(c);
			}
		}
		else {
			if (FIRSTof2(c)){
				XCADD(c);
				/* avoid seeing 2nd byte as '"' or '\' */
				if ((c = input()) == '\n' || c == '\0')
					goto nlerror;
				else {
					XCADD(c);
				}

			}
			else {
				XCADD(c);
			}
		}
	}
	cbuf[clen]=0;
	s = tostring(cbuf);
	cbuf[clen] = ' ';
	cbuf[++clen] = 0;
	yylval.cp = setsymtab(cbuf, s, 0.0, STRCON|CON|STR, symtab);
	return(0);
}
