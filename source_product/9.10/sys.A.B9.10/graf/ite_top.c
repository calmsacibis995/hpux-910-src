/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_top.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:00:33 $
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

#include "../h/param.h"
#include "../graf/ite.h"
#include "../graf/ite_color.h"
#include "../graf/ite_font.h"
#include "../graf/ite_scroll.h"
#include "../graf.300/ite_top.h"

/*
 * Low-resolution Bobcat does not have a Katakana font in its ID rom,
 * so we put one here.
 */
unsigned short lo_res_kata_font[] = {
	0x0000, 0x4200, 0x4200, 0x7E00, 0x4200, 
	0x4200, 0x0000, 0x07C0, 0x0420, 0x0420, 
	0x07C0, 0x0400, 0x0400, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0E00, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x1F80, 0x1800, 0x1800, 
	0x1800, 0x1800, 0x1800, 0x1800, 0x1800, 
	0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0180, 0x0180, 0x0180, 
	0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 
	0x1F80, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0E00, 
	0x0E00, 0x0600, 0x0C00, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0E00, 0x0E00, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FE0, 
	0x0060, 0x3FE0, 0x00C0, 0x0180, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x3FC0, 0x00C0, 0x0780, 0x0600, 
	0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x00C0, 0x0300, 0x0E00, 0x7600, 0x0600, 
	0x0600, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0600, 0x3FC0, 0x30C0, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x3FC0, 0x0600, 0x0600, 
	0x3FC0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0300, 0x7FC0, 0x0700, 0x0F00, 0x1B00, 
	0x6300, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x1800, 0x7FC0, 0x18C0, 0x1980, 0x1800, 
	0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x1F80, 0x0180, 0x0300, 0x0600, 
	0x7FE0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x3FC0, 0x00C0, 0x1FC0, 0x00C0, 
	0x7FC0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x6C60, 0x3660, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x7FC0, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FC0, 
	0x0060, 0x0660, 0x0780, 0x0600, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0060, 0x00C0, 0x0180, 
	0x0380, 0x0D80, 0x7180, 0x0180, 0x0180, 
	0x0180, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0600, 0x0600, 0x7FE0, 
	0x6060, 0x6060, 0x0060, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3FC0, 
	0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 
	0x7FE0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0180, 0x0180, 0x7FE0, 
	0x0380, 0x0780, 0x0D80, 0x1980, 0x6180, 
	0x0180, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0C00, 0x0C00, 0x7FE0, 
	0x0C60, 0x0C60, 0x0C60, 0x1860, 0x1860, 
	0x31C0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0600, 0x0600, 0x3FC0, 
	0x0600, 0x7FE0, 0x0600, 0x0600, 0x0600, 
	0x0600, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x1FE0, 
	0x3060, 0x6060, 0x00C0, 0x0180, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x1800, 0x1800, 0x1FE0, 
	0x3180, 0x6180, 0x0180, 0x0180, 0x0300, 
	0x1C00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3F80, 
	0x00C0, 0x00C0, 0x00C0, 0x00C0, 0x00C0, 
	0x7FC0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x1980, 0x1980, 0x7FE0, 
	0x1980, 0x1980, 0x0180, 0x0180, 0x0300, 
	0x1C00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3C00, 
	0x0000, 0x7860, 0x0060, 0x00C0, 0x3180, 
	0x3E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FC0, 
	0x0060, 0x0060, 0x0180, 0x0600, 0x1980, 
	0x6060, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x1800, 0x1800, 0x7FC0, 
	0x1860, 0x18C0, 0x1800, 0x1800, 0x1860, 
	0x0FC0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x60E0, 
	0x3460, 0x1860, 0x0060, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x1FE0, 
	0x3060, 0x6060, 0x06C0, 0x0180, 0x0660, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x01C0, 0x3F00, 
	0x0300, 0x7FE0, 0x0300, 0x0300, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x6660, 
	0x3360, 0x0060, 0x0060, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3FC0, 
	0x0000, 0x7FE0, 0x0300, 0x0300, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x1800, 0x1800, 0x1800, 
	0x1F80, 0x18E0, 0x1800, 0x1800, 0x1800, 
	0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0300, 0x0300, 0x7FE0, 
	0x0300, 0x0300, 0x0300, 0x0300, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3FC0, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x7FE0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FC0, 
	0x0060, 0x3060, 0x0CC0, 0x0300, 0x0CC0, 
	0x3060, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0600, 0x0600, 0x7FC0, 
	0x0180, 0x0600, 0x1F80, 0x6660, 0x0600, 
	0x0600, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x03C0, 
	0x00C0, 0x00C0, 0x0180, 0x0300, 0x0C00, 
	0x7000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0300, 0x0180, 
	0x18C0, 0x18C0, 0x1860, 0x3060, 0x3060, 
	0x6060, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x3000, 0x3000, 0x3FC0, 
	0x3000, 0x3000, 0x3000, 0x3000, 0x30C0, 
	0x1F80, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FE0, 
	0x0060, 0x0060, 0x00C0, 0x0180, 0x0600, 
	0x3800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x1E00, 
	0x3300, 0x6180, 0x00C0, 0x00C0, 0x0060, 
	0x0060, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0600, 0x0600, 0x7FE0, 
	0x0600, 0x0600, 0x36C0, 0x36C0, 0x36C0, 
	0x6660, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FC0, 
	0x0060, 0x00C0, 0x0180, 0x3B00, 0x0600, 
	0x0180, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3F80, 
	0x0040, 0x1F00, 0x0080, 0x0000, 0x7FC0, 
	0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0300, 0x0600, 
	0x0C00, 0x1800, 0x3000, 0x60C0, 0x6060, 
	0x3FC0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0060, 0x0060, 
	0x30C0, 0x0D80, 0x0300, 0x06C0, 0x1860, 
	0x6000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3FC0, 
	0x0C00, 0x7FE0, 0x0C00, 0x0C00, 0x0C60, 
	0x07C0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x1800, 0x1800, 0x7FE0, 
	0x1860, 0x18C0, 0x1800, 0x1800, 0x1800, 
	0x1800, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3F00, 
	0x0180, 0x0180, 0x0300, 0x0300, 0x0600, 
	0x7FE0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3FC0, 
	0x0060, 0x0060, 0x1FE0, 0x0060, 0x0060, 
	0x7FE0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3FC0, 
	0x0000, 0x7FE0, 0x0060, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x3180, 0x3180, 
	0x3180, 0x0180, 0x0180, 0x0180, 0x0300, 
	0x1C00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3300, 
	0x3300, 0x3300, 0x3360, 0x3360, 0x3360, 
	0x63C0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3000, 
	0x3000, 0x30C0, 0x30C0, 0x3180, 0x3300, 
	0x3C00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x3F80, 
	0x60C0, 0x60C0, 0x60C0, 0x60C0, 0x60C0, 
	0x7FC0, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7FE0, 
	0x6060, 0x6060, 0x0060, 0x00C0, 0x0180, 
	0x0E00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x7800, 
	0x00C0, 0x00C0, 0x00C0, 0x0180, 0x6300, 
	0x7C00, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x3300, 0x3300, 0x3300, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x1E00, 0x3300, 0x3300, 0x1E00, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
};

/*     				B O B C A T				*/

/* Must pause after reading/writing NERIED chip */
#define NERIED_WAIT {char neried_waiter = *(char *) (DIO_START+0x3b); }

/*
 * Wait for block mover to finish.
 */
bobcat_bmove_wait(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);
    register short planes, *busy_reg;
    register int wait_time, i;

    /*
     * Catseye has a nice non-intruding bugless status register.
     * Or so they say...
     */
    if (ite->flags & CATSEYE) {
	while (cat->cat_wbusy)
		/* Wait for block mover */;
	return;
    }

    planes = ite->cur_planes << 8;
    busy_reg = &cat->wmove;

    /*
     * Due to a defect in topcat or the cpu board or something,
     * the block mover sometimes says that it's finished when it's
     * really still busy.  Hence, insist on several yes's in a row.
     */
    wait_time=10;
    while ((planes & *busy_reg)
	    || (planes & *busy_reg)
	    || (planes & *busy_reg)
	    || (planes & *busy_reg)
	    || (planes & *busy_reg)) {
	/* Reading from topcat slows it down.  Kill time */
	for (i=wait_time; --i>=0; )
	    ;
	wait_time += wait_time;
    }
}

bobcat_cursor(ite, x, y, color_num)				    /* ENTRY */
    struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);
    register int i;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }
    bobcat_bmove_wait(ite);
    cat->tcwen = 0xff00;	/* enable all planes for block move */
    cat->prr = 0x0600;		/* set pixel replacement rule XOR */
    cat->fben = ite->cur_planes<<8; /* enable all planes for frame buffer */

    bitmap_cursor(ite, x, y, color_num);

    /* start the fastcat up again */
    if (ite->cursor_on && (ite->flags & CAT_TFORM_ENGN)
    && ((cat->fc_status & 0x40)==0)) {	/* Fastcat not halted */
	cat->fc_halt_req = 0x00008000;	/* partial state restore */
	for (i=1; i<=1000; i++) {
	    if (cat->fc_halt_ack == 0)
		break;
	    snooze(1000);			/* wait 1ms */
	}
	if (cat->fc_halt_ack != 0) {		/* did it restart? */
	    cat->fc_status = 0x000000c0;	/* halt/reset it */
	    msg_printf("bobcat_cursor: 98556 timeout; halted by kernel\n");
	}
    }
}

bobcat_clear(ite, y, x, count)					    /* ENTRY */
    register struct iterminal *ite;
    register y, count;
{
    register struct cat *cat = (struct cat *)(ite->card);
    register short height, width;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    bobcat_bmove_wait(ite);

    /* enable background color for write */
    cat->tcwen = (ite->color_pairs[0].BG & ite->cur_planes) << 8;	
    cat->wrr = 15;			/* replacement rule = set */

    /* enable other planes for clear */
    cat->tcwen = (~ite->color_pairs[0].BG & ite->cur_planes) << 8;	
    cat->wrr = 0;			/* replacement rule = clear */

    cat->tcwen = 0xff00;		/* enable all planes for block move */

    /*
     * The area to be cleared can be viewed as follows:
     *
     *	           aaaaaaaaaaaaaaaaaaaaaaa
     *	bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
     *	bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
     *	bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
     *	cccccc
     *
     * We treat this as three rectangular regions: a, b, & c.
     * If we are lucky, some of them are empty.
     * For the full-screen case, only region b exists.
     */

    /* Region a */
    if (x>0) {
	width = scr_min(ite->screenwidth-x, count);
	do_bobcat_clear(ite, x, y, width, 1);
	y++;
	count -= width;
    }

    /* Region b */
    height = count/ite->screenwidth;
    if (height>0) {
	do_bobcat_clear(ite, 0, y, ite->screenwidth, height);
	y+=height;
	count-=height*ite->screenwidth;
    }

    /* Region c */
    if (count>0)
	do_bobcat_clear(ite, 0, y, count, 1);
}

do_bobcat_clear(ite, x, y, width, height)			    /* ENTRY */
    register struct iterminal *ite;
    register short x, y, width, height;
{
    register struct cat *cat = (struct cat *)(ite->card);

    if (width<=0 || height<=0)
	return;

    x *= ite->font_width;
    y *= ite->line_height;
    width *= ite->font_width;
    height *= ite->line_height;

    bobcat_bmove_wait(ite); 

    /* set source and dest the same -- just in case of noise?? */
    cat->sox = x;
    cat->dox = x;
    cat->soy = y;
    cat->doy = y;
    cat->wwidth = width;
    cat->wheight = height;

    /* clear just available planes */
    cat->wmove = ite->cur_planes<<8;
}

bobcat_scroll(ite, to_row, dir, width)				    /* ENTRY */
    register struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);
    register short dest_y;

    /* calculate width of line to move */
    if ((width *= ite->font_width) == 0)
	return;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, to_row);
	return;
    }
    dest_y = to_row * ite->line_height;

    bobcat_bmove_wait(ite);
    cat->dox = 0;			/* set dest x pixel number */
    cat->doy = dest_y;		/* set dest y pixel number */
    cat->sox = 0;			/* source x pixel is same as dest */

    /* source is one line behind or one line ahead */
	cat->soy = dest_y + 
    	((dir == UP) ? ite->line_height : -ite->line_height);

    cat->wwidth = width;		/* width of window to move */
    cat->wheight = ite->font_height;	/* height of window to move */
    cat->tcwen = 0xff00;		/* enable all planes */
    cat->wrr = 3;			/* replacement rule = move */
    cat->wmove = ite->cur_planes<<8;	/* move just available planes */
}

/*
 * Set up the hardware TCWEN, FBEN & PRR before writing out pixels.
 */
bobcat_write(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);

    bobcat_bmove_wait(ite);
    cat->tcwen = 0xff00;		/* enable all planes for block move */
    cat->fben = ite->cur_planes<<8;	/* enable planes for frame buffer */
    cat->prr = 0x0300;			/* set pixel replacement rule */ 
}

bobcat_write_off(ite, vlp, y, x, count)				    /* ENTRY */
    register struct iterminal *ite;
    register SCROLL *vlp; 
    register x, count;
{
    register struct cat *cat = (struct cat *)(ite->card);
    register color_pair, scrollvalue, dest_x;
    register plane_mask = ite->cur_planes<<8;
    int back_c, fore_c;
    struct offscreen_font offscreen_xy;
    unsigned short dest_y;

    /* check if anything to do */
    if (count <= 0)
	return;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }
    dest_x = ite->font_width * x;  /* get dest x pixel number */
    dest_y = ite->line_height * y; /* get dest y pixel number */

    /* set up the hardware TCWEN, FBEN & PRR before writing out pixels */
    bobcat_write(ite);

    cat->wwidth = ite->font_width;   /* width of window to move */
    cat->wheight = ite->font_height; /* height of window to move */
    cat->doy = dest_y;		 /* set dest y pixel number */

    /* force calculation of current colors on first entry */
    color_pair = ITE_BOGUS_CPAIR;

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
	    bobcat_bmove_wait(ite);

	    /*        rule    case 1  case 2  case 3  case 4 */
	    /*    FG =		0	1	0	1    */
	    /*    BG =		0	0	1	1    */
	    /*	              _____   _____   _____   _____  */
	    /* ~(FG | BG)   0	1       0       0       0    */
	    /* ~BG & FG	    3	0	1	0	0    */
	    /* ~FG & BG    12	0	0	1	0    */
	    /*  FG & BG    15	0	0	0	1    */

	    /* case 1 */
	    cat->tcwen = ~(back_c | fore_c) & plane_mask;
	    cat->wrr = 0; /* replacement rule = clear */

	    /* case 2 */
	    cat->tcwen = ~back_c & fore_c & plane_mask;
	    cat->wrr = 3; /* replacement rule = move */

	    /* case 3 */
	    cat->tcwen = ~fore_c & back_c & plane_mask;
	    cat->wrr = 12; /* replacement rule = complement move */

	    /* case 4 */
	    cat->tcwen = back_c & fore_c & plane_mask;
	    cat->wrr = 15; /* replacement rule = set */

	    cat->tcwen = 0xff00; /* enable all planes */
	}
	else bobcat_bmove_wait(ite);

	/* source x,y pixels depend on char in offscreen memory */
	cat->sox = offscreen_xy.X;
	cat->soy = offscreen_xy.Y;

	cat->dox = dest_x;		/* dest pixel is next char in line */

	cat->wmove = plane_mask;	/* move just available planes */
	dest_x += ite->font_width;
	x++;
    } while (--count);
}

/*
 * Turn on/off alpha and graphics.
 */
bobcat_set_st(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);

    switch (ite->flags & (GRAPHICSON | ALPHAON)) {
	default: /* black */
		/* cannot allow because of NERIED/TOPCAT quirk */
		ite->flags |= ALPHAON;
		/* fall thru */

	case ALPHAON: /* alpha on , graphics off */
		/* select alpha frames for video output */
		cat->nblank = ite->cur_planes<<8; 
		break;

	case GRAPHICSON | ALPHAON: /* graphics on, alpha on */
		/* select all frames for video output */
		cat->nblank = 0xff00; 
		break;

	case GRAPHICSON: /* graphics on, alpha off */
		/* select non-alpha frames for video output */
		cat->nblank = ~(ite->cur_planes<<8)&0xff00; 
		break;
    }
}

bobcat_init(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);
    register i;

    bobcat_bmove_wait(ite);
    cat->tcren = 0x100;		/* enable plane 0 for register read */
    cat->curon = 0x0;		/* turn off cursor in case it was left on */
    cat->altframe = 0x0;	/* select the zero frame for video output */
    bobcat_set_st(ite);		/* get graphics/alpha on/off right */
    cat->blink = 0x0;		/* unblink all planes */

    /* skip setting color maps if no color */
    if (ite->flags & ITE_COLOR) {
	/* set up 1st 8 color maps maps */

	/* init color maps 0 - 7 */
	for (i = 0; i < 8; i++) {
	    /* skip maps that are not currently allowed */
	    if ((i & ite->cur_planes) != i)
		continue;

	    /* wait clear cmap bit */
	    while (cat->cmap_busy)
		NERIED_WAIT;
		
	    /* address next color map reg (complemented) */
	    cat->ioab = 255 - i; NERIED_WAIT;

	    /* set the RGB regs */
	    cat->rdata = bitm_colormap[i][0]; NERIED_WAIT;
	    cat->gdata = bitm_colormap[i][1]; NERIED_WAIT;
	    cat->bdata = bitm_colormap[i][2]; NERIED_WAIT;
	
	    /* do the write trigger */
	    cat->writeb = 0; NERIED_WAIT;
	    /* caution!-there must be 250 ns before NERIED cmap busy test */
	}

	/* wait clear cmap bit */
	while (cat->cmap_busy)
	    NERIED_WAIT;

	/* clear the maps without trigger to avoid noise ?? */ 
	cat->rdata = 0; NERIED_WAIT;
	cat->gdata = 0; NERIED_WAIT;
	cat->bdata = 0; NERIED_WAIT;
    }

    /* now restore offscreen memory from in ram font */
    (*ite->crt_font_restore)(ite);
}

/*
 * clear all frame buffers
 */
bobcat_pwr_reset(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    register struct cat *cat = (struct cat *)(ite->card);

    if (ite->flags & CATSEYE)
	idrom_init(ite->card);

    bobcat_bmove_wait(ite);

    cat->wrr = 0;		/* replacement rule = clear */
    cat->tcwen = 0xff00;	/* enable all planes for block move */
    cat->wheight = max(ite->framebuf_height, 0x200); /* set window height */

    /* set source and dest the same -- just in case of noise?? */
    cat->sox = 0;
    cat->dox = 0;
    cat->soy = 0;
    cat->doy = 0;
    cat->wwidth = 0x400;

    cat->wmove = 0xff00;			/* clear all planes */

    bobcat_bmove_wait(ite);

    /* if NERIED, then video enable all planes */
    if (ite->flags & ITE_COLOR) {
 	/* enable alpha video color planes for current display planes */
	cat->maskb = 0xff; NERIED_WAIT;
    }

    /* Make sure our structure definition is correct. */
    if ((int) &((struct cat *) 0)->fc_halt_ack != 0x80004)
	panic("struct cat messed up");
}
