/* @(#) $Revision: 66.2 $ */     

#include	"curses.h"
#include	<termio.h>
#include	<term.h>
#include	<fcntl.h>

extern	struct term *cur_term;

def_prog_mode()
{
#ifdef USG
	ioctl(cur_term -> Filedes, TCGETA, &(cur_term->Nttyb));
#else
	ioctl(cur_term -> Filedes, TIOCGETP, &(cur_term->Nttyb));
#endif
	/* don't forget to save fcntl(2) flags (pvs, 2/27/85) */
	cur_term->Nfcntl = fcntl(cur_term->Filedes, F_GETFL,0);
}
