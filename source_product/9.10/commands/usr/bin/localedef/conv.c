/* @(#) $Revision: 70.2 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

/* con_init: initialize shift (conv) tables.
** Get here when you see a 'ul', 'toupper' or 'tolower' keyword.
*/
void
con_init(token)
int token;				/* keyword token */
{
	extern void con_num();
	extern void con_finish();
	/*
	extern void con_l_angle();
	extern void con_r_angle();
	*/
        extern void con_l_paren();
	extern void con_r_paren();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	number = con_num;		/* set number function for main.c */
	finish = con_finish;		/* do a shift table finish */
	/*
	l_angle = con_l_angle;*/	/* tell main.c come here on a '<' */
/*	r_angle = con_r_angle;*/	/* tell main.c come here on a '>' */
	l_paren = con_l_paren;          /* tell main.c come here on a '{' */
	r_paren = con_r_paren;          /* -------------------------- '}' */
	left = right = AVAILABLE;	/* a pair of numbers is available */
	lab = rab = 0;			/* no angle brackets initially */
}

/* con_num: process numbers by putting them into 'left' and 'right'.
** Come here when you see a number after '<'.  Can't already have
** a full pair.
*/
void
con_num()
{
	/* check shift code range */
	if (num_value < ZERO || num_value > DFL_HICHAR)
		error(CHR_CODE);

	if (left == AVAILABLE)
		left = num_value;
	else if (right == AVAILABLE)
		right = num_value;
	else
		error(EXPR);
}

/* con_exec: process a shift table pair.
** The 'left' number must be the upper case of the 'right'.
** The 'right' number must be the lower case of the 'left'.
*/
void
con_exec()
{
	if (META) error(EXPR);

/*
	if (op == UL) {		
		(upper+s_offset)[right] = left;*/		/* to_upper */
/*
		(lower+s_offset)[left] = right;*/		/* to_lower */
/*	} */
	
	if (op == TOUPPER)			
	  {    /* to_upper */
		(upper+s_offset)[left] = right;
	      }
	if (op == TOLOWER)				/* to_lower */
	  {
		(lower+s_offset)[left] = right;
	/*	(upper+s_offset)[left] = right; */
	      }

	left = right = AVAILABLE;
}

/* con_l_paren: process a left angle bracket.
** Come here when you see a '{'.  Nothing to do.
*/
void
con_l_paren()
{
/*if (!(found_tolower))
                (lower+s_offset)[left] = right; */
}

/* con_r_paren: process a right angle bracket.
** Come here when you see a '}'.  This ends a shift table pair.
** We must have seen only 1 left & right angle bracket and must have
** one pair of numbers.  After this syntax checking, execute the
** shift table routine.
*/
void
con_r_paren()
{
	extern void con_exec();
	if ( lp!=1 || rp!=1 )
		error(EXPR);
	else if (left == AVAILABLE || right == AVAILABLE)
		error(EXPR);
	else {
		lp = rp = 0;
		con_exec();
	}
}

/* con_finish: finish off shift table processing.
** Just be sure there are no left-over metachars and numbers.
** Also store the shift table header information.
*/
void
con_finish()
{
	if (META) error(EXPR);

	if (left != AVAILABLE  ||  right != AVAILABLE)
		error(EXPR);
}


