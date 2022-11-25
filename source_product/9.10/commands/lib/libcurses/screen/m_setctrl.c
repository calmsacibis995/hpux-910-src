/* @(#) $Revision: 64.2 $ */   
# include	"curses.ext"

extern	short	_oldx;
extern	short	_oldy;
extern	chtype	_oldctrl;
extern	int	_poschanged;

m_setctrl()
{
	char	*uctr;
	short	_prevx;
	short	_prevy;
	chtype	_cur_attrs;
	chtype	ch = _oldctrl;
	int	_prevpos;

	_oldctrl = 0;

	if ( _poschanged )
	{
		_prevx = stdscr->_curx;
		_prevy = stdscr->_cury;
		_prevpos = 1;
	}
	else
	{
		_prevpos = 0;
	}

	stdscr->_curx = _oldx;
	stdscr->_cury = _oldy;

	_cur_attrs = stdscr->_attrs;
	stdscr->_attrs = ch & A_ATTRIBUTES;

	uctr = unctrl(ch & A_CHARTEXT);
	m_addch((chtype)uctr[0] | (ch&A_ATTRIBUTES));
	m_addch((chtype)uctr[1] | (ch&A_ATTRIBUTES));

	stdscr->_attrs = _cur_attrs;

	if ( _prevpos )
	{
		stdscr->_curx = _prevx;
		stdscr->_cury = _prevy;
	}

	return OK;
}
