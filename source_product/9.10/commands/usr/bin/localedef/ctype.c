/* @(#) $Revision: 70.3 $ */     
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
	/* dash changed to ellipsis for posix.2 */
	ellipsis = FALSE;		/* meta flags to false */
}

/* cty_exec: execute ctype range.
** Get here at the end of a range.
*/
void
cty_exec()
{
	register int i;				/* loop counter */

/*	if (META) error(EXPR);		*/	/* can't have any meta-chars */

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
/*	if (META) error(EXPR); */

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
		ellipsis = FALSE; /* dash changed to ellipsis for posix.2 */
		if (rang_exec == NULL) error(STATE);
		(*rang_exec)();
	} else if (i == IMPLICIT_RANGE) {
		ellipsis = FALSE;
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
	if (ellipsis) error(EXPR);/* dash changed to ellipsis for posix.2 */
		left = num_value;
		return HALF_RANGE;
	} else if (right == AVAILABLE ) {
		if (ellipsis) { 
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

/*
	The following function is no longer called from main, or any
	other function. Commenting out the code. Need to revisit to
	see if we really do not need it.  AL 6/27/92

 * cty_defs will add automatic memberships if not present and emit
 * warning + plus corrective action for prohibited memberships
 *Add function cty_defs() to make sure that:
            upper contains A-Z;
            lower contains a-z;
            alpha contains A-Za-z and none of cntrl, digit, punct or
             space;
            digit contains 0-9;
          space contains <space>, <form-feed>, <newline>, <carriage-return>,
             <tab>, and <vertical-tab> and none of alpha, digit, graph,
             or xdigit;
          cntrl contains none of alpha, digit, punct, graph, print, or
             xdigit
          punct contains none of alpha, digit, cntrl, xdigit, or <space>;
          graph contains alpha, digit, punct and not cntrl or <space>;
          print contains all those specified for graph plus <space>;
          xdigit contains 0-9A-Fa-f
          blank, if unspecified, contains <space> and <tab>;
          tolower, if not specified, contains the reverse mapping of
             toupper.
void
cty_defs()
{
extern       unsigned char   *__ctype;
extern       unsigned char   *__ctype2;
extern unsigned char *ctype1,*ctype5;
int i =0 ;

/* A to Z 
for (i = 65; i<= 90; i++)
  {
    *(ctype1+i) |= _U;
  }
/* a to z 
for (i = 97; i<= 122; i++)
  {
    *(ctype1+i) |= _L;
  }
/* from 0 to 9 
for (i = 48; i<= 57; i++)
  {
    *(ctype1+i) |= _N;
  }
for (i =0; i<=256; i++)
  {
    if( (*(ctype5+i) & (_A)) && ((*(ctype1+i)) & (_C|_N|_P|_S)) )
      warning(CTYPE_TAB);
   
    /* 32 == space ; 12 == form feed; newline == 10
     * cr ==13;  vtab = 11
     
    if( (i == 32) || (i == 12) || (i ==14) || (i == 10) || (i == 13) || (i == 11))
   {
     (*(ctype1+i)) |= _S;
   }
    
    if( ((*ctype1+i)&(_S)) && ( ((*(ctype1+i)) & (_U|_L|_N|_X)) ||
				((*(ctype1+i)) & (_G|_PR)) ) )
      warning(CTYPE_TAB);
    if( ((*(ctype1+i)) &(_C)) && ( ((*(ctype1+i)) &(_U|_L|_N|_P|_X|_B|_S))
				 || ((*(ctype1+i)) & (_G|_PR)) ) )
      warning(CTYPE_TAB);
    if( ((*(ctype1+i)) &(_P)) && ( ((*(ctype1+i)) &(_U|_L|_C|_X|_N))
				 || ( i == 32 ) ) )
      warning(CTYPE_TAB);
    if( ((*(ctype1+i)) &(_G)) && ( ((*(ctype1+i)) &(_C))
				 || (i == 32) ) )
      warning(CTYPE_TAB);
    if ((*(ctype5+i)) & (_PR))
      {
      *(ctype5+i) |= _PR;      
      }
    if ((*(ctype1+i)) & (_C))
      warning(CTYPE_TAB);
  }
/* 0 to 9 
for (i = 48; i<= 57 ; i++)
  (*(ctype1+i)) |= _X;
/* A to Z 
for (i = 65; i<= 90; i++);
   (*(ctype1+i)) |=_X;
/* a to z 
for (i = 97; i<= 122; i++);
   (*(ctype1+i)) |=_X;

if (!blank_found)
/* Space and tab 
  {
  (*(ctype1+32)) = _B;
  (*(ctype1+10)) = _B;
  }

}

*/














