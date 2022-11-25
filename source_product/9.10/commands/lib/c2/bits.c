/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

node *release();

/*
 * Handle bit tests.
 *
 * For example:
 *
 *	move.l	d6,d0		->	btst	#2,d6
 *	and.l	#4,d0		->
 *	jeq	away		->	jeq	away
 *
 * Also, we have to ascertain that d0 is unused.
 */
bit_test()
{
	register node *p, *p_and, *p_jump;
	register int temp_reg, bit;

	for (p=first.forw; p!=NULL; p = p->forw) {

		/*
		 * Must be move.l into a data register.
		 */
		if (!(p->op==MOVE && p->subop==LONG && p->mode2==DDIR)) {
			continue;
		}

		if (p->mode1 != IMMEDIATE) {
			/**** For the moment, it must be from a register ****/
			if (p->mode1!=DDIR)
				continue;
	
			/* this is our temporary register */
			temp_reg = p->reg2;
			
			p_and = p->forw;
			if (p_and==NULL)
				continue;
	
			/*
			 * Next instruction must be and.l #n,<temp_reg>
			 */
			if (!(p_and->op==AND && p_and->subop==LONG
			&& p_and->mode1==IMMEDIATE && p_and->type1==INTVAL
			&& p_and->mode2==DDIR && p_and->reg2==temp_reg))
				continue;

			/*
			 * nab the jump instruction
			 */
			p_jump = p_and->forw;
		}
		else {
			if (p->type1!=INTVAL) {
				continue;
			}

			/* this is our temporary register */
			temp_reg = p->reg2;
			
			p_and = p->forw;
			if (p_and==NULL)
				continue;
	
			/*
			 * Next instruction must be and.l %dn,<temp_reg>
			 */
			if (!(p_and->op==AND && p_and->subop==LONG &&
			      p_and->mode1==DDIR && p_and->mode2==DDIR && 
			      p_and->reg1!=temp_reg && p_and->reg2==temp_reg))
				continue;

			/*
			 * rearrange pointers so that the following 
			 * code works for both cases of this if
			 * and nab the jump instruction (p_jump is
			 * used as a temp first)
			 */

			p_jump = p;
			p      = p_and;
			p_and  = p_jump;
			p_jump = p->forw;
		}

		/* Must be a power of two (only testing one bit) */
		bit = bitnum(p_and->addr1);
		if (bit==-1)
			continue;

		/*
		 * Temporary register must be unused after this.
		 */
		if (p_jump==NULL)
			continue;

		if (reg_used(p_jump, temp_reg, p_and))
			continue;

		/*
		 * Next instruction must be jeq/jne
		 */
		if (!(p_jump->op==CBR
		&& (p_jump->subop==COND_EQ || p_jump->subop==COND_NE)))
			continue;

		if (cc_used(p_jump->forw) || cc_used(p_jump->ref))
			continue;

		/* ok, let's do it. */
		p->op = BTST;
		p->subop = UNSIZED;
		copyarg(&p->op1, &p->op2);
		make_arg(&p->op1, IMMEDIATE, INTVAL, bit);
		release(p_and);
		
		opt("and converted to bit test");
	}
}
