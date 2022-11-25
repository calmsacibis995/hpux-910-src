#include "unifdef.h"
#include "y.tab.h"

static void print_expr();
static void deallocate_expr();

/*
 * parse_if() -- parse a "#if" statement
 */
parse_if(if_text_ptr, expr_text_ptr)
char *if_text_ptr;
char *expr_text_ptr;
{
    int return_val;
    static char *savebuf = NULL;
    static int savebuf_siz = 0;

    /*
     * set up globals used by yylex()
     */
    newp = expr_text_ptr;
    if_substituted = 0;

    /*
     * Save the stuff between if_text_ptr and expr_text_ptr.
     * This is in case we get continuation lines that would clobber
     * the stuff currently at if_text_ptr.
     */
    {
	int len = (expr_text_ptr - if_text_ptr);

	if (len > savebuf_siz)
	{
	    savebuf_siz += (len + 20); /* leave extra room */
	    if (savebuf == NULL)
		savebuf = safe_malloc(savebuf_siz);
	    else
		savebuf = safe_realloc(savebuf, savebuf_siz);
	}
	strncpy(savebuf, if_text_ptr, len);
	savebuf[len] = '\0';
    }

#ifdef TESTING
fprintf (stderr, "about to parse \"%s\"\n", expr_text_ptr);
#endif
    if (yyparse())
	return OTHER;		/* syntax error in #if */

#ifdef TESTING
fprintf (stderr, "yyparse returned 0x%x\n", if_expr);
#endif
    if (if_expr == NULL)
    {
	fprintf(stderr, "error in parsing #if\n");
	exit(EXIT_ERROR);
    }

    if (!if_substituted)
	return_val = OTHER;
    else if (if_expr->expr_tag == FIXED)
	return_val = if_expr->expr_union.fixed.fixed_value
	    ? TRUE : FALSE;
    else
    {
	if ((reject < 2) ^ complement)
	{
	    /* print the #if as it appears in the input */
	    fputs(savebuf, stdout);

	    /* print the expression as it's been simplified */
	    print_expr(if_expr);
	    putchar('\n');
	}
	else if (lnblank)
	{
	    extern int nsaved;
	    int i;

	    for (i = nsaved; i >= 0; i++)
		putchar('\n');
	}

	return_val = PARTIAL;
    }

    deallocate_expr(if_expr);
#ifdef TESTING
fprintf (stderr, "parse returning %d\n", return_val);
#endif
    return return_val;
}

static void
print_expr(ep)
expr *ep;
{
    char *format;

    if (ep == NULL)
    {
	fprintf(stderr, "Unexpected print of null expression in #if\n");
	return;
    }

    if (ep->expr_tag == FIXED)
    {
	switch (ep->expr_union.fixed.fixed_radix)
	{
	case 16: 
	    format = "0x%x";
	    break;

	case 8: 
	    format = "0%o";
	    break;

	default: 
	    format = "%d";
	    break;
	}

	printf(format, ep->expr_union.fixed.fixed_value);
    }
    else			/* SYMBOLIC */
    {
	if (ep->expr_union.symbolic.symbolic_parenthesized)
	    putchar('(');

	switch (ep->expr_union.symbolic.symbolic_operand_count)
	{
	case 0: 
	    if (ep->expr_union.symbolic.symbolic_operator == DEFINED)
	    {
		printf("defined");
		if (ep->expr_union.symbolic
			.symbolic_defined_parenthesized)
		    putchar('(');
		else
		    putchar(' ');
	    }
	    fputs(ep->expr_union.symbolic.symbolic_string, stdout);
	    if (ep->expr_union.symbolic.symbolic_operator == DEFINED &&
		ep->expr_union.symbolic.symbolic_defined_parenthesized)
		putchar(')');
	    break;

	case 1: 
	    printf("%c ", ep->expr_union.symbolic.symbolic_operator);
	    print_expr(ep->expr_union.symbolic.symbolic_operand[0]);
	    break;

	case 2: 
	    print_expr(ep->expr_union.symbolic.symbolic_operand[0]);
	    switch (ep->expr_union.symbolic.symbolic_operator)
	    {
	    case LS: 
		printf(" << ");
		break;

	    case RS: 
		printf(" >> ");
		break;

	    case LE: 
		printf(" <= ");
		break;

	    case GE: 
		printf(" >= ");
		break;

	    case EQ: 
		printf(" == ");
		break;

	    case NE: 
		printf(" != ");
		break;

	    case ANDAND: 
		printf(" && ");
		break;

	    case OROR: 
		printf(" || ");
		break;

	    default: 
		printf(" %c ",
		    ep->expr_union.symbolic.symbolic_operator);
		break;
	    }
	    print_expr(ep->expr_union.symbolic.symbolic_operand[1]);
	    break;

	case 3: 
	    print_expr(ep->expr_union.symbolic.symbolic_operand[0]);
	    printf(" ? ");
	    print_expr(ep->expr_union.symbolic.symbolic_operand[1]);
	    printf(" : ");
	    print_expr(ep->expr_union.symbolic.symbolic_operand[3]);
	    break;

	default: 
	    fprintf(stderr, "%d operands in expression!\n",
		    ep->expr_union.symbolic.symbolic_operand_count);
	    exit(EXIT_ERROR);
	}
	if (ep->expr_union.symbolic.symbolic_parenthesized)
	    putchar(')');
    }
}

static void
deallocate_expr(ep)
expr *ep;
{
    register int i;

    if (ep == NULL)
    {
	fprintf(stderr,
	    "Unexpected deallocation of null expression in #if\n");
	return;
    }

    if (ep->expr_tag == SYMBOLIC)
    {
	if (ep->expr_union.symbolic.symbolic_operand_count == 0)
	    free(ep->expr_union.symbolic.symbolic_string);

	for (i = 0;
		i < ep->expr_union.symbolic.symbolic_operand_count;
		i++)
	    deallocate_expr(ep->expr_union.symbolic.symbolic_operand[i]);
    }

    free(ep);
}
					

expr *
binop(expr1, op, expr2)
expr *expr1, *expr2;
{
    expr *result;

    if (expr1->expr_tag == FIXED && expr2->expr_tag == FIXED)
    {
	result = expr1;

	if (expr1->expr_union.fixed.fixed_radix == 16 ||
		expr2->expr_union.fixed.fixed_radix == 16)
	    expr1->expr_union.fixed.fixed_radix = 16;
	else if (expr1->expr_union.fixed.fixed_radix == 8 ||
		expr2->expr_union.fixed.fixed_radix == 8)
	    expr1->expr_union.fixed.fixed_radix = 8;
	else
	    expr1->expr_union.fixed.fixed_radix = 10;

	switch (op)
	{
	case '*': 
	    expr1->expr_union.fixed.fixed_value *=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '/': 
	    expr1->expr_union.fixed.fixed_value /=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '%': 
	    expr1->expr_union.fixed.fixed_value %=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '+': 
	    expr1->expr_union.fixed.fixed_value +=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '-': 
	    expr1->expr_union.fixed.fixed_value -=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case LS: 
	    expr1->expr_union.fixed.fixed_value <<=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case RS: 
	    expr1->expr_union.fixed.fixed_value >>=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case LE: 
	    expr1->expr_union.fixed.fixed_value =
		expr1->expr_union.fixed.fixed_value <=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case GE: 
	    expr1->expr_union.fixed.fixed_value =
		expr1->expr_union.fixed.fixed_value >=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case EQ: 
	    expr1->expr_union.fixed.fixed_value =
		expr1->expr_union.fixed.fixed_value ==
		expr2->expr_union.fixed.fixed_value;
	    break;

	case NE: 
	    expr1->expr_union.fixed.fixed_value =
		expr1->expr_union.fixed.fixed_value !=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '&': 
	    expr1->expr_union.fixed.fixed_value &=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '^': 
	    expr1->expr_union.fixed.fixed_value ^=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case '|': 
	    expr1->expr_union.fixed.fixed_value |=
		expr2->expr_union.fixed.fixed_value;
	    break;

	case ANDAND: 
	    expr1->expr_union.fixed.fixed_value =
		expr1->expr_union.fixed.fixed_value &&
		expr2->expr_union.fixed.fixed_value;
	    break;

	case OROR: 
	    expr1->expr_union.fixed.fixed_value =
		expr1->expr_union.fixed.fixed_value ||
		expr2->expr_union.fixed.fixed_value;
	    break;

	case ',': 
	    *expr1 = *expr2;
	    break;
	}
	free((char *)expr2);
    }
    else if ((op == ANDAND || op == OROR) &&
	    (expr1->expr_tag == FIXED || expr2->expr_tag == FIXED))
    {
	/* we can simplify this expression */

	/* set up expr1 pointing to the fixed, expr2 to the symbolic */
	if (expr2->expr_tag == FIXED)
	{
	    result = expr2;
	    expr2 = expr1;
	    expr1 = result;
	}

	if ((op == ANDAND && expr1->expr_union.fixed.fixed_value) ||
		(op == OROR && !expr1->expr_union.fixed.fixed_value))
	{
	    result = expr2;
	    free(expr1);
	}
	else
	{
	    result = expr1;	/* set to FIXED */
	    deallocate_expr(expr2);
	}
    }
    else
    {
	result = newexpr();
	result->expr_tag = SYMBOLIC;
	result->expr_union.symbolic.symbolic_operand_count = 2;
	result->expr_union.symbolic.symbolic_operator = op;
	result->expr_union.symbolic.symbolic_operand[0] = expr1;
	result->expr_union.symbolic.symbolic_operand[1] = expr2;
	result->expr_union.symbolic.symbolic_parenthesized = 0;
    }
    return result;
}

char *
safe_malloc(bytes)
int bytes;
{
    char *ptr;

    if ((ptr = malloc(bytes)) == (char *)0)
    {
	fputs("out of memory\n", stderr);
	exit(EXIT_ERROR);
    }
#ifdef TESTING
fprintf (stderr, "malloc of %d bytes at 0x%x\n", bytes, ptr);
#endif
    return ptr;
}

char *
safe_realloc(ptr, bytes)
char *ptr;
int bytes;
{
    if ((ptr = realloc(ptr, bytes)) == (char *)0)
    {
	fputs("out of memory\n", stderr);
	exit(EXIT_ERROR);
    }
#ifdef TESTING
fprintf (stderr, "malloc of %d bytes at 0x%x\n", bytes, ptr);
#endif
    return ptr;
}

#undef free
void
safe_free(ptr)
void *ptr;
{
#ifdef TESTING
fprintf (stderr, "freeing 0x%x\n", ptr);
#endif
    if (ptr == NULL)
	abort();
    free(ptr);
}
