/* file trees.c */
/*    SCCS    REV(64.5);       DATE(92/04/03        14:22:35) */
/* KLEENIX_ID @(#)trees.c	64.5 92/02/27 */

#if defined xcomp300_800
       /* for 'signal.h' */
#      define _HPUX_SOURCE
#endif /* xcomp300_800 */

#ifdef BRIDGE
# include "bridge.I"
#else  /*BRIDGE*/
# include <errno.h>
# include <signal.h>
# include "mfile1"
# include "messages.h"
# ifdef SA
# include "sa.h"
# endif
#endif /*BRIDGE*/

# ifdef QUADC
	extern QUAD _U_Qfcnvxf_sgl_to_quad();
	extern QUAD _U_Qfcnvxf_usgl_to_quad();
	extern QUAD _U_Qfcnvff_dbl_to_quad();
	extern QUAD _U_Qfadd();
	extern QUAD _U_Qfmpy();
	extern QUAD _U_Qfdiv();
	extern QUAD _U_Qfsub();
	extern QUAD _U_Qfneg();
	extern int _U_Qflogneg();
	extern int _U_Qfcmp();

#	define LDEQ  4
#	define LDLT  8
#	define LDGT 16
# endif /* QUADC */

#ifdef ANSI
        static char *arg_name = NULL;  /* current argument name */
        static char *arg_fname = NULL; /* current function */
#endif /* ANSI */


	    /* corrections when in violation of lint */

/*	some special actions, used in finding the type of nodes */
# define NCVT 	        0x1
# define PUN 	        0x2
# define TYPL 	        0x4
# define TYPR 	        0x8
# define BEFORE_CVT     0x10
# define TYMATCH        0x20
# define LVAL 	        0x40
# define CVTO 	        0x80
# define CVTL 	        0x100
# define CVTR 	        0x200
# define PTMATCH        0x400
# define OTHER 	        0x800
# define NCVTR 	        0x1000
# define RINT 	        0x2000
# define LLINT 	        0x4000
# define PATTRIB	0x8000
# define LONGASSIGN	0x10000
# define NOFTNPTRS      0x20000
# define VOIDCK		0x40000

/* Pointer attribute for C1OREG in longassign() */
#define C1PTR 		0x04000000

/* ISSIMPLE evaluates to non-zero if the tword t represents a
 * simple type & non-void  (ie, all 0's in modifier fields. This is used to
 * put restrictions on integer constant folding here.
 */
# define ISSIMPLE(t) ( !((t)&(~BTMASK)) && ((t) != VOID) )

#ifdef ANSI
/* ISNULL determines if the node is a null pointer constant */

# define ISNULL(p) ((Is_Integral_Constant_Expression(p)||\
		     (((p)->in.type == VOIDPTR)&&((p)->in.op == ICON))) \
		     && ((p)->tn.lval == NULL))
#else /*ANSI*/
# define ISNULL(p) (((p)->in.op == ICON)&&((p)->tn.rval == NULL)&& \
		    (((p)->in.type == VOIDPTR)||ISSIMPLE((p->in.type))))
#endif /*ANSI*/
# define MAX(a,b) 	( (a)<(b)? (b):(a) )

/*
 * BITSINTYPE evaluates to 33 for FLOAT so that BITS(FLOAT) > BITS(INT).
  */
# define BITSINTYPE(nodep)      ((nodep->in.type==FLOAT)?33:(dimtab[nodep->fn.csiz]))

# define ISPTR_PAF(x) ( ( x >> TSHIFT ) & ARY ) /* true if type of x is ptr
						   to PTR|FTN|ARY ( == ARY ) */

# define INCQUAL(x) ( INCREF(x) - PTR )         /* make room for new qualifiers */

/* return attribute without top level of qualifiers */
# define DEQUALIFY(type,attribute) ((type&~BTMASK)?(attribute&(~(PATTR_CON|PATTR_VOL))):(attribute&(~(ATTR_CON|ATTR_VOL))))

/* node conventions:

	NAME:	rval>0 is stab index for external
		rval<0 is -inlabel number
		lval is offset in bits
	ICON:	lval has the value
		rval has the STAB index, or - label number,
			if a name whose address is in the constant
		rval = NONAME means no name
	REG:	rval is reg. identification cookie

	*/

#ifdef DEBUGGING
flag tdebug = 0;
flag bdebug = 0;
extern int eprint();
#endif /* DEBUGGING */

#ifdef C1_C
extern OFFSZ p1maxoff;		/* Split-C version of maxoff  */
#endif /* C1_C */

#ifndef IRIF
        extern flag strcpy_inline_enabled;
        LOCAL flag lalignflag;
#else /* IRIF */
        NODE *check_INDEX();
#endif /* IRIF */

static report_no_errors = 0;/* shuts off psize's error messages */

NODE *longassign();
LOCAL int fixstasg();
#ifdef ANSI
TWORD default_type();
#else
NODE *strargs();
#endif

#ifndef BRIDGE

NODE *
buildtree( o, l, r ) register short o; register NODE *l, *r; {
	register NODE *p;
	register actions;
	register short opty;
	NODE *lr, *ll;
	int islval;

# ifdef ANSI
	int narg;
# endif

# ifdef DEBUGGING
	if( bdebug ) (void)prntf( "buildtree( %s, %x, %x )\n", opst[o], l, r );
# endif /* DEBUGGING */

# ifdef ANSI
	if (o==CALL) {
		/* get the argcount and strip the extra node.  This is
		 * done early (ie, here) rather than at the main body of
		 * code for CALL to minimize impact on the rest of
		 * build tree */
		narg=r->tn.rval;
		r->in.op = FREE;
		r=r->in.left;
		}
	else if (o==UNARY CALL) narg = 0;
# endif

	opty = (short)optype(o);

	if ( (opty == BITYPE && (!l || !r))
		|| (opty == UTYPE && !l) )
		{
		cerror("panic in buildtree: ill-formed expression, op %s", opst[o]);
		}

#ifndef ANSI
	/* check for constants in order to do constant folding */

	if( opty == UTYPE && l->in.op == ICON ){

		switch( o ){

		case NOT:
#ifdef LINT
			/* "constant argument to NOT" */
			if( hflag ) WERROR( MESSAGE( 22 ) );
#endif /* LINT */
		case UNARY MINUS:
		case COMPL:
			if ( ISSIMPLE(l->in.type) && conval( l, o, l ) )
			   return(l);
			break;

			}
		}

	else if( o==UNARY MINUS && l->in.op==FCON && l->in.type !=VOID ){
		l->fpn.dval = -l->fpn.dval;
		return(l);
		}

	else if( o==QUEST && l->in.op==ICON && l->in.type !=VOID ) {
		l->in.op = FREE;
		r->in.op = FREE;
		if( l->tn.lval ){
			tfree( r->in.right );
			return( r->in.left );
			}
		else {
			tfree( r->in.left );
			return( r->in.right );
			}
		}

	else if( (o==ANDAND || o==OROR) && (l->in.op==ICON||r->in.op==ICON) ) goto ccwarn;

	else if (opty == BITYPE)
		{
		if (l->in.op == ICON && r->in.op == ICON) switch(o)
		{
		case LT:
		case GT:
		case LE:
		case GE:
		case EQ:
		case NE:
		case ANDAND:
		case OROR:
#ifndef IRIF
		case CBRANCH:
#endif /* not IRIF */

ccwarn:
#ifdef LINT
			/* "constant in conditional context" */
			if( hflag ) WERROR( MESSAGE( 24 ) );
#endif /* LINT */

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			if ( ISSIMPLE(l->in.type) && ISSIMPLE(r->in.type) &&
			   conval( l, o, r ) ) {
				r->in.op = FREE;
				return(l);
				}
			break;
			}
		else if( 
		((l->in.op==FCON && l->in.type !=VOID )||(l->in.op==ICON && ISSIMPLE(l->in.type))) &&
		((r->in.op==FCON && r->in.type !=VOID )||(r->in.op==ICON && ISSIMPLE(r->in.type))) )
			switch(o)
			{
		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
			if( l->in.op == ICON )
				{
				    if (ISUNSIGNED(l->in.type))
					l->fpn.dval = (unsigned) l->tn.lval;
				    else
					l->fpn.dval = l->tn.lval;
				}
			if( r->in.op == ICON )
				{
				    if (ISUNSIGNED(r->in.type))
					r->fpn.dval = (unsigned) r->tn.lval;
				    else
					r->fpn.dval = r->tn.lval;
				}
			l->in.op = FCON;
			l->in.type = l->fn.csiz = DOUBLE;
			r->in.op = FREE;
			switch(o)
				{
				case PLUS:
					l->fpn.dval += r->fpn.dval;
					return(l);
				case MINUS:
					l->fpn.dval -= r->fpn.dval;
					return(l);
				case MUL:
					l->fpn.dval *= r->fpn.dval;
					return(l);
				case DIV:
					/* "division by 0." */
					if( r->fpn.dval == 0 )
						UERROR( MESSAGE( 32 ) );
					else l->fpn.dval /= r->fpn.dval;
					return(l);
				}
			}
		}

#endif /*ANSI*/
	/* it's real; we must make a new node */

	p = block( o, l, r, INT, 0, INT );

#ifdef C1_C
#ifndef IRIF
        /* bubble up the C1 arrayref tag (if not fixed ) */
        if ((l) && !(l->in.fixtag) )  p->in.c1tag = l->in.c1tag;
#endif /* not IRIF */
#endif /* C1_C */

	actions = opact(p);
# ifdef DEBUGGING
	if (bdebug)
		(void)prntf("actions = %x\n", actions);
# endif

	if( actions&LVAL ){ /* check left descendent */
	     if( islval = notlval(l) ) {
		  if (islval == 2)
			  /* "Left-hand side of '%s' should be an lvalue" */
			  WERROR( MESSAGE( 62 ), opst[p->in.op] );
		  else
			  /* "Left-hand side of '%s' should be an lvalue" */
			  UERROR( MESSAGE( 62 ), opst[p->in.op] );
	     }
	}
	if( actions & BEFORE_CVT ) {
	     switch( p->in.op ) {
		case CAST:
		  /* ensure type-name is scalar; conversions are not
		   * done first - they apply only to lvalues and ftn designators
		   */
		  if( !ISSCALAR( p->in.left->in.type )) {
		       /* "type-name of CAST should be scalar" */
		       UERROR( MESSAGE( 166 ));
		  }
	     }
	}
	if( actions & NCVTR ){
		p->in.left = l = pconvert( l );
		}
	else if( !(actions & NCVT ) ){
		/* if something needs to be made into a pointer, pconvert will
		   do it.
		*/
		switch( opty ){

		case BITYPE:
			p->in.right = r = pconvert( r );
			/* fall thru */
		case UTYPE:
			p->in.left = l = pconvert( l );

			}
		}

	if( (actions&PUN) && (o!=CAST||cflag) ){
		chkpun(p);
		}

#ifndef ANSI
	if ( actions&VOIDCK )
		switch( opty )
		{
		case BITYPE:
			if (r->in.type == VOID )
				UERROR( MESSAGE(118) );
			/* fall thru */
		case UTYPE:
			if (l->in.type == VOID )
				UERROR( MESSAGE(118) );
		}

#endif /*ANSI*/
	if (actions & PATTRIB) p = pattrib(p);
	if (actions & RINT) p->in.right = r = makety( r, INT, 0, INT, p->in.right->in.tattrib );
	if (actions & LLINT) p->in.left = l = makety( l, INT, 0, INT, p->in.right->in.tattrib );

	if( actions & (TYPL|TYPR) ){

		register NODE *q;

		q = (actions&TYPL) ? l : r;
#ifdef IRIF
		p = check_INDEX( p, q );
#else /* not IRIF */
		p->in.type = q->in.type;
		p->fn.cdim = q->fn.cdim;
#endif /* not IRIF */

		p->fn.csiz = q->fn.csiz;
#ifdef LINT
                p->in.promote_flg = ((l ? l->in.promote_flg : 0) |
				     (r ? r->in.promote_flg : 0));
#endif
		}

#ifdef ANSI	
	if( actions & NOFTNPTRS )
	    if( ISFTN( DECREF( l->in.type )) || ISFTN( DECREF( r->in.type ))) {
	           /* "function designators or pointers to functions
		       should not be operands of %s" */
		     UERROR( MESSAGE( 41 ), opst[p->in.op] );
		     actions = NCVT;
		   }
#endif /*ANSI*/

#ifndef IRIF
	if( actions & CVTL ) p = convert( p, CVTL );
	if( actions & CVTR ) p = convert( p, CVTR );
	if( actions & LONGASSIGN ) {
		p = longassign(p);
		if (p->in.op != o) return(p);
		}
#endif /* not IRIF */

#ifdef LINT
	if (p->in.op == CAST) p->in.right->in.cast_flg = 1;
#endif /* LINT */
	if( actions & TYMATCH ) p = tymatch(p);
	if( actions & PTMATCH ) p = ptmatch(p);

	if( actions & OTHER ){
		l = p->in.left;
		r = p->in.right;

		switch(o){

#ifdef ANSI
		case LS:
		case RS:    /* ansi mode only */
		  {
		    int t = l->in.type;

		    /* do integral promotions on both operands and
		       then propagate left type upwards */

		    if( t == CHAR || t == UCHAR || t == SCHAR ||
			             t == SHORT || t == USHORT ) 
		        p->in.left = makety( l, INT, 0, INT, 0 );

		    t = r->in.type;
		    if( t == CHAR || t == UCHAR || t == SCHAR ||
			             t == SHORT || t == USHORT ) 
		        p->in.right = makety( r, INT, 0, INT, 0 );
	
		    p->in.type = p->in.left->in.type;
		    break;
		  }
#endif /*ANSI*/
		case NAME:
			{
			register struct symtab *sp;
			sp = idname;
			if( sp->stype == UNDEF ){
				/* "%s undefined" */
				UERROR( MESSAGE( 4 ), sp->sname );
				/* make p look reasonable */
                                p->in.type = INT;
                                p->fn.cdim = INT;
                                p->fn.csiz = INT;
                                p->nn.sym = idname;
                                p->tn.rval = 0;
                                p->tn.lval = 0;
                                defid( p, SNULL );
                                break;
				}
			p->in.type = sp->stype;
			p->in.tattrib = sp->sattrib;
			p->fn.cdim = sp->dimoff;
			p->fn.csiz = sp->sizoff;
			p->tn.lval = 0;
			p->nn.sym = idname;

			/* special case: MOETY is really an ICON... */
			if( p->in.type == MOETY ){
				p->tn.rval = NONAME;
				p->tn.lval = sp->offset;
				p->fn.cdim = 0;
				p->in.type = INT;
				p->in.op = ICON;
				}

#ifndef IRIF
			/* set csiz to int for enums */
			if( ISENUM(BTYPE(p->in.type),p->in.tattrib) )
#ifdef SIZED_ENUMS
				p->fn.csiz = BTYPE(p->in.type);
#else
				p->fn.csiz = INT;
#endif
#endif
			if ( ISARY(p->in.type) &&
			     !dimtab[p->fn.cdim] &&
			     (idname->sclass != EXTERN) &&
			     (idname->sclass != PARAM) &&
			     (idname->sclass != STATIC) &&
			     !typ_initialize ){
			        /* "use of an incomplete type: %s" */
				WERROR( MESSAGE(203), idname->sname );
			}

#ifdef C1_C
                        opty = p->in.type;
                        if (ISARY(opty) || 
			    opty==STRTY ||
			    opty==UNIONTY)
                                p->in.c1tag = idname;
#endif /* C1_C */

#ifdef IRIF
#ifdef HPCDB
			if( cdbflag ) {
			     idname->cdb_info.info.buildname = 1;
			}
#endif /* HPCDB */
#endif /* IRIF */

			break;
			}

		case STRING:
#ifndef IRIF
			p->in.op = NAME;
			p->tn.lval = 0;
#else /* IRIF */
		        p->tn.lval = (int) addasciz( irif_string_buf );
#endif /* IRIF */
			p->in.type = CHAR+ARY;
			p->tn.rval = NOLAB;
			p->nn.sym = NULL;
			/* size set by lxEndStr */
#ifndef IRIF
			p->fn.cdim = (inlineflag ? curdim-1 : curdim);
#else /* IRIF */
                        p->fn.cdim = curdim;
#endif /* IRIF */
			p->fn.csiz = CHAR;
			break;

#ifdef ANSI
		case WIDESTRING:
			p->in.op = NAME;
			p->in.type = WIDECHAR+ARY;
			p->tn.lval = 0;
			p->tn.rval = NOLAB;
			p->nn.sym = NULL;
			p->fn.cdim = curdim;
			p->fn.csiz = WIDECHAR;
			break;

#endif
		case FCON:
			p->tn.lval = 0;
			p->tn.rval = 0;
			p->in.type = DOUBLE;
			p->fn.cdim = 0;
			p->fn.csiz = DOUBLE;
			break;

		case STREF:
#ifdef IRIF
		case DOT:
#endif /* IRIF */
			{
			register struct symtab *sp;
			register j;
			register char *name, *member;
			int isdot = (r->tn.rval == DOT);

			/* the RHS node ***IS ASSUMED TO BE*** NAMESTRING 
			 * the LHS a structure/union
			 */

			r->in.op = FREE;

#ifndef IRIF
			if( ( l->in.type != PTR+STRTY ) &&( l->in.type != PTR+UNIONTY )) {
			     /* "struct/union %s required before '%s'" */
			     if( (r->tn.rval == DOT ))
			          UERROR( MESSAGE( 180 ), "name", "." );
			     else
			          UERROR( MESSAGE( 180 ), "pointer", "->" );
			     break;
			}
#else /* IRIF */
			if( (r->tn.rval == DOT )) {
			     if( ( l->in.type != STRTY ) &&( l->in.type != UNIONTY )) {
				  /* "struct/union %s required before '%s'" */
				  UERROR( MESSAGE( 180 ), "name", "." );
				  break;
			     }
			}
			else 
			     if( ( l->in.type != PTR+STRTY ) &&( l->in.type != PTR+UNIONTY )) {
				  /* "struct/union %s required before '%s'" */
			          UERROR( MESSAGE( 180 ), "pointer", "->" );
				  break;
			     }
#endif /* IRIF */
			if( l->fn.csiz <= 0 ) {
			     /* formerly uerror("undefined structure or union") */
			     cerror("panic in buildtree: undefined struct for STREF");
			     break;
			   }

			/* search structure for member with name */

			for( j = dimtab[l->fn.csiz+1];
			     (sp=(struct symtab *) dimtab[j]) != NULL; 
			     ++j ){
			        
			        register k;

				name = (char *)r->tn.lval;
				member = sp->sname;
				for( k=0; k<NCHNAM; ++k ){
				     if(*name++!=*member)
				          goto trynext;
				     if(!*member++) break;
				   }
				/* found it! */
				idname = sp;
				break;

				trynext: continue;
			      }

			if( sp == NULL ) {
				/* "struct/union does not contain member: %s" */
				sp = (struct symtab *) dimtab[l->fn.csiz+3];
				if (sp && sp->sname)
					UERROR( MESSAGE( 235 ), 
						(j==0)?"incomplete ":"",
						sp->sname,
						(char *)r->tn.lval);
				else
					UERROR( MESSAGE( 167 ),
						(j==0)?"incomplete ":"",
						(char *)r->tn.lval);
				break;
			      }
			/* This NAMESTRING case escapes the normal setting */
                        /* of suse in the scanner, so set it here          */
			sp->suse = lineno;
# ifdef SA
			if (saflag)
			   add_xt_info(sp, USE);
# endif			
			p->in.right = buildtree( NAME, NIL, NIL );

			/* copy up storage class information */
			p->tn.tattrib = p->in.left->tn.tattrib;
#ifdef ANSI
			if( r->tn.rval == DOT) {
			    /* reference is not an lvalue if LHS is not */
			    if( p->in.left->in.not_lvalue )
				p->in.not_lvalue = p->in.left->in.not_lvalue;
			} else {
			    /* rhs of -> is always an lvalue */
			    p->in.not_lvalue = 0;
			}
			/* set the lval bit for later use in notlval */
			if (p->in.left->in.op == QUEST)
			    p->in.left->in.not_lvalue = p->in.not_lvalue;
#endif /*ANSI*/
#ifndef IRIF
			p = stref( p, isdot );
#else /* IRIF */
			p = stref( p );
#endif /* IRIF */
			break;
			}

		case UNARY MINUS:
		case COMPL:
			/* Promote char, short, unsigned char and unsigned short to int */
			/* Fix to ANSI conformance problem, DTS#FSDdt08969. */
			if (
#ifdef ANSI
/* only do this test in ANSI mode.  this test broke K&R mode because the
   conversion to unsigned int later on assumed l->in.type was unsigned.
   we may want to change the K&R signed case later to handle promotion
   to int, but this allows the behavior to remain what it was in 9.0 and
   before.
*/
			    (l->in.type == CHAR) || (l->in.type == SHORT) || 
#endif
                            (l->in.type == UCHAR) || (l->in.type == USHORT)) {
				register NODE *mr;
#ifdef ANSI
				mr = block( SCONV,l,NIL,INT,0,INT);
				mr = clocal(mr);
				p->in.left = mr;
				p->in.type = INT;
#  ifdef LINT
				p->in.promote_flg = 1;
#  endif
#else
				mr = block( SCONV,l,NIL,UNSIGNED,0,UNSIGNED);
				mr = clocal(mr);
				p->in.left = mr;
				p->in.type = UNSIGNED;
#endif
			} else {
#ifdef IRIF
				if( l->in.op == ICON && o == UNARY MINUS ) {
				     p->in.op = FREE;
				     p  = l;
				     p->tn.lval = -p->tn.lval;
				}
				else {
#endif /* IRIF */
				p->in.type = l->in.type;
				p->fn.cdim = l->fn.cdim;
				p->fn.csiz = l->fn.csiz;
#ifdef IRIF
				}
#endif /* IRIF */
			}
			break;

#ifdef ANSI
		case UNARY PLUS:
			/* Promote char, short, unsigned char and unsigned short to int 
			   and get rid of U+ operator */
			/* Fix to ANSI conformance problem, DTS#FSDdt08969. */
			
			p->in.op = FREE;

			if ((l->in.type == CHAR) || (l->in.type == SHORT) ||
                            (l->in.type == UCHAR) || (l->in.type == USHORT)) {
				p = block( SCONV,l,NIL,INT,0,INT);
				p = clocal(p);
#ifdef LINT
				p->in.promote_flg = 1;
#endif
			} else 
			        p = l;
			break;
#endif 
		case UNARY MUL:

#ifndef IRIF 
			if( l->in.op == UNARY AND ){
				p->in.op = FREE;
				l->in.op = FREE;
				p = l->in.left;
				}
#endif /* not IRIF */
			/* "illegal indirection" */
			if ( !ISPTR(l->in.type) || (l->in.type == VOIDPTR)) 
			        UERROR( MESSAGE( 60 ) );
#ifdef IRIF
		        /* rewrite    *     to       *
			 *            |              |
			 *            +            INDEX
                         *           / \            / \
                         *        WAS_  value    WAS_  value
			 *        ARRAY          ARRAY
			 *
			 * These arise from, e.g.,
			 *        int a[];
			 *        *(a+i);
			 */
		        if(  (l->in.op == PLUS || l->in.op == MINUS)
			   && WAS_ARRAY( l->in.left->in.tattrib )) {
			     
			     if( l->in.op == MINUS ) {
				  l = buildtree( INDEX,
						l->in.left,
						buildtree( UNARY MINUS,
							  l->in.right,
							  NIL ));
			     }
			     else {
				  l = buildtree( INDEX, l->in.left, l->in.right );
			     }	
			     p->in.left->in.op = FREE;
			     p->in.left = l;
			     l->in.tattrib = 
				  l->in.left->in.tattrib | ATTR_NEXT_DIM;
			}
#endif /* IRIF */
			p->in.type = DECREF(l->in.type);
                        p->in.tattrib = DECREF(l->in.tattrib) & (~ATTR_CLASS);
			p->fn.cdim = l->fn.cdim;
			p->fn.csiz = l->fn.csiz;
			break;

		case UNARY AND:
#ifdef ANSI
			/* strip out () nodes where necessary */
			while ( l->in.op == PAREN ) {
				l->in.op = FREE;
				p->in.left = l = l->in.left;
			}
#endif

			switch( l->in.op ){

			case UNARY MUL:
				p->in.op = FREE;
				l->in.op = FREE;
# ifdef C1_C
#ifndef IRIF
        /* Retain tag information for cases like int a[5]; foo(a); */
        /*   which causes U&(U*(+ %a6, ICON)                       */
                                if (l->in.fixtag) l->in.left->in.fixtag = 1;
#endif /* not IRIF */
# endif /* C1_C */
				p = l->in.left;
				/* fall thru */
#ifdef IRIF
			case DOT:
#ifdef ANSI
				if( l->in.not_lvalue ) {
				     p->in.not_lvalue = l->in.not_lvalue;
				}
#endif /* ANSI */
				/* fall thru */
			case STRING:
			case STREF:
#endif /* IRIF */
			case NAME:
				p->in.type = INCREF( l->in.type );
				p->fn.cdim = l->fn.cdim;
				p->fn.csiz = l->fn.csiz;
				p->in.tattrib = INCQUAL( l->in.tattrib ) & (~ATTR_CLASS);
				break;

			case COMOP:
				lr = buildtree( UNARY AND, l->in.right, NIL );
				p->in.op = FREE;
				l->in.op = FREE;
				p = buildtree( COMOP, l->in.left, lr );
				break;

			case QUEST:
				lr = buildtree( UNARY AND, l->in.right->in.right, NIL );
				ll = buildtree( UNARY AND, l->in.right->in.left, NIL );
				p->in.op = FREE;
				l->in.op = FREE;
				l->in.right->in.op = FREE;
				p = buildtree( QUEST, l->in.left, 
						buildtree( COLON, ll, lr ) );
				break;
#ifdef IRIF
			case CALL:
			case UNARY CALL:
				if( l->in.type == STRTY || l->in.type == UNIONTY ) {
				     if( p->in.left->in.left->in.op == PCONV ) {
					  adjust_su_ftnname_cast( p->in.left );
				     }
				     p->in.op = FREE;
				     p = p->in.left;
				}
				else {
				     /* "unacceptable operand of &" */
				     UERROR( MESSAGE( 110 ) );
				}
				break;
#else /* not IRIF */
			case REG:
				if (unionflag && l->in.type == UNIONTY)
					{
					/* Can only get into this configuration
					   if unionflag is set.
					*/
					p->in.op = FREE;
					p = l;
					break;
					}
				/* fall thru */
#endif /* not IRIF */
			default:
				/* "unacceptable operand of &" */
				UERROR( MESSAGE( 110 ) );
				break;
				}
			break;
#ifndef ANSI
		case ASG LS:
		case ASG RS:
		case LS:
		case RS:
			{
			register NODE *q = p->in.right;
#ifndef IRIF
			if(tsize(q->in.type, q->fn.cdim, q->fn.csiz, NULL, 0) > SZINT) /* SWFfc00726 fix */
#else /* IRIF */
			if( ir_t_bytesz( q ) > SZINT/SZCHAR )
#endif /* IRIF */
				p->in.right = makety(q, INT, 0, INT, p->in.tattrib );
			break;
			}
#endif 
#ifndef IRIF
		case ASSIGN:
		case RETURN:
			/* structure assignment */
			/* take the addresses of the two sides; then make an
			/* operator using STASG and
			/* the addresses of left and right */

			{
			flag lreg, rreg;
			TWORD type;

			if( l->fn.csiz != r->fn.csiz ) 
			     /* "%s types incompatible, op %s" */
			     UERROR( MESSAGE( 171 ), opst[o], "structure/union" );

			r = buildtree( UNARY AND, r, NIL );
			l = block( STASG, l, r, r->in.type, r->fn.cdim,
					r->fn.csiz );
			l->in.tattrib = r->in.tattrib;
			l->in.tattrib = r->in.tattrib;

			if( o == RETURN )
				{
				p->in.op = FREE;
				p = l;
				break;
				}

			p->in.op = UNARY MUL;
			p->in.left = l;
			p->in.right = NIL;
			break;
			}
#endif /* not IRIF */
		case COLON:
			/* structure colon */

			if( l->fn.csiz != r->fn.csiz )
			     /* "operands of '%s' have incompatible or 
				 incorrect types" */
			     UERROR( MESSAGE( 89 ), ":" );
			break;


		case CALL:
#ifndef ANSI
#ifndef IRIF
			p->in.right = r = strargs( p->in.right );
#else /* IRIF */
		        r = p->in.right;
#endif /* IRIF */

			/* Check the first param for VOID type calls. Succeeding
			   params will be checked via CM op.
			*/
			checkvoidparam(r);

			if (r->in.type == FLOAT && !singleflag)
				/* only 1 parameter */
				p->in.right = makety(p->in.right, DOUBLE, 0,
							DOUBLE, p->in.tattrib);
			else pfltconv(p->in.right);
#endif /* not ANSI */
                /* fall thru */
		case UNARY CALL:
			/* "function designator/pointer expected%s%s%s" */
			if( !ISPTR(l->in.type)) {
			     if( l->nn.sym == NULL || l->nn.op == FCON)
				  UERROR( MESSAGE( 58 ), "", "", "");
			     else
				  UERROR( MESSAGE( 58 ), 
					  " for '", l->nn.sym->sname, "'" );
			   break;
			   }
			p->in.type = DECREF(l->in.type);
                        p->in.tattrib = DECREF(l->in.tattrib) & (~ATTR_CLASS);
			/* "function designator/pointer expected%s%s%s" */
			if( !ISFTN(p->in.type))  {
			     if( l->nn.sym == NULL )
				  UERROR( MESSAGE( 58 ), "", "", "" );
			     else
				  UERROR( MESSAGE( 58 ), 
					  " for '", l->nn.sym->sname, "'" );
			   break;
			   }
			p->in.type = DECREF( p->in.type );
			p->fn.csiz = l->fn.csiz;
                        p->in.tattrib = DECREF(p->in.tattrib) & (~ATTR_CLASS);

#ifdef SEQ68K
			if (p->in.type == FLOAT) {
				p->in.type = DOUBLE;
				p->fn.csiz = DOUBLE;
			}
#endif

			p->fn.cdim = l->fn.cdim+1;   /* skip past prototype */
#ifdef ANSI
			check_and_convert_args(p, narg);
#endif

#ifndef IRIF
# ifdef ANSI
			if( p->in.type == STRTY || p->in.type == UNIONTY ||
                            p->in.type == LONGDOUBLE) {
# else
			if( p->in.type == STRTY || p->in.type == UNIONTY ){
# endif /* ANSI */
				/* function returning structure */
				/*  make function really return ptr to str., with * */

				p->in.op += STCALL-CALL;
				p->in.type = INCREF( p->in.type );
				p = buildtree( UNARY MUL, p, NIL );

				}
#ifndef LINT
			if ((p->in.left->in.op == (UNARY AND)) &&
			    (p->in.left->in.left->in.op == NAME) &&
			    inlineflag && (optlevel == 3))
                                if (strcpy_inline_enabled &&
				    (strcmp(p->in.left->in.left->nn.sym->sname,
						"strcpy") == 0) )
                                        strinline(p);
                                else if (!flibflag)
                                        monadicize(p);
#endif /* LINT */

#else /* IRIF */
			if ((p->in.left->in.op == (UNARY AND)) &&
			    (p->in.left->in.left->in.op == NAME) &&
			    !strcmp( p->in.left->in.left->nn.sym->sname,
				    "__builtin_va_start" ))
			     /*
			      * indicates 'va_start' macro (varargs)
			      */
			     p->in.op = VA_START;
#endif /* IRIF */
			break;

# ifdef ANSI
		case ARGASG:
			/* New node type used to aid argument conversion. 
			 * what we really want is the rhs. 
			 * Free the root and the lhs (which should be
			 * just a single NAME node.
			 */
			if (((l->fn.type == UNIONTY)||(l->fn.type == STRTY))&&
			    (l->fn.csiz != r->fn.csiz)){
				UERROR( MESSAGE( 229 ), arg_fname, arg_name );
			}
			p->in.op = FREE;
			tfree(p->in.left);
			return(clocal(p->in.right));
# endif
		case PAREN:
			p->in.type = l->in.type;
			p->in.tattrib = l->in.tattrib;
			p->fn.csiz = l->fn.csiz;
			p->fn.cdim = l->fn.cdim;
			break;
		case CM:	/* comma in parameter list */
#ifndef ANSI
			checkvoidparam(l);
			checkvoidparam(r);
#endif
			break;

		case INIT:
		case ANDAND:
		case OROR:
			 if ( !ISSCALAR( r->in.type ) )
				{
		                /* "operands of %s should have %s type" */
		                UERROR( MESSAGE( 164 ), opst[o], "scalar" );
				break;
				}
			/* fall thru */
		case NOT:
			if ( !ISSCALAR( l->in.type ) )
		                /* "operands of %s should have %s type" */
		                UERROR( MESSAGE( 164 ), opst[o], "scalar" );
			break;
		
		case QUEST:
#ifdef ANSI
		        /* inherit attributes of COLON (except ATTR_REG) */
		        p->in.tattrib = r->in.tattrib & 
			                ~( ATTR_CLASS & ~ATTR_WAS_ARRAY );
			/* QUEST is not an lvalue */
			p->in.not_lvalue = 1;
#endif
			if ( !ISSCALAR( l->in.type ) )
			     /* "left operand of ? should have scalar type" */
                	     UERROR( MESSAGE( 48 ));
			break;

		default:
			cerror( "other code %x", o );
			}

		}

	if( actions & CVTO ) p = oconvert(p);

# ifdef DEBUGGING
	if( bdebug > 2 )
		{
		(void)prntf("At end of buildtree before calling clocal:\n");
		fwalk( p, eprint, 0 );
		}
# endif	/* DEBUGGING */

	p = clocal(p);

# ifdef DEBUGGING
	if( bdebug )
		{
		(void)prntf("At end of buildtree after calling clocal:\n");
 		fwalk( p, eprint, 0 );
		}
# endif /* DEBUGGING */

	return(p);

	}

#ifdef IRIF
/*************************************************************************
 * check_INDEX () : handles portions of array indexing for 'buildtree()'
 *                  for actions TYPL or TYPR
 *
 * 'p'            : tree node
 * 'q'            : p->in.left (TYPL) or p->in.right (TYPR)
 *
 * returns        : ptr to updated node
 */
NODE *check_INDEX( p, q ) NODE *p, *q; {

     if( q->in.op == INDEX ) {
	  if( q->in.tattrib & ATTR_NEXT_DIM ) {

	       /* The type of 'p' must be rewritten to reflect that it is
		* [or is soon to be rewritten into] an INDEX node.
		*
		* If 'p' is PLUS or MINUS [rather than INDEX] we have a
		* case like
		*
		*      int a[2][2];
		*      a[1]+7;
		* with the tree
		*
		*                                      +
		*                                     / \
		*                                    /   \
		*  (INDEX for the 1st dim)        INDEX   7
		*                                  / \
		*                                 /   \
		*                                a     1
		*
		* The '+' is the 2nd dim index.  We rewrite as
		*
		*  (INDEX for the 2nd dim)        INDEX
		*                                  / \
		*                                 /   \
		*  (INDEX for the 1st dim)     INDEX   7    
		*                               / \
		*                              /   \
		*                             a     1
		*/
	       switch( p->in.op ) {
		  case MINUS: {
		       NODE **minus = ISPTR( p->in.left->in.type ) ?
			                  &p->in.right : &p->in.left; 
		       *minus = buildtree( UNARY MINUS, *minus, 
					  NIL );
		  }

		    /*fall thru*/
		  case PLUS:
		    p->in.op = INDEX;
		    /* for PLUS or MINUS, the next PLUS or MINUS is not
		     * in the next dimension
		     */
		    p->in.tattrib &= ~ATTR_NEXT_DIM;

		    /*fall thru */
		  case INDEX:
		    p->in.type = INCREF( DECREF( DECREF (q->in.type)));
		    p->fn.cdim = q->fn.cdim + 1;
	       }
	  }
	  else { /* q->in.op == INDEX but not ( q->in.tattrib & ATTR_NEXT_DIM ) */

	       switch( p->in.op ) {
		  case INDEX: 
		    /* cases like
		     *         (a[1]+2)[3]
		     * The first index is 1, the second 2+3.  We have to
		     * get rid of the latest INDEX node and turn on 
		     * ATTR_NEXT_DIM as any further indexing will be for
		     * dimension 3.
		     */
		    q->in.tattrib &= ATTR_NEXT_DIM;

		    /* fall thru */
		  case MINUS:
		  case PLUS: {
		       /* cases like
			*      int a[2][2];
			*      i+a[1]+7;
			* with the tree
			*
			*                                      +
			*                                     / \
			*                                    /   \
			*  (INDEX for the 2nd dim)        INDEX   7
			*                                  / \
			*                                 /   \
			*  (INDEX for the 1st dim)     INDEX   i    
			*                               / \
			*                              /   \
			*                             a     1
			*
			* We can't introduce another INDEX node as '7'
			* is part of the 2nd dim index.  We rewrite as
			*
			*         INDEX
			*          / \
			*         /   \
			*      INDEX  i+7
			*       / \
			*      /   \
			*     a     1
			*/
		       NODE **notindexq =  ISPTR( q->in.left->in.type ) ?
			                         &q->in.right :
				                 &q->in.left;
		       *notindexq = buildtree( p->in.op == INDEX ? PLUS : p->in.op, 
					      *notindexq,
					      ISPTR( p->in.left->in.type ) ?
					      p->in.right :
					      p->in.left );
		       p->in.op = FREE;
		       p = q;
		  }
	       }
	  }
     }
     else { /* q->in.op != INDEX */
	  if(   WAS_ARRAY( q->in.tattrib )
	     && ( q->in.op == PLUS || q->in.op == MINUS || 
		 ( q->in.op == NAME && !ISARY( DECREF( q->in.type ))))) {

	       p->in.type = INCREF( DECREF( DECREF( q->in.type )));
	  }
	  else {
	       p->in.type = q->in.type;
	       p->fn.cdim = q->fn.cdim;
	  }
     }
     return( p );
}
#endif /* IRIF */
#ifndef IRIF
LOCAL int fixstasg( p ) register NODE *p; {
	register NODE *l,*r;
	int lreg,rreg;
	if (p->in.op == UNARY MUL && p->in.left->in.op == STASG){
		l = p->in.left->in.left;
		r = p->in.left->in.right;
		lreg = l->in.op==REG && !ISPTR(l->in.type);
		rreg = r->in.op==REG && !ISPTR(r->in.type);
		if (rreg && !lreg){
			p->in.left->in.op = FREE;
			p->in.op = ASSIGN;
			l->in.type = r->in.type = p->in.type = INT;
			p->in.left = l;
			p->in.right = r;
		} else if (lreg && !rreg) {
			p->in.left->in.op = FREE;
			p->in.op = ASSIGN;
			r = block(UNARY MUL,r,0,INT,0,INT,0);
			l->in.type = r->in.type = p->in.type = INT;
			p->in.left = l;
			p->in.right = r;
		}
	}
}
#endif
#endif /*BRIDGE*/

/* t1copy() is a (sub)tree copy routine for trees in the shape of pass1 trees */
NODE *t1copy(p) NODE *p;
{
	register NODE *q;

	q = talloc();
#ifdef OSF
	/* This should not be neccessary, but without it OSF lint fails. */
	/* when linting the HP-UX cb command - sje			 */
	memcpy(q,p,sizeof(NODE));
#else
	*q = *p;
#endif /* OSF */
	switch( optype(q->in.op) )
		{
		case BITYPE:
			q->in.right = t1copy(q->in.right);
			/* fall thru */
		case UTYPE:
			q->in.left = t1copy(q->in.left);
		}
	return (q);
}

#ifndef BRIDGE

/* checkvoidparam() checks a function parameter to ensure that it is not
   an expression with type void.
*/
LOCAL checkvoidparam(p)	register NODE *p;
{
	if (p->in.op != CM && p->in.type == VOID) UERROR( MESSAGE( 118 ));
}


#ifndef ANSI

#ifndef IRIF
LOCAL NODE *
strargs( p ) register NODE *p;  { /* rewrite structure flavored arguments */
	register TWORD ty;

	if( p->in.op == CM ){
		p->in.left = strargs( p->in.left );
		p->in.right = strargs( p->in.right );
		return( p );
		}

	ty = p->in.type;
	if( ty== STRTY || ty== UNIONTY ){
		TWORD tattrib = p->in.tattrib;
		p = block( STARG, p, NIL, ty, p->fn.cdim, p->fn.csiz );
		p->in.tattrib = tattrib;
		p->in.left = buildtree( UNARY AND, p->in.left, NIL );
		p = clocal(p);
		}
	else if (ty==CHAR || ty==UCHAR || ty==SHORT || ty==USHORT)
		p = makety(p, INT, 0, INT, p->in.tattrib);

	return( p );
	}
#endif /* not IRIF */



/* pfltconv checks an actual parameter list for FLOAT parameters that must
   be converted to DOUBLE before the call. Recursion is avoided.
*/

LOCAL pfltconv(p) register NODE *p;
{
if (singleflag) return;
top:	if (optype(p->in.op) == BITYPE && p->in.right->in.type == FLOAT)
		p->in.right = makety(p->in.right, DOUBLE, 0 , DOUBLE, p->in.tattrib );
	if (p->in.op == CM /* comma, not COMOP */ )
	{
		if (p->in.left->in.type == FLOAT)
			p->in.left = makety(p->in.left, DOUBLE, 0, DOUBLE, p->in.left->in.tattrib );
		else /* CM type nodes are int type */
		{
			p = p->in.left;
			goto top;
		}
	}
}

conval( p, o, q ) register short o; register NODE *p, *q; {
	/* apply the op o to the lval part of p; if binary, rhs is val */
	register CONSZ val;
	flag u;
	flag qbigger;

	val = q->tn.lval;
	qbigger = ((optype(o)==BITYPE) && (p->tn.rval == NONAME) &&
	    ((BITSINTYPE(q) > BITSINTYPE(p)) ||
	     ((BITSINTYPE(q) == BITSINTYPE(p))&&(ISUNSIGNED(q->in.type)))));
	u = ISUNSIGNED(p->in.type) || ISUNSIGNED(q->in.type);
	if( u && (o==LE||o==LT||o==GE||o==GT)) o += (UGE-GE);

	if( p->tn.rval != NONAME && q->tn.rval != NONAME ) return(0);
	if( q->tn.rval != NONAME && o!=PLUS ) return(0);
	if( p->tn.rval != NONAME && o!=PLUS && o!=MINUS ) return(0);

	switch( o ){

	case PLUS:
		p->tn.lval += val;
		if( q->tn.rval != NONAME ){
			p->tn.rval = q->tn.rval;
			p->nn.sym = q->nn.sym;
			p->in.type = q->in.type;
#ifdef C1_C
#ifndef IRIF
			/* q is about to be zapped.  copy tag from q to p */
			p->in.c1tag = q->in.c1tag;
			p->in.fixtag = q->in.fixtag;
#endif /* not IRIF */
#endif  /* C1_C */
			qbigger = 0;
			}
		break;
	case MINUS:
		p->tn.lval -= val;
		break;
	case MUL:
		if (u) p->tn.lval = (unsigned) p->tn.lval * val;
		else p->tn.lval *= val;
		break;
	case DIV:
		/* "division by 0" */
		if( val == 0 ) UERROR( MESSAGE( 31 ) );
		else if (u) p->tn.lval = (unsigned) p->tn.lval / val;
		else p->tn.lval /= val;
		break;
	case MOD:
		/* "division by 0" */
		if( val == 0 ) UERROR( MESSAGE( 31 ) );
		else if (u) p->tn.lval = (unsigned) p->tn.lval % val;
		else p->tn.lval %= val;
		break;
	case AND:
		p->tn.lval &= val;
		break;
	case OR:
		p->tn.lval |= val;
		break;
	case ER:
		p->tn.lval ^=  val;
		break;
	case LS:
		p->tn.lval <<= val;
		qbigger = 0;
		break;
	case RS:
		if (u) p->tn.lval = (unsigned) p->tn.lval >> val;
		else p->tn.lval >>= val;
		qbigger = 0;
		break;

	case UNARY MINUS:
		p->tn.lval = - p->tn.lval;
		break;
	case COMPL:
		p->tn.lval = ~p->tn.lval;
		break;
	case NOT:
		p->tn.lval = !p->tn.lval;
		break;
	case LT:
		p->tn.lval = p->tn.lval < val;
		break;
	case LE:
		p->tn.lval = p->tn.lval <= val;
		break;
	case GT:
		p->tn.lval = p->tn.lval > val;
		break;
	case GE:
		p->tn.lval = p->tn.lval >= val;
		break;
	case ULT:
		p->tn.lval = ((unsigned)p->tn.lval) < ((unsigned)val);
		break;
	case ULE:
		p->tn.lval = ((unsigned)p->tn.lval) <= ((unsigned)val);
		break;
	case UGE:
		p->tn.lval = ((unsigned)p->tn.lval) >= ((unsigned)val);
		break;
	case UGT:
		p->tn.lval = ((unsigned)p->tn.lval) > ((unsigned)val);
		break;
	case EQ:
		p->tn.lval = p->tn.lval == val;
		break;
	case NE:
		p->tn.lval = p->tn.lval != val;
		break;
	case ANDAND:
		p->tn.lval = (p->tn.lval && val);
		break;
	case OROR:
		p->tn.lval = (p->tn.lval || val);
		break;
	default:
		return(0);
		}

	if (qbigger) p->in.type = q->in.type;
	/* the constant may have grown */
	switch (p->in.type)
		{
		case CHAR:
			if (p->tn.lval < -128 || p->tn.lval > 127)
				p->tn.type = SHORT;
			/* fall thru */
		case SHORT:
			if (p->tn.lval < -32768 || p->tn.lval > 32767)
				p->tn.type = INT;
			/* fall thru */
		}
	return(1);
	}

#else /*ANSI*/

/* we may wish to get these defines from <limits.h> */
#define INT_MIN ( -2147483647 - 1)
#define INT_MAX 2147483647

conval( l, o, r ) register short o; register NODE *l, *r; {
	/* fold constants, apply op "o" to "l" and "r" 
	 *
	 * requires: l and r must both be ICON or FCON nodes
	 *           cleanup happens after this routine is called.
	 *
	 * effects: modifies the value in the l node to be the folded value.
	 *          returns 1 on success, 0 on failure.
	 */
        NODE *p;
	double dtmp;
	register is_quad = (l->in.type == LONGDOUBLE);
	register is_float = (ISFTP(l->in.type) && (!is_quad));
	QUAD zero_quad;

	zero_quad.d[0] = 0; zero_quad.d[1] = 0;

	if ((!is_float) && (!is_quad)) {
	        if (l->tn.rval != NONAME && r->tn.rval != NONAME) return(0);
		if (l->tn.rval != NONAME && o != PLUS && o != MINUS) return(0);
		if (r->tn.rval != NONAME && o != PLUS ) return(0);
		}

	switch( o ){

	case PLUS:
		if (is_float) l->fpn.dval += r->fpn.dval;
#ifdef QUADC
		else if (is_quad) l->qfpn.qval = _U_Qfadd(l->qfpn.qval,r->qfpn.qval);
#endif
		else {
			if (ISUNSIGNED(l->in.type)){
				l->tn.lval += (unsigned) r->tn.lval;
			} else {
				/* do in a double to detect overflow */
				constant_folding = 1;
				dtmp = ((double) l->tn.lval +
				        (double) r->tn.lval);
				if (dtmp > INT_MAX || dtmp < INT_MIN)
					/* integer overflow in constant expression */
					WERROR( MESSAGE(196) );
				l->tn.lval = dtmp;
			}
			constant_folding = 0;
			if (l->tn.rval == NONAME) {
				l->nn.sym = r->nn.sym;
				l->in.type = r->in.type;
				l->tn.rval = r->tn.rval;
#ifdef C1_C
#ifndef IRIF
				/* r is about to be zapped, copy tag */
				l->in.c1tag = r->in.c1tag;
				l->in.fixtag = r->in.fixtag;
#endif /* not IRIF */
#endif
				}
		}
		break;
	case MINUS:
		if (is_float) l->fpn.dval -= r->fpn.dval;
#ifdef QUADC
		else if (is_quad) l->qfpn.qval = _U_Qfsub(l->qfpn.qval,r->qfpn.qval);
#endif
		else {
			if (ISUNSIGNED(l->in.type)){
				l->tn.lval -= (unsigned) r->tn.lval;
			} else {
				/* use a double to detect overflow */
				constant_folding = 1;
				dtmp = ((double) l->tn.lval -
				        (double) r->tn.lval);
				if (dtmp > INT_MAX || dtmp < INT_MIN)
					/* integer overflow in constant expression */
					WERROR( MESSAGE(196) );
				l->tn.lval = dtmp;
			}
			constant_folding = 0;
			if (l->tn.rval == NONAME) {
				l->nn.sym = r->nn.sym;
				l->in.type = r->in.type;
				l->tn.rval = r->tn.rval;
#ifdef C1_C
#ifndef IRIF
				/* r is about to be zapped, copy tag */
				l->in.c1tag = r->in.c1tag;
				l->in.fixtag = r->in.fixtag;
#endif /* not IRIF */
#endif
				}
		}
		break;
	case MUL:
		if (is_float) l->fpn.dval *= r->fpn.dval;
#ifdef QUADC
		else if (is_quad) l->qfpn.qval = _U_Qfmpy(l->qfpn.qval,r->qfpn.qval);
#endif
		else if (ISUNSIGNED(l->in.type)) {
			l->tn.lval *= (unsigned) r->tn.lval;
		} else {
			/* do the arithmetic in double precision so we can
			 * detect overflow */
			constant_folding = 1;
			dtmp = ((double) l->tn.lval * (double) r->tn.lval);
			if (dtmp > INT_MAX || dtmp < INT_MIN)
				/* integer overflow in constant expression */
				WERROR( MESSAGE(196) );
			l->tn.lval = dtmp;
			constant_folding = 0;
		}
		break;
	case DIV:
		if (is_float) l->fpn.dval /= r->fpn.dval;
#ifdef QUADC
		else if (is_quad) l->qfpn.qval = _U_Qfdiv(l->qfpn.qval,r->qfpn.qval);
#endif
		/* "division by 0" */
		else if (r->tn.lval == 0) UERROR( MESSAGE( 31 ));
		else if (ISUNSIGNED(l->in.type)) {
			l->tn.lval /= (unsigned) r->tn.lval;
		}
		else {
			/* do the arithmetic in double precision so we can
			 * detect overflow */
			constant_folding = 1;
			dtmp = ((double) l->tn.lval / (double) r->tn.lval);
			if (dtmp > INT_MAX || dtmp < INT_MIN)
				/* integer overflow in constant expression */
				WERROR( MESSAGE(196) );
			l->tn.lval = dtmp;
			constant_folding = 0;
		}
		break;
	case MOD:
		/* "division by 0" */
		if (r->tn.lval == 0) UERROR( MESSAGE( 31 ));
		else if (ISUNSIGNED(l->in.type)) {
			l->tn.lval %= (unsigned) r->tn.lval;
		} else {
			/* do the arithmetic in double precision so we can
			 * detect overflow */
			extern double fmod();
			constant_folding = 1;
			dtmp = fmod((double) l->tn.lval,(double) r->tn.lval);
			if (dtmp > INT_MAX || dtmp < INT_MIN)
				/* integer overflow in constant expression */
				WERROR( MESSAGE(196) );
			l->tn.lval = dtmp;
			constant_folding = 0;
		}
		break;
	case AND:
		l->tn.lval &= r->tn.lval;
		break;
	case OR:
		l->tn.lval |= r->tn.lval;
		break;
	case ER:
		l->tn.lval ^=  r->tn.lval;
		break;
	case LS:
		l->tn.lval <<= r->tn.lval;
		break;
	case RS:
		if (ISUNSIGNED(l->in.type))
			l->tn.lval = (unsigned) l->tn.lval >> r->tn.lval;
		else
			l->tn.lval >>= r->tn.lval;
		break;
	case UNARY MINUS:
		if (is_float) l->fpn.dval = - r->fpn.dval;
#ifdef QUADC
		else if (is_quad) l->qfpn.qval = _U_Qfneg(l->qfpn.qval);
#endif
		else {
#if 0
			if (!ISUNSIGNED(r->in.type) && (r->tn.lval == INT_MIN))
				/* integer overflow in constant expression */
				WERROR( MESSAGE(196) );
#endif
			l->tn.lval = - r->tn.lval;
			}
		break;
	case UNARY PLUS:
		break;
	case COMPL:
		l->tn.lval = ~r->tn.lval;
		break;
	case NOT:
		if (is_float)
			{
			p = bcon(!r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			}
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qflogneg(r->qfpn.qval),INT);
			*l = *p; p->in.op = FREE;
			}
#endif
		else l->tn.lval = !r->tn.lval;
		break;
	case LT:
		if (is_float)
			{
		        p = bcon(l->fpn.dval<r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			} 
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qfcmp(l->qfpn.qval,r->qfpn.qval,LDLT),INT);
			*l = *p; p->in.op = FREE;
			} 
#endif
		else l->tn.lval = l->tn.lval < r->tn.lval;
		break;
	case LE:
		if (is_float)
			{
		        p = bcon(l->fpn.dval<=r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			}
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qfcmp(l->qfpn.qval,r->qfpn.qval,LDLT|LDEQ),INT);
			*l = *p; p->in.op = FREE;
			}
#endif
		else l->tn.lval = l->tn.lval <= r->tn.lval;
		break;
	case GT:
		if (is_float)
			{
		        p = bcon(l->fpn.dval>r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			}
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qfcmp(l->qfpn.qval,r->qfpn.qval,LDGT),INT);
			*l = *p; p->in.op = FREE;
			}
#endif
		else l->tn.lval = l->tn.lval > r->tn.lval;
		break;
	case GE:
		if (is_float)
			{
		        p = bcon(l->fpn.dval>=r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			}
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qfcmp(l->qfpn.qval,r->qfpn.qval,LDGT|LDEQ),INT);
			*l = *p; p->in.op = FREE;
			}
#endif
		else l->tn.lval = l->tn.lval >= r->tn.lval;
		break;
	case ULT:
		l->tn.lval = ((unsigned)l->tn.lval) < ((unsigned)r->tn.lval);
		break;
	case ULE:
		l->tn.lval = ((unsigned)l->tn.lval) <= ((unsigned)r->tn.lval);
		break;
	case UGE:
		l->tn.lval = ((unsigned)l->tn.lval) >= ((unsigned)r->tn.lval);
		break;
	case UGT:
		l->tn.lval = ((unsigned)l->tn.lval) > ((unsigned)r->tn.lval);
		break;
	case EQ:
		if (is_float)
			{
		        p = bcon(l->fpn.dval==r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			}
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qfcmp(l->qfpn.qval,r->qfpn.qval,LDEQ),INT);
			*l = *p; p->in.op = FREE;
			}
#endif
		else l->tn.lval = l->tn.lval == r->tn.lval;
		break;
	case NE:
		if (is_float)
			{
		        p = bcon(l->fpn.dval!=r->fpn.dval,INT);
			*l = *p; p->in.op = FREE;
			}
#ifdef QUADC
		else if (is_quad)
			{
			p = bcon(_U_Qfcmp(l->qfpn.qval,r->qfpn.qval,LDLT|LDGT),INT);
			*l = *p; p->in.op = FREE;
			}
#endif
		else l->tn.lval = l->tn.lval != r->tn.lval;
		break;
	case ANDAND:
		if (l->tn.lval && r->tn.lval) l->tn.lval = 1;
		else l->tn.lval = 0;
		break;
	case OROR:
		if (l->tn.lval || r->tn.lval) l->tn.lval = 1;
		else l->tn.lval = 0;
		break;
	default:
		return(0);
		}
	return(1);
	}
#endif

#ifdef IRIF
/****************************************************************************
 * ptrcvt () : removes ARRAY designation from immediate subtype
 *
 * 'type'    : type [assumed to be a pointer type]
 *
 * i.e., 'ptr->array-of-char' becomes 'ptr->char'
 */
ptrcvt( type ) TWORD type; {
     
     for( type = DECREF( type ); ISARY( type ); type = DECREF( type )) ;
     return( INCREF( type ));
}
#endif /* IRIF */

#ifndef ANSI
LOCAL chkpun(p) register NODE *p; {

	/* checks p for the existance of a pun */

	/* this is called when the op of p is ASSIGN, RETURN, CAST, COLON,
	   or relational
	*/

	/* one case is when enumerations are used: this applies only to lint */
	/* in the other case, one operand is a pointer, the other integer type */
	/* we check that this integer is in fact a constant zero... */

	/* in the case of ASSIGN, any assignment of pointer to integer is illegal */
	/* this falls out, because the LHS is never 0 */

	register NODE *q;
	register TWORD t1, t2;
	register d1, d2;

#ifndef IRIF
	t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
#else /* IRIF */
	t1 = ptrcvt( p->in.left->in.type );
	t2 = ptrcvt( p->in.right->in.type );
#endif /* IRIF */	

	if (t1==ENUMTY || t2==ENUMTY) {
		/* This section used to contain code that performed three types
		 * of checking on enumerations:
		 *    1. The only logical operators allowed are "==" and "!="
		 *    2. If an operator contained two enums, they must be the
		 *       same type.
		 *    3. If the right hand type were an enumeration, then
		 *       it must be the result of a logical op
		 * ANSI has since said, that enumerations are interchangeable
		 * with ints, so these warnings have been removed.  In the
		 * long run, we should fix up the cases in "opact" where this
		 * routine gets called.
		 */
		if (!(ISPTR(t1) || ISPTR(t2))) return;
	}

	if( ISPTR(t1) || ISARY(t1) ) q = p->in.right;
	else q = p->in.left;

	if( !ISPTR(q->in.type) && !ISARY(q->in.type) ){
		if( q->in.op != ICON || q->tn.lval != 0 ){
			/* "illegal combination of pointer and integer, op %s" */
			WERROR( MESSAGE( 53 ), opst[p->in.op] );
			}
		}
	else {
		d1 = p->in.left->fn.cdim;
		d2 = p->in.right->fn.cdim;
		for( ;; ){
			/* ANSI C allows pointers to VOID */
			if ( (t1 == VOIDPTR) || (t2 == VOIDPTR)) return;
			if( t1 == t2 ) {;
				if(p->in.left->fn.csiz!=p->in.right->fn.csiz) {
					/* "incompatible %s types use as 
					    operands of '%s" */
					WERROR( MESSAGE( 171 ), 
					        opst[ p->in.op ],
					        "structure/union pointer" );
					}
				return;
				}
#ifdef LINT

			/* changes 10/23/80 - complain about pointer casts if cflag
			 *	is set (this implies pflag is also set)
			 */
			if ( p->in.op == CAST ) /* this implies cflag is set */
				if( ISPTR(t1) && ISPTR(t2) ) {
					/* pointer casts may be troublesome */
					WERROR( MESSAGE( 98 ) );
					return;
				}

#endif
			if( ISARY(t1) || ISPTR(t1) ){
				if( !ISARY(t2) && !ISPTR(t2) ) break;
				if( ISARY(t1) && ISARY(t2) &&
					dimtab[d1] != dimtab[d2] ){
					/* "incompatible %s types use as 
					    operands of '%s" */
					WERROR( MESSAGE( 171 ),
					        opst[ p->in.op ], "array" );
					return;
					}
				if( ISARY(t1) ) ++d1;
				if( ISARY(t2) ) ++d2;
				}
			else break;
			t1 = DECREF(t1);
			t2 = DECREF(t2);
			}
		/* "illegal pointer combination" */
		WERROR( MESSAGE( 66 ) );
		}

	}
#else /*ANSI*/
static char *basename[21] = {"","","character","short","integer",
		            "long","","","","","","",
		            "unsigned char", "unsigned short","unsigned",
		            "unsigned long","void","","signed char","",
		            "signed" };
		  
LOCAL chkpun(p) register NODE *p; {

        /* checks the branches of p for
	 *      type compatibility 
	 *
	 * it is assumed at least one of these branches is a pointer;
	 *      the other is a pointer or integer.
	 *
	 * it is assumed p->in.op is one of < <= > >= == != = : - cast
	 */

	register NODE *q;
	register TWORD t1, t2;
	int status;

#ifndef IRIF
	t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
#else /* IRIF */
	t1 = ptrcvt( p->in.left->in.type );
	t2 = ptrcvt( p->in.right->in.type );
#endif /* IRIF */	

#ifdef LINT

	/* changes 10/23/80 - complain about pointer casts if cflag
	 * is set (this implies pflag is also set)
	 */
	 if ( p->in.op == CAST ) { /* this implies cflag is set */
	      if( ISPTR(t1) && ISPTR(t2) ) 
	           /* pointer casts may be troublesome */
		   WERROR( MESSAGE( 98 ) );
	      return;
	    }
#endif

	status = iscompat(t1,p->in.left->in.tattrib,p->in.left->fn.csiz,p->in.left->fn.cdim,t2,p->in.right->in.tattrib,p->in.right->fn.csiz,p->in.right->fn.cdim,0);

	if( status & ICSTR ) {
	     /* "%s types incompatible, op %s" */
	     if( (p->in.op == EQ) || (p->in.op == NE) || (p->in.op == ASSIGN))
		  WERROR( MESSAGE( 171 ), opst[p->in.op], "structure/union pointer" );
	     else if (p->in.op != ARGASG)
		  UERROR( MESSAGE( 171 ), opst[p->in.op], "structure/union pointer" );
	     else
		  UERROR( MESSAGE( 227 ), arg_fname,"struct/union pointer",arg_name);
	}
	else if( (status & ICARY) || (status & ICARYSIZE) ) {
	     /* "%s types incompatible, op %s" */
	     if( (p->in.op == EQ) || (p->in.op == NE) || (p->in.op == ASSIGN))
		  WERROR( MESSAGE( 171 ), opst[p->in.op], "array" ); 
	     else if (p->in.op != ARGASG)
		  UERROR( MESSAGE( 171 ), opst[p->in.op], "array" ); 
	     else
		  UERROR( MESSAGE( 227 ),arg_fname,"array",arg_name);
	     return;
	}
	else if( (status & ICPTR) && !(status & ICTYPE)) {
	     /* "types pointed to by %s operands must be 
		 qualified or unqualified versions of compatible types" */
	     if( (p->in.op == EQ) || (p->in.op == NE) || (p->in.op == ASSIGN)
		                  || (p->in.op == MINUS))
		  WERROR( MESSAGE( 146 ), opst[p->in.op]  ); 
	     else if (p->in.op != ARGASG)
		  UERROR( MESSAGE( 146 ), opst[p->in.op]  ); 
	     else
		  UERROR( MESSAGE( 228 ), arg_fname, arg_name );
	}

	if( status & ICTYPE ) {

	     if( ISPTR(t1) ) q = p->in.right;
	     else q = p->in.left;

	     if( !ISPTR(q->in.type) ) { 
	          /* q has an integer-like type (CHAR, SHORT, INT, etc.)
		   *
		   * if q is null pointer constant
		   *      error unless p->in.op is among those below
		   * else 
		   *      if q and p have the same size && op is EQ, NE ,
		   *                                 ASSIGN, RETURN or ASGARG
		   *           warn
		   *      else error
		   */
	          if( ISNULL( q ) ) {
		     if( (  ( p->in.op != EQ ) 
				   &&( p->in.op != NE )
				   &&( p->in.op != COLON )
				   &&( p->in.op != ASSIGN )
				   &&( p->in.op != ARGASG )
				   &&( p->in.op != RETURN )))
		          /* "pointer and %s are type incompatible, op %s" */
			  UERROR( MESSAGE( 163 ), opst[p->in.op], "null" ); 
		   }
		  else {
		       char *typename = ((q->in.op == FLD)?(ISUNSIGNED(q->in.type)?"unsigned bitfield":"bitfield"):((q->in.tattrib&ATTR_ENUM)?"enumeration":basename[q->in.type]));
		       if(   (   (p->in.op == EQ) || (p->in.op == NE) 
			      || (p->in.op == ASSIGN) || (p->in.op == RETURN)
			      || (p->in.op == ARGASG))
			  && ( dimtab[ q->in.type ] == SZPOINT )) {

			    if( p->in.op != ARGASG ) {
				 /* "pointer and %s are type incompatible, op %s" */
				 WERROR( MESSAGE( 163 ), opst[p->in.op], typename );
			    }
			    else {
				/* types in call and definition of '%s' are
				 * incompatible (pointer and %s) for parameter
				 * %s
				 */
				     WERROR( MESSAGE( 226 ),arg_fname,typename,arg_name);
			    }
		       }
		       else {
			   if (p->in.op == ARGASG)
			       UERROR( MESSAGE( 226 ), arg_fname,typename,arg_name);
			   else
			       UERROR( MESSAGE( 163 ), opst[p->in.op], typename );
			    }
		  }
	     }
	     else {
	          /* both are pointers
		   * if one is void pointer and the other is NOT pointer to ftn
		   *                 or one is void pointer to NULL
		   *      error unless p->in.op is among those below
		   * else ... see below
		   */
	          if(   ( (t1 == VOIDPTR ) && !ISFTN( DECREF( t2 )) )
		     || ( (t2 == VOIDPTR ) && !ISFTN( DECREF( t1 )) )
		     || ( ISNULL( p->in.left ) )
		     || ( ISNULL( p->in.right ) ) )
		    {
		       if( (  ( p->in.op != EQ ) 
				       &&( p->in.op != NE )
				       &&( p->in.op != COLON )
				       &&( p->in.op != ASSIGN )
				       &&( p->in.op != ARGASG )
				       &&( p->in.op != RETURN )))
			    /* "pointer and %s are type incompatible, op %s" */
			    UERROR( MESSAGE( 163 ), opst[p->in.op], "void pointer" );
		     }
		  else {
		          /* if op is EQ, NE, ASSIGN, ARGASG, RETURN or MINUS 
			   *      warn
			   * else error
			   *
			   */
			  switch( p->in.op ) {

			  case EQ:
			  case NE:
			       if( ( t1 == VOIDPTR ) || ( t2 == VOIDPTR ) ) {
			            /* "void pointer should be NULL pointer constant
				        when used with %s and pointer to function" */
			            WERROR( MESSAGE( 142 ), opst[p->in.op] );
				  }
			       else {
				    /* "%s types incompatible, op %s" */
				    WERROR( MESSAGE( 171 ), opst[p->in.op], "pointer" );
				  }
			       break;
			  case ASSIGN:
			       if( t1 == VOIDPTR ) {
				    /* "void pointer should not be on lhs of %s
				       when rhs is pointer to function/function designator" */
				    WERROR( MESSAGE( 143 ), opst[p->in.op] );
			       }
			       else 
				    if( t2 == VOIDPTR ) {
					 /* "void pointer should be NULL pointer constant
					    when used on the rhs with %s and pointer to function" */
					 WERROR( MESSAGE( 144 ), opst[p->in.op] );
				    }
				    else {
					 /* "%s types incompatible, op %s" */
					 WERROR( MESSAGE( 171 ), opst[p->in.op], "pointer" );
				    }
			       break;
			  case MINUS:
			  case RETURN:
			       /* "%s types incompatible, op %s" */
			       WERROR( MESSAGE( 171 ), opst[p->in.op], "pointer" );
			       break;
			  case ARGASG:
			       /* "function parameter %s types in call and 
				   definition are incompatible" */
			       WERROR( MESSAGE( 227 ), arg_fname, "pointer", arg_name );
			       break;
			  default:
	                       /* "%s types incompatible, op %s" */
			       UERROR( MESSAGE( 171 ), opst[p->in.op], "pointer" );
			     }
			}
		}
	   }      /* end case ICTYPE */
      }
#endif /*ANSI*/

#ifndef IRIF

NODE *
stref( p, isdot ) register NODE *p; int isdot; {

	register OFFSZ off;
	register struct symtab *q;
#ifdef C1_C
	struct symtab *symidx;
	struct symtab *tag;
	int fixtag;
#endif  /* C1_C */
	TWORD t,ta;
	int d, s, align;
	char dsc;
	char bitsize;
#ifdef ANSI
	flag not_lvalue = 0;
#endif /*ANSI*/
	/* make p->x */
	/* this is also used to reference automatic variables */

#ifdef C1_C
        symidx = p->in.right->nn.sym;
        q = symidx;
        tag = p->in.c1tag;
	fixtag = p->in.fixtag;
#else
	q = p->in.right->nn.sym;
#endif  /* C1_C */
	p->in.right->in.op = FREE;
	p->in.op = FREE;
	p = pconvert( p->in.left );

	/* make p look like ptr to x */

	if( !ISPTR(p->in.type)){
		p->in.type = PTR+UNIONTY;
		}


	s = 0;	/* a simple flag for bitfields at this point */

	/* compute the offset to be added */

	off = q->offset;
	dsc = q->sclass;

	if( dsc & FIELD ) {  /* normalize offset */
		switch(q->stype) {
#ifdef ANSI
		case SCHAR:
#endif
		case CHAR:
		case UCHAR:
			align = ALCHAR;
			s = CHAR;
			break;

		case SHORT:
		case USHORT:
			align = ALSHORT;
			s = SHORT;
			break;

#ifndef ANSI
		case ENUMTY:	
#endif /*ANSI*/
		case INT:
		case UNSIGNED:
			align = ALINT;
			s = INT;
			break;

#ifdef ANSI
		case LONG:
		case ULONG:
			align = ALLONG;
			s = LONG;
			break;
# endif

		default:
			cerror( "undefined bit field type" );
			}

		bitsize = dsc & FLDSIZ;

		/* Check to see if we can get away with .w masking. Immediate
		   operands are shorter and faster if they're legal.
		*/
		if ( (s==INT) && (off/ALSHORT == (off+bitsize-1)/ALSHORT) )
			{
			q->stype = ISUNSIGNED(q->stype)? USHORT : SHORT;
			align = ALSHORT;
			s = SHORT;
			}

		off = (off/align)*align;
		}

	ta = INCQUAL(  (ISCON( p->in.tattrib ))       /* inherit CONST attrib */
		     ? (( BTYPE(q->stype)==q->stype) 
			? q->sattrib| ATTR_CON
			: q->sattrib| PATTR_CON)      /* from structure */
		     : q->sattrib );

#ifdef ANSI
	if(( p->in.op == STCALL ) || ( p->in.op == UNARY STCALL )) {
	     not_lvalue = 1;
	     p->in.not_lvalue = 1;
	}
	/* this is bogus, no matter what the standard sez ... */
	if( p->in.not_lvalue && (p->in.op != ICON) && ISARY( q->stype ) && isdot) {
	     /* "should not subscript an array which has no lvalue" */
	     WERROR( MESSAGE( 173 ));
	}
#endif
	t = INCREF( q->stype );
	d = q->dimoff;
	if (s==0)
	  {  /* make sure enums get converted here as in buildtree */
	  if( ISENUM(BTYPE(t),ta) )
#ifdef SIZED_ENUMS
	    s = BTYPE(t);
#else
	    s = INT;
#endif
	  else
	    s = q->sizoff;
	  }

#ifdef ANSI
	if( p->in.not_lvalue ) not_lvalue = p->in.not_lvalue;
#endif /*ANSI*/
	p = makety( p, t, d, s, ta );
#ifdef SA	
	if( off != 0 || saflag) {
#else
	if( off != 0 ) {
#endif	   
	   p = clocal( block( PLUS, p, makety(offcon( off),INT,0,INT,0), t, d, s) );
#ifdef ANSI
	   if( !isdot ) not_lvalue = 0;
	   if( not_lvalue ) p->in.not_lvalue = not_lvalue;
#endif /*ANSI*/
#ifdef C1_C
           p->in.c1tag = tag;
	   p->in.fixtag = fixtag;
#endif /* C1_C */
	   p->in.tattrib = ta;
	 }
#ifdef SADEBUG	
	if (saflag && p->in.right->tn.op != ICON)
	   cerror ("static has problem in stref()\n");
#endif	
#ifdef SA
	if (saflag)
	   p->in.right->nn.sym = q;
#endif	
	p = buildtree( UNARY MUL, p, NIL );
#ifdef C1_C
        p->in.c1tag = tag;
	p->in.fixtag = fixtag;
#endif /* C1_C */

	/* if field, build field info */

	if( dsc & FIELD ){
#ifdef C1_C
                /* Fix the tag on the object from which the field is extracted.
                 * (c1 doesn't understand ARRAYREF on FLD nodes).  For example,
                 * a[i].bitfield should tag the U*, not the FLD   (FSDdt06079)
		 * If the bitfield is accessed via a pointer (e.g p->field),
		 * c1tag will be 0 (c1 doesn't consider pointer dereferences
		 * to be structure references), so don't set the fixtag.
                 */
                p->in.c1tag = tag;
                if (tag) p->in.fixtag = 1;
#endif /* C1_C */
		p = block( FLD, p, NIL, q->stype, 0, q->sizoff );
		p->in.tattrib = ta;
		p->tn.rval = PKFIELD( bitsize, q->offset%align );
		}

	return( clocal(p) );
	}

#else /* IRIF */

NODE *
stref( p ) register NODE *p; {

     register struct symtab *q;
	
     q = p->in.right->nn.sym;

#ifdef ANSI
     if(  (   p->in.left->in.op == CALL || p->in.left->in.op == UNARY CALL )
	&&(   p->in.op == DOT  )) {

	  p->in.left->in.not_lvalue = p->in.not_lvalue = 2;
     }
     if( p->in.left->in.not_lvalue && ISARY( q->stype )) {
	  /* "should not subscript an array which has no lvalue" */
	  WERROR( MESSAGE( 173 ));
	  p->in.not_lvalue = p->in.left->in.not_lvalue;
     }
#endif /* ANSI */

     p->in.type = p->in.right->in.type;
     p->in.tattrib = p->in.right->in.tattrib & (~ATTR_CLASS);
     p->fn.cdim = p->in.right->fn.cdim;
     p->fn.csiz = p->in.right->fn.csiz;

#ifndef HAIL /* in HAIL don't convert DOT to STREF */
     if( p->in.op == DOT ) {
	  /* 
	   * change 'op' to 'STREF' and adjust tree
	   */
	  if( p->in.left->in.op == UNARY MUL ){
	       p->in.left->in.op = FREE;
	       p->in.left = p->in.left->in.left;
	  }
	  else if( p->in.left->in.op != UNARY CALL &&
		  p->in.left->in.op != CALL ) {
	       p->in.left = buildtree( UNARY AND, p->in.left, NIL ); 
	  }	
	  else {
	       /* 'p->in.left->in.op' is a structure call */
	       if( p->in.left->in.left->in.op == PCONV ) {
		    adjust_su_ftnname_cast( p->in.left );
	       }
	  }
	  p->in.op = STREF;
     }
#endif /* not HAIL */

     /* if field, build field info */
     
     if( q->sclass & FIELD ){
	  int ta = INCQUAL(  (ISCON( p->in.tattrib ))       /* inherit CONST attrib */
			   ? (( BTYPE(q->stype)==q->stype) 
			      ? q->sattrib| ATTR_CON
			      : q->sattrib| PATTR_CON)      /* from structure */
			   : q->sattrib );

	  p = block( FLD, p, NIL, q->stype, 0, q->sizoff );
	  p->tn.rval = q->sclass & FIELD;
	  p->in.tattrib = ta;
     }
     
     return( clocal(p) );
}

adjust_su_ftnname_cast( p ) register NODE *p; {
     /*
      * A function name [return type struct/union] has been cast:
      *
      *      the top node is a (UNARY) CALL, its descendant a PCONV
      *           INCREF the type of the call, and
      *           adjust type of PCONV to reflect this
      */
     int type;
     
     p->in.type = INCREF( p->in.type );
     type = INCREF( INCRF( p->in.type, FTN ));
     p->in.left = makety( p->in.left, type,
				  p->in.left->fn.cdim,
				  p->in.left->fn.csiz,
				  p->in.left->in.tattrib );
}
#endif /* IRIF */

#endif /* BRIDGE */

#ifndef IRIF

notlval(p)
register NODE *p;
  {
  int ansi_cast = 0;
  register NODE *pl;

	/* For ANSI mode: return 0 if p is an lvalue,
				 1 if p is not an lvalue
				 2 if p is not an lvalue, but would be
				   accepted in compatability mode.
	   Yuck!, but deals with many cases PCC used to let through.
	/* For compatability mode: return 0 if p is an lvalue
				          1 if p is not an lvalue */
	again:

#ifdef ANSI
        if (p->in.not_lvalue) ansi_cast = 2;
#endif

	switch( p->in.op )
	{
	case PAREN:
		p = p->in.left;
		goto again;

	case FLD:
		p = p->in.left;
		goto again;

	case UNARY MUL:
		pl = p->in.left;
		/* fix the &(a=b) bug, given that a and b are structures */
		if( pl->in.op == STASG ) return( 1 );
                /* the remaining checks in this case involve structures   */
                /* references. we must detect the case of the first field */
		/* as well as following fields.  Following fields always  */
		/* have the form (plus ... icon), where the first field   */
		/* is just ...                                            */
                if ((pl->in.op == PLUS) && (pl->in.right->in.op == ICON))
                  pl = pl->in.left;
                /* the f().a bug, where f returns a struct */
                if(pl->in.op == UNARY STCALL || pl->in.op == STCALL)
                  return( 1 );
#ifdef ANSI
		/* The following clause is somewhat of a kludge to
		 * distinguish between *(i?p:q) = 1; <where the type
		 * won't be STRTY+INT> and then between the "." and "->"
		 * operators from flags set in the STREF section of buildtree
		 */
		if((pl->in.op == QUEST) &&
                   ((pl->in.right->in.type == PTR+STRTY) ||
                    (pl->in.right->in.type == PTR+UNIONTY)))
                    return( pl->in.not_lvalue?1:ansi_cast );
#endif /*ANSI*/
	case NAME:
	case OREG:
		if( ISARY(p->in.type) || ISFTN(p->in.type) ) return(1);
	case REG:
		return(ansi_cast);

	default:
		return(1);

	}

}

#else /* IRIF */

notlval(p) register NODE *p; {
     register NODE *pl;

	/* return 0 if p an lvalue, 1 otherwise */

	again:

	switch( p->in.op ){

	case PAREN:
	case FLD:
	case DOT:
		p = p->in.left;
		goto again;

	case UNARY MUL:
	case INDEX:
                if( p->in.op == UNARY MUL ) {
		     pl = p->in.left;
		}
		else {
		     pl = p;
		}
		
                /* the remaining checks in this case involve structures   */
                /* references.  we must detect the case of a first field  */
                /* reference as well as following field references.       */
		/* Following field references always have the form        */
                /* plus ... icon, where first field is just ...           */
                if ((pl->in.op == INDEX
		     ) && (pl->in.right->in.op == ICON))
		     pl = pl->in.left;

                /* the f().a bug, where f returns a struct */
/***X what to do in other cases ??? X***/
#if (defined IRIF) && (defined ANSI)
		if( pl->in.not_lvalue == 2 ) return( 1 );
#endif /* not IRIF or not ANSI */

#ifdef ANSI
		/* The following clause is somewhat of a kludge to
		 * distinguish between *(i?p:q) = 1; <where the type
		 * won't be STRTY+INT> and then between the "." and "->"
		 * operators from flags set in the STREF section of buildtree
		 */
		if((p->in.left->in.op == QUEST) &&
                   ((p->in.left->in.right->in.type == PTR+STRTY) ||
                    (p->in.left->in.right->in.type == PTR+UNIONTY)))
                    return( p->in.left->in.not_lvalue );
#endif /*ANSI*/
	
	case NAME:
		if( ISARY(p->in.type) || ISFTN(p->in.type) ) return(1);
	case STREF:
		return(0);

	default:
		return(1);

	   }
}
   
#endif /* IRIF */

#ifndef BRIDGE
# ifndef IRIF

#	define bpsize(p) (offcon(psize(p)))
# endif /* not IRIF */

OFFSZ
psize( p ) register NODE *p; {
	/* p MUST BE a node of type pointer; psize returns the
	   size of the thing pointed to */
        int size;

	if( !ISPTR(p->in.type) ){
		/* formerly uerror( "pointer required" ) */
		cerror("psize: ptr type required for parameter; type = %d",
		        p->in.type );
		return( SZINT );
		}

	if( p->in.type == VOIDPTR){
	        /* "pointer to VOID inappropriate" */
	        if( !report_no_errors) UERROR( MESSAGE( 130 ));
		return( SZINT );
	        }
	/* note: no pointers to fields */
#ifndef IRIF
	if( size = tsize( (TWORD)DECREF(p->in.type), p->fn.cdim, p->fn.csiz, (char *)NULL, 0 ) ) /* SWFfc00726 fix */
		return( size );
	else {
	     /* "pointer to object of unknown size used in context where size
		 must be known" */
	       UERROR( MESSAGE( 71 ));
	       return( SZINT );
	}
#else /* IRIF */
	size = ir_t_deref_bytesz( p );
	if( size <= 0 ) {
	     /* "unknown size" */
	     UERROR( MESSAGE( 114 ) );
	}
	return( size );
#endif /* IRIF */
   }

NODE *
convert( p, f )  register NODE *p; {

	register NODE *q, *r;

#ifdef IRIF
	/* no conversions done for IRIF; adjustments for type sizes taken
	 * care of in the interface.  usually, the value can pass thru
	 * unchanged ... 
	 *
	 * the case that needs attention is arrays.  the interface will expect
	 * the value to represent the number of elements offset into the
	 * array, and so care must be taken here regarding the
	 * multidimensional case
	 */

	register int t,d,dim;

	if( f==CVTL ) {
	     r = p->in.right;
	}
	else {
	     r = p->in.left;
	}
	if( ISPTR( r->in.type ) && ISARY( t = DECREF( r->in.type ))) {
	     for( dim = 1, d = r->fn.cdim ; ISARY( t ) ; t = DECREF( t ), d++ ) {
		  dim *= dimtab[ d ];
	     }
	     if( f==CVTR ) {
	          p->in.right = buildtree( MUL, p->in.right, bcon( dim, INT ));
	     }
	     else {
	          p->in.left = buildtree( MUL, p->in.left, bcon( dim, INT ));
	     }
	}
	return( p );
   
#else /* not IRIF */

	/*  convert an operand of p
	    f is either CVTL or CVTR
	    operand has type int, and is converted by the size of the other side
	    */

	q = (f==CVTL)?p->in.left:p->in.right;

	r = block( PMCONV,
		q, bpsize(f==CVTL?p->in.right:p->in.left), INT, 0, INT );
	r = clocal(r);
	if( f == CVTL )
		p->in.left = r;
	else
		p->in.right = r;
	return(p);

#endif /* not IRIF */
	}

NODE *
pconvert( p ) register NODE *p; {

	/* if p should be changed into a pointer, do so */

	if( ISARY( p->in.type) ){
#ifdef IRIF
	     if(  
		p->in.op != UNARY MUL || ( 
			 p->in.left->in.op != INDEX && !( 
				 ISPTR( p->in.left->in.type ) && ( 
					     p->in.left->in.op == PLUS || p->in.left->in.op == MINUS 
								  )
							 )
					  )
		) {
#endif /* IRIF */
		  p->in.type = DECREF( p->in.type );
		  p->in.tattrib = DECREF( p->in.tattrib ) & (~ATTR_CLASS);
		  ++p->fn.cdim;
#ifdef IRIF
	     }
#endif /* IRIF */
	     p = buildtree( UNARY AND, p, NIL );
	     p->in.tattrib |= ATTR_WAS_ARRAY;
	     return( p );
	}
	if( ISFTN( p->in.type) )
		return( buildtree( UNARY AND, p, NIL ) );

	return( p );
	}

NODE *
oconvert(p) register NODE *p; {
	/* convert the result itself: used for pointer and unsigned */

	switch(p->in.op) {

	case LE:
	case LT:
	case GE:
	case GT:
		if( ISUNSIGNED(p->in.left->in.type) || ISUNSIGNED(p->in.right->in.type) ) 
			p->in.op += (ULE-LE);
	case EQ:
	case NE:
		return( p );

#ifndef IRIF
	case MINUS: 
		{ NODE *q;
		report_no_errors++;  /* caught by ptmatch() */
		q = clocal( block( PVCONV,
			p, bpsize(p->in.left), INT, 0, INT ) );
		report_no_errors = 0;
		return( q );
		}
#endif /* not IRIF */
	   }

	cerror( "illegal oconvert: %x", p->in.op );

	return(p);
	}

NODE *
ptmatch(p)  register NODE *p; {

	/* makes the operands of p agree; they are
	   either pointers or integers, by this time */
	/* with MINUS, the sizes must be the same */
	/* with COLON, see below ... */

	short o = p->in.op;
	register TWORD t1, t2, t;
	int d2, d, s2, s;

	t = t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
	d = p->in.left->fn.cdim;
	d2 = p->in.right->fn.cdim;
	s = p->in.left->fn.csiz;
	s2 = p->in.right->fn.csiz;

	switch( o ){

	case ASSIGN:
	case RETURN:
	case CAST:
		{
		    p->in.right->in.tattrib = p->in.left->in.tattrib;
#ifdef ANSI
		   /* set the array attribute of the right based on the
		    * array attribute of the left.  Added to make certain
		    * (char *) "" doesn't still have array attribute set.
		    */
		   if (p->in.left->in.tattrib&ATTR_WAS_ARRAY)
		       p->in.right->in.tattrib |= ATTR_WAS_ARRAY;
		   else
		       p->in.right->in.tattrib &= ~ATTR_WAS_ARRAY;
#endif /*ANSI*/
		   break;
		}

	case MINUS:
		if( ( psize(p->in.left) != psize(p->in.right) )) {
		     /* "incorrect pointer subtraction" */
		     UERROR( MESSAGE( 67 ) );
			}
		break;
	case COLON:
#ifndef ANSI
		{  if( (t1 == VOID+PTR) || (t2 == VOID+PTR)) break;
		   if( (t1 == ENUMTY) || (t2 == ENUMTY)) break;
		   if( t1 != t2 ) 
			/* "operands of '%s' have incompatible or 
			    incorrect types" */
			UERROR( MESSAGE( 89 ), ":" );
		   break;
		   }
#else /*ANSI*/
		/* determine result type and qualifiers
		 *
		 * result type is
		 *
		 *    o  composite, if both are pointers to (possibly 
		 *       different) qualified versions of compatible types;
		 *    o  type of the other, if one is NULL pointer constant;
		 *    o  ptr to void, if one is a ptr to void - the other
		 *       operand is converted to a ptr to void
		 *
		 * result qualifiers are the union of the operand qualifiers
		 *
		 * assumptions:
		 *
		 *    o  the operands have been through CHKPUN (and LL/RINT if
		 *       needed) and so either
		 *            both are pointers (to qualified versions of
		 *                 compatible types or one is a ptr to void) or
		 *            one is a pointer, the other an integer;
		 *
		 *    if one is an integer, it is treated like NULL for
		 *       type determination
		 */

		/* qualifiers */
		p->in.tattrib = (  (  p->in.left->in.tattrib 
				    | p->in.right->in.tattrib )
				 & ( ATTR_CON | ATTR_VOL | ATTR_HAS_CON ) );
		
		p->in.tattrib |= ATTR_WAS_ARRAY & p->in.left->in.tattrib 
		                                & p->in.right->in.tattrib;

		/* types */
		if( ISSIMPLE( t1 ) || ISNULL( p->in.left ) ) {
		     p->in.type = t2;
		     p->fn.cdim = d2;
		     p->fn.csiz = s2;
		   }
		else if( ISSIMPLE( t2 ) || ISNULL( p->in.right ) ) {
		     p->in.type = t1;
		     p->fn.cdim = d;
		     p->fn.csiz = s;
		   }
# ifndef HAIL
	/*
	 * In HAIL the void conversions need be made explicit.
	*/
		else if( ( t1 == VOIDPTR ) || ( t2 == VOIDPTR ) ) {
		     p->in.type = VOIDPTR;
		     p->in.left->in.type = VOIDPTR;
		     p->in.right->in.type = VOIDPTR;
		     p->fn.csiz = VOID;
		     p->in.left->fn.csiz = VOID;
		     p->in.right->fn.csiz = VOID;
		   }
# endif
	        else {
		     /* construct composite type - both are compatible ptrs */
		     /* BEFORE PROTOTYPES */
		     p->in.type = t1;
		     if( WAS_ARRAY( p->in.tattrib ))
		         p->fn.cdim = ( (dimtab[d-1] >= dimtab[d2-1]) 
				                      ? d : d2 ); 
		     else
		         p->fn.cdim = d;
		     p->fn.csiz = s;
		   };
# ifdef IRIF
	/*
	 * In the case of IRIF we want to go through the
	 * makety conversions.
	*/

		t = p->in.type;
		d = p->fn.cdim;
		s = p->fn.csiz;
		break;
# else
	        return( clocal( p ));
# endif /*IRIF*/
#endif /*ANSI*/

	default:  /* must work harder: relationals or comparisons */

		if( !ISPTR(t1) ){
			t = t2;
			d = d2;
			s = s2;
			break;
			}
		if( !ISPTR(t2) ){
			break;
			}
#ifndef IRIF
		/* both are pointers */
		if( talign(t2,s2, FALSE) < talign(t,s, FALSE) ){
			t = t2;
			s = s2;
			}
#endif /* not IRIF */

		break;
		}

	p->in.left = makety( p->in.left, t, d, s, p->in.left->in.tattrib );

#ifdef IRIF
	/*
	 * For the IRIF case with CAST, the RHS cannot be rewritten with the 
	 * composite type.  For example:
	 *
	 *     struct bar a[];
	 *     struct foo *pfoo;
	 *     pfoo = (struct foo *) &a[i]
	 *
	 * IRIF needs to know the original size of 'a[i]'; rewriting the
	 * RHS here would lead IRIF to believe the size of 'a[i]' to be
	 * 'sizeof( struct foo )' 
	 *
	 * Therefore, cover the RHS with a PCONV node
	 */
	if( o == CAST ) {
	     p->in.right = block( PCONV, p->in.right, NIL, t, d, s);
	     p->in.right->in.tattrib =  p->in.right->in.left->in.tattrib;
	     p->in.type = t;
	     p->fn.cdim = d;
	     p->fn.csiz = s;
	     return( clocal(p)); 
	}
#endif /* IRIF */

	p->in.right = makety( p->in.right, t, d, s, p->in.right->in.tattrib );
	if( o!=MINUS ){

		p->in.type = t;
		p->fn.cdim = d;
		p->fn.csiz = s;
		}

	return(clocal(p));
	}

#ifdef IRIF
/*************************************************************************
 * wider_real_type () : determines the wider real type of two types
 *
 * 't1', 't2'         : types
 *
 * returns            : 0 if neither type is real
 *                    : 't1' or 't2' whichever is the wider real
 */
int wider_real_type( t1, t2 ) {
     int i;
     int t;
     int count = 0;
     int wider;

     for( ( i=0, t=t1 ) ; i<2 ; ( i++, t=t2 )) {
	  switch( t ) {
	     case FLOAT:
	       count++;
	       break;
	     case DOUBLE:
	       count = count + 3;
	       break;
	     case LONGDOUBLE:
	       count = count + 7;
	       break;
	  }
     }
     switch( count ) {
	case 0:
	  wider = 0;
	  break;
	case 1:
	case 2:
	  wider = FLOAT;
	  break;
	case 3:
	case 4:
	case 6:
	  wider = DOUBLE;
	  break;
	default:
	  wider = LONGDOUBLE;
	  break;
     }
     return( wider );
}
#endif /* IRIF */

#ifndef ANSI
NODE *
tymatch(p)  register NODE *p; {

	/* satisfy the types of various arithmetic binary ops */

	/* rules are:
		if assignment op, type of LHS
		if any float or doubles, make double (unless singleflag)
		if any longs, make long
		otherwise, make int
		if either operand is unsigned, the result is...
	*/

	register TWORD t1, t2, t;
	register short o;
	short u,u1,u2;
	TWORD tu;
#ifdef IRIF
	int rhs_done = 0;
#endif /* IRIF */

	o = p->in.op;

	t1 = p->in.left->in.type;
	t2 = p->in.right->in.type;
	if( (t1==UNDEF || t2==UNDEF) && o!=CAST )
		/* "void type illegal in expression" */
		UERROR( MESSAGE( 118 ) );

	u = 0;
	if( ISUNSIGNED(t1) ){
		u = u1 = 1;
		t1 = DEUNSIGN(t1);
		}
	else
		u1 = 0;
	if( ISUNSIGNED(t2) ){
		u = u2 = 1;
		t2 = DEUNSIGN(t2);
		}
	else
		u2 = 0;

	if ( logop(o) || bilogop(o) ) 
		{
		/* by default we always promote to INT or DOUBLE before doing
		 * the op.  If an operand is an integer constant, we do a
		 * short-circuit evaluation (avoid promoting), if we can.
		 */
		char constflag;		/* at least one op is ICON */
		long lval;		/* left-side constant value */
		long rval;		/* rt-side constant value */
		constflag = 0;

		/* set proper type of constant -- rest of compiler is sloppy */
		if (p->in.left->in.op == ICON)
			{
			lval = p->in.left->tn.lval;
			if ((lval >= -128) && (lval <= 127)) t1 = CHAR;
			else if ((lval >= -32768) && (lval <= 32767))
								t1 = SHORT;
			else t1 = INT;
			constflag = 1;
			}
		if (p->in.right->in.op == ICON)
			{
			rval = p->in.right->tn.lval;
			if ((rval >= -128) && (rval <= 127)) t2 = CHAR;
			else if ((rval >= -32768) && (rval <= 32767))
								t2 = SHORT;
			else t2 = INT;
			constflag = 2;
			}

		t = MAX(t1, t2);
		switch (t)
			{
			case FLOAT:
				if (!singleflag) t = DOUBLE;
				/* fall thru */
			case DOUBLE:
				break;
			default:
				/* don't promote to INT if either:
				 * (a) both are signed
				 * (b) both are unsigned
				 * (c) one is unsigned, other is constant
				 *	with value >= 0
				 * else both must be promoted to INT first
				 */
				if (!u || (u1 & u2)
				 || (u1 && (constflag == 2) && (rval >= 0))
				 || (u2 && (constflag == 1) && (lval >= 0)))
					{
					/* don't promote to INT */
					p->in.left->in.type =
					        (u1)?ENUNSIGN(t1):t1;
					p->in.right->in.type =
					        (u2)?ENUNSIGN(t2):t2;
					}
				else 	  /* promote to INT */
					t = INT;
				break;
			}
		}
	else if (t1 == DOUBLE || t2 == DOUBLE) t = DOUBLE;
	else if (t1 == FLOAT || t2 == FLOAT) t = singleflag? FLOAT : DOUBLE;
	else t = INT;

	if (asgop(o)) switch (o)
		{
		case ASG PLUS:
		case ASG MINUS:
		case ASG MUL:
		case ASG DIV:
#ifndef IRIF
			if ( t == FLOAT || t1 == FLOAT )
				return( longassign(p) );
#endif /* not IRIF */
			/* fall thru */

		case ASSIGN:
		case ASG AND:
		case ASG MOD:
		case ASG LS:
		case ASG RS:
		case ASG OR:
		case ASG ER:
		case INCR:
		case DECR:
		case RETURN:
#ifndef IRIF
		case STASG:
#endif /* not IRIF */
		case CAST:
			tu = p->in.left->in.type;
			t = t1;
			break;

# ifdef DEBUGGING
		default:
			cerror("strange asgop in tymatch()");
# endif	/* DEBUGGING */
		}
	else {
		tu = (u && UNSIGNABLE(t))?ENUNSIGN(t):t;
		}

	/* because expressions have values that are at least as wide
	   as INT or UNSIGNED, the only conversions needed
	   are those involving FLOAT/DOUBLE, and those
	   from LONG to INT and ULONG to UNSIGNED */

	if( t != t1 ) p->in.left = makety( p->in.left, tu, 0, (int)tu, 0 );

	if( t != t2 || o==CAST ) p->in.right = makety( p->in.right, tu, 0, (int)tu, 0 );

	if( asgop(o) ) {
	     p->in.type = p->in.left->in.type;
	     p->fn.cdim = p->in.left->fn.cdim;
	     p->fn.csiz = p->in.left->fn.csiz;
#if (defined IRIF) && !(defined HAIL)
	     if( ( dope[o] & ASGOPFLG ) && ( p->in.right->in.op == SCONV )) {
		  /* 
		   * see ANSI 'tymatch()' for comments
		   */
		  if( wider_real_type( t1, t2 ) == t2 ) { 
		       p->in.right->in.op = ASGOPCONV;
		       p->in.right->in.type = t2;
		       p->in.right->fn.cdim = p->in.right->in.left->fn.cdim;
		       p->in.right->fn.csiz = (int)t2;
		       rhs_done++;
		  }
	     }
#endif /* IRIF & !HAIL */
	}
	else {
	     if ( logop(o) || bilogop(o) )
		  p->in.type = (t==DOUBLE || t==FLOAT)? INT : tu;
	     else p->in.type = tu;

	     p->fn.cdim = 0;
	     p->fn.csiz = t;
	}

#ifdef IRIF
	if( ( !logop( o ) && !bilogop( o )) || ( t!=DOUBLE && t!=FLOAT )){
	     if( p->in.type != p->in.left->in.type ) 
		  p->in.left = makety( p->in.left, p->in.type, 0, (int)p->in.type, 0 );
	     if( !rhs_done && ( p->in.type != p->in.right->in.type ))
		  p->in.right = makety( p->in.right, p->in.type, 0, (int)p->in.type, 0 );
	}
#endif /* IRIF */

# ifdef DEBUGGING
	if( tdebug ) (void)prntf( "tymatch(%x): %x %s %x => %x\n",p,t1,opst[o],t2,tu );
# endif /* DEBUGGING */

	return(p);
	}

#else /*ANSI*/
NODE *
tymatch(p) register NODE *p; {
    /* implement "usual arithmetic conversions" */
    /* Rules in 3.2.1.5 are:
     * If either operand has type long double, the other operand is converted
     * to long double
     * If either operand has type double, the other operand is converted to
     * double
     * If either operand has type float, the other operand is converted to
     *  float
     * Otherwise, the integral promotions are performed on both operands:
     *    If either operand has type unsigned int, the other operand is
     *    converted to unsigned int
     *    Otherwise, both operands have type int
     */
    register TWORD t1,t2,t;
    register short o;

    t1 = p->in.left->in.type;
    t2 = p->in.right->in.type;
    o = p->in.op;

    if (asgop(o)) {
	t = t1;
	if (t != t2) {
	     p->in.right = makety( p->in.right,t,p->in.left->fn.cdim,(int)t,0);
#if (defined IRIF) && !(defined HAIL)
	     if( ( dope[o] & ASGOPFLG ) && p->in.right->in.op == SCONV ) {
		  /* 
		   * trees for assignment ops now look like
		   *
		   *                   ASGOP [+=, *=, etc]
		   *                   /      \
		   *                  /        \
		   *                LHS       SCONV to LHS
		   *                             \
		   *                              \
		   *                              RHS
		   *
		   * The type of the ASGOP node is determined like assignment.
		   * The issue here is the type of the intermediate 
		   * expression, the 'a+b' portion of 'a+=b'.  The SCONV
		   * node may have to be changed.
		   *
		   * If both the LHS and RHS have integral type, the type
		   * of the intermediate expression will be taken as the
		   * LHS type and no change is necessary.
		   *
		   * If either node has a real type, the type of the
		   * intermediate expression will be the wider real type.
		   * If this type matches the LHS no change is necessary.
		   * If it matches the RHS, an ASGOPCONV-to-RHS-type 
		   * will overwrite the SCONV node
		   *
		   * The ASGOPCONV node will cause a conversion when it is
		   * encountered.  [Tree nodes are usually processed in
		   * POST order - certainly SCONV is.  ASGOPCONV is not
		   * processed in post order, so that it has the effect
		   * of converting the LHS type as it participates in the
		   * intermediate expression.]
		   */
		  if( wider_real_type( t1, t2 ) == t2 ) { 
		       p->in.right->in.op = ASGOPCONV;
		       p->in.right->in.type = t2;
		       p->in.right->fn.cdim = p->in.right->in.left->fn.cdim;
		       p->in.right->fn.csiz = (int)t2;
		  }
	     }
#endif /* IRIF & !HAIL */
	}
	else if (o == CAST && t != t2) p->in.right = clocal(block(SCONV,p->in.right,NIL,t,p->fn.cdim,p->fn.csiz));
    } 
    else {
	 t = integral_promote( p );
	 /* preserve the signed char/plain char distinction for lint
	  * (to avoid the nonportable character comparison message)
	  */
#ifdef LINT
	 if (t!=CHAR && t1!=SCHAR)
#endif
	      if (t != t1) p->in.left = makety( p->in.left,t,0,(int)t,0);
#ifdef LINT
	 if (t!=CHAR && t2!=SCHAR)
#endif
	      if (t != t2) p->in.right = makety( p->in.right,t,0,(int)t,0);
	 /* For the logop case, the proper integral promotions have
	  * occurred on the arguments, now make the result type INT */
	 if ( logop(o) ) t = INT;
    }

    p->in.type = t;
    p->fn.csiz = t;
    return(p);
}

int integral_promote( p ) NODE *p; { 

	/* The variables suf1 and suf2 keep track of "short unsigned bitfields"
	 * The issue is whether a field shorter than 32 bits promotes to
	 * "int" or "unsigned" in "usual arithmetic conversions".  The ANSI
	 * Standard states that "usual arithmetic conversions" are applied,
	 * however it is ambiguous if those conversions are to "int" or to
	 * "unsigned".  Pending a more formal ruling we choose the priniple
	 * of least surprise and choose "int".
	 *    suf1 is set if p->in.left is a short unsigned bitfield
	 *    suf2 is set if p->in.right is a short unsigned bitfield
	 * If either flag is set, the tests for "unsignedness" don't work.
	 */
        int t1 = p->in.left->in.type;
	int t2 = p->in.right->in.type;
	int t;
	int o = p->in.op;

	flag suf1 = ((t1==UNSIGNED)&&(p->in.left->in.op==FLD)&&
		     (UPKFSZ(p->in.left->tn.rval) != dimtab[p->in.left->fn.csiz]));
	flag suf2 = ((t2==UNSIGNED)&&(p->in.right->in.op==FLD)&&
		     (UPKFSZ(p->in.right->tn.rval) != dimtab[p->in.right->fn.csiz]));
	if (t1 == LONGDOUBLE || t2 == LONGDOUBLE) t = LONGDOUBLE;
	else if (t1 == DOUBLE || t2 == DOUBLE) t = DOUBLE;
	else if (t1 == FLOAT || t2 == FLOAT) t = FLOAT;
	else if (t1 == ULONG || t2 == ULONG) t = ULONG;
	else if ((t1 == LONG && ((t2 == UNSIGNED) && !suf2)) ||
		 (t2 == LONG && ((t1 == UNSIGNED) && !suf1))) t = ULONG;
	else if (t1 == LONG || t2 == LONG) t = LONG;
	else if (((t1 == UNSIGNED) && !suf1) ||
		 ((t2 == UNSIGNED) && !suf2)) t = UNSIGNED;
	else if (!logop(o)) 
            {
                t = INT;
#ifdef LINT
        /* bubble up the flag that indicates when old promotion rules would
         * have selected UNSIGNED instead of INT.  This happens when either
         * operand is uchar or ushort, or when either would
         * have been UNSIGNED due to a promotion lower in the subtree.
         */
        if ( t1==UCHAR || t1==USHORT || p->in.left->in.promote_flg ||
             t2==UCHAR || t2==USHORT || p->in.right->in.promote_flg )
                p->in.promote_flg = 1;
#endif /* LINT */
	   }
	else if (suf1 || suf2) t = INT;
	else {
	    /* Attempt to minimize the type promotions required.
	     * The resultant type is "INT" so no one can tell the
	     * difference (except for performance).
	     *
	     * The following chart can be constructed.  The column headings
	     * represent the types of one node, and the row headings represent
	     * the types of the other node.  The entries in the table
	     * represent the minimal type that can represent the values
	     * contained in the union of types.
	     */
	    static TWORD logtable[14][5] = {
		                        /* char   short  int  ushort  uchar */
	     /* char                  */ { CHAR,  SHORT, INT, INT,    SHORT },
	     /* short                 */ { SHORT, SHORT, INT, INT,    SHORT },
             /* int                   */ { INT,   INT,   INT, INT,    INT },
	     /* ushort                */ { INT,   INT,   INT, USHORT, USHORT },
	     /* uchar                 */ { SHORT, SHORT, INT, USHORT, UCHAR },
	     /* ICON <= -32769        */ { INT,   INT,   INT, INT,    INT },
	     /* -32768 <= ICON <= -129*/ { SHORT, SHORT, INT, INT,    SHORT },
	     /* -128 <= ICON <= -1    */ { CHAR,  SHORT, INT, INT,    SHORT },
	     /* 0 <= ICON <= 127      */ { CHAR,  SHORT, INT, USHORT, UCHAR },
	     /* 128 <= ICON <= 255    */ { SHORT, SHORT, INT, USHORT, UCHAR },
	     /* 256 <= ICON <= 32767  */ { SHORT, SHORT, INT, USHORT, USHORT },
	     /* 32768 <= ICON <= 65535*/ { INT,   INT,   INT, USHORT, USHORT },
	     /* 65536 <= ICON         */ { INT,   INT,   INT, INT,    INT }
	    };
	    int index1, index2;
	    long lval;
	    if (p->in.left->in.op != ICON) {
		switch(t1) {
		  case SCHAR:
		  case CHAR:   index1 = 0; break;
		  case SHORT:  index1 = 1; break;
		  case INT:    index1 = 2; break;
		  case USHORT: index1 = 3; break;
		  case UCHAR:  index1 = 4; break;
#ifdef DEBUGGING
		  default:     cerror("unexpected type in tmatch\n");
#endif		    
		}
	    } else {
		lval = p->in.left->tn.lval;
		if (lval <= -32769) index1 = 5;
		else if (lval <=  -129) index1 = 6;
		else if (lval <= -1) index1 = 7;
		else if (lval <= 127) index1 = 8;
		else if (lval <= 255) index1 = 9;
		else if (lval <= 32767) index1 = 10;
		else if (lval <= 65535) index1 = 11;
		else index1 = 12;
	    }
	    if (p->in.right->in.op != ICON || p->in.left->in.op == ICON) {
		switch(t2) {
		  case SCHAR:
		  case CHAR:   index2 = 0; break;
		  case SHORT:  index2 = 1; break;
		  case INT:    index2 = 2; break;
		  case USHORT: index2 = 3; break;
		  case UCHAR:  index2 = 4; break;
#ifdef DEBUGGING
		  default:     cerror("unexpected type in tmatch\n");
#endif		    
		}
	    } else {
		lval = p->in.right->tn.lval;
		if (lval <= -32769) index2 = 5;
		else if (lval <=  -129) index2 = 6;
		else if (lval <= -1) index2 = 7;
		else if (lval <= 127) index2 = 8;
		else if (lval <= 255) index2 = 9;
		else if (lval <= 32767) index2 = 10;
		else if (lval <= 65535) index2 = 11;
		else index2 = 12;
	    }
	    if (index1 < index2) {
		t = index1;
		index1 = index2;
		index2 = t;
	    }
	    t = logtable[index1][index2];
       }


	return( t );
   }
   
#endif /*ANSI*/

#endif /*BRIDGE*/

NODE *
makety( p, t, d, s, ta ) register NODE *p; register TWORD t,ta; {
	/* make p into type t by inserting a conversion */

	if( t == p->in.type ){
		p->fn.cdim = d;
		p->fn.csiz = s;
		p->fn.tattrib = ta;
		return( p );
		}

#ifdef BRIDGE
	u_do_simple_cvt(t, u_tree_type(p), p);
	return (u_pop_stack());
#else /* BRIDGE */
	if( t & TMASK ){
		/* non-simple type */
		p = block( PCONV, p, NIL, t, d, s);
		p->in.tattrib = ta;
#ifdef LINT
                p->in.cast_flg = p->in.left->in.cast_flg;
#endif /* LINT */
		return( clocal(p) );
		}

#ifndef ANSI
	if ( ( p->in.op == ICON ) && (t == DOUBLE || t== FLOAT) ) {
#else /*ANSI*/
	if ( ( p->in.op == ICON ) && (t == DOUBLE || t== FLOAT || t == LONGDOUBLE) ) {
#endif /*ANSI*/
			p->in.op = FCON;
			/* note: we're not permitting float length fcon 
			   constants.
			*/
#ifdef QUADC
			if (t == LONGDOUBLE) {
				if (ISFTP(p->in.type)) {
					p->qfpn.qval = _U_Qfcnvff_dbl_to_quad(p->fpn.dval);
					}
				else if (ISUNSIGNED(p->in.type) ){
					p->qfpn.qval = _U_Qfcnvxf_usgl_to_quad((unsigned long) p->tn.lval);
					}
				else {
					p->qfpn.qval = _U_Qfcnvxf_sgl_to_quad((long) p->tn.lval);
					}
				}
			else {
#endif /* QUADC */
				if( ISUNSIGNED(p->in.type) ){
					p->fpn.dval = (unsigned) p->tn.lval;
					}
				else {
					p->fpn.dval = p->tn.lval;
					}
#ifdef QUADC
			}
#endif /* QUADC */

			p->in.type = t;
			p->fn.csiz = t;
			p->in.tattrib = ta;
			return( clocal(p) );
		}

	p = block( SCONV, p, NIL, t, d, s);
        /* register class applies only to original node - don't propagate */
        p->in.tattrib = ta & (~ATTR_CLASS);
#ifdef LINT
        p->in.cast_flg = p->in.left->in.cast_flg;
#endif /* LINT */
	return( clocal(p) );
#endif /*BRIDGE*/

	}

#ifndef BRIDGE

#ifdef ANSI
/*
 * Is_Integral_Constant_Expression
 *
 * If the tree pointed to by *p is an integral constant expression, it will
 * be rewritten into an ICON node, with the value in p->tn.lval.
 * 
 * If the tree  pointed to by *p is not an integral constant expression, it
 * may have been modified (constant folded).
 */

int Is_Integral_Constant_Expression(p) register NODE *p; {
	register NODE *q;
	if (p->in.op != ICON) {
	    while (p->in.op == PAREN){ p->in.op = FREE; p = p->in.left; }
	    walkf(p,rm_paren);
	    q = optim(p);
	    if (q != p) { *p = *q; q->in.op = FREE; }
	}
	if (p->in.op == ICON && p->tn.rval == NONAME && (ISSIMPLE(p->in.type)))
	    return(1);
	else return(0);
}

/*
 * icons = Integral_Constant_Value
 *
 * Returns the value of an integal constant expression.  If it is not given
 * an integral constant expression, *error is -1
 */

icons(p, error) register NODE *p; flag *error; {
	/* if p is an integer constant, return its value */
	int val;

	*error = 0;
	if (Is_Integral_Constant_Expression(p))
		val = p->tn.lval;
	else	{
		/* "constant expected" */
		UERROR(MESSAGE(23));
		val = 1;
		*error = -1;
		}
	tfree( p );
	return(val);
	}
#else
icons(p, error) register NODE *p; flag *error; {
	/* if p is an integer constant, return its value */
	int val;
	*error = 0;

	if( p->in.op != ICON ){
		/* "constant expected" */
		UERROR( MESSAGE( 23 ) );
		val = 1;
		*error = -1;
		}
	else {
	     val = p->tn.lval;
	}
	tfree( p );
	return(val);
	}
#endif


/* 	the intent of this table is to examine the
	operators, and to check them for
	correctness.

	The table is searched for the op and the
	modified type (where this is one of the
	types INT (includes char and short), LONG,
	DOUBLE (includes FLOAT), and POINTER

	The default action is to make the node type integer

	The actions taken include:
		PUN	0x2	  check for puns
		CVTL	0x100	  convert the left operand
		CVTR	0x200	  convert the right operand
		TYPL	0x4	  the type is determined by the left operand
		TYPR	0x8	  the type is determined by the right operand
		TYMATCH	0x20	  force type of left and right to match, by
					inserting conversions
		PTMATCH	0x400	  like TYMATCH, but for pointers
		LVAL	0x40	  left operand must be lval
		CVTO	0x80	  convert the op
		NCVT	0x1	  do not convert the operands
		OTHER	0x800	  handled by code
		NCVTR	0x1000	  convert the left operand, not the right...

	*/

# define MINT 01  /* integer */
# define MDBI 02   /* integer or double */
# define MSTR 04  /* structure */
# define MPTR 010  /* pointer */
# define MPTI 020  /* pointer or integer */
# define MVOID 040  /* VOID */

LOCAL opact( p )  register NODE *p; {

	register mt12, mt1, mt2;
	register short o = p->in.op;
	NODE *r;

	mt12 = 0;

	switch( optype(o) ){
	case BITYPE:
		mt12=mt2 = moditype( p->in.right->in.type );
	case UTYPE:
		mt12 &= (mt1 = moditype( p->in.left->in.type ));
		}

	switch( o ){

	case NAME :
	case STRING :
#ifdef ANSI
	case WIDESTRING :
#endif 
	case FCON :
	case CALL :
	case UNARY CALL:
	case PAREN:
		{  return( OTHER ); }
	case UNARY MUL:
		{  return( OTHER+PATTRIB ); }
	case UNARY MINUS:
#ifdef ANSI
	case UNARY PLUS:
#endif 
		if( mt1 & MDBI ) return( OTHER );
		/* "operands of %s should have %s type" */
		UERROR( MESSAGE( 164 ), opst[o], "arithmetic" );
		return( NCVT );

	case COMPL:
		if( mt1 & MINT ) return( OTHER );
		/* "operands of %s should have %s type" */
		UERROR( MESSAGE( 164 ), opst[o], "integral" );
		return( NCVT );

	case UNARY AND:
		{  return( NCVT+OTHER ); }
	case CM:
		return ( OTHER );
	case NOT:
	case INIT:
	case ANDAND:
	case OROR:
		return( OTHER );
#ifndef IRIF
	case CBRANCH:
		return( NCVT );
#endif /* not IRIF */
	case MUL:
	case DIV:
		if( mt12 & MDBI ) return( TYMATCH );
		/* "operands of %s should have %s type" */
		UERROR( MESSAGE( 164 ), opst[o], "arithmetic" );
		return( NCVT );

	case MOD:
	case AND:
	case OR:
	case ER:
		if( mt12 & MINT ) return( TYMATCH );
		/* "operands of %s should have %s type" */
		UERROR( MESSAGE( 164 ), opst[o], "integral" );
		return( NCVT );

	case LS:
	case RS:
#ifndef ANSI
		if( mt12 & MINT ) return( /* LLINT+ */ TYMATCH+OTHER );
#else /*ANSI*/
		if( mt12 & MINT ) return( OTHER );
#endif /*ANSI*/
		/* "operands of %s should have %s type" */
		UERROR( MESSAGE( 164 ), opst[o], "integral" );
		return( NCVT );

#ifndef ANSI
	case EQ:
	case NE:
		if ( !(mt1|mt2) )	/* NULLs */
			return( VOIDCK+PTMATCH+PUN );
		if ( (mt1&MPTR && !mt2) || (mt2&MPTR && !mt1) )
			/* ptr .eq. NULL ? */
			return( VOIDCK+PTMATCH+PUN );
		/* fall thru */
	case LT:
	case LE:
	case GT:
	case GE:
		if( mt12 & MDBI ) return( VOIDCK+TYMATCH+CVTO );
		if( (mt12 & MPTR) || (mt12 & MPTI) ) return( VOIDCK+PTMATCH+PUN );
		if ( (mt1&MPTR) && (p->in.right->in.op==ICON))
			return ( VOIDCK+PTMATCH+PUN+RINT );
		if ( (mt2&MPTR) && (p->in.left->in.op==ICON))
			return ( VOIDCK+PTMATCH+PUN+LLINT );
		break;

#else /* not ANSI */

	case EQ:
	case NE:
		if( mt12 & MDBI ) return( TYMATCH+CVTO );
		if( (mt12 & MPTR) || (mt12 & MPTI) ) return( PTMATCH+PUN );
		break;
	case LT:
	case LE:
	case GT:
	case GE:
		if( mt12 & MDBI ) return( TYMATCH+CVTO+NOFTNPTRS );
		if( (mt12 & MPTR) || (mt12 & MPTI) ) return( PTMATCH+PUN+NOFTNPTRS );
		break;

#endif /* ANSI */

	case QUEST:
		return( TYPR+OTHER );

	case COMOP:
		return( TYPR+PATTRIB ); /* SWFfc00992 - Enable attrib check */

	case STREF:
#ifdef IRIF
	case DOT:
#endif /* IRIF */

		return( NCVTR+OTHER );

	case FORCE:
		return( TYPL );

	case COLON:
#ifdef ANSI
		if( (mt1 & MDBI) && (mt2 & MDBI)) return ( TYMATCH );
		if( (mt1 & MSTR) && (mt2 & MSTR) ) return( NCVT+TYPL+OTHER );
		if( (mt1 & MPTR) && (mt2 & MPTR ) ) return( PTMATCH+PUN );
		if( (mt1&MINT) && (mt2&MPTR) ) return( PTMATCH+PUN+LLINT );
		if( (mt1&MPTR) && (mt2&MINT) ) return( PTMATCH+PUN+RINT );
	        if( (mt1 == MVOID) && (mt2 == MVOID)) return( TYPL );
		break;
#else
		if( (mt1 & MDBI) && (mt2 & MDBI)) return (TYMATCH);
		if( (mt1 & MSTR) && (mt2 & MSTR) ) return( NCVT+TYPL+OTHER );
		if( (mt1 == 0) && (mt2 == 0) ) return( NCVT+TYPL+OTHER );
		if( (mt1 & MPTR) && (mt2 & MPTR ) ) return( TYPL+PTMATCH+PUN );
		if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+PUN+LLINT );
		if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+PUN+RINT );
		break;
#endif

	case ASSIGN:
		r = p->in.right;
		if ( blevel == 0 && r->in.op == NAME && ISPTR(r->in.type)
			&& ((r->nn.sym->sclass == EXTDEF) ||
			    (r->nn.sym->sclass == EXTERN)) )
		     /* "rhs address/value not known at compile time" */
		     UERROR( MESSAGE( 112 ));
#ifndef IRIF
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+OTHER+PATTRIB );
#else /* IRIF */
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+PATTRIB );
#endif /* IRIF */

#ifndef ANSI
		if ( mt2 == 0 && mt1&MPTR) return (TYPL+LVAL+PTMATCH+VOIDCK+PATTRIB);
#endif
		if( mt12 & MDBI ) return( TYPL+LVAL+TYMATCH+PATTRIB );
		if( mt12 == 0 ) break;
		if( mt1 & MPTR ) return( LVAL+PTMATCH+PUN+PATTRIB );
		if( mt12 & MPTI ) return( TYPL+LVAL+TYMATCH+PUN+PATTRIB );
		break;

# ifdef ANSI
	case ARGASG:
		/* argument conversion.
		 * Since this is "as if by assignment, it is very similar
		 * to the ASSIGN case, except no LVAL.  Still do PATTRIB,
		 * although the the checks there will be minimal.
		 * OTHER will strip away the ARGASG node, and lhs as
		 * this was all a kludge to get the type checking
		 * and conversions done.
		 */
		if( mt12 & MSTR ) return( OTHER+NCVT+TYPL+PATTRIB );
		if( mt12 & MDBI ) return( OTHER+TYPL+TYMATCH+PATTRIB );
		if( mt12 == 0 ) {
		    UERROR( MESSAGE( 229 ), arg_fname, arg_name );
		    return(NCVT);
		}
		if( mt1 & MPTR ) return( OTHER+PTMATCH+PUN+PATTRIB );
		if( mt12 & MPTI ) return( OTHER+TYPL+TYMATCH+PUN+PATTRIB );
		UERROR( MESSAGE( 229 ), arg_fname, arg_name );
		return(NCVT);
# endif

	case RETURN:
#ifndef IRIF
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+OTHER+PATTRIB );
#else /* IRIF */
		if( mt12 & MSTR ) return( LVAL+NCVT+TYPL+PATTRIB );
#endif /* IRIF */

#ifndef ANSI
		if ( mt2 == 0 && mt1&MPTR) return (TYPL+LVAL+PTMATCH+VOIDCK);
#endif
		if( mt12 & MDBI ) return( TYPL+LVAL+TYMATCH+PATTRIB );
		if( mt12 == 0 ) break;
		if( mt1 & MPTR ) return( LVAL+PTMATCH+PUN+PATTRIB );
		if( mt12 & MPTI ) return( TYPL+LVAL+TYMATCH+PUN+PATTRIB );
		break;

	case CAST:
#ifndef ANSI
		if ( mt2 == 0 && mt1&MPTR) return (TYPL+PTMATCH+VOIDCK+BEFORE_CVT);
		if( mt1==0 ) return(TYPL+TYMATCH);
#else
		if( mt1==MVOID ) return(TYPL+TYMATCH);
#endif
		if( mt12 & MDBI ) return( TYPL+TYMATCH+BEFORE_CVT);
		if( mt12 != 0 ) {
		     if( mt1 & MPTR ) return( PTMATCH+PUN+BEFORE_CVT );
		     if( mt12 & MPTI ) return( TYPL+TYMATCH+PUN+BEFORE_CVT );
		   }
	        /* "types of CAST operands must be scalar unless first operand is VOID" */
	        UERROR( MESSAGE( 177 ) );
	        return( NCVT );

	case ASG LS:
	case ASG RS:
#ifndef ANSI
		if( mt12 & MINT ) return( TYPL+LVAL+OTHER+PATTRIB+LONGASSIGN );
#else
		if( mt12 & MINT ) return( TYPL+LVAL+PATTRIB+LONGASSIGN );
#endif
		break;

	case ASG MUL:
	case ASG DIV:
		if( mt12 & MDBI ) return( LVAL+TYMATCH+PATTRIB+LONGASSIGN );
		break;

	case ASG MOD:
	case ASG AND:
	case ASG OR:
	case ASG ER:
		if( mt12 & MINT ) return( LVAL+TYMATCH+PATTRIB+LONGASSIGN );
		break;

	case ASG PLUS:
	case ASG MINUS:
		if( mt12 & MDBI ) return( TYMATCH+LVAL+PATTRIB+LONGASSIGN );
		if( (mt1&MPTR) && (mt2&MINT) ) 
		     return( TYPL+LVAL+CVTR+RINT+PATTRIB );
		break;
	case INCR:
	case DECR:
		if( mt12 & MDBI ) return( TYMATCH+LVAL+PATTRIB );
		if( (mt1&MPTR) && (mt2&MINT) ) 
		     return( TYPL+LVAL+CVTR+RINT+PATTRIB );
		/* "operands of %s should have %s type" */
		UERROR( MESSAGE( 164 ), opst[o], "scalar" );
		return( NCVT );

	case MINUS:

#ifndef ANSI
#ifndef IRIF
		if( mt12 & MPTR ) return( CVTO+PTMATCH+PUN+PATTRIB );
#else /* IRIF */
		if( mt12 & MPTR ) return( PTMATCH+PUN+PATTRIB );
#endif /* IRIF */
		if( mt2 & MPTR+PATTRIB ) break;

#else /*ANSI*/
#ifndef IRIF
		if( mt12 & MPTR ) return( CVTO+PTMATCH+PUN+PATTRIB+NOFTNPTRS );
#else /* IRIF */
		if( mt12 & MPTR ) return( PTMATCH+PUN+PATTRIB+NOFTNPTRS );
#endif /* IRIF */
		if( mt2 & MPTR ) break;
		if( mt12 & MDBI ) return( TYMATCH );
		if( (mt1&MPTR) && (mt2&MINT) ) 
		     return( TYPL+CVTR+RINT+PATTRIB+NOFTNPTRS );
		break;
#endif /*ANSI*/
	case PLUS:
		if( mt12 & MDBI ) return( TYMATCH );
#ifndef ANSI
		if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+CVTR+RINT+PATTRIB );
		if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+CVTL+LLINT+PATTRIB );
#else /*ANSI*/
		if( (mt1&MPTR) && (mt2&MINT) ) 
		     return( TYPL+CVTR+RINT+PATTRIB+NOFTNPTRS );
		if( (mt1&MINT) && (mt2&MPTR) ) 
		     return( TYPR+CVTL+LLINT+PATTRIB+NOFTNPTRS );
#endif /*ANSI*/
		break;
#ifdef IRIF
	case INDEX:
		if( (mt1&MPTR) && (mt2&MINT) ) return( TYPL+RINT+PATTRIB );
		if( (mt1&MINT) && (mt2&MPTR) ) return( TYPR+LLINT+PATTRIB );
		break;
#endif /* IRIF */
		}
	/* "operands of %s have incompatible or illegal types" */
	UERROR( MESSAGE( 89 ), opst[o] );
	return( NCVT );
	}


LOCAL moditype( ty ) TWORD ty; {

	switch( ty ){

	case VOID:
#ifdef ANSI
	        return( MVOID );
#else
		return( 0 );
#endif /*ANSI*/
	case UNDEF:
		return(0);

	case STRTY:
	case UNIONTY:
		return( MSTR );

#ifdef ANSI
	case SCHAR:
#endif 
	case CHAR:
	case SHORT:
	case UCHAR:
	case USHORT:
	case UNSIGNED:
	case ULONG:
	case INT:
	case LONG:
		return( MINT|MDBI|MPTI );
	case FLOAT:
	case DOUBLE:
#ifdef ANSI
	case LONGDOUBLE:
#endif /*ANSI*/
		return( MDBI );
	default:
		return( MPTR|MPTI );

		}
	}

NODE *
doszof( p )  register NODE *p; {
	/* do sizeof p */
	register int i = 0;

#ifdef LINT_TRY
	if (haseffects (p) )
	     /* "side effects on objects in sizeof() are ignored" */
	     WERROR( MESSAGE( 90 ));
#endif

	/* ANSI proposes to make sizeof(bitfield) illegal. So do we. */
	if (p->fn.op == FLD) 
	     /* "%s should not be arguments to sizeof()" */
	     UERROR( MESSAGE( 129 ), "bitfields" );

	if (p->fn.op == ICON)
	     i = SZINT/SZCHAR;
	else if( ISFTN( p->in.type ))
	     /* "%s should not be an argument to sizeof()" */
	     UERROR( MESSAGE( 129 ), "function designators" );
	else
#ifndef IRIF
	     i = tsize( p->in.type, p->fn.cdim, p->fn.csiz, (char *)NULL ,0)/SZCHAR; /* SWFfc00726 fix */
#else /* IRIF */
	     i = ir_t_bytesz( p );
#endif /* IRIF */

	tfree(p);
	/* "'sizeof' returns a zero value" */
	if( i <= 0 ) WERROR( MESSAGE( 99 ) );

#ifndef ANSI
	p = bcon(i, 0);
	p->tn.type = ENUNSIGN(p->tn.type);	/* sizeof is unsigned */
#else /*ANSI*/
	p = bcon(i, UNSIGNED);
#endif /*ANSI*/
	return( p );
	}
#endif /*BRIDGE*/


# ifdef DEBUGGING
eprint( p, down, a, b ) register NODE *p; int *a, *b; register int down; {
	short ty;

#ifdef XPRINT
	if ((treedebug)&&!(down)) xtprint(p);
#endif
	*a = *b = down+1;
	while( down > 1 ){
		(void)prntf( "\t" );
		down -= 2;
		}
	if( down ) printf( "    " );

	ty = optype( p->in.op );

	printf("0x%x) %s, ", p, opst[p->in.op] );

#ifdef ANSI
	if (p->in.op == PHEAD) {
		printf(" phead = 0x%x", p->ph.phead);
		printf(" nparam = %d", p->ph.nparam);
		printf(" flags = %o\n", p->ph.flags);
		fflush(stdout);
		return;
		}
#endif

	if( ty == LTYPE ){
	        if ((p->in.op == NAME) && (p->nn.sym != NULL))
#ifdef BRIDGE
			printf("%s ",p->tn.name);
#else
                       if (p->tn.rval == HAVENAME)
                                printf("%s ",p->tn.name);
                        else
                                printf("%s ",p->nn.sym->sname);
#endif /*BRIDGE*/
		printf("lval/rval=");
		printf( CONFMT2, p->tn.lval );
		printf( ", 0x%x, ", p->tn.rval );
		}
	tprint( p->in.type, p->in.tattrib );
#ifndef IRIF
#ifdef C1_C
#ifdef BRIDGE
	if (optlevel >= 2) {
		printf(" tag=%d",p->in.c1tag);
		if (p->in.fixtag) printf("!");
	}
#else
	printf(" tag=%d",p->in.c1tag);
        if (p->in.fixtag) printf("!");
        if (p->in.c1tag!=NULL) printf("(%d),", p->in.c1tag->offset/8);
#endif /*BRIDGE*/
#endif /* C1_C */
#endif /* not IRIF */
	printf(" cdim/csiz=");
	printf( ", 0x%x, 0x%x\n", p->fn.cdim, p->fn.csiz );
	fflush( stdout );
	}
# endif	/* DEBUGGING */

#ifndef IRIF
#ifndef BRIDGE
prtdcon( p ) register NODE *p; {
	int i;

	if( p->in.op == FCON )
		{
	  	(void)locctr( ADATA );

		/* put out a "lalign" pseudo to align succeeding floats. */
		if ( lalignflag==0)
			{
			lalignflag = 1;
# ifndef LINT
# ifndef C1_C
			(void)fprntf(outfile, "\tlalign\t4\n");
# else
                        p2pass( "\tlalign\t4");
# endif /* C1_C */
# endif
			}

		if (p->fpn.dval)
			{
# ifndef C1_C
                        deflab( (unsigned)(i = GETLAB()) );
# else
                        deftextlab( i = GETLAB() );
# endif /* C1_C */
# ifdef QUADC
			fincode(p->qfpn.qval,p->in.type==FLOAT  ? SZFLOAT  :
                                             p->in.type==DOUBLE ? SZDOUBLE :
                                                                  SZLONGDOUBLE);
# else
			fincode(p->fpn.dval, p->in.type==FLOAT?	  SZFLOAT :
								  SZDOUBLE );
# endif
			}
	  	else
			{
			if (fzerolabel==0)
				{
# ifndef C1_C
                                deflab( fzerolabel = GETLAB() );
# else
                                deftextlab( fzerolabel = GETLAB() );
# endif /* C1_C */
#ifdef QUADC
				qcon.d[0] = 0;
				qcon.d[1] = 0;
				fincode(qcon,SZLONGDOUBLE);
#else
				fincode(0.0, SZDOUBLE );
#endif
				}
			i = fzerolabel;
			}
	    	p->tn.lval = 0;
	    	p->tn.rval = -i;
		p->nn.sym = NULL;
	    	p->in.op = NAME;
		}
	}

#endif /*BRIDGE*/


/* longassign() translates an asgop into its longer form in cases where the
   collapsed form is too difficult to do correctly. In particular, this
   situation arises when the lhs type is smaller than the rhs type for 
   scalars or floating pt types.
*/

NODE * longassign(p)	register NODE *p;
{
	register TWORD tl, tr, opt;
	NODE *l, *r, *temp, *temp1, *temp2,* result, *q;
	TWORD ty; /* handles are contained in the left operand */
	int tempoff,iscomplex;

	l = p->in.left;
	r = p->in.right;
	tl = l->in.type;

	switch(p->in.op) {
	  case ASG PLUS:
	  case ASG MINUS:
	  case ASG MUL:
	  case ASG DIV:
	    if ((p->in.left->in.type == FLOAT)||
		(p->in.right->in.type == FLOAT))
	      break;
	    /* fall thru */
	  case ASG AND:
	  case ASG MOD:
	  case ASG OR:
	  case ASG ER:
#ifdef BRIDGE
	    if (u_get_size(p->in.left) < u_get_size(p->in.right))
#else
	    if (BITSINTYPE(p->in.left) < BITSINTYPE(p->in.right))
#endif /* BRIDGE */
	      break;
	    /* fall thru */
	  case ASG LS:
	  case ASG RS:
	    if (haseffects(p->in.left)) break;
	    /* fall thru */
	  case ASSIGN:
	  case INCR:
	  case DECR:
	    return(p);
#ifdef DEBUGGING
	  default:
	    cerror("unexpected case in long assign, %d",p->in.op);
#endif
	}

	tr = r->in.type;

	if ( ISFTP(tl) || ISFTP(tr) )
		{
#ifdef ANSI
		if ((tl == LONGDOUBLE) || (tr == LONGDOUBLE))
			opt = LONGDOUBLE;
		else
#endif /* ANSI */

			{
			/* Both the library routines and the 68881 do their internal
			   math in full precision even if the type is FLOAT. Hence
			   no precision is lost by specifying a FLOAT operation.
			*/
			opt = (tl == DOUBLE || tr == DOUBLE)?
				DOUBLE : FLOAT;
			}
		}
	else	/* scalar coercion */
#ifndef ANSI
		opt = ( ISUNSIGNED(tl) || ISUNSIGNED(tr) )? UNSIGNED : INT;
#else
		{
		    /* usual arithmetic conversions */
		    if ((tl==ULONG)||(tr==ULONG))
			opt = ULONG;
		    else if (((tl==LONG)&&(tr==UNSIGNED))||
			     ((tr==LONG)&&(tl==UNSIGNED)))
			opt = ULONG;
		    else if ((tl == UNSIGNED)||(tr == UNSIGNED))
			opt = UNSIGNED;
		    else opt = INT;
		}
#endif
	/* create a temporary to hold the address of the left-hand side.
	   Make certain the left side is a lval before splitting the tree.
	   The cases remaining are: FLD, UNARY MUL, NAME, OREG, REG. */
	if (notlval(l)) return(p);
	while (l->in.op == PAREN) { l->in.op = FREE; p->in.left=l=l->in.left; }
 	iscomplex = haseffects(l);
	switch(l->in.op) {
	  case UNARY MUL:
	    if (iscomplex) {
		q = NULL; 
#ifdef C1_C
		if (l->in.fixtag) l->in.left->in.fixtag = 1;
#endif  /* C1_C */
		l->in.op = FREE;
		l = l->in.left;
	    }
	    break;
	  case FLD: 
	    if ((l->in.left->in.op == UNARY MUL) && iscomplex) {
		q = talloc(); 
		*q = *l; 
		l->in.op = FREE; 
		l->in.left->in.op = FREE;
		l = l->in.left->in.left;
	    }
	    break;
	  case NAME:
	  case OREG:
	  case REG:
	  default: 
	    break;
	}
	if (!iscomplex) {
	    p->in.op -= 1;
	    p->in.type = opt;
	    p->in.right = makety(r,opt,0,(int)opt,0);
	    l = t1copy(l);
	    p->in.left = makety(p->in.left,opt,0,(int)opt,0);
	    p = makety(p,tl,0,(int)tl,0);
	    p = block(ASSIGN,l,p,tl,0,(int)tl);
	    return(p);
	} else {
	    /* introduce a temporary to hold the left side */
#ifdef BRIDGE
	    ty = INCREF(tl);    /* type of temp */
	    tempoff = u_alloc_temp(4 /* size of temp in bytes */);
#else
	    ty = INCREF(l->in.type);
	    (void)upoff(32 /*size*/, 32 /*align*/, &autooff);		
	    tempoff = -autooff;
#endif /*BRIDGE*/

#ifdef ONEPASS
#  ifndef BRIDGE
	    		/* ??? could do something like "freetemp(1);" ,
				to get stmt level reuse of these temps */
	    p2upoff( autooff );
#  endif /*BRIDGE*/
#else
#  ifdef C1_C
#    ifdef BRIDGE
	    puttyp(C1OREG, 14, l->in.type, 0);
	    p2word(tempoff);
#    else
	    p1maxoff += 32; /* normally set at beginning of block by bccode */
	    /* OP=C1OREG, VAL=%a6, REST=type int */
	    puttyp(C1OREG, 14, l->in.type, 0);
	    p2word(-autooff/SZCHAR);
#    endif /*BRIDGE*/
	    p2word(C1PTR | (blevel%256) );	/* C1 pointer attribute */
	    p2name("asgop-tmp");
#  endif /* C1_C */
#endif /* ONEPASS */
#ifdef BRIDGE
	    /* create temp */
	    u_push_oreg(A6, tempoff, ty);
	    temp = u_pop_stack();
#else
	    temp = block(REG,NIL,NIL,ty,l->fn.cdim,l->fn.csiz);
	    temp->tn.lval = 0;
	    temp->tn.rval = 14;
	    temp=block(PLUS,temp,makety(offcon((OFFSZ)tempoff),INT,0,INT,0),
		ty,l->fn.cdim,l->fn.csiz);
	    temp = buildtree(UNARY MUL,temp,NIL);
#endif /* BRIDGE */

	    temp1 = t1copy(temp); 
#ifdef BRIDGE
	    u_push_op(UNARY MUL, tl, temp1, NULL);
	    temp1 = u_pop_stack();
#else
	    temp1 = buildtree(UNARY MUL,temp1,NIL);
#endif /* BRIDGE */
	    if (q != NULL) {
		q->in.left = temp1;
		temp1 = q;
	    }
	    temp2 = t1copy(temp1); 
#ifdef BRIDGE
	    /* Transformation example.  Transform
	     *    *p++ += i;
	     * to
	     *    temp = p++, *temp = (tl) ((opt) *temp + (opt) i);
	     * where
	     *    tl  = lhs type
	     *    opt = operation type
	     */
	    /* result = (opt) *temp + (opt) i */
	    result = block(p->in.op-1,
			   makety(temp2,opt,0,(int)opt,0),
			   makety(r,opt,0,(int)opt,0),
			   opt, NOPREF, opt);
	    /* *temp = (tl) result */
	    result = block(ASSIGN,temp1,makety(result,tl,0,(int)tl,0),
			   tl, NOPREF, tl);
	    /* temp = p++, result */
	    result = block(COMOP,block(ASSIGN,temp,l,ty,NOPREF,ty),result,
			   tl, NOPREF, tl);
#else
	    result = buildtree(p->in.op-1,
			       makety(temp2,opt,0,(int)opt,0),
			       makety(r,opt,0,(int)opt,0));
	    result = buildtree(ASSIGN,temp1,makety(result,tl,0,(int)tl,0));
	    result = buildtree(COMOP,buildtree(ASSIGN,temp,l),result);
#endif /* BRIDGE */
	    p->in.op = FREE;
	    return(result);
	}
}


#endif /* not IRIF */

#ifndef BRIDGE

#ifdef DEBUGGING
flag edebug = 0;
#endif /* DEBUGGING */

ecomp( p ) register NODE *p; {

#ifndef IRIF
# ifdef DEBUGGING
	if( edebug ) {
		(void)prntf("entering ecomp: 0x%x\n", p);
		fwalk( p, eprint, 0 );
		}
# endif /* DEBUGGING */
	if( unionflag ){
		fwalk( p, fixstasg, 0);
# ifdef DEBUGGING
		if( edebug > 1 ){
			(void)prntf("ecomp after fixstasg: 0x%x\n", p);
			fwalk( p, eprint, 0 );
			}
# endif
		}
# ifdef LINT_TRY
	if( !reached && !reachflg){
# else
	if( !reached ){
# endif
		/* "statement not reached" */
		WERROR( MESSAGE( 100 ) );
		reached = 1;
		}
	p = optim(p);
# ifdef DEBUGGING
	if( edebug > 1) {
		(void)prntf("ecomp after optim: 0x%x\n", p);
		fwalk( p, eprint, 0 );
		}
# endif /* DEBUGGING */
	walkf( p, prtdcon );
# ifdef DEBUGGING
	if( edebug > 1) {
		(void)prntf("ecomp after prtdcon: 0x%x\n", p);
		fwalk( p, eprint, 0 );
		}
# endif /* DEBUGGING */
	(void)locctr( PROG );
	ecode( p );
	tfree(p);
	(void)locctr( PROG );
	}
#else /* IRIF defined */

# ifdef LINT_TRY
	if( !reached && !reachflg){
# else
	if( !reached ){
# endif
		/* "statement not reached" */
		WERROR( MESSAGE( 100 ) );
		reached = 1;
		}
	p = do_optim( p );
#ifdef HPCDB
	if( cdbflag ) (void) sltnormal();
#endif /* HPCDB */
	ir_eval( p );
	tfree( p );
   }
#endif /* IRIF */

#endif /*BRIDGE*/

# ifndef IRIF 
# ifdef STDPRTREE
#	 ifdef C1_C



/* Compaction of type and attribute information in the intermediate file node.
 *  The format of the node is as follows:
 *                     |<-16 bits type->|<- 8 ->|<- 8 ->|
 *                     |nnaammmmmmtttttt|  val  |  op   |
 *  where  tttttt = the 6 bits of basic type
 *	   mmmmmm = three 2-bit type modifiers
 *  	       aa = a 2-bit volatile/no-alias modifier of the basic type
 *  	       nn = indicates optional type extension words follow
 *			(nn=10 for 1 long word, nn=11 for 2 words)
 * The first extension word contains the remainder of the modifiers and
 *  6 attribute fields:
 *		       |<-20 bits modifier->|<-   12   ->|
 *		       |mmmmmmmmmmmmmmmmmmmm|aaaaaaaaaaaa|
 *
 * The remaining attribute fields reside in an optional second extension word:
 *		       |<- 14 bits  ->|  unused          |
 *		       |aaaaaaaaaaaaaa|------------------|
 *
 * This scheme is somewhat convoluted, but results in no extension words for
 *  the common case of few modifiers and/or attributes of basic type only.
 *  If extension words are needed, this packing is designed for a minimum of
 *  shifting.
 */

puttyp(op, val, type, attr)
int op, val, type, attr;
{
register int node;

	type = ctype(type); /* get rid of LONG's et.al. for C1 */
	node = (op)&0xff | ((val)&0xff)<<8 | ((type)&0xfff)<<16 |((attr)&3)<<28;
	if (!(attr&0xfffc0000)) { /* 0 or 1 extension words */
		if ((!((type)&0xfffff000)) && (!((attr)&0xffffffc0))) {
		    p2word(node);
		}
		else { /* 1 extension word */
		    p2word( 0x80000000 | node );
		    p2word(((type)&0xfffff000)|((attr)&0x3ffc0)>>6);
		}
	}
	else { /* 2 extension words */
		p2word( 0xc0000000 | node );
		p2word(((type)&0xfffff000)|((attr)&0x3ffc0)>>6);
		p2word((attr)&0xfffc0000);
	}
}

#ifndef BRIDGE

/* putsym is used to write a C1OREG or C1NAME record.  Its primary purpose
 *  is to convert enumty to the proper scalar (char, short, or int)
 */
putsym(op, val, symptr)
int op, val;
register struct symtab *symptr;
{
register TWORD ty;
	ty = symptr->stype;
#ifndef ANSI
	if( (BTYPE(ty)) == ENUMTY || BTYPE(ty) == MOETY ) {
		if( dimtab[ symptr->sizoff ] == SZCHAR ) ty = CHAR;
		else if( dimtab[ symptr->sizoff ] == SZSHORT ) ty = SHORT;
		else ty = INT;
		}
#endif /*ANSI*/
	puttyp(op, val, ty, symptr->sattrib);
	
}


/* primary_tag emits an ARRAYREF or STRUCTREF on the highest node in the tree
 *  corresponding to a fixed memory location, usually a UNARY MUL.  ARRAYREF
 *  is used to tag arrays of all object types, except arrays of structs or
 *  unions (which are tagged with STRUCTREF to discourage c1 from assigning
 *  an element to a register).
 */
void
primary_tag(idptr)
struct symtab *idptr;
{
register TWORD styp;

	styp = idptr->stype;
	if (ISARY(styp))
	    {
	    /* It's an array of some type */
	    if (BTYPE(styp)==STRTY || BTYPE(styp)==UNIONTY)
		{
		/* Need to special-case arrays of structs 
		 *    (including multi-dimensional) 
		 */
		    styp = DECREF(styp);
		    while ( ISARY(styp) )
			{
			    styp = DECREF(styp);
			}
		    /* At this point, contiguous ARY modifiers are gone */
		    if (styp==STRTY || styp==UNIONTY)
			p2triple(STRUCTREF, 1, 0); /* e.g., ARY STRTY */
		    else
			p2triple(ARRAYREF, 0, 0);  /* e.g., ARY PTR STRTY */
		}
	    else
		p2triple(ARRAYREF, 0, 0);
	    }
	else
	    p2triple(STRUCTREF, 1, 0); /* VAL=1 means primary ref*/
	switch (idptr->sclass) 
		{
		case AUTO:
		case PARAM:
			p2word(idptr->offset/SZCHAR);
			break;
		default:
			p2word(0);
		}
}




/* secondary_tag emits a secondary STRUCTREF tag on OREG, ICON, or NAME
 *  nodes when appropriate.  The rule is to tag these nodes if they are not
 *  already primary tagged, and if they are of type strty or any-dimension
 *  array of strty.  Pointers to structs and functions returning structs are
 *  not tagged.  The symbol table information is checked because the type
 *  information on the node of an element reflects the type of that member,
 *  not a struct.  The node type is checked in case a pointer to a struct
 *  has been dereferenced.
 *
 * The caller should make sure this node is not already primary tagged.
 */
void 
secondary_tag(symptr)
struct symtab *symptr;
{
register TWORD styp;

	if (symptr != NULL)
		{
		styp = symptr->stype;
		if (BTYPE(styp)==STRTY || BTYPE(styp)==UNIONTY)
			{ 
			/* candidate for tagging - allow only ARY modifiers */
			while ( styp & TMASK )
				{
				if ( ISPTR(styp) || ISFTN(styp) )  return;
				styp = DECREF(styp);
				}
			p2triple(STRUCTREF, 0, 0);
			}
		}
	return;

}
#endif /*BRIDGE*/



prtree(p) register NODE *p; {

	register struct symtab *q;
	register short ty;
	register talgn;
	char buf[32];
	register struct symtab *sidptr;
#ifdef NEWC0SUPPORT
	NODE *params;
	char buff[1024];
	TWORD typelist[98];
	char *bp;
	int nparams;
#endif /* NEWC0SUPPORT */

#		 ifdef MYPRTREE
	MYPRTREE(p);  /* local action can be taken here; then return... */
#		 endif

	/* In ANSI mode, convert types into a form the backend understands.
	 * In non-ANSI mode, this is already done */
#ifndef BRIDGE
#ifdef ANSI
	p->in.type = ctype(p->in.type);
#endif
#endif /*BRIDGE*/
	/* convert to OREG prior to pass2 oreg2() for c1's benefit */
	/* leave the tree intact for subsequent short-circuit rewriting */
	if (p->in.op == UNARY MUL) 
		{
		register NODE *pl;
		pl = p->in.left;
		if ((pl->in.op == PLUS || pl->in.op == MINUS) &&
		    pl->in.left->in.op == REG &&
		    pl->in.left->tn.rval == 14 /* %a6 */ &&
		    pl->in.right->in.op == ICON &&
		    pl->in.right->tn.rval == NONAME &&
		    !pl->in.fixtag) 
			{
			/* The test for pl->in.fixtag above avoids rewriting
			 * (&s)->a into an OREG, losing the STRUCTREF on
			 * the pointer portion (+ or -) in the process.
			 */
			if (p->in.fixtag) primary_tag(p->in.c1tag);
			if (!p->in.fixtag) secondary_tag(p->in.c1tag);
			puttyp(OREG, pl->in.left->tn.rval, p->in.type, 
								p->in.tattrib);
			if (pl->in.op == PLUS)
				p2word(pl->in.right->tn.lval);
			else
				p2word(-pl->in.right->tn.lval);
			return;
			}
		}
	else if (p->in.op == FORCE)
		{
		/* c1/codegen doesn't understand () nodes a children of
		 * force.  sigh.
		 * Strip them out, but we can't modify the tree so just
		 * skip the extra puts, sigh */
		NODE *l = p->in.left;
		while (l->in.op == PAREN) l = l->in.left;
		prtree(l);
#ifdef NEWC0SUPPORT
		/*
		   p->in.rall == 0 means that this is a return force,
		   put out a #c0 q comment for c0
		*/
		if (c0forceflag)
			{
			p2pass("#c0 q");
			}
#endif /* NEWC0SUPPORT */
		puttyp(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
		return;
		}

	ty = optype(p->in.op);

	if( ty != LTYPE ) prtree( p->in.left );
	if( ty == BITYPE ) prtree( p->in.right );

	/* handle special cases */

	sidptr = p->in.c1tag;
	if (p->in.fixtag) primary_tag(sidptr);
	switch( p->in.op ){

	case NAME:
		/* Secondary STRUCTREF */
		if (!p->in.fixtag) secondary_tag(sidptr); 
		/* print external name */
		if (p->tn.lval) {
			puttyp(p->in.op,1,p->in.type,p->in.tattrib);
			p2word(p->tn.lval);
		} else 
			puttyp(p->in.op, 0, p->in.type, p->in.tattrib);
		/* need to write rval because of pass 2 checks for labels,
		 *   e.g., ISFZERO   */
		/* p2word(p->tn.rval); */
		if( p->nn.sym != NULL ){
#ifdef BRIDGE
			p2name( exname(p->tn.name));
#else /*BRIDGE*/
			q = p->nn.sym;
			p2name( exname(q->sname) );
#endif /*BRIDGE*/
			}
		else { /* label */
			(void)sprntf(buf, LABFMT, -p->tn.rval );
			p2name(buf);
		}
		break;
		
			
	case ICON:
		/* Secondary STRUCTREF tags on certain nodes so that C1 will
		 *   know which of its symbol tables to search.  Tag this node
		 *   only if the fixtag has not already tagged it above, and if
		 *   it represents a structure name.
		 *   (U& of NAME) is converted by optim to ICON, retaining
		 *   the U& type, which means it is a pointer to the element
		 *   type rather than STRTY.
		 */
		if (!p->in.fixtag) secondary_tag(sidptr);

		if( p->tn.rval == NONAME ) {
			puttyp(p->in.op, 0, p->in.type, p->in.tattrib);
			p2word(p->tn.lval);
		} else {
			puttyp(p->in.op, 1, p->in.type, p->in.tattrib);
			p2word(p->tn.lval);
#ifdef BRIDGE
			if (p->tn.name != NONAME) 
				p2name( exname(p->tn.name));
#else /*BRIDGE*/
			if (p->tn.rval == HAVENAME){
				p2name(p->in.name);
			} else if (p->nn.sym != NULL) {
				q = p->nn.sym;
				p2name( exname(q->sname) );
                        }
#endif /*BRIDGE*/
			else {
				(void)sprntf(buf, LABFMT, -p->tn.rval);
				p2name(buf);
			}
		}
		break;

		
	case STARG:
	case STASG:
	case STCALL:
	case UNARY STCALL:
		/* print out size */
#ifdef BRIDGE
		talgn = u_get_stalign(p);
		puttyp(p->in.op,talgn,p->in.type,p->in.tattrib);
		p2word(u_get_stsize(p));
#else /*BRIDGE*/
		/* use lhs size, in order to avoid hassles with the structure `.' operator */

		/* note: p->in.left not a field... */
		talgn=talign(STRTY, p->in.left->fn.csiz, FALSE)/SZCHAR;
		puttyp(p->in.op,talgn,p->in.type,p->in.tattrib);
		p2word((tsize(STRTY, p->in.left->fn.cdim, p->in.left->fn.csiz, NULL ,0)+SZCHAR -1)/SZCHAR); /* SWFfc00726 fix */
#endif /*BRIDGE*/
#ifdef NEWC0SUPPORT
		if ((p->in.op == STCALL) || (p->in.op == UNARY STCALL)) goto calllabel;
#endif /* NEWC0SUPPORT */
		break;

	case OREG:
		if (!p->in.fixtag) secondary_tag(sidptr);
		puttyp(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
		p2word(p->tn.lval);
		break;

	case FEXPR:
		p2triple(FEXPR, 0, lineno);
		break;
	case FLD:
		puttyp(p->in.op, 0, p->in.type, p->in.tattrib);
		p2word(p->tn.rval);
		break;

	case PLUS:
	case MINUS:  
		/* Preceed with secondary STRUCTREF tag in the special FOREG
		 *   form (+/- REG ICON) when it points to a structure.
		 * Used to test BTYPE(p->in.type)==STRTY, but then a struct
		 *   containing an array has type PTR to array elem in p->in.
		 */
		if (p->in.left->in.op == REG &&
		    p->in.right->in.op == ICON &&
		    (sidptr != NULL) && !p->in.fixtag )
			secondary_tag(sidptr);
		puttyp(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
		break;

#ifdef NEWC0SUPPORT
	case CALL:
	case UNARY CALL:
		puttyp(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
calllabel:
		if (c0flag)
			{
			nparams = 0;
			for (params=p->in.right;params->in.op == CM;params=params->in.left)
				if (nparams < 98)
					typelist[nparams++] =
						params->in.right->in.type;
			if (params != NULL)
				typelist[nparams++] = params->in.type;
			bp = buff + sprintf(buff,"#c0 c%.8x,%.4x,%.8x",
						ctype(p->in.type),nparams,lineno);
			while (nparams > 0)
				bp += sprintf(bp,",%.8x",ctype(typelist[--nparams]));
			p2pass(buff);
			}
		break;
#endif /* NEWC0SUPPORT */

	default:
		puttyp(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
		}


	}

#	 else	/* C1_C */
#		 ifndef BRIDGE

p2tree(p) register NODE *p; {
	short ty;
	char buf[12];

#		 ifdef MYP2TREE
	MYP2TREE(p);  /* local action can be taken here; then return... */
#		 endif

	switch( p->in.op ){

	case STCALL:
	case UNARY STCALL:
	case STARG:
	case STASG:
		/* set up size parameters */

		p->stn.stsize = (tsize(STRTY,p->in.left->fn.cdim,p->in.left->fn.csiz, NULL, 0) /* SWFfc00726 fix */
			+SZCHAR-1)/SZCHAR;
		p->stn.stalign = talign(STRTY,p->in.left->fn.csiz, FALSE)/SZCHAR;
		break;

	case NAME:
	case ICON:
		if( p->tn.rval == NONAME ) p->in.name = 0;
		else if ( p->tn.rval == HAVENAME ) break;
		else if( p->nn.sym != NULL ) /* copy name from exname */
			p->in.name = addtreeasciz(exname(p->nn.sym->sname));
		else 
			{
			(void)sprntf( buf, LABFMT, -p->tn.rval );
			p->in.name = addtreeasciz(buf);
			}
		break;

	case REG:
		rbusy( p->tn.rval, p->in.type );
	default:
		p->in.name = 0;
		}

	p->in.rall = NOPREF;

	ty = (short)optype(p->in.op);
	if( ty != LTYPE ) p2tree( p->in.left );
	if( ty == BITYPE ) p2tree( p->in.right );
	/* IN ANSI MODE, convert types into a form the backend understands.
	 * in NON-ANSI MODE this is already done in tymerge */
#ifdef ANSI
	p->in.type = ctype(p->in.type);
#endif 
	}

#               endif /* BRIDGE */
#	 endif	/* C1_C */
# endif		/* STDPRTREE */
# endif         /* not IRIF */

#ifndef BRIDGE
/*
 * pattrib - propagate type attributes, also check for violations of
 *           the const attribute.
 */

NODE *pattrib(p) NODE *p; {
	switch(p->in.op) {
		case MINUS:
		case PLUS:
			if (ISPTR(p->in.right->in.type) &&
			    !ISPTR(p->in.left->in.type))
			{
				p->in.tattrib =
                                        p->in.right->in.tattrib & (~ATTR_CLASS);
			}
			else if (ISPTR(p->in.left->in.type) &&
				 !ISPTR(p->in.right->in.type))
			{
				p->in.tattrib =
                                        p->in.left->in.tattrib & (~ATTR_CLASS);
			}
			break;
		case ASSIGN:
		case ASG LS:
		case ASG RS:
		case ASG MUL:
		case ASG DIV:
		case ASG MOD:
		case ASG AND:
		case ASG OR:
		case ASG ER:
		case ASG PLUS:
		case ASG MINUS:
			if (p->in.left->in.type & ~BTMASK) {
				if (ISPTR(p->in.left->in.type) &&
			            ISPCON(p->in.left->in.tattrib) &&
				    !typ_initialize)
					UERROR(MESSAGE(131));
			}
			else if (ISCON(p->in.left->in.tattrib) &&
				 !typ_initialize)
				UERROR(MESSAGE(131));
			else if (HAS_CON( BTYPE(p->in.left->in.type), p->in.left->in.tattrib) && !typ_initialize)
			     /* "structure/union has const members & 
			      *  cannot be assigned to" */
			     UERROR( MESSAGE( 40 ));
			/* fall through for next check too */
#ifdef ANSI
		case RETURN:
		case ARGASG:
			     if( !CheckQualInclusion(p->in.left, p->in.right) )
		                  /* "qualifiers are not assignment-compatible */
				  WERROR( MESSAGE( 145 ));
#endif /*ANSI*/
			break;
		case INCR:
		case DECR:
			if (p->in.left->in.type & ~BTMASK) {
				if (ISPTR(p->in.left->in.type) &&
			            ISPCON(p->in.left->in.tattrib))
					UERROR(MESSAGE(131));
			}
			else if (ISCON(p->in.left->in.tattrib))
				UERROR(MESSAGE(131));
                        p->in.tattrib = p->in.left->in.tattrib & (~ATTR_CLASS);
			break;
                case UNARY MUL:
                        p->in.tattrib =
                                DECREF(p->in.left->in.tattrib & (~ATTR_CLASS));
                        break;
                case COMOP:  /* SWFfc00992 - Propogate right side attrib */
                        p->in.tattrib = p->in.right->in.tattrib & (~ATTR_CLASS);
                        break;

		default: break;
	}
	return(p);
      }

#ifdef ANSI
/* Following routines are new, added to support ANSI. */

LOCAL CheckQualInclusion( left, right ) NODE *left, *right; {
			      
		/* ensure qualifiers of type pointed to by left 
		   are superset of those pointed to by right */

  int L = 1, R = 1;
  int whereL, whereR;
					
  if( ISPTR_PAF( left->in.type ) || ISPTR_PAF( right->in.type ) ) {
		    
                    whereL = DECREF( left->in.tattrib );
		    whereR = DECREF( right->in.tattrib );

		    L =   ISPVOL( whereL ) | ISPCON( whereL );
		    R =   ISPVOL( whereR ) | ISPCON( whereR );
		  }
  else if( ISPTR( left->in.type ) || ISPTR( right->in.type ) )  {

                    whereL = left->in.tattrib;
		    whereR = right->in.tattrib;

		    L =   ISVOL( whereL ) | ISCON( whereL );
		    R =   ISVOL( whereR ) | ISCON( whereR );
		  }
  return( L == (L | R) );
}

#ifndef IRIF

LOCAL NODE *
strarg(p)
  register NODE *p;
{
  register TWORD ty;

  ty = p->in.type;
  if( ty== STRTY || ty== UNIONTY ){
	TWORD tattrib = p->in.tattrib;
	p = block( STARG, p, NIL, ty, p->fn.cdim, p->fn.csiz );
	p->in.tattrib = tattrib;
	p->in.left = buildtree( UNARY AND, p->in.left, NIL );
	p = clocal(p);
	}

  return( p );

}
#endif /* not IRIF */

/***********************************************************************
 * The following are routines to support trees for CALLs, and were
 * added for function prototype support.
 */


/***********************************************************************
 * next_param :
 * Search for the next parameter id, starting at s, and following
 * the slev_link chain.
 * Used by various routines that need to step down the symbol list of
 * a function prototype parameter list.
 */

SYMLINK
next_param(s)
  SYMLINK s;
{
  while ( (s != NULL_SYMLINK) && !(s->sflags & (SFARGID|SPARAM)) ) 
	s = s->slev_link;
  return(s);
}


/*********************************************************************
 * fname :
 * Try to find the function name for a CALL node.   
 * Used when generating error messages in "check_and_convert_args.
 * If can't find name (for example, it's using an expression for a  pointer
 * to a  function, return 'callstring' as a generic that can be used in error
 * message.
 */
char * callstring = "(function call)";

char *
fname(p)
  NODE * p;
{ if ( callop(p->in.op) )
	if (p->in.left->in.op == NAME )
	    return(p->in.left->nn.sym->sname);
	else if (p->in.left->in.op == UNARY AND  && p->in.left->in.left->in.op == NAME ) 
  	    return( p->in.left->in.left->nn.sym->sname );

  /* generic default */
  return( callstring );
}

/*********************************************************************
 * argname:
 * Construct an argument name for each parameter.
 * If a name cannot be found, use "#%d".  The "#%d" string is
 * printed in static storage.
 */

static char argname[8];
char *
aname(s,n)
  struct symtab *s; int n;
{
    if (s->sname) return s->sname;
    else { (void)sprntf(argname,"#%d",n); return argname; }
}


/*********************************************************************
 * check_and_convert_args :
 *   Do the type checking and conversion of arguments of a CALL as
 *   required by the function prototype.
 */

LOCAL
check_and_convert_args(p, narg)
  NODE *p;	/* points to the CALL node */
  int narg;	/* number of arguments in the CALL */
{ NODE * ph;	/* PHEAD NODE */
  int nparam;	/* number of parameters in the prototype */
  int isold;	/* declaration style */
  int isnew;	/* declaration style */
  int isellipsis;	/* does the declaration have an ELLIPSIS */


# ifdef DEBUGGING
  if (p->in.op != CALL && p->in.op != UNARY CALL)
	cerror("bad node type in check_and_convert_args");
# endif

  /* get the PHEAD, find the style and number of parameters */
  ph = (NODE *) dimtab[p->fn.cdim-1];

  if (ph == NIL)
	cerror("bad PHEAD in check_and_convert_args");

# ifdef DEBUGGING
  if (bdebug) {
	(void)prntf("start CALL arg check:\n");
	fwalk( p, eprint, 0 );
	(void)prntf("cdim=%d ph=%x \n", p->fn.cdim-1, ph);
	}
# endif

  isold = (ph->ph.flags&SFARGID);
  isnew = (ph->ph.flags&SPARAM);
  isellipsis = (ph->ph.flags&SELLIPSIS);
  nparam = ph->ph.nparam;

  if ( !(isold || isnew) ) {
	/* it was an implicit declaration by using a function before any
	 * declaration or definition. 
	 */
	isold = 1;
	/* ?? what else */
	}

  /* UNARY CALL */
  if (narg==0) {
	if (isnew && nparam != 0) {
	   /* "too few parameters for CALL" */
	UERROR( MESSAGE( 198 ), fname(p) );
	   }
	else if (isold && nparam != 0) {
	   /*"number of arguments in CALL does not agree with function definition";*/
#ifdef LINT_TRY
	   WERROR( MESSAGE( 241 ) );
#endif
	   }
	return;
	}

  /* CALL */

  /* compare argument counts */
  if (isnew) {
	if ( (narg < nparam) )
	   /* "too few arguments for CALL" */
	   UERROR( MESSAGE( 198 ), fname(p) );
	else if ( ((narg > nparam) && !isellipsis) )
	   /* "too many arguments for CALL" */
	   UERROR( MESSAGE( 195 ), fname(p) );
	   }

  else  {
	/* ??? do we want to give a warning here or just ignore ??? */
	/*if ( (narg != nparam) && (ph->ph.flags&SFDEF) )
	/* werror("number of arguments in CALL does not agree with function definition");
	*/
	}

  /* Step through each argument: 
   *  ** checkvoidparam() : this catches arguments that are in the form of a
   *       call to a void function.  This is an old error check of the
   *	   pre-ANSI compiler that has been retained.
   *
   *  ** Do checks against corresponding parameter if there is a
   *	   prototype to make sure the "as-if" assignment is O.K.
   *       ?? What are the appropriate checks for ELLIPSIS args ???
   *       ?? Do we want to do checks against old-style paramters and give
   *       warnings even if there was no prototype???
   *
   *  ** Set up for the correct conversions:
   *	   - structure args require special hooks, same for old or new
   *	   - for (new and !ellipsis):
   *		FLOAT stay FLOAT
   *		CHAR and SHORT -> INT because we are still going to pass
   *			32 bits to maintain compatibility with non-ANSI,
   *			and old .o's.
   *		LONG DOUBLE -> ???
   *
   *	   - for (old or ellipsis):
   *		FLOAT -> DOUBLE
   *		CHAR and SHORT -> INT 
   *		LONG DOUBLE -> ???
   *
   * Special considerations as stepping through the lists:
   *	* Because of errors, the lists may not match up in numbers
   *	* The arguments and parameters will be accessed backwards; the
   *	  parameters are linked linearly as symtab structures, linked
   *	  via the slev_link field.  The arguments are in a tree structure
   *	  with intermediate CM ops.  There could be extra entries on the
   *	  paramter list, and these must be skipped by checking the sclass.
   *	* Any ellipsis arguments come first.  The number of these should
   *	  be (narg - nparam)
   */
   { register SYMLINK sparam;	/* step through the prototype list */
     register NODE * q;		/* move through the arg tree */
     register NODE  ** q1;	/* move through the arg tree; trails q.  It
				 * always points to the field that contained
				 * the pointer to q */
     short inellipsis;		/* a counter for # of ellipsis arguments */
     register TWORD ty;		/* type of the argument */
     int argnumber = nparam;	/* used for error messages */

     inellipsis = (isnew && isellipsis) ? (narg - nparam) : 0;
     sparam = isnew ? next_param(ph->ph.phead) : NULL_SYMLINK;
     q = p->in.right;
     q1 = &(p->in.right);

     /* set the global name for use in error messages */
     arg_fname = fname(p);

     while (q) {
	if (q->in.op == CM) {
	   /* readjust q1, since the argument is really q->in.right */
	   q1 = &(q->in.right);
	   }
	checkvoidparam(*q1);
	ty = (*q1)->in.type;

	if ( isnew && !(inellipsis > 0) ) {
	   /* find the next prototype parameter, and do the "as if by
	    * assignment" fixups.  The necessary conversions are
	    * generated by a buildtree ARGASG call, which does type
	    * checking and forces the conversions, and then throws away
	    * the dummy ARGASG node.
       	    */
	   sparam = next_param(sparam);
	   /* if sparam is NULL_SYMLINK there is a mismatch error, but
	    * this has already been detected. Just break out of the
	    * loop.
	    */
	   if (sparam == NULL_SYMLINK) {
		/*uerror("more arguments than parameter types");*/
		break;
		}

	   /* set the name for use in error messages */
	   arg_name = aname(sparam,argnumber--);

	   idname = sparam;	/* needed by buildtree NAME */
	   *q1 = buildtree(ARGASG, buildtree(NAME, NIL, NIL), *q1);

	   /* move parameter pointer for next iteration */
	   sparam = sparam->slev_link;
	   }
	else {
	   /* ??? is inellipsis and isold really the same, or are there
	    * differentiating error checks to make ???
	    */
	   /* old style specialities:
	    * FLOAT->DOUBLE
	    * LONG DOUBLE-> ???
	    */
	   if ( inellipsis > 0 ) inellipsis--;
	   if ( ty == FLOAT )
		*q1 = makety(*q1, DOUBLE, 0, DOUBLE, (*q1)->in.tattrib);
#ifndef ANSI
	   else if ( ty == LONGDOUBLE) /* ???? */
		*q1 = makety(*q1, DOUBLE, 0, DOUBLE, (*q1)->in.tattrib);
#endif /* ANSI */
	   }

	/* Now, finally some conversions to do in any case:
	 *   * special handling of structure args
	 *   * CHAR and SHORT really get passed as INT
	 */

	/* Reset 'ty' because the type could have been rewritten by
	 * some of the above conversions.
	 */
	ty = (*q1)->in.type;

#ifdef IRIF
#if !defined(HAIL) && !defined(LINT)
	/* tell HAIL real types for function arguments. */
	if ( ty==CHAR || ty==SCHAR || ty==UCHAR || ty==SHORT || ty==USHORT )
	    *q1 = makety(*q1, INT, 0, INT, (*q1)->in.tattrib);
#endif /* HAIL */
#else /* IRIF */
	if ( ty == STRTY || ty == UNIONTY ) {
	   /* rewrite structured flavored argument.
	    * See routine strargs in non-ANSI compiler.
	    */
	   *q1 = strarg(*q1);
	    }
#ifndef LINT
	/* inhibit promotions to get right types to .ln file */
	else if ( ty==CHAR || ty==SCHAR || ty==UCHAR || ty==SHORT || ty==USHORT )
		  *q1 = makety(*q1, INT, 0, INT, (*q1)->in.tattrib);
#endif /* LINT */
#endif /* IRIF */
	
	if (q->in.op==CM) {
	   q1 = &(q->in.left);
	   q = q->in.left;
	   }
	else 
	   break;	/* termination of loop */
	}
   }

# ifdef DEBUGGING
  if (bdebug) {
	(void)prntf("end CALL arg check:\n");
	fwalk( p, eprint, 0 );
	(void)prntf("cdim=%d ph=%x \n", p->fn.cdim-1, ph);
	}
# endif

}

/***********************************************************************
 * End of routines for CALLs and function prototypes
 **********************************************************************/



# endif /* ANSI */


int iscompat(t1,ta1,s1,d1,t2,ta2,s2,d2,modify) 
     register TWORD t1,t2,ta1,ta2; int s1,s2,d1,d2; {
    /* checks type compatability between type tuples: <t1,ta1,s1,d1> and <t2,ta2,s2,d2>
     */
    int status = 0;

    if (ISARY(t1) && ISARY(t2)) {
	/* The following bits are set for arrays:
	 * 
	 * ICARY      - if array objects are incompatible
         * ICARYSIZE  - if array sizes are different
         */
	status = iscompat((TWORD)DECREF(t1),(TWORD)DECREF(ta1),s1,d1+1,
			   (TWORD)DECREF(t2),(TWORD)DECREF(ta2),s2,d2+1,modify);
	if (status != 0) status |= ICARY;
	if ((dimtab[d1] == 0) || (dimtab[d2] == 0)) {
	    if (modify) {
#ifndef IRIF
		if (!dimtab[d1] && dimtab[d2]) dimtab[d1] = dimtab[d2];
		else if (!dimtab[d2] && dimtab[d1]) dimtab[d2] = dimtab[d1];
#else
		if (!dimtab[d1] && dimtab[d2]) { 
			dimtab[d1] = dimtab[d2];
			ir_complete_array(t1,ta1,s1,d1);
		}
		else if (!dimtab[d2] && dimtab[d1]) {
			dimtab[d2] = dimtab[d1];
			ir_complete_array(t2,ta2,s2,d2);
		}
#endif
	    }
	} else if (dimtab[d1] != dimtab[d2]) status |= ICARYSIZE;
#if 0
/* removed because of warnings about e.g. (x?p->array_of_char:"string") */
    } else if (WAS_ARRAY(ta1) && WAS_ARRAY(ta2)) {
	/* This array section is similar to the one above.  It is necessary
	 * because pconvert may have changed ARRAY types into pointer types.
	 */
	status = iscompat(DECREF(t1),DECREF(ta1)&(~ATTR_WAS_ARRAY),s1,d1,
			   DECREF(t2),DECREF(ta2)&(~ATTR_WAS_ARRAY),s2,d2,modify);
	if (status != 0) status |= ICARY;
	if ((dimtab[d1-1] > 0) && (dimtab[d2-1] > 0) 
	    && (dimtab[d1-1] != dimtab[d2-1])) status |= ICARYSIZE;

#endif
    } else if (ISPTR(t1) && ISPTR(t2)) {
	/* The following bits are set for pointers:
	 *
	 * ICQUAL  - if qualifiers on the pointers are different.
	 * ICPTRQ  - if qualifiers on the items pointed to are different.
	 * ICPTR   - if the objects pointed to are incompatible.
	 */
	status = iscompat((TWORD)DECREF(t1),(TWORD)(DECREF(ta1)&(~ATTR_WAS_ARRAY)),s1,d1,
			   (TWORD)DECREF(t2),(TWORD)(DECREF(ta2)&(~ATTR_WAS_ARRAY)),s2,d2,modify);

	if (status & (~ICQUAL))   status |= ICPTR;
	else if (status & ICQUAL) status |= ICPTRQ;

	if ((ISPVOL(ta1) != ISPVOL(ta2)) ||
	    (ISPCON(ta1) != ISPCON(ta2))) status |= ICQUAL;
	else status &= ~ICQUAL;

    } else if (ISFTN(t1) && ISFTN(t2)) {
	/* The following bits are set for functions:
	 *
	 * ICFTN        functions have incompatible return types 
	 * ICFTNPARG    prototypes disagree on # of args 
	 * ICFTNPELI    prototypes disagree on use of elipses 
	 * ICFTNPPAR    prototypes have incompatible arguments 
	 * ICFTNDARG    prototype and old style decl disagree on # of args
	 * ICFTNDELI    not allowed to use ellipses in combination of old
         *              style definition
	 * ICFTNDPAR    prototype and old style definition have incompatible
	 *              parameters
         * ICFTNXELI    prototype use of ... isn't compatible with empty
	 *              function list
	 * ICFTNXPAR    prototype parameters must be compatible with default
	 *              widening rules.
	 */
#ifndef ANSI
	status = iscompat(DECREF(t1),DECREF(ta1),s1,d1+1,
			   DECREF(t2),DECREF(ta2),s2,d2+1,modify);
	if (status != 0) status |= ICFTN;
#else /* ANSI */
	NODE *ph1 = (NODE *) dimtab[d1];
	NODE *ph2 = (NODE *) dimtab[d2];
	struct symtab *st1,*st2;
	int i;
	status = iscompat((TWORD)DECREF(t1),(TWORD)DECREF(ta1),s1,d1+1,
			   (TWORD)DECREF(t2),(TWORD)DECREF(ta2),s2,d2+1,modify);
	if (status != 0) status |= ICFTN;
#ifdef DEBUGGING
	if ((ph1 == NULL) || (ph2 == NULL))
	    cerror("Function is missing PHEAD node in iscompat");
#endif

	/* Three distinct cases to worry about:
	 *
	 * - two parameter lists
	 * - one parameter list and empty old style braces that aren't
	 *   part of a function call.
	 * - one parameter list and old style function definition.
	 */
	if ((ph1->ph.flags & SPARAM) && (ph2->ph.flags & SPARAM)) {
	    /* Two parameter lists.  Make the following checks:
	     *
	     * 1. agreement on # of parameters and use of ...
	     * 2. each parameter must be compatible
	     *
	     */
	    if (ph1->ph.nparam != ph2->ph.nparam) status |= ICFTNPARG;
	    else if ((ph1->ph.flags&SELLIPSIS) != (ph2->ph.flags&SELLIPSIS))
	      status |= ICFTNPELI;
	    else {
		st1 = ph1->ph.phead;
		st2 = ph2->ph.phead;
		for (i=0;i<ph1->ph.nparam;i++) {
		    st1 = next_param(st1);
		    st2 = next_param(st2);
		    if ((st1 == NULL) || (st2 == NULL))
		      cerror("incorrect prototype list in iscompat");
		    if (iscompat(st1->stype,(TWORD)DEQUALIFY(st1->stype,st1->sattrib),
				 st1->sizoff,st1->dimoff,
				 st2->stype,(TWORD)DEQUALIFY(st2->stype,st2->sattrib),
				 st2->sizoff,st2->dimoff,
				 modify)) { status |= ICFTNPPAR; }
		    st1 = st1->slev_link;
		    st2 = st2->slev_link;
		}
	    }
	    /* free extra prototype lists (if possible) */
	    if (modify) {
#ifdef HAIL
		hail_share_ftn_type_info(t1,ta1,s1,d1,t2,ta2,s2,d2);
#endif
		if (!(ph1->ph.flags&SFDEF)){
		    if (ph1->ph.phead)
			stab_free_list(ph1->ph.phead);
		    free_proto_header(ph1);
		    dimtab[d1] = dimtab[d2];
		} else if (!(ph2->ph.flags&SFDEF)){
		    if (ph2->ph.phead)
			stab_free_list(ph2->ph.phead);
		    free_proto_header(ph2);
		    dimtab[d2] = dimtab[d1];
		}
	    }
	} else if ((ph1->ph.flags & SPARAM) || (ph2->ph.flags & SPARAM)) {
	    /* 
	     * One parameter list, one old style list
	     */
	    int protofirst = !(ph2->ph.flags&SPARAM); /* is first the proto? */
	    NODE *proto = protofirst?ph1:ph2;
	    NODE *oldstyle = protofirst?ph2:ph1;
	    if (oldstyle->ph.flags & SFDEF) {
		/* A prototype and an old style function definition.  Check:
		 *
		 * 1. Agreement on number of arguments
		 * 2. No ellipses may be used
		 * 3. Each prototype parameter must be compatible with default
		 *    rules applied to each arg.
		 */

		if (proto->ph.nparam != oldstyle->ph.nparam)
		  status |= ICFTNDARG;
		else if (proto->ph.flags & SELLIPSIS)
		  status |= ICFTNDELI;
		else {
		    /* check argument lists */
		    st1 = proto->ph.phead;
		    st2 = oldstyle->ph.phead;
		    for (i=0;i<ph1->ph.nparam;i++) {
			st1 = next_param(st1);
			st2 = next_param(st2);
			if ((st1 == NULL) || (st2 == NULL))
			  cerror("incorrect prototype list in iscompat");
			/* 0 means: 
			 *           don't merge old style entries with new
                         *           style. */
			if (iscompat(st1->stype,(TWORD)DEQUALIFY(st1->stype,st1->sattrib),
				     st1->sizoff,st1->dimoff,
				     default_type(st2->stype),(TWORD)DEQUALIFY(st2->stype,st2->sattrib),
				     st2->sizoff,st2->dimoff,0))
			  { status |= ICFTNDPAR; }
			st1 = st1->slev_link;
			st2 = st2->slev_link;			
		    }
		}
		/* free extra prototype lists (if possible) */
		if (modify) {
#ifdef HAIL
		    hail_share_ftn_type_info(t1,ta1,s1,d1,t2,ta2,s2,d2);
#endif
		    if (ph1->ph.flags&SPARAM){
			/* parameter list: prototype parameter list */
			ph2->ph.phead = ph1->ph.phead;
			/* phead: SFARGID|SPARAM */
			ph2->ph.flags |= SPARAM;
			free_proto_header(ph1);
			dimtab[d1] = dimtab[d2];
		    } else {
			/* SPARAM case wins */
			if (ph1->ph.phead)
			    stab_free_list(ph1->ph.phead);
			free_proto_header(ph1);
			dimtab[d1] = dimtab[d2];
		    }
		}
	    } else {
		/* A prototype and old style declarator id list (not defn).
		 *
		 * 1. No elipses in prototype.
		 * 2. Each prototype parameter compatible with default rules.
		 */
		if (proto->ph.flags & SELLIPSIS)
		  status |= ICFTNXELI;
		else {
		    /* check argument lists */
		    st1 = proto->ph.phead;
		    for (i=0;i<proto->ph.nparam;i++) {
			st1 = next_param(st1);
			if (iscompat(st1->stype,(TWORD)DEQUALIFY(st1->stype,st1->sattrib),
				      st1->sizoff,st1->dimoff,
				      default_type(st1->stype),(TWORD)DEQUALIFY(st1->stype,st1->sattrib),
				      st1->sizoff,st1->dimoff,0))
			  { status |= ICFTNXPAR; }
			st1 = st1->slev_link;
		    }
		    /* share information */
		    if (modify) {
#ifdef HAIL
			hail_share_ftn_type_info(t1,ta1,s1,d1,t2,ta2,s2,d2);
#endif
			if (ph1 == proto)
			    dimtab[d2] = dimtab[d1];
			else
			    dimtab[d1] = dimtab[d2];
		    }
		}
	    }
	} else {
	    /* two old style declarations */
	    if (modify) {
#ifdef HAIL
		hail_share_ftn_type_info(t1,ta1,s1,d1,t2,ta2,s2,d2);
#endif
		if (!(ph1->ph.flags&SFDEF)){
		    free_proto_header(ph1);
		    dimtab[d1] = dimtab[d2];
		} else if (!(ph2->ph.flags&SFDEF)) {
		    free_proto_header(ph2);
		    dimtab[d2] = dimtab[d1];
		}
	    }
	}
#endif /*ANSI*/
    } else { 
	/* The following bits are set for basic types:
	 *
	 * ICTYPE  - if the types are different
	 * ICSTR   - if the structure/union types don't match
	 * ICQUAL  - if the qualifiers don't match
	 */

	if (t1 != t2) status |= ICTYPE;
	else {
	    if ((ISCON(ta1) != ISCON(ta2))||(ISVOL(ta1) != ISVOL(ta2)))
		status |= ICQUAL;
#ifdef ANSI
    	    if (ISENUM(t1,ta1) && ISENUM(t2,ta2) && (s1 != s2))
		status |= ICTYPE;
#endif
	    /* structures and unions */
	    if ((BTYPE(t1)==STRTY||BTYPE(t1)==UNIONTY)&&(s1 != s2))
		{
		    status |= ICSTR;
		}
	}
    }
    return(status);
}

#ifdef ANSI
/*
 * returns the type that results from default promotions applied to type t
 */

TWORD default_type(t) register TWORD t;
{
    switch(t) {
      case FLOAT: return DOUBLE;
      case CHAR:  return INT;
      case UCHAR: return INT;
      case SCHAR: return INT;
      case USHORT: return INT;
      case SHORT: return INT;
      default: return t;
    }
}
#endif /* ANSI */


isitfloat( s , type ) char *s; TWORD type; {

#ifdef QUADC
	QUAD _atold();
#endif
	double atof();

#ifdef ANSI
	errno = 0;
#endif /*ANSI*/

#ifdef ANSI
# ifdef QUADC
	if ((type == FLOAT) || (type == DOUBLE)) {
		qcon.d[0] = atof(s);
		qcon.d[1] = 0;
		dcon = qcon.d[0];
		}
	else {
		qcon = _atold(s);
		dcon = 1.0;	/* just to force errno message */
		}
# else /* QUADC */
	dcon = atof(s);
	qcon.d[0] = atof(s);
	qcon.d[1] = 0;
# endif /* QUADC */
#else /* ANSI */
	dcon = atof(s);
#endif /* ANSI */

#ifdef ANSI
	if( errno == ERANGE )
	     /* "floating point constant %s too %s for represention;\n\t\tinformation will be lost" */
	     WERROR( MESSAGE( 140 ), s, ( dcon == 0 ) ? "small" : "large" );
#endif /*ANSI*/
	return( FCON );
	}

/*ARGSUSED*/
void
bad_fp(sig, code, scp)
  int sig, code;
  struct sigcontext *scp;
{
#ifndef ANSI
	if (constant_folding) ;
	else if( fcon_to_icon )
	     /* "error in floating point constant to %sinteger constant conversion" */
	     UERROR( MESSAGE( 17 ), ( fcon_to_icon == 1 ) ?
		                       "unsigned " : "" );
	else 
	     /* "error in floating point constant" */
	     UERROR( MESSAGE( 221 ));
#else /*ANSI*/
        if( double_to_float )
	     /* "double to float conversion exception" */
	     UERROR( MESSAGE( 176 ) );
	else if (constant_folding)
	     /* ignore errors during constant folding, they've been emitted */
	     ;
	else if( fcon_to_icon )
	     /* "error in floating point constant to %sinteger constant conversion" */
	     UERROR( MESSAGE( 17 ), ( fcon_to_icon == 1 ) ?
		                       "unsigned " : "" );
	else
	     /* "error in floating point constant" */
	     UERROR( MESSAGE( 221 ));
#endif /*ANSI*/

#if defined(xcomp300_800) && !defined(DOMAIN)
	/* Special hooks are needed to restart from a trap on the s800.
	 * This code was taken from the FP exception handler in xdb.
	 */
	scp->sc_pcoq_head += 4;
	scp->sc_pcoq_tail += 4;
	scp->sc_sl.sl_ss.ss_frstat &= 0xffffffbf;
#endif

	(void)signal(SIGFPE, (void *)bad_fp);		/* to reset */
}

#ifdef SA

/* sawalk is originally given trees where assignments
   occur (e.g. lhs).  The basic idea is to find the
   name whose type matches the type in the top node
   of the tree.  In the simple cases this trival,
   arrays, structs and fields add more complexity.
      
   Handling arrays requires that the sa_equal_types
   routine considers an array of some type X equal 
   to just the plain type X.  
   
   Structs and fields add the possibility of more
   than one name being modified at a time.  For
   instance, in "a.b.c=20", a, b and c are all
   modified.  The parameter match controls whether
   a modified name has been found and the search
   is now for any "parent" structs or fields that
   have been modified.  In this situation the types
   of names that were modified must be structs or
   unions with no pointer modifications.  
   
   The order that names are seen (when traversing
   the trees) has a lot to do with struct and 
   field cases working correctly.  The rightmost
   names are seen first.
   
   A better approach might be to follow the types
   of each node to the proper names.  sawalk_params
   works in this way by necessity, but sawalk was
   implemented first and it seems to work.  
      
   */
   
sawalk( t, result_type, match) register NODE *t; {
   int match_here = FALSE, 
       no_name = FALSE;
   TWORD btype;

   /* Basic idea is to continue when !match or if
      there is a match and it happened here or
      if there has been a match but there wasn't 
      a name to check here (an example of this
      last situation is an array offset) */
   
more:	if( t == NULL ) return;
#ifdef SADEBUG
if (ddebug)
        printf("sawalk: %s\n", opst[t->in.op]);
#endif
	switch( optype( t->in.op ) ){
	   case BITYPE:
	      /* Avoid calls, just end up marking
		 parameters and they can't be modified
		 at from this point. */
	      if (t->in.op == CALL)
		 break;
	      
#ifndef IRIF
              /* Look to see if this was for a field. 
		 (ICON's were set in stref().)
		 However, need to continue past 
		 ICONS for array offsets. */
	      if (t->in.right->tn.op == ICON &&
		  /*  t->in.right->tn.rval == NONAME &&
		     It is true that "field" ICONS have 
		     NONAME set, but to test for this seems
		     unnecessary, as long as nn.sym is
		     always 0 at this point for anything
		     other than a "field" ICON.  */
		  t->in.right->nn.sym)
		 /* This is an ICON for a field. */
#else /* IRIF */
	      if( t->in.right->in.op == NAME )
#endif /* IRIF */		   
		   {
		 if (sa_equal_types(result_type,
				    t->in.right->nn.sym->stype,
				    match)) {
		    add_xt_info(t->in.right->nn.sym, 
				MODIFICATION);
		    match = match_here = TRUE;
		 }
	      }
	      else
#ifdef IRIF
		   if( t->in.op == INDEX )
#endif /*IRIF */
		 /* This is an array offset. */
		 no_name = TRUE;
	      
	      /* If haven't matched, or matched here or 
		 wasn't a name to check, then 
		 continue because haven't reached deadend yet. */
	      if (!match || match_here || no_name) {
		 /* Don't go down if the type is an int, this can
		    never lead to a name representing storage
		    that was modified; avoids marking b in
		    *(b + &global) = 20; */
		 if (t->in.left->in.type!=INT)
		    sawalk( t->in.left, result_type, match);
		 /* Not sure if you can possibly go down both sides
		    and find multiple names that were modified */
		 if (t->in.right->in.type!=INT)
		    sawalk( t->in.right, result_type, match);
	      }
	      break;
	   case UTYPE:
	      t = t->in.left;
	      goto more;
	   case LTYPE:
	      if ((t->in.op == REG || t->in.op == NAME) && 
		  t->nn.sym &&
		  sa_equal_types(result_type, 
				 t->nn.sym->stype,
				 match))
		 add_xt_info(t->nn.sym, MODIFICATION);
	      
	}
}

sa_equal_types(result_type, testing_type, have_matched_before) 
   TWORD result_type, testing_type;
{
   TWORD btype, tmptype;
   
   /* If have_matched_before then the type must be a 
      struct or union without any ptr modifiers
      for the types to be equivalent.   */
   if (have_matched_before) {
      if ((btype=BTYPE(testing_type))==STRTY ||
	  btype==UNIONTY) {
	 tmptype = testing_type;
	 /* while have modifiers, make sure none
	    are pointers.  Arrays and function
	    modifiers are okay.  ?? Should functions
	    be?? */
	 while ( tmptype > TNULL ) 
	    if (ISPTR(tmptype))
	       return(FALSE);
	    else
	       tmptype = DECREF(tmptype);
	 return(TRUE);
      }
   }
   else {
	 /* If !have_matched_before then either
	    must have a direct match or be
	    an array whose type directly matches. */
	 if (result_type == testing_type)
	    return(TRUE);
	 /* while testing_type has a modifier and
	          it is an array */
	 while (testing_type > TNULL &&
		ISARY(testing_type)) {
	    testing_type=DECREF(testing_type);
	    if (result_type == testing_type)
	       return(TRUE);
	 }
	 return(FALSE);
      }
}

sawalk_params( t ) register NODE *t; {
   if (ISPTR(t->in.type) ||
       ISARY(t->in.type))
      sawalk(t, DECREF(t->in.type), FALSE);
}
#endif /* SA */
#endif /*BRIDGE*/
