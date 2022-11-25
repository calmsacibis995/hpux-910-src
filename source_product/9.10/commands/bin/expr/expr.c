static char *HPUX_ID = "@(#) $Revision: 70.3 $";

# define A_STRING 258
# define NOARG 259
# define OR 260
# define AND 261
# define EQ 262
# define LT 263
# define GT 264
# define GEQ 265
# define LEQ 266
# define NEQ 267
# define ADD 268
# define SUBT 269
# define MULT 270
# define DIV 271
# define REM 272
# define MCH 273
# define MATCH 274
# define SUBSTR 275
# define LENGTH 276
# define INDEX 277

#define ESIZE	256
#define error(c)	errxx()
#define EQL(x,y) !strcmp(x,y)
#define Malloc(s,n)	if((s=malloc(n))==0) mallerr()

#include	<stdio.h>

#ifdef NLS
#include	<nl_types.h>
#include	<locale.h>
nl_catd		catd;
#define		NL_SETN		1
#else NLS
#define		catgets(i, j, k, s)	(s)
#endif NLS

#ifdef NLS16
#include	<nl_ctype.h>
#endif NLS16

#include  <regex.h>
regex_t preg;
regmatch_t pmatch[2];

long atol();
char *ltoa(), *strcpy(), *strncpy();
void exit();
char	**Av;
char *buf;
int	Ac;
int	Argi;
int noarg;
int paren;

char Mstring[1][128];
char *malloc();


char *operator[] = { 
	"|", "&", "+", "-", "*", "/", "%", ":",
	"=", "==", "<", "<=", ">", ">=", "!=",
	"match", "substr", "length", "index", "\0" };
int op[] = { 
	OR, AND, ADD,  SUBT, MULT, DIV, REM, MCH,
	EQ, EQ, LT, LEQ, GT, GEQ, NEQ,
	MATCH, SUBSTR, LENGTH, INDEX };
int pri[] = {
	1,2,3,3,3,3,3,3,4,4,5,5,5,6,7,8,9,9};
yylex() {
	register char *p;
	register i;

	if(Argi >= Ac) return NOARG;

	p = Av[Argi];

	if((*p == '(' || *p == ')') && p[1] == '\0' )
		return (int)*p;
	for(i = 0; *operator[i]; ++i)
		if(EQL(operator[i], p))
			return op[i];


	return A_STRING;
}

char *rel(oper, r1, r2) register char *r1, *r2; 
{
	long i;
	long ar1,ar2;

	if(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$"))
	{	
		ar1 = atol(r1);
		ar2 = atol(r2);
		switch(oper) {
		case EQ: 
			i = ar1==ar2; 
			break;
		case GT: 
			i = ar1>ar2; 
			break;
		case GEQ: 
			i = ar1>=ar2; 
			break;
		case LT: 
			i = ar1<ar2; 
			break;
		case LEQ: 
			i = ar1<=ar2; 
			break;
		case NEQ: 
			i = ar1!=ar2; 
			break;
		}
		return i? "1": "0";
	}
	else
		i = strcmp(r1, r2);
	switch(oper) {
	case EQ: 
		i = i==0; 
		break;
	case GT: 
		i = i>0; 
		break;
	case GEQ: 
		i = i>=0; 
		break;
	case LT: 
		i = i<0; 
		break;
	case LEQ: 
		i = i<=0; 
		break;
	case NEQ: 
		i = i!=0; 
		break;
	}
	return i? "1": "0";
}

char *arith(oper, r1, r2) char *r1, *r2; 
{
	long i1, i2;
	register char *rv;

	if(!(ematch(r1, "-\\{0,1\\}[0-9]*$") && ematch(r2, "-\\{0,1\\}[0-9]*$")))
		yyerror(catgets(catd, NL_SETN, 1, "non-numeric argument"));
	i1 = atol(r1);
	i2 = atol(r2);

	switch(oper) {
	case ADD: 
		i1 = i1 + i2; 
		break;
	case SUBT: 
		i1 = i1 - i2; 
		break;
	case MULT: 
		i1 = i1 * i2; 
		break;
	case DIV: 
		if ( ((i1==0) && (i2==0)) ) {
			fputs(catgets(catd,NL_SETN,9,"expr: undefined operation 0 / 0 \n"), stderr );
			exit(2);
		} 
		i1 = i1 / i2; 
		break;
	case REM: 
		if ( ((i1==0) && (i2==0)) ) {
			fputs(catgets(catd,NL_SETN,10,"expr: undefined operation 0 % 0\n"), stderr );
			exit(2); 
		}
		i1 = i1 % i2; 
		break;
	}
	Malloc(rv,16);
	(void) strcpy(rv, ltoa(i1));
	return rv;
}
char *conj(oper, r1, r2) char *r1, *r2; 
{
	register char *rv;

	switch(oper) {

	case OR:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			if(EQL(r2, "0")
			    || EQL(r2, ""))
				rv = "0";
			else
				rv = r2;
		else
			rv = r1;
		break;
	case AND:
		if(EQL(r1, "0")
		    || EQL(r1, ""))
			rv = "0";
		else if(EQL(r2, "0")
		    || EQL(r2, ""))
			rv = "0";
		else
			rv = r1;
		break;
	}
	return rv;
}

char *substr(v, s, w) char *v, *s, *w; 
{
	register si, wi;
	register char *res;

	si = atol(s);
	wi = atol(w);
#ifdef NLS16	/* advance by characters, not single bytes */
	while(--si > 0) if(*v) ADVANCE(v);

	res = v;

	if (wi >= 0) {
		while(wi-- > 0) if(*v) ADVANCE(v);
	}
	else
		while( *v ) ADVANCE(v);
#else NLS16
	while(--si > 0) if(*v) ++v;

	res = v;

	if (wi >= 0) {
		while(wi-- > 0) if(*v) ++v;
	}
	else
		while( *v ) ++v;
#endif NLS16

	*v = '\0';
	return res;
}

char *length(s) register char *s; 
{
	register i = 0;
	register char *rv;

#ifdef NLS16	/* count characters, not bytes */
	while(CHARADV(s)) ++i;
#else NLS16
	while(*s++) ++i;
#endif NLS16

	Malloc(rv,8);
	strcpy(rv, ltoa((long)i));
	return rv;
}

char *index(s, t) char *s, *t; 
{
#ifdef NLS16	/* change from array indexed to string pointers */
	register char_cnt = 1;
	register char *i, *j;
	register char *rv;

	for (i = s; *i; ADVANCE(i), char_cnt++)
		for (j = t; *j; ADVANCE(j))
			if (CHARAT(i) == CHARAT(j)) {
				Malloc(rv,8);
				strcpy(rv, ltoa((long)char_cnt));
#else NLS16
	register i, j;
	register char *rv;

	for(i = 0; s[i] ; ++i)
		for(j = 0; t[j] ; ++j)
			if(s[i]==t[j]) {
			 Malloc(rv,8);
				strcpy(rv, ltoa((long)++i));
#endif NLS16
				return rv;
			}
	return "0";
}

char *match(s, p)
char *s, *p;
{
	register char *rv;

	 Malloc(rv,8);
	(void) strcpy(rv, ltoa((long)ematch(s, p)));
	if(preg.re_nsub) {
		Malloc(rv,(unsigned) strlen(Mstring[0]) + 1);
		(void) strcpy(rv, Mstring[0]);
	}
	return rv;
}

ematch(s, p)
char *s;
register char *p;
{
	register num;
	int err_num;

	if (!*p)				/* empty RE */
		errxx( REG_ENOSEARCH );

	if ( err_num = regcomp(&preg, p, _REG_NOALLOC))
		errxx( err_num );
	if(preg.re_nsub > 1)
		yyerror(catgets(catd, NL_SETN, 2, "Too many '\\('s"));
	preg.__anchor = 1;
	if (!regexec(&preg, s, 2, pmatch, 0)) {
		if(preg.re_nsub == 1) {
			p = (char *)((int)s+pmatch[1].rm_so);
			num = pmatch[1].rm_eo - pmatch[1].rm_so;
			if ((num > 127) || (num < 0)) yyerror(catgets(catd, NL_SETN, 3, "Paren problem"));
			(void) strncpy(Mstring[0], p, num);
			Mstring[0][num] = '\0';
		}
#ifdef NLS16
		{
		char *os;
		os = s;
		for (num = 0; s != os+pmatch[0].rm_eo; ADVANCE(s))
			num++;
		return(num);
		}
#else NLS16
		return(pmatch[0].rm_eo);
#endif NLS16
	}
	return(0);
}

mallerr()
{
	puts(catgets(catd, NL_SETN, 4, "expr: not enough memory"),stderr);
	exit(3);
}

errxx()
{
	yyerror(catgets(catd, NL_SETN, 5, "RE error"));
}

yyerror(s)
char *s;
{
	(void) write(2, "expr: ", 6);
	(void) write(2, s, (unsigned) strlen(s));
	(void) write(2, "\n", 1);
	exit(2);
}

char *expres(prior,par)  int prior, par; 
{
	int ylex, temp=0, op1;
	char *r1, *ra, *rb, *rc;
	ylex = yylex();
	if (ylex >= NOARG && ylex < MATCH) {
		yyerror(catgets(catd, NL_SETN, 6, "syntax error"));
	}
	if (ylex == A_STRING) {
		r1 = Av[Argi++];
		temp = Argi;
	}
	else {
		if (ylex == '(') {
			paren++;
			Argi++;
			r1 = expres(0,Argi);
			Argi--;
		}
	}
lop:
	ylex = yylex();
	if (ylex > NOARG && ylex < MATCH) {
		op1 = ylex;
		Argi++;
		if (pri[op1-OR] <= prior ) 
			return r1;
		else {
			switch(op1) {
			case OR:
			case AND:
				r1 = conj(op1,r1,expres(pri[op1-OR],0));
				break;
			case EQ:
			case LT:
			case GT:
			case LEQ:
			case GEQ:
			case NEQ:
				r1=rel(op1,r1,expres(pri[op1-OR],0));
				break;
			case ADD:
			case SUBT:
			case MULT:
			case DIV:
			case REM:
				r1=arith(op1,r1,expres(pri[op1-OR],0));
				break;
			case MCH:
				r1=match(r1,expres(pri[op1-OR],0));
				break;
			}
			if(noarg == 1) {
				return r1;
			}
			Argi--;
			goto lop;
		}
	}
	ylex = yylex();
	if(ylex == ')') {
		if(par == Argi) {
			yyerror(catgets(catd, NL_SETN, 7, "syntax error"));
		}
		if(par != 0) {
			paren--;
			Argi++;
		}
		Argi++;
		return r1;
	}
	ylex = yylex();
	if(ylex > MCH && ylex <= INDEX) {
		if (Argi == temp) {
			return r1;
		}
		op1 = ylex;
		Argi++;
		switch(op1) {
		case SUBSTR: 
			rc = expres(pri[op1-OR],0);
		case MATCH:
		case INDEX: 
			rb = expres(pri[op1-OR],0);
		case LENGTH: 
			ra = expres(pri[op1-OR],0);
		}
		switch(op1) {
		case MATCH: 
			r1 = match(rb,ra); 
			break;
		case INDEX: 
			r1 = index(rb,ra); 
			break;
		case SUBSTR: 
			r1 = substr(rc,rb,ra); 
			break;
		case LENGTH: 
			r1 = length(ra); 
			break;
		}
		if(noarg == 1) {
			return r1;
		}
		Argi--;
		goto lop;
	}
	ylex = yylex();
	if (ylex == NOARG) {
		noarg = 1;
	}
	return r1;
}
main(argc, argv) char **argv; 
{
	static char expbuf[ESIZE];

#if defined NLS || defined NLS16	/* initialize to the right language */
	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale("expr"), stderr);
		putenv("LANG=");
		catd = (nl_catd)-1;
	} else
		catd = catopen("expr", 0);
#endif NLS || NLS16

	preg.__c_re	= expbuf;
	preg.__c_buf_end	= &expbuf[ESIZE];

	Ac = argc;
	Argi = 1;
	noarg = 0;
	paren = 0;
	Av = argv;
	buf = expres(0,1);
	if(Ac != Argi || paren != 0) {
		yyerror(catgets(catd, NL_SETN, 8, "syntax error"));
	}
	(void) write(1, buf, (unsigned) strlen(buf));
	(void) write(1, "\n", 1);
	exit((!strcmp(buf, "0") || !buf[0])? 1: 0);
}
