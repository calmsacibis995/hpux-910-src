/* @(#) $Revision: 64.1 $ */   
#include "curses.h"

extern	WINDOW	*_oldwin;
extern	chtype	_oldctrl;
extern	short	_oldx;
extern	short	_oldy;

_showctrl()
{
	if ( _oldctrl )
	{
		_setctrl();
	}

	_oldwin = 0;
	_oldx = -1;
	_oldy = -1;

	return OK;
}
