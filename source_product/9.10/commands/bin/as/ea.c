/* @(#) $Revision: 70.1 $ */    

# include "symbols.h"
# include "adrmode.h"
# include "asgram.h"
# include "bitmask.h"
# include "verify.h"
# include "opfmts.h"
# include "icode.h"
# include "ivalues.h"

# ifdef SEQ68K
# include "seq.a.out.h"
# else
# include <a.out.h>
# endif

# include "sdopt.h"
# include "fixup.h"

extern symbol * dot;
extern unsigned int line;

/* Look up table for effective address mode and Reg values. The index
 * into this array must agree with the defines for the addressing modes
 * in file addrmode.h.
 * Also info on mapping an addressing mode to the corresponding bit in
 * the template masks.
 */

struct ea_encode {
	unsigned char	ea_mode;
	unsigned char	ea_reg;
	unsigned char	ea_rflag;	/* 1 if reg is a reg# */
	unsigned long	ea_matchmask;
	};

struct ea_encode ea_encode[] = {
  /* 0 */	{	0,	0,	1, M(T_DREG) },	/* A_DREG */
  /* 1 */	{	1,	0,	1, M(T_AREG) },	/* A_AREG	 */
  /* 2 */	{	2,	0,	1, M(T_ABASE) },/* A_ABASE */
  /* 3 */	{	3,	0,	1, M(T_AINCR) },	/* A_AINCR */
  /* 4 */	{	4,	0,	1, M(T_ADECR) },	/* A_ADECR */
  /* 5 */	{	5,	0,	1, M(T_ADISP16) },	/* A_ADISP16 */
  /* 6 */	{	6,	0,	1, M(T_AINDXD8) },	/* A_AINDXD8 */
  /* 7 */	{	6,	0,	1, M(T_AINDXBD) },	/* A_AINDXBD */
  /* 8 */	{	6,	0,	1, M(T_AMEM) },	/* A_AMEM	 */
  /* 9 */	{	6,	0,	1, M(T_AMEMX) },	/* A_AMEMX */
  /* 10 */	{	7,	2,	0, M(T_PCDISP16) },	/* A_PCDISP16 */
  /* 11 */	{	7,	3,	0, M(T_PCINDXD8) },	/* A_PCINDXD8 */
  /* 12 */	{	7,	3,	0, M(T_PCINDXBD) },	/* A_PCINDXBD */
  /* 13 */	{	7,	3,	0, M(T_PCMEM) },	/* A_PCMEM */
  /* 14 */	{	7,	3,	0, M(T_PCMEMX) },	/* A_PCMEMX */
  /* 15 */	{	7,	0,	0, M(T_ABS16) },	/* A_ABS16 */
  /* 16 */	{	7,	1,	0, M(T_ABS32) },	/* A_ABS32 */
  /* 17 */	{	7,	4,	0, M(T_IMM) },	/* A_IMM	 */

  /* 18 */	{	255,	0,	0, M(T_CCREG) },	/* A_CCREG	 */
  /* 19 */	{	255,	0,	0, M(T_SRREG) },	/* A_SRREG	 */
  /* 20 */	{	255,	0,	0, M(T_CREG) },	/* A_CREG	 */
  /* 21 */	{	255,	0,	0, M(T_DREGPAIR) },	/* A_DREGPAIR */
  /* 22 */	{	255,	0,	0, M(T_REGPAIR) },	/* A_REGPAIR */
  /* 23 */	{	255,	0,	0, M(T_REGLIST) },	/* A_REGLIST */
  /* 24 */	{	255,	0,	0, M(T_CACHELIST) },	/* A_CACHELIST */
  /* 25 */	{	255,	0,	0, M(T_PMUREG) },	/* A_PMUREG (was A_LABEL slot)*/
  /* 26 */	{	255,	0,	0, M(T_FPDREG) },	/* A_FPDREG */
  /* 27 */	{	255,	0,	0, M(T_FPCREG) },	/* A_FPCREG */
  /* 28 */	{	255,	0,	0, M(T_BFSPECIFIER) },	/* A_BFSPECIFIER */
  /* 29 */	{	255,	0,	0, M(T_FPDREGPAIR) },	/* A_FPDREGPAIR */
  /* 30 */	{	255,	0,	0, M(T_FPDREGLIST) },	/* A_FPDREGLIST */
  /* 31 */	{	255,	0,	0, M(T_FPCREGLIST) },	/* A_FPCREGLIST */
  /* 32 */	{	255,	0,	0, M(T_FPPKSPECIFIER) },/* A_FPPKSPECIFIER */
  /* 33 */	{	255,	0,	0, 0 },	/* A_ABS	 */
  /* 34 */	{	255,	0,	0, 0 },	/* A_ADISP */
  /* 35 */	{	255,	0,	0, 0 },	/* A_PCDISP */
  /* 36 */	{	255,	0,	0, 0 },	/* A_AINDX */
  /* 37 */	{	255,	0,	0, 0 },	/* A_PCINDX */
  /* 38 */	{	255,	0,	0, 0 },	/* A_SOPERAND */
  /* 39 */	{	255,	0,	0, 0 },	/* unused */
  /* 40 */	{	255,	0,	0, 0 },	/* unused */
  /* 41 */	{	255,	0,	0, M(T_DRGDREG) },	/* A_DRGDREG */
  /* 42 */	{	255,	0,	0, M(T_DRGCREG) },	/* A_DRGCREG */

};


/* The following routine is called from the parser during pass1 to
 * "massage" the operands:
 *	-- determine a specific addressing type from generic modes
 *	-- the "operand" parameter is a pointer to a operand_addr
 *	   structure.  This structure will be modified by the
 *	   routine.
 */

extern short sdispflag;

#ifdef PIC
short        immed_flag = 0;
short        abs_flag   = 0;
short        data_flag  = 0;
short        jump_flag  = 0;
short        lea_flag   = 0;
long         drelocs    = 0;
extern short pea_flag;
extern short pic_flag;
extern short big_got_flag;
extern short rpc_flag;
extern short shlib_flag;
#endif

void  fix_addr_mode(operand)
  register operand_addr * operand;
{
  switch (operand->opadtype) {

	default:
		aerror("unknown operand type in fix-addr-mode");
		break;

	
	case A_SOPERAND:
		/*  this includes all the "normal" addressing modes.
		 *  Must look at subfields to further determine the
		 *  type.
		 */
		{  register addrmode * paddr = &(operand->unoperand.soperand);
		   int dsize;
		   switch (paddr->admode) {
			default:
				aerror("unknown soperand admode");
				break;

			case  A_DREG:
			case  A_AREG:
			case  A_CREG:
			case  A_CCREG:
			case  A_SRREG:
			case  A_ABASE:
			case  A_AINCR:
			case  A_ADECR:
				/* these are "simple" addressing modes.
				 * No changes need to be made.
				 */
				break;
# ifdef M68881
			case  A_FPDREG:
			case  A_FPCREG:
				break;
# endif /* M68881 */

# ifdef DRAGON
			case A_DRGDREG:
			case A_DRGCREG:
				break;
# endif /* DRAGON */

# ifdef M68851
			case A_PMUREG:
				break;
# endif /* M68851 */

			case  A_ADISP:
				dsize = dispsize(&paddr->adexpr1,TINT|TSYM|TDIFF);
# ifdef M68020
				/* note that for the 68010 case only AREG is 
				 * possible and the DISP will be DISP16 in all
				 * cases.
				 * For the 68020, if the sdispflag is on and
				 * the register is not suppressed, then assume
				 * a 16-bit displacement.
				 */
				if ((sdispflag||dsize<=2) && paddr->adreg1.regkind == AREG) {
# endif
					paddr->admode = A_ADISP16;
					paddr->adexpr1.expsize = SZWORD;
					break;
# ifdef M68020
				}
				else  {
					paddr->admode =  A_AINDXBD;
					paddr->adxreg.xrreg.regkind = NOREG;
					break;
					}
# endif

			case  A_PCDISP:
				/*adjust_pc_displacement(&paddr->adexpr1);*/
				dsize = pcdispsize(&paddr->adexpr1, paddr->
				   adreg1.regkind==PC);
				if (dsize==1) {
					paddr->adexpr1.expsize = SZWORD;
					}
# ifdef M68020
				if ((sdispflag||dsize<=2) && paddr->adreg1.regkind == PC) {
# endif
					paddr->admode = A_PCDISP16;
					paddr->adexpr1.expsize = SZWORD;
					break;
# ifdef M68020
				}
				else {
					paddr->admode = A_PCINDXBD;
					paddr->adxreg.xrreg.regkind = NOREG;
					break;
					}
# endif

			case  A_AINDX:
				/* For M68020 and sdispflag, assume a short
				 * displacement as long as the registers fit
				 * the 68010 format (not suppressed, and
				 * no scale factor). 
				 * Note we can assume there was an xreg because
				 * otherwise we would have initially marked it
				 * as A_ADISP.
				 */
				dsize = expsize(&paddr->adexpr1,TINT|TSYM|TDIFF|TYPNULL);
# ifdef M68020
				if ( (dsize<=1 || (sdispflag && paddr->adxreg.
				  xrscale==XSF_NULL)) && paddr->adreg1.regkind==AREG
				  && (paddr->adxreg.xrreg.regkind == AREG ||
				      paddr->adxreg.xrreg.regkind == DREG)) {
# endif
					paddr->admode = A_AINDXD8;
					break;
# ifdef M68020
					}
				paddr->admode = A_AINDXBD;
				if (dsize==1) {
					dsize = 2;
					paddr->adexpr1.expsize = SZWORD;
					}
				break;
# endif
			case  A_PCINDX:
				/*adjust_pc_displacement(&paddr->adexpr1);*/
				dsize = pcdispsize(&paddr->adexpr1, paddr->
				   adreg1.regkind==PC);
# ifdef M68020
				if ( (dsize<=1 || (sdispflag && paddr->adxreg.
				  xrscale==XSF_NULL)) && paddr->adreg1.regkind==PC
				  && (paddr->adxreg.xrreg.regkind == AREG ||
				     paddr->adxreg.xrreg.regkind == DREG)) {
# endif
					paddr->admode = A_PCINDXD8;
					break;
# ifdef M68020
					}
				paddr->admode = A_PCINDXBD;
				if (dsize==1) {
					paddr->adexpr1.expsize = SZWORD;
					}
				break;
# endif

# ifdef M68020
			case  A_AMEM:
			case  A_AMEMX:
				(void ) dispsize(&paddr->adexpr1,TINT|TSYM|TDIFF|TYPNULL);
				(void ) dispsize(&paddr->adexpr2,TINT|TSYM|TDIFF|TYPNULL);
				break;

			case  A_PCMEM:
			case  A_PCMEMX:
				if (pcdispsize(&paddr->adexpr1, paddr->
				   adreg1.regkind==PC) == 1)
					paddr->adexpr1.expsize = SZWORD;
					
				(void) dispsize(&paddr->adexpr2,TINT|TSYM|TDIFF|TYPNULL);
				break;
# endif

			case  A_IMM:
				/* size depends on the operation, not the
				 * operand.
				 * call expsize just to get the expsize field
				 * set in the expression.
				 */
				(void) expsize(&paddr->adexpr1,TINT|TSYM|TDIFF|TYPNULL|TFLT);
				break;

			case  A_ABS:
				/* If it's a branch, we'll turn it into an
				 * implicit PC indirect.
				 * Else (at least for now) it's going to be
				 * absolute.
				 */
				/* If it's a branching type instruction
				 *  change into a PC-displacement 
				 *  set up sdi info if appropriate.
				 * 
				 *  else
				 */
				if (dispsize(&paddr->adexpr1,TINT|TSYM|TDIFF)<=2)
					paddr->admode = A_ABS16;
				else paddr->admode = A_ABS32;
				break;

			case A_ABS16:
				paddr->adexpr1.expsize = SZWORD;
				break;

			case A_ABS32:
				paddr->adexpr1.expsize = SZLONG;
				break;

# ifdef M68020
			case A_BFSPECIFIER:
				  /* bit field operand -- no change */
				  break;
# endif
			}
		  break;
		}
# ifdef OFORTY
        case A_CACHELIST:
                /* for cinv and cpush (68040 instructions) */
# endif

	case A_REGLIST:
		/* register list -- no change */
		break;

# ifdef M68020
	case A_DREGPAIR:
	case A_REGPAIR:
		/* register pair ( for mul and div ) */
		break;
# endif  /* M68020 */

# ifdef M68881
	case A_FPDREGPAIR:
	case A_FPDREGLIST:
	case A_FPCREGLIST:
	case A_FPPKSPECIFIER:
		break;
# endif  /* M68881 */
	}
  return;

}

/*  ??? what if it's NULL?  do we build a -2 instead?  Similarly, if it's
 *  +2, do we change it to 0 or to NULL?
 */
# ifdef NOT_CURRENTLY_USED
adjust_pc_displacement(exp)
  rexpr * exp;
{
  if (exp->exptype & TINT) {
	exp->expval.lngval -= 2;
	return;
	}
  /*else {
  /* if (exp->exptype==TYPNULL) {
	exp->exptype = TYPL;
	exp->expval.lngval = -2;
	return;
	}
  */
  else {
    werror("no adjustment to PC-offset done");
    }


}
# endif
int pcdispsize(exp, pcflag)
  register rexpr * exp;
  int pcflag;
{ symbol * sym;
  register long dispval;

  if (!(exp->exptype&(TINT|TSYM|TDIFF|TYPNULL)))
	uerror("illegal expression type for pc-relative expression");
  /* if the expression is absolute or if %zpc is used, the expression
   * is the displacement with no adjustment.
   */
   if (!pcflag || exp->exptype&(TINT|TYPNULL|TDIFF))
	return(expsize(exp, TINT|TYPNULL|TDIFF|TSYM));

   /* the expression is symbolic. If it's already defined and in the
    * current segment, calculate what the displacement will be.
      *** Currently we can base our calculation on (.+2) .  This
	  will have to change if branch labels use this routine
	  because an fdb will be based from (.+4).
      
      *** This calculation could be bad when used with span-dependent:  if
	  the address is something like L+d(%pc) (instead of just a simple
	  target L(%pc) ), then span-dependent optimization could actually
	  cause the displacement to grow larger, and beyond the range
	  chosen here.
	  So, if span-dependent optimization is on, a non-simple target for
	  a symbol other than <dot> is assigned the worst case.
    */
   sym = exp->symptr;
   if ( ((sym->stype&STYPE) == dot->stype) && 
	   ( sym==dot || !sdopt_flag || !exp->expval.lngval) ) {
	dispval = (sym->svalue + exp->expval.lngval) - (dot->svalue + 2);
	if (dispval >= -128 && dispval <= 127) {
	   exp->expsize = SZBYTE;
	   return(1);
	}
	else if (dispval >= -32768 && dispval <= 32767) {
	   exp->expsize = SZWORD;
	   return(2);
	}
	else {
	   exp->expsize = SZLONG;
	   return(4);
	   }
	}
  else {
	exp->expsize = SZLONG;
	return(4);
	}


}

/*  NOTE:   this routine currently only talks integer expression types !!! */
/*  need to modify to understand float types. */
int expsize(exptr, exptype)
  rexpr * exptr;
  unsigned long exptype;
{
  register long value;
  if (!(exptr->exptype & exptype)) {
	uerror("illegal expression type");
	return(0);
	}

  switch (exptr->exptype) {

	default:
		aerror("illegal expression type");
		return(0);

	case TSYM:
	case TDIFF|TSYM:
#ifdef PIC
		if (pic_flag && !big_got_flag && !lea_flag) {
		   exptr->expsize = SZWORD;
		   return(2);
		}
#endif
		/* return size long */
		exptr->expsize = SZLONG;
		return(4);

	case  TYPNULL:
		exptr->expsize = SZNULL;
		return(0);

	case  TYPL:
		value = exptr->expval.lngval;
		if (value <= 127 && value >= -128 ) {
		   exptr->expsize = SZBYTE;
		   return(1);
		   }
		else if (value <= 32767 && value >= -32768) {
		   exptr->expsize = SZWORD;
		   return(2);
		   }
		else {
		   exptr->expsize = SZLONG;
		   return(4);
		   }
		break;

	case  TYPF:
		exptr->expsize = SZSINGLE;
		return(4);
		break;
	case  TYPD:
		exptr->expsize = SZDOUBLE;
		return(8);
		break;
	case  TYPP:
		exptr->expsize = SZPACKED;
		return(12);
		break;
	case  TYPX:
		exptr->expsize = SZEXTEND;
		return(12);
		break;

	}

}


int dispsize(exptr, exptype)
  rexpr * exptr;
  unsigned long exptype;
{ int dsize;
  dsize = expsize(exptr, exptype);
  if (dsize == 1) {
	dsize = 2;
	exptr->expsize = SZWORD;
	}
  return(dsize);
}


struct instruction instruction;

int match_and_verify(instrp, instr_size, opaddr1, nops)
  instr * instrp;
  int instr_size;
  register operand_addr1 * opaddr1[];
  register int nops;
{
  register int i;
  register operand_addr * op;
  int mindex;		/* index into match table */
  register struct  verify_entry  * pventry;
  short imatch;	/* the I_XXX value	  */
  unsigned long minstr_size;
  unsigned long maddr[MAXOPERAND];
  register operand_addr *opaddr = &instruction.opaddr[0];

  /* a kludgy copy from new parser data structures to old data structures,
   * just to help get a quicker idea of whether this will speed up parser.
   */
 { register addrmode1 * addr1;

#ifdef PIC
  if (instrp->imatch == I_LEA) {
      lea_flag = 1;
  }
#endif

  for (i=0, op=opaddr; i<nops; i++, op++) {
	/* from opaddr1[i], build opaddr[i] */
	addr1 = opaddr1[i]->unoperand.soperand;
	op->opadtype = opaddr1[i]->opadtype;

	switch(op->opadtype) {
		default:
			aerror("bad opadtype- in opaddr conversion");
			break;
		case A_SOPERAND:
			op->unoperand.soperand.admode = addr1->admode;
			op->unoperand.soperand.adsize = addr1->adsize;
			op->unoperand.soperand.adreg1 = addr1->adreg1;
			op->unoperand.soperand.adxreg = addr1->adxreg;
			if (addr1->adexptr1 != 0)
			   op->unoperand.soperand.adexpr1 = *(addr1->adexptr1);
			else 
			   op->unoperand.soperand.adexpr1.exptype = TYPNULL;
			if (addr1->adexptr2 != 0)
			   op->unoperand.soperand.adexpr2 = *(addr1->adexptr2);
			else 
			   op->unoperand.soperand.adexpr2.exptype = TYPNULL;
			break;

		case A_REGLIST:
# ifdef M68020
		case A_FPDREGLIST:
		case A_FPCREGLIST:
# endif
# ifdef OFORTY
                case A_CACHELIST:
# endif
			op->unoperand.reglist = opaddr1[i]->unoperand.reglist;
			break;
# ifdef M68020
		case A_REGPAIR:
		case A_DREGPAIR:
		case A_FPDREGPAIR:
			op->unoperand.regpair = *opaddr1[i]->unoperand.regpair;
			break;
		case A_FPPKSPECIFIER:
			op->unoperand.soperand.admode = addr1->admode;
			if (op->unoperand.soperand.admode==A_IMM)
			   op->unoperand.soperand.adexpr1 = *addr1->adexptr1;
			else
			   op->unoperand.soperand.adreg1 = addr1->adreg1;
			break;
# endif
		}

	/* do the generic operand translation */
	fix_addr_mode(op);

	/* set up the masks to be used in the pattern matching process */
	switch(op->opadtype) {
		case A_SOPERAND:
			maddr[i] = ea_encode[op->unoperand.soperand.admode].ea_matchmask;
			break;

		case A_REGLIST:
# ifdef M68020
		case A_DREGPAIR:
		case A_REGPAIR:
		case A_FPDREGLIST:
		case A_FPCREGLIST:
		case A_FPDREGPAIR:
		case A_FPPKSPECIFIER:
# endif
# ifdef OFORTY
                case A_CACHELIST:
# endif
			maddr[i] = ea_encode[op->opadtype].ea_matchmask;
			break;

		default:
			aerror("unknown addrtype in match");
			break;
		}

		
	}

#ifdef PIC
  lea_flag = 0;
#endif

 }


  /* pattern match: opcode operand types */
  /* actions upon matching:
   *		- set a default operation size if necessary
   *		- change the opcode if necessary
   *		- make a special call if necessary
   *		- calculate the sizing info
   */

  imatch = instrp->imatch;
  mindex = matchindex[imatch];
  if (mindex < 0) aerror("no vtable entry");
  pventry = &verify_table[mindex];

  minstr_size = M(instr_size);

	

  
  while (pventry->icode == imatch ) {
	if ( !(minstr_size & pventry->instr_size) || (nops != pventry->noperands))
	   goto try_next_template;
	for (i=0; i<nops; i++) {
	   if ( !(pventry->oprnd_mask[i] & maddr[i]) )
		goto try_next_template;
	   }
	/* at this point we must have a match */
	goto match_found;

	try_next_template:
	pventry++;
	}

   /* if we fall through the loop no match was found. */
   uerror("syntax error (opcode/operand mismatch)");
   /* ??? some kind of default garbage here ??? */
   return 0;

   match_found:
   /* pventry points to the template that matched */
   /* some debugging info */
# ifdef TRACE
   printf("match found: [%d]\n",pventry-verify_table);
# endif
   /* Use the template matched to:
   	- if sizenull  && default-size-field nonzero, set the size field.
   	- change opcode# if specified
   	- call a special routine if indicated
   	- calculate the total space needed by the instruction and its operands
   */
   instruction.ivalue = instrp->ivalue;
   instruction.iclass = imatch; 	/* really needs to be added as a
					 * field of the template.  For
					 * now this will do. 
					 */
   instruction.operation_size = instr_size;
   instruction.noperands = nops;
   instruction.sdi_info = 0;

   if (pventry->actions[1]) instruction.ivalue = pventry->actions[1];
   if (instruction.operation_size == SZNULL && pventry->instr_size>0)
	instruction.operation_size = pventry->actions[0];
   if (pventry->actions[2] >0 )
	/* call special routine */
	match_special_handling(&instruction, pventry, pventry->actions[2]);

   /* Opsize fixup:
	-- for any immediate operands now that we have the instruction
	   type settled.
	-- for SBRANCH, determine size needed for displacement.
    */
   for (i=0; i<nops; i++) {
	if (opaddr[i].opadtype==A_SOPERAND && opaddr[i].unoperand.soperand.admode
		== A_IMM) {
		instruction.opaddr[i].unoperand.soperand.adsize =
			instruction.operation_size;
		}
	}

   return;
}


match_special_handling(instrp, pv, action)
  struct instruction * instrp;
  struct verify_entry * pv;
  int action;
{
  switch(action) {
	default:
		aerror("unknown action number in match-special-handling");
		return;

	case 1:
		/* add &i,<ea>
		 * if i is in the range 1-8, then convert to addq.
		 */
		if( check_abs_range(&instrp->opaddr[0].unoperand.soperand.adexpr1,
		  1, 8, 0) ) {
			/* then it can be an addq */
			instrp->ivalue = I_ADDQ;
			}
		  return;

	case 2:
		/* tas <ea>
		 *	<ea> is memory-alterable.  
		 *	Give a warning message because HP9000s300 machines
		 *	don't support this instruction.
		 */
		werror("\"tas\" with memory operand not a supported instruction");
		return;

	case 3:
		/* bkpt */
		werror("\"bkpt\" not a supported instruction");
		return;

	case 4:
		/* cas */
		werror("\"cas\" not a supported instruction");
		return;

	case 5:
		/* cas2 */
		werror("\"cas2\" not a supported instruction");
		return;

	case 51:
		/* sub &i,<ea>
		 * if i is in the range 1-8, then convert to subq.
		 */
		if( check_abs_range(&instrp->opaddr[0].unoperand.soperand.adexpr1,
		  1, 8, 0) ) {
			/* then it can be a subq */
			instrp->ivalue = I_SUBQ;
			}
		  return;

	case 30:
		/* mov creg,reg
		 * check for mov %usp,%areg
		 * which becomes I_MOVE_from_USP
		 */
		if(instrp->opaddr[0].unoperand.soperand.adreg1.regno == USPREG
		  && instrp->opaddr[1].unoperand.soperand.adreg1.regkind == AREG)
			instrp->ivalue = I_MOVE_from_USP;
		return;

	case 31:
		/* mov reg,creg
		 * check for mov %areg,%usp
		 * which becomes I_MOVE_to_USP
		 */
		if(instrp->opaddr[1].unoperand.soperand.adreg1.regno == USPREG
		  && instrp->opaddr[0].unoperand.soperand.adreg1.regkind == AREG)
			instrp->ivalue = I_MOVE_to_USP;
		return;

	case 33:
		/* mov &i,%dreg
		 * if i is in the range [-128,127], then convert to movq.
		 */
		if( check_abs_range(&instrp->opaddr[0].unoperand.soperand.adexpr1,
		  -128, 127, 0) ) {
			/* then it can be a movq */
			instrp->ivalue = I_MOVEQ;
			}
		return;

	case 34:
		/* movm  <reglist>,-(%an)
		 * We have to reverse the register mask list that was built
		 * by the parser.
		 * Then continue and execute case 35 to convert the <reglist>
		 * to A_IMM form. 
		 */
		{ register unsigned int newreglist, oldreglist;
		  register int i;
		  /* If the first operand is A_IMM, assume the user
		   * already used the "reversed" form. So nothing
		   * to do.
		   */
		  if (instrp->opaddr[0].opadtype != A_REGLIST)
			return;
		  newreglist = 0;
		  oldreglist = instrp->opaddr[0].unoperand.reglist;
		  for (i=0; i<16; i++) {
			newreglist = (newreglist<<1) |  (oldreglist & 0x1);
			oldreglist >>= 1;
			}
		  instrp->opaddr[0].unoperand.reglist = newreglist;
		  /* continue through and execute case 35 code too. */

		}

	case 35:
		/* movm  <reglist>,<EA>   (I_MOVEM_MEM)
		 * movm  <EA>,<reglist>   (I_MOVEM_REG)
		 * If a reglist was used, convert it to an A_IMM, so pass 2
		 * routines only have to deal with the A_IMM operand form.
		 */
		{ operand_addr * rlop;
		  long reglist_val;
		  rlop = &(instrp->opaddr[instrp->ivalue==I_MOVEM_MEM? 0 : 1]);
		  if (rlop->opadtype == A_REGLIST) {
			reglist_val = rlop->unoperand.reglist;
			rlop->opadtype = A_SOPERAND;
			rlop->unoperand.soperand.admode = A_IMM;
			rlop->unoperand.soperand.adsize = SZWORD;
			rlop->unoperand.soperand.adexpr1.exptype = TYPL;
			rlop->unoperand.soperand.adexpr1.expval.lngval = 
				reglist_val;
			}
		  return;
		}

	case 36:
		/* shiftop &imm,<ea>
		 * The &imm must be &1 and is redundant.
		 * Convert the operation to
		 * 	shiftop <ea>
		 */
	      { rexpr *exp;
		exp = &instrp->opaddr[0].unoperand.soperand.adexpr1;
		if (exp->exptype != TYPL || exp->expval.lngval != 1 )
		  { uerror("first operand must be \"&1\"");
		    return;
		  }
		instrp->noperands = 1;
		instrp->opaddr[0] = instrp->opaddr[1];
		return;
	     }

# ifdef M68010
	case 40:
		/* Bcc.l is an error on 68010 */
		uerror("long branch illegal on 68010");
		return;
# endif
		
/* NOTE: this code could now go into the build_instruction bits, or
 *       the build_bcc_disp routines now (under new pass1/pass2 structure)
 *	 if that would be more efficient.
 */
# ifdef SDOPT
	{ int sditype;
	case 80: /* Bcc */
		sditype = SDBCC;
		goto Lsdi;
	case 81: /* BRA */
		sditype = SDBRA;
		goto Lsdi;
	case 82: /* BSR */
		sditype = SDBSR;
		goto Lsdi;
	case 83: /* cpCC */
		sditype = SDCPBCC;
		goto Lsdi;

	  Lsdi:
		if (!sdopt_flag || (dot->stype&STYPE)!=STEXT) {
		   instrp->operation_size = SZWORD;
		   }
		else {
		   instrp->sdi_info = makesdi_spannode(sditype,instrp);
		   }
		return;
	}
# endif

# ifdef M68881
	case 101:
		/* I_Fmop <ea>,FPDREG
		 * I_Fdop <ea>,FPDREG
		 *
		 * the correct I_value is + 1 the value of I_Fmop or I_Fdop.
		 */
		 instrp->ivalue +=  1;
		 return;

	case 102:
		/* I_Fmop FPDREG,FPDREG
		 * I_Fdop FPDREG,FPDREG
		 *
		 * the correct I_value is + 2 the value of I_Fmop.
		 */
		 instrp->ivalue +=  2;
		 return;

	case 103:
	case 104:
		/* fmov areg<->fpcreg
		 * the fpcreg must be %fpiar
		 */
		if (instrp->opaddr[action==103?1:0].unoperand.soperand.adreg1.regno
		  != FPIADDR)
		   uerror("fmov with %%an only legal for %%fpiar");
		return;


	case 105:
		/* fmovm <reglist>,-(%an)  (I_FMOVEM_MEM)
		 * If a <reglist> was used, we have to reverse the register
		 * mask list.
		 * The continue down and executte case 106, to change the
		 * reglist to an A_IMM form operand.
		 */
		{ register unsigned int newreglist, oldreglist;
		  register int i;
		  if (instrp->opaddr[0].opadtype != A_FPDREGLIST)
			return;
		  newreglist = 0;
		  oldreglist = instrp->opaddr[0].unoperand.reglist;
		  for (i=0; i<8; i++) {
			newreglist = (newreglist<<1) |  (oldreglist & 0x1);
			oldreglist >>= 1;
			}
		  instrp->opaddr[0].unoperand.reglist = newreglist;
		  /* continue through and execute case 106 code too. */

		}

	case 106:
		/* I_FMOVEM_xxx
		 * fmovm  <reglist>,<EA>	(I_FMOVEM_MEM)
		 * fmovm  <EA>,<reglist>	(I_FMOVEM_REG)
		 * If a reglist was used, convert it to an A_IMM form
		 * operand for uniformity in pass 2.
		 */
		{ operand_addr * rlop;
		  long reglist_val;
		  rlop = &(instrp->opaddr[instrp->ivalue>=I_FMOVEM_fromFP_mode0
			   &&instrp->ivalue<=I_FMOVEM_fromFP_mode3? 0 : 1]);
		  if (rlop->opadtype == A_FPDREGLIST) {
			reglist_val = rlop->unoperand.reglist;
			rlop->opadtype = A_SOPERAND;
			rlop->unoperand.soperand.admode = A_IMM;
			rlop->unoperand.soperand.adsize = SZBYTE;
			rlop->unoperand.soperand.adexpr1.exptype = TYPL;
			rlop->unoperand.soperand.adexpr1.expval.lngval = 
				reglist_val;
			}
		  return;
		}
	case 107:
	case 109:
		/* fmovm dreg<->fpcreglist
		 * the "list" must consist of exactly one register.
		 */
		{ int crlist;
		  crlist = instrp->opaddr[action==107?1:0].unoperand.reglist;
		  if (crlist != FPIADDR && crlist != FPCONTROL && crlist !=
		     FPSTATUS)
		     uerror("only one fpcontrol register when %%dn used");
		  return;
		}

	case 108:
	case 110:
		/* fmovm areg<->fpcreglist
		 * the "list" must consist of only fpiar
		 */
		{ int crlist;
		  crlist = instrp->opaddr[action==108?1:0].unoperand.reglist;
		  if (crlist != FPIADDR)
		     uerror("fmovm with %%an only legal with %%fpiar");
		  return;
		}

	case 111:
		/* fmovm &xxx -> fpcreglist
		 * the "list" is only supported for one fpcreg
		 */
		{ int crlist;
		  crlist = instrp->opaddr[1].unoperand.reglist;
		  if (crlist != FPIADDR && crlist != FPCONTROL && crlist !=
		     FPSTATUS)
		     uerror("fmovm from immediate only supported for one fpcontrol register\n");
		  return;
		}
# endif	/* M68881 */

# ifdef M68851
	case 201:
		/* Check the PMU-FC argument (always first arg).
		 * If the argument is a CREG, then check that it is
		 * %sfc or %dfc.
		 * Used by pflush, pflushs, ploadr, ploadw, ptestr, ptestw
		 */
		{ int regno;
		  if ( (instrp->opaddr[0].unoperand.soperand.admode==A_CREG) &&
		  ( (regno = instrp->opaddr[0].unoperand.soperand.adreg1.regno)
		   != SFCREG) && (regno != DFCREG ) )
		    uerror("Illegal register for PMU-FC operand");
		  return;
		}

	case 202:
	case 203:
		/* pmove to EA 
		 * pmove to PMU
		 * Check PMU-reg for format1 vs. format2, and reset the
		 * I_PMOVE_EA1 -> I_PMOVE_EA2 for format2.
		 * Also check for AREG, DREG used properly.
		 */
		{ int pmureg;
		  addrmode * eaaddr;
		  pmureg = instrp->opaddr[action==202?0:1].unoperand.soperand.
		    adreg1.regno;
		  eaaddr = &(instrp->opaddr[action==202?1:0].unoperand.soperand);
		  switch(pmureg) {
			default:
				aerror("unkown PMUREGno in PMOVE");
				break;

			case PMCRP: case PMSRP: case PMDRP:
				/* Format1 is O.K.
				 * The <ea> cannot be AREG or DREG
				 */
				if (eaaddr->admode==A_AREG || eaaddr->admode==A_DREG){
				   uerror("illegal <EA> in pmove");
				   }
				break;

			case PMTC:  case PMCAL:  case PMVAL:
			case PMSCC: case PMAC:
				/* Nothing to do: format1 is o.k., and
				 * no additional restrictions on <ea>. 
				 */
				break;

			case PMBAC0:  case PMBAC1:  case PMBAC2:  case PMBAC3:  
			case PMBAC4:  case PMBAC5:  case PMBAC6:  case PMBAC7:  
			case PMBAD0:  case PMBAD1:  case PMBAD2:  case PMBAD3:  
			case PMBAD4:  case PMBAD5:  case PMBAD6:  case PMBAD7:  
			case PMPSR:
				/* Should be format-2.
				 * No additional restrictions on <ea>.
				 */
				instrp->ivalue += 1;
				break;

			case PMPCSR:
				/* Direction must be to <EA>
				 * No additional restrictions on <ea>.
				 * Format should be format-2 
				 */
				if (action==203)
				   uerror("pmove to %%pscr illegal");
				instrp->ivalue += 1;
				break;

			}  /* switch on pmuregno */

		  return;
		}


	case 204:
		/* pvalid
		 * check that first argument (known to be a PMUREG) is
		 * %val
		 */
		if (instrp->opaddr[0].unoperand.soperand.adreg1.regno != PMVAL)
		   uerror("illegal 68851 register: %%val expected");
		return;

# ifdef OFORTY
	/* allow %srp and %tc (both are pmu regs) to be used in "movec" on 040 */
	case 210:
	case 211:
		{	short	pmureg;
		pmureg=instrp->opaddr[action==210?0:1].unoperand.soperand.adreg1.regno;
		if (pmureg != PMTC && pmureg != PMSRP)
	    	uerror("Illegal register for MOVEC");
		}
		break;

	/* cinvl {cache},(%an)		cinvp {cache},(%an)	*/
	case 215:
		if (instrp->opaddr[1].unoperand.soperand.admode != A_ABASE)
	    	uerror("Illegal <EA> for CINV");
		break;

	/* cpushl {cache},(%an)		cpushp {cache},(%an)	*/
	case 216:
		if (instrp->opaddr[1].unoperand.soperand.admode != A_ABASE)
	    	uerror("Illegal <EA> for CPUSH");
		break;

	/* mov16	(%an),{LABEL}	*/
	case 217:
		if (instrp->opaddr[0].unoperand.soperand.admode != A_ABASE)
	    	uerror("Illegal <EA> for MOV16");
		break;

	/* mov16	{LABEL},(%an)	*/
	case 218:
		if (instrp->opaddr[1].unoperand.soperand.admode != A_ABASE)
	    	uerror("Illegal <EA> for MOV16");
		break;

	/* pflush	(%an)	*/
	case 219:
		if (instrp->opaddr[0].unoperand.soperand.admode != A_ABASE)
	    	uerror("Illegal <EA> for PFLUSH");
		break;

	/* ptest	(%an)	*/
	case 220:
		if (instrp->opaddr[0].unoperand.soperand.admode != A_ABASE)
	    	uerror("Illegal <EA> for PFLUSH");
		break;
# endif	/* OFORTY */

# endif  /* M68851 */

	}
}

 
/* Check if the expression is of type TYPL and within the specified
 * range.  If so, return 1, else return 0.
 * If eflag is set, then give a warning if the check fails.
 */
int check_abs_range(exp, lo, high, eflag)
  rexpr * exp;
  int eflag;
  long lo, high;
{ long value;
  if (exp->exptype != TYPL) {
	if (eflag)
		werror("non absolute expression -- may be truncated");
	return(0);
	}
  else if ((value=exp->expval.lngval)>=lo && value <= high)
	return(1);

  else {
	if (eflag)
	   werror("absolute value out of range -- truncated");
	return(0);
	}
}


build_ea_bits(operand, ea, instrp, iop)
  register operand_addr * operand;
  register struct ea_bit_template * ea;
  struct instruction * instrp;
  int iop;
{
  ea->nwords = 0;
  switch ( operand->opadtype) {
	default:
		aerror("unknown operand type in build-ea");
		break;

	case A_SOPERAND:
		/* this is all the "normal" addressing modes. */
		{  register addrmode * paddr;
		   register unsigned int opadmode;
		   register int n;
		   int ismem = 0;
		   int ismemx = 0;

		   paddr = &(operand->unoperand.soperand);
		   opadmode = paddr->admode;

		   if (opadmode > A_MAX_EAMODE) {
				aerror("unexpected admode in build-ea");
				break;
				}
		   /* get ea_mode from table.  If ea_flag is true, the
		    * ea_reg comes from the regno in the operand, else
		    * its fixed, and is read from table.
		    */
		   ea->eamode = ea_encode[opadmode].ea_mode;
		   ea->eareg = ea_encode[opadmode].ea_rflag ?
			(paddr->adreg1.regkind==NOREG ? 0 : paddr->adreg1.regno)
			: ea_encode[opadmode].ea_reg;

		   switch(opadmode) {
			default:
				aerror("unknown soperand admode in build-ea");
				break;

				/* simple cases: no extension words. */
			case A_DREG:
			case A_AREG:
			case A_ABASE:
			case A_AINCR:
			case A_ADECR:
				ea->nwords = 0;
				break;

				/* simple format, with extension word(s) */
			case A_ADISP16:
				ea->nwords = 1;
				if (paddr->adexpr1.exptype==TYPNULL)
				  ea->extension_word[0] = 0;
				else
				  build_displacement(&(ea->extension_word[0]),
					&(paddr->adexpr1), SZWORD);
				break;
			case A_PCDISP16:
				ea->nwords = 1;
				if (paddr->adexpr1.exptype==TYPNULL)
				  ea->extension_word[0] = 0;
				else
				  build_branch_disp(&(ea->extension_word[0]),
					&(paddr->adexpr1), SZWORD, 0, instrp);
				break;

			case A_ABS16:
#ifdef PIC
				abs_flag = 1;
#endif
				ea->nwords = 1;
				build_displacement(&(ea->extension_word[0]),
					&(paddr->adexpr1), SZWORD);
#ifdef PIC
				abs_flag = 0;
#endif
				break;

			case A_ABS32:
#ifdef PIC
				abs_flag = 1;
#endif
				ea->nwords = 2;
				build_displacement(&(ea->extension_word[0]),
					&(paddr->adexpr1), SZLONG);
#ifdef PIC
				abs_flag = 0;
#endif
				break;
			case A_IMM:
#ifdef PIC
				immed_flag = 1;
#endif
				ea->nwords = 
				  build_imm_data(&(ea->extension_word[0]),
					&(paddr->adexpr1), paddr->adsize) / 2;
#ifdef PIC
				immed_flag = 0;
#endif
				break;

				/* Brief-20-fmt.  brief-fmt-baseword, no
				 * additional extension words.
				 */
			case A_AINDXD8:
			case A_PCINDXD8:
				{ register struct brief_ea * bea;
				  register xreg * xr;
				  bea = (struct brief_ea *) &(ea->extension_word[0]);
				  xr = &(paddr->adxreg);

				  /* set xreg type -- note that suppressed or
				   * missing xreg not allowed here.
				   */
				  bea->bea_xregtype = xr->xrreg.regkind ==AREG
				  	? 1:0;
				  bea->bea_xregno = xr->xrreg.regno;
				  bea->bea_xregsz = xr->xrsize==SZLONG
				  	? EA_XSZ_LONG : EA_XSZ_WORD;
				  bea->bea_xregscale = xr->xrscale;
				  bea->bea_cf1 = 0;
				  if (paddr->adexpr1.exptype==TYPNULL)
					bea->bea_disp8 = 0;
				  else if (opadmode==A_AINDXD8) {
					/* note: range check [-128..127] now
					 * done by build_displacement()
					 */
					build_displacement( (char *)bea + 1,
					    &(paddr->adexpr1), SZBYTE);
					}
				  else
					build_branch_disp( (char *)bea + 1,
					    &(paddr->adexpr1), SZBYTE, 0, instrp);

				  ea->nwords = 1;
				  break;
				}

#ifdef M68020
				/* Full-20-fmt.  Full-fmt-baseword, plus
				 * 0-5 extension words for displacements.
				 */
			case A_AMEM:
			case A_PCMEM:
				ismem = 1;
				goto full_format;
			case A_AMEMX:
			case A_PCMEMX:
				ismemx = 1;
				goto full_format;
			case A_AINDXBD:
			case A_PCINDXBD:
				full_format:
				{ register struct full_ea_baseword * fea;
				  register xreg * xr;
				  register unsigned int xrkind;
				  register int nw;
				  register unsigned int expsize;
				  int pcmode;

				  pcmode = (paddr->adreg1.regkind==PC);
				  fea = (struct full_ea_baseword *) &(ea->extension_word[0]);
				  xr = &(paddr->adxreg);
				  xrkind = xr->xrreg.regkind;

				  /* note that NOREG here defaults to ZD0;
				   * size field will be random (is that a
				   * problem??
				   */
				  if (xrkind == NOREG) {
					fea->fea_xregtype = 0;
					fea->fea_xregno = 0;
					fea->fea_xregsz =  0;
					fea->fea_xregscale = 0;
					}
				  else {
				  	fea->fea_xregtype = (xrkind==AREG
				  	  || xrkind==ZAREG) ? 1:0;
				  	fea->fea_xregno = xr->xrreg.regno;
				  	fea->fea_xregsz = xr->xrsize==SZLONG
				  	  ? EA_XSZ_LONG : EA_XSZ_WORD;
				  	fea->fea_xregscale = xr->xrscale;
					}
				  fea->fea_cf1 = 1;
				  fea->fea_bsuppress = (paddr->adreg1.regkind==
					AREG||paddr->adreg1.regkind==PC)?0:1;
				  fea->fea_isuppress = (xrkind==AREG ||
					xrkind==DREG) ? EA_XR_EVAL : EA_XR_SUPP;
				  fea->fea_cf2 = 0;

				  nw = 1;
				  /* base displacement */
				  expsize = paddr->adexpr1.expsize;
				  if (expsize!=SZNULL) {
				     if (pcmode)
				  	nw += build_branch_disp(&(ea->extension_word[nw]),
				   	&paddr->adexpr1, expsize, 0, instrp)/2;
				     else
					nw += build_displacement(&(ea->extension_word[nw]),
						&(paddr->adexpr1), expsize)/2;
				     }
				     fea->fea_bdsize = expsize==SZNULL ? EA_BDSZ_NULL :
					(expsize==SZWORD ? EA_BDSZ_WORD :
							   EA_BDSZ_LONG );

				  /* outer displacement */
				  if (! (ismem || ismemx) ) {
					/* no od in this case */
					fea->fea_iiselect = 0;
					fea->fea_odsize = 0;
					}
				  else {
					expsize = paddr->adexpr2.expsize;
					nw += build_displacement(&(ea->extension_word[nw]),
					   &(paddr->adexpr2), expsize)/2;
				  	fea->fea_odsize = expsize==SZNULL ? EA_ODSZ_NULL :
					   (expsize==SZWORD ? EA_ODSZ_WORD :
							   EA_ODSZ_LONG );
					fea->fea_iiselect = (fea->fea_isuppress 
					  ==EA_XR_EVAL)&&ismem ? 1 : 0 ;
					}

				  ea->nwords = nw;
				  break;
				}
#endif /* M68020 */

			}	/* switch opadmode */
			
		}	/* case A_SOPERAND */
	}	/* switch  operand->opadtype */

}

/* declare the codebuffer as long's to besure we get sufficient
 * alignment.
 */
#define CODEBUFSIZE 48
long codewords[CODEBUFSIZE/4];
char * codebuf = (char *)&codewords[0];
int codeindx = 0;

#define RELBUFSZ 4
#ifdef SEQ68K
struct reloc  relbuf[RELBUFSZ];
#else
struct r_info relbuf[RELBUFSZ];
#endif
int relindx = 0;

extern symbol * dot;

/*** !!!
 *** Modify the  way ea-bits are generated so that everything goes
 *** directly into the code buffer instead of "ea_template"
     Just keep a separate structure for the address mode/reg, but
     generate the auxilary addressing mode words directly into
     the codebuf.
     This will save the "memcpy" at the end of this procedure and
     make it possible to avoid the adjustment to the location field
     of fixup records.
  ***/

/* generate an effective address and then copy it into the codebuffer */
gen_ea_bits(operand, ea_template, instrp, iop)
  operand_addr * operand;
  struct ea_bit_template * ea_template;
  struct instruction * instrp;
  int iop;
{ int nbytes;
  int fixbase;
  int fi;
  long fixadjust;

  /* diagnostic check:  codeindx had better be on a word boundary within
   * the codebuf;
   */
  if (codeindx<0 || codeindx >= CODEBUFSIZE)
	aerror("codeindx out of range in gen-ea-bits");
  if (codeindx & 0x01)
	aerror("codeindx odd in gen-ea-bits");
  
  fixbase = fixindx;

  build_ea_bits(operand, ea_template, instrp, iop);

  /* if any relocation records were generated, fixup the address field */
  /* ?? can we modify the way fixup records are built so this adjustment
   * won't be necessary ???
   */
  if (fixbase < fixindx) {
	fixadjust = codeindx + (long) codebuf;
	for (fi=fixbase; fi<fixindx; fi++) {
	   /* diagnostic check */
	   if ((fixbuf[fi].f_location< (unsigned long)(&ea_template->extension_word[0]))

	      || (fixbuf[fi].f_location>= (unsigned long)(&ea_template->extension_word[5])) )
		aerror("bad preliminary reloc address");
	   fixbuf[fi].f_location -=  (long) (& ea_template->extension_word[0]);
	   fixbuf[fi].f_location += fixadjust;
	   }
	}

  if ( (nbytes = 2*ea_template->nwords)>0) {
	memcpy(&codebuf[codeindx], ea_template->extension_word, nbytes);
	codeindx += nbytes;
	}
  

}

/* build a displacement, and return a count of the number of
 * bytes generated.
 * exp is a pointer to the expression
 * loc is a pointer to where the output should go.
 *
 * The generic routine build_expr is called to do the actual work.
 *
 * SZBYTE and SZWORD do a range check here and warn if the expression
 * is out of bounds, or is symbolic.  A precise test would require
 * that a flag indicating a range check is desired is passed to build_expr,
 * and that for non-pass1 absolute expressions this info is passed via
 * a field in the fixup records to pass2 fixup_expr, and the range check
 * done then.
 */

int build_displacement(loc, exp, size) 
  char * loc;
  rexpr * exp;
  unsigned int size;
{ int nbytes;
  switch(size) {
	default:
		aerror("unexpected size in build_displacement");
		return(0);
	case SZNULL:
		return(0);
	case SZBYTE:		/* added for A_AINDX8 */
		nbytes = 1;
		/* NOTE: the following check is not needed on the as20 unless
		 * the -d option was used because the longer displacement would
		 * have automatically been selected.
		 */
# ifdef M68020
		if (sdispflag && !check_abs_range(exp, -128, 127, 0))
# else
		if (!check_abs_range(exp, -128, 127, 0))
# endif
			werror("8-bit displacement field may be truncated");
		break;
	case SZWORD:
		nbytes = 2;
		/* NOTE: the following check is not needed on the as20 unless
		 * the -d option was used.
		 */
# ifdef M68020
		if (sdispflag && !check_abs_range(exp, -32768, 32767, 0))
# else
		if (!check_abs_range(exp, -32768, 32767, 0))
# endif
			werror("16-bit displacement field may be truncated");
		break;
	case SZLONG:
		nbytes = 4;
		break;
	}
  if (nbytes != build_expr(loc, exp, size))
	aerror("build-displacement did not generate expected size expr");
  return(nbytes);

}


int build_imm_data(loc, exp, size) 
  char * loc;
  rexpr * exp;
  unsigned int size;
{ int nbytes;
  switch(size) {
	default:
		aerror("unexpected size in build_imm_data");
		return(0);
	case SZWORD:
	case SZLONG:
	case SZSINGLE:
	case SZDOUBLE:
	case SZEXTEND:
	case SZPACKED:
		nbytes = build_expr(loc, exp, size);
  		return(nbytes);
	case SZBYTE:
		*loc++ = 0;
		nbytes = build_expr(loc, exp, size);
		return(1+nbytes);
	}
}

extern short sdopt_flag;


/* convert the label expression to get a displacement from "dot".
 */
long branch_disp_val(exp, size,offset,piclocation)
  rexpr * exp;
  unsigned int size;
  long offset;
  long piclocation;
{
  long dispval;
  symbol * sym;
  sym = exp->symptr;

   if (exp->exptype & TYPNULL)
	return(0);

  if (exp->exptype&TDIFF)
	eval_tdiff(exp);

  if (exp->exptype&TINT) {
	/*werror("absolute displacement in branch");*/
	return(exp->expval.lngval);  /* no adjustment will be done */
	}
  else if ((exp->exptype&TSYM) && ((sym->stype&STYPE)==SABS)) {
	/*werror("absolute displacement in branch");*/
	/* no adjustment from "dot" will be done */
	return(exp->expval.lngval + sym->svalue);
	}
  else if (exp->exptype != TSYM) {
	uerror("illegal expression type for PC-relative displacement");
	return(0L);
	}

  /* the displacement value is "(dot + 2) - (label expression value)" */
  /* should that be newdot??? */
  if ((sym->stype&STYPE) != dot->stype) {
#ifdef PIC
        if (pic_flag) {
	   if (rpc_flag) {
              gen_reloc(
		 piclocation,
		 sym, 
		 size, 
	         RPC
	      );
	   }
	   else {
              gen_reloc(
		 piclocation,
		 sym, 
		 size, 
	         RPLT
	      );
	   }
	   return(0L);
	}
	else {
	   pic_flag = 1;
           gen_reloc(
	      piclocation,
	      sym, 
	      size, 
	      RPC
	   );
	   pic_flag = 0;
/*
 * no need to give a warning any more, 
 * the linker will handle the RPC fixup
 * just fine
	   werror("PC-relative target not in current assembly segment");
 */
	   return(0L);
	}
#else
	uerror("PC-relative target not in current assembly segment");
	return(0L);
#endif
	}

  dispval = (exp->expval.lngval + sym->svalue) - (dot->svalue + codeindx +
	offset);
  return(dispval);

}

extern int relent;

/* build a prelimnary relocation record in relbuf[].
 * the address points to the instruction-template being built and so
 * will need to be adjusted before being output.
 */
gen_reloc(offset, sym, size, pic_reloc) 
  long	offset;
  register symbol * sym;
  unsigned int size;
  int pic_reloc;
{
#ifdef SEQ68K
   register struct reloc * rel;
#else
   register struct r_info * rel;
#endif

   if (relindx >= RELBUFSZ)
	aerror("relbuf overflow");
   
   relent++;
   rel = &relbuf[relindx++];

#ifdef SEQ68K
   rel->rpos      = offset;
#else
   rel->r_address = offset;
#endif

#ifdef PIC
   if (pic_flag) {
#ifdef SEQ68K
      rel->rsymbol = sym->slstindex;
      rel->rsegment = pic_reloc;
#else
      rel->r_symbolnum = sym->slstindex;
      rel->r_segment = pic_reloc;
#endif
      if (pic_reloc == RDLT) {
         sym->got = 1;
      }
      else if (pic_reloc == RPLT) {
	 sym->plt = 1;
      }
      else if (pic_reloc == RPC) {
	 sym->pc = 1;
      }
      else if (pic_reloc == REXT) {
	 if ( dot->stype == STEXT) {
	    char buff[100];

            /*
	     * Don't complain about absolute use of fpa_loc, it is
	     * a constant which the linker knows
	     */

	    if (strcmp(sym->snamep,"fpa_loc")) {
	        sprintf(buff,
		    "Absolute address in text: symbol \"%s\"",sym->snamep);
	        werror(buff);
	    }
	 }
	 if (((sym->stype&STYPE) != SUNDEF) && 
	     !(sym->stype & (SEXTERN | SEXTERN2))) {
#ifdef SEQ68K
	    rel->rsymbol = 0;
	    rel->rsegment = reloc_segment(sym->stype & STYPE);
#else
	    rel->r_symbolnum = 0;
	    rel->r_segment = reloc_segment(sym->stype & STYPE);
#endif
	 }
	 else {
	    sym->ext = 1;
	 }
	 drelocs++;
      }
   }
   else {
      if ((size==SZALIGN) || 
	  ((sym->stype&STYPE)==SUNDEF) || 
	  (sym->stype&SALIGN) || 
	  (shlib_flag && (sym->stype & (SEXTERN | SEXTERN2)))) {
#else
      if ((size==SZALIGN) || ((sym->stype&STYPE)==SUNDEF) || 
	  (sym->stype&SALIGN)) { 
#endif
#ifdef SEQ68K
   	rel->rsymbol = sym->slstindex;
	rel->rsegment = REXT;
#else
   	rel->r_symbolnum = sym->slstindex;
	rel->r_segment = REXT;
#endif
	sym->ext = 1;
      }
      else {
#ifdef SEQ68K
   	rel->rsymbol = 0;
	rel->rsegment = reloc_segment(sym->stype & STYPE);
#else
   	rel->r_symbolnum = 0;
	rel->r_segment = reloc_segment(sym->stype & STYPE);
#endif
      }
#ifdef PIC
      drelocs++;
   }
#endif
#ifdef SEQ68K
   rel->rsize = reloc_size(size);
#else
   rel->r_length = reloc_size(size);
#endif
}


/* map a symbol's stype into the relocation segment type */
int reloc_segment(stype) 
   unsigned int stype;
{  int rsegment;
   switch(stype & STYPE) {
	default:
		aerror("unknown stype in gen_reloc");
	case STEXT:
		rsegment = RTEXT;
		break;
	case SDATA:
		rsegment = RDATA;
		break;
	case SBSS:
		rsegment = RBSS;
		break;
	case SUNDEF:
		rsegment = REXT;
		break;
	case SABS:
        case SGNTT:  case SLNTT:  case SSLT:  case SVT:  case SXT:
		uerror("illegal symbol type for relocation");
		rsegment = 0;
		break;
	}
   return(rsegment);
}


int reloc_size(size)
  int size;
{ int rsize;
   switch(size) {
	default:
		aerror("illeal size for relocation");
		break;
	case SZALIGN:
		/* this case just for special handling of alignment pseudo-op */
#ifdef SEQ68K
		uerror("Sequoia has no RALIGN relocation type");
#else
		rsize = RALIGN;
#endif
		break;
	case SZBYTE:
		werror("relocation of SZBYTE may be truncated");
		rsize = RBYTE;
		break;
	case SZWORD:
#ifdef PIC
                if (!pic_flag)
#endif
		werror("relocation of SZWORD may be truncated");
		rsize = RWORD;
		break;
	case SZLONG:
		rsize = RLONG;
		break;
	}
   return(rsize);
}

expvalue  expval;


/* Build an expression directly into the code buffer. */
gen_expr(exp, size)
  rexpr * exp;
  unsigned int size;
{ int nbytes;
  nbytes = build_expr(codebuf+codeindx, exp, size);
  codeindx += nbytes;
}


/* Build an immediate data type expression directly into the code buffer. */
gen_imm_data(exp, size)
  rexpr * exp;
  unsigned int size;
{ int nbytes;
  nbytes = build_imm_data(codebuf+codeindx, exp, size);
  codeindx += nbytes;
}


/* Extract the absolute value from an expression.  Give an error if
 * the expression is not absolute.
 * If the check flag is on, then also verify the range of the value.
 */
long abs_value(exp, chkflag, lo, hi)
  rexpr * exp;
  int chkflag;
  long lo, hi;
{
  unsigned int etype;
  long eval;

  if (exp->exptype&TDIFF)
	eval_tdiff(exp);

  etype = exp->exptype;
  if (etype == TYPL)
	eval = exp->expval.lngval;

  else if ((etype==TSYM) && ((exp->symptr->stype&STYPE)==SABS))
	eval = exp->expval.lngval + exp->symptr->svalue;

  else {
	uerror("absolute expression required");
	eval = 0;
	}

  if (chkflag && (eval < lo || eval > hi))
	werror("value out of range--truncated");

  return(eval);
}
	
# ifdef M68020
gen_bitfield_specifier(operand)
  operand_addr * operand;
{ struct fmtbfs * pbfs;
  pbfs = (struct fmtbfs *) &codebuf[codeindx];
  pbfs->bfs_cf1 = 0;
  if (operand->unoperand.soperand.adreg1.regkind==DREG) {
	pbfs->bfs_offtype =  1;
  	pbfs->bfs_offset = operand->unoperand.soperand.adreg1.regno;
	}
  else {
	pbfs->bfs_offtype = 0;
  	pbfs->bfs_offset = abs_value(&operand->unoperand.soperand.adexpr1,
		1, 0, 31);
	}
  if (operand->unoperand.soperand.adxreg.xrreg.regkind==DREG) {
  	pbfs->bfs_wdtype = 1;
  	pbfs->bfs_width = operand->unoperand.soperand.adxreg.xrreg.regno;
	}
  else {
  	pbfs->bfs_wdtype = 0;
	pbfs->bfs_width = abs_value(&operand->unoperand.soperand.adexpr2,
		1, 1, 32)&0x1f;
	}
  codeindx += 2;
}
# endif


/* if an expression is TDIFF, evaluate it (if possible) and remark it's
 * type.
 *	- if both symbols are relative and in the same segment, then
 *	  it can now be evaluated to absolute (TYPL).
 *	- if the second symbol is now SABS, then it can be marked as
 *	  a TSYM.  (<sym> - <abs>).
 *	- otherwise, we have an error.
 */
extern short passnbr;

eval_tdiff(exp) 
  rexpr * exp;
{ symbol * lsym, * rsym;
  /* If we are in pass1, a TDIFF is meant to last to pass2 fixup, even
   * if (we think) we could evaluate it now.
   */
  if (passnbr != 2)
	return;
  if (! (exp->exptype & TDIFF))
	return;

  lsym = exp->symptr;
  rsym = (symbol *) exp->expval.lngval;

  if ( ((lsym->stype&STYPE)!=SUNDEF) && ((lsym->stype&STYPE)==(rsym->stype
	&STYPE)) ) {
	if ( !(lsym->stype&(SABS|STEXT|SDATA|SBSS)) )
	   uerror("illegal symbol type for general expression");
	exp->exptype = TYPL;
	exp->expval.lngval = lsym->svalue - rsym->svalue;
	}
  else if ( (rsym->stype&STYPE)==SABS ) {
	exp->exptype = TSYM;
	exp->expval.lngval = - rsym->svalue;
	}
  else 
	uerror("illegal subtraction of symbols");
}

/* build a prelimnary fixup record in fixbuf[].
 * the address points to the instruction-template being built and so
 * will need to be adjusted before being output.
 */
gen_expr_fixup(offset, exp, size) 
  long	offset;
  register rexpr * exp;
  unsigned int size;
{
   register struct fixup_info * fix;

   if (fixindx >= FIXBUFSZ)
	aerror("fixbuf overflow");
   
   fixent++;
   fix = &fixbuf[fixindx++];

   fix->f_type = F_EXPR;
   fix->f_size = size;
   fix->f_location = offset;
   fix->f_dotval = dot->svalue;
#ifdef PIC
/*
 * For PIC: if the operand is immediate, the
 * pcoffs field of the fixup will be set to 
 * RPC. This will tell pass2 to generate an
 * RPC reloc rather than an RDLT reloc.
 */
   if (pic_flag) {
      if (data_flag || jump_flag || abs_flag) {
         fix->f_pcoffs = REXT;
         exp->symptr->ext = 1;
      }
      else if (immed_flag) {
         fix->f_pcoffs = RPC;
         exp->symptr->pc = 1;
      }
      else {
         fix->f_pcoffs = RDLT;
         exp->symptr->got = 1;
      }
   }
#else
   fix->f_pcoffs = 0;	/* not used */
#endif
   fix->f_lineno = line;
   fix->f_symptr = exp->symptr;
   fix->f_offset = exp->expval.lngval;
   fix->f_exptype = exp->exptype;
}


int build_bcc_branch_disp(loc, exp, size, instrp)
  char * loc;
  rexpr * exp;
  unsigned int size;
  struct instruction * instrp;
{ long dispval;
  dispval = 0;

  /* generate a fixup record */
  gen_bcc_fixup(loc, exp, size, instrp);

  switch(size) {
# ifdef SDOPT
	case SZNULL:
		if (sdopt_flag) {
			return( sdi_length[instrp->sdi_info->sdi_optype] [SDUNKN] 
			  - 1);
			}
		/* else fall through to default error */
# endif
	default:
		aerror("illegal size in build_bcc_branch_disp");
		return(0);
	case SZBYTE:
		*loc = (char) dispval;
		return(1);
	case SZWORD:
		*loc++ = 0;
		*(short *)loc = (short) dispval;
		return(3);
	case SZLONG:
# ifdef M68020
      
		*loc++ = (char) 0xff;

/*  If we are cross-compiling on the 800, we cannot do a simple assignment
 *  here.  The 800 demands 32 bit alignment for long words, and we don't 
 *  always have it here.  Hence, we do a memcpy instead of an assign    */
# ifdef xcomp300_800
	        memcpy(loc,&dispval,4);
# else
		*(long *)loc = dispval; 
# endif


		return(5);
# else
# ifdef SDOPT
		/* NOTE: this code currently dead.  It is left here
		 * for completeness since we could make the pass1 span
		 * code smarter.  Then some instructions (with TYPL target
		 * or nonsimple target) might already have been assigned
		 * a SZLONG size by this point.
		 */
		if (sdopt_flag) {
			/* convert to a branch around */
			short * piw;	/* hack to get back and
						 * modify branch.*/
			piw = (short *) (loc - 1);
			*piw++ ^= 0x0106;	/* bxx.b 6 */
			*piw++ = 0x4ec0 | 0x0039;		/* jmp xxx.l */
			build_displacement(piw, exp, SZLONG);
			return(7);
			}
		else
			/* shouldn't happen */
			aerror("long branch illegal");
# else
			aerror("long branch illegal");
# endif
# endif
	}
}

int build_branch_disp(loc, exp, size,offset, instrp)
  char * loc;
  rexpr * exp;
  unsigned int size;
  long offset;
  struct instruction * instrp;
{ long dispval;
  dispval = 0;

  /* generate a fixup record */
  gen_pcdisp_fixup(loc, exp, size, offset, instrp);

  switch(size) {
	case SZNULL:
# ifdef SDOPT
		if (sdopt_flag) {
			return(sdi_length[instrp->sdi_info->sdi_optype]
			  [SDUNKN]-2);
			}
		/* else fall through to default error */
# endif

	default:
		aerror("illegal size in build_branch_disp");
		return(0);
	case SZBYTE:
		*loc = (char) dispval;
		return(1);
	case SZWORD:
		*(short *)loc = (short) dispval;
		return(2);
	case SZLONG:
#ifdef xcomp300_800
		memcpy(loc,&dispval,4);  /* memcpy to avoid alignment problems */
#else
		*(long *)loc = dispval;
#endif
		return(4);
	}
}


gen_pcdisp_fixup(loc, exp, size, offset, instrp)
  char * loc;
  rexpr * exp;
  unsigned int size;
  long offset;
  struct instruction * instrp;
{ register struct fixup_info * fix;

  if (fixindx >= FIXBUFSZ)
	aerror("fixbuf overflow");
   
  fixent++;
  fix = &fixbuf[fixindx++];

  fix->f_type = F_PCDISP;
  fix->f_size = size;
  fix->f_exptype = exp->exptype;
  fix->f_pcoffs = codeindx + offset;
  fix->f_location = (long) loc;
  fix->f_dotval = dot->svalue;
  fix->f_lineno = line;
  fix->f_symptr = exp->symptr;
  fix->f_offset = exp->expval.lngval;
# ifdef SDOPT
  fix->f_sdiinfo = (sdopt_flag && size==SZNULL)? instrp->sdi_info : 0;
# endif
}


gen_bcc_fixup(loc, exp, size, instrp)
  char * loc;
  rexpr * exp;
  unsigned int size;
  struct instruction * instrp;
{ register struct fixup_info * fix;

  if (fixindx >= FIXBUFSZ)
	aerror("fixbuf overflow");
   
  fixent++;
  fix = &fixbuf[fixindx++];

  fix->f_type = F_BCCDISP;
  fix->f_size = size;
  fix->f_exptype = exp->exptype;
  fix->f_pcoffs = 2;
  fix->f_location = (long) loc - 1;
  fix->f_dotval = dot->svalue;
  fix->f_lineno = line;
  fix->f_symptr = exp->symptr;
  fix->f_offset = exp->expval.lngval;
# ifdef SDOPT
  if (sdopt_flag && size==SZNULL)
	fix->f_sdiinfo = instrp->sdi_info;
  else fix->f_sdiinfo = 0;
# endif
}

#if (defined(M68881) || defined(M68851)) && defined(SDOPT)

gen_cpbcc_fixup(loc, exp, instrp)
  char * loc;
  rexpr * exp;
  struct instruction * instrp;
{ register struct fixup_info * fix;

  if (fixindx >= FIXBUFSZ)
	aerror("fixbuf overflow");
   
  fixent++;
  fix = &fixbuf[fixindx++];

  fix->f_type = F_CPBCCDISP;
  fix->f_size = SZNULL;
  fix->f_exptype = exp->exptype;
  fix->f_pcoffs = 2;
  fix->f_location = (long) loc;
  fix->f_dotval = dot->svalue;
  fix->f_lineno = line;
  fix->f_symptr = exp->symptr;
  fix->f_offset = exp->expval.lngval;
  fix->f_sdiinfo = instrp->sdi_info;
}
# endif


int fix_bcc_branch_disp(fix, cbuf)
  struct fixup_info *fix;
  char *cbuf;
{ 
  char * loc;
  rexpr  expr;
  rexpr * exp = &expr;
  unsigned int size;
  long dispval;
  struct sdi * psdi;
  int sdioptype;

  loc = cbuf + 1;
  size = fix->f_size;
  exp->exptype = fix->f_exptype;
  exp->symptr = fix->f_symptr;
  exp->expval.lngval = fix->f_offset;
  /*dot->svalue = fix->f_dotval;*/
  codeindx = 0;

# ifdef SDOPT
  if (sdopt_flag && (psdi = fix->f_sdiinfo)) {
	size = map_sdisize(sdioptype=psdi->sdi_optype, psdi->sdi_size);
	if (size==SZNULL) {
	   /* bCC 0 and bra 0 become "nop" */
	   * (short *) cbuf = iopcode[I_NOP];
	   return(2);
	   }
# ifdef M68010
	else if (size==SZLONG) {
	   /* bsr ==> jsr <EA>
	    * bra ==> jmp <EA>
	    * and the <EA> must become an absolute.l access.
	    * For simplification we will only accept a symbolic
	    * address here because that's all the compiler generates
	    * anyway.
	    */
	    if (fix->f_exptype != TSYM) 
		uerror("target of span dependent operation must be symbolic");
	    if (sdioptype==SDBSR) {
		*(short *) cbuf = 0x4e80 | 0x0039; /* jsr xxx.l */
		/*build_displacement( (short *)cbuf+1, exp, SZLONG);*/
		fix->f_size = SZLONG;
		fix->f_location += 2;
		fix_expr( fix, cbuf+2 );
		return(6);
		}
	    if (sdioptype==SDBRA) {
		*(short *) cbuf = 0x4ec0 | 0x0039; /* jmp xxx.l */
		/*build_displacement( (short *)cbuf+1, exp, SZLONG);*/
		fix->f_size = SZLONG;
		fix->f_location += 2;
		fix_expr( fix, cbuf+2 );
		return(6);
		}
	    }
# endif /* M68010 */
	}
# endif /* SDOPT */
		

  dispval = branch_disp_val(exp, size, fix->f_pcoffs, fix->f_location + 2);

  switch(size) {
	default:
		aerror("illegal size in build_bcc_branch_disp");
		return(0);
	case SZBYTE:
		if (dispval<-128 || dispval > 127)
			uerror("branch displacement too large");
		if (dispval == 0)
			uerror("byte displacement cannot be 0");
		*loc = (char) dispval;
		return(2);
	case SZWORD:
		if (dispval<-32768 || dispval > 32767)
# if defined(SDOPT) && defined(M68010)
		   if (!sdopt_flag)
			uerror("branch displacement too large: try -O assembler option (compiler option -Wa,-O) (with no size on branch statement)");
		   else
# endif  /* SDOPT && M68010 */
			uerror("branch displacement too large");
		*loc++ = 0;
		*(short *)loc = (short) dispval;
		return(4);
	case SZLONG:
# ifdef M68020
		*loc++ = (char) 0xff;
#ifdef xcomp300_800
		memcpy(loc,&dispval,4); /* memcpy to avoid alignment problems */
#else
		*(long *)loc = dispval;
#endif
		return(6);
# else
# ifdef SDOPT
		if (sdopt_flag) {
			/* convert to a branch around */
			short * piw;	/* hack to get back and
						 * modify branch.*/
			piw = (short *) (loc - 1);
			*piw++ ^= 0x0106;	/* bxx.b 6 */
			*piw++ = 0x4ec0 | 0x0039;		/* jmp xxx.l */
			/*build_displacement(piw, exp, SZLONG);*/
			fix->f_location += 4;
			fix->f_size = SZLONG;
			fix_expr(fix, cbuf+4);
			return(8);
			}
		else
			/* shouldn't happen */
			aerror("long branch illegal");
# else
			aerror("long branch illegal");
# endif
# endif
	}
}

int fix_branch_disp(fix, cbuf)
  struct fixup_info *fix;
  char *cbuf;
{
  char * loc;
  rexpr  exp;
  unsigned int size;
  long offset;
  long dispval;
  struct sdi * psdi;
  int rpc_flag_save;

  loc = cbuf;
  size = fix->f_size;
  exp.exptype = fix->f_exptype;
  exp.symptr = fix->f_symptr;
  exp.expval.lngval = fix->f_offset;
  /*dot->svalue = fix->f_dotval;*/
  codeindx = 0;

# ifdef SDOPT
  if (sdopt_flag && (psdi = fix->f_sdiinfo)) {
	size = map_sdisize(psdi->sdi_optype, psdi->sdi_size);
	}
# endif

#ifdef PIC
   if (pic_flag) {
       rpc_flag_save = rpc_flag;
       rpc_flag = 1;
   }
#endif

  dispval = branch_disp_val(&exp, size, fix->f_pcoffs, fix->f_location);

#ifdef PIC
   if (pic_flag) {
       rpc_flag = rpc_flag_save;
   }
#endif

  switch(size) {
	default:
		aerror("illegal size in build_branch_disp");
		return(0);
	case SZNULL:
		return(0);
	case SZBYTE:
		if (dispval<-128 || dispval > 127)
			uerror("PC-relative or branch displacement too large");
		* loc = (char) dispval;
		return(1);
	case SZWORD:
		if (dispval<-32768 || dispval > 32767)
			uerror("PC-relative or branch displacement too large");
		*(short *)loc = (short) dispval;
		return(2);
	case SZLONG:
#ifdef xcomp300_800
		memcpy(loc,&dispval,4); /* memcpy to avoid alignment problems */
#else
		*(long *)loc = dispval;
#endif
		return(4);
	}
}


/* if the expression is TYPL (absolute), go ahead and generate the bits.
 * otherwise we need a fixup record.
 * For now, even with fixup we'll put "place holder" 0's into the output.
 */
/* ?? add an extra parameter to indicate whether we want to do range
 * checking, and if so, whether the check is signed or unsigned. ???
 */
/* ?? combine build_expr and fix_expr into a single routine, and check
 * passnbr to determine whether to generate fixup or relocation
 * information.
 */
int build_expr(ptr, exp, size)
  char *ptr;
  rexpr *exp;
  unsigned int size;
{ int nbytes = 0;
  int exptype;

  if (exp->exptype&(TINT|TFLT)) {
	/* generate the value, according to size */
	switch(size) {
	   default:
		aerror("illegal size in  gen-expr");
		break;

	   case SZBYTE:
		expval.bytval = exp->expval.lngval;
		nbytes = 1;
		goto inttype;
		break;
	   case SZWORD:
		expval.shtval = exp->expval.lngval;
		nbytes = 2;
		goto inttype;
		break;
	   case SZLONG:
		expval.lngval = exp->expval.lngval;
		nbytes = 4;
		goto inttype;
		break;
	   case SZSINGLE:
		/* Note: the following line does not appear to be portable
		 * code.  It dies for NAN's on a s500 because of extraneous
		 * converts between single and double.
		 */
		if ((exp->exptype & TINT)) {
			expval.lngval = exp->expval.lngval;
		}
		else {
			expval.fltval = exp->expval.fltval;
		}
		/****expval.fltval = exp->expval.dblval;****/
		nbytes = 4;
		goto flttype;
		break;
	   case SZDOUBLE:
		if ((exp->exptype & TINT)) {
			expval.lngpair.msw = 0;
			expval.lngpair.lsw = exp->expval.lngval;
		}
		else {
			expval.dblval = exp->expval.dblval;
		}
		nbytes = 8;
		goto flttype;
		break;
	   case SZEXTEND:
		expval.extval = exp->expval.extval;
		nbytes = 12;
		goto flttype;
		break;
	   case SZPACKED:
		expval.pckval = exp->expval.pckval;
		nbytes = 12;
		goto flttype;
		break;

	   inttype:
		if (!(exp->exptype & TINT))
			uerror("integer constant required");
		break;

	   flttype:
		break;
	   }
  	memcpy(ptr, &expval, nbytes);
	return(nbytes);
	}

   else {  long stype;
	   stype = exp->symptr->stype;
	   if ((stype&STYPE)!=SABS) {
		/* we need a fixup record */
		gen_expr_fixup(ptr,exp,size);
		}

# if DEBUG
	   /* generate "place-holder" bits */
	   /* ?? do we really need/want these ?? */
	expval.lngval = 0;
# endif
	switch(size) {
	   /* note only INT types can be symbolic at all, even in
	    * pass 1, since there is no "fset" capability.
	    */
	   default:
		aerror("illegal size in  gen-expr");
		break;

	   case SZBYTE:
# if DEBUG
		expval.bytval = 0;
# endif
		nbytes = 1;
		break;
	   case SZWORD:
# if DEBUG
		expval.shtval = 0;
# endif
		nbytes = 2;
		break;
	   case SZLONG:
# if DEBUG
		expval.lngval = 0;
# endif
		nbytes = 4;
		break;
	   }
# if DEBUG
  	   memcpy(ptr, &expval, nbytes);
# endif
  	   return(nbytes);
	}


}

int fix_expr(fix, cbuf)
   struct fixup_info *fix;
   char * cbuf;
{
  rexpr expr;
  rexpr *exp = &expr;
  unsigned int size;
  int nbytes = 0;

  size = fix->f_size;
  exp->exptype = fix->f_exptype;
  exp->symptr = fix->f_symptr;
  exp->expval.lngval = fix->f_offset;
  /*dot->svalue = fix->f_dotval;*/

  if (exp->exptype&TDIFF)
	eval_tdiff(exp);

  if (exp->exptype&TSYM) {
  	register long stype;
	/* Currently we only speak integer types here. */

	stype = exp->symptr->stype;
	if ((stype&STYPE)!=SABS) {
		/* we need a relocation record */
		gen_reloc(fix->f_location, fix->f_symptr, size, fix->f_pcoffs);
#ifdef PIC
                /*
		 * If pic, a segment relative fixup is
		 * generated iff the fixup type is not one 
		 * of the special pic fixups, and the 
		 * symbol is local. 
		 */
                if ((pic_flag && (fix->f_pcoffs != REXT)) ||
		    (shlib_flag && (((stype & STYPE) == SUNDEF) ||
		     (stype & (SEXTERN | SEXTERN2))))) {
		   goto copyval;
		}
#endif
		/* fall through to generate value part */
		}

	/* add the value of the symbol if the relocation would be
	 * segment relative.
	 * Note that an "SALIGN" symbol will use symbol relative 
	 * relocation even though it is defined.
	 */
	if ( ((stype&STYPE)!=SUNDEF) && !(stype&SALIGN) )
		exp->expval.lngval += exp->symptr->svalue;
	goto copyval;
	}

  copyval:
  /* generate the value, according to size */
  switch(size) {
	default:
		aerror("illegal size in  fixup-expr");
		break;

	case SZBYTE:
		expval.bytval = exp->expval.lngval;
		nbytes = 1;
		break;
	case SZWORD:
		expval.shtval = exp->expval.lngval;
		nbytes = 2;
		break;
	case SZLONG:
		expval.lngval = exp->expval.lngval;
		nbytes = 4;
		break;
	}

  memcpy(cbuf, &expval, nbytes);
  return(nbytes);


}
