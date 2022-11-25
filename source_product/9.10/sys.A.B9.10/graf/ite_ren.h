/*
 * @(#)ite_ren.h: $Revision: 1.3.84.3 $ $Date: 93/09/17 20:59:55 $
 * $Locker:  $
 */

#ifndef __ITE_REN_H_INCLUDED
#define __ITE_REN_H_INCLUDED

struct ren_cmap {
    char dummy[3];
    char data;
};

struct renaissance {
    char w0[0x0001-0x0000  ]; char rreset;    /* reset register */
    char w1[0x0003-0x0001-1]; char intrpt;    /* frame buffer enable */
    char x0[0x4046-0x0003-1]; short wbusy;    /* window move is in progress */
    char x1[0x405a-0x4046-2]; short scanbusy; /* Scan Board has frame buffer */
    char x2[0x4082-0x405a-2]; short fbven;    /* frame buffer video enable */
    char x3[0x4086-0x4082-2]; short dispen;   /* enable video to the display */
    char x4[0x4090-0x4086-2]; int fbwen;      /* enable write frame buffer */
    char x5[0x409e-0x4090-4]; short wmove;    /* initiate window move */
    char x6[0x40b2-0x409e-2]; short fold;     /* fold to byte/int per pix */
                              int opwen;      /* overlay plane write enable */
    char x8[0x40be-0x40b2-6]; short drive;    /* FB overlay board select */
    char x9[0x40ee-0x40be-2]; short rep_rule; /* replacement rule */
    char y0[0x40f2-0x40ee-2]; short source_x; /* source window X addr */
    char y1[0x40f6-0x40f2-2]; short source_y; /* source window Y addr */
    char y2[0x40fa-0x40f6-2]; short dest_x;   /* Dest window origin X addr */
    char y3[0x40fe-0x40fa-2]; short dest_y;   /* Dest window origin Y addr */
    char y4[0x4102-0x40fe-2]; short wwidth;   /* Window width (in pixels) */
    char y5[0x4106-0x4102-2]; short wheight;  /* Window height (in pixels) */
    char y6[0x6006-0x4106-2]; short color_stat; /* color map status */
    char y7[0x63c0-0x6006-2]; struct ren_cmap ovlmp0[16];    /* overlay cmap */
			      struct ren_cmap rdata0[256];
			      struct ren_cmap gdata0[256];
			      struct ren_cmap bdata0[256];
    char y8[0x73c0-0x7000  ]; struct ren_cmap ovlmp1[16];    /* overlay cmap */
			      struct ren_cmap rdata1[256];
			      struct ren_cmap gdata1[256];
			      struct ren_cmap bdata1[256];
    char z5[0x8002-0x8000  ]; short te_halt;
    char z6[0x8006-0x8002-2]; short bank;	    /* transform bank select */
    char z7[0x8012-0x8006-2]; short te_int0;
    char z8[0xc000-0x8012-2]; short cstore;	     /* xform program memory */
};

#ifdef _KERNEL
    extern unsigned short renaissance_kata_font[];

    extern renais_write(), renais_write_off(), renais_cursor(),
	   renais_clear(), renais_scroll(), renais_init(), renais_pwr_reset(),
	   renais_set_st(), renais_check_screen_access(),
	   renais_iterminal_init();
#endif

#endif /* __ITE_REN_H_INCLUDED */
