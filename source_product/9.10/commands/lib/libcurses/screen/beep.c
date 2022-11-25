/* @(#) $Revision: 27.1 $ */    

#include "curses.ext"

extern	int	_outch();

beep()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "beep().\n");
#endif
    if (bell)
	tputs (bell, 0, _outch);
    else
	tputs (flash_screen, 0, _outch);
    __cflush();
}
