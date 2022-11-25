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

char *_c_why_not = NULL;
static char *
_stupid = "Sorry, I don't know how to deal with your '%s' terminal.\r\n";
static char *
_unknown = "Sorry, I need to know a more specific terminal type than '%s'.\r\n";

struct screen *
_new_tty(type, fd)
char	*type;
FILE	*fd;
{
	int retcode;
	struct map *_init_keypad();
	char *calloc();

#ifdef DEBUG
	if(outf) fprintf(outf, "__new_tty: type %s, fd %x\n", type, fd);
#endif
	/*
	 * Allocate an SP structure if there is none, or if SP is
	 * still pointing to an old structure from a previous call
	 * to this routine.  But don't allocate one if we are being
	 * called from a higher level curses routine.  Since it's our
	 * job to initialize the phys_scr field and higher level routines
	 * aren't supposed to, we check that field to figure which to do.
	 */
	if (SP == NULL || SP->cur_body!=0)
		SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == (struct screen *)0)
	    return (struct screen *)0;
	SP->term_file = fd;
	if (type == 0)
		type = "unknown";
	_setbuffered(fd);
	setupterm(type, fileno(fd), &retcode);
	if (retcode < 0) {
		/*
		 * This happens if /usr/lib/terminfo doesn't exist, there is
		 * no such terminal type, or the file is corrupted.
		 * This would be a good place to print an error message.
		 */
		return NULL;
	}
	savetty();	/* as a "useful default" - hanging up is nasty. */
	if (chk_trm() == ERR)
		return NULL;
	SP->tcap = cur_term;
	SP->doclear = 1;
	SP->cur_body = (struct line **) calloc(lines+2, sizeof (struct line *));
	if (SP->cur_body == (struct line **)0)
	    return (struct screen *)0;
	SP->std_body = (struct line **) calloc(lines+2, sizeof (struct line *));
	if (SP->std_body == (struct line **)0)
	    return (struct screen *)0;
#ifdef SIGWINCH
	SP->alloc_ln = lines;	/* lines is recorded though lines+2 is actually allocated */
#endif
#ifdef KEYPAD
	SP->kp = _init_keypad();
#endif
	SP->input_queue = (short *) calloc(20, sizeof (short));
	if (SP->input_queue == (short *)0)
	    return (struct screen *)0;
	SP->input_queue[0] = -1;

	SP->virt_x = 0;		/* X and Y coordinates of the SP->curptr */
	SP->virt_y = 0;		/* between updates. */
	_init_costs();
	return SP;
}

static int
chk_trm()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "chk_trm().\n");
#endif

	if (generic_type) {
		_c_why_not = _unknown;
		return ERR;
	}
	if (clear_screen == 0 || hard_copy || over_strike) {
		_c_why_not = _stupid;
		return ERR;
	}
	if (cursor_address) {
		/* we can handle it */
	} else 
	if ( (cursor_up || cursor_home)	/* some way to move up */
	    && cursor_down		/* some way to move down */
	    && (cursor_left || carriage_return)	/* ... move left */
					/* printing chars moves right */
		) {
		/* we can handle it */
	} else {
		_c_why_not = _stupid;
		return ERR;
	}
	return OK;
}
