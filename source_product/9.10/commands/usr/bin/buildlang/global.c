/* @(#) $Revision: 66.1 $ */   
/* LINTLIBRARY */
/*
** Global data definitions 
*/

#include <limits.h>
#include "define.h"		/* bring in definitions */
#include "types.h"		/* bring in derived types */
#include "lctypes.h"		/* bring in lc_category types */

unsigned char *ctype1;		/* standard ctype table */
unsigned char *ctype2;		/* 2-byte ctype table */
unsigned char *ctype3;		/* first of 2 ctype table */
unsigned char *ctype4;		/* second of 2 ctype table */

unsigned char *upper;		/* up-shift table */
unsigned char *lower;		/* down-shift table */

#ifdef EUC
unsigned char io_charsize[] =	/* in/out char size table mb char code */
	{ 1, 1, 1, 1,		/* input char size */
	  1, 1, 1, 1  };	/* output char size */

unsigned char *e_cset;		/* expanded table for EUC charater set */
unsigned char *ein_csize;	/* expanded table for input char size */
unsigned char *eout_csize;	/* expanded table for output char size */

#endif /* EUC */
seq_ent seq_tab[TOT_ELMT];	/* sequence table (no & priority) */
two1 two1_tab[TOT_ELMT];	/* 2-1 table */
seq_ent one2_tab[TOT_ELMT];	/* 1-2 table */

unsigned char *info_tab[MAX_INFO_MSGS+1];	/* langinfo table */
struct _era_data *era_tab[MAX_ERA_FMTS];	/* era struct table */
unsigned char *era_tab2[2*MAX_ERA_FMTS];	/* era names and formats table */

unsigned char *mntry_tab[MNTRY_MSGS+1];		/* monetary table */
unsigned char *nmrc_tab[NMRC_MSGS+1];		/* numeric table */

int op;				/* current keyword operator value */

int dash;			/* true if we have a dash */
int lab;			/* true if we have a left angle bracket */
int rab;			/* true if we have a right angle bracket */
int lcb;			/* true if we have a left curly bracket */
int rcb;			/* true if we have a right curly bracket */
int lp;				/* true if we have a left paren */
int rp;				/* true if we have a right paren */
int lb;				/* true if we have a left bracket */
int rb;				/* true if we have a right bracket */

int left;			/* left side of a numeric range */
int right;			/* right side of a numeric range */

int no_install;			/* true if we have a -n command line option */
int newlang;			/* true if we have a new langname */
int newid;			/* true if we have a new langid */
int is1st;			/* true if we have isfirst keyword */
int collate;			/* true if we have collate table specification*/

				/* pointers to void functions */

void (*number)();		/* process number tokens */
void (*string)();		/* process string tokens */
void (*rang_exec)();		/* execute range numbers */
void (*finish)();		/* finish up last keyword */
void (*r_angle)();		/* process right angle bracket tokens
				 * (shift or collate) */
void (*l_angle)();		/* process left angle bracket tokens
				 * (shift or collate) */
void (*r_square)();		/* process right square bracket tokens
				 * (priority or 1-2) */
void (*l_square)();		/* process left square bracket tokens
				 * (priority or 1-2) */
void (*two1_xec)();		/* two to one number process 
				 * (priority or 2-1) */
void (*one2_xec)();		/* one to two number process 
				 * (priority or 1-2) */

int cur_cat;			/* current category id value */
int cur_mod;			/* current index to modifier log */
int gotinfo;			/* flag for modifier */

int seq_no;			/* sequence number */
int max_pri_no;			/* maximum priority number */

list *two1_head;		/* head of temporarily saved 2-1 info. */
list *two1_tail;		/* tail of temporarily saved 2-1 info. */
int two1_len;			/* Length of two-to-one table */
int one2_len;			/* Length of one-to-two table */
int twoi;			/* index into two1_tab */
int onei;			/* index into one2_tab */

int c_offset;			/* offset of ctype's 0th element */
int s_offset;			/* offset of shift's 0th element */

int era_num;			/* number of era in the era table */
int era_len;			/* length of the era data table */
int era_len2;			/* length of the era names and format table */
int pad_byte;			/* flag for padding a byte before era table */

cat_mod cmlog[N_CATEGORY];	/* a log of all category/modifier defined */

				/* locale file headers */
struct lctable_header	lctable_head;
struct catinfotype	catinfo[N_CATEGORY*MAXCMNUM];

struct lcall_header	lcall_head;
struct lccol_header	lccol_head;
struct lcctype_header	lcctype_head;
struct lcmonetary_header	lcmonetary_head;
struct lcnumeric_header	lcnumeric_head;
struct lctime_header	lctime_head;

#ifdef	DBG
FILE *dfp;
#endif
