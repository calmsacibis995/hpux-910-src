/*	SCCS	REV(47.13);	DATE(90/05/16	13:31:41) */
/* KLEENIX_ID @(#)pass2.c	47.13 90/04/18 */

#include "mfile2"
#include "c0.h"

FILE *ipfd;		/* integrated proc file descriptor */

/*	stack for reading nodes in postfix form */


NODE **ipstack;		/* used for callee input */
long ipmaxstack;	/* max index of expression stack */
NODE *ipnode;		/* expression tree nodes (callee) */
long maxipnodesz = TREESZ;	/* # expression tree nodes (callee) */
NODE *ipfreenode;  /* pointer to next free node; (for allocator) */
NODE *ipmaxnode;  /* pointer to last node; (for allocator) */
long ipnoderealloctimes;  /* # times ipnode array alloc'd for current tree */

long	maxstack;	/* max stack use per procedure */

long 	nargs;		/* number of arguments in call */
long	maxargs = MAXARGS;	/* max supported # args */
long	minargoffset;	/* last arg offset from %a6 */
long	cstackoffset; /* C only - the base of the stack where args are eval */
NODE	*arg[MAXARGS];	/* pointers to argument expressions */
flag	argvf[MAXARGS];	/* 1=c0 temp used, may need c2 volatile flag */

pPD	ippd;		/* pd for integrated proc */
long tempbase;		/* offset to subtract from all temporaries */

ExprCallRec call[MAXEXPRCALLS];	/* pointers to all calls in expression */
long nxtcall;		/* next available slot in "call" array */
TWORD argtypes[MAXARGS];	/* arg types */
long callerSetregsMask;	/* SETREGS environment in calling routine */
long callerSetregsFDMask;
long callerC1opts;	/* C1 Options envir. in calling routine */

char	tmpfilename[] = "/tmp/Fc0XXXXXX";

/*****************************************************************************/

/* pass2 -- 2nd pass; discover and do procedure integration */
pass2()
{
    register NODE ** fsp;  /* points to next free position on the stack */
    register long x;	   /* input record */
    register NODE *p;	   /* current tree node */
    long	i;
    long	lineno;	   /* source line number (for messages) */
    char	filename[MAXSTRBYTES];	/* source file name (for messages) */
    long	strwords;		/* # 4-byte words in string */
    long	inthunk = NO;		/* currently in FORMAT "thunk" ? */
    long	isthunkcall = NO;	/* is this call to "thunk" ? */
#ifdef C1
    C1DATA	c1nodesseen;
#endif
    long arrayoffset;
    long structoffset;
    NODE	localnode;	/* local tree node, for scratch use */
    char	*lname;		/* local name pointer */
    HVREC	*hvlist=NULL, *hvtail;  /* c1 hidden vars list */
    flag	inbss = NO;
    flag	altreturn = NO;

    /* Initialize files and data structures */

    nxtlabel++;			/* increment available label counter */
    initipnp();			/* initialize callee name pool */
    initiptree();		/* allocate storage for callee expr tree */
    initipstack();		/* initialize callee expression stack */
    dlines = defaultlines;	/* set maximum size of inlineable proc */

    opermfd = ofd;
    ipermfd = fileopen(outfilename,"r");  /* open target procedure input file */

    if (noemitseen)	/* there are procs which aren't output stand-alone */
	{
	mktemp(tmpfilename);		     /* create file name */
	otmpfd = fileopen(tmpfilename,"w");
	itmpfd = fileopen(tmpfilename,"r");
	unlink(tmpfilename);		     /* disappears with this process */
	}

    c1nodesseen.c1word = 0;	/* no C1 data seen yet */

    fsp = fstack;		/* no expression nodes yet */

    puttext("\ttext");		/* start assembly file in "text" */

    /* step through all procedures in output order, processing them */
    for (currpd=firstoutpd; currpd; currpd=currpd->nextout)
	{
	if (currpd->typeproc & 2) continue;	/* skip ENTRYs */

#if 0
	if ( (currpd->typeproc & 1) && currpd->rettype == LONGDOUBLE )
	  {		/* issue warnings if attempted quad-real force */
	  if (((pNameRec)(currpd->pnpoffset+pnp))->initflag == -1)
	    {
	    sprintf(warnbuff,"FUNCTION \"%s\" forced by command line option (-Wp,-f)\n\t-- real*16 functions cannot be inlined\n\tFILE \"%s\", LINE %d definition -- forces for this function ignored",
		((pNameRec)(currpd->pnpoffset+pnp))->name, filename, lineno);
	    warn(warnbuff);
	    }
	  else if (((pNameRec)(currpd->pnpoffset+pnp))->everforced) 
	    {
	    sprintf(warnbuff,"FUNCTION \"%s\" forced by source directive ($INLINE FORCE)\n\t -- real*16 functions cannot be inlined\n\tFILE \"%s\", LINE %d definition -- forces for this function ignored",
		((pNameRec)(currpd->pnpoffset+pnp))->name, filename, lineno);
	    warn(warnbuff);
	    }
	  }
#endif

	fseek(ifd,currpd->inoffset,0);	/* seek to start of procedure */

	/* set output file */
	fflush(ofd);
	if (currpd->noemitflag)
	    ofd = otmpfd;	/* no-emit procs go to temp file */
	else
	    ofd = opermfd;	/* regular procs to regular output file */

	/* set up temporary labels array to hold all new labels added in
	 * integration + the old ones.  At end of proc, replace old labels
	 * array with this one.
	 */
	memcpy(labarray, currpd->labels, currpd->nlab*4);
	nxtfreelab = currpd->nlab;

	/* restore force/omit context at beginning of procedure */
	restoreforceomitoffsets( currpd->nforced, currpd->nomitted,
		currpd->forceoffsets, currpd->omitoffsets, currpd->begindlines);

	/* read input nodes under end-of-procedure (RBRACKET) and process
	 * them (in-line calls where appropriate).
	 */
	for(;;){
	    x = rdlong();	/* read an input record */

#ifdef DEBUGGING
	    if( xdebugflag ) fprintf( stdout, "\nop=%s., val = %d., rest = 0%o\n",
			xfop(FOP(x)), VAL(x), (int)REST(x) );
#endif
	    switch( (int)FOP(x) ){  /* switch on opcode */

	    case 0:		/* should never be seen */
	        fprintf( stdout, "null opcode ignored\n" );
	        break;

	    case C0TEMPASG:
	        putlong(x);
	        break;

	    case FMAXLAB:	/* maximum label value gen'd by f77pass1 */
	        rdlong();
	        break;
    
	    case FENTRY:
		strwords = rdstring();
		putlong(x);
		putstring(strbuf);
		break;

	    case ARRAYREF:
		c1nodesseen.c1flags.isarray = 1;
		c1nodesseen.c1flags.iselement = VAL(x);
	        			 /* yes iff the reference is to
					    a specific element */
	        arrayoffset = rdlong();
	        break;
    
	    case STRUCTREF:
		c1nodesseen.c1flags.isstruct = YES;
		c1nodesseen.c1flags.isprimaryref = VAL(x);
	        if ( VAL(x) )
	          structoffset = rdlong();
	        else
	          structoffset = 0;
	        break;

	    case STASG:		 /* NODE *newnod; */
		p = getnode();
		p->in.op = STASG;
		gettype(x, &p->in.type, &p->in.tattrib);
		p->in.right = *--fsp;
		p->in.left = *--fsp;
#if 0
		p->in.type = STRTY + PTR;
#endif
		p->stn.stsize = rdlong();
		goto bump;

	    case STARG:
		p = getnode();
		p->in.op = STARG;
		gettype(x, &p->in.type, &p->in.tattrib);
		p->stn.stalign = VAL(x);
		p->stn.stsize = rdlong();
		p->tn.rval = 0;
		p->in.left = *--fsp;
		goto bump;

	    case NAME:
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
		if (c1nodesseen.c1word)	/* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
	        goto bump;

	    case ICON:
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

		if (c1nodesseen.c1word)	 /* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }

bump:
	        *fsp++ = p;		/* remember node in postfix stack */
	        if( fsp >= &fstack[fmaxstack] )  /* stack is full */
		    {
		    NODE **fsptemp = fsp;
		    reallocfstack(&fsptemp);  /* reallocate expression stack */
		    fsp = fsptemp;
		    }
    	        break;

	    case GOTO:
	        if( VAL(x) )
		    {
		    /* unconditional branch */
		    if ((!inthunk) || currpd->hasasgnfmts)
		      {
		      putlong(x);
		      putlong(rdlong());
		      }
		    else
		      rdlong();
		    break;
		    }
	        /* otherwise, treat as unary */
	        goto def;

	    case SETREGS:
	        /* reserve/release certain registers as regvars used in pass1 */
		callerSetregsMask = x;
		callerSetregsFDMask = rdlong();
		putlong(callerSetregsMask);
		putlong(callerSetregsFDMask);
	        break;

	    case REG:
	        p = getnode();	/* store leaf REG node for tree */
	        p->in.op = REG;
	        gettype(x, &p->in.type, &p->in.tattrib);
	        p->tn.rval = VAL(x);
	        p->tn.lval = 0;
	        p->in.name = NULL;
		if (c1nodesseen.c1word)  /* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
	        goto bump;

	    case OREG:
	        p = getnode();	/* store leaf OREG node for tree */
	        p->in.op = OREG;
	        gettype(x, &p->in.type, &p->in.tattrib);
	        p->tn.rval = VAL(x);
	        p->tn.lval = rdlong();
	        p->in.name = NULL;
		if (c1nodesseen.c1word)
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
	        goto bump;

	    case FTEXT:		/* literal data, copy unchanged, unless c0
				   comment or bss/data area */
	        strwords = VAL(x);
	        rdnlongs( strwords );
	        strbuf[strwords*4] = '\0';

	        if (!strncmp(strbuf, "#c0 ", 4))  /* c0 comment */
		    {

		    switch (strbuf[4]) {

		    case 's':		/* subroutine/function defn */
			/* no registers used yet -- clear mask */
			callerSetregsMask = SETREGS | CLEARSETREGS;
			callerSetregsFDMask = CLEARFDSETREGS;
			callerC1opts = C1OPTIONS | (CLEARC1OPTS<<16)
					| (CLEARC1OPTLEV<<8);
			break;

		    /* the following supplied info to pass1 -- ignored here */
		    case 'r':		/* return value offset */
		    case 'd':		/* adjustable dimension bytes */
		    case 'n':		/* do not emit as standalone */
					/* don't emit */
		        break;

		    case 'f':		/* force routines inline */
			doforcelist(strbuf+5,NO);	/* update list */
			break;				/* don't emit */

		    case 'o':		/* force routines not inline */
			doomitlist(strbuf+5,NO);	/* update list */
			break;				/* don't emit */

		    case 'D':		/* reset force/omit defaults */
			dodefaultlist(strbuf+5);	/* update list */
			break;				/* don't emit */

		    case 't':		/* variable expression FORMAT label */
			if (strbuf[5] == 'r')		/* call to thunk */
			    isthunkcall = YES;
			else if (strbuf[5] == 'l')	/* thunk definition*/
			    inthunk = YES;
			/* else ASSIGN'd FORMAT flag -- ignore */
			break;

		    case 'c':		/* call info */
		        {
		        NODE *nodep;  	/* pointer to ICON/NAME node */
			NODE *nodep2;
			register TWORD *ps;
			unsigned short nargs;		/* # args to proc */
			TWORD rettype;			/* return type */

			if (isthunkcall)		/* call to thunk? */
			    break;			/* handle another way */

		        nodep = *(fsp-1);		/* CALL node */
		        nodep = nodep->in.left;		/* ICON/NAME node */

			if (nodep->in.op != ICON)	/* dummy proc arg */
			    break;			/* can't inline */

			/* find pd for called procedure */
			ippd = pdfromname(nodep->in.name, nodep->tn.rval);
			sscanf(strbuf+19,"%8x",&lineno);   /* set lineno for errors */

			/* check if OK to inline this call */
			if (ippd			/* there is source */
			 && ippd->inlineOK		/* pass1 checks OK */
			 && (isforced(ippd->pnpoffset)	/* forced inline */
			  || (ippd->nlines <= dlines))	/* lines < threshold */
			 && !isomitted(ippd->pnpoffset)	/* not omitted */
			 && (ippd->integorder < currpd->integorder)
						/* already in output file --
						 * indirect recursion check */
			 && !INCOMPAT_C1OPTS(ippd->c1opts,currpd->c1opts)
			 && !inthunk)		/* not a call to thunk */
						/* ok from source side */
			    {
			    register char *i,*j;
			    long rettypeonly;	/* return type, no CHAR leng */

			    /* check on call args, return type */

			    sscanf(strbuf+14,"%4hx",&nargs);	/* # args */
			    sscanf(strbuf+5,"%8x",&rettype);

			    /* mask off CHARACTER length, if any */
			    rettypeonly = rettype & 0xff;

			    if (nargs > MAXARGS)	/* too many arguments */
				{
				/* print message only if forced inline */
				if (isforced(ippd->pnpoffset))
				    {
				    sprintf(warnbuff,
"More than %d arguments in call to \"%s\"\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
				        MAXARGS,
				        ((pNameRec)(pnp+(ippd->pnpoffset)))->name,
				        filename,
				        lineno,
				        ((pNameRec)(pnp+(currpd->pnpoffset)))->name);
				    warn(warnbuff);
				    }
				break;
				}
			    for (i=strbuf+28,j=strbuf+28+nargs*9,ps=argtypes;
				 i < j; i+=9,ps++)
				{
				sscanf(i,"%8x",ps);
				}
			    }
			else
			    {
			    long pnpoffset = offsetfromname(nodep->in.name,
							    nodep->tn.rval);
			    /* explain why not inlining this call (if forced) */
			    explainNoInline(ippd,pnpoffset,filename,lineno,
					    currpd,inthunk);
			    break;	/* forget this!!! */
			    }

			/* We will integrate call -- store info in array */
			if (nxtcall >= MAXEXPRCALLS)	/* too many calls in tree */
			    break;		/* don't inline this call */
			call[nxtcall].pd = ippd;

			/* store info in special C0 node above actual CALL node */
			call[nxtcall].nptr = nodep = getnode();
			nodep->cn.op = FC0CALL;
			nodep->cn.nargs = nargs;
			nodep->cn.rettype = rettype;

			/* copy argument types to special nodes (4 per node) */
			ps = argtypes;
			while (nargs > C0_TYPES_PER_NDU)
			    {
			    memcpy(nodep->cn.argtype,ps,(C0_TYPES_PER_NDU*sizeof(TWORD)));
			    ps+=C0_TYPES_PER_NDU;
			    nargs-=C0_TYPES_PER_NDU;
			    nodep->cn.right = nodep2 = getnode();
			    nodep2->cn.op = FC0CALLC;
			    nodep = nodep2;
			    }
			if (nargs > 0)
			    {
			    memcpy(nodep->cn.argtype,ps,nargs*sizeof(TWORD));
			    }

			/* stick on expression stack */
			p = call[nxtcall].nptr;
			p->cn.left = *--fsp;	/* must be CALL or UCALL */
			nxtcall++;
			goto bump;
		        }
nogood:
		        break;

		    case 'l':		/* max inlineable proc size */
			/* update current environment */
			sscanf(strbuf+5,"%d",&dlines);
			break;

		    case 'a':		/* assign statement follows */
		    case 'p':		/* prolog end */
		    case 'R':		/* alternate return flag */
			goto emitftext;

		    case 'q':		/* C return stmnt */
			altreturn = YES;
			goto emitftext;

		    }  /* switch */
		    }
		else if (!strncmp(strbuf, "\ttext", 5))
		    {
		    inbss = NO;
		    }
		else if (!strncmp(strbuf, "\tbss", 4))
		    {
		    inbss = YES;
		    }
		else if (!strncmp(strbuf, "\tdata", 5))
		    {
		    inbss = YES;
		    }
		else	/* not a c0 comment */
		    {
		    if (inthunk)	/* in thunk definition -- get label */
			{
			if (!ftnflag) fatal("Illegal thunk in C program");
			if (*strbuf == '_')	/* thunk label definition */
			    /* if the procedure has ASSIGN'd FORMAT usage, we
			     * must emit the thunk because the thunk address
			     * is assigned to an integer variable (somewhere)
			     * and the thunk is called indirectly.  Obviously,
			     * we can't inline indirect calls.
			     */
			    if (currpd->hasasgnfmts)	/* ASSIGN'd FORMATs */
				goto emitftext;		/* do emit thunk */
			    else			/* no ASSIGN'd FORMATs */
				{
				char *ps = strbuf + 1;

				/* get thunk name and emit "set" instruction
				 * to assembly file.  Because the thunk is
				 * referenced in the FORMAT structure, we need
				 * to define the name (as a 0)
				 */
				while (*ps++ != ':')
				    ;
				*--ps = '\0';
				fprintf(oasfd,"\tset\t%s,0\n", strbuf);
				break;		/* don't emit label */
				}
			}
emitftext:					      /* echo text to output */
		    if ((!inbss) && (altreturn != YES))
			{
			putlong(x);
			putnlongs(strwords);
			}
		    }
	        break;

	    case FEXPR:		/* expression root node, process tree */
	        lineno = REST(x);		/* source line number */
	        if( strwords = (VAL(x)) )	/* a file name !!! */
		    {
		    rdnlongs( strwords );
		    strbuf[strwords*4] = '\0';	/* guarantee a NULL */
		    if (strbuf[0] == '"')
			{
			strcpy(filename,&strbuf[1]);
			filename[strlen(filename)-1] = '\0';
			}
		    else
			{
		    	strcpy(filename, strbuf);
			}
		    }
	        if( fsp == fstack )   /* filename only -- emit unchanged */
		    {
		    			/* nothing emitted from thunks */
		    if ((!inthunk) || currpd->hasasgnfmts)
			{
			putlong(x);
			if (strwords)
		            putnlongs(strwords);
			}
		    break;
		    }

	        if( --fsp != fstack )
		    fatal( "expression poorly formed (pass2)" );

	        p = fstack[0];	/* root of tree */

	        if (nxtcall > 0) 	/* replaceable calls */
		    {
		    long emittree;
		    emittree = inlinecalls(p,filename,lineno,currpd);
						/* inline calls and emit */
		    if (emittree)	/* original tree should be emitted */
			goto emit;		/* emit FEXPR node */
		    }
		else if (isthunkcall)	/* var expr fmt "thunk" call */
		    {
		    long thunkoffset;	/* pnp offset of thunk name */
		    NODE *th;		/* thunk name ICON node */
		    pTNameRec thunkrec; /* thunk pnp name record */

		    isthunkcall = 0;	/* reset for next time */
		    th = p->in.left;	/* UCALL -> ICON */

		    /* find pnp record for thunk name */
		    thunkoffset = offsetfromname(th->in.name,th->tn.rval);
		    thunkrec = ((pTNameRec)(pnp + thunkoffset));

		    if (! thunkrec->isempty)	/* something to inline */
		        inlinethunk(thunkrec->fileoffset);  /* inline it */
		    /* else do nothing */
		    /* don't emit original call */
		    }
		else  /* not thunk call and nothing to inline */
		    {
		    /* copy tree unchanged unless it's in a thunk (but we
		     * do emit thunks if there is ASSIGN'd FORMAT usage)
		     */
		    if ((!inthunk) || currpd->hasasgnfmts)
			{
emit:
		        puttree(p,altreturn);		/* emit tree */
			
			/* emit FEXPR node */
		        putlong(x);
		        if (strwords)
			    {
			    strcpy(strbuf,filename);
			    putnlongs(strwords);
			    }
			}
		    }

		nxtcall = 0;		/* reset for next expression tree */
	        freetree(p);		/* recover all tree/namepool space */
		altreturn = 0;
	        break;

	    case FSWITCH:
	        fatal( "switch not yet done" );
	        break;
	
	    case FCMGO:			/* copy COMPUTED GOTO unchanged */
		putlong(x);
		for (i=REST(x); i > 0; i--)
		    putlong(rdlong());
		break;

	    case SWTCH:
		putlong(x);
	        for (i = REST(x); i > 0; i--)
		    {
	            putlong(rdlong());
		    putlong(rdlong());
		    }
	        break;

	    case FLBRAC:		/* beginning of procedure */
	        putlong(x);
	        putlong(rdlong());
	        putlong(rdlong());
		putlong(rdlong());

	        /* record output location -- if we inline any calls in this
		 * proc, we'll need to fix up the local stack use later.
		 */
	        fflush(ofd);
	        currpd->outoffset = ftell(ofd) - 16;
	        maxstack = currpd->stackuse >> 3;  /* bits -> bytes */
		strwords = rdstring();
		putstring(strbuf);
	        break;

	    case FRBRAC:		/* end of procedure */
		{
	        long curroffset;	/* current output file offset */

	        putlong(x);
		fflush(ofd);
		curroffset = ftell(ofd);

	        /* adjust local stack size increase -- due to inlining */
	        if (maxstack > (currpd->stackuse >> 3))	  /* bits -> bytes */
		    {
		    currpd->stackuse = maxstack << 3;	  /* bytes -> bits */
		    fflush(ofd);
		    fseek(ofd,currpd->outoffset+8,0);
		    putlong(currpd->stackuse);
		    putlong(currpd->stackuse);
		    fflush(ofd);
		    fseek(ofd,curroffset,0);
		    }

		/* remember end of function location */
		currpd->outroffset = curroffset - 4;

		/* update labels array with any added labels */
		if (nxtfreelab > currpd->nlab)
		    {
		    if ((currpd->labels =
				(long*)realloc(currpd->labels,
				               nxtfreelab*sizeof(long)*2))
			== NULL)
			fatal("malloc failed for FRBRAC -- pass 2");
		    memcpy(currpd->labels,labarray,nxtfreelab*4);
		    currpd->nlab = nxtfreelab;
		    }
		}
		goto endproc;

	    case FEOF:		/* end of file */
	        fatal("FEOF encountered in pass2()");


	    case LABEL:		/* label */
	    case FICONLAB:	/* ICON contain label value due to ASSIGN */
		if ((!inthunk) || currpd->hasasgnfmts)
	        	putlong(x);
	        break;

	    case MINUS:
	    case PLUS:
	        p = getnode();	/* save interior node of tree */
	        p->in.op = FOP(x);
	        gettype(x, &p->in.type, &p->in.tattrib);
		p->in.right = *--fsp;	/* top on postfix stack */
		p->in.left = *--fsp;	/* next on postfix stack */
		if (c1nodesseen.c1word) /* C1 data for this node ? */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
		goto bump;		/* store this node on stack */

	    case UNARY MUL:
	        p = getnode();	/* save interior node of tree */
	        p->in.op = FOP(x);
	        gettype(x, &p->in.type, &p->in.tattrib);
		p->in.left = *--fsp;	/* top on postfix stack */
		p->tn.rval = 0;
		if (c1nodesseen.c1word)	/* C1 data for this node */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
		goto bump;		/* store this node on stack */
    
	    case C1SYMTAB:
		putlong(x);
	        if (VAL(x) == 1)
	          {
	          farg_high = rdlong();
	          farg_low = rdlong();
	          farg_pos = rdlong();
		  putlong(farg_high);
		  putlong(farg_low);
		  putlong(farg_pos);
	          }
	        break;

	    case NOEFFECTS:	/* Copied in pass1.c, toss it */
		rdstring();
		break;

	    case C1NAME:  /* Throw away in C, copy in FTN */
		p = &localnode;
		p->tn.op = NAME;
		gettype(x, &p->in.type, &p->in.tattrib);
		if (ftnflag)
			puttype(FOP(x), VAL(x), p->in.type, p->in.tattrib);
	        if (VAL(x))
			{
			x = rdlong();
			if (ftnflag)
				putlong(x);
			}
	        strwords = rdstring();
		x = rdlong();
		if (ftnflag)
			{
			putstring(strbuf);
			putlong(x);
			}
		if ( x & C1ARY )
			{
			x = rdlong();
			if (ftnflag)
				putlong(x);
			}
	        strwords = rdstring();
		if (ftnflag) putstring(strbuf);
	        break;

	    case C1OREG:
		p = &localnode;
		p->tn.op = OREG;
		gettype(x, &p->in.type, &p->in.tattrib);
		puttype(FOP(x), VAL(x), p->in.type, p->in.tattrib);
		putlong(rdlong());
		x = rdlong();
		putlong(x);
		if ( x & C1ARY )
		  putlong( rdlong() );
	        strwords = rdstring();
		putstring(strbuf);
	        break;

	    case C1OPTIONS:
		callerC1opts = x;
		putlong(x);
		break;

	    case VAREXPRFMTREF:
		if ( ! isthunkcall )
		  putlong(x);
		break;

	    case VAREXPRFMTDEF:
	    	if (currpd->hasasgnfmts)	/* ASSIGN'd FORMATs */
		  putlong(x);
		break;

	    case VAREXPRFMTEND:
	        inthunk = 0;
	    	if (currpd->hasasgnfmts)	/* ASSIGN'd FORMATs */
		  putlong(x);			/* need to emit thunk */
		break;

	    case C1HIDDENVARS:
	        {
	        int i,j,r,v;
		HVREC *t;
    
	        j = REST(x);
	        for (i=0; i < j; ++i)
	          {
	          x = rdlong();
		  v = VAL(x);
		  r = REST(x);
	          if (FOP(x) == C1HVOREG)
		    {
		    x = rdlong();
		    t = gethvrec(C1HVOREG, x, r, v, NULL); /* pass unchanged */
		    }
	          else if (FOP(x) == C1HVNAME)
		    {
		    x = rdlong();
		    strwords = rdstring();
		    t = gethvrec(C1HVNAME, x, r, v, strbuf); /*pass unchanged */
		    }
	          else
		    fatal("unexpected opcode in C1HIDDENVARS");
		  if ( hvlist == NULL )
		    {
		    hvlist = t;
		    hvlist->cnt = j;
		    }
		  else
		    hvtail->next = t;
		  hvtail = t;
	          }
	        }
	        break;

	    case CALL:
	    case UNARY CALL:
	        p = getnode();
	        p->in.op = FOP(x);
	        gettype(x, &p->in.type, &p->in.tattrib);
		p->in.rall  = VAL(x);
	        p->in.c1data.c1word = 0;
		p->in.name = (char *) hvlist;
		hvlist = NULL;
		if ( FOP(x) == CALL )
	          p->in.right = *--fsp;
	        p->in.left = *--fsp;
		c1nodesseen.c1word = 0;
	        goto bump;

	    case STCALL:
	    case UNARY STCALL:
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
		c1nodesseen.c1word = 0;
		goto bump;

	    case FLD:
		p = getnode();
		p->tn.op = FOP(x);
		gettype(x, &p->in.type, &p->in.tattrib);
		p->tn.rval = rdlong();
		p->in.left = *--fsp;
		if (c1nodesseen.c1word) /* C1 data for this node ? */
		    {
		    p->in.c1data.c1word = c1nodesseen.c1word;
		    if (c1nodesseen.c1flags.isarray)
			p->in.arraybase = arrayoffset;
		    else if (c1nodesseen.c1flags.isstruct)
			p->in.arraybase = structoffset;
		    c1nodesseen.c1word = 0;
		    }
		goto bump;

	    default:		/* all interior (operator) OPs */
def:
	        p = getnode();	/* save interior node of tree */
	        p->in.op = FOP(x);
	        gettype(x, &p->in.type, &p->in.tattrib);

	        switch( optype( p->in.op ) ){

	        case BITYPE:		/* binary operator -- set children */
		    p->in.right = *--fsp;	/* top on postfix stack */
		    p->in.left = *--fsp;	/* next on postfix stack */
		    c1nodesseen.c1word = 0;
		    goto bump;		/* store this node on stack */

	        case UTYPE:			/* unary operator -- set children */
		    p->in.left = *--fsp;	/* top on postfix stack */
		    p->tn.rval = 0;
		    c1nodesseen.c1word = 0;
		    goto bump;		/* store this node on stack */

	        case LTYPE:			/* leaf node */
		    sprintf(warnbuff,"illegal leaf node (pass2): %d", p->in.op );
		    fatal(warnbuff);
	        }
	    }
        }
endproc:
    continue;
    }	/* for (currpd..... */

    if (ofd == otmpfd)
	{
	fflush(ofd);
	ofd = opermfd;
	}
    puttriple(FEOF,0,0);
    fflush(ofd);

    /* update MAXLABEL record at very beginning.  Contains the maximum
     * generated label before f1.
     */
    fseek(ofd,4,0);
    putlong(nxtlabel);
    fflush(ofd);
    fseek(ofd,0,2);	/* seek to end */
    fclose(ofd);
    if (ftnflag)
	{
	fflush(oasfd);
	fclose(oasfd);
	}
}

/*****************************************************************************/

/* initiptree -- initialize expression tree storage (callee space) */
initiptree()
{
    register NODE *p, *end;

    if ((ipnode = (NODE *) malloc ( maxipnodesz * sizeof (NODE) + sizeof(NODE**) ))
	== NULL)
	fatal ("Out of memory in initiptree()");
    *((NODE**)ipnode) = (NODE*) 0;	/* no previous node tables */
    ((NODE**)ipnode)++;
    ipmaxnode = &(ipnode[maxipnodesz-1]);	/* last node is the end */
    ipfreenode = ipnode;		/* free nodes start at beginning */
}

/*****************************************************************************/

/* allocate next free tree node (callee space) */
NODE* getipnode()
{
    if (ipfreenode <= ipmaxnode)	/* there are more free nodes */
	{
#ifdef C1
	ipfreenode->in.c1data.c1word = 0;	/* no C1 info (yet) */
#endif
	ipfreenode->in.left = NULL;
	ipfreenode->in.right = NULL;
	return(ipfreenode++);
	}
    else				/* no more free nodes -- get more */
	{
	NODE * oldipnode = ipnode;

	/* Allocate secondary table.  First field is pointer to original
	 * table.
	 */
        if ((ipnode = (NODE *) malloc ( maxipnodesz * sizeof (NODE) + sizeof(NODE**) ))
	    == NULL)
	    fatal ("Out of memory in getipnode()");
        *((NODE**)ipnode) = oldipnode;	/* store addr of previous */
        ((NODE**)ipnode)++;		/* table starts after back pointer */
        ipfreenode = ipnode;		/* free nodes at start */
        ipmaxnode = &(node[maxipnodesz-1]);	/* last node is end */
	ipnoderealloctimes++;		/* remember that we've alloc'd */
#ifdef C1
	ipfreenode->in.c1data.c1word = 0;
#endif
	ipfreenode->in.left = NULL;
	ipfreenode->in.right = NULL;
	return(ipfreenode++);
	}
}

/*****************************************************************************/

/* free expression tree storage (caller space) -- called after every callee
 * expression tree has been processed
 */
freeiptree()
{
    NODE * oldipnode;		/* previous node table, if any */
    register long inx;

    freeipnp = ipnp;		/* free all callee name pool space (ipnp) */
    if (ipnoderealloctimes)	/* more than one node array around -- collapse*/
	{
	for (inx = ipnoderealloctimes; inx > 0; inx--)
	    {
	    oldipnode = *(--((NODE**)ipnode));	/* get addr of previous table */
	    free(ipnode);			/* free current table */
	    ipnode = oldipnode;
	    }
	maxipnodesz *= ipnoderealloctimes;    /* allocate single large table */
        if ((ipnode = (NODE *) malloc ( maxipnodesz * sizeof (NODE)
					+ sizeof(NODE**) ))
	    == NULL)
	    fatal ("Out of memory in freeiptree()");
        *((NODE**)ipnode) = (NODE*) 0;	/* no previous node tables */
        ((NODE**)ipnode)++;
        ipmaxnode = &(ipnode[maxipnodesz-1]);
	ipnoderealloctimes = 0;
	}
    ipfreenode = ipnode;
}

/*****************************************************************************/

/* allocate callee expression stack */
initipstack()
{
    ipmaxstack = NSTACKSZ;
    if ((ipstack = (NODE **) malloc( ipmaxstack * sizeof(NODE*))) == NULL)
	fatal("Out of memory in initipstack()");
}

/*****************************************************************************/

/* reallocate callee expression stack */
reallocipstack(pipsp)
NODE ***pipsp;		/* pointer to current top-of-stack pointer */
{
    long ipspoffset;	/* relative offset of top-of-stack */

    ipspoffset = *pipsp - ipstack;
    ipmaxstack += NSTACKSZ;
    if (( ipstack = (NODE**) realloc(ipstack,ipmaxstack*sizeof(NODE*))) == NULL)
	fatal("Out of memory in reallocipstack()");
    *pipsp = ipstack + ipspoffset;	/* reset absolute top-of-stack pointer*/
}

/*****************************************************************************/

/* write a 32-bit long to the output file */
putlong(x)
long x;
{
    fwrite(&x, 4, 1, ofd);
}

/*****************************************************************************/

/* write "n" 32-bit longs to output file */
putnlongs(x)
register long x;
{
    register long *p = (long *) strbuf;
    for (;x;x--,p++)
    fwrite(p, 4, 1, ofd);
}

/*****************************************************************************/

/* write an FTEXT entry to output file */
puttext(s)
char *s;
{
    long strleng, leng;
    strleng = strlen(s) + 1;
    leng = strleng / 4 + ((strleng % 4) != 0);
    puttriple(FTEXT, 0, leng);
    putstring(s);
}

/*****************************************************************************/

/* write a string to output file, followed by NULL, total output is
 * multiple of 32 bits
 */
putstring(s)
char *s;
{
    register long strleng;
    long leftover;
    union {
	char ch[4];
	long i;
	} un;
    register char *p;
    register char *q;
    long inx;

  if ( s == NULL )
    putlong(0);
  else if ( *s == '\0' )
    putlong(0);
  else
    {
    strleng = strlen(s) + 1;
    leftover = strleng % 4;
    strleng -= leftover;
    fwrite(s, 1, strleng, ofd);
    if (leftover)
	{
	un.i = 0;
	for (p=s+strleng, q= &(un.ch[0]); *p; )
	    *q++ = *p++;
        fwrite(&(un.i), 4, 1, ofd);
	}
    }
}

/*****************************************************************************/

/* emit an OP code triple */
puttriple(op, type, rval)
long op, type, rval;
{
    long x;
    x = (type << 16) | (rval << 8) | op;
    fwrite(&x, 4, 1, ofd);
}

/*****************************************************************************/

/* write an expression tree to the output file */
puttree(p,forcereturn)
NODE *p;
int forcereturn;
{
	register short opty;	/* arity of opcode */

	if (p->in.op == FC0CALL)	/* special C0 node -- don't emit */
	    p = p->in.left;

	opty = optype(p->in.op);

	if( opty != LTYPE ) puttree( p->in.left,0);
	if( opty == BITYPE ) puttree( p->in.right,0);

	if (forcereturn == YES)
		{
		if (p->in.op != FORCE) fatal("Unexpected FORCE comment");
		puttext("#c0 q");
		}

	switch( p->in.op ){  /* switch on opcode */

	case STASG:		 /* NODE *newnod; */
		puttriple(STASG, 0, 0 );
		putlong(p->stn.stsize);
		break;

	case STARG:
		puttype(p->in.op, p->stn.stalign, p->in.type, p->in.tattrib);
                putlong(p->stn.stsize);
		break;

	case NAME:
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}

	    puttype(p->in.op, (p->tn.lval != 0), p->in.type, p->in.tattrib);

	    if (p->tn.lval)
		putlong(p->tn.lval);
	    putstring(p->in.name);
	    break;

	case ICON:
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}
	    puttype(p->in.op, (p->in.name != 0), p->in.type, p->in.tattrib);
	    putlong(p->tn.lval);
	    if (p->in.name)
		putstring(p->in.name);
    	    break;

	case REG:
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}
	    puttype(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
	    break;

	case OREG:
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}
	    puttype(p->in.op, p->tn.rval, p->in.type, p->in.tattrib);
	    putlong(p->tn.lval);
	    break;

	case PLUS:
	case UNARY MUL:
	case MINUS:
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}
	    puttype(p->in.op, 0, p->in.type, p->in.tattrib);
	    break;

	case FC0OREG:		/* special OREG node -- emit as regular OREG */
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}
	    puttype(OREG, p->tn.rval, p->in.type, p->in.tattrib);
	    putlong(p->tn.lval);
	    break;

	case FLD:
	    if (p->in.c1data.c1word)	/* C1 data -- emit C1 node records */
		{
		if (p->in.c1data.c1flags.isarray)
		    {
		    puttriple(ARRAYREF,0,p->in.c1data.c1flags.iselement);
		    putlong(p->in.arraybase);
		    }
		else if (p->in.c1data.c1flags.isstruct)
		    { int t;
		    puttriple(STRUCTREF,0,
				(t=p->in.c1data.c1flags.isprimaryref));
		    if (t)
		      putlong(p->in.arraybase);
		    }
		}
	    puttype(FLD,0,p->in.type,p->in.tattrib);
	    putlong(p->tn.rval);
	    break;

	case STCALL:
	case UNARY STCALL:
	    puttype(p->in.op,p->stn.stalign,p->in.type,p->in.tattrib);
	    putlong(p->stn.stsize);
	    break;

	case CALL:
	case UNARY CALL:
	    {
	    HVREC *h = (HVREC *) p->in.name;
	    HVREC *t;
	    int n;
	    if ( h != NULL )	/* c1 hidden vars info */
	      {
	      puttriple( C1HIDDENVARS, h->cnt, 0 );
	      while ( h != NULL )
		{
		puttriple( h->op, h->rest, h->val );
		putlong( h->off );
		if ( h->op == C1HVNAME )
		  {
		  if ( h->str != NULL )
	            {
	            strcpy(strbuf,h->str);
	            n = strlen(strbuf);
		    free( h->str );
	            }
	          else
	            {
	            n = 0;
	            strbuf[0] = '\0';
	            }
	          strbuf[n+1] = '\0';
	          strbuf[n+2] = '\0';
	          strbuf[n+3] = '\0';
	          putstring(strbuf);
		  }
		t = h;
		h = h->next;
		free( t );
		}
	      }
	    p->in.name = NULL;
	    puttype(p->in.op, p->tn.rall, p->in.type, p->in.tattrib);
	    }
	    break;

	default:
	    puttype(p->in.op, 0, p->in.type, p->in.tattrib);
	    break;
	}
}

/*****************************************************************************/

/* pdfromname -- hash name "s" and find corresponding pd entry.
 * Return pointer to it if found, else return a NULL pointer.
 */
pPD pdfromname(s,nwords)
char *s;	/* name of proc */
long nwords;	/* number of 4-byte words in name */
{
long offset;	/* offset of name record when all done */
pNameRec np;

    if ((offset = offsetfromname(s,nwords)) == 0)
	return(0);
    else
	{
	np = (pNameRec) (pnp + offset);
	return (np->pd);
	}
}  /* pdfromname */

/*****************************************************************************/

/* offsetfromname -- hash name "s" and find corresponding pnp NameRec entry
 * Return offset to it if found, else return zero.
 */
long offsetfromname(s,nwords)
char *s;
long nwords;
{
long offset;	/* offset of name record when all done */
register long i;
register unsigned long hashval;
register pNameRec np;

    /* make sure only nulls after first null in name */
    {
    register char *p;
    register char *pend;

    pend = s + nwords*4;
    for (p = pend - 4; *p; p++)
	;
    for (p++; p < pend; p++)
	*p = '\0';
    }

    /* hash name */
    {
    register unsigned long *p = (unsigned long *) s;
    hashval = 0;
    for (i = nwords; i > 0; p++, i--)
	hashval += *p;
    hashval %= HASHSIZE;
    }

    /* search chain for matching procedure name */
    offset = hash[hashval];
    while (offset)
	{
	np = (pNameRec) (pnp + offset);
	if ((np->nlongs == nwords) && !memcmp(np->name,s,nwords*4))
	    /* found */
	    return (offset);
	else
	    /* check next in chain */
	    offset = np->onext;
	}
    return(0);  /* procedure not found */
}

/*****************************************************************************/

/* Explain why not inlining a call */
explainNoInline(ippd,pnpoffset,filename,lineno,currpd,inthunk)
pPD ippd;	/* target procedure */
long pnpoffset; /* pnp offset of target procedure */
char *filename; /* current filename */
long lineno;    /* current line number */
pPD currpd;	/* calling procedure */
long inthunk;	/* is this a call from within a thunk ? */
{
    if (!ippd)	/* no source for this procedure */
	{
	if (isforced(pnpoffset))
	    {
	    sprintf(warnbuff,
"PROC \"%s\" -- source not found\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
	    warn(warnbuff);
	    }
#ifdef DEBUGGING
	else if (verboseflag)
	    {
	    sprintf(warnbuff,
"PROC \"%s\" -- source not found (no force)\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
	    warn(warnbuff);
	    }
#endif /* DEBUGGING */
	}
    else if (isforced(pnpoffset))
	{
	if (!ippd->inlineOK)
	    {
	    if (ippd->typeproc & 2)	/* is ENTRY */
		{
		sprintf(warnbuff,
"FORCED inline of ENTRY \"%s\" at\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (ippd->next
		  && ((ippd->next)->typeproc & 2))
			/* contains ENTRY */
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- contains ENTRY\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (!valid_return_type(ippd->rettype))
		{
		if (ftnflag)
			sprintf(warnbuff, "FORCED inline of CHARACTER or COMPLEX FUNCTION \"%s\" at\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
			((pNameRec)(pnp+pnpoffset))->name,
			filename,
			lineno,
			((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		else
			sprintf(warnbuff, "FORCED inline of STRUCTURE or LONG DOUBLE FUNCTION \"%s\" at\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
			((pNameRec)(pnp+pnpoffset))->name,
			filename,
			lineno,
			((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (ippd->nparms > MAXARGS)
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- too many formal parameters\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (ippd->hasCharOrCmplxParms)
		{
		if (ftnflag)
			sprintf(warnbuff,
			"FORCED inline of PROC \"%s\" -- CHARACTER or COMPLEX formal parameters\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
			((pNameRec)(pnp+pnpoffset))->name,
			filename,
			lineno,
			((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		else
			sprintf(warnbuff,
			"FORCED inline of PROC \"%s\" -- STRUCTURE or LONG DOUBLE formal parameters\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
			((pNameRec)(pnp+pnpoffset))->name,
			filename,
			lineno,
			((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (ippd->hasasgnfmts)
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- has ASSIGN'd FORMAT usage\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (ippd->imbeddedAsm)
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- has imbedded assembly code\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else
		{
		sprintf(warnbuff, "FORCED inline of PROC \"%s\" -- not inlined",
			((pNameRec)(pnp+pnpoffset))->name,
			filename,
			lineno,
			((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    }
	else  /* good procedure to inline, but can't under circumstances */
	    {
	    if (inthunk)	/* procedure call from within thunk */
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- in variable expression FORMAT\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if (ippd->integorder >= currpd->integorder)
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- in indirect recursion cycle\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else if ( INCOMPAT_C1OPTS( ippd->c1opts, currpd->c1opts ) )
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" -- incompatible optimization option settings\n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    else
		{
		sprintf(warnbuff,
"FORCED inline of PROC \"%s\" --- not inlined",
			((pNameRec)(pnp+pnpoffset))->name,
			filename,
			lineno,
			((pNameRec)(pnp+(currpd->pnpoffset)))->name);
		warn(warnbuff);
		}
	    }
	}
#ifdef DEBUGGING
    else if (verboseflag)
	    {
	    sprintf(warnbuff,
"PROC \"%s\" -- no force - no explanation \n\tFILE \"%s\", LINE %d, PROC \"%s\" -- not inlined",
		((pNameRec)(pnp+pnpoffset))->name,
		filename,
		lineno,
		((pNameRec)(pnp+(currpd->pnpoffset)))->name);
	    warn(warnbuff);
	    }
#endif /* DEBUGGING */
}
