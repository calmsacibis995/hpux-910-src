/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_gen.c,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:59:21 $
 */

/************************************************************************/
/*									*/
/*			      G E N E S I S				*/
/*									*/
/*  This file contains the ITE support routines used to allow the ITE   */
/*  to work with the Genesis graphics accelerator.			*/
/*									*/
/************************************************************************/

#include "../h/param.h"
#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"		       /* for CHAR_VAL and UNDERLINE */
#include "../graf.300/ite_gen.h"

extern int static_colormap;

/*
 *  Genesis registers must be read and written using word (32 bit) aligned
 *  addresses.  As a result of this and the fact that 68K bit operations are
 *  performed using byte address accesses, we cannot use bit fields to specify
 *  individual bits in the FBC status word.  Several macros are defined below
 *  to help with the bit manipulation.
 */
#define  PIPE_PLUG(genesis)        ((genesis)->FBCStatus & 0x00000010)
#define  BM_BUSY(genesis)          ((genesis)->FBCStatus & 0x00000004)
#define  PIPE_PLUG_REQ(genesis)    ((genesis)->FBCStatus & 0x00000001)

#define  GEN_REG_SET		0x0   		/* which of the 32 reg sets  */
#define  GENESIS_BMOVE_WAIT(genesis)  while (BM_BUSY(genesis))
#define  GENESIS_PIPE_WAIT(ite, genesis, goal)	 			\
{									\
    register i; 						 	\
    for (i=60*1000; --i>0; ) {						\
	if ((PIPE_PLUG(genesis)>>4) == (goal)) 				\
	    break; 							\
	snooze(1000); 							\
    }									\
    if (i==0) { 							\
	msg_printf("Genesis has timed out: PipePlug=%x resetting it.\n",\
		   PIPE_PLUG(genesis)); 				\
	genesis_pwr_reset(ite); 					\
    } 									\
}


/*  
 *  The following registers are saved during ITE operations.  They are also 
 *  the ones the ITE uses during normal operation.
 */ 

struct {
    int  PXC_PWE;
    int  PXC_PRE;
} gen_saved_image_pixel_cache[3];

#define GEN_BLUE	0
#define GEN_GREEN	1
#define GEN_RED		2

struct {
    int  PXC_PWE, PXC_PRE;
    int  PXC_AROP, PXC_DATAPATH, PXE_COMBMODE, PXC_GAMMA, PXC_DITHER,
         PXC_BPP, PXC_DISPMODE, PXC_ZCOMP, PXC_SRCCOMP, PXC_DESTCOMP;
    int  PXC_RR0, PXC_RR1, PXC_RR2, PXC_RR3, PXC_RR4, PXC_RR5, PXC_RR6,
         PXC_RR7, PXC_BROP;
} gen_saved_ovly_pixel_cache;

struct {
    int FrameBufferAccess, GbusWidth, TMConfig, ZConfig, BROPConfig, 
        WindowID, WindowOffsetX, WindowOffsetY, TConfig;
    int sourceX, sourceY, destX, destY, width, height;
} gen_saved_fb_ctrlr;


/*
 *  The following variable is a flag telling whether the above registers
 *  are currently saved (1=saved).
 */
int gen_saved_reg = 0;

/*
 *  The following variable saves the value of the plug request register
 *  on the way into the ITE code.  This way it can be restored to what the
 *  user thinks is there on exit from ITE code.  It is important that this be
 *  initialiazed to 0x0, so that we do not leave the pipeline plugged on 
 *  our way out the first time.
 */
unsigned int gen_saved_plug_req = 0x0;		

/*  
 *  The following variable tells whether the Genesis is currently halted,
 *  (i.e., the pipeline is plugged (1=halted)).
 */
int genesis_halted = 0;		



genesis_check_screen_access (ite)
    register struct iterminal *ite;
/* 
 *  It is this routine's job to check to see if access to the bit mapped
 *  display is allowed.  It does so by halting the transform engines'
 *  pipelines.  Once this is done, the ITE can have access to the display. 
 *  See the comment under the genesis_halt() routine below.
 */

{
    genesis_halt(ite);
    return(0);			/* we always succeed */
}


genesis_save_regs (genesis)
    register struct genesis *genesis;
/*
 *  This routine saves all the genesis registers that the ITE will use 
 *  during its processing.  It is important that the pixel cache registers
 *  be saved and restored in the proper order.
 */

{
    gen_saved_fb_ctrlr.FrameBufferAccess = genesis->FrameBufferAccess;

    genesis->FrameBufferAccess = 0x11;   	/* blue pixel cache enable   */
    gen_saved_image_pixel_cache[GEN_BLUE].PXC_PWE = genesis->PXC_PWE;
    gen_saved_image_pixel_cache[GEN_BLUE].PXC_PRE = genesis->PXC_PRE;
    genesis->FrameBufferAccess = 0x12;   	/* green pixel cache enable  */
    gen_saved_image_pixel_cache[GEN_GREEN].PXC_PWE = genesis->PXC_PWE;
    gen_saved_image_pixel_cache[GEN_GREEN].PXC_PRE = genesis->PXC_PRE;
    genesis->FrameBufferAccess = 0x14;   	/* red pixel cache enable    */
    gen_saved_image_pixel_cache[GEN_RED].PXC_PWE = genesis->PXC_PWE;
    gen_saved_image_pixel_cache[GEN_RED].PXC_PRE = genesis->PXC_PRE;

    genesis->FrameBufferAccess = 0x51;   	/* ovly pixel cache enable   */
    gen_saved_ovly_pixel_cache.PXC_PWE = genesis->PXC_PWE;
    gen_saved_ovly_pixel_cache.PXC_PRE = genesis->PXC_PRE;
    gen_saved_ovly_pixel_cache.PXC_AROP = genesis->PXC_AROP;
    gen_saved_ovly_pixel_cache.PXC_DATAPATH = genesis->PXC_DATAPATH;
    gen_saved_ovly_pixel_cache.PXE_COMBMODE = genesis->PXE_COMBMODE;
    gen_saved_ovly_pixel_cache.PXC_GAMMA = genesis->PXC_GAMMA;
    gen_saved_ovly_pixel_cache.PXC_DITHER = genesis->PXC_DITHER;
    gen_saved_ovly_pixel_cache.PXC_BPP = genesis->PXC_BPP;
    gen_saved_ovly_pixel_cache.PXC_DISPMODE = genesis->PXC_DISPMODE;
    gen_saved_ovly_pixel_cache.PXC_ZCOMP = genesis->PXC_ZCOMP;
    gen_saved_ovly_pixel_cache.PXC_SRCCOMP = genesis->PXC_SRCCOMP;
    gen_saved_ovly_pixel_cache.PXC_DESTCOMP = genesis->PXC_DESTCOMP;
    gen_saved_ovly_pixel_cache.PXC_RR0 = genesis->PXC_RR0;
    gen_saved_ovly_pixel_cache.PXC_RR1 = genesis->PXC_RR1;
    gen_saved_ovly_pixel_cache.PXC_RR2 = genesis->PXC_RR2;
    gen_saved_ovly_pixel_cache.PXC_RR3 = genesis->PXC_RR3;
    gen_saved_ovly_pixel_cache.PXC_RR4 = genesis->PXC_RR4;
    gen_saved_ovly_pixel_cache.PXC_RR5 = genesis->PXC_RR5;
    gen_saved_ovly_pixel_cache.PXC_RR6 = genesis->PXC_RR6;
    gen_saved_ovly_pixel_cache.PXC_RR7 = genesis->PXC_RR7;

    gen_saved_fb_ctrlr.TMConfig = genesis->TMConfig;
    gen_saved_fb_ctrlr.ZConfig = genesis->ZConfig;
    gen_saved_fb_ctrlr.BROPConfig = genesis->BROPConfig;
    gen_saved_fb_ctrlr.WindowID = genesis->WindowID;

    genesis->WindowID = GEN_REG_SET;   	       /* just pick one, but restore */
    gen_saved_fb_ctrlr.WindowOffsetX = genesis->WindowOffsetX;
    gen_saved_fb_ctrlr.WindowOffsetY = genesis->WindowOffsetY;
    gen_saved_fb_ctrlr.TConfig = genesis->TConfig;
    gen_saved_fb_ctrlr.GbusWidth = genesis->GbusWidth; 
    gen_saved_fb_ctrlr.sourceX = genesis->SourceX;	
    gen_saved_fb_ctrlr.sourceY = genesis->SourceY;
    gen_saved_fb_ctrlr.destX = genesis->DestX;
    gen_saved_fb_ctrlr.destY = genesis->DestY;
    gen_saved_fb_ctrlr.width = genesis->Width;
    gen_saved_fb_ctrlr.height = genesis->Height;
    
    gen_saved_reg = 1;			/* don't save twice 		     */
}



genesis_restore_regs (genesis)
    register struct genesis *genesis;
/*
 *  This routine restores all the genesis registers that the ITE uses
 *  during its processing.  It is important that the pixel cache registers
 *  be saved and restored in the proper order.
 */

{
    genesis->FrameBufferAccess = 0x11;
    genesis->PXC_PWE = gen_saved_image_pixel_cache[GEN_BLUE].PXC_PWE;
    genesis->PXC_PRE = gen_saved_image_pixel_cache[GEN_BLUE].PXC_PRE;
    genesis->FrameBufferAccess = 0x12;
    genesis->PXC_PWE = gen_saved_image_pixel_cache[GEN_GREEN].PXC_PWE;
    genesis->PXC_PRE = gen_saved_image_pixel_cache[GEN_GREEN].PXC_PRE;
    genesis->FrameBufferAccess = 0x14;
    genesis->PXC_PWE = gen_saved_image_pixel_cache[GEN_RED].PXC_PWE;
    genesis->PXC_PRE = gen_saved_image_pixel_cache[GEN_RED].PXC_PRE;

    genesis->FrameBufferAccess = 0x51;
    genesis->PXC_PWE = gen_saved_ovly_pixel_cache.PXC_PWE;
    genesis->PXC_PRE = gen_saved_ovly_pixel_cache.PXC_PRE;
    genesis->PXC_AROP = gen_saved_ovly_pixel_cache.PXC_AROP;
    genesis->PXC_DATAPATH = gen_saved_ovly_pixel_cache.PXC_DATAPATH;
    genesis->PXE_COMBMODE = gen_saved_ovly_pixel_cache.PXE_COMBMODE;
    genesis->PXC_GAMMA = gen_saved_ovly_pixel_cache.PXC_GAMMA;
    genesis->PXC_DITHER = gen_saved_ovly_pixel_cache.PXC_DITHER;
    genesis->PXC_BPP = gen_saved_ovly_pixel_cache.PXC_BPP;
    genesis->PXC_DISPMODE = gen_saved_ovly_pixel_cache.PXC_DISPMODE;
    genesis->PXC_ZCOMP = gen_saved_ovly_pixel_cache.PXC_ZCOMP;
    genesis->PXC_SRCCOMP = gen_saved_ovly_pixel_cache.PXC_SRCCOMP;
    genesis->PXC_DESTCOMP = gen_saved_ovly_pixel_cache.PXC_DESTCOMP;
    genesis->PXC_RR0 = gen_saved_ovly_pixel_cache.PXC_RR0;
    genesis->PXC_RR1 = gen_saved_ovly_pixel_cache.PXC_RR1;
    genesis->PXC_RR2 = gen_saved_ovly_pixel_cache.PXC_RR2;
    genesis->PXC_RR3 = gen_saved_ovly_pixel_cache.PXC_RR3;
    genesis->PXC_RR4 = gen_saved_ovly_pixel_cache.PXC_RR4;
    genesis->PXC_RR5 = gen_saved_ovly_pixel_cache.PXC_RR5;
    genesis->PXC_RR6 = gen_saved_ovly_pixel_cache.PXC_RR6;
    genesis->PXC_RR7 = gen_saved_ovly_pixel_cache.PXC_RR7;

    genesis->FrameBufferAccess = gen_saved_fb_ctrlr.FrameBufferAccess;
    genesis->TMConfig = gen_saved_fb_ctrlr.TMConfig;
    genesis->ZConfig = gen_saved_fb_ctrlr.ZConfig;
    genesis->BROPConfig = gen_saved_fb_ctrlr.BROPConfig;

    genesis->WindowID = GEN_REG_SET;
    genesis->WindowOffsetX = gen_saved_fb_ctrlr.WindowOffsetX;
    genesis->WindowOffsetY = gen_saved_fb_ctrlr.WindowOffsetY;
    genesis->TConfig = gen_saved_fb_ctrlr.TConfig;
    genesis->GbusWidth = gen_saved_fb_ctrlr.GbusWidth;
    genesis->SourceX = gen_saved_fb_ctrlr.sourceX;
    genesis->SourceY = gen_saved_fb_ctrlr.sourceY;
    genesis->DestX = gen_saved_fb_ctrlr.destX;
    genesis->DestY = gen_saved_fb_ctrlr.destY;
    genesis->Width = gen_saved_fb_ctrlr.width;
    genesis->Height = gen_saved_fb_ctrlr.height;
    genesis->WindowID = gen_saved_fb_ctrlr.WindowID;
    
    gen_saved_reg = 0;		/* registers restored 	*/
}



genesis_write_setup (ite)
    struct iterminal *ite;
/*  
 *  This routine is called to prepare the Genesis box for writes.  It 
 *  first waits for the pipeline and block mover to cease all activity.
 *  If after 1 minute, the activity is not complete, the routine times out and 
 *  causes a reset on the graphics device.  Once the activity is stopped, 
 *  the routine saves all the registers that the ITE changes, but that 
 *  should not be changed permanently.  After that, several Genesis registers
 *  are initialized.
 *
 *  This routine is basically the first routine called on the way into the 
 *  Genesis ITE code.  Therefore, this is the routine in which the 
 *  registers are saved away (see note under genesis_cursor() below).
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    register i;

    for (i=60*1000; --i>0; ) {
	if (PIPE_PLUG(genesis) && !BM_BUSY(genesis))
	    break;

	snooze(1000);			/* wait another millisecond */
    }
    if (i==0) {
	msg_printf("Genesis has timed out: PipePlug=%x BmBusy=%x; \
                    resetting it.\n", PIPE_PLUG(genesis), BM_BUSY(genesis));
	genesis_pwr_reset(ite);
    }

    if (!gen_saved_reg) {		/* did we already save the stuff? */
	genesis_save_regs(genesis);
    }

    genesis->FrameBufferAccess = 0x17;	/* enable all 3 image pixel caches   */
    genesis->PXC_PWE = 0x0;		/* don't write into any image planes */
    genesis->PXC_PRE = 0x0;		/* don't read from any image planes  */
    genesis->FrameBufferAccess = 0x51;	/* overlay plane enable, byte mode   */
    genesis->PXC_PWE = ite->cur_planes;	/* typically equal to 0x7 (3 ovlys)  */
    genesis->PXC_PRE = 0x0;		/* don't read from any planes        */
    genesis->PXC_AROP = 0x2;		/* who knows why?  but do it.	     */
    genesis->PXC_DATAPATH = 0x0;	/* take src input from pixel port    */
    genesis->PXE_COMBMODE = 0x0;	/* set many pixel cache bit reg to 0 */
    genesis->PXC_GAMMA = 0x0;		/* disable gamma correction	     */
    genesis->PXC_DITHER = 0x0;		/* disable dithering		     */
    genesis->PXC_BPP = 0x0;		/* disable bit per pixel	     */
    genesis->PXC_DISPMODE = 0x0;     	/* do not force low ovly bits to 0   */
    genesis->PXC_ZCOMP = 0x0;		/* disable Z compare		     */
    genesis->PXC_SRCCOMP = 0xff;	/* disable clip plane src compare    */
    genesis->PXC_DESTCOMP = 0xff;	/* disable clip plane dest compare   */
    genesis->PXC_BROP = 0x33;		/* replacement rule for itecrt_write */

    genesis->TMConfig = 0x0;		/* disable texture mapping	     */
    genesis->ZConfig = 0x0;		/* disable Z buffering	       	     */
    genesis->BROPConfig = 0x0;		/* screen relative		     */

    genesis->WindowID = GEN_REG_SET;	/* just pick one, but restore 	     */
    genesis->WindowOffsetX = 0x0;	/* set origin offset		     */
    genesis->WindowOffsetY = 0x0;       /* set origin offset		     */
    genesis->GbusWidth = 0x3;		/* byte per pixel     		     */
    genesis->TConfig = 0x30;		/* do not hard clip the screen,      */
					/* put ovlys bits in low nibble      */
}


genesis_cursor (ite, x, y, color_num)
    register struct iterminal *ite;
/*
 *  This routine updates the cursor position.  It first checks for a busy
 *  Genesis, then sets the pixel replacement rule to XOR.  With the
 *  replacement rule in XOR, writing a block (the cursor) to the Genesis
 *  will cause an underlying letter to appear in inverse.  After this, the 
 *  routine is called that actually draws the cursor.
 *
 *  This routine is the last routine called on the way out of the Genesis
 *  ITE code.  Therefore, this is the routine in which the registers are 
 *  restored to the user's values.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;

    genesis_halt(ite);		/* halt the pipeline before doing anything   */
    genesis_write_setup(ite);   /* set up the H/W before writing out pixels  */

    genesis->PXC_BROP = 0x66; 	/* set pixel replacement rule XOR 	     */

    /*  
     *  We do not set BROPConfig to 0x1 and PWE_DATHPATH to 0x1 here because
     *  bitmap_cursor always uses crt_font_draw() which gets its source from
     *  system memory not from offscreen.
     */
    bitmap_cursor(ite, x, y, color_num);

    if (ite->cursor_on) {
	/* 
	 *  The user may have been just about to do a block move when we 
	 *  interrupted.  The user already waited for the block mover and 
	 *  is sure that it's ready for use.  Hence, make sure that it's not 
	 *  left running.
	 */
	GENESIS_BMOVE_WAIT(genesis);	/* wait for block mover */

	genesis_restore_regs(genesis);
	genesis_unhalt(ite);		/* resume transform engine */
    }
}



genesis_clear (ite, y, x, count)
    register struct iterminal *ite;
    register y, count;
/*
 *  This routine clears a specified portion of the Genesis screen. It
 *  begins clearing the characters on the same row and to the right of
 *  the (x,y) character position passed in, and then continues clearing
 *  characters on the rows below.  Using this method, the routine clears
 *  a 'count' number of characters.
 *  
 *  The routine first checks to ensure the GPU is not busy.  It first
 *  calculates source_x and source_y, the (x,y) pixel coordinates of the
 *  starting character.  It then determines 'len', the number of
 *  characters to clear off the current row.  Since the height of all the
 *  rows to clear will be the same, the block height, 'Height', is written
 *  to the GPU.
 *  
 *  The routine then begins to clear the screen, using the block mover.
 *  For each row, the pixel width, 'width', of the block to clear is
 *  calculated and written to the GPU.  The pixel (x,y) coordinates
 *  (DestX, DestY) of the upper right corner of the block to clear are
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
    register struct genesis *genesis = (struct genesis *) ite->card;
    register len, clear_planes, set_planes;
    register short source_x, source_y, width;

    genesis_halt(ite);	       /* halt the pipeline before doing anything   */
    genesis_write_setup(ite);   /* set up the H/W before writing out pixels */

    source_x = x * ite->font_width;
    source_y = y * ite->line_height;
    len = scr_min(ite->screenwidth - x, count);

    /* height of window to move */
    genesis->Height = ite->line_height;

    while (count) {
	count -= len;
	width = len * ite->font_width;
	len = scr_min(ite->screenwidth, count); /* length of current line   */

	GENESIS_BMOVE_WAIT(genesis);		/* wait for block mover     */

	genesis->Width = width;			/* width of window to move  */
	genesis->DestX = source_x;		/* destination for clear    */
	genesis->DestY = source_y;		/* destination for clear    */

	/* write enable just the planes that need setting */
	set_planes = ite->color_pairs[0].BG & ite->cur_planes;
	if (set_planes) {
	    genesis->PXC_PWE = set_planes;
	    genesis->PXC_BROP = 0xff; 		/* replacement rule = set   */
	    genesis->PXC_NOP = 0x0; 		/* NOP before block move    */
	    genesis->BlockMoveStart = 0x1;	/* start block mover        */
	    GENESIS_BMOVE_WAIT(genesis);	/* wait for block mover     */
	}

	/* write enable just the planes that need clearing */
	clear_planes = ~ite->color_pairs[0].BG & ite->cur_planes;
	if (clear_planes) {
	    genesis->PXC_PWE = clear_planes;
	    genesis->PXC_BROP = 0x00; 		/* replacement rule = clear */
	    genesis->PXC_NOP = 0x0; 		/* NOP before block move    */
	    genesis->BlockMoveStart = 1;	/* start block mover        */
	}

	source_x = 0; 				/* the rest start at zero   */
	source_y += ite->line_height;		/* bump to the next line    */
    }
}


genesis_block_clear (ite, x, y, height, width)
    register struct iterminal *ite;
    int x, y, height, width;
/*
 *  This routine simply implements a device dependent (Genesis) block
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
    register struct genesis *genesis = (struct genesis *) ite->card;
    register clear_planes, set_planes;

    genesis_halt(ite);	       /* halt the pipeline before doing anything   */
    genesis_write_setup(ite);   /* set up the H/W before writing out pixels */

    genesis->Height = height;		    	/* height of window to move */
    genesis->Width  = width;			/* width of window to move  */
    genesis->DestX  = x;			/* destination for clear    */
    genesis->DestY  = y;			/* destination for clear    */

    /* write enable just the planes that need setting */
    set_planes = ite->color_pairs[0].BG & ite->cur_planes;
    if (set_planes) {
	genesis->PXC_PWE = set_planes;
	genesis->PXC_BROP = 0xff; 		/* replacement rule = set   */
	genesis->PXC_NOP = 0x0; 		/* NOP before block move    */
	genesis->BlockMoveStart = 0x1;		/* start block mover        */
	GENESIS_BMOVE_WAIT(genesis);		/* wait for block mover     */
    }

    /* write enable just the planes that need clearing */
    clear_planes = ~ite->color_pairs[0].BG & ite->cur_planes;
    if (clear_planes) {
	genesis->PXC_PWE = clear_planes;
	genesis->PXC_BROP = 0x00; 		/* replacement rule = clear */
	genesis->PXC_NOP = 0x0; 		/* NOP before block move    */
	genesis->BlockMoveStart = 1;		/* start block mover        */
    }
}



genesis_scroll (ite, to_row, dir, width)
    register struct iterminal *ite;
    register width;
/*
 *  This routines moves (scrolls) one row of characters either up one row
 *  or down one row on the screen.  
 *  
 *  The 'width' parameter contains the number of CHARACTERS on the line to
 *  be scrolled.  A check is first made to see if the line contains
 *  anything.  If not, the routine returns.  During this check, the
 *  variable, 'width', is reassigned the width in PIXELS of the row
 *  (block) to be moved.  The y-coordinate of the upper left corner of the
 *  destination block is computed from the 'to_row' parameter, which
 *  contains the destination ROW number.  Genesis HW access is checked and
 *  the HW is prepared.  The block mover HW registers are then filled.
 *  The routine uses the 'dir' parameter to determine the location of the
 *  source row.  If 'dir' is UP (ie., we are scrolling a line up), the
 *  source row is the row below the destination, and vice versa.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    register short dest_y;

    /* calculate width of line to move */
    if ((width *= ite->font_width) == 0)
        return;

    dest_y = to_row * ite->line_height;

    genesis_halt(ite);		/* halt the pipeline before doing anything   */
    genesis_write_setup(ite);   /* set up the H/W before writing out pixels  */

    /*  
     *  We set BROPConfig and PWE_DATHPATH here because we are doing an
     *  actual framebuffer (overlay) source block move.
     */
    genesis->BROPConfig = 0x1;		/* screen relative, source read	     */
    genesis->PXC_DATAPATH = 0x1;	/* take src input from src cache     */

    genesis->DestX   = 0x0;		/* set dest x pixel number 	     */
    genesis->SourceX = 0x0;		/* source x pixel is same as dest    */
    genesis->DestY   = dest_y;		/* set dest y pixel number 	     */

    /* source is one line behind or one line ahead */
    genesis->SourceY = dest_y + 
                       ((dir == UP) ? ite->line_height : -ite->line_height);

    genesis->Width   = width;			/* width of window to move   */
    genesis->Height  = ite->font_height;	/* height of window to move  */
    genesis->PXC_BROP = 0x33;			/* replacement rule = source */
    genesis->PXC_NOP = 0x0; 			/* NOP before block move     */
    genesis->BlockMoveStart = 0x1;		/* start window move 	     */

    GENESIS_BMOVE_WAIT(genesis);		/* wait for block mover      */
    genesis->BROPConfig = 0x0;			/* restore these registers   */
    genesis->PXC_DATAPATH = 0x0;
}




genesis_write_off (ite, vlp, y, x, count)
    register struct iterminal *ite;
    SCROLL *vlp;
    register x;
    register count;
/*
 *  This routine write characters from the ite's scroller onto the Genesis.
 *  It starts at the 'vlp' position in the scroller and continues to output
 *  a 'count' number of characters.  It starts placing characters at the (x,y)
 *  character coordinate passed in.  
 *
 *  The routine first halts the Genesis, then sets up the initial values
 *  for the block mover registers.  Then, for each character to be output,
 *  the routine does the following:  check to see if the color of the 
 *  current character is the same as the last;  if not, a new color table is
 *  built (this table is used to determine which planes need to be set in 
 *  a particular color, and which planes need to be cleared);  using this 
 *  color/plane information, the character is block moved as many times as
 *  necessary from offscreen, setting those planes that have to be set, and
 *  clearing those planes that need clearing.  It attempts to minimize the
 *  number of block moves necessary.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    register color_pair, scrollvalue, dest_x; 
    register clear_planes, set_planes;
    register move_case;
    register int planes = ite->cur_planes;
    register int back_c, fore_c;
    struct offscreen_font offscreen_xy;
    unsigned short dest_y;

    if (count <= 0)			/* nothing to do? */
        return;				/* do nothing */

    genesis_halt(ite);		/* halt the pipeline before doing anything  */
    genesis_write_setup(ite);   /* set up the H/W before writing out pixels */

    dest_x = ite->font_width * x;		/* get dest x pixel number  */
    dest_y = ite->line_height * y;		/* get dest y pixel number  */

    genesis->Width = ite->font_width;		/* width of window to move  */
    genesis->Height = ite->font_height;		/* height of window to move */
    genesis->DestY = dest_y;			/* set dest y pixel number  */

    /* force calculation of current color on first entry */
    color_pair = -1;

    do {
	/* source x & y pixel depends on char in offscreen memory */
	offscreen_xy = offscreen_font[(scrollvalue = *vlp++) & CHAR_VAL];

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

	    fore_c = ite->pixel_colors[15]; /* foreground color */
	    back_c = ite->pixel_colors[0];  /* background color */

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
			
	    /* wait for block mover hardware */
	    GENESIS_BMOVE_WAIT(genesis); 

	    /* set to all enabled planes as default */
	    genesis->PXC_PWE = planes;

	    /* check if just simple move rule enough */
	    if ((~back_c & fore_c & planes) == planes) {

		/* replacement rule = MOVE */
		genesis->PXC_BROP = 0x33;

	    /* check if just simple compliment move rule enough */
	    } else if ((~fore_c & back_c & planes) == planes) {

		/* replacement rule = CMP_MOVE */
		genesis->PXC_BROP = 0xcc;

	    /* well we must do at least one extra move, maybe two */
	    } else 
	        move_case = TRUE;
	} 
	else {
	    /* wait for block mover hardware */
	    GENESIS_BMOVE_WAIT(genesis); 
	}

	/* set dest pixel to next character in line */
	genesis->DestX = dest_x;

	if (move_case) {

	    /* check if planes to set */
	    if (set_planes) {
		genesis->BROPConfig = 0x0;
		genesis->PXC_DATAPATH = 0x0;
		genesis->PXC_PWE = set_planes;
		genesis->PXC_BROP = 0xff; /* rule=set */
		genesis->PXC_NOP = 0x0;		/* NOP before block move    */
		genesis->BlockMoveStart = 1; 	/* start block mover */
		GENESIS_BMOVE_WAIT(genesis); 
	    }

	    /* now clear non background planes, if any */
	    if (clear_planes) {
		genesis->BROPConfig = 0x0;
		genesis->PXC_DATAPATH = 0x0;
		genesis->PXC_PWE = clear_planes;
		genesis->PXC_BROP = 0x00;	/* rule=clear */
		genesis->PXC_NOP = 0x0;		/* NOP before block move    */
		genesis->BlockMoveStart = 1;	/* start block mover */
		GENESIS_BMOVE_WAIT(genesis); 
	    }

	    /* move only changed planes from offscreen mem */
	    genesis->PXC_PWE = (fore_c ^ back_c) & planes;
	    genesis->PXC_BROP = 0x66;	/* rule=XOR */
	}
	/* source x pixel depends on char in offscreen memory */
	genesis->SourceX = offscreen_xy.X;
	
	/* source x pixel depends on char in offscreen memory */
	genesis->SourceY = offscreen_xy.Y;

	/*  
	 *  We set BROPConfig and PWE_DATHPATH here because we will do an
	 *  actual framebuffer (overlay) source block move.
	 */
	genesis->BROPConfig = 0x1;	/* screen relative, source read	     */
	genesis->PXC_DATAPATH = 0x1;	/* take src input from src cache     */

	genesis->PXC_NOP = 0x0;		/* NOP before block move    	     */
	genesis->BlockMoveStart = 1;	/* start block mover 		     */

	/* bump to next character address for next pass */
	dest_x += ite->font_width;
	x++;

    } while (--count);
}



genesis_set_st (ite)
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
 *  routine transfers these values to the Genesis registers. 
 *
 *  For Genesis, we are not allowed to touch these registers unless the
 *  pipeline is plugged.  We therefore call genesis_halt() as with the
 *  other routines.  However this routine is called both by itself and in
 *  the middle of the normal ITE sequences. Therefore genesis_cursor() is
 *  only sometimes called after this routine (on the path out of the
 *  kernel), as is guaranteed for the routines above.  Therefore, we MUST
 *  call genesis_unhalt() at the end of this routine... BUT only if this
 *  routine was called by itself.  Why not just do it always?  Well, since
 *  we don't keep a stack of saved_pipe_plug requests, we would be
 *  restoring the user's pipe state prematurely.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    register int i, alpha, transparent, called_directly = 0;

    if (static_colormap)
        return;

    /*
     *  We do not have to call genesis_write_setup() here to save and 
     *  initialize the normally used registers because what we do here does 
     *  not effect them.  However, we still must plug the pipeline (see 
     *	comment above).
     */
    if (!genesis_halted) {
        called_directly = 1;
	genesis_halt(ite);	/* halt the pipeline before doing anything   */
	GENESIS_PIPE_WAIT(ite, genesis, 0x1);
    }

    switch (ite->flags & (GRAPHICSON|ALPHAON)) {
	case 0: /* black */
	    transparent = 0x0000;	alpha = 0x00;
	    break;
	case ALPHAON:
	    transparent = 0x0000;	alpha = (int)ite->plane_mask; 
	    break;
	case GRAPHICSON:
            transparent = 0xffff;	alpha = 0x00;
	    break;
	case GRAPHICSON|ALPHAON:
	    transparent = 0x0001;	alpha = (int)ite->plane_mask; 
	    break;
    }

    /*
     *  Here we initialize the 16 overlay register sets.  We must initialize
     *  all of them because we cannot assume nor depend on the value of the
     *  display mode planes.
     */
    for (i=0; i<16; i++) {
	genesis->window_regs[i].OPE_P = alpha;	/* typically equal to 0x7)  */
	genesis->window_regs[i].OPE_S = alpha;	/* typically equal to 0x7)  */

	genesis->window_regs[i].OIA_P = ~transparent;
	genesis->window_regs[i].OIA_S = ~transparent;
    }

    if (called_directly) {
        genesis_unhalt(ite);		/* unhalt the pipeline (see comment) */
    }
}



genesis_halt (ite)
    register struct iterminal *ite;
/*
 *  Non-transform engine type drivers (ITE, window manager, etc.)  need quick
 *  access to hardware downstream of the transform engine.  Since the transform
 *  engine was designed for maximum thruput rather than minimum latency, these
 *  drivers need access to the frame buffer without waiting for the transform
 *  engine to finish processing its queued commands.
 *
 *  This can be done by simply writing a 1 to the PipePlugRequest register.
 *  This process prevents the pipeline from sending more pixels to the
 *  frame buffer and the transform engine from accessing the frame buffer or
 *  colormap hardware, respectively.  Once this is done, and the frame buffer
 *  controller is not busy (done storing current pixels or finished with a
 *  previously started block move), the plugging process may access the
 *  hardware downstream from the pipeline provided that it restores the
 *  state of certain hardware registers when finished.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;

    if (!genesis_halted) { 	/* if already halted, don't stop it again */
	genesis_halted = 1; 	/* now mark it halted 			  */

	/*
	 *  Since genesis always has at least 1 transform engine, and since 
	 *  the pipe must be plugged before accessing most registers, we
	 *  don't check to see if microcode is running, just plug it.
	 *
	 *  The ITE must save the current state of the plug request register
         *  because it doesn't use any device locking mechanism so it may be
         *  interrupting another device driver's current access to the
         *  hardware.  If either the bit which signifies a plug or the bit
         *  which signifies a requested plug is set, we need to restore the
	 *  the user's value.  The order of checking these PlugReq and Plug
	 *  bits is important (think about it).
	 */

	gen_saved_plug_req = (PIPE_PLUG_REQ(genesis) || 
			      PIPE_PLUG(genesis)) ? 0x1 : 0x0;
	genesis->PipePlugRequest = 0x1;

	/*
	 *  The ITE now has to wait for hardware to complete pending 
	 *  work... this could include finishing up a previously started 
	 *  block move or flushing the currently cached pixels into the 
	 *  frame buffer.  The timeout period to allow for the worst 
	 *  case block move is approximately 300 milliseconds.  We do 
	 *  not do the wait in this routine however.  The wait actually
	 *  takes place in the genesis_write_setup() routine (see above).
	 */ 
    }
}



genesis_unhalt (ite)
    register struct iterminal *ite;
/*
 *  This routines simpy turns the transform engine loose again to continue
 *  where it left off when the genesis_halt() routine was called.  It restores
 *  the original state of the pipe plug..
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    register int i;

    if (!genesis_halted)
        panic("genesis_unhalt: not halted?");

    genesis_halted = 0;
    genesis->PipePlugRequest = gen_saved_plug_req;

    /*
     *  Here we must wait until the pipe is returned to its former state. 
     *  A user may have made a plug request and just determined that the pipe
     *  WAS at the state he or she wanted when we interrupted.  We have made
     *  the request to return it to the state the user expects, but it has
     *  not yet been granted.  There is no race in the kernel's check here 
     *  because there cannot exist a command in the pipeline which itself
     *  unplugs the pipe.
     */
    GENESIS_PIPE_WAIT(ite, genesis, gen_saved_plug_req);
}



genesis_font_restore (ite)
    register struct iterminal *ite;
/*
 *  This routine sets up the offscreen dimensions of the planes the ITE
 *  uses, namely the overlay planes.  Genesis overlays are only 0x600
 *  (1280 + 256) pixels across.  There is only a limited amount of
 *  offscreen memory reserved for the overlay planes, namely an area of
 *  256 X 1024.
 */

{
    ite->offscrn_max_width = ite->framedsp_width + 256;
    ite_font_restore(ite);
}



genesis_init (ite)
    register struct iterminal *ite;
/*
 *  This routine initializes the Genesis box for ITE use.  It first halts
 *  any activity on the GPU with a genesis_halt() call.  It calls
 *  genesis_write_setup() and genesis_set_st() to initialize several GPU
 *  registers.  It loads off-screen memory with the current font and then
 *  loads all 16 overlay colormaps with the correct values for the ITE's
 *  use.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    register int i, j;
    struct gen_rgb temp_rgb;	      /* needed for long word alignment      */
    extern char bitm_colormap[8][3];  /* Standard colormap, from ite_driv.c  */

    genesis_halt(ite);		/* halt the pipeline before doing anything   */
    genesis_write_setup(ite);   /* set up the H/W before writing out pixels  */
    genesis_set_st(ite);	/* turn on the correct alpha/image planes    */

    /* now restore offscreen memory from in ram font */
    (*ite->crt_font_restore)(ite);

    /* turn on video in case some application turned it off */
    genesis->DISPEN = 0x1;

    /*
     *  Here we initialize the 16 overlay colormaps.  We must initialize
     *  all of them because we cannot assume nor depend on the value of the
     *  display mode planes.  We initialize them in column order to reduce
     *  the number of times the temporary rgb values are copied.
     */

    /* (the ITE only uses eight colors) */
    for (j=0; j<8; j++) {

	/* skip maps that are not currently allowed */
	if ((j & ite->cur_planes) != j)
	    continue;		

	temp_rgb.red   = bitm_colormap[j][0];
	temp_rgb.green = bitm_colormap[j][1];
	temp_rgb.blue  = bitm_colormap[j][2];

	for (i=0; i<15; i++) {
	    genesis->oprimary[i][j] = temp_rgb;
	    genesis->osecondary[i][j] = temp_rgb;
	}

	genesis->oiprimary[j] = temp_rgb;
	genesis->oisecondary[j] = temp_rgb;
    }
}



genesis_pwr_reset (ite)
    register struct iterminal *ite;
/*
 *  During bootup, this routine is called to do whatever Genesis hardware
 *  preparation is necessary for the ITE.  It first resets the GPU by
 *  writing a 0x0 into the RESET register.  It then executes whatever
 *  initialization code is present in the idrom.  It then initializes
 *  other HW registers and clears the screen.  Finally it checks to see 
 *  if the Genesis software structure was created correctly.
 */

{
    register struct genesis *genesis = (struct genesis *) ite->card;
    int reset_value = 0x0;

    /* Reset hardware with protected copy in case it's hung. */
    /* Do it twice just for luck. */
    bcopy_prot(&reset_value, &genesis->reset, 1);
    snooze(5000);			/* 5 milliseconds worst case         */
    bcopy_prot(&reset_value, &genesis->reset, 1);
    snooze(5000);			/* 5 milliseconds worst case         */

    idrom_init(ite->card);	       	/* do the init seq in the idrom      */

    /*
     *  Since the idrom init sequence leaves the pipeline plugged, here we 
     *  initialize the driver variable which flags a plugged pipe.
     */
    genesis_halted = 1;

    /*
     *  Now set up box and clear the visible screen and the offscreen memory.
     *  We do not clear the full 2048 X 1024 region because part of it 
     *  corresponds to the display mode planes.
     */
    genesis->WindowID = GEN_REG_SET;
    genesis->DestX = 0;
    genesis->DestY = 0;
    genesis->Width = 0x600;  		/* 1280 visible + 256 offscreen      */
    genesis->Height = 0x400;		/* 1024 			     */
    genesis->TConfig = 0x30; 

    genesis->FrameBufferAccess = 0x51;	/* overlay cache registers enabled   */
    genesis->PXC_PWE = ite->cur_planes;
    genesis->PXC_BROP = 0x00; 		/* replacement rule		     */
   
    /*
     *  These values are set to 0x1 by the initialization sequence in the
     *  IDROM.  They are reset here to values needed by the ITE.
     */
    genesis->window_regs[15].OPE_P = (int)ite->plane_mask;
    genesis->window_regs[15].OPE_S = (int)ite->plane_mask;

    genesis->PXC_NOP = 0x0; 		/* NOP before block move	     */
    genesis->BlockMoveStart = 1;
    GENESIS_BMOVE_WAIT(genesis); 

    /* Check the last structure entry for consistency */
    if ((int) &(((struct genesis *) 0)->DISPEN) != 0x94404)
        panic("error in struct genesis");
}



genesis_iterminal_init (ite)
    register struct iterminal *ite;
/*
 *  During bootup, this routine is called from the initialization portion 
 *  of ite_driv.c (crt_init()) and initializes Genesis specific fields in 
 *  the iterminal structure.
 *
 *  The routine itecrt_decide() is from ite_driv.c and is used to decide
 *  whether offscreen memory can currently be used.
 */

{
    extern itecrt_decide();			/* see comment above */

    ite->flags |= (ITE_COLOR|GENESIS_PRES);

    /* Genesis always has overlay planes */
    ite->overlay_planes = TRUE;

    /* ITE only uses 3 planes (8 colors) */
    ite->plane_mask = 0x7;
    ite->crt_write = itecrt_decide;
    ite->crt_write_setup  = genesis_write_setup;
    ite->crt_write_off  = genesis_write_off;
    ite->crt_cursor = genesis_cursor; 
    ite->crt_clear  = genesis_clear;
    ite->crt_block_clear = genesis_block_clear;
    ite->crt_scroll = genesis_scroll;
    ite->crt_reset  = genesis_init;
    ite->crt_pwr_reset  = genesis_pwr_reset;
    ite->crt_set_state = genesis_set_st;
    ite->crt_font_restore  = genesis_font_restore;
}
