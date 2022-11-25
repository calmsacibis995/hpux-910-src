/* @(#) $Revision: 70.1 $ */   
#include "o68.h"

node *release();
node *insert();
node *resolve_movem();
node *resolve_fmovm();
node *postinctify();
node *predecrify();
char *copy();
char *convert_reg();
boolean bumpable();

/*
 * Convert the program to canonical form.
 *
 * This includes:
 *
 *	o Removing NOP's
 *
 *	o Resolve as many set's as possible.
 *
 *	o Change movem's to moves. (for all of them?)
 *
 *	o Changing lea's & pea's to moves when possible.
 *	   (unfortunately, pea 4(a6) has no analogue with move)
 *
 *	o Peel predecrement and postincrement off to seperate adds/subs.
 *
 *	o Convert many subtracts to adds.
 *
 * Some conversion to canonical form has already been accomplished.
 * Upon input, moveq was converted to move,
 * adda & addq were converted to add, etc.
 */
canonize()
{
	register node *p, *oldp;
	register int subop;

	remove_sets();


	for (p = first.forw; p!=NULL; p = p->forw) {	/* for whole list */

restart:
		if (p->op==MOVEM) {
			register int mask;
			register argument *arg;

			for (arg = &p->op1; arg <= &p->op2; arg++) {

				/*
				 * Convert a single register to a mask
				 */
				if (arg->mode==ADIR || arg->mode==DDIR)
					make_arg(arg, IMMEDIATE,
						 INTVAL, 1<<(arg->reg));

				if (arg->mode==ABS_L && arg->type==STRING) {
					mask = parse_movem_arg(arg->string);
					if (mask!=-1)
						make_arg(arg, IMMEDIATE,
							 INTVAL, mask);
				}
			}
		}


			
		/* peel off predecrement & postincrement to adds/subs */
		peel_off_incdec(p);

		subop = p->subop;
		switch (p->op) {

#ifndef M68020
		case DLABEL:
			if ((oldp = p->forw) != NULL 
			   && oldp->op ==MOVE
			   && oldp->subop == LONG
			   && oldp->mode1 == ADIR
			   && oldp->mode2 == DEC
			   && oldp->reg1 == reg_a6
			   && oldp->reg2 == reg_sp
			   && (oldp = oldp->forw) != NULL
			   && oldp->op == MOVE
			   && oldp->subop == LONG
			   && oldp->mode1 == ADIR
			   && oldp->mode2 == ADIR
			   && oldp->reg1 == reg_sp
			   && oldp->reg2 == reg_a6
			   && (oldp = oldp->forw) != NULL
			   && oldp->op == ADD
			   && oldp->subop == LONG
			   && oldp->mode1 == IMMEDIATE
			   && oldp->type1 == INTVAL
			   && SIXTEEN_BITTABLE (oldp->addr1)
			   && oldp->mode2 == ADIR
			   && oldp->reg2 == reg_sp )
			{
				/* make it link */
				cleanse(&oldp->op2);
				copyarg(&oldp->op1,&oldp->op2);
				oldp->op = LINK;
				oldp->subop = WORD;
				cleanse(&oldp->op1);
				oldp->mode1 = ADIR;
				oldp-> reg1 = reg_a6;
				release(p->forw);
				release(p->forw);
				opt("replaced prolog on lcd by link.w");
			}
			break;
#endif
		case MOVEM:
			p=resolve_movem(p);
			break;


#ifdef M68020
		case FMOVM:
			p=resolve_fmovm(p);
			break;
#endif M68020


		case MOVE:
			if (subop==UNSIZED)		/* if no length given */
				p->subop = WORD;	/* length is word */
			break;


		/* convert clr <op> into move #0,<op> */
		case CLR:
			if (subop==UNSIZED)		/* if no length given */
				p->subop = WORD;	/* length is word */

			p->op = MOVE;
			copyarg(&p->op1, &p->op2);	/* move argument */
			make_arg(&p->op1, IMMEDIATE, INTVAL, 0);
			break;


		case LEA:
			break;



		/* convert pea <op> into move <op>,-(sp) */
		case PEA:
			if (p->mode1==ABS_W && p->type1==INTVAL)
				p->mode1=ABS_L;

			p->subop = (p->mode1==ABS_W) ? WORD : LONG;
				
			if (p->mode1 == ABS_L) {	/* if pea 3 */
				p->op = MOVE;		/* now move #3,-(sp) */
				p->mode1 = IMMEDIATE;
				p->mode2 = DEC;
				p->reg2 = reg_sp;
				goto restart;
			}

			else if (p->mode1 == IND) {	/* if indirect mode */
				p->op = MOVE;
				p->mode1 = ADIR;	/* a0, not (a0) */
				p->mode2 = DEC;		/* predecrement mode */
				p->reg2 = reg_sp;
				goto restart;
			}
			break;


		case SUB:
			if (p->mode1==IMMEDIATE && p->type1==INTVAL) {
				p->op=ADD;
				p->addr1 = -p->addr1;
			}
			/* fall into ADD */

		case ADD:
			if (subop==UNSIZED)		/* if no length given */
				p->subop = WORD;	/* length is word */

			/* convert add.w #n,an to add.l #n,an */
			if (subop==WORD && p->mode1==IMMEDIATE 
			    && p->mode2==ADIR)
				p->subop = LONG;
			break;
		}
	}
}




/*
 * Convert from canonical form to external efficient form.
 *
 * This includes:
 *
 *	o Changing moves to movem's when possible.
 *
 *	o Changing moves to lea's & pea's when possible.
 *
 *	o Changing adds/subs to postincrement and predecrement mode.
 *
 *	o Changing adds/subs to leas's on address registers.
 *
 *	o Changing moves to the stack to pea's.
 *
 *	o Changing adds of negative constants to subtracts.
 */

#define word0(x) ((x) & 0xFFFF)
#define word1(x) (((unsigned) (x)) >> 16)

#define byte0(x) ((x) & 0xFF)
#define byte1(x) byte0((x)>>8)
#define byte2(x) byte0((x)>>16)
#define byte3(x) ((unsigned) (x)) >> 24)

decanonize()
{
	register int number, subop;
	register node *p, *oldp;

	for (p = first.forw; p!=NULL; p = p->forw) {	/* for whole list */

restart:

		if (oforty) {
			expand_bigoffset(p);
		}

		subop = p->subop;
		switch (p->op) {

		case MOVE:
			/*
			 * If moving zero into an address register,
			 * just subtract the register from itself.
			 */
			if (subop==LONG && p->mode1==IMMEDIATE
			&& p->type1==INTVAL && p->addr1==0
			&& p->mode2==ADIR) {
				p->op = SUB;
				p->mode1 = ADIR;
				p->reg1 = p->reg2;
				opt("move of zero to %s -> subtract itself",
					convert_reg(p->reg1));
				goto restart;
			}

			/*
			 * Convert move.l #3,a5 to move.w #3,a5
			 */
			if (p->mode2==ADIR
			&& subop==LONG
			&& p->mode1==IMMEDIATE
			&& p->type1==INTVAL
			&& SIXTEEN_BITTABLE(p->addr1)) {
				p->subop=WORD;
				break;
			}

			/*
			 * Convert mov.l &n,-(sp) to pea n
			 * Leave mov.l &0,-(sp) to clr.l -(sp)
			 */
			if (!oforty && 
			    subop==LONG && p->mode1==IMMEDIATE &&
			    !(p->type1==INTVAL && p->addr1==0) &&
			    p->mode2==DEC && p->reg2==reg_sp) {
				p->op = PEA;
				p->subop = UNSIZED;
				p->mode1 = ABS_L;
				cleanse(&p->op2);

				/* if pushing word constant, make it word */
				if (p->type1==INTVAL
				&& SIXTEEN_BITTABLE(p->addr1))
						p->mode1 = ABS_W;
				break;
			}

			if (p->mode1==IMMEDIATE
			 && p->type1==INTVAL) {	    /* #<num> seen */
				number = p->addr1;

				/* convert mov.l #n,dn to moveq #n,dn */
				if (p->mode2==DDIR && subop==LONG
				    && EIGHT_BITTABLE(number)) {
					p->op = MOVEQ;
					p->subop=UNSIZED;  /* inherently long */
					break;
				}

				/* convert mov #0,<dest> to clr <dest> */
				/* as long as <dest> ain't an address reg */
				/* that is illegal */
				if (number==0 && p->mode2!=ADIR) {
					p->op = CLR;
					copyarg(&p->op2, &p->op1);
					cleanse(&p->op2);
					break;
				}

			} /* end if number */
			break;

#ifdef M68020
			/*
			 * Convert link.l %a6,-28 to link.w %a6,-28
			 */
		case LINK:
			if (subop==LONG && p->type2==INTVAL
				&& SIXTEEN_BITTABLE(p->addr2))
				p->subop = WORD;
			break;
#endif

		case PEA:
			p->subop = UNSIZED;	/* size is in operand */
			break;

caseSUB:
		case SUB:
			/* 
			 * if this is for the 68040 then
			 * convert small sub to an addq
			 * if it will fit (in case the
			 * the destination is an A-reg)
			 * else settle for a subq  
			 */
			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			&& THREE_BITTABLE(p->addr1)) {
				if (oforty && THREE_BITTABLE(-p->addr1)) {
					p->op = ADDQ;
					p->addr1 = -p->addr1;
				}
				else {
					p->op=SUBQ;
				}
				break;
			}

			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			 && p->subop==LONG) {	    /* #<num> seen */
				number = p->addr1;

				/* convert long sub of areg to word */
				if (p->mode2==ADIR && SIXTEEN_BITTABLE(number))
					p->subop = WORD;

				/* 
				 * for the 68040, convert word sub of
				 * areg to add. for the 68030 turn
				 * it into an LEA
				 */
				if (p->mode2==ADIR && p->subop==WORD
				&& p->addr1!=-32768) {
					if (oforty) {
						p->op = ADD;
						p->addr1 = -p->addr1;
					}
					else {
 						p->op = LEA;
                                        	p->subop = UNSIZED;
                                        	p->mode1 = ADISP;
                                        	p->reg1 = p->reg2;
                                        	p->addr1 = -p->addr1;
                                        	/* offset already set up */
					}
				}
			}
			break;

		case ADD:
			/*
			 * Witness the pathetic case of:
			 *
			 *	jsr	alpha	  ->	jsr	alpha
			 *	add.l	#4,sp	  ->	add.l	#8,sp
			 *	move.l	d0,(sp)	  ->	move.l	d0,-(sp)
			 *	jsr	beta	  ->	jsr	beta
			 *
			 * Now we can't go beyond eight or we can't go to addq.
			 * However, if it's already at 12 (must use lea)
			 * we may as well add it in.
			 */

			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			&& (p->addr1==4 || p->addr1>=12)
			&& p->mode2==ADIR && p->reg2==reg_sp
			&& p->forw!=NULL
			&& p->forw->mode2==IND && p->forw->reg2==reg_sp) {
				p->addr1 += 4;
				p->forw->mode2=DEC;
			}

			/* see if we can make it into -(a0) or (a0)+ */
			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			&& p->mode2==ADIR) {
				oldp=p;
				if ((p->addr1 > 0) &&
				    (!oforty || (p->addr1 <= 32767)))
					p=postinctify(p);
				else if (!oforty || (p->addr1 >= -32768))
					p=predecrify(p);
				if (p!=oldp)
					goto restart;
			}

			/*
			 * add of negative number to 
			 * something other than an A-reg?
			 */
			if (p->op==ADD && (!oforty || p->mode2 != ADIR) && 
			    p->mode1==IMMEDIATE && p->type1==INTVAL && 
			    p->addr1<0) {
				p->op=SUB;
				p->addr1 = -p->addr1;
				goto caseSUB;
			}


			/* convert small add to addq */
			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			&& THREE_BITTABLE(p->addr1)) {
				p->op=ADDQ;
				break;
			}

			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			 && p->subop==LONG) {	    /* #<num> seen */
				number = p->addr1;

				/* convert long add of areg to word */
				if (p->mode2==ADIR && SIXTEEN_BITTABLE(number))
					p->subop = WORD;

				/* convert word add of areg to lea */
				if (!oforty && 
				    p->mode2==ADIR && p->subop==WORD) {
					p->op = LEA;
					p->subop = UNSIZED;
					p->mode1 = ADISP;
					p->reg1 = p->reg2;
					/* offset already set up */
				}
			}
			break;

		case AND:
			/*
			 * Change word size if upper bits are ones
			 * Change and.l #-2,d0 -> and.b #-2,d0
			 *
			 * Of course, the condition code must be unused.
			 */
			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			&& !cc_used(p->forw)
			&& (p->mode2==DDIR || bumpable(&p->op2, 4))) {
				number = p->addr1;

				/* Only changing word1? (MSW) */
				if (p->subop==LONG && word0(number)==0xFFFF
				&& p->mode2!=DDIR) {
					p->addr1 = number = word1(number);
					p->subop=WORD;
				}

				/* Only changing word0? (LSW) */
				if (p->subop==LONG && word1(number)==0xFFFF) {
					p->addr1 = number = word0(number);
					p->subop=WORD;
					if (p->mode2!=DDIR)
						bump(&p->op2, 2);
				}

				/* Only changing byte1? (MSB) */
				if (p->subop==WORD && byte0(number) == 0xFF
				&& p->mode2!=DDIR) {
					p->addr1 = number = byte1(number);
					p->subop=BYTE;
				}

				/* Only changing byte0? (LSB) */
				if (p->subop==WORD && byte1(number) == 0xFF) {
					p->addr1 = number = byte0(number);
					p->subop=BYTE;
					if (p->mode2!=DDIR)
						bump(&p->op2, 1);
				}

				/* Try to convert into BCLR */
				if (p->mode2==DDIR || p->subop==BYTE) {
					register int bit;

					if (p->subop==BYTE)
						number |= 0xFFFFFF00;
					if (p->subop==WORD)
						number |= 0xFFFF0000;
					bit = bitnum(~number);
					if (bit!=-1) {
						p->op = BCLR;
						p->subop = UNSIZED;
						p->addr1 = bit;
						break;
					}
				}
			}
			break;

		case OR:
		case EOR:
			/*
			 * Change word size if upper bits are zeros
			 * Change or.l #3,d0 -> or.b #3,d0
			 */
			if (p->mode1==IMMEDIATE && p->type1==INTVAL
			&& !cc_used(p->forw)
			&& (p->mode2==DDIR || bumpable(&p->op2, 4))) {
				number = p->addr1;

				/* Only changing word1? (MSW) */
				if (p->subop==LONG && word0(number)==0
				&& p->mode2!=DDIR) {
					p->addr1 = number = word1(number);
					p->subop=WORD;
				}

				/* Only changing word0? (LSW) */
				if (p->subop==LONG && word1(number)==0) {
					p->addr1 = number = word0(number);
					p->subop=WORD;
					if (p->mode2!=DDIR)
						bump(&p->op2, 2);
				}

				/* Only changing byte1? (MSB) */
				if (p->subop==WORD && byte0(number) == 0
				&& p->mode2!=DDIR) {
					p->addr1 = number = byte1(number);
					p->subop=BYTE;
				}

				/* Only changing byte0? (LSB) */
				if (p->subop==WORD && byte1(number) == 0) {
					p->addr1 = number = byte0(number);
					p->subop=BYTE;
					if (p->mode2!=DDIR)
						bump(&p->op2, 1);
				}

				/* Try to convert into BSET/BCHG */
				if (p->mode2==DDIR || p->subop==BYTE) {
					register int bit;
					bit = bitnum(number);
					if (bit!=-1) {
						p->op = p->op==OR ? BSET : BCHG;
						p->subop = UNSIZED;
						p->addr1 = bit;
						break;
					}
				}
			}
			break;


		case CMP:
			/*
			 * Convert long compares of aregs to word compares.
			 *
			 * cmp.l  %a5,&0  ->	cmp.w  %a5,&0
			 */

			if (p->mode1==ADIR
			&& subop==LONG
			&& p->mode2==IMMEDIATE
			&& p->type2==INTVAL
			&& SIXTEEN_BITTABLE(p->addr2)) {
				p->subop=WORD;
				break;
			}

		} /* end of switch p->op */

	} /* end for whole list */
}



/*
 * Seperate the increment/decrement from addressing into seperate add/subs.
 */
expand_bigoffset(p)
register node *p;
{
	register node *mov, *add;
	register int   reg, tmpreg;
	register long  disp;

	if ((p->op == MOVEM) || (p->op == FMOVM)) {
		return;
	}
	if ((p->mode1 == ADISP && 
	     p->type1 == INTVAL && 
	    !SIXTEEN_BITTABLE(p->addr1)) ||
	    (p->mode2 == ADISP && 
	     p->type2 == INTVAL && 
	    !SIXTEEN_BITTABLE(p->addr2))) {
		if (!reg_used(p,reg_a0)) {
			tmpreg = reg_a0;
	    	}
	    	else if (!reg_used(p,reg_a1)) {
			tmpreg = reg_a1;
	    	}
	    	else {
			return;
	    	}
		if (p->mode1 == ADISP && !SIXTEEN_BITTABLE(p->addr1)) {
			reg  = p->reg1;
			disp = p->addr1;

			p->mode1 = IND;
			p->reg1  = tmpreg;
	    	}
		else {
			reg  = p->reg2;
			disp = p->addr2;

			p->mode2 = IND;
			p->reg2  = tmpreg;
		}

		add = insert(p,ADD);
		add->subop = LONG;
		add->mode1 = IMMEDIATE;
		add->type1 = INTVAL;
		add->addr1 = disp;
		add->mode2 = ADIR;
		add->reg2  = tmpreg;

		mov = insert(add,MOVE);
		mov->subop = LONG;
		mov->mode1 = ADIR;
		mov->reg1  = reg;
		mov->mode2 = ADIR;
		mov->reg2  = tmpreg;
	}

}



/*
 * Seperate the increment/decrement from addressing into seperate add/subs.
 */
peel_off_incdec(p)
register node *p;
{
	register node *new;
	register argument *arg;
	register int isize, mode;

	/* if (p->op == MOVEM) return;
	 * oops! breaks assumptions in parse_movem and print_movem
	 * created major problems in 5.1 : review ! 
	 */
	for (arg = &p->op1; arg <= &p->op2; arg++) {
		mode = arg->mode;
		if (mode!=INC && mode!=DEC)
			continue;

		isize = size(p, arg);

		/* if predecrement, insert an add instruction */
		if (mode==DEC) {
			isize = -isize;			/* negative add */
			new=insert(p, ADD);		/* place before p */
		}

		/* postincrement, add an add instruction */
		else /* mode==INC */ {
			new=insert(p->forw, ADD);	/* place after p */
		}

		/* fill in the new instruction */
		new->subop=LONG;
		make_arg(&new->op1, IMMEDIATE, INTVAL, isize);
		make_arg(&new->op2, ADIR, arg->reg);

		arg->mode=IND;		/* now register indirect */
	}
}


/*
 * P points to an ADD of a positive number to an address register.
 *
 * Try to merge it with a preceding instruction into postincrement mode.
 */
node *
postinctify(pa)
register node *pa;
{
	register node *p;
	register argument *arg;
	register int op, mode, reg;

	reg = pa->reg2;

	for (p=pa->back; p!=NULL; p=p->back) {
		op=p->op;

		/* certain things destroy our search */
		if (op==LABEL || op==DLABEL ||  op==RTS
		|| op==JSR || op==JMP || op==CBR || op==MOVEM
		|| op==LEA || op==PEA 
#ifdef M68020
		|| (op >=BFCHG && op<=BFTST)
		|| (op >=FBEQ && op<=FBNLT)
#endif M68020
#ifdef DRAGON
		|| (op >=FPBEQ && op<=FPBNLT)
#endif DRAGON
		   )
			break;
		
		/* process arguments right to left */
		for (arg = &p->op2; arg >= &p->op1; arg--) {

			/* if we refer to our register, fail */
			mode=arg->mode;
#ifndef PREPOST
			if ((mode==ADIR || (mode >= INC && mode <=PINDBASE))
#else PREPOST
			if ((mode==ADIR || (mode >= INC && mode <=PCPOST))
#endif PREPOST
			&& arg->reg==reg)
				return(pa);

#ifndef PREPOST
			if ((mode>=AINDEX && mode<=PINDBASE) && arg->index==reg)
#else PREPOST
			if ((mode>=AINDEX && mode<=PCPOST) && arg->index==reg)
#endif PREPOST
				return(pa);

			/* if not register indirect, march on */
			if (mode!=IND)
				continue;

			/* must be our register */
			if (arg->reg!=reg)
				continue;

			/* make it postincrement */
			arg->mode=INC;

			/* reduce our add accordingly */
			pa->addr1 -= size(p, arg);

			/* delete useless add instruction */
			if (pa->addr1==0)
				pa=release(pa);

			return(pa);
		}
	}

	/* nothing good found, just return */
	return(pa);
}



/*
 * P points to an ADD of a negative number to an address register.
 *
 * Try to merge it with a following instruction into predecrement mode.
 */
node *
predecrify(pa)
register node *pa;
{
	register node *p;
	register argument *arg;
	register int op, mode, reg;

	reg = pa->reg2;

	for (p=pa->forw; p!=NULL; p=p->forw) {
		op=p->op;

		/* certain things destroy our search */
		if (op==LABEL || op==DLABEL ||  op==RTS
		|| op==JSR || op==JMP || op==CBR || op==MOVEM
		|| op==LEA || op==PEA 
#ifdef M68020
		|| (op >=BFCHG && op<=BFTST)
		|| (op >=FBEQ && op<=FBNLT)
#endif M68020
#ifdef DRAGON
		|| (op >=FPBEQ && op<=FPBNLT)
#endif DRAGON
		   )
			break;
		
		for (arg = &p->op1; arg <= &p->op2; arg++) {

			/* if we refer to our register, fail */
			mode=arg->mode;
#ifndef PREPOST
			if ((mode==ADIR || (mode >= INC && mode <=PINDBASE))
#else PREPOST
			if ((mode==ADIR || (mode >= INC && mode <=PCPOST))
#endif PREPOST
			&& arg->reg==reg)
				return(pa);

#ifndef PREPOST
			if ((mode>=AINDEX && mode<=PINDBASE) && arg->index==reg)
#else PREPOST
			if ((mode>=AINDEX && mode<=PCPOST) && arg->index==reg)
#endif PREPOST
				return(pa);

			/* if not register indirect, march on */
			if (mode!=IND)
				continue;

			/* must be our register */
			if (arg->reg!=reg)
				continue;

			/* make it predecrement */
			arg->mode=DEC;

			/* reduce our negative add accordingly */
			pa->addr1 += size(p, arg);

			/* delete useless add instruction */
			if (pa->addr1==0)
				pa=release(pa);

			return(pa);
		}
	}

	/* nothing good found, just return */
	return(pa);
}


/*
 * Resolve sets if possible.
 */
remove_sets()
{
	char *strchr();
#define BUFSIZE 100
	node *buf[BUFSIZE+2];		/* hold the set's & NULL */
	register node *p, **bufptr;

	/* first, find all the set's */
	bufptr = &buf[0];
	for (p = first.forw; p!=NULL; p = p->forw) {
		if (p->op!=SET)
			continue;
		/* assume: all sets to be resolved are of the form
		 *          set Lxxx,num
		 */
		if (*(p->string1)!='L')
			continue;

		if (bufptr>=&buf[BUFSIZE])	/* buffer full? */
			break;			/* hold everything! */

		*bufptr++ = p;			/* add it to the buffer */
	}
	*bufptr = NULL;				/* terminate the list */

#ifdef DEBUG
	if (debug3) {
		printf("remove_sets: contents of buf:\n");
		for (bufptr=buf; *bufptr!=NULL; bufptr++) {
			printf("** \tset\t");
			print_arg(&(*bufptr)->op1);
			printf(",");
			print_arg(&(*bufptr)->op2);
			printf("\n");
		}
	}
#endif DEBUG

	/* now, resolve all the set's */
	for (p=first.forw; p!=NULL; p=p->forw) {
#ifdef M68020
		if (p->op==LINK || p->op==MOVEM || p->op==FMOVM)
#else
		if (p->op==MOVEM || p->op==ADD)
#endif M68020
		{
			if(p->type1==STRING)
				resolve_set(&p->op1, buf);
			if(p->type2==STRING)
				resolve_set(&p->op2, buf);
		}
	}

	/* remove set instructions */
	for (bufptr=buf; *bufptr!=NULL; bufptr++) release(*bufptr);

}


/*
 * Resolve a single argument of any set's in buf.
 * Buf is an array of pointers to nodes that are SET's.
 * Buf is terminated by a null entry.
 */
resolve_set(arg, buf)
register argument *arg;
register node *buf[];
{
	register int negative;
	register char *cp;

#ifdef DEBUG
	if (debug3) {
		printf("** resolve_set(");
		print_arg(arg);
		printf(",buf)\n");
	}
#endif DEBUG

		cp = arg->string;

		if (*cp=='-') {
			cp++;
			negative=true;
		} else {
			negative=false;
		}

		/* loop thru the buffer looking for this string */
		for (; *buf!=NULL; buf++) {
#ifdef DEBUG
			if (debug3) {
				printf("** checking against \tset\t");
				print_arg(&(*buf)->op1);
				printf(",");
				print_arg(&(*buf)->op2);
				printf("\n");
			}
#endif DEBUG

			if (equstr((*buf)->string1, cp)) {
				/* a match!  a match! */
#ifdef DEBUG
				bugout("3: resolve_set: freeing '%s' at addr %x", arg->string, arg->string);
#endif DEBUG
				free_string(arg->string);
				arg->string = NULL;
				arg->type = INTVAL;
				arg->addr = (*buf)->addr2;
				if (negative)
					arg->addr = - arg->addr;
				return;
			}
		}

}

/*
 * Resolve a movem to nothing or individual move's.
 */
node *
resolve_movem(p)
register node *p;
{
	register argument *from, *to;
	register unsigned mask;
	register int reg;
	register node *q;
	register boolean prolog;
	int   size;
	node *link = NULL;

	if (p->mode1==IMMEDIATE && p->type1==INTVAL) {
		prolog = true;
		from = &p->op1;
		to = &p->op2;
		if (p->back->op == LINK) {
		   link = p->back;
		}
	}
	else if (p->mode2==IMMEDIATE && p->type2==INTVAL) {
		prolog = false;
		from = &p->op2;
		to = &p->op1;
	}
	else
		/* what kind of movem is this?? */
		return(p);
	
	mask=from->addr;			/* mask of reg's to save */
	if (mask==0) {				/* if none to save */
		p=release(p);			/* nuke the sucker */
		return(p);
	}

	if (to->mode != IND || to->reg != reg_sp)
		return(p);

	/* find the bit! */
	/* assume that the field is of the form */
	/* a7 a6 a5 a4 a3 a2 a1 a0 d7 d6 d5 d4 d3 d2 d1 d0 */
	/* we'll flip it later if this is not the case */

	reg = reg_d0;
	size = 0;
	q = p;

	while (mask!=0)
	{
	   while ((mask&01)!=1) {
		mask >>= 1;		/* shift it right one */
		reg++;
	   }

	   if (prolog)
	   {
	        q = insert(q, MOVE);
	        q->subop = LONG;
	   	q->mode1 = isD(reg) ? DDIR : ADIR;
	   	q->reg1 = reg;
	   	q->mode2 = DEC;
	   	q->reg2 = reg_sp;
		size += 4;
	   }
	   else
	   {
	        q = insert(p, MOVE);
	        q->subop = LONG;
	   	q->mode1 = INC;
	   	q->reg1 = reg_sp;
	   	q->mode2 = isD(reg) ? DDIR : ADIR;
	   	q->reg2 = reg;
	   }

	   mask >>= 1;
	   reg++;
	}

	if (link && (link->type2 == INTVAL))
	{
	    link->addr2 += size;
	}

	p=release(p);			/* nuke the sucker */
	return(p);

}

#ifdef M68020
/*
 * Resolve a fmovm to nothing or individual fmov's.
 */
node *
resolve_fmovm(p)
register node *p;
{
	register argument *from, *to;
	register unsigned mask;
	register int reg;

	if (p->mode1==IMMEDIATE && p->type1==INTVAL) {
		from = &p->op1;
		to = &p->op2;
	}
		else if (p->mode2==IMMEDIATE && p->type2==INTVAL) {
		from = &p->op2;
		to = &p->op1;
	}
	else
		/* what kind of fmovm is this?? */
		return(p);

	mask=from->addr;			/* mask of reg's to save */
	if (mask==0) {				/* if none to save */
		p=release(p);			/* nuke the sucker */
		return(p);
	}

	/* unfold fmovm into a sequence of individual fmov's */

	return(p);
}
#endif M68020


boolean
bumpable(arg, count)
register argument *arg;
int count;
{
	register int mode;

	mode = arg->mode;
	if (mode==ABS_L)
		return true;

	if (arg->addr+count<16384
	&& arg->type != UNKNOWN
#ifndef PREPOST
	&& (mode>=ADISP && mode<=PINDBASE))
#else PREPOST
	&& (mode>=ADISP && mode<=PCPOST))
#endif PREPOST
		return true;
	
	return false;
}


bump(arg, count)
register argument *arg;
register int count;
{
	char buf[100];
	register char *s;
	register int len;
	char *p;
	char *strchr();
	int   offset;

	switch(arg->mode) {
	case ABS_L:
	case ADISP:
	case AINDEX:
	case AINDBASE:
	case PDISP:
	case PINDEX:
	case PINDBASE:
#ifdef PREPOST
	case MEMPRE:
	case MEMPOST:
	case PCPRE:
	case PCPOST:
#endif PREPOST
		switch (arg->type) {
		case INTVAL:
			arg->addr += count;
			break;
		case INTLAB:
			sprintf(buf, "L%d+0x%x", arg->labno, count);
			arg->type = STRING;
			arg->string = copy(buf);
			break;
		case STRING:
			s = arg->string;

   			p = strchr(s,'+');

   			if (p && (p[1] == '0') && 
			    ((p[2] == 'x') || (p[2] == 'X'))) {
      				sscanf(p + 3, "%x", &offset);
				p[0] = NULL;
   			}
   			else {
      				offset = 0;
   			}

			sprintf(buf, "%s+0x%x", s, count + offset);
			free_string(s);
			arg->string = copy(buf);
			break;
		default:
			internal_error("bump: arg->type=%d ?", arg->type);
		}
		break;
	default:
		internal_error("bump: arg->mode=%d ?", arg->mode);
	}
}
