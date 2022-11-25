/* file pftn.c */
/* DOND_ID @(#)pftn.c           72.3 93/11/10 */
/* SCCS pftn.c    REV(64.7);       DATE(92/04/16        12:15:41) */
/* KLEENIX_ID @(#)pftn.c	64.7 92/03/04 */
# include <string.h>

# include "mfile1"
# include "messages.h"
# include "mac2defs"

#ifdef IRIF
#ifndef HAIL
#      include "feifcodes.h"
#endif /* HAIL */
# define CONSTANT_NODE( p )  ( p->in.op == ICON || \
				   p->in.op == UNARY AND || \
				   p->in.op == FCON || \
				   p->in.op == STRING )
  static NODE *doinit1();
  static NODE *doinit2();
  static irbrace_parent = 0;  /* set if 'irbrace()' calls 'gotscal()' */
  static initializations = 0;
#endif /* IRIF */

# ifdef LINT_TRY
# include "lmanifest"
extern int libflag;
extern int xflag;
extern int stdlibflag;
# endif /* LINT_TRY */

# ifdef SA
# include "sa.h"
# endif

/* dimtab status word bit: */
# define TYPE_CON  01           /*structure/union contains CONST member */
# define TYPE_VOL  02		/*structure/union contains VOL member */

# define HAS_ANY_VOL(p)  ( (p->sattrib&ANY_VOL_MASK) || ( ( (BTYPE(p->stype)==STRTY)||(BTYPE(p->stype)==UNIONTY) ) && (dimtab[p->sizoff+4]&TYPE_VOL) ) )

# define INSTACKSZ 25

# define IN_LEX_CHAIN( sp )     ( sp != sp->slev_link )
# define IS_NEW_ENTRY( sp )     ( !IN_LEX_CHAIN( sp ))

extern int cdbfile_vtindex;
extern char	*addasciz();	/* adds names to asciz table */
void gotscal();

#ifdef C1_C
extern flag volatile_flag;	/* set by +OV */
#endif

#ifdef IRIF
     flag commflag = 0;         /* used in 'feif.c' */
#else /* not IRIF */
     static flag commflag;
     unsigned int offsz;
#endif /* not IRIF */

# ifdef LINT
extern flag no_null_in_init;
# endif /* LINT */

flag unionflag;			/* true iff unions can be in registers */

LOCAL flag must_check_globals = 0; /* true if, e.g.,
				    *     USTATIC class occurs 
				    *     tentative STATIC definition [no initializer]
				    */
LOCAL short tshift;             /* global counter for tyreduce     */

static flag report_no_errors = 0;/* shuts off tsize's error message */

#ifndef IRIF
#ifdef ANSI

/* status of auto aggregate initialization */
int auto_aggregate_label;	/* label # for current aggregate */
int aggregate_expr;		/* used to distinguish between
				 *   struct x = e;  and
				 *   struct x = { e };
				 *   unknown
				 */

#endif
#endif /*not IRIF */

LOCAL struct instk {
	int in_sz;   /* size of array element */
	int in_x;    /* current index for structure member in structure initializations */
	int in_n;    /* number of initializations seen */
	int in_s;    /* sizoff */
	int in_d;    /* dimoff */
	TWORD in_t;    /* type */
	TWORD in_tattrib; /* type attribute */
	struct symtab *in_id;   /* stab pointer */
	flag in_fl;   /* flag which says if this level is controlled by {} */
	OFFSZ in_off;  /* offset of the beginning of this level */
	}
instack[INSTACKSZ],
*pstk;

	/* defines used for getting things off of the initialization stack */


struct symtab *hide();


#ifdef DEBUGGING
flag ddebug = 0;
#endif

LOCAL struct symtab *lookup_struct();
LOCAL struct symtab *add_entry();
LOCAL struct symtab *struct_member();
#ifdef ANSI
     LOCAL struct symtab *block_extern();
     LOCAL struct symtab *promote();
#endif

defid( q, class )  
register NODE *q;
register char class;
{
	register struct symtab *p;
	register char scl;
	register TWORD type;
	TWORD tattrib;
	TWORD stp;
	char slev;
	int status;
#ifndef IRIF
#ifdef C1_C
	int regflag;    /* set if register requested, even if moved to auto */
#endif	/* C1_C */
#endif  /* not IRIF */

	if( q == NIL ) return;  /* an error was detected */

	if( q < node || q >= &node[maxnodesz] ) cerror( "defid call" );

	p = q->nn.sym;

	if( p == NULL ) cerror( "tyreduce" );

# ifdef DEBUGGING
	if( ddebug ){
		printf( "defid( %s (%x), ", p->sname?p->sname:"(SNONAME)", p );
		tprint( q->in.type, q->in.tattrib );
		printf( ", %s, (%x,%x) ), level %x\n", scnames(class), q->fn.cdim, q->fn.csiz, blevel );
		}
# endif

#ifndef IRIF
#ifdef C1_C
	regflag = (class == REGISTER);
#endif  /* C1_C */
#endif /* not IRIF */

	/* force enums out in dntt, there members may be referenced */
#ifdef HPCDB
	if(q->in.type == ENUMTY)
		p->sflags |= SDBGREQD;
#endif
	fixtype( q, class );
	type = q->in.type;
	tattrib = q->in.tattrib;
#ifdef ANSI
	class = fixclass( class, type, tattrib, p );
#else
	class = fixclass( class, type, tattrib );
#endif

#if defined IRIF && defined ANSI && !defined HAIL
	if( class == PARAM && q->in.type == FLOAT && inproto == 0 ) {
	     type = q->in.type = DOUBLE;
	     p->sflags |= SFLTCVT;
	}
#endif /* IRIF && ANSI */

	if ( type == UNIONTY && class == REGISTER
		&& ( dimtab[q->fn.csiz] != SZINT ) )
			class = (blevel == 1||inproto > 0)? PARAM : AUTO;

	/* For ANSI params and autos are at the same level */
	if ( blevel == 2 && p->slevel == 1) {
	     /* "redeclaration of '%s' hides formal parameter" */
	     WERROR( MESSAGE( 222 ), p->sname);
	}
#ifdef ANSI
	if( class == EXTERN && blevel > 1 ) 
	     p = q->nn.sym = block_extern( p );

        /* Clear sflags.SBEXTERN_SCOPE if the block level extern was */
        /* resolved to an file scope.                                */

        else if( class == EXTERN || class == EXTDEF && blevel == 0 )
           p->sflags &= ~SBEXTERN_SCOPE;
#endif /* ANSI */

	stp = p->stype;
	slev = p->slevel;

# ifdef DEBUGGING
	if( ddebug ){
		printf( "	modified to " );
		tprint( type, tattrib );
		printf( ", %s\n", scnames(class) );
		printf( "	previous def'n: " );
		tprint( stp, p->sattrib );
		printf( ", %s, (%x,%x) ), level %x\n", scnames(p->sclass), p->dimoff, p->sizoff, slev );
		}
# endif

	if( stp == FTN && IS_NEW_ENTRY( p ) )goto enter;
		/* name encountered as function, not yet defined */
		/* BUG here!  can't incompatibly declare func. in inner block */
	if( stp == UNDEF|| stp == FARG ){
		if( blevel==1 && stp!=FARG ) switch( class ){

		default:
			/* "declared argument %s is missing" */
			if(!(class&FIELD)) UERROR( MESSAGE( 28 ), p->sname );
		case MOS:
		case STNAME:
		case MOU:
		case UNAME:
		case MOE:
		case ENAME:
		case TYPEDEF:
			;
			}
		goto enter;
		}

	if (class != PARAM) {
		status = iscompat(stp,p->sattrib,p->sizoff,p->dimoff,
				  type,tattrib,q->fn.csiz,q->fn.cdim,
				  (blevel == p->slevel)); /*SR#1653023085*/
                                  /* DTS#CLL4600024, SR#5000677054       */

		if ((class==STNAME)||(class==UNAME)||(class==ENAME)){
		    if ((status != 0) && (status != ICSTR)) goto mismatch;
		} else if (status != 0) goto mismatch;
	} else status = 0;

	scl = ( p->sclass );

# ifdef DEBUGGING
	if( ddebug ){
		printf( "	previous class: %s\n", scnames(scl) );
		}
# endif

	if( class&FIELD ){
	     cerror( "defid: redefinition of field ???" );
	   }
#ifdef ANSI
	else if( p->sflags & SWEXTERN ) {
	     /* 'wild card' EXTERN - matches any storage class */
	     p->sclass = class;
	     p->sflags &= ~SWEXTERN;

	     if( ! ISFTN( type )) return;

	     /* a function definition needs doing */
	     scl = ( class == STATIC ? USTATIC : EXTERN );
	}
#endif
	switch( class ){

	case EXTERN:
		switch( scl ){
		case USTATIC:
		case STATIC:
			if( slev==0 ) {
#ifdef ANSI
			     /* [blevel is zero in ANSI] */

			     if( ISFTN( type )) {
				  curftn = p;
			     }				
			     else {
				  if( commflag ) {
				       /* "linkage conflict with prior declaration for '%s'; 
					  %s linkage assumed" */
				       WERROR( MESSAGE( 109 ), p->sname, "internal" );
				       /* ensure 'comm' psuedo op not generated */
				       commflag = 0;
				  }
			     }
#endif
			     return;
			}
			break;
		case EXTDEF:
# ifdef SA
			/* Have something like "extern int x;" after
			   "int x = 20;". 
			   Functions are handled from cgram.y */
			if (saflag && !ISFTN(type)) {
			   add_xt_info(p, DECLARATION);
			   return;
			}
# endif /* SA */
		case EXTERN:
#ifdef IRIF
			if( !ISFTN( type )) {
			     if( ir_storage_alloc( p ) == FE_ZERO_SIZE ) {
				  /* "unknown size for '%s'" */
				  UERROR( MESSAGE( 183 ), p->sname );
			     }
			}
#endif /* IRIF */
# ifdef SA
			if (saflag && !ISFTN(type)) {
			   /* Functions are handled from cgram.y */

			   if (p->cdb_info.info.extvar_isdefined == 1)
			  /* Have something like "extern int x;"
			   * after "int x;" or "int x;" after
			   * "extern int x;".   In the first case,
			   * "int x; ... extern int x;", 
			   * the extern occurance is being called
			   * a definition when it isn't.
			   * I don't see how to properly handle this
			   * case, because extvar_isdefined
			   * is set for the "int x".  So, this is a known
			   * defect.  */
			      add_xt_info(p, DEFINITION | DECLARATION);
		           else
			  /* Have "extern int x;" after "extern int x;" */
			      add_xt_info(p, DECLARATION);
			}
# endif /* SA */
			return;
			}
		break;

	case STATIC:
#ifdef ANSI
		if( scl==USTATIC ) {
		     p->sclass = STATIC;
# ifdef SA			
		     if (saflag && !ISFTN(type)) {
			  /* Functions are handled from cgram.y */
			  add_xt_info(p, DEFINITION | DECLARATION);
		     }
# endif /* SA */
		     if( ISFTN(type) ) {
			  curftn = p;
		     }
		     return;
		}
		if( blevel == 0 ) {
		     if( ISFTN( type )) {
			  curftn = p;
			  if( scl == EXTERN ) {
			       /* "linkage conflict with prior declaration for '%s'; 
				  %s linkage assumed" */
			       WERROR( MESSAGE( 109 ), p->sname, "external" );
			       p->sclass = EXTDEF; /* needed for external reference */
			       return;
			  }
		     }
		     else {
			  if( scl == EXTDEF || scl == EXTERN ) {
			       /* "linkage conflict with prior declaration for '%s'; 
				  %s linkage assumed" */
			       WERROR( MESSAGE( 109 ), p->sname, "external" );
			       /* ensure 'comm' psuedo op not generated */
			       commflag = 0;
# ifdef SA			
			       if (saflag && !ISFTN(type)) {
				    /* Functions are handled from cgram.y */
				    add_xt_info(p, DEFINITION | DECLARATION);
			       }
# endif /* SA */
			       return;
			  }
		     }
		}
#else /* ANSI not defined */

		if( scl==USTATIC || (scl==EXTERN && blevel==0) ){
			p->sclass = STATIC;
# ifdef SA			
			/* Have a static definition after a static function
			 * declaration or an extern function declaration or
			 * definition.
			 */
			if (saflag && !ISFTN(type)) {
			   /* Functions are handled from cgram.y */
			   add_xt_info(p, DEFINITION | DECLARATION);
			}
# endif /* SA */
			if( ISFTN(type) ) curftn = p;
			return;
		        }
#endif /* ANSI */	     
		/* helps with tentative definitions: 
		   not necessarily an error */
		if( scl == STATIC && blevel == 0 ){
		    if (!ISFTN(p->stype) 
/* CLL4600018 fix - disable p->sflags&SFDEF check if non-ansi. The check
    would always be true in non-ansi mode. */
#ifdef ANSI  
                                         || !(p->sflags&SFDEF)
#endif /* ANSI */
                                                              )
			return;
		}
		break;

	case USTATIC:
		if( scl==STATIC || scl==USTATIC ) {
# ifdef SA
		   if (saflag) {
		      add_xt_info(p, DECLARATION);
		   }
# endif /* SA */
		   return;
		}
#ifdef ANSI
		if( blevel == 0 && ( scl == EXTERN || scl == EXTDEF )) {
	             /* "linkage conflict with prior declaration for '%s'; 
			%s linkage assumed" */
		     WERROR( MESSAGE( 109 ), p->sname, "external" );
# ifdef SA
		     if (saflag) {
			  add_xt_info(p, DECLARATION);
		     }
# endif /* SA */
		     return;
		}     
#endif /* ANSI */
		break;

	case CLABEL:
		if( scl == ULABEL ){
			p->sclass = CLABEL;
# ifdef SA			
			if (saflag) {
			   add_xt_info(p, DECLARATION|DEFINITION);
			}
# endif /* SA */

#ifndef IRIF
			(void)locctr(PROG);
#endif /* not IRIF */

# ifdef HPCDB
			if (cdbflag) cdb_prog_label(p);
# endif
			deflab( (unsigned)p->offset );
#ifdef IRIF
			ir_declare_label( 
					 p->offset,
					 p->sname,
					 p->source_position = spos_$current_source_position );
#endif /* IRIF */
			return;
			}
		break;

	case TYPEDEF:
/*		if( scl == class ) return;*/
		break;

	case MOU:
	case MOS:
		/* this should never be reached ??? */
#ifdef IRIF
		cerror( "defid (IRIF only): struct/union inconsistent" );
#else /*not IRIF */
		if( scl == class ) {
			if( oalloc( p, &strucoff ) ) break;
			if( class == MOU ) strucoff = 0;
			psave( p );
			return;
			}
#endif /* not IRIF */
		break;

#ifndef ANSI
	case MOE:
		if( scl == class ){
			if( p->offset!= strucoff++ ) break;
			psave( p );
			}
		break;
#endif
	case EXTDEF:
		if( scl == EXTERN ) {
			p->sclass = EXTDEF;
			if( ISFTN(type) ) {
			     curftn = p;
			}
# ifdef SA
			/* Have somethink like "int x = 20;"  */
			if (saflag && !ISFTN(type)) {
			   /* Functions are handled from cgram.y */
			   add_xt_info(p, DEFINITION | DECLARATION);
			}
# endif /* SA */
			return;
			}
#ifdef ANSI
		if( blevel == 0 ) {
		     if( ISFTN( type )) {
			  curftn = p;
			  if( scl == USTATIC ) {
			       p->sclass = STATIC;
			       return;
			  }
		     }
		     else {
			  if( scl == STATIC ) {
			       /* "linkage conflict with prior declaration for '%s'; 
				  %s linkage assumed" */
			       WERROR( MESSAGE( 109 ), p->sname, "internal" );
# ifdef SA
			       /* Have somethink like "int x = 20;"  */
			       if (saflag && !ISFTN(type)) {
				    /* Functions are handled from cgram.y */
				    add_xt_info(p, DEFINITION | DECLARATION);
			       }
# endif /* SA */
			       return;
			  }
		     }
		}
#endif /* ANSI */
		break;

	case STNAME:
	case UNAME:
	case ENAME:
		if( scl != class ) break;
#ifdef ANSI
		if( ( dimtab[p->sizoff+1] == 0 ) && ( blevel == slev )) return;
#else
		if( dimtab[p->sizoff+1] == 0 ) return;  /* previous entry just a mention */
#endif
		break;

	case ULABEL:
		if( scl == CLABEL || scl == ULABEL ) return;
		break;
	case PARAM:
		/* "declared argument %s is missing" */
		if (scl != PARAM) UERROR( MESSAGE( 28 ), p->sname);
		/* fall thru */
	case AUTO:
	case REGISTER:
		;  /* mismatch.. */

		}

mismatch:
	if( blevel > slev && class != EXTERN 
		&& !( class == CLABEL && slev >= 2 ) ){

		p = q->nn.sym = hide( p );
#ifdef SA		
		/* Maybe this initialization should
		   be removed from here, add_entry and
		   ftnarg to stab_alloc.  Would be even
		   better if 0 was the NULL_XT_INDEX. */
        	p->xt_table_index = NULL_XT_INDEX;
#endif		
		goto enter;
		}
        p->sname = p->sname?p->sname:"(SNONAME)";
	if (status){
	    if (ISFTN(type)){
		/* "function declarations for %s have incompatible return types" */
		if (status&ICFTN) UERROR( MESSAGE( 149 ), p->sname);
#ifdef ANSI
		/* "function declarations for %s have incompatible number of parameters" */
		else if (status&ICFTNPARG) UERROR( MESSAGE( 150 ), p->sname);
		/* "function declarations for %s have incompatible use of ELIPSIS" */
		else if (status&ICFTNPELI) UERROR( MESSAGE( 151 ), p->sname);
		/* "function declarations for %s have incompatible parameters" */
		else if (status&ICFTNPPAR) UERROR( MESSAGE( 152 ), p->sname);
		/* "function prototype for %s and old style definition have incompatible number of parameters" */
		else if (status&ICFTNDARG) UERROR( MESSAGE( 153 ), p->sname);
		/* "function prototype for %s may not use ELIPSIS when used with old style declaration" */
		else if (status&ICFTNDELI) UERROR( MESSAGE( 154 ), p->sname);
		/* "function prototype for %s and old style definition have incompatible parameters" */
		else if (status&ICFTNDPAR) UERROR( MESSAGE( 155 ), p->sname);
		/* "function prototype for %s may not use ELIPSIS when used with an empty declaration" */
		else if (status&ICFTNXELI) UERROR( MESSAGE( 156 ), p->sname);
		/* "function prototype for %s must contain parameters compatible with default argument promotions, when used with an empty declaration" */
		else if (status&ICFTNXPAR) UERROR( MESSAGE( 157 ), p->sname);
#endif /*ANSI*/
		/* "incompatible declaration for %s" */
		else UERROR( MESSAGE( 161 ), p->sname);
	    } else if (ISARY(type)) {
		/* "incompatible array declarations for %s" */
		if (status&(ICARY|ICARYSIZE)) UERROR( MESSAGE( 158 ), p->sname);
		/* "incompatible declaration for %s" */
		else UERROR( MESSAGE( 161 ), p->sname);
	    } else if (ISPTR(type)) {
		/* "incompatible pointer declarations for %s" */
		if (status&(ICPTR|ICPTRQ)) UERROR( MESSAGE( 159 ), p->sname);
		/* "incompatible declaration for %s" */
		else UERROR( MESSAGE( 161 ), p->sname);
	    } else {
		/* "incompatible qualifier declarations for %s" */
		if (status&ICQUAL) UERROR( MESSAGE( 160 ), p->sname);
		/* "incompatible declaration for %s" */
		else UERROR( MESSAGE( 161 ), p->sname);
	    }
	} 
	else if( class == PARAM )
	     /* "redeclaration of formal parameter '%s' */
	     WERROR( MESSAGE( 97 ), p->sname );
	else {
	     /* "redeclaration of %s" */	    
	     UERROR( MESSAGE( 96 ), p->sname);
	}
	if( class==EXTDEF && ISFTN(type) ) curftn = p;
	return;

enter:  /* make a new entry */

# ifdef DEBUGGING
	if( ddebug ) printf( "	new entry made\n" );
# endif
	/* "void type incorrect for identifier '%s'" */
#ifndef ANSI
	if( type == UNDEF || ((type == VOID ) && (class != TYPEDEF)))
#else /* ANSI */
	if( type == UNDEF || ((type == VOID ) && (class != TYPEDEF) && 
	   !(p->sflags & SNONAME) ))
#endif /* ANSI */
	  UERROR( MESSAGE( 117 ), p->sname);
	p->stype = type;

# ifdef HPCDB
	if( !cdbflag || class != TYPEDEF || type != WIDECHAR
	   || strcmp( p->sname, "wchar_t" ))
	     p->sattrib = tattrib;
	else p->sattrib = tattrib |= ATTR_WIDE;

	if(in_comp_unit)
	     p->sflags |= SDBGREQD;
# else
	p->sattrib = tattrib;
# endif
	p->sclass = class;

	/* FARGs have their slevel link set early to facilitate getting
	 * an ordered parameter list for old style function declarations.
	 */
#ifndef ANSI
	if (stp != FARG) {
	     set_slev_link(p,blevel);
	}
	p->slevel = blevel;
#else /* ANSI */
	if (stp != FARG) {
		set_slev_link(p,blevel);
		p->slevel = blevel;
		}
	else if (p->slevel != blevel)
		cerror("FARG slevel/blevel [%d/%d] conflict in defid",
		   p->slevel, blevel);
#endif /* ANSI */
#ifndef IRIF
	p->offset = NOOFFSET;
#endif /* not IRIF */
	/* Reset SNAMEREF to indicate definition */ 
	p->sflags &= ~SNAMEREF;
	if( class == STNAME || class == UNAME || class == ENAME ) {
# ifdef ANSI
		if (inproto>0) warn_struct_in_proto(p, (TWORD)BTYPE(p->stype));
# endif
		p->sizoff = curdim;
		dstash( ( class==ENAME )? SZINT : 0 );  /* size */
		dstash( 0 ); /* index to members of str or union */
#ifndef IRIF
		dstash( ALSTRUCT );  /* alignment */
#else /* IRIF */
		dstash( 0 );
#endif /* IRIF */
		dstash( (int)p );  /* name index */
		dstash( 0 );  /* status word */
# ifdef HPCDB
		if (cdbflag) dstash( 0 );	/* ccom_dnttptr for this type */
# endif
		}
	else {
	     if( ISENUM( BTYPE(type), p->sattrib ) ) {
		  p->sizoff = q->fn.csiz;
	     }
	     else {
		  switch( BTYPE(type) ){
		     case STRTY:
		     case UNIONTY:
		       /* propagate CONST member information */
		       p->sattrib |= ( ( dimtab[q->fn.csiz+4] & TYPE_CON )
				      ? ATTR_HAS_CON : 0 );
		       p->sizoff = q->fn.csiz;

		       if( p->sclass == EXTERN && p->slevel > 1 ) {
			    /* block extern struct/union
			     * move the members to global level
			     */
			    make_members_global( p );
		       }
		       break;
		     default:
		       p->sizoff = BTYPE(type);
		  }
	     }
	}

	/* copy dimensions */

	p->dimoff = q->fn.cdim;

	/* allocate offsets */
	if( class&FIELD ){
		(void)falloc( p, class&FLDSIZ, 0, NIL );  /* new entry */
		psave( p );
		/* make certain the enum values fit in the field */
		if (ISENUM(BTYPE(type),p->sattrib))
		    {
		    int index;
		    struct symtab *sp;
		    int max = (ISUNSIGNED(type)?
			       (1<<(class&FLDSIZ))-1:
			       (1<<((class&FLDSIZ)-1))-1);
		    int min = (ISUNSIGNED(type)?0:-max-1);
		    for (index=dimtab[p->sizoff+1];dimtab[index];index++)
			{
			sp = (struct symtab *) dimtab[index];
			if (sp->offset < min || sp->offset > max)
			    /* "enumeration constant '%s' is outside range representable by bitfield '%s' */
			    WERROR( MESSAGE( 230 ), sp->sname, p->sname );
		    	}
		    }
		}
	else switch( class ){

	case AUTO:
#ifdef IRIF
	case REGISTER:
	     if( ir_storage_alloc( p ) == FE_ZERO_SIZE ) {
		  /* "unknown size for '%s'" */
		  UERROR( MESSAGE( 183 ), p->sname );
	     }
	     break;
#else /* not IRIF */

		(void)oalloc( p, &autooff );
#ifdef C1_C
		putsym(C1OREG, 14, p);
		p2word(-autooff/SZCHAR);
		putc1attr(p, regflag);  /* c1 attribute word */
		p2name(p->sname);
		if ( HAS_ANY_VOL(p) )
			{
			char vbuf[32];
			(void)sprntf(vbuf,"#7 -%d(%%a6),%d",autooff/SZCHAR,
				tsize(p->stype,p->dimoff,p->sizoff)/SZCHAR,NULL,0); /* SWFfc00726 fix */
			p2pass(vbuf);
			}
#else
#if (!defined(LINT) && !defined(CXREF))
		if ( HAS_ANY_VOL(p) )
			(void)prntf("#7 -%d(%%a6),%d\n",autooff/SZCHAR,
				tsize(p->stype,p->dimoff,p->sizoff,NULL)/SZCHAR,0); /* SWFfc00726 fix */
# endif /* !LINT && !CXREF */
#endif /* C1_C */
		break;
#endif /* not IRIF */

	case STATIC:
	case EXTDEF:
#ifdef IRIF
	     if( ISFTN( type )) curftn = p;
	     else if( ir_storage_alloc( p ) == FE_ZERO_SIZE ) {
		  /* "unknown size for '%s'" */
		  UERROR( MESSAGE( 183 ), p->sname );
	     }
	     break;
#else /* not IRIF */

	     p->offset = GETLAB();
	     if( ISFTN(type) ) curftn = p;

#ifdef C1_C
		/* fall through */
	case USTATIC:
                {
		 char buff[256];
			if (p->sclass == STATIC && p->slevel>1)
                                {
				if ( HAS_ANY_VOL(p) || volatile_flag)
					{
					(void)sprntf(buff,"#8 L%d",p->offset);
					p2pass(buff);
					}
                                (void)sprntf(buff,"L%d", p->offset);
                                }
			else
				{
				/* static, function names are _name */
				if ( HAS_ANY_VOL(p) || volatile_flag)
					{
					(void)sprntf(buff,"#8 %s",exname(p->sname));
					p2pass(buff);
					}
				(void)sprntf(buff,"%s",exname(p->sname));
				}
                        putsym(C1NAME, 0, p);
			p2name(buff);  /* e.g. L20 or _name */
                        putc1attr(p, regflag);
                        p2name(p->sname);
                }
#else /* C1_C */
#  if (!defined(LINT) && !defined(CXREF))
		if ( HAS_ANY_VOL(p) )
			if (p->sclass == STATIC && p->slevel>1)
				(void)prntf("#8 L%d\n", p->offset);
			else
				(void)prntf("#8 %s\n", exname(p->sname));
#  endif  /* !LINT && !CXREF */
#endif /* C1_C */
		break;
#endif /* not IRIF */

	case ULABEL:
	case CLABEL:
		p->offset = GETLAB();
		if (p->slevel != 2) {
		    reset_slev_link(p,2);
		    p->slevel = 2;
		}
		if( class == CLABEL ){
#ifndef IRIF
			(void)locctr( PROG );
#endif /* not IRIF */
# ifdef HPCDB
			if (cdbflag) cdb_prog_label(p);
# endif
			deflab( (unsigned)p->offset );
#ifdef IRIF
			ir_declare_label(p->offset,p->sname,p->source_position);
#endif /* IRIF */
			}
		break;

	case EXTERN:
#ifndef IRIF
		p->offset = GETLAB();
#endif /* not IRIF */
#ifndef ANSI
		if (p->slevel) {
		    reset_slev_link(p,0);
		    p->slevel = 0;
		}
#endif
#ifdef IRIF
		if( !ISFTN( type )) {
		     if( ir_storage_alloc( p ) == FE_ZERO_SIZE ) {
			  /* "unknown size for '%s'" */
			  UERROR( MESSAGE( 183 ), p->sname );
		     }
		}
		break;

#else /* not IRIF */
#ifdef C1_C
                putsym(C1NAME, 0, p);
                p2name(exname(p->sname));
                putc1attr(p, regflag);
                p2name(p->sname);
		if ( HAS_ANY_VOL(p) || volatile_flag)
			{
			char vbuf[256];
			(void)sprntf(vbuf,"#8 %s", exname(p->sname));
			p2pass(vbuf);
			}
#else /* C1_C */
#  if (!defined(LINT) && !defined(CXREF))
		if ( HAS_ANY_VOL(p) )
			(void)prntf("#8 %s\n", exname(p->sname));
#endif /* !LINT && !CXREF */
#  ifdef LINT_TRY
                        if( (xflag && !libflag) || stdlibflag ){
				int mask;
                                if (ISFTN(p->stype) )
                                    {
                                        NODE *kn = (NODE *)dimtab[p->dimoff];
                                        int narg = kn->ph.nparam;
                                        if (kn->ph.flags & SELLIPSIS)
                                                narg = -(narg + 1);
                                        if (kn->ph.flags & SPARAM) {
						mask = LPR|LDX;
						if (stdlibflag)
						    mask |= LBS;
					} else {
						mask = LDX;
						narg = 0;
						if (stdlibflag)
						    /* old-style function declaration for '%s' in LINTSTDLIB */
						    WERROR( MESSAGE( 244 ), p->sname );
					}
					outftn(p,mask,narg,lineno);
					if (stdlibflag && 
						DECREF(p->stype) != VOID){
						    outftn(p,LRV,0,lineno);
                                        }
                                    }
                                else {
					mask = LDX;
					if (stdlibflag)  mask |= LBS;
                                        outdef( p, mask, 0, lineno );
				}
			}
#  endif /* LINT_TRY */
#endif /* C1_C */
		break;
#endif /* not IRIF */

#ifndef IRIF
	case MOU:
	case MOS:
		(void)oalloc( p, &strucoff );
		if( class == MOU ) strucoff = 0;
		psave( p );
		break;

#else /* IRIF */
	case MOS:
	case MOU:
	        (void)ir_alloc_member( p, class==MOS ? STRTY : UNIONTY );
		psave( p );
		break;
#endif /* IRIF */

	case MOE:
#ifdef ANSI
		if( strucoff < last_enum_const )
		     /* "enum constant overflow: %s given value INT_MIN" */
		     WERROR( MESSAGE( 141 ), p->sname );
		last_enum_const = strucoff;

#endif
		p->offset = strucoff++;
		psave( p );
		break;

#ifdef IRIF
	case TYPEDEF:
		ir_declare_typedef(p);
		break;
#endif
#ifdef HPCDB
        case TYPEDEF:
                p->vt_file = cdbfile_vtindex;
                break;
#endif

#ifdef IRIF
	case PARAM:
#ifndef ANSI
		ir_declare_parameter(p,0);
#endif
		break;
#endif /* IRIF */

#ifndef IRIF
	case REGISTER:
#ifdef C1_C
    /* allow register variables after #pragma OPT_LEVEL 1 or OPTIMIZE OFF*/
	    if (optlevel < 2)
#endif  /* C1_C */
		{
		if (unionflag && type == UNIONTY)
			/* iff unionflag */
			/* change type to type of first field */
			type = ((struct symtab *) dimtab[dimtab[p->sizoff+1]])->stype;
		if ( ISPTR(type) ) 
			{
		  	p->offset = (regvar>>8)+A0;
		  	regvar -= 1<<8;
		  	if ((regvar&~0377) < (minrvar&~0377))
		    		{
				minrvar &= 0377;
				minrvar |= regvar&~0377; }
			}
		else
		  if ( ISFTP(type) )
			{
			register int t;
			if ( fpaflag )
			  {		/* dragon flt pt. */
			  t = (fdregvar >> 16) & 0xFF;
			  p->offset = t + FP0;
			  fdregvar = ((t - 1) << 16) | (fdregvar & 0xFF);
			  if (t < minrdvar)
				minrdvar = t;
			  }
			else
			  {		/* 881 only flt pt */
			  t = fdregvar & 0xFF;
			  p->offset = t + F0;
			  fdregvar = (t - 1) | (fdregvar & ~0xFF);
			  if (t < minrfvar)
				minrfvar = t;
			  }
			}
		  else
			{
		  	p->offset = regvar&0377;
		  	regvar = ((regvar&0377)-1) | (regvar&~0377);
		  	if ((regvar&0377) < (minrvar&0377))
		    		{
				minrvar &= ~0377; 
				minrvar |= regvar&0377;
				}
			}		   
		if( blevel == 1 ) p->sflags |= SSET;
	    }
/* In the 2-pass version, register-declared params are allowed by cisreg() to
 *   look like registers until bfcode().
 */
		break;
#endif /* not IRIF */
		}


# ifdef SA	
  /* Proper distinction between definitions and declarations */
#ifdef ANSI  
  if (saflag && p->sname!=NULL && !ISFTN(p->stype))
#else     
  if (saflag && !ISFTN(p->stype))
#endif     
    if (!(p->sclass==ULABEL || p->sclass==STNAME || p->sclass==UNAME || p->sclass==ENAME))
      if (p->sclass != EXTERN || (p->sclass == EXTERN &&  
				  p->cdb_info.info.extvar_isdefined == 1
				  ))
	 /* 2nd clause to || checks for "int x;",
          * which for some reason has class EXTERN. */
	add_xt_info(p, DEFINITION | DECLARATION);
      else
	 /* Have an extern.  Something like "extern int x;". */
	 add_xt_info(p, DECLARATION);


# endif /* SA */
# ifdef DEBUGGING
	if( ddebug ) printf( "	dimoff, sizoff, offset: %x, %x, %x\n",
		p->dimoff, p->sizoff, p->offset );
# endif

	}

LOCAL psave( i ) struct symtab *i; {
	if( paramno >= maxparamsz ){
		newparamstk();
		}
# ifdef DEBUGGING
	if (ddebug>3) printf("\tpsave[%d] = %d\n",paramno,(int)i);
# endif

	paramstk[ paramno++ ] = (int)i;
	}

ftnend(){ /* end of function */

	if( retlab != NOLAB ){ /* inside a real function */

#ifndef IRIF
		efcode();

#else /* IRIF */
#ifdef HPCDB
		if( cdbflag ) {
		     sltexit();
		     cdb_efunc();
		}
#endif /* HPCDB */
		ir_ftnend( retlab, curftn ); 
#endif /* IRIF */

		}
#ifndef FORT
	treset();
	resettreeasciz();
#endif
	checkst(0);
	retstat = 0;
	tcheck();
	curclass = SNULL;
	brklab = NOLAB;
	contlab = NOLAB;
	retlab = NOLAB;
	flostat = 0;
	if( nerrors == 0 ){
		if( psavbc != & asavbc[0] ) cerror("bcsave error");
		if( paramno != 0 ) cerror("parameter reset error");
		if( swx != 0 ) cerror( "switch error");
		}
	psavbc = &asavbc[0];
	paramno = 0;

#ifndef IRIF
	autooff = AUTOINIT;
	minrvar = regvar = MAXRVAR | ((MAXRVAR-2)<<8);
	minrfvar = MAXRFVAR;
	if ( fpaflag > 0 )	/* "if dragon_present use it, else use 881 */
	  minrdvar = MAXRFVAR;  /* alloc dragon regs that map to 881 regs */
	else
	  minrdvar = MAXRDVAR;
	fdregvar = (minrdvar << 16) | minrfvar;
#endif /* not IRIF */

	reached = 1;
	swx = 0;
	swp = swtab;
	}

dclargs(){
	register i;
	register struct symtab *p;
#ifndef ANSI
	register NODE *q;
#endif

#ifndef IRIF
	argoff = ARGINIT;
#endif /* not IRIF */

# ifdef DEBUGGING
	if( ddebug > 2) printf("dclargs()\n");
# endif

#ifndef IRIF

/* Emit the c2 comment indicating start-of-function.  Do it here rather
 * than in bfcode() so that #9 comes before any #7 for volatile parameters.
 */
    	(void)locctr( PROG );
#ifdef C1_C
#    ifndef NEWC0SUPPORT
	p2pass("#9");
#    endif /* NEWC0SUPPORT */
#else
#ifndef LINT
	prntf("#9\n");
#endif /* LINT */
#endif /* C1_C */
	p = curftn;
#ifdef C1_C
#    ifndef NEWC0SUPPORT
	defnam( p );
#    endif /* NEWC0SUPPORT */
#else
	defnam( p );
#endif /* C1_C */
#endif /* not IRIF */

/* Due to the implementation of prototypes, the ANSI compiler ends up
 * doing the psave's in the opposite order.
  */
# ifndef ANSI
	for( i=0; i<paramno; ++i ){
# else
	for ( i=paramno-1; i>=0; --i) {
# endif
		if( (p = ((struct symtab *) paramstk[i])) == NULL ) continue;
# ifdef DEBUGGING
		if( ddebug > 2 ){
			printf("\t%s (%x) ", p->sname, p);
			tprint(p->stype, p->sattrib );
			printf("\n");
			}
# endif

/* Set up a default type for any argument that did not have a declaration.
 * For ANSI, this setting of a has already been done earlier in a call to 
 * do_default_param_dcls()
 */
# ifndef ANSI
		if( p->stype == FARG ) {
			q = block(FREE,NIL,NIL,INT,0,INT);
			q->nn.sym = p;
			defid( q, PARAM );
			}
# endif

#ifndef IRIF
#  if (!defined(LINT) && !defined(CXREF))
		if ( HAS_ANY_VOL(p) )
			{
			int tsz = tsize(p->stype, p->dimoff, p->sizoff, NULL, 0)/SZCHAR; /* SWFfc00726 fix */
#ifdef C1_C
			char volbuf[64];
			(void)sprntf(volbuf, "#7 %d(%%a6),%d", argoff/SZCHAR, tsz);
			p2pass(volbuf);
#else  /* C1_C */
			(void)prntf("#7 %d(%%a6),%d\n", argoff/SZCHAR, tsz);
#endif /* C1_C */
			}
#  endif  /* !LINT && !CXREF */
		(void)oalloc( p, &argoff );  /* always set aside space, even for register arguments */
#else /* IRIF */
		if( ir_s_iszero_size( p )) {
		     /* "unknown size for '%s'" */
		     UERROR( MESSAGE( 183 ), p->sname );
		}
#endif /* IRIF */
		

		}
#ifndef IRIF
	CENDARG();
	(void)locctr(PROG);
	++ftnno;
	bfcode( (struct symtab **)paramstk, paramno );
#else /* IRIF */

	(void)ir_ftnbeg( curftn, paramno );
	retlab = GETLAB();
# ifdef HPCDB
	if (cdbflag) cdb_bfunc( curftn, (struct symtab **)paramstk, paramno ); 
# endif /* HPCDB */
# ifndef HAIL
	/* check for old-style function with a FLOAT parameter. Under
	 * IRIF && ANSI, the type is rewritten to DOUBLE in 'defid()'
	 * Here it is converted back to FLOAT and prolog code is issued
	 * to convert the function parameter back to FLOAT
	 */
	for( i=0; i < paramno; i++ ){
	     p = (struct symtab *)paramstk[i];
	     if( p->sflags & SFLTCVT ) {
		  p->stype = FLOAT;
		  ir_cvt_arg_double_to_float( p );
	     }
	}
# endif /* !HAIL */
#endif /* IRIF */

	paramno = 0;
	}

NODE *
rstruct( idp, soru ) struct symtab *idp; { /* reference to a structure or union, with no definition */
	register struct symtab *p;
	register NODE *q;
	NODE *tm;

	p = idp;
	switch( p->stype ){

	case UNDEF:
	def:
		q = block( FREE, NIL, NIL, 0, 0, 0 );
		q->nn.sym = idp;
		q->in.type = (soru&INSTRUCT) ? STRTY : ( (soru&INUNION) ? UNIONTY : ENUMTY );
		defid( q, (soru&INSTRUCT) ? STNAME : ( (soru&INUNION) ? UNAME : ENAME ) );
#ifdef IRIF
		ir_reference_tag(idp);
#endif
		break;

	case STRTY:
		if( soru & INSTRUCT ) break;
		goto def;

	case UNIONTY:
		if( soru & INUNION ) break;
		goto def;

	case ENUMTY:
		if( !(soru&(INUNION|INSTRUCT)) ) break;
		goto def;

		}
# ifdef SA
	/* Since in MKTY below the assumption is made that p->stype is
	 * valid -- even after defid, meaning that stype might have 
	 * been changed, but the symbol entry &stab[idn] wasn't --
	 * therefore static will use idn.    */
	if (saflag)
	   add_xt_info(idp, USE);
# endif	
	stwart = instruct;
	tm = MKTY( p->stype, p->dimoff, p->sizoff);
	tm->in.tattrib = p->sattrib;
	tm->nn.sym = idp;
	return( tm );
	}

moedef( idp ) struct symtab *idp; {
	register NODE *q;

	q = block( FREE, NIL, NIL, MOETY, 0, 0 );
	q->nn.sym = idp;
	if( idp != NULL ) defid( q, MOE );
#ifdef SA	
	if (saflag)
	   add_xt_info(q->nn.sym,MODIFICATION);
#endif	
	}

bstruct( idp, soru ) struct symtab *idp; { /* begining of structure or union declaration */
	register NODE *q;

	psave( (struct symtab *)instruct );
	psave( (struct symtab *)curclass );
	psave( (struct symtab *)strucoff );
	strucoff = 0;
	psave( (struct symtab *)inproto );       /* save even in non-ANSI to keep code that
				 * accessed the paramstk in 'dclstruct'
				 * less complicated.
				 */
	instruct = soru;
	q = block( FREE, NIL, NIL, 0, 0, 0 );
	q->nn.sym = idp;
	if( instruct==INSTRUCT ){
		curclass = MOS;
		q->in.type = STRTY;
#ifdef IRIF
		ir_initdecl_sutype(STRTY,(idp != NULL)?idp->sizoff:0);
#endif /* IRIF */
		if( idp != NULL ){
			defid( q, STNAME );
#ifdef IRIF
			ir_declare_tag(idp);
#endif
			}
		}
	else if( instruct == INUNION ) {
		curclass = MOU;
		q->in.type = UNIONTY;
#ifdef IRIF
		ir_initdecl_sutype(UNIONTY,(idp != NULL)?idp->sizoff:0);
#endif /* IRIF */
		if( idp != NULL ){
			defid( q, UNAME );
#ifdef IRIF
			ir_declare_tag(idp);
#endif
			}
		}
	else { /* enum */
		curclass = MOE;
		q->in.type = ENUMTY;
#ifdef ANSI
		last_enum_const = 0;
#endif
#ifdef IRIF
		ir_initdecl_enumty((idp != NULL)?idp->sizoff:0);
#endif
		if( idp != NULL ){
			defid( q, ENAME );
#ifdef IRIF
			ir_declare_tag(idp);
#endif
			}
		}
	psave( idp = q->nn.sym );

# ifdef ANSI
	if (inproto>0)
	   warn_struct_in_proto(idp, q->in.type);
	inproto = 0;
# endif

	/* the "real" definition is where the members are seen */
# ifdef SA
        if (saflag && idp != 0) 
	   add_xt_info( idp, DEFINITION | DECLARATION);
# endif	
	if( idp != 0 ) {
	  /* Reset SNAMEREF to indicate definition */ 
	  idp->sflags &= ~SNAMEREF;
#ifdef HPCDB
          idp->vt_file = cdbfile_vtindex; /*set indx of declaring file*/
#endif
         }
	return( paramno-5 );
	}

NODE *
dclstruct( oparam ){
	register struct symtab *p;
	register sa, j, sz, szindex;
#ifndef IRIF
	register al;
#endif /* not IRIF */
	register TWORD temp;
	register struct symtab *i;
#ifdef LINT_TRY
	int alignment_ok = 1;	/* alignment is ok, until proven otherwise */
	int maxalign = 8;	/* maximum object alignment seen */
#endif

	/* paramstack contains:
		paramstk[ oparam ] = previous instruct
		paramstk[ oparam+1 ] = previous class
		paramstk[ oparam+2 ] = previous strucoff
		paramstk[ oparam+3 ] = previous inproto
		paramstk[ oparam+4 ] = structure name

		paramstk[ oparam+5, ... ]  = member stab indices

		*/


	temp = (instruct&INSTRUCT)?STRTY:((instruct&INUNION)?UNIONTY:ENUMTY);
	if( (i=((struct symtab *) paramstk[oparam+4])) == NULL ){
		szindex = curdim;
		dstash( 0 );  		/* size */
		dstash( 0 );  		/* index to member names */
#ifndef IRIF
		dstash( ALSTRUCT );	/* alignment */
#else /* IRIF */
		dstash( 0 );
#endif /* IRIF */
		dstash( -lineno );	/* name of structure */
		dstash( 0 );            /* status word */
# ifdef HPCDB
		if (cdbflag) dstash(0);	/* ccom_dnttptr for this type */
# endif
		}
	else {
		szindex = i->sizoff;
		}

# ifdef DEBUGGING
	if( ddebug ){
		printf( "dclstruct( %s ), szindex = %x\n",
			(i != NULL)? i->sname : "??", szindex );
		}
# endif
        dimtab[ szindex+1 ] = curdim;
#ifndef IRIF
        al = al_struct[align_like];
#endif /* not IRIF */

	for( j = oparam+5;  j< paramno; ++j ){
		dstash( (int)(p=((struct symtab *) paramstk[j])) );
		if( temp == ENUMTY ){
			p->sizoff = szindex;
			continue;
			}
		/* type is either STRTY or UNIONTY
		 *
		 * update status word if member contains CONST or VOL
		 */
		if( p->stype & ~BTMASK ){
			if ( ISPTR(p->stype) && ISPCON(p->sattrib))
				dimtab[szindex+4] |= TYPE_CON;
			}
		else if(  ISCON( p->sattrib ) || HAS_CON( BTYPE(p->stype), p->sattrib ) )
		     dimtab[szindex+4] |= TYPE_CON;
		if( HAS_ANY_VOL(p) ) 
		     dimtab[szindex+4] |= TYPE_VOL;
#ifndef IRIF
		sa = talign( p->stype, p->sizoff, FALSE);
		if( p->sclass & FIELD ){
			sz = p->sclass&FLDSIZ;
			}
		else {  
		        report_no_errors = 1; /* 2nd time around for tsize */
			sz = tsize( p->stype, p->dimoff, p->sizoff, p->sname, 0 ); /* SWFfc00726 fix */
			report_no_errors = 0;
			}
		if( sz > strucoff ) strucoff = sz;  /* for use with unions */
		SETOFF( al, sa );
		/* set al, the alignment, to the lcm of the alignments of the members */
#endif /* not IRIF */
#ifdef LINT_TRY
		if (sflag && !(p->sclass&FIELD)){
			if ((p->stype == STRTY)||(p->stype == UNIONTY)){
				int align = lint_max_align(p->stype,p->sizoff);
				if (align >= maxalign) maxalign = align;
				if (p->offset%align) alignment_ok = 0;
			} else if (ISARY(p->stype)){
				if (sa >= maxalign) maxalign = sa;
				if (p->offset%sa) alignment_ok = 0;
			} else {
				if (sz >= maxalign) maxalign = sz;
				if (p->offset%sz) alignment_ok = 0;
			}
		}
#endif
		}
	dstash( 0 );  /* endmarker */
#ifdef LINT_TRY
	if (temp != ENUMTY){
	    if (!alignment_ok)
		/* "alignment of struct '%s' may not be portable" */
		WERROR( MESSAGE( 239 ),(i==NULL)?"??":i->sname);
	    if (sflag && ((strucoff % maxalign)||
		      ((maxalign < al)&&(strucoff%al))))
	  	/* "trailing padding of struct '%s' may not be portable" */
	  	WERROR( MESSAGE( 240 ),(i==NULL)?"??":i->sname);
	    }
#endif
#ifndef IRIF
	SETOFF( strucoff, al );

	if( temp == ENUMTY ){
		register TWORD ty;

		ty = INT;
		strucoff = tsize( ty, 0, (int)ty, (char *)NULL, 0); /* SWFfc00726 fix */
		al = talign( ty, (int)ty, FALSE );
		}

	if( strucoff == 0 ) {
	     /* "zero sized structure/union" */
	     UERROR( MESSAGE( 185 ) );
	     strucoff = SZINT;
	   }
	dimtab[ szindex ] = strucoff;
	dimtab[ szindex+2 ] = al;
#else /* IRIF */
	if( temp == ENUMTY ){
	     dimtab[ szindex ] = SZINT;
	     ir_termdecl_enumty(szindex);
	}
	else {
	     /* terminate, getting size and alignment */
	     ir_termdecl_sutype( &dimtab[ szindex ], &dimtab[ szindex+2 ], szindex );
	     if( dimtab[ szindex ] == 0 ) {
		  /* "zero sized structure/union" */
		  UERROR( MESSAGE( 185 ) );
		  dimtab[ szindex ] = SZINT;
	     }
	}
#endif /* IRIF */

	dimtab[ szindex+3 ] = paramstk[ oparam+4 ];  /* name index */

# ifdef DEBUGGING
	if( ddebug>1 ){
		printf( "\tdimtab[%x,%x,%x,%x,%x] = %x,%x,%x,%x,%x\n",
			szindex,szindex+1,szindex+2,szindex+3,szindex+4,
			dimtab[szindex],dimtab[szindex+1],dimtab[szindex+2],
			dimtab[szindex+3],dimtab[szindex+4] );
		for( j = dimtab[szindex+1]; dimtab[j] != NULL; ++j ){
			printf( "\tmember %s(%x)\n",
				((struct symtab *) dimtab[j])->sname, dimtab[j] );
			}
		}
# endif

        stwart = instruct = paramstk[ oparam ];
        curclass = paramstk[ oparam+1 ];
	strucoff = paramstk[ oparam+2 ];

# ifdef ANSI
	inproto = paramstk[ oparam+3 ];
# endif
	paramno = oparam;

	return( MKTY( temp, 0, szindex ) );
	}

# ifdef ANSI
/* Called when processing a structure, union, or enum type and inside
 * a prototype  or new-style function header.
 * Give a warning that this struct/union scope is limited to the
 * function declaration or definition (and so can't be compatible with
 * any other declaration for the function).  Don't warn in the case of an
 * 'enum', they are compatible with any int type.
 */
warn_struct_in_proto(tagname, type)
  struct symtab * tagname;
  TWORD type;
{ char * tagtype;
  if (type != STRTY && type != UNIONTY)
	return;
  tagtype = type==STRTY?"struct" : "union";
  if (tagname && tagname->sname)
	/* "%s '%s' declared in function parameter list will have scope
	    limited to this function declaration or definition"
	 */
	WERROR( MESSAGE( 231 ), tagtype, tagname->sname);
  else
	/* "untagged %s declared in function parameter list will have scope
	    limited to this function declaration or definition"
	 */
	WERROR( MESSAGE( 232 ), tagtype);
}
# endif

	/* VARARGS */

yyaccpt(){
        if( must_check_globals ) check_globals();
	ftnend();
	}

ftnarg( idp ) struct symtab *idp; {
	switch( idp->stype ){

	case UNDEF:
		/* this parameter, entered at scan */
# ifdef SA
	        if (saflag) {
		   idp->xt_table_index = NULL_XT_INDEX; 
		   idp->slevel = 1;
		   add_xt_info(idp, DECLARATION);
		}
# endif		
		break;
	case FARG:
		/* "redeclaration of formal parameter '%s'" */
		UERROR( MESSAGE( 97 ), idp->sname);
		/* fall thru */
	case FTN:
		/* the name of this function matches parm */
		/* fall thru */
	default:
		idp = hide( idp );
# ifdef SA
	        if (saflag) {
		   idp->xt_table_index = NULL_XT_INDEX;
		   idp->slevel = 1;
		   add_xt_info(idp, DECLARATION);
		}
# endif		
		break;
	case TNULL:
		/* unused entry, fill it */
		/* ??? I don't think this entry is reachable anymore
		 * with the new symbol table structure. Remove it ??? */
		;
		}
	idp->stype = FARG;
	idp->sclass = PARAM;
	set_slev_link( idp, 1 );
# ifndef ANSI
	/* the ANSI compiler delays pushing the param pointer onto the
	 * paramstk until 'rescope_parameter_list()' when we know we
	 * are starting a function definition versus a prototype declaration.
	 */
	psave( idp );
# else
	idp->slevel = blevel;
	idp->sflags |= SFARGID;
#endif
	}

#ifndef IRIF

talign( ty, s, outerflag) register TWORD ty; int s; flag outerflag;
{
	/* Compute the alignment (in bits)of an object with type ty, sizeoff
	   index s. outerflag TRUE iff called from oalloc() and the sclass is
	   AUTO or STATIC.
	*/

	register short i;
	register aryflag = 0;
        register int struct_union_flag = (instruct & (INSTRUCT|INUNION));
	if (s < 0) switch (ty)
		{
		case UNSIGNED:
		case INT:
		case USHORT:
		case SHORT:
		case UCHAR:
		case CHAR:
# ifdef ANSI
		case SCHAR:
		case LONG:
		case ULONG:
# endif	
			break;
		default:
			return(fldal(ty));
		}

	for( i=0; i<=(SZINT-BTSHIFT-1); i+=TSHIFT ){
		switch( (ty>>i)&TMASK ){

		case FTN:
			cerror( "compiler takes alignment of function");
                case PTR:
                        return( struct_union_flag ? al_point[align_like]
                                : outerflag ? ALSTACK
                                : ALPOINT );
		case ARY:
			aryflag++;
			continue;
		case 0:
			goto validtype;	/* break out of both control structures */
			}
		}
validtype:
	switch( BTYPE(ty) ){

	case UNIONTY:
	case ENUMTY:
	case STRTY:
		return( outerflag? ALSTACK : (unsigned int) dimtab[s+2] );
#ifdef ANSI
	case SCHAR:
#endif 
	case CHAR:
	case UCHAR:
		return( struct_union_flag ? al_char[align_like] 
			: (outerflag&&aryflag) ? ALSTACK 
			: ALCHAR );
	case FLOAT:
		return( struct_union_flag ? al_float[align_like] 
			: outerflag ? ALSTACK 
			: ALFLOAT );
	case DOUBLE:
		return( struct_union_flag ? al_double[align_like] 
#ifdef DBL8
			: outerflag ? ALCOMMONDOUBLE
#else
			: outerflag ? ALSTACK 
#endif /* DBL8 */
			: ALDOUBLE );
#ifdef ANSI
	case LONGDOUBLE:
		return( struct_union_flag ? al_longdouble[align_like] 
#ifdef DBL8
			: outerflag ? ALCOMMONDOUBLE
#else
			: outerflag ? ALSTACK 
#endif /* DBL8 */
			: ALLONGDOUBLE );
#endif
	case SHORT:
	case USHORT:
		return( struct_union_flag ? al_short[align_like] 
			: ALSHORT );
	case LONG:
	case ULONG:
		return( struct_union_flag ? al_long[align_like] 
			: outerflag ? ALSTACK 
			: ALLONG );
	default:
		return( struct_union_flag ? al_int[align_like] 
			: outerflag ? ALSTACK 
			: ALINT );
		}
	}

#endif /* not IRIF */

/* return # bits needed to store a constant or an item of type ty */
/* we may wish to get these defines from <limits.h> */
#define INT_MIN ( -2147483647 - 1)
#define INT_MAX 2147483647

#ifndef IRIF

static int tsize_overflow = 0;
OFFSZ
tsize( ty, d, s, sname, cflag )  register TWORD ty; char *sname; int cflag; {
	/* compute the size (in bits) associated with type ty,
	    dimoff d, and sizoff s  . SWFfc00726 - added cflag to indicate
            if code generation is in progress. */
	/* BETTER NOT BE CALLED WHEN t, d, and s REFER TO A BIT FIELD... */

	register short i;
	OFFSZ result;
	int breakloop = 0;
	double mult = 1.0;
	constant_folding = 1;

	for( i=0; i<=(SZINT-BTSHIFT-1); i+=TSHIFT ){
		switch( (ty>>i)&TMASK ){

		case FTN:
		        /* "function designator used in an incorrect context" */
			UERROR( MESSAGE( 77 ));
			return( SZINT );
		case PTR:
			return( SZPOINT * mult );
		case ARY:
                        /* SWFfc00726 - don't break out of loop if dimtab[d]
                           is equal to zero and cflag is 1. Continue process */
                        if (dimtab[d] == 0) {
                           if (!cflag)  {
                              breakloop++;
                               mult *= (unsigned int) dimtab[ d++ ];
                           }
                           else {
                              d++;
                              mult = (unsigned int) 1;
                           }
                        }
                        else mult *= (unsigned int) dimtab[ d++ ];
			break;
		case 0:
			break;

			}
		if (breakloop) break;
		}

	if(  ( dimtab[s] == 0 ) ||( ( mult == 0 ) && ( instruct & (INSTRUCT|INUNION)))) {
	        if( !report_no_errors ) {
		     if( sname == NULL )
  		          /* "unknown size" */
		          UERROR( MESSAGE( 114 ) );
		     else
		          /* "unknown size: %s" */
		          UERROR( MESSAGE( 183 ), sname );
		   }
		if( !( instruct & (INSTRUCT|INUNION)))
		     return( SZINT );
		}
	mult *= (unsigned int) dimtab[s];
	if ((mult > INT_MAX) || (mult < INT_MIN)) tsize_overflow++;
	result = mult;
	constant_folding = 0;
	return( result );
	}

LOCAL inforce( n ) register OFFSZ n; {  /* force inoff to have the value n */
	/* inoff is updated to have the value n */
	register OFFSZ wb;
	register rest;
	/* rest is used to do a lot of conversion to ints... */

	if( inoff == n ) return;
	if( inoff > n ) {
		cerror( "initialization alignment error");
		}

	wb = inoff;
	SETOFF( wb, SZINT );

	/* wb now has the next higher word boundary */

	if( wb >= n ){ /* in the same word */
		rest = n - inoff;
		vfdzero( rest );
		return;
		}

	/* otherwise, extend inoff to be word aligned */

	rest = wb - inoff;
	vfdzero( rest );

	/* now, skip full words until near to n */

	rest = (n-inoff)/SZINT;
	zecode( rest );

	/* now, the remainder of the last word */

	rest = n-inoff;
	vfdzero( rest );
	if( inoff != n ) cerror( "inoff error");

	}

LOCAL vfdalign( n ){ /* make inoff have the offset the next alignment of n */
	OFFSZ m;

	m = inoff;
	SETOFF( m, n );
	inforce( m );
	}
#endif /* not IRIF */

#ifdef DEBUGGING
flag idebug = 0;
#endif

LOCAL int ibseen = 0;  /* the number of } constructions which have been filled */

#if defined(ANSI) || defined(HAIL)
LOCAL flag flush_to_RC = 0;  /* when true throws away initializers to } */
#endif /* ANSI */

#ifndef IRIF
LOCAL char iclass;        /* storage class of thing being initialized */
#else /* IRIF */
char iclass;
#endif /* IRIF */

#define EXT_INIT_ERR -1   /* iclass value if attempt to init block scope extern */

LOCAL short ilocctr = 0;  /* location counter for current initialization */

beginit(curid,indata)
struct symtab *curid;
int indata; /* TRUE=.data segment, FALSE=.bss segment */
{
	/* beginning of initilization; set location ctr and set type */
	register struct symtab *p;
	typ_initialize = 1;

# ifdef DEBUGGING
	if( idebug >= 3 ) printf( "beginit(), curid = 0x%x\n", curid );
# endif

	p = curid;

	iclass = p->sclass;
#ifndef ANSI
	if( curclass == EXTERN ) iclass = EXTERN;
#endif
	switch( iclass ){
	case TYPEDEF:		/* 13 */
		/* incorrect storage class */
		UERROR( MESSAGE( 52 ));
		break;
	case UNAME:		/* 12 */
	case AUTO:		/* 1  */
	case REGISTER:		/* 4  */
#ifndef IRIF
#ifdef ANSI
		/* set up aggregate_expr to be "-1" in the unknown cases */
		if (ISARY(p->stype)||(p->stype==STRTY)||(p->stype==UNIONTY))
			aggregate_expr = -1;
		else aggregate_expr = 1;
#endif
#endif /* not IRIF */
		break;
	case EXTERN:		/* 2  */
#ifdef ANSI
		if (blevel == 0) iclass = p->sclass = EXTDEF;
		else return;
#else
		return;
#endif
		/* fall through */
	case EXTDEF:		/* 5  */
	case STATIC:		/* 3  */
		if( blevel == 0 ) {
		     /* ensure first that variable is uninitialized */
		     if( p->sflags & SINITIALIZED ) {
			  /* "reinitialization of %s" */
			  UERROR( MESSAGE( 200 ), p->sname );
			  return;
		     }
		     /* mark as initialized */
		     else p->sflags |= SINITIALIZED;
		}
#ifndef IRIF
# if (!defined(LINT) && !defined(CXREF))
#ifndef C1_C
		if (blevel > 0) (void)prntf("\tlalign\t1\n");
			/* just to suck up any loose labels */
		ilocctr = (indata)?DATA:BSS;
		(void)locctr( ilocctr );
		switch ( talign(p->stype, 0, TRUE) )
			{
			case ALCHAR:
				break;
			case ALINT:
				(void)prntf("\tlalign\t2\n");
				break;
			case ALSTACK:
				(void)prntf("\tlalign\t4\n");
				break;
#ifdef DBL8
			case ALCOMMONDOUBLE:
				(void)prntf("\tlalign\t8\n");
				break;
#endif /* DBL8 */
			}
#else /*C1_C*/
		if (blevel > 0) 
			p2pass("\tlalign\t1");
			/* just to suck up any loose labels */
		ilocctr = (indata)?DATA:BSS;
		(void)locctr( ilocctr );
		switch ( talign(p->stype, 0, TRUE) )
			{
			case ALCHAR:
				break;
			case ALINT:
				p2pass("\tlalign\t2");
				break;
			case ALSTACK:
				p2pass("\tlalign\t4");
				break;
#ifdef DBL8
			case ALCOMMONDOUBLE:
				p2pass("\tlalign\t8");
				break;
#endif /* DBL8 */
			}
#endif /*C1_C*/
#else  /*LINT or CXREF*/
		ilocctr = (indata)?DATA:BSS;
		(void)locctr( ilocctr );
#endif /*LINT, CXREF*/
		defnam( p );
#endif /* not IRIF */

		}

#ifdef IRIF
	ir_init_init( curid );
#endif /* IRIF */

	inoff = 0;
	ibseen = 0;

	pstk = 0;

	instk( curid, p->stype, p->sattrib, p->dimoff, p->sizoff, inoff );

	}

LOCAL instk( id, t, ta, d, s, off ) OFFSZ off; TWORD t; struct symtab *id; {
	/* make a new entry on the parameter stack to initialize id */

	register struct symtab *p;
	register struct instk *lp;

	for(;;){
# ifdef DEBUGGING
		if( idebug ) printf( "instk( id=0x%x, t=0x%x, ta=0x%x, d=0x%x, s=0x%x, off=0x%x)\n",
					id, t, ta, d, s, off );
# endif

		/* save information on the stack */

		if( !pstk ) pstk = instack;
		else if (pstk >= &instack[INSTACKSZ-1])
		     /* "type too complex" */
		     UERROR( MESSAGE( 72 ));
		else ++pstk;

		lp = pstk;
		lp->in_fl = 0;	/* 'left brace'  flag */
		lp->in_id =  id ;
		lp->in_t =  t ;
		lp->in_tattrib = ta;
		lp->in_d =  d ;
#ifndef IRIF
		/* make sure enums get converted here as in buildtree */
		if( ISENUM(BTYPE(t),ta) )
#ifdef SIZED_ENUMS
			lp->in_s = BTYPE(t);
#else
			lp->in_s = INT;
#endif
		else
#endif

			lp->in_s =  s ;
		lp->in_n = 0;  /* number seen */
#ifndef ANSI
		lp->in_x =  t==STRTY ?dimtab[s+1] : 0 ;
#else
		lp->in_x =(t==STRTY||t==UNIONTY)?dimtab[s+1] : 0 ;
#endif
		lp->in_off =  off;   /* offset at the beginning of this element */
# ifdef DEBUGGING
		if (off < 0)
			cerror ("negative in_off");
# endif	/* DEBUGGING */

#ifndef IRIF
		/* if t is an array, DECREF(t) can't be a field */
		/* INS_sz has size of array elements, and -size for fields */
		if( ISARY(t) ){
			lp->in_sz = tsize( (TWORD)DECREF(t), d+1, s, (char *)NULL, 0 ); /* SWFfc00726 fix */
			}
		else if( id->sclass & FIELD ){
			lp->in_sz = - ( id->sclass & FLDSIZ );
			}
		else lp->in_sz = 0;
#endif /* not IRIF */

#if !defined(ANSI) && !defined(HAIL)
		if( (iclass==AUTO || iclass == REGISTER ) &&
			/* "no automatic aggregate initialization" */
			(ISARY(t) || t==STRTY) ) UERROR( MESSAGE( 79 ) );
#endif
		/* now, if this is not a scalar, put on another element */

		if( ISARY(t) ){
#ifdef IRIF
		     if( pstk == instack || pstk->in_id != (pstk-1)->in_id ){
			  /* just one call per array */
			  ir_init_sdtype( pstk->in_id );
		     }
#endif /* IRIF */
			t = DECREF(t);
			++d;
			continue;
			}
#ifndef ANSI
		else if( t == STRTY ){
#else
		else if( t == STRTY || t == UNIONTY ){
#endif
			if (dimtab[s] == 0)
				return; 
				/* error will be reported in tsize() later */

#ifdef IRIF
			ir_init_sdtype( pstk->in_id );
#endif /* IRIF */
			id = (struct symtab *) dimtab[lp->in_x];
			if (id == NULL) {
			     /* "inaccessible structure members - 
				 no initialization possible" */
			     WERROR( MESSAGE( 76 ));
			     return;
			}
			p = id;
#ifndef ANSI
			if( p->sclass != MOS && !(p->sclass&FIELD) ) 
#else
			if( p->sclass != MOS && p->sclass != MOU && !(p->sclass&FIELD) ) 
#endif
				cerror( "insane structure member list" );
			t = p->stype;
			ta = p->sattrib;
			d = p->dimoff;
			s = p->sizoff;
			off += p->offset;
			continue;
			}
		else return;
		}
	}

#ifndef IRIF
#ifdef ANSI
setup_aggregate_initialization() {
	/* first determine which of the following cases applies:
	 * struct x = e;	use normal structure assign
	 * struct x = { e };	use same code as for static aggregates and
	 *			copy the results in.
	 */
	register struct instk *temp = pstk;
	for (;temp >= instack;--temp) /* SWFfc00674 fix */
		if (temp->in_fl == 1) { aggregate_expr=0; break; } 
	if (aggregate_expr==-1) { aggregate_expr=1; return; }
	/* get a label for the auto aggregate */
	auto_aggregate_label = GETLAB();
#ifndef LINT
#ifndef C1_C
	(void)prntf("\tlalign\t1\n");
	(void)locctr(DATA);
	(void)prntf("\tlalign\t4\n");
#else
	{
		char buff[16];
		struct symtab *id = instack[0].in_id;
		(void)sprntf(buff,"L%d",auto_aggregate_label);
		putsym(C1NAME,0,id);
		p2name(buff);
		putc1attr(id,0);
		p2name(buff);
		p2pass("\tlalign\t1");
		(void)locctr(DATA);
		p2pass("\tlalign\t4");
	}
#endif
	defnam(instack[0].in_id);
#endif /* LINT */
}
#endif /*ANSI*/
#endif /* not IRIF */

#ifndef ANSI
NODE *
getstr(){ /* decide if the string is external or an initializer, and get the contents accordingly */

	register NODE *p;
	unsigned int l, temp;

#ifdef DEBUGGING
/*	if (idebug >= 3) dumpinstk();*/
#endif
	if( (iclass==EXTDEF||iclass==STATIC) && (pstk->in_t == CHAR || pstk->in_t == UCHAR) &&
			pstk!=instack && ISARY( pstk[-1].in_t ) ){
		/* treat "abc" as { 'a', 'b', 'c', 0 } */
		is_initializer = 1;
		ilbrace();  /* simulate { */
#ifndef IRIF
		inforce( pstk->in_off );
		/* if the array is inflexible (not top level), pass in the size and
			be prepared to throw away unwanted initializers */
		(void)lxstr((pstk-1)!=instack?dimtab[(pstk-1)->in_d]:0);  /* get the contents */
#else /* IRIF */
		(pstk-1)->in_n =
		     ir_str_initializer( irif_string_buf,
					lxstr( (pstk-1)!=instack?
					      dimtab[(pstk-1)->in_d]:0 ),
					(pstk-1)->in_id,instack[0].in_id,
					initializer_offset(),(pstk-1)->in_t,
					(pstk-1)->in_tattrib,(pstk-1)->in_s,
					(pstk-1)->in_d,pstk != &instack[0]);
#endif /* IRIF */		
		irbrace();  /* simulate } */
		return( NIL );
		}
	else { 
#ifndef IRIF
	     /* make a label, and get the contents and stash them away */
		if( iclass != SNULL ){ /* initializing */
			/* fill out previous word, to permit pointer */
			vfdalign( ALPOINT );	/* minimun alignment */
			}
		/* set up location counter */
		temp = locctr(  STRNG );	/* ALWAYS in data segment */
#ifndef C1_C
		deflab( l = GETLAB() );
#else
		tdeflab( l = GETLAB() );
#endif /* C1_C */
#endif /* not IRIF */

		is_initializer = 0;
		(void)lxstr(0); /* get the contents */
#ifdef IRIF
		p = buildtree( STRING, NIL, NIL );
#else /* not IRIF */
		(void)locctr( blevel==0? ilocctr:(short)temp );
		p = buildtree( STRING, NIL, NIL );
		p->tn.rval = -l;
#endif /* not IRIF */
		return(p);
		}
	}
#else /*ANSI*/
NODE *
getstr( string_type )
        int string_type;   /* indicates STRING or WIDESTRING  */
{ 
	register NODE *p;
	unsigned int label; 
	int temp;

#ifdef DEBUGGING
/*	if (idebug >= 3) dumpinstk();*/
#endif

	/* decide if the string is external or an initializer
	   and get the contents accordingly */

	if( ( iclass==EXTDEF || iclass==STATIC || iclass==AUTO || iclass==REGISTER) && 
	    ( ( string_type == STRING ) ?
		        (pstk->in_t == CHAR ||
			 pstk->in_t == UCHAR||
			 pstk->in_t == SCHAR ) :
	     (pstk->in_t == WIDECHAR ) ) &&
	   pstk!=instack && ISARY( pstk[-1].in_t ) ){

#ifndef IRIF
		inforce( pstk->in_off );
#endif /* not IRIF */
		
		/* get contents; pass in max no. of initializers  */

		is_initializer = 1;
                /* SWFfc01128 fix - If the previous lbrace flag (in_fl)
                   is already set, then set the current pstk */

                if ( pstk[-1].in_fl != 0 && pstk-1 != instack ) pstk->in_fl = 1;
		  else pstk[-1].in_fl = 1;

#ifndef IRIF
		if ((iclass==AUTO||iclass==REGISTER)&&(aggregate_expr == -1))
			setup_aggregate_initialization();
		if ( string_type == STRING )
		     (void)lxstr( dimtab[ ( pstk-1 )->in_d ] ) ;
		else
		     lxWideStr( dimtab[ ( pstk-1 )->in_d ] ); 
#else /* IRIF */
		if ( string_type == STRING )
		     (pstk-1)->in_n = lxstr( dimtab[ ( pstk-1 )->in_d ] ) ;
		else
		     (pstk-1)->in_n = lxWideStr( dimtab[ ( pstk-1 )->in_d ] );
		(pstk-1)->in_n = 
		     ir_str_initializer( irif_string_buf, (pstk-1)->in_n,
					(pstk-1)->in_id, instack[0].in_id,
					initializer_offset()-(pstk-1)->in_n*SZCHAR,(pstk-1)->in_t,
					(pstk-1)->in_tattrib,
					(pstk-1)->in_s,(pstk-1)->in_d,
					pstk != &instack[0]); 
#endif /* IRIF*/		
	
		if (ibseen > 0) ibseen--; /* SWFfc00674 fix */
		else {
		     pstk[-1].in_fl = 0;
		     --pstk;
#ifdef IRIF
		     ir_term_sdtype( pstk->in_id );
#endif /* IRIF */
		     gotscal();
		}

		return( NIL );
		}
	else { 
#ifndef IRIF
	        /* make a label, and get the contents and stash them away */
		if( iclass != SNULL ){ /* initializing */
			/* fill out previous word, to permit pointer */
			vfdalign( ALPOINT );	/* minimun alignment */
			}

		/* set up location counter */
		temp = locctr(  STRNG );	/* ALWAYS in data segment */

#ifndef C1_C
		deflab( label = GETLAB() );
#else
		tdeflab( label = GETLAB() );
#endif /* C1_C */
#endif /* IRIF not defined */

                /* get the contents */
		is_initializer = 0;
		if ( string_type == STRING )
		     (void)lxstr( 0 );
		else
		     lxWideStr( 0 );

#ifdef IRIF
		p = buildtree( string_type, NIL, NIL );
#else /* not IRIF */
		(void)locctr( blevel==0? ilocctr:temp );
		p = buildtree( string_type, NIL, NIL );
                p->tn.rval = -label;
#endif /* not IRIF */

		return(p);
		}
	}
#endif /*ANSI*/

#ifndef IRIF
putdatum( v, type ) int v; TWORD type;  { /* emit v */
	register NODE *p;
	p = bcon(v, type);
	incode( p, (( type == WIDECHAR ) ? SZWIDE : SZCHAR ) );
	tfree( p );
	gotscal();
	}
#endif /* not IRIF */

endinit(){
	register TWORD t;
	register d, s, n, d1;
	register struct instk *lp;
# ifdef ANSI
	struct symtab *id;
# endif /* ANSI */

#ifdef IRIF
	ir_end_init( instack->in_id,type_size(instack->in_id->stype,instack->in_id->sizoff,instack->in_id->dimoff) );
#endif /* IRIF */

	pstk = lp = instack;
# ifdef ANSI
	id = pstk->in_id;
# endif /* ANSI */

# ifdef DEBUGGING
	if( idebug ) printf( "endinit(), inoff = 0x%x\n", inoff );
# endif

# ifdef LINT
/* issue 1 warning/initializer, e.g. struct {char tz[3];} t = {"mst"}; */
	if (hflag && no_null_in_init)
              {
              /* null terminator not included in string initializer */
              WERROR(MESSAGE(197));
              no_null_in_init = 0;
              }
# endif /* LINT */

	switch( iclass ){

	case EXTERN:
	case EXT_INIT_ERR:     /* attempt to initialize block scope extern */
		typ_initialize = 0;
		return;
#ifndef IRIF
       	case AUTO:
	case REGISTER:
#ifndef ANSI
		typ_initialize = 0;
		return;
#else
		if (aggregate_expr) { typ_initialize = 0; return; }
#endif
#endif /* not IRIF */
	   }

	t = lp->in_t;
	d = lp->in_d;
	s = lp->in_s;
	n = lp->in_n;

	if( ISARY(t) ){
		d1 = dimtab[d];
#ifndef IRIF
		if (lp->in_sz){
		     vfdalign( lp->in_sz );  /* fill out part of the last element, if needed */
		     n = inoff/lp->in_sz;  /* real number of initializers */
	        }
		if( d1 >= n ){
		     /* once again, t is an array, so no fields */
		     inforce( tsize( t, d, s, (char *)NULL, 0)); /* SWFfc00726 fix */
		     n = d1;
		}
#else /* IRIF */
		if( initializations && d1 == 0 ) n++;
#endif /* IRIF */

		/* "number of initializers exceeds array size%s%s%s" */
		if( 
#ifndef IRIF
	             d1!=0 && d1!=n
#else /* IRIF */
		     d1!=0 && n>d1
#endif /* IRIF */
			) {
		     if( pstk->in_id == NULL || pstk->in_id->sname == NULL  ) {
			  UERROR( MESSAGE( 108 ), "", "", "" );
		     }
		     else {
			  UERROR( MESSAGE( 108 ), " for '", 
				                  pstk->in_id->sname, "'" );
		     }
		}

		/* "empty array declaration%s %s" */
		if( n==0
#ifdef IRIF
		    && d1 == 0
#endif /* IRIF */
			 ) {
		     if( pstk->in_id == NULL || pstk->in_id->sname == NULL ) {
			  UERROR( MESSAGE( 35 ), "", "", "" );
		     }
		     else {
			  UERROR( MESSAGE( 35 ), " for '", 
				 pstk->in_id->sname, "'" );
		     }
		}
#ifndef IRIF
#ifdef ANSI
		if (iclass != REGISTER && iclass != AUTO)
#endif
			dimtab[d] = n;
#else /* IRIF */
		if( dimtab[d] == 0 ) {
		     dimtab[d] = n;
		     ir_fix_array_size( pstk->in_id );
		     ir_complete_array( pstk->in_t,pstk->in_tattrib,pstk->in_s,pstk->in_d);
		}
#endif /* IRIF */
		}

	else if( t == STRTY || t == UNIONTY ){
#ifndef IRIF
		/* clearly not fields either */
		inforce( tsize( t, d, s, (char *)NULL, 0 ) ); /* SWFfc00726 fix */
#endif /* not IRIF */
		}
	/* formerly uerror( "bad scalar initialization" ) */
	else if( n > 1 ) 
	     cerror("panic in endinit: bad scalar initialization");
	/* this will never be called with a field element... */
#ifndef IRIF
	else inforce( tsize(t,d,s,(char *)NULL, 0 ) ); /* SWFfc00726 fix */
#endif /* not IRIF */

	paramno = 0;

#ifndef IRIF
	vfdalign( AL_INIT );
	inoff = 0;

#ifdef ANSI
	if (iclass==REGISTER||iclass==AUTO) {
		int sz;
		NODE *static_blk,*stack_blk,*p;	
#ifdef C1_C
		struct symtab sym;
#endif
		/* fix up zero sized arrays */
		if (ISARY(id->stype)&&!dimtab[id->dimoff]) {
		        dimtab[id->dimoff] = n;
			sz = tsize(id->stype,id->dimoff,id->sizoff,id->sname, 0); /* SWFfc00726 fix */
			(void)upoff((unsigned)sz,(unsigned)ALSTACK,&autooff);
			id->offset = -autooff;
			}
		else sz = tsize(id->stype,id->dimoff,id->sizoff,id->sname, 0); /* SWFfc00726 fix */
		idname = id;
		stack_blk = buildtree(NAME,(NODE *)0,(NODE *)0);
		static_blk = block(NAME,(NODE *)0,(NODE *)-auto_aggregate_label,stack_blk->in.type,stack_blk->fn.cdim,stack_blk->fn.csiz);
		static_blk->nn.sym = 0;
#ifdef C1_C
		sym = *(stack_blk->in.c1tag);
		sym.offset = auto_aggregate_label;
		sym.sclass = STATIC;
		static_blk->in.c1tag = &sym;
		static_blk->in.fixtag = stack_blk->in.fixtag = 1;
#endif
		if (ISARY(id->stype)) {
			/* fake up a structure copy to copy the array into
			 * the stack frame:
			 *
			 *    int a[] = { 1,2,3 };
			 *
			 * is translated into the following pseudo code:
			 *
			 * static int static_blk[3] = { 1,2,3 };
			 * int a[3];
			 * struct copy { char blk[12]; };
			 * *((struct copy *)a) = *((struct copy *)static_blk);
			 */
			NODE *cast;
			int initd,inits;
			/* add enough entries to dimtab so that tsize works */
			inits = curdim; dstash(sz);
			initd = curdim; dstash(0);
			cast = block(NAME,(NODE *)0,(NODE *)0,PTR+STRTY,initd,inits);

			stack_blk = buildtree(CAST,t1copy(cast),stack_blk);
			tfree(stack_blk->in.left);
			stack_blk->in.op = FREE;
			stack_blk = stack_blk->in.right;

			static_blk = buildtree(CAST,cast,static_blk);
			tfree(static_blk->in.left);
			static_blk->in.op = FREE;
			static_blk = static_blk->in.right;

			static_blk = buildtree(UNARY MUL,static_blk,(NODE *)0);
			stack_blk = buildtree(UNARY MUL,stack_blk,(NODE *)0);
#ifdef C1_C
			/* emit c1 symbol table information (again).  The
			 * first time we didn't know the array size or offset
			 */
			putsym(C1OREG,14,id);
			p2word(id->offset/SZCHAR);
			putc1attr(id,0);
			p2name(id->sname);
			
#endif
			}
		p = buildtree(ASSIGN,stack_blk,static_blk);
		ecomp(p);
		tfree(p);
		}

#endif /*ANSI*/

#else /* IRIF */
	ir_term_init();
#endif /* IRIF */

	typ_initialize = 0;
	iclass = SNULL;

	}

#if !defined(ANSI) && !defined(HAIL)
doinit( p ) register NODE *p; {

	/* take care of generating a value for the initializer p */
	/* inoff has the current offset (last bit written)
		in the current word being generated */

	register NODE *pleft;
#ifndef IRIF
	register sz;
#endif /* IRIF */
	register char liclass = iclass;		/* for better code generation */
	int d, s;
	TWORD t;

	/* note: size of an individual initializer is assumed to fit into an int
		 (except for initialized doubles)
	*/

	if( liclass <= 0 ) goto leave;	/* SNULL (==0) is also inappropriate */
	if( liclass == EXTERN || liclass == UNAME ){
		/* "identifiers with %s should not be initialized " */
		UERROR( MESSAGE( 19 ), ( liclass == EXTERN ) ?
		                                "'extern' storage class" : 
		                                "'union' type" );
		iclass = EXT_INIT_ERR;
		goto leave;
		}

#ifndef IRIF
	if( liclass == AUTO || liclass == REGISTER ){
		/* do the initialization and get out, without regard 
		    for filing out the variable with zeros, etc. */
		bccode();
		idname = pstk->in_id;
		if (pstk->in_n++) UERROR( MESSAGE( 25 ));
		p = buildtree( ASSIGN, buildtree( NAME, NIL, NIL ), p );
#ifdef	C1_C
		if ((p->in.type==STRTY)||(p->in.type==UNIONTY))
			/* tree becomes U*(STRASG(id,&p)) */
			fixtag(p->in.left->in.left);
#endif	/* C1_C */
		ecomp(p);
		return;
		}
#endif /* not IRIF */

	if( p == NIL ) return;  /* for throwing away strings that have been turned into lists */

#ifdef IRIF
	     if( iclass == AUTO || iclass == REGISTER ){
		  p = doinit1( p );
	     }
#endif /* IRIF */

	if( ibseen > 0 ){   /* SWFfc00674 fix */
		/* "} expected" */
		UERROR( MESSAGE( 122 ) );
		goto leave;
		}

# ifdef DEBUGGING
	if( idebug > 1 ) printf( "doinit(0x%x)\n", p );
# endif

	t = pstk->in_t;  /* type required */
	d = pstk->in_d;
	s = pstk->in_s;

#ifndef IRIF
	if( pstk->in_sz < 0 ){  /* bit field */
		sz = -pstk->in_sz;
		}
	else {
		sz = tsize( t, d, s, NULL, 0 ); /* SWFfc00726 fix */
		}

	if (inoff && !pstk->in_off) {
	     /* "incorrect initialization or too many initializers" */
	     UERROR( MESSAGE( 25 ));
	     goto leave;
	}

	inforce( pstk->in_off );

	pleft = block(NAME,NIL,NIL,t,d,s);
	pleft->in.tattrib =pstk->in_tattrib;
	p = buildtree( ASSIGN, pleft, p);
	p->in.left->in.op = FREE;
	p->in.left = p->in.right;
	p->in.right = NIL;
	p->in.left = pleft = optim( p->in.left );
	if( pleft->in.op == UNARY AND ){
		pleft->in.op = FREE;
		p->in.left = pleft = pleft->in.left;
		}
	p->in.op = INIT;

	if( sz < SZINT ){ /* special case: bit fields, etc. */
		/* "incorrect initialization" */
		if( pleft->in.op != ICON ) UERROR( MESSAGE( 61 ) );
		else 
		    {
                        int upper = pleft->tn.lval >> sz;
                        incode( pleft, sz );
                        if (upper != 0 && upper != -1)
                                /* constant too large for field */
                                WERROR( MESSAGE( 225 ) );
                    }
		}
	else if( pleft->in.op == FCON ){
		fincode( pleft->fpn.dval, sz );
		}
	else if ( (liclass==EXTDEF || liclass==STATIC)
		&&  pleft->in.op!=ICON && pleft->in.op!=NAME )
		{
		/* "incorrect initialization" */
		UERROR( MESSAGE( 61 ) );
		goto leave;
		}
	else {
		cinit( optim(p), sz );
		}

	gotscal();

#else /* IRIF defined */
	p = doinit2( p, block(NAME,NIL,NIL,t,d,s) );
#endif /* IRIF */

leave:
	tfree(p);
	}
#else /*ANSI*/
doinit( p ) register NODE *p; {

	/* take care of generating a value for the initializer p */
	/* inoff has the current offset (last bit written)
		in the current word being generated */

	register NODE *pleft;
	register char liclass = iclass;		/* for better code generation */
	int d, s;
	TWORD t;
#ifndef IRIF
	register sz;
#endif /* not IRIF */

	/* note: size of an individual initializer is assumed to fit into an int
		 (except for initialized doubles)
	*/

	if( flush_to_RC ) goto leave;

	if( liclass <= 0 ) goto leave;	/* SNULL (==0) is also inappropriate */
	if( liclass == EXTERN ){
		/* "cannot initialize extern" */
		UERROR( MESSAGE( 42 ) );
		iclass = EXT_INIT_ERR;
		goto leave;
		}

#ifndef IRIF
	if( liclass == AUTO || liclass == REGISTER ){
		bccode();
		if (aggregate_expr==-1)
			setup_aggregate_initialization();
		if (aggregate_expr==1) {
			if (instack[0].in_n++)
			     /* "incorrect initialization or too many initializers" */
			     UERROR( MESSAGE( 25 ));
			idname = instack[0].in_id;
			p=buildtree(ASSIGN,buildtree(NAME,NIL,NIL),p);
#ifdef	C1_C
			if ((p->in.type==STRTY)||(p->in.type==UNIONTY))
				/* tree becomes U*(STRASG(id,&p)) */
				fixtag(p->in.left->in.left);
#endif	/* C1_C */
			ecomp(p);
			return;
			}
	   }
#endif /* not IRIF */

	if( p == NIL ) return;  /* for throwing away strings that have been turned into lists */
   
#ifdef IRIF
	     if( iclass == AUTO || iclass == REGISTER ){
		  p = doinit1( p );
	     }
#endif /* IRIF */

	if( ibseen > 0 ){
	        flush_to_RC = 1;
		/* "excess values in initializer ignored" */
		WERROR( MESSAGE( 175 ) );
		goto leave;
		}

# ifdef DEBUGGING
	if( idebug > 1 ) printf( "doinit(0x%x)\n", p );
# endif

	t = pstk->in_t;  /* type required */
	d = pstk->in_d;
	s = pstk->in_s;

#ifndef IRIF
	if( pstk->in_sz < 0 ){  /* bit field */
		sz = -pstk->in_sz;
		}
	else {
		sz = tsize( t, d, s, (char *)NULL, 0 ); /* SWFfc00726 fix */
	   }

	if (inoff && !pstk->in_off) {
	     /* "incorrect initialization or too many initializers" */
	     UERROR( MESSAGE( 25 ));
	     goto leave;
	}
	inforce( pstk->in_off );

	pleft = block(NAME,NIL,NIL,t,d,s);
	pleft->in.tattrib = pstk->in_tattrib;
	p = buildtree( ASSIGN, pleft, p);
	p->in.left->in.op = FREE;
	p->in.left = p->in.right;
	p->in.right = NIL;

	walkf(p->in.left,rm_paren);
	p->in.left = pleft = optim( p->in.left );

	if( pleft->in.op == UNARY AND ){
	     pleft->in.op = FREE;
	     p->in.left = pleft = pleft->in.left;
	}

	p->in.op = INIT;

	if( sz < SZINT ){ /* special case: bit fields, etc. */
	     /* "incorrect initialization" */
	     if( pleft->in.op != ICON ) UERROR( MESSAGE( 61 ) );
	     else if (!ISUNSIGNED(p->in.type)) {
		  int mask = (~((1<<sz)-1))>>1;
		  incode( pleft, sz );
		  if ((pleft->tn.lval&mask)&&(~pleft->tn.lval&mask))
		       /* constant too large for field */
		       WERROR( MESSAGE( 225 ) );
	     } else {
		  int upper = pleft->tn.lval >> sz;
		  incode( pleft, sz );
		  if (upper != 0 && upper != -1)
		       /* constant too large for field */
		       WERROR( MESSAGE( 225 ) );
	     }
	}
#ifdef QUADC
	else if( pleft->in.op == FCON ){
		fincode( pleft->qfpn.qval, sz );
		}
#else
	else if( pleft->in.op == FCON ){
		fincode(pleft->fpn.dval,((sz == SZLONGDOUBLE) ? SZDOUBLE : sz));
		}
#endif /* QUADC */
	else {
	     if ( (liclass==EXTDEF || liclass==STATIC)
		 &&  (pleft->in.op!=ICON && pleft->in.op!= NAME) ||
		 (
		  p = optim(p),
		  p->in.left->in.op != ICON)) {

		  /* "incorrect initialization" */
		  UERROR( MESSAGE( 61 ) );
		  goto leave;
	     }
	     else {
		  cinit( p, sz );
	     }
	}
	gotscal();

#else /* IRIF defined */
	p = doinit2( p, block(NAME,NIL,NIL,t,d,s) );
#endif /* IRIF */

leave:
	tfree(p);
	}
#endif /*ANSI*/

#ifdef IRIF

int is_addrconst( p ) NODE *p; {

     /* verifies 'p' contains no nodes to prevent it from
      * being part of address constant
      *
      * returns '0' if it just kenna be
      */

     switch( p->in.op ) {

	case ICON:
	case NAME:
	case PADDICON:
	  return( 1 );

	case STREF:
	  if( !is_addrconst( p->in.left )) return( 0 );
	  /* fall thru */
	  
	case DOT:
	  return( is_addrconst( p->in.right ));

	case UNARY AND:
	  return( is_addrconst( p->in.left ));

	default:
	  return( 0 );
     }
}

static NODE *doinit1( p ) NODE *p; {

     if( p->in.type == STRTY || p->in.type == UNIONTY ) {
	  /* setup for struct/union assignment */
	  pstk = instack;
     }
     idname = instack[0].in_id;
     if( ISARY( idname->stype )  ||
	( idname->stype == STRTY && p->in.type != STRTY )  ||
	( idname->stype == UNIONTY && p->in.type != UNIONTY )) {
	  /*
	   * initializers must be constant expressions
	   */
	  walkf( p, rm_paren );  /* removing PARENs is ok - e.g,
				  * 
				  *   double d[] = { (1.+2.)+(3.+4.) }
				  *
				  * gets folded correctly by 'optim()'
				  */
	  p = optim( p );
	  if( !CONSTANT_NODE( p )) {
	       /* "incorrect initialization" */
	       UERROR( MESSAGE( 61 ) );
	  }
     }
#ifdef HPCDB
     if( cdbflag ) (void)sltnormal(); 
#endif /* HPCDB */ 
     return( p );
}

static NODE *doinit2( p, pleft ) NODE *p, *pleft; {

     pleft->in.tattrib = pstk->in_tattrib;
     p = buildtree( ASSIGN, pleft, p);
     p->in.op = p->in.left->in.op = FREE;
     p = do_optim( p->in.right );
     
     if( pstk->in_id->sclass & FIELD ) {
	  /* bitfield checks */
	  int sz = pstk->in_id->sclass & FLDSIZ;

	  if( sz < SZINT ) {
#ifdef ANSI
	       if ( !ISUNSIGNED( p->in.type )) {
		    int mask = ( ~(( 1<<sz ) -1 ) ) >> 1;
		    if (( p->tn.lval&mask )&& ( ~p->tn.lval&mask ))
			 /* constant too large for field */
			 WERROR( MESSAGE( 225 ) );
	       }
	       else {
#endif /* ANSI */
		    int upper = p->tn.lval >> sz;
		    if (upper != 0 && upper != -1)
			 /* constant too large for field */
			 WERROR( MESSAGE( 225 ) );
#ifdef ANSI
	       }
#endif /* ANSI */
	  }
     }

     if( doinit3( p )) {
	  /* "incorrect initialization" */
	  UERROR( MESSAGE( 61 ) );
     }
     else {
	  ir_init( pstk->in_id, p, instack[0].in_id, initializer_offset(),
		  type_size(pstk->in_t,pstk->in_s,pstk->in_d),
		  pstk != &instack[0] );
	  initializations++;
	  gotscal();
     }
     
     return( p );
}
  
int doinit3( p ) NODE *p; {
     /* verifies 'p' is up to snuff;
      * returns '0' if ok, '1' otherwise
      */
     
     NODE *pleft = p->in.left;

     switch( p->in.op ) {
	  
	case ICON:
	case FCON:
	case PADDICON:
	case PSUBICON:
	  return( 0 );

	case UNARY AND:
	  
	  if( ISARY( pstk->in_id->stype ) 
	     || pstk->in_id->sclass == MOS
	     || pstk->in_id->sclass == MOU ) {
	       
	       /* '&foo' used to initialized part of a structured
		* data type.  Since sdtypes are initialized statically,
		* need to make sure 'foo' is not a local name
		*/
	       if( pleft->in.op == NAME 
		  && ( pleft->nn.sym->sclass == AUTO
		      || pleft->nn.sym->sclass == REGISTER )) {
		    
		    return( 1 );
	       }
	  }
	  if( pstk->in_id->sclass != AUTO 
	     && pstk->in_id->sclass != REGISTER ) {
	       
	       /* address expression must be constant */
	       if( pleft->in.op != STRING && !is_addrconst( pleft )) {
		    return( 1 );
	       }
	  }
	  return( 0 );

	case SCONV:
	case PCONV:
	  return( doinit3( pleft ));

	default:

	  if( pstk->in_id->sclass == EXTERN 
	     || pstk->in_id->sclass == EXTDEF
	     || pstk->in_id->sclass == STATIC ) {

	       return( 1 );
	  }
	  return( 0 );

     } /* end switch */
}

#endif /* IRIF */

void gotscal(){
	register struct symtab *p;
	register struct instk *lpstk;

	for( ; pstk > instack; ) {

        /* SWFfc00674 fix - If the psuedo brace flag is set then don't 
                            increment the brace flag: ibseen           */
		if( pstk->in_fl == 1 ) ++ibseen;

		lpstk = --pstk;
        /* SWFfc00674 fix - Added a psuedo brace value if the type is STRUC,
                            UNION, or ARRAY and the left brace flag in_fl is 0.
                            This solves the problem when braces are left out 
                            and the ANSI compiler is invoked*/
#ifdef ANSI
		if((lpstk->in_t == STRTY || lpstk->in_t == UNIONTY || 
                    ISARY(lpstk->in_t)) && (!pstk->in_fl)) pstk->in_fl = 255;
#endif /* ANSI */

#ifdef IRIF
		if( lpstk->in_t == UNIONTY ){
		     ir_term_sdtype( lpstk->in_id );
		}
#endif /* IRIF */
		if( lpstk->in_t == STRTY ){
			register struct symtab *id;
			if( (id=(struct symtab *) dimtab[++lpstk->in_x]) == NULL ) 
				{
#ifdef IRIF
				     ir_term_sdtype( lpstk->in_id );
#endif /* IRIF */
				     continue;
				}

			/* otherwise, put next element on the stack */
			p = id;
			instk( id, p->stype, p->sattrib, p->dimoff, p->sizoff,
				(OFFSZ)(p->offset+lpstk->in_off) );
			return;
			}
		else if( ISARY(lpstk->in_t) ){
			register int n = ++lpstk->in_n;
#ifndef IRIF
			if( n >= dimtab[lpstk->in_d] && lpstk > instack) {
			     continue;
			}
#else /* IRIF */
			if( n >= dimtab[lpstk->in_d] && lpstk > instack ) {

			     if( lpstk->in_id != (lpstk-1)->in_id ) {
				  /* really finished the entire array - 
				   * not just one of many dimensions
				   */
				  ir_term_element( 1 );
				  ir_term_sdtype( lpstk->in_id );
			     }
			     continue;
			}
			else {
			     if( lpstk == instack && dimtab[lpstk->in_d] == 0 )
				  initializations = 0;
			     if( irbrace_parent ) {
				  /* determine number of array elements skipped by 
				   * inner brace and 'ir_term_element()' them.  
				   */
				  struct instk *temp = pstk;
				  int mult = 1;
				  int total = 0;
				  
				  for( ; ISARY( temp->in_t ); temp++ );
				  for( temp-- ; temp > pstk ; temp-- ) {
				       total += mult * 
					    ( dimtab[ temp->in_d ] - temp->in_n - 1 );
				       mult *= dimtab[ temp->in_d ];
				  }
				  ir_term_element( ++total );
			     }
			     else ir_term_element( 1 );
			}
#endif /* IRIF */
			/* put the new element onto the stack */
			instk( lpstk->in_id, (TWORD)DECREF(lpstk->in_t),
			      (TWORD)(DECREF(lpstk->in_tattrib)),
				lpstk->in_d+1, lpstk->in_s, 
				(OFFSZ)(lpstk->in_off+n*lpstk->in_sz) );
			return;
			}

		}

	}

#if !defined(ANSI) && !defined(HAIL)
ilbrace(){ /* process an initializer's left brace */
	register TWORD t;
	register struct instk *temp;

	temp = pstk;

	for( ; temp >= instack; --temp ){

		t = temp->in_t;
		if( t != STRTY && !ISARY(t) ) continue; /* not an aggregate */
		if( temp->in_fl ){ /* already associated with a { */
			/* "incorrect {" */
			if( temp->in_n ) UERROR( MESSAGE( 74 ) );
			continue;
			}

		/* we have one ... */
		temp->in_fl = 1;
		break;
		}

	/* cannot find one */
	/* ignore such right braces */

	}


irbrace(){
	/* called when a '}' is seen */

# ifdef DEBUGGING
	if( idebug ) printf( "irbrace(): paramno = %x on entry\n", paramno );
# endif

	if( ibseen > 0) {  /* SWFfc00674 fix */
		--ibseen;
		return;
		}

	for( ; pstk > instack; --pstk ){
		if( pstk->in_fl != 1 ) continue; /* SWFfc00674 fix */

		/* we have one now */

		pstk->in_fl = 0;  /* cancel { */
#ifdef IRIF
		if( pstk->in_t == STRTY || 
		         ( ISARY( pstk->in_t ) && !ISARY( (pstk-1)->in_t ))) {
		     ir_term_sdtype( pstk->in_id );
		}
#endif /* IRIF */
		gotscal();  /* take it away... */
		return;
		}

	/* these right braces match ignored left braces: throw out */

	}
#else /* ANSI defined */
ilbrace(){ /* process an initializer's left brace */
	register TWORD t;
	register struct instk *temp;

	for (temp=instack;temp<pstk;temp++){
		t = temp->in_t;
		if (t != STRTY && t != UNIONTY && !ISARY(t)) continue; /* not an aggregate */
		if (!temp->in_fl && !temp->in_n){
			temp->in_fl = 1;
			return;
			}
		}
	/* allow one optional brace on scalars */
	if (!instack[0].in_fl) instack[0].in_fl = 1;
	else 
	     /* "incorrect {" */
	     UERROR(MESSAGE(74));
	}

irbrace(){
	/* called when a '}' is seen */

# ifdef DEBUGGING
	if( idebug ) printf( "irbrace(): paramno = %x on entry\n", paramno );
# endif

	flush_to_RC = 0;
	if( ibseen > 0) {   /* SWFfc00674 fix */
		--ibseen;
		return;
		}

	for( ; pstk > instack; --pstk ){
		if( pstk->in_fl != 1 ) continue; /* SWFfc00674 fix */

		/* we have one now */

		pstk->in_fl = 0;  /* cancel { */
#ifdef IRIF
		if( pstk->in_t == STRTY || 
		         ( ISARY( pstk->in_t ) && !ISARY( (pstk-1)->in_t ))) {
		     ir_term_sdtype( pstk->in_id );
		}
		irbrace_parent++;
		gotscal();  /* take it away... */
		irbrace_parent = 0;
#else /* not IRIF */
		gotscal();  /* take it away... */
#endif /* IRIF */
		return;
		}

	/* these right braces match ignored left braces: throw out */

	}
#endif

upoff( size, alignment, poff ) unsigned size; 
				register unsigned alignment; 
				register *poff; {
	/* update the offset pointed to by poff; return the
	/* offset of a value of size `size', alignment `alignment',
	/* given that off is increasing */

	unsigned register off;

	off = *poff;
	SETOFF( off, alignment );

#ifndef IRIF
        if( (offsz-off) <=  size ){
                if( instruct!=INSTRUCT )cerror("too many local variables");
                else cerror("Structure too large");
                }
#endif /* not IRIF */
	*poff = off+size;
	return( off );
	}


#ifndef IRIF

oalloc( p, poff )
register struct symtab *p; 
register *poff;
{
	/* allocate p with offset *poff, and update *poff */
	register unsigned al, off, tsz;
	unsigned int noff;

	al = talign( p->stype, p->sizoff, p->sclass==AUTO || p->sclass==STATIC);
	noff = off = *poff;
	tsz = tsize( p->stype, p->dimoff, p->sizoff, p->sname, 0 ); /* SWFfc00726 fix */
#ifdef ANSI
	/* without a prototype, FLOAT's will now have type FLOAT (instaead
	 * of DOUBLE), but we will still allocate size of a double.
	 */
	if ((p->sflags&SFARGID) && (p->stype==FLOAT)) {
		tsz = SZDOUBLE;
		}
#endif
	if( p->sclass == AUTO ){
		if( tsize_overflow || ((offsz-off) <= tsz))
		    cerror("too many local variables");
		noff = off + tsz;
		SETOFF( noff, al );
		off = -noff;
		}
	else
		if( (p->sclass==PARAM || p->sclass==REGISTER) && ( tsz < SZINT ) ){
			off = upoff( p->stype==STRTY || p->stype==UNIONTY? 
				tsz:(unsigned)SZINT, (unsigned)ALINT, (int *)&noff );
			/* structures passed by value are the only thing pushed onto
			   the stack that don't take at least 32 bits of stack space.
			*/
			off = noff - tsz;
			}
		else
		{
		off = upoff( tsz, al, (int *)&noff );
		}

	if( p->sclass != REGISTER ){ /* in case we are allocating stack space for register arguments */
		if( p->offset == NOOFFSET ) p->offset = off;
		else if( off != p->offset ) return(1);
		}

	*poff = noff;
	return(0);
	}

falloc( p, w, new, pty )  
register struct symtab *p; 
register NODE *pty; 
register int new, w;
{
	/* allocate a field of width w */
	/* new is 0 if new entry, 1 if redefinition, -1 if alignment */

	register al,sz,type;
	char inunion = instruct&INUNION;

	type = (new<0)? pty->in.type : p->stype;

	if (apollo_align || (align_like == ALIGNCOMMON))
	  { /* treat all fields as ints in DOMAIN and NATURAL */
	  al = al_int[align_like];
	  sz = SZINT;
	  type = UNSIGNED;
	  }
	else
	  switch( type )
	    {
	    case ENUMTY:
		{
		int s;

		s = new<0 ? pty->fn.csiz : p->sizoff;
		al = dimtab[s+2];
		sz = dimtab[s];
		break;
		}
	    case CHAR:
	    case UCHAR:
#ifdef ANSI
	    case SCHAR:
#endif
                al = al_char[align_like];
		sz = SZCHAR;
		break;

	    case SHORT:
	    case USHORT:
                al = al_short[align_like];
		sz = SZSHORT;
		break;

	    case INT:
	    case UNSIGNED:
                al = al_int[align_like];
		sz = SZINT;
		break;
#ifdef ANSI
	    case LONG:
	    case ULONG:
                al = al_long[align_like];
		sz = SZLONG;
		break;
#endif
	    default:
		if( new < 0 )
		  {
		  /* "incorrect field type" */
		  UERROR( MESSAGE( 57 ) );
		  return(1);
		  }
		else
		  {
		  al = fldal( p->stype );
		  sz =SZINT;
		  }
	    } /* switch */

	if( w > sz )
	  {
	  if( ( p->sname == NULL ) || ( new < 0 ))
	  /* "bitfield size of '%d' is out of range */
	    UERROR( MESSAGE( 56 ), w );
	  else
	  /* "bitfield size of '%d' for '%s' is out of range" */
	    UERROR( MESSAGE( 64 ), w, p->sname ); 
	  w = sz;
	  }

	if( w == 0 )
	  { /* align only */
	  if (!inunion)
		SETOFF( strucoff, al );
	   /* "zero sized field: %s" */
	  if( new >= 0 ) UERROR( MESSAGE( 184 ), p->sname );
	  return(0);
	  }

	if( strucoff%al + w > sz && !inunion)
	  SETOFF( strucoff, al );
	if( new < 0 )
	  {
	  if( (offsz-strucoff) <= w )
		cerror("structure too large");
	  if (!inunion)
		strucoff += w;  /* we know it will fit */
	  return(0);
	  }

	/* establish the field */

	if( new == 1 )
	  { /* previous definition */
	  if( p->offset != strucoff || p->sclass != (FIELD|w) ) return(1);
	  }
	p->offset = strucoff;
	if( (offsz-strucoff) <= w ) cerror("structure too large");
	if (!inunion)
	  strucoff += w;
	p->stype = type;
#ifdef LINT_TRY
	fldty(p);
#endif
	return(0);
	}

#else /* IRIF */

falloc( p, w, new, pty )  
register struct symtab *p; 
register NODE *pty; 
register int new, w;
{
        int type;
	/* allocate a field of width w */
	/* new is 0 if new entry, -1 if alignment */

	/* treat all fields as ints in DOMAIN and NATURAL */
	if( apollo_align || (align_like == ALIGNCOMMON)) {
	     type = UNSIGNED;
	}
	else {
	     type = (new<0)? pty->in.type : p->stype ;

	     switch( type ) {
		      
		case ENUMTY:
		case CHAR:
		case UCHAR:
		case SCHAR:
		case SHORT:
		case USHORT:
		case INT:
		case UNSIGNED:
		case LONG:
		case ULONG:
		  break;
		  
		default:
		  /* "incorrect field type" */
		  UERROR( MESSAGE( 57 ) );
		  return;
	     }
	}
	switch( new ) {

	   case -1: 
	     /* unnamed bitfield */

	     if( ir_alloc_unnamed_field( type, w, 
					instruct & INSTRUCT ? STRTY : UNIONTY )
		== FE_OVERFLOW ) {
		  /* "bitfield size of '%d' is out of range */
		  UERROR( MESSAGE( 56 ), w );
		  return;
	     }
	     break;

	   case 0:
	     /* new entry */
	     p->stype = type;
	     switch( ir_alloc_member( p,
				     instruct & INSTRUCT ? STRTY : UNIONTY )) {
		case FE_ZERO_SIZE:
		  /* "zero sized field: %s" */
		  UERROR( MESSAGE( 184 ), p->sname );
		  return;
		  
		case FE_OVERFLOW:
		  /* "bitfield size of '%d' for '%s' is out of range" */
		  UERROR( MESSAGE( 64 ), w, p->sname ); 
		  return;
	     }
	     break;

	} /* end switch( new ) */

	return;
   }

#endif /* IRIF */

nidcl( p ) NODE *p; { /* handle uninitialized declarations */
	/* assumed to be not functions */
	register char class;

	commflag = 0;

	/* compute class */
	if( (class=curclass) == SNULL ){
		if( blevel > 1 ) class = AUTO;
		else if( blevel != 0 || instruct ) cerror( "nidcl error" );
		else { /* blevel = 0 */
			class = EXTERN;
			if( p->nn.sym->sclass != EXTDEF )
			     /* file scope tentative defn of the form 'int i;'
			      * [no storage class]
			      */
			     commflag = 1;
# ifdef HPCDB
			if (cdbflag && p->nn.sym) {
				/* set defined flag in cdb_info field of
				 * symtab entry.  This is needed to distinguish
				 * uninitalized globals from 'externs' since
				 * both have storage class EXTERN.
				 */
				p->nn.sym->cdb_info.info.extvar_isdefined = 1;
				}
# endif
		   }
	   }

	defid( p, class );

	switch( class ) {

	   case STATIC:
	     if( blevel == 0 ) {
		  /* set flag to ensure initialization later [to permit an 
		     intervining definition to initialize] */
		  must_check_globals = 1;
		  break;
	     }
	     /*fall thru*/
	   case EXTDEF:
	     /* simulate initialization by 0 */
	     beginit(p->nn.sym,FALSE);
	     endinit();
	     break;

	}
#ifndef IRIF		     
	if( commflag ) commdec( p->nn.sym );
#endif /* not IRIF */
	}

NODE *types (p1,p2,p3,p4,p5,p6) NODE *p1,*p2,*p3,*p4,*p5,*p6; {
	/* set the type and class fields of p1 from types p1-p6 */
	NODE *p[6];
	int tnum = 0;
	int cflg = 0;	/* number of storages classes given */
	int qflg = 0;   /* signals repetitive type qualifiers */
	int class = 0;  /* storage class */
	int sflag = 0;	/* struct/union/enum flag */
	int unsg = INT;  /* INT or UNSIGNED */
	int noun = UNDEF;  /* INT, CHAR, FLOAT or DOUBLE */
	int adj = INT;  /* INT, LONG, or SHORT */
	TWORD rtype = 0;	/* result type */
	TWORD rattrib = 0;	/* result attribute */
	register i,t;
	int rsiz = 0;   /* structure size */
	int rdim = 0;   /* dimension table */
	struct symtab *rsym = 0; /* struct symbol table entry */
        int typedefsym = 0; /* typedef name symbol table entry */
	flag qualifier_seen = 0;
	int const_mask = ATTR_CON;
	int vol_mask = ATTR_VOL;
	if (p1) {
		if (p1->in.op == CLASS)	{
		     cflg++;
		     class = (int) p1->in.left;
		   }
		else p[tnum++] = p1;
	}
	if (p2) {
		if (p2->in.op == CLASS) {
		     cflg++;
		     class = (int) p2->in.left;
		     p2->in.op = FREE;
		   }
		else p[tnum++] = p2;
	}
	if (p3) {
		if (p3->in.op == CLASS) {
		     cflg++;
		     class = (int) p3->in.left;
		     p3->in.op = FREE;
		   }
		else p[tnum++] = p3;
	}
	if (p4) {
		if (p4->in.op == CLASS) {
		     cflg++;
		     class = (int) p4->in.left;
		     p4->in.op = FREE;
		   }
		else p[tnum++] = p4;
	}
	if (p5) {
		if (p5->in.op == CLASS) {
		     cflg++;
		     class = (int) p5->in.left;
		     p5->in.op = FREE;
		   }
		else p[tnum++] = p5;
	}
	if (p6) {
		if (p6->in.op == CLASS) {
		     cflg++;
		     class = (int) p6->in.left;
		     p6->in.op = FREE;
		   }
		else p[tnum++] = p6;
	}
	for (i=0;i<tnum;i++) {
		/* Grab the attributes from typedef declarations */
		if (rattrib  & p[i]->in.tattrib) qflg = 1;
		rattrib |= p[i]->in.tattrib;
#if defined(HPCDB) || defined(APEX)
		if ((p[i])->in.op == TYPE_DEF)
			typedefsym = (p[i])->in.nameunused;
#endif
		/* now, free the node */
		(p[i])->in.op = FREE;
		switch((t=(p[i])->in.type)) {
		default:	/* How to get here?
				 *   - internal errors
				 *   - using typedef
				 *     e.g. typedef int A[2][3];
				 *          const A x;
				 * ignore the internal errors, but try to
				 * catch the typedef case.
				 */
		                rtype = t;
				rsiz = p[i]->fn.csiz;
				rdim = p[i]->fn.cdim;
				rsym = p[i]->nn.sym;
				if (sflag++ > 0) goto illegaltype;
				break;
		case CONST:	if( !qualifier_seen ) {
		                     /* first qualifier; cycle through
				      * everybody to see what the qualifier
				      * modifies: base type iff no nonbasic
				      * types are present
				      */
		                     int j;
		                     qualifier_seen = 1;
				     for( j=0; j<tnum; j++ ) {
					  if( p[j]->in.type & ~BTMASK ) {
					       const_mask = PATTR_CON;
					       vol_mask = PATTR_VOL;
					       break;
					  }
				     }
				}
				     
				if (rattrib & const_mask)
		                        qflg = 1;
		                rattrib |= const_mask;
				break;
		case VOLATILE:	if( !qualifier_seen ) {
		                     /* first qualifier; cycle through
				      * everybody to see what the qualifier
				      * modifies: base type iff no nonbasic
				      * types are present
				      */
		                     int j;
		                     qualifier_seen = 1;
				     for( j=0; j<tnum; j++ ) {
					  if( p[j]->in.type & ~BTMASK ) {
					       const_mask = PATTR_CON;
					       vol_mask = PATTR_VOL;
					       break;
					  }
				     }
				}
				     
				if (rattrib & vol_mask)
					qflg = 1;
				rattrib |= vol_mask;
				break;
		case UNSIGNED:
		case SIGNED:	if (unsg != INT) goto illegaltype;
				unsg = t;
				break;
		case SHORT:
		case LONG:	if (adj != INT) goto illegaltype;
				adj = t;
				continue;
		case INT:
				if (p[i]->in.tattrib&ATTR_ENUM){
				    rsiz = p[i]->fn.csiz;
				    rdim = p[i]->fn.cdim;
				}
				/* fall through */
		case CHAR:
		case FLOAT:
		case DOUBLE:	
				if (noun != UNDEF) goto illegaltype;
				noun = t;
				continue;
#ifdef ANSI

		case SCHAR:	if (noun != UNDEF) goto illegaltype;
				if (unsg != INT) goto illegaltype;
				noun = CHAR;
				unsg = SIGNED;
				continue;

		case LONGDOUBLE:if (noun != UNDEF) goto illegaltype;
				if (adj != INT) goto illegaltype;
				adj = LONG;
				noun = DOUBLE;
				continue;
#endif
		case STRTY:
		case UNIONTY:
		case ENUMTY:	rtype = t;
		                rsiz = p[i]->fn.csiz;
				rdim = p[i]->fn.cdim;
				rsym = p[i]->nn.sym;
		  		if (sflag++ > 0) goto illegaltype;
		}
	}
	if (sflag)
		{
#ifdef SIZED_ENUMS
		if ((rtype == ENUMTY)&&(((adj == SHORT)||(adj == LONG))||(noun == CHAR))){
			if ((unsg != INT)||((noun != UNDEF)&&(noun != CHAR))) goto illegaltype;
			p1->fn.csiz = rsiz;
			p1->fn.cdim = rdim;
			p1->nn.sym = rsym;
			if (qflg || (cflg > 1)) goto illegaltype2;
			was_class_set = cflg;
			p1->in.op = TYPE;
			p1->in.type = (noun==CHAR)?CHAR:adj;
			p1->in.tattrib = rattrib|ATTR_ENUM;
#ifdef HPCDB
			p1->in.nameunused = typedefsym;
#endif
			return(p1);
		} else
#endif /* SIZED_ENUMS */
		if ((noun != UNDEF) || (unsg != INT) || (adj != INT))
			goto illegaltype;
		p1->fn.csiz = rsiz;
		p1->fn.cdim = rdim;
		p1->nn.sym = rsym;
		goto checkflags;
		
		}
	switch (noun) {
		case UNDEF:
		case INT:	noun = INT;
				if (adj != INT) noun = adj;
				rtype = (unsg != UNSIGNED) ? noun : noun + (UNSIGNED-INT);
		  		if (rattrib&ATTR_ENUM){
				    p1->fn.csiz = rsiz;
				    p1->fn.cdim = rdim;
				}
       				break;
		case CHAR:	if (adj != INT) goto illegaltype;
				if (unsg == INT) rtype = CHAR;
#ifdef ANSI
				else if (unsg == SIGNED) rtype = SCHAR;
#else
				else if (unsg == SIGNED) rtype = CHAR;
#endif
				else rtype = UCHAR;
				break;
		case FLOAT:	if (unsg != INT || adj == SHORT) goto illegaltype;
				rtype = (adj == INT) ? FLOAT :
				     ( /* "'long float' type supported in 
					   compatibility (non-ANSI) mode only" */
#ifdef ANSI
				      UERROR( MESSAGE( 210 )),
#else
#ifdef LINT
				      WERROR( MESSAGE( 210 )),
#endif
#endif /* ANSI */
				      DOUBLE );
				break;
		case DOUBLE:	if (unsg != INT || adj == SHORT) goto illegaltype;
#ifdef ANSI
				rtype = (adj == INT) ? DOUBLE : LONGDOUBLE;
#ifdef HAIL
				{
					extern int map_ldouble_to_double;
					if ((rtype == LONGDOUBLE) && !map_ldouble_to_double){
						uerror("The 16 byte quad precision long double type is not yet supported, and will be added in a future release.  As a workaround, use the -map_long_double_to_double option to make this type use the same representation as double.");
					}
				}
#endif /* HAIL */
#else /* not ANSI */
				rtype = DOUBLE;
				if (adj == LONG)
				     /* "long double type only supported for ANSI C" */
#ifndef IRIF
				     WERROR( MESSAGE( 211 ));
#else /* IRIF */
				     UERROR( MESSAGE( 211 ));
#endif /* IRIF */
#endif /* not ANSI */
				if (apollo_align
				    && (instruct&(INSTRUCT|INUNION)))
				  { /* apollo treats long double as double */
				  rtype = DOUBLE;
				  }
				break;
	}
#ifndef ANSI
	/* The following flag is set for the sake of signed bitfields.
	 * Normally bitfields are unsigned, but this should not be the
	 * case if the keyword "signed" is seen */
	type_signed = (unsg == SIGNED);
#endif
	
checkflags:
        if( qflg || ( cflg > 1 )) 
	     goto illegaltype2;

	        was_class_set = cflg;
		p1->in.op = TYPE;
		p1->in.type = rtype;

                if (class == REGISTER) rattrib|=(ATTR_REG<<ATTR_CSHIFT);
		p1->in.tattrib = rattrib;
#if defined(HPCDB) || defined(APEX)
		p1->in.nameunused = typedefsym;
#endif
	        return(p1);

illegaltype:
	        /* "multiple %s in declaration" */
		UERROR( MESSAGE( 172 ), "types");
illegaltype2:
	        if (cflg > 1) 
	             /* "multiple %s in declaration" */
	             UERROR( MESSAGE( 172 ), "storage classes" );
	        if( qflg )
		     /* "multiple %s in declaration" */
	             UERROR( MESSAGE( 172 ), "type qualifiers" );
		p1->in.op = TYPE;
		p1->in.type = INT;
		p1->in.tattrib = 0;
		return(p1);
}

NODE *ntattrib(p1,p2) NODE *p1,*p2; {
	/* return the const/volatile type field */
	TWORD attrib = 0;
	if (p1) 
	    { switch(p1->in.type)
		{
			case CONST:    attrib |= ATTR_CON; break;
			case VOLATILE: attrib |= ATTR_VOL; break;
		}
	    }
		
	if (p2)
	    { 
		switch(p2->in.type)
		{
			case CONST:    attrib |= ATTR_CON; break;
			case VOLATILE: attrib |= ATTR_VOL; break;
		}
		if( !ISCON( attrib ) || !ISVOL( attrib ) ) {
		     /* "multiple %s in declaration" */
		     UERROR( MESSAGE(172), "type qualifiers" );
		}
		p2->in.op = FREE;
	   }

	p1->in.tattrib = attrib;
	return(p1);
      }

NODE *
tymerge( typ, idp ) register NODE *typ, *idp; {
	/* merge type typ with identifier idp  */

	register TWORD t;
	register i;
	int j;
# ifdef ANSI
	long diminfo[MAXTMOD];
	int d;
# endif
# ifdef DEBUGGING
	extern int eprint();
# endif /* DEBUGGING */

	if( typ->in.op != TYPE ) cerror( "tymerge: arg 1" );
	if(idp == NIL ) return( NIL );

# ifdef DEBUGGING
	if( ddebug > 2 ) fwalk( idp, eprint, 0 );
# endif

# ifdef  ANSI
	/* copy any prototypes in the type inherited from "typ". To maintain
	 * the linear, non-nested dimtab layout, this must be done before
	 * the call to "tyreduce" below which will start to put info for this
	 * declaration into the dimtab.
	 */
	for( t=typ->in.type, i=typ->fn.cdim, d=0; t&TMASK; t = DECREF(t) ){
		if (ISARY(t)) diminfo[d++] = dimtab[i++];
		else if (ISFTN(t)) diminfo[d++] =(long) copy_prototype((NODE *)dimtab[i++]);
		}
# endif
	idp->fn.cdim = curdim;
	tyreduce( idp, (TWORD)(typ->in.type&BTMASK), (TWORD)(typ->in.tattrib&BTMASK) );
	/*
	 * handle the type modifiers in typ  -  
	 * install them on the left (innermost modifier)
	 */
        /* after the type has been reduced, put in the pointer to a */
        /* possible corresponding typedef, on non-nameless nodes    */
        if (idp->nn.sym != NULL)
          {
	  /* Note: ANSI calls tymerge even on a redefintion of a type     */
	  /* as a variable, which is syntactically incorrect.  To protect */
	  /* code in cdbsyms from an infinite loop, check here that the   */
	  /* type and variable aren't the same.                           */
#if defined(HPCDB) || defined(APEX)
	  if (idp->nn.sym == (struct symtab *)(typ->in.nameunused))
	    typ->in.nameunused = 0;
          idp->nn.sym->typedefsym = (struct symtab *)(typ->in.nameunused);
#endif
          }
	if( i = ( typ->in.type & ~BTMASK )) {
	     if( ( ( j = ( i << tshift )) >> tshift ) != i ) { 
		  /* "type too complex" */
		  UERROR( MESSAGE( 72 ));
		  return(idp);
	     }
	     idp->in.type |= j ;
	     idp->in.tattrib |= ( ( typ->in.tattrib & ~BTMASK ) << tshift );
	}
	     
	idp->fn.csiz = typ->fn.csiz;

# ifndef ANSI
	for( t=typ->in.type, i=typ->fn.cdim; t&TMASK; t = DECREF(t) ){
		if( ISARY(t) ) dstash( dimtab[i++] );
		}
# else /* ANSI */
	for( i=0; i<d; i++ ) dstash( (int)diminfo[i] );
# endif

	/* now idp is a single node: fix up type */
#ifndef ANSI
	idp->in.type = ctype( idp->in.type );
#endif
/*
 *  both types of tests for ENUM types required below, 'cause for
 *
 *        enum foo { .... } bar;
 *
 *  bar has type ENUMTY.  If later on
 *
 *        enum foo bar2;
 *
 *  bar2 has type INT, with ATTR_ENUM.    sigh.
 */
	if( (t = BTYPE(idp->in.type)) != STRTY && t != UNIONTY && t != ENUMTY
                                           && !ISENUM( t, idp->in.tattrib ) ){
		idp->fn.csiz = t;  /* in case ctype has rewritten things */
		}

	return( idp );
	}

LOCAL tyreduce( p, type, ta ) register NODE *p; TWORD type, ta; {

	/* build a type, and stash away dimensions, 
	 * from a parse tree of the declaration 
	 *
	 * type and ta are assumed to be basic; modifiers handled in tymerge
	 */

	register short o = p->in.op;
	register short tmod;
	register temp;
#ifdef ANSI
	struct symtab * paramlist;
#endif /* ANSI */

	p->in.op = FREE;

	if( o == NAME ) {
	     p->in.type = type;
	     p->in.tattrib = ta;
	     tshift = 0;
	     return;
	}
	tyreduce( p->in.left, type, ta );

	tmod = PTR;
#ifndef ANSI
	if( o == UNARY CALL ) tmod = FTN;
#else /* ANSI */
	if( o == UNARY CALL || o == CALL) {
		tmod = FTN;
		paramlist = (struct symtab *) p->in.right;
		}
#endif /* ANSI */
	else if( o == LB ){
		tmod = ARY;
		temp = p->in.right->tn.lval;
		p->in.right->in.op = FREE;
		}

	if( o == LB ) dstash( temp );
# ifdef ANSI
	/* following added for ANSI -- for correct nesting it must 
	 * come after the recursive call to tyreduce.
	 */
	if (o == UNARY CALL || o == CALL) dstash((int)paramlist);
# else
	if (o == UNARY CALL || o == CALL) dstash(0);
# endif /* ANSI */

	if( LTMASK & p->in.left->in.type ) {
	     /* "type too complex" */
	     UERROR( MESSAGE( 72 ));
	     p->nn.sym = p->in.left->nn.sym;  /* prevents null references */
	     return;
	}
	p->in.type = ( tmod << tshift ) | ( p->in.left->in.type );
	p->nn.sym = p->in.left->nn.sym;
	p->tn.rval = p->in.left->tn.rval;
	if( ISCON( p->in.tattrib )) {
	     tmod = PATTR_CON;
	}
	else if( ISVOL( p->in.tattrib )){
	     tmod = PATTR_VOL;
	}
	else tmod = 0;
	p->in.tattrib = ( tmod << tshift ) | ( p->in.left->in.tattrib );
	tshift += TSHIFT;
	}

LOCAL fixtype( p, class ) register NODE *p; {
	register TWORD t, type;
	register TWORD mod1, mod2;
	register dimcnt;
	/* fix up the types, and check for legality */

	if( (type = p->in.type) == UNDEF ) return;

	if( BTYPE(type) == ENUMTY ) {  /* enumeration types are INT */
#ifndef ANSI
	     if (class&FIELD) type = MODTYPE( p->in.type, UNSIGNED ); else
#endif
	     type = MODTYPE( p->in.type, INT );
	     p->in.tattrib |= ATTR_ENUM;
	}

	mod1 = 0;
	if( mod2 = (type&TMASK) ){
		t = DECREF(type);
		dimcnt = p->fn.cdim;
		while( mod1=mod2, mod2 = (t&TMASK) ){
			if( mod1 == ARY && mod2 == FTN ){
				/* "array of functions is incorrect" */
				UERROR( MESSAGE( 14 ) );
				break;
				}
			else if (mod1 == ARY && mod2 == ARY && dimtab[dimcnt+1]==0)
				{
				/* "invalid null dimension in declaration %s" */
				UERROR( MESSAGE( 126 ), 
				       (p->nn.sym->sname ==  NULL) ?
				            "" : p->nn.sym->sname );
				break;
				}
			else if( mod1 == FTN && ( mod2 == ARY || mod2 == FTN ) ){
			     if( p->nn.sym->sname == NULL ) {
#ifdef ANSI
				  /* "type-name declared as function returning %s" */
				  UERROR( MESSAGE( 47 ),
				       (mod2 == ARY) ? "array" : "function" );
#else
				  /* "incorrect type detected, 'function returning %s'" */
				  cerror("type-name declared as function returning %s",
				       (mod2 == ARY) ? "array" : "function" );
#endif
			     }
			     else {
				  /* "'%s' declared as function returning %s" */
				  UERROR( MESSAGE( 44 ), p->nn.sym->sname,
				       (mod2 == ARY) ? "array" : "function" );
			     }
				break;
				}
			t = DECREF(t);
			if (mod1 == ARY || mod1 == FTN ) dimcnt++;
			}
		}


	if ( type && (type&BTMASK) == VOID && mod1 == ARY)
	     /* "void type incorrect for identifier '%s'" */
	     UERROR( MESSAGE( 117 ), ( p->nn.sym->sname == NULL ?
				       "" : p->nn.sym->sname ));

	/* detect function arguments, watching out for structure declarations */
	/* for example, beware of f(x) struct [ int a[10]; } *x; { ... } */
	/* the danger is that "a" will be converted to a pointer */

	if( class==SNULL && (blevel==1 || inproto>0) && !(instruct&(INSTRUCT|INUNION)) )
		class = PARAM;
	if( class == PARAM || ( class==REGISTER && ( blevel==1 || inproto >0 )) ){
# ifndef ANSI
		if( type == FLOAT && !singleflag ) type = DOUBLE;
		else
# endif /* ANSI */
		     if( ISARY(type) ){
			++p->fn.cdim;
			type += (PTR-ARY);
			p->in.tattrib |= ATTR_WAS_ARRAY;
			}
		else if( ISFTN(type) ){
#ifndef ANSI
			/* "a function is declared as an argument" */
			WERROR( MESSAGE( 11 ) );
#endif
			type = INCREF(type);
			}

		}

	if( instruct && ISFTN(type) ){
		/* "function incorrect in structure or union" */
		UERROR( MESSAGE( 46 ) );
		type = INCREF(type);
		}
	p->in.type = type;
	}

uclass( class ) register class; {
	/* give undefined version of class */
	if( class == SNULL ) return( EXTERN );
	else if( class == STATIC ) {
	     must_check_globals = 1;
	     return( USTATIC );
	   }
	else return( class );
	}

#ifdef ANSI

LOCAL fixclass( class, type, tattrib, sp ) 
register int class;
register TWORD type, tattrib;
struct symtab *sp;

#else

LOCAL fixclass( class, type, tattrib ) 
register int class;
register TWORD type, tattrib;
#endif

{

	/* first, fix null class */

	if( class == SNULL )
		{
		if( instruct&INSTRUCT ) class = MOS;
		else if( instruct&INUNION ) class = MOU;
		else class = (blevel==1 || inproto>0)? PARAM :
			(blevel==0)? EXTDEF : AUTO;
		}

	/* now, do general checking */

	if( ISFTN( type ) ){
		switch( class ) {
#ifdef ANSI
		case USTATIC:
			if( blevel <= 1 ) break;
			/* fall thru */
#endif
		default:
			/* "function storage class incorrect; 
			    'extern' assumed" */
			WERROR( MESSAGE( 45 ) );
			/* fall thru */
			class = EXTERN;
#ifndef ANSI
		case USTATIC:
#endif
		case EXTERN:
		case EXTDEF:
		case TYPEDEF:
		case STATIC:
			break;
#ifdef ANSI
		case WEXTERN:
			/* 'wild card' EXTERN, will match storage class of
			 * any subsequent declaration
			 * i.e., a function used before declared */
			class = EXTERN;
			sp->sflags |= SWEXTERN;
			break;
#endif
			}
		}

	if( class&FIELD ){
	     return( class );
	}

	switch( class ){

	case MOU:
		/* "incorrect class" */
		if( !(instruct&INUNION) ) UERROR( MESSAGE( 52 ) );
		return( class );

	case MOS:
		/* "incorrect class" */
		if( !(instruct&INSTRUCT) ) UERROR( MESSAGE( 52 ) );
		return( class );

	case MOE:
		/* "incorrect class" */
		if( instruct & (INSTRUCT|INUNION) ) UERROR( MESSAGE( 52 ) );
		return( class );

	case REGISTER:
		/* "incorrect register declaration" */
		if( blevel == 0 ) UERROR( MESSAGE( 68 ) );
#ifdef IRIF
		else if( blevel == 1 || inproto>0 ) return( PARAM );
		else if( type == STRTY || type == UNIONTY ) 
		     /* not supporting register unions or structs */
		     return( AUTO );
		else return( REGISTER );
#else /* not IRIF */
		else if ( ISVOL(tattrib) || ISPVOL(tattrib) )
			{
			if( blevel == 1 || inproto>0 ) return( PARAM );
			else return( AUTO );
			}
		else if ( ISPTR(type) )
			{
		  	if( ((regvar>>8)&0377) >= minrvarb && cisreg( type ) )
				return( class );
			}
		else if ( ISFTP(type) )
			{
			if ( ! flibflag )
			  if ( fpaflag )
			    {			/* dragon only flt pt */
			    if ((((fdregvar>>16)&0xFF) >= MINRDVAR) 
					&& cisreg(type))
				return( class );
			    }
			  else
			    {			/* 881 only flt pt */
			    if (((fdregvar&0xFF) >= MINRFVAR) && cisreg(type))
				return( class );
			    }
			}
		else
			{
		  	if( (regvar&0377) >= MINRVAR && cisreg( type ) )
				return( class );
			}
		if( blevel == 1 || inproto>0 ) return( PARAM );
		else return( AUTO );
#endif /* not IRIF */

	case AUTO:
	case CLABEL:
	case ULABEL:
		if( blevel < 2 || inproto>0 ) {
		    /* "incorrect class" */
		    UERROR( MESSAGE( 52 ) );
		    if (blevel == 1 || inproto>0) return ( PARAM );
		}
		return( class );

	case PARAM:
		/* "incorrect class" */
		if( !(blevel == 1 || inproto>0) ) UERROR( MESSAGE( 52 ) );
		return( class );

	case USTATIC:
		must_check_globals = 1;
	case STNAME:
	case UNAME:
	case ENAME:
	case EXTDEF:
		return( class );

	case EXTERN:
	case STATIC:
	case TYPEDEF:
		if (blevel==1 || inproto>0){
		    /* incorrect class */
		    UERROR( MESSAGE(52) );
		    class = PARAM;
		}
		return( class );

	default:
		cerror( "incorrect class: %x", class );
		/* NOTREACHED */

		}
	}




/**********************************************************************
 * SYMBOL TABLE ROUTINES
 *
 * (K. Harris, Sept. 19, 1984)
 * (Adapted for s300, August 1, 1988 by mev)
 * The following routines implement the handling of the symbol table and
 * related structures.
 * Note that significant changes have been made here from the original BELL
 * routines.  The primary difference is that collisions (names that hash
 * to the same value) are handled using a chaining scheme rather than 
 * linear probing.
 *
 * The symbol table is organized as an array (stab) of "symtab" structures.
 * Symtab structures  are linked in two ways: by hash value and by lexical
 * level.  The "lookup" procedure defines the hash function.  Collisions
 * are handled by chaining all entries that hash to the same value into
 * a doubly linked circular list via the "shash_link_forw" and "shash_link_back"
 * fields in the "symtab" structure.  The hash table (stab_hashtab) is an array
 * of headers to these doubly linked lists.  The "hashtab" structure is
 * defined with the "shash_link_..." fields;  these two fields MUST appear
 * as the first two fields of "symtab".  This allows for efficient insert/
 * delete algorithms, without alot of special checks for empty lists, etc.
 *
 * All symbols with the same lexical level (slevel) are linked via the
 * "slev_link" field.  This aids symbol removal and cdb-symbol generation
 * at the end of a lexical scope.
 *
 * A few notes about strategies:
 *  (1) Allocation and freelist. Two mechanisms are maintained for locating
 *	a free symtab entry.  Initially, all entries are free of course.
 *	The index variable "stab_nextfree" points to a free entry in stab[]
 *	such that this stab[stab_nextfree] and all following entries have
 *	never been used.
 *	As entries are allocated and later freed, they are linked into
 *	a freelist via the "slev_link" field, and with "stab_freelist" as
 *	the header.  The "slev_link" field was used for efficient freeing
 *	of every symbol at a given lexical level at once.
 *	The "stab_nextfree" variable eliminates the need to initially link
 *	all stab[] entries onto a freelist.
 *
 *  (2)	Hiding/unhiding when a name in redefined at an inner scope.  This
 *	is handled implicitly by linking the new symtab structure
 *	into the hash chain BEFORE the old entry.
 *	The "hide" routine should really now be a "duplicate_a_name" routine.
 *	Currently the SHIDES, SHIDDEN bits are still defined.  However,
 *	I don't think they are needed anymore.  The current algoritms no
 *	longer set or check these bits except when the CHECKST compile flag
 *	was set.
 *	I think the "unhide" routine could really be totally scrapped. It's
 *	just still here to provide some checks when CHECKST is turned on.
 *	In a later revision will probably want to just remove SHIDES, SHIDDEN,
 *	and all the code for unhide.
 *
 *  (3)	Storage allocation.  The symbol table array "stab" is allocated at
 *	run time as an ems segment.  On overflow, it is dynamically expanded.
 *
 * Adaptations for the s300:
 *      The s300 doesn't have an external memory segment (ems) that can be
 *      grown by the compiler.  As a result, the symbol table is grown in
 *	blocks of size SYMTBLKSZ.  Instead of one common stab[], there
 *      is a current stab_blk[] containing the currently allocated block.
 *      All instances of offsets into stab[] have been replaced with pointers
 *      to the appropriate stab_blk[] entry.
 */

/*************************************************************************
 * symtab_init :
 * 	Initialize the various symbol table data structures and
 *	related variables/tables.
 *	Allocate an ems segment for stab[].
 */

symtab_init() {

  int i;
  struct hashtab * hp;
  stab_blk = (struct symtab *) ckalloc(SYMTBLKSZ*sizeof(struct symtab));
  for ( i=0; i<SYMTBLKSZ; i++ )  stab_blk[i].stype = TNULL;
  for ( i=0, hp=stab_hashtab; i<SYMHASHSZ; i++, hp++) 
	hp->shash_link_forw = hp->shash_link_back = (SYMLINK) hp;
  for ( i=0; i<NSYMLEV; i++ )  stab_lev_head[i] = NULL_SYMLINK;

  stab_freelist = NULL_SYMLINK;
  stab_nextfree = 0;
}

/**************************************************************************
 * stab_alloc :
 *	Find a free stab[] element and return a pointer.
 *	There are two places to look:  the "stab_freelist" linked list of
 *	stab[] elements that have been used and then freed;  the "stab_nextfree"
 *	index to the next stab[] element that has never been used ( a kind of
 *	first pass through the array).
 *
 *	There are obviously two implementation strategies here depending on
 *	where you look first.
 *	Currently, I am using the stab_nextfree first.  This is faster than
 *	unlinking a list element.  It also aids debugging because except for
 *	large programs, all symbols remain accessible (from a debugger)
 *	before the entry is reused.
 *	If the ems segment for stab is ever marked as PAGED, this decision
 *	should be reevaluated since needlessly stepping through the entire
 *	stab[] array could cause extra page faults.
 *
 *	If no free entry is available, then the stab[] array is dynamically
 *	expanded.
 */

struct symtab *
stab_alloc() {
   SYMLINK p;
   int i;

   if (stab_nextfree < SYMTBLKSZ)
	return(&stab_blk[stab_nextfree++]);

   if (stab_freelist != NULL_SYMLINK) {
	p = stab_freelist;
	stab_freelist = stab_freelist->slev_link;

	/* initialize reused entry */
	p->shash_link_forw = NULL;
	p->shash_link_back = NULL;
	p->sname = NULL;
	p->slev_link = NULL;
	p->sattrib = p->sflags = p->offset = 0;
	p->dimoff = p->sizoff = p->suse = 0;
#ifdef CHECKST
	p->checkflags = 0;
#endif
	p->sclass = p->slevel = NULL;
	p->stype = TNULL;

#ifdef IRIF
	p->sprivate.word = NULL;
#ifdef HAIL
	p->hail_info = NULL;
#endif /* HAIL */
#endif /* IRIF */

	return (p);
    }

    stab_blk = (struct symtab *) ckalloc(SYMTBLKSZ*sizeof(struct symtab));    
    for (i=0; i<SYMTBLKSZ; i++ )  stab_blk[i].stype = TNULL;
    stab_nextfree = 0;
    return (&stab_blk[stab_nextfree++]);
}


/***************************************************************************
 * set_slev_link :
 *	Called from the points where the code sets "slevel" to also link
 *	the symtab structure onto the indicated lexical chain.
 *
 */
set_slev_link(sp, lev)
  struct symtab * sp;
{
   if (sp->slev_link != sp) {
	/* this symbol really already entered, but defid thinks it's new.
	 * This can happen when errors in decl cause stype field to
	 * get reset to 0 (UNDEF).  
	 * For now, we'll just give an error to make sure we don't
	 * try to relink the element and messup the linked lists.
	 */
	cerror ("panic in set_slev_link");
   }
   /* link sp into the list at 'lev' lexical level */
   sp->slev_link = stab_lev_head[lev];
   stab_lev_head[lev] = sp;
}


/***************************************************************************
 * reset_slev_link :
 *	Called from points where the slevel field is getting reset.
 *	We must get the symtab structure off its current lexical chain
 *	and onto a new one.
 *	This happens for things like labels (slevel reset to 2) and
 *	extern decls inside a function.
 */

reset_slev_link(sp, lev)
  struct symtab * sp;
  int lev;
{
  int oldlev = sp->slevel;
  if (stab_lev_head[oldlev] == sp ) {
	/* its still at the head of old list and so easy to remove */
	stab_lev_head[oldlev] = sp->slev_link;
  }
  else {
	/* its not at head of list.  We could do a search, but I don't
	 * think this will ever occur. So for now, just put an error
	 * call in case I've oversimplified things.
	 */
	cerror("panic in reset_slev_link");
  }
  /* now link onto new list */
  sp->slev_link = stab_lev_head[lev];
  stab_lev_head[lev] = sp;

}

#ifdef ANSI
/******************************************************************************
 * stab_free :
 *	Free a single stab[] element.
 *	Set its stype to TNULL and link it onto the freelist.
 *	If it is on a hash chain, get it off.
 *
 * ** this was used in debugging: it was called from clearstab() for every
 * 	entry.
 *	However, now for efficiency, clearstab() now links an entire lexical
 *	chain onto the freelist at once.
 */
stab_free(sp)
  struct symtab * sp;
{
   sp->slev_link = stab_freelist;
   stab_freelist = sp;
   sp->stype = TNULL;
   if (sp->shash_link_forw != NULL_SYMLINK) {
	sp->shash_link_back->shash_link_forw = sp->shash_link_forw;
	sp->shash_link_forw->shash_link_back = sp->shash_link_back;
	}
   sp->shash_link_forw = sp->shash_link_back = NULL_SYMLINK;
}

/******************************************************************************
 * stab_free_list :
 *	Free a list of stab[] elements linked via slev_link
 *	Set each stype to TNULL and link it onto the freelist.
 *
 * This was derived from the list above for use in type compatability.
 */
stab_free_list(sp)
  struct symtab * sp;
{
   struct symtab *next;
   while ((next = sp->slev_link) != NULL_SYMLINK) {
      stab_free(sp);
      sp = next;
   }
   stab_free(sp);
}


/***************************************************************************
 * remove_from_slev:
 *	Finds and removes a symbol table entry from a lexical level chain
 */

LOCAL flag remove_from_slev( sp, level ) 
     register struct symtab *sp;
     int level; {

     /* find and remove <sp> from lexical chain <level>
      * return nonzero if successful, zero otherwise
      */
     register struct symtab *xspx;
	  
     if( ( xspx = stab_lev_head[level] ) == sp ) {
          /*
	   * sp is first on the chain 
	   */
          stab_lev_head[level] = sp->slev_link;
	  sp->slev_link = sp;  /* indicates on no level chain */
	  return( 1 );
	     }
     else {
          for( ; xspx->slev_link != NULL_SYMLINK; xspx = xspx->slev_link ){
	       if( xspx->slev_link == sp ) {
		    /*
		     * found it!  remove it! 
		     */
		    xspx->slev_link = sp->slev_link;
		    sp->slev_link = sp; /* indicates on no level chain */
		    return( 1 );
		  }
	     }
	  /* <sp> is not on <level> slev chain */
	  return( 0 );
	}
}
#endif /*ANSI*/

/*****************************************************************************
 * hash_namestring :
 *	calculate a hash value from the first HASHNCHNAM charaters of
 *	"name"
 *
 */
/* maximum number of characters to hash on in a name */
# define  HASHNCHNAM  8

int hash_namestring(name)
   char * name;
{
   int i,j;
   char *p;

   i = 0;
   for( p=name, j=0; *p != '\0'; ++p ){
   	i = (i<<1)+*p;
   	if( ++j >= HASHNCHNAM ) break;
   	}
   /* this will not have overflowed, since HASHNCHNAM is small relative to
   /* the wordsize... */
   return(i%SYMHASHSZ);
}


/*****************************************************************************
 * lookup :
 *	Calculate the hash value for "name" and look it up in the symbol
 *	table.  If not found, allocate a free symbol table entry and
 *	initialize the fields.
 *	The paramter interface from the original BELL "lookup" has been
 *	changed so that lookup returns a pointer to the symtab entry,
 *	instead of an index into the table.
 *	found symbol must match s wrt STAG, SMOS, SLAB
 */

struct symtab *lookup(name, s) char *name; {

	int  hashval;
	struct symtab *sp, *sp1;

# ifdef DEBUGGING
	if( ddebug > 2 ){
		printf( "lookup( %s, %x ), stwart=%x, instruct=%x\n", name, s, stwart, instruct );
		}
# endif

	if(  !( s & STAG ) && (instruct & ( INSTRUCT|INUNION )))
		/* parser state: processing identifiers in struct/union list */
		/* other than tags (i.e., typedef names, declarators)        */
		{
		sp = lookup_struct( name, s );
		sp->suse = lineno;
		return(sp);
		}
	/* calculate hash index */
	hashval = hash_namestring(name);

	/* look for name */
	sp1 = (SYMLINK) &stab_hashtab[hashval];
	for (sp = sp1->shash_link_forw; sp != sp1; sp = sp->shash_link_forw) {
		if( (sp->sflags & (STAG|SMOS|SLAB)) != s ) continue;
# ifdef CHECKST
		if(sp->checkflags & SHIDDEN)
			cerror("SHIDDEN check in lookup fails for %s", name);
# endif
		if (strncmp(sp->sname,name,NCHNAM)==0) 
			{ /* found match */
			sp->suse = lineno;
			return(sp);
			}
	}

	/* No match found.  Return a new entry */
	sp = add_entry (name, s, sp1);
	sp->suse = lineno;
	return(sp);

}

/***************************************************************************
 * add_entry :
 *	Adds name to the symbol table and initializes it
 */

LOCAL struct symtab *add_entry( name, s, symspot ) 
        char *name; 
        struct symtab *symspot; {

        struct symtab *sp, *sp2;

	sp = stab_alloc();
	sp->sflags = s;  /* set STAG, SMOS, SLAB if needed, turn off others */
	sp->sname = addasciz(name);
	sp->stype = UNDEF;
#if defined(HPCDB) || defined(APEX)
	sp->typedefsym = (struct symtab *) 0;
# ifndef APEX
	sp->cdb_info.word = 0;
# endif
# endif
#ifdef SA	
	sp->xt_table_index = NULL_XT_INDEX;
#endif	
#ifdef IRIF
#ifdef HAIL
	sp->source_position = spos_$current_source_position;
#endif /* HAIL */
#endif /* IRIF */
	/* link to hash chain */
	sp->shash_link_forw = sp2 = symspot->shash_link_forw;
	sp->shash_link_back = symspot;
	symspot->shash_link_forw = sp2->shash_link_back = sp;
	/* slevel link will get set later; a self-referencing pointer
	 * marks it as unset: so set_slev_link can check for erroneous
	 * double linking.
	 */
	sp->slev_link = sp;

	return( sp );
}
/***************************************************************************
 * lookup_struct:
 * handles identifiers on struct/union declaration lists
 *
 * returns either a new entry, a previously defined TYPEDEF, or a
 *         previously defined enumeration constant
 *
 * assuming no duplicates:
 *
 *     o   s == SMOS: name is syntactically a declarator and
 *                    a new entry is created;
 *     o   s == 0   : if the closet duplicate entry of name 
 *                      outside of any structure is a TYPEDEF or enum constant,
 *                        return this entry;
 *                    else create a new entry
 */
LOCAL struct symtab *lookup_struct( name, s ) char *name; {

	int  hashval;
	struct symtab *sp, *sp_head;

	/* calculate hash index */
	hashval = hash_namestring(name);
	sp_head = (SYMLINK) &stab_hashtab[hashval];

	/* check for duplicate entry in structure */
	if( ( sp = struct_member( name )) && ( s & SMOS )) {
	     /* "redeclaration of %s" */
	     UERROR( MESSAGE( 96 ), name );
	     return( add_entry( name, SMOS, sp_head )); /* to get thru this */
	}
	/*
	 * find first occurrence of 'name' on hash chain - e.g., lookup
	 */
	for (sp = sp_head->shash_link_forw; sp != sp_head; sp = sp->shash_link_forw) {
	     if (strncmp(sp->sname,name,NCHNAM)==0) 
		  /* found match */
		  break;
	}
	/* 
	 * find previous definition, ignoring SMOS and STAG
	 */
	for ( ; sp != sp_head; sp = sp->shash_link_forw) {
	     
	     if( sp->sflags & (SMOS|STAG) )
		  continue;
	     else {
		  /* check for type definition for name */
		  if( strncmp(sp->sname,name,NCHNAM)==0 ) {
		       if( (sp->sclass) == TYPEDEF ) {
			    if( s & SMOS ) {
				 /* name cannot be used as type specifier */
				 /* "local redeclaration of typedef 
				    as declarator: %s" */
				 /*WERROR( MESSAGE( 169 ), sp->sname );*/
				 break;
			    }
			    else
				 /* name must be used as typedef */
				 return( sp );
		       }
		       /* no typedef - 
			* before making entry, check to see if enum constant,
			* as in:
			*      struct { int a[ enum_constant ]; };
			* or
			*      struct ( int a : enum_constant };
			*/
		       if( s != SMOS && sp->sclass == MOE )
			    return( sp );
		       break;
		  }
	     }
	}
	return( add_entry( name, SMOS, sp_head ) );
}

/***************************************************************************
 * struct_member:
 * searches current structure members for occurrence of name;
 */ 
LOCAL struct symtab *struct_member( name ) char *name; {

     register int cnt;
     register char *oname, *cname;
     register unsigned char k;
     struct symtab *sp;

     for( cnt=1;
	  sp = (struct symtab *) paramstk[paramno-cnt],
	       sp != NULL && (sp->sclass) != STNAME && sp->sclass != UNAME;
	  ++cnt){

          cname=name;
          oname=sp->sname;
	  for(k=1; k<=NCHNAM; ++k){
	       if(*cname++ != *oname)goto diff;
	       if(!*oname++)break;
	     }
	  /* found it! */
	  return( sp );

     diff: continue;
     } 
     /* not there ... */
     return( NULL );
}


/****************************************************************************
 * clearstab :
 *	remove all entries of slevel==lev from the symbol table.
 *	This replaces the BELL routine "clearst".
 */

#ifndef LINT
clearstab( lev ) 
#else
clearstab( lev, aoflag)
#endif
{
	struct symtab *p;
	struct symtab *plast = NULL_SYMLINK;

	int temp = lineno;
	p = stab_lev_head[lev];
	while (p != NULL_SYMLINK) {
		if (p->slevel != lev) cerror ("bad slevel in clearstab");
		if (p->sflags&SNONAME) cerror("SNONAME in clearstab");
# ifdef LINT_TRY
		lineno = p->suse;
# endif
		if( p->stype == UNDEF || ( p->sclass == ULABEL && lev <= 2 ) ){
			lineno = temp;
			/* "%s undefined" */
			UERROR( MESSAGE( 4 ), p->sname );
			}
# ifdef LINT_TRY
		/* don't call aocode() during error recovery */
		else if (aoflag) aocode(p);
# endif
# ifdef DEBUGGING
		if (ddebug) printf("removing %s from stab[ %x], flags %o level %x\n",
			p->sname,p,p->sflags,p->slevel);
# endif
# ifdef CHECKST
		if( p->checkflags & SHIDES ) unhide(p);
# endif
# ifdef HAIL
		/* special hail hack to set sizes for zero sized arrays */
		if (ISARY(p->stype) &&
		    !dimtab[p->dimoff] &&
		    (p->sclass != EXTERN) &&
		    (p->sclass != PARAM) &&
		    (p->sclass != STATIC)){
		    dimtab[p->dimoff] = 1;
		    ir_complete_array(p->stype,p->sattrib,p->sizoff,p->dimoff);
		}
# endif
#ifdef ANSI
		if( ( p->sclass == EXTERN ) && ( lev > 1 )) {
		     /* p is a block extern: promote it */
		     p = promote( p, plast );
		   }
		else {
		     /* remove it from the hash chain */
		     p->shash_link_back->shash_link_forw = p->shash_link_forw;
	             p->shash_link_forw->shash_link_back = p->shash_link_back;
		     p->shash_link_forw = p->shash_link_back = NULL_SYMLINK;
		   }
#else /*ANSI*/
		/* remove it from the hash chain too */
		p->shash_link_back->shash_link_forw = p->shash_link_forw;
		p->shash_link_forw->shash_link_back = p->shash_link_back;
		p->shash_link_forw = p->shash_link_back = NULL_SYMLINK;
#endif /*ANSI*/
		p->stype = TNULL;
		plast = p;
		p = p->slev_link;
		}
	if (stab_lev_head[lev] != NULL_SYMLINK) {
	    /* insert entire chain onto the free list now */
	    plast->slev_link = stab_freelist;
	    stab_freelist = stab_lev_head[lev];
	    stab_lev_head[lev] = NULL_SYMLINK;
	}
	lineno = temp;
	}

# ifdef CHECKST
/***************************************************************************
 * checkst:  some debugging checks.
 *	     Too costly to run in normal production compiler.
 */
/* if not debugging, make checkst a macro */
checkst(lev){
   /* current checks:  all lists at level > lev should now be empty.
    *		    verify the slevel field of each entry.
    */
   int i, j;
   struct symtab *p, *q;

   for (i = lev+1; i<NSYMLEV; i++) {
	if (stab_lev_head[i] != NULL_SYMLINK)
	   cerror("checkst[%x]: level %x is nonempty",lev,i);
   }

   for (i = 0; i<=lev; i++) {
	for (p=stab_lev_head[i]; p != NULL_SYMLINK; p = p->slev_link) {
	    if (p->slevel != i)
		cerror("checkst[%x]: %s [%x] is level %x on chain %x",lev,
			p->sname, p, p->slevel, i);
	}
   }
/* Following check is VERY costly. Put it at another level still */
#  ifdef CHECKST2
   for (i=0, p=stab_blk; i<SYMTBLKSZ; i++, p++) {
	if (p->stype == TNULL) continue;
	q = lookup( p->sname, p->sflags&(SMOS|STAG|SLAB) );
	if ( q != p) {
		if( q->stype == UNDEF ||
		    q->slevel <= p->slevel)
			cerror( "checkst error: %s [%x]", q->sname, j);
	}
	else if (p->slevel > lev) cerror("checkst: %s at level %x", p->sname, lev);
   }
# endif /* CHECKST2 */
}
# endif /* CHECKST */

/*****************************************************************************
 * hide:
 *	called to create a duplicate symbol name  -- when a name is redefined
 *	Returns the index of the new stab element.
 *	inside a nested lexcial scope.
 */
struct symtab *hide( p ) register struct symtab *p; {
	register struct symtab *q, *q1;
	q = stab_alloc();
	/* initially copy all the fields -- some will get reset. */
	*q = *p;
	q->sflags = (p->sflags&(SMOS|STAG|SLAB));
# ifdef CHECKST
	p->checkflags |= SHIDDEN;
	q->checkflags |= SHIDES;
# endif
	q->slev_link = q;
	q1 = q->shash_link_back = p->shash_link_back;
	q->shash_link_forw = p;
	q1->shash_link_forw = p->shash_link_back = q;
#if defined(HPCDB) || defined(APEX)
        q->typedefsym = (struct symtab *) 0;
#endif
# ifdef HPCDB
        q->cdb_info.word = 0;
# endif
#ifdef SA
        q->xt_table_index = NULL_XT_INDEX;
#endif
#ifdef IRIF
#ifdef HAIL
	q->source_position = spos_$current_source_position;
	q->hail_info = NULL;
#endif /* IRIF */
#endif /* HAIL */
#ifdef LINT
	/* "%s redefinition hides earlier one" */
	if( hflag ) WERROR( MESSAGE( 2 ), p->sname );
#endif /* LINT */
# ifdef DEBUGGING
	if( ddebug ) printf( "	%x hidden in %x\n", p, q );
# endif
	return( idname = q );
	}

# ifdef CHECKST
/************************************************************************
 * unhide :
 *	called when a duplicate definition of a name in an inner scope
 *	is removed.
 *	Not used by symbol table management anymore.
 *	Just retained, for now, as part of CHECKST code.
 */
unhide( p ) register struct symtab *p; {
	register struct symtab *q;
	register s, j;

	s = p->sflags & (SMOS|STAG|SLAB);
	q = p->shash_link_forw;

	for(;;){

		if( q == p ) break;

		if( (q->sflags&(SMOS|STAG|SLAB)) == s ){
			if (strncmp(p->sname,q->sname,NCHNAM)==0) {
			   /* found the name */
			   if(q->checkflags & SHIDDEN) {
				q->checkflags &= ~SHIDDEN;
# ifdef DEBUGGING
		   		if( ddebug ) printf( "unhide uncovered %x from %x\n", q,p);
				}
			   else 
				cerror("unhide error: %s [%x] not SHIDDEN",
					q->sname, q);
# endif
			   return;
			   }
			}

		}
	cerror( "unhide fails to find %s", p->sname );
	}
# endif  /* CHECKST */

LOCAL check_globals() {
     /* check global symbol table entries after compilation
      *
      * looks for
      *
      *     o     undefined statics (USTATIC) which were used in expressions;
      *     o     uninitialized statics (and initializes them)
      *
      */
     register struct symtab *sp, *sp_last, *sp_tmp;

     /* reverse list ... */

     sp = stab_lev_head[0];
     sp_last = NULL_SYMLINK;

     while( sp != NULL_SYMLINK ) {
	  sp_tmp = sp->slev_link;
	  sp->slev_link = sp_last;
	  sp_last = sp;
	  sp = sp_tmp;
     }

     /* do the work and reverse again ... */

     sp = sp_last;
     sp_last = NULL_SYMLINK;

     while( sp != NULL_SYMLINK ) {

          if((sp->sclass == USTATIC) && (sp->sflags & SNAMEREF)) {
	       /* "static function identifier '%s' used but not defined; 
		   external reference generated" */
	       WERROR( MESSAGE( 178 ), sp->sname );
	  }
	  if( ( sp->sclass == STATIC ) && !ISFTN( sp->stype )
	     && !(sp->sflags & SINITIALIZED )) {
	       /* static (nonfunction) was never initialized; 
		  now is a good time ... */
	       beginit( sp, FALSE );
	       endinit();
	  }
	  sp_tmp = sp->slev_link;
	  sp->slev_link = sp_last;
	  sp_last = sp;
	  sp = sp_tmp;
      }
}

/***************************************************************************
 *
 * The following functions deal with block externs (and know a great deal
 * about symbol table structure)
 *
 * NOTE: make_members_global() is used in nonANSI mode too
 ***************************************************************************
 * make_members_global
 *      handles members of block extern structures/unions
 *
 *      the lex level of the members is set to global, if not already so.
 */
make_members_global( sp ) struct symtab *sp; {
     /*
      *  'sp' points to the symtab entry for the struct/union
      *  (sp->sizoff)+1 is the dimtab index whose value is the
      *       dimtab index containing the symtab address of the first member
      *
      *  Assumption:
      *
      *       the members are on the *same* level chain in the *reverse* order 
      *       of their lexical appearance, i.e., reverse of the order
      *       of their appearance following the sp->sizoff chain
      */
     int index = dimtab[sp->sizoff + 1];
     int slevel;
     struct symtab *firstp, *p, *q, *current, *global, *local;
     int done;

     if( index == 0 )
	  /* no members */
	  return;

     if( ( p = firstp = ((struct symtab *) dimtab[ index ] ))->slevel  == 0 )
	  /* already global */
	  return;

     slevel = firstp->slevel;

     /* get to the end, marking each as global */
     do {
	  if( p->stype == STRTY || p->stype == UNIONTY )
	       make_members_global( p );
#ifdef SA
	  if (saflag)
	       change_xt_table(p, p->slevel==0, TRUE);
#endif	  
	  p -> slevel = 0;
     }
     while( dimtab[++index] ? p = (struct symtab *) dimtab[index] : 0 );

     /*
      * traverse level chain to find 'p'
      */
     for (q=stab_lev_head[slevel]; q->slev_link != p; q = q->slev_link) {
	  if( q->slev_link == NULL_SYMLINK )
	       cerror("make_members_global: level chain corrupted");
     }
     /*
      * fix up level chains
      *
      * traverse level chain from 'p' to 'firstp',
      * extracting entries marked as global.
      * When done, the new global level prefix
      * will run from 'p' to 'firstp'
      *
      * 'current' is current entry being processed
      * 'local' is the last entry to date left on the
      *          local level chain
      * 'global' is the last entry to date appended to
      *          the global level prefix
      *
      * NOTE: the following assumes both 'p' and 'firstp'
      *       are to be moved to the global chain
      */
     local = q;
     q->slev_link = current = p->slev_link;
     global = p;
     done = ( p == firstp );

     while( !done ) {
	  if( current->slevel == 0 ) {
	       /* append 'current' to global prefix chain 
		* and adjust local chain
		*/
	       local->slev_link = current->slev_link;
	       global->slev_link = current;
	       global = current;
	  }
	  else {
	       /* 'current' is local: leave it alone */
	       local = current;
	  }
	  if( current == firstp ) {
	       done = 1;
	  }
	  else {
	       current = current->slev_link;
	  }
     }
     /* global prefix can now be prepended */
     firstp->slev_link = stab_lev_head[0];
     stab_lev_head[0] = p;
}

#ifdef ANSI
/*************************************************************************
 * make_local :
 *	Makes an existing symbol table entry *local* by 
 *         o   moving it to the front of the hash chain; and
 *         o   creating a dummy placeholder in the entry's former place
 *             on the chain
 */
LOCAL void make_local( sp, head ) register struct symtab *sp, *head; {

     register struct symtab *dummy;

     /* create placeholder for sp */

     dummy = stab_alloc();
     *dummy = *sp;                /* copy fields */
     dummy->sflags |= SDUMMY;
     sp->sflags &= ~SBEXTERN_SCOPE; /* clear block extern flag */
     dummy->slev_link = dummy;    
     (dummy->shash_link_back)->shash_link_forw = dummy;
     (dummy->shash_link_forw)->shash_link_back = dummy;
  
     /* move sp to the front of the hash chain [i.e., a local entry] */

     sp->shash_link_forw = head->shash_link_forw;
     sp->shash_link_back = head;
     (head->shash_link_forw)->shash_link_back = sp;
     head->shash_link_forw = sp;
}

LOCAL struct symtab *external_decl( sp, head ) 
     register struct symtab *sp, *head; {
     /*
      * locates the lexically closest previous external declaration 
      *     for sp->sname
      *
      * sp is handed to us by defid, as either a previous or new symtab
      *     entry for sp->sname.  If previous, it may not be external.
      *
      * "external declaration" means either
      *     o   a declaration with storage class EXTERN; or
      *     o   a file scope declaration
      *
      * if no previous external decl exists, a new entry is created with file
      *     scope, placed so that is accessed after all active nonexternal
      *     declarations of the name.
      */
      struct symtab *last;
      char *name;

      if( IS_NEW_ENTRY( sp )) {
	   sp->sclass = EXTERN;
	   sp->slevel = 0;
#ifdef ANSI
           /* Set flag if block level extern found */
           sp->sflags |= SBEXTERN_SCOPE; 
#endif
	   return( sp );
	 }
      /* search for external declaration */
      name = sp->sname;
      do {
	   if( ( sp->sclass == EXTERN ) || ( sp->slevel == 0 ))
	        return( sp ); /* EXTDEF ==> level 0 */
	   else
	        last = sp;  /* where to insert global declaration */

	   while( sp = sp->shash_link_forw, 
	          (  ( sp != head ) 
		   &&( strncmp(sp->sname,name,NCHNAM)!=0 ))) ;
	 }
      while( sp != head );

      /* no previous external decl- create new global one after <last> */
      sp = add_entry( name, 0, last );
      sp->sclass = EXTERN;
      /* sp->slevel = 0;  --done by stab_alloc() initialization */
      return( sp );
}
/***************************************************************************
 * promote:
 *      handles promotion of block externs at block exit time.  The nearest
 *      placeholder for the entry will be located and swapped with the
 *      entry.
 *
 *      The function returns a symtab pointer to the (swapped in) placeholder
 */
LOCAL struct symtab *promote( sp, splastlev ) 
     register struct symtab *sp;        /* pointer to the entry */
     register struct symtab *splastlev; /* previous entry on level chain */ {

     register struct symtab *splace;
     register char *name = sp->sname;

     /* find placeholder for <sp> */
     for( splace = sp->shash_link_forw; 
	       splace != sp; 
	       splace = splace->shash_link_forw ) {

          if(   ( splace->sflags & SDUMMY ) 
	     && ( strncmp(splace->sname,name,NCHNAM)==0 ) )  break;
	}
     if( splace == sp )
          cerror( "panic in promote: no placeholder for %s", sp->sname );

     /* remove <sp> from hash chain */
     sp->shash_link_back->shash_link_forw = sp->shash_link_forw;
     sp->shash_link_forw->shash_link_back = sp->shash_link_back;

     /* put <sp> in <splace>'s spot on the hash chain */
     splace->shash_link_forw->shash_link_back = sp;
     splace->shash_link_back->shash_link_forw = sp;
     sp->shash_link_forw = splace->shash_link_forw;
     sp->shash_link_back = splace->shash_link_back;

#ifdef IRIF
     ir_promote_extern(sp,splace);
#endif

     /* move <splace> to <sp>'s spot on the lex chain */
     if( splastlev == NULL_SYMLINK )
          stab_lev_head[sp->slevel] = splace;
     else 
          splastlev->slev_link = splace;
     splace->slev_link = sp->slev_link;
     
#ifdef SA     
     /* Switching of levels means a different
	xt table */
     if (saflag)
	change_xt_table(sp, sp->slevel==0, splace->slevel==0);
#endif
     
     /* inherit class and level from placeholder
      *    and link onto appropriate level chain
      */
     sp->slevel = splace->slevel;
     sp->sclass = splace->sclass;
     sp->slev_link = sp;
     set_slev_link( sp, sp->slevel );
#ifdef ANSI
     /* copy block extern flag to table entry if flag is set */
     if (splace->sflags & SBEXTERN_SCOPE) sp->sflags |= SBEXTERN_SCOPE;
#endif

     return( splace );
}

/***************************************************************************
 * demote:
 *      handles the demotion of an external definition to the local level
 *      [i.e., block externs]
 */
LOCAL struct symtab *demote( sp ) register struct symtab *sp; {
     register struct symtab *head;

     head = (SYMLINK) &stab_hashtab[ hash_namestring( sp->sname )];

     /*
      * find previous external declaration for <sp->sname>
      */
 
    sp = external_decl( sp, head );

     /*
      * ... and make it local
      */

     make_local( sp, head );
 
     /*
      * if <sp> is in a lexical chain, move it to the appropriate one 
      */
     if( ( IN_LEX_CHAIN( sp ))) {

          if( !remove_from_slev( sp, sp->slevel ) )
	       cerror( "panic in demote: lexical level chain" );
	  /*
	   * set up <sp> on new lex chain 
	   */
	  sp->sclass = EXTERN;
#ifdef SA     
          /* Switching of levels means a different
	     xt table */
          if (saflag)
	    change_xt_table(sp, sp->slevel==0, blevel==0);
#endif
	  sp->slevel = blevel;
	  set_slev_link( sp, blevel );
	}
     return( sp );
}

/***************************************************************************
 * block_extern:
 *      handles EXTERN declarations at block level
 *
 *      if no prior declaration of sp->sname exists in the current block, the
 *      function returns a symtab entry, which is either
 *           o   a "new" one, meaning no previous external decl exists
 *               ["new" will be installed locally by the balance of defid]; or
 *           o   a previous external decl [installed locally here]
 *
 *      in both cases, an appropriate placeholder for the entry will be
 *           installed in the symbol table, which will be replaced by the
 */
LOCAL struct symtab *block_extern( sp ) register struct symtab *sp; {

     if( blevel == sp->slevel )
          /*
	   * previous declaration at this scope
	   */
          return( sp );
     /*
      * find previous external declaration for <sp->sname>
      *      and make it local
      */
     return( demote( sp ));
}

/***********************************************************************
 * end block extern functions
 **********************************************************************/


/*******************************************************************
 * Symbol table access routines added for function protoype needs.
 * These routines directly access the internal symbol table structures.
 **********************************************************************/

/******************************************************************
 * unhash_symbol
 *    remove a symbol from its hash chain, but do not free the symbol.
 */

void
unhash_symbol(p)
  SYMLINK p;
{
  p->shash_link_back->shash_link_forw = p->shash_link_forw;
  p->shash_link_forw->shash_link_back = p->shash_link_back;
  p->shash_link_forw = p->shash_link_back = NULL_SYMLINK;
}

/********************************************************************
 * rehash_symbol
 *    put a symbol back on a hash chain
 */

void
rehash_symbol(p)
  SYMLINK p;
{ SYMLINK ht;
  int hashval;
# ifdef DEBUGGING
  if (p->shash_link_forw != NULL_SYMLINK || p->shash_link_back != NULL_SYMLINK)
	werror("internal compiler problem: rehash finds non NULL links");
# endif
  hashval = hash_namestring(p->sname);
  ht = (SYMLINK) & stab_hashtab[hashval];
  p->shash_link_forw = ht->shash_link_forw;
  ht->shash_link_forw->shash_link_back = p;
  ht->shash_link_forw = p;
  p->shash_link_back = ht;
}




/* *****************************************************************
 * unscope_parameter_list
 *
 * Remove a list of symbols representing parameter prototypes, so they
 * won't show up in symbol table searches.  This is used whenever
 * we exit a parameter_list.  If it turns out to be the parameter
 * list of a function definition, the names have to be rescoped later.
 * The parameter symbols are linked from the head of the slev lexical level.
 * There  may be non-PARAM sclass elements on the list (if struct's were
 * declared for example).  But everything on the slev chain was
 * declared in the prototype parameter list scope and should now
 * be removed from the general symbol table.
 *
 * Get the symbols off the lexical chain (this is a simple pointer 
 * reassignment).  Also, for each non-SNONAME symbol, it has to be
 * removed from its hash-chain, too.  This involves a removal from
 * a doubly linked list.
 *

 * [NOTE: look back in earlier versions of the code, for an implementation
 *    that just set a bit here to unscope the names, but did not actually
 *    unhash them until it was known that the parameter list was not
 *    for a function definition.
 *    I changed the structure because I thought this made the code
 *    clearer (esp. in cgram.y), at perhaps some expense in efficiency. 
 *  ]
 */

void
unscope_parameter_list(ph, slev)
  NODE * ph;
  int slev;
{ register struct symtab * p;
#ifdef LINT
  int sfflag;
#endif

  p = ph ? ph->ph.phead : NULL_SYMLINK;
#ifdef LINT_TRY
  /* don't check for unused args if this is just a proto declaration */
  sfflag = ph ? ph->ph.flags&SFDEF : 0;
#endif
  if ( p != stab_lev_head[slev] )
	cerror("internal compiler error in unscope_parameter_list");

  for (; p != NULL_SYMLINK; p = p->slev_link) {
# ifdef DEBUGGING
	if (ddebug) 
	   if ( !(p->sflags&SNONAME)) 
		printf("unscoping %s from stab[%x]\n", p->sname, p);
	   else 
		printf("unscope skipping (noname) stab[%x]\n", p);
# endif
	if ( p->sflags&SNONAME) continue;	/* not on hash chains */
	unhash_symbol(p);
#ifdef LINT_TRY
        if (sfflag)
                aocode(p);
#endif
	}
  stab_lev_head[slev] = NULL_SYMLINK;
}


/***********************************************************************
 * rescope_parameter_list :
 *
 * Make a parameter_list visible again and get parameter pointers on the
 * paramstk for the various start of function processing.  This is called
 * at the head of a function.
 * Note that this causes the "psave's" to come reverse of the pre-ANSI
 * compiler.  This must be accounted for in all the routines that
 * step through the paramstk.
 * No SNONAME's should be encountered here in non-error case.
 * The lexical chain needs to be relinked to the proper stab_lev_head[]
 * and each name needs to be reinserted onto the hash chains.
 */
void
rescope_parameter_list(ph, slev)
  NODE * ph;
  int slev;
{ register struct symtab * p;
  if (stab_lev_head[slev] != NULL_SYMLINK)
	cerror("panic in rescope_parameter_list: slevel chain not empty");
  if (ph->in.op != PHEAD)
	cerror("bad PHEAD node in rescope_parameter_list");
  if (ph == NULL || (p = ph->ph.phead) == NULL_SYMLINK)
	return;
  
  stab_lev_head[slev] = p;
  for (; p != NULL_SYMLINK; p = p->slev_link) {
# ifdef DEBUGGING
	if (ddebug) {
	   printf("rescoping %s at stab[%x]\n", p->sname, p);
	   if (p->slevel != slev)
		werror("rescope changes slevel of %s", p->sname);
	   if (p->sflags & SNONAME)
		werror("rescope of (snoname) at %x", p);
	   }
# endif
	if (p->sflags & SNONAME) continue;	/* error flagged elsewhere */
	p->slevel = slev;	/* ??? */
	rehash_symbol(p);
	if (p->sflags & (SFARGID|SPARAM))
	   psave(p);
	}
}




/************************************************************************
 * slevel_head :
 *
 * Return the head of the indicated lexical level chain.  The return
 * value could be null.
 **** FOR EFFICIENCY REPLACE BY A MACRO AFTER CODE IS DEBUGGED.
 */

SYMLINK
slevel_head(slev)
  int slev;
{
  return(stab_lev_head[slev]);
}

/***********************************************************************
 * make_snoname :
 *
 * Get a dummy symbol table entry that can be used for a prototype
 * entry that did not include a symbol name.
 */
SYMLINK 
make_snoname()
{  register SYMLINK sym;
   
   /* Get a free symbol table entry.
    * ?? Do we want to do any initialization of the fields ????
    * ?? Call set_slev_link here or from routine that calls this ????
    */
   sym = stab_alloc();
   sym->stype = UNDEF;
   sym->sclass = SNULL;
   sym->sname = 0;
   sym->sflags = SNONAME;
   sym->shash_link_forw = NULL_SYMLINK;
   sym->shash_link_back = NULL_SYMLINK;
   sym->slev_link = sym;
   sym->slevel = blevel;
   /*set_slev_link(sym,blevel);*/

   return(sym);
}

/*************************************************************************
 * defid_fparam :
 *
 * Process a function prototype parameter declaration.
 * Do work similar to ftnarg(), and then call defid() to process the
 * declaration.
 */
defid_fparam(typ, dec, cclass)
  NODE * typ, *dec;
  int cclass;
{
  struct  symtab *p;
  NODE *q;
  if (dec==NIL) {
	/* error detected elsewhere */
	return;
	}
  q = tymerge(typ, dec);
  p = q->nn.sym;
  if (p==NULL_SYMLINK) cerror("tyreduce");
  if ((p->stype != UNDEF) && (p->slevel<blevel)) {
	p = hide(p);
	p->stype = UNDEF;
	q->nn.sym = p;  /* is that enough ?? */
	}
  if (p->stype != UNDEF) {
  	/* "redeclaration of formal parameter '%s'" */
	UERROR( MESSAGE( 97 ), p->sname);
	}
  else {
	p->stype = FARG;
	p->sclass = PARAM;
	set_slev_link(p, blevel);
	p->slevel = blevel;
	p->sflags |= SPARAM;
	/*psave(p);	/* No.  Don't do the psave until we know we have
			 * a function definition. */

	defid( q, cclass); 
	}
}


/***********************************************************
 * make_proto_header and free_proto_header :
 * 
 * Manage the  allocation and freeing of phead NODE's.  The normal node
 * allocation (talloc, and tfree) cannot be used because phead NODE's do
 * not obey the rules of when NODE's must be free (in particular they live
 * across statement boundaries).
 * Currently no nodes are preallocated, and ckalloc is called to get
 * new nodes.  Freed nodes are reused.
 * Obvious possible tuning changes are to preallocate a block of nodes
 * and to do storage allocations in a block, rather than one node at
 * a time.
 */

static NODE * phead_freelist = NIL;

/*********************************************************************
 * make_proto_header :
 *
 *  Allocate a PHEAD node, and initialize the fields.
 */
NODE *
make_proto_header()
{
  NODE * q;
  if (phead_freelist != NIL) {
	q = phead_freelist;
	phead_freelist = (NODE *) phead_freelist->ph.phead;
	}
  else
  	q = (NODE *) ckalloc(sizeof(NODE));
  q->ph.op = PHEAD;
  q->ph.nparam = 0;
  q->ph.phead = NULL_SYMLINK;
  q->ph.flags = 0;
  return (q);

}


/*********************************************************************
 * free_proto_header :
 *
 * Free a PHEAD note by putting it on a freelist for reuse.
 */
void
free_proto_header(q)
  NODE * q;
{ q->ph.phead = (SYMLINK) phead_freelist;
  phead_freelist = q;
}

/********************************************************************
 * check_parameter_list :
 *
 * Do parsing-time checks of the parameter list format:
 *   Can't mix paramter declarations and old-style names.
 *   If void occurs, there must only be one parameter on the list, and
 *   then remark it to be 0 parameters.
 *   Mark the header to show whether there were any abstract constructors
 *   (SNONAME).
 */
check_parameter_list(q)
  NODE *q;
{ struct symtab * p;
  register int flags;
 
  if (q == (NODE *) 0 ) return;
  if (q->ph.op != PHEAD)
     cerror("panic in check_parameter_list");
 
  flags = q->ph.flags;
  if ((flags & SFARGID) && (flags & SPARAM)) {
	/* "incorrect mix of names and parameter types parameter list" */
	UERROR( MESSAGE( 189 ) );
	/* clear the SFARGID flag;  having both SFARGID and SPARAM set
	 * in the PHEAD node is meant to signal a prototype followed by
	 * an old-style definition.
	 */
	q->ph.flags &= ~SFARGID;
	}

  if ((flags & SFARGID) && (flags & SELLIPSIS))
	/* "ELLIPSIS used with old style parameter name list" */
	UERROR( MESSAGE( 192 ) );

  for (p=q->ph.phead; p != NULL_SYMLINK; p=p->slev_link ) {

	if ( !(p->sflags & (SFARGID|SPARAM)) )
		continue;
	if (p->stype==VOID) {
		if (q->ph.nparam==1 && !(q->ph.flags&SELLIPSIS) ) {
			/* void would be only thing on list, delete it */
			q->ph.nparam=0;
			q->ph.phead = NULL_SYMLINK;
			stab_free(p);
			break;
			}
		else {
			/* "incorrect use of 'void' in parameter list" */
			UERROR( MESSAGE( 193 ) );
			continue;	/* don't set SNONAME in header */
			}
		}

	if (p->sflags&SNONAME) q->ph.flags |= SNONAME;
     }

}


/*********************************************************************
 * check_function_header :  
 * 
 *   additional (to check_parameter_list ) parsing time checks to be 
 *   performed when we know that the declaration is actually the head of
 *   a function definition.
 *	** No SNONAMES allowed.
 *	** ?? what else ???
 *	** Set some PHEAD flag fields
 */
check_function_header(q)
  NODE * q; 	/* result from tymerge */
{
  NODE * ph;
  struct symtab * p;
  int dim;
  p = q->nn.sym;
  dim = q->fn.cdim;
  ph = (NODE *) dimtab[dim];

# ifdef DEBUGGING
  if (ddebug > 3) {
	printf("check_function_header: %s at stab[%x], phead[%x]\n",
		p->sname, ph);
	}
# endif
  /* if no parameter list, set the flag to indicate old-style */
  if ( !(ph->ph.flags & (SFARGID|SPARAM)) )
	ph->ph.flags |= SFARGID;

  /* Set SFDEF flags for defid() and iscompat() */
  ph->ph.flags |= SFDEF;
  p->sflags |= SFDEF;
  
  if (ph->ph.flags & SNONAME)
	/* "abstract declarator incorrect in function definition" */
	UERROR( MESSAGE( 194 ) );

}

/**********************************************************************
 * do_default_param_dcls
 *    Scan the parameter list for old-style parameters that did not
 *    have an explicit type declaration, and set the default type.
 *    For ANSI, this is moved up before dclargs(), because dclargs does
 *    too much function start up code, and we need to have these set
 *    before we define the function id name so that parameter types
 *    can be compared to any prototype.
 */

void
do_default_param_dcls()
{ register int i;
  register struct symtab *p;
  register NODE * q;
	
  for ( i=paramno-1; i>=0; --i) {
	p = (struct symtab *) paramstk[i];

# ifdef LINT
      /* This is a dirty fix for the DTS#CLLbs00095.  Asok Datla 4/19/92*/
      /* The paramstk is not deep enough for the loop to complete.      */
      /* This results in a core dump, which the user doesn't want to    */
      /* see.                                                           */
      /* The C 300 compiler cann't recover when a syntax error is       */
      /* encountered for typedef enum { aa ; bb };                      */
      if ( p == NULL)
      {
      (void)stderr_fprntf( "cannot recover from earlier errors!\n" );
      exit(1);
      }
# endif /* LINT */

	if( p->stype == FARG ) {
		q = block(FREE,NIL,NIL,INT,0,INT);
		q->nn.sym = p;
		defid( q, PARAM );
		}
	}
}


/**************************************************************************
 * reset_lev_heads
 *
 * Clean up for error recovery in parser.  Unrecoverable syntax errors
 * while parsing function declarators can leave garbage entries linked
 * to the symbol table chains.  Do clean up to help allow parsing to
 * continue if possible.
 */

void
reset_lev_heads(lo, hi)
  int lo, hi;
{ register int i;
  for (i=lo; i<hi; i++) {
#ifndef LINT
	clearstab(i);
#else
	clearstab(i,0);
#endif /* LINT */
	stab_lev_head[i] = NULL_SYMLINK;
	proto_head[i] = NIL;
	}
}



/************************************************************************
 * copy_prototype
 *   Given a pointer to a PHEAD NODE, this routine copies a prototype
 *   by making copies of the PHEAD, and all the SPARAM symtab struct's
 *   on the parameter list. 
 *
 *   This routine is intended to be called from "tymerge" when type
 *   info is being inherited from a typedef.  We need a copy of the
 *   the prototype, it can't be shared, because the type compatibility
 *   and composite routines expect to be able to rewrite prototype entry
 *   types when necessary.
 *
 *   For each parameter, copying the type includes making a copy of
 *   dimtab info for ARY and FTN modifiers.  For ARY, this just requires
 *   copying  the dimension from dimtab;  for FTN, it requires a recursive
 *   call to this routine, and then dstashing the pointer to the copy PHEAD.  
 *   Since dimtab info must come in a linear block, it is necessary to
 *   make all the recursive calls for any subordinate FTN's, storing the
 *   returned PHEAD pointers in a local array, and then building all the
 *   ARY and FTN info into the dimtab.
 *
 *   Note that following what is currently done in "tymerge" csiz/sizoff is
 *   just copied, so STRUCT's are shared not copied.
 */

NODE *
copy_prototype(ph)
  NODE * ph;
{ NODE * phnew;
  SYMLINK s, s1, *slast;
  long  diminfo[MAXTMOD];
  int  i, d;
  TWORD t;

# ifdef DEBUGGING
  if (ddebug > 2) {
	printf("Start copy_prototype: 0x%x\n", ph);
	fwalk(ph, eprint, 0);
	}
# endif

  if (ph==NIL || ph->ph.op != PHEAD)
	cerror("bad phead node in copy_prototype");

  /* make a duplicate phead node */
  phnew = make_proto_header();
# ifdef DEBUGGING
  if (ddebug >2)
	printf("new phead node: 0x%x\n", phnew);
# endif
  *phnew = *ph;


  /* traverse the parameter list, and copy each SPARAM on the list.
   * There should not be any SFARGID's on the list, and other id-types
   * don't require a copy.
   */
  for (s=next_param(ph->ph.phead), slast=(&phnew->ph.phead); s!=NULL_SYMLINK;
	s=next_param(s->slev_link), slast=(&s1->slev_link) ) {
	/* Create a new symtab struct and copy the fields;  if this is
	 * the first param to be copied, set the phead field in phnew.
	 */
	s1 = stab_alloc();
	*slast = s1;
	*s1 = *s;

	/* See if the param type includes any FTN modifiers.  If so
	 * call copy_prototype recursively, and store the phead value
	 * returned so it can be dstashed in next loop.  If ARY save
	 * the dimension.
	 */
	for ( t=s->stype, d=0, i=s->dimoff; t&TMASK; t=DECREF(t) ) {
	   if ( ISFTN(t) )
		diminfo[d++] = (long) copy_prototype((NODE *)dimtab[i++]);
	   else if ( ISARY(t) )
		diminfo[d++] = dimtab[i++];
	   }

	/* Now go back and put out the diminfo for the current parameter
	 * in a single linear block.
	 */
	s1->dimoff = curdim;
	for ( i=0; i<d; i++)  dstash((int)diminfo[i]);
# ifdef DEBUGGING
	if (ddebug > 2)
	   printf("symtab [%x/%d] copied to [%x/%d]\n", s, s->dimoff,
	     s1, s1->dimoff);
# endif
	
	}  /* loop through SPARAM list */

  *slast = NULL_SYMLINK;

# ifdef DEBUGGING
  if (ddebug > 2) {
	printf("End copy_prototype: 0x%x\n", phnew);
	fwalk(phnew, eprint, 0);
	}
# endif

  return(phnew);

}

/*********************************************************************
 *   End of Prototype Support Functions
 ********************************************************************/

# endif  /* ANSI */


#ifdef IRIF
/************************************************************************
 * initializer_offset
 *   Returns the offset of the current initializer in bits, using
 *   the information in instack.  This routine handles non-agregates
 *   and union types, always returning 0 as the offset.
 *
 *   This routine depends on global variables instack and pstk.
 *   These variables are read and not modified.  Normal FE operations
 *   are left to take care of elided braces, etc.
 */
static int type_size();
int initializer_offset(){
	int offset = 0;
	struct instk *istk;
	for (istk = instack;istk <= pstk;istk++){
		if (ISARY(istk->in_t)){
			offset = offset + istk->in_n * type_size(DECREF(istk->in_t),istk->in_s,istk->in_d+1);
		} else if (istk->in_t == STRTY) {
			offset = offset + istk[1].in_id->offset;
		}
	}
	return offset;
}

/* simplified version of tsize to get size from TWORD/csiz/cdim.  No error
 * checking is done, integrate with other size computation. */
static int type_size(type,csiz,cdim) TWORD type; int csiz,cdim; {
	int dim = 1;
	while (ISARY(type)){
		dim = dim * dimtab[cdim];
		type = DECREF(type);
		cdim++;
	}
	if (ISPTR(type)) return SZPOINT*dim;
	else return dimtab[csiz]*dim;
}
#endif /* IRIF */
