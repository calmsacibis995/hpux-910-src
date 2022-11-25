/* @(#) $Revision: 70.1 $ */      
/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

#include	"curses.ext"
#include	<nl_ctype.h>

extern	WINDOW *lwin;

/* Put out pad but don't actually update screen. */
pnoutrefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)
register WINDOW	*pad;
int pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol;
{
	register int pr, r, c;
	register chtype	*nsp, *lch;

	_showctrl();

# ifdef DEBUG
	if(outf) fprintf(outf, "PREFRESH(pad %x, pcorner %d,%d, smin %d,%d, smax %d,%d)", pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
	_dumpwin(pad);
	if(outf) fprintf(outf, "PREFRESH:\n\tfirstch\tlastch\n");
# endif

	/* Make sure everything fits */
	if (pminrow >= pad->_maxy || pmincol >= pad->_maxx)
		return ERR;
	if (pminrow < 0) pminrow = 0;
	if (pmincol < 0) pmincol = 0;
	if (sminrow < 0) sminrow = 0;
	if (smincol < 0) smincol = 0;
	if (smaxrow >= lines) smaxrow = lines-1;
	if (smaxcol >= columns) smaxcol = columns-1;
	if (smaxrow - sminrow >= pad->_maxy - pminrow)
		smaxrow = sminrow + (pad->_maxy - pminrow)-1;
	if (smaxcol - smincol >= pad->_maxx - pmincol)
		smaxcol = smincol + (pad->_maxx - pmincol)-1;

	/* save left_hand corner of the pad window on
	   physical screen.					 */

		r = pad->_cury - pminrow;
		if ( r < 0 ) r = 0;
		r = r + sminrow;
		if ( r > smaxrow ) r = smaxrow;
		pad->_begy = r;

		c = pad->_curx - pmincol;
		if ( c < 0 ) c = 0;
		c = c + smincol;
		if ( c > smaxcol ) c = smaxcol;
		pad->_begx = c;

	/* Copy it out, like a refresh, but appropriately offset */
	for (pr=pminrow,r=sminrow; r <= smaxrow; r++,pr++) {
		/* No record of what previous loc looked like, so do it all */
		lch = &pad->_y[pr][pad->_maxx-1];
		nsp = &pad->_y[pr][pmincol];
		_ll_move(r, smincol);

		if (!_CS_SBYTE) { /* if using 16-bit chars */
			c=smincol;
			if ( nsp<=lch && SP->virt_x < columns && c <= smaxcol){
				if ( *SP->curptr & A_SECOF2 ){
					*(SP->curptr-1) = *(SP->curptr-1) & A_ATTRIBUTES | ' ';
				}

				if ( *nsp & A_SECOF2 ){
					*SP->curptr = *nsp & A_ATTRIBUTES | ' ';
				} else{
					*SP->curptr = *nsp;
				}

				*SP->curptr++;
				*nsp++;
				SP->virt_x++;
				c++;
			}

			for (; nsp<=lch; c++) {
				if (SP->virt_x < columns && c <= smaxcol){
					SP->virt_x++;
					if ( *nsp & A_SECOF2 ){
						*SP->curptr++ = ((*(nsp-1) & A_ATTRIBUTES) | (*nsp++ & ~A_ATTRIBUTES));
					} else{
						*SP->curptr++ = *nsp++;
					}
				} else{
					break;
				}
			}

			if ( *--SP->curptr & A_FIRSTOF2 ){
				*SP->curptr = *SP->curptr & A_ATTRIBUTES | ' ';
			}
			SP->curptr++;

			if ( SP->virt_x < columns ){
				if ( *SP->curptr & A_SECOF2 ){
					*SP->curptr = *SP->curptr & A_ATTRIBUTES | ' ';
				}
			}
		} else {
                	for (c=smincol; nsp<=lch; c++) {
                        	if (SP->virt_x++ < columns && c <= smaxcol)
                                	*SP->curptr++ = *nsp++;
                        	else
                                	break;
                        }
		}

		pad->_firstch[pr] = pad->_lastch[pr] = _NOCHANGE;
	}
	lwin = pad;
	return OK;
}
