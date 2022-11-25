/* @(#) $Revision: 70.1 $ */     
#include <stdio.h>
#include <ctype.h>
#include "opcodes.h"

/*
 * Header for object code improver
 */

typedef	char	boolean;
#define false	(0)
#define true	(1)
#define maybe	(2)

#define first_c2_label 9000000

#ifndef DEBUG
#define bugout(str)
#define bugdump(x)
#endif  DEBUG

#define SAVE_TST		/* save a tst.b after a link for os purposes */

/*
 * Values for subop for most operands
 *
 * If the instruction doesn't specify a size, then subop==0
 */
#define UNSIZED 0
#define	BYTE	1
#define WORD	2
#define LONG	3

/* Sizes for 68881 instructions */
#define SINGLE		21
#define	DOUBLE		22
#define EXTENDED	23

/*
 * Values for subop in case of conditional branches
 *
 * The conditions are paired (4,5) (6,7) etc., so to reverse the condition
 * just flip the low-order bit!
 */

#define REVERSE_CONDITION(cond) ((cond)^01)

/* These values must not collide with BYTE, WORD, LONG, or UNSIZED! */
#define COND_CC	4
#define COND_CS	5
#define COND_EQ	6
#define COND_NE	7
#define COND_GE	8
#define COND_LT	9
#define COND_GT	10
#define COND_LE	11
#define COND_HI	12
#define COND_LS	13
#define COND_MI	14
#define COND_PL	15
#define COND_VC	16
#define COND_VS	17
#define COND_T	18
#define COND_F	19

/* and some synonyms */
#define COND_HS	COND_CC
#define COND_LO	COND_CS
#define COND_ZR COND_EQ
#define COND_NZ COND_NE




typedef struct argument {

#define UNKNOWN	0
#define OTHER	1
#define	DDIR	2		/* %d7				*/
#define	ADIR	3		/* %a0 or %sp			*/
#define	IND	4		/* (%a0)			*/
#define	INC	5		/* (%a0)+			*/
#define	DEC	6		/* -(%a0)			*/
#define	ADISP	7		/* 34(%a0)			*/
#define	AINDEX	8		/* 34(%a0,%d3) or 34(%a0,%a2)	*/
#define AINDBASE 9		/* (bd,%an,%rn.size*scale)	*/
#define	PDISP	10		/* 34(%pc)			*/
#define	PINDEX	11		/* 34(%pc,%d3) or 34(%pc,%a2)	*/
#define PINDBASE 12		/* (bd,%pc,%rn.size*scale)	*/
#define	ABS_W	13		/* 123				*/
#define	ABS_L	14		/* L1234			*/
#define	IMMEDIATE 15		/* &24 or &L1234		*/
#define	DPAIR	16		/* %dn:%dm used in div & mul 	*/
#define FREG	17		/* %fp3  			*/

#ifdef DRAGON
#define FPREG	18		/* %fpa14  			*/
#define FPREGS	19		/* %fpa14 ,%fpa13 (in fpmOP)    */
#endif DRAGON

#ifdef PREPOST
#define MEMPRE 	20		/* ([bd,%an,%rn.size*scale],od) */
#define MEMPOST	21		/* ([bd,%an],%rn.size*scale,od) */
#define PCPRE 	22		/* ([bd,%pc,%rn.size*scale],od) */
#define PCPOST	23		/* ([bd,%pc],%rn.size*scale,od) */
#endif PREPOST

	char	mode;		/* addressing mode or UNKNOWN */
	char	reg;		/* Dn:0-7  An:8-15 */
	char	index;		/* Dn:0-7  An:8-15 */
	boolean	word_index;	/* is index word or long? */
#ifdef M68020
	char 	scale;		/* scale factor for index reg */
#endif

/* UNKNOWN is defined as 0 */
#define	INTLAB	1		/* an integer label		*/
#define INTVAL	2		/* a plain old number		*/
#define STRING  3		/* something else in string	*/
	char	type;		/* value type or UNKNOWN */
	union {
		int	u_addr;	    /* integer value of thing	*/
		int	u_labno;    /* 234 for L234		*/
		char	*u_string;  /* same as addr only a string	*/
	} sub_arg;
#ifdef PREPOST
	char	od_type;		/* type for outer disp */
	union {
		int	od_addr;	/* integer value of thing	*/
		int	od_labno;	/* 234 for L234		*/
		char	*od_string;	/* same as addr only a string	*/
	} od_arg;
#endif PREPOST
#ifdef VOLATILE
#define	V_ATTR	1
#define	isvolatile(arg)	(((arg)->attributes)&V_ATTR)
	unsigned short attributes;
#endif
} argument;

typedef struct node {
	short	op;			/* the op code, e.g., MOVE or UKNOWN */
					/* if LABEL then op1.labno is number */
					/* if DLABEL, op1.string is label */
	char	subop;			/* the sub-opcode, e.g., LONG or JEQ */
	char	info;			/* other info */
	struct	node	*forw;		/* pointer to next node */
	struct	node	*back;		/* pointer to previous node */
	struct	node	*ref;		/* pointer to node that we refer to */
					/* used by JMP, CBR, etc. */
	int	refc;			/* reference count if we're a label */
#ifdef M68020
	char 	offset;			/* for bit fld instructions */
	char 	width;			/* for bit fld instructions */
#endif
	argument arg[2];		/* our two arguments/operands */
} node;

#define op1	arg[0]			/* synonym for first argument */
#define op2	arg[1]			/* synonym for second argument */

#define mode1	op1.mode
#define mode2	op2.mode

#define reg1	op1.reg
#define reg2	op2.reg

#define index1	op1.index
#define index2	op2.index

#define type1	op1.type
#define type2	op2.type

#define labno	sub_arg.u_labno
#define labno1	op1.labno
#define labno2	op2.labno

#define	addr	sub_arg.u_addr
#define addr1	op1.addr
#define addr2	op2.addr

#define	string	sub_arg.u_string
#define string1	op1.string
#define string2	op2.string

#ifdef PREPOST
#define odlabno	od_arg.od_labno
#define odaddr	od_arg.od_addr
#define odstring	od_arg.od_string
#endif PREPOST

#define FORW(p) ((p)=(p)->forw)
#define BACK(p) ((p)=(p)->back)

struct op_info {
	char	*opstring;
	int	opcode;
};

char	input_line[1024];
node	first;
int	nbrbr;
int	nsaddr;
int	redunm;
int	iaftbr;
int	njp1;
int	nrlab;
int	nxjump;
int	ncmot;
int	nrevbr;
int	loopiv;
int	nredunj;
int	nskip;
int	ncomj;
int	nrtst;

boolean oforty;			/* are we doing inst sched for the 040? */
boolean inst_sched;		/* instruction scheduling */
boolean debug_is;		/* print stuff related to inst scheduling */
boolean worthless_is;		/* do inst sched even on worthless blocks */
boolean junk_is;		/* don't do any other opt's just is */
boolean pass_unchanged;		/* disable *all* optimizations */
boolean eliminate_link_unlk;	/* Remove link/unlk instructions if possible */
boolean leaf_movm_reduce;       /* replace perm regs by scratch */
boolean memory_is_ok;		/* We can assume the memory is ordinary */
				/* If false, we may have I/O locations  */
boolean	bit_test_ok;		/* Generate bit test instructions */
boolean exit_exits;		/* Assume that the routine 'exit' exits */
boolean print_statistics;	/* Print optimization stats at end of run */
boolean flow_opts_ok;		/* All flow optimization */
boolean simplify_addressing;	/* Simplified addressing */
boolean code_movement_ok;	/* Code motion */
boolean invert_loops;		/* Loops inverted */
boolean data_to_bss;		/* move zero data to bss section */
boolean allow_asm;		/* optimize procedure even if it has asm's */
boolean dragon;			/* dragon source */
boolean dragon_and_881;		/* dragon/881 source from +b option */
boolean fort;			/* source came from fortran compiler */
boolean span_dep_opt;		/* Attempt span dependent optimizations */
boolean unsafe_opts;		/* Attempt unsafe optimizations */

/* temp flags ?? */
boolean ult_881;
boolean ult_dragon;
boolean tail_881;
boolean tail_dragon;
boolean combine_dragon;

#ifdef PIC
boolean pic;
#endif

#ifdef DEBUG
boolean debug;
boolean debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
#endif DEBUG
boolean change_occured;

#define CACHE_SIZE 100
node cache[CACHE_SIZE+1];		/* 0..CACHE_SIZE */
node *cache_end;
argument ccloc;
int      cctyp;

argument aregs[8];			/* to store addresses moved into 
					 * aregs using LEA instructions */

#define in(min,what,max) ((min)<=(what) && (what)<=(max))
#define equstr(a,b) (strcmp((a),(b))==0)

/* the registers */
#define reg_d0	(0)
#define reg_d1	(1)
#define reg_d2	(2)
#define reg_d3	(3)
#define reg_d4	(4)
#define reg_d5	(5)
#define reg_d6	(6)
#define reg_d7	(7)
#define reg_a0	(8)
#define reg_a1	(9)
#define reg_a2 	(10)
#define reg_a3 	(11)
#define reg_a4	(12)
#define reg_a5	(13)
#define reg_a6	(14)
#define reg_a7	(15)
#define reg_sp	(reg_a7)
#define reg_pc	(-2)
#define reg_za0	(-3)
#define reg_fp0 (16)
#define reg_fp7 (23)
#ifdef DRAGON
#define reg_fpa0 (24)
#define reg_fpa15 (39)
#endif DRAGON

/*
 * part of the cache stuff, but needs to be
 * after the definition of reg_fp7
 */
unsigned char fpsize[reg_fp7+1]; /* size of result in fp register */

#define isD(r)	((r)>=reg_d0 && (r)<=reg_d7)
#define isA(r)	((r)>=reg_a0 && (r)<=reg_a7)
#define isF(r)	((r)>=reg_fp0 && (r)<=reg_fp7)
#ifdef DRAGON
#define isFP(r)	((r)>=reg_fpa0 && (r)<=reg_fpa15)
#endif DRAGON
#define isPC(r)	((r)==reg_pc)
#define isSP(r)	((r)==reg_sp)
#define isAD(r)	((r)>=reg_d0 && (r)<=reg_a7)

/* status bits for data_simplify */
#define AREG_OK		(1<<0)
#define QUICK_THREE_BIT	(1<<1)
#define	QUICK_EIGHT_BIT	(1<<2)
#define CONST_OK	(1<<3)
#define ZERO_PREFERRED  (1<<4)

/*
 * Tests for fitting into a various bit sizes
 *
 * MOVEQ uses an eight bit quick constant.
 * ADDQ uses a three bit quick constant.
 */

#define THREE_BITTABLE(n) (1<=(n) && (n)<=8)
#define EIGHT_BITTABLE(n) (-128<=(n) && (n)<=127)
#define SIXTEEN_BITTABLE(n) (-32768<=(n) && (n)<=32767)

/* Test for 68881 opcodes */
#define isFOP(x) (x>= FLOW && x<=FHIGH)

#ifdef DRAGON
/* Test for dragon opcodes */
#define isFPOP(x) (x>= FPLOW && x<=FPHIGH)
#endif DRAGON

/* test for mode being a register */
#ifndef M68020
#define isREGMODE(x) (x==DDIR || x==ADIR)
#else
#ifndef DRAGON
#define isREGMODE(x) (x==DDIR || x==ADIR || x==FREG)
#else
#define isREGMODE(x) (x==DDIR || x==ADIR || x==FREG || x==FPREG)
#endif DRAGON
#endif M68020
