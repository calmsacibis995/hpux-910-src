/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_hyper.c,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:59:35 $
 */

/*
 * Device specific ITE code for the Monochrome Hyperion/Frodo graphics display.
 */

#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_scroll.h"
#include "../graf/gr_hyper.h"

#define hyper_xy_to_ubytep(ite, xxx, yyy) (unsigned char *)\
	(ite->frame_start + \
	 ( (ite->framebuf_width * (yyy * ite->line_height)) + \
	   ((ite->font_width * (xxx))) )/BPB )
		
#define BPW	32					    /* bits per word */
#define BPB	8					    /* bits per byte */

static int hyper_left_mask[] = {
    0x00,	/* 0 - aligned case'    */
    0x00,	/* 1 - does not happen */
    0xc0,	/* 2 */
    0xc0,	/* 3 - does not happen */
    0xf0,	/* 4 */
    0xf0,	/* 5 - does not happen */
    0xfc,	/* 6 */
    0xfc	/* 7 - does not happen */
};

static int hyper_right_mask[] = {
    0x00,	/* 0 - aligned case    */
    0x00,	/* 1 - does not happen */
    0x03,	/* 2 */
    0x03,	/* 3 - does not happen */
    0x0f,	/* 4 */
    0x0f,	/* 5 - does not happen */
    0x3f,	/* 6 */
    0x3f	/* 7 - does not happen */
};

extern bmap_nop();

/*
 * hyper_reset() performs a logical reset of the display.
 *
 * This merely involves clearing the display and re-enabling the monitor
 * display bit.
 */
hyper_reset(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    idrom_init((unsigned char *)(ite->card));
}

/*
 * hyper_clear() clears "count" characters begining a logical screen
 *               location <x,y>.
 *
 * For each line of pixels on the screen (indexed by "y"):
 *    it first clears the part of the line that is word aligned
 *        ("num_aligned" pixels wide)
 *    then it clears the non-aligned portion by taking the contents already
 *        in that location and and-ing with with zero bits ("mask") for the
 *        number of unaligned pixels that remain along the right edge of the
 *        screen ("remainder").
 * We go to this trouble because it is possible that Starbase may be using
 *     off-screen memory for something.
 *
 * IMPORTANT NOTE: This routine can be called to clear more characters than
 *                 just from one logical line.  For instance, when the
 *	           scroller wants to clear the whole screen, it starts at
 *                 <0,0> and clears (ite->screensize = ite->screen_height *
 *                 ite->screen_width) characters.
 */
hyper_clear(ite, y, x, count)					    /* ENTRY */
    struct iterminal *ite;
{
    register unsigned char *ul_destp, *p;
    register xi, yi;
    unsigned notouch_left, notouch_right, mask_left, mask_right, lineadvance;
    int len, min_x_pixel, max_x_pixel, full_bytes, min_y_pixel, max_y_pixel;

    if (count <= 0)
	return;				/* oooh, I hate when this happens... */

    /*
     * (First) area to be cleared should be no longer than to the right
     * edge of the screen.
     */
    len = scr_min(ite->screenwidth - x, count);

    ul_destp = hyper_xy_to_ubytep(ite, x, y);
    lineadvance = (ite->framebuf_width / BPB);

    while (count) {
	count        -= len;
	min_x_pixel   = (ite->font_width * x);
	max_x_pixel   = min_x_pixel + (len * ite->font_width);
	full_bytes    = (max_x_pixel/BPB) - ((min_x_pixel+BPB-1)/BPB);
	notouch_left  = min_x_pixel % BPB;
	notouch_right = (BPB - (max_x_pixel % BPB)) % BPB;
	mask_left     = hyper_left_mask[notouch_left];
	mask_right    = hyper_right_mask[notouch_right];

	p = ul_destp;
	for (yi=0; yi<ite->line_height; yi++) {
	    if (notouch_left)		   /* clear any partial leading part */
		*p++ &= mask_left;
	    for (xi=0; xi<full_bytes; xi++)	     /* clear all full bytes */
		*p++ = 0;
	    if (notouch_right)		  /* clear any partial trailing part */
		*p &= mask_right;
	    p = (ul_destp += lineadvance);
	}

	/*
	 * Prepare for the next block to clear.  Bump "y" to the next line and
	 * always set x to be 0 since all lines after the first start in the
	 * first column (i.e. left edge -> 0).  Recompute the beginning of the
	 * upper-left corner of the next rectangle to be cleared (ul_destp).
	 */
	len = scr_min(ite->screenwidth, count);
	x = 0;
	y++;
	ul_destp = hyper_xy_to_ubytep(ite, x, y);
    }
}

/*
 * hyper_scroll() scrolls a single line either up or down depending on "dir".
 *
 * Unlike hyper_clear(), we do not worry about any un-aligned trailing pixels
 * because the remainder of the line is going to be blank (so it does not
 * matter if we trash anything beyond the "exact" end-of-line.  For this
 * reason, we round the number of words to be scrolled up to the next word
 * boundary.
 *
 * The total width (in pixels) of the region to be scrolled is "pixel_width"
 * with the number of aligned words to be transfered being "words_to_scroll".
 *
 * "to_row" is the "dest"ination of the scroll, with the "source" being
 * one line of characters below ("ite->line_height" pixels) if "dir" is
 * UP, or the same amount above the destination if "dir" is DOWN.
 */
hyper_scroll(ite, to_row, dir, width)				    /* ENTRY */
    struct iterminal *ite;
{
    int x_max, leftover, mask, z;
    register int x, y, full_bytes, bytes_per_fbline, lineadvance;
    register unsigned char *start_source, *start_dest, *sp, *dp;

    if (width == 0)
	return;				/* oooh, I hate when this happens... */

    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, to_row);
	return;
    }

    lineadvance = (ite->framebuf_width / BPB);
    x_max = width * ite->font_width;
    full_bytes = x_max / BPB;
    leftover   = x_max % BPB;
    mask       = hyper_left_mask[leftover];

    bytes_per_fbline = (ite->framebuf_width + ite->line_height)/BPB;

    start_dest = hyper_xy_to_ubytep(ite, 0, to_row);

    z = to_row + ((dir == UP) ? 1 : -1);
    start_source = hyper_xy_to_ubytep(ite, 0, z);

    for (y=0; y<ite->line_height; y++) {
	sp = start_source;	    /* point to leftmost (source) line pixel */
	dp = start_dest;	    /* point to leftmost (destn.) line pixel */
	for (x=0; x<full_bytes; x++)
	    *dp++ = *sp++;
	if (leftover)			 /* scroll unaligned trailing pixels */
	    *dp = (*sp & mask);
	start_source += lineadvance;	   /* recompute next new source line */
	start_dest   += lineadvance;	   /* recompute next new destn. line */
    }
}

/*
 * hyper_write() writes "count" character(s) in the scroll buffer pointed to
 * by "vlp" beginning at logical character location <x,y> on the screen.
 *
 * Unlike hyper_clear() and hyper_scroll(), the transfers are done in byte
 * units because it is easier to do.
 *
 * The following is a table that describes the issues involved in preserving
 * characters to the left and right of where the new characters will be
 * placed.  "ntl" means "notouch_left" and "a" and "b" are the pixels from
 * the first byte (leftmost) of the glyph, and from the second (rightmost)
 * byte of the glyph, respectively.
 *
 * IMPORTANT NOTE: The table, of course, assumes a glyph width of 10 bits.
 *
 * byte #   0        1        2        3        4        5        6
 * bit# |01234567|01234567|01234567|01234567|01234567|01234567|01234567
 * ntl  |        |        |        |        |        |        |        |
 *  0   |        |aaaaaaaa|bb      |        |        |        |        |
 *      |        |        |        |        |        |        |        |
 *  2   |        |  aaaaaa|aabb    |        |        |        |        |
 *      |        |        |        |        |        |        |        |
 *  4   |        |    aaaa|aaaabb  |        |        |        |        |
 *      |        |        |        |        |        |        |        |
 *  6   |        |      aa|aaaaaabb|        |        |        |        |
 *      |        |        |        |        |        |        |        |
 */
hyper_write(ite, vlp, y, x, count)				    /* ENTRY */
    struct iterminal *ite;
    register SCROLL *vlp;
{
    register unsigned char *ul_destp,    /* upper left corner of destination */
	*sp,					       /* iterator on screen */
	bwidth = (ite->font_width+(BPB-1))/BPB,		   /* width in bytes */
	c0, c1;
    unsigned char *charp,   /* pointer to glyph char. within ite->font_start */
	reverse;
    int first_pixel,		 /* starting absolute pixel number on screen */
	scroll_value,		/* SCROLL sized datum from the scroll buffer */
	charval,	     /* the "real" character portion of scroll_value */
	notouch_left,		/* number of pixels to the left not to touch */
	row,			        /* iterator for each character glyph */
	lineadvance = (ite->framebuf_width/BPB);

    if (count <= 0)
	return;				/* oooh, I hate when this happens... */

    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    /*
     * Find the absolute starting pixel for the write from the left edge of
     * the screen (the left edge of the screen is guaranteed to be byte-
     * aligned).  All pixels/bits between this point and the first pixel
     * boundary behind (to the left of) it must remain untouched.  If the
     * first character to be generated lies on a byte boundary, "notouch_left"
     * will be 0.
     */

    for ( ; count>0 ; count--, x++) {
	scroll_value = *vlp++;

	if (!(scroll_value & CHAR_ON))
	    continue;

	charval      = scroll_value & CHAR_VAL;
	first_pixel  = (ite->font_width * x);
        notouch_left = first_pixel % BPB;
	charp        = ite->font_start + (ite->font_bytpchar * charval);

	ul_destp = hyper_xy_to_ubytep(ite, x, y);

	for (row=0; row<ite->line_height; row++) {
	    /* move to leftmost pixel column in the next row */
	    sp = ul_destp + (row * lineadvance);

	    if (scroll_value & INVERSE_VIDEO) {
		c0 = ~(*charp++);
		c1 = ~(*charp++);
	    }
	    else {
		c0 = *charp++;
		c1 = *charp++;
	    }

	    /*
	     * If this is the bottom row of an underlined character, replace
	     * the row with an underline (set all the bits).
	     */
	    if ((row==(ite->line_height-1)) && (scroll_value & UNDERLINE))
		c0 = c1 = 0xff;

	    /*
	     * If we are color pair 7 (which is also used for softkeys)
	     * reverse the sense of the character.
	     */
	    if (char_colorpair(scroll_value) == ITE_SOFTKEYS_CPAIR) {
		c0 = ~c0;
		c1 = ~c1;
	    }

	    /*
	     * Lay down each of the two bytes of the glyph.
	     */
	    switch (notouch_left) {
		case 0:
		    *sp++  = c0;
		    *sp   &= 0x3f;
		    *sp   |= (c1 & 0xc0);
		    break;
		case 2: 
		    *sp   &= 0xc0;
		    *sp++ |= (c0 & 0xfc) >> 2;
		    *sp   &= 0x0f;
		    *sp   |= (c0 & 0x03) << 6;
		    *sp   |= (c1 & 0xc0) >> 2;
		    break;
		case 4: 
		    *sp   &= 0xf0;
		    *sp++ |= (c0 & 0xf0) >> 4;
		    *sp   &= 0x03;
		    *sp   |= (c0 & 0x0f) << 4;
		    *sp   |= (c1 & 0xc0) >> 4;
		    break;
		case 6: 
		    *sp   &= 0xfc;
		    *sp++ |= (c0 & 0xc0) >> 6;
		    *sp    = (c0 & 0x3f) << 2;
		    *sp   |= (c1 & 0xc0) >> 6;
		    break;
		default:
		    break;
	    }
	} /* for row */
    } /* for count */
}

hyper_cursor(ite, x, y, color_num)				    /* ENTRY */
    register struct iterminal *ite;
{
    unsigned char *lp,		 /* pointer to beginning of every pixel line */
	*sp,					       /* iterator on screen */
	c, c0, c1;
    register row;
    int first_pixel, lineadvance;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    lp = hyper_xy_to_ubytep(ite, x, y);
    first_pixel = (ite->font_width * x);
    lineadvance = (ite->framebuf_width/BPB);

    for (row=0; row<ite->line_height; row++) {
	sp = lp;
	c = *sp;		 /* get first of two characters to dork with */
	switch ((ite->font_width * x) % BPB) {		     /* not to touch */
	    case 0:
		*sp = ~c;		  /* invert first character in-place */
		c = *(++sp);
		c0 = (~(c & 0xc0)) & 0xc0;		/* c0 = under cursor */
		c1 = c & 0x3f;
		*sp = c0 | c1;
		break;
	    case 2: 
		c0 = c & 0xc0;
		c1 = (~(c & 0x3f)) & 0x3f;		/* c1 = under cursor */
		*sp++ = c0 | c1;
		c = *sp;
		c0 = (~(c & 0xf0)) & 0xf0;		/* c0 = under cursor */
		c1 = c & 0x0f;
		*sp = c0 | c1;
		break;
	    case 4: 
		c0 = c & 0xf0;
		c1 = (~(c & 0x0f)) & 0x0f;		/* c1 = under cursor */
		*sp++ = c0 | c1;
		c = *sp;
		c0 = (~(c & 0xfc)) & 0xfc;		/* c0 = under cursor */
		c1 = c & 0x03;
		*sp = c0 | c1;
		break;
	    case 6: 
		c0 = c & 0xfc;
		c1 = (~(c & 0x03)) & 0x03;
		*sp++ = c0 | c1;
		*sp = ~(*sp);
		break;
	    default:
		panic("hyper_cursor(): unexpected alignment");
		break;
	}
	lp += lineadvance;
    }
    ite->cursor_on ^= TRUE;		    /* toggle on/off state of cursor */
}

/*
 * hyper_iterminal_init() is called from the initialization portion of
 * ite_driv.c that hooks function pointers into the iterminal structure.
 */
hyper_iterminal_init(ite)					    /* ENTRY */
    struct iterminal *ite;
{
    ite->crt_write        = hyper_write;
    ite->crt_write_setup  = hyper_write;
    ite->crt_write_off    = hyper_write;
    ite->crt_cursor       = hyper_cursor;
    ite->crt_clear        = hyper_clear;
    ite->crt_scroll       = hyper_scroll;
    ite->crt_reset        = hyper_reset;
    ite->crt_pwr_reset    = hyper_reset;
    ite->crt_set_state    = bmap_nop;
    ite->crt_font_restore = bmap_nop;
}
