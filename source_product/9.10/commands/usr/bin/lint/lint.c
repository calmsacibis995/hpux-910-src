/* @(#) $Revision: 70.19 $ */    
/* lint.c
 *	This file contains the major functions for the first pass of lint
 *
 *	There are basically two types of functions in this file:
 *	functions called directly by the portable C compiler and other
 *	(lint) functions.
 *
 *	lint functions:
 *	===============
 *		astype	hashes a struct/union line number to ensure uniqueness
 *		ctargs	count arguments of a function
 *		fldcon	check assignment of a constant to a field
 *		fsave	write out a new filename to the intermediate file
 *		fcomment write out lint comment directive to intermediate file
 *		lmerge
 *		lprt
 *		lpta
 *		main	driver routine for the first pass (lint1)
 *		outdef	write info out to the intermediate file
 *		strip	strip a filename down to its basename
 *		where	print location of an error
 *
 *	pcc interface routines:
 *	=======================
 *		andable		returns 1 if a node can accept the & operator
 *		aocode		called when an automatic is removed from symbol table
 *		bfcode		emit code for beginning of function
 *		branch		dummy, unused by lint
 *		bycode		dummy, unused by lint
 *		cinit		
 *		cisreg		dummy for lint; returns 1
 *		clocal		do local transformations on the tree
 *		commdec		put out a common declaration
 *		ctype		dummy for lint
 *		defalign	dummy, unused by lint
 *		deflab		dummy, unused by lint
 *		defname		
 *		ecode		emit code for tree (lint processing for whole tree)
 *		efcode		emit code for end of function
 *		ejobcode	end of job processing
 *		exname		create external name
 *		fldal		check field alignment
 *		fldty		check field type
 *		isitfloat
 *		noinit		return storage class for uninitialized objects
 *		offcon		make structure offset node
 *		zecode		create arg integer words of zero
 */

# include <ctype.h>
# include <signal.h>
#ifndef PAXDEV
# include <locale.h>
#endif /* PAXDEV */
# include <string.h>
# include <sys/types.h>

# include "mfile1"
# include "messages.h"

# include "lerror.h"
# include "lmanifest"

# define VAL 0
# define EFF 1

#ifdef APEX
# include <malloc.h>
# include "apex.h"
# define DEF_OUT_SZ  512
# define OUTBUF_INC  512
char *outbuf;
size_t outbuf_sz;
char *outbuf_ptr;
struct std_defs *target_list, *origin_list;
char *target_file = "/usr/apex/lib/targets";
char *origin_file = "/usr/apex/lib/origins";
extern int opterr;

int apex_flag;
int domain_extensions = 1;
int all_ext_flag = 0;		/* warn on each occurrence of ref parms? */
#ifdef DOMAIN_LINT
int ignore_extensions = 1;	/* be silent about domain extensions */
#else
int ignore_extensions = 0;	
#endif
#endif 	/* APEX */

/*DTS#CLLbs00678*/
# define SAFETY_ZONE          64

#ifdef STATS_UDP
#define INST_UDP_PORT		42963
#define INST_UDP_ADDR		0x0f01780f 
#define INST_VERSION		1 
/* instrumentation for beta releases. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
/* udp packet definition */
struct udp_packet {
    int size;			/* size of the source file in bytes */
    char name[16];		/* basename of the file */
    unsigned long inode;	/* inode */
    int flag_ansi:1;		/* -Aa used */
    int flag_a:1;	/* -a used */
    int flag_b:1;	/* -b used */
    int flag_h:1;	/* -h used */
    int flag_p:1;	/* -p used */
    int flag_u:1;	/* -u used */
    int flag_v:1;	/* -v used */
    int flag_x:1;	/* -x used */
    int flag_y:1;	/* -y used */
    int flag_N:1;	/* -N used (table size reset) */
    int flag_Z:1;	/* -Z used (lint2 table size) */
    int flag_w:1;
    int dummy:16;
    int pass:2;		/* lint1 vs. lint2 */
    int version:2;		/* compiler version id */
};
int lint1size = 0;	/* see if any pass1 tables are expanded */
/* The following defines should be passed in, the examples below were used
 * in the prototype: -DINST_UDP_PORT=49963 -DINST_UDP_ADDR=0x0f01780d */
/* pick a udp port that is unlikely to be used elsewhere
#define INST_UDP_PORT		42963 */
/* internet address of hpfcpas 15.1.120.15
#define INST_UDP_ADDR		0x0f01780f */
/* compiler version id 
#define INST_VERSION		1 */
#endif /* STATS_UDP */

/* these are appropriate for the -p flag */
/* October 1980 -
 *	It would be nice if these values could be set so as to eliminate any
 *	possibility of machine dependency.  When we tried this by making
 *	the sizes relatively prime, several obscure problems were encountered.
 *	PCC apparently makes some sort of assumption(s) that can result in a
 *	compiler error when these sizes are used.
 */

int	SZCHAR	=	8;
int	SZSCHAR	=	8;
int	SZSHORT	=	16;
int	SZINT	=	32;
int	SZLONG	= 	32;
int	SZFLOAT	=	32;
int     SZLONGDOUBLE =  128;
int	SZDOUBLE=	64;
int	SZPOINT	=	16;
int	SZWIDE	=	16;
int	ALCHAR	=	8;
int	ALSHORT	=	16;
int	ALINT	=	16;
int	ALLONG	=	32;
int	ALFLOAT	=	32;
int	ALDOUBLE=	64;
int	ALLONGDOUBLE=	64;
int	ALPOINT	=	16;
int	ALSTRUCT=	16;
int	ALSTACK=	32;

int vflag = 1;  /* tell about unused argments */
/* 28 feb 80  reverse sense of xflag */
int xflag = 1;  /* tell about unused externals */
int argflag = 0;  /* used to turn off complaints about arguments */
int libflag = 0;  /* used to generate library descriptions */
int stdlibflag = 0;	/* used to generate library descriptions from protos */
int vaflag = -1;  /* used to signal functions with a variable number of args */
/* 28 feb 80  reverse sense of aflag */
int aflag = 1;  /* used to check precision of assignments */
int ansilint = 0; /* set if cpp.ansi is called */
int nullif = 0;	  /* warn about 'if ();' */
int notusedflag = 0; /* NOTUSED directive seen */
int notdefflag = 0; /* NOTDEFINED directive seen */
int stroffset = 0;	/* cgram.y expects code.c to provide this */

#ifdef LINTHACK
extern short nerrors;
#endif /* LINTHACK */
extern 	tmpopen( );
extern  void exit();
extern  pid_t getpid();
extern char *htmpname;
extern char *optarg;
extern int optind;

# define MAXSYMTSZ maxsymtsz

	char sourcename[ BUFSIZ + 1 ] = "";

# define LNAMES 250

struct lnm {
	struct symtab *lid; short flgs;
	}  lnames[LNAMES], *lnp;

/* contx - check context of node
 *	contx is called for each node during tree walk (fwalk);
 *	it complains about nodes that have null effect.
 *	VAL is passed to a child if that child's value is used
 *	EFF is passed to a child if that child is used in an effects context
 *
 *	arguments:
 *		p - node pointer
 *		down - value passed down from ancestor
 *		pl, pr - pointers to values to be passed down to descendants
 */
contx( p, down, pl, pr ) register NODE *p; register *pl, *pr;
{
	*pl = *pr = VAL;
	switch( p->in.op ){

		/* left side of ANDAND, OROR, and QUEST always evaluated for value
	 	   (value determines if right side is to be evaluated) */
		case ANDAND:
		case OROR:
		case QUEST:
			*pr = down; break;

                /* left side and right side treated identically */
                case SCONV:
                case PCONV:
                        *pr = *pl = down; break;

		/* be quiet about null effect if either side has side effect */
		case COLON:
			if (down==EFF && 
			    (haseffects(p->in.left)||haseffects(p->in.right) ) )	
				*pr = *pl = VAL;
			else
				*pr = *pl = down; 
			break;

		/* comma operator uses left side for effect */
		case COMOP:
			*pl = EFF;
			*pr = down;

		case FORCE:
		case INIT:
		case UNARY CALL:
		case STCALL:
		case UNARY STCALL:
		case CALL:
		case CBRANCH:
			break;

		default:
			/* assignment ops are OK */
			if( asgop(p->in.op) ) break;

			/* struct x f( );  main( ) {  (void) f( ); }
		 	 *  the cast call appears as U* UNDEF 
			 */
			if( p->in.op == UNARY MUL &&
				( p->in.type == STRTY
				|| p->in.type == UNIONTY
				|| p->in.type == VOID) )
				break;  /* the compiler does this... */

			/* found a null effect ... */
			if( down == EFF && hflag ) WERROR( MESSAGE( 86 ) );
	}
}




/* preserve - check for value-preserving/unsigned-preserving integral promotion
 *	unsigned char and unsigned short used to be promoted to unsigned int - 
 *	ANSI now specifies that they are promoted to int.
 *	Warn users of this silent change in cases where it matters:
 *	     1. An expression involving an unsigned char or unsigned short
 *		produces an int-wide result in which the sign bit is set:
 *		i.e., either a unary operation on such a type, or a binary
 *		operation in which the other operand is an int or "narrower"
 *		type.
 *	     2. The result of the preceeding expression is used in a context
 *		in which its signedness is significant:
 *		-it is the left operand of the right-shift operator, or
 *		-it is either operand of /, %, <, <=, >, or >=.
 *
 * Walk the tree and look for nodes where the conversion matters, warning
 * if that node's children had been implicitly converted (explicit casts
 * silence the warning).
 * lint's clocal removes SCONVs, forces the requested conversion, and
 * sets the promote_flg for value-preserving conversions.  The grammar
 * sets the new lint flag, cast_flg, on explicit casts.
 * This routine used to be invoked by fwalk, but that was looking too
 * far down the tree.  For example, we don't want to warn about an array
 * access whose index has been promoted.
 * 
 */

void preserve(p)
	NODE *p;
{

	/* check this node */
	switch( p->in.op ) {

		case DIV:
		case DIVP2:
		case MOD:
		case LT:
		case LE:
		case GT:
		case GE: 
			if (p->in.right->in.promote_flg && !p->in.right->in.cast_flg)
				WERROR( MESSAGE( 223 ) );
			/* fall through */
		case RS:
			if (p->in.left->in.promote_flg && !p->in.left->in.cast_flg)
				WERROR( MESSAGE( 223 ) );
			 break;

	}

	/* recursively descend the tree */
	switch (optype(p->in.op) ) {

		case BITYPE: preserve(p->in.left);
			     preserve(p->in.right);
			     break;

		case UTYPE: preserve(p->in.left);
			    break;
		}

}



/* ecode - compile code for node */
ecode( p ) NODE *p;
{
	fwalk( p, contx, EFF );	/* do a preorder tree walk */
	preserve(p); /* look for unsigned preserving */
	lnp = lnames;		/* initialize pointer to start of array */
	lprt( p, EFF, 0 );
	nullif = IFOK; 
}

/*ARGSUSED*/
ejobcode( errflag )
{	/* called after processing each job */
	/* flag is nonzero if errors were detected */
	register k;
	register struct symtab *p;
	extern		hdrclose( ),
			unbuffer( );

	for ( p=stab_lev_head[0]; p != NULL_SYMLINK; p = p->slev_link )
	    {

		if( p->stype != TNULL ) {

			if( p->stype == STRTY || p->stype == UNIONTY )
				if( dimtab[p->sizoff+1] < 0 )
					/* "struct/union %.8s never defined" */
					/* "struct/union %s never defined" */
					if( hflag ) WERROR( MESSAGE( 102 ), p->sname );

			if( p->sclass == STATIC && !(p->sflags&SNAMEREF) ){
				k = lineno;
				lineno = p->suse;
				/* "static %s unused" */
				WERROR( MESSAGE( 101 ), p->sname );
				lineno = k;
				}
		}
	}
	hdrclose( );
	if (notusedflag)
	    fcomment(0,0);	/* turn off NOTUSED flag */
	if (notdefflag)
	    fcomment(1,0);	/* turn off NOTDEFINED flag */
	unbuffer( );
#ifdef LINTHACK
	exit( nerrors );
#else
	exit( 0 );
#endif /* LINTHACK */
}
/* astype - hash a struct/union line number to ensure uniqueness */
/* This routine used to set t->extra = (stab[j].suse<<5) ^ dimtab[i];
 * Nice idea, but struct declarations that appear on different lines
 * in separate files produces "value used inconsistently" errors (FSDlq03119).
 */
astype( t, i ) ATYPE *t;
{ TWORD tt;
	int j, k=0;
	struct symtab *structnm;
	int complex;

	if( (tt=BTYPE(t->aty))==STRTY || tt==UNIONTY ) {
		if( i<0 || i>= maxdimtabsz-3 )
			uerror( "lint's little mind is blown" );
		else {
			j = dimtab[i+3];
			if( j <= 0 )
#ifdef APEX
				structnm = NULL;
#else
				k = ((-j)<<5)^dimtab[i]|1;
#endif
			else
			    {
			    structnm = (struct symtab *)dimtab[i+3];
			    if (structnm->sflags&SNAMEREF)
			        uerror("no line number for %s",structnm->sname);
#ifndef APEX
			    else
			        k = hash_namestring(structnm->sname);
#endif
			    }
			}
		
#ifdef APEX
                complex = check_complex(dimtab[i+1]);
                if (complex==FTYCOMPLEX)
                    t->extra |= ATTR_COMPLEX;
                else if (complex==FTYDCOMPLEX)
                    t->extra |= ATTR_DCOMPLEX;

                t->stcheck = compute_struct(dimtab[i+1], tt==UNIONTY);
                t->stname = ((structnm==NULL) ? (char *)NULL : structnm->sname );
#else
		t->extra = k;
#endif
		return( 1 );
	}
	else return( 0 );
}
/* bfcode - handle the beginning of a function */
/*ARGSUSED*/
bfcode( a, n ) 
struct symtab *a[];
{	/* code for the beginning of a function; a is an array of
		indices in stab for the arguments; n is the number */
	/* this must also set retlab */
int ph, fclass;
NODE *pn;
int ncheck;

	retlab = 1;
	ph = curftn->dimoff;
	pn = (NODE *)dimtab[ph];

	/* if variable number of arguments,
	  only print the ones which will be checked */
	if( vaflag >= 0 )
		if( n < vaflag )
		    {
			/* "declare the VARARGS arguments you want checked!" */
			WERROR( MESSAGE(224) ); 
			vaflag = -1;
		    }

	fsave( ftitle );
	fclass=(curftn->sclass==STATIC)?
		LDS : (libflag?LIB:LDI);
	ncheck = (vaflag >= 0) ? -(vaflag + 1) :
			((pn->ph.flags&SELLIPSIS) ? 
				-(pn->ph.nparam + 1) : pn->ph.nparam);
	outftn(curftn, fclass, ncheck, lineno);
	vaflag = -1;

}


outftn(p, fclass, ncheck, line)
struct symtab *p;
int fclass, ncheck, line;
{
NODE *phd;
struct symtab *fsym, *prev, *next;
static ATYPE t;
int argnum=0; 
#ifdef APEX
TWORD typesave;
char tag;
short altret;
short numargs;
#endif

	fsave( ftitle );
	phd = (NODE *)dimtab[p->dimoff];
	fclass |= (phd->ph.flags & SPARAM) ? LPR : 0;

	/* Chain down the param list, reversing the linked list as we go */
	prev = NULL_SYMLINK;
	fsym = phd->ph.phead;
	while (fsym != NULL_SYMLINK)
	    {
		next = fsym->slev_link;
		fsym->slev_link = prev;
		prev = fsym;
		fsym = next;
		argnum++;
	    }

        /* some syntax errors can produce a list a args shorter than nparam */
        if (ncheck < 0)
                {
                if (argnum < -(ncheck+1) )
                        ncheck = -(argnum+1);
                }
        else
                if (argnum < ncheck)
                        ncheck = argnum;

#ifdef APEX
        tag = rec_cnv(fclass);
        (void)fwrite((void *)&tag, sizeof(tag), 1, stdout);
        outbuf_ptr = outbuf + 2;
        memmove(outbuf_ptr, &line, 4);
        outbuf_ptr += 4;
        shroud(exname(p->sname));
        outtype(p);
        numargs = ncheck;       /* convert to short */
        memmove(outbuf_ptr, &numargs, 2);
        outbuf_ptr += 2;
        altret = 0;
        memmove(outbuf_ptr, &altret, 2); /* # alternate returns */
        outbuf_ptr += 2;
#else
	outdef(p, fclass, ncheck, line);
#endif	/* APEX */
	if (ncheck < 0) ncheck = -(ncheck + 1);

	/* prev points to the last node on the chain; 
	 * next_param makes sure it is a param */
	fsym = next_param(prev);	
	for (argnum=1; argnum<=ncheck; argnum++)
	    {
		t.aty = fsym->stype;
		/* left-shift the basic-type const/volatile field to leave
		 * room for the ICON bit (preserves backward compatibility
		 * with old .ln files)
		 */
		t.extra = ((fsym->sattrib& ~BTMASK) | 
			(fsym->sattrib&(ATTR_CON|ATTR_VOL))<<1 );
		if (!astype(&t, fsym->sizoff) && !(phd->ph.flags&SPARAM) )
		    /* promote argument types on old-style def's */
		    switch( t.aty ){

			case ULONG:
				break;

			case CHAR:
			case SHORT:
			case SCHAR:
				t.aty = INT;
				break;

			case UCHAR:
			case USHORT:
				/* Changed from UNSIGNED to INT 11/03/88
				 * since the actual args of this type
				 * are promoted to INT.  Spurious
				 * warnings were produced with matched
				 * unsigned short formal and actual args
				 * Furthermore, this matches ANSI
				 * value-preserving rules.
				 */
				t.aty = INT;
				break;

			case UNSIGNED:
				t.aty = UNSIGNED;
				break;

			case FLOAT:
				    t.aty = DOUBLE;
				break;
				
			}

#ifdef APEX
                typesave = fsym->stype;
                fsym->stype = t.aty;
                outtype(fsym);
                fsym->stype = typesave;
                shroud(NULL);   /* format string */
#else
		(void)fwrite( (void *)&t.aty, sizeof(t.aty), 1, stdout );
		(void)fwrite( (void *)&t.extra, sizeof(t.extra), 1, stdout );
#endif

		fsym = next_param(fsym->slev_link);
		}

	/* restore the linked list to its proper order */
	fsym = prev;
	prev = NULL_SYMLINK;
	while (fsym != NULL_SYMLINK)
	    {
		next = fsym->slev_link;
		fsym->slev_link = prev;
		prev = fsym;
		fsym = next;
	    }
#ifdef APEX
        *(short *)outbuf = (outbuf_ptr - outbuf - 2);   /* don't include cnt */
        (void)fwrite((void *)outbuf, (outbuf_ptr - outbuf), 1, stdout);
#endif

}


/* ctargs - count arguments; p points to at least one */
ctargs( p ) NODE *p;
{
	/* the arguments are a tower of commas to the left */
	register c;
	c = 1; /* count the rhs */
	while( p->in.op == CM ){
		++c;
		p = p->in.left;
		}
	return( c );
}
/* lpta */
lpta( p, promote ) NODE *p; int promote;
{
	static ATYPE t;
#ifdef APEX
        TWORD mods;
#endif

	if( p->in.op == CM ){
		lpta( p->in.left,promote );
		p = p->in.right;
		}

	t.aty = p->in.type;
	t.extra = p->in.tattrib;
	t.extra = ((p->in.tattrib & ~BTMASK) | 
		(p->in.tattrib&(ATTR_CON|ATTR_VOL))<<1 );
	t.extra |= (p->in.op==ICON) ? ATTR_ICON : 0;

#ifdef APEX
        mods = t.aty & ~BTMASK;
        t.dimptr = 0;
        while (mods) {
            if (ISARY(mods)) {
                t.dimptr = (int *)p->fn.cdim;
                break;
            }
            mods = DECREF(mods);
        }
#endif

	if( !astype( &t, p->fn.csiz ) && promote )
		switch( t.aty ){

			case CHAR:
			case SHORT:
				t.aty = INT;
			case LONG:
			case ULONG:
			case INT:
			case UNSIGNED:
				break;

			case UCHAR:
			case USHORT:
				/* used to be UNSIGNED - should match bfcode */
				t.aty = INT;
				break;

			case FLOAT:
				/* old-style float args cause explicit SCONV
				 * to double; new-style should stay float,
				 * so remove the following stmt
				 * t.aty = DOUBLE; */
				t.extra &= ~ATTR_ICON;
				break;

			default:
				t.extra &= ~ATTR_ICON;
				break;
		}
#ifdef APEX
        outatype(t);
#else
	(void)fwrite( (void *)&t.aty, sizeof(t.aty), 1, stdout );
	(void)fwrite( (void *)&t.extra, sizeof(t.extra), 1, stdout );
#endif
}

# define VALSET 1
# define VALUSED 2
# define VALASGOP 4
# define VALADDR 8

lprt( p, down, uses ) register NODE *p;
{
	register struct symtab *q;
	register id;
	register acount;
	register down1, down2;
	register use1, use2;
	register struct lnm *np1, *np2;

	/* first, set variables which are set... */
	use1 = use2 = VALUSED;
	if( p->in.op == ASSIGN ) use1 = VALSET;
	else if( p->in.op == UNARY AND ) use1 = VALADDR;
	else if( asgop( p->in.op ) ) { /* =ops */
		use1 = VALUSED|VALSET;
		if( down == EFF ) use1 |= VALASGOP;
		}


	/* print the lines for lint */

	down2 = down1 = VAL;
	acount = 0;

	switch( p->in.op ){

	case EQ:
	case NE:
		if( ISUNSIGNED(p->in.left->in.type) &&
		    p->in.right->in.op == ICON && p->in.right->tn.lval < 0 &&
		    p->in.right->tn.rval == NONAME &&
		    !ISUNSIGNED(p->in.right->in.type) 
			)
			/* "comparison of unsigned with negative constant" */
			WERROR( MESSAGE( 21 ) );
		goto charchk;

	case GT:
	case GE:
	case LT:
	case LE:
		/* 26 Oct 89 Warn only with pflag because 
		 *	plain char == signed char on HP machines.
		 */
		if (pflag  &&
		    p->in.left->in.type == CHAR && 
		    p->in.right->in.op == ICON &&
		    p->in.right->tn.lval == 0)
			/* "nonportable character comparison" */
			WERROR(MESSAGE(82));
	charchk:
		if (pflag && 
		    p->in.left->in.op == NAME && p->in.left->nn.sym &&
		   p->in.left->nn.sym->stype == CHAR && p->in.right->in.op==ICON &&
		   p->in.right->tn.rval == NONAME && p->in.right->tn.lval < 0)
			/* "nonportable character comparison" */
			WERROR( MESSAGE( 82 ) );

		break;

	case UGE:
	case ULT:
		if( p->in.right->in.op == ICON && p->in.right->tn.lval == 0
		    && p->in.right->tn.rval == NONAME ) {
			/* "degenerate unsigned comparison" */
			WERROR( MESSAGE( 30 ) );
			break;
			}

	case UGT:
	case ULE:
	    if ( p->in.right->in.op == ICON && p->in.right->tn.rval == NONAME
		 && !ISUNSIGNED( p->in.right->in.type ) ) {
			if( p->in.right->tn.lval < 0 ) 
				/* "comparison of unsigned with negative constant" */
				WERROR( MESSAGE( 21 ) );

			if ( p->in.right->tn.lval == 0 )
				/* "unsigned comparison with 0?" */
				WERROR( MESSAGE( 115 ) );
	    }
	    break;

	case COMOP:
		down1 = EFF;

	case ANDAND:
	case OROR:
	case QUEST:
		down2 = down;
		/* go recursively left, then right  */
		np1 = lnp;
		lprt( p->in.left, down1, use1 );
		np2 = lnp;
		lprt( p->in.right, down2, use2 );
		lmerge( np1, np2, 0 );
		return;

	case SCONV:
	case PCONV:
	case COLON:
		down1 = down2 = down;
		break;

	case CALL:
	case STCALL:
		acount = ctargs( p->in.right );
	case UNARY CALL:
	case UNARY STCALL:
		{
		NODE * pn;
		if( p->in.left->in.op == ICON && (id=p->in.left->tn.rval) != NONAME ){
		  /* used to be &name */
			struct symtab *sp = p->in.left->nn.sym;
			int lty;
			/*  if a function used in an effects context is
			 *  cast to type  void  then consider its value
			 *  to have been disposed of properly
			 *  thus a call of type  undef  in an effects
			 *  context is construed to be used in a value
			 *  context
			 */
			if ((down == EFF) && (p->in.type != VOID)) lty = LUE;
			else if (down == EFF) lty = LUV | LUE;
			else lty = LUV;

			pn = (NODE *)dimtab[sp->dimoff];
			if (pn->ph.flags & SPARAM) 
				lty |= LPR;

#ifdef APEX
                        outbuf_ptr = outbuf;
                        outbase(sp, lty, acount, lineno);
#else
			outdef(sp, lty, acount, lineno);
#endif
			if( acount )
				lpta( p->in.right,(pn->ph.flags&SPARAM)==0 );
#ifdef APEX
                        *(short *)outbuf = (outbuf_ptr - outbuf - 2);
                        (void)fwrite( (void *)outbuf, (outbuf_ptr - outbuf), 1,
stdout );
#endif
			}
		}
		break;

	case ICON:
		/* look for &name case */
		if( (id = p->tn.rval) >= 0 && id != NONAME ){
			q = p->nn.sym;
			q->sflags |= (SREF|SSET);
		}
		return;

	case NAME:
		if( (id = p->tn.rval) >= 0 && id != NONAME ){
			q = p->nn.sym;
			if( (uses&VALUSED) && !(q->sflags&SSET) ){
				if( q->sclass == AUTO || q->sclass == REGISTER ){
				    if( !ISARY(q->stype ) && !ISFTN(q->stype)
				      && q->stype!=STRTY && q->stype!=UNIONTY){
						/* "%.8s may be used before set" */
						/* "%s may be used before set" */
						WERROR( MESSAGE( 1 ), q->sname );
						q->sflags |= SSET;
					}
				}
			}
			if( !(uses & VALASGOP) ) /* ASGOP not real uses */
			    {
				if( uses & VALSET ) q->sflags |= SSET;
				if( uses & VALUSED ) q->sflags |= SREF;
				if( uses & VALADDR ) q->sflags |= (SREF|SSET);
			    }
			if( p->tn.lval == 0 ){
				lnp->lid = q;
				lnp->flgs = (uses&VALADDR)?0:((uses&VALSET)?VALSET:VALUSED);
				if( ++lnp >= &lnames[LNAMES] ) --lnp;
			}
		}
		return;
	}

	/* recurse, going down the right side first if we can */

	switch( optype(p->in.op) ){

	case BITYPE:
		np1 = lnp;
		lprt( p->in.right, down2, use2 );
	case UTYPE:
		np2 = lnp;
		lprt( p->in.left, down1, use1 );
	}

	if( optype(p->in.op) == BITYPE ){
		if( p->in.op == ASSIGN && p->in.left->in.op == NAME )
		  /* special case for a =  .. a .. */
			lmerge( np1, np2, 0 );
		
		else if (asgop(p->in.op) && p->in.left->in.op==NAME )
		  /* special case for a += (a=1); */
			lmerge( np1, np2, 2 );

		else lmerge( np1, np2, p->in.op != COLON );
		/* look for assignments to fields, and complain */
		if( p->in.op == ASSIGN && p->in.left->in.op == FLD
		  && p->in.right->in.op == ICON ) fldcon( p );
		}
}
/* lmerge */
lmerge( np1, np2, lflag ) struct lnm *np1, *np2;
{
	/* np1 and np2 point to lists of lnm members, for the two sides
	 * of a binary operator
	 * lflag is 1 if commutation is possible
	 *         2 if ASGOP,
	 *	   0 otherwise
	 * lmerge returns a merged list, starting at np1, resetting lnp
	 * it also complains, if appropriate, about side effects
	 */

	register struct lnm *npx, *npy;

	for( npx = np2; npx < lnp; ++npx ){

		/* is it already there? */
		for( npy = np1; npy < np2; ++npy ){
			if( npx->lid == npy->lid ){ /* yes */
				if( npx->flgs == 0 || npx->flgs == (VALSET|VALUSED) )
					;  /* do nothing */
				else
					if( ( (npx->flgs|npy->flgs)== (VALSET|VALUSED) && (lflag!=2) )
					  || (npx->flgs&npy->flgs&VALSET) )
						/* "%.8s evaluation order undefined" */
						/* "%s evaluation order undefined" */
						if( lflag ) WERROR( MESSAGE( 0 ), npy->lid->sname );

				if( npy->flgs == 0 ) npx->flgs = 0;
				else npy->flgs |= npx->flgs;
				goto foundit;
			}
		}

		/* not there: update entry */
		np2->lid = npx->lid;
		np2->flgs = npx->flgs;
		++np2;

		foundit: ;
	}

	/* all finished: merged list is at np1 */
	lnp = np2;
}
/* efcode - handle end of a function */
efcode()
{
	/* code for the end of a function */
	register struct symtab *cfp;

	cfp = curftn;
	if( retstat & RETVAL ) outdef( cfp, LRV, 0, lineno );
	if( !vflag ){
		vflag = argflag;
		argflag = 0;
	}
	if( retstat == RETVAL+NRETVAL )
		/* "function %.8s has return(e); and return;" */
		/* "function %s has return(e); and return;" */
		WERROR( MESSAGE( 43 ), cfp->sname);
	/*
	* See if main() falls off its end or has just a return;
	*/
	if (!strcmp(cfp->sname, "main") && (reached || (retstat & NRETVAL)))
		/* "main() returns random value to invocation environment" */
		WERROR(MESSAGE(127));
#ifdef APEX
        outdef(cfp, LFE, 0, lineno);
#endif
}
/* aocode - called when automatic p is removed from stab */
aocode(p) struct symtab *p;
{
	register struct symtab *cfs;
	cfs = curftn;
	if(!(p->sflags&SNAMEREF) && !(p->sflags&(SMOS|STAG)) ){
	    if( (p->sclass==PARAM) || (p->sclass==REGISTER && p->slevel==1) ) {
			/* "argument %.8s unused in function %.8s" */
			/* "argument %s unused in function %s" */
			if( vflag ) WERROR( MESSAGE( 13 ), p->sname, cfs->sname );
	    }
	    else
			/* "%.8s unused in function %.8s" */
			/* "%s unused in function %s" */
			if( p->sclass != TYPEDEF ) WERROR( MESSAGE( 6 ),
			  p->sname, cfs->sname );
	    }

	if((p->sflags&SNAMEREF) && (p->sflags & (SSET|SREF|SMOS)) == SSET
	  && !ISARY(p->stype) && !ISFTN(p->stype) )
		/* "%.8s set but not used in function %.8s" */
		/* "%s set but not used in function %s" */
		WERROR( MESSAGE( 3 ), p->sname, cfs->sname );

	if( p->stype == STRTY || p->stype == UNIONTY || p->stype == ENUMTY )
		/* "structure %.8s never defined" */
		/* "structure %s never defined" */
		if( dimtab[p->sizoff+1] < 0 ) WERROR( MESSAGE( 104 ), p->sname );
}

/* defname - define the current location as the name p->sname */
defnam( p ) register struct symtab *p;
{
	if( p->sclass == STATIC && p->slevel>1 ) return;

	if( !ISFTN( p->stype ) )
		outdef( p, p->sclass==STATIC? LDS : (libflag?LIB:LDI), 0, lineno );
}

/* zecode - n integer words of zeros */
zecode( n )
{
	OFFSZ temp;
	temp = n;
	inoff += temp*SZINT;
}

/* clocal - do local checking on tree
 *	(pcc uses this routine to do local tree rewriting)
 */
NODE *
clocal(p) NODE *p;
{
	register o;
	register unsigned t, tl;

	switch( o = p->in.op ){

	case SCONV:
	case PCONV:
		if( p->in.left->in.type==ENUMTY )
			p->in.left = pconvert( p->in.left );

		/* assume conversion takes place; type is inherited */
		t = p->in.type;
		tl = p->in.left->in.type;
/* for the future: put aflag in a place where NAME is available; that is, check
 * assignment and arithmetic operators for leftchild NAME and rightchild SCONV
 * so that when message is printed it is possible to name the offending lval
 * note that the lval may be a temporary (NONAME)
 */
		/* Remember value-preserving conversions (ANSI silent change */
		if (t==INT && (tl==UCHAR || tl==USHORT)) 
			p->in.left->in.promote_flg = 1;

		/* 29 may 80  complain about loss of accuracy with long rhs
		 * only if longs are larger than ints
		 * also, no error if lhs is FLOAT or DOUBLE
		 * 21 sep 89  considered complaining about any integral 
		 * truncation, not just from long, but too many valid
		 * expressions are promoted to int; e.g. c = a?3:5;
		 */
		if ( aflag && !p->in.cast_flg && (tl==LONG || tl==ULONG) )
		    if ( ((t==INT || t==UNSIGNED) && (SZLONG>SZINT)) ||
			 (t==CHAR || t==SHORT || t==UCHAR || t==USHORT) &&
			p->in.left->in.op != ICON )
			/* "conversion from long may lose accuracy" */
			WERROR( MESSAGE( 26 ) );

		if( aflag && !p->in.cast_flg && pflag &&
			  (t==LONG || t==ULONG) &&
			  (tl!=LONG && tl!=ULONG && tl!=FLOAT && tl!=DOUBLE) &&
			  p->in.left->in.op != ICON )
			/* conversion to long may sign-extend incorrectly */
			WERROR( MESSAGE( 27 ) );

		if( ISPTR(tl) && ISPTR(t) ){
			tl = DECREF(tl);
			t = DECREF(t);
			switch( ISFTN(t) + ISFTN(tl) ){

			case 0:  /* neither is a function pointer */
				if( tl!=VOID && t!=VOID && 
				    talign(t,p->fn.csiz,1) > talign(tl,p->in.left->fn.csiz,1) )
					/* "possible pointer alignment problem" */
#ifdef __hp9000s300
					/* no pointer align problems on 68k */
					if( pflag||sflag ) WERROR( MESSAGE( 91 ) );
#else
					if( hflag||pflag||sflag ) WERROR( MESSAGE( 91 ) );
#endif
				break;

			case 1:
				/* "questionable conversion of function pointer" */
				WERROR( MESSAGE( 95 ) );

			case 2:
				;
			}
		}
		p->in.left->in.type = p->in.type;
		p->in.left->in.tattrib = p->in.tattrib;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) 
#ifdef BBA_COMPILE
#pragma	    		BBA_IGNORE
#endif
			cerror( "bad conversion");
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	/* These comparisons copied from local.c 11/02/88
	 * Fix up the result type on pointer comparisons, e.g. (p1==p2)
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
		{ register TWORD m;
		/* if not a scalar type, convert */
		m = p->in.type;
		if ( m < CHAR || m > ULONG || (m > LONG && m < UCHAR) )
			p = makety(p, INT, 0, INT, 0);
		break;
		}
	}
	return(p);
}
/* offcon - make structure offset node */
NODE *
offcon( off ) OFFSZ off; 
{
	register NODE *p;
	p = bcon(0, 0);
	p->tn.lval = off/SZCHAR;
	return(p);
}


/* cinit - initialize p into size sz */
cinit( p, sz ) NODE *p;
{
	inoff += sz;
	if( p->in.op == INIT ){
		if( p->in.left->in.op == ICON ) return;
		if( p->in.left->in.op == NAME && p->in.left->in.type == MOE ) return;
	}
	/* "illegal initialization" */
	UERROR( MESSAGE( 61 ) );
}

/* exname - make a name look like an external name
 *		if checking for portability or if running on gcos or ibm,
 *		truncate name to six characters
 */
char *
exname( p ) char *p;
{
	static char aa[8];
	register int i;

	if( !pflag ) return(p);
	for( i=0; i<6; ++i ){
		if( isupper(*p ) ) aa[i] = tolower( *p );
		else aa[i] = *p;
		if( *p ) ++p;
	}
	aa[6] = '\0';
	return( aa );
}
/* strip - strip full name to get basename */
char *
strip(s) char *s;
{
	static char x[BUFSIZ+1];
	register char *p;

	for( p=x; *s; ++s ){
		if( *s == '/' ) p=x;
		else if( *s != '"' ) *p++ = *s;

		if ( p > &x[BUFSIZ] ) {
			/* 5 feb 80:  simulate a call to cerror( ) */
			(void)fprntf(stderr, "%s: compiler error: filename too long\n", x);
			exit(1);
			/* cannot call cerror( )
			 * because cerror( ) calls where( ) and where( ) calls
			 * strip( ) and this is strip
			 */
		}
	}
	*p = '\0';
	return( x );
}

/* fcomment - save lint comment on intermediate file
   Currently uses the following simple encoding for OSF comments:
   	NOTUSED		decflag = LND
	NOTDEFINED	decflag = LNU
   nargs = 0 means turn the comment off
   nargs = 1 means turn the comment on
 */
fcomment( type, state ) int type,state; { /* 0 = NOTUSED, 1 = NOTDEFINED */
#ifdef APEX
char tag;
short len;
char state_char;

	tag = (type) ? REC_NOTDEFINED : REC_NOTUSED;
	(void) fwrite((void *)&tag,1,1,stdout);
	len = 1;	/* 1 byte follows */
	(void) fwrite((void *)&len,2,1,stdout);
	state_char = state;
	(void) fwrite((void *)&state_char,1,1,stdout);
#else
	static union rec comment;
	comment.l.decflag = (type)?LND:LNU;
	comment.l.nargs = state;
	(void) fwrite((void *)&comment.l.decflag,sizeof(comment.l.decflag),1,stdout);
	(void) fwrite((void *)&comment.l.name,sizeof(comment.l.name),1,stdout);
	(void) fwrite((void *)&comment.l.nargs,sizeof(comment.l.nargs),1,stdout);
	(void) fwrite((void *)&comment.l.fline,sizeof(comment.l.fline),1,stdout);
	(void) fwrite((void *)&comment.l.type.aty,sizeof(comment.l.type.aty),1,stdout);
	(void) fwrite((void *)&comment.l.type.extra,sizeof(comment.l.type.extra),1,stdout);
#endif
}

/* fsave - save file name on intermediate file */
fsave( s ) char *s; {
	static char buf[BUFSIZ+1];
#ifdef APEX
        char tag;
        int version;
        short rec_len;
        short name_len;
#else
	static union rec fsname;
#endif

	s = strip( s );
	if( strcmp( s, buf ) ){
		/* new one */
		(void)strcpy( buf, s );
#ifdef APEX
                tag = REC_CFILE;
                (void)fwrite( (void *)&tag, 1, 1, stdout);
                name_len = strlen(buf);
                rec_len = 1 + 2 + name_len;   /* version + string_len + name */
                (void)fwrite( (void *)&rec_len, 2, 1, stdout);
                version = 0;
                (void)fwrite( (void *)&version, 1, 1, stdout);
                outbuf_ptr = outbuf;
                (void)shroud(buf);
                (void)fwrite( (void *)outbuf, outbuf_ptr - outbuf, 1, stdout);
#else
		fsname.f.decflag = LFN;
		fsname.f.mno = getpid();
		/* write out the union rec one element at a time.  This
		 * code used to fwrite the entire structure but was
		 * changed to produce a s300/s800 portable structure 
		 * (void)fwrite( (void *)&fsname, sizeof(fsname), 1, stdout );
		 */
		(void)fwrite( (void *)&fsname.l.decflag, sizeof(fsname.l.decflag), 1, stdout );
		(void)fwrite( (void *)&fsname.l.name, sizeof(fsname.l.name), 1, stdout );
		(void)fwrite( (void *)&fsname.l.nargs, sizeof(fsname.l.nargs), 1, stdout );
		(void)fwrite( (void *)&fsname.l.fline, sizeof(fsname.l.fline), 1, stdout );
		(void)fwrite( (void *)&fsname.l.type.aty, sizeof(fsname.l.type.aty), 1, stdout );
		(void)fwrite( (void *)&fsname.l.type.extra, sizeof(fsname.l.type.extra), 1, stdout );
		(void)fwrite( (void *)buf, strlen( buf ) + 1, 1, stdout );
#endif
	}
}

/* where  - print the location of an error
 *  if the filename is a C file (the source file) then just print the lineno
 *    the filename is taken care of in a title
 *  if the file is a header file ( unlikely but possible)
 *    then the filename is printed with the line number of the error
 *    where is called by cerror, uerror and werror
 *    (it is not called by luerror or lwerror)
 */
extern enum boolean	iscfile( );
/*ARGSUSED*/
where( f ) char	f;
{
    extern char		*strip( );
    char		*filename;

    /* used to decrement nerrors to prevent "too many errors"
     * cases have been reported in which too many errors does sufficiently
     * confuse lint
     */

    filename = strip( ftitle );
    if ( iscfile( filename ) == true )
		(void)fprntf( stderr, "(%d)  ", lineno );
    else
		(void)fprntf( stderr, "%s(%d): ", filename, lineno );
}
/* beg_file() 
 * Used to do stuff to handle floating point exceptions
 * generated during floating point constant folding
 */
extern void bad_fp();
extern void dexit();
beg_file()
{

	/*
	 * Catch floating exceptions generated by the constant
	 * folding code.
	 */
	(void) signal( SIGFPE, bad_fp );
	if(signal( SIGHUP, SIG_IGN) != SIG_IGN) (void)signal(SIGHUP, dexit);
	if(signal( SIGINT, SIG_IGN) != SIG_IGN) (void)signal(SIGINT, dexit);
	if(signal( SIGTERM, SIG_IGN) != SIG_IGN) (void)signal(SIGTERM, dexit);
}
/* a number of dummy routines, unneeded by lint */

/*ARGSUSED*/
branch(n) unsigned n; {;}
/*ARGSUSED*/
deflab(n) unsigned n; {;}
/*ARGSUSED*/
bycode(t,i,type,done){;}
/*ARGSUSED*/
cisreg(t) TWORD t; {return(1);}  /* everything is a register variable! */

/* fldty - check field type (called from pcc) */
fldty(p) struct symtab *p;
{
	/* for a field to be portable, it must be unsigned int */
	/* "the only portable field type is unsigned int" */
	if( pflag && ((BTYPE(p->stype) != UNSIGNED) ||
		      (BTYPE(p->stype) != INT)) )
		WERROR( MESSAGE( 83 ) );
}

/* fldal - field alignment (called from pcc) 
 *		called for alignment of funny types (e.g. arrays, pointers) 
 */
fldal(t) unsigned t;
{
	if( t == ENUMTY )	/* this should be thought through better... */
		return( ALINT );/* jwf try this */
	/* "illegal field type" */
	UERROR( MESSAGE( 57 ) );
	return(ALINT);
}
/* main - driver for the first pass */
main( argc, argv ) int	argc; char	*argv[ ];
{
    char	 *p;
    int		i;
    char 	stdbuf[BUFSIZ];	/*stderr output buffer*/
#ifdef APEX
    int		apex_warn = WOBSOLETE|WALWAYS;
    int		argnum;
    int		format_magic;
#endif

#if !defined(PAXDEV) && !defined(_LOCALE_INCLUDED)
	if (!setlocale(LC_ALL,"")) {
#if !defined(OSF) && !defined(DOMAIN)
		werror(_errlocale(""));
#endif /* !OSF && !DOMAIN */
	}
#endif /* PAXDEV */

	beg_file();
#ifndef LINTHACK
	setbuf(stderr, stdbuf);
#endif /* LINTHACK */
    /* handle options */

    /* 28 feb 80  reverse the sense of hflag and cflag */
    hflag = 1;
	/*
    cflag = 1;
		9/25/80	undo the (28 feb 80) change to cflag */
    /* 31 mar 80  reverse the sense of brkflag */
    brkflag = 1;
#ifdef OSF
	while((i=getopt(argc,argv,"abcfhnpr:suvw:xyYAH:L:M:N:O:PS:T:X:Z:")) != EOF)
#else
#ifdef APEX
        while((i=getopt(argc,argv,"abcfhnpsuvw:xyYAH:LN:O:PS:T:X:")) != EOF)
#else
	while((i=getopt(argc,argv,"abchnpsuvxyYAH:LN:T:X:w:")) != EOF)
#endif
#endif
		switch(i) {
		case 'a':
			aflag = 0;
			break;

		case 'b':
			brkflag = 0;
			break;

		case 'c':
#ifdef BBA_COMPILE
#pragma	    		BBA_IGNORE
#endif
			(void)fprntf(stderr,"lint: -c option ignored - no longer available\n");
			break;

#ifdef APEX
                case 'f':
                        fileflag = true;
                        break;
#endif
		case 'h':
			hflag = 0;
			break;

		case 'p':
			pflag = 1;
			cflag = 1;	/* added 9/25/80 */
			break;

		case 's':
			sflag = 1;
			break;

		case 'v':
			vflag = 0;
			break;

		case 'x':
			xflag = 0;
			break;

		case 'H':
			htmpname = optarg;
			break;

		case 'A':
			ansilint = 1;
			break;
#ifdef OSF
		case 'L':	/* for creating libraries named "optarg" */
			freopen(optarg,"w+",stdout);
			htmpname = "/dev/null";
			break;
		case 'M':
			ansilint = 1;
			break;
#else
		case 'L':		/*undocumented; make input look like library*/
#ifdef BBA_COMPILE
#pragma	    		BBA_IGNORE
#endif
			libflag = 1;
			vflag = 0;
			break;
#endif
# define MINTABSZ 40
		case 'N':	/* table size resets */
			{
			char *nptr = optarg;
			int k = 0;
# ifdef STATS_UDP
			lint1size = 1;
# endif
			while ( (*++nptr >= '0') && (*nptr <= '9') )
				k = 10*k + (*nptr - '0');
			if (k <= MINTABSZ)
				{
				/* simple-minded check */
				(void)fprntf(stderr, "lint:Table size specified too small (ignored)\n" );
				k = 0;
				}
			if (k) resettablesize(optarg, k);
			break;
			}
		case 'w':
			{
			int k;
			char *nptr = optarg;
			for (nptr = optarg;*nptr;nptr++){
			    switch(*nptr){
			      case 'A':
				warnmask = ~warnmask & WALLMSGS;
				apex_warn = WALLMSGS;
				break;
			      case 'a':
				warnmask ^= WANSI;
				apex_warn ^= WANSI;
				break;
			      case 'c':
				warnmask ^= WUCOMPARE;
				apex_warn ^= WUCOMPARE;
				break;
			      case 'd':
				warnmask ^= WDECLARE;
				apex_warn ^= WDECLARE;
				break;
			      case 'h':
				warnmask ^= WHEURISTIC;
				apex_warn ^= WHEURISTIC;
				break;
			      case 'k':
				warnmask ^= WKNR;
				apex_warn ^= WKNR;
				break;
			      case 'l':
				warnmask ^= WLONGASSIGN;
				apex_warn ^= WLONGASSIGN;
				break;
			      case 'n':
				warnmask ^= WNULLEFF;
				apex_warn ^= WNULLEFF;
				break;
			      case 'o':
				warnmask ^= WEORDER;
				apex_warn ^= WEORDER;
				break;
			      case 'p':
				warnmask ^= WPORTABLE;
				apex_warn ^= WPORTABLE;
				break;
			      case 'r':
				warnmask ^= WRETURN;
				apex_warn ^= WRETURN;
				break;
			      case 's':
				break;	/* lint2 option */
			      case 'u':
				warnmask ^= WUSAGE;
				apex_warn ^= WUSAGE;
				break;
#ifdef APEX
			      case 'x':	
				ignore_extensions = 1;
				break;
#endif
			      case 'C':
				warnmask ^= WCONSTANT;
				apex_warn ^= WCONSTANT;
				break;
			      case 'D':
				warnmask ^= WUDECLARE;
				apex_warn ^= WUDECLARE;
				break;
			      case 'H':
			      case 'L':
			      case 'N':
				break;	/* lint2 options */
			      case 'O':
				warnmask ^= WOBSOLETE;
				apex_warn ^= WOBSOLETE;
				break;
			      case 'P':
				warnmask ^= WPROTO;
				apex_warn ^= WPROTO;
				break;
			      case 'R':
				warnmask ^= WREACHED;
				apex_warn ^= WREACHED;
				break;
			      case 'S':
				warnmask ^= WSTORAGE;
				apex_warn ^= WSTORAGE;
				break;
#ifdef APEX
			      case 'X':
				all_ext_flag = 1;
				break;
#endif
			    }
			}
		    	}
			break;

		case 'n':		/* ccom option to enable NLS */
		case 'u':		/* second pass option */
		case 'y':		/* second pass option */
		case 'Y':		/* second pass option */
#ifdef APEX
                case 'O':
                        origin_file = optarg;
                        break;

                case 'P':
                        apex_flag = 1;
                        break;

                case 'S':
                        target_file = optarg;
                        break;
#endif
		case 'T':		/* second pass option */
		case 'X':		/* second pass option */
			break;
		} /*end switch*/

	/* process file name */
	if( argv[optind] ) {
		p = strip( argv[optind] );
		(void)strncpy( sourcename, p, LFNM );
	}
    tmpopen( );

#ifdef APEX
        if (apex_flag) {
            target_list = get_names(target_file);
            origin_list = get_names(origin_file);
	    warnmask = apex_warn;  /* pick up the set of assumptions appropriate
					for apex */
        }
#endif
	if( !pflag ){  /* set sizes to sizes of target machine */
		struct s1 { char a; char b; };
		struct s2 { char a; short b; };
		struct s3 { char a; int b; } s3;
		struct s4 { char a; long b; } s4;
		struct s5 { char a; char *b; } s5;
		struct s6 { char a; float b; } s6;
		struct s7 { char a; double b; } s7;
		struct s9 { char a; struct b { char c; } b; } s9;
#ifndef offsetof
#define offsetof(__s_name,__m_name)  ((size_t)&(((__s_name*)0)->__m_name))
#endif
		SZCHAR = 8;
		SZSHORT = sizeof(short)*SZCHAR;
		SZINT = sizeof(int)*SZCHAR;
		SZLONG = sizeof(long)*SZCHAR;
		SZPOINT = sizeof(char *)*SZCHAR;
		SZFLOAT = sizeof(float)*SZCHAR;
		SZDOUBLE = sizeof(double)*SZCHAR;
		SZLONGDOUBLE = sizeof(double)*2*SZCHAR;

		al_char[align_like] = ALCHAR = offsetof(struct s1,b)*SZCHAR;
		al_short[align_like] = ALSHORT = offsetof(struct s2,b)*SZCHAR;
		al_int[align_like] = ALINT = offsetof(struct s3,b)*SZCHAR;
		al_long[align_like] = ALLONG = offsetof(struct s4,b)*SZCHAR;
		al_point[align_like] = ALPOINT = offsetof(struct s5,b)*SZCHAR;
		al_float[align_like] = ALFLOAT = offsetof(struct s6,b)*SZCHAR;
		al_double[align_like] = ALDOUBLE = offsetof(struct s7,b)*SZCHAR;
		al_longdouble[align_like] = ALLONGDOUBLE = ALDOUBLE;
		al_struct[align_like] = ALSTRUCT = offsetof(struct s9,b)*SZCHAR;

		/* now, fix some things up for various machines (I wish we had "alignof") */
	}


#ifdef STATS_UDP
		{
		    /* code to send out a UDP packet with information about
		     * the current compiler.  Requires a server that is 
		     * listening for packets.
		     */
		    int s = socket(AF_INET,SOCK_DGRAM,0);
		    struct udp_packet packet;
		    struct sockaddr_in address,myaddress;
		    struct stat statbuf;
		    /* initialize the data */
		    fstat(0,&statbuf);
		    packet.size = statbuf.st_size;
		    packet.inode = statbuf.st_ino;
		    (void)strncpy(packet.name,sourcename,14);
		    packet.flag_ansi = (ansilint != 0);
		    packet.flag_a = (aflag != 0);
		    packet.flag_b = (brkflag != 0);
		    packet.flag_h = (hflag != 0);
		    packet.flag_p = (pflag != 0);
		    packet.flag_u = 0;
		    packet.flag_v = (vflag != 0);
		    packet.flag_x = (xflag != 0);
		    packet.flag_y = 0;
		    packet.flag_N = lint1size;
		    packet.flag_Z = 0;
		    packet.pass = 0;
		    packet.version = INST_VERSION;
		    /* set up the addresses */
		    address.sin_family = AF_INET;
		    address.sin_port = INST_UDP_PORT;
		    address.sin_addr.s_addr = INST_UDP_ADDR;
		    myaddress.sin_family = AF_INET;
		    myaddress.sin_port = 0;
		    myaddress.sin_addr.s_addr = INADDR_ANY;
		    /* try blasting a packet out, no error checking here */
		    bind(s,&myaddress,sizeof(myaddress));
		    sendto(s,&packet,sizeof(packet),0,&address,
			   sizeof(address));
		}
#endif /* STATS_UDP */
#ifdef APEX
        outbuf_sz = DEF_OUT_SZ;
        outbuf = malloc(outbuf_sz);
        if (outbuf == NULL) {
#ifdef BBA_COMPILE
#pragma	    	BBA_IGNORE
#endif
                fprntf(stderr, "out of memory (outbuf)\n");
                exit(-1);
        }
	format_magic = 0x26000101;
	(void)fwrite( (void *)&format_magic, 4, 1, stdout );

#endif
	return( mainp1( argc, argv ) );
}

/* commdec - common declaration */
commdec( q )
register struct symtab *q;
{
	/* 10/14/80 - the compiler complains, and so should lint */
/* this check is not quite right, e.g.,		void (*f)();
 *		-- tsize() performs a similar check, but will return
 *		before so if we've got a pointer.
 *	so, let tsize figure it out.
 *
 *	if( dimtab[ stab[i].sizoff ] == 0 )
 */
	(void) tsize(q->stype, q->dimoff, q->sizoff, q->sname);
	/* put out a common declaration */
	outdef( q, libflag?LIB:LDC, 0, lineno );
}

/* fldcon - check assignment of a constant to a field */
fldcon( p ) register NODE *p;
{
	/* check to see if the assignment is going to overflow,
	  or otherwise cause trouble */
	register s;
	CONSZ v;

	if( !hflag & !pflag ) return;

	s = UPKFSZ(p->in.left->tn.rval);
	v = p->in.right->tn.lval;

	switch( p->in.left->in.type ){

	case CHAR:
	case INT:
	case SHORT:
	case LONG:
	case ENUMTY:
		if( v>=0 && (v>>(s-1))==0 ) return;
		/* "precision lost in assignment to (possibly sign-extended) field" */
		WERROR( MESSAGE( 93 ) );
	default:
		return;

	case UNSIGNED:
	case UCHAR:
	case USHORT:
	case ULONG:
		/* "precision lost in field assignment" */
		if( v<0 || (v>>s)!=0 ) WERROR( MESSAGE( 94 ) );
	}
}
/* outdef - output a definition for the second pass */


#ifdef APEX
rec_cnv(lty) {
	switch (lty) {
	    case LDI:		return REC_LDI; 
	    case LDI|LPR:	return REC_LDI_NEW; 
	    case LIB:		return REC_LIB; 
	    case LIB|LPR:	return REC_LIB_NEW; 
	    case LDC:		return REC_LDC; 
	    case LDX:		return REC_LDX; 
	    case LDX|LPR:	return REC_LDX_NEW; 
	    case LRV:		return REC_LRV; 
	    case LRV|LPR:	return REC_LRV; 
	    case LUV:		return REC_LUV; 
	    case LUV|LPR:	return REC_LUV; 
	    case LUE:		return REC_LUE; 
	    case LUE|LPR:	return REC_LUE; 
	    case LUE|LUV:	return REC_LUE_LUV;
	    case LUE|LUV|LPR:	return REC_LUE_LUV;
	    case LUM:		return REC_LUM; 
	    case LUM|LPR:	return REC_LUM; 
	    case LDS:		return REC_LDS; 
	    case LDS|LPR:	return REC_LDS_NEW;
	    case LFE:		return REC_PROCEND;
	    case LDX|LBS|LPR:	return REC_STD_LPR;
	    case LDX|LBS:	return REC_STD_LDX;
	}
	return REC_UNKNOWN;
}

outdef( p, lty, acount, line ) struct symtab *p;
{
	/* Special test to suppress LUM records for local statics
	 * (no LDS is emitted, so avoid "used but not defined"
	 */
	if (lty==LUM && p->sclass==STATIC && p->slevel>1) return;

	outbuf_ptr = outbuf;

	outbase(p, lty, acount, line);

	if (outbuf_ptr != outbuf) {
	    *(short *)outbuf = (outbuf_ptr - outbuf - 2);
	    (void)fwrite( (void *)outbuf, (outbuf_ptr - outbuf), 1, stdout );
	}
	
}


/* fill buffer with the tag and base type, but defer writing (to allow call
 * argument types to be appended.
 */
outbase( p, lty, acount, line ) struct symtab *p;
{
char tag;
char *name;
short len;
short scnt, rets;

	fsave(ftitle);

	tag = rec_cnv(lty);
	(void)fwrite((void *)&tag, sizeof(tag), 1, stdout);
	if (tag == REC_PROCEND) {
		len = 0;
		(void)fwrite((void *)&len, 2, 1, stdout);
		outbuf_ptr = outbuf;
		return;
	} else {
		outbuf_ptr = outbuf + 2;	/* leave room for len */
		memmove(outbuf_ptr, &line, 4);
		outbuf_ptr += 4;
		name = ( p->sclass == STATIC ) ? p->sname : exname( p->sname );
		shroud(name);
		outtype(p);
		if ( ISFTN(p->stype) ) {
		    scnt = acount;
		    memmove(outbuf_ptr, &scnt, 2);
		    outbuf_ptr += 2;
		    rets = 0;
		    memmove(outbuf_ptr, &rets, 2);
		    outbuf_ptr += 2;
		}
	}

}



/* Write the type encoding for symbol p:
 *	Type word
 *	Attribute word, including flags for typedef, const arg, and complex
 *	If basetype==STRTY, write checksum and size
 *	For each ARY modifier in the type word, write one word of size
 *	If typedef bit is set in the attribute word, write null-terminated name
 */
outtype(p)
struct symtab *p;
{
    long attr;
    long checksum;
    char numdim;
    int dimindex;
    short len;
    struct symtab *s;
    TWORD base, mods;
    int complex;

	memmove(outbuf_ptr, &(p->stype), 4);
	outbuf_ptr += 4;
	attr = p->sattrib & ~(0x3c);
	/* add const arg, typedef, complex flags */
	base = BTYPE(p->stype);
	if (base == STRTY || base == UNIONTY) {
	    complex = check_complex(dimtab[p->sizoff+1]);
	    if (complex==FTYCOMPLEX)
		attr |= ATTR_COMPLEX;
	    else if (complex==FTYDCOMPLEX)
		attr |= ATTR_DCOMPLEX;
	    memmove(outbuf_ptr, &attr, 4);
	    outbuf_ptr += 4;
	    checksum = compute_struct(dimtab[p->sizoff+1], base==UNIONTY);
	    memmove(outbuf_ptr, &checksum, 4);
	    outbuf_ptr += 4;
	    s = (struct symtab *)dimtab[p->sizoff+3]; 
	    if ( p->sizoff && dimtab[p->sizoff + 3] && s ) 
		shroud(s->sname);
	    else
		shroud(NULL);
	} else {
	    memmove(outbuf_ptr, &attr, 4);
	    outbuf_ptr += 4;
	}
	mods = p->stype & ~BTMASK;
	numdim = 0;
	while (mods) {
	    if (ISARY(mods)) {
		    numdim++;
	    }
	    mods = DECREF(mods);
	}
	*(char *)outbuf_ptr = numdim;
	outbuf_ptr += 1;

	dimindex = p->dimoff;
        while (numdim) {        /* CLLbs00678 Asok   10/28/93 */
            if (outbuf_ptr + SAFETY_ZONE > outbuf + outbuf_sz){
                int offset = outbuf_ptr - outbuf;
                outbuf_sz += SAFETY_ZONE ;                
                if ((outbuf=realloc(outbuf, outbuf_sz)) == NULL) {
                  fprntf(stderr, "out of memory (outbuf resize)\n");
                  exit (-1);
                }
                outbuf_ptr = outbuf + offset;
            }

	    memmove(outbuf_ptr, &dimtab[dimindex], 4);
	    outbuf_ptr += 4;
	    dimindex++;
	    numdim--;
	}


	/* if typedef seen, print name */
	if (p->typedefsym) {
	    shroud(p->typedefsym->sname);
	} else {
	    len = 0;
	    memmove(outbuf_ptr, &len, 2);
	    outbuf_ptr += 2;
	}

}


/* Fill buffer with an ATYPE encoding.
 *  Called by lpta  (no dim, typedef, or fmt string info available)
 */
outatype(t) 
ATYPE t;
{
    long checksum;
    char numdim;
    int dimindex;
    short len;
    int mods;

	memmove(outbuf_ptr, &(t.aty), 4);
	outbuf_ptr += 4;
	/* add complex flags */
	memmove(outbuf_ptr, &(t.extra), 4);
	outbuf_ptr += 4;
	if (BTYPE(t.aty)==STRTY || BTYPE(t.aty)==UNIONTY) {
	    checksum = t.stcheck;
	    memmove(outbuf_ptr, &checksum, 4);
	    outbuf_ptr += 4;
	    shroud(t.stname);
	}

	numdim = 0;
	dimindex = (int)t.dimptr;
	mods = t.aty & ~BTMASK;
	while ( mods ) {
	    if (ISARY(mods) )
		numdim++;
	    mods = DECREF(mods);
	}
	*(char *)outbuf_ptr = numdim;
	outbuf_ptr += 1;
        while ( numdim && dimtab[dimindex] ) {/* CLLbs00678 Asok   10/28/93 */
            if (outbuf_ptr + SAFETY_ZONE > outbuf + outbuf_sz){
                int offset = outbuf_ptr - outbuf;
                outbuf_sz += SAFETY_ZONE ;
                if ((outbuf=realloc(outbuf, outbuf_sz)) == NULL) {
                  fprntf(stderr, "out of memory (outbuf resize)\n");
                  exit (-1);
                }
                outbuf_ptr = outbuf + offset;
            }
	    memmove(outbuf_ptr, &dimtab[dimindex], 4);
	    outbuf_ptr += 4;
	    dimindex++;
	    numdim--;
	}

	/* no typedef info */
	len = 0;
	memmove(outbuf_ptr, &len, 2);
	outbuf_ptr += 2;

	/* shroud the format string (empty for now)  */
	memmove(outbuf_ptr, &len, 2);
	outbuf_ptr += 2;
}



/* Do a simple encryption of strings to protect the investment in portability
 * hints.  Fill the outbuf with the encrypted string.
 */
shroud(p)
  char *p;
{
short s_len;
int offset;

    if (p)
	s_len = strlen(p);
    else
	s_len = 0;
    memmove(outbuf_ptr, &s_len, 2);
    outbuf_ptr += 2;

    /* CLLbs00678 Asok   10/28/93 */
    if (outbuf_ptr + s_len +SAFETY_ZONE > outbuf + outbuf_sz) {
	offset = outbuf_ptr - outbuf;
	outbuf_sz += outbuf_sz + s_len + SAFETY_ZONE ;
	if ((outbuf=realloc(outbuf, outbuf_sz)) == NULL) {
#ifdef BBA_COMPILE
#pragma	    BBA_IGNORE
#endif
	    fprntf(stderr, "out of memory (outbuf resize)\n");
	    exit (-1);
	}
	outbuf_ptr = outbuf + offset;
    }
#ifdef CLEAR_TEXT
    strcpy(outbuf_ptr, p);		
    outbuf_ptr += s_len;
#else
    /* a simple cipher that maps lower-case letters to unprintable ASCII */
    while (p && *p) {
	*outbuf_ptr++ = (*p++ + 160) % 256;
    }
#endif
}


/* Compute a checksum based on a struct's member names and types.
 * indx is the dimtab index of a pointer to the first member symtab entry.
 */
compute_struct(indx, union_flag)
int indx;
int union_flag;
{
    struct symtab *s;
    int checksum, member;

	checksum = 0;
	while (dimtab[indx]) {
	    s = (struct symtab *)dimtab[indx];
	    if (s->stype==STRTY)
		member = compute_struct(dimtab[s->sizoff+1], 0);
	    else if (s->stype==UNIONTY)
		member = compute_struct(dimtab[s->sizoff+1], 1);
	    else
		member = hash_struct(s->sname, s->stype, s->sclass);

	    if (union_flag)
		checksum += member;
	    else
		checksum = (checksum << 1) + member;

	    indx++;
	}

	return checksum;
}


hash_struct(name, type, class)
char *name;
TWORD type;
int class;
{
	return (hash_string(name) + type + (class&FIELD) );
}


hash_string(name)
char *name;
{
int sum;

	sum = 0;
	while (*name) {
	    sum = (sum << 1) + *name;
	    name++;
	}
	return sum;
}


/* Check to see if this C struct is type compatible with a Fortran complex
 * or double complex.
 */

int check_complex(index)
int index;
{
    struct symtab *s;

	if (dimtab[index]) {
	    s = (struct symtab *)dimtab[index];
	    if (s->stype==FLOAT) {
		if ( (s = (struct symtab *)dimtab[index+1]) &&
		     (s->stype==FLOAT) &&
		     (dimtab[index+2] == 0) )
		    return FTYCOMPLEX;
	    } else if (s->stype==DOUBLE) {
		if ( (s = (struct symtab *)dimtab[index+1]) &&
		     (s->stype==DOUBLE) &&
		     (dimtab[index+2] == 0) )
		    return FTYDCOMPLEX;
	    } 
	}

	return UNDEF;
}


/* output an APEX comment record in a form compatible with a normal record */
void
outstd(tag, origin, stds, detail_min, detail_max, cmt)
int tag;	/* REC_STD or REC_HINT */
int origin;
int stds[4];	/* the bit mask of applicable standards */
int detail_min, detail_max;
char *cmt;	/* a (possibly-null) portability hint */
{
char tag_char;
size_t cmt_len;
short len;
char detail_char;

	tag_char = tag;
	(void)fwrite((void *)&tag_char, 1, 1, stdout);
	outbuf_ptr = outbuf;
	shroud(cmt);
	cmt_len = outbuf_ptr - outbuf;
	len = 22 + cmt_len;	 /* 16 bytes stds + string (including len) */
	(void)fwrite((void *)&len, 2, 1, stdout);
	(void)fwrite((void *)&origin, 4, 1, stdout);
	(void)fwrite((void *)stds, 16, 1, stdout);
	detail_char = detail_min;
	(void)fwrite((char *)&detail_char, 1, 1, stdout);
	detail_char = detail_max;
	(void)fwrite((char *)&detail_char, 1, 1, stdout);
	(void)fwrite((void *)outbuf, cmt_len, 1, stdout);
}


/* warn about __attribute or __options */
warn_domain(token)
int token;
{
/* could customize the message based on token==ATTRIBUTE or token==OPTIONS */

	if (ignore_extensions)
                return;
        if (all_ext_flag)
                WERROR( MESSAGE( 242 ) );       /* warn unconditionally */
        else if (iscfile(strip(ftitle)))
                WERROR( MESSAGE( 242 ) );       /* warn only if in sourcefile */

}


#else	/* non-APEX */

outdef( p, lty, acount, line ) struct symtab *p;
{
	static union rec rc;

	/* Special test to suppress LUM records for local statics
	 * (no LDS is emitted, so avoid "used but not defined"
	 */
	if (lty==LUM && p->sclass==STATIC && p->slevel>1) return;

	fsave(ftitle);

	rc.l.decflag = lty;
	if( lty == LRV ) /* FTN returning type => type */
		{
		rc.l.type.aty = DECREF(p->stype);
		rc.l.type.extra = ((DECREF(p->sattrib)) & ~BTMASK) |
					(p->sattrib & (ATTR_CON|ATTR_VOL))<<1;
		}
	else
		{
		rc.l.type.aty = p->stype;
		rc.l.type.extra = (p->sattrib & ~BTMASK) |
					(p->sattrib & (ATTR_CON|ATTR_VOL))<<1;
		}
	(void)astype( &rc.l.type, p->sizoff );
	rc.l.nargs = acount;
	rc.l.fline = line;
	/* Write out the rc structure an element at a time.  This code used
	 * to fwrite the entire structure, but was changed to produce a
	 * s300/s800 portable structure 
	 * (void)fwrite( (void *)&rc, sizeof(rc), 1, stdout ); */
	(void)fwrite((void *)&rc.l.decflag, sizeof(rc.l.decflag), 1, stdout);
	(void)fwrite((void *)&rc.l.name, sizeof(rc.l.name), 1, stdout);
	(void)fwrite((void *)&rc.l.nargs, sizeof(rc.l.nargs), 1, stdout);
	(void)fwrite((void *)&rc.l.fline, sizeof(rc.l.fline), 1, stdout);
	(void)fwrite((void *)&rc.l.type.aty, sizeof(rc.l.type.aty), 1, stdout);
	(void)fwrite((void *)&rc.l.type.extra, sizeof(rc.l.type.extra), 1, stdout);
	
	rc.l.name = ( p->sclass == STATIC ) ? p->sname : exname( p->sname );
	(void)fwrite( (void *)rc.l.name, strlen( rc.l.name ) + 1, 1, stdout );
}

#endif

#ifdef __lint	/* stubs for use when linting lint */
/*ARGSUSED*/
locctr(l) {return 0;}
/*ARGSUSED*/
QUAD _U_Qfadd(d1, d2) QUAD d1, d2; {return d1;}
/*ARGSUSED*/
QUAD _U_Qfsub(d1, d2) QUAD d1, d2; {return d1;}
/*ARGSUSED*/
QUAD _U_Qfmpy(d1, d2) QUAD d1, d2; {return d1;}
/*ARGSUSED*/
QUAD _U_Qfdiv(d1, d2) QUAD d1, d2; {return d1;}
/*ARGSUSED*/
int _U_Qfcmp(d1, d2, t) QUAD d1, d2; int t; {return d1.d[1]<d2.d[1];}
QUAD _U_Qfneg(d1) QUAD d1; {return d1;}
int _U_Qflogneg(d1) QUAD d1; {return d1.u[1];}
QUAD _U_Qfcnvff_dbl_to_quad(d) double d; {QUAD d1; d1.d[1]=d; return d1;}
QUAD _U_Qfcnvxf_usgl_to_quad(d) unsigned long d; {QUAD d1; d1.u[1]=d;return d1;}
QUAD _U_Qfcnvxf_sgl_to_quad(d) signed long d; {QUAD d1; d1.u[1]=d;return d1;}
/*ARGSUSED*/
QUAD _atold(s) char *s; {QUAD d; d.u[1]=0; return d;}

#   ifdef __hp9000s300
/*VARARGS1*/
/*ARGSUSED*/
int prntf(s) char *s; {return 0;}
/*VARARGS2*/
/*ARGSUSED*/
int fprntf(f,s) FILE *f; char *s; {return 0;}
/*VARARGS2*/
/*ARGSUSED*/
int sprntf(s,f) char *s, *f; {return 0;}
#endif

#ifndef SLOW_WALKF
/*ARGSUSED*/
int walkf(p,f) NODE *p; int (*f)(); {return 0;}
#endif

#endif

/* lint_max_align - determines the maximal alignment of a structure for use in
 *                  lint alignment checking */

int lint_max_align(type,csiz) TWORD type; int csiz; {
  int maxalign = 8;	/* start with maximal alignment being an int */
  int align;
  int index;
  struct symtab *sp;
  int i;
  
	for (index=dimtab[csiz+1];dimtab[index];index++){
		sp = (struct symtab *) dimtab[index];
		if (sp->sizoff < 0) continue; /* fields, keep alignment */
		for (i=0;i<=(SZINT-BTSHIFT-1);i+=TSHIFT){
		    switch((sp->stype>>i)&TMASK){
			case PTR: align = al_point[align_like]; break;
			case ARY: continue;
			case 0:
			    switch(BTYPE(sp->stype)){
				case UNIONTY:
				case STRTY:
				    align = lint_max_align(sp->stype,
							   sp->sizoff);
				    break;
				default: align = tsize(BTYPE(sp->stype),
						       sp->dimoff,sp->sizoff,
						       NULL);
				    break;
				}
				break;
		    }
		    break;
		}
		if (align > maxalign) maxalign = align;
	}
	return maxalign;
}
