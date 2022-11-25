/* @(#) $Revision: 70.1 $ */      
# include	"curses.ext"
# include	<signal.h>
# include	<ctype.h>
# include	<nl_ctype.h>

extern	short	_oldx;
extern	short	_oldy;
extern	chtype	_oldctrl;
extern	WINDOW	*_oldwin;
extern	int	_poschanged;

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 */
m_addch(c)
register chtype		c;
{
	register int		x, y;
	char *uctr;
	register int rawc = c & A_CHARTEXT;
	register chtype	ch;

	if (_oldctrl)
	{
		if ( (!(_oldy==stdscr->_cury && _oldx==stdscr->_curx-1) && !(_oldx==stdscr->_maxx-1 && _oldy==stdscr->_cury-1)) ||
	 	     (!(FIRSTof2(_oldctrl & A_CHARTEXT) && SECof2(rawc))) )
		{
			m_setctrl();
		}
	}

	_poschanged = 0;

#ifdef DEBUG
	if (outf) fprintf(outf, "m_addch: [(%d,%d)] ", stdscr->_cury, stdscr->_curx);
#endif
	c = c & ~A_NLSATTR;
	x = stdscr->_curx;
	y = stdscr->_cury;
# ifdef DEBUG
	if (c == rawc)
		if(outf) fprintf(outf, "'%c'", rawc);
	else
		if(outf) fprintf(outf, "'%c' %o, raw %o", c, c, rawc);
# endif
	if (y >= stdscr->_maxy || x >= stdscr->_maxx || y < 0 || x < 0) {
		return ERR;
	}
	switch (rawc) {
	  case '\t':
	  {
		register int newx;

		for (newx = x + (8 - (x & 07)); x < newx; x++)
			if (m_addch(' ') == ERR)
				return ERR;
		return OK;
	  }
	  default:
/* 
#ifndef NONLS
		if (rawc < ' ' || rawc == 0177 ) {
#else
		if (rawc < ' ' || rawc > '~') {
#endif
*/
                if ( _CS_SBYTE && !(0240 <= rawc && rawc <= 0376) && !isprint(rawc) ) {
			uctr = unctrl(rawc);
			m_addch((chtype)uctr[0]|(c&A_ATTRIBUTES));
			m_addch((chtype)uctr[1]|(c&A_ATTRIBUTES));
			return OK;
		}

		if ( x==0 && y>=1 ) 
		{
			if ( _oldx==stdscr->_maxx-1 && _oldy==y-1 &&
			     !((ch=SP->std_body[y]->body[stdscr->_maxx-1]) & A_NLSATTR) &&
			     FIRSTof2(ch & A_CHARTEXT) && SECof2(rawc))
			{
				SP->std_body[y]->body[stdscr->_maxx-1] =
				SP->std_body[y]->body[stdscr->_maxx-1] & A_ATTRIBUTES | ' ';
				m_addch(ch & A_CHARTEXT);
				m_addch(c);
				return OK;
			}
		}

		if (stdscr->_attrs) {
#ifdef DEBUG
			if(outf) fprintf(outf, "(attrs %o, %o=>%o)", stdscr->_attrs, c, c | stdscr->_attrs);
#endif
			c |= stdscr->_attrs;;
		}

		_ll_move(y,x); /* SP->curptr may still be uninitialized (pvs) */
		/* overwrite process on NLS character */

		if ( *SP->curptr & A_NLSATTR )
		{
			if ( *SP->curptr & A_FIRSTOF2 )
			{
				if ( x >= SP->std_body[y+1]->length )
				{
					SP->std_body[y+1]->length=x+1;
				}
				*(SP->curptr+1) = *(SP->curptr+1) & A_ATTRIBUTES | ' ';
			}
			else
			{
				*(SP->curptr-1) = *(SP->curptr-1) & A_ATTRIBUTES | ' ';
			}
		}

		/* This line actually puts it out. */
		*SP->curptr = c;

		/* Set NLS Attributes */

		if ( x>0 && _oldx==x-1 && _oldy==y &&
		     !(*(SP->curptr-1) & A_NLSATTR) )
		{
			if ( FIRSTof2(*(SP->curptr-1) & A_CHARTEXT) &&
			     SECof2(*(SP->curptr) & A_CHARTEXT) ) 
			{
				*(SP->curptr-1) |= A_FIRSTOF2;
				*(SP->curptr) |= A_SECOF2;
			}
		}

		/* Check extended control characters */

		if (!(*SP->curptr & A_NLSATTR) && !( 0240 <= rawc && rawc <= 0376 ) && !isprint(rawc) )
		{
			_oldctrl = *SP->curptr;
		}
		else
		{
			_oldctrl = 0;
		}
		_oldx = x;
		_oldy = y;

		SP->curptr++;
		SP->virt_x++;
		x++;

		if (x >= stdscr->_maxx) {
			x = 0;
new_line:
			if (++y >= stdscr->_maxy)
				if (stdscr->_scroll) {
					--y;
					_ll_move(y, x);
					if (_oldctrl)
					{
						--_oldy;
					}
					else
					{
						_ll_refresh(stdscr->_use_idl);
					}
					_scrdown();
				}
				else {
# ifdef DEBUG
					int i;
					if(outf) fprintf(outf,
					    "ERR because (%d,%d) > (%d,%d)\n",
					    x, y, stdscr->_maxx, stdscr->_maxy);
					if(outf) fprintf(outf, "line: '");
					if(outf) for (i=0; i<stdscr->_maxy; i++)
						fprintf(outf, "%c", stdscr->_y[y-1][i]);
					if(outf) fprintf(outf, "'\n");
# endif
					return ERR;
				}
			_ll_move(y, x);
		}
# ifdef FULLDEBUG
		if(outf) fprintf(outf, "ADDCH: 2: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, stdscr->_firstch[y], stdscr->_lastch[y]);
# endif
		break;
	  case '\n':
# ifdef DEBUG
		if (outf) fprintf(outf, "newline, y %d, lengths %d->%d, %d->%d, %d->%d\n", y, y, SP->std_body[y]->length, y+1, SP->std_body[y+1]->length, y+2, SP->std_body[y+2]->length);
# endif
		if (SP->std_body[y+1])
			SP->std_body[y+1]->length = x;
		x = 0;
		goto new_line;
	  case '\r':
		x = 0;
		break;
	  case '\b':
		if (--x < 0)
			x = 0;
		break;
	}

	stdscr->_curx = x;
	stdscr->_cury = y;
#ifdef DEBUG
	if (outf) fprintf(outf, " => (%d,%d)]\n", stdscr->_cury, stdscr->_curx);
#endif
	return OK;
}
