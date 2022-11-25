/* @(#) $Revision: 70.1 $ */     

#include <model.h>
extern short iopcode[];

/* Structures to define the various opcode/effective address formats
 * for the 68020.
 */

/* PORTABILITY NOTE:  For portablity (cross-assemblers), the declarations
 * here should use "u_int16" instead of "unsigned int" -- see
 * /usr/include/model.h.
 */

/* BRIEF EFFECTIVE ADDRESS : 16-bits */
struct brief_ea {
   u_int16 bea_xregtype:	1;		/* D/A bit */
   u_int16 bea_xregno:	3;
   u_int16 bea_xregsz:	1;
   u_int16 bea_xregscale:	2;
   u_int16 bea_cf1:	1;		/* always 0 */
   u_int16 bea_disp8:	8;
};

/* FULL EFFECTIVE ADDRESS : 1 "core" word, plus base and outer displacements */
struct full_ea_baseword {
   u_int16 fea_xregtype:	1;		/* D/A bit */
   u_int16 fea_xregno:	3;
   u_int16 fea_xregsz:	1;
   u_int16 fea_xregscale:	2;
   u_int16 fea_cf1:	1;		/* always 1 */
   u_int16 fea_bsuppress:	1;
   u_int16 fea_isuppress:	1;
   u_int16 fea_bdsize:	2;
   u_int16 fea_cf2:	1;		/* always 0 */
   u_int16 fea_iiselect:	1;		/* these two fields are just */
   u_int16 fea_odsize:	2;		/* iislect:3 in motorola manual*/
};

/* DISPLACEMENTS FOR FULL EFFECTIVE ADDRESS */
union ea_displacement {
  unsigned char  ead_d8;
  unsigned short ead_d16;
  unsigned int   ead_d32;
};

struct full_ea {
  struct full_ea_baseword  fea_ea;
  union  ea_displacement   fea_bd;
  union  ea_displacement   fea_od;
};

/* Defines for fields in the EA fields. */
/* The Register Type Bit */
# define  EA_RTYPE_D	0
# define  EA_RTYPE_A	1

/* The Index Size Bit */
# define  EA_XSZ_WORD	0
# define  EA_XSZ_LONG	1

/* The Index Scale Factor */
/*  ???? Use an array for these instead ??????? */
# define  EA_XSCL_1	0
# define  EA_XSCL_2	1
# define  EA_XSCL_4	2
# define  EA_XSCL_8	3

/* Base Register Suppress Bit */
# define  EA_BR_EVAL	0
# define  EA_BR_SUPP	1

/* Index Register Suppress Bit */
# define  EA_XR_EVAL	0
# define  EA_XR_SUPP	1

/* Base Displacement Size */
# define  EA_BDSZ_NOTUSED	0
# define  EA_BDSZ_NULL		1
# define  EA_BDSZ_WORD		2
# define  EA_BDSZ_LONG		3

/* Outer Displacement Size */
# define  EA_ODSZ_NONE		0
# define  EA_ODSZ_NULL		1
# define  EA_ODSZ_WORD		2
# define  EA_ODSZ_LONG		3

/* Index/Indirect Selection */
/* Defines here don't seem real clear -- since depends on the value of
 * the IS bit.
 */
/* ??????????????????  */


/* Immediate data -- could be 1 or 2 words */
union  immed_data {
	unsigned long  imd_long;
	unsigned short imd_word;
	struct {
	  unsigned char hibyte;
	  unsigned char lobyte;
	} imd_bytes;
};

struct ea_bit_template {
	unsigned char  eamode;
	unsigned char  eareg;
	int  nwords;
	unsigned short  extension_word[6];
};

struct instruction_bit_template {
	int nwords;
	unsigned short iwords[11];
};

/* The following structures define the different "primary word" layouts
 * for the instructions.
 */

struct  pwgeneric {
	u_int16	pwg_opspec:	10;
	u_int16	pwg_eamode:	3;
	u_int16	pwg_eareg:	3;
	u_int16	pwg_wd1:	16;
	u_int16	pwg_wd2:	16;
};

/* ori, andi, subi, addi, eori, cmpi */
/* negx, clr, neg, not, tst */
struct  pwfmt1 {
	u_int16	fmt1_opcode:	8;
	u_int16	fmt1_size:	2;
	u_int16	fmt1_eamode:	3;
	u_int16	fmt1_eareg:	3;
};

/* Entire word specified. No variable fields.
 * ori, eori, andi to CCR, SR
 * illegal, reset, nop, stop, rte, rtd, rts, trapv, rtr, tCC
 */
struct pwfmt2 {
	u_int16	fmt2_opcode:	16;
};

/* cmp2, chk2 */
struct pwfmt3 {
	u_int16	fmt3_cf1:	5;	/* always 0 */
	u_int16	fmt3_size:	2;
	u_int16	fmt3_cf2:	3;	/* always 3 */
	u_int16	fmt3_eamode:	3;
	u_int16	fmt3_eareg:	3;
	/* word two */
	u_int16	fmt3_daflag:	1;
	u_int16	fmt3_reg2:	3;
	u_int16	fmt3_cf3:	12;	/* fixed for each op */
};

/* cas */
struct pwfmt4 { /* two words */
	u_int16	fmt4_cf1:	5;	/* always 1 */
	u_int16	fmt4_size:	2;
	u_int16 	fmt4_cf2:	3;	/* always 3 */
	u_int16	fmt4_eamode:	3;
	u_int16	fmt4_eareg:	3;
	/* word 2 */
	u_int16	fmt4_cf3:	7;	/* always 0 */
	u_int16	fmt4_du:	3;
	u_int16	fmt4_cf4:	3;	/* always 0 */
	u_int16	fmt4_dc:	3;
};

/* cas2 */
struct pwfmt5 { /* 3 words */
	u_int16	fmt5_cf1:	5;	/* always 1 */
	u_int16	fmt5_size:	2;
	u_int16 	fmt5_cf2:	9;	/* always 0x0fc */
	/* word 2 */
	u_int16	fmt5_daflg1:	1;
	u_int16	fmt5_reg1:	3;
	u_int16	fmt5_cf3:	3;	/* always 0 */
	u_int16	fmt5_du1:	3;
	u_int16	fmt5_cf4:	3;	/* always 0 */
	u_int16	fmt5_dc1:	3;
	/* word 3 */
	u_int16	fmt5_daflg2:	1;
	u_int16	fmt5_reg2:	3;
	u_int16	fmt5_cf5:	3;	/* always 0 */
	u_int16	fmt5_du2:	3;
	u_int16	fmt5_cf6:	3;	/* always 0 */
	u_int16	fmt5_dc2:	3;
};

/* callm */
struct pwfmt6 { /* 2 words */
	u_int16	fmt6_cf1:	10;
	u_int16	fmt6_eamode:	3;
	u_int16	fmt6_eareg:	3;
	/* word 2 */
	u_int16	fmt6_cf2:	8;
	u_int16	fmt6_d8:	8;
};

/* rtm  */
struct pwfmt7 {
	u_int16	fmt7_cf1:	12;
	u_int16	fmt7_daflag:	1;
	u_int16	fmt7_reg:	3;
};

/* Dynamic bit: bset, bchg, bclr, btst */
struct pwfmt8 {
	u_int16	fmt8_cf1:	4;
	u_int16	fmt8_dreg:	3;
	u_int16	fmt8_cf2:	3;
	u_int16	fmt8_eamode:	3;
	u_int16	fmt8_eareg:	3;
};

/* Static bit: btst, bchg, bclr, bset */
struct pwfmt9 {	/* 2 words */
	u_int16	fmt9_cf1:	10;
	u_int16	fmt9_eamode:	3;
	u_int16	fmt9_eareg:	3;
	/* word two */
	u_int16	fmt9_cf2:	8;
	u_int16	fmt9_bitnum:	8;
};

/* movep */
struct pwfmt10 { /* 2 words */
	u_int16	fmt10_cf1:	4;
	u_int16	fmt10_dreg:	3;
	u_int16	fmt10_direction:2;
	u_int16	fmt10_size:	1;
	u_int16	fmt10_cf2:	3;
	u_int16	fmt10_areg:	3;
	/* 2 word */
	u_int16	fmt10_d16:	16;
};

/* moves */
struct pwfmt11 { /* 2 words */
	u_int16	fmt11_cf1:	8;
	u_int16	fmt11_size:	2;
	u_int16	fmt11_eamode:	3;
	u_int16	fmt11_eareg:	3;
	/* word 2 */
	u_int16	fmt11_daflag:	1;
	u_int16	fmt11_reg2:	3;
	u_int16	fmt11_direction:1;
	u_int16	fmt11_cf2:	11;
};

/* move byte, movea long, move long, move word */
struct pwfmt12 {
	u_int16	fmt12_cf1:	2;
	u_int16	fmt12_size:	2;
	u_int16	fmt12_eareg2:	3;
	u_int16	fmt12_eamode2:	3;
	u_int16	fmt12_eamode1:	3;
	u_int16	fmt12_eareg1:	3;
};

/* Fill in a 'mode' and 'reg' */
/* move SR, move CCR, nbcd, pea, tas, jsr, jmp */
struct pwfmt13 {
	u_int16	fmt13_cf1:	10;
	u_int16	fmt13_eamode:	3;
	u_int16	fmt13_eareg:	3;
};

/* chk */
struct pwfmt14 {
	u_int16	fmt14_cf1:	4;
	u_int16	fmt14_dreg:	3;
	u_int16	fmt14_size:	3;
	u_int16	fmt14_eamode:	3;
	u_int16	fmt15_eareg:	3;
};

/* lea */
struct pwfmt15 {
	u_int16	fmt15_cf1:	4;
	u_int16	fmt15_areg:	3;
	u_int16	fmt15_cf2:	3;
	u_int16	fmt15_eamode:	3;
	u_int16	fmt15_eareg:	3;
};

/* Fill in: reg# */
/* swap, unlk, move USP */
/* link */
struct pwfmt16 {
	u_int16	fmt16_cf1:	13;
	u_int16	fmt16_reg:	3;
};

/* movem */
struct pwfmt17 { /* 2 words */
	u_int16	fmt17_cf1:	5;
	u_int16	fmt17_direction:1;
	u_int16	fmt17_cf2:	3;
	u_int16	fmt17_size:	1;
	u_int16	fmt17_eamode:	3;
	u_int16	fmt17_eareg:	3;
	/* word 2 */
	u_int16	fmt17_d16:	16;
};

/* bkpt */
struct pwfmt18 {
	u_int16	fmt18_cf1:	13;
	u_int16	fmt18_d3:	3;
};

/* ext */
struct pwfmt19 {
	u_int16	fmt19_cf1:	7;
	u_int16	fmt19_type:	3;
	u_int16	fmt19_cf2:	3;
	u_int16	fmt19_dreg:	3;
};


/* muls, mulu, divs divu -- long forms */
struct pwfmt20 { /* two words */
	u_int16	fmt20_cf1:	10;
	u_int16	fmt20_eamode:	3;
	u_int16	fmt20_eareg:	3;
	u_int16	fmt20_cf2:	1;	/* always 0 */
	u_int16	fmt20_dreg1:	3;
	u_int16	fmt20_signed:	1;
	u_int16	fmt20_size:	1;
	u_int16	fmt20_cf3:	7;	/* always 0 */
	u_int16	fmt20_dreg2:	3;
};

/* trap #vector */
struct pwfmt21 {
	u_int16	fmt21_cf1:	12;
	u_int16	fmt21_d4:	4;
};

/* movec */
struct pwfmt22 { /* two words */
	u_int16	fmt22_cf1:	15;
	u_int16	fmt22_direction:1;
	u_int16	fmt22_rtype:	1;
	u_int16	fmt22_reg:	3;
	u_int16	fmt22_creg:	12;
};

/* addq, subq */
struct pwfmt23 {
	u_int16	fmt23_cf1:	4;
	u_int16	fmt23_d3:	3;
	u_int16	fmt23_type:	1;
	u_int16	fmt23_size:	2;
	u_int16	fmt23_eamode:	3;
	u_int16	fmt23_eareg:	3;
};

/* Scc */
struct pwfmt24 {
	u_int16	fmt24_cf1:	4;
	u_int16	fmt24_condition:4;
	u_int16	fmt24_cf2:	2;
	u_int16	fmt24_eamode:	3;
	u_int16	fmt24_eareg:	3;
};

/* DBcc, TRAPcc */
struct pwfmt25 {
	u_int16	fmt25_cf1:	4;
	u_int16	fmt25_condition:4;
	u_int16	fmt25_cf2:	5;
	u_int16	fmt25_d3:	3;
};

/* Bcc, BSR, BRA */
struct pwfmt26 {
	u_int16	fmt26_cf1:	4;
	u_int16	fmt26_condition:4;
	u_int16	fmt26_d8:	8;
};

/* moveq */
struct pwfmt27 {
	u_int16	fmt27_cf1:	4;
	u_int16	fmt27_dreg:	3;
	u_int16	fmt27_cf2:	1;
	u_int16	fmt27_d8:	8;
};

/* standard arithmetic op format */
/* or, sub, and, add, cmp, eor  */
struct pwfmt28 {
	u_int16	fmt28_cf1:	4;
	u_int16	fmt28_dreg:	3;
	u_int16	fmt28_direction:1;
	u_int16	fmt28_size:	2;
	u_int16	fmt28_eamode:	3;
	u_int16	fmt28_eareg:	3;
};

/* divs, divu --  short form */
/* muls, mulu --  short form */
struct pwfmt29 {
	u_int16	fmt29_cf1:	4;
	u_int16	fmt29_dreg:	3;
	u_int16	fmt29_type:	1;
	u_int16	fmt29_cf2:	2;
	u_int16	fmt29_eamode:	3;
	u_int16	fmt29_eareg:	3;
};

/* BCD instructions: abcd, sbcd, pack, unpk */
struct pwfmt30 {
	u_int16	fmt30_cf1:	4;
	u_int16	fmt30_dstreg:	3;
	u_int16	fmt30_cf2:	5;
	u_int16	fmt30_type:	1;
	u_int16	fmt30_srcreg:	3;
};

/* subx, addx, cmpm */
struct pwfmt31 {
	u_int16	fmt31_cf1:	4;
	u_int16	fmt31_dstreg:	3;
	u_int16	fmt31_cf2:	1;
	u_int16	fmt31_size:	2;
	u_int16	fmt31_cf3:	2;
	u_int16	fmt31_type:	1;
	u_int16	fmt31_srcreg:	3;
};

/* exg */
struct pwfmt32 {
	u_int16	fmt32_cp1:	4;
	u_int16	fmt32_regx:	3;
	u_int16	fmt32_cp2:	1;
	u_int16	fmt32_opmode:	5;
	u_int16	fmt32_regy:	3;
};

/* shift/rotate register */
struct pwfmt33 {
	u_int16	fmt33_cf1:	4;
	u_int16	fmt33_cnt:	3;
	u_int16	fmt33_direction:1;
	u_int16	fmt33_size:	2;
	u_int16	fmt33_cnttype:	1;
	u_int16	fmt33_optype:	2;
	u_int16	fmt33_dreg:	3;
};

/* shift/rotate memory */
struct pwfmt34 {
	u_int16	fmt34_cf1:	7;
	u_int16	fmt34_direction:1;
	u_int16	fmt34_cf2:	2;
	u_int16	fmt34_eamode:	3;
	u_int16	fmt34_eareg:	3;
};

/* Bit field instructions */
/* bftst, bfextu, bfchg, bfexts, bfclr, bfffo, bfset, bfins */
struct pwfmt35 { /* two words */
	u_int16	fmt35_cf1:	10;
	u_int16	fmt35_eamode:	3;
	u_int16	fmt35_eareg:	3;
	u_int16	fmt35_cf2:	1;
	u_int16	fmt35_dreg:	3;
	u_int16	fmt35_offtype:	1;
	u_int16	fmt35_offset:	5;
	u_int16	fmt35_wdtype:	1;
	u_int16	fmt35_width:	5;
};

/* address arithmetic op format */
/* suba, adda, cmpa */
struct pwfmt36 {
	u_int16	fmt36_cf1:	4;
	u_int16	fmt36_areg:	3;
	u_int16	fmt36_opmode:	3;
	u_int16	fmt36_eamode:	3;
	u_int16	fmt36_eareg:	3;
};

/* COPROCESSORS */
/* Formats for "generic" coprocessor instructions */
/* generic operation word */
struct cpfmt0 {
	u_int16	cpfmt0_cf1:	4;
	u_int16	cpfmt0_cpid:	3;
	u_int16	cpfmt0_cptype:	3;
	u_int16	cpfmt0_tdep:	6;
};

/* cpBcc */
struct cpfmt1 {
	u_int16	cpfmt1_cf1:	4;
	u_int16	cpfmt1_cpid:	3;
	u_int16	cpfmt1_cf2:	2;
	u_int16	cpfmt1_size:	1;
	u_int16	cpfmt1_condition:6;
};

/* cpDBcc */
struct cpfmt2 { /* two words */
	u_int16	cpfmt2_cf1:	4;
	u_int16	cpfmt2_cpid:	3;
	u_int16	cpfmt2_cf2:	6;
	u_int16	cpfmt2_dreg:	3;
	u_int16	cpfmt2_cf3:	9;
	u_int16	cpfmt2_cond:	6;
};
/* cpRESTORE, cpSAVE */
struct cpfmt3 { /* one word */
	u_int16	cpfmt3_cf1:	4;
	u_int16	cpfmt3_cpid:	3;
	u_int16	cpfmt3_cf2:	3;
	u_int16	cpfmt3_eamode:	3;
	u_int16	cpfmt3_eareg:	3;
};

/* cpScc */
struct cpfmt4 { /* two words */
	u_int16	cpfmt4_cf1:	4;
	u_int16	cpfmt4_cpid:	3;
	u_int16	cpfmt4_cf2:	3;
	u_int16	cpfmt4_eamode:	3;
	u_int16	cpfmt4_eareg:	3;
	u_int16	cpfmt4_cf3:	10;
	u_int16	cpfmt4_cond:	6;
};

/* cpTcc, cpTPcc, cpTRAPcc */
struct cpfmt5 { /* two words */
	u_int16	cpfmt5_cf1:	4;
	u_int16	cpfmt5_cpid:	3;
	u_int16	cpfmt5_cf2:	3;
	u_int16	cpfmt5_cf3:	3;
	u_int16	cpfmt5_mode:	3;
	u_int16	cpfmt5_cf4:	10;
	u_int16	cpfmt5_cond:	6;
};


/* Formats for 68881 */

/* fsincos */
struct fpfmt1 { /* two words */
	u_int16	fpfmt1_cf1:	4;
	u_int16	fpfmt1_fpid:	3;
	u_int16	fpfmt1_cf2:	3;
	u_int16	fpfmt1_eamode:	3;
	u_int16	fpfmt1_eareg:	3;
	u_int16	fpfmt1_cf3:	3;
	u_int16	fpfmt1_src:	3;
	u_int16	fpfmt1_fpsin:	3;
	u_int16	fpfmt1_cf4:	4;
	u_int16	fpfmt1_fpcos:	3;
};

/* 68881 op's with 1 <ea>, 1 FP-reg */
struct fpfmt2 { /* two words */
	u_int16	fpfmt2_cf1:	4;
	u_int16	fpfmt2_fpid:	3;
	u_int16	fpfmt2_cf2:	3;
	u_int16	fpfmt2_eamode:	3;
	u_int16	fpfmt2_eareg:	3;
	u_int16	fpfmt2_cf3:	3;
	u_int16	fpfmt2_easize:	3;
	u_int16	fpfmt2_fpreg:	3;
	u_int16	fpfmt2_cf4:	7;
};

/* 68881 op's with 2 FP-reg's */
struct fpfmt3 { /* two words */
	u_int16	fpfmt3_cf1:	4;
	u_int16	fpfmt3_fpid:	3;
	u_int16	fpfmt3_cf2:	9;
	u_int16	fpfmt3_cf3:	3;
	u_int16	fpfmt3_fpreg1:	3;
	u_int16	fpfmt3_fpreg2:	3;
	u_int16	fpfmt3_cf4:	7;
};

/*  fmov to <ea>{k-factor} */
struct fpfmt4 { /* two words */
	u_int16	fpfmt4_cf1:	4;
	u_int16	fpfmt4_fpid:	3;
	u_int16	fpfmt4_cf2:	3;
	u_int16	fpfmt4_eamode:	3;
	u_int16	fpfmt4_eareg:	3;
	u_int16	fpfmt4_cf3:	3;
	u_int16	fpfmt4_dstfmt:	3;
	u_int16	fpfmt4_fpreg:	3;
	u_int16	fpfmt4_kval:	7;
};

/* fmov  FPCREG 
 * fmovm FPCREG
 */
struct fpfmt5 { /* two words */
	u_int16	fpfmt5_cf1:	4;
	u_int16	fpfmt5_fpid:	3;
	u_int16	fpfmt5_cf2:	3;
	u_int16	fpfmt5_eamode:	3;
	u_int16	fpfmt5_eareg:	3;
	u_int16	fpfmt5_cf3:	3;
	u_int16	fpfmt5_crlist:	3;
	u_int16	fpfmt5_cf4:	10;
};

/* fmovcr */
struct fpfmt6 { /* two words */
	u_int16	fpfmt6_cf1:	4;
	u_int16	fpfmt6_fpid:	3;
	u_int16	fpfmt6_cf2:	9;
	u_int16	fpfmt6_cf3:	6;
	u_int16	fpfmt6_fpreg:	3;
	u_int16	fpfmt6_romoff:	7;
};

/* fmovm */
struct fpfmt7 { /* two words */
	u_int16	fpfmt7_cf1:	4;
	u_int16	fpfmt7_fpid:	3;
	u_int16	fpfmt7_cf2:	3;
	u_int16	fpfmt7_eamode:	3;
	u_int16	fpfmt7_eareg:	3;
	u_int16	fpfmt7_cf3:	3;
	u_int16	fpfmt7_amodeflag:1;
	u_int16	fpfmt7_dynflag:	1;
	u_int16	fpfmt7_cf4:	3;
	u_int16	fpfmt7_reglist:	8;
};

 struct fmtbfs {
	u_int16	bfs_cf1:	4;
	u_int16	bfs_offtype:	1;
	u_int16	bfs_offset:	5;
	u_int16	bfs_wdtype:	1;
	u_int16	bfs_width:	5;
};

/* Formats for 68851 */

/* pflush, pflusha, pflushs */
struct pmufmt1 { /* two words */
	u_int16	pmufmt1_cf1:	4;
	u_int16	pmufmt1_pmuid:	3;
	u_int16	pmufmt1_cf2:	3;
	u_int16	pmufmt1_eamode:	3;
	u_int16	pmufmt1_eareg:	3;
	u_int16	pmufmt1_cf3:	3;
	u_int16	pmufmt1_mode:	3;
	u_int16	pmufmt1_cf4:	1;
	u_int16	pmufmt1_mask:	4;
	u_int16	pmufmt1_fc:	5;
};

#ifdef OFORTY
/* pflush, ptest */
/* one word pmu instructions */
struct pmufmt2 { /* two words */
	u_int16 pmufmt2_inst:	11;
	u_int16 pmufmt2_opmode:	2;
	u_int16 pmufmt2_reg:	3;
};
#else
/* pflushr */
/* two words, second word constant */
struct pmufmt2 { /* two words */
	u_int16	pmufmt2_cf1:	4;
	u_int16	pmufmt2_pmuid:	3;
	u_int16	pmufmt2_cf2:	3;
	u_int16	pmufmt2_eamode:	3;
	u_int16	pmufmt2_eareg:	3;
	u_int16	pmufmt2_cf3:	16;
};
#endif

/* pload */
struct pmufmt3 { /* two words */
	u_int16	pmufmt3_cf1:	4;
	u_int16	pmufmt3_pmuid:	3;
	u_int16	pmufmt3_cf2:	3;
	u_int16	pmufmt3_eamode:	3;
	u_int16	pmufmt3_eareg:	3;
	u_int16	pmufmt3_cf3:	11;
	u_int16	pmufmt3_fc:	5;
};

/* pmove - format 1 */
struct pmufmt4 { /* two words */
	u_int16	pmufmt4_cf1:	4;
	u_int16	pmufmt4_pmuid:	3;
	u_int16	pmufmt4_cf2:	3;
	u_int16	pmufmt4_eamode:	3;
	u_int16	pmufmt4_eareg:	3;
	u_int16	pmufmt4_cf3:	3;
	u_int16	pmufmt4_preg:	3;
	u_int16	pmufmt4_cf4:	10;
};

/* pmove - format 2 */
struct pmufmt5 { /* two words */
	u_int16	pmufmt5_cf1:	4;
	u_int16	pmufmt5_pmuid:	3;
	u_int16	pmufmt5_cf2:	3;
	u_int16	pmufmt5_eamode:	3;
	u_int16	pmufmt5_eareg:	3;
	u_int16	pmufmt5_cf3:	3;
	u_int16	pmufmt5_preg:	3;
	u_int16	pmufmt5_cf4:	5;
	u_int16	pmufmt5_num:	3;
	u_int16	pmufmt5_cf5:	2;
};

/* ptest  */
struct pmufmt6 { /* two words */
	u_int16	pmufmt6_cf1:	4;
	u_int16	pmufmt6_pmuid:	3;
	u_int16	pmufmt6_cf2:	3;
	u_int16	pmufmt6_eamode:	3;
	u_int16	pmufmt6_eareg:	3;
	u_int16	pmufmt6_cf3:	3;
	u_int16	pmufmt6_level:	3;
	u_int16	pmufmt6_cf4:	1;
	u_int16	pmufmt6_aregbit:	1;
	u_int16	pmufmt6_aregno:	3;
	u_int16	pmufmt6_fc:	5;
};

/* pvalid  */
struct pmufmt7 { /* two words */
	u_int16	pmufmt7_cf1:	4;
	u_int16	pmufmt7_pmuid:	3;
	u_int16	pmufmt7_cf2:	3;
	u_int16	pmufmt7_eamode:	3;
	u_int16	pmufmt7_eareg:	3;
	u_int16	pmufmt7_cf3:	13;
	u_int16	pmufmt7_areg:	3;
};

union iwfmt {
  short  	ifmtword0;
  struct pwgeneric ifmtgeneric;
  struct pwfmt1 ifmt1;
  struct pwfmt2 ifmt2;
  struct pwfmt3 ifmt3;
  struct pwfmt4 ifmt4;
  struct pwfmt5 ifmt5;
  struct pwfmt6 ifmt6;
  struct pwfmt7 ifmt7;
  struct pwfmt8 ifmt8;
  struct pwfmt9 ifmt9;
  struct pwfmt10 ifmt10;
  struct pwfmt11 ifmt11;
  struct pwfmt12 ifmt12;
  struct pwfmt13 ifmt13;
  struct pwfmt14 ifmt14;
  struct pwfmt15 ifmt15;
  struct pwfmt16 ifmt16;
  struct pwfmt17 ifmt17;
  struct pwfmt18 ifmt18;
  struct pwfmt19 ifmt19;
  struct pwfmt20 ifmt20;
  struct pwfmt21 ifmt21;
  struct pwfmt22 ifmt22;
  struct pwfmt23 ifmt23;
  struct pwfmt24 ifmt24;
  struct pwfmt25 ifmt25;
  struct pwfmt26 ifmt26;
  struct pwfmt27 ifmt27;
  struct pwfmt28 ifmt28;
  struct pwfmt29 ifmt29;
  struct pwfmt30 ifmt30;
  struct pwfmt31 ifmt31;
  struct pwfmt32 ifmt32;
  struct pwfmt33 ifmt33;
  struct pwfmt34 ifmt34;
  struct pwfmt35 ifmt35;
  struct pwfmt36 ifmt36;

  struct cpfmt0  icpfmt0;
  struct cpfmt1  icpfmt1;
  struct cpfmt2  icpfmt2;
  struct cpfmt3  icpfmt3;
  struct cpfmt4  icpfmt4;
  struct cpfmt5  icpfmt5;

  struct fpfmt1  ifpfmt1;
  struct fpfmt2  ifpfmt2;
  struct fpfmt3  ifpfmt3;
  struct fpfmt4  ifpfmt4;
  struct fpfmt5  ifpfmt5;
  struct fpfmt6  ifpfmt6;
  struct fpfmt7  ifpfmt7;

  struct pmufmt1  ipmufmt1;
  struct pmufmt2  ipmufmt2;
  struct pmufmt3  ipmufmt3;
  struct pmufmt4  ipmufmt4;
  struct pmufmt5  ipmufmt5;
  struct pmufmt6  ipmufmt6;
  struct pmufmt7  ipmufmt7;
};
 
