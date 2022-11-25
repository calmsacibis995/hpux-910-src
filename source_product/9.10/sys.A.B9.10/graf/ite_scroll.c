/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_scroll.c,v $
 * $Revision: 1.6.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 18:59:52 $
 */

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
#include "../h/file.h"
#include "../h/conf.h"
#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_scroll.h"
#include "../graf/kbd.h"
#include "../mach.300/cpu.h"

/* semaphore has screen locked */
char ite_bmap_semi_lock = 0;

extern int kbd_mute_char(), kbd_language();
extern struct k_keystate k_keystate;
extern unsigned short mute_enable[];
extern char sysflags;

/* SOFTKEY definitions */

#define	HP_CHAR	223

char *aids_label1[NUMBER_OF_SFKS] = {
	"                ",
	"tab/mrgn        ",
	"                ",	/* "service ", */
	" modes          ",
	"FlexDisc        ",
	"                ",
	"                ",
	"config          ",
};

char *aids_label2[NUMBER_OF_SFKS] = {
	"                ",
	"margins/tabs/col",
	"                ",	/* "service ", */
	" modes          ",
	"FlexDisc        ",
	"                ",
	"                ",
	" config   keys  ",
};

char *modes_label1[NUMBER_OF_SFKS] = {
	"                ",	/* "LN MODE ", */
	"                ",	/* "MDFY AL ", */
	"                ",	/* "BLOCK   ", */
	"REMOTE *        ",
	"                ",	/* " T TEST ", */
	"                ",	/* "MEM LCK ", */
	"DSPY FN         ",
	"AUTO LF         ",
};

char *modes_label2[NUMBER_OF_SFKS] = {
	"                ",	/* "LN MODE ", */
	"                ",	/* "MDFY AL ", */
	"                ",	/* "BLOCK   ", */
	" REMOTE   MODE *",
	"                ",	/* " T TEST ", */
	"                ",	/* "MEM LCK ", */
	"DISPLAY FUNCTNS ",
	"  AUTO     LF   ",
};

char *marg_label1[NUMBER_OF_SFKS] = {
	"                ",	/* "STRT COL", */
	"SET TAB         ",
	"CLR TAB         ",
	"CLR TABS        ",
	"                ",	/* "L MARGIN", */
	"                ",	/* "R MARGIN", */
	"                ",	/* "CLR MRGN", */
	"                ",
};

char *marg_label2[NUMBER_OF_SFKS] = {
	"                ",	/* "STRT COL", */
	"  SET     TAB   ",
	" CLEAR    TAB   ",
	"CLR ALL   TABS  ",
	"                ",	/* "L MARGIN", */
	"                ",	/* "R MARGIN", */
	"                ",	/* "CLR MRGN", */
	"                ",
};

char *conf_label1[NUMBER_OF_SFKS] = {
	"                ",
	"                ",
	"                ",	/* "datacomm", */
	"                ",
	"terminal        ",
	"                ",
	"                ",
	"                ",
};

char *conf_label2[NUMBER_OF_SFKS] = {
	"                ",
	"                ",
	"                ",	/* "datacomm", */
	"                ",
	"terminal config ",
	"                ",
	"                ",
	"                ",
};

char *serv_label1[NUMBER_OF_SFKS] = {
	"                ",
	"                ",
	"                ",
	"                ",
	" T TEST         ",
	"ID ROMS         ",
	"DC TEST         ",
	"                ",
};

char *serv_label2[NUMBER_OF_SFKS] = {
	"                ",
	"                ",
	"                ",
	"                ",
	"TERMINAL  TEST  ",
	"IDENTIFY  ROMS  ",
	"DATACOMM  TEST  ",
	"                ",
};

char *tconf_label1[NUMBER_OF_SFKS] = {
	"SAVE CFG        ",
	"  NEXT          ",
	"PREVIOUS        ",
	"DEFAULT         ",
	"                ",
	"                ",
	"DSPY FN         ",
	"config          ",
};

char *tconf_label2[NUMBER_OF_SFKS] = {
	"  SAVE   CONFIG ",
	"  NEXT   CHOICE ",
	"PREVIOUS CHOICE ",
	"DEFAULT  VALUES ",
	"                ",
	"                ",
	"DISPLAY FUNCTNS ",
	" config   keys  ",
};

char *suser_label1[NUMBER_OF_SFKS] = {
	"                ",
	"  NEXT          ",
	"PREVIOUS        ",
	"DEFAULT         ",
	"                ",
	"                ",
	"DSPY FN         ",
	"                ",
};

char *suser_label2[NUMBER_OF_SFKS] = {
	"                ",
	"  NEXT   CHOICE ",
	"PREVIOUS CHOICE ",
	"DEFAULT  VALUES ",
	"                ",
	"                ",
	"DISPLAY FUNCTNS ",
	"                ",
};

char *drive_label1[NUMBER_OF_SFKS] = {
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
};

char *drive_label2[NUMBER_OF_SFKS] = {
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
};

char *user_defaults[NUMBER_OF_SFKS][2] = {
	{ "   f1           ",  "\033p"}, { "   f2           ",  "\033q"},
	{ "   f3           ",  "\033r"}, { "   f4           ",  "\033s"},
	{ "   f5           ",  "\033t"}, { "   f6           ",  "\033u"},
	{ "   f7           ",  "\033v"}, { "   f8           ",  "\033w"},
};

char **aids_label, **modes_label, **marg_label,  **conf_label,
     **serv_label, **tconf_label, **suser_label, **drive_label;

set_sfkdefaults(nlines)
    int nlines;
{
    if (nlines == 1) {
	aids_label  = aids_label1;
	modes_label = modes_label1;
	marg_label  = marg_label1;
	conf_label  = conf_label1;
	serv_label  = serv_label1;
	tconf_label = tconf_label1;
	suser_label = suser_label1;
	drive_label = drive_label1;
    }
    else {
	aids_label  = aids_label2;
	modes_label = modes_label2;
	marg_label  = marg_label2;
	conf_label  = conf_label2;
	serv_label  = serv_label2;
	tconf_label = tconf_label2;
	suser_label = suser_label2;
	drive_label = drive_label2;
    }
}

unsigned char user_key_1[81], user_key_2[81];	 /* +1 for string terminator */
unsigned char user_key_3[81], user_key_4[81];
unsigned char user_key_5[81], user_key_6[81];
unsigned char user_key_7[81], user_key_8[81];

unsigned char *user_key[NUMBER_OF_SFKS] = {
	user_key_1, user_key_2,
	user_key_3, user_key_4,
	user_key_5, user_key_6,
	user_key_7, user_key_8,
};

unsigned char user_label_1[LABEL_SIZE+1]; 	 /* +1 for string terminator */
unsigned char user_label_2[LABEL_SIZE+1];
unsigned char user_label_3[LABEL_SIZE+1];
unsigned char user_label_4[LABEL_SIZE+1];
unsigned char user_label_5[LABEL_SIZE+1];
unsigned char user_label_6[LABEL_SIZE+1];
unsigned char user_label_7[LABEL_SIZE+1];
unsigned char user_label_8[LABEL_SIZE+1];

unsigned char *user_label[NUMBER_OF_SFKS] = {
	user_label_1, user_label_2,
	user_label_3, user_label_4,
	user_label_5, user_label_6,
	user_label_7, user_label_8,
};

/* User Key configuraton Screen */
#define NUMBER_OF_UFORMS 24

#define	DISPLAY	0	/* Must be no bits set */
#define NEXT	1
#define	PREV	2
#define	DEFAULT	4

#define scroll_inc_line(line) if (++(line) >= ite->scroll_lines) \
					(line) = 0
#define scroll_dec_line(line) if (--(line) < 0) \
					(line) = (ite->scroll_lines - 1)

#define crt_xy_to_scroll(x, y) (ite->scroll_buf + (ite->screenwidth * (scroll_add_line(ite->start_line,(y)))+(x)))

char user_key_type[NUMBER_OF_SFKS];

/* Terminal Configuration Screen */
#define NUMBER_OF_TFORMS 16
struct tform {
	char	ly,lx;		/* Screen x,y for start of label */
	char	*label;		/* Field Label */
	char	fy,fx;		/* Screen x,y for start of field */
	char	skip;		/* 1==skip this one */
} tform[NUMBER_OF_TFORMS] = {
	{	0,	27,	"TERMINAL CONFIGURATION",	0,	0, 0,},
	{	2,	42,	"FrameRate",	2,	52, 1,},
	{	3,	5,	"Language",	3,	14, 0,},
	{	4,	4,	"ReturnDef",	4,	14, 0,},
	{	6,	4,	"LocalEcho",	6,	14, 0,},
	{	6,	23,	"CapsLock",	6,	32, 0,},
	{	6,	42,	"Start Col",	6,	52, 1,},
	{	6,	59,	"ASCII 8 Bits",	6,	72, 0,},
	{	7,	1,	"XmitFnctn(A)",	7,	14, 0,},
	{	7,	24,	"SPOW(B)",	7,	32, 1,},
	{	7,	39,	"InhEolWrp(C)",	7,	52, 0,},
	{	7,	59,	"Line/Page(D)",	7,	72, 1,},
	{	8,	1,	"InhHndShk(G)",	8,	14, 1,},
	{	8,	21,	"Inh DC2(H)",	8,	32, 1,},
	{	10,	1,	"FldSeparator",	10,	14, 1,},
	{	10,	19,	"BlkTermnator",	10,	32, 1,},
};


/* Table to convert internal KATAKANA characters to displayable screen values
*/
unsigned char kat1[63] = {
	129,130,131,132,133,134,135,		/* A1-A7 */
	136,137,138,139,140,141,142,143,	/* A8-AF */
	144,145,146,147,148,149,150,151,	/* B0-B7 */
	152,153,154,155,156,157,158,159,	/* B8-BF */
	160,161,162,163,164,165,166,167,	/* C0-C7 */
	173,174,177,178,180,188,190,191,	/* C8-CF */
	224,225,226,227,228,229,230,231,	/* D0-D7 */
	232,233,234,235,236,237,238,179,	/* D8-DF */
};

/* Table to convert internal Roman-Extended characters to displayable screen values
*/
unsigned char rom1[] = {
	'A','A','E','E','E','I','I',			/* A1-A7 */
	168,169,170,171,172,'U','U',175,		/* A8-AF */
	176,HP_CHAR,HP_CHAR,179,'C',181,182,183,	/* B0-B7 */
	184,185,186,187,128,189,HP_CHAR,HP_CHAR,	/* B8-BF */
	192,193,194,195,196,197,198,199,		/* C0-C7 */
	200,201,202,203,204,205,206,207,		/* C8-CF */
	208,209,210,211,212,213,214,215,		/* D0-D7 */
	216,217,218,219,220,221,222,'O',		/* D8-DF */
	'A','A','a','D','d','I','I','O',		/* E0-E7 */
	'O','O','o','S','s','U','Y','y',		/* E8-EF */
};

/*
 * All CRT requests must come thru this procedure.  This ensures
 * that only on event happens at a time and that events happen
 * sequentially.
*/
scroll_add_line(line, amount)
{
    struct iterminal *ite = &iterminal;

    if ((amount += line) >= ite->scroll_lines)
	amount -= ite->scroll_lines;
    return(amount);
}

/*
 * Restore display from the min unoutputted position.
 */
restore_missing_scn(ite)
    register struct iterminal *ite;
{
	if (ite->missed_output == 0)
		return;

	scr_restore_dsp(ite, ite->rxpos, ite->rypos);

	ite->rxpos = ite->screenwidth;
	ite->rypos = ite->screenheight;
	ite->missed_output = 0;

	/* put back cursor */
	if (ite->cursor_on == FALSE)
		scr_cursor(ite->xpos, ite->ypos);
}

/*************************************************************************
** Scroller Command Interpreter:					**
**									**
** scroller_init(type)		Reset for Scroller.			**
** scroller(action,parm1,parm2,parm3,parm4)				**
*************************************************************************/

scroller_init(type)
{
	register struct iterminal *ite = &iterminal;
	register i;

	/* Initialize the Scroller Command Pending Queue to Empty
	** and not busy.
	*/
	/* set alpha on & graphics on*/
	ite->flags |= ALPHAON|GRAPHICSON;
	/* clear the semophore */
	ite_bmap_semi_lock = 0;
	ite->missed_output = 0;

	if (type == HARD) {
		/* Initialize Scroller Memory to Empty.
		** With Space reserved for one CRT screen image.
		*/
		ite->bottom_line = ite->start_line = ite->top_line=0;

		/* restore number of planes */
		ite->cur_planes = ite->plane_mask;

		/* Initialize color pairs */
		for (i = 0; i < 8; i++) {
			/* if no color map, just use black & white */
			ite->color_pairs[i] = init_colors[(ite->flags&ITE_COLOR)?i:0];
		}
		/* special kludge to invert color pair 7 if no color top */
		if (!(ite->flags & ITE_COLOR)) {
			ite->color_pairs[7].FG = ite->color_pairs[0].BG;
			ite->color_pairs[7].BG = ite->color_pairs[0].FG;
		}
		ite->c_mode = RGB;			/* RGB Color Method */

		/* do the power on reset stuff */
		(*ite->crt_pwr_reset)(ite);

		if (ite->flags & BIT_MAPPED) {
			/* deallocate scroll buffer if allocated */
			if (ite->scroll_buf != ite->scroll_buf_rom) {
				sys_memfree(ite->scroll_buf, ite->scroll_lines *
					ite->screenwidth * sizeof(SCROLL));
			}
			/* now get the font from rom */
			if (!(ite->flags & STI))
				ite_get_fontrom(ite);

			/* now calculate screen parameters */
			ite_calc_screen(ite);

			/* restore the hardware color maps */
			(*ite->crt_reset)(ite);
		}
		/* allocate scroll memory if needed */
 		if (ite->scroll_buf_rom == NULL)
 			ite->scroll_buf_rom = (SCROLL *)sys_memall(ite->screenwidth *
				ite->scroll_lines_boot * sizeof(SCROLL));

		/* now set the default scroll buffer from bootstrap */
		ite->scroll_lines = ite->scroll_lines_boot;
		ite->scroll_buf = ite->scroll_buf_rom;

		/* Clear the screen. */
		ite->crt_stop_line = 0;
		(*ite->crt_clear)(ite, 0, 0, ite->screensize);
		ite->cursor_on = FALSE;
		ite->first_line = 1;
		ite->last_line = 1;
		ite->flds = '\037';	/* US is default Field Separator */
		ite->blkt = '\036';	/* RS is default Block Terminator */
		ite->retdef[0] = '\015'; /* CR by itself is default RETURN key */
		ite->retdef[1] = ' ';	/* CR by itself is default RETURN key */

		/* set up default soft keys */
 		for (i = 0; i < NUMBER_OF_SFKS; i++)
 			init_softkey(i, user_defaults[i][0], user_defaults[i][1]);
		ite->key_state = MODES;
 		scroller(DSP_LABELS, CLEAR, 0);
		scroller(SET_CURSOR, 0, 0);	/* Set x,y to 0,0 */

	} else { /* SOFT */
		if ((ite->key_state!=TCONFIG) && (ite->key_state!=SET_USER)) {
			/* restore the hardware color maps */
			(*ite->crt_reset)(ite);
			/*
			 *  We clear the whole screen here in case some
			 *  application has left garbage in any portion of the
			 *  screen which scr_restore_dsp() will not clear.
			 */
			(*ite->crt_block_clear)(ite, 0, 0,
			        ite->framedsp_height, ite->framedsp_width);

			scr_restore_dsp(ite, 0, 0);
			if (ite->flags & SFK_ON)
				scr_dsp_labels(ite->key_state, FORCE);
			scroller(SET_CURSOR, ite->xpos, ite->ypos);
		}
	}
}

/*VARARGS1*/
scroller(action,parm1,parm2)
{
	register struct iterminal *ite = &iterminal;
	register struct tty *tp = ite->ite_tty;
	register x;
	register unsigned i,j;

	/* Filter actions here when in configure mode or in user
	** key define
	*/
	if ((ite->key_state == TCONFIG) || (ite->key_state == SET_USER)) {
		switch (action){
		case SFK_INPUT:
		case TAB_DIR:
		case DSP_LABELS:
		case CLEAR_LINE:
		case HOME_UP:
		case SET_CRT_START:
		case DISP_FUNCS:
		case LEFTDRIVE:
		case RIGHTDRIVE:
		case SCR_HARD_RESET:
		case SCR_SOFT_RESET:
		case CURSOR_LEFT:
		case SCR_BACKSPACE:
		case CURSOR_RIGHT:
		case DELETE:
		case INSERT_MODE:
		case SET_CURSOR:
			if (ite->cursor_on) { 	/* undo cursor if necessary */
				if (ite->key_state==TCONFIG ||
				    ite->key_state==SET_USER)
					toggle_config_cursor(ite);
			}
			break;

		default:
			kbd_beep();
			return;
		}
	} else {
		/* undo cursor */
		if (ite->cursor_on)
			scr_cursor(ite->xpos, ite->ypos);
	}

	switch (action) {

	case IND_OVERRUN:
		scr_char_out(127);	/* Box character */
		break;

	case ENHANCE:
		/* Change all enhancements from current x,y
		** to next set enhancement or end of line,
		** whichever comes first.
		*/
		scr_enhance(parm1);
		break;

	case COLORPAIR:
		/* Change all colorpairs from current x,y
		** to next set colorpairs or end of line,
		** whichever comes first.
		*/
		scr_colorpair(parm1);
		break;


	case ABS_SENSE:
		ite_flush_check(13);
		send_escape("\033&a",0);
		send_num(ite->xpos);
		kbd_mute_char((unsigned int)'c');
		send_num(ite->first_line+ite->ypos-1);
		kbd_mute_char((unsigned int)'R');
		kbd_mute_char((unsigned int)CR);
		break;

	case REL_SENSE:
		ite_flush_check(13);
		send_escape("\033&a",0);
		send_num(ite->xpos);
		kbd_mute_char((unsigned int)'c');
		send_num(ite->ypos);
		kbd_mute_char((unsigned int)'Y');
		kbd_mute_char((unsigned int)CR);
		break;

	case SET_CURSOR:
		scr_set_cursor(parm1,parm2);
		break;

	case ABS_CURSOR: if (parm2 > (ite->scroll_lines + ite->screenheight - 2))
		parm2 = ite->scroll_lines + ite->screenheight - 2;
		for (;;) {
			i = ite->first_line-1;
			if (parm2<i)
				scr_line(parm2+1);
			else if (parm2>(i+ite->screenheight-1))
				scr_line(parm2-(ite->screenheight-1)+1);
			scr_set_cursor(parm1,
				(parm2-ite->first_line+1));
			j = ite->first_line-1;
			scr_line_include();
			if ((parm2>=j && parm2<=(j+ite->screenheight-1)) ||
				i==j)
				break;
		}
		break;

	case DELETE_LINE:
		scr_delete_line(parm1);
		break;

	case SCROLL_UP:
		scr_scroll_up();
		break;

	case INSERT_LINE:
		scr_insert_line(parm1);
		break;

	case SCROLL_DOWN:
		scr_scroll_down();
		break;

	case CLEAR_LINE:
		if (ite->key_state == TCONFIG) {
			kbd_beep();
			break;
		}
		if (ite->key_state == SET_USER) {
			j = ite->select;
			switch (j%3) {
			case 0:	/* Type */
				kbd_beep();
				break;
			case 1:	/* Label */
				for (i=ite->sx; i<LABEL_SIZE; i++)
					user_label[j/3][i] = ' ';
				scr_value_uf(j,DISPLAY);
				break;
			case 2: /* Key Definition */
				user_key[j/3][ite->sx] = (char)0;
				scr_value_uf(j,DISPLAY);
				break;
			}
			break;
		}
		scr_alpha_clear(0);
		break;

	case CLEAR_SCREEN:
		scr_alpha_clear(1);
		break;

	case CURSOR_DOWN:
		if (ite->ypos >= ite->maxy)
			scr_scroll_up();
		else
			scr_set_cursor(ite->xpos, ite->ypos+1);
		break;

	case LF_CURSOR_DOWN:
		if (ite->ypos >= ite->maxy) {
			scr_scroll_up();
		} else
			ite->ypos++;
		scr_line_include();
		scr_set_cursor(ite->xpos, ite->ypos);
		break;

	case DOWN_ARROW:
		scr_set_cursor(ite->xpos,
			(ite->ypos >= ite->maxy ? 0 : ite->ypos+1));
		break;

	case CURSOR_UP:
		if (ite->ypos <= 0)
			scr_scroll_down();
		else
			scr_set_cursor(ite->xpos, ite->ypos-1);
		break;

	case UP_ARROW:
		scr_set_cursor(ite->xpos,
			(ite->ypos <= 0 ? ite->maxy : ite->ypos-1));
		break;

	case CURSOR_LEFT:
	case SCR_BACKSPACE:
		if ((ite->key_state == TCONFIG) ||
			(ite->key_state == SET_USER)) {
			conf_left();
			break;
		}
		if (ite->xpos <= 0) {
			switch (action) {
			case CURSOR_LEFT:
				if (ite->ypos <= 0)
					scr_set_cursor(ite->maxx, ite->maxy);
				else
					scr_set_cursor(ite->maxx, ite->ypos-1);
				break;
			case SCR_BACKSPACE:
				scr_set_cursor(0, ite->ypos);
				break;
			}
		} else
			scr_set_cursor(ite->xpos-1, ite->ypos);
		break;

	case CURSOR_RIGHT:
		if ((ite->key_state == TCONFIG) ||
			(ite->key_state == SET_USER)) {
			conf_right();
			break;
		}
		if (ite->xpos >= ite->maxx)
			if (ite->ypos >= ite->maxy)
				scr_set_cursor(0, 0);
			else
				scr_set_cursor(0, ite->ypos+1);
		else
			scr_set_cursor(ite->xpos+1, ite->ypos);
		break;

	case ROLL_UP:
		scr_scroll_up();
		break;

	case ROLL_DOWN:
		scr_scroll_down();
		break;

	case PREV_PAGE:
		scr_line(ite->first_line-ite->screenheight);
		scr_set_cursor(0, 0);
		break;

	case NEXT_PAGE:
		scr_line(ite->first_line+ite->screenheight);
		scr_set_cursor(0, 0);
		break;

	case REDRAW:
		if (ite->key_state!=TCONFIG && ite->key_state!=SET_USER)
			scr_line(ite->first_line);
		if (ite->flags & SFK_ON)
			scr_dsp_labels(ite->key_state,FORCE);
		else
			scr_dsp_labels(CLEAR, 0);
		break;

	case HOME_UP:
		if (ite->key_state == TCONFIG)
			scr_cursor_tf(1,1);
		else if (ite->key_state == SET_USER)
			set_key_tf(0);
		else {
			if (ite->first_line!=1)
				scr_line(1);
			scr_set_cursor(0, 0);
		}
		break;

	case HOME_DOWN:
		if(ite->first_line+ite->screenheight<=ite->last_line)
			scr_line(ite->last_line-ite->screenheight+1);
		scr_set_cursor(0, scr_min(ite->screenheight-1,
			ite->last_line-ite->first_line));
		break;

	case INSERT_MODE:
		ite->insert_mode = (parm1 ? TRUE : FALSE);
		if ((ite->key_state != CLEAR) && (ite->flags&SFK_ON))
			scr_insert_ind();
		break;

	case INSERT:
		scr_insert(1);
		break;

	case DELETE:
		if (ite->key_state == TCONFIG) {
			j = ite->select;
			switch (j) {
			default:
				kbd_beep();
				break;
			case 3:	/* ReturnDef */
				if (ite->sx==0)
					ite->retdef_config[0] = ite->retdef_config[1];
				ite->retdef_config[1] = ' ';
				scr_value_tf(j,DISPLAY);
				break;
			case 14:	/* FldSeparator */
				ite->flds_config = (char)' ';
				scr_value_tf(j,DISPLAY);
				break;
			case 15:	/* BlkTermnator */
				ite->blkt_config = (char)' ';
				scr_value_tf(j,DISPLAY);
				break;
			}
			break;
		}
		if (ite->key_state == SET_USER) {
			j = ite->select;
			switch (j%3) {
			case 0:	/* Type */
				kbd_beep();
				break;
			case 1:	/* Label */
				i=ite->sx;
				while (i<(LABEL_SIZE-1)) {
					user_label[j/3][i] = user_label[j/3][i+1];
					i++;
				}
				user_label[j/3][i] = ' ';
				scr_value_uf(j,DISPLAY);
				break;
			case 2: /* Key Definition */
				i=ite->sx;
				while (user_key[j/3][i]) {
					user_key[j/3][i] = user_key[j/3][i+1];
					i++;
				}
				scr_value_uf(j,DISPLAY);
				break;
			}
			break;
		}
		scr_delete();
		break;

	case TAB_DIR:
		{
		/* Handle Configuration Screen Cases */
		if (ite->key_state==TCONFIG ||
			ite->key_state==SET_USER) {
			if (parm1 == FORWARD)
				tf_next(NEXT);
			else {
				/* If inside a field just go to
				** beginning of current field.
				*/
				if (ite->sx)
					tf_next(DISPLAY);
				else
					tf_next(PREV);
			}
			break;
		}
		/* Handle Normal Tabs */
		if (parm1 == FORWARD) {
			ite->xpos++;
			if (ite->tabstops[ite->xpos] == 0) {
				if (ite->ypos >= ite->maxy)
					scr_scroll_up();
				else
					ite->ypos++;
			}
			ite->xpos = ite->tabstops[ite->xpos];
		} else {
			if (ite->xpos <= 0) {
				if (--ite->ypos < 0) {
					i = ite->first_line;
					scr_scroll_down();
					ite->ypos = 0;
					if (i == 1)
						ite->xpos = 0;
					else
						ite->xpos = ite->maxx;
				} else
					ite->xpos = ite->maxx;
			}
			i = ite->tabstops[ite->xpos];
			while (ite->xpos && (ite->tabstops[ite->xpos] == i))
				ite->xpos--;
			scr_set_cursor(ite->tabstops[ite->xpos], ite->ypos);
		}

		scr_line_include();
		if (parm1!=FORWARD || ite->xpos!=0) {
			{
			SCROLL *scroll_ptr;

			/* Enable enhancements and color pairs from this
			** point and backwards
			*/

			scroll_ptr = crt_xy_to_scroll(ite->xpos, ite->ypos);
			*scroll_ptr |= CHAR_ON;

			(*ite->crt_write)(ite, scroll_ptr, ite->ypos, ite->xpos, 1);
			scr_include_enh(); /* enable enhancement backward */
			}
		}
		break;
		}

	case SET_TAB:
		scr_set_tab();
		break;

	case CLEAR_TAB:
		scr_clear_tab();
		break;

	case CLEAR_ALL_TABS:
		scr_clear_all_tabs();
		break;

	case SCR_HARD_RESET:
		term_reset(HARD);
		break;

	case SCR_SOFT_RESET:
		term_reset(SOFT);
		break;

	case WAIT_SEC:
		timeout(scroller, REENTER, HZ, NULL);
		break;

	case ALPHA_OFF_FULL:
		/* Alpha Off for Escape Sequence {ESC}*dF */
		x = splx(tp->t_int_lvl);
		ite->flags &= ~ALPHAON;
		ite->flags |= GRAPHICSON;
		if (ite->key_state!=CLEAR && (ite->flags&SFK_ON))
			scr_dsp_labels(ite->key_state, FORCE);
		else
			(*ite->crt_set_state)(ite);
		splx(x);
		break;

	case LEFTDRIVE:
		{
		register char *ptr;

		ptr = (char *)parm1;
		for (i=0; (*ptr && i<LABEL_SIZE); i++)
			drive_label[0][i] = *ptr++;
		if (ite->key_state == FLEXDISC)
			print_label(drive_label[0], 0);
		break;
		}

	case RIGHTDRIVE:
		{
		register char *ptr;

		ptr = (char *)parm1;
		for (i=0; (*ptr && i<LABEL_SIZE); i++)
			drive_label[7][i] = *ptr++;
		if (ite->key_state == FLEXDISC)
			print_label(drive_label[7], 7);
		break;
		}

	case SFK_INPUT:
		{
		register unsigned c;
		register unsigned char *ptr;

		switch (ite->key_state) {
		case FLEXDISC:
			kbd_beep();
			break;
		case AIDS:
			switch (parm1) {
			case 2: scr_dsp_labels(MARGINS,0);	break;
			case 4: scr_dsp_labels(MODES,0);	break;
			case 5: scr_dsp_labels(FLEXDISC,0);	break;
			case 8: scr_dsp_labels(CONFIG,0);	break;
			default: kbd_beep(); break;
			}
			break;
		case MODES:
			switch (parm1) {
			case 4:	/* Remote */
				scr_remote(ite->remote_mode ^= TRUE);
				break;
			case 7:	/* Display Functions */
				if (ite->xmit_fnctn && ite->disp_funcs==FALSE) {
					send_escape("\033Y",2);
					break;
				}
				scr_disp_funcs(ite->disp_funcs ^= TRUE);
				break;
			case 8:	/* AUTO LF */
				scr_autolf(ite->autolf_mode ^= TRUE);
				break;
			default:
				kbd_beep();
				break;
			}
			break;
		case MARGINS:
			switch (parm1) {
			case 2: scr_set_tab();		break;
			case 3: scr_clear_tab();	break;
			case 4: scr_clear_all_tabs();	break;
			default: kbd_beep();		break;
			}
			break;
		case CONFIG:
			switch (parm1) {
			case 5:
				/* Copy real values to config temporaries */
				k_keystate.flags_config = k_keystate.flags;
				k_keystate.language_config= k_keystate.language;
				ite->retdef_config[0] = ite->retdef[0];
				ite->retdef_config[1] = ite->retdef[1];
				ite->local_echo_config = ite->local_echo;
				ite->xmit_fnctn_config = ite->xmit_fnctn;
				ite->inheolwrp_config = ite->inheolwrp;
				ite->flds_config = ite->flds;
				ite->blkt_config = ite->blkt;

				/* Turn off local display functions */
				ite->dsp_funcs = FALSE;
				set_label(tconf_label[6], FALSE);
				scr_dsp_labels(TCONFIG,0);
				scr_save_dsp();
				scr_tform();
				break;
			default:
				kbd_beep();
				break;
			}
			break;
		case TCONFIG:
			switch (parm1) {
			case 2:	/* NEXT CHOICE */
				scr_value_tf(ite->select,NEXT);
				break;
			case 3: /* PREVIOUS CHOICE */
				scr_value_tf(ite->select,PREV);
				break;
			case 4:	/* DEFAULT VALUES */
				scr_value_tf(0,DEFAULT);
				break;

			case 1:	/* SAVE CONFIG */
				k_keystate.flags = k_keystate.flags_config;
				k_keystate.language= k_keystate.language_config;
				ite->retdef[0] = ite->retdef_config[0];
				ite->retdef[1] = ite->retdef_config[1];
				ite->local_echo = ite->local_echo_config;
				ite->xmit_fnctn = ite->xmit_fnctn_config;
				ite->inheolwrp = ite->inheolwrp_config;
				ite->flds = ite->flds_config;
				ite->blkt = ite->blkt_config;
				kbd_language();
				char_switch();

				/* fall thru to case 8: */

			case 8: /* Go back to CONFIG */
				scr_dsp_labels(CONFIG,0);
				scr_restore_dsp(ite, 0, 0);
				break;

			case 7:	/* DISPLAY FUNCTIONS */
				set_label(tconf_label[6],ite->dsp_funcs ^= TRUE);
				print_label(tconf_label[6], 7-1);
				break;
			default:
				kbd_beep();
				break;
			}
			break;
		case DCONFIG:
			switch (parm1) {
			case 8: scr_dsp_labels(CONFIG,0); break;
			default: kbd_beep(); break;
			}
			break;
		case SET_USER:
			switch (parm1) {
			case 1:	/* DISPLAY FUNCTIONS for 50 wide */
				kbd_beep();
				break;
			case 2:	/* NEXT CHOICE */
				scr_value_uf(ite->select,NEXT);
				break;
			case 3: /* PREVIOUS CHOICE */
				scr_value_uf(ite->select,PREV);
				break;
			case 4:	/* DEFAULT VALUES */
				scr_value_uf(0,DEFAULT);
				break;
			case 7:	/* DISPLAY FUNCTIONS for 80 wide */
				set_label(suser_label[6],ite->dsp_funcs ^= TRUE);
				print_label(suser_label[6], 7-1);
				break;
			default:
				kbd_beep();
				break;
			}
			break;
		case SERVICE:
			kbd_beep();
			break;
		}
		break;
		}

	case DSP_LABELS:
		i = parm1;	/* Next State */
		j = ite->key_state;	/* Previous State */

		/* If previous state is neither TCONFIG or SET_USER,
		** and new state is either TCONFIG nor SET_USER ->
		** then save scroller crt image.
		*/
		if ( (j!=TCONFIG) && (j!=SET_USER) &&
			(i==TCONFIG || i==SET_USER) ) {
				scr_save_dsp();
				ite->last_labels = j;
		}

		/* Put up labels (and set crt parameters).
		** Possibly changing screen dimensions.
		*/
		scr_dsp_labels(parm1,parm2);

		/* If previous state is either TCONFIG or SET_USER,
		** and new state is neither TCONFIG nor SET_USER ->
		** then restore scroller crt image.
		*/
		if ( (j==TCONFIG || j==SET_USER) &&
			(i!=TCONFIG) && (i!=SET_USER) )
				scr_restore_dsp(ite, 0, 0);
		break;

	case SET_CRT_START:
		(*ite->crt_set_state)(ite);
		break;

	case AUTOLF:
		scr_autolf(parm1);
		break;

	case DISP_FUNCS:
		scr_disp_funcs(parm1);
		break;

	case REMOTE:
		scr_remote(parm1);
		break;

	case PLANE_MASK:
		if (j = parm1 & ite->plane_mask)
			if (ite->cur_planes != j) {
				(*ite->crt_clear)(ite, 0, 0, ite->screensize);
				ite->cursor_on = FALSE;
				ite->cur_planes = j;
				/* restore the hardware color maps */
				(*ite->crt_reset)(ite);
				scr_restore_dsp(ite, 0, 0);
				scr_dsp_labels(ite->key_state,FORCE);
			}
		break;

	case CURSOR_ON:
		/* kbd_beep(); */
		break;

	case CURSOR_OFF:
		/* kbd_beep(); */
		break;

	default:
		kbd_beep();
		scr_char_out(127);	/* Box character */
		break;
	}
	/* restore cursor and display if necessary */
	/* must be false !!!!!!!!!!!!!!!!!!!!!!!1 */
	if (ite->cursor_on == FALSE) {
		if (ite->key_state==TCONFIG || ite->key_state==SET_USER)
			toggle_config_cursor(ite);
		else
			scr_cursor(ite->xpos, ite->ypos); /* put back cursor */
	}
}

/*
 * There's a problem with cursors in the configuration screens.
 * The STI routines can't do an XOR like most bitmapped displays,
 * so we just re-write the character in inverse video.  For a normal
 * screen, this works great, because we know what character should be
 * under the cursor by looking at the scroll buffer.  For configuration
 * screens, though, there *is* no scroll buffer.  We fix this by creating
 * a scroll buffer for the configuration screens and consulting it
 * when we want to write a cursor for those screens.
 */

SCROLL config_screen[80][24];		/* All config screens fit into this */

/*
 * Use this routine instead of ite->crt_write when writing to a
 * configuration screen.  It stashes away the characters in a config_screen.
 * It's a bit slower, but who cares for configuration screens.
 */

config_write(ite, buf, y, x, count)
struct iterminal *ite;
SCROLL *buf;
{
	for (; count--; x++, buf++) {
		(*ite->crt_write)(ite, buf, y, x, 1);
		config_screen[x][y] = *buf;
	}
}

/* Write a configuration-screen cursor */
toggle_config_cursor(ite)
struct iterminal *ite;
{
	(*ite->crt_cursor)(ite, ite->cxpos, ite->cypos,
		config_screen[ite->cxpos][ite->cypos]);
}


/*************************************************************************
** Console Interface Routines:						**
**									**
** iteputchar(c)		Print a char - used by Kernel printf.	**
*************************************************************************/

iteputchar(c)
register unsigned c;
{
	register struct iterminal *ite = &iterminal;
	register x;

	/* Don't interrupt someone in remote mode */
	if (ite->remote_mode == FALSE) return;
	if (c == 0) return;

	x = spl6();

	if (!(ite->flags & ALPHAON))		/* Is alpha off? */
		alpha_on();			/* This output is important! */

	if (ite->key_state==TCONFIG || ite->key_state==SET_USER)
		scroller(DSP_LABELS, ite->last_labels, 0);
	if (c == '\n') crtio((unsigned int)'\r');
	crtio(c);

	splx(x);
}


/*************************************************************************
** Internal Terminal Emulator State Routines:				**
**									**
** scr_remote(state)		Set REMOTE state.			**
** scr_autolf(state)		Set AUTOLF state.			**
** scr_disp_funcs(state)	Set Display Functions state.		**
** alpha_on()			Turn alpha Section of screen on.	**
** alpha_off()			Turn alpha Section of screen off.	**
** crt_set_state(ite)		Set up CRT Controller Properly.		**
** crt_cursor(ite, x ,y, char)	Move cursor to x,y.			**
*************************************************************************/

scr_remote(state)
{
	register struct iterminal *ite = &iterminal;

	ite->remote_mode = (state ? TRUE : FALSE);
	if (state)
		ite->escape = FALSE;
	set_label(modes_label[3], ite->remote_mode);
	if (ite->key_state == MODES && (ite->flags & SFK_ON))
		print_label(modes_label[3], 4-1);
}

scr_autolf(state)
{
	register struct iterminal *ite = &iterminal;

	ite->autolf_mode = (state ? TRUE : FALSE);
	set_label(modes_label[7], ite->autolf_mode);
	if (ite->key_state == MODES && (ite->flags & SFK_ON))
		print_label(modes_label[7], 8-1);
}

scr_disp_funcs(state)
{
	register struct iterminal *ite = &iterminal;

	ite->disp_funcs = (state ? TRUE : FALSE);
	set_label(modes_label[6], ite->disp_funcs);
	if (ite->key_state == MODES && (ite->flags & SFK_ON))
		print_label(modes_label[6], 7-1);
}


alpha_on()
{
	register struct iterminal *ite = &iterminal;

	ite->flags |= ALPHAON;
	if (ite->key_state!=CLEAR && (!(ite->flags&SFK_ON)) &&
		!(ite->flags&BIT_MAPPED))
		scr_dsp_labels(ite->key_state, FORCE);
	else
		(*ite->crt_set_state)(ite);
}

alpha_off()
{
	register struct iterminal *ite = &iterminal;

	ite->flags &= ~ALPHAON;
	ite->flags |= GRAPHICSON;
	(*ite->crt_set_state)(ite);
}

graphics_on()
{
	register struct iterminal *ite = &iterminal;

	ite->flags |= GRAPHICSON;
	(*ite->crt_set_state)(ite);
}

graphics_off()
{
	register struct iterminal *ite = &iterminal;

	ite->flags &= ~GRAPHICSON;
	ite->flags |= ALPHAON;
	(*ite->crt_set_state)(ite);
}

/*************************************************************************
** Screen Switch Routines for Moving Between Screens:			**
**									**
** scr_clear(start,stop)	Clear Range of CRT.			**
** scr_clear_mem(start)		Clear line of Scroller memory.		**
** scr_save_dsp()		Save display for Configure Mode.	**
** scr_restore_dsp(ite, 0, 0)	Restore display for Configure Mode.	**
*************************************************************************/

scr_clear_mem(line_num)
int line_num;
{
	struct iterminal *ite = &iterminal;
	register int c = ' ';
	register short i = ite->screenwidth;
	register SCROLL *start = ite->scroll_buf + (ite->screenwidth * line_num);

	do {
		*start++ = c;
	} while (--i);
}

/* Save display for Configure Mode */
scr_save_dsp()
{
	register struct iterminal *ite = &iterminal;

	/* Clear screen area. */
	(*ite->crt_clear)(ite, 0, 0, ite->screensize);
	ite->cursor_on = FALSE;
}

/* Restore display from passed position */
scr_restore_dsp(ite, x, y)
register struct iterminal *ite;
{
	register SCROLL *scroll_ptr;
	register size;
	register screenwidth = ite->screenwidth;

	/* Turn off local display functions. */
	ite->dsp_funcs = FALSE;

	/* Clear screen area. */
	if ((size = (ite->screenheight - y) * screenwidth - x) <= 0)
		return;
	(*ite->crt_clear)(ite, y, x, size);
	ite->cursor_on = FALSE;

	/* first line may not start at left margin */
	screenwidth -= x;

	/* Restore screen from scroller memory. */
	for (; y < ite->crt_stop_line; y++) {
		scroll_ptr = crt_xy_to_scroll(x, y);
		(*ite->crt_write)(ite, scroll_ptr, y, x, screenwidth);

		/* further lines start at left margin */
		x = 0;
		screenwidth = ite->screenwidth;
	}
}
/*************************************************************************
** Miscellaneous Routines						**
**									**
** char *scr_on_off(yes)	Return " ON" or "OFF"			**
** char *scr_yes_no(yes)	Return "YES" or " NO"			**
** scr_min(x,y)			Returns min				**
** scr_max(x,y)			Returns max				**
*************************************************************************/

char *
scr_on_off(yes)
{
	char *p;

	p = (yes ? " ON" : "OFF");
	return(p);
}

char *
scr_yes_no(yes)
{
	char *p;

	p = (yes ? "YES" : " NO");
	return(p);
}

scr_min(x,y)
{
	return(x<y ? x : y);
}

scr_max(x,y)
{
	return(x>y ? x : y);
}
/*************************************************************************
** Soft Key Label Routines:						**
**									**
** scr_insert_ind()		Indicate Insert Mode or Not.		**
** print_label(str_ptr, f_num)	Place str_ptr in sfk f_num+1.		**
** scr_dsp_labels(l_num,force)	Put up labels of type l_num.		**
** set_label(str_ptr, star)	Set STAR on or off in label.		**
*************************************************************************/

/* Indicate Insert Mode or Not. */
scr_insert_ind()
{
	register struct iterminal *ite = &iterminal;
	static SCROLL ins_on[]  = {'I'|CHAR_ON, 'C'|CHAR_ON};
	static SCROLL ins_off[] = {' '|CHAR_ON, ' '|CHAR_ON};

	(*ite->crt_write)(ite,
			  ite->insert_mode ? ins_on : ins_off,
			  ite->screenheight, 39, 2);
}

/*
		50 wide screen template
123456789|123456789|123456789|123456789|123456789!
12345678!12345678!12345678!12345678!!!!!!!!!!!!!!!
12345678!12345678!12345678!12345678!!!!!!!!!!!!!!!
		80 wide screen template
!123456789|123456789|123456789|123456789|123456789|123456789|123456789|123456789
12345678!12345678!12345678!12345678!!!!!!!!!!12345678!12345678!12345678!12345678
*/
/* Place character string (at str_ptr) in sfk number f_num+1 */
print_label(str_ptr, f_num)
register unsigned char *str_ptr;
register int f_num;
{
	register struct iterminal *ite = &iterminal;
	register i;
	int x, y;
	SCROLL scroll_b[LABEL_WIDTH];

	y = ite->screenheight;
	x = f_num * (1+LABEL_WIDTH);

	 if (f_num >= 4) {
		x += 9; /* skip 9 spaces for 80 wide screen */
	}

	/*
	 * If we have two lines of softkeys, make sure that
	 * the background regions touch.  We do this by clearing
	 * the first line to the background color of color pair 7,
	 * the softkey color.  Since clear acts on line_height, this works.
	 */
	if (ite->sfkheight > 1 && ite->font_height != ite->line_height) {
		i = ite->color_pairs[0].BG;
		ite->color_pairs[0].BG = ite->color_pairs[7].BG;
		(*ite->crt_clear)(ite, y, x, LABEL_WIDTH);
		ite->color_pairs[0].BG = i;
	}

	/* Write the softkey labels in color pair 7. */
	for (i = 0; i < LABEL_WIDTH; i++)
		scroll_b[i] = *str_ptr++ | (CHAR_ON|(7<<CPAIR_SFT));
	(*ite->crt_write)(ite, scroll_b, y, x, LABEL_WIDTH);
	if (ite->sfkheight > 1) {
		for (i = 0; i < LABEL_WIDTH; i++)
			scroll_b[i] = *str_ptr++ | (CHAR_ON|(7<<CPAIR_SFT));
		(*ite->crt_write)(ite, scroll_b, y+1, x, LABEL_WIDTH);
	}
	/*
	** Place Separators.
	*/
	switch (f_num) {
	default:
		/*
		** Usually end a label with one separator in color pair 0
		** otherwise try nothing.
		*/
		(*ite->crt_clear)(ite, y, x + LABEL_WIDTH, 1);
		if (ite->sfkheight > 1)
			(*ite->crt_clear)(ite, y+1, x + LABEL_WIDTH, 1);
		break;
	case 3:
		/*
		** Most have 10 separators after the fourth label,
		** except for 50 wide screens which have 15 separators.
		** Do it in color pair 0.
		*/
		(*ite->crt_clear)(ite, y, x + LABEL_WIDTH, 4);
		i = 4;
		(*ite->crt_clear)(ite, y, x + LABEL_WIDTH + 6, i);
		if (ite->sfkheight > 1) {
			/* Don't leave a space for IC on second line */
			(*ite->crt_clear)(ite, y+1, x + LABEL_WIDTH, i+6);
		}
		break;
	case NUMBER_OF_SFKS-1:
		/*
		** Last label will normaly have no separators after it.
		** Except for 50 wide which has 15 separators after it.
		*/
		break;
	}
}

scr_dsp_labels(l_num,force)
int l_num,force;
{
	register struct iterminal *ite = &iterminal;
	register char **s;
	register i;

	/* Check for clear of Labels */
	if (l_num==CLEAR || (!(ite->flags&ALPHAON))) {
		/* remove labels */
		i = ite->screenwidth * 1;
		(*ite->crt_clear)(ite, ite->screenheight, 0, i);

		if (ite->sfkheight > 1)
			(*ite->crt_clear)(ite, ite->screenheight+1, 0, i);

		ite->flags &= ~SFK_ON;
		(*ite->crt_set_state)(ite);
		return;
	}
	switch (l_num) {
	default:
	case USER:     s = (char **) user_label;   break;   /* user labels */
	case AIDS:     s = aids_label;   break;   /* aids labels */
	case MODES:    s = modes_label;  break;   /* modes labels */
	case MARGINS:  s = marg_label;   break;   /* margins/tabs/col labels */
	case CONFIG:   s = conf_label;   break;   /* config keys labels */
	case TCONFIG:
	case DCONFIG:  s = tconf_label;  break;   /* configuration labels */
	case SERVICE:  s = serv_label;   break;   /* service test labels */
	case FLEXDISC: s = drive_label;  break;   /* Internal floppy status */
	case SET_USER:
		/* display configuration labels */
		s = suser_label;
		/* Turn off local display functions */
		ite->dsp_funcs = FALSE;
		set_label(suser_label[6], FALSE);
		set_key_dsp();
		break;
	}
	(*ite->crt_clear)(ite, ite->screenheight, 0, ite->screenwidth);
	if (ite->sfkheight>1)
		(*ite->crt_clear)(ite, ite->screenheight + 1, 0, ite->screenwidth);

	/* Go to correct state */
	ite->key_state = (char)l_num;
	ite->flags |= SFK_ON;
	for (i = 0; i < NUMBER_OF_SFKS; i++, s++)
		print_label(*s, i);

	scr_insert_ind();
	(*ite->crt_set_state)(ite);
}

set_label(str_ptr, star)
register char *str_ptr;
int star;
{
	struct iterminal *ite = &iterminal;
	str_ptr[ite->sfkheight*8-1] = (star ? '*' : ' ');
}
/*************************************************************************
** Scroller Screen Routines:						**
**									**
** scr_alpha_clear(flag)	Clear from current x,y to line/screen.	**
** scr_set_tab()		Set tab from current x.			**
** scr_clear_tab()		Clear tab from current x.		**
** scr_clear_all_tabs()		Clear all tabs.				**
** scr_line_include()		Include x,y in scroller.		**
** scr_delete_line(start_row)	Delete line start_row.			**
** scr_scroll_up()		Scroll up one line off top of screen.	**
** scr_insert_line(start_row)	Insert line at start_row.		**
** scr_scroll_down()		Scroll one line off bottom of screen.	**
** scr_delete()			Delete a character.			**
** scr_insert()			Open space on line for inserting char.	**
** scr_set_cursor(x,y)		Move cursor to x,y.			**
** char_switch()		Switch to appropriate character set.	**
** SCROLL *scr_inc_ptr(ptr,cnt)	Increment ptr by cnt.			**
** SCROLL *scr_dec_ptr(ptr,cnt)	Decrement ptr by cnt.			**
** scr_line(line)		Put line at top of screen.		**
** scr_char_out(c)		Place char in scroller & screen.	**
** scr_enhance(type)		Start new enhancements.			**
** scr_colorpair(type)		Start new color pair.			**
** scr_include_enh()		Include and turn on everything to left. **
*************************************************************************/

/* Clear from cursor rest of line or screen */
scr_alpha_clear(flag)
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *dest;
	register j, k;
	int n;


	/* If this clear goes to end of screen then reset
	** scroller also.
	*/
	if (flag) { /* for case of rest of screen */
		/* Reset end of crt */
		k = ite->crt_stop_line = ite->ypos + (ite->xpos?1:0);
		/* Erase end of scroller memory. */
		ite->bottom_line = scroll_add_line(ite->start_line, k);
		ite->last_line = ite->first_line + k;
		n = ite->screensize - (ite->screenwidth * ite->ypos + ite->xpos);
	} else { /* for case of line only */
		n = ite->screenwidth - ite->xpos;
	}

	/* Blank from current position to end of one line in scroller memory */
	/* Copy previous enhancement also, but don't enable it */
	if (ite->xpos == 0)
		scr_clear_mem(scroll_add_line(ite->start_line, ite->ypos));
	else {
		dest = crt_xy_to_scroll(ite->xpos, ite->ypos);
		/* Fetch enhancements from previous position */
		/* set char enhancements to forward chars in line */
		j = (*(dest-1) & (CHAR_CPAIR|CHAR_ENHANCE)) + ' ';
		k = ite->screenwidth - ite->xpos;
		do {
			*dest++ = j;
		} while (--k);
	}

	/* Blank crt from x,y to end of line or screen */
	(*ite->crt_clear)(ite, ite->ypos, ite->xpos, n);
	ite->cursor_on = FALSE;
}

scr_set_tab()
{
	register struct iterminal *ite = &iterminal;
	register i;

	ite->tabstops[ite->xpos] = ite->xpos;
	for (i=ite->xpos-1; i >= 0; i--) {
		if (ite->tabstops[i] != i)
			ite->tabstops[i] = ite->xpos;
		else
			break;
	}
}

scr_clear_tab()
{
	register struct iterminal *ite = &iterminal;
	register i,j;

	ite->tabstops[ite->xpos] = j = ite->tabstops[ite->xpos+1];
	for (i=ite->xpos-1; i >= 0; i--) {
		if (ite->tabstops[i] != i)
			ite->tabstops[i] = j;
		else
			break;
	}
}

scr_clear_all_tabs()
{
	register struct iterminal *ite = &iterminal;
	register i;

	for (i=scr_min(ite->screenwidth, 128); --i>=0; )
		ite->tabstops[i] = 0;
}

/* Include current x,y in scroller. */
scr_line_include()
{
	register struct iterminal *ite = &iterminal;
	register source;
	register i;

	i = ite->ypos;

	/* Check if more space must be added to the scroller */
	if (i++ >= ite->crt_stop_line) {
		source = scroll_add_line(ite->start_line, ite->crt_stop_line);
		ite->crt_stop_line = i;
		ite->bottom_line = scroll_add_line(ite->start_line, i);
		while(source != ite->bottom_line) { /* Add a full line */
			/* Check if a line must be deleted from the top */
			if (source==ite->top_line &&
					ite->first_line!=ite->last_line) {
				scroll_inc_line(ite->top_line);
				ite->first_line--;
			} else
				ite->last_line++;
			scr_clear_mem(source);
			scroll_inc_line(source);
		}
	}
}

/* Delete line at row. */
scr_delete_line(row)
{
	register struct iterminal *ite = &iterminal;
	SCROLL *scroll_ptr;

	/* Do not delete a line that is not there */
	if (row >= ite->crt_stop_line)
		return;

	/* Delete line from scroller memory */
	scroll_dec_line(ite->bottom_line);

	/* Scroll up Screen Section */
	(*ite->crt_big_scroll)(row, UP);

	scr_moveb(scroll_add_line(ite->start_line, row), ite->bottom_line);
	ite->last_line--;
	ite->crt_stop_line--;

	(*ite->crt_clear)(ite, ite->crt_stop_line, 0, ite->screenwidth);

	if (scroll_add_line(ite->start_line, ite->crt_stop_line) != ite->bottom_line) {
		/* From scroller memory */
		scroll_ptr = crt_xy_to_scroll(0, ite->crt_stop_line);
		(*ite->crt_write)(ite, scroll_ptr, ite->crt_stop_line, 0, ite->screenwidth);
		ite->crt_stop_line++;
	}
}

/* Scroll up one line off the top of the screen. */
scr_scroll_up()
{
	register struct iterminal *ite = &iterminal;
	SCROLL *scroll_ptr;

	/* Do not scroll off more than there is */
	if (ite->crt_stop_line <= 1)
		return;

	ite->first_line++;

	/* Scroll up Screen Section */
	(*ite->crt_big_scroll)(0, UP);

	/* Scroll top line of CRT Image into scroller memory */
	/* It's already there so just increment pointers. */
	scroll_inc_line(ite->start_line);
	ite->crt_stop_line--;

	(*ite->crt_clear)(ite, ite->crt_stop_line, 0, ite->screenwidth);
	/* Scroll into bottom of screen from scroller memory */
	if (scroll_add_line(ite->start_line, ite->crt_stop_line) != ite->bottom_line) {
		/* From scroller memory */
		scroll_ptr = crt_xy_to_scroll(0, ite->crt_stop_line);
		(*ite->crt_write)(ite, scroll_ptr,ite->crt_stop_line,0,ite->screenwidth);
		ite->crt_stop_line++;
	}
}

/* scroll the crt screen */
scroll_screen(row, dir)
register row;
{
	register struct iterminal *ite = &iterminal;
	register stop_line = ite->crt_stop_line;
	register SCROLL *vlp;
	register new_width, old_width;

	/* This code will calculate the line length of 1st line to scroll */
	old_width = ite->screenwidth;

	/* Scroll up Screen Section */
	if (dir == UP) {
		while (stop_line > ++row) {
		/* This code will calculate the min line length to scroll */
			new_width = ite->screenwidth;
			vlp = crt_xy_to_scroll(0, row) + new_width;
			do
				if (*--vlp & CHAR_ON)
					break;
			while (--new_width);

			(*ite->crt_scroll)(ite, row-1, UP, max(old_width,new_width));
			old_width = new_width;
		}

	/* Scroll down Screen Section */
	} else {
		/* don't scroll into the softkeys */
		if (stop_line >= ite->screenheight)
			--stop_line;
		while (stop_line-- > row) {
		/* This code will calculate the min line length to scroll */
			new_width = ite->screenwidth;
			vlp = crt_xy_to_scroll(0, stop_line) + new_width;
			do
				if (*--vlp & CHAR_ON)
					break;
			while (--new_width);

			(*ite->crt_scroll)(ite, stop_line+1, DOWN, max(old_width,new_width));
			old_width = new_width;
		}
	}
}


/* Insert line at start_row */
scr_insert_line(start_row)
register short start_row;
{
	register struct iterminal *ite = &iterminal;
	register j;

	/* Do not insert a line that is not on the screen */
	if (start_row >= ite->crt_stop_line)
		return;

	/* Insert a line in scroller memory. */
	/* If necessary lose a line from top of scroller. */
	if (ite->top_line!=ite->bottom_line || ite->first_line==ite->last_line) {
		/* Scroller has More Space */
		ite->last_line++;
	} else { /* Scroller is Full UP */
		/* Delete top line
		** unless top is on the screen then delete a
		** line off the bottom.
		*/
		if (ite->top_line != ite->start_line) {	/* Delete from top */
			scroll_inc_line(ite->top_line);
			ite->first_line--;
		} else	/* Delete from bottom */
			scroll_dec_line(ite->bottom_line);
	}

	/* Scroll down Screen Section */
	(*ite->crt_big_scroll)(start_row, DOWN);

	/* Move bottom part forward */
	scroll_inc_line(ite->bottom_line);
	scr_movef(ite->bottom_line, scroll_add_line(ite->start_line, start_row+1));

	/* Adjust pointers */
	/* Scroll bottom of screen into scroller memory if screen is full */
	/* Otherwise expand extent of image on CRT. */
	if (ite->crt_stop_line < ite->screenheight) {
		ite->crt_stop_line++;
	}

	/* Blank out line in question */
	scr_clear_mem(scroll_add_line(ite->start_line, start_row));
	(*ite->crt_clear)(ite, start_row, 0, ite->screenwidth);
}

/* Scroll one line off the bottom of the screen */
scr_scroll_down()
{
	register struct iterminal *ite = &iterminal;
	SCROLL *scroll_ptr;

	/* Do not scroll down if there is nothing to scroll down */
	if (ite->top_line == ite->start_line)
		return;

	/* Scroll bottom of screen into scroller memory if screen is full */
	/* Otherwise expand extent of image on CRT. */

	if (ite->crt_stop_line < ite->screenheight)
		ite->crt_stop_line++;
	ite->first_line--;

	/* Scroll down Screen Section */
	(*ite->crt_big_scroll)(0, DOWN);

	/* Scroll into top of screen from scroller memory */
	scroll_dec_line(ite->start_line);

	(*ite->crt_clear)(ite, 0, 0, ite->screenwidth);

	scroll_ptr = crt_xy_to_scroll(0, 0);
	(*ite->crt_write)(ite, scroll_ptr, 0, 0, ite->screenwidth);
}

scr_delete() /* delete one character at cursor position */
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *s, *ds;
	register i, j;
	SCROLL *scroll_ptr;

	/* Compute offset to character to delete */

	/* If past end of data then return */
	if (ite->ypos >= ite->crt_stop_line)
		return;

	scroll_ptr = ds = crt_xy_to_scroll(ite->xpos, ite->ypos);
	s = ds + 1; /* char past cursor */

	/* Compute number of characters to move. */
	/* If i==0 then last character in line is being deleted. */
	i = ite->screenwidth - ite->xpos - 1;

	/* Enhancements and Color Pen Starts stay with position!!!, only to
	** be overriden by a new one to the right.
	*/

	if (i && (CHAR_ON & *s)) {
		/* Check if enhancement should be copied ahead one position */
		if ((CHAR_ENSTART & *ds) && (CHAR_ENSTART & *s)==0)
			*s = (*s & (~CHAR_ENHANCE)) | (*ds & CHAR_ENHANCE);

		/* Check if color pen should be copied ahead one position */
		if ((CHAR_CPAIRSTART & *ds) && (CHAR_CPAIRSTART & *s)==0)
			*s = (*s & (~CHAR_CPAIR))   | (*ds & CHAR_CPAIR);
	}

	while(i--)  /* copy char char to previous char to line in scroll buf */
		if (!((*ds++ = *s++) & CHAR_ON)) {
			ds--;
			break;
		}
	*ds = CHAR_ON | ' '; /* blank last char in line */
	scroll_ptr = crt_xy_to_scroll(ite->xpos, ite->ypos);
	(*ite->crt_write)(ite, scroll_ptr, ite->ypos, ite->xpos, ite->screenwidth-ite->xpos);

	*ds = ' '; /* now turn off the char */
}

scr_insert(num) /* make space for insert of chars in line */
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *s, *l;
	register i, j;

	if (ite->ypos >= ite->crt_stop_line)
		return;
	i = ite->screenwidth - ite->xpos - num;
	if (i <= 0)
		return;
	/* find last char address in line  + 1 */
	l = crt_xy_to_scroll(ite->maxx, ite->ypos) + 1;
	s = l - num;

	/* Enhancements and Color Pen Starts stay with 1st position, only to
	** be overridden by a new one to the right.
	*/
	/* we know that scroll_buf does not wrap around within line */
	do {
		*--l = *--s;
	} while(--i);

	/* move Color & Enhancements without start bits into inserted spaces */
	s = l;
	j = *s;  /* complier bug prevents this as one statement */
	j &= ~(CHAR_ENSTART|CHAR_CPAIRSTART);
	for (i = 0; i < num; i++)
		*s-- = j;

	/* write out the moved characters, inserted will get rewritten later */
	i = ite->xpos + num;
	j = ite->screenwidth - i;
	(*ite->crt_write)(ite, l, ite->ypos, i, j);
}

scr_set_cursor(x, y)
register x, y;
{
	register struct iterminal *ite = &iterminal;

	if (x >= ite->screenwidth) ite->xpos = ite->maxx;
	else if (x < 0) ite->xpos = 0;
	else ite->xpos = x;

	if (y >= ite->screenheight) ite->ypos = ite->maxy;
	else if (y < 0) ite->ypos = 0;
	else ite->ypos = y;
}

scr_cursor(x, y)
register x, y;
{
	register struct iterminal *ite = &iterminal;
	register oldc = 0;

	/* use background color of char if in scroll memory */
	if (y < ite->crt_stop_line)
		oldc = *crt_xy_to_scroll(x, y);
	(*ite->crt_cursor)(ite, x, y, oldc); /* put back cursor */
}

/* Switch to the appropriate character set. */
char_switch()
{
	register struct iterminal *ite = &iterminal;
	register struct k_keystate *k = &k_keystate;
	register i, j;
	register unsigned char *sp;
	register unsigned short *cp;
	register kana_flag = FALSE;
	register eight_bit;

	ite->flags &= ~KANA8;

	eight_bit = k_keystate.flags&K_ASCII8;

	/* set the alternate char set flag if 8 bit and katakana */
	if (k_keystate.language == K_N_KATAKANA ||
		 	k_keystate.language == K_I_KANJI ||
			k_keystate.language == K_I_KATAKANA) {
		kana_flag = TRUE;
		if (eight_bit)
			ite->flags |= KANA8;
	}

	/* Handle ISO 7 bit or 8 bit */
	if (eight_bit)
	    for (i=0; i<256; i++)
		ite->char_out[i] = i;
	else for (i = 0; i < 128; i++)
		ite->char_out[i] = ite->char_out[i + 128] = k->k_iso7to8[i];

	if (ite->flags & BIT_MAPPED) {
		if (kana_flag) {
			/* Simulate KANA8 */
			ite->char_out['\\'] = 188; /* backslash becomes YEN */

			/* Map 160-224 to the Katakana chars at 256-320 */
			if (eight_bit) {
				for (i=128, cp = &ite->char_out[128]; i<161; i++)
					*cp++ = '?';
				for (j=257; i<224; i++)
					*cp++ = j++;
				for (; i<256; i++)
					*cp++ = '?';
			}
		}
	} else { /* alpha plane */

		/* If CRT ID Register Present */
		if (sysflags&0x10) {
			/* And CRT ID Register Says Do Not Map */
			if (((*(unsigned short *)(0x051fffe+LOG_IO_OFFSET))) & 0x2000)
				return;
		}
		/* Is it Katakana */
		if (kana_flag) {

			/* backslash becomes YEN */
			ite->char_out['\\'] = 128;

			/* handle 8 bit case */
			if (eight_bit) {
				for (i = 128, cp = &ite->char_out[128]; i<161; i++)
					*cp++ = HP_CHAR;
				for (sp = (unsigned char *)&kat1[0]; i<224; i++)
					*cp++ = *sp++;
				for (; i<256; i++)
					*cp++ = HP_CHAR;
			}
		} else {
			/* Is it ROMAN Extended and 8 bit and alpha plane */
			if (eight_bit) {
				for (i = 128, cp = &ite->char_out[128]; i<161; i++)
					*cp++ = HP_CHAR;
				for (sp = (unsigned char *)&rom1[0]; i<240; i++)
					*cp++ = *sp++;
				for (; i<256; i++)
					*cp++ = HP_CHAR;
			}
		}
	}
	(*ite->crt_font_restore)(ite);
}

/* scr_line(line)	Put line at top of screen. -- next page	*/
scr_line(line)
register line;
{
	register struct iterminal *ite = &iterminal;
	register int next_line;
	register i = 0;
	SCROLL *scroll_ptr;

	/* Find new start of screen */
	line = scr_min(ite->last_line-1, line);
	line = scr_max(1, line);
	ite->start_line = next_line = scroll_add_line(ite->top_line, line-1);
	ite->first_line = line;
	line = scr_min(ite->screenheight, ite->last_line - ite->first_line);

	/* Copy out new data */
	while (i < line) {
		scroll_ptr = crt_xy_to_scroll(0, i);
		(*ite->crt_clear)(ite, i, 0, ite->screenwidth);
		(*ite->crt_write)(ite, scroll_ptr, i++, 0, ite->screenwidth);
		scroll_inc_line(next_line);
		if (next_line == ite->bottom_line)
			break;
	}
	ite->crt_stop_line = i;

	/* Clear rest of screen area. */
	line = ite->screenwidth * (ite->screenheight - ite->crt_stop_line);
	(*ite->crt_clear)(ite, ite->crt_stop_line, 0, line);
}

/* Place characters in both scroller and on screen at current position */

scr_chars_out(cp, n)
register unsigned char *cp;
register n;
{
	register SCROLL *dest, *scroll_ptr;
	register j, k;
	register struct iterminal *ite = &iterminal;

	while (n) {
		/* clean up prev chars & EOL chars and get screen mem */
		do
			scr_char_out(*cp++);
		while (--n && (ite->xpos >= ite->maxx));

		if (n == 0) return;

		scroll_ptr = dest = crt_xy_to_scroll(ite->xpos, ite->ypos);
		k = j = scr_min(n, ite->maxx - ite->xpos);
		if (ite->insert_mode)
			/* push remaining line right */
			scr_insert(j);
		n -= j;

		/* stuff scroller memory */
		do {
			*dest &= ~CHAR_VAL;
			*dest++ |= *cp++ | CHAR_ON;
		}
		while (--j);

		/* output it to the crt */
		(*ite->crt_write)(ite, scroll_ptr, ite->ypos, ite->xpos, k);
		ite->xpos += k;
	}
}

/* Place character in both scroller and on screen at current position */

scr_char_out(c)
unsigned int c;
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *dest;
	register x, z;

	if ((ite->key_state == TCONFIG) || (ite->key_state == SET_USER)) {
		conf_alpha(c);
		return;
	}

	if (c != ' ') {
		z = c & 0177;
		if ((z <= 32 || z == 0177) && (ite->disp_funcs == FALSE))
			return;
	}

	if (ite->insert_mode)
		scr_insert(1);

	x = ite->xpos;

	/* Check to see if we need to allocate new line(s). */
	if (ite->ypos >= ite->crt_stop_line)
		scr_line_include();

	dest = crt_xy_to_scroll(x, ite->ypos);

	/* Place character in scroller buf with old enhancements */
	*dest &= ~CHAR_VAL;
	*dest |= (c | CHAR_ON);

	/* Enable enhancements and color pairs from this point backwards */
	z = 1;
	while (x > 0) {
		if (*--dest&CHAR_ON) {
			dest++;
			break;
		}
		*dest |= CHAR_ON;
		z++;
		x--;
	}

	(*ite->crt_write)(ite, dest, ite->ypos, x, z);

	/* make sure all know that the cursor is gone */
	ite->cursor_on = FALSE;

	/* Move to next location if allowed.  At end of a line */
	if (ite->xpos >= ite->maxx)  {
		/* Wrap around if permitted */
		if (ite->inheolwrp == 0) {
			if (ite->ypos >= ite->maxy) { /* At end of the screen */
				scr_scroll_up();
			} else	/* Just increment row number */
				ite->ypos++;
			ite->xpos = 0;
			scr_line_include();
		}
	} else 	/* Just increment to next position */
		ite->xpos++;
}

/* Change all enhancements from current x,y
** to next set enhancement or end of line,
** whichever comes first.
**     !half bright!underline!inverse video!blinking!
*/
scr_enhance(type)
register type;
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *d;
	register i, j = 0;
	SCROLL *scroll_ptr;
	int value = 0;

	/* Make sure that current position is included in scroller */
	scr_line_include();

	/* Compute offset to position to change */
	scroll_ptr = d = crt_xy_to_scroll(ite->xpos, ite->ypos);
	if (type & 0x1) value |= BLINKING;
	if (type & 0x2) value |= INVERSE_VIDEO;
	if (type & 0x4) value |= UNDERLINE;
	if (type & 0x8) value |= HALF_BRIGHT;

	*d = (*d&(~(CHAR_ENSTART|CHAR_ENHANCE))) | value | CHAR_ENSTART;
	d++;

	i = ite->screenwidth - ite->xpos;
	/* Go to end of line or next start enhancement */
	while (++j < i) {
		/* Look for another start enhancement */
		if (*d&CHAR_ENSTART)
			break;
		/* Set enhancement */
		*d = (*d&(~(CHAR_ENSTART|CHAR_ENHANCE))) | value;
		d++;
	}
	(*ite->crt_write)(ite, scroll_ptr, ite->ypos, ite->xpos, j);
	scr_include_enh();	/* Turn on everything to the left */
}

/* Change all colorpairs from current x,y
** to next set colorpairs or end of line,
** whichever comes first.
*/
scr_colorpair(type)
register type;
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *d;
	register i, j = 0;
	SCROLL *scroll_ptr;

	/* Make sure that current position is included in scroller */
	scr_line_include();

	/* Compute offset to position to change */
	scroll_ptr = d = crt_xy_to_scroll(ite->xpos, ite->ypos);
	type <<= CPAIR_SFT;
	*d = (*d&(~CHAR_CPAIR)) | type | CHAR_CPAIRSTART;
	d++;

	i = ite->screenwidth - ite->xpos;
	/* Go to end of line or next start enhancement */
	while (++j < i) {
		/* Look for another start enhancement */
		if (*d&CHAR_CPAIRSTART)
			break;
		/* Set enhancement */
		*d = (*d&(~(CHAR_CPAIRSTART|CHAR_CPAIR))) | type;
		d++;
	}
	(*ite->crt_write)(ite, scroll_ptr, ite->ypos, ite->xpos, j);
	scr_include_enh();	/* Turn on everything to the left */
}

/* turn all enhancements to the left. */

scr_include_enh()
{
	register struct iterminal *ite = &iterminal;
	register SCROLL *dest;
	register i;

	/* Enable enhancements and color pairs from this
	** point -1 and backwards
	*/
	i = ite->xpos;
	dest = crt_xy_to_scroll(ite->xpos, ite->ypos);

	while (i) {
		if (*--dest&CHAR_ON) {
			dest++;
			break;
		}
 		*dest |= CHAR_ON;
 		--i;
	}
 	(*ite->crt_write)(ite, dest, ite->ypos, i, ite->xpos - i);
}

/*************************************************************************
** Terminal Configuration Screen Routines				**
**									**
** tf_next(type)		Move +/- to next Item.			**
** scr_cursor_tf(n,dir)		Move to field n, search with delta dir. **
** scr_value_tf(n,action)	Print value of field n.			**
** scr_tform()			Put up form for terminal config 	**
** conf_alpha(c)		Handle typing of character c.		**
*************************************************************************/

/*
** Move to next item in configuration display
** 	NEXT	= Move forward to next field (Wrapping around to field 1)
**	DISPLAY	= Just move to beginning of current field.
**	PREV	= Move back to next field (Stopping at field 1)
*/
tf_next(type)
int type;
{
	register struct iterminal *ite = &iterminal;
	register dir;

	if (ite->key_state == TCONFIG) {
		switch(type) {
		default:
		case NEXT:
			if (++ite->select >= NUMBER_OF_TFORMS)
				ite->select = 1;
			dir = 1;
			break;
		case PREV:
			if (--ite->select < 1)
				ite->select = 1;
			dir = -1;
			break;
		case DISPLAY:
			dir = 0;
			break;
		}
		scr_cursor_tf(ite->select,dir);
		return;
	}
	if (ite->key_state == SET_USER) {
		switch(type) {
		default:
		case NEXT:
			if (++ite->select >= NUMBER_OF_UFORMS)
				ite->select = 0;
			break;
		case PREV:
			if (--ite->select < 0)
				ite->select = 0;
			break;
		case DISPLAY:
			break;
		}
		set_key_tf(ite->select);
		return;
	}
}

/* Move to field n in Terminal Configure */
scr_cursor_tf(n,dir)
int n,dir;
{
	register struct tform *tf = tform;
	register struct iterminal *ite = &iterminal;
	int oldn;

	if (n<1 || n> NUMBER_OF_TFORMS)
		return;
	/* Reset character index */
	oldn = n;
	tf += n;
	/* Make sure it is a legal field */
	while (tf->skip) {
		n += dir;
		if (n<1) {
			dir=1;
			n = oldn+1;
		}
		if (n> NUMBER_OF_TFORMS)
			n=1;
		if (n==oldn)
			return;
		tf = tform;
		tf += n;
	}
	ite->sx = 0;
	ite->select = n;
	ite->cxpos = tf->fx;
	ite->cypos = tf->fy;
}


char *language_names[] = {
	"    USASCII      ",	/* K_N_USASCII */
	"  FRANCAIS qw    ",	/* K_N_FRENCHQ */
	"    DEUTSCH      ",	/* K_N_GERMAN */
	"  SVENSK/SUOMI   ",	/* K_N_SWEDISH */
	"     ESPANOL     ",	/* K_N_SPANISH */
	"    KATAKANA     ",	/* K_N_KATAKANA */
	"  FRANCAIS az    ",	/* K_N_FRENCHA */
	"    USASCII      ",	/* K_I_USASCII */
	"     VLAAMS      ",	/* K_I_BELGIAN */
	"ENGLISH CANADIAN ",	/* K_I_CANENG */
	"     DANSK       ",	/* K_I_DANISH */
	"    NEDERLANDS   ",	/* K_I_DUTCH */
	"     SUOMI       ",	/* K_I_FINNISH */
	"   FRANCAIS      ",	/* K_I_FRENCH */
	"CANADIEN FRANCAIS",	/* K_I_CANFRENCH */
	"  SUISSE ROMAND  ",	/* K_I_SWISSFRENCH */
	"    DEUTSCH      ",	/* K_I_GERMAN */
	" SCHWEIZ-DEUTSCH ",	/* K_I_SWISSGERMAN */
	"    ITALIANA     ",	/* K_I_ITALIAN */
	"     NORSK       ",	/* K_I_NORWEGIAN */
	"   ESPANOL EUR.  ",	/* K_I_EUROSPANISH */
	"   ESPANOL LAT.  ",	/* K_I_LATSPANISH */
	"    SVENSK       ",	/* K_I_SWEDISH */
	"      UK         ",	/* K_I_UNITEDK */
	"    KATAKANA     ",	/* K_I_KATAKANA */
	" SUISSE ROMAND*  ",	/* K_I_SWISSFRENCH2 */
	"SCHWEIZ-DEUTSCH* ",	/* K_I_SWISSGERMAN2 */
	"    JAPANESE     ",	/* K_I_KANJI */
};

/* Print value of field n in Terminal Configure */
/*	action:
**		NEXT	Change to Next Choice
**		DISPLAY	Just print current choice
**	        PREV	Change to Previous Choice
**		DEFAULT	Change all to Defaults (n must equal 0)
*/
scr_value_tf(n,action)
int n,action;
{
	register struct tform *tf = tform;
	register struct iterminal *ite = &iterminal;
	register struct k_keystate *k = &k_keystate;
	register char *p;
	register unsigned int c;
	register x,y;
	SCROLL scroll_b;

	tf += n;
	do {
		if (tf->skip)
			goto skipdisp;
		/* Print the Field Value */
		x = tf->fx;
		y = tf->fy;
		switch (n){
		default:
			if (action&0x3) kbd_beep();
			p = "";
			break;
		case 1:	/* FrameRate */
			if (action&0x3) kbd_beep();
			p = "60";
			break;
		case 2:	/* Language */
			switch (action) {
			case NEXT:
				if (k->type==K_NIMITZ) {
					switch (k->language_config) {
					case K_N_FRENCHQ:
					case K_N_SPANISH:
					case K_N_FRENCHA:
						if (k->flags_config&K_MUTE) {
							k->flags_config &= ~K_MUTE;
							break;
						}
					default:
						k->language_config++;
						if (k->language_config > K_N_LAST)
							k->language_config = K_N_FIRST;
						k->flags_config &= ~K_MUTE;
						k->flags_config |= mute_enable[(int)k->language_config];
						break;
					}
				} else {
					k->language_config++;
					if (k->language_config > K_I_LAST)
						k->language_config = K_I_FIRST;
					k->flags_config &= ~K_MUTE;
					k->flags_config |= mute_enable[(int)k->language_config];
				}
				break;
			case PREV:
				if (k->type==K_NIMITZ) {
					switch (k->language_config) {
					case K_N_FRENCHQ:
					case K_N_SPANISH:
					case K_N_FRENCHA:
						if (!(k->flags_config&K_MUTE)) {
							k->flags_config |= K_MUTE;
							break;
						}
					default:
						k->language_config--;
						if (k->language_config < K_N_FIRST)
							k->language_config = K_N_LAST;
						k->flags_config &= ~K_MUTE;
						k->flags_config |= mute_enable[(int)k->language_config];
						break;
					}
				} else {
					k->language_config--;
					if (k->language_config < K_I_FIRST)
						k->language_config = K_I_LAST;
					k->flags_config &= ~K_MUTE;
					k->flags_config |= mute_enable[(int)k->language_config];
				}
				break;
			case DEFAULT:
				k->language_config = k->pwr_language;
				k->flags_config &= ~K_MUTE;
				k->flags_config |= mute_enable[(int)k->language_config];
			default:
				break;
			}

			p = language_names[k->language_config];
			if (k->flags_config&K_MUTE) {
				switch (k->language_config) {
				case K_N_FRENCHQ: p="  FRANCAIS qwM   "; break;
				case K_N_SPANISH: p="   ESPANOL   M   "; break;
				case K_N_FRENCHA: p="  FRANCAIS azM   "; break;
				}
			}
			break;

		case 3:	/* ReturnDef */
			if (action&(NEXT|PREV)) kbd_beep();
			else if (action==DEFAULT) {
				ite->retdef_config[0] = '\015';
				ite->retdef_config[1] = ' ';
			}
			/* Do them as Inverse Video in Color Pair 0 */
			scroll_b = (CHAR_ON|INVERSE_VIDEO) | ite->retdef_config[0];
			config_write(ite, &scroll_b, y, x++, 1);
			scroll_b = (CHAR_ON|INVERSE_VIDEO) | ite->retdef_config[1];
			config_write(ite, &scroll_b, y, x++, 1);
			ite->cursor_on = FALSE;
			goto skipdisp;
		case 4:	/* Local Echo */
			if (action&(NEXT|PREV))	ite->local_echo_config ^=TRUE;
			else if (action==DEFAULT) ite->local_echo_config = FALSE;
			p = scr_on_off(ite->local_echo_config);
			break;
		case 5:	/* CapsLock */
			if (action&(NEXT|PREV))	k->flags_config ^= K_ASR33TTY;
			else if (action==DEFAULT) k->flags_config &= ~K_ASR33TTY;
			p = scr_on_off(k->flags_config&K_ASR33TTY);
			break;
		case 6:	/* Start Col */
			if (action&(NEXT|PREV)) kbd_beep();
			p = "01";
			break;
		case 7:	/* ASCII 8 Bits */
			if (action&(NEXT|PREV))	k->flags_config ^= K_ASCII8;
			else if (action==DEFAULT) k->flags_config &= ~K_ASCII8;
			p = scr_yes_no(k->flags_config&K_ASCII8);
			break;
		case 8:	/* XmitFnctn(A) */
			if (action&(NEXT|PREV))	ite->xmit_fnctn_config ^=TRUE;
			else if (action==DEFAULT) ite->xmit_fnctn_config = FALSE;
			p = scr_yes_no(ite->xmit_fnctn_config);
			break;
		case 10:	/* InhEolWrp(C) */
			if (action&(NEXT|PREV))	ite->inheolwrp_config ^=TRUE;
			else if (action==DEFAULT) ite->inheolwrp_config = FALSE;
			p = scr_yes_no(ite->inheolwrp_config);
			break;
		case 9:	/* SPOW(B) */
		case 12:	/* InhHndShk(G) */
		case 13:	/* Inh DC2(H) */
			if (action&(NEXT|PREV)) kbd_beep();
			p = scr_yes_no(FALSE);
			break;
		case 11:	/* Line/Page(D) */
			if (action&(NEXT|PREV)) kbd_beep();
			p = "LINE";
			break;
		case 14:	/* FldSeparator */
			if (action&(NEXT|PREV)) kbd_beep();
			else if (action==DEFAULT)
				ite->flds_config = '\037';
			/* Do it as Inverse Video in Color Pair 0*/
			scroll_b = (CHAR_ON|INVERSE_VIDEO) | ite->flds_config;
			config_write(ite, &scroll_b, y, x++, 1);
			ite->cursor_on = FALSE;
			goto skipdisp;
		case 15:	/* BlkTermnator */
			if (action&(NEXT|PREV)) kbd_beep();
			else if (action==DEFAULT)
				ite->blkt_config = '\036';
			/* Do it as Inverse Video underline in Color Pair 0 */
			scroll_b = (CHAR_ON|INVERSE_VIDEO|UNDERLINE) | ite->blkt_config;
			config_write(ite, &scroll_b, y, x++, 1);
			ite->cursor_on = FALSE;
			goto skipdisp;
		}
		/* Print it as Inverse video underline in Color Pair 0 */
		while (c = *p++) {
			scroll_b = (CHAR_ON|INVERSE_VIDEO|UNDERLINE) | c;
			config_write(ite, &scroll_b, y, x++, 1);
		}
		ite->cursor_on = FALSE;
skipdisp:
		n++;
		tf++;
	} while ((action==DEFAULT) && (n<NUMBER_OF_TFORMS));
}


/* Put up form for terminal configure mode */
scr_tform()
{
	register struct tform *tf = tform;
	register struct iterminal *ite = &iterminal;
	register char *p;
	register unsigned int c;
	register i,x,y;
	SCROLL scroll_b;

	for (i=0; i<NUMBER_OF_TFORMS; i++, tf++) {

		if (tf->skip)	/* Do not do disabled fields */
			continue;
		/* Print the Field Label */
		x = tf->lx;
		y = tf->ly;
		p = tf->label;
		/* Use Color Pair 0 */
		while (c = *p++) {
			scroll_b = CHAR_ON | c;
			config_write(ite, &scroll_b, y, x++, 1);
		}
		scr_value_tf(i,DISPLAY);
		scr_cursor_tf(1,1);
	}
}


send_num(value)
register value;
{
	register i,v,d;

	/*
	 * OK, here's the problem.  We have to send out three digits,
	 * with leading zeros.  But what if the value is greater than 999?
	 * The number is a row or column, for cursor sensing, and we can
	 * configure the kernel to have more than 999 lines.
	 *
	 * Solution: make it wider if we have to, but always send out
	 * at least three digits.
	 */

	for (i=6,d=100000; --i>=0; value%=d,d/=10) {
		v = value/d;
		if (v || i<3)
			kbd_mute_char((int)('0'+v));
	}
}

conf_alpha(c)
int c;
{
	struct iterminal *ite = &iterminal;
	int i,j, compile_bug;

	if (ite->key_state == TCONFIG) {
		j = ite->select;
		switch (j) {
		default:
			kbd_beep();
			break;
		case 3:	/* ReturnDef */
			if (ite->insert_mode && ite->sx==0)
				ite->retdef_config[1] = ite->retdef_config[0];
			ite->retdef_config[ite->sx++] = (char)c;
			scr_value_tf(j,DISPLAY);
			if (ite->sx > 1) {
				tf_next(NEXT);
				kbd_beep();
			} else {
			        (ite->cxpos)++;
			}
			break;
		case 14:	/* FldSeparator */
			ite->flds_config = (char)c;
			scr_value_tf(j,DISPLAY);
			tf_next(NEXT);
			kbd_beep();
			break;
		case 15:	/* BlkTermnator */
			ite->blkt_config = (char)c;
			scr_value_tf(j,DISPLAY);
			tf_next(NEXT);
			kbd_beep();
			break;
		}
		return;
	}
	if (ite->key_state == SET_USER) {
		j = ite->select;
		switch (j%3) {
		case 0:	/* Type */
			kbd_beep();
			break;
		case 1:	/* Label */
			if (ite->insert_mode) {
				for(i=LABEL_SIZE+1; --i>ite->sx; )
					user_label[j/3][i] = user_label[j/3][i-1];
			}
			user_label[j/3][ite->sx++] = (char)c;
			scr_value_uf(j,DISPLAY);
			if (ite->sx >= LABEL_SIZE) {
				tf_next(NEXT);
				kbd_beep();
			} else {
				if (ite->sx == LABEL_WIDTH)
					(ite->cxpos)++;
				(ite->cxpos)++;
			}
			break;
		case 2: /* Key Definition */
			if (ite->insert_mode) {
				for(i=81; --i>ite->sx; )
					user_key[j/3][i] = user_key[j/3][i-1];
			}
			if (user_key[j/3][ite->sx] == (char)0) {
				compile_bug = ite->sx + 1;
				user_key[j/3][compile_bug] = (char)0;
			}
			user_key[j/3][ite->sx++] = (char)c;
			scr_value_uf(j,DISPLAY);
			if (ite->sx >= 80) {
				tf_next(NEXT);
				kbd_beep();
			} else {
				ite->cxpos++;
				if (ite->cxpos > ite->maxx) {
					ite->cxpos = 0;
					ite->cypos++;
				}
			}
			break;
		}
		return;
	}
}

conf_right()
{
	register struct iterminal *ite = &iterminal;
	register j;

	if (ite->key_state == TCONFIG) {
		j = ite->select;
		switch (j) {
		default:
			kbd_beep();
			break;
		case 3:	/* ReturnDef */
			if (ite->sx<1) {
				ite->sx++;
				++(ite->cxpos);
			} else
				kbd_beep();
			break;
		}
		return;
	}
	if (ite->key_state == SET_USER) {
		j = ite->select;
		switch (j%3) {
		case 0:	/* Type */
			kbd_beep();
			break;
		case 1:	/* Label */
			if (ite->sx<(LABEL_SIZE-1)) {
				ite->sx++;
				if (ite->sx == LABEL_WIDTH)
					++(ite->cxpos);
				++(ite->cxpos);
			} else
				kbd_beep();
			break;
		case 2: /* Key Definition */
			if (user_key[j/3][ite->sx]) {
				ite->sx++;
				++(ite->cxpos);
			} else
				kbd_beep();
			break;
		}
		return;
	}
}

conf_left()
{
	register struct iterminal *ite = &iterminal;
	register j;

	if (ite->key_state == TCONFIG) {
		j = ite->select;
		switch (j) {
		default:
			kbd_beep();
			break;
		case 3:	/* ReturnDef */
			if (ite->sx) {
				ite->sx--;
				--(ite->cxpos);
			} else
				kbd_beep();
			break;
		}
		return;
	}
	if (ite->key_state == SET_USER) {
		j = ite->select;
		switch (j%3) {
		case 0:	/* Type */
			kbd_beep();
			break;
		case 1:	/* Label */
			if (ite->sx == LABEL_WIDTH)
				--(ite->cxpos);
		case 2: /* Key Definition */
			if (ite->sx) {
				ite->sx--;
				--(ite->cxpos);
			} else
				kbd_beep();
			break;
		}
		return;
	}
}
/*************************************************************************
** SET USER Keys Configuration Screen Routines				**
**									**
** init_softkey(key,label,str)	Set f<key+1> to label and str.		**
** set_key_dsp()		Display keys on screen to be editted.	**
** scr_value_uf(n,action)	Print value of field n in User Key Set.	**
** set_key_tf(n)		Move +/- to next Item.			**
*************************************************************************/

/* Set f <key+1> to label and string */
init_softkey(key, label, string)
char *label, *string;
{
	register struct iterminal *ite = &iterminal;
	register unsigned char *p;

	p = user_label[key];
	strncpy(p, label, LABEL_SIZE);
	strcpy(user_key[key], string);
	user_key_type[key] = 'T';
}

/* Display keys on the screen to be editted */
set_key_dsp()
{
	register struct iterminal *ite = &iterminal;
	register char *p;
	register i, m, x;
	SCROLL scroll_b;

	/* Clear screen if previous state was TCONFIG or screen is 50 wide */
	if (ite->key_state==TCONFIG) {
		i = ite->screensize + ite->screenwidth * 1;
		(*ite->crt_clear)(ite, 0, 0, i);
	}

	/* Set 50/80 wide factor */
	m = 2;

	/* Position cursor */
	set_key_tf(0);
	/* Use color pair 0 */
	for (i=0; i < NUMBER_OF_SFKS; i++) {
		/* Position to beginning of correct line */
		/* Put out "f#" */
		x = 0;
		scroll_b = 'f' | CHAR_ON;
		config_write(ite, &scroll_b, i*m, x++, 1);
		scroll_b = ('1'+i) | CHAR_ON;
		config_write(ite, &scroll_b, i*m, x++, 1);
		/* Put out "     LABEL " */
		p = "    LABEL ";
		while (*p) {
			scroll_b = *p++ | CHAR_ON;
			config_write(ite, &scroll_b, i*m, x++, 1);
		}
	}
	for (i=0; i< NUMBER_OF_UFORMS; i++)
		scr_value_uf(i,DISPLAY);
}

/* Print value of field n in User Key Set */
/*	action:
**		NEXT	Change to Next Choice
**		DISPLAY	Just print current choice
**	        PREV	Change to Previous Choice
**		DEFAULT	Change all to Defaults (n must equal 0)
*/
scr_value_uf(n,action)
int n,action;
{
	register struct iterminal *ite = &iterminal;
	register unsigned char *p;
	register x,y,f,i;
	SCROLL scroll_b;

	do {
		/* Print the Field Value */
		f = y = n/3;
		y *= 2;
		switch (n%3) {
		case 0:	/* Attribute */
			x = 3; break;
		case 1:	/* Label */
			x = 12; break;
		case 2:	/* Definition */
			x = 0; y += 1; break;
		}
		switch (n%3){
		case 0:	/* Put out Type field */
			switch (action) {
			case NEXT:
				switch(user_key_type[f]) {
				case 'T': user_key_type[f] = 'N'; break;
				case 'N': user_key_type[f] = 'L'; break;
				case 'L': user_key_type[f] = 'T'; break;
				}
				break;
			case PREV:
				switch(user_key_type[f]) {
				case 'T': user_key_type[f] = 'L'; break;
				case 'N': user_key_type[f] = 'T'; break;
				case 'L': user_key_type[f] = 'N'; break;
				}
			default:
				break;
			case DEFAULT:
				user_key_type[f] = 'T';
				break;
			}
			/* Do it as Inverse in color pair 0 */
			scroll_b = user_key_type[f] | (CHAR_ON|INVERSE_VIDEO);
			config_write(ite, &scroll_b, y, x++, 1);
			ite->cursor_on = FALSE;
			break;
		case 1: /* Put out label definition */
			if (action&(NEXT|PREV))
				kbd_beep();
			else if (action == DEFAULT)
				strcpy(user_label[f], user_defaults[f][0]);
			p = user_label[f];
			/* Do it as inverse video in color pair 7 */
			for (i=0; i<LABEL_SIZE; i++) {
				if (i == LABEL_WIDTH) x++;
				scroll_b = *p++ | (CHAR_ON|(7<<CPAIR_SFT));
				config_write(ite, &scroll_b, y, x++, 1);
			}
			ite->cursor_on = FALSE;
			break;
		case 2: /* Put out key definition */
			if (action&(NEXT|PREV))
				kbd_beep();
			else if (action == DEFAULT)
				strcpy(user_key[f], user_defaults[f][1]);
			p = user_key[f];
			/* Do it in color pair 0 */
			for (i=0; ((i<80) && (*p!=0)); i++) {
				scroll_b = *p++ | CHAR_ON;
				config_write(ite, &scroll_b, y, x++, 1);
			}
			/* Blank out rest of line */
			(*ite->crt_clear)(ite, y, i, 80-i);
			ite->cursor_on = FALSE;
			break;
		}
		n++;
	} while ((action==DEFAULT) && (n<NUMBER_OF_UFORMS));
}

/* Position cursor to correct field of set user keys display */
set_key_tf(n)
int n;
{
	register struct iterminal *ite = &iterminal;

	if (n<0 || n>= NUMBER_OF_UFORMS)
		return;
	/* Reset character index */
	ite->sx = 0;
	ite->select = n;
	ite->cypos = n/3;
	ite->cypos *= 2;
	switch (n%3) {
	case 0: ite->cxpos = 3; break; /* Attribute Field */
	case 1: ite->cxpos = 12; break; /* Label Field */
	case 2: ite->cxpos = 0; ite->cypos +=1; break; /* Definition Field */
	}
}

/* Forward copy one line */
/*	destination = end of destination plus 4 */
/*	dest_stop = start of destination */
scr_movef(dest_row, stop_row)
{
	register struct iterminal *ite = &iterminal;
	register d_stp, i, j;
	int destination = (int)(ite->scroll_buf + (ite->screenwidth*dest_row));
	int dest_stop =   (int)(ite->scroll_buf + (ite->screenwidth*stop_row));

	i = ite->screenwidth * sizeof(SCROLL);

	if ((dest_stop > destination) || (dest_stop==(int)ite->scroll_buf)) {
		/* Copy in up to three chunks */

		/* Copy chunk from current position to beginning of buffer */
		d_stp =	(int)ite->scroll_buf + i;
		if (destination > d_stp)
			bltf(destination,(destination-i),(destination-d_stp));

		/* Copy last line in buffer to start of buffer */
		j = (int)(ite->scroll_buf +
			(ite->screenwidth * ite->scroll_lines));
		if (destination != (int)ite->scroll_buf)
			bltf(d_stp, j, i);

		/* Copy rest */
		if (dest_stop != (int)ite->scroll_buf)
			bltf(j, j-i, j-dest_stop);
	} else
		/* Copy in one chunk */
		bltf(destination, destination-i, destination-dest_stop);
}

/* Backward copy one line */
/*	destination = start of destination */
/*	dest_stop = end of destination + 4 */
scr_moveb(dest_row, stop_row)
{
	register struct iterminal *ite = &iterminal;
	register d_stp, i, j;

	int destination = (int)(ite->scroll_buf + (ite->screenwidth*dest_row));
	int dest_stop =   (int)(ite->scroll_buf + (ite->screenwidth*stop_row));

	i = ite->screenwidth * sizeof(SCROLL);

	if (destination > dest_stop) {
		/* Copy in up to three chunks */

		/* Copy chunk from current position to end of buffer */
		d_stp = (int)(ite->scroll_buf +
			(ite->screenwidth * ite->scroll_lines)) - i;
		if (destination < d_stp)
			bltb(destination, destination+i, d_stp-destination);

		/* Copy first line in buffer to last line in buffer */
		j = (int)ite->scroll_buf;
		bltb(d_stp, j, i);

		/* Copy rest */
		if (j != dest_stop)
			bltb(j, j+i, dest_stop-j);
	} else
		/* Copy in one chunk */
		bltb(destination, destination+i, dest_stop-destination);
}
