/* @(#) $Revision: 70.1 $ */      

# include "symbols.h"
# include "adrmode.h"
# include "asgram.h"
# include "bitmask.h"
# include "verify.h"
# include "opfmts.h"
# include "ivalues.h"
# include "icode.h"
# include <stdio.h>

# ifdef SEQ68K
# include "seq.a.out.h"
# else
# include <a.out.h>
# endif

# include "sdopt.h"
# include "header.h"

extern int fp_cpid;
extern gen_ea_bits();
extern gen_expr();
extern long abs_value();

extern struct instruction instruction;
unsigned int opsize_bits();
extern FILE * fdout;
extern FILE * fdrel;
extern FILE * fdcsect;
extern char * codebuf;
extern int codeindx;

struct ea_bit_template ea;

#ifdef PIC
extern short jump_flag;
#endif

# define SOP0  sop[0]
# define SOP1	sop[1]
# define SOP2	sop[2]
# define SOP3	sop[3]

# define OP0  (&instrp->opaddr[0])
# define OP1  (&instrp->opaddr[1])
# define OP2  (&instrp->opaddr[2])
# define OP3  (&instrp->opaddr[3])
# define OP(i)  (&instrp->opaddr[i])

# define SET_EA_MODE_AND_REG { piw->ifmtgeneric.pwg_eamode = ea.eamode;\
	piw->ifmtgeneric.pwg_eareg = ea.eareg;}

build_instruction_bits(instrp) 
 register struct instruction * instrp;
{
   unsigned long ivalue;
   union iwfmt * piw;
   addrmode *sop[3];

   ivalue = instrp->ivalue;
   if (ivalue >= I_LASTOP)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad ivalue in build_instruction_bits");

   /* integrity check */
   if ( (iopcode[ivalue]==0) && (ivalue!=I_ORI) && (ivalue!=I_MOVE) )
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("no opcode for ivalue");

   piw = (union iwfmt *) &codebuf[codeindx];
   piw->ifmtword0 = iopcode[ivalue];
   codeindx += 2;
   sop[0] = &(instrp->opaddr[0].unoperand.soperand);
   sop[1] = &(instrp->opaddr[1].unoperand.soperand);
   sop[2] = &(instrp->opaddr[2].unoperand.soperand);

   switch(ivalue) {

	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected ivalue in build-instruction-bits");
		break;

#ifdef OFORTY
	/* Note that the scope is already filled in from table */
	case I_CINVL:	case I_CINVP:
	case I_CPUSHL:	case I_CPUSHP:
		/* cache field */
		piw->ifmt1.fmt1_size = instrp->opaddr[0].unoperand.reglist;
		/* register field */
		piw->ifmt1.fmt1_eareg = SOP1->adreg1.regno;
		break;
	case I_CINVA:	case I_CPUSHA:
		/* cache field */
		piw->ifmt1.fmt1_size = instrp->opaddr[0].unoperand.reglist;
		break;

	case I_MOVE16_INC_INC:
		piw->ifmt3.fmt3_eareg = SOP0->adreg1.regno;
		piw->ifmt3.fmt3_daflag = 1;
		piw->ifmt3.fmt3_reg2 = SOP1->adreg1.regno;
		codeindx += 2;
		break;
	case I_MOVE16_INC_ABS:
		piw->ifmt33.fmt33_optype = 0;
		piw->ifmt33.fmt33_dreg = SOP0->adreg1.regno;
		if (SOP1->admode == A_ABS16)
			SOP1->admode = A_ABS32;
		gen_ea_bits(OP1, &ea, instrp, 1);
		break;
	case I_MOVE16_ABS_INC:
		piw->ifmt33.fmt33_optype = 1;
		piw->ifmt33.fmt33_dreg = SOP1->adreg1.regno;
		if (SOP0->admode == A_ABS16)
			SOP0->admode = A_ABS32;
		gen_ea_bits(OP0, &ea, instrp, 0);
		break;
	case I_MOVE16_IND_ABS:
		piw->ifmt33.fmt33_optype = 2;
		piw->ifmt33.fmt33_dreg = SOP0->adreg1.regno;
		if (SOP1->admode == A_ABS16)
			SOP1->admode = A_ABS32;
		gen_ea_bits(OP1, &ea, instrp, 1);
		break;
	case I_MOVE16_ABS_IND:
		piw->ifmt33.fmt33_optype = 3;
		piw->ifmt33.fmt33_dreg = SOP1->adreg1.regno;
		if (SOP0->admode == A_ABS16)
			SOP0->admode = A_ABS32;
		gen_ea_bits(OP0, &ea, instrp, 0);
		break;
#endif

	case I_NEGX:  case I_NEG:
	case I_CLR:   case I_NOT:
	case I_TST:
		/* Format-1 */
		piw->ifmt1.fmt1_size = opsize_bits(ivalue,instrp->operation_size);
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_ADDI:	case I_SUBI:
	case I_ORI:	case I_EORI:
	case I_CMPI:	case I_ANDI:
		{ int imop, eaop;
		  eaop = (ivalue==I_CMPI) ? 0:1;
		  imop = (ivalue==I_CMPI) ? 1:0;
		  /* Format 1 + immediate data */
		  piw->ifmt1.fmt1_size = opsize_bits(ivalue,instrp->operation_size);
		  gen_ea_bits(OP(imop), &ea, instrp, imop);
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

	case I_ILLEGAL:	case I_NOP:
	case I_RESET:
	case I_RTR:
	case I_RTS:	case I_RTE:
	case I_TRAPV:

# ifdef M68020
	case I_TCC:	case I_TCS:	case I_TEQ:
	case I_TF:	case I_TGE:	case I_TGT:
	case I_THI:	case I_TLE:
	case I_TLS:	case I_TLT:
	case I_TMI:	case I_TNE:	case I_TPL:
	case I_TT:	case I_TVC:	case I_TVS:
# endif	/* M68020 */
		/* Format-2 */
		/* entire word specified--no variable fields */
		break;

	case I_ANDI_SR: case I_EORI_SR:
	case I_ORI_SR:
	case I_RTD:
	case I_STOP:
		/* Format-2 */
		/* entire baseword specified, 16-bit immediate data */
		/* ??? should we require absolute here instead ???  */
		gen_imm_data(&SOP0->adexpr1,SZWORD);
		break;

	case I_ANDI_CCR: case I_EORI_CCR:
	case I_ORI_CCR:
		/* Format-2 */
		/* entire baseword specified, byte immediate data */
		/* ??? should we require absolute here instead ???  */
		gen_imm_data(&SOP0->adexpr1,SZBYTE);
		break;

	case I_CMP2:	case I_CHK2:
		/* Format 3 */
		{ int eaop, rop;
		  eaop = (ivalue==I_CHK2) ? 0:1;
		  rop = (ivalue==I_CHK2) ? 1:0;
		  piw->ifmt3.fmt3_size = opsize_bits(ivalue,instrp->operation_size);
		  piw->ifmt3.fmt3_daflag = sop[rop]->adreg1.regkind==DREG ? 0:1;
		  piw->ifmt3.fmt3_reg2 = sop[rop]->adreg1.regno;
		  piw->ifmt3.fmt3_cf3 = (ivalue==I_CMP2) ? 0x000 : 0x800;
		  codeindx += 2;
		  gen_ea_bits(eaop? OP1:OP0, &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

# ifdef M68020
	case I_CAS:
		/* Format 4 */
		piw->ifmt4.fmt4_size = opsize_bits(ivalue, instrp->operation_size);
		piw->ifmt4.fmt4_cf3 =  piw->ifmt4.fmt4_cf4 =  0;
		piw->ifmt4.fmt4_du = SOP1->adreg1.regno;
		piw->ifmt4.fmt4_dc = SOP0->adreg1.regno;
		codeindx += 2;
		gen_ea_bits(OP2, &ea, instrp, 2);
		SET_EA_MODE_AND_REG;
		break;

	case I_CAS2:
		/* Format 5 */
		piw->ifmt5.fmt5_size = opsize_bits(ivalue, instrp->operation_size);
		piw->ifmt5.fmt5_cf3 = piw->ifmt5.fmt5_cf4 = 0;
		piw->ifmt5.fmt5_daflg1 = OP2->unoperand.regpair.rpreg1.regkind==
			DREG ? 0:1;
		piw->ifmt5.fmt5_reg1 = OP2->unoperand.regpair.rpreg1.regno;
		piw->ifmt5.fmt5_du1 = OP1->unoperand.regpair.rpreg1.regno;
		piw->ifmt5.fmt5_dc1 = OP0->unoperand.regpair.rpreg1.regno;

		piw->ifmt5.fmt5_cf5 = piw->ifmt5.fmt5_cf6 = 0;
		piw->ifmt5.fmt5_daflg2 = OP2->unoperand.regpair.rpreg2.regkind==
			DREG ? 0:1;
		piw->ifmt5.fmt5_reg2 = OP2->unoperand.regpair.rpreg2.regno;
		piw->ifmt5.fmt5_du2 = OP1->unoperand.regpair.rpreg2.regno;
		piw->ifmt5.fmt5_dc2 = OP0->unoperand.regpair.rpreg2.regno;
		codeindx += 4;
		break;
# endif	/* M68020 */

# ifdef M68020
	case I_CALLM:
		/* Format 6 */
		gen_imm_data(&(SOP0->adexpr1), SZBYTE);
		gen_ea_bits(OP1,&ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_RTM_AREG:	case I_RTM_DREG:
		/* Format 7 */
		piw->ifmt7.fmt7_reg = SOP0->adreg1.regno;
		break;
# endif	/* M68020 */

	case I_BCHG_REG:	case I_BCLR_REG:
	case I_BSET_REG:	case I_BTST_REG:
		/* Format 8 */
		piw->ifmt8.fmt8_dreg = SOP0->adreg1.regno;
		gen_ea_bits(OP1,&ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_BCHG_IMM:	case I_BCLR_IMM:
	case I_BSET_IMM:	case I_BTST_IMM:
		/* Format 9 */
		gen_imm_data(&(SOP0->adexpr1), SZBYTE);
		gen_ea_bits(OP1,&ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_MOVEP_MEM: case I_MOVEP_REG:
		/* Format 10 */
		{ int dreg, areg;
		  dreg = (ivalue==I_MOVEP_MEM)? 0:1;
		  areg = (ivalue==I_MOVEP_MEM)? 1:0;
		  piw->ifmt10.fmt10_size = (instrp->operation_size==SZLONG);
		  piw->ifmt10.fmt10_dreg = sop[dreg]->adreg1.regno;
		  piw->ifmt10.fmt10_areg = sop[areg]->adreg1.regno;
		  gen_expr(&sop[areg]->adexpr1, SZWORD);
		  break;
		}

	case I_MOVES_MEM:	case I_MOVES_REG:
		/* Format 11 */
		{ int rop, eaop;
		  rop = (ivalue==I_MOVES_MEM)? 0:1;
		  eaop = (ivalue==I_MOVES_MEM)? 1:0;
		  piw->ifmt11.fmt11_size = opsize_bits(ivalue,instrp->operation_size);
		  piw->ifmt11.fmt11_daflag = (sop[rop]->adreg1.regkind==AREG);
		  piw->ifmt11.fmt11_reg2 = sop[rop]->adreg1.regno;
		  piw->ifmt11.fmt11_direction = (ivalue==I_MOVES_MEM);
		  piw->ifmt11.fmt11_cf2 = 0;
		  codeindx += 2;
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

	case I_MOVE:
		/* Format 12 */
		piw->ifmt12.fmt12_size = opsize_bits(ivalue, instrp->operation_size);
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		gen_ea_bits(OP1, &ea, instrp, 1);
		piw->ifmt12.fmt12_eareg2 = ea.eareg;
		piw->ifmt12.fmt12_eamode2 = ea.eamode;
		break;

	case I_MOVEA:
		/* Format 12 */
		piw->ifmt12.fmt12_size = (instrp->operation_size==SZWORD)?
		   3:2;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		piw->ifmt12.fmt12_eareg2 = SOP1->adreg1.regno;
		break;

	case I_NBCD: case I_PEA:
	case I_JMP:	case I_JSR:
	case I_TAS:
		/* Format 13 */
#ifdef PIC
                if ((ivalue == I_JMP) || (ivalue == I_JSR)) {
                   jump_flag = 1;
                }
#endif
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
#ifdef PIC
                jump_flag = 0;
#endif
		break;

	case I_MOVE_to_CCR:
	case I_MOVE_to_SR:
		/* Format 13 */
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_MOVE_from_CCR:
	case I_MOVE_from_SR:
		/* Format 13 */
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_CHK:
		/* Format 14 */
		piw->ifmt14.fmt14_dreg = SOP1->adreg1.regno;
		piw->ifmt14.fmt14_size = (instrp->operation_size==SZWORD)?
			6:4;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_LEA:
		/* Format 15 */
		piw->ifmt15.fmt15_areg = SOP1->adreg1.regno;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_MOVE_to_USP:
	case I_SWAP:
	case I_UNLK:
		/* Format 16 */
		piw->ifmt16.fmt16_reg = SOP0->adreg1.regno;
		break;

	case I_MOVE_from_USP:
		/* Format 16 */
		piw->ifmt16.fmt16_reg = SOP1->adreg1.regno;
		break;

	case I_LINK_WORD:
	case I_LINK_LONG:
		/* Format 16 + imm-data */
		piw->ifmt16.fmt16_reg = SOP0->adreg1.regno;
		gen_ea_bits(OP1, &ea, instrp, 1);
		break;

		/* Format 17 */
		{ int rlop, eaop;
	case I_MOVEM_MEM:
		  rlop = 0; eaop = 1;
		  goto movem;
	case I_MOVEM_REG:
		  rlop = 1; eaop = 0;
		movem:
		  piw->ifmt17.fmt17_size = (instrp->operation_size==SZLONG);
		  gen_expr(&sop[rlop]->adexpr1, SZWORD);
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

# ifdef M68020
	case I_BKPT:
		/* Format 18 */
		piw->ifmt18.fmt18_d3 = abs_value(&SOP0->adexpr1,1,0,7);
		break;
# endif	/* M68020 */

	case I_EXT:
		/* Format 19 */
		piw->ifmt19.fmt19_type = (instrp->operation_size==SZWORD)?
			2 : 3;
		piw->ifmt19.fmt19_dreg = SOP0->adreg1.regno;
		break;

# ifdef M68020
	case I_EXTB:
		/* Format 19 */
		piw->ifmt19.fmt19_type = 7;
		piw->ifmt19.fmt19_dreg = SOP0->adreg1.regno;
		break;

	case I_DIVS2: case I_DIVU2:
		/* Format 20 */
		piw->ifmtgeneric.pwg_wd1 = 0x0000;
		piw->ifmt20.fmt20_signed = (ivalue==I_DIVS2);
		piw->ifmt20.fmt20_size = 0;
		piw->ifmt20.fmt20_dreg1 = SOP1->adreg1.regno;
		piw->ifmt20.fmt20_dreg2 = SOP1->adreg1.regno;
		codeindx += 2;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_DIVS3: case I_DIVSL:
	case I_DIVU3: case I_DIVUL:
		/* Format 20 */
		piw->ifmtgeneric.pwg_wd1 = 0x0000;
		piw->ifmt20.fmt20_signed = (ivalue==I_DIVS3 || ivalue==I_DIVSL);
		piw->ifmt20.fmt20_size = (ivalue==I_DIVS3 || ivalue==I_DIVU3);
		piw->ifmt20.fmt20_dreg1 = OP1->unoperand.regpair.rpreg2.regno; 
		piw->ifmt20.fmt20_dreg2 = OP1->unoperand.regpair.rpreg1.regno;
		codeindx += 2;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_MULS2: case I_MULU2:
		/* Format 20 */
		piw->ifmtgeneric.pwg_wd1 = 0x0000;
		piw->ifmt20.fmt20_signed = (ivalue==I_MULS2);
		piw->ifmt20.fmt20_size = 0;
		piw->ifmt20.fmt20_dreg1 = SOP1->adreg1.regno; 
		piw->ifmt20.fmt20_dreg2 = 0;
		codeindx += 2;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_MULS3: case I_MULU3:
		/* Format 20 */
		piw->ifmtgeneric.pwg_wd1 = 0x0000;
		piw->ifmt20.fmt20_signed = (ivalue==I_MULS3);
		piw->ifmt20.fmt20_size = 1;
		piw->ifmt20.fmt20_dreg1 = OP1->unoperand.regpair.rpreg2.regno; 
		piw->ifmt20.fmt20_dreg2 = OP1->unoperand.regpair.rpreg1.regno;
		codeindx += 2;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;
# endif	/* M68020 */

	case I_TRAP:
		/* Format 21 */
		piw->ifmt21.fmt21_d4 = abs_value(&SOP0->adexpr1,1,0,15);
		break;

	case I_MOVEC_CTL:
	case I_MOVEC_REG:
		/* Format 22 */
		{ int rop, crop;
		  rop = (ivalue==I_MOVEC_CTL)?  0:1;
		  crop = (ivalue==I_MOVEC_CTL)?  1:0;
		  piw->ifmt22.fmt22_rtype = (sop[rop]->adreg1.regkind==AREG);
		  piw->ifmt22.fmt22_reg = sop[rop]->adreg1.regno;
# ifdef OFORTY
		/* because the 68040 allows two pmu registers to appear in a movec */
		  if (sop[crop]->admode == A_PMUREG
			  && sop[crop]->adreg1.regno == PMTC)
			piw->ifmt22.fmt22_creg = 0x003;
		  else if (sop[crop]->admode == A_PMUREG
				   && sop[crop]->adreg1.regno == PMSRP)
			piw->ifmt22.fmt22_creg = 0x807;
		  else
		  	piw->ifmt22.fmt22_creg = sop[crop]->adreg1.regno;
# else
		  piw->ifmt22.fmt22_creg = sop[crop]->adreg1.regno;
# endif
		  codeindx += 2;
		  break;
		}

	case I_ADDQ:
	case I_SUBQ:
		/* Format 23 */
		piw->ifmt23.fmt23_d3 = abs_value(&SOP0->adexpr1, 1, 1, 8);
		piw->ifmt23.fmt23_size = opsize_bits(ivalue, instrp->operation_size);
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_SCC:	case I_SCS:	case I_SEQ:
	case I_SF:	case I_SGE:	case I_SGT:
	case I_SHI:	case I_SLE:
	case I_SLS:	case I_SLT:
	case I_SMI:	case I_SNE:	case I_SPL:
	case I_ST:	case I_SVC:	case I_SVS:
		/* Format 24 */
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_DBCC:	case I_DBCS:	case I_DBEQ:
	case I_DBF:	case I_DBGE:	case I_DBGT:
	case I_DBHI:	case I_DBLE:
	case I_DBLS:	case I_DBLT:
	case I_DBMI:	case I_DBNE:	case I_DBPL:
	case I_DBT:	case I_DBVC:	case I_DBVS:
		/* Format 25 */
		piw->ifmt25.fmt25_d3 = SOP0->adreg1.regno;
		codeindx += build_branch_disp((char*)piw+2, &SOP1->adexpr1,
			SZWORD,0);
		break;

# ifdef M68020
	case I_TPCC:	case I_TPCS:	case I_TPEQ:
	case I_TPF:	case I_TPGE:	case I_TPGT:
	case I_TPHI:	case I_TPLE:
	case I_TPLS:	case I_TPLT:
	case I_TPMI:	case I_TPNE:	case I_TPPL:
	case I_TPT:	case I_TPVC:	case I_TPVS:
		/* Format 25 */
		piw->ifmt25.fmt25_d3 = (instrp->operation_size==SZWORD)?2:3;
		gen_expr(&SOP0->adexpr1, instrp->operation_size);
		break;
# endif	/* M68020 */

	case I_BCC:	case I_BCS:	case I_BEQ:
	case I_BGE:	case I_BGT:	case I_BHI:
	case I_BLE:	case I_BLS:	case I_BLT:
	case I_BMI:	case I_BNE:	case I_BPL:
	case I_BVC:	case I_BVS:	case I_BRA:
	case I_BSR:
		/* Format 26 */
		codeindx += build_bcc_branch_disp((char *)piw+1, &SOP0->adexpr1,
			instrp->operation_size, instrp) - 1;
		break;

	case I_MOVEQ:
		/* Format 27 */
		piw->ifmt27.fmt27_dreg = SOP1->adreg1.regno;
		(void) check_abs_range(&SOP0->adexpr1, -128, 127, 1);
		build_expr((char*)piw+1, &SOP0->adexpr1, SZBYTE);
		break;

	case I_ADD_REG:	case I_ADD_MEM:
	case I_SUB_REG: case I_SUB_MEM:
	case I_OR_REG:	case I_OR_MEM:
	case I_AND_REG:	case I_AND_MEM:
	case I_EOR:
		/* Format-28 */
		{ int eaop, dop;
		  eaop = piw->ifmt28.fmt28_direction ? 1:0,
		  dop = piw->ifmt28.fmt28_direction ? 0:1,
		  piw->ifmt28.fmt28_dreg = sop[dop]->adreg1.regno;
		  piw->ifmt28.fmt28_size = opsize_bits(ivalue,
			instrp->operation_size);
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

	case I_CMP:
		/* Format 28 */
		piw->ifmt28.fmt28_dreg = SOP0->adreg1.regno;
		piw->ifmt28.fmt28_size = opsize_bits(ivalue,
			instrp->operation_size);
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_MULS1: case I_MULU1:
	case I_DIVS1: case I_DIVU1:
		/* Format 29 */
		piw->ifmt29.fmt29_dreg = SOP1->adreg1.regno;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_ABCD_REG:  case I_ABCD_MEM:
	case I_SBCD_REG:  case I_SBCD_MEM:
		/* Format-30: fill in 2 registers */
		piw->ifmt30.fmt30_dstreg = SOP1->adreg1.regno;
		piw->ifmt30.fmt30_srcreg = SOP0->adreg1.regno;
		break;
		
# ifdef M68020
	case I_PACK_MEM:  case I_PACK_REG:
	case I_UNPK_REG:  case I_UNPK_MEM:
		/* Format-30 + adjustment word */
		piw->ifmt30.fmt30_dstreg = SOP1->adreg1.regno;
		piw->ifmt30.fmt30_srcreg = SOP0->adreg1.regno;
		gen_expr(&SOP2->adexpr1,  SZWORD);
		break;
# endif	/* M68020 */

	case I_ADDX_REG:	case I_ADDX_MEM:
	case I_SUBX_REG:	case I_SUBX_MEM:
		/* Format 31 */
		piw->ifmt31.fmt31_size = opsize_bits(ivalue, instrp->operation_size);
		piw->ifmt31.fmt31_dstreg = SOP1->adreg1.regno;
		piw->ifmt31.fmt31_srcreg = SOP0->adreg1.regno;
		break;

	case I_CMPM:
		/* Format 31 */
		piw->ifmt31.fmt31_size = opsize_bits(ivalue, instrp->operation_size);
		piw->ifmt31.fmt31_dstreg = SOP0->adreg1.regno;
		piw->ifmt31.fmt31_srcreg = SOP1->adreg1.regno;
		break;

	case I_EXG_DD:	case I_EXG_AA:
	case I_EXG_DA:
		/* Format 32 */
		piw->ifmt32.fmt32_regx = SOP0->adreg1.regno;
		piw->ifmt32.fmt32_regy = SOP1->adreg1.regno;
		break;

	case I_EXG_AD:
		/* Format 32 */
		piw->ifmt32.fmt32_regx = SOP1->adreg1.regno;
		piw->ifmt32.fmt32_regy = SOP0->adreg1.regno;
		break;

	case I_ASL_IMM: case I_ASR_IMM:
	case I_LSL_IMM: case I_LSR_IMM:
	case I_ROL_IMM: case I_ROR_IMM:
	case I_ROXL_IMM: case I_ROXR_IMM:
		/* Format 33 */
		piw->ifmt33.fmt33_size = opsize_bits(ivalue,instrp->operation_size);
		piw->ifmt33.fmt33_cnt = abs_value(&SOP0->adexpr1, 1, 1, 8);
		piw->ifmt33.fmt33_dreg = SOP1->adreg1.regno;
		break;

	case I_ASL_REG: case I_ASR_REG:
	case I_LSL_REG: case I_LSR_REG:
	case I_ROL_REG: case I_ROR_REG:
	case I_ROXL_REG: case I_ROXR_REG:
		/* Format 33 */
		piw->ifmt33.fmt33_size = opsize_bits(ivalue,instrp->operation_size);
		piw->ifmt33.fmt33_cnt = SOP0->adreg1.regno;
		piw->ifmt33.fmt33_dreg = SOP1->adreg1.regno;
		break;

	case I_ASL_MEM:	case I_ASR_MEM:
	case I_LSL_MEM:	case I_LSR_MEM:
	case I_ROL_MEM:	case I_ROR_MEM:
	case I_ROXL_MEM:	case I_ROXR_MEM:
		/* Format 34 */
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

# ifdef M68020
	case I_BFCHG: case I_BFCLR:
	case I_BFSET: case I_BFTST:
		/* Format 35 */
		gen_bitfield_specifier(OP0);
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_BFEXTS: case I_BFEXTU:
	case I_BFFFO:
		/* Format 35 */
		gen_bitfield_specifier(OP0);
		piw->ifmt35.fmt35_dreg = SOP2->adreg1.regno;
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_BFINS:
		/* Format 35 */
		gen_bitfield_specifier(OP1);
		piw->ifmt35.fmt35_dreg = SOP0->adreg1.regno;
		gen_ea_bits(OP2, &ea, instrp, 2);
		SET_EA_MODE_AND_REG;
		break;
# endif	/* M68020 */

	case I_ADDA:  case I_SUBA:
	case I_CMPA:
		{ int eaop, arop;
		  eaop = (ivalue==I_CMPA) ? 1:0;
		  arop = (ivalue==I_CMPA) ? 0:1;
		  /* Format-36 */
		  piw->ifmt36.fmt36_areg = sop[arop]->adreg1.regno;
		  piw->ifmt36.fmt36_opmode = opsize_bits(ivalue,
			instrp->operation_size);
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

	}  /* end switch on ivalue */
}

/* Do the "ususal" translation of operation size to opsize bit pattern.
 * Note that there are a few instructions that use a different encoding
 * and so have to be handled as special cases.
 */
unsigned int opsize_bits(ivalue,opsize)
  unsigned int opsize;
{
  switch(ivalue) {
	default:
	   switch(opsize) {
		default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("illegal opsize for encoding");
			return(0);
		case SZBYTE:
			return(0);
		case SZWORD:
			return(1);
		case SZLONG:
			return(2);
		}

	case I_CAS:  case I_CAS2:
	   switch(opsize) {
		default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("illegal opsize for encoding");
			return(0);
		case SZBYTE:
			return(1);
		case SZWORD:
			return(2);
		case SZLONG:
			return(3);
		}

	case I_MOVE:
	   switch(opsize) {
		default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("illegal opsize for encoding");
			return(0);
		case SZBYTE:
			return(1);
		case SZWORD:
			return(3);
		case SZLONG:
			return(2);
		}


	case I_ADDA:
	case I_SUBA:
	case I_CMPA:
	   if (opsize==SZWORD) return(3);
	   else if (opsize==SZLONG) return(7);
	   else {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("illegal opsize for encoding");
		return(0);
		}
	
	}
}


# if defined(M68881) || defined(M68851)
/* Generic coprocessor instructions:
 *	cpBcc, cpDBcc, cpScc, cpTPcc, cpTcc, cpTPcc
 *	cpSAVE, cpRESTORE
 */
build_cpinstruction_bits(instrp, icp_type)
  register struct instruction * instrp;
  int icp_type;
{
   unsigned long ivalue;
   union iwfmt * piw;
   addrmode *sop[3];

   ivalue = instrp->ivalue;
   if (ivalue >= I_LASTOP)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad ivalue in build_cpinstruction_bits");

   piw = (union iwfmt *) &codebuf[codeindx];
   sop[0] = &(instrp->opaddr[0].unoperand.soperand);
   sop[1] = &(instrp->opaddr[1].unoperand.soperand);
   sop[2] = &(instrp->opaddr[2].unoperand.soperand);

   switch(icp_type) {
	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("bad coprocessor instruction type");
	   break;

	case 0:		/* cpGEN */
	case 3:		/* cpBcc.l -- cpBcc are all type 2 for now */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("unexpected cp_type in build_cpinstruction_bits");
	   break;

	case 1:		/* cpTRAPcc, cpScc, cpDBcc */
	   /* two core words -- second is in table. */
	   piw->ifmtword0 = 0xf000;
	   piw->icpfmt0.cpfmt0_cptype = icp_type;
	   piw->ifmtgeneric.pwg_wd1 = iopcode[ivalue];
	   codeindx += 4;
	   break;

	case 2:		/* cpBcc , fnop */
	case 4:		/* cpSAVE */
	case 5:		/* cpRESTORE */
	   /* one core word -- get from table */
	   piw->ifmtword0 = iopcode[ivalue];
	   codeindx += 2;
	   break;
	}

   piw->icpfmt0.cpfmt0_cpid = instrp->cpid;
   switch(instrp->iclass) {

	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected iclass in build-cpinstruction-bits");
		break;

	case I_FBcc:
	case I_PBcc:
# ifdef SDOPT
		if (instrp->operation_size==SZNULL && sdopt_flag) {
		   gen_cpbcc_fixup(piw, &SOP0->adexpr1, instrp);
		   codeindx += 4;
		   }
		else
#endif
		   {
		   piw->icpfmt1.cpfmt1_size = (instrp->operation_size==SZLONG);
		   codeindx += build_branch_disp((char *)piw+2, &SOP0->adexpr1,
			instrp->operation_size,0);
		   }
		break;

	case I_FNOP:
		piw->ifmtgeneric.pwg_wd1 = 0;
		codeindx += 2;
		break;

	case I_FDBcc:
	case I_PDBcc:
		piw->icpfmt2.cpfmt2_cf2 = 9;
		piw->icpfmt2.cpfmt2_dreg = SOP0->adreg1.regno;
		codeindx += build_branch_disp((char*)piw+4, &SOP1->adexpr1,
			SZWORD,0);
		break;

	case I_FScc:
	case I_PScc:
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_FTcc:
	case I_FTPcc:
	case I_PTRAPcc:
		if (instrp->noperands==0) {
		   piw->icpfmt5.cpfmt5_cf3 = 7;
		   piw->icpfmt5.cpfmt5_mode = 4;
		   }
		else {
		   piw->icpfmt5.cpfmt5_cf3 = 7;
		   piw->icpfmt5.cpfmt5_mode = (instrp->operation_size==SZWORD)?2:3;
		   gen_expr(&SOP0->adexpr1, instrp->operation_size);
		   }
		break;


	case I_FSAVE:
	case I_FRESTORE:
	case I_PSAVE:
	case I_PRESTORE:
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;


	}

}
# endif

# ifdef M68881
/* map SZ... to the fp-easize value */
int fp_easize_encode[] = { -1, 6, 4, 0, 1, 5, 2, 3, -1 };

build_fpinstruction_bits(instrp, ifp_type)
  register struct instruction * instrp;
  int ifp_type;
{
   unsigned long ivalue;
   union iwfmt * piw;
   addrmode *sop[3];

   floatop_seen = 1;	/* used in setting version (a_stamp) field */

   ivalue = instrp->ivalue;
   if (ivalue >= I_LASTOP)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad ivalue in build_fpinstruction_bits");

   piw = (union iwfmt *) &codebuf[codeindx];
   sop[0] = &(instrp->opaddr[0].unoperand.soperand);
   sop[1] = &(instrp->opaddr[1].unoperand.soperand);
   sop[2] = &(instrp->opaddr[2].unoperand.soperand);

   if (ifp_type==0) {
	/* two core words -- second is in table. */
	piw->ifmtword0 = 0xf000;
	piw->icpfmt0.cpfmt0_cptype = ifp_type;
	piw->ifmtgeneric.pwg_wd1 = iopcode[ivalue];
	codeindx += 4;
	}
   else
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad fp-type in build_fpinstruction_bits");

   piw->icpfmt0.cpfmt0_cpid = instrp->cpid;
   switch(ivalue) {

	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected ivalue in build-fpinstruction-bits");
		break;

	/* case I_Fdop_EA: */
	case I_FADD_EA:	case I_FDIV_EA:
	case I_FMOD_EA:	case I_FMUL_EA:
	case I_FREM_EA:	case I_FSCALE_EA:
	case I_FSGLDIV_EA:	case I_FSGLMUL_EA:
	case I_FSUB_EA:
	/* case I_Fmop_EA: case I_Fdop_EA: */
	case I_FABS_EA:	case I_FACOS_EA:	case I_FASIN_EA:
	case I_FATAN_EA:	case I_FATANH_EA:	case I_FCOS_EA:
	case I_FCOSH_EA:	case I_FETOX_EA:	case I_FETOXM1_EA:
	case I_FGETEXP_EA:	case I_FGETMAN_EA:	case I_FINT_EA:
	case I_FINTRZ_EA:	case I_FLOGN_EA:	case I_FLOGNP1_EA:
	case I_FLOG10_EA:	case I_FLOG2_EA:	case I_FNEG_EA:
	case I_FSIN_EA:	case I_FSINH_EA:	case I_FSQRT_EA:
	case I_FTAN_EA:	case I_FTANH_EA:	case I_FTENTOX_EA:
	case I_FTWOTOX_EA:
	case I_FMOVE_from_EA:
#ifdef OFORTY
	case I_FSABS_EA:	case I_FDABS_EA:
	case I_FSADD_EA:	case I_FDADD_EA:
	case I_FSDIV_EA:	case I_FDDIV_EA:
	case I_FSMOVE_EA:	case I_FDMOVE_EA:
	case I_FSMUL_EA:	case I_FDMUL_EA:
	case I_FSNEG_EA:	case I_FDNEG_EA:
	case I_FSSQRT_EA:	case I_FDSQRT_EA:
	case I_FSSUB_EA:	case I_FDSUB_EA:
#endif
		piw->ifpfmt2.fpfmt2_easize = fp_easize_encode[instrp->
			operation_size];
		piw->ifpfmt2.fpfmt2_fpreg = SOP1->adreg1.regno;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_FMOVE_to_EA:
		piw->ifpfmt2.fpfmt2_easize = fp_easize_encode[instrp->
			operation_size];
		piw->ifpfmt2.fpfmt2_fpreg = SOP0->adreg1.regno;
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	/* case I_Fmop_FPDREG: */
	case I_FABS_FPDREG:	case I_FACOS_FPDREG:	case I_FASIN_FPDREG:
	case I_FATAN_FPDREG:	case I_FATANH_FPDREG:	case I_FCOS_FPDREG:
	case I_FCOSH_FPDREG:	case I_FETOX_FPDREG:	case I_FETOXM1_FPDREG:
	case I_FGETEXP_FPDREG:	case I_FGETMAN_FPDREG:	case I_FINT_FPDREG:
	case I_FINTRZ_FPDREG:	case I_FLOGN_FPDREG:	case I_FLOGNP1_FPDREG:
	case I_FLOG10_FPDREG:	case I_FLOG2_FPDREG:	case I_FNEG_FPDREG:
	case I_FSIN_FPDREG:	case I_FSINH_FPDREG:	case I_FSQRT_FPDREG:
	case I_FTAN_FPDREG:	case I_FTANH_FPDREG:	case I_FTENTOX_FPDREG:
	case I_FTWOTOX_FPDREG:
		piw->ifpfmt3.fpfmt3_fpreg1 = SOP0->adreg1.regno;
		piw->ifpfmt3.fpfmt3_fpreg2 = (instrp->noperands==1) ?
		SOP0->adreg1.regno : SOP1->adreg1.regno;
		break;

	/* case I_Fdop_FPDREG: */
	case I_FADD_FPDREG:	case I_FDIV_FPDREG:
	case I_FMOD_FPDREG:	case I_FMUL_FPDREG:
	case I_FREM_FPDREG:	case I_FSCALE_FPDREG:
	case I_FSGLDIV_FPDREG:	case I_FSGLMUL_FPDREG:
	case I_FSUB_FPDREG:
	case I_FMOVE_FPDREG:
#ifdef OFORTY
	case I_FSABS_FPDREG:	case I_FDABS_FPDREG:
	case I_FSADD_FPDREG:	case I_FDADD_FPDREG:
	case I_FSDIV_FPDREG:	case I_FDDIV_FPDREG:
	case I_FSMOVE_FPDREG:	case I_FDMOVE_FPDREG:
	case I_FSMUL_FPDREG:	case I_FDMUL_FPDREG:
	case I_FSNEG_FPDREG:	case I_FDNEG_FPDREG:
	case I_FSSQRT_FPDREG:	case I_FDSQRT_FPDREG:
	case I_FSSUB_FPDREG:	case I_FDSUB_FPDREG:
#endif
		piw->ifpfmt3.fpfmt3_fpreg1 = SOP0->adreg1.regno;
		piw->ifpfmt3.fpfmt3_fpreg2 = SOP1->adreg1.regno;
		break;

	case I_FCMP_EA:
		piw->ifpfmt2.fpfmt2_easize = fp_easize_encode[instrp->
			operation_size];
		piw->ifpfmt2.fpfmt2_fpreg = SOP0->adreg1.regno;
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_FCMP_FPDREG:
		piw->ifpfmt3.fpfmt3_fpreg1 = SOP1->adreg1.regno;
		piw->ifpfmt3.fpfmt3_fpreg2 = SOP0->adreg1.regno;
		break;

	case I_FTEST_EA:
		piw->ifpfmt2.fpfmt2_easize = fp_easize_encode[instrp->
			operation_size];
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_FTEST_FPDREG:
		piw->ifpfmt3.fpfmt3_fpreg1 = SOP0->adreg1.regno;
		break;

	case I_FSINCOS_EA:
		piw->ifpfmt1.fpfmt1_src = fp_easize_encode[instrp->
			operation_size];
		piw->ifpfmt1.fpfmt1_fpsin = OP1->unoperand.regpair.rpreg2.regno;
		piw->ifpfmt1.fpfmt1_fpcos = OP1->unoperand.regpair.rpreg1.regno;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_FSINCOS_FPDREG:
		piw->ifpfmt1.fpfmt1_src = SOP0->adreg1.regno;
		piw->ifpfmt1.fpfmt1_fpsin = OP1->unoperand.regpair.rpreg2.regno;
		piw->ifpfmt1.fpfmt1_fpcos = OP1->unoperand.regpair.rpreg1.regno;
		break;

	case I_FMOVE_to_FPCREG:
		piw->ifpfmt5.fpfmt5_crlist = SOP1->adreg1.regno;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_FMOVE_from_FPCREG:
		piw->ifpfmt5.fpfmt5_crlist = SOP0->adreg1.regno;
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_FMOVEM_to_FPCREG:
		piw->ifpfmt5.fpfmt5_crlist = OP1->unoperand.reglist;
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;

	case I_FMOVEM_from_FPCREG:
		piw->ifpfmt5.fpfmt5_crlist = OP0->unoperand.reglist;
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

		{ int rlop, eaop;
	case I_FMOVEM_toFP_mode2:
		  rlop = 1; eaop = 0;
		  goto fmovem;
		
	case I_FMOVEM_fromFP_mode0:
	case I_FMOVEM_fromFP_mode2:
		  rlop = 0; eaop = 1;
		fmovem:
		  /* skip the following check since compiler uses forward
		   * references for an immediate register list.
		   */
		  /*(void) check_abs_range(&sop[rlop]->adexpr1, 0, 255, 1);*/
		  build_expr((char*)piw+3, &sop[rlop]->adexpr1, SZBYTE);
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

		{ int dregop, eaop;
	case I_FMOVEM_toFP_mode3:
		  eaop = 0; dregop = 1;
		  goto fmovem_dynamic;

	case I_FMOVEM_fromFP_mode1:
	case I_FMOVEM_fromFP_mode3:
		  eaop = 1; dregop = 0;
		fmovem_dynamic:
		  piw->ifpfmt7.fpfmt7_reglist = sop[dregop]->adreg1.regno << 4;
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

	case I_FMOVECR:
		piw->ifpfmt6.fpfmt6_fpreg = SOP1->adreg1.regno;
		piw->ifpfmt6.fpfmt6_romoff = abs_value(&SOP0->adexpr1,1,0,0x7f);
		break;

	case I_FMOVE_toEA_PK:
		piw->ifpfmt4.fpfmt4_fpreg = SOP0->adreg1.regno;
		if (SOP1->admode == A_DREG) {
		   piw->ifpfmt4.fpfmt4_dstfmt = 7;
		   piw->ifpfmt4.fpfmt4_kval = SOP1->adreg1.regno << 4;
		   }
		else {
		   piw->ifpfmt4.fpfmt4_dstfmt = 3;
		   piw->ifpfmt4.fpfmt4_kval = abs_value(&SOP1->adexpr1,1,
			-64,63);
		   }
		gen_ea_bits(OP2, &ea, instrp, 2);
		SET_EA_MODE_AND_REG;
		break;

	}

}
# endif  /* 68881 */

# ifdef M68851

/* This array of structures contains "preg" and "num" field info for the MC68851
 * registers.  The ordering of this array must agree with the enumeration
 * constant values assigned to the register identifiers in symbols.h.
 */

struct {
   char preg;
   char num;
   } pmureg_info[] =  {
	{ 3, 0 }, /* crp */
	{ 2, 0 }, /* spr */
	{ 1, 0 }, /* drp */
	{ 0, 0 }, /* tc */
	{ 7, 0 }, /* ac */
	{ 0, 0 },	/* psr */
	{ 1, 0 }, /* pcsr */
	{ 4, 0 }, /* cal */
	{ 5, 0 }, /* val */
	{ 6, 0 }, /* scc */
	{ 5, 0 }, /* bac0 */
	{ 5, 1 }, /* bac1 */
	{ 5, 2 }, /* bac2 */
	{ 5, 3 }, /* bac3 */
	{ 5, 4 }, /* bac4 */
	{ 5, 5 }, /* bac5 */
	{ 5, 6 }, /* bac6 */
	{ 5, 7 }, /* bac7 */
	{ 4, 0 }, /* bad0 */
	{ 4, 1 }, /* bad1 */
	{ 4, 2 }, /* bad2 */
	{ 4, 3 }, /* bad3 */
	{ 4, 4 }, /* bad4 */
	{ 4, 5 }, /* bad5 */
	{ 4, 6 }, /* bad6 */
	{ 4, 7 }, /* bad7 */
	};

# define PMUID	0	/* coprocessor id */

build_pmuinstruction_bits(instrp, ipmu_type)
  register struct instruction * instrp;
  int ipmu_type;
{
   unsigned long ivalue;
   union iwfmt * piw;
   addrmode *sop[4];

   ivalue = instrp->ivalue;
   if (ivalue >= I_LASTOP)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad ivalue in build_pmuinstruction_bits");

   piw = (union iwfmt *) &codebuf[codeindx];
   sop[0] = &(instrp->opaddr[0].unoperand.soperand);
   sop[1] = &(instrp->opaddr[1].unoperand.soperand);
   sop[2] = &(instrp->opaddr[2].unoperand.soperand);
   sop[3] = &(instrp->opaddr[3].unoperand.soperand);

   if (ipmu_type!=0)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad ipmu_type in build_pmuinstruction_bits");

#ifdef OFORTY
	if (ivalue != I_PFLUSH && ivalue != I_PFLUSHN && ivalue != I_PFLUSHA &&
		ivalue != I_PFLUSHAN && ivalue != I_PTESTR && ivalue != I_PTESTW)
#endif
	{
		/* two core words -- second is in table. */
		piw->ifmtword0 = 0xf000;
		piw->icpfmt0.cpfmt0_cptype = ipmu_type;
		piw->ifmtgeneric.pwg_wd1 = iopcode[ivalue];
		codeindx += 4;

		piw->icpfmt0.cpfmt0_cpid = PMUID;
	}
#ifdef OFORTY
	else	{
		/*
			for 68040 support, ptest and pflush are only one word
			instructions so first/only word of instruction comes
			from table and just fill in the register field (if
			applicable) since the opmode field is already encoded
			in the table
		*/
		codeindx += 2;
		piw->ifmtword0 = iopcode[ivalue];	/* 1 op word */
	}
#endif
   switch(ivalue) {

	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected ivalue in build-pmuinstruction-bits");
		break;

	case I_PVALID_VAL:
		/* second word constant, set <ea> */
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_PVALID_AREG:
		piw->ipmufmt7.pmufmt7_areg = SOP0->adreg1.regno;
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

#ifdef OFORTY
	case I_PFLUSH:	case I_PFLUSHN:
		piw->ipmufmt2.pmufmt2_reg = SOP0->adreg1.regno;
		break;

	case I_PFLUSHA:	case I_PFLUSHAN:
		break;
#else
	case I_PFLUSHA:
		/* only need to set mode, everything else, leave at zero. */
		piw->ipmufmt1.pmufmt1_mode = 1;
		break;

	case I_PFLUSH:
	case I_PFLUSH_EA:
	case I_PFLUSHS:
	case I_PFLUSHS_EA:
		piw->ipmufmt1.pmufmt1_mode = pflush_mode(ivalue);
		piw->ipmufmt1.pmufmt1_mask = abs_value(&SOP1->adexpr1, 1,
		  0, 15);
		piw->ipmufmt1.pmufmt1_fc = pmu_fc(SOP0);
		if (instrp->noperands == 3) {
		  gen_ea_bits(OP2, &ea, instrp, 2);
		  SET_EA_MODE_AND_REG;
		  }
		break;

	case I_PFLUSHR:
		gen_ea_bits(OP0, &ea, instrp, 0);
		SET_EA_MODE_AND_REG;
		break;
#endif

	case I_PLOADR:
	case I_PLOADW:
		piw->ipmufmt3.pmufmt3_fc = pmu_fc(SOP0);
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;

	case I_PMOVE_to_EA1:
	case I_PMOVE_to_PMU1:
		{ int eaop, pmuregop;
		  eaop = (ivalue==I_PMOVE_to_EA1)? 1: 0;
		  pmuregop = (ivalue==I_PMOVE_to_EA1)? 0: 1;
		  piw->ipmufmt4.pmufmt4_preg = pmureg_info[sop[pmuregop]->
		    adreg1.regno].preg;
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}
	case I_PMOVE_to_EA2:
	case I_PMOVE_to_PMU2:
		{ int eaop, pmuregop;
		  eaop = (ivalue==I_PMOVE_to_EA2)? 1: 0;
		  pmuregop = (ivalue==I_PMOVE_to_EA2)? 0: 1;
		  piw->ipmufmt5.pmufmt5_preg = pmureg_info[sop[pmuregop]->
		    adreg1.regno].preg;
		  piw->ipmufmt5.pmufmt5_num = pmureg_info[sop[pmuregop]->
		    adreg1.regno].num;
		  gen_ea_bits(OP(eaop), &ea, instrp, eaop);
		  SET_EA_MODE_AND_REG;
		  break;
		}

#ifdef OFORTY
	case I_PTESTR:	case I_PTESTW:
		piw->ipmufmt2.pmufmt2_reg = SOP0->adreg1.regno;
		break;
#else
	case I_PTESTR:
	case I_PTESTW:
		piw->ipmufmt6.pmufmt6_level = abs_value(&SOP2->adexpr1, 1,
		  0, 15);
		piw->ipmufmt6.pmufmt6_fc = pmu_fc(SOP0);
		if (instrp->noperands==4) {
		   piw->ipmufmt6.pmufmt6_aregbit = 1;
		   piw->ipmufmt6.pmufmt6_aregno = SOP3->adreg1.regno;
		   }
		gen_ea_bits(OP1, &ea, instrp, 1);
		SET_EA_MODE_AND_REG;
		break;
#endif

	}

}

#ifndef OFORTY
int pflush_mode(ivalue) {
   switch(ivalue) {
	case I_PFLUSH:
		return 4;
	case I_PFLUSH_EA:
		return 6;
	case I_PFLUSHS:
		return 5;
	case I_PFLUSHS_EA:
		return 7;
	}
}
#endif

/* Determine the pmu-fc field for an 68851 instruction.
 * Several instructions have a  5-bit fc field, and they all define it in
 * a similar fashion.
 */
int pmu_fc(fcop)
  addrmode * fcop;
{ switch(fcop->admode) {
	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected address mode for PMU-FC operand");
		return(0);

	case A_IMM:
		return abs_value(&(fcop->adexpr1), 1, 0, 15)|0x10;
	case A_DREG:
		return fcop->adreg1.regno|0x08;
	case A_CREG:
		return (fcop->adreg1.regno == SFCREG)? 0:1 ;
	}
}

# endif  /* 68851 */

