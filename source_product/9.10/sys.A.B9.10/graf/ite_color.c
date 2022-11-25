/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/graf/RCS/ite_color.c,v $
 * $Revision: 1.7.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 15:25:35 $
 */

#include "../graf/ite.h"
#include "../graf/ite_color.h"

#ifdef lint
    struct dux_mbuf { int foo; };
#endif

#ifdef __hp9000s300
    color_pairs_t init_colors[ITE_MAX_CPAIRS] = {
	FWHITE,		FBLACK,	/* [0] White on Black */
	FRED,		FBLACK,	/* [1] Red on Black */
	FGREEN,		FBLACK,	/* [2] Green on Black */
	FYELLOW,	FBLACK,	/* [3] Yellow on Black */
	FBLUE,		FBLACK,	/* [4] Blue on Black */
	FMAGENTA,	FBLACK,	/* [5] Magenta on Black */
	FCYAN,		FBLACK,	/* [6] Cyan on Black */
	FBLACK,		FYELLOW	/* [7] Black on Yellow */
    };
#endif

char bitm_colormap[ITE_MAX_CPAIRS][3] = {	
	000, 000, 000, /* 0 black   */ 
	255, 255, 255, /* 1 white   */ 
	255, 000, 000, /* 2 red     */ 
	255, 255, 000, /* 3 yellow  */
	000, 255, 000, /* 4 green   */
	000, 255, 255, /* 5 cyan    */
	000, 000, 255, /* 6 blue    */
	255, 000, 255  /* 7 magenta */
};

/*
 * Name: convert_to_HSL
 *
 * Description:
 *     This routine uses Hue, Saturation, and Luminosity to determine
 *     what color the user wants.
 */
convert_to_HSL(a, b, c)						    /* ENTRY */
{
    if (c<25) return(ITE_BLACK);
    if (b<25) return(ITE_WHITE);
    if (a<9)  return(ITE_RED);
    if (a<25) return(ITE_YELLOW);
    if (a<42) return(ITE_GREEN);
    if (a<59) return(ITE_CYAN);
    if (a<75) return(ITE_BLUE);
    if (a<92) return(ITE_MAGENTA);
    return(ITE_RED);
}

/*
 * Name: convert_to_RGB
 *
 * Description:
 *      This routine uses Red, Green and Blue values to determine what
 *      color the user wants.
 */
convert_to_RGB(a, b, c)						    /* ENTRY */
{
    int n = imax(a,imax(b,c))>>1;

    if ((a<25) || (a<n)) {
	/* Must be BLACK, GREEN, CYAN, or BLUE */
	if ((b<25) || (b<n)) {
	    /* Must be BLACK or BLUE */
	    if ((c<25) || (c<n)) return(ITE_BLACK);
	    return(ITE_BLUE);
	}
	/* Must be GREEN or CYAN */
	if ((c<25) || (c<n)) return(ITE_GREEN);
	return(ITE_CYAN);
    }
    /* Must be WHITE, YELLOW, MAGENTA, or RED */
    if ((b<25) || (b<n)) {
	/* Must be MAGENTA or RED */
	if ((c<25) || (c<n)) return(ITE_RED);
	return(ITE_MAGENTA);
    }
    /* Must be WHITE or YELLOW */
    if ((c<25) || (c<n))
	return(ITE_YELLOW);
    return(ITE_WHITE);
}

#ifdef __hp9000s800
/*
 * This function resets mono or color pairs back to their defaults.
 * This is done at system initialization time, and at hard reset.
 */
void
reset_color_pairs(ite)						    /* ENTRY */
    struct iterminal *ite;
{
    if (ite->flags & ITE_COLOR) {
        ite->color_pairs[0].FG = ITE_WHITE;
        ite->color_pairs[0].BG = ITE_BLACK;
        ite->color_pairs[1].FG = ITE_RED;
        ite->color_pairs[1].BG = ITE_BLACK;
        ite->color_pairs[2].FG = ITE_GREEN;
        ite->color_pairs[2].BG = ITE_BLACK;
        ite->color_pairs[3].FG = ITE_YELLOW;
        ite->color_pairs[3].BG = ITE_BLACK;
        ite->color_pairs[4].FG = ITE_BLUE;
        ite->color_pairs[4].BG = ITE_BLACK;
        ite->color_pairs[5].FG = ITE_MAGENTA;
        ite->color_pairs[5].BG = ITE_BLACK;
        ite->color_pairs[6].FG = ITE_CYAN;
        ite->color_pairs[6].BG = ITE_BLACK;
        ite->color_pairs[7].FG = ITE_BLACK;
        ite->color_pairs[7].BG = ITE_YELLOW;
    }
    else {
	int i;

        for (i=0; i < ITE_MAX_CPAIRS; i++) {
            ite->color_pairs[i].FG = ITE_WHITE;
            ite->color_pairs[i].BG = ITE_BLACK;
        }
        /* last entry is inverse */
        ite->color_pairs[ITE_SOFTKEYS_CPAIR].FG = ITE_BLACK;
        ite->color_pairs[ITE_SOFTKEYS_CPAIR].BG = ITE_WHITE;
    }
}
#endif

/*
 * Change the value (contents) of a color pair.
 */
void
set_color_pair(ite, color_pair)					    /* ENTRY */
    register struct iterminal *ite;
{
    register fg_color, bg_color;

    if ((color_pair<0) || (color_pair>=ITE_MAX_CPAIRS))
        return;

    /* Compute character and background colors */ 
    if (ite->c_mode == HSL) { 
	/* Use HSL color model */ 
	fg_color = convert_to_HSL(ite->color_a, ite->color_b, ite->color_c);
	bg_color = convert_to_HSL(ite->color_x, ite->color_y, ite->color_z);
    }
    else {
	/* Use RGB color model */
	fg_color = convert_to_RGB(ite->color_a, ite->color_b, ite->color_c);
	bg_color = convert_to_RGB(ite->color_x, ite->color_y, ite->color_z);
    }

#   ifdef __hp9000s300
	if (!(ite->flags & ITE_COLOR)) {
	    if (fg_color != FBLACK)
		fg_color = FWHITE;
	    if (bg_color != FBLACK)
		bg_color = FWHITE;
	}

	fg_color |= fg_color<<8;
	fg_color |= fg_color<<16;
	bg_color |= bg_color<<8;
	bg_color |= bg_color<<16;
	ite->color_pairs[color_pair].FG = fg_color;
	ite->color_pairs[color_pair].BG = bg_color;
#   endif

#   ifdef __hp9000s800
	if (!(ite->flags & ITE_COLOR)) {
	    if (((fg_color != ITE_BLACK) && (fg_color != ITE_WHITE)) || 
	        ((bg_color != ITE_BLACK) && (bg_color != ITE_WHITE)))
		/* There is only black and white, so cannot change to colors */
		return;
	}

	ite->color_pairs[color_pair].FG = fg_color;
	ite->color_pairs[color_pair].BG = bg_color;
#   endif

    /* force to impossible value to force table build */
    ite->current_cpair = ITE_BOGUS_CPAIR;

    ite->redraw = 1;
}
