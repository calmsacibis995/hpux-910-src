/* @(#) $Revision: 64.1 $ */     
#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;
extern	int	_outch();

fixterm()
{

	if ( cur_term->Nfcntl & 0x40000000 )
	{
		if ( enter_insert_mode )
			tputs(enter_insert_mode, 1, _outch);
	}
	else
	{
		if ( exit_insert_mode )
			tputs(exit_insert_mode, 1, _outch);
	}
	if ( cur_term->Nfcntl & 0x80000000 )
	{
		if ( keypad_xmit )
			tputs(keypad_xmit, 1, _outch);
	}
	else
	{
		if ( keypad_local )
			tputs(keypad_local, 1, _outch);
	}
	fflush(stdout);

 	cur_term->Nfcntl &= 0x3fffffff;

	reset_prog_mode();
}
