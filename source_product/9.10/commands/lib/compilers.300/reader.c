/* file reader.c */
/*      SCCS    REV(64.1);       DATE(92/04/03        14:22:23) */
/* KLEENIX_ID @(#)reader.c	64.1 91/08/28 */

# include "mfile2"
# include "messages.h"

# define ISFZERO(p) ((p)->in.op == NAME && (p)->tn.rval == -fzerolabel && fzerolabel != 0)
# define NEEDTOSTORE(p)	( ((p)->in.su > fregs) || (((p)->in.fpaside) ? ((p)->in.fsu > dfregs) : ((p)->in.fsu > ffregs)) && ((p)->in.type != VOID))
# define MINTABSZ	40

/*	some storage declarations */

# ifndef ONEPASS
char filename[FTITLESZ] = "";  	/* the name of the file */
int ftnno;  			/* number of current function */
int lineno;			/* current line number from pass 1 */
# else
# define NOMAIN
#endif

flag lflag;

	NODE adrregnode;	/* save info for SADRREG node seen in adrreg */

	flag profiling = 0;	/* non-zero => generate profile code */

# ifdef DEBUGGING
	flag d2debug = 0;	/* if not 0, is number of available data regs */
	flag fdebug = 0;	/* if not 0, is number of available fhw regs */
#if defined(ONEPASS) || defined(FORT)
	flag xdebug = 0;
#endif /* ONEPASS || FORT */
	flag udebug = 0;
	flag edebug = 0;
	flag odebug = 0;
	extern int yydebug;	/* if not 0, turns on yacc debug state info */
# endif /* DEBUGGING */



OFFSZ tmpoff;  /* offset for first temporary, in bits for current block */
OFFSZ treetmpoff;  /* offset for first temporary, in bits for current block */
OFFSZ maxoff;  /* maximum temporary offset over all blocks in current ftn, in bits */
OFFSZ baseoff = 0;
OFFSZ maxtemp = 0;

# ifndef ONEPASS
	OFFSZ tmpoff_c1; /* offset of first temp before c1 adds more */
# endif /* ONEPASS */

# if (defined(FORT))
	NODE *topnode;	 /* top of tree node */
# endif /* FORT */

short callflag;
short fcallflag;
int maxtreg;
int maxtfdreg;
int stocook;
int deli;		/* # of delayed operations to be done on this tree */
int fregs;		/* # available type A temp regs */
int afregs;		/* # available type B temp regs */
int ffregs;		/* # available type F scratch regs */
int dfregs;		/* # available type D scratch regs */

NODE *deltrees[DELAYS];
NODE *stotree;

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
  if (p->in.op != REG && p->in.op != SCONV)
    s = 0;
  else if ( ISPTR(t) )
    s = SZPOINT;
  else if ( ISARY(t) || ISFTN(t) )
    s = 0;
  else
    {
    t = BTYPE(p->in.type);
    s = typeszary[ t ];
    if ( s < 0 )
      s = p->stn.stsize * SZCHAR;
    }
  return (s);
}



p2init( argc, argv ) char *argv[];{
	/* set the values of the pass 2 arguments */

	register short c;
	register char *cp;
	register files;

	allo0();  /* free all regs */
	files = 0;

	for( c=1; c<argc; ++c ){
		if( *(cp=argv[c]) == '-' ){
			while( *++cp ){
				switch( *cp ){

				case '!':  /* system mode */
					break;


				case 'N':	/* table size resets */
					{
					char *lcp = ++cp;
					int k = 0;
					while ( (*++cp >= '0') && (*cp <= '9') )
						k = 10*k + (*cp - '0');
					--cp;
					if (k <= MINTABSZ) {
					     /* simple-minded check */
					     /* "small table size of '%d' specified 
						for '%c' ignored" */
					     /* WERROR( MESSAGE( 219 ), k, *lcp ); */
					     werror( "small table size of '%d' specified for '%c' ignored",
						     k, *lcp );
					     k = 0;
					}
					if (k) resettablesize(lcp, k);
					break;
					}

						

				case 'Y':  /* pass1 flags */
					while( *++cp ) { /* VOID */ }
					--cp;
					break;
					
			        case 'W':
					cp++;
					break;

				case 'l':  /* linenos */
					++lflag;
					break;
#ifdef MSG_COVERAGE
				case 'm':       /* pass1 ... ignore here */
					while( *++cp )
					     ;
					cp--;
					break;
#endif
				case 'n':	/* pass 1... nls support */
					break;

				case 'p':
					profiling = 1;
					break;

# ifdef YYCOVERAGE
				case 'R':	/* pass1 ... ignore here */
					while( *++cp )
					     ;
					cp--;
					break;
# endif

				case 'b':	/* switch -- DRAGON fpa,68881 */
					fpaflag = 1;
					break;
#ifndef FORT
				case 'c':
					compatibility_mode_bitfields = 1;
					break;
#endif

				case 'f':	/* inline DRAGON fpa code */
					fpaflag = -1;
					break;

				case 'i':
					/* use word offset <ea> forms */
					picoption = 1;
					break;

				case 'I':
					/* use long offset <ea> forms */
					picoption = 2;
					break;

#ifdef FORTY
				case 'q': /* FORTY code */
					fortyflag = 1;
					break;
#endif /* FORTY */

#ifdef DEBUGGING
				case 'a':  /* rallo */
					++radebug;
					break;

				case 'd':	/* data register set */
					++d2debug;
					break;

				case 'e':  /* expressions */
					++edebug;
					break;

				case 'h':	/* fhw register set */
					fdebug++;
					break;

				case 'o':  /* orders */
					++odebug;
					break;
				case 'r':  /* register allocation */
					++rdebug;
					break;

				case 's':  /* shapes */
					++sdebug;
					break;

				case 't':  /* ttype calls */
					++tdebug;
					break;

				case 'u':  /* Sethi-Ullman testing (machine dependent) */
					++udebug;
					break;

				case 'x':  /* general machine-dependent debugging flag */
					++xdebug;
				case 'E':
					break;
# endif	/* DEBUGGING */

				case 'M':		/* no in-line 68881 */
					flibflag = 1;
					break;

# ifdef C1_C
/* cpass2 warns about unrecognized options.  
 * code.c:mainp1() has already parsed options and warned for ccom.
 */
				default:
					/* "unrecognized option '%s%s' ignored" */
					/* WERROR( MESSAGE( 220 ), "-", cp );"-", cp ); */
					werror( "unrecognized option '%s%s' ignored",
					        "-", cp );
# endif /* C1_C */
					}
				}
			}
		else files = 1;  /* assumed to be a filename */
		}
	if ( picoption )
	  {
	  if ( fpaflag )
	    {
	    minrvarb = MINRVAR+2;
	    picreg = A3;
	    }
	  else
	    {
	    minrvarb = MINRVAR+1;
	    picreg = A2;
	    }
	  }
	else if ( fpaflag )
	  minrvarb = MINRVAR+1;

	mkdope();	/* in common - inits the dope array */
	setrew();	/* in match.c - inits the rwtable */

	return( files );
	}


# ifdef ONEPASS

p2upoff( newoff ) 
{
  tmpoff = baseoff = newoff;
  if ( baseoff > maxoff ) maxoff = baseoff;
}



p2compile( p ) register NODE *p; {

	register short oldpicopt = 0; /* short circuit PIC handling */

	if( lflag ) lineid( lineno, filename );
	treetmpoff = tmpoff = baseoff;  /* expr at top level reuses temps */
	/* generate code for the tree p */
# ifdef DEBUGGING
	if( edebug ) fwalk( p, eprint, 0 );
# endif
	if ( picoption && p->in.op == INIT )
	  {
	  oldpicopt = picoption;
	  picoption = 0;
	  }

# ifdef MYREADER
	MYREADER(p);  /* do your own laundering of the input */
# endif
	nrecur = 0;
	delay( p );  /* do the code generation */
	reclaim( p, RNULL, 0 );
	allchk();
	/* can't do tcheck here; some stuff (e.g., attributes) may be around from first pass */
	/* first pass will do it... */

	if ( oldpicopt )
	  picoption = oldpicopt;
	}

p2bbeg( aoff, myreg, myfdreg ) 
     {
	static int myftn = -1;
	tmpoff = baseoff = aoff;
	maxtreg = myreg;
	maxtfdreg = myfdreg;
	if( myftn != ftnno ){ /* beginning of function */
		maxoff = baseoff;
		myftn = ftnno;
		maxtemp = 0;
		}
	else {
		if( baseoff > maxoff ) maxoff = baseoff;
		/* maxoff at end of ftn is max of autos and temps over all blocks */
		}
	setregs();
	}

p2bend(){
	SETOFF( maxoff, ALSTACK );
	eobl2( oflag );
	}

# endif


delay( p ) register NODE *p; {
	/* look in all legal places for COMOP's and ++ and -- ops to delay */
	/* note; don't delay ++ and -- within calls or things like
	/* getchar (in their macro forms) will start behaving strangely */
	register i;

	/* look for visible COMOPS, and rewrite repeatedly */

	while( delay1( p ) ) { /* VOID */ }

	/* look for visible, delayable ++ and -- */

	deli = 0;
	delay2( p );
	codgen( p, FOREFF );  /* do what is left */
	for( i = 0; i<deli; ++i ) codgen( deltrees[i], FOREFF );  /* do the rest */
	}

delay1( p ) register NODE *p; {  /* look for COMOPS */
	register short o = p->in.op;
	short ty;

	ty = (short)optype( o );
	if( ty == LTYPE ) return( 0 );
	else if( ty == UTYPE ) return( delay1( p->in.left ) );

	switch( o ){

	case QUEST:
	case ANDAND:
	case OROR:
		/* don't look on RHS */
		return( delay1(p->in.left ) );

	case COMOP:  /* the meat of the routine */
		delay( p->in.left );  /* completely evaluate the LHS */
		/* rewrite the COMOP */
		{ register NODE *q;
			q = p->in.right;
			ncopy( p, p->in.right );
			q->in.op = FREE;
			}
		return( 1 );
		}

	return( delay1(p->in.left) || delay1(p->in.right ) );
	}

delay2( p ) register NODE *p; {

	/* look for delayable ++ and -- operators */

	register short o = p->in.op;
	short ty = (short)optype(o);

	switch( o ){

	case NOT:
	case QUEST:
	case ANDAND:
	case OROR:
	case CALL:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
	case COMOP:
	case CBRANCH:
		/* for the moment, don't delay past a conditional context, or
		/* inside of a call */
		return;

	case INCR:
	case DECR:
		if( deltest( p ) ){
			if( deli < DELAYS ){
				register NODE *q;
				deltrees[deli++] = tcopy(p);
				q = p->in.left;
				tfree( p->in.right );
				ncopy( p, q );
				q->in.op = FREE;
				return;
				}
			}

		}

	if( ty == BITYPE ) delay2( p->in.right );
	if( ty != LTYPE ) delay2( p->in.left );
	}

codgen( p, cookie ) register NODE *p; {

	/* generate the code for p;
	   order may call codgen recursively */
	/* cookie is used to describe the context */

	for(;;){
		canon(p);  /* creats OREG from * if possible and does sucomp */
		stotree = NIL;
		if ( p->in.op == FREE )	/* "void" expression "stored" */
		  return;
# ifdef DEBUGGING
		if( edebug ){
			prntf( "store called on:\n" );
			fwalk( p, eprint, 0 );
			}
# endif
		store(p);
		if( stotree==NIL ) break;

		/* because it's minimal, can do w.o. stores */

		order( stotree, stocook );
		}

	order( p, cookie );

	}

# ifdef DEBUGGING
/* Note: revise this table whenever revising defines of the constants involved! */
#ifdef FASTMATCH
char *cnames[] = {

	/* shape,	cookie */

	"SANY",		"FOREFF",
	"SAREG",	"INAREG",
	"STAREG",	"INTAREG",
	"SBREG",	"INBREG",
	"STBREG",	"INTBREG",
	"SCC",		"FORCC",
	"SNAME",	"oops",
	"SCON",		"oops",
	"SFLD",		"oops",
	"SOREG",	"oops",
	"STARNM",	"oops",
	"STARREG",	"oops",
	"INTEMP",	"INTEMP",
	"oops",		"FORARG",
	"SADRREG",	"FORREW",
	"SZERO",	"oops",
	"SCCON",	"oops",
	"SICON",	"oops",
	"S8CON",	"oops",
	"SONE",		"oops",
	"SMONE",	"oops",
	"SFREG",	"INFREG",
	"STFREG",	"INTFREG",
	"oops",		"oops",
	"SDREG",	"INDREG",
	"STDREG",	"INTDREG",
	0,		0};

prcook( cookie ){

	/* print a nice-looking description of cookie */

	register i;
	register short lpflag;

	lpflag = 0;
	for( i=0; cnames[(i*2)+1]; ++i ){
		if( cookie & (1<<i) ){
			if( lpflag ) prntf( "|" );
			++lpflag;
			prntf( cnames[(i*2)+1] );
			}
		}
	}

prshape( shape ){

	/* print a nice-looking description of shape */

	register i;
	register short lpflag;

	lpflag = 0;
	for( i=0; cnames[i*2]; ++i ){
		if( shape & (1<<i) ){
			if( lpflag ) prntf( "|" );
			++lpflag;
			prntf( cnames[i*2] );
			}
		}

	}
#else
char *cnames[] = {
	"SANY",
	"SAREG",
	"STAREG",
	"SBREG",
	"STBREG",
	"SCC",
	"SNAME",
	"SCON",
	"SFLD",
	"SOREG",
	"STARNM",
	"STARREG",
	"INTEMP",
	"FORARG",
	"FORREW",
	"oops",
	"oops",
	"oops",
	"oops",
	"oops",
	"oops",
	"SFREG",
	"STFREG",
	"oops",
	"SDREG",
	"STDREG",
	0,
	};

prcook( cookie ){

	/* print a nice-looking description of cookie */

	register i;
	register short lpflag;

	if( cookie & SPECIAL ){
		if( cookie == SZERO ) prntf( "SZERO" );
		else if( cookie == SONE ) prntf( "SONE" );
		else if( cookie == SMONE ) prntf( "SMONE" );
		else prntf( "SPECIAL+0x%x", cookie & ~SPECIAL );
		return;
		}

	lpflag = 0;
	for( i=0; cnames[i]; ++i ){
		if( cookie & (1<<i) ){
			if (SPECIAL == (1<<i) ) continue;
			if( lpflag ) prntf( "|" );
			++lpflag;
			prntf( cnames[i] );
			}
		}

	}
# endif /* FASTMATCH */
# endif /* DEBUGGING */


/* rm_seffects() removes all side effects from the tree and returns a ptr to
   the top. Always called as an argument to fwalk().
*/
int rm_seffects(p)	register NODE *p;
{
	register NODE *temp;

	switch(p->in.op)
		{
		case RETURN:
			uerror("weird side effect on asgop");
			break;

		case INCR:
		case DECR:
			temp = p->in.left;
			tfree(p->in.right);
			*p = *temp;
			temp->in.op = FREE;
			break;

		/*
		case CAST:
			break;
		*/

		/* callop() */
		case UNARY CALL:
		case CALL:
# ifndef FORT
		case UNARY STCALL:
		case STCALL:
# endif /* FORT */
			werror("ambiguous call. code may be wrong");
			break;

		/* asgop() */
		case ASSIGN:
		case ASG PLUS:
		case ASG MINUS:
		case ASG MUL:
		case ASG AND:
		case ASG DIV:
		case ASG MOD:
		case ASG LS:
		case ASG RS:
		case ASG OR:
		case ASG ER:
		case ASG DIVP2:
# ifndef FORT
		case STASG:
# endif /* FORT */
			temp = p->in.left;
			tfree(p->in.right);
			*p = *temp;
			temp->in.op = FREE;
			break;
		}
}



#pragma OPT_LEVEL 1
order(p,cook) register NODE *p; {

	register short o;
	register m;
	register NODE *p1, *p2;
	NODE *q,*q2;
	short ty;
	unsigned int m1;
	int cookie;

	cookie = cook;
	rcount();			/* macro -- just counts recursion */
	canon(p);			/* reader.c (~1280) */
	rallo( p, p->in.rall );		/* order.c (~372) */
	goto first;
	/* by this time, p should be able to be generated without stores;
	   the only question is how */

again:

	cookie = cook;
	rcount();
	canon(p);
	rallo( p, p->in.rall );
	/* if any rewriting and canonicalization has put
	 * the tree (p) into a shape that cook is happy
	 * with (exclusive of FOREFF, FORREW, and INTEMP)
	 * then we are done.
	 * this allows us to call order with shapes in
	 * addition to cookies and stop short if possible.
	 */
#ifdef FASTMATCH
	if (tshape(p) & (cook &(~(FOREFF|FORREW|INTEMP)))) {
#ifdef DEBUGGING
		if (odebug) {
			prntf("re-order( 0x%x, ", p);
			prcook(cookie);
			prntf(" ) finished\n");
			}
#endif /* DEBUGGING */
		return;
		}
#else /* FASTMATCH */
	if( tshape(p, cook &(~(FOREFF|FORREW|INTEMP))) )return;
#endif /* FASTMATCH */

first:

# ifdef DEBUGGING
	if( odebug ){
		prntf( "order( 0x%x, ", p ); 
		prcook( cookie );
		prntf( " )\n" );
		fwalk( p, eprint, 0 );
		}
# endif

	o = p->in.op;
	ty = (short)optype(o);

	/* first of all, for most ops, see if it is in the table */

	/* look for ops */

	switch( m = o ){

	default:
		/* look for op in table */
		if (flibflag) cookie &= ~(INFREG|INTFREG|INDREG|INTDREG);
		else if ( p->in.fpaside && ISFMONADIC_NOT_DRAGON(p)
			  && ISDNODE( p->in.right ))
			{
			m = BITYPE;
			break;
			}
		for(;;){
			if( (m = match( p, cookie ) ) == MDONE ) goto cleanup;
			else if( m == MNOPE ){
				if( !(cookie = nextcook( p, cookie ) ) ) goto nomat;
				continue;
				}
			else break;
			}
		break;
	case DBRA:
	case COMOP:
	case FORCE:
	case CBRANCH:
	case QUEST:
	case ANDAND:
	case OROR:
	case NOT:
	case UNARY CALL:
	case CALL:
	case UNARY STCALL:
	case STCALL:
		/* don't even go near the table... */
		;

		}
	/* get here to do rewriting if no match or
	   fall through from above for hard ops */

	p1 = p->in.left;
	if( ty == BITYPE ) p2 = p->in.right;
	else p2 = NIL;
	
# ifdef DEBUGGING
	if( odebug ){
		prntf( "order( 0x%x, ", p );
		prcook( cook );
		prntf( " ), cookie " );
		prcook( cookie );
		prntf( ", rewrite %s\n", opst[m] );
		}
# endif	/* DEBUGGING */
	switch( m ){
	default:

nomat:

# ifdef DEBUGGING
	if( odebug ){
		prntf( "At order(nomat): order( 0x%x, ", p );
		prcook( cookie );
		prntf( " )\n" );
		fwalk( p, eprint, 0 );
		}
# endif
		cerror( "no table entry for op %s", opst[p->in.op] );

	case COMOP:
		codgen( p1, FOREFF );
		p2->in.rall = p->in.rall;
		codgen( p2, cookie );
		ncopy( p, p2 );
		p2->in.op = FREE;
		goto cleanup;

	case FORCE:
		/* recurse, letting the work be done by rallo */
		{
		int reg;
		p = p->in.left;
/*
# ifdef LCD
		cook = (p->in.fhwside && ISFTP(p->in.type))?
			INTFREG|INTAREG|INTBREG : INTAREG|INTBREG;
# else
		cook = (flibflag || !ISFTP(p->in.type))?
				INTAREG|INTBREG :
				(p->in.fpaside) ? INTDREG|INTAREG|INTBREG :
					          INTFREG|INTAREG|INTBREG;
# endif
*/
		reg = p->in.rall & ~MUSTDO;
		cook = (reg < A0)? INTAREG :
				   (reg< F0)? INTBREG :
					      (reg< FP0)? INTFREG : INTDREG;
		goto again;
		}

	case CBRANCH:
		cbranch( p1, -1, (int)p2->tn.lval );
		p2->in.op = FREE;
		p->in.op = FREE;
		return;

	case DBRA:
		dbra( p );
		tfree(p);
		return;

	case QUEST:
		{
		flag fpanode = (fopseen==p) & (fpaflag > 0);
		flag voided = p2->in.left->in.type==VOID || (cookie & SANY);
		flag fpanode_or_voided = fpanode || voided;

		/* fpanode true iff we are working on the top node of a duped
		   float tree.
		*/
		cbranch( p1, -1, m=GETLAB() );
		p2->in.left->in.rall = p->in.rall;
		if (p2->in.left->in.type == LONGDOUBLE) {
			q = talloc();
			q->in.op = ASSIGN;
			q->in.right = p2->in.left;
			p2->in.left = q;
			p2->in.left->in.rall = p->in.rall;
			p2->in.left->in.left = q = talloc();
			q->in.op = OREG;
			q->in.type = LONGDOUBLE;
			q->tn.rval = 14;
			q->tn.lval = freetemp(SZLONGDOUBLE/SZINT);
			q->in.name = 0;
			q2 = talloc();
			*q2 = *q;
			codgen( p2->in.left,INTEMP);
			}
		else {
			codgen( p2->in.left, (fpanode_or_voided) ? SANY :
					p2->in.left->in.fpaside ?
						(INTAREG|INTBREG|INTDREG) :
						(INTAREG|INTBREG|INTFREG) );
			}
		/* Force right to compute result into same reg used by left
		   unless we are dealing with the ? node of a duplicate float
		   tree. In that case we want to leave it as NOPREF so that
		   extra code to fill a register isn't generated.
		*/
		cbgen( (short)0, m1 = GETLAB(), 'I' );
		deflab( (unsigned)m );
		if (p2->in.type == LONGDOUBLE) {
			p2->in.op = ASSIGN;
/*
			p2->in.right->in.rall = (fpanode_or_voided) ?
					NOPREF : p2->in.left->tn.rval|MUSTDO;
*/
			reclaim( p2->in.left, RNULL, 0 );
			p2->in.left = q2;
			codgen( p2, INTEMP);
			}
		else {
			p2->in.right->in.rall = (fpanode_or_voided) ?
					NOPREF : p2->in.left->tn.rval|MUSTDO;
			reclaim( p2->in.left, RNULL, 0 );
			codgen( p2->in.right, (fpanode_or_voided) ? SANY :
					p2->in.right->in.fpaside ?
						(INTAREG|INTBREG|INTDREG) :
						(INTAREG|INTBREG|INTFREG) );
			}
		deflab( m1 );
		if (fpanode_or_voided && p2->in.type != LONGDOUBLE)
			{
			if (p2->in.left->in.op==FORCE)
				reclaim(p2->in.left, RNULL, 0);
			if (voided || p2->in.right->in.op==FORCE)
				reclaim(p2->in.right, RNULL, 0);
			p2->in.op = FREE;
			p->in.op  = FREE;	/* we're done. No result to speak of. */
			if (voided)
				p->in.type = VOID; /* should already be */
			return;
			}
                if (p2->in.type == LONGDOUBLE) {
                        *p = *p2;
                        p2->in.op = FREE;
                        }
                else {
			p->in.op = REG;  /* set up node describing result */
			p->tn.lval = 0;
			p->tn.rval = p2->in.right->tn.rval;
			p->in.type = p2->in.right->in.type;
			tfree( p2->in.right );
			p2->in.op = FREE;
		}
		goto cleanup;
		}

	case ANDAND:
	case OROR:
	case NOT:  /* logical operators */
		/* if here, must be a logical operator for 0-1 value */
		cbranch( p, -1, m=GETLAB() );
		p->in.op = CCODES;
		p->bn.label = m;
		if (ISFTP(p->in.type))
			/* Float reg comparison. Since the SCC result is
			   already in a D register (and since a logical result
			   is implicitly scalar), avoid further conversions.
			*/
			p->in.type = INT;
		order( p, INTAREG );
		goto cleanup;

	case UNARY MINUS:
		/* the next if clause used to be if (o==SCONV || o==UNARY MINUS...
		*/
		if ( (p1->rn.su_total == 0)
			&& ( (cook==STAREG || cook==STFREG || cook==STDREG) 
				|| (p1->in.op==REG && ISAREG(p1->tn.rval)
					&& !(cook&INTAREG) ) ) )
			{
			/* No match on STFREGS so attempt to get it into a
			   data register first.
			   The reverse case is also supported here.
			*/
			cookie = p->in.fpaside ? (STAREG|STDREG) :
						 (STAREG|STFREG);
			order(p, cookie);
			goto cleanup;
			}
		if ((o == SCONV) && (p->in.type == VOID))
			{ /* Fix up type conversion to void.
			   * Checking for void should occur in pass 1.
			   */
			order(p1,FOREFF);
			p->in.op = FREE;
			return;
			}
		if (o == FLD)
			{	
			/* fields of funny type or trying to go
			   to strange places.
			*/
			if ( p1->in.op == UNARY MUL )
				{
				offstar(p1 /* , cookie */ );
				goto again;
				}
			if (p1->rn.su_total==0 && !(cook&STAREG) )
				{
				cookie = STAREG;
				order(p, cookie);
				goto cleanup;
				}
			}
		if (p1->in.op == REG && ISBREG(p1->tn.rval) )
			/* Things didn't work in an Address reg. Try again
			   in a Data reg.
			*/
			cookie = INTAREG|INTEMP;
		else
			{
			cookie = (!ISFTP(p1->in.type))? INBREG|INAREG|INTAREG :
					p1->in.fpaside ? INTAREG|INTDREG|INBREG :
						         INTAREG|INTFREG|INBREG;
			}
		order( p1,  cookie );
		goto again;

	case NAME:
		/* all leaves end up here ... */
		if( o == REG ) goto nomat;
		order( p, (!ISFTP(p->in.type))? INTAREG|INTBREG :
				(p->in.fpaside)? INTAREG|INTBREG|INTDREG :
					         INTAREG|INTBREG|INTFREG );
		goto again;

	case INIT:
		/* "incorrect initialization" */
		/* UERROR( MESSAGE( 61 )); */
		uerror( "incorrect initialization" );
		return;

	case UNARY CALL:
		p->in.right = NIL;
	case CALL:
		p->in.op = o = UNARY CALL;
		if (((!picoption) && (p1->in.op == ICON)) || ISPIC_OREG(p1)) 
		  {
		  int i;
		  for ( i=0; i < MAXINLINERS; i++ )
		    if ( !strcmp( p1->in.name, inliners[ i ] ) )
		      {
		      if ( geninlinecall( i, p, cook ) ) goto nomat;
		      else goto cleanup;
		      }
		  }
		if( gencall( p ) ) goto nomat;
		goto cleanup;

	case UNARY STCALL:
		p->in.right = NIL;
	case STCALL:
		p->in.op = o = UNARY STCALL;
		if( GENSCALL( p ) ) goto nomat;
		goto cleanup;

		/* if arguments are passed in register, care must be taken that reclaim
		/* not throw away the register which now has the result... */

	case UNARY MUL:
		if( cook == FOREFF ){
			/* do nothing */
			order( p1, FOREFF );
			p->in.op = FREE;
			return;
			}
# if 0	/* should be subsumed by the stref() call after again: */
		if (p1->in.op == STASG)
			{
			/* possible register union assignment problem */
			p2 = p1->in.right;
			p1 = p1->in.left;
			if (p1->in.type == UNIONTY)
				{
				if ( mvunionreg(p1) || mvunionreg(p2) )
					goto again;
				}
			}
# endif /* 0 */
		offstar( p /*, cookie*/ );
		goto again;

	case INCR:  /* INCR and DECR */
		if( setincr( 0 ) ) /* p, cookie */ goto again;

		/* x++ becomes (x += 1) -1; */

		if( cook & FOREFF ){  /* result not needed so inc or dec and be done with it */
			/* x++ => x += 1 */
			p->in.op = (p->in.op==INCR)?ASG PLUS:ASG MINUS;
			goto again;
			}

		p1 = tcopy(p);
		reclaim( p->in.left, RNULL, 0 );
		p->in.left = p1;
		p1->in.op = (p->in.op==INCR)?ASG PLUS:ASG MINUS;
		p->in.op = (p->in.op==INCR)?MINUS:PLUS;
#ifndef FORT
		if ((p->in.type == FLOAT) && !flibflag) 
			{
			/* setasop() will make the += into a double op. Here
			   we must only make the subtract at the top double.
			*/
			p2 = talloc();
			ncopy(p2, p);
			p2->in.type = DOUBLE;
			p2->in.left = makety2(p->in.left, DOUBLE);
			p2->in.right = makety2(p->in.right, DOUBLE);
			p->in.op = SCONV;
			p->in.left = p2;
			}
		/* fix up (++) case for bitfield ops.
		 * x++ => (x+=1)-1
		 * is incorrect in the case where x+=1 overflows the bitfield
		 * size, so add an additional mask of the field size */
		if (p1->in.left->in.op == FLD)
			{
			p2 = tcopy(p);
			reclaim(p->in.left, RNULL, 0);
			p->in.left = p2;
			p->in.op = AND;
			p->in.right->tn.lval = ((1<<p1->in.left->tn.rval)-1);
			}
#endif
		goto again;

	case STASG:
		if( setstr( p	/*, cookie*/ ) ) goto again;
		goto nomat;

	case ASG PLUS:  /* and other assignment ops */
	/*
	case ASG MINUS:
	case ASG OR:
	case ASG ER:
	case ASG AND:
	case ASG LS:
	case ASG RS:
	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
	*/
	/* Note that the switch is on m, not on in.op. m is set by match().
	*/

		if( setasop(p	/*, cookie */) ) goto again;

		p2 = tcopy(p);
		p2->in.op -= 1;		/* remove the assignment addition */
			/* e.g. ASG RS becomes RS */
		p->in.op = ASSIGN;

		/* If there are any side effects on the lhs, remove them now */
		fwalk(p1, rm_seffects, 0);

		reclaim( p->in.right, RNULL, 0 );
		p->in.right = p2;
		/*
		canon(p);
		rallo( p, p->in.rall );

# ifdef DEBUGGING
		if( odebug ) fwalk( p, eprint, 0 );
# endif 

# ifdef LCD
		order( p2->in.left, INTBREG|INTAREG|INTFREG );
		order( p2, INTBREG|INTAREG|INTFREG );
# else
		order( p2->in.left, p2->in.left->in.fpaside ?
					     INTBREG|INTAREG|INTDREG :
					     INTBREG|INTAREG|INTFREG );
		order( p2, p2->in.fpaside ? INTBREG|INTAREG|INTDREG :
				            INTBREG|INTAREG|INTFREG );
# endif
		*/
		goto again;

	case ASSIGN:
	/* setasg moved inline to facilitate easier tree rewriting */
	/* setasg code does setup for assignment operator */
		{
		int tc;
		int dir = 0;

		if ( (cook & FOREFF) && p2->in.op == SCONV )
		  {
		  NODE *q = p2->in.left;
		  int r = q->tn.rval;
		  if ( q->in.op == REG && ! ISTREG( r )
			&& (ISAREG(r) || ISBREG(r)) )
		     {
		     long tosz = typesz( p2 ),
		          fromsz = typesz( p2->in.left );
		     if ( tosz && fromsz ) /* both known type sizes */
		       if ( tosz < fromsz  /* truncation */
			    && !(tosz <= SZCHAR
                                 && ISBREG(r)) )
		       {			/* fold it out */
			q->in.type = p->in.type; /* note, assign node's type */
			p2->in.op = FREE;
		        p->in.right = q;
			goto again;
		       }
		     }
		  }
		if ( !flibflag && ISFTP( p->in.type ) )
		  {
		  /* Sometimes, despite best efforts, an SCONV ends up on the
		     lhs instead of rhs. It should only occur because of tree
		     rewriting and SCONV removal for 68881.
		  */
		    if (!p->in.fpaside)
		    {
		      if (p1->in.op == SCONV)
			  {
			  p->in.type = p1->in.left->in.type;
			  p->in.left = p1->in.left;
			  p1->in.op = FREE;
			  p->in.right = makety2(p2, p->in.type);
			  goto again;
			  }
		      /* There's no direct move of a double to fregs from aregs */
		      if ( p2->in.type == DOUBLE && p2->in.type != p1->in.type
			  && p2->in.op == REG && ISAREG(p2->tn.rval) )
			  {
			  order (p2, INTEMP);
			  goto again;
			  }
		    }
		    if ( p1->in.fsu > p2->in.fsu ) dir = 1;
		    else if ( p1->in.fsu < p2->in.fsu ) dir = 2;
		    else if ( p1->in.su > p2->in.su ) dir = 1;
		    else if ( p1->in.su < p2->in.su ) dir = 2;
		    else if ( FLT_EA( p2 ) && ! FLT_EA( p1 ) ) dir = 1;
		    else if ( p2->rn.su_total != 0 ) dir = 2;
		    else if ( p1->rn.su_total != 0 ) dir = 1;
		    else 
		      goto pastlabel;
		    if ( dir == 1 )
		      { if ( p1->in.op == UNARY MUL || ( p1->in.op == FLD 
				&& p1->in.left->in.op == UNARY MUL ) )
			  offstar( p1 );
			else
			  { 
			  tc = (p1->in.fpaside) ?
				INDREG|INTDREG|INTAREG|INAREG
					|INBREG|SOREG|SNAME|SCON :
				INFREG|INTFREG|INTAREG|INAREG
					|INBREG|SOREG|SNAME|SCON;
			  if ( tc & INTDREG &&
				ISFMONADIC_NOT_DRAGON(p1) )
			      { p1->in.fpaside = 0;
			        tc = (tc & ~(INTDREG|INDREG))|INTFREG|INFREG;
			      }
			  order( p1, tc );
			  }
		      }
		    else
		      { if ( p2->in.op == UNARY MUL )
			  {
				if ( p2->in.left->in.op == STASG)
					{
					/* Possible only if unionflag != 0 */
					p1 = p2->in.left;	/* new use */
					*p2 = *p1;
					p1->in.op = FREE;
					tc = (p2->in.fpaside)?
					      INDREG|INTDREG|INTAREG|INAREG
					        |INTBREG|INBREG :
					      INFREG|INTFREG|INTAREG|INAREG
					        |INTBREG|INBREG;
					order( p2, tc );
					goto again;
					}
			  offstar( p2 );
			  }
			else
			  { 
			    tc = (p2->in.fpaside) ?
				INDREG|INTDREG|INTAREG|INAREG
					|INBREG|SOREG|SNAME|SCON :
				INFREG|INTFREG|INTAREG|INAREG
					|INBREG|SOREG|SNAME|SCON;
			    if ( tc & INTDREG &&
				ISFMONADIC_NOT_DRAGON(p2) )
			      { p2->in.fpaside = 0;
			        tc = (tc & ~(INTDREG|INDREG))|INTFREG|INFREG;
			      }
			    order( p2, tc );
			  }
		      }
		  goto again;
		  }
pastlabel:
		if( p2->rn.su_total != 0 && p2->in.op != REG ) {
		
			if ((p1->in.op == UNARY MUL) &&
			    (p1->rn.su_total > p2->rn.su_total))
				{
				offstar( p1 );
				goto again;
				}
			if( p2->in.op == UNARY MUL )
				{
				if (p2->in.left->in.op == STASG)
					{
					/* Possible only if unionflag != 0 */
					p1 = p2->in.left;	/* new use */
					*p2 = *p1;
					p1->in.op = FREE;
					order(p2,INTAREG|INAREG|INTBREG|INBREG);
					goto again;
					}
				else offstar( p2 );
				}
			else if (((p2->in.op == PLUS) || (p2->in.op == MINUS)) && ISPTR(p2->in.type))
				{
				adrstar(p2); /* order p2 OR leave p2 in SADRREG shape */
				if (adrreg(p2) == SADRREG)
					{
					if ((p1->in.op != REG) || !ISBREG(p1->tn.rval))
						order(p2,INTBREG|INBREG);
					}
				else
					{
					if (p2->in.op != REG)
						order(p2, INTAREG|INAREG|INBREG|INTBREG);
					}
				}
			else
				{
				tc = (!ISFTP(p2->in.type))?
					INAREG|INTAREG|INBREG|SOREG|SNAME|SCON :
					(p2->in.fpaside) ?
					INDREG|INTDREG|INTAREG|INAREG
						|INBREG|SOREG|SNAME|SCON :
					INFREG|INTFREG|INTAREG|INAREG
						|INBREG|SOREG|SNAME|SCON;
				if ( tc & INTDREG &&
					ISFMONADIC_NOT_DRAGON(p2) )
				  {
				  p2->in.fpaside = 0;
				  tc = (tc & ~(INTDREG|INDREG))|INTFREG|INFREG;
				  }
				order( p2, tc );
				}
			goto again;
			}
		if( p1->in.op == UNARY MUL )
			{
			offstar( p1 );
			goto again;
			}
		if( p1->in.op == FLD && p1->in.left->in.op == UNARY MUL )
			{
			offstar( p1->in.left);
			goto again;
			}

		/* allow multiple assignments without going through a float
		 * register unnecessarily */
		if ((p->rn.su_total == 0) && (p2->in.op != REG) &&
		    (p2->in.op != NAME) && (p2->in.op != OREG) &&
		    (p2->in.op != ICON))
			{
			tc = (!ISFTP(p2->in.type))?
				INAREG|INTAREG|INBREG|SOREG|SNAME|SCON :
				(p2->in.fpaside) ?
				INDREG|INTDREG|INTAREG|INAREG|INBREG|SOREG
					|SNAME|SCON :
				INFREG|INTFREG|INTAREG|INAREG|INBREG|SOREG
					|SNAME|SCON;
			if ( tc & INTDREG &&
				ISFMONADIC_NOT_DRAGON(p2) )
			  {
			  p2->in.fpaside = 0;
			  tc = (tc & ~(INTDREG|INDREG))|INTFREG|INFREG;
			  }
			order( p2, tc );
			goto again;
			}
		/* Sometimes, despite the best efforts, an SCONV ends up on the
		   lhs instead of the rhs. It should only occur because of tree
		   rewriting and SCONV removal for 68881.
		*/
		if (!p->in.fpaside)
		{
		    if (p1->in.op == SCONV)
			{
			p->in.type = p1->in.left->in.type;
			p->in.left = p1->in.left;
			p1->in.op = FREE;
			p->in.right = makety2(p2, p->in.type);
			goto again;
			}
		    /* There's no direct move of a double to fregs from aregs */
		    if ( p2->in.type == DOUBLE && p2->in.type != p1->in.type
			&& p2->in.op == REG && ISAREG(p2->tn.rval) )
			{
			order (p2, INTEMP);
			goto again;
			}
		}
		/* if things are really strange, get rhs into a register */
		if( p2->in.op != REG )
			{
			tc = (!ISFTP(p2->in.type))?
				INTAREG|INAREG|INBREG :
				(p2->in.fpaside) ?
					INDREG|INTDREG|INTAREG|INAREG|INBREG :
					INFREG|INTFREG|INTAREG|INAREG|INBREG;
			if ( tc & INTDREG &&
				ISFMONADIC_NOT_DRAGON(p2) )
			  {
			  p2->in.fpaside = 0;
			  tc = (tc & ~(INTDREG|INDREG))|INTFREG|INFREG;
			  }
			order( p2, tc );
			goto again;
			}
		goto nomat;
		}



	case BITYPE:
		if( setbin( p	/*, cookie */ ) ) goto again;
		/* try to replace binary ops by =ops */
		switch(o){

		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case DIVP2:
		case MOD:
		case AND:
		case OR:
		case ER:
		case LS:
		case RS:
			p->in.op = ASG o;
			goto again;
			}
		goto nomat;

		}

	cleanup:

	/* if it is not yet in the right state, put it there */

	if( cook & FOREFF ){
		reclaim( p, RNULL, 0 );
		return;
		}

	if( p->in.op==FREE ) return;

#ifdef FASTMATCH
	if (tshape(p) & cook) return;
#else
	if( tshape( p, cook ) ) return;
#endif /* FASTMATCH */

	if( (m=match(p,cook) ) == MDONE ) return;

	/* we are in bad shape, some implementations try lastchance(); */

	goto nomat;
	}
#pragma OPT_LEVEL 2


store( p ) register NODE *p; {

	/* find a subtree of p which should be stored */

	register short o = p->in.op;
	short ty = (short)optype(o);

	if( ty == LTYPE ) return;

	switch( o ){

	case UNARY CALL:
	case UNARY STCALL:
		++callflag;
		break;

	case UNARY MUL:
		break;

	case CALL:
	case STCALL:
		store( p->in.left );
		stoarg( p->in.right, o );
		++callflag;
		return;

	case COMOP:
		markcall( p->in.right );
		if NEEDTOSTORE(p->in.right)
			SETSTO( p );
		store( p->in.left );
		return;

	case QUEST:
		if (fopseen == p) return;
		/* i.e. if on the top node of a float dup tree, dont store
		   it ever.
		*/
		/* fall thru */
	case ANDAND:
	case OROR:
		markcall( p->in.right );
		if NEEDTOSTORE(p->in.right)
			SETSTO( p );
	case CBRANCH:   /* to prevent complicated expressions on the LHS from being stored */
	case NOT:
		constore( p->in.left );
		return;

		}

	if( ty == UTYPE ){
		store( p->in.left );
		return;
		}


	if NEEDTOSTORE(p)	/* must store */
		mkadrs( p );  	/* set up stotree and stocook to subtree
				 that must be stored */

	store( p->in.right );
	store( p->in.left );
	}

constore( p ) register NODE *p; {

	/* store conditional expressions */
	/* the point is, avoid storing expressions in conditional
	   conditional context, since the evaluation order is predetermined */

	switch( p->in.op ) {

	case ANDAND:
	case OROR:
	case QUEST:
		markcall( p->in.right );
	case NOT:
		constore( p->in.left );
		return;

		}

	store( p );
	}

/* mark off calls below the current node */
markcall( p ) register NODE *p; {

	again:
	switch( p->in.op ){

	case UNARY STCALL:
	case STCALL:
	case UNARY CALL:
	case CALL:
		++callflag;
		return;

		}

	switch( optype( p->in.op ) ){

	case BITYPE:
		markcall( p->in.right );
	case UTYPE:
		p = p->in.left;
		/* eliminate recursion (aren't I clever...) */
		goto again;
	case LTYPE:
		return;
		}

	}

stoarg( p, calltype ) NODE *p; {
	/* arrange to store the args */

	if( p->in.op == CM ){
		stoarg( p->in.left, calltype );
		p = p->in.right ;
		}

	callflag = 0;
	store(p);
	}

/* negatives of relationals */
short negrel[] = { NE, EQ, GT, GE, LT, LE, UGT, UGE, ULT, ULE } ;

extern short revrel[];	/* defined in optim.c */

short fnegrel[]= { FNEQ, FEQ, FNGT, FGT, FNGE, FGE, FNLT, FLT, FNLE, FLE};
short fposrel[]= { FEQ, FNEQ, FLE, FLT, FGE, FGT};
short frevrel[]= { FEQ, FNEQ, FLT, FNLT, FLE, FNLE, FGT, FNGT, FGE, FNGE};

cbranch( p, truelabel, falselabel ) register NODE *p; {
	/* evaluate p for truth value, and branch to truelabel or falselabel
	/* accordingly: label <0 means fall through */

	register short o;
	register lab;
	int flab, tlab;
	flag ftp = ISFTP(p->in.type);

		fpaside = p->in.fpaside;

		o = p->in.op;
		lab = -1;

		switch( o ) {
	
		case ULE:
		case ULT:
		case UGE:
		case UGT:
		if (ftp)
			{
			cerror("impossible relational in cbranch");
			break;
			}
		case EQ:
		case NE:
		case LE:
		case LT:
		case GE:
		case GT:
			if (ftp) o = fposrel[o-EQ];
			if( truelabel < 0 ){
				p->in.op = o = ftp? 
					fnegrel[o-FEQ] : negrel[o-EQ];
				truelabel = falselabel;
				falselabel = -1;
				}
#	ifndef FORT	/* Fortran doesn't know 0.0 when it sees it. */
			if ( ftp && ISFZERO(p->in.right) )
				{
				if ((logop(p->in.left->in.op)) ||
				    (p->in.left->in.op == COMOP)) {
					/* strange situation: e.g., (a!=0) == 0 */
					/* must prevent reference to p->in.left->lable, so get 0/1 */
					/* we could optimize, but why bother */
					codgen( p->in.left, p->in.left->in.fpaside ?
						INAREG|INBREG|INTDREG :
						INAREG|INBREG|INTFREG );
					}
				codgen( p->in.left, FORCC );
				cbgen( o, (unsigned)truelabel, 'I' );
		    		}
		    else
#	endif	/* FORT */
			if( p->in.right->in.op == ICON && p->in.right->tn.lval == 0 &&
				p->in.right->in.name == 0 ){
				switch( o ){
	
# ifdef DEBUGGING
				default:
					{
					cerror("impossible relational in cbranch");
					break;
					}
# endif	/* DEBUGGING */
				case UGT:
				case ULE:
					p->in.op = o = (o==UGT)?NE:EQ;
				case EQ:
				case NE:
				case LE:
				case LT:
				case GE:
				case GT:
				case FNEQ:
				case FEQ:
				case FNGT:
				case FGT:
				case FNGE:
				case FGE:
				case FNLT:
				case FLT:
				case FNLE:
				case FLE:
					if( logop(p->in.left->in.op) ||
						(p->in.left->in.op == COMOP)) {
						/* Strange situation: e.g., (a!=0) == 0.
						   Must prevent reference to 
						   p->in.left->lable, so get 0/1.
						   We could optimize, but why bother.
					  	*/
						codgen( p->in.left, INAREG|INBREG );
						}
					codgen( p->in.left, FORCC );
					cbgen( o, (unsigned)truelabel, 'I' );
					break;
	
				case UGE:
					codgen(p->in.left, FORCC);
					cbgen( 0, (unsigned)truelabel, 'I' );
					/* i.e. unconditional branch */
					break;
				case ULT:
					codgen(p->in.left, FORCC);
					}
				}
			else
				{
				p->bn.label = truelabel;
#ifdef FASTMATCH
				if (!(tshape(p->in.right) & (SCON|SOREG|SNAME|STARREG)) &&
				    (tshape(p->in.left) & (SOREG|SNAME|STARREG)))
#else
				if (!tshape(p->in.right,SCON|SOREG|SNAME|STARREG) &&
				     tshape(p->in.left, SOREG|SNAME|STARREG))
#endif /* FASTMATCH */
					{
					/* reverse the sense of the comparison to get a
				   	better addressing mode on mc68000.
					*/
					NODE *ptemp = p->in.left;
					p->in.op = ftp? frevrel[o-FEQ] : revrel[o-EQ];
					p->in.left = p->in.right;
					p->in.right = ptemp;
					}
				codgen( p, FORCC );
				}
			if( falselabel>=0 ) cbgen( 0, (unsigned)falselabel,'I');
			reclaim( p, RNULL, 0 );
			return;
	
		case ANDAND:
			lab = falselabel<0 ? GETLAB() : falselabel ;
			cbranch( p->in.left, -1, lab );
			cbranch( p->in.right, truelabel, falselabel );
			if( falselabel < 0 ) deflab( (unsigned)lab );
			p->in.op = FREE;
			return;
	
		case OROR:
			lab = truelabel<0 ? GETLAB() : truelabel;
			cbranch( p->in.left, lab, -1 );
			cbranch( p->in.right, truelabel, falselabel );
			if( truelabel < 0 ) deflab( (unsigned)lab );
			p->in.op = FREE;
			return;
	
		case NOT:
			cbranch( p->in.left, falselabel, truelabel );
			p->in.op = FREE;
			break;
	
		case COMOP:
			codgen( p->in.left, FOREFF );
			p->in.op = FREE;
			cbranch( p->in.right, truelabel, falselabel );
			return;
	
		case QUEST:
			flab = falselabel<0 ? GETLAB() : falselabel;
			tlab = truelabel<0 ? GETLAB() : truelabel;
			cbranch( p->in.left, -1, lab = GETLAB() );
			cbranch( p->in.right->in.left, tlab, flab );
			deflab( (unsigned)lab );
			cbranch( p->in.right->in.right, truelabel, falselabel );
			if( truelabel < 0 ) deflab( (unsigned)tlab);
			if( falselabel < 0 ) deflab( (unsigned)flab );
			p->in.right->in.op = FREE;
			p->in.op = FREE;
			return;
	
		case ICON:
			if( p->in.type != FLOAT && p->in.type != DOUBLE ){
	
				if( p->tn.lval || p->in.name ){
					/* addresses of C objects are never 0 */
					if( truelabel>=0 ) 
					    cbgen( 0, (unsigned)truelabel, 'I');
					}
				else if( falselabel>=0 ) 
					cbgen( 0, (unsigned)falselabel, 'I' );
				p->in.op = FREE;
				return;
				}
			/* fall through to default with other strange constants */
	
		default:
			/* get condition codes */
			codgen( p, FORCC );
		if( truelabel >= 0 ) 
			cbgen( ftp? FNEQ : NE, (unsigned)truelabel, 'I' );
		if( falselabel >= 0 )
			cbgen( truelabel >= 0? 0 : (ftp? FEQ : EQ), 
					(unsigned)falselabel, 'I' );
			reclaim( p, RNULL, 0 );
			return;
	
			}
	
}
	


# ifdef DEBUGGING
eprint( p, down, a, b ) NODE *p; int *a, *b; {

	*a = *b = down+1;
	while( down >= 2 ){
		printf( "\t" );
		down -= 2;
		}
	if( down-- ) printf( "    " );


	printf( "0x%x) %s", p, opst[p->in.op] );
	switch( p->in.op ) { /* special cases */

	case REG:
		printf( " %s", rnames[p->tn.rval] );
		break;

	case ICON:
	case NAME:
	case OREG:
		printf( " " );
		adrput( p );
		break;

	case STCALL:
	case UNARY STCALL:
	case STARG:
	case STASG:
		printf( " size=0x%x", p->stn.stsize );
		printf( " align=0x%x", p->stn.stalign );
		break;
		}

	printf( ", " );
	tprint( p->in.type, 0 );
	printf( ", " );
	if( p->in.rall == NOPREF ) printf( "NOPREF" );
	else {
		if( p->in.rall & MUSTDO ) printf( "MUSTDO " );
		else printf( "PREF " );
		printf( "%s", rnames[p->in.rall&~MUSTDO]);
		}
	printf( ", SU= 0x%x, FSU= 0x%x\n", p->in.su, p->in.fsu );

	}
# endif /* DEBUGGING */


oreg2( p ) register NODE *p; {

	/* look for situations where we can turn * into OREG */

	register NODE *q;
	register char *cp;
	register NODE *ql, *qr;
	int	r;
	CONSZ temp;

#if !defined(ONEPASS) && !defined(FORT)
	/* Trees from c1 have constants on the left.  Move them to the right
	 *  so that the shift&add multiply is triggered.
	 */
	if ( p->in.op == MUL && 
	     p->in.left->in.op == ICON &&   /* no-name constant on left */
	     p->in.left->tn.rval == NONAME )
		{
		NODE *sp;
		sp = p->in.right;
		p->in.right = p->in.left;
		p->in.left = sp;
		}
#endif  /* !ONEPASS && !FORT */


	if( p->in.op == UNARY MUL ){
		q = p->in.left;
		if( q->in.op == REG )
			{
			temp = q->tn.lval;
			r = q->tn.rval;
			cp = q->in.name;
			goto ormake;
			}

# if 0
		if (q->in.op == OREG && q->tn.rval < 0)
			{
			union indexpacker x;

			x.rval = q->tn.rval;
			if (x.i.mode != ADDIND)
			    goto notindex;
			x.i.indexed = 1;
			x.i.mode = MEMIND_PRE;
		  	p->in.op = OREG;
			p->tn.name = q->tn.name;
			p->tn.lval = q->tn.lval;
			p->tn.rval = x.rval;
			tfree(q);
			return;
			}
# endif /* 0 */

		if( q->in.op != PLUS && q->in.op != MINUS ) return;
		ql = q->in.left;
		qr = q->in.right;


		/* look for doubly indexed expressions */
		if ((q->in.op == PLUS)
			|| (q->in.op == MINUS))
			{
		  	int bd = 0;
			/* NOTE: For MEMIND_POST/PRE we will need bd as well as
			   od. Some further mapping must be devised onto the 
			   ndu definition.
			*/
			char *name = 0;
		  	union indexpacker xx;
			    	/* union indexpacker
				    	{
					    	int rval;
					    	struct
						    	{
						    	unsigned char indexed:1;
							unsigned char mode:4;
						    	unsigned char  pad:3;
						    	unsigned char addressreg:8;
						    	unsigned char xreg:8;
						    	unsigned char scale:8;
						    	} i;
				    	}
			    	*/

			if (q->in.op == PLUS &&
				(INDEXREG(qr) || !goodindexer(ql) ))
				{
				/* swap for canonicalization */
				q->in.left = qr;
				q->in.right = qr = ql;
				ql = q->in.left;
				}
			/* Since the top is U* and we've swapped, the lhs must
			   be a ptr at this time. (Or at least in an A register
			   due to casting or some other tree collapsing
			   mechanism.)
			*/
			xx.i.indexed = 1;
			xx.i.mode = ADDIND;
			if (ql->in.op == REG)
				{
				xx.i.addressreg = ql->tn.rval;
				goto rightsideindex;
				}
			/* the following is slower than the simple code */
# if 0
			if (ql->in.op == ICON || ql->in.op == NAME)
				{
				xx.i.addressreg = 0;
				bd = ql->tn.lval;
				name = ql->tn.name;
				goto rightsideindex;
				}
# endif	/* 0 */
			if ( regplusoffset(ql) )
					{
					bd += (ql->in.right->tn.lval);
					if (ql->in.op == MINUS) bd = -bd;
					xx.i.addressreg = ql->in.left->tn.rval;
					name = ql->in.right->tn.name;
rightsideindex:
					if (qr->in.op == REG)
						{
						xx.i.xreg = qr->tn.rval;
						xx.i.scale = 0;
					  	if (q->in.op == MINUS)
					     	  goto notindex;
						}
					else if ( goodscaler(qr, &xx) )
					  {
					  if (q->in.op == MINUS)
					    goto notindex;
					  }
					/* variable "xx" is set in goodscaler()
					   as side effects.
					*/

					else if ((qr->in.op == PLUS)
						|| (qr->in.op == MINUS))
						{
						NODE *qrr;
						if (qr->in.op == PLUS
						   &&(qr->in.left->in.op == ICON
						   || qr->in.left->in.op == NAME))
							{
							/* canonicalize */
							NODE * regtemp;
							regtemp = qr->in.left;
							qr->in.left = qr->in.right;
							qr->in.right = regtemp;
							}
						qrr = qr->in.right;

						/* NAME op disallowed -- only
						 * ICON accepted -- sjo
						 */
						if ((qrr->in.op == ICON) &&
						    (qrr->tn.name == NULL) &&
						    goodscaler(qr->in.left,&xx))
							{
							if (qr->in.op == MINUS)
							  bd -= qrr->tn.lval;
							else
							  bd += qrr->tn.lval;
							if (q->in.op == MINUS)
					  		  goto notindex;
							/* Everything else done
							   in goodscaler()
					   		   as side effects.
							*/
							}
						else goto notindex;
						}
					else goto notindex;

					tfree(q);
					p->in.op = OREG;
					p->tn.name = name;
					p->tn.lval = bd;
					p->tn.rval = xx.rval;
					return;
					}
				}


notindex:
		if( qr->in.op == ICON && ql->in.op==REG && szty(qr->in.type)==1) {
			temp = qr->tn.lval;
			if( q->in.op == MINUS ) temp = -temp;
			r = ql->tn.rval;
			temp += ql->tn.lval;
			cp = qr->in.name;
			if( cp && ( q->in.op == MINUS || ql->in.name ) ) return;
			if( !cp ) cp = ql->in.name;

ormake:
			if( notoff( /* p->in.type,*/ r, temp, cp ) )
				return;
			p->in.op = OREG;
			p->tn.rval = r;
			p->tn.lval = temp;
			p->in.name = cp;	/* just copying the ptr */
			tfree(q);
			return;
			}
		}

	else if( (p->in.op == PLUS) && (p->in.left->in.op == UNARY MINUS) ){
		/* convert trees from       PLUS      to       MINUS
					   /    \             /     \
				 UNARY MINUS	 OP2        OP2      OP1
					|
				       OP1
		*/

		ql = p->in.left;
		p->in.op = MINUS;
		p->in.left = p->in.right;
		p->in.right = ql->in.left;
		ql->in.op = FREE;
		}
	else
	  if ( picoption )
	    pictrans( p );

	}


adrreg( p ) register NODE *p; {

	/* Extract adress info from SADRREG shape and put in adrregnode */
	/* Since we don't actually change the tree to a new node type   */
	/* the info is put in the global variable adrregnode            */

	register NODE *q;
	register char *cp;
	register NODE *ql, *qr;
	int	r;
	CONSZ temp;
	union  indexpacker xx;
  	int bd = 0;
	char *name = 0;

	q = p;

	if( q->in.op != PLUS && q->in.op != MINUS ) return(0);
	ql = q->in.left;
	qr = q->in.right;
	if ((ql->in.op == REG) && (qr->in.op == REG)) return(0);

	/* NOTE: For MEMIND_POST/PRE we will need bd as well as
	   od. Some further mapping must be devised onto the 
	   ndu definition.
	*/
	    	/* union indexpacker
		    	{
			    	int rval;
			    	struct
				    	{
				    	unsigned char indexed:1;
					unsigned char mode:4;
				    	unsigned char  pad:3;
				    	unsigned char addressreg:8;
				    	unsigned char xreg:8;
				    	unsigned char scale:8;
				    	} i;
		    	}
	    	*/

	if ((q->in.op == PLUS) && (INDEXREG(qr) || !goodindexer(ql)))
		{
		/* swap for canonicalization */
		q->in.left = qr;
		q->in.right = qr = ql;
		ql = q->in.left;
		}
	/* Since the top is U* and we've swapped, the lhs must
	   be a ptr at this time. (Or at least in an A register
	   due to casting or some other tree collapsing
	   mechanism.)
	*/
	xx.i.indexed = 1;
	xx.i.mode = ADDIND;
	if (INDEXREG(ql))
		{
		xx.i.addressreg = ql->tn.rval;
		goto rightsideindex;
		}

	if ( regplusoffset(ql) )
		{
		bd += (ql->in.right->tn.lval);
		if (ql->in.op == MINUS) bd = -bd;
		xx.i.addressreg = ql->in.left->tn.rval;
		name = ql->in.right->tn.name;
rightsideindex:
		if (qr->in.op == REG)
			{
			xx.i.xreg = qr->tn.rval;
			xx.i.scale = 0;
		  	if (q->in.op == MINUS)
		     	  goto notindex;
			}
		else if ( goodscaler(qr, &xx) )
		  {
		  if (q->in.op == MINUS)
		    goto notindex;
		  }
		else if ((qr->in.op == PLUS)
			|| (qr->in.op == MINUS))
			{
			NODE *qrr;
			if ((qr->in.op == PLUS) && (qr->in.left->in.op == ICON
			   || qr->in.left->in.op == NAME))
				{
				/* canonicalize */
				NODE * regtemp;
				regtemp = qr->in.left;
				qr->in.left = qr->in.right;
				qr->in.right = regtemp;
				}
			qrr = qr->in.right;
			if ( (qrr->in.op == ICON) && (qrr->tn.name == NULL)
			   && goodscaler(qr->in.left, &xx) )
				{
				if (qr->in.op == MINUS)
				  bd -= qrr->tn.lval;
				else
				  bd += qrr->tn.lval;
				if (q->in.op == MINUS)
		  		  goto notindex;
				}
			else goto notindex;
			}
		else goto notindex;

		adrregnode.in.op = OREG;
		adrregnode.tn.name = name;
		adrregnode.tn.lval = bd;
		adrregnode.tn.rval = xx.rval;
		return(SADRREG);
		}
notindex:
	return(0);
	}

/* goodscaler() returns TRUE if the subtree p is an appropriate index register
   plus scale factor. NOTE: Side effect of setting indexpacker union fields.
*/
LOCAL goodscaler(p, indexp)	register NODE *p; union indexpacker *indexp;
{
	if (p->in.op==LS && p->in.left->in.op==REG
			 && p->in.right->in.op==ICON
			 && p->in.right->tn.lval <= 3
			 && p->in.right->tn.lval >= 0)
			{
			indexp->i.scale = (unsigned char)(p->in.right->tn.lval);
			indexp->i.xreg = p->in.left->tn.rval;
			return (TRUE);
			}
	if (p->in.op == REG)
			{
			indexp->i.scale = 0;
			indexp->i.xreg = p->tn.rval;
			return (TRUE);
			}
	return (FALSE);
}


/* goodindexer() returns TRUE iff p points to a tree node that is an
   acceptable indexing register or something that can be made so.
*/
LOCAL goodindexer(p)	register NODE *p;
{
	return (INDEXREG(p) || ISPTR(p->in.type) || regplusoffset(p) );
}


/* regplusoffset() returns TRUE iff p points to a treenode that is an
   acceptable indexing register plus an offset.
*/
LOCAL regplusoffset(p)	register NODE *p;
{
	return ( (p->in.op == PLUS || p->in.op == MINUS)
		&& INDEXREG(p->in.left) && p->in.right->in.op==ICON );
}

canon(p) NODE *p; {
	/* put p in canonical form */
	int oreg2(), sucomp();

	if ( p->in.op == FREE )
		return;		/* already reclaimed, must be void */

	walkf( p, oreg2 );  /* look for and create OREG nodes */
#ifdef MYCANON
	MYCANON(p);  /* your own canonicalization routine(s) */
#endif
	fcallflag = 0;	/* marks calls on fhwside for fhw */
	callflag = 0;	/* new use for callflag distinct from its use in store() */
	walkf( p, sucomp );  /* do the Sethi-Ullman computation */
	callflag = 0;

	}


#ifdef FORT
/* cbranch_rm_sconvs -- remove extraneous SCONV's on floating-point logical
 *			ops -- they cause poor code gen
 *	input:  tree to be checked
 *	output: head node of (modified) tree 
 *
 * We have to be careful to remove only SCONV's between the top-of-tree
 * CBRANCH and first lower-level logical op's.  Once we get below any ops,
 * removing an SCONV may cause incorrect code.
 */
NODE *cbranch_rm_sconvs(p)
NODE *p;
{
	register NODE *ret = p;
	if ((p->in.op == SCONV) && logop(p->in.left->in.op))
		{
		ret = p->in.left;	/* remove this SCONV */
		p->in.op = FREE;
		}
#if 0
	if (logop(ret->in.op))		/* logop -- both children are
					 * candidates, too */
		{
		ret->in.left = cbranch_rm_sconvs(ret->in.left);
		if (optype(ret->in.op) == BITYPE)	
			ret->in.right = cbranch_rm_sconvs(ret->in.right);
		}
#endif
	return(ret);	/* not logical OP or SCONV above logop --
			 *  stop traversal right here */
}
#endif


dbra(p)
NODE *p;
{
	flag is_short;

	/* dbf works on short (word) values only.  If the value
	   is an integer the clr/subq/bgt is needed.    - sje   */

	is_short = (tsize2(p->in.left->in.type) <= sizeof(short));
	expand(p, FOREFF, "\tdbf\tAL,");
	prntf("L%d\n", p->in.right->tn.lval);
	if (!is_short)
		{
		expand(p, FOREFF, "\tclr.w\tAL\n");
		expand(p, FOREFF, "\tsubq.l\t&1,AL\n");
		prntf("\tbgt.w\tL%d\n", p->in.right->tn.lval);
		}
}  /* dbra */
