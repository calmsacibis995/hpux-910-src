/*
 * @(#)gr_98720.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 16:54:10 $
 * $Locker:  $
 */

#ifndef GR_98720_H_INCLUDED
#define GR_98720_H_INCLUDED

/* HP98720 status bit registers */
struct rn_ctlspace {
	int rn_idrom[0x1000];
	int nothing01[0x10];	/* 4000 */
	int rn_vblank;		/* 4043 vertical blanking is in progress */
	int rn_wbusy;		/* 4047 Window move in progress */
	int rn_intv;		/* 404B VBLANK caused interrupt */
	int rn_intb;		/* 404F WMOVE done interrupt */
	int nothing02[2];	/* 4050 */
	int rn_scanbusy;	/* 405B Scan Board still got frame buffer */
	int nothing03[9];	/* 405C */
	int rn_fbven;		/* 4083 frame buffer video enable */
	int rn_dispen;		/* 4087 enable video to the display */
	int nothing04[2];	/* 4088 */
	int rn_fbwen;		/* 4092 enable write frame buffer */
	int rn_intenv;		/* 4097 Enable interrupt on VBLANK */
	int rn_intenb;		/* 409B Enable WMOVE done interrupt */
	int rn_wmove;		/* 409F initiate window move */
	int rn_blink;		/* 40A3 causes planes to blink */
	int nothing05[3];	/* 40B0 */
	int rn_fold;		/* 40B3 select fold mode byte/int per pix */
	int rn_opwen;		/* 40B7 overlay plane write enable */
	int rn_tmode;		/* 40BB selects scan board tilling mode */
	int rn_drive;		/* 40BF FB overlay plane board select */
	int rn_vdrive;		/* 40C3 video drive select */
	int nothing06[3];	/* 40C4 */
	int rn_dmode;		/* 40D3 dither mode */
	int rn_en_scan;		/* 40D7 enable scan board (enable DTACK) */
	int nothing07[5];	/* 40D8 */
	int rn_rep_rule;	/* 40EF replacement rule for all accesses */
	int rn_source_x;	/* 40F2 source window origin X address */
	int rn_source_y;	/* 40F6 source window origin Y address */
	int rn_dest_x;		/* 40FA Dest window origin X address */
	int rn_dest_y;		/* 40FE Dest window origin Y address */
	int rn_wwidth;		/* 4102 Window width (in pixels) */
	int rn_wheight;		/* 4106 Window height (in pixels) */
	int nothing08[0x1be];	/* 4108 - 47FC */
	int rn_no_dtack;	/* 4800 Always times out */
	int nothing09[0x1ff];	/* 4804 - 4FFC */
	int rn_bytectl;		/* 5000 */
	int rn_hildata;		/* 5004 */
	int nothing10;		/* 5008 */
	int rn_hilcmd;		/* 500C */
	int nothing11[0x3fd];	/* 5010 - 6000 */
	int rn_color_stat;	/* 6007 color map status register */
	int nothing12[0xee];	/* 6008 - 63BC */
	int rn_ovlmp0[0x10];	/* 63C3 overlay map register */
	int rn_rdata0[0x100];	/* 6403 color map red data register */
	int rn_gdata0[0x100];	/* 6803 color map green data register */
	int rn_bdata0[0x100];	/* 6C03 color map blue data register */
	int nothing13[0xf0];	/* 7000 - 73BC */
	int rn_ovlmp1[0x10];	/* 73C3 overlay map register */
	int rn_rdata1[0x100];	/* 7403 color map red data register */
	int rn_gdata1[0x100];	/* 7803 color map green data register */
	int rn_bdata1[0x100];	/* 7C03 color map blue data register */
	int rn_te_halt;		/* 8002 Halt Register */
	int rn_bank;		/* 8006 Bank select */
	int rn_cdf;		/* 800A CD buffer full */
	int nothing14;		/* 800C */
	int rn_te_status;	/* 8012 */
	int nothing15[0xffb];	/* 8014 - BFFC */
	int rn_cstore;		/* C000 Control Store */
};

#ifdef _KERNEL
    /* Data exported from gr_98720.c */
    extern crt_frame_buffer_t template_98720;

#   define HP98720_MICROCODE_LEN	35
    extern int HP98720_microcode[HP98720_MICROCODE_LEN];
#endif
#endif
