/* @(#) $Revision: 70.2 $ */      
#include "o68.h"

/*
 * cc_used(where)
 *
 * See if the condition code's value is used before it's destroyed.
 */
cc_used(where)
node *where;
{
	/* search from here, max eight jumps */
	return is_used(where, 8);
}


is_used(where, jump_count)
node *where;
{
	register node *p;

	for (p=where; p!=NULL; p=p->forw) {
retry:
		switch (p->op) {

		case JMP:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (--jump_count<=0)
				return true;

			p=p->ref;
			goto retry;

		case CBR:
			return true;

		case JSR:
			/* We assume that subroutines destroy the code */
			return false;

		case RTS:
			/* We assume that subroutines don't return a code */
			return false;

		case EXG:
			break;			/* no effect */

		case AND:
		case ASL:
		case ASR:
		case BCHG:
		case BCLR:
		case BSET:
		case BTST:
		case CMP:
		case EOR:
		case OR:
		case ROL:
		case ROR:
			return false;
		
#ifdef M68020
		case BFCHG:
		case BFCLR:
		case BFEXTS:
		case BFEXTU:
		case BFFFO:
		case BFINS:
		case BFSET:
		case BFTST:
			return false;
#endif
		
		case MOVE:
		case ADD:
		case SUB:
			if (p->mode2==ADIR)
				break;
			else
				return false;


		case MOVEM:
			break;

		case NOT:
		case NEG:
		case EXT:
		case EXTB:
		case TST:
		case SWAP:
			return false;

		case DIVS:
		case DIVSL:
		case DIVU:
		case DIVUL:
		case MULS:
		case MULU:
			return false;

		case PEA:
		case LEA:
		case LINK:
		case UNLK:
			break;		/* no effect */
			
		case LABEL:
		case DLABEL:
			break;

		default:
#ifdef M68020
			/* 881 inst. won't effect  cond. code on the 68020 */
			if (isFOP(p->op))
				break;
#ifdef DRAGON
			/* dragon inst. won't effect  cond. code on the 68020 */
			if (isFPOP(p->op))
				break;
#endif DRAGON
#endif M68020

			return true;		/* assume used */
				/* What about scc instructions ? Does the
				 * compiler ever generate them ? Review. 
				 * Suhaib 
				 */
		}
	}

	/* Ran out of nodes?  Let's assume it's used. */
	return true;
}

#ifdef M68020
/*
 * fpcc_used(where)
 *
 * See if the floating point condition code's value is used before
 * it's destroyed.
 */
fpcc_used(where)
node *where;
{
	/* search from here, max eight jumps */
	return fp_is_used(where, 8);
}


fp_is_used(where, jump_count)
node *where;
{
	register node *p;

	for (p=where; p!=NULL; p=p->forw) {
retry:
		switch (p->op) {

		case JMP:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (--jump_count<=0)
				return true;

			p=p->ref;
			goto retry;

		case FBEQ:
		case FBGE:
		case FBGL:
		case FBGLE:
		case FBGT:
		case FBLE:
		case FBLT:
		case FBNEQ:
		case FBNGE:
		case FBNGL:
		case FBNGLE:
		case FBNGT:
		case FBNLE:
		case FBNLT:
			/* floating point condition code definitely used */
			return true;

		case FABS:
		case FACOS:
		case FADD:
		case FASIN:
		case FATAN:
		case FCMP:
		case FCOS:
		case FCOSH:
		case FDIV:
		case FETOX:
		case FINTRZ:
		case FLOG10:
		case FLOGN:
		case FMOD:
		case FMOV:
		case FMUL:
		case FNEG:
		case FSGLMUL:
		case FSGLDIV:
		case FSIN:
		case FSINH:
		case FSQRT:
		case FSUB:
		case FTAN:
		case FTANH:
		case FTEST:
			/* floating point condition code definitely not used */
			return false;

		case CBR:
		case FPBEQ:
		case FPBGE:
		case FPBGL:
		case FPBGLE:
		case FPBGT:
		case FPBLE:
		case FPBLT:
		case FPBNE:
		case FPBNGE:
		case FPBNGL:
		case FPBNGLE:
		case FPBNGT:
		case FPBNLE:
		case FPBNLT:
			/* It's not worth the time to chase two-way branches */
			return true;

		case JSR:
			/* We assume that subroutines destroy the code */
			return false;

		case RTS:
			/* We assume that subroutines don't return a code */
			return false;

		}
	}

	/* Ran out of nodes?  Let's assume it's used. */
	return true;
}
#endif M68020
