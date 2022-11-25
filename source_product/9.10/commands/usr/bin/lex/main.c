/* @(#) $Revision: 70.9 $ */     
# include "ldefs.c"
# include "msgs.h"
# include "once.c"

#ifdef NLS16
# include <nl_types.h>
# include <langinfo.h>
#endif

	/* lex [-[drcyvntfwm]] [file] ... [file] */

	/* Copyright 1976, Bell Telephone Laboratories, Inc.,
	    written by Eric Schmidt, August 27, 1976   */

# define MINTABSZ 40

/* NOTES ON %x state implementation.  KAH  Sept. 1990 
 * %x start states were implemented to follow the POSIX draft 10.

 * The implementation required restructuring the grammar's recognition of
 * start conditions, so that patterns with no start condition can be clearly
 * identified.

 * In the pre-%x lex implementation, a pattern with a start condition has a
 * node RSCON, but there is no special marking for patterns without a start
 * condition.  The important code is in procedures cgoto() and first().  In
 * calculating first(), the RSCON nodes drive the check of whether the
 * pattern is valid in the given start state.  Case RNEWE causes all
 * patterns to get scanned.

 * To handle %x, we need to be able to recognize that a pattern had
 * no start condition, and so if the current state is a %x state
 * do not factor it in.  This could potentially be done at the
 * RNEWE case, by scanning down the tree to see if there are start
 * conditions.  But it better fits with the tree walk structure
 * of first() to instead define a new node type to head patterns
 * with no start condition.

 * A new node type RNOSCON heads up patterns with no start condition.
 * In first(), when the pattern is RNOSCON, collect first info if the
 * state is type %s but do not if the state is type %x.

 * Appropriate code has to be added in all the places where tree nodes
 * are manipulated.  Basically, look for case RSCON now.  RNOSCON will
 * be a unary, rather than a binary node.  The grammar is modified
 * to recognize whether the patterns had any start conditions.  A new
 * array stype[] is added to keep track of the type of each start
 * condition.  It parallels sname[].

 * Here as a side note, the grammar rules that handle start conditions
 * are right recursive, which is inefficient for a LALR parser.  If
 * I was doing major rework to lex (or starting a design), I would
 * change this to left recursion.  At this point, however, to change
 * the recursion would require defining a different shape to the
 * trees for start conditions, and would require changes throughout
 * the tree handling routines.  So in the grammar changes, I left it
 * so that it was still right recursive, and would build the same
 * shape pattern trees as before.

 * The files modified were:
 *	ldefs.c  main.c  once.c  parser.y  sub1.c  sub2.c
 */

main(argc,argv)
  int argc;
  char **argv; {
	register int i;
        char fname[20];
# ifdef DEBUG
	signal(10,buserr);
	signal(11,segviol);
# endif

	setlocale(LC_ALL,"");
        open_err_cat();

	while (argc > 1 && argv[1][0] == '-' ){
		i = 0;
		while(argv[1][++i]){
			switch (argv[1][i]){
# ifdef DEBUG
				case 'd': debug++; break;
				case 'y': yydebug = TRUE; break;
# endif
				case 'r': case 'R':
					warning(NORATFOR);
					ratfor=TRUE; break;
				case 'c': case 'C':
					ratfor=FALSE; break;
				case 't': case 'T':
					fout = stdout;
					errorf = stderr;
					break;
				case 'v': case 'V':
					report = 1;
					break;
# ifdef NLS16
				case 'w': opt_w_set = 1;
                                          if (!(strcmp(nl_langinfo(BYTES_CHAR),
                                                       "2"))) 
                                             nls_wchar = nls16 = 1;
                                          else 
                                             nls_wchar = nls16 = 0;
					  break;
				case 'm': if (!(strcmp(nl_langinfo(BYTES_CHAR),
                                                       "2")))
                                             nls16=1;
                                          else 
                                             nls16=0;
					  break;
# endif
				case 'n': case 'N':
					report = 0;
					break;
				case 'X':	/* table size resets */
					{ char *cp = &argv[1][i];
					  char c = *++cp;
				  	int sz;
				  	if (c==0 || *++cp==0) {
						warning(BADXOPT);
						goto nextarg;
						}
				  	sz = atoi(cp);
				  	/* simple-minded check */
				  	if (sz <= MINTABSZ) {
						warning(BADTBLSIZE);
						goto nextarg;
						}
					reset_table_size(c, sz);
					goto nextarg;
					}
				case 'L':	/* use alternate ncform */
					{
					char *cp;
					int c=0;
					cp = &argv[1][i];
					while(*cp != 0)
						ncformfile[c++]=*++cp;
					alt_ncform = 1;
					goto nextarg;
					}
				default:
					warning(BADOPTION, &argv[1][i]);
				}
			}
		nextarg:
		argc--;
		argv++;
		}
        if(fout == NULL){
                sprintf(fname, "lex.yy.%c", ratfor ? 'r' : 'c' );
                fout = fopen(fname, "w");
                }
        if(fout == NULL) error(NOPEN, fname);
	sargc = argc;
	sargv = argv;
	if (argc > 1){
		fin = fopen(inputfile=argv[++fptr], "r");	/* open argv[1] */
		sargc--;
		}
	else {
		fin = stdin;
		inputfile = "-";
		}

	if(fin == NULL){
		if ( argc > 1 )
			error(NOINPUT, argv[1]);
		else
			error(NOSTDIN);
	}

	gch();
		/* may be gotten: def, subs, sname, schar, ccl, dchar */
	get1core();
		/* may be gotten: name, left, right, nullstr, parent */
	scopy("INITIAL",sp);
	sname[0] = sp;
	stype[0] = 's';
	sp += slength("INITIAL") + 1;
	sname[1] = 0;
	if(yyparse(0)) exit(1);	/* error return code */
		/* may be disposed of: def, subs, dchar */
	free1core();
		/* may be gotten: tmpstat, foll, positions, gotof, nexts, nchar, state, atable, sfall, cpackflg */
	get2core();
	ptail();
	mkmatch();
# ifdef DEBUG
	if(debug) pccl();
# endif
	sect  = ENDSECTION;
	if(tptr>0)cfoll(tptr-1);
# ifdef DEBUG
	if(debug)pfoll();
# endif
	cgoto();
# ifdef DEBUG
	if(debug){
		printf("Print %d states:\n",stnum+1);
		for(i=0;i<=stnum;i++)stprt(i);
		}
# endif
		/* may be disposed of: positions, tmpstat, foll, state, name, left, right, parent, ccl, schar, sname */
		/* may be gotten: verify, advance, stoff */
	free2core();
	get3core();
	layout();
		/* may be disposed of: verify, advance, stoff, nexts, nchar,
			gotof, atable, ccpackflg, sfall */
# ifdef DEBUG
	free3core();
# endif
#ifdef OSF
#ifdef PAXDEV
	if (ZCH>NCH) cname="/paXdev/lib/ebcform";
#else
	if (ZCH>NCH) cname="/usr/ccs/lib/ebcform";
#endif /* PAXDEV */
	{ char *skeleton = (char *) getenv("LEXER");
	  if (skeleton) cname = skeleton;
	}
#else
	if (ZCH>NCH) cname="/usr/lib/lex/ebcform";
#endif /* OSF */
        if (alt_ncform == 1)
		cname = &ncformfile[0];
	fother = fopen(ratfor?ratname:cname,"r");
	if(fother == NULL)
		error(NOLEXDRIV, ratfor?ratname:cname);
	while ( (i=getc(fother)) != EOF)
		putc(i,fout);

	fclose(fother);
	fclose(fout);
	if(
# ifdef DEBUG
		debug   ||
# endif
			report == 1)statistics();
	fclose(stdout);
	fclose(stderr);
	exit(0);	/* success return code */
	}
get1core(){
	ccptr =	ccl = myalloc(cclsize,sizeof(*ccl));
	pcptr = pchar = myalloc(pchlen, sizeof(*pchar));
	def = (uchar **)myalloc(defsize,sizeof(*def));
	subs = (uchar **)myalloc(defsize,sizeof(*subs));
	dp = dchar = myalloc(defchar,sizeof(*dchar));
	sname = (uchar **)myalloc(startsize,sizeof(*sname));
	stype = (uchar *)myalloc(startsize,sizeof(*stype));
	sp =  schar = myalloc(startchar,sizeof(*schar));
	slptr = slist = myalloc(startsize,sizeof(*slist));
	extra = myalloc(nactions,sizeof(*extra));
	if(ccl == 0 || def == 0 || subs == 0 || dchar == 0 || sname == 0 || 
	    stype == 0 || schar == 0 || slist == 0 || extra == 0)
		error(NOCORE1);
	}
free1core(){
	if (def) cfree((char *)def,defsize,sizeof(*def));
	if (subs) cfree((char *)subs,defsize,sizeof(*subs));
	if (dchar) cfree((char *)dchar,defchar,sizeof(*dchar));
	}
get2core(){
	register int i;
	gotof = (int *)myalloc(nstates,sizeof(*gotof));
	nexts = (int *)myalloc(ntrans,sizeof(*nexts));
	nchar = myalloc(ntrans,sizeof(*nchar));
	state = (int **)myalloc(nstates,sizeof(*state));
	atable = (int *)myalloc(nstates,sizeof(*atable));
	sfall = (int *)myalloc(nstates,sizeof(*sfall));
	cpackflg = myalloc(nstates,sizeof(*cpackflg));
	tmpstat = myalloc(tptr+1,sizeof(*tmpstat));
	foll = (int **)myalloc(tptr+1,sizeof(*foll));
	nxtpos = positions = (int *)myalloc(maxpos,sizeof(*positions));
	actemp = (int *)myalloc(maxpos_state, sizeof(*actemp));
	acneg =  (int *)myalloc(maxpos_state, sizeof(*acneg));
	if(tmpstat == 0 || foll == 0 || positions == 0 ||
		gotof == 0 || nexts == 0 || nchar == 0 || state == 0 || atable == 0 || sfall == 0 || cpackflg == 0 )
		error(NOCORE2);
	for(i=0;i<=tptr;i++)foll[i] = 0;
	}
free2core(){
	if (positions) cfree((char *)positions,maxpos,sizeof(*positions));
	if (actemp) cfree((char *)actemp,maxpos_state,sizeof(*actemp));
	if (acneg) cfree((char *)acneg,maxpos_state,sizeof(*acneg));
	if (tmpstat) cfree((char *)tmpstat,tptr+1,sizeof(*tmpstat));
	if (foll) cfree((char *)foll,tptr+1,sizeof(*foll));
	if (name) cfree((char *)name,treesize,sizeof(*name));
	if (left) cfree((char *)left,treesize,sizeof(*left));
	if (right) cfree((char *)right,treesize,sizeof(*right));
	if (parent) cfree((char *)parent,treesize,sizeof(*parent));
	if (nullstr) cfree((char *)nullstr,treesize,sizeof(*nullstr));
	if (state) cfree((char *)state,nstates,sizeof(*state));
	if (sname) cfree((char *)sname,startsize,sizeof(*sname));
	if (stype) cfree((char *)stype,startsize,sizeof(*stype));
	if (schar) cfree((char *)schar,startchar,sizeof(*schar));
	if (ccl) cfree((char *)ccl,cclsize,sizeof(*ccl));
	}
get3core(){
	verify = (int *)myalloc(outsize,sizeof(*verify));
	advance = (int *)myalloc(outsize,sizeof(*advance));
	stoff = (int *)myalloc(stnum+2,sizeof(*stoff));
	if(verify == 0 || advance == 0 || stoff == 0)
		error(NOCORE3);
	}
# ifdef DEBUG
free3core(){
	if (advance) cfree((char *)advance,outsize,sizeof(*advance));
	if (verify) cfree((char *)verify,outsize,sizeof(*verify));
	if (stoff) cfree((char *)stoff,stnum+1,sizeof(*stoff));
	if (gotof) cfree((char *)gotof,nstates,sizeof(*gotof));
	if (nexts) cfree((char *)nexts,ntrans,sizeof(*nexts));
	if (nchar) cfree((char *)nchar,ntrans,sizeof(*nchar));
	if (atable) cfree((char *)atable,nstates,sizeof(*atable));
	if (sfall) cfree((char *)sfall,nstates,sizeof(*sfall));
	if (cpackflg) cfree((char *)cpackflg,nstates,sizeof(*cpackflg));
	}
# endif
uchar *myalloc(a,b)
  int a,b; {
	register uchar *p;
	if ((p = (uchar *) calloc(a, b)) == (uchar *) NULL) {
		warning(CALLOCFAILED);
	}
	return p;
	}
# ifdef DEBUG
buserr(){
	fflush(errorf);
	fflush(fout);
	fflush(stdout);
	fprintf(errorf,"Bus error\n");
	if(report == 1)statistics();
	fflush(errorf);
	}
segviol(){
	fflush(errorf);
	fflush(fout);
	fflush(stdout);
	fprintf(errorf,"Segmentation violation\n");
	if(report == 1)statistics();
	fflush(errorf);
	}
# endif

yyerror(s)
uchar *s;
{
        /* For NLS purposes, attempt to intercept known "hard-coded" run-   */
        /* time error messages sent to yyerror and use the message catalog  */
        /* versions.  These messages are generated by yacc when parser.y is */
        /* being yacc'ed during building of the final yacc (Make sense?).   */

        if (!strcmp(s,"syntax error - cannot backup")) error (SYNTAXBKUP);
        else if (!strcmp(s,"yacc stack overflow"))     error (STACKOVR);
        else if (!strcmp(s,"syntax error"))            error (SYNTAX);
        else if (!strcmp(s,"unable to allocate space for yacc stacks"))
           error (NOSPACE);
        else
           /* Oh well, we tried.  Just output the hard coded string. */
	   message(LINE, itos(b_, yyline), s);
}


reset_table_size(c, newsize)
  char  c;
  int newsize;
{
  switch(c) {
	case 'd':
		defsize = newsize;
		break;
	case 'D':
		defchar = newsize;
		break;
	case 's':
		startsize = newsize;
		break;
	case 'S':
		startchar = newsize;
		break;
	case 'c':
		cclsize = newsize;
		break;
	case 'a':
		nactions = newsize;
		break;
	default:
		warning(BADTBLSPC, ctos(b_, c));
		break;
	}
}
