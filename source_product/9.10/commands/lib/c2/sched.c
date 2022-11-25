/* @(#) $Revision: 70.1 $ */   
/*
 * Instruction Scheduler
 */


#include "sched.h"


int   flops;		/* # of floating point ops in the block		*/
int   floads;		/* # of floating point loads in the block	*/
int   fstores;		/* # of floating point stores in the block 	*/
int   iops;		/* # of integer ops in the block		*/
int   aloads;		/* # of a register loads in the block		*/

int   cndflops; 	/* # of flops in the candidate list		*/
int   cndfloads; 	/* # of floads in the candidate list		*/
int   cndfstores; 	/* # of fstores in the candidate list		*/
int   cndiops;		/* # of iops in the candidate list		*/

dag  *ccinst;		/* the last instruction to set the cc		*/
int   ccused;		/* cc used at the end of the basic block	*/

dag  *dagnode();
dag  *makedag();
dag  *bestinst30();
dag  *bestinst40();

dag  *dagfreelist = NULL;

int printflag = 1;	/* used in printing the dag			*/

o40pipe o40;

#define LINE "\n\n===========================================================\n"


/***************************************************
 *
 * I N S T   S C H E D
 *
 * Performs instruction scheduling on the list of
 * instructions pointed to by the global "first".
 *
 */
instsched()
{
	node 		*p, *bbstart, *bbend;
	dag		*root;

	root = NULL;

	for (p = first.forw; p != NULL;) {

		flops = floads = iops = aloads = 0;

		/* find a basic block */
		if (bb(&p, &bbstart, &bbend)) {

			/* produce the dag */
			root = makedag(bbstart, bbend);

			if (debug_is) {
				printf(LINE);
				numberdag(root);
				printdag(root);
				printf(LINE);
				fflush(0);
			}

			/* schedule the instructions */
			schedule(root, bbstart->back, bbend->forw);

#ifdef DEBUG
			if (debug_is) {
				printf(LINE);
				fflush(0);
			}
#endif
		}
	}
}


/***************************************************
 *
 * B B
 *
 * Isolates a basic block. "start" and "end" are set 
 * to bracket the basic block, "ptr" is set to the next
 * instruction. BB returns true if the basic block contains
 * something worth scheduling (a flop or an aload), or
 * the debug ("debug_is") flag is true.
 *
 */
bb(ptr, start, end)
node **ptr, 
     **start, 
     **end;
{
	node *p;
	int   flops_or_aloads;
	int   length;

	flops_or_aloads = length = 0;

	/* skip over junk */
	for (
	     p = *ptr; 
	     (p != NULL) &&
	     (isBBEND(p->op) || movDLT(p) ||
	     (pseudop(p) && (p->op != DATA) && (p->op != BSS)));
	     p = p->forw
	    );

	/* if junk is all that was left, we are done */
	if ((p == NULL) || (p->op == DATA) || (p->op == BSS)) {
		*ptr = NULL;
		return(0);;
	}

	*start = p;

	/* while it is not the end of the basic block */
	while (!(isBBEND(p->op) || pseudop(p)) && (length < maxBB)) {

		if (isFLOP(p->op) || isALOAD(&p->op2)) {
			/* have something worth scheduling */
			flops_or_aloads = 1;
		}

		length++;

		p = p->forw;
	}

	/*
	 * if the instruction teminating the basic block uses the
	 * condition codes, then back set flag such that condition
	 * codes will be considered in building the dag
	 */
	if (isiCCBRANCH(p->op) && (*start != p)) {
		ccused = iCC;
	}
	else if (isfCCBRANCH(p->op) && (*start != p)) {
		ccused = fCC;
	}
	else {
		ccused = noCC;
	}

	*end    = p->back;
	*ptr    = p;
#ifdef DEBUG
	return(flops_or_aloads || worthless_is);
#else
	return(flops_or_aloads || debug_is);
#endif
}


/***************************************************
 *
 * M A K E   D A G
 *
 * Produces the dag from a basic block
 *
 */
dag *makedag(start, end)
node *start, *end;
{
	node *p, /* current instruction 		*/
	     *t; /* terminator of the instruction list	*/
	dag  *c, /* candidates for dependencies		*/
	     *ct,/* candidate tail for dependencies	*/
	     *n, /* next candidate			*/
	     *d, /* dag node for misc. uses		*/
	     *r, /* root list				*/
	     *a; /* annotated instruction list		*/
	int   l; /* level (to check for candidates	*/
	int   i; /* interlock value			*/

	r = NULL;
	a = NULL;
	l = 0;
	t = start->back;

	ccinst = 0;

	/* go through the basic block last instruction to first */
	for (p = end; p != t; p = p->back) {

		l++;

		d = dagnode(p);

		itype(d); 
		
		/* if the instruction needs to be annotated, do so */
		if (isUNIOP(p->op) || maybeUNIOP(p->op)) {
			d->annotate = a;
			a = d;
			annotate(p);
		} 

		/* get the timing info */
		if (oforty) {
			itime40(d);
		}
		else {
			itime30(d);
		}

		/*
		 * put the roots of the dag into the list of
		 * candidates to be checked for dependencies
		 * with the current instruction, perform this
		 * loop until there are no candidates
		 */
		for (rootcandy(r,&c,&ct); c != NULL; c = n) {

			/*
			 * check if the current instruction and
			 * the current candidate are dependent
			 */
			if (i = interlock(d,c)) {
				/*
				 * they are dependent so put an
				 * arc in the dag from the current
				 * instruction to the current candidate
				 */
				arc(d,c,&r,i,l);
			}
			else {
				/*
				 * they are not dependent so add
				 * the kids of the current candidate
				 * to the candidate list
				 */
				kidcandy(c,&ct,l);
			}

			/* skip to next candidate */
			n = c->candidate;
			c->candidate = NULL;
		}

		/*
		 * take care of things regarding the 
		 * condition codes, see if this instruction
		 * must interlock with ccinst to prevent
		 * ruining the cc
		 */
		if (ccused != noCC) {
			if (ccruin(d)) {
				arc(d,ccinst,&r,lckVANILLA,l);
			}
		}

		/*
		 * add the current instruction to
		 * the root list for the dag
		 */
		addroot(&r,d);

	}
	/* deannotate those instructions that were annotated */
	for (; a; a = a->annotate) {
		deannotate(a->inst);
	}

	return(r);
}


/***************************************************
 *
 * A N N O T A T E
 *
 * Take an instruction with one operand (source), and give it
 * a second (destination) that describes the result of the
 * instruction
 *
 */
annotate(p)
node *p;
{
	argument *a;
	argument *b;

	switch (p->op) {

	/*
	 * for these instructions, which are
	 * always monadic, just copy the
	 * source to the destination
	 */
	case CLR:
	case EXT:
	case EXTB:
	case NEG:
	case NOT:
	case BFCHG:
	case BFCLR:
	case BFSET:
	case BFTST:
	case SWAP:
	case TST:
	case FTEST:
		a = &p->op1;
		b = &p->op2;
		*b = *a;
		break;

	/*
	 * for PEA the destination is
	 * implicitly -(%sp)
	 */
	case PEA:
		a = &p->op2;
		a->mode = DEC;
		a->reg = reg_sp;
		break;

	/*
	 * for these instructions, which are
	 * sometimes monadic, copy the
	 * source to the destination if the
	 * destination is currently UNKNOWN
	 */
	case ASL:
	case ASR:
	case LSL:
	case LSR:
	case ROL:
	case ROR:
	case FABS:
	case FNEG:
        case FACOS:
        case FASIN:
        case FATAN:
        case FCOS:
        case FCOSH:
        case FETOX:
        case FINTRZ:
        case FLOG10:
        case FLOGN:
        case FSIN:
        case FSINH:
        case FSQRT:
        case FTAN:
        case FTANH:
		b = &p->op2;
		if (b->mode == UNKNOWN) {
			a = &p->op1;
			*b = *a;
		}
		break;

	}
}

/***************************************************
 *
 * D E A N N O T A T E
 *
 * Undo the effects of Annotate
 *
 */
deannotate(p)
node *p;
{
	argument *a;
	argument *b;

	switch (p->op) {

	case CLR:
	case EXT:
	case EXTB:
	case NEG:
	case NOT:
	case PEA:
	case BFCHG:
	case BFCLR:
	case BFSET:
	case BFTST:
	case SWAP:
	case TST:
	case FTEST:
		a = &p->op2;
		a->mode = UNKNOWN;
		a->reg = reg_d0;
		break;

	case ASL:
	case ASR:
	case LSL:
	case LSR:
	case ROL:
	case ROR:
	case FABS:
	case FNEG:
        case FACOS:
        case FASIN:
        case FATAN:
        case FCOS:
        case FCOSH:
        case FETOX:
        case FINTRZ:
        case FLOG10:
        case FLOGN:
        case FSIN:
        case FSINH:
        case FSQRT:
        case FTAN:
        case FTANH:
		a = &p->op1;
		b = &p->op2;
		if (samearg(a,b)) {
			b->mode = UNKNOWN;
			b->reg = reg_d0;
		}
		break;
	}
}


/***************************************************
 *
 * R O O T   C A N D Y
 *
 * Put the roots of the dag into the candidate list
 *
 */
rootcandy(r,c,ct)
dag  *r;
dag **c;
dag **ct;
{
	dag *p;

 	for (p = r; p != NULL; p = p->rootn) {
		p->candidate = p->rootn;
		*ct = p;
	}
	*c = r;
}


/***************************************************
 *
 * K I D   C A N D Y
 *
 * Put the kids of the candidate "c" at the end of
 * the candidate list if "c" is their only parent
 *
 */
kidcandy(c,ct,l)
dag  *c;
dag **ct;
int   l;
{
	dag **p;
	int   i;
	int   k;

	k = c->kids;
	if (k) {
		p = c->kid;
 		for (i = 0; i < k; i++) {
			if ((*p)->level != l) {
				(*ct)->candidate = *p;
				(*p)->candidate = NULL;
				(*p)->level = l;
				*ct = *p;
			}
			p++;
		}
	}
}


/***************************************************
 *
 * K I D   R O O T
 *
 * Put the kids of the root node "d" into
 * the root list
 *
 */
kidroot(d,r)
dag  *d,**r;
{
	dag **p;
	int   i;
	int   k;

	k = d->kids;
	p = d->kid;

 	for (i = 0; i < k; i++,p++) {
		if (!--((*p)->parents)) {
			addroot(r,*p);
			addrootparent(*p);
		}
	}
}



/***************************************************
 *
 * I N T E R L O C K
 *
 * Detect dependencies and interlocks between
 * instructions d1 and d2
 *
 */
interlock(di1,di2)
dag *di1,
    *di2;
{
	node	 *i1, /* instruction 1			*/
		 *i2; /* instruction 2			*/
	argument *s1, /* source operand of instr 1	*/
		 *d1, /* source operand of instr 2	*/
		 *s2, /* destination operand of instr 1	*/
		 *d2; /* destination operand of instr 2	*/
	int       o1, /* operator of instr 1		*/
		  o2; /* operator of instr 2            */
	int       o1LEA, /* true if o1 is a PEA or LEA	*/
		  o2LEA; /* true if o2 is a PEA or LEA	*/
	int       o1EXG, /* true if o1 is a EXG 	*/
		  o2EXG; /* true if o2 is a EXG		*/
	int       l1, /* operand length for instr 1     */
	          l2; /* operand length for instr 2     */

	/* initialize all the variables. LEA's and PEA's
	 * are special cased because loading or pushing 
	 * an address is not a use of the contents. EXG
	 * is a special case because the source operand
	 * is really a destination
	 */

	i1 = di1->inst;
	o1 = i1->op;
	l1 = i1->subop;
	s1 = &i1->op1;
	d1 = &i1->op2;
	o1LEA = ((i1->op == LEA) || (i1->op == PEA)) ? 1 : 0;
	o1EXG = (i1->op == EXG) ? 1 : 0;

	i2 = di2->inst;
	o2 = i2->op;
	l2 = i2->subop;
	s2 = &i2->op1;
	d2 = &i2->op2;
	o2LEA = ((i2->op == LEA) || (i2->op == PEA)) ? 1 : 0;
	o2EXG = (i2->op == EXG) ? 1 : 0;

	switch (d1->mode) {

	case UNKNOWN:
		internal_error("unknown mode in interlock");
		return(0);
 
	case ADIR:
	case DDIR:
	case FREG:
		if (usereg(d1->reg, s2)) {
			if (oforty && (d1->mode == ADIR) &&
			    (s1->mode != ADIR) && (s1->mode != IMMEDIATE) &&
			    isAIND(s2->mode)) {
				return(lckAR);
			}
			else if ((d1->mode == FREG) && (o1 != FMOV)) {
				return(lckFPR);
			}
			else {
				return(lckVANILLA);
			}
		}
		else if (usereg(d1->reg, d2)) {
			if ((d1->mode == FREG) && isFLOAD(o2,d2)) {
				return(lckFPR);
			}
			else {
				return(lckVANILLA);
			}
		}
		break;

	case INC:
	case DEC:
		if (usereg(d1->reg, s2)) {
			if (isAIND(s2->mode)) {
				return(lckAR);
			}
			else {
				return(lckVANILLA);
			}
		}
		else if (usereg(d1->reg, d2) ||
		         (o2LEA ? 0 : usemem(d1, l1, s2, l2)) ||
			 usemem(d1, l1, d2, l2)) {
			return(lckVANILLA);
		}
		break;

	case IND:
	case ADISP:
		if (setreg(d1->reg, s2, 0) ||
		    setreg(d1->reg, d2, 1) ||
		    (o2LEA ? 0 : usemem(d1, l1, s2, l2)) ||
		    usemem(d1, l1, d2, l2)) {
			return(lckVANILLA);
		}
		break;

	case PDISP:
	case PINDEX:
	case PINDBASE:
		break;

	case AINDEX:
	case AINDBASE:
		if (setreg(d1->reg, s2, 0) ||
		    setreg(d1->index, s2, 0) ||
		    setreg(d1->reg, d2, 1) ||
		    setreg(d1->index, d2, 1) ||
		    (o2LEA ? 0 : usemem(d1, l1, s2, l2)) ||
		    usemem(d1, l1, d2, l2)) {
			return(lckVANILLA);
		}
		break;

	case DPAIR:
		if (usereg(d1->reg, s2) || usereg(d1->index, s2) ||
		    usereg(d1->reg, d2) || usereg(d1->index, d2)) {
			return(lckVANILLA);
		}
		break;

	case ABS_W:
	case ABS_L:
	case IMMEDIATE:
		if ((o2LEA ? 0 : usemem(d1, l1, s2, l2)) ||
		     usemem(d1, l1, d2, l2)) {
			return(lckVANILLA);
		}
		break;

	}

	switch (s1->mode) {

	case UNKNOWN:
		internal_error("unknown mode in interlock");
		return(0);

	case ADIR:
	case DDIR:
	case FREG:
		if (o1EXG) {
			if (usereg(s1->reg, s2)) {
				if (s1->mode == ADIR) {
					if (isAIND(s2->mode)) {
						return(lckAR);
					}
					else {
						return(lckVANILLA);
					}
				}
				else {
					return(lckVANILLA);
				}
			}
			else if (usereg(s1->reg, d2)) {
				return(lckVANILLA);
			}
		}
		else if (o2EXG) {
			if (setreg(s1->reg, s2, 1) || setreg(s1->reg, d2, 1)) {
				return(lckVANILLA);
			}
		}
		else if (setreg(s1->reg, s2, 0) || setreg(s1->reg, d2, 1)) {
			return(lckVANILLA);
		}
		break;

	case INC:
	case DEC:
		if (usereg(s1->reg, s2) || 
		    usereg(s1->reg, d2) || 
		    (o1LEA ? 0 : usemem(s1, l1, d2, l2))) {
			return(lckVANILLA);
		}
		break;

	case IND:
	case ADISP:
		if (setreg(s1->reg, s2, 0) ||
		    setreg(s1->reg, d2, 1) ||
		    (o1LEA ? 0 : usemem(s1, l1, d2, l2))) {
			return(lckVANILLA);
		}
		break;

	case PDISP:
	case PINDEX:
	case PINDBASE:
		break;

	case AINDEX:
	case AINDBASE:
		if (setreg(s1->reg, s2, 0)   ||
		    setreg(s1->index, s2, 0) ||
		    setreg(s1->reg, d2, 1)   ||
		    setreg(s1->index, d2, 1) ||
		    (o1LEA ? 0 : usemem(s1, l1, d2, l2))) {
			return(lckVANILLA);
		}
		break;

	case ABS_W:
	case ABS_L:
	case IMMEDIATE:
		if (o1LEA ? 0 : usemem(s1, l1, d2, l2)) {
			return(lckVANILLA);
		}
		break;


	}

	return(lckNULL);
}


/***************************************************
 *
 * U S E   R E G
 *
 * Does argument arg make use of register reg?
 *
 */
usereg(reg,arg)
int       reg;
argument *arg;
{
	int m;

	m = arg->mode;

	switch(m) {

		case PDISP:
		case ABS_W:
		case ABS_L:
		case IMMEDIATE:
			return(0);

		case AINDEX:
		case AINDBASE:
		case PINDEX:
		case PINDBASE:
		case DPAIR:
			if (reg == arg->index) {
				return(1);
			}
			if ((m == PINDEX) || (m == PINDBASE)) {
				return(0);
			}

		case DDIR:
		case ADIR:
		case FREG:
		case IND:
		case INC:
		case DEC:
		case ADISP:
			if (arg->reg == reg) {
				return(1);
			}
			return(0);
	}
}


/***************************************************
 *
 * S E T   R E G
 *
 * Does argument arg set register reg?
 *
 */
setreg(reg, arg, dest)
int       reg;
argument *arg;
int       dest;
{
	int m;

	m = arg->mode;

	switch(m) {

		case IND:
		case ADISP:
		case AINDEX:
		case AINDBASE:
		case PDISP:
		case PINDEX:
		case PINDBASE:
		case ABS_W:
		case ABS_L:
		case IMMEDIATE:
			return(0);

		case DPAIR:
			if (arg->index == reg) {
				return(dest);
			}
		case DDIR:
		case ADIR:
		case FREG:
			if (arg->reg == reg) {
				return(dest);
			}
			return(0);

		case INC:
		case DEC:
			if (arg->reg == reg) {
				return(1);
			}
			return(0);
	}
}


/***************************************************
 *
 * U S E   M E M
 *
 * Do arguments arg1 and arg2 access the same memory?
 *
 */
usemem(arg1, size1, arg2, size2)
argument *arg1;
int       size1;
argument *arg2;
int       size2;
{
	int m1,m2;
	int r1,r2;

	m1 = arg1->mode;
	r1 = arg1->reg;

	m2 = arg2->mode;
	r2 = arg2->reg;

	switch(m2) {

		case DDIR:
		case ADIR:
		case FREG:
		case PDISP:
		case PINDEX:
		case PINDBASE:
		case IMMEDIATE:
		case DPAIR:
			return(0);

		case IND:
		case ADISP:
		case INC:
		case DEC:

			if (isvolatile(arg1) || isvolatile(arg2)) {
				return(1);
			}

			/*
			 * if any A-register other then a6 or sp
			 * is used indirectly then all of memory
			 * is considered touched
			 */

			if (arg2->reg < reg_a6) {
				return(1);
			}

			if (anyMEM(arg1)) {
				return(1);
			}

			return(overlapargs(arg1, size1, arg2, size2));

		case ABS_W:
		case ABS_L:
			if (isvolatile(arg1) || isvolatile(arg2)) {
				return(1);
			}

			if (anyMEM(arg1)) {
				return(1);
			}

			return(overlapargs(arg1, size1, arg2, size2));

		case AINDEX:
		case AINDBASE:
			return(1);

	}
}


/***************************************************
 *
 * A R C
 *
 * Draw an arc from "from" to "to" in the dag
 *
 */
arc(from, to, root, ilock, level)
dag  *from,
     *to,
    **root;
int   ilock;
int   level;
{
	dag **p;
	int   i;
	int   k;

	if (from->kids == from->maxkids) {
		from->maxkids *= 2;
		from->kid = (struct dags * *) 
		realloc(from->kid,from->maxkids * sizeof(struct dags *));
		from->kidintrlck = (byte *) 
		realloc(from->kidintrlck,from->maxkids * sizeof(byte));

	}
	from->kid[from->kids] = to;
	from->kidintrlck[from->kids++] = ilock;
	subroot(root,to);
	to->parents++;
	to->level = level;

	if (k = to->kids) {
		p = to->kid;
 		for (i = 0; i < k; i++) {
			(*p)->level = level;
			p++;
		}
	}
}



/***************************************************
 *
 * C C   R U I N
 *
 * Take care of condition code stuff & return true
 * if this instruction will ruin the cc if it is
 * not made to interlock with ccinst.
 *
 */
ccruin(d)
dag  *d;
{
	int   i;
	int   k;

	if (ccused == iCC && !setsiCC(d->inst)) {
		return(0);
	}

	if (ccused == fCC && !setsfCC(d->inst)) {
		return(0);
	}

	d->cc = 1;

	if (!ccinst) {
		ccinst = d;
		return(0);
	}

	k = d->kids;
	if (k) {
 		for (i = 0; i < k; i++) {
			if (d->kid[i]->cc) {
				return(0);
			}
		}
	}
	return(1);
}

/***************************************************
 *
 * A D D   R O O T
 *
 * Add node d to the root list
 *
 */
addroot(r,d)
dag **r,
     *d;
{
	if (*r) {
		d->rootn = *r;
		(*r)->rootp = d;
	}
	else {
		d->rootn = NULL;
	}
	d->rootp = NULL;
	*r = d;

	switch (d->insttype) {

	case iFLOAD:
		cndfloads++;
		break;

	case iFSTORE:
		cndfstores++;
		break;

	case iFLOP:
		cndflops++;
		break;

	case iALOAD:
	case iIOP:
		cndiops++;
		break;

	}
}


/***************************************************
 *
 * S U B   R O O T
 *
 * Remove node d from the root list
 *
 */
subroot(r,d)
dag **r,
     *d;
{
	if (d->rootn || d->rootp || (*r == d)) {
		if (d->rootn) {
			d->rootn->rootp = d->rootp;
		}
		if (d->rootp) {
			d->rootp->rootn = d->rootn;
		}
		else {
			*r = d->rootn;
		}
		d->rootn = NULL;
		d->rootp = NULL;
	
		switch (d->insttype) {
	
		case iFLOAD:
			cndfloads--;
			break;
	
		case iFSTORE:
			cndfstores--;
			break;
	
		case iFLOP:
			cndflops--;
			break;
	
		case iALOAD:
		case iIOP:
			cndiops--;
			break;
	
		}
	}
}


/***************************************************
 *
 * S C H E D U L E
 *
 * Schedule the instructions in the dag pointed
 * to by "root" and insert them between "head" 
 * and "tail".
 *
 */
schedule(root, head, tail)
dag  *root;
node *head,
     *tail;
{
	dag   *d, **p;
	dag   *last, *lastflop;
	node  *s, *st;
	int    i,  k;
	int    lock;
	int    floptail;

	if (oforty) {
		initforty();
	}
	else {
		last = lastflop = NULL;
		floptail = 0;
	}

	s = st = NULL;

	/*
	 * sets things up to help schedule a flop as quickly
	 * as possible
	 */
	setrootparents(root);

	while (root) {

		/* pick the "best" instruction */
		if (oforty) {
			d = bestinst40(root);
			seteac(d);
		}
		else {
                	if (floptail && (last != lastflop)) {
                        	d = bestinst30(
					root,last,lastflop,floptail,&lock
				    );
                	}
                	else {
                        	d = bestinst30(
					root,last,NULL,floptail,&lock
				    );
                	}
		}

#ifdef DEBUG
		if (debug_is) {
			printf("\nScheduled: %x",d);
			fflush(0);
		}
#endif
		if (oforty) {
			while (!eacfree()) {
				ticktock();
			}
		}
		else {
                	newtail(d,lock,&floptail);

                	if (last && (last != lastflop)) {
                        	freedagnode(last);
                	}

                	last = d;

                	if (d->insttype <= iFSTORE) {
                        	if (lastflop) {
                                	freedagnode(lastflop);
                        	}
                        	lastflop = d;
                	}
		}

		deduct(d);

		/*
		 * take "d" out of the root list, and
		 * readjust the rootparent count of
		 * its children
		 */

		subroot(&root,d);

		subrootparent(d);

		/* insert the chosen instruction into the chain */

		if (s) {
			st->forw = d->inst;
			d->inst->back =  st;
			st = d->inst;
		}
		else { 
			s = d->inst; 
			st = d->inst; 
		} 

		/*
		 * add d's children to the root list if
		 * d was their only parent
		 */

		kidroot(d,&root);

	}

	/* clean up */

	if (oforty) {
		cleanforty();
	}
	else {
		freedagnode(last);
	}

	head->forw = s;
	s->back = head;

	if (tail) {
		st->forw = tail;
		tail->back = st;
	}

}



/***************************************************
 *
 * S E T   R O O T   P A R E N T S
 *
 * For all children of all the root nodes
 * set the number of root parents they 
 * have. The root parents field is used to 
 * determine when a node is easy to make a
 * root node.  That is, if parents==rootparents
 * we know that all its parents are roots and thus
 * the path to this node is easy to schedule
 *
 */
setrootparents(r)
dag *r;
{
	dag  *d;

	for (d = r; d; d = d->rootn) {
		addrootparent(d);
	}
}


/***************************************************
 *
 * A D D   R O O T   P A R E N T S
 *
 * increment the root parents fields of the
 * children of this particular root node
 *
 */
addrootparent(d)
dag *d;
{
	int   i;
	int   k = d->kids;
	dag **p = d->kid;

 	for (i = 0; i < k; i++,p++) {
		(*p)->rootparents++;
	}
}


/***************************************************
 *
 * S U B   R O O T   P A R E N T S
 *
 * decrement the root parents fields of the
 * children of this particular root node
 *
 */
subrootparent(d)
dag *d;
{
	int   i;
	int   k = d->kids;
	dag **p = d->kid;

 	for (i = 0; i < k; i++,p++) {
		(*p)->rootparents--;
	}
}



/***************************************************
 *
 * D A G N O D E
 *
 * allocate a dagnode for the instruction "p"
 * get it from the free list if possible
 *
 */
dag *dagnode(p)
node *p;
{
	static long m = 0;

	dag *d;

	if (dagfreelist) {
		d = dagfreelist;
		dagfreelist = d->rootn;
	}
	else {
		d = (dag *) malloc(sizeof(dag));
		m += sizeof(dag);
		if (!d) {
			internal_error("Could not malloc a dag node");
		}
		d->maxkids = 10;
		d->kid = (struct dags * *) 
			malloc(d->maxkids * sizeof(struct dags *));
		d->kidintrlck = (byte *) 
			malloc(d->maxkids * sizeof(byte));
	}
	d->inst = p;
	d->rootn = NULL;
	d->rootp = NULL;
	d->annotate = NULL;
	d->candidate = NULL;
	if (oforty) {
		d->time.t40.eac = 0;
		d->time.t40.eac_gets_xu = 0;
		d->time.t40.eaf = 0;
		d->time.t40.ixu = 0;
		d->time.t40.fcu = 0;
		d->time.t40.fxu = 0;
		d->time.t40.fnu = 0;
		d->time.t40.cvrt = 0;
	}
	else {
		d->time.t30.head = 0;
		d->time.t30.body = 0;
		d->time.t30.tail = 0;
	}
	d->insttype = 0;
	d->parents = 0;
	d->rootparents = 0;
	d->kids = 0;
	d->cc = 0;
	d->printflag = 0;
	d->level = 0;
	d->number = 0;

	return(d);
}


/***************************************************
 *
 * F R E E   D A G N O D E
 *
 * free up a dagnode 
 *
 */
freedagnode(d)
dag *d;
{
	if (d) {
#ifdef DEBUG
		if (d == dagfreelist) {
			internal_error("freeing the same dagnode twice");
		}
		else {
#endif
			d->rootn = dagfreelist;
			d->rootp = NULL;
			dagfreelist = d;
#ifdef DEBUG
		}
#endif
	}
}


/***************************************************
 *
 * P R I N T   D A G
 *
 * print the dag (what else?)
 *
 */
printdag(d)
dag *d;
{
	node *ff,*nf;
	int i;

        if (d->printflag == printflag) return;

	d->printflag = printflag;
        ff = first.forw;
	first.forw = d->inst;

	nf = d->inst->forw;
	d->inst->forw = NULL;

#ifdef DEBUG
	printf("Node = %x\n",d);
#else
	printf("Node = %d\n",d->number);
#endif
#ifdef DEBUG
	if (oforty) {
		printf(
		    "eac = %d   eac_xu = %d   eaf = %d\n",
	       	    d->time.t40.eac, d->time.t40.eac_gets_xu,
		    d->time.t40.eaf
		);
		printf(
		    "ixu = %d   fcu = %d   fxu = %d   fnu = %d   cvrt = %d\n",
	       	    d->time.t40.ixu, d->time.t40.fcu,
		    d->time.t40.fxu,d->time.t40.fnu,d->time.t40.cvrt
		);
	}
	else {
	        printf(
		    "Head = %d  Body = %d  Tail = %d",
		    d->time.t30.head,d->time.t30.body,d->time.t30.tail
		);
	}
#endif
	printf("Inst =");
	output();
	first.forw = ff;
	d->inst->forw = nf;
	printf("Kids = ");
	if (d->kids == 0) {
		printf("none");
	}
	else {
		for (i = 0; i < d->kids; i++) {
#ifdef DEBUG
			printf("(%x,%d)  ",d->kid[i],d->kidintrlck[i]);
#else
			printf("(%d,%d)  ",d->kid[i]->number,d->kidintrlck[i]);
#endif
		}
	}
	printf("\n\n");
	for (i = 0; i < d->kids; i++) {
		printdag(d->kid[i]);
	}
	if (d->rootn) {
		printdag(d->rootn);
	}
}


/***************************************************
 *
 * N U M B E R   D A G
 *
 * number the dag nodes so that we can
 * print the dag
 *
 */
numberdag(d)
dag *d;
{
	int n = 1;

	numdag(d,&n);
}



/***************************************************
 *
 * N U M   D A G
 *
 * does all the real work for number dag
 *
 */
numdag(d,n)
dag *d;
int *n;
{
	int i;

        if (d->number) return;

	d->number = *n;

	*n = *n + 1;

	for (i = 0; i < d->kids; i++) {
		numdag(d->kid[i],n);
	}

	if (d->rootn) {
		numdag(d->rootn,n);
	}
}



/***************************************************
 *
 * D E D U C T
 *
 * decrement the appropriate counter
 *
 */
deduct(d)
dag *d;
{
	if (d->insttype == iFLOP) {
		flops--;
	}
	else if (d->insttype == iFLOAD) {
		floads--;
	}
	else if (d->insttype == iFSTORE) {
		fstores--;
	}
	else if (d->insttype == iALOAD) {
		aloads--;
	}
	else {
		iops--;
	}
}

/***************************************************
 *
 * I T Y P E
 *
 * Set the insttype field, and bump the 
 * appropriate counter
 *
 */
itype(d)
dag *d;
{
	if (isFPOP(d->inst->op)) {
		internal_error("dragon operator in itype");
	}

	if (isFLOP(d->inst->op)) {
		d->insttype = iFLOP;
		flops++;
	}
	else if (isFLOAD(d->inst->op,&d->inst->op2)) {
		d->insttype = iFLOAD;
		floads++;
	}
	else if (isFSTORE(d->inst->op,&d->inst->op2)) {
		d->insttype = iFSTORE;
		fstores++;
	}
	else if (isALOAD(&d->inst->op2)) {
		d->insttype = iALOAD;
		aloads++;
	}
	else {
		d->insttype = iIOP;
		iops++;
	}
}


/***************************************************
 *
 * S A M E   A R G
 *
 * Are "a" and "b" the same argument?
 *
 */
samearg(a,b)
register argument *a, *b;
{

	if (a->mode != b->mode)
		return(0);		/* addr modes must be equal */

	switch (a->mode) {

	case ADIR:
	case DDIR:
	case IND:
	case INC:
	case DEC:
	case FREG:
		if(a->reg == b->reg)
			return(1);
		else
			return(0);

	case PDISP:
	case PINDEX:
	case PINDBASE:
		return(0);		/* no good */

	case AINDEX:
	case AINDBASE:
		if (a->index != b->index)
			return(0);

		if (a->word_index != b->word_index)
			return(0);

		if (a->scale != b->scale)
			return(0);

		/* fall into ADISP to check register & displacement */

	case ADISP:
		if (a->reg != b->reg)
			return(0);
		/* fall into IMMEDIATE case */

	case ABS_W:
	case ABS_L:
	case IMMEDIATE:
		if (a->type != b->type)
			return(0);

		switch (a->type) {

		case INTLAB:
		case INTVAL:
			return (a->addr == b->addr);
		case STRING:
			return equstr(a->string, b->string);

		case UNKNOWN:
			return(1);

		default:
			/* God only knows what it might be */
			return(0);
		}

	default:
		return(0);
	}
}

max(a,b)
int a,b;
{
	return(a > b ? a : b);
}

min(a,b)
int a,b;
{
	return(a < b ? a : b);
}
