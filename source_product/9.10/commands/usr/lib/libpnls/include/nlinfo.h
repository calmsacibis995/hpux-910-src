/*
 */
#define ALTTOASCII	0
#define ASCIITOALT	1

#define SPACES "                                                                 "

extern int _nl_errno;   /* nls errors */

/*
 *	define for twpo byte character language (nlinfo 35)
 */
#define TWOBYTECHAR	1


/*
 * for the time being these two intrinsics will be the same as their
 *	counterparts. for speed the user can supply the tables.
 */
#define nlscanmovesys(a, b, c, d, e, f, g, h) nlscanmove(a, b, c, d, e, f, g, h)
#define nltranslatesys(a, b, c, d, e, f, g)   nltranslate(a, b, c, d, e, f, g)


/*
 * checks for ascii digit
 */
#define isascdigit(n)	(n >= '0'  &&  n <= '9')



/*
 * to be compatible with mpe, we have defined these boolean values.
 */
#define NL_TRUE		-1
#define NL_FALSE	0

/* almanac errors */
#define E_DAYOFYEAROUTOFRANGE		2
#define E_YEAROFCENTURYOUTOFRANGE	3

/* nlappend errors */
#define E_APPENDNOTHREEBLANKS	4

/* nlconvclock errors */
#define E_INVALIDTIMEFORMAT	3
#define E_INVALIDINPUTLENGTH    4

/* nlfmtcalendar errors */
#define E_INVALIDDATEFORMAT	3

/* nlfmtnum errors */
#define E_INVALIDLENGTH		3
#define E_INVALIDNUMBER		4
#define E_5INTERNAL		5
#define E_6INTERNAL		6
#define E_TRUNCATION		7
#define E_INVALIDNUMSPEC	8
#define E_INVALIDFMTMASK	9
#define E_INVALIDDECIMALS	10

/* nlinfo errors */
#define E_NOTINSTALLED	1
#define E_LNOTCONFIG	2	/* also used by nlnumspec() */
#define E_CNOTCONFIG	3
#define E_NONATTABLE	4
#define E_BADNLT	5
#define E_BADLDST	6
#define E_OUTOFRANGE	10
#define E_SYSTEMERR	1       /* i.e.: NLS not properly installed */

/* nltranslate errors */
#define E_INVALIDCODE	3

/* nlconvcustdate errors */
#define E_INVALIDSTRINGLENGTH	4


/*
**	character set definitions
*/
#define	NLI_NUMERIC	'\000'
#define	NLI_LOWER	'\001'
#define	NLI_UPPER	'\002'
#define	NLI_GRAPH	'\003'
#define	NLI_SPECIAL	'\004'
#define	NLI_CONTROL	'\005'
#define	NLI_FIRSTOF2	'\006'


/*
 * Values for nlcollate and nlkeycompare
 *
 */

#define  GREATER      1
#define  EQUAL        0
#define  SMALLER     -1

#define  GREATER_TIE  2
#define  SMALLER_TIE -2


/*
 *	The following defines correspond to the item numbers
 *	that nlinfo() expects.
 */
#define L_CALFORM	1		/* calendar format */
#define L_CUSTDATE	2		/* customer date */
#define L_CLKSPEC	3		/* clock specification */
#define L_MNTHABBR	4		/* month abbreviations */
#define L_MNTHFULL	5		/* full month names */
#define L_WKDYABBR	6		/* week day abbreviations */
#define L_WKDYFULL	7		/* full week day names */
#define L_YESNO		8		/* yes/no fields */
#define L_DECTHOU	9		/* decimal and tousands seperators */
#define L_CURRSIGNS	10		/* currency signs */
#define L_COLLATE	11		/* collating sequence */
#define L_TYPES		12		/* character types table */
#define L_UPSHIFT	15		/* upshift table */
#define L_DOWNSHIFT	16		/* downshift table */
#define L_LANGS		17		/* array of language numbers.
					   element 0 contains total number of
					   languages in the array */
#define L_SUPPORTED	18		/* -1 = language supported, 0 = no */
#define L_LANGNAME	21		/* language name */
#define L_LANGID	22		/* language id */
#define L_LANGCLASS	26		/* language class number */
#define L_LENCOLLATE	27		/* collating sequence length */
#define L_NDILENGTH	28		/* national dependant info tbl length */
#define L_NDITABLE	29		/* national dependant info table */
#define L_LONGCALFORM	30		/* long calendar format, 36 bytes */
#define L_CURRENCYNAME	31		/* the currency name is returned */
#define L_ALTDIGITS	32		/* alternate set of digits, 8 bytes */
#define L_DIRECTION	33		/* language direction */
#define L_DATAORDER	34		/* 0 - keyboard
					   1 - Left-to-right
					   2 - Right-to-left */
#define L_CHARSIZE	35		/* 0 - 1 byte
					   1 - 2 byte */
#define L_LONGLANGNAME 121           /* long language name */

/*
 *	the following are intended for use within the intrinsics only
 */
#define L_DATELINE	1003		/* MPE Dateline format */
#define L_CURRLANG	1004		/* get current language (currN) */

/*
 *	size definitions
 */
#define SIZE_CHARSET	256		/* size of char defs table */

#define SIZE_ABMONTH	4		/* sizeof month abbreviation */
#define SIZE_MONTH	12		/* sizeof full month name */

#define SIZE_ABDAY	3		/* sizeof day abbreviation */
#define SIZE_DAY	12		/* sizeof full day name */

#define SIZE_YESNOSTR	6		/* sizeof yes/no string */

#define SIZE_NUMSPEC	60		/* sizeof numspec field */

/*
 *	mask characters for nlscanmove() flags
 */
#define M_L	0x0001		/* lower case */
#define M_U	0x0002		/* upper case */
#define M_N	0x0004		/* numeric */
#define M_S	0x0008		/* special */
#define M_WU	0x0010		/* while/until 0/1 */
#define M_US	0x0020		/* upshift */
#define M_DS	0x0040		/* downshift */
#define M_TB	0x0080		/* two byte only flag */
#define M_OB	0x0100		/* one byte only flag */
#define M_OTHER 0xFE00          /* invalid values */

#define M_ALL   M_U | M_L | M_N | M_S

#define M_WHILE 0x0		/* while */
#define M_UNTIL M_WU		/* until */


/*
 * masks for nlconvnum
 */
#define M_STRIPTHOU	0x0001		/* strip thousands seperator */
#define M_STRIPDEC	0x0002		/* strip decimal seperator */
#define M_NUMBERSONLY	0x0004		/* numbers only in input */


/*
 * masks for nlfmtnum
 */
#define M_INSTHOU	0x0001		/* insert thousands seperator */
#define M_INSDEC	0x0002		/* inssert decimal seperator */
#define M_CURRENCY	0x0004		/* insert currency symbol */
#define M_LEFTJUST	0x0008		/* left justify */
#define M_RIGHTJUST	0x0010		/* right justify */
#define M_RETLENGTH	(M_LEFTJUST | M_RIGHTJUST) /* return length of outstr */



#define SME_NOTCONF	 2
#define SME_OVERLAP	 3
#define SME_LENGTH	 4
#define SME_SCANSHIFT	 7
#define SME_UPDOWN	 8
#define SME_INVALIDENTRY 9
#define SME_OUTOFRANGE   10


/*
 *	lengths of fields in the nlinfo structure
 */

#define LENDATELINE		28		/* mpe dateline intrinsic */
#define LENCURRNAME		16		/* currency name */
#define LENALTDIGITS		8		/* alternate digits */
#define LENDIRECT		4		/* language direction */
#define LENLANGNAME		16		/* language name */
#define LENLONGLANGNAME		64		/* long language name */
#define LENCHARSETNAME		16		/* character set name */
#define LENDECTHOU		2		/* decimal/thousand seperator */
#define LENCURRSIGN		6		/* currency sign */
#define LENCALFORMAT		18		/* calendar format */
#define LENLONGCALFORM		36		/* long calendar format */
#define LENCUSTDATEFORMAT	13		/* custom date format */
#define LENCLOCKSPEC		8		/* clock specification */
#define LENWKDYABBR		21		/* week day abbreviation */
#define LENWKDYFULL		84		/* week day full name */
#define LENMNTHABBR		48		/* month abbreviation */
#define LENMNTHFULL		144		/* month full name */


/*
 *	used in nlnumspec
 */
#define CURRENCY_PRECEDES	0
#define CURRENCY_SUCCEEDES	1
#define CURRENCY_REPLACES	2

#define LENCURRENCYSYMBOL	18		/* length of currency symbol */

/* 
 * offsets within buffer for data 
 */
#define NUMSP_ALTDIGITS		2
#define NUMSP_09RANGE		6
#define NUMSP_LANGDIR		4
#define NUMSP_DECSEP		8
#define NUMSP_ALTDEC		9
#define NUMSP_THOUSEP		10
#define NUMSP_ALTTHOU		11
#define NUMSP_ALTPLUS		12
#define NUMSP_ALTMINUS		13
#define NUMSP_RTOLSP		14
#define NUMSP_CURRENCYPLACE	16
#define NUMSP_BYTESCURRENCY	18
#define NUMSP_CURRENCY		20



/*
 *	language structure.
 *	used in a one way linked list.
 */
struct l_info {
	struct l_info	*next;				/* next node in list */

	char	*ctype;

	short	langid;					/* language id */
	short	len_collate;				/* length of collate 
							   table */
	short	class_no;				/* the class this 
							   language belongs to 
							 */
	char	cal_format[LENCALFORMAT];		/* calendar format */
	char	cust_date_format[LENCUSTDATEFORMAT];	/* date format */
	char	clock_specification[LENCLOCKSPEC];	/* clock specs */
	char	long_cal_format[LENLONGCALFORM];	/* long cal format */
	char	date_line[LENDATELINE];			/* MPE DATELINE format 
							 */
	char	month_abbrev[SIZE_ABMONTH * 12];	/* month abbrev */
	char	month_full[SIZE_MONTH * 12];		/* full month names */
	char	wk_day_abbrev[SIZE_ABDAY * 7];		/* wk day abbrev */
	char	wk_day_full[SIZE_DAY * 7];		/* full wk day names */
	char	yes_no[SIZE_YESNOSTR * 2];		/* yes/no */
	char	dec_thou_symbols[LENDECTHOU];		/* decimal/thousands */
	char	curr_signs[LENCURRSIGN];		/* xbssss
					   		   x - currency sign
							   b - if 0, the sign
							       preceeds the 
							       value, otherwise
							       it comes after
							   ssss - the fully
								  qualified 
								  sign */
	char	*collate;				/* collate table */
	char	char_set_definition[SIZE_CHARSET];	/* character set defs.
							   table. similar to
							   ctype. */
	char	upshift[SIZE_CHARSET];			/* upshift table */
	char	downshift[SIZE_CHARSET];		/* downshift table */
	char	name_language[LENLANGNAME];     /* MPE - compatible names */
	char    longname_language[LENLONGLANGNAME];  /* to store long names */
	char	curr_name[LENCURRNAME]; 	/* currency name */
	char	alt_digits[LENALTDIGITS];	/* 0 No alternate set defined
						   1 Alternate digits defined
						   2 The digit '0'
						   3 The digit '9'
						   4 The '+' sign
						   5 The '-' sign
						   6 The decimal separator
						   7 The thousands separator
						*/
	char	lang_dir[LENDIRECT];	/* 0 Direction is 'left-to-right'
					   1 Direction is 'right-to-left'
					   2 The 'right-to-left space
					   3 Reserved
					*/
	short	data_order;		/* 0 - keyboard
					   1 - Left-to-right
					   2 - Right-to-left */
	short	char_size;		/* 0 - 1 byte
					   1 - 2 byte */

	char	numspec[SIZE_NUMSPEC];
};


void nlinfo ();
void free   ();

