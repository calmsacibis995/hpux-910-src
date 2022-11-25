/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/ite_alpha.c,v $
 * $Revision: 1.5.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 18:59:45 $
 */
/* HPUX_ID: @(#)alpha.c	55.1		88/12/23 */

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
#include "../s200io/bootrom.h"
#include "../graf/ite.h"
#include "../graf/ite_scroll.h"
#include "../mach.300/cpu.h"

extern char sysflg;

/*--------------------------------------------*\
|	2622/9836 enhancement types and codes  |
+----------------------------------------------+
| half-bright  | | | | | | | | |x|x|x|x|x|x|x|x|
| underline    | | | | |x|x|x|x| | | | |x|x|x|x|
| inverse video| | |x|x| | |x|x| | |x|x| | |x|x|
| blinking     | |x| |x| |x| |x| |x| |x| |x| |x|
| end enhance  |x| | | | | | | | | | | | | | | |
+--------------+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| 2622 code    |@|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|
| 9836 code    |0|2|1|3|4|6|5|7|8|A|9|B|C|E|D|F|
\*--------------------------------------------*/

/* THE ALPHA PLANE DRIVER FOR 9826 - 9836 TYPE DISPLAYS */

/* defines for the 9836 enhancement codes */
#define	N_HL	(0x00 << 8)	/* normal                         */
#define I_HL	(0x01 << 8)	/* inverse video                  */
#define H_HL	(0x08 << 8)	/* halfbright                     */
#define U_HL	(0x04 << 8)	/* underline                      */
#define B_HL	(0x02 << 8)	/* blinking                       */
#define RED_HL	(0x10 << 8)	/* Red Gun On			  */
#define NGREEN_HL (0x20 << 8)	/* Green Gun Off		  */
#define	BLUE_HL	(0x40 << 8)	/* Blue Gun On			  */

/* CRT io register addresses */
#define CRTBASE  (0x510000+LOG_IO_OFFSET)	/* start of CRT registers */
#define CRTRPR   1       	/* CRT register pointer offset   */
#define CRTCMD	 3       	/* CRT command register offset   */
 
/* CRT display memory addresses */
#define CRTSFK_26 (0x512704-0x64+LOG_IO_OFFSET)	/* start of CRT display memory
				** with 2 rows of sfk labels     */
#define CRTSFK_36 (0x5121A0-0xA0+LOG_IO_OFFSET)	/* start of CRT display memory
				** with 2 rows of sfk labels     */
/* CRT cursor offsets */
/* #define	CRTCUR_26	(0x3381-0x32)	/* start of 9826 cursor */
#define	CRTCUR_36	(0x30CF-0x50)	/* start of 9836 cursor */

/* CRT command register defines */
#define CRTSTRTH 0x0C		/* high byte of crt display address */
#define CRTSTRTL 0x0D		/* high byte of crt display address */
#define HICURSOR 0x0E		/* high byte of cursor address */
#define LOCURSOR 0x0F		/* low byte of cursor address  */

int alpha_colormap[8] = {
    NGREEN_HL,			/* Black = 0 */
    RED_HL+BLUE_HL,		/* White = 1 */
    RED_HL+NGREEN_HL,		/* Red = 2 */
    RED_HL,			/* Yellow = 3 */
    0,				/* Green = 4 */
    BLUE_HL,			/* Cyan = 5 */
    BLUE_HL+NGREEN_HL,		/* Blue = 6 */
    RED_HL+NGREEN_HL+BLUE_HL,	/* Magenta = 7 */
};
#define xy_to_crt(x, y) ((short *)ite->frame_start + (ite->screenwidth * (y) + (x)))

alpha_command(reg, data)					    /* ENTRY */
    char reg, data;
{
    register char *crt = (char *)CRTBASE;

    crt[CRTRPR] = reg;	/* indicate which register */
    crt[CRTCMD] = data;	/* give it the data        */
}

alpha_set_st(ite)						    /* ENTRY */
    register struct iterminal *ite;
{
    register char mode;
    union {
	short	location;
	char	byte[2];
    } crt_addr;

    /* call graphics driver to set graphics on/off mode */
    alpha_graphics(ite->flags & GRAPHICSON);

    switch (ite->flags & (ALPHAON | SFK_ON)) {
	default:
	case ALPHAON | SFK_ON:
	case ALPHAON:
		mode = 0x30;
		break;
	case SFK_ON:
		mode = 0x10;
		break;
	case 0:
		mode = 0x00;
		break;
    }

    crt_addr.location = ((unsigned)ite->frame_start % 8192) >> 1;
    alpha_command(CRTSTRTH, mode);
    alpha_command(CRTSTRTL, crt_addr.byte[1]);
}

alpha_cursor(ite, x, y)						    /* ENTRY */
    register struct iterminal *ite;
{
    register char *crt = (char *) CRTBASE;
    union {
	short	location;
	char	byte[2];
    } cursor;

    cursor.location = CRTCUR_36+1 + (ite->screenwidth*y + x);
    crt[CRTRPR] = HICURSOR; crt[CRTCMD] = cursor.byte[0];
    crt[CRTRPR] = LOCURSOR; crt[CRTCMD] = cursor.byte[1];
}

alpha_clear(ite, ypos, xpos, count)				    /* ENTRY */
    struct iterminal *ite;
    register count;
{
    register short c, *start = xy_to_crt(xpos, ypos);
    register color;
	
    if (count <= 0)
	return;

    /* get the foreground color of pair zero */
    color = ite->color_pairs[0].FG & 7;

    /* clear in the foreground color for the cursor's sake */
    c = ' ' | alpha_colormap[color];

    /* if foreground color 0 is black then use background color */
    if (color == ITE_BLACK) {
	/* use the background color of pair zero */
	color = ite->color_pairs[0].BG & 7;
	c = ' ' | alpha_colormap[color];
	/* if background not black, invert it */
	if (color != ITE_BLACK)
	    c |= I_HL;
    }

    /* if 9826 or old video card, then use special clear character */
    if (!(sysflg & HIGHLIGHTS)) {
	/* and in the label area */
	if (ypos > ite->maxy)
	    c = 0x00fb;				/* clear is all white box */
    }

    do {
	*start++ = c;
    } while (--count); 
}

alpha_scroll(row, dir)						    /* ENTRY */
    register row;
{
    register struct iterminal *ite = &iterminal;
    register short *source, *dest;
    register int count;

    count = ite->screenwidth * (ite->screenheight-row-1);

    if (dir == UP) {
	dest = xy_to_crt(0, row);
	source = dest + ite->screenwidth;
	bcopy(source, dest, count<<1);
    }
    else {
	source = xy_to_crt(0, row);
	dest = source + ite->screenwidth;
	/* copy backward */
	source+=count; dest+=count;
	while (--count>=0)
	    *--dest = *--source;
    }
}

alpha_write(ite, vlp, y, x, count)				    /* ENTRY */
    struct iterminal *ite;
    register SCROLL *vlp;				/* a5 */
    register count;					/* d7 */
{
    register short *dest = xy_to_crt(x, y);		/* a4 */
    register unsigned int scrollvalue;			/* d6 */
    register cur_color;					/* d5 */
    register color_pair = ITE_BOGUS_CPAIR;		/* d4 */
    register color;					/* d3 */

    while (count--) {
	/* check if color or enhancements changed from last char? */
	if (color_pair != char_enhance(scrollvalue = *vlp++)) {

	    /* skip all not enabled chars */
	    if ((scrollvalue & CHAR_ON) == 0)
		return;
			
	    /* get color pair number */
	    color_pair = char_colorpair(scrollvalue);

	    /* get the current foreground color from number */
	    color = ite->color_pairs[color_pair].FG & 7;

	    /* if foreground black then use background inverted */
	    if (color == ITE_BLACK) {
		/* get the background color */
		color = ite->color_pairs[color_pair].BG & 7; 
		cur_color = alpha_colormap[color];

		/* if background not black, invert it */
		if (color != ITE_BLACK)
		    cur_color |= I_HL;
	    }
	    else cur_color = alpha_colormap[color];

	    /* now add enhancements to word */
	    if (scrollvalue & HALF_BRIGHT)	 cur_color |= H_HL;
	    if (scrollvalue & UNDERLINE)	 cur_color |= U_HL;
	    if (scrollvalue & BLINKING) 	 cur_color |= B_HL;
	    if (scrollvalue & INVERSE_VIDEO) cur_color ^= I_HL;

	    /* get current color and enhancements bits */
	    color_pair = char_enhance(scrollvalue);
	}
    *dest++ = ite->char_out[scrollvalue&CHAR_VAL] | cur_color;
    }
}

/*
 * Do we have an alpha display?
 *
 * Consider location 51fffe.  It will always DTACK.
 * If it is a ROM location, we have an alpha display.
 * If it is a floating value, we do not have an alpha display.
 */
we_have_alpha()							    /* ENTRY */
{
    unsigned char *test_loc = (unsigned char *) (0x51fffe + LOG_IO_OFFSET);
    unsigned char value;

    /* Location must be read/write */
    if (!testr(test_loc,1) || !testw(test_loc,1))
	return(FALSE);

    *test_loc = 0x5a;
    if (*test_loc != 0x5a)				 /* Different stuff? */
	goto good_so_far;

    /*
     * So it was equal.  That might just have been what was there in ROM.
     */
    *test_loc = 0xa5;
    if (*test_loc == 0xa5)	/* Oops, two equalities -- must be floating. */
	return(FALSE);

good_so_far:
    value = *test_loc;
    *test_loc = value+1;				/* Write old value+1 */
    if (*test_loc == value+1)			       /* Is it still there? */
	return(FALSE);				  /* must be floating bus... */
    return(TRUE);
}
