/* @(#) $Revision: 70.3 $ */      

#include "Ktty.h"
#include "basic.h"
#include "sti.h"
#include "catseye.h"
#define NULL 0

int cursor_x=0, cursor_y=0;

/* Catseye declarations */
struct cat *cat;
struct cat_font *cat_font;
unsigned char *frame_buffer;
int plane_mask=07;

/* STI declarations */
struct sti_rom *sti;
struct sti_font *f;

glob_cfg glob_config;
init_flags i_flags;
init_inptr i_in;
init_outptr i_out;

font_flags f_flags;
font_inptr f_in;
font_outptr f_out;

int (*init_graph)();                    /* STI routine from rom */
int (*font_unpmv)();                    /* STI routine from rom */
int (*block_move)();                    /* STI routine from rom */

get_int(addr)
register int *addr;
{
	register int a,b,c,d;
	a = *addr++ & 0xff;
	b = *addr++ & 0xff;
	c = *addr++ & 0xff;
	d = *addr++ & 0xff;

	return (a<<24) + (b<<16) + (c<<8) + d;
}

get_word(addr)
register short *addr;
{
	register int a,b;
	a = *addr++ & 0xff;
	b = *addr++ & 0xff;

	return (a<<8) + b;
}


typedef int (*function)();

function
copy_function(addr)
int *addr;
{
	register int i, size, *start_addr;
	register char *code;
	extern char *malloc();

	start_addr = (int *) ((get_int(addr) & ~03) + (char *) sti);
	size = (get_int(addr+4) - get_int(addr))/4 ;
	code = malloc(size);
	for (i=0; i<size; i++)
		code[i] = *start_addr++;
	kdb_purge();			/* purge our brains out */
	return (function) code;		/* Pointer to the new code */
}

int screen_width, screen_height;

sti_init()
{
	int *rlist, i, addr;
	region_desc r;

	/* Search the four SGC slots */
	for (addr=0x20000000; addr<=0x2c000000; addr+=0x02000000) {
		sti = (struct sti_rom *) addr;
		if (kdb_testr(&sti->device_type) && sti->device_type==1)
			goto good;
	}
	sti=NULL;
	return;

good:
	f = (struct sti_font *) (((int) sti) + get_int(sti->font_start) & ~03);

	init_graph = copy_function(sti->init_graph);
	font_unpmv = copy_function(sti->font_unpmv);
	block_move = copy_function(sti->block_move);

	i=0;
	rlist = (int *) ((int) sti + get_int(sti->dev_region_list) & ~03);
	do {
		* (int *) &r = get_int(rlist); rlist += 4;
		glob_config.region_ptrs[i++] = (int) sti + r.offset*4096;
	} while (!r.last);
        /* Initialize the card */

        i_flags.wait = true;
        i_flags.reset = true;
        i_flags.text = true;
        i_flags.clear = true;
        i_flags.cmap_blk = true;
        i_flags.enable_be_timer = true;
        i_flags.init_cmap_tx = true;
        i_in.text_planes = 3;

	(*init_graph)(&i_flags, &i_in, &i_out, &glob_config);

	screen_width = glob_config.onscreen_x / f->width;
	screen_height = glob_config.onscreen_y / f->height - 2;	/* softkeys */
	cursor_y = screen_height-1;		/* last usable line */
}

catseye_at(p)
struct cat *p;
{
	if (kdb_testr(&p->id) && (p->id & 0x7f)==0x39) {
		switch (p->secondary_id) {
		case 2: /* generic topcat */
		case 5: /* low-cost color (LCC) Catseye */
		case 6: /* high-res color (HRC) Catseye */
		case 7: /* 1280x1024 mono Catseye */
		case 9: /* 640x480 4-plane color Catseye 98541 (319x) */
			return 1;
		}
	}
	return 0;
}

catseye_init()
{
	register int fontbase, i;

	cat = (struct cat *) 0x560000;

	if (catseye_at(cat))
		goto good;

	for (i=132; i<=255; i++) {
		cat = (struct cat *) (0x1000000 + (i - 132) * 0x400000);
		if (catseye_at(cat))
			goto good;
	}
	cat=NULL;				/* failure */
	return;

good:

	fontbase = (int) cat + get_word(cat->font_offset);
	cat_font = (struct cat_font *) ((int) cat + get_word(fontbase+3));

	screen_height = get_word(cat->framedsp_height) / cat_font->height - 2;
	screen_width = get_word(cat->framedsp_width) / cat_font->width;
	cursor_y = screen_height-1;		/* last usable line */

	idrom_init((unsigned char *) cat);

	/* Flash/clear the screen */
	cat_bmove_wait();
	for (i=0; i<=10; i++) {
		cat->wrr = (i&1) ? 15 : 0; /* replacement rule = set or clear */

		cat->sox = 0;
		cat->dox = 0;

		cat->soy = 0;
		cat->doy = 0;

		cat->wwidth = screen_width*cat_font->width;
		cat->wheight = screen_height*cat_font->height;

		cat->wmove = 0xff<<8;
		cat_bmove_wait();
	}
}

struct cat *save_cat;


graphics_char_out(x,y,c)
{
	if (x<0 || y<0)
		return;

	if (cat) {
		register unsigned char *screen_addr;
		short *cell;
		register int bit, fb_width;

		cat_bmove_wait();
		fb_width = get_word(cat->framebuf_width);
		frame_buffer = (unsigned char *)
			((int) cat + get_word(cat->fb_loc));
		if ((int) cat==0x560000)
			frame_buffer = (unsigned char *) ((*frame_buffer) << 16);
		else
			frame_buffer = (unsigned char *) ((int) cat + ((*frame_buffer) << 16));
		screen_addr = frame_buffer
			      + y*cat_font->height*fb_width
			      + x*cat_font->width;
		cat->wrr = 3;		/* replacement rule = move */
		cat->tcwen = 07<<8;	/* enable all planes for block move */
		cat->fben = 07<<8;	/* enable planes for frame buffer */
		cat->prr = 3<<8;	/* set pixel replacement rule */ 

		cell = cat_font->chars
			+ c*(cat_font->height * ((cat_font->width+7)>>3));
		*(int *) &cell = (int) cell - 1;

		for (y=0; y<cat_font->height; y++) {
			for (x=0; x<cat_font->width; x++) {
				/* fetch the bit from the font */
				bit = cell[x>>3] >> (7-(x & 07));

				bit &= 01;	/* only use lower-order bit */
				*screen_addr++ = bit;
			}
			
			screen_addr += fb_width - cat_font->width;
			cell += (cat_font->width+7) >> 3;
		}
	}

	else if (sti) {
		f_flags.wait = true;
		f_in.font_start_addr = (int) f;
		f_in.dest_x = x * f->width;
		f_in.dest_y = y * f->height;

		f_in.index = c;
		f_in.fg_color = 1;
		f_in.bg_color = 0;

		(*font_unpmv)(&f_flags, &f_in, &f_out, &glob_config);
	}
}

blkmv_flags  b_flags;
blkmv_inptr  b_in;
blkmv_outptr b_out;

cat_bmove_wait()
{
	while (cat->wmove & (01<<8));
}

scroll_up()
{
	if (cat) {
		cat_bmove_wait();
		cat->sox = 0;
		cat->soy = cat_font->height;
		cat->dox = 0;
		cat->doy = 0;

		cat->wwidth = screen_width*cat_font->width;
		cat->wheight = (screen_height-1) * cat_font->height;
		cat->tcwen = 07<<8;		/* enable all planes */
		cat->wrr = 3;			/* replacement rule = move */
		cat->wmove = 07<<8;		/* move just our planes */
		cat_bmove_wait();

		/* Now clear the last line */
		cat->wrr = 0;			/* replacement rule = clear */

		cat->sox = 0;
		cat->dox = 0;

		cat->soy = (screen_height-1)*cat_font->height;
		cat->doy = (screen_height-1)*cat_font->height;

		cat->wwidth = screen_width*cat_font->width;
		cat->wheight = cat_font->height;

		/* clear just available planes */
		cat->wmove = 3<<8;
		cat_bmove_wait();
	}

	else if (sti) {
		b_flags.clear = false;
		b_in.src_x = 0;
		b_in.src_y = f->height;
		b_in.dest_x = 0;
		b_in.dest_y = 0;
		b_in.width = glob_config.onscreen_x;
		b_in.height = (screen_height-1)*f->height;

		b_flags.wait = true;

		(*block_move)(&b_flags, &b_in, &b_out, &glob_config);

		/* Now clear the last line */
		b_flags.clear = true;
		b_in.dest_y = (screen_height-1)*f->height;
		b_in.height = f->height;

		(*block_move)(&b_flags, &b_in, &b_out, &glob_config);
	}
}

graphics_out(c)
{
	static int initialized=0;
	static int escape=0;
	extern int kdb_processor;

	/* 
	 * We don't support graphics on the 68020 boxes because it doesn't
	 * have the transparent translation hardware in the MMU and there
	 * just isn't enough ROI to emulate it at this point in this piece
	 * of software's lifecycle (i.e. 320s and 350 are almost obsolete).
	 */
	if (kdb_processor == 1)
		return;

	if (!initialized) {
		initialized=1;
		catseye_init();
		if (cat==NULL)
			sti_init();
	}

	tt_window_on();
	if (escape)
		escape=0;	/* Eat the 2-character escape sequence */
	else if (c=='\033')
		escape=1;
	else if (c=='\r')
		cursor_x = 0;
	else if (c=='\n') {
		cursor_y++;
		if (cursor_y>=screen_height) {
			scroll_up();
			cursor_y--;
		}
	}
	else if (c=='\b' && cursor_x>0)
		cursor_x--;
	else if (c<' ')
		/* do nothing */;
	else
		graphics_char_out(cursor_x++, cursor_y, c);
	tt_window_off();
}


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

    
    if ((offset = WORD_AT(card+0x23)) == 0)
	return;		   /* card not responding or no init region? */

    init = card + offset;
    while (1) {
	opcode = *init;
	init += 2;			/* skip over "opcode" in ROM */

	/* choke if there is a bit set that we do not recognize */
	if (opcode & ~(IDROM_CTL_LASTBLOCK | IDROM_CTL_REPL2WORDS |
		       IDROM_CTL_LONGWRITE | IDROM_CTL_BITTEST))
		Panic("idrom_init(): undefined control bit");

	/* choke on control restriction mentioned in IDROM spec */
	if ((opcode & (IDROM_CTL_REPL2WORDS|IDROM_CTL_BITTEST)) ==
		      (IDROM_CTL_REPL2WORDS|IDROM_CTL_BITTEST))
		Panic("idrom_init(): undefined control combination");

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
		Panic("idrom_init(): cannot do longword bittests yet");
	    else {
		for (milli_wait = IDROM_BITTEST_WAIT_MS; --milli_wait > 0; ) {

		    test_val = *addr;
		    /* bcopy(addr, &test_val, 1); */
		    if (((test_val >> init[2]) & 0x01) == init[0])
			break;

		    WAIT_ONE_MILLISEC;
		}
		if (milli_wait == 0)
		    return;
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
			*laddr = ldata;
			/* bcopy(&ldata, laddr, 1); */
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
			*addr = data;
			/* bcopy(&data, addr, 1); */
			addr++;
			*addr = data2;
			/* bcopy(&data2, addr, 1); */
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
		    *laddr = ldata;
		    /* bcopy(&ldata, laddr, 1); */
		    laddr++;
		}
		else {
		    data = (init[0]<<8) + init[2];
		    init += 4;
		    *addr = data;
		    /* bcopy(&data, addr, 1); */
		    addr++;
		}
	    }
	}

	if (opcode & IDROM_CTL_LASTBLOCK)
	    break;
    }
    snooze(1000000);			    /* give hardware time to recover */
}

snooze(usec)
{
	/* usec <<= 3; */
	usec >>= 1;
	while (--usec>0)
		;
}
