/* file local2.c */
/*	SCCS	REV(64.3);	DATE(92/04/03	14:22:07) */
/* KLEENIX_ID @(#)local2.c	64.3 92/02/04 */

# include <malloc.h>
# include "mfile2"
/* a lot of the machine dependent parts of the second pass */

/* information on how tree of shape SADRREG should be turned into address */
/* set by adrreg when recognizing shape SADRREG */
extern NODE adrregnode;

/* Opcodes passed to the quad (long double) comparision routine */

#    define LDEQ  4
#    define LDLT  8
#    define LDGT 16

# define MAXSPOFF 32767	/* 2^16-1 (largest value usable as disp for link inst)*/
# define BITMASK(n) ((1L<<n)-1)
# define BYPI	(SZINT/SZCHAR)	/* bytes per integer */
# define USEDREG(r)  ((r < F0)? (usedregs |= 1<<(r)): (usedfdregs |= 1<<(r-F0)))

# define YES 1
# define NO  0

#ifdef FORT
#ifdef DBL8
# define STACK_BOUNDRY 8
int stack_offset = 0;
#endif /* DBL8 */
#endif /* FORT */

short picreg = A2;	/* addresss reg reserved for PIC table base (GOT) */
			/* gets bumped to A3 if A2 used for dragon base */
short noreg_dragon = 0;	/* 0 => normal processing
			   non-zero => dragon option on, but cur. rtn does'nt
			   	       req'r dragon base reg accesses */
short noreg_PIC = 0;	/* 0 => normal processing
			   non-zero => PIC option on, but cur. rtn does'nt
			   	       req'r PIC base reg accesses */
flag pic_used = NO;	/* NO => no actual PIC refs gen'ed by cdg
			   YES => actual PIC refs gen'ed by cdg */
short must881opseen; 	/* 1 == seen op that must be done on 881 */
short fpaside;		/* true iff working on DRAGON fpa code */
# define A2BIT 0x0400	/* bit positions within "movm" register mask */
# define A3BIT 0x0800
			/* PIC base reg bit position in movm reg mask */
# define PICREGBIT(x) ((x==A2) ? A2BIT : A3BIT)



	NODE * fopseen;		/* pts to fhw '?' iff a simple flt pt op was
						seen (+-/*)
					*/

int usedregs;		/* flag word for registers used in current routine */
int could_trash_flt_regs = NO;	/* could "perm" flt regs get trashed? */
int calling_others = NO;	/* does current routine have call that
					could trash "perm" flt regs? */
int incoming_flt_perms = NO;	/* flt regs alloc'ed before codegen? */


int usedfdregs;		/* flag word for flt pt regs used in current routine */

LOCAL int moveneeded = 0; /* flag for shift/add optimization */
LOCAL int inst_cost = 0;  /* cost for shift/add optimization */

extern char	*addtreeasciz();	/* in common or comm1.c */
#ifdef FORT1PASS
extern long	autoleng;
#endif

#if !defined(FORT) && defined(ONEPASS)
# define ISFZERO(p) ((p)->in.op == NAME && (p)->tn.rval == -fzerolabel && fzerolabel != 0)
#endif


/*************************************************************************/

put_flt_version_directive()	/* set object file "version" directive */
{
#ifndef FLINT
	int n;
	
				/* Non-LCD target */
	if ( could_trash_flt_regs )
	  n = 3;	/* possible incompatibility with old object */
	else
	  n = 2;	/* old object is compatible */

	prntf("\tversion\t%d\n",n);
#endif /*!FLINT*/
} /* put_flt_version_directive */


# define DRAGON_PROLOG_ALLOC_SZ 20  /* number of pairs per alloc chunk */

# define DRAGON_ALLOC_CHUNK(pair_cnt) (pair_cnt * sizeof(long int) * 2);

long int *dragon_prologs=NULL;	/* head of array of prolog label pairs */
long int *last_dragon_prolog=NULL; /* last pair of prolog labels */
long int *max_dragon_prolog=NULL; /* tail of prolog label pairs */

static unsigned int dragon_prolog_tbl_sz=0; /* "current" size of prolog table */



/*************************************************************************/

init_dragon_prologs()
{
	dragon_prolog_tbl_sz = DRAGON_ALLOC_CHUNK( DRAGON_PROLOG_ALLOC_SZ );
	if ((dragon_prologs = (long int *)malloc(dragon_prolog_tbl_sz)) == NULL)
	  cerror("can't allocate floating point register prolog label table");
	max_dragon_prolog =
		(long int *)((int)dragon_prologs + dragon_prolog_tbl_sz);
	last_dragon_prolog = NULL;
} /* init_dragon_prologs() */


next_dragon_prolog()
{
	if ( dragon_prologs == NULL )
	  init_dragon_prologs();
	if (last_dragon_prolog == NULL)
	  last_dragon_prolog = dragon_prologs;
	else if ((last_dragon_prolog + 2) >= max_dragon_prolog)
	  {
	  int dif = last_dragon_prolog - dragon_prologs;
	  dragon_prolog_tbl_sz += DRAGON_ALLOC_CHUNK( DRAGON_PROLOG_ALLOC_SZ );
	  if ((dragon_prologs = 
	      (long int *)realloc((char *)dragon_prologs,dragon_prolog_tbl_sz))
			== NULL)
	    cerror("can't expand floating point register prolog label table");
	  max_dragon_prolog =
		(long int *)((int)dragon_prologs + dragon_prolog_tbl_sz);
	  last_dragon_prolog = dragon_prologs + dif + 2;
	  }
	else
	  last_dragon_prolog += 2;
	*last_dragon_prolog = GETLAB();
	*(last_dragon_prolog+1) = GETLAB();
} /* next_dragon_prolog() */


reset_dragon_prologs()
{
	if ( dragon_prologs != NULL )
	  {
	  last_dragon_prolog = NULL;
	  }
} /* reset_dragon_prologs() */



/* do_ext_prolog - bfcode() utility to emit extended prolog branch & label
*/
do_ext_prolog( oflag, ftnno )
{
	if ( oflag )
		    {
		    next_dragon_prolog();
		    prntf("\tbra.l\tL%d\n", *last_dragon_prolog);
		    prntf("L%d:\n", *(last_dragon_prolog+1));
		    }
	else
		    prntf( "\tbsr.l\tLPROLOG%d\n", ftnno );
} /* do_ext_prolog() */




lineid( l, fn ) char *fn; {
#ifndef FLINT
	/* identify line l and file fn */
	prntf( "#	line %d, file %s\n", l, fn );
#endif /* FLINT */
	}

LOCAL cntbits(i) register short i;
  {	register short j,ans;
	for (ans=0, j=0; i!=0 && j<16; j++) { if (i&1) ans++; i>>= 1; }
	return(ans);
}




eobl2(optimizing)
	int optimizing;
	{
#ifndef FLINT
	register OFFSZ spoff;	/* offset from stack pointer */
	int regsz;
 	int fmask, dmask;
 	int fcnt, dcnt;
	int useddregs;

#ifdef FORT1PASS
	spoff = autoleng;
#else
	spoff = maxoff/SZCHAR;
#endif /* FORT1PASS */
	SETOFF(spoff,2);
	usedregs &= 036374;	/* only save regs used for reg vars */
	useddregs = usedfdregs & 0xFFFF00;
 	usedfdregs &= 0xFFF8FC;
 	fmask = 0; fcnt = 0; dmask = 0; dcnt = 0;
 	if (usedfdregs)
 		{ register int i;
		for (i=0; i <= (F7 - F0); i++)
 		  {
 		  if ( usedfdregs & (1 << i) ) /* 881 flt reg used */
 		    {
 		    fmask |= (1 << ( 7 - i ) );
 		    fcnt++;
 		    }
		  }
		for (i=0; i <= (FP15 - FP0); i++)
		  {
		  if ( usedfdregs & (1 << (i + 8)) ) /* dragon reg used */
		    {
 		    dmask |= (1 << ( 15 - i ) );
 		    dcnt++;
		    }
		  }
		}
	if ( flibflag || !fpaflag )
	  dcnt = dmask = useddregs = 0; /* short circuit to nullify dragon */
	if ( picoption )
	  {
	  if ( dmask && (fpaflag > 0) )
	    {			/* will emit +bfpa dragon tst in PIC */
	    usedregs |= PICREGBIT( picreg );
	    pic_used = YES;
	    }
	  if ( (usedregs & PICREGBIT(picreg)) && !noreg_PIC )
	    pic_used = YES;
	  }
	if ((fpaflag && noreg_dragon && useddregs) ||
            (picoption && noreg_PIC && pic_used))
	  cerror("Global optimizer does not integrate with this version of codegen");
	if ( useddregs )		/* implied dragon base reg use */
	  usedregs |= A2BIT;
 	regsz = 4*cntbits(usedregs);
 	spoff += regsz + (fcnt*12) + (dcnt*8);
 	if (fpaflag && dmask && !flibflag)
	  {
	  long tlab;
	  if ( fpaflag > 0 )
	    {
	    tlab = GETLAB();
	    if (picoption)
              {
	      prntf( "\tmov.l\t%s(%s),%%a0\n", FPANAME, rnames[picreg] );
	      prntf( "\ttst.w\t(%%a0)\n");
              }
	    else
              {
	      prntf( "\ttst.w\t%s\n", FPANAME );
              }
	    prntf( "\tbeq\tL%d\n", tlab );
	    }
	  dragon_list( 1, (long)( spoff - ( regsz + fcnt * 12 ) ) );
	  if ( fpaflag > 0 )
	    prntf( "L%d:\n", tlab );
	  }
	if (fmask && !flibflag)
 	  prntf("\tfmovm.x\tLFF%d(%%a6),&LSF%d\n", ftnno, ftnno);
	if (usedregs)
 		prntf( "\tmovm.l\t(%%sp),&LS%d\n", ftnno );
	prntf( "\tunlk\t%%a6\n\trts\n" );
	if ( picoption || (fpaflag && !flibflag) ) /* need extended prolog */
	  if ( optimizing )
	    {
	    long int *k;
	    for ( k = dragon_prologs; k <= last_dragon_prolog;  k += 2  )
	      {
	      prntf( "L%d:\n", *k );
	      if (pic_used)
		init_picbase();
	      if (dmask && fpaflag > 0 )
		{
		if (picoption)
                  {
	          prntf( "\tmov.l\t%s(%s),%%a0\n", FPANAME, rnames[picreg] );
	          prntf( "\ttst.w\t(%%a0)\n");
                  }
		else
                  {
	          prntf( "\ttst.w\t%s\n", FPANAME );
                  }
		prntf( "\tbeq.l\tL%d\n", *(k+1) );
		}
	      if (useddregs)
	        prntf( "\tlea.l\tfpa_loc,%%a2\n" );
	      if (dmask)
	        dragon_list( 0, (long)( spoff - ( regsz + fcnt * 12 ) ) );
	      prntf( "\tbra.l\tL%d\n", *(k+1) );
	      }
	    }
	  else
	    {
	      long tlab;
	      prntf( "LPROLOG%d:\n", ftnno );
	      if (pic_used)
		init_picbase();
	      if ( dmask && fpaflag > 0 )
		{
		tlab = GETLAB();
		if (picoption)
                  {
	          prntf( "\tmov.l\t%s(%s),%%a0\n", FPANAME, rnames[picreg] );
	          prntf( "\ttst.w\t(%%a0)\n");
                  }
		else
                  {
	          prntf( "\ttst.w\t%s\n", FPANAME );
                  }
		prntf( "\tbeq\tL%d\n", tlab );
		}
	      if (useddregs)
	        prntf( "\tlea.l\tfpa_loc,%%a2\n" );
	      if (dmask)
	        dragon_list( 0, (long)( spoff - ( regsz + fcnt * 12 ) ) );
	      if ( dmask && fpaflag > 0 )
		prntf( "L%d:\n", tlab );
	      prntf( "\trts\n" );
	    }
	reset_dragon_prologs();
#ifdef FORT
#ifdef DBL8
	prntf( "\tset\tLF%d,-%d\n", ftnno,
		(((spoff+STACK_BOUNDRY-1)/STACK_BOUNDRY)*STACK_BOUNDRY));
#else
	prntf( "\tset\tLF%d,-%d\n", ftnno, spoff );
#endif /* DBL8 */
#else
	prntf( "\tset\tLF%d,-%d\n", ftnno, spoff );
#endif /* FORT */
	prntf( "\tset\tLS%d,%d\n", ftnno, usedregs );
	if ( !flibflag )
	  {
 	  prntf( "\tset\tLFF%d,-%d\n", ftnno, (spoff-regsz) );
 	  prntf( "\tset\tLSF%d,%d\n", ftnno, fmask );
	  }
	if ( incoming_flt_perms && calling_others )
	  could_trash_flt_regs = YES;
	calling_others = NO;
	pic_used = NO;
/*	if (oldsysflag) prntf( "\tset\tLM%d,-%d\n", ftnno, maxtoff ); */
	/* LF<#> is stack space needed for auto variables. */
	/* LS<#> is a bit mask for used registers (in movem form) */
	/* LM<#> is stack space needed for temporaries. */

/*	maxtoff = 0; */
#endif /* not FLINT */
	}

#ifdef FORTY
#ifndef INST_SCHED
#define INST_SCHED
#endif
#endif

#ifndef INST_SCHED
init_picbase()
{
	register char *r = rnames[ picreg ];

	prntf( "\tmov.l\t&%s,%s\n", PIC_GOT_NAME, r );
	prntf( "\tlea.l\t-6(%%pc,%s.l),%s\n", r, r );
} /* init_picbase() */
#else
init_picbase()
{
	register char *r = rnames[ picreg ];
	register tlab = GETLAB();

	prntf( "L%d:\tmov.l\t&%s,%s\n", tlab, PIC_GOT_NAME, r );
	prntf( "\tlea.l\tL%d+2(%%pc,%s.l),%s\n", tlab, r, r );
} /* init_picbase() */
#endif




dragon_list( direction, bd )
		int direction;		/* 0 => save; !0 => restore regs */
		register long bd;	/* displ of stack frame save area */
{
	register short i;

	for (i=0; i<= (FP15-FP0); i++)
	      {
	      if ( usedfdregs & (1 << (i + 8)) )
	          {
  		  if ( direction )
	            prntf("\tfpmov.d\t-%d(%%a6),%%fpa%d\n", bd, i );
		  else
	            prntf("\tfpmov.d\t%%fpa%d,-%d(%%a6)\n", i, bd );
		  bd -= 8;
	          }
	      }
}




struct hoptab { short opmask; char * opstring; } ioptab[]= {

	ASG PLUS, "add",
	ASG MINUS, "sub",
	ASG OR,	"or",
	ASG AND, "and",
	ASG ER,	"eor",
	ASG MUL, "mul",
	ASG DIV, "div",
	ASG MOD, "div",
	ASG LS,	"sl",
	ASG RS,	"sr",
	-1, ""    };

hopcode( fp, o ) char *fp;	short o;{
#ifndef FLINT
	/* output the appropriate string from the above table */

	register struct hoptab *q;

	for( q = ioptab;  q->opmask>=0; ++q ){
		if( q->opmask == o ){
			prntf( "%s", q->opstring );
			prntf( (*fp=='F')? "f" : (*fp=='D')?"d" : "" );
			return;
			}
		}
	cerror( "no hoptab for %s", opst[o] );
#endif /* not FLINT */
	}






char *
rnames[]= {  /* keyed to register number tokens */

	"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
	"%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp",
	"%fp0", "%fp1", "%fp2", "%fp3", "%fp4", "%fp5", "%fp6", "%fp7"
       ,"%fpa0", "%fpa1", "%fpa2", "%fpa3", "%fpa4", "%fpa5", "%fpa6",
	"%fpa7", "%fpa8", "%fpa9", "%fpa10", "%fpa11", "%fpa12", "%fpa13",
	"%fpa14", "%fpa15"
	};






unsigned rstatus[] = {
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG,

	SBREG|STBREG, SBREG|STBREG,
	SBREG|STBREG, SBREG|STBREG,
	SBREG|STBREG, SBREG|STBREG,
	SBREG,	      SBREG,

	SFREG|STFREG,	SFREG|STFREG,
	SFREG|STFREG,	SFREG|STFREG,
	SFREG|STFREG,	SFREG|STFREG,
	SFREG|STFREG,	SFREG|STFREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	};

/* zzzcode() returns a value equal to the number of advances to the cp ptr. */

/* zzzcode() returns a value equal to the number of advances to the cp ptr. */
zzzcode( p, cookie, cp ) register NODE *p; char *cp;
{
#ifndef FLINT
	register TWORD m;
	register int temp,temp2;
	char *suf;

	/* NOTE: second actual param to expand() is never interrogated on
		mc68000
	*/
	switch( *cp )
	{
	case 'a':	/* FMONADIC expansion (DRAGON fpa only) */
		prntf("%s", p->in.left->tn.name);
		break;

	case 'b':	/* like 'B' but no .s or .d suffix possible */
	case 'B':
		m = p->in.type;		/* fall into suffix: */

suffix:		switch (m)
		{
		case CHAR :
		case UCHAR: 	suf = "b";
				break;
		case SHORT :
		case USHORT :
				suf = "w";
				break;
		case FARG :
				/* FARG in its true sense is impossible by now.
				   It's being used here simply to mark a 68881
				   source register.
				*/
				suf = "x";
				break;
		case FLOAT :
				suf = (*cp == 'b')? "l":"s";
				break;
		case DOUBLE :
				suf = (*cp == 'b')? "l":"d";
				break;
		default : 	
				suf = "l";
		}
		prntf(".%s", suf);
		break;

	case 'C':
		if ( p->in.op == UNARY STCALL ) 
			prntf("\tlea	%d(%%a6),%%a1\n", p->stn.stsize );
			/* stn.stsize is (ab)used here to store temp offset
			   that was originally set in genscall().
			*/
		p = p->in.left;

		switch ( p->in.op )
		{
	  	case ICON:	
#ifdef FORTY
				if (fortyflag)
					prntf("\tbsr.l\t");
				else
					prntf("\tjsr\t");
#else
				prntf("\tjsr\t");
#endif /* FORTY */
				acon(p);
				break;

	  	case REG:	prntf("\tjsr\t");
				prntf("(");
				adrput(p);
				prntf(")");
				break;

	  	case OREG:	
				/* This hack fixes the problem of pictrans
				   converting all names and icons to oregs
				   but the bsr instruction for a call generates
				   a name and not an oreg reference.  The
				   rbusy fixes the problem of freeing %a2 
				   even though we don't really use it.
				   This all wouldn't be neccessary if pictrans
				   didn't change call ICONS to OREGS.
				*/

				if ( ISPIC_OREG( p ) )
				  { /* transformed ICON, revert to "optimize" */
				  prntf( "\tbsr.l\t" );
				  acon( p );
				  prntf("\n");
				  rbusy(picreg,(TWORD)(PTR+INT));
				  break;
				  }
				/* else fall thru */

	  	case NAME:
				prntf("\tmov.l\t");
				adrput(p);
				prntf(",%%a0\n\tjsr\t(%%a0)");
				break;

	  	default:
#ifdef DEBUGGING
				cerror("bad subroutine name");
#else
				goto error;
#endif
		}
		break;

	case 'D':	/* double to double move */
			/* or float to float move */
		if ( (ISFNODE(p->in.left) && p->in.right->in.op != REG)
		     || (ISFNODE(p->in.right) && p->in.left->in.op != REG))
		  expand(p, FOREFF, "\tfmovZB\tAR,AL\n");
		else 
		  if ( (ISDNODE(p->in.left) && p->in.right->in.op != REG)
			|| (ISDNODE(p->in.right) && p->in.left->in.op != REG))
		    expand(p, FOREFF, "\tfpmovZB\tAR,AL\n");
		else
		  if (p->in.right->in.op == REG && p->in.left->in.op == REG)
		   rmove(p->in.left->tn.rval, p->in.right->tn.rval, p->in.type,
				cookie);
		else  
		  /* no flt regs & not reg->reg, expand the normal way */
		  if (p->in.type == DOUBLE)
		    {
		    NODE *rtdecr=NULL, *rtincr=NULL,
			 *lftdecr=NULL, *lftincr=NULL;
		    /* expand(p, FOREFF, "\tmov.l\tUR,UL\n\tmov.l\tAR,AL\n");
		    */
#ifdef FASTMATCH
		    if ((p->in.right->in.op == UNARY MUL) &&
			(tshape(p->in.right) & STARREG))
#else
		    if ( p->in.right->in.op == UNARY MUL &&
			 tshape( p->in.right, STARREG ) )
#endif /* FASTMATCH */
		      if ( p->in.right->in.left->in.op == INCR )
		        rtincr = p->in.right->in.left->in.left;
		      else
		        rtdecr = p->in.right->in.left->in.left;
#ifdef FASTMATCH
		    if ((p->in.left->in.op == UNARY MUL) &&
			(tshape(p->in.left) & STARREG))
#else
		    if ( p->in.left->in.op == UNARY MUL &&
			 tshape( p->in.left, STARREG ) )
#endif /* FASTMATCH */
		      if ( p->in.left->in.left->in.op == INCR )
		        lftincr = p->in.left->in.left->in.left;
		      else
		        lftdecr = p->in.left->in.left->in.left;
		    if ( rtincr != NULL || lftincr != NULL )
		      if ( lftdecr != NULL || rtdecr != NULL )
			{	/* one pre-decr, one post-incr opd <ea>s */
			NODE *t;
			if ( rtdecr != NULL )
			  {
			  t = p->in.right->in.left;
			  p->in.right->in.op = OREG;
			  p->in.right->in.rall = rtdecr->in.rall;
			  p->in.right->tn.lval = rtdecr->tn.lval;
			  p->in.right->tn.rval = rtdecr->tn.rval;
			  p->in.right->in.name = rtdecr->in.name;
			  prntf("\tsubq.l\t&8,%s\n", rnames[rtdecr->tn.rval]);
			  prntf("\tmov.l\t(%s),(%s)+\n",
					rnames[rtdecr->tn.rval],
			  		rnames[lftincr->tn.rval]);
			  prntf("\tmov.l\t4(%s),", rnames[rtdecr->tn.rval]);
			  adrput( p->in.left );
			  prntf("\n");
			  USEDREG( rtdecr->tn.rval );
			  tfree( t );
			  }
			else	/* rtincr != NULL && lftdecr != NULL */
			  {
			  t = p->in.left->in.left;
			  p->in.left->in.op = OREG;
			  p->in.left->in.rall = lftdecr->in.rall;
			  p->in.left->tn.lval = lftdecr->tn.lval;
			  p->in.left->tn.rval = lftdecr->tn.rval;
			  p->in.left->in.name = lftdecr->in.name;
			  prntf("\tsubq.l\t&8,%s\n", rnames[lftdecr->tn.rval]);
			  prntf("\tmov.l\t(%s)+,(%s)\n",
					rnames[rtincr->tn.rval],
			  		rnames[lftdecr->tn.rval]);
			  prntf("\tmov.l\t");
			  adrput( p->in.right );
			  prntf(",4(%s)\n", rnames[lftdecr->tn.rval]);
			  USEDREG( lftdecr->tn.rval );
			  tfree( t );
			  }
			}
		      else /* at least one post-incr mode, no pre-decr mode */
			{
			prntf("\tmov.l\t");
			if ( rtincr ) upput_dbl( p->in.right );
			else adrput( p->in.right );
			prntf(",");
			if ( lftincr ) upput_dbl( p->in.left );
			else adrput( p->in.left );

		        prntf("\n\tmov.l\t");
			if ( rtincr ) adrput( p->in.right );
			else upput_dbl( p->in.right );
		        prntf(",");
			if ( lftincr ) adrput( p->in.left );
			else upput_dbl( p->in.left );
			prntf("\n");
			}
		    else /* neither opd is post increment mode <ea> */
		      {
		      prntf("\tmov.l\t");
		      upput_dbl( p->in.right );
		      prntf(",");
		      upput_dbl( p->in.left );
		      expand( p, FOREFF, "\n\tmov.l\tAR,AL\n");
		      }
		    if (cookie & FORCC)
			{
			if (p->in.fpaside)
			    {
			    order(p->in.left,INTDREG);
			    expand(p, FOREFF, "\tfptest.d\tAL\n");
			    }
			else
			    expand(p, FOREFF, "\tftest.d\tAL\n");
			}
		    }
		  else
		    {
		    expand(p, FOREFF, "\tmov.l\tAR,AL\n");
		    if (cookie & FORCC)
			{
			if (p->in.fpaside)
			    {
			    order(p->in.left,INTDREG);
			    expand(p, FOREFF, "\tfptest.s\tAL\n");
			    }
			else
			    expand(p, FOREFF, "\tftest.s\tAL\n");
			}
		    }
		break;


	/* The following is used to put a double OREG into a register
	   The order is chosen in order to eliminate the problem of
	   using the destination register of the first move in the second
	   of the two moves needed to accomplish the double move.
	*/
	case 'd':
		if (p->tn.rval < 0)
			{
			union indexpacker x;
			x.rval = p->tn.rval;
			if (x.i.xreg == (resc[0].tn.rval+1))
				expand(p,FOREFF,"\tmov.l\tAR,A1\n\tmov.l\tUR,U1");
			else
				expand(p,FOREFF,"\tmov.l\tUR,U1\n\tmov.l\tAR,A1");
			}
		else
			expand(p,FOREFF,"\tmov.l\tUR,U1\n\tmov.l\tAR,A1");
		break;


	/* The following 2 cases differ only in that 'e' forces expansion to
           LONG by ext's whereas 'E' does the minimum ext's (or none) to make
	   the two types the same. Simply a way to minimize templates.
	*/
	case 'E':
	case 'e':
		{
		register TWORD mr;
		m = (*cp++ == 'E')? p->in.left->in.type : LONG;
		mr = (p->in.right)? p->in.right->in.type : p->in.type;
		switch (*cp)
			{
			case 'L':
				(void)rext(p, "AL\n", mr, m);
				break;
			case 'l':
				if ( !rext(p, "AL\n", m, mr) 
					&& ( cookie & FORCC ) )
				  { char c;
				  if ( p->in.type == CHAR ) c = 'b';
				  else c = 'w';
				  prntf( "\ttst.%c\t", c );
				  expand( p, FOREFF, "AL\n" );
				  }
				break;
			case 'R':
				(void)rext(p, "AR\n", mr, m);
				break;
			case 'r':
				(void)rext(p, "AR\n", m, mr);
				break;
			case '1':
				(void)rext(p, "A1\n", m, mr);
				break;
			default:
#ifdef DEBUGGING
				cerror("bad suffix for ZE in zzzcode()" );
				break;
#else
				goto error;
#endif
			}

		return(1);
		}


	case 'F':
		/* do moves only for moves that don't go to the same place. */
		/* lhs is assumed to be a reg */
		if ( (cookie&FORCC && !p->in.fpaside) ||
			p->in.right->in.op !=REG ||
			p->in.left->tn.rval != p->in.right->tn.rval)
			{
			/* nothing */
			}
		else
			{
			m = -1;
			while (*cp++) m++;
			return(m);
			}
		break;

	case 'G':	/* Double FORARGS */
		if (p->in.op != UNARY MUL)
			if ( ISFNODE(p) )
				{
				/* least significant part first */
				expand(p, FORARG, p->in.fpaside ?
						"Z9\tfpmov.d\tAR,Z-\n" :
						"Z9\tfmov.d\tAR,Z-\n");
				}
			else
				expand(p, FORARG,"\tmov.l\tUR,Z-\n\tmov.l\tAR,Z-\n");
		else	/* STARREG - probably only for increment/decrement */
			{
			register NODE *l = p->in.left;
			register NODE *q = l->in.left;
			short lop = l->in.op;

			p->in.op = OREG;
			p->in.rall = q->in.rall;
			p->tn.lval = q->tn.lval;
			p->tn.rval = q->tn.rval;
			p->in.name = q->in.name;
			expand(p, FORARG,"\tmov.l\tUR,Z-\n\tmov.l\tAR,Z-\n");
			tfree(l);
			if (lop == INCR)
				prntf("\taddq.w\t&8,%s\n",rnames[q->tn.rval]);
			else if (lop == DECR)
				prntf("\tsubq.w\t&8,%s\n",rnames[q->tn.rval]);
			else
#ifdef DEBUGGING
				cerror("illegal STARREG reference in ZG");
#else
				goto error;
#endif
			}
		break;

	case 'H':		/* Integer division by a power of 2 */
		switch( *(++cp) )
		{
		  case '1':
			/* We are dividing by a constant 2 */
			expand(p, FOREFF, "\tasrZL\tAR,AL\n" );
			prntf("\tbpl.b\tL%d\n",m=GETLAB() );
			prntf("\tbcc.b\tL%d\n",m );
			expand(p, FOREFF, "\taddqZL\t&1,AL\n" );
			deflab((unsigned)m);
			break;
		  case '8':
			/* We are shifting by a constant <= 8.
			 * This can be done by using immediate operand to the
			 * 'lsl' instruction.
			 */
			expand(p, FOREFF, "\ttstZL\tAL\n");
			prntf("\tbpl.b\tL%d\n", m=GETLAB() );
			expand(p, FOREFF, 
			       "\tnegZL\tAL\n\tlsrZL\tAR,AL\n\tnegZL\tAL\n");
			prntf("\tbra.b\tL%d\n", temp=GETLAB() );
			deflab((unsigned)m);
			expand(p, FOREFF, "\tlsrZL\tAR,AL\n");
			deflab((unsigned)temp);
			break;
		  case '0':
			/* The catch all */
			expand(p, FOREFF, "\tmovq\tAR,A1\n\ttstZL\tAL\n");
			prntf("\tbpl.b\tL%d\n", m=GETLAB() );
			expand(p, FOREFF,
			       "\tnegZL\tAL\n\tlsrZL\tA1,AL\n\tnegZL\tAL\n");
			prntf("\tbra.b\tL%d\n", temp=GETLAB() );
			deflab((unsigned)m);
			expand(p, FOREFF, "\tlsrZL\tA1,AL\n");
			deflab((unsigned)temp);
			break;
		}
		return( 1 );         /* Since we incremented 'cp' */

	case 'J':
		/* expansion of scalar constant field assignments */
		{
		register CONSZ val;
		unsigned typelsz = SZCHAR * tsize2(p->in.left->in.type);

		temp = 1;
		/* looking first at the field to be filled */
		val = p->in.left->tn.rval;
		fldsz = UPKFSZ(val);
		fldshf = UPKFOFF(val);
		/* now the value to assign into it */
		val = p->in.right->tn.lval;
		if (m = (val<0) ) val = (-val);	/* m is a flag here for <0 */
		/* count necessary bits for the constant */
		if (val < 0)	/* val must be 0x80000000 which is its own reciprocal */
			temp = SZINT - 1;
		else
			while (val >>= 1) temp++;
		if (m) temp++;

		/* now make the mask */
		val = p->in.right->tn.lval;
		val &= (1<<fldsz)-1; /* mask off overflow bits */
		if (m)
			{
			val <<= ( temp = typelsz - fldsz );	/* new temp */
			val = (unsigned)val >> (temp);
			}
		val <<= typelsz - fldshf - fldsz;

		adrcon(val);
		break;
		}

	case 'c':
		/* Single precision name modifier for fmul, fdiv.  */
		/* This is being ifdef'ed out because the 040 does */
		/* not support fsglmul				   */
		break;

	case 'h':
		/* global or external name initialization */
		m = tsize2(p->in.left->in.type);
		prntf("\t%s", m == sizeof(int) ? "long" :
				   m == sizeof(short) ? "short" : "byte");
		break;

		/* stack management macros */
	case 'l':
		m = p->in.left->in.type;
		if (ISFTP(m)) m = INT;
		goto suffix;

	case 'L':
		m = p->in.left->in.type;
		if (ISFTP(m))
			{
			temp = ISFNODE(p->in.left);
			if (optype(p->in.op) == BITYPE)
			   temp &= ISFNODE(p->in.right);
			if (temp) m = FARG;
			}
		goto suffix;

	case 'N':  /* logical ops, turned into 0-1 */
		/* use register given by register 1 */
		cbgen( (short)0, (unsigned)(m=GETLAB()), 'I' );
		deflab( p->bn.label );
		prntf( "	movq	&0,%s\n", rnames[temp = getlr( p, '1' )->tn.rval] );
		USEDREG(temp);
		deflab( (unsigned)m );
		break;

        case 'm': /* multiply by constant
                   * Attempt to turn multiply into shift/add sequence
                   * For the 68020 the following is true:
                   *
                   *     size - each shift/add takes two bytes
                   *            multiply takes eight bytes
                   *     speed - one multiply is as fast as eighteen
                   *            shift and add instructions.
                   *
                   * For the 68040 we should revisit this issue.
                   */

# define MAXSHFTADDS 24	/* threshold for multiply convers. to shift/add seq. */
# define MAXCOST 44

		{
		  NODE *constant = p->in.right;
		  int i;
                  char *code[MAXSHFTADDS];           /* result instructions */
		  int icon;
		  int negative = 0;
		  moveneeded = 0;                    /* flag */
		  inst_cost = 0;
		  icon = (int) constant->in.left;
		  if (icon < 0) {
		    negative = 1;
		    inst_cost += 2;
		    i = factor((unsigned)(-icon),code,(MAXSHFTADDS-1));
		  }
		  else if (icon == 0) {
		    expand(p,FOREFF,"\tclr.l\tAL\n");
		    break;
		  }
		  else i = factor((unsigned)icon,code,MAXSHFTADDS);
		  if (moveneeded) inst_cost += 2;
		  if (inst_cost <= MAXCOST) {
		    if (negative) expand(p,FOREFF,"\tneg.l\tAL\n");
		    if (moveneeded) expand(p,FOREFF,"\tmov.l\tAL,A1\n");
		    while (i--) { expand(p,FOREFF,code[i]); }
		  }
		  else expand(p,FOREFF,"\tmulQZR\tAR,AL\n");
		  break;
		}

	case 'M': /* 'M' must preceded 'O' immediately */
		/* negated CR */
		p->in.right->tn.lval = (-p->in.right->tn.lval);
		/* fall thru */

	case 'O':
		conput( getlr( p, 'R' ) );
		p->in.right->tn.lval = (-p->in.right->tn.lval);
		break;

	case '-': /* '-' must precede 'P' immediately */
		prntf( "-(%%sp)" );
		/* fall thru */

	case 'P':
		break;

	case 'r':	/* undocumented rounding mode support */
		if (froundflag == 0)
			expand(p, FOREFF, p->in.fpaside ? "\tfpintrzZL\tAL" :
						       "\tfintrz.x\tAL");
		else if (p->in.fpaside)
			expand(p, FOREFF, "\tfpcvlZL\tAL");
		break;

	case 'q':
		/* Should never be called with p->fpaside set */
		if (p->in.fpaside) cerror("Unexpected fpa register usage");
		expand(p, FOREFF, "\tfmov.l\tAL,A1\n");
		expand(p, FOREFF, "\ttst.l\tA1\n");
		prntf("\tbgt.b\tL%d\n",temp = GETLAB());
		prntf("\tbeq.b\tL%d\n",temp2 = GETLAB());
		expand(p, FOREFF, "\tfcmp.l\tAL,A1\n");
		prntf("\tfble.w\tL%d\n",temp2);
		expand(p, FOREFF, "\taddq.l\t&1,A1\n");
		prntf("\tbra.b\tL%d\nL%d:",temp2,temp);
		expand(p, FOREFF, "\tfcmp.l\tAL,A1\n");
		prntf("\tfbge.w\tL%d\n",temp2);
		expand(p, FOREFF, "\tsubq.l\t&1,A1\n");
		prntf("L%d:\n",temp2);
		break;

	case 'R':
		p = getlr(p, 'R');
		/* FARG is an impossible type at this point. It's being used here
		   to mark a 68881 source register.
		*/
		m = ISFNODE(p)? FARG : p->in.type;
		goto suffix;

/* # ifndef FORT */
	case 's':	/* structure assignment */
		{

		long size;

		size = p->stn.stsize;
		if ( size == 1 || size == 2 || size == 4 )
		  {	/* do it with single move instruction */
		  char szch = (size==1)? 'b' : (size==2)? 'w' : 'l';
		  p->in.right->in.op = OREG;
		  prntf("\tmov.%c\t", szch);
		  expand( p, FOREFF, "AR,AL\n" );
		  p->in.right->in.op = REG;
		  }
		else
		  {	/* use block copy utility routine */
		  NODE *l, *t;
		  short rn;
		  l = p->in.left;
		  if ( l->in.op != NAME && l->in.op != OREG )
		    cerror("Illegal left hand side in structure assignment");
		  if ( size > 0 )
		    {
		    rn = resc[1].tn.rval;
		    if ( ! (l->in.op == OREG && l->tn.rval == rn ) )
		      {
		      rbusy( rn, INT );
		      expand( p, FOREFF, "\tlea\tAL,A2\n" );
		      t = talloc();
                      *t = *l;
                      reclaim(t, RNULL, 0);
		      }
		    l->tn.op = REG;
		    l->tn.rall = NOPREF;
		    l->tn.type = PTR | l->in.type;
		    l->tn.rval = rn;
		    l->tn.lval = 0;
		    cpyblk(l, p->in.right, size, resc[0].tn.rval, FOREFF);
		    }
		  else
		    cerror("Invalid structure size for assignment generation");
		  }

		break;
		}
/* # endif FORT */

	case 'T':
		/* Truncate longs for type conversions:
		    INT|UNSIGNED -> CHAR|UCHAR|SHORT|USHORT
		   increment offset to second word */

		m = p->in.type;
		p = p->in.left;
		switch( p->in.op )
		{
		case NAME:
		case OREG:
			if (p->in.type==SHORT || p->in.type==USHORT)
			p->tn.lval += (m==CHAR || m==UCHAR) ? 1 : 0;
			else p->tn.lval += (m==CHAR || m==UCHAR) ? 3 : 2;
			break;
		case REG:
			break;
		default:
#ifdef DEBUGGING
			cerror( "Illegal ZT type conversion" );
#else
			goto error;
#endif

		}
		break;

#ifndef FORT
	case 'U':
		/* Similar to ZY. Used to generate FLD tests when lhs is a
		   STARNM, not an OREG.
		*/
		fldsz = UPKFSZ(p->tn.rval);
		expand(p, FORCC, "\tbftst\t(A1)");
		prntf("{&%d:&%d}", UPKFOFF(p->tn.rval), fldsz);
		break;
# endif /* ifndef FORT */

	case 'V':	/* push constant onto stack as argument */
	    	m = p->tn.type;
		if (p->tn.name == NULL)		/* simple constant */
			{
			p->tn.type = INT;
				/* to ensure length=4 for update of the 
				stack later */
			m = p->tn.lval;
			prntf("\tpea\t%d", m);
			}
		else
			{
			/* external name */
			prntf("\tpea\t%s", p->tn.name);
			if (m=p->tn.lval)	/* offset */
				prntf("%s%d", m>0? "+":"", m);
			}
/*		goto fixspoff; */
		break;

	case 'W':	/* structure size */
		if( p->in.op == STASG )
			prntf( "%d", p->stn.stsize);
		else
# ifdef DEBUGGING
			cerror( "Not a structure" );
# else
			goto error;
# endif
		break;

# ifndef FORT
	case 'Y':	/* field assignment of constant value */
		m = fldsz;
		if (ISUNSIGNED(p->in.left->in.type)) {
		    if ((~((1<<fldsz)-1))&p->in.right->tn.lval)
			werror("constant too big for field");
		} else {
		    int mask=(~((1<<fldsz)-1))>>1;
		    if ((mask&p->in.right->tn.lval)&&
			(mask&(~p->in.right->tn.lval)))
			werror("constant too big for field");
		}
		if (m > 1)
			expand(p, FOREFF, (p->in.right->tn.lval)?
				"andZB\t&N,AL\n\torZB\t&ZJ,AL" :
				"andZB\t&N,AL" );
		else	/* a bit field of size 1 */
			{
			register NODE *l = p->in.left;
			suf = (p->in.right->tn.lval&01)? "set" : "clr";
			m = UPKFOFF(l->tn.rval);	/* the offset */
			while (m >= 8)
				{
				l->in.left->tn.lval++;
				m -= 8;
				}
			prntf("b%s\t&%d,", suf, SZCHAR - m - 1 /* fldsz */);
			adrput(l);
			}
		return (1);

	case '0':	/* FLD = x */
		m = UPKFOFF(p->in.left->tn.rval);	/* offset */
		while (m >= 8)
			{
			p->in.left->in.left->tn.lval++;
			m -= 8;
			}
	    	prntf("bfins\t");
	    	adrput(p->in.right);
	    	prntf(",");
	    	adrput(p->in.left);
	    	prntf("{&%d:&%d}", m, fldsz);
		if (cookie&INTAREG) {
			prntf("\n\tbfext%c\t",(ISUNSIGNED(p->in.left->in.type)||compatibility_mode_bitfields)?'u':'s');
			adrput(p->in.left);
			prntf("{&%d:&%d}", m, fldsz);
			prntf(",");
			adrput(p->in.right);
		}
		return(0);
# endif	/* FORT */

	case '1':	/* SCONV fixer for unsigned ops */
		m =    tsize2(p->in.left->in.type);
		temp = tsize2(p->in.type);
		if (m >= temp) break;	/* nothing to do */
		if (ISUNSIGNED(p->in.left->in.type))
		    prntf ("\tand.l\t&%s,", (m == 2)? "0xFFFF" : "0xFF") ;
		else
		    {
		    if ((m==1) && (temp==2))
			prntf("\text.w\t");
		    else if ((m==1) && (temp==4))
			prntf("\text.b.l\t");
		    else if ((m==2) && (temp==4))
			prntf("\text.l\t");
		    }
		adrput( getlr(p, 'L') );
		break;

	case '2':	/* UNARY MINUS on float/double */
		m = p->in.left->in.type;
		if (ISFNODE(p->in.left) ) expand(p, FOREFF,"Zg");
		else 
			{
			if (m == DOUBLE) expand(p, FOREFF, "\tmovZb\tUL,U1\n");
			expand(p, FOREFF, "\tmovZb\tAL,A1\n");
			}
		temp = GETLAB();
		if (ISFTP(m))
			prntf("\tbeq.b\tL%d\n", (unsigned)temp);
		switch(m)
		{
		case DOUBLE:
		case FLOAT:
			suf = "\teor.l\t&0x80000000,A1\n"; 
			break;
		default:
			suf = "\tnegZB\tA1\n"; 
			break;
		}
		expand(p, FOREFF, suf);
		deflab((unsigned)temp);
		break;

	case '3':
	case '4':
		/* like ZE or Ze except not for SCONV nodes. Mostly for mul nodes */
		{
		register TWORD mr;
		m = (*cp++ == '3')? SHORT : LONG;
		mr = (p->in.right)? p->in.right->in.type : p->in.type;
		switch (*cp)
			{
			case 'L':
				(void)rext(p, "AL\n", mr, m);
				break;
			case 'l':
				(void)rext(p, "AL\n", m, mr);
				break;
			case 'R':
				(void)rext(p, "AR\n", mr, m);
				break;
			case '1':
				(void)rext(p, "A1\n", m, mr);
				break;
			default:
#ifdef DEBUGGING
				cerror("bad suffix for Z3 in zzzcode()" );
				break;
#else
				goto error;
#endif
			}

		return(1);
		}

	case '5':		/* possible float prefix for ops on 68881 */
		if (ISFTP(p->in.type))
			prntf("f");
	  	break;

	case '6':		/* FLD in rvalue context */
	case '7':		/* FLD FORCC in rvalue context */

		{
		NODE *pl = p->in.left;
		flag isareg = pl->in.op == REG && ISAREG( pl->tn.rval);

		fldsz = UPKFSZ(p->tn.rval);
		temp = UPKFOFF(p->tn.rval);	/* offset */
		if ( isareg )
		  {
		  if ( temp >= SZLONG )
		    cerror("Unexpected bit field offset for register operand");
		  }
		else
		  while (temp >= SZCHAR)
			{
			p->in.left->tn.lval++;
			temp -= SZCHAR;
			}
		if (*cp == '6')
			{
#ifndef FORT
			if (ISUNSIGNED(p->in.type)||compatibility_mode_bitfields)
#else
			if (ISUNSIGNED(p->in.type))
#endif
				expand(p, FOREFF, "\tbfextu\tAL");
			else
				expand(p, FOREFF, "\tbfexts\tAL");
		    	prntf("{&%d:&%d}", temp, fldsz);
			expand(p, FOREFF, ",A1");
			}
		else
			{
			expand(p, FORCC, "\tbftst\tAL");
	    		prntf("{&%d:&%d}", temp, fldsz);
			}
		break;
		}

	case '8':		/* FMONADIC expander */
		prntf("%s.x\t", p->in.left->tn.name);
		adrput(p->in.right);
		break;

	case '9':		/* toff adjuster for fmov to/from stack */
				/* Necessary because "Z-" doesn't understand
				   8 byte moves to the stack.
				*/
		break;

	default:
error:
		cerror( "illegal zzzcode" );
	}
#endif /* not FLINT */
	return(0);
}








/* rmove() does difficult register-to-register moves */


rmove( rdest, rsource, type, cookie ) register rdest, rsource; TWORD type;
{
#ifndef FLINT
	short sz = szty(type);
	char szch = (type == DOUBLE)? 'd' : (type==FLOAT) ? 's' : 'l';

	if (rdest == rsource)
		{
		if ( flibflag )
		  { /* a cheat to get the count right for later rfree */
		  ++busy[rdest];
		  if ( (sz == 2) && (rdest < F0) )
			++busy[rdest+1];
		  }
		return;		/* This kind of move can do nothing useful.
				   Never used to set condition codes. */
		}

	if (ISDREG(rsource) && ISDREG(rdest))	/* src & dest DRAGON fpa */
			/* WARNING: will not set condition codes correctly */
	    prntf("\tfpmov.%c\t%s,%s\n", szch, rnames[rsource], rnames[rdest]);
	else if (ISFREG(rsource) && ISFREG(rdest))/* src & dest 68881 */
	    prntf("\tfmov.x\t%s,%s\n", rnames[rsource], rnames[rdest]);
	else if (ISDREG(rdest))	/* dest DRAGON fpa */
		{
			/* WARNING: will not set condition codes correctly */
		if (ISFREG(rsource))
		  {
			/* use the stack as intermediate store */
			prntf("\tfmov.%c\t%s,-(%%sp)\n", szch, rnames[rsource]);
			prntf("\tfpmov.%c\t(%%sp)+,%s\n", szch, rnames[rdest]);
		  }
		else
		  {
		  if (sz == 2)	/* double */
			{
			prntf("\tfpmov.d\t%s:%s,%s\n",rnames[rsource],
					rnames[rsource+1], rnames[rdest]);
			USEDREG(rsource+1);
			}
		  else		/* float */
			prntf("\tfpmov.%c\t%s,%s\n", szch,
				rnames[rsource], rnames[rdest]);
		  }
		}
	else if ( ISFREG(rdest))	/* dest 68881 */
		{
		if (ISDREG(rsource))
		  {
			/* use the stack as intermediate store */
		  prntf("\tfpmov.%c\t%s,-(%%sp)\n",szch,rnames[rsource]);
		  prntf("\tfmov.%c\t(%%sp)+,%s\n", szch, rnames[rdest]);
		  }
		else
		  {
		  if (sz == 2)	/* double */
			{
			/* use the stack as intermediate store */
			prntf("\tmov.l\t%s,-(%%sp)\n", rnames[rsource+1]);
			prntf("\tmov.l\t%s,-(%%sp)\n", rnames[rsource]);
			prntf("\tfmov.d\t(%%sp)+,%s\n", rnames[rdest]);
			USEDREG(rsource+1);
			}
		  else		/* float */
			prntf("\tfmov.%c\t%s,%s\n", szch,
				rnames[rsource], rnames[rdest]);
		  }
		}
	else if (ISDREG(rsource))	/* src DRAGON fpa */
		{
		/* typically used only to satisfy FORCE to d0 requests */
		if (sz == 2)	/* double */
			{
			/* WARNING: will not set condition codes correctly */
			prntf("\tfpmov.d\t%s,%s:%s\n", rnames[rsource],
				rnames[rdest], rnames[rdest+1] );
			USEDREG(rdest+1);
			}
		else		/* float */
			prntf("\tfpmov.%c\t%s,%s\n", szch,
				rnames[rsource], rnames[rdest]);
		}
	else if (ISFREG(rsource))	/* src 68881 */
		{
		/* typically used only to satisfy FORCE to d0 requests */
		if (sz == 2)	/* double */
			{
			/* use the stack as intermediate store */
			/* WARNING: will not set condition codes correctly */
			prntf("\tfmov.d\t%s,-(%%sp)\n", rnames[rsource]);
			prntf("\tmov.l\t(%%sp)+,%s\n", rnames[rdest]);
			prntf("\tmov.l\t(%%sp)+,%s\n", rnames[rdest+1]);
			USEDREG(rdest+1);
			}
		else		/* float */
			prntf("\tfmov.%c\t%s,%s\n", type==FLOAT? 's' : 'l',
				rnames[rsource], rnames[rdest]);
		}
	else	/* float registers aren't involved at all */
		{
		if (sz == 2)
			{
			prntf("\tmov.l\t%s,%s\n",rnames[rsource+1],
				rnames[rdest+1]);
			USEDREG(rdest+1);
			USEDREG(rsource+1);
			}
		prntf("\tmov.l\t%s,%s\n",rnames[rsource], rnames[rdest]);
		if ( (cookie & FORCC) && !flibflag && ISFTP(type) )
		  {		/* use stack to get condition code set */
		  if (sz == 2)
		    prntf("\tmov.l\t%s,-(%%sp)\n", rnames[rdest+1]);
		  prntf("\tmov.l\t%s,-(%%sp)\n", rnames[rdest]);
		  prntf("\tftest.%c\t(%%sp)+\n", (sz==2)? 'd': 's');
		  }
		}

	USEDREG(rsource);
	USEDREG(rdest);
#endif /* FLINT */
}






struct respref
respref[] = {
	INTAREG|INTBREG,	INTAREG|INTBREG|INAREG|INBREG,
	INTFREG,		INTFREG|INFREG,
	INTDREG,		INTDREG|INDREG,
	INAREG|INBREG,		INAREG|INBREG|SOREG|STARREG|SNAME|STARNM|SCON,
	INTEMP,			INTEMP,
	FORARG,			FORARG,
	INTAREG,		SOREG|SNAME|SCON,
	INTFREG,		SOREG|SNAME|SAREG|STAREG|SBREG|STBREG,
	INTDREG,		SOREG|SNAME|SAREG|STAREG|SBREG|STBREG,
	0,			0 };


/* Setregs called for every function. Sets fregs (# of available scratch regs),
   and the rstatus array (a dynamic array detailing whether each reg is a 
   temporary or dedicated to a register var).
*/
setregs(){
	register int i;
# ifdef FORT
	register int naddrregs = 5;
	usedregs = 0;	/* we never call bfcode() from fortran. */
 	usedfdregs = 0;
# else	/* FORT */
	register int naddrregs = (maxtreg>>8)&0377;
		/* maxtreg is a packed integer, high byte for addr regs.
		   maxtreg numbers are the highest non-reserved registers which
		   can be used as temps. Therefore naddrregs is the number
		   of the highest address register var. usable as a temp.
		*/

# endif	/* FORT */

	/* use any unused variable registers as scratch registers */
	maxtreg &= 0377;
	fregs = maxtreg>=MINRVAR ? maxtreg + 1 : MINRVAR;
	i = maxtfdreg & 0xFF;
	ffregs = (i >= MINRFVAR) ? (i + 1) : MINRFVAR;
	i = (maxtfdreg >> 16) & 0xFFFF;
	dfregs = (i >= MINRDVAR) ? (i + 1) : MINRDVAR;
	if ( fpaflag > 0 )
	  ffregs = dfregs;	/* doing 1 to 1 mapping: dragon to 881 */
	afregs = naddrregs;
# ifdef DEBUGGING
	if( d2debug ){
		/* -d changes number of free regs to 1, -dd to 2, etc. */
		if( d2debug < fregs ) fregs = d2debug;
		}
	if (fdebug)
		{
		/* -f changes number of free fhw regs to 1, -ff to 2, etc. */
		if ( fdebug < ffregs) ffregs = fdebug;
		}
# endif

	/* first the data registers (aregs in the compiler's vocabulary) */
	for( i=MINRVAR; i<=MAXRVAR; i++ )
		rstatus[i] = i<fregs ? SAREG|STAREG : SAREG;
		/* i.e. if fregs = 2, then d0, d1 would be usable as scratch */

	/* Now the address registers (bregs in the compiler's vocabulary).
	*/
	for( i=minrvarb; i<=MAXRVAR; i++ )
		rstatus[i+8] = i<=naddrregs ? SBREG|STBREG : SBREG;
	if (fpaflag && !noreg_dragon)
				/* A2 used for FPA (dragon) base reg */
		rstatus[MINRVAR+8] = SBREG;
	if (picoption && !noreg_PIC)
			 /* picreg used for Position Indep. Code base reg */
		rstatus[ picreg ] = SBREG;

	/* Now the floating point registers
	*/

	for ( i = MINRFVAR; i <= MAXRFVAR; i++ )
	  rstatus[ i + F0 ] = (i < ffregs) ? SFREG|STFREG 
					   : (incoming_flt_perms=YES, SFREG);
	for ( i = MINRDVAR; i <= MAXRDVAR; i++ )
	  rstatus[ i + FP0 ] = (i < dfregs) ? SDREG|STDREG 
					    : ( (fpaflag>0 && i>MAXRFVAR) ? 0
						 : (incoming_flt_perms=YES),
					      SDREG);
	if ( fpaflag > 0 )
	  for ( i = MAXRFVAR + 1; i <= MAXRDVAR; i++ )
	    {
	    rstatus[ i + FP0 ] = SDREG|STDREG;
	    dfregs++;
	    }


	}




# ifdef DEBUGGING
szty2(t) TWORD t; { /* size, in 32-bit words, needed to hold thing of type t */
	/* really is the number of registers to hold type t */
 	return((t==DOUBLE) ? 2 : 1);
	}
# endif /* DEBUGGING */








genscall( p ) NODE *p;
/* structure valued function call */
{
	register int i = p->stn.stsize;
	i = i/BYPI + (i % BYPI ?  1:0);
	p->stn.stsize = freetemp(i)/SZCHAR;	/* new use for stsize field */
	return( gencall( p ) );
}

shltype( o, p ) short o; NODE *p; {
	if ( daleafop(o) ) return(1);
	return( o==UNARY MUL && shumul(p->in.left) );
	}

flshape( p ) register NODE *p; {
	if ( daleafop(p->in.op) ) return(1);
	return( p->in.op==UNARY MUL && shumul(p->in.left)==STARNM );
	}

/* shtemp() returns TRUE if p points to a NDOE that is acceptable in lieu
   of a temporary.
*/
shtemp( p ) register NODE *p; {
	if( p->in.op == UNARY MUL ) p = p->in.left;
	if( p->in.op == REG ) return( !ISTREG( p->tn.rval ) );
	return( daleafop(p->in.op) );
	}

spsz( t, v ) TWORD t; CONSZ v; {

	/* is v the size to increment something of type t */

	if( !ISPTR(t) ) return( 0 );
	t = DECREF(t);

	if( ISPTR(t) ) return( v == 4 );

	switch( t ){

	case UCHAR:
	case CHAR:
		return( v == 1 );

	case SHORT:
	case USHORT:
		return( v == 2 );

	case INT:
	case UNSIGNED:
	case FLOAT:
		return( v == 4 );

	case DOUBLE:
		return( v == 8 );
		}

	return( 0 );
	}




shumul( p ) register NODE *p; {

	if( INDEXREG(p) ) return( STARNM );

	if( p->in.op == INCR && INDEXREG(p->in.left) && p->in.right->in.op==ICON
		&& p->in.right->in.name == NULL
		&& spsz( p->in.left->in.type, p->in.right->tn.lval ) )
			return( STARREG );

	return( 0 );
	}










conput( p ) NODE *p; {
#ifndef FLINT
	switch( p->in.op ){

	case ICON:
		acon( p );
		return;

	case REG:
		prntf( "%s", rnames[p->tn.rval] );
		USEDREG(p->tn.rval);
		return;

	default:
		cerror( "illegal conput" );
		}
#endif /* FLINT */
	}







upput( p, offset ) register NODE *p; int offset;
	{

#ifndef FLINT
	/* output the address of the second word in the
	   pair pointed to by p (for LONGs)*/
	CONSZ save;

# ifndef FORT
	if( p->in.op == FLD ){
		p = p->in.left;
		}
# endif

	save = p->tn.lval;
	switch( p->in.op ){

	case NAME:
		p->tn.lval += SZINT/SZCHAR*offset;
		acon( p );
		break;

	case ICON:
		/* addressable value of the constant */
		p->tn.lval &= BITMASK(SZINT);
		prntf( "&" );
		acon( p );
		break;

	case REG:
		prntf( "%s", rnames[p->tn.rval+1] );
		USEDREG(p->tn.rval + 1);
		break;

	case OREG:
		p->tn.lval += SZINT/SZCHAR*offset;

# ifndef FORT
		/* Name in the argument region? */
		if( (p->tn.rval == A6) && p->in.name )
			werror( "bad arg temp" );
# endif	/* FORT */
		if (p->tn.rval < 0)
			{
			/* It's an indexed-mode oreg (packed) */
			print_indexed(p);
			}
		else
			{
			if (picoption && ISPIC_OREG(p))
			  {		/* transformed PIC NAME/ICON ref */
			  pic_used = YES;
			  if ( picoption > 1 )
			    {	/* force long displ form of instr */
			    prntf( "(" );
			    acon( p );
			    prntf( ",%s,%%za0.l)", rnames[p->tn.rval] );
			    USEDREG(p->tn.rval);
			    break;
			    }
			  }
			if( p->tn.lval != 0 || p->in.name != NULL ) acon( p );
			prntf( "(%s)", rnames[p->tn.rval] );
			USEDREG(p->tn.rval);
			}
		break;

	default:
		cerror( "illegal upper address" );
		break;

		}
	p->tn.lval = save;
#endif /* FLINT */
	}







adrput( p ) register NODE *p; {
#ifndef FLINT
	/* output an address, with offsets, from p */

# ifndef FORT
	if( p->in.op == FLD ){
		p = p->in.left;
		}
# endif
	switch( p->in.op ){

	case NAME:
		acon( p );
		return;

	case ICON:
		/* addressable value of the constant */
		if( p->in.type == DOUBLE ) {
			/* print the high order value */
			CONSZ save;
			save = p->tn.lval;
			p->tn.lval = ( p->tn.lval >> SZINT ) & BITMASK(SZINT);
			prntf( "&" );
			acon( p );
			p->tn.lval = save;
			return;
			}
		prntf( "&" );
		acon( p );
		return;

	case REG:
		prntf( "%s", rnames[p->tn.rval] );
		USEDREG(p->tn.rval);
		return;

	case OREG:
		if (p->tn.rval < 0)
			{
			/* It's a (packed) indexed oreg. */
			print_indexed(p);
			return;
			}
		if( p->tn.rval == A6 ){  /* in the argument region */
#ifndef FORT
			if( p->in.name != NULL ) werror( "bad arg temp" );
#endif
			prntf( CONFMT, p->tn.lval );
			prntf( "(%%a6)" );
			return;
			}
		if ( picoption && ISPIC_OREG( p ) )
		  {		/* Transformed PIC NAME/ICON ref */
		  pic_used = YES;
		  if ( picoption > 1 )
		    {		/* force long displ instr format */
		    prntf( "(" );
		    acon( p );
		    prntf( ",%s,%%za0.l)", rnames[p->tn.rval] );
		    USEDREG(p->tn.rval);
		    break;
		    }
		  }
		if( p->tn.lval != 0 || p->in.name != NULL )
		  { acon( p ); }
		prntf( "(%s)", rnames[p->tn.rval] );
		USEDREG(p->tn.rval);
		break;

	case UNARY MUL:
		/* STARNM or STARREG found */
#ifdef FASTMATCH
		if (tshape(p) & STARNM)
#else
		if( tshape(p, STARNM) )
#endif /* FASTMATCH */
			{
			adrput( p->in.left);
			}
		else {	/* STARREG - really auto inc or dec */
			/* turn into OREG so replacement node will
			   reflect the value of the expression */
			register NODE *q, *l;

			l = p->in.left;
			q = l->in.left;
			p->in.op   = OREG;
			p->in.rall = q->in.rall;
			p->tn.lval = q->tn.lval;
			p->tn.rval = q->tn.rval;
			p->in.name = q->in.name;	/* just copy the ptr */
			if( l->in.op == INCR ) {
				adrput( p );
				prntf( "+" );
				p->tn.lval -= l->in.right->tn.lval;
				}
			else {	/* l->in.op == ASG MINUS */
				prntf( "-" );
				adrput( p );
				}
			tfree( l );
		}
		return;

	case PLUS:
	case MINUS:
		/* SADRREG, OREG without the U* on top found */
		if (adrreg(p))
			print_indexed(&adrregnode);
		else
			cerror( "illegal address" );
		return;

	default:
		cerror( "illegal address" );
		return;

		}
#endif /* FLINT */
	}


/***************************************************************
*
*	upput_dbl() - put out the "upper half" for STARREG dbls
*/

upput_dbl( p )
	NODE *p;
{

# ifndef FORT
  if ( p->in.op == FLD )
    p = p->in.left;
# endif
  if ( p->in.op == UNARY MUL )
    {
#ifdef FASTMATCH
    if (tshape(p) & STARREG)
#else
    if ( tshape( p, STARREG ) )
#endif /* FASTMATCH */
      {
      NODE a,b,c,d,*t;
      t = p->in.left;
      a = *p;			/* make copy of STARREG opd tree */
      b = *t;
      c = *(t->in.left);
      d = *(t->in.right);
      a.in.left = &b;
      b.in.left = &c;
      b.in.right = &d;
      t = &a;
      adrput( t );
      }
    else
      cerror("Illegal address in upput_dbl()");
    }
  else
    upput( p, 1 );
} /* upput_dbl() */



/* print_indexed() prints out an indexed-mode address */
LOCAL print_indexed(p)	register NODE *p;
{
#ifndef FLINT
	    /* NOTE: Not completed for MEMIND_POST/PRE. Current code assumes
	       that od is 0 (not always the case)!
	    */
	    union indexpacker x;
	    int bd;

	    x.rval = p->tn.rval;
	    bd = p->tn.lval;
	    prntf(x.i.mode == MEMIND_PRE? "([" :  "(" );
	    if (p->tn.name)
		    {
		    prntf("%s", p->tn.name);
		    if (bd) prntf("+%d,", bd);
		    else prntf(",");
		    }
	    else if (bd) prntf("%d,", bd);
	    if (x.i.addressreg)
		    {
		    prntf("%s,", rnames[x.i.addressreg]);
		    USEDREG(x.i.addressreg);
		    }
	    else
		    prntf("%%za0,");
	    prntf(x.i.mode==MEMIND_PRE? "%s.l*%d],0)" : "%s.l*%d)",
			rnames[x.i.xreg], 1<<(x.i.scale) );
	    USEDREG(x.i.xreg);
	    return;
#endif /* FLINT */
}





LOCAL rext(p, cp, tw, m)	NODE *p; char	*cp;	TWORD tw, m;
/* rext puts one or more extensions on the cp register so that types are matched.
*	returns 1 if any code is emitted, otherwise returns 0
*/
{
#ifndef FLINT
	if (tw != m)
		{
	   switch(tw)
		{
		case UCHAR:
			prntf( (m==SHORT || m==USHORT)? "\tand.w\t&0xff," :
								"\tand.l\t&0xff,");
			break;
		case CHAR:
			prntf( (m==SHORT || m==USHORT)? "\text.w\t" : "\textb.l\t");
			break;
		case USHORT:
			prntf("\tand.l\t&0xffff,");
			break;
		case SHORT:
			prntf("\text.l\t");
			break;
		default:
			return ( 0 );
		}
	  expand(p, FOREFF, cp);
	  return ( 1 );
	  }
	else
	  return ( 0 );
#endif /* FLINT */
}






LOCAL acon( p ) register NODE *p; { /* print out a constant */
#ifndef FLINT
	if( p->in.name == NULL )		/* constant only */
		{
		if (p->in.op == ICON) switch (p->tn.type)
			/* don't mask for OREG lvals */
			{
			/* for signed operands, explicitly extend */
			case CHAR:
				if (p->tn.lval & 0x00000080)
					p->tn.lval |= 0xffffff00;
					break;
			case SHORT:
				if (p->tn.lval & 0x00008000)
					p->tn.lval |= 0xffff0000;
					break;
			}
		prntf( CONFMT, p->tn.lval);
		}
	else if( p->tn.lval == 0 ) 	/* name only */
		prntf( "%s", p->in.name );
	else 				/* name + offset */
		{
		prntf( "%s+", p->in.name );
		prntf( CONFMT2, p->tn.lval );
		}
#endif /* FLINT */
	}



gencall( p ) register NODE *p; {
	/* generate the call given by p */
	int temp, m;
#ifdef FORT
#ifdef DBL8
	int pad;
#endif /* DBL8 */
#endif /* FORT */
	register NODE *pchild = p->in.right;
#ifdef FORTY
	flag fp040_emulation = (((!picoption) && (p->in.left->in.op == ICON)) || (picoption && ISPIC_OREG(p->in.left))) ? (strncmp(p->in.left->in.name,"__fp040_",8) == 0) : 0;
	long calllabel,endlabel;
	int name_offset;
#endif /* FORTY */

	calling_others = YES;	/* current routine does call other routines */

	/* first work on the right side (the arguments) */
#ifdef FORTY
	if (fp040_emulation)
		{
		if (pchild->in.fpaside)
			{
			cerror("Invalid FPA 68040 emulation call");
			}
		order(pchild, INFREG|INTFREG);
		if (ISFNODE(pchild))
			{
			if (pchild->tn.rval != F0)
				{
				expand(pchild,FOREFF,"\tfmov.x\tAL,%fp0");
				}
			rfree(pchild->tn.rval,p->in.type);
			pchild->in.op = FREE;
			if (picoption) {
				prntf("\n\tmov.l\tflag_68040(%s),%%a0",rnames[picreg]);
				prntf("\n\ttst.w\t(%%a0)\n");
				usedregs |= PICREGBIT( picreg );
				pic_used = YES;
			}
			else {
				prntf("\n\ttst.w\tflag_68040\n");
			}
			prntf("\tbne.l\tL%d\n",calllabel = GETLAB());
			name_offset = (strncmp(p->in.left->in.name,"__fp040_d",9) == 0) ? 9 : 8;
			if (strcmp(p->in.left->in.name+name_offset,"exp") == 0)
				prntf("\tfetox.x\t%%fp0\n");
			else if (strcmp(p->in.left->in.name,"__fp040_dlogn") == 0) {
				prntf("\tfmov.d\t%%fp0,-(%%sp)\n");
				prntf("\tbsr.l\t__log\n");
				prntf("\tmov.l\t%%d1,4(%%sp)\n");
				prntf("\tmov.l\t%%d0,(%%sp)\n");
				prntf("\tfmov.d\t(%%sp)+,%%fp0\n");
				}
			else
				prntf("\tf%s.x\t%%fp0\n",p->in.left->in.name+name_offset);
			if (p->in.rall & (MUSTDO | D0)) {
				expand(pchild,FOREFF,"\tfmov.d\tAL,-(%sp)\n");
				expand(pchild,FOREFF,"\tmov.l\t(%sp)+,%d0\n");
				expand(pchild,FOREFF,"\tmov.l\t(%sp)+,%d1\n");
				}
			prntf("\tbra.l\tL%d\n",endlabel = GETLAB());
			prntf("L%d:",calllabel);
			}
		else
			{
			cerror("Invalid 68040 emulation call");
			}
		temp = 0;
		}
	else if (pchild)
#else /* FORTY */
	if( pchild )
#endif /* FORTY */
		{
		temp = argsize( pchild );
#ifdef FORT
#ifdef DBL8
		if ((pad = (STACK_BOUNDRY - ((temp + stack_offset) % STACK_BOUNDRY)) % STACK_BOUNDRY) != 0)
			{
			prntf("\tadd.l\t&-%d,%%sp\n",pad);
			temp += pad;
			stack_offset += pad;
			}
#endif /* DBL8 */
#endif /* FORT */
		genargs( pchild );	/* generate arguments */
		}
	else
		temp = 0;

	/* now handle the left subtree. Possibly a ptr to a function. */
	pchild = p->in.left;

/*
	-- The use of shltype was removed since it allowed post-increment
	-- and a post increment is not allowed on a jsr instruction

	if( shltype( pchild->in.op, pchild ) == 0 ||
		(pchild->in.op==UNARY MUL && pchild->in.left->in.op==OREG) ) 
*/

	if ((!daleafop(pchild->in.op)) &&
	    ((pchild->in.op!=UNARY MUL) || (!INDEXREG(pchild->in.left))))
		{
		order( pchild, INBREG|SOREG );
		}

	if (p->in.op != UNARY STCALL) p->in.op = UNARY CALL;

#ifdef FORTY
	if (fp040_emulation)
		{
		prntf("\tftest.x\t%%fp0\n");
		}
#endif /* FORTY */
	m = match( p, INTAREG|INTBREG );
	popargs( temp );
#ifdef FORTY
	if (fp040_emulation)
	{
		if (p->in.rall & (MUSTDO | D0)) {
			expand(p,FOREFF,"\tfmov.d\t%fp0,-(%sp)\n");
			expand(p,FOREFF,"\tmov.l\t(%sp)+,%d0\n");
			expand(p,FOREFF,"\tmov.l\t(%sp)+,%d1\n");
		}
		else {
			rfree(p->tn.rval,p->in.type);
			p->tn.rval = F0;
			rbusy(F0,p->in.type);
		}
		prntf("L%d:\n",endlabel);
	}
#endif /* FORTY */
	return(m != MDONE);
	}



char *inliners[ MAXINLINERS ] = { "__BUILTIN_strcpy" };


geninlinecall( n, p, cookie ) 
	int n;			/* index into "inliners" array */
	register NODE *p;	/* call tree */
	int cookie;		/* order cookie */
{
	register NODE *a;	/* argument tree */
  a = p->in.right;
  switch ( n )
    {
    case 0:
	{				/* strcpy(s1,s2) */
	  NODE *s1, *s2, *t;
	  char *r1, *r2, *r3;
	  long lab;
	  long size;

	if ( a->in.op == CM &&
		a->in.left->in.op == CM &&
		(t = a->in.left->in.left)->in.op == ICON &&
		t->in.name == NULL )
		/* there are two user args, and the last CM'ed
		   icon tells whether a literal string was
		   passed as the source arg (w/size incl. null)
	        */
	  {
	  s1 = a->in.left->in.right;
	  s2 = a->in.right;
	  size = t->tn.lval;
	  if ( size > 0 )	/* Literal source arg, known length */
	    {
	    if ( (size == 1) || (size == 2) || (size == 4) )
	      {		/* short (single move instr.) copy */
	      char szch;
	      char *r1;
		/* get destination arg into scratch address reg */
	      if ( ! ( ISTNODE( s1 ) && ISBREG( s1->tn.rval ) ) )
		order( s1, INTBREG );
	      r1 = rnames[ s1->tn.rval ];
	      if ( !(cookie & FOREFF) )
	        {				 /* reserve %d0 for result */
	        rbusy( D0, INT );
	        prntf("	mov.l	%s,%%d0\n", r1);	/* return dest addr */
	        }
	      szch = (size == 1) ? 'b'
				 : (size == 2) ? 'w'
					       : 'l';
	      prntf("	mov.%c	", szch);
	      if ( size == 1 )
		prntf("&0,(%s)\n", r1);
	      else
		{
	        acon( s2 );
	        prntf(",(%s)\n", r1);
		}
	      USEDREG( s1->tn.rval );
	      rfree( s1->tn.rval, PTR+CHAR );
	      }
	    else
	      {
	      cpyblk( s1, s2, size, -1, cookie );
	      rfree( s1->tn.rval, PTR+CHAR );
	      rfree( s2->tn.rval, PTR+CHAR );
	      }
	    }
	  else			/* Don't know size of source arg */
	    {	/* general purpose byte loop move */
		/* load source and destination args into scratch address reg */

	    if (!haseffects(s2)) {
		order(s1,INTBREG);
		order(s2,INTBREG);
	    }
	    else {
		if (haseffects(s1)) order(s1,INTEMP);
		order(s2,INTBREG);
		order(s1,INTBREG);
	    }

	    r1 = rnames[ s1->tn.rval ];
	    r2 = rnames[ s2->tn.rval ];
	    if ( !(cookie & FOREFF) )
	      { /* reserve %d0 for result */
	      rbusy( D0, INT );
	      prntf("	mov.l	%s,%%d0\n", r1); /* return dest addr */
	      }
	    prntf("L%d:\n", lab = GETLAB());
	    prntf("	mov.b	(%s)+,(%s)+\n", r2, r1);
	    prntf("	bne.b	L%d\n", lab);
	    USEDREG( s1->tn.rval );
	    rfree( s1->tn.rval, PTR+CHAR );
	    USEDREG( s2->tn.rval );
	    rfree( s2->tn.rval, PTR+CHAR );
	    }
	  s1->in.op = FREE;
	  s2->in.op = FREE;
	  t->in.op = FREE;
	  a->in.left->in.op = FREE;
	  a->in.op = FREE;
	  p->in.left->in.op = FREE;
	  if ( cookie & FOREFF )
	    {
	    p->in.op = ICON;
	    p->in.name = NULL;
	    p->tn.lval = 0;
	    }
	  else
	    {
	    USEDREG( D0 );
	    p->in.op = REG;
	    p->tn.rval = D0;
	    }
	  return( 0 );
	  }
	else
	  cerror("Invalid argument list for inlined call to \"strcpy\"");
	}
	break;
# if 0
    case 1:
	{				/* strcmp(s1,s2) */
# define MINLONGCMPCHARS 13

	  NODE *s1, *s2, *t;
	  char *r1, *r2, *r3;
	  long lnglab, bytlab, resetlab, endlab, altendlab;
	  long size;

	if ( a->in.op == CM &&
		a->in.left->in.op == CM &&
		(t = a->in.left->in.left)->in.op == ICON &&
		t->in.name == NULL )
		/* there are two user args, and the last CM'ed
		   icon tells the length of the shortest arg,
		   including the null byte, if either arg's length 
		   is known (i.e. literal string arg or args).
		*/
	  {
	  s1 = a->in.left->in.right;
	  s2 = a->in.right;
		/* load arguments into scratch address regs */
	  if ( ! ( ISTNODE( s2 ) && ISBREG( s2->tn.rval ) ) )
	    order( s2, INTBREG );
	  if ( ! ( ISTNODE( s1 ) && ISBREG( s1->tn.rval ) ) )
	    order( s1, INTBREG );
	  if ( !( ISTNODE( s1 ) && ISBREG( s1->tn.rval )
		  && ISTNODE( s2 ) && ISBREG( s2->tn.rval ) ) )
	    cerror("Unable to load inlined \"strcmp\" arguments");
	  r1 = rnames[ s1->tn.rval ];
	  r2 = rnames[ s2->tn.rval ];
		/* reserve %d0 for result */
	  rbusy( D0, INT );
					/* identical strings check */
	  prntf("	cmp.l	%s,%s\n", r1, r2);
	  prntf("	bne.b	L%d\n", lnglab = GETLAB());
	  prntf("	mov.l	&0,%%d0\n");
	  prntf("	bra.b	L%d\n", endlab = GETLAB());
	  prntf("L%d:\n", altendlab = GETLAB());
					/* return last bytes compare
					   when null seen in 1st arg */
	  prntf("	sub.b	(%s),%%d0\n", r2); 
	  prntf("	bra.b	L%d\n", endlab);
	  prntf("L%d:\n", lnglab);
	  size = t->tn.lval;
	  if ( ! (size > 0 && size < MINLONGCMPCHARS) )
	    {
	    rbusy( D1, INT );	/* reserve %d1 for temp */
					/* do long word compares, while
					   no imbedded nulls, and equal */
	    prntf("	mov.l	(%s)+,%%d0\n", r1);
	    prntf("	mov.l	%%d0,%%d1\n");
	    prntf("	sub.l	&0x01010101,%%d1\n");
	    prntf("	or.l	%%d0,%%d1\n");
	    prntf("	eor.l	%%d0,%%d1\n");
	    prntf("	and.l	&0x80808080,%%d1\n");
	    prntf("	bne.b	L%d\n", resetlab = GETLAB());
	    prntf("	sub.l	(%s)+,%%d0\n", r2);
	    prntf("	beq.b	L%d\n", lnglab);
	    prntf("	subq.l	&4,%s\n", r2);
	    prntf("L%d:\n", resetlab);
	    prntf("	subq.l	&4,%s\n", r1);
	    USEDREG( D1 );
	    rfree( D1, INT );
	    }
	  prntf("L%d:\n", bytlab = GETLAB());
						/* do byte compares */
	  prntf("	mov.b	(%s)+,%%d0\n", r1);
	  prntf("	beq.b	L%d\n", altendlab);
	  prntf("	sub.b	(%s)+,%%d0\n", r2);
	  prntf("	beq.b	L%d\n", bytlab);
					/* return last bytes compare
					   when null seen in 2nd arg
					   or until unequal bytes */
	  prntf("L%d:\n", endlab);
	  prntf("	extb.l	%%d0\n"); /* return result as int */

	  USEDREG( s1->tn.rval );
	  USEDREG( s2->tn.rval );
	  USEDREG( D0 );
	  rfree( s1->tn.rval, PTR+CHAR );
	  rfree( s2->tn.rval, PTR+CHAR );
	  s1->in.op = FREE;
	  s2->in.op = FREE;
	  t->in.op = FREE;
	  a->in.left->in.op = FREE;
	  a->in.op = FREE;
	  p->in.left->in.op = FREE;
	  p->in.op = REG;
	  p->tn.rval = D0;
	  return( 0 );
	  }
	else
	  cerror("Invalid argument list for inlined call to \"strcmp\"");
	}
	break;
# endif /* 0 */
    default:
	cerror("Invalid index used for inlined call generation");
	break;
    }
  return( 1 );
} /* geninlinecall */




# define MAXCHARSWITHMOVES 64

cpyblk( s1, s2, size, dataregnum, cookie )
	NODE *s1, *s2;
	long size;
	int dataregnum;
	int cookie;
{
	char *r1, *r2;
	long lab;

		/* load destination arg into scratch address reg */
	    if ( ! ( ISTNODE( s1 ) && ISBREG( s1->tn.rval ) ) )
	      order( s1, INTBREG );
		/* load source arg into scratch address reg */
	    if ( ! ( ISTNODE( s2 ) && ISBREG( s2->tn.rval ) ) )
	      order( s2, INTBREG );
	    USEDREG(  s1->tn.rval );
	    USEDREG(  s2->tn.rval );
	    r1 = rnames[ s1->tn.rval ];
	    r2 = rnames[ s2->tn.rval ];
	    if ( !(cookie & FOREFF) )
	      {				 /* reserve %d0 for result */
	      rbusy( D0, INT );
	      prntf("	mov.l	%s,%%d0\n", r1); /* return dest addr */
	      USEDREG( D0 );
	      }
	    if ( size > MAXCHARSWITHMOVES )
	      {		/* long word move loop */
	      char *r3;
	      register NODE *t;
	      t = talloc();
	      if ( dataregnum < 0 )	/* need to allocate date reg */
		{
	        t->tn.op = ICON;
	        t->tn.type = INT;
	        t->tn.rall = NOPREF;
	        t->tn.name = NULL;
	        t->tn.lval = ((size>>2) - 1);
	        t->tn.rval = 0;
	        order( t, INTAREG );
		}
	      else	/* data reg already allocated (e.g. via template) */
		{
		t->tn.op = REG;
		t->tn.type = INT;
	        t->tn.rall = NOPREF;
	        t->tn.name = NULL;
	        t->tn.rval = dataregnum;
	        t->tn.lval = 0;
		prntf("	mov.l	&%d,%s\n", (size>>2) - 1, rnames[dataregnum]);
		}
	      if ( ! (t->in.op == REG && ISAREG( t->tn.rval )
			&& ISTREG( t->tn.rval ) ) )
		cerror("Unable to load needed scratch data reg for block move");
	      USEDREG(  t->tn.rval );
	      r3 = rnames[ t->tn.rval ];
	      prntf("L%d:\n", lab = GETLAB());
	      prntf("	mov.l	(%s)+,(%s)+\n", r2, r1);
	      prntf("	dbf	%s,L%d\n", r3, lab);
	      if (size > (32768*4))
	        {
		prntf("	sub.l	&0x10000,%s\n", r3);
		prntf("	bge.b	L%d\n", lab);
		}
	      if ( size & 0x2 )
	        prntf("	mov.w	(%s)+,(%s)+\n", r2, r1);
	      if ( size & 0x1 )
	        prntf("	mov.b	(%s),(%s)\n", r2, r1);
	      if ( dataregnum < 0 )
		rfree( t->tn.rval, INT );
	      t->in.op = FREE;
	      }
	    else
	     if ( size > 0 )
	      {		/* sequence of moves, no looping */
	      char szch;
	      while (size != 1 && size != 2 && size != 4)
		  {
		  if ( size >= 4 )
		    { szch = 'l'; size -= 4; 
		    }
		  else if ( size >= 2 )
		    { szch = 'w'; size -= 2; 
		    }
		  else
		    { szch = 'b'; size -= 1; 
		    }
		  prntf("	mov.%c	(%s)+,(%s)+\n", szch, r2, r1);
		  }
	      szch = (size == 1) ? 'b'
				   : (size == 2) ? 'w'
						 : 'l';
	      prntf("	mov.%c	(%s),(%s)\n", szch, r2, r1);
	      }
	     else
	      cerror("Unexpected size for block copy generation");
} /* cpyblk */




LOCAL popargs( size ) register size; {
#ifndef FLINT
	/* pop arguments from stack */

	if ( size ) 
		{
#ifdef FORT
#ifdef DBL8
		stack_offset -= size;
#endif /* DBL8 */
#endif /* FORT */
		if (size <= 8)
			prntf("\taddq\t&%d,%%sp\n", size);
#ifdef FORTY
		else if (fortyflag)
			prntf("\tadd\t&%d,%%sp\n", size);
#endif /* FORTY */
		else
			prntf("\tlea\t%d(%%sp),%%sp\n", size);
		}
#endif /* FLINT */
	}






char *
fccbranches[] = {
	"fbeq",
	"fbneq",
	"fbgt",
	"fbngt",
	"fbge",
	"fbnge",
	"fblt",
	"fbnlt",
	"fble",
	"fbnle",
	};
char *
fpccbranches[] = {
	"fpbeq",
	"fpbne",
	"fpbgt",
	"fpbngt",
	"fpbge",
	"fpbnge",
	"fpblt",
	"fpbnlt",
	"fpble",
	"fpbnle",
	};

char *
ccbranches[] = {
	"beq",
	"bne",
	"ble",
	"blt",
	"bge",
	"bgt",
	"bls",
	"bcs",		/* blo */
	"bcc",		/* bhis */
	"bhi",
	};


#ifdef ONEPASS
/* logical relations when compared in reverse order (cmp L,R) */
extern short revrel[] ;

#else							/* to support 2 pass */
/* mapping relationals when the sides are reversed */
short revrel[] ={ EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };
#endif



/*   printf conditional and unconditional branches */
cbgen( o, lab, mode ) register short o; unsigned lab;
{ 
#ifndef FLINT
	if( o == 0 ) prntf( "	bra.l\tL%d\n", lab );
	else if (o >= FEQ && o <= FNLE)
		{
	    	if( mode=='F' ) o = frevrel[ o-FEQ ];
	    	prntf( "\t%s.l\tL%d\n", 
			(fpaside ? fpccbranches[o-FEQ] : fccbranches[o-FEQ]),
			lab );
		}
	else	if( o > UGT ) cerror( "bad conditional branch: %s", opst[o] );
	else
		{
	    	if( mode=='F' ) o = revrel[ o-EQ ];
	    	prntf( "\t%s.l\tL%d\n", ccbranches[o-EQ], lab );
		}
#endif /* FLINT */
}







nextcook( p, cookie ) NODE *p; {
	/* we have failed to match p with cookie; try another */
	if( cookie == FORREW ) return( 0 );  /* hopeless! */
	if (flibflag)
		{
		if ( ! (cookie & (INTAREG|INTBREG)) )
			return( INTAREG|INTBREG );
		if ( ! (cookie & (INTEMP)) )
			return( INTEMP|INAREG|INTAREG|INBREG|INTBREG );
		}
	else
	if(p->in.fpaside)
		{
		if( !(cookie&(INTAREG|INTBREG|INTDREG)) )
			return( INTAREG|INTBREG|INTDREG );
		if( !(cookie&INTEMP) && asgop(p->in.op) )

			return( INTEMP|INAREG|INTAREG|INTBREG|INBREG
				|INTDREG|INDREG );
		}
	else
		{
		if( !(cookie&(INTAREG|INTBREG|INTFREG)) )
			return( INTAREG|INTBREG|INTFREG );
		if( !(cookie&INTEMP) && asgop(p->in.op) )
			return( INTEMP|INAREG|INTAREG|INTBREG|INBREG
				|INTFREG|INFREG );
		}
	return( FORREW );
	}

struct functbl {
	short fop;
	TWORD ftype;
	char *func;
	flag swonly;	/* true iff hardware flt pt can do this op. */
	} opfunc[] = {
	BICCODES,	FLOAT,		"___fcmpf",	0,
	BICCODES,	DOUBLE,		"___fcmp",	0,
	EQ,		LONGDOUBLE,	"__U_Qfcmp",	0,
	NE,		LONGDOUBLE,	"__U_Qfcmp",	0,
	GT,		LONGDOUBLE,	"__U_Qfcmp",	0,
	GE,		LONGDOUBLE,	"__U_Qfcmp",	0,
	LT,		LONGDOUBLE,	"__U_Qfcmp",	0,
	LE,		LONGDOUBLE,	"__U_Qfcmp",	0,
	NOT,		LONGDOUBLE,	"__U_Qflogneg",	0,
#if 0
	DIV,		INT,	"___ldiv",		0,
	MOD,		INT,	"___lrem",		0,
	ASG DIV,	INT,	"___aldiv",		0,
	ASG MOD,	INT,	"___alrem",		0,
	DIV,		UNSIGNED,	"___uldiv",	0,
	MOD,		UNSIGNED,	"___ulrem",	0,
	ASG DIV,	UNSIGNED,	"___auldiv",	0,
	ASG MOD,	UNSIGNED,	"___aulrem",	0,
#endif /* 0 */
#ifdef FORT
	PLUS,		FLOAT,		"_faddf",	1,
	MINUS,		FLOAT,		"_fsubf",	1,
	MUL,		FLOAT,		"_fmulf",	1,
	DIV,		FLOAT,		"_fdivf",	1,
#else	/* C requires that these ops be done in 64 bit precision. */
	PLUS,		FLOAT,		"___ffadd",	1,
	MINUS,		FLOAT,		"___ffsub",	1,
	MUL,		FLOAT,		"___ffmul",	1,
	DIV,		FLOAT,		"___ffdiv",	1,
#endif
	PLUS,		DOUBLE,		"___fadd",	1,
	MINUS,		DOUBLE, 	"___fsub",	1,
	MUL,		DOUBLE, 	"___fmul",	1,
	DIV,		DOUBLE, 	"___fdiv",	1,
	UNARY MINUS,	FLOAT,		"___fneg",	1,
	UNARY MINUS,	DOUBLE, 	"___fneg",	1,
	UNARY MINUS,	LONGDOUBLE,     "__U_Qfneg",	0,
	PLUS,           LONGDOUBLE,     "__U_Qfadd",    0,
	MINUS,          LONGDOUBLE,     "__U_Qfsub",    0,
	MUL,            LONGDOUBLE,     "__U_Qfmpy",    0,
	DIV,            LONGDOUBLE,     "__U_Qfdiv",    0,
	/* fneg for float is same as for doubles since it only
	   twiddles the sign bit in both cases.
	*/
	ASG PLUS,	DOUBLE,		"___afadd",	1,
	ASG PLUS,	FLOAT,		"___afaddf",	1,
	ASG MINUS,	DOUBLE, 	"___afsub",	1,
	ASG MINUS,	FLOAT,		"___afsubf",	1,
	ASG MUL,	DOUBLE, 	"___afmul",	1,
	ASG MUL,	FLOAT,		"___afmulf",	1,
	ASG DIV,	DOUBLE, 	"___afdiv",	1,
	ASG DIV,	FLOAT,		"___afdivf",	1,
	0,	0,	0 };

#ifdef FORT
struct fmon {
	char *op881;
	char *opFPA;
	} fmontbl[] = {
	"fabs",		"fpabs",
	"fsqrt",	0,
	"facos",	0,
	"fasin",	0,
	"fatan",	0,
	"fcos",		0,
	"fetox",	0,
	"flog10",	0,
	"flog2",	0,
	"flogn",	0,
	"flognp1",	0,
	"fsin",		0,
	"ftan",		0,
	"fcosh",	0,
	"fsinh",	0,
	"ftanh",	0,
	0,		0 };

#endif /* FORT */





hardops(p)  register NODE *p; {
	/* change hard to do operators into function calls. */

	/* NOTE, be sure any "return"s from this function are actually
	   effected by using "goto retlab;" .vs. "return;" statements */

	register NODE *q;
	register NODE *r;
	register struct functbl *f;
	register short o = p->in.op;
	register TWORD t;
	register NODE *pleft;
	register NODE *qright;
	extern int pictrans();

	p->in.fpaside = fpaside;
	reduce_sconvs(p);
	o = p->in.op;  /* reduce_sconvs might have changed *p */
#ifdef FORT
	if (!fpaside && !flibflag) {
		if (asgop(o)) {
			if ((p->in.left->in.type != LONGDOUBLE) &&
			    (p->in.right->in.type != LONGDOUBLE)) goto retlab;
			}
		else if (o == SCONV) {
			if ((p->in.type != LONGDOUBLE) &&
			    (p->in.left->in.type != LONGDOUBLE)) {
#ifdef FORTYX
				if (fortyflag) hardconv(p);
#endif /* FORTY */
				goto retlab;
				}
			}
		else {
			if (p->in.type != LONGDOUBLE) goto retlab;
			}
		}
#endif /* FORT */
	if (fpaside) {
#ifdef FORT
		if ( fpaflag && (p->in.op == FMONADIC)) {
			/* convert 881 op name to FPA name */
			register struct fmon *i;
			for (i=fmontbl; i->op881; i++) {
				if (!strcmp(i->op881,p->in.left->tn.name)) {
					if (i->opFPA)
						p->in.left->tn.name =
							addtreeasciz(i->opFPA);
					break;
					}
				}
				
			}
#endif /* FORT */
		/* check for converts  BYTE/WORD to/from FLOAT/DOUBLE */
		/* insert intermediate converts to INT as necessary */
		if (p->in.op == SCONV)
			{
			register TWORD tl;
			register flag m, ml;
			t = p->in.type;
			tl = p->in.left->in.type;
			m = ISHWFTP(t);
			ml = ISHWFTP(tl);
			if (m && !ml && (tl != INT) && (tl != UNSIGNED) && (tl != LONGDOUBLE) && (tl != VOID))
				harditofl(p->in.left);
			else if (!m && ml && (t != INT) && (t != UNSIGNED) && (t != LONGDOUBLE) && (t != VOID))
				harditofl(p->in.left);
			else hardconv(p);
			}
		if (asgop(o)) {
			if ((p->in.left->in.type != LONGDOUBLE) &&
			    (p->in.right->in.type != LONGDOUBLE)) goto retlab;
			}
		else if (o == SCONV) {
#ifdef FORTYX
			if (fortyflag) {
				hardconv(p);
				goto retlab;
			}
#endif /* FORTY */
			if ((p->in.type != LONGDOUBLE) &&
			    (p->in.left->in.type != LONGDOUBLE)) goto retlab;
			}
		else if (logop(o)) {
			if (p->in.left->in.type != LONGDOUBLE) goto retlab;
			}
		else {
			if (p->in.type != LONGDOUBLE) goto retlab;
			}
		}

	if (p->in.op==SCONV) 
		{ 
		hardconv(p); 
		goto retlab; 
		}

	if (!fpaside && !flibflag) {
		if (logop(o)) {
			if ((p->in.left->in.type != LONGDOUBLE)) goto retlab;
			}
		else {
			if (p->in.type != LONGDOUBLE) goto retlab;
			}
		}

	t = p->in.type;

	if ( ISPTR(t) ) t = INT;	/* to allow pointer arithmetic */

	if (logop(o) && (p->in.left->in.type == LONGDOUBLE)) {
		t = LONGDOUBLE;
		}


	if ((p->in.type == LONGDOUBLE) && ((o == INCR) || (o == DECR))) {

		p->in.op = (o == INCR) ? MINUS : PLUS;
		pleft = p->in.left;
		p->in.left = q = talloc();
		*q = *p;
		q->in.op = (o == INCR) ? ASG PLUS : ASG MINUS;
		q->in.left = pleft;
		q->in.right = tcopy(p->in.right);
		
		hardops(p->in.left);

		o = p->in.op;
		}

	/* Convert long double assign ops to regular operators */
	if ((p->in.type == LONGDOUBLE) &&
			((o == ASG MUL)  || (o == ASG DIV) ||
			 (o == ASG PLUS) || (o == ASG MINUS))) {

		/* Change p to assignement whose right child is regular */
		/* operator orginal left and right are now p->in.left and */
		/* q->in.right */
		q = talloc();
		*q = *p;
		q->in.op = p->in.op - 1;
		q->in.right = p->in.right;
		p->in.op = ASSIGN;
		p->in.right = q;

		/* If no sideaffects copy p->in.left to q->in.right */
		/* Otherwise, ....                                  */
		if (!haseffects(p->in.left)) {
			q->in.left = tcopy(p->in.left);
			hardops(p->in.right);
			goto retlab;
			}
		else {
			q = talloc();
			*q = *p;
			p->in.op = COMOP;
			p->in.right = q;
			p->in.left = talloc();
			*(p->in.left) = *(p->in.right);
			p->in.left->in.type = INCREF(p->in.right->in.type);
			p->in.left->in.left = q = talloc();
			q->in.op = OREG;
			q->in.type = p->in.left->in.type;  /* PTR LONGDOUBLE */
			q->tn.rval = 14;                   /* Off of A6 */
			q->tn.lval = BITOOR(freetemp(SZPOINT/SZINT));
			q->in.name = 0;

			if (p->in.right->in.left->in.op != UNARY MUL) {
				p->in.left->in.right = q = talloc();
				q->in.op = UNARY AND;
				q->in.type = INCREF(p->in.right->in.left->in.type);
				q->in.left = p->in.right->in.left;
				}
			else {
				p->in.left->in.right = p->in.right->in.left->in.left;
				p->in.right->in.left->in.op = FREE;
				}

			p->in.right->in.left = q = talloc();
			q->in.op = UNARY MUL;
			q->in.type = p->in.right->in.type;
			q->in.left = tcopy(p->in.left->in.left);

			p->in.right->in.right->in.left = tcopy(p->in.right->in.left);
			hardops(p->in.right->in.right);
			goto retlab;
			}
		}


	for( f=opfunc; f->fop; f++ ) {
		if( o==f->fop && t==f->ftype ) goto convert;
		}
	goto retlab;

	/* need address of left node for ASG OP */
	/* WARNING - this won't work for long in a REG */
convert:
	if( asgop( o ) ) {
		pleft = p->in.left;
		switch( pleft->in.op ) {

		case UNARY MUL:	/* convert to address */
			pleft->in.op = FREE;
			p->in.left = pleft->in.left;
			break;

		case NAME:	/* convert to ICON pointer */
			pleft->in.op = ICON;
			pleft->in.type = INCREF( pleft->in.type );
			break;

		case OREG:	/* convert OREG to address */
			{
			union indexpacker x;

			if ( ISPIC_OREG( pleft ) )
			  order( pleft, INTEMP );
			x.rval = pleft->tn.rval;
			if (x.i.indexed)
				pleft->tn.rval = x.i.addressreg;
			pleft->in.op = REG;
			pleft->in.type = INCREF( pleft->in.type );
			if( pleft->tn.lval != 0 ) {
				q = talloc();
				q->in.op = PLUS;
				q->in.rall = NOPREF;
				q->in.type = pleft->in.type;
				q->in.left = pleft;
				q->in.right = qright = talloc();

				qright->in.op = ICON;
				qright->in.rall = NOPREF;
				qright->in.type = INT;
				qright->in.name = NULL;
				qright->tn.lval = pleft->tn.lval;
				qright->tn.rval = 0;

				pleft->tn.lval = 0;
				p->in.left = q;
				}
			if (x.i.indexed)
			    {
			    register NODE *adrside, *idxreg, *idxside;

			    adrside = p->in.left;
			    p->in.left = talloc();
			    p->in.left->in.op = PLUS;
			    p->in.left->in.type = adrside->in.type;
			    p->in.left->in.rall = NOPREF;
			    p->in.left->in.left = adrside;
			    idxreg = talloc();
			    idxreg->in.op = REG;
			    idxreg->in.type = adrside->in.type;
			    idxreg->in.rall = NOPREF;
			    idxreg->tn.rval = x.i.xreg;
			    idxreg->tn.lval = 0;
			    if (x.i.scale > 1)
				{
				p->in.left->in.right = idxside = talloc();
				idxside->in.op = LS;
				idxside->in.type = idxreg->in.type;
				idxside->in.rall = NOPREF;
				idxside->in.left = idxreg;
				idxside->in.right = talloc();
				idxside->in.right->in.op = ICON;
				idxside->in.right->in.type = INT;
				idxside->in.right->tn.lval = x.i.scale;
				idxside->in.right->tn.rval = 0;
				idxside->in.right->in.name = NULL;
				}
			    else
				p->in.left->in.right = idxreg;
			    }
			}
			break;

		/* rewrite "foo <op>= bar" as "foo = foo <op> bar" for foo in a reg */
		case REG:
			if ( !flibflag )
				break;
			/* else fall thru */
		case FLD:
			q = talloc();
			q->in.op = p->in.op - 1;	/* change <op>= to <op> */
			q->in.rall = p->in.rall;
			q->in.type = p->in.type;
			q->in.left = talloc();
			q->in.right = p->in.right;
			p->in.op = ASSIGN;
			p->in.right = q;
			q = q->in.left;			/* make a copy of "foo" */
			q->in.op = pleft->in.op;
			q->in.rall = pleft->in.rall;
			q->in.type = pleft->in.type;
			q->tn.rval = pleft->tn.rval;
			if (pleft->in.op == FLD) 
				q->in.left = tcopy(pleft->in.left);
			else q->tn.lval = pleft->tn.lval;
			hardops(p->in.right);
			goto retlab;

		default:
			cerror( "Bad address for hard ops" );
			/* NO RETURN */

			}
		}

	/* build comma op for args to function */
	if ( optype(p->in.op) == BITYPE ) {
	  q = talloc();
	  q->in.op = CM;
	  q->in.rall = NOPREF;
	  q->in.type = INT;
	  q->in.left = p->in.left;
	  q->in.right = p->in.right;
	} else q = p->in.left;

	if (t == LONGDOUBLE) {
                if (logop(o)) {
			p->in.op = CALL;
			p->in.type = INT;

			p->in.right = r = talloc();
			r->in.op = CM;
          		r->in.rall = NOPREF;
          		r->in.type = INT;
			r->in.left = q;
			r->in.right = q = talloc();
			q->in.op = ICON;
			q->in.rall = NOPREF;
			q->in.type = INT;
			q->in.name = NULL;
			q->tn.rval = 0;
			q->tn.lval = 0;
			if (o==EQ || o==GE || o==LE) q->tn.lval += LDEQ;
			if (o==GT || o==GE || o==NE) q->tn.lval += LDGT;
			if (o==LT || o==LE || o==NE) q->tn.lval += LDLT;

			/* put function name in left node of call */
			p->in.left = q = talloc();
			q->in.op   = ICON;
			q->in.rall = NOPREF;
			q->in.type = INCREF( (TWORD)(FTN + INT) );
			q->in.name = addtreeasciz(f->func);
			q->tn.lval = 0;
			q->tn.rval = 0;
			}
		else {
			r = talloc();
                	r->in.op = STCALL;
                	r->in.rall = NOPREF;
                	r->in.type = INCREF(t);
                	r->tn.lval = 0;
                	r->tn.rval = 0;
                	r->stn.stsize = SZLONGDOUBLE/SZCHAR;
                	r->in.right = q;
                	r->in.left = q = talloc();

			q->in.op = ICON;             /* create new Name node */
                	q->in.rall = NOPREF;
                	q->in.type = INCREF( (TWORD)(FTN + INCREF(t)));
                	q->in.name = addtreeasciz(f->func);
                	q->tn.lval = 0;
                	q->tn.rval = 0;

			p->in.op = UNARY MUL;        /* use ORIG OP as dereference */
                	p->in.left = r;
			}
		}
	else {
		p->in.op = CALL;
		p->in.right = q;
		if (f->fop == BICCODES) p->in.type = INT;

		/* put function name in left node of call */
		p->in.left = q = talloc();
		q->in.op   = ICON;
		q->in.rall = NOPREF;
		t = (f->fop == BICCODES)? INT : p->in.type;
		q->in.type = INCREF( (TWORD)(FTN + t) );
		q->in.name = addtreeasciz(f->func);
		q->tn.lval = 0;
		q->tn.rval = 0;
		}

retlab:		/* do any needed canonicalizations, etc. */
	if ( picoption )
	  walkf( p, pictrans );
	return;

	}






/* hardconv() does fix and float conversions.  */
hardconv(p)
  register NODE *p;
  {	register NODE *q;
	register NODE *r;
	register TWORD t,tl;
	register flag m;
	flag ml;
	flag fhw = !flibflag;

	t = p->in.type;
	tl = p->in.left->in.type;
	m = ISFTP(t);
	ml = ISFTP(tl);


#ifdef FORTYX
	if (m==ml)
		if ( (m==0) || (t == tl) ||
			((!fortyflag) && fhw && t != LONGDOUBLE && tl != LONGDOUBLE)) return;
#else
	if (m==ml)
		if ( (m==0) || (t == tl) ||
                     (fhw && t != LONGDOUBLE && tl != LONGDOUBLE)) return;
#endif /* FORTY */
	if ( fhw && m && !ml && ISUNSIGNED(tl) && tl != UNSIGNED &&
			t != LONGDOUBLE && tl != LONGDOUBLE)
		{
		/* scalar to floating point conversion of some type */
		/* floating point hardware can do it if an intermediate SCONV is
		   there.  Except for UNSIGNED types.
		*/
		harditofl(p->in.left);
		return;
		}

#ifdef FORTYX
	if (fhw && (t != LONGDOUBLE) && (tl != LONGDOUBLE))
		{
		if (m == ml) return;
		if (fortyflag) {
			if ((t != UNSIGNED) && (tl != UNSIGNED) && (!ml)) return;
			}
		else {
			if ((t != UNSIGNED) && (tl != UNSIGNED)) return;
			}
		}
#else
	if ((fhw && t != LONGDOUBLE && tl != LONGDOUBLE)
	    && ((m == ml)		/* REAL->REAL  ||  SCALAR->SCALAR */
	     || (!(ml && (t == UNSIGNED))     /* ! REAL -> UNSIGNED */
	      && !(m && (tl == UNSIGNED)))))  /* ! UNSIGNED -> REAL */
		return;
#endif /* FORTY */

	if (t == VOID) return; /* no conv routine available for void */


	if ( ml && logop( p->in.left->in.op ) /* LOGICALOP(REAL)->SCALAR */
	    && !m )
	  return;	/* logical ops return 0 or 1 scalar anyway */

	if (t == LONGDOUBLE) {
                r = talloc();                    /* create new CALL node */
                r->in.op = STCALL;
                r->in.rall = NOPREF;
                r->in.type = INCREF(t);
                r->tn.lval = 0;
                r->tn.rval = 0;
                r->stn.stsize = SZLONGDOUBLE/SZCHAR;
                r->in.right = p->in.left;
                r->in.left = q = talloc();

                q->in.op = ICON;                /* create new Name node */
                q->in.rall = NOPREF;
                q->in.type = INCREF( (TWORD)(FTN + INCREF(t)));
                q->tn.lval = 0;
                q->tn.rval = 0;

                p->in.op = UNARY MUL;           /* use SCONV as dereference */
                p->in.left = r;
		}
	else {
		p->in.op = CALL;
		p->in.right = p->in.left;

		/* put function name in left node of call */
		p->in.left = q = talloc();
		q->in.op = ICON;
		q->in.rall = NOPREF;
		q->in.type = INCREF( FTN + t );
		q->tn.lval = 0;
		q->tn.rval = 0;
		}

	if ( ISPTR(t) ) t = INT;
	switch (t) {
	case UCHAR:
	case CHAR:
	case USHORT:
	case SHORT:
	case INT :
		q->tn.name = addtreeasciz(tl == DOUBLE ? "___fix" :
					  tl == LONGDOUBLE ? "__U_Qfcnvfxt_quad_to_sgl" :
                                       /* tl == FLOAT */ "___fltoi");
		return;

	case UNSIGNED:
		q->tn.name = addtreeasciz(tl == DOUBLE ? "___fixu" :
					  tl == LONGDOUBLE ? "__U_Qfcnvfxt_quad_to_usgl" :
                                       /* tl == FLOAT */ "___fltou");
		return;

	case FLOAT :
		q->tn.name = addtreeasciz(tl == DOUBLE ?   "___dtof" : 
					  tl == UNSIGNED ? "___utofl" :
					  tl == LONGDOUBLE ? "__U_Qfcnvff_quad_to_sgl" :
                                       /* tl == INTEGER */ "___itofl");
		return;
	case DOUBLE :
		q->tn.name = addtreeasciz(tl == FLOAT ?    "___ftod" : 
					  tl == UNSIGNED ? "___floatu" :
					  tl == LONGDOUBLE ? "__U_Qfcnvff_quad_to_dbl" :
                                       /* tl == INTEGER */ "___float");
		return;
	case LONGDOUBLE :
        	q->tn.name = addtreeasciz(tl == FLOAT ? "__U_Qfcnvff_sgl_to_quad" :
                                  	  tl == DOUBLE ? "__U_Qfcnvff_dbl_to_quad" :
                                  	  tl == UNSIGNED ? "__U_Qfcnvxf_usgl_to_quad" :
                               	       /* tl == INTEGER */ "__U_Qfcnvxf_sgl_to_quad");
        return;

	default :
		cerror("hardconv got an impossible convert request");

	}
}

/* harditofl() supports fhw. In the case where a convert from scalar to floating
   point is attempted (except INT), it inserts an intermediate SCONV to INT.
*/
LOCAL harditofl(p)	register NODE *p;
{
	register NODE *q;

	q = talloc();
	ncopy(q, p);
	p->in.left = q;
	p->in.right = 0;
	p->in.op = SCONV;
	p->in.type = INT;
	p->in.rall = NOPREF;
}






/* return 1 if node is a SCONV from short or char to int */
LOCAL shortconv( p )
  register NODE *p;
{
	if (p->in.op == SCONV)
		{
		if (p->in.type == INT && p->in.left->in.type == SHORT)
			return (TRUE);
		if (p->in.type == UNSIGNED && p->in.left->in.type == USHORT)
			return (TRUE);
		}
	return (FALSE);
}





/* shortconv2() returns 1 if the node is a short or char integer constant.
*/
LOCAL shortconv2( p )
NODE *p;
{
	return ( nncon(p) && p->tn.lval >=-32768L && p->tn.lval <= 32767L);
}





/* do local tree transformations and optimizations just as the tree is first
   handed to pass2. Called from local2.c:myreader. At present it does 2 opt-
   imizations to the tree: 1) short muls and divs are changed so that hardware
   can do them directly with muls and divs instructions. No optimization is done
   on unsigned ops although it could be added in a similar fashion if
   it is determined to be worth the effort; 2) floating point comparisons
   are done as a subtraction followed by comparison to zero (0).  This
   should be reconsidered, I think.
*/



pictrans( p )
	register NODE *p;
{
	register short pop;
	register NODE *q;
	NODE *l,*r;

	if ( p == NULL ) cerror("Invalid null pointer in pictrans()");
	pop = p->in.op;
	if ( pop == NAME )
	  {
	  q = talloc();
	  ncopy( q, p );
	  p->in.op = UNARY MUL;
	  p->in.name = NULL;
	  p->in.left = q;
	  p->in.right = NULL;
	  q->in.op = OREG;		/* .lval & .name fields copy */
	  q->tn.rval = picreg;
	  q->in.type = INCREF( q->in.type );
	  }
	else if ( pop == ICON && p->in.name != NULL )
	  {
	  p->in.op = OREG;		/* .lval & .name fields copy */
	  p->tn.rval = picreg;
	  q = p;
	  }
	else
	  q = NULL;

	if ( q != NULL && q->tn.lval )	/* convert involvesd nonzero .lval, */
	  {				/*	hang it as ICON offset */
	  l = talloc();
	  ncopy( l, q );
	  r = talloc();
	  ncopy( r, q );
	  q->in.op = PLUS;
	  q->in.left = l;
	  q->in.right = r;
	  q->in.name = NULL;
	  l->tn.lval = 0;
	  r->in.op = ICON;
	  r->tn.name = NULL;
	  r->tn.rval = 0;
	  }
} /* pictrans */



optim2( p )
  register NODE *p;
  {	register NODE *q;
	register short pop = p->in.op;
	NODE *p2;
	TWORD type1, type2;

	/* NOTE: At this time only signed divide and multiply are optimized into
   	hardware instructions since the respective unsigned ops are pretty rare.
	*/

	/* Divide of a short by a short can be done directly in hardware.
	*/
	/* multiply of two shorts to produce an int can be done directly
	 * in the hardware.
	 */

	switch (pop)
	{
	case MUL:
	case DIV:
	case MOD:
		if ( shortconv(p->in.left)
			&& (shortconv(p->in.right) || shortconv2(p->in.right)) )
			/* Divisor is short, dividend is short */
			/* We cannot do short divide if only the divisor
			   is short because the quotient could be longer
			   than 16 bits thus causing an overflow.
			*/

		{
		flag us = ISUNSIGNED(p->in.left->in.type) ||
			ISUNSIGNED(p->in.right->in.type);
	  	q = talloc();		/* get a new node */
	  	ncopy(q, p);		/* make it into a copy of p */
	  	p->in.op = SCONV;	/* make the top an SCONV to int */
	  	p->in.left = q;	/* top->lhs is the original tree */
	  	p->in.right = NULL;	/* probably superfluous */
	  	p2 = q;		/* from here on, p2 points to the top->lhs */
	
	  	p2->in.type = us? USHORT : SHORT;
	  	p2->in.left = (q = p2->in.left)->in.left;
	  	q->in.op = FREE;
	  	if ( (q=p2->in.right)->in.op==ICON ) q->in.type = SHORT;
	  	else
			{
	    		p2->in.right = q->in.left;
	    		q->in.op = FREE;
	  		}
		}
		break;

	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
	/*	-- These shouldn't be possible with floating pt.
	case ULE:
	case ULT:
	case UGE:
	case UGT:
	*/
	/* first change type of op to match lhs&rhs types -- necessary for
	   floating pt to ensure correct flt compare instructions will be 
	   generated later. Similar to quickfltconv() in pass1.
	*/
	if ( !flibflag && ISFTP(p->in.left->in.type) )
		{
		p->in.type = p->in.left->in.type;
		if (logop(p->in.left->in.op))
			p->in.type = INT;
		}

	/* change <flt exp>1 <logop> <flt exp>2 to
	 * (<exp>1 <ccodes> <exp>2) <logop> 0.0
	 */			/* real comparison spot */
	 /* NOTE : I haven't yet figured out whether or not to try optimizing
		mixed mode <logop> constructions.
	*/
	if (flibflag && 
	   ((type1 = p->in.left->in.type)==FLOAT || type1 == DOUBLE) &&
	   ((type2 = p->in.right->in.type)==FLOAT|| type2 == DOUBLE) &&
	   (type1==type2))
		{
	  	q = talloc();
	  	q->in.op = BICCODES;
		/* BICCODES will be recognized by hardops() and replaced by a
		   call before the remainder of pass 2 sees the tree.
		*/
	  	q->in.rall = NOPREF;
	  	q->in.type = (type1==FLOAT) ? FLOAT : DOUBLE;
	  	q->in.left = p->in.left;
	  	q->in.right = p->in.right;
	  	p->in.left = q;
	  	p->in.right = q = talloc();
	  	q->tn.op = ICON;
	  	q->tn.type = INT;
	  	q->tn.name = NULL;
	  	q->tn.rval = 0;
	  	q->tn.lval = 0;
		}
		break;

	}	/* switch */

	if ( shortconv(p) && (q=p->in.left)->in.op == MUL )
		{
		/* the result of a mul on two shorts is a 32-bit #. Therefore
		   it is necessary to throw away the SCONV at the top before
		   an assignment takes place. Note that this operation is re-
		   quired if the above optimization is to be valid.
		*/
		*p = *q;
		q->in.op = FREE;
		p->in.type = ISUNSIGNED(p->in.type)? UNSIGNED : INT;
		}


	/* this is a convenient place to set fopseen for simple float ops */
	if ( (ISHWFTP(p->in.type) && (dope[pop]&FLOFLG || pop==ASSIGN) )
	  || ((pop == SCONV) && ISHWFTP(p->in.left->in.type) && (p->in.type != LONGDOUBLE))
	  || ((pop == REG) && ISHWFTP(p->in.type))
	  || ((dope[pop]&LOGFLG)
	       && (ISHWFTP(p->in.left->in.type)
		   || ((dope[pop] &BITYPE) && ISHWFTP(p->in.right->in.type)))) )
		{
		fopseen = p;	/* not set to the top here ... will be later */
#ifdef FORT
		if (fpaflag && (pop == FMONADIC))
			{
			/* convert 881 op name to FPA name */
			register struct fmon *i;
			for (i=fmontbl; i->op881; i++)
				{
				if (!strcmp(i->op881,p->in.left->tn.name))
					{
					if (! i->opFPA)
						must881opseen = 1;
					break;
					};
				}
				
			}
#endif
		}

}









/* floattreedup() is called if a basic float op has been seen in pass1. Its
   purpose is to transform the tree into a conditional check on a global
   flag for hardware floating point (set by the operating system). For example,
   if the original tree p is of the form

		p
	       / \
	      l   r

   It will be transformed into the following by adding a superstructure:

		? (same address as old p)
	       / \
	      flag :
		  / \
		 p   p'
		/ \ / \
	       l   rl' r'

*/

floattreedup(p)	register NODE *p;
{
	register NODE *q;
	static char *fname = FPANAME;

	/* first make the new right node -- the colon node */
	q = talloc();
	q->in.op = COLON;
	q->in.rall = NOPREF;
	q->in.type = p->in.type;
	q->in.left = talloc();
	ncopy(q->in.left, p);	/* copy only the node itself */
	q->in.right = tcopy(p);	/* make a copy of the whole subtree */

	/* now make the new tree top -- the ? node */
	p->in.op = QUEST;
	p->in.rall = NOPREF;
	p->in.type = q->in.type;
	p->in.right = q;

	/* now make the left node -- the flag node */
	p->in.left = q = talloc();
	q->tn.op = NAME;
	q->in.rall = NOPREF;
	q->tn.type = SHORT;
	q->tn.name = fname;
	q->tn.lval = 0;
}


/*	floattreemap - map duplicated float tree's Dragon regs to 881 regs */

floattreemap( p ) register NODE *p;
{
	register int r = p->tn.rval;
	if ( ISDNODE( p ) && !ISTREG( r ) )
	  if ( (r < (MINRDVAR + FP0)) || (r > (MAXRFVAR + FP0)) )
	    {
	    cerror("Illegal float reg assigned with +bfpa option");
	    }
	  else
	    p->tn.rval -= (FP0 - F0);	/* map dragon reg -> 881 reg */
}


#ifdef FORTY

/*	floattreemap2 - map apollo routine calls into FMONADIC nodes */

floattreemap2(p) register NODE *p;
{
	register NODE *pchild = p->in.left;
	int name_offset;
	char name[80];

	if ((p->in.op == CALL) || (p->in.op == UNARY CALL))
	    {
	    if ((((!picoption) && (pchild->in.op == ICON)) ||
		 (picoption && ISPIC_OREG(pchild))) &&
		 (strncmp(pchild->in.name,"__fp040_",8) == 0))
		{
		p->in.op = FMONADIC;
		name_offset = (strncmp(pchild->in.name,"__fp040_d",9) == 0) ?
									9 : 8;
		strcpy(name+1,(pchild->in.name+name_offset));
                if (strcmp(name+1,"exp") == 0)
			strcpy(name+1,"etox");
		name[0] = 'f';
		pchild->in.name = addtreeasciz(name);
		}
	    }
}

#endif /* FORTY */


myreader(p) register NODE *p;
{

#	 ifdef DEBUGGING
	if (xdebug) {
		prntf("myreader entry: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#	 endif
	fopseen = 0;	/* reset the trigger before each expression */
	must881opseen = 0;
#ifndef FORT
	walkf( p, rm_paren );
#ifdef DEBUGGING
	if (xdebug>1) {
		prntf("myreader after rm_paren: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#endif
#endif /*FORT*/
	if ( picoption )
	  {
	  walkf( p, pictrans );	/* Position Indep. Code TRANSlation */
#	    ifdef DEBUGGING
	  if (xdebug>1) {
		prntf("myreader after pictrans: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#	    endif	/* DEBUGGING */
	  }
	walkf( p, optim2 );
#	 ifdef DEBUGGING
	if (xdebug>1) {
		prntf("myreader after optim2: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#	 endif

	/* do a tree transformation for support of floating point HW */
	if (fopseen && fpaflag > 0) floattreedup(p);
#	 ifdef DEBUGGING
	if ((xdebug>1) && fopseen && fpaflag > 0)
		{
		prntf("myreader after floattreedup: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#	 endif	/* DEBUGGING */

	/* convert FMONADIC ops or ops to function calls*/
	if (fopseen && fpaflag > 0)
		{
		fopseen = p;	/* set it to the top of the tree ('?' node) */
		fpaside = 0;
		walkf( p->in.right->in.right, floattreemap );
		walkf( p->in.right->in.right, hardops );
		fpaside = 1;
#ifdef FORTY
		walkf( p->in.right->in.left, floattreemap2 );
#endif /* FORTY */
		walkf( p->in.right->in.left, hardops );
		}
	else
		{
		fpaside = fpaflag;
#ifdef FORTY
		if (fopseen && (fpaflag < 0))
			walkf( p, floattreemap2 );
#endif /* FORTY */
		walkf( p, hardops );
		}
#ifdef DEBUGGING
	if (xdebug>1) {
		prntf("myreader after hardops: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#endif	

	/* expands r-vals for fields */
	canon( p );
#	 ifdef DEBUGGING
	if (xdebug>1) {
		prntf("myreader after canon: 0x%x\n", p );
		fwalk( p, eprint, 0 );
		}
#	 endif

}


/* tsize2() returns the number of bytes used for internal storage of a basic
   type (including pointers).
*/
tsize2(btype) TWORD btype;
{
	switch(btype)
		{
		case CHAR:
		case UCHAR:
			return ( sizeof (char) );
		case SHORT:
		case USHORT:
			return ( sizeof (short) );
		case LONG:
		case INT:
		case UNSIGNED:
		case ULONG:
		case FLOAT:
			return ( sizeof (int) );
		case DOUBLE:
			return ( sizeof (double) );
		default:
			if ( ISPTR(btype) )  return ( sizeof (int) );
			cerror("tsize called with a non-simple type");
			return(0);
		}
}





/* rsconv() is strictly a helper function for reduce_sconvs() below. */
LOCAL NODE *rsconv(psconv)	register NODE *psconv;
{
	/* The idea here is to throw away SCONV nodes that are extraneous. For
	   types UCHAR and USHORT they are not extraneous because the upper half
	   must be zero filled. Otherwise they could be treated as negatively
	   signed quantities when brought into the float regs on the 68881.
	*/
	switch(psconv->in.left->in.type)
		{
		case UCHAR:
		case USHORT:
			/* bfext instructions do auto-convert to int */
			if (psconv->in.left->in.op != FLD)
				psconv->in.type = INT;
			return(psconv);
		case ULONG:
		case UNSIGNED:
			if ((psconv->in.type == DOUBLE)
			 || (psconv->in.type == FLOAT))
				return(psconv);
			else
				goto def;
		case DOUBLE:
		case FLOAT:
			if ((psconv->in.type == ULONG)
			 || (psconv->in.type == UNSIGNED))
				return(psconv);
			else
				goto def;
		default:
def:
			if (psconv->in.fpaside)
			{
				return(psconv);
			}
			else
			{
				psconv->in.op = FREE;
				return(psconv->in.left);
			}
		}
}

/* reduce_sconvs() removes or alters selected SCONV nodes to satisfy
   68881 requirements.  In most cases the 68881 will automatically do
   conversions.  In cases involving unsigned operands certain SCONV
   nodes may have to be rewritten.
*/
LOCAL reduce_sconvs(p) register NODE *p;
{
	register NODE *q;

    if (flibflag)
	{
	q = p->in.left;
	if ((p->in.op == SCONV) && (p->in.type == q->in.type))
	    {
	    ncopy(p, q);
	    q->in.op = FREE;
	    }
	return;
	}
    if ( ISFTP(p->in.type) && ( p->in.type != LONGDOUBLE))
	    switch(p->in.op)
	    {
	    case ASSIGN:
		    if ( (!p->in.fpaside) && ((q=p->in.left)->in.op == SCONV))
			    {
			    p->in.left = q->in.left;
			    q->in.op = FREE;
			    }
		    break;
	    case SCONV:
		    while (!p->in.fpaside && ((q=p->in.left)->in.op == SCONV
			   && ISFTP(q->in.type)) )
			    {
			    p->in.left = q->in.left;
			    q->in.op = FREE;
			    }
		    q = p->in.left;		
		    if ( (q->in.type == UCHAR || q->in.type == USHORT)
			&& q->in.op != FLD )
			/* Need to zero fill unsigns for 68881 and DRAGON. */
			p->in.left = makety2(q, INT);
		    else if (p->in.type==q->in.type && !logop(q->in.op) )
			    {
			    ncopy(p, q);
			    q->in.op = FREE;
			    }
		    break;
	    case PLUS:
	    case MINUS:
	    case MUL:
	    case DIV:
	    case ASG PLUS:
	    case ASG MINUS:
	    case ASG MUL:
	    case ASG DIV:
		    /* throw away SCONV nodes above operands that will be used
		    in 68881 regs because the hardware will support auto
		    conversions as they are brought in.
		    */
		    if ( p->in.left->in.op == SCONV )
				p->in.left = rsconv(p->in.left);
		    if ( p->in.right->in.op == SCONV )
			    	p->in.right = rsconv(p->in.right);
		    break;

	    case EQ:
	    case NE:
	    case LE:
	    case LT:
	    case GE:
	    case GT:
		    /* throw away SCONV nodes above operands that will be used
		    in 68881 regs because the hardware will support auto
		    conversions as they are brought in.  Can't do this for
		    relational op's comparing SCONV'ed operand against
		    floating pointer constant 0.0.
		    */
#if defined(FORT) || !defined(ONEPASS)
		/* FORTRAN and split C) don't know 0.0 when they see it */
		    if ((p->in.left->in.op == SCONV) && ISFTP(p->in.right->in.type))
				p->in.left = rsconv(p->in.left);
		    if ((p->in.right->in.op == SCONV) && ISFTP(p->in.left->in.type))
			    	p->in.right = rsconv(p->in.right);
#else
		    if (( p->in.left->in.op == SCONV )
		      && !ISFZERO(p->in.right))
				p->in.left = rsconv(p->in.left);
		    if (( p->in.right->in.op == SCONV )
		      && !ISFZERO(p->in.left))
			    	p->in.right = rsconv(p->in.right);
#endif
		    break;
	    }
#ifndef FORT
	else if (p->in.op == SCONV && (p->in.left->in.type == p->in.type ||
		 (p->in.left->in.op == FLD && (compatibility_mode_bitfields ||
		  (ISUNSIGNED(p->in.type) == ISUNSIGNED(p->in.left->in.type))))))
#else
	else if (p->in.op == SCONV && (p->in.left->in.type == p->in.type ||
		 (p->in.left->in.op == FLD && (
		  (ISUNSIGNED(p->in.type) == ISUNSIGNED(p->in.left->in.type))))))
#endif
		{
		/* Case 1: bfextu and bfexts instructions automatically extend
		   fully without help anyway.
		   Case 2: f77pass1 sometimes passes SCONV nodes that do no
		   conversions.
		*/
		q = p->in.left;
		q->in.type = p->in.type;
		*p = *q;
		q->in.op = FREE;
		}

}








/* pass2 main routine used to be here.  fort.c:main or cpass2:main instead */


/*
 * factor - finds a sequence of shift/add to replace a multiply
 *
 * This routine builds the sequence up in reverse order looking at the
 * last bits:
 *    For a string of two or more 0's: "lsl" is emitted
 *                                      (it was measured to be slightly faster
 *                                      than "asl").
 *    For a single 0:                  "add" is emitted.
 *    For a string of three or more 1's: "sub" is emitted (the next call can
 *                                      then do the lsl).
 *    For a single 1:                  "add" is emitted.
 *
 * If the return value from this routine is negative no sequence of
 * instructions was found.  Otherwise, the number of instructions is
 * returned.
 */
factor(num,code,lev)
     unsigned int num;  /* number to compute */
     char *code[];      /* instruction stream */
     int lev;           /* max number of instructions */
{
#ifndef FLINT
  static char *subone = "\tsub.l\tA1,AL\n";
  static char *addone = "\tadd.l\tA1,AL\n";
  static char *multwo = "\tadd.l\tAL,AL\n";
  static char *shift[] = { "", "", "\tlsl.l\t&2,AL\n",
			   "\tlsl.l\t&3,AL\n","\tlsl.l\t&4,AL\n",
			   "\tlsl.l\t&5,AL\n","\tlsl.l\t&6,AL\n",
			   "\tlsl.l\t&7,AL\n","\tlsl.l\t&8,AL\n" };
  int numshifts = 0;
  if (lev == 0) {
	inst_cost = MAXCOST + 1;
	return(-100);
	}
  else if ((num == 1) || (num == 0)) return(0);
  else if (num & 01) {
    if ((num & 07) == 07) {
      code[0] = subone;
      moveneeded = 1;
      inst_cost += 2;
      return(factor(num+1,&code[1],lev-1)+1);
    }
    else {
      code[0] = addone;
      moveneeded = 1;
      inst_cost += 2;
      return(factor(num-1,&code[1],lev-1)+1);
    }
  }
  else if (num & 02) {
    code[0] = multwo;
    inst_cost += 2;
    return(factor(num/2,&code[1],lev-1)+1);
  }
  else {
    while ((num & 01) == 0) {
      if (numshifts >= 8) break;
      numshifts++;
      num >>= 1;
    }
    code[0] = shift[numshifts];
    inst_cost += 4;
    return(factor(num,&code[1],lev-1)+1);
  }
#endif
}
