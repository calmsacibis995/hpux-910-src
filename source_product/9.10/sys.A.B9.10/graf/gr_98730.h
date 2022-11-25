/*
 * @(#)gr_98730.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 16:54:21 $
 * $Locker:  $
 */

#ifndef GR_98730_H_INCLUDED
#define GR_98730_H_INCLUDED

/* The 16-bit contents of daVinci's transform engine IDREG */
#define HP98730_XFORM_ID	0xFFF8

/* HP98730 (DaVinci) status bit registers */
struct dav_ctlspace {
	int dav_idrom[0x1000];
	int nothing00[0x10];	/* 4000 */
	int dav_vblank;		/* 4043 Vertical blanking is in progress */
	int dav_wbusy;		/* 4047 Window move in progress */
	int dav_intv;		/* 404B VBLANK caused interrupt */
	int dav_intb;		/* 404F WMOVE done interrupt */
	int nothing01;		/* 4050 - 4053 */
	int dav_c_togl;		/* 4057 For diagnostic test */
	int dav_as_busy;	/* 405B Scan converter is busy accessing FB */
	int nothing02[13];	/* 405C - 408F */
	int dav_fbwen;		/* 4090 Enable write frame buffer */
	int dav_intenv;		/* 4097 Enable interrupt on VBLANK */
	int dav_intenb;		/* 409B Enable WMOVE done interrupt */
	int dav_wmove;		/* 409F Initiate window move */
	int nothing03;		/* 40A3 Used to be BLINK */
	int nothing04[3];	/* 40A4 - 40AF */
	int dav_fold;		/* 40B3 Select fold mode byte/int per pix */
	int dav_opwen;		/* 40B7 Overlay plane write enable */
	int dav_diag_crl;	/* 40BB Diagnostic control (ex-TMODE) reg. */
	int dav_drive;		/* 40BF FB overlay plane board select */
	int dav_clrdiagstat;	/* 40C3 Clear diagnostic status */
	int nothing06[2];	/* 40C4 */
	int dav_arr;		/* 40CF Alternate replacement rule */
	int dav_zrr;		/* 40D3 Z replacement rule */
	int dav_en_scan;	/* 40D7 enable scan board (enable DTACK) */
	int nothing07[5];	/* 40D8 */
	int dav_rep_rule;	/* 40EF replacement rule for all accesses */
	int dav_source_x;	/* 40F2 source window origin X address */
	int dav_source_y;	/* 40F6 source window origin Y address */
	int dav_dest_x;		/* 40FA Dest window origin X address */
	int dav_dest_y;		/* 40FE Dest window origin Y address */
	int dav_wwidth;		/* 4102 Window width (in pixels) */
	int dav_wheight;	/* 4106 Window height (in pixels) */
	int nothing08[0x1be];	/* 4108 - 47FC */
	int dav_no_dtack;	/* 4800 Always times out */
	int nothing09[0x1ff];	/* 4804 - 4FFC */
	int dav_bytectl;	/* 5000 Spectrograph FIXME register */
	int dav_hildata;	/* 5004 HIL data register */
	int nothing10;		/* 5008 */
	int dav_hilcmd;		/* 500C HIL command register */
	int nothing11[0x3fc];	/* 5010 - 6000 */
	int dav_cmapbank;	/* 6003 Bank select 0/1 */
				  /* The following are only valid */
				  /* when dav_cmapbank = 0 */
	int dav_dispen;		  /* 6007 Display enable register */
	int dav_fbven0p;	  /* 600B FB primary   map video enable */
	int dav_fbven1p;	  /* 600F FB primary   map video enable1 */
	int dav_fbven2p;	  /* 6013 FB primary   map video enable2 */
	int dav_fbven0s;	  /* 6017 FB secondary map video enable */
	int dav_fbven1s;	  /* 601B FB secondary map video enable1 */
	int dav_fbven2s;	  /* 601F FB secondary map video enable2 */
	int dav_vdrive;		  /* 6023 FB video display mode */
	int nothing12[0x17];	  /* 6024 - 607F */
	int dav_panxh;		  /* 6083 Pan X high byte */
	int dav_panxl;		  /* 6087 Pan X low byte */
	int dav_panyh;		  /* 608B Pan Y high byte */
	int dav_panyl;		  /* 608F Pan Y low byte */
	int dav_zoom;		  /* 6093 Zoom factor */
	int dav_pz_trig;	  /* 6097 Pan and Zoom value trigger */
	int dav_ovly0p;		  /* 609B Overlay 0 primary map transparency */
	int dav_ovly1p;		  /* 609F Overlay 1 primary map transparency */
	int dav_ovly0s;		  /* 60A3 Overlay 0 secondary map transprncy */
	int dav_ovly1s;		  /* 60A7 Overlay 1 secondary map transprncy */
	int dav_opvenp;		  /* 60AB Overlay 0 plane enable register */
	int dav_opvens;		  /* 60AF Overlay 1 plane enable register */
	int dav_fv_trig;	  /* 60B3 Trigger control registers */
	int dav_cdwidth;	  /* 60B7 CDWIDTH timing param for IRIS */
	int dav_chstart;	  /* 60BB CHSTART timing param for IRIS */
	int dav_cvwidth;	  /* 60BF CVWIDTH timing param for IRIS */
	int nothing13[0x10];	  /* 60C0 - 60FF */
	int dav_ovlcmap[0xc0];	  /* 6100 Overlay color map */
	int dav_fbcmap02[0x300];  /* 6400 - 6FFF Framebuffer color maps 0/2 */
	int nothing14[0x100];	  /* 7000 - 73FF */
	int dav_fbcmap1[0x300];	  /* 7400 - 7FFF Framebuffer color map 1 */
	int dav_xform_idreg;	/* 8002 Transform Engine ID register */
	int nothing15;		/* 8004 - 8007 */
	int dav_bitreg1;	/* 800A master bit register 1 */
	int nothing155;		/* 800C - 800F */
	int dav_status1;	/* 8012 Status1 register */
	int nothing16[0x3fb];	/* 8014 - 8FFF */
	int nothing161[0x2];	/* 9000 - 9007 */
	int dav_status2;	/* 900A Status2 register */
	int nothing162;		/* 900C - 9010 */
	int nothing163[0x2];	/* 9010 - 9017 */
	int dav_status3;	/* 901A Status3 register */
	int nothing164[0xbf9];	/* 901C - BFFF */
	int nothing17[0x8f];	/* C000 - C23B */
	int dav_pstop;		/* C23E Pace valve */
	int nothing18[0x3ef];	/* C240 - D1FB */
	int dav_lgb_holdoff0;	/* D1FC LGB Holdoff 0 */
	int nothing19[0x200];	/* D200 - D9FF */
	int dav_dcinf1;		/* DA02 Draw command input fifo */
	int nothing20[0x1fe];	/* DA04 - E1FB */
	int dav_lgb_holdoff1;	/* E1FC LGB Holdoff 0 */
	int nothing21[0x200];	/* E200 - E9FF */
	int dav_dcinf2;		/* EA02 Draw command input fifo */
	int nothing22[0x1fe];	/* EA04 - F1FB */
	int dav_lgb_holdoff2;	/* F1FC LGB Holdoff 0 */
	int nothing23[0x200];	/* F200 - F9FF */
	int dav_dcinf3;		/* FA02 Draw command input fifo */
};

#ifdef _KERNEL
    /* Data exported from gr_98730.c */
    extern crt_frame_buffer_t template_98730;

    extern int is_hp98730();
#endif
#endif
