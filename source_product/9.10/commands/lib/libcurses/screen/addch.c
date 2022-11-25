/* @(#) $Revision: 72.2 $ */   
# include	"curses.ext"
# include	<ctype.h>
# include	<nl_ctype.h>

extern	short	_oldx;
extern	short	_oldy;
extern	chtype	_oldctrl;
extern	WINDOW	*_oldwin;
extern	int	_poschanged;

/*
 *	This routine prints the character in the current position.
 *	Think of it as putc.
 *
 */
waddch(win, c)
register WINDOW	*win;
register chtype		c;
{
	register int		x, y;
	char *uctr;
	register int rawc = c & A_CHARTEXT;
	register chtype	ch;

	if (_oldctrl)
	{
		if ( (win != _oldwin) ||
		     (!(_oldy==win->_cury && _oldx==win->_curx-1) && !(_oldx==win->_maxx-1 && _oldy==win->_cury-1)) ||
	 	     (!(FIRSTof2(_oldctrl & A_CHARTEXT) && SECof2(rawc))) )
		{
			_setctrl();
		}
	}

	_poschanged = 0;

	c = c & ~A_NLSATTR;
	x = win->_curx;
	y = win->_cury;
# ifdef DEBUG
	if (outf)
		if (c == rawc)
			fprintf(outf, "'%c'", rawc);
		else
			fprintf(outf, "'%c' %o, raw %o", c, c, rawc);
# endif
	if (y >= win->_maxy || x >= win->_maxx || y < 0 || x < 0)
	{
# ifdef DEBUG
if(outf)
{
fprintf(outf,"off edge, (%d,%d) not in (%d,%d)\n",y,x,win->_maxy,win->_maxx);
}
# endif
		return ERR;
	}
	switch( rawc )
	{
	case '\t':
		{
			register int newx;

			for( newx = x + (8 - (x & 07)); x < newx; x++ )
			{
				if( waddch(win, ' ') == ERR )
				{
					return ERR;
				}
			}
			return OK;
		}
	  default:
		if ( _CS_SBYTE && !(0240 <= rawc && rawc <= 0376) && !isprint(rawc) ) 
		{
			uctr = unctrl(rawc);
			waddch(win, (chtype)uctr[0]|(c&A_ATTRIBUTES));
			waddch(win, (chtype)uctr[1]|(c&A_ATTRIBUTES));
			return OK;
		}

		if (!_CS_SBYTE)		/* only for 16-bit chars */
		{
			if (x == 0 && y >= 1)
			{
				if ( _oldwin==win && _oldx==win->_maxx-1 && _oldy==y-1 &&
				     !((ch=win->_y[y-1][win->_maxx-1]) & A_NLSATTR) &&
				     FIRSTof2(ch & A_CHARTEXT) && SECof2(rawc))
				{
					win->_y[y-1][win->_maxx-1] = 
					win->_y[y-1][win->_maxx-1] & A_ATTRIBUTES | ' ';
	
					waddch(win,ch & A_CHARTEXT);
					waddch(win,c);
					return OK;
				}
			}
		}
				
		if( win->_attrs )
		{
#ifdef	DEBUG
if(outf) fprintf(outf, "(attrs %o, %o=>%o)", win->_attrs, c, c | win->_attrs);
#endif	DEBUG
			c |= win->_attrs;;
		}

		if( win->_firstch[y] == _NOCHANGE )
		{
			win->_firstch[y] = win->_lastch[y] = x;
		}
		else
		{
			if( x < win->_firstch[y] )
			{
				win->_firstch[y] = x;
			}
			else
			{
				if( x > win->_lastch[y] )
				{
					win->_lastch[y] = x;
				}
			}
		}

		if (!_CS_SBYTE)
		{
			/* overwrite process on NLS character */

			if ( (ch=win->_y[y][x]) & A_NLSATTR )
			{
				if ( ch & A_FIRSTOF2 )
				{
					if ( x >= win->_lastch[y] )
					{
						win->_lastch[y]=x+1;
					}
					win->_y[y][x+1] = win->_y[y][x+1] & A_ATTRIBUTES | ' ';
				}
				else
				{
					if ( x <= win->_firstch[y] )
					{
						win->_firstch[y]=x-1;
					}
					win->_y[y][x-1] = win->_y[y][x-1] & A_ATTRIBUTES | ' ';
				}
			}

			win->_y[y][x] = c;

			/* Set NLS Attributes */

			if ( x>0 && !(win->_y[y][x-1] & A_NLSATTR) )
			{
				if ( FIRSTof2(win->_y[y][x-1] & A_CHARTEXT) &&
				     SECof2(win->_y[y][x] & A_CHARTEXT) )	
				{
					win->_y[y][x-1] |= A_FIRSTOF2;
					win->_y[y][x] |= A_SECOF2;
				}
			}
		}
		else
		{
			win->_y[y][x] = c;
		}

		/* Check extended control characters */

 		if (!(win->_y[y][x] & A_NLSATTR) && FIRSTof2(rawc))
		{
			_oldx = x;
			_oldy = y;
			_oldwin = win;
			_oldctrl = win->_y[y][x];
		}
		else
		{
			_oldctrl = 0;
		}

		x++;
		if (x >= win->_maxx)
		{
			x = 0;
new_line:
			if (++y > win->_bmarg)
			{
				if (win->_scroll && !(win->_flags&_ISPAD))
				{
#ifdef	DEBUG
	if(outf)
	{
		fprintf( outf, "Calling wrefresh( 0%o ) & _tscroll(  0%o  )\n",
				win, win );
	}
#endif	DEBUG
					if (_oldctrl)
					{
						--_oldy;
					}
					else
					{
						wrefresh(win);
					}
					_tscroll( win );
					--y;
				}
				else
				{
# ifdef DEBUG
					int i;
					if(outf)
					{
					    fprintf(outf,
					    "ERR because (%d,%d) > (%d,%d)\n",
					    x, y, win->_maxx, win->_maxy);
					    fprintf(outf, "line: '");
					    for (i=0; i<win->_maxy; i++)
						fprintf(outf, "%c",
							win->_y[y-1][i]);
					    fprintf(outf, "'\n");
					}
# endif	DEBUG
					return ERR;
				}
			}
		}
# ifdef DEBUG
		if(outf) fprintf(outf, "ADDCH: 2: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, win->_firstch[y], win->_lastch[y]);
# endif	DEBUG
		break;
	  case '\n':
		wclrtoeol(win);
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

	win->_curx = x;
	win->_cury = y;
	return OK;
}
