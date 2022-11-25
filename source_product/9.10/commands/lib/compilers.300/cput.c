/*SCCS  cput.c     REV(64.1);       DATE(04/03/92 14:22:00) */
/* KLEENIX_ID @(#)cput.c	64.1 91/08/28 */

#ifdef BRIDGE
#   include "bridge.I"
#else  /*BRIDGE*/

#include <stdio.h>
#include "opcodes.h"
#include "mfile1"
#include "messages.h"

extern FILE *outfile;
OFFSZ p1maxoff;		/* Split-C version of maxoff (size to be reserved by 
			 *  the link instruction */
#endif /*BRIDGE*/

#define FOUR 4
#define P2BUFFMAX 128
long int p2buff[P2BUFFMAX];
long int *p2bufp		= &p2buff[0];
long int *p2bufend	= &p2buff[P2BUFFMAX];
long int maxlabloc;
long int flbracloc;	/* seek location of the FLBRAC record - for fixup */
extern flag volatile_flag;


p2pass(s)
char *s;
{
	p2triple(FTEXT, (strlen(s) + FOUR-1)/FOUR, 0);
	p2str(s); 
}



p2str(s)
register char *s;
{
union { long int word; char str[FOUR]; } u;
register int i;

i = 0;
u.word = 0;
while(*s)
	{
	u.str[i++] = *s++;
	if(i == FOUR)
		{
		p2word(u.word);
		u.word = 0;
		i = 0;
		}
	}
if(i > 0)
	p2word(u.word);
}



p2name(s)
register char *s;
{
register int i;

	/* arbitrary length names, terminated by a null,
	   padded to a full word (like p2str, except that null is included) */

#	define WL   sizeof(long int)
	union { long int word; char str[WL]; } w;
	
	w.word = 0;
	i = 0;
	while(w.str[i++] = *s++)
		if(i == WL)
			{
			p2word(w.word);
			w.word = 0;
			i = 0;
			}
	if(i > 0)
		p2word(w.word);

}




/* p2triple puts out 3 quantities onto the intermediate code buffer.
	order:
		first byte		  	  last byte
		    ------------------------------------------
		    |	type (word) | var (byte) | op (byte) |
		    ------------------------------------------
*/

p2triple(op, var, type)
int op, var, type;
{
register long word;

word = op | (var<<8);
word |= ( (long int) type) <<16;
p2word(word);
}






/* p2word puts out a LONG onto the intermediate code file and checks for a full
	buffer. 
*/
p2word(w)
long int w;
{

	fwrite((char *)&w, sizeof(long int), 1, outfile);
}


/* Write a FMAXLAB record and fill the data field with 0.  Record the
 *  location in the file of this data field so that fixmaxlab() can later
 *  go back and fill in the correct maximum assigned label number.
 */
putmaxlab()
{
	p2triple(FMAXLAB, 0, 0);
	maxlabloc=ftell(outfile);
	p2word(0);

#ifndef BRIDGE
	if (inlineflag) 
		/* unrelated to fmaxlab, but a convenient place to emit
		 * C1NAMEs for all possible FMONADIC routine names
		 */
		c1name_builtins();
#endif /*BRIDGE*/

}

fixmaxlab(ml)
long int ml;
{
long here;

	here=ftell(outfile);
	fseek(outfile, maxlabloc,0L);
	fwrite( &ml, sizeof(long int), 1, outfile);
	fseek(outfile,here,0L);

}


/* ASSUME_NO_PARAMETER_OVERLAP, ASSUME_PARM_TYPES_MATCHED,
 * ASSUME_NO_EXTERNAL_PARMS, and ASSUME_NO_SHARED_COMMON_PARMS
 *  are all safe assumptions with C's passing-by-value.
 * ASSUME_NO_SIDE_EFFECTS is off by default, and turned on with the
 * #pragma NO_SIDE_EFFECTS func1,func2..., which produces a NOEFFECTS node 
 * for each function in the list
 */
#define ASSUMEFLAGS 0xf000

/* Write the beginning-of-function FLBRAC record and a C1OPTIONS record.
 *   Write placeholder fields following the FLBRAC
 *   for the size of local stack space required.  fixlbrac will go back
 *   and write the correct value after it is known.
 */
putlbrac(fnc, nm)
int fnc;
char *nm;
{
	flbracloc=ftell(outfile);
	p2triple(FLBRAC, 0, fnc);
	p2word(0);	/* maxtfdreg */
	p2word(0);	/* c1 tmpoff */
	p2word(0);
	p2name(nm);	/* name of ftn */
	p2triple(C1OPTIONS, optlevel, ASSUMEFLAGS);
}


extern flag asm_esc;	/* in scan.c */
#if defined(NEWC0SUPPORT) || defined(BRIDGE)
extern flag staticftn;  /* in code.c */
#endif /* NEWC0SUPPORT || BRIDGE */

fixlbrac(fnc, localmax)
int fnc;
long localmax;
{
long here;
	
	here=ftell(outfile);
	fseek(outfile, flbracloc, 0L);
#if defined(NEWC0SUPPORT) || defined(BRIDGE)
	p2triple(FLBRAC, asm_esc | staticftn, fnc);
#else
	p2triple(FLBRAC, asm_esc, fnc);
#endif /* NEWC0SUPPORT || BRIDGE */
	asm_esc = 0;	/* reset asm flag for next function */
	p2word(MAXRFVAR | MAXRDVAR << 16);	/* maxtfdreg */
	p2word(localmax);	/* c1 tmpoff */
	p2word(localmax);
	fseek(outfile, here, 0L);
}


#ifdef BRIDGE
void putc1attr(attribs, regflag, class, typ, array_size)
   int         attribs, regflag, class, typ, array_size;

#else
int putc1attr(p, regflag)
struct symtab *p;
int regflag;

#endif
{
register int attr;
#ifndef BRIDGE
register int typ;
int class;
#endif /* BRIDGE */
int vol_flag = 1;

#ifndef BRIDGE
	typ = p->stype;
	class = p->sclass;
#endif /* BRIDGE */
	if (ISPTR(typ)) attr=C1PTR;
		else 	attr = 0;
	if (ISFTN(typ)) {vol_flag = 0; attr |= C1FUNC;}
	if (ISARY(typ)) attr |= C1ARY;
	if (regflag) {vol_flag = 0; attr |= C1REGISTER;}
	if (typ == UNIONTY)  attr |= C1EQUIV;
	if (class == PARAM)  {vol_flag = 0; attr |= C1FARG;}
	if (class==EXTERN || class==STATIC) attr |= C1EXTERN;
	if (class==AUTO) {vol_flag = 0;}
#ifdef BRIDGE
    	if ( (attribs & ANY_VOL_MASK) || (volatile_flag && vol_flag) ) 
#else
    	if ( (p->sattrib & ANY_VOL_MASK) || (volatile_flag && vol_flag) ) 
#endif /* BRIDGE */
		attr |= C1VOLATILE;
	attr |= (blevel % 256);
	p2word(attr);
	if (ISARY(typ)) 
		{
#ifndef BRIDGE
		/* This code patterned after pftn.c:tsize(), except don't
		 * abort on unknown size arrays, e.g. struct s a[]; before
		 * struct s is defined.
		 */
		int d;
		int mult;
		int array_size;

		d = p->dimoff;
		mult = dimtab[d++];
		array_size = mult * dimtab[p->sizoff]/SZCHAR;
		typ = DECREF(typ);

		while (typ&TMASK)
			{
			if ((typ&TMASK) == PTR) 
				{
				array_size = mult * SZPOINT/SZCHAR;
				break;
				}
			else if ((typ&TMASK) == ARY) 
				{
				mult *= (unsigned int) dimtab[d++];
				array_size = mult * dimtab[p->sizoff]/SZCHAR;
				}
			else if ((typ&TMASK) == FTN) 
				{
				array_size = 0;  /* can't take size of ftn */
				break;
				}

			typ = DECREF(typ);
			}
#endif /* BRIDGE */
		
		p2word(array_size>0 ? array_size : -1 );
		}
}

/* putsetregs - writes a SETREG node to indicate which registers are in use.
 * Normally, this is not needed since cpass1 does not allocate registers.
 * However, after a #pragma OPT_LEVEL 1, it reverts to the ccom allocation
 * scheme and we need to let codegen know which regs are available.  This
 * routine is called by bccode() at the beginning of a block after all
 * variables have been declared, and by the grammar at the end of a block
 * to release the register variables in that block.
 * The format is 	|%d0   -  %d7 %a0   -  %a7| VAL unused| SETREGS |
 *			| FP0       -        FP15 |   unused  | F0 - F7 |
 * A 1 in the bit position indicates that the register is in use.
 */
void 
putsetregs()
{
unsigned short regmask = 0x3fff;	/* %d0, %d1 always temp, avoid setting
					 * sign bit for shifting below
					 */
unsigned short aregmask = 0x3f;

	regmask = regmask >> ((regvar&0xff) - 1);  /* -2 for d0 & d1 uncovered
						  * by the initial mask, +1
						  * because regvar has next
						  * available temp reg, e.g.,
						  * 7 will shift off all 1's
						  */
	regmask = regmask & 0xff00;
	aregmask = (aregmask >> ((regvar >> 8) - 1) & 0xfc);
	regmask |= aregmask;

	p2triple(SETREGS, 0, regmask);
	p2word(0);		/* cpass1 doesn't assign dragon or 881 regs */
}


#ifndef BRIDGE

/* This is an alternative to the pass2 version in local2.c.  It is called
 *  from code.c:bccode() and its purpose is to keep track of the maximum 
 *  stack space and registers used in case OPT_LEVEL 1 has been selected 
 *  (which implies one-pass style register and stack allocation).
 */
p2bbeg( aoff, myreg, myfdreg ) 
     {
	static int myftn = -1;
	if( myftn != ftnno ){ /* beginning of function */
		p1maxoff = aoff;
		myftn = ftnno;
		}
	else {
		if( aoff > p1maxoff ) p1maxoff = aoff;
		/* p1maxoff at end of ftn is max of autos and temps over all blocks */
		}
	if (optlevel < 2) putsetregs();
	}




/* putinit handles initialization of longs.  We get here via doinit()--cinit().
 * doinit() has handled all AUTO and REGISTER initialization via ecomp(), 
 * floating point via fincode(), and all sz<SZINT via incode().  ccom handles
 * the remaining cases in cinit by calling ecode() on the p tree, which consists
 * of INIT-ICON or INIT-NAME (or else error message "incorrect initialization").
 */

putinit(p, sz)
NODE *p;
int sz;
{
NODE *pleft;
struct symtab *sidptr;
char buf[256];
char buf2[32];
register TWORD typ;

	pleft = p->in.left;
	typ = pleft->in.type;
	if (typ!=INT && typ!=LONG && typ!=UNSIGNED && typ!=ULONG && !ISPTR(typ))
		{
		/* "incorrect initialization" - as in reader.c */
		UERROR( "incorrect initialization" );
		return;
		}

	if (pleft->tn.rval == NONAME) {
		sprntf(buf, "\tlong\t%d", pleft->tn.lval);
		p2pass(buf);
		}
	else {
		sidptr = pleft->nn.sym;
		if (pleft->nn.sym != NULL) {
			sprntf(buf, "\tlong\t%s", exname(sidptr->sname));
			if (pleft->tn.lval) {
				sprntf(buf2, "+0x%x", pleft->tn.lval);
				strcat(buf, buf2);
				}
			p2pass(buf);
			}
		else {
			if (pleft->tn.lval )
			   sprntf(buf, "\tlong\tL%d+0x%x",-pleft->tn.rval,pleft->tn.lval);
			else 
			   sprntf(buf, "\tlong\tL%d", -pleft->tn.rval);
			p2pass(buf);
			}
		}

}
#endif /*BRIDGE*/

void puttree();




/* buriedsc() is used to determine if an expression tree is acceptable to
 * putcbranch().  It walks the tree, looking for a short circuit operator
 * (&&, ||, ?:, or COMOP) whose parent is not also a short circuit.  It is
 * needed to differentiate control-flow-altering sequences like "if (a&&b)"
 * from logical-valued expressions like "x=a&&b".  buriedsc() returns 1 if
 * there are isolated short circuits; 0 otherwise.
 *
 * Parent is 1 if the parent of p is a short circuit
 */

buriedsc(p, parent)
NODE *p;
int parent;
{
register short op, ty;

	op = p->in.op;
	ty = optype(p->in.op);

	if (!parent)
	    switch (op) {
		case ANDAND:
		case OROR:
		case QUEST:
		case COMOP:
			return(1);
		}
	else
	    switch (p->in.op) {
		case ANDAND:
		case OROR:
		case QUEST:
		case COMOP:
		case COLON:
		        if (buriedsc(p->in.left, 1)) return(1);
		        if (buriedsc(p->in.right, 1)) return(1);
			return(0);
		}

	if ( ty != LTYPE ) 
	    if (buriedsc(p->in.left, 0)) return(1);
	if ( ty == BITYPE )  return(buriedsc(p->in.right, 0));
	return(0);
}



/* putcbranch() writes a sequence of trees that perform the same logic as does
 *  the tree p; it converts trees with &&, ||, ?:, and COMOP into smaller
 *  trees without these short circuit operators.  This routine does not handle
 *  short circuit operators buried below non-short-circuit operators.  For
 *  example,  it handles "if (a&&b)...", but not "if (a==(b&&c))...".
 *  The helper routine, buriedsc, detects the latter condition.  If detected,
 *  rewrite_sc() will convert the tree into a sequence of trees: 
 *	if (!b) goto L1
 *      if (!c) goto L1
 *	   if (a==1) goto ...
 *	   goto next;
 *  L1:    if (a==0) goto ...
 *  next:
 */


putcbranch( p, truelabel, falselabel ) register NODE *p; {
	/* evaluate p for truth value, and branch to truelabel or falselabel
	/* accordingly: label <0 means fall through */

	register short o;
	register lab;
	int flab, tlab;
	flag ftp = ISFTP(p->in.type);

		o = p->in.op;
		lab = -1;

		switch( o ) {
	
		case ANDAND:
			lab = falselabel<0 ? GETLAB() : falselabel ;
			putcbranch( p->in.left, -1, lab );
			putcbranch( p->in.right, truelabel, falselabel );
			if( falselabel < 0 ) deflab( lab );
			return;
	
		case OROR:
			lab = truelabel<0 ? GETLAB() : truelabel;
			putcbranch( p->in.left, lab, -1 );
			putcbranch( p->in.right, truelabel, falselabel );
			if( truelabel < 0 ) deflab( lab );
			return;
	
		case NOT:
			putcbranch( p->in.left, falselabel, truelabel );
			break;
	
		case COMOP:
			puttree( p->in.left, 0 /*longform*/, 0);
			putcbranch( p->in.right, truelabel, falselabel );
			return;
	
		case QUEST:
			flab = falselabel<0 ? GETLAB() : falselabel;
			tlab = truelabel<0 ? GETLAB() : truelabel;
			putcbranch( p->in.left, -1, lab = GETLAB() );
			putcbranch( p->in.right->in.left, tlab, flab );
			deflab( lab );
			putcbranch( p->in.right->in.right, truelabel, falselabel );
			if( truelabel < 0 ) deflab( tlab);
			if( falselabel < 0 ) deflab( flab );
			return;
	
		case ICON:
			if( p->in.type != FLOAT && p->in.type != DOUBLE && p->in.type != LONGDOUBLE ){
	
				if( p->tn.lval || p->tn.rval != NONAME ){
					/* addresses of C objects are never 0 */
					if( truelabel>=0 ) branch( truelabel );
					}
				else if( falselabel>=0 ) branch( falselabel );
				return;
				}
			/* fall through to default with other strange constants */
	
		default:
			if( falselabel >= 0 )
			    {
			    /* Emit CBRANCH tree in postfix order */
			    prtree(p);		/* first the test condition */
			    puttyp(ICON, 0, INT, 0);	/* then the target */
			    p2word(falselabel);
			    puttyp(CBRANCH, 0, p->in.type, 0);
			    p2triple(FEXPR, 0, lineno);
			    if (truelabel >= 0) branch(truelabel);
			    }
			else	/* falselabel is "fall-through" */
			    {
/* opportunity exists here to reverse sense of relational (saves NOT node) */
			    prtree(p);
			    puttyp(NOT, 0, p->in.type, p->in.tattrib);
			    p2triple(ICON, 0, INT);	
			    p2word(truelabel);
			    puttyp(CBRANCH, 0, p->in.type, 0);
			    p2triple(FEXPR, 0, 0);
			    }
			return;
	
			}
	
}




/* Re-write A && B into 
 *	if !A goto L1; 
 *	if !B goto L1; 
 *	<tree with && replaced by 1>
 *	goto L2;
 *  L1: <tree with && replaced by 0>
 *  L2:
 *
 * Return 1, indicating that the tree has been written.
 */
putandand(p, parent, top, leftside, longform)
NODE *p, *parent, *top;
int leftside, longform;
{
register int lab1, lab2;
register NODE *icon, *cb;

	lab1 = GETLAB();
	icon = bcon(lab1, INT);

	/* if !A goto lab1 */
	if ( !buriedsc(p->in.left, 1) )
	    putcbranch(p->in.left, -1, lab1);
	else  /* complicated A expression */
	    {
	    cb = block(CBRANCH, p->in.left, icon, p->in.type, 0, p->fn.csiz);
	    /* We know from buriedsc test that there is a short circuit in A, 
	     *  so the tree will be written by rewrite_sc.  
	     */
	    (void) rewrite_sc(cb, NIL, cb, 0, longform, 0); 
	    cb->in.op = FREE;
	    }

	/* if !B goto lab1 */
	if ( !buriedsc(p->in.right, 1) )
	    putcbranch(p->in.right, -1, lab1);
	else  /* complicated B expression */
	    {
	    cb = block(CBRANCH, p->in.right, icon, p->in.type, 0, p->fn.csiz);
	    (void) rewrite_sc(cb, NIL, cb, 0, longform, 0); 
	    cb->in.op = FREE;
	    }

	if (parent)
	    {
	    /* <replace && with 1> */
	    icon->tn.lval = 1;
	    if (leftside)
		parent->in.left = icon;
	    else
		parent->in.right = icon;
	    /* let puttree check for sibling short circuits and write tree */
	    puttree(top, longform, 0);

	    /* goto lab2 */
	    branch( lab2 = GETLAB() );

	    /* <replace && with 0> */
	    icon->tn.lval = 0;
	    deflab(lab1);
	    puttree(top, longform, 0);
	    deflab(lab2);
	    if (leftside)
		parent->in.left = p;
	    else
		parent->in.right = p;
	    }
	/* else top of tree - no need to generate code, but define lab1 */
	else deflab(lab1);

	icon->in.op = FREE;
        return(1);


}


/* Re-write A || B into 
 *	if A goto lab1;
 *	if B goto lab1; 
 *	<tree with || replaced with 0>
 *	goto lab2;
 * lab1:<tree with || replaced with 1>
 * lab2:
 */
putoror(p, parent, top, leftside, longform)
NODE *p, *parent, *top;
int leftside, longform;
{
register int lab1, lab2;
register NODE *icon, *cb, *not;

	lab1 = GETLAB();
	icon = bcon(lab1, INT);

	/* if A goto lab1 */
	if ( !buriedsc(p->in.left, 1) )
	    putcbranch(p->in.left, lab1, -1);
	else  /* complicated A expression */
	    {
	    not = block(NOT, p->in.left, NIL, p->in.type, 0, p->fn.csiz);
	    cb = block(CBRANCH, not, icon, p->in.type, 0, p->fn.csiz);
	    /* We know from buriedsc test that there is a short circuit in A, 
	     *  so the tree will be written by rewrite_sc.  
	     */
	    (void) rewrite_sc(cb, NIL, cb, 0, longform, 0); 
	    cb->in.op = FREE;
	    not->in.op = FREE;
	    }

	/* if B goto lab1 */
	if ( !buriedsc(p->in.right, 1) )
	    putcbranch(p->in.right, lab1, -1);
	else  /* complicated B expression */
	    {
	    not = block(NOT, p->in.right, NIL, p->in.type, 0, p->fn.csiz);
	    cb = block(CBRANCH, not, icon, p->in.type, 0, p->fn.csiz);
	    (void) rewrite_sc(cb, NIL, cb, 0, longform, 0); 
	    cb->in.op = FREE;
	    not->in.op = FREE;
	    }

	icon->tn.lval = 0;
	if (parent)
	    {
	    /* <replace && with 0> */
	    icon->tn.lval = 0;
	    if (leftside)
		parent->in.left = icon;
	    else
		parent->in.right = icon;
	    /* let puttree check for sibling short circuits and write tree */
	    puttree(top, longform, 0);

	    /* goto lab2 */
	    branch( lab2 = GETLAB() );

	    /* <replace && with 1> */
	    icon->tn.lval = 1;
	    deflab(lab1);
	    puttree(top, longform, 0);
	    deflab(lab2);
	    if (leftside)
		parent->in.left = p;
	    else
		parent->in.right = p;
	    }
	/* else top of tree - no need to generate code, but define lab1 */
	else deflab(lab1);

	icon->in.op = FREE;
        return(1);


}



/* Create a temp variable of type matching p (used by putquest) */

NODE *
mkoreg(p)
NODE *p;
{
TWORD type;
unsigned int size, align;
NODE *oreg;
int c1attr;
#ifdef BRIDGE
int temp_off;
#endif /*BRIDGE*/

	type = p->in.type;
	/* if used for effect, return NULL.  
	 * structs used for value will have type PTR strty on the rhs
	 * do-nothing expressions like (a? struct1 : struct2); need this test
	 */
	if (type == VOID || type==STRTY) return(NULL);
#ifdef BRIDGE
	temp_off = u_alloc_temp(u_get_size(p));
#else  /*BRIDGE*/
	size = tsize(type, p->fn.cdim, p->fn.csiz, NULL, 0); /* SWFfc00726 fix */
	align = talign(type, p->fn.csiz, 0);
	upoff(size, align, &autooff);
	p1maxoff += size;  /* normally set at beginning of block by bccode */
#endif /*BRIDGE*/
	/* OP=C1OREG, VAL=%a6, REST=type, attr=0 */
	puttyp(C1OREG, 14, type, p->in.tattrib);
#ifdef BRIDGE
	p2word(temp_off);
#else /*BRIDGE*/
	p2word(-autooff/SZCHAR);
#endif /*BRIDGE*/
	c1attr = (type==STRTY) ? C1EQUIV : 0; 
	c1attr |= (ISPTR(type)) ? C1PTR : 0;
	c1attr |= (blevel % 256);
	p2word(c1attr);
	p2name("?:-tmp");
	oreg = block(OREG, NULL, NULL, type, p->fn.cdim, p->fn.csiz);
	oreg->tn.rval = 14;
#ifdef BRIDGE
	oreg->tn.lval = temp_off;
#else /*BRIDGE*/
	oreg->tn.lval = -autooff/SZCHAR;
#endif /*BRIDGE*/
	oreg->in.tattrib = p->in.tattrib;
	/* No c1tag here because rhs of struct types are PTR strty */

	return(oreg);
}


/* Build an ASSIGN or STASG tree as appropriate (for putquest) */

NODE *
mkassign(lhs, rhs, stflag)
NODE *lhs, *rhs;
int stflag;
{
NODE *asg, *stval, *a6, *pntr, *plus, *offset, *stasg;
TWORD type, c1attr;
unsigned int size, align;
#ifdef BRIDGE
int temp_off;
#endif /*BRIDGE*/

	if (!lhs) /* lhs==null => evaluate for effect only */
		return(rhs);

	type = rhs->in.type;
	if ( (type=DECREF(type)) != STRTY || !stflag )
	    asg = block(ASSIGN,lhs,rhs,rhs->in.type,rhs->fn.cdim,rhs->fn.csiz);
	else  
	    {
	    /* structs are hard.  The lhs (oreg) we constructed in mkoreg()
	     * will hold a pointer to the result area.  Here we need to
	     * allocate space to hold the struct result and emit trees
	     * to copy the struct contents into that area and its address
	     * into lhs. 
	     *
	     *			      asg=COMOP
	     *			    /           \
	     *		   stasg=STASG         pntr=ASSIGN
	     *		   /        \          /        \
	     *	     stval=OREG	    rhs     lhs      plus=PLUS
	     *					        /        \
	     *					      a6=REG   offset=ICON
	     */

	    /* Allocate the result area */
#ifdef BRIDGE
            temp_off = u_alloc_temp( (u_tree_type(rhs) == STRTY) ?
                                         u_get_stsize(rhs) :
                                         u_get_size(rhs));

#else /*BRIDGE*/
	    size = tsize(type, rhs->fn.cdim, rhs->fn.csiz, NULL, 0); /* SWFfc00726 fix */
	    align = talign(type, rhs->fn.csiz, 0);
	    upoff(size, align, &autooff);
	    p1maxoff += size;  
#endif /*BRIDGE*/
	    c1attr = DECREF(rhs->in.tattrib);
	    puttyp(C1OREG, 14, type, c1attr);
#ifdef BRIDGE
	    p2word(temp_off);
#else /*BRIDGE*/
	    p2word(-autooff/SZCHAR);
#endif /*BRIDGE*/
	    p2word(C1EQUIV | (blevel%256) );
	    p2name("?:-val");

	    /* Build the tree to assign struct value to result area */
	    stval = block(OREG, NULL, NULL, type, rhs->fn.cdim, rhs->fn.csiz);
	    stval->tn.rval = 14;
#ifdef BRIDGE
	    stval->tn.lval = temp_off;
#else /*BRIDGE*/
	    stval->tn.lval = -autooff/SZCHAR;  /* autooff used below */
#endif /*BRIDGE*/
	    stval->in.tattrib = c1attr;
	    stval->in.c1tag = rhs->in.c1tag;
	    if (stval->in.c1tag) stval->in.fixtag = 1;
	    stasg=block(STASG,stval,rhs,rhs->in.type,rhs->fn.cdim,rhs->fn.csiz);

	    /* Build the tree to assign result address to lhs */
	    a6 = block(REG, NULL, NULL, INCREF(STRTY), 0, 0);
	    a6->tn.rval = 14;
	    a6->tn.lval = 0;
	    offset = block(ICON, NULL, NULL, INT, 0, 0);
#ifdef BRIDGE
	    offset->tn.lval = temp_off;
#else /*BRIDGE*/
	    offset->tn.lval = -autooff/SZCHAR;
#endif /*BRIDGE*/
	    offset->tn.rval = NONAME;
	    plus = block(PLUS, a6, offset, INCREF(STRTY), 0, 0);
	    pntr=block(ASSIGN,lhs,plus,INCREF(STRTY),lhs->fn.cdim,lhs->fn.csiz);

	    /* Glue the trees together */
	    asg = block(COMOP,stasg,pntr,0,0,0);
	    }

	return(asg);
}



/* freeasg() - a companion to mkasg() that frees the assignment tree */

void
freeasg(asg)
NODE *asg;
{
	if (asg->in.op == ASSIGN)
		asg->in.op = FREE;  
	else /* struct case */
		{
		tfree(asg->in.right->in.right);	  	/* REG + ICON subtree */
		asg->in.right->in.op  = FREE;		/* ASSIGN */
		asg->in.left->in.left->in.op = FREE;  	/* stval OREG */
		asg->in.left->in.op = FREE;  		/* STASG */
		asg->in.op = FREE;			/* COMOP */
		}
}




/* reassign() - another mate for mkassign() to splice in an alternative rhs */
void reassign(asg, newrhs)
NODE *asg, *newrhs;
{

	if (asg->in.op == ASSIGN)
		asg->in.right = newrhs;
	else
		asg->in.left->in.right = newrhs;   /* right child of STASG */
}




/* Controls how many ?: will be expanded before reverting to temp var form */
#define BEGIN_TEMP_FORM 2

/* Translate (a ? b : c) into
 *	  if !a goto lab1
 *	  <replace ? with b>
 *	  goto lab2
 *  lab1: <replace ? with c>
 *  lab2:
 */
putquest(p, parent, top, leftside, longform, stval)
NODE *p, *parent, *top;
int leftside, longform, stval;
{
register NODE *cb, *icon;
register int lab1, lab2;

	lab1 = GETLAB();
	lab2 = GETLAB();

	/* if !a goto lab1 */
	if ( !buriedsc(p->in.left, 1) )
	    putcbranch(p->in.left, -1, lab1);
	else  /* complicated A expression */
	    {
	    icon = bcon(lab1, INT);
	    cb = block(CBRANCH, p->in.left, icon, p->in.type, 0, p->fn.csiz);
	    /* We know from buriedsc test that there is a short circuit in A, 
	     *  so the tree will be written here.  
	     */
	    puttree(cb, longform, 0);	/* left oprnd of '?' should be scalar */
	    cb->in.op = FREE;
	    icon->in.op = FREE;
	    }

	/* Now handle the true and false clauses.
	 * For the first encounters of ?:, successively replace QUEST with
	 * the true and false subtrees.  This produces faster code than
	 * assigning to a temp, but leads to exponential growth for
	 * multiple ?:'s in an expression tree (even when not nested;
	 * for example, multiple ?: function arguments).
	 *
	 * First the longform with temp variables:
	 */

	if (longform++ >= BEGIN_TEMP_FORM)
		{
		NODE *asg, *tmp;
		tmp = mkoreg(p->in.right);
		asg = mkassign(tmp, p->in.right->in.left, stval);
		if (!rewrite_sc(asg, NULL, asg /* top */, 0, longform, stval) )
			puttree(asg, longform, stval);
		branch(lab2);
		deflab(lab1);
		if (tmp)
			reassign(asg, p->in.right->in.right);
		else
			asg = p->in.right->in.right;	/* for effect */
		if (!rewrite_sc(asg, NULL, asg /* top */, 0, longform, stval) )
			puttree(asg, longform, stval);
		deflab(lab2);
		if (parent)
			{
			if (leftside)
				parent->in.left = tmp;
			else
				parent->in.right = tmp;
			puttree(top, longform, stval);
			}
		/* else evaluated for side effects only */
				
		if (tmp)
			{
			tmp->in.op = FREE;
			freeasg(asg);
			}

		/* put the original tree back in place so that it will
		 * be expanded during each alternative of the previous
		 * short form expansions.
		 */
		if (parent)
			if (leftside)
				parent->in.left = p;
			else
				parent->in.right = p;
		}

	else /* short form: <replace ? with b, then with c> */
	if (parent)
		{
		if (leftside)
			{
			parent->in.left = p->in.right->in.left;
			puttree(top, longform, stval);
			branch(lab2);
			deflab(lab1);
			parent->in.left = p->in.right->in.right;
			puttree(top, longform, stval);
			deflab(lab2);
			parent->in.left = p;
			}
		else
			{
			parent->in.right = p->in.right->in.left;
			puttree(top, longform, stval);
			branch(lab2);
			deflab(lab1);
			parent->in.right = p->in.right->in.right;
			puttree(top, longform, stval);
			deflab(lab2);
			parent->in.right = p;
			}
		}
	else
		{ /* QUEST at top of tree; emit trees for effect only */
		puttree(p->in.right->in.left, longform, 0);
		branch(lab2);
		deflab(lab1);
		puttree(p->in.right->in.right, longform, 0);
		deflab(lab2);
		}

	return(1);

}





/* A COMMA sequencing node has been discovered.  Write out the left subtree
 *  as a standalone tree preceeding the current one, then cleanse the right
 *  subtree.
 */
putcomma(p, parent, top, leftside, stval)
NODE *p, *parent, *top;
int leftside, stval;
{

	puttree(p->in.left, 0, 0);

	if (parent)
		{
		if (leftside)  
			{
			parent->in.left = p->in.right;
			puttree(top, 0/*longform*/, stval);
			parent->in.left = p;
			}
		else
			{
			parent->in.right = p->in.right;
			puttree(top, 0/*longform*/, stval);
			parent->in.right = p;
			}
		}
	else	/* COMOP is at the top of the tree */
		{
		puttree(p->in.right, 0/*longform*/, 0/*stval*/);
		}

	return(1);

}




/* Descend the tree, recursively re-writing the short circuit 
 *   operators when they appear.  This routine returns 0 if the subtree at p 
 *   is free of short circuits; 1 if a short circuit somewhere in the subtree
 *   has been replaced, which implies that the tree has been written to the
 *   intermediate file.
 */

int
rewrite_sc(p, parent, top, leftside, longform, stval)
NODE *p, *parent, *top;
int leftside; 	/* Is this left or right side of parent? */
int longform;	/* When to switch to temp variable form of ?: */ 
int stval;	/* Child of STASG vs ASSIGN  */
{
register short op;

	op = p->in.op;

	switch (op) {
		case QUEST:   
		    return(putquest(p, parent, top, leftside, longform, stval));
			      
		case ANDAND:  
		    return(putandand(p, parent, top, leftside, longform)); 
			     
		case OROR:    
		    return(putoror(p, parent, top, leftside, longform)); 
			    
		case COMOP:   
		    return(putcomma(p, parent, top, leftside, stval));
			   
		case STASG:
		    stval = 1;
		    /* fall through */
		default:      
		    if (optype(op) == BITYPE) 
				  /* The bottom short circuit on the left side
				   * will have called puttree on the whole tree,
				   * so rewrite the right side only if the left
				   * side was initially clean.
				   */
			return( 
			     rewrite_sc(p->in.left,p,top,1,longform,stval) ||
			     rewrite_sc(p->in.right,p,top,0,longform,stval));

		      if (optype(op) != LTYPE) 
			  return( rewrite_sc(p->in.left, p, top, 1, 
							longform,stval) );
			
		      return(0);	/* leaf node */
		}
}
			      


/* Re-write short circuit operators and write a tree to the intermediate file.*/
void
puttree(p, longform, stval)
NODE* p;
int longform, stval;
{
	if (p->in.op == CBRANCH && !buriedsc(p->in.left, 1)) {
		putcbranch(p->in.left, -1, p->in.right->tn.lval);
		}
	else
	    {
	    if ( ! rewrite_sc(p, NIL, p, 0, longform, stval) )
	        {
		/* If short circuit code hasn't done it already, 
		 *  write the tree here 
		 */
		prtree(p);	/* Write it out */
		p2triple(FEXPR, 0, lineno);
		}
	    }
}


