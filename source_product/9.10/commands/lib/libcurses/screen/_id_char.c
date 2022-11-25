/* @(#) $Revision: 70.4 $ */    
/*
 * Modify current screen line 'old' to match desired line 'new'.
 * The old line is at position ln.  Each line is divided into 4 regions:
 *
 *	nlws, olws		- amount of leading white space on new/old line
 *	com_head		- length of common head
 *	nchanged, ochanged	- length of the parts changed
 *	com_tail		- length of a common tail
 */


#include "curses.ext"

#define min(a,b) (a<b ? a : b)

_id_char (old, new, ln)
register struct line   *old, * new;
{
	register chtype  *oc_beg, *nc_beg,/* Beginning of changed part */
			 *oc_end, *nc_end;/* End of changed part */
	chtype *p, *q;			/* scratch */
	int olws, nlws;			/* old/new leading white space */
    	int com_head, com_tail;		/* size of common head/tail */
	int cost_clear;			/* cost of clearing line */
	int cost_ovrwrt;		/* cost of overwriting line */
	int run, blanks;		/* used for 			*/
	chtype prevhl, curhl, oldhl;	/*	computing costs		*/
	int rl_clear, rl_ovrwrt;	/* used for computing		*/
	int hl_clear, hl_ovrwrt;	/* 	showstring complex parm	*/
	int ochanged, nchanged;		/* size of changed part */
	int samelen;			/* both lines are same length */
	int i, j, n;	/* scratch */
	int len, diff;
	static chtype nullline[] = {0, 0};

#ifdef DEBUG
	if(outf) fprintf(outf, "_id_char(%x, %x, %d)\n", old, new, ln);
	if(outf) fprintf(outf, "old: ");
	if(outf) fprintf(outf, "%8x: ", old);
	if (old == NULL) {
		if(outf) fprintf(outf, "()\n");
	} else {
		if(outf) fprintf(outf, "%4d ", old->length);
		for (j=0; j<old->length; j++) {
			n = old->body[j];
			if (n & A_ATTRIBUTES)
				putc('\'', outf);
			n &= A_CHARTEXT;
			if(outf) fprintf(outf, "%c", n>=' ' ? n : '.');
		}
		if(outf) fprintf(outf, "\n");
	}
	if(outf) fprintf(outf, "new: ");
	if(outf) fprintf(outf, "%8x: ", new);
	if (new == NULL) {
		if(outf) fprintf(outf, "()\n");
	} else {
		if(outf) fprintf(outf, "%4d ", new->length);
		for (j=0; j<new->length; j++) {
			n = new->body[j];
			if (n & A_ATTRIBUTES)
				putc('\'', outf);
			n &= A_CHARTEXT;
			if(outf) fprintf(outf, "%c", n>=' ' ? n : '.');
		}
		if(outf) fprintf(outf, "\n");
	}
#endif	DEBUG

	if (old == new)
	{
		return;
	}

	/* Start at the ends of the lines */
	if( old )
	{
		oc_beg = old -> body;
		oc_end = &old -> body[old -> length];
	}
	else
	{
		oc_beg = nullline;
		oc_end = oc_beg;
	}
	if( new )
	{
		nc_beg = new -> body;
		nc_end = &new -> body[new -> length];
	}
	else
	{
		nc_beg = nullline;
		nc_end = nc_beg;
	}

	/* Find leading and trailing blanks */
	olws = nlws = com_head = com_tail = 0;
	while (*--oc_end == ' ' && oc_end >= oc_beg)
		;
	while (*--nc_end == ' ' && nc_end >= nc_beg)
		;
	samelen = (nc_end-nc_beg) == (oc_end-oc_beg)
		;
	while( *oc_beg == ' ' && oc_beg <= oc_end )
	{
		oc_beg++;
		olws++;
	}
	while( *nc_beg == ' ' && nc_beg <= nc_end )
	{
		nc_beg++;
		nlws++;
	}

	/*
	 * Now find common heads and tails (com_head & com_tail).  If the
	 * lengths are the same, the change was probably at the beginning,
	 * so count common tail first.  This only matters if it could match
	 * both ways, for example, when changing
	 * "                  ####"
	 *    to
	 * "####              ####"
	 */
	if ( magic_cookie_glitch > 0 && nlws == olws )
	{
		chtype	*oc_pntr;
		chtype	*nc_pntr;

		oc_pntr = oc_beg;
		nc_pntr = nc_beg;

		while ( *oc_pntr==*nc_pntr && oc_pntr<=oc_end && nc_pntr<=nc_end )
		{
			oc_pntr++;
			nc_pntr++;
		}

		if ( oc_pntr - oc_end == 1 && nc_pntr - nc_end == 1 )
		{
			return;
		}
	}

	/* only calculate common head and tail if terminal has insert
	 * capability, since if it does not, we must rewrite entire line
	 * anyway.
	 */
	if (samelen && has_ic())
	{
		while( *oc_end==*nc_end && oc_beg<=oc_end && nc_beg<=nc_end )
		{
			if ( magic_cookie_glitch > 0 && 
			    ((*(nc_end-1) & A_ATTRIBUTES) != 
			     (*nc_end & A_ATTRIBUTES)) )
			{
				break;
			}

			if (*oc_end & A_SECOF2)
			{
				if (*(oc_end-1)!=*(nc_end-1))
				{
					break;
				}
				else
				{
					oc_end--;
					nc_end--;
					com_tail++;
				}
			}

			oc_end--;
			nc_end--;
			com_tail++;
		}
		while( *oc_beg==*nc_beg && oc_beg<=oc_end && nc_beg<=nc_end )
		{
			if ( magic_cookie_glitch > 0 &&
			    ((*(nc_beg-1) & A_ATTRIBUTES) != 
			     (*nc_beg & A_ATTRIBUTES)) )
			{
				break;
			}

			if (*oc_beg & A_FIRSTOF2)
			{
				if (*(oc_beg+1)!=*(nc_beg+1))
				{
					break;
				}
				else
				{
					oc_beg++;
					nc_beg++;
					com_head++;
				}
			}

			oc_beg++;
			nc_beg++;
			com_head++;
		}
	}
	else if (has_ic())
	{
		while( *oc_beg==*nc_beg && oc_beg<=oc_end && nc_beg<=nc_end )
		{
			if ( magic_cookie_glitch > 0 &&
			    ((*(nc_beg-1) & A_ATTRIBUTES) != 
			     (*nc_beg & A_ATTRIBUTES)) )
			{
				break;
			}

			if (*oc_beg & A_FIRSTOF2)
			{
				if (*(oc_beg+1)!=*(nc_beg+1))
				{
					break;
				}
				else
				{
					oc_beg++;
					nc_beg++;
					com_head++;
				}
			}

			oc_beg++;
			nc_beg++;
			com_head++;
		}
		while( *oc_end==*nc_end && oc_beg<=oc_end && nc_beg<=nc_end )
		{
			if ( magic_cookie_glitch > 0 &&
			     ((*(nc_end-1) & A_ATTRIBUTES) != 
			      (*nc_end & A_ATTRIBUTES)) )
			{
				break;
			}

			if (*oc_end & A_SECOF2)
			{
				if (*(oc_end-1)!=*(nc_end-1))
				{
					break;
				}
				else
				{
					oc_end--;
					nc_end--;
					com_tail++;
				}
			}

			oc_end--;
			nc_end--;
			com_tail++;
		}
	}

/*
 *	some basic checks to make sure preserving the common head or
 *	tail is worth the effort
 */
	cost_ovrwrt = hl_ovrwrt = hl_clear = 
		rl_clear = rl_ovrwrt = blanks = run = 0;
	if (com_head > 0) {
		if ((olws < nlws && com_head + olws < 8) ||
		    (olws > nlws && com_head + olws<<1 < nlws - 3) ||
		    (ceol_standout_glitch && olws == 0 && nlws && old->body[0] & A_ATTRIBUTES)) {
			oc_beg -= com_head;
			nc_beg -= com_head;
			com_head = 0;
		}
	}
	if (com_tail > 0) {
		int otail_col, ntail_col;
		if (com_head) {
			otail_col = oc_end - oc_beg;
			ntail_col = nc_end - nc_beg;
		}
		else {
			otail_col = oc_end - old->body;
			ntail_col = nc_end - new->body;
		}
		if (ntail_col > otail_col)
			cost_ovrwrt = 4;
		else if (ntail_col < otail_col)
			cost_ovrwrt = (otail_col - ntail_col) << 1;
		if (com_tail < cost_ovrwrt) {
			oc_end += com_tail;
			nc_end += com_tail;
			cost_ovrwrt = com_tail = 0;
		}
		/* can't allow insertion of plain text in highlighted region */
		else if (ceol_standout_glitch &&
		   	(hl_clear = nc_end[1] & A_ATTRIBUTES) &&
		   	(ntail_col > otail_col) &&
		   	((nc_beg[otail_col + 1] & A_ATTRIBUTES) == 0))
			cost_ovrwrt = 10000;
	}
	cost_clear = 1 + com_tail;

/*
 *	some checks to determine if we're better off clearing or overwriting
 *	the old line
 *	- we've made the assumption that the cost of rewriting the tail is
 *      simply it's length - this would be on the low side if there were
 *      highlighted chars in the tail, thus favoring clearing cost
 */
	p = nc_beg;
	q = oc_beg;
	if (com_head == 0) {
		if ((j = nlws - olws) > 0) p -= j;
		else if (j < 0) q += j;
	}
	prevhl = (p > new->body) ? p[-1] & A_ATTRIBUTES : 0;
	for (; p <= nc_end; p++) {
		if (*p == ' ') {
			if (++blanks < 4) cost_clear++;
			else rl_clear = 1;
		} else {
			blanks = 0;
			cost_clear++;
		}
		if (*p == *q) {
			if (++run < 4) cost_ovrwrt++;
			else rl_ovrwrt = 1;
		} else {
			run = 0;
			cost_ovrwrt++;
		}

		if (curhl = *p & A_ATTRIBUTES)
			hl_ovrwrt = hl_clear = 1;

		if (curhl != prevhl) {
			cost_clear += 4;
			if (!ceol_standout_glitch) cost_ovrwrt += 4;
		}

		if (ceol_standout_glitch) {
			oldhl = (q > oc_end) ? 0 : *q++ & A_ATTRIBUTES;
			if (oldhl) {
				hl_ovrwrt = 1;
				if (curhl == 0) 
					cost_ovrwrt = 10000;
				else if (curhl != oldhl)
					cost_ovrwrt += 4;
			}
			else if (curhl != prevhl)
				cost_ovrwrt += 4;
		}

		prevhl = curhl;
	}

	if (cost_clear < cost_ovrwrt) {
		if (com_tail) {
			oc_end += com_tail;
			nc_end += com_tail;
			com_tail = 0;
		 	/* don't know what's in the tail so play it safe */
			rl_clear = 1;
		}
	}

	ochanged = oc_end - oc_beg + 1;
	nchanged = nc_end - nc_beg + 1;

	/* Optimization: lines are identical, so return now */
	if( ochanged==0 && nchanged==0 && nlws==olws )
	{
#ifdef DEBUG
		if(outf) fprintf(outf, "identical lines, returning early\n");
#endif
		return;
	}

	/*
	 * On magic cookie terminals, we have to check for the possibility
	 * that there is a cookie that we must overwrite.  This is only
	 * necessary because a "go normal" cookie looks like an ordinary
	 * blank but must compare differently.
	 */
	if( magic_cookie_glitch > 0 && com_tail > 0 && nc_end[1] == ' ' &&
			oc_end[0]&A_ATTRIBUTES && oc_end[1] == ' ' )
	{
#ifdef DEBUG
		if (outf) fprintf(outf, "adding one because of magic cookie\n");
#endif
		oc_end++;
		nc_end++;
		ochanged++;
		nchanged++;
		com_tail--;
	}

	/* use last existing cookie as current attribute setting */
        if (magic_cookie_glitch >= 0) {
                if (com_head == 0) _fake_vidputs(0);
                else _fake_vidputs(nc_beg[-1] & A_ATTRIBUTES);
        }
	
	/*
	 * Now actually go fix up the line.
	 * There are several cases to consider.
	 * The most important thing to keep in mind is
	 * that deletions need to be done before insertions
	 * to prevent shifting good text off the end of the line.
	 */

	if (com_head == 0 && com_tail == 0)
	{
		/* No common text or terminal lacks insert capability
		   - must redraw entire line */
		if( ochanged == 0 && nchanged == 0 )
		{
			/* Empty lines - do nothing */
			_chk_typeahead();
			return;
		}
		/* If empty old line, pretend leading blanks */
		if( ochanged == 0 && !insert_null_glitch )
		{
			olws = nlws;
		}

		/* Make sure changed parts start at same column */
		j = nlws - olws;
		if( j > 0 )
		{
			nc_beg -= j;
			nchanged += j;
			nlws = olws;
		}
		else
		{
			if( j < 0 )
			{
				oc_beg += j;
				ochanged -= j;
				olws = nlws;
			}
		}

		if( !clr_eol || insert_null_glitch || cost_ovrwrt<=cost_clear )
		{
			_showstring(ln, nlws, nc_beg, nc_end, old, rl_ovrwrt | hl_ovrwrt);
			if( nlws + nchanged < olws + ochanged )
			{
				_pos(ln, nlws + nchanged);
				_clreol();
			}
		}
		else
		{
			if( ochanged > 0 )
			{
				_pos(ln, 0);
				_clreol();
				for (p=oc_beg; p<=oc_end; p++)
					*p = ' ';
			}
			_showstring(ln, nlws, nc_beg, nc_end, old, rl_clear | hl_clear);
		}
		_chk_typeahead();
		return;
	}

	if( com_head==0 )
	{
		/* We have only a common tail. */
		if( nchanged == 0 && ochanged == 0 )
		{
			_chk_typeahead();
			return;
		}
		i = (nlws + nchanged) - (olws + ochanged);
		/* Simplify things - force olws == nlws */
		j = nlws - olws;
		if( j > 0 )
		{
			nc_beg -= j;
			nchanged += j;
			nlws = olws;
		}
		else
		{
			if( j < 0 )
			{
				oc_beg += j;
				ochanged -= j;
				olws = nlws;
			}
		}
		if( i >= 0 )
		{
			_showstring(ln, nlws, nc_beg, nc_end-i, old, rl_ovrwrt | hl_ovrwrt);
			if( i > 0 )
			{
				_ins_string(ln, nlws+nchanged-i, nc_end-i+1, nc_end, hl_clear);
			}
		}
		else
		{
			_showstring(ln, nlws, nc_beg, nc_end, old, rl_ovrwrt | hl_ovrwrt);
			_pos(ln,nlws+nchanged);
			_delchars(-i);
		}
		_chk_typeahead();
		return;
	}

	/* At this point, we know there is a common head (com_head != 0) */
	if( nlws < olws )
	{
		/* Do the leading delete chars right away */
		_pos(ln, 0);
		_delchars(diff = olws - nlws);
		olws = nlws;
		len = old->length;
		for( i=0; i<len; i++ )
		{
			old->body[i] = old->body[i+diff];
		}
	}
	if (com_tail == 0)
	{
		if( nchanged == 0 && ochanged == 0 )
		{
			if( nlws > olws )
			{
				_ins_blanks(ln, 0, nlws - olws);
			}
			_chk_typeahead();
			return;
		}
		if( !clr_eol || insert_null_glitch || cost_ovrwrt<=cost_clear ) 
		{
			_showstring(ln, olws+com_head, nc_beg, nc_end, old, rl_ovrwrt | hl_ovrwrt);
			if( nchanged < ochanged )
			{
				_pos(ln, olws + com_head + nchanged);
				_clreol();
			}
		}
		else {
			if (ochanged > 0) {
				_pos(ln, olws+com_head);
				_clreol();	
				for (p=oc_beg; p<=oc_end; p++)
					*p = ' ';
				}
			_showstring(ln, olws+com_head, nc_beg, nc_end, old, rl_clear | hl_clear);
		}

		if( nlws > olws )
		{
			_ins_blanks(ln, 0, nlws - olws);
		}
	}
	else
	{
		if( nchanged > 0 && ochanged > 0 )
		{
			i = min(nchanged, ochanged);
			_showstring(ln, olws+com_head, nc_beg, nc_beg+i-1, old, rl_ovrwrt | hl_ovrwrt);
		}
		if( nchanged < ochanged )
		{
			_pos(ln, olws + com_head + nchanged);
			_delchars(ochanged - nchanged);
		}
		else
		{
			if( nchanged > ochanged )
			{
				_ins_string(ln, olws+com_head+ochanged,
					nc_beg + ochanged, nc_end, hl_clear);
			}
		}
		if( nlws > olws )
		{
			_ins_blanks(ln, 0, nlws-olws);
		}
	}
	_chk_typeahead();
	return;
}

/*
 * Insert nblanks blanks at position (sline, scol).
 */
static int
_ins_blanks(sline, scol, nblanks)
int sline, scol, nblanks;
{
#ifdef DEBUG
	if (outf) fprintf(outf, "_ins_blanks at (%d, %d) %d blanks\n",
		sline, scol, nblanks);
#endif
	_pos(sline, scol);
	if( nblanks > 1 && parm_ich )
	{
		/* Insert the characters and then draw on the blanks */
		_inschars(nblanks);
	}
	else
	{
		/*
		 * Type the blanks in "insert mode".  This includes
		 * having to send insert_character before each character
		 * is output.
		 */
		_insmode(1);
		_blanks(nblanks);
	}
	_insmode(0);
	_setmode();
}

/*
 * Insert the given string on the screen.
 * This is like _showstring but you know you're in insert mode.
 */
static
_ins_string(sline, scol, first, last, showflag)
int sline, scol;
chtype *first, *last; 
int showflag;
{
	int len = last-first+1;

#ifdef DEBUG
	if (outf) fprintf(outf, "_ins_string at (%d, %d) %d chars\n",
		sline, scol, len);
#endif
	_pos(sline, scol);

	/* fake out current attribute setting */
	if (ceol_standout_glitch)
                SP->phys_gr = last[1] & A_ATTRIBUTES;

	if( len > 1 && parm_ich )
	{
		/* Insert the characters and then draw on the blanks */
		_inschars(len);
		_showstring(sline, scol, first, last, NULL, showflag);
	}
	else
	{
		/*
		 * Type the characters in "insert mode".  This includes
		 * having to send insert_character before each character
		 * is output.
		 */
		_insmode(1);
		_showstring(sline, scol, first, last, NULL, showflag);
	}
	_insmode(0);
	_setmode();
}
