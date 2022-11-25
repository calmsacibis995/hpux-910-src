/* file order.c */
/*	SCCS	REV(64.3);	DATE(92/04/03	14:22:16) */
/* KLEENIX_ID @(#)order.c	64.3 92/03/12 */

# include "mfile2"




# define max(x,y) ((x)<(y)?(y):(x))
# define min(x,y) ((x)<(y)?(x):(y))
# define ZCHAR 01
# define ZLONG 02
# define ZFLOAT 04
# define SAFEANUM A3-A0	/* Mininum number of STBREGS available minus 1 */

# define usefulstore(p) ((p)->in.op!=OREG || DECREF((p)->in.type) == DOUBLE)
		/* i.e. it's a node that will reduce su when stored in temp.*/

/* macro to see if an operand of an assignment is already in memory */
# define niceasop(np) (np->in.op==UNARY MUL && np->in.left->in.op==OREG)

/* macro to see if TBREG is a good destination ... not a critical resource */
# define SAFESTBREG() 	( afregs >= SAFEANUM )


deltest( p ) register NODE *p; {
	/* should we delay the INCR or DECR operation p */
	/* PDP-11 dependent stuff removed from here */

	p = p->in.left;
	if( p->in.op == UNARY MUL ) p = p->in.left;
	return( p->in.op == NAME || p->in.op == OREG || p->in.op == REG );
	}



/* setsto() is a version of SETSTO for that checks for floating point trees.  If one of
   the children is a call op, then the return value is in d0 (and
   possibly d1).  It is better to spill these registers into temporaries
   rather than floating point registers which would involve a loss of
   precision.  In these cases it is more accurate to spill a smaller
   subtree.
*/
void setsto(p)	register NODE *p;
{
	if (p->in.type == VOID)
	  stocook = FOREFF;
	else
	  stocook = INTEMP;
	if (ISFTP(p->in.type))
		switch(optype(p->in.op))
		{
		case BITYPE:
			if (callop(p->in.left->in.op)
					&& p->in.op != CM) /* precedence OK */
				{
				stotree = p->in.left;
				if (stotree->in.type == VOID) stocook = FOREFF;
				return;
				}
			else if (callop(p->in.right->in.op)
					&& p->in.op != COMOP
					&& p->in.op != CM) /* precedence OK */
				{
				stotree = p->in.right;
				return;
				}
			break;
		case UTYPE:
			if (callop(p->in.left->in.op))
				{
				stotree = p->in.left;
				return;
				}
			break;
		}
	stotree = p;
}






mkadrs(p) register NODE *p; {

	register short rsu = max(p->in.right->in.su, p->in.right->in.fsu);
	register short lsu = max(p->in.left->in.su, p->in.left->in.fsu);

	if( asgop(p->in.op) ){
		if( lsu >= rsu ){
			if( p->in.left->in.op == UNARY MUL) 
				{
				if (lsu>0 && usefulstore(p->in.left->in.left))
					SETSTO( p->in.left->in.left );
#if 0
				else if ((lsu > 0) && (rsu == 0))
					SETSTO(p->in.left);
#endif
				else
					{
					if( rsu > 0 )
						SETSTO( p->in.right );
					else
						cerror( "store finds both sides trivial" );
					}
				}
			else if((p->in.left->in.op == FLD) &&
				(p->in.left->in.left->in.op == UNARY MUL) &&
				(usefulstore(p->in.left->in.left->in.left))) {
				SETSTO( p->in.left->in.left->in.left );
				}
			else if( p->in.left->in.op == FLD
				&& p->in.left->in.left->in.op == OREG ){
				SETSTO( p->in.right );
				}
#if 0
			else { /* should be only structure assignment */
				SETSTO( p->in.left );
				}
#endif
			}
		else SETSTO( p->in.right );
		}
	else {
		if( lsu > rsu ){
			SETSTO( p->in.left );
			}
		else {
			SETSTO( p->in.right );
			}
		}
	}

/*ARGSUSED*/
notoff( r, off, cp) CONSZ off; char *cp; {
/* notoff( t, r, off, cp) TWORD t; CONSZ off; char *cp; { */
	/* is it legal to make an OREG or NAME entry which has an
	/* offset of off, (from a register of r), if the
	/* resulting thing had type t */

	if ( cp==0 && r>=A0 && r<=SP )
	  return(0);	/* YES */
	return( 1 );  /* NO */
	}


# if 0	/* revised and simplified */
zum( p, zap ) register NODE *p; {
	/* zap Sethi-Ullman number for chars, longs, floats */
	/* in the case of longs, only STARNM's are zapped */
	/* ZCHAR, ZLONG, ZFLOAT are used to select the zapping */

	register short su;

	su = p->in.su;

	if (su == 0)
		switch( p->in.type ){

		case LONG:
		case ULONG:
			if( p->in.op == UNARY MUL ) p->in.su = su = 2;
			break;
	

			}

	return( su );
	}
# endif /* 0 */






sucomp( p ) register NODE *p; {

	/* set the su field in the node to the sethi-ullman
	   number, or local equivalent */

	register short o = p->in.op;
	register short sul, sur;
	register nr;
	register TWORD type = p->in.type;
	register NODE *pr, *pl, *ptmp;
	short opty;
	short nfr;
	short fsul, fsur;
	short addn_l_nfr = 0;
	short addn_r_nfr = 0;

	opty = (short)optype( o );
	/* nr = szty( type ) */;
	nr = (type == DOUBLE) ? 2 : 1;
	nfr = ( !flibflag && ISFTP(type) && (type != LONGDOUBLE))? 1 : 0;
	/* 68881 regs are big enough to hold doubles. */
	p->in.su = 0;
	p->in.fsu = 0;

	if( opty == LTYPE )
		return;
	else 
		{
		pl = p->in.left;

		if( opty == UTYPE )
			switch( o ) {
		case UNARY CALL:
		case UNARY STCALL:
			p->in.su = fregs ;  	/* all regs needed */
			if ( ! flibflag )
			  {
			  p->in.fsu = (p->in.fpaside) ? dfregs : ffregs;
			  p->in.fsu += fcallflag;
			  fcallflag++;
			  }
				/* All f regs needed. fcallflag added to ensure
				   that fsu > ffregs for more than 1 call.
				   This forces a store later.
				*/
			callflag++;		/* this use for sucomp() only */
			return;
 
		case SCONV:
			if (nfr)
				nr = 0;
			p->in.su = max( pl->in.su, nr);
			if ( nfr && ! p->in.fpaside )
				{			/* 881 */
				p->in.fsu = pl->in.fsu;
				return;
				}
			if (!flibflag)
				p->in.fsu = max( pl->in.fsu, nfr );
			return;

		case UNARY MUL:
			if( shumul( pl ) )
			  return;
		        if ( pl->in.op == PLUS   /* poss. indexed addr mode */
				|| pl->in.op == MINUS )
			        { NODE *l, *r;
				nr = 1;
			        if ( pl->in.op == PLUS
					&& !ISPTR(pl->in.left->in.type))
				        {
				        /* reverse for canonicalization */
				        l = pl->in.right;
				        pl->in.right = pl->in.left;
				        pl->in.left = l;
				        }
			        l = pl->in.left;
			        r = pl->in.right;
			        if ( pl->in.op == PLUS && ISPTR(r->in.type)
				        && ((r->in.su < l->in.su) 
					     || offstarscaler(l)) )
				        {
				        /* swap cheapest addr reg on the left */
				        l = r;
				        pl->in.right = pl->in.left;
				        pl ->in.left = l;
				        r = pl->in.right;
				        }
			        if (r->in.op==SCONV && 
					   r->in.left->rn.su_total == 0)
				  nr++;
			        if ( (r->in.op == REG) 
				        || ((l->in.su < fregs) && 
					      offstarscaler(r)) )
				        {
					nfr = 0;
					if (pl->in.op == MINUS)
					  nr++;
					else
				          if (r->in.op == LS && 
					    r->in.left->in.op != REG)
					        nr++;
				        if (l->in.op == REG
					      /* || l->in.op == ICON */)
					        goto defaultlab;
				        if ((l->in.op == MINUS ||
						  l->in.op == PLUS)
					        && l->in.right->in.op==ICON)
					        {
					        if (l->in.left->in.op == REG)
						  goto defaultlab; 
					        else
						  nr++;
					        }
		  		        else
					  if ( l->in.op != ICON && 
					       l->in.op != NAME )
					        nr++;
				        goto defaultlab;
				        }
        
			      /* r->in.right->in.op == NAME disallowed to avoid
			       * conflict with FORTRAN static vars -- sjo
			       */
			        if ((r->in.op == PLUS || r->in.op == MINUS) &&
				     (r->in.right->in.op == ICON) &&
				        offstarscaler(r->in.left))
				        {
					nfr = 0;
					nr++;
				        if (ISPTR(l->in.type) &&
					    (l->in.op != REG ||
					       !ISBREG(l->tn.rval)) )
					  nr++;
				        goto defaultlab;
				        }
		           /* at this point we could r into a register and try
			      again but the cost of this address type is higher
			      than the cost of the simpler addressing schemes.
		            */
	        
	  		        }
			if ( nfr )
			  {
			  nr = 1;
			  if ( pl->in.op == OREG )
			    {
			    if ( pl->tn.rval < 0 )
			      {
			      union indexpacker x;
			      x.rval = pl->tn.rval;
			      if ( x.i.addressreg )
			        nr++;
			      }
			    }
			  else if ( ! daleafop( pl->in.op ))
			    nr++;
			  p->in.su = max(pl->in.su, nr);
			  p->in.fsu = pl->in.fsu;
			  return;
			  }

defaultlab:
		default:
			p->in.su = max( pl->in.su, nr);
			if (!flibflag)
				p->in.fsu = max( pl->in.fsu, nfr );
			return;
			}
		else	
			/* op is BITYPE */
			if (ISFTP(type))
				{
				pr = p->in.right;
				fsul = pl->in.fsu;
				fsur = pr->in.fsu;
				sul = pl->in.su;
				sur = pr->in.su;
				switch (o)
					{
				case PLUS:
				case MINUS:
				case MUL:
				case DIV:
				case ASG PLUS:
				case ASG MINUS:
				case ASG MUL:
				case ASG DIV:
				case EQ:
				case NE:
				case LE:
				case LT:
				case GE:
				case GT:
				case FEQ:
				case FNEQ:
				case FGT:
				case FNGT:
				case FGE:
				case FNGE:
				case FLT:
				case FNLT:
				case FLE:
				case FNLE:
				        if (p->in.fpaside) /* dragon */
				        {
					if (o==PLUS || o==MUL)
					  { /* can flip in setbin() */
					  if ( ISTDNODE( pl ) ) {
					    if ( ! ISDNODE( pr ) )
					      addn_r_nfr++;
					    }
					  else if ( ISTDNODE( pr ) ) {
					    if ( ! ISDNODE( pl ) )
					      addn_l_nfr++;
					    }
					  else if ( ISDNODE( pr ) )
					    addn_l_nfr++;
					  else if ( ISDNODE( pl ) )
					    addn_r_nfr++;
					  else
					    {
					    addn_r_nfr++;
					    addn_l_nfr++;
					    }
					  }
					else /* can not flip in setbin() */
					  {
					  if ( o & 1 ) /* ASG OP form */
					    {
				            if ( ! ISDNODE(pl) )
				              addn_l_nfr++;
					    }
					  else
				            if ( ! ISTDNODE(pl) )
				              addn_l_nfr++;
				          if ( ! ISDNODE(pr) )
				            addn_r_nfr++;
					  }
				        }
				        else		/* 881 */
				        {
					  if ( o==PLUS || o==MUL )
					    {	/* can flip in setbin() */
					    if ( ISTFNODE( pl ) )
					      {
					      if ( !FLT_EA( pr ) )
						addn_r_nfr++;
					      }
					    else if ( ISTFNODE( pr ) )
					      {
					      if ( !FLT_EA( pl ) )
						addn_l_nfr++;
					      }
					    else if (FLT_EA( pr ))
					      addn_l_nfr++;
					    else if (FLT_EA( pl ))
					      addn_r_nfr++;
					    else
					      {
					      addn_r_nfr++;
					      addn_l_nfr++;
					      }
					    }
					  else	/* can not flip in setbin() */
					    {
					    if ( o & 1 ) /* ASG OP form */
					      {
				              if ( ! ISFNODE(pl) )
				                 addn_l_nfr++;
					      }
					    else
				              if ( ! ISTFNODE(pl) )
				                 addn_l_nfr++;
				            if ( ! FLT_EA(pr) )
				              addn_r_nfr++;
					    }
				        }
				        break;
				case ASSIGN:
					if ( ! FLT_EA(pr) )
					    addn_r_nfr++;
					if ( ! FLT_EA(pl) )
					    addn_l_nfr++;
					break;
				case FMONADIC:
					if ( p->in.fpaside )
					  {  /* dragon side exprs assume 
						that 881's F0 is available
						for 881-only fmonadics */
					  if ( ! NOT_DRAGON_FMONADIC(pl))
					    if ( ! ISTDNODE( pr ) )
					      addn_r_nfr++;
					  }
					else
					  if ( ! ISTFNODE( pr ) )
					    addn_r_nfr++;
					break;
				case CALL:
				case STCALL:
					/* in effect, takes all free regs */
					p->in.fsu = (p->in.fpaside) ? 
								dfregs : ffregs;
					p->in.fsu += fcallflag;
					fcallflag++;
					p->in.su = fregs;
					callflag++; /* this use in sucomp only*/
					return;
				case QUEST:
				case COLON:
				case COMOP:
					p->in.fsu = max( max(fsul, fsur),
								nfr );
					p->in.su = max(sul, sur);
					return;
				} /* switch */
			} /* if ISFTP */
		} /* if BITYPE */

	/* get here for binary operators */

	/* If rhs needs n, lhs needs m, regular su computation */

	pr = p->in.right;
	sul = pl->in.su;
	sur = pr->in.su;
	fsul = max( pl->in.fsu, addn_l_nfr );
	fsur = max( pr->in.fsu, addn_r_nfr );
	if (nfr)		/* flt rewrites take precedence */
	  {
	  nr = 0;		/* may not need address reg(s) */
	  if (fsur > fsul)
	    {			/* evaluate flts right side first */
	    if ( pr->in.op == OREG ) {
	      if ( pr->tn.rval < 0 ) {	/* indexed addr modes */
	        union indexpacker x;
	        x.rval = pr->tn.rval;
	        if ( ISTREG( x.i.addressreg ) ) 
			nr++;
	        if ( ISTREG( x.i.xreg ) ) 
			nr++;
	        }
	      else
		if ( ISTREG( pr->tn.rval ) )
		  nr++;
	      }
	    else 
		if ( ! daleafop( pr->in.op )) 
			nr++;
	    if ( addn_r_nfr )
	      { /* if we req'r flt scratch reg for right side opd, we 
		   don't need right address reg(s) while doing left side */
	      if ( pr->in.fpaside )
		{
		if ( !callop(pr->in.op) )
		  nr = 0;
		}
	      else 
		if ( pr->in.type == DOUBLE )
		  nr = 0;
	      }
	    if ( callop (pr->in.op) )
	      {
	      if ( nr < 1 )
	        nr = 1;	/* need a data reg to hold float result */
	      if ( nr < 2 && pr->in.type == DOUBLE )
	        nr = 2;	/* need two data regs to hold double result */
	      }
	    p->in.fsu = max( fsul+nfr, fsur );
	    p->in.su = max( sul+nr, sur );
	    }
	  else			/* evaluate flts left side first */
	   {
	    if ( pl->in.op == OREG ) {	/* check for two reg addrs */
	      if ( pl->tn.rval < 0 ) {
	        union indexpacker x;
	        x.rval = pl->tn.rval;
	        if ( ISTREG( x.i.addressreg ) )
			nr++;
	        if ( ISTREG( x.i.xreg ) )
			nr++;
	        }
	      else
	        if ( ISTREG( pl->tn.rval ) )
		  nr++;
	      }
	    else
		if ( ! daleafop( pl->in.op )) 
			nr++;
	    if ( addn_l_nfr )
	      if ( pl->in.fpaside )
		{
		if ( !callop(pl->in.op) )
                  {
                  if ((p->in.op == ASSIGN) &&
                      (pl->in.op == UNARY MUL) &&
                      ((pl->in.left->in.op == PLUS) ||
                       (pl->in.left->in.op == MINUS)))
                        nr = 1;
                  else
                        nr = 0;
                  }
		}
	      else 
		if ( pl->in.type == DOUBLE )
		  {
		  if ((p->in.op == ASSIGN) &&
		      (pl->in.op == UNARY MUL) &&
		      ((pl->in.left->in.op == PLUS) ||
		       (pl->in.left->in.op == MINUS)))
			nr = 1;
		  else
			nr = 0;
		  }
	    if ( callop (pl->in.op) )
	      {
	      if ( nr < 1 )
	        nr = 1;	/* need a data reg to hold float result */
	      if ( nr < 2 && pl->in.type == DOUBLE )
	        nr = 2;	/* need two data regs to hold double result */
	      }
	    p->in.fsu = max( fsul, fsur+nfr );
	    p->in.su = max( sul, sur+nr );
	    }
	  return;
	  }
	else		/* not flt op node, regular su precedence */
	  {
	          /* use worst case default for fsu, and compute su as before */
	  p->in.fsu = max( fsul, fsur );
	  }

	if (o == ASSIGN )
		{
asop:		  /* also used for +=, etc., to memory */
		if( pl->rn.su_total==0 ){
			/* don't need to worry about the left side */
			p->in.su = max( sur, nr );
			}
		else {
			/* right, left address, op */
			if( pr->rn.su_total == 0 ){
				/* just get the lhs address into a register, and mov */
				/* the `nr' covers the case where value is in
					reg afterwards */
				p->in.su = max( sul, nr );
				}
			else if ( callflag==0 && (niceasop(pl) || niceasop(pr)) )
				/* Calls are a special case. They really take
				   fregs+1 registers (a0 is also used) but in
				   most circumstances other than ASSIGN it is
				   not necessary to store the call result which
				   is the effect of su=fregs+1.
				*/
				p->in.su = max(sul, sur);
			else {
				/* right, left address, op */
				p->in.su = max( sur, nr+sul );
				}
			}
		return;
		}

	if (o == STASG)
		{
		p->in.su = max( min( max(sul+nr,sur), max(sul,sur+nr) ),
				fregs );
		return;
		}

	if( logop(o) ){
		/* do the harder side, then the easier side, into registers */
		/* left then right, max(sul,sur+nr) */
		/* right then left, max(sur,sul+nr) */
		/* to hold both sides in regs: nr+nr */
		/* nr = szty( pl->in.type ); */
		nr = (pl->in.type == DOUBLE) ? 2 : 1;
		sul = pl->in.su;
		sur = pr->in.su;
		p->in.su = min( max(sul,sur+nr), max(sur,sul+nr) );
		return;
		}

	if( asgop(o) ){
		/* computed by doing right, doing left address, doing left, op,
		   and store */
		switch( o ) {
		case INCR:
		case DECR:
			/* do as binary op */
			break;

		case ASG ER:
		case ASG OR:
		case ASG AND:
			if (o==ASG ER)
			  { if ( ! (pr->in.op == REG && ISAREG(pr->tn.rval))
			        && !(pr->in.op == ICON || pr->in.op == NAME))
				/* 'eor' on 68020 requires source data reg */
				/* 'eori' on 68020 requires source constant */ 
			      nr++;
			  }
			else
			  if (pr->in.op == REG && ISBREG(pr->tn.rval))
			  /* 'or' & 'and' on 68020 requires non-BREG source */
			    nr++;
			if( type == INT || type == UNSIGNED || ISPTR(type)
				|| ISFTP(type) )
				goto asop;
			if( pr->rn.su_total==0 && pl->rn.su_total != 0 )
			  {
			  p->in.su = max( sul, nr );
			  return;
			  }
			goto gencase;
		case ASG MUL:
		case ASG PLUS:
		case ASG MINUS:
			if( type == INT || type == UNSIGNED || ISPTR(type)
				|| ISFTP(type) )
				goto asop;
			goto gencase;

		case ASG MOD:
			ptmp = (pr->in.op == SCONV) ? pr->in.left : pr;
			if (ptmp->rn.su_total != 0) nr++;
			if (tsize2(p->in.type) > 4) sul++;
			goto gencase;

		case ASG DIVP2:
			sul++;	/* Integer div by 2 needs an extra AREG */
			goto gencase;
		default:
gencase:
			sur = pr->in.su;
			if( pr->rn.su_total==0 ){ /* easy case: if addressable,
					do left value, op, store */
				if( pl->rn.su_total == 0 ) p->in.su = nr;
				/* harder: left adr, val, op, store */
				else p->in.su = max( sul, nr+1 );
				}
			else { /* do right, left adr, left value, op, store */
				if( pl->rn.su_total == 0 ){ 
					   /* right, left value, op, store */
					p->in.su = max( sur, nr+nr );
					}
				else {
					/* replaced by Chris Aoki's theory 
					p->in.su = max( sur, max( sul+nr, 1+nr+nr ) );
					*/
					p->in.su = max(sur, max(sul+nr,nr+nr));
					}
				}
			return;
			}
		}

	  switch (o)
		{
		case CALL:
		case STCALL :
			{
			/* in effect, takes all free registers */
			p->in.su = fregs;
			if ( flibflag )
			  p->in.fsu = 0;
			else
			  {
			  p->in.fsu = (p->in.fpaside) ? dfregs : ffregs;
			  fcallflag++;
			  }
			callflag++;	/* this use in sucomp() only */
			/* This is similar to the above CALL,STCALL stuff. A
			   test to see if all regs are really needed.
			*/
			return;
			}

		case RS:
		case LS:
			/* MC68000 does not permit arbitrary size immediate
			   shift count.  In that case the count must be stuffed
			   into a register.
			*/
			if (pr->in.op != REG)
				{
				if (pr->in.op != ICON ||
					(pr->tn.lval < 1 || pr->tn.lval > 8) )
					nr++;
				}
			break;

		case QUEST:
		case COLON:
		case COMOP:
			if (nfr) nr = 0;
			/* fall thru */
		case ANDAND:
		case OROR:
			p->in.su = max( max(sul,sur), nr);
			return;

		case DIVP2:
		case EQV:
		case NEQV:
			nr++;	/* an extra AREG needed */
			break;
		case DIV:
			break;
		case MOD:
			nr++;
			break;
		case MUL:
			/* fall thru */
		case PLUS:
		case OR:
		case AND:
		case ER:
		{

		/* AND is ruined by the hardware */
		/* permute: get the harder on the left */

		register TWORD rt;


		if ( o == ER ) {
# ifdef EORIFIX
		  /* 68020 'eor' req's AREG or constant as source opd */
		  if ( ! (pr->in.op == REG && ISAREG( pr->tn.rval ))
		      && ! (pr->in.op == ICON || pr->in.op == NAME) )
# else /* EORIFIX */
		  /* 68020 'eor' req's AREG as source opd */
		  if ( ! (pr->in.op == REG && ISAREG( pr->tn.rval )) )
# endif /* EORIFIX */ 
		    {
# ifdef EORIFIX
		    if ( (pl->in.op == REG && ISAREG( pl->tn.rval ))
		        || pl->in.op == ICON || pl->in.op == NAME )
# else /* EORIFIX */
		    if ( pl->in.op == REG && ISAREG( pl->tn.rval ) )
# endif /* EORIFIX */ 
		      goto swap;
		    else
		      nr++;
		    }
		  }
		else if ( o == OR || o == AND )
				/* 68020 'or' & 'and' req'r non-BREG as src */
		  if ( pr->in.op == REG && ISBREG( pr->tn.rval ) )
		    {
		    if ( !(pl->in.op == REG && ISBREG( pl->tn.rval ))
			&& (pl->rn.su_total == 0) )
		      goto swap;
		    else
		      nr++;
		    }

		if( ISTNODE( pl ) || sul > sur ) 
			goto noswap;  /* don't do it! */

		/* look for a funny type on the left, one on the right */


		type = pl->in.type;	/* new use for type */
		rt = pr->in.type;

		if( (rt==CHAR||rt==UCHAR) && (type==INT||type==UNSIGNED||ISPTR(type)) ) 
			goto swap;

		if( type==LONG || type==ULONG ){
			if( rt==LONG || rt==ULONG ){
				/* if one is a STARNM, swap */
				if( pl->in.op == UNARY MUL && sul==0 )
					goto noswap;
				if( pr->in.op == UNARY MUL &&
					pl->in.op != UNARY MUL )
						goto swap;
				goto noswap;
				}
			else if( pl->in.op == UNARY MUL && sul == 0 )
				goto noswap;
			else goto swap;  /* put long on right, unless STARNM */
			}

		/* we are finished with the type stuff now; if one is addressable,
			put it on the right */
		if( pl->rn.su_total == 0 && pr->rn.su_total != 0 )
			{

			register int ssu;

swap:
			ssu = sul;
			sul = sur;
			sur = ssu;
			p->in.left = pr;
			p->in.right = pl;
			pr = p->in.right;
			}
		}
		}
noswap:

	sur = pr->in.su;
	if( pr->rn.su_total == 0 ){
		/* get left value into a register, do op */
		p->in.su = max( nr, sul );
		}
	else {
		/* do harder into a register, then easier */
		p->in.su = max( nr+nr, min( max( sul, nr+sur ), max( sur, nr+sul ) ) );
		}
	}





#ifdef DEBUGGING
flag radebug = 0;
#endif



# if 0	/*not needed because all calls commented out anyway */
mkrall( p, r ) register NODE *p; {
	/* insure that the use of p gets done with register r; in effect, */
	/* simulate offstar */

	if( p->in.op == FLD ){
		p->in.left->in.rall = p->in.rall;
		p = p->in.left;
		}

	if( p->in.op != UNARY MUL ) return;  /* no more to do */
	p = p->in.left;
	if( p->in.op == UNARY MUL ){
		p->in.rall = r;
		p = p->in.left;
		}
	if( p->in.op == PLUS && p->in.right->in.op == ICON ){
		p->in.rall = r;
		p = p->in.left;
		}
	rallo( p, r );
	}
# endif /* 0 */





rallo( p, down ) register NODE *p; {
	/* do register preference assignments */
	register short o = p->in.op;
	register TWORD type;
	register down1, down2;
	short ty;

#ifdef DEBUGGING
	if( radebug ) prntf( "rallo( 0x%x, 0x%x )\n", p, down );
#endif

	down2 = NOPREF;
	p->in.rall = down;
	down1 = ( down &= ~MUSTDO );

	ty = (short)optype( o );
	type = p->in.type;


	if( type == DOUBLE || type == FLOAT )
		{
		if( o == FORCE ) down1 = D0|MUSTDO;
		else if ( o == ASSIGN )
		  down2 = down1;
		}
	else switch( o ) {
	case ASSIGN:	
		down1 = NOPREF;
		down2 = down;
		break;

# if 0	/* this stuff should not be needed in 68000 family */
	/*case ASG MUL:*/
	case ASG DIV:
	case ASG MOD:
		/* NOTE: all register references here were or'd with MUSTDO */
		/* e.g. D1|MUSTDO */
		/* keep the addresses out of the hair of (r0,r1) */
		if(fregs == 2 ){
			/* lhs in (r0,r1), nothing else matters */
			down1 = D1;
			down2 = NOPREF;
			break;
			}
		/* at least 3 regs free */
		/* compute lhs in (r0,r1), address of left in r2 */
		p->in.left->in.rall = D1;
		mkrall( p->in.left, D2);
		/* now, deal with right */
		if( fregs == 3 ) rallo( p->in.right, NOPREF );
		else {
			/* put address of long or value here */
			p->in.right->in.rall = D3;
			mkrall( p->in.right, D3);
			}
		return;

	/*case MUL:*/
	case DIV:
	case MOD:
		rallo( p->in.left, D1);

		if( fregs == 2 ){
			rallo( p->in.right, NOPREF );
			return;
			}
		/* compute addresses, stay away from (r0,r1) */

		p->in.right->in.rall = (fregs==3) ? D2: D3;
		mkrall( p->in.right, D2);
		return;
# endif	/* 0 */

	case CALL:
	case STASG:
	case EQ:
	case NE:
	case GT:
	case GE:
	case LT:
	case LE:
	case NOT:
	case ANDAND:
	case OROR:
		down1 = NOPREF;
		break;

	case FORCE:	
		down1 = D0|MUSTDO;
		break;

		}

	if( ty != LTYPE ) rallo( p->in.left, down1 );
	if( ty == BITYPE ) rallo( p->in.right, down2 );

	}






offstar( p ) register NODE *p; {
	/* handle indirections */
	register NODE *pl;
	register NODE *l,*r;
	int indexregtarget;
	NODE *lside, *rside;

	if( p->in.op == UNARY MUL )
		{
		/* The following code has an incestuous relationship with
		   canon() code in reader.c. We attempt here to rewrite the tree
		   so that canon() can recognize it as an indexed addressing
		   mode. Certain cases lend themselves to indexed mode but are
		   not detected here simply because the indexed mode is more
		   costly than the simpler modes it would replace.

		   Undoubtedly other cases remain that could reasonably be added
		   here.
		*/
		if ((p->in.left->in.op == PLUS)
			|| (p->in.left->in.op == MINUS))
			{
			pl = p->in.left;
			if (pl->in.op == PLUS && !ISPTR(pl->in.left->in.type))
				{
				/* reverse for canonicalization */
				l = pl->in.right;
				pl->in.right = pl->in.left;
				pl->in.left = l;
				}
			l = pl->in.left;
			r = pl->in.right;
	
			if ( pl->in.op == PLUS && ISPTR(r->in.type)
				&& ((r->in.su < l->in.su) || offstarscaler(l)) )
				{
				/* swap to get cheapest address reg on the left */
				l = r;
				pl->in.right = pl->in.left;
				pl ->in.left = l;
				r = pl->in.right;
				}
	
			indexregtarget = ( SAFESTBREG () ) ?
						INTAREG|INAREG|INTBREG|INBREG :
					    	INTAREG|INAREG|INBREG;

			if (r->in.op==SCONV && r->in.left->rn.su_total == 0
				&& l->in.su < fregs )
				order(r, indexregtarget);
			if ( (r->in.op == REG) 
				|| ((l->in.su < fregs) && offstarscaler(r)) )
				{
				lside = rside = NULL;
				
				if (pl->in.op == MINUS)
					{
				  	if ( r->in.op != REG )
						rside = r;
				  	else
				    	{
				    	order(pl, INTBREG|INBREG);
				    	return;
				    	}
				  	}
				else
					if (r->in.op == LS && r->in.left->in.op != REG)
						rside = r->in.left;
				if ((l->in.op == MINUS || l->in.op == PLUS)
					&& l->in.right->in.op==ICON)
					{
					if (!INDEXREG(l->in.left))
						lside = l->in.left;
					}
		  		else
					{
					if (!INDEXREG(l))
						lside = l;
					}
				if ((lside != NULL) && (rside != NULL) && (lside->in.su > rside->in.su))
					{
					order(lside,INTBREG|INBREG);
					order(rside,indexregtarget);
					}
				else
					{
					if (rside != NULL)
						order(rside,indexregtarget);
					if (lside != NULL)
						order(lside,INTBREG|INBREG);
					}
				return;
				}

			/* r->in.right->in.op == NAME disallowed to avoid
			 * conflict with FORTRAN static vars -- sjo
			 */
			if ((r->in.op == PLUS || r->in.op == MINUS)
				&& (r->in.right->in.op == ICON) &&
				offstarscaler(r->in.left))
				{
				rside = lside = NULL;

				if (pl->in.op == MINUS)
					rside = r;
				else
					rside = r->in.left->in.left;

				if (ISPTR(l->in.type) &&
					(l->in.op != REG || !ISBREG(l->tn.rval)) )
					lside = l;

				if ((lside != NULL) && (rside != NULL) && (lside->in.su > rside->in.su))
					{
					order(lside,INTBREG|INBREG);
					order(rside,INTAREG|INAREG);
					}
				else
					{
					if (rside != NULL)
						order(rside,INTAREG|INAREG);
					if (lside != NULL)
						order(lside,INTBREG|INBREG);
					}
				return;
				}
		    /* at this point we could r into a register and try
			 again but the cost of this address type is higher than
			 the cost of the simpler addressing schemes.
		    */
	
	  		}
		p = p->in.left;
		}

	if( p->in.op == PLUS || p->in.op == MINUS ){
		if( p->in.right->in.op == ICON )
		    	{
			/* Put the pointer side into an Address register.
			   (One side or the other must be a pointer ...
			   otherwise it wouldn't be a UMUL top node.)
			*/
			order((ISPTR(p->in.left->in.type)||haseffects(p->in.left))?
				p->in.left : p->in.right , INTBREG|INBREG );
			return;
			}
		}
	order( p, INTBREG|INBREG );
	}

adrstar( p ) register NODE *p; {
	/* If a tree can be ordered into SADREG form do do, else just order */
	/* it.  SADRREG is an OREG without the U* on top.  */
	register NODE *l,*r;
	int indexregtarget;
	NODE *lside, *rside;

	/* We check for op == PLUS/MINUS and type == ptr before calling */

	if ((p->in.op == PLUS) && (!ISPTR(p->in.left->in.type)))
		{
		/* reverse for canonicalization */
		l = p->in.right;
		p->in.right = p->in.left;
		p->in.left = l;
		}
	l = p->in.left;
	r = p->in.right;
	
	if ((p->in.op == PLUS) &&
            ISPTR(r->in.type) &&
            ((r->in.su < l->in.su) || offstarscaler(l)))
		{
		/* swap to get cheapest address reg on the left */
		l = r;
		p->in.right = p->in.left;
		p->in.left = l;
		r = p->in.right;
		}
	
	indexregtarget = ( SAFESTBREG () ) ?
				INTAREG|INAREG|INTBREG|INBREG : INTAREG|INAREG|INBREG;

	if (r->in.op==SCONV && r->in.left->rn.su_total == 0
		&& l->in.su < fregs )
		order(r, indexregtarget);

	if ( (r->in.op == REG) 
		|| ((l->in.su < fregs) && offstarscaler(r)) )
		{
		lside = rside = NULL;
		
		if (p->in.op == MINUS)
			{
		  	if ( r->in.op != REG )
				rside = r;
		  	else
		    	{
		    	order(p, INTBREG|INBREG);
		    	return;
		    	}
		  	}
		else
			if (r->in.op == LS && r->in.left->in.op != REG)
				rside = r->in.left;
		if ((l->in.op == MINUS || l->in.op == PLUS)
			&& l->in.right->in.op==ICON)
			{
			if (l->in.left->in.op != REG)
				lside = l->in.left;
			}
	  	else
			{
			if (l->in.op != REG)
				lside = l;
			}
		if ((lside != NULL) && (rside != NULL) && (lside->in.su > rside->in.su))
			{
			order(lside,INTBREG|INBREG);
			order(rside,indexregtarget);
			}
		else
			{
			if (rside != NULL)
				order(rside,indexregtarget);
			if (lside != NULL)
				order(lside,INTBREG|INBREG);
			}
		return;
		}

	/* r->in.right->in.op == NAME disallowed to avoid
	 * conflict with FORTRAN static vars -- sjo
	 */
	if ((r->in.op == PLUS || r->in.op == MINUS)
		&& (r->in.right->in.op == ICON) &&
		offstarscaler(r->in.left))
		{
		rside = lside = NULL;

		if (p->in.op == MINUS)
			rside = r;
		else
			rside = r->in.left->in.left;

		if (ISPTR(l->in.type) &&
			(l->in.op != REG || !ISBREG(l->tn.rval)) )
			lside = l;

		if ((lside != NULL) && (rside != NULL) && (lside->in.su > rside->in.su))
			{
			order(lside,INTBREG|INBREG);
			order(rside,INTAREG|INAREG);
			}
		else
			{
			if (rside != NULL)
				order(rside,INTAREG|INAREG);
			if (lside != NULL)
				order(lside,INTBREG|INBREG);
			}
		return;
		}
	order(p,INTBREG|INBREG);
	}

LOCAL offstarscaler(p)	register NODE *p;
{
	return(p->in.op == LS && p->in.right->in.op == ICON
		&& p->in.right->tn.lval <= 3);
}










/* I think this returns true iff p represents a nice unary type node */
LOCAL niceuty( p ) register NODE *p; {
	register TWORD t;

	return( p->in.op == UNARY MUL && (t=p->in.type)!=CHAR &&
		t!= UCHAR && t!= FLOAT &&
		shumul( p->in.left) != STARREG );
	}



/* rewriter for binary ops. Anytime setbin() can rewrite the tree to a new
   useful shape it returns a non-zero value. */
setbin( p ) register NODE *p; {
	register NODE *r, *l;
	unsigned tsul, tsur;
	unsigned fsul, fsur;

	r = p->in.right;
	l = p->in.left;

	tsur = r->in.su;
	tsul = l->in.su;
	fsur = r->in.fsu;
	fsul = l->in.fsu;

	if (p->in.op == FMONADIC)
	  {
	  if (p->in.fpaside && ! NOT_DRAGON_FMONADIC(l))
		order(r, INTDREG);
	  else
		{
		if (r->in.fpaside && 
		    ((r->in.op != FMONADIC &&
			( ! (FLT_EA(r) && ! ISFDNODE(r)) ) )
		      || (r->in.op == FMONADIC &&
			 ! NOT_DRAGON_FMONADIC(r->in.left))))
		  order(r, INTEMP);
		r->in.fpaside = 0;
		if (r->in.op != REG)
			order(r, INTFREG);
		else if (p->in.rall & (MUSTDO|D0))
			order(p, INTFREG|INTAREG);
		else
			return(0);
		}
	  return(1);
	  }
	if ( !flibflag && ISFTP(p->in.type) )
		{
		/* With the 68881 it's advantageous to have the lhs in a STFREG
		   asap. We can work memory ops onto STFREGs but not the reverse.
		*/
		if ( ISTFDNODE(r) && (p->in.op == PLUS || p->in.op == MUL)
		    && ! ISTFDNODE(l) )
			{
			/* Swap to ensure lhs is in an STFREG. These are
			   commutative ops.
			*/
			p->in.left = r;
			p->in.right = r = l;
			l = p->in.left;
			}
		 if ( ISTFDNODE( l ) )
		  {		/* left opd is in scratch flt reg */
		  if (p->in.fpaside)	/* dragon */
		    if ( ISDNODE( r ) )
		      return( 0 );	/* rewrite as ASG OP */
		    else
		      order( r, callop(r->in.op)?INTAREG:INTDREG );
		  else			/* 881 */
		   if ( daleafop( r->in.op ) )
		    {
		    if ( ISDNODE( r ) )
		      order( r, INTEMP );
		    else
		      return( 0 );
		    }
		   else if (r->in.op == UNARY MUL && r->in.fsu == 0)
		    offstar( r );
		   else
		    order( r, (r->in.type!=DOUBLE)?INTAREG|INTFREG:INTFREG );
		  }
		else {	/* lft opd is not in scratch flt reg */
		  NODE *tt;
		  int targ;
		  if ( p->in.fpaside )	{ /* dragon */
		    targ = callop(r->in.op)? INTAREG : INTDREG;
		    if ( ISDNODE( r ) )
		      order( l, INTDREG );
		    else if ( ISDNODE( l ) )
		      order( r, targ );
		    else {
		      tt = (fsur > fsul)            ? r
 			   : (fsur < fsul)          ? l
 			   : (tsur > tsul)	    ? r
 			   : (tsur < tsur)	    ? l
 			   : ((optype(l->in.op)==LTYPE) &&
 			        (optype(r->in.op)!=LTYPE) ) ? r : l;
		      order(tt, (tt==r)? targ : INTDREG);
		      }
		    }
		  else {			/* 881 */
		    if ( FLT_EA( r ) )
		      order( l, INTFREG );
		    else if ( FLT_EA( l ) )
		      order( r, (r->in.type!=DOUBLE)?INTAREG|INTFREG:INTFREG);
		    else	{ /* neither side opd is OK as 881 <ea> */
		      tt = (fsur > fsul)            ? r
 			   : (fsur < fsul)          ? l
 			   : (tsur > tsul)	    ? r
 			   : (tsur < tsul)	    ? l
 			   : ((optype(l->in.op)==LTYPE) &&
 			        (optype(r->in.op)!=LTYPE) ) ? r : l;
		      order(tt, (tt==r&&r->in.type!=DOUBLE)?INTAREG|INTFREG
							  :INTFREG);
		    }
		   }
		  }
		return(1);
		}
	if( r->rn.su_total == 0 ){ /* rhs is addressable */
		if( logop( p->in.op ) ){
			if( l->in.op == UNARY MUL && l->in.type != FLOAT
				&& shumul( l->in.left ) != STARREG )
					offstar( l);
			else order( l, INAREG|INTAREG|INBREG|INTBREG|INTEMP);
			return( 1 );
			}
		if ( ISTNODE(l) )
			{
			if (ISBREG(l->tn.rval) && p->in.op != PLUS
				&& p->in.op != MINUS)
				/* + or - can be done in address regs. Other
				   ops will have to be done in data regs.
				*/
				{
				order( l, INTAREG);
				return( 1 );
				}
			}
		else
			{	
			order( l, l->rn.su_total==0 &&
					ISPTR(p->in.type) && ISPTR(l->in.type)
					&& SAFESTBREG() ?
				INTBREG : INTAREG|INTBREG);
			return( 1 );
			}
		/* rewrite */
		return( 0 );
		}
	/* now, rhs is complicated: must do both sides into registers */
	/* do the harder side first */

	if( logop( p->in.op ) ){
		/* relational: do both sides into regs if need be */

		if( tsur > tsul ){
			if( niceuty(r) ){
				offstar( r);
				return( 1 );
				}
			else if( !ISTNODE( r ) ){
				order( r, INTAREG|INAREG|INTBREG|INBREG|INTEMP);
				return( 1 );
				}
			}
		if( niceuty(l) ){
			offstar( l);
			return( 1 );
			}
		else if( !ISTNODE( l ) ){
			order( l, INTAREG|INAREG|INTBREG|INBREG|INTEMP);
			return( 1 );
			}
		else if( niceuty(r) ){
			offstar( r);
			return( 1 );
			}
		if( !ISTNODE( r ) ){
			order( r, INTAREG|INAREG|INTBREG|INBREG|INTEMP);
			return( 1 );
			}
		cerror( "setbin can't deal with %s", opst[p->in.op] );
		}

	/* ordinary operator */

	if( !ISTNODE(r) && r->in.su > l->in.su ){
		/* if there is a chance of making it addressable, try it... */
		if( niceuty(r) ){
			offstar( r);
			return( 1 );  /* hopefully, it is addressable by now */
			}
		order( r, INTAREG|INAREG|INTBREG|INBREG|INTEMP); 
		/* anything goes on rhs */
		return( 1 );
		}
	else {
		if( !ISTNODE( l ) ){
			order( l, INTAREG|INTBREG);
			return( 1 );
			}
		/* rewrite */
		return( 0 );
		}
	}




setstr( p ) register NODE *p; { /* structure assignment */

	NODE *l = p->in.left;
	NODE *r = p->in.right;

	if( r->in.op == REG )
	  {
	  if ( mvunionreg(r) ) 	/* Note, this call will order(r,INTBREG)
					if actual arg is data reg */
	    return(1);
	  if( l->in.op != NAME && l->in.op != OREG && l->in.op != REG )
	    {
		/* l->in.op==REG iff unionflag is set in pass 1 */
	    if( l->in.op != UNARY MUL ) cerror( "bad setstr" );
	    order( l->in.left, INTBREG );
	    }
	  else
	    return( 0 );
	  }
	else	/* right side is not appropriate, must be in INBREG|INTBREG */
	  if ( l->in.op == NAME || l->in.op == OREG || l->in.op == REG )
	    			/* left side is in appropriate form */
	    order( r, INTBREG );
	  else 
	    {
	    int lsu = l->in.su;
	    int rsu = r->in.su;
	    if( l->in.op != UNARY MUL )
	      cerror( "bad setstr" );
					/* Note, afregs is set to the
					   ordinal number of the highest
					   scratch addr reg (STBREG), unlike
					   fregs/ffregs/dfregs, see setregs()*/
	    if ( rsu >= lsu && (lsu /*+1*/) <= (afregs /*+1*/) )
	      order( r, INTBREG );
	    else
	      {
	      l = l->in.left;		/* note, l moves down one here */
	      if (  ( rsu /*+1*/ > afregs  /*+1*/ )
		 && ( l->in.op != REG )
		 && ( l->in.op != OREG )
		 && ( l->in.op != NAME ) )
	        order( l, INTEMP );
	      else
		if ( l->in.op != REG )
	          order( l, INTBREG );
		else
	          order( r, INTBREG );
	      }
	    }
	return( 1 );
	}


/* setasop() called from order() to (possibly) rewrite +=, -=, *=, etc. into a
   form easier to deal with.
*/
setasop( p ) register NODE *p;
{
	/* setup for =ops */
	register short sul, sur;
	short fsul, fsur;
	register NODE *p1, *p2;	/* Note: overloaded */
	flag addressregop = FALSE;

	sul = (p1 = p->in.left)->in.su;
	sur = (p2 = p->in.right)->in.su;
	fsul = p1->in.fsu;
	fsur = p2->in.fsu;

	if ( !flibflag && ISFTP( p->in.type ) )
	  {
	  if ( p->in.fpaside ) {
	    if ( ISDNODE( p1 ) ) {	
	      order( p2, INTDREG );
	      return( 1 );
	      }
	    }
	  else
	    if ( ISFNODE( p1 ) ) {
	      if ( FLT_EA( p2 ) )
	        offstar( p2 );		/* turn rt into OREG */
	      else
	        order( p2, INTFREG );	/* get rt into scratch reg */
	      return( 1 );
	      }
	  }

	switch( p->in.op ){
	  case ASG PLUS:
	  case ASG MINUS:
			addressregop = TRUE;
			if (ISFTP(p->in.type))
				{
				if (p1->rn.su_total && sur < fregs /* No calls*/
					&& (fsul >= fsur && sul >= sur)
					&& p1->in.op == UNARY MUL)
					goto complexlhs;
				if ((!flibflag) && (p->in.type != LONGDOUBLE))
			    		{
			    /* Singleflag not checked because 68881 does all ops
				 in full precision anyway.
			    */
			    		fltasop68881(p);
			    		return(1);
			    		}
				}
			/* fall thru */
	  case ASG OR:
	  case ASG ER:
	  case ASG AND:
		if (ISFTP(p->in.type))
			{
			walkf(p, hardops);	/* change into calls */
			if ( p->in.type==FLOAT && 
			    (p1=p->in.left)->in.op == REG && p1->tn.lval == D0
			     && (p2=p->in.right)->in.op == CALL )
				{
				/* special case - no need to ASSIGN from a CALL
				   into D0 since a call automatically fills D0.
				*/
				p1->in.op = FREE;
				ncopy(p, p2);
				p2->in.op = FREE;
				}
			return (1);
			}
		if ( p1->rn.su_total == 0 && p2->in.op != REG)
		/*
		if ( sul == 0 && (p2->in.op != REG
			|| (!addressregop && ISBREG(p2->tn.rval)) ) )
		*/
			{
		  	order(p2, INAREG|INTAREG);
			return(1);
			}
		break;

	  case ASG DIVP2:
	  case ASG LS:
	  case ASG RS:
		if (p1->in.op != REG) return(0);
		if ( !ISAREG(p1->tn.rval) ) 
			{
			order(p1, INAREG|INTAREG);
			}
		if (p2->in.op == REG ||
		    (p2->in.op==ICON && p2->tn.lval>=1 && p2->tn.lval<=8))
	  		break;
		order(p2, INAREG|INTAREG);
		return(1);

	  case ASG MUL:
	  case ASG DIV:
			if (ISFTP(p->in.type))
				{
				if (p1->rn.su_total && sur < fregs /* No calls*/
					&& (fsul >= fsur && sul >= sur)
					&& p1->in.op == UNARY MUL)
					goto complexlhs;
				if (!flibflag)
			    		{
			    		/* Singleflag not checked because 68881
					   does all ops in full precision
					   anyway.
			    		*/
			    		fltasop68881(p);
			    		return(1); 
			    		}
				}
			/* fall thru */
	  case ASG MOD:
		if (p1->in.op != REG) return(0);
	}

	if( p2->rn.su_total == 0 )
		{

leftadr:
		/* easy case: if addressable, do left value, op, store */
		if( p1->rn.su_total == 0 ) goto rew;  /* rewrite */

		/* harder; make a left address, val, op, and store */

		if( p1->in.op == UNARY MUL )
			{
complexlhs:
			offstar( p1);
			return( 1 );
			}
# ifndef FORT	/* field ops are impossible from FORTRAN */
		if( p1->in.op == FLD && p1->in.left->in.op == UNARY MUL )
			{
			offstar( p1->in.left);
			return( 1 );
			}
# endif

rew:	/* rewrite, accounting for autoincrement and autodecrement */

		if (!addressregop && p2->in.op == REG && !ISAREG(p2->tn.rval) )
			{
			order(p2, INAREG|INTAREG);
			return(1);
			}

		p1 = p->in.left;
# ifndef FORT
		if( p1->in.op == FLD ) p1 = p1->in.left;
# endif /* FORT */
		if( p1->in.op != UNARY MUL || shumul(p1->in.left) != STARREG )
			return(0); /* let reader.c do it */

		/* mimic code from reader.c */

		p2 = tcopy( p );
		p->in.op = ASSIGN;
		reclaim( p->in.right, RNULL, 0 );
		p->in.right = p2;

		/* now, zap INCR on right, ASG MINUS on left */

		if( p1->in.left->in.op == INCR ){
			p1 = p2->in.left;
			if( p1->in.op == FLD ) p1 = p1->in.left;
			if( p1->in.left->in.op != INCR ) cerror( "bad incr rewrite" );
			}
		else if( p1->in.left->in.op != ASG MINUS )  cerror( " bad -= rewrite" );

		tfree( p1->in.left->in.right );
		p1->in.left->in.op = FREE;
		p1->in.left = p1->in.left->in.left;

		/* now, resume reader.c rewriting code */

		canon(p);
		rallo( p, p->in.rall );
		if (p2->in.fpaside)
		{
			order( p2->in.left, INTBREG|INTAREG|INTDREG );
			order( p2, INTBREG|INTAREG|INTDREG );
		}
		else
		{
			order( p2->in.left, INTBREG|INTAREG|INTFREG );
			order( p2, INTBREG|INTAREG|INTFREG );
		}
		return( 1 );
		}

	/* harder case: do right, left address, left value, op, store */

	if( p->in.right->in.op == UNARY MUL && p->in.left->in.op==REG){
		offstar( p->in.right);
		return( 1 );
		}
	/* sur> 0, since otherwise, done above */
	if( p->in.right->in.op == REG ) goto leftadr;  /* make lhs addressable */
	order( p->in.right, INAREG|INBREG );
	return( 1 );
	}



/* makety2() is like makety() in pass1 in that it inserts an SCONV node where
   appropriate. 
*/
NODE *makety2(p, type)	NODE *p; TWORD type;
{
	register NODE *q;

	if (p->in.type == type) return(p);
	q = talloc();
	ncopy(q, p);
	q->in.type = type;
	q->in.op = SCONV;
	q->in.left = p;
	return(q);
}





LOCAL fltasop68881(p)	register NODE *p;
{
	register NODE *p2;

		{
		p2 = tcopy(p);
		p2->in.op -= 1;
		p->in.op = ASSIGN;
		reclaim(p->in.right, RNULL, 0);
		p->in.type = p->in.left->in.type;
		p->in.right = makety2(p2, p->in.type);
		walkf(p, optim2);
		}
}







/* specific for pass 2 */
deflab( l ) unsigned l; {
#ifndef FLINT
	prntf( "L%d:\n", l );
#endif /* FLINT */
	}

genargs( p) register NODE *p; {
	register size,inc;
#ifdef FORT
#ifdef DBL8
	int argsz;
	extern int stack_offset;
#endif /* DBL8 */
#endif /* FORT */

	/* generate code for the arguments */

	/*  first, do the arguments on the right (last->first) */
	while( p->in.op == CM ){
		genargs( p->in.right );
		p->in.op = FREE;
		p = p->in.left;
		}

#ifdef FORT
#ifdef DBL8
	argsz = argsize(p);
#endif /* DBL8 */
#endif /* FORT */

# ifndef FORT
	if( p->in.op == STARG ) /* structure valued argument */
		{
		size = p->stn.stsize;
		if( p->in.left->in.op == ICON ){
			/* make into a name node */
			p->in.op = FREE;
			p= p->in.left;
			p->in.op = NAME;
			if ( picoption )
			  pictrans( p );
			}
		else {
			/* make it look beautiful... */
			p->in.op = UNARY MUL;
again:
			canon( p );  /* turn it into an oreg */
			if( p->in.op != OREG ){
				offstar( p->in.left );		/* maybe p ? */
				canon( p );
				if (p->in.op == UNARY MUL) goto again;
				if( p->in.op != OREG ) cerror( "stuck starg" );
				}
			}

		/*p->tn.lval += size;  /* end of structure */
		/* put on stack backwards */
		if (size >= 12
			)
			{
			unsigned int l;
                        if (size%2 != 0) {  /* Check if odd num */
                          if (size%4 == 1)  /* Check if num/4 has 1 remain */
                          /* SWFfc01090 fix - add + 2 to 1 remain not 3 remain*/
                            p->tn.lval += (((size+1)/4 * 4) + 2); /* 1 remain */
                          else 
                            p->tn.lval += ((size+1)/4 * 4);       /* 3 remain */
                          prntf("\tmov.l\t&%d,%%d0\n", (size+1)/4 - 1);
                          /* SWFfc01090 fix - ouput dbf count */ 
                        }
                        else {              /* even num detected */
                          p->tn.lval += size;
			  prntf("\tmov.l\t&%d,%%d0\n", (size >> 2) - 1);
                        }
			expand(p, FOREFF, "\tlea\tAR,%a0\n");
			deflab( l = GETLAB() );
			expand(p, RNOP, "\tmov.l\t-(%a0),Z-\n");
			prntf("\tdbf\t%%d0,L%d\n", l);
			if (size > (32768*4))
				{
				prntf("\tsub.l\t&0x10000,%%d0\n");
				prntf("\tbge.b\tL%d\n", l);
				}
			if ((size%4) == 1 || (size%4) == 2)
                        /* SWFfc01090 fix - do expand if size/4 remainder=1,2 */
				expand(p, FOREFF, "\tmov.w\t-(%a0),Z-\n");
			}
		else
			{
                        p->tn.lval += size;
			for( ; size>0; size -= inc )
				{
                                if (size%2 != 0) {
                                  inc = 1;
                                  p->tn.lval -= inc;
                                  expand (p, RNOP, "\tmov.w\tAR,Z-\n");
                                }
                                else {
				  inc = (size>2) ? 4 : 2;
				  p->tn.lval -= inc;
				  expand( p, RNOP, (inc==4)?
					"\tmov.l\tAR,Z-\n":"\tmov.w\tAR,Z-\n" );
                                }
				}  /*end for*/
			}
		reclaim( p, RNULL, 0 );
#ifdef FORT
#ifdef DBL8
		stack_offset += argsz;
#endif /* DBL8 */
#endif /* FORT */
		return;
		}
# endif /* FORT */

	/* ordinary case */

	order( p, FORARG );
#ifdef FORT
#ifdef DBL8
	stack_offset += argsz;
#endif /* FORT */
#endif /* DBL8 */
	}






/* argsize - computes the size of a function/procedure argument.
   NOTE: the C language assumes all float arithmetic is actually performed with
	 doubles. However, F77 does use legitimate single precision arithmetic;
	 intrinsic calls to float arithmetic functions should use sizeof( float)
	 argsizes.
*/

argsize( p ) register NODE *p;
{
	register t;
	t = 0;
	if( p->in.op == CM ){
		t = argsize( p->in.left );
		p = p->in.right;
		}
	if( p->in.type == DOUBLE ) {
		SETOFF( t, 2 );
		/* logical ops, if typed double in the tree, really generate
		   only 4 bytes of result. The confusion is due to 68881
		   auto converts.
		*/
		return( !logop(p->in.op)? t+8 : t+4 );
		}
	if( p->in.type == LONGDOUBLE ) {
		SETOFF( t, 2 );
		return(!logop(p->in.op)? t+16 : t+4);
		}
	else if( p->in.op == STARG ){
		SETOFF( t, p->stn.stalign );  /* alignment */
		return( t + p->stn.stsize );  /* size */
		}
	else {
		SETOFF( t, 2 );
		return( t+4 );
		}
}
	
	

/* mvunionreg() called iff unionflag to get register unions into 
   address regs for correct addressing.
*/
int mvunionreg(np)	register NODE *np;
{
	if ( np->in.op == REG && ISPTR(np->tn.type) && np->tn.rval < A0 )
		{
		/* Ptr in a D reg. Impossible addressing mode. */
		order (np, INTBREG);
		return (TRUE);
		}
	return (FALSE);
}
