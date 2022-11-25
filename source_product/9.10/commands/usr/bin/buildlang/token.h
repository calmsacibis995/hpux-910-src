/* @(#) $Revision: 66.1 $ */     
/*
** Token constants used as the interface between the lexical analyser
** and the main interpreter program.
*/

/*
** Tokens
*/

#define TLC_ALL		101
#define TLC_CTYPE	102
#define TLC_COLLATE	103
#define TLC_MONETARY	104
#define TLC_NUMERIC	105
#define TLC_TIME	106
#define END_LC		107


#define ALL		2
#define CTYPE		3
#define MONEY		4
#define NUMERIC		5
#define TIME		6

#define LANGNAME	7
#define LANGID		8

#define HPREVISION	9
#define REVISION	10

/* #define CTYPERANGE	11 */
#define ISUPPER		12
#define ISLOWER		13
#define ISDIGIT		14
#define ISXDIGIT	15
#define ISBLANK		16
#define ISSPACE		17
#define ISPUNCT		18
#define ISCNTRL		19
#define ISFIRST		20
#define ISSECOND	21
#ifdef EUC
#define CODE_SCHEME	42
#define CSWIDTH		43
#endif /* EUC */

/* #define SHIFTRANGE	22 */
#define UL		23
#define TOUPPER		24
#define TOLOWER		25

#define MODIFIER	26
#define SEQUENCE	27

#define ERA		28

#define NUMBER		29
#define DASH		30
#define LAB		31
#define RAB		32
#define LCB		33
#define RCB		34
#define LP		35
#define RP		36
#define LB		37
#define RB		38
#define STRING		39

#define BAD_KEYWORD	40
#define BAD_LEXEME	41

/*
** Sub-Tokens
*/


#include	<langinfo.h>
#define CONTEXT		80

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
