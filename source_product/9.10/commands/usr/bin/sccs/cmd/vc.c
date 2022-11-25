/* @(#) $Revision: 64.2 $ */    
static char *HPUX_ID = "@(#) $Revision: 64.2 $";
#ifdef NLS
#define NL_SETN  10
#include	<msgbuf.h>
#include	<locale.h>
#else
#define nl_msg(i, s) (s)
#endif

# include	"stdio.h"
# include	"sys/types.h"
# include	"macros.h"
# include	<setjmp.h>
# include	"fatal.h"

#ifdef NLS16
#include <nl_ctype.h>
#endif NLS16

/*
 * The symbol table size is set to a limit of forty keywords per input
 * file.  Should this limit be changed it should also be changed in the
 * Help file.
 */

# define SYMSIZE 40
# define PARMSIZE 10
# define NSLOTS 32

# define USD  1
# define DCL 2
# define ASG 4

# define EQ '='
# define NEQ '!'
# define GT '>'
# define LT '<'
# define DELIM " \t"
# define TRUE 1
# define FALSE 0

char Error[128];

#ifdef NLS16		/* has to be big enough for 16 bit kanji */
int	Ctlchar = ':';
#else NLS16
char	Ctlchar = ':';
#endif NLS16

struct	symtab	{
	int	usage;
	char	name[PARMSIZE];
	char	*value;
	int	lenval;
};
struct	symtab	Sym[SYMSIZE];


int	Skiptabs;
int	Repall;

/*
 * Delflag is used to indicate when text is to be skipped.  It is decre-
 * mented whenever an if condition is false, or when an if occurs
 * within a false if/end statement.  It is decremented whenever an end is
 * encountered and the Delflag is greater than zero.  Whenever Delflag
 * is greater than zero text is skipped.
 */

int	Delflag;

/*
 * Ifcount keeps track of the number of ifs and ends.  Each time
 * an if is encountered Ifcount is incremented and each time an end is
 * encountered it is decremented.
 */

int	Ifcount;
int	Lineno;

char	*Repflag;
char	*Linend;
int	Silent;
char	*getid(), *replace();
char	*findch(), *ecopy(), *findstr();
char	*fmalloc();


/*
 * The main program reads a line of text and sends it to be processed
 * if it is a version control statement. If it is a line of text and
 * the Delflag is equal to zero, it is written to the standard output.
 */

main(argc, argv)
int argc;
char *argv[];
{
	register  char *lineptr, *p;
	register int i;
	char line[BUFSIZ];
	extern int Fflags;

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("vc"), stderr);
		putenv("LANG=");
	}
	nl_catopen("sccs");
#endif NLS || NLS16

	Fflags = FTLCLN | FTLMSG | FTLEXIT;
	setsig();
	for(i = 1; i< argc; i++) {
		p = argv[i];
		if (p[0] == '-')
			switch (p[1]) {
			case 's':
				Silent = 1;
				break;
			case 't':
				Skiptabs = 1;
				break;
			case 'a':
				Repall = 1;
				break;
			case 'c':
#ifdef NLS16	/* get the entire char (in case it is a kanji) */
				Ctlchar = CHARAT(p+2);
#else NLS16
				Ctlchar = p[2];
#endif NLS16
				break;
			}
		else {
			p[size(p) - 1] = '\n';
			asgfunc(p);
		}
	}
	while (fgets(line,sizeof(line),stdin) != NULL) {
		lineptr = line;
		Lineno++;

		if (Repflag != 0) {
			ffree(Repflag);
			Repflag = 0;
		}

		if (Skiptabs) {
			for (p = lineptr; *p; p++)
				if (*p == '\t')
					break;
			if (*p++ == '\t')
				lineptr = p;
		}

#ifdef NLS16	/* do the compares on proper 8/16 bit char's, as appropriate */
		if (CHARAT(lineptr) != Ctlchar) {
			if (lineptr[0] == '\\' && CHARAT(lineptr+1) == Ctlchar)
#else NLS16
		if (lineptr[0] != Ctlchar) {
			if (lineptr[0] == '\\' && lineptr[1] == Ctlchar)
#endif NLS16
				for (p = &lineptr[1]; *lineptr++ = *p++; )
					;
			if(Delflag == 0) {
				if (Repall)
					repfunc(line);
				else
					fputs(line,stdout);
			}
			continue;
		}

#ifdef NLS16	/* 1st char is a control char, advance past it */
		ADVANCE(lineptr);
#else NLS16
		lineptr++;
#endif NLS16

		if (imatch("if ", lineptr))
			iffunc(&lineptr[3]);
		else if (imatch("end", lineptr))
			endfunc();
		else if (Delflag == 0) {
			if (imatch("asg ", lineptr))
				asgfunc(&lineptr[4]);
			else if (imatch("dcl ", lineptr))
				dclfunc(&lineptr[4]);
			else if (imatch("err", lineptr))
				errfunc(&lineptr[3]);
			else if (imatch("msg", lineptr))
				msgfunc(&lineptr[3]);
#ifdef NLS16	/* Retrieve & advance by the proper char width */
			else if (CHARAT(lineptr) == Ctlchar) {
				ADVANCE(lineptr);
				repfunc(lineptr);
			}
#else NLS16
			else if (lineptr[0] == Ctlchar)
				repfunc(&lineptr[1]);
#endif NLS16
			else if (imatch("on", lineptr))
				Repall = 1;
			else if (imatch("off", lineptr))
				Repall = 0;
			else if (imatch("ctl ", lineptr))
#ifdef NLS16	/* Retrieve the proper size char */
				Ctlchar = CHARAT(lineptr+4);
#else NLS16
				Ctlchar = lineptr[4];
#endif NLS16
			else {

				sprintf(Error,nl_msg(1,"unknown command on line %d"),Lineno);
				fatal(strcat(Error," (vc1)"));
			}
		}
	}
	for(i = 0; Sym[i].usage != 0 && i<SYMSIZE; i++) {
		if ((Sym[i].usage&USD) == 0 && !Silent)

			{ fprintf(stderr,nl_msg(2,"`%s' never used"),Sym[i].name);
			fprintf(stderr, " (vc2)\n"); }
		if ((Sym[i].usage&DCL) == 0 && !Silent)
			{ fprintf(stderr,nl_msg(3,"`%s' never declared"),
				Sym[i].name);
			fprintf(stderr, " (vc3)\n"); }
		if ((Sym[i].usage&ASG) == 0 && !Silent)
			{ fprintf(stderr,nl_msg(4,"`%s' never assigned a value"),
				Sym[i].name);
			fprintf(stderr, " (vc20)\n"); }
	}
	if (Ifcount > 0)
	{	
		sprintf(Error,"%s (vc4)",nl_msg(5,"`if' with no matching `end'"));
		fatal(Error);
	}
	exit(0);

}


/*
 * Asgfunc accepts a pointer to a line picks up a keyword name, an
 * equal sign and a value and calls putin to place it in the symbol table.
 */

asgfunc(aptr)
register char *aptr;
{
	register char *end, *aname;
	char *avalue;

	aptr = replace(aptr);
	NONBLANK(aptr);
	aname = aptr;
	end = Linend;
	aptr = findstr(aptr,"= \t");
	if (*aptr == ' ' || *aptr == '\t') {
		*aptr++ = '\0';
		aptr = findch(aptr,'=');
	}
	if (aptr == end) {

		sprintf(Error,nl_msg(6,"syntax on line %d"),Lineno);
		fatal(strcat(Error," (vc17)"));
	}
	*aptr++ = '\0';
	avalue = getid(aptr);
	chksize(aname);
	putin(aname, avalue);
}


/*
 * Dclfunc accepts a pointer to a line and picks up keywords
 * separated by commas.  It calls putin to put each keyword in the
 * symbol table.  It returns when it sees a newline.
 */

dclfunc(dptr)
register char *dptr;
{
	register char *end, *dname;
	int i;

	dptr = replace(dptr);
	end = Linend;
	NONBLANK(dptr);
	while (dptr < end) {
		dname = dptr;
		dptr = findch(dptr,',');
		*dptr++ = '\0';
		chksize(dname);
		if (Sym[i = lookup(dname)].usage&DCL) {
#ifdef NLS 
			sprintmsg(Error,(nl_msg(7,"`%1$s' declared twice on line %2$d")), dname, Lineno);
			strcat(Error," (vc5)");
#else
			sprintf(Error,"`%s' declared twice on line %d (vc5)", 
				dname, Lineno);
#endif  
			fatal(Error);
		}
		else
			Sym[i].usage |= DCL;
		NONBLANK(dptr);
	}
}


/*
 * Errfunc calls fatal which stops the process.
 */

errfunc(eptr)
char *eptr;
{
	if (!Silent)
		{  fprintf(stderr,nl_msg(8,"ERROR:%s"),replace(eptr));
		fprintf(stderr, "\n"); }
	sprintf(Error,nl_msg(9,"err statement on line %d"), Lineno);
	fatal(strcat(Error," (vc15)"));
}


/*
 * Endfunc indicates an end has been found by decrementing the if count
 * flag.  If because of a previous if statement, text was being skipped,
 * Delflag is also decremented.
 */

endfunc()
{
	if (--Ifcount < 0) {

		sprintf(Error,nl_msg(10,"`end' without matching `if' on line %d"),Lineno);
		fatal(strcat(Error," (vc10)")); 
	}
	if (Delflag > 0)
		Delflag--;
	return;
}


/*
 * Msgfunc accepts a pointer to a line and prints that line on the 
 * diagnostic output.
 */

msgfunc(mptr)
char *mptr;
{
	if (!Silent)
#ifdef NLS
		{ fprintmsg(stderr,nl_msg(11,"Message(%1$d):%2$s"), Lineno, replace(mptr));
		fprintf(stderr, "\n"); }
#else
		fprintf(stderr,"Message(%d):%s\n", Lineno, replace(mptr));
#endif
}


repfunc(s)
char *s;
{
	fprintf(stdout,"%s\n",replace(s));
}


/*
 * Iffunc and the three functions following it, door, doand, and exp
 * are responsible for parsing and interperting the condition in the
 * if statement.  The BNF used is as follows:
 *	<iffunc> ::=   [ "not" ] <door> EOL
 *	<door> ::=     <doand> | <doand> "|" <door>
 *	<doand>::=     <exp> | <exp> "&" <doand>
 *	<exp>::=       "(" <door> ")" | <value> <operator> <value>
 *	<operator>::=  "=" | "!=" | "<" | ">"
 * And has precedence over or.  If the condition is false the Delflag
 * is bumped to indicate that lines are to be skipped.
 * An external variable, sptr is used for processing the line in
 * iffunc, door, doand, exp, getid.
 * Iffunc accepts a pointer to a line and sets sptr to that line.  The
 * rest of iffunc, door, and doand follow the BNF exactly.
 */

char *sptr;

iffunc(iptr)
char *iptr;
{
	register int value, not;

	Ifcount++;
	if (Delflag > 0)
		Delflag++;

	else {
		sptr = replace(iptr);
		NONBLANK(sptr);
		if (imatch("not ", sptr)) {
			not = FALSE;
			sptr += 4;
		}
		else not = TRUE;

		value = door();
		if( *sptr != 0) {

			sprintf(Error,nl_msg(13,"syntax on line %d"),Lineno);
			fatal(strcat(Error," (vc18)"));
		}

		if (value != not)
			Delflag++;
	}

	return;
}


door()
{
	int value;
	value = doand();
	NONBLANK(sptr);
	while (*sptr=='|') {
		sptr++;
		value |= doand();
		NONBLANK(sptr);
	}
	return(value);
}


doand()
{
	int value;
	value = exp();
	NONBLANK(sptr);
	while (*sptr=='&') {
		sptr++;
		value &= exp();
		NONBLANK(sptr);
	}
	return(value);
}


/*
 * After exp checks for parentheses, it picks up a value by calling getid,
 * picks up an operator and calls getid to pick up the second value.
 * Then based on the operator it calls either numcomp or equal to see
 * if the exp is true or false and returns the correct value.
 */

exp()
{
	register char op, save;
	register int value;
	char *id1, *id2, next;

	NONBLANK(sptr);
	if(*sptr == '(') {
		sptr++;
		value = door();
		NONBLANK(sptr);
		if (*sptr == ')') {
			sptr++;
			return(value);
		}
		else {
			sprintf(Error,nl_msg(14,"parenthesis error on line %d"),
				Lineno);
			strcat(Error," (vc11)");
		}
	}

	id1 = getid(sptr);
	if (op = *sptr)
		*sptr++ = '\0';
	if (op == NEQ && (next = *sptr++) == '\0')
		--sptr;
	id2 = getid(sptr);
	save = *sptr;
	*sptr = '\0';

	if(op ==LT || op == GT) {
		value = numcomp(id1, id2);
		if ((op == GT && value == 1) || (op == LT && value == -1))
			value = TRUE;
		else value = FALSE;
	}

	else if (op==EQ || (op==NEQ && next==EQ)) {
		value = equal(id1, id2);
		if ( op == NEQ)
			value = !value;
	}

	else {
		sprintf(Error,nl_msg(15,"invalid operator on line %d"), Lineno);
		fatal(strcat(Error," (vc12)"));
	}
	*sptr = save;
	return(value);
}


/*
 * Getid picks up a value off a line and returns a pointer to the value.
 */

char *
getid(gptr)
register char *gptr;
{
	register char *id;

	NONBLANK(gptr);
	id = gptr;
	gptr = findstr(gptr,DELIM);
	if (*gptr)
		*gptr++ = '\0';
	NONBLANK(gptr);
	sptr = gptr;
	return(id);
}


/*
 * Numcomp accepts two pointers to strings of digits and calls numck
 * to see if the strings contain only digits.  It returns -1 if
 * the first is less than the second, 1 if the first is greater than the
 * second and 0 if the two are equal.
 */

numcomp(id1, id2)
register char *id1, *id2;
{
	int k1, k2;

	numck(id1);
	numck(id2);
	while (*id1 == '0')
		id1++;
	while (*id2 == '0')
		id2++;
	if ((k1 = size(id1)) > (k2 = size(id2)))
		return(1);
	else if (k1 < k2)
		return(-1);
	else while(*id1 != '\0') {
		if(*id1 > *id2)
			return(1);
		else if(*id1 < *id2)
			return(-1);
		id1++;
		id2++;
	}
	return(0);
}


/*
 * Numck accepts a pointer to a string and checks to see if they are
 * all digits.  If they're not it calls fatal, otherwise it returns.
 */

numck(nptr)
register char *nptr;
{
	for (; *nptr != '\0'; nptr++)
		if (!numeric(*nptr)) {
			sprintf(Error,nl_msg(16,"non-numerical value on line %d"),
				Lineno);
			fatal(strcat(Error," (vc14)"));
		}
	return;
}


/*
 * Replace accepts a pointer to a line and scans the line for a keyword
 * enclosed in control characters.  If it doesn't find one it returns
 * a pointer to the begining of the line.  Otherwise, it calls
 * lookup to find the keyword.
 * It rewrites the line substituting the value for the
 * keyword enclosed in control characters.  It then continues scanning
 * the line until no control characters are found and returns a pointer to
 * the begining of the new line.
 */

# define INCR(int) if (++int==NSLOTS) { \
		sprintf(Error,nl_msg(17,"out of space [line %d]"),Lineno); \
		fatal(strcat(Error," (vc15)")); }

char *
replace(ptr)
char *ptr;
{
	char *slots[NSLOTS];
	int i,j,newlen;
	register char *s, *t, *p;

	for (s=ptr; *s++!='\n';);
	*(--s) = '\0';
	Linend = s;
	i = -1;
	for (p=ptr; *(s=findch(p,Ctlchar)); p=t) {
		*s++ = '\0';
#ifdef NLS16		/* skip full width of a 16 bit control char */
		if (Ctlchar > 0377)
			s++;
#endif NLS16
		INCR(i);
		slots[i] = p;
		if (*(t=findch(s,Ctlchar))==0) {
#ifdef NLS16		/* may have to output 2 bytes for Ctlchar */
			if (Ctlchar > 0377) {
				sprintmsg( Error,
					(nl_msg(23,"unmatched `%1$c%2$c' on line %3$d")),
				(Ctlchar>>8)&0377,Ctlchar&0377,Lineno);
			} else
#endif NLS16
			sprintmsg( Error,
				(nl_msg(18,"unmatched `%1$c' on line %2$d")),
				Ctlchar,Lineno);
			strcat(Error," (vc7)");
			fatal(Error);
		}
		*t++ = '\0';
#ifdef NLS16		/* skip full width of a 16 bit control char */
		if (Ctlchar > 0377)
			t++;
#endif NLS16
		INCR(i);
		slots[i] = Sym[j = lookup(s)].value;
		Sym[j].usage |= USD;
	}
	INCR(i);
	slots[i] = p;
	if (i==0) return(ptr);
	newlen = 0;
	for (j=0; j<=i; j++)
		newlen += (size(slots[j])-1);
	t = Repflag = fmalloc(++newlen);
	for (j=0; j<=i; j++)
		t = ecopy(slots[j],t);
	Linend = t;
	return(Repflag);
}


/*
 * Lookup accepts a pointer to a keyword name and searches the symbol
 * table for the keyword.  It returns its index in the table if its there,
 * otherwise it puts the keyword in the table.
 */

lookup(lname)
char *lname;
{
	register int i;
	register char *t;
	register struct symtab *s;
#ifdef NLS16	/* need to store first char of lname */
	int first_char;
#endif NLS16

	t = lname;
#ifdef NLS16
	/*
	**  Advance by proper char width and ensure the 1st char is alpha
	**  and subsequent char's are alphanumeric.
	**         *****ASSUMES ALL KANJI ARE ALPHABETIC*****
	*/
	first_char = CHARAT(t);
	while ((i = CHARADV(t)) &&
		(i > 0377 ||
		isalpha(i) ||
		(i != first_char && isalnum(i))));
#else NLS16
	while ((i = *t++) &&
		((i>='A' && i<='Z') || (i>='a' && i<='z') ||
			(i!= *lname && i>='0' && i<='9')));
#endif NLS16
	if (i) {
		sprintf(Error,nl_msg(19,"invalid keyword name on line %d"),Lineno);
		fatal(strcat(Error," (vc9)"));
	}

	for(i =0; Sym[i].usage != 0 && i<SYMSIZE; i++)
		if (equal(lname, Sym[i].name)) return(i);
	s = &Sym[i];
	if (s->usage == 0) {
		copy(lname,s->name);
		copy("",(s->value = fmalloc(s->lenval = 1)));
		return(i);
	}
	sprintf(Error,"%s (vc6)",nl_msg(20,"out of space"));
	fatal(Error);
}


/*
 * Putin accepts a pointer to a keyword name, and a pointer to a value.
 * It puts this information in the symbol table by calling lookup.
 * It returns the index of the name in the table.
 */

putin(pname, pvalue)
char *pname;
char *pvalue;
{
	register int i;
	register struct symtab *s;

	s = &Sym[i = lookup(pname)];
	ffree(s->value);
	s->lenval = size(pvalue);
	copy(pvalue, (s->value = fmalloc(s->lenval)));
	s->usage |= ASG;
	return(i);
}


chksize(s)
char *s;
{
	if (size(s) > PARMSIZE) {
		sprintf(Error,nl_msg(21,"keyword name too long on line %d"),Lineno);
		fatal(strcat(Error," (vc8)"));
	}
}


char *
findch(astr,match)
#ifdef NLS16	/* make 'match' big enough to hold 16 bit char */
char *astr;
int match;
#else NLS16
char *astr, match;
#endif NLS16
{
#ifdef NLS16	/* make 'c' big enough to hold 16 bit char */
	register char *s, *t;
	register int c;
#else NLS16
	register char *s, *t, c;
#endif NLS16
	char *temp;

#ifdef NLS16
	/*
	**  Retrieve and advance by the proper width
	*/
	for (s=astr; (c = CHARAT(s)) && c!=match; ADVANCE(s))
#else NLS16
	for (s=astr; (c = *s) && c!=match; s++)
#endif NLS16
		if (c=='\\') {
			if (s[1]==0) {
				sprintf(Error,nl_msg(22,"syntax on line %d"),Lineno);
				fatal(strcat(Error," (vc19)"));
			}
			else {
				for (t = (temp=s) + 1; *s++ = *t++;);
				s = temp;
			}
		}
	return(s);
}


char *
ecopy(s1,s2)
char *s1, *s2;
{
	register char *r1, *r2;

	r1 = s1;
	r2 = s2;
	while (*r2++ = *r1++);
	return(--r2);
}


char *
findstr(astr,pat)
char *astr, *pat;
{
#ifdef NLS16	/* make 'c' big enough to hold a 16 bit char */
	register char *s, *t;
	register int c;
#else NLS16
	register char *s, *t, c;
#endif NLS16
	char *temp;

#ifdef NLS16
	/*
	**  Retrieve and advance by the proper width and skip the any()
	**  call for kanji since they could never match the pattern string.
	*/
	for (s=astr; (c = CHARAT(s)) && (c > 0377 || any(c,pat)==0); ADVANCE(s))
#else NLS16
	for (s=astr; (c = *s) && any(c,pat)==0; s++)
#endif NLS16
		if (c=='\\') {
			if (s[1]==0) {
				sprintf(Error,nl_msg(22,"syntax on line %d"),Lineno);
				fatal(strcat(Error," (vc19)"));
			}
			else {
				for (t = (temp=s) + 1; *s++ = *t++;);
				s = temp;
			}
		}
	return(s);
}
