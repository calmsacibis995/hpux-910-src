/* @(#) $Revision: 70.2 $ */    
/*
 *	 c2: a c object code improver
 *
 *	11/81	CVD -> CVD & PCD ???
 *	 4/82	received from MIT (through DCD)
 *	 5/82	commented and debugged by Andy Rood at CVD
 *	 8/82	more comments, debugging and coding by Anny Seagraves at DCD
 *	12/82	update to accept motorola standard syntax by Anny at DCD
 *	 3/83	CVD & PCD recombine to become PCD.
 *	 3/83	DCD becomes FSD.  Inhabitants unimpressed.
 *	 4/83	continued commenting and debugging by Steve Heibert at PCD
 *	 6/83	Anny Seagraves becomes Anny Randel.
 *	 8/83	continued work by Jack Applin at FSD
 *	 6/85 	continued work by Suhaib Khan at FSD
 *	 7/85	c2 makes it out to customers in release 5.0/5.1
 *       8/85	modified for 68020, 68881 and new assembler syntax
 *
 *****************
 *
 *	Name directory:
 *	ja  = Jack Applin at FSD
 *	acs = Anny Randel nee Seagraves at FSD nee DCD
 *	alr = Andy Rood at PCD nee CVD
 *	sph = Steve Heibert at PCD
 *
 *****************
 *
 *  c2 [ -<tons of options> ] [ <source> ] [ <destination> ]
 *
 *      For informations on options, see file options.c.
 *	<source> ... default stdin
 *	<object> ... default stdout
 */

#include "o68.h"

char *strcpy(), *strcat(), *malloc(), *getenv();
node *getnode(), *release();
char *copy(), *parse_string();

char *input_ptr;
/* flags used to remember information obtained via comments */
static boolean alternate_entry_pt = false;
static boolean asm_seen;
static boolean alt_ent_seen;

char *procname[100];		/* procedures to optimize */
int pnum;			/* number of above */

int input_map[] = {
	ADDA,	ADD,
	ADDQ,	ADD,
	BCC,	(COND_CC<<16)+CBR,
	BCS,	(COND_CS<<16)+CBR,
	BEQ,	(COND_EQ<<16)+CBR,
	BGE,	(COND_GE<<16)+CBR,
	BGT,	(COND_GT<<16)+CBR,
	BHI,	(COND_HI<<16)+CBR,
	BLE,	(COND_LE<<16)+CBR,
	BLS,	(COND_LS<<16)+CBR,
	BLT,	(COND_LT<<16)+CBR,
	BMI,	(COND_MI<<16)+CBR,
	BNE,	(COND_NE<<16)+CBR,
	BPL,	(COND_PL<<16)+CBR,
	BRA,	JMP,
	BSR,	JSR,
	BVC,	(COND_VC<<16)+CBR,
	BVS,	(COND_VS<<16)+CBR,
	DC_B,	(BYTE<<16)+DC,
	DC_W,	(WORD<<16)+DC,
	DC_L,	(LONG<<16)+DC,
	MOVEQ,	(LONG<<16)+MOVE,
	SUBA,	SUB,
	SUBQ,	SUB,
	0
	};


struct op_info op_info[] = {	/* sorted alphabetically for a binary search */
	"add",		ADD,
	"adda",		ADDA,
	"addq",		ADDQ,
	"and",		AND,
	"asciz",	ASCIZ,
	"asl",		ASL,
	"asr",		ASR,
	"bcc",		BCC,
	"bchg",		BCHG,
	"bclr",		BCLR,
	"bcs",		BCS,
	"beq",		BEQ,
	"bfchg",	BFCHG,
	"bfclr",	BFCLR,
	"bfexts",	BFEXTS,
	"bfextu",	BFEXTU,
	"bfffo",	BFFFO,
	"bfins",	BFINS,
	"bfset",	BFSET,
	"bftst",	BFTST,
	"bge",		BGE,
	"bgt",		BGT,
	"bhi",		BHI,
	"ble",		BLE,
	"bls",		BLS,
	"blt",		BLT,
	"bmi",		BMI,
	"bne",		BNE,
	"bpl",		BPL,
	"bra",		BRA,
	"bset",		BSET,
	"bsr",		BSR,
	"bss",		BSS,
	"btst",		BTST,
	"bvc",		BVC,
	"bvs",		BVS,
	"byte",		DC_B,
	"clr",		CLR,
	"cmp",		CMP,
	"comm",		COMM,
	"data",		DATA,
	"dbcc",		DBCC,
	"dbcs",		DBCS,
	"dbeq",		DBEQ,
	"dbf",		DBF,
	"dbge",		DBGE,
	"dbgt",		DBGT,
	"dbhi",		DBHI,
	"dble",		DBLE,
	"dbls",		DBLS,
	"dblt",		DBLT,
	"dbmi",		DBMI,
	"dbne",		DBNE,
	"dbpl",		DBPL,
	"dbra",		DBRA,
	"dbt",		DBT,
	"dbvc",		DBVC,
	"dbvs",		DBVS,
	"divs",		DIVS,
	"divsl",	DIVSL,
	"divu",		DIVU,
	"divul",	DIVUL,
	"eor",		EOR,
	"exg",		EXG,
	"ext",		EXT,
	"extb",		EXTB,
	"fabs",		FABS,
	"facos",	FACOS,
	"fadd",		FADD,
	"fasin",	FASIN,
	"fatan",	FATAN,
	"fbeq",		FBEQ,
	"fbge",		FBGE,
	"fbgl",		FBGL,
	"fbgle",	FBGLE,
	"fbgt",		FBGT,
	"fble",		FBLE,
	"fblt",		FBLT,
	"fbneq",	FBNEQ,
	"fbnge",	FBNGE,
	"fbngl",	FBNGL,
	"fbngle",	FBNGLE,
	"fbngt",	FBNGT,
	"fbnle",	FBNLE,
	"fbnlt",	FBNLT,
	"fcmp",		FCMP,
	"fcos",		FCOS,
	"fcosh",	FCOSH,
	"fdiv",		FDIV,
	"fetox",	FETOX,
	"fintrz",	FINTRZ,
	"flog10",	FLOG10,
	"flogn",	FLOGN,
	"fmod",		FMOD,
	"fmov",		FMOV,
	"fmovm",	FMOVM,
	"fmul",		FMUL,
	"fneg",		FNEG,
#ifdef DRAGON
	"fpabs",	FPABS,
	"fpadd",	FPADD,
	"fpbeq",	FPBEQ,
	"fpbge",	FPBGE,
	"fpbgl",	FPBGL,
	"fpbgle",	FPBGLE,
	"fpbgt",	FPBGT,
	"fpble",	FPBLE,
	"fpblt",	FPBLT,
	"fpbne",	FPBNE,
	"fpbnge",	FPBNGE,
	"fpbngl",	FPBNGL,
	"fpbngle",	FPBNGLE,
	"fpbngt",	FPBNGT,
	"fpbnle",	FPBNLE,
	"fpbnlt",	FPBNLT,
	"fpcmp",	FPCMP,
	"fpcvd",	FPCVD,
	"fpcvl",	FPCVL,
	"fpcvs",	FPCVS,
	"fpdiv",	FPDIV,
	"fpintrz",	FPINTRZ,
	"fpmov",	FPMOV,
	"fpmul",	FPMUL,
	"fpneg",	FPNEG,
	"fpsub",	FPSUB,
	"fptest",	FPTEST,
#endif DRAGON
	"fsgldiv",	FSGLDIV,
	"fsglmul",	FSGLMUL,
	"fsin",		FSIN,
	"fsinh",	FSINH,
	"fsqrt",	FSQRT,
	"fsub",		FSUB,
	"ftan",		FTAN,
	"ftanh",	FTANH,
	"ftest",	FTEST,
	"global",	GLOBAL,
	"jmp",		JMP,
	"jsr",		JSR,
	"lalign",	LALIGN,
	"lcomm",	LCOMM,
	"lea",		LEA,
	"link",		LINK,
	"long",		DC_L,
	"lsl",		LSL,
	"lsr",		LSR,
	"mov",		MOVE,
	"movm",		MOVEM,
	"movq",		MOVEQ,
	"muls",		MULS,
	"mulu",		MULU,
	"neg",		NEG,
	"not",		NOT,
	"or",		OR,
	"pea",		PEA,
	"rol",		ROL,
	"ror",		ROR,
	"rts",		RTS,
	"scc",		SCC,
	"scs",		SCS,
	"seq",		SEQ,
	"set",		SET,
	"sf",		SF,
	"sge",		SGE,
	"sglobal",	SGLOBAL,
	"sgt",		SGT,
	"shi",		SHI,
	"shlib_version",SHLIB_VERSION,
	"short",	DC_W,
	"sle",		SLE,
	"sls",		SLS,
	"slt",		SLT,
	"smi",		SMI,
	"sne",		SNE,
	"space",	DS,
	"spl",		SPL,
	"st",		ST,
	"sub",		SUB,
	"suba",		SUBA,
	"subq",		SUBQ,
	"svc",		SVC,
	"svs",		SVS,
	"swap",		SWAP,
	"text",		TEXT,
	"trap",		TRAP,
	"tst",		TST,
	"unlk",		UNLK,
	"version",	VERSION,
	NULL,		0};


char *program_name;		/* argv[0] for error messages */


/* ARGSUSED */
main(argc, argv)
int argc;
register char **argv;
{
	register int iteration_count, iteration_max;
	register boolean isend;
	register char *s;
	FILE *optrc;
	int  rval = 0;
#ifdef DEBUG
	char fname[100], line[100];
#endif DEBUG

	/* Kleenix optimizer */


	set_default_options();


	program_name = argv[0];		/* get program name for messages */
	argv++;


#ifdef DEBUG
	/*
	 * Check to see if we can run.
	 * We keep 32-bit quantities in int's, you see.
	 */
	if (sizeof(int)!=4)
		internal_error("sizeof(int) must be 4, not %d!", sizeof(int));
	if (sizeof(char)!=1)
		internal_error("sizeof(char) must be 1, not %d!", sizeof(char));

	/*
	 * Process $HOME/.optrc
	 */
	if (getenv("HOME")==NULL)
		goto norc;

	strcpy(fname, getenv("HOME"));
	strcat(fname, "/.optrc");
	optrc = fopen(fname, "r");
	if (optrc==NULL)
		goto norc;

	while (fscanf(optrc, "-%s", line)>0) {
		for (s=line; *s!='\0'; s++)
			toggle_option(*s);
	}

norc:

#endif DEBUG

	/*
	 * For arguments of the form -<letter>,
	 * set the appropriate flag to enable/disable that optimization.
	 */
	for ( ; *argv!=NULL && **argv=='-'; argv++)
	{
		for (s = *argv+1; *s!='\0'; s++)
		{
		  if (*s=='O')         /* take care of -Oname */
		  {
		    procname[pnum]= s+1;
		    pnum++;
		    break;
		  }
		  else
		    toggle_option(*s);	/* toggle a simple option */
		}
	}

#ifdef VERBOSE
	if (pnum)
	{
	  int i;
	  fprintf(stderr,"Only %d procedures are to be optimized\n",pnum);
	  for (i=0; i <pnum; i++)
	    fprintf(stderr,"%s \n",procname[i]);
	}
#endif VERBOSE


	/* open input file (if present, otherwise just retain stdin) */
	if (*argv!=NULL) {
		if (freopen(*argv, "r", stdin) == NULL) {
			fprintf(stderr,"%s: can't find %s\n",
				program_name, *argv);
			exit(1);
		}
		argv++;
	}

	/* open output file (if present, otherwise just retain stdout) */
	if (*argv!=NULL) {
		if (freopen(*argv, "w", stdout) == NULL) {
			fprintf(stderr,"%s: can't create %s\n",
				program_name, *argv);
			exit(1);
		}
		argv++;
	}

	if (*argv!=NULL) {
		fprintf(stderr, "Extra arguments starting with %s\n", *argv);
		exit(1);
	}

	/* attempt to tune c2 */
#ifdef DEBUG
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 8192);
#else DEBUG
	setvbuf(stdout, NULL, _IOFBF, 8192);
	setvbuf(stdin, NULL, _IOFBF, 8192);
#endif DEBUG

	if (pass_unchanged) {
		int c;
		bugout("Passing code unchanged");

		while ((c=getchar())!=EOF)
			putchar(c);
		exit(0);
	}

restart:

	if (fort && !pnum) {
		register int c;

		for(;;)
		{
			c=getchar();
			if (c == EOF) exit(0);
			putchar(c);
			if (c != '\t') continue;
			c=getchar();
			putchar(c);
			if (c == 'g')  break;
		}
		while ((c=getchar())!='\n')
			putchar(c);
		putchar(c);

	}

	bugout("Beginning optimization");
	iteration_max = 0;

	do {
		status_unknown();
		bugout("input");
		isend = input();
		if (asm_seen) {
			bugout("asm seen");
			goto out;
		}

		bugdump("9: after input, code is:");
#ifdef VOLATILE
		mark_volatile();
#endif

#ifdef DEBUG
		if (junk_is) goto schedule;
#else
		if (debug_is) goto schedule;
#endif
		bugout("tidy");
		tidy();
		bugdump("9: after tidy, code is:");
		bugout("canonize");
		canonize();
		bugdump("9: after canonize, code is:");
		bugout("movezero");
		movezero();
		bugdump("9: after movezero, code is:");
		bugout("movedat");
		movedat();
		bugdump("9: after movedat, code is:");
		change_occured = false;
		iteration_count = 0;
		do {

			refcount();
			bugdump("9: after refcount, code is:");

			do {
				change_occured=false;
				bugout("flow_control");
				flow_control();
				bugdump("9: after flow_control, code is:");
				bugout("forget current status");
				status_unknown();
				iteration_count++;
			} while (change_occured);

#ifdef DEBUG
			bugout("ext_remove");
			ext_remove();
			bugdump("9: after ext_remove, code is:");
#endif DEBUG
			bugout("comjump");
			comjump();
			bugdump("9: after comjump, code is:");

			if (bit_test_ok)	/* default true */
			{
				bugout("bit_test");
				bit_test();
				bugdump("9: after bit_test, code is:");
			}

			bugout("rmove");
			rmove();
			bugdump("9: after rmove, code is:");
			bugout("ultimate_destination");
			ultimate_destination();
#ifdef M68020
			if (ult_881)
			ultimate_881_destination();
#endif M68020
#ifdef DRAGON
			if ((dragon || dragon_and_881) && ult_dragon)
			  ultimate_dragon_destination();
#endif DRAGON
			bugdump("9: after ultimate, code is:");

			bugout("jumpsw");
			jumpsw();
			bugdump("9: after jumpsw, code is:");
			bugout("collapse");
			collapse();
			bugdump("9: after collapse, code is:");

			if (exit_exits) {	/* default false */
				bugout("combine exit");
				exits();
				bugdump("9: after exits, code is:");
			}

		} while (change_occured);

		bugout("decanonize");
		decanonize();
		bugdump("9: after decanonize, code is:");
		bugout("tidy");
		tidy();
		bugout("9: tidy complete");
		refcount();
		bugdump("9: after refcount, code is:");

		if (leaf_movm_reduce && !alt_ent_seen)           /* default true */
		{
		  movm_reduce();
#ifdef M68020
		  fmovm_reduce();
#endif M68020
#ifdef DRAGON
		  if (dragon || dragon_and_881)
		    fpmovm_reduce();
#endif DRAGON
		}

#ifdef M68020
		if (eliminate_link_unlk && !alt_ent_seen)	/* default true */
		{
		  if (remove_link_unlk() == 0)
		    stack_adjust_before_unlk();
		}
#endif M68020
		if (oforty) {
			lalign_bras();
		}


		if (span_dep_opt)	/* default true */
		{
			refcount();
			bugdump("9: after refcount, code is:");
			bugout("span_dep");
			span_dep();
			bugdump("9: after span_dep code is:");
		}

#ifdef DRAGON
		/*
		 * do this after span dependendent to avoid the complications
		 * of combined move ops in the span dependent code 
		 */
		if (combine_dragon)
		if (dragon || dragon_and_881)
			combine_move_op();
#endif DRAGON

	schedule:
		if (inst_sched && !dragon && !dragon_and_881) {
			instsched();
		}

out:
		bugout("output");
		output();
		fflush(stdout);
		bugout("release_list");
		release_list(first.forw);
		first.forw=NULL;
		if (iteration_count > iteration_max)
			iteration_max = iteration_count;

	} while (isend);

#ifdef DEBUG
	if (debug5) {
		bugout("About to check if memory freed");
		memory_check();
	}
#endif DEBUG

	bugout("About to print statistics");
	if (print_statistics) {
		printf("** %d iterations\n", iteration_max);
		printf("** %d jumps to jumps\n", nbrbr);
		printf("** %d instructions after jumps\n", iaftbr);
		printf("** %d jumps to the next instruction\n", njp1);
		printf("** %d redundant labels\n", nrlab);
		printf("** %d cross jumps\n", nxjump);
		printf("** %d code motions\n", ncmot);
		printf("** %d branches reversed\n", nrevbr);
		printf("** %d redundant moves\n", redunm);
		printf("** %d simplified addresses\n", nsaddr);
		printf("** %d loops inverted\n", loopiv);
		printf("** %d redundant jumps\n", nredunj);
		printf("** %d common sequences before jump's\n", ncomj);
		printf("** %d skips over jumps\n", nskip);
		printf("** %d redundant tst's\n", nrtst);
	}
	bugout("About to engage in final exit");
	exit(0);
	bugout("Wow!  We survived the exit?");
}

input()
{
	register node *p, *lastp;
	register int op, subop;
	register char *save, *cp, ch;
	static node *saved = NULL;

	asm_seen = false;
	alt_ent_seen = false;

	/* Is there any leftover input from the last time? */
	/* If so, let first.forw point to that */
	if (saved!=NULL) {
		first.forw = saved;
		saved->back = &first;
		saved = NULL;
	} else
		first.forw = NULL;

	/* have lastp point to the last node */
	for (lastp = &first; lastp->forw!=NULL; lastp=lastp->forw)
		;
#ifdef VERBOSE
	if (lastp->op == GLOBAL)
	    fprintf(stderr,"Optimizing %s\n",lastp->string1);
#endif VERBOSE

	if (pnum)
	{
	  int i;
	  asm_seen = true;
	  for(i=0; i<pnum; i++)
	  {
	    if (!strcmp(procname[i],lastp->string1))
	    {
	      /* optimize this procedure */
	      asm_seen = false;
	    }
	  }
	}


	for (;;) {
		op = getline();
		subop = (op>>16)&0377;	/* subop is in high word of op */
		op &= 0xffff;
		switch (op) {

		case LABEL:
			p = getnode(LABEL);
			if (*input_ptr++!='L')
				internal_error("Compiler label doesn't begin with L?\nline: %s",
					input_line);
			p->labno1 = atoi(input_ptr);

			if (p->labno1 >= first_c2_label) {
		           internal_error(
			      "maximum label number exceeded, source file must be split"

			   );
			}

			/* Skip over the numeric label */
			while (isdigit(*input_ptr))
				input_ptr++;
			if (*input_ptr++!=':')
				internal_error("Compiler label doesn't end with :?\nline: %s",
					input_line);
			break;

		case DLABEL:
			p = getnode(DLABEL);
			p->mode1 = ABS_L;
			p->type1 = STRING;
			for (cp=input_ptr; *cp != ':'; cp++);
			*cp++ = '\0';
			p->string1 = copy(input_ptr);
			input_ptr = cp;
			break;

		case ASM:
			asm_seen = true;
			p = getnode(COMMENT);
			p->mode1 = ABS_L;
			p->type1 = STRING;
			p->string1 = copy(input_ptr);
			break;

#ifdef OUT
		case COMMENT:
			/* fortran do-loop stuff is no longer needed,
			 * but other commnets may be */
			if (fort && (subop==1))
				/* fortran DO loop */
				if (lastp->op == CBR) lastp->info = 1;
			continue;
#endif OUT

		case COMMENT:
			switch (subop)
			{
				case 9:
					/*
			 		 * Create the save list.
			 		 */
					if (lastp->op==TEXT) {
						saved = lastp;
						saved->back->forw=NULL;
						saved->back=NULL;
					}
					else {
						saved = NULL;
					}
					return(1);
#ifdef VOLATILE
				case 7:
				case 8:
					/* add item to vlist */
					p = getnode(COMMENT);
					parse(input_ptr,p);
					append_to_vlist(p,subop==7);
#endif
				default:
					continue;
			}

		case DC:
			p = getnode(DC);
			if (subop==LONG && *input_ptr== 'L') {
			/*
			 * look for jump table for switch statements
			 * this is of the form long Lxx-Lyy
			 */
				save = input_ptr;
				input_ptr++;			/* skip L */
				p->labno1 = getnum(input_ptr);	/* get number */
				while (isdigit(*input_ptr))
					input_ptr++;
				if (*input_ptr == NULL) 
				{
					p->subop = subop;
					p->mode1= ABS_L;
					p->type1 = INTLAB;
					break;
				}
				else if (*input_ptr++ != '-' || 
					 *input_ptr++ != 'L') 
				{
					input_ptr = save;
					goto plain;
				}
				p->labno2 = getnum(input_ptr);
				p->mode1 = p->mode2 = ABS_L;
				p->type1 = p->type2 = INTLAB;
				p->op = JSW;
				p->subop = UNSIZED;	/* implicit long */
				break;
			}
			/* plain old dc.b/dc.w/dc.l */
plain:
			p->subop = subop;
			p->string1 = copy(input_ptr);
			p->mode1 = ABS_L;
			p->type1 = STRING;
			break;

		case END:
			p = getnode(END);
			break;

		/* everything else */
		default:
			p = getnode(op);
			p->subop = subop;
			parse(input_ptr, p);
			break;

		} /* end of switch */


		/*
		 * Link our new node into the list
		 */

		p->ref = NULL;		/* no reference */
		p->forw = NULL;		/* we are the end of the list */
		p->back = lastp;	/* link to previous */
		lastp->forw = p;	/* previous point to us */
		lastp = p;		/* we are last node now */

		/*
		 * alternate entry points in Fortran cause a "global"
		 * pseudo-op to be generated inside a procedure. Hence
		 * encountering a "global" which is not for an alternate
		 * entry point would indicate the beginning of a new
		 * procedure.
		 */
		if (op==GLOBAL && !alternate_entry_pt && p->back != &first) {

			/*
			 * Create the save list.
			 *
			 * This will consist of GLOBAL
			 * or perhaps TEXT/GLOBAL.
			 */
			saved = p;	/* include GLOBAL at least */
			if (p->back->op==TEXT)
				saved = p->back;

			/*
			 * Pinch off the saved list from the list
			 */
			saved->back->forw=NULL;
			saved->back=NULL;
		}

		/*
		 * How was this hunk of code terminated?
		 *
		 * If we found an END, return zero to tell them
		 * not to call us anymore.
		 */

		if (op==END) return(0);

		if (op==GLOBAL && !alternate_entry_pt && p->back != &first) {
		   return(1);
		}

		if (alternate_entry_pt) 
		{
			alternate_entry_pt = false;
			alt_ent_seen = true;
		}

	} /* end of for */
}   /* end of function */



/*
 * opcode = getline();
 *
 * Getline() returns an opcode, or END if no more input.
 * The pointer input_ptr is set to the next character in the input.
 */

int
getline()
{
	register int c, opcode, len;
	static boolean input_required=true;
	static boolean in_asm=false;


  again:
	if (input_required) {
#ifdef DEBUG
		if (debug2 && isatty(fileno(stdin))) {
			printf("input:\t");
			fflush(stdout);
		}
#endif DEBUG
		input_ptr = input_line;
		if (fgets(input_line, sizeof(input_line), stdin)==NULL) {
			bugout("2: getline: end of file");
			input_line[0]='\0';
			return END;
		}
		input_required=false;
		len = strlen(input_line);
		if (len>0 && input_line[--len]=='\n')	/* if newline: */
			input_line[len]='\0';		/* strip it off */
#ifdef DEBUG
		bugout("2: getline: line: %s", input_line);
#endif DEBUG
	}

	/* get the first character of the line */
	c = *input_ptr;			/* get the character */


	/* is this a comment line? */
	if (c=='#') {
		c = *++input_ptr;		/* get the character */
		 /*
		  * Certain comments are reserved to pass information
		  * from the code-generator to the optimizer.
		  * 1. asm_begin and asm_end are used to mark the
		  *    beginning and end of an asm statement. This is
		  *    important because the optimizer is designed only
		  *    to understand what the code generator produces,
		  *    and cannot handle hand-written assembly. Any 
		  *    procedure containing an asm statement will be
		  *    output un-optimized, unless the user uses the
		  *    -A option. (default false)
		  * 2. alternate_entry_point is used to indicate that
		  *    the following "global" pseudo-op is not the
		  *    beginning of a new procedure, but due to an
		  *    alternate entry point in Fortran.
		  */
		if (!allow_asm && c == '2')
			in_asm = true;
		else if (!allow_asm && c == '3')
			in_asm = false;
 		/*
 		 * problem: if you use -F -F to toggle fortran 
 		 * optimizations out, alt entry point will create
 		 * a problem, hence this check must be even if fort is false 
 		 */
 		else if (c == '4')
 				alternate_entry_pt = true;
#ifdef OUT
		else if (fort)
		     {
			/* comments from fortran compiler */
			/* do-loop comment no longer needed */
			if (c == '1')
			{
			  /* #1 -- DO loop w/no jumps outside */
			  input_required=true;		/* used up this line */
			  return(COMMENT + (1<<16));
			}
		     }
#endif OUT
		else if (c == '5') {
			++input_ptr;
			input_required = true;
			return(COMMENT + ((c-'0')<<16));
		}
#ifdef VOLATILE
		else if (c == '7' || c == '8')
		{
			++input_ptr;
			input_required = true;
			return(COMMENT + ((c-'0')<<16));
		}
#endif
		else if (c == '9') {
			++input_ptr;
			input_required = true;
			return(COMMENT + ((c-'0')<<16));
		}

		input_required=true;		/* used up this line */
		goto again;			/* and try again */
	}

	if (in_asm) { 				/* part of asm statement */
		input_required=true;
		return(ASM);
	}

	if (c=='L' && isdigit(input_ptr[1]))	/* Is this a label generated */
		return(LABEL);			/* by the compiler?  (L0123) */

	if (isgraph(c))			/* Is it a user label? */
	  	return(DLABEL);

	/*
	 * After we process this possible opcode, we'll need another line,
	 * so let's just set the flag now.
	 */
	input_required=true;

	while (isspace(*input_ptr))		/* skip white space */
		input_ptr++;			/* before the opcode */

	if (*input_ptr=='\0' || *input_ptr=='#')/* is there any opcode? */
		goto again;			/* no opcode, get next line */

	opcode = oplook();

#ifdef DEBUG
	bugout("2: opcode=%d", opcode);
#endif DEBUG
	return (opcode);
}



getnum(p)
register char *p;
{
	register n, c;

	n = 0;
	while (isdigit(c = *p++))
		n = n*10 + c - '0';
	if (*--p!='\0' && *p!='-')
		return(0);
	return(n);
}





oplook()
{
	register subop;
	register char *op;
	register int *p, opcode;
	register int low, high;
	char operand[32];

	/* put the opcode into operand */
	op = operand;
	while (isgraph(*input_ptr))
		*op++ = *input_ptr++;
	*op = '\0';

	/* skip the whitespace after the opcode */
	while (isspace(*input_ptr))
		++input_ptr;

	/* determine what the subop (size) is */
	subop = UNSIZED;			/* default */

	/* Cheaply transform beq.l into beq */
	if (operand[0]=='b' && operand[3]=='.') {
		*--op = '\0'; *--op = '\0';	/* remove the .l */
	}

	if (op[-2]=='.') {
		switch (op[-1]) {
		case 'b':
			subop=BYTE;
			break;
		case 'w':
			subop=WORD;
			break;
		case 'l':
			subop=LONG;
			break;
#ifdef M68020
		case 's':
			subop=SINGLE;
			break;
		case 'd':
			subop=DOUBLE;
			break;
		case 'x':
			subop=EXTENDED;
			break;
#endif
		default:
			internal_error("oplook: bad size in line %s",
				input_line);
		}
		*--op = '\0'; *--op = '\0';	/* remove the .b */
	}

	/* Binary search through opcode table */
	low=0; high=sizeof(op_info)/sizeof(struct op_info)-2;

	while (low<=high) {
		register int middle;
		register int i;
		register char *midop;
		middle = (low+high)>>1;
		midop = op_info[middle].opstring;
		/* i = strcmp(operand, op_info[middle].opstring); */

		op = operand;
		while(*op == *midop++)
			if(*op++ == '\0') 
			{
			/*
			 * We have a match, translate it.
			 */
			opcode = op_info[middle].opcode;
			for (p=input_map; *p!=0; p+=2)
				if (opcode==*p) {
					opcode = *(p+1);
					break;
				}
			return (opcode + (subop<<16));
			}

		i = *op - *--midop;

		if (i>0)
			low=middle+1;
		else if (i<0)
			high=middle-1;
	}

	internal_error("oplook: can't find opcode in \"%s\"", input_line);
	return(0);
}



/*
 * Return true (i.e. 1) if the operator of the node "p" is a
 * pseudo-op, label, or unrecognized instruction.  This information is
 * used to prevent eliminating instructions after a jump that don't
 * need to be executed to be significant.
 */
pseudop(p)
register node *p;
{
	if (!p) return(true);	/* we call non-existant nodes pseudo-ops */

	switch (p->op) {
	case DLABEL:
	case LABEL:
	case GLOBAL:
	case SGLOBAL:
	case TEXT:
	case DATA:
	case LALIGN:
	case DS:
	case DC:
	case END:
	case JSW:
	case BSS:
	case SET:
	case COMM:
	case LCOMM:
	case VERSION:
	case SHLIB_VERSION:
		return(true);

	default:
		return(false);
	}
}




/*
 * Scan through the nodes counting the number of references to each label.
 * Make each thing point to the lowest-numbered of a group of labels.
 * When done, if a label has no references and the instruction
 * following is not an equate, dc.w (JSW), or an unrecognized
 * instruction (for safety), the label is deleted.
 */

static struct hash_info *hash_info_free = NULL;

#define HASHSIZE 256	/* must be power of two for the pseudo-mod operation */
struct hash_info {
	node	*pointer;		/* pointer to label */
	struct hash_info *next;		/* linked list for collisions */
};

refcount()
{
	register node *p, *lp;
	struct hash_info labhash[HASHSIZE];
	struct hash_info *get_hash_info();
	register struct hash_info *hp;
	int op1isstringintlab;
	int op2isstringintlab;

	/* clear the hash table */
	for (hp = labhash; hp < &labhash[HASHSIZE]; hp++)
	{
		hp->pointer = NULL;
		hp->next = NULL;
	}

	/* fill the hash table (take care of collisions via a linked list) */
	for (p = first.forw; p!=NULL; p = p->forw)
		if (p->op==LABEL) {
			hp = &labhash[p->labno1 & (HASHSIZE-1)] ;
			if (hp->pointer == NULL)
			{
				hp->pointer = p;
				hp->next = NULL;
			}
			else
			{
			        register struct hash_info *new;

				new = get_hash_info();

				new->next = hp->next;
				hp->next = new;
				new->pointer = p;
			}
			p->refc = 0;
		}


	for (p = first.forw; p!=NULL; p = p->forw) {
	register node *best;
	register int label;

		if (p->op==LABEL)
			continue;

		op1isstringintlab = 0;
		op2isstringintlab = 0;

		if (p->type1==INTLAB) {
			label = p->labno1;
		}
		else if (p->type2==INTLAB) {
			label = p->labno2;
		}
		else if ((p->type1==STRING) && (p->string1[0]=='L') &&
			 (p->string1[1]>='0') && (p->string1[1]<='9')) {
			label = atoi(&p->string1[1]);
			op1isstringintlab = 1;
		}
		else if ((p->type2==STRING) && (p->string2[0]=='L') &&
			 (p->string2[1]>='0') && (p->string2[1]<='9')) {
			 label = atoi(&p->string2[1]);
			op2isstringintlab = 1;
		}
		else {
			continue;
		}

		p->ref = NULL;

		/* look for the label */
		hp = &labhash[label & (HASHSIZE-1)];
		lp = hp->pointer;

		if (lp==NULL)			/* If can't find target */
			continue;		/* Can't be helped */

		/* if not in hash table, look at linked list */
		if (label!=lp->labno1)
			while(hp=hp->next) {
				lp = hp->pointer;
				if (label==lp->labno1)
					break;
			}

		if (label!=lp->labno1) continue;

		/*
		 * Gather references to the lowest numbered label of a group.
		 * This tends to preserve original labels.
		 * This way the others will be unreferenced and get annihilated.
		 */

		/* find the first of the group */
		while (lp->back!=NULL && lp->back->op==LABEL)
			lp = lp->back;

		/* find the lowest of the group */
		best = NULL;
		for (; lp!=NULL && lp->op==LABEL; lp = lp->forw)
			if (best==NULL || lp->labno1 < best->labno1)
				best = lp;

		if (best==NULL)
			internal_error("refcount: null best???");

		if (p->type1==INTLAB) {
			p->labno1 = best->labno1;
		}
		else if (p->type2==INTLAB) {
			p->labno2 = best->labno1;
		}
		else if (op1isstringintlab) {
			replacelab(p->string1,best->labno1);
		}
		else if (op2isstringintlab) {
			replacelab(p->string2,best->labno1);
		}
		p->ref = best;
		best->refc++;
	}


	/* go through the list, delete non-referenced labels */
	for (p = first.forw; p!=NULL; p = lp) {
		lp = p->forw;
		if (p->op==LABEL && p->refc==0
		    && lp!=NULL && 
		    (lp->op==LABEL || lp->op==DLABEL || !(pseudop(lp))))
			decref(p);
	}

	/* free up the hash table */
	for (hp = labhash; hp < &labhash[HASHSIZE]; hp++)
	{
		struct hash_info *hp2, *hp3;

		hp2 = hp->next;

		while (hp2)
		{
			hp3 = hp2->next;

			free_hash_info(hp2);

			hp2 = hp3;
		}
	}

}

struct hash_info *get_hash_info()
{
        struct hash_info *h;

	if (hash_info_free)
	{
		h = hash_info_free;

		hash_info_free = hash_info_free->next;
	}
	else
	{
		h = (struct hash_info *) malloc(sizeof(struct hash_info));
	}

	return(h);
}

free_hash_info(h)
struct hash_info *h;
{
	h->next = hash_info_free;

	hash_info_free = h;
}

replacelab(c,l)
char *c;
int  l;
{
	char  lab[12];
	char *p1,*p2;

	sprintf(lab,"%d",l);
	p1 = c + 1;
	p2 = lab;
	while (*p2) {
		*p1++ = *p2++;
	}
	while ((*p1 >= '0') && (*p1 <= '9')) {
		p2 = p1; 
		while (*p2) {
			*p2 = *(p2+1);
			p2++;
		}
	}

}

#ifdef DEBUG
/*
 * Remove ext's someday.
 */
ext_remove()
{
}
#endif DEBUG


