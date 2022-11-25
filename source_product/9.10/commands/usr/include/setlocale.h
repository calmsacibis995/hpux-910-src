/* @(#) $Revision: 70.3 $ */

#ifndef _SETLOCALE_INCLUDED /* allow multiple inclusions */
#define _SETLOCALE_INCLUDED

#ifdef _NAMESPACE_CLEAN
#define _1kanji __1kanji
#define _2kanji __2kanji
#define _downshift __downshift
#define _upshift __upshift
#endif /* _NAMESPACE_CLEAN */

#include	<locale.h>
#include	<limits.h>
#include	<nl_types.h>
#include	<langinfo.h>
#include	<collate.h>

#define		TRUE	1
#define		FALSE	0

struct _era_data {			/* defines an Emperor/Era time period */
	short start_year;		/* starting date of era */
	unsigned short start_month;
	unsigned short start_day;
	short end_year;			/* ending date of era */
	unsigned short end_month;
	unsigned short end_day;
	short origin_year;		/* time axis origin for era (one of start_year or end_year) */
	short offset;			/* offset from 0 for 1st year of era */
	short signflag;			/* adjusts sign of (year - origin_year) value */
	unsigned short reserved;
	unsigned char *name;		/* name of era */
	unsigned char *format;		/* instead of nl_langinfo(ERA_FMT) */
};

extern	int		__nl_langid[];	/* langid of currently loaded language	*/
extern	unsigned char	*__ctype;	/* pointer to ctype table	*/
extern	unsigned char	*__ctype2;	/* pointer to ctype2 table	*/
extern	unsigned char	*_1kanji;	/* pointer to 1st of 2 kanji table */
extern	unsigned char	*_2kanji;	/* pointer to 2nd of 2 kanji table */
extern	unsigned char	*_upshift;	/* pointer to up shift table */
extern	unsigned char	*_downshift;	/* pointer to down shift table */
#ifdef EUC
extern	unsigned char	*__e_cset;	/* pointer to expanded char set table */
extern	unsigned char	*__ein_csize;	/* pointer to expanded in_csize table */
extern	unsigned char	*__eout_csize;	/* pointer to expanded out_csize table*/
#endif /* EUC */
extern struct _era_data *_nl_era[];	/* array of era info str pointer */
extern	int		_nl_radix;	/* radix character */
extern	int		_sh_low;	/* lowest char in shift table domain */
extern	int		_sh_high;	/* highest char in shift table domain */
extern	int		__nl_char_size;	/* size of characters */
#ifdef EUC
extern	int		__nl_code_scheme;/* flag for char code scheme */
extern	int		__cs_SBYTE;	/* flag for 1 byte char code scheme */
extern	int		__cs_HP15;	/* flag for HP15 char code scheme */
extern	int		__cs_EUC;	/* flag for EUC char code scheme */
extern	unsigned char	__in_csize[];	/* input char size */
extern	unsigned char	__out_csize[];	/* output char size */
extern	unsigned int	__euc_template[]; /* euc process code template */
#endif /* EUC */
extern	nl_direct	_nl_direct;	/* direction flag */
extern	int		_nl_context;	/* directionality context flag */
extern  nl_order	_nl_order;	/* order flag */
extern  nl_mode		_nl_mode;	/* mode flag; Latin or non-Latin */
extern  nl_outdgt	_nl_outdigit;	/* digit output : ascii or alt digit */

extern	int		_nl_space_alt;	/* value of alternative space */
extern	unsigned char   *_nl_dgt_alt;	/* buffer for alt digit string */
extern	unsigned char	*_nl_punct_alt;	/* buffer for alt punctuation string */
extern  unsigned char	*_nl_pascii;	/* buffer for ascii punctuation string */
extern  unsigned char	*_nl_dascii;	/* buffer for ascii digits string */
extern	int		_nl_map21;	/* non-zero if 2-to-1 mappings */
extern	int		_nl_onlyseq;	/* true if only 1-to-1 char w no pri */
extern  int		_nl_collate_on;	/* true if collation table loaded */
extern  int		_nl_mb_collate;	/* true if collation is multibyte */

extern	unsigned char	 *_seqtab;	/* dictionary sequence number table */
extern	unsigned char	 *_pritab;	/* 1to2/2to1 flag + priority table */
extern	struct col_21tab *_tab21;	/* 2-to-1 mapping table	*/
extern	struct col_12tab *_tab12;	/* 1-to-2 mapping table */

extern	unsigned char	_sort_rules[COLL_WEIGHTS_MAX];	/* sort directives */

extern unsigned char	*__errptr;	/* pointer to an area _errlocale() can use as a buffer */

extern struct lconv	*_lconv;
extern unsigned char	*__category_name[];

extern unsigned char	**__nl_info;	/* pointers to locale langinfo strings */
extern unsigned char	*__C_langinfo[];/* default langinfo strings for the C locale */
#define _NL_MAX_MSG	_NL_ITEM_MAX	/* last nl_langinfo item (langinfo.h) */

/***************************************************************************

    The remainder of this file includes structures for the language files.
    The files are built by buildlang(1M).

    The structure of the files is as follows :

        ----------------------------------
	|  Table Header (A)	         |
	----------------------------------
	| Category/Modifier Structures(B)|
	==================================
	|  LC_ALL Table Header  (C)      |
	-   -   -   -   -   -   -   -    -
	|  LC_ALL Data			 |
	----------------------------------
	|  LC_COLLATE Table Header (D)   |
	-   -   -   -   -   -   -   -    -
	|  LC_COLLATE Data		 |
	----------------------------------
	|  LC_CTYPE  Table Header (E)	 |
	-   -   -   -   -   -   -   -    -
	|  LC_CTYPE  Data		 |
	----------------------------------
	|  LC_MONETARY Table Header (F)  |
	-   -   -   -   -   -   -   -    -
	|  LC_MONETARY  Data		 |
	----------------------------------
	|  LC_NUMERIC  Table Header (G)	 |
	-   -   -   -   -   -   -   -    -
	|  LC_NUMERIC  Data		 |
	----------------------------------
	|  LC_TIME  Table Header (H)	 |
	-   -   -   -   -   -   -   -    -
	|  LC_TIME  Data		 |
	----------------------------------
	|  LC_MESSAGES Table Header (I)  |
	-   -   -   -   -   -   -   -    -
	|  LC_MESSAGES  Data		 |
	----------------------------------

*****************************************************************************/
/* The order and types of these items below are highly sensitive
   to data alignement problems.  In some cases, data has been added
   to pad data structures so that the localedef generated locale.inf
   structures are the same on s300 and s800. */


/* Category Id's */


/* Table Header (A) */

struct table_header {
	unsigned int size;		/* size of table header and category
					   structure. (A) + (B)  */
	unsigned short nl_langid;	/* language name,
					   _nl_langid, msgindex[LANGID] */
	unsigned char lang[NL_LANGMAX+1];	/* msgindex[LANGNAME] */
	unsigned short cat_no;		/* number of categories defined */
	unsigned short mod_no;		/* number of modifiers defined */
	unsigned short rev_flag;	/* true if HP defined */
	unsigned char rev_str[36];	/* Revision String,
					   msgindex[REVISION] */
	unsigned short codeset;		/* 0 if 1 byte, 1 if 2 byte */
	unsigned char comment_char;	/* used in localedef/charmap,
					   msgindex[COMM_CHAR],not an address */
	unsigned char escape_char;	/* used in localedef/charmap,
					   msgindex[ESC_CHAR], not an address */
	unsigned char charmap[NL_LANGMAX+1];	/* name of charmap,
						   msgindex[CHARMAP] */
	unsigned short reserved1;
	unsigned short reserved2;	/* padding for locale.inf */
};

/* Category/Modifier Structure (B)

   Catinfotype structure describes a category/modifier table
   There is one structure for each category and modifier defined.
   These entries follow the table header */


struct catinfotype
{
	int size;				/* size of category table */
	int address;				/* address of category table -
						   offset from the beginning of
						   the category tables () */
	short catid;				/* category id */
	unsigned char mod_name[_POSIX_NAME_MAX+1];	/* name of modifier */
	short mod_addr;				/* address of category table
						   for modifier - offset from
						   beginning of file */
};


/* Below are the category headers for each of the defined categories
   All addresses are offset from the beginning of the category information */

/* LC_ALL Table  (C) */

struct all_header {
	unsigned short yes_addr;	/* msgindex[YESSTR] */
	unsigned short no_addr;		/* msgindex[NOSTR] */
	unsigned short direct_addr;	/* msgindex[DIRECTION] */
					/* _nl_direct */
	unsigned short context_addr;	/* _nl_context, msgindex[CONTEXT] */
};

/* LC_COLLATE Tables (D) */

struct col_header {
	unsigned int seqtab_addr;	/* _seqtab */
	unsigned int pritab_addr;	/* _pritab */
	unsigned short nl_map21;	/* not an address */
	unsigned short nl_onlyseq;	/* not an address */
	unsigned int sortrule[COLL_WEIGHTS_MAX];	/* sort directives */
	unsigned int tab21_addr;
	unsigned int tab12_addr;
};


/* LC_CTYPE Tables (E) */

struct ctype_header {
	unsigned int _sh_high;		/* _sh_high */
	int _sh_low;			/* _sh_low */
	unsigned int _ctype_addr;	/* __ctype */
	unsigned int _ctype2_addr;	/* __ctype2 */
	unsigned int kanji1_addr;	/* _1kanji */
	unsigned int kanji2_addr;	/* _2kanji */
	unsigned int upshift_addr;	/* _upshift */
	unsigned int downshift_addr;	/* _downshift */
	unsigned int byte_char_addr;	/* msgindex[BYTES_CHAR] */
	unsigned int alt_punct_addr;	/* msgindex[ALT_PUNCT] */
					/* _nl_punct_alt[] */
					/* _nl_space_alt   */
#ifdef EUC
	unsigned int io_csize_addr;	/* __io_csize[] */
	unsigned int e_cset_addr;	/* __e_cset */
	unsigned int ein_csize_addr;	/* __ein_csize */
	unsigned int eout_csize_addr;	/* __eout_csize */
	unsigned int code_scheme_addr;	/* msgindex[CODE_SCHEME] */
	unsigned int cswidth_addr;	/* msgindex[CSWIDTH] */
#endif /* EUC */
	unsigned int reserved1;		/* reserved for locale.inf data */
};



/* LC_MONETARY Tables (F) */


struct monetary_header {
	unsigned short int_frac_digits;		/* _lconv->short_frac_digits */
	unsigned short frac_digits;		/* _lconv->frac_digits */
	unsigned short p_cs_precedes;		/* _lconv->p_cs_precedes */
	unsigned short p_sep_by_space;		/* _lconv->p_sep_by_space */
	unsigned short n_cs_precedes;		/* _lconv->n_cs_precedes */
	unsigned short n_sep_by_space;		/* _lconv->n_sep_by_space */
	unsigned short p_sign_posn;		/* _lconv->p_sign_posn */
	unsigned short n_sign_posn;		/* _lconv->n_sign_posn */
	unsigned short curr_symbol_lconv;		/* _lconv->currency_symbol */
	unsigned short curr_symbol_li;		/* msgindex[CRNCYSTR] */
	unsigned short mon_decimal_point;		/* _lconv->mon_decimal_point */
	unsigned short int_curr_symbol;		/* _lconv->short_curr_symbol */
	unsigned short mon_thousands_sep;		/* _lconv->mon_thousands_sep */
	unsigned short mon_grouping;		/* _lconv->mon_grouping */
	unsigned short positive_sign;		/* _lconv->positive_sign */
	unsigned short negative_sign;		/* _lconv->negative_sign */
};



/* LC_NUMERIC Tables (G) */


struct numeric_header {
	unsigned short grouping;			/* _lconv->grouping */
	unsigned short decimal_point;		/* _lconv->decimal_point */
						/* msgindex[RADIXCHAR] */
						/* _nl_radix */
	unsigned short thousands_sep;		/* _lconv->thousands_sep */
						/* msgindex[THOUSEP] */
	unsigned short alt_digit_addr;		/* msgindex[ALT_DIGIT] */
};



/* LC_TIME Tables (H) */

struct time_header {
	unsigned short d_t_fmt;			/* msgindex[D_T_FMT] */
	unsigned short d_fmt;			/* msgindex[D_FMT] */
	unsigned short t_fmt;			/* msgindex[T_FMT] */
	unsigned short day_1;			/* msgindex[DAY_1] */
	unsigned short day_2;			/* msgindex[DAY_2] */
	unsigned short day_3;			/* msgindex[DAY_3] */
	unsigned short day_4;			/* msgindex[DAY_4] */
	unsigned short day_5;			/* msgindex[DAY_5] */
	unsigned short day_6;			/* msgindex[DAY_6] */
	unsigned short day_7;			/* msgindex[DAY_7] */
	unsigned short abday_1;			/* msgindex[ABDAY_1] */
	unsigned short abday_2;			/* msgindex[ABDAY_2] */
	unsigned short abday_3;			/* msgindex[ABDAY_3] */
	unsigned short abday_4;			/* msgindex[ABDAY_4] */
	unsigned short abday_5;			/* msgindex[ABDAY_5] */
	unsigned short abday_6;			/* msgindex[ABDAY_6] */
	unsigned short abday_7;			/* msgindex[ABDAY_7] */
	unsigned short mon_1;			/* msgindex[MON_1] */
	unsigned short mon_2;			/* msgindex[MON_2] */
	unsigned short mon_3;			/* msgindex[MON_3] */
	unsigned short mon_4;			/* msgindex[MON_4] */
	unsigned short mon_5;			/* msgindex[MON_5] */
	unsigned short mon_6;			/* msgindex[MON_6] */
	unsigned short mon_7;			/* msgindex[MON_7] */
	unsigned short mon_8;			/* msgindex[MON_8] */
	unsigned short mon_9;			/* msgindex[MON_9] */
	unsigned short mon_10;			/* msgindex[MON_10] */
	unsigned short mon_11;			/* msgindex[MON_11] */
	unsigned short mon_12;			/* msgindex[MON_12] */
	unsigned short abmon_1;			/* msgindex[ABMON_1] */
	unsigned short abmon_2;			/* msgindex[ABMON_2] */
	unsigned short abmon_3;			/* msgindex[ABMON_3] */
	unsigned short abmon_4;			/* msgindex[ABMON_4] */
	unsigned short abmon_5;			/* msgindex[ABMON_5] */
	unsigned short abmon_6;			/* msgindex[ABMON_6] */
	unsigned short abmon_7;			/* msgindex[ABMON_7] */
	unsigned short abmon_8;			/* msgindex[ABMON_8] */
	unsigned short abmon_9;			/* msgindex[ABMON_9] */
	unsigned short abmon_10;		/* msgindex[ABMON_10] */
	unsigned short abmon_11;		/* msgindex[ABMON_11] */
	unsigned short abmon_12;		/* msgindex[ABMON_12] */
	unsigned short am_str;			/* msgindex[AM_STR] */
	unsigned short pm_str;			/* msgindex[PM_STR] */
	unsigned short year_unit;		/* msgindex[YEAR_UNIT] */
	unsigned short mon_unit;		/* msgindex[MON_UNIT] */
	unsigned short day_unit;		/* msgindex[DAY_UNIT] */
	unsigned short hour_unit;		/* msgindex[HOUR_UNIT] */
	unsigned short min_unit;		/* msgindex[MIN_UNIT] */
	unsigned short sec_unit;		/* msgindex[SEC_UNIT] */
	unsigned short era_fmt;			/* msgindex[ERA_FMT] */
	unsigned short era_count;		/* number of era entries */
	unsigned short era_names;		/* address of era name and format strings */
	unsigned short era_addr;		/* address of era data structure entries */
	unsigned short t_fmt_ampm;		/* msgindex[T_FMT_AMPM] */

	unsigned short reserved1;		/* reserved for XPG4 */
	unsigned short reserved2;
	unsigned short reserved3;
	unsigned short reserved4;
	unsigned short reserved5;
	unsigned short reserved6;
};

/* LC_MESSAGES Tables (I) */

struct msg_header {
	unsigned short yesexpr_addr;		/* msgindex[YESEXPR] */
	unsigned short noexpr_addr;		/* msgindex[NOEXPR] */
	unsigned short yes_addr;		/* msgindex[YESSTR] */
	unsigned short no_addr;			/* msgindex[NOSTR] */
};

#endif /* _SETLOCALE_INCLUDED */
