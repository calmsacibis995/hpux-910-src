/* @(#) $Revision: 70.1 $ */   
/*
 * Instruction Scheduler Include File
 */

#include "o68.h"

#define byte		unsigned char

#define MIN(X,Y)       ((X) < (Y) ? (X) : (Y))


#define MAX(X,Y)       ((X) < (Y) ? (Y) : (X))


#define isUNIOP(op)    ((op == CLR)  || (op == EXT) || \
                        (op == EXTB) || (op == NEG) || \
		        (op == NOT)  || (op == PEA) || \
		        (op == SWAP) || (op == TST) || \
		        ((op >= SCC)  && (op <= ST)) || \
		        (op == SVC)  || (op == SVS) || \
		        (op == BFCHG) || (op == BFCLR) || \
			(op == BFSET) || (op == BFTST) || \
			(op == FTEST))
 

#define maybeUNIOP(op) ((!isFOP(op) && \
                       ((op == ASL)    || (op == ASR)   || \
                        (op == LSL)    || (op == LSR)   || \
                        (op == ROL)    || (op == ROR))) || \
                       (isFOP(op) && \
                       ((op == FABS)    || (op == FACOS)   || \
                        (op == FASIN)   || (op == FATAN)   || \
                        (op == FCOS)    || (op == FSIN)    || \
                        (op == FCOSH)   || (op == FETOX)   || \
                        (op == FLOGN)   || (op == FLOG10)  || \
		        (op == FSINH)   || (op == FSQRT)   || \
		        (op == FTAN)    || (op == FTANH)   || \
			(op == FNEG)    || (op == FINTRZ))))

#define isNotOn40(op)  ((op == FACOS)   || (op == FASIN)  || \
		        (op == FATAN)   || (op == FMOD)   || \
		        (op == FCOS)    || (op == FCOSH)  || \
		        (op == FETOX)   || (op == FINTRZ) || \
		        (op == FLOG10)  || (op == FLOGN)  || \
		        (op == FSIN)    || (op == FSINH)  || \
		        (op == FTAN)    || (op == FTANH)  || \
		        (op == FSGLDIV) || (op == FSGLMUL))

#define isBBEND(op)    ((op == CBR)   || (op == JMP)    || \
		        (op == JSR)   || (op == RTS)    || \
		        (op == MOVEM) || (op == FMOVM)  || \
		        (op == LINK)  || (op == UNLK)   || \
		        ((FBEQ <= op) && (op <= FBNLT)) || \
		        ((SCC <= op)  && (op <= ST))    || \
		        (op == SVC)   || (op == SVS)    || \
		        ((DBCC <= op) && (op <= DBVS))  || \
			(oforty && isNotOn40(op)))

#define isFLOP(op)     (isFOP(op) && \
		        ((op < FBEQ) || (op > FBNLT)) && \
		         (op != FMOV) && (op != FMOVM))


#define isFLOAD(op,dst) ((op == FMOV) && ((dst)->mode == FREG))


#define isFSTORE(op,dst) ((op == FMOV) && ((dst)->mode != FREG))


#define isALOAD(dst)  ((dst)->mode == ADIR)


#define isAIND(mode)     ((mode >=  IND) && (mode <= PINDBASE) && \
			  (mode != PDISP))


#define anyMEM(arg)    ((IND <= arg->mode) && (arg->mode <= ADISP) \
		          ? (arg->reg < reg_a6) \
		 	 : ((AINDEX <= arg->mode) && (arg->mode <= AINDBASE)))


#define isSLOWTAIL(op) ((op == FADD) || (op == FCMP) || \
		        (op == FDIV) || (op == FMOD) || \
		        (op == FMUL) || (op == FSGLDIV) || \
		        (op == FSUB) || (op == FSGLMUL))

#define isiCCBRANCH(op) ((op == CBR) || ((DBCC <= op) && (op <= DBVS)) || \
		         ((SCC <= op)  && (op <= ST)))

#define isfCCBRANCH(op) ((FBEQ <= op) && (op <= FBNLT))

#define setsiCC(i) (((i->op != ADD && i->op != SUB && i->op != MOVE) || \
		     i->mode2 != ADIR) && \
		    i->op != SUBA && i->op != ADDA && \
		    i->op != PEA && i->op != LEA && \
		    (i->op <= FLOW || i->op >= FHIGH))

#define setsfCC(i) (i->op >= FLOW && i->op <= FHIGH)

#define movDLT(p)  (pic && p->op == MOVE && p->mode1 == IMMEDIATE && \
	            p->type1 == STRING && !strcmp(p->string1,"DLT") && \
	            p->mode2 == ADIR)


#define noCC	    0
#define iCC	    1
#define fCC	    2

#define maxBB	  200

#define iFLOP       1
#define iFLOAD      2
#define iFSTORE     3
#define iIOP        4
#define iALOAD      5

#define lckNULL     0
#define lckVANILLA  1
#define lckAR       2
#define lckFPR      3


#define NoCvrt      0
#define IntFlt      1
#define FltInt      2

typedef struct {
	unsigned char	 head;
	unsigned char	 body;
	unsigned char	 tail;
} tim30;

typedef struct {
	byte		 eac;
	byte		 eac_gets_xu;
	byte		 eaf;
	byte		 ixu;
	byte		 fcu;
	byte		 fxu;
	byte		 fnu;
	byte		 cvrt;
} tim40;

typedef struct dags {
	node		 *inst;
	struct dags	 *rootn;
	struct dags	 *rootp;
	struct dags	 *annotate;
	struct dags	 *candidate;
	byte		  insttype;
	byte		  parents;
	byte		  rootparents;
	byte		  cc;
	byte		  printflag;
	unsigned short	  level;
	unsigned short	  number;
	unsigned short	  kids;
	unsigned short	  maxkids;
	struct dags	**kid;
	byte		 *kidintrlck;
	union {
		tim30	  t30;
		tim40	  t40;
	}		  time;
} dag;


typedef struct {
        int clock;
        struct {
                dag  *inst;
                byte  clks;
        } eac;
        struct {
                dag  *inst;
                byte  clks;
        } eaf;
        struct {
                dag  *inst;
                byte  clks;
        } ixu;
        struct {
                dag  *cu_inst;
                byte  cu_clks;
                dag  *xu_inst;
                byte  xu_clks;
                dag  *nu_inst;
                byte  nu_clks;
        } fxu;
} o40pipe;
