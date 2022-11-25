/* @(#) $Revision: 70.2 $ */   
/*
 * C object code improver
 */

#include "o68.h"
#ifndef DEBUG
#define cc_unknown() ccloc.mode = UNKNOWN
#endif  DEBUG

extern char *copy();
node *release();
node *insert();
node *insertl();
node *find_cache();
node *getnode();
node *nonlab();
argument *find_const();
char *convert_reg();

rmove()
{
	char *convert_reg();
	register node *p, *p2, *p3, *ptr;
	register argument *cp1, *cp2;
	register int i, r, option, m1,m2, t1,t2, r1,r2;

	for (p=first.forw; p!=NULL; p = p->forw) { /* for whole linked list */

#ifdef VOLATILE
	if (isvolatile(&p->op1) || isvolatile(&p->op2)) {
	   cc_unknown();
	   if (!isvolatile(&p->op2)) {
	      dest(&p->op2, p->subop);
           }
	   continue;
        }
#endif

#ifdef DEBUG
	print_cache();
#endif DEBUG

	/* Get some useful stuff handy */
	m1 = p->mode1;
	m2 = p->mode2;
	t1 = p->type1;
	t2 = p->type2;
	r1  = p->reg1;
	r2  = p->reg2;

	switch (p->op) {

	case MOVE:
		/*
		 * Are we moving a quick constant into a non-quick place?
		 * If so, find a data register,
		 * moveq the constant into the data reg,
		 * and move the register into the destination.
		 *
		 * (Note that this is independent of move size (.b/.w/.l)).
		 */
		/*
		 * Don't do this for a move into a data register
		 * (might as well just quick it into the register itself)
		 * or for a move of zero anywhere.
		 * We can replace the zero with a clear.
		 * Except of course, for address registers, which can't
		 * be cleared, but can be subtracted from themselves.
		 *
		 * Also, don't do this do this if we're moving to the stack,
		 * since we'll do a pea for that.
		 */
		if ( !(m1==IMMEDIATE && t1==INTVAL && p->addr1==0)  /* not 0 */
		&& (m2!=DDIR) )			/* not to data register */
			split(p);
		
		if (p->subop==BYTE || p->subop==WORD)
			goto badmov;


		/*
		 * Copying a data register to itself?
		 *
		 * If the condition code is used, we *could* transform
		 * it into a test, but why bother?
		 * The register-to-register move is just as fast.
		 */
		if (r1==r2 && m1==DDIR && m2==DDIR && !cc_used(p->forw)) {
			opt("copy %s to itself", convert_reg(r1));
			goto useless_move;
		}

		/* Copying an address register to itself? */
		if (r1==r2 && m1==ADIR && m2==ADIR) {
			opt("copy %s to itself", convert_reg(r1));
			goto useless_move;
		}

		
		/*
		 * Is anybody going to ever use it?
		 *
		 * Condition code doesn't matter for an address register.
		 */
		if (m2==DDIR || m2==ADIR)
		{
			if (!reg_used(p->forw, r2)
			&& (m2==ADIR || !cc_used(p->forw)))
			    {
				opt("data in %s unused", convert_reg(r2));
				goto useless_move;
			    }
		}

		/* Does it contain that data already? */

		if (m1==DDIR && m2==DDIR && p->back!=NULL
		    && p->back->op==MOVE && p->back->subop==LONG
		    && p->back->mode1==DDIR
		    && p->back->mode2==DDIR
		    && p->back->reg1==r2
		    && p->back->reg2==r1)
		{
			opt("mov dn,dm followed by mov dm,dn");
			goto useless_move;
		}

		if (m1==DDIR && m2==DDIR && !cc_used(p->forw)
		    && cache[r1].op
		    && equarg(&p->op2,&cache[r1].op2))
		{
			opt("data is in dreg already");
			goto useless_move;
		}

		if (m2==ADIR 
		    && cache[r2].op
		    && equarg(&p->op1,&cache[r2].op2))
		{
			opt("data is in areg already");
			goto useless_move;
		}


		cp1 = find_const(&p->op1);
		if (cp1!=NULL)
			cp2 = find_const(&p->op2);
		else
			cp2 = NULL;

		/* General case of equal pointers */
		if (cp1!=NULL && cp1==cp2 && !cc_used(p->forw)) {
			opt("data is there already");
			goto useless_move;
		}

		/* Do they contain equal integers? */
		if (cp1!=NULL && cp2!=NULL
		&& cp1->mode==IMMEDIATE && cp2->mode==IMMEDIATE
		&& cp1->type==INTVAL && cp2->type==INTVAL
		&& cp1->addr==cp2->addr && !cc_used(p->forw)) {
			opt("move'd integer data %d already there", cp1->addr);
			goto useless_move;
		}


		option = CONST_OK | AREG_OK;
		if (p->mode2!=ADIR)			/* if not move x,an */
			option |= ZERO_PREFERRED;	/* can use clr dest */
		if (p->mode2==DDIR)			/* if move x,dn */
			option |= QUICK_EIGHT_BIT;	/* try to use moveq */

		data_simplify(&p->op1, p->subop, option);

		/* this is because if we do not check for this now 
		   ultimate_dest may screw-us up. Should we keep this ? */
		if (p->reg1==r2 && p->mode1==DDIR && m2==DDIR && !cc_used(p->forw)) {
			opt("copy %s to itself", convert_reg(r2));
			goto useless_move;
		}

		address_simplify(&p->op2);

		if (p->mode1==ADIR && p->mode2==ADIR && p->reg1==p->reg2)
			break;

		dest(&p->op2, p->subop);

		save_cache(&p->op1, &p->op2, p->subop);

		if (p->mode2!=ADIR)
			setcc(&p->op2, p->subop);

		break;

/*
 * A useless move, delete the instruction;
 */
useless_move:
		p=release(p);
		redunm++;
		change_occured=true;
		break;


	case LEA:
		if (!reg_used(p->forw, r2))
		{
		    /* if fortran code uses dragon (-D) then do
		       not remove lea to a2 since it may be used
		       by variable format funcs */
		    /* lea is not removed since reg_used does not 
		       understand dragon instructions at present */
		    if (!(r2==reg_a2 && (dragon || dragon_and_881) && fort))
		    {
			opt("data in %s unused", convert_reg(r2));
			goto useless_move;
		    }
		}

		r = find_areg(&p->op1);
		if (r != -1)
		{
			p->op = MOVE;
			p->subop = LONG;
			cleanse(&p->op1);
			p->mode1 = ADIR;
			p->reg1 = r;
			opt("lea xx,an --> mov.l am,an");
			if (r != r2) dest(&p->op2, p->subop);
			break;
		}
		/*
		 * look for sequence like
		 * lea    -48(%a6),%a0   	lea   -48(%a6),%a5
		 * mov.l  %a0,%a5		mov.l %a5,%a0
		 */
		if (p->forw!=NULL && p->forw->op==MOVE
		    && p->forw->subop==LONG
		    && p->forw->mode1==ADIR &&p->forw->mode2==ADIR
		    && p->reg2==p->forw->reg1
		    && p->forw->reg2 > p->forw->reg1)
		{
			p->reg2 = p->forw->reg2;
			p->forw->reg2 = p->forw->reg1;
			p->forw->reg1 = p->reg2;
		}
		dest(&p->op2, p->subop);
		save_areg_cache(&p->op1, (p->reg2-reg_a0));
		break;

	case ADD:
	case SUB:
		/* If data not used, don't bother */
		if (m2==DDIR && !reg_used(p->forw, r2) && !cc_used(p->forw))
		{
			opt("add/sub instruction not needed on d%d", r2);
			p = release(p);
			break;
		}

		/* Adding zero? */
		if (m1==IMMEDIATE && t1==INTVAL && p->addr1==0) {
			if (m2==ADIR) {
				opt("Add/sub of 0 to %s", convert_reg(p->reg2));
				p=release(p);
				break;
			}

			/* Make it into a test */
			p->op = TST;
			copyarg(&p->op2, &p->op1);
			cleanse(&p->op2);
			opt("Add/sub of 0 converted into test");
			break;
		}

		option = CONST_OK|QUICK_THREE_BIT;
		if (m2==DDIR || m2==ADIR)	/* ADD/ADDA */
			option |= AREG_OK;
		
		/*
		 * do not attempt this if second arg is sp
		 * since it causes problems in remove_link_unlk() 
		 * if add -4,sp is replaced by add d6,sp
		 */
		if (m2!=ADIR || r2!=reg_sp)
			data_simplify(&p->op1, p->subop, option);

		/*
		 * Shall we try to optimize add #n,<ea> ?
		 *
		 * If the constant is small enough, leave it for addq.
		 *
		 * If <ea> is an address register, leave it to be
		 * converted into a lea later.
		 */
		if (p->mode2!=ADIR
		&& p->mode1==IMMEDIATE && p->type1==INTVAL
		&& (i=p->addr1, EIGHT_BITTABLE(i))
		&& !THREE_BITTABLE(-i)			/* allow for subq */
		&& !THREE_BITTABLE(i))
			split(p);

		address_simplify(&p->op2);
		dest(&p->op2, p->subop);

		if (p->mode2!=ADIR) {
			if (p->subop==LONG)
				setcc(&p->op2, p->subop);
			else
				cc_unknown();
		}
		break;


	case MOVEM:
		/* A movem followed by its reverse is of no use. */
		if (p->forw!=NULL && p->forw->op==MOVEM 
		    && p->subop==LONG && p->forw->subop==LONG
		    && equarg(&p->op1,&p->forw->op2)
		    && equarg(&p->op2,&p->forw->op1))
		{
			release(p->forw);	/* nuke the movem */
			p=release(p);		/* nuke the reverse movem */
			opt("movem followed by reverse movem obliterated!");
			continue;
		}

		address_simplify(&p->op1);
		address_simplify(&p->op2);
		if (m2==IMMEDIATE && t2==INTVAL)
		/* movem to regs will destroy their contents */
		{
			if (m1==DEC)
				dest_movem_arg(p->addr2,1);
			else 
				dest_movem_arg(p->addr2,0);
		}
		break;

	case AND:
	case OR:
	case EOR:
		/* Combine adjacent ands, ors, and exors. */
		while (p->forw!=NULL && p->forw->op==p->op
		&& p->forw->subop==p->subop
		&& m1==IMMEDIATE && t1==INTVAL
		&& p->forw->mode1==IMMEDIATE && p->forw->type1==INTVAL
		&& equarg(&p->op2, &p->forw->op2)) {
			switch (p->op) {
			case AND: p->addr1 &= p->forw->addr1; break;
			case OR:  p->addr1 |= p->forw->addr1; break;
			case EOR: p->addr1 ^= p->forw->addr1; break;
			}
			release(p->forw);
			opt("Adjacent logical operations combined");
		}

		/* Fall into code below: */
	case MULS:
	case MULU:
	case DIVS:
	case DIVU:

		/* If data not used, don't bother */
		if (m2==DDIR && !reg_used(p->forw, r2) && !cc_used(p->forw)) {
			opt("or/and/eor instruction not needed on d%d", r2);
			p = release(p);
			break;
		}

		/* Fall into code below: */

	case DIVSL:
	case DIVUL:
		split(p);		/* move constant op to data reg */
	badmov:
		data_simplify(&p->op1, p->subop, CONST_OK);
		address_simplify(&p->op2);
		dest(&p->op2, p->subop);
		if (p->subop==LONG)
			setcc(&p->op2, p->subop);
		else
			cc_unknown();
		break;

	case EXT:
	case EXTB:
		/*
		 * Don't bother to EXT an already proper constant
		 */
		ptr = &cache[r1];
		if (ptr->op && ptr->mode1==DDIR
		&& ptr->mode2==IMMEDIATE && ptr->type2==INTVAL) {
			i = ptr->addr2;

			if (p->subop==WORD && EIGHT_BITTABLE(i)
			||  p->op==EXT && p->subop==LONG && SIXTEEN_BITTABLE(i)
			||  p->op==EXTB && EIGHT_BITTABLE(i)) {

				if (!cc_used(p->forw)) {
					p=release(p);
					opt("ext/extb of d%d useless", r1);
					break;
				}
			}
		}

		p2 = p->forw;
		if (p2->op == MOVE && p2->mode1 == DDIR && 
		    p2->reg1 == r1 && p2->mode2 == DDIR && 
		    p->subop == p2->subop && !reg_used(p2->forw,r1))
		{
			p3 = p->back;
			if (p3->op == MOVE && 
			    p3->mode2 == DDIR && 
			    p3->reg2 == r1)
			{
				p3->reg2 = p2->reg2;
				p->reg1 = p2->reg2;
				reg_changed(r1);
				reg_changed(p2->reg2);
				release(p2);
				opt("ext/extb changed to reg %d", p2->reg2);
				break;
			}
		}
		goto monadic;



	/* Monadic operators */
monadic:
	case NOT:
	case NEG:
		/* If data not used, don't bother */
		if (m1==DDIR && !reg_used(p->forw, r1) && !cc_used(p->forw)) {
			opt("monadic instruction not needed on d%d", r1);
			p = release(p);
			break;
		}
		address_simplify(&p->op1);
		dest(&p->op1, p->subop);
		if (p->subop==LONG)
			setcc(&p->op1, p->subop);
		else
			cc_unknown();
		break;

	case LSL:
		/* If data not used, don't bother */
		if (m2==DDIR && !reg_used(p->forw, r2) && !cc_used(p->forw)) {
			opt("lsl/lsr instruction not needed on d%d", r2);
			p = release(p);
			break;
		}

		dest(&p->op2, p->subop);
		if (p->subop==LONG)
			setcc(&p->op2, p->subop);
		else
			cc_unknown();
		break;

	case SEQ:
	case SNE:
		dest(&p->op1, p->subop);
		break;

	case TST:
		/*
		 * Do we need this test at all?
		 *
		 * If it's not the magic tst.b after a LINK,
		 * and the condition code isn't used,
		 * we can wipe it out.
		 */
#ifdef SAVE_TST
		if (p->subop==BYTE && p->back!=NULL && p->back->op==LINK)
			goto normal_test;
#endif

		/*
		 * If this a float card test?
		 */
		if (p->subop==WORD && p->forw!=NULL && p->forw->op==MOVEM)
			goto normal_test;

		if (cc_used(p->forw))
			goto normal_test;

		opt("useless test");
		nrtst++;
		p = release(p);
		break;

normal_test:
		data_simplify(&p->op1, p->subop, 0);	/* constant not ok */

                if (incc(&p->op1) && p->mode1!=INC && p->mode1!=DEC) 
		{

			p2 = p->back;
			while (p2 && 
			       (((p2->op == ADD || p2->op == SUB || 
				p2->op == MOVE) && p2->mode2 == ADIR) ||
                    		p2->op == SUBA || p2->op == ADDA ||
                    		p2->op == PEA  || p2->op == LEA))
			{
				p2 = p2->back;
			}

                        if (p2 != NULL && (p->subop == p2->subop))
			{
                        	if (p2->op==MOVE || p2->op==AND ||
                        	    p2->op==OR   || p2->op==EOR ||
                        	    p2->op==LSL  || p2->op==LSR) 
				 {
					/*
					 * the inst that sets the cc does
					 * not set the overflow bit, so
					 * we can remove the tst
					 */
					p = release(p);
					nrtst++;
					opt("redundant test");
					break;
				}
				else if (p->forw != NULL && p->forw->op==CBR)
				{
					/*
					 * COND_GE and COND_LT can be mapped
					 * to COND_PL and COND_MI which don't
					 * check the overflow flag
					 */
					if (p->forw->subop == COND_GE) 
					{
						p->forw->subop = COND_PL;
					}
					else if (p->forw->subop == COND_LT)
					{
						p->forw->subop = COND_MI;
					}
					
					if (p->forw->subop == COND_EQ ||
					    p->forw->subop == COND_NE ||
					    p->forw->subop == COND_MI ||
					    p->forw->subop == COND_PL) 
					{
						p = release(p);
						nrtst++;
						opt("redundant test");
						break;
					}
				}
			}
		}

		if (p->subop==LONG)
			setcc(&p->op1, p->subop);
		else
			cc_unknown();
		break;


	case CMP:
		/*
		 * Do we need this compare at all?
		 *
		 * We know there's no autoincrement/decrement modes because
		 * we pulled those off in canonization.
		 * If the condition code isn't used,
		 * we can wipe it out.
		 */
		if (!cc_used(p->forw)) {
			opt("useless compare");
			nrtst++;
			p = release(p);
			break;
		}

		/*
		 * Can we convert this compare to one to a compare to zero?
		 * (which may get converted into a tst)
		 *
		 * For example:
		 *
		 *	cmp.l	%d7,&1		cmp.l	%d7,&0
		 *	jge	L13		jgt	L13
		 *
		 * We must make sure that the condition code is not used!
		 */
		if (m2==IMMEDIATE && t2==INTVAL
		&& p->forw!=NULL && p->forw->op==CBR
		&& !cc_used(p->forw->forw)	/* not used after jump */
		&& p->forw->ref!=NULL		/* jump goes somewhere */
		&& !cc_used(p->forw->ref)) {	/* not used thru jump */

			if (p->addr2==1 && p->forw->subop==COND_GE) {
				p->addr2=0;
				p->forw->subop=COND_GT;
				opt("compare x>=1 -> x>0");
			}

			else if (p->addr2==1 && p->forw->subop==COND_LT) {
				p->addr2=0;
				p->forw->subop=COND_LE;
				opt("compare x<1 -> x<=0");
			}

			else if (p->addr2==-1 && p->forw->subop==COND_GT) {
				p->addr2=0;
				p->forw->subop=COND_GE;
				opt("compare x>-1 -> x>=0");
			}

			else if (p->addr2==-1 && p->forw->subop==COND_LE) {
				p->addr2=0;
				p->forw->subop=COND_LT;
				opt("compare x<=-1 -> x<0");
			}

		}



		/*
		 * Can we convert this compare to zero into a test?
		 */
		if (m2==IMMEDIATE && t2==INTVAL && p->addr2==0
		&& m1!=ADIR) {
			p->op = TST;
			cleanse(&p->op2);
			setcc(&p->op1, p->subop);
			opt("compare to zero -> test");
			break;
		}


		/*
		 * If we're comparing an address register to zero
		 * (a common occurance in c programs where NULL==0)
		 * then try to find a spare data register
		 * and move the address reg into it to set the codition code.
		 */
		if (m2==IMMEDIATE && t2==INTVAL && p->addr2==0
		&& m1==ADIR
		&& (r=unused_dreg(p->forw))!=-1) {
			p->op = MOVE;
			p->mode2 = DDIR;
			p->reg2 = r;
			dest(&p->op2, p->subop);
			setcc(&p->op2, p->subop);
			opt("compare %s to zero -> move to d%d",
				convert_reg(p->reg1), r);
			break;
		}


		/*
		 * If we're comparing a data register to (1 <= x <= 8),
		 * and the register isn't later used, then change
		 * the compare into a subtract (quick).
		 *
		 * Since canonical form demands that all constant
		 * subtracts be adds, we make it an add and switch the sign.
		 *
		 * This conversion can not be done for (-8 <= x <= -1)
		 * because this will result in an add quick of -x which 
		 * does not set the condition codes (at least the carry
		 * bit) the same as the cmp instruction
		 */
                i=p->addr2;
                if (m2==IMMEDIATE && t2==INTVAL && THREE_BITTABLE(i)
                && p->mode1==DDIR && p->forw!=NULL && p->forw->op==CBR
                && p->forw->subop != COND_CC && p->forw->subop != COND_CS
                && p->forw->subop != COND_HI && p->forw->subop != COND_LS
                && !reg_used(p->forw, r1)
                && !cc_used(p->forw->forw)      /* not used after jump */
                && p->forw->ref!=NULL           /* jump goes somewhere */
                && !cc_used(p->forw->ref)) {    /* not used thru jump */
                        cleanse(&p->op1);
                        cleanse(&p->op2);
                        p->op=ADD;
                        p->mode1 = IMMEDIATE;
                        p->type1 = INTVAL;
                        p->addr1 = -i;
                        p->mode2 = DDIR;
                        p->reg2 = r1;
                        dest(&p->op2, p->subop);
                        setcc(&p->op2, p->subop);
                        opt("compare %%d%d,&%d -> subtract",
                                i, r1);
                        break;
                }


		/*
		 * Here are the types of compares:
		 *
		 * o  cmp   Dn,<ea>
		 *    (but cmp.b Dn,An is not allowed)
		 *
		 * o  cmpa  An,<ea>
		 *
		 * o  cmpi  <ea>,&n
		 *    (cmpi An,&n is not allowed,
		 *     but it is allowed as cmpa An,&n)
		 *
		 * o  cmpm  (Ay)+,(Ax)+
		 *    (let's hope that never occurs)
		 */

		/*
		 * Data simplify the first operand
		 */
		if (p->mode1==DDIR) {		/* cmp */
			option = CONST_OK;
			if (p->subop!=BYTE)
				option |= AREG_OK;
		}
		else if (p->mode1==ADIR) {	/* cmpa */
			option = CONST_OK | AREG_OK;
		}
		else				/* cmpi */
			option = CONST_OK | AREG_OK;
			/* If we get an areg it'll become cmpa */

		if (p->mode1==DDIR || p->mode1==ADIR)
			data_simplify(&p->op2, p->subop, option);

		/* Can't split cmp.l #4,(a3) without switching ops */
		if (p->mode1==ADIR || p->mode1==DDIR)
		     {
			swap_args(p);
			split(p);
			swap_args(p);
		     }
		else 
		     {
			register int new_cond;

			if (p->mode2==IMMEDIATE && p->type2==INTVAL
			&& p->forw!=NULL
			&& p->forw->op==CBR
			&& (new_cond=negate_condition(p->forw->subop))!=-1
			&& !cc_used(p->forw->forw)
			&& p->forw->ref!=NULL
			&& !cc_used(p->forw->ref)) {
				swap_args(p);
				split(p);
				swap_args(p);
				/* Now repair it */
				if (p->mode2==DDIR && p->mode1!=DDIR) {
					swap_args(p);
					p->forw->subop = new_cond;
				}
			}
		}

		cc_unknown();
		break;

	case CBR:
		/*
		 * If an unconditional jump to the same place follows, delete us
		 */
		if (p->forw!=NULL && p->forw->op==JMP
		&& p->mode1==ABS_L && p->forw->mode1==ABS_L
		&& p->type1==INTLAB && p->forw->type1==INTLAB
		&& p->labno1==p->forw->labno1) {
			decref(p->ref);
			p = release(p);
			opt("conditional branch to L%d before jmp L%d",
				p->labno1, p->labno1);
			break;
		}

		/*
		 * If the same type of conditional branch follows, delete it.
		 */
		if (p->forw!=NULL
		&& p->forw->op==CBR && p->forw->subop==p->subop) {
			decref(p->forw->ref);
			release(p->forw);
			opt("conditional branch to L%d before similar branch",
				p->labno1);
			break;
		}

		/*
		 * Will the conditional branch always/never succeed?
		 */
		if (ccloc.mode==IMMEDIATE && ccloc.type==INTVAL) {
			if (p->subop==COND_ZR && ccloc.addr==0
			||  p->subop==COND_NZ && ccloc.addr!=0) {
				/* make into unconditional jump */
				p->op = JMP;
				p->subop = UNSIZED;
				opt("conditional branch into jump");
				goto caseJMP;
			}
			else if (p->subop==COND_ZR && ccloc.addr!=0
			     ||  p->subop==COND_NZ && ccloc.addr==0) {
				/* the jump will never succeed */
				decref(p->ref);
				p = release(p);
				opt("conditional branch never taken");
				break;
			}
		}
		break;

caseJMP:
	case JMP:
		redunbr(p);
		status_unknown();
		break;

	case JSR:
		/*
		 * Everything is now unknown except the registers.
		 * And of those, d0,d1,a0,a1 can be trashed by the subroutine.
		 * Hence, only d2-7, a2-7 are safe.
		 * All 881 registers can be trashed
		 */
		cache[reg_d0].op = false;
		cache[reg_d1].op = false;
		cache[reg_a0].op = false;
		cache[reg_a1].op = false;

		aregs[0].mode = UNKNOWN;
		aregs[1].mode = UNKNOWN;

		reg_changed(reg_d0);
		reg_changed(reg_d1);
		reg_changed(reg_a0);
		reg_changed(reg_a1);

		/* Cleanse the registers that contain memory locations */
		for (ptr = &cache[reg_d2]; ptr<&cache[reg_a7]; ptr++) {
			if (ptr->mode2 == IMMEDIATE || ptr->mode2 == DDIR
			    || ptr->mode2 == ADIR)
			    continue;
			ptr->op = false;
		}

#ifdef SAVE_FP_REGS
		/* for 6.5 and beyond, we actually save float regs */
		/* so cleanse only those containing memory locations */
		reg_changed(reg_fp0);
		reg_changed(reg_fp0+1);
		reg_changed(reg_fpa0);
		reg_changed(reg_fpa0+1);
		for (ptr = &cache[reg_fp0+2]; ptr < &cache[reg_fp7]; ++ptr)
		{
			if (ptr->mode2 == IMMEDIATE
			    || ptr->mode2 == DDIR
			    || ptr->mode2 == ADIR)
				continue;
			ptr->op = false;
		}
		for (ptr = &cache[reg_fpa0+2]; ptr < &cache[reg_fpa15]; ++ptr)
		{
			if (ptr->mode2 == IMMEDIATE
			    || ptr->mode2 == DDIR
			    || ptr->mode2 == ADIR)
				continue;
			ptr->op = false;
		}
		for (ptr = &cache[reg_fpa15] + 1; ptr <= cache_end; ++ptr)
			ptr->op = false;
#else
		/* Cleanse the 881 registers and non-registers */
		for (ptr = &cache[16]; ptr<=cache_end; ptr++) {
			ptr->op = false;
		}
#endif

		cc_unknown();
		break;

	case LINK:
		/* A link followed by an unlink is of no use. */
		if (p->forw!=NULL && p->forw->op==UNLK && r1==p->forw->reg1) {
			release(p->forw);	/* nuke the unlink */
			p=release(p);		/* nuke the link */
			opt("link followed by unlink obliterated!");
			continue;
		}
		break;

#ifdef M68020
	case FMOV:

		/* Copying an 881 register to itself? */
		if (r1==r2 ) 
		{
			p = release(p);
			opt("redundant fmov");
			break;
		}

		/* fmov xx,fpn ; fmov yy,fpn  -> eliminate first fmov */
		/* watch out for fmov xx,fpn; fmov fpn,fpn - in that case
		 * we do not want to get rid of the first fmov */
		if (p->forw!=NULL && m2 == FREG && p->forw->op == FMOV
		    && r2==p->forw->reg2 && r2!=p->forw->reg1) 
		{
			p = release(p);
			opt("redundant fmov");
			break;
		}
		
		/* see if anyone will use it */
                /* SWFfc00082: condition codes must be checked, too */
		if (m2==FREG && 
		    ((r2==reg_fp0) || (r2==reg_fp0+1)) && 
		    !f_reg_used(p->forw, r2) && !fpcc_used(p->forw))
		{
			p = release(p);
			opt("redundant fmov");
			break;
		}

		if (m2==FREG && m1!=FREG)
		{
			r = find_freg(&p->op1, p->subop);
			if (r!=-1)
			{
				if (r==r2)
				{
				  p = release(p);
				  opt("redundant fmov");
				  break;
				}
				cleanse(&p->op1);
				p->mode1 = FREG;
				p->reg1 = r;
				p->subop = EXTENDED;
				opt("fmov xx,fpi => fmov.x fpj,fpi");
			}
		}

		/* If we have an fmov from mem to a flt. pt. reg
		 * followed by a monadic flt pt operation, we
		 * can combine the two. e.g.
		 *
		 *  fmov.d 8(%a6),%fp0		fneg.d 8(%a6),%fp0
		 *  fneg.x %fp0
		 *
		 */
		if (p->forw!=NULL && m2 == FREG && fmonadic(p->forw->op)
		    && r2==p->forw->reg1 && p->forw->mode2==UNKNOWN 
		    && p->forw->op != FTEST)
		    {
		    	p->op = p->forw->op;
		    	release(p->forw);
			dest(&p->op2, p->subop);
		    	opt("fmov xx,fpi; fmono fpi => fmono xx,fpi (881)");
			break;
		    }

		if (p->forw!=NULL && p->forw->op==FMOV
		    && p->subop==p->forw->subop && m1==FREG &&
		    p->forw->mode2==FREG && r1==p->forw->reg2
		    && equarg(&p->op2,&p->forw->op1) )
		    {
			release(p->forw);
		    	opt("fmov fpi,xx; fmov xx,fpi => remove red. move (881)");
		    }

		if ( (p->subop==SINGLE || p->subop==LONG)
			&& m2==FREG && m1==ADISP)
		{
			r = find_reg(&p->op1);
			if ( isD(r) )
			{
				cleanse(&p->op1);
				p->mode1 = DDIR;
				p->reg1 = r;
				opt("fmov.s xx,fpi => fmov.s di,fpi");
			}
		}

		/* make sure that cache is updated */
		if (p->subop==DOUBLE && m1==FREG && m2!=FREG)
		{
		   /* if fmov.d %fp0,-12(%a6) then -8(%a6) is also dest */
		   if (m2==ADISP && r2==reg_a6)
		   {
			p->addr2 += 4;
			dest(&p->op2, p->subop);
			p->addr2 -= 4;
		   }
		   else 
			/* play it safe */
			status_unknown();
		}
		dest(&p->op2, p->subop);
		save_cache(&p->op1, &p->op2, p->subop);

		break;
#endif M68020

#ifdef DRAGON
	case FPMOV:

		/* Copying a dragon register to itself? */
		if (r1==r2 ) 
		{
			p = release(p);
			opt("redundant fpmov");
			break;
		}

		/* fpmov xx,fpan ; fpmov yy,fpan  -> eliminate first fpmov */
		/* watch out for fpmov xx,fpan; fpmov fpan,fpan - in that case
		 * we do not want to get rid of the first fpmov */
		if (p->forw!=NULL && m2 == FPREG && p->forw->op == FPMOV
		    && r2==p->forw->reg2 && r2!=p->forw->reg1) 
		{
			p = release(p);
			opt("redundant fpmov");
			break;
		}

		/* see if anyone will use it */
		if (m2==FPREG && r2==reg_fpa0 && !fp_reg_used(p->forw, r2))
		{
			p = release(p);
			opt("redundant fpmov");
			break;
		}

		if (m2==FPREG && m1!=FPREG)
		{
			r = find_fpreg(&p->op1, p->subop);
			if (r!=-1)
			{
				if (r==r2)
				{
				  p = release(p);
				  opt("redundant fpmov");
				  break;
				}
				cleanse(&p->op1);
				p->mode1 = FPREG;
				p->reg1 = r;
				opt("fmov xx,fpai => fmov fpaj,fpai");
			}
		}

		/* dragon fpmOP code here ???? */

		/* If we have an fpmov from flt. pt. reg to a flt. pt. reg
		 * followed by a monadic flt pt operation, we
		 * can combine the two. e.g.
		 *
		 *  fpmov.d %fpa15,%fpa0	fpneg.d %fpa15,%fpa0
		 *  fpneg.d %fpa0
		 *
		 */
		if (p->forw!=NULL && m1 == FPREG && m2 == FPREG 
		    && fpmonadic(p->forw->op)
		    && p->subop == p->forw->subop
		    && r2==p->forw->reg1 && p->forw->mode2==UNKNOWN
		    && p->forw->op != FPTEST)
		    {
		    	p->op = p->forw->op;
		    	release(p->forw);
			dest(&p->op2, p->subop);
		    	opt("fpmov fpaj,fpai; fpmono fpai => fpmono fpaj,fpai (fpa)");
			break;
		    }


		if (p->forw!=NULL && p->forw->op==FPMOV
		    && p->subop==p->forw->subop && m1==FPREG &&
		    p->forw->mode2==FPREG && r1==p->forw->reg2
		    && equarg(&p->op2,&p->forw->op1) )
		    {
			release(p->forw);
		    	opt("fpmov fpi,xx; fpmov xx,fpi => remove red. move (dragon)");
		    }

		if ( (p->subop==SINGLE || p->subop==LONG)
			&& m2==FPREG && m1==ADISP)
		{
			r = find_reg(&p->op1);
			if ( isD(r) )
			{
				cleanse(&p->op1);
				p->mode1 = DDIR;
				p->reg1 = r;
				opt("fpmov.s xx,fpi => fpmov.s di,fpi");
			}
		}

		/* make sure that cache is updated */
		if (p->subop==DOUBLE && m1==FPREG && m2!=FPREG)
		{
		   /* if fpmov.d %fpa0,-12(%a6) then -8(%a6) is also dest */
		   if (m2==ADISP && r2==reg_a6)
		   {
			p->addr2 += 4;
			dest(&p->op2, p->subop);
			p->addr2 -= 4;
		   }
			/* we can have fpmov.d %fpa0,%d0:%d1 */
		   else if (m2!=DPAIR)
			/* play it safe */
			status_unknown();
		}
		dest(&p->op2, p->subop);
		save_cache(&p->op1, &p->op2, p->subop);

		break;
#endif DRAGON


	case LABEL:
	case DLABEL:
		status_unknown();
		break;

#ifdef M68020
	case FABS:
	case FACOS:
	case FADD:
	case FASIN:
	case FATAN:
	case FCOS:
	case FCOSH:
	case FDIV:
	case FETOX:
	case FINTRZ:
	case FLOG10:
	case FLOGN:
	case FMOD:
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
		
		if (m1!=FREG)
		{
			r = find_freg(&p->op1, p->subop);
			if (r!=-1)
			{
				cleanse(&p->op1);
				p->mode1 = FREG;
				p->reg1 = r;
				p->subop = EXTENDED;
				opt("fop xx,fpi => fop.x fpj,fpi");
			}

			else if ( (p->subop==SINGLE || p->subop==LONG)
				&& m1==ADISP)
			{
				r = find_reg(&p->op1);
				if ( isD(r) )
				{
					cleanse(&p->op1);
					p->mode1 = DDIR;
					p->reg1 = r;
					opt("fop.s xx,fpi => fop.s di,fpi");
				}
			}
		}

		if (m2==UNKNOWN) dest(&p->op1, p->subop);
		else dest(&p->op2, p->subop);
		break;	
#endif M68020

#ifdef DRAGON
	case FPABS:
	case FPADD:
	case FPDIV:
	case FPINTRZ:
	case FPMUL:
	case FPNEG:
	case FPSUB:
		
		if (m1!=FPREG)
		{
			r = find_fpreg(&p->op1, p->subop);
			if (r!=-1)
			{
				cleanse(&p->op1);
				p->mode1 = FPREG;
				p->reg1 = r;
				opt("fpop xx,fpai => fpop fpj,fpai");
			}

			else if ( (p->subop==SINGLE || p->subop==LONG)
				&& m1==ADISP)
			{
				r = find_reg(&p->op1);
				if ( isD(r) )
				{
					cleanse(&p->op1);
					p->mode1 = DDIR;
					p->reg1 = r;
					opt("fpop.s xx,fpai => fpop.s di,fpai");
				}
			}
		}

		if (m2==UNKNOWN) dest(&p->op1, p->subop);
		else dest(&p->op2, p->subop);
		break;	
#endif DRAGON


	default:
		status_unknown();
		break;

	}  /* end of switch */

	}  /* END OF FOR WHOLE LINKED LIST */

} /* end of rmove */


/*
 * Swap args 1 and 2
 * Used because new syntax of CMP is not compatible with split()
 */
swap_args(p)
register node *p;
{
	argument arg;
	arg = p->op1;
	p->op1 = p->op2;
	p->op2 = arg;
}


/*
 * Split a data movement into two parts:
 * o Move quick constant into a data register
 * o Move the data register into the destination.
 *
 * (Note that this is independent of move size (.b/.w/.l)).
 */
split(p)
register node *p;
{
	register int reg, con;
	register node *new, *prev;

	/*
	 * on the 040, splitting isn't faster if the instruction
	 * has no extension words (and is thus 3 words or less)
	 */
	if (oforty && p->mode2 <= DEC) {
		return;
	}
	if (p->mode1==IMMEDIATE && p->type1==INTVAL
	&& (con=p->addr1,EIGHT_BITTABLE(con))
	&& (reg=unused_dreg(p))!=-1) {	/* if spare reg found */

		new = insert(p, MOVE);		/* get MOVE node */
		new->subop = LONG;		/* move.l */
						/* it'll map to moveq */
		make_arg(&new->op1, IMMEDIATE, INTVAL, con);
		make_arg(&new->op2, DDIR, reg);
		make_arg(&p->op1, DDIR, reg);
		opt("move to non-quick split up using d%d", reg);
		dest(&new->op2, new->subop);
		save_cache(&new->op1, &new->op2, new->subop);
		setcc(&new->op2, new->subop);

		/*
		 * If we just split up a stack access and its add,
		 * rejoin them.
		 */
		prev = new->back;
		if (prev!=NULL
		&& prev->op==ADD && prev->subop==LONG
		&& prev->mode1==IMMEDIATE && prev->type1==INTVAL
		&& prev->mode2==ADIR
		&& p->mode2==IND) {

			/* Swap prev and new */
			/* First, remove new altogether */
			prev->forw = p;
			p->back = prev;

			/* Link new before prev */
			new->forw = prev;
			new->back = prev->back;
			prev->back = new;
			if (new->back!=NULL)
				new->back->forw = new;
		}
	}
}


/*
 * This optimization looks for an conditional/unconditional branch
 * to a test/compare of constant values followed by a conditional branch.
 *
 * Based on what the branch will do, we do the following:
 *
 * o Branch is always taken:
 * 	The destination of the original jump is changed
 *	to that of the conditional branch.
 *
 * o Branch is never taken:
 *	The destination of the original jump is changed
 *	to just after the conditional branch.
 *
 * o Branch may or may not be taken:
 *	No change made.
 *
 * It is assumed that p points to a JMP/CBR node.
 *
 *	
 * The instruction cmp a,b computes a-b and sets the codes from that.
 * It's like a subtract instruction, you see.
 * For example:
 *
 *		cmp	&1,&2
 *		jlt	away
 *
 * The jlt always succeeds!  The cmp instruction computes 1-2 (which is -1),
 * and since that is less than zero, the branch is always taken.
 */
redunbr(p)
register node *p;
{
	argument temparg;
	register node *p1;
	register argument *a1, *a2;
	register int compare_result;

	p1 = p->ref;
	if (p1==NULL)			/* if we refer to nothing */
		return;

	p1 = nonlab(p1);		/* find next non-label */
	if (p1==NULL)
		return;			/* ran out of source, give up */

	if (p1->op==TST) {
		a1 = &p1->op1;
		/* make_arg(&temparg,IMMEDIATE,INTVAL,0); */	
		/* fake arg of zero */
		a2 = &temparg;
		a2->mode = IMMEDIATE;
		a2->type = INTVAL;
		a2->addr = 0;
		a2->reg = -1;
		a2->index = -1;
	}

	else if (p1->op==CMP) {
		a1 = &p1->op1;
		a2 = &p1->op2;
	}

	else					/* if not compare or test */
		return;				/* can't do anything */

	p1 = p1->forw;			/* go on to conditional branch op */
	if (p1==NULL || p1->op!=CBR)
		return;

	compare_result = compare(p1->subop, a1, a2);
	if (compare_result==true && p->ref != p1->ref) {
		/*
		 * The conditional branch is always taken.
		 * Replace the target of the original branch
		 * with that of the conditional branch.
		 */
		opt("redundant remove test/compare");
		nrtst++;
		decref(p->ref);
		p->ref = p1->ref;
		p->labno1 = p1->labno1;
		if (p->ref!=NULL)
			p->ref->refc++;
	}
	else if (compare_result==false) {
		/*
		 * The branch is never taken.
		 * Have the original branch go to
		 * just after the later conditional branch.
		 */
		opt("redundant remove test/compare");
		nrtst++;
		decref(p->ref);
		p->ref = insertl(p1->forw);
		p->labno1 = p->ref->labno1;
	}
}



equop(p1, p2)
register node *p1, *p2;
{
	register int o1;

	o1=p1->op;

	if (o1!=p2->op || p1->subop!=p2->subop)	/* op/subop equal? */
		return false;		/* they are not equal */

#ifdef M68020
	if (p1->offset != p2->offset || p1->width != p2->width)
		return false;		/* they are not equal */
#endif

	if (o1==JMP || o1==CBR
	|| o1==LABEL || o1==DLABEL
	|| o1==GLOBAL || o1==DC)
		return false;

	return equarg(&p1->op1, &p2->op1) && equarg(&p1->op2, &p2->op2);
}



/*
 * If passed a label node, this routine will decrement the reference
 * count and, if the reference count goes to zero, remove the node.
 */
decref(p)
register node *p;
{
	if (p==NULL) return;
	if (p->op != LABEL) return;
	if (--p->refc <= 0
	&& (p->forw!=NULL || p->forw->op!=DC)) {
		nrlab++;
		opt("redundant label L%d", p->labno1);
		release(p);
	}
}



/*
 * Search forward in the list of nodes for the first non-label
 * following the input parameter "p".
 */
node *nonlab(p)
register node *p;
{
	while (p!=NULL && p->op==LABEL)
		p = p->forw;
	return(p);
}




/*
 * Forget the current status altogether
 */
status_unknown()
{
	cache_unknown();
	cc_unknown();
}



/*
 * Set the condition code to unknown.
 */
#ifdef DEBUG
cc_unknown()
{
	ccloc.mode = UNKNOWN;
}
#endif DEBUG




/*
 * Try to simplify the data on 'arg'.
 *
 * We don't care where the data comes from, as long as it's the same thing.
 *
 * For example:
 *
 *	---before---			---after---
 *	move.l (a0),d0			move.l (a0),d0
 *	tst.l (a0)			tst.l d0
 *
 * If the areg_ok flag isn't set, then an address register is not
 * acceptable for a replacement.  For example:
 *
 *	---before---			---after---
 *	move.l (a0),a1			move.l (a0),a1
 *	tst.l (a0)			tst.l a1
 *
 * This is *not* acceptable, since tst can't test an address register!
 * Fortunately, find_cache looks in the data registers first,
 * so if we're stuck with an address register we can just give up.
 *
 * The options are:
 *	AREG_OK:		is an address register acceptable?
 *				(if subop is byte, then it's not.)
 *	QUICK_THREE_BIT:	this is an addq/subq,
 *				so the constants 1 to 8 are preferred.
 *	QUICK_EIGHT_BIT:	this is a moveq,
 *				so the constants -128 to 127 are preferred.
 *	ZERO_PREFERRED:		the constant zero is preferred over registers
 *				useful for moves converting to clears.
 */
data_simplify(arg, subop, options)
register argument *arg;		/* the argument to simplify */
register int subop;
register int options;			/* allowable stuff */
{
	register int areg_ok, quick3, quick8, const_ok, zero;
	register argument *cache_const;
	register int cache_reg;


	if (!simplify_addressing)	/* are we allowed to do this? */
		return;			/* exit if not supposed to do this */

	address_simplify(arg);		/* simplify the address first */

	if (subop!=LONG)		/* KLUDGE CITY */
		return;

	areg_ok = options & AREG_OK;
	if (subop==BYTE)
		areg_ok = false;

	quick3 = options & QUICK_THREE_BIT;	/* 1 thru 8 preferred */
	quick8 = options & QUICK_EIGHT_BIT;	/* -128 thru 127 preferred */
	const_ok = options & CONST_OK;		/* is any constant ok? */
	zero = options & ZERO_PREFERRED;	/* is zero preferable? */

	if ((quick3 || quick8 || zero) && !const_ok)
		internal_error("data_simplify: inconsistent options=%d",
				options);


	/* try to resolve it into a quick constant or zero */
	cache_const = find_const(arg);
	if (cache_const!=NULL) {
		if (zero && cache_const->addr==0
		|| quick3 && THREE_BITTABLE(cache_const->addr)
		|| quick8 && EIGHT_BITTABLE(cache_const->addr)) {
			/* only copy if it's not the same thing */
			if (cache_const!=arg) {
				cleanse(arg);
				copyarg(cache_const, arg);
				change_occured = true;
			}
			return;
		}
	}


	/* try to resolve it into a register */

	if (arg->mode==DDIR || arg->mode==ADIR) return;
	cache_reg = find_reg(arg);

	if (cache_reg!=-1 && (areg_ok || !isA(cache_reg))) 
	{
		cleanse(arg);
		arg->mode = isA(cache_reg) ? ADIR : DDIR;
		arg->reg = cache_reg;
		change_occured = true;
		return;
	}


	/* try to resolve it into any old constant */
	if (const_ok && cache_const!=NULL) {
		if (cache_const!=arg) {
			cleanse(arg);
			copyarg(cache_const, arg);
			change_occured = true;
		}
		return;
	}
}



/*
 * Simplify the addressing on arg.
 * We must still refer to the same place, though,
 * not just something with the same data.
 */
address_simplify(arg)
register argument *arg;
{
	register argument *rarg;

	if (!simplify_addressing)	/* are we allowed to do this? */
		return;

	/* what if reg/index is a constant?  add to displacement! */
	/* Be careful if you attempt that - need to take word/long
	 * as well as scale into consideration */

	/* convert indexed mode to displacement mode if index reg==0 */
	if (arg->mode==AINDEX) {
		rarg = &cache[arg->index].op2;	/* addr of index arg */
 		if (cache[arg->index].op && rarg->mode==IMMEDIATE && rarg->type==INTVAL
		 && rarg->addr==0) {
			arg->mode=ADISP;
			change_occured = true;
		}
	}

	/* convert indexed to displacement if x(a0,a2) && a0==0 */
	if (arg->mode==AINDEX && isA(arg->index)) {
		rarg = &cache[arg->reg].op2;
 		if (cache[arg->reg].op && rarg->mode==IMMEDIATE && rarg->type==INTVAL
		 && rarg->addr==0) {
			arg->mode = ADISP;
			arg->reg = arg->index;
			change_occured = true;
		}
	}

	/* convert 0(a0) to (a0) */
	if (arg->mode==ADISP && arg->type==INTVAL && arg->addr==0) {
		arg->mode = IND;
		change_occured = true;
	}

	/*
	 * Try to find a register that contains this address
	 */
	if (arg->mode==ABS_L) {
		register int reg;

		arg->mode = IMMEDIATE;		/* look for the address */
		reg = find_reg(arg);		/* find a register with addr */
		arg->mode = ABS_L;		/* put back proper mode */

		if (isA(reg)) {			/* if addr reg was found */
			make_arg(arg, IND, reg);
			change_occured = true;
		}
	}

#ifdef XXM68020 
/* consider later */
	/* 
	 * If mode is (bd,%a1,*d0.l*8) we would like to replace
	 * a1 by a lower areg. This helps to remove unused regs
	 * in situations like:
	 * 	mov.l %a0,%a1
	 *	mov.l (xx,%a0,%d2.l*4),(xx,%a1,%d3.l*4)
	 * if a1 is not used later on, we could replace it by a0
	 * and get rid of the move.
	 */

	if (arg->mode==AINDBASE && isA(arg->reg)
	    && arg->reg!= reg_a0) {
		rarg = &cache[arg->reg].op2;
 		if (cache[arg->reg].op && rarg->mode==ADIR && rarg->reg < arg->reg)
		{
			arg->reg = rarg->reg;
			change_occured = true;
		}
	}
#endif 

}



/*
 *  Collect data and text segments into one of each with data segment
 *  at the end.
 */
movedat()
{
	register node *p1, *p2;
	register node *datend;
	register int segment_type, final_segment_type;
	node data;
	register node *datp;

	if (first.forw == NULL)			/* is there any code at all? */
		return;				/* if no code, give up now */

	data.forw = NULL;
	datp = &data;

#define IS_DIRECTIVE(op) ((op)==TEXT || (op)==DATA || (op)==BSS)

	/* Find final segment type (TEXT, DATA, etc.) */
	final_segment_type=UNKNOWN;
	for (p1 = first.forw; p1!=NULL; p1 = p1->forw)
		if (IS_DIRECTIVE(p1->op))
			final_segment_type=p1->op;

	for (p1 = first.forw; p1!=NULL; p1 = p1->forw) {
		if (p1->op == DATA) {
			datend = p1;

			/* go to the end of the data segment */
			while (datend->forw!=NULL && datend->op!=TEXT)
				datend = datend->forw;

			/*
			 * Leave TEXT right where it is.
			 * We might have been in a DATA segment before this,
			 * and if we strip the TEXT, we'll put our TEXT in DATA!
			 * Hence, leave the TEXT and strip it later.
			 */
			if (datend->op==TEXT)
				datend = datend->back;

			p2 = p1->back;			/* just before data */
			p2->forw = datend->forw;	/* pinch off data */
			if (datend->forw)
				datend->forw->back = p2;
			datend->forw = NULL;		/* seal off data */
			datp->forw = p1;
			p1->back = datp;
			p1 = p2;
			datp = datend;
		}
	}


	/* Put the data after the code */
	if (data.forw!=NULL) {
		/* Find end of code list */
		for (p1 = &first; p1->forw!=NULL; p1=p1->forw);

		if (p1->op == END) {
			datp->forw = p1;
			p2 = p1->back;
			p1->back = datp;
			p1 = p2;
		}
		p1->forw = data.forw;
		data.forw->back = p1;
	}


	/*
	 * Tack the final segment type onto the end.
	 * This assures that we leave the segment type the
	 * same way we found it.
	 *
	 * If there was no final segment type, then there
	 * must have been no data segments to move around,
	 * and so we didn't change anything.
	 */
	if (final_segment_type!=UNKNOWN) {
		register node *x;

		/* Find end of code list */
		for (p1 = &first; p1->forw!=NULL; p1=p1->forw);

		x = getnode(final_segment_type);
		p1->forw = x;
		x->back = p1;
	}


	/*
	 * A segment directive is one of TEXT/DATA/BSS.
	 *
	 * Delete redundant segment directives.
	 * Also, delete empty segments (i.e., TEXT followed directly by DATA.
	 */

	segment_type = UNKNOWN;
	for (p1 = first.forw; p1!=NULL; p1 = p1->forw) {

		if (IS_DIRECTIVE(p1->op)) {

			/* Is it the same one we're already in? */
			if (p1->op == segment_type) {
				p1 = release(p1);
				continue;
			}

			/* Is it an empty segment? */
			if (p1->forw!=NULL && IS_DIRECTIVE(p1->forw->op)) {
				p1 = release(p1);
				continue;
			}
			segment_type = p1->op;
		}
	}
}


/*
 * Predict what the following will produce:
 *
 *	cmp   arg1,arg2
 *	j<op> away
 *
 * Return one of the following:
 *
 * true:	if the branch will always succeed.
 * false:	if the branch will never succeed.
 * maybe:	if we don't know.
 */
compare(op, arg1, arg2)
register int op;
register argument *arg1, *arg2;
{
	register node *cache_ptr;
	register int n1,n2;
	register boolean eqop;		/* does operator include equality? */

	if (op==COND_HS || op==COND_LS
	|| op==COND_EQ || op==COND_GE || op==COND_LE)
		eqop=true;
	else if (op==COND_LO  || op==COND_HI
	|| op==COND_NE || op==COND_LT || op==COND_GT)
		eqop=false;
	else
		eqop=maybe;


	cache_ptr = find_cache(arg1);
	if (cache_ptr!=NULL)
		arg1 = &cache_ptr->op2;

	cache_ptr = find_cache(arg2);
	if (cache_ptr!=NULL)
		arg2 = &cache_ptr->op2;

	/* Are they both the same register? */
	if (arg1->mode==ADIR && arg2->mode==ADIR && arg1->reg==arg2->reg)
		return eqop;

	if (arg1->mode==DDIR && arg2->mode==DDIR && arg1->reg==arg2->reg)
		return eqop;

	/* Are they both the same place? */
	if (arg1->mode==ABS_L && arg2->mode==ABS_L
	&& arg1->type==STRING && arg2->type==STRING
	&& equstr(arg1->string, arg2->string))
		return eqop;

	/* They must be constants from now on */
	if (arg1->mode!=IMMEDIATE || arg1->type!=INTVAL)
		return maybe;

	if (arg2->mode!=IMMEDIATE || arg2->type!=INTVAL)
		return maybe;

	n1 = arg1->addr;
	n2 = arg2->addr;

	switch(op) {

	case COND_EQ:
		return(n1 == n2);
	case COND_NE:
		return(n1 != n2);
	case COND_LE:
		return(n1 <= n2);
	case COND_GE:
		return(n1 >= n2);
	case COND_LT:
		return(n1 < n2);
	case COND_GT:
		return(n1 > n2);
	case COND_LO:
		return((unsigned)n1 < (unsigned)n2);
	case COND_HI:
		return((unsigned)n1 > (unsigned)n2);
	case COND_LS:
		return((unsigned)n1 <= (unsigned)n2);
	case COND_HS:
		return((unsigned)n1 >= (unsigned)n2);
	case COND_T:
		return true;
	case COND_F:
		return false;
	default:			/* whatever it might be? */
		return maybe;
	}
}



/*
 * Set the condition code 'ccloc' to the value of the argument 'a'
 */
setcc(arg,typ)
register argument *arg;
int                typ;
{
	register int reg;
	register argument *cache_const;

	cleanse(&ccloc);		/* we don't know nothin' */

	/*
	 * If memory is suspect (I/O space, maybe), don't save things.
	 */
	if (!cacheable(arg))
		return;

	if (!natural(arg))			/* if argument too weird */
		return;				/* give up now */

	copyarg(arg, &ccloc);			/* put arg in ccloc */
	cctyp = typ;

	/* try to resolve it into a constant */
	if (arg->mode==IMMEDIATE && arg->type==INTVAL)
		return;

	cache_const = find_const(arg);
	if (cache_const!=NULL) {		/* if constant found */
		cleanse(&ccloc);		/* wipe out old thing */
		copyarg(cache_const, &ccloc);
		return;				/* exit in any case */
	}

	/* try to resolve it into a register */
	if (arg->mode==DDIR || arg->mode==ADIR)
		return;

	reg = find_reg(arg);
	if (isAD(reg)) {
		make_arg(&ccloc, isA(reg) ? ADIR : DDIR, reg);
	}
}



/*
 * Is the argument 'arg' ok to stash away?
 *
 * Examples of non-ok things:
 *	sp addressing (changes to much to keep track of)
 *	pc addressing (good lord)
 *	postincrement stuff
 */
natural(arg)
register argument *arg;
{
	switch (arg->mode) {

	case INC:			/* postincrement changed the reg */
	case PDISP:			/* pc stuff is always exciting */
	case PINDEX:			/* pc stuff is always exciting */
	case PINDBASE:			/* pc stuff is always exciting */
#ifdef PREPOST
	case PCPRE:			/* pc stuff is always exciting */
	case PCPOST:			/* pc stuff is always exciting */
#endif PREPOST
		return false;

	case AINDEX:
	case AINDBASE:
#ifdef PREPOST
	case MEMPRE:
	case MEMPOST:
#endif PREPOST
		if (arg->index==reg_sp)	/* if indexing off the stack ptr */
			return false;
		/* else fall thru and test register */

	case ADIR:
	case IND:
	case DEC:
	case ADISP:
		if (arg->reg==reg_sp)	/* if addressing off sp */
			return false;
		break;
	}

	return true;	/* got thru that gauntlet, must be ok */
}



/*
 * copy argument 'from' to argument 'to'
 */
copyarg(from, to)
register argument *to, *from;
{
	*to = *from;		/* wasn't that simple? */

	if (to->type == STRING)	/* if the label field contains something */
		to->string = copy(to->string);	/* get a private copy */
#ifdef PREPOST
	if (to->od_type == STRING)	/* do the same for the outer disp. */
		to->odstring = copy(to->odstring);
#endif PREPOST
}


/*
 * Is the argument 'arg' already in the condition code?
 *
 * For example, following 'tst d0', d0 is in the condition code.
 */
incc(arg)
register argument *arg;
{
	return equarg(arg,&ccloc);	/* compare them */
}

/*
 * Are arguments 'a' and 'b' equal?
 */
equarg(a,b)
register argument *a, *b;
{

#ifdef DEBUG
	if (a==NULL || b==NULL)
		return false;		/* no null args allowed! */
#endif DEBUG

	if (a->mode != b->mode)
		return false;		/* addr modes must be equal */

	/*
	 * let's look at the individual modes 
	 * INC and DEC modes cannot be eligible for equarg
	 * since (a2)+ is not the same as (a2)+ etc.
	 */
	switch (a->mode) {

	case UNKNOWN:
		return true;	/* for nonexistent arguments */

	case ADIR:
	case DDIR:
	case IND:
#ifdef M68020
	case FREG:
#endif M68020
#ifdef DRAGON
	case FPREG:
#endif DRAGON
		if(a->reg == b->reg)
			return true;
		else
			return false;

	case PDISP:
	case PINDEX:
	case PINDBASE:
#ifdef PREPOST
	case PCPRE:
	case PCPOST:
#endif PREPOST
		return false;		/* no good */

#ifdef PREPOST
	case MEMPRE:
	case MEMPOST:
		if (a->od_type != b->od_type)
			return false;

		switch (a->od_type) {

		case INTLAB:
			if (a->odlabno != b->odlabno) return false;
		case INTVAL:
			if (a->odaddr != b->odaddr) return false;
		case STRING:
			if (!equstr(a->odstring, b->odstring)) return false;
		}
		/* fall into AINDEX to check other stuff */
#endif PREPOST

	case AINDEX:
#ifdef M68020
	case AINDBASE:
#endif 
		if (a->index != b->index)
			return false;
		if (a->word_index != b->word_index)
			return false;
#ifdef M68020
		if (a->scale != b->scale)
			return false;
#endif 

		/* fall into ADISP to check register & displacement */

	case ADISP:
		if (a->reg != b->reg)
			return false;
		/* fall into IMMEDIATE case */

	case ABS_W:
	case ABS_L:
	case IMMEDIATE:
		if (a->type != b->type)
			return false;

		switch (a->type) {

		case INTLAB:
		case INTVAL:
			return (a->addr == b->addr);
		case STRING:
			return equstr(a->string, b->string);
		default:
			/* God only knows what it might be */
			return false;
		}

	default:
		return false;
	}
}


/*
 * So you want to have an argument!
 *
 * Construct one out of the list given.
 */
/* VARARGS2 */
make_arg(arg, mode, a, b)
register argument *arg;
int mode;
{
	cleanse(arg);
	arg->mode = mode;

	switch (mode) {
		
	case UNKNOWN:
		break;

	case DDIR:
		if (!isD(a))
			internal_error("make_arg(DDIR,%d) ???", a);
		arg->reg = a;
		break;

	case ADIR:
	case IND:
	case INC:
	case DEC:
		if (!isA(a))
			internal_error("make_arg(ADIR/IND/INC/DEC,%d) ???", a);
		arg->reg = a;
		break;

	case ADISP:
	case AINDEX:
	case AINDBASE:
	case ABS_W:
	case ABS_L:
	case PDISP:
	case PINDEX:
	case PINDBASE:
#ifdef PREPOST
	case MEMPRE:
	case MEMPOST:
	case PCPRE:
	case PCPOST:
#endif PREPOST
	default:
		internal_error("make_arg called with mode %d", mode);


	case IMMEDIATE:
		arg->type = a;
		switch (arg->type) {
		case STRING:
			arg->string = (char *) b;
			break;
		case INTVAL:
			arg->addr = b;
			break;
		case INTLAB:
			arg->labno = b;
			break;
		default:
			internal_error("make_arg called with IMMEDIATE %d", a);
		}
		break;

	}
}




#ifdef DEBUG
/*
 * Print the cache and condition code.
 */
print_cache()
{
	register node *p;
	boolean any_yet;

	if (!debug4)
		return;

	any_yet=false;				/* nothing seen yet */

	for (p=cache; p<=cache_end; p++)
	{
	        if (p->op == false) continue;
		if (p->mode1!=UNKNOWN) {
			if (p->mode2==UNKNOWN)
				internal_error("print_cache: cache[%d].mode2==UNKNOWN", p-cache);
			any_yet=true;
			printf("** Cache: ");
			print_arg(&p->op1);
			printf(" = ");
			print_arg(&p->op2);
			printf("\n");
		}

	}
	if (!any_yet)
		printf("** No cache information\n");

	if (ccloc.mode!=UNKNOWN) {
		printf("** cc: ");
		print_arg(&ccloc);
		printf("\n");
	}
}
#endif DEBUG

#ifdef M68020
/*
 * Return true if the operator o is a floating point (68881)
 * monadic operator. 
 */
fmonadic(o)
register int o;
{
	switch (o) {
	case FABS:
	case FACOS:
	case FASIN:
	case FATAN:
	case FCOS:
	case FETOX:
	case FINTRZ:
	case FLOG10:
	case FLOGN:
	case FNEG:
	case FSIN:
	case FSINH:
	case FSQRT:
	case FTAN:
	case FTANH:
		return (true);

	default:
		return (false);

	}
}
#endif M68020
#ifdef DRAGON
/*
 * Return true if the operator o is a floating point (dragon)
 * monadic operator. 
 */
fpmonadic(o)
register int o;
{
	switch (o) {
	case FPABS:
	case FPCVD:
	case FPCVL:
	case FPCVS:
	case FPINTRZ:
	case FPNEG:
	case FPTEST:
		return (true);

	default:
		return (false);

	}
}
#endif DRAGON
