/* @(#) $Revision: 70.4 $ */

/*
 *   _fixScreen.c: Upon SIGWINCH, this routine adjusts the few basic elements
 *		   of screen layout: SP->std_body, SP->cur_body, stdscr
 *		   and _sumscr. (Where _sumscr is an internal ground window
 *		   which, derived from SP->cur_body, contains the graphics 
 *		   of the application at the time SIGWINCH is received.) 
 *		   If the result of SIGWINCH caused an expansion of the 
 *		   display area, these basic elements are padded with blank 
 *		   spaces.
 *
 *   Note: _fixScreen.c, _fixWindow.c, _winchTrap.c and _winlist.c 
 *   	   are used only when SIGWINCH is active. 
 */
#include "curses.ext"
#include <signal.h>

_fixScreen()
{
	register short  olines, ocolumns;
	register struct line **tline;
	register chtype *tbody;
	register int    i, j;
	register chtype **ty;
	register short  *tfirstch, *tlastch;
	register chtype *tln;
	register chtype *cp;

	olines = stdscr->_maxy;		/* original screen size */
	ocolumns = stdscr->_maxx;

	if (lines == olines && columns == ocolumns)
		return OK;

	if (lines > olines) {
		if (lines > SP->alloc_ln) {
			if ((tline = (struct line **) calloc(lines+2, sizeof(struct line *))) == NULL)
				return ERR;
			memcpy(tline, SP->std_body, (size_t)((SP->alloc_ln+2) * sizeof(struct line*)));
			cfree((char *)SP->std_body);
			SP->std_body = tline;

			if ((tline = (struct line **) calloc(lines+2, sizeof(struct line *))) == NULL)
				return ERR;
			memcpy(tline, SP->cur_body, (size_t)((SP->alloc_ln+2) * sizeof(struct line*)));
			cfree((char *)SP->cur_body);
			SP->cur_body = tline;

			for (i=SP->alloc_ln+1; i<lines+2; i++) {
				SP->std_body[i] = _line_alloc();
				SP->cur_body[i] = _line_alloc();
			}
			SP->alloc_ln = lines;
		} 

		if (lines > stdscr->_allocy) {
			/* fix stdscr */
			if ((ty = (chtype **)calloc(lines, sizeof (chtype *))) 
			    == NULL) 
				return ERR;
			memcpy(ty, stdscr->_y, (size_t)(stdscr->_allocy*sizeof(chtype *)));

			if ((tfirstch = (short *)calloc(lines, sizeof (short)))
			     == NULL) {	
				cfree((char *)ty);
				return ERR;
			}
			memcpy(tfirstch, stdscr->_firstch, (size_t)(stdscr->_allocy*sizeof(short)));

			if ((tlastch = (short *)calloc(lines, sizeof (short))) 
			     == NULL) {
				cfree((char *)ty);
				cfree((char *)tfirstch);
				return ERR;
			}
			memcpy(tlastch, stdscr->_lastch, (size_t)(stdscr->_allocy*sizeof(short)));

			for (i=stdscr->_allocy; i<lines; i++) {
                		if ((ty[i] = (chtype *)calloc(stdscr->_allocx, sizeof(chtype))) == NULL) {
                        		cfree((char *)tfirstch);
                        		cfree((char *)tlastch);
                        		cfree((char *)ty);
                        		return ERR;
                		}
                		else
                        		for (cp=ty[i]; cp < ty[i]+stdscr->_allocx;)
						/* pad extra area with blanks */
                                		*cp++ = ' ';
				tfirstch[i] = tlastch[i] = columns - 1;
			}

			/* now replace the temp with real */
			cfree((char *)stdscr->_firstch);
			stdscr->_firstch = tfirstch;
			cfree((char *)stdscr->_lastch);
			stdscr->_lastch = tlastch;
			cfree((char *)stdscr->_y);
			stdscr->_y = ty;

			stdscr->_allocy = lines;

			/* fix _sumscr: */
			if ((ty = (chtype **)calloc(lines, sizeof (chtype *))) 
			    == NULL) 
				return ERR;
			memcpy(ty, _sumscr->_y, (size_t)(_sumscr->_allocy*sizeof(chtype *)));

			if ((tfirstch = (short *)calloc(lines, sizeof (short)))
			     == NULL) {	
				cfree((char *)ty);
				return ERR;
			}
			memcpy(tfirstch, _sumscr->_firstch, (size_t)(_sumscr->_allocy*sizeof(short)));

			if ((tlastch = (short *)calloc(lines, sizeof (short))) 
			     == NULL) {
				cfree((char *)ty);
				cfree((char *)tfirstch);
				return ERR;
			}
			memcpy(tlastch, _sumscr->_lastch, (size_t)(_sumscr->_allocy*sizeof(short)));

			for (i=_sumscr->_allocy; i<lines; i++) {
                		if ((ty[i] = (chtype *)calloc(_sumscr->_allocx, sizeof(chtype))) == NULL) {
                        		cfree((char *)tfirstch);
                        		cfree((char *)tlastch);
                        		cfree((char *)ty);
                        		return ERR;
                		}
                		else
                        		for (cp=ty[i]; cp < ty[i]+_sumscr->_allocx;)
						/* pad extra area with blanks */
                                		*cp++ = ' ';
				tfirstch[i] = tlastch[i] = columns - 1;
			}

			/* now replace the temp with real */
			cfree((char *)_sumscr->_firstch);
			_sumscr->_firstch = tfirstch;
			cfree((char *)_sumscr->_lastch);
			_sumscr->_lastch = tlastch;
			cfree((char *)_sumscr->_y);
			_sumscr->_y = ty;

			_sumscr->_allocy = lines;
	
		} /* if lines > stdscr->_allocy */
	} /* if lines > olines */

	stdscr->_maxy   = _sumscr->_maxy  = lines;
	stdscr->_bmarg  = _sumscr->_bmarg = lines - 1;
	SP->des_bot_mgn = lines - 1;    /* update bottom margin */

	if (columns > ocolumns) {
		register struct line* si;
		register struct line* ci;

		for (i=0; i < SP->alloc_ln + 2; i++) {
			if ((si = SP->std_body[i]) != 0) {
				if (columns > si->alloc_ch) {
					if ((tbody = (struct chtype *) calloc(columns, sizeof(chtype))) == NULL)
						return ERR;
					memcpy(tbody, si->body, (size_t)(si->alloc_ch* sizeof(chtype)));
					cfree((char *)si->body);
					si->body = tbody;
					if (si->length == ocolumns)
						si->length = si->alloc_ch;
					si->alloc_ch = columns;
				}
				else if (si->length == ocolumns)
					si->length = columns;
			}

			if ((ci = SP->cur_body[i]) != si) {
				if (ci != 0) {
					if (columns > ci->alloc_ch) {
						if ((tbody = (struct chtype *) calloc(columns, sizeof(chtype))) == NULL)
							return ERR;
						memcpy(tbody, ci->body, (size_t)(ci->alloc_ch * sizeof(chtype)));
						cfree((char *)ci->body);
						ci->body = tbody;
						if (ci->length == ocolumns)
							ci->length = ci->alloc_ch;
						ci->alloc_ch = columns;
					}
					else if (ci->length == ocolumns)
						ci->length = columns;
				}
			}
		} /* for i */

		if (columns > stdscr->_allocx) {
			/* fix stdscr: */
			for (i=0; i < stdscr->_allocy; i++) {
				if ((tln = (chtype *)calloc(columns, sizeof(chtype))) == NULL) 
				return ERR;
				memcpy(tln, stdscr->_y[i], (size_t)(stdscr->_allocx*sizeof(chtype)));
				for (j=stdscr->_allocx; j<columns; j++)
					/* pad extra area with blanks */
					tln[j] = ' ';
				cfree((char *) stdscr->_y[i]);
				stdscr->_y[i] = tln;
				stdscr->_firstch[i] = stdscr->_lastch[i] = columns-1;
			}
			stdscr->_allocx = columns;

			/* fix _sumscr: */
			for (i=0; i < _sumscr->_allocy; i++) {
				if ((tln = (chtype *)calloc(columns, sizeof(chtype))) == NULL) 
				return ERR;
				memcpy(tln, _sumscr->_y[i], (size_t)(_sumscr->_allocx*sizeof(chtype)));
				for (j=_sumscr->_allocx; j<columns; j++)
					/* pad extra area with blanks */
					tln[j] = ' ';
				cfree((char *) _sumscr->_y[i]);
				_sumscr->_y[i] = tln;
				_sumscr->_firstch[i] = _sumscr->_lastch[i] = columns-1;
			}
			_sumscr->_allocx = columns;
		} 
	} else if (columns < ocolumns)
		/* update line length for std_body only */
		for (i=1; i<SP->alloc_ln + 2; i++) 
			if (SP->std_body[i]->length > columns) 
				SP->std_body[i]->length = columns;

	stdscr->_maxx = _sumscr->_maxx = columns;
	SP->doclear = 1;                /* clear screen at next refresh */

	/* fay: extract from makenew(), but need modification to unset values */
	/*
        if (bx + ncols == columns) {
                stdscr->_flags |= _ENDLINE;
                if (nlines == lines && ncols == columns &&
                    by == 0 && bx == 0 && scroll_forward)
                        stdscr->_flags |= _FULLWIN;
                if (by + nlines == lines && auto_right_margin)
                        stdscr->_flags |= _SCROLLWIN;
        }
	*/

	/* synchronize _sumscr contents by copying from SP->cur_body */
	for (i=0; i<olines; i++)
		for (j=0; j<ocolumns; j++)
			_sumscr->_y[i][j] = SP->cur_body[i+1]->body[j];

	touchwin(stdscr); 
	wnoutrefresh(stdscr);
	touchwin(_sumscr);
}
