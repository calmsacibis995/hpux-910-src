/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_sti.c,v $
 * $Revision: 1.3.84.5 $	$Author: jwe $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/11 10:02:04 $
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

#include "../graf/ite.h"
#include "../graf/ite_scroll.h"
#include "../graf.300/graphics.h"
#include "../h/debug.h"

#ifndef true
#  define true 1
#  define false 0
#endif

#define STI_SAVED REN_SAVED_REG		/* borrow a flag bit */

extern struct graphics_descriptor *graphics_ite_dev;

#define sti_call(routine,a,b,c) \
     (*graphics_ite_dev->routine)((a),(b),(c), &graphics_ite_dev->glob_config)

union {
	blkmv_flags b_flags;
	blkmv_inptr b_in;
	font_flags f_flags;
	font_inptr f_in;
	init_flags i_flags;
	init_inptr i_in;
	state_flags s_flags;
	state_inptr s_in;
} sti_zero;

/*
 * This routine writes characters to the screen.  It will
 * call FONT_UNP/MV() once for each character.  If the
 * character is inverse video, reverse foreground and
 * background.  If the character is underlined, underline
 * it by calling BLOCK_MOVE to write (clear to the
 * foreground color) the underline.  Blinking & half-bright
 * are ignored, as they always have been.
 */
sti_write(ite, buf, y, x, count)
struct iterminal *ite;		/* big structure of ITE stuff */
SCROLL *buf;			/* buffer of characters & attributes */
int y,x;			/* Where to write (character coordinates) */
int count;			/* length of buf */
{
	register int cpair, temp;
	register SCROLL scrollvalue;
	register struct sti_rom *sti;
	register struct sti_font *f, *roman, *katakana;
	blkmv_flags  b_flags;
	blkmv_inptr  b_in;
	blkmv_outptr b_out;
	font_flags  f_flags;
	font_inptr  f_in;
	font_outptr f_out;

	save_sti_state(ite);		/* Save the state of the STI card. */

	b_flags = sti_zero.b_flags;
	b_in = sti_zero.b_in;
	f_flags = sti_zero.f_flags;
	f_in = sti_zero.f_in;

	if (count<=0) return;

	sti = (struct sti_rom *) ite->card;
	roman = (struct sti_font *)
			(((int) sti) + get_int(sti->font_start) & ~03);

	/* Look for a katakana font */
	if (get_int(roman->offset)==0)
		katakana = roman;
	else
		katakana = (struct sti_font *)
				(((int) roman)+get_int(roman->offset) & ~03);

	f_flags.wait = true;
	f_in.font_start_addr = ((int) sti) + get_int(sti->font_start);
	f_in.dest_y = y * ite->line_height;
	f_in.dest_x = x * ite->font_width;

	while (count--) {
		scrollvalue = *buf++;
		if (!(scrollvalue & CHAR_ON))
			break;
		f_in.index = ite->char_out[scrollvalue & CHAR_VAL];
		f = roman;
		if (f_in.index >= 256) {
			f = katakana;
			f_in.index -= 96;	/* Scale back to Katakana */
		}
		f_in.font_start_addr = (int) f;
		cpair = char_colorpair(scrollvalue);
		f_in.fg_color = ite->color_pairs[cpair].FG;
		f_in.bg_color = ite->color_pairs[cpair].BG;

		if (scrollvalue & INVERSE_VIDEO) {
			temp = f_in.fg_color;
			f_in.fg_color = f_in.bg_color;
			f_in.bg_color = temp;
		}
		sti_call(font_unpmv, &f_flags, &f_in, &f_out);

		if (scrollvalue & UNDERLINE) {
			b_flags.wait = true;
			b_flags.clear = true;

			b_in.bg_color = f_in.fg_color;
			b_in.dest_y = f_in.dest_y+f->underline_offset;
			b_in.dest_x = f_in.dest_x;
			b_in.width = ite->font_width;
			b_in.height = f->underline_height;
			sti_call(block_move, &b_flags, &b_in, &b_out);
		}

		f_in.dest_x += ite->font_width;
	}
}

/*
 * This routine puts a cursor on the screen, or removes one.
 * Sometimes the ITE removes a cursor by overwriting it,
 * and sometimes it's explicitly removed by calling this routine.
 * The cursor is put up by writing the appropriate
 * character in inverse video.  If the cursor is not over a
 * character, write an inverse video blank.
 *
 * Also, restore the state of the graphics card.
 */

/* Patch this to UNDERLINE (0x200) for an underline cursor rather than a box */
int sti_cursor_enhance = INVERSE_VIDEO;

sti_cursor(ite, x,y, scroll)
struct iterminal *ite;	/* big structure of ITE stuff */
int x,y;		/* Where to write (character coordinates) */
int scroll;		/* Color pair number (0-7) */
{
	SCROLL s;

	save_sti_state(ite);		/* Save the state of the STI card. */

	s = scroll;				/* What's here, anyway? */

	if (!(s & CHAR_ON))			/* Is there a character here? */
		s = ' '|CHAR_ON;		/* No, use a blank */
	if (!ite->cursor_on)			/* Cursor or char rewrite? */
		s ^= sti_cursor_enhance;	/* This is a cursor. */
	sti_write(ite, &s, y, x, 1);		/* Write the character */
	ite->cursor_on = !ite->cursor_on;

	if (ite->cursor_on)
		restore_sti_state(ite);
}

/*
 * This routine clears a number of character cells.  Note
 * that this is not necessarily a rectangular area, but
 * rather a series of character cells, which might wrap
 * around past the end-of-line.  For example, to clear an
 * 80x24 screen:  sti_clear(ite, 0,0, 80*24).  This routine
 * will call sti_block_clear a few times to do the real work.
 */

sti_clear(ite, y, x, count)
struct iterminal *ite;		/* big structure of ITE stuff */
int y,x;			/* Where to clear (character coordinates) */
int count;			/* how many characters to clear */
{
	register int fw = ite->font_width, fh = ite->line_height;
	register int width, height;

	save_sti_state(ite);		/* Save the state of the STI card. */

	/*
	 * The area to be cleared can be viewed as follows:
	 *
	 *             aaaaaaaaaaaaaaaaaaaaaaa
	 *  bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
	 *  bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
	 *  bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
	 *  cccccc
	 *
	 * We treat this as three rectangular regions: a, b, & c.
	 * If we are lucky, some of them are empty.
	 * For the full-screen case, only region b exists.
	 */

	/* Region a */
	if (x>0) {
		width = scr_min(ite->screenwidth-x, count);
		sti_block_clear(ite, x*fw, y*fh, fh, width*fw);
		y++;
		count -= width;
	}

	/* Region b */
	height = count/ite->screenwidth;
	if (height>0) {
		sti_block_clear(ite, 0, y*fh, height*fh, ite->screenwidth*fw);
		y+=height;
		count-=height*ite->screenwidth;
	}

	/* Region c */
	if (count>0)
		sti_block_clear(ite, 0, y*fh, fh, count*fw);
}

/*
 * This routine clears an area by calling BLOCK_MOVE().
 */

sti_block_clear(ite, x, y, height, width)
struct iterminal *ite;	/* big structure of ITE stuff */
int y,x;		/* Where to clear (pixel coordinates) */
int height, width;	/* How much to clear (pixel coordinates) */
{
	blkmv_flags  b_flags;
	blkmv_inptr  b_in;
	blkmv_outptr b_out;

	save_sti_state(ite);		/* Save the state of the STI card. */

	b_flags = sti_zero.b_flags;
	b_in = sti_zero.b_in;

	b_flags.wait = true;
	b_flags.clear = true;
	b_in.bg_color = ite->color_pairs[0].BG;
	b_in.dest_x = x;
	b_in.dest_y = y;
	b_in.width = width;
	b_in.height = height;

	sti_call(block_move, &b_flags, &b_in, &b_out);
}

/*
 * This routine scrolls a single line up or down by calling BLOCK_MOVE().
 */

sti_scroll(ite, to_row, dir, width)
register struct iterminal *ite;	/* big structure of ITE stuff */
register int to_row;		/* destination row (character coordinates) */
register int dir;		/* UP or DOWN */
register int width;		/* How wide an area to scroll (char coords) */
{
	blkmv_flags  b_flags;
	blkmv_inptr  b_in;
	blkmv_outptr b_out;

	save_sti_state(ite);		/* Save the state of the STI card. */

	b_flags = sti_zero.b_flags;
	b_in = sti_zero.b_in;

	/* calculate width of line to move */
	if (width == 0)
		return;
	width *= ite->font_width;

	b_in.dest_y = to_row * ite->line_height;
	b_in.src_y = b_in.dest_y +
			((dir == UP) ? ite->line_height : -ite->line_height);
	b_in.width = width;
	b_in.height = ite->font_height;

        b_flags.wait = true;

        sti_call(block_move, &b_flags, &b_in, &b_out);
}


/* Convert a plane mask (value 1-7) to a plane number.  */
		       /* 0 1 2 3 4 5 6 7 */
char mask_to_planes[8] = {1,1,2,2,3,3,3,3};

/*
 * This routine turns the alpha/graphics planes on/off,
 * according to the ite->flags bits GRAPHICSON and ALPHAON.
 * INIT_GRAPH() will be called to enable/disable the text/non-text planes.
 */

sti_set_state(ite)
struct iterminal *ite;			/* big structure of ITE stuff */
{
	init_flags  i_flags;
	init_inptr  i_in;
	init_outptr i_out;

	i_flags = sti_zero.i_flags;
	i_in = sti_zero.i_in;

	save_sti_state(ite);		/* Save the state of the STI card. */

	i_flags.wait = true;
	i_flags.no_chg_bet = true;	/* Don't change berr timer settings. */
	i_flags.no_chg_bei = true;	/* Don't change berr int settings. */
	i_in.text_planes = mask_to_planes[ite->cur_planes];
	i_flags.text = !!(ite->flags & ALPHAON);
	i_flags.nontext = !!(ite->flags & GRAPHICSON);

	sti_call(init_graph, &i_flags, &i_in, &i_out);
}
	
/*
 * This is a soft reset routine.
 * Call INIT_GRAPH() to:
 * 	set up colormap entries
 * 	enable/disable alpha/graphics planes
 * 	do not clear text planes
 * 	do not disable graphics planes
 */
sti_init(ite)
struct iterminal *ite;	/* big structure of ITE stuff */
{
	init_flags  i_flags;
	init_inptr  i_in;
	init_outptr i_out;

	save_sti_state(ite);		/* Save the state of the STI card. */

	i_flags = sti_zero.i_flags;
	i_in = sti_zero.i_in;

	i_flags.wait = true;
	i_flags.reset = true;
	i_flags.text = true;
	i_flags.no_chg_bet = true;	/* Don't change berr timer settings. */
	i_flags.no_chg_bei = true;	/* Don't change berr int settings. */
	i_flags.no_chg_ntx = true;	/* Don't change non-text planes. */
	i_flags.init_cmap_tx = true;	/* initialize color map */
	i_in.text_planes = mask_to_planes[ite->cur_planes];

	sti_call(init_graph, &i_flags, &i_in, &i_out);
}

#define get_short(addr) ((((int *) (addr))[0] & 0xff)<<8 | \
			 (((int *) (addr))[1] & 0xff))

extern caddr_t kmem_alloc();

/*
 * This is a hard reset routine.
 * 
 * Call INIT_GRAPH() to:
 * 	clear the text planes
 * 	set graphics colormaps to black
 */

sti_pwr_reset(ite)
struct iterminal *ite;	/* big structure of ITE stuff */
{
	glob_cfg *g;
	struct sti_rom *sti;
	struct sti_font *f;
	init_flags  i_flags;
	init_inptr  i_in;
	init_outptr i_out;

	/* ROM definition good? */
	if ((int) (((struct sti_rom *) 0) -> inq_conf) != 0x260)
		panic("struct sti_rom messed up");

	/* Font definition good? */
	if ((int) &(((struct sti_font *) 0) -> underline_offset) != 0x37)
		panic("struct sti_font messed up");

	i_flags = sti_zero.i_flags;
	i_in = sti_zero.i_in;

	i_flags.wait = true;
	i_flags.reset = true;
	i_flags.clear = true;
	i_flags.cmap_blk = true;
	i_flags.text = true;
	i_flags.nontext = true;
	i_flags.enable_be_timer = true;
	i_flags.init_cmap_tx = true;
	i_in.text_planes = 3;

	sti_call(init_graph, &i_flags, &i_in, &i_out);

	g = &graphics_ite_dev->glob_config;
	ite->framebuf_width = g->total_x;
	ite->framebuf_height = g->total_y;
			
	ite->framedsp_width = g->onscreen_x;
	ite->framedsp_height = g->onscreen_y;

	sti = (struct sti_rom *) ite->card;

	f = (struct sti_font *) (((int) sti) + get_int(sti->font_start) & ~03);

	/* font size */
	ite->line_height = ite->font_height = f->height;
	ite->font_width = f->width;

	ite->flags &= ~STI_SAVED;	/* It ain't saved now! */
}

caddr_t sti_save_space;
extern int ite_memory_used;

save_sti_state(ite)
struct iterminal *ite;
{
	state_flags s_flags;
	state_inptr s_in;
	state_outptr s_out;
	init_flags  i_flags;
	init_inptr  i_in;
	init_outptr i_out;

	register struct sti_rom *sti = (struct sti_rom *) ite->card;

	if (!ite_memory_used)		/* does anybody care about the state? */
		return;			/* no, don't bother */

	if (ite->flags & STI_SAVED)
		return;
	ite->flags |= STI_SAVED;		/* avoid recursion */

	s_flags = sti_zero.s_flags;
	s_in = sti_zero.s_in;
	i_flags = sti_zero.i_flags;
	i_in = sti_zero.i_in;

	if (sti_save_space == 0) {
		sti_save_space = kmem_alloc(get_int(sti->max_state_storage)*sizeof(int));
		if (sti_save_space==NULL)
			panic("save_sti_state: can't allocate");
	}
	s_flags.wait = true;
	s_flags.save = true;
	s_in.save_addr = (int) sti_save_space;

	woody_fix_save();
	sti_call(state_mgmt, &s_flags, &s_in, &s_out);

	/* Now we have to initialize the device */

	i_flags.wait = true;
	i_flags.no_chg_bet = true;	/* Don't change berr timer settings. */
	i_flags.no_chg_bei = true;	/* Don't change berr int settings. */
	i_flags.no_chg_tx = true;	/* Don't change berr timer settings. */
	i_flags.no_chg_ntx = true;	/* Don't change berr int settings. */
	i_in.text_planes = mask_to_planes[ite->cur_planes];

	sti_call(init_graph, &i_flags, &i_in, &i_out);
}


restore_sti_state(ite)
struct iterminal *ite;
{
	state_flags s_flags;
	state_inptr s_in;
	state_outptr s_out;
	register struct sti_rom *sti = (struct sti_rom *) ite->card;

	if (!(ite->flags & STI_SAVED))
		return;
	ite->flags &= ~STI_SAVED;

	VASSERT(sti_save_space);		/* Must've done this already */

	s_flags = sti_zero.s_flags;
	s_in = sti_zero.s_in;

	s_flags.wait = true;
	s_in.save_addr = (int) sti_save_space;

	sti_call(state_mgmt, &s_flags, &s_in, &s_out);
	woody_fix_restore();
}

/* Space to save Mace stuff for patch */
static int ByteShift, BlockMode;

woody_fix_save()
{
	register struct sti_rom *sti = (struct sti_rom *) iterminal.card;
	int id1=get_int(&sti->graphics_id[0]),
	    id2=get_int(&sti->graphics_id[4]);
	char *cs = (char *)graphics_ite_dev->glob_config.region_ptrs[2];

	if (id2==0x40A00499 && 
	   (id1==0x27134C8E		/* 1024x768  color*/
	    || id1==0x27134C9F		/* 1280x1024 color*/
	    || id1==0x27134CC5		/* 1280x1024 grey scale*/
	    || id1==0x27134CB4		/* 640x480 color*/
	    || id1==0x2739d2f2)) {	/* 640x480 grey scale*/
		/* Now actually save it */
		ByteShift = cs[0x020603];
		cs[0x020603] = 1;	/* disable byte shift */
		BlockMode = cs[0x02060b];
		cs[0x02060b] = 1;	/* disable block mode */
	}
}


woody_fix_restore()
{
	register struct sti_rom *sti = (struct sti_rom *) iterminal.card;
	int id1=get_int(&sti->graphics_id[0]),
	    id2=get_int(&sti->graphics_id[4]);
	char *cs = (char *)graphics_ite_dev->glob_config.region_ptrs[2];

	if (id2==0x40A00499 && 
	   (id1==0x27134C8E		/* 1024x768  color*/
	    || id1==0x27134C9F		/* 1280x1024 color*/
	    || id1==0x27134CC5		/* 1280x1024 grey scale*/
	    || id1==0x27134CB4		/* 640x480 color*/
	    || id1==0x2739d2f2)) {	/* 640x480 grey scale*/
		cs[0x02060b] = 1;	/* Clear BlockMode */
		cs[0x020603] = ByteShift;
		cs[0x02060b] = BlockMode;
	}
}
