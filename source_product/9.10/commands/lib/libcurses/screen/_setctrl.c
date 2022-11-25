/* @(#) $Revision: 72.2 $ */   
# include	"curses.ext"

extern	short	_oldx;
extern	short	_oldy;
extern	WINDOW	*_oldwin;
extern	chtype	_oldctrl;
extern	int	_poschanged;

_setctrl()
{
	unsigned char	*uctr;
	short	_prevx;
	short	_prevy;
	chtype	_cur_attrs;
	chtype	ch = _oldctrl;
	int	_prevpos;

	_oldctrl = 0;

	if ( _poschanged )
	{
		_prevx = _oldwin->_curx;
		_prevy = _oldwin->_cury;
		_prevpos = 1;
	}
	else
	{
		_prevpos = 0;
	}

	_oldwin->_curx = _oldx;
	_oldwin->_cury = _oldy;

	_cur_attrs = _oldwin->_attrs;
	_oldwin->_attrs = ch & A_ATTRIBUTES;

	if ( 0240 <= ch && ch <= 0376 )  /* be sure there is a unctrl seq */
	{
	   uctr = unctrl(ch & A_CHARTEXT);
	   if ( uctr[0] != ch ) waddch(_oldwin,(chtype)uctr[0]);
	   if ( uctr[1]  != 0 ) waddch(_oldwin,(chtype)uctr[1]);
	}

	_oldwin->_attrs = _cur_attrs;

	if ( _prevpos )
	{
		_oldwin->_curx = _prevx;
		_oldwin->_cury = _prevy;
	}

	return OK;
}
