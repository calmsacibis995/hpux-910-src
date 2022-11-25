/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

node *release();

/*
 * Collapse add instructions.
 *
 * o Remember that all predecrements/postincrements have been broken
 *   up into preceding/following positive/negative adds.
 *
 * o Remember that all subtracts of constants have been mapped into
 *   adds of the opposite sign.
 */
collapse()
{
	register node *p, *p2, *p3;
	register int reg, op;

	for (p=first.forw; p!=NULL; p = p->forw) {

		/* we must have an add or move to an register */
		if ((p->op!=ADD && p->op!=MOVE) || 
		    p->mode2!=ADIR && p->mode2!=DDIR)
			continue;

		/* must be adding a constant */
		if (p->mode1!=IMMEDIATE || p->type1!=INTVAL)
			continue;

		reg = p->reg2;

		/* skip a series of meaningless instructions */
		for (p2=p->forw; ; p2=p2->forw) {
			if (p2==NULL)			/* ran out of nodes? */
				return;
			op = p2->op;
			if (op!=ADD && op!=MOVE && op!=AND && op!=OR && op!=EOR
			&& op!=EXT && op!=ASR && op!=ASL)
				break;
			if (reg_in_arg(&p2->op1, reg)
			|| reg_in_arg(&p2->op2,reg))
				break;
		}

		/* we must have an add to the same register */
		if (p2->op!=ADD || p2->mode2!=ADIR && p2->mode2!=DDIR
		|| p2->reg2!=reg)
			continue;

		/* must be same size ops */
		if (p->subop != p2->subop)
			continue;

		/* must be adding a constant */
		if (p2->mode1!=IMMEDIATE || p2->type1!=INTVAL)
			continue;

		/*
		 * Consider the case of:
		 *
		 *  ---external---	---internal---
		 *  addq    #8,sp	addq	#8,sp
		 *  move.l  #3,-(sp)	addq    #-4,sp
		 *			move.l  #3,(sp)
		 *
		 * We want to preserve the addq #-4,
		 * so we can make the move.l #3,(sp) into pea 3.w,
		 * which will save space.  (sigh).  What a special case.
		 *
		 * However, if it's move.l  #0,(sp) then we do collapse
		 * the adds, because we'll do the move with a clear.
		 */

		p3=p2->forw;
		if (p3==NULL) return;
		if (!oforty 
		&& p2->reg2==reg_sp
		&& p3->op==MOVE && p3->subop==LONG 
		&& p3->mode1==IMMEDIATE && p3->type1==INTVAL
		&& p3->addr1<=32767 && p3->addr1>=-32768 && p3->addr1!=0
		&& p3->mode2==IND && p3->reg2==reg_sp)
			continue;

		/* well, let's combine the two */
		p2->addr1 += p->addr1;

		if (p->op == MOVE) {
			p2->op = MOVE;
		}

		p=release(p);		/* nuke first add */
		if (p2->addr1==0 && p2->op != MOVE ) {	/* now useless */
			/*
			 * If this is an add to a data register,
			 * next instruction can't be a conditional branch.
			 */
			if (!isD(reg) || p3->op!=CBR)
				p2=release(p2);	/* nuke it also */
		}
		
		change_occured=true;
	}
}



/*
 * Compute the size of autodecrement/increment given subop & argument
 *
 * The tricky part is that byte accesses to the stack
 * increment the stack by two to keep it on a word boundary.
 */

size(p, arg)
register node *p;
register argument *arg;
{
	register int mask, count;

	switch (p->op) {
	case MOVEM:
		/* OK, which one is the register mask? */
		if (p->mode1==IMMEDIATE && p->type1==INTVAL)
			mask = p->addr1;
		else if (p->mode2==IMMEDIATE && p->type2==INTVAL)
			mask = p->addr2;
		else
			internal_error("size: movem has no mask?");
		
		/* Count the bits in the mask */
		for (count=0; mask!=0; mask>>=1)
			if (mask & 01)
				count++;

		/*
		 * Note that movem always pushes 4 bytes/register,
		 * even if it's movem.w (it extends the low word).
		 */
		return (4*count);

	case SCC: case SCS: case SEQ: case SF:
	case SGE: case SGT: case SHI: case SLE:
	case SLS: case SLT: case SMI: case SNE:
	case SPL: case ST:  case SVC: case SVS:
		return 1;		/* always byte */

	case BCHG:
	case BSET:
	case BCLR:
	case BTST:
		return p->mode2==DDIR ? 4 : 1;	/* long to dreg, else byte */

	case LEA:
	case PEA:
		return 4;		/* always long */
	}

	switch (p->subop) {
	case DOUBLE:
		return 8;
	case LONG:
	case SINGLE:
		return 4;
	case WORD:
		return 2;
	case BYTE:
		/* byte access to sp is word access */
		if (arg->reg==reg_sp)
			return 2;
		else
			return 1;
	default:
		/*
		 * Let's hope that this is a MOVE to or from SR or CCR.
		 */
		return 2;
	}
}
