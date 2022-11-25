/* @(#) $Revision: 72.2 $ */   
# include "dextern"
# include <signal.h>

#if defined( NLS) || defined( NLS16)
  nl_catd nlmsg_fd;		/* message catalog descriptor */
#endif
#ifdef NLS16
  char    mbuf_[2];  /* Internal buffer used only by IF2BYTE */
  int     nls16;
#endif

# define IDENTIFIER 257
# define MARK 258
# define TERM 259
# define LEFT 260
# define RIGHT 261
# define BINARY 262
# define PREC 263
# define LCURLY 264
# define C_IDENTIFIER 265  /* name followed by colon */
# define NUMBER 266
# define START 267
# define TYPEDEF 268
# define TYPENAME 269
# define UNION 270
# define ENDFILE 0

	/* communication variables between various I/O routines */

uchar *infile;	/* input file name */
int numbval;	/* value of an input number */
uchar tokname[NAMESIZE];	/* input token name */

	/* storage of names */

extern uchar *cnames;	/* place where token and nonterminal names are stored */
extern int cnamsz;	/* size of cnames */
uchar * cnamp;		/* place where next name is to be put in */
int ndefout = 3;  /* number of defined symbols output */

	/* storage of types */
int ntypes;	/* number of types defined */
uchar * typeset[NTYPES];	/* pointers to type tags */

	/* symbol tables for tokens and nonterminals */

int ntokens = 0;
extern struct toksymb *tokset;	/* tokset[maxterms] */
extern int *toklev;		/* toklev[maxterms] */
int nnonter = -1;
extern struct ntsymb *nontrst;	/* nontrst[maxnonterm] */
static int start;	/* start symbol */

	/* assigned token type values */
int extval = 0;

	/* input and output file descriptors */

FILE * finput;		/* yacc input file */
FILE * faction;		/* file for saving actions */
FILE * fdefine;		/* file for # defines */
FILE * ftable;		/* y.tab.c file */
FILE * ftemp;		/* tempfile to pass 2 */
FILE * fdebug;		/* where the strings for debugging are stored */
FILE * foutput;		/* y.output file */

	/* temp file names */
uchar * tempname;
uchar * actname;
uchar * debugname;


	/* storage for grammar rules */

extern int *mem0 ;	/* mem0[memsize]: production storage */
int *mem;
int nprod= 1;	/* number of productions */
extern int **prdptr;	/* prdptr[maxprod]: pointers to descriptions of productions */
extern int *levprd ;	/* levprd[maxprod]: precedence levels for the productions */
extern uchar *had_act;	/* had_act[maxprod]: set to 1 if the reduction has action code to do */

int gen_lines = 1;	/* flag for generating the # line's default is yes */
int gen_testing = 0;	/* flag for whether to include runtime debugging */
extern int fatfl;

# define MINTABSZ	40

char dflag;
char  vflag;
uchar * basename = (uchar *) BASENAME;      /* prefix string for output file names */
uchar * yyprefix = (uchar *) YYPREFIX;
uchar * yystorage_class = (uchar *) "__YYSCLASS ";

setup(argc,argv) int argc; char *argv[];
{	int i,j,lev,t, ty;
	int c;
	int *p;
	uchar actname[8];
	extern int errno;

#if defined( NLS) || defined( NLS16)
	/* initialize language locale */
	nls16 = 0;
	if (!setlocale( LC_ALL, "")) {
		/* bad initialization */
#ifndef OSF
		fputs( _errlocale(), stderr);
#endif /* OSF */
		nlmsg_fd = (nl_catd) -1;
		putenv( "LANG=");	/* for perror */
                nlmsg_fd = catopen( "/usr/lib/nls/C/yacc.cat",NL_CAT_LOCALE);
	}
	else {
		/* good initialization: check char set size, open message 
                   catalog, ... keep on going if it isn't there */

		if (!(strcmp(nl_langinfo(BYTES_CHAR), "2"))) {
			nls16 = 1;
			}
		nlmsg_fd = catopen( "yacc", NL_CAT_LOCALE);
                if (nlmsg_fd == -1) {
                   printf(stderr,"yacc: warning: Cannot read %s message catalog; using default language\n",getenv("LANG"));
                   nlmsg_fd = catopen( "/usr/lib/nls/C/yacc.cat",NL_CAT_LOCALE);
                   }
	}
#endif

	foutput = NULL;
	fdefine = NULL;
	i = 1;
	while( argc >= 2  && argv[1][0] == '-' ) {
		while( *++(argv[1]) ){
			switch( *argv[1] ){
#if defined(OSF) || defined(PAXDEV)
			case 's':
			case 'S':
				SplitFlag++;
				continue;
#endif
			case 'v':
			case 'V':
				vflag = 1;
				continue;
			case 'D':
			case 'd':
				dflag = 1;
				continue;
			case 'l':
			case 'L':
				gen_lines = !gen_lines;
				continue;
			case 't':
			case 'T':
				gen_testing = !gen_testing;
				continue;
			case 'o':
			case 'O':
				message( stderr, OISDEFAULT);
				continue;

			case 'r':
			case 'R':
				error( NORATFOR);
				continue;
			case 'b':
				{ uchar * cp = (uchar *) argv[1];
				  if (*++cp) {
					basename = cp;
					}
				  else {
					/* next arg is the prefix name */
					argc--; argv++;
					if (argc<2)
					   error( BADBNAME);
					basename = (uchar *) argv[1];
					}
				  goto nextarg;
				  }
				
			case 'p':
				{ uchar * cp = (uchar *) argv[1];
				  if (*++cp) {
					yyprefix = cp;
					}
				  else {
					/* next arg is the prefix name */
					argc--; argv++;
					if (argc<2)
					   error( BADPNAME);
					yyprefix = (uchar *) argv[1];
					}
				  goto nextarg;
				  }
				

			case 'N':	/* table size resets */
				{ uchar * cp = (uchar *)argv[1];
				  uchar c = *++cp;
				  int sz;
				  if (c==0 || *++cp==0) {
					error(BADNOPT);
					goto nextarg;
					}
				  sz = atoi(cp);
				  /* simple-minded check */
				  if (sz <= MINTABSZ) {
					error(SMALLTBL, argv[1]);
					goto nextarg;
					}
				reset_table_size(c, sz);
				goto nextarg;
				}

			case 'X':	/* undocumented (internal)  options */
				{ uchar * cp = (uchar *)argv[1];
				  uchar c = *++cp;
				  if (c==0 || *++cp==0) {
					error(BADXOPT);
					goto nextarg;
					}
				  switch(c) {
					default:
						error(BADXOPT);
						goto nextarg;
					case 'p':
						/* -Xp<file>
						   reset the <file> to be used
						   as the parser.
						 */
						parser = cp;
						message(stderr,PSRRST, parser);
						goto nextarg;
					}
				}

			default:
				error( BADOPT, *argv[1]);
				}
			}
		nextarg:
		argv++;
		argc--;
		}

	if ( argc < 2 ) 
		error( NOGRAMMAR);
	if ( argc > 2 )
		error( BADARGS);
	if( ((finput=fopen( infile=(uchar *)argv[1], "r" )) == NULL ) ){
		error( CANTOPNINP);
		}

	setup_signal_handling();
	open_output_files();
	open_temp_files();

	init_tables();
	cnamp = cnames;
	defin(0,"$end");
	extval = 0400;
	defin(0,"error");
	defin(1,"$accept");
	mem=mem0;
	lev = 0;
	ty = 0;
	i=0;
	beg_debug();	/* initialize fdebug file */

	/* sorry -- no yacc parser here.....
		we must bootstrap somehow... */

	for( t=gettok();  t!=MARK && t!= ENDFILE; ){
		switch( t ){

		case ';':
			t = gettok();
			break;

		case START:
			if( (t=gettok()) != IDENTIFIER ){
				error( BADPCTSTRT);
				}
			start = chfind(1,tokname);
			t = gettok();
			continue;

		case TYPEDEF:
			if( (t=gettok()) != TYPENAME ) error( BADPCTTYPE);
			ty = numbval;
			for(;;){
				t = gettok();
				switch( t ){

				case IDENTIFIER:
					if( (t=chfind( 1, tokname ) ) < NTBASE ) {
						j = TYPE( toklev[t] );
						if( j!= 0 && j != ty ){
							error( TKNREDECL, tokset[t].name );
							}
						else SETTYPE( toklev[t],ty);
						}
					else {
						j = nontrst[t-NTBASE].tvalue;
						if( j != 0 && j != ty ){
							error( NTERMREDECL, nontrst[t-NTBASE].name );
							}
						else nontrst[t-NTBASE].tvalue = ty;
						}
				case ',':
					continue;

				case ';':
					t = gettok();
					break;
				default:
					break;
					}
				break;
				}
			continue;

		case UNION:
			/* copy the union declaration to the output */
			cpyunion();
			t = gettok();
			continue;

		case LEFT:
		case BINARY:
		case RIGHT:
			++i;
		case TERM:
			lev = t-TERM;  /* nonzero means new prec. and assoc. */
			ty = 0;

			/* get identifiers so defined */

			t = gettok();
			if( t == TYPENAME ){ /* there is a type defined */
				ty = numbval;
				t = gettok();
				}

			for(;;) {
				switch( t ){

				case ',':
					t = gettok();
					continue;

				case ';':
					break;

				case IDENTIFIER:
					j = chfind(0,tokname);
					if( lev ){
						if( ASSOC(toklev[j]) ) error( PRECREDECL,tokname );
						SETASC(toklev[j],lev);
						if (i > NPLEVS)
						   error(MANYPREC );
						SETPLEV(toklev[j],i);
						}
					if( ty ){
						if( TYPE(toklev[j]) ) error( TYPEREDECL,tokname );
						SETTYPE(toklev[j],ty);
						}
					if( (t=gettok()) == NUMBER ){
						tokset[j].value = numbval;
						if( j < ndefout && j>2 ){
							error( LATEPCTS, tokset[j].name );
							}
						t=gettok();
						}
					continue;

					}

				break;
				}

			continue;

		case LCURLY:
			defout();
			cpycode();
			t = gettok();
			continue;

		default:
			error( SYNTAX);

			}

		}

	if( t == ENDFILE ){
		error( BADEOF);
		}

	/* t is MARK */

	write_default_code();
	pdefout();
	defout();
	end_toks();	/* all tokens dumped - get ready for reductions */

	fprintf( ftable,  "#define yyclearin yychar = -1\n" );
	fprintf( ftable,  "#define yyerrok yyerrflag = 0\n" );
	fprintf( ftable,  "extern int yychar;\n" );
	fprintf( ftable,  "#ifndef YYMAXDEPTH\n#define YYMAXDEPTH 150\n#endif\n" );

	/* print ifdef's for __YYSCLASS which will control the scoping class
	 * of "local" parser variables. */
	fprintf( ftable, "\n/* __YYSCLASS defines the scoping/storage class for global objects\n");
	fprintf( ftable, " * that are NOT renamed by the -p option.  By default these names\n");
	fprintf( ftable, " * are going to be 'static' so that multi-definition errors\n");
	fprintf( ftable, " * will not occur with multiple parsers.\n");
	fprintf( ftable, " * If you want (unsupported) access to internal names you need\n");
	fprintf( ftable, " * to define this to be null so it implies 'extern' scope.\n");
	fprintf( ftable, " * This should not be used in conjunction with -p.\n");
	fprintf( ftable, " */\n");
	fprintf( ftable, "#ifndef __YYSCLASS\n# define __YYSCLASS static\n#endif\n");

	if( !ntypes ) fprintf( ftable,  "#ifndef YYSTYPE\n#define YYSTYPE int\n#endif\n" );
	fprintf( ftable,  "YYSTYPE yylval;\n" );
	fprintf( ftable,  "%sYYSTYPE yyval;\n",yystorage_class );
	fprintf( ftable,  "typedef int yytabelem;\n" );
	prdptr[0]=mem;
	/* added production */
	*mem++ = NTBASE;
	*mem++ = start;  /* if start is 0, we will overwrite with the lhs of the first rule */
	*mem++ = 1;
	*mem++ = 0;
	prdptr[1]=mem;

	while( (t=gettok()) == LCURLY ) cpycode();

	if( t != C_IDENTIFIER ) error( BADSYNTAX1);

	if( !start ) prdptr[0][1] = chfind(1,tokname);

	/* read rules */

	while( t!=MARK && t!=ENDFILE ){

		/* process a rule */

		if( t == '|' ){
			rhsfill( (uchar *) 0 );	/* restart fill of rhs */
			*mem++ = *prdptr[nprod-1];
			}
		else if( t == C_IDENTIFIER ){
			*mem = chfind(1,tokname);
			if( *mem < NTBASE ) error( LHSILL);
			++mem;
			lhsfill( tokname );	/* new rule: restart strings */
			}
		else error( NOSEMI);

		/* read rule body */


		t = gettok();
	more_rule:
		while( t == IDENTIFIER ) {
			*mem = chfind(1,tokname);
			if( *mem<NTBASE ) levprd[nprod] = toklev[*mem];
			++mem;
			rhsfill( tokname );	/* add to rhs string */
			t = gettok();
			}


		if( t == PREC ){
			if( gettok()!=IDENTIFIER) error( BADPCTPREC);
			j = chfind(2,tokname);
			if( j>=NTBASE)error(BADTERMPREC,nontrst[j-NTBASE].name);
			levprd[nprod]=toklev[j];
			t = gettok();
			}

		if( t == '=' ){
			had_act[nprod] = 1;
			levprd[nprod] |= ACTFLAG;
#if defined(OSF) || defined(PAXDEV)
			if (SplitFlag)
				 fprintf(faction,"\nstatic int yyf%d(void) {",nprod);
			    else fprintf( faction, "\ncase %d:", nprod );
			cpyact( mem-prdptr[nprod]-1 );
			if (!SplitFlag) fprintf( faction, " break;" );
			    else fprintf(faction,"\nreturn(-1);}");
#else
			fprintf( faction, "\ncase %d:", nprod );
			cpyact( mem-prdptr[nprod]-1 );
			fprintf( faction, " break;" );
#endif
			if( (t=gettok()) == IDENTIFIER ){
				/* action within rule... */

				lrprnt();		/* dump lhs, rhs */
				sprintf( actname, "$$%d", nprod );
				j = chfind(1,actname);  /* make it a nonterminal */

				/* the current rule will become rule number nprod+1 */
				/* move the contents down, and make room for the null */

				for( p=mem; p>=prdptr[nprod]; --p ) p[2] = *p;
				mem += 2;

				/* enter null production for action */

				p = prdptr[nprod];

				*p++ = j;
				*p++ = -nprod;

				/* update the production information */

				levprd[nprod+1] = levprd[nprod] & ~ACTFLAG;
				levprd[nprod] = ACTFLAG;

				if( ++nprod >= maxprod ) error( MANYPRODS);
				prdptr[nprod] = p;

				/* make the action appear in the original rule */
				*mem++ = j;

				/* get some more of the rule */

				goto more_rule;
				}

			}

		while( t == ';' ) t = gettok();

		*mem++ = -nprod;

		/* check that default action is reasonable */

		if( ntypes && !(levprd[nprod]&ACTFLAG) && nontrst[*prdptr[nprod]-NTBASE].tvalue ){
			/* no explicit action, LHS has value */
			register tempty;
			tempty = prdptr[nprod][1];
			if( tempty < 0 ) error( NOVALUE);
			else if( tempty >= NTBASE ) tempty = nontrst[tempty-NTBASE].tvalue;
			else tempty = TYPE( toklev[tempty] );
			if( tempty != nontrst[*prdptr[nprod]-NTBASE].tvalue ){
                                int fatfl_save=fatfl; /* make err non-fatal */
                                fatfl = 0;
				error( TYPECLASH);
                                fatfl = fatfl_save;
				}
			}

		if( ++nprod >= maxprod ) error( MANYPRODS);
		prdptr[nprod] = mem;
		levprd[nprod]=0;

		}

	/* end of all rules */

	end_debug();		/* finish fdebug file's input */
	finact();
	if( t == MARK ){
		if ( gen_lines )
			fprintf( ftable, "\n# line %d \"%s\"\n",
				lineno, infile );
#ifdef NLS16
		if (nls16) 
			while( (c=getc(finput)) != EOF ) {
				putc( c, ftable );
				IF2BYTE(c) {
					c=getc(finput);
					putc( c, ftable );
					}
				} /*end while*/
		else
#endif
		while( (c=getc(finput)) != EOF ) putc( c, ftable );
		}
	fclose( finput );
	}


/* Open y.tab.c, and if options used, y.tab.h and y.output */

open_output_files() {
	int base_size;
	uchar * filename;

	/* Open the output files.  */
	/* malloc space where the names can be built */
	base_size = strlen(basename);
	filename = (uchar *) malloc( base_size + NSUFFIX );
	strcpy(filename, basename);

	strcpy(filename+base_size, OFILE);
	ftable = fopen( filename, "w" );
	if( ftable == NULL ) error( CANTOPEN, filename );
	if (vflag) {
	   strcpy(filename+base_size, FILEU);
	   foutput = fopen(filename, "w" );
	   if( foutput == NULL ) error( CANTOPEN, filename );
	   }
	if (dflag) {
	   strcpy(filename+base_size, FILED);
	   fdefine = fopen( filename, "w" );
	   if ( fdefine == NULL ) error( CANTOPEN, filename );
	   }
	free(filename);
}

/* Open the temp files used by yacc.  In the "traditional" implementation
 * these are yacc.tmp, yacc.acts, and yacc.debug in the current directory.
 * But here they are implemented as uniquely named tempfiles in a tmp 
 * directory.
 */
open_temp_files() {
	/* Set up unique temp file names */
	tempname = (uchar *) tempnam(TMPDIR, TMPFILE);
	actname = (uchar *) tempnam(TMPDIR, ACTFILE);
	debugname = (uchar *) tempnam(TMPDIR, DEBUGFILE);

	/* open the temp files */
	ftemp = fopen( tempname, "w+" );
	faction = fopen( actname, "w+" );
	fdebug = fopen( debugname, "w+" );
	if( ftemp==NULL || faction==NULL || fdebug==NULL )
	    error( CANTOPNTMP);

	/* unlink the files now so they automatically go away if yacc
	 * dies or is killed.
	 */
	ZAPFILE(tempname);
	ZAPFILE(actname);
	ZAPFILE(debugname);
}


/* Write some default declarations at the head of the ftable file.
 * Currently this consists of declarations required for C++.
 */
write_default_code() {
	fprintf(ftable, "#ifdef __cplusplus\n");
	fprintf(ftable, "#  include <stdio.h>\n");
	fprintf(ftable, "   extern \"C\" {\n");
	fprintf(ftable, "     extern void yyerror(char *);\n");
	fprintf(ftable, "     extern int yylex();\n");
	fprintf(ftable, "   }\n");
	fprintf(ftable, "#endif	/* __cplusplus */ \n");
}


finact(){
	/* finish action routine */

	fflush(faction);

	fprintf( ftable, "# define YYERRCODE %d\n", tokset[2].value );

	}

defin( t, s ) register uchar  *s; {
/*	define s to be a terminal if t=0
	or a nonterminal if t=1		*/

	register val;

	if (t) {
		if( ++nnonter >= maxnonterm ) error(MANYNTERMS);
		nontrst[nnonter].name = cstash(s);
		return( NTBASE + nnonter );
		}
	/* must be a token */
	if( ++ntokens >= maxterms ) error(MANYTERMS,dtos(maxterms) );
	tokset[ntokens].name = cstash(s);

	/* establish value for token */

	if( s[0]==' ' && s[2]=='\0' ) /* single character literal */
		val = s[1];
	else if ( s[0]==' ' && s[1]=='\\' ) { /* escape sequence */
		if( s[3] == '\0' ){ /* single character escape sequence */
			switch ( s[2] ){
					 /* character which is escaped */
			case 'n': val = '\n'; break;
			case 'r': val = '\r'; break;
			case 'b': val = '\b'; break;
			case 't': val = '\t'; break;
			case 'f': val = '\f'; break;
			case '\'': val = '\''; break;
			case '"': val = '"'; break;
			case '\\': val = '\\'; break;
			default: error( BADESCAPE);
				}
			}
		else if( s[2] <= '7' && s[2]>='0' ){ /* \nnn sequence */
			if( s[3]<'0' || s[3] > '7' || s[4]<'0' ||
				s[4]>'7' || s[5] != '\0' ) error(BADNNN);
			val = 64*s[2] + 8*s[3] + s[4] - 73*'0';
			if( val == 0 ) error( NONULL);
			}
		}
	else {
		val = extval++;
		}
	tokset[ntokens].value = val;
	toklev[ntokens] = 0;
	return( ntokens );
	}

defout(){ /* write out the defines (at the end of the declaration section) */

	register int i, c;
	register uchar *cp;

	for( i=ndefout; i<=ntokens; ++i ){

		cp = tokset[i].name;
		if( *cp == ' ' )	/* literals */
		{
			fprintf( fdebug, "\t\"%s\",\t%d,\n",
				tokset[i].name + 1, tokset[i].value );
			cp++; /* in my opinion, this should be continue */
		}

#ifdef NLS16
		if (nls16)
			for( ; (c= *cp)!='\0'; ++cp ){
				if( islower(c) || isupper(c) || 
				    isdigit(c) || c=='_' );  /* VOID */
				else if (mblen( (char *) cp, 2) == 2) {
					++cp;
					if (*cp == '\0') 
						error( ILLCHAR,tokset[i].name);
					}
				else goto nodef;
				}
		else
#endif
			for( ; (c= *cp)!='\0'; ++cp ){
				if( islower(c) || isupper(c) || 
				    isdigit(c) || c=='_' );  /* VOID */
				else goto nodef;
				}

		if ( tokset[i].name[0] != ' ' )
			fprintf( fdebug, "\t\"%s\",\t%d,\n",
				tokset[i].name, tokset[i].value );
		fprintf( ftable, "# define %s %d\n", tokset[i].name, tokset[i].value );
		if( fdefine != NULL ) fprintf( fdefine, "# define %s %d\n", tokset[i].name, tokset[i].value );

	nodef:	;
		}

	ndefout = ntokens+1;

	}


pdefout() { /* write out the defines needed when a -p option is used */
  /* When the -p option is used, put out defines to rename the externals.
   * Check that the -p arg really is different than "yy" so don't emit
   * defines that will loop.
   */
  static char *pnames[] = { "parse", "lex", "error", "lval", "char",
	"debug", "maxdepth", "nerrs", 0 };
  
  char *p;
  int i;
  if (strcmp(yyprefix, YYPREFIX) == 0) return;

  for (i=0; (p = pnames[i]) != 0; i++) {
	fprintf( ftable, "# define %s%s %s%s\n", YYPREFIX, p, yyprefix, p );
	if( fdefine != NULL ) fprintf( fdefine, "# define %s%s %s%s\n", 
		YYPREFIX, p, yyprefix, p );
	}
}
	


uchar *
cstash( s ) register uchar *s; {
	uchar *temp;

	temp = cnamp;
	do {
		if( cnamp >= &cnames[cnamsz] ) error(MANYCHARS);
		else *cnamp++ = *s;
		}  while ( *s++ );
	return( temp );
	}

gettok() {
	register i, base;
	static int peekline; /* number of '\n' seen in lookahead */
	register c, match, reserve;
#ifdef NLS16
	int twobyte;
#endif

begin:
	reserve = 0;
	lineno += peekline;
	peekline = 0;
	c = getc(finput);
	while( c==' ' || c=='\n' || c=='\t' || c=='\f' ){/* skip white spaces */
		if( c == '\n' ) ++lineno;
		c=getc(finput);
		}
	if( c == '/' ){ /* skip comment */
		lineno += skipcom();
		goto begin;
		}

	switch(c){

	case EOF:
		return(ENDFILE);
	case '{':
		ungetc( c, finput );
		return( '=' );  /* action ... */
	case '<':  /* get, and look up, a type name (union member name) */
		i = 0;
#ifdef NLS16
                if (nls16)
		   while( (c=getc(finput)) != '>' && c>=0 && c!= '\n' ){
			   tokname[i] = c;
			   if( ++i >= NAMESIZE ) --i;
                           IF2BYTE(c) {
				tokname[i] = getc(finput);
				if( ++i >= NAMESIZE ) --i;
				}
			   }
                else
#endif /*NLS16*/
		   while( (c=getc(finput)) != '>' && c>=0 && c!= '\n' ){
			   tokname[i] = c;
			   if( ++i >= NAMESIZE ) --i;
			   }
		if( c != '>' ) error( BADCLAUSE);
		tokname[i] = '\0';
		for( i=1; i<=ntypes; ++i ){
			if( !strcmp( typeset[i], tokname ) ){
				numbval = i;
				return( TYPENAME );
				}
			}
		if (ntypes >= NTYPES) 
			error(MANYTYPES);
		typeset[numbval = ++ntypes] = cstash( tokname );
		return( TYPENAME );

	case '"':	
	case '\'':
		match = c;
		tokname[0] = ' ';
		i = 1;
#ifdef NLS16
                if (nls16)
		   for(;;){
			   c = getc(finput);
                           IF2BYTE(c) {
			       tokname[i] = c;
			       if( ++i >= NAMESIZE ) --i;
			       c = getc(finput); /* consume second half */
			       tokname[i] = c;
			       if( ++i >= NAMESIZE ) --i;
                               }
                           else {
			       if( c == '\n' || c == EOF )
				       error(NOQUOTE);
			       if( c == '\\' ){
				       c = getc(finput);
				       tokname[i] = '\\';
				       if( ++i >= NAMESIZE ) --i;
				       }
			       else if( c == match ) break;
			       tokname[i] = c;
			       if( ++i >= NAMESIZE ) --i;
                               }
			   }/* end for*/
                   else
#endif /*NLS16*/
		   for(;;){
			   c = getc(finput);
			   if( c == '\n' || c == EOF )
				   error(NOQUOTE);
			   if( c == '\\' ){
				   c = getc(finput);
				   tokname[i] = '\\';
				   if( ++i >= NAMESIZE ) --i;
				   }
			   else if( c == match ) break;
			   tokname[i] = c;
			   if( ++i >= NAMESIZE ) --i;
			   }
		break;

	case '%':
	case '\\':

		switch(c=getc(finput)) {

		case '0':	return(TERM);
		case '<':	return(LEFT);
		case '2':	return(BINARY);
		case '>':	return(RIGHT);
		case '%':
		case '\\':	return(MARK);
		case '=':	return(PREC);
		case '{':	return(LCURLY);
		default:	reserve = 1;
			}

	default:

#ifdef NLS16
		twobyte = 0;
#endif
		if( isdigit(c) ){ /* number */
			numbval = c-'0' ;
			base = (c=='0') ? 8 : 10 ;
			for( c=getc(finput); isdigit(c) ; c=getc(finput) ){
				numbval = numbval*base + c - '0';
				}
			ungetc( c, finput );
			return(NUMBER);
			}
		else if( islower(c) || isupper(c) || c=='_' || c=='.' || c=='$' ){
			i = 0;
#ifdef NLS16
			if (nls16)
nlsident:
                                while( islower(c) || isupper(c) || isdigit(c) ||
                                       c=='_' || c=='.' || c=='$' || twobyte){
                                          tokname[i] = c;
                                          if( reserve && isupper(c) && !twobyte)
                                                  tokname[i] += 'a'-'A';
                                          if( ++i >= NAMESIZE ) --i;
                                          c = getc(finput);
                                          if (twobyte) { /* consume 2nd byte */
                                                twobyte = 0;
                                                tokname[i] = c;
                                                if( ++i >= NAMESIZE ) --i;
                                                c = getc(finput);
                                                IF2BYTE(c)
                                                        twobyte = 1;
                                                }
					  else {
						IF2BYTE(c)
                                                	twobyte = 1;
						}
                                          }
			else
#endif
				while( islower(c) || isupper(c) || isdigit(c) || c=='_' || c=='.' || c=='$' ){
					tokname[i] = c;
					if( reserve && isupper(c) ) tokname[i] += 'a'-'A';
					if( ++i >= NAMESIZE ) --i;
					c = getc(finput);
					}
			}
#ifdef NLS16
		else if (nls16) { /* check for identifier with 2byte 1st char */
			IF2BYTE(c){ 
				i = 0;
				twobyte = 1; 
				goto nlsident;
				}
			else return(c);
			}
#endif
		else return(c);

		ungetc( c, finput );
		}

	tokname[i] = '\0';

	if( reserve ){ /* find a reserved word */
		if( !strcmp(tokname,"term")) return( TERM );
		if( !strcmp(tokname,"token")) return( TERM );
		if( !strcmp(tokname,"left")) return( LEFT );
		if( !strcmp(tokname,"nonassoc")) return( BINARY );
		if( !strcmp(tokname,"binary")) return( BINARY );
		if( !strcmp(tokname,"right")) return( RIGHT );
		if( !strcmp(tokname,"prec")) return( PREC );
		if( !strcmp(tokname,"start")) return( START );
		if( !strcmp(tokname,"type")) return( TYPEDEF );
		if( !strcmp(tokname,"union")) return( UNION );
		error(BADRSRVDWRD, tokname );
		}

	/* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */

	c = getc(finput);
	while( c==' ' || c=='\t'|| c=='\n' || c=='\f' || c== '/' ) {
		if( c == '\n' ) ++peekline;
		else if( c == '/' ){ /* look for comments */
			peekline += skipcom();
			}
		c = getc(finput);
		}
	if( c == ':' ) return( C_IDENTIFIER );
	ungetc( c, finput );
	return( IDENTIFIER );
}

fdtype( t ){ /* determine the type of a symbol */
	register v;
	if( t >= NTBASE ) v = nontrst[t-NTBASE].tvalue;
	else v = TYPE( toklev[t] );
	if( v <= 0 ) error( MISSNGTYPE, (t>=NTBASE)?nontrst[t-NTBASE].name:
			tokset[t].name );
	return( v );
	}

chfind( t, s ) register uchar *s; {
	int i;

	if (s[0]==' ')t=0;
	TLOOP(i){
		if(!strcmp(s,tokset[i].name)){
			return( i );
			}
		}
	NTLOOP(i){
		if(!strcmp(s,nontrst[i].name)) {
			return( i+NTBASE );
			}
		}
	/* cannot find name */
	if( t>1 )
		error( LATEDEFIN, s );
	return( defin( t, s ) );
	}

cpyunion(){
	/* copy the union declaration to the output, and the define file if present */

	int level, c;
	if ( gen_lines )
		fprintf( ftable, "\n# line %d \"%s\"\n", lineno, infile );
	fprintf( ftable, "typedef union " );
	if( fdefine ) fprintf( fdefine, "\ntypedef union " );

	level = 0;
#ifdef NLS16
	/* This is added for when C accepts multibyte identifiers */
	if (nls16)  
		for(;;){
			c=getc(finput);
			IF2BYTE(c) {
				putc( c, ftable );
				if( fdefine ) putc( c, fdefine );

				/* consume 2nd byte */
				c=getc(finput); 
				putc( c, ftable );
				if( fdefine ) putc( c, fdefine );
				}
			else {
				if(c < 0 ) error( EOFINUNION);
				putc( c, ftable );
				if( fdefine ) putc( c, fdefine );
	
				switch( c ){
	
				case '\n':
					++lineno;
					break;
	
				case '{':
					++level;
					break;
	
				case '}':
					--level;
					if( level == 0 ) { /* done copying */
						fprintf( ftable, " YYSTYPE;\n" );
						if( fdefine ) fprintf( fdefine, " YYSTYPE;\nextern YYSTYPE yylval;\n" );
						return;
						}
					}
				} /*end else*/
			} /*end for*/
	else
#endif
	for(;;){
		if( (c=getc(finput)) < 0 ) error( EOFINUNION);
		putc( c, ftable );
		if( fdefine ) putc( c, fdefine );

		switch( c ){

		case '\n':
			++lineno;
			break;

		case '{':
			++level;
			break;

		case '}':
			--level;
			if( level == 0 ) { /* we are finished copying */
				fprintf( ftable, " YYSTYPE;\n" );
				if( fdefine ) fprintf( fdefine, " YYSTYPE;\nextern YYSTYPE yylval;\n" );
				return;
				}
			}
		}
	}

cpycode(){ /* copies code between \{ and \} */

	int c;
	c = getc(finput);
	if( c == '\n' ) {
		c = getc(finput);
		lineno++;
		}
	if ( gen_lines )
		fprintf( ftable, "\n# line %d \"%s\"\n", lineno, infile );
#ifdef NLS16
	if (nls16) {
startagain:
		IF2BYTE(c) {
			putc(c, ftable);
			putc (c=getc(finput), ftable); /* consume 2nd byte */
			c=getc(finput);
			goto startagain;
			}
		while( c>=0 ){
			if( c=='\\' )
				if( (c=getc(finput)) == '}' ) return;
				else putc('\\', ftable );
			if( c=='%' )
				if( (c=getc(finput)) == '}' ) return;
				else putc('%', ftable );
			putc( c , ftable );
			if( c == '\n' ) ++lineno;
			IF2BYTE(c) {
				c = getc(finput);
				putc( c , ftable );
				}
			c = getc(finput);
			} /*end while*/
		} /*end if*/
	else
#endif
	while( c>=0 ){
		if( c=='\\' )
			if( (c=getc(finput)) == '}' ) return;
			else putc('\\', ftable );
		if( c=='%' )
			if( (c=getc(finput)) == '}' ) return;
			else putc('%', ftable );
		putc( c , ftable );
		if( c == '\n' ) ++lineno;
		c = getc(finput);
		}
	error(EOFINCODE);
	}

skipcom(){ /* skip over comments */
	register c, i=0;  /* i is the number of lines skipped */

	/* skipcom is called after reading a / */

	if( getc(finput) != '*' ) error( BADCMNT);
	c = getc(finput);
#ifdef NLS16
	if (nls16) 
/* SWFfc00964 fix - Correct problem with 2 byte char. See SR 1653-086454 */
                while( c != EOF ){
                   while( c == '*' ){
                      if( (c=getc(finput)) == '/' ) return( i );
                      }
                   if( c == '\n' ) ++i;
                    else {
                       IF2BYTE(c)
                          c = getc(finput);
                       }
                    c = getc(finput);
                    }
	else
#endif
	while( c != EOF ){
		while( c == '*' ){
			if( (c=getc(finput)) == '/' ) return( i );
			}
		if( c == '\n' ) ++i;
		c = getc(finput);
		}
	error( EOFINCMNT);
	/* NOTREACHED */
	}

cpyact(offset){ /* copy C action to the next ; or closing } */
	int brac, c, match, j, s, tok;

	if ( gen_lines )
		fprintf( faction, "\n# line %d \"%s\"\n", lineno, infile );

	brac = 0;

loop:
	c = getc(finput);
swt:
	switch( c ){

case ';':
		if( brac == 0 ){
			putc( c , faction );
			return;
			}
		goto lcopy;

case '{':
		brac++;
		goto lcopy;

case '$':
		s = 1;
		tok = -1;
		c = getc(finput);
		if( c == '<' ){ /* type description */
			ungetc( c, finput );
			if( gettok() != TYPENAME ) error( IDENTSYNTAX);
			tok = numbval;
			c = getc(finput);
			}
		if( c == '$' ){
			fprintf( faction, "yyval");
			if( ntypes ){ /* put out the proper tag... */
				if( tok < 0 ) tok = fdtype( *prdptr[nprod] );
				fprintf( faction, ".%s", typeset[tok] );
				}
			goto loop;
			}
		if( c == '-' ){
			s = -s;
			c = getc(finput);
			}
		if( isdigit(c) ){
			j=0;
			while( isdigit(c) ){
				j= j*10+c-'0';
				c = getc(finput);
				}

			j = j*s - offset;
			if( j > 0 ){
				error( BADDOLLAR, dtos(j+offset) );
				}

			fprintf( faction, "yypvt[-%d]", -j );
			if( ntypes ){ /* put out the proper tag */
				if( j+offset <= 0 && tok < 0 ) error( NOTYPE, dtos(j+offset) );
				if( tok < 0 ) tok = fdtype( prdptr[nprod][j+offset] );
				fprintf( faction, ".%s", typeset[tok] );
				}
			goto swt;
			}
		putc( '$' , faction );
		if( s<0 ) putc('-', faction );
		goto swt;

case '}':
		if( --brac ) goto lcopy;
		putc( c, faction );
		return;


case '/':	/* look for comments */
		putc( c , faction );
		c = getc(finput);
		if( c != '*' ) goto swt;

		/* it really is a comment */

		putc( c , faction );
		c = getc(finput);
#ifdef NLS16
		if (nls16)
			for(;;) {
				while( c != '*' ) {
					IF2BYTE(c) {
						putc( c , faction );
						c = getc(finput);
						}
					else if ( c=='\n' ) lineno++;
					else if ( c==EOF ) 
						error( EOFINCMNT);
					putc( c , faction );
					c = getc(finput);
					} /*end while*/
				putc( c , faction );
                                if( (c=getc(finput)) == '/' ) goto lcopy;
				} /*end for*/
		else
#endif
		while( c != EOF ){
			while( c=='*' ){
				putc( c , faction );
				if( (c=getc(finput)) == '/' ) goto lcopy;
				}
			putc( c , faction );
			if( c == '\n' )++lineno;
			c = getc(finput);
			}
		error( EOFINCMNT);

case '\'':	/* character constant */
		match = '\'';
		goto string;

case '"':	/* character string */
		match = '"';

	string:

		putc( c , faction );
#ifdef NLS16
		if (nls16)
			while( c=getc(finput) ){
				if( c=='\\' ){
					putc( c , faction );
					c=getc(finput);
					if( c == '\n' ) ++lineno;
					}
				else if( c==match ) goto lcopy;
				else if( c=='\n' ) error( BADSTRING);
				else {
					IF2BYTE(c) {
						putc( c , faction );
						c=getc(finput);
						}
					}
				putc( c , faction );
				} /*end while*/
		else
#endif
		while( c=getc(finput) ){

			if( c=='\\' ){
				putc( c , faction );
				c=getc(finput);
				if( c == '\n' ) ++lineno;
				}
			else if( c==match ) goto lcopy;
			else if( c=='\n' ) error( BADSTRING);
			putc( c , faction );
			}
		error( EOFINSTRING);

case EOF:
		error(NOTERMINATE);

case '\n':	++lineno;
		goto lcopy;

		}

lcopy:
	putc( c , faction );
#ifdef NLS16
	if (nls16) {
		IF2BYTE(c) {
			c=getc(finput);
			putc( c , faction );
			}
		}
#endif
	goto loop;
	}


#define RHS_TEXT_LEN		( BUFSIZ * 4 )	/* length of rhstext */

uchar lhstext[ BUFSIZ ];	/* store current lhs (non-terminal) name */
uchar rhstext[ RHS_TEXT_LEN ];	/* store current rhs list */

lhsfill( s )	/* new rule, dump old (if exists), restart strings */
	uchar *s;
{
	rhsfill( (uchar *) 0 );
	strcpy( lhstext, s );	/* don't worry about too long of a name */
}


rhsfill( s )
	uchar *s;	/* either name or 0 */
{
	static uchar *loc = rhstext;	/* next free location in rhstext */
	register uchar *p;

	if ( !s )	/* print out and erase old text */
	{
		if ( *lhstext )		/* there was an old rule - dump it */
			lrprnt();
		( loc = rhstext )[0] = '\0';
		return;
	}
	/* add to stuff in rhstext */
	p = s;
	*loc++ = ' ';
	if ( *s == ' ' )	/* special quoted symbol */
	{
		*loc++ = '\'';	/* add first quote */
		p++;
	}
	while ( *loc = *p++ )
		if ( loc++ > &rhstext[ RHS_TEXT_LEN ] - 2 )
			break;
	if ( *s == ' ' )
		*loc++ = '\'';
	*loc = '\0';		/* terminate the string */
}


lrprnt()	/* print out the left and right hand sides */
{
	uchar *rhs;

	if ( !*rhstext )		/* empty rhs - print usual comment */
		rhs = (uchar*) " /* empty */";
	else
		rhs = rhstext;
	fprintf( fdebug, "	\"%s :%s\",\n", lhstext, rhs );
}


beg_debug()	/* dump initial sequence for fdebug file */
{
	fprintf( fdebug,
		"typedef struct { char *t_name; int t_val; } yytoktype;\n" );
	fprintf( fdebug,
		"#ifndef YYDEBUG\n#\tdefine YYDEBUG\t%d", gen_testing );
	fprintf( fdebug, "\t/*%sallow debugging */\n#endif\n\n",
		gen_testing ? " " : " don't ");
	fprintf( fdebug, "#if YYDEBUG\n\n%syytoktype yytoks[] =\n{\n",
		yystorage_class);
}


end_toks()	/* finish yytoks array, get ready for yyred's strings */
{
	fprintf( fdebug, "\t\"-unknown-\",\t-1\t/* ends search */\n");
	fprintf( fdebug, "};\n\n%schar * yyreds[] =\n{\n", yystorage_class);
	fprintf( fdebug, "\t\"-no such reduction-\",\n");
}


end_debug()	/* finish yyred array, close file */
{
	lrprnt();		/* dump last lhs, rhs */
	fprintf( fdebug, "};\n#endif /* YYDEBUG */\n" );
	fflush( fdebug );
}


/*****************************************************************************
 * setup_signal_handling
 *      Call "signal" to catch interrupt singlas for hang-up, break,
 *      and terminate, so temp files can get cleaned up.  Otherwise,
 *	there is a short window in 'open_temp_files' between the fopen's
 *	and the unlinks where temp files would be left around if yacc
 *	is killed.
 */

void onintr() {
   if (tempname) ZAPFILE(tempname);
   if (actname)  ZAPFILE (actname);
   if (debugname) ZAPFILE (debugname);
}

setup_signal_handling()
{
   if (signal(SIGHUP,SIG_IGN) == SIG_DFL)
	signal(SIGHUP,onintr);
   if (signal(SIGINT,SIG_IGN) == SIG_DFL)
	signal(SIGINT,onintr);
   if (signal(SIGTERM,SIG_IGN) == SIG_DFL)
	signal(SIGTERM,onintr);
}


