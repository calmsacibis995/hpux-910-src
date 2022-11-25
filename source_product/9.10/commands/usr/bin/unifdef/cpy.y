/*
# @(#) $Revision: 64.1 $       
*/

%{
#include "unifdef.h"
%}

%union {
	int	intval;
	expr	*ep;
	char	*cp;
};

%term <cp> identifier
%term <intval> number stop DEFINED
%term EQ NE LE GE LS RS
%term <intval> ANDAND OROR
%left <intval> ','
%right <intval> '='
%right <intval> '?' ':'
%left OROR
%left ANDAND
%left <intval> '|' '^'
%left <intval> '&'
%binary <intval> EQ NE
%binary <intval> '<' '>' LE GE
%left <intval> LS RS
%left <intval> '+' '-'
%left <intval> '*' '/' '%'
%right <intval> '!' '~' UMINUS
%left <intval> '(' '.'
%type <ep> e term S

%%
S:	e stop	={if_expr = $1; YYACCEPT; }


e:	  e '*' e
		={$$ = binop ($1, $2, $3);}
	| e '/' e
		={$$ = binop ($1, $2, $3);}
	| e '%' e
		={$$ = binop ($1, $2, $3);}
	| e '+' e
		={$$ = binop ($1, $2, $3);}
	| e '-' e
		={$$ = binop ($1, $2, $3);}
	| e LS e
		={$$ = binop ($1, $2, $3);}
	| e RS e
		={$$ = binop ($1, $2, $3);}
	| e '<' e
		={$$ = binop ($1, $2, $3);}
	| e '>' e
		={$$ = binop ($1, $2, $3);}
	| e LE e
		={$$ = binop ($1, $2, $3);}
	| e GE e
		={$$ = binop ($1, $2, $3);}
	| e EQ e
		={$$ = binop ($1, $2, $3);}
	| e NE e
		={$$ = binop ($1, $2, $3);}
	| e '&' e
		={$$ = binop ($1, $2, $3);}
	| e '^' e
		={$$ = binop ($1, $2, $3);}
	| e '|' e
		={$$ = binop ($1, $2, $3);}
	| e ANDAND e
		={$$ = binop ($1, $2, $3);}
	| e OROR e
		={$$ = binop ($1, $2, $3);}
	| e ',' e
		={$$ = binop ($1, $2, $3);}

	| e '?' e ':' e
		={
		if (($1->expr_tag == FIXED) &&
		    ($3->expr_tag == FIXED) &&
		    ($5->expr_tag == FIXED))
		{
			$$ = $1;
			FIXED_EXPR($$).fixed_value = 
				FIXED_EXPR($1).fixed_value
					? FIXED_EXPR($3).fixed_value
					: FIXED_EXPR($5).fixed_value;

			if (FIXED_EXPR($1).fixed_radix == 16 ||
			    FIXED_EXPR($3).fixed_radix == 16 ||
			    FIXED_EXPR($5).fixed_radix == 16)
				FIXED_EXPR($1).fixed_radix = 16;
			else if (FIXED_EXPR($1).fixed_radix == 8 ||
			    FIXED_EXPR($3).fixed_radix == 8 ||
			    FIXED_EXPR($5).fixed_radix == 8)
				FIXED_EXPR($1).fixed_radix = 8;
			else
				FIXED_EXPR($1).fixed_radix = 10;

			free ((char *)$3);
			free ((char *)$5);
		} else {
			$$ = newexpr ();
			$$->expr_tag = SYMBOLIC;
			SYM_EXPR($$).symbolic_operand_count = 3;
			SYM_EXPR($$).symbolic_operator = '?';
			SYM_EXPR($$).symbolic_operand[0] = $1;
			SYM_EXPR($$).symbolic_operand[1] = $3;
			SYM_EXPR($$).symbolic_operand[2] = $5;
			SYM_EXPR($$).symbolic_parenthesized = 0;
		}
		}

	| term
		={$$ = $1;}
term:
	  '-' term %prec UMINUS
		={
			if ($2->expr_tag == FIXED)
			{
				$$ = $2;
				FIXED_EXPR($$).fixed_value =
					- FIXED_EXPR($2).fixed_value;
			} else {
				$$ = newexpr ();
				$$->expr_tag = SYMBOLIC;
				SYM_EXPR($$).symbolic_operand_count = 1;
				SYM_EXPR($$).symbolic_operator = '-';
				SYM_EXPR($$).symbolic_operand[0] = $2;
			}
		}
			
	| '!' term
		={
			if ($2->expr_tag == FIXED)
			{
				$$ = $2;
				FIXED_EXPR($$).fixed_value =
					! FIXED_EXPR($2).fixed_value;
			} else {
				$$ = newexpr ();
				$$->expr_tag = SYMBOLIC;
				SYM_EXPR($$).symbolic_operand_count = 1;
				SYM_EXPR($$).symbolic_operator = '!';
				SYM_EXPR($$).symbolic_operand[0] = $2;
			}
		}
			

	| '~' term
		={
			if ($2->expr_tag == FIXED)
			{
				$$ = $2;
				FIXED_EXPR($$).fixed_value =
					~ FIXED_EXPR($2).fixed_value;
			} else {
				$$ = newexpr ();
				$$->expr_tag = SYMBOLIC;
				SYM_EXPR($$).symbolic_operand_count = 1;
				SYM_EXPR($$).symbolic_operator = '~';
				SYM_EXPR($$).symbolic_operand[0] = $2;
			}
		}
			
	| '(' e ')'
		={
			$$ = $2;
			if ($2->expr_tag == SYMBOLIC)
				SYM_EXPR($2).symbolic_parenthesized = 1;
		}

	| DEFINED '(' number ')'
		={
			$$ = newexpr ();
			$$->expr_tag = FIXED;
			FIXED_EXPR($$).fixed_value = $3;
			FIXED_EXPR($$).fixed_radix = current_radix;
		}

	| DEFINED number
		={
			$$ = newexpr ();
			$$->expr_tag = FIXED;
			FIXED_EXPR($$).fixed_value = $2;
			FIXED_EXPR($$).fixed_radix = current_radix;
		}

	| DEFINED '(' identifier ')'
		={
			$$ = newexpr ();
			$$->expr_tag = SYMBOLIC;
			SYM_EXPR($$).symbolic_operand_count = 0;
			SYM_EXPR($$).symbolic_operator = DEFINED;
			SYM_EXPR($$).symbolic_string = (char *)$3;
			SYM_EXPR($$).symbolic_defined_parenthesized = 1;
		}

	| DEFINED identifier
		={
			$$ = newexpr ();
			$$->expr_tag = SYMBOLIC;
			SYM_EXPR($$).symbolic_operand_count = 0;
			SYM_EXPR($$).symbolic_operator = DEFINED;
			SYM_EXPR($$).symbolic_string = (char *)$2;
			SYM_EXPR($$).symbolic_defined_parenthesized = 0;
		}

	| number
		={
			$$ = newexpr ();
			$$->expr_tag = FIXED;
			FIXED_EXPR($$).fixed_value = $1;
			FIXED_EXPR($$).fixed_radix = current_radix;
		}

	| identifier
		={
			$$ = newexpr ();
			$$->expr_tag = SYMBOLIC;
			SYM_EXPR($$).symbolic_operand_count = 0;
			SYM_EXPR($$).symbolic_string = (char *)$1;
			SYM_EXPR($$).symbolic_parenthesized = 0;
		}
%%
