/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

#define reg_fpa1 reg_fpa0+1
#define reg_fpa2 reg_fpa0+2
#define reg_fpa7 reg_fpa0+7
node *release();
node *nonlab();

/*
 * Move things to their ultimate destination.
 *
 * For example:
 *
 *	move.l	d6,d0		->	move.l	d6,d7
 *	add.l	d5,d0		->	add.l	d5,d7
 *	add.l	#37,d0		->	add.l	#37,d7
 *	move.l	d0,d7		-> * already in d7!
 *
 * Also, we have to ascertain that d0 is unused.
 */
ultimate_destination()
{
	register node *p, *p2, *ptr;
	register int temp_reg, dest_reg, op;

	for (p=first.forw; p!=NULL; p = p->forw) {

		/* must be move.l */
		if (p->op!=MOVE || p->subop!=LONG)
			continue;

		/* must move into a register */
		if (p->mode2!=ADIR && p->mode2!=DDIR)
			continue;

		/* this is our temporary register */
		temp_reg = p->reg2;
		
		/* a sequence of instructions on temp_reg follows */
		for (p2=p->forw; p2!=NULL ;p2=p2->forw) {
			op = p2->op;
			/* Is it an irrevelant instruction? */
			if ((op==MOVE || op==ADD || op==SUB
			    || op==AND || op==OR || op==EOR
			    || op==ASR || op==ASL 
			    || op==MULS || op==DIVS
			    || op==MULU || op==DIVU)
			&& (p2->mode1!=ADIR && p2->mode1!=DDIR
			    || p2->reg1!=temp_reg)
			&& (p2->mode2!=ADIR && p2->mode2!=DDIR
			    || p2->reg2!=temp_reg))
				continue;

			/* Is it an instruction on temp_reg? */
			if (p2->subop!=LONG)
				break;

			if (p2->mode2!=ADIR && p2->mode2!=DDIR)
				break;

			if (p2->reg2 !=	temp_reg)
				break;

			if (op!=ADD && op!=SUB && op!=AND && op!=OR
			    && op!=EOR && op!=ASR && op!=ASL 
			    && op!=MULS && op!=DIVS
			    && op!=MULU && op!=DIVU)
				break;
		}
		if (p2==NULL)
			continue;

		/* now we must have a move of temp_reg to dest_reg */
		if (p2->op!=MOVE || p2->subop!=LONG)
			continue;
		if (p2->mode1!=ADIR && p2->mode1!=DDIR)
			continue;
		if (p2->mode2!=ADIR && p2->mode2!=DDIR)
			continue;
		if (p2->reg1!=temp_reg)
			continue;
		dest_reg=p2->reg2;

		/*
		 * Allow things like:
		 *
		 * 	moveq	#3,d0
		 *	move.l	d0,a4
		 *
		 * to remain because the moveq is cheap.
		 */
		if (isD(temp_reg) && isA(dest_reg)
		&& p->mode1==IMMEDIATE && p->type1==INTVAL
		&& EIGHT_BITTABLE(p->addr1)
		&& p->forw==p2)
			continue;

		/* dest_reg must be greater than temp_reg */
		/* cannot replace an areg by dreg */
		if (!(dest_reg>temp_reg))
			continue;

		/*
		 * If the destination is an address register,
		 * the field of instructions is limited.
		 */
		if (isA(dest_reg)) {
			for (ptr=p->forw; ptr!=p2; ptr=ptr->forw) {
				/* If the instruction applies to temp_reg: */
				if (ptr->mode2!=ADIR && ptr->mode2!=DDIR)
					continue;
				if (ptr->reg2!=temp_reg)
					continue;
				op = ptr->op;
				if (op==AND || op==OR
				|| op==EOR || op==ASR || op==ASL
				|| op==MULS || op==DIVS
				|| op==MULU || op==DIVU)
					goto next_try;
			}
		}

		/* none of the instructions must refer to dest_reg */
		/* should not have MOVEM in this sequence */
		for (ptr=p->forw; ptr!=p2; ptr=ptr->forw)
		{
			if (reg_in_arg(&ptr->op1, dest_reg))
				goto next_try;
			if (reg_in_arg(&ptr->op2, dest_reg))
				goto next_try;
			if (ptr->op == MOVEM)
				goto next_try;
		}


		/* fix first instrction */
		p->mode2=isA(dest_reg) ? ADIR : DDIR;
		p->reg2=dest_reg;	/* right to dest */
		p = p->forw;
		/* ok, it's a sequence.  Change destination registers */
		/* this includes initial move, also! */
		for (; p!=p2; p=p->forw) {
			if ((p->mode2==ADIR || p->mode2==DDIR)
			&& p->reg2==temp_reg) {	  /* if applies to temp_reg */
				p->mode2=isA(dest_reg) ? ADIR : DDIR;
				p->reg2=dest_reg;	/* right to dest */
			}
			if (p->reg2==temp_reg) p->reg2 = dest_reg;
			if (p->index2==temp_reg) p->index2 = dest_reg;
			if ((p->mode1==ADIR || p->mode1==DDIR)
			&& p->reg1==temp_reg) {	  /* if applies to temp_reg */
				p->mode1=isA(dest_reg) ? ADIR : DDIR;
				p->reg1=dest_reg;	/* right to dest */
			}
			if (p->reg1==temp_reg) p->reg1 = dest_reg;
			if (p->index1==temp_reg) p->index1 = dest_reg;
		}

		/* change final move to move dest_reg,temp_reg */
		/* Which may get removed later */
		p2->mode1=isA(dest_reg) ? ADIR : DDIR;
		p2->reg1 = dest_reg;
		p2->mode2=isA(temp_reg) ? ADIR : DDIR;
		p2->reg2 = temp_reg;

		/* something has happened */
		opt("move to ultimate destination");

	next_try:;
	}
}

#ifdef M68020
/*
 * Move things to their ultimate 68881 destination.
 *
 * For example:
 *
 *	fmov.x	%fp4,%fp0	->	
 *	fadd.x	%fp7,%fp0	->	fadd.x	%fp7,%fp4
 *	fadd.x	%fp6,%fp0	->	fadd.x	%fp6,%fp4
 *	fsub.x	%fp5,%fp0	->	fsub.x	%fp5,%fp4
 *	fmul.x	%fp3,%fp0	->	fmul.x	%fp3,%fp4
 *	fmov.x	%fp0,%fp4	->	fmov.x	%fp4,%fp0
 *
 * 
 */
ultimate_881_destination()
{
	register node *p, *p2, *ptr;
	register int temp_reg, dest_reg, op;

	for (p=first.forw; p!=NULL; p = p->forw) {

	        if (p->op == UNLK)
		{
		  if (tail_881)
		    tail_881_optimization(p);
		  continue;
		}
		/* must be fmov.x */
		if (p->op!=FMOV || p->subop!=EXTENDED)
			continue;

		/* must move from a scratch register */
		if ((p->reg1 != reg_fp0) && (p->reg1 != reg_fp0 + 1))
			continue;

		/* this is our temporary register */
		temp_reg = p->reg1;
		dest_reg = p->reg2;
		
		/* a sequence of instructions on temp_reg is before this */
		for (p2=p->back; p2!=NULL ;p2=p2->back) {
			if (fmonadic(p2->op) && p2->reg1== temp_reg)
			        continue;
			if (p2->reg2 != temp_reg)
				break;
		}
		if (p2==NULL)
			continue;

		p2 = p2->forw;

		/* now we must have a move to temp_reg */
		if (p2->op!=FMOV)
			continue;
		if (p2->reg2!=temp_reg)
			continue;
		if (p2->mode1== FREG && p2->reg1!=dest_reg)
		{
		        ptr = p2->forw;
		        if (ptr!= NULL && ptr->op==FADD 
			    && ptr->reg1==dest_reg)
			{
			  /* swap p2 and ptr */
			  ptr->reg1 = p2->reg1;
			  p2->reg1  = dest_reg;
			}
		}

		/* none of the instructions must refer to dest_reg */
		/* should not have FMOVM in this sequence */
		for (ptr=p2->forw; ptr!=p; ptr=ptr->forw)
		{
			if (reg_in_arg(&ptr->op1, dest_reg))
				goto next_try;
			if (reg_in_arg(&ptr->op2, dest_reg))
				goto next_try;
			if (ptr->op == FMOVM)
				goto next_try;
		}


		/* fix first instrction */
		p2->reg2=dest_reg;	/* right to dest */
		p2 = p2->forw;
		/* ok, it's a sequence.  Change destination registers */
		/* this includes initial move, also! */
		for (; p2!=p; p2=p2->forw) 
		{
		        if (p2->reg1==temp_reg) p2->reg1 = dest_reg;
			p2->reg2=dest_reg;	/* right to dest */
		}

		/* change final move to move dest_reg,temp_reg */
		/* Which may get removed later */
		p->reg1 = dest_reg;
		p->reg2 = temp_reg;

		/* something has happened */
		opt("move to 881 ultimate destination");

	next_try:;
	}
}

/* #ifdef b1d ??? - or is this more general */
/* orginally writing code with dp3 in mind (in b1d) - generalize later */
tail_881_optimization(nodep)
node *nodep;
{

  register node *p;
  register node *q;
  register node *ptr;
  register int  dest_reg;
  int fpr;

  p = nodep->back;
  if (p->op != FMOVM) 
    return;

  for (p = p->back; p != NULL; p = p->back)
  {
    if (isFOP(p->op)) break;
    if (p->op == LABEL || p->op==CBR || p->op==JMP) break;
  }
  if (p->op != FMOV)
    return;

  fpr = p->reg1;

  if ((fpr != reg_fp0) && (fpr != reg_fp0)) 
    return;

  for (q = p->back; q != NULL; q = q->back)
  {
    if (isFOP(q->op))
    {
      if (q->reg2 != fpr) break;
      else continue;
    }
    if (q->op == MOVE) 
	continue;
    else
	break;
  }

  q = q->forw;

  if (q->op!= FMOV || q->reg2 != fpr)
    return;

  if (q->mode1 != FREG)
    return;

  if (p==q) return;

  dest_reg = q->reg1;

  for (ptr=q->forw; ptr != p; ptr = ptr->forw)
  {
    if (isFOP(ptr->op) && ptr->reg1==dest_reg)
      return;
  }


  /* Hopefully now we are all set for the optimization */
  q = release(q);
  q = q->forw;
  
  for (; q != p; q = q->forw)
  {
    if (isFOP(q->op)) q->reg2 = dest_reg;
  }

  p->reg1 = dest_reg;

  opt("tail 881 optmization");

}

/*
 * if a routine does not use fp0 or fp1, but it does use other
 * 68881 registers and it does not have any subroutine calls, then
 * we can use the scratch registers (fp0/fp1) instead of other
 * 881 registers. This helps us eliminate reg save/restore via
 * fmovm instructions. This is a big help in the b1/b1d benchmarks
 *
 * can be made more efficient 
 * this routine is not called for fortran code with alternate entry points.
 */
fmovm_reduce()
{
	register node *p;
	register int mask;
	register int new_mask;
	register int used0;
	register int used1;
	register int reg;
	register int reg_fp1;


	mask = 0;
	used0 = 0;
	used1 = 0;

	reg_fp1 = reg_fp0 + 1;

	for (p=first.forw; p!=NULL; p = p->forw) 
	{

	  if (p->op == JSR) return;

	  if (isFOP(p->op))
	  {
	    if (p->op == FMOVM && p->mode1 == IMMEDIATE && p->type1 == INTVAL)
	      mask = p->addr1;
	    if (p->reg1 == reg_fp0 || p->reg2 == reg_fp0) 
	      used0 = 1;
	    if (p->reg1 == reg_fp1 || p->reg2 == reg_fp1) 
	      used1 = 1;
	    if (used0 && used1)
	      return;
	  }

	}

	if (mask == 0) return;
	new_mask = mask;

	reg = reg_fp7;
	while ((mask & 01) != 1)
	{
	  mask >>= 1;
	  reg--;
	}

	if (!used1)
	{
	  new_mask -= 1 << (7 - (reg - reg_fp0));

	  for (p=first.forw; p!=NULL; p = p->forw) 
	  {
	    if (isFOP(p->op))
	    {
	      if (p->reg1 == reg) p->reg1 = reg_fp1;
	      if (p->reg2 == reg) p->reg2 = reg_fp1;
	    }
	  }

	  mask >>= 1;
	  reg--;
	  if (mask != 0)
	    while ((mask & 01) != 1)
	    {
	  	  mask >>= 1;
	  	  reg--;
	    }
	}

	if (mask && !used0)
	{
	  new_mask -= 1 << (7 - (reg - reg_fp0));

	  for (p=first.forw; p!=NULL; p = p->forw) 
	  {
	    if (isFOP(p->op))
	    {
	      if (p->reg1 == reg) p->reg1 = reg_fp0;
	      if (p->reg2 == reg) p->reg2 = reg_fp0;
	    }
	  }
	}

	/* now fix movem for save and restore */
	for (p=first.forw; p!=NULL; p = p->forw) 
	{
	  if (p->op == FMOVM)
	  {
	    if (new_mask == 0) 
	      p = release(p);
	    else if (p->mode1==IMMEDIATE && p->type1==INTVAL)
	      p->addr1 = new_mask;
	    else if (p->mode2==IMMEDIATE && p->type2==INTVAL)
	      p->addr2 = new_mask;
	  }
	}


}
#endif M68020

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#ifdef DRAGON
/*
 * Move things to their ultimate dragon destination.
 *
 * For example:
 *
 *	fpmov.d	%fpa4,%fpa0	->	
 *	fpadd.d	%fpa7,%fpa0	->	fpadd.d	%fpa7,%fpa4
 *	fpadd.d	%fpa6,%fpa0	->	fpadd.d	%fpa6,%fpa4
 *	fpsub.d	%fpa5,%fpa0	->	fpsub.d	%fpa5,%fpa4
 *	fpmul.d	%fpa3,%fpa0	->	fpmul.d	%fpa3,%fpa4
 *	fpmov.d	%fpa0,%fpa4	->	fpmov.d	%fpa4,%fpa0
 *
 * 
 */
ultimate_dragon_destination()
{
	register node *p, *p2, *ptr;
	register int temp_reg, dest_reg, op;

	for (p=first.forw; p!=NULL; p = p->forw) {

	        if (p->op == UNLK)
		{
		  if (tail_dragon)
		    tail_dragon_optimization(p);
		  continue;
		}
		/* must be fpmov */
		if (p->op!=FPMOV)
			continue;

		/* must move from a scratch register */
		if (p->mode2 != FPREG)
			continue;
		if (p->mode1 != FPREG)
			continue;
		if (p->reg1 != reg_fpa0) 
			continue;

		/* this is our temporary register */
		temp_reg = p->reg1;
		dest_reg = p->reg2;
		
		/* a sequence of instructions on temp_reg is before this */
		for (p2=p->back; p2!=NULL ;p2=p2->back) {
			if (p2->reg2 != temp_reg && p2->reg2!=reg_fpa1)
				break;
		}
		if (p2==NULL)
			continue;

		p2 = p2->forw;

		/* now we must have a move of dest_reg to temp_reg */
		if (p2->op!=FPMOV)
			continue;
		if (p2->reg2!=temp_reg)
			continue;
		if (p2->mode1==FPREG && p2->reg1!=dest_reg)
		{
		        ptr = p2->forw;
		        if (ptr!= NULL && ptr->op==FPADD 
			    && ptr->reg1==dest_reg)
			{
			  /* swap p2 and ptr */
			  ptr->reg1 = p2->reg1;
			  p2->reg1  = dest_reg;
			}
		}

		/* none of the instructions must refer to dest_reg */
		for (ptr=p2->forw; ptr!=p; ptr=ptr->forw)
		{
			if (reg_in_arg(&ptr->op1, dest_reg))
				goto next_try;
			if (reg_in_arg(&ptr->op2, dest_reg))
				goto next_try;
		}


		/* fix first instrction */
		if(p2->reg2==temp_reg)
			  p2->reg2=dest_reg;	/* right to dest */
		p2 = p2->forw;
		/* ok, it's a sequence.  Change destination registers */
		/* this includes initial move, also! */
		for (; p2!=p; p2=p2->forw) 
		{
		        if (p2->reg1==temp_reg) 
			  p2->reg1 = dest_reg;
			if(p2->reg2==temp_reg)
			  p2->reg2=dest_reg;	/* right to dest */
		}

		/* change final move to move dest_reg,temp_reg */
		/* Which may get removed later */
		p->reg1 = dest_reg;
		p->reg2 = temp_reg;

		/* something has happened */
		opt("move to dragon ultimate destination");

	next_try:;
	}
}

/* #ifdef b1d ??? - or is this more general */
/* orginally writing code with dp3 in mind (in b1d) - generalize later */
tail_dragon_optimization(nodep)
node *nodep;
{

  register node *p;
  register node *q;
  register node *ptr;
  register int  dest_reg;

  p = nodep->back;
  /* there must be a better way to figure this out - ?? */
  while (p && p->op != FPMOV) 
    p = p->back;

  if (p==NULL) return;

  if (p->mode1 != ADISP || p->reg1 != reg_a6 || p->mode2 != FPREG)
    return;

  /* skip over fpmov's to restore fpregs */
  /* should we try to tag these fpmov's ?? */
  while (p->op == FPMOV && p->mode2==FPREG && p->mode1==ADISP
	 && p->reg1==reg_a6 && p->addr1 < 0) 
    p = p->back;

  for (; p != NULL; p = p->back)
  {
    if (isFPOP(p->op)) break;
    /* what about flt pt branches */
    if (p->op == LABEL || p->op==CBR || p->op==JMP) break;
  }
  if (p->op != FPMOV)
    return;

  if (p->reg1 != reg_fpa0)
    return;

  for (q = p->back; q != NULL; q = q->back)
  {
    if (isFPOP(q->op))
    {
      if (q->reg2 != reg_fpa0 && q->reg2 != reg_fpa1) break;
      else continue;
    }
    if (q->op == MOVE) 
	continue;
    else
	break;
  }

  q = q->forw;

  if (q->op != FPMOV || q->reg2 != reg_fpa0)
    return;

  if (q->mode1 != FPREG)
    return;

  if (p==q) return;

  dest_reg = q->reg1;

  for (ptr=q->forw; ptr != p; ptr = ptr->forw)
  {
    if (isFPOP(ptr->op) && ptr->reg1==dest_reg)
      return;
  }


  /* Hopefully now we are all set for the optimization */
  q = release(q);
  q = q->forw;
  
  for (; q != p; q = q->forw)
  {
    if (isFPOP(q->op) && q->reg2==reg_fpa0) q->reg2 = dest_reg;
  }

  p->reg1 = dest_reg;

  opt("tail dragon optmization");

}

/*
 * if a routine does not use fpa0, fpa1 or fpa2, but it does use other
 * dragon registers and it does not have any subroutine calls, then
 * we can use the scratch registers (fpa0/fpa1/fpa2) instead of other
 * dragon registers. This helps us eliminate reg save/restore via
 * fpmov instructions. This is a big help in the b1/b1d benchmarks
 *
 * can be made more efficient 
 * this routine is not called for fortran code with alternate entry points.
 */

fpmovm_reduce()
{
	register node *p;
	register int i;
	register int reg;
	node *link_p;
	node *save_p;
	node *unlk_p;
	node *restore_p;
	boolean changed;
	boolean saved[40];	/* array to remember which  regs 
				 * are saved and restored */
	boolean used[40];	/* array to remember which  regs 
				 * are used */

	restore_p = 0;
	changed = false;
	for (i = 0; i < 40; i++)
	{
	    saved[i] = false;
	    used[i] = false;
	}

	for (p=first.forw; p!=NULL; p = p->forw) 
	{

	  if (p->op == JSR) return;

	  if (p->op == LINK)
	  {

	        /* what if lea has been removed ? */
		while (p && p->op != LEA)
			p = p->forw;
		if (p == NULL) return;
		if (p->mode2!=ADIR || p->reg2 != reg_a2 || p->mode1!=ABS_L
		    || strcmp(p->string1, "fpa_loc"))
			return;

		p = p->forw;
	        link_p = p;
		reg = reg_fpa2;
		if (p->op == FPMOV && p->subop==DOUBLE
		    && p->mode2==ADISP && p->reg2==reg_a6
		    && p->mode1==FPREG && p->addr2 < 0)
		{
		    while(p->op == FPMOV && p->subop==DOUBLE
		    	&& p->mode2==ADISP && p->reg2==reg_a6
		    	&& p->mode1==FPREG && p->reg1 > reg && p->addr2 < 0)
		    {
			reg = p->reg1;
			saved[reg] = true;	
			p = p->forw;
		    }
		    /* don't skip over instruction following move's! */
		    if (p != link_p)
			p = p->back;
		}
		else
			/* no regs being saved - why bother */
			return;

		save_p = p;

	  }

	  if (p->op == UNLK)
	  {
	        unlk_p = p;
		p = p->back;
		while (p->op != FPMOV)
			p = p->back;

		if (dragon_and_881) 
		  reg = reg_fpa7;
		else
		  reg = reg_fpa15;

		if (p->op == FPMOV && p->subop==DOUBLE
		    && p->mode1==ADISP && p->reg1==reg_a6
		    && p->mode2==FPREG && p->addr1 < 0)

		    while(p->op == FPMOV && p->subop==DOUBLE
		    	&& p->mode1==ADISP && p->reg1==reg_a6
		    	&& p->mode2==FPREG && p->reg2 == reg && p->addr1 < 0)
		    {
			reg = p->reg2;
			if (saved[reg] != true)
				return;
			p = p->back;
			reg--;
		    }
		else
			/* no regs being saved - why bother */
			return;

		restore_p = p->forw;
		p = unlk_p ;

	  }


	  used[p->reg1] = true;
	  used[p->reg2] = true;

	}

	/* if all scratch regs are used, give up */
	if (used[reg_fpa0] && used[reg_fpa1] && used[reg_fpa2])
		return;

	/* if scratch is not used, find permanent fpareg to replace it with */
	for (reg=reg_fpa0; reg <=reg_fpa2; reg++)
	{
		if (used[reg])
		   continue;
		for (i= reg_fpa15; i > reg_fpa2; i--)
		{
		    if (saved[i]) break;
		}
	   	if (i > reg_fpa2)
	   	{
		   /* now replace all occurences of reg i by reg reg */
	  	   for (p=first.forw; p!=NULL; p = p->forw) 
		   {
		    if (p->reg1 == i) p->reg1 = reg;
	            if (p->reg2 == i) p->reg2 = reg;
		   }
		   saved[i] = false;
		   used[reg] = true;
		   changed = true;
		}
	}

	if (!changed) return;

	/* now fix fpmovem for save and restore */
	p = link_p;
	while(p->op == FPMOV && p->subop==DOUBLE
		&& p->mode2==ADISP && p->reg2==reg_a6
	  	&& p->mode1==FPREG && p->addr2 < 0)
	{
		if (!saved[p->reg1])
			p = release(p);
		p = p->forw;
		if (p == save_p) break;
	}


	if (!restore_p) return;
	p = restore_p;

	while(p->op == FPMOV && p->subop==DOUBLE
		&& p->mode1==ADISP && p->reg1==reg_a6
	  	&& p->mode2==FPREG && p->addr1 < 0)
	{
		if (!saved[p->reg2])
			p = release(p);
		p = p->forw;
	}

}

#endif DRAGON

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
 * if a routine does not use d0, d1, a0 or a1 - but it does use other
 * a or d registers and it does not have any subroutine calls, then
 * we can use the scratch registers instead of the permanent ones.
 * This helps us eliminate reg save/restore via movm instructions.
 *
 * can be made more efficient 
 * this routine is not called for fortran code with alternate entry points.
 */
movm_reduce()
{
	register node *p;
	register int i;
	register int reg;
	register node *link_p;
	register node *unlk_p;
	boolean changed;
	boolean saved[40];	/* array to remember which  regs 
				 * are saved and restored */
	boolean used[40];	/* array to remember which  regs 
				 * are used */

	unlk_p = 0;
	changed = false;
	for (i = 0; i < 16; i++)
	{
	    saved[i] = false;
	    used[i] = false;
	}

	for (p=first.forw; p!=NULL; p = p->forw) 
	{

	  if (p->op == JSR) return;

	  if (p->op == LINK)
	  {

	        link_p = p;
		p = p->forw;
		if (p->op == MOVE && p->subop==LONG
		    && p->mode2==DEC && p->reg2==reg_sp
		    && (p->mode1==DDIR || p->mode1==ADIR))
		{
		    while (p->op == MOVE && p->subop==LONG
		       && p->mode2==DEC && p->reg2==reg_sp
		       && (p->mode1==DDIR || p->mode1==ADIR))
		    {
			saved[p->reg1] = true;	
			p =p->forw;
		    }
		    /* don't skip over instruction following move's! */
		    if (p != link_p->forw)
			p = p->back;
		}
		else
			/* no regs being saved - why bother */
			return;

	  }

	  if (p->op == UNLK)
	  {
	        unlk_p = p;
		p = p->back;
		if (p->op == MOVE && p->subop==LONG
		    && p->mode1==INC && p->reg1==reg_sp
		    && (p->mode2==DDIR || p->mode2==ADIR))

		    while (p->op == MOVE && p->subop==LONG
		       && p->mode1==INC && p->reg1==reg_sp
		       && (p->mode2==DDIR || p->mode2==ADIR))
		    {
			if (saved[p->reg2] != true)
				return;
			p =p->back;
		    }
		else
			/* no regs being saved - why bother */
			return;

		p = unlk_p ;

	  }


	  used[p->reg1] = true;
	  used[p->reg2] = true;
	  used[p->index1] = true;
	  used[p->index2] = true;

	}

	/* if all scratch regs are used, give up */
	if (used[reg_d1] && used[reg_d0] && used[reg_a0] && used[reg_a1])
		return;

	/* if d1 is not used, find permanent dreg to replace it with */
	if (!used[reg_d1])
	{
	   for (i = reg_d7; i >=reg_d2; i--)
	   {
		if (saved[i]) break;
	   }
	   if (i >= reg_d2)
	   {
		/* now replace all occurences of reg i by reg d1 */
		/* need index reg for dn:dm */
	  	for (p=first.forw; p!=NULL; p = p->forw) 
		{
		    if (p->reg1 == i) p->reg1 = reg_d1;
	            if (p->reg2 == i) p->reg2 = reg_d1;
		    if (p->index1 == i) p->index1 = reg_d1;
	            if (p->index2 == i) p->index2 = reg_d1;
		}
		saved[i] = false;
		used[reg_d1] = true;
		changed = true;
	   }

	}

	/* if d0 is not used, find permanent dreg to replace it with */
	/* how likely is this to ever happen */
	if (!used[reg_d0])
	{
	   for (i = reg_d7; i >=reg_d2; i--)
	   {
		if (saved[i]) break;
	   }
	   if (i >= reg_d2)
	   {
		/* now replace all occurences of reg i by reg d0 */
		/* need index reg for dn:dm */
	  	for (p=first.forw; p!=NULL; p = p->forw) 
		{
		    if (p->reg1 == i) p->reg1 = reg_d0;
	            if (p->reg2 == i) p->reg2 = reg_d0;
		    if (p->index1 == i) p->index1 = reg_d0;
	            if (p->index2 == i) p->index2 = reg_d0;
		}
		saved[i] = false;
		used[reg_d0] = true;
		changed = true;
	   }

	}

	/* if a1 is not used, find permanent areg to replace it with */
	if (!used[reg_a1])
	{
	   /* worry about fpa code with lea to a2 later on */
	   for (i = reg_a5; i >=reg_a3; i--)
	   {
		if (saved[i]) break;
	   }
	   if (i >= reg_a3)
	   {
		/* now replace all occurences of reg i by reg a1 */
	  	for (p=first.forw; p!=NULL; p = p->forw) 
		{
		    if (p->reg1 == i) p->reg1 = reg_a1;
	            if (p->reg2 == i) p->reg2 = reg_a1;
		    if (p->index1 == i) p->index1 = reg_a1;
	            if (p->index2 == i) p->index2 = reg_a1;
		}
		saved[i] = false;
		used[reg_a1] = true;
		changed = true;
	   }

	}

	/* if a0 is not used, find permanent areg to replace it with */
	if (!used[reg_a0])
	{
	   /* worry about fpa code with lea to a2 later on */
	   for (i = reg_a5; i >=reg_a3; i--)
	   {
		if (saved[i]) break;
	   }
	   if (i >= reg_a3)
	   {
		/* now replace all occurences of reg i by reg a0 */
	  	for (p=first.forw; p!=NULL; p = p->forw) 
		{
		    if (p->reg1 == i) p->reg1 = reg_a0;
	            if (p->reg2 == i) p->reg2 = reg_a0;
		    if (p->index1 == i) p->index1 = reg_a0;
	            if (p->index2 == i) p->index2 = reg_a0;
		}
		saved[i] = false;
		used[reg_a0] = true;
		changed = true;
	   }

	}

	if (!changed) goto nextopt;

	/* now fix movem for save and restore */
	p = link_p->forw;
	while (p->op == MOVE && p->subop==LONG
	       && p->mode2==DEC && p->reg2==reg_sp
	       && (p->mode1==DDIR || p->mode1==ADIR))
	{
		if (!saved[p->reg1])
			p = release(p);
		p =p->forw;
	}

	if (unlk_p)
	{
	  p = unlk_p->back;
	  while (p->op == MOVE && p->subop==LONG
	       	 && p->mode1==INC && p->reg1==reg_sp
	       	 && (p->mode2==DDIR || p->mode2==ADIR))
		 {
			if (!saved[p->reg2] )
			  p = release(p);
			else
			  p =p->back;
		 }
	}
	

	/* if all scratch regs are used, give up */
	if (used[reg_d1] && used[reg_d0] && used[reg_a0] && used[reg_a1])
		return;
	/*
	 * now let us look for places where we can save/restore
	 * permanent regs in scratch regs 
	 */
	changed = false;
nextopt:

	for (reg = reg_d0; reg <= reg_d1 ; reg++)
	{
		if (used[reg])
			continue;
	   	for (i = reg_a5; i >=reg_a2; i--)
	   	{
			if (saved[i])
			{
			   /* save/retore i in reg */
			   saved[i] = false;
			   used[i] = reg;
			   changed = true;
			   break;
			}
	   	}

	}

	for (reg = reg_a0; reg <= reg_a1 ; reg++)
	{
		if (used[reg])
			continue;
	   	for (i = reg_d7; i >=reg_d2; i--)
	   	{
			if (saved[i])
			{
			   /* save/retore i in reg */
			   saved[i] = false;
			   used[i] = reg;
			   changed = true;
			   break;
			}
	   	}

	}

	if (!changed) return;

	/* now fix movem for save and restore */
	p = link_p->forw;
	while (p->op == MOVE && p->subop==LONG
	       && p->mode2==DEC && p->reg2==reg_sp
	       && (p->mode1==DDIR || p->mode1==ADIR))
	{
		if (!saved[p->reg1])
		{
			p->mode2 = isD(used[p->reg1]) ? DDIR : ADIR ;
			p->reg2  = used[p->reg1];
		}
		p =p->forw;
	}

	if (!unlk_p) return;
	p = unlk_p->back;
	while (p->op == MOVE && p->subop==LONG
	       && p->mode1==INC && p->reg1==reg_sp
	       && (p->mode2==DDIR || p->mode2==ADIR))
	{
		if (!saved[p->reg2] )
		{
			p->mode1 = isD(used[p->reg2]) ? DDIR : ADIR ;
			p->reg1  = used[p->reg2];
		}
		p =p->back;
	}

}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef M68020
/*
 * Attempt to remove the link and unlk instructions if routine does
 * not use local space. (no -x(a6)).
 */

int remove_link_unlk()
{
	register node *p, *q;
	register node *link_p;
	register node *unlk_p;
	register boolean param;
	register int stack;
	unsigned int length;
	/* make this a flag later ?? */
	boolean remove_link_unlk_in_leaf_only = false;
	boolean jsrs = false;
	int saves = 0; /* number of regs saved by procedure */

	link_p = 0;
	unlk_p = 0;
	param = false;
	for (p=first.forw, length = 0; p!=NULL; p = p->forw, ++length) 
	{

	  if (p->op == MOVEM) return 0;
	  
	  if (p->op == LINK)
	  {
	    link_p = p;

	    q = p->forw;

	    while (q->op == MOVE && q->subop == LONG && 
		   (q->mode1 == DDIR || q->mode1 == ADIR) &&
		   q->mode2 == DEC && q->reg2 == reg_sp) {
		saves++;
		q = q->forw;
	    }
	    continue;
	  }

	  if (p->op == JMP)
	  {
	    /* jmp (an) put out for assigned go-to in fortran */
	    if (p->mode1 == IND)
	      return 0;
	  }

	  if (p->op == JSR)
	  {
	    /*
	     * in Fortran we have problems with variable expression formats
	     * since they are control flow within a routine using jsr's
	     * c2 cannot follow this control flow to fix x(a6) to y(sp)
	     * until we figure out a better way to handle this, play
	     * it safe !
	     */
	    if (fort)
	      return 0;
	    /* if this optimization is to be done for leaf routine only */
	    if (remove_link_unlk_in_leaf_only)
	      return 0;
	    jsrs = true;
	  }

	  if (p->op == DBF)
	  {
	    /*
	     * passing structs by value is tricky, so let's just give up
	     * if that is attempted
	     * tell-tale signs: a dbf preceded by a mov.l -(%a0),-(%sp)
	     */
	    if (p->back->mode2==DEC && p->back->reg2==reg_sp)
		return 0;
	  }

	  if (p->op == UNLK)
	  {
	    unlk_p = p;
	    continue;
	  }

	  if (p->mode1==ADISP && p->reg1==reg_a6 && p->type1==INTVAL)
	  {
	      if (p->addr1 < 0)
		return 0;
	      else
		param = true;
	  }
	  else if (p->reg1==reg_a6)
	    return 0;

	  if (p->mode2==ADISP && p->reg2==reg_a6 && p->type2==INTVAL)
	  {
	      if (p->addr2 < 0)
		return 0;
	      else
		param = true;
	  }
	  else if (p->reg2==reg_a6)
	    return 0;

	}

	/* completely arbitrary length limit */
	/* don't do it for big procedures because gain is small */
	/* this is always a risky optimization; why take chances? */
	if (length >= 60)
	  return 0;

	if (link_p == 0)
	  return 0;


	/* if parameters then make sure that we can do this
	 * easily - give up if problems. Assumptions about use
	 * of sp.
	 */
	if (param)
	{
	  for (p=first.forw; p!=NULL; p = p->forw) 
	  {
	    p->info = 0;
	  }
	  if (jsrs && !(saves & 1)) {
	  	replace_a6_by_sp(link_p, -4);
	  }
	  else {
	  	replace_a6_by_sp(link_p, 0);
	  }
	}

	/*
	 * no parameters or it is safe anyway 
	 * remove link and unlk if there are 
	 * no calls, else turn the link into
	 * a "add.l &-4,%sp" and the unlk into
	 * a "add.l &4,%sp" to preserve 8-byte
	 * alignment of the stack
	 */
	if (jsrs && !(saves & 1)) {
		link_p->op = ADD;
		link_p->subop = LONG;
		make_arg(&link_p->op1, IMMEDIATE, INTVAL, -4);
		make_arg(&link_p->op2, ADIR, reg_sp);
	}
	else {
		release(link_p);
	}
	/* there may be some cases where the unlk has been removed
         * due to an infinite loop before */
	if (unlk_p) {
		if (jsrs && !(saves & 1)) {
			unlk_p->op = ADD;
			unlk_p->subop = LONG;
			make_arg(&unlk_p->op1, IMMEDIATE, INTVAL, 4);
			make_arg(&unlk_p->op2, ADIR, reg_sp);
		}
		else {
			release(unlk_p);
		}
	}

	return 1;

}

replace_a6_by_sp(nodep,stack)
node *nodep;
register int stack;
{
	  register node *p;

	  for (p=nodep; p!=NULL; p = p->forw) 
	  {
	    if (p->info) return;

	    if (p->op == RTS)
	      return;

	    p->info = 1;

	    if (p->op == CBR 
#ifdef M68020
		|| (p->op >= FBEQ && p->op <= FBNLT)
		|| (p->op >= FPBEQ && p->op <= FPBNLT)
#endif M68020
	        || p->op == DBF)
		
	    {
		replace_a6_by_sp(p->ref, stack);
	    }
	    if (p->op == JMP)
	    {
	      /* normal jump */
	      if (p->mode1 == ABS_L && p->type1 == INTLAB)
		p = p->ref;
	      else
	      {
		/*
		 * we assume that only jmps that we can have 
		 * are of the form
		 *         jmp Lxx
		 *         jmp 2(pc,d0.l)  - switch tables, computed goto
		 *         jmp (a0) - assigned go to
		 */
		if (p->mode1 != PINDEX)
		  internal_error("remove_link_unlk: unusual jmp encountered");

		/* take care of switch statments */
		p = p->forw;
		if (p->op != LABEL)
		  internal_error("no label following switch");
		p = p->forw;
		if (p->op != LALIGN)
		  internal_error("no lalign following switch");
		p = p->forw;
		if (p->op != LABEL)
		  internal_error("no label following switch");
		p = p->forw;
		if (p->op != JSW)
		  internal_error("no jsw following switch");
		while (p->op == JSW)
		{
		  replace_a6_by_sp(p->ref,stack);
		  p = p->forw;
		}
		return;
	      }
	    }

	    if (p->op == LEA && p->reg2 == reg_sp)
	    {
	      if (p->mode1==ADISP && p->reg1 == reg_sp)
	      {
	    	    stack += p->addr1;
	    	    continue;
	      }
	      else
		internal_error("remove_link_unlk: incorrect use of sp 1");
	    }

	    if ((p->op == ADDQ || p->op == ADD) && p->reg2 == reg_sp)
	    {
	      if (p->mode1==IMMEDIATE && p->type1 == INTVAL)
	      {
	    	    stack += p->addr1;
	    	    continue;
	      }
	      else
		internal_error("remove_link_unlk: incorrect use of sp 2");
	    }

	    if ((p->op == SUBQ || p->op == SUB) && p->reg2 == reg_sp)
	    {
	      if (p->mode1==IMMEDIATE && p->type1 == INTVAL)
	      {
	    	    stack -= p->addr1;
	    	    continue;
	      }
	      else
		internal_error("remove_link_unlk: incorrect use of sp 2.5");
	    }

	    if (p->reg1==reg_sp)
	    {
	      if (p->mode1==DEC)
	      {
		/* what about .b - can we ever encounter it */
		if (p->subop == DOUBLE) stack -= 8;
		else if (p->subop == WORD) stack -= 2;
		else if (p->subop == UNSIZED) stack -= 2;
		else stack -= 4;
	      }
	      else if (p->mode1==INC)
	      {
		if (p->subop == DOUBLE) stack += 8;
		else if (p->subop == WORD) stack += 2;
		else if (p->subop == UNSIZED) stack += 2;
		else stack += 4;
	      }
	      else if (p->mode1==IND)
		continue;
	      else
	      internal_error("remove_link_unlk: incorrect use of sp 3");
	    }

	    /* replace paramter x(a6) by y(sp) */
	    if (p->mode1==ADISP && p->reg1==reg_a6 && p->type1==INTVAL
	        && p->addr1 > 0)
	    {
	            p->reg1 = reg_sp;
	      	    p->addr1 -= (4 + stack) ;
	    }

	    if (p->op == PEA)
	    {
	    	    stack -= 4;
	    	    continue;
	    }

	    if (p->reg2==reg_sp)
	    {
	      if (p->mode2==DEC)
	      {
		if (p->subop == DOUBLE) stack -= 8;
		else if (p->subop == WORD) stack -= 2;
		else if (p->subop == UNSIZED) stack -= 2;
		else stack -= 4;
	      }
	      else if (p->mode2==INC)
	      {
		if (p->subop == DOUBLE) stack += 8;
		else if (p->subop == WORD) stack += 2;
		else if (p->subop == UNSIZED) stack += 2;
		else stack += 4;
	      }
	      else if (p->mode2==IND)
		continue;
	      else
		internal_error("remove_link_unlk: incorrect use of sp 4");
	    }

	    /* replace paramter x(a6) by y(sp) */
	    if (p->mode2==ADISP && p->reg2==reg_a6 && p->type2==INTVAL
	        && p->addr2 > 0)
	    {
	            p->reg2 = reg_sp;
	      	    p->addr2 -= (4 + stack) ;
	    }

	  }

}


/*
 * Stack adjust followed by an unlk? 
 * cannot do it until after remove_link_unlk() 
 * since if unlk is removed then stack adjust is needed 
 */

stack_adjust_before_unlk()
{
	register node *p;
	register node *q;

	for (p=first.forw; p!=NULL; p = p->forw) 
	{
	  if ((p->op == ADDQ || p->op == ADD)
	      && p->mode2==ADIR && p->reg2==reg_sp
	      && p->mode1==IMMEDIATE)
	  {
	    q = nonlab(p->forw);
	    if (q!=NULL && q->op==UNLK)
		p=release(p);
		opt("Stack adjustment followed by unlk");
	  }
	}
}
#endif M68020
