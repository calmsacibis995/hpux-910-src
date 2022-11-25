/*	SCCS	REV(47.11.1.1);	DATE(90/11/19	15:59:34) */
/* KLEENIX_ID @(#)inline.c	47.11.1.1 90/07/18 */

#include "mfile2"
#include "c0.h"


long mindimoffset;	/* most negative offset for dimension info */
long minfnoffset;	/* most negative offset for fn result location */
long retoffset;		/* offset of result location (relocated) */
long argoffset;

long c0tempnum = 0;	/* c0 temp "variable" name index, keeps them unique */
C0TEMPREC *c0temps,*c0tempstail;	/* nested c0temp opd list */

short argc1data[MAXARGS];	/* c1data info for actual arguments */
# define C0STRUCT 0x100		/* flags c1data structref arg types */
# define C0ARRAY  0x200		/* flags c1data arrayref arg types */

NODE *evalarg();

NODE *assignargtotmp();

/*****************************************************************************/


int valid_return_type(type)
TWORD type;
{

#ifdef DEBUGGING
	if (verboseflag)
		fprintf(stderr,"checking return type (%d)\n",type);
#endif /* DEBUGGING */
	if ((type == UNDEF) || (type == LONGDOUBLE) ||
	    (type == STRTY) || (type == UNIONTY))
		return(0);
	return(1);
}

int valid_param_type(formal,actual)
TWORD formal, actual;
{
#ifdef DEBUGGING
	if (verboseflag)
		{
		fprintf(stderr,"checking param types, formal=%d, actual=%d)\n",
					formal,actual);
		}
#endif /* DEBUGGING */

	if ((formal == UNDEF) || (actual == UNDEF))
		return(0);
	if (!ftnflag && (ISPTR(formal)) && (ISPTR(actual)))
		return(1);
	if ((!ftnflag) && ((actual == INT) || (actual == UNSIGNED)) &&
	    ((formal == CHAR) || (formal == SHORT) ||
	     (formal == UCHAR) || (formal == USHORT) ||
	     (formal == INT) || (formal == UNSIGNED)))
		return(1);
	if (ftnflag && ISARY(formal) && (actual == DECREF(formal)))
		return(1);
	if ((formal == LONGDOUBLE) || (actual == LONGDOUBLE))
		return(0);
	if ((!ftnflag) && ((formal == STRTY) || (formal == UNIONTY)))
		return(0);
	return(formal == actual);
}


/*****************************************************************************/

flag c0match( t )	/* check for matching nested c0 temp offset */
  register NODE *t;
{
  register long o;
  register C0TEMPREC *tmp;
  NODE *l;

  o = t->in.arraybase;
  if ( t->in.op == PLUS
	&& (l=t->in.left)->in.op == UNARY MUL
	&& (l=l->in.left)->in.op == ICON && l->in.name != NULL )
      return(NO);		/* can't be a ref to shared common */
  if ( t->in.op == UNARY MUL )
    t = t->in.left;
  if ( t->in.op == PLUS )
    if ( t->in.left->in.op == NAME  )
      return(NO);		/* can't be a ref to shared common */
    else
      t = t->in.right;
  if ( t->in.op == NAME
	|| ( t->in.op == ICON && t->in.name != NULL ) )
      return(NO);		/* can't be a ref to common */

  for ( tmp = c0temps; tmp != NULL; tmp=tmp->next )
    {
    if ( o == tmp->baseoff )
      return(YES);
    }
  return(NO);
} /* c0match() */

/*****************************************************************************/

/* inlinecalls -- inline all calls in an expression tree and emit */
long inlinecalls(topnode, filename, lineno, parentpd)
NODE *topnode;		/* top of expression tree */
char *filename;		/* current filename */
long lineno;		/* source line number of expression tree */
pPD  parentpd;		/* pd of calling procedure */
{
	/* exprbaseoffset is the stack required for calling proc's local
	 * variables plus any temp result locations for inlined function
	 * calls.
	 */
    long exprbaseoffset = (globaloptim ? maxstack
				: currpd->stackuse>>3);	/* bits -> bytes */
    long i;
    NODE *pcall;		/* current call tree node */
    long stack;			/* max required stack after inlining 1 call */
#if 0
    pPD ippd;			/* pd of called procedure */
#endif
    register ExprCallRec *pc;	/* current call "slot" in call vector */
    long inlinedoneflag;	/* 1 iff a call was actually inlined */
    int attrs;
    char buf[60];


    /* assign result area offsets to all function calls */
    for (i = 0; i < nxtcall; i++)
	{
	pc = call + i;		/* set pointer into call info vector */
	if ((pc->nptr)->cn.rettype == VOID)
	    pc->retloc = 0;	/* SUBROUTINES don't need result areas */
	else
	    {
	    pc->retloc = (exprbaseoffset += 8);
	    puttype(C1OREG, AUTOREG, ((pc->nptr)->cn.rettype), 0);
	    putlong(-(pc->retloc));
	    attrs = 0x10;             /* New OREG is a C0 generated temp */
	    if (ISPTR((pc->nptr)->cn.rettype))
		attrs |= 0x0400;        /* Argument is a pointer */
	    if (ISFTN((pc->nptr)->cn.rettype))
		attrs |= 0x0100;
	    puttriple(2,attrs,0);
	    sprintf(buf,"passc0-ret%d", c0tempnum);
	    c0tempnum++;
	    putstring(buf);
	    }
	}

    argoffset = 0;

    /* inline each call in turn */
    inlinedoneflag = NO;
    for (i = 0; i < nxtcall; i++)
	{
    	inlinedoneflag = NO;
	pc = call + i;		/* set pointer to call info vector */
	ippd = pc->pd;		/* pd for called proc */

	/* check if function/subroutine usage correct -- issue warning and
	 * don't inline if incorrect
	 */
	if (topnode == pc->nptr)	/* SUBROUTINE w/o alt returns */
	    {
	    if (ftnflag && (ippd->typeproc == 1))	/* FUNCTION */
		{
		if (isforced(ippd->pnpoffset))
		    {
		    sprintf(warnbuff,
"FUNCTION \"%s\" referenced as SUBROUTINE in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		   	   ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		           filename, lineno,
		           ((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		    warn(warnbuff);
		    }
		continue;
		}
	    else if (ippd->hasaltrets)	/* called proc has alternate RETURNs */
		{
		if (isforced(ippd->pnpoffset))
		    {
		    sprintf(warnbuff,
"PROC \"%s\" referenced without alternate RETURNS in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		           ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		           filename, lineno,
		           ((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		    warn(warnbuff);
		    }
		continue;
		}
	    }
	else if ((topnode->in.op == FORCE)
	      && (topnode->in.left == pc->nptr))  /* SUBR w/ alt returns */
						/* or outermost FUNCTION call */
	    {
	    if ((ippd->typeproc == 0) && ! ippd->hasaltrets)  /* SUBR w/o alt */
		{
		if (isforced(ippd->pnpoffset))
		    {
		    sprintf(warnbuff,
"PROC \"%s\" referenced with alternate RETURNS in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		            ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		            filename, lineno,
		            ((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		    warn(warnbuff);
		    }
		continue;
		}
	    }
	else				/* FUNCTION call */
	    {
	    if (ippd->typeproc == 0)	/* SUBROUTINE */
		{
		if (isforced(ippd->pnpoffset))
		    {
		    sprintf(warnbuff,
"SUBROUTINE \"%s\" referenced as FUNCTION in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		            ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		            filename, lineno,
		            ((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		    warn(warnbuff);
		    }
		continue;
		}
	    }

	/*
	 * Check that # arguments and types agree with parameter definitions.
	 * Don't inline if disagreement.
	 */
	if ((ippd->typeproc == 1)		      /* FUNCTION */
	 && (ippd->rettype != pc->nptr->cn.rettype))  /* result type mismatch */
	    {
	    if (isforced(ippd->pnpoffset))
		{
	        sprintf(warnbuff,
"FUNCTION \"%s\" result types differ in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		        ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		        filename, lineno,
		        ((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    continue;
	    }
	if (pc->nptr->cn.nargs != ippd->nparms)	  /* formal/actual # disagree */
	    {
	    if (isforced(ippd->pnpoffset))
		{
	        sprintf(warnbuff,
"PROC \"%s\" formal/actual argument counts differ in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		        ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		        filename, lineno,
		        ((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    continue;
	    }

	/* Check formal/actual argument type compatibility */
	if (ippd->nparms > 0)	/* there are arguments */
	    {
	    register TWORD *parg;		/* argument type */
	    register TWORD *pparm;		/* formal param type */
	    register long count;		/* # left in current record */
	    register TWORD *eparm;		/* last formal parameter */
	    NODE* argnode = pc->nptr;		/* call info node containing
						 * actual argument types */

	    if ( globaloptim && ftnflag )
	      {
	      nargs = 0;
	      collectargtypes( pc->nptr->in.left->in.right );
	      }

	    pparm = &(ippd->parmtype[0]);	/* formals from pd record */
	    eparm = pparm + ippd->nparms - 1;
	    parg = &(argnode->cn.argtype[0]);	/* actuals from call info */
	    count = C0_TYPES_PER_NDU;		/* 8 actuals per records */
	    nargs = -1;			/* overall arg # */
	    while (1)		/* terminate when pparm > eparm */
	        {
	        nargs++;
				/* note, 0x100 flags structure types */
				/*       0x200 flags array types */
		if (!valid_param_type(*pparm,*parg))
						/* incompatible types */
		    {
		    if (isforced(ippd->pnpoffset))
			{
	                sprintf(warnbuff,
"PROC \"%s\" formal/actual argument types differ in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		       		((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		    		filename, lineno,
		    		((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		    	warn(warnbuff);
			}
		    goto nextcall;
		    }
	        else if (globaloptim && (ISARY(*pparm))
				&& !(argc1data[nargs] & C0ARRAY))
				/* formal array or struct, scalar actual */
		    {
		    if (isforced(ippd->pnpoffset))
			{
	                sprintf(warnbuff,
"PROC \"%s\" formal/actual arguments incompatible for global optimizing, in\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- no inline", 
		       		((pNameRec)(pnp+(ippd->pnpoffset)))->name,
		    		filename, lineno,
		    		((pNameRec)(pnp+(parentpd->pnpoffset)))->name);
		    	warn(warnbuff);
			}
		    goto nextcall;
		    }
	        else	/* compatible -- increment pointers */
		    {
		    pparm++;
		    if (pparm > eparm)
		        break;		/* terminate loop */
		    if (--count == 0)	/* need next argument record */
		        {
		        argnode = argnode->cn.right;
		        parg = argnode->cn.argtype;
			count = C0_TYPES_PER_NDU;
		        }
		    else
		        parg++;
		    }
	        }
	    }

	if (verboseflag)
	    fprintf(stderr,"c0: FILE \"%s\", LINE %d, PROC \"%s\" inlining call to \"%s\"\n",
		   filename, lineno,
		   ((pNameRec)(pnp+(parentpd->pnpoffset)))->name,
		   ((pNameRec)(pnp+(ippd->pnpoffset)))->name);
	if (ftnflag)
		nargs = 0;	/* set global vars for arg collection */
	else
		nargs = ippd->nparms;

	pcall = ((pc->nptr)->in.left);
	if (pcall->in.op == CALL)	/* there are args -- collect them
					 * into an argument vector */
	    collectargs(pcall->in.right, exprbaseoffset, lineno);

	/* During argument collection, some actuals need to be evaluated into
	 * temporary locations on the stack.  Add the amount of temporary
	 * space to the stack requirements.
	 */

	tempbase = exprbaseoffset + argoffset;
	cstackoffset = exprbaseoffset;
#if 0
	fprintf(stdout,"cstackoffset = %d, argoffset = %d\n",cstackoffset,argoffset);
#endif

	/* Inline the procedure call */
	
	doip(pc,parentpd,topnode);

	inlinedoneflag = YES;	/* We've inlined at least one proc this tree */

	/* Update called procedure stack requirements -- high water mark */
	stack = tempbase + (ippd->stackuse >> 3);
	if ( globaloptim )
	  exprbaseoffset = stack; /* afford c1 unique operands across calls */
	if (stack > maxstack)
	    maxstack = stack;	/* record max stack for calling proc */
nextcall:
	continue;
	}

    /* tell caller whether original tree must be emitted */
    if ((topnode->in.op == FC0CALL) && (inlinedoneflag == YES))
	return(0);	/* no */
    else
	return(1);	/* yes */
}  /* inlinecalls */

/*****************************************************************************/

/* collectargs -- collect args and update amount of additional stack space
 * used.
 */
collectargs(p, exprbaseoffset, lineno)
NODE *p;		/* expression tree nodes describing arguments */
long exprbaseoffset;	/* stack base offset for expression */
long lineno;		/* for emitted FEXPR nodes */
{
    long i;

    if (!ftnflag)
	{
    	if (p->in.op == CM)
		{
		collectargs(p->in.right, exprbaseoffset, lineno);
		collectargs(p->in.left, exprbaseoffset, lineno);
		}
	else
		{
		--nargs;
		argvf[nargs] = YES;             /* c0 temp opd needed */
		arg[nargs] = evalarg(p, exprbaseoffset, lineno, ippd->parmtype[nargs]);
		}
	}
    else
    {
    switch (p->in.op)
    {
    case CM:				/* COMMA operator -- joins args */
	collectargs(p->in.left, exprbaseoffset, lineno);
	collectargs(p->in.right, exprbaseoffset, lineno);
	break;

    case COMOP:				/* evaluating expr to local space */
	/* emit code to evaluate argument */
	puttree(p->in.left,0);
	puttriple(FEXPR,lineno,0);
	argvf[nargs] = NO;		/* c0 temp opd not needed */
	arg[nargs++] = p->in.right;	/* argument is then a temporary loc */
	break;

    case PLUS:				/* item on the local stack */
	if ((p->in.left->in.op == REG)
	  && (p->in.left->tn.rval == A6)
	  && ((p->in.right->in.op == ICON) && (p->in.right->in.name == NULL)))
	    {
	    argvf[nargs] = NO;		/* c0 temp opd not needed */
	    arg[nargs++] = p;	    /* should be local variable on stack */
	    }
	else
	    {
	    /* probably an array element -- evaluate argument to temporary
	     * location (OREG), then use temporary as the argument
	     */
	    argvf[nargs] = YES;		/* c0 temp opd needed */
	    arg[nargs++] = assignargtotmp(p, exprbaseoffset, lineno);
	    }
	break;
	
    case ICON:		/* static (COMMON) variable */
	argvf[nargs] = NO;		/* c0 temp opd not needed */
	arg[nargs++] = p;
	break;

    case OREG:		/* argument is parameter in current procedure */
	if (p->tn.rval == A6)
	    {
	    argvf[nargs] = NO;		/* c0 temp opd not needed */
	    arg[nargs++] = p;
	    }
	else		/* OREG, but not %a6 -- should never happen */
	    fatal("strange argument in collectargs() - 3");
	break;

    case UNARY MUL:	/* first element of a SHARED COMMON */
	if ((p->in.left->in.op == ICON)
	  && (p->in.left->tn.name != NULL))
	    {
	    argvf[nargs] = YES;		/* c0 temp opd needed */
	    arg[nargs++] = assignargtotmp(p, exprbaseoffset, lineno);
	    }
	else
	    fatal("strange argument in collectargs() - 5");
	break;

    default:	   /* only CM, COMOP, PLUS, ICON and OREG types acceptable */
	fatal("strange argument in collectargs() - 4");
    }
    }
    return;
}


/*****************************************************************************/

NODE * evalarg(p, exprbaseoffset, lineno, parmtype)
NODE *p;
long exprbaseoffset;
long lineno;
TWORD parmtype;
{
    register NODE *q;
    register NODE *l;
    NODE *tmp;
    long attrs;

    tmp = (p->in.op == FC0CALL) ? p->in.left : p;
    argoffset += BITOOR(typesz(tmp));
    l = getnode();
    l->in.op = OREG;
    l->in.type = tmp->in.type;
    l->tn.rval = A6;

    if ((parmtype == STRTY) || (parmtype == UNIONTY))
	{
        l->tn.lval = -(exprbaseoffset + argoffset);
	l->in.name = NULL;
	l->in.c1data.c1word = 0; /* for initial assignment only */
	l->in.arraybase = 0;
	q = getnode();
	q->in.op = STASG;
	q->in.type = tmp->in.type;
	q->in.left = l;
	/* need to make this ptr to p ??? */
	q->in.right = p;
	}
    else
	{
	if ((parmtype != CHAR) && (parmtype != UCHAR) &&
	    (parmtype != SHORT) && (parmtype != USHORT))
	    {
            l->tn.lval = - (exprbaseoffset + argoffset);
	    q = getnode();
	    }
	else /* Must be SHORT/CHAR to INTEGER conversion */
	    {
	    l->in.type = parmtype;
	    if (p->in.op == SCONV)
		{
		if (p->in.left->in.type == parmtype)
		   { /* Remove SCONV node */
		   q = p; /* Remove q, re-use as ASSIGN node */
		   p = q->in.left;
		   }
		else
		   { /* Change SCONV node type */
		   p->in.type = parmtype;
		   q = getnode();  /* For ASSIGN node */
		   }
		}
	    else
		{ /* Add SCONV node */
		q = getnode();
		q->in.op = SCONV;
		q->in.type = parmtype;
		q->in.left = p;
		p = q;
		q = getnode();  /* For ASSIGN node */
		}
	    /* Fix up size to deal with SHORT/CHAR to INT conversion */
            l->tn.lval = -(exprbaseoffset + argoffset);
	    l->tn.lval += ((parmtype == CHAR) || (parmtype == UCHAR)) ? 3 : 2;
	    }

	l->in.name = NULL;
	l->in.c1data.c1word = 0; /* for initial assignment only */
	l->in.arraybase = 0;
	q->in.op = ASSIGN;
	q->in.type = l->in.type;
	q->in.tattrib = tmp->in.tattrib;
	q->in.left = l;
	q->in.right = p;
	}

			/* emit c1 symbol table entry for new temp */
    if ( globaloptim )
      {
	char buf[60];
      puttype(C1OREG, AUTOREG, l->in.type, 0);
      putlong( l->tn.lval );
      attrs = 0x10;		/* New OREG is a C0 generated temp */
      if (ISPTR(l->in.type))
        attrs |= 0x0400;	/* Argument is a pointer */
      if (ISFTN(l->in.type))
        attrs |= 0x0100;
#if 0
      if (ISVOL(tmp->in.tattrib))
        attrs |= 0x1000;
      if (SCLASS(tmp->in.tattrib) == ATTR_REG)
        attrs |= 0x0040;
#endif
      puttriple(2, attrs, 0);	/* 2 means this is a local variable */
      sprintf(buf, "passc0-arg%d", c0tempnum);
      c0tempnum++;
      putstring(buf);
      }

    puttree(q,0);			/* emit the assignment code */
    puttriple(FEXPR,lineno,0);	/* FEXPR node on top */
    l->in.c1data = p->in.c1data;  /* subsequent uses flag c1data */
    l->in.arraybase = l->tn.lval; /* special baseoff arrangement
					for c1 */
    return (l);
}  /* evalarg */


/*****************************************************************************/

NODE * assignargtotmp(p, exprbaseoffset, lineno)
NODE *p;
long exprbaseoffset;
long lineno;
{
    register NODE *q;
    register NODE *l;
    argoffset += 4;
    l = getnode();
    l->in.op = OREG;
    l->in.type = p->in.type;
    l->tn.rval = A6;
    l->tn.lval = -(exprbaseoffset + argoffset);
    l->in.name = NULL;
    l->in.c1data.c1word = 0; /* for initial assignment only */
    l->in.arraybase = 0;
    q = getnode();
    q->in.op = ASSIGN;
    q->in.type = p->in.type;
    q->in.left = l;
    q->in.right = p;

    if ( p->in.c1data.c1word )
			/* emit special assignment node flag for c1 */
      puttriple(C0TEMPASG, 0, 0);
			/* emit c1 symbol table entry for new temp */
    if ( globaloptim )
      {
	char buf[60];
      puttype(C1OREG, AUTOREG, l->in.type, 0);
      putlong( l->tn.lval );
      if ( p->in.c1data.c1word )
	puttriple(0, 0x0410, 0); /*flag/c1 to avoid aliasing problems*/
      else
	puttriple(0, 0, 0);
      sprintf(buf, "passc0-tmp%d", c0tempnum);
      c0tempnum++;
      putstring(buf);
      }

    puttree(q,0);			/* emit the assignment code */
    puttriple(FEXPR,lineno,0);	/* FEXPR node on top */
    l->in.c1data = p->in.c1data;  /* subsequent uses flag c1data */
    l->in.arraybase = l->tn.lval; /* special baseoff arrangement
					for c1 */
    return (l);
}  /* assignargtotmp */



/*****************************************************************************/

/* 
 *  collectargtypes -- collect argument array/struct type info
 */
collectargtypes(p)
NODE *p;		/* expression tree nodes describing arguments */
{
    flag ok;
    C1DATA data;

    ok = NO;
    data.c1word = 0;

    switch (p->in.op)
    {
    case CM:				/* COMMA operator -- joins args */
	collectargtypes(p->in.left);
	collectargtypes(p->in.right);
	break;

    case COMOP:			/* evaluating expr to local space */
				/* argument is then a temporary loc */
	data = p->in.right->in.c1data;
	ok = YES;
	break;

    case ICON:				/* Static (COMMON) variable */
    case PLUS:				/* Item on the local stack */
    case UNARY MUL:			/* First element in Shared Common */
	data = p->in.c1data;
	ok = YES;
	break;
	
    case OREG:		/* argument is parameter in current procedure */
	if (p->tn.rval == A6)
	    {
	    data = p->in.c1data;
	    ok = YES;
	    break;
	    }
	else		/* OREG, but not %a6 -- should never happen */
	    fatal("strange argument in collectargtypes() - 3");
    default:	   /* only CM, COMOP, PLUS, ICON and OREG types acceptable */
	fatal("strange argument in collectargtypes() - 4");
    }
    if ( ok )
      {
      if ( data.c1flags.isstruct )
	argc1data[ nargs++ ] = C0STRUCT;
      else if ( data.c1flags.isarray )
	argc1data[ nargs++ ] = C0ARRAY;
      else
	argc1data[ nargs++ ] = 0;
      }
    return;
} /* collectargtypes */

/*****************************************************************************/

/* doip -- integrate called procedure into caller */
long doip(callinfo,parentpd,topnode)
ExprCallRec *callinfo;		/* call info structure for this call */
pPD parentpd;			/* pd of calling routine */
NODE *topnode;			/* top of tree containing call */
{
    long inprolog = YES;	/* in prolog of callee */
    register NODE **ipsp;  	/* next free position on the postfix stack */
    register long x;		/* input record */
    register NODE *p;		/* current tree node */
    long	strwords;	/* # 4-byte words in string */
    register long i;
    long	lineno;		/* current line number (for messages) */
    char	filename[MAXSTRBYTES];	/* current filename */
    char	buffer[MAXSTRBYTES];	/* buffer for FTEXT messages */
    long	altreturnflag = 0;	/* seen flag of alt return FORCE? */
    C1DATA	c1nodesseen;
    long	arrayoffset;
    long	structoffset;
    NODE	localnode;	/* local scratch node */
    HVREC	*hvlist = NULL, *hvtail;	/* hidden var list */
    flag	c0tempasgflag;	/* just seen nested c0temp ? */
    VOLREC *vf;			/* volatile flagging pointer */
    NODE *q;

    ippd = callinfo->pd;	/* called procedure pd */

    /* set callee FILE descriptor to location in input file */
    fflush(ofd);
    if (ippd->noemitflag)
	ifd = itmpfd;
    else
	ifd = ipermfd;
    fseek(ifd, ippd->outoffset, 0);

    /* local stack is divided into several regions.  In order from the
     * frame pointer (a6), they are:
     *		adjustable array dimension values
     *		function result value	(12 bytes if a function)
     *		local copies of the formal parameters
     *		local variables
     */
    mindimoffset = -(ippd->dimbytes);	/* adjustable array dimension values */
    minfnoffset = mindimoffset - ippd->typeproc * 12; /* mindimoffset if TYSUBR,
						      * mindimoffset-12 if fn */
    retoffset = -(callinfo->retloc);	/* offset from A6 of result */
    minargoffset = nargs * (-4) + minfnoffset;
					/* OREG with offset more negative than
					 * this are local vars, not arguments */

    if (ftnflag)
        tempbase += minargoffset - mindimoffset;
				/* no sense allocating stack space for */
				/* non-existent arguments and result values */

    maplabels(ippd,parentpd);	/* assign "new" label values */

    freeipnp = ipnp;		/* initialize ip name pool */
    ipfreenode = ipnode;	/* initialize ip tree node storage */

    ipsp = ipstack;		/* postfix expression stack is empty */

    c1nodesseen.c1word = 0;	/* ain't seen nothin' yet */
    assignflag = NO;
    c0tempasgflag = NO;
    c0temps = NULL;

      	/* flag c0temps corresponding to formal parms marked c2 volatile */

    vf = ippd->vollist;
    while ( vf != NULL )
      {
      if (!ftnflag)
	if (vf->off > 0)
	  {
	  i = formatc2(( -cstackoffset - vf->off + 4), vf->siz);
	  puttriple(FTEXT,0,i);
	  putnlongs(i);
	  }
      else if (vf->off < minfnoffset && vf->off >= minargoffset)
	{		/* formal arg flagged volatile */
	i = -((vf->off - minfnoffset) / 4) - 1;		/* arg number */
	if ( argvf[ i ] )		/* c0temp was used for this parm */
	  {
	  i = formatc2( arg[ i ]->tn.lval, 4 );
	  puttriple(FTEXT, 0, i);
	  putnlongs( i ); /* emit "#7 -X(%a6),Y" for c2 volatile flagging */
	  }
	}
      vf = vf->next;
      }

    sprintf(buffer,"# begin procedure %s",
	    ((pNameRec)(pnp + ippd->pnpoffset))->name);
    puttext(buffer);		/* emit comment as FTEXT -- for debugging */

    if (ippd->regsused)			/* if callee needs regs, save */
	saveregs(ippd->regsused);	/* caller's copy */

    /* read nodes from called procedure and in-line the procedure !!! */
    for(;;){
	x = rdlong();		/* get input record */

#ifdef DEBUGGING
	if( xdebugflag ) fprintf( stdout, "\nop=%s., val = %d., rest = 0%o\n",
			xfop(FOP(x)), VAL(x), (int)REST(x) );
#endif
	switch( (int)FOP(x) ){  /* switch on opcode */

	case 0:		/* should never be seen */
	    fprintf( stdout, "null opcode ignored\n" );
	    break;

	case C0TEMPASG:
	    putlong(x);
	    c0tempasgflag = YES;
	    break;

	case ARRAYREF:
	    c1nodesseen.c1flags.isarray = 1;
	    c1nodesseen.c1flags.iselement = VAL(x);
	    arrayoffset = rdlong();
	    break;

	case STRUCTREF:
	    if ( c1nodesseen.c1flags.isarray )
	      fatal("Active ARRAYREF at time of STRUCTREF");
	    c1nodesseen.c1flags.isstruct = 1;
	    c1nodesseen.c1flags.isprimaryref = VAL(x);
	    if ( VAL(x) )
	      structoffset = rdlong();
	    else
	      structoffset = 0;
	    break;

	case FENTRY:
	    strwords = rdstring();
	    putlong(x);
	    putstring(strbuf);
	    break;

	case NAME:		/* static name leaf node */
	    p = getipnode();
	    p->in.op = NAME;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = 0;
	    if( VAL(x) )
		{
		p->tn.lval = rdlong();
		}
	    else p->tn.lval = 0;
	    strwords = rdstring();
	    p->in.name = addtoipnp(strbuf, strwords);
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;

	case ICON:		/* constant or static name leaf node */
	    p = getipnode();
	    p->in.op = ICON;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = 0;
	    p->tn.lval = rdlong();
	    if( VAL(x) )
		{
		strwords = rdstring();
	        p->in.name = addtoipnp(strbuf,strwords);
		}
	    else p->in.name = 0;
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}

bump:
	    *ipsp++ = p;		/* push node on postfix stack */
	    if( ipsp >= &(ipstack[ipmaxstack]) )  /* stack full */
		{
		NODE **ipsptemp = ipsp;
		reallocipstack(&ipsptemp);    /* reallocate callee expr stack */
		ipsp = ipsptemp;
		}
    	    break;

	case GOTO:
	    if( VAL(x) )
		{
		/* unconditional branch */
	        putlong(x);
		putlong(newlabel(rdlong()));
	        break;
		}
	    else   /* treat as unary */
		goto def;

	case SETREGS:
	    putlong(x);
	    putlong(rdlong());
	    break;

	case REG:		/* register leaf node */
	    p = getipnode();
	    p->in.op = REG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = VAL(x);
	    p->tn.lval = 0;
	    p->in.name = 0;
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;

	case OREG:		/* register + offset leaf node */
	    p = getipnode();
	    p->in.op = OREG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = VAL(x);
	    p->tn.lval = rdlong();
	    p->in.name = NULL;
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;

	case C1SYMTAB:
	    if (VAL(x) == 1)
	      {
	      farg_high = rdlong();
	      farg_low = rdlong();
	      farg_pos = rdlong();
	      }
	    break;

	case NOEFFECTS:		/* copied in pass1, toss it */
	    rdstring();
	    break;

	case C1NAME:		/* copy in FTN, shouldn't see in C */
	    if (!ftnflag) fatal("Unexpected C1NAME in cpass2");
	    p = &localnode;
	    p->tn.op = NAME;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    puttype(FOP(x), VAL(x), p->in.type, p->in.tattrib);
	    p->tn.rval = 0;
	    if ( VAL(x) )
	      {
	      p->tn.lval = rdlong();
	      putlong(p->tn.lval);
	      }
	    else
	      p->tn.lval = 0;
	    strwords = rdstring();
	    putstring(strbuf);
	    x = rdlong();
	    putlong( x );
	    if ( x & C1ARY )
	      putlong( rdlong() );
	    strwords = rdstring();
	    putstring(strbuf);
	    break;

	case C1OREG:	/* pass local stack vars thru with offset mapping */
	    p = &localnode;
	    p->in.op = OREG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.lval = rdlong();
	    if ((!ftnflag) && (p->tn.lval > 0))
		{
		x = rdlong();
		if (x & C1ARY) rdlong();
		rdstring();
		break;
		}
	    if ((ftnflag) && (p->tn.lval >= minargoffset)) /* ftn result and formal arg area */
	      {
	      if (p->tn.lval >= mindimoffset)	/* dimension info location */
		p->tn.lval -= tempbase - minargoffset + mindimoffset;
	      else if (p->tn.lval >= minfnoffset)
	        p->tn.lval = retoffset + (p->tn.lval - minfnoffset);
	      else
	        { /* discard pass1 formal arg descr., c0 creates as needed */
		x = rdlong();
	        if ( x & C1ARY )
		  rdlong();
	        rdstring();
	        break;
	        }
	      }
	    else				/* local stack variable */
	      p->tn.lval -= tempbase;	/* relocate stack variable */

	    puttype(FOP(x), VAL(x), p->in.type, p->in.tattrib);
	    putlong( p->tn.lval );
	    x = rdlong();
	    putlong( x );
	    if ( x & C1ARY )
	      putlong( rdlong() );
	    strwords = rdstring();
	    putstring(strbuf);
	    break;

	case C1HIDDENVARS:
	        {
	        int i,j,r,v;
		HVREC *t;
    
	        j = REST(x);
	        for (i=0; i < j; ++i)
	          {
	          x = rdlong();
		  r = REST(x);
		  v = VAL(x);
	          if (FOP(x) == C1HVOREG)
		    t = mapc1hvoreg( r, v, rdlong() );	/* map or replace */
	          else if (FOP(x) == C1HVNAME)
		    {
		    x = rdlong();
		    strwords = rdstring();
		    t = gethvrec(C1HVNAME, x, r, v, strbuf); /*pass unchanged */
		    }
	          else
		    fatal("unexpected opcode in C1HIDDENVARS");
		  if ( hvlist == NULL )
		    {
		    hvlist = t;
		    hvlist->cnt = j;
		    }
		  else
		    hvtail->next = t;
		  hvtail = t;
	          }
	        }
	        break;

	case CALL:
	case UNARY CALL:
	        p = getipnode();
	        p->in.op = FOP(x);
	        gettype(x, &p->in.type, &p->in.tattrib);
		p->in.rall = VAL(x);
	        p->in.c1data.c1word = 0;
		p->in.name = (char *) hvlist;
		hvlist = NULL;
		if ( FOP(x) == CALL )
	          p->in.right = *--ipsp;
	        p->in.left = *--ipsp;
		c1nodesseen.c1word = 0;
	        goto bump;

	case STCALL:
	case UNARY STCALL:
		p = getipnode();
		p->in.op = FOP(x);
		p->stn.stalign = VAL(x);
		gettype(x,&p->in.type, &p->in.tattrib);
		p->stn.stsize = rdlong();
		if (optype(p->in.op) == BITYPE)
		    p->in.right = *--ipsp;
		else
		    p->tn.rval = REST(x);
		p->in.left = *--ipsp;
		c1nodesseen.c1word = 0;
		goto bump;

	case FLD:
		p = getipnode();
		p->in.op = FOP(x);
		gettype(x, &p->in.type, &p->in.tattrib);
		p->tn.rval = rdlong();
		p->in.left = *--ipsp;
		p->in.c1data.c1word = 0;
		if (c1nodesseen.c1word)	/* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
		goto bump;

	case C1OPTIONS:
		break;

	case VAREXPRFMTDEF:
	case VAREXPRFMTREF:
	case VAREXPRFMTEND:
		putlong(x);
		break;

	case FORCE:
	    if (altreturnflag)	/* this FORCE is part of an alternate RETURN */
		{
		NODE *q;

		/* alternate return --  FORCE into D0 */
		/* replace FORCE with ASSIGN to result location */
		/* construct OREG to describe result location */
		p = getipnode();
		p->in.op = FC0OREG;
		if (ftnflag)
			p->in.type = LONG;
		else
			p->in.type = ((callinfo->nptr)->in.left)->in.type;
		p->tn.rval = A6;
		p->tn.lval = retoffset;
		p->in.name = 0;
		p->in.c1data.c1word = 0;
		q = p;

		/* construct ASSIGN op */
		p = getipnode();
		p->in.op = ASSIGN;
		p->in.type = q->in.type;

		/* create tree */
		p->in.left = q;
		p->in.right = *--ipsp;

		altreturnflag = 0;	/* turn off flag for next time */
		goto bump;
		}
	    else
		goto def;
	    break;

	case FTEXT:
	    strwords = VAL(x);
	    rdnlongs(strwords);
	    strbuf[strwords*4] = '\0';		/* guarantee a NULL */
	    if (inprolog)	/* am I still in procedure prolog? */
				/* expect to see "link", "movm", argument
				 * "mov"s, "lea.l fpa_loc", nothing else
				 */
		{			/* don't emit any prolog junk */
		if (!strncmp(strbuf,"#c0 pe",6))	/* prolog end */
		    {
		    inprolog = NO;
		    }
		if ( !strncmp(strbuf,"#7",2) ) /* C2 volatile flag */
		  mapvolatile( x );
		}
	    else			/* not in procedure prologue */
		{
		if ((strwords == 2) && !strncmp(strbuf,"#c0 R",5))
		    {
		    altreturnflag = YES;  /* alternate RETURN */
		    }
		else if ((strwords == 2) && !strncmp(strbuf,"#c0 q",5))
		    {
		    altreturnflag = YES;  /* Standard C return */
		    }
		else if ((strwords == 2) && !strncmp(strbuf,"#c0 a", 5))
		    {
	    	    assignflag = YES;	/* ASSIGN -- label in ICON node */
		    }
		else if ((strwords >= 6) && !strncmp(strbuf,"\tmovm.l\t",8))
		    {
		    unsigned short regmask;
		    long offset, nbytes;

		    /* These movm's are not the prolog/epilog save/restore
		     * operations.  Rather, they save/restore registers
		     * around in-lined procedures.  The following code
		     * will only be executed for a 2+ level effect.
		     * Relocate the save location for the registers.
		     */
		    if (strbuf[8] == '&')	/* save operation */
			{
			sscanf(strbuf+16,"%d",&offset);
			offset -= tempbase;	/* relocate saved regs */
			nbytes = sprintf(strbuf+16,"%d(%%a6)",offset);
			strbuf[17+nbytes] = '\0';
			strbuf[18+nbytes] = '\0';
			strbuf[19+nbytes] = '\0';
			strwords = 5 + nbytes/4;
			}
		    else			/* restore operation */
			{
			sscanf(strbuf+8, "%d%*c%*c%*c%*c%*c%*c%*c%*c%*c%hx",
			       &offset, &regmask);
			offset -= tempbase;	/* relocate saved regs */
			nbytes = sprintf(strbuf,"\tmovm.l\t%d(%%a6),&0x%.4x",
					   offset, regmask);
			strbuf[1 + nbytes] = '\0';
			strbuf[2 + nbytes] = '\0';
			strbuf[3 + nbytes] = '\0';
			strwords = nbytes / 4 + 1;
			}
		    }
		else if ( !strncmp(strbuf,"#7",2) ) /* C2 volatile flag */
		  {
		  mapvolatile( x );
		  break;
		  }
		if (!altreturnflag)
		    {
	            puttriple(FTEXT,REST(x),strwords);
		    putnlongs(strwords);
		    }
		}
	    break;

	case FEXPR:			/* end of expression marker */
	    lineno = REST(x);
	    if( strwords = (VAL(x)) )	/* there is a filename -- read it */
		{
		rdnlongs( strwords );
		strbuf[strwords*4] = '\0';	/* guarantee a NULL */
		strcpy(filename, strbuf);
		}
	    if( ipsp == ipstack )   /* filename only -- emit FTEXT unchanged */
		{
		putlong(x);
		if (strwords)
		    putnlongs(strwords);
		break;
		}

	    if( --ipsp != ipstack )
		{
		fatal( "expression poorly formed (doip)" );
		}
	    p = ipstack[0];		/* root of expression */

	    if ((p->in.op == FORCE) && ((ftell(ifd) + 4) == ippd->outroffset))
		{			/* last FORCE of function is store
					 * of result into d0,d1.  Don't emit
					 * this.
					 */
		/* Do nothing */
		}
	    else if ( inprolog )
		{		 /* Skip it */
		assignflag = NO;	/* turn off flag for next time */
		c0tempasgflag = NO;
	        freeiptree(p);		/* recover node and name pool storage */
		break;
		}
	    else	/* normal tree -- relocate labels, local vars and
			 * replace parameters by actual arguments.  Then
			 * emit the fixed tree.
			 */
		{
		if ((assignflag) && (p->in.op == ASSIGN)
		 && (p->in.right->in.op == ICON) && (p->in.right->in.name))
		    {			/* if it's an ASSIGN -- fix the ICON */
		    char buf[8];
		    long lab;
		    sscanf(p->in.right->in.name+1,"%d",&lab);
		    sprintf(buf,"L%d",newlabel(lab));	/* relocate the label */
		    p->in.right->in.name = addtoipnp(buf,2);
		    }
		assignflag = NO;	/* turn off flag for next time */

		if ( c0tempasgflag )
		  {
		  if ( p->in.op == ASSIGN &&
			p->in.left->in.op == OREG &&
			p->in.left->tn.rval == A6 )
		    {
		    C0TEMPREC *t;
		    t = (C0TEMPREC *) malloc( sizeof( C0TEMPREC ) );
		    if ( t == NULL )
		      fatal("Out of memory building C0TEMPREC list");
		    t->next = NULL;
		    t->baseoff = p->in.left->tn.lval;
		    if ( c0temps == NULL )
		      c0temps = t;
		    else
		      c0tempstail->next = t;
		    c0tempstail = t;
		    }
		  c0tempasgflag = NO;
		  }
	        fixtree(p,0,0,0);	/* perform all translations on tree */
	        puttree(p,0);		/* emit the fixed tree */
	        putlong(x);		/* emit FEXPR node, too */
	        if (strwords)
		    {
		    strcpy(strbuf,filename);
		    putstring(strbuf);
		    }
		}
	    freeiptree(p);		/* recover node and name pool storage */
	    break;

	case FSWITCH:
	    fatal( "switch not yet done" );
	    break;

	case FCMGO:		/* computed goto -- relocate code labels */
	    putlong(x);
	    for (i = REST(x); i > 0; i--)
		putlong(newlabel(rdlong()));
	    break;

	case SWTCH:
	    putlong (x);
	    for (i = REST(x); i > 0; i--)
		{
	        putlong(rdlong());		/* put out values */
		putlong(newlabel(rdlong()));	/* relocate code labels */
		}
	    break;

	case FLBRAC:			/* beginning of procedure */
	    x = rdlong();
	    x = rdlong();
	    x = rdlong();
	    strwords = rdstring();
	    break;		/* ignored */

	case FRBRAC:			/* end of procedure */
	    if (ippd->regsused || 
			ippd->fdregsused) /* restore caller's registers */
		restoreregs(ippd->regsused,ippd->fdregsused);

	    putlong(callerSetregsMask);	/* restore SETREGS environment */
	    putlong(callerSetregsFDMask);

	    /* emit comment for debugging purposes */
	    sprintf(buffer,"# end procedure %s\0",
		    ((pNameRec)(pnp + ippd->pnpoffset))->name);

	    /* If the inlined procedure was a FUNCTION or acted like a
	     * FUNCTION (SUBR w/ alt RETURNs), replace the CALL in the
	     * original tree with the result location.  The modified
	     * expression tree will be omitted by pass2 after all calls
	     * are inlined.
	     */
	    if ((ftnflag && (minfnoffset || ippd->hasaltrets)) ||
		((!ftnflag) && (topnode != callinfo->nptr)))	 /* ftn call */
		{
		/* replace CALL or UCALL with result location (OREG) */
		p = getnode();
		p->in.type = ((callinfo->nptr)->in.left)->in.type;
		(callinfo->nptr)->in.left = p;
		p->in.op = OREG;
		p->tn.rval = A6;
		p->tn.lval = retoffset;
		p->in.name = NULL;
		p->in.c1data.c1word = 0;
		}
	    puttext(buffer);		/* emit comment as FTEXT */
	    ifd = isrcfd;		/* reset input file to caller code */
	    while ( c0temps != NULL )
		{			/* cleanup nested c0temp list */
		C0TEMPREC *t;
		t = c0temps;
		c0temps = c0temps->next;
		free( t );
		}
	    return;

	case STASG:
		/* NODE *newnod; */
		p = getipnode();
		p->in.op = STASG;
		gettype(x, &p->in.type, &p->in.tattrib);
		p->in.right = *--ipsp;
		p->in.left = *--ipsp;
#if 0
		p->in.type = STRTY + PTR;
#endif
		p->stn.stsize = rdlong();
		p->in.c1data.c1word = 0;
	    	if (c1nodesseen.c1word)	/* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
		        p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
		        p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }

		goto bump;

	case STARG:
		p = getipnode();
		p->in.op = STARG;
		gettype(x, &p->in.type, &p->in.tattrib);
		p->stn.stalign = VAL(x);
		p->stn.stsize = rdlong();
		p->tn.rval = 0;
		p->in.left = *--ipsp;
		p->in.c1data.c1word = 0;
	    	if (c1nodesseen.c1word)	/* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
		        p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
		        p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
		goto bump;

	case FICONLAB:
	    {
	    long lab = (long) (((x) >> 8) & 0xffffff);
	    if ( ! inprolog )
	      {
	      x = (newlabel(lab) << 8) | FICONLAB;
	      putlong(x);
	      }
	    assignflag = YES;	/* ASSIGN -- label in ICON node */
	    break;
	    }

	case LABEL:			/* relocate label */
	    {
	    long lab = (long) (((x) >> 8) & 0xffffff);
	    x = (newlabel(lab) << 8) | LABEL;
	    putlong(x);
	    }
	    break;

	case MINUS:
	case PLUS:
	    p = getipnode();	/* save interior node of tree */
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.right = *--ipsp;	/* top on postfix stack */
	    p->in.left = *--ipsp;	/* next on postfix stack */
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;		/* store this node on stack */

	case UNARY MUL:
	    p = getipnode();	/* save interior node of tree */
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.left = *--ipsp;	/* top on postfix stack */
	    p->tn.rval = 0;
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;		/* store this node on stack */

	default:			/* interior OP node */
def:
	    p = getipnode();
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.c1data.c1word = 0;

	    switch( optype( p->in.op ) ){

	    case BITYPE:
		p->in.right = *--ipsp;
		p->in.left = *--ipsp;
		goto bump;

	    case UTYPE:
		p->in.left = *--ipsp;
		p->tn.rval = 0;
		goto bump;

	    case LTYPE:
		sprintf(warnbuff,"illegal leaf node (doip): %d", p->in.op );
		fatal(warnbuff);
	    }
	}
    }
}  /* doip */

/*****************************************************************************/

/* maplabels -- allocate new labels to avoid assembler problems
 * When a procedure is inlined, all its defined labels (on code) must
 * be reassigned to unique values so the assembler doesn't see
 * multiply-defined labels
 */
maplabels(ippd,parentpd)
pPD ippd;	/* called proc pd */
pPD parentpd;	/* calling proc pd */
{
    long nlab = ippd->nlab;			/* number of labels in callee */
    register long *lab = ippd->labels + nlab;	/* callee label array */
    register long *labend = lab + nlab;		/* end of callee label array */
    register long *parentlab = labarray + nxtfreelab;	/* next free slot in
							 * caller label array */
    if ((nlab + nxtfreelab) > maxlabels)	/* not enough free slots */
	{
	realloclab(nlab + nxtfreelab);		/* create bigger parent array */
	parentlab = labarray + nxtfreelab;
	}
    for (; lab < labend; *parentlab++ = *lab++ = nxtlabel++)
	;					/* assign new label values */
    nxtfreelab += nlab;
}  /* maplabels */

/*****************************************************************************/

/* newlabel -- map argument to corresponding new label number */
newlabel(n)
register long n;	/* label to be relocated */
{
    register long *l = ippd->labels;	/* current label */
    register long *e = l + ippd->nlab;	/* end of label table */

    while (l < e)			/* search entire label table */
	if (n == *l)			/* found it */
	    return( *(l + ippd->nlab) );
	else
	    l++;
    return (n);		/* no match (data label?) -- return unchanged */
}

/*****************************************************************************/

/* fixtree -- perform all modifications on callee tree -- relocate labels
 *   and local vars, replace formal parameters with actual arguments.
 *   fixtree() is called recursively (prefix order) on an expression tree.
 */
fixtree(t,follow,lr,incall)
register NODE *t;	/* input tree */
NODE *follow;		/* parent node of input tree */
long lr;		/* tree is which child of parent? 0 (left),1 (right) */
long incall;		/* 1 == seen CALL or UNARY CALL, 0 == haven't */
{
    register NODE *l;	/* left child of top node */
    register NODE *r;	/* right child of top node */
    register NODE *m;	/* handy node pointer */
    register short opty;
    long argno;		/* argument number */
    NODE *p;		/* 'p' and 'q' are used in argument transformation */
    NODE *q;

    C1DATA newc1data();

    if ((!ftnflag) && (t->in.c1data.c1word))
	{
	if (t->in.arraybase < 0)
	    t->in.arraybase -= tempbase; /* Shift local array reference */
	else if (t->in.arraybase > 0)
	    fatal("unexpected positive arraybase");
	}
    switch(t->in.op) {
    case CBRANCH:	/* conditional branch -- relocate target label */
        if ((r = t->in.right)->in.op == ICON)
	    {
	    r->tn.lval = newlabel(r->tn.lval);
	    fixtree(t->in.left,t,0,incall);
	    return;
	    }
	break;

    case UNARY MUL:	/* dereference -- may be a formal parameter here */
	if (!ftnflag) break;
	if ((((l=t->in.left)->in.op) == OREG) && (l->tn.rval == A6) &&
	    (l->tn.lval >= minargoffset))	/* a formal parameter !!! */
	    {
	    argno = -((l->tn.lval - minfnoffset) / 4) - 1;
						/* argument number */
	    p = arg[argno];			/* actual argument */
	    q = getipnode();
	    q->in.c1data.c1word = 0;
	    if (p->in.op == OREG)		/* actual is parameter too */
		{
		memcpy(q,p,sizeof(NODE));
		t->in.left = q;
		t->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		t->in.arraybase = p->in.arraybase;
		q->in.c1data.c1word = 0;
		if ( t->in.c1data.c1flags.isstruct )
		  q->in.c1data.c1flags.isstruct = YES;
		return;
		}
	    else if (p->in.op == PLUS)		/* argument is stack var */
		{
		q->in.op = OREG;		/* convert to OREG form */
		q->in.type = t->in.type;
		q->tn.rval = A6;
		q->tn.lval = (p->in.right)->tn.lval;
	        if (lr)
		    follow->in.right = q;
	        else
		    follow->in.left = q;
		q->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		q->in.arraybase = p->in.arraybase;
	        return;
		}
	    else if (p->in.op == ICON)		/* argument is static var */
		{
		memcpy(q,p,sizeof(NODE));
		if (q->tn.name)
		    q->in.op = NAME;		/* change to NAME form  */
		q->in.type = t->in.type;
		q->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		q->in.arraybase = p->in.arraybase;
	        if (lr)
		    follow->in.right = q;
	        else
		    follow->in.left = q;
	        return;
		}
	    else
		fatal("unexpected arg type in fixtree - 1");
	    break;
	    }

	else if ((((m=t->in.left)->in.op) == PLUS) &&
		 ((((l=m->in.left)->in.op) == OREG) &&
		  (l->tn.rval == A6) &&
		  (l->tn.lval >= minargoffset)) ||
		 ((((r=m->in.right)->in.op) == OREG) &&
		  (r->tn.rval == A6) &&
		  (r->tn.lval >= minargoffset))) 	/* array subscripting */
	    {
	    if (l->in.op != OREG)	/* canonicalize -- OREG on LEFT */
		{
		m->in.left = r;
		m->in.right = l;
		p = r;
		r = l;
		l = p;
		}
	    else
		{
		r = m->in.right;
		}
	    argno = -((l->tn.lval - minfnoffset) / 4) - 1;
	    p = arg[argno];
	    q = getipnode();
	    memcpy(q,p,sizeof(NODE));
	    if (p->in.op == OREG)		/* argument OREG type */
		{
		m->in.left = q;
		fixtree(r,m,1,incall);
		t->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		t->in.arraybase = p->in.arraybase;
		q->in.c1data.c1word = 0;
		if ( t->in.c1data.c1flags.isstruct )
		  q->in.c1data.c1flags.isstruct = YES;
		return;
		}
	    else if (p->in.op == ICON)	/* argument ICON type */
		{
		fixtree( r, m, 1, incall );
		if ((r->in.op == ICON) && (r->tn.name == NULL))
		    /* constant subscript -- collapse it */
		    {
		    q->in.op = NAME;
		    q->in.type = t->in.type;
		    q->tn.lval += r->tn.lval;
		    if (lr)
			follow->in.right = q;
		    else
			follow->in.left = q;
		    q->in.c1data = newc1data( t->in.c1data, p->in.c1data);
		    return;
		    }
		else		/* non-constant subscript */
		    {
		    			/* canonicalize ICON on right */
		    m->in.left = m->in.right;
		    m->in.right = q;
		    t->in.c1data = newc1data( t->in.c1data, p->in.c1data);
		    t->in.arraybase = p->in.arraybase;
		    q->in.c1data.c1word = 0;
		    if ( t->in.c1data.c1flags.isstruct )
		      q->in.c1data.c1flags.isstruct = YES;
		    return;
		    }
		}
	    else if (p->in.op == PLUS)	/* argument == REG PLUS ICON */
		{
		/*
		 *  Convert tree    U*   to         U*
		 *		    |		    |
		 *                  +               +
		 *		  /   \  	  /   \
		 *		OREG   ??	REG    +
		 *				     /   \
		 *				   ??     ICON
		 */
		m->in.left = m->in.right;
		l = getipnode();
		memcpy(l,p->in.right,sizeof(NODE));
		m->in.right = l;
		memcpy(q,m,sizeof(NODE));
		q->in.right = m;
		t->in.left = q;
		l = getipnode();
		memcpy(l,p->in.left,sizeof(NODE));
		l->in.c1data.c1word = 0;
		q->in.left = l;
		fixtree(m->in.left,m,0,incall);
		l = m->in.left;
		r = m->in.right;
		if ( l->in.op == ICON && r->in.op == ICON &&
		       l->in.name == NULL && r->in.name == NULL )
		  {		/* keep in canonical form, use oreg */
		  q->in.op = OREG;
		  q->tn.lval = l->tn.lval;
		  q->tn.lval += r->tn.lval;
		  q->tn.rval = A6;
		  q->in.type = t->in.type;
		  q->in.c1data = newc1data( t->in.c1data, p->in.c1data);
		  q->in.arraybase = p->in.arraybase;
		  if ( lr )
		    follow->in.right = q;
		  else
		    follow->in.left = q;
		  return;
		  }
		t->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		t->in.arraybase = p->in.arraybase;
		if ( t->in.c1data.c1flags.isstruct )
		  q->in.left->in.c1data.c1flags.isstruct = YES;
		return;
		}
	    return;
	    }
	else if (t->in.c1data.c1word)
	    {
	    l = t->in.left;
	    if ((l->in.op == OREG) && (l->tn.rval == A6) &&
			(l->tn.lval < minargoffset))
		{
		t->in.arraybase -= tempbase;
	    	}
	    else if (l->in.op == PLUS)
		{
		if ((l->in.left->in.op == REG) && (l->in.left->tn.rval == A6))
		    {
	    	    r = l->in.right;  /* r is what we are adding to REG A6 */
	    	    if (r->in.op == PLUS) r = r->in.right;
	    	    if ((r->in.op == ICON) && (r->in.name == 0))
			t->in.arraybase -= tempbase;
	    	    }
		else if ((l->in.left->in.op == OREG) && (l->in.left->tn.rval == A6) && (l->in.left->tn.lval < minargoffset))
		    {
			t->in.arraybase -= tempbase;
		    }
		}
	    }
	break;

    case MINUS:
	if (!ftnflag)
	    {
	    l = t->in.left;
	    if ((l->in.op == REG) && (l->tn.rval == A6))
		{
		r = t->in.right;
		if ((r->in.op == ICON) && (r->in.name == 0))
		    {
		    if (r->tn.lval <= 0)  /* Formal parameter */
			{
			r->tn.lval = - (- tempbase - 8 - r->tn.lval);
			}
		    else
			{
			r->tn.lval += tempbase;
			}
		    }
		else
		    fatal("A6 MINUS non-ICON error");
		}
	    }
	break;

    case PLUS:
	if (!ftnflag)
	    {
	    l = t->in.left;
	    if ((l->in.op == REG) && (l->tn.rval == A6))
		{
		r = t->in.right;
		if ((r->in.op == ICON) && (r->in.name == 0))
		    {
		    if (r->tn.lval > 0)  /* Formal parameter */
			{
			r->tn.lval = - tempbase - 8 + r->tn.lval;
			}
		    else
			{
			r->tn.lval -= tempbase;
			}
		    }
		else
		    fatal("A6 PLUS non-ICON error");
		}
	    break;
	    }
        if (((l = t->in.left)->in.op == REG) && (l->tn.rval == A6))
	    {
	    if (((r=t->in.right)->in.op == ICON) && (r->in.name == 0))
	        {
		if (r->tn.lval >= minfnoffset)	/* fn result area */
		    {
		    r->tn.lval = retoffset + (r->tn.lval - minfnoffset);
		    if (r->in.c1data.c1word)
			warn("internal error, Unexpected ARRAYREF node");
		    }
		else				/* local stack variable */
		    {
	            r->tn.lval -= tempbase;	/* relocate stack variable */
		    if (t->in.c1data.c1word)
			t->in.arraybase -= tempbase;
		    }
	        return;
	        }
	    else if ((r->in.op == PLUS) && ((r=(r->in.right))->in.op == ICON)
			&& (r->in.name == 0))
		{
		if (r->tn.lval >= minfnoffset)	/* fn result area */
		    {
		    r->tn.lval = retoffset + (r->tn.lval - minfnoffset);
		    if (r->in.c1data.c1word)
			warn("internal error, Unexpected ARRAYREF node");
		    }
		else				/* local stack variable */
		    {
	            r->tn.lval -= tempbase;	/* relocate stack variable */
		    if (t->in.c1data.c1word)
			t->in.arraybase -= tempbase;
		    }
		fixtree((t->in.right)->in.left,t->in.right,0,incall);
	        return;
		}
	    }
	else if ((((l=t->in.left)->in.op) == OREG) && (l->tn.rval == A6)
	  && (l->tn.lval >= minargoffset)	/* a formal parameter !!! */
	  && (l->tn.lval < minfnoffset))
	    {
	    argno = -((l->tn.lval - minfnoffset) / 4) - 1;
						/* argument number */
	    p = arg[argno];			/* actual argument */
	    q = getipnode();
	    if (p->in.op == OREG)		/* actual is parameter too */
		{
		memcpy(q,p,sizeof(NODE));
		t->in.left = q;
		fixtree(t->in.right,t,1,incall);
		t->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		t->in.arraybase = p->in.arraybase;
		q->in.c1data.c1word = 0;
		if ( t->in.c1data.c1flags.isstruct )
		  q->in.c1data.c1flags.isstruct = YES;
		return;
		}
	    else if (p->in.op == PLUS)		/* argument is stack var */
		{
		/*
		 *  Convert tree    +    to        +        or     +
		 *		  /   \          /   \           /   \
		 *		OREG   ??      REG  ICON        +    ??
		 *                                            /   \
		 *			 	 	    REG  ICON
		 *
		 *
		 * The first form is generated if ?? is an ICON, else the 
		 * second form is generated.  Due to wierdness, the first
		 * form is changed to (REG + (ICON + ??)), then if ?? is
		 * an ICON  The two ICON's are collapsed.  If ?? isn't an
		 * ICON, change the above tree to ((REG + ICON) + ICON)
		 */
		memcpy(q,t,sizeof(NODE));
		q->in.c1data.c1word = 0;
		q->in.left = t->in.right;
		l = getipnode();
		memcpy(l,p->in.right,sizeof(NODE));
		q->in.right = l;
		t->in.right = q;
		l = getipnode();
		memcpy(l,p->in.left,sizeof(NODE));
		l->in.c1data.c1word = 0;
		t->in.left = l;
		fixtree(q->in.left,q,0,incall);
		l = q->in.left;
		r = q->in.right;
		if ( l->in.op == ICON && r->in.op == ICON &&
		       (l->in.name == NULL && r->in.name == NULL) )
		  {		/* keep in canonical form, fold icons */
		  q->in.op = ICON;
		  q->tn.lval = l->tn.lval + r->tn.lval;
		  q->tn.name = NULL;
		  t->in.c1data = newc1data(t->in.c1data,p->in.c1data);
		  t->in.arraybase = p->in.arraybase;
		  }
		else
		  {
		  t->in.c1data = newc1data(t->in.c1data,p->in.c1data);
		  t->in.arraybase = p->in.arraybase;
		  /* Put in "right" order */
                  if ((t->in.op == PLUS) && (t->in.right->in.op == PLUS))
			{
			l = t->in.left;
			t->in.left = t->in.right;
			t->in.right = l;

		  	l = t->in.left->in.left;
		  	t->in.left->in.left = t->in.right;
			t->in.right = l;
			}
		  else
			fatal("unexpected arg type in fixtree - 3");

		  if ( t->in.c1data.c1flags.isstruct )
		    t->in.left->in.c1data.c1flags.isstruct = YES;
		  }
                return;
		}
	    else if (p->in.op == ICON)		/* argument is static var */
		{
		memcpy(q,p,sizeof(NODE));
		t->in.left = t->in.right;
		t->in.right = q;	/* canonicalize, icon on rt. */
		l = t->in.left;
		fixtree(l,t,0,incall);
		if ( l->in.op == ICON && l->in.name == NULL )
		  {		/* keep in canonical form, fold icons */
		  q->tn.lval += l->tn.lval;
		  q->in.c1data = newc1data(t->in.c1data,p->in.c1data);
		  q->in.arraybase = p->in.arraybase;
		  if ( lr )
		    follow->in.right = q;
		  else
		    follow->in.left = q;
		  return;
		  }
		t->in.c1data = newc1data(t->in.c1data,p->in.c1data);
		t->in.arraybase = p->in.arraybase;
		q->in.c1data.c1word = 0;
		if ( t->in.c1data.c1flags.isstruct )
		  q->in.c1data.c1flags.isstruct = YES;
	        return;
		}
	    else
		fatal("unexpected arg type in fixtree - 2");
	    }
	else if ((((l=t->in.left)->in.op) == OREG) && (l->tn.rval == A6)
			&& (l->tn.lval < minargoffset))
	    {
	    if (t->in.c1data.c1word)
		t->in.arraybase -= tempbase;
	    }
	break;

    case OREG:
	if (!ftnflag)
	    {
	    if (t->tn.rval == A6)
		{
		if (t->tn.lval > 0) /* formal parameter */
		    {
		    t->tn.lval = - tempbase - 8 + t->tn.lval;
		    }
		else                /* local variable */
		    {
		    t->tn.lval -= tempbase;
		    }
		}
	    break;
	    }
        if (t->tn.rval == A6)
	    {
	    if (t->tn.lval >= mindimoffset)	/* dimension info location */
		{
		t->tn.lval -= tempbase - minargoffset + mindimoffset;
		if (t->in.c1data.c1word)
			warn("internal error, Unexpected ARRAYREF node");
		}
	    else if (t->tn.lval >= minfnoffset)	/* fn result area */
		{
		t->tn.lval = retoffset + (t->tn.lval - minfnoffset);
		if (t->in.c1data.c1word)
			warn("internal error, Unexpected ARRAYREF node");
		}
	    else if (t->tn.lval >= minargoffset)	/* formal parameter */
		{
		C1DATA tc1;
	        argno = -((t->tn.lval - minfnoffset) / 4) - 1;
		p = arg[argno];
		q = getipnode();
		memcpy(q,p,sizeof(NODE));
		q->in.c1data = newc1data( t->in.c1data, p->in.c1data );
		if ( ! incall ) /* Not passing as argument in call */
			/* argument -- must be assign to A register for array */
			/* copy argument unchanged */
		    {
		    if (p->in.op == PLUS)
			{
		        r = getipnode();
			q->in.right = r;
			memcpy(r,p->in.right,sizeof(NODE));
		        l = getipnode();
			q->in.left = l;
			memcpy(l,p->in.left,sizeof(NODE));
			l->in.c1data.c1word = 0;
			if ( q->in.c1data.c1flags.isstruct )
			  l->in.c1data.c1flags.isstruct = YES;
			}
		    }
		if ( lr )
		  follow->in.right = q;
		else
		  follow->in.left = q;
		return;
		}
	    else 	/* not a formal parameter, relocate stack variable */
		{
	        t->tn.lval -= tempbase;
		if (t->in.c1data.c1word)
			t->in.arraybase -= tempbase;
		}
	    return;
	    }
	break;

    case FC0OREG:	/* special OREG that must not be relocated */
	return;

    case CALL:
    case UNARY CALL:
	incall = YES;	/* processing call arguments */
	break;
    }

    /* recursively descend tree */
    opty = optype(t->in.op);
    if( opty != LTYPE )
	{
	if ((!ftnflag) &&
            (t->in.op != PLUS) &&
            (t->in.op != MINUS) &&
            (t->in.left->in.op == REG) &&
            (t->in.left->tn.rval == A6))
	    fatal("Unexpected use of A6 (1)");
	fixtree( t->in.left, t, 0, incall );
	}
    if( opty == BITYPE )
	{
	if ((!ftnflag) &&
            (t->in.right->in.op == REG) &&
            (t->in.right->tn.rval == A6))
	    fatal("Unexpected use of A6 (2)");
	fixtree( t->in.right, t, 1, incall );
	}
    return;
}

/*****************************************************************************/

/*
   This is only called for C routines for UNARY MUL nodes with a arrayref
   node attached to them.  It returns 1 if the arraybase should be shifted.
   i.e.  It represents the base of a stack based array.  It returns -1 if
   it gets confused (parameter with a arrayref node.
*/
   

shifted(p) NODE *p;
    {
    register NODE *l,*r;

    l = p->in.left;
    if ((l->in.op == PLUS) || (l->in.op == MINUS))
	{
	r = l->in.right;
	l = l->in.left;
	if ((l->in.op == REG) && (l->tn.rval == A6) &&
	    (r->in.op == ICON) && (r->in.name == NULL))
	    {
	    if (r->tn.lval > 0)
		return(-1);
	    else
		return(1);
	    }
	}
    if ((l->in.op == OREG) && (l->tn.rval == A6))
        {
        if (l->tn.lval > 0)
	    return(-1);
	else
	    return(1);
	}
    if ((l->in.op == PLUS) || (l->in.op == MINUS))
	{
	r = l->in.right;
	l = l->in.left;
	if ((l->in.op == REG) && (l->tn.rval == A6) &&
	    (r->in.op == ICON) && (r->in.name == NULL))
	    {
	    if (r->tn.lval > 0)
		return(-1);
	    else
		return(1);
	    }
	}
    }

/*****************************************************************************/

/* saveregs -- emit "movm.l" instruction to save registers in calling proc */
saveregs(regmask,fdregmask)
long regmask;			/* register mask for save operation */
long fdregmask;			/* flt pt register mask for save operation */
{
    register unsigned short regs = regmask & 0xffff;
    char buffer[100];		/* for construction of FTEXT code */
    long saveoffset;		/* where registers will be saved */
    register long i;
    register long nregs = 0;

    /* count the number of registers saved -- to determine save location */
    for (i=0; i<=15; i++)
	{
	if ((regs >> i) & 1)
	    nregs++;
	}

    /* save registers just below relocated local vars */
    saveoffset = -(tempbase + (ippd->stackuse>>3) + nregs*4);

    /* construct and emit the movm.l instruction to do it */
    sprintf(buffer,"\tmovm.l\t&0x%.4x,%d(%%a6)\0\0\0\0", regs, saveoffset);
    puttext(buffer);
}

/*****************************************************************************/

/* restoreregs -- emit "movm.l" instruction to restore registers */
/*       also, adjust "tempbase" to account for register save area */
restoreregs(regmask)
long regmask;			/* registers to be restored */
{
    register unsigned short regs = regmask & 0xffff;
    char buffer[100];		/* buffer for construction of FTEXT code */
    long saveoffset;		/* where registers are saved */
    register long i;
    register long nregs = 0;

    /* count the number of registers saved -- to determine save location */
    for (i=0; i<=15; i++)
	{
	if ((regs >> i) & 1)
	    nregs++;
	}

    saveoffset = -(tempbase + (ippd->stackuse>>3) + nregs*4);

    /* emit code to actually do it */
    sprintf(buffer,"\tmovm.l\t%d(%%a6),&0x%.4x\0\0\0\0", saveoffset, regs);
    puttext(buffer);

    /* local stack requirements of calling proc must be increased by
     * register save area.
     */
    tempbase += nregs*4;	/* 4 bytes per register */
}  /* restoreregs */

/*****************************************************************************/

/* inlinethunk -- integrate called var expr fmt "thunk" into caller */
inlinethunk(fileoffset)
long fileoffset;		/* offset in ipermfd of start of thunk */
{
    register NODE **ipsp;  	/* next free position on the postfix stack */
    register long x;		/* input record */
    register NODE *p;		/* current tree node */
    long	strwords;	/* # 4-byte words in string */
    register long i;
    long	lineno;		/* source line number for messages */
    char	filename[MAXSTRBYTES];	/* source file name for messages */
    char	buffer[MAXSTRBYTES];	/* buffer for construction of FTEXT */
    NODE	localnode;	/* local scratch node */
    C1DATA	c1nodesseen;
    long	arrayoffset;
    long	structoffset;
    HVREC	*hvlist = NULL, *hvtail;	/* hidden var list */

    /* set callee FILE descriptor to location in input file */
    fflush(ofd);
    ifd = ithunkfd;
    fseek(ifd, fileoffset, 0);

    /* initialize callee data structures */
    freeipnp = ipnp;		/* initialize ip name pool */
    ipfreenode = ipnode;	/* initialize ip tree node storage */

    ipsp = ipstack;		/* postfix expression stack is empty */

    c1nodesseen.c1word = 0;	/* haven't seen anything yet */

    sprintf(buffer,"# begin thunk\0");
    puttext(buffer);		/* emit comment as FTEXT */

    initthunklabels();	/* set up for mapping labels, if needed */

    /* read nodes and in-line the thunk !!! */
    for(;;){
	x = rdlong();		/* get input record */

#ifdef DEBUGGING
	if( xdebugflag ) fprintf( stdout, "\nop=%s., val = %d., rest = 0%o\n",
			xfop(FOP(x)), VAL(x), (int)REST(x) );
#endif
	switch( (int)FOP(x) ){  /* switch on opcode */

	case 0:			/* should never see this */
	    fprintf( stdout, "null opcode ignored\n" );
	    break;

	case C0TEMPASG:
	    putlong(x);
	    break;

	case ARRAYREF:
	    c1nodesseen.c1flags.isarray = 1;
	    c1nodesseen.c1flags.iselement = VAL(x);
	    arrayoffset = rdlong();
	    break;

	case STRUCTREF:
	    if ( c1nodesseen.c1flags.isarray )
	      fatal("Active ARRAYREF at time of STRUCTREF");
	    c1nodesseen.c1flags.isstruct = 1;
	    c1nodesseen.c1flags.isprimaryref = VAL(x);
	    if ( VAL(x) )
	      structoffset = rdlong();
	    else
	      structoffset = 0;
	    break;

	case NAME:		/* static name leaf node */
	    p = getipnode();
	    p->in.c1data.c1word = 0;
	    p->in.op = NAME;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = 0;
	    if( VAL(x) )
		{
		p->tn.lval = rdlong();
		}
	    else p->tn.lval = 0;
	    strwords = rdstring();
	    p->in.name = addtoipnp(strbuf, strwords);
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;

	case ICON:		/* constant or static name leaf node */
	    p = getipnode();
	    p->in.c1data.c1word = 0;
	    p->in.op = ICON;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = 0;
	    p->tn.lval = rdlong();
	    if( VAL(x) )
		{
		strwords = rdstring();
	        p->in.name = addtoipnp(strbuf,strwords);
		}
	    else p->in.name = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}

bump:
	    *ipsp++ = p;		/* push node on postfix stack */
	    if( ipsp >= &(ipstack[ipmaxstack]) )  /* stack is full */
		{
		NODE **ipsptemp = ipsp;
		reallocipstack(&ipsptemp);    /* reallocate callee expr stack */
		ipsp = ipsptemp;
		}
    	    break;

	case GOTO:
	    if( VAL(x) )
		{
		/* unconditional branch */
	        putlong(x);
		putlong(newthunklabel(rdlong()));
	        break;
		}
	    else   /* treat as unary */
		goto def;

	case SETREGS:
	    fatal("SETREGS in inlinethunk()");

	case REG:		/* register leaf node */
	    p = getipnode();
	    p->in.c1data.c1word = 0;
	    p->in.op = REG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = VAL(x);
	    p->tn.lval = 0;
	    p->in.name = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;

	case OREG:		/* register + offset leaf node */
	    p = getipnode();
	    p->in.c1data.c1word = 0;
	    p->in.op = OREG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = VAL(x);
	    p->tn.lval = rdlong();
	    p->in.name = NULL;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;
	    
	case C1SYMTAB:
	    if (VAL(x) == 1)
	    {
	    farg_high = rdlong();
	    farg_low = rdlong();
	    farg_pos = rdlong();
	    }
	    break;
	    
	case C1NAME:		/* copy in FTN, shoudn't see in C */
	    if (!ftnflag) fatal("Unexpected C1NAME in cpass2");
	    p = &localnode;
	    p->in.op = NAME;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    puttype(FOP(x), VAL(x), p->in.type, p->in.tattrib);
	    p->tn.rval = 0;
	    if ( VAL(x) )
	      {
	      p->tn.lval = rdlong();
	      putlong(p->tn.lval);
	      }
	    else
	      p->tn.lval = 0;
	    strwords = rdstring();
	    putstring(strbuf);
	    x = rdlong();
	    putlong( x );
	    if ( x & C1ARY )
	      putlong( rdlong() );
	    strwords = rdstring();
	    putstring(strbuf);
	    break;

	case C1OREG:
	    p = &localnode;
	    p->in.op = OREG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.lval = rdlong();
	    puttype(FOP(x), VAL(x), p->in.type, p->in.tattrib);
	    putlong( p->tn.lval );
	    x = rdlong();
	    putlong( x );
	    if ( x & C1ARY )
	      putlong( rdlong() );
	    strwords = rdstring();
	    putstring(strbuf);
	    break;

	case C1HIDDENVARS:
	        {
	        int i,j,r,v;
		HVREC *t;
    
	        j = REST(x);
	        for (i=0; i < j; ++i)
	          {
	          x = rdlong();
		  r = REST(x);
		  v = VAL(x);
	          if (FOP(x) == C1HVOREG)
		    {
		    x = rdlong();
		    t = gethvrec(C1HVOREG, x, r, v, NULL);
		    }
	          else if (FOP(x) == C1HVNAME)
		    {
		    x = rdlong();
		    strwords = rdstring();
		    t = gethvrec(C1HVNAME, x, r, v, strbuf);
		    }
	          else
		    fatal("unexpected opcode in C1HIDDENVARS");
		  if ( hvlist == NULL )
		    {
		    hvlist = t;
		    hvlist->cnt = j;
		    }
		  else
		    hvtail->next = t;
		  hvtail = t;
	          }
	        }
	        break;

	case CALL:
	case UNARY CALL:
	        p = getipnode();
	        p->in.op = FOP(x);
	        gettype(x, &p->in.type, &p->in.tattrib);
		p->in.rall = VAL(x);
	        p->in.c1data.c1word = 0;
		p->in.name = (char *) hvlist;
		hvlist = NULL;
		if ( FOP(x) == CALL )
	          p->in.right = *--ipsp;
	        p->in.left = *--ipsp;
		c1nodesseen.c1word = 0;
	        goto bump;
		break;

	case C1OPTIONS:
	case VAREXPRFMTDEF:
	case VAREXPRFMTREF:
	    break;
	    
	case VAREXPRFMTEND:
	    sprintf(buffer,"# end thunk\0");
	    puttext(buffer);
	    ifd = isrcfd;
	    return;
	    break;

	case FORCE:
	    goto def;

	case FTEXT:
	    strwords = VAL(x);
	    rdnlongs(strwords);
	    puttriple(FTEXT,REST(x),strwords);
	    putnlongs(strwords);
	    break;

	case FEXPR:			/* end of expression marker */
	    lineno = REST(x);
	    if( strwords = (VAL(x)) )	/* there is a filename -- read it */
		{
		rdnlongs( strwords );
		strbuf[strwords*4] = '\0';	/* guarantee a NULL */
		strcpy(filename, strbuf);
		}
	    if( ipsp == ipstack )   /* filename only -- emit FTEXT unchanged */
		{
		putlong(x);
		if (strwords)
		    putnlongs(strwords);
		break;
		}

	    if( --ipsp != ipstack )
		{
		fatal( "expression poorly formed (inlinethunk)" );
		}
	    p = ipstack[0];		/* root of expression */

	    /* emit tree unchanged.  We don't call fixtree() because all
	     * variable references are already in caller space.
	     */
	    fixthunklabels( p );	/* map labels */
	    puttree(p,0);
	    putlong(x);		/* emit FEXPR node, too */
	    if (strwords)
		{
		strcpy(strbuf,filename);
		putstring(strbuf);
		}
	    freeiptree(p);		/* recover node and name pool storage */
	    break;

	case PLUS:
	    p = getipnode();	/* save interior node of tree */
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.right = *--ipsp;	/* top on postfix stack */
	    p->in.left = *--ipsp;	/* next on postfix stack */
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;		/* store this node on stack */

	case UNARY MUL:
	    p = getipnode();	/* save interior node of tree */
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.left = *--ipsp;	/* top on postfix stack */
	    p->tn.rval = 0;
	    p->in.c1data.c1word = 0;
	    if (c1nodesseen.c1word)	/* C1 data for this node */
		{
		p->in.c1data.c1word = c1nodesseen.c1word;
		if (c1nodesseen.c1flags.isarray)
		    p->in.arraybase = arrayoffset;
		else if (c1nodesseen.c1flags.isstruct)
		    p->in.arraybase = structoffset;
		c1nodesseen.c1word = 0;
		}
	    goto bump;		/* store this node on stack */

	case LABEL:			/* relocate label */
	    {
	    long lab = (long) (((x) >> 8) & 0xffffff);
	    x = (newthunklabel(lab) << 8) | LABEL;
	    putlong(x);
	    }
	    break;

	case FSWITCH:
	    fatal( "switch not yet done" );
	    break;

	case FCMGO:
	    fatal( "FCMGO seen in inlinethunk()" );

	case SWTCH:
	    fatal( "SWTCH seen in inlinethunk()" );

	case FLBRAC:			/* beginning of procedure */
	    fatal( "FLBRAC seen in inlinethunk()" );

	case FRBRAC:			/* end of procedure */
	    fatal( "FRBRAC seen in inlinethunk()" );
	    
	case STASG:
	    fatal( "STASG seen in inlinethunk()" );

	case FICONLAB:
	    fatal( "FICONLAB seen in inlinethunk()" );

	case FENTRY:
	    fatal( "FENTRY seen in inlinethunk()" );

	default:			/* interior OP node */
def:
	    p = getipnode();
	    p->in.c1data.c1word = 0;
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);

	    switch( optype( p->in.op ) ){

	    case BITYPE:
		p->in.right = *--ipsp;
		p->in.left = *--ipsp;
		goto bump;

	    case UTYPE:
		p->in.left = *--ipsp;
		p->tn.rval = 0;
		goto bump;

	    case LTYPE:
		sprintf(warnbuff, "illegal leaf node (inlinethunk): %d", p->in.op );
		fatal(warnbuff);
	    }
	}
    }
}  /* inlinethunk */



# define MAXTHUNKLABS 30
# define MAXTHUNKBYTES ( MAXTHUNKLABS * sizeof( long ) )

long nextthunklab;	/* next slot avail. in label map array */
long thunklabsize;	/* current size of label map array */
long *thunklabs = NULL;	/* thunk label map array */

/********************************************************************
*
*   initthunklabels - initialize thunk label mapping
*/
initthunklabels()
{
  if ( thunklabs == NULL )
    {
    thunklabsize = MAXTHUNKLABS;
    thunklabs = (long *) malloc( MAXTHUNKBYTES );
    if ( thunklabs == NULL )
      fatal("Out of memory in initthunklabels()");
    }
  memset(thunklabs, (char)0, (thunklabsize*sizeof(long)));  /* zero init */
  nextthunklab = 0;
}

/********************************************************************
*
*   newthunklabel - get mapped thunk label value
*/
long newthunklabel( n )
	register long n;
{
  register long *l = thunklabs;
  register long *e = thunklabs + nextthunklab;
  long m;

  while ( l < e )
    if ( n == *l )
      return( *(l + 1) );	/* already mapped */
    else
      l += 2;
				/* add new mapped entry */
  if ( nextthunklab > thunklabsize )
    {
    thunklabs = (long *) realloc( (thunklabsize+MAXTHUNKLABS)*sizeof(long) );
    if ( thunklabs == NULL )
      fatal("Out of memory in newthunklabel()");
    memset( thunklabs+thunklabsize, (char) 0, MAXTHUNKBYTES );
				/* zero init only new portion */
    thunklabsize += MAXTHUNKLABS;
    }
  *(thunklabs + nextthunklab++) = n;
  m = nxtlabel++;
  *(thunklabs + nextthunklab++) = m;
  *(labarray + nxtfreelab++) = m;	/* update caller's label set */
  return( m );
}

/********************************************************************
*
*   fixthunklabels - map thunk labels in expression tree
*/

fixthunklabels( t )
	register NODE *t;	/* input tree */
{
    register NODE *r;	/* right child of top node */
    register short opty;

    if ( t->in.op == CBRANCH )
        if ((r = t->in.right)->in.op == ICON)
	    {
	    r->tn.lval = newthunklabel(r->tn.lval);
	    fixthunklabels(t->in.left);
	    return;
	    }

    /* recursively descend tree */
    opty = optype(t->in.op);
    if( opty != LTYPE ) fixthunklabels( t->in.left );
    if( opty == BITYPE ) fixthunklabels( t->in.right );
    return;
}


/********************************************************************
*
*    gethvrec - get a hiddenvars record, and set it for input parms
*/
HVREC *gethvrec( op, o, r, v, s )
	short op;
	long o;
	char *s;
{
  HVREC *rec;
  char *t;
  int n;

  t = NULL;
  if ( s != NULL )
    if ( *s != '\0' )
      {
      n = strlen( s );
      n++;
      t = (char *) malloc( n );
      if ( t == NULL )
	fatal("Out of memory in gethvrec(), 1");
      strcpy( t, s );
      }
  rec = (HVREC *) malloc( sizeof( HVREC ) );
  if ( rec == NULL )
    fatal("Out of memory in gethvrec(), 2");
  rec->next = NULL;
  rec->cnt = 0;
  rec->op = op;
  rec->rest = r;
  rec->val = v;
  rec->off = o;
  rec->str = t;
  return( rec );
} /* gethvrec */


/********************************************************************
*
*    mapc1hvoreg - map the c1hvoreg offset passed, returning
*		   the appropriate hiddenvars record
*/

HVREC *mapc1hvoreg( r, v, o )
	register long o;	/* c1hvoreg offset to be mapped */
{
	int argno;
	register NODE *p;
	char *s;
	short op;
	C1DATA adat;

    op = C1HVOREG;
    s = NULL;
    if (o < minargoffset)
      o -= tempbase;			/* local stack variable */
    else
      if (o >= minfnoffset)			/* ftn result */
	o = retoffset + (o - minfnoffset);
      else	 				/* formal arg */
	{
	argno = -((o - minfnoffset) / 4) - 1;	/* argument number */
	p = arg[argno];				/* actual argument */
	adat = p->tn.c1data;
	if (p->in.op == OREG)			/* actual is parameter too */
	  o = p->tn.lval;
	else if (p->in.op == PLUS)		/* actual is stack var */
	  o = p->in.right->tn.lval;
	else if (p->in.op == ICON)		/* actual is static var */
	  {
	  op = C1HVNAME;
	  o = p->tn.lval;
	  s = p->tn.name;
	  }
        if ( adat.c1flags.isstruct ) 		/* actual is struct */
          {
          r = 1;
          v = 0;
          }
        else if ( r )				/* formal is struct */
          fatal( "Unexpected struct formal in mapc1hvoreg()" );
        else if ( v )				/* formal is array */
          {
          if ( adat.c1flags.isarray ) 	/* array actual */
	      {
	      v = 1;
	      r = 0;
              }
            else
              {				/* scalar actual */
	      fatal( "Unexpected scalar actual in mapc1hvoreg()" );
              }
          }
        else
          {					/* formal is scalar */
          if ( adat.c1flags.isarray )
            { 				/* array actual */
            v = 1;
            r = 0;
            }
          else
            {				/* scalar actual */
            r = 0;
            v = 0;
            }
          }
        }
  return( gethvrec( op, o, r, v, s ) );
} /* mapc1hvoreg */



/********************************************************************
*
*    newc1data - returns the appropriate c1data info for
*		 given formal and actual parm substitution
*/

char secondaryref[] = "Secondary structure reference in newc1data()";
char illegaltypes[] = "Illegal parm types for substitution in newc1data()";


C1DATA newc1data( f, a )
	C1DATA f;	/* formal arg's c1data attributes */
	C1DATA a;	/* actual arg's c1data attributes */
{
  C1DATA ret;		/* returned value */

  ret.c1word = 0;	/* default return, null c1data */

  if (   ( f.c1flags.isstruct && !f.c1flags.isprimaryref )
      || ( a.c1flags.isstruct && !a.c1flags.isprimaryref ) )
    {				/* can't have secondary struct refs */
    fatal( secondaryref );
    }

  if ( a.c1flags.isstruct )			/* STRUCT actual */
    {
    ret.c1flags.isstruct = YES;
    ret.c1flags.isprimaryref = YES;
    return( ret );
    }

  if ( f.c1flags.isstruct )
    {						/* STRUCT formal */
    fatal( illegaltypes );
    }
  else if ( f.c1flags.isarray )
    {
    if ( f.c1flags.iselement )
      {						/* ARRAY ELEM formal */
      if ( a.c1flags.isarray )
        {				/* whole array or array elem actual */
	ret.c1flags.isarray = YES;
	ret.c1flags.iselement = YES;
        }
      else
        {				/* scalar actual */
	fatal( illegaltypes );
        }
      }
    else
      {						/* WHOLE ARRAY formal */
      if ( a.c1flags.isarray )
        {		 	/* whole array or array elem actual */
	ret.c1flags.isarray = YES;
        }
      else
        {				/* scalar actual */
	fatal( illegaltypes );
        }
      }
    }
  else
    {						/* SCALAR formal */
    if ( a.c1flags.isarray )
      { 			/* whole array or array elem actual */
      ret.c1flags.isarray = YES;
      ret.c1flags.iselement = YES;
      }
    else
      {					/* scalar actual */
      /* do nothing, use null default */
      }
    }
  return( ret );
} /* newc1data */



/********************************************************************
*
*    formatc2 - format peephole optimizer, C2, comment/FTEXT volatile
*		flag
*/
int formatc2(off,siz)
	long off, siz;
{
	int i;
	sprintf( strbuf, C2PATTERN, off, siz );
	siz = strlen(strbuf);
	for (i=0; i<sizeof(long); i++) strbuf[siz+i] = '\0';
	return ( (siz + sizeof(long) - 1) / sizeof(long) );
} /* formatc2() */



/********************************************************************
*
*    mapvolatile - as necessary, map the offset which is used by the
*		peephole optimizer, C2, to flag volatile operand space
*		AND output the possibly modified FTEXT record
*
*	assumes - !strncmp(strbuf,"#7",2)
*/

mapvolatile( x )
	long x;
{
	long off, siz;
	register int wdlen;

	if ( sscanf( strbuf, C2PATTERN, &off, &siz ) == 2 )
	  {		/* matched the local var opd form, do mapping */

	  if ((!ftnflag) && (off > 0))
		return;				/* C formal parameter */
	  else if (!ftnflag)
		off -= tempbase;
	  else if (off >= mindimoffset)		/* dimension info location */
		off -= tempbase - minargoffset + mindimoffset;
	  else if (off >= minfnoffset)		/* fn result area */
		off = retoffset + (off - minfnoffset);
	  else if (off >= minargoffset)		/* formal parameter */
		return;				/*   THROW IT AWAY */
	  else					/* not a formal parameter */
	        off -= tempbase;		/*   relocate stack variable */

	  wdlen = formatc2( off, siz );
	  }
	else			/* copy thru unchanged */
	  wdlen = VAL(x);
	puttriple(FTEXT, REST(x), wdlen);
	putnlongs(wdlen);
} /* mapvolatile */



/* typeszary and typesz are modified from versions in reader.c of the backend */

short typeszary[] = {	0,	/*UNDEF*/
			SZINT,	/* FARG */
			SZCHAR,	/* CHAR */
			SZSHORT, /* SHORT */
			SZINT,	/* INT */
			SZLONG,	/* LONG */
			SZFLOAT, /* FLOAT */
			SZDOUBLE, /* DOUBLE */
		       -1,	/* STRTY */
		       -1,	/* UNIONTY */
			0,	/* ENUMTY */
			0,	/* MOETY */
			SZCHAR,	/* UCHAR */
			SZSHORT, /* USHORT */
			SZINT,	/* UNSIGNED */
			SZLONG,	/* ULONG */
			0,	/* VOID */
			0,	/* LABTY */
			SZINT,	/* SIGNED */
			0,	/* CONST */
			0,	/* VOLATILE */
			0,	/* TNULL */
		 };

/* typesz() - return the size (in bits) of the REG or SCONV expr node
*		if it is known for sure, otherwise return zero
*
*/
	
long typesz( p )
	NODE *p;
{
  TWORD t;
  long s;
  
  t = p->in.type;
  if ( ISPTR(t) )
    s = SZPOINT;
  else if ( ISARY(t) || ISFTN(t) )
    s = 0;
  else
    {
    t = BTYPE(t);
    s = typeszary[ t ];
    if ( s < 0 )
      s = p->stn.stsize * SZCHAR;
    }
  return (s);
}
