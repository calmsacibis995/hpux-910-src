/* @(#) $Revision: 72.1 $ */      
#include "curses.ext"

static int oldmode = 0;	/* This really should be in the struct term */
char *tparm();

/* nooff: modes that don't have an explicit "turn me off" capability */
#define nooff	(A_PROTECT|A_INVIS|A_BOLD|A_DIM|A_BLINK|A_REVERSE)
/* hilite: modes that could be faked with standout in a pinch. */
#define hilite	(A_UNDERLINE|A_BOLD|A_DIM|A_BLINK|A_REVERSE)

vidputs(newmode, outc)
int newmode;
int (*outc)();
{
	int curmode = oldmode;

#ifdef DEBUG
	if (outf) fprintf(outf, "vidputs oldmode=%o, newmode=%o\n", oldmode, newmode);
#endif
	if (newmode || !exit_attribute_mode
				 || (!newmode && !exit_alt_charset_mode)) {
		if (set_attributes) {
			int mode;

			mode = newmode & A_ATTRIBUTES;
	  	  /* if attrib strings defined and only 1 mode bit set        */
		  /*   (mode-1)&mode :  True if 2 or more bits are set        */
		  /*   attrstr?attrstr[hashattr(mode)]:0 : Checks if there is */
		  /*                        a pre-generated attribute string. */

			if ( mode && !( (mode-1) & mode) && ( attrstr?attrstr[hashattr(mode)]:0 ) )
				tputs(attrstr[hashattr(mode)],
				      1, outc);
		    	else
				tputs(tparm(set_attributes,
					(newmode & A_STANDOUT) != 0,
					(newmode & A_UNDERLINE) != 0,
					(newmode & A_REVERSE) != 0,
					(newmode & A_BLINK) != 0,
					(newmode & A_DIM) != 0,
					(newmode & A_BOLD) != 0,
					(newmode & A_INVIS) != 0,
					(newmode & A_PROTECT) != 0,
					(newmode & A_ALTCHARSET) != 0),
				      1, outc);
			curmode = newmode;
		} else {
			if ((oldmode&nooff) != (newmode&nooff)) {
				if (exit_attribute_mode) {
					tputs(exit_attribute_mode, 1, outc);
				} else if (oldmode == A_UNDERLINE && exit_underline_mode) {
					tputs(exit_underline_mode, 1, outc);
				} else if (exit_standout_mode) {
					tputs(exit_standout_mode, 1, outc);
				}
				curmode = oldmode = 0;
			}

			/* First, exit all the modes we need to exit: */

			if ( !(newmode&A_ALTCHARSET) && (oldmode&A_ALTCHARSET) ) {
				tputs(exit_alt_charset_mode, 1, outc);
				curmode &= ~A_ALTCHARSET;
			}
			if ( !(newmode&A_UNDERLINE) && (oldmode&A_UNDERLINE) ) {
				tputs(exit_underline_mode, 1, outc);
				curmode &= ~A_UNDERLINE;
			}
			if ( !(newmode&A_STANDOUT) && (oldmode&A_STANDOUT) ) {
				tputs(exit_standout_mode, 1, outc);
				curmode &= ~A_STANDOUT;
			}

			/* now enter all the modes we need to enter: */

			if ( ceol_standout_glitch )
			    curmode = 0;

			if ((newmode&A_ALTCHARSET) &&
			    (ceol_standout_glitch || !(oldmode&A_ALTCHARSET))) {
				tputs(enter_alt_charset_mode, 1, outc);
				curmode |= A_ALTCHARSET;
			}
			if ((newmode&A_PROTECT) &&
			    (ceol_standout_glitch || !(oldmode&A_PROTECT))) {
				tputs(enter_protected_mode, 1, outc);
				curmode |= A_PROTECT;
			}
			if ((newmode&A_INVIS) &&
			    (ceol_standout_glitch || !(oldmode&A_INVIS))) {
				tputs(enter_secure_mode, 1, outc);
				curmode |= A_INVIS;
			}
			if ((newmode&A_BOLD) &&
			    (ceol_standout_glitch || !(oldmode&A_BOLD)))
				if (enter_bold_mode) {
					curmode |= A_BOLD;
					tputs(enter_bold_mode, 1, outc);
				}
			if ((newmode&A_DIM) &&
			    (ceol_standout_glitch || !(oldmode&A_DIM)))
				if (enter_dim_mode) {
					curmode |= A_DIM;
					tputs(enter_dim_mode, 1, outc);
				}
			if ((newmode&A_BLINK) &&
			    (ceol_standout_glitch || !(oldmode&A_BLINK)))
				if (enter_blink_mode) {
					curmode |= A_BLINK;
					tputs(enter_blink_mode, 1, outc);
				}
			if ((newmode&A_REVERSE) &&
			    (ceol_standout_glitch || !(oldmode&A_REVERSE)))
				if (enter_reverse_mode) {
					curmode |= A_REVERSE;
					tputs(enter_reverse_mode, 1, outc);
				}
			if ((newmode&A_UNDERLINE) &&
			    (ceol_standout_glitch || !(oldmode&A_UNDERLINE)))
				if (enter_underline_mode) {
					curmode |= A_UNDERLINE;
					tputs(enter_underline_mode,1,outc);
				}
			if ((newmode&A_STANDOUT) &&
			    (ceol_standout_glitch || !(oldmode&A_STANDOUT)))
				if (enter_standout_mode) {
					curmode |= A_STANDOUT;
					tputs(enter_standout_mode,1,outc);
				}
		}
	} else {
		if (oldmode & A_ALTCHARSET && exit_alt_charset_mode)
			tputs(exit_alt_charset_mode, 1, outc);
		if (exit_attribute_mode)
			tputs(exit_attribute_mode, 1, outc);
		else if (oldmode == A_UNDERLINE && exit_underline_mode)
			tputs(exit_underline_mode, 1, outc);
		else if (exit_standout_mode)
			tputs(exit_standout_mode, 1, outc);
		curmode = 0;
	}
	/*
	 * If we asked for bold, say, on a terminal with only standout,
	 * and we aren't already in standout, we settle for standout.
	 */
	if ( (newmode&hilite) && curmode != newmode &&
	     (curmode&A_STANDOUT == 0 || ceol_standout_glitch ) ) {
		tputs(enter_standout_mode, 1, outc);
		curmode |= newmode&hilite;
	}
	oldmode = curmode;
}

/*
 * This routine is used for magic cookie terminals to set the
 * current attribute setting without actually transmitting any
 * characters.  Needed for cases where a cookie has been left
 * lying around on the old line.
 */
_fake_vidputs(newmode)
int newmode;
{

	SP->phys_gr = oldmode = newmode;
}
