/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_driv.c,v $
 * $Revision: 1.7.84.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/11 12:12:05 $
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

#include "../s200io/bootrom.h"
#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"
#include "../graf.300/ite_gator.h"		 /* for gatoraid_microcode[] */
#include "../graf.300/ite_top.h"
#include "../mach.300/cpu.h"
#include "../graf.300/graphics.h"
#include "../h/malloc.h"

#define frame_offset(ite, x, y) (((y) * ite->line_height \
				* ite->framebuf_width)\
				+ ((x) * ite->font_width))

int console_width, console_height;
char sysflg;
struct font_def cursor_font;

int (*ite_bitmap_call)(), bmap_nop();
char ite_bmap_semi_lock;
struct graphics_descriptor *graphics_ite_dev;

extern int *cons_tty;
extern int scroll_lines;
extern int ite_type;

/*  BEGIN BIT MAPPED DRIVERS FOR GATORBOX, BOBCAT ETC. */ 

int mask_table[16] = {
	0x00000000,
	0x000000ff,
	0x0000ff00,
	0x0000ffff,
	0x00ff0000,
	0x00ff00ff,
	0x00ffff00,
	0x00ffffff,
	0xff000000,
	0xff0000ff,
	0xff00ff00,
	0xff00ffff,
	0xffff0000,
	0xffff00ff,
	0xffffff00,
	0xffffffff
};

unsigned char *get_frame_addr();

/*
 * By virtue of a call to this routine means offscreen mem is destroyed.
 */
set_min_xy(ite, x, y)
    register struct iterminal *ite;
    register x, y;
{
    ite->missed_output++;

    if (y > ite->rypos)				/* check if less that before */
	return;

    /* if on the same line, use min */
    if (y == ite->rypos)
	 ite->rxpos = scr_min(ite->rxpos, x);
    else ite->rxpos = x;			     /* use the new position */

    ite->rypos = y;					    /* set new min y */
}

/*
 * Routine to test if display hardware is busy or semaphore locked.
 */
int
check_screen_access(ite)
    struct iterminal *ite;
{
    register i, pass, flags = ite->flags;
    register int *dio  = (int   *) (ite->card);
    register short *sdio = (short *) (ite->card);

    if (flags & DAVINCI)
	return(davinci_check_screen_access(ite));

    if (flags & TIGERSHARK)
	return(tiger_check_screen_access(ite));

    if (flags & GENESIS_PRES)
	return(genesis_check_screen_access(ite));
		
    /* renaissance have transform engine? */
    if (flags & REN_TFORM_ENGN) {
	/* if in bad shape, skip and let operator do reset */
	if (!testr((char *)ite->card+0x8012, 1))
		return(SCR_TFORM_BROKE);

	/* if CDF busy or transform busy, pause transform engine */
	if ((*((char *)ite->card+0x800a) | *((char *)ite->card+0x8012))&0x80) {

		/* However, if transform already halted, exit */
		if (*((char *)ite->card+0x8012) & 0x01)
			return(0);

		/* set I halted transform engine flag */
		ite->flags |= REN_TFORM_HALT;

		/* request a pause(halt) from transform engine */
		*((char *)ite->card+0x8012) = 0x80; 

		/* wait up to 1000 ms for a pause(halt) */
		i = 1000;
		do {
			/* if status clear, exit */
			if (((*((char *)ite->card+0x800a) | 
				*((char *)ite->card+0x8012)) & 0x80) == 0x00)
				break;

			/* or, if transform halted, exit */
			if (*((char *)ite->card+0x8012) & 0x01)
				break;
		
			/* wait another millisecond */
			snooze(1000);
		} while (--i);
		if (i == 0) {
			/* reset and halt transform engine*/
			*((char *)ite->card+0x8002) = 0xa0;
			*((char *)ite->card+0x8002) = 0x20;
		}
		/* make double sure transform engine is halted*/
		*((char *)ite->card+0x8002) = 0x80;
	}
	/* now go do the thing to the screen */
	return(0);
    }
    /* if overlay planes, ignore semaphore */
    if (!ite->overlay_planes) {
	/* semaphore lock from graphics driver */
		if (ite_bmap_semi_lock) {
			if (!graf_really_locked(graphics_ite_dev)) {
				ite_bmap_semi_lock = 0;
				/* wakeup procs waiting for this device */
				wakeup(graphics_ite_dev); 
			/* yes, it is really locked */
			} else return(SCR_SEMIP_LOCK);
		}
		/* alpha off and no overlay planes */
		if (!(flags & ALPHAON))
			return(SCR_ALPHAOFF);

		/* gatoraid busy */
		if ((flags & GATORAID) && (*((char *)ite->card+0x8004) < 0)) 
			return(SCR_TFORM_BUSY);
    }
	
    /* If we have an unhalted fastcat, halt it. */
    if ((flags & CAT_TFORM_ENGN) && (dio[FC_STATUS] & 0x00000040)==0) {
	dio[FC_HALT_REQ] = 0x80000000;	/* request a halt */
	/* all the pauses sum up to 1 second */
	for (i=1; i<=1414 && dio[FC_HALT_ACK] != 0x80000000; i++)
		snooze(i);		/* wait a bit */
		
	if (dio[FC_HALT_ACK] != 0x80000000) {  /* STILL not halted? */
		dio[FC_STATUS] = 0x000000c0;  /* halt/reset */
		msg_printf("check_screen_access: 98556 timeout; halted by kernel\n");
		for (pass=0; pass<4; pass++) {
			/* Wait for catseye not busy */
			for (i=1000; --i>=0; ) {
				if ((sdio[CAT_STATUS] & 01)==0)
					break;
				snooze(1000);	/* wait 1ms */
			}
			if (i<0) 
				msg_printf("check_screen_access: 98556 timeout; pass %d\n", pass);
			if ((sdio[CAT_STATUS] & 02)==0)
				sdio[0x409c/2]=0; /* clr pipe */
		}
		if (((i=sdio[CAT_STATUS]) & 02)==0) {
			msg_printf("check_screen_access: 98556 still not ready, resetting it, status=%x\n", i);
			bobcat_pwr_reset(ite);
		}	
	}
    }
    /* now go do the thing to the screen */
    return(0);
}

/*
 * set or clear the semaphore from graphics driver
 */
ite_bmap_semiphore(flag)
int flag;
{
    register struct iterminal *ite = &iterminal;
    register struct tty *tp = ite->ite_tty;
    register x;

    /* check if any missed output (probably none on lock request) */
    if (ite->missed_output || tp->t_outq.c_cc) {
	x = splsx(tp->t_int_lvl);

	/* make sure locked is not the reason */
	ite_bmap_semi_lock = 0;

	/* check screen availability */
	switch (check_screen_access(ite)) {
		case SCR_TFORM_BROKE:
			(*ite->crt_pwr_reset)(ite);
			break;

		case SCR_TFORM_BUSY:
			/* temp kludge!!!!!, spin on transform engine */
			while (check_screen_access(ite));
			/* fall thru */
		case 0:
			/* restore any missed output */
			restore_missing_scn(ite);

			/* and empty clist */
			while (tp->t_outq.c_cc) 
				ite_out_cblock(tp);
			break;

		case SCR_ALPHAOFF:
			/* if alpha is off, just ignore updating */
			break;
		}
		splsx(x);
    }
    /* only modification of this flag */
    ite_bmap_semi_lock = flag;
    ite->time_out_count = 0;
}

/*
 * Put a new color and/or enhancement into pixel table.
 */
ite_build_pixtbl(ite, scrollvalue)
    struct iterminal *ite;
    register scrollvalue;
{
    register int *mask_tablep, *pixel_colorsp;
    register color, fgxbg, count;

    color = char_enhance(scrollvalue);

    /* check if bogus call */
    if (ite->current_cpair == color)
	return(0);

    /* if underline only thing to change skip rebuilding pixel table */
    if ((ite->current_cpair ^ color) == UNDERLINE)
	goto skipcolor; 

    /* check if char enabled */
    if ((scrollvalue & CHAR_ON) == 0)
	return(1);

    /* get the new color pair number */
    color = char_colorpair(scrollvalue);

    /* get the foreground XOR background colors */
    fgxbg = ite->color_pairs[color].FG ^ ite->color_pairs[color].BG; 

    /* if inverse video */ 
    if (scrollvalue & INVERSE_VIDEO) 
	 color = ite->color_pairs[color].FG; 
    else color = ite->color_pairs[color].BG; 

    mask_tablep = mask_table;
    pixel_colorsp = ite->pixel_colors;
    count = 16;

    /* and according to the rules of woo */
    do
	*pixel_colorsp++ = (*mask_tablep++ & fgxbg) ^ color;
    while(--count);
	
skipcolor:
    /* put new color of pixel table into ite structure */
    ite->current_cpair = char_enhance(scrollvalue);

    return(0);
}

itecrt_decide(ite, vlp, y, x, count)
    register struct iterminal *ite;
    SCROLL *vlp;
{
    extern int ite_memory_used, ite_memory_trashed;

    if (ite_memory_used)
	itecrt_write(ite, vlp, y, x, count);
    else {
	if (ite_memory_trashed) {
	    ite_memory_trashed = 0;
	    if (ite->crt_font_restore == bmap_nop)
	        ite_font_restore(ite);
	    else
	        (*ite->crt_font_restore)(ite);
	    /*
	     * Restoring the screen display seems like a good idea,
	     * but it wipes out any picture that a program might
	     * leave on the screen.  Let them do soft reset.
	     */
	    /* scr_restore_dsp(ite, 0, 0);	/* recursive call! */
	}
	(*ite->crt_write_off)(ite, vlp, y, x, count);
    }
}

itecrt_write(ite, vlp, y, x, count)
    register struct iterminal *ite;
    SCROLL *vlp;
{
    register int scrollvalue;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    /* set up the font structure */
    ite->cur_font.frame_mem = ite->frame_start + frame_offset(ite, x, y);
    /* set up the hardware before writing out pixels */
    (*ite->crt_write_setup)(ite);

    while (count--) {
	/* check if any color or enhancement changes */
	if (ite->current_cpair != char_enhance(scrollvalue = *vlp++)) {
		/* set new value of last color & enhancements */
		if (ite_build_pixtbl(ite, scrollvalue))
			return;
	}
	ite->cur_font.font_buf = 
		ite->font_start + (ite->char_out[scrollvalue & CHAR_VAL] *
			ite->font_bytpchar);

	/* call the font draw routine with font structure */
	(*ite->crt_font_draw)(&ite->cur_font);

	/* if underline, go and write the underline character */
	if (scrollvalue & UNDERLINE) {
		ite->und_font.frame_mem = ite->cur_font.frame_mem +
			ite->uline_frame_off; 
		/* call the font draw routine with correct font width */
		(*ite->crt_font_draw)(&ite->und_font);
	}
	/* bump to next character in line */
	ite->cur_font.frame_mem += ite->font_width;
    }
}

bitmap_cursor(ite, x, y, scrollvalue)
    register struct iterminal *ite;	
    register scrollvalue;
{
    register color, fgxorbg, *pointer, *mask_tablep;

    /* calculate frame address for current cursor position */
    cursor_font.frame_mem = ite->frame_start + frame_offset(ite, x, y);

    /* get color pair number */
    if (scrollvalue & CHAR_ON)
	 color = char_colorpair(scrollvalue);
    else color = 0;

    /* check if color changed from last time */
    fgxorbg = ite->color_pairs[color].FG ^ ite->color_pairs[color].BG;

    if (fgxorbg != ite->FGxorBG[15]) {
	mask_tablep = mask_table;
	pointer = ite->FGxorBG;
	color = 16;
	do
	    *pointer++ = *mask_tablep++ & fgxorbg;
	while (--color);
    }

    /* call the font draw routine with correct font width */
    (*ite->crt_font_draw)(&cursor_font);

    /* set cursor on/off flag */
    ite->cursor_on ^= TRUE;
}

bmap_nop()
{
}

/* 		now calculate screen parameters from: 			*/
/*    ite->line_height    	font pixel spacing between lines        */
/*    ite->font_height		font pixel spacing between lines        */
/*    ite->font_start           font buffer start address               */
/*    ite->font_bytpchar	font buffer chars per font character    */

ite_calc_screen(ite)
    register struct iterminal *ite;
{
    register temp1, temp2;
    register unsigned char *cp;
 
    /* Initially set offscreen boundaries to boundaries of framebuffer */
    ite->offscrn_max_width = ite->framebuf_width;
    ite->offscrn_max_height = ite->framebuf_height;

    if (!(ite->flags & STI)) {
    /* save font height as one less because of the hokey "dbf" instruction*/
    ite->cur_font.heightm1 = ite->font_height - 1;

    /* save font width as one less because of the hokey "dbf" instruction */
    ite->cur_font.widthm1 = ite->font_width - 1;

    /* save pixels to add to char to get to next pixel row */
    ite->cur_font.next_row_add = ite->framebuf_width - ite->font_width;

    /* put the pixel_colors in the font structures */
    ite->cur_font.pixel_colors_ptr = ite->pixel_colors;

    /* stuff parameters in current font and underline font too */
    cursor_font = ite->und_font = ite->cur_font; 

    /* cursor has its own pixel table */
    cursor_font.pixel_colors_ptr = ite->FGxorBG;
 
    /* set cursor font pointer to char 257! = background box */
    temp2 = ite->font_bytpchar;
    cursor_font.font_buf = cp = ite->font_start + (256 * temp2);
    while (temp2--)
	*cp++ = 0xff;

    /* set underline font pointer to beginning of "_" character */
    ite->und_font.font_buf = ite->font_start + ('_' * ite->font_bytpchar);

    /* calculate number of bytes per pixel row in font buffer */
    temp2 = (ite->und_font.widthm1 >> 3) + 1;

    /* now find where the actual underline pixels are */
    ite->uline_frame_off = temp1 = 0;
    while (!(*ite->und_font.font_buf)) { 
 	if (temp1++ >= ite->font_height)
	    break;
 	/* bump one pixel line */
 	ite->und_font.font_buf += temp2; 
 	ite->uline_frame_off += ite->framebuf_width;
     }

    /* now set underline = one pixel high */
    ite->und_font.heightm1 = (1-1);
    }
 
    /* calculate the screen size and SFK size */
    ite->screenheight = (ite->framedsp_height / ite->line_height);

    if (ite->screenheight <= 25)	/* can not handle two lines? */
	 ite->sfkheight = 1;		/* one line of softkeys */
    else ite->sfkheight = 2;		/* two lines of softkeys */

    ite->screenheight -= ite->sfkheight;	/* softkeys eat screen space */
    set_sfkdefaults(ite->sfkheight);

    /*
     * Calculate screen width.
     *
     * Unfortunately, the Bobcat lo-resolution screen is 85.333
     * characters wide, which upsets people.  Make it 80.
     */
    ite->screenwidth = ite->framedsp_width / ite->font_width;
    if (ite->screenwidth < 90 && ite->screenwidth >= 80) {
 	ite->screenwidth = 80;
    }
    if (!ite->tabstops) {
	/*
	 *	Changed MALLOC call to kmalloc to save space. When
	 *	MALLOC is called with a variable size, the text is
	 *	large. When size is a constant, text is smaller due to
	 *	optimization by the compiler. (RPC, 11/11/93)
	 */
	ite->tabstops = (unsigned char *) kmalloc((ite->screenwidth + 1),
					   M_ITE, M_WAITOK);
	bzero(ite->tabstops, ite->screenwidth+1);	/* no tabs initially */
    }

    /* Set variables for panic message screen management */
    console_width = ite->screenwidth;
    console_height = ite->screenheight;

    /* plug in high speed font_draw routine for this font width */
    cp = cursor_font.font_buf;
    temp1 = ite->font_bytpchar;

    if (!(ite->flags & STI))
    switch (ite->font_width) {
 	/* special high speed custom routines */
 	case 10:
 		/* for renaissance */
 		ite->crt_font_draw = ite_10by_draw;

		/* form the cursor shape */
		cp[0] = cp[temp1-2] = 0x7f;
		cp[1] = cp[temp1-1] = 0x80;
 		break;

 	case 12:
 		/* for lo-res bobcat */
 		ite->crt_font_draw = ite_12by_draw;

		/* form the cursor shape */
		cp[0] = cp[temp1-2] = 0x00;
		cp[1] = cp[temp1-1] = 0x00;
		cp[2] = cp[temp1-4] = 0x7f;
		cp[3] = cp[temp1-3] = 0xe0;
 		break;
 	default:
 		if (ite->font_width & 7) {
 		    /* plug in low speed font_draw routine */
 		    ite->crt_font_draw = ite_any_draw;
 		}
		else {
 		    ite->crt_font_draw = ite_8modby_draw;

		    /* form the cursor shape */
		    cp[0] = cp[temp1-1] = 0x00;
		    cp[1] = cp[temp1-2] = 0x7e;
 		}
 		break;
    }

    ite->screensize = ite->screenwidth * ite->screenheight;
    ite->maxx = ite->screenwidth-1;
    ite->maxy = ite->screenheight-1;
}

extern bobcat_write(), bobcat_write_off(), bobcat_cursor(), bobcat_clear(),
       bobcat_scroll(), bobcat_init(), bobcat_pwr_reset(), bobcat_set_st();

extern davinci_write(), davinci_write_off(), davinci_cursor(), davinci_clear(),
       davinci_block_clear(), davinci_scroll(), davinci_init(),
       davinci_pwr_reset(), davinci_set_st();

extern tiger_write_setup(), tiger_write_off(), tiger_cursor(), tiger_clear(),
       tiger_block_clear(), tiger_scroll(), tiger_init(), tiger_pwr_reset(),
       tiger_set_st();

extern renais_write(), renais_write_off(), renais_cursor(), renais_clear(),
       renais_block_clear(), renais_scroll(), renais_init(), 
       renais_pwr_reset(), renais_set_st();

extern alpha_write(), alpha_cursor(), alpha_clear(),
       alpha_scroll(), alpha_set_st();

extern gatorbox_write(), gatorbox_cursor(), gatorbox_clear(),
       gatorbox_scroll(), gatorbox_init();

extern sti_write(), sti_cursor(), sti_clear(), sti_scroll(),
       sti_init(), sti_pwr_reset(), sti_set_state(), sti_block_clear();

extern scroll_screen();

/*
** Should only be called once from
** ite_init.
*/
crt_init(ite)
struct iterminal *ite;
{
    register short temp_d7;				/* d7 */
    register char *diostart_a5 = (char *)ite->card;	/* a5 */
    register short *gaid_p1, *gaid_p2;

    ite->scroll_lines = ite->scroll_lines_boot = scroll_lines;

    /* Graphics is initially off, Alpha is initially on,
     * and softkeys are off. 
     */
    if ((sysflg = sysflags) & 0x02)
	sysflg ^= 0x04;

    ite->flags = ALPHAON|GRAPHICSON;
    ite->flags |= NOT_9826;
    ite->crt_reset = bmap_nop;
    ite->crt_pwr_reset = bmap_nop;
    ite->crt_set_state = bmap_nop;
    ite->crt_block_clear = bmap_nop;
    ite->crt_font_restore = bmap_nop;
    ite->crt_big_scroll = scroll_screen;

	if (ite_type==SGC_TYPE) {
		ite->crt_write  = sti_write;
		ite->crt_cursor = sti_cursor;
		ite->crt_clear  = sti_clear;
		ite->crt_scroll = sti_scroll;
		ite->crt_reset  = sti_init;
		ite->crt_pwr_reset = sti_pwr_reset;
		ite->crt_set_state = sti_set_state;
		ite->crt_block_clear = sti_block_clear;
		ite->plane_mask =
			(1<<graphics_ite_dev->glob_config.text_planes)-1;
		ite->cur_planes = ite->plane_mask;

		ite->flags |= STI|BIT_MAPPED;
		if (graphics_ite_dev->glob_config.text_planes > 1)
			ite->flags |= ITE_COLOR;
		ite->card = (unsigned char *)
			graphics_ite_dev->glob_config.region_ptrs[0];
	}
	else if (ite->card != NULL) {		/* NULL means alpha display */
		ite->flags |= BIT_MAPPED;

		/* set up default writes to come from the "from" ram routine */
		ite->crt_write      = itecrt_write;
		ite->crt_write_ram  = itecrt_write;
		ite->crt_write_off  = itecrt_write;

		/* Is there anything there? */
		if (!testr(diostart_a5, 1))
			return(FALSE);

		switch (diostart_a5[0x1] & 0x7f) {
		case 0x39: /* later than gator bit_mapped */
		        
			/* get frame start */
			ite->frame_start = get_frame_addr(diostart_a5);

			/* check if frame buffer is in bounds */
			/*** doesn't work for DIO2 cards
			if ((ite->frame_start != (unsigned char *)0x200000) &&
			    (ite->frame_start !=(unsigned char *)0x300000)){
			    cons_tty = NULL;
			    panic("ITE Framebuf not = 0x200000 or 0x300000");
			}
			***/

			switch (diostart_a5[0x15]) {
			case HPGATOR_SECONDARY_ID:
				ite->flags |= ITE_COLOR;

				/* max of three planes */
				ite->plane_mask = 7;
				/* check if gatoraid present */
				if (testr((diostart_a5+0x8004), 1)) {

					/* reset and halt transform engine */
					diostart_a5[0x8000] = 0xa0;
					diostart_a5[0x8000] = 0x20;
					diostart_a5[0x8000] = 0x80;

					gaid_p1 = gatoraid_microcode;
					gaid_p2 =(short *)(diostart_a5+0xc000);

					/* map 1st bank ucode addrs space */
					diostart_a5[0x8003] = 0x0;

					/* load microcode into gatoraid */
					for (temp_d7=0;
					     temp_d7<GATORAID_UCODE_LEN;
					     temp_d7++)
						*gaid_p2++ = *gaid_p1++;

					/* reset and run transform engine */
					diostart_a5[0x8000] = 0xa0;
					diostart_a5[0x8000] = 0x20;
					diostart_a5[0x8000] = 0x00;

					/* wait for gatoraid non busy */
					for (temp_d7=0;temp_d7<1000;temp_d7++){
						if (diostart_a5[0x8004] < 0)
							continue;
						ite->flags |= GATORAID;
						break;
					}
				}
				ite->crt_write_setup  = gatorbox_write;
				ite->crt_cursor = gatorbox_cursor;
				ite->crt_clear  = gatorbox_clear;
				ite->crt_scroll = gatorbox_scroll;
				ite->crt_reset  = gatorbox_init;
				break;

			case HP98549_SECONDARY_ID:
			case HP98550_SECONDARY_ID:
			case HP98548_SECONDARY_ID:
			case HP98541_SECONDARY_ID:
				ite->flags |= CATSEYE;

				/* Fastcat present? */
				if (ite->card > (unsigned char *) LOGICAL_IO_BASE + 0x600000 &&
				(((short *)diostart_a5)[CAT_STATUS] & 0x08)==0)
					ite->flags |= CAT_TFORM_ENGN;

				/* Fall through to Bobcat/Topcat */

			case HPBOBCT_SECONDARY_ID:
				ite->crt_write  = itecrt_decide;
				ite->crt_write_setup  = bobcat_write;
				ite->crt_write_off  = bobcat_write_off;
				ite->crt_cursor = bobcat_cursor;
				ite->crt_clear  = bobcat_clear;
				ite->crt_scroll = bobcat_scroll;
				ite->crt_reset  = bobcat_init;
				ite->crt_pwr_reset = bobcat_pwr_reset;
				ite->crt_set_state = bobcat_set_st;
				ite->crt_font_restore  = ite_font_restore;

				/* get no. planes mask bits for wait routine */
				if ((temp_d7 = diostart_a5[0x5b]) == 0) {
					*ite->frame_start = -1;
					ite->plane_mask = *ite->frame_start;
				} else {
					while (temp_d7--)
						ite->plane_mask = (ite->plane_mask<<1)+1;
				}
				/* check if there is a color map */
asm("		movp.w	0x33(%a5),%d7		 ");
				if (temp_d7 != 0)
					ite->flags |= ITE_COLOR;
				break;

			case HP98720_SECONDARY_ID:

				/*
				 *  Although the SRX is not supported on the 
				 *  98726A, it will work without problems on
				 *  the 98726A if in DIO address space.  If in
				 *  DIO II, however, an obscure panic occurs.
				 *  We check here so that we can provide a
				 *  meaningful panic message.
				 */
				if (((unsigned int) ite->card < 
				                LOGICAL_IO_BASE) ||
				    ((unsigned int) ite->card > 
				                LOGICAL_IO_BASE + 0x600000)) 
				    panic("SRX is not supported in DIO II");


				ite->flags |= (ITE_COLOR|RENAISSANCE);

				/* max of 3 planes for alpha */
				ite->plane_mask = 7;

				ite->crt_write = itecrt_decide;
				ite->crt_write_setup  = renais_write;
				ite->crt_write_off  = renais_write_off;
				ite->crt_cursor = renais_cursor;
				ite->crt_clear  = renais_clear;
				ite->crt_block_clear = renais_block_clear;
				ite->crt_scroll = renais_scroll;
				ite->crt_reset  = renais_init;
				ite->crt_pwr_reset  = renais_pwr_reset;
				ite->crt_set_state = renais_set_st;
				ite->crt_font_restore  = ite_font_restore;
				
				renais_pwr_reset(ite);
				break;

			case HP98730_SECONDARY_ID:
				ite->flags |= (ITE_COLOR|DAVINCI);

				/* A DaVinci always has overlay planes */
				ite->overlay_planes = TRUE;

				/* max of 3 planes for alpha */
				ite->plane_mask = 7;

				ite->crt_write = itecrt_decide;
				ite->crt_write_setup  = davinci_write;
				ite->crt_write_off  = davinci_write_off;
				ite->crt_cursor = davinci_cursor;
				ite->crt_clear  = davinci_clear;
				ite->crt_block_clear = davinci_block_clear;
				ite->crt_scroll = davinci_scroll;
				ite->crt_reset  = davinci_init;
				ite->crt_pwr_reset  = davinci_pwr_reset;
				ite->crt_set_state = davinci_set_st;
				ite->crt_font_restore  = ite_font_restore;
				
				davinci_pwr_reset(ite);
				break;

			case HP98705_SECONDARY_ID:
				ite->flags |= (ITE_COLOR|TIGERSHARK);

				/* A Tigershark always has overlay planes */
				ite->overlay_planes = TRUE;

				/* ITE only uses 3 planes (8 colors) */
				ite->plane_mask = 0x7;

				ite->crt_write = itecrt_decide;
				ite->crt_write_setup  = tiger_write_setup;
				ite->crt_write_off  = tiger_write_off;
				ite->crt_cursor = tiger_cursor;
				ite->crt_clear  = tiger_clear;
				ite->crt_block_clear = tiger_block_clear;
				ite->crt_scroll = tiger_scroll;
				ite->crt_reset  = tiger_init;
				ite->crt_pwr_reset  = tiger_pwr_reset;
				ite->crt_set_state = tiger_set_st;
				ite->crt_font_restore  = ite_font_restore;
				
				tiger_pwr_reset(ite);
				break;

			case HPWAMPM_SECONDARY_ID:
			case HPBLKFT_SECONDARY_ID:
                                genesis_iterminal_init(ite);
                                genesis_pwr_reset(ite);
                                break;

			case HPHYPER_SECONDARY_ID:
				hyper_iterminal_init(ite);
				hyper_reset(ite);
				break;

			default:
				return(FALSE);
			}
			ite_bitmap_call = ite_bmap_semiphore;
			break;

		default:
			return(FALSE);
		}
		/* set the number of planes enabled masks */
		ite->cur_planes = (ite->plane_mask &= 0x7);

	}
	else if (we_have_alpha()) {
		ite->overlay_planes = TRUE;
		ite->crt_write  = alpha_write;
		ite->crt_write_ram  = alpha_write;
		ite->crt_write_off  = alpha_write;

		ite->crt_cursor = alpha_cursor;
		ite->crt_clear  = alpha_clear;
		ite->crt_scroll = NULL;
		ite->crt_big_scroll = alpha_scroll;
		ite->crt_set_state = alpha_set_st;

#define CRTSFK_26 (0x512704-0x64+LOG_IO_OFFSET)	/* start of CRT display memory
				** with 2 rows of sfk labels     */
#define CRTSFK_36 (0x5121A0-0xA0+LOG_IO_OFFSET)	/* start of CRT display memory
				** with 2 rows of sfk labels     */

		if (sysflg & ALPHA50) {
			ite->screenwidth = 50;
			ite->screenheight = 23;
			ite->frame_start = (unsigned char *) (CRTSFK_26 + 100);
			ite->flags &= ~NOT_9826;

		} else {
			ite->screenwidth = 80;
			ite->screenheight = 24;
			ite->frame_start = (unsigned char *) CRTSFK_36;
			/* Check for color display */
			if (sysflg & CRTCONFREG) 
				if (0x18 & *((char *)(0x51fffe+LOG_IO_OFFSET)))
					ite->flags |= ITE_COLOR;
		}
 		ite->screensize = ite->screenwidth * ite->screenheight;
 		ite->maxx = ite->screenwidth-1;
 		ite->maxy = ite->screenheight-1;
		ite->sfkheight = 1;
		set_sfkdefaults(ite->sfkheight);
		alpha_set_st(ite);	

		/* Set variables for panic message screen management */
		console_width = ite->screenwidth;
		console_height = ite->screenheight;
	}
	else
		return(FALSE);		/* not bitmap or alpha */

	scroller_init(HARD);
	return(TRUE);
}
