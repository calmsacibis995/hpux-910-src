/* @(#) $Revision: 70.2 $ */      

/*
 * This structure describes the hardward registers on the Catseye
 * family of graphics cards (9854x, 98550).
 */

struct cat {
    char x0[1]; char id;		/* ID register */
    char z1[2]; short framebuf_width[2];
    /* 0009 */                short framebuf_height[2];
    /* 000d */                short framedsp_width[2];
    /* 0011 */                short framedsp_height[2];
    char z5[1];               char secondary_id;
    char z2[0x003a-0x0015-1]; short font_offset[2];
    char z3[0x005c-0x003a-4]; short fb_loc[2];
    char x1[0x4080-0x005c-4]; short nblank;	/* display enable planes */
    char x2[0x4088-0x4080-2]; short tcwen;	/* write enable plane */
    char x3[0x408c-0x4088-2]; short tcren;	/* read enable plane */
    char x4[0x4090-0x408c-2]; short fben;	/* frame buffer write enable */
    char x5[0x409c-0x4090-2]; short wmove;	/* start window move */
    char x6[0x40a0-0x409c-2]; short blink;	/* enable blink planes */
    char x7[0x40a8-0x40a0-2]; short altframe;	/* enable alternate frame */
    char x8[0x40ac-0x40a8-2]; short curon;	/* cursor control register */
    char x9[0x40ea-0x40ac-2]; short prr;	/* pixel replacement rule */
    char y0[0x40ee-0x40ea-2]; short wrr;	/* replacement rule */
    char y1[0x40f2-0x40ee-2]; short sox;	/* source x pixel # */
    char y2[0x40f6-0x40f2-2]; short soy;	/* source y pixel # */
    char y3[0x40fa-0x40f6-2]; short dox;	/* dest x pixel # */
    char y4[0x40fe-0x40fa-2]; short doy;	/* dest y pixel # */
    char y5[0x4102-0x40fe-2]; short wwidth;	/* block mover pixel width */
    char y6[0x4106-0x4102-2]; short wheight;	/* block mover pixel height */
    char y7[0x4800-0x4106-2]; int :15, cat_wbusy:1;     /* Catseye status */
    char y8[0x6002-0x4800-2]; int :13, cmap_busy:1, :2; /* NERIED busy */
    char y9[0x60b2-0x6002-2]; short rdata;	/* color map red data */
			      short gdata;	/* color map green data */
			      short bdata;	/* color map blue data */
			      short ioab;	/* color map index */
			      short maskb;	/* color map mask for video */
    char z0[0x60f0-0x60ba-2]; short writeb;	/* color map trigger */
};


struct cat_font {
	unsigned char height;
	unsigned char x1;
	unsigned char width;
	unsigned char x2;
	unsigned char depth;
	unsigned char x3;
	unsigned char start_char;
	unsigned char x4;
	unsigned char end_char;
	short chars[1];
};
