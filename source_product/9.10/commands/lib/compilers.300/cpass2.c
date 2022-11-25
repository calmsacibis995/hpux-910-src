/*    SCCS    REV(64.1);       DATE(92/04/03        14:21:59) */
/* KLEENIX_ID @(#)cpass2.c	64.1 91/08/28 */

# include "mfile2"
# include <signal.h>
# include <string.h>
# define NO 0
# define YES 1
extern int usedregs;
extern int usedfdregs;
extern long int *last_dragon_prolog; 
extern char ftitle[];
extern char ftnname[];
extern flag pic_used;

#ifdef PRINTTREE
# define talloc() &tmpnode
# define tcheck()
# define addtreeasciz(buf) (buf)
# define cerror printf
# define uerror printf
# define fprntf fprintf
# define prntf printf
int xbufx;
char xbuff[256];
#include "commonb"
#endif /*PRINTTREE*/

#ifdef DEBUGGING
flag xdebug;
#endif

/*	masks for unpacking longs */

# ifndef FOP
# define FOP(x) (int)((x)&0377)
# endif

# ifndef VAL
# define VAL(x) (int)(((x)>>8)&0377)
# endif

# ifndef REST
# define REST(x) (((x)>>16)&0177777)
# endif

LOCAL FILE * lrd;  /* for default reading routines */


/* lread - reads 1 long while checking for a read error */

LOCAL long lread(){
	static long x;
	if( fread( (char *) &x, 4, 1, lrd ) <= 0 )
		cerror( "intermediate file read error" );
#ifdef PRINTTREE
	xbufx += sprintf(&xbuff[xbufx], "%08x ", x);
#endif /*PRINTTREE*/

	return( x );
	}


/* Unpack the compacted type information.  See trees.c for a description of 
 * the packing scheme.  
 */
#define TYWD1(x)	((x)&0x80000000)
#define TYWD2(x)	((x)&0x40000000)
gettyp(x,tptr,aptr)		
int x;
long *tptr, *aptr;
{
register int tmp;
	
	*tptr = ((x)&0x0fff0000)>>16; 
	*aptr = ((x)&0x30000000)>>28; 
	if (TYWD1(x)) {  /* at least 1 extension word follows */
		tmp = lread(); 
		*tptr |= ((tmp)&0xfffff000); 
		*aptr |= ((tmp)&0xfff)<<6; 
		if (TYWD2(x)) {   /* a second extension word follows */
			tmp = lread(); 
			*aptr |= ((tmp)&0xfffc0000);
		}
	} 
}

					


# define FBUFSIZE 300

#ifndef PRINTTREE
extern char	*addtreeasciz(); /* adds a name to the asciz table & returns
				   a pointer to it*/
#endif /*PRINTTREE*/

static char	lcrbuf[NCHNAM+5]; /* a temporary name building buffer 
				   *   allow NCHNAM + prepended underscore +
				   *   up to 4 nulls in next long word to 
				   *   terminate */
LOCAL lopen( s ) char *s; {
	/* if s null, opens the standard input */
	if( *s ){
		lrd = fopen( s, "r" );
		if( lrd == NULL ) cerror( "cannot open intermediate file %s", s );
		}
	else  lrd = stdin;
	(void)setvbuf(lrd,NULL,_IOFBF,8192);
	}


/* lcread - reads n longs into a buffer lcrbuf. Bytes are later read as
	chars.
*/

LOCAL lcread( n ) {
	register char *cp;
#ifdef PRINTTREE
	long *lp;
#endif /*PRINTTREE*/

	if( n > 0 ){
		if( fread( lcrbuf, 4, n, lrd ) != n ) cerror( "intermediate file read error" );
		/* until f77pass1 puts out well formed asciz names, the
		   following is needed to remove blank pads.
		*/
		for (cp = lcrbuf; *cp && *cp != ' '; cp++) ;
		*cp = '\0';
		}
#ifdef PRINTTREE
		lp = (long *)lcrbuf;
		while (n--) {
			xbufx += sprintf(&xbuff[xbufx],"%08x ", *lp++);
                }
#endif /*PRINTTREE*/
	}

#define SYMLONGS (NCHNAM+5)/sizeof(long)

/* lstrread reads a series of longs from lrd until reading a null char. Used
   primarily to read arbitrary length external ids.
*/

LOCAL lstrread()
{
	register short 	i, j = 1;
	register char 	*lcp;
	register long	*cp = (long *)lcrbuf;

	while (j++ < SYMLONGS)
		{
		lcp = (char *) cp;
		if ( fread(cp++, sizeof(long), 1, lrd) != 1 )
			cerror("intermediate file read error");
#ifdef PRINTTREE
		xbufx += sprintf(&xbuff[xbufx], "%0x ", *(cp-1));
#endif /*PRINTTREE*/

		for (i=1; i<=sizeof(long); i++)
			if (! *lcp++) 
				{
				return; /* return at first null byte */
				}
		}
	return;
}


/* lccopy - copies stuff from stdin into stdout while checking for overflow */

LOCAL lccopy( n ) register n; {
	register i;
	static char fbuf[FBUFSIZE];
#ifdef PRINTTREE
	long *lptr;
#endif /*PRINTTREE*/

	if( n > 0 ){
		if( n > FBUFSIZE/4 ) cerror( "lccopy asked to copy too much" );
		if( fread( fbuf, 4, n, lrd ) != n ) cerror( "intermediate file read error" );
		for( i=4*n; fbuf[i-1] == '\0' && i>0; --i ) { /* VOID */ }
#ifdef PRINTTREE
		lptr = (long *)fbuf;
		while (n--) {
			xbufx += sprintf(&xbuff[xbufx], "%08x ", *lptr++);
		}
#endif /*PRINTTREE*/

		if( i ) {
			if( fwrite( fbuf, 1, i, stdout ) != i ) cerror( "output file error" );
			}
		}
	}

LOCAL bad_fp(sig, code, scp)
{
	uerror("error in floating point constant. - Pass 2.");
	signal(SIGFPE, bad_fp);		/* to reset */
}

/*	stack for reading nodes in postfix form */



#ifdef DEBUGGING

struct backop {short op; char *oname;} backops[] = 
	{
	FTEXT, "FTEXT",
	FEXPR, "FEXPR",
	FSWITCH, "FSWITCH",
	FLBRAC, "FLBRAC",
	FRBRAC, "FRBRAC",
	FEOF, "FEOF",
	FARIF, "FARIF",
	SETREGS, "SETREGS",
	FMAXLAB, "FMAXLAB",
	LABEL,"LABEL",
	ARRAYREF, "ARRAYREF",
	STRUCTREF, "STRUCTREF",
	FICONLAB, "FICONLAB",
	C1SYMTAB, "C1SYMTAB",
	C1OPTIONS, "C1OPTIONS",
	C1OREG, "C1OREG",
	C1NAME, "C1NAME",
	VAREXPRFMTDEF, "VFEDEF",
	VAREXPRFMTEND, "VFEEND",
	VAREXPRFMTREF, "VFEREF",
	SWTCH, "SWTCH",
	NOEFFECTS, "NOEFFECTS",
	-1,""
	};
LOCAL char * xfop(o)	register short o;
{
	register short testop;
	register i;

	if (o >= FORTOPS)
		{
		for (i = 0; (testop = backops[i].op) >= 0; i++)
			if (o == testop) return(backops[i].oname);
		}
	else
		return (opst[o]);
}
# endif


OFFSZ tmpoff_c1; /* offset of first temp before c1 adds more */
extern	char	*taz;
extern int maxtascizsz;		/*  = TASCIZSZ in common */
extern char	*treeasciz;	/* asciz table for syms in tree nodes */
extern char	*lasttac;	/* ptr to first free char in treeasciz */
extern NODE *node;

NODE **fstack;
#define NSTACKSZ 250
int fstacksz = NSTACKSZ;

main( argc, argv ) char *argv[]; {
	register NODE ** fsp;  /* points to next free position on the stack */
	register int files = 0;
	register long x,y;
	register NODE *p;
	int attr; 
	int i;
	int tmp1,tmp2;
	long labs[660];
	long nlabs;
	long coerced_index;
	long arysize;
#ifdef PRINTTREE
	NODE tmpnode;
	char ftitle[FTITLESZ];
	char ftnname[FTNNMLEN];
	int lineno;
	int maxtreg;
	int maxtfdreg;
	int crslab;
        int tmpoff;
        int baseoff;
	int nerrors=0;


	xdebug = 2;
	mkdope();
	lopen(NULL);
#else /*PRINTTREE*/
	(void)p2init( argc, argv );	/* reader.c */
	inittaz();		/* common: Allocates node and taz tables */
	tinit();		/* common:tinit. Initializes expression trees */
	fstack = (NODE **) ckalloc( fstacksz * sizeof(NODE *) );

	signal(SIGFPE, bad_fp);

		
	for (i=1; i<argc; ++i) {
	    if (argv[i][0] != '-') {
		switch (files++) {
		    case 0: lopen(argv[i]);
			    break;
		    case 1: if (freopen(argv[i], "w", stdout) == NULL)
				fprntf(stderr,"%s: cannot open %s\n",
							argv[0],argv[i]);
			    break;
		    default:
			;
		    }
		}
	    }

	fsp = fstack;
#endif /*PRINTTREE*/

	for(;;){
#ifdef PRINTTREE
		if (xbufx > 256) {
			fprintf(stderr, "output buffer overflowed!!!\n");
			exit(1);
		}
		if (xbufx > 0) {
			fprintf(stdout, "**********\n");
			fprintf(stdout, xbuff);
			fprintf(stdout, "\n##########\n");
		}
		xbufx = 0;
#endif /*PRINTTREE*/

		/* read nodes, and go to work... */
		x = lread();

#ifdef DEBUGGING
	if( xdebug ) fprntf( stdout, "op=%s ", xfop(FOP(x)));
	if (xdebug > 1) fprntf( stdout, "val = %d., rest = 0x%x\n",
			VAL(x), (int)REST(x) );
#endif
		switch( (int)FOP(x) ){  /* switch on opcode */

		case 0:
			fprntf( stdout, "null opcode ignored" );
			break;


		case NAME:			/* 2 from manifest */
			p = talloc();
			p->in.op = NAME;
			gettyp(x,&p->in.type,&attr);
			if( VAL(x) ) {
				p->tn.lval = lread();
			}
			else p->tn.lval = 0;
			/* p->tn.rval = lread(); */
			p->tn.rval = -1;
			lstrread( );
			p->in.name = addtreeasciz(lcrbuf);
#ifdef DEBUGGING
			if (xdebug) {
				prntf(" %s  ",p->in.name);
				tprint(p->in.type, p->in.tattrib);
				}
			if (xdebug > 1) 
				prntf("\n\tlval = 0x%x, rval=0x%x",
					p->tn.lval, p->tn.rval);
#endif
			goto bump;

		case ICON:			/* 4 from manifest */
			p = talloc();
			p->in.op = ICON;
			gettyp(x,&(p->in.type),&attr);
			p->tn.lval = lread();
			if( VAL(x) ){
				lstrread( );
				p->in.name = addtreeasciz(lcrbuf);
				p->tn.rval = 0;
				}
			else { 
				p->in.name = 0;
				p->tn.rval = NONAME;
			}
#ifdef DEBUGGING
			if (xdebug) {
				prntf("  0x%x  %s ",
					p->tn.lval, p->in.name);
				tprint(p->in.type, p->in.tattrib);
				}
#endif


bump:
#ifndef PRINTTREE
			p->in.su = 0;
			p->in.rall = NOPREF;
			*fsp++ = p;
			if( fsp >= &fstack[fstacksz] )
				{
				int fstackdiff = fstacksz;
				fstacksz += NSTACKSZ;
				fstack = (NODE **)
					realloc(fstack,fstacksz*sizeof(NODE *));
				if (fstack == NULL)
					cerror("Out of space making expression stack");
				fsp = fstack + fstackdiff;
				}
#endif /*PRINTTREE*/
			break;

                case DBRA:
                        p = talloc();
                        p->in.op = FOP(x);
                        p->in.type = VAL(x);
#ifdef DEBUGGING
			if (xdebug && VAL(x))  prntf(" (short) ");
#endif /* DEBUGGING */

#ifndef PRINTTREE
                        p->in.right = *--fsp;
                        p->in.left = *--fsp;
#endif /*PRINTTREE*/
                        goto bump;

		case GOTO:			/* 0x25 = 37. */
			if( VAL(x) )
				{
				  /* unconditional branch */
				x = lread();
#ifdef DEBUGGING
				if (xdebug)  prntf(" L%d\n", x);
#endif /* DEBUGGING */
#ifndef PRINTTREE
			  	cbgen( 0, x, 'I' );
#endif /*PRINTTREE*/
				break;
				}
			/* otherwise, treat as unary */
			goto def;

		case STARG:
		case STASG:
		case STCALL:
		case UNARY STCALL:
			p = talloc();
			p->in.op = FOP(x);
			/*p->stn.stalign = (VAL(x) + (SZCHAR-1) )/SZCHAR;*/
			p->stn.stalign = VAL(x);
			gettyp(x,&(p->in.type), &attr);
			/*p->stn.stsize = lread()/SZCHAR;*/
			p->stn.stsize = lread();
#ifndef PRINTTREE
			if (optype(p->in.op) == BITYPE)
				p->in.right = *--fsp;
			else
				p->tn.rval = REST(x);
			p->in.left = *--fsp;
#endif /*PRINTTREE*/
#ifdef DEBUGGING
			if (xdebug) {
				tprint(p->in.type, p->in.tattrib);
				prntf("  align=%d, size=%d",
					p->stn.stalign, p->stn.stsize);
				}
#endif /* DEBUGGING */
			goto bump;

		case SETREGS:			/* 0xD0 = 208 */
			/* reserve (or release) certain registers as regvars
			   used in pass1.
			*/

			y = lread();	/* get flt. pt. reg mask */
#ifndef PRINTTREE
			f77setregs(REST(x),y);
#endif /*PRINTTREE*/
#ifdef DEBUGGING
			if (xdebug) {
				prntf("\t%%d0-%%a7: 0x%x, dragon: 0x%x ",
					REST(x),y);
				}
#endif /* DEBUGGING */
			break;

		case REG:			/* 0x5E = 94. from manifest */
			{
			register int rn;
			p = talloc();
			p->in.op = REG;
			gettyp(x,&(p->in.type),&attr);
			rn = VAL(x);
			p->tn.rval = rn;
#ifdef DEBUGGING
			if (xdebug) {
				(p->tn.rval < 8) ?
					prntf(" %%d%d ", rn) :
					prntf(" %%a%d ", rn-8);
				tprint(p->in.type, p->in.tattrib);
				}
#endif /* DEBUGGING */
#ifndef PRINTTREE
			if (rn>D1 && rn!=A0 && rn!=A1) 
				/* i.e. not a FORCE or struct-valued function
				 * %a1 pointer.  Should probably also disallow
				 * 881 and Dragon temp regs here, but they are
				 * not allocated by cpass1
				 */
				{
				rstatus[rn] = (rn < A0)? SAREG
					: (rn < F0)? SBREG
					  : (rn < FP0)? SFREG
								: SDREG;
				if (fpaflag > 0 && rn >= FP0)
				  { /* mapping dragon 1 to 1 onto 881 */
		  		  if ( rn < (MINRDVAR+FP0)
					|| rn > (MAXRFVAR+FP0) )
		       cerror( "Illegal float reg assigned with +bfpa option");
				  else
				    {
		  		    rstatus[ rn - (FP0 - F0) ] = SFREG;
				    rbusy( rn - (FP0 - F0), p->in.type );
				    }
		  		  }
				if ( ( rn >= (MINRFVAR+F0) &&
				       rn <= (MAXRFVAR+F0) )
				    || ( rn >= (MINRDVAR+FP0) &&
				           rn <= (MAXRDVAR+FP0)  ) )
				  incoming_flt_perms = YES;
				}
			rbusy( rn, p->in.type );
			p->tn.lval = 0;
			p->in.name = 0;
#endif /*PRINTTREE*/
			goto bump;
			}

		case OREG:			/* 0x5F = 95. from manifest */
			p = talloc();
			p->in.op = OREG;
			gettyp(x,&(p->in.type),&attr);
			p->tn.rval = VAL(x);
			p->tn.lval = lread();
#ifdef DEBUGGING
			if (xdebug) {
				prntf(" %d(",p->tn.lval);
				(p->tn.rval < 8) ?
					prntf("%%d%d) ", p->tn.rval) :
					prntf("%%a%d) ", p->tn.rval-8);
				tprint(p->in.type, p->in.tattrib);
				}
#endif
			p->in.name = NULL; /* addtreeasciz(lcrbuf); */
			/* cpass1/c1 will assure only %a6 base regs
                         * so don't have to rbusy/rstatus other regs
                         */
			goto bump;

		case FLD:
			p = talloc();
			p->in.op = FLD;
			gettyp(x, &(p->in.type), &attr);
			p->tn.rval = lread();
#ifdef DEBUGGING
			if (xdebug) {
				prntf(" rval=%d ", p->tn.rval);
				tprint(p->in.type, p->in.tattrib);
				}
#endif /* DEBUGGING */
#ifndef PRINTTREE
			p->in.left = *--fsp;
#endif /*PRINTTREE*/
			goto bump;

		case FTEXT:			/* 0xC8 = 200. */
			lccopy( VAL(x) );
			prntf( "\n");
			break;

		case FEXPR:			/* 0xC9 = 201. */
			{
			short oldpicopt;
			lineno = REST(x);
			if( VAL(x) ) {
				lcread( VAL(x) );
				(void)strcpy(ftitle, lcrbuf);
#ifdef DEBUGGING
				if (xdebug) prntf("\t%s\n",ftitle);
#endif /* DEBUGGING */
				}
#ifndef PRINTTREE
			if( fsp == fstack ) break;  /* filename only */
#endif /*PRINTTREE*/
#ifdef DEBUGGING
			if (xdebug) prntf("  %d\n",lineno);
#endif /* DEBUGGING */
#ifndef PRINTTREE
			if( --fsp != fstack )
				{
#ifdef DEBUGGING
				if (xdebug)
				prntf("\tafter processing fstack-fsp= %d.\n",
					fstack-fsp);
#endif /* DEBUGGING */
				uerror( "expression poorly formed" );
				}
			if( lflag ) lineid( lineno, ftitle );
			treetmpoff = tmpoff = baseoff;
			p = fstack[0];
#ifdef DEBUGGING
			if( edebug ) fwalk( p, eprint, 0 );
#endif
			if ( picoption && p->in.op == INIT )
			  {	/* turn off pic transformations for INITs */
			  oldpicopt = picoption;
			  picoption = 0;
			  }
			else
			  oldpicopt = 0;
# ifdef MYREADER
			MYREADER(p);
			/* note : MYREADER is defined to be local2.c:myreader */
# endif

			nrecur = 0;
			delay( p );
			reclaim( p, RNULL, 0 );

			allchk();
			resettreeasciz();
			treset();
			tcheck();		
			if ( oldpicopt )
			  picoption = oldpicopt;
#endif /*PRINTTREE*/
			}
			break;

		case FSWITCH:			/* 0xCA = 202. */
			uerror( "switch not yet done" );
			for( x=VAL(x); x>0; --x ) lread();
			break;

		case FLBRAC:			/* 0xCB = 203. */
			/* VAL(x) was originally designated for maxtreg,
			 * but this 1 byte field is inadequate to hold
			 * it (maxtreg's lower byte is the number of the
			 * highest D register available for use, and the
			 * upper byte is the highest A reg available.
			 * Since cpass1 is doing no register allocation,
			 * the available registers are always D7 and A5
			 * at the beginning of a function.  SETREG nodes
			 * will indicate any change from this default.
			 * Now, VAL=1 means asm statements occurred within
			 * the function - C1 should pass the function unchanged.
			 */
			maxtreg = 0x0507;
			maxtfdreg = lread();
			noreg_dragon = ( maxtfdreg & 0x100 );
			noreg_PIC = ( maxtfdreg & 0x200 );
			maxtfdreg &= 0xffff00ff;
			if ( (noreg_dragon && !fpaflag) 
			    || (noreg_PIC && !picoption) )
			  cerror("Mismatched compiler components being used");
			if ( picoption )
			  {
			  if ( !flibflag && fpaflag && !noreg_dragon )
			    picreg = A3;
			  else
			    picreg = A2;
			  }
			tmpoff_c1 = lread();	/* temps beyond this offset were
						   made by c1 */
			tmpoff_c1 >>= 3; 	/* now in bytes */
			tmpoff = lread();
			if (tmpoff % SZSHORT) tmpoff += SZCHAR;
			baseoff = tmpoff;
			lstrread( );   /* function name */
			strncpy(ftnname, lcrbuf, FTNNMLEN-2);
			ftnname[FTNNMLEN-1] = '\0';	/* ensure teminator */
#ifdef DEBUGGING
			if (xdebug>1)
				{
				if (VAL(x)) prntf(" (asm flag) ");
				prntf("\tname=%s\n",lcrbuf);
				prntf("\tmaxtfdreg=0x%x\n",maxtfdreg);
				prntf("\ttmpoff_c1 = %d\n", tmpoff_c1);
				prntf("\ttmpoff = baseoff = %d",tmpoff);
				}
#endif /* DEBUGGING */
#ifndef PRINTTREE
			if( ftnno != REST(x) ){
				/* beginning of function */
				maxoff = baseoff;
				ftnno = REST(x);
				maxtemp = 0;
				if (profiling)
				  {
				  int plab;
				  plab = GETLAB();
				  prntf( "\tmov.l\t&L%d,%%a0\n", plab );
				  if (picoption)
				    prntf( "\tbsr.l	mcount\n" );
				  else
				    prntf( "\tjsr	mcount\n" );
				  prntf( "\tdata\n");
				  prntf( "L%d:\tlong 0\n", plab);
				  prntf( "\ttext\n");
				  }

	/* routine prolog */

				prntf( "\tlink.l\t%%a6,&LF%d\n", ftnno );
				prntf( "\tmovm.l\t&LS%d,(%%sp)\n", ftnno);
				if ( !flibflag )
				  {
				  prntf( "\tfmovm.x\t&LSF%d,LFF%d(%%a6)\n", 
								ftnno, ftnno);
				  if ( fpaflag )
				    do_ext_prolog( YES, ftnno );
				  }
				if ( picoption && ! (!flibflag && fpaflag))
				  do_ext_prolog( YES, ftnno );

				usedregs = 0;
				usedfdregs = 0;
				incoming_flt_perms = NO;

				}
			else {
				if( baseoff > maxoff ) maxoff = baseoff;
				/* maxoff at end of ftn is max of autos and 
				   temps over all blocks in the function */
				}
			setregs();
#endif /*PRINTTREE*/
			break;

		case FRBRAC:			/* 0xCC = 204. */
#ifndef PRINTTREE
			SETOFF( maxoff, ALSTACK );
			eobl2( 1 );
			incoming_flt_perms = NO;
#endif /*PRINTTREE*/
			break;

		case FEOF:			/* 0xCD = 205. */
			
#ifndef PRINTTREE
				/* set object file "version" directive */
			put_flt_version_directive();
#endif /*PRINTTREE*/
# ifdef DEBUGGING
			tcheck();
			if (xdebug) prntf("\n");
# endif	/* DEBUGGING */
#ifdef PRINTTREE
			if (xbufx > 0) {
				fprntf(stdout, "**********\n");
				fprntf(stdout, xbuff);
				fprntf(stdout, "\n##########\n");
			}
#endif /*PRINTTREE*/
			return( nerrors );

		case LABEL: 
#ifdef DEBUGGING
			if (xdebug)
				prntf(" L%d:\n", (((x)>>8)&0xffffff));
#endif /* DEBUGGING */
			prntf("L%d:\n", (((x)>>8)&0xffffff));
			break;
			
		case FMAXLAB:			/* 211 */
			crslab = lread() + 1;	/* max label used by pass1 */
# ifdef DEBUGGING
			if (xdebug)
				prntf("\tmaxlab=%d",crslab-1);
# endif	/* DEBUGGING */
			break;



		case ARRAYREF:
			lread();
			break;

		case STRUCTREF:
			if (VAL(x))
				y = lread();
#ifdef DEBUGGING
			if (xdebug) {
				if (VAL(x)) 
					{
					prntf(" (primary) ");
					prntf(" base=%d",y);
					}
				}
#endif /* DEBUGGING */
			break;

		case SWTCH:
			{
			register i;
			register dlab;
			CONSZ j, range;
			int nlabs;
			int	flagw;

				nlabs = REST(x);
				flagw = VAL(x);
				range = lread();
				dlab = lread();

#ifdef PRINTTREE
				for (i=1; i<nlabs-1; i++) {
					tmp1 = lread();
					tmp2 = lread();
					printf("\tval=%d\tlab=%d\n",tmp1,tmp2);
				}
#else /*PRINTTREE*/

				if( range>0 && range <= 3*(nlabs-1) && (nlabs-1)>=4 ) 
				/* implement a direct switch */
					dirswtch(nlabs, flagw, range, dlab);
				else {
					swpair *swp;
					swp = (swpair *)
						malloc(sizeof(swpair)*nlabs);
					for (i=1; i<=nlabs-1; i++) {
						swp[i].sval = (int)lread();
						swp[i].slab = (int)lread();
#ifdef DEBUGGING
						if (xdebug)
						    prntf("\tval=%d\tlab=%d\n",
							swp[i].sval, swp[i].slab);
#endif /* DEBUGGING */
						}
					binswtch(swp,1,nlabs-1, 0, flagw, dlab);
					free(swp);
				}
#endif /*PRINTTREE*/
			break;
			}



		case C1OPTIONS:
#ifdef DEBUGGING
			if (xdebug)
				prntf("\tlevel %d", VAL(x));
#endif /* DEBUGGING */
			break;

		case C1OREG:
			gettyp(x,&tmp1,&tmp2);
			x=lread();	/* offset */
			y=lread();	/* c1 attributes */
			if (y&C1ARY) arysize=lread();
			lstrread();
#ifdef DEBUGGING
			if (xdebug) {
				prntf("\t%s ",lcrbuf);
				prntf("offset=%d ",x);
				c1symprint(y, arysize);
				tprint(tmp1, tmp2);
				}
#endif /* DEBUGGING */
			break;

		case C1NAME:
			gettyp(x,&tmp1,&tmp2);
			if (VAL(x))
				(void)lread();
			lstrread();
			x=lread();	/* c1 attributes */
			if (x&C1ARY) arysize=lread();
#ifdef DEBUGGING
			if (xdebug) {
				prntf("\tname=%s ",lcrbuf);
				c1symprint(x,arysize);
				tprint(tmp1, tmp2);
				}
#endif /* DEBUGGING */
			lstrread();   /* user name */
#ifdef DEBUGGING
			if (xdebug) {
				prntf("\tsymbol=%s ",lcrbuf);
				}
#endif /* DEBUGGING */

			break;


		case FENTRY:
		case VAREXPRFMTEND:	/* end of vfe "thunk" */
		case VAREXPRFMTDEF:
		case VAREXPRFMTREF:
		case FICONLAB:
		case C1HIDDENVARS:
		case C1HVOREG:
		case C1HVNAME:
			prntf("unrecognized FORTRAN op-code\n");
			break;

		case NOEFFECTS:
			lstrread();
#ifdef DEBUGGING
			if (xdebug)
				{
				prntf(lcrbuf);
				}
#endif /* DEBUGGING */
			break;

		default:
def:
			p = talloc();
			p->in.op = FOP(x);
			gettyp(x,&(p->in.type),&attr);
#ifdef DEBUGGING
			if (xdebug) {
				prntf("  ");
				tprint(p->in.type, p->in.tattrib);
				}
#endif /* DEBUGGING */
#ifndef PRINTTREE
			switch( optype( p->in.op ) ){

			case BITYPE:			/* 0x8 in manifest */
				p->in.right = *--fsp;
				p->in.left = *--fsp;
				goto bump;

			case UTYPE:			/* 0x4 in manifest */
				p->in.left = *--fsp;
				p->tn.rval = VAL(x);
				goto bump;

			case LTYPE:			/* 0x2 in manifest */
				uerror( "illegal leaf node: %d", p->in.op );
				exit( 1 );
				}
#endif /*PRINTTREE*/
			}
#ifdef DEBUGGING
			if (xdebug) 
				prntf("\n");
#ifndef PRINTTREE
			if (xdebug>2)
				prntf("\tafter processing fstack-fsp= %d.\n",
					fstack-fsp);
#endif /*PRINTTREE*/
#endif
		}
	}


#ifdef DEBUGGING
/* Mnemonic print of C1OREG and C1NAME attribute field */
c1symprint(x,asize)
int x;
long asize;
{
	if (x & C1FARG) prntf(" C1FARG");
	if (x & C1EQUIV) prntf(" C1EQUIV");
        if (x & C1VOLATILE) prntf(" C1VOLATILE" );
	if (x & C1PTR) prntf(" C1PTR");
	if (x & C1ARY) prntf(" C1ARY[%d]",asize);
	if (x & C1FUNC) prntf(" C1FNC");
	if (x & C1REGISTER) prntf(" C1REG");
	if (x & C1EXTERN) prntf(" C1EXTERN");
	prntf(" ");
}
#endif /* DEBUGGING */



#ifndef PRINTTREE
static char *suffixw = ".w";
static char *suffixl = ".l";

dirswtch(nlabs, flagw, range, dlab)
int nlabs; int flagw; int range, dlab;
{
	register i,j,k;
	register swlab;
	register char	*suffix;
    	int swlab1;

	j = lread();
	suffix = (flagw&SWWORD) ? suffixw : suffixl;

	if( j  )
		{
		prntf("\t%s%s\t&", ((j>=1)&&(j<=8))? "subq" : "sub",
			suffix );
		prntf( "%d", j );
		prntf( ",%%d0\n" );
		}

	/* Note that the range depends on the case labels whereas
	   suffix depends on the type of the switch var.
	*/
	prntf( "\tcmp%s\t%%d0,&0x%x\n", suffix, range );
	prntf( "\tbhi%s\tL%d\n", suffix, dlab );
	if (picoption)
		{
#ifdef OLDSWITCH
		prntf("\tlea.l\t(L%d,%%pc,%%za0),%%a0\n",swlab1=GETLAB());
		prntf("\tmov.l\t(0,%%a0,%%d0%s*4),%%d0\n",suffix);
		prntf("\tjmp\t%d(%%pc,%%d0.l)\n",2);
		prntf("L%d:\n",swlab = GETLAB());
		prntf("\tlalign\t4\n");
		prntf("L%d:\n",swlab1);
		prntf("\tlong\tL%d-L%d\n",lread(),swlab);
		k = j;
		for (i=1; i<=nlabs-2; i++)
			{
			j = lread();
			while (++k < j)
				prntf("\tlong\tL%d-L%d\n",dlab,swlab);
			prntf("\tlong\tL%d-L%d\n",lread(),swlab);
			}
#else
		prntf("\tmov.l\tL%d(%s),%%a0\n",swlab1=GETLAB(),rnames[picreg]);
		prntf("\tmov.l\t(%%a0,%%d0%s*4),%%a0\n",suffix);
		pic_used = YES;

		prntf( "\tjmp\t(%%a0)\n");

		prntf("\tdata\n");
		prntf( "L%d:\n", swlab = GETLAB() );
		prntf( "\tlalign\t4\n");

		/* output table */

		prntf( "L%d:\n", swlab1 );
		prntf( "\tlong \tL%d\n", lread());
		k = j;
		for( i=1; i<=nlabs-2; i++ ) {
			j = lread();
			while (++k < j)
				prntf( "\tlong\tL%d\n", dlab);
			prntf( "\tlong\tL%d\n", lread());

			}
		prntf("\ttext\n");
#endif /* OLDSWITCH */
		}
	else
		{
		prntf("\tmov.l\t(L%d,%%za0,%%d0%s*4),%%a0\n",
						swlab1=GETLAB(),suffix);
		prntf( "\tjmp\t(%%a0)\n");
		prntf( "L%d:\n", swlab = GETLAB() );
		prntf( "\tlalign\t4\n");

		/* output table */

		prntf( "L%d:\n", swlab1 );
		prntf( "\tlong \tL%d\n", lread());
		k = j;
		for( i=1; i<=nlabs-2; i++ )
			{
			j = lread();
			while (++k < j)
				prntf( "\tlong\tL%d\n", dlab);
			prntf( "\tlong\tL%d\n", lread());
			}
	  	}
}

binswtch(p,lo,hi,lab, flagw, dlab)
  register swpair *p;
  register int lo,hi;
  int flagw, dlab;
  {	register int i,lab1, sival;

	if (lab) prntf("L%d:",lab);	/* print label, if any */
	if (hi-lo > 4) 			/* if lots more, do another level */
		{
	  	i = lo + ((hi-lo)>>1);	/* index where we'll break this time */
	  	sival = p[i].sval;
	  	prntf( "\tcmp%s\t",flagw&SWWORD ? suffixw:suffixl );
	  	prntf( "%%d0,&0x\%x\n\t%s\tL%d\n", sival, flagw&SWUNSIGN ?
			"bhi.l" : "bgt.l", lab1=GETLAB() );
	  	prntf( "\tbeq.l\tL%d\n", p[i].slab );
	  	binswtch(p,lo,i-1,0,flagw, dlab);
	  	binswtch(p,i+1,hi,lab1,flagw, dlab);
		}
	else 			/* simple switch code for remaining cases */
		{
	  	for( i=lo; i<=hi; ++i )
			{
			sival = p[i].sval;
	    		prntf( "\tcmp%s\t", flagw&SWWORD ? suffixw:suffixl );
	    		prntf( "%%d0,&0x\%x\n\tbeq.l\tL%d\n", sival, p[i].slab );
	  		}
	  	if( flagw & SWDEFL) prntf( "\tbra.l L%d\n", dlab);
		}
}



LOCAL f77setregs(mask,fdmask)	register short mask;
 				register int fdmask;
{
	register unsigned *rstatusp = rstatus;
	register int rval;

	fregs = 0;	/* setup our own count */
	ffregs = 0;
	dfregs = 0;
	for (rstatusp = rstatus; rstatusp <= &rstatus[D7]; rstatusp++)
		{
		if (mask < 0)
			{
			*rstatusp = SAREG;
			}
		else
			{
			*rstatusp = SAREG|STAREG;
			fregs++;
			}
		mask += mask;	/* left shift by 1 */
		}

	/* At this time there's no need to account for Address regs. */
	/* Address registers have no free count associated with them */
	afregs = 0;
	if (fpaflag)
	  mask |= 0x2000;
	if (picoption)
	  mask |= (picreg==A3)? 0x1000 : 0x2000;
	for ( ; rstatusp <= &rstatus[A0+MAXARVAR]; rstatusp++)
		{
		*rstatusp = (mask < 0)? SBREG : (afregs++,STBREG|SBREG);
		mask += mask;	/* left shift by 1 */
		}
	rval = FP0;
	for (rstatusp = &rstatus[FP0]; rstatusp <= &rstatus[FP15]; rstatusp++)
		{
		if (fdmask < 0)
			{
			*rstatusp = SDREG;
			if ( fpaflag > 0
				&& rval >= MINRDVAR + FP0
				&& rval <= MAXRFVAR + FP0)
			  {
			  rstatus[ rval - (FP0 - F0) ] = SFREG;
			  }
			incoming_flt_perms = YES;
			}
		else
			{
			*rstatusp = SDREG|STDREG;
			 if ( fpaflag > 0
				 && rval >= MINRDVAR + FP0
				 && rval <= MAXRFVAR + FP0 )
			  {
			  rstatus[ rval - (FP0 - F0) ] = SFREG|STFREG;
			  ffregs++;
			  }
			dfregs++;
			}
		fdmask += fdmask;	/* left shift by 1 */
		rval++;
		}
	fdmask = fdmask << 8;	/* skip empty byte field */
	if ( fpaflag > 0 )
	    for ( rval = F0 + MINRDVAR - 1; rval >= F0; rval-- )
	       {
	       rstatus[ rval ] = SFREG|STFREG;
	       ffregs++;
	       }
	else
	  {
	  for (rstatusp = &rstatus[F0]; rstatusp <= &rstatus[F7]; rstatusp++)
		{
		if (fdmask < 0)
			{
			incoming_flt_perms = YES;
			*rstatusp = SFREG;
			}
		else
			{
			*rstatusp = SFREG|STFREG;
			ffregs++;
			}
		fdmask += fdmask;	/* left shift by 1 */
		}
	  }
# ifdef DEBUGGING
	if (xdebug > 1)
		{
		prntf("At end of f77setregs, fregs = %d\n",fregs);
		prntf("                     ffregs = %d\n",ffregs);
		prntf("At end of f77setregs, dfregs = %d\n",dfregs);
		}
# endif	/* DEBUGGING */
}



/* This routine initializes the fstack and node arrays, which are the only
 *   arrays in inittabs() which are needed by the second pass
 */
init_p2_tabs()
{
	node = (NODE *) ckalloc ( maxnodesz * sizeof (NODE) );
	fstack = (NODE **) ckalloc( fstacksz * sizeof(NODE *) );

}
#endif /*PRINTTREE*/
