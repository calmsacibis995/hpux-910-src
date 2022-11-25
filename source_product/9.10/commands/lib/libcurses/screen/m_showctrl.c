/* @(#) $Revision: 64.1 $ */   
#include "curses.h"

extern	chtype	_oldctrl;
extern	short	_oldx;
extern	short	_oldy;

m_showctrl()
{

	if ( _oldctrl ) {
		m_setctrl();
	}

	_oldx = -1;
	_oldy = -1;

	return OK;
}
