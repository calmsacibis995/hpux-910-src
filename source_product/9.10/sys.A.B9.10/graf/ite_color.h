/*
 * @(#)ite_color.h: $Revision: 1.6.83.4 $ $Date: 93/12/09 15:25:01 $
 * $Locker:  $
 */

#ifndef __ITE_COLOR_H_INCLUDED
#define __ITE_COLOR_H_INCLUDED

#define ITE_MAX_CPAIRS		8
#define ITE_DEFAULT_CPAIR	0
#define ITE_SOFTKEYS_CPAIR	7
#define ITE_BOGUS_CPAIR		-1

#define	ITE_BLACK	0
#define	ITE_WHITE	1
#define	ITE_RED		2
#define	ITE_YELLOW	3
#define	ITE_GREEN	4
#define	ITE_CYAN	5
#define	ITE_BLUE	6
#define	ITE_MAGENTA	7

#define MAX_COLORS      8
#define ITE_FIRST_COLOR ITE_BLACK
#define ITE_LAST_COLOR  ITE_MAGENTA

#define	RGB	0
#define	HSL	1

#define	FBLACK	 (ITE_BLACK|ITE_BLACK<<8|ITE_BLACK<<16|ITE_BLACK<<24)
#define	FWHITE	 (ITE_WHITE|ITE_WHITE<<8|ITE_WHITE<<16|ITE_WHITE<<24)
#define	FRED	 (ITE_RED|ITE_RED<<8|ITE_RED<<16|ITE_RED<<24)
#define	FYELLOW	 (ITE_YELLOW|ITE_YELLOW<<8|ITE_YELLOW<<16|ITE_YELLOW<<24)
#define	FGREEN	 (ITE_GREEN|ITE_GREEN<<8|ITE_GREEN<<16|ITE_GREEN<<24)
#define	FCYAN	 (ITE_CYAN|ITE_CYAN<<8|ITE_CYAN<<16|ITE_CYAN<<24)
#define	FBLUE	 (ITE_BLUE|ITE_BLUE<<8|ITE_BLUE<<16|ITE_BLUE<<24)
#define	FMAGENTA (ITE_MAGENTA|ITE_MAGENTA<<8|ITE_MAGENTA<<16|ITE_MAGENTA<<24)

typedef struct {
    unsigned FG, BG;
} color_pairs_t;

#ifdef _KERNEL
    /* Routines supplied in ite_color.c */
    extern int convert_to_HSL(), convert_to_RGB();
    extern void reset_color_pairs(), set_color_pair();

    extern char bitm_colormap[ITE_MAX_CPAIRS][3];

#   ifdef __hp9000s300
	extern color_pairs_t init_colors[];
#   endif
#endif

#endif /* not __ITE_COLOR_H_INCLUDED */
