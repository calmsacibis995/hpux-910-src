/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_font.c,v $
 * $Revision: 1.5.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 19:00:28 $
 */

#include "../h/param.h"					       /* for NULL ! */
#include "../graf.300/ite_ren.h"	      /* for renaissance_kata_font[] */
#include "../graf.300/ite_top.h"		   /* for lo_res_kata_font[] */
#include "../graf/ite.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"					  /* CHAR_ON */

#ifdef lint
    struct pte { int foo; };
    struct proc { int foo; };
    struct dux_mbuf { int foo; };
#endif

#define REGION0 0
#define REGION1 1
#define REGION2 2

#define WORD_AT(addr) ((*((unsigned char *) addr)<<8)	\
		    + (*(((unsigned char *) addr) + 2)))

/* 96 chars, 10 pixels tall, 15 pixels (two bytes) tall */
#define ren_font_size (96*10*2)

struct offscreen_font offscreen_font[ITE_NGLYPHS];

extern caddr_t *calloc();

/*
 *  The ID/FONT ROM contains an initialization sequence.  It should be used
 *  wherever possible (it does not work for Gator, you have to look at some
 *  switches to determine which init region to use).
 *
 *  As of release 7.03, a new control bit (bit 2) has been defined for dealing
 *  with longwords for data movement and long displacements for getting to
 *  the framebuffer from the ID/FONT ROM base.
 * 
 *  We wait 4 seconds for the bittest command to succeed.  If it does not do
 *  so in this time period, idrom_init() will fail.  We make the wait time
 *  a variable so that we may poke it with adb if necessary.
 */

#define IDROM_CTL_LASTBLOCK	0x80
#define IDROM_CTL_REPL2WORDS	0x40
#define IDROM_CTL_LONGWRITE	0x04
#define IDROM_CTL_BITTEST	0x01		      /* clear means "write" */
#define WAIT_ONE_MILLISEC	snooze(1000)
int 	IDROM_BITTEST_WAIT_MS = 4000;				/* 4 seconds */

int
idrom_init(card)
unsigned char *card;
{
    register unsigned char *init;
    register unsigned char opcode;				 /* "byte 1" */
    register count, milli_wait;					 /* "byte 3" */
    register unsigned doing_longwords, offset;
    register unsigned short *addr;		      /* for word-sized data */
    unsigned short data, data2, test_val;
    register unsigned int *laddr;		  /* for longword-sized data */
    unsigned int ldata;
    register unsigned int laddr_offset;

#define WORD_AT(addr) ((*((unsigned char *) addr)<<8)	\
		    + (*(((unsigned char *) addr) + 2)))

    
    if (!testr(card, 1) || (offset = WORD_AT(card+0x23)) == 0)
	return(0);		   /* card not responding or no init region? */

    init = card + offset;
    while (1) {
	opcode = *init;
	init += 2;			/* skip over "opcode" in ROM */

	/* choke if there is a bit set that we do not recognize */
	if (opcode & ~(IDROM_CTL_LASTBLOCK | IDROM_CTL_REPL2WORDS |
		       IDROM_CTL_LONGWRITE | IDROM_CTL_BITTEST))
		panic("idrom_init(): undefined control bit");

	/* choke on control restriction mentioned in IDROM spec */
	if ((opcode & (IDROM_CTL_REPL2WORDS|IDROM_CTL_BITTEST)) ==
		      (IDROM_CTL_REPL2WORDS|IDROM_CTL_BITTEST))
		panic("idrom_init(): undefined control combination");

	doing_longwords = (opcode & IDROM_CTL_LONGWRITE) ? 1 : 0;

#	ifndef FIXME
	    if (opcode == 0x4)
		opcode = 0x44;
#	endif

	if (doing_longwords) {
	    count = (init[0] << 24) + (init[2] << 16) +
		    (init[4] <<  8) +  init[6];
	    init += 8;			       /* skip over "count" longword */
	    laddr_offset = (init[0] << 24) + (init[2] << 16) +
			   (init[4] <<  8) +  init[6];
	    laddr = (unsigned int *)(card + laddr_offset);
	    init += 8;			   /* skip over destination longword */
	}
	else {
	    count = *init;
	    init += 2;				   /* skip over "count" byte */

	    addr  = (unsigned short *)(card + (init[0]<<8) + init[2]);
	    init += 4;			       /* skip over destination word */
	}

	if (opcode & IDROM_CTL_BITTEST) {
	    if (doing_longwords)
		panic("idrom_init(): cannot do longword bittests yet");
	    else {
		for (milli_wait = IDROM_BITTEST_WAIT_MS; --milli_wait > 0; ) {

		    if (!scopy_prot(addr, &test_val, 1))
			return(0);
		    if (((test_val >> init[2]) & 0x01) == init[0])
			break;

		    WAIT_ONE_MILLISEC;
		}
		if (milli_wait == 0)
		    return(0);
		init += 4;
	    }
	}
	else if (opcode & IDROM_CTL_REPL2WORDS) {
	        /* replicate the following two words "count"/2 times */
		if (doing_longwords) {
		    ldata = (init[0] << 24) + (init[2] << 16) +
			    (init[4] <<  8) +  init[6];
		    init += 8;		       /* skip over "count" longword */
		    do {
			if (!lcopy_prot(&ldata, laddr, 1)) {
			    return(0);
			}
			laddr++;
			count -= 4;
		    } while (count >= 0);
		}
		else {
		    data  = (init[0]<<8) + init[2];
		    init += 4;
		    data2 = (init[0]<<8) + init[2];
		    init += 4;
		    do {
			if (!scopy_prot(&data, addr, 1)) {
			    return(0);
			}
			addr++;
			if (!scopy_prot(&data2, addr, 1)) {
			    return(0);
			}
			addr++;
			count -= 2;
		    } while (count >= 0);
		}
	}
	else {
	    /* "count" multiples of write data */
	    while (count-- >= 0) {
		if (doing_longwords) {
		    ldata = (init[0] << 24) + (init[2] << 16) +
			    (init[4] <<  8) +  init[6];
		    init += 8;
		    if (!lcopy_prot(&ldata, laddr, 1)) {
			return(0);
		    }
		    laddr++;
		}
		else {
		    data = (init[0]<<8) + init[2];
		    init += 4;
		    if (!scopy_prot(&data, addr, 1)) {
			return(0);
		    }
		    addr++;
		}
	    }
	}

	if (opcode & IDROM_CTL_LASTBLOCK)
	    break;
    }
    snooze(1000000);			    /* give hardware time to recover */
    return(1);
}


/*
 * This routine copies the Font ROM into a font array (8 pixels/byte).
 * NOTE: this routine used to be called ite_getfontrom() (of course, not
 *       to be confused with get_fontrom()...)
 */
ite_copyfont(ite, font_rom, font_to, num_chars)			    /* ENTRY */
    register struct iterminal *ite;
    register short *font_rom;
    register char *font_to;
{
    register int i, j, k;
    register width_loop = ite->font_bytpchar / ite->font_height;

    for (i = 0; i < num_chars; i++) {
	for (j = 0; j < ite->font_height; j++) {
	    for (k = 0; k < width_loop; k++) {
		*font_to++ = *font_rom++;
	    }
	}
    }
}

/*
 * Copy the ROMAN8 font, and any other fontss for that matter, in from the
 * Font_ROM into ite->font_start for faster access (that is, faster than ROM).
 */
ite_get_fontrom(ite)						    /* ENTRY */
    register struct iterminal *ite;				       /* a5 */
{
    register unsigned short short_d7;				       /* d7 */
    register unsigned char *diostart_a4 = (unsigned char *)DIO_START;  /* a4 */
    register unsigned char *char_pointer_a3;			       /* a3 */
    register struct font_rom_def *font_rom;

    /* deallocate font buffer if allocated */
    if (ite->font_start && (ite->font_start != ite->font_from_rom)) 
	sys_memfree(ite->font_start, ite->font_bytpchar * (ITE_NGLYPHS+64+1));
	
#   ifdef lint
	short_d7 = 0;	short_d7 = short_d7;	
#   endif

    /* relative 16 bit pointer to font space.  If no font return panic */
asm("		movp.w	0x3b(%a4),%d7		 ");

    if (short_d7 == 0)
	panic("no font rom");

    /* index to font id byte */
    char_pointer_a3 = diostart_a4 + short_d7 + 2;
			
    /* id must = 1 */
    if (*char_pointer_a3 != 1)
	panic("ite_get_fontrom(): id != 1");

asm("	movp.w	0x5(%a4),%d7 ");
    ite->framebuf_width = short_d7;
			
asm("	movp.w	0x9(%a4),%d7 ");
    ite->framebuf_height = short_d7;

asm("	movp.w	0xd(%a4),%d7 ");
    ite->framedsp_width = short_d7;

asm("	movp.w	0x11(%a4),%d7 ");
    ite->framedsp_height = short_d7;

asm("	movp.w	0x2(%a3),%d7 ");		  /* pointer to font with id */

    /* get start of font address */
    font_rom = (struct font_rom_def *)(diostart_a4 + (short_d7 - 1));

    /* id=1 has to be full set for now!!! */
    if ((font_rom->end_char != ITE_NGLYPHS-1) || (font_rom->start_char != 0))
	panic("ite_get_fontrom(): not full set");
 
    /* font height */
    ite->line_height = ite->font_height = font_rom->cell_height;

    /* special kludge for renaissance */
    if (ite->flags & (RENAISSANCE | DAVINCI))
	ite->line_height += 6;
 
    /* font width */
    ite->font_width = font_rom->cell_width;

    /* calculate number of chars in font buf per font character */
    ite->font_bytpchar = (((ite->font_width-1)>>3)+1) * ite->font_height;

    /*
     * If first pass, allocate font buffer and get font characters.
     * NOTE: this area needs work for the katakana set !!!!!!!!!
     */
    if (ite->font_from_rom == NULL) {
	/* allocate the font buffer */
	if ((ite->font_from_rom = (unsigned char *)
		calloc(ite->font_bytpchar * (ITE_NGLYPHS+64+1))) == NULL)
	    panic("ite_get_fontrom(): calloc(ite->font_from_rom) failed");

 	/* move and unpack ROMAN-8 rom font to font buffer */
	ite_copyfont(ite, &font_rom->font_buffer[0], ite->font_from_rom,
			ITE_NGLYPHS);

	/* get the Katakana font */
	if ((char_pointer_a3[-2] > 1) &&	/* more than one font */
	    (char_pointer_a3[6] == 2)) {	/* this is font #2 */

	    char_pointer_a3 += 6;		/* point to font #2 */
asm("	movp.w	0x2(%a3),%d7 ");		/* pointer to font with id */

	    /* get start of font address */
	    font_rom = (struct font_rom_def *)(diostart_a4 + (short_d7 - 1));
	    ite_copyfont(ite,
		    &font_rom->font_buffer[(160-font_rom->start_char)*ite->font_bytpchar],
		    /* &font_rom->font_buffer[64-font_rom->start_char], */
		    &ite->font_from_rom[ITE_NGLYPHS*ite->font_bytpchar], 64);
	}

	/* Supply font for Renaissance */
	else if (ite->font_width==10 && ite->font_height==15) {
		bcopy(renaissance_kata_font,
			&ite->font_from_rom[ITE_NGLYPHS*ite->font_bytpchar],
			ren_font_size);
	}
	/* Supply font for low-res Bobcat */
	else if (ite->font_width==12 && ite->font_height==15) {
	    bcopy((caddr_t)lo_res_kata_font,
		  (caddr_t)&ite->font_from_rom[ITE_NGLYPHS*ite->font_bytpchar],
		  LO_RES_KATA_FONT_SIZE);
	}
	else {
		/* fill it with characters from ROMAN-8 set */
		ite_copyfont(ite,
		    &font_rom->font_buffer[(160-font_rom->start_char)*ite->font_bytpchar],
		    &ite->font_from_rom[ITE_NGLYPHS*ite->font_bytpchar], 64);
	}
    }
    ite->font_start = ite->font_from_rom;
}

/*
 * Routine to initialize offscreen memory to the current character font
 */
ite_font_restore(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    SCROLL vlp;
    register int x, y, i, rem, right_margin, failure=0;
    int state = REGION0;
    color_pairs_t save_cur_color;	/* save color pair 0 temp */

    save_cur_color = ite->color_pairs[0];

    /* set color pair 0 to currently enabled planes */
    ite->color_pairs[0].FG = 0x07070707;
    ite->color_pairs[0].BG = 0x00000000;

    /* force rebuild of color pair table to color pair 0 */
    ite->current_cpair = ITE_BOGUS_CPAIR;
    ite_build_pixtbl(ite, CHAR_ON);

    /*
     * Make sure the the right margin is on a
     * character cell boundary.
     */
    right_margin = ite->framedsp_width;
    rem = right_margin % ite->font_width;	/* perfect multiple? */
    if (rem!=0)
	right_margin += ite->font_width - rem;
    x = right_margin;
    y = 0;

    for (i = 0; i < ITE_NGLYPHS; i++) {
checkagain:
	switch (state) {
	    /* offscreen memory is to right of display memory */
	    case REGION0:
		if ((y + ite->line_height) > ite->framedsp_height) {
			state = REGION1;
			y = ite->framedsp_height;
			rem = y % ite->line_height;	/* perfect multiple? */
			if (rem!=0)
				y += ite->line_height - rem;
			x = right_margin = 0;
			goto checkagain;
		}
		/* fall thru */

setit:		if ((x + ite->font_width) > ite->offscrn_max_width) {
			y += ite->line_height;
			x = right_margin;
			goto checkagain;
		}
		/* set the location of the char in offscreen memory */
		offscreen_font[i].X = x;
		offscreen_font[i].Y = y;

		vlp = i | CHAR_ON;

		/* write char in upper-left corner on screen, color 0 */
		(*ite->crt_write_ram)(ite, &vlp, y/ite->line_height, x/ite->font_width, 1);

		x += ite->font_width;
		break;

	    /* move to offscreen memory below display memory */
	    case REGION1:
		if ((y + ite->line_height) > ite->offscrn_max_height) {
			/* not enough offscreen mem to support it */
			state = REGION2;
			failure = 1;
			break;
		}
		goto setit;

	    /* not enough offscreen memory */
	    case REGION2:
		failure = 1;
		break;
	}
    }

    /* restore origional and for rebuild of color pair table */
    ite->color_pairs[0] = save_cur_color; 
    ite->current_cpair = ITE_BOGUS_CPAIR;

    return(failure);
}

/*  Parameters for fast font routines, with frame memory on word boundry */
/* The assumption is that an even width font char will always start */
/* on a word boundry                                                */

ite_8modby_draw(font_a5)					    /* ENTRY */
register struct font_def *font_a5;
{
    register unsigned char *font_buf_a4 	= font_a5->font_buf;
    register unsigned char *frame_mem_a3 	= font_a5->frame_mem;
    register int	*pixel_colorp		= font_a5->pixel_colors_ptr;
    register int	font_height_d7		= font_a5->heightm1;
    register int	frame_width_d6		= font_a5->next_row_add;
    register short	mask_d5			= 0x3c;
    register int	background_d4		= pixel_colorp[0];
    register int	font_bytpcrowm1_d3	= font_a5->widthm1 >> 3;

    (int *)font_a5 = pixel_colorp;	/* kludge for LCD compiler */

asm("		mov.l	%d3,%a0		 "); /* number of bytes per row - 1 */
asm("mod8by1:	mov.l	%a0,%d3		 "); /* number of bytes per row - 1 */
asm("mod8by2:	mov.b	(%a4)+,%d0	 "); /* d0 = next 8 bits from font buf */
asm("		bne.b	mod8by3		 "); /* check if any bits ? (shortcut) */

asm("		mov.l	%d4,(%a3)+	 "); /* move background value to frame */
asm("		mov.l	%d4,(%a3)+	 "); /* move background value to frame */
asm("		bra.b	mod8by4		 "); /* done */

asm("mod8by3:	mov.w	%d0,%d1		 "); /* save copy in d1 */
asm("		lsr.w	&2,%d0		 "); /* position for long index */
asm("		lsl.w	&2,%d1		 "); /* position for long index */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		and.w	%d5,%d1		 "); /* mask to proper bits */
asm("		mov.l	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("		mov.l	0(%a5,%d1.w),(%a3)+  ");/* move table value frame */
asm("mod8by4:	dbf     %d3,mod8by2	 "); /* another 8 columns to do ? */
asm("		adda.l	%d6,%a3		 "); /* bump to next row (8*16) */
asm("		dbf     %d7,mod8by1	 "); /* another row to do ? */
}

ite_any_draw(font)						    /* ENTRY */
    register struct font_def *font;
{
    register unsigned char *font_buf_a4 	= font->font_buf;
    register unsigned char *frame_mem_a3 	= font->frame_mem;
    register int	font_height_d7		= font->heightm1;
    register int	font_width_d6		= font->widthm1;
    register int	frame_width_d5		= font->next_row_add;
    register int	background_d4		= font->pixel_colors_ptr[0];
    register int	foreground_d3		= font->pixel_colors_ptr[15];
    register int	font_bytpcrow_d2	= 0; /*dummy to fool compiler*/

asm("anyby1:	mov.w	%d6,%d2		 "); /* font width -1 */
asm("		lsr.w	&3,%d2		 "); /* calculate bytes / pix row */

asm("anyby2:	movq	&7,%d1		 "); /* set up for 8 more pixels */
asm("		tst.w	%d2		 "); /* check if last pass */
asm("		bne.b	anyby3		 "); /* no */
asm("		and.w	%d6,%d1		 "); /* yes, set count 1 to 8 */

asm("anyby3:	mov.b	(%a4)+,%d0	 "); /* d0 = next 8 bits from fontbuf */
asm("		bne.b	anyby6		 "); /* check if any bits ? */

asm("anyby4:	mov.b	%d4,(%a3)+	 "); /* do a fast background write */
asm("		dbf     %d1,anyby4	 "); /* another pixel in this row ? */
asm("		bra.b	anyby9		 "); /* do another 8 pixels */

asm("anyby5:	mov.b	%d3,(%a3)+	 "); /* move foreground color */
asm("		bra.b	anyby8		 "); /* done, exit */

asm("anyby6:	lsl.w	&8,%d0		 "); /* yes, position for index */

asm("anyby7:	lsl.w	&1,%d0		 "); /* position for next index */
asm("		bcs.b	anyby5		 "); /* check bit shifted out */
asm("		mov.b	%d4,(%a3)+	 "); /* move background color */
asm("anyby8:	dbf     %d1,anyby7	 "); /* another pixel in this byte ? */
asm("anyby9:	dbf     %d2,anyby2	 "); /* get more pixels for this row */
asm("		adda.l	%d5,%a3		 "); /* frame buffer next pixel line */
asm("		dbf     %d7,anyby1	 "); /* another row to do ? */
}

ite_10by_draw(font_a5)						    /* ENTRY */
    register struct font_def *font_a5;
{
    register unsigned char *font_buf_a4 	= font_a5->font_buf;
    register unsigned char *frame_mem_a3 	= font_a5->frame_mem;
    register int	*pixel_colorp		= font_a5->pixel_colors_ptr;
    register int	font_height_d7		= font_a5->heightm1;
    register int	frame_width_d6		= font_a5->next_row_add;
    register int	index_mask_d5		= 0x3c;
    register int	background_d4		= pixel_colorp[0];

    (int *)font_a5 = pixel_colorp;	/* kludge for LCD compiler */

asm("tenby1:	mov.w	(%a4)+,%d1	 "); /* d0 =next 10 bits from font buf */
asm("		and.w	&0xffc0,%d1	 "); /* mask to proper bits */
asm("		bne.b	tenby2		 "); /* check if any bits ? (shortcut) */
asm("		mov.l	%d4,(%a3)+	 ");/*  backgnd color value to frame */
asm("		mov.l	%d4,(%a3)+	 ");/*  backgnd color value to frame */
asm("		mov.w	%d4,(%a3)+	 ");/*  backgnd color value to frame */
asm("		bra.b	tenby3		 "); /* check if any bits ? (shortcut) */

asm("tenby2:	rol.w	&6,%d1		 "); /* position for long index */
asm("		mov.b	%d1,%d0		 "); /* d1 = next 4 bits from font buf */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		mov.l	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("		rol.w	&4,%d1		 "); /* position for long index */
asm("		mov.b	%d1,%d0		 "); /* d1 = next 4 bits from font buf */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		mov.l	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("		rol.w	&4,%d1		 "); /* position for long index */
asm("		mov.b	%d1,%d0		 "); /* d1 = next 4 bits from font buf */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		mov.w	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("tenby3:	adda.l	%d6,%a3		 "); /* bump to next row (10*15) */
asm("		dbf     %d7,tenby1	 "); /* another row to do ? */
}

ite_12by_draw(font_a5)						    /* ENTRY */
    register struct font_def *font_a5;
{
    register unsigned char *font_buf_a4 	= font_a5->font_buf;
    register unsigned char *frame_mem_a3 	= font_a5->frame_mem;
    register int	*pixel_colorp		= font_a5->pixel_colors_ptr;
    register int	font_height_d7		= font_a5->heightm1;
    register int	frame_width_d6		= font_a5->next_row_add;
    register int	index_mask_d5		= 0x3c;
    register int	background_d4		= pixel_colorp[0];

    (int *)font_a5 = pixel_colorp;	/* kludge for LCD compiler */

asm("twlvby1:	mov.w	(%a4)+,%d1	 "); /* d0 =next 12 bits from font buf */
asm("		and.w	&0xfff0,%d1	 "); /* mask to proper bits */
asm("		bne.b	twlvby2		 "); /* check if any bits ? (shortcut) */
asm("		mov.l	%d4,(%a3)+	 ");/*  backgnd color value to frame */
asm("		mov.l	%d4,(%a3)+	 ");/*  backgnd color value to frame */
asm("		mov.l	%d4,(%a3)+	 ");/*  backgnd color value to frame */
asm("		bra.b	twlvby3		 "); /* check if any bits ? (shortcut) */

asm("twlvby2:	rol.w	&6,%d1		 "); /* position for long index */
asm("		mov.b	%d1,%d0		 "); /* d1 = next 4 bits from font buf */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		mov.l	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("		rol.w	&4,%d1		 "); /* position for long index */
asm("		mov.b	%d1,%d0		 "); /* d1 = next 4 bits from font buf */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		mov.l	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("		rol.w	&4,%d1		 "); /* position for long index */
asm("		mov.b	%d1,%d0		 "); /* d1 = next 4 bits from font buf */
asm("		and.w	%d5,%d0		 "); /* mask to proper bits */
asm("		mov.l	0(%a5,%d0.w),(%a3)+  ");/* move table value frame */
asm("twlvby3:	adda.l	%d6,%a3		 "); /* bump to next row (12*15) */
asm("		dbf     %d7,twlvby1	 "); /* another row to do ? */
}
