/*
 * @(#)ite_dav.h: $Revision: 1.3.84.3 $ $Date: 93/09/17 20:58:46 $
 * $Locker:  $
 */

#ifndef __ITE_DAV_H_INCLUDED
#define __ITE_DAV_H_INCLUDED

struct dav_rgb {				/* An overlay colormap entry */
	char dummy1[3]; char r;		/* red */
	char dummy2[3]; char g;		/* green */
	char dummy3[3]; char b;		/* blue */
};

struct davinci {
	char x00[0x0001         ]; char  reset;
	char x01[0x0003-0x0001-1]; char  intrpt;
	char x02[0x4047-0x0003-1]; int   :7, wbusy:1;
	char x03[0x405b-0x4047-1]; int   :7, scanbusy:1;
	char x04[0x4090-0x405b-1]; int   fbwen;
	char x05[0x409f-0x4090-4]; char  wmove;
	char x06[0x40b3-0x409f-1]; char  fold;
	char x07[0x40b7-0x40b3-1]; char  opwen;
	char x08[0x40bf-0x40b7-1]; char  drive;
	char x09[0x40d7-0x40bf-1]; char  en_scan;
	char x10[0x40ef-0x40d7-1]; char  rep_rule;
	char x11[0x40f2-0x40ef-1]; short source_x;
	char x12[0x40f6-0x40f2-2]; short source_y;
	char x13[0x40fa-0x40f6-2]; short dest_x;
	char x14[0x40fe-0x40fa-2]; short dest_y;
	char x15[0x4102-0x40fe-2]; short wwidth;
	char x16[0x4106-0x4102-2]; short wheight;
	char x17[0x6003-0x4106-2]; char  cmapbank;
	char x18[0x6007-0x6003-1]; char  dispen;
	char x19[0x6023-0x6007-1]; char  vdrive;
	char x20[0x6083-0x6023-1]; char  panxh;
	char x21[0x6087-0x6083-1]; char  panxl;
	char x22[0x608b-0x6087-1]; char  panyh;
	char x23[0x608f-0x608b-1]; char  panyl;
	char x24[0x6093-0x608f-1]; char  zoom;
	char x25[0x6097-0x6093-1]; char  pz_trig;
	char x26[0x609b-0x6097-1]; char  ovly0p;
	char x27[0x609f-0x609b-1]; char  ovly1p;
	char x28[0x60a3-0x609f-1]; char  ovly0s;
	char x29[0x60a7-0x60a3-1]; char  ovly1s;
	char x30[0x60ab-0x60a7-1]; char  opvenp;
	char x31[0x60af-0x60ab-1]; char  opvens;
	char x32[0x60b3-0x60af-1]; char  fv_trig;
	char x33[0x60b7-0x60b3-1]; char  cdwidth;
	char x34[0x60bb-0x60b7-1]; char  chstart;
	char x35[0x60bf-0x60bb-1]; char  cvwidth;
	char x36[0x6100-0x60bf-1]; struct dav_rgb rgb[8];/* ovrlay color map */
	char x37[0x6403-0x6100-sizeof(struct dav_rgb)*8]; char image_r0;
	char x38[0x6803-0x6403-1]; char image_g0;
	char xa2[0x6c03-0x6803-1]; char image_b0;
	char xa3[0x7403-0x6c03-1]; char image_r1;
	char xa4[0x7803-0x7403-1]; char image_g1;
	char xa5[0x7c03-0x7803-1]; char image_b1;
	char x39[0x8012-0x7c03-1]; int :1, screset:1, :14;
	char x40[0xC23E-0x8012-2]; short pace_plug;
	char x41[0xD1FC-0xC23E-2]; int lgb_holdoff0;
	char x42[0xE1FC-0xD1FC-4]; int lgb_holdoff1;
	char x43[0xF1FC-0xE1FC-4]; int lgb_holdoff2;
};

#ifdef _KERNEL
    extern davinci_write(), davinci_write_off(), davinci_cursor(),
	   davinci_clear(), davinci_scroll(), davinci_init(),
	   davinci_pwr_reset(), davinci_set_st();
#endif

#endif /* __ITE_DAV_H_INCLUDED */
