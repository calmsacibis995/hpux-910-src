/* @(#) $Revision: 70.11 $ */   
/* LINTLIBRARY */
/*
** Global data definitions 
*/

#include <limits.h> /* Remember: change this to /usr/include */
#include "define.h"		/* bring in definitions */
#include "types.h"		/* bring in derived types */
#include "lctypes.h"		/* bring in lc_category types */
#include <nl_types.h>

int right;			/* right side of a numeric range */
struct lctable_header	lctable_head;
int op;				/* current keyword operator value */

cat_mod cmlog[N_CATEGORY];	/* a log of all category/modifier defined */
int ellipsis=0;			/* true if we have a ellipsis */

struct catinfotype	catinfo[N_CATEGORY*MAXCMNUM];
int lab=0;			/* true if we have a left angle bracket */

struct lcall_header	lcall_head;
int rab=0;			/* true if we have a right angle bracket */

struct lcmesg_header    lcmesg_head; /* Posix.2 */
int lp=0;				/* true if we have a left paren */

struct lccol_header	lccol_head;
int rp=0;				/* true if we have a right paren */

struct lcctype_header	lcctype_head;
int lb=0;				/* true if we have a left bracket */

struct lcmonetary_header	lcmonetary_head;
int rb=0;				/* true if we have a right bracket */

struct lcnumeric_header	lcnumeric_head;
int left;			/* left side of a numeric range */

struct lctime_header	lctime_head;

unsigned char *ctype1;		/* standard ctype table */
unsigned char *ctype2;		/* 2-byte ctype table */
unsigned char *ctype3;		/* first of 2 ctype table */
unsigned char *ctype4;		/* second of 2 ctype table */
unsigned char *ctype5;          /* Posix 11.2 ctype2 */

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
void (*l_paren)();              /* process left paren tokens: Posix.2 */
void (*r_paren)();              /* process right paren tokens: Posix.2 */
void (*collate_sym)();         /* collating symbol process - Posix 11.2 */

int cur_cat;			/* current category id value */
int cur_mod;			/* current index to modifier log */
int gotinfo;			/* flag for modifier */
int blank_found;                /* flag for blank defn in cty_defs() */
int pri_seq=FALSE;              /* flag used in pri.c and coll.c */


int seq_no;			/* sequence number */
int max_pri_no;			/* maximum priority number */

list *two1_head;		/* head of temporarily saved 2-1 info. */
list *two1_tail;		/* tail of temporrily saved 2-1 info. */
int two1_len;			/* Length of two-to-one table */
int one2_len;			/* Length of one-to-two table */
int twoi;			/* index into two1_tab */
int onei;			/* index into one2_tab */

int c_offset;			/* offset of ctype's 0th element */
int c2_offset;			/* offset of ctype22's 0th element */
int s_offset;			/* offset of shift's 0th element */

int era_num;			/* number of era in the era table */
int era_len;			/* length of the era data table */
int era_len2;			/* length of the era names and format table */
int pad_byte;			/* flag for padding a byte before era table */
int exit_val;                   /* exit value in error routines and calls */



int copy_flag;
				/* locale file headers */

#ifdef	DBG
FILE *dfp;
#endif


nl_catd  catd;
int pri_flag;
int ctype_flag = FALSE;         /* used in getstr code */
int first_time=TRUE;			/* used in coll.c */
int semi_flag = FALSE;         /* used for mon_grouping and grouping in grp.c*/
col_ent *col_tab[3];		/* rolling table of 3 adjacent collating
				   entries */
unsigned char *cp;
/*
 * Posix 11.2 for localedef 
 */
int final_buf_len;
unsigned char buf_sym[MAX_COLL_ELEMENT];
unsigned char buf_str[MAX_COLL_ELEMENT];
unsigned char sort_rules[COLL_WEIGHTS_MAX];
col_ent   entry[COLL_WEIGHTS_MAX+1];
int	last_class;
int warn_flag=0;
int undef_seq = UNDEF;
/* adding declarations for each ctype */
int found_coll_sym=0;
int found_modifier=0;
int found_isupper=0;
int found_islower=0;
int found_isdigit=0;
int found_isspace=0;
int found_ispunct=0;
int found_iscntrl=0;
int found_isblank=0;
int found_isxdigit=0;
int found_isfirst=0;
int found_issecond=0;
int found_toupper=0;
int found_tolower=0;              /* flag used in conv.c */
int found_code_scheme=0;
int found_cswidth=0;
int found_ctype=0;
int found_string=0;
int found_number=0;
int found_dash=0;
int found_isgraph=0;
int found_isprint=0;
int found_isalpha=0;
int found_lp=0;
int found_rp=0;
int found_semi=0;
int cflag=0;
char fname[L_tmpnam];
char fch[2] = "x";              /* if no "-o", default to "-ox" */
char *langname = (char *)0;
