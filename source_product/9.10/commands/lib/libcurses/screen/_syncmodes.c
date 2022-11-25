/* @(#) $Revision: 27.1 $ */      

#include "curses.ext"

_syncmodes()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_syncmodes().\n");
#endif
	_sethl();
	_setmode();
	_setwind();
}
