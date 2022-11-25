/* @(#) $Revision: 72.1 $ */      
/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

#include	"curses.ext"
#include 	<signal.h>
#include	<nl_ctype.h>

/*----------------------------------------------------------------------*/
/*									*/
/* The following ifdef is a method of determining whether or not we are */
/* on a 9.0 system or 8.xx system.					*/
/* The specific problem is that if we are using a 8.07 os then SIGWINCH */
/* is defined in signal.h however, curses does not yet support this     */
/* feature; thus the header /usr/include/curses.h does not check and    */
/* create/not create the SIGWINCH entries into the window structure.    */
/* The method that we will use to determine if this is a 9.0 os, which  */
/* is a valid system for sigwinch is to check the SIGWINDOW flag.  If   */
/* This flag is set, then it's 9.0 otherwise, it is a os prior to 9.0   */
/*  									*/
/*  Assumptions:  							*/
/*     		OS		SIGWINCH	SIGWINDOW		*/
/*		-----------------------------------------		*/
/*		9.0 +		True		True			*/
/*		8.07		True		Not Set			*/
/*		Pre-8.07	Not Set		Not Set			*/
/*									*/
/* This should probably be removed after support for 8.xx is dropped	*/
/*									*/
/*	Note: SIGWINCH should be Defined for 9.0 and forward.  If it 	*/
/*	      is defined for Pre-9.0, then it should be undefined.	*/
/*									*/

#ifndef _SIGWINDOW
#  ifdef SIGWINCH
#    undef SIGWINCH
#  endif
#endif
/*									*/
/*		Done with special SIGWINCH definitions.			*/
/*----------------------------------------------------------------------*/

extern	WINDOW *lwin;

/* Put out window but don't actually update screen. */
wnoutrefresh(win)
register WINDOW	*win;
{
	register int wy, y;
	register chtype	*nsp, *lch;
	int cnt_blanks;

	_showctrl();

# ifdef DEBUG
	if( win == stdscr )
	{
		if(outf) fprintf(outf, "REFRESH(stdscr %x)", win);
	}
	else
	{
		if( win == curscr )
		{
			if(outf) fprintf(outf, "REFRESH(curscr %x)", win);
		}
		else
		{
			if(outf) fprintf(outf, "REFRESH(%d)", win);
		}
	}
	if(outf) fprintf(outf, " (win == curscr) = %d, maxy %d\n", win, (win == curscr), win->_maxy);
	if( win != curscr )
	{
		_dumpwin( win );
	}
	if(outf) fprintf(outf, "REFRESH:\n\tfirstch\tlastch\n");
# endif	DEBUG
	/*
	 * initialize loop parameters
	 */

	if( win->_clear || win == curscr || SP->doclear )
	{
# ifdef DEBUG
		if (outf) fprintf(outf, "refresh clears, win->_clear %d, curscr %d\n", win->_clear, win == curscr);
# endif	DEBUG
		SP->doclear = 1;
		win->_clear = FALSE;
		if( win != curscr )
		{
			touchwin( win );
		}
	}

	if( win == curscr )
	{
#ifdef	DEBUG
	if(outf) fprintf(outf, "Calling _ll_refresh(FALSE)\n" );
#endif	DEBUG
		_ll_refresh(FALSE);
		return OK;
	}
#ifdef	DEBUG
	if(outf) fprintf(outf, "Didn't do _ll_refresh(FALSE)\n" );
#endif	DEBUG

	for( wy = 0; wy < win->_maxy; wy++ )
	{
		if( win->_firstch[wy] != _NOCHANGE )
		{
			cnt_blanks = 0;
			y = wy + win->_begy;
			lch = &win->_y[wy][win->_lastch[wy]];
			nsp = &win->_y[wy][win->_firstch[wy]];
			_ll_move(y, win->_begx + win->_firstch[wy]);

			if (!_CS_SBYTE)
			{
				if( nsp <= lch && SP->virt_x < columns )
				{
					if( *SP->curptr & A_SECOF2 )
					{
						*(SP->curptr-1) = *(SP->curptr-1) & A_ATTRIBUTES | ' ';
					}
				}

				while( nsp <= lch )
				{
					if( SP->virt_x < columns )
					{
						SP->virt_x++;
						if( *nsp & A_SECOF2 )
						{
							*SP->curptr++ = ((*(nsp-1) & A_ATTRIBUTES) | (*nsp++ & ~A_ATTRIBUTES));
						} else
						{
							*SP->curptr++ = *nsp++;
						}
					}
					else
					{
						break;
					}
				}

				if( SP->virt_x < columns )
				{
					if( *SP->curptr & A_SECOF2 )
					{
						*SP->curptr = *SP->curptr & A_ATTRIBUTES | ' ';
					}
				}
			}
			else
			{
				while( nsp <= lch )
				{
					if (SP->virt_x < columns)
					{
						if ((*SP->curptr++ = *nsp++) == ' ')
						{
							++cnt_blanks;
						}
						else
						{
							SP->virt_x += cnt_blanks + 1;
							cnt_blanks = 0;
						}
					}
					else
					{
						break;
					}
				}
			}

			win->_firstch[wy]= win->_lastch[wy]= _NOCHANGE;
		}
	}
#ifdef  SIGWINCH
	/* if window is invisible, don't bother for futher process */
	if (win->_maxy > 0) lwin = win;
#else
	lwin = win;
#endif
	return OK;
}
