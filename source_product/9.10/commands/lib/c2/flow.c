/* @(#) $Revision: 70.1 $ */    
#include "o68.h"
node *ivloop();
node *release();
node *nonlab();
node *insert();
node *insertl();
node *codemove();

#define normal_jump(p) ((p)->op==JMP && (p)->mode1==ABS_L && (p)->type1==INTLAB)

/* # of common instructions that we apply comjump to */
#define COMJUMP_LENGTH 3

/*
 * Iterate looping for an opportunity for a flow-of-control optimization
 * and performing the optimization.
 */
flow_control()
{
	register node *p, *b;

	if (!flow_opts_ok)		/* are we allowed to do this? */
		return;

	/* for every node in the linked list do the following loop */

	for (p = first.forw; p!=NULL; FORW(p)) {

		/* if the opcode is a CBR, JMP or JSW with references */
		if ((p->op==CBR || p->op==JMP || p->op==JSW) && p->ref!=NULL)
			jump_to_jump(p);


		/* A jump to a return is just a return. */
		if (p->op==JMP && p->ref!=NULL
		&& p->ref->forw!=NULL && p->ref->forw->op==RTS) {
			p->op=RTS;
			decref(p->ref);
			p->ref = NULL;			/* we refer to none */
			cleanse(&p->op1);
			opt("Jump to return -> return");
			continue;
		}


		/*
		 * A conditional branch over a jump can be replaced
		 * by a single conditional branch.
		 */
		if (p->op==CBR && p->forw->op==JMP) {
			bugout("skip over jump");
			skip_over_jump(p);
			bugdump("9: after skip over jump, output is:");
		}

		if (p->op==JMP || p->op==RTS
		|| exit_exits &&
		   p->op==JSR && p->mode1==ABS_L && p->type1==STRING
		   && equstr(p->string1,"_exit")) {
			bugout("unreferenced instruction");
			unreferenced_instruction(p);
			bugdump("9: after unreferenced instruction, output is:");
		}

		if (p->op==JMP || p->op==CBR) {
			bugout("jump to next");
			jump_to_next(p);
			bugdump("9: after jump to next, output is:");
		}

		/*** now go check for cross jump optimizations */
		if (p->op == JMP) {
			bugout("cross jump");
			cross_jump(p);
			bugdump("9: after cross jump, output is:");
		}

		/*** now go check for code motion optimizations */
		if (p->op==JMP) {
			bugout("codemove");
			p = codemove(p);
			bugdump("9: after codemove, output:");
		}

		/*
		 * Consider the following case:
		 *
		 *	tst.l	alpha
		 *	jeq	L1
		 *	jmp	L2
		 *
		 * where L2 can't be moved but L1 can.
		 */
		b = p->back;
		if (b!=NULL
		&& b->op==CBR
		&& b->mode1==ABS_L
		&& b->type1==INTLAB
		&& normal_jump(p)) {
			register int old_ncmot, lab;

			old_ncmot = ncmot;

			/* Reverse the branches */
			b->subop = REVERSE_CONDITION(b->subop);
			lab = b->addr1; b->addr1=p->addr1; p->addr1=lab;

			/* try code motion on this */
			p = codemove(p);

			/* If no success, flip them back */
			if (ncmot==old_ncmot) {
				b->subop = REVERSE_CONDITION(b->subop);
				lab = b->addr1; b->addr1=p->addr1; p->addr1=lab;
			}
		}


		/*** now go check for loop inversion possibilities */
		if (p->op==JMP) {
			bugout("ivloop");
			p = ivloop(p);
			bugdump("9: after ivloop, output:");
		}

	}  /* end of for whole list */

}  /* end of flow_control() */




/*
 * Remove degenerate jumps and conditional branches.
 *
 * These are things that transfer to the next instruction.
 *
 * P points to a JMP/CBR node.
 */
jump_to_next(p)
register node *p;
{
	register node *l;
	register short op;

	/*
	 * Look at the (quite possibly null) string of labels that follows us.
	 * If any of these are our target, then we are useless.
	 * Skip over pseudo ops like GLOBAL, TEXT, LALIGN 1
	 * Terminate us at dawn.
	 */
	for (l=p->forw; l ;FORW(l)) {

		op = l->op;
		if (op==LABEL || op==DLABEL)
		{
		    if (p->ref == l)
		    { 
			p = release(p);			/* wipe out jmp/cbr */
			decref(l);			/* one less ref to l */
			njp1++;
			opt("jump to next instruction");
			break;
		    }
		}
		else if (op == TEXT || op == SET || (op == LALIGN && l->addr1==1)) continue;
		else if (op != GLOBAL) break;
	}

}




/*
 * Delete unreferenced instructions.
 * P points to a node that has transferred away
 * and isn't coming back, so we can delete instructions after it.
 */
unreferenced_instruction(p)
register node *p;
{
	/* while the following is true it's an unreferenced instruction */

	while (!pseudop(p->forw)) {
		iaftbr++;
		opt("instruction after jump");
		if (p->forw->ref)
			decref(p->forw->ref);
		release(p->forw);
	}
}




/*
 * We have a conditional branch followed by a jump.
 * See if it can be reduced to a single conditional branch.
 */
skip_over_jump(p)
register node *p;
{
	register node *p1, *target;

	target = p->ref;		/* where the CBR goes to */
	
	/* do we have a target at all? */
	if (target==NULL)
		return;

	/* slide back to the first label target */
	while (target->back!=NULL && target->back->op==LABEL)
		  BACK(target);

	/* are we jumping just over the unconditional jump? */
	if (target->back!=p->forw)		/* if not */
		return;				/* give up */
		
	/* go ahead and do it */
	p1 = p->forw;			/* node after the CBR */
	decref(p->ref);
	p->ref = p1->ref;
	p->labno1 = p1->labno1;
	p1->forw->back = p;
	p->forw = p1->forw;
	p->subop = REVERSE_CONDITION(p->subop);
	release(p1);
	nskip++;
	opt("skip over jump");
}




/*
 * Find jumps to jumps.
 *
 * It is assumed that p points to a JMP/CBR/JSW with references.
 */
jump_to_jump(p)
register node *p;
{
	register node *rp;

	rp = nonlab(p->ref);		/* find the next non-label */
	if (rp==NULL)			/* If we run out of source: */
		return;			/* give up. */

	/*
	 * Well, we must have a jump.
	 */
	if (rp->op!=JMP)
		return;

	/*
	 * And it must be jumping to a compiler-generated label.
	 */
	if (rp->type1!=INTLAB)
		return;
		
	/*
	 * And our target can't be jumping to itself.
	 */
	if (p->labno1==rp->labno1)
		return;

	/*
	 * Looks good, let's do it.
	 */
	nbrbr++;
	opt("jump L%d to jump L%d", p->labno1, rp->labno1);
	p->labno1 = rp->labno1;
	decref(p->ref);
	if (rp->ref!=NULL)		/* If target's target existed: */
		rp->ref->refc++;	/* Increment its reference count */
	p->ref = rp->ref;
}




cross_jump(p1)
register node *p1;
/*
 * Look for cross jump optimization opportunities
 *
 * I.e. test this node to see if it is a jump where the preceding
 * instruction is the same as the instruction preceding the destination.
 *
 * If so, turn the instruction preceding the jump into a jump to the
 * instruction before the original destination.	 The original jump
 * will be eliminated later because it is now an unreachable instruction.
 *
 * For example:
 *
 *		move.l	d0,d1			jmp	L2001
 *		jmp	L23			jmp	L23
 *		...				...
 *		move.l	d0,d1		L2001	move.l	d0,d1
 *	L23	...			L23	...
 */
{
	register node *p2, *p3;

	if (!code_movement_ok)		/* are we allowed to do this? */
		return ;
	p2 = p1->ref;			/* where we refer to */
	if (p2==NULL)			/* if we refer to nobody */
		return;

	for (;;) {
		while (BACK(p1)!=NULL && p1->op==LABEL)
			;
		if (p1==NULL)			/* hit start of list? */
			return;
		while (BACK(p2)!=NULL && p2->op==LABEL)
			;
		if (p2==NULL)
			return;			/* hit start of list? */
		if (p1==p2)			/* the same node? */
			return;
		if (!equop(p1, p2))		/* must be same stuff */
			return;
		p3 = insertl(p2);
		/* transform p1 into a jmp */
		cleanse(&p1->op1);
		cleanse(&p1->op2);
		p1->op = JMP;
		p1->subop = UNSIZED;
		p1->ref = p3;
		p1->mode1 = ABS_L;
		p1->type1 = INTLAB;
		p1->labno1 = p3->labno1;
		nxjump++;
		opt("cross jump");
	}
}




/*
 *	check for code motion optimization opportunities
 *	including:	code motion
 *			loop inversion
 *
 *  Code motion looks for situations such as:
 *
 *		jra L1
 *
 *	     L2 aaa
 *		jra L3				  bbb
 *			  and turns them into:	  aaa
 *	     L1 bbb				  ccc
 *		jra L2
 *
 *	     L3 ccc
 *
 *  by yanking the code at L1 and putting it after the jra L1.
 *
 *  Our argument points to a JMP node.
 */

node *
codemove(p1)
register node *p1;
{
	register node *start_block, *end_block;

	if (!code_movement_ok)		/* are we allowed to do this? */
		return p1;

	/* We must be a jump */
	if (!normal_jump(p1))
		return p1;
		
	/*
	 * Find how the block before our target ends.
	 */
	for (start_block=p1->ref;
		start_block!=NULL && start_block->op==LABEL; BACK(start_block))
			;

	if (start_block==NULL)
		return p1;

	/*
	 * This block must be flow-disconnected to the previous one.
	 * (That is, the previous block must not run into this one.)
	 */
	if (! (start_block->op==RTS || normal_jump(start_block)))
		return p1;

	FORW(start_block);		/* now pointing to start of block */
	
	/* Find the end of the target block */
	for (end_block=start_block; end_block!=NULL; FORW(end_block)) {
		if (normal_jump(end_block) || end_block->op==RTS)
			break;
	}

	/* If we fell off the end of the code, do nothing */
	if (end_block==NULL)
		return p1;

	/*
	 * Ok, we must have ended in a jump or a return.
	 */

	/* If we're jumping to ourselves, do nothing */
	if (end_block==p1)
		return p1;

	ncmot++;
	opt("code motion");

	/* remove the block from the code altogether */
	if(start_block->back != NULL)
	start_block->back->forw = end_block->forw;   /* pre-start to post-end */
	if(end_block->forw != NULL)
	end_block->forw->back = start_block->back;   /* post-end to pre-start */

	start_block->back = p1;
	end_block->forw = p1->forw;

	if(p1->forw != NULL) p1->forw->back = end_block;
	p1->forw = start_block;

	decref(p1->ref);		/* one less reference to the label */
	return release(p1);		/* remove the jump */
}




/*
 * Code inversion looks for situations such as:
 *
 *	L1  aaa					     jra L1
 *	    jeq L2    and turns them into:   L20001  bbb
 *	    bbb				     L1	     aaa
 *	    jra L1				     jne L20001
 *	L2
 *
 *  This permutation will sometimes enable some other optimization that
 *  was previously hidden.
 *
 *  However, consider the case of the loop that is never (or rarely) executed.
 *  This technique will force it to always execute two jumps, not just one.
 *  We assume that loops of this sort are rare enough not to worry about.
 */
node *
ivloop(ap)
register node *ap;
{
	register node *p1, *p3;
	register node *target;
	register node *t, *tl;
	register int n;

	p1 = ap;

	if (!invert_loops)		/* are we allowed to do this? */
		return(p1);

	/* We must be a jump */
	if (!normal_jump(p1))
		return (p1);
		
	/* We must be jumping to somewhere */
	target = p1->ref;
	if (target==NULL)
		return(p1);

	/* Pull target back to before the labels */
	while (target->op == LABEL)
		if (BACK(target) == NULL)
			return(p1);

	if (target->op == JMP)
		return(p1);

	if (p1->forw->op!=LABEL) return(p1);
	p3 = FORW(target);
	if (p3==NULL)
		return (p1);
	n = 16;

	do {
		if (FORW(p3) == NULL || p3==p1 || --n==0)
			return(p1);
	} while (p3->op!=CBR || p3->labno1!=p1->forw->labno1);

	do
		if (BACK(p1)==NULL)
			return(ap);
	while (p1!=p3);

	p1 = ap;
	tl = insertl(p1);
	p3->subop = REVERSE_CONDITION(p3->subop);
	decref(p3->ref);
	target->back->forw = p1;
	p3->forw->back = p1;
	p1->back->forw = target;
	p1->forw->back = p3;
	t = p1->back;
	p1->back = target->back;
	target->back = t;
	t = p1->forw;
	p1->forw = p3->forw;
	p3->forw = t;
	target = insertl(p1->forw);
	p3->mode1 = ABS_L;
	p3->type1 = INTLAB;
	p3->labno1 = target->labno1;
	p3->ref = target;
	decref(tl);
	if (tl->refc<=0)
		nrlab--;
	loopiv++;
	opt("loop inversion");
	return(p3);
}


comjump()
{
	register node *p1, *p2;
	register node *target;		/* destination of both jumps */
	register short op_jmp = JMP;

	for (p1 = first.forw; p1!=NULL; FORW(p1))	/* for whole list */
		if (p1->op==op_jmp && (target = p1->ref) && target->refc > 1)
			for (p2 = p1->forw; p2!=NULL; FORW(p2))
				if (p2->op==op_jmp && p2->ref == target)
					backjmp(p1, p2);
}


/*
 * Both p1 & p2 point to a JMP node.
 * Look for a common sequence before the two JMP's and combine them.
 */
backjmp(p1, p2)
register node *p1, *p2;
{
	register node *newlab;
	register node *p22;
	register int i;

	i = 0;
	p22 = p2;
	for(;;) {
		/* back up past any labels */
		while (BACK(p1)!=NULL && p1->op==LABEL)
			;

		/* why not back up past labels for p2? */
		BACK(p2);

		/* are the previous instructions the same? */
		if (!equop(p1, p2))
			break;
		
		i++;

	}

	/*
	 * common sequence must be atleast of some length to make
	 * this worthwhile.
	 */
	if (i < COMJUMP_LENGTH) return;

	p1 = p1->forw;
	p2 = p2->forw;

	while (p2->forw != p22)
		p2 = release(p2)->forw;

	newlab = insertl(p1);
	p2 = release(p2)->forw;
	decref(p2->ref);
	cleanse(&p2->op1);
	cleanse(&p2->op2);
	p2->mode1 = ABS_L;
	p2->type1 = INTLAB;
	p2->labno1 = newlab->labno1;
	p2->ref = newlab;
	ncomj++;
	opt("common sequence before jump");
}


/*
 * This optimization looks for situations in which a conditional
 * branch is followed by an uncondtional one.  If found, the routine
 * then computes the distance (in number of nodes) from each branch
 * to its destination.	If the conditional branch is going farther
 * than the unconditional (and they're not going the same place),
 * the sense of the conditional branch is reversed and the destinations
 * are swapped.
 */
jumpsw()
{
	register node *p, *p1, *temp;
	register t;

	t = 0;

	/*
	 * note: value in refc is destroyed here.
	 * refc is not used here as a reference count
	 * for # of nodes jumping to a label
	 */
	for (p=first.forw; p!=NULL; FORW(p))
		p->refc = ++t;

	for (p=first.forw; p!=NULL; p = p1) {
		p1 = p->forw;

		if (p->op == CBR && p1->op==JMP
		&& p->ref && p1->ref
		&& abs(p->refc - p->ref->refc)
		   > abs(p1->refc - p1->ref->refc)) {
			if (p->ref==p1->ref)
				continue;

			p->subop = REVERSE_CONDITION(p->subop);

			temp = p1->ref;
			p1->ref = p->ref;
			p->ref = temp;
			t = p1->labno1;
			p1->labno1 = p->labno1;
			p->labno1 = t;
			nrevbr++;
			opt("reversed branch");
		}
	}
	return;
}

lalign_bras()
{
	node *p,*q;

	for (p = first.forw; p!=NULL; FORW(p)) {
		if (p->op == JMP && p->mode1 == ABS_L && p->forw) {
			q = insert(p->forw,LALIGN);
			q->arg[0].mode = ABS_L;
			q->arg[0].type = INTVAL;
			q->arg[0].sub_arg.u_addr = 8;
		}
	}
}
