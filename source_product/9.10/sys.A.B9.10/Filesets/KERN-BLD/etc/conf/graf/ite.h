/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/graf/RCS/ite.h,v $
 * $Revision: 1.13.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 15:24:53 $
 */

#ifndef _GRAF_ITE_INCLUDED /* allows multiple inclusion */
#define _GRAF_ITE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL
#   if defined(__hp9000s800) || defined(__hp9000s700)
#	include "../h/types.h"
#	include "../graf/ite_scroll.h"		     /* for struct scr_queue */
#   endif
#   include "../h/tty.h"
#   include "../graf/ite_font.h"
#   include "../graf/ite_color.h"
#endif /* _KERNEL */

#define DIO_START	 (ite->card)
#define ITE_PRIMARY	 0
#define ITE_CONSOLE_UNIT 0
#ifdef __hp9000s800
#    define ITE_MAJOR	 15
#endif

#ifdef _WSIO
#    define ITE_LU_MASK		0xf00000
#    define ITE_LU_SHIFT	20
#else
#    define ITE_LU_MASK		0xff00
#    define ITE_LU_SHIFT	8
#endif /* _WSIO */

/* 2622 terminal emulation defines */

#define	HARD	1	/* hard (power on) reset */
#define	SOFT	2	/* soft reset */

#define	ITE_NO_FORCE	0		     /* Force new labels or colormap */
#define	ITE_FORCE	1
#define	FORCE		1	/* Force new labels */

#define	ITE_FORWARD     1				      /* tab forward */
#define	ITE_BACKWARD    2					 /* back tab */
#define	FORWARD		1
#define	BACKWARD	2

#define ITE_UP		1
#define ITE_DOWN	0
#define UP		1
#define DOWN		0

#ifndef TRUE
#   define TRUE		1
#   define FALSE	0
#endif

#define	ITE_TRUE	1
#define	ITE_FALSE	0

/*
 * ite->flags bit values
 */
#define	ITE_COLOR		0x08		     /* if color top present */

#ifdef __hp9000s300
#define	ALPHAON		0x00000001  /* State of top section of display */
#define	NOT_9826	0x00000002
#define SFK_ON		0x00000004  /* State of bottom section of display */
#define	GATORAID	0x00000010  /* Gatoraid present flag */
#define GRAPHICSON	0x00000020  /* State of graphics section of display */
#define BIT_MAPPED	0x00000040  /* Bit mapped top present */
#define KANA8		0x00000080  /* use kana8 charactor set */
#define	REN_TFORM_ENGN	0x00000100  /* Renais transform engine present */
#define	RENAISSANCE	0x00000200  /* Renais present flag */
#define	REN_TFORM_HALT	0x00000400  /* Renais "I" halted transform engine */
#define	REN_SAVED_REG	0x00000800  /* Renais already save registers flag */
#define CATSEYE		0x00001000  /* It's a Catseye, not a Topcat */
#define CAT_TFORM_ENGN	0x00002000  /* Fastcat transform engine is present */
#define CAT_TFORM_HALT	0x00004000  /* Fastcat has been halted by us */
#define	DAVINCI		0x00008000  /* DaVinci present flag */
#define TIGERSHARK      0x00010000  /* Tigershark present flag */
#define GENESIS_PRES   	0x00020000  /* Genesis present flag */
#define STI   		0x00040000  /* STI/SGC (Standard Text Interface) */
#endif
#if defined(__hp9000s800) || defined(__hp9000s700)
/*
 * Bit definitions for ite->flags
 */
#define	ITE_ALPHAON		0x01      /* State of top section of display */
#define ITE_KBD_PRESENT		0x02	/* If ITE has an associated keyboard */
#define ITE_SFK_ON		0x04   /* State of bottom section of display */
#define ITE_GRAPHICSON		0x20 /* State of graphics section of display */
#define ITE_BIT_MAPPED		0x40		   /* Bit mapped top present */
#define ITE_KANA8		0x80		  /* use kana8 charactor set */
#define	ITE_TFORM_ENGN_PRESENT	0x100	    /* Transform engine present flag */
#define ITE_SAVED_MODES		0x200   /* Already saved mode registers flag */
#define	ITE_TFORM_ENGN_HALTED	0x400	     /* "I" halted tform engine flag */
#define	ITE_SAVED_REG		0x800	     /* Already saved registers flag */
#define ITE_HARD_RESET_BEGUN	0x1000        /* A hard reset is in progress */
#define ITE_DMA_HALTED		0x2000	     /* Already stopped DMA accesses */
#define ITE_GRAPH_CALL_DIRECT	0x4000     /* call from graphics in progress */
#endif /* defined(__hp9000s800) || defined(__hp9000s700) */


/* SOFTKEY definitions */
#define NUMBER_OF_SFKS	8	/* number of soft keys */
#define	LABEL_SIZE 	16	/* max length of softkey label text */
#define	LABEL_WIDTH 	8	/* width of a label as displayed on screen
				   (may be broken up into two lines) */

/*
 * Values for ite->isopen
 */
#define ITE_CLOSED	    0
#define ITE_PRINTF_ONLY	    1
#define ITE_OPEN	    2

/*
 * Reasons for ite_isr() interrupt
 */
#define ITE_EXP_TIMEOUT	    1
#define ITE_EXP_BMOVE	    2
#define ITE_EXP_VERT	    3
#define ITE_EXP_XFORM_INT   4

/*
 * ITE service routine requests
 */
#define GRAPH_INT	1
#define GRAPH_MAP	2
#define GRAPH_CLOSE	3
#define COLOR_STATIC	4
#define COLOR_VARIABLE	5
#define GRAPH_POWERFAIL	6
#define GRAPH_POWER_ON	7
#define GRAPH_GCON	8
#define GRAPH_GCOFF	9
#define GRAPH_GCAON	10
#define GRAPH_GCAOFF	11
#define GRAPH_BUS_CHECK	12
#define GRAPH_GCSOFT	13

#define	ITE_TTY	ite_tty

/* ASCII character defines */
#define	FF	0x0c
#define	US	0x1f
#define	LF	0x0a
#define	CR	0x0d
#define	FS	0x1c
#define	BS	0x08
#define	TAB	0x09
#define	RING	0x07
#define	ESCAPE	0x1b
#define	DEL	0x7f
#define	NUL	0x00

#define MAX_FONT_BYTPCHAR	40		 /* max font size for calloc */

#define SCR_MAXWIDTH		256		/* max screen width in chars */
#define SCR_MAX_QLEN		5    /* key queue length is pretty arbitrary */
#define ITE_KEY_MAX_QLEN	64
#define ITE_NAMELEN		8

#define ITE_THREE_COLOR_PLANES 0x7

/*
 * Bounds and a reasonable default for the configurable
 * parameter "ite_buf_lines (a.k.a SCROLL_LINES)".
 */
#define ITE_BUFLINES_MIN	 60
#define ITE_BUFLINES_MAX	999
#define ITE_BUFLINES_DEFAULT	100

/*
 * Possible values for the "caller" parameter to ite_init().
 */
#define ITE_KBD_CALLING		0
#define ITE_VIDEO_CALLING	1
#define ITE_INITSTRUCT_CALLING	2

#define SCROLL		short

#if defined(__hp9000s800) || defined(__hp9000s700)
struct iterminal {
	unsigned int	unit;		/* unit number of this display */
	int	hpa;			/* address of hpa for this module */
	int	*romstart;		/* address of rom for this module */
	int	*ctlspace;		/* address of control registers */
	int 	*frame_start;		/* start of frame buffer for crt */
					/*   also used in ioblk_*() routines */
	struct sti_data *sti_data;	/* pointer to shared sti information */
	struct tty *ite_tty;		/* tty structure */
	struct k_keystate *k_keystate;	/* struct k_keystate pointer */
	SCROLL	*scroll_buf;		/* start address of scroll buf */
	SCROLL	*scroll_buf_end;	/* address of scroll buf end +1 */
	SCROLL  config_screen[80][24];	/* all config screens fit in this */
	unsigned char 	*font_start;	/* start of roman8 font */
#ifdef _WSIO
	unsigned char 	*kana_start;	/* start of katakana font */
#endif
	unsigned char 	*font_from_rom;	/* start of roman8 font from rom */

	/* pointers to device-dependent routines */
	int	(*ite_log_diag)();	/* pointer to logging routine */
	int	(*kbd_beep)();		/* pointer to beep routine */
	int     (*gpu_reset_func)();	/* pointer to GPU reset routine */
	int	(*graph_service)();	/* pointer to graphics request rout. */
	int	(*crt_get_fontrom)();	/* copy in font from ROM, if there */
	int	(*crt_init)();		/* pointer crt init (power down) */
	int	(*crt_save)();	    	/* pointer crt save registers */
	int	(*crt_restore)();	/* pointer crt restore registers */
	int	(*crt_write_off)();	/* pointer crt write from offscreen */
	int	(*crt_write_ram)();	/* pointer crt write from ram */
	int	(*crt_write_setup)();	/* pointer crt write setup routine */
	int	(*crt_write)();		/* pointer crt write routine */
	int	(*crt_font_draw)();	/* pointer crt font write routine */
	int	(*crt_cursor)();	/* pointer crt cursor routine */
	int	(*crt_scroll)();	/* pointer crt scroll routine */
	int	(*crt_clear)();		/* pointer crt clear routine */
	int	(*crt_reset)();		/* pointer crt reset routine */
	int	(*crt_pwr_reset)();	/* pointer power on reset routine */
	int	(*crt_font_restore)();	/* pointer font restore routine */
	int	(*crt_set_state)();	/* pointer crt colormap routine */
	int	(*crt_calc_screen)();
	int     (*crt_xform_alive)();	/* pointer to xform probing routine */
	unsigned char	(*crt_idbyte)();/* pointer crt idrom read routine */

	/* framebuffer dimensions */
	int	framebuf_width;		/* width frame buffer in pixels */
	int	framebuf_height;	/* height frame buffer in pixels */
	int	framedsp_width;		/* width framebuf display in pixels */
	int	framedsp_height;	/* height framebuf display in pixels */

	/* character font support */
	int	font_width;		/* pixels per column of font */
	int	font_height;		/* pixels per row of font */
	int	line_height;		/* vertical pixels per line spacing */
	int	font_bytpchar;		/* bytes per char in font buffer */
	struct font_def cur_font;	/* to pass font info to draw routine */
	struct font_def und_font;	/* to pass "_" font to draw routine */
	struct font_def cursor_font;	/* to pass " " font to draw routine */
	int	uline_frame_off;	/* framemem offset for underline bits*/

	int	(*ite_font_shift)();		/* font shift in/out routine */
	int	(*ite_set_charset)();		/* set base, set alternate   */
	int	active_charset;			/* currently active char set */
	int	base_charset;			/* designated base char set  */
	int	alternate_charset;		/* designated alt. char set  */
	int	default_base_charset;		/* default base char set     */
	int	default_alternate_charset;	/* default alt. char set     */
	int	char_width;
	int	char_height;

	/* color support */
	int      current_cpair;			  /* NOTE: was pix_tbl_color */
	color_pairs_t color_pairs[ITE_MAX_CPAIRS];
	int	cur_planes;		    /* current planes enabled by ite */
	int	plane_mask;			 /* max planes usable by ite */
	int	pixel_colors[16];
	int	FGxorBG[16];

	/* scroller -- screen dimensions, cursor position */
	int  	screenwidth;				/* number of columns */
	int	screenheight;				   /* number of rows */
	int	screensize;		     /* size of the display in bytes */
	int	maxx, maxy;			      /* maximum row, column */
	int	xpos, ypos;			/* row, col on crt of cursor */
	int	cxpos, cypos;		 /* row, col on crt during configure */
	int	smxpos, smypos;		/* row, col on crt for state machine */

	/* scroller -- line pointers */
	int	scroll_lines;		      /* # of lines in scroll buffer */
	int	top_line;	/* first line in Top of Full Scroller Memory */
	int	start_line;	/* Next Empty Scroller line Top of CRT Image */
	int	crt_stop_line;/* Next Empty CRT Slot for Bottom of CRT Image */
	int	bottom_line;  /* Next Empty Scroller line for Bottom of Full */
	int	first_line;		/* First absolute line on the screen */
	int	last_line;	       /* Last absolute line in the scroller */

	/* softkey intermediate processing state */
	unsigned char	*soft_ptr;	   /* pointer to partially processed */
	int	soft_type;		       /* softkey, and type (N or L) */

	/* input key queue */
	int	     key_q_len;
	unsigned int *key_q_front;
	unsigned int *key_q_back;
	unsigned int key_queue[ITE_KEY_MAX_QLEN];

	/* scroller command queue */
	int    scr_q_len;
	struct scr_queue_req *scr_q_front;
	struct scr_queue_req *scr_q_back;
	struct scr_queue_req scr_queue[SCR_MAX_QLEN];

	/* ite_output -- cblock intermediate processing state */
	struct cblock *cb_ptr;		 /* pointer to current output cblock */
	unsigned char *cb_cbuf;			 /* cblock current character */
	int	cb_num;			      /* cblock remaining characters */

	int	scr_state;		   /* scroller state machine "state" */

	/* parser state */
	char	pstate;					  /* State of parser */
	int	pint1, pint2;	   /* Parser temporaries - FIXME(need these?)*/

	/* parser internal variables */
	int ll, sl;
	int strsize, llen;
	int row, column;	
	int ScreenRelative; /* AGA */
	int attr, key;
	int cur_rel;
	int color_a, color_b, color_c, color_x, color_y, color_z, redraw;
	int num, point;
	int negate;

	/* Misc globals */
	int	select;			/* index into terminal configuration */
	int	sx;			  /* index into configuration string */
	unsigned flags;

	/* Terminal State */
	char	flds;					  /* Field separator */
	char	blkt;					 /* Block terminator */
	char	retdef[2];				/* Return definition */
	char	dsp_funcs;		      /* Configure display functions */
	char	disp_funcs;			 /* Normal display functions */
	char	local_echo;
	char	xmit_fnctn;
	char	remote_mode;
	char	insert_mode;
	char	autolf_mode;
	char	escape;
	char	inheolwrp;
	unsigned char *tabstops;			/* tab stop settings */
	char	key_state;			      /* state of f1 thru f8 */
	char	last_labels;	    /* State of f1 thru f8 previous to going
						         to a configure mode */

	/* Flags for interface with dev framebuf */
	char	ite_busy;
	char	ite_memory_used;
	char	ite_memory_trashed;
	char	static_colormap;

	/* Powerfail flags */
	char	power_down;
	char	power_failed;
	char	pon_queued;

	/* More misc globals */
	char	int_pending;		  /* graphics int or timeout pending */
	char	int_late;	     /* interrupt has been pending long time */
	char	output_busy;			/* Recursion prevention flag */

	char	overlay_planes;		       /* HP98720 has overlay planes */
	char	c_mode;					       /* color mode */
	char	cursor_on;

	char	isopen;		 /* ITE_OPEN, ITE_CLOSED, or ITE_PRINTF_ONLY */
	char	configured;

	/* location of offscreen chars in frame memory */
	struct offscreen_font offscreen_font[ITE_NGLYPHS];

	unsigned short char_out[ITE_NGLYPHS];

	char	*name;			      /* device name for crt self-id */

	/* softkey support */
	int	sfkheight;		     /* Number of lines of SFK label */
	unsigned char **aids_label;
	unsigned char **marg_label;
	unsigned char **conf_label;

	unsigned char user_key_type[NUMBER_OF_SFKS];	  /* Writable softkey arrays */
	unsigned char *modes_label[NUMBER_OF_SFKS];
	unsigned char *user_key[NUMBER_OF_SFKS];
	unsigned char *user_label[NUMBER_OF_SFKS];
	unsigned char *suser_label[NUMBER_OF_SFKS];
	unsigned char *tconf_label[NUMBER_OF_SFKS];
	unsigned char labelbuf[LABEL_SIZE+1];
	unsigned char strbuf[81];

	unsigned char user_key_a[NUMBER_OF_SFKS][81];
	unsigned char user_label_a[NUMBER_OF_SFKS][LABEL_SIZE+1];
	unsigned char modes_label_a[NUMBER_OF_SFKS][LABEL_SIZE+1];
	unsigned char suser_label_a[NUMBER_OF_SFKS][LABEL_SIZE+1];
	unsigned char tconf_label_a[NUMBER_OF_SFKS][LABEL_SIZE+1];

#ifdef _WSIO
	int	sti_ite_saverest_start, sti_ite_saverest_retries,
	        sti_ite_save_start;
#endif

	int	hw_type;		/* S9000_ID identifier */
	char	*ite_dd_info;		/* DD device information */
};
#endif /* defined(__hp9000s800) || defined(__hp9000s700) */

#ifdef __hp9000s300
struct iterminal {
     int unit;
     struct tty *ite_tty;	/* tty structure */
     struct k_keystate *k_keystate;
     short *diostart;		/* select code address of controller */
     unsigned char *card;	/* start of card */
     unsigned char *frame_start;/* start of frame buffer for crt */

     SCROLL *scroll_buf;	/* start address of scroll buf */
     SCROLL *scroll_buf_rom;	/* start address of scroll buf from boot */
     int scroll_lines;		/* number of lines in scroll buffer */
     int scroll_lines_boot;	/* number of lines in boostrap scroll buffer */
 
     int (*crt_write_off)();	/* pointer crt write from offscreen */
     int (*crt_write_ram)();	/* pointer crt write from ram */
     int (*crt_write_setup)();	/* pointer to crt write setup routine */
     int (*crt_write)();	/* pointer to crt write routine */
     int (*crt_font_draw)();	/* pointer to crt font write routine */
     int (*crt_cursor)();	/* pointer to crt cursor routine */
     int (*crt_scroll)();	/* pointer to crt scroll routine */
     int (*crt_clear)();	/* pointer to crt clear routine */
     int (*crt_block_clear)();  /* pointer to block clear routine */
     int (*crt_reset)();	/* pointer to crt reset routine */
     int (*crt_pwr_reset)();	/* pointer to power on reset routine */
     int (*crt_font_restore)();	/* pointer to font restore routine */
     int (*crt_set_state)();	/* pointer to crt junk routine */
     int (*crt_big_scroll)();	/* pointer to full-screen scroller */
     int (*crt_chk_screen1)();	/* pointer to check_screen_access() hook #1 */
     int (*crt_chk_screen2)();	/* pointer to check_screen_access() hook #2 */

     int framebuf_width;	/* width frame buffer in pixels */
     int framebuf_height;	/* height frame buffer in pixels */
     int framedsp_width;	/* width framebuf display in pixels */
     int framedsp_height;	/* height framebuf  display in pixels */
     int offscrn_max_width;     /* horiz edge of offscreen in pixels */
     int offscrn_max_height;    /* vert edge of offscreen in pixels */

     unsigned char *font_start;		/* start of roman8 font */
     unsigned char *font_from_rom;	/* start of roman8 font from rom */
     int font_width;		/* pixels per column of font */
     int font_height;		/* pixels per row of font */
     int line_height;		/* vertical pixels per line spacing */
     int font_bytpchar;		/* chars per charactor in font buffer */
     struct font_def cur_font;	/* to pass font info to draw routine */
     struct font_def und_font;	/* to pass "_" font to draw routine */
     struct font_def cursor_font;	/* to pass " " font to draw routine */
     int uline_frame_off;	/* frame mem offset for underline bits*/

     int current_cpair;		/* current color in pixel_colors[] */
     color_pairs_t color_pairs[ITE_MAX_CPAIRS];
     int cur_planes;		/* current planes enabled by ite */
     int plane_mask;
     int pixel_colors[16];
     int FGxorBG[16];		/* define a font for the cursor */

     int screenwidth;	        /* number of columns */
     int screenheight;		/* number of rows */
     int screensize;		/* size of the display in bytes */
     int maxx, maxy;		/* maximum row, column */
     int xpos, ypos;		/* row, col on crt of cursor */
     int cxpos, cypos;		/* row, col on crt during configure */
     int rxpos, rypos;		/* row, col on crt of missed output */

     int top_line;	    /* First line in Top of Full Scroller Memory */
     int start_line;	    /* Next Empty Scroller line Top of CRT Image*/
     int crt_stop_line;	    /* Next Empty CRT Slot for Bottom of CRT Image*/
     int bottom_line;	    /* Next Empty Scroller line for Bottom of Full*/
     int first_line;	    /* First absolute line on the screen */
     int last_line;	    /* Last absolute line in the scroller */

     int select;		/* Index into Terminal Configuration */
     int sx;			/* Index into Configuration String */
     unsigned flags;

     unsigned char flds;	/* Field separator */
     unsigned char flds_config;	/* Temporary value for config screen */	
     unsigned char blkt;	/* Block terminator */
     unsigned char blkt_config;	/* Temporary value for config screen */
     unsigned char retdef[2];	/* Return definition */
     unsigned char retdef_config[2];/* Temporary value for config screen */

     char dsp_funcs;		/* Save of Terminal Configure Display funcs */
     char disp_funcs;
     char local_echo;
     char local_echo_config;	/* Temporary value for config screen */
     char xmit_fnctn;
     char xmit_fnctn_config;	/* Temporary value for config screen */
     char remote_mode;
     char insert_mode;
     char autolf_mode;
     char escape;
     char inheolwrp;
     char inheolwrp_config;		/* temporary value for config screen */
     char c_mode;
     char cursor_on;
     char reverse_copy;
     char missed_output;		/* output missing on screen flag */
     char overlay_planes;

     unsigned char *tabstops;			/* tab stop settings */
     char key_state;		/* State of f1 thru f8 */
     char last_labels;		/* State of f1 thru f8 previous to going
						         to a configure mode */
     /* parser state variables */
     char pstate;
     int pint1, pint2;

     /* parser internal variables */
     char labelbuf[LABEL_SIZE+1];
     char strbuf[81];
     int ll, sl, llen, strsize;
     int row, column;	
     int ScreenRelative;
     int attr, key, cur_rel;
     int color_a, color_b, color_c, color_x, color_y, color_z, redraw;
     int num, point, negate;
     int sfkheight;

     unsigned short char_out[ITE_NGLYPHS];

     /* Flags for interface with dev framebuf */
     char ite_memory_used, ite_memory_trashed;
     char static_colormap;			   /* ok to change colormap? */

     int time_out_count;		/* semaphore timeout counter */
};
#endif /* __hp9000s300 */

/*
 * Series 800 Diagnostic logging defines
 */
#define ITE_FROM_DAVSAVE	  0x200
#define ITE_FROM_DAVRESTORE	  0x201
#define ITE_FROM_RENSAVE	  0x202
#define ITE_FROM_FEYSAVE	  0x203
#define ITE_FROM_UNQUEUE	  0x204
#define ITE_FROM_SCROLLERINIT  0x205
#define ITE_FROM_SCROLLER	  0x206

#ifdef _KERNEL
#   ifdef __hp9000s300
	extern (*ite_bitmap_call)(), (*ite_service_call)();
	extern struct iterminal iterminal;
	extern char ite_bmap_been_locked, ite_bmap_semi_lock;
#   endif

#   if defined(__hp9000s800) || defined(__hp9000s700)
	extern int ite_graph_service(), ite_beep(), null_func();

#	define GWRD(rombase, x) (((*ite->crt_idbyte)(rombase, x)   << 24) | \
				 ((*ite->crt_idbyte)(rombase, x+4) << 16) | \
				 ((*ite->crt_idbyte)(rombase, x+8) << 8)  | \
				 ((*ite->crt_idbyte)(rombase, x+12)))

#	define GHLFWRD(rombase, x) (((*ite->crt_idbyte)(rombase, x) << 8) | \
				    ((*ite->crt_idbyte)(rombase, x+4)))
#   endif
#endif /* _KERNEL */

#ifdef _WSIO
#ifdef __hp9000s800
#   define MAX_ITE	2
#endif
#else
#   define MAX_ITE	4
#endif /* _WSIO */

#endif /* ! _GRAF_ITE_INCLUDED */
