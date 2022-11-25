/* @(#) $Revision: 66.2 $ */     

#include "unifdef.h"
#include "y.tab.h"

#define	isid(a)	(isalpha(a) || (a) == '_')

yylex()
{
    extern char *eatcomment();
    extern int lookup();
    static char *op2[] =
    {
	"||", "&&", ">>", "<<", ">=", "<=", "!=", "=="
    };
    static int val2[] =
    {
	OROR, ANDAND, RS, LS, GE, LE, NE, EQ
    };
    static char *opc = "b\bt\tn\nf\fr\r\\\\";
    extern char *newp;
    register char *inp;
    register char savc, *s;
    int val;
    register char **p2;
    int id_type;

    for (;;)
    {
#ifdef TESTING
fprintf(stderr, "yylex - newp = 0x%x, *newp = %c\n", newp, *newp);
#endif
	inp = newp;

	/*
	 * Skip any white space and comments.
	 * Detect end of line (return).
	 */
	while (isspace(*inp))
	{
	    if (*inp == '\n')
		return stop;	/* end of #if */
	    newp = inp = eatcomment(inp + 1);
	}

	/*
	 * Check for two character operators
	 */
	for (p2 = op2 + 8; --p2 >= op2;)
	    if ((*p2)[0] == inp[0] && (*p2)[1] == inp[1])
	    {
		yylval.intval = val = val2[p2 - op2];
		newp += 2;
		goto ret;
	    }

	/*
	 * Check for 1-char operators
	 */
	s = "+-*/%<>&^|?:!~(),";
	while (*s)
	    if (*s++ == *inp)
	    {
		yylval.intval = val = *--s;
		newp++;
		goto ret;
	    }

	/*
	 * Check for numeric tokens
	 */
	if (*inp <= '9' && *inp >= '0')
	{
#ifdef TESTING
fprintf(stderr, "branch 1, number\n");
#endif
	    if (*inp == '0')
		yylval.intval = (inp[1] == 'x' || inp[1] == 'X') ?
		    tobinary(inp + 2, 16) : tobinary(inp + 1, 8);
	    else
		yylval.intval = tobinary(inp, 10);
	    val = number;
	    goto ret;
	}

	/*
	 * Check for a valid identifier ([a-ZA-Z] and '_')
	 * Also checks for keyword "defined".
	 */
	if (isid(*inp))
	{
#ifdef TESTING
fprintf(stderr, "branch 2, identifier\n");
#endif
	    /*
	     * find and mark the end of the token
	     */
	    while (isid(*newp) || isdigit(*newp))
		newp++;
	    savc = *newp;
	    *newp = '\0';

	    if (strcmp(inp, "defined") == 0)
		val = DEFINED;
	    else
	    {
		id_type = lookup(inp);
		if (id_type == MAYBE)
		{
		    yylval.cp = safe_malloc(strlen(inp) + 1);
		    strcpy(yylval.cp, inp);
		    val = identifier;
		}
		else
		{
		    yylval.intval = id_type;
		    val = number;
		    if_substituted = 1;
		}
	    }
	    *newp = savc;
	    goto ret;
	}

	/*
	 * Now check for a character constant ('c')
	 */
	if (*inp == '\'')
	{
#ifdef TESTING
fprintf (stderr, "branch 3, character constant\n");
#endif
	    val = number;
	    if (inp[1] == '\\')
	    {			/* escaped */
		char c;
		s = opc;
		while (*s)
		    if (*s++ != inp[2])
			++s;
		    else
		    {
			yylval.intval = *s;
			newp += 4;
			goto ret;
		    }
		if (inp[2] <= '9' && inp[2] >= '0')
		{
		    yylval.intval = c = tobinary(inp + 2, 8);
		    newp++;
		}
		else
		{
		    yylval.intval = inp[2];
		    newp += 4;
		}
	    }
	    else
	    {
		yylval.intval = inp[1];
		newp += 3;
	    }
	}
	else if (inp[0] == '\\' && inp[1] == '\n')
	{
	    if ((newp = more_input()) == NULL)
	    {
		pperror("Line continued but no more input");
		exit(EXIT_ERROR);
	    }
	    continue;
	}
	else
	{
	    pperror("Illegal character %c in preprocessor if", *inp);
#ifdef TESTING
fprintf(stderr, "char is 0x%x, isalpha() = %d, isid = %d\n",
    *inp, isalpha(*inp), isid(*inp));
#endif
	    newp++;
	    continue;
	}
    ret: 
#ifdef TESTING
fprintf (stderr, "yylex returning %d\n", val);
#endif
	return (val);
    }
}

tobinary(st, b)
char *st;
int b;
{
    int n, c, t;
    char *s;
    n = 0;
    for (s = st; isxdigit(c = *s); s++)
    {
	switch (c)
	{
	case '0': case '1': 
	case '2': case '3': 
	case '4': case '5': 
	case '6': case '7': 
	case '8': case '9': 
	    t = c - '0';
	    break;
	case 'a': case 'b': 
	case 'c': case 'd': 
	case 'e': case 'f': 
	    t = c - 'a' + 10;
	    if (b > 10)
		break;
	case 'A': case 'B': 
	case 'C': case 'D': 
	case 'E': case 'F': 
	    t = c - 'A' + 10;
	    if (b > 10)
		break;
	default: 
	    t = -1;
	    pperror("Illegal number %s", st);
	}
	if (t < 0)
	    break;
	n = n * b + t;
    }

    current_radix = b;
    newp = s;
    if (*s == 'l' || *s == 'L' || *s == 'u' || *s == 'U')
	newp++;
    return n;
}

pperror(string, a1, a2, a3, a4, a5)
char *string;
int a1, a2, a3, a4, a5;
{
    extern int linenum;		/* current line number */

    fprintf(stderr, "unifdef: line %d: ", linenum);
    fprintf(stderr, string, a1, a2, a3, a4, a5);
    fputc('\n', stderr);
}

yyerror(s, a1, a2)
char *s;
int a1, a2;
{
    fprintf(stderr, "yyerror: ");
    pperror(s, a1, a2);
}
