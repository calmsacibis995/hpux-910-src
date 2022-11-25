/* SCCS local.c    REV(64.1);       DATE(92/04/03        14:22:06) */
/* KLEENIX_ID @(#)local.c	64.1 91/08/28 */
/* file local.c */

#  ifndef _SIZE_T
#    define _SIZE_T
     typedef unsigned int size_t;
#endif /* not _SIZE_T */

# include <malloc.h>
# include <string.h>
# include "mfile1"
# include "messages.h"
#ifndef ONEPASS
# include "opcodes.h"
extern char buff[];
#endif  /* ONEPASS */

#ifndef BRIDGE

# ifdef QUADC
        extern QUAD _U_Qfcnvxf_sgl_to_quad();
        extern QUAD _U_Qfcnvxf_usgl_to_quad();
        extern QUAD _U_Qfcnvff_dbl_to_quad();
	extern double _U_Qfcnvff_quad_to_dbl();
	extern int _U_Qfcnvfxt_quad_to_sgl();
        extern QUAD _U_Qfadd();
        extern QUAD _U_Qfmpy();
        extern QUAD _U_Qfdiv();
        extern QUAD _U_Qfsub();
        extern QUAD _U_Qfcmp();
# endif /* QUADC */

/*	this file contains code which is dependent on the target machine */




/* fixfltcmp() checks for appropriate cases to replace a simple test for
   existence on a floating pt node into a comparison on 0.0 so that a
   comprehensive check is performed by fcmp() or fcmpf() as appropriate.
   The whole point of this is that 0x80000000 is a legitimate -0.0
   according to IEEE so a simple test for non-zero does not guarantee
   a correct test for existence.
*/
LOCAL NODE *fixfltcmp(p)	register NODE *p;
{
	NODE *r;

# ifdef ANSI
	if ( (flibflag || p->in.type == LONGDOUBLE) &&
		!logop(p->in.op) && ISFTP(p->in.type) )
# else
	if ( flibflag && !logop(p->in.op) && ISFTP(p->in.type) )
# endif /* ANSI */
		{
		r = buildtree(FCON, NIL, NIL);
		r->fpn.dval = 0;
		return( buildtree(NE, p, r) );
		}
	return (p);
}


/* quickfltconvert() is a helper function to clocal() that modifies the type
   of certain operator nodes directly rather than using a tymatch() call.
   It is used to make floating pt compares and conditional ops into their true
   types when using the 68881.
*/
LOCAL quickfltconvert(p)	register NODE *p;
{
	p->in.type = p->in.left->in.type;
	p->fn.csiz = p->in.left->fn.csiz;
}






NODE *
clocal(p) register NODE *p; {

	/* this is called to do local transformations on
	   an expression tree preparitory to its being
	   written out in intermediate code.
	*/

	/* the major essential job is rewriting the
	   automatic variables and arguments in terms of
	   REG and OREG nodes */
	/* conversion ops which are not necessary are also clobbered here */
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register struct symtab *q;
	register NODE *r, *pleft;
	short o;
	register m, ml;
	int classflag;
#ifndef ONEPASS
	struct symtab *tag;
#endif  /* ONEPASS */

	switch( o = p->in.op ){

	case NAME:
		/* for NAME nodes, sym has the pointer into the stab */
		if( p->nn.sym == NULL || p->tn.rval==NONAME ) { /* already processed; ignore... */
			return(p);
			}
#ifndef ONEPASS
		tag = p->in.c1tag;
#endif  /* ONEPASS */
		q = p->nn.sym;
		switch( q->sclass ){
#ifndef IRIF
		case AUTO:
		case PARAM:
			/* fake up a structure reference */
			r = block( REG, NIL, NIL, PTR+STRTY, 0, 0 );
#ifdef SA
			/* need to save the symtab pointer */
			r->nn.sym = q;
#endif			
			r->tn.lval = 0;
			r->tn.rval = (q->sclass==AUTO?STKREG:ARGREG);
			p = block(STREF,r,p,0,0,0);
			p->in.tattrib = q->sattrib;
			classflag = p->in.tattrib & ATTR_CLASS;
#ifndef ONEPASS
			p->in.c1tag = tag;
#endif  /* ONEPASS */
			p = stref( p, 0 );
			p->in.tattrib |= classflag;
			break;
#endif /* IRIF not defined */

		case STATIC:
		case ULABEL:
		case CLABEL:
			if( q->slevel == 0 ) break;
#ifndef IRIF
			p->nn.sym = NULL;
#endif /* IRIF */
			p->tn.rval = -q->offset;
			/* this is where rval is made neg to show its been processed. */
			break;
#ifndef IRIF
		case REGISTER:
			p->in.op = REG;
			p->tn.lval = 0;
			p->tn.rval = q->offset;
			break;
#endif /* not IRIF */
			}
		break;

	case LT:
	case LE:
	case GT:
	case GE:
		if( ISPTR( p->in.left->in.type ) || ISPTR( p->in.right->in.type ) )
			p->in.op += (ULT-LT);
		/* fall thru */

	case EQ:
	case NE:
		/* if not a scalar type, convert */
		m = p->in.type;
		if ( !(m >= CHAR && m <= LONG) && !(m >= UCHAR && m <= ULONG) && (m != SCHAR) )
			p = makety(p, INT, 0, INT, 0);
		break;

	case ANDAND:
	case OROR:
		/* nothing changes except possibly for flt pt types. */
		p->in.left = fixfltcmp(p->in.left);
		p->in.right = fixfltcmp(p->in.right);
		break;

	case QUEST:
		/* nothing changes except possibly for flt pt types. */
		p->in.left = fixfltcmp(p->in.left);
		break;

#ifndef IRIF
	case CBRANCH:
		if ( !flibflag && ISFTP(p->in.left->in.type) )
			quickfltconvert(p);
	/* must be careful not to throw away necessary SCONV nodes. */
	/* i.e. compiling the following pgm shows that the SCONV is needed:
	main()
	{
	register short x;
	if ((char) x);
	}
	*/
		/* note that *not all unnecessary* SCONV nodes are detected,
		 * for example
		 *
		 *       if( !exp ) ...
		 *
		 * where NOT may hide an SCONV
		 */
		if ( ((pleft=p->in.left)->in.op == SCONV ) &&
			tsize(pleft->in.type,pleft->fn.cdim, 
			        pleft->fn.csiz, NULL, 0) == /* SWFfc00726 fix */
			tsize(pleft->in.left->in.type, pleft->in.left->fn.cdim,
				pleft->in.left->fn.csiz, NULL, 0) ) /* SWFfc00726 fix */

			{
			/* only clobber SCONVS of the same size */
			/* The conversion isn't necessary since all we need is
			   to have the condition codes set. The original op can
			   do that. Thus we can clobber the SCONV here.
			*/
			pleft->in.op = FREE;
			p->in.left = pleft->in.left;
			}

		p->in.left = fixfltcmp(p->in.left);
		/* nothing changes except possibly for flt pt types. */
		break;
#endif /* not IRIF */

	case PCONV:
		/* do pointer conversions for char and shorts */
		pleft = p->in.left;
		ml = pleft->in.type;
		if( !ISPTR(ml) && pleft->in.op != ICON )
			{
		  	p->in.op = SCONV;
		  	break;
			}
#ifdef IRIF
# ifdef HAIL
		/* For HAIL, always leave the PCONV ops in.
		 */
		     break;
# else
		/* for IRIF, only ICON, NAME and PCONV nodes can be 
		 * rewritten now.  The rest keep their PCONV node,
		 * and hence, the node under PCONV keeps its original type
		 */

		if( pleft->in.op != ICON && pleft->in.op != NAME 
		   && pleft->in.op != PCONV )

		     break;
# endif /* HAIL */
#endif /* IRIF */

		/* pointers all have the same representation; 
		 * the type & attributes are inherited
		*/
		pleft->in.type = p->in.type;
		pleft->in.tattrib = p->in.tattrib;
		pleft->fn.cdim = p->fn.cdim;
		pleft->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( pleft );

	case SCONV:
#ifdef IRIF
		if( p->in.type == VOID ) break;
#endif /* IRIF */
		pleft = p->in.left;

		/* now, look for conversions downwards */

		m = p->in.type;
		ml = pleft->in.type;
		if( pleft->in.op == ICON && 
		   (pleft->tn.rval==NONAME || 
#ifdef ANSI
		    ((m==INT)||(m==UNSIGNED)||(m==LONG)||(m==ULONG))
#else
		    ((m==INT)||(m==UNSIGNED))
#endif
		    )){
		  /* simulate the conversion here */
			CONSZ val;
			val = pleft->tn.lval;
			switch( m ){
#ifdef ANSI
			case SCHAR:
#endif
			case CHAR:
				pleft->tn.lval = (char) val;
				break;
			case UCHAR:
				pleft->tn.lval = val & 0XFF;
				break;
			case USHORT:
				pleft->tn.lval = val & 0XFFFFL;
				break;
			case SHORT:
				pleft->tn.lval = (short)val;
				switch (ml)
					{
					case CHAR:	/* i.e. CHAR->SHORT */
						if (val & 0x80L)
							pleft->tn.lval |= 
								0xffffff00;
						break;
					}
				break;
#ifdef ANSI
			case LONG:				
#endif
			case INT:
				switch (ml)
					{
					case CHAR:	/* i.e. CHAR->INT */
						if (val & 0x80L)
							pleft->tn.lval |= 
								0xffffff00;
						break;
					case SHORT:	/* SHORT->INT */
						if (val & 0x8000L)
							pleft->tn.lval |= 
								0xffff0000;
						break;
					}
				break;

			case LONGDOUBLE:
#ifdef QUADC
				if (ISUNSIGNED(pleft->in.type))
					pleft->qfpn.qval = _U_Qfcnvxf_usgl_to_quad((unsigned) val);
				else
					pleft->qfpn.qval = _U_Qfcnvxf_sgl_to_quad(val);
				pleft->qfpn.op = FCON;
				break;
#endif /*QUADC*/
			case FLOAT:
			case DOUBLE:
				if (ISUNSIGNED(pleft->in.type))
					pleft->fpn.dval = (double)(unsigned) val;
				else
					pleft->fpn.dval = (double) val;
				pleft->fpn.op = FCON;
				break;
				}
			pleft->in.type = m;
			pleft->fn.csiz = p->fn.csiz;	/* for enums */
		    }   /* ICON */
		else if( pleft->in.op == FCON ) /* simulate the conversion here */
			{
			if (UNSIGNABLE(m) || ISUNSIGNED(m) || (m == ENUMTY)
			    || (m == MOETY) )
				{ 
				unsigned u;
				/* FCON to ICON convert */
				pleft->tn.op = ICON;
				fcon_to_icon = ISUNSIGNED(m) ? 1 : -1;
# ifdef QUADC
				if (pleft->in.type == LONGDOUBLE)
					pleft->tn.lval = _U_Qfcnvfxt_quad_to_sgl(pleft->qfpn.qval);
				else if (ISUNSIGNED(m))
					pleft->tn.lval = u = pleft->fpn.dval; 
				else
					pleft->tn.lval = pleft->fpn.dval;
# else
				if (ISUNSIGNED(m))
					pleft->tn.lval = u = pleft->fpn.dval;
				else
					pleft->tn.lval = pleft->fpn.dval;
# endif /* QUADC */
				pleft->tn.rval = NONAME;
				/* NONAME is a signal to p2tree to regard the
				   node as a constant, not a name.
				*/
				pleft->tn.type = m;
				fcon_to_icon = 0;
				}
			/* double to float const converts will be done later in
			   fincode().
			*/
			else if (ISFTP(m))
# ifdef QUADC
				if ((m == LONGDOUBLE) && (ml != LONGDOUBLE)) {
					pleft->qfpn.qval = _U_Qfcnvff_dbl_to_quad(pleft->fpn.dval);
					}
				else if ((m != LONGDOUBLE) && (ml == LONGDOUBLE)) {
					pleft->fpn.dval = _U_Qfcnvff_quad_to_dbl(pleft->qfpn.qval);
					}
# endif /* QUADC */
				pleft->tn.type = m;
			/* else do nothing */
			}
		else if (m != 0) break; /* clobber conversion to void */

		/* clobber conversion */
		pleft->in.tattrib = p->in.tattrib;
		p->in.op = FREE;
		return( pleft );  /* conversion gets clobbered */

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion", 0);
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	case COLON:
		/* see if the daughter nodes are constants that can be folded. */
		p->in.left = clocal(p->in.left);
		p->in.right = clocal(p->in.right);
		break;
		}

	return(p);
	}

#ifndef IRIF

cisreg( t ) TWORD t; { /* is an automatic variable of type t OK for a register variable */
#ifndef ONEPASS
    if (optlevel < 2)
	{
#endif  /* ONEPASS */
	switch (t) {
	  case UNIONTY:	return(unionflag);

	  case INT:
	  case UNSIGNED:
	  case SHORT:
	  case USHORT:
	  case CHAR:
#ifdef ANSI
	  case SCHAR:
	  case ULONG:
	  case LONG:
#endif /*ANSI*/
	  case UCHAR:	
	  case ENUMTY:
# if 0	
  		/* removed flt pt types on 9/23/88 to avoid obj. incompat,
		   at least until further notice */
		/* ALSO NOTE, need to make sure routine 'bfcode()', and
		   any equivalent routines also handle register class
		   parameter initializations for these types as well */
	  case FLOAT:
	  case DOUBLE:
#ifdef QUADC
	  case LONGDOUBLE:
#endif /* QUADC */
# endif /* 0 */
			return(1);
	  default:	if ( ISPTR(t) ) return(1);
			return(0);
	}
#ifndef ONEPASS
    }
    else
	/* allow only register parameters (to be denied later in dclargs) */
	return((blevel==1)? 1 : 0);
#endif /* ONEPASS */
}

NODE *
offcon( off
		) OFFSZ off;
				{

	/* return a node, for structure references, which is suitable for
	   being added to a pointer of type t, in order to be off bits offset
	   into a structure */

	register NODE *p;

	/* t, d, and s are the type, dimension offset, and sizeoffset */
	/* in general they  are necessary for offcon, but not on H'well */

	p = bcon(0, INT);
	p->tn.lval = off/SZCHAR;
	return(p);

	}

static inwd		/* current bit offsed in word */;
static long word	/* word being built from fields */;

incode( p, sz ) register NODE *p; register sz; {

	/* generate initialization code for assigning a scalar constant c
	   to a field of width sz.  we assume that the proper alignment has
	   been obtained. inoff is updated to have the proper final value.
	*/

	if((sz+inwd) > SZINT) cerror("incode: field > int");

	if( sz == SZINT ) {  /* used for wide character strings */
#ifdef ONEPASS
	     prntf( "\tlong\t%d\n", p->tn.lval );
#else
	     sprntf( buff, "\tlong\t%d\n", p->tn.lval );
             p2pass(buff);
#endif
	     inoff += sz;
	     return;  }

	word |= (p->tn.lval & ((1 << sz) -1)) << (SZINT - sz - inwd);
	inwd += sz;
	inoff += sz;
	while (inwd >= 16) {
#ifdef ONEPASS						/* to support 2 pass */
          if ((word>>16)&0xFFFFL)
		prntf( "	short\t%d\n", (word>>16)&0xFFFFL );
	  else
		prntf( "	space\t2\n");
#else							/* to support 2 pass */
          if ((word>>16)&0xFFFFL)
		sprntf(buff, "	short\t%d", (word>>16)&0xFFFFL );
	  else
		sprntf(buff, "	space\t2");
	  p2pass(buff);
#endif							/* to support 2 pass */
	  word <<= 16;
	  inwd -= 16;
	}
}

#ifdef QUADC
fincode( q, sz ) QUAD q; short sz;
#else
fincode( d, sz ) double d; short sz;
#endif
	{
	/* output code to initialize space of size sz to the value d */
	/* the proper alignment has been obtained */

	long *mi;
	float dtof();		/* library routine to convert double to float */
	union {
		int	i;
		float	f;
		} coerce;
# ifdef QUADC
	double d = 0;		/* don't print long doubles in # lines */

	if (sz != SZLONGDOUBLE)
		d = q.d[0];

# endif /* QUADC */

	if( sz==SZDOUBLE )
		{
		mi = (long *)&d;
# ifdef ONEPASS
		fprntf( outfile, "	long	0x%x,0x%x\n", mi[0], mi[1] );
# else  /* ONEPASS */
		sprntf( buff, "	long	0x%x,0x%x", mi[0], mi[1] );
		p2pass(buff);
# endif /* ONEPASS */
		}
# ifdef QUADC
	else if (sz == SZLONGDOUBLE)
		{
# ifdef ONEPASS
		fprntf( outfile, "	long	0x%x,0x%x,0x%x,0x%x\n", q.u[0], q.u[1], q.u[2], q.u[3] );
# else
		sprntf( buff, "	long	0x%x,0x%x,0x%x,0x%x", q.u[0], q.u[1], q.u[2], q.u[3]);
		p2pass(buff);
# endif /* ONEPASS */
		}
# endif /* QUADC */

	else
		{
#ifdef ANSI
		double_to_float = 1; /* set in case of error */
#endif /*ANSI*/
		coerce.f = dtof(d);
#ifdef ANSI
		double_to_float = 0;
#endif /*ANSI*/
# ifdef ONEPASS
		fprntf( outfile, "	long	0x%x\n", coerce.i );
# else  /* ONEPASS */
		sprntf( buff, "	long	0x%x", coerce.i );
		p2pass(buff);
# endif /* ONEPASS */
		}
	inoff += sz; 	/* Usually not necessary since global floats now go to
				   outfile.
				*/
# ifdef DEBUGGING
#ifdef QUADC
	if (sz != SZLONGDOUBLE)
#endif /* QUADC */
	    {
# ifdef ONEPASS
		fprintf(outfile, "		# %.17g\n", d);
# else  /* ONEPASS */
		sprintf(buff, "		# %.17g\n", d);
		p2pass(buff);
# endif /* ONEPASS */
	    }
# endif	/* DEBUGGING */
	}

cinit( p, sz ) NODE *p; {
	/* arrange for the initialization of p into a space of
	size sz */
	/* the proper alignment has been opbtained */
	/* inoff is updated to have the proper final value */
# ifdef ONEPASS
	ecode( p );
# else  /* ONEPASS */
	putinit(p, sz);
# endif /* ONEPASS */
	inoff += sz;
	}

vfdzero( n ){ /* define n bits of zeros in a vfd */

	if( n <= 0 ) return;

	inwd += n;
	inoff += n;
	while (inwd >= 16) {
#ifdef ONEPASS						/* to support 2 pass */
          if ((word>>16)&0xFFFFL)
		prntf( "	short\t%d\n", (word>>16)&0xFFFFL );
	  else
		prntf( "	space\t2\n");
#else							/* to support 2 pass */
          if ((word>>16)&0xFFFFL)
		sprntf(buff, "	short\t%d", (word>>16)&0xFFFFL );
	  else
		sprntf(buff, "	space\t2");
	  p2pass(buff);
#endif							/* to support 2 pass */
	  word <<= 16;
	  inwd -= 16;
	}
}


char * exname( p ) register char *p; {
	/* make a name look like an external name in the local machine */

	static char text[NCHNAM+1];

	/* NOTE p==0 occurs after recovering from an error in declaration.
	   Therefore no code will actually be generated in that case and a
	   nil name can be made by exname with no consequences.
	*/


	register char *q;
	if (p)	
		{
		text[0] = '_';
		for( q = text + 1; *p && q < (text + NCHNAM); )
			*q++ = *p++;
		}

	*q = '\0';
#ifdef LIPINSKI
	{
		char *alias,*naliaslookup();
		if ((alias = naliaslookup(text)) == NULL)
			return(text);
		else return(alias);
	}
#endif

	return( text );
	}

#endif /* not IRIF */

#endif /* BRIDGE */

ctype( type ) TWORD type; { /* map types which are not defined on the local machine */
	switch( BTYPE(type) ){
#ifdef ANSI
	case SCHAR:
		MODTYPE(type,CHAR);
		break;
#endif /*ANSI*/
	case LONG:
		MODTYPE(type,INT);
		break;
	case ULONG:
		MODTYPE(type,UNSIGNED);
		break;
#ifndef QUADC
	case LONGDOUBLE:
		MODTYPE(type,DOUBLE);
		break;
#endif /*QUADC*/
		}
	return( type );
	}

#ifndef IRIF

#ifndef BRIDGE

commdec( id ) struct symtab *id; { /* make a common declaration for id, if reasonable */
	register struct symtab *q;
	OFFSZ off;

	q = id;
	off = tsize( q->stype, q->dimoff, q->sizoff, q->sname, 1 ); /* SWFfc00726 fix */

#ifdef ONEPASS						/* to support 2 pass */
# ifdef DBL8
	if (q->stype == DOUBLE
#  ifdef ANSI
	    || q->stype == LONGDOUBLE
#endif /* ANSI */
	   )
		prntf("\tlalign\t8\n");
# endif /* DBL8 */
	prntf( "	comm	%s,", exname( q->sname ) );
	prntf( CONFMT, off/SZCHAR );
	prntf( "\n" );
#ifdef ANSI
	if (off) prntf( "	sglobal	%s\n", exname( q->sname ) );
#endif /* ANSI */
#else							/* to support 2 pass */
# ifdef DBL8
	if (q->stype == DOUBLE
#  ifdef ANSI
	    || q->stype == LONGDOUBLE
#endif /* ANSI */
	   )
		p2pass("\tlalign\t8\n");
# endif /* DBL8 */
	sprntf(buff, "	comm	%s,%d", exname( q->sname ), off/SZCHAR );
	p2pass(buff);
#ifdef ANSI
	if (off) {
	    sprntf(buff, "	sglobal	%s", exname( q->sname ) );
	    p2pass(buff);
	}
#endif
#endif							/* to support 2 pass */
	}

extern short lastloc;

ecode( p ) NODE *p; {

	/* walk the tree and write out the nodes.. */

	if( nerrors ) return;
#ifdef ONEPASS					/* added for 2 pass */
# ifdef HPCDB
	/* Generate a slt-normal, but only if we are generating code for
	 * a statement or initialization of an automatic. Note that 'ecode'
	 * also gets invoked when matching the initialization tree of a global
	 * and here we want no slt.
	 */
	if (cdbflag && (lastloc==PROG)) (void)sltnormal();
# endif
	p2tree( p );
	p2compile( p );
#else						/* added for 2 pass */
#ifndef IRIF
	puttree( p, 0, (p->in.op==STASG) );	/* added for 2 pass */
#endif
	tfree( p );
#endif						/* added for 2 pass */
#ifdef FORT
	treset();
	resettreeasciz();
#endif
        }

#endif /* not IRIF */

#ifdef xcomp300_800

float dtof(d)
double d;
{
  return( (float)d) ;
}

#endif /* xcomp300_800 */

#ifndef IRIF

#define PTRFTNDOUBLE	((FTN<<TSHIFT)+PTR+DOUBLE)
#define PTRFTNFLOAT	((FTN<<TSHIFT)+PTR+FLOAT)

/*
 * Table of builtin definitions...keep sorted.
 */

struct builtin builtins[] = {
{"acos","facos","_acos","__fp040_dacos",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"asin","fasin","_asin","__fp040_dasin",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"atan","fatan","_atan","__fp040_datan",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"cos" ,"fcos" ,"_cos" ,"__fp040_dcos",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"cosh","fcosh","_cosh","__fp040_dcosh",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"exp" ,"fetox","_exp" ,"__fp040_dexp",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"fabs","fabs" ,"_fabs",0,DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"log10","flog10","_log10","__fp040_dlog10",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"sin" ,"fsin" ,"_sin" ,"__fp040_dsin",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"sinh","fsinh","_sinh","__fp040_dsinh",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"sqrt","fsqrt","_sqrt",0,DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"tan" ,"ftan" ,"_tan" ,"__fp040_dtan",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
{"tanh","ftanh","_tanh","__fp040_dtanh",DOUBLE,FLOAT,PTRFTNDOUBLE,PTRFTNFLOAT,TRUE},
};

int builtins_table_size = sizeof(builtins);

/*
 * Comparison routine, to be used in searching
 */

builtin_compare(b1,b2) struct builtin *b1,*b2; {
	return(strcmp(b1->name,b2->name));
}

/*
 * Turn a function into a builtin name 
 */
monadicize(p) NODE *p; {
	struct builtin bi,*bptr;

	/* Check for expected argument count and types */
	if ( (p->in.op != CALL) || (p->in.right->in.op == CM) ||
	     (p->in.type != DOUBLE) || (p->in.right->in.type != DOUBLE) )
		return;
	bi.name = p->in.left->in.left->nn.sym->sname;
	bptr = (struct builtin *) 
	       	bsearch((char *)&bi,(char *)builtins,
		    (unsigned)(builtins_table_size/sizeof(struct builtin)),
			    sizeof(struct builtin),builtin_compare);
	if (bptr == NULL || !(bptr->enabled)) {
		return;
	};
	if (flibflag) 
	{
		p->in.left->in.left->in.name = bptr->nlcd;
		p->in.left->in.left->tn.rval = HAVENAME;
		return;

	}
#ifdef FORTY
	if ((fortyflag) && (bptr->forty != 0))
	{
		p->in.left->in.left->in.name = bptr->forty;
		p->in.left->in.left->tn.rval = HAVENAME;
		return;
	}
#endif /* FORTY */
	p->in.left->in.left->in.op = FREE;
	p->in.left->in.op = FREE;
	p->in.left = bcon(0,(singleflag&&(p->in.right->in.type==FLOAT))?
				bptr->fftyp:bptr->ftyp);
	p->in.left->in.name = bptr->n881;
	p->in.left->tn.rval = HAVENAME;
	p->in.op = FMONADIC;
	p->in.type = (singleflag&&(p->in.right->in.type==FLOAT))?
				bptr->frtyp:bptr->rtyp;
	return;
}


char *builtin_strcpy = "__BUILTIN_strcpy";
/* convert strcpy(dest,src) to _BUILTIN_strcpy(n,dest,src), where n is the
 * length of src (0 == not a string literal).
 */
void
strinline(p) NODE *p;
{
register NODE *src;
NODE *icon, *comma;
int srclen = 0;

	/* Type and argument count correct? */
	if ( (p->in.right->in.op != CM) || 
	     (p->in.right->in.left->in.op == CM) ||
	     /* allow (char *) return type or implicit (int) */
	     (p->in.type != PTR+CHAR && p->in.type != INT) || 
	     (p->in.right->in.left->in.type != PTR+CHAR) ||
	     (p->in.right->in.right->in.type != PTR+CHAR) )
		return;

	/* Is src a string literal? */
	src = p->in.right->in.right;
	if ( (src->in.op==UNARY AND) && (src->in.left->in.op==NAME) &&
	     (src->in.left->in.type==CHAR) && (src->in.left->tn.rval<0) )
		/* the string length was dstash'ed by lxEndStr, then
		 * pconvert incremented cdim as part of its DECREF,
		 * so the length is at cdim-1
		 */
		srclen = dimtab[src->in.left->fn.cdim - 1];

	/* Splice in the new length parameter */
	icon = bcon(srclen,INT);
        comma = block(CM,icon,p->in.right->in.left,INT,0,INT);
        p->in.right->in.left = comma;

        /* Change the function name */
        p->in.left->in.left->in.name = builtin_strcpy;
        p->in.left->in.left->tn.rval = HAVENAME;
}


#ifdef C1_C
/* Emit C1NAME for each routine that could be inlined.
 * Called from putmaxlab() if inlineflag is set.
 */
c1name_builtins()
{
int i;

	for (i=0; i< (sizeof(builtins)/sizeof(struct builtin)); i++)
	    {
		puttyp(C1NAME, 0 /*val*/, FTN+DOUBLE, 0 /*attr*/);
		p2name(builtins[i].n881);
		p2word(C1FUNC);
		p2name(builtins[i].name);
#ifdef FORTY
		if (builtins[i].forty != 0)
			{
			puttyp(C1NAME, 0 /*val*/, FTN+DOUBLE, 0 /*attr*/);
			p2name(builtins[i].forty);
			p2word(C1FUNC);
			p2name(builtins[i].name);
			}
#endif
	    }
	
	puttyp(C1NAME, 0, (PTR<<TSHIFT)+FTN+CHAR, 0);
	p2name("__BUILTIN_strcpy");
	p2word(C1FUNC);
	p2name("strcpy");
}
#endif /* C1_C */

#endif /* not IRIF */

#ifdef LIPINSKI
/*
 * naliastab routines added to support ALIAS pragma
 */
#define NALIASSIZ 41 /* number of buckets */
#define HLENGTH  5   /* number of characters to hash on */
struct nalias *naliastab[NALIASSIZ];

naliasinit() /* Initialize alias table */
{
  int i;
  for (i = 0;i < NALIASSIZ;i++) naliastab[i] = NULL;
}

char *naliaslookup(internal)
char *internal;
/* Returns the external name from the alias table, returns NULL if not found */
{
  int bucket,i;
  char *p;
  struct nalias *nptr;
  
  for (bucket=0,p=internal,i=0;*p != '\0';++p) {
    bucket = (bucket<<1)+*p;
    if (++i >= HLENGTH) break;
  }
  bucket = bucket % NALIASSIZ;
  for (nptr = naliastab[bucket]; nptr != NULL; nptr = nptr->nxt)
    if (strcmp(nptr->internal,internal) == 0)
      return(nptr->external);
  return(NULL);
}

naliasinsert(internal,external)
char *internal,*external;
/* Add a name to the alias table, return value: 0 = duplicate, 1 = ok */
{
  int bucket,i;
  char *p;
  struct nalias *nptr,*new;


  for (bucket = 0, p = internal,i=0;*p != '\0';++p){
    bucket = (bucket<<1)+*p;
    /* Only hash on first characters */
    if (++i >= HLENGTH) break;
  }
  bucket = bucket % NALIASSIZ;
  /* see if name already exists */
  for (nptr = naliastab[bucket];nptr != NULL;nptr=nptr->nxt)
    if (strcmp(nptr->internal,internal) == 0) return(0); /*Duplicate */
  /* name doesn't exist, add to alias table */
  new = (struct nalias *) malloc(sizeof(struct nalias));
  new->internal = (char *) malloc(strlen(internal)+1);
  (void)strcpy(new->internal,internal);
  new->external = (char *) malloc(strlen(external)+1);
  strcpy(new->external,external);
  new->nxt = naliastab[bucket];
  naliastab[bucket] = new;
}
#endif /* LIPINSKI */

#endif /* BRIDGE */
