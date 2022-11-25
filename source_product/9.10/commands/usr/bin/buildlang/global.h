/* @(#) $Revision: 66.1 $ */    
/*
** This file contains global information including:
** 	1). External declarations for functions and variables
** 	2). Constants
** 	3). Macros
*/

#include "define.h"			/* bring in definitions */
#include "types.h"			/* bring in derived types */
#include "lctypes.h"			/* bring in lc_catagory types */
#include "token.h"			/* bring in token types */
#include "error.h"			/* bring in error messages */

extern unsigned char *ctype1;		/* defined in global.c */
extern unsigned char *ctype2;		/* defined in global.c */
extern unsigned char *ctype3;		/* defined in global.c */
extern unsigned char *ctype4;		/* defined in global.c */
extern unsigned char *upper;		/* defined in global.c */
extern unsigned char *lower;		/* defined in global.c */
#ifdef EUC
extern unsigned char io_charsize[];	/* defined in global.c */
extern unsigned char *e_cset;		/* defined in global.c */
extern unsigned char *ein_csize;	/* defined in global.c */
extern unsigned char *eout_csize;	/* defined in global.c */
#endif /* EUC */

extern seq_ent seq_tab[];		/* defined in global.c */
extern two1 two1_tab[];			/* defined in global.c */
extern seq_ent one2_tab[];		/* defined in global.c */

extern unsigned char *info_tab[];	/* defined in global.c */
extern struct _era_data *era_tab[];	/* defined in global.c */
extern unsigned char *era_tab2[];	/* defined in global.c */
extern unsigned char *mntry_tab[];	/* defined in global.c */
extern unsigned char *nmrc_tab[];	/* defined in global.c */

extern int op;				/* defined in global.c */

extern int dash;			/* defined in global.c */
extern int lab;				/* defined in global.c */
extern int rab;				/* defined in global.c */
extern int lcb;				/* defined in global.c */
extern int rcb;				/* defined in global.c */
extern int lp;				/* defined in global.c */
extern int rp;				/* defined in global.c */
extern int lb;				/* defined in global.c */
extern int rb;				/* defined in global.c */

extern int left;			/* defined in global.c */
extern int right;			/* defined in global.c */

extern int no_install;			/* defined in global.c */
extern int newlang;			/* defined in global.c */
extern int newid;			/* defined in global.c */
extern int is1st;			/* defined in global.c */
extern int collate;			/* defined in global.c */

extern void (*number)();		/* defined in global.c */
extern void (*string)();		/* defined in global.c */
extern void (*rang_exec)();		/* defined in global.c */
extern void (*finish)();		/* defined in global.c */
extern void (*r_angle)();		/* defined in global.c */
extern void (*l_angle)();		/* defined in global.c */
extern void (*r_square)();		/* defined in global.c */
extern void (*l_square)();		/* defined in global.c */
extern void (*two1_xec)();		/* defined in global.c */
extern void (*one2_xec)();		/* defined in global.c */

extern int cur_cat;			/* defined in global.c */
extern int cur_mod;			/* defined in global.c */
extern int gotinfo;			/* defined in global.c */

extern int seq_no;			/* defined in global.c */
extern int max_pri_no;			/* defined in global.c */
extern list *two1_head;			/* defined in global.c */
extern list *two1_tail;			/* defined in global.c */
extern int two1_len;			/* defined in global.c */
extern int one2_len;			/* defined in global.c */
extern int twoi;			/* defined in global.c */
extern int onei;			/* defined in global.c */

extern int c_offset;			/* defined in global.c */
extern int s_offset;			/* defined in global.c */

extern int era_num;			/* defined in global.c */
extern int era_len;			/* defined in global.c */
extern int era_len2;			/* defined in global.c */
extern int pad_byte;			/* defined in global.c */

extern void error();			/* defined in error.c */
extern void Error();			/* defined in error.c */

extern int lineno;			/* defined in lexical analyser
					   value of current line number */
extern int num_value;			/* defined in lexical analyser
					   value of numbers */
extern char yytext[];			/* defined in lexical analyser
					   pointer to strings */
extern int yylex();			/* defined in lexical analyser
					   returns tokens */

extern cat_mod cmlog[];			/* defined in global.c */

extern struct lctable_header	lctable_head;	/* defined in global.c */
extern struct catinfotype	catinfo[];	/* defined in global.c */

extern struct lcall_header	lcall_head;	/* defined in global.c */
extern struct lccol_header	lccol_head;	/* defined in global.c */
extern struct lcctype_header	lcctype_head;	/* defined in global.c */
extern struct lcmonetary_header	lcmonetary_head;/* defined in global.c */
extern struct lcnumeric_header	lcnumeric_head;	/* defined in global.c */
extern struct lctime_header	lctime_head;	/* defined in global.c */

#ifdef	DBG
extern FILE *dfp;			/* defined in global.c */
#endif
