/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

node *release();

/*
 * Tidy things up.
 *
 * We may have performed certain changes that might have resulted in
 * optimizations, or they might not have.  It's time to undo the ones
 * that didn't work for the sake of nice looking stuff.
 */
tidy()
{
#ifndef M68020
	tidy_peas();		/* transform into pea's */
#endif
	tidy_moves();		/* do move byte/word directly */
}



#ifndef M68020
/*
 * tidy_peas()
 *
 *	moveq	#1,d0		==>	pea	#1.w
 *	move.l	d0,-(sp)
 */
tidy_peas()
{
	register node *p, *p2;
	register int temp_reg;

	for (p=first.forw; p!=NULL; p = p->forw) {

		/* must be moveq #n,dn */
		if (p->op==MOVEQ) {
			temp_reg = p->reg2;
			p2 = p->forw;

			if (p2!=NULL
			&& p2->op==MOVE && p2->subop==LONG
			&& p2->mode1==DDIR && p2->reg1==temp_reg
			&& p2->mode2==DEC && p2->reg2==reg_sp
			&& !reg_used(p2->forw, temp_reg)) {

				release(p2);
				p->op=PEA;
				p->subop=UNSIZED;
				p->mode1=ABS_W;
				cleanse(&p->op2);
				opt("recombination of pea with d%d", temp_reg);
			}
		}
	}
}

#endif

/*
 * tidy_moves()
 *
 *	moveq	#1,d0		==>	move.b	#1,zulu
 *	move.b	d0,zulu
 */
tidy_moves()
{
	register node *p, *p2;
	register int temp_reg;

	for (p=first.forw; p!=NULL; p = p->forw) {

		/* must be moveq #n,dn */
		if (p->op==MOVEQ) {
			temp_reg = p->reg2;
			p2 = p->forw;

			if (p2!=NULL
			&& p2->op==MOVE && p2->subop!=LONG
			&& p2->mode1==DDIR && p2->reg1==temp_reg
			&& !reg_used(p2->forw, temp_reg)) {

				p->op=MOVE;			/* not moveq */
				p->subop=p2->subop;
				copyarg(&p2->op2, &p->op2);
				release(p2);
				opt("recombination of move with d%d", temp_reg);
			}
		}
	}
}


#ifdef DRAGON
/*
 * combine_move_op()
 *
 *	For dragon code:
 *
 *      fpmov.X zz,%fpaR1       ==>  	fpmOP.X zz,%fpaR1,%fpaR2
 *	fpOP.X  %fpaR1,%fpaR2
 *
 *	except if fpOP is cmp, add, etc
 *
 *	fpmov.X zz,%fpaR2       ==>	fpm2OP.X zz,%fpaR1,%fpaR2
 *	fpOP.X	%fpaR1,%fpaR2
 */
combine_move_op()
{
	register node *p, *p2;
	register int temp_reg;
	register int i;

	for (p=first.forw; p!=NULL; p = p->forw)
	{
		/* must be fpmov */
		if (p->op==FPMOV && p->mode2==FPREG && p->mode1!=FPREG)
		{
			temp_reg = p->reg2;
			p2 = p->forw;

			if (p2!=NULL
			&& isFPOP(p2->op) && p2->subop==p->subop
			&& p2->mode1==FPREG 
			&& (p2->mode2==FPREG || fpmonadic(p2->op)))
			{
			   if (p2->reg1==temp_reg)
			   {
				i = get_move_op(p2->op);
				if (i == -1) continue;
				p->op = i;
				if (p2->mode2==FPREG)
				{
					p->mode2 = FPREGS;
					p->index2 = p2->reg2;
				}
				release(p2);
				
				/*
				 * now look for fpm2OPs 
				 * 
				 * fpmov.X R3,R2    ==> fpm2mul.X zz,R3,R2
				 * fpmmul.X zz,R1,R2
				 *
				 * is it OK to assume that R1 is not used?
				 *
				 * 87/4/17 : found out the hard way that 
				 * it is not OK to assume this. bug found
				 * at GE beta site where R1 was being used 
				 * later now. Remove code below by commeting
				 * out. (so that we do no attempt it again)
				 *
				 * 87/5/14 : unfortunately we lost too
				 * much in performance (5%) in the linpacks
				 * by removing this opt. Put it back but
				 * do not assume that R1 is not used. It
				 * so happens that in the linpack inner
				 * loop, the next instruction destroys R1
				 * via an fpmov. In that case this optimization
				 * is safe. So keep this around as a ifdef
				 * linpack if nothing else.
				 */

				 p2 = p->back;
				 if (p2->op == FPMOV && p2->subop==p->subop
				 && p2->mode1==FPREG && p2->mode2==FPREG
				 && p2->reg2==p->index2
				 && (p->op == FPMMUL || p->op == FPMADD)
				 && p->forw->op == FPMOV
				 && p->forw->reg2 == p->reg2)
				 {
				    i = get_move2_op(p->op);
				    if (i == -1) continue;
				    p->op = i;
				    p->reg2 = p2->reg1;
				    release(p2);
				 }
				 

			   }
			   else if (p2->reg2==temp_reg)
			   {
				i = get_move2_op(p2->op);
				if (i == -1) continue;
				p->op = i;
				p->mode2 = FPREGS;
				p->reg2 = p2->reg1;
				p->index2 = p2->reg2;
				release(p2);
			   }

			}
		}
	}

}

get_move_op(op)
{
	switch(op) {
	case FPABS:	return FPMABS;
	case FPADD:	return FPMADD;
	case FPCVD:	return FPMCVD;
	case FPCVL:	return FPMCVL;
	case FPCVS:	return FPMCVS;
	case FPDIV:	return FPMDIV;
	case FPINTRZ:	return FPMINTRZ;
	case FPMOV:	return FPMMOV;
	case FPMUL:	return FPMMUL;
	case FPNEG:	return FPMNEG;
	case FPSUB:	return FPMSUB;
	case FPTEST:	return FPMTEST;
	default:	return -1;
	}
}

get_move2_op(op)
{
	switch(op) {

	case FPADD:
	case FPMADD:	return FPM2ADD;

	case FPCMP:	return FPM2CMP;

	case FPDIV:
	case FPMDIV:	return FPM2DIV;

	case FPMUL:
	case FPMMUL:	return FPM2MUL;

	case FPSUB:
	case FPMSUB:	return FPM2SUB;

	default:	return -1;
	}
}
#endif DRAGON
