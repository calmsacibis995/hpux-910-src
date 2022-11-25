/* file utils.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)utils.c	16.2 90/11/15 */

# include <signal.h>

# include "c1.h"

# define SYMLONGS (NCHNAM+1)/sizeof(int)
# define FTABINCR	10
# define FIRST_PARAM_OFFSET 8

/* Used by rm_far_aliases() and its support routines */
typedef struct
	{
	NODE *assign;
	NODE *local;
	ushort larrayindex;
	ushort farrayindex;
	} FBLOCK;

/* variables shared with c1.c only */
extern LBLOCK 	*lastlbl;
extern CGU 	*llp;		/* array of labels for latest computed goto */
extern NODE   	*lastnode;	/* ptr to last used node in current block. */
extern int 	nlabs;
extern int 	ficonlabmax;	/* size of asgtp array */
extern flag	need_ep;
extern flag 	need_block;
extern flag 	postponed_nb;
extern flag	assigned_goto_seen;	/* ASSIGNED GOTO seen in this func. */
extern flag	asm_flag;
extern unsigned char breakop;
extern unsigned char lastop;
extern char lcrbuf[];

LOCAL BBLOCK 	*endblock;
LOCAL LBLOCK 	**ltable;
LOCAL LBLOCK	*toplbl;	/* ptr to first label in table (if any). */
LOCAL LBLOCK 	*labelpool;	/* ptr to LBLOCK table */
LOCAL LBLOCK 	*endlblp;
LOCAL int 	maxlblsz;
LOCAL FBLOCK 	*ftab;
LOCAL FBLOCK 	*ftabend;
LOCAL FBLOCK 	*lastftab;
LOCAL int 	maxftabsz;

FILE 		*lrd;  		/* for default reading routines */
FILE 		*outfile;	/* for normal output */
BBLOCK		*blockpool;	/* ptr to BBLOCK table */
int  		lastn;
int  		numblocks;	/* number of basic blocks */
# ifdef COUNTING
	FILE	*countp;	/* for counts */
# endif COUNTING
# ifdef DEBUGGING
	FILE	*debugp;	/* for debugging output */
# endif DEBUGGING

extern NODE 	*basic_node();
LOCAL unsigned 	poplbl();

extern NODE     *makety();

struct comment_buf *comment_head = NULL;


/******************************************************************************
*
*	read_comment()
*
*	Description:		read_comment - reads an FTEXT record from 
*				stdin.  FTEXT records are saved so that 
*				they can be emitted while preserving their
*				relationship to the FLBRAC records.  Those that
*				came before and FLBRAC stay before the FLBRAC.
*				Those that came after an FLBRAC stay after 
*				the FRBRAC record.
*
*	Called by:		main()
*
*	Input parameters:	a - a copy of the FTEXT long word.
*
*	Output parameters:	none
*
*	Globals referenced:	lrd
*
*	Globals modified:	comment_head
*
*	External calls:		cerror()
*				fread()
*
*******************************************************************************/
void read_comment( a ) int a; {
	register n;
	struct comment_buf *com_buf = 
		(struct comment_buf *) ckalloc(sizeof(struct comment_buf));

	com_buf->ftextint = a;
	com_buf->comment = NULL;
	com_buf->next = comment_head;
	comment_head = com_buf;

	n = VAL(a);
	if( n > 0 ){
		com_buf->comment = (char *) ckalloc(n * 4); 
		if( fread( com_buf->comment, sizeof(int), n, lrd ) != n )
#pragma BBA_IGNORE
			cerror( "intermediate file read error" );

		}
	}
/******************************************************************************
*
*	write_comments()
*
*	Description:		write_comments - write FTEXT records to stdout 
*				All FTEXT records have been saved so that
*				they can be emitted after the FRBRAC record.
*				write_comments is called recursively so that
*				the linked list of FTEXT records can be
*				written out in the same order they were 
*				read in.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	outfile
*
*	Globals modified:	comment_head
*
*	External calls:		cerror()
*				fwrite()
*
*******************************************************************************/
void write_comments(list) struct comment_buf *list;  {
	int n;

	if (list != NULL)
	  {
	  write_comments(list->next);

	  p2flush();
	  if (fwrite(&(list->ftextint), sizeof(int), 1, outfile) != 1)
#pragma BBA_IGNORE
	    goto outerror;
	  n = VAL(list->ftextint);
	  if (fwrite(list->comment, sizeof(int), n, outfile) != n)
#pragma BBA_IGNORE
	    goto outerror;
	  if (list->comment) FREEIT(list->comment);
	  FREEIT(list);
	  return;
outerror: cerror("output file error");
	  }
	}

/******************************************************************************
*
*	lccopy()
*
*	Description:		lccopy - copies stuff from stdin into outfile
*				while checking for overflow.
*
*	Called by:		p2pass()
*
*	Input parameters:	a - a copy of the FTEXT long word.
*
*	Output parameters:	none
*
*	Globals referenced:	outfile
*				lrd
*				xdebug
*				debugp
*
*	Globals modified:	none
*
*	External calls:		cerror()
*				fwrite()
*				fread()
*				fprintf()
*
*******************************************************************************/
void lccopy( a ) int a; {
	register n;
	static char fbuf[FBUFSIZE];
	n = VAL(a);
	if( n > 0 ){
		p2flush();
		if( n > FBUFSIZE/4 )
#pragma BBA_IGNORE
			cerror( "lccopy asked to copy too much" );
		if (fwrite(&a, sizeof(int), 1, outfile ) != 1)
#pragma BBA_IGNORE
			goto outerror;
		if( fread( fbuf, sizeof(int), n, lrd ) != n )
#pragma BBA_IGNORE
			cerror( "intermediate file read error" );

#ifdef DEBUGGING
		if ( xdebug ) {
			register i, j;
			fprintf(debugp, "\t");
			for (j=0, i = n*4; i>0; i--,j++) fprintf(debugp, "%c ", fbuf[j]);
			fprintf(debugp, "\n");
		}
#endif	DEBUGGING
# ifndef NOOUTPUT	
		if( fwrite( fbuf, sizeof(int), n, outfile ) != n )
#pragma BBA_IGNORE
			goto outerror;
# endif	NOOUTPUT
		}
	return;
outerror:
	cerror("output file error");
	}

/******************************************************************************
*
*	lcread()
*
*	Description:		reads n int into a buffer lcrbuf. Bytes are
*				later read as chars.
*
*	Called by:		main()
*
*	Input parameters:	n - number of longs to be read
*
*	Output parameters:	none
*
*	Globals referenced:	lcrbuf
*				lrd
*				xdebug
*
*	Globals modified:	lcrbuf
*
*	External calls:		fread()
*				cerror()
*
*******************************************************************************/
void lcread( n ) {
	register char *cp;

	if( n > 0 ){
		if( fread( lcrbuf, 4, n, lrd ) != n )
#pragma BBA_IGNORE
			cerror( "intermediate file read error" );
		/* until f77pass1 puts out well formed asciz names, the
		   following is needed to remove blank pads.
		*/
		for (cp = lcrbuf; *cp && *cp != ' '; cp++) ;
		*cp = '\0';
		}
#	ifdef DEBUGGING
	if (xdebug) lcrdebug(lcrbuf, n);
#	endif	DEBUGGING
	}

/******************************************************************************
*
*	lstrread()
*
*	Description:		lstrread reads a series of int from lrd until
*				reading a null char. Used primarily to read
*				arbitrary length external ids.
*
*	Called by:		main()
*				do_hiddenvars()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	lrd
*				lcrbuf
*				xdebug
*
*	Globals modified:	lcrbuf
*
*	External calls:		cerror()
*				fread()
*
*******************************************************************************/
void lstrread()
{
	register short 	i;
	register short 	j = 0;
	register char 	*lcp;
	register int	*cp = (int *)lcrbuf;

	while (++j <= SYMLONGS)
		{
		lcp = (char *) cp;
		if ( fread(cp++, sizeof(int), 1, lrd) != 1 )
#pragma BBA_IGNORE
			cerror("intermediate file read error");
		for (i=1; i<=sizeof(int); i++)
			if (! *lcp++) 
				{
#ifdef DEBUGGING
				if (xdebug) lcrdebug( lcrbuf, j);
#endif	DEBUGGING
				return; /* return at first null byte */
				}
		}
	return;
}

/******************************************************************************
*
*	lopen
*
*	Description:		opens files from argv or stdin.
*
*	Called by:		flags_and_files()
*
*	Input parameters:	s - a character string filename
*				readonly - a flag describing open status.
*
*	Output parameters:	none
*
*	Globals referenced:	stdin
*
*	Globals modified:	none
*
*	External calls:		fopen()
*				cerror()
*
*******************************************************************************/
LOCAL FILE *lopen( s, readonly ) char *s; flag readonly;
{
	FILE *llrd;
	/* if s null, opens the standard input */
	if( *s ){
		llrd = fopen( s, readonly? "r" : "w" );
		if( llrd == NULL ) 
#pragma BBA_IGNORE
			cerror( "cannot open intermediate file %s", s );
		}
	else  llrd = stdin;
	return (llrd);
}

/******************************************************************************
*
*	bad_fp()
*
*	Description:		bad_fp() is a floating point exception signal
*				handler.
*
*	Called by:		flags_and_files() indirectly thru signal().
*
*	Input parameters:	sig	( See signal() )
*				code
*				scp
*
*	Output parameters:	none
*
*	Globals referenced:	SIGFPE
*
*	Globals modified:	none
*
*	External calls:		cerror()
*
*******************************************************************************/
#pragma BBA_IGNORE
LOCAL bad_fp(sig, code, scp)
{
	cerror("floating point exception");
}


/******************************************************************************
*
*	flags_and_files()
*
*	Description:		opens pertinent input, output and debug files,
*				sets signal handlers, and controls the setting
*				of flags.
*
*	Called by:		main()
*
*	Input parameters:	argc
*				argv
*
*	Output parameters:	none
*
*	Globals referenced:	stderr
*				nerrors
*				count_file
*
*	Globals modified:	files
*				debugp
*				outfile
*				countp
*
*	External calls:		p2init()
*				signal()
*				setbuf()
*				cerror()
*				setvbuf()
*				lopen()
*				fopen()
*
*******************************************************************************/
int flags_and_files(argc, argv) register char *argv[];
{
	register int files;

	files = p2init( argc, argv );
	signal(SIGFPE, bad_fp);
	if( files ){
		while( files < argc && argv[files][0] == '-' ) {
			++files;
			}
		if( files > argc ) return( nerrors );
		/* first file in arglist is the input file */
		/* second file is the debug file, if there are 3 */
		lrd = lopen( argv[files], YES );
# ifdef DEBUGGING
		if (files < argc - 2)
			{
			debugp = lopen(argv[files+1], NO);
			outfile = lopen(argv[files+2], NO);
			}
		else
			{
			debugp = stderr;
			outfile = lopen(argv[files+1], NO);
			}
		setbuf(outfile, 0);		/* No buffering */
		if (debugp != stderr)
			setbuf(debugp, 0);
# else
		if (files < argc - 2)
#pragma BBA_IGNORE
			cerror("too many file names");
		else
			outfile = lopen(argv[files+1], NO);
		setvbuf(outfile, NULL, _IOFBF, 8192);	/* Big buffering */
# endif DEBUGGING
# ifdef COUNTING
    		if (count_file && (countp = fopen( count_file, "w" )) == NULL)
			cerror("cannot open count file \"%s\"", count_file);
# endif COUNTING
		}
	else lopen( "", YES );
	return(0);
}

/******************************************************************************
 *
 *	DO_HIDDENVARS()
 *
 *	Description:		Process hidden vars for a call.  Read var
 *				descriptors, add to table, attach table to
 *				CALL or UNARY CALL node.
 *
 *	Called by:		main()
 *
 *	Input parameters:	hp - a ptr to the newly created hiddenvars
 *					table.
 *
 *	Output parameters:	none
 *
 *	Globals referenced:	debugp
 *				xdebug
 *				f_find_array
 *				f_find_struct
 *				lcrbuf
 *
 *	Globals modified:	f_find_array
 *				f_find_struct
 *
 *	External calls:		addtreeasciz()
 *				cerror()
 *				find()
 *				fprintf()
 *				lread()
 *				lstrread()
 *				talloc()
 *
 ******************************************************************************
 */

void do_hiddenvars(hp)	register HIDDENVARS *hp;
{
	register NODE *p;
	register int x;
	register int a;

#ifdef DEBUGGING
	if (xdebug)
		fprintf(debugp, "\tnitems = %d", hp->nitems);
#endif DEBUGGING
	allow_insertion = YES;
	for (a = 0; a < hp->nitems; ++a)
	    {
	    x = lread();
#ifdef DEBUGGING
	    if( xdebug )
		fprintf( debugp, "\nop=%s., val = %d., rest = 0x%x\n",
			xfop(FOP(x)), VAL(x), (int)REST(x) );
#endif	DEBUGGING
	    if (FOP(x) == C1HVOREG)
		{
		p = talloc();
		p->tn.op = OREG;
		if (VAL(x))
		    f_find_array = YES;
		else if (REST(x))
		    f_find_struct = YES;
		p->tn.lval = lread();
#ifdef DEBUGGING
		if( xdebug )
		    fprintf( debugp, "\toffset = %d\n", p->tn.lval);
#endif	DEBUGGING
		hp->var_index[a] = find(p);
		f_find_array = NO;
		f_find_struct = NO;
		p->in.op = FREE;	/* throw it away now */
		}
	    else if (FOP(x) == C1HVNAME)
		{
		p = talloc();
		p->tn.op = NAME;
		if (VAL(x))
		    f_find_array = YES;
		else if (REST(x))
		    f_find_struct = YES;
		p->tn.lval = lread();
		lstrread();	/* fills lcrbuf */
		p->atn.name = addtreeasciz(lcrbuf);
#ifdef DEBUGGING
		if( xdebug )
		    fprintf( debugp, "\tname = \"%s\", offset = %d\n",
			     p->atn.name, p->tn.lval);
#endif	DEBUGGING
		hp->var_index[a] = find(p);
		f_find_array = NO;
		f_find_struct = NO;
		p->in.op = FREE;	/* throw it away now */
		}
	    else
		{
#ifdef DEBUGGING
		fprintf( debugp,
#else
		fprintf( stderr,
#endif DEBUGGING
		    "unexpected opcode = %d in do_hiddenvars\n", FOP(x) );
		cerror("bad opcode in do_hiddenvars()");
		}
	    }
	allow_insertion = NO;
}

/******************************************************************************
 *
 *	COMPLETE_ASGTP_TABLE()
 *
 *	Description:		complete_asgtp_table() fills out the asgtp
 *				table then it associates a refined copy of
 *				this table to GOTO nodes, thereby completing
 *				the information necessary to compute
 *				predecessor-successor relationships later.
 *			
 *
 *	Called by:		main()
 *
 *	Input parameters:	none
 *
 *	Output parameters:	none
 *
 *	Globals referenced:	asgtp
 *				assigned_goto_seen
 *				ficonlabsz
 *				lastblock
 *				lastlbl
 *				llp
 *				nlabs
 *				topblock
 *				toplbl
 *
 *	Globals modified:	llp
 *				nlabs
 *
 *	External calls:		ckalloc()
 *
 ******************************************************************************
 */

void complete_asgtp_table()
{
	register CGU *cp;
	register BBLOCK *bp;
	register CGU *xllp;
	CGU *tabend;

	if (!assigned_goto_seen)
		return;

	/* ASSIGNED GOTOs will be handled almost as though they were computed
	   gotos (with the exception that there is no default fall thru branch).
	   It must be assumed that any label number used in an ASSIGN statement
	   can be a target of the goto except those which label non-executable
	   statements.
	*/

	/* The first task is to eliminate labels on non-executable statements.
	   These have no basic block associated with them.
	*/
	tabend = &asgtp[ficonlabsz-1];
	nlabs = 0;
	for (cp = tabend; cp >= asgtp; cp--)
		{
		register LBLOCK *lp;
		for (lp = toplbl; lp <= lastlbl; lp++)
			if (lp->val == cp->val)
				{
				if (lp->bp)
					{
					cp->nonexec = NO;
					nlabs++;
					}
				else lp->preferred = NO;
				break;
				}
		}

	/* Attach a well-formed cgu table to each ASSIGNED GOTO block */
	for (bp = lastblock; bp >= topblock; bp--)
		if (bp->b.breakop == GOTO)
			{
			register NODE *np;

			np = bp->b.treep;
			while (np->in.op == SEMICOLONOP)
				np = np->in.right;
			if (np->in.op == UNARY SEMICOLONOP)
				np = np->in.left;
			if (np->in.op == GOTO)
				{
				xllp = (CGU *)ckalloc(nlabs * sizeof(CGU));
				bp->cg.ll = xllp;
				np->cbn.ll = xllp;
				np->cbn.nlabs = nlabs;
				bp->cg.nlabs = nlabs;
				for (cp = asgtp; cp <= tabend; cp++)
					if (!cp->nonexec)
						xllp++->val = cp->val;
				}
			}

}  /* complete_asgtp_table() */

/*****************************************************************************
 *
 *  PRE_ADD_ARRAYELEMENTS() 
 *
 *  Description:  Add first element of static arrays to symtab.  
 *                The AN records would get added anyway, but not 
 *                as arrayelements.  This way we're sure they're 
 *                in as arrayelements.  Should reduce the number 
 *                of static items for C --> fewer definitions.
 *
 *  Called by:		main()    
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: lastfilledsym
 *                      symtab 
 *
 *  Globals Modified:   none
 *
 *  External Calls: 	insert_array_element()
 *
 *****************************************************************************
 */

void pre_add_arrayelements()
{
	HASHU *sp;
        register int i;
        int last;

	last = lastfilledsym;
	for (i=0; i<=last; i++)
	  {
          sp = symtab[i];
	  if (sp->a6n.array && (!sp->a6n.arrayelem) &&
              (!sp->a6n.isstruct) && (!sp->a6n.farg) && sp->a6n.seenininput)
	    insert_array_element(i);
	  }
}

/*****************************************************************************
 *
 *  INSERT_DUMMY_BLOCK();
 *
 *  Description:    insert_dummy_block creates an empty block at the 
 *                  beginning of the routine. This allows for a place 
 *                  to put initial loads if the 'real' first block 
 *                  happens to be an 'undetected' loop.
 *
 *  Called by:		funcinit()
 *			main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: pseudolbl
 *                      need_ep  
 *
 *  Globals Modified:   breakop  
 *                      need_block  
 *                      need_ep  
 *                      ep  
 *
 *  External Calls:	addexpression()
 *
 *****************************************************************************
 */
 
void insert_dummy_block()
{
	if (need_ep)
	  {
	  ep[lastep].b.val = pseudolbl;
	  need_ep = NO;
	  }

	need_block = YES;
	addexpression(NULL,0);

	need_block = YES; /* addexpression just set this to NO */
        breakop = FREE;
}

/*****************************************************************************
 *
 *  FUNCINIT()
 *
 *  Description:	funcinit() is called at the beginning of processing
 *			each fuctional unit to do mundane initialization.
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	maxbblsz
 *			baseoff
 *			master_global_disable
 *			count_file
 *
 *  Globals Modified:	maxlblsz
 *			labelpool
 *			blockpool
 *			need_block
 *			global_disable
 *			maxoff
 *			topblock
 *			lastblock
 *			toplbl
 *			lastop
 *			defsets_on
 *			top_preheader
 *			lastdef
 *			lastep
 *			lastn
 *			lastcom
 *			lastfarg
 *			lastptr
 *			need_ep
 *			ncalls
 *			nexprs
 *			hiddenfargchain
 *			assigned_goto_seen
 *			exitblockno
 *			ficonlabmax
 *			ficonlabsz
 *			asgtp
 *			last_vfe_thunk
 *			curr_vfe_thunk
 *			non_empty_vfe_seen
 *			vfe_anon_refs
 *			dfo
 *			array_forms_fixed
 *			farg_slots
 *			saw_dragon_access
 *			saw_global_access
 *
 *  External Calls:	ckalloc()
 *			clralloc()
 *			memset()
 *			proc_init_storage()
 *			proctinit()
 *                      insert_dummy_block()
 *
 *****************************************************************************
 */

void funcinit()
{
	maxlblsz = LBLSZ;
	labelpool = (LBLOCK *) clralloc (maxlblsz * sizeof(LBLOCK) );

	memset(blockpool, 0, maxbblsz * sizeof(BBLOCK));

	maxoff = baseoff;
	need_block = YES;
	topblock = (BBLOCK *) NULL;
	lastblock = (BBLOCK *) NULL;
	toplbl = (LBLOCK *) NULL; 
	lastop = FREE;		/* necessary ? */
	breakop = FREE;
	defsets_on = NO;
	top_preheader = NULL;
	lastdef = 0;	/* deftab[0] is not usable */
	lastep = 0;	/* ep[0] will have function header LBLOCK */
	lastn = -1;
	lastcom = -1;
	lastfarg = -1;
	lastptr = -1;
	need_ep = NO;
	ncalls = 0;
	nexprs = 0;
	hiddenfargchain = -1;
	assigned_goto_seen = NO;
	exitblockno = -1;
	ficonlabmax = FICONSZ;	/* table starts small (again) */
	ficonlabsz = 0;
	asgtp = (CGU *) ckalloc(ficonlabmax * sizeof(CGU) );

	/* no variable expression FORMATS seen yet */
	last_vfe_thunk = -1;
	curr_vfe_thunk = -1;
	non_empty_vfe_seen = NO;
	vfe_anon_refs = (VFEREF *) NULL;

	/* initialize register allocation storage structures */
	proc_init_storage();	/* regweb.c */

	/* re-initialize tree nodes and tasciz tables */
	proctinit();		/* misc.c */

	global_disable = master_global_disable;
	dfo[0] = NULL;		/* to ensure that mkblock() will know when
				   the dfo[] is active.
				*/
	array_forms_fixed = NO;
	memset(farg_slots, 0, max_farg_slots * sizeof(long));

        insert_dummy_block();
	saw_dragon_access = YES; /* assume YES until we check later for sure */
        saw_global_access = YES;
# ifdef COMPLEXITY
	if (count_file)
		complexitize(INITIALIZATION);
# endif COMPLEXITY
}  /* funcinit */

/*****************************************************************************
 *
 *  MKBLOCK()
 *
 *  Description:	Make a new basic block.
 *
 *  Called by:		add_block_between_blocks()
 *			addexpression()
 *			mknewpreheader()
 *			fix_overlapping_regions()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	return value -- pointer to new block structure
 *
 *  Globals Referenced:	blockpool
 *			dfo[]
 *			endblock
 *			ep[]
 *			lastblock
 *			lastep
 *			lastlbl
 *			maxbblsz
 *			topblock
 *			toplbl
 *
 *  Globals Modified:	blockpool
 *			dfo[]
 *			endblock
 *			lastblock
 *			maxbblsz
 *			topblock
 *
 *  External Calls:	ckalloc()
 *			ckrealloc()
 *			free()		(FREEIT)
 *			memset()
 *
 *****************************************************************************
 */

BBLOCK *mkblock()
{
	register BBLOCK *bp;

	if ( !topblock )
		{
		topblock = bp = blockpool;
		endblock = &blockpool[maxbblsz];
		}
	else
		{
		bp = lastblock+1;
		if (bp >= endblock)
			{
			BBLOCK *bpold, *bpnew;
			LBLOCK *lbp;
			int j;

			maxbblsz += BBLSZ;
			blockpool = (BBLOCK *)ckalloc( maxbblsz * sizeof(BBLOCK) );

			adjust_vfe_block_pointers((int)blockpool - (int)topblock);

			/* copy old table into new one */
			for (bpold = topblock, bpnew = blockpool;
				bpold < endblock; bpold++, bpnew++)
				{
				*bpnew = *bpold;
				if (bpold->b.l.bp == bpold)
					bpnew->b.l.bp = bpnew;
				}
			/* zero out unused blocks */
			memset(bpnew, 0, (maxbblsz - (endblock - topblock))
					 * sizeof(BBLOCK) );

			/* fix up labelpool, if it still exists */
			if (toplbl != NULL)
				{
				for (lbp = toplbl; lbp <= lastlbl; lbp++)
					lbp->bp = &blockpool[lbp->bp - topblock];
				}
			else	/* label fields already converted to BBLOCK * */
				{
				register BBLOCK *bbp;
				register i;
				CGU *cgp;

				for (i = lastep; i >= 0; i--)
					ep[i].b.bp =
					      &blockpool[ep[i].b.bp - topblock];

				for (bbp = &blockpool[endblock - topblock - 1];
						bbp >= blockpool; bbp--)
				    {
				    switch (bbp->b.breakop)
					{
					case CBRANCH:
					    bbp->bb.rbp =
					      &blockpool[bbp->bb.rbp - topblock];
							/* fall thru */

					case FREE:
					case LGOTO:
					    bbp->bb.lbp =
					      &blockpool[bbp->bb.lbp - topblock];
					    break;

					case EXITBRANCH:
					    break;

					case FCOMPGOTO:
					case GOTO:
					    cgp = bbp->cg.ll;
					    for (i = bbp->cg.nlabs-1; i >= 0;
									i--)
						cgp[i].lp =
						    &(blockpool[cgp[i].lp->bp - topblock].b.l);
					    break;
					}
				    }
				}

			/* fix up the dfo if it exists yet */
			dfo = (BBLOCK **) ckrealloc(dfo,
					maxbblsz * sizeof(BBLOCK *) );
			if (dfo[0])
				{
				for (j = numblocks - 1; j >= 0; j--)
					dfo[j] = &blockpool[dfo[j] - topblock];
				}

			bp = &blockpool[endblock - topblock];
			FREEIT(topblock);
			topblock = blockpool;
			endblock = &blockpool[maxbblsz];
			lastblock = bp - 1;
			}
		}
	bp->l.treep = (NODE *) NULL;
	bp->l.llabel = 0;
	bp->l.rlabel = 0;
	bp->l.gen = (SET *) NULL;
	bp->l.kill = (SET *) NULL;
	return(bp);
}  /* mkblock */

/*****************************************************************************
 *
 *  ADDEXPRESSION()
 *
 *  Description:	Add an expression to the current basic block. 
 *
 *  Called by:		main()
 *
 *  Input Parameters:	p -- expression to be added
 *			lineno -- source lineno
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	breakop
 *			labvals
 *			lastblock
 *			lastnode
 *			llp
 *			lstackp
 *			need_block
 *			nlabs
 *			postponed_np
 *			pseudolbl
 *
 *  Globals Modified:	need_block
 *			postponed_np
 *			pseudolbl
 *
 *  External Calls:	block()
 *			enterlblock()
 *			mkblock()
 *			poplbl()
 *			pushlbl()
 *
 *****************************************************************************
 */

void addexpression(p, lineno)
register NODE *p;
long lineno;
{
	register BBLOCK *bp;
	LBLOCK *lp;

# ifdef DEBUGGING
	if (bdebug > 1)
		{
		fprintf(debugp, "addexpression(0x%x, %d) called.\n", p, lineno);
		fprintf(debugp, "need_block = %d, postponed_nb = %d\n", need_block,
			postponed_nb);
		}
# endif	DEBUGGING

	if (need_block)
		{
		bp = mkblock();
		bp->l.treep = p? block(UNARY SEMICOLONOP, p, NULL, INT) : NULL;
		if (p /* or bp->l.treep */)
			bp->l.treep->nn.source_lineno = lineno;
		if ( lstackp < labvals )	/* no awaiting labels */
			{
			/* Unlabeled blocks are possible, particularly after
			   a CBRANCH.
			*/
			pushlbl(pseudolbl);
			enterlblock(pseudolbl);
			}

		/* at this point we're guaranteed to have at least 1 label
		   in the stack.
		*/

		if (lastblock) lastblock->l.breakop = breakop;

		switch(breakop)	/* the reason for the new block */
		{
		case FCOMPGOTO:
			lastblock->cg.nlabs = llp->nlabs;
			lastblock->cg.ll = llp;

			if (fortran)   /* not a SWITCH */
				{
				/* Finish the cgu table for the default label */
				llp[llp->nlabs-1].val = *lstackp;
				lp = enterlblock(*lstackp);
				lp->preferred = YES;
				}
			llp = llp->next;
			break;
		case LGOTO:	/* Everyday GOTO */
			/* unary operator ... but is the tree top unary? */
			lastblock->l.llabel = (lastnode->in.op == SEMICOLONOP)?
				lastnode->in.right->bn.label :
				lastnode->in.left->bn.label;
			break;

		case CBRANCH:
			/* binary. llp points to "fall thru" lblock;
			   rlp points to "jump" lblock.
			*/
			lastblock->l.llabel = *lstackp;
			lastblock->l.rlabel = (lastnode->in.op == SEMICOLONOP)?
				lastnode->in.right->in.right->bn.label :
				lastnode->in.left->in.right->bn.label;
			break;

		case FREE:
			/* First call to addexpression() uses neither
			   GOTO nor CBRANCH.
			*/
			if (lastblock) lastblock->l.llabel = *lstackp;
			break;

		case GOTO:	/* ASSIGNED GOTO */
			/* links will be completed after FRBRAC */
			break;

		default:
#pragma BBA_IGNORE
			cerror("impossible reason for new block in addexpression()");
		}

		pseudolbl++;
		while ( lstackp >= labvals )	/* nonempty stack */
			{
			/* fill out uncompleted LBLOCKS in the pool */
			lp = enterlblock( poplbl() );	/* as a search routine */
			lp->bp = bp;
			}

		lastblock = bp;
		lastnode = bp->l.treep;
		}
	else
		{
		bp = lastblock;
		if (p)
			{
			bp->l.treep = lastnode =
				block(SEMICOLONOP, lastnode,p,INT);
			lastnode->nn.source_lineno = lineno;
			}
		}

	/* Reset need_block. By setting it to postponed_nb we ensure that
	   a new block is started for the NEXT expression, not the one that
	   caused the break (e.g. a CBRANCH node).
	*/
	need_block = postponed_nb;
	postponed_nb = NO;

# ifdef DEBUGGING
	if (bdebug > 1)
		{
		fprintf(debugp, "\nat end of addexpression()\n");
		if (p)
			fwalk( bp->l.treep, eprint, 0 );
		else
			fprintf(debugp, "\t bp->l.treep == NULL\n");
		dumpblockpool(0, 0);
		if (ldebug) dumplblpool( 0 );
		}
# endif DEBUGGING
}  /* addexpression */

/******************************************************************************
 *
 *	ENTERLBLOCK()
 *
 *	Description:		enterlblock() adds a label (int) to the label
 *				table (labelpool) if it's not already there. 
 *
 *	Called by:		main()
 *				addexpression()
 *				update_links()
 *
 *	Input parameters:	ll - the numeric value of the label.
 *
 *	Output parameters:	none
 *				(returns the address of the new LBLOCK)
 *
 *	Globals referenced:	ldebug
 *				debugp
 *				toplbl
 *				lastlbl
 *				labelpool
 *				endlblp
 *				maxlblsz
 *
 *	Globals modified:	toplbl
 *				lastlbl
 *				endlblp
 *				maxlblsz
 *				labelpool
 *
 *	External calls:		fprintf()
 *				ckrealloc()
 *
 ******************************************************************************
 */

LBLOCK *enterlblock(ll)
register long ll;
{
	register LBLOCK *lp;
	register LBLOCK *lstlbl;
	long difflp;

# ifdef DEBUGGING
	if (ldebug>1)
		fprintf(debugp, "enterlblock(%d) called.\n", ll);
# endif DEBUGGING

	if ( !toplbl )
		{
		toplbl = labelpool;
		lastlbl = toplbl;
		endlblp = &labelpool[maxlblsz];

		}
	else
		{
		lstlbl = lastlbl;
		for (lp = toplbl; lp <= lstlbl; lp++)
			if (lp->val == ll) return (lp);
		++lastlbl;
		if (lastlbl >= endlblp)
			{
			difflp = lastlbl - toplbl;
			maxlblsz += LBLSZ;
			labelpool = (LBLOCK *) ckrealloc (labelpool, 
						maxlblsz * sizeof(LBLOCK) );

			adjust_vfe_label_pointers((int)labelpool - (int)toplbl);

			toplbl = labelpool; 
			endlblp = &labelpool[maxlblsz];
			lastlbl = toplbl + difflp;

			/* Since clrealloc calls realloc to get more space,
			   the new space is not necessarily zeroed. Zero it.
			*/
			memset(lastlbl, 0, ((char*)endlblp) - ((char*)lastlbl));
			}
		}

	lastlbl->val = ll;
	return (lastlbl);
}  /* enterlblock */

/*****************************************************************************
 *
 *  PUSHLBL()
 *
 *  Description:	Pushes an unentered label onto the stack to save for
 *			when the next block is made. At that time the labels
 *			will be associated with the new block.
 *
 *  Called by:		addexpression()
 *			main()
 *
 *  Input Parameters:	lval -- label value to be pushed.
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	labvals[]
 *			lstackp
 *			maxlabstksz
 *
 *  Globals Modified:	labvals[]
 *			lstackp
 *			maxlabstksz
 *
 *  External Calls:	ckrealloc()
 *
 *****************************************************************************
 */

void pushlbl(lval)
{

	/* lstackp points to top active member of the labvals stack */
	if (++lstackp >= &labvals[maxlabstksz])
		{
		int diff = lstackp - labvals;
		maxlabstksz += MAXLABVALS;
		labvals = (unsigned *)ckrealloc(labvals, 
					maxlabstksz * sizeof(unsigned));
		lstackp = labvals + diff;
		}
	*lstackp = lval;
}  /* pushlbl */

/*****************************************************************************
 *
 *  POPLBL()
 *
 *  Description:	Return the topmost label from the stack and adjust
 *			lstackp.
 *
 *  Called by:		addexpression()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	return value -- topmost label in label stack
 *
 *  Globals Referenced:	labvals
 *			lstackp
 *
 *  Globals Modified:	lstackp
 *
 *  External Calls:	cerror()
 *
 *****************************************************************************
 */

LOCAL unsigned poplbl()
{
	if ( lstackp < labvals )
#pragma BBA_IGNORE
		cerror("overpopped the label stack!");
	return ( *(lstackp--) );
}  /* poplbl */

/*****************************************************************************
 *
 *  COALESCE()
 *
 *  Description:
 *
 *  Called by:		clean_flowgraph()
 *			coalesce()
 *
 *  Input Parameters:
 *
 *  Output Parameters:
 *
 *  Globals Referenced:
 *
 *  Globals Modified:
 *
 *  External Calls:	coalesce()
 *
 *****************************************************************************
 */

LOCAL void coalesce(lp)		register LBLOCK *lp;
{
	register int i;
	CGU *cgup;

	if (lp && !lp->visited)
		{
		lp->visited = YES;
		if (lp->bp)
			{
			i = lp->bp - topblock;
			if ( !ltable[i] )
				ltable[i] = lp;
			switch (lp->bp->b.breakop)
				{
				case FREE:
				case LGOTO:	/* everyday GOTO */
					coalesce(lp->bp->b.llp);
					break;
				case CBRANCH:
					coalesce(lp->bp->b.llp);
					coalesce(lp->bp->b.rlp);
					break;
				case FCOMPGOTO:
				case GOTO:	/* ASSIGNED GOTO */
					cgup = lp->bp->cg.ll;
					for (i = lp->bp->cg.nlabs-1; i>=0; i--)
						coalesce(cgup[i].lp);
					break;
				case EXITBRANCH:
					break;
				}
			}
		}
}  /* coalesce */

/*****************************************************************************
 *
 *  FIXUP_LABELS()
 *
 *  Description:
 *
 *  Called by:		final_collapse()
 *
 *  Input Parameters:
 *
 *  Output Parameters:
 *
 *  Globals Referenced:
 *
 *  Globals Modified:
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void fixup_labels(desired, now)	LBLOCK *desired, *now;
{
	register LBLOCK *lp;
	register BBLOCK *bpx;
	NODE *np;
	CGU *cgup;
	int i;

	for (lp = toplbl; lp <= lastlbl; lp++)
		{
		bpx = lp->bp;
		if (!bpx)
			continue;
		switch(bpx->b.breakop)
			{
			case CBRANCH:
				if (bpx->b.rlp == now)
					{
					bpx->b.rlp = desired;
					np = (bpx->b.treep->in.op==SEMICOLONOP)?
						bpx->b.treep->in.right :
						bpx->b.treep->in.left;
					np = np->in.right;
					/* np points to the ICON node */
					np->tn.lval = desired->val;
					}
				if (bpx->b.llp == now)
					bpx->b.llp = desired;
				break;
			case FREE:
				if (bpx->b.llp == now)
					bpx->b.llp = desired;
				break;
			case LGOTO:
				if (bpx->b.llp == now)
					{
					bpx->b.llp = desired;
					np = (bpx->b.treep->in.op==SEMICOLONOP)?
						bpx->b.treep->in.right :
						bpx->b.treep->in.left;
					/* np points to the LGOTO node */
					np->bn.label = desired->val;
					}
				break;
			case FCOMPGOTO:
			case GOTO:
				cgup = bpx->cg.ll;
				for (i = bpx->cg.nlabs-1; i >= 0; i--)
					if (cgup[i].lp == now)
						cgup[i].lp = desired;
				break;
			case EXITBRANCH:
				break;
			}
		}

	/* Fix the entry point table, which points to LBLOCK entries, too. */
	for (i = lastep; i >= 0; i--)
		if (ep[i].l.lp == now)
			{
			ep[i].l.lp = desired;
			ep[i].l.val = desired->val;
			break;
			}
}  /* fixup_labels */

/*****************************************************************************
 *
 *  FIRMUP_LABELS()
 *
 *  Description:
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastlbl
 *			ltable[]
 *			topblock
 *			toplbl
 *
 *  Globals Modified:	none
 *
 *  External Calls:	fixup_labels()
 *
 *****************************************************************************
 */
LOCAL void firmup_labels()
{
	register LBLOCK *lp;
	register LBLOCK *lte;

	for (lp = toplbl; lp <= lastlbl; lp++)
		{
		/* lp entries without associated bp entries are possible
		   for FORMAT statements.
		*/
		if (!lp->bp)
			continue;
		lte = ltable[lp->bp - topblock];
		if (lte != lp)
			{
			if (lp->visited)
				{
				lte->visited = YES;
				lp->visited = NO;
				}
			fixup_labels(lte, lp);
			}
		}
}  /* firmup_labels */

/*****************************************************************************
 *
 *  RM_DEADCODE()
 *
 *  Description:
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastblock
 *			ltable
 *			topblock
 *
 *  Globals Modified:	none
 *
 *  External Calls:	tfree()
 *
 *****************************************************************************
 */
LOCAL void rm_deadcode()
{
	register  BBLOCK *bp;
	register  LBLOCK **lpp;

	for (bp = topblock, lpp = ltable; bp <= lastblock; bp++, lpp++)
		{
		if (*lpp == (LBLOCK *)NULL)
			{
			if (bp->b.treep)
				goto deadcode;
			}
		else
			if ( ! (*lpp)->visited )
				/* It could be in the ltable but not visited
				   if the label was added as a result of an
				   ASSIGN statement.
				*/
				goto deadcode;
		continue;

deadcode:

# ifdef DEBUGGING
		if (bdebug)
			fprintf(debugp, "BBLOCK[%d] dead. Freeing tree.\n",
				bp-topblock);
# endif DEBUGGING
		tfree(bp->b.treep);
		bp->b.treep = NULL;
		bp->l.breakop = EXITBRANCH;
		}
}  /* rm_deadcode */

/*****************************************************************************
 *
 *  LCOMPAR()
 *
 *  Description:	Return neg if llp->bp < rlp->bp;
 *			returns 0 if they are the same;
 *			return pos if llp->bp > rlp->bp.
 *			Used by qsort.
 *
 *  Called by:		qsort(3)
 *
 *  Input Parameters:	llp -- first label block pointer
 *			rlp -- second label block pointer
 *
 *  Output Parameters:	value indicating relationship between input parms.
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL int lcompar(llp, rlp)	LBLOCK *llp, *rlp;
{
	return(llp->bp - rlp->bp);
}  /* lcompar */

/*****************************************************************************
 *
 *  FINAL_COLLAPSE()
 *
 *  Description:
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastlbl
 *			topblock
 *			toplbl
 *
 *  Globals Modified:	lastblock
 *
 *  External Calls:	fixup_labels()
 *
 *****************************************************************************
 */

LOCAL void final_collapse()
{
	register  BBLOCK *bp;
	register  LBLOCK *lp, *lx;

	for (lx = lastlbl, lp = toplbl; lp <= lastlbl; lp++)
		{
		register LBLOCK *lp2;

		if (lp->visited)
			lx = lp;
		else
			{
			lp2 = lp;
			while (++lp2 <= lastlbl)
				{
				if (lp2->visited)
					{
					lx = lp;
					*lp = *lp2;
					fixup_labels(lp, lp2);
					lp2->visited = NO;
					break;
					}
				}
			}
		}
	lastlbl = lx;

	for (lp = toplbl, bp = topblock; lp <= lastlbl; bp++, lp++)
		{
		if (lp->bp != bp)
			{
			*bp = *lp->bp;
			lp->bp = bp;
			}
		}
	lastblock = bp-1;
}  /* final_collapse */

/*****************************************************************************
 *
 *  UPDATE_LINKS()
 *
 *  Description:	Finish linking lblpool and blockpool.
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastblock
 *			topblock
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *			enterlblock()
 *
 *****************************************************************************
 */

LOCAL void update_links()
{
	register BBLOCK *bp;
	CGU *llp;
	int i;

	for (bp = topblock; bp < lastblock; bp++)	/* last one is vacuous */
		{
		switch (bp->l.breakop)
			{
			case LGOTO:
			case FREE:
				bp->b.llp = enterlblock(bp->l.llabel);
				break;

			case CBRANCH:
				bp->b.llp = enterlblock(bp->l.llabel);
				bp->b.rlp = enterlblock(bp->l.rlabel);
				break;

			case FCOMPGOTO:
			case GOTO:
				llp = bp->cg.ll;
				for (i = bp->cg.nlabs-1; i >= 0; i--)
					llp[i].lp = enterlblock(llp[i].val);
				break;

			case EXITBRANCH:
				break;

			default:
#pragma BBA_IGNORE
				cerror("unknown breakop in update_links()");
				break;
			}
		}
}  /* update_links */

/*****************************************************************************
 *
 *  ADD_PREDECESSOR()
 *
 *  Description:	Attach a LINK node to an BBLOCK lp. The link node
 *			identifies a predecessor BBLOCK to bp.
 *
 *  Called by:		compute_predecessors()
 *
 *  Input Parameters:	predecessor -- predecessor block
 *			bp -- successor block
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_plink()
 *
 *****************************************************************************
 */

LOCAL void add_predecessor(predecessor, bp)	BBLOCK *predecessor, *bp;
{
	register LINK *xlp, *linkp;
	LINK *ylp;

	for (ylp = (LINK *) NULL, xlp = bp->b.l.pred; xlp; xlp = xlp->next)
		{
		if (xlp->bp == predecessor) /* it's already there */
			return;
		ylp = xlp;
		}

	linkp = (LINK *) alloc_plink();
	if (ylp) ylp->next = linkp;
	else bp->bb.l.pred = linkp;
	linkp->next = (LINK *) NULL;
	linkp->bp = predecessor;
}  /* add_predecessor */

/*****************************************************************************
 *
 *  COMPUTE_PREDECESSORS()
 *
 *  Description:	Compute predecessor sets (actually implemented as
 *			linked lists).
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastblock
 *			topblock
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_predecessor()
 *
 *****************************************************************************
 */
LOCAL void compute_predecessors()
{
	register BBLOCK *bp;
	int i;

	for (bp = topblock; bp <= lastblock; bp++)
		{
		switch (bp->bb.breakop)
			{
			case CBRANCH:	/* binary */
				add_predecessor(bp, bp->bb.rbp->bb.l.bp);
				/* fall thru */
			case FREE:
			case LGOTO:	/* unary */
				add_predecessor(bp, bp->bb.lbp->bb.l.bp);
				break;

			case FCOMPGOTO:
			case GOTO:
				for (i = bp->cg.nlabs-1; i >= 0; i--)
					add_predecessor(bp, bp->cg.ll[i].lp->bp);
				break;

			case EXITBRANCH:	/* leaf */
				break;
			}
		}
}  /* compute_predecessors */

/*****************************************************************************
 *
 *  ENSURE_PREFERRED_LABELS()
 *
 *  Description:	Go thru the lblock table and insert preferred
 *			labels into ltable[] first before coalesce() is called.
 *			By this means the preferred labels will be used if at
 *			all possible. It also marks a preferred label "visited"
 *			if it is the first preferred label seen for a
 *			particular bblock.
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastlbl
 *			ltable[]
 *			topblock
 *			toplbl
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_plink()
 *
 *****************************************************************************
 */

LOCAL void ensure_preferred_labels()
{
	register LBLOCK *lp;
	PLINK *plinkp;
	int i;

	for (lp = toplbl; lp <= lastlbl; lp++)
		if (lp->preferred && lp->bp)
			/* Preferred labels have an associated block by now
			   if they reference executable code. If they reference
			   a FORMAT statement they will not.
			*/
			{
			i = lp->bp - topblock;
			if (ltable[i])
				{
				/* Another preferred label has already been
				   inserted here.
				*/
				if (plinkp = ltable[i]->pref_llistp)
					{
					while (plinkp->next)
						plinkp = plinkp->next;
					plinkp->next = alloc_plink();
					plinkp = plinkp->next;
					plinkp->next = (PLINK *) NULL;
					}
				else
					{
					plinkp = alloc_plink();
					plinkp->next = (PLINK *) NULL;
					ltable[i]->pref_llistp = plinkp;
					}
				plinkp->val = lp->val;
				}
			else
				{
				ltable[i] = lp;
				}
			}
}  /* ensure_preferred_labels */

/*****************************************************************************
 *
 *  COPY_LBLOCKS()
 *
 *  Description:	copy_lblocks() is called to copy the contents of the
 *			labelpool into the corresponding BBLOCK unions so that
 *			from this point onward only one structure is required
 *			for both.
 *
 *			It also frees the labelpool and issues a warning for
 *			possible infinite loops.
 *
 *  Called by:		clean_flowgraph()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	ep[]
 *			labelpool
 *			lastblock
 *			lastep
 *			lastlbl
 *			topblock
 *
 *  Globals Modified:	labelpool (free'd)
 *			toplbl
 *
 *  External Calls:	FREEIT()
 *			werror()
 *
 *****************************************************************************
 */

LOCAL void copy_lblocks()
{
	register LBLOCK *lp;
	register BBLOCK *bp;
	register i;
	CGU *cgp;

	for (i = lastep; i >= 0; i--)
		ep[i].b.bp = ep[i].l.lp->bp;

	for (lp = lastlbl; lp >= labelpool; lp--)
		{
		bp = lp->bp;
		bp->b.l = *lp;
		}

	for (bp = lastblock; bp >= topblock; bp--)
		{
		switch (bp->b.breakop)
			{
			case CBRANCH:
				bp->bb.rbp = bp->b.rlp->bp;	
				/* fall thru */

			case FREE:
			case LGOTO:
				bp->bb.lbp = bp->b.llp->bp;	
				if (bp == bp->bb.lbp)
					werror("Possible infinite loop detected.");
				break;

			case EXITBRANCH:
				break;

			case FCOMPGOTO:
			case GOTO:
				cgp = bp->cg.ll;
				for (i = bp->cg.nlabs-1; i >= 0; i--)
					cgp[i].lp = &(cgp[i].lp->bp->b.l);
				break;
			}
		}

	FREEIT(labelpool);
	toplbl = NULL;
}  /* copy_lblocks */

/*****************************************************************************
 *
 *  CLEAN_FLOWGRAPH()
 *
 *  Description:
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	ep[]
 *			labelpool
 *			lastblock
 *			lastep
 *			lastlbl
 *			topblock
 *			toplbl
 *
 *  Globals Modified:	ep[]
 *			ltable
 *
 *  External Calls:	FREEIT()
 *			alloc_plink()
 *			cerror()
 *			clralloc()
 *			coalesce()
 *			compute_predecessors()
 *			copy_lblocks()
 *			ensure_preferred_labels()
 *			final_collapse()
 *			firmup_labels()
 *			rm_deadcode()
 *			qsort(3)
 *			update_links()
 *
 *****************************************************************************
 */

void clean_flowgraph()
{
	register LBLOCK *lp;
	register int i;

	/* sort the lblock table. Use the system routine qsort(3C). */
	qsort( labelpool, lastlbl - labelpool + 1, sizeof(LBLOCK), lcompar);

# ifdef	DEBUGGING
	if (ldebug > 1)
		{
		fprintf(debugp, "\nbefore updating the lblock table:\n");
		dumplblpool( 0 );
		}
	if (bdebug > 1)
		{
		fprintf(debugp, "\nbefore updating the block table:\n");
		dumpblockpool(0, 0);
		}
# endif DEBUGGING

	update_links();

	/* now coalesce the labels, getting rid of multiples on the same block */

# ifdef DEBUGGING
	if (bdebug > 2)
		{
		fprintf(debugp, "\nbefore coalescing the block table:\n");
		dumpblockpool(0, 1);
		}
	if (ldebug > 2)
		{
		fprintf(debugp, "\nbefore coalescing the lblock table:\n");
		dumplblpool( 0 );
		}
# endif DEBUGGING
	ltable = (LBLOCK **)clralloc((lastblock-topblock+1)*sizeof(LBLOCK *));

	/* It is incorrect to assume that the top label corresponds to the top
	   basic block. Find the the label that does to start off coalescing.
	*/
	for (lp = toplbl; lp; lp++)
		if (lp->bp == topblock) break;
	if (!lp)
#pragma BBA_IGNORE
		cerror("Strange LBLOCk-BBLOCK structure in clean_flowgraph()");
	ensure_preferred_labels();
	if (ltable[0])
		lp = ltable[0];

	/* Enter top into entry point table */
	ep[0].l.lp = lp;
	ep[0].l.val = lp->val;
	/* Once the labelpool has been shuffled, finish linking the ep (except
	   for ep[0], which will be done below).
	*/
	for (i = 1; i <= lastep; i++)
		for (lp = labelpool; lp <= lastlbl; lp++)
			if (ep[i].l.val == lp->val)
				{
				ep[i].l.lp = lp;
				break;
				}

	for (i = 0; i <= lastep; i++)
		coalesce(ep[i].l.lp);
	firmup_labels();
	rm_deadcode();
# ifdef DEBUGGING
	if (bdebug > 2)
		{
		fprintf(debugp, "\nbefore final_collapse (the block table):\n");
		dumpblockpool(0, 1);
		}
	if (ldebug > 2)
		{
		fprintf(debugp, "\nbefore final_collapse (the lblock table):\n");
		dumplblpool( 0 );
		}
# endif DEBUGGING
	final_collapse();
	FREEIT(ltable);

# ifdef DEBUGGING
	if ( (lastblock-topblock) != (lastlbl-toplbl) )
		{
		dumplblpool(0);
		dumpblockpool(1,0);
		cerror("one-to-one correspondence failed in clean_flowgraph()");
		}
# endif DEBUGGING

	copy_lblocks();

	/* From this point onward, only bp->bb and bp->cg BBLOCKS should be
	   accessed because pointers have now been moved to point to BBLOCKS.
	*/

	compute_predecessors();

	for (i = 0; i <= lastep; ++i)
		{
		register PLINK *pp;

		ep[i].b.nodep = ep[i].b.bp->b.treep;	/* (U)SEMICOLONOP */
		if (ep[i].b.nodep)	/* to handle the null program */
			ep[i].b.nodep->in.entry = (i > 0);
		/* Top entry is not marked since no code is output other than
		   FTEXT code.
		*/

		pp = alloc_plink();
		pp->val = i;
		pp->next = ep[i].b.bp->bb.entries;
		ep[i].b.bp->bb.entries = pp;
		}

# ifdef DEBUGGING
	if (bdebug)
		{
		fprintf(debugp, "\nafter coalescing the block table:\n");
		dumpblockpool(1, 1);
		}
# endif DEBUGGING
}  /* clean_flowgraph() */

/*****************************************************************************
 *
 *  BFREE()
 *
 *  Description:	Free the flow graph.  The trees have already been
 *				freed.
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastblock
 *			topblock
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREESET()
 *			free_plink()
 *
 *****************************************************************************
 */

void bfree()
{	
	register BBLOCK *bp = topblock;

	while (bp <= lastblock)
		{
		if (bp->b.l.pred)
			{
			register LINK *linkp;

			linkp = bp->b.l.pred;
			while (linkp)
				{
				LINK *linkpnext = linkp->next;
				free_plink(linkp);
				linkp = linkpnext;
				}
			}
		if (bp->b.l.pref_llistp)
			{
			register PLINK *pl;

			pl = bp->b.l.pref_llistp;
			while (pl)
				{
				PLINK *plnext = pl->next;
				free_plink(pl);
				pl = plnext;
				}
			}
		if (bp->b.gen)
			FREESET(bp->b.gen);
		if (bp->b.kill)
			FREESET(bp->b.kill);
		bp++;
		}
}  /* bfree */

/*****************************************************************************
 *
 *  COPY_IN_TO_OUT()
 *
 *  Description:	Copy contents of input file to output file
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lrd
 *			outfile
 *
 *  Globals Modified:	none
 *
 *  External Calls:	feof()
 *			fread()
 *			fwrite()
 *
 *****************************************************************************
 */

void copy_in_to_out()
{
    char buff[1024];
    long nchars;

    while (!feof(lrd))
	{
	nchars = fread(buff, 1, 1024, lrd);
	fwrite(buff, 1, nchars, outfile);
	}
}  /* copy_in_to_out */

/*****************************************************************************
 *
 *  PASS_PROCEDURE()
 *
 *  Description:	Copies through from input to output enough intermediate
 *			code for 1 procedure.
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	asgtp
 *			assumptions
 *			fortran
 *			labelpool
 *			lcrbuf
 *			olevel
 *
 *  Globals Modified:	asgtp
 *			assumptions
 *			labelpool
 *			olevel
 *
 *  External Calls:	addexternasciz()
 *			addtreeasciz()
 *			basic_node()
 *			cerror()
 *			fprintf()
 *			free()	{FREEIT}
 *			lccopy()
 *			lread()
 *			lstrread()
 *			p2name()
 *			p2word()
 *			symtab_insert()
 *
 *****************************************************************************
 */

void pass_procedure()
{
    register long x;
    register long i;
    register long j;
    char *name;
    NODE *p;
    SYMTABATTR attr;
    int arraysize;

    for (;;)
	{
	x = lread();

#ifdef DEBUGGING
	if( xdebug>2 )
	    fprintf( debugp, "\nPassing op=%s., val = %d., rest = 0x%x\n",
			xfop(FOP(x)), VAL(x), (int)REST(x) );
#endif	DEBUGGING

	switch( FOP(x) )
	    {
	    case 0:
#ifdef DEBUGGING
		fprintf(debugp, "null opcode ignored\n");
#else
		fprintf(stderr, "null opcode ignored\n");
#endif
		break;

	    case ARRAYREF:	/* c1 only -- throw away */
		lread();
		break;

	    case STRUCTREF:
		if (VAL(x))	/* primary ?? */
		    lread();
		break;

	    case C1OPTIONS:
		/* A C1OPTIONS record follows each FLBRAC regardless of anything
		   new.
		*/
		olevel = VAL(x);
		assumptions = REST(x);
		break;

	    case NOEFFECTS:
#pragma BBA_IGNORE
	        cerror("NOEFFECTS within procedure");
		lstrread();
		break;

	    case FLD:
	    case STARG:
	    case STASG:
	    case OREG:
	    case STCALL:
	    case UNARY STCALL:
		p2word(x);
		if ( TMODS1(REST(x)) )
		    {
		    p2word(lread());
		    if ( TMODS2(REST(x)) )
			p2word(lread());
		    }
		p2word(lread());
		break;

	    case C1HIDDENVARS:
		j = REST(x);
		for (i = 0; i < j; ++i)
		    {
		    x = lread();
		    if (FOP(x) == C1HVOREG)
			{
			lread();
			}
		    else if (FOP(x) == C1HVNAME)
			{
			lread();
			lstrread();
			}
		    else
#pragma BBA_IGNORE
			cerror("bad HIDDENVARS opcode in pass_procedure()");
		    }
		break;

	    case FENTRY:
		p2word(x);
		lstrread();
		p2name(lcrbuf);
		break;

	    case FICONLAB:
		break;

	    case FCOMPGOTO:
	    case SWTCH:
		p2word(x);
		j = REST(x);
		for (i = 0; i < j; i++)
		    {
		    if (! fortran)
			p2word(lread());
		    p2word(lread());
		    }
		break;

	    case NAME:
		p2word(x);
		if ( TMODS1(REST(x)) )
		    {
		    p2word(lread());
		    if ( TMODS2(REST(x)) )
			p2word(lread());
		    }
		if (VAL(x))
		    p2word(lread());
		lstrread();
		p2name(lcrbuf);
		break;

	    case ICON:
		p2word(x);
		if ( TMODS1(REST(x)) )
		    {
		    p2word(lread());
		    if ( TMODS2(REST(x)) )
			p2word(lread());
		    }
		p2word(lread());
		if (VAL(x))
		    {
		    lstrread();
		    p2name(lcrbuf);
		    }
		break;

	    case GOTO:
		p2word(x);
		if (VAL(x))
		    p2word(lread());
		break;

	    case FTEXT:
		lccopy(x);
		break;

	    case FEXPR:
		if (VAL(x))
		    lccopy(x);
		else
		    p2word(x);
		break;

	    case FRBRAC:
		p2word(x);
		if (VAL(x))		/* structure-valued function */
		    {
		    p2word(lread());
		    p2word(lread());
		    }
		FREEIT(asgtp);
		FREEIT(labelpool);
		in_procedure = NO;
		proc_init_symtab();	/* clear local items from symtab */
		return;

	    case SETREGS:
		p2word(x);
		p2word(lread());
		break;

	    case C1SYMTAB:
		if (VAL(x) == 1)
		    {
		    lread();
		    lread();
		    lread();
		    }
		break;

	    case C1OREG:
		if ( TMODS1(REST(x)) )
		    {
		    lread();
		    if ( TMODS2(REST(x)) )
			lread();
		    }
		lread();
		attr.l = lread();
		if (attr.a.array)
		  arraysize = lread();
		else
		  arraysize = 1;
		lstrread();
		break;

	    case C1NAME:
		/* A C1SYMTAB entry describing a NAME ... not 
		   a tree node.
		*/

		p = basic_node(x);
		p->tn.op = NAME;
		p->tn.type.base = REST(x);
		p->tn.rval = 0;
		if (VAL(x))
		    p->tn.lval = lread();
		else
		    p->tn.lval = 0;
		lstrread();
		attr.l = lread();
		if (attr.a.array)
		  arraysize = lread();
		else
		  arraysize = 1;
		p->atn.name = (!attr.a.isexternal) ?
			addtreeasciz(lcrbuf) : addexternasciz(lcrbuf);
		lstrread();

		if (lcrbuf[0])	/* name != null */
		    name = (!attr.a.isexternal) ?
			addtreeasciz(lcrbuf) : addexternasciz(lcrbuf);
		else
		    name = (char *) 0;

		symtab_insert(p,attr.l,name,YES,arraysize);
		p->in.op = FREE;	/* return to free pool */
		break;

	    default:
		p2word(x);
		if ( TMODS1(REST(x)) )
		    {
		    p2word(lread());
		    if ( TMODS2(REST(x)) )
			p2word(lread());
		    }
		break;

	    case FLBRAC:
#pragma BBA_IGNORE
		cerror("FLBRAC seen in pass_procedure()");
		break;

	    case FEOF:
#pragma BBA_IGNORE
		cerror("FEOF seen in pass_procedure()");
		break;

	    case FMAXLAB:
#pragma BBA_IGNORE
		cerror("FMAXLAB seen in pass_procedure()");
		break;
	    }
	}
}  /* pass_procedure */

/******************************************************************************
*
*	same()
*
*	Description:		returns YES iff the two nodes point to 
*				essentially the "same" datum. Some
*				restrictions may apply wrt flagcheck.
*
*	Called by:		same() recursively
*				check()
*				idiomcheck()
*				constwalk()
*				basic_indvar_shape()
*
*	Input parameters:	np - node into the syntax tree
*				qp - another node into the same syntax tree
*				flagcheck - a boolean flag. If YES then be
*				more restrictive in saying np and qp point
*				to the "same" datum.
*				dagcheck - true if called from dag.c only.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		strncmp()
*
*******************************************************************************/
flag same(np, qp, flagcheck, dagcheck)
	register NODE *np, *qp;
	flag flagcheck, dagcheck;
	/* This routine is recursive. It could be done faster using the
	   value-number method described by A&U, p. 427. Some sort of
	   symbol table structure would be required. Save this for later.
	*/

{
	if ((np->in.op != qp->in.op) || !SAME_TYPE(np->in.type,qp->in.type) )
		return (NO);
	if (np == qp) return (YES);

	switch( optype(np->in.op) )
	{
	case LTYPE :
		if (flagcheck 
			&& ((np->allo.flagfiller|qp->allo.flagfiller) & (EREF|CALLREF|SREF)))
			return(NO);
		return ( !(dagcheck && np->tn.callref) 
			&& (np->tn.lval == qp->tn.lval)
			&& (np->tn.rval ==qp->tn.rval)
			&& !strcmp(np->tn.name, qp->tn.name) );

	case BITYPE :
		if (asgop(np->in.op)) return(NO);
		if ( ! same (np->in.right, qp->in.right, flagcheck, dagcheck) )
			return (NO);
		/* fall thru */

	case UTYPE :
		/* UNARY MULS can have EREF set on the UTYPE, not the LTYPE */
		if (flagcheck 
			&& ((np->allo.flagfiller|qp->allo.flagfiller) & (EREF|CALLREF|SREF)))
			return(NO);
		if (np->in.op == FLD)
			return ( (np->sin.stsize == qp->sin.stsize)
				&& same(np->in.left, qp->in.left, flagcheck, dagcheck) );
		return ( COMMON_OP(np->in.op)
			&& same(np->in.left, qp->in.left, flagcheck, dagcheck));
	}
}

/******************************************************************************
*
*	add_farg_alias()
*
*	Description:		adds an entry to the ftab[] array.
*
*	Called by:		rm_farg_aliases()
*
*	Input parameters:	np - a NODE ptr. Unless it's a ptr to an
*					ASSIGN op the routine returns
*					immediately.
*
*	Output parameters:	none
*
*	Globals referenced:	lastftab
*				ftab
*				ftabend
*				maxftabsz
*
*	Globals modified:	lastftab
*				maxftabsz
*				ftab
*				ftabend
*				_n_farg_prolog_moves_removed
*
*	External calls:		ckrealloc()
*
*******************************************************************************/
LOCAL void add_farg_alias(np)	register NODE *np;
{
	int diffp;

	if (np->in.op != ASSIGN || RO(np) != OREG || RV(np) <= 0)
		return;
	if (++lastftab >= ftabend)
		{
		diffp = lastftab - ftab;
		maxftabsz += FTABINCR;
		ftab = (FBLOCK *)ckrealloc(ftab, maxftabsz * sizeof(FBLOCK));
		ftabend = &ftab[maxftabsz];
		lastftab = ftab + diffp;
		}
	lastftab->assign = np;
	lastftab->local = np->in.left;
#ifdef COUNTING
	_n_farg_prolog_moves_removed++;
#endif COUNTING
}

/******************************************************************************
*
*	substitute_farg_aliases()
*
*	Description:		traverses the non-empty code blocks other than
*				block[1] to replace references to the copies
*				of the formal arguments with the formal
*				arguments themselves. It also updates the 
*				arrayrefno in certain intermediate tree nodes.
*
*	Called by:		rm_farg_aliases()
*
*	Input parameters:	np - NODE ptr to a subtree.
*
*	Output parameters:	none
*
*	Globals referenced:	ftab
*				lastftab
*
*	Globals modified:	the expression tree is modified
*				_n_farg_translations
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void substitute_farg_aliases(np)	register NODE *np;
{
	register FBLOCK *fp;

	switch (np->in.op)
		{
		case FOREG:
		case OREG:
				for (fp = ftab; fp <= lastftab; fp++)
					if (np->tn.lval == fp->local->tn.lval)
						{
						/* Just paint over the offset */
						np->tn.lval = fp->assign->in.right->tn.lval;
						np->tn.arrayrefno = fp->farrayindex;
# ifdef COUNTING
						_n_farg_translations++;
# endif COUNTING
						break;
						}
			break;

		case UNARY MUL:
		case PLUS:
			if (np->allo.flagfiller & (AREF|ABASEADDR))
				for (fp = ftab; fp <= lastftab; fp++)
					if (np->in.arrayrefno==fp->larrayindex)
						{
						np->in.arrayrefno = fp->farrayindex;
						break;
						}
			break;
		}
}


/******************************************************************************
*
*	update_farg_aliases()
*
*	Description:		Updates the ftab and the symtab for those
*				entries involved in useless block[1] copies
*				of formal arguments.
*
*	Called by:		rm_farg_aliases()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	ftab
*				lastftab
*				lastfarg
*				fargtab
*				symtab
*
*	Globals modified:	ftab
*				f_find_array
*				symtab
*
*	External calls:		find()
*				location()
*
*******************************************************************************/
LOCAL void update_farg_aliases()
{
	register HASHU *fsp;
	register HASHU *asp;
	register unsigned *up;
	register HASHU *lsp;
	HASHU *qsp;
	FBLOCK *fp;
	unsigned ano;

	for (fp = ftab; fp <= lastftab; fp++)
		{
		fsp = location(fp->assign->in.right);
		lsp = asp = location(fp->local);
		if ((ano = asp->allo.wholearrayno) != NO_ARRAY)
			{
			asp = symtab[ano];
			if (asp->an.array)
			  f_find_array = YES;
			else
			  f_find_struct = YES;
			ano = find(fp->assign->in.right);	/* inserts it */
			f_find_array = NO;
			f_find_struct = NO;
			qsp = symtab[ano];
			qsp->allo.attributes = asp->allo.attributes;
			qsp->allo.type = asp->allo.type;
			qsp->allo.pass1name = asp->allo.pass1name;
			asp->a6n.seenininput = NO;
			}
		fsp->allo.attributes = lsp->allo.attributes;
		fsp->allo.wholearrayno = ano;
		fsp->allo.pass1name = lsp->allo.pass1name;
		fsp->allo.type = lsp->allo.type;
		lsp->a6n.farg = NO;
		lsp->a6n.seenininput = NO;
		fp->larrayindex = lsp->allo.wholearrayno;
		fp->farrayindex = fsp->allo.wholearrayno;

		/* Update the fargtab to point to the real formal param. */
		for (up = &fargtab[lastfarg]; up >= fargtab; up--)
			if (*up == asp->allo.symtabindex)
				{
				*up = (fsp->allo.wholearrayno == NO_ARRAY)?
					fsp->allo.symtabindex
					: fsp->allo.wholearrayno;
				break;
				}
		}
}


/******************************************************************************
*
*	rm_farg_aliases()
*
*	Description:		For FORTRAN routines with a single entry
*				point, the prolog code in block[1] contains
*				unnecessary formal argument copy code. This
*				routine directs the removal of this copy code
*				and the appropriate replacement of occurrences
*				of the unneeded local copies of the fargs later
*				in the routine.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	ftab
*				maxftabsz
*				lastftab
*				ftabend
*				topblock
*				lastblock
*
*	Globals modified:	ftab
*				maxftabsz
*				lastftab
*				ftabend
*
*	External calls:		ckalloc()
*				add_farg_alias()
*				update_farg_aliases()
*				walkf()
*				substitute_farg_aliases()
*				tfree()
*				free()	{FREEIT}
*
*******************************************************************************/
void rm_farg_aliases()
{
	register NODE *np;
	register FBLOCK *fp;
	BBLOCK *bp;

	/* block[0] is artificial and empty at this point and may be ignored */

	/* Initialization of the data structures */
	ftab = (FBLOCK *)ckalloc(FTABINCR * sizeof(FBLOCK));
	maxftabsz = FTABINCR;
	lastftab = ftab - 1;
	ftabend = &ftab[FTABINCR];

	/* Searching block[1, 2 , or ...] for the prolog assignments */
	bp = topblock + 1;
	while (bp)
		{
		/* Find the pertinent block. VFE processing may insert
		   additional blocks before the one of interest if FORMAT
		   statements precede executable statements.
		*/
		np = bp->b.treep;
		if (!np)
			return;
		if (np->in.op == SEMICOLONOP)
			if (np->in.right->in.op == LGOTO)
				bp++;
			else break;
		else	/* np->in.op == UNARY SEMICOLONOP */
			if (np->in.left->in.op == LGOTO)
				bp++;
			else break;
		}

	/* Look for parameter copies in this block. */
	while (np->in.op == SEMICOLONOP)
		{
		add_farg_alias(np->in.right);
		np = np->in.left;
		}
	if (np->in.op == UNARY SEMICOLONOP)
		add_farg_alias(np->in.left);
	
	/* Now update the symtab to make the fargs look like their aliases */
	update_farg_aliases();

	/* Now go thru remainder of trees doing the substituting */
	while ( ++bp <= lastblock )
		if (np = bp->b.treep)
			walkf(np, substitute_farg_aliases);

	/* Now free the data structures */
	for (fp = ftab; fp <= lastftab; fp++)
		{
		tfree(fp->assign);
		fp->assign->in.op = NOCODEOP;
		}
	FREEIT(ftab);
}

/******************************************************************************
*
*	PARM_EXPER_TO_TEMP() 
* 
*	Description:		np points to the parameter expression. 
*				Create an assignment of that expression
*				to a stack temporary.  Put the resulting
*				assignment in a tree of assignments.
* 
*	Called by:		parm_expers_to_temps() 
* 
*	Input parameters:	np: parameter expression
*				result: assignment tree 
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
void parm_exper_to_temp(np,result) NODE *np, **result; 
{
	NODE *stack_temp;
	HASHU *sp;

	/* allocate the stack temporary */
	if (ISPTR(np->in.type))
	  tmpoff += 4;
	else
	  tmpoff += (np->in.type.base == DOUBLE) ? 8 : 4;
	stack_temp = block(OREG, -tmpoff, TMPREG, 0, np->in.type);
	allow_insertion = YES;
	sp = symtab[find(stack_temp)];
	allow_insertion = NO;
	sp->a6n.type = np->in.type;
	sp->a6n.ptr = ISPTR(sp->a6n.type);

	stack_temp = block(ASSIGN, stack_temp, np, 0, np->in.type);
	if (*result == NULL)
	  *result = block(UNARY SEMICOLONOP, stack_temp, NULL, INT);
	else
	  *result = block(SEMICOLONOP, *result, stack_temp, INT);
} /* parm_exper_to_temp */

/******************************************************************************
*
*	PARM_EXPERS_TO_TEMPS() 
* 
*	Description:		np points to the parameter tree operand of
*				a call node.  Allocate a temporary for each
*				parameter expression and create an 
*				assignment of the parameter expression to
*				that temporary.  Return a tree of 
*				assignments to temporaries.
* 
*	Called by:		check_tail_recursion() 
* 
*	Input parameters:	np: parameter tree 
* 
*	Output parameters:	none 
* 
*	Globals referenced:	
* 
*	Globals modified:	
* 
*	External calls:		parm_exper_to_temp()
*
*******************************************************************************/
NODE *parm_expers_to_temps(np, remaining_nodes, param_count) 
  NODE *np, *remaining_nodes; 
  int  *param_count;
{
	NODE	*result = remaining_nodes; 
	NODE    *oldnp;
	
	*param_count = 0;
	while (np != NULL)
	  {
	  if (np->in.op == CM)
	    {
	    parm_exper_to_temp(np->in.right,&result);
	    oldnp = np;
	    np = np->in.left;
	    tfree1(oldnp);
	    *param_count += 1;
	    }
	  else
	    {
	    if (np->in.op == UCM)
	      {
	      parm_exper_to_temp(np->in.left,&result);
	      tfree1(np);
	      }
	    else
	      parm_exper_to_temp(np,&result);
	    np = NULL;
	    *param_count += 1;
	    }
	  }
	return result;
} /* parm_expers_to_temps */

/******************************************************************************
*
*	TEMP_TO_PARM() 
* 
*	Description:		np is a pointer to a particular entry in a
*				tree of assignments from parameter expressions 
*				to stack temporary locations.  Create an 
*				assignment of the temporary location to the
*				proper parameter location.  
* 
*	Called by:		temps_to_parms() 
* 
*	Input parameters:	np: pointer to a stack temporary node.
*				new_np: Tree of assignments being built.
*				param_offset: The offset of the parameter
*					      being assigned to.  It it
*					      does not exist in the symbol
*					      table return without doing
*					      anything.
* 
*	Output parameters:	new_np: The newly created assignment as the
*				        right operand of a SEMI COLON node
*					and the previous new_np as the left
*					hand operand.
* 
*	Globals referenced:	
* 
*	Globals modified:	
* 
*	External calls:		find()
*
*******************************************************************************/
void temp_to_parm(np,new_np,param_offset) 
  NODE *np, **new_np;
  int  param_offset;
{
	NODE *parm_node;
	NODE *temp_copy; 
	long symloc;

	parm_node = block(OREG, param_offset, A6, 0, np->in.type);
	/* Find out if the parameter location exists */
	f_do_not_insert_symbol = YES;
	symloc = find(parm_node);
	f_do_not_insert_symbol = NO; /* restore */

	if (symloc == (unsigned) (-1)) 
	  { /* parameter does not exist */
	    /* See if there is a parameter at +2 or +3 from param_offset */
	  tfree(parm_node);
	  parm_node = block(OREG, param_offset+2, A6, 0, np->in.type);
	  f_do_not_insert_symbol = YES;
	  symloc = find(parm_node);
	  f_do_not_insert_symbol = NO; /* restore */

	  if (symloc == (unsigned) (-1)) 
            {
	    tfree(parm_node);
	    parm_node = block(OREG, param_offset+3, A6, 0, np->in.type);
	    f_do_not_insert_symbol = YES;
	    symloc = find(parm_node);
	    f_do_not_insert_symbol = NO; /* restore */

	    if (symloc == (unsigned) (-1)) 
	      {
              tfree(parm_node);
              cerror("Too many parameter expressions in temp_to_parm()");
	      }
            }
          /* insert and SCONV to the type of symtab[symloc] */
          temp_copy = block(OREG, np->tn.lval, TMPREG, 0, np->in.type);
	  temp_copy = makety(temp_copy,symtab[symloc]->a6n.type);
	  parm_node->in.type = symtab[symloc]->a6n.type;
          parm_node = block(ASSIGN, parm_node, temp_copy, 0, 
						      symtab[symloc]->a6n.type);
          *new_np = block(SEMICOLONOP, *new_np, parm_node, INT);
	  }
	else
	  {
          temp_copy = block(OREG, np->tn.lval, TMPREG, 0, np->in.type);
          parm_node = block(ASSIGN, parm_node, temp_copy, 0, np->in.type);
          *new_np = block(SEMICOLONOP, *new_np, parm_node, INT);
	  }
}

/******************************************************************************
*
*	TEMPS_TO_PARMS() 
* 
*	Description:		np is a pointer to a tree of assignments from
*				parameter expressions to stack temporary 
*				locations.  Create assignments of the 
*				temporary locations to their proper parameter 
*				locations.  Return a pointer to a tree 
*				containing the assignments of parameter 
*				expressions to temporaries (the input tree)
*				concatenated with the assignments of the
*				temporaries to the parameter locations.  
* 
*	Called by:		check_tail_recursion() 
* 
*	Input parameters:	np: pointer to a tree of assignments from
*				    parameter expressions to stack temporary
*				    locations.
* 
*	Output parameters:	none 
* 
*	Globals referenced:	
* 
*	Globals modified:	
* 
*	External calls:		temp_to_parm()
*
*******************************************************************************/
NODE *temps_to_parms(top_of_temps_np, np, param_offset, param_count) 
  NODE *top_of_temps_np, *np; 
  int	param_offset, param_count;
{
	NODE	*np_temp;
	int	next_param_offset;


	if (np->in.op == SEMICOLONOP)
	    {
	    param_count -= 1;
	    if (param_count)
	      {
	      if (ISPTR(np->in.right->in.left->in.type))
	        next_param_offset = param_offset + 4;
	      else
	        next_param_offset = param_offset + 
		      ((np->in.right->in.left->in.type.base == DOUBLE) ? 8 : 4);
	      np_temp = temps_to_parms(top_of_temps_np, np->in.left,
                                       next_param_offset, param_count);
	      temp_to_parm(np->in.right->in.left,&np_temp,param_offset);
	      }
	    else
	      {
	      np_temp = top_of_temps_np;            
	      temp_to_parm(np->in.right->in.left,&np_temp,param_offset);
	      }
	    }
	if (np->in.op == UNARY SEMICOLONOP)
	    {
	    np_temp = top_of_temps_np;            
	    temp_to_parm(np->in.left->in.left,&np_temp,param_offset);
	    }
	return np_temp;
}

/*****************************************************************************
 *
 *  DELETE_PREDECESSOR()
 *
 *  Description:	Delete a LINK node to an BBLOCK lp. The link node
 *			identifies a predecessor BBLOCK to bp.
 *
 *  Called by:		check_tail_recursion()
 *
 *  Input Parameters:	predecessor -- predecessor block to delete
 *			bp -- (old) successor block
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	free_plink()
 *
 *****************************************************************************
 */

LOCAL void delete_predecessor(predecessor, bp)	BBLOCK *predecessor, *bp;
{
	register LINK *xlp;
	LINK **ylp;

	for (ylp = &bp->b.l.pred, xlp = bp->b.l.pred; xlp; 
					ylp = &xlp->next, xlp = xlp->next)
		{
		if (xlp->bp == predecessor)
			{
			*ylp = xlp->next;
			free_plink(xlp);
			return;
			}
		}
	cerror("Predecessor not found in delete_predecessor");

}  /* delete_predecessor */

/*****************************************************************************
 *
 *  FORMAL_PARAM_COUNT() 
 *
 *  Description:	Count the number of formal parameters
 *
 *  Called by:		check_tail_recursion()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *		        f_do_not_insert_symbol
 *
 *  Globals Modified:	none
 *
 *  External Calls:	find
 *			block
 *
 *****************************************************************************
 */
int formal_param_count()
{
	int count = 0;
	NODE *parm_node;
	long symloc;
	int param_offset = FIRST_PARAM_OFFSET;

      f_do_not_insert_symbol = YES;
      for (;;)
	{
	parm_node = block(OREG, param_offset, A6, 0, INT);
	/* Find out if the parameter location exists */
	symloc = find(parm_node);
	tfree(parm_node);

	if (symloc == (unsigned) (-1)) 
	  { /* parameter does not exist */
	    /* See if there is a parameter at +2 or +3 from param_offset */
	  parm_node = block(OREG, param_offset+2, A6, 0, INT);
	  symloc = find(parm_node);
	  tfree(parm_node);

	  if (symloc == (unsigned) (-1)) 
            {
	    tfree(parm_node);
	    parm_node = block(OREG, param_offset+3, A6, 0, INT);
	    symloc = find(parm_node);
	    tfree(parm_node);

	    if (symloc == (unsigned) (-1)) 
              {
	      f_do_not_insert_symbol = NO; /* restore */
	      return (count);
              }
            }
	  }
	count += 1;
	if (ISPTR(symtab[symloc]->a6n.type))
	  param_offset = param_offset + 4;
	else
	  param_offset = param_offset + 
	     ((symtab[symloc]->a6n.type.base == DOUBLE) ? 8 : 4);
	}
}

/*****************************************************************************
 *
 *  LOCAL_PARMS() 
 *
 *  Description:	Is a local variable of formal parameter being 
 *			passed as a parameter?
 *
 *  Called by:		check_tail_recursion()
 *
 *  Input Parameters:	np -- pointer to a call subtree
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	
 *
 *****************************************************************************
 */
LOCAL int local_parms(np) NODE *np;
{
if ((np->in.op == CALL) || (np->in.op == STCALL))
  {
  np = np->in.right;  /* parameter subtree */
  while (np != NULL)
    {
    NODE *lnp;
    
    if (np->in.op == CM)
      {
      lnp = np->in.right;
      np = np->in.left;
      }
    else if (np->in.op == UCM)
      {
      lnp = np->in.left;
      np = NULL;
      }
    else /* Only one parameter */
      {
      lnp = np;
      np = NULL;
      }

    if (lnp->in.op == FOREG)
      return(1); 
    else if (lnp->in.op == COMOP) 
      {
      if (lnp->in.right->in.op == FOREG)
        return(1); 
      }
    }
  }

return (0); /* No locals or formal parameters encountered. */
}

/******************************************************************************
*
*	CHECK_TAIL_RECURSION() 
* 
*	Description:		If a recursive call is the last stmt to be 
*				executed on a particular path avoid the 
*				overhead of a call by replacing it with a 
*				move of values to parameter locations and 
*				then jump to the beginning of the routine.  
* 
*	Called by:		main() 
* 
*	Input parameters:	none 
* 
*	Output parameters:	none 
* 
*	Globals referenced:	topblock
*				lastblock
*				procname 
* 
*	Globals modified:	
* 
*	External calls:		
*		   		formal_param_count
*                 		parm_expers_to_temps
*		  		temps_to_parms
*		  		tfree
*		  		tfree1
*		  		cerror        
*		  		callop        
*				addgoto;
*				delete_predecessor;
*				add_predecessor;
*
*******************************************************************************/
void check_tail_recursion()
{
	register BBLOCK *bp;
	register NODE   *np, *npcall;
	NODE   		*remaining_nodes;
	register unsigned *pp;

	/* Does this routine contain a local variable or a parameter
	 * in the ptrtab[]?  In other words, has the address been taken
	 * of a local variable or a parameter?  If so do not allow tail
	 * recursion elimination to be performed.  FSDdt07922
	 */
	for (pp = &ptrtab[lastptr]; pp >= ptrtab; pp--)
                if ((symtab[*pp]->a6n.tag == A6N) ||
		    (symtab[*pp]->a6n.tag == X6N) ||
		    (symtab[*pp]->a6n.tag == S6N)) return;

	for (bp = topblock; bp <= lastblock; bp++)
	  {
	  /* For each basic block:
	   *   - Does execution unconditionally go to the EXIT block?
	   *   - Is the last stmt of the block a CALL or a FORCE/CALL?
	   *   - Is it a call to itself?
	   */
	  if (((bp->l.breakop == FREE) || (bp->l.breakop == LGOTO)) &&
	      (bp->bb.lbp->l.breakop == EXITBRANCH))
	    if (bp->l.treep != NULL)
	      {
	      if (bp->l.treep->in.op == UNARY SEMICOLONOP)
		{
		np = bp->l.treep->in.left;
		remaining_nodes = NULL;
		}
	      else if (bp->l.treep->in.op == SEMICOLONOP)
		if (bp->l.treep->in.right->tn.op != LGOTO)
		  {
		  np = bp->l.treep->in.right;
		  remaining_nodes = bp->l.treep->in.left;
		  }
		else 
		  {
		  if (bp->l.treep->in.left->in.op == UNARY SEMICOLONOP)
		    {
		    np = bp->l.treep->in.left->in.left;
		    remaining_nodes = NULL;
		    }
		  else if (bp->l.treep->in.left->in.op == SEMICOLONOP)
		    {
		    np = bp->l.treep->in.left->in.right;
		    remaining_nodes = bp->l.treep->in.left->in.left;
		    }
	          else 
#pragma BBA_IGNORE
				  cerror("Unexpected node in check_tail_recursion");
		  }
	      else 
#pragma BBA_IGNORE
			  cerror("Unexpected node in check_tail_recursion");

	      npcall = NULL;
              if (callop(np->in.op))
		npcall = np;
	      else if (np->in.op == FORCE)
                if (callop(np->in.left->in.op))
		  npcall = np->in.left;

	      /* Is this a call to itself?  procname will not have a leading
	       * underscore.  The ICON node operand to the CALL node will
	       * have a leading underscore.
	       */
	      if ((npcall) && (npcall->in.left->tn.op == ICON) &&
		  (strcmp(procname,npcall->in.left->tn.name+1)==0))
		/* In the case of Fortran, is one of the parameter
		 * expressions a local variable or formal parameter?
		 * Do not perform tail recursion if this is the case.
		 */
		{
		NODE *copy_tree;

		if (fortran && local_parms(np))
		  return;

		/* OK, lets party.
		 * Move all parameter expressions to temporaries. Then 
		 * move the temporaries to the parameter locations.  This
		 * way any parameter expression that contains a parameter
		 * value will be evaluated correctly.  The optimizer will
		 * eliminate this store later if it is OK.  The number of
		 * parameter expressions may not match the number of
		 * expected parameters.  Use the lowest of these values
		 * when moving from temporaries to the parameter locations.
		 */ 
		if (npcall->in.op != UNARY CALL)
		  {
		  NODE *parm_tree;
		  NODE *expers_to_temps_np;
		  int  param_count = 0;

		  parm_tree = npcall->in.right;
		  while (parm_tree != NULL)
		    {
	  	    if (parm_tree->in.op == CM)
	    	      {
		      if (!ISPTR(parm_tree->in.right->in.type))
		        switch (parm_tree->in.right->in.type.base)
			  {
			  case CHAR:
			  case SHORT:
			  case INT:
			  case LONG:
			  case FLOAT:
			  case DOUBLE:
			  case ENUMTY:
			  case UCHAR:
			  case USHORT:
			  case UNSIGNED:
			  case ULONG:
			  case SCHAR:
			  case SIGNED:
		  			  param_count += 1;
					  break;
			  default: /* don't do tail recursion */
				return;
			  }
		      else /* pointer */
		  	param_count += 1;
	    	      parm_tree = parm_tree->in.left;
	    	      }
		    else
		      { 
	  	      if (parm_tree->in.op == UCM)
			parm_tree = parm_tree->in.left;
		      if (!ISPTR(parm_tree->in.type))
		        switch (parm_tree->in.type.base)
		          {
		          case CHAR:
			  case SHORT:
			  case INT:
			  case LONG:
			  case FLOAT:
			  case DOUBLE:
			  case ENUMTY:
			  case UCHAR:
			  case USHORT:
			  case UNSIGNED:
			  case ULONG:
			  case SCHAR:
			  case SIGNED:
		  			    param_count += 1;
					    break;
			  default: /* don't do tail recursion */
				return;
			  }
		      else /* pointer */
		  	param_count += 1;
	    	      parm_tree = NULL;
		      } 
		    } 
		  
		  if (param_count > formal_param_count())
		    return; 
                  expers_to_temps_np = parm_expers_to_temps(npcall->in.right,
						              remaining_nodes,
							      &param_count);
		    
		  copy_tree = temps_to_parms(expers_to_temps_np,
					       expers_to_temps_np,
					       FIRST_PARAM_OFFSET,
					       param_count); 
		  }
		else  /* UNARY CALL */
		  copy_tree = remaining_nodes;

		/* The old call related nodes must be freed.  Change the
		 * CALL node to a UNARY CALL node so that the parameter
		 * nodes will not be freed. np and npcall are the same
		 * unless there was a FORCE node.
		 */ 
		npcall->in.op = UNARY CALL;
		tfree(np);

	        if (bp->l.treep->in.op == UNARY SEMICOLONOP)
		  tfree1(bp->l.treep);
	        else /* SEMICOLONOP */
		  if (bp->l.treep->in.right->tn.op != LGOTO)
		    tfree1(bp->l.treep);
		  else 
		    {
		    tfree1(bp->l.treep->in.right);
		    tfree1(bp->l.treep->in.left);
		    tfree1(bp->l.treep);
		    }
		bp->l.treep = copy_tree;
		addgoto(bp,topblock->l.l.val);
	  	bp->l.breakop = LGOTO;
		delete_predecessor(bp,bp->bb.lbp);
		bp->bb.lbp = topblock;
		add_predecessor(bp,topblock);
	        }
	      }
	  }
} /* check_tail_recursion() */

# ifdef DEBUGGING

FILE * debugp;

struct backop {short op; char *oname;} backops[] = 
	{
	FTEXT, "FTEXT",
	FEXPR, "FEXPR",
	FSWITCH, "FSWITCH",
	FLBRAC, "FLBRAC",
	FRBRAC, "FRBRAC",
	FEOF, "FEOF",
	FARIF, "FARIF",
	LABEL, "LABEL",
	SETREGS, "SETREGS",
	ARRAYREF, "ARRAYREF",
	FMAXLAB, "FMAXLAB",
	FCOMPGOTO, "FCOMPGOTO",
	FICONLAB, "FICONLAB",
	C1SYMTAB, "C1SYMTAB",
	VAREXPRFMTDEF, "VFEDEF",
	VAREXPRFMTEND, "VFEEND",
	VAREXPRFMTREF, "VFEREF",
	C1OPTIONS, "C1OPTIONS",
	C1OREG, "C1OREG",
	C1NAME, "C1NAME",
	STRUCTREF, "STRUCTREF",
	C1HIDDENVARS, "C1HIDDEN",
	C1HVOREG, "C1HVOREG",
	C1HVNAME, "C1HVNAME",
	SWTCH, "SWTCH",
	NOEFFECTS, "NOEFFECTS",
	-1,""
	};


char * xfop(o)	register short o;
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


void lcrdebug(cp, n)	char	*cp;
{
	register i=0;

	fprintf (debugp, "\t\tatn.name = ");
	for (	; i < 4*n; i++) fprintf(debugp, "%c ",cp[i]);
	fprintf (debugp, "\n");
}



void dumpblockpool(fullflag, hexflag) flag fullflag, hexflag;
{
	BBLOCK *bp;
	register BBLOCK *bbp;
	int i;

	if (topblock)
		{
		i = 0;
		bp = bbp = topblock;
		while ( bp <= lastblock )
			{
			if (dfo[0])
				{
				bbp = dfo[i];
				fprintf(debugp, "\ndfo[%d] = 0x%x\n", i, bbp);
				fprintf(debugp,"(treep) 0x%x", bbp->l.treep);
				}
			else
				fprintf(debugp,"block[%d](0x%x) = (treep) 0x%x",
					i, bbp, bbp->l.treep);
			fprintf(debugp, hexflag? 
				", (llp) 0x%x, (rlp) 0x%x, (breakop) %s, label = %d\n":
				", (llp) %d, (rlp) %d, (breakop) %s, label = %d\n",
				bbp->b.llp, bbp->b.rlp,
				opst[bbp->b.breakop], bbp->b.l.val);
			if (fullflag)
				if (bbp->l.treep)
					fwalk( bbp->l.treep, eprint, 0 );
				else
					fprintf(debugp, 
						"\t bbp->l.treep == NULL\n");
			i++;
			bbp = ++bp;
			}
		}
	else fprintf(debugp, "blockpool empty\n");
}


void dumplblpool( predflag) 	flag predflag;
{
	register LBLOCK *lp;
	register LINK *linkp;
	int i;

	if (toplbl)
		{
		i = 0;
		lp = toplbl;
		while ( lp <= lastlbl )
			{
			fprintf(debugp,
			   "lblock[%d](0x%x) = (pred) 0x%x, (val) %d. (bp) 0x%x, (vis/pref) %d/%d\n",
				i, lp, lp->pred, lp->val, lp->bp, lp->visited,
				lp->preferred);
			if (predflag)
				for (linkp=lp->pred; linkp; linkp = linkp->next)
					{
					fprintf(debugp,"\t\tpredecessor 0x%x\n",
						linkp->bp);
					}
			i++;
			lp++;
			}
		}
	else fprintf(debugp, "labelpool empty\n");
}

# endif DEBUGGING
