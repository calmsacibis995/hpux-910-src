/* @(#) $Revision: 70.4 $ */     
/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

#include	"curses.ext"
#include 	<signal.h>

extern	WINDOW *lwin;

/* Update screen */
doupdate()
{
	int rc;
	extern int _endwin;
	int _outch();

#ifdef	DEBUG
	if(outf) fprintf( outf, "doupdate()\n" );
#endif	DEBUG

	if( lwin == NULL )
	{
		return ERR;
	}

	if( _endwin )
	{
		/*
		 * We've called endwin since last refresh.  Undo the
		 * effects of this call.
		 */

		_fixdelay(FALSE, SP->fl_nodelay);
		if (stdscr->_use_meta)
			tputs(meta_on, 1, _outch);
		_endwin = FALSE;
		SP->doclear = TRUE;
		reset_prog_mode();
	}

	/* Tell the back end where to leave the cursor */
	if( lwin->_leave )
	{
#ifdef	DEBUG
		if(outf) fprintf( outf, "'_ll_move(-1, -1)' being done.\n" );
#endif	DEBUG
		_ll_move(-1, -1);
	}
	else
	{
#ifdef	DEBUG
		if(outf) fprintf( outf, "'lwin->_cury+lwin->_begy, lwin->_curx+lwin->_begx' being done.\n" );
#endif	DEBUG
		if(!(lwin->_flags & _ISPAD))
			_ll_move( lwin->_cury+lwin->_begy, lwin->_curx+lwin->_begx );
		else /* PAD Window */
			_ll_move( lwin->_begy, lwin->_begx );
	}
#ifdef	DEBUG
	if(outf) fprintf( outf, "doing 'rc = _ll_refresh(lwin->_use_idl)'.\n" );
#endif	DEBUG
	rc = _ll_refresh(lwin->_use_idl);
#ifdef	DEBUG
	_dumpwin(lwin);
#endif	DEBUG
	return rc;
}
