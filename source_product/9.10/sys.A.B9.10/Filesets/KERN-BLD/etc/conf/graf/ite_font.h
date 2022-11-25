/*
 * @(#)ite_font.h: $Revision: 1.6.83.4 $ $Date: 93/12/06 17:44:54 $
 * $Locker:  $
 */

#ifndef ITE_FONT_INCLUDED
#define ITE_FONT_INCLUDED

/* Number of glyphs/characters that make up a given font */
#define ITE_NGLYPHS	256

/*
 * ITE font type definitions
 */
#define FONT_REGULAR		0
#define FONT_INVERSE		1
#define FONT_TOTAL		2

/*
 * ITE character set definitions
 */
#define CHARSET_UNDEFINED	-1
#define CHARSET_DEFAULT8	0
#define	CHARSET_ROMAN8		1	/* base character set      */
#define CHARSET_GREEK8		2	/* base character set      */
#define CHARSET_TURKISH8	3	/* base character set      */
#define	CHARSET_KANA8		4	/* base character set      */
#define	CHARSET_LINE8		5	/* alternate character set */
#define	CHARSET_MATH8		6	/* alternate character set */
#define CHARSET_TOTAL		7

#define CHARSET_BASE_MIN	CHARSET_ROMAN8
#define CHARSET_BASE_MAX	CHARSET_KANA8

#define CHARSET_ALT_MIN		CHARSET_LINE8
#define CHARSET_ALT_MAX		CHARSET_MATH8

/* Param to (*ite_font_shift)() */
#define ITE_FONT_SHIFT_IN	1
#define ITE_FONT_SHIFT_OUT	2

/* Param to (*ite_set_charset)() */
#define ITE_SET_BASE_CHARSET	1
#define ITE_SET_ALT_CHARSET	2

/*
 * NOTE:  Add new character set definitions here and be sure to
 * update actual font_table_init, CHARSET_TOTAL, CHARSET*MIN,
 * and CHARSET*MAX.
 */

/*
 * Possible future 8 bit character sets:
 *
 * #define CHARSET_HEBREW8
 * #define CHARSET_ARABIC8
 * #define CHARSET_KOREAN8
 * #define CHARSET_THAI
 */

/*
 * Macros for font table and character set information
 */
#define GOOD_BASE_CHARSET(charset)	((charset >= CHARSET_BASE_MIN) && \
					 (charset <= CHARSET_BASE_MAX))

#define GOOD_ALT_CHARSET(charset)	((charset >= CHARSET_ALT_MIN) && \
					 (charset <= CHARSET_ALT_MAX))

#define ACTIVE_CHARSET(ite)		(ite->active_charset)
#define BASE_CHARSET(ite)		(ite->base_charset)
#define ALTERNATE_CHARSET(ite)		(ite->alternate_charset)
#define DEFAULT_BASE_CHARSET(ite)	(ite->default_base_charset)
#define DEFAULT_ALTERNATE_CHARSET(ite)	(ite->default_alternate_charset)

struct font_def {
    unsigned char *font_buf;		/* pointer to current font char */
    int *pixel_colors_ptr;		/* pointer to the pixel color table */
#   ifdef __hp9000s300
	unsigned char *frame_mem;       /* pointer to current frame char */
	int     widthm1;                /* pixels per column of font - 1 */
	int     heightm1;               /* pixels per row of font - 1 */
#   endif
#   ifdef __hp9000s800
	int     *frame_mem;
	int     width;
	int     height;
#   endif
    int next_row_add;			/* frame_width - font_width */
};

struct font_rom_def {
    unsigned char	fill0, cell_height;	/* font height in pixels */
    unsigned char	fill1, cell_width;	/* font width in pixels	*/
    unsigned char	fill2, cell_depth;	/* font depth in pixels	*/
    unsigned char	fill3, start_char;	/* font start char number */
    unsigned char	fill4, end_char;	/* font end char number	*/
    short		font_buffer[1];		/* font	ROM characters	*/
};

typedef struct offscreen_font {
    unsigned short X, Y;
};

#ifdef _KERNEL
#   ifdef __hp9000s300
	extern struct offscreen_font offscreen_font[ITE_NGLYPHS];
	extern int ite_10by_draw(), ite_12by_draw(), ite_any_draw(),
		   ite_8modby_draw();
#   endif
#endif

#endif /* ITE_FONT_INCLUDED */
