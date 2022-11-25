/* @(#) $Revision: 70.2 $ */    
/*
** Error number constants used in error routine: error(constant)
** (see error.c)
*/

#define KEY		0	/* invalid keyword */
#define NUM		1	/* invalid number */
#define CHAR		2	/* invalid character */
#define STATE		3	/* invalid statement */
#define EXPR		4	/* invalid expression */
#define RANGE		5	/* invalid range */

#define NAME_LEN	6	/* invalid langname length */
#define ID_RANGE	7	/* invalid langid number */
#define LANG_LEN	8	/* invalid language name length in langname */
#define TERR_LEN	9	/* invalid territory name length in langname */
#define CODE_LEN	10	/* invalid codeset name length in langname */
#define REV_LEN		11	/* invalid revision string length */
#define LOW_CHAR	12	/* invalid lowest character code value */
#define CHR_CODE	13	/* invalid character code */

#define FST_CODE	14	/* invalid isfirst character code */
#define SND_CODE	15	/* invalid issecond character code */

#ifdef EUC
#define CSCHM_LEN	26	/* invalid code_cscheme name length */
#define CSCHM		27	/* invalid code_cscheme name */

#define CSWIDTH_LEN	28	/* invalid cswidth string length */
#define CSWIDTH_FMT	29	/* invalid cswidth string format */

#endif /* EUC */
#define INFO_LEN	16	/* invalid langinfo string length */
#define MNTRY_LEN	17	/* invalid monetary string length */
#define NMRC_LEN	18	/* invalid numeric string length */
#define MOD_LEN		19	/* invalid modifier string length */

#define NOMEM		20	/* malloc failed */
#define Q_OVER		21	/* priority queue overflow */
#define SAME_YESNO	22	/* yes and no strings can't have the same 
				   first char */
#define ERA_LEN		23	/* invalid era string length */
#define BAD_ERA_FMT	24	/* invalid era string format */
#define TOO_MANY_ERA	25	/* too many era strings are specified */
/*
 * New defines for POSIX
 */
#define SYM_NOT_FOUND   30      /* Symbol not found */
#define MORE_THAN_COPY  31      
#define CHAR_TWICE      32      
#define ILLEGAL_CTYPE_MEM 33
#define NOT_PORTABLE    34
#define NO_CHARMAP      35
#define NOT_SUPP        36
#define SYM_DUPLICATED  37
#define NO_UNDEFINED    38
#define TOO_MANY_ORDER  39
#define WEIGHT_ELLIPSIS 40
#define OPEN_ERR	45
#define READ_ERR	46
#define	SEEK_ERR	47
#define INV_CATEGORY    48
#define CTYPE_TAB	49
#define INV_COL_STMT	50

