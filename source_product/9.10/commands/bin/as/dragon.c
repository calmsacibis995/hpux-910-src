/* @(#) $Revision: 70.1 $ */     

# ifdef DRAGON
/* dragon.c
 * This file contains routines to translate dragon pseudo-ops to
 * 68020 moves and whatever other instructions are necessary.
 * Modified 861215 to reflect redefinition of DRAGON instruction set.
 */

# include "symbols.h"
# include "adrmode.h"
# include "ivalues.h"
# include "icode.h"
# include "asgram.h"
# include "opfmts.h"
# include "sdopt.h"
# include "header.h"

# define DRAGON_AREG	2
int dragon_areg = DRAGON_AREG;

int dragon_nowait = 0;

# define PRBIT	  0x0004	/* bit 2 */
# define PRDOUBLE 0x0004	/* bit 2 set */
# define PRSINGLE 0x0000	/* bit 2 clear */
# define FASTBIT  0x0008
# define LOADLONG 0x0000
# define LOADWORD 0x0002	/* No longer implemented by DRAGON HW */
# define LOADBYTE 0x0003	/* No longer implemented by DRAGON HW */
# define NOLOAD   0x0001
# define PRMSW	  0x0000	/* bit 2 clear */
# define PRLSW	  0x0004	/* bit 2 set */

extern symbol * dot;

/* Usually we'll be translating DRAGON ops to move.b instructions
 * that look very similar.
 * We'll use a fixed structure that can be setup to contain most
 * of the necessary info, and then just reset the fields that
 * change.
 */

struct instruction dragon_inst;
rexpr dragon_wait;
rexpr dragon_test;
rexpr subq8;

/* When using absolute addressing, the symbol "fpa_loc" will be used.
 * But we don't want to generate this external into the assembly file
 * unless a program actually uses it, hence the extra pushups here.
 */
symbol * fpaloc_sym = 0;
# define FPALOC_SYM ( fpaloc_sym ? fpaloc_sym : ( fpaloc_sym = (symbol *)\
		lookup("fpa_loc", INSTALL, USRNAME)) )


/* initialize the fixed fields of the dragon_inst instruction structure. */

dragon_setup() {
	dragon_inst.noperands = 2;
	dragon_inst.ivalue = I_MOVE;
	dragon_inst.iclass = I_MOVE;
	dragon_inst.operation_size = SZBYTE;
	dragon_inst.cpid = 0;
	dragon_inst.sdi_info = 0;
	
	dragon_inst.opaddr[0].opadtype = A_SOPERAND;
	dragon_inst.opaddr[0].unoperand.soperand.admode = A_DREG;
	dragon_inst.opaddr[0].unoperand.soperand.adreg1.regkind = DREG;
	dragon_inst.opaddr[0].unoperand.soperand.adreg1.regno = 0;

	dragon_inst.opaddr[1].opadtype = A_SOPERAND;
	dragon_inst.opaddr[1].unoperand.soperand.admode = A_ADISP;
	dragon_inst.opaddr[1].unoperand.soperand.adreg1.regkind = AREG;
	dragon_inst.opaddr[1].unoperand.soperand.adreg1.regno = dragon_areg;
	dragon_inst.opaddr[1].unoperand.soperand.adexpr1.exptype = TYPL;
	/*dragon_inst.opaddr[1].unoperand.soperand.adexpr1.expsize = 0; */
	dragon_inst.opaddr[1].unoperand.soperand.adexpr1.symptr = 0;
	dragon_inst.opaddr[1].unoperand.soperand.adexpr1.expval.lngval = 0;

	dragon_wait.expval.lngval = 0x4a106bfc | (dragon_areg<<16);
	dragon_wait.exptype = TYPL;
	
	dragon_test.expval.lngval = 0x4a280000 | (dragon_areg<<16);
	dragon_test.exptype = TYPL;

	subq8.exptype = TYPL;


}

translate_dragon(instrp, rawops)
  register struct instruction *instrp;
  operand_addr1 * rawops[];
{ unsigned long ivalue;
  unsigned long iclass;
  register unsigned long dragon_offset;
  short src1, src2;
  short upop;
  short size;
  int drgopno;
  int memopno;
  
  floatop_seen = 1;	/* Set to 1 to indicate that a floating point op has
			 * been seen.  This is used in setting version
			 * (a_stamp) field when no explicit version pseudo-op
			 * is specified.
			 */

  ivalue = instrp->ivalue;
  iclass = instrp->iclass;
  size = instrp->operation_size;

  /* Initial offset - iopcode is a table of short, and we want sign
   * extension if the high bit is set (so offset will look like a 
   * negative value that will fit in the 16-bit range).
   */
  dragon_offset = (int) (short) iopcode[ivalue];

  /* since the iopcode[] array is only a short, the UPOP bit is put
   * in the low-order bit (part of the LOAD field, which is going to
   * have to be set anyway.
   */
  upop = dragon_offset & 01;
  if (upop) {
	dragon_offset &= 0xffff;	/* clear any sign extension that
					 * occurred in the initial assignment
					 * to dragon_offset:  now it's going
					 * to need to look like a large
					 * positive number that is out of 
					 * the 16-bit range.
					 */
	dragon_offset ^= 0x10001;	/* turn on UPOP bit, zero low bit */
	}

  switch(iclass==I_FPMOVE? ivalue: iclass) {
	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	   aerror("unknown iclass or unimplemented instruction in translate_dragon");
	   break;

	case I_FPmop:
	case I_FPCVS:	case I_FPCVD:	case I_FPCVL:
	   src1 = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
	   src2 = (instrp->noperands==1)? src1 :
		instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   goto simpleop;

	case I_FPdop:
	case I_FPMOVE_on_DRG:
	   src1 = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
	   src2 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   goto simpleop;

	case I_FPTEST:
	   src1 = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
	   src2 = src1;
	   goto simpleop;

	/* a FPCMP is really a FPdop except that the operands are swapped.
	 */
	case I_FPCMP:
	   src2 = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
	   src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   goto simpleop;

	 simpleop:
	   if (size == SZDOUBLE)
		dragon_offset |= PRDOUBLE;
	   dragon_offset |= src1<<8;	/* source1 field */
	   dragon_offset |= src2<<4;	/* source2 field */
	   dragon_offset |= NOLOAD;	/* set LOAD field */
	   dragon_inst.opaddr[1].unoperand.soperand.adexpr1.expval.lngval = 
		dragon_offset;
	   
	   /* set the addressing mode for the dragon move. */
	   if (!upop) {
		dragon_inst.opaddr[1].unoperand.soperand.admode = A_ADISP16;
		dragon_inst.opaddr[1].unoperand.soperand.adexpr1.exptype = TYPL;
		/* note: don't need to reset adreg1: the ABS32 cases won't have
		 * corrupted it.
		 */
		}
	   else {
		dragon_inst.opaddr[1].unoperand.soperand.admode = A_ABS32;
		dragon_inst.opaddr[1].unoperand.soperand.adexpr1.exptype = TSYM;
		dragon_inst.opaddr[1].unoperand.soperand.adexpr1.symptr = FPALOC_SYM;
		}
	   generate_icode(I_INSTRUCTION, &dragon_inst);

	   if ( !dragon_nowait && !(dragon_offset & FASTBIT) &&
	      (ivalue != I_FPINTRZ) && (ivalue != I_FPNEG))
		/* throw out a wait loop */
		generate_icode(I_LONG, &dragon_wait, 0);
	   break;

	case I_FPMmop:
	case I_FPMCVS:	case I_FPMCVD:	case I_FPMCVL:
	   src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   src2 = (instrp->noperands==2)? src1 :
		instrp->opaddr[2].unoperand.soperand.adreg1.regno;
	   goto combinedop;

	case I_FPMdop:
	case I_FPMMOVE:
	   src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   src2 = instrp->opaddr[2].unoperand.soperand.adreg1.regno;
	   goto combinedop;

	case I_FPM2dop:
	   src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   src2 = instrp->opaddr[2].unoperand.soperand.adreg1.regno;
	   goto combinedop;

	case I_FPM2CMP:
	   src2 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   src1 = instrp->opaddr[2].unoperand.soperand.adreg1.regno;
	   goto combinedop;

	case I_FPMTEST:
	   src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
	   src2 = src1;
	   goto combinedop;

	 combinedop:
	  {register addrmode * drgaddr;
	   drgaddr = &instrp->opaddr[1].unoperand.soperand;

	   instrp->noperands = 2;
	   if (instrp->operation_size==SZDOUBLE) {
		first_dragon_move(instrp, iclass!=I_FPM2dop?src1:src2,
		   &instrp->opaddr[1], &instrp->opaddr[0], 1);
		adjust_addr_for_second_move(instrp, rawops[0],
			&instrp->opaddr[0], 1);
		/* note that the dragon op is left mostly setup now in
		 * opaddr[1] after the first move.  Just reset the address
		 * mode, which gets done below.
		 * Also the operation ivalue/iclass/operation_size are
		 * also still o.k. for the next move.
		 */
		}
	   else {
		/* need to set up dragon op */
		/* already know opadtype is A_SOPERAND */
		drgaddr->adreg1.regkind = AREG;
		drgaddr->adreg1.regno = dragon_areg;
		drgaddr->adexpr1.exptype = TYPL;

		/* set operation ivalue/iclass/operation_size */
		instrp->ivalue = I_MOVE;
		instrp->iclass = I_MOVE;
		/* NOTE: DRAGON HW no longer supports .b or .w */
		/* mov size is .b,.w, or .l . If operation size
		 * was .s, change it to .l; .b, .w, .l stay the same.
		 */
		if (size==SZSINGLE) instrp->operation_size = SZLONG;
		}


	   if (size == SZDOUBLE)
		dragon_offset |= PRDOUBLE;
	   dragon_offset |= src1<<8;	/* source1 field */
	   dragon_offset |= src2<<4;	/* source2 field */
	   /* set LOAD field */
	   /* NOTE: DRAGON HW no longer supports .b or .w */
	   /*dragon_offset |= (size==SZBYTE)? LOADBYTE: ((size==SZWORD)?LOADWORD:
	   /*	LOADLONG);
	    */
	   dragon_offset |= LOADLONG;
	   instrp->opaddr[1].unoperand.soperand.adexpr1.expval.lngval = 
		dragon_offset;
	   
	   /* set the addressing mode for the dragon move. */
	   if (!upop) {
		drgaddr->admode = A_ADISP16;
		drgaddr->adexpr1.exptype = TYPL;
		}
	   else {
		drgaddr->admode = A_ABS32;
		drgaddr->adexpr1.exptype = TSYM;
		drgaddr->adexpr1.symptr = FPALOC_SYM;
		}
	   generate_icode(I_INSTRUCTION, instrp);

	   if ( !dragon_nowait && !(dragon_offset & FASTBIT) &&
	      (ivalue != I_FPMINTRZ) && (ivalue != I_FPMNEG) )
		/* throw out a wait loop */
		generate_icode(I_LONG, &dragon_wait, 0);
	   break;
	   }

	case I_FPBcc1:  case I_FPBcc2:
	 { register rexpr * exp;
	   /* Generate:  tst.b	dragon_offset(%dareg) */
	   dragon_test.expval.lngval &= 0xffff0000;
	   dragon_test.expval.lngval |= iopcode[ivalue];
	   generate_icode(I_LONG, &dragon_test, 0);

	   /* Fix up the branch instruction to be a "bmi" or "bpl"
	    * but leave the target and size alone.
	    * ?? If we want to do span dependent, call match_and_verify
	    * again??? 
	    * Check that branch target is symbolic, and doesn't use DOT
	    * else give a warning.
	    **** NOTE if the expr uses a forward referenced symbol
	    * that later turns out to be set to absolute, we miss it!!!!
	    */
	   instrp->ivalue = (iclass==I_FPBcc1)? I_BMI : I_BPL;
	   exp = &instrp->opaddr[0].unoperand.soperand.adexpr1;
	   if (exp->exptype != TSYM)
		werror("non-symbolic branch target may not translate as expected");
	   else if (exp->symptr==dot)
		werror("'.' branch target may not translate as expected");

# ifdef SDOPT
	   if (instrp->operation_size == SZNULL) {
		if (sdopt_flag && (dot->stype&STYPE)==STEXT) {
			instrp->sdi_info = makesdi_spannode(SDBCC, instrp);
			}
		else {
			instrp->operation_size = SZWORD;
			}
		}
# endif /* SDOPT */

	   generate_icode(I_INSTRUCTION, instrp);
	   break;
	}

	case I_FPMOVE_to_DRGCREG:
		dragon_offset = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
		drgopno = 1;
		goto single_move2;

	case I_FPMOVE_from_DRGCREG:
		dragon_offset = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
		drgopno = 0;
		goto single_move2;


	/* Single moves (.b,.w.s,.l)
	 * NOTE: DRAGON HW no longer implements .b or .w
	 * To finish the dragon offset:
	 *	- set the LOAD field
	 *	- set the source1 field
	 *	- set the PR filed as PRMSW
	 *	- (leave source2 as 0)
	 */
	case I_FPMOVE_to_DRG:
		src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
		drgopno = 1;
		goto single_move1;

	case I_FPMOVE_from_DRG:
		src1 = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
		drgopno = 0;
	     
	     single_move1:
		/*dragon_offset |= ( size==SZBYTE? LOADBYTE : (size==SZWORD?
		/*	LOADWORD: LOADLONG));
		 */
		dragon_offset |= LOADLONG;
		dragon_offset |= PRMSW;
		dragon_offset |= src1 << 8;

	     single_move2:
		/* modify the opcode, and dragon operand of the I_FPMOVE.
		 * Note we know A_ADISP16 is mode of second operand, so
		 * don't need to call fix_addr_mode.
		 */
		{ register addrmode * drgaddr;
		  instrp->ivalue = I_MOVE;
		  instrp->iclass = I_MOVE;	/* not needed now, but just
						 * in case bit-gen starts
						 * looking at iclass field.
						 */
		  if (size==SZSINGLE)
			instrp->operation_size = SZLONG;
		  drgaddr = &(instrp->opaddr[drgopno].unoperand.soperand);
		  drgaddr->admode = A_ADISP16;
		  drgaddr->adreg1.regkind = AREG;
		  drgaddr->adreg1.regno = dragon_areg;
		  drgaddr->adexpr1.exptype = TYPL;
		  drgaddr->adexpr1.expval.lngval = dragon_offset;
		}
		generate_icode(I_INSTRUCTION, instrp);
		break;
		

	/* Double moves (.d)
	 */
	case I_FPMOVED_to_DRG:
		src1 = instrp->opaddr[1].unoperand.soperand.adreg1.regno;
		drgopno = 1;
		memopno = 0;
		goto  double_move;

	case I_FPMOVED_from_DRG:
		src1 = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
		drgopno = 0;
		memopno = 1;

	     double_move:
		first_dragon_move(instrp, src1, &instrp->opaddr[drgopno],
			&instrp->opaddr[memopno], 0);
		adjust_addr_for_second_move(instrp, rawops[memopno],
			&instrp->opaddr[memopno], 0);
		/* do the second move */
		/* toggle the PR bit */
		instrp->opaddr[drgopno].unoperand.soperand.adexpr1.expval.lngval
			^= PRBIT;
		generate_icode(I_INSTRUCTION, instrp);
		break;

	case I_FPWAIT:
		/* throw out a wait loop */
		generate_icode(I_LONG, &dragon_wait, 0);
	   	break;

	case I_FPAREG:
		/* reset global variables that contains areg number */
		dragon_areg = instrp->opaddr[0].unoperand.soperand.adreg1.regno;
		/* reset preset structures */
		dragon_inst.opaddr[1].unoperand.soperand.adreg1.regno = dragon_areg;
		dragon_wait.expval.lngval = 0x4a106bfc | (dragon_areg<<16);
		dragon_test.expval.lngval = 0x4a280000 | (dragon_areg<<16);
		break;

	}	/* end switch */
}

first_dragon_move(instrp, drgregno, drgop, memop, combined_flag)
  register struct instruction *instrp;
  int drgregno; 
  register operand_addr * drgop;
  register operand_addr * memop;
  int combined_flag;
{ int addrtype;
  register unsigned long dragon_offset;
  register addrmode * drgaddr;
  int backwards_flag = 0;
  
  addrtype = (memop->opadtype==A_SOPERAND)? memop->unoperand.soperand.admode :
	memop->opadtype;
  
  /* Do special mods based on the address mode. */
  switch(addrtype) {
	/* remove after debugging */
	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected addrmode for double_dragon_move");
		break;
  
	/* these cases don't have to do anything.
	 * ?? remove after debugged ??
	 */
	case A_ABASE:	case A_AINCR:
	case A_ADISP16:	case A_AINDXD8:	case A_AINDXBD:
	case A_AMEM:	case A_AMEMX:
	case A_ABS16:	case A_ABS32:
		break;

	
	case A_PCDISP16:	case A_PCINDXD8:
	case A_PCINDXBD:	case A_PCMEM:	 case A_PCMEMX:
		/* check that the displacement is symbolic and
		 * doesn't use '.'
		 **** NOTE if the expr uses a forward referenced symbol
		 * that later turns out to be set to absolute, we miss it!!!!
		 */
		if (memop->unoperand.soperand.adexpr1.exptype != TSYM)
		   uerror("PC relative double move address must be symbolic");
		else if (memop->unoperand.soperand.adexpr1.symptr==dot)
		   uerror("'.' not allowed for PC relative double move address");
		break;

	case A_ADECR:
		/* If doing just a move, we'll do the move backwards. 
		 * If this is part of a combined_op, the move has to be
		 * done (MSW, LSW) so special pushups are needed:
		 * Then ADECR becomes:
		 *		- subq &8 from the areg
		 *		- mov.l using (%areg)
		 *		- mov.l using 4(%areg)
		 *
		 * If the %areg is the same as the %dragon_areg ???
		 */
		 if (combined_flag) {
			register addrmode * memaddr;
			subq8.expval.lngval = 0x5148 | (memop->unoperand.
			    soperand.adreg1.regno);
			generate_icode(I_SHORT, &subq8, 0);
			memaddr = & memop->unoperand.soperand;
			memaddr->admode = A_ABASE;
			memaddr->adexpr1.exptype = TYPNULL;
			}
		else backwards_flag = 1;
		break;

	case A_DREGPAIR:
		/* use A_DREG addressing, one register at a time.  The
		 * registers are MSW:LSW.
		 */
		{ reg reg1;	/* with current layout of structures
				 * this temporary not needed.  But
				 * any shift in structures could cause
				 * an overlay in fields that would 
				 * make the structure copy incorrect,
				 * the temporary is safer.
				 */
		  reg1 = memop->unoperand.regpair.rpreg1;
		  memop->unoperand.soperand.adreg1 = reg1;
		  memop->opadtype = A_SOPERAND;
		  memop->unoperand.soperand.admode = A_DREG;
		  break;
		}

	case A_IMM:
		/* access the immediate as two long values.
		 * Here we access the first word by just changing the
		 * appropriate sizes.
		 */
		memop->unoperand.soperand.adsize = SZLONG;
		memop->unoperand.soperand.adexpr1.exptype = TYPL;
		memop->unoperand.soperand.adexpr1.expsize = SZLONG;
		break;

	}	/* switch end */


  dragon_offset = iopcode[I_FPMOVED_to_DRG];	/* value same for _to_/_from_ */
  dragon_offset |= backwards_flag? PRLSW : PRMSW;
  dragon_offset |= LOADLONG;
  dragon_offset |= drgregno << 8;
  drgaddr = &(drgop->unoperand.soperand);
  drgaddr->admode = A_ADISP16;
  drgaddr->adreg1.regkind = AREG;
  drgaddr->adreg1.regno = dragon_areg;
  drgaddr->adexpr1.exptype = TYPL;
  drgaddr->adexpr1.expval.lngval = dragon_offset;

  instrp->ivalue = I_MOVE;
  instrp->iclass = I_MOVE;
  instrp->operation_size = SZLONG;

  generate_icode(I_INSTRUCTION, instrp);

}

adjust_addr_for_second_move(instrp, rawmemop, memop, combined_flag)
  register struct instruction *instrp;
  operand_addr1 * rawmemop;
  register operand_addr * memop;
  int combined_flag;
{ int rawaddrtype;	/* original addressing mode before addr_fixup() */
  register rexpr * exp;

  rawaddrtype = (rawmemop->opadtype==A_SOPERAND)? rawmemop->unoperand.soperand->admode :
	rawmemop->opadtype;

  /* Do special mods based on the address mode. */
  switch(rawaddrtype) {
	/* remove after debugging */
	default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unexpected addrmode for double_dragon_move");
		break;
  
	/* these cases don't have to do anything.
	 * ?? remove after debugged ??
	 */
	case A_AINCR:
		break;

	case A_ADISP:	case A_AINDX:
	case A_ABS16:	case A_ABS32:
	case A_ABS:
	case A_PCDISP:	case A_PCINDX:
		/* Reset the mode to the original.
		 * Add 4 to the appropriate displacement expression.  For
		 * modes with only one expression this is adexpr1;  for
		 * modes with two expressions the appropriate one to
		 * increment is the adexpr2, which corresponds to the "outer
		 * displacement."
		 * Be careful to check for a previously TYPNULL expression.
		 * Check that the expression was not a TDIFF: we can't add 4 to
		 * these.A
		 * If the expression is TYPL and now has value 0, make
		 * it into a TYPNULL, and if the mode was ADISP it can now
		 * be ABASE.
		 */
		exp = &memop->unoperand.soperand.adexpr1;
		goto add4toexp;

	case A_AMEM:	case A_AMEMX:
	case A_PCMEM:	 case A_PCMEMX:
		exp = &memop->unoperand.soperand.adexpr2;
	     add4toexp:
		if (exp->exptype & TDIFF) {
			uerror("displacement in double move cannot be symbol difference");
			return;
			}
		if (exp->exptype==TYPNULL) {
			/* set up an expression where there was none before */
			exp->exptype = TYPL;
			exp->expval.lngval = 0;
			}
		exp->expval.lngval += 4;

		if (exp->exptype==TYPL && exp->expval.lngval==0) {
			exp->exptype = TYPNULL;
			if (rawaddrtype==A_ADISP) rawaddrtype=A_ABASE;
			}
		memop->unoperand.soperand.admode = rawaddrtype;


		break;


	case A_ABASE:
		memop->unoperand.soperand.admode = A_ADISP;
		memop->unoperand.soperand.adexpr1.exptype = TYPL;
		memop->unoperand.soperand.adexpr1.expval.lngval = 4;
		break;

	
	case A_ADECR:
		/* If we are doing a combined op, the second move will
		 * now be ADISP with disp 4, otherwise it's just
		 * ADECR again and we are going backwards (LSW, then MSW).
		 */
		if (combined_flag) {
			memop->unoperand.soperand.admode = A_ADISP;
			memop->unoperand.soperand.adexpr1.exptype = TYPL;
			memop->unoperand.soperand.adexpr1.expval.lngval = 4;
			}
		break;

	case A_DREGPAIR:
		/* the operand was already converted to DREG addressing.
		 * Now we need to set it up to access the second register
		 * of the pair.
		 */
		memop->unoperand.soperand.adreg1 = rawmemop->unoperand.regpair->
			rpreg2;
		break;

	case A_IMM:
		/* need to access the second word (LSW). */
		memop->unoperand.soperand.adexpr1.expval.lngval =
			rawmemop->unoperand.soperand->adexptr1->expval.lngpair.lsw;
		break;
	}	/* switch end */

   /* call fix_addr_mode to translate the generic modes to the
    * specific mode required.
    */
   fix_addr_mode(memop);
}

# endif /* DRAGON */
