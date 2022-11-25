/* @(#) $Revision: 70.1 $ */   
#include "o68.h"

/*
 * define HREG - the highest register number. Entries 0 thru HREG
 * in the cache are reserved for the registers.
 */
#ifndef M68020
#define	HREG	reg_a7		/* 68010 */
#else
#ifndef DRAGON
#define	HREG	reg_fp7		/* 68020 */
#else
#define	HREG	reg_fpa15	/* dragon */
#endif DRAGON
#endif M68020

node *find_empty_cache();
node *look_in_cache();


/* 
 * Tuning changes: for each entry in the cache the "op" fld. will
 * indicate whether that entry is currently a valid entry or not.
 * op is true (1) when the entry is in the cache and false (0) otherwise.
 * Similarly in aregs (a register cache) a mode of unknown implies
 * an invalid entry. The same principle is also used in ccloc.
 * The entity is cleansed only before the next use.
 */

/*
 * Set values in the cache to unknown.
 */
cache_unknown()
{
    {
	register node *p;
	register node *q;

	bugout("4: cache_unknown");
	q = cache_end;
	p = cache;
	do 
	{ 
		p->op = false;			/* not a valid entry */
		p++;
	}
	while (p<=q);

	cache_end = &cache[HREG];		/* always include regs */
    }

    {

					/* cleanse areg cache */
	register argument *arg;
	register argument *arg7;
	arg = &aregs[0];
	arg7 = &aregs[7];
	do 
	{ 
		arg->mode = UNKNOWN ; 
		arg++; 
	}
	while (arg <= arg7);
    }
}


/*
 * Save_cache stores the info from src into dst in the cache.
 */
save_cache(src, dst, size)
register argument *src, *dst;
register short size;
{
	register node *cache_ptr;

#ifdef DEBUG
	if (debug4) {
		printf("** save_cache(");
		print_arg(src);
		printf(",");
		print_arg(dst);
		printf(")\n");
	}
#endif DEBUG

	/*
	 * If memory is faulty, only save immediate stuff & registers.
	 */
	if (!cacheable(src) || !cacheable(dst))
		return;
		
	/*
	 * if "move (a0),a0" - do not store in cache since (a0) is
	 * destroyed if we move it into a0 
	 */
	if (dst->mode == ADIR && src->reg == dst->reg)
		return;
#ifdef M68020
	/* same for "move (%a0,%d0),%d0" */
	if ((dst->mode == DDIR || dst->mode == ADIR) && src->index == dst->reg)
		return;
#endif

	/*
	 * if "fmov.[bwl]"
	 * do not store in cache since fmov performs implicit conversion
	 */
	if ((src->mode == FREG || dst->mode == FREG)
	    && (size == LONG || size == WORD || size == BYTE))
		return;

	/*
	 * The source register does not contain the same value as the
	 * destination memory location if the store does not use the
	 * same subop size as the instruction that last changed the
	 * source register
	 *
	 * An example:
	 *   fmul.x %fp1,%fp2
	 *   fmov.d %fp2,8(%a6)
	 * 
	 * 8(%a6) contains a 64 bit value which is not exactly
	 * equal to the contents of %fp2
	 */
	if ((src->mode == FREG) && (dst->mode != FREG) &&
	    (size != fpsize[src->reg])) {
		return;
	}

	/* find where this should go. */
	cache_ptr = look_in_cache(dst);		/* try to find it */

	if (cache_ptr==NULL) {			/* Is it in the cache? */

		if (isREGMODE(dst->mode))	/* if a register */
			cache_ptr = &cache[dst->reg];	/* use reserved spot */
		else 				/* find an empty spot */
			cache_ptr = find_empty_cache();
		
		copyarg(dst, &cache_ptr->op1);	/* copy name */
	}

	cleanse(&cache_ptr->op2);		/* clean out destination */
	copyarg(src, &cache_ptr->op2);		/* copy in the value */
	cache_ptr->op = true;			/* mark as valid */
	cache_ptr->subop = size;

}

/*
 * Save_areg_cache  updates the areg cache on encountering an LEA
 */
save_areg_cache(src,dst)
register argument *src;
register char dst;
{
	/* cannot save lea (a0),a0 */
	if (src->reg == reg_a0+dst) return;

	cleanse(&aregs[dst]);
	copyarg(src, &aregs[dst]);
}

/*
 * Find an empty or replacable place in the cache
 */
node *
find_empty_cache()
{
	static node *next = &cache[HREG+1];	/* who to replace next */
	register node *p;

	/* first, try to find an empty spot */
	/* start looking after the registers */
	for (p = &cache[HREG+1]; p<=&cache[CACHE_SIZE]; p++)
		if (p->op==false) {
			if (p>cache_end)		/* past current end? */
				cache_end=p;		/* reset high-mark */
			cleanse(&p->op1);
			/* op2 is cleansed in save_cache */
			return (p);
		}


	/* Oh well, no empty spots, so just use 'next' */
	if (next++ >= &cache[CACHE_SIZE])
		next = &cache[HREG+1];

	/* clean out this spot */
	cleanse(&next->op1);		/* op2 is cleansed in save_cache */

	if (next>cache_end)		/* past current end? */
		cache_end=next;		/* reset high-mark */
	return (next);
}





/*
 * Return cache number of arg's cache entry, NULL otherwise.
 * Try to resolve to a constant.
 * This may entail calling look_in_cache several times.
 */
node *
find_cache(arg)
register argument *arg;
{
	register node *new, *cache_ptr;
	register count;

	cache_ptr = NULL;
	for (count=10; count; count--) {

		/* do we have something acceptable? */
		if (arg->mode==IMMEDIATE && arg->type==INTVAL)
			return cache_ptr;
		
		new = look_in_cache(arg);
		if (new!=NULL) {
			cache_ptr = new;
			arg = &cache_ptr->op2;
		}
	}

	return cache_ptr;
}



/*
 * Try to resolve the argument into a constant.
 *
 * Return an argument pointer to the constant or NULL.
 */
argument *
find_const(arg)
register argument *arg;
{
	register node *new;
	register count;

	if (arg==NULL)
		return NULL;

	for (count=10; count; count--) {

		/*
		 * Do we have something acceptable?
		 */
		if (arg->mode==IMMEDIATE && arg->type==INTVAL)
			return arg;
		
		/*
		 * Try looking in the cache again.
		 */
		new = look_in_cache(arg);
		if (new==NULL)				/* if not found */
			return NULL;			/* return failure */

		arg = &new->op2;
	}

	/*
	 * Couldn't find anything after 10 tries, give up.
	 */
	return NULL;
}

		

/*
 * Return pointer to arg's cache entry, NULL otherwise
 */
node *
look_in_cache(arg)
register argument *arg;
{
	register node *p;
	register node *q;

	q = cache_end;
	if (arg) {
		for (p=cache; p<=q; p++) {
			if (p->op && arg->mode==p->mode1 && equarg(&p->op1, arg))
				return p;
		}
	}

	return NULL;		/* couldn't find it */
}


/*
 * Return register that contains arg, -1 otherwise.
 */
find_reg(arg)
register argument *arg;
{
	register node *p;
	register node *q;

#ifdef DEBUG
	if (arg==NULL)
		internal_error("find_reg: null argument");

	/* is it a register already? */
	/* we make sure that it is not in all the calling routines */
	if (arg->mode==ADIR || arg->mode==DDIR)
		internal_error("find_reg: register already");
#endif DEBUG

	/* look in the registers */
	q = &cache[15];
	for (p=cache; p<=q; p++) {
		if (p->op && equarg(&p->op2, arg))
			return (p-cache);	/* return reg number */
	}

	/* look in memory for one that contains it */
	q = cache_end;
	for (; p<=q; p++) {
		if (p->op 
		&& (p->mode2==ADIR || p->mode2==DDIR)
		&& equarg(&p->op1, arg))
			return p->reg2;
	}

	return (-1);		/* couldn't find it */
}

#ifdef M68020
/*
 * Return 68881 register that contains arg, -1 otherwise.
 */
find_freg(arg,size)
register argument *arg;
register short size;
{
	register node *p;

	if (arg==NULL)
		internal_error("find_reg: null argument");

	/* is it a register already? */
	if (arg->mode==FREG)
		return (arg->reg);

	/* look in the registers */
	for (p = &cache[reg_fp0]; p<=&cache[reg_fp7]; p++) {
		if (p->op && p->mode1!=UNKNOWN
		    && p->subop==size
		    && equarg(&p->op2, arg))
			return (p-cache);	/* return reg number */
	}

	/* look in memory for one that contains it */
	for (p = &cache[HREG+1]; p<=cache_end; p++) {
		if (p->op && p->mode1!=UNKNOWN
		&& p->mode2==FREG
		&& p->subop==size
		&& equarg(&p->op1, arg))
			return p->reg2;
	}

	return (-1);		/* couldn't find it */
}
#endif

#ifdef DRAGON
/*
 * Return dragon register that contains arg, -1 otherwise.
 */
find_fpreg(arg,size)
register argument *arg;
register short size;
{
	register node *p;

	if (arg==NULL)
		internal_error("find_fpreg: null argument");

	/* is it a register already? */
	if (arg->mode==FPREG)
		return (arg->reg);

	/* look in the registers */
	for (p = &cache[reg_fpa0]; p<=&cache[reg_fpa15]; p++) {
		if (p->op && p->mode1!=UNKNOWN
		    && p->subop==size
		    && equarg(&p->op2, arg))
			return (p-cache);	/* return reg number */
	}

	/* look in memory for one that contains it */
	for (p = &cache[HREG+1]; p<=cache_end; p++) {
		if (p->op && p->mode1!=UNKNOWN
		&& p->mode2==FPREG
		&& p->subop==size
		&& equarg(&p->op1, arg))
			return p->reg2;
	}

	return (-1);		/* couldn't find it */
}
#endif DRAGON

/*
 * Return AREG that contains address of arg (from a prev. lea), -1 otherwise.
 */
find_areg(arg)
register argument *arg;
{
	register i;

	if (arg==NULL)
		internal_error("find_areg: null argument");

	for (i=0; i<8; i++)
	{
		if (aregs[i].mode != UNKNOWN && equarg(arg, &aregs[i]))
			return (i+reg_a0);
	}
	return(-1);
}

/*
 * Look through register contents table, clearing entries based on value
 * of register 'reg'.
 */
reg_changed(reg)
register char reg;
{

#ifdef DEBUG
	if (isF(reg)) bugout("4: reg_changed(fp%d)", reg - reg_fp0);
	else bugout("4: reg_changed(%c%d)", isA(reg) ? 'a' : 'd', reg&07);
#endif DEBUG
					
	/* fix areg cache */
    {

	register argument *arg;
	register argument *arg7;

	arg = &aregs[0];
	arg7 = &aregs[7];
	if (isA(reg)) aregs[reg - reg_a0].mode = UNKNOWN;
	do 
	{ 
		if (arg->mode!=UNKNOWN && (arg->reg==reg || arg->index==reg))
			arg->mode = UNKNOWN ; 
		arg++; 
	}
	while (arg <= arg7);
    }


    {
	register node *p;
	register node *q;

	/* Is it in the condition code? */
		if (ccloc.reg==reg || ccloc.index==reg)
			ccloc.mode = UNKNOWN;
		
	q = cache_end;
	for (p=cache; p<=q; p++) {

		if (p->op == false) continue;	/* only look at valid entries */
		/* no need to check for the mode : assume reg1 and index1
		 * are either real or -1 */
		if (p->reg1==reg || p->reg2==reg || p->index1==reg || p->index2==reg)
			p->op = false;
	}
    }
}



/*
 * Update the cache assuming operand passed as argument
 * was nuked by instruction.
 */
dest(arg,typ)
register argument *arg;
int                typ;
{
	register char mode;
	register node *p;
	register node *q;

#ifdef DEBUG
	if (debug4) {
		printf("** dest(");
		print_arg(arg);
		printf(")\n");
	}
#endif DEBUG

	/*
	 * If we have a register destination, clear that register
	 * and all things that use that register (i.e., 3(a0)).
	 */
	mode=arg->mode;
	if (isREGMODE(mode))
	{
		reg_changed(arg->reg);

		if (mode == FREG) {
		    fpsize[arg->reg] = typ;
		}

		return;
	}

	if (mode==DPAIR)
		{
			reg_changed(arg->reg);
			reg_changed(arg->index);
			return;
		}

	/*
	 * If anybody claims to have a copy of our value,
	 * call them a liar.
	 */
	if (ccloc.mode==mode && 
	    overlapargs(arg, typ,  &ccloc, cctyp)) /* Is it in the cc? */
		ccloc.mode = UNKNOWN;

	q = cache_end;
	for (p=cache; p<=q; p++)			/* Look thru cache */
		if (p->op && mode==p->mode2 && 
		    overlapargs(arg, typ, &p->op2, p->subop))
							/* Is it here? */
			p->op = false;			/* Trash it if so. */
			
	switch (mode) {
	case ABS_W:
	case ABS_L:
		/*
		 * If we have a memory destination, clear it
		 */
		for (p = &cache[HREG+1]; p<=q; p++)		/* Look thru cache */
			if (p->op && mode==p->mode1 && 
			    overlapargs(arg, typ, &p->op1, p->subop))
							/* Is it here? */
				p->op = false;		/* Trash it if so. */
		break;

	/*
	 * If there is any indirection, then all info is bad except
	 * registers that hold constants or other registers.
	 */

	case IND:
	case INC:
	case DEC:
	case ADISP:
	case PDISP:
	case AINDEX:
	case AINDBASE:
	case PINDEX:
	case PINDBASE:
#ifdef PREPOST
	case MEMPRE:
	case MEMPOST:
	case PCPRE:
	case PCPOST:
#endif PREPOST
		/* Clear the condition code */
		ccloc.mode = UNKNOWN;

		/* Clear all non-registers */
		for (p = &cache[HREG]; p<=q; p++) {

			if (p->op==false) continue;	/* only look at valid entries */

#ifdef DEBUG
			if (debug4) {
				printf("** in dest, cache: ");
				print_arg(&p->op1);
				printf(" = ");
				print_arg(&p->op2);
				printf("\n");
			}
#endif DEBUG


			if (mode==ADISP && p->mode1==ADISP
			&& arg->reg==p->reg1
			&& arg->type==INTVAL && p->type1==INTVAL
			&& !overlapi(arg->addr, typ, p->addr1, p->subop)
			&& (p->mode2==ADIR || p->mode2==DDIR 
			    || p->mode2==FREG)) {
				bugout("4: left alone");
				/* leave it alone */;
			} else {
				p->op = false;		/* Trash it if so. */
			}
		}

		/* Check the registers */
		q = &cache[HREG];
		for (p = &cache[0]; p<=q; p++) {
			if (p->op==false) continue;	/* only look at valid entries */
			if (p->mode2==DDIR || p->mode2==ADIR
			|| p->mode2==FREG
			|| p->mode2==IMMEDIATE)
				/* leave it alone */;
			else if (mode==ADISP && p->mode2==ADISP
			&& arg->reg==p->reg2
			&& arg->type==INTVAL && p->type2==INTVAL
			&& !overlapi(arg->addr, typ, p->addr2, p->subop))
				/* leave it alone */;
			else if (fort && p>=&cache[reg_a2] 
				 && p<=&cache[reg_a5])
				/* leave it alone */;
			else
				p->op = false;		/* Trash it if so. */
		}
		break;
	}
}


#ifdef DEBUG
/*
 * Cleanse a cache entry.
 */
cleanse_cache_entry(p)
register node *p;
{
	cleanse(&p->op1);
	cleanse(&p->op2);
}
#endif DEBUG


/*
 * Is it ok to put this argument in the cache?
 * It's not if memory is suspect (possible I/O locations)
 * and this is a memory location.
 */
int
cacheable(arg)
register argument *arg;
{
#ifdef VOLATILE
	return(!isvolatile(arg));
#else
	if (memory_is_ok			/* normal memory, anything ok */
	|| arg->mode==DDIR || arg->mode==ADIR	/* registers always ok */
	|| arg->mode==IMMEDIATE			/* constants are ok */
	|| arg->mode==ADISP && arg->reg==reg_a6) /* stack frame is ok */
		return true;			/* ok, put it in the cache */
	else
		return false;			/* don't put it in the cache */
#endif
}

#ifdef VOLATILE

/*
 * volatile items are kept in a linked list with header
 * global items are at the front; a pointer is kept to the last global
 */

static node vlist;		/* global volatiles (#8) */
static node *lvlist = &vlist;	/* local volatiles (#7) */

/* for C: #n with no arguments means everything is volatile */
static unsigned int kludge8 = false, kludge7 = false;

append_to_vlist (p, local_flag)
register node *p;
int local_flag;
{
	register node *q;
	register node *head;

	head = local_flag ? lvlist : &vlist;
	q = head->forw;
	head->forw = p;
	p->forw = q;
	p->back = head;
	if (q != NULL)
		q->back = p;
	/* if this was first non-local, adjust local pointer */
	if (!local_flag && lvlist == &vlist)
		lvlist = p;
	/* set kludge flag if C wimped out on us */
	if (p->mode1 == UNKNOWN)
	{
		if (local_flag)
			kludge7 = true;
		else
			kludge8 = true;
	}
}

int is_on_vlist (arg)
register argument *arg;
{
	register node *p;

	/* search vlist for arg, or something related to it */
	for (p = vlist.forw; p != NULL; FORW(p))
	{
		/* is kludge in effect? */
		if (kludge7 || kludge8 || !(memory_is_ok))
		{
			/* memory is volatile */
			switch (arg->mode)
			{
				case ADIR:
				case DDIR:
				case IMMEDIATE:
					return(false);
				default:
					return(true);
			}
		}

		/* do they match exactly? */
		if (equarg(arg,&p->op1))
			return(true);

		/* is it close enough for jazz? */
		switch (p->mode1)
		{
			/* if %a3 points to volatile
			   anything accessed off %a3 is also volatile */
			case IND:
				switch (arg->mode)
				{
					case IND:
					case INC:
					case DEC:
					case ADISP:
					case AINDEX:
					case AINDBASE:
						if (arg->reg == p->reg1)
							return(true);
				}
				break;

			/* if the volatile item is a stack offset
			   anything within (size) bytes is also volatile
			   anything indexed off that offset is also volatile */
			case ADISP:
				switch (arg->mode)
				{
					case AINDBASE:
					case ADISP:
						if (arg->reg == p->reg1
						    && arg->type == INTVAL
						    && arg->addr >= p->addr1
						    && arg->addr <
						       p->addr1 + p->addr2)
							return(true);
				}
				break;

			/* if the volatile item is a named global
			   then anything using that name is volatile */
			case ABS_L:
				switch (arg->mode)
				{
					case AINDBASE:
					case ABS_L:
						if (arg->type == STRING
						    && !strncmp(arg->string,p->string1,strlen(p->string1)))
						{
							/* looks good so far */
							switch (arg->string[strlen(p->string1)])
							{
								case '\0':
								case '+':
								case '-':
									return(true);
							}
						}
				}
				break;
		}
	}
	return(false);
}

mark_volatile ()
/* marks appropriate operands in function as volatile */
{
	register node *p;
	register argument *arg;

	for (p = first.forw; p != NULL; FORW(p))
	{
		switch (p->op)
		{
			case LABEL:
			case DLABEL:
			case JMP:
				/* don't track aregs any more */
				for (arg = &aregs[7]; arg >= &aregs[2]; --arg)
					arg->mode = UNKNOWN;
				/* fall through */
			case JSR:
				/* stop tracking %a0 and %a1 only */
				aregs[1].mode = UNKNOWN;
				aregs[0].mode = UNKNOWN;
				continue;
		}
		for (arg = &p->op1; arg <= &p->op2; ++arg)
		{
			if (arg->mode == UNKNOWN)
				continue;
			if (is_on_vlist(arg))
				arg->attributes |= V_ATTR;
			/* if there was a previous "lea _v,%a0"
			   then indirect modes involving %a0 are volatile */
			switch (arg->mode)
			{
				case IND:
				case INC:
				case DEC:
				case ADISP:
				case AINDEX:
				case AINDBASE:
					if (aregs[arg->reg-reg_a0].mode == IND)
						arg->attributes |= V_ATTR;
			}
		}
		/* if "lea _v,%a0" then "(%a0)" and its ilk are now volatile */
		if ((p->op == LEA || p->op == MOVE) && isvolatile(&p->op1))
			aregs[p->reg2-reg_a0].mode = IND;
#ifdef DEBUG
		if (debug4 && (isvolatile(&p->op1) || isvolatile(&p->op2)))
		{
			extern char *out_op[];

			printf("%s",out_op[p->op]);
			print_subop(p->subop);
			printf("\t");
			printargs(p);
			printf("\n");
		}
#endif
	}
#ifdef DEBUG
	if (debug4)
	{
		printf("\tGlobal volatiles:\n");
		for (p = &vlist; p != lvlist; FORW(p))
		{
			printargs(p->forw);
			printf("\n");
		}
		printf("\tLocal volatiles:\n");
		for ( ; p->forw != NULL; FORW(p))
		{
			printargs(p->forw);
			printf("\n");
		}
	}
#endif
	/* we're done with local volatiles now */
	release_list(lvlist->forw);
	kludge7 = false;
	/* cleanse areg cache */
	for (arg = &aregs[0]; arg <= &aregs[7]; ++arg)
		arg->mode = UNKNOWN;
}

#endif

/*
 * Update the cache for a movem inst. to registers.
 */
dest_movem_arg(mask, dec)
register int mask;
{
	register int r;
	register int nmask;
	argument narg;

	if (dec)
	{
		/* reverse mask */
		nmask = 0;
		for (r=0; r<=15; r++, mask >>= 1) 
		{
			nmask ++;
			nmask <<= 1;
		}
		mask = nmask;
	}
	for (r=0; r<=15; r++, mask >>= 1) 
	{
		if (mask&01) 				/* if bit set: */
		{
			narg.mode= (r <= reg_d7) ? DDIR:ADIR ;
			narg.reg = r;
			dest(&narg, LONG);
		}
	}
}

/*
 * Do arguments 'a' and 'b' overlap?
 */
overlapargs(a, asize, b, bsize)
register argument *a;
register int       asize;
register argument *b;
register int       bsize;
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
			if (a->odlabno != b->odlabno) 
			   return false;
		case INTVAL:
			if (!overlapi(a->odaddr, asize, b->odaddr, bsize))
			   return false;
		case STRING:
			if (!overlaps(a->odstring, asize, b->odstring, bsize)) 
			   return false;
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
		if (a->type != b->type) {
			if (((a->type == INTLAB) || (a->type == STRING)) &&
			    ((b->type == INTLAB) || (b->type == STRING))) {

				char buf[80];

				if (a->type == INTLAB) {
					sprintf(buf,"L%d",a->addr);
			   		return(
					   overlaps(
					      buf,
					      asize, 
					      b->string, 
					      bsize
					    )
					 );
				}
				else {
					sprintf(buf,"L%d",b->addr);
			   		return(
					   overlaps(
					      a->string, 
					      asize, 
					      buf,
					      bsize
					    )
					 );
				}
			}
			return false;
		}

		switch (a->type) {

		case INTLAB:
			return (a->addr == b->addr);
		case INTVAL:
#ifdef PREPOST
			if ((a->mode == MEMPRE) || (a->mode == MEMPOST))
			   return(a->addr == b->addr);
			else
#endif
			   return(overlapi(a->addr, asize, b->addr, bsize));
		case STRING:
#ifdef PREPOST
			if ((a->mode == MEMPRE) || (a->mode == MEMPOST))
			   return(equstr(a->string, b->string));
			else
#endif
			   return(overlaps(a->string, asize, b->string, bsize));
		default:
			/* God only knows what it might be */
			return false;
		};

	default:
		return false;
	}
}

/*
 * Check for overlap (where the offset is an integer)
 */
int overlapi(offset1, typ1, offset2, typ2)
int offset1;
int typ1;
int offset2;
int typ2;
{
   int size1;
   int size2;

   size1 = typetobytes(typ1);
   size2 = typetobytes(typ2);

   return((offset1 == offset2) 
	  ? 1
	  : (offset1 > offset2) 
            ? (offset2 + size2 > offset1) 
	      ? 1 
	      : 0 
            : (offset1 + size1 > offset2) 
	      ? 1 
	      : 0
	 );

}

/*
 * Check for overlap where the offset is a string of the
 * form <label> or <label>+<hex number>
 */
overlaps(offset1, typ1, offset2, typ2)
char *offset1;
int   typ1;
char *offset2;
int   typ2;
{
   char *p1;
   char *p2;
   int   len1;
   int   len2;
   int   off1;
   int   off2;
   char *strchr();

   p1 = strchr(offset1, '+');
   if (p1) {
      len1 = p1 - offset1;
   }
   else {
      len1 = strlen(offset1);
   }

   p2 = strchr(offset2, '+');
   if (p2) {
      len2 = p2 - offset2;
   }
   else {
      len2 = strlen(offset2);
   }

   if (len1 != len2) {
      return(0); /* can't be the same label */
   }

   if (strncmp(offset1, offset2, len1) != 0) {
      return(0); /* can't be the same label */
   }

   if (p1) {
      if ((p1[1] != '0') || ((p1[2] != 'x') && (p1[2] != 'X'))) {
	 return(1);
      }
      sscanf(p1 + 3, "%x", &off1);
   }
   else {
      off1 = 0;
   }

   if (p2) {
      if ((p2[1] != '0') || ((p2[2] != 'x') && (p2[2] != 'X'))) {
	 return(1);
      }
      sscanf(p2 + 3, "%x", &off2);
   }
   else {
      off2 = 0;
   }

   return(overlapi(off1, typ1, off2, typ2));
}

/*
 * Convert a type to its size in bytes
 */
typetobytes(type)
int type;
{
   switch (type) {
   case BYTE:
      return(1);
      break;

   case WORD:
      return(2);
      break;

   case LONG:
   case SINGLE:
   case UNSIZED:
      return(4);
      break;

   case DOUBLE:
      return(8);
      break;

   case EXTENDED:
      return(12);
      break;
   }
}
