/*
 * @(#)ite_top.h: $Revision: 1.3.84.3 $ $Date: 93/09/17 21:00:39 $
 * $Locker:  $
 */

#ifndef __ITE_TOP_H_INCLUDED
#define __ITE_TOP_H_INCLUDED

#define LO_RES_KATA_FONT_SIZE		(96*10)			 /* gross... */

/* Catseye-only registers */
#define CAT_STATUS	(0x4800/2)	/* Catseye status */

/* Fastcat registers */
#define FC_STATUS	(0x10000/4)	/* Fastcat status register (16bit) */
#define FC_HALT_REQ	(0x80000/4)	/* Fastcat halt request (32bit) */
#define FC_HALT_ACK	(0x80004/4)	/* Fastcat halt acknowledge (32bit) */

/*
 * Fastcat Halt Acknowledge register (halt_ack) bit definition(s).
 */
#define FASTCAT_HALTACK_TRUE		0x80000000

/*
 * FastCat Status Register bits.  Read-only unless otherwise noted.
 */
#define FASTCAT_STAT_MATRIX_ROTATES	0x4000
#define FASTCAT_STAT_FATE_OUTBUF_EMPTY	0x2000
#define FASTCAT_STAT_FATE_INBUF_EMPTY	0x1000
#define FASTCAT_STAT_DRAW_NOT_MOVE	0x0800
#define FASTCAT_STAT_CIRCLE_NOT_LINE	0x0400
#define FASTCAT_STAT_FATE_FLAG2		0x0200
#define FASTCAT_STAT_FATE_FLAG3		0x0100
#define	FASTCAT_STAT_RESET		0x0080	/* r/w */
#define	FASTCAT_STAT_HALT		0x0040	/* r/w */
#define FASTCAT_STAT_INTR6		0x0020	/* r/w */
#define FASTCAT_STAT_INTR4		0x0010	/* r/w */
#define FASTCAT_STAT_FATE_BUSY		0x0008
#define FASTCAT_STAT_FATERUG_NOT_RDY	0x0004
#define FASTCAT_STAT_RUG_BUSY		0x0002
#define FASTCAT_STAT_RUG_NOT_RDY	0x0001

/*
 * Fastcat Halt Request register (halt_req) commands with no/partial/full
 * state restore, respectively.  The last command (INTERRUPT) is
 * used to get the attention of Fastcat.
 */
#define FASTCAT_HALTREQ_CONT		0x00000000
#define FASTCAT_HALTREQ_CONT_PRESTORE	0x00008000
#define FASTCAT_HALTREQ_CONT_FRESTORE	0x00800000
#define FASTCAT_HALTREQ_PAUSE		0x80000000

struct cat {
    char x1[0x4080         ]; short nblank;	/* display enable planes */
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
    char z1[0x10000-0x60f0-2];  int fc_status;	/* Fastcat status */
    char z2[0x80000-0x10000-4]; int fc_halt_req;/* Fastcat halt request */
                                int fc_halt_ack;/* Fastcat halt acknowledge */
};

#ifdef _KERNEL
    extern unsigned short lo_res_kata_font[];

    extern bobcat_write(), bobcat_write_off(), bobcat_cursor(), bobcat_clear(),
	   bobcat_scroll(), bobcat_init(), bobcat_pwr_reset(), bobcat_set_st(),
	   bobcat_iterminal_init(), catseye_iterminal_init(),
	   catseye_check_screen_access();
#endif

#endif /* __ITE_TOP_H_INCLUDED */
