/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

/*
 * This file is a crude attempt to do span dependent optimizations
 * in c2. Note: we do it for 680x0 branches and dragon, and not
 * for the 881. This is because on the 881 there are only 2 sizes
 * on fbcc - .w and .l - and there is not too much of a diff. whichever
 * we use.
 */

span_dep()
{
	register node *p;
	register int t;
	node *saved;

	t = 0;
	for (p=first.forw; p!=NULL; FORW(p))
	{
		p->refc = t;
		t += max_inst_size(p);
	}

	for (p=first.forw; p!=NULL; FORW(p))
	{
		if ( (p->op == CBR) || (p->op ==JMP && p->mode1==ABS_L)
#ifdef DRAGON
		    || (p->op >= FPBEQ && p->op <= FPBNLT)
#endif DRAGON
		   )
		{
			if (p->forw == p->ref) 
			{ 
				saved = p->back;
				release(p);
				p = saved;
				continue;
			}
			t = distance(p,p->ref);
			if ( in(-126, t, 129)) p->ref = (node *) BYTE;
#ifdef M68020
			else if ( in(-32766, t, 32769)) p->ref = (node *) WORD;
			else p->ref = (node *) LONG;
#else
			else p->ref = UNSIZED;
#endif
		}
	}

}

max_inst_size(p)
register node *p;
{
	register int size;
	register argument *arg;
	register short op;
#ifdef DRAGON
	boolean	double_move = false;
	int d;
#endif

	op = p->op;
	if (pseudop(p))
	{
		switch (op)
		{
			case DC:	if (p->subop == LONG) {
					   return 4;
					}
					else if (p->subop == WORD) {
					   return 2;
					}
					else {
					   return 1;
					}
			case JSW:	return 4;
			case LALIGN:	return 6;
			default:	return 0;
		}
	}

	size = 2;	/* in general all instructions are 2 bytes */

#ifdef M68020
	if (isFOP(op)) size +=2;	/* extra word for 881 inst */
#ifdef DRAGON
	else if (isFPOP(op)) 
	{
		size +=2;		/* extra word for dragon inst */
		d = dragon_size(p);
		if (d  < 0) { double_move = true; d = 0; }
		size += d;
	}
#endif DRAGON
	else if ( in(BFCHG, op, BFTST))
		size += 2;	/* extra word for bit fld. inst */
	else if (p->subop==LONG &&
	    (op==MULS||op==MULU||op==DIVS||op==DIVU||op==DIVUL||op==DIVSL))
		size += 2;	/* extra word for long mult. and div. */
#else
	/*
	 * for 68010 - take care of bra that may be replaced by
	 * a jump or a bcc that may be replaced by a bcc around a jmp
	 * for bra xx to jmp xx do nothing because we are already
	 * counting the size of bra xx to be 6 bytes. 
	 * for ble yy  -> 	bgt new
	 *                	jmp yy
	 *                new: 	______
	 * add 2 bytes since new sequence is going to be 8 bytes.
	 */
	if (op == CBR ) size += 2;
#endif

	for (arg = &p->op1; arg <= &p->op2;  arg++)
	{
		switch (arg->mode)
		{
#ifndef M68020
			case ADIR:
			case DDIR:
			case IND:
			case INC:
			case DEC:
			case FREG:
				break;	/* nothing extra needed */
#else
			case IND:
#ifdef DRAGON
				/* double moves to/from (%an) generate
				   (%an) and 4(%an)
				   the latter takes two more bytes */
				if (double_move)
					size += 1; /* will be doubled later */
			case FPREG:
#endif
			case ADIR:
			case DDIR:
			case INC:
			case DEC:
			case FREG:
				break;
#endif
			case ADISP:
			case PDISP:
#ifdef M68020
				if (arg->type == INTVAL && 
					SIXTEEN_BITTABLE(arg->addr))
				{
					size += 2;
					break;
				}
				size += 6;
#else
				size += 2;
#endif
				break;

			case AINDEX:
			case PINDEX:
			case AINDBASE:
			case PINDBASE:
#ifdef M68020
				if (arg->type == INTVAL && 
					SIXTEEN_BITTABLE(arg->addr))
				{
					size += 4;
					break;
				}
				size += 6;
#else
				size += 2;
#endif
				break;

#ifdef PREPOST
			case MEMPRE:
			case MEMPOST:
			case PCPRE:
			case PCPOST:
				size +=10;
				break;
#endif PREPOST

			case ABS_W:
				size += 2;
				break;

			case ABS_L:
				size += 4;
				break;

			case IMMEDIATE:
				if (op==MOVEQ || op==ADDQ || op==SUBQ)
					break;
				if (arg->type == INTVAL && 
					SIXTEEN_BITTABLE(arg->addr)&&
					p->subop != LONG)
				{
					size += 2;
					break;
				}
				/* assume: no flt. pt. immediates */
				size += 4;
				break;

			case DPAIR:
				size += 2;
				break;

		}
	}

#ifdef DRAGON
	if (double_move) size = size * 2;
#endif DRAGON

	return(size);
}

distance(p,q)
register node *p, *q;
{
	return(q->refc - p->refc);

}

#ifdef DRAGON
dragon_size(p)
register node *p;
{
	register int size;

	size = 0;
	switch(p->op)
	{
		case FPMOV:
			if (p->subop == DOUBLE && 
			   (p->mode1 != FPREG || p->mode2 != FPREG))
			{
				size = -1 ;
			}
			break;

		default:
			/* check for branches */
			if (p->op >= FPBEQ && p->op <= FPBNLT)
				size += 2;
	}

	return(size);

}
#endif DRAGON
