/*
 * @(#)gr_98705.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:53:57 $
 * $Locker:  $
 */

#ifndef GR_98705_H_INCLUDED
#define GR_98705_H_INCLUDED

#define TIG_ZHERE_REG           0x7053     /* Location of optional Z-buffer. */

struct tiger_rgb {			/* An overlay colormap entry */
    int r;
    int g;
    int b;
};

/* HP98705 (Tigershark) status bit registers. Do we need these???? */
struct tiger_ctlspace {
    int reset[1];			/* 0001 */
    int nothing1[0x13ff];		/* 4FFC */
    int fv_trig;			/* 5003 */
    int command0;			/* 5007 */
    int command1;			/* 500B */
    int dispen;				/* 500F */
    int vdrive;				/* 5013 */
    int fbvenp;				/* 5017 */
    int fbvens;				/* 501B */
    int fb_blink;			/* 501F */
    int opvenp;				/* 5023 */
    int opvens;				/* 5027 */
    int ov_blink;			/* 502B */
    int ovly0p;				/* 502F */
    int ovly1p;				/* 5033 */
    int ovly0s;				/* 5037 */
    int ovly1s;				/* 503B */
    int nothing4[0x71];			/* 51FC */
    struct tiger_rgb oprimary[16]; 	/* 5200 */
    int nothing5[0x40-(16*3)];
    struct tiger_rgb osecondary[16]; 	/* 5300 */
    int nothing6[0x40-(16*3)];
    int image_cmap_red_p[0x100];	/* 5400-57FF */
    int image_cmap_grn_p[0x100];	/* 5800-5BFF */
    int image_cmap_blu_p[0x100];	/* 5C00-5FFF */
    int nothing6b[0x100];
    int image_cmap_red_s[0x100];	/* 6400-67FF */
    int image_cmap_grn_s[0x100];	/* 6800-6BFF */
    int image_cmap_blu_s[0x100];	/* 6C00-6FFF */
    int nothing6c[0x11];
    int wbusy;				/* 7047 */
    int nothing7[0x2];			/* */
    int zhere;				/* 7053 */
    int nothing8[0x1];			/* */
    int as_busy;			/* 705b */
    int nothing9[0xD];			/* */
    int fbwen;				/* 7093 */
    int nothing10[0x2];			/* 709b */
    int wmove;				/* 709f */
    int nothing11[0x4];			/* 70af */
    int fold;				/* 70b3 */
    int opwen;				/* 70b7 */
    int nothing12[0x1];			/* 70ba */
    int drive;				/* 70bf */
    int nothing13[0xB];			/* 70eb */
    int rep_rule;			/* 70ef */
    int source_x;			/* 70f2 */
    int source_y;			/* 70f6 */
    int dest_x;				/* 70fa */
    int dest_y;				/* 70fe */
    int wwidth;				/* 7102 */
    int wheight;			/* 7106 */
    int nothing14[0x204c];		/* */
    int pace_plug;			/* f23e */
    int nothing15[0x5C377];		/* */
    int te_status;			/* 180018 */
    int nothing16[0x9fff8];		/* */
    int lgb_holdoff;			/* 3ffffc */
};

#ifdef _KERNEL
    /* Data exported from gr_98705.c */
    extern crt_frame_buffer_t template_98705;
#endif

#endif /* not GR_98705_H_INCLUDED */
