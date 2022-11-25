/* $Revision: 70.3 $ */
%{
/*
 * (c) Copyright 1989,1990,1991,1992 Hewlett-Packard Company, all rights reserved.
 *
 * #   #  ##  ##### ####
 * ##  # #  #   #   #
 * # # # #  #   #   ####
 * #  ## #  #   #   #
 * #   #  ##    #   ####
 *
 * Please read the README file with this source code. It contains important
 * information on conventions which must be followed to make sure that this
 * code keeps working on all the platforms and in all the environments in
 * which it is used.
 */

#include "support.h"
extern long if_value;
%}

/* Tokens with lexical values. */
%token IDENTIFIER
%token CONSTANT

%token DEFINED LPAREN RPAREN

/* Relational operators. */
%token LESS_THAN GREATER_THAN LESS_THAN_OR_EQUAL GREATER_THAN_OR_EQUAL
%token DOUBLE_EQUALS NOT_EQUALS

/* Functional operators. */
%token PLUS MINUS TILDE BANG SLASH PERCENT LSHIFT RSHIFT
%token CARAT BAR AND OR QUESTION COLON AMPERSAND STAR

%start constant_expression
%%

constant_expression
	: conditional_expression
	{
		if_value = $1.value;
	}
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression QUESTION conditional_expression COLON
	conditional_expression
	{
		$$.is_unsigned = $3.is_unsigned || $5.is_unsigned;
		$$.value = $1.value ? $3.value : $5.value;
	}
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR logical_and_expression
	{
		$$.is_unsigned = FALSE;
		$$.value = $1.value || $3.value;
	}
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression AND inclusive_or_expression
	{
		$$.is_unsigned = FALSE;
		$$.value = $1.value && $3.value;
	}
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression BAR exclusive_or_expression
	{
		$$.is_unsigned = $1.is_unsigned || $3.is_unsigned;
		$$.value = $1.value | $3.value;
	}
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression CARAT and_expression
	{
		$$.is_unsigned = $1.is_unsigned || $3.is_unsigned;
		$$.value = $1.value ^ $3.value;
	}
	;

and_expression
	: equality_expression
	| and_expression AMPERSAND equality_expression
	{
		$$.is_unsigned = $1.is_unsigned || $3.is_unsigned;
		$$.value = $1.value & $3.value;
	}
	;

equality_expression
	: relational_expression
	| equality_expression DOUBLE_EQUALS relational_expression
	{
		$$.is_unsigned = FALSE;
		$$.value = $1.value == $3.value;
	}
	| equality_expression NOT_EQUALS relational_expression
	{
		$$.is_unsigned = FALSE;
		$$.value = $1.value != $3.value;
	}
	;

relational_expression
	: shift_expression
	| relational_expression LESS_THAN shift_expression
	{
		$$.is_unsigned = FALSE;
		if($1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value < (unsigned long)$3.value;
		else
			$$.value = $1.value < $3.value;
	}
	| relational_expression GREATER_THAN shift_expression
	{
		$$.is_unsigned = FALSE;
		if($1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value > (unsigned long)$3.value;
		else
			$$.value = $1.value > $3.value;
	}
	| relational_expression LESS_THAN_OR_EQUAL shift_expression
	{
		$$.is_unsigned = FALSE;
		if($1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value <= (unsigned long)$3.value;
		else
			$$.value = $1.value <= $3.value;
	}
	| relational_expression GREATER_THAN_OR_EQUAL shift_expression
	{
		$$.is_unsigned = FALSE;
		if($1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value >= (unsigned long)$3.value;
		else
			$$.value = $1.value >= $3.value;
	}
	;

shift_expression
	: additive_expression
	| shift_expression LSHIFT additive_expression
	{
		$$.is_unsigned = $1.is_unsigned;
		$$.value = $1.value << $3.value;
	}
	| shift_expression RSHIFT additive_expression
	{
		if($$.is_unsigned = $1.is_unsigned)
			$$.value = (unsigned long)$1.value >> $3.value;
		else
			$$.value = $1.value >> $3.value;
	}
	;

additive_expression
	: multiplicative_expression
	| additive_expression PLUS multiplicative_expression
	{
		$$.is_unsigned = $1.is_unsigned || $3.is_unsigned;
		$$.value = $1.value + $3.value;
	}
	| additive_expression MINUS multiplicative_expression
	{
		$$.is_unsigned = $1.is_unsigned || $3.is_unsigned;
		$$.value = $1.value - $3.value;
	}
	;

multiplicative_expression
	: unary_expression
	| multiplicative_expression STAR unary_expression
	{
		if($$.is_unsigned = $1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value * (unsigned long)$3.value;
		else
			$$.value = $1.value * $3.value;
	}
	| multiplicative_expression SLASH unary_expression
	{
		if($$.is_unsigned = $1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value / (unsigned long)$3.value;
		else
			$$.value = $1.value / $3.value;
	}
	| multiplicative_expression PERCENT unary_expression
	{
		if($$.is_unsigned = $1.is_unsigned || $3.is_unsigned)
			$$.value = (unsigned long)$1.value % (unsigned long)$3.value;
		else
			$$.value = $1.value % $3.value;
	}
	;

unary_expression
	: postfix_expression
	| PLUS unary_expression
	{
		$$ = $2;
	}
	| MINUS unary_expression
	{
		$$.is_unsigned = $2.is_unsigned;
		$$.value = -$2.value;
	}
	| TILDE unary_expression
	{
		$$.is_unsigned = $2.is_unsigned;
		$$.value = ~$2.value;
	}
	| BANG unary_expression
	{
		$$.is_unsigned = $2.is_unsigned;
		$$.value = !$2.value;
	}
	;

postfix_expression
	: CONSTANT
	{
		$$ = $1;
	}
	| LPAREN conditional_expression RPAREN
	{
		$$ = $2;
	}
	;
