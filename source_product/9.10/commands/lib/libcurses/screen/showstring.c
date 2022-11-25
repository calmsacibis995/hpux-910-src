/* @(#) $Revision: 70.7 $ */   

#include "curses.ext"

extern	int	_outch();
extern	char	*tparm();

/*
 * Dump the string running from first to last out to the terminal.
 * Take into account attributes, and attempt to take advantage of
 * large pieces of white space and text that's already there.
 * oldline is the old text of the line.
 *
 * Variable naming convention: *x means "extension", e.g. a rubber band
 * that briefly looks ahead; *c means a character version of an otherwise
 * chtype pointer; old means what was on the screen before this call;
 * left means the char 1 space to the left.
 */
_showstring(sline, scol, first, last, oldlp, complex)
int sline, scol;
chtype *first, *last; 
struct line *oldlp;
int complex;				/* nontrivial update required */
{
	int prevhl=SP->virt_gr, thishl;	/* highlight state tty is in	 */
	int oldchl;			/* hl state prev char was in	 */
	register chtype *p, *px;	/* current char being considered */
	register chtype *oldp, *oldpx;	/* stuff under p and px		 */
	register char *pc, *pcx;	/* like p, px but in char buffer */
	chtype *tailoldp;		/* last valid oldp		 */
	int oldlen;			/* length of old line		 */
	int lcol, lrow;			/* position on screen		 */
	int oldc;			/* char at oldp			 */
	chtype *prev_oldp;		/* old prev char pointer         */
	int leftoldc, leftnewc;		/* old & new chars to left of p  */
	int diff_cookies;		/* magic cookies changed	 */
	int diff_attrs;			/* highlights changed		 */
	int change_attrs;		/* hightlights changed		 */
	chtype *oldline;
#ifdef NONSTANDARD
	static
#endif NONSTANDARD
	   char firstc[256], *lastc;	/* char copy of input first, last */
	   char *eol_ptr;		/* end of new line pointer	  */
	chtype *end_of_line_ptr();      /* Returns pointer to the last    */
					/* character in the new copy of   */
					/* the current line.              */
#ifdef DEBUG
	if(outf) fprintf(outf, "_showstring((%d,%d) %d:'", sline, scol, last-first+1);
	if(outf)
		for (p=first; p<=last; p++) {
			thishl = *p & A_ATTRIBUTES;
			if (thishl)
				putc('\'', outf);
			putc(*p & A_CHARTEXT, outf);
		}
	if(outf) fprintf(outf, "').\n");
#endif
	if (last-first > columns) {
		_pos(lines-1, 0);
#ifndef		NONSTANDARD
		fprintf(stderr, "Bad call to _showstring, first %x, last %x,\
 diff %dpcx\n", first, last, last-first);
#endif
		abort();
	}

	if (oldlp) {
		oldline = oldlp->body;
		oldp = oldline+scol;
		oldlen = oldlp->length;
	}
	else {
		oldline = NULL;
		oldp = 0;
		oldlen = 0;
	}


	for (p=first,lastc=firstc,lcol=scol; p<=last; lcol++) {
		*lastc++ = (char) (*p & A_CHARTEXT);
		p++;	/* On a separate line due to C optimizer bug */
#ifdef FULLDEBUG
	if(outf) fprintf(outf, "p %x '%c' %o, lastc %x %o, oldp %x %o, hl %d\n", p, p[-1], p[-1], lastc, lastc[-1], oldp, oldp ? oldp[-1] : 0, hl);
#endif
	}
	*lastc = '\0';
	lastc--;

	lcol = scol; lrow = sline;

	if (!complex) {
		/* Simple, common case.  Do it fast. */
		_pos(lrow, lcol);
		_hlmode(0);
		_writechars(firstc, lastc);
		return;
	}

#ifdef DEBUG
	if(outf) fprintf(outf, "oldlp %x, oldline %x, oldlen %d 0x%x\n", oldlp, oldline, oldlen, oldlen);
	if(outf) fprintf(outf, "old body('");
	if (oldlp)
		for (p=oldline; p<=oldline+oldlen; p++)
			if(outf) fprintf(outf, "%c", *p);
	if(outf) fprintf(outf, "').\n");
#endif
	if (lcol == 0 || oldlp == NULL) oldc = ' ';
	else oldc = first[-1];

	tailoldp = oldline + oldlen;
	change_attrs = 0;

	/*
	** If we have an HP terminal, adjust highlighting from right to left
	** before doing the normal processing.  This in intended to reduce
	** flutter.
	*/
	if (ceol_standout_glitch) {
		chtype	attr,nxtattr;
		int	i;

		/*
		** Start by setting attribute just past end of line if it is
		** different from what we are writing.
		*/
		i = last - first + 1;
		if (oldlp && scol + i < oldlen) {		/* DSDe420363 */
			attr = oldp[i] & A_ATTRIBUTES;
			if (attr != (*last & A_ATTRIBUTES)) {
				_pos(lrow, scol + i);
				_hlmode(attr);
				_forcehl();
			}
		}
	}

	for (p=first, oldp=oldline+lcol, pc=firstc; pc<=lastc; p++,oldp++,pc++) {
		leftoldc = oldc & A_ATTRIBUTES;
		leftnewc = p[-1] & A_ATTRIBUTES;

		thishl = *p & A_ATTRIBUTES;
#ifdef DEBUG
		if(outf) fprintf(outf, "prevhl %o, thishl %o\n", prevhl, thishl);
#endif
		diff_cookies = magic_cookie_glitch>=0 && (leftoldc != leftnewc);
		diff_attrs = ceol_standout_glitch && (((*p)&A_ATTRIBUTES) != leftnewc);
		if (oldp >= tailoldp)
			oldc = ' ';
		else
			oldc = *oldp;
		oldchl = oldc & A_ATTRIBUTES;
#ifdef DEBUG
		if(outf) fprintf(outf, "p %x *p %o, pc %x *pc %o, oldp %x, *oldp %o, lcol %d, lrow %d, oldc %o\n", p, *p, pc, *pc, oldp, *oldp, lcol, lrow, oldc);
#endif
		if (*p != oldc ||
		SP->virt_irm == 1 || diff_cookies || diff_attrs ||
		((*p & A_FIRSTOF2) && (*(p+1) != *(oldp+1))) ||
		((*p & A_SECOF2) && (*(p-1) != *(oldp-1))) ||
		(( magic_cookie_glitch > 0 ) && ( thishl != prevhl )) ||
		change_attrs != 0 ||
		insert_null_glitch && oldp >= oldline+oldlen) {
			register int n;

			if ( magic_cookie_glitch > 0 )
				change_attrs = 1;

			_pos(lrow, lcol);

			/* Force highlighting to be right */
			_hlmode(thishl);

			if (ceol_standout_glitch) {
				if (oldchl && thishl != oldchl) 
					_forcehl();
			        else if ((oldchl != leftoldc) &&
					 (thishl != oldchl ||
				   	 (thishl == oldchl && p > first)))
					SP->phys_gr = oldchl;
			}

			if (thishl != prevhl) {
				if (magic_cookie_glitch > 0) {
					_sethl();
					p += magic_cookie_glitch;
					oldp += magic_cookie_glitch;
					pc += magic_cookie_glitch;
					lcol += magic_cookie_glitch;
				}
			}

			/*
			 * Gather chunks of chars together, to be more
			 * efficient, and to allow repeats to be detected.
			 * Not done for blanks on cookie terminals because
			 * the last one might be a cookie.
			 */
			if (repeat_char &&
			   (magic_cookie_glitch<0 || *pc != ' ')) {
				for (px=p+1,oldpx=oldp+1;
					px<=last && *p==*px;
					px++,oldpx++) {
					if(!(oldpx<tailoldp && *p==*oldpx))
						break;
				}
				px--; oldpx--;
				n = px - p;
				pcx = pc + n;
			} else {
				n = 0;
				pcx = pc;
			}
			_writechars(pc, pcx);
			lcol += n; pc += n; p += n; oldp += n;
			prevhl = thishl;
		}
		lcol++;
	}
	if (magic_cookie_glitch >= 0
		&& ((lcol < COLS && *p&A_ATTRIBUTES) || prevhl)) { 
		if (!(ceol_standout_glitch && lcol >=COLS))
		     _pos(lrow, lcol);  /* Position ourselves correctly. */
					/* hp terminals do not allow to   
					   move cursor to out of screen. */

		/* Have to turn off highlighting if at end of line */

		/*if ( p <= end_of_line_ptr(sline) )*/
		if (lcol < COLS )
		    _hlmode(*p&A_ATTRIBUTES);
		else {
		    /*_hlmode(0);*/ /* end of line */
		    SP->virt_gr = 0;
		    SP->phys_gr = 0;
		}
		_sethl();
	}

}


/* end_of_line_ptr - return pointer to the last character in
 *                   the new copy of the current line.
 */

    static chtype *
end_of_line_ptr(line)
    int line;
{
    struct line *newlp; /* pointer to "new" line */

    newlp = SP->std_body[line+1];
    return &newlp->body[newlp->length-1];
}
