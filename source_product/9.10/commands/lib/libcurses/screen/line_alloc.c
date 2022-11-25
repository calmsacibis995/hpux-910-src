/* @(#) $Revision: 72.1 $ */      

#include "curses.ext"
#include <signal.h>

/*----------------------------------------------------------------------*/
/*									*/
/* The following ifdef is a method of determining whether or not we are */
/* on a 9.0 system or 8.xx system.					*/
/* The specific problem is that if we are using a 8.07 os then SIGWINCH */
/* is defined in signal.h however, curses does not yet support this     */
/* feature; thus the header /usr/include/curses.h does not check and    */
/* create/not create the SIGWINCH entries into the window structure.    */
/* The method that we will use to determine if this is a 9.0 os, which  */
/* is a valid system for sigwinch is to check the SIGWINDOW flag.  If   */
/* This flag is set, then it's 9.0 otherwise, it is a os prior to 9.0   */
/*  									*/
/*  Assumptions:  							*/
/*     		OS		SIGWINCH	SIGWINDOW		*/
/*		-----------------------------------------		*/
/*		9.0 +		True		True			*/
/*		8.07		True		Not Set			*/
/*		Pre-8.07	Not Set		Not Set			*/
/*									*/
/* This should probably be removed after support for 8.xx is dropped	*/
/*									*/
/*	Note: SIGWINCH should be Defined for 9.0 and forward.  If it 	*/
/*	      is defined for Pre-9.0, then it should be undefined.	*/
/*									*/

#ifndef _SIGWINDOW
#  ifdef SIGWINCH
#    undef SIGWINCH
#  endif
#endif
/*									*/
/*		Done with special SIGWINCH definitions.			*/
/*----------------------------------------------------------------------*/

/*
 * _line_alloc returns a pointer to a new line structure.
 */
struct line *
_line_alloc ()
{
	register struct line   *rv = SP->freelist;
	char *calloc();
	register int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "mem: _line_alloc (), prev SP->freelist %x\n", SP->freelist);
#endif
	if (rv) {
#ifdef SIGWINCH
		if (columns > rv->alloc_ch) {
			/* realloc a bigger row if its too small */
			rv->body = (chtype *) realloc((void *)rv->body, (size_t)(columns * sizeof(chtype)));
			rv->alloc_ch = columns;
		}
#endif
		SP->freelist = rv -> next;
	} else {
#ifdef NONSTANDARD
		_ec_quit("No lines available in line_alloc", "");
#else
		rv = (struct line *) calloc (1, sizeof *rv);
		if (rv == (struct line *)0)
		    return rv;
		rv -> body = (chtype *) calloc (columns, sizeof (chtype));
#ifdef SIGWINCH
		rv->alloc_ch = columns;
#endif
#endif
	}
#ifdef SIGWINCH
                /* blank paded the line, just in case */
                for (i=0; i<rv->alloc_ch; i++)
                        rv->body[i] = ' ';
#endif
	rv -> length = 0;
	rv -> hash = 0;
	return rv;
}
