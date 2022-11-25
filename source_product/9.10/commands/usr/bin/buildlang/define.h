/* @(#) $Revision: 66.1 $ */       
/*
** This file contains definitions used globally
*/

#define FILENAME	"locale.def"

#define AVAILABLE	-99		/* variable available for use */
#define TRUE		1		/* true constant */
#define FALSE		0		/* false constant */

#define MINLANGID	1		/* min number for langid */
#define MAXLANGID	999		/* max number for langid */
#define MIDLANGID	901		/* min number for user defined langid */

#define MAXCMNUM	5		/* max number of modifier per category*/
#define MAXRSTRLEN	36		/* max length of revision string */

#define MNTRY_MSGS	16		/* number of monetary elements */
#define NMRC_MSGS	3		/* number of numeric elements */

#define ZERO		0
#define DFL_LOWCHAR	-1		/* default lowest character code */
#define DFL_HICHAR	255		/* default highest character code */
#define CTYPESIZ	257
/* #define CTYPESIZ	(lcctype_head.sh_high - DFL_LOWCHAR + 1) */
#define SHIFTSIZ	256
/* #define SHIFTSIZ	(shift_head.hi_range - (DFL_LOWCHAR + 1) + 1) */

#define FULL_RANGE	0		/* already have a numeric range*/
#define HALF_RANGE	1		/* got a left part*/
#define EXPLICIT_RANGE	2		/* got a left and right part */
#define IMPLICIT_RANGE	3		/* made a range out of one number */

#define META  				( dash ||		\
			  		  lp   ||   rp  ||	\
			  		  lb   ||   rb  ||	\
			  		  lab  ||   rab ||	\
			  		  lcb  ||   rcb )

#define MAX_Q		19

#define CONSTANT	0
#define TWO1		0100
#define ONE2		0200
#define DC		0300

#define CODE1BYTE	0		/* 1 byte char code scheme */
#ifdef EUC
#define CODE_HP15	1		/* 2 byte HP15 char code scheme */
#define CODE_EUC	2		/* multibyte EUC char code scheme */

#define SS2		0x8e		/* single shift 2 char */
#define SS3		0x8f		/* single shift 3 char */
#define MAX_CODESET	4		/* max number of codesets for EUC */
#else /* EUC */
#define CODE2BYTE	1
#endif /* EUC */

#define N_COMPUTER	0
#define C_LOCALE	99

#define TOT_ELMT	256		/* total elements in sequence table */
#define ENDTABLE	0377		/* end mark of 2-1 character */
#define CHARTYPE	0300		/* flag for character type */

#define LANG2BYTE	(strcmp((char *)info_tab[BYTES_CHAR], "2") ? 0 : 1)
#define xisprint(c)	(ctype1[c]&(_P|_U|_L|_N|_B))
#define xfirstof2(c)	(ctype3[c])
#define xsecof2(c)	(ctype4[c])
