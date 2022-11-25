/*
 * @(#)ite_scroll.h: $Revision: 1.4.83.4 $ $Date: 93/12/09 15:25:07 $
 * $Locker:  $
 */

#ifndef __ITE_SCROLL_H_INCLUDED
#define __ITE_SCROLL_H_INCLUDED

/*
 * Scroller commands
 */
#define ILLEGAL          0      /* illegal op, beep instead */
#define SET_CURSOR       1	/* set_cursor(x,y) */
#define SCROLL_UP        2	/* scroll_up(start_row) */
#define SCROLL_DOWN      3	/* scroll_down(start_row) */
#define CLEAR_LINE       4	/* clear_line() */
#define CURSOR_DOWN      5	/* cursor_down() */
#define CURSOR_UP        6	/* cursor_up() */
#define CURSOR_LEFT      7	/* cursor_left() */
#define CURSOR_RIGHT     8	/* cursor_right() */
#define INSERT           9	/* insert() */
#define DELETE          10	/* delete() */
#define TAB_DIR         11	/* tab(direction) */
#define SET_TAB         12	/* set_tab() */
#define CLEAR_TAB       13	/* clear_tab() */
#define CLEAR_ALL_TABS  14	/* clear_all_tabs() */
#define ALPHA_ON        15	/* alpha_on() */
#define ALPHA_OFF       16	/* alpha_off() */
#define SFK_INPUT       17	/* sfk_input(key) */
#define DSP_LABELS      18	/* display_labels(mode,force) */
#define SET_CRT_START   19	/* set_crt_start() */
#define IND_OVERRUN     20	/* Indicate display scroller overrun */
#define DELETE_LINE     21	/* delete_line(start_row) */
#define INSERT_LINE     22	/* insert_line(start_row) */
#define ROLL_UP         23	/* roll_up() */
#define ROLL_DOWN       24	/* roll_down() */
#define LF_CURSOR_DOWN  25	/* lf_cursor_down() */
#define DOWN_ARROW      26	/* down_arrow() */
#define UP_ARROW        27	/* up_arrow() */
#define PREV_PAGE       28	/* prev_page() */
#define NEXT_PAGE       29	/* next_page() */
#define AUTOLF          30	/* autolf(state) */
#define DISP_FUNCS      31	/* disp_funcs(state) */
#define REMOTE          32	/* remote(state) */
#define HOME_DOWN       33	/* home_down() */
#define HOME_UP         34	/* home_up() */
#define INSERT_MODE     35	/* insert_mode(state) */
#define CLEAR_SCREEN    36	/* clear_screen() */
#define ABS_CURSOR      37	/* abs_cursor(x,y) */
#define SCR_ENHANCE     38	/* set_enhance(type) */
#define ENHANCE         38	/* FIXME */
#define LEFTDRIVE       39	/* set_leftdrive(string)  (300 only) */
#define RIGHTDRIVE      40	/* set_rightdrive(string) (300 only) */
#define COLORPAIR       41	/* select_colorpair(num) */
#define PLANE_MASK      42	/* plane_mask(mask) */
#define SCREEN_START    43	/* screen_start(linenum) */
#define SCR_HARD_RESET  44	/* term_reset(HARD) */
#define SCR_SOFT_RESET  45	/* term_reset(SOFT) */
#define WAIT_SEC        46	/* wait_one_second() */
#define REENTER         47	/* reenter scroller after a wait one second */
#define ABS_SENSE       48	/* return absolute cursor address */
#define REL_SENSE       49	/* return relative cursor address */
#define SCR_BACKSPACE   50	/* backspace() */
#define REDRAW          51	/* redraw_screen() */
#define ALPHA_OFF_FULL  52	/* alpha_off_full() */
#define GRAPHICS_OFF    53	/* graphics_off() */
#define GRAPHICS_ON     54	/* graphics_on() */
#define CURSOR_ON       55	/* alpha cursor on */
#define CURSOR_OFF      56	/* alpha cursor off */
#define G_CURSOR_ON     57	/* graphics cursor on */
#define G_CURSOR_OFF    58	/* graphics cursor off */
#define GRAPHICS_CLEAR  59	/* graphics_clear */
#define CHAR_OUT        60      /* output a single character (s800 only) */
#define DRAW_CURSOR     61      /* draw cursor in place (turn back on)
                                                                 (s800 only) */
#define PON_RESET       62      /* restore display after powerfail
                                                                 (s800 only) */
#define SELF_ID         63      /* identify crt type to cpu (s800 only) */

#define	USER		1
#define	AIDS		2
#define	CLEAR		3
#define MODES		4
#define	MARGINS		5
#define	CONFIG		6
#define	TCONFIG		7
#define	DCONFIG		8
#define	SERVICE		9
#define	SET_USER	10
#define	FLEXDISC	11

/*
 * Fields in scroller memory entry
 */
#define	CHAR_VAL	0xFF		       /* 8 bits for character value */
#define	    HALF_BRIGHT		0x000			  /* half bright bit */
#define	    UNDERLINE		0x200			    /* underline bit */
#define	    BLINKING		0x000			     /* blinking bit */
#define	    INVERSE_VIDEO	0x100			/* inverse video bit */
#define	CHAR_ENHANCE	(HALF_BRIGHT|UNDERLINE|BLINKING|INVERSE_VIDEO)
#define	CHAR_ON		0x400		/* Enable all character features */
#define	CHAR_ENSTART	0x800		/* Start a new display enhancement */
#define	CHAR_CPAIR	0x7000		/* 3 bits for color pair number */
#define	CHAR_CPAIRSTART	0x8000		/* Start a new color pair number */
#define CPAIR_SFT	(12)		/* shift to color pair position FIXME*/
#define CPAIR_SHIFT	(12)

#define PIX_ENHANCE		(CHAR_ON|CHAR_CPAIR|CHAR_ENHANCE)
#define char_colorpair(x)	(((x) & CHAR_CPAIR)>>CPAIR_SFT)
#define char_enhance(x)		((x) & PIX_ENHANCE)
#define char_on(x)		((x) & CHAR_ON)

/*
 * Screen busy state flags for check_screen_access().
 */
#define SCR_OK		0x00
#define SCR_ALPHAOFF	0x01
#define SCR_TFORM_BUSY	0x02
#define SCR_SEMIP_LOCK	0x04
#define SCR_TFORM_BROKE	0x08
#define SCR_PFAIL	0x10
#define SCR_WAIT	0x20

#define SCR_SM_IDLE		0x00
#define SCR_SM_BUSY		0x01
#define SCR_SM_RECURSE		0x02
#define SCR_SM_PAUSE		0x03
#define SCR_SM_SCRL_TOP		0x04
#define SCR_SM_REST_SCRN	0x06
#define SCR_SM_CLR_N_REST	0x07
#define SCR_SM_RESET_N_REST	0x08
#define SCR_SM_HRD_RESET_CONT	0x09
#define SCR_SM_CLR_LINE		0x10

#ifdef __hp9000s800
    /*
     * Holds parameters to scroller
     */
    struct scr_queue_req {
        int     qaction;
        int     qparm1;
        int     qparm2;
    };
#endif

#if defined(_KERNEL) && defined(__hp9000s300)
#   include "../graf/ite.h"			       /* for NUMBER_OF_SFKS */

    /* Data exported from ite_scroll.c */
    extern char *user_defaults[NUMBER_OF_SFKS][2];

    /* Code exported from ite_scroll.c */
    extern char_switch();
#endif

#endif /* __ITE_H_SCROLL_INCLUDED */
