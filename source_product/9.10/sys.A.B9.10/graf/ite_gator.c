/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_gator.c,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:59:08 $
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
#include "../graf.300/ite_gator.h"

#define frame_offset(ite, x, y) (((y) * ite->line_height * ite->framebuf_width)\
				+ ((x) * ite->font_width))

short gatoraid_microcode[GATORAID_UCODE_LEN] = {
	0x4efe, 0x8a3c, 0x0000, 0x0000, 0x0000, 0, 0, 0,
	0x6efe, 0x8a3c, 0x0004, 0x40f7, 0xa006, 0, 0, 0,
	0x43fe, 0x8a3c, 0x0000, 0x0000, 0x010c, 0, 0, 0,
	0x4efe, 0x8a3c, 0x0000, 0x0000, 0x0000
};

gatorbox_cursor(ite, x, y, color_num)				    /* ENTRY */
    struct iterminal *ite;
{
    register char *diostart = (char *)DIO_START;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }

    while((*(diostart+0x0002) & 0x10))
	/* wait for block mover hardware */;

    *(diostart+0x0003) = 0x04;			      /* enable frame buffer */
    *(diostart+0x6009) = ~ite->cur_planes;   /* enable just available planes */
    *(diostart+0x5007) = 6;			 /* set replacement rule XOR */

    bitmap_cursor(ite, x, y, color_num);

    *(diostart+0x0003) = 0x00;			     /* disable frame buffer */
}

gatorbox_clear(ite, y, x, count)				    /* ENTRY */
    register struct iterminal *ite;
    register y, count;
{
    register unsigned char *frame_mem;
    register char *diostart = (char *)DIO_START;
    register len, tile_len, bg_cmap_color;

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, x, y);
	return;
    }
    len = scr_min(ite->screenwidth - x, count);

    while((*(diostart+0x0002) & 0x10))
	/* wait for block mover hardware */;

    *(diostart+0x0003) = 0x04;			      /* enable frame buffer */
    *(diostart+0x5003) = -(ite->line_height>>2);  /* set the height register */

    while (count) {
	frame_mem = ite->frame_start + frame_offset(ite, x, y);
	x = 0;
	y++;
	count -= len;
	tile_len = -len * (ite->font_width>>2);
	len = scr_min(ite->screenwidth, count);

	while((*(diostart+0x0002) & 0x10))
	    /* wait for block mover */;

	*(diostart+0x5001) = tile_len;			  /* set length line */

	/* check if background planes need setting */
	if (bg_cmap_color = ite->color_pairs[0].BG & ite->cur_planes) {
 	    /* set replacement mode set */
	    *(diostart+0x5007) = 0xc0 + 15;

 	    /* protect all but planes but thoes to set */
	    *(diostart+0x6009) = ~bg_cmap_color;

	    *frame_mem = 0;		   /* tell block mover to munge line */

	    while((*(diostart+0x0002) & 0x10))
		/* wait for block mover */;
	}

	/* check if planes need clearing */
	if (bg_cmap_color = ~ite->color_pairs[0].BG & ite->cur_planes) {
	    *(diostart+0x5007) = 0xc0 + 0;	   /* replacement mode=clear */

 	    /* protect all but planes but thoes to clear */
	    *(diostart+0x6009) = ~bg_cmap_color;

	    *frame_mem = 0;		   /* tell block mover to color line */
	}
    }
}

gatorbox_scroll(ite, to_row, dir, width)			    /* ENTRY */
    struct iterminal *ite;
{
    register unsigned char *frame_mem = ite->frame_start + 
		frame_offset(ite, 0, to_row);
/*****		(to_row * ite->font_line);  *****/
    register char *diostart = (char *)DIO_START;
    register font_line = frame_offset(ite, 0, 1);

    /* calculate the width of the scroll line */
    if ((width = -((width * ite->font_width) >> 2)) == 0)
	return; 

    /* check if access to bit mapped screen hardware allowed */
    if (check_screen_access(ite)) {
	set_min_xy(ite, 0, to_row);
	return;
    }

    /* determine the direction of the scroll */
    font_line = (dir ==UP) ? font_line : -font_line;


    while((*(diostart+0x0002) & 0x10))
	/* wait for block mover hardware */;

    *(diostart+0x0003) = 0x04;			      /* enable frame buffer */
    *(diostart+0x6009) = ~ite->cur_planes;   /* enable just available planes */
    *(diostart+0x5007) = 0xc0 + 3;   /* set replacement rule for block mover */
    *(diostart+0x5001) = width;				    /* set width reg */

    /* set the height register (must be multiple of 4) */
    *(diostart+0x5003) = -(ite->font_height >> 2);

    *frame_mem = *(frame_mem + font_line);		/* block move a line */
}

gatorbox_write(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register char *diostart = (char *)DIO_START;

    while((*(diostart+0x0002) & 0x10))
	/* wait for block mover hardware */;

    *(diostart+0x0003) = 0x04;			      /* enable frame buffer */
    *(diostart+0x6009) = ~ite->cur_planes;   /* enable just available planes */
    *(diostart+0x5007) = 3;			       /* normal replacement */
}

gatorbox_init(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    register char *diostart_a5 = (char *)DIO_START;		/* a5 */
    register char *secintreg_a4 = diostart_a5+0x2; 		/* a4 */
    register short *color_map_reg_a3;				/* a3 */
    register short map_value_d7; 				/* d7 */
    register i, j;
    char cmap_id = *(diostart_a5+0x600d);

    while((*(diostart_a5+0x0002) & 0x10))
	/* wait for block mover hardware */;

    *secintreg_a4 = 0x01;		   /* enable video output to monitor */

    if (cmap_id == 0x56) {
	/* clear NEREID test color registers */
	*((short*)diostart_a5+(0x699c/2)) = 0;
	*((short*)diostart_a5+(0x69ac/2)) = 0;

	/* enable all color planes for write */
	*((short *)diostart_a5+(0x68ba/2)) = 0x00ff;
    }

    for (i=0 ; i<ITE_MAX_CPAIRS; i++) { 
	/* skip maps that are not currently allowed */
	if ((i & ite->cur_planes) != i)
	    continue;

	while (*(diostart_a5+0x6803) & 0x4)
	    /* wait for busy cmap bit */;
		
	*(diostart_a5+0x68b9) = i;	       /* address next color map reg */

	/* set up a3 */
	color_map_reg_a3 = (short *)(diostart_a5+0x69b2); 
		
	/* set the RGB regs */
	for (j = 0; j < 3; j++) {
	    map_value_d7 = bitm_colormap[i][j];

asm("		movq	&1,%d0		 "); /* test bit # 1 */
asm("gatbox1:	btst	%d0,(%a4)		 "); /* wait for blank off */
asm("		beq.b	gatbox1 	 "); /* ready ? */
asm("gatbox2:	btst	%d0,(%a4)		 "); /* wait for blank on */
asm("		bne.b	gatbox2 	 "); /* ready ? */
/* in asm because of the 5 microsecond window to execute this instruction */
asm("		mov.w	%d7,(%a3)+	 "); /* set the color map reg */
	}

	/* do the write trigger for NERIED (nop for gatorbox) */
	*(diostart_a5+0x68f1) = 0; 
	/* CAUTION! - there must be 250 ns before NERIED cmap busy test */
    }

    /* clear the maps in NERIED without trigger to avoid noise ?? */ 
    if (cmap_id == 0x56) {
	while (*(diostart_a5+0x6803) & 0x4)
	    /* wait for busy cmap bit */;

	/* set up color map register address */
	color_map_reg_a3 = (short *)(diostart_a5+0x69b2); 

	/* clear RGB registers for NERIED without trigger */
	for (j = 0; j < 3; j++) 
	    *color_map_reg_a3++ = 0;
    }

    *(diostart_a5+0x0003) = 0x04;		      /* enable frame buffer */

    /* enable blink A&B to just available planes ?? */
    *(diostart_a5+0x6001) = ite->cur_planes; 
    *(diostart_a5+0x6005) = ite->cur_planes;

    /* if gatoraid present and busy do a reset!! */
    if ((ite->flags & GATORAID) && (*(diostart_a5+0x8004) < 0)) {
	*(diostart_a5+0x8000) = 0xa0;
	*(diostart_a5+0x8000) = 0x20;
	*(diostart_a5+0x8000) = 0x00;

	while (*(diostart_a5+0x8004) < 0)
	    /* wait for gatoraid non busy */;
    }
}
