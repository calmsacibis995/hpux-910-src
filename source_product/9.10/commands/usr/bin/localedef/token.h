/* @(#) $Revision: 70.2 $ */     
/*
** Token constants used as the interface between the lexical analyser
** and the main interpreter program.
*/

/*
** Tokens
*/

#define TLC_ALL		101
#define TLC_CTYPE	303
#define TLC_COLLATE	306
#define TLC_MONETARY	104
#define TLC_NUMERIC	105
#define TLC_TIME	106
#define TLC_MESG	107
#define END_LC		108
/*  These are defined with orthogonal values so they can be
 *  OR'd together to identify the type of a collating-entry.
 */
#define NUMBER		1 
#define STRING		2
#define COLL_SYM	4
#define ELLIPSIS	8
#define UNDEFINED	16
#define IGNORE 		32
#define FUTURE1		64

#define ALL		17
#define CTYPE		18
#define MESG		19
#define MONEY		20
#define NUMERIC		21
#define TIME		22

#define LANG_NAME	23
#define LANG_ID		24

#define HPREVISION	25
#define RE_VISION	26

/* #define CTYPERANGE	?? */
#define ISUPPER		27
#define ISLOWER		28
#define ISDIGIT		29
#define ISXDIGIT	30
#define ISBLANK		31 	/* 32 reserved */
#define ISSPACE		33
#define ISPUNCT		34
#define ISCNTRL		35
#define ISFIRST		36
#define ISSECOND	37
#ifdef EUC
#define CODE__SCHEME	38
#define CS_WIDTH		39 
#endif  /* EUC */

/* #define SHIFTRANGE	?? */
#define UL		40
#define TOUPPER		41
#define TOLOWER		42

#define MODIFIER	43
#define SEQUENCE	44

#define ERA		45

#define ISALPHA		46
#define COMMENT		47
#define COPY		48

#define POSITION	50
#define ISPRINT		51
#define REVERSE		52
#define ESCAPE		53
#define ISGRAPH		54
#define LP		55
#define RP		56


#define BAD_KEYWORD	57
#define BAD_LEXEME	58
#define COMMA		59
#define SEMI		60
#define S_ORDER		61
#define E_ORDER		62
#define FORWARD		63	/*  64 reserved */
#define EOL		65
#define AM_PM		66
/* #define ALT_DIGITS	67 */
#define ELEM		68
#define SYM		69

/*  This has got to do with Substitute: Revisit if needed */
#define FROM		71
#define WITH		72

#define T_FMT_AP	73
#define LAB		74
#define RAB		75
#define BACKWARD	76 /* was needed in strcoll .c - Revisit again */

/* posix.2 LC_TIME */
/* #define T_FMT_AMPM      80 */
#define DAY             81
#define ABDAY           82
#define MON             83
#define ABMON           84
#define ERA_YEAR        85
/* #define ERA_D_FMT       86 */

/* Posix.2 LC_MESSAGES */
#define   END_MESG      90
/* #define   LC_MESSAGES   91 */
/* #define   YESEXPR       92 */
/* #define   NOEXPR        93 */

/*
** Sub-Tokens
*/


#include        <langinfo.h> 
#define CON_TEXT		80

#define INT_FRAC	1
#define FRAC_DIGITS	2
#define P_CS		3
#define P_SEP		4
#define N_CS		5
#define N_SEP		6
#define P_SIGN		7
#define N_SIGN		8
#define CURRENCY_LC	9
#define CURRENCY_LI	10
#define MON_DECIMAL	11
#define INT_CURR	12
#define MON_THOUSANDS	13
#define MON_GROUPING	14
#define POSITIVE_SIGN	15
#define NEGATIVE_SIGN	16

#define GROUPING	1
#define DECIMAL_P	2
#define THOUSANDS_S	3

#define ZERO		0






