/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_tiger.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:00:20 $
 */

/************************************************************************/
/*									*/
/*			   T I G E R S H A R K				*/
/*									*/
/*  This file contains the ITE support routines used to allow the ITE   */
/*  to work with the Tigershark graphics accelerator.			*/
/*									*/
/************************************************************************/

#include "../h/param.h"
#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"		       /* for CHAR_VAL and UNDERLINE */
#include "../graf.300/ite_tiger.h"

#define nSCRESET(tiger)		 ((tiger)->te_status & 0x00004000)
#define TIGER_BMOVE_WAIT(tiger)  while (tiger->wbusy)

#define LGB_HOLD     (1<<31)    /*  Write a one into the most sig. bit	     */
#define PACE_HOLD    0x1     	/*  Scan converter halts with a 1 in PACE    */

unsigned int   saved_lgb_holdoff;	/*  Save current values of these     */
unsigned short saved_pace_plug;		/*  two registers		     */

int tiger_halted = 0;		/*  Flag telling whether the Tigershark
				 *  is currently halted (1=halted).
				 */

/*  
 *  These registers are saved during ITE operations.  They are also 
 *  the ones the ITE uses during normal operation.
 */ 
char	TIG_fbwen, TIG_fold, TIG_opwen, TIG_drive, TIG_rep_rule;
short 	TIG_source_x, TIG_source_y, TIG_dest_x, TIG_dest_y, 
        TIG_wwidth, TIG_wheight;

int  	TIG_saved_reg = 0;     /*  Flag telling whether the above registers
				*  are currently saved (1=saved).
				*/

int 	TIG_xforms_present;    /*  Flag telling whether the transform engine
				*  is alive (ie., responding to writes).  If 
				*  not, we do not have to plug PACE.
				*/

tiger_check_screen_access (ite)					    /* ENTRY */
    register struct iterminal *ite;
/* 
 *  It is the job of this routine to check to see if access to the bit mapped
 *  display is allowed.  It does so by halting the Tigershark transform engine
 *  pipeline.  Once this is done, the ITE can have access to the display.  See
 *  the comment under the halt_tiger() routine below.
 */

{
    halt_tiger(ite);
    return(0);			/* we always succeed */
}


tiger_write_setup (ite)						    /* ENTRY */
    struct iterminal *ite;
/*  
 *  This routine is called to prepare the Tigershark box for writes.  It 
 *  first waits for the scan converter and block mover to cease all activity.
 *  If after 1 minute, the activity is not complete, the routine times out and 
 *  causes a reset on the graphics device.  Once the activity is stopped, 
 *  the routine saves all the registers that the ITE changes ,but that 
 *  should not be changed permanently (like OPVEN for alpha/graphics on/off).
 *  After that, several Tigershark registers are initialized.
 *
 *  This routine is basically the first routine called on the way into the 
 *  Tigershark ITE code.  Therefore, this is the routine in which the 
 *  registers are saved away (see note under tiger_cursor() below).
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    register i;

    for (i=60*1000; --i>0; ) {
	if (!tiger->as_busy && !tiger->wbusy)
	    break;

	snooze(1000);			/* wait another millisecond 	     */
    }
    if (i==0) {
	msg_printf("HP98705 has timed out: as_busy=%x wbusy=%x; \
                    resetting it.\n", tiger->as_busy, tiger->wbusy);
	tiger_pwr_reset(ite);
    }

    if (!TIG_saved_reg) {		/* did we already save the stuff?    */
	TIG_fbwen    = tiger->fbwen;
	TIG_fold     = tiger->fold;	
	TIG_opwen    = tiger->opwen;	
	TIG_drive    = tiger->drive;	
	TIG_rep_rule = tiger->rep_rule;	
	TIG_source_x = tiger->source_x;	
	TIG_source_y = tiger->source_y;
	TIG_dest_x   = tiger->dest_x;
	TIG_dest_y   = tiger->dest_y;
	TIG_wwidth   = tiger->wwidth;
	TIG_wheight  = tiger->wheight;

	TIG_saved_reg = 1;		/* do not save twice		     */
    }
	
    tiger->drive = 0x10;		/* overlay plane enable 	     */
    tiger->rep_rule = 0x33;		/* replacement rule for itecrt_write */
    tiger->opwen = ite->cur_planes;	/* typically equal to 0x7 (3 ovlys)  */
    tiger->fbwen = 0x0;			/* do not touch the frame buffer     */
    tiger->fold  = 0x1;			/* byte per pixel     		     */
    tiger->ov_blink = 0x00;		/* ensure ovly plane blinking is off */
    tiger->fv_trig  = 0x01;		/* trigger ov_blink change	     */
}


tiger_cursor (ite, x, y, color_num)				    /* ENTRY */
    register struct iterminal *ite;
/*
 *  This routine updates the cursor position.  It first checks for a busy
 *  Tigershark, then sets the pixel replacement rule to XOR.  With the
 *  replacement rule in XOR, writing a block (the cursor) to the Tigershark
 *  will cause an underlying letter to appear in inverse.  After this, the 
 *  routine is called that actually draws the cursor.
 *
 *  This routine is the last routine called on the way out of the Tigershark
 *  ITE code.  Therefore, this is the routine in which the registers are 
 *  restored to the values from the user.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);

    /* check if access to bit mapped screen hardware allowed */
    halt_tiger(ite);

    tiger_write_setup(ite);     /* set up the H/W before writing out pixels  */
    tiger->rep_rule = 0x66; 	/* set pixel replacement rule XOR 	     */

    bitmap_cursor(ite, x, y, color_num);

    /* restore registers */
    if (ite->cursor_on) {
	tiger->rep_rule = TIG_rep_rule;
	tiger->drive    = TIG_drive;
	tiger->opwen    = TIG_opwen;
	tiger->fbwen    = TIG_fbwen;
	tiger->fold     = TIG_fold;
	tiger->wheight  = TIG_wheight; 
	tiger->wwidth	= TIG_wwidth; 
	tiger->dest_x	= TIG_dest_x; 
	tiger->dest_y	= TIG_dest_y; 
	tiger->source_x = TIG_source_x; 
	tiger->source_y = TIG_source_y; 

	unhalt_tiger(ite);			/* resume transform engine   */
	TIG_saved_reg = 0;			/* registers restored 	     */

	/* 
	 * The user may have been just about to do a block move
	 * when we interrupted.  The user already waited for the
	 * block mover and is sure that it is ready for use.
	 * Hence, make sure that it is not left running.
	 */
	TIGER_BMOVE_WAIT(tiger);
    }
}


tiger_clear (ite, y, x, count)					    /* ENTRY */
    register struct iterminal *ite;
    register y, count;
/*
 *  This routine clears a specified portion of the Tigershark screen. It
 *  begins clearing the characters on the same row and to the right of
 *  the (x,y) character position passed in, and then continues clearing
 *  characters on the rows below.  Using this method, the routine clears
 *  a 'count' number of characters.
 *  
 *  The routine first checks to ensure the GPU is not busy.  It first
 *  calculates source_x and source_y, the (x,y) pixel coordinates of the
 *  starting character.  It then determines 'len', the number of
 *  characters to clear off the current row.  Since the height of all the
 *  rows to clear will be the same, the block height, 'wheight', is written
 *  to the GPU.
 *  
 *  The routine then begins to clear the screen, using the block mover.
 *  For each row, the pixel width, 'wwidth', of the block to clear is
 *  calculated and written to the GPU.  The pixel (x,y) coordinates
 *  (dest_x, dest_y) of the upper right corner of the block to clear are
 *  written to the GPU.  Since clearing a screen really just means
 *  setting the screen to the currently selected background color, there
 *  are two 'if' statements.  The first covers the case where planes need
 *  to be set (a color turned on).  The second covers the case where
 *  planes need to be cleared.  These cases loosely correspond to
 *  filling in a colored background and clearing out a black background
 *  respectively.
 *  
 *  Finally, the character 'count' is updated, along with the 'len' and
 *  upper right corner coodinates of the next row to be cleared.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    register len;
    register short source_x, source_y, width;

    /* check if access to bit mapped screen hardware allowed */
    halt_tiger(ite);

    source_x = x * ite->font_width;
    source_y = y * ite->line_height;
    len = scr_min(ite->screenwidth - x, count);

    /* set up the hardware before writing out pixels */
    tiger_write_setup(ite);

    /* height of window to move */
    tiger->wheight = ite->line_height;

    while (count) {
	count -= len;
	width = len * ite->font_width;
	len = scr_min(ite->screenwidth, count); /* length of current line    */

	TIGER_BMOVE_WAIT(tiger);

	tiger->wwidth = width;
	tiger->dest_x = source_x;
	tiger->dest_y = source_y;

	/* write enable just the planes that need setting */
	if (tiger->opwen = ite->color_pairs[0].BG & ite->cur_planes) {
	    tiger->rep_rule = 0xff; 		/* replacement rule = set    */
	    tiger->wmove = 1;	    		/* start block mover         */
	    TIGER_BMOVE_WAIT(tiger);
	}
	/* write enable just the planes that need clearing */
	if (tiger->opwen = ~ite->color_pairs[0].BG & ite->cur_planes) {
	    tiger->rep_rule = 0x00; 		/* replacement rule = clear  */
	    tiger->wmove = 1;	    		/* start block mover         */
	}

	source_x = 0; 				/* the rest start at zero    */
	source_y += ite->line_height;		/* bump to the next line     */
    }
}


tiger_block_clear (ite, x, y, height, width)			    /* ENTRY */
    register struct iterminal *ite;
    int x, y, height, width;
/*
 *  This routine simply implements a device dependent (Tigershark) block
 *  move.  In this case a block clear.
 *  
 *  The routine first checks to ensure the GPU is not busy.  It then readies
 *  the box for a block move, setting up the necessary registers.
 *
 *  The routine then performs the clear, using the hardware block mover. Since
 *  clearing a block really just means setting the block to the currently
 *  selected background color, there are two 'if' statements.  The first
 *  covers the case where planes need to be set (a color turned on).  The 
 *  second covers the case where planes need to be cleared.  These cases
 *  loosely correspond to filling in a colored background and clearing out 
 *  a black background respectively.
 */

{
    register struct tiger *tiger = (struct tiger *) ite->card;

    /* check if access to bit mapped screen hardware allowed */
    halt_tiger(ite);

    /* set up the hardware before writing out pixels */
    tiger_write_setup(ite);

    tiger->wheight = height;			/* height of window to move  */
    tiger->wwidth  = width;			/* width of window to move   */
    tiger->dest_x  = x;				/* destination for clear     */
    tiger->dest_y  = y;				/* destination for clear     */

    /* write enable just the planes that need setting */
    if (tiger->opwen = ite->color_pairs[0].BG & ite->cur_planes) {
	tiger->rep_rule = 0xff; 		/* replacement rule = set    */
	tiger->wmove = 1;	    		/* start block mover         */
	TIGER_BMOVE_WAIT(tiger); 		/* wait for block mover      */
    }
    /* write enable just the planes that need clearing */
    if (tiger->opwen = ~ite->color_pairs[0].BG & ite->cur_planes) {
	tiger->rep_rule = 0x00; 		/* replacement rule = clear  */
	tiger->wmove = 1;	    		/* start block mover         */
    }
}


tiger_scroll (ite, to_row, dir, width)				    /* ENTRY */
    register struct iterminal *ite;
/*
 *  This routine moves (scrolls) one row of characters either up one row
 *  of down one row on the screen.  
 *  
 *  The 'width' parameter contains the number of CHARACTERS on the line 
 *  to be scrolled.  A check is first made to see if the line contains
 *  anthing.  If not, the routine returns.  During this check, the
 *  variable, 'width', is reassigned the width in PIXELS of the row
 *  (block) to be moved.  Tigershark HW access is checked and the HW is
 *  prepared.  The y-coordinate of the upper left corner of the destination
 *  block is computed from the 'to_row' parameter, which contains the
 *  destination ROW number.  The block mover HW registers are then filled.
 *  The routine uses the 'dir' parameter to determine the location of the
 *  source row.  If 'dir' is UP (ie., we are scrolling a line up), the
 *  source row is the row below the destination, and vice versa.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    register short dest_y;

    /* calculate width of line to move */
    if ((width *= ite->font_width) == 0)
        return;

    /* check if access to bit mapped screen hardware allowed */
    halt_tiger(ite);
    
    dest_y = to_row * ite->line_height;

    /* set up the hardware before writing out pixels */
    tiger_write_setup(ite);

    tiger->dest_x   = 0x0;		/* set dest x pixel number 	     */
    tiger->source_x = 0x0;		/* source x pixel is same as dest    */
    tiger->dest_y   = dest_y;		/* set dest y pixel number 	     */

    /* source is one line behind or one line ahead */
    tiger->source_y = dest_y + 
                      ((dir == UP) ? ite->line_height : -ite->line_height);

    tiger->wwidth   = width;			/* width of window to move   */
    tiger->wheight  = ite->font_height;		/* height of window to move  */
    tiger->rep_rule = 0x33;			/* replacement rule = source */
    tiger->wmove    = 0x1;	    		/* start block mover         */
}


tiger_write_off (ite, vlp, y, x, count)				    /* ENTRY */
    register struct iterminal *ite;
    register SCROLL *vlp; 
    register x, count;
/*
 *  This routine write characters from the ITE scroller onto the Tigershark.
 *  It starts at the 'vlp' position in the scroller and continues to output
 *  a 'count' number of characters.  It starts placing characters at the (x,y)
 *  character coordinate passed in.
 *
 *  The routine first halts the Tigershark, then sets up the initial values
 *  for the block mover registers.  Then, for each character to be output,
 *  the routine does the following:  check to see if the color of the 
 *  current character is the same as the last;  if not, a new color table is
 *  built (this table is used to determine which planes need to be set in 
 *  a particular color, and which planes need to be cleared);  using this 
 *  color/plane information, the character is block moved twice from 
 *  offscreen, once into the planes that have to be set, and once into the
 *  planes that need clearing.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    register color_pair, scrollvalue, dest_x, clear_planes, set_planes;
    register move_case;
    int back_c, fore_c, planes=ite->cur_planes;
    struct offscreen_font offscreen_xy;
    unsigned short dest_y;

    if (count <= 0)				/* nothing to do? 	     */
        return;

    /* check if access to bit mapped screen hardware allowed */
    halt_tiger(ite);

    dest_x = ite->font_width * x;		/* calculate x pixel number  */
    dest_y = ite->line_height * y;		/* calculate y pixel number  */

    tiger_write_setup(ite); 	/* set up the H/W before writing out pixels */

    tiger->wwidth = ite->font_width;
    tiger->wheight = ite->font_height;
    tiger->dest_y = dest_y;

    /* force calculation of current color on first entry */
    color_pair = ITE_BOGUS_CPAIR;

    do {
	/* source x & y pixel depends on char in offscreen memory */
	offscreen_xy = offscreen_font[(scrollvalue = *vlp++) &CHAR_VAL];

	/* check if current color and inverse video bits changed */
	if (color_pair != char_enhance(scrollvalue)) {

	    /* if underline finish off using ram font routine */
	    if (scrollvalue & UNDERLINE) {
		itecrt_write(ite, --vlp, y, x, count);
		return;
	    }
	    /* build new pixel_color tab, exit if char not enabled*/
	    if (ite_build_pixtbl(ite, scrollvalue))
	        return;

	    /* set the colorpair to current color and enhancement */
	    color_pair = ite->current_cpair;

	    fore_c = ite->pixel_colors[15]; 	/* foreground color 	     */
	    back_c = ite->pixel_colors[0];  	/* background color 	     */

            /*           rule case 1  case 2  case 3  case 4 */
	    /*    FG =	    	0	1	0	1    */
            /*    BG =		0	0	1	1    */
	    /*	              _____   _____   _____   _____  */
	    /* ~(FG | BG)   0	1       0       0       0    */
            /* ~BG & FG	    3	0	1	0	0    */
	    /* ~FG & BG    12	0	0	1	0    */
	    /*  FG & BG    15	0	0	0	1    */

	    /* set up all planes in case simple case */
	    move_case = FALSE;
	    set_planes = back_c & planes;
	    clear_planes = ~back_c & planes;
			
	    TIGER_BMOVE_WAIT(tiger); 

	    /* set to all enabled planes as default */
	    tiger->opwen = planes;

	    /* check if just simple move rule enough */
	    if ((~back_c & fore_c & planes) == planes) {
		tiger->rep_rule = 0x33;		/* rep rule = MOVE           */
	    }
	    /* check if just simple compliment move rule enough */
	    else if ((~fore_c & back_c & planes) == planes) {
		tiger->rep_rule = 0xcc;	      	/* rep rule = CMP_MOVE       */
	    }
	    /* well we must do at least one extra move, maybe two */
	    else
	        move_case = TRUE;
	}	

	TIGER_BMOVE_WAIT(tiger); 

	/* set dest pixel to next character in line */
	tiger->dest_x = dest_x;

	if (move_case) {
	    /* check if planes to set */
	    if (tiger->opwen = set_planes) {

		tiger->rep_rule = 0xff; 		/* rule = set        */
		tiger->wmove = 1;    	      		/* start block mover */
		TIGER_BMOVE_WAIT(tiger); 
	    }
	    /* now clear non background planes, if any */
	    if (tiger->opwen = clear_planes) {
		tiger->rep_rule = 0x00;		    	/* rule=clear 	     */
		tiger->wmove = 1;	    		/* start block mover */
		TIGER_BMOVE_WAIT(tiger); 
	    }
	    /* move only changed planes from offscreen mem */
	    tiger->opwen = (fore_c ^ back_c) & planes;

	    tiger->rep_rule = 0x66;		      	/* rule=XOR 	     */
	}

	/* source x,y pixels depend on char in offscreen memory */
	tiger->source_x = offscreen_xy.X;
	tiger->source_y = offscreen_xy.Y;

	tiger->wmove = 1;	    			/* start block mover */

	/* bump to next character address for next pass */
	dest_x += ite->font_width;
	x++;
    } while (--count);
}


tiger_set_st (ite)						    /* ENTRY */
    register struct iterminal *ite;
/*
 *  This routine enables the correct overlay planes and sets the
 *  transparency level of the overlay colormaps based on the values of the
 *  GRAPHICSON and ALPHAON ITE flags.  The routine first checks to see if
 *  the colormap is flagged as currently unchangeable.  If not, the
 *  transparency and alpha plane values are determined.  If graphics is
 *  off and alpha is on, no overlay colors are transparent, and all three
 *  alpha planes are enabled. If graphics is on and alpha is off, all
 *  overlay plane colors are transparent, and no alpha planes are enabled.
 *  If both graphics and alpha are on, the color black is transparent for
 *  overlay planes, and all three alpha planes are enabled.  The
 *  routine transfers these values to the Tigershark shadow ram.  The
 *  registers are then triggered into the actual registers.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    register short alpha, transparent;
    extern int static_colormap;

    if (static_colormap)
        return;

    switch (ite->flags & (GRAPHICSON | ALPHAON)) {
	case 0: /* black */
	    transparent = 0x00;		alpha = 0x00;
	    break;
	case ALPHAON:
	    transparent = 0x00;		alpha = ite->plane_mask; 
	    break;
	case GRAPHICSON:
            transparent = 0xff;		alpha = 0x00;
	    break;
	case GRAPHICSON|ALPHAON:
	    transparent = 0x01;		alpha = ite->plane_mask; 
	    break;
    }

    tiger->opvenp = alpha;
    tiger->opvens = alpha;

    tiger->ovly0p = transparent;
    tiger->ovly0s = transparent;

    /*
     * We would also like to make the high 8 colors transparent too by 
     * writing 0xff to ovly1[ps].  However, due to a hardware error
     * inherited from daVinci, the fourth overlay plane is included in the
     * transparency decision.  Since there is junk in the 4th plane, the
     * colormap board may decide that the pixel should be transparent.
     * To get around this, we set ovly1[ps] to 0x00.  This forces the
     * upper 8 colors to always be dominant.  We rely then on the 
     * opvenp register to "mask" off the junk in the 4th plane when it 
     * actually gets displayed.
     */

    tiger->ovly1p  = 0x00;
    tiger->ovly1s  = 0x00;

    tiger->fv_trig = 0x01;
}


halt_tiger (ite)
    register struct iterminal *ite;
/*
 *  Non-transform engine type drivers (ITE, window manager, etc.)  need quick
 *  access to hardware downstream of the transform engine.  Since the transform
 *  engine was designed for maximum thruput rather than minimum latency, these
 *  drivers need access to the frame buffer without waiting for the transform
 *  engine to finish processing its queued commands.
 *
 *  This can be done simply by writing a 1 to PACE_PLUG and writing the most
 *  significant bit (31) of LGB_HOLDOFF in the transform engine(s) data RAM.
 *  This process prevents the scan converter from sending more pixels to the
 *  frame buffer and the transform engine from accessing the frame buffer or
 *  colormap hardware, respectively.  Once this is done, and the frame buffer
 *  controller is not busy (done storing current pixels or finished with a
 *  previously started block move), the plugging process may access the
 *  hardware downstream from the scan converter provided that it restores the
 *  state of certain hardware registers when finished.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);

    if (!tiger_halted) {	/* if already halted, do not stop it again   */
	tiger_halted = 1;	/* now mark it halted 			     */

	if (TIG_xforms_present && nSCRESET(tiger)) {   

	    /*
	     *  There is no need to halt the transform engine if there 	 
	     *  is not one present or if no microcode is running.
	     *
	     *  The ITE must save the the current state of the PACE plug
	     *  and LGB_HOLDOFF because it does not use any device locking
	     *  mechanism so it may be interrupting another current device
	     *  driver access to the hardware.
	     */

	    saved_lgb_holdoff = tiger->lgb_holdoff;
	    saved_pace_plug   = tiger->pace_plug;

	    tiger->lgb_holdoff = LGB_HOLD;

	    /*
	     *  Due to the overhead involved in transform engine accesses to
	     *  the LGB, 5 LGB cycles must be performed after LGB_HOLDOFF is
	     *  written to guarantee that the transform has seen LGB_HOLDOFF
	     *  if it is currently accessing the LGB. This allows a transform
	     *  engine LGB cycle to complete after the first driver LGB
	     *  access is made.
	     */
		
	    /* Plug the pace chip. */
	    tiger->pace_plug = PACE_HOLD; 
	                           /* 1st LGB cycle after LGB_HOLDOFF write */

	    /* Kill time */
	    tiger->pace_plug = PACE_HOLD;  /* 2nd LGB cycle		    */
	    tiger->pace_plug = PACE_HOLD;  /* 3rd LGB cycle		    */
	    tiger->pace_plug = PACE_HOLD;  /* 4th LGB cycle		    */
	    tiger->pace_plug = PACE_HOLD;  /* 5th LGB cycle		    */
	    tiger->pace_plug = PACE_HOLD;
	                           /* 
				    *  Once more for luck and because I like 
	                            *  plugging PACE valves
				    */
	    /*
	     *  The ITE now has to wait for hardware to complete pending 
	     *  work... this could include finishing up a previously started 
	     *  block move or flushing the currently cached pixels into the 
	     *  frame buffer.  The timeout period to allow for the worst 
	     *  case block move is approximately 300 milliseconds.  We do 
	     *  not do the wait in this routine however.  The wait actually
	     *  takes place in the tiger_write_setup() routine (see above).
	     */ 
	}
    }
}

unhalt_tiger (ite)
    register struct iterminal *ite;
/*
 *  This routine simply turns the transform engine loose again to continue
 *  where it left off when the halt_tiger() routine was called.  It restores
 *  the original values of the lgb_holdoff and pace_plug registers.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);

    if (!tiger_halted)
        panic("unhalt_tiger: not halted?");

    tiger_halted = 0;
    if (TIG_xforms_present && nSCRESET(tiger)) {
	tiger->pace_plug   = saved_pace_plug;
	tiger->lgb_holdoff = saved_lgb_holdoff;
    }
}



tiger_init (ite)
    struct iterminal *ite;
/*
 *  This routine initializes the Tigershark box for ITE use.  It first
 *  halts any activity on the GPU with a halt_tiger() call.
 *  It then calls tiger_write_setup() and tiger_set_st() to furthur
 *  initializes GPU registers.  It then loads off-screen memory with
 *  the current font and enables the colormaps to be able to display on
 *  the monitor.  It then initializes those overlay plane colormaps that 
 *  will be used by the ITE.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    register int i;
    extern char bitm_colormap[8][3];  /*  Standard colormap, from ite_driv.c */

    /* check if access to bit mapped screen hardware allowed */
    halt_tiger(ite);

    /* set up the hardware */
    tiger_write_setup(ite);
    tiger_set_st(ite);

    /* now restore offscreen memory from in ram font */
    (*ite->crt_font_restore)(ite);

    /* turn on video in case some application turned it off */
    tiger->dispen = 0x01;
    tiger->fv_trig  = 0x01;

    /* init overlay color maps 0 - 7 (the ITE only uses eight colors) */
    for (i=0; i<8; i++) {

        /* skip maps that are not currently allowed */
	if ((i & ite->cur_planes) != i)
	    continue;		
	
	tiger->oprimary[i].r   = bitm_colormap[i][0];
	tiger->osecondary[i].r = bitm_colormap[i][0];
	tiger->oprimary[i].g   = bitm_colormap[i][1];
	tiger->osecondary[i].g = bitm_colormap[i][1];
	tiger->oprimary[i].b   = bitm_colormap[i][2];
	tiger->osecondary[i].b = bitm_colormap[i][2];
    }
}


tiger_pwr_reset(ite)						    /* ENTRY */
    register struct iterminal *ite;
/*
 *  During bootup, this routine is called to do whatever Tigershark
 *  hardware preparation is necessary for the ITE.  It first resets the
 *  GPU by writing a 0x80 into the RESET register.  It then executes
 *  whatever initialization code is present in the idrom.  It then
 *  initializes other HW registers and checks to see if a transform engine
 *  is responding.  If not, we do not have to plug the PACE valve when we
 *  want access to the framebuffer (there is no transform engine with
 *  which to compete).  Finally it checks to see if the Tigershark
 *  software structure was created correctly.
 */

{
    register struct tiger *tiger = (struct tiger *)(ite->card);
    char reset_value = 0x80;

    /*
     * Reset hardware with protected copy in case it is hung.
     * Do it twice just for luck.
     */
    bcopy_prot(&reset_value, &tiger->reset, 1);
    snooze(10);				/* REMORA asserts reset for 4us      */
    bcopy_prot(&reset_value, &tiger->reset, 1);
    snooze(10);				/* REMORA asserts reset for 4us      */

    idrom_init(ite->card);		/* do what little the IDROM does     */

    tiger->dest_x = 0;
    tiger->dest_y = 0;
    tiger->wwidth = 2048;
    tiger->wheight = 1024;
    tiger->rep_rule = 0x00;
    tiger->opwen = ite->cur_planes;
    tiger->wmove = 1;	    		/* start block mover        	     */
    TIGER_BMOVE_WAIT(tiger); 

    tiger->vdrive = 0x00;	     	/* display frame buffer 0 	     */
    tiger->fv_trig = 0x01;		/* trigger vdrive change 	     */

    TIGER_BMOVE_WAIT(tiger);

    /* make sure transform engine is responding */
    TIG_xforms_present = 0;
    if (testr(&tiger->lgb_holdoff, 1)) 
        TIG_xforms_present = 1;

    /* Check the last structure entry for consistency */
    if ((int) &(((struct tiger *) 0)->lgb_holdoff) != 0x3ffffc)
        panic("error in struct tiger");
}
