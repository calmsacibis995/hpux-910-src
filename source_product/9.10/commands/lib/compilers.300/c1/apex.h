/* SCCS apex.h    REV(64.3);       DATE(92/04/03        14:22:45) */
/* KLEENIX_ID @(#)apex.h	64.3 92/03/13 */

#define NUM_STD_WORDS	4	/* number of 32-bit words in stds array */
#define ALL_STD_BITS	-1	/* '*' means all stds set */

#define FILE_VERSION	0

struct std_defs {char *name; int std_num; int bits[NUM_STD_WORDS];};

extern struct std_defs *target_list, *origin_list;
extern struct std_defs *get_names();
extern int get_std_num();
extern int get_bit();
extern void set_bit();
extern void clear_bit();
extern void std_print();
extern void std_or();
extern void std_and();
extern int std_equal();
extern int std_overlap();
extern void std_cpy();
extern void std_zero();

extern int apex_flag;
extern int show_stds;


#define REC_CFILE	1
#define REC_F77FILE	2
#define REC_CPLUSFILE	3
#define REC_LDI		4
#define REC_LDI_NEW	5
#define REC_LDI_MAIN	6
#define REC_LDI_NEW_MAIN	7
#define REC_LIB		8
#define REC_LIB_NEW	9
#define REC_LDC		10
#define REC_LDX		11
#define REC_LDX_NEW	12
#define REC_LRV		13
#define REC_LUV		14
#define REC_LUE		15
#define REC_LUM		16
#define REC_LDS		17
#define REC_LDS_NEW	18
#define REC_COM		19
#define REC_F77SET	20
#define REC_F77PASS	21
#define REC_F77BBLOCK	22
#define REC_F77GOTO	23
#define REC_PROCEND	24
#define REC_STD		25
#define REC_HINT	26
#define REC_HDR_ERR	27
#define REC_HDR_HINT	28
#define REC_HDR_DETAIL	29
#define REC_PRINTF	30
#define REC_SCANF	31
#define REC_LUE_LUV	32	
#define REC_NOTUSED	33
#define REC_NOTDEFINED	34
#define REC_LFM		35
#define REC_STD_LDX	36
#define REC_STD_LPR	37
#define REC_FILE_FMT	38
#define REC_F77USE 	39
#define REC_PASFILE	40
#define REC_UNKNOWN	255
