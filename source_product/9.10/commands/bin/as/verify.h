/* @(#) $Revision: 70.1 $ */     

# ifdef M68851
# define MAXARG 4
# else
# define MAXARG 3
# endif

/******* We might as well removed the oprnd_size_type[] now, and
 * the corresponding size array from the instruction structure since
 * we just get a bit count from the bitgeneration.  We don't need
 * these pass1 size calculations anymore.
 */

struct verify_entry {
   short		icode;
   unsigned long	instr_size;
   short		noperands;
   unsigned long	oprnd_mask[MAXARG];
   unsigned short 	actions[3];
   /*short		icode_type; ??? */
};

extern struct verify_entry verify_table[];
extern short  matchindex[];


/* Map a key value (had better be 0..31 !! ) to a bit in a mask */
/*  !!! when (if) go to automated generation of this file, can check
 *  this values as masks are built.
 */
# define M(key)  BITMASK(key,1)


/* Defines for the template matching bits.
 * For each addressing mode, define a corresponding bit in the template
 * match mask.  If certain addressing modes always occur together, then
 * they can have the same bit here.
 * Since, currently, only one 32-bit long is allocated in the match
 * templates for each operand, these defines must stay in the range of
 * 0..31.  If this ever becomes insufficient, we will have to modify the
 * templates and matching to use multiple words.
 */
# define T_DREG		0
# define T_AREG		1
# define T_ADECR	2
# define T_AINCR	3

/* indirect through Areg type addressing */
# define T_ABASE	4
# define T_ADISP16	5	/* kept distinct for "movp" */
# define T_AINDXD8	4
# define T_AINDXBD	4
# define T_AMEM		4
# define T_AMEMX	4

/* PC-relative type addressing */
# define T_PCDISP16	6
# define T_PCINDXD8	6
# define T_PCINDXBD	6
# define T_PCMEM	6
# define T_PCMEMX	6

# define T_IMM		7

/* absolute */
# define T_ABS16	8
# define T_ABS32	8

/* Specials */
# define T_CREG		9
# define T_CCREG	10
# define T_SRREG	11
# define T_DREGPAIR	12
# define T_REGPAIR	13
# define T_REGLIST	14
# define T_BFSPECIFIER	15

# define T_CACHELIST	16	/* #ifdef OFORTY	*/

/* 68881 modes */
# define T_FPDREG	17
# define T_FPCREG	18
# define T_FPDREGPAIR	19
# define T_FPDREGLIST	20
# define T_FPCREGLIST	21
# define T_FPPKSPECIFIER	22

/* 68881 */
# define T_PMUREG	23

/* DRAGON Floating point accelerator */
# define T_DRGDREG	24
# define T_DRGCREG	25
