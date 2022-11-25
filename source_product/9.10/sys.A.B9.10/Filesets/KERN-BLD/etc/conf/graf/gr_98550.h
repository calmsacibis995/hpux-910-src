/*
 * @(#)gr_98550.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 16:53:45 $
 * $Locker:  $
 */

#ifndef GR_98550_H_INCLUDED
#define GR_98550_H_INCLUDED

#define fe_addr rn_te_halt
#define fe_mode rn_bank

/* HP98550 status bit registers */
struct fc_ctlspace {
	int fc_idrom[0x4000];
	int nothing00[0x40];
	int fc_vblank_obs;	/* 4040 Vertical blanking is in progress */
	int nothing01[3];
	int fc_wbusy_obs;	/* 4044 Window move in progress, obsolete */
	int nothing02[0x3b];
	int fc_plane_mask_obs;	/* 4080 Bit plane enable mask, old position */
	int nothing03[3];
	int fc_sync_enb_obs;	/* 4084 Sync enable, old position */
	int nothing04[3];
	int fc_rr_wen_obs;	/* 4088 Rep rule write enable, old position */
	int nothing05[3];
	int fc_rr_ren_obs;	/* 408C Rep rule read enable, old position */
	int nothing06[3];
	int fc_fbwen_obs;	/* 4090 Frame buffer write enable, old pos. */
	int nothing07[0xb];
	int fc_wmove_trig;	/* 409C Start window move */
	int nothing08[3];
	int fc_blink_enb_obs;	/* 40A0 Plane blink enable, old position */
	int nothing09[0x49];
	int fc_prr_obs;		/* 40EA Pixel replacement rule, old position */
	int nothing10[3];
	int fc_wrr_obs;		/* 40EE Window replacement rule, old pos. */
	int nothing11[3];
	int fc_src_x_obs;	/* 40F2 X source/center/start, old posiiton */
	int nothing12[3];
	int fc_src_y_obs;	/* 40F6 Y source/center/start, old position */
	int nothing13[3];
	int fc_dest_x_obs;	/* 40FA X destination/center/end, old pos. */
	int nothing14[3];
	int fc_dest_y_obs;	/* 40FE Y destination/end, old position */
	int nothing15[3];
	int fc_wwidth_obs;	/* 4102 Window width, old position */
	int nothing16[3];
	int fc_wheight_obs;	/* 4106 Window height, old position */
	int nothing17[0xf9];
	int fc_srcdest_x;	/* 4200 X source and destination */
	int nothing18[3];
	int fc_rug_status;	/* 4204 Block mover status */
	int nothing19;
	int fc_rug_cmd;		/* 4206 Block mover command */
	int nothing20;
	int fc_wwidth;		/* 4208 Window width */
	int nothing21;
	int fc_wheight;		/* 420A Window height */
	int nothing22;
	int fc_line_rep;	/* 420C Line repeat pattern */
	int nothing23;
	int fc_line_type;	/* 420E Line type pointer */
	int nothing24;
	int fc_src_x;		/* 4210 X source/center/start */
	int nothing25;
	int fc_src_y;		/* 4212 Y source/center/start */
	int nothing26;
	int fc_dest_x;		/* 4214 X destination/center/end */
	int nothing27;
	int fc_dest_y;		/* 4216 Y destination/end */
	int nothing28;
	int fc_left_clip;	/* 4218 Left clip boundary */
	int nothing29;
	int fc_right_clip;	/* 421A Right clip boundary */
	int nothing30;
	int fc_top_clip;	/* 421C Top clip boundary */
	int nothing31;
	int fc_bottom_clip;	/* 421E Bottom clip boundary */
	int nothing32[0xe1];
	int fc_srcdest_x_trig;	/* 4300 X source and destination */
	int nothing33[7];
	int fc_wwidth_trig;	/* 4308 Window width */
	int nothing34;
	int fc_wheight_trig;	/* 430A Window height */
	int nothing35[5];
	int fc_src_x_trig;	/* 4310 X source/center/start */
	int nothing36;
	int fc_src_y_trig;	/* 4312 Y source/center/start */
	int nothing37;
	int fc_dest_x_trig;	/* 4314 X destination/center/end */
	int nothing38;
	int fc_dest_y_trig;	/* 4316 Y destination/end */
	int nothing39[0xe9];
	int fc_patt[0x100];	/* 4400 Pattern register file */
	int fc_fbwen;		/* 4500 Frame buffer write enable */
	int nothing40;
	int fc_prr;		/* 4502 Pixel replacement rule */
	int nothing41;
	int fc_rr_ren;		/* 4504 Rep rule read enable, norm planes */
	int nothing42;
	int fc_wrr;		/* 4506 Window replacement rule */
	int nothing43;
	int fc_rr_wen;		/* 4508 Rep rule write enable, norm planes */
	int nothing44;
	int fc_barc_rev;	/* 450A BARC revision ID */
	int nothing45;
	int fc_trr;		/* 450C Three op replacement rule */
	int nothing46;
	int fc_color;		/* 450E Vector color register */
	int nothing47;
	int fc_vector;		/* 4510 Vector/bitblit select */
	int nothing48;
	int fc_trr_ctrl;	/* 4512 Three op control */
	int nothing49;
	int fc_acntrl;		/* 4514 Bitblit control */
	int nothing50;
	int fc_plane_ctrl;	/* 4516 Plane source control */
	int nothing51;
	int fc_barc_id;		/* 4518 Barc ID register */
	int nothing52[0xe7];
	int fc_patt_ovl[0x60];	/* 4600 Pattern register file, overlay */
	int nothing53[0xa0];
	int fc_fbwen_ovl;	/* 4700 Frame buffer write enable, overlay */
	int nothing54;
	int fc_prr_ovl;		/* 4702 Pixel replacement rule, overlay */
	int nothing55;
	int fc_rr_ren_ovl;	/* 4704 Rep rule read enable, ovl planes */
	int nothing56;
	int fc_wrr_ovl;		/* 4706 Window replacement rule, overlay */
	int nothing57;
	int fc_rr_wen_ovl;	/* 4708 Rep rule write enable, ovl planes */
	int nothing58;
	int fc_barc_rev_ovl;	/* 470A BARC revision ID, overlay */
	int nothing59;
	int fc_trr_ovl;		/* 470C Three op replacement rule, overlay */
	int nothing60;
	int fc_color_ovl;	/* 470E Vector color register, overlay */
	int nothing61[0xf1];
	int fc_status;		/* 4800 Catseye status */
	int nothing62[0x1801];
	int fc_cmap_stat;	/* 6002 Color map status */
	int nothing63[0x3d];
	int fc_vblank_stat;	/* 6040 Vert blank status */
	int nothing64;
	int fc_disp_width;	/* 6042 Displayed width */
	int nothing65[3];
	int fc_hor_width;	/* 6046 Horizontal width */
	int nothing66[3];
	int fc_hsync_start;	/* 604A Horizontal sync start */
	int nothing67[3];
	int fc_hsync_stop;	/* 604E Horizontal sync stop */
	int nothing68;
	int fc_skdhs_start;	/* 6050 Skewed horiz sync start */
	int nothing69;
	int fc_scan_lower;	/* 6052 Total scan lines, lower byte */
	int nothing70;
	int fc_scan_upper;	/* 6054 Total scan lines, upper byte */
	int nothing71;
	int fc_vblank_width;	/* 6056 Vertical blank width */
	int nothing72;
	int fc_vsync_start;	/* 6058 Vertical sync start */
	int nothing73;
	int fc_vsync_stop;	/* 605A Vertical sync stop */
	int nothing74;
	int fc_blank_ctrl;	/* 605C Blank(video) control */
	int nothing75;
	int fc_skdhs_stop;	/* 605E Skewed horiz sync stop */
	int nothing76[0x25];
	int fc_sync_enb;	/* 6084 Sync enable */
	int nothing77[0x1b];
	int fc_blink_enb;	/* 60A0 Plane blink enable */
	int nothing78;
	int fc_ovl_ctrl;	/* 60A2 Overlay enable, blink, and mask */
	int nothing79[0xd];
	int fc_cmap_addr;	/* 60B0 Colormap address */
	int nothing80;
	int fc_rdata;		/* 60B2 Red data */
	int nothing81;
	int fc_gdata;		/* 60B4 Green data */
	int nothing82;
	int fc_bdata;		/* 60B6 Blue data */
	int nothing83;
	int fc_cmap_iaddr;	/* 60B8 Colormap inverted address */
	int nothing84;
	int fc_plane_mask;	/* 60BA Bit plane enable mask */
	int nothing85;
	int fc_cmap_ram_sel;	/* 60BC Cmap select, reg vs overlay */
	int nothing86;
	int fc_cmap_id;		/* 60BE Cmap controller id */
	int nothing87[0x31];
	int fc_cmap_wtrig;	/* 60F0 Colormap write trigger */
	int nothing88[7];
	int fc_cmap_rtrig;	/* 60F8 Colormap read trigger */
	int nothing89[0x9F07];
	int fcat_status;		/* 10000 Status register */ 
	int nothing100[0x6FFFF];
	int fcat_halt_req;		/* 80000 Halt request */
	int nothing101[3];
	int fcat_halt_ack;		/* 80004 Halt acknowledge */
	int nothing102[3];
	int fcat_cd_buf_sel;		/* 80008 CD buffer SPU select (0/1) */
	int nothing103[3];
	int fcat_cursor_buf_full;	/* 8000C Cursor buffer full (true>0) */
	int nothing104[3];
	int fcat_cd_buf1_full;		/* 80010 CD buffer #1  full (true>0) */
	int nothing105[3];
	int fcat_cd_buf2_full;		/* 80014 CD buffer #2  full (true>0) */
	int nothing106[3];
	int fcat_read_buf_owner;	/* 80018 Read buffer owner (SPU=0) */
	int nothing107[3];
	int fcat_cd_buf1_next;		/* 8001C Next location for SPU write */
	int nothing108[3];
	int fcat_cd_buf2_next;		/* 80020 Next location for SPU write */
	int nothing109[0xDF];
	int fcat_cd_buf1[0x800];	/* 80100 Command/data buffer #1 */
	int fcat_cd_buf2[0x800];	/* 80900 Command/data buffer #2 */
	int fcat_read_buf[0x800];	/* 81100 Read buffer */
	int fcat_cursor_buf[0x100];	/* 81900 Cursor buffer */
	int fcat_code[0x600];		/* 81A00 Fastcat code space SRAM */
	int nothing110[0x7E000];
	int fcat_dram[0x40000];		/* 100000 Fastcat state DRAM */
	int nothing111[0xC0000];	/* 140000-> 200000 */
};

/* Valid values for fc_ctlspace->fe_mode */
#define FEY_MODE_LONGWORD	0
#define FEY_MODE_WORD		2
#define FEY_MODE_BYTE		3

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

/*
 * Fastcat Halt Acknowledge register (halt_ack) bit definition(s).
 */
#define FASTCAT_HALTACK_TRUE		0x80000000

#ifdef _KERNEL
    /* Data exported from gr_98550.c */
    extern crt_frame_buffer_t template_98550;
#   ifdef FV_300
	extern crt_frame_buffer_t template_video300;
#   endif
#endif
#endif
