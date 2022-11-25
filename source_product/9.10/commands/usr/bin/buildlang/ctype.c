/* @(#) $Revision: 64.1 $ */     
/* LINTLIBRARY */

#include <stdio.h>
#include <ctype.h>
#include "global.h"

unsigned char mask;			/* ctype table mask */
unsigned char *table;			/* pointer to 1 of 4 ctype tables */

/* cty_init: initialize ctype tables.
** Get here when you see a ctype table keyword.
** (issupper, islower, isdigit, ispunct, iscntrl, isblank, isxdigit,
**  isfirst, issecond).
*/
void
cty_init(token,cty_mask,cty_table) 
int token;				/* keyword token */
unsigned char cty_mask;			/* ctype table mask */
unsigned char *cty_table;		/* ctype table pointer (1 of 4) */
{
	extern void rang_num();
	extern void cty_exec();
	extern void cty_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	mask = cty_mask;		/* get the mask */
	table = cty_table;		/* the table we're dealing with */
	number = rang_num;		/* set number function for main.c */
	rang_exec = cty_exec;		/* doing ctype action in range */
	finish = cty_finish;		/* set up ctype finish */

	left = right = AVAILABLE;	/* range pair is available */
	dash = FALSE;			/* meta flags to false */
}

/* cty_exec: execute ctype range.
** Get here at the end of a range.
*/
void
cty_exec()
{
	register int i;				/* loop counter */

	if (META) error(EXPR);			/* can't have any meta-chars */

	for (i=left ; i<=right ; i++) {		/* loop thru the range */
		*(table+i) |= mask;		/* set ctype table entry */
		if (op == ISFIRST && i < 0200)  /* enforce HP15 rules */
			error(FST_CODE);
		if (op == ISSECOND && ctype1[i+c_offset]&_C)
			error(SND_CODE);
	}

	left = right = AVAILABLE;		/* range pair is available */
}

/* cty_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Can't be any meta-chars and finish any implicit ranges.
*/
void
cty_finish()
{
	if (META) error(EXPR);

	if (left != AVAILABLE  &&  right == AVAILABLE)
		rang_num();
}

/* rang_num: execute a routine based on a range of numbers.
** The routine to excute is in the gloabal pointer to void
** function -- rang_exec -- which is set in the initialization
** routines.  Note that we shouldn't already have a full range
** of numbers here.  Also after an implicit range is processed,
** the numeric value from yylex() is the new left to the next range.
*/
void
rang_num()
{
	extern int range();
	register int i;

	if ((i = range()) == EXPLICIT_RANGE) {
		dash = FALSE;
		if (rang_exec == NULL) error(STATE);
		(*rang_exec)();
	} else if (i == IMPLICIT_RANGE) {
		dash = FALSE;
		if (rang_exec == NULL) error(STATE);
		(*rang_exec)();
		left = num_value;
	} else if (i == FULL_RANGE) error(EXPR);
}

/* range: get a range of numbers in the form num-num.
** Puts numbers in the globals left and right.
** Explicit range: numbers of the form num-num.
** Half range: an isolated number.
** Implicit range: isolated number put into the form num-num.
** Full range: already have an explict range.
*/
int
range()
{
	if (op >= ISUPPER && op <= ISSECOND) {	/* check ctype code range */
		if (num_value < lcctype_head.sh_low ||
		    num_value > lcctype_head.sh_high)
			error(CHR_CODE);
	}

	if (left == AVAILABLE) {
		if (dash) error(EXPR);
		left = num_value;
		return HALF_RANGE;
	} else if (right == AVAILABLE ) {
		if (dash) { 
			right = num_value;
			if (right < left) error(RANGE);
			return EXPLICIT_RANGE;
		} else {
			right = left;
			return IMPLICIT_RANGE;
		}
	}
	return FULL_RANGE;
}
