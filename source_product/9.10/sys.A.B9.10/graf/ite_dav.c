/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_dav.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:58:40 $
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
#include "../graf/ite.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"
#include "../graf.300/ite_dav.h"

#define LGBHOLD 	0x80000000
#define PACEHOLD	0x1
#define davinci_bmove_wait(davinci) while (davinci->wbusy);

unsigned int saved_lgb_holdoff[3];
unsigned short save_pace_plug;

int davinci_saved=0;
int DV_SAVED_REG=0;
int xforms_present;	/* bit mask of which transform engines present */
extern int static_colormap;

/*			   D A V I N C I				*/

/* What we save (also what we use in normal usage). */
char  DV_drive, DV_opwen, DV_rep_rule, DV_fold;
short DV_wheight, DV_wwidth, DV_dest_x, DV_dest_y, DV_source_x, DV_source_y;
int   DV_fbwen;

davinci_check_screen_access(ite)
    struct iterminal *ite;
{
    halt_davinci(ite);
    return(0);						/* we always succeed */
}

davinci_write(ite)						    /* ENTRY */
struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    register i;

    /* wait up to 60 seconds for things to stop */
    for (i=60*1000; --i>0; ) {
	/* wait for scan board and bit mover to not be busy */
	if (!davinci->scanbusy && !davinci->wbusy)
		break;
			
	snooze(1000);				 /* wait another millisecond */
    }

    /* if timed out, punt */
    if (i==0) {
	msg_printf("HP98730 has timed out: scanbusy=%x wbusy=%x; resetting it.\n", davinci->scanbusy, davinci->wbusy);
	davinci_pwr_reset(ite);
    }

    if (!DV_SAVED_REG) {		/* did we already save stuff? */
	/*
	 * Save everything we change that we don't intend
	 * to be permanent (like OPVEN for alpha/graphics on/off).
	 */
	DV_rep_rule = davinci->rep_rule;
	DV_drive    = davinci->drive;
	DV_opwen    = davinci->opwen;
	DV_fbwen    = davinci->fbwen;
	DV_fold     = davinci->fold;
	DV_wheight  = davinci->wheight;
	DV_wwidth   = davinci->wwidth;
	DV_dest_x   = davinci->dest_x;
	DV_dest_y   = davinci->dest_y;
	DV_source_x = davinci->source_x;
	DV_source_y = davinci->source_y;

	DV_SAVED_REG = 1;		/* do not save twice */
    }
	
    davinci->drive = 0x10;		/* overlay plane enable */
    davinci->intrpt = 0x04;		/* enable frame buffer */
    davinci->rep_rule = 0x33;		/* ... for itecrt_write() */
    davinci->opwen = ite->cur_planes;
    davinci->fbwen = 0;			/* do not touch the frame buffer */
    davinci->fold  = 1;			/* byte per pixel */
}

davinci_cursor(ite, x, y, color_num)				    /* ENTRY */
    struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }
    /* set up the hardware before writing out pixels */
    davinci_write(ite);

    davinci->rep_rule = 0x66; /* set pixel replacement rule XOR */

    bitmap_cursor(ite, x, y, color_num);

    /* restore registers */
    if (ite->cursor_on) {		/* what a way to run a kernel! */
	davinci->rep_rule = DV_rep_rule;
	davinci->drive    = DV_drive;
	davinci->opwen    = DV_opwen;
	davinci->fbwen    = DV_fbwen;
	davinci->fold     = DV_fold;
	davinci->wheight  = DV_wheight; 
	davinci->wwidth	  = DV_wwidth; 
	davinci->dest_x	  = DV_dest_x; 
	davinci->dest_y	  = DV_dest_y; 
	davinci->source_x = DV_source_x; 
	davinci->source_y = DV_source_y; 

	unhalt_davinci(ite);	/* resume transform engine */
	DV_SAVED_REG=0;		/* registers restored */

	/*
	 * The user may have been just about to do a block move
	 * when we interrupted.  He *already* waited for the
	 * block mover and he's sure that it's ready.
	 * Hence, make sure that we don't leave it running.
	 */
	davinci_bmove_wait(davinci);
    }
}

davinci_clear(ite, y, x, count)					    /* ENTRY */
    register struct iterminal *ite;
    register y, count;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    register len;
    register short source_x, source_y, width;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    source_x = x * ite->font_width;
    source_y = y * ite->line_height;

    len = scr_min(ite->screenwidth - x, count);

    /* set up the hardware before writing out pixels */
    davinci_write(ite);

    /* height of window to move */
    davinci->wheight = ite->line_height;

    while (count) {
	count -= len;
	width = len * ite->font_width;
	len = scr_min(ite->screenwidth, count);

	davinci_bmove_wait(davinci);
	davinci->wwidth = width;

	davinci->dest_x = source_x;		/* destination for clear */
	davinci->dest_y = source_y;

	/* write enable just planes that need setting */
	if (davinci->opwen = ite->color_pairs[0].BG & ite->cur_planes) {
	    davinci->rep_rule = 0xff;		/* replacement rule=set */
	    davinci->wmove = 1;
	    davinci_bmove_wait(davinci);
	}
	/* write enable just planes that need clearing */
	if (davinci->opwen = ~ite->color_pairs[0].BG & ite->cur_planes) {
	    davinci->rep_rule = 0x00;		/* replacement rule=clear */
	    davinci->wmove = 1;
	}

	source_x = 0;			/* the remaining lines start at zero */
	source_y += ite->line_height;	/* bump to the next line */
    }
}

davinci_block_clear(ite, x, y, height, width)
    register struct iterminal *ite;
    int x, y, height, width;
/*
 *  This routine simply implements a device dependent (daVinci) block
 *  move.  In this case a block clear.
 *  
 *  The routine first checks to ensure the GPU is not busy.  It then readies
 *  the box for a block move, setting up the necessady registers.
 *
 *  The routine then performs the clear, using the block mover.  Since
 *  clearing a screen really just means setting the screen to the currently
 *  selected background color, there are two 'if' statements.  The first
 *  covers the case where planes need to be set (a color turned on).  The 
 *  second covers the case where planes need to be cleared.  These cases
 *  loosely correspond to filling in a colored background and clearing out 
 *  a black background respectively.
 */

{
    register struct davinci *davinci = (struct davinci *) ite->card;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    /* set up the hardware before writing out pixels */
    davinci_write(ite);

    davinci->wheight = height;	       		/* height of window to move */
    davinci->wwidth  = width;	   		/* width of window to move */
    davinci->dest_x  = x;		   	/* destination for clear */
    davinci->dest_y  = y;			/* destination for clear */

    /* write enable just planes that need setting */
    if (davinci->opwen = ite->color_pairs[0].BG & ite->cur_planes) {

	davinci->rep_rule = 0xff; 		/* replacement rule set */
	davinci->wmove = 1;	    		/* start block mover */
	davinci_bmove_wait(davinci); 		/* wait for block mover */
    }
    /* write enable just planes that need clearing */
    if (davinci->opwen = ~ite->color_pairs[0].BG & ite->cur_planes) {

	davinci->rep_rule = 0x00; 		/* replacement rule clear */
	davinci->wmove = 1;	    		/* start block mover */
    }
}


davinci_scroll(ite, to_row, dir, width)				    /* ENTRY */
    register struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    register short dest_y;

    /* calculate width of line to move */
    if ((width *= ite->font_width) == 0)
	return;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, to_row);
	return;
    }
    dest_y = to_row * ite->line_height;

    /* set up the hardware before writing out pixels */
    davinci_write(ite);

    davinci->dest_x = 0;		/* set dest x pixel number */
    davinci->source_x = 0;		/* source x pixel is same as dest */
    davinci->dest_y = dest_y;		/* set dest y pixel number */

    /* source is one line behind or one line ahead */
    davinci->source_y = dest_y + 
		((dir == UP) ? ite->line_height : -ite->line_height);

    davinci->wwidth = width;
    davinci->wheight = ite->font_height;	/* height of window to move */
    davinci->rep_rule = 0x33;			/* replacement rule = move */
    davinci->wmove = 1;
}

davinci_write_off(ite, vlp, y, x, count)			    /* ENTRY */
    register struct iterminal *ite;
    register SCROLL *vlp; 
    register x;
    register count;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    register color_pair, scrollvalue, dest_x; 
    register clear_planes, set_planes;
    register move_case;
    int back_c, fore_c;
    int planes = ite->cur_planes;
    struct offscreen_font offscreen_xy;
    unsigned short dest_y;

    if (count <= 0)			/* nothing to do? */
	return;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }
    dest_x = ite->font_width * x;		/* get dest x pixel number */
    dest_y = ite->line_height * y;		/* get dest y pixel number */

    davinci_write(ite); /* set up the hardware before writing out pixels */
    davinci->wwidth = ite->font_width;
    davinci->wheight = ite->font_height;
    davinci->dest_y = dest_y;

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

		fore_c = ite->pixel_colors[15];
		back_c = ite->pixel_colors[0];

		/*            rule    case 1  case 2  case 3  case 4 */
		/*    FG =		0	1	0	1    */
		/*    BG =		0	0	1	1    */
		/*	              _____   _____   _____   _____  */
		/* ~(FG | BG)	0	1       0       0       0    */
		/* ~BG & FG	3	0	1	0	0    */
		/* ~FG & BG    12	0	0	1	0    */
		/*  FG & BG    15	0	0	0	1    */

		/* set up all planes in case simple case */
		move_case = FALSE;
		set_planes = back_c & planes;
		clear_planes = ~back_c & planes;
			
		davinci_bmove_wait(davinci); 

		/* set to all enabled planes as default */
		davinci->opwen = planes;

		/* check if just simple move rule enough */
		if ((~back_c & fore_c & planes) == planes) {
		 	/* replacement rule = MOVE */
			davinci->rep_rule = 0x33;
		}
		/* check if just simple compliment move rule enough */
		else if ((~fore_c & back_c & planes) == planes) {
	 		/* replacement rule = CMP_MOVE */
			davinci->rep_rule = 0xcc;
		/* well we must do at least one extra move, maybe two */
		}
		else move_case = TRUE;
	}

	davinci_bmove_wait(davinci); 

	/* set dest pixel to next character in line */
	davinci->dest_x = dest_x;

	if (move_case) {
		/* check if planes to set */
		if (davinci->opwen = set_planes) {
			davinci->rep_rule = 0xff;	/* rule=set */
			davinci->wmove = 1;
			davinci_bmove_wait(davinci); 
		}
		/* now clear non background planes, if any */
		if ( davinci->opwen = clear_planes) {
	 		davinci->rep_rule = 0x00;	/* rule=clear */
			davinci->wmove = 1;
			davinci_bmove_wait(davinci); 
		}
		/* move only changes planes from offscreen mem */
		davinci->opwen = (fore_c ^ back_c) & planes;

		davinci->rep_rule = 0x66;		/* rule=XOR */
	}

	/* source x, y pixels depends on char in offscreen memory */
	davinci->source_x = offscreen_xy.X;
	davinci->source_y = offscreen_xy.Y;
	davinci->wmove = 1;

	/* bump to next character address for next pass */
	dest_x += ite->font_width;
	x++;
    } while (--count);
}

/*
 * Turn on/off alpha and graphics.
 */
davinci_set_st(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    register short alpha, transparent;

    /* Ignore if we have a static colormap */
    if (static_colormap)
	return;

    switch (ite->flags & (GRAPHICSON | ALPHAON)) {
	case 0: /* black */
		transparent=0;		alpha=0;		break;
	case ALPHAON:	
		transparent=0;		alpha=ite->plane_mask;	break;
	case GRAPHICSON:
		transparent=0xff;	alpha=0;		break;
	case GRAPHICSON | ALPHAON:
		transparent=1<<0;	alpha=ite->plane_mask;	break;
    }

    /* Enable or disable the overlay planes (which contain alpha) */
    davinci->opvenp = alpha;
    davinci->opvens = alpha;

    /*
     * If graphics on and alpha off, make all overlays transparent.
     * If both graphics and alpha on, make only black transparent.
     */
    davinci->ovly0p = transparent;
    davinci->ovly0s = transparent;

    /*
     * Due to hardware error, the fourth overlay plane is not
     * adequately supressed.  Make sure that we are independent
     * of that bit by setting ovly1[ps] also.
     */
    davinci->ovly1p = transparent;
    davinci->ovly1s = transparent;

    davinci->fv_trig = 1;	/* trigger writes to opven[ps] & ovly0[ps] */
}

davinci_init(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    register int i, bank, save_cmap_bank;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, 0);
	return;
    }

    /* set up the hardware */
    davinci_write(ite);
    davinci_set_st(ite);

    /* now restore offscreen memory from in ram font */
    (*ite->crt_font_restore)(ite);

    /* turn on video in case some application turned it off */
    davinci->dispen = 0x01;

    for (i=0; i<ITE_MAX_CPAIRS; i++) {
	/* skip maps that are not currently allowed */
	if ((i & ite->cur_planes) != i)
	    continue;
	
	save_cmap_bank = davinci->cmapbank; /* current bank */

	for (bank=0; bank<=1; bank++) {
	    davinci->cmapbank = bank;     /* select bank */
	    davinci->rgb[i].r = bitm_colormap[i][0];
	    davinci->rgb[i].g = bitm_colormap[i][1];
	    davinci->rgb[i].b = bitm_colormap[i][2];
	}

	davinci->cmapbank = save_cmap_bank; /* restore bank */
    }
}

davinci_pwr_reset(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    char reset_value = 0x80;

    /* Reset hardware with protected copy in case it is hung. */
    /* Do it twice just for luck. */
    bcopy_prot(&reset_value, &davinci->reset, 1);
    snooze(10);					/* DaVinci is OTL for 4us */
    bcopy_prot(&reset_value, &davinci->reset, 1);
    snooze(10);

    davinci->intrpt = 0x04;		/* tell frame buffer space to dtack */
    davinci->en_scan = 1;		/* tell scan board to dtack */
    idrom_init(ite->card);		/* do what little the idrom does */
    ite->overlay_planes = TRUE;		/* daVinci always has overlay planes */
    davinci->vdrive=0;			/* display frame buffer 0 */
    davinci_bmove_wait(davinci);

    /*
     * The following code should be in the id rom.
     * However, the Einsteins in the hardware group have
     * run out of memory in the ID ROM, CANNOT get any more,
     * and CANNOT have the board reset do a complete sensible reset.
     *
     * Hence, it MUST be done here.  Good work, people.
     */

    davinci->cmapbank = 0;

    /* init image planes colormap 0 so 0 is black */
    davinci->image_r0 = 0;  davinci->image_r1 = 0;
    davinci->image_g0 = 0;  davinci->image_g1 = 0;
    davinci->image_b0 = 0;  davinci->image_b1 = 0;

    davinci->panxh = 0; davinci->panxl = 0;
    davinci->panyh = 0; davinci->panyl = 0;
    davinci->zoom  = 0;
    davinci->cdwidth = 0x50;
    davinci->chstart = 0x52;
    davinci->cvwidth = 0x22;
    davinci->pz_trig = 1;		/* trigger pan/zoom changes */

    /* END OF USELESS HARDWARE IDIOCY CODE */

    /* What transform engines do we have? */
    xforms_present = 0;
    if (testr(&davinci->lgb_holdoff0, 1)) xforms_present |= 1<<0;
    if (testr(&davinci->lgb_holdoff1, 1)) xforms_present |= 1<<1;
    if (testr(&davinci->lgb_holdoff2, 1)) xforms_present |= 1<<2;

    /* Check the last structure entry for consistency */
    if ((int) &(((struct davinci *) 0)->lgb_holdoff2) != 0xf1fc)
	panic("error in struct davinci");
}

halt_davinci(ite)						    /* ENTRY */
register struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);
    int dummy;

    if (davinci_saved)
	return;
    davinci_saved = 1;

    if (xforms_present == 0)	       /* do not access screset if no xforms */
	return;

    if (davinci->screset==0)			  /* if no microcode running */
	return;

    if (xforms_present & (1<<0)) {
	saved_lgb_holdoff[0] = davinci->lgb_holdoff0;
	davinci->lgb_holdoff0 = LGBHOLD;
    }
    if (xforms_present & (1<<1)) {
	saved_lgb_holdoff[1] = davinci->lgb_holdoff1;
	davinci->lgb_holdoff1 = LGBHOLD;
    }
    if (xforms_present & (1<<2)) {
	saved_lgb_holdoff[2] = davinci->lgb_holdoff2;
	davinci->lgb_holdoff2 = LGBHOLD;
    }

    /* Plug the pace chip. */
    if (xforms_present) {			      /* any at all present? */
	save_pace_plug = davinci->pace_plug;
	davinci->pace_plug = PACEHOLD;

	/* Kill time for hardware reasons */
	dummy = davinci->lgb_holdoff0;
	dummy = davinci->lgb_holdoff0;
	dummy = davinci->lgb_holdoff0;
    }
}

unhalt_davinci(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    register struct davinci *davinci = (struct davinci *)(ite->card);

    if (!davinci_saved)
	panic("unhalt_davinci: not saved?");
    davinci_saved=0;

    if (xforms_present == 0)	       /* do not access screset if no xforms */
	return;

    if (davinci->screset==0)			  /* if no microcode running */
	return;

    if (xforms_present)
	davinci->pace_plug = save_pace_plug;

    if (xforms_present & (1<<0))
	davinci->lgb_holdoff0 = saved_lgb_holdoff[0];
    if (xforms_present & (1<<1))
	davinci->lgb_holdoff1 = saved_lgb_holdoff[1];
    if (xforms_present & (1<<2))
	davinci->lgb_holdoff2 = saved_lgb_holdoff[2];
}
