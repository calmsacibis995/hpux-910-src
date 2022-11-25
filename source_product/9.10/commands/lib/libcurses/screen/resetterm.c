/* @(#) $Revision: 64.2 $ */   

#include "curses.ext"
#include "../local/uparm.h"

extern	struct term *cur_term;
extern	int	_outch();

resetterm()
{
	if ( isatty(1) )
	{
		if ( exit_insert_mode )
			tputs(exit_insert_mode, 1, _outch);
		if ( keypad_local )
			tputs(keypad_local, 1, _outch);
		fflush(stdout);
	}

	reset_shell_mode();
}
