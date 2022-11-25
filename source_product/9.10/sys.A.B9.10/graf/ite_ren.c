/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_ren.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:59:49 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"
#include "../graf.300/ite_ren.h"

short ovl_bitm_cmap[8] = { /*     RGB   */	
	0x000, /* 0 black   */ 
	0x007, /* 1 white   */ 
	0x004, /* 2 red     */ 
	0x006, /* 3 yellow  */
	0x002, /* 4 green   */
	0x003, /* 5 cyan    */
	0x001, /* 6 blue    */
	0x005  /* 7 magenta */
};

static short renaissance_microcode[] = {
	0x5efe, 0x8a38, 0x0000, 0x0000, 0x0f00, 0, 0, 0,
	0x3efe, 0x8a38, 0x0000, 0x7003, 0xf706, 0, 0, 0,
	0x1efe, 0x8a38, 0x0000, 0x0000, 0x0000, 0, 0, 0,
	0x3efe, 0x8a38, 0x0000, 0x7003, 0xfc06, 0, 0, 0,
	0x1efe, 0x8a38, 0x0000, 0x0000, 0x0000, 0, 0, 0,
	0x3efe, 0x8a38, 0x0004, 0x40f7, 0xa006, 0, 0, 0,
	0x9efe, 0x8a38, 0x0000, 0x0000, 0x0000, 0, 0, 0,
	0x1efe, 0x8a38, 0x0000, 0x0000, 0x0000
};

/*
 * The Renaissance does not have a katakana font, so here is one.
 */
unsigned short renaissance_kata_font[120*8] = {
	0x0000, 0x3800, 0x4400, 0x7c00, 0x4400, 0x4400, 0x0000, 0x0700,
	0x0880, 0x0880, 0x0880, 0x0700, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0c00,
	0x1200, 0x1200, 0x0c00, 0x0000, 0x0000, 0x0000, 0x0000, 0x7c00,
	0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000,
	0x4000, 0x4000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0080, 0x0080,
	0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080,
	0x0f80, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x0800, 0x0400,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0c00, 0x0c00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x7f00, 0x0100, 0x0100, 0x3f00,
	0x0100, 0x0200, 0x0200, 0x0400, 0x0800, 0x1000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7e00, 0x0100,
	0x0900, 0x0a00, 0x0c00, 0x0800, 0x1000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0200, 0x0400, 0x0c00,
	0x1400, 0x2400, 0x0400, 0x0400, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0800, 0x0800, 0x7f00, 0x4100, 0x4100,
	0x0200, 0x0400, 0x0800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x3e00, 0x0800, 0x0800, 0x0800,
	0x0800, 0x7f00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0400, 0x0400, 0x7f00, 0x0c00, 0x1400, 0x2400, 0x4400,
	0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x1000, 0x1000, 0x7f80, 0x1080, 0x1100, 0x1200, 0x1000, 0x1000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x3e00, 0x0200, 0x0200, 0x0200, 0x0200, 0x0400, 0x7f80, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3f00,
	0x0100, 0x0100, 0x3f00, 0x0100, 0x0100, 0x3f00, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4900, 0x4900,
	0x2500, 0x0100, 0x0200, 0x0400, 0x0800, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7f80, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x7f00, 0x0080, 0x0080, 0x0900, 0x0a00, 0x0c00, 0x0800,
	0x0800, 0x1000, 0x2000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100,
	0x0100, 0x0200, 0x0400, 0x0c00, 0x1400, 0x2400, 0x4400, 0x0400,
	0x0400, 0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0400,
	0x7f80, 0x4080, 0x4080, 0x4080, 0x0100, 0x0300, 0x0200, 0x0400,
	0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7f00, 0x0800,
	0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x7f00,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0200, 0x0200, 0x7f80, 0x0600,
	0x0600, 0x0a00, 0x0a00, 0x1200, 0x2200, 0x4200, 0x0200, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0800, 0x0800, 0x7f80, 0x0880, 0x0880,
	0x0880, 0x0880, 0x1080, 0x1080, 0x2100, 0x4600, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0800, 0x0800, 0x7f00, 0x0800, 0x0800, 0x7f00,
	0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x1f80, 0x1080, 0x1080, 0x2100, 0x0100, 0x0100,
	0x0200, 0x0200, 0x0400, 0x1800, 0x0000, 0x0000, 0x0000, 0x0000,
	0x1000, 0x1000, 0x1f80, 0x2200, 0x4200, 0x0200, 0x0200, 0x0200,
	0x0200, 0x0400, 0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x3e00, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
	0x0100, 0x3f00, 0x0000, 0x0000, 0x0000, 0x0000, 0x1200, 0x1200,
	0x7f80, 0x1200, 0x1200, 0x1200, 0x0200, 0x0400, 0x0400, 0x0800,
	0x1000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3800, 0x0000,
	0x0080, 0x7080, 0x0080, 0x0100, 0x0100, 0x0200, 0x2400, 0x3800,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3e00, 0x0100, 0x0100,
	0x0200, 0x0200, 0x0400, 0x0a00, 0x1100, 0x2100, 0x4080, 0x0000,
	0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x7f00, 0x1080, 0x1080,
	0x1100, 0x1200, 0x1000, 0x1000, 0x0880, 0x0700, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x4180, 0x4080, 0x2080, 0x1080, 0x0100,
	0x0100, 0x0200, 0x0200, 0x0400, 0x1800, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0f80, 0x0880, 0x1080, 0x2880, 0x0500, 0x0300,
	0x0200, 0x0400, 0x0800, 0x3000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0100, 0x3f00, 0x0400, 0x0400, 0x7f80, 0x0400, 0x0400, 0x0400,
	0x0800, 0x0800, 0x3000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x4880, 0x4880, 0x2480, 0x0080, 0x0080, 0x0100, 0x0100, 0x0200,
	0x0400, 0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3f00,
	0x0000, 0x0000, 0x7f80, 0x0400, 0x0400, 0x0400, 0x0800, 0x0800,
	0x3000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x2000, 0x2000,
	0x2000, 0x3800, 0x2400, 0x2200, 0x2000, 0x2000, 0x2000, 0x2000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0400, 0x0400, 0x7f80, 0x0400,
	0x0400, 0x0400, 0x0400, 0x0400, 0x0800, 0x0800, 0x3000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3f00, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x7f80, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x3e00, 0x0100, 0x0100, 0x0200, 0x1200,
	0x0c00, 0x0c00, 0x1200, 0x2100, 0x4000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0400, 0x0400, 0x3f00, 0x0100, 0x0200, 0x0400, 0x0e00,
	0x1500, 0x2480, 0x4400, 0x0400, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0380, 0x0080, 0x0080, 0x0100, 0x0100, 0x0200, 0x0200,
	0x0400, 0x0800, 0x3000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0200, 0x1100, 0x1100, 0x1080, 0x1080, 0x1080, 0x2040, 0x2040,
	0x2040, 0x4040, 0x0000, 0x0000, 0x0000, 0x0000, 0x2000, 0x2000,
	0x3f00, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2000, 0x2080,
	0x1f00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7f00, 0x0080,
	0x0080, 0x0080, 0x0100, 0x0100, 0x0200, 0x0200, 0x0400, 0x1800,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x1400, 0x2200,
	0x2100, 0x0100, 0x0100, 0x0080, 0x0080, 0x0080, 0x0080, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0800, 0x0800, 0x7f00, 0x0800, 0x0800,
	0x2a00, 0x2a00, 0x2a00, 0x4900, 0x4900, 0x0800, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x7f00, 0x0080, 0x0080, 0x0100, 0x0100,
	0x2200, 0x1400, 0x0800, 0x0400, 0x0400, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x3e00, 0x0100, 0x0000, 0x3c00, 0x0200, 0x0000,
	0x0000, 0x3c00, 0x0300, 0x0080, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0400, 0x0800, 0x0800, 0x1000, 0x1000, 0x2000, 0x2100, 0x4100,
	0x4080, 0x4080, 0x3f00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0100, 0x0100, 0x1100, 0x0e00, 0x0600, 0x0600, 0x0500, 0x0900,
	0x0800, 0x3000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3f00,
	0x0800, 0x0800, 0x7f80, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800,
	0x0780, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0x1000, 0x7f00,
	0x1080, 0x1080, 0x1100, 0x1200, 0x1000, 0x1000, 0x1000, 0x1000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e00, 0x0100, 0x0100,
	0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x3f80, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x3e00, 0x0100, 0x0100, 0x0100,
	0x3f00, 0x0100, 0x0100, 0x0100, 0x0100, 0x3f00, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x3f00, 0x0000, 0x0000, 0x7f00, 0x0080,
	0x0080, 0x0100, 0x0100, 0x0200, 0x1c00, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x1100, 0x1100, 0x1100, 0x1100, 0x1100, 0x0100,
	0x0200, 0x0200, 0x0400, 0x1800, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0400, 0x2400, 0x2400, 0x2400, 0x2400, 0x2480, 0x2480,
	0x2480, 0x2500, 0x4600, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x2000, 0x2000, 0x2000, 0x2100, 0x2100, 0x2100, 0x2200, 0x2200,
	0x2400, 0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1e00,
	0x2100, 0x2100, 0x2100, 0x2100, 0x2100, 0x2100, 0x2100, 0x2100,
	0x3f00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3f00, 0x4080,
	0x4080, 0x4080, 0x0080, 0x0100, 0x0100, 0x0200, 0x0400, 0x1800,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6000, 0x1000,
	0x0880, 0x0080, 0x0080, 0x0100, 0x0200, 0x4c00, 0x7000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0400, 0x2200, 0x1200, 0x1000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x1800, 0x2400, 0x2400, 0x1800, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/*			   R E N A I S S A N C E 			*/
/*			       (alias SRX)				*/
/*			     (alias hp98720a)				*/

#define renais_bmove_wait(ren) while ((ren)->wbusy & 0x1);

#define REN_ENABLE 0x8;
#define REN_DISABLE 0x0;

short	ren_drive;
int	ren_opwen;
short	ren_rep_rule;
int	ren_fbwen;
short	ren_fbven;
short	ren_wheight;
short	ren_wwidth;
short	ren_dest_x;
short	ren_dest_y;
short	ren_source_x;
short	ren_source_y;

extern int static_colormap;

renais_write(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);
    register i;

    /* check if already saved registers */
    if (ite->flags & REN_SAVED_REG) {
	renais_bmove_wait(ren);
	}
    else {
	/* wait up to 1000 ms for a go */
	i = 1000;
	do {
		/* wait for scan board and bit mover to not be busy */
		if (((ren->scanbusy & 0xff) | (ren->wbusy & 0x1)) == 0)
			break;
				
		/* wait another millisecond */
		snooze(1000);
	} while (--i);
	
	/* if timed out, punt */
	if (i == 0)
	    renais_pwr_reset(ite);
	
	ren_rep_rule = ren->rep_rule;
	ren_drive = ren->drive;
	
	/* write enable allowed planes to frame buffer */
	if (ite->overlay_planes) {
	    ren->drive = 0x0010;		/* overlay plane enable */
	    ren_opwen = ren->opwen;
	}
	else {
	    ren->drive = 0x0001;		/* board 0 plane enable */
	    ren_fbwen = ren->fbwen;
	}
	/* save window mover registers */
	ren_wheight	= ren->wheight;
	ren_wwidth	= ren->wwidth;
	ren_dest_x	= ren->dest_x;
	ren_dest_y	= ren->dest_y;
	ren_source_x	= ren->source_x;
	ren_source_y	= ren->source_y;
	ite->flags |= REN_SAVED_REG;
    }
	
    ren->intrpt = 0x04;				/* enable frame buffer */
    ren->rep_rule = 0x0033;			/* ... for itecrt_write() */

    if (ite->overlay_planes)
	 ren->opwen = ite->cur_planes;
    else ren->fbwen = ite->cur_planes;
}

renais_cursor(ite, x, y, color_num)				    /* ENTRY */
    struct iterminal *ite;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    /* set up the hardware before writing out pixels */
    renais_write(ite);

    ren->rep_rule = 0x0066;		/* set pixel replacement rule XOR */
    bitmap_cursor(ite, x, y, color_num);

    /* restore registers */
    if (ite->cursor_on) {
	ren->rep_rule = ren_rep_rule;
	ren->drive = ren_drive;
	if (ite->overlay_planes)
	     ren->opwen=ren_opwen;
	else ren->fbwen=ren_fbwen;
		
	/* restore window mover registers */
	ren->wheight	= ren_wheight; 
	ren->wwidth	= ren_wwidth; 
	ren->dest_x	= ren_dest_x; 
	ren->dest_y	= ren_dest_y; 
	ren->source_x	= ren_source_x; 
	ren->source_y	= ren_source_y; 

	/* if I halted transform engine, resume it! */
	if (ite->flags & REN_TFORM_HALT) {
		ren->te_int0 = 0x00; 
		ren->te_halt = 0x00; /* start the transform engine */
		ite->flags &= ~REN_TFORM_HALT; /* not halted now */
	}
	ite->flags &= ~REN_SAVED_REG;
    }
}

renais_clear(ite, y, x, count)					    /* ENTRY */
    register struct iterminal *ite;
    register y, count;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);
    register len;
    register short source_x, source_y, width;
    int *ren_FBWEN;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    /* set pointer to current alpha plane write enable register */
    ren_FBWEN = ite->overlay_planes ?  &ren->opwen : &ren->fbwen;

    source_x = x * ite->font_width;
    source_y = y * ite->line_height;

    len = scr_min(ite->screenwidth - x, count);

    /* set up the hardware before writing out pixels */
    renais_write(ite);

    ren->wheight = ite->line_height;

    while (count) {
	count -= len;
	width = len * ite->font_width;
	len = scr_min(ite->screenwidth, count);

	renais_bmove_wait(ren);
	ren->wwidth = width;

	/* set dest of x and y for clear */
	ren->dest_x = source_x;
	ren->dest_y = source_y;

	/* write enable just planes that need setting */
	if (*ren_FBWEN = ite->color_pairs[0].BG & ite->cur_planes) {
	    ren->rep_rule = 0x00ff;		   /* replacement rule = set */
	    ren->wmove = 1;
	    renais_bmove_wait(ren);
	}

	/* write enable just planes that need clearing */
	if (*ren_FBWEN = ~ite->color_pairs[0].BG & ite->cur_planes) {
	    ren->rep_rule = 0x0000;	/* replacement rule = clear */
	    ren->wmove = 1;
	}

	source_x = 0;			/* the remaining lines start at zero */
	source_y += ite->line_height;		    /* bump to the next line */
    }
}


renais_block_clear(ite, x, y, height, width)			    /* ENTRY */
    register struct iterminal *ite;
    int x, y, height, width;
/*
 *  This routine simply implements a device dependent (Renaissance) block
 *  move.  In this case a block clear.
 *  
 *  The routine first checks to ensure the GPU is not busy.  It then readies
 *  the box for a block move, setting up the necessady registers.
 *
 *  The routine then performs the clear, using the block mover.  Since
 *  clearing a screen really just means setting the screen to the currently
 *  selected background color, there are two 'if' statements.  The first
 *  covers the case where planes need to be set (a color turned on).  The 
 *  second covers the case where planes need to be cleared.  These cases
 *  loosely correspond to filling in a colored background and clearing out 
 *  a black background respectively.
 */

{
    register struct renaissance *ren = (struct renaissance *) ite->card;
    int *ren_FBWEN;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    /* set pointer to current alpha plane write enable register */
    ren_FBWEN = ite->overlay_planes ?  &ren->opwen : &ren->fbwen;

    /* set up the hardware before writing out pixels */
    renais_write(ite);

    ren->wheight = height;	/* height of window to move */
    ren->wwidth  = width;	/* width of window to move */
    ren->dest_x  = x;
    ren->dest_y  = y;

    /* write enable just planes that need setting */
    if (*ren_FBWEN = ite->color_pairs[0].BG & ite->cur_planes) {

	ren->rep_rule = 0x00ff;	/* replacement rule = set */
	ren->wmove = 1;		/* start block mover */
	renais_bmove_wait(ren); /* wait for it */
    }
    /* write enable just planes that need clearing */
    if (*ren_FBWEN = ~ite->color_pairs[0].BG & ite->cur_planes) {

	ren->rep_rule = 0x0000;	/* replacement rule = clear */
	ren->wmove = 1;		/* start block mover */
    }
}


renais_scroll(ite, to_row, dir, width)				    /* ENTRY */
register struct iterminal *ite;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);
    register short dest_y;

    if ((width *= ite->font_width) == 0)  /* calculate width of line to move */
	return;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, to_row);
	return;
    }
    dest_y = to_row * ite->line_height;

    /* set up the hardware before writing out pixels */
    renais_write(ite);

    ren->dest_x = 0;
    ren->source_x = 0;			   /* source x pixel is same as dest */
    ren->dest_y = dest_y;
    /* source is one line behind or one line ahead */
    ren->source_y = dest_y + 
	((dir == UP) ? ite->line_height : -ite->line_height);

    ren->wwidth = width;
    ren->wheight = ite->font_height;
    ren->rep_rule = 0x0033;			  /* replacement rule = move */
    ren->wmove = 1;
}

renais_write_off(ite, vlp, y, x, count)				    /* ENTRY */
    register struct iterminal *ite;
    register SCROLL *vlp; 
    register x, count;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);
    register int *ren_FBWEN;
    register color_pair, scrollvalue, dest_x; 
    register clear_planes, set_planes;
    register move_case;
    int back_c, fore_c, planes=ite->cur_planes;
    struct offscreen_font offscreen_xy;
    unsigned short dest_y;

    /* check if any thing to do */
    if (count <= 0)
	return;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }
    dest_x = ite->font_width * x;
    dest_y = ite->line_height * y;

    /* set up the hardware before writing out pixels */
    renais_write(ite);

    ren->wwidth = ite->font_width;
    ren->wheight = ite->font_height;

    /* set pointer to current alpha plane write enable register */
    ren_FBWEN = ite->overlay_planes ?  &ren->opwen : &ren->fbwen;

    ren->dest_y = dest_y;

    color_pair = ITE_BOGUS_CPAIR;      /* force calculation of current color */

    do {
	/* source x & y pixel depends on char in offscreen memory */
	offscreen_xy = offscreen_font[(scrollvalue = *vlp++) &CHAR_VAL];

	/* check if current color and inverse video bits changed */
	if (color_pair != char_enhance(scrollvalue)) {
		/* if underline finish off using ram font routine */
		if (scrollvalue & UNDERLINE) {
			itecrt_write(ite, --vlp, y, x, count);
			return;
		}
		/* build new pixel_color tab, exit if char not enabled*/
		if (ite_build_pixtbl(ite, scrollvalue))
			return;

		/* set the colorpair to current color and enhancement */
		color_pair = ite->current_cpair;

		fore_c = ite->pixel_colors[15];
		back_c = ite->pixel_colors[0];

		/*            rule    case 1  case 2  case 3  case 4 */
		/*    FG =		0	1	0	1    */
		/*    BG =		0	0	1	1    */
		/*	              _____   _____   _____   _____  */
		/* ~(FG | BG)	0	1       0       0       0    */
		/* ~BG & FG	3	0	1	0	0    */
		/* ~FG & BG    12	0	0	1	0    */
		/*  FG & BG    15	0	0	0	1    */

		/* set up all planes incase simple case */
		move_case = FALSE;
		set_planes = back_c & planes;
		clear_planes = ~back_c & planes;
			
		renais_bmove_wait(ren);
		/* set to all enabled planes as default */
		*ren_FBWEN = planes;

		/* check if just simple move rule enough */
		if ((~back_c & fore_c & planes) == planes) {
			ren->rep_rule = 0x0033; /* rep rule: MOVE */

		/* check if just simple complement move rule enough */
		} else if ((~fore_c & back_c & planes) == planes) {
			ren->rep_rule = 0x00cc; /* rep rule: CMP_MOVE */
		/* well we must do at least one extra move, maybe two */
		}
		else move_case = TRUE;
	}
	renais_bmove_wait(ren);

	/* set dest pixel to next character in line */
	ren->dest_x = dest_x;

	if (move_case) {
		/* check if planes to set */
		if (*ren_FBWEN = set_planes) {
			ren->rep_rule = 0x00ff; /* rep rule: set  */
			ren->wmove = 1;
			renais_bmove_wait(ren);
		}
		/* now clear non background planes, if any */
		if (*ren_FBWEN = clear_planes) {
	 		ren->rep_rule = 0x0000; /* rep rule: clear  */
			ren->wmove = 1;
			renais_bmove_wait(ren);
		}
		/* move only changes planes from offscreen mem */
		*ren_FBWEN = (fore_c ^ back_c) & planes;
		ren->rep_rule = 0x0066;		/* rep rule: XOR */
	}
	/* source x pixel depends on char in offscreen memory */
	ren->source_x = offscreen_xy.X;

	/* source x pixel depends on char in offscreen memory */
	ren->source_y = offscreen_xy.Y;

	ren->wmove = 1;				/* start block mover */

	/* bump to next character address for next pass */
	dest_x += ite->font_width;
	x++;
    } while (--count);
}

/*
 * turn on/off alpha and graphics
 */
renais_set_st(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);
    register i, k=0, map0_enable, map1to7_enable, black_flag=FALSE;

    /* Ignore if we have a static colormap */
    if (static_colormap)
	return;

    if (!ite->overlay_planes) {
	/* If neither is on, turn on alpha */
	if ((ite->flags & (GRAPHICSON|ALPHAON)) == 0)
	    ite->flags |= ALPHAON;
	ren_fbven=0;
	if (ite->flags & GRAPHICSON)
	    ren_fbven |= ~ite->cur_planes;

	if (ite->flags & ALPHAON)
	    ren_fbven |= ite->cur_planes;

	ren_fbven &= 0x0f;		/* only four planes total! */
	ren->fbven = ren_fbven;

	/* set up 1st 8 color maps maps */
	for (i=0; i<ITE_MAX_CPAIRS; i++) {
	    /* skip maps that are not currently allowed */
	    if ((i & ite->cur_planes) != i)
		continue;

	    ren->rdata0[i].data = bitm_colormap[i][0];	   /* set color maps */
	    ren->rdata1[i].data = bitm_colormap[i][0];
	    ren->gdata0[i].data = bitm_colormap[i][1];
	    ren->gdata1[i].data = bitm_colormap[i][1];
	    ren->bdata0[i].data = bitm_colormap[i][2];
	    ren->bdata1[i].data = bitm_colormap[i][2];
	}
	return;
    }

    switch (ite->flags & (GRAPHICSON | ALPHAON)) {
	default: /* black */
		black_flag = TRUE;
		/* fall thru */

	case ALPHAON: /* alpha on , graphics off */
		map0_enable = REN_ENABLE;
		map1to7_enable = REN_ENABLE;
		break;

	case GRAPHICSON|ALPHAON: /* graphics on, alpha on */
		map0_enable = REN_DISABLE;
		map1to7_enable = REN_ENABLE;
		break;

	case GRAPHICSON: /* graphics on, alpha off */
		map0_enable = REN_DISABLE;
		map1to7_enable = REN_DISABLE;
		break;
    }

    for (i=0; i<ITE_MAX_CPAIRS; i++) {
	/* skip maps that are not currently allowed */
	if ((i & ite->cur_planes) != i)
	    continue;
	
	/* special case both off to all black */
	if (!black_flag)
	    k = i;

	/* set the RGB regs */
	ren->ovlmp0[i].data = ovl_bitm_cmap[k] | map0_enable;
	ren->ovlmp1[i].data = ovl_bitm_cmap[k] | map0_enable;

	/* enable only after the first map (clear not black) */
	map0_enable = map1to7_enable;
    }
}

renais_init(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, 0);
	return;
    }

    renais_write(ite);				      /* set up the hardware */
    renais_set_st(ite);

    /* now restore offscreen memory from in ram font */
    (*ite->crt_font_restore)(ite);

    /* turn on video in case some application turned it off */
    ren->dispen = 0x0001;
}

renais_pwr_reset(ite)						    /* ENTRY */
struct iterminal *ite;
{
    register struct renaissance *ren = (struct renaissance *)(ite->card);
    register i;
    register short *renais_p1, *renais_p2;
    char dummy;

    /* Did we mess up the structure?  Make sure the last entry is right. */
    if ((int) &(((struct renaissance *)0)->cstore) != 0xc000)
	panic("struct renaissance messed up");

    /*
     * The Renaissance may be hung, in a bad state.
     *
     * If it is, we can not just write to it because it might
     * not DTACK (handshake the write).  Hence we use bcopy_prot()
     * to write to it and ignore bus errors.
     *
     * Do this twice in case the first one failed for some hardware reason.
     */

    dummy = 0x39;
    bcopy_prot(&dummy, &ren->rreset, 1);
    snooze(10);			/* Renaissance is OTL for 3us */
    bcopy_prot(&dummy, &ren->rreset, 1);
    snooze(10);			/* Renaissance is OTL for 3us */

    if (testr(&ren->te_halt, 1)) {
	/* reset and halt transform engine */
	ren->te_halt = 0xa000;
	ren->te_halt = 0x2000;
	ren->te_halt = 0x8000;

	renais_p1 = renaissance_microcode;
	renais_p2 = &ren->cstore;

	/* map 1st bank microcode addrs space */
	ren->bank = 0x0000;

	/* load microcode into renaissance */
	for (i = 0; i < sizeof(renaissance_microcode)/sizeof(renaissance_microcode[0]); i++)
	    *renais_p2++ = *renais_p1++;

	ren->te_halt = 0x2000;			 /* assert RESET, clear HALT */
	ren->te_halt = 0x0000;			     /* run transform engine */

	/* wait for renaissance non busy */
	for (i = 0; i < 1000; i++) {
	    if (ren->te_int0 < 0)
		continue;
	    ite->flags |= REN_TFORM_ENGN;
		break;
	}
    }

    /* check if there are overlay planes */
    if (ren->color_stat & 0x10) 
	ite->overlay_planes = TRUE;

    ren->fold = 0x0001;

    /* set up color map entry 0 to black for full memory clear */
    ren->rdata0[0].data = 0;
    ren->rdata1[0].data = 0;
    ren->gdata0[0].data = 0;
    ren->gdata1[0].data = 0;
    ren->bdata0[0].data = 0;
    ren->bdata1[0].data = 0;

    /* clear all 16 overlay color maps */
    for (i = 0; i<16; i++) {
	ren->ovlmp0[i].data = 0;
	ren->ovlmp1[i].data = 0;
    }
    /* now clear all frame and overlay memory */
    ren->fbven = ite->overlay_planes ? 0x00 : 0x07;

    ren->fbwen = 0xffffffff;		   /* enable all frame buffer planes */
    ren->opwen = 0xf;			 /* enable all overlay buffer planes */
    ren->drive = 0x00ff;
    ren->rep_rule = 0x0000;			 /* replacement rule = CLEAR */
    ren->dest_x = 0x0000;
    ren->dest_y = 0x0000;
    ren->wwidth = 0x800;
    ren->wheight = 0x400;
    ren->wmove = 1;
    renais_bmove_wait(ren);
    ren->dispen = 0x0001;				    /* turn on video */
    ren->intrpt = 0x04;				      /* enable frame buffer */
}
