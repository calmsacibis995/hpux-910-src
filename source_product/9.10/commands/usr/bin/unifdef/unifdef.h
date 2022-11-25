#include <ctype.h>
#include <malloc.h>
#include <stdio.h>

#define BSS
FILE *input;
#define YES	1
#define NO	0
#define	MAYBE	2

/* types of input lines: */
#define PLAIN       0   /* ordinary line */
#define TRUE        1   /* a true  #ifdef of a symbol known to us */
#define FALSE       2   /* a false #ifdef of a symbol known to us */
#define OTHER       3   /* an #ifdef of a symbol not known to us */
#define PARTIAL     4   /* an #if of an expression only partially evaluated */
#define ELSE        5   /* #else */
#define ENDIF       6   /* #endif */
#define LEOF        7   /* end of file */

char *progname BSS;
char *filename BSS;
char text BSS;          /* -t option in effect: this is a text file */
char lnblank BSS;       /* -l option in effect: blank deleted lines */
char complement BSS;    /* -c option in effect: complement the operation */
#define MAXSYMS 1000
#define	MAX_IF_DEPTH	20	/* maximum nesting of #if's */
#define TOTAL_SYM_SLOTS	(MAXSYMS+MAX_IF_DEPTH)
char true[TOTAL_SYM_SLOTS] BSS;
char ignore[TOTAL_SYM_SLOTS] BSS;
char *sym[TOTAL_SYM_SLOTS] BSS;
char insym[TOTAL_SYM_SLOTS] BSS;
char nsyms BSS;
char incomment BSS;
char reject BSS;    /* 0 or 1: pass thru; 1 or 2: ignore comments */
#define QUOTE1 0
#define QUOTE2 1
char inquote[2] BSS;
#define	EXIT_NO_CHANGE	0
#define	EXIT_CHANGED	1
#define	EXIT_ERROR	2
char *skipcomment();
char *eatcomment();
char *skipquote();
int linenum BSS;    /* current line number */

/* definitions for parsing #if's */

typedef enum    { FIXED, SYMBOLIC }     expression_type;

struct expression   
{
    expression_type		expr_tag;   
    union
    {
	struct
	{
	    int			fixed_value;
	    int			fixed_radix;
	}			fixed;
 
	struct   
	{
	    int 		symbolic_operand_count;	/* 0 - 3 */
	    int 		symbolic_operator;	/* a lexical token */
	    struct expression	*symbolic_operand[3];
	    int			symbolic_parenthesized : 1;
	    int			symbolic_defined_parenthesized : 1;
	    char		*symbolic_string;
	}			symbolic;
    }				expr_union;
};

#define free(x) safe_free(x)

typedef struct expression expr;

int	current_radix;
int	if_substituted;
expr	*if_expr;
expr	*binop();
void	safe_free();
char	*safe_malloc();
char	*safe_realloc();
char	*more_input();
char	*newp;
char	*entire_expr_text BSS;

#define	newexpr()	(expr *)safe_malloc(sizeof(expr))
#define	FIXED_EXPR(n)	(n)->expr_union.fixed
#define	SYM_EXPR(n)	(n)->expr_union.symbolic
