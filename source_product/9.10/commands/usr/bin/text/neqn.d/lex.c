/* @(#) $Revision: 66.1 $ */     
#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <msgbuf.h>
#endif NLS
#include "e.h"
#include "e.def"

#define	SSIZE	400
char	token[SSIZE];
int	sp;
#define	putbak(c)	*ip++ = c;
#define	PUSHBACK	300	/* maximum pushback characters */
#ifdef NLS16
/* Prepare room for 16-bit code flag. */
int	ibuf[PUSHBACK+SSIZE];	/* pushback buffer for definitions, etc. */
int	*ip	= ibuf;
#else
char	ibuf[PUSHBACK+SSIZE];	/* pushback buffer for definitions, etc. */
char	*ip	= ibuf;
#endif

#ifdef NLS16
/* Getc() set 16-bit code flag. */
int	hp15_2nd = 0;
int	illegal_code = 0;
int Getc(fdes) 
FILE *fdes;
{
	int ch;
	int ch2;
	ch=getc(fdes);
	if ( ch == EOF ) {
		return(EOF);
	}
	if ( illegal_code ) {
		illegal_code = 0;
	} else if ( hp15_2nd ) {
		hp15_2nd = 0;
		ch |= HP15_2nd;
	} else {
		if ( FIRSTof2(ch&0377) ) {
			ch2 = getc(fdes);			
			ungetc(ch2,fdes);
			if ( SECONDof2(ch2&0377) ) {
				hp15_2nd = 1;
				ch |= HP15_1st;
			} else {
				illegal_code = 1;
			}
		}
	}
	return(ch);
}
#endif
gtc() {
  loop:
	if (ip > ibuf)
		return(*--ip);	/* already present */
#ifdef NLS16
	/* Call gGetc() to set 16-bit code flag. */
	lastchar = Getc(curfile);
#else
	lastchar = getc(curfile);
#endif
	if (lastchar=='\n')
		linect++;
	if (lastchar != EOF)
		return(lastchar);
	if ((strcmp(svargv[ifile],"-")==0)||(++ifile > svargc)) {
		return(EOF);
	}
	fclose(curfile);
	linect = 1;
	if (strcmp(svargv[ifile], "-")==0)
		curfile=stdin;
	else 
		curfile=fopen(svargv[ifile],"r");
	if (curfile != NULL)
		goto loop;
	error(FATAL, (nl_msg(3, "can't open file %s")), svargv[ifile]);
	return(EOF);
}

pbstr(str)
register char *str;
{
	register char *p;

	p = str;
	while (*p++);
	--p;
	if (ip >= &ibuf[PUSHBACK])
		error( FATAL, (nl_msg(4, "pushback overflow")));
	while (p > str)
		putbak(*--p);
}

yylex() {
	register int c;
	tbl *tp, *lookup();
#ifdef NLS16
	extern tbl *keytbl[], *deftbl[];
#else
	extern tbl **keytbl, **deftbl;
#endif

  beg:
	while ((c=gtc())==' ' || c=='\n')
		;
	yylval=c;
	switch(c) {

	case EOF:
		return(EOF);
	case '~':
		return(SPACE);
	case '^':
		return(THIN);
	case '\t':
		return(TAB);
	case '{':
		return('{');
	case '}':
		return('}');
	case '"':
		for (sp=0; (c=gtc())!='"' && c != '\n'; ) {
			if (c == '\\')
				if ((c = gtc()) != '"')
					token[sp++] = '\\';
			token[sp++] = c;
			if (sp>=SSIZE)
				error(FATAL, (nl_msg(5, "quoted string %.20s... too long")), token);
		}
		token[sp]='\0';
		yylval = (int) &token[0];
		if (c == '\n')
			error(!FATAL, (nl_msg(6, "missing \" in %.20s")), token);
		return(QTEXT);
	}
	if (c==righteq)
		return(EOF);

	putbak(c);
	getstr(token, SSIZE);
	if (dbg)printf(".\tlex token = |%s|\n", token);
#ifdef NLS16
	if ((tp = lookup(deftbl, token, NULL)) != NULL) {
#else
	if ((tp = lookup(&deftbl, token, NULL)) != NULL) {
#endif
		putbak(' ');
		pbstr(tp->defn);
		putbak(' ');
		if (dbg)
			printf(".\tfound %s|=%s|\n", token, tp->defn);
	}
#ifdef NLS16
	else if ((tp = lookup(keytbl, token, NULL)) == NULL) {
#else
	else if ((tp = lookup(&keytbl, token, NULL)) == NULL) {
#endif
		if(dbg)printf(".\t%s is not a keyword\n", token);
		return(CONTIG);
	}
	else if (tp->defn == (char *) DEFINE || tp->defn == (char *) NDEFINE || tp->defn == (char *) TDEFINE)
		define(tp->defn);
	else if (tp->defn == (char *) DELIM)
		delim();
	else if (tp->defn == (char *) GSIZE)
		globsize();
	else if (tp->defn == (char *) GFONT)
		globfont();
	else if (tp->defn == (char *) INCLUDE)
		include();
	else {
		return((int) tp->defn);
	}
	goto beg;
}

getstr(s, n) char *s; register int n; {
	register int c;
	register char *p;

	p = s;
	while ((c = gtc()) == ' ' || c == '\n')
		;
	if (c == EOF) {
		*s = 0;
		return;
	}
	while (c != ' ' && c != '\t' && c != '\n' && c != '{' && c != '}'
	  && c != '"' && c != '~' && c != '^' && c != righteq) {
		if (c == '\\')
			if ((c = gtc()) != '"')
				*p++ = '\\';
		*p++ = c;
		if (--n <= 0)
			error(FATAL, (nl_msg(7, "token %.20s... too long")), s);
		c = gtc();
	}
	if (c=='{' || c=='}' || c=='"' || c=='~' || c=='^' || c=='\t' || c==righteq)
		putbak(c);
	*p = '\0';
	yylval = (int) s;
}

cstr(s, quote, maxs) char *s; int quote; {
	int del, c, i;

	while((del=gtc()) == ' ' || del == '\t' || del == '\n');
#ifdef NLS16
	/* Cut 16-bit flag to be specified illegally. */
	del &= 0377;	
#endif
	if (quote)
		for (i=0; (c=gtc()) != del && c != EOF;) {
			s[i++] = c;
			if (i >= maxs)
				return(1);	/* disaster */
		}
	else {
		s[0] = del;
		for (i=1; (c=gtc())!=' ' && c!= '\t' && c!='\n' && c!=EOF;) {
			s[i++]=c;
			if (i >= maxs)
				return(1);	/* disaster */
		}
	}
	s[i] = '\0';
	if (c == EOF)
		error(FATAL, (nl_msg(8, "Unexpected end of input at %.20s")), s);
	return(0);
}

define(type) int type; {
	char *strsave(), *p1, *p2;
	tbl *lookup();
#ifdef NLS16
	extern tbl *deftbl[];
#else
	extern tbl **deftbl;
#endif

	getstr(token, SSIZE);	/* get name */
	if (type != DEFINE) {
		cstr(token, 1, SSIZE);	/* skip the definition too */
		return;
	}
	p1 = strsave(token);
	if (cstr(token, 1, SSIZE))
		error(FATAL, (nl_msg(9, "Unterminated definition at %.20s")), token);
	p2 = strsave(token);
#ifdef NLS16
	lookup(deftbl, p1, p2);
#else
	lookup(&deftbl, p1, p2);
#endif
	if (dbg)printf(".\tname %s defined as %s\n", p1, p2);
}

char *strsave(s)
char *s;
{
	char *malloc();
	register char *q;

	q = malloc(strlen(s)+1);
	if (q == NULL)
		error(FATAL, (nl_msg(10, "out of space in strsave on %s")), s);
	strcpy(q, s);
	return(q);
}

include() {
	error(!FATAL, (nl_msg(11, "Include not yet implemented")));
}

delim() {
	yyval = eqnreg = 0;
	if (cstr(token, 0, SSIZE))
		error(FATAL, (nl_msg(12, "Bizarre delimiters at %.20s")), token);
	lefteq = token[0];
	righteq = token[1];
	if (lefteq == 'o' && righteq == 'f')
		lefteq = righteq = '\0';
}
