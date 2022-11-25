/* @(#) $Revision: 70.1 $ */    

# include  "symbols.h"
# include  "adrmode.h"
# include  "bitmask.h"
# include  "ivalues.h"
# include  "verify.h"

/************************** vtables.c ***********************************/
/* This file defines
 *	verify_table[]
 * an array of structures defining the legal syntax templates for each
 * opcode.  The array matchindex[] (match.c) built by the program
 * "genmatch" contains the index to the first template for each opcode
 * number.  It is required that all the templates for a given I_xxx opcode
 * number be contiguous in the verify_table[].  There are no requirements
 * for the relative ordering of templates for different I_xxx opcode numbers.
 *
 * The operand addressing modes of the templates use specific (not generic)
 * addressing modes -- reflecting the fact that these templates are searched
 * after the routine "fix_addr_mode" has converted generic addressing modes
 * to specific 68020 addressing modes.
 */

/* define some masks for the common combinations of addressing modes */
# define DREG		M(T_DREG)
# define AREG		M(T_AREG)
# define ADECR		M(T_ADECR)
# define AINCR		M(T_AINCR)
# define ABASE		M(T_ABASE)
# define ADISP16	M(T_ADISP16)
# define AINDXD8	M(T_AINDXD8)
# define AINDXBD	M(T_AINDXBD)
# define AMEM		M(T_AMEM)
# define AMEMX		M(T_AMEMX)
# define PCDISP16	M(T_PCDISP16)
# define PCINDXD8	M(T_PCINDXD8)
# define PCINDXBD	M(T_PCINDXBD)
# define PCMEM		M(T_PCMEM)
# define PCMEMX		M(T_PCMEMX)
# define ABS16		M(T_ABS16)
# define ABS32		M(T_ABS32)
# define IMM		M(T_IMM)
# define CACHELIST	M(T_CACHELIST)
# define FPDREG		M(T_FPDREG)
# define FPCREG		M(T_FPCREG)
# define FPDREGPAIR	M(T_FPDREGPAIR)
# define FPDREGLIST	M(T_FPDREGLIST)
# define FPCREGLIST	M(T_FPCREGLIST)
# define PKSPEC		M(T_FPPKSPECIFIER)
# define DRGDREG	M(T_DRGDREG)
# define DRGCREG	M(T_DRGCREG)

/* combinations of these */
# define REG  ( AREG|DREG )
# define ANY  ( DREG|AREG|AINCR|ADECR|ABASE|ADISP16|AINDXD8|AINDXBD|AMEM|\
  AMEMX|PCDISP16|PCINDXD8|PCINDXBD|PCMEM|PCMEMX|ABS16|ABS32|IMM )
# define DATA		( ANY & ~AREG )
# define MEMORY		(ANY & ~(AREG|DREG) )
# define CONTROL	( ANY & ~(AREG|DREG|AINCR|ADECR|IMM) )
# define ALTER  ( DREG|AREG|AINCR|ADECR|ABASE|ADISP16|AINDXD8|AINDXBD|\
  AMEM|AMEMX|ABS16|ABS32 )
# define DAT_ALT	( DATA & ALTER )
# define MEM_ALT	( MEMORY & ALTER )
# define CTL_ALT	( CONTROL & ALTER )

# define DRG_DBL_DATA	( MEMORY|DREGPAIR )
# define DRG_DBL_DATA_ALT	( MEM_ALT | DREGPAIR )

/* special cases */
# define CREG		M(T_CREG)
# define CCREG		M(T_CCREG)
# define SRREG		M(T_SRREG)
# define DREGPAIR	M(T_DREGPAIR)
# define REGPAIR	M(T_REGPAIR)|DREGPAIR
# define REGLIST	M(T_REGLIST)
/*# define LABEL		M(T_LABEL)*/
# define LABEL		M(T_ABS32)|M(T_ABS16)
# define BFS		M(T_BFSPECIFIER)

/* MC6851 */
# define PMUREG		M(T_PMUREG)
# define PMUFC		(IMM|DREG|CREG)

/* define masks for sizes */
# define N	M(SZNULL)
# define B	M(SZBYTE)
# define W	M(SZWORD)
# define L	M(SZLONG)
# define S	M(SZSINGLE)
# define D	M(SZDOUBLE)
# define X	M(SZEXTEND)
# define P	M(SZPACKED)
# define NB	N|B
# define NW	N|W
# define NL	N|L
# define NBWL	N|B|W|L
# define NWL	N|W|L
# define NBW	N|B|W
# define WL	W|L
# define BWL	B|W|L
# define BW	B|W
# define BWLS	B|W|L|S
# define BWLSDX B|W|L|S|D|X
# define BWLSDXP B|W|L|S|D|X|P
# define NX	N|X
# define SD	S|D
# define BWLSD	B|W|L|S|D
# define BWLD	B|W|L|D
# define LS	L|S


/* Flag for end-of-table */
# define I_TBLEND -1	

/*  Note : in future, will remove the "operand size" array -- it's no
 *  longer needed under the new bit generation strategy.
 */

struct verify_entry  verify_table[] = {

{ I_ABCD,	NB,	2,		{ DREG, DREG, 0},	
	{ SZBYTE, I_ABCD_REG, 0 } },

{ I_ABCD,	NB,	2,		{ ADECR, ADECR, 0},	
	{ SZBYTE, I_ABCD_MEM, 0 } },


{ I_ADD,	NWL,	2,		{ AREG, DREG, 0},	
	{ SZWORD, I_ADD_REG, 0 } },

{ I_ADD,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, I_ADDI, 1} },

{ I_ADD,	NBWL,	2,		{ DATA, DREG, 0},	
	{ SZWORD, I_ADD_REG, 0} },

{ I_ADD,	NWL,	2,		{ IMM , AREG, 0},	
	{ SZWORD, I_ADDA, 1 } },

{ I_ADD,	NWL,	2,		{ ANY&~IMM, AREG, 0},	
	{ SZWORD, I_ADDA, 0 } },

{ I_ADD,	NBWL,	2,		{ DREG, MEM_ALT, 0},	
	{ SZWORD, I_ADD_MEM, 0 } },

{ I_ADDA,	NWL,	2,		{ ANY, AREG, 0},	
	{ SZWORD, 0, 0 } },

{ I_ADDI,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, 0, 0 } },

{ I_ADDQ,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, 0, 0 } },

{ I_ADDQ,	NWL,	2,		{ IMM, AREG, 0},	
	{ SZWORD, 0, 0 } },

{ I_ADDX,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ADDX_REG, 0 } },

{ I_ADDX,	NBWL,	2,		{ ADECR, ADECR, 0},	
	{ SZWORD, I_ADDX_MEM, 0 } },


{ I_AND,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, I_ANDI, 0 } },

{ I_AND,	NBWL,	2,		{ DATA, DREG, 0},	
	{ SZWORD, I_AND_REG, 0 } },

{ I_AND,	NBWL,	2,		{ DREG, MEM_ALT, 0},	
	{ SZWORD, I_AND_MEM, 0 } },

{ I_AND,	NB,	2,		{ IMM, CCREG, 0},	
	{ SZBYTE, I_ANDI_CCR, 0 } },

{ I_AND,	NW,	2,		{ IMM, SRREG, 0},	
	{ SZWORD, I_ANDI_SR, 0 } },

{ I_ANDI,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, 0, 0 } },

{ I_ANDI,	NB,	2,		{ IMM, CCREG, 0},	
	{ SZBYTE, I_ANDI_CCR, 0 } },

{ I_ANDI,	NW,	2,		{ IMM, SRREG, 0},	
	{ SZWORD, I_ANDI_SR, 0 } },

{ I_ASL,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ASL_REG, 0 } },

{ I_ASL,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_ASL_IMM, 0 } },

{ I_ASL,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_ASL_MEM, 0 } },

{ I_ASL,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_ASL_MEM, 36 } },

{ I_ASR,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ASR_REG, 0 } },

{ I_ASR,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_ASR_IMM, 0 } },

{ I_ASR,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_ASR_MEM, 0 } },

{ I_ASR,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_ASR_MEM, 36 } },


# ifdef M68020
{ I_Bcc,	BWL,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 0 } },
{ I_Bcc,	N,	1,	{ LABEL, 0, 0},	
# ifdef SDOPT
	{ 0, 0, 80/* span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif
# else	/* M68010 */
{ I_Bcc,	N,	1,	{ LABEL, 0, 0},	
# ifdef SDOPT
	{ 0, 0, 80/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif
{ I_Bcc,	BW,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 0 } },
{ I_Bcc,	L,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 40 } },
# endif

{ I_BCHG,	NL,	2,		{ DREG, DREG, 0},	
	{ SZLONG, I_BCHG_REG, 0 } },

{ I_BCHG,	NB,	2,		{ DREG, MEM_ALT, 0},	
	{ SZBYTE, I_BCHG_REG, 0 } },

{ I_BCHG,	NL,	2,		{ IMM, DREG, 0},	
	{ SZLONG, I_BCHG_IMM, 0 } },

{ I_BCHG,	NB,	2,		{ IMM, MEM_ALT, 0},	
	{ SZBYTE, I_BCHG_IMM, 0 } },


{ I_BCLR,	NL,	2,		{ DREG, DREG, 0},	
	{ SZLONG, I_BCLR_REG, 0 } },

{ I_BCLR,	NB,	2,		{ DREG, MEM_ALT, 0},	
	{ SZBYTE, I_BCLR_REG, 0 } },

{ I_BCLR,	NL,	2,		{ IMM, DREG, 0},	
	{ SZLONG, I_BCLR_IMM, 0 } },

{ I_BCLR,	NB,	2,		{ IMM, MEM_ALT, 0},	
	{ SZBYTE, I_BCLR_IMM, 0 } },


{ I_BSET,	NL,	2,		{ DREG, DREG, 0},	
	{ SZLONG, I_BSET_REG, 0 } },

{ I_BSET,	NB,	2,		{ DREG, MEM_ALT, 0},	
	{ SZBYTE, I_BSET_REG, 0 } },

{ I_BSET,	NL,	2,		{ IMM, DREG, 0},	
	{ SZLONG, I_BSET_IMM, 0 } },

{ I_BSET,	NB,	2,		{ IMM, MEM_ALT, 0},	
	{ SZBYTE, I_BSET_IMM, 0 } },


{ I_BTST,	NL,	2,		{ DREG, DREG, 0},	
	{ SZLONG, I_BTST_REG, 0 } },

{ I_BTST,	NB,	2,		{ DREG, MEMORY, 0},	
	{ SZBYTE, I_BTST_REG, 0 } },

{ I_BTST,	NL,	2,		{ IMM, DREG, 0},	
	{ SZLONG, I_BTST_IMM, 0 } },

{ I_BTST,	NB,	2,		{ IMM, (MEMORY & ~IMM), 0},	
	{ SZBYTE, I_BTST_IMM, 0 } },

# ifdef M68020
{ I_BFCHG,	N,	2,	{ BFS, DREG|CTL_ALT, 0},	
	{ 0, 0, 0 } },

{ I_BFCLR,	N,	2,	{ BFS, DREG|CTL_ALT, 0},	
	{ 0, 0, 0 } },

{ I_BFEXTS,	N,	3,	{ BFS, DREG|CONTROL,DREG},	
	{ 0, 0, 0 } },

{ I_BFEXTU,	N,	3,	{ BFS, DREG|CONTROL,DREG},	
	{ 0, 0, 0 } },

{ I_BFFFO,	N,	3,	{ BFS, DREG|CONTROL,DREG},	
	{ 0, 0, 0 } },

{ I_BFINS,	N,	3,	{ DREG, BFS, DREG|CTL_ALT},	
	{ 0, 0, 0 } },

{ I_BFSET,	N,	2,	{ BFS, DREG|CTL_ALT,0},	
	{ 0, 0, 0 } },

{ I_BFTST,	N,	2,	{ BFS, DREG|CONTROL,0},	
	{ 0, 0, 0 } },


{ I_BKPT,	N,	1,	{ IMM, 0, 0},	
	{ 0, 0, 3 } },
# endif

# ifdef M68020
{ I_BRA,	BWL,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 0 } },
{ I_BRA,	N,	1,	{ LABEL, 0, 0},	
# ifdef SDOPT
	{ 0, 0, 81/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif

{ I_BSR,	BWL,	1,	{ LABEL, 0, 0},	
	{ 0 , 0, 0 } },
{ I_BSR,	N,	1,	{ LABEL, 0, 0},	
# ifdef SDOPT
	{ 0, 0, 82/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif
# else  /* M68010 */
{ I_BRA,	BW,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 0 } },
{ I_BRA,	N,	1,	{ LABEL, 0, 0},	
# ifdef SDOPT
	{ 0, 0, 81/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif
{ I_BRA,	L,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 40 } },

{ I_BSR,	BW,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 0 } },
{ I_BSR,	N,	1,	{ LABEL, 0, 0},	
# ifdef SDOPT
	{ 0, 0, 82/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif
{ I_BSR,	L,	1,	{ LABEL, 0, 0},	
	{ 0, 0, 40 } },
# endif

# ifdef M68020
{ I_CALLM,	N,	2,	{ IMM, CONTROL, 0},	
	{ 0, 0, 0 } },

{ I_CAS,	NBWL,	3,	{ DREG, DREG, MEM_ALT },	
	{ SZWORD, 0, 4 } },

{ I_CAS2,	NWL,	3,	{ DREGPAIR, DREGPAIR, REGPAIR }, 	
	{ SZWORD, 0, 5 } },

{ I_CHK2,	NBWL,	2,	{ CONTROL, REG, 0},	
	{ SZWORD, 0, 0 } },
# endif

# ifdef M68020
{ I_CHK,	NWL,	2,	{ DATA, DREG, 0 },	
	{ SZWORD, 0, 0 } },
# else
{ I_CHK,	NW,	2,	{ DATA, DREG, 0 },	
	{ SZWORD, 0, 0 } },
# endif

# ifdef OFORTY
{ I_CINVL,	N,	2,	{ CACHELIST, ABASE, 0 },	
	{ 0, 0, 215 } },

{ I_CINVP,	N,	2,	{ CACHELIST, ABASE, 0 },	
	{ 0, 0, 215 } },

{ I_CINVA,	N,	1,	{ CACHELIST, 0, 0 },	
	{ 0, 0, 0 } },
# endif

{ I_CLR,	NBWL,	1,	{ DAT_ALT, 0, 0 },	
	{ SZWORD, 0, 0 } },


{ I_CMP,	NWL,	2,	{ AREG, ANY, 0 },	
	{ SZWORD, I_CMPA, 0 } },

{ I_CMP,	NBWL,	2,	{ DATA & ~IMM, IMM, 0 },	
	{ SZWORD, I_CMPI, 0 } },

{ I_CMP,	NBWL,	2,	{ AINCR, AINCR, 0 },	
	{ SZWORD, I_CMPM, 0 } },

{ I_CMP,	NBWL,	2,	{ DREG, DATA, 0 },	
	{ SZWORD, 0, 0 } },

{ I_CMP,	NWL,	2,	{ DREG, AREG, 0 },	
	{ SZWORD, 0, 0 } },

{ I_CMPA,	NWL,	2,	{ AREG, ANY, 0 },	
	{ SZWORD, 0, 0 } },

{ I_CMPI,	NBWL,	2,	{ DATA & ~IMM, IMM, 0 },	
	{ SZWORD, 0, 0 } },

{ I_CMPM,	NBWL,	2,	{ AINCR, AINCR, 0 },	
	{ SZWORD, 0, 0 } },

# ifdef M68020
{ I_CMP2,	NBWL,	2,	{ REG, CONTROL, 0 }, 
	{ SZWORD, 0, 0 } },
# endif

# ifdef OFORTY
{ I_CPUSHL,	N,	2,	{ CACHELIST, ABASE, 0 },	
	{ 0, 0, 216 } },

{ I_CPUSHP,	N,	2,	{ CACHELIST, ABASE, 0 },	
	{ 0, 0, 216 } },

{ I_CPUSHA,	N,	1,	{ CACHELIST, 0, 0 },	
	{ 0, 0, 0 } },
# endif

{ I_DBcc,	NW,	2,	{ DREG, LABEL, 0 },	
	{ SZWORD, 0, 0 } },

{ I_DIVS,	NW,	2,	{ DATA, DREG, 0 },	
	{ SZWORD, I_DIVS1, 0 } },

# ifdef M68020
{ I_DIVS,	L,	2,	{ DATA, DREG, 0 },	
	{ 0, I_DIVS2, 0 } },

{ I_DIVS,	NL,	2,	{ DATA, DREGPAIR, 0 },	
	{ SZLONG, I_DIVS3, 0 } },

{ I_DIVSL,	NL,	2,	{ DATA, DREGPAIR, 0 },	
	{ SZLONG, 0, 0 } },

{ I_TDIVS,	NL,	2,	{ DATA, DREG, 0 },	
	{ SZLONG, I_DIVS2, 0 } },

{ I_TDIVS,	NL,	2,	{ DATA, DREGPAIR, 0 },	
	{ SZLONG, I_DIVSL, 0 } },
# endif

{ I_DIVU,	NW,	2,	{ DATA, DREG, 0 },	
	{ SZWORD, I_DIVU1, 0 } },

# ifdef M68020
{ I_DIVU,	L,	2,	{ DATA, DREG, 0 },	
	{ 0, I_DIVU2, 0 } },

{ I_DIVU,	NL,	2,	{ DATA, DREGPAIR, 0 },	
	{ SZLONG, I_DIVU3, 0 } },

{ I_DIVUL,	NL,	2,	{ DATA, DREGPAIR, 0 },	
	{ SZLONG, 0, 0 } },

{ I_TDIVU,	NL,	2,	{ DATA, DREG, 0 },	
	{ SZLONG, I_DIVU2, 0 } },

{ I_TDIVU,	NL,	2,	{ DATA, DREGPAIR, 0 },	
	{ SZLONG, I_DIVUL, 0 } },
# endif


{ I_EOR,	NBWL,	2,	{ IMM,	DAT_ALT, 0 },	
	{ SZWORD, I_EORI, 0 } },

{ I_EOR,	NBWL,	2,	{ DREG, DAT_ALT, 0 },	
	{ SZWORD, 0, 0 } },

{ I_EOR,	NB,	2,		{ IMM,	CCREG,	0 },	
	{ SZBYTE, I_EORI_CCR, 0 } },

{ I_EOR,	NW,	2,		{ IMM,	SRREG,	0 },	
	{ SZWORD, I_EORI_SR, 0 } },

{ I_EORI,	NBWL,	2,	{ IMM,	DAT_ALT, 0 },	
	{ SZWORD, 0, 0 } },

{ I_EORI,	NB,	2,		{ IMM,	CCREG,	0 },	
	{ SZBYTE, I_EORI_CCR, 0 } },

{ I_EORI,	NW,	2,		{ IMM,	SRREG,	0 },	
	{ SZWORD, I_EORI_SR, 0 } },


{ I_EXG,	NL,	2,		{ DREG, DREG, 0 }, 
	{ SZLONG, I_EXG_DD, 0 } },

{ I_EXG,	NL,	2,		{ AREG, AREG, 0 }, 
	{ SZLONG, I_EXG_AA, 0 } },

{ I_EXG,	NL,	2,		{ DREG, AREG, 0 }, 
	{ SZLONG, I_EXG_DA, 0 } },

{ I_EXG,	NL,	2,		{ AREG, DREG, 0 }, 
	{ SZLONG, I_EXG_AD, 0 } },


{ I_EXT,	NWL,	1,	{ DREG, 0, 0 },		
	{ SZWORD, 0, 0 } },

# ifdef M68020
{ I_EXTW,	NL,	1,	{ DREG, 0, 0 },		
	{ SZLONG, I_EXT, 0 } },

{ I_EXTB,	NL,	1,	{ DREG, 0, 0 },		
	{ SZLONG, 0, 0 } },
# endif


{ I_ILLEGAL,	N,	0,	{ 0, 0, 0 },		
	{ 0, 0, 0 } },


{ I_JMP,	N,	1,	{ CONTROL, 0, 0 },	
	{ 0, 0, 0 } },

{ I_JSR,	N,	1,	{ CONTROL, 0, 0 },	
	{ 0, 0, 0 } },


{ I_LEA,	NL,	2,	{ CONTROL, AREG, 0 },	
	{ SZLONG, 0, 0 } },

{ I_LINK,	NW,	2,		{ AREG, IMM, 0 },	
	{ SZWORD, I_LINK_WORD, 0 } },

# ifdef M68020
{ I_LINK,	L,	2,		{ AREG, IMM, 0 },	
	{ 0, I_LINK_LONG, 0 } },
# endif


{ I_LSL,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_LSL_REG, 0 } },

{ I_LSL,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_LSL_IMM, 0 } },

{ I_LSL,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_LSL_MEM, 0 } },

{ I_LSL,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_LSL_MEM, 36 } },

{ I_LSR,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_LSR_REG, 0 } },

{ I_LSR,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_LSR_IMM, 0 } },

{ I_LSR,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_LSR_MEM, 0 } },

{ I_LSR,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_LSR_MEM, 36 } },


{ I_MOVE,L,2,{ IMM, DREG, 0 },	
	{ SZLONG, 0, 33 } },

 {I_MOVE,NBWL,2,{ DATA,DAT_ALT,0},	
	{SZWORD,0,0}}, 

{ I_MOVE,	NWL,	2,	{ ANY, AREG, 0 },	
	{ SZWORD, I_MOVEA, 0 } },

{ I_MOVE,	NWL,	2,	{ AREG, DAT_ALT, 0 },	
	{ SZWORD, 0, 0 } },
 
{ I_MOVE,	NW,	2,		{ CCREG, DAT_ALT, 0 },	
	{ SZWORD, I_MOVE_from_CCR, 0 } },

{ I_MOVE,	NW,	2,		{ DATA, CCREG, 0 },	
	{ SZWORD, I_MOVE_to_CCR, 0 } },

{ I_MOVE,	NW,	2,		{ SRREG, DAT_ALT, 0 },	
	{ SZWORD, I_MOVE_from_SR, 0 } },

{ I_MOVE,	NW,	2,		{ DATA, SRREG, 0 },	
	{ SZWORD, I_MOVE_to_SR, 0 } },

# ifdef M68851
# ifdef OFORTY
{ I_MOVE,	NL,	2,		{ PMUREG, REG, 0},	
	{ SZLONG, I_MOVEC_REG, 210 } },

{ I_MOVE,	NL,	2,		{ REG, PMUREG, 0},	
	{ SZLONG, I_MOVEC_CTL, 211 } },
# endif	/* OFORTY */
# endif	/* M68851 */

{ I_MOVE,	NL,	2,		{ CREG, REG, 0},	
	{ SZLONG, I_MOVEC_REG, 30 } },

{ I_MOVE,	NL,	2,		{ REG, CREG, 0},	
	{ SZLONG, I_MOVEC_CTL, 31 } },


{ I_MOVEA,	NWL,	2,	{ ANY, AREG, 0 },	
	{ SZWORD, 0,0 } },

{ I_MOVEQ,	NL,	2,	{ IMM, DREG, 0 },	
	{ SZLONG, 0, 0 } },

# ifdef M68851
# ifdef OFORTY
{ I_MOVEC,	NL,	2,		{ REG, PMUREG, 0 },	
	{ SZLONG, I_MOVEC_CTL, 211 } },

{ I_MOVEC,	NL,	2,		{ PMUREG, REG, 0 },	
	{ SZLONG, I_MOVEC_REG, 210 } },
# endif	/* OFORTY */
# endif	/* M68851 */

{ I_MOVEC,	NL,	2,		{ REG, CREG, 0 },	
	{ SZLONG, I_MOVEC_CTL, 0 } },

{ I_MOVEC,	NL,	2,		{ CREG, REG, 0 },	
	{ SZLONG, I_MOVEC_REG, 0 } },


{ I_MOVEM,	NWL,	2,		{ REGLIST|IMM, CTL_ALT, 0 },
	{ SZWORD, I_MOVEM_MEM, 35 } },

{ I_MOVEM,	NWL,	2,		{ REGLIST|IMM, ADECR, 0 },  
	{ SZWORD, I_MOVEM_MEM, 34 } },

{ I_MOVEM,	NWL,	2,		{ CONTROL|AINCR, REGLIST|IMM, 0 }, 
	{ SZWORD, I_MOVEM_REG, 35 } },

/* ***	Bug here: a forward reference is going to get turned into a 
 *	A_AINDXBD by the fix_addr_mode routines, and so won't be able
 *	to match ADISP16.
 * ***/
{ I_MOVEP,	NWL,	2,		{ DREG, ADISP16, 0 },	
	{ SZWORD, I_MOVEP_MEM, 0 } },

{ I_MOVEP,	NWL,	2,		{ ADISP16, DREG, 0 },	
	{ SZWORD, I_MOVEP_REG, 0 } },


{ I_MOVES,	NBWL,	2,		{ REG, MEM_ALT, 0 },	
	{ SZWORD, I_MOVES_MEM, 0 } },

{ I_MOVES,	NBWL,	2,		{ MEM_ALT, REG, 0 },	
	{ SZWORD, I_MOVES_REG, 0 } },

#ifdef OFORTY
{ I_MOVE16,	N,	2,		{ AINCR, AINCR, 0 },	
	{ 0, I_MOVE16_INC_INC, 0 } },

{ I_MOVE16,	N,	2,		{ AINCR, LABEL, 0 },	
	{ 0, I_MOVE16_INC_ABS, 0 } },

{ I_MOVE16,	N,	2,		{ LABEL, AINCR, 0 },	
	{ 0, I_MOVE16_ABS_INC, 0 } },

{ I_MOVE16,	N,	2,		{ ABASE, LABEL, 0 },	
	{ 0, I_MOVE16_IND_ABS, 217 } },

{ I_MOVE16,	N,	2,		{ LABEL, ABASE, 0 },	
	{ 0, I_MOVE16_ABS_IND, 218 } },

#endif

{ I_MULS,	NW,	2,	{ DATA, DREG, 0 },	
	{ SZWORD, I_MULS1, 0 } },

# ifdef M68020
{ I_MULS,	L,	2,	{ DATA, DREG, 0 },	
	{ 0, I_MULS2, 0 } },

{ I_MULS,	L,	2,	{ DATA, DREGPAIR, 0 },	
	{ 0, I_MULS3, 0 } },

{ I_TMULS,	NL,	2,	{ DATA, DREG, 0 },	
	{ SZLONG, I_MULS2, 0 } },
# endif

{ I_MULU,	NW,	2,	{ DATA, DREG, 0 },	
	{ SZWORD, I_MULU1, 0 } },

# ifdef M68020
{ I_MULU,	L,	2,	{ DATA, DREG, 0 },	
	{ 0, I_MULU2, 0 } },

{ I_MULU,	L,	2,	{ DATA, DREGPAIR, 0 },	
	{ 0, I_MULU3, 0 } },

{ I_TMULU,	NL,	2,	{ DATA, DREG, 0 },	
	{ SZLONG, I_MULU2, 0 } },
# endif


{ I_NBCD,	NB,	1,	{ DAT_ALT, 0, 0 },	
	{ SZBYTE, 0, 0 } },

{ I_NEG,	NBWL,	1,	{ DAT_ALT, 0, 0 },	
	{ SZWORD, 0, 0 } },

{ I_NEGX,	NBWL,	1,	{ DAT_ALT, 0, 0 },	
	{ SZWORD, 0, 0 } },


{ I_NOP,	N,	0,	{ 0, 0, 0 },		
	{ 0, 0, 0 } },


{ I_NOT,	NBWL,	1,	{ DAT_ALT, 0, 0 },	
	{ SZWORD, 0, 0 } },


{ I_OR,		NBWL,	2,	{ IMM, DAT_ALT, 0},	
	{ SZWORD, I_ORI, 0 } },

{ I_OR,		NBWL,	2,		{ DATA, DREG, 0},	
	{ SZWORD, I_OR_REG, 0 } },

{ I_OR,		NBWL,	2,		{ DREG, MEM_ALT, 0},	
	{ SZWORD, I_OR_MEM, 0 } },

{ I_OR,		NB,	2,		{ IMM, CCREG, 0},	
	{ SZBYTE, I_ORI_CCR, 0 } },

{ I_OR,		NW,	2,		{ IMM, SRREG, 0},	
	{ SZWORD, I_ORI_SR, 0 } },

{ I_ORI,	NBWL,	2,	{ IMM, DAT_ALT, 0},	
	{ SZWORD, 0, 0 } },

{ I_ORI,	NB,	2,		{ IMM, CCREG, 0},	
	{ SZBYTE, I_ORI_CCR, 0 } },

{ I_ORI,	NW,	2,		{ IMM, SRREG, 0},	
	{ SZWORD, I_ORI_SR, 0 } },


# ifdef M68020
{ I_PACK,	N,	3,	{ ADECR, ADECR, IMM },	
	{ 0, I_PACK_MEM, 0 } },

{ I_PACK,	N,	3,	{ DREG, DREG, IMM },	
	{ 0, I_PACK_REG, 0 } },
# endif

{ I_PEA,	NL,	1,	{ CONTROL, 0, 0 },	
	{ SZLONG, 0, 0 } },

{ I_RESET,	N,	0,	{ 0, 0, 0 },		
	{ 0, 0, 0 } },


{ I_ROL,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ROL_REG, 0 } },

{ I_ROL,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_ROL_IMM, 0 } },

{ I_ROL,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_ROL_MEM, 0 } },

{ I_ROL,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_ROL_MEM, 36 } },

{ I_ROR,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ROR_REG, 0 } },

{ I_ROR,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_ROR_IMM, 0 } },

{ I_ROR,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_ROR_MEM, 0 } },
 
{ I_ROR,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_ROR_MEM, 36 } },


{ I_ROXL,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ROXL_REG, 0 } },

{ I_ROXL,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_ROXL_IMM, 0 } },

{ I_ROXL,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_ROXL_MEM, 0 } },

{ I_ROXL,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_ROXL_MEM, 36 } },

{ I_ROXR,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_ROXR_REG, 0 } },

{ I_ROXR,	NBWL,	2,		{ IMM, DREG, 0},	
	{ SZWORD, I_ROXR_IMM, 0 } },

{ I_ROXR,	NW,	1,		{ MEM_ALT,0, 0},	
	{ SZWORD, I_ROXR_MEM, 0 } },

{ I_ROXR,	NW,	2,		{ IMM, MEM_ALT, 0},	
	{ SZWORD, I_ROXR_MEM, 36 } },


{ I_RTD,	N,	1,	{ IMM, 0, 0 },	
	{ 0, 0, 0 } },

{ I_RTE,	N,	0,	{ 0, 0, 0 },	
	{ 0, 0, 0 } },

# ifdef M68020
{ I_RTM,	N,	1,	{ AREG, 0, 0 },	
	{ 0, I_RTM_AREG, 0 } },

{ I_RTM,	N,	1,	{ DREG, 0, 0 },	
	{ 0, I_RTM_DREG, 0 } },
# endif

{ I_RTR,	N,	0,	{ 0, 0, 0 },	
	{ 0, 0, 0 } },

{ I_RTS,	N,	0,	{ 0, 0, 0 },	
	{ 0, 0, 0 } },


{ I_SBCD,	NB,	2,		{ DREG, DREG, 0},	
	{ SZBYTE, I_SBCD_REG, 0 } },

{ I_SBCD,	NB,	2,		{ ADECR, ADECR, 0},	
	{ SZBYTE, I_SBCD_MEM, 0 } },


{ I_Scc,	NB,	1,	{ DAT_ALT, 0, 0 },	
	{ SZBYTE, 0, 0 } },

{ I_STOP,	N,	1,	{ IMM, 0, 0 },	
	{ 0, 0, 0 } },


{ I_SUB,	NWL,	2,		{ AREG, DREG, 0},	
	{ SZWORD, I_SUB_REG, 0 } },

{ I_SUB,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, I_SUBI, 51 } },

{ I_SUB,	NBWL,	2,		{ DATA, DREG, 0},	
	{ SZWORD, I_SUB_REG, 0 } },

{ I_SUB,	NWL,	2,		{ IMM, AREG, 0},	
	{ SZWORD, I_SUBA, 51 } },

{ I_SUB,	NWL,	2,		{ ANY&~IMM, AREG, 0},	
	{ SZWORD, I_SUBA, 0 } },

{ I_SUB,	NBWL,	2,		{ DREG, MEM_ALT, 0},	
	{ SZWORD, I_SUB_MEM, 0 } },

{ I_SUBA,	NWL,	2,		{ ANY, AREG, 0},	
	{ SZWORD, 0, 0 } },

{ I_SUBI,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, 0, 0 } },

{ I_SUBQ,	NWL,	2,		{ IMM, AREG, 0},	
	{ SZWORD, 0, 0 } },

{ I_SUBQ,	NBWL,	2,		{ IMM, DAT_ALT, 0},	
	{ SZWORD, 0, 0 } },

{ I_SUBX,	NBWL,	2,		{ DREG, DREG, 0},	
	{ SZWORD, I_SUBX_REG, 0 } },

{ I_SUBX,	NBWL,	2,		{ ADECR, ADECR, 0},	
	{ SZWORD, I_SUBX_MEM, 0 } },


{ I_SWAP,	NW,	1,	{ DREG, 0, 0 },		
	{ SZWORD, 0, 0 } },

{ I_TAS,	NB,	1,	{ DREG, 0, 0 },	
	{ SZBYTE, 0, 0 } },

{ I_TAS,	NB,	1,	{ MEM_ALT, 0, 0 },	
	{ SZBYTE, 0, 2 } },

{ I_TRAP,	N,	1,	{ IMM, 0, 0 },		
	{ 0, 0, 0 } },

{ I_TRAPV,	N,	0,	{ 0, 0, 0 },		
	{ 0, 0, 0 } },

# ifdef M68020
{ I_Tcc,	N,	0,	{ 0, 0, 0 },		
	{ 0, 0, 0 } },

{ I_TPcc,	NWL,	1,	{ IMM, 0, 0 },		
	{ SZWORD, 0, 0 } },
# endif

{ I_TST,	NWL,	1,	{ ANY & ~IMM, 0, 0 },	
	{ SZWORD, 0, 0 } },

{ I_TST,	B,	1,	{ DATA & ~IMM, 0, 0 },	
	{ 0, 0, 0 } },


{ I_UNLK,	N,	1,	{ AREG, 0, 0 },		
	{ 0, 0, 0 } },


# ifdef M68020
{ I_UNPK,	N,	3,		{ ADECR, ADECR, IMM },	
	{ 0, I_UNPK_MEM, 0 } },

{ I_UNPK,	N,	3,		{ DREG, DREG, IMM },	
	{ 0, I_UNPK_REG, 0 } },
# endif

/* 68881 ops */
# ifdef M68881
{ I_Fmop,	BWLS,		2,	{ DREG, FPDREG, 0},
	{ 0, 0, 101 } },

{ I_Fmop,	BWLSDXP,	2,	{ MEMORY, FPDREG, 0},
	{ 0, 0, 101 } },

{ I_Fmop,	NX,		2,	{ FPDREG, FPDREG, 0},
	{ SZEXTEND, 0, 102 } },

{ I_Fmop,	NX,		1,	{ FPDREG, 0, 0},
	{ SZEXTEND, 0, 102 } },

{ I_Fdop,	BWLS,		2,	{ DREG, FPDREG, 0},
	{ 0, 0, 101 } },

{ I_Fdop,	BWLSDXP,	2,	{ MEMORY, FPDREG, 0},
	{ 0, 0, 101 } },

{ I_Fdop,	NX,		2,	{ FPDREG, FPDREG, 0},
	{ SZEXTEND, 0, 102 } },

{ I_FBcc,	WL,		1,	{ LABEL, 0, 0},
	{ 0, 0, 0 } },

{ I_FBcc,	N,		1,	{ LABEL, 0, 0},
# ifdef SDOPT
	{ 0, 0, 83/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif

{ I_FCMP,	BWLS,		2,	{ FPDREG, DREG, 0},
	{ 0, I_FCMP_EA, 0 } },

{ I_FCMP,	BWLSDXP,	2,	{ FPDREG, MEMORY, 0},
	{ 0, I_FCMP_EA, 0 } },

{ I_FCMP,	NX,		2,	{ FPDREG, FPDREG, 0},
	{ SZEXTEND, I_FCMP_FPDREG, 0 } },

{ I_FDBcc,	N,		2,	{ DREG, LABEL, 0},
	{ SZWORD, 0, 0 } },

{ I_FMOVE,	BWLS,		2,	{ DREG, FPDREG, 0},
	{ 0, I_FMOVE_from_EA, 0 } },

{ I_FMOVE,	BWLSDXP,	2,	{ MEMORY, FPDREG, 0},
	{ 0, I_FMOVE_from_EA, 0 } },

{ I_FMOVE,	NX,		2,	{ FPDREG, FPDREG, 0},
	{ SZEXTEND, I_FMOVE_FPDREG, 0 } },

{ I_FMOVE,	BWLS,		2,	{ FPDREG, DREG, 0},
	{ 0, I_FMOVE_to_EA, 0 } },

{ I_FMOVE,	BWLSDX,		2,	{ FPDREG, MEM_ALT, 0},
	{ 0, I_FMOVE_to_EA, 0 } },

{ I_FMOVE,	P,		3,	{ FPDREG, PKSPEC, MEM_ALT},
	{ SZPACKED, I_FMOVE_toEA_PK, 0 } },

{ I_FMOVE,	NL,		2,	{ DATA, FPCREG, 0},
	{ SZLONG, I_FMOVE_to_FPCREG, 0 } },

{ I_FMOVE,	NL,		2,	{ AREG, FPCREG, 0},
	{ SZLONG, I_FMOVE_to_FPCREG, 103 } },

{ I_FMOVE,	NL,		2,	{ FPCREG, DAT_ALT, 0},
	{ SZLONG, I_FMOVE_from_FPCREG, 0 } },

{ I_FMOVE,	NL,		2,	{ FPCREG, AREG, 0},
	{ SZLONG, I_FMOVE_from_FPCREG, 104 } },


{ I_FMOVECR,	NX,		2,	{ IMM, FPDREG, 0},
	{ SZEXTEND, 0, 0} },



{ I_FMOVEM,	NX,	2,		{ CONTROL|AINCR,IMM|FPDREGLIST, 0},
	{ SZEXTEND, I_FMOVEM_toFP_mode2, 106} },

{ I_FMOVEM,	NX,	2,		{ CONTROL|AINCR,DREG, 0},
	{ SZEXTEND, I_FMOVEM_toFP_mode3, 0} },

{ I_FMOVEM,	NX,	2,		{ IMM|FPDREGLIST,ADECR, 0},
	{ SZEXTEND, I_FMOVEM_fromFP_mode0, 105} },

{ I_FMOVEM,	NX,	2,		{ DREG,ADECR, 0},
	{ SZEXTEND, I_FMOVEM_fromFP_mode1, 0} },

{ I_FMOVEM,	NX,	2,		{ IMM|FPDREGLIST,CTL_ALT, 0},
	{ SZEXTEND, I_FMOVEM_fromFP_mode2, 106} },

{ I_FMOVEM,	NX,	2,		{ DREG,CTL_ALT, 0},
	{ SZEXTEND, I_FMOVEM_fromFP_mode3, 0} },

{ I_FMOVEM,	NL,	2,		{ DREG, FPCREGLIST, 0},
	{ SZLONG, I_FMOVEM_to_FPCREG, 107} },

{ I_FMOVEM,	NL,	2,		{ AREG, FPCREGLIST, 0},
	{ SZLONG, I_FMOVEM_to_FPCREG, 108} },

{ I_FMOVEM,	NL,	2,		{ (MEMORY)&~IMM, FPCREGLIST, 0},
	{ SZLONG, I_FMOVEM_to_FPCREG, 0} },

{ I_FMOVEM,	NL,	2,		{ IMM, FPCREGLIST, 0},
	{ SZLONG, I_FMOVEM_to_FPCREG, 111} },

{ I_FMOVEM,	NL,	2,		{ FPCREGLIST, DREG, 0},
	{ SZLONG, I_FMOVEM_from_FPCREG, 109} },

{ I_FMOVEM,	NL,	2,		{ FPCREGLIST, AREG, 0},
	{ SZLONG, I_FMOVEM_from_FPCREG, 110} },

{ I_FMOVEM,	NL,	2,		{ FPCREGLIST, MEM_ALT, 0},
	{ SZLONG, I_FMOVEM_from_FPCREG, 0} },

{ I_FNOP,	N,	0,	{ 0, 0, 0},
	{ 0, 0, 0 } },

{ I_FRESTORE,	N,		1,	{ CONTROL|AINCR, 0, 0},
	{ 0, 0, 0 } },

{ I_FSAVE,	N,		1,	{ CTL_ALT|ADECR, 0, 0},
	{ 0, 0, 0 } },

{ I_FScc,	NB,		1,	{ DAT_ALT, 0, 0},
	{ SZBYTE, 0, 0 } },


{ I_FSINCOS,	BWLS,		2,	{ DREG, FPDREGPAIR, 0},
	{ 0, I_FSINCOS_EA, 0 } },

{ I_FSINCOS,	BWLSDXP,	2,	{ MEMORY, FPDREGPAIR, 0},
	{ 0, I_FSINCOS_EA, 0 } },

{ I_FSINCOS,	NX,		2,	{ FPDREG, FPDREGPAIR, 0},
	{ SZEXTEND, I_FSINCOS_FPDREG, 0 } },

{ I_FTcc,	N,		0,	{ 0, 0, 0},
	{ 0, 0, 0 } },

{ I_FTPcc,	NWL,		1,	{ IMM, 0, 0},
	{ SZWORD, 0, 0 } },

{ I_FTEST,	BWLS,		1,	{ DREG, 0, 0},
	{ 0, I_FTEST_EA, 0 } },

{ I_FTEST,	BWLSDXP,	1,	{ MEMORY, 0, 0},
	{ 0, I_FTEST_EA, 0 } },

{ I_FTEST,	NX,		1,	{ FPDREG, 0, 0},
	{ SZEXTEND, I_FTEST_FPDREG, 0 } },
# endif /* 68881 */

# ifdef M68851 
/* PMU 68851 ops */

{ I_PBcc,	WL,		1,	{ LABEL, 0, 0},
	{ 0, 0, 0 } },

{ I_PBcc,	N,		1,	{ LABEL, 0, 0},
# ifdef SDOPT
	{ 0, 0, 83/*span-dependent*/ } },
# else
	{ SZWORD, 0, 0 } },
# endif

{ I_PDBcc,	N,		2,	{ DREG, LABEL, 0},
	{ SZWORD, 0, 0 } },

#ifdef OFORTY
{ I_PFLUSH,	N,		1,	{ ABASE, 0, 0},
	{ 0, 0, 219 } },

{ I_PFLUSHN,	N,		1,	{ ABASE, 0, 0},
	{ 0, 0, 219 } },

{ I_PFLUSHA,	N,		0,	{ 0, 0, 0},
	{ 0, 0, 0 } },

{ I_PFLUSHAN,	N,		0,	{ 0, 0, 0},
	{ 0, 0, 0 } },

#else
{ I_PFLUSHA,	N,		0,	{ 0, 0, 0},
	{ 0, 0, 0 } },

{ I_PFLUSH,	N,		2,	{ PMUFC, IMM, 0},
	{ 0, 0, 201 } },

{ I_PFLUSH,	N,		3,	{ PMUFC, IMM, CTL_ALT},
	{ 0, I_PFLUSH_EA, 201 } },

{ I_PFLUSHS,	N,		2,	{ PMUFC, IMM, 0},
	{ 0, 0, 201 } },

{ I_PFLUSHS,	N,		3,	{ PMUFC, IMM, CTL_ALT},
	{ 0, I_PFLUSHS_EA, 201 } },

{ I_PFLUSHR,	N,		1,	{ MEMORY, 0 , 0},
	{ 0, 0, 0 } },
#endif

{ I_PLOADR,	N,		2,	{ PMUFC, CTL_ALT, 0},
	{ 0, 0, 201 } },

{ I_PLOADW,	N,		2,	{ PMUFC, CTL_ALT, 0},
	{ 0, 0, 201 } },

{ I_PMOVE,	N,		2,	{ PMUREG, ALTER, 0},
	{ 0, I_PMOVE_to_EA1, 202 } },

{ I_PMOVE,	N,		2,	{ ANY, PMUREG, 0},
	{ 0, I_PMOVE_to_PMU1, 203 } },

{ I_PRESTORE,	N,		1,	{ CONTROL|AINCR, 0, 0},
	{ 0, 0, 0 } },

{ I_PScc,	NB,		1,	{ DAT_ALT, 0, 0},
	{ SZBYTE, 0, 0 } },

{ I_PSAVE,	N,		1,	{ CTL_ALT|ADECR, 0, 0},
	{ 0, 0, 0 } },

#ifdef OFORTY
{ I_PTESTR,	N,		1,	{ ABASE, 0, 0, 0},
	{ 0, 0, 220 } },

{ I_PTESTW,	N,		1,	{ ABASE, 0, 0, 0},
	{ 0, 0, 220 } },
#else
{ I_PTESTR,	N,		3,	{ PMUFC, CTL_ALT, IMM, 0},
	{ 0, 0, 201 } },

{ I_PTESTR,	N,		4,	{ PMUFC, CTL_ALT, IMM, AREG},
	{ 0, 0, 201 } },

{ I_PTESTW,	N,		3,	{ PMUFC, CTL_ALT, IMM, 0},
	{ 0, 0, 201 } },

{ I_PTESTW,	N,		4,	{ PMUFC, CTL_ALT, IMM, AREG},
	{ 0, 0, 201 } },
#endif

{ I_PTRAPcc,	N,		0,	{ 0, 0, 0},
	{ 0, 0, 0 } },

{ I_PTRAPcc,	NWL,		1,	{ IMM, 0, 0},
	{ SZWORD, 0, 0 } },

{ I_PVALID,	NL,		2,	{ PMUREG, CTL_ALT, 0},
	{ SZLONG, I_PVALID_VAL, 204 } },

{ I_PVALID,	NL,		2,	{ AREG, CTL_ALT, 0},
	{ SZLONG, I_PVALID_AREG, 0 } },
# endif /* M68851 */

/* DRAGON Floatint Point Accelarator */
# ifdef DRAGON
{ I_FPdop,	SD,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, 0, 0 } },

{ I_FPCMP,	SD,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, 0, 0 } },

{ I_FPmop,	SD,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, 0, 0 } },

{ I_FPmop,	SD,		1,	{ DRGDREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPTEST,	SD,		1,	{ DRGDREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPCVS,	L,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPCVSL, 0 } },

{ I_FPCVS,	D,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPCVSD, 0 } },

{ I_FPCVS,	L,		1,	{ DRGDREG, 0, 0 },
	{ 0, I_FPCVSL, 0 } },

{ I_FPCVS,	D,		1,	{ DRGDREG, 0, 0 },
	{ 0, I_FPCVSD, 0 } },

{ I_FPCVD,	L,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPCVDL, 0 } },

{ I_FPCVD,	S,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPCVDS, 0 } },

{ I_FPCVD,	L,		1,	{ DRGDREG, 0, 0 },
	{ 0, I_FPCVDL, 0 } },

{ I_FPCVD,	S,		1,	{ DRGDREG, 0, 0 },
	{ 0, I_FPCVDS, 0 } },

{ I_FPCVL,	S,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPCVLS, 0 } },

{ I_FPCVL,	D,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPCVLD, 0 } },

{ I_FPCVL,	S,		1,	{ DRGDREG, 0, 0 },
	{ 0, I_FPCVLS, 0 } },

{ I_FPCVL,	D,		1,	{ DRGDREG, 0, 0 },
	{ 0, I_FPCVLD, 0 } },

{ I_FPMdop,	S,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, 0, 0 } },

{ I_FPMdop,	D,		3,	{ DRG_DBL_DATA, DRGDREG, DRGDREG },
	{ 0, 0, 0 } },

{ I_FPMmop,	S,		3,	{ DATA, DRGDREG, DRGDREG, 0 },
	{ 0, 0, 0 } },

{ I_FPMmop,	D,		3,	{ DRG_DBL_DATA, DRGDREG, DRGDREG, 0 },
	{ 0, 0, 0 } },

{ I_FPMmop,	S,		2,	{ DATA, DRGDREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPMmop,	D,		2,	{ DRG_DBL_DATA, DRGDREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPM2dop,	S,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, 0, 0 } },

{ I_FPM2dop,	D,		3,	{ DRG_DBL_DATA, DRGDREG, DRGDREG },
	{ 0, 0, 0 } },

{ I_FPM2CMP,	S,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, 0, 0 } },

{ I_FPM2CMP,	D,		3,	{ DRG_DBL_DATA, DRGDREG, DRGDREG },
	{ 0, 0, 0 } },

{ I_FPMTEST,	S,		2,	{ DATA, DRGDREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPMTEST,	D,		2,	{ DRG_DBL_DATA, DRGDREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPMCVS,	L,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, I_FPMCVSL, 0 } },

{ I_FPMCVS,	D,		3,	{ DRG_DBL_DATA, DRGDREG, DRGDREG },
	{ 0, I_FPMCVSD, 0 } },

{ I_FPMCVS,	L,		2,	{ DATA, DRGDREG, 0 },
	{ 0, I_FPMCVSL, 0 } },

{ I_FPMCVS,	D,		2,	{ DRG_DBL_DATA, DRGDREG, 0 },
	{ 0, I_FPMCVSD, 0 } },

{ I_FPMCVD,	L,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, I_FPMCVDL, 0 } },

{ I_FPMCVD,	S,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, I_FPMCVDS, 0 } },

{ I_FPMCVD,	L,		2,	{ DATA, DRGDREG, 0 },
	{ 0, I_FPMCVDL, 0 } },

{ I_FPMCVD,	S,		2,	{ DATA, DRGDREG, 0 },
	{ 0, I_FPMCVDS, 0 } },

{ I_FPMCVL,	S,		3,	{ DATA, DRGDREG, DRGDREG },
	{ 0, I_FPMCVLS, 0 } },

{ I_FPMCVL,	D,		3,	{ DRG_DBL_DATA, DRGDREG, DRGDREG },
	{ 0, I_FPMCVLD, 0 } },

{ I_FPMCVL,	S,		2,	{ DATA, DRGDREG, 0 },
	{ 0, I_FPMCVLS, 0 } },

{ I_FPMCVL,	D,		2,	{ DRG_DBL_DATA, DRGDREG, 0 },
	{ 0, I_FPMCVLD, 0 } },

{ I_FPMOVE,	LS,		2,	{ DATA, DRGDREG, 0 },
	{ 0, I_FPMOVE_to_DRG, 0 } },

{ I_FPMOVE,	D,		2,	{ DRG_DBL_DATA, DRGDREG, 0 },
	{ 0, I_FPMOVED_to_DRG, 0 } },

{ I_FPMOVE,	LS,		2,	{ DRGDREG, DAT_ALT, 0 },
	{ 0, I_FPMOVE_from_DRG, 0 } },

{ I_FPMOVE,	D,		2,	{ DRGDREG, DRG_DBL_DATA_ALT, 0 },
	{ 0, I_FPMOVED_from_DRG, 0 } },

{ I_FPMOVE,	SD,		2,	{ DRGDREG, DRGDREG, 0 },
	{ 0, I_FPMOVE_on_DRG, 0 } },

{ I_FPMOVE,	L,		2,	{ DATA, DRGCREG, 0 },
	{ 0, I_FPMOVE_to_DRGCREG, 0 } },

{ I_FPMOVE,	L,		2,	{ DRGCREG, DATA, 0 },
	{ 0, I_FPMOVE_from_DRGCREG, 0 } },

# ifdef SDOPT
{ I_FPBcc1,	NBWL,		1,	{ LABEL, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPBcc2,	NBWL,		1,	{ LABEL, 0, 0 },
	{ 0, 0, 0 } },
# else
{ I_FPBcc1,	NBWL,		1,	{ LABEL, 0, 0 },
	{ SZWORD, 0, 0 } },

{ I_FPBcc2,	NBWL,		1,	{ LABEL, 0, 0 },
	{ SZWORD, 0, 0 } },
# endif

{ I_FPAREG,	N,		1,	{ AREG, 0, 0 },
	{ 0, 0, 0 } },

{ I_FPWAIT,	N,		0,	{ 0, 0, 0 },
	{ 0, 0, 0 } },

# endif /* DRAGON */

/* Mark the end of templates */
{ I_TBLEND,	N, 	0,	{ 0, 0, 0},
	{ 0, 0, 0 } },
};

