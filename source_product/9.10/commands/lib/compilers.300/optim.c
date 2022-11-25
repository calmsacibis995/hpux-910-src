/* SCCS optim.c    REV(64.1);       DATE(92/04/03        14:22:15) */
/* KLEENIX_ID @(#)optim.c	64.1 91/08/28 */
/* file optim.c */

#ifdef BRIDGE
# include "bridge.I"
#else /* BRIDGE */
# include "mfile1"
# include "messages.h"
#endif /* BRIDGE */

#ifdef IRIF
       static paren_found = 0;
#endif /* IRIF */

# define SWAP(p,q) {sp=p; p=q; q=sp;}
#ifdef ANSI
# define LCON(p) (p->in.left->in.op==ICON||p->in.left->in.op==FCON)
# define RCON(p) (p->in.right->in.op==ICON||p->in.right->in.op==FCON)
# define LNONAME(p) ((p->in.left->in.op != ICON)||(p->in.left->tn.rval == NONAME))
#else /*ANSI*/
# define RCON(p) (p->in.right->in.op==ICON)
# define LCON(p) (p->in.left->in.op==ICON)
#endif /*ANSI*/
# define RO(p) p->in.right->in.op
# define RV(p) p->in.right->tn.lval
# define LO(p) p->in.left->in.op
# define LV(p) p->in.left->tn.lval






	/* mapping relationals when the sides are reversed */
short revrel[] ={ EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };

/* flag to disable constant folding for "||","&&","?:","," operations */
#ifdef BRIDGE
int fold_constants = 0;
#else /*BRIDGE*/

#ifdef ANSI
int fold_constants = 1;
#endif /*ANSI*/

#endif /*BRIDGE*/

#ifdef IRIF
NODE *do_optim( p ) register NODE *p; {
     p = optim( p );
     if( paren_found ) {
	  paren_found = 0;
	  walkf( p, rm_paren );
     }
     return( p );
}
#endif /* IRIF */

#pragma OPT_LEVEL 1
NODE * optim(p) register NODE *p;
{
	/* local optimizations, most of which are probably machine independent */

	register short o = p->in.op;
	register short ty;
	register int i;
	NODE *sp,*l,*r;

#ifdef IRIF
	/* preliminary checks */
	switch( o ){

	    case FLD:
	      /* get rid of this */
	      p->in.op = FREE;
	      return( optim( p->in.left ));

	    case PAREN:
	      paren_found++;
	      break;
	 }
#endif /* IRIF */

#if defined(ANSI) || defined(BRIDGE)
	ty = optype( o );

	/* check for constant folding */
	switch(o) {
	    case QUEST:
	    case OROR:
	    case ANDAND:
	    case COMOP:{int saved_fold_state = fold_constants;
			p->in.left = optim(p->in.left);
			fold_constants = 0;
			p->in.right = optim(p->in.right);
			fold_constants = saved_fold_state;
			break;
			}
	    default:
		switch(ty) {
		    case LTYPE:	return(p);
		    case BITYPE: r = p->in.right = optim(p->in.right);
				 /* fall through */
		    case UTYPE:	l = p->in.left = optim(p->in.left);
#ifndef BRIDGE
				r = (ty==BITYPE)?p->in.right:p->in.left;
				if (!LCON(p) || (ty==BITYPE&&!RCON(p)) || !fold_constants)
					break;
#ifdef LINT
				if (hflag && (o == CBRANCH) && LCON(p))
					/* "constant in conditional context" */
					WERROR( MESSAGE( 24 ));
#endif
				if (conval(l,o,r)) {
				    if (l != r) r->tn.op = FREE;
				    l = makety(l,p->tn.type,p->fn.cdim,
					       p->fn.csiz,p->in.tattrib);
#ifdef C1_C
#ifndef IRIF
				    /* retain arrayref tag in cases like a[0] */
				    if (p->in.fixtag) l->in.fixtag = 1;
#endif /* not IRIF */
#endif /* C1_C */
				    p->in.op = FREE;
#ifdef LINT
				    switch(o){
					case LT:
					case GT:
					case GE:
					case LE:
					case ULT:
					case UGT:
					case UGE:
					case ULE:
					case NE:
					case EQ:
					case ANDAND:
					case OROR:
						/* "constant in conditional context" */
						WERROR( MESSAGE ( 24 ));
						break;
					case NOT:
						/* "constant argument to NOT" */
						WERROR( MESSAGE ( 22 ));
						break;
					default: break;
					      }
#endif
				    return(optim(l));
				  }
#endif /*BRIDGE*/
		      break;
			}
		}
#else /*ANSI || BRIDGE*/
	ty = (short)optype( o );
	if( ty == LTYPE ) return(p);
	if( ty == BITYPE ) p->in.right = optim(p->in.right);
	p->in.left = optim(p->in.left);

	/* check for constant folding */
	{ 
	  l = p->in.left;
	  r = p->in.right;
	  
	  switch(ty)
		{
		case UTYPE:
			r = l;
			/* fall thru */
		case BITYPE:
			if (l->tn.op != ICON || r->tn.op != ICON) break;
	  		if (conval(l,o,r)) {
			   if (l != r) r->tn.op = FREE;
			   l = makety(l, p->tn.type, p->fn.cdim, p->fn.csiz, p->in.tattrib);
#ifdef C1_C
#ifndef IRIF
			   /* retain arrayref tag in cases like a[0]  */
			   if (p->in.fixtag) l->in.fixtag = 1;
#endif /* not IRIF */
#endif  /* C1_C */
			   p->in.op = FREE;
			   return (optim(l));
			}
			break;
		}
	}
#endif /*ANSI || BRIDGE*/

	/* Collect constants and do some machine-dependent strength
	   reductions.
	*/

	switch(o)
	{
	case MUL:
	case PLUS:
	case OR:
	case AND:
	case ER:
#ifdef IRIF
	case INDEX:
#endif /* IRIF */
		/* commutative ops; for now, just collect constants */
		if( nncon(p->in.left) || ( LCON(p) && !RCON(p) ) )
			SWAP( p->in.left, p->in.right );

	}

	switch(o)
	{

#ifndef BRIDGE
	case SCONV:
	case PCONV:
		return( clocal(p) );

	case UNARY MUL:
		if( (LO(p) != ICON) || ISPTR(p->in.left->tn.type) ) break;
		LO(p) = NAME;
#ifndef IRIF
		goto setuleft;

	case UNARY AND:
		if( LO(p) != NAME ) cerror( "& error" );
		LO(p) = ICON;

setuleft:
#endif /* IRIF not defined */
		/* paint over the type of the left hand side with the type of
		   the top */
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
#ifdef C1_C
#ifndef IRIF
		/* copy down ARRAYREF if U& had been tagged, but leave
		 *   NAME (now ICON) as before    */
		/* Used to copy down the tag itself, but that caused problems
		 *   if the NAME were tagged but not the U&, e.g., pushing the
		 *   address of a global array for a call.
		 * p->in.left->in.c1tag = p->in.c1tag;
		 */
		if (p->in.fixtag) p->in.left->in.fixtag = p->in.fixtag;
#endif /* not IRIF */
#endif  /* C1_C */
		p->in.op = FREE;
		return( p->in.left );
#endif /*BRIDGE*/

	case MINUS:
		if( !nncon(p->in.right) ) {
#ifdef IRIF
		     if(   ISPTR( p->in.left->in.type ) 
			&& ISPTR( p->in.right->in.type )) {
			  
			  p->in.op = PSUBP;
		     }
		     else if( ISPTR( p->in.type )) {
			  p->in.op = PSUBI;
		     }
#endif /* IRIF */
		     break;
		}
		if (RV(p) == 0) goto zapright;
		RV(p) = -RV(p);
		p->in.op = o = PLUS;
		/* fall thru */

	case PLUS:
	case OR:
	case ER:
		if (nncon(p->in.right) && RV(p) == 0) goto zapright;
		goto tower;

	case AND:
		if (nncon(p->in.right) && RV(p)==0 && !haseffects(p->in.left) )
			goto zeroout;
		goto tower;

	case MUL:
		/* zap identity operations */
		if (nncon(p->in.right))
			{
			if (RV(p) == 0 && !haseffects(p->in.left) )
				goto zeroout;
			if (RV(p) == 1) goto zapright;
			}

		/* make ops tower to the left, not the right */
tower:		 if( RO(p) == o ){
			NODE *t1, *t2, *t3;
			t1 = p->in.left;
			sp = p->in.right;
			t2 = sp->in.left;
			t3 = sp->in.right;
			/* now, put together again */
			if (sp->in.type == t1->in.type)
				{
				/* The additional restriction on type is
				   necessary to prevent code gen problems
				   mixing pointers/nonpointers, etc.
				*/
				p->in.left = sp;
				sp->in.left = t1;
				sp->in.right = t2;
				p->in.right = t3;
				}
			}
#ifdef BRIDGE
                /* don't do constant folding for bridge.  identity
                 * folding (for integers) and strength reduction should
                 * be OK.
                 */
                goto no_const_folding;
#else /*BRIDGE*/
		if(o == PLUS && LO(p) == MINUS && RCON(p) && RCON(p->in.left) &&
		  conval(p->in.right, MINUS, p->in.left->in.right)){
#endif /*BRIDGE*/
zapleft:
			RO(p->in.left) = FREE;
			LO(p) = FREE;
#ifdef C1_C
#ifndef IRIF
			/* Used to copy fixtag down if either disappearing node
			 * was set.  &(a.x[i.y])+2 was collapsing &a+2, copying
			 * fixtag down to index side:    +
			 *                              / \
			 *			       +   NONAME ICON
			 *			index /	\named ICON
			 * Allow either + or ICON node to be tagged:
			 *   *(array+strlen(array)-11)
			 */
			if (p->in.left->in.fixtag)
				{
				p->in.fixtag = 1;
				/* copy tag as well, in case it had bubbled up
				 * only as far as the lower op
				 */
				p->in.c1tag = p->in.left->in.c1tag;
				}
			else if (p->in.left->in.right->in.fixtag)
				p->in.right->in.fixtag = 1;
#endif /* not IRIF */
#endif  /* C1_C */
			p->in.left = p->in.left->in.left;
#ifdef BRIDGE
			goto no_const_folding;
#else /*BRIDGE*/
		}
		if( RCON(p) && LO(p)==o && RCON(p->in.left) && 
		    conval( p->in.right, o, p->in.left->in.right ) ){
			goto zapleft;
			}
		else if( LCON(p) && RCON(p) &&
			conval( p->in.left, o, p->in.right ) ){
#endif /*BRIDGE*/
zapright:
			tfree(p->in.right);
			p->in.left = makety( p->in.left, p->in.type, p->fn.cdim,
					     p->fn.csiz, p->in.tattrib );
			p->in.op = FREE;
#ifdef C1_C
#ifndef IRIF
			if (p->in.fixtag || p->in.right->in.fixtag)
				p->in.left->in.fixtag = 1;
#endif /* not IRIF */
#endif  /* C1_C */
#ifdef BRIDGE
			return (p->in.left);
#else /*BRIDGE*/
			return( clocal( p->in.left ) );
			}
#endif /*BRIDGE*/

		/* change muls to shifts */

		if( o==MUL && nncon(p->in.right) )
			{
			if ( !RV(p) && !haseffects(p->in.left) )
				goto zeroout;	/* multiply by 0 */
			i = RV(p);
			if (i == (i & -i)) /* an exact power of 2 */
				{
				p->in.op = o = LS;
				p->in.right->in.type = INT;
				p->in.right->fn.csiz = INT;
				RV(p) = ispow2((CONSZ)i);
				}
			else if ( oflag && ( --i, i && i == (i & -i)) )
				{
				/* 1 greater than a power of 2 */
				makepow2(p, PLUS, ispow2((CONSZ)i) );
				}
			else if ( oflag && (i += 2, i && i == (i & -i)) )
				{
				/* 1 less than a power of 2 */
				makepow2(p, MINUS, ispow2((CONSZ)i) );
				}
		      	}

		/* change +'s of negative consts back to - */
		if( o==PLUS && nncon(p->in.right) && RV(p)<0 ){
			RV(p) = -RV(p);
			p->in.op = MINUS;
			/* o = MINUS; */
			}
#ifdef IRIF
		if( p->in.op == PLUS ){
		     if( ISPTR( p->in.type )) {

			  /* pointer arithmetic */
			  if( nncon( p->in.right )) {
			       p->in.op = PADDICON;
			       if( p->in.left->in.op == PADDICON ) {
				    p->in.left->in.op = FREE;
				    p->in.left->in.right->in.op = FREE;
				    p->in.right->tn.lval += p->in.left->in.right->tn.lval;
				    p->in.left = p->in.left->in.left;
			       }
			  }
			  else {
			       p->in.op = PADDI;
			  }
		     }
		     else if( nncon( p->in.right )) {
			  p->in.op = ADDICON;
			  if( p->in.left->in.op == ADDICON ) {
			       p->in.left->in.op = FREE;
			       p->in.left->in.right->in.op = FREE;
			       p->in.right->tn.lval += p->in.left->in.right->tn.lval;
			       p->in.left = p->in.left->in.left;
			  }
		     }
		}

		if( p->in.op == MINUS ){
		     /* RHS is assumed constant */

		     if( ISPTR( p->in.type )) {
			  /* pointer arithmetic */
			  p->in.op = PSUBICON;
			  if( p->in.left->in.op == PSUBICON ) {
			       p->in.left->in.op = FREE;
			       p->in.left->in.right->in.op = FREE;
			       p->in.right->tn.lval += p->in.left->in.right->tn.lval;
			       p->in.left = p->in.left->in.left;
			  }
		     }
		     else {
			  p->in.op = SUBICON;
			  if( p->in.left->in.op == SUBICON ) {
			       p->in.left->in.op = FREE;
			       p->in.left->in.right->in.op = FREE;
			       p->in.right->tn.lval += p->in.left->in.right->tn.lval;
			       p->in.left = p->in.left->in.left;
			  }
		     }
		}
#endif /* IRIF */
 
		break;

#if defined(IRIF)
	case INDEX:
		/* array index arithmetic */
		if( nncon( p->in.right )) {
		     p->in.op = PADDICON;
		}
		else {
		     p->in.op = PADDI;
		}
		break;
#endif /* IRIF and not HAIL */

	case ASG MUL:
		if (nncon(p->in.right))
			{
			if ((i = RV(p)) == 0)	/* multiply by 0 */
				{
				p->in.op = ASSIGN;
				break;
				}
			if (i == 1) goto zapright;
			else if (i == (i & -i) )
				{
				p->in.op = ASG LS;
				RV(p) = ispow2((CONSZ)i);
				p->in.right->in.type = INT;
				p->in.right->fn.csiz = INT;
				}
			else if ( oflag && 
				((--i, i == (i & -i)) || (i += 2, i == (i & -i))) )
				{
				/*
						*=
					       /  \
					      X    const

					      becomes

						=
					       / \
					      X   *
						 / \
						X   const

					Which is then optimized in another pass.
				*/
				p->in.op = ASSIGN;
				p->in.right = block(MUL, t1copy(p->in.left),
						p->in.right, INT, 0, INT);
				p->in.right = optim(p->in.right);
				}
			}
		break;

	case ASG PLUS:
	case ASG MINUS:
		if( nncon(p->in.right) )
			if (RV(p) == 0) goto zapright;
			else if (RV(p) < 0)
				{
				RV(p) = - RV(p);
				p->in.op = (o==ASG PLUS)?
					ASG MINUS : ASG PLUS;
				}
#ifdef IRIF
		if( p->in.op == ASG PLUS ) {
		     if( ISPTR( p->in.type )) {
			  if( nncon( p->in.right )) {
			       p->in.op = ASG PADDICON; 
			  }
			  else {
			       p->in.op = ASG PADDI;
			  }
		     }
		     else {
			  if( nncon( p->in.right )) {
		               p->in.op = ASG ADDICON;
			  }
		     }
		}
		else { /* ASG MINUS */
		     if( ISPTR( p->in.type )) {
			  if( nncon( p->in.right )) {
			       p->in.op = ASG PSUBICON; 
			  }
			  else {
			       if( ISPTR( p->in.right->in.type )) {
				    p->in.op = ASG PSUBP;
			       }
			       else {
				    p->in.op = ASG PSUBI;
			       }
			  }
		     }
		     else {
			  if( nncon( p->in.right )) {
		               p->in.op = ASG SUBICON;
			  } 
		     }
		}
#endif /* IRIF */

		break;

	case DIV:
	case ASG DIV:
		if( nncon( p->in.right ) ) 
# ifndef ANSI
			if ( (i=RV(p)) == 0) UERROR( MESSAGE(31) );
# else /*ANSI*/
			if ( (i=RV(p)) == 0) {
#ifdef BRIDGE
				;
#else /*BRIDGE*/
				if (fold_constants) UERROR( MESSAGE(31) );
#endif /*BRIDGE*/
				}
# endif /*ANSI*/
			else if (i == 1 ) goto zapright;
			else
				{
				if ( i == (i & -i) ) /* exact power of 2 */
					{
					if (ISUNSIGNED(p->in.left->in.type) )
						/* remember: -1/2  integer divide = 0 */
						{
						p->in.op = (o == DIV) ?
							RS : ASG RS;
						RV(p) = ispow2((CONSZ)i);
						p->in.right->in.type = INT;
						p->in.right->fn.csiz = INT;
						}
					else
						{
						/* subvert hardops */
						p->in.op = (o==DIV)?
							DIVP2 : ASG DIVP2;
						RV(p) = ispow2((CONSZ)i);
						}
					}
				}
		break;

	case MOD:
		if ( nncon(p->in.right) )
			{
#ifndef ANSI
			if ( RV(p) == 0 ) UERROR( MESSAGE(31) );
#else /*ANSI*/
			if ( RV(p) == 0 && fold_constants)
#ifdef BRIDGE
				;
#else /*BRIDGE*/
				UERROR( MESSAGE(31) );
#endif /*BRIDGE*/
#endif /*ANSI*/
			if ( (RV(p) == 1 || RV(p) == -1)
				&& !haseffects(p->in.left) )
				{
				RV(p) = 0;
#ifndef ANSI
				goto zeroout;
#else /*ANSI*/
zeroout:
				tfree(p->in.left);
				p->in.op = FREE;
				return (p->in.right);
#endif /*ANSI*/
				}
			}
		break;

	case ASG MOD:
		if ( nncon(p->in.right) )
			{
#ifndef ANSI
			if ( RV(p) == 0 ) UERROR( MESSAGE(31) );
#else /*ANSI*/
			if ( RV(p) == 0 && fold_constants)
#ifdef BRIDGE
				;
#else /*BRIDGE*/
				UERROR( MESSAGE(31) );
#endif /*BRIDGE*/
#endif /*ANSI*/
			if ( RV(p) == 1 || RV(p) == -1)
				{
				p->in.op = ASSIGN;
				RV(p) = 0;
				}
			}
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		if( !LCON(p) ) break;

		/* exchange operands */

		sp = p->in.left;
		p->in.left = p->in.right;
		p->in.right = sp;
		p->in.op = revrel[p->in.op - EQ ];
		break;

	case ASG OR:
	case ASG ER:
	case LS:
	case RS:
	case ASG LS:
	case ASG RS:
		if (nncon(p->in.right) && RV(p) == 0 ) goto zapright;
		break;

	case ASG AND:
		/* turn "X &= 0" into "X = 0" */
		if (nncon(p->in.right) && RV(p) == 0) p->in.op = ASSIGN;
		break;

	case OROR:
#ifndef ANSI
		if ( nncon(p->in.right) )
			{
			if ( RV(p) == 0 ) goto zapright;
			if ( !haseffects(p->in.left) ) { RV(p) = 1; goto zeroout; }
			/* Not really zeroed out -- merely returning rhs */
			}
		break;
#else /*ANSI*/
		if ( nncon(p->in.left) ) {
		     if ( LV(p) != 0 ) { LV(p) = 1; goto zapright; }
		     if (fold_constants) p->in.right = optim(p->in.right);
#ifndef BRIDGE
		     if ( RCON(p) && conval(p->in.left,p->in.op,p->in.right)) {
			  p->in.right->in.op = FREE;
			  p->in.left = makety(p->in.left,p->tn.type,
					      p->fn.cdim,p->fn.csiz,
					      p->in.tattrib);
			  p->in.op = FREE;
			  return(p->in.left);
		     }
#endif /*BRIDGE*/
		}
		break;
#endif /*ANSI*/
	case ANDAND:
#ifndef ANSI
		if (nncon(p->in.right))
			{
			if ( RV(p) == 0 && !haseffects(p->in.left) )
				{
zeroout:
				tfree(p->in.left);
				p->in.op = FREE;
				return (p->in.right);
				}
			}
		break;
#else /*ANSI*/
		if (nncon(p->in.left)) {
		     if ( LV(p) == 0 ) goto zapright;
		     if (fold_constants) p->in.right = optim(p->in.right);
#ifndef BRIDGE
		     if ( RCON(p) && conval(p->in.left,p->in.op,p->in.right)) {
			  p->in.right->in.op = FREE;
			  p->in.left = makety(p->in.left,p->tn.type,
					      p->fn.cdim,p->fn.csiz,
					      p->in.tattrib);
			  p->in.op = FREE;
			  return(p->in.left);
		     }
#endif /*BRIDGE*/
		}
		break;
#endif /*ANSI*/
#ifndef ANSI
		}
#else /* ANSI */
	case QUEST:
		if (LCON(p) && LNONAME(p))
			{
			if (((p->in.left->in.op == ICON) && (LV(p) == 0 )) ||
                            ((p->in.left->in.op == FCON) &&
                                              (p->in.left->fpn.dval == 0.0))) {
				p->in.op = LO(p) = RO(p) = FREE;
				tfree(p->in.right->in.left);
				return(optim(p->in.right->in.right));
				}
			else	{
				p->in.op = LO(p) = RO(p) = FREE;
				tfree(p->in.right->in.right);
				return(optim(p->in.right->in.left));
				}
			}
	}
#endif /*ANSI*/
	return(p);
}
#pragma OPT_LEVEL 2

ispow2( c ) register CONSZ c; {
	register i;
	if (c == 0x80000000) return (31);
	else if( c <= 0 || (c&(c-1)) ) return(-1);
	for( i=0; c>1; ++i) c >>= 1;
	return(i);
	}



/* makepow2() converts a (assign) mul tree that is almost a power of two into
   a tree of left shift plus/minus the lhs.

   e.g.			*
		       / \
		      X   3  becomes

			+
		       / \
		      <<  X
		     / \
		    X   1

NOTE: If "X" has side effects, then all bets are off. Consequently we restrict
the optimization to allow only LEAF type lnodes.
*/

LOCAL makepow2(p, op2, power) register NODE *p; short op2; int power;
{

        if ( optype(p->in.left->in.op) != LTYPE) return;
	p->in.op = op2;
	p->in.right->in.op = FREE;
	p->in.right = t1copy(p->in.left);
	p->in.left = block(LS, p->in.left, bcon(power,INT), INT, 0, 0);

}

