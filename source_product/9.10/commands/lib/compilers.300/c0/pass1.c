/*	SCCS	REV(47.11.1.1);	DATE(90/10/15	09:38:02) */
/* KLEENIX_ID @(#)pass1.c	47.11.1.1 90/10/15 */

#include "mfile2"
#include "c0.h"

long nxtlabel;		/* next free label for use by integrated proc */

flag globaloptim = NO;	/* expecting the global optimizer (c1) to run? */

/*      call graph node definitions */

long nextnode;		/* index of next available node in free list */
pCallRecBlock callreclist;	/* pointer to list of free blocks */

/*	stack for reading nodes in postfix form */

NODE ** fstack;			/* used for caller expressions */
long fmaxstack;			/* max # items in fstack array */
NODE *node;			/* node block */
int maxnodesize = TREESZ;		/* number of nodes in node block */
NODE *freenode;  /* pointer to next free node; (for allocator) */
NODE *maxnode;  /* pointer to last node; (for allocator) */
long noderealloctimes;		/* # times node block has been malloc'd */

/*****************************************************************************/

/* pass1 -- 1st pass; collect procedure info and build call graph */
pass1()
{
    register NODE ** fsp;  /* points to next free position on the stack */
    register long x;	   /* input record */
    register NODE *p;	   /* usually current tree node */
    long	strleng;   /* length of string read from input file */
    long	i,j;
    long	lineno;	   /* source line number */
    char	filename[MAXSTRBYTES];	/* source file name */
    char	procname[MAXSTRBYTES];	/* current procedure name */
    long	procwords;	/* # words in procname */
    long entryflag = NO;	/* current proc has ENTRY */
    long isfunction = NO;	/* current item is function */
    long inthunk = NO;		/* currently in "thunk"? */
    long isthunkcall = NO;	/* current call is to thunk? */
    long currthunknameoffset;	/* offset of TNameRec in pnp */
    long junk;
    long strwords;		/* length of TEXT string */
    long stackuse;		/* # bytes in local stack */
    pPD	 lastprocpd;		/* last procedure (not entry) pd */
    long lastline;		/* last executable line number seen */
    long nlines;		/* # executable lines in proc */
    long globaloffset;		/* offset of "global" FTEXT item */
    char regs[16];		/* registers used tally */

    flag arrayflag = NO;	/* array operand flag */
    flag structflag = NO;	/* struct operand flag */
    flag arrayelement = NO;		/* array opd actually an element ? */
    long arrayoffset;			/* array base offset */
    long structoffset;			/* structure base offset */

    flag inprolog = NO;		/* inside routine prolog ? */
    flag inproc = NO;		/* In a procedure or function (C only) */
    flag inbss = NO;		/* In a data segment (C only) */
    flag imbeddedAsm = NO;	/* True iff the current routine has imbedded
				   asm statments (C only) */

    long c1options = C1OPTIONS | (CLEARC1OPTS<<16)
					| (CLEARC1OPTLEV<<8);

    NODE localnode;			/* local scratch node */
    VOLREC *volatiles = NULL;	/* volatile formal parms */
    VOLREC *voltail = NULL;	/* tail of list/volatile formal parms */

    initpd();			/* initialize proc descriptor list */
    initlab();			/* initialize labels table */
    inittnp();			/* initialize temporary name pool */
    inittree();			/* allocate storage for expression tree */
    initcallnodes();		/* allocate storage for call graph nodes */
    initfstack();		/* initialize expression stack */

    currpd = NULL;		/* no "current" procedure yet */
    dlines = defaultlines;	/* set integration size maximum */

    fsp = fstack;		/* no expression nodes yet */

    /* Read nodes in postfix order and re-construct expression trees.
     * Since we're only collecting procedure information and building
     * the call graph, most of the input is merely discarded.
     */

    for(;;){
	x = rdlong();	/* read the record */

#ifdef DEBUGGING
	if( xdebugflag ) fprintf( stdout, "\nop=%s(%d)., val = %d., rest = 0%o\n",
			xfop(FOP(x)), FOP(x), VAL(x), (int)REST(x) );
#endif

	switch( (int)FOP(x) ){  /* switch on opcode */

	case 0:			/* this should never be seen */
	    fprintf( stdout, "null opcode ignored\n" );
	    break;

	case ARRAYREF:
	    if ((!inproc) || inbss) fatal("Unexpected ARRAYREF node outside of function");
	    arrayflag = YES;
	    arrayelement = VAL(x); /* yes iff the reference is to
					a specific element */
	    arrayoffset = rdlong();
	    break;

	case STRUCTREF:
	    if ((!inproc) || inbss) fatal("Unexpected STRUCTREF node outside of function");
	    structflag = YES;
	    if (VAL(x))
	      structoffset = rdlong();
	    break;

	case STASG:		 /* NODE *newnod; */
	    if ((!inproc) || inbss) fatal("Unexpected STASG node outside of function");
	    p = getnode();
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.op = STASG;
	    p->in.right = *--fsp;
	    p->in.left = *--fsp;
#if 0
	    p->in.type = STRTY + PTR;
#endif
	    p->stn.stsize = rdlong();
	    goto bump;

	case STARG:
	    if ((!inproc) || inbss) fatal("Unexpected STARG node outside of function");
	    p = getnode();
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->in.op = STARG;
	    p->stn.stalign = VAL(x);
	    p->stn.stsize = rdlong();
	    p->tn.rval = 0;
	    p->in.left = *--fsp;
	    goto bump;

	case STCALL:
	case UNARY STCALL:
	    if ((!inproc) || inbss) fatal("Unexpected STCALL node outside of function");
	    p = getnode();
	    p->in.op = FOP(x);
	    p->stn.stalign = VAL(x);
	    gettype(x,&p->in.type, &p->in.tattrib);
	    p->stn.stsize = rdlong();
	    if (optype(p->in.op) == BITYPE)
		p->in.right = *--fsp;
	    else
		p->tn.rval = REST(x);
	    p->in.left = *--fsp;
	    arrayflag = NO;
	    arrayelement = NO;
	    structflag = NO;
	    goto bump;

	case NOEFFECTS:
	    if (ftnflag) fatal("Unexpected NOEFFECTS node in FORTRAN code");
	    putlong(x);
	    rdstring();
	    putstring(strbuf);
	    break;

	case FMAXLAB:		/* max label value gen'd by f77pass1/cpass1 */
	    if (inbss) fatal("Unexpected FMAXLAB node in bss section");
	    nxtlabel = rdlong() + 1;	/* start c0 labels after it */
	    break;

	case NAME:		/* static variable ref's */
	    if ((!inproc) || inbss) fatal("Unexpected NAME node outside of function");
	    p = getnode();		/* store leaf NAME node for tree */
	    p->in.op = NAME;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    if( VAL(x) )
		{
		p->tn.lval = rdlong();
		}
	    else p->tn.lval = 0;
	    strwords = rdstring();
	    p->tn.rval = strwords;
	    p->in.name = addtotnp(strbuf, strwords);
					/* put name in temp name pool */
	    arrayflag = NO;
	    arrayelement = NO;
	    structflag = NO;
	    goto bump;

	case ICON:
	    if ((!inproc) || inbss) fatal("Unexpected ICON node outside of function");
	    p = getnode();		/* store leaf ICON node for tree */
	    p->in.op = ICON;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = 0;
	    p->tn.lval = rdlong();
	    if( VAL(x) )
		{
		strwords = rdstring();
	        p->tn.rval = strwords;
	        p->in.name = addtotnp(strbuf,strwords); /* add name to tnp */
		}
	    else
	      {
	      p->in.name = 0;
	      goto bump;
	      }
	    arrayflag = NO;
	    arrayelement = NO;
	    structflag = NO;

bump:
	    *fsp++ = p;			/* remember node in postfix stack */
	    if( fsp >= &fstack[fmaxstack] )	/* stack is full -- expand */
		{
		NODE **fsptemp = fsp;
		reallocfstack(&fsptemp);
		fsp = fsptemp;
		}
    	    break;

	case C1HIDDENVARS:
	    if ((!inproc) || inbss) fatal("Unexpected C1HIDDENVARS node outside of function");
	    if (!ftnflag) fatal("Unexpected C1HIDDENVARS node");
	    {
	    int i,j;

	    j = REST(x);
	    for (i=0; i < j; ++i)
	      {
	      x = rdlong();
	      if (FOP(x) == C1HVOREG)
		{
		rdlong();
		}
	      else if (FOP(x) == C1HVNAME)
		{
		rdlong();
		strwords = rdstring();
		}
	      else
		fatal("unexpected opcode in C1HIDDENVARS");
	      }
	    }
	    break;

	case GOTO:
	    if ((!inproc) || inbss) fatal("Unexpected GOTO node outside of function");
	    if (VAL(x))
		junk = rdlong();
	    else
		goto def;
	    break;

	case FENTRY:
	    if ((!inproc) || inbss) fatal("Unexpected FENTRY node outside of function");
	    /* alternate entry point code */
	    procwords = rdstring(); /* entry point name */
		/* remember offset of entry point start for pass2 so that we
		 * can seek directly to it then
		 */
	    globaloffset = ftell(ifd) - procwords*4 - 4;
	    *(procname) = '_';		/* add leading underscore */
	    for ( i = 0; i < (procwords*4); i++ )
	      { char *t; int r;
	      t = procname + i + 1;	/* adjust for leading underscore */
	      *t = strbuf[ i ];
	      if ( ! *t )
		{ r = ( i + 1 ) % 4;
		if ( ! r )  /* make sure word length includes trailing nulls */
		  { procwords++;
		  r = 1;
		  }
		for ( i = r; i < 4; i++ )
		  *++t = '\0';
		break;
		}
	      }
	    entryflag = YES;	/* next "global" is an ENTRY */
	    break;

	case SETREGS:
	    if (!inproc) fatal("Unexpected SETREGS node outside of function");
	    /* reserve/release certain registers as regvars used in pass1 */
	    currpd->regsused = REST(x);  /* remember register use mask */
	    currpd->fdregsused = rdlong();
	    break;

	case REG:
	    if ((!inproc) || inbss) fatal("Unexpected REG node outside of function");
	    p = getnode();	/* store leaf REG node for tree */
	    p->in.op = REG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = VAL(x);
	    regs[p->tn.rval] = 1;	/* remember registers used */
	    p->tn.lval = 0;
	    p->in.name = NULL;
	    goto bump;

	case OREG:
	    if ((!inproc) || inbss) fatal("Unexpected OREG node outside of function");
	    p = getnode();	/* store leaf OREG node for tree */
	    p->in.op = OREG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = VAL(x);
	    regs[p->tn.rval] = 1;	/* remember registers used */
	    p->tn.lval = rdlong();
	    p->in.name = NULL;
	    arrayflag = NO;
	    arrayelement = NO;
	    structflag = NO;
	    goto bump;

	case FTEXT:		/* decode comments for info */
	    strwords = VAL(x);
	    rdnlongs( strwords );
	    strbuf[strwords*4] = '\0';
	    if ((!inproc) || inbss)
		{
		putlong(x);
		putnlongs(strwords);
		}
	    if (!strncmp(strbuf, "#7", 2))
		{		/* peephole optimizer, c2, volatile flags */
		long off, siz;
		VOLREC *t;
		if ( sscanf( strbuf, C2PATTERN, &off, &siz ) == 2 )
		  {		/* only interested in possible formals */
				/* will wait till doip() to check */
		  t = (VOLREC *) malloc( sizeof(VOLREC) );
		  if ( t == NULL )
			fatal("Out of memory building volatile flag list.");
		  t->next = NULL;
		  t->off = off;
		  t->siz = siz;
		  if ( volatiles == NULL )
		    volatiles = t;
		  else
		    voltail->next = t;
		  voltail = t;
		  }
		}
	    else if (!strncmp(strbuf, "#5 retval", 9))
		{
		isfunction = YES;	/* function, not subroutine */
		}
	    else if (!strncmp(strbuf, "\ttext", 5))
		{
		inbss = NO;
		}
	    else if (!strncmp(strbuf, "\tbss", 4))
	 	{
		if (ftnflag) fatal("Unexpected bss segment in FORTRAN source");
		if (!inbss)
		   {
		   inbss = YES;
		   putlong(x);
		   putnlongs(strwords);
		   }
		}
	    else if (!strncmp(strbuf, "\tdata", 5))
	 	{
		if (ftnflag) fatal("Unexpected data segment in FORTRAN source");
		if (!inbss)
		   {
		   inbss = YES;
		   putlong(x);
		   putnlongs(strwords);
		   }
		}
	    else if (!strncmp(strbuf, "#c0 ", 4))  /* special c0 comments */
		{

		switch (strbuf[4]) {
		case 's':		/* subroutine/function defn */
		    {
		    long nparms;	/* number of formal parameters */
		    register TWORD *p;
		    register char *i,*j;

		    sscanf(strbuf+14,"%4x",&nparms);	/* decode # parms */
		    currpd = getpd(nparms);	/* allocate proc descriptor */
		    addpd(currpd);		/* add to list of procs */
		    currpd->nparms = nparms;	/* store nparms in pd */
		    currpd->pnpoffset = addtopnp(procname,procwords);
					/* add name to permanent name pool */
		    currpd->stackuse = stackuse;  /* remember local stack use */
		    if (entryflag)	/* ENTRY's have 2-bit set in typeproc */
		        currpd->typeproc = isfunction ? 3 : 2;
		    else		/* SUBROUTINE/FUNCTION definition */
			{
			currpd->typeproc = isfunction;
		        lastprocpd = currpd;
			lastline = 0;		/* no "last-executable" line */
			nlines = 0;		/* # executable lines == 0 */
			memset(regs,0,16);	/* clear regs-used array */
			}
		    currpd->c1opts = c1options;

		    c1options = C1OPTIONS | (CLEARC1OPTS<<16)
					| (CLEARC1OPTLEV<<8);
		    isfunction = NO;		/* reset for next "global" */
		    entryflag = NO;		/* reset for next "global" */
		    currpd->inoffset = globaloffset;  /* store offset of
						       * "global" text */
		    /* decode function return type */
		    sscanf(strbuf+5,"%8x",&(currpd->rettype));

		    /* decode formal parameter types */
		    if (nparms <= MAXARGS)
		        for (i=strbuf+19,j=strbuf+19+nparms*9,p=currpd->parmtype ;
		             i < j; i+=9,p++)
			    sscanf(i,"%8x",p);
		    }

		    /* save force/omit environment at textual start of proc */
		    saveforceomitoffsets( &(currpd->nforced), &(currpd->nomitted),
				&(currpd->forceoffsets), &(currpd->omitoffsets),
				&(currpd->begindlines));
		    break;

		case 'c':		/* call info */
		    {
		    register NODE *nodep;  	/* pointer to ICON/NAME node */
		    long to;			/* offset of callee name */
		    pNameRec pn;		/* pnp record of target proc */

		    nodep = *(fsp-1);	/* CALL node on top-of-stack */
		    if ((nodep->in.op != CALL) &&
			(nodep->in.op != UNARY CALL) &&
			(nodep->in.op != STCALL) &&
			(nodep->in.op != UNARY STCALL))
			fatal("call comment not after CALL/UCALL node");
		    nodep = nodep->in.left;	/* left child is callee name */

		    if (nodep->in.op != ICON)	/* dummy proc argument */
			break;			/* can't do anything with it */

		    /* create pnp record for called procedure to store info */
		    to = addtopnp(nodep->in.name, nodep->tn.rval);
		    pn = ((pNameRec)(pnp+to));

		    /* if FORMAT "thunk" call, don't create call record because
		     *    we'll always inline thunks and only one procedure
		     *    can call them -- don't clutter up call graph.
		     * We can never inline calls in a variable-expression
		     *    FORMAT, nor those explicitly omitted, so don't add
		     *    these calls to graph, either.
		     */
		    if (isthunkcall)
			isthunkcall = NO;
		    else if ((! pn->omitflag) && (! inthunk))
		        addcall(lastprocpd,to,pn->forceflag,dlines);
		    }
		    break;
		case 'a':		/* assign statement follows */
		    break;
		case 'p':		/* prolog end */
		    inprolog = NO;
		    break;
		case 'r':		/* return value offset */
		    sscanf(strbuf+5,"%8x",&(currpd->retoffset));
		    break;
		case 'd':		/* adjustable dimension bytes */
		    sscanf(strbuf+5,"%8x",&(currpd->dimbytes));
		    break;
		case 'f':		/* force routines inline */
		    doforcelist(strbuf+5,NO);
		    break;
		case 'o':		/* force routines not inline */
		    doomitlist(strbuf+5,NO);
		    break;
		case 'n':		/* do not emit standalone */
		    donoemitlist(strbuf+5);
		    break;
		case 'D':		/* reset force/omit defaults */
		    dodefaultlist(strbuf+5);
		    break;
		case 'l':		/* set max size of inlineable proc */
		    sscanf(strbuf+5,"%d",&dlines);
		    break;
		case 'R':		/* alternate return stuff */
		    currpd->hasaltrets = 1;
		    currpd->rettype = LONG;
		    break;
		case 'q':		/* C return statement */
		    break;
		case 't':		/* variable expr FORMAT label */
		    if (strbuf[5] == 'r')  /* "r"eference -- thunk call */
			isthunkcall = YES;
		    else if (strbuf[5] == 'l')  /* label -- thunk definition */
			inthunk = YES;
		    else if (strbuf[5] == 'a')  /* ASSIGN'd FORMAT usage flag */
			currpd->hasasgnfmts = YES;
		    break;

		default:
		    fatal("unrecognized c0 comment");
		}
		}
	    else if (inthunk)
		{
		/* We're in thunk definition.  We must record the thunk name
		 * and offset in a pnp name record so that we can inline the
		 * the thunk during pass 2.  Thunks do not have a "global"
		 * pseudo-op tagging them, so we pick up the label directly
		 * from the assembly code -- it will be the only assembly
		 * code with '_' in column 1.
		 */
		if (strbuf[0] == '_')			/* thunk label name */
		    {
		    char *ps = strbuf+1;
		    pTNameRec pthunk;
		    long labelwords;

		    /* replace trailing ':' with NULL */
		    while (*ps++ != ':')
			;
		    *--ps = '\0';
		    *++ps = '\0';
		    *++ps = '\0';
		    *++ps = '\0';

		    /* adjust size of label to account for omitted ':' */
		    labelwords = strwords;
		    if ((ps - strbuf) % 4 == 2)
			labelwords--;

		    /* add label to pnp */
		    currthunknameoffset = addtopnp(strbuf,labelwords);
		    pthunk = (pTNameRec) (pnp + currthunknameoffset);
		    pthunk->fileoffset = ftell(ifd);	/* record offset */
		    pthunk->isempty = YES;	/* no code in thunk (so far) */
		    }
		}
	    break;

	case VAREXPRFMTDEF:
	    if ((!inproc) || inbss) fatal("Unexpected VAREXPRFMTDEF node outside of function");
# if 0
	    x = rdlong();
	    if (FOP(x) != FTEXT)
	      fatal("no FTEXT after VAREXPRFMTDEF");
	    strwords = rdnlongs(VAL(x));
	    strbuf[VAL(x) * 4] = '\0';	/* in case all words filled */
	    x = strlen( strbuf );
	    strbuf[ x - 2 ] = '\0';	/* erase the ':\n' */
# endif 0
	    break;

	case VAREXPRFMTEND:
	    if ((!inproc) || inbss) fatal("Unexpected VAREXPFMTEND node outside of function");
	    inthunk = 0;	/* turn off flag */
	    break; 

	case VAREXPRFMTREF:
	    if ((!inproc) || inbss) fatal("Unexpected VAREXPFMTREF node outside of function");
	    break;

	case FEXPR:		/* expression root node, process tree */
	    lineno = REST(x);	/* remember line number for messages */
	    if (inthunk)	/* we're seeing executable code in thunk */
		((pTNameRec)(pnp+currthunknameoffset))->isempty = NO;
	    else if (lineno != lastline)	/* new executable source line */
		{
		lastline = lineno;
		nlines++;	      /* increment # executable lines in proc */
		}
	    if( strwords = (VAL(x)) )	/* a file name !!! */
		rdnlongs( strwords );
	    if( fsp == fstack )   /* filename only */
		{
		if ((!inproc) || inbss)
		    {
		    putlong(x);
		    if (strwords)
			putnlongs(strwords);
		    }
		break;
		}
	    else
		{
		if ((!inproc) || inbss)
		    fatal("non-filename FEXPR node outside of function");
		}

	    if( --fsp != fstack )
		{
		fatal( "expression poorly formed (pass1)" );
		}
	    freetree();		/* recover all tree/namepool space */
	    break;

	case FSWITCH:
	    if ((!inproc) || inbss) fatal("Unexpected FSWITCH node outside of function");
	    fatal( "switch not yet done" );
	    break;

	case FCMGO:
	    if ((!inproc) || inbss) fatal("Unexpected FCMGO node outside of function");
	    for (i = REST(x); i > 0; i--)
	        rdlong();		/* consume labels and throw away */
	    break;

	case SWTCH:
	    if ((!inproc) || inbss) fatal("Unexpected SWTCH node outside of function");
	    for (i = REST(x); i > 0; i--)
		{
		rdlong();		/* consume labels and throw away */
		rdlong();
		}
	    break;

	case FLBRAC:			/* beginning of procedure */
	    if (inproc) fatal("Nested FLBRAC nodes");
	    if (inbss) fatal("Unexpected FLBRAC node in bss area");
	    inproc = YES;
	    imbeddedAsm = (VAL(x) & 0x80);
	    rdlong();		/* extra word for C1-added temporaries */
	    stackuse = rdlong();	/* remember local stack use of proc */
	    rdlong();
	    procwords = rdstring(); /* procedure name */
		/* remember offset of procedure start for pass2 so that we
		 * can seek directly to procedure in input file
		 */
	    globaloffset = ftell(ifd) - procwords*4 - 4*4;
	    *(procname) = '_';		/* add leading underscore */
	    for ( i = 0; i < (procwords*4); i++ )
	      { char *t;
		int r;
	      t = procname + i + 1;	/* adjust for leading underscore */
	      *t = strbuf[ i ];
	      if ( ! *t )
		{
		r = ( i + 1 ) % 4;
		if ( ! r )  /* make sure word length includes trailing nulls */
		  {
		  procwords++;
		  r = 1;
		  }
		for ( i = r; i < 4; i++ )
		  *++t = '\0';
		break;
		}
	      }
	    inprolog = YES;
	    volatiles = NULL;
	    break;

	case FRBRAC:		/* end of procedure */
	    if ((!inproc) || inbss) fatal("Unmatched FRBRAC node");
	    inproc = NO;
	    lastprocpd->nlab = nxtfreelab;	/* number of labels seen in proc */
	    /* copy labels from temp array to label array for proc */
	    if ((lastprocpd->labels = (long*) malloc(nxtfreelab*sizeof(long)*2))
		== NULL)
		fatal("malloc failed for FRBRAC -- phase 1");
	    memcpy(lastprocpd->labels,labarray,nxtfreelab*sizeof(long));
	    nxtfreelab = 0;		/* reset temp label array counter */
#ifdef DEBUGGING
	    /*dumplabels(currpd->labels,currpd->nlab);
	    */
#endif
	    /* store number of executable lines in procedure */
	    lastprocpd->nlines = nlines;

	    /* set flag indicating formal arg(s) marked "c2" volatile */
	    lastprocpd->vollist = volatiles;
	    volatiles = NULL;

	    /* set flag indicating imbedded asm statements */
	    lastprocpd->imbeddedAsm = imbeddedAsm;

	    /* add A-registers to register usage info */
	    {
	    register long regmask = 0;
	    register long inx;
	    for (inx = 15; inx >= 0; inx--)
		if (regs[inx])
		    regmask = (regmask << 1) | 1;
		else
		    regmask <<= 1;
	    regmask &= 0x3cfc;		/* skip A7, A6, A1, A0, D1, D0 */
	    lastprocpd->regsused |= regmask;	/* store register usage */
	    /* ??? what about flt pt regs */
	    }
	    break;

	case FEOF:		/* end of file */
	    return;

	case LABEL:		/* label */
	    if ((!inproc) || inbss) fatal("Unexpected LABEL node outside of function");
	    {
	    /* record label in temp label array */
	    long lab = (long) (((x) >> 8) & 0xffffff);
	    if (nxtfreelab >= maxlabels)	/* label array is full */
		realloclab(maxlabels+1);	/* create bigger array */
	    labarray[nxtfreelab++] = lab;	/* add this label */
	    }	
	    break;

	case FICONLAB:	/* ICON contain label value due to ASSIGN */
	    if ((!inproc) || inbss) fatal("Unexpected FICONLAB node outside of function");
	    break;

	case C1SYMTAB:
	    if ((!inproc) || inbss) fatal("Unexpected C1SYMTAB node outside of function");
	    if (VAL(x) == 1)
	      {
	      farg_high = rdlong();
	      farg_low = rdlong();
	      farg_pos = rdlong();
	      }
	    globaloptim = YES;
	    break;

	case C1NAME:		/* copy it in C, toss it in FORTRAN */
	    p = &localnode;
	    p->tn.op = NAME;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    if ( VAL(x) )
	      p->tn.lval = rdlong();
	    else
	      p->tn.lval = 0;
	    strwords = rdstring();

	    if (!ftnflag)
		{
		puttype(C1NAME,VAL(x),p->in.type,p->in.tattrib);
		if (VAL(x)) putlong(p->tn.lval);
		putstring(strbuf);
		}

	    x = rdlong();
	    if ( x & C1ARY )
	      j = rdlong();
	    strwords = rdstring();

	    if (!ftnflag)
		{
		putlong(x);
		if (x & C1ARY) putlong(j);
		putstring(strbuf);
		}

	    break;

	case C1OREG:
	    if (!inproc) fatal("Unexpected C1OREG node outside of function");
	    p = getnode();
	    p->tn.op = OREG;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.lval = rdlong();
	    x = rdlong();
	    if ( x & C1ARY )
	      (void) rdlong();
	    strwords = rdstring();
	    break;

	case FLD:
	    if ((!inproc) || inbss) fatal("Unexpected FLD node outside of function");
	    p = getnode();
	    p->tn.op = FLD;
	    gettype(x, &p->in.type, &p->in.tattrib);
	    p->tn.rval = rdlong();
	    p->in.left = *--fsp;
	    goto bump;

	case QUEST:
	    fatal("op = QUEST seen in input stream");
	    break;

	case COLON:
	    fatal("op = COLON seen in input stream");
	    break;

	case ANDAND:
	    fatal("op = ANDAND seen in input stream");
	    break;

	case OROR:
	    fatal("op = OROR seen in input stream");
	    break;

	case C1OPTIONS:
	    if ((!inproc) || inbss) fatal("Unexpected C1OPTIONS node outside of function");
	    c1options = x;
	    globaloptim = YES;
	    break;

	default:		/* all interior (operator) OPs */
	    if ((!inproc) || inbss)
		{
		sprintf(warnbuff,"Unexpected node (%d) outside of function",(int)FOP(x));
		fatal(warnbuff);
		}
def:
	    arrayflag = NO;
	    arrayelement = NO;
	    structflag = NO;

	    p = getnode();	/* save interior node of tree */
	    p->in.op = FOP(x);
	    gettype(x, &p->in.type, &p->in.tattrib);

	    switch( optype( p->in.op ) ){

	    case BITYPE:		/* binary operator -- set children */
		p->in.right = *--fsp;	/* top on postfix stack */
		p->in.left = *--fsp;	/* next on postfix stack */
		goto bump;		/* store this node on stack */

	    case UTYPE:			/* unary operator -- set children */
		p->in.left = *--fsp;	/* top on postfix stack */
		p->tn.rval = 0;
		goto bump;		/* store this node on stack */

	    case LTYPE:			/* leaf node */
		sprintf(warnbuff,"illegal leaf node (pass1): %d", p->in.op );
		fatal(warnbuff);
	    }
	}
    }
}  /* pass1 */

/*****************************************************************************/

/* inittree -- initialize caller expression tree storage */

inittree()
{
    register NODE *p, *end;

    /* allocate node table */
    if ((node = (NODE *) malloc ( maxnodesize * sizeof (NODE) + sizeof(NODE**) ))
	== NULL)
	fatal ("Out of memory in inittree()");
    *((NODE**)node) = (NODE*) 0;	/* no previous node tables */
    ((NODE**)node)++;			/* table starts after link pointer */
    freenode = node;			/* free nodes are at start of table */
    maxnode = &(node[maxnodesize-1]);	/* last node at end of table */
}  /* inittree */

/*****************************************************************************/

/* allocate next free tree node (caller space) */
NODE* getnode()
{
    if (freenode <= maxnode)	/* there are still free nodes in table */
	{
#ifdef C1
	freenode->in.c1data.c1word = 0;	 /* clear C1 data fields */
#endif
	freenode->in.left = NULL;
	freenode->in.right = NULL;
	return(freenode++);
	}
    else			/* no more free nodes -- allocate next table */
	{
	NODE * oldnode = node;
        if ((node = (NODE *) malloc ( maxnodesize * sizeof (NODE) + sizeof(NODE**) ))
	    == NULL)
	    fatal ("Out of memory in getnode()");
        *((NODE**)node) = oldnode;	/* store addr of previous */
        ((NODE**)node)++;		/* table starts after link pointer */
        freenode = node;		/* free nodes start at beginning */
        maxnode = &(node[maxnodesize-1]);	 /* set end of table pointer */
	noderealloctimes++;		/* flag a reallocation */
#ifdef C1
	freenode->in.c1data.c1word = 0;  /* clear C1 data fields */
#endif
	freenode->in.left = NULL;
	freenode->in.right = NULL;
	return(freenode++);
	}
}  /* getnode */

/*****************************************************************************/

/* free expression tree storage (caller space) */
freetree()
{
    NODE * oldnode;
    register long inx;

    freetnp = tnp;	/* reset temporary name pool to beginning */
    if (noderealloctimes)	/* more than one node array around -- collapse*/
	{
	for (inx = noderealloctimes; inx > 0; inx--)
	    {
	    oldnode = *(--((NODE**)node));	/* get addr of previous table */
	    free(node);			/* free current table */
	    node = oldnode;
	    }
	maxnodesize *= noderealloctimes;	/* allocate single large table */
        if ((node = (NODE *) malloc ( maxnodesize * sizeof (NODE) + sizeof(NODE**) ))
	    == NULL)
	    fatal ("Out of memory in freetree()");
        *((NODE**)node) = (NODE*) 0;	/* no previous node tables */
        ((NODE**)node)++;
        maxnode = &(node[maxnodesize-1]);
	noderealloctimes = 0;
	}
    freenode = node;	/* free nodes start at beginning of table */
}  /* freetree */

/*****************************************************************************/

/* allocate expression stack for building trees from postfix input */
initfstack()
{
    fmaxstack = NSTACKSZ;
    if ((fstack = (NODE **) malloc( fmaxstack * sizeof(NODE*))) == NULL)
	fatal("Out of memory in initfstack()");
}  /* initfstack */

/*****************************************************************************/

/* reallocate expression stack */
reallocfstack(pfsp)
NODE ***pfsp;		/* pointer to current top-of-stack pointer */
{
    long fspoffset;	/* relative offset of top-of-stack pointer */

    fspoffset = *pfsp - fstack;	 /* calculate relative offset */
    fmaxstack += NSTACKSZ;	 /* bump stack size */
    if (( fstack = (NODE**) realloc(fstack,fmaxstack*sizeof(NODE*))) == NULL)
	fatal("Out of memory in reallocfstack()");
    *pfsp = fstack + fspoffset;	 /* store new (absolute) top-of-stack pointer */
}  /* reallocfstack */

/*****************************************************************************/

/* initialize call graph node free list */
initcallnodes()
{
    if ((callreclist = (pCallRecBlock) calloc(1,sizeof(CallRecBlock))) == NULL)
	fatal("Out of memory in initcallnodes()");
    nextnode = 0;  /* next free node is the first one */
}  /* initcallnodes */

/*****************************************************************************/

/* allocate and return call node */
pCallRec getcallrec()
{
    pCallRecBlock  newblock;	/* new block of free nodes */

    if (nextnode >= MAXNODES)	/* no more free nodes -- allocate 2nd table */
	{
	if ((newblock = (pCallRecBlock) calloc(1,sizeof(CallRecBlock))) == NULL)
	    fatal("Out of memory in getcallrec()");
	newblock->next = callreclist;	/* save pointer to previous table */
	callreclist = newblock;	 /* 2nd table is now primary */
	nextnode = 0;		/* next free node is the first one in table */
	}

    return(&(callreclist->node[nextnode++]));
}  /* getcallrec */

/*****************************************************************************/

/* add node to call graph */
addcall(from,to,forced,lines)
pPD from;	/* "from" procedure */
long to;	/* offset of "to" name entry in pnp */
long forced;	/* 1 if forced inline */
long lines;	/* current default size */
{
    register pCallRec i;
    pNameRec pto = (pNameRec) (pnp + to);

    /* is caller-callee pair already in graph? */
    for (i=from->callsto; i ; i=i->nextto)
	{
	if ((((long)(i->callsto)) == to) && (i->forced == forced)
         && (i->clines == lines))	/* yes, already in graph with same
					 * lines/forced characteristics */
	    {
	    i->count++;			/* simply increment the count field */
	    return(0);
	    }
	}

    /* not in current graph -- add node */
    i = getcallrec();
    i->calledfrom = from;
    i->callsto = (pPD) to;
    i->nextto = from->callsto;
    from->callsto = i;
    i->nextfrom = pto->ffrom;
    pto->ffrom = i;
    i->count = 1;
    i->forced = forced;
    i->clines = lines;
    return(0);
}  /* addcall */

/*****************************************************************************/

/* freecallgraph -- free malloc'd space used by call graph */
freecallgraph()
{
    pCallRecBlock nxtblk;

    while (callreclist)	 /* go thru list of malloc'd tables -- free each one */
	{
	nxtblk = callreclist->next;
	free(callreclist);
	callreclist = nxtblk;
	}
}  /* freecallgraph */

/*****************************************************************************/

/* isforced -- return 1 if procedure currently "forced" inline */
isforced(offset)
long offset;	/* offset of pnp name record */
{
return(((pNameRec)(pnp+offset))->forceflag);
}  /* isforced */

/*****************************************************************************/

/* isomitted -- return 1 if procedure currently "omitted" inline */
isomitted(offset)
long offset;	/* offset of pnp name record */
{
return(((pNameRec)(pnp+offset))->omitflag);
}  /* isomitted */
