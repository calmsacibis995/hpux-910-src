/* @(#) $Revision: 70.3 $ */      

#include  <limits.h> 
#include "define.h"

#define LC_ALL 		0
#define LC_COLLATE	1
#define LC_CTYPE	2
#define LC_MONETARY	3
#define LC_NUMERIC	4
#define LC_TIME		5
#define LC_MESSAGES	6

#define N_CATEGORY	7		/* number of defined categories */


/* LOCALE Table Header */

/* #pragma HP_ALIGN HPUX_WORD PUSH  */

struct lctable_header {
	unsigned int size;		/* size of table header and category
					   structure */
	unsigned short nl_langid;	/* _nl_langid */
	unsigned char lang[NL_LANGMAX+1];	/* language name */
	unsigned short cat_no;		/* number of categories defined */
	unsigned short mod_no;		/* number of modifiers defined */

	unsigned short rev_flag;	/* true if HP defined locale */
	unsigned char rev_str[MAXRSTRLEN];	/* revision string */
#ifdef EUC
	unsigned short codeset;		/* 0 if 1 byte; 1 if 2 byte HP15;
					   2 if multibyte EUC */
#else /* EUC */
	unsigned short codeset;		/* 0 if 1 byte; 1 if 2 byte */
#endif /* EUC */

	unsigned char comment_char;		/* reserved */
	unsigned char escape_char;	
/*	unsigned int reserved3; */
	unsigned char charmap[NL_LANGMAX +1];
	unsigned short reserved1;
	unsigned short reserved2;
};

/* #pragma HP_ALIGN POP  */


/* CATEGORY/MODIFIER Structure */
/*
 * Catinfotype structure describes a category/modifier table.
 * There is one structure for each category and modifier defined.
 * These entries follow the locale table header.
 */

struct catinfotype 
{
	int size;			/* size of category/modifier table */
	int addr;			/* address of category/modifier table */
	short catid;			/* category id */
	unsigned char mod_name[_POSIX_NAME_MAX+1];	/* name of modifier */
	short mod_addr;			/* address of catinfotype
					   for modifier */
};


/* These are the headers for each of the defined categories.
 * All addresses are offset from the beginning of each category table.
 */

/* LC_ALL Table Header */

struct lcall_header {
	unsigned short yes_addr;	/* __nl_info[YESSTR] */
	unsigned short no_addr;		/* __nl_info[NOSTR] */
	unsigned short direct_addr;	/* __nl_info[DIRECTION] */
					/* _nl_direct */
	unsigned short context_addr;	/* _nl_context */
};

/* Posix 11.2 
 * LC_MESSAGES Table header
 */
struct lcmesg_header {
  unsigned short yes_addr; /* YESEXPR */
  unsigned short no_addr;  /* NOEXPR */
  unsigned short yesstr_addr; /*YESSTR*/
  unsigned short nostr_addr; /*NOSTR*/
};

/* LC_COLLATE Table Header */
struct lccol_header {
	unsigned int seqtab_addr;	/* _seqtab */
	unsigned int pritab_addr;	/* _pritab */
	unsigned short nl_map21;	/* true if exists 2-to-1 mapping */
	unsigned short nl_onlyseq;	/* true if seq no only (no priority) */
	unsigned int sortrule[COLL_WEIGHTS_MAX];   /* Posix 11.2 localedef */
	unsigned int two1tab_addr;	/* _tab21 */
	unsigned int one2tab_addr;	/* _tab12 */
};


/* LC_CTYPE Table Header */

struct lcctype_header {
	unsigned int sh_high;		/* _sh_high */
	int sh_low;			/* _sh_low */
	unsigned int ctype_addr;	/* __ctype */
	unsigned int ctype2_addr;	/*__ctype2 */
	unsigned int kanji1_addr;	/* _1kanji */
	unsigned int kanji2_addr;	/* _2kanji */
	unsigned int upshift_addr;	/* _upshift */
	unsigned int downshift_addr;	/* _downshit */
	unsigned int byte_char_addr;	/* __nl_info[BYTES_CHAR] */
	unsigned int alt_punct_addr;	/* __nl_info[ALT_PUNCT] */
					/* _nl_punct_alt[] */
					/* _nl_space_alt   */
#ifdef EUC
	unsigned int io_csize_addr;	/* __io_csize[] */
	unsigned int e_cset_addr;	/* __e_cset */
	unsigned int ein_csize_addr;	/* __ein_csize */
	unsigned int eout_csize_addr;	/* __eout_csize */
	unsigned int code_scheme_addr;
	unsigned int cswidth_addr;
#endif /* EUC */
	unsigned int reserved1;         /* For XPG4 extensions */
};


/* LC_MONETARY Table Header */

struct lcmonetary_header {
	unsigned short int_frac_digits;		/* _lconv->short_frac_digits */ 
	unsigned short frac_digits;		/* _lconv->frac_digits */
	unsigned short p_cs_precedes;		/* _lconv->p_cs_precedes */
	unsigned short p_sep_by_space;		/* _lconv->p_sep_by_space */
	unsigned short n_cs_precedes;		/* _lconv->n_cs_precedes */
	unsigned short n_sep_by_space;		/* _lconv->n_sep_by_space */
	unsigned short p_sign_posn;		/* _lconv->p_sign_posn */
	unsigned short n_sign_posn;		/* _lconv->n_sign_posn */
	unsigned short currency_symbol_lc;	/* _lconv->currency_symbol */
	unsigned short currency_symbol_li;	/* __nl_info[CRNCYSTR] */
	unsigned short mon_decimal_point;	/* _lconv->mon_decimal_point */	
	unsigned short int_curr_symbol;		/* _lconv->short_curr_symbol */
	unsigned short mon_thousands_sep;	/* _lconv->mon_thousands_sep */
	unsigned short mon_grouping;		/* _lconv->mon_grouping */
	unsigned short positive_sign;		/* _lconv->positive_sign */
	unsigned short negative_sign;		/* _lconv->negative_sign */
};


/* LC_NUMERIC Table Header */

struct lcnumeric_header {
	unsigned short grouping;	/* _lconv->grouping */
	unsigned short decimal_point;	/* _lconv->decimal_point */
					/* __nl_info[RADIXCHAR] */
					/* _nl_radix */
	unsigned short thousands_sep;	/* _lconv->thousands_sep */
					/* __nl_info[THOUSEP] */
	unsigned short alt_digit_addr;	/* __nl_info[ALT_DIGIT] */
};


/* LC_TIME Table Header */

struct lctime_header {
 	unsigned short d_t_fmt;		/* __nl_info[D_T_FMT] */
 	unsigned short d_fmt;		/* __nl_info[D_FMT] */
 	unsigned short t_fmt;		/* __nl_info[T_FMT] */
	unsigned short day_1;		/* __nl_info[DAY_1] */
	unsigned short day_2;		/* __nl_info[DAY_2] */
	unsigned short day_3;		/* __nl_info[DAY_3] */
	unsigned short day_4;		/* __nl_info[DAY_4] */
	unsigned short day_5;		/* __nl_info[DAY_5] */
	unsigned short day_6;		/* __nl_info[DAY_6] */
	unsigned short day_7;		/* __nl_info[DAY_7] */
	unsigned short abday_1;		/* __nl_info[ABDAY_1] */
	unsigned short abday_2;		/* __nl_info[ABDAY_2] */
	unsigned short abday_3;		/* __nl_info[ABDAY_3] */
	unsigned short abday_4;		/* __nl_info[ABDAY_4] */
	unsigned short abday_5;		/* __nl_info[ABDAY_5] */
	unsigned short abday_6;		/* __nl_info[ABDAY_6] */
	unsigned short abday_7;		/* __nl_info[ABDAY_7] */
	unsigned short mon_1;		/* __nl_info[MON_1] */	
	unsigned short mon_2;		/* __nl_info[MON_2] */
	unsigned short mon_3;		/* __nl_info[MON_3] */
	unsigned short mon_4;		/* __nl_info[MON_4] */
	unsigned short mon_5;		/* __nl_info[MON_5] */
	unsigned short mon_6;		/* __nl_info[MON_6] */
	unsigned short mon_7;		/* __nl_info[MON_7] */
	unsigned short mon_8;		/* __nl_info[MON_8] */
	unsigned short mon_9;		/* __nl_info[MON_9] */
	unsigned short mon_10;		/* __nl_info[MON_10] */
	unsigned short mon_11;		/* __nl_info[MON_11] */
	unsigned short mon_12;		/* __nl_info[MON_12] */
	unsigned short abmon_1;		/* __nl_info[ABMON_1] */	
	unsigned short abmon_2;		/* __nl_info[ABMON_2] */
	unsigned short abmon_3;		/* __nl_info[ABMON_3] */
	unsigned short abmon_4;		/* __nl_info[ABMON_4] */
	unsigned short abmon_5;		/* __nl_info[ABMON_5] */
	unsigned short abmon_6;		/* __nl_info[ABMON_6] */
	unsigned short abmon_7;		/* __nl_info[ABMON_7] */
	unsigned short abmon_8;		/* __nl_info[ABMON_8] */
	unsigned short abmon_9;		/* __nl_info[ABMON_9] */
	unsigned short abmon_10;	/* __nl_info[ABMON_10] */
	unsigned short abmon_11;	/* __nl_info[ABMON_11] */
	unsigned short abmon_12;	/* __nl_info[ABMON_12] */
	unsigned short am_str;		/* __nl_info[AM_STR] */
	unsigned short pm_str;		/* __nl_info[PM_STR] */
	unsigned short year_unit;	/* __nl_info[YEAR_UNIT] */
	unsigned short mon_unit;	/* __nl_info[MON_UNIT] */
	unsigned short day_unit;	/* __nl_info[DAY_UNIT] */
	unsigned short hour_unit;	/* __nl_info[HOUR_UNIT] */
	unsigned short min_unit;	/* __nl_info[MIN_UNIT] */
	unsigned short sec_unit;	/* __nl_info[SEC_UNIT] */
	unsigned short era_fmt;		/* __nl_info[ERA_FMT] */
	unsigned short era_count;	/* number of era entries */
	unsigned short era_names;  /* address of era names and formats table */
	unsigned short era_addr;   /* address of era data structure table */
	unsigned short t_fmt_ampm; /* was unsigned short reserved; */
				   /* __nl_info[T_FMT_AMPM] */
	unsigned short reserved1;  /* Reserved for future implementation */
	unsigned short reserved2;
	unsigned short reserved3;
	unsigned short reserved4;
	unsigned short reserved5;
	unsigned short reserved6;

};		

struct lconv  
{
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char frac_digits;
	char int_frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
};

struct  _era_data {		/* type for data associated with an era */
	short start_year;		/* starting year of the era */
	unsigned short start_month;	/* starting month of the era */
	unsigned short start_day;	/* starting day of the era */
	short end_year;			/* ending year of the era */
	unsigned short end_month;	/* ending month of the era */
	unsigned short end_day;		/* ending day of the era */
	short origin_year;		/* time axis origin for era
					   (one of start_year or end_year) */
	short offset;			/* year offset in the era */
	short signflag;			/* adjusts sign of
					   (year - origin_year) value */
	unsigned short reserved;
	unsigned char *name;		/* name of the era */
	unsigned char *format;		/* format of the era */
};








