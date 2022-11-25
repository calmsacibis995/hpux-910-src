/* file misc.c */
/* @(#) $Revision: 70.4 $ */
/* KLEENIX_ID @(#)misc.c	20.3 91/05/08 */

# include "c1.h"

/* defines for worthit() */
# 	define LOTS_USE		6
# define AREG_BREAKPOINT	5	/* 4 regvars + 1 guesstimate not used */
# define DREG_BREAKPOINT	6	/* 5 regvars + 1 guesstimate not used */
# define OFTEN_USED(np)	(np->ind.usage >= often_count)

# define BDECREF(x) ((((x)>>TSHIFT)&~BTMASK)|((x)&BTMASK))
# define ISVOL(x)	(x&0x2000)
# define NO_STMTNO	65535

/* a lot of the machine dependent parts of the second pass */

/*	some storage declarations */

int usedregs;		/* flag word for registers used in current routine */
int ftnno;  			/* number of current function */
int lineno;			/* current line number from pass 1 */
OFFSZ tmpoff;  /* offset for first temporary, in bytes for current block */
OFFSZ maxoff;  /* maximum temporary offset over all blocks in current ftn,
		  in bytes
		*/
OFFSZ baseoff;
unsigned int offsz;
NODE *node;
EPB *ep;			/* entry point table */

int maxtascizsz = TASCIZSZ;
int maxeascizsz = TASCIZSZ;
int maxnodesz = TREESZ;
int maxbblsz = BBLSZ;
int maxseqsz = DAGSZ/2;
int maxregion = REGIONSZ;
int maxepsz = EPSZ;
int maxlabstksz = MAXLABVALS;
int acount = AREG_BREAKPOINT;	/* Set high by default intentionally */
int dcount = DREG_BREAKPOINT;	/* Set high by default intentionally */

int dope[ DSIZE ];
char *opst[DSIZE];
long reg_savings[DSIZE][N_REG_SAVINGS_TYPES];
long reg_store_cost[N_REG_SAVINGS_TYPES];
long reg_save_restore_cost[N_REG_SAVINGS_TYPES];
long hiddenfargchain;
int lastep;

unsigned *labvals;		/* the label stack */
unsigned *lstackp;		/* the label stack pointer */
unsigned fwalk2_bitmask;	/* flag settings for fwalk2() */
unsigned fwalk2_clrmask;	/* flag settings for fwalk2() */
unsigned w_halt;		/* halting condition for fwalk2(), fwalkc() */
long	farg_low;
long	farg_high;
long	farg_diff;
short 	nerrors;  		/* number of errors */
ushort 	assumptions;		/* bit mask for C1 assumptions */
char	*treeasciz;		/* asciz table for syms in tree nodes */
char	*lasttac;		/* ptr to first free char in treeasciz */
char	*externasciz;		/* asciz table for external syms */
char	*lasteac;		/* ptr to first free char in externasciz */
char 	olevel = 2;		/* C1 optimization level specified */
flag	verbose;		/* C1 will warn if verbose and too complex */
flag	gcp_disable;		/* YES iff no global constant propagation */
flag	sr_disable;		/* YES iff no strength reduction on ivs */
flag	fortran;		/* YES iff working on FORTRAN code */
flag    flibflag;		/* YES if codegen is to make lib calls */
flag    fpaflag;		/* value FFPA, BFPA if codegen is for FPA */
flag	mc68040 = YES;          /* YES iff register allocation for 68040 */
flag	comops_disable;		/* YES iff want to disable rewrite_comops() */
flag 	global_disable;		/* disable global opts */
flag	master_global_disable;
flag	loop_allocation_flag = YES;
flag 	defsets_on;		/* YES iff auto creation of sp->defset is
				   desired in find().
				*/
	flag	pic_flag = NO;  /* YES: compiling position independant code */

LOCAL NODE *lastfree;  /* pointer to last free node; (for allocator) */
LOCAL NODE *currsemicolon;
LOCAL int often_count = LOTS_USE;
LOCAL	char	*taz;
LOCAL	char	*eaz;




#ifdef DEBUGGING
	flag bdebug;	/* for blocks */
	flag ddebug;	/* for DAGs */
	flag edebug;
	flag fdebug;	/* to put block numbers out to the intermediate file */
	flag gdebug;	/* for global optimizations & sets */
	flag ldebug;	/* for lblocks */
	flag mdebug;	/* malloc, free debugging */
	flag pdebug;	/* for p2pass debugging */
	flag qcdebug;	/* (deadstore) constant propagation */
	flag qddebug;	/* (deadstore) definetab and reaching defs */
	flag qpdebug;	/* (deadstore) copy propagation */
	flag qsdebug;	/* (deadstore) dead store removal */
	flag qtdebug;	/* (deadstore) trees */
	flag qudebug;	/* (deadstore) du blocks */
	flag rddebug;	/* (reg) definetab and reaching defs */
	flag rldebug;	/* (reg) live vars */
	flag rodebug;	/* (reg) statement and register comments in assem */
	flag rrdebug;	/* (reg) regions */
	flag rRdebug;	/* (reg) register maps, registers */
	flag rsdebug;	/* (reg) symtab */
	flag rtdebug;	/* (reg) trees */
	flag rudebug;	/* (reg) du blocks */
	flag rwdebug;	/* (reg) webs and loops */
	flag sdebug;	/* for symbol table debugging */
	flag udebug;	/* for heap usage information */
	flag xdebug;
	flag odisable;	/* optim() disable */
	flag emitname;	/* if YES, emit procname to debugp file. */
#endif DEBUGGING
	flag dagdisable;/* disable dagging */
	flag reg_disable;	/* disable register allocation */
	flag pass_flag;
	flag coalesce_disable;
	flag warn_disable;
	flag udisable;	/* to turn off combine_cmtemps() */

	flag paren_enable = YES;

#ifdef COUNTING
	flag print_counts;
	char *count_file;
#endif

char	**optz_procnames;
char	**pass_procnames;
long 	last_optz_procname;
long	max_optz_procname;
long	last_pass_procname;
long	max_pass_procname;




char * rnames[]= {  /* keyed to register number tokens */

	"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
	"%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp",
	"%fp0", "%fp1", "%fp2", "%fp3", "%fp4", "%fp5", "%fp6", "%fp7",
	"%fpa0", "%fpa1", "%fpa2", "%fpa3", "%fpa4", "%fpa5", "%fpa6", "%fpa7",
	"%fpa8", "%fpa9", "%fpa10", "%fpa11", "%fpa12", "%fpa13", "%fpa14",
	"%fpa15"
	};

void rewrite_comops_in_block();
LOCAL flag rewrite_comops_in_stmt();
void node_constant_collapse();

/******************************************************************************
*
*	decref()
*
*	Description:		decref removes the topmost attribute/modifier
*				from the expanded TTYPE struct.
*
*	Called by:		computed_aref_unary_mul()
*
*	Input parameters:	t - a TTYPE struct describing type info.
*
*	Output parameters:	return value -- new type
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
TTYPE decref(t)	TTYPE t;
{
	register TTYPE newt;

	newt.base = (t.base & 0xf03f) | (((t.base & 0xfc0) >> 2) & 0xfc0);
	newt.mods1 = 0;
	newt.mods2 = 0;
	if (TMODS1(t.base))
		{
		/* at least 1 long word of extension */
		newt.base |= ( (t.mods1 >> 12) & 3) << 10;
		newt.mods1 = ((t.mods1 & 0xfff) >> 2) 
				| ((t.mods1 >> 2) & 0xfffff000);
		if (TMODS2(t.base))
			{
			/* a second short of extension */
			newt.mods1 |= (t.mods2 & 3) << 10;
			newt.mods2 = t.mods2 >> 2;
			}
		}
	return (newt);
}  /* decref */

/******************************************************************************
*
*	INCREF()
*
*	Description:		incref adds the topmost attribute/modifier
*				to the expanded TTYPE struct.
*
*	Called by:		make_load_store_stmt()
*
*	Input parameters:	t - a TTYPE struct describing type info.
*				m - modifier
*				a - attribute
*
*	Output parameters:	return value -- new type
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
TTYPE incref(t,m,a)
TTYPE t;
long m;
long a;
{
    register TTYPE newt;
    long mcarry;
    long acarry;

    mcarry = t.base & 0xc00;
    newt.base = (t.base & 0xf03f) | (((t.base & 0xfc0) << 2) & 0xfc0) | m;
    if (mcarry | TMODS1(t.base) | a)
	{
	acarry = t.mods1 & 0xc00;
	mcarry <<= 2;
	newt.mods1 = (((t.mods1 & 0xfff) << 2) & 0xfff) | a
			| ((t.mods1 & 0xfffff000) << 2) | mcarry;
	if (TMODS2(t.base) | acarry)
	    {
	    acarry >>= 10;
	    newt.mods2 = ((t.mods2 << 2) & 0xcfff) | acarry;
	    newt.base |= MODS1MASK | MODS2MASK;
	    }
	else
	    {
	    newt.mods2 = 0;
	    newt.base |= MODS1MASK;
	    }
	}
    else
	{
	newt.mods1 = 0;
	newt.mods2 = 0;
	}
    return (newt);
}  /* incref */


/******************************************************************************
*
*	p2init()
*
*	Description:		set the values of the c1 arguments.
*
*	Called by:		flags_and_files()
*
*	Input parameters:	argc
*				argv
*
*	Output parameters:	none
*
*	Globals referenced:	max_optz_procname
*				optz_procnames
*				last_optz_procname
*				max_pass_procname
*				pass_procnames
*				last_pass_procname
*				reg_types
*				pass_flag
*				fortran
*
*	Globals modified:	max_optz_procname
*				optz_procnames
*				last_optz_procname
*				max_pass_procname
*				pass_procnames
*				last_pass_procname
*				sr_disable
*				gcp_disable
*				fdebug
*				dagdisable
*				fpaflag
*				master_global_disable
*				global_disable
*				flibflag
*				reg_types
*				pass_flag
*				copyprop_disable
*				deadstore_disable
*				disable_empty_loop_deletion
*				disable_unreached_loop_deletion
*				reg_disable
*				icon_disable
*				loop_allocation_flag
*				bdebug
*				ddebug
*				edebug
*				gdebug
*				ldebug
*				mdebug
*				pdebug
*				qcdebug
*				qddebug
*				qpdebug
*				qsdebug
*				qtdebug
*				qudebug
*				rddebug
*				rldebug
*				rodebug
*				rrdebug
*				rRdebug
*				rsdebug
*				rtdebug
*				rudebug
*				rwdebug
*				sdebug
*				udebug
*				xdebug
*				fortran
*				pic_flag
*				emitname
*				goblockcutoff
*				godefcutoff
*				goregioncutoff
*				verbose
*
*	External calls:		strtol()
*				ckalloc()
*				werror()
*				uerror()
*				ckrealloc()
*				mkdope()
*				tolower()
*
*******************************************************************************/
p2init( argc, argv ) char *argv[];
{

	register short c;
	register char *cp;
	register files;
	char *cpp;

	files = 0;
	max_optz_procname = PROCLISTSIZE;
	optz_procnames = (char **) ckalloc(max_optz_procname * sizeof(char *));
	last_optz_procname = -1;
	max_pass_procname = PROCLISTSIZE;
	pass_procnames = (char **) ckalloc(max_pass_procname * sizeof(char *));
	last_pass_procname = -1;

	for( c=1; c<argc; ++c ){
		if( *(cp=argv[c]) == '-' ){
			while( *++cp ){
				switch( *cp ){

#ifdef DEBUGGING
				case 'b':  /* block numbers passed */
					++fdebug;
					break;
#endif DEBUGGING

				case 'c':	/* glob. constant prop. */
					gcp_disable = YES;
					break;

				case 'd':  /* dags */
					werror("-W g,-d has been specified. CSE disabled.");
					dagdisable = YES;
					break;

# ifdef DEBUGGING
				case 'e':	/* optim() */
					odisable = YES;
					break;
# endif DEBUGGING

				case 'f':  /* fpa code gen */
					fpaflag = FFPA;
					break;

				case 'h':  /* +bfpa */
					fpaflag = BFPA;
					break;

				case 'i':  /* global opts */
					master_global_disable = YES;
					global_disable = YES;
					break;

# ifdef DEBUGGING
				case 'j': /* comops rewrite disabling */
					comops_disable = YES;
					/* WARNING: This option will cause
					   c1 to bomb unless -qa and -r
					   are also specified!
					*/
					break;
# endif DEBUGGING

				case 'l':  /* f1 will make lib calls */
					flibflag = YES;
					break;

				case 'm':       /* 68040 register allocation */
                                        mc68040 = NO;
                                        break;

				case 'n':  /* set # regs available */
					switch(*++cp)
					{
					long nregs;

					case 'a':
					    reg_types[REGATYPE].nregs =
								*++cp - '0';
					    break;
					case 'd':
					    reg_types[REGDTYPE].nregs =
								*++cp - '0';
					    break;
					case 'f':
					    nregs = *++cp - '0';
					    if (*(cp + 1))
					        nregs = nregs * 10 +
								(*++cp - '0');
					    reg_types[REG8TYPE].nregs = nregs;
					    reg_types[REGFTYPE].nregs = nregs;
					    break;
					default:
					    uerror("bad option: n%c", *cp);
					}
					break;

				case 'o':  /* optz procname */
					if (*(cp + 1) == 0)
					    uerror("missing name with -o option");
					if (++last_optz_procname
						>= max_optz_procname)
					    {
					    max_optz_procname += PROCLISTSIZE;
					    optz_procnames = (char **)
						ckrealloc(optz_procnames,
					     max_optz_procname * sizeof(char*));
					    }
					optz_procnames[last_optz_procname] =
					    cp + 1;
					for (++cp; *cp; ++cp)
						;	/* skip to end */
					--cp;
					break;

				case 'p':  /* pass option, pass procname */
					if (*(cp + 1) == 0)
					    {
					    pass_flag = YES;
					    break;
					    }

					if (++last_pass_procname
						>= max_pass_procname)
					    {
					    max_pass_procname += PROCLISTSIZE;
					    pass_procnames = (char **)
						ckrealloc(pass_procnames,
					     max_pass_procname * sizeof(char*));
					    }
					pass_procnames[last_pass_procname] =
					    cp + 1;
					for (++cp; *cp; ++cp)
						;	/* skip to end */
					--cp;
					break;

				case 'q':  /* dead store, local copy prop */
					switch (*(++cp))
					    {
					    case 0:
						uerror("missing suboption for -q");
						break;
					    case 'a':	/* disable them all */
						constprop_disable = YES;
						copyprop_disable = YES;
						deadstore_disable = YES;
						disable_empty_loop_deletion
							= YES;
						disable_unreached_loop_deletion
							= YES;
						dbra_disable = YES;
						loop_unroll_disable = YES;
						coalesce_disable = YES;
						break;
					    case 'c':
						constprop_disable = YES;
						break;
					    case 'd':
						dbra_disable = YES;
						break;
					    case 'e':
						disable_empty_loop_deletion
							= YES;
						break;
					    case 'm':
						coalesce_disable = YES;
						break;
					    case 'p':
						copyprop_disable = YES;
						break;
					    case 'r':
						loop_unroll_disable = YES;
						break;
					    case 's':
						deadstore_disable = YES;
						break;
					    case 'u':
						disable_unreached_loop_deletion
							= YES;
						break;
					    default:
					       uerror( "bad option: -q%c", *cp );
					    break;
					    }
					break;

				case 'r':  /* register allocation */
					reg_disable = YES;
					break;

				case 's':  /* do simple register allocation */
					switch (*(++cp))
					    {
					    case 0:
						uerror("missing suboption for -s");
						break;
					    case 'b':
						goblockcutoff = strtol(++cp,&cpp,10);
						break;
					    case 'd':
						godefcutoff = strtol(++cp,&cpp,10);
						break;
					    case 'r':
						goregioncutoff = strtol(++cp,&cpp,10);
						break;
					    default:
					       uerror( "bad option: -s%c", *cp );
					    break;
					    }
					cp = cpp - 1;
					break;

				case 't':  /* disable loop allocation */
					loop_allocation_flag = NO;
					break;

				case 'u':	/* turn off combine_cmtemps() */
					udisable = YES;
					break;

				case 'v':	/* Warn of too complex functions */
					verbose = YES;
					break;

				case 'w':  /* disable warnings */
					warn_disable = YES;
					break;

				case 'x':  /* Optimize nonreducible procs */
					/* After the 8.0 release "-x" will
					 * be the default.  Silently accept
					 * "-x" and do nothing.
					 */
					break;

				case 'y':  /* strength_reduction disabled */
					sr_disable = YES;
					break;

				case 'z':  /* disable PAREN nodes */
					paren_enable = NO;
					break;
				case 'a':
				case 'A':	/* Override default global complexity */
					cpp = cp;
					if (tolower(*++cp) == 'l' &&
					    tolower(*++cp) == 'l' &&
					    !*++cp)
					    	{
						godefcutoff = 10000000;
						goregioncutoff = godefcutoff;
						goblockcutoff = godefcutoff;
						}
					else 
					       uerror( "unrecognized -%s option",
							cpp);
					--cp;
					break;

# ifdef DEBUGGING
				case 'B':  /* BLOCKs */
					++bdebug;
					emitname = YES;
					break;
# endif DEBUGGING

# ifdef COUNTING
				case 'C':	/* internal counts */
					print_counts = YES;
					if (! *++cp)
					    uerror( "bad option: C -- missing file name" );
					count_file = cp;
					while (*++cp)
					    ;
					--cp;
					break;
# endif COUNTING

# ifdef DEBUGGING
				case 'D':  /* DAGs */
					++ddebug;
					emitname = YES;
					break;

				case 'E':  /* expressions */
					++edebug;
					emitname = YES;
					break;
# endif DEBUGGING

				case 'F':	/* source is for FORTRAN */
					++fortran;
					break;

# ifdef DEBUGGING
				case 'G':  /* Sets & global opts */
					++gdebug;
					emitname = YES;
					break;

				case 'L':  /* LBLOCKs */
					++ldebug;
					emitname = YES;
					break;

				case 'M':  /* malloc, free */
					++mdebug;
					emitname = YES;
					break;

				case 'P':  /* P2passing */
					++pdebug;
					emitname = YES;
					break;

				case 'Q':  /* Dead store elimination */
					switch (*++cp)
					    {
					    case 'c':   qcdebug++;
							break;
					    case 'd':   qddebug++;
							break;
					    case 'p':   qpdebug++;
							break;
					    case 's':   qsdebug++;
							break;
					    case 't':   qtdebug++;
							break;
					    case 'u':   qudebug++;
							break;
					    default:
					       uerror( "bad option: -Q%c", *cp );
					    }
					emitname = YES;
					break;

				case 'R':  /* Register allocation */
					switch (*++cp)
					    {
					    case 'd':   rddebug++;
							break;
					    case 'l':   rldebug++;
							break;
					    case 'o':   rodebug++;
							break;
					    case 'r':   rrdebug++;
							break;
					    case 'R':   rRdebug++;
							break;
					    case 's':   rsdebug++;
							break;
					    case 't':   rtdebug++;
							break;
					    case 'u':   rudebug++;
							break;
					    case 'w':   rwdebug++;
							break;
					    default:
					       uerror( "bad option: -R%c", *cp );
					    }
					emitname = YES;
					break;

				case 'S':  /* Symbol table */
					++sdebug;
					emitname = YES;
					break;

				case 'U':  /* Heap usage info */
					++udebug;
					emitname = YES;
					break;

				case 'X':  /* general machine-dependent debugging flag */
					++xdebug;
					emitname = YES;
					break;

# endif	DEBUGGING

				case 'Z':  /* Position independant code */
					pic_flag = YES;
					break;

				default:
					uerror( "bad option: %c", *cp );
					}
				}
			}
		else files = 1;  /* assumed to be a filename */
		}

	mkdope();	/* in common - inits the dope array */
	return( files );
}




# ifdef DEBUGGING
LOCAL void acon( p ) register NODE *p; { /* print out a constant */

	if( p->tn.name == NULL )		/* constant only */
		{
		if (p->tn.op == ICON)
			switch (p->tn.type.base)
			/* For unsigned operands, zero out nonpertinent bits.
			   Don't do it for signed operands or moveq will break.
			*/
			{
			case USHORT:
				p->tn.lval &= 0xffff;
				break;
			case UCHAR:
				p->tn.lval &= 0xff;
				break;
			}
		fprintf(debugp,  CONFMT2, p->tn.lval);
		}
	else if( p->tn.lval == 0 ) 	/* name only */
		fprintf(debugp,  "%s", p->tn.name );
	else 				/* name + offset */
		{
		fprintf(debugp,  "%s+", p->tn.name );
		fprintf(debugp,  CONFMT2, p->tn.lval );
		}
	}





eprint( p, down, a, b ) NODE *p; int *a, *b; 
{
	uchar op = p->in.op;

	*a = *b = down+1;
	while( down >= 2 ){
		fprintf(debugp, "\t" );
		down -= 2;
		}
	if( down-- ) fprintf(debugp, "    " );


	fprintf(debugp, "0x%x) %s", p, (op >= FORTOPS)? xfop(op) : opst[op] );
	switch( op )  /* special cases */
	{

	case REG:
		fprintf(debugp, " %s", rnames[p->tn.rval] );
		break;

	case ICON:
		fprintf(debugp, " " );
		/* addressable value of the constant */
		if( p->in.type.base == DOUBLE )
			{
			/* print the high order value */
			CONSZ save;
			fprintf(debugp,"\nBINGO. Hit ICON of type DOUBLE!\n");
			save = p->tn.lval;
			p->tn.lval = ( p->tn.lval >> SZINT ) & BITMASK(SZINT);
			fprintf(debugp, "&" );
			acon( p );
			p->tn.lval = save;
			break;
			}
		fprintf(debugp, "&" );
		acon( p );
		break;

	case NAME:
		fprintf(debugp, " " );
		acon(p);
		break;

	case FOREG:
	case OREG:
		fprintf(debugp, " " );
# ifdef R2REGS
		if (p->tn.rval < 0)
			{
			/* It's a (packed) indexed oreg. */
			print_indexed(p);
			break;
			}
# endif	/* 	R2REGS */
		if( p->tn.rval == A6 )  /* in the argument region */
			{
			fprintf(debugp, CONFMT, p->tn.lval );
			fprintf(debugp, "(%%a6)" );
			break;
			}
		if( p->tn.lval != 0 || p->tn.name != NULL )
		  	acon( p );
		fprintf(debugp, "(%s)", rnames[p->tn.rval] );
		break;
	
	case LGOTO:
		fprintf(debugp, "{%d}", p->bn.label );
		break;

	case STCALL:
	case UNARY STCALL:
	case STARG:
	case STASG:
		break;

	}
	fprintf(debugp, ", " );
	tprint( p->in.type );
	if ( optype(p->in.op) != LTYPE ) fprintf(debugp,", ((%d))", p->ind.seq);
	fprintf(debugp, ", flags = %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d%d%d %d%d%d\n",
		p->in.common_node, p->in.invariant, p->in.arrayref,
			p->in.equivref, p->in.dagobsolete, 

		p->in.temptarget, p->in.lhsiref,
		p->in.csefound, p->in.no_iconsts, p->in.common_base,
		p->in.callref, p->in.iv_checked, p->in.entry,
		p->in.arrayelem, p->in.isload,

		p->in.structref, p->in.isptrindir, p->in.arraybaseaddr,
		p->in.isfpa881stmt, p->in.isarrayform,

		p->in.parm_types_matched, p->in.comma_ss, p->in.no_arg_uses,
		p->in.no_arg_defs, p->in.no_com_uses,

		p->in.no_com_defs, p->in.isc0temp, p->in.safe_call);
}


tcheck(){ /* ensure that all nodes have been freed */

	register NODE *p, *end;
	NODE *tab;

	if( !nerrors )
	    {
	    tab = node;
	    while (tab != (NODE *) 0)
		{    
		for( p=tab, end= &tab[maxnodesz-1]; p<=end; ++p )
			if( p->in.op != FREE ) cerror( "wasted space: %x", p );
		tab = *(((NODE **)tab) - 1);
		}
	    }

	if (udebug > 0)
	  if (in_procedure) /* Avoid redundant output at end of input file */
	    {
	    if (udebug > 1)
	      {
	      int  node_bytes_allocated, node_bytes_used;
	      int  taz_bytes_allocated, taz_bytes_used;
	      int  eaz_bytes_allocated, eaz_bytes_used;
	      int  du_bytes_allocated, du_bytes_used;
	      int  loadst_bytes_allocated, loadst_bytes_used;
	      int  plink_bytes_allocated, plink_bytes_used;
	      int  web_bytes_allocated, web_bytes_used;
	      int  total_alloc = 0, total_used = 0;
	      char *ptr;
    
	      node_bytes_allocated = 4 + maxnodesz * sizeof(NODE);
	      node_bytes_used = (lastfree - node) * sizeof(NODE) + 4;
	      tab = node;
	      while (*(((NODE **)tab) - 1) != (NODE *) 0)
	        {
	        node_bytes_allocated =  node_bytes_allocated +
					    4 + maxnodesz * sizeof(NODE);
	        node_bytes_used = node_bytes_used +
					    4 + maxnodesz * sizeof(NODE);
	        tab = *(((NODE **)tab) - 1);
	        }
	      fprintf(debugp,"'node'         bytes allocated: %7d used: %7d\n",
		      node_bytes_allocated, node_bytes_used);
	      total_alloc += node_bytes_allocated;
	      total_used += node_bytes_used; 
    
	      taz_bytes_allocated = maxtascizsz * sizeof(char);
	      taz_bytes_used = lasttac - taz;
	      ptr = taz;
	      while (*(((char **)ptr) - 1) != (char *) 0)
	        {
	        taz_bytes_allocated =  taz_bytes_allocated +
					    maxtascizsz * sizeof(char);
	        taz_bytes_used = taz_bytes_used +
					    maxtascizsz * sizeof(char);
	        ptr = *(((char **)ptr) - 1);
	        }
	      fprintf(debugp,"'taz'          bytes allocated: %7d used: %7d\n",
		      taz_bytes_allocated, taz_bytes_used); 
	      total_alloc += taz_bytes_allocated;
	      total_used += taz_bytes_used; 
  
	      if (!fortran)
	      	{
		eaz_bytes_allocated = maxeascizsz * sizeof(char);
	      	eaz_bytes_used = lasteac - eaz;
	      	ptr = eaz;
	      	while (*(((char **)ptr) - 1) != (char *) 0)
	        	{
	        	eaz_bytes_allocated =  eaz_bytes_allocated +
					    	maxeascizsz * sizeof(char);
	        	eaz_bytes_used = eaz_bytes_used +
					    	maxeascizsz * sizeof(char);
	        	ptr = *(((char **)ptr) - 1);
	        	}
	      	fprintf(debugp,"'eaz'          bytes allocated: %7d used: %7d\n",
		      	eaz_bytes_allocated, eaz_bytes_used);
	      	total_alloc += eaz_bytes_allocated;
	      	total_used += eaz_bytes_used; 
		}
	      
	      fprintf(debugp,"'common_child' bytes allocated: %7d used: %7d\n",
		      maxseqsz * sizeof(CHILD), 
		      (max_common_child - common_child + 1) * sizeof(CHILD));
	      total_alloc += maxseqsz * sizeof(CHILD);
	      total_used += (max_common_child - common_child + 1) * sizeof(CHILD);
    
	      fprintf(debugp,"'seqtab'       bytes allocated: %7d used: %7d\n",
		      maxseqsz * sizeof(NODE *), topseq * sizeof(NODE *));
	      total_alloc += maxseqsz * sizeof(NODE *);
	      total_used += topseq * sizeof(NODE *); 
  
	      fprintf(debugp,"'blockpool'    bytes allocated: %7d used: %7d\n",
		      maxbblsz * sizeof(BBLOCK), 
		      (lastblock - topblock) * sizeof(BBLOCK));
	      total_alloc += maxbblsz * sizeof(BBLOCK);
	      total_used += (lastblock - topblock) * sizeof(BBLOCK); 
  
	      web_bytes_allocated = 4 + max_web * size_web;
	      web_bytes_used = (int)next_web - (int)curr_web_bank;
	      ptr = (char *)first_web_bank;
	      while ((char **)ptr != curr_web_bank)
	        {
	        web_bytes_allocated =  web_bytes_allocated +
					    4 + max_web * size_web;
	        web_bytes_used = web_bytes_used +
					    4 + max_web * size_web;
	        ptr = *((char **)ptr);
	        }
	      ptr = (char *)curr_web_bank;
	      while (*((char **)ptr) != (char *) 0)
	        {
	        web_bytes_allocated =  web_bytes_allocated +
					    4 + max_web * size_web;
	        ptr = *((char **)ptr);
	        }
	      fprintf(debugp,"'web'          bytes allocated: %7d used: %7d\n",
		      web_bytes_allocated, web_bytes_used); 
	      total_alloc += web_bytes_allocated;
	      total_used += web_bytes_used;
  
	      fprintf(debugp,"'regioninfo'   bytes allocated: %7d used: %7d\n",
		      maxregion * sizeof(REGIONINFO), 
		      (lastregioninfo + 1) * sizeof(REGIONINFO));
	      total_alloc += maxregion * sizeof(REGIONINFO);
	      total_used += (lastregioninfo + 1) * sizeof(REGIONINFO);
  
	      du_bytes_allocated = 4 + max_dublock * size_dublock;
	      du_bytes_used = (int)next_dublock - (int)curr_dublock_bank;
	      ptr = (char *)first_dublock_bank;
	      while ((char **)ptr != curr_dublock_bank)
	        {
	        du_bytes_allocated =  du_bytes_allocated +
					    4 + max_dublock * size_dublock;
	        du_bytes_used = du_bytes_used +
					    4 + max_dublock * size_dublock;
	        ptr = *((char **)ptr);
	        }
	      ptr = (char *)curr_dublock_bank;
	      while (*((char **)ptr) != (char *) 0)
	        {
	        du_bytes_allocated =  du_bytes_allocated +
					    4 + max_dublock * size_dublock;
	        ptr = *((char **)ptr);
	        }
	      fprintf(debugp,"'dublocks'     bytes allocated: %7d used: %7d\n",
		      du_bytes_allocated, du_bytes_used); 
	      total_alloc += du_bytes_allocated;
	      total_used += du_bytes_used;
    
	      loadst_bytes_allocated = 4 + max_loadst * size_loadst;
	      loadst_bytes_used = (int)next_loadst - (int)curr_loadst_bank;
	      ptr = (char *)first_loadst_bank;
	      while ((char **)ptr != curr_loadst_bank)
	        {
	        loadst_bytes_allocated =  loadst_bytes_allocated +
					    4 + max_loadst * size_loadst;
	        loadst_bytes_used = loadst_bytes_used +
					    4 + max_loadst * size_loadst;
	        ptr = *((char **)ptr);
	        }
	      ptr = (char *)curr_loadst_bank;
	      while (*(((char **)ptr)) != (char *) 0)
	        {
	        loadst_bytes_allocated =  loadst_bytes_allocated +
					    4 + max_loadst * size_loadst;
	        ptr = *((char **)ptr);
	        }
	      fprintf(debugp,"'loadst'       bytes allocated: %7d used: %7d\n",
		      loadst_bytes_allocated, loadst_bytes_used); 
	      total_alloc += loadst_bytes_allocated;
	      total_used += loadst_bytes_used;
    
	      plink_bytes_allocated = 4 + max_plink * size_plink;
	      plink_bytes_used = (int)next_plink - (int)curr_plink_bank;
	      ptr = (char *)first_plink_bank;
	      while ((char **)ptr != curr_plink_bank)
	        {
	        plink_bytes_allocated =  plink_bytes_allocated +
					    4 + max_plink * size_plink;
	        plink_bytes_used = plink_bytes_used +
					    4 + max_plink * size_plink;
	        ptr = *((char **)ptr);
	        }
	      ptr = (char *)curr_plink_bank;
	      while (*((char **)ptr) != (char *) 0)
	        {
	        plink_bytes_allocated =  plink_bytes_allocated +
					    4 + max_plink * size_plink;
	        ptr = *((char **)ptr);
	        }
	      fprintf(debugp,"'plink'        bytes allocated: %7d used: %7d\n",
		      plink_bytes_allocated, plink_bytes_used); 
	      total_alloc += plink_bytes_allocated;
	      total_used += plink_bytes_used;
    
	      fprintf(debugp,"-------------------------------------------\n");
	      fprintf(debugp,"Total bytes UNUSED: %d\n",
                              total_alloc - total_used);
	      }
	    fprintf(debugp,"Total heap allocated: %d\n",sbrk(0) - initial_sbrk);
	    }
	}



# endif DEBUGGING


/******************************************************************************
*
*	heavytree()
*
*	Description:		Counts the number of non-leaf subnodes (up to
*				a certain maximum above which we don't care).
*
*	Called by:		sr_worthit()
*				worthit()
*				replacecommons()
*
*	Input parameters:	np	A NODE ptr to the top of a subtree.
*
*	Output parameters:	none
*				(returns the count of subtree non-LEAF nodes
*				up to a certain maximum count)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
heavytree(np)	register NODE *np;
{
	register int sum;

	sum = 0;
top:
	switch(optype(np->in.op))
		{
		case BITYPE:
			/* The addition of a COMOP (in order to create a temp
			   assignment) must not suddenly cause the tree
			   to become heavy.
			*/
			if (np->in.op == COMOP)
				break;
			sum++;
			if (ISPTR(np->in.left->in.type) || ISPTR(np->in.right->in.type))
				sum += aheavyop(np->in.op)? COST_BREAKEVEN+1 : 0;
			else
				sum += heavyop(np->in.op)? COST_BREAKEVEN+1 : 0;
			if (sum > COST_BREAKEVEN+1)
				break;
			sum += heavytree(np->in.right);
			if (sum > COST_BREAKEVEN+1)
				break;
			np = np->in.left;
			goto top;
		case UTYPE:
			sum++;
			if (sum > COST_BREAKEVEN+1)
				break;
			np = np->in.left;
			goto top;
		}
	return(sum);
}



/******************************************************************************
*
*	worthit()
*
*       Description:            Because moves to/from memory are
*				relatively expensive on MC68020, some
*				CSEs which seem otherwise worthwhile may
*				in fact cost more to store into a temp
*				and recover than to recalculate.  The
*				worthit calculation below is a crude
*				cost estimator that can be tuned later.
*				It returns YES iff it feels the CSE
*				replacement will result in a code speed
*				improvement. The same algorithm applies
*				to code motion of loop invariants.
*
*	Called by:		rplc()
*				cm_dandidate()
*				minor_cm_candidate()
*
*	Input parameters:	np - NODE pointer pointing to a subtree
*				     under consideration for CSE.
*
*	Output parameters:	none
*
*	Globals referenced:	acount
*				dcount
*				global_disable
*				sr_disable
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
flag worthit(np)	register NODE *np;
{
	register int extra_uses;
	int benefit;

	if (ISFTP(np->in.type) && np->in.op != SCONV && np->in.op != UNARY MUL)
		return(YES);

	benefit = heavytree(np);
	if (ISPTR(np->in.type))
		{
		extra_uses = np->ind.usage - often_count;
		if (extra_uses < 0)
			extra_uses = 0;
		if (benefit)
			benefit += extra_uses; /* Should it be *= ? */
		if (extra_uses > 0) ;	/* Do nothing */
		else if ((global_disable || sr_disable) &&
			(acount < AREG_BREAKPOINT))
			/* Strength reduction will usually do a better
			   job for those marginal cases involving A
			   registers (indexed mode array refs, usually).
			*/
			benefit++;
		}
	else
		if (dcount < DREG_BREAKPOINT)
			benefit++;
	return(benefit > COST_BREAKEVEN+1);
}

/******************************************************************************
*
*	haseffects()
*
*	Description:		haseffects() checks the operand to determine if
*				possible side effects are requested. It returns
*				NO if no side effects and YES otherwise.
*
*	Called by:		oreg2plus()
*
*	Input parameters:	p - a NODE ptr to a subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
flag haseffects(p)	NODE *p;
{
	short o = p->in.op;
	short ty = optype(o);

	if (ty == LTYPE) return (NO);
	if ( asgop(o) || (callop(o) && !p->in.safe_call) ) return (YES);
	if ( haseffects(p->in.left) ) return (YES);
	if (ty == UTYPE) return (NO);
	return( haseffects(p->in.right) );
}

/******************************************************************************
*
*	oreg2plus()
*
*	Description:		a tree walking routine that rewrites
*				subexpressions into forms that do not contain
*				implicit assignments. It also rewrites some
*				subexpressions into OREGS if appropriate
*				and does other largely machine independent
*				rewriting that's convenient for c1.
*
*	Called by:		main()
*
*	Input parameters:	p - a NODE ptr to a candidate subexpression.
*				treetop
*				arrayform - flag to propagate downwards for 
*	`				later use in the register allocator.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
void oreg2plus( p, treetop, arrayform )
register NODE *p; 
flag treetop;
flag arrayform;
{


	register NODE *q;
	register char *cp;
	register NODE *ql, *qr;
	int	r;
	CONSZ temp;

	if (p->in.arrayref)
		arrayform = YES;

	switch(p->in.op)
		{
		case INCR:
		case DECR:
			if (haseffects(p->in.left))
				/* Leave as INCR/DECR */
				break;
			if (treetop)
				{
				qr = talloc();
				*qr = *p;
				qr->in.op = (p->in.op == INCR) ? PLUS : MINUS;
				qr->in.left = tcopy(p->in.left);
				qr->in.left->in.lhsiref = NO;
				p->in.right = qr;
				p->in.op = ASSIGN;
				}
			else
				{
				qr = tcopy(p);
				qr->in.op = (p->in.op==INCR)? PLUS : MINUS;
				qr->in.left->in.lhsiref = NO;
				p->in.left = block(ASSIGN, p->in.left, qr, 0,
						p->in.type);
				p->in.op = (p->in.op==INCR)? MINUS : PLUS;
				}
			break;
		case ASG PLUS:
		case ASG MINUS:
		case ASG MUL:
		case ASG DIV:
		case ASG MOD:
		case ASG AND:
		case ASG OR:
		case ASG ER:
		case ASG LS:
		case ASG RS:
		case ASG DIVP2:
			if (haseffects(p->in.left))
				/* Leave as ASGOP */
				break;
			qr = talloc();
			*qr = *p;
			qr->in.op -= ASG 0;		/* NOASG the op */
			qr->in.left = tcopy(p->in.left);
			qr->in.left->in.lhsiref = NO;
			p->in.right = qr;
			p->in.op = ASSIGN;
			break;

		case CM:
			if (p->in.left->in.op != CM && p->in.left->in.op != UCM)
				{
				ql = talloc();
				ql->in.op = UCM;
				ql->in.type = p->in.type;
				ql->in.left = p->in.left;
				p->in.left = ql;
				}
			break;

		case CALL:
			break;

		case UNARY MUL:
			{
			/* look for situations where we can turn * into OREG */
			q = p->in.left;
			if( q->in.op == REG )
				{
				temp = q->tn.lval;
				r = q->tn.rval;
				cp = q->tn.name;
				goto ormake;
				}
	
			if( q->in.op != PLUS && q->in.op != MINUS )
				break;
			ql = q->in.left;
			qr = q->in.right;
	
			if( qr->in.op == ICON && ql->in.op==REG )
				{
				temp = qr->tn.lval;
				if( q->in.op == MINUS )
					temp = -temp;
				r = ql->tn.rval;
				temp += ql->tn.lval;
				cp = qr->tn.name;
				if( cp && ( q->in.op == MINUS || ql->tn.name ) )
					break;
				if( !cp )
					cp = ql->tn.name;
	
ormake:
				if( NOTOFF( p->in.type, r, temp, cp ) )
					break;
				p->in.op = OREG;
				p->tn.rval = r;
				p->tn.lval = temp;
				p->tn.name = cp;	/* just copying the ptr */
				tfree(q);
				break;
				}
			}
		case ICON:
			p->tn.no_iconsts = arrayform;
			break;
		}

	switch(optype(p->in.op))
		{
		case LTYPE:
			return;
		case UTYPE:
			oreg2plus( p->in.left, NO, arrayform );
			return;
		case BITYPE:
			oreg2plus( p->in.left, (p->in.op == COMOP) ? YES:NO,
				arrayform);
			oreg2plus( p->in.right, NO, arrayform );
			return;
		}
}



/******************************************************************************
*
*	fix_array_form()
*
*	Description:		fix_array_form() canonicalizes the tree into
*				shapes most convenient for the register
*				allocator.
*
*	Called by:		recognize_array()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	baseoff
*
*	Globals modified:	none
*				(subtree topped by np may be altered)
*
*	External calls:		none
*
*******************************************************************************/
void fix_array_form(topnode)
NODE *topnode;
{
    NODE *topplusnode;
    register NODE *p;
    register NODE *lp;
    register NODE *rp;
    NODE *base;
    NODE *subs;

    if (topnode->in.op == UNARY MUL)
	topplusnode = topnode->in.left;
    else
	topplusnode = topnode;

    switch(topplusnode->in.op)
	{
	case PLUS:
		break;
	default:
		topnode->in.isarrayform = NO;
		/* fall thru */
	case NAME:
	case OREG:
        case ICON:
                return;
        case FOREG:
                if (topnode->in.op == UNARY MUL)
                        node_constant_collapse(topnode);
                return;
	}

    if (LO(topplusnode) == NAME && RO(topplusnode) == ICON
	&& topplusnode->in.right->tn.arraybaseaddr)
		{
		/* Canonicalize */
		NODE *sp;
		SWAP(topplusnode->in.left, topplusnode->in.right)
		}

    if (LO(topplusnode) == NAME)
	{
	topplusnode->in.arrayref = NO;
	topnode->in.isarrayform = NO;
	return;
	}

    p = topplusnode;

    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;
	if (ISPTR(lp->in.type)
	 && ((lp->in.op == ICON) || (lp->in.op == FOREG) || (lp->in.op == NAME)
	  || (lp->in.op == OREG) || (lp->in.op == REG)))
	    {
	    break;
	    }
	if (ISPTR(rp->in.type)
	 && ((rp->in.op == ICON) || (rp->in.op == FOREG) || (rp->in.op == NAME)
	  || (rp->in.op == OREG) || (rp->in.op == REG)))
	    {
	    /* canonicalize */
	    p->in.right = lp;
	    p->in.left = rp;
	    break;
	    }
	if (ISPTR(rp->in.type)
	 && ((rp->in.op == ICON) || (rp->in.op == FOREG) || (rp->in.op == NAME)
	  || (rp->in.op == OREG)))
	    {
	    /* canonicalize */
	    p->in.right = lp;
	    p->in.left = rp;
	    break;
	    }
	if ((lp->in.op == PLUS) && ISPTR(lp->in.type))
	    {
	    p = lp;
	    }
	else if ((rp->in.op == PLUS) && ISPTR(rp->in.type))
	    {
	    /* canonicalize */
	    p->in.right = lp;
	    p->in.left = rp;
	    p = rp;
	    }
	else
#pragma BBA_IGNORE
	    cerror("lost in fix_array_form()");		/* no good */
	}

    /* collect base and subscripts */
    base = NULL;
    subs = NULL;

    p = topplusnode;

    while (1)
	{
	lp = p->in.left;
	rp = p->in.right;
	if (p != topplusnode)
	    p->in.op = FREE;	/* throw it away */

	if (subs == NULL)
	    subs = rp;
	else
	    subs = block( PLUS, rp, subs, INT);
	if (ISPTR(lp->in.type)
	 && ((lp->in.op == ICON) || (lp->in.op == FOREG) || (lp->in.op == NAME)
	  || (lp->in.op == OREG) || (lp->in.op == REG)))
	    {
	    base = lp;
	    break;
	    }
	else
	    p = lp;
	}

    /* write it as a tree */
    p = topplusnode;
    p->in.left = base;
    p->in.right = subs;

    /* extract constant subscripts */
    extract_constant_subtree(p, base->tn.lval);
}

/******************************************************************************
*
*	uerror()
*
*	Description:		reports fatal user errors.
*
*	Called by:		process_name_icon()
*				process_oreg_foreg()
*
*	Input parameters:	s - a string error message
*				a,b,c - used by s
*
*	Output parameters:	none
*
*	Globals referenced:	stderr
*				procname - the procedure name
*
*	Globals modified:	none
*
*	External calls:		fprintf()
*				exit()
*
*******************************************************************************/
	/* VARARGS1 */
void uerror( s, a, b, c ) char *s;
{
	if( nerrors && nerrors <= MAXUSEFULERRS )
		{ 
		/* give the compiler the benefit of the doubt */
		fprintf( stderr, "Global optimizer: Cannot recover from earlier errors: goodbye!\n" );
		}
	else 
		{
		if (procname)
			fprintf( stderr, "Global optimizer found error in \"%s\": ",
				procname);
		else
			fprintf( stderr, "Global optimizer found error in command line: ");
		fprintf( stderr, s, a, b, c );
		fprintf( stderr, "\n" );
		}
	exit(1);
}

/******************************************************************************
*
*	cerror()
*
*	Description:		reports fatal internal errors
*
*	Called by:		everyone
*
*	Input parameters:	s - a string error message
*				a,b,c - as needed by s
*
*	Output parameters:	none
*
*	Globals referenced:	stderr
*				procname - the current procedure name
*
*	Globals modified:	none
*
*	External calls:		fprintf()
*				exit()
*
*******************************************************************************/
#pragma BBA_IGNORE
	/* VARARGS1 */
void cerror( s, a, b, c ) char *s; { /* compiler error: die */
	if( nerrors && nerrors <= MAXUSEFULERRS ){ 
		/* give the compiler the benefit of the doubt */
		fprintf( stderr, "C1: Cannot recover from earlier errors: goodbye!\n" );
		}
	else {
		if (procname)
			fprintf( stderr, "C1 internal error in \"%s\": ",
				procname);
		else
			fprintf( stderr, "C1 internal error in command line: ");
#ifdef DEBUGGING
		fprintf( stderr, s, a, b, c );
#else
		if (fortran)
		  fprintf( stderr, "Compile routine with $OPTIMIZE LEVEL1 ON.");
		else
		  fprintf( stderr, "Compile routine with #pragma OPT_LEVEL 1.");
#endif
		fprintf( stderr, "\n" );
		}
	exit(1);
	}

/******************************************************************************
*
*	werror()
*
*	Description:		Puts out a non fatal warning about a user
*				problem.
*
*	Called by:		process_name_icon()
*				process_oreg_foreg()
*				delete_unreached_block()
*				setup_global_data()
*				uninitialized_var()
*				analyze_do_loop()
*				analyze_for_loop()
*				copy_lblocks()
*				conval()
*				optim()
*
*	Input parameters:	s - a message string to be used as a printf
*					format.
*				a,b - optional parameters to be used only if
*					specified in s.
*
*	Output parameters:	none
*
*	Globals referenced:	warn_disable
*				procname
*				stderr
*
*	Globals modified:	none
*
*	External calls:		fprintf()
*
*******************************************************************************/
	/* VARARGS1 */
werror( s, a, b ) char *s; {  /* warning */
	if (! warn_disable)
		{
		if (procname)
			fprintf( stderr, "Global optimizer warning in \"%s\": ",
				procname);
		else
			fprintf( stderr, "Global optimizer warning in command line: ");
		fprintf( stderr, s, a, b );
		fprintf( stderr, "\n" );
		}
	}


/******************************************************************************
*
*	tinit()
*
*	Description:		Initializes all the NODEs to FREE.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	maxnodesz
*				taz
*
*	Globals modified:	lastfree
*				treeasciz
*				lasttac
*
*	External calls:		none
*
*******************************************************************************/
void tinit(){

	register NODE *p, *end;

	for( p=node, end = &node[maxnodesz-1]; p<= end; ++p ) p->in.op = FREE;
	lastfree = node;
	treeasciz = taz;
	lasttac = treeasciz;

	}


/******************************************************************************
*
*	proctinit()
*
*	Description:		Re-initialize expression nodes and tasciz table
*				for next proc.
*
*	Called by:		funcinit()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced: 	node
*
*	Globals modified:	taz
*				treeasciz
*				lasttac
*				lastfree
*
*	External calls:		free()	{FREEIT}
*
*******************************************************************************/
void proctinit()
{
    char *tasciztable;
    NODE *nodetable;

    while (tasciztable = *(((char **)taz) - 1))
	{
	FREEIT(((long *)taz) - 1);
	taz = tasciztable;
	}
    treeasciz = taz;
    lasttac = treeasciz;

    while (nodetable = *(((NODE **)node) - 1))
	{
	FREEIT(((long *)node) - 1);
	node = nodetable;
	}
    lastfree = node;
}


/******************************************************************************
*
*	talloc()
*
*	Description:		gets a FREE node from the reservoir and
*				initializes it. If no FREE ones are available,
*				it expands the reservoir.
*
*	Called by:		everywhere
*
*	Input parameters:	none
*
*	Output parameters:	none
*				(returns a ptr to the initialized FREE node)
*
*	Globals referenced:	maxnodesz
*
*	Globals modified:	lastfree
*				node
*
*	External calls:		ckalloc()
*
*******************************************************************************/

NODE *talloc(){
	register NODE *p, *q;
	NODE *newnodes;
	static struct allo clean = {FREE, {UNDEF,0,0}, NULL, 0, NO_STMTNO, 
					NULL, NULL, NULL, 0,0};

again:
	q = lastfree;
	for( p = TNEXT(q); p!=q; p= TNEXT(p))
		if( p->in.op == FREE )
			{
			(*p).allo = clean;
			return(lastfree=p);
			}

	/* out of tree nodes -- allocate another block of nodes */

	newnodes = (NODE *) ckalloc ( 4 + maxnodesz * sizeof (NODE) );
	*((NODE **) newnodes) = node;	/* backpointer to previous table */
	node = newnodes;
	((long *)node) = ((long *) node) + 1;	/* advance to start */
	for( p=node, q = &node[maxnodesz-1]; p<= q; ++p ) p->in.op = FREE;
	lastfree = node;
	goto again;
	/* NOTREACHED */
	}



/******************************************************************************
*
*	tfree1()
*
*	Description:		Frees a node if the ptr is not NULL.
*				Checks to ensure the pointer is valid first.
*
*	Called by:		tfree()
*				tfree2()
*
*	Input parameters:	p - a ptr to a NODE.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		cerror()
*
*******************************************************************************/
void tfree1(p)  NODE *p; {
	if( p == 0 ) 
#pragma BBA_IGNORE
		cerror( "freeing blank tree!");
	else p->in.op = FREE;
	}



/******************************************************************************
*
*	tfree()
*
*	Description:		Frees the tree pointed to by p unconditionally.
*
*	Called by:		everwhere
*
*	Input parameters:	p - a NODE ptr to the top of a subtree.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		walkf()
*				tfree1()
*
*******************************************************************************/
void tfree( p )  NODE *p;
{
	if( p->in.op != FREE ) walkf( p, tfree1 );
}





/******************************************************************************
*
*	tfree2()
*
*	Description:		Free the tree p, taking due care not to free
*				subtrees marked as common_nodes.
*
*	Called by:		redundant()
*
*	Input parameters:	p - a NODE ptr to the subtree to be freed.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	w_halt
*				fwalk2_bitmask
*
*	External calls:		tfree1()
*				fwalk2()
*
*******************************************************************************/
void tfree2( p )  NODE *p; {

	w_halt = 0;
	if( (p->in.op != FREE) && !p->in.common_node )
		FWALK2( p, tfree1, COMMON_NODE);
	}




/******************************************************************************
*
*	bcon()
*
*	Description:		Make a constant node with value i.
*
*	Called by:		extract_constant_subtree()
*				makepow2()
*				add_indvars()
*				set_cd()
*				minor_aiv2()
*				detect_indvars()
*				reg_check_plus_array_form()
*				xform_do_loop_to_dbra()
*				xform_loop_to_straight_line()
*
*	Input parameters:	i - the value of the node
*				tbase - the type of the node
*
*	Output parameters:	none
*				(returns a NODE ptr to the constant node)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		block()
*
*******************************************************************************/
NODE *bcon( i, tbase )	register int i; TWORD tbase;
{
	register NODE *p;

	p = block( ICON, NIL, NIL, INT);
	p->tn.type.base = tbase;
	p->tn.type.mods1 = 0;	/* whoever made a bcon of complex type? */
	p->tn.type.mods2 = 0;
	p->tn.lval = i;
	p->tn.name = NULL;
	return( p );
}




/* varargs */
NODE * block( o, l, r, tbase, tt) short o; NODE *l, *r; TWORD tbase; TTYPE tt;
{

	register NODE *p;

	p = talloc();
	p->in.op = o;
	p->in.left = l;
	p->in.right = r;
	if (tbase)
		{
		p->in.type.base = tbase;
		}
	else
		p->in.type = tt;
	return(p);
}


/* fwalk2() is a prefix tree traversal routine that visits nodes in the order
   P - L - R. It stops when w_halt is TRUE.
*/
fwalk2( t, f) register NODE *t; int (*f)(); 
{
	ushort opty;

more:
	opty = optype (t->in.op);
	if ( !w_halt )
		{
		(*f)(t);
		switch( opty )
			{
			case BITYPE:
				w_halt = t->in.left->allo.flagfiller & fwalk2_bitmask;
				fwalk2( t->in.left, f);
				t = t->in.right;
				w_halt = t->allo.flagfiller & fwalk2_bitmask;
				goto more;
	
			case UTYPE:
				t = t->in.left;
				w_halt = t->allo.flagfiller & fwalk2_bitmask;
				goto more;

			default:
				break;
			}
		}
}
fwalkc( t, f) register NODE *t; int (*f)(); 
{
	ushort opty;

more:
	opty = optype (t->in.op);
	if ( !w_halt )
		{
		(*f)(t);
		switch( opty )
			{
			case BITYPE:
				w_halt = t->in.left->allo.flagfiller & fwalk2_bitmask;
				t->in.left->allo.flagfiller &= fwalk2_clrmask;
				fwalkc( t->in.left, f);
				t = t->in.right;
				w_halt = t->allo.flagfiller & fwalk2_bitmask;
				t->allo.flagfiller &= fwalk2_clrmask;
				goto more;
	
			case UTYPE:
				t = t->in.left;
				w_halt = t->allo.flagfiller & fwalk2_bitmask;
				t->allo.flagfiller &= fwalk2_clrmask;
				goto more;

		    default:
				break;
			}
		}
}



/* fwalk() is a prefix tree traversal routine that visits nodes in the order
   P - L - R. General purpose.
*/
fwalk( t, f, down ) register NODE *t; int (*f)(); {

	int down1, down2;

more:
	down1 = 0; down2 = 0;

	(*f)( t, down, &down1, &down2 );

	switch( optype( t->in.op ) ){

	case BITYPE:
		fwalk( t->in.left, f, down1 );
		t = t->in.right;
		down = down2;
		goto more;

	case UTYPE:
		t = t->in.left;
		down = down1;
		goto more;

	  default:
		break;
		}
	}



/* walkf() is a postfix tree traversal routine that traverses the tree in the
   order L - R - P.
*/
walkf( t, f ) register NODE *t;  void (*f)(); {
	register short opty;

	opty = optype(t->in.op);

	if( opty != LTYPE ) walkf( t->in.left, f );
	if( opty == BITYPE ) walkf( t->in.right, f );
	(*f)( t );
	}




struct dopest {
	short dopeop;
	char opst[8];
	int dopeval;
	int saveA;
	int saveD;
	int savef8;
	int saved8;
	int savefF;
	int savedF;
	} indope[] = {

	NAME, "NAME", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	REG, "REG", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	OREG, "OREG", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	FOREG, "FOREG", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	ICON, "ICON", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	FCON, "FCON", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	CCODES, "CCODES", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	NOCODEOP, "NOCODEOP", LTYPE|INVFLG|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	FENTRY, "FENTRY", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,/* never entered into a tree */
	UNARY MINUS, "U-", UTYPE|FLOFLG, 0,8,19,25,10,20,
	UNARY MUL, "U*", UTYPE, 4,4,0,0,0,0,
	UNARY AND, "U&", UTYPE, 0,0,0,0,0,0,
	UNARY CALL, "UCALL", UTYPE|CALLFLG|HEAVYFLG|AHEAVYFLG|NCOMMONFLG, 0,0,0,0,0,0,
	NOT, "!", UTYPE|LOGFLG|AHEAVYFLG, 0,9,0,0,0,0,
	COMPL, "~", UTYPE|AHEAVYFLG, 0,9,0,0,0,0,
	FORCE, "FORCE", UTYPE, 0,7,-80,-86,0,0,
	INIT, "INIT", UTYPE, 0,0,0,0,0,0,
	SCONV, "SCONV", UTYPE|FHWFLG|FLOFLG, 0,5,52,58,10,20,
	PLUS, "+", BITYPE|FLOFLG|SIMPFLG|COMMFLG|FHWFLG, 7,7,21,27,10,20,
	ASG PLUS, "+=", BITYPE|ASGFLG|ASGOPFLG|FLOFLG|SIMPFLG|COMMFLG, 7,7,21,27,10,20,
	MINUS, "-", BITYPE|FLOFLG|SIMPFLG|FHWFLG, 7,7,21,27,10,20,
	ASG MINUS, "-=", BITYPE|FLOFLG|SIMPFLG|ASGFLG|ASGOPFLG, 7,7,21,27,10,20,
	MUL, "*", BITYPE|FLOFLG|FHWFLG|HEAVYFLG|AHEAVYFLG, 0,4,21,27,10,20,
	ASG MUL, "*=", BITYPE|FLOFLG|ASGFLG|ASGOPFLG|HEAVYFLG|AHEAVYFLG, 0,4,21,27,10,20,
	AND, "&", BITYPE|SIMPFLG|COMMFLG|LTYFLG|AHEAVYFLG, 0,7,0,0,0,0,
	ASG AND, "&=", BITYPE|SIMPFLG|COMMFLG|ASGFLG|ASGOPFLG|AHEAVYFLG, 0,7,0,0,0,0,
	QUEST, "?", BITYPE|NODELAYFLG|HEAVYFLG|AHEAVYFLG, 0,0,0,0,0,0,
	COLON, ":", BITYPE|NODELAYFLG|HEAVYFLG|AHEAVYFLG, 0,0,0,0,0,0,
	ANDAND, "&&", BITYPE|LOGFLG|NODELAYFLG|AHEAVYFLG, 0,0,0,0,0,0,
	OROR, "||", BITYPE|LOGFLG|NODELAYFLG|AHEAVYFLG, 0,0,0,0,0,0, 
	SEMICOLONOP, ";", BITYPE|NCOMMONFLG, 0,0,0,0,0,0,
	UNARY SEMICOLONOP, "U;", UTYPE|NCOMMONFLG, 0,0,0,0,0,0,
	DBRA, "DBRA", BITYPE, 0,0,0,0,0,0,
	CM, ",", BITYPE|NCOMMONFLG|NODELAYFLG, 0,0,0,0,0,0,
	UCM, "U,", UTYPE|NCOMMONFLG|NODELAYFLG, 0,0,0,0,0,0, 
	COMOP, ",OP", BITYPE|NODELAYFLG|NCOMMONFLG, 0,0,0,0,0,0, 
	ASSIGN, "=", BITYPE|ASGFLG|HEAVYFLG|AHEAVYFLG, 3,3,80,86,10,20,
	DIV, "/", BITYPE|FLOFLG|DIVFLG|FHWFLG|HEAVYFLG|AHEAVYFLG, 0,3,21,27,10,20,
	ASG DIV, "/=", BITYPE|FLOFLG|DIVFLG|ASGFLG|ASGOPFLG|HEAVYFLG|AHEAVYFLG, 0,3,21,27,10,20,
	MOD, "%", BITYPE|DIVFLG|HEAVYFLG|AHEAVYFLG, 0,3,19,25,10,20,
	ASG MOD, "%=", BITYPE|DIVFLG|ASGFLG|ASGOPFLG|HEAVYFLG|AHEAVYFLG, 0,3,19,25,10,20,
	LS, "<<", BITYPE|SHFFLG|AHEAVYFLG, 0,3,-80,-86,-10,-20,
	ASG LS, "<<=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG|AHEAVYFLG, 0,3,-160,-172,-20,-40,
	RS, ">>", BITYPE|SHFFLG|AHEAVYFLG, 0,0,-80,-86,-10,-20,
	ASG RS, ">>=", BITYPE|SHFFLG|ASGFLG|ASGOPFLG|AHEAVYFLG, 0,3,-160,-172,-20,-40,
	OR, "|", BITYPE|COMMFLG|SIMPFLG|LTYFLG|AHEAVYFLG, 0,3,-80,-86,-10,-20,
	ASG OR, "|=", BITYPE|COMMFLG|SIMPFLG|ASGFLG|ASGOPFLG|AHEAVYFLG, 0,3,-160,-172,-20,-40,
	ER, "^", BITYPE|COMMFLG|AHEAVYFLG, 0,3,-80,-86,-10,-20,
	ASG ER, "^=", BITYPE|COMMFLG|ASGFLG|ASGOPFLG|AHEAVYFLG, 0,3,-160,-172,-20,-40,
	INCR, "++", BITYPE|ASGFLG|FLOFLG, 3,3,19,25,10,20,
	DECR, "--", BITYPE|ASGFLG|FLOFLG, 3,3,19,25,10,20,
	CALL, "CALL", BITYPE|CALLFLG|HEAVYFLG|AHEAVYFLG|NCOMMONFLG, 3,3,-74,-74,0,0,
	EQ, "==", BITYPE|LOGFLG, 3,3,19,25,10,20,
	NE, "!=", BITYPE|LOGFLG, 3,3,19,25,10,20,
	LE, "<=", BITYPE|LOGFLG, 3,3,19,25,10,20,
	LT, "<", BITYPE|LOGFLG, 3,3,19,25,10,20,
	GE, ">=", BITYPE|LOGFLG, 3,3,19,25,10,20,
	GT, ">", BITYPE|LOGFLG, 3,3,19,25,10,20,
	UGT, "UGT", BITYPE|LOGFLG, 3,3,-80,-86,-10,-20,
	UGE, "UGE", BITYPE|LOGFLG, 3,3,-80,-86,-10,-20,
	ULT, "ULT", BITYPE|LOGFLG, 3,3,-80,-86,-10,-20,
	ULE, "ULE", BITYPE|LOGFLG, 3,3,-80,-86,-10,-20,
	ARS, "A>>", BITYPE|AHEAVYFLG, 0,0,0,0,0,0,
	CBRANCH, "CBRANCH", BITYPE|HEAVYFLG|AHEAVYFLG|INVFLG, 3,3,0,0,0,0,
	RETURN, "RETURN", BITYPE|ASGFLG|ASGOPFLG|HEAVYFLG|AHEAVYFLG, 0,0,0,0,0,0,
	CAST, "CAST", BITYPE|ASGFLG|ASGOPFLG|FHWFLG, 0,0,0,0,0,0,
	GOTO, "GOTO", UTYPE, 0,0,0,0,0,0,
	FMONADIC, "FMONADIC", BITYPE, 0,0,19,25,-10,-20,
	DIVP2, "/2", BITYPE|DIVFLG|FHWFLG|HEAVYFLG|AHEAVYFLG, 0,3,21,27,10,20,
	ASM, "ERROR", BITYPE, 0,0,0,0,0,0,
	ASG DIVP2, "/=2", BITYPE|DIVFLG|ASGFLG|ASGOPFLG|HEAVYFLG|AHEAVYFLG, 0,3,21,27,10,20,
	LGOTO, "LGOTO", LTYPE|INVFLG|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	FCOMPGOTO, "FCGOTO", LTYPE|INVFLG|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	STRING, "STRING", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	STREF, "->", BITYPE|HEAVYFLG|AHEAVYFLG, 0,0,0,0,0,0,
	FLD, "FLD", UTYPE|HEAVYFLG|AHEAVYFLG|NCOMMONFLG, 0,0,0,0,0,0,
	STASG, "STASG", BITYPE|ASGFLG|HEAVYFLG|AHEAVYFLG|NCOMMONFLG, 0,0,0,0,0,0,
	STARG, "STARG", UTYPE|HEAVYFLG|AHEAVYFLG, 0,0,0,0,0,0,
	STCALL, "STCALL", BITYPE|CALLFLG|HEAVYFLG|AHEAVYFLG|NCOMMONFLG, 0,0,0,0,0,0,
	UNARY STCALL, "USTCALL", UTYPE|CALLFLG|HEAVYFLG|AHEAVYFLG|NCOMMONFLG, 0,0,0,0,0,0,
	EXITBRANCH, "EXIT", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,
	EQV, "EQV", BITYPE|COMMFLG, 0,3,-80,-86,-10,-20,
	NEQV, "NEQV", BITYPE|COMMFLG, 0,3,-80,-86,-10,-20,
	FREE, "FREE", LTYPE|NCOMMONFLG|NOSEQFLG, 0,0,0,0,0,0,

	PAREN, "()", UTYPE, 4,4,0,0,0,0,
-1,	0
};

mkdope(){
	register struct dopest *q;
	register int i;

	for( q = indope; q->dopeop >= 0; ++q ){
		i = q->dopeop;
		dope[i] = q->dopeval;
		opst[i] = q->opst;
		reg_savings[i][REG_A_SAVINGS] = q->saveA;
		reg_savings[i][REG_D_SAVINGS] = q->saveD;
		reg_savings[i][REG_881_FLOAT_SAVINGS] = q->savef8;
		reg_savings[i][REG_881_DOUBLE_SAVINGS] = q->saved8;
		reg_savings[i][REG_FPA_FLOAT_SAVINGS] = q->savefF;
		reg_savings[i][REG_FPA_DOUBLE_SAVINGS] = q->savedF;
		}
 	if (mc68040)
          {
	  reg_store_cost[REG_A_SAVINGS] = 1;
	  reg_store_cost[REG_D_SAVINGS] = 1;
	  reg_store_cost[REG_881_FLOAT_SAVINGS] = 5;
	  reg_store_cost[REG_881_DOUBLE_SAVINGS] = 5;
	  reg_save_restore_cost[REG_A_SAVINGS] = 1;
	  reg_save_restore_cost[REG_D_SAVINGS] = 1;
	  reg_save_restore_cost[REG_881_FLOAT_SAVINGS] = 8;  /*2 fmovem w/ 2*/
	  reg_save_restore_cost[REG_881_DOUBLE_SAVINGS] = 8;
          }
	else
	  {
	  reg_store_cost[REG_A_SAVINGS] = 5;
	  reg_store_cost[REG_D_SAVINGS] = 5;
	  reg_store_cost[REG_881_FLOAT_SAVINGS] = 80;
	  reg_store_cost[REG_881_DOUBLE_SAVINGS] = 86;
	  reg_save_restore_cost[REG_A_SAVINGS] = 5;
	  reg_save_restore_cost[REG_D_SAVINGS] = 5;
	  reg_save_restore_cost[REG_881_FLOAT_SAVINGS] = 46;  /*2 fmovem w/ 2*/
	  reg_save_restore_cost[REG_881_DOUBLE_SAVINGS] = 46;
	  }
	reg_store_cost[REG_FPA_FLOAT_SAVINGS] = 10;
	reg_store_cost[REG_FPA_DOUBLE_SAVINGS] = 20;
	reg_save_restore_cost[REG_FPA_FLOAT_SAVINGS] = 20;
	reg_save_restore_cost[REG_FPA_DOUBLE_SAVINGS] = 20;
	}

# ifdef DEBUGGING
tprint( t )  TTYPE t; {/* output a nice description of the type of t */

	ushort lt;

	static char * tnames[] = {
		"undef",
		"farg",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"uchar",
		"ushort",
		"unsigned|widechar",
		"ulong",
		"void",
		"longdouble",
		"schar",
		"labty",
		"signed",
		"const",
		"volatile",
		"tnull",
		"?", "?"
		};

	if (t.base & 0xC000)
		fprintf(debugp, "... " );
	for(lt = t.base&0x3FFF;; lt = BDECREF(lt)){
		if (ISVOL(lt))
			{
			fprintf(debugp, "VOLATILE ");
			lt &= 0xFFF;
			}
		if( (lt&TMASK) == PTR ) fprintf(debugp, "PTR " );
		else if( (lt&TMASK) == FTN ) fprintf(debugp, "FTN " );
		else if( (lt&TMASK) == ARY ) fprintf(debugp, "ARY " );
		else if( lt >= UNDEF && lt <= TNULL )
			{
			fprintf(debugp, "%s", tnames[lt] );
			return;
			}
		else
			cerror("unknown type in tprint(): 0x%x\n",lt);
		}
}
# endif DEBUGGING








/* addtreeasciz adds an asciz symbol to the treeasciz array after first
   checking for overflow. It updates lasttac to point to the next free
   char. The routine is typically used to fill tree node names in the
   second pass routines.
*/

char	*addtreeasciz(cp)	char	*cp;
{
	register short i;
	register char	*lcp = cp;
	char *newtaz;

	for (i=1; *lcp++; i++) /* NULL */ ;	/* count the chars */
	lcp = lasttac;				/* save it */
	if (i > (maxtascizsz - (lasttac-treeasciz) - 4))
		{
		newtaz =  (char *) ckalloc ( maxtascizsz * sizeof(char) );
		*((char **) newtaz) = taz;  /* backpointer to previous table */
		taz = newtaz;
		taz = taz + 4;			/* advance to start */
		lcp = lasttac = treeasciz = taz;
		}
	lasttac += i;
	return ( strcpy(lcp, cp) );
}





/* addexternasciz adds an asciz symbol to the externasciz array after first
   checking for overflow. It updates lasteac to point to the next free
   char. The routine is typically used to fill external symbol names in
   the symbol table.
*/

char	*addexternasciz(cp)	char	*cp;
{
	register short i;
	register char	*lcp = cp;
	char *neweaz;

	for (i=1; *lcp++; i++) /* NULL */ ;	/* count the chars */
	lcp = lasteac;				/* save it */
	if (i > (maxeascizsz - (lasteac-externasciz) - 4))
		{
		neweaz =  (char *) ckalloc ( maxeascizsz * sizeof(char) );
		*((char **) neweaz) = eaz - 4;  /* backpointer to previous table */
		eaz = neweaz;
		eaz = eaz + 4;			/* advance to start */
		lcp = lasteac = externasciz = eaz;
		}
	lasteac += i;
	return ( strcpy(lcp, cp) );
}





many(s, c)
char *s, c;
{
	fprintf(stderr,"%s table overflow. Try the -W1,-N%c option.\n", s, c);	
	exit(1);
}




pointr clralloc(n)
int n;	/* total # of (CLEARED) bytes needed in the contiguous space */
{
	register pointr p;

#ifdef fast_malloc
	if (n <= 0) n = 1;	/* fast malloc won't allocate 0 bytes */
#endif fast_malloc

	p = (pointr) calloc(n, sizeof(char));
	if (p)
		{
# ifdef DEBUGGING
		if (mdebug)
			fprintf(debugp, "clralloc(%d) returns 0x%x\n", n, p);
# endif DEBUGGING
		return (p);
		}
	else
#pragma BBA_IGNORE
	uerror("out of space - unable to allocate memory for internal use");
}


pointr ckalloc(n)
int n;	/* total # of bytes needed in the contiguous space */
{
	register pointr p;

#ifdef fast_malloc
	if (n <= 0) n = 1;	/* fast malloc won't allocate 0 bytes */
#endif fast_malloc

	p = (pointr) malloc(n);
	if (p)
		{
# ifdef DEBUGGING
		if (mdebug)
			fprintf(debugp, "ckalloc(%d) returns 0x%x\n", n, p);
# endif DEBUGGING
		return (p);
		}
	else
#pragma BBA_IGNORE
	uerror("out of space - unable to allocate memory for internal use");
}


pointr ckrealloc(oldp,n)
char *oldp;	/* old pointer */
long n;		/* total # of bytes needed in the contiguous space */
{
	register pointr p;

#ifdef fast_malloc
	if (n <= 0) n = 1;	/* fast malloc won't allocate 0 bytes */
#endif fast_malloc

	p = (pointr) realloc(oldp, n);
	if (p)
		{
# ifdef DEBUGGING
		if (mdebug)
			fprintf(debugp, "ckrealloc(oldp=0x%x, %d) returns 0x%x\n", 
				oldp, n, p);
# endif DEBUGGING
		return (p);
		}
	else
#pragma BBA_IGNORE
	uerror("out of space - unable to allocate memory for internal use");
}


inittaz()
{
	labvals = (unsigned *) ckalloc(maxlabstksz * sizeof(unsigned));
	lstackp = labvals - 1;
	taz =  (char *) ckalloc ( maxtascizsz * sizeof(char) );
	*((long *) taz) = 0;		/* backpointer to previous table */
	taz = taz + 4;			/* advance to start */

	if (!fortran)
		{
		eaz =  (char *) ckalloc ( maxeascizsz * sizeof(char) );
		*((long *) eaz) = 0;	/* backpointer to previous table */
		eaz = eaz + 4;		/* advance to start */
		externasciz = eaz;
		lasteac = externasciz;
		}

	comtab = (unsigned *) ckalloc(comtsize * sizeof(unsigned));
	fargtab = (unsigned *) ckalloc(fargtsize * sizeof(unsigned));
	ptrtab = (unsigned *) ckalloc(ptrtsize * sizeof(unsigned));

	node = (NODE *) ckalloc ( 4 + maxnodesz * sizeof (NODE) );
	*((long *) node) = 0;		/* backpointer to previous table */
	((long *)node) = ((long *) node) + 1;	/* advance to start */

	seqtab = (NODE **) ckalloc(maxseqsz * sizeof(NODE *) );
	blockpool = (BBLOCK *) ckalloc (maxbblsz * sizeof(BBLOCK) );
	dfo = (BBLOCK **) ckalloc(maxbblsz * sizeof(BBLOCK *) );
	region = (SET **) ckalloc(maxregion * sizeof(SET *) );
	regioninfo = (REGIONINFO *) ckalloc(maxregion * sizeof(REGIONINFO));
	ivtab = (struct iv *) ckalloc(maxivsz * sizeof (struct iv));
	ep = (EPB *) ckalloc(maxepsz * sizeof(EPB) );
	max_vfes = VFE_TABLE_SIZE;
	vfe_thunks = (struct vfethunk *) ckalloc(max_vfes *
						  sizeof(struct vfethunk));
}


/* hardconv() checks fix and float conversions to see if they will eventually
   wind up as CALLs to library conversion routines.
*/
flag hardconv(p) 	register NODE *p;
{
	TTYPE t,tl;
	flag m;
	flag ml;
	flag fhw;	/* YES iff the op will be done in hardware, not by CALL */
	fhw = !flibflag;

	t = p->in.type;
	tl = p->in.left->in.type;
	m = ISFTP(t);
	ml = ISFTP(tl);


	if (m==ml)
		if ( (m==0) || fhw || ((t.base==tl.base)&&(t.mods1==tl.mods1)
			&&(t.mods2==tl.mods2)) ) 
			return (NO);
	if ( fhw )
		{
		/* Scalar to floating point conversion or vice versa of some
		type.  Floating point hardware can do it if an intermediate
		SCONV is there.
		*/
		return (NO);
		}
	
	return (YES);
}



# ifdef DEBUGGING
/* printset() does a low level print to debugp of a set. */
/* NOTE: It prints at least as many elements as are actually in the set. size
   is very crude!
*/
printset(sp)	SET *sp;
{
	register unsigned size, i;

	size = set_size( sp );
	for (i=0; i < size; i++)
		{
		if ( i%5 == 0 ) fprintf(debugp,(!i || i%50)? " ":"\n");
		fprintf(debugp,xin(sp, i)? "1":"0" );
		}
	fprintf(debugp,"\n");
}


/* print_settab() prints out the contents of a table of sets for debugging
   purposes.
*/
print_settab(setp, name)
	register SET **setp; register char *name;
{
	register unsigned i;
	register unsigned lnumblocks = numblocks;

	for (i=0; i < lnumblocks; i++)
		{
		fprintf(debugp, "%s[%3d]=", name, i);
		printset(setp[i]);
		}
}
# endif DEBUGGING




# ifdef DEBUGGING
ckfree(np)	char *np;
{
	if (mdebug)
		fprintf(debugp, "freeing 0x%x\n", np);
	free(np);
}




void printinvs(np)	NODE *np;
{
	if (np->ind.invariant && 
		(np->in.op == SEMICOLONOP || np->in.op == UNARY SEMICOLONOP) )
		fprintf(debugp, "\tnp = 0x%x\n", np);
}
# endif DEBUGGING




/* tcopy() is a (sub)tree copy routine for trees. */
NODE *tcopy(p) NODE *p;
{
	register NODE *q;

	q = talloc();
	*q = *p;
	q->in.temptarget = NO;
	switch( optype(q->in.op) )
		{
		case BITYPE:
			q->in.right = tcopy(q->in.right);
			/* fall thru */
		case UTYPE:
			q->in.left = tcopy(q->in.left);
	    default:
			break;
		}
	return (q);
}




NODE *makety( p, t) register NODE *p; register TTYPE t; 
{
    long ptype;
    flag iptype;
    flag ittype;

	/* make p into type t by inserting a conversion */

    	ptype = p->in.type.base;

	if( t.base == ptype && t.mods1 == p->in.type.mods1
		&& t.mods2 == p->in.type.mods2 )
		return( p );

	if ( ISPTR(p->in.type) && ISPTR(t) )   /* pointers are pointers */
	    {
	    p->in.type = t;
	    return( p );
	    }

    	if ( ((ptype == INT) && ISPTR(t))
		|| ((ISPTR(p->in.type)) && (t.base == INT)) )
		{
		/* the SCONV does nothing useful and only confuses reg alloc. */
		p->in.type = t;
		return(p);
		}

	if ( (p->in.op == ICON) && ((p->in.type.base & MODS1MASK) == 0) )
	    {
	    /* avoid SCONV's on non-floating-point ICONs */

	    if ( (t.base & MODS1MASK) == 0 )
		{
		switch (t.base)
		    {
		    case INT:
		    case LONG:
		    case SHORT:
		    case UNSIGNED:
		    case CHAR:
		    case UCHAR:
		    case USHORT:
		    case ULONG:
			ittype = YES;
			break;
		    default:
			ittype = NO;
			break;
		    }
		switch (ptype)
		    {
		    case INT:
		    case LONG:
		    case SHORT:
		    case UNSIGNED:
		    case CHAR:
		    case UCHAR:
		    case USHORT:
		    case ULONG:
			iptype = YES;
			break;
		    default:
			iptype = NO;
			break;
		    }

		if (ittype && iptype)
		    {
		    p->in.type.base = t.base;
		    return(p);
		    }
		}
	    }
	return( block( SCONV, p, NIL, 0, t) );
}




/******************************************************************************
*
*	conval()
*
*	Description:		Apply the op o to the lval part of p;
*				if binary, rhs is val.
*
*	Called by:		node_constant_fold()
*				optim()
*
*	Input parameters:	o - an opcode (UNARY or BINARY)
*				p - lhs child NODE ptr
*				q - rhs child NODE ptr (if BINARY op)
*
*	Output parameters:	none
*				(returns YES iff the subexpression was constant
*				 folded)
*
*	Globals referenced:	fortran
*
*	Globals modified:	none
*
*	External calls:		werror()
*
*******************************************************************************/
LOCAL flag conval( p, o, q ) short o; register NODE *p, *q; {
	register CONSZ val;
	flag lconst;
	flag rconst;

	flag u = fortran? NO : ISUNSIGNED(p->in.type) || ISUNSIGNED(q->in.type);

	lconst = (p->in.op == ICON) && (p->tn.name == NULL);

	/* Take special care for unary ops */
	rconst = !q || ((q->in.op == ICON) && (q->tn.name == NULL));

# ifdef FOLD_ADDRESSES
	if (o == PLUS || o == MINUS)
		{
		/* These are the only ops valid on addresses */
		if (p->in.op != ICON || q->in.op != ICON)
			return(NO);
		}
	else
# endif FOLD_ADDRESSES
		if (!lconst || !rconst)
	    		return(NO);

	if (!fortran && u && (o==LE||o==LT||o==GE||o==GT))
	    o += (UGE-GE);

	val = q->tn.lval;

	switch( o ){

	case PLUS:
		p->tn.lval += val;
		if (q->tn.name != NULL)
			{
			q->tn.lval = p->tn.lval;
			*p = *q;
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
		if( val == 0 ) 
			{
			werror( "Division by 0.");
			return(NO);
			}
		else if (u) p->tn.lval = (unsigned) p->tn.lval / val;
		else p->tn.lval /= val;
		break;
	case MOD:
		if( val == 0 ) 
			{
			werror( "Division by 0.");
			return(NO);
			}
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
		break;
	case RS:
		if (u) p->tn.lval = (unsigned) p->tn.lval >> val;
		else p->tn.lval >>= val;
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
	default:
		return(NO);
		}

	switch(p->tn.type.base)
		{
		case SHORT:
			p->tn.lval = (short)p->tn.lval;
			break;
		case USHORT:
			p->tn.lval = (ushort)p->tn.lval;
			break;
		case CHAR:
			p->tn.lval = (char)p->tn.lval;
			break;
		case UCHAR:
			p->tn.lval = (uchar)p->tn.lval;
			break;
		}
	return(YES);
	}


/******************************************************************************
*
*	const_eqv()
*
*	Description:		Constant folds NEQV and EQV subtrees with at
*				least 1 child a constant.
*
*	Called by:		node_constant_fold()
*				optim()
*
*	Input parameters:	p - a NODE ptr to either a EQV or a NEQV node.
*				lconst - YES iff lhs is a constant
*				rconst - YES iff rhs is a constant
*
*	Output parameters:	p - Possibly modified subtree
*				(return value YES iff constant folding occurred)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL flag const_eqv(p, lconst, rconst)	register NODE *p; flag lconst, rconst;
{
	register NODE *t1, *t2;

	t1 = p->in.left;
	t2 = p->in.right;

	if (rconst && !lconst)
	    {
	    p->in.right = t1;
	    p->in.left = t2;
	    t1 = p->in.left;
	    t2 = p->in.right;
	    lconst = YES;
	    rconst = NO;
	    }
	if (lconst)
	    {
	    if (rconst)
		{
		p->in.op = ICON;
		p->tn.name = NULL;
		if (p->in.op == EQV)
		    p->tn.lval = (t1->tn.lval && t2->tn.lval);
		else
		    p->tn.lval = !(t1->tn.lval && t2->tn.lval);
		t1->in.op = FREE;
		t2->in.op = FREE;
		}
	    else
		{
		if (p->in.op == EQV)
		    {
		    if (t1->tn.lval)
			{
			p->in.op = NE;
			p->in.left = t2;
			p->in.right = t1;
			t1->tn.lval = 0;
			}
		    else
			{
			p->in.op = EQ;
			p->in.left = t2;
			p->in.right = t1;
			t1->tn.lval = 0;
			}
		    }
		else
		    {
		    if (t1->tn.lval)
			{
			p->in.op = EQ;
			p->in.left = t2;
			p->in.right = t1;
			t1->tn.lval = 0;
			}
		    else
			{
			p->in.op = NE;
			p->in.left = t2;
			p->in.right = t1;
			t1->tn.lval = 0;
			}
		    }
		}
	    return(YES);
	    }
	return(NO);
}

/******************************************************************************
*
*	FIND_PARENT()
*
*	Description:		If def/use information exists for the symbol
*				indicated by node 'p', find the def/use block
*                               with a parent of 'oldparent' and update the
*                               the parent field to its correct value.
*
*	Called by:		node_constant_fold()
*				extract_constant_subtree()
*
*	Input parameters:	p - NODE being searched for.
*				tree - a NODE ptr to a statement tree
*				containing the node 'p'.          
*
*	Output parameters:	Return value is the parent of 'p'.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:	        none	
*
*******************************************************************************/
NODE *find_parent(p,tree)
NODE *p, *tree;
{
NODE *parent;
switch (optype(tree->in.op))
  {
  case BITYPE:
	if (tree->in.right == p)
	  return tree;
	if ((parent = find_parent(p,tree->in.right)) != NULL)
	  return parent;
	/* Fall through to look at the left operand */
  case UTYPE:
	if (tree->in.left == p)
	  return tree;
	if ((parent = find_parent(p,tree->in.left)) != NULL)
	  return parent;
  default: return NULL;
  }
}

/******************************************************************************
*
*	CHANGE_PARENT()
*
*	Description:		If def/use information exists for the symbol
*				indicated by node 'p', find the def/use block
*                               with a parent of 'oldparent' and update the
*                               the parent field to its correct value.
*
*	Called by:		node_constant_fold()
*				extract_constant_subtree()
*
*	Input parameters:	p - a NODE ptr to either a NAME or an OREG node.
*				oldparent - original parent of 'p'
*
*	Output parameters:	none
*
*	Globals referenced:	def/use blocks for the symbol in NODE p
*
*	Globals modified:	def/use blocks for the symbol in NODE p
*
*	External calls:		find()
*                               find_parent()
*
*******************************************************************************/
void change_parent(p, oldparent)
NODE *p, *oldparent;
{
long sym_index;
DUBLOCK *du;
DUBLOCK *duend;
HASHU *sp;
NODE *topnode;

if ((sym_index = find(p)) != (unsigned) (-1))
  sp = symtab[sym_index];
else
  return;

if ((du = sp->an.du) == NULL)
  return;

duend = du;

do {
   du = du->next;
   if ((du->d.parent == oldparent) && !(du->deleted))
     {
     /* search the appropriate statement for     *
      * node 'p' to determine its current parent */

     topnode = stmttab[du->stmtno].np;
     if (topnode->in.op == SEMICOLONOP)
       /* Search only this statement. Look only at right operand. */
       if (topnode->in.right == p)
	 du->d.parent = topnode;
       else
	 du->d.parent = find_parent(p,topnode->in.right);
     else /* UNARY SEMICOLONOP */
       du->d.parent = find_parent(p,topnode);
     if (du->d.parent == NULL) /* something went wrong */
       cerror("Change_parent() did not find node");
     return;
     }
   } while (du != duend);

} /* change_parent */

void node_constant_fold(p)
NODE *p;
{
    register NODE *t1, *t2;
    register short o;
    register short ty;
    flag lconst;
    flag rconst;

    ty = optype( o=p->in.op);
    if( ty == LTYPE ) return;

    t1 = p->in.left;
    t2 = p->in.right;
	  
    switch(ty)
	{
	case UTYPE:
	    t2 = t1;
	    /* fall thru */
	case BITYPE:
	    lconst = (t1->tn.op == ICON) && (t1->tn.name == NULL);
	    rconst = (t2->tn.op == ICON) && (t2->tn.name == NULL);

	    if ( ((p->in.op == EQV) || (p->in.op == NEQV))
		&& const_eqv(p, lconst, rconst) )
			{
			/* const_eqv() has side effects. */
#ifdef COUNTING
			_n_consts_folded++;
#endif
			return;
			}

	    if (!lconst || !rconst)
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   This would be a good place to handle  ICON(name) <op> <const>
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
		break;

	    if (conval(t1,o,t2))
		{
		if (t1 != t2)
		    t2->tn.op = FREE;
		t1 = makety(t1, p->tn.type);
		*p = *t1;
		t1->in.op = FREE;
#ifdef COUNTING
		_n_consts_folded++;
#endif
		return;
		}
	    break;
	}

    if (p->in.arrayref || p->in.isarrayform)
	{
	t1 = p;
	if (p->in.op == UNARY MUL)
	    t1 = p->in.left;
	if (t1->in.op != PLUS)
	    goto check_collapse;
	t2 = t1;
	if (t1->in.left->in.op == PLUS)
	    t2 = t1->in.left;
	o = t2->in.left->in.op;
	if ((o == FOREG) || (o == OREG) || (o == NAME) || (o == ICON))
	    {
	    extract_constant_subtree(t2, t2->in.left->tn.lval);

	    if ((t2->in.op == PLUS) && (t2->in.left->in.op != PLUS))
	        {
	        if ((t1 != t2) && (t2->in.right->in.op == ICON)
	         && (t2->in.right->atn.name == NULL))
		    {
		    t1->in.right->tn.lval += t2->in.right->tn.lval;
		    t1->in.left = t2->in.left;
		    if ((o == OREG) || (o == NAME))
		      change_parent(t2->in.left /*node*/, t2 /*old parent*/);
		    t2->in.right->in.op = FREE;
		    t2->in.op = FREE;
		    }
	        }
	    else
	        {
	        if ((t2->in.op == PLUS) && (t2->in.left->in.op == PLUS))
		    {
		    if (t1 != t2)	/* was 2-level before */
		        {
		        t1->in.right->tn.lval += t2->in.right->tn.lval;
		        t1->in.left = t2->in.left;
		        t2->in.right->in.op = FREE;
		        t2->in.op = FREE;
		        }
		    }
	        else		/* must be leaf node now */
		    {
		    if (t1 == t2)
		        {
		        if (p == t1)
			    node_constant_collapse(p);
		        else
			    p->in.left = t2;
		        }
		    else
		        t1->in.left = t2;
		    }
	        }
	    }

check_collapse:
	if (p->in.op == UNARY MUL)
	    node_constant_collapse(p);
	if ((p->in.op == UNARY MUL) && p->in.isptrindir)
	    computed_ptrindir(p);	/* check if still arrayform */
	}

    return;
}  /* node_constant_fold */

/*****************************************************************************
 *
 *  NODE_CONSTANT_COLLAPSE()
 *
 *  Description:
 *
 *  Called by:
 *
 *  Input Parameters:
 *
 *  Output Parameters:
 *
 *  Globals Referenced:
 *
 *  Globals Modified:
 *
 *  External Calls:
 *
 *****************************************************************************
 */

void node_constant_collapse(p)
NODE *p;
{
    register NODE *lp;
    register NODE *llp;
    register NODE *rlp;
    HASHU *sp;
    long offset;

    if (optype(p->in.op) == LTYPE)
	goto process;

    lp = p->in.left;
    if (lp->in.op == PLUS)
	{
	llp = p->in.left;
	rlp = p->in.right;
        if ((llp->in.op == REG)
	 && (rlp->in.op == ICON) && (rlp->atn.name == NULL))
	    {
	    lp->in.op = FOREG;
	    lp->tn.rval = llp->tn.rval;
	    lp->tn.lval = rlp->tn.lval;
	    llp->in.op = FREE;
	    rlp->in.op = FREE;
#ifdef COUNTING
	    _n_nodes_constant_collapsed++;
#endif
	    }
	}

    if ((lp->in.op == FOREG)
     || ((lp->in.op == ICON) && (lp->atn.name)))
	{
	p->tn.lval = lp->tn.lval;
	p->tn.rval = lp->tn.rval;
	p->atn.name = (lp->in.op == FOREG) ? NULL : lp->atn.name;
	p->in.op = (lp->in.op == FOREG) ? OREG : NAME;
	if (p->in.isptrindir)
	    {
	    p->in.arrayref = lp->in.arrayref;
	    p->in.arrayrefno = lp->in.arrayrefno;
	    p->in.structref = lp->in.structref;
            p->in.isarrayform = NO;
	    p->in.isptrindir = NO;
	    }
	lp->in.op = FREE;
	goto process;
	}
    else
      if (lp->in.op == PLUS)
	{
	llp = lp->in.left;
	rlp = lp->in.right;
	if ((rlp->in.op == ICON) && (rlp->atn.name == NULL))
	    {
	    if ((llp->in.op == FOREG)
	     || ((llp->in.op == ICON) && llp->atn.name))
		{
		p->tn.lval = llp->tn.lval += rlp->tn.lval;
		p->tn.rval = llp->tn.rval;
		p->atn.name = (llp->in.op == FOREG) ? NULL : llp->atn.name;
		p->in.op = (llp->in.op == FOREG) ? OREG : NAME;
		if (p->in.isptrindir)
		    {
		    if (lp->in.arrayref || lp->in.structref)
			{
			p->in.arrayrefno = lp->in.arrayrefno;
		    	p->in.arrayref = lp->in.arrayref;
		    	p->in.structref = lp->in.structref;
			}
		    else if (llp->in.arrayref || llp->in.structref)
			{
			p->in.arrayrefno = llp->in.arrayrefno;
		    	p->in.arrayref = llp->in.arrayref;
		    	p->in.structref = llp->in.structref;
			}
		    else
			{
			p->in.arrayrefno = NO_ARRAY;
		    	p->in.arrayref = NO;
		    	p->in.structref = NO;
			}
                    p->in.isarrayform = NO;
		    p->in.isptrindir = NO;
		    }
		llp->in.op = FREE;
		rlp->in.op = FREE;
		lp->in.op = FREE;
process:
#ifdef COUNTING
		_n_nodes_constant_collapsed++;
#endif
		if (p->in.arrayref)
		    {
		    sp = symtab[p->in.arrayrefno];
		    arrayflag = YES;
		    arrayelement = YES;
		    offset = sp->a6n.offset;
		    p->in.arrayelem = YES;
		    }
		else if (p->in.structref)
		    {
		    structflag = YES;
		    sp = symtab[p->in.arrayrefno];
		    offset = sp->a6n.offset;
		    isstructprimary = YES;
		    }
		if (p->in.op == OREG)
		    process_oreg_foreg(p, offset, offset);
		else
		    process_name_icon(p, offset, offset);
		}
	    }
	}
}

	/* mapping relationals when the sides are reversed */
short revrel[] ={ EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };

# pragma OPT_LEVEL 1
/******************************************************************************
*
*	optim()
*
*	Description:		Performs local constant-folding, reassociativity
*				transformations on the parse trees.
*
*	Called by:		constprop()
*				optim()	recursively
*				global_constant_propagation()
*				addnode()
*				loop_optimization()
*				detect_indvars()
*				add_sincr()
*
*	Input parameters:	p	a NODE ptr to top of a subtree.
*
*	Output parameters:	none
*				return value is a ptr to a modified tree.
*
*	Globals referenced:	deftab[]
				revrel[]
*
*	Globals modified:	deftab[]
*
*	External calls:		nncon()	- a macro after rel6.5
*				const_eqv()
*				makety()
*				cerror()
*				haseffects()
*				block()
*				tcopy()
*				conval()
*				tfree()
*				ispow2()
*				makepow2()
*				werror()
*
*******************************************************************************/
NODE * optim(p) register NODE *p; 
{
	register NODE *sp;
	register NODE *t1, *t2;
	register short o;
	register short ty;
	register int i;
	NODE *t3;
	flag lconst, rconst;

# ifdef DEBUGGING
	if (odisable)
		return(p);
# endif DEBUGGING
	ty = optype( o=p->in.op);
	if( ty == LTYPE ) return(p);

constantfold:
	if( ty == BITYPE ) p->in.right = optim(p->in.right);
	p->in.left = optim(p->in.left);

	/* check for constant folding */
	{
	  t1 = p->in.left;
	  t2 = p->in.right;
	  
	  switch(ty)
		{
		case UTYPE:
			t2 = t1;
			/* fall thru */
		case BITYPE:
			lconst = nncon(t1);
			rconst = nncon(t2);
			if ( (p->in.op == EQV || p->in.op == NEQV)
				&& const_eqv(p, lconst, rconst) )
				/* const_eqv() has side effects. */
				return(p);
# ifdef FLOAT_SR
			/* Special purpose rewrite for strength reduction
			   of floating point stuff.
			*/
			if ( p->in.op == MUL && ISFTP(p->in.type) &&
				t1->in.op == SCONV && nncon(t1->in.left)
				&& ISINT(t1->in.left->in.type) )
				{
				if (t1->in.left->tn.lval == 1)
					{
					p->in.op = FREE;
					tfree(t1);
					return(t2);
					}
				else if (t1->in.left->tn.lval == 0)
					{
					p->in.op = FREE;
					tfree(t2);
					t1->in.op = FREE;
					return(t1->in.left);
					}
				}
# endif FLOAT_SR

	  		if (conval(t1,o,t2))
				{
			   	if (t1 != t2) t2->tn.op = FREE;
			   	t1 = makety(t1, p->tn.type);
			   	p->in.op = FREE;
			   	return (optim(t1));
				}
			break;
	  	}
	}



	/* Collect constants and do some machine-dependent strength
	   reductions.
	*/

	switch(o)
	{
	case ASSIGN:
		/* Constant propagation may simplify a rhs to the extent
		   that it becomes constant. We need to recheck after
		   folding.
		*/
		if (p->in.defnumber)
			deftab[p->in.defnumber].constrhs = (RO(p) == ICON);
		return (p);
	case COMOP:
		if (optype(LO(p)) == LTYPE)
			{
			LO(p) = FREE;
			p->in.op = FREE;
			p = p->in.right;
			}
		return(p);
	case MUL:
	case PLUS:
		/* commutative ops; for now, just collect constants */
		if ( ( nncon(p->in.left) || ( LCON(p) && !RCON(p) ) ) &&
		!(p->in.arrayref || p->in.arraybaseaddr || p->in.structref) )
			SWAP( p->in.left, p->in.right );

	}


	switch(o)
	{

	case SCONV:
		t1 = p->in.left;
		if ( (p->in.type.base == t1->in.type.base
			&& p->in.type.mods1 == t1->in.type.mods1
			&& p->in.type.mods2 == t1->in.type.mods2)
		    || (p->in.type.base == INT && ISPTR(t1->in.type)) )
			{
			/* Throw away the SCONV. */
			p->in.op = FREE;
			return(t1);
			}
		if (nncon(t1) && !ISFTP(p->in.type))
			{
			/* Constants can have their type changed directly. */
			t1->in.type = p->in.type;
			p->in.op = FREE;

			switch (t1->tn.type.base)
			/* For unsigned operands, zero out nonpertinent bits.
			   Don't do it for signed operands or moveq will break.
			*/
			{
			case USHORT:
				t1->tn.lval &= 0xffff;
				break;
			case UCHAR:
				t1->tn.lval &= 0xff;
				break;
			}

			return(t1);
			}
		return( p );

	case UNARY MUL:
		if( (LO(p) != ICON) || ISPTR(p->in.left->tn.type)
			|| (p->in.type.base & 0xff) == STRTY )
				break;
		LO(p) = NAME;
		goto setuleft;

	case UNARY AND:
		if( LO(p) != NAME ) cerror( "& error" );
		LO(p) = ICON;

setuleft:
		/* paint over the type of the left hand side with the type of
		   the top */
		p->in.left->in.type = p->in.type;
		p->in.op = FREE;
		return( p->in.left );

	case MINUS:
		if( !nncon(p->in.right) ) break;
		if (RV(p) == 0) goto zapright;
		RV(p) = -RV(p);
		p->in.op = o = PLUS;
		/* fall thru */

	case PLUS:
	case OR:
	case ER:
		if (nncon(p->in.right) && RV(p) == 0) goto zapright;
		goto tower;

	case AND:
		if (nncon(p->in.right) && RV(p) == 0 
			&& !haseffects(p->in.left) )
				goto zeroout;
		goto tower;

	case MUL:
# if 0	/*... something isn't right here. I'm not sure it's worth it anyway. */
		if ( p->in.left->in.op == PLUS && (ISCONST(p->in.right)
				|| ISCONST(p->in.left->in.left)) )
			{
			/* Apply distributive property in hopes that the new
			   shape is more useful for optimization.
			*/
			p->in.op = PLUS;
			p->in.left->in.op = FREE;
			t1 = p->in.left->in.right;
			p->in.left = block(MUL, p->in.left->in.left,
					tcopy(p->in.right), 0, p->in.type);
			p->in.right = block(MUL, t1, p->in.right, 0,p->in.type);
			p = optim(p);
			}
# endif 0
		/* zap identity operations */
		if (nncon(p->in.right))
			{
			if (RV(p) == 0) goto zeroout;
			if (RV(p) == 1) goto zapright;
			}

		/* make ops tower to the left, not the right */
tower:		 if( RO(p) == o )
			{
			t1 = p->in.left;
			sp = p->in.right;
			/* now, put together again */
			if ( (sp->in.type.base == t1->in.type.base) &&
				(sp->in.type.mods1 == t1->in.type.mods1)
				&& (sp->in.type.mods2 == t1->in.type.mods2) )
				{
				if (ISPTR(t1->in.type))
					/* Never move the ptr down the tree. */
					SWAP(sp, t1)
				else
					{
					/* The additional restriction on type is
				   	necessary to prevent code gen problems
				   	mixing pointers/nonpointers, etc.
					i.e., the tree changes from

						p
					       / \
					      /   \
					     t1	  sp
						 /  \
						/    \
					       t2     t3
					to
					
						p
					       / \
					      /   \
					     sp	   t3
					    / \
					   /   \
					  t1	t2
					*/
					t2 = sp->in.left;
					t3 = sp->in.right;
					p->in.left = sp;
					sp->in.left = t1;
					sp->in.right = t2;
					p->in.right = t3;
					}
				}
			}

		if ( optype(LO(p)) == LTYPE )
			{
			/* Attempt to get constants as far left as possible
			   and REGS as far right as possible.
			*/
			if ( nncon(p->in.right) || LO(p)==REG )
				SWAP(p->in.left, p->in.right)
			else if ( (LO(p) == OREG && RO(p) == OREG)
				&& RV(p) > LV(p) )
				SWAP(p->in.left, p->in.right)
			}
		else if ( (LO(p) == o)
			&& (p->in.type.base==p->in.left->in.type.base)
			&& (p->in.type.mods1==p->in.left->in.type.mods1)
			&& (p->in.type.mods2==p->in.left->in.type.mods2) )
			{
			/* The eventual order of commutative towering op trees
			is (from left to right)
				(1) constants
				(2) OREGS ... hightest value left
					(consider FOREGS later)
					NOTE: highest left because these are
					first declared variables (least
					negative or formals) which by
					guesstimate are the most frequently
					used.
				(3) REGS
			*/


			sp = p->in.left;
			t1 = sp->in.left;
			t2 = sp->in.right;
			t3 = p->in.right;

			/* First attempt to get constants as far left as
			   possible and REGs as far right as possible.
			*/
			if ( nncon(t2) || t1->in.op == REG ) SWAPX(sp, t1, t2);

			/* This section is compiled out because it tends to
			   destroy commonality for CSE optimization on array
			   references. It may pay to do local constant 
			   propagation again after CSE optimization. In that
			   case this section of code probably should be
			   reenabled at that time. -mfm
			*/
			/* Use associativity to (possibly) associate
			   constants.
			*/
			t2 = sp->in.right;
			if ( nncon(t3) && !nncon(t2) 
				&& t3->in.arraybaseaddr==t2->in.arraybaseaddr )
				{
				sp->in.right = t3;
				p->in.right = t2;
				t1 = sp->in.left;
				/* Ptr + int may have become int + int */
				if ( (t3->in.type.base == t1->in.type.base)
				   &&(t3->in.type.mods1 == t1->in.type.mods1)
				   &&(t3->in.type.mods2 == t1->in.type.mods2) )
					sp->in.type = t1->in.type;
				goto constantfold;
				}

reassociate:
			t1 = sp->in.left;
			t2 = sp->in.right;
			if ( nncon(t2) || t1->in.op == REG ) SWAPX(sp, t1, t2);
			t1 = sp->in.left;
			t2 = sp->in.right;
			if ( !nncon(t1) )
				{
				if ( (t1->in.op != OREG)
					&& (t2->in.op == OREG) )
					SWAPX(sp, t1, t2);
				t1 = sp->in.left;
				t2 = sp->in.right;
				if ( (t2->in.op == OREG) && (t1->in.op == OREG)
					&& (t2->tn.lval > t1->tn.lval) )
					SWAPX(sp, t1, t2);
				}
			t2 = sp->in.right;
			t3 = p->in.right;
			if ( (t2->in.op == OREG) && (t3->in.op == OREG)
				&& (t2->tn.lval < t3->tn.lval) )
				{
				sp->in.right = t3;
				p->in.right = t2;
				goto reassociate;
				}
			t1 = sp->in.left;
			if (t1->in.op == OREG && t3->in.op == OREG
				&& t1->tn.lval < t3->tn.lval)
				{
				sp->in.left = t3;
				p->in.right = t1;
				goto reassociate;
				}
			}
		else if ( RO(p) < LO(p) )
			/* canonicalize by topolically sorting the tree */
			SWAP(p->in.left, p->in.right)

		if(o == PLUS && LO(p) == MINUS && RCON(p) && RCON(p->in.left) &&
		  conval(p->in.right, MINUS, p->in.left->in.right)){
zapleft:
			RO(p->in.left) = FREE;
			LO(p) = FREE;
			p->in.left = p->in.left->in.left;
		}
		if( RCON(p) && LO(p)==o && RCON(p->in.left) && 
		    conval( p->in.right, o, p->in.left->in.right ) ){
			goto zapleft;
			}
		else if( LCON(p) && RCON(p) &&
			conval( p->in.left, o, p->in.right ) ){
zapright:
			RO(p) = FREE;
			p->in.left = makety( p->in.left, p->in.type);
			p->in.op = FREE;
			return( p->in.left );
			}

		/* change muls to shifts */

		if( o==MUL && (nncon(p->in.right) || nncon(p->in.left)) )
			{
			if (nncon(p->in.left))
				SWAP(p->in.left, p->in.right);

			if ( !RV(p) ) /* multiply by 0 */
				{
zeroout:
				tfree(p->in.left);
				p->in.op = FREE;
				return (p->in.right);
				}
			i = RV(p);
			if (i == (i & -i)) /* an exact power of 2 */
				{
				p->in.op = o = LS;
				p->in.right->in.type.base = INT;
				RV(p) = ispow2(i);
				/* Shifts are faster with a constant
				   than with a register shift factor.
				*/
				i = RV(p);
				if (i >= 1 && i <= 8)
					p->in.right->in.no_iconsts = YES;
				}

			/* Nothing was accomplished. Put back the way it was. */
			if (o == MUL && nncon(p->in.right))
				SWAP(p->in.left, p->in.right);
			}
		break;

	case DIV:
		if( nncon( p->in.right ) ) 
			if ( (i=RV(p)) == 0 )
				{
				werror( "Division by 0. Proceding.");
				break;
				}
			else if (i == 1 ) goto zapright;
			else
				{
				if ( i == (i & -i) ) /* exact power of 2 */
					{
					if (ISUNSIGNED(p->in.left->in.type) )
						/* remember: -1/2  integer divide = 0 */
						{
						p->in.op = RS;
						RV(p) = ispow2(i);
					/* Shifts are faster with a constant
				   	than with a register shift factor.
					*/
					i = RV(p);
					if (i >= 1 && i <= 8)
						p->in.right->in.no_iconsts = YES;
						p->in.right->in.type.base = INT;
						}
					else
						{
						/* subvert hardops */
						if (!fortran)
							{
							p->in.op = DIVP2;
							RV(p) = ispow2(i);
							}
						}
					}
				}
		break;

	case MOD:
		if( nncon( p->in.right ) && RV(p)==0 )
			{
			werror( "Division by 0. Proceding.");
			break;
			}
		break;

	case EQ:
	case NE:
	case LT:
	case LE:
	case GT:
	case GE:
	case ULT:
	case ULE:
	case UGT:
	case UGE:
		if( !LCON(p) ) break;

		/* exchange operands */

		sp = p->in.left;
		p->in.left = p->in.right;
		p->in.right = sp;
		p->in.op = revrel[p->in.op - EQ ];
		break;

	case LS:
	case RS:
		if (o == LO(p) && nncon(p->in.right) && nncon(p->in.left->in.right))
			{
			/* Towering shifts with constant factors can be combined. */
			p->in.left->in.right->tn.lval += RV(p);
			p->in.left->in.right->tn.type = p->in.left->in.type;
			p->in.op = FREE;
			RO(p) = FREE;
			return(p->in.left);
			}
		if (nncon(p->in.right) && RV(p) == 0 ) goto zapright;
		break;
		}

	return(p);
}
# pragma OPT_LEVEL 2

/******************************************************************************
*
*	ispow2()
*
*	Description:		Returns value > 0 if the argument is an
*				exact power of 2.
*
*	Called by:		optim()
*				sr_worthit()
*				idiomcheck()
*
*	Input parameters:	c - an integer constant
*
*	Output parameters:	none
*				(returns the n for c == 2**n. Returns -1 if
*				c is not a power of 2 or it is 0.)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
ispow2( c ) register CONSZ c; {
	register i;
	if( c <= 0 || (c&(c-1)) ) return(-1);
	for( i=0; c>1; ++i) c >>= 1;
	return(i);
	}


/******************************************************************************
*
*	mktemp()
*
*	Description:		Create a temporary and stick it in the symbol
*				table.
*
*	Called by:		everybody
*
*	Input parameters:	tbase - the TYPE base 
*				type - the full TYPE
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	tmpoff
*				symtab
*
*	External calls:		block()
*				find()
*
*******************************************************************************/
NODE *mktemp(tbase, type) TWORD tbase; TTYPE type;
{
	NODE *np;
	HASHU *sp;

	tmpoff += (type.base == DOUBLE)? 8 : 
			(type.base == LONGDOUBLE)? 16 : 4;
	np = block(OREG, -tmpoff, TMPREG, 0, type);
	allow_insertion = YES;
	sp = symtab[find(np)];
	allow_insertion = NO;
	if (tbase)
		{
		sp->a6n.type.base = tbase;
		sp->a6n.type.mods1 = 0;
		sp->a6n.type.mods2 = 0;
		}
	else 
		sp->a6n.type = type;
	sp->a6n.ptr =ISPTR(sp->a6n.type);
	return(np);
}


/* make a copy of a temporary and return it */
NODE *cptemp(np)
NODE *np;
{
	NODE *cp;

	cp = block(OREG, np->tn.lval, TMPREG, 0, np->tn.type);
	return(cp);
}

/*****************************************************************************
 *
 *  REWRITE_COMOPS()
 *
 *  Description:	Traverse all blocks, removing COMOP's by rewriting
 *			trees appropriately.
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	topblock
 *			lastblock
 *
 *  Globals Modified:	none
 *
 *  External Calls:	rewrite_comops_in_block()
 *
 *****************************************************************************
 */

void rewrite_comops()
{
    BBLOCK *bp;

    for (bp = topblock; bp <= lastblock; ++bp)
	{
	if (bp->b.treep)
	    {
	    rewrite_comops_in_block(bp->b.treep);
	    }
	}
}  /* rewrite_comops */

/*****************************************************************************
 *
 *  REWRITE_COMOPS_IN_BLOCK()
 *
 *  Description:	For a single block, traverse by statement to rewrite
 *			CM or UCM ops.
 *
 *  Called by:		local_optimizations()
 *			global_init()
 *			rewrite_comops()
 *			rewrite_comops_in_block()	{RECURSIVE}
 *			strength_reduction()
 *
 *  Input Parameters:	np - a NODE ptr to the top of the block tree.
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	currsemicolon
 *
 *  External Calls:	rewrite_comops_in_block()
 *			rewrite_comops_in_stmt()
 *
 *****************************************************************************
 */

void rewrite_comops_in_block(np) register NODE *np;
{
    if (np->in.op == SEMICOLONOP)
	rewrite_comops_in_block(np->in.left);

    currsemicolon = np;

    while (rewrite_comops_in_stmt(
		(np->in.op == SEMICOLONOP) ? np->in.right : np->in.left));

}  /* rewrite_comops_in_block */

/*****************************************************************************
 *
 *  REWRITE_COMOPS_IN_STMT()
 *
 *  Description:	Changes subexpressions topped by a CM or UCM node into
 *			a series of separate expressions in the correct order.
 *
 *  Called by:		rewrite_comops_in_block()
 *			rewrite_comops_in_stmt()	{RECURSIVE}
 *
 *  Input Parameters:	np - a NODE ptr to the top of a subexpression.
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	_n_comops_rewritten
 *			currsemicolon
 *
 *  External Calls:	rewrite_comops_in_stmt()
 *			talloc()
 *
 *****************************************************************************
 */

LOCAL flag rewrite_comops_in_stmt(np)
register NODE *np;
{
    register NODE *newnp;
    NODE *p;

    switch (optype(np->in.op))
	{
	case LTYPE:
		return(NO);
	case UTYPE:
		return(rewrite_comops_in_stmt(np->in.left));
	}

    if (np->in.op == COMOP)
	{
#ifdef COUNTING
	_n_comops_rewritten++;
#endif
	if (optype(LO(np)) == LTYPE)
	    {
	    /* Throw it away because it cannot have a useful purpose */
	    LO(np) = FREE;
	    newnp = np->in.right;
	    *np = *newnp;
	    newnp->in.op = FREE;
	    }
	else
	    {
	    newnp = talloc();
	    if (currsemicolon->nn.op == SEMICOLONOP)
	    	{
	    	newnp->nn.left = currsemicolon->nn.left;
	    	currsemicolon->nn.left = newnp;
	    	newnp->nn.op = SEMICOLONOP;
	    	newnp->nn.right = np->in.left;
	    	}
	    else  /* UNARY SEMICOLONOP */
	    	{
	    	currsemicolon->nn.right = currsemicolon->nn.left;
	    	currsemicolon->nn.op = SEMICOLONOP;
	    	newnp->nn.left = currsemicolon->nn.left;
	    	currsemicolon->nn.left = newnp;
	    	newnp->nn.op = UNARY SEMICOLONOP;
	    	newnp->nn.left = np->in.left;
	    	}
	    /* The stmttab does not yet have an entry for the new
	       statement so the best we can do now is to mark the stmt
	       itself as having side-effects.  It will be migrated to
	       the stmttab later.  Note that if the stmtno is bogus it
	       means that the node was created by local or global
	       optimizations and it was not originally present;
	       therefore it cannot have side effects.
	    */

	    newnp->nn.haseffects = currsemicolon->nn.stmtno == NO_STMTNO?
			NO : stmttab[currsemicolon->nn.stmtno].hassideeffects;

	    p = currsemicolon;
	    currsemicolon = newnp;
	    while (rewrite_comops_in_stmt(np->in.left))
	    	;
	    currsemicolon = p;
	    currsemicolon->nn.haseffects = newnp->nn.haseffects;
	
	    p = np->in.right;
	    *np = *p;
	    p->in.op = FREE;
	    }
	return(YES);
	}

    return(rewrite_comops_in_stmt(np->in.left)
	|| rewrite_comops_in_stmt(np->in.right));

}  /* rewrite_comops_in_stmt */


/*****************************************************************************
 *
 *  FIND_NODE_STATEMENT()
 *    GET_STATEMENT_NO()
 *      NODE_IN_TREE()          
 *
 *  Description: find_node_statement() is called to determine what original
 *		 source line a particular node is in.  get_statement_no() and
 *               node_in_tree() are routines called at this time only by
 *		 find_node_statement() so they are listed here.  
 *		 find_node_statement() searches all basic block until it
 *		 finds the desired node.  Because of this it is not
 *		 recommended for general use.  A result of 0 means the line
 * 		 containing the node was not in the original source but was
 *		 created by c1.
 *
 *  Called by: process_name_icon
 *	       process_oreg_foreg
 *
 *  Input Parameters: np -- the node being searched for
 *
 *  Output Parameters: The result is the original source line number containing
 *		       the node.
 *
 *  Globals Referenced: dfo[]
 *
 *  Globals Modified: none
 *
 *  External Calls: cerror
 *		    optype
 *
 *****************************************************************************
 */
flag node_in_tree(np,tree)
  NODE *np, *tree;
/* called by: get_statement_no(np,tree) */
{
  short opty;

  if (np == tree) return (YES);

  opty = optype(tree->in.op);

  if (opty == LTYPE) return (NO);

  if (node_in_tree(np,tree->in.left)) return (YES);
  else
    if (opty == BITYPE)
      return (node_in_tree(np,tree->in.right));
    else
      return (NO);
}

int get_statement_no(np,tree)
  NODE *np, *tree;
/* called by: find_node_statement */
{
  int stmtno;

  if (tree->in.op == SEMICOLONOP)
    {
    if ((stmtno = get_statement_no(np,tree->in.left)) >= 0) return(stmtno);
    if (node_in_tree(np,tree->in.right))
      return (tree->nn.source_lineno);
    }
  else
    if (node_in_tree(np,tree->in.left))
      return (tree->nn.source_lineno);
  return(-1);
}

int find_node_statement (np)
  NODE *np;
/* called by: process_name_icon */
{
  int statement_no;
  int i;

  statement_no = -1;
  for (i = 0; i < numblocks; i++)
    {
    if (dfo[i]->bb.treep)
      if ((statement_no = get_statement_no(np,dfo[i]->bb.treep)) >= 0) break;
    }
  if (statement_no < 0) cerror("Node not found anywhere in trees.");
  return(statement_no);
}
