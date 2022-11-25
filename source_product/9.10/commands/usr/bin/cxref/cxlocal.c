/* @(#) $Revision: 70.1 $ */      

/* This file is a combination of Sys 5.2 xlocal.c and lint.c.
 * The xlocal.c portion consists of functions unique to cxref (ref, def, newf).
 * The lint.c-like portion is a "customized" version of local.c.  It contains
 * the 'main' procedure to parse arguments and open files;  and "stubs"/special
 * versions of just enough procedures to make the front-end of the compiler
 * run as an independent program.
 *
 * This file is linked with the frontend ccom files to build xpass.
 * Note that I have put an object code sccsid in this file, so the 'what'
 * identification of 'xpass' is two numbers: the version-id of ccom, and the
 * version-id of cxlocal.c
 *
 * This file is used for the s300 build only.
 */

# include <signal.h>
# include "mfile1"

FILE *outfp;
char infile[120];
extern char infile[];

/* Following needed for ANSI version and long double code in cgram.y */
# ifdef LDBL
int stroffset;
# endif

# ifdef DEBUGGING
extern flag ddebug, idebug, bdebug, tdebug, edebug, xdebug;
# endif

# define MINTABSZ 40

int blocknos[NSYMLEV];
int blockptr = 0;
int nextblock = 1;

int vflag = 1;  /* tell about unused argments */
int xflag = 0;  /* tell about unused externals */
int argflag = 0;  /* used to turn off complaints about arguments */
int libflag = 0;  /* used to generate library descriptions */
int vaflag = -1;  /* used to signal functions with a variable number of args */
int aflag = 0;  /* used th check precision of assignments */
int notdefflag = 0; /* notdef comment seen */
int notusedflag = 0; /* notused comment seen */
int stdlibflag = 0; /* stdlibrary */
int fcomment(){} 
extern bad_fp();


ecode( p ) NODE *p; {
	/* compile code for p */
	}

zecode( n ){
	/* n integer words of zeros */
	OFFSZ temp;
	temp = n;
	inoff += temp*SZINT;
	}

NODE *
clocal(p) NODE *p; {

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

	register o;
	register unsigned t, tl;

	switch( o = p->in.op ){

	case SCONV:
	case PCONV:
		if( p->in.left->in.type==ENUMTY ){
			p->in.left = pconvert( p->in.left );
			}
		/* assume conversion takes place; type is inherited */
		t = p->in.type;
		tl = p->in.left->in.type;
		if( aflag && (tl==LONG||tl==ULONG) && (t!=LONG&&t!=ULONG) ){
			werror( "long assignment may lose accuracy" );
			}
		if( aflag>=2 && (tl!=LONG&&tl!=ULONG) && (t==LONG||t==ULONG) && p->in.left->in.op != ICON ){
			werror( "assignment to long may sign-extend incorrectly" );
			}
		if( ISPTR(tl) && ISPTR(t) ){
			tl = DECREF(tl);
			t = DECREF(t);
			switch( ISFTN(t) + ISFTN(tl) ){

			case 0:  /* neither is a function pointer */
				if( talign(t,p->fn.csiz) > talign(tl,p->in.left->fn.csiz) ){
					if( hflag||pflag ) werror( "possible pointer alignment problem" );
					}
				break;

			case 1:
				werror( "questionable conversion of function pointer" );

			case 2:
				;
				}
			}
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion");
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	/* following cases were found to be necessary for the s200.
	 * ??? what other cases from the s200's clocal are really 
	 * necessary here?
	 */
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
	      { int m;
		m = p->in.type;
		if ( m < CHAR || m > ULONG || (m > LONG && m < UCHAR) )
			p = makety(p, INT, 0, INT);
		break;
	      }

		}

	return(p);
	}


	/* a number of dummy routines, unneeded by lint */

branch(n){;}
defalign(n){;}
deflab(n){;}
bycode(t,i){;}

cisreg(t) TWORD t; {return(1);}  /* everything is a register variable! */

main( argc, argv ) char *argv[]; {
	/* definitions for CXREF */
	char *p, *cp;
	int n = 1;

	/* set up signal trapping for floating point */
	signal(SIGFPE, bad_fp);

	/* handle options */

	for( p=argv[1]; n < argc && *p == '-'; p = argv[++n]){

		switch( *++p ){

		/* the next bunch of cases are for CXREF */
# ifdef DEBUGGING
		case 'd':
			++ddebug;
			continue;
		case 'I':
			++idebug;
			continue;
		case 'b':
			++bdebug;
			continue;
		case 't':
			++tdebug;
			continue;
		case 'e':
			++edebug;
			continue;
		case 'x':
			++xdebug;
			continue;
# endif

		case 'N':	/* table size resets */
			{
			char *lcp = ++p;
			int k = 0;
			while ( (*++p >= '0') && (*p <= '9') )
				k = 10*k + (*p - '0');
			if (k <= MINTABSZ)
				{
				/* simple-minded check */
				fprintf(stderr, "cxref:Table size specified too small (ignored)\n" );
				k = 0;
				}
			if (k) resettablesize(lcp, k);
			break;
			}

		case 'f':	/* filename sent to cpp */
			p = argv[++n];
			infile[0] = '"';	/* put quotes around name */
			cp = &infile[1];
			while (*cp++ = *p++) ;	/* copy filename */
			*--cp = '"';
			*++cp = '\0';
			continue;

		case 'n':	/* ccom option to enable NLS support */
			continue;
		case 'F':	/* ccom option for inlining */
			continue;

		case 'i':	/* actual input filename */
			p = argv[++n];
			if (freopen(p,"r",stdin) == NULL) {
				fprintf(stderr, "Can't open %s\n",p);
				exit(1);
			}
			continue;

		case 'o':
			p = argv[++n];
			if ((outfp = fopen(p,"a")) == NULL) {
				fprintf(stderr, "Can't Open %s\n",p);
				exit(2);
			}
			continue;

		default:
			werror( "illegal option: %c", *p );
			continue;

			}
		}


	n = mainp1(argc, argv);
	if (feof(stdout) || ferror(stdout))
		perror("cxref.xpass");
	return( n );
	}

/* the following functions occur in the Bell cxref lint.c file because
 * they are normally in local.c or code.c.
 * Define at least a shell for s200.
 */
ctype( type ) TWORD type; { /* map types which are not defined on the local machine */
	switch( BTYPE(type) ){
	case LONG:
		MODTYPE(type,INT);
		break;
	case ULONG:
		MODTYPE(type,UNSIGNED);
		}
	return( type );
	}

NODE *
offcon( off ) OFFSZ off; {
	register NODE *p;

	p = bcon(0, INT);
	p->tn.lval = off/SZCHAR;
	return(p);
}

cinit( p, sz ) NODE *p; {
	/* arrange for the initialization of p into a space of size sz */
	inoff += sz;
}

fldal( t ) unsigned t; {
	uerror( "illegal field type" );
	return( ALINT );
}



bbcode()	/* CXREF */
{
	/* code for beginning a new block */
	blocknos[blockptr] = nextblock++;
	fprintf(outfp, "B%d\t%05d\n", blocknos[blockptr], lineno);
	blockptr++;
}

becode()	/* CXREF */
{
	/* code for ending a block */
	if (--blockptr < 0)
		uerror("bad nesting");
	else
		fprintf( outfp, "E%d\t%05d\n", blocknos[blockptr], lineno);
}

/*
 * The following functions are taken from system 5.2 cxref/xlocal.c and 
 * are local to CXREF.  They put their output to outfp.
 * cgram.c has calls to these functions whenever a NAME is seen.
 */


ref( s, line)
	int  line;
	SYMLINK s;
{
	fprintf(outfp, "R%s\t%05d\n", s->sname, line);
}

/* refname:  like 'ref' except that it takes a pointer to the name
 *		instead of the symtab entry.
 */
refname( s, line)
	int  line;
	char * s;
{
	fprintf(outfp, "R%s\t%05d\n", s, line);
}

def( s, line )
	int  line;
	SYMLINK s;
{
	if (s->sclass == EXTERN)
		ref(s, line);
	else
		fprintf(outfp, "D%s\t%05d\n", s->sname, line);
}


newf(s, line)
	int  line;
	SYMLINK s;
{
	fprintf(outfp, "F%s\t%05d\n", s->sname, line);
}

/* Output the file name, in quotes (which should be a part of the *fp
 * string.
 */
cxftitle(ftp)
	char * ftp;
{
	if (*ftp != '"')
	   cerror("cxref: srcfile name <%s> : unexpected format", ftp);
	(void)fprintf(outfp, "%s\n", ftp);
}


