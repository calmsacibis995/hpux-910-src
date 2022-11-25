/* @(#) $Revision: 64.1 $ */    

#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;

saveterm()
{
	def_prog_mode();

	cur_term->Nfcntl &= 0x3fffffff;
	if ( SP && SP->phys_irm ) {
		cur_term->Nfcntl |= 0x40000000;
	}
	if ( SP && SP->kp_state ) {
		cur_term->Nfcntl |= 0x80000000;
	}
}
