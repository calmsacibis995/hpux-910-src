/* @(#) $Revision: 27.1 $ */   

#include "curses.ext"

extern	int	_outch();

flash()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "flash().\n");
#endif
    if (flash_screen)
	tputs (flash_screen, 0, _outch);
    else
	tputs (bell, 0, _outch);
    __cflush();
}
