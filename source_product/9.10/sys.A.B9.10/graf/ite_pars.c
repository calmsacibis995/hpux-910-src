/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_pars.c,v $
 * $Revision: 1.8.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:59:41 $
 */

/* HPUX_ID: @(#)ite_pars.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../h/param.h"
#include "../h/tty.h"
#include "../h/conf.h"
#include "../graf/kbd.h"
#include "../graf/ite.h"
#include "../graf/ite_scroll.h"

extern char *aids_label[];
extern char *modes_label[];
extern char *user_key[];
extern char *user_label[];
extern char *user_defaults[][2];
extern char user_key_type[];
extern struct k_keystate k_keystate;

#define	BEGIN	0
#define	SESCAPE	1
#define	AMPERSIGN	2
#define	STAR	3
#define	CURSOR	4
#define	SENHANCE	5
#define	DEFIN	6
#define	FUNCS	7
#define	SCONFIG	8
#define	SEND	9
#define	CPAIR	10
#define	WINDOW	11
#define	RASTER	12
#define	IDENT	13
#define	CONFIG2	14
#define	SEND2	15
#define	CPR1	16
#define	CPR2	17
#define	CURSOR2	18
#define	CURSOR3	19
#define	CURSOR4	20
#define	WINDOW2	21
#define	WINDOW3	22
#define	WINDOW4	23
#define	DEFIN1	24
#define	DEFIN2	25
#define	EATIT	27
#define	LABEL	28
#define	LABEL2	29
#define	CPR3	30
#define	SCREEN	31
#define	SCREEN1	32

ite_pars(c)
register unsigned c;
{
	register struct iterminal *ite = &iterminal;

	switch (ite->pstate) {
	case BEGIN:
		switch (c) {
		case 0x07:
			kbd_beep();return;
		case 0x08:
			scroller(SCR_BACKSPACE);return;
		case 0x09:
			scroller(TAB_DIR,FORWARD);return;
		case 0x0a:
			scroller(LF_CURSOR_DOWN);return;
		case 0x0d:	/* carriage return */
			scroller(SET_CURSOR,0,ite->ypos);return;
		case 0x1b:
			ite->pstate = SESCAPE;return;
		case 0x1c:
			scroller(CURSOR_RIGHT);return;
		case 0x1f:
			scroller(UP_ARROW);return;
		default:
			scr_char_out(c);return;
		}

	case SESCAPE:
		switch (c) {
		case '`':	/* Relative Cursor sense */
			ite->pstate = BEGIN;
			scroller(REL_SENSE);
			goto lex_exit;
		case '0':	/* Copy memory to destinations */
			scr_char_out((ushort)0x1b);goto lex_exit;
		case '1':	/* Set tab */
			scroller(SET_TAB);goto lex_exit;
		case '2':	/* Clear tab */
			scroller(CLEAR_TAB);goto lex_exit;
		case '3':	/* Clear all tabs */
			scroller(CLEAR_ALL_TABS);goto lex_exit;
		case 'A':	/* Cursor up */
			scroller(UP_ARROW);goto lex_exit;
		case 'B':	/* Cursor down */
			scroller(DOWN_ARROW);goto lex_exit;
		case 'C':	/* Cursor right */
			scroller(CURSOR_RIGHT);goto lex_exit;
		case 'D':	/* Cursor left */
			scroller(CURSOR_LEFT);goto lex_exit;
		case 'E':	/* Hard reset (power on) */
			scroller(SCR_HARD_RESET);goto lex_exit;
		case 'F':	/* Cursor home down */
			scroller(HOME_DOWN);goto lex_exit;
		case 'G':	/* Move cursor to left margin */
			scroller(SET_CURSOR,0, ite->ypos);goto lex_exit;
		case 'H':	/* Cursor home up */
			scroller(HOME_UP);goto lex_exit;
		case 'I':	/* Horizontal tab */
			scroller(TAB_DIR,FORWARD);goto lex_exit;
		case 'J':	/* Clear to end of memory */
			scroller(CLEAR_SCREEN);goto lex_exit;
		case 'K':	/* Clear to end of line */
			scroller(CLEAR_LINE);goto lex_exit;
		case 'L':	/* Insert line */
			scroller(INSERT_LINE,ite->ypos);
			scroller(SET_CURSOR,0, ite->ypos);goto lex_exit;
		case 'M':	/* Delete line */
			scroller(DELETE_LINE,ite->ypos);
			scroller(SET_CURSOR,0, ite->ypos);goto lex_exit;
		case 'P':	/* Delete character */
			scroller(DELETE);goto lex_exit;
		case 'Q':	/* Start insert character mode */
			scroller(INSERT_MODE,TRUE);goto lex_exit;
		case 'R':	/* End insert character mode */
			scroller(INSERT_MODE,FALSE);goto lex_exit;
		case 'S':	/* Roll up */
			scroller(ROLL_UP);goto lex_exit;
		case 'T':	/* Roll down */
			scroller(ROLL_DOWN);goto lex_exit;
		case 'U':	/* Next page */
			scroller(NEXT_PAGE);goto lex_exit;
		case 'V':	/* Previous page */
			scroller(PREV_PAGE);goto lex_exit;
		case 'Y':	/* Display functions on */
			scroller(DISP_FUNCS, TRUE);goto lex_exit;
		case 'Z':	/* Display functions off */
			scroller(DISP_FUNCS, FALSE);goto lex_exit;
		case 'a':	/* Absolute Cursor sense */
			ite->pstate = BEGIN;
			scroller(ABS_SENSE);
			goto lex_exit;
		case 'd':	/* enter */
			ite->pstate = BEGIN;	/* Finished with the sequence */
			send_current_line(ite);
			goto lex_exit;
		case 'g':	/* Soft reset */
			scroller(SCR_SOFT_RESET);goto lex_exit;
		case 'h':	/* Cursor home up */
			scroller(HOME_UP);goto lex_exit;
		case 'i':	/* Back tab */
			scroller(TAB_DIR,BACKWARD);goto lex_exit;
		case '&': ite->pstate = AMPERSIGN; return;
		case '*': ite->pstate = STAR; return;

		case '(':
		case ')': goto eatit;		/* These begin long sequences */

		default: goto lex_exit;		/* Must be a short sequence */
		} 

	case AMPERSIGN:
		switch (c) {
		case 'a': ite->pstate = CURSOR;return;
		case 'd': ite->pstate = SENHANCE;return;
		case 'f': ite->pstate = DEFIN;return;
		case 'j': ite->pstate = FUNCS;return;
		case 'k': ite->pstate = SCONFIG;return;
		case 's': ite->pstate = SEND;return;
		case 'v': ite->pstate = CPAIR;
			ite->redraw = ite->color_a = ite->color_b = ite->color_c = ite->color_x = ite->color_y = ite->color_z = 0;
			return;
		case 'w': ite->pstate = WINDOW;return;
		default: goto eatit;
		}

	case STAR:
		switch (c) {
		case 'd': ite->pstate = RASTER;return;
		case 's': ite->pstate = IDENT;return;
		default: goto eatit;
		}

	/*    <esc>&a   ?     */
	case CURSOR:
		ite->pstate = CURSOR2;
		/* set defaults to present position */
		ite->ScreenRelative = SET_CURSOR;
		ite->row = ite->ypos;
		ite->column = ite->xpos;

	case CURSOR2: /* + or - or number expected */
		ite->negate = ite->num = ite->cur_rel = 0;
		ite->pstate = CURSOR3;
		switch (c) {
		case '-': 
			ite->negate = 1;
		case '+': 
			ite->cur_rel = 1;
			return;
		}
	case CURSOR3: /*  number expected */
		switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ite->num *= 10; ite->num += c - '0';
		case ' ':
			return;
		}
	case CURSOR4: /* terminator expected */
		ite->num &= 0x7fff;
		if (ite->negate) ite->num = -(ite->num);
		switch (c) {
		case 'Y':	/* Screen relative row addressing */
		case 'y':
			ite->row = ite->num;
			ite->ScreenRelative = SET_CURSOR;
			/* check if cursor relative */
			if (ite->cur_rel)
				ite->row += ite->ypos;
			break;
		case 'C':	/* Screen relative column addressing */
		case 'c':
			ite->column = ite->num;
			/* check if cursor relative */
			if (ite->cur_rel)
				ite->column += ite->xpos;
			break;
		case 'R':	/* Absolute row addressing*/
		case 'r':
			ite->row = ite->num;
			ite->ScreenRelative = ABS_CURSOR;
			/* check if cursor relative */
			if (ite->cur_rel)
				ite->row += ite->ypos + ite->first_line - 1;
			break;
		default: goto eatit;
		}
		/* if not caps letter continue */
		if (c > 'Z') {
			ite->pstate = CURSOR2;
			return;
		} else { /* set new cursor position */
			scroller(ite->ScreenRelative, ite->column, ite->row);
			goto lex_exit;
		}

	case SENHANCE:	/* Set display enhancements */
		if (c=='@')	/* end enhancement */
			scroller(ENHANCE,0);
		else if (c >= 'A' && c <= 'O')
			scroller(ENHANCE,(c-'A'+1));
		else 
			goto eatit;
		goto lex_exit;

	case DEFIN:	/* <esc>&f  ? Soft Key Definitions */
		ite->key = ite->ll = ite->llen = ite->strsize =
		    ite->attr = ite->negate = ite->num = 0;
		ite->sl = 1;

	case DEFIN1: /* + or - or number expected */
		ite->negate = ite->num = 0;
		ite->pstate = DEFIN2;
		switch (c) {
		case '-': 
			ite->negate = 1;
		case '+': 
			return;
		}
	case DEFIN2: /*  number expected */
		switch (c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ite->num *= 10; ite->num += c - '0';
		case ' ':
			return;
		default:
			if (ite->negate) 
				ite->num = -(ite->num);
		}
		switch (c) {
		case 'a':	/* Get attribute */
		case 'A':
			ite->attr = ite->num;
			if ((ite->attr < 0) || (ite->attr > 2))
				goto eatit;
			break;

		case 'k':	/* Get key number */
		case 'K':
			ite->key = ite->num - 1;
			if ((ite->key < 0) || (ite->key > 7))
				goto eatit;
			break;

		case 'd':	/* Get Label length, ll */
		case 'D':
			ite->ll = ite->num;
			if ((ite->ll < 0) || (ite->ll > 16))
				goto eatit;
			break;

		case 'l':	/* Get Definition length, sl */
		case 'L':
			ite->sl = ite->num;
			if ((ite->sl < -1) || (ite->sl > 80))
				goto eatit;
			break;

		default: goto eatit;
		}
		/* continue if not cap letter */
		if (c > 'Z') {
			ite->pstate = DEFIN1;
			return; 
		}
		/* Set key attribute */
		switch(ite->attr) {
		case 0:
			user_key_type[ite->key] = 'N';
			break;
		case 1:
			user_key_type[ite->key] = 'L';
			break;
		case 2:
			user_key_type[ite->key] = 'T';
			break;
		}
		if (ite->sl == -1) 
			strcpy(user_key[ite->key], "\0");
		if (ite->ll > 0) {
			ite->pstate = LABEL;
			return;
		}
		if (ite->sl > 0) {
			ite->pstate = LABEL2;
			return;
		}
		goto lex_exit;

	case LABEL: /* Fetch Label Definition. */
		/* set the label buffer */
		if (ite->llen < LABEL_SIZE)
			ite->labelbuf[ite->llen++] = c;
		else ite->llen++;

		if (ite->llen >= ite->ll) {
			while (ite->llen<LABEL_SIZE)
				ite->labelbuf[ite->llen++]=' ';
			strncpy(user_label[ite->key],ite->labelbuf,LABEL_SIZE);
			if ((ite->flags & SFK_ON) && ite->key_state==USER)
				print_label(user_label[ite->key], ite->key);
			if (ite->sl > 0) {
				ite->pstate = LABEL2;
				return;
			}
			goto lex_exit;
		}
		return;

	case LABEL2: /* Fetch Key Definition and set it */
		ite->strbuf[ite->strsize++] = c;
		if (ite->strsize >= ite->sl) {
			ite->strbuf[ite->strsize] = '\0';
			strcpy(user_key[ite->key], ite->strbuf);
			goto lex_exit;
		} 
		return;

	case FUNCS:
		switch (c) {
		case '@':	/* disable function keys and
				** remove all key labels from
				** the screen.  
				** if in remote mode then the
				** fkeys are still transmitted
				*/
				scroller(DSP_LABELS, CLEAR, 0);
				break;
		case 'A':	/* enable mode selection keys */
				scroller(DSP_LABELS, MODES, 0);
				break;
		case 'B':	/* enable user keys */
				scroller(DSP_LABELS, USER, 1);
				break;
		case 'C':
		case 'R':
		case 'S':	/* remove labels from the screen
				** and display <message>
				*/
				break;
		default: goto eatit;
		}
		goto lex_exit;
	case SCONFIG:
		switch (c) {
		case '0': ite->pint1 = 0; ite->pstate = CONFIG2; break;
		case '1': ite->pint1 = 1; ite->pstate = CONFIG2; break;
		default: goto eatit;
		}
		break;
	case CONFIG2:
		switch (c) {
		case 'A':	/* Auto lf */
			scroller(AUTOLF,(ite->pint1 ? TRUE : FALSE));
			break;
		case 'C':	/* Caps lock */
			if (ite->pint1) k_keystate.flags |= K_ASR33TTY;
			else k_keystate.flags &= ~K_ASR33TTY;
			break;

		case 'I':	/* Ascii 8 bit */
			{
				int oldflags;

				oldflags = k_keystate.flags;
				if (ite->pint1)
					k_keystate.flags |= K_ASCII8;
				else
					k_keystate.flags &= ~K_ASCII8;

				if (oldflags != k_keystate.flags) {
					/* set up char mapping */
					char_switch();
					scroller(REDRAW);  /* repaint screen */
				}
			}
			break;

		case 'L':	/* Local echo */
			ite->local_echo = (ite->pint1 ? TRUE : FALSE);
			break;
		case 'P':	/* Caps mode */
			if (ite->pint1) k_keystate.flags |= K_CAPSLOCK;
			else k_keystate.flags &= ~K_CAPSLOCK;
			set_caps_light(&k_keystate);
			break;
		case 'R':	/* Remote mode */
			scroller(REMOTE, (ite->pint1 ? TRUE : FALSE));
			break;
		default: goto eatit;
		}
		goto lex_exit;
	case SEND:
		switch (c) {
		case '0': ite->pint1 = 0; ite->pstate = SEND2; return;
		case '1': ite->pint1 = 1; ite->pstate = SEND2; return;
		default: goto eatit;
		}
	case SEND2:
		switch (c) {
		case 'A':	/* Transmit Funcitions */
			ite->xmit_fnctn = (ite->pint1 ? TRUE : FALSE);
			break;
		case 'C':	/* InhEolWrp */
			ite->inheolwrp = (ite->pint1 ? TRUE : FALSE);
			break;
		default: goto eatit;
		}
		goto lex_exit;

	case CPAIR:  	/* Color Pair command */
		if (c == ' ') /* eat leading spaces */
			return;
		ite->point = ite->pint1 = 0;
		ite->pstate = CPR1;
		/* fall into */

	case CPR1: /* get numbers before '.' */
		if (c == '.') 
			goto cpr2;
		if (c < '0' || c > '9') {
			ite->pint1 *= 100;
			goto cpr4;
		}
		ite->pint1 *= 10; ite->pint1 += c - '0';
		return;

cpr2:	ite->point++;
	if (ite->pint1 == 1) { /* decimal point found - get numbers after '.' */
		ite->pstate = CPR2;
		ite->pint1 = 100;
		return;
	}
	if (ite->pint1 == 0) {
		ite->pstate = CPR3;
		ite->num = 10;
		return;
	}
	goto eatit; /* only 0 or 1 allowed before '.' */

	case CPR2: /* only '0's allowed after 1.xxx  */
		if (c == '0')
			return;
		goto cpr4;

	case CPR3: /* accumulate number after '.' from 0 - 99 */
		if (c < '0' || c > '9') 
			goto cpr4;
		ite->pint1 += (c - '0') * ite->num;
		ite->num /= 10;
		return;
		
cpr4: /* the end of number sequence - must be termination letter */
		if (ite->pint1 < 0)
			goto eatit;
		ite->num = ite->pint1 / 100;
		switch (c) {
		case 'S':	/* Select a color pair */
		case 's':
			if (ite->point || (ite->num>7)) 
				goto eatit;
			scroller(COLORPAIR, ite->num);
			if (c == 'S') {
				if (ite->redraw) scroller(REDRAW);
				goto lex_exit;
			}
			break;
		case 'a': ite->color_a = ite->pint1; break;
		case 'b': ite->color_b = ite->pint1; break;
		case 'c': ite->color_c = ite->pint1; break;
		case 'x': ite->color_x = ite->pint1; break;
		case 'y': ite->color_y = ite->pint1; break;
		case 'z': ite->color_z = ite->pint1; break;
		case 'I':	/* Set a color pair */
		case 'i':	/* Set a color pair */
			if ((ite->num>7) || ite->point)
				goto eatit;
			set_color_pair(ite, ite->num);
			ite->color_a = ite->color_b = ite->color_c =
			    ite->color_x = ite->color_y = ite->color_z = 0;
			if (c=='I') {
				if (ite->redraw) scroller(REDRAW);
				goto lex_exit;
			}
			break;

		case 'M':	/* Select a color method */
		case 'm':
			if (ite->point) goto eatit;
			if (ite->num == 0)	ite->c_mode = RGB;
			else if (ite->num == 1)	ite->c_mode = HSL;
			else goto eatit;
			if (c=='M') {
				if (ite->redraw) scroller(REDRAW);
				goto lex_exit;
			}
			break;
		case 'P':	/* Set plane mask */
		case 'p':
			scroller(PLANE_MASK, ite->num);
			ite->redraw = 1;
			if (c=='P') {
				scroller(REDRAW);
				goto lex_exit;
			}
			break;
		default: goto eatit;
		}
		ite->pstate = CPAIR;
		return;

	case WINDOW:
		if (c == '1') {
			ite->pstate = WINDOW2;
			return;
		}
		if (c == '2') {
			ite->pstate = SCREEN;
			return;
		}
		goto eatit;
	case WINDOW2:
		if (c =='2') {
			ite->pstate = WINDOW3;
			return;
		}
		if (c == '3') {
			ite->pstate = WINDOW4;
			return;
		}
		goto eatit;
	case WINDOW3:
		if (c=='F') {	/* display window on (top 24 lines)*/
			alpha_on();
			goto lex_exit;
		}
		goto eatit;
	case WINDOW4:
		if (c=='F') {	/* display window off (top 24 lines) */
			alpha_off();
			goto lex_exit;
		}
		goto eatit;

	case SCREEN:
		if (c =='f') {
			ite->pint1 = 0;
			ite->pstate = SCREEN1;
			return;
		}
		goto eatit;
	case SCREEN1:
		if (c == ' ') break;
		if (c >= '0' && c <= '9') {
			ite->pint1 *= 10;
			ite->pint1 += c - '0';
			return;
		}
		if (c == 'U') {
			scroller(SCREEN_START, ite->pint1);
			goto lex_exit;
		}
		goto eatit;

	case RASTER:	/* Raster control escape sequences */
		switch (c) {
		case 'a':
		case 'A':
			scroller(GRAPHICS_CLEAR);
			break;

		case 'c':
		case 'C':
			graphics_on();
			break;

		case 'd':
		case 'D':
			graphics_off();
			break;

		case 'e':
		case 'E':
			alpha_on();
			break;

		case 'f':
		case 'F':
			scroller(ALPHA_OFF_FULL);
			break;

		case 'k':
		case 'K':
			scroller(G_CURSOR_ON);
			break;

		case 'l':
		case 'L':
			scroller(G_CURSOR_OFF);
			break;

		case 'Q':
		case 'q':
			scroller(CURSOR_ON);
			break;

		case 'R':
		case 'r':
			scroller(CURSOR_OFF);
			break;

		default: goto eatit;
		}
		/* if upper case, terminate sequence */
		if (c <= 'Z')
			goto lex_exit;
		return;

	case IDENT:	/* Read device I.D. */
		if (c=='1') break;
		if (c!='^') 
			goto eatit;
		ite->pstate = BEGIN;	/* Finished with the sequence */
		ite_self_id();
		goto lex_exit;
eatit:
	case EATIT: 	/* Terminate Eat mode if a valid terminate
			** character happens or a character that
			** must be recognized.
			*/
			ite->pstate = EATIT;
			if (c==0x1B) {
				ite->pstate = SESCAPE;
				return;
			} 
			if (c <= 0x1f || (c >= '@' && c <= '_')) {
lex_exit:
				/* Terminate on Capitals & @[\]^_*/
				ite->pstate = BEGIN;
			}
		return;
	}
}
