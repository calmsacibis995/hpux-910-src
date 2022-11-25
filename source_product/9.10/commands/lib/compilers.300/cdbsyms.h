/*SCCS  cdbsyms.h     REV(64.1);       DATE(04/03/92        14:21:53) */
/* KLEENIX_ID @(#)cdbsyms.h	64.1 91/08/28 */

/* Definitions internal to the compiler -- for producing a HP-cdb symbol 
 * table.
 */

/* The following structures define the compiler's internal view of a
 * dnttpointer.  There are four CCOM_DNTTPTR_KIND's defined:
 *	1) a symbolic label Dnnn
 *	2) a numeric table index -- build into a complete NON_IMMED DNTTPOINTER
 *	3) an immediate pointer.
 *	4) a null pointer -- ie, does not contain a dnttpointer; but may hold
 *	 		     other info needed by compiler (eg, 'recursion' bit)
 */

/* These values of CCOM_DNTTPTR_KIND and the fields in CCOM_DNTTPTR structures
 * are defined so that the bits of a CCOM_DNTTPTR agree with those of a symtab.h
 * DNTTPOINTER for the INDEXED and IMMED cases.
 * Don't change without recoding 'putdnttpointer' in cdbsyms.c !!!!
 */
# include "symtab.h"

#define DP_NULL          0
#define DP_LNTT_SYMBOLIC 1
#define DP_GNTT_SYMBOLIC 2
#define DP_LNTT_INDEXED  4
#define DP_GNTT_INDEXED  5
#define DP_IMMED         6
#define DP_NIL           7

typedef unsigned long  CCOM_DNTTPTR_KIND;

struct DNTTPTR_SYMBOLIC  {
	CCOM_DNTTPTR_KIND  dptrkind: 3;	/* 1 -> LNTT; 2 -> GNTT */
	unsigned long recursive: 1;
	unsigned long  labnum:28;
};


struct DNTTPTR_INDEXED  {
	CCOM_DNTTPTR_KIND dptrkind: 3;	/* 4 -> LNTT ; 5-> GNTT */
	unsigned long index: 29;
};

struct DNTTPTR_IMMED  {
	CCOM_DNTTPTR_KIND dptrkind: 3;	/* always 6 */
	BASETYPE  type : 5;
	unsigned long bitlength : 24;
};

struct DNTTPTR_NULL {
	CCOM_DNTTPTR_KIND dptrkind: 3;	/* always 0 */
	unsigned long recursive: 1;
	unsigned long special: 28;
};


typedef union {
	struct DNTTPTR_IMMED	dnttptr_immed;
	struct DNTTPTR_SYMBOLIC	dnttptr_symbolic;
	struct DNTTPTR_INDEXED	dnttptr_index;
	struct DNTTPTR_NULL	dnttptr_null;
	unsigned long 		dnttptr_ccom;  /* for efficient argument 
						 passing */
} CCOM_DNTTPTR;


/* macros for constructing CCOM_DNTTPTR's */
#ifndef IRIF
#       define SYMBOLIC_LNTT_DP(labnum) (0x20000000 | (labnum))
#       define SYMBOLIC_GNTT_DP(labnum) (0x40000000 | (labnum))
#       define INDEXED_LNTT_DP(index) (0x80000000 | (index))
#       define INDEXED_GNTT_DP(index) (0xA0000000 | (index))
#endif /* IRIF */

# define IMMED_DP(basetype,bitlength) (0xC0000000 | (unsigned int) (basetype)<<24 | (bitlength))

# define DNTTINT IMMED_DP(T_INT,SZINT)

int sltnormal();

/* Number of nested scopes "{ }" that can be handled -- currently limited by
 * a static array used to store "begin markers".
 */
# define CDB_NSCOPES 50
