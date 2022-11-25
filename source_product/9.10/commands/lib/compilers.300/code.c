/* file code.c */
/* SCCS code.c    REV(64.1);       DATE(92/04/03        14:21:55) */
/* KLEENIX_ID @(#)code.c	64.1 91/08/28 */

#ifdef BRIDGE
# include "bridge.I"
#else /*BRIDGE*/

# define _HPUX_SOURCE       /* for 'signal.h' */
# include <stdio.h>
# include <signal.h>

# include "mfile1"
# include "messages.h"

#ifndef IRIF
# include "mac2defs"
# include "vtlib.h"
#endif /* not IRIF */

#endif /*BRIDGE*/

# ifndef YES
# define YES 1
# endif

extern short picoption;
extern short picreg;
extern flag pic_used;
extern flag compiletoassembler;


# define BREAKINSTLEN	6	/* length of bra.l "break" jump instr. */

#if defined(NEWC0SUPPORT) || defined(BRIDGE)
#define STATICFLAG 0x40
#endif /* NEWC0SUPPORT || BRIDGE */

#ifndef IRIF
#ifdef ANSI
extern int auto_aggregate_label; /* current label */
#endif
#endif /* not IRIF */

extern bad_fp();

#ifdef ONEPASS
extern int usedregs;	/* bit == 1 if reg was used in subroutine */
extern int usedfdregs;	/* bit == 1 if reg was used in subroutine */
extern char *rnames[];

#else /* ONEPASS */

extern OFFSZ p1maxoff;	/* pass1 version of maxoff */
int usedregs;	/* bit == 1 if reg was used in subroutine */
int usedfdregs;	/* bit == 1 if reg was used in subroutine */
char *
rnames[]= {  /* keyed to register number tokens */

	"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
	"%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp",
	"%fp0", "%fp1", "%fp2", "%fp3", "%fp4", "%fp5", "%fp6", "%fp7"
       ,"%fpa0", "%fpa1", "%fpa2", "%fpa3", "%fpa4", "%fpa5", "%fpa6",
	"%fpa7", "%fpa8", "%fpa9", "%fpa10", "%fpa11", "%fpa12", "%fpa13",
	"%fpa14", "%fpa15"
	};
extern char ftnname[];
#endif /* ONEPASS */

extern long int *last_dragon_prolog;

flag proflag;
flag strftn;		/* is the current function one which returns a value */
#ifdef ANSI
flag ldftn;		/* does current function return a long double value */
#endif /* ANSI */
#if defined(NEWC0SUPPORT) || defined(BRIDGE)
#    ifndef ONEPASS
flag staticftn;
#    endif /* ONEPASS */
#endif /* NEWC0SUPPORT || BRIDGE */

#ifndef IRIF
flag dataflag;		/* true if .data was seen last */
flag bssflag;           /* true if .bss was seen last */
FILE *ctmpfile;		/* temp file for string consts. */
FILE *ftmpfile;		/* temp file for float and double consts. */
FILE *dtmpfile;		/* temp file for debugger symbol table gntt */
FILE *ltmpfile;		/* temp file for debugger symbol table lntt */

FILE *outfile = stdout;
#ifndef ONEPASS
FILE *textfile = stdout;
char buff[1024];
char *bp; 
#endif /* ONEPASS */
#endif /* not IRIF */



int stroffset;		/* computed offset for save location of a1 in curftn */
char *ftmpname;
char *tmpname;
char *dtmpname;
char *ltmpname;
short lastloc = PROG;

#ifndef IRIF

branch( n ) unsigned n; {
	/* output a branch to label n */
#ifdef ONEPASS
	prntf( "\tbra.l\tL%d\n", n );
#else
	p2triple(GOTO, 1, 0);  /* VAL=read_label_flag, REST=unused */
	p2word(n);
	p2triple(FEXPR, 0, lineno);
#endif
	}

locctr( l ){
	/* l is PROG, ADATA, DATA, STRNG, ISTRNG, BSS, GNTT or LNTT */
	short temp;

	if( l == lastloc ) return(l);
	temp = lastloc;
	lastloc = l;
	switch( l ){

	case PROG:
#ifdef ONEPASS
		outfile = stdout;
#else
		outfile = textfile;
#endif /* ONEPASS */
		if (dataflag || bssflag) {
#ifdef ONEPASS
		  prntf( "\ttext\n" );
#else
		  p2pass( "\ttext" );
#endif /* ONEPASS */
		  dataflag = bssflag = 0;
		  }
		break;

	case DATA:
#ifdef ONEPASS
		outfile = stdout;
#else
		outfile = textfile;
#endif /* ONEPASS */
		if( !dataflag )
#ifdef ONEPASS
			prntf( "\tdata\n" );
#else
			p2pass( "\tdata" );
#endif /* ONEPASS */
		dataflag = 1;
		bssflag = 0;
		break;

        case BSS:
#ifdef ONEPASS
		outfile = stdout;
		if (!bssflag)
		        prntf( "\tbss\n");
#else
		outfile = textfile;
		if (!bssflag)
		        p2pass( "\tbss");
#endif /* ONEPASS */
		dataflag = 0;
		bssflag = 1;
		break;

	case ADATA:	/* used for float/double constants only */
		if (ftmpfile == NULL)
			{
			ftmpfile = fopen( ftmpname, "w+" );
			if(ftmpfile == NULL) cerror( "Cannot open temp file" );
			(void)unlink(ftmpname);
			}
		outfile = ftmpfile;
		break;

	case STRNG:
		outfile = ctmpfile;
		break;

#ifdef HPCDB
	case GNTT:
		outfile = dtmpfile;
		break;
	case LNTT:
		outfile = ltmpfile;
		break;
#endif /* HPCDB */

	default:
		cerror( "illegal location counter in locctr()" );
		}

	return( temp );
	}

deflab( n ) unsigned n; {
	/* output something to define the current position as label n */
#ifdef ONEPASS
	fprntf( outfile, "L%d:\n", n );
#else
	p2triple(LABEL, n%256, n>>8);
#endif /* ONEPASS */
	}

#ifndef ONEPASS
deftextlab( n ) unsigned n; {
	/* Like deflab, except that the label is written as an FTEXT
	 *  record, rather than a LABEL node.  This is used for float 
	 *  and string constants, so that C1 won't throw away this label
	 *  (it appears not to be a target of any jump).
	 */
char labstr[16];
	sprntf(labstr, "L%d:", n);
	p2pass(labstr);
}


tdeflab( n ) unsigned n; {
	/* output something to define the current position as label n 
	     in the ctmpfile string */
	fprntf( outfile, "L%d:\n", n );
	}

#endif /* ONEPASS */

#ifndef BRIDGE
efcode(){
	/* Code for the end of a function */

	/* First, cause a dance around the svf copy code to avoid imporoper
	   copy operations in two cases:
		1. The function is called for side effects only.
		2. The nerd programmer is forgetting to assign an explicit
		   return value.
	*/
#ifdef ONEPASS
#ifdef ANSI
	if (strftn||ldftn) branch(svfdefaultlab);
#else
	if (strftn) branch(svfdefaultlab);
#endif /* ANSI */
#endif /* ONEPASS */
		
	deflab( (unsigned)retlab );
#ifdef HPCDB
	if (cdbflag) (void)sltexit();
#endif /* HPCDB */
#ifndef ONEPASS
	if (strftn)
	tsize( DECREF(curftn->stype), curftn->dimoff+1, curftn->sizoff, NULL ,0); /* SWFfc00726 fix */
#endif /* ONEPASS */
#ifdef ONEPASS
	if( strftn ){  /* copy output (in r0) to caller */
		struct symtab *p;
		register unsigned size;
		register unsigned short i;
		unsigned l, lsize;

		p = curftn;

		prntf("\tmov.l\t%d(%%a6),%%a1\n", stroffset);
		prntf( "\tmov.l\t%%d0,%%a0\n" );
		prntf( "\tmov.l\t%%a1,%%d0\n" );
		size = tsize( DECREF(p->stype), p->dimoff+1, p->sizoff, NULL, 0 ) / SZCHAR; /* SWFfc00726 fix */
		while( size > 0 ) /* simple load/store loop */
			{
			lsize = (size/4) % 32768;
			if (size > 8 && lsize > 1)
				{
				prntf("\tmov.w\t&%d,%%d1\n", lsize -1);
				deflab(l = GETLAB() );
				prntf("\tmov.l\t(%%a0)+,(%%a1)+\n");
				prntf("\tdbf\t%%d1,L%d\n", l);
				size -= lsize * 4;
				}
			else
				{
		  		i = (size > 2) ? 4 : 2;
		  		prntf("\tmov.%c\t(%%a0)+,(%%a1)+\n",
					i==2 ? 'w' : 'l');
		  		size -= i;
				}
			}
		deflab (svfdefaultlab);
		dataflag = bssflag = 0;
		/* turn off strftn flag, so return sequence will be generated */
		strftn = 0;
		}
#ifdef ANSI
	if (ldftn) {
		deflab(svfdefaultlab);
		prntf("\tmov.l\t%d(%%a6),%%d0\n", stroffset);
		dataflag = bssflag = 0;
		ldftn = 0;
		}
#endif /* ANSI */

	p2bend();
# ifdef HPCDB
	 if (cdbflag) cdb_efunc();
# endif
#else	/* ONEPASS */
	p2triple(FRBRAC,0,0);
	fixlbrac(ftnno, p1maxoff);
#endif /* ONEPASS */

	}
#endif /* BRIDGE */



	/*
	Calling code sequence for structure valued functions.

	a. Calling routine (for each call - static):
		1) Makes a temporary for the function result on the stack
		   just like other temporaries are made.
		2) Updates its own requirements for local storage (_Mx)
		   based on the type of the called function.
		3) Puts the address of the temporary location in a1.
		4) Does the jbsr with normal parameter passing.
	b. Called routine
		1) Normal function protocol.
		1a) Saves a1 in local storage as the first auto variable.
		1b) Updates location of first usable local data space.
		2) Does whatever it does.
		3) For return, reload a1.
		4) Do a multiple copy indirect from a0 to a1 of the return value.
		5) Puts address of the return value (original contents
		   of a1) in d0 for return.
		6) Does a normal return.
	c. Calling routine, upon return.
		1) Treats function as though it was a pointer function.
	*/


/* code for the beginning of a function; a is an array of indices in stab for
   the arguments; n is the number.
*/
bfcode( a, n ) struct symtab *a[]; {
	register short i;
	register temp;
	register struct symtab *p;
	int off;
	char type;
#ifndef ONEPASS
	int regflag;
#endif /* ONEPASS */

#ifndef BRIDGE
	p = curftn;
#ifdef  C1_C
	putlbrac(ftnno,p->sname);
#ifdef NEWC0SUPPORT
	if (c0flag)
	    {
	    p2triple(FEXPR,((strlen(ftitle)+3)/4), lineno);
	    p2str(ftitle);
	    }
#endif /* NEWC0SUPPORT */
	strncpy(ftnname, p->sname, FTNNMLEN-2);
	ftnname[FTNNMLEN-1] = '\0';	/* just in case name was truncated */
#endif  /* C1_C */
	temp = p->stype;
	temp = DECREF(temp);
	strftn = (temp==STRTY) || (temp==UNIONTY);
#ifdef ANSI
	ldftn = (temp == LONGDOUBLE);
#endif /* ANSI */

	retlab = GETLAB();
#endif /* BRIDGE */

#ifdef ONEPASS
#ifndef BRIDGE
#ifdef ANSI
	svfdefaultlab = (strftn||ldftn) ? GETLAB() : retlab;
#else
	svfdefaultlab = strftn? GETLAB() : retlab;
#endif /* ANSI */
#endif /* BRIDGE */

	if( proflag ){
		int plab;
		plab = GETLAB();
		prntf( "\tmov.l\t&L%d,%%a0\n", plab );
#ifdef FORTY
		if (picoption || fortyflag)
#else
		if (picoption)
#endif
		  prntf( "\tbsr.l	mcount\n" );
		else
		  prntf( "\tjsr	mcount\n" );
		prntf( "\tdata\nL%d:\tlong 0\n\ttext\n", plab);
		dataflag = bssflag = 0;
		}

	/* routine prolog */

	prntf( "\tlink.l\t%%a6,&LF%d\n", ftnno );
	prntf( "\tmovm.l\t&LS%d,(%%sp)\n", ftnno);
	if ( !flibflag )
	  {
	  prntf( "\tfmovm.x\t&LSF%d,LFF%d(%%a6)\n", ftnno, ftnno);
	  if ( fpaflag )
	    do_ext_prolog( oflag, ftnno );
	  }
	if (picoption && ! (!flibflag && fpaflag))
	  do_ext_prolog( oflag, ftnno );
#else  /* ONEPASS */
#ifndef BRIDGE
	svfdefaultlab = retlab;
#ifdef NEWC0SUPPORT
	if (p->sclass == STATIC)
		staticftn = STATICFLAG;
	else
		staticftn = 0;
	if (c0flag)
		{
		if (p->stype != FVOID)
		    p2pass("#5 retval");
		bp = buff;
		bp += sprintf(bp,"#c0 s%.8x,%.4x",DECREF(ctype(p->stype)),n);
# ifndef ANSI
        	for( i=0; i<n; ++i )
# else
        	for( i=n-1; i>=0; --i)
# endif
                	{
			if (i < 98)
				bp += sprintf(bp,",%.8x",ctype(a[i]->stype));
			}
		p2pass(buff);
		}
#endif /* NEWC0SUPPORT */
#endif /* BRIDGE */
#endif /* ONEPASS */

#ifndef BRIDGE
	usedregs = 0;
 	usedfdregs = 0;
	off = ARGINIT;

/* Due to the implementation of prototypes, the ANSI compiler ends up
 * doing the psave's in the opposite order.
 */
# ifndef ANSI
	for( i=0; i<n; ++i ){
# else
	for( i=n-1; i>=0; --i) {
# endif
		p = a[i];
		/* NOTE: oalloc() computes offsets for each of the parameters
		   in turn here and these offsets are put into the respective
		   symtab entries. None of the global counters are updated
		   at this time, however; only the local counter 'off' is
		   changed.
		*/
#ifndef ONEPASS
		if (optlevel < 2)
		{
#endif  /* ONEPASS */
		if( p->sclass == REGISTER )
			{
			temp = p->offset;  /* save register number */
			p->sclass = PARAM;  /* forget that it is a register */
			p->offset = NOOFFSET;
			(void)oalloc( p, &off );
			if (p->stype==CHAR || p->stype==UCHAR || p->stype == SCHAR) type = 'b';
			else if (p->stype==SHORT || p->stype==USHORT) type = 'w';
			else if (ISFTP(p->stype))
				cerror("register float types not supported");
			else type = 'l';
#ifdef ONEPASS
			prntf( "\tmov.%c\t%d(%%a6),%s\n", type, p->offset/SZCHAR,
			  rnames[temp] );
#else  /* ONEPASS */
			sprntf(buff, "\tmov.%c\t%d(%%a6),%s\n", type, 
				p->offset/SZCHAR, rnames[temp] );
			p2pass(buff);
#endif  /* ONEPASS */
 			if (temp < 16)		/* really temp < F0 */
 			  usedregs |= 1<<temp;
 			else
 			  usedfdregs |= 1<<(temp-16); /* temp-F0 */
			p->offset = temp;  /* remember register number */
			p->sclass = REGISTER;   /* remember that it is a register */
			}
		else {
			if( oalloc( p, &off ) ) cerror( "bad argument" );
# ifdef ANSI
			/* Old-style function with a FLOAT parameter.  Under
			 * ANSI, the type in now left at FLOAT (not rewritten
			 * to DOUBLE), but a DOUBLE is still passed on the
			 * stack.  So prologue code is needed to convert the
			 * stack value from DOUBLE to FLOAT.
			 */
			if (p->stype==FLOAT && p->sflags&SFARGID)
			    convert_float_arg(p);
# endif /* ANSI */
			}

#ifndef ONEPASS
		}
		else
		{
		/* Force register-declared parms to be accessed from stk */
			regflag = (p->sclass == REGISTER) ? TRUE : FALSE;
			p->sclass = PARAM;
			p->offset = NOOFFSET;
			if( oalloc( p, &off ) ) cerror( "bad argument" );
# ifdef ANSI
			if (p->stype==FLOAT && p->sflags&SFARGID)
			   convert_float_arg(p);
# endif /* ANSI */
			putsym(C1OREG, 14, p);
			p2word((p->offset)/SZCHAR);
			putc1attr(p, regflag);	/* c1 attribute word */
			p2name(p->sname);
		}
#endif /* ONEPASS */

		}
#ifdef ANSI
	if (strftn||ldftn)
#else
	if (strftn)
#endif /* ANSI */
		{
		/* make the address an auto var */
		char save_class;
		p = curftn;
		save_class = p->sclass;		/* temp save */

		p->sclass = AUTO;
		p->stype = INCREF(p->stype);
		p->offset = NOOFFSET;
		(void)oalloc(p, &autooff);
#ifdef ONEPASS
		p->sclass = save_class;		/* restore */
		p->stype = DECREF(p->stype);
		prntf("\tmov.l	%%a1,%d(%%a6)\n", stroffset = p->offset/SZCHAR);
#else
		/* Emit a tree as for "mov.l %a1,-4(%a6)" */
		putsym(C1OREG, 14, p);
		p2word(-autooff/SZCHAR);
		putc1attr(p, 0 /*regflag*/ );  /* c1 attribute word */
		p2name("%a1-save");
		puttyp(OREG, 14, p->stype, 0);
		p2word(stroffset = p->offset/SZCHAR);
		puttyp(REG, 9, p->stype, 0);
		puttyp(ASSIGN, 0, p->stype, 0);
		p2word(FEXPR, 0, 0);
		p->sclass = save_class;		/* restore */
		p->stype = DECREF(p->stype);
#endif /* ONEPASS */
		}

# ifdef HPCDB
	if (cdbflag) cdb_bfunc(curftn,a,n);
# endif

#ifndef ONEPASS
#ifdef NEWC0SUPPORT
	if (c0flag) p2pass("#c0 pe");
#endif /* NEWC0SUPPORT */
#endif /* ONEPASS */
#endif /* BRIDGE */
	}

#ifndef BRIDGE
# ifdef ANSI
/* convert_float_arg :
 * Generate prologue code to convert the call stack value from DOUBLE 
 * to FLOAT for an old-style FLOAT parameter.
 *
 * We have an old-style function definition with a FLOAT parameter.  Under
 * ANSI the parameter type is now left as FLOAT (not rewritten to DOUBLE),
 * but a DOUBLE is still passed on the stack. So we need to generate
 * code to convert the parameter from DOUBLE to FLOAT and put it back on
 * the stack.
 * We  assume 68881 available, and %fp0 is scratch.
 */

convert_float_arg(p)
  struct symtab *p;
{ int offset;
	offset = p->offset/SZCHAR;
# ifdef ONEPASS
	prntf("\tfmov.d %d(%%a6),%%fp0\n", offset);
	prntf("\tfmov.s %%fp0,%d(%%a6)\n", offset);
# else
	sprntf(buff,"\tfmov.d %d(%%a6),%%fp0\n\tfmov.s %%fp0,%d(%%a6)\n",
		offset, offset);
	p2pass(buff);
# endif


}
# endif /* ANSI */

bccode(){ /* called just before the first executable statment in a BLOCK. */
	/* by now, the automatics and register variables are allocated */
	SETOFF( autooff, SZINT );
	/* set aside store area offset */
	p2bbeg( autooff, regvar, fdregvar );
	}
#endif /* BRIDGE */

#ifdef BRIDGE
void defnam(name, class, label)
   char    *name;
   int            class, label;
/*
 * name is the external name.
 */
{
static char buff[NCHNAM + 16 + 1];

       /* define the current location as name */

        if (class == EXTDEF ){
                sprntf(buff, "	global	%s", name);
                p2pass(buff);
        }

        if (class == STATIC && label != NOLAB) deftextlab(label);
        else {
                sprntf(buff, "%s:", name);
                p2pass(buff);
        }

}
#else /* BRIDGE */
defnam( p ) register struct symtab *p; {
	/* define the current location as the name p->sname */

	if( p->sclass == EXTDEF ){
#ifdef ONEPASS
		prntf( "	global	%s\n", exname( p->sname ) );
#else
		sprntf(buff, "	global	%s", exname( p->sname ) );
		p2pass(buff);
#endif /* ONEPASS */
		}
#ifdef ONEPASS
	if( p->sclass == STATIC && p->slevel>1 ) deflab( (unsigned)p->offset );
#ifdef ANSI
	else if (p->sclass == AUTO||p->sclass == REGISTER) deflab(auto_aggregate_label);
#endif
	else prntf( "%s:\n", exname( p->sname ) );
#else
	if( p->sclass == STATIC && p->slevel>1 ) deftextlab( p->offset );
#ifdef ANSI
	else if (p->sclass==AUTO||p->sclass==REGISTER) deftextlab(auto_aggregate_label);
#endif
	else {
		sprntf(buff, "%s:", exname( p->sname ) );
		p2pass(buff);
		}
#endif /* ONEPASS */

	}
#endif /* BRIDGE */

#endif /* IRIF not defined */

#ifndef BRIDGE
bycode( t, i, type, done ) register i;
	{
	/* emit t as a string of bytes or longs */

	i &= 0xf;

	if( i == 0 ) 
	     fprntf( outfile, (( type == WIDECHAR ) ? "\tlong\t" : "\tbyte\t" ));
	else fprntf( outfile, "," );

	fprntf( outfile, "%d", t );

	if( (i == 0xf) || done ) fprntf( outfile, "\n" );
   }

#ifndef IRIF

zecode( n ){
	/* n integer words of zeros */
	register ii;

	if( n <= 0 ) return;
	ii = n;
	inoff += ii*SZINT;
	if (ii > 0)
#ifdef ONEPASS
		prntf( "\tspace\t4*%d\n", ii);
#else
		sprntf(buff, "\tspace\t4*%d", ii);
		p2pass(buff);
#endif /* ONEPASS */
	}

/*ARGSUSED*/
fldal( t ) unsigned t; { /* return the alignment of field of type t */
     /* "incorrect field type" */
     UERROR( MESSAGE( 57 ));
     return(al_int[align_like]);
	}

#endif /* not IRIF */

#ifdef ANSI
extern flag cpp_ccom_merge;       /* true if cpp/ccom one process */
#endif

#ifdef HAIL_GLUE_HACK
#ifdef ANSI
main_ansi( argc, argv )  char *argv[];
#else /* ANSI */
main_compat( argc, argv )  char *argv[];
#endif /* ANSI */
#else /* HAIL_GLUE_HACK */
main( argc, argv )  char *argv[];
#endif /* HAIL_GLUE_HACK */
{
	void dexit();
	register int c, i;
	int fdef = 0;
	int r;
        int len; /* CLL4600054 fix  */

#ifdef ANSI
	if( cpp_ccom_merge = ( strncmp( "cpp", argv[0], 3 ) == 0 )) {

	     /* the preprocessor and scanner are merged into one process */
             /* CLL4600054 fix - Break early if argv[i] is equal to ccom,
                                 ccom.ansi,cpass1 or cpass1.ansi.        */

	     for( i=1; i<argc; ++i ) {
                  len = strlen(argv[i]);
		  if(  ( strncmp( "ccom", argv[i], len ) == 0 )
                     ||( strncmp( "ccom.ansi", argv[i], len ) == 0 )
                     ||( strncmp( "cpass1.ansi", argv[i], len ) == 0 )
		     ||( strncmp( "cpass1", argv[i], len ) == 0 )) break;
	     }
	     startup( i,argv );
	     argc -= i;
	     argv += i;
	     fdef = 1;
	}
#endif /* ANSI */

	for( i=1; i<argc; ++i ) {
		if (argv[i][0] == '-')	/* a debugging argument of some kind */
			{
			if (argv[i][1] == 'Y')
				{
#ifdef DEBUGGING
				/* make stdout unbuffered for debugging */
				setbuf(stdout, 0);
#endif
				if (argv[i][2] == 'p') proflag = 1;
				}
			}
		else switch (fdef++ ) {
#ifdef ONEPASS
			case 0:
			case 1:
				if( freopen(argv[i], fdef==1 ? "r" : "w", fdef==1 ? stdin : stdout) == NULL) {
					stderr_fprntf("ccom:can't open %s\n", argv[i]);
					exit(1);
				}
				if ((fdef == 2) && (truncate(argv[i],0))){
					stderr_fprntf("ccom:can't truncate %s\n", argv[i]);
					exit(1);
				    }
				    
# ifdef HPCDB				
				/*  Set up ftitle here. */
				if (fdef==1) {
				   {  char *cp,*argv_cp;

				      for( cp=ftitle+1, argv_cp=argv[i]; 
				           *argv_cp!='\0' && cp<&ftitle[FTITLESZ-3];
				           cp++,argv_cp++) 
					 *cp = *argv_cp;
				      
				      ftitle[0] = '"';
				      *cp++ = '"';
				      *cp = '\0';
				   }
				}
# endif				
				break;

#else
			case 0: 
				if( freopen(argv[i], "r",  stdin) == NULL) {
					stderr_fprntf("%s:can't open %s\n", argv[0],argv[i]);
					exit(1);
				}
				break;

			case 1:
#ifdef IRIF
				ir_outfile( argv[i] );
				break;
#else /* not IRIF */
				if((textfile= fopen(argv[i],"w")) == NULL) {
					stderr_fprntf("%s:can't open %s\n", argv[0],argv[i]);
					exit(1);
				}
				outfile=textfile;
				break;
#endif /* not IRIF */
#endif /* ONEPASS */
			default:
				;
			}
	}

	tmpname = (char *)tempnam("/tmp","pc");
	ftmpname = (char *)tempnam("/tmp","pcf");
	signal(SIGFPE, (void *)bad_fp);
	if(signal( SIGHUP, SIG_IGN) != SIG_IGN) signal(SIGHUP, dexit);
	if(signal( SIGINT, SIG_IGN) != SIG_IGN) signal(SIGINT, dexit);
	if(signal( SIGTERM, SIG_IGN) != SIG_IGN) signal(SIGTERM, dexit);
#ifndef IRIF
	ctmpfile = fopen( tmpname, "w+" );
	if(ctmpfile == NULL)
		cerror( "Cannot open temp file" );
	(void)unlink(tmpname);	/* Not actually lost until closed */
#endif /* not IRIF */

# ifndef DEBUGGING
	/* attempt to tune ccom */
	(void)setvbuf(stdout, NULL, _IOFBF, 8192);
	(void)setvbuf(stdin,  NULL, _IOFBF, 8192);
	(void)setvbuf(ctmpfile,  NULL, _IOFBF, 8192);
# endif  /* DEBUGGING */

        r = mainp1( argc, argv );	/* mainp1 will in turn call mainp2 */

#ifdef IRIF
#ifdef HPCDB
	if( cdbflag ) 
	     remove_vt_tempfile();
#endif /* HPCDB */
	ir_gen_code(&r);
	return( r );
   }

#else  /* not IRIF */

	/* first see if the file buffer was ever written     */
	/* if not, reset the write flag and move the current */
	/* pointer to base and adjust cnt; otherwise do the  */
	/* actual rewind   */
	if ((long)lseek(fileno(ctmpfile),0,1) == 0)
		{
		ctmpfile->__flag &= ~_IOWRT;
		ctmpfile->__cnt = (int)(ctmpfile->__ptr - ctmpfile->__base);
		ctmpfile->__ptr = ctmpfile->__base;
		}
	else
		rewind (ctmpfile);
	if (ftmpfile)
		{
		if ((long)lseek(fileno(ftmpfile),0,1) == 0)
			{
			ftmpfile->__flag &= ~_IOWRT;
			ftmpfile->__cnt =
			     (int)(ftmpfile->__ptr - ftmpfile->__base);
			ftmpfile->__ptr = ftmpfile->__base;
			}
		else
			rewind (ftmpfile);
		}
#ifdef HPCDB
	if (cdbflag)
		{
		if ((long)lseek(fileno(dtmpfile),0,1) == 0)
			{
			dtmpfile->__flag &= ~_IOWRT;
			dtmpfile->__cnt =
			     (int)(dtmpfile->__ptr - dtmpfile->__base);
			dtmpfile->__ptr = dtmpfile->__base;
			}
		else
			rewind (dtmpfile);
		if ((long)lseek(fileno(ltmpfile),0,1) == 0)
			{
			ltmpfile->__flag &= ~_IOWRT;
			ltmpfile->__cnt =
			      (int)(ltmpfile->__ptr - ltmpfile->__base);
			ltmpfile->__ptr = ltmpfile->__base;
			}
		else
			rewind (ltmpfile);
		}
#endif /* HPCDB */
	if( ftmpfile ) 
		{
		(void)locctr(PROG);
		while ( (c=getc(ftmpfile)) != EOF )
#ifdef ONEPASS
			(void)putchar(c);
#else
			putc(c,textfile);
#endif /* ONEPASS */
		}
	(void)locctr(DATA);
#ifdef ONEPASS
	while((c=getc(ctmpfile)) != EOF ) 
		(void)putchar(c);
#else
	bp=buff;
	while((c=getc(ctmpfile)) != EOF ) {
		if (c == '\n') {
			*bp = '\0';
			p2pass(buff);
			bp = buff;
		} else *bp++ = c;
	}
#endif /* ONEPASS */
#ifdef HPCDB
		if (cdbflag) 
			{
			char *vtopstring;

			/* Dump the vt info. Note, if -S was specified or  */
			/* there were errors, dump the vt info into the    */
			/* assembler file to avoid creation of the vt temp */
			/* file.                                           */
			if (compiletoassembler || (r > 0))
			  {
			  dump_vt_to_dot_s(prntf,TRUE);
			  }
			else
                          {
                          vtopstring = dump_vt();
                          /* take off silly '\n' that is put on by dump_vt */
                          if (*(vtopstring + (strlen(vtopstring)-1)) == '\n')
                            *(vtopstring + (strlen(vtopstring)-1)) = '\0';
                          prntf("%s\n",vtopstring);
                          }
			prntf("\tlntt\n");
			while ((c=getc(ltmpfile))!=EOF)
				(void)putchar(c);
			prntf("\tgntt\n");
			while ((c=getc(dtmpfile))!=EOF)
				(void)putchar(c);
# if (defined (SA)) && (! defined(ANSI))
                        sa_macro_file_remove();
# endif
			}
#endif /* HPCDB */
			
#ifndef ONEPASS
	fixmaxlab(crslab-1);
	p2triple(FEOF,0,0);
	close(textfile);
#else   /* ONEPASS */
	put_flt_version_directive(); /* set object file "version" directive */
#endif  /* ONEPASS */

	return( r );
	}

#endif /* not IRIF */
#endif /*BRIDGE*/

#ifndef IRIF

static char *suffixw = ".w";
static char *suffixl = ".l";

#ifdef ONEPASS
genswitch(p,n, type) register n; register struct sw *p; TWORD type; {
	/*	p points to an array of structures, each consisting
		of a constant value and a label.
		The first is >=0 if there is a default label;
		its value is the label number
		The entries p[1] to p[n] are the nontrivial cases
		*/
	register i;
	register unsigned dlab, swlab;
	register char	*suffix;
	CONSZ j, range;
	flag	flagw;
    	unsigned int defjumplab, swlab1;

	j = p[1].sval;
	range = p[n].sval - j;

	switch (type)
		{
		case CHAR:
		case SCHAR:
		case UCHAR:
		case SHORT:
		case USHORT:	
			flagw = 1;
			suffix = suffixw;	/* no speed advantage for .b */
			break;
		
		default:
			flagw = 0;
			suffix = suffixl;
		}
	if( range>0 && range <= 3*n && n>=4 ) /* implement a direct switch */
		{

		dlab = p->slab >= 0 ? p->slab : GETLAB();
	    	defjumplab = brklab? brklab : GETLAB();
		if( j  )
			{
			prntf("\t%s%s\t&", ((j>=1)&&(j<=8))? "subq" : "sub",
				suffix );
			prntf( CONFMT2, j );
			prntf( ",%%d0\n" );
			}

		/* Note that the range depends on the case labels whereas
		   suffix depends on the type of the switch var.
		*/
		suffix = flagw ? suffixw : suffixl;
		prntf( "\tcmp%s\t%%d0,&0x%x\n", suffix, range );
		prntf( "\tbhi.l\tL%d\n", dlab );
		if (picoption)
			{
#ifdef OLDSWITCH
		  	prntf("\tlea.l\t(L%d,%%pc,%%za0),%%a0\n",
						swlab1=GETLAB());
		  	prntf("\tmov.l\t(0,%%a0,%%d0%s*4),%%d0\n",suffix);
			prntf("\tjmp\t%d(%%pc,%%d0.l)\n",2);
			deflab(swlab = GETLAB());
			prntf("\tlalign\t4\n");
			deflab(swlab1);
			for (i=1; i<=n; ++j)
				prntf("\tlong\tL%d-L%d\n", (j == p[i].sval) ?
					p[i++].slab : dlab, swlab);
#else
		  	prntf("\tmov.l\tL%d(%s),%%a0\n",swlab1=GETLAB(),
							rnames[picreg]);
		  	prntf("\tmov.l\t(%%a0,%%d0%s*4),%%a0\n",suffix);
		  	pic_used = YES;
		  	prntf("\tjmp\t(%%a0)\n");
	
		  	prntf("\tdata\n");
		  	deflab( swlab = GETLAB());
		  	prntf("\tlalign\t4\n", 4);

			/* output table */

		  	deflab( swlab1 );
		  	for( i=1; i<=n; ++j )
				{
				prntf( "\tlong\tL%d\n", ( j == p[i].sval ) ?
					p[i++].slab : dlab );
				}

		  	prntf("\ttext\n");

#endif /* OLDSWITCH */
			}
		else
		  	{
		  	prntf("\tmov.l\t(L%d,%%za0,%%d0%s*4),%%a0\n",
					swlab1=GETLAB(), suffix );
		  	prntf( "\tjmp\t(%%a0)\n" );
		  	deflab( swlab = GETLAB() );
		  	prntf( "\tlalign\t4\n", 4);

		  	/* output table */

		  	deflab( swlab1 );
		  	for( i=1; i<=n; ++j )
				{
				prntf( "\tlong\tL%d\n", ( j == p[i].sval ) ?
					p[i++].slab : dlab );
				}
		  	}
		if( !brklab ) deflab( defjumplab );
		return;
		}
	genbinary( p,1,n,0, flagw, type );
}

genbinary(p,lo,hi,lab, flagw, type)
  register struct sw *p;
  register int lo,hi;
  flag flagw;
  TWORD type;
  {	register int i,lab1, sival;

	if (lab) prntf("L%d:",lab);	/* print label, if any */
	if (hi-lo > 4) 			/* if lots more, do another level */
		{
	  	i = lo + ((hi-lo)>>1);	/* index where we'll break this time */
	  	sival = p[i].sval;
	  	prntf( "\tcmp%s\t",flagw? suffixw:suffixl );
	  	prntf( "%%d0,&0x%x\n\t%s\tL%d\n", sival, ISUNSIGNED(type)?
			"bhi.l" : "bgt.l", lab1=GETLAB() );
	  	prntf( "\tbeq.l\tL%d\n", p[i].slab );
	  	genbinary(p,lo,i-1,0,flagw,type);
	  	genbinary(p,i+1,hi,lab1,flagw,type);
		}
	else 			/* simple switch code for remaining cases */
		{
	  	for( i=lo; i<=hi; ++i )
			{
			sival = p[i].sval;
	    		prntf( "\tcmp%s\t", flagw? suffixw:suffixl );
	    		prntf( "%%d0,&0x%x\n\tbeq.l\tL%d\n", sival, p[i].slab );
	  		}
	  	if( p->slab>=0 ) branch( (unsigned)p->slab );
		}
}

#else /* ONEPASS */

#ifndef BRIDGE
genswitch(p,n, type) register n; register struct sw *p; TWORD type; {
	/*	p points to an array of structures, each consisting
		of a constant value and a label.
		The first is >=0 if there is a default label;
		its value is the label number
		The entries p[1] to p[n] are the nontrivial cases
		*/
	register int i;
	register dlab;
	CONSZ j, range;
	flag	flagw;

	j = p[1].sval;
	range = p[n].sval - j;
	dlab = p->slab >= 0 ? p->slab : GETLAB();

	switch (type)
		{
		case CHAR:
		case UCHAR:
		case SCHAR:
		case SHORT:
		case USHORT:	
			flagw = SWWORD; /* no speed advantage for .b */
			break;
		
		default:
			flagw = 0;
		}

	if (ISUNSIGNED(type)) flagw |= SWUNSIGN;
	if (p->slab >= 0) flagw |= SWDEFL;

	/* | # pairs | word flag | SWTCH  |
	 * |     range                    |  |       dlab            |
	 * |     case value 1             |  |     label 1           |
	 *      ...
	 */
	p2triple(SWTCH, flagw, n+1);   /* number of pairs includes range/dlab */
	p2word(range);	 p2word(dlab);
	for (i=1; i<=n; i++) {
		p2word(p[i].sval);  p2word(p[i].slab);
		}
}

#endif /* BRIDGE */

#endif /* ONEPASS */

#endif /* not IRIF */
