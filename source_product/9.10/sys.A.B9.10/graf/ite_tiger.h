/*
 * @(#)ite_tiger.h: $Revision: 1.3.84.4 $ $Date: 93/12/06 16:41:25 $
 * $Locker:  $
 */

#ifndef __ITE_TIGER_H_INCLUDED
#define __ITE_TIGER_H_INCLUDED

struct tiger_rgb {			/* An overlay colormap entry */
    char dummy1[3]; char r;
    char dummy2[3]; char g;
    char dummy3[3]; char b;
};

struct tiger {
    char x00[0x0001         ]; char   reset;

    char x01[0x5003-0x0001-1]; char   fv_trig;
    char x04[0x500f-0x5003-1]; char   dispen;
    char x05[0x5013-0x500f-1]; char   vdrive;
    char x06[0x5023-0x5013-1]; char   opvenp;		/*  color map    */
    char x07[0x5027-0x5023-1]; char   opvens;
    char x08[0x502b-0x5027-1]; char   ov_blink;
    char x09[0x502f-0x502b-1]; char   ovly0p;
    char x10[0x5033-0x502f-1]; char   ovly1p;
    char x11[0x5037-0x5033-1]; char   ovly0s;
    char x12[0x503b-0x5037-1]; char   ovly1s;
    char x13[0x5200-0x503b-1]; struct tiger_rgb oprimary[8]; 
    char x14[0x5300-0x5200-sizeof(struct tiger_rgb)*8]; 
                               struct tiger_rgb osecondary[8];

    char x15[0x7047-0x5300-sizeof(struct tiger_rgb)*8]; int    :7, wbusy:1;
    char x16[0x705b-0x7047-1]; int    :7, as_busy:1;
    char x17[0x7093-0x705b-1]; char   fbwen;
    char x18[0x709f-0x7093-1]; char   wmove;
    char x19[0x70b3-0x709f-1]; char   fold;
    char x20[0x70b7-0x70b3-1]; char   opwen;
    char x21[0x70bf-0x70b7-1]; char   drive;
    char x22[0x70ef-0x70bf-1]; char   rep_rule;		/*  framebuffer  */
    char x23[0x70f2-0x70ef-1]; short  source_x;
    char x24[0x70f6-0x70f2-2]; short  source_y;
    char x25[0x70fa-0x70f6-2]; short  dest_x;
    char x26[0x70fe-0x70fa-2]; short  dest_y;
    char x27[0x7102-0x70fe-2]; short  wwidth;
    char x28[0x7106-0x7102-2]; short  wheight;

    char x29[0xf23e-0x7106-2];     short pace_plug;
    char x30[0x180018-0xf23e-2];   int   te_status;
    char x31[0x3ffffc-0x180018-4]; int   lgb_holdoff;
};

#ifdef _KERNEL
    extern tiger_write_setup(), tiger_write_off(), tiger_cursor(),
	   tiger_clear(), ite_block_clear(), tiger_scroll(), tiger_init(),
	   tiger_pwr_reset(), tiger_set_st();
#endif

#endif /* __ITE_TIGER_H_INCLUDED */
