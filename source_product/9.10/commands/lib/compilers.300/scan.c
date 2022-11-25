/* DOND_ID @(#)scan.c           70.11 93/11/11 */
/*    SCCS    REV(64.9);       DATE(92/06/29        17:43:46) */
/* KLEENIX_ID @(#)scan.c	64.9 92/06/24 */
/* file scan.c */

#define _HPUX_SOURCE               /* for 'FIRSTof2' and 'signal.h'*/

#ifdef OSF
#if defined(PAXDEV) 
typedef unsigned int wchar_t;
#endif /* PAXDEV */
#endif /* OSF */

#ifdef SADEBUG
# include <signal.h>
#endif /* SADEBUG */

#if defined(HAIL) || defined(OSF)
# include "mfile1"
# include "messages.h"
# ifndef __DATE__ /* use __DATE__ because pre-ANSI versions of Domain/OS define __STDC__ */
# include <malloc.h>
# endif
# include <string.h>
# include "opcodes.h"
# if !defined(PAXDEV) 
# include "stdlib.h"
#endif /* PAXDEV */
# include <time.h>
#undef MB_CUR_MAX
#define MB_CUR_MAX 1
#else
# include "mfile1"
# include "messages.h"
# include <nl_ctype.h>
# include <locale.h>
# include <malloc.h>
# include <string.h>
# include "opcodes.h"
# include <stdlib.h>
# include <time.h>
#endif

#if defined(OSF) && !defined(PAXDEV) 
# include <locale.h>
#endif /* OSF && !PAXDEV */

#ifdef APEX
# include "lmanifest"
# include "apex.h"
int opt_attr_flag = 0;
#endif

# define MB_LEN_MAX 2   /* should include <limits.h> for this ***/

extern short picoption; /* Position Indep. Code option */

#ifdef HAIL
extern int map_ldouble_to_double;
#endif /* HAIL */

#ifdef HPCDB
char cdbfile[FTITLESZ]  = "";
int cdbfile_vtindex;
int  comp_unit_vtindex;
char comp_unit_title[FTITLESZ];
flag in_comp_unit = 0;           /* Indicates whether tokens are from
				  * the .c, the compilation unit. */
flag Allflag = 0; 
               /* Output ALL debug info from .h files  */
# ifdef SA
#include "sa.h"
char *satitle;
flag saflag = 0;                 /* Indicates whether -y is on */
# endif /* SA */ 

# ifdef SADEBUG
flag sadebug = 0;
sig_handler() { abort();}
# endif /* SADEBUG */

#endif /* HPCDB */

flag compiletoassembler = 0;
#ifdef YYCOVERAGE
char *yy_rfile = NULL;		/* file to save and restore reduction hits */
int record_reductions = 0;	/* true: record reduction hits */
extern long yy_rtable[];	/* the table of reduction hits */
#endif /* YYCOVERAGE */

#ifdef MSG_COVERAGE
char *msgdb_filename = "msg.db";/* file name */
flag msg_coverage = 0;          /* record message error if true */
extern void update_msgdb();     /* in common */
#endif /* MSG_COVERAGE */

# ifdef C1_C
      flag    asm_esc = 0;      /* asm escaped used in file */
LOCAL flag    asm_exit = 0;	/* -W0,-Wa makes asm warning cause exit(1) */
      flag    volatile_flag = 0;/* +OV if nls is enabled */
      extern FILE *outfile;
# endif /* C1_C */

      flag    echoflag;	        /* do we echo input to output? */
      flag    nls;	        /* true if nls is enabled */
LOCAL flag    wide_seen;        /* true if wide characters/strings seen */
LOCAL flag    overflow;         /* indicates integer constant overflow */ 
LOCAL int turn_off_ansi_keywords(); /* disables const,volatile,signed keywds */
#if defined(LINTHACK) || defined(APEX)
      flag    dollarflag = 1;   /* unconditionally accept $ in dff and apex */
#else
#ifdef ANSI
      flag    dollarflag = 0;   /* flag to accept $ as an id character */
#else
      flag    dollarflag = 1;   /* unconditionally accept $ in compatability */
				/* mode */
#endif
#endif

#ifdef LINT
flag no_null_in_init = 0;
extern int argflag; 
extern int vflag;
extern int libflag;
extern int vaflag;
extern int ansilint;
extern int stdlibflag;
extern int notdefflag;
extern int notusedflag;
#endif /* LINT */

#ifndef IRIF
flag strcpy_inline_enabled = TRUE;
extern struct builtin builtins[];
extern int builtins_table_size;
extern int builtin_compare();
#endif /* not IRIF */

LOCAL int gettemp;
LOCAL void lxinit();
LOCAL int lxResWide();
LOCAL void lxtitle();
LOCAL void lxnewline();
LOCAL void lxdirective();
LOCAL char get_non_WS();

/* -------------------------------------------------------------- */
/* source file I/O */
/* -------------------------------------------------------------- */
LOCAL char *source_line = " ";  /* avoid dereferencing NULL ptr in GETCHAR */
LOCAL char *source_line_start;
LOCAL char *source_buffer;
LOCAL int buffersize = BUFFERSIZE;
LOCAL char *cr = "\n";

#ifdef ANSI

#if !defined CXREF && !defined LINT
     extern char *get_output_line();        /* in cpp.ansi */
     extern int is_error;                   /* in cpp.ansi */
#else
     LOCAL char *get_output_line(){return "";}     /* stubs for CXREF & LINT */
     LOCAL int is_error = 0;
#endif  /* stubs for CXREF & LINT */

extern flag cpp_ccom_merge;      /* preprocessor/scanner  merged if true */

#define GETLINE ( (( source_line_start =                                      \
		    ( cpp_ccom_merge ? get_output_line()                      \
		                     : get_next_line( source_buffer,          \
						       buffersize ) ))        \
		   == NULL) ?                                                 \
		              source_line = cr, EOF :                            \
                              (*(source_line = source_line_start))&0377 )

#else /* not ANSI */

#define GETLINE ( (( source_line_start = get_next_line( source_buffer,     \
						        buffersize ))      \
		     == NULL ) ?                                           \
		              source_line = cr, EOF :                         \
                              *(source_line = source_line_start) )

#endif /* ANSI */

#define GETCHAR ( gettemp =                                          \
     ( ( *(++source_line) == NULL ) ? (int) GETLINE : (unsigned char) *source_line ),      \
     ( echopt<echoend ) ? ( *echopt++ = gettemp ) : 0,gettemp )

/*
 *  with line-oriented GETCHAR/UNGETC, multiple UNGETC's are always possible
 *  when the characters do not span lines
 *
 *  [ cpp.ansi splices \<newline> ... ]
 */

#define UNGETC(x) ( echopt--,                                      \
                     ( (x == EOF) ? x :                            \
                        ((source_line < source_line_start) ? EOF : \
                           (*(source_line--) = x) )))

char *expand_source_buffer();
char *get_next_line( buffer, size ) char *buffer; {
     
     /* Set the last read character to a newline for later checking. */
     buffer[size-2] = '\n';
     if(fgets( buffer, size, stdin ) == NULL) {
	  return FALSE;
     }
     /* If last read character is not a newline, 
      * then it was overwritten by an input line that was too long. */
     if( buffer[size-2] != '\n' && buffer[size-2] != '\0' ) {
	  return( expand_source_buffer());
     }
     return( buffer );
}
char *expand_source_buffer() {
     /* expands source_buffer in increments of BUFFERSIZE */

     source_buffer = realloc( (void *)source_buffer, (size_t)(buffersize += BUFFERSIZE) );
     (void)get_next_line( source_buffer+(buffersize-BUFFERSIZE)-1, 
			    BUFFERSIZE+1 );
     return( source_buffer );
}
/* -------------------------------------------------------------- */
/* end source file I/O */
/* -------------------------------------------------------------- */

#ifndef ANSI
#    define LOOKFOR_ASGNOP( x )                                            \
 if( (lxmask[ lxchar+1 ] & LEXWS) ||                                       \
     ((lxchar == '\n' )                                                    \
      ? (++lineno,(echoflag?echo():0),echopt=(&echoline[2]),lxnewline(),1) \
      : 0 ))							           \
     {   							           \
                                                                           \
          lxchar = get_non_WS();                                           \
	  if( lxchar == '=' ) {                                            \
	       yylval.intval = ASG x;                                      \
	       return( ASGNOP );                                           \
	  }                                                                \
     }
#endif /* not ANSI */

#define BITMASK(n) (((n)==SZLONG)? -1L : ((1L << (n)) -1 ))
			/* taken from ESD's definition on manifest */


	/* lexical actions */

# define A_ERR 0		/* illegal character */
# define A_LET 1		/* saw a letter */
# define A_DIG 2		/* saw a digit */
# define A_1C 3			/* return a single character */
# define A_STR 4		/* string */
# define A_CC 5			/* character constant */
# define A_SL 7			/* saw a / */
# define A_DOT 8		/* saw a . */
# define A_PL 9		        /* + */
# define A_MI 10		/* - */
# define A_EQ 11		/* = */
# define A_NOT 12		/* ! */
# define A_LT 13		/* < */
# define A_GT 14		/* > */
# define A_AND 16		/* & */
# define A_OR 17		/* | */
# define A_WS 18		/* whitespace (not \n) */
# define A_NL 19		/* \n */
# define A_ASCK 20              /* saw one of * ^ % */
# define A_DOL 21
# if defined SA && defined ANSI
# define A_CTRL 22              /* saw one of CTRL_A,
				   CTRL_B, CTRL_C */
# endif     

	/* character classes */

# define LEXLET 01
# define LEXDIG 02
# define LEXOCT 04
# define LEXHEX 010
# define LEXWS 020
# define LEXDOT 040
# define LEXDOL 0100
# ifdef SA
# define LEXSLASH 0200
# endif

	/* reserved word actions */

# define AR_TY 0		/* type word */
# define AR_RW 1		/* simple reserved word */
# define AR_CL 2		/* storage class word */
# define AR_S 3		/* struct */
# define AR_U 4		/* union */
# define AR_E 5		/* enum */
# define AR_A 6		/* asm */
# define AR_TQ 7	/* type qualifier */

	/* text buffer */
# define LXTSZ NCHNAM

extern int	newasciz();

char yytext[LXTSZ];
char * lxgcp;

	/* these are added to support echoing input into assembly output */
char	echoline[300] = "#\t	";
char	*echoend = echoline + sizeof(echoline) - 3;
char	*echopt = &echoline[2];

#ifndef IRIF
     extern unsigned int offsz;
     int stderr_piped = 1;         /* 'stderr' piped if set */
#else /* IRIF */
#    define putdatum( a, b ) ( gotscal() )
#endif /* IRIF */

extern FILE *dtmpfile;	/* temp file for debugger symbol table (gntt) */
extern char    *dtmpname;
extern FILE *ltmpfile;	/* temp file for debugger symbol table (lntt) */
extern char    *ltmpname;
extern flag volatile_flag;
flag o2flag;
void display_error(); 
flag display_line = 0;
#define DISPLAY_SIZE 71
char display_text[DISPLAY_SIZE+2];

#ifndef IRIF
process_options(argc,argv) int argc; char *argv[]; {
	register int i;
	register char *cp;

	for( i=1; i<argc; ++i )
	   if( *(cp=argv[i]) == '-' )
	     switch (*++cp)
		{
		case 'Y':
		  while( *++cp )
		     switch( *cp )
			{
#ifdef DEBUGGING

			case 'b':
				++bdebug;
				break;
			case 'd':
				++ddebug;
				break;
			case 'e':
				++edebug;
				break;
#endif	/* DEBUGGING */
			case 'f':	/* 32 bit float ops option */
				++singleflag;
				break;
#ifdef HPCDB
			case 'g':  /* create a cdb symbol table */
				cdbflag = 1;
				dtmpname = tempnam("/tmp","gnt");
				dtmpfile = fopen( dtmpname, "w+" );
				if (dtmpfile == NULL)
				  cerror( "Cannot open temp file" );
				(void)unlink(dtmpname);
				(void)setvbuf(dtmpfile, NULL, _IOFBF, 8192);
				ltmpname = tempnam("/tmp","lnt");
				ltmpfile = fopen( ltmpname, "w+" );
				if (ltmpfile == NULL)
				  cerror( "Cannot open temp file" );
				(void)unlink(ltmpname);
				(void)setvbuf(ltmpfile, NULL, _IOFBF, 8192);
				(void)cdb_init();
				break;
#endif /* HPCDB */

#ifdef DEBUGGING
			case 'i':
				++idebug;
				break;
#endif	/* DEBUGGING */
			case 'k':
				++compiletoassembler;
				break;

			case 'o':	/* +O1 */
				++oflag;
				break;

			case 'O':       /* +O2 */
				++o2flag;
				++oflag;
				break;

                        case 'p':
                                     /* cc sends -Yp profiling flag to
                                      * cpass1, but all work is done by
                                      * cpass2.  Ignore here.
                                      */
                                break;
#ifdef SILENT_ANSI_CHANGE
			case 's':
				/* issue diagnostics about silent
				 * ANSI C changes.  Not certain
				 * if we want this in lint or in
				 * a compiler option or both */
				silent_changes = 1;
				break;
#endif
			case 'S':
				/* passed in if '+s' to 'cc' 
				 * 'stderr_piped' is defaulted *on* so that
				 * error messages associated with bogus 
				 * options before 'stderr_piped' is toggled
				 * are counted as if piped
				 */
				stderr_piped = 0;
				break;
			case 'r':	/* undocumented f.p. rounding modes */
					/* actually a pass 2 flag. KAH has it
					   in the 500 this way, though.
					*/
				++froundflag;
				break;
#ifdef DEBUGGING
			case 't':
				++tdebug;
				break;
			case 'T':
#ifdef XPRINT
			        ++treedebug;
				break;
#endif
#endif	/* DEBUGGING */
			case 'u':
				++unionflag;
				break;
#ifdef DEBUGGING
			case 'x':
				++xdebug;
				break;
#endif	/* DEBUGGING */
#ifdef HPCDB
			case 'A':
			        ++Allflag;
			        break;
# ifdef SA
			case 'X':
			        ++saflag;
				break;
# endif 				
# ifdef SADEBUG				   
			case 'D':
			        ++sadebug;
			     	break;
# endif
#endif					
			case 'w':
				++wflag;
				break;

			case 'E':	/* similar to FOCUS E option */
#ifndef C1_C
				++echoflag;
#endif /* C1_C */
				break;

			case '$':
			        ++dollarflag;
			     	break;

			default:  
				/* "unrecognized option '%s%s' ignored" */
				WERROR( MESSAGE( 220 ), "-Y", cp );
				while (*++cp) /* discard */ ;
				--cp;
				break;

			} 	/* end of -Y sub-options */

			break;	/* end of -Y case */

#ifdef YYCOVERAGE
		case 'R':
			record_reductions++;
			if(*(++cp))
			  yy_rfile = cp;
			else
			  yy_rfile = "Reductions";
			break;	
#endif /* YYCOVERAGE */
		case 'n':
			/* ignore -n for HAIL for now */
			{
			/* enable NLS support */
			nls = 1;
			get_tongue();
			}
			break;

#ifdef MSG_COVERAGE
		case 'm':
			msg_coverage++;
			if(*(++cp))
			   msgdb_filename = cp;
			break;
#endif /* MSG_COVERAGE */

		case 'M':	/* no in-line 68881 */
#ifdef C1_C
			flibflag = 1;
#endif  /* C1_C */
			break;

#define MINTABSZ 40
                case 'N':       /* table size resets */
                        {
                        char *lcp = ++cp;
                        int k = 0;
                        while ( (*++cp >= '0') && (*cp <= '9') )
                          k = 10*k + (*cp - '0');
                        --cp;
                        if (k <= MINTABSZ)
			  {
			  /* simple-minded check */
			  /* "small table size of '%d' specified 
			     for '%c' ignored" */
			  WERROR( MESSAGE( 219 ), k, *lcp );
			  k = 0;
			  }
			if (k) resettablesize(lcp, k);
                        break;
                        }


                case 'b':
#ifdef C1_C
                        fpaflag = 1;    /* non-LCD only */
#endif  /* C1_C */
                        break;

		case 'c':
			compatibility_mode_bitfields = 1;
			break;

                case 'f':
#ifdef C1_C
                        fpaflag = -1;   /* non-LCD only */
#endif  /* C1_C */
                        break;

		case 'F':
			inlineflag = 1;	/* turn on inlining */
		        oflag++;        /* and additional FE optimizations */
			break;

		case 'i':
			/* use word offset <ea> forms */
			picoption = 1;
			break;

		case 'I':
			/* use long offset <ea> forms */
			picoption = 2;
			break;

#ifdef FORTY
		case 'q':
			fortyflag = 1;
			break;
#endif /* FORTY */

		case 'V':
#ifdef  C1_C
		        volatile_flag = 1;
#endif  /* C1_C */
		        break;

#ifdef NEWC0SUPPORT
#ifndef ONEPASS
		case 'O':
			c0flag = 1;
			break;
#endif /* ONEPASS */
#endif /* NEWC0SUPPORT */

		case 'W':
			/* -W0,-Wa turns asm warnings into exit(1)'s
			 * so the commands folks can more easily
			 * weed out those cases */
			++cp;
			if (*cp == 'a')
				{
#ifdef C1_C
				asm_exit = 1;
#endif /* C1_C */
				}
#ifdef ANSI
			/* -W[c0],-We enables asms in ANSI mode */
			else if (*cp == 'e')
				ansiflag = 1; 
#endif
			/* -Wc,Ws makes bitfields signed by default
			   -Wc,Wu makes bitfields unsigned by default */
			else if (*cp == 's')
				signed_bitfields = 1;
			else if (*cp == 'u')
				signed_bitfields = 0;
			/* -Wc,-Wk turns off recognition of
			   const, volatile and signed keyworks */
			else if (*cp == 'k')
				turn_off_ansi_keywords();
			break;
		/* Pass 2 options */
		case 'l':
		case 'p':
			break;

#ifdef DEBUGGING
		/* Pass 2 debugging options */
		case 'a':
		case 'd':
		case 'e':
		case 'h':
		case 'o':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'x':
		case 'E':
			break;
#endif /* DEBUGGING */

#ifndef LINT_TRY
	 	default:  
			/* "unrecognized option '%s%s' ignored" */
			WERROR( MESSAGE( 220 ), "-", cp );
#endif /* !LINT_TRY */
		}       /* End of option switch */
}
#endif /* not IRIF */

	/* ARGSUSED */
mainp1( argc, argv ) int argc; char *argv[]; {  /* control multiple files */

	register int i;
	register char *cp;

#ifndef IRIF
	offsz = 0xfffffffe;	/* max offset and != NOOFFSET
				 *   used to call caloff() */
#endif /* IRIF */

	/* allocate buffer between the preprocessor and the scanner 
	   (unmerged case) */
#ifdef ANSI
	if( !cpp_ccom_merge )
	     source_buffer = malloc( BUFFERSIZE );
#else
	source_buffer = malloc( BUFFERSIZE );
#endif /* ANSI */

#ifdef IRIF
	ir_options(argc,argv);
#else
	process_options(argc,argv);
#endif

	optlevel = ccoptlevel = inlineflag ? 3 :
	                        o2flag ? 2 : 
				oflag ? 1 : 
				0 ;
#ifdef IRIF
# ifdef HPCDB
# ifdef SA
	ir_initialize( optlevel, cdbflag, argc, argv, saflag);
# else /* not SA */
	ir_initialize( optlevel, cdbflag, argc, argv, 0);
# endif /* not SA */
# else /* not HPCDB */
	ir_initialize( optlevel, 0, argc, argv, 0);
# endif /* not HPCDB */
#endif /* IRIF */

#ifndef IRIF
#ifdef C1_C
	putmaxlab();
#endif  /* C1_C */
#endif /* IRIF */

# ifdef ONEPASS
	(void)p2init( argc, argv );		/* in reader.c */
# else /* ONEPASS */
	if (picoption && fpaflag) minrvarb = MINRVAR+2;
	else if (picoption || fpaflag) minrvarb = MINRVAR+1;
# endif /* ONEPASS */

	newasciz(0);			/* allocate asciz table - in common */

	lineno = 1;

#ifdef LIPINSKI
	naliasinit();			/* initialize the alias table */
#endif			
# ifdef SA
	if (saflag) 
	  {
	    satitle = ftitle;
	    /* Need to setup the xt tables */
	    init_xt_tables(); 
# ifdef SADEBUG
	    if (sadebug) {
	       (void)signal(SIGSEGV,sig_handler);
	       (void)signal(SIGBUS,sig_handler);
	    }
# endif	    
	  }
# endif

# ifdef HPCDB
	if (cdbflag)
	  {
	  /* This forces out all debug info even if the item */
	  /* is not referenced, when compiling a .i file     */
	  in_comp_unit = 1;
	  cdb_module_begin();
	  }
# endif
#ifdef LINT_TRY
	if (!ansilint) dollarflag = 1;
#endif

	lxinit();  /* for IRIF, 'lxinit()' must follow 'cdb_module_begin()' */

# ifndef ONEPASS
	mkdope();			/* in common */
#endif
	inittabs();
	inittaz();			/* init expandable mode and asciz tables */
	tinit();			/* expression tree init - in common */

	symtab_init();			

	/* dimension table initialization */

	dimtab[NULL] = 0;
	dimtab[CHAR] = SZCHAR;
	dimtab[INT] = SZINT;
	dimtab[FLOAT] = SZFLOAT;
	dimtab[DOUBLE] = SZDOUBLE;
	dimtab[LONG] = SZLONG;
	dimtab[SHORT] = SZSHORT;
	dimtab[UCHAR] = SZCHAR;
	dimtab[USHORT] = SZSHORT;
	dimtab[UNSIGNED] = SZINT;
	dimtab[ULONG] = SZLONG;
	dimtab[LONGDOUBLE] = SZLONGDOUBLE;
	dimtab[SCHAR] = SZSCHAR;
	/* starts past any of the above */
	curdim = 19;
	reached = 1;
# ifdef LINT_TRY
	reachflg = 0;
# endif
#if defined(ANSI) && !defined(LINT)
	if (!ansiflag) turn_off_asms();
#endif
	ftnend();

#ifdef YYCOVERAGE
        if(record_reductions)
            yy_init_rtable( yy_rfile, yy_rtable); /* init table */
#endif /* YYCOVERAGE */

	(void)yyparse();			/* in cgram.c */

#ifdef YYCOVERAGE
       if(record_reductions)
           yy_update_rfile( yy_rfile, yy_rtable);
#endif /* YYCOVERAGE */

	yyaccpt();			/* in pftn.c */
# ifdef HPCDB
	if (cdbflag) {
		cdb_global_names();
		cdb_module_end();
	}
# endif
	
#ifdef MSG_COVERAGE
	if( msg_coverage ) update_msgdb();
#endif /* MSG_COVERAGE */

#ifdef IRIF
	ir_terminate();
#else /* not IRIF */
#ifndef OSF
	stderr_signoff();
#endif /* OSF */
#endif /* not IRIF */

#ifdef LINT_TRY
	ejobcode( nerrors ? 1 : 0 );
#endif

#ifdef ANSI
	if( cpp_ccom_merge )
	     return( ( nerrors || is_error ) ? 1:0 );
#endif
	return(nerrors?1:0);

	}


# define CSMASK 0377
# define CSSZ 256


short lxmask[CSSZ+1];

LOCAL lxenter( s, m ) register char *s; register short m; {
	/* enter a mask into lxmask */
	register c;

	while( c= *s++ ) lxmask[c+1] |= m;

	}


# define lxget(c,m) (lxgcp=yytext,lxmore(c,m))

LOCAL lxmore( c, m )  register c; register short m; {
	register char *cp;
#ifndef __lint
	register int gettemp;	/* quicker than the static */
#endif
	flag toolong = FALSE;

	*(cp = lxgcp) = c;
	while( c=GETCHAR, lxmask[c+1]&m ){
		if( cp < &yytext[LXTSZ-1] )
			*++cp = c;
		else
			toolong = TRUE;
		}
	(void)UNGETC(c);
	*(lxgcp = cp+1) = '\0';
        if(toolong)
                WERROR(MESSAGE(238));
	}

struct lxdope {
	short lxch;	/* the character */
	short lxact;	/* the action to be performed */
	short lxtok;	/* the token number to be returned */
	short lxval;	/* the value to be returned */
	} lxdope[] = {

	'\\',	A_ERR,	0,	0,	/* illegal characters go here... */
	'_',	A_LET,	0,	0,	/* letters point here */
	'0',	A_DIG,	0,	0,	/* digits point here */
	' ',	A_WS,	0,	0,	/* whitespace goes here */
	'\n',	A_NL,	0,	0,
	'"',	A_STR,	0,	0,	/* character string */
	'\'',	A_CC,	0,	0,	/* character constant */
	'`',	A_ERR,	0,	0,
	'(',	A_1C,	LP,	0,
	')',	A_1C,	RP,	0,
	'{',	A_1C,	LC,	0,
	'}',	A_1C,	RC,	0,
	'[',	A_1C,	LB,	0,
	']',	A_1C,	RB,	0,
#ifdef APEX
	'#',	A_1C, 	SHARP,	0,
#endif
	'*',	A_ASCK,	MUL,	MUL,
	'?',	A_1C,	QUEST,	0,
	':',	A_1C,	COLON,	0,
	'+',	A_PL,	PLUS,	PLUS,
	'-',	A_MI,	MINUS,	MINUS,
	'/',	A_SL,	DIVOP,	DIV,
	'%',	A_ASCK,	DIVOP,	MOD,
	'&',	A_AND,	AND,	AND,
	'|',	A_OR,	OR,	OR,
	'^',	A_ASCK,	ER,	ER,
	'!',	A_NOT,	UNOP,	NOT,
	'~',	A_1C,	UNOP,	COMPL,
	',',	A_1C,	CM,	CM,
	';',	A_1C,	SM,	0,
	'.',	A_DOT,	STROP,	DOT,
	'<',	A_LT,	RELOP,	LT,
	'>',	A_GT,	RELOP,	GT,
	'=',	A_EQ,	ASSIGN,	ASSIGN,
	'$',	A_ERR,	0,	0,
#if defined SA && defined ANSI	
	CTRL_A, A_CTRL, 0,      0,
	CTRL_B, A_CTRL, 0,      0,
	CTRL_C, A_CTRL, 0,      0,
#endif	
	-1,	A_1C,	0,	0,
	};

struct lxdope *lxcp[CSSZ+1];

LOCAL void lxinit(){
	register struct lxdope *p, **pp;
	register char *cp;

	/* set up character classes */
	lxenter( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_", LEXLET );
	lxenter( "0123456789", LEXDIG );
	lxenter( "0123456789abcdefABCDEF", LEXHEX );
	lxenter( " \t\r\b\f\v\007", LEXWS );     /* \007 is alert - \a */
	lxenter( "01234567", LEXOCT );
	lxenter( "$", LEXDOL );
	lxmask['.'+1] |= LEXDOT;
# ifdef SA
	lxmask['/'+1] |= LEXSLASH;
# endif

	/* make lxcp point to appropriate lxdope entry for each character */

	/* initialize error entries */

	for (pp = &lxcp[CSSZ+1]; pp >= lxcp; )
		*--pp = lxdope;

	/* make unique entries */

	for( p=lxdope; ; ++p ) {
		lxcp[p->lxch+1] = p;
		if( p->lxch < 0 ) break;
		}
	if (dollarflag) 
	  lxcp['$'+1]->lxact = A_DOL; /* change '$' act to A_DOL from A_ERR */

	/* handle letters, digits, and whitespace */
	/* by convention, first, second, and third places */

	cp = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[1];
	cp = "123456789";
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[2];
	cp = "\t\b\r\f\v\007";     /* \007 is \a */
	while( *cp ) lxcp[*cp++ + 1] = &lxdope[3];

	/* first line might have title or directive */
	lxnewline();

      }

char lxmatch;  /* character to be matched in char or string constant */

LOCAL char *lxescape( c, hexbytes, cp ) register c;
	                                int hexbytes; /* max bytes in representable
							 hex char constant */
	                                char *cp; { 
	/* translates escape sequences to character values 
	 *      [at most hexbytes are translated for hex escape sequences],
	 * deposits the value at *cp and returns a pointer to first unused
	 */
	  switch( c ) {
			case 'n':
	                        *cp++ = '\n';
				return( cp );

			case 'r':
				*cp++  = '\r';
				return( cp );

			case 'b':
				*cp++ = '\b';
				return( cp );

			case 't':
				*cp++ = '\t';
				return( cp );

			case 'f':
				*cp++ = '\f';
				return( cp );

			case 'v':
				*cp++ = '\v';
				return( cp );

			case 'a':
#ifndef ANSI
				/* "undefined escape sequence: \\%s treated as %s" */
				WERROR( MESSAGE( 205 ), 'a', "\\007" );
#endif
				*cp++ = '\007';
				return( cp );

			case 'x': {
				/* hex character constant 
				 * overflow if more than SZLONG/4 hex characters
				 * if overflow, retain least significant
				 */
			  int i;
			  int bytes;
			  int size = 0;
			  flag still_zero = 1;
#ifndef ANSI
			  int leading_zeroes = 0;
#endif

			  lastcon = 0;

			  /* process hex constant */
			  while( ( c = GETCHAR ), ( lxmask[c+1] & LEXHEX )) {
			       if( still_zero )
				    if( c == '0') { still_zero ++; continue; }
			            else {
#ifndef ANSI
					 leading_zeroes = still_zero - 1;
#endif
					 still_zero = 0;
				    }
			       size++;
			       lastcon <<= 4;
			       if( isdigit( c ) ) lastcon += c - '0';
			       else if( isupper( c ) ) lastcon += c - 'A'+ 10;
			       else lastcon += c - 'a'+ 10;
			     }
			  (void)UNGETC( c ); 

			  if( still_zero ) {
			       bytes = 1;
			       if( still_zero == 1 ) 
			            /* "empty hex escape sequence" */
			            UERROR( MESSAGE( 139 ) );
			     }
			  else bytes = (size/2) + (size%2);
#ifndef ANSI
			  if( ( size + leading_zeroes ) > 2 ) {
			       display_line++;
			       /* "hex escape sequence contains more than two digits" */
			       WERROR( MESSAGE( 207 ));
			  }
#endif
			  if( bytes > hexbytes ) {
			       display_line++;
			       /* "%s constant too large to represent" */
			       WERROR( MESSAGE( 138 ), "hex character");
			       bytes = hexbytes;
			     }
			  /* transfer lastcon bytes to *cp */

			  for( i = bytes-1; i >= 0; i-- ) 
			       *cp++ = (char)( lastcon >> (i*SZCHAR) );
			  return( cp );
			}

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':  {
				*cp = c-'0';
				c=GETCHAR;  /* try for 2 */
				if( lxmask[c+1] & LEXOCT ){
					*cp = (*cp<<3) | (c-'0');
					c = GETCHAR;  /* try for 3 */
					if( lxmask[c+1] & LEXOCT ){
					        if( *cp & 040 ) { /* overflow imminent */
						     display_line++;
				                     /* "%s  constant too large to represent" */
				                     WERROR( MESSAGE( 138 ), "octal character");
						}
						*cp = (*cp<<3) | (c-'0');
						}
					else (void)UNGETC( c );
					}
				else (void)UNGETC( c );

				return( ++cp );
			      }

			case '?':
#ifndef ANSI
				/* "undefined escape character: \\%c treated as %c" */
				WERROR( MESSAGE( 206 ), '?', '?' );
#endif
				*cp++ = c;
				return( cp );

			default:
				/* "undefined escape character: \\%c treated as %c" */
				WERROR( MESSAGE( 206 ), c, c );
				/* fall thru */

			case '\\':
			case '\"':
			case '\'':
				*cp++ = c;
				return( cp );

			}  /* end switch */
	} /* end lxescaspe */

#ifndef IRIF

# define lxEndStr( count, limit, type)                                  \
	 if( is_initializer ){ /* end initializer */                    \
		 if( limit == 0 || count < limit )                      \
		      putdatum( 0, type );  /* the null at the end */   \
		 }                                                      \
	 else {  /* the string gets a null byte */                      \
		 bycode( 0, count++, type, 1 );                         \
		 /* in case of later sizeof ... */ \
		 if (inlineflag) dstash(count); else dimtab[curdim] = count; \
		 }
#else /* IRIF */

# define lxEndStr( count, limit, type)                                  \
	 if( is_initializer ){ /* end initializer */                    \
		 if( limit == 0 || count < limit )                      \
		      putdatum( 0, type );  /* the null at the end */   \
		 }                                                      \
	 else {  /* the string gets a null byte */                      \
		 bycode( 0, count++, type, 1 );                         \
		 dimtab[curdim] = count;  /* in case of later sizeof ... */ \
		 }

#endif /* IRIF */

lxstr(ct) register ct;
	{
	/* match a string or character constant, up to lxmatch */

	/*
	 * modified for NLS : HP-15 processing
	 */

	register c;
	register val;
	register i;
#ifndef __lint
	register int gettemp;		/* quicker than the static global */
#endif
	flag second = 0;
        flag quote_check = 1;           /* Added for SWFfc00660 */
        int j;                          /* Added for SWFfc00660 */

#if defined SA && defined ANSI
        c =  GETCHAR;                  /* Initialize c  CLL4600041       */
        (void)UNGETC( c );
#endif  /* if SA && ANSI */


	i=0;
        do{
#if defined SA && defined ANSI 
          quote_check = 1;                     /* Added for SWFfc00660 */
          switch(lxcp[(c)+1]->lxact){          /* DTS#FSDdt-8973 */
                case A_CTRL:
                        /* a control character from cpp.ansi */
                        {
                        char *linep,*ctl_beg;
                        flag count_on;
                        int col;
                        col = (int) (source_line - source_line_start) + 1;
                        linep = source_line ; 
                        while(*linep)
                        {
                          switch (*linep)
                          {
                          case CTRL_A:
                               strcpy(linep,linep+1);
                               count_on = FALSE;
                               break;

                          case CTRL_B:
                               ctl_beg = linep;
                               while ((*++linep!=CTRL_C) && (*linep!='\0')) ;
                               if (*linep != CTRL_C)
                                       cerror("bad macro marking");
                               *linep++ = '\0';
                               do_Scanner_macro_use(
                                          ctl_beg,cdbfile_vtindex,lineno,col);
                                col += strlen(ctl_beg);
                                strcpy(ctl_beg,linep);
                                linep = ctl_beg;
                                count_on = TRUE;
                                break;
                          case  CTRL_C:
                                ctl_beg = linep;
                                while ((*++linep!=CTRL_C) && (*linep!='\0')) ;
                                if (*linep != CTRL_C)
                                        cerror("bad macro marking");
                                *linep++ = '\0';
                                do_Scanner_macro_define(ctl_beg+1,
                                                 cdbfile_vtindex,lineno,col);
                                col += strlen(ctl_beg);
                                strcpy(ctl_beg,linep);
                                linep = ctl_beg;
                                count_on = TRUE;
                                break;

                          default:
                                ++linep;
                                if (count_on) col++;
                          }  /* switch(*line) */
                        } /* while(*linep) */
                        /* SWFfc00660 fix - Scan backwards until a " is found
                           or the limit is reached. If a " was found then 
                           update source_line. This prevents the next token
                           from being overwritten. */
                        if (i) { 
                            source_line--;
                            col = strlen (source_line_start) - 
                                          strlen (source_line);

                            for (j = 0; source_line[-j] != lxmatch && 
                                          j < col; j++);
                            if (source_line[-j] == lxmatch) {
                                for (;source_line[0] != lxmatch;source_line--);
                               source_line--; 
                              }
                            else quote_check = 0;
                           }
                        else
                            c = get_non_WS();
                        break;
                        }/* Control Character from cpp.ansi  */
          } /* switch(lxmatch) */
#endif /* if SA && ANSI */
          /* SWFfc00660 fix - added quote_check to by pass the while loop */
	  while( (quote_check && (c=GETCHAR) != lxmatch) ){
		switch( c ) {

		case EOF:
			/* "unexpected EOF" */
			UERROR( MESSAGE( 113 ) );
			break;

		case '\n':
			/* "newline in string or char constant" */
			UERROR( MESSAGE( 78 ) );
			++lineno;
			continue;

		case '\\': { 
			if( ( c = GETCHAR ) == '\n' ) {
				++lineno;
				if (echoflag) echo();
				echopt = & echoline[2];
				continue;
			      }
			val = *( ( lxescape( c, 1, yytext )) - 1 );
		        goto mkcc;
                     }
		default:
			val = c;
	       
mkcc:

			if( lxmatch == '\'' ){
				val = CHARCAST(val);  /* it is, after all, a "character" constant */
				if (i==0) lastcon = val;
				else
					{
					lastcon <<= SZCHAR;
					lastcon |= (val & ((1<<SZCHAR)-1) );
					}
				}
			else { /* stash the byte into the string */
#ifdef LINT_TRY
				if (val=='?') 
				    if ((c=GETCHAR)=='?') 
				    {
				    char tri;

					i += 2;
					c = GETCHAR;
					switch (c) {
					    case '=': tri = '#'; break;
					    case '(': tri = '['; break;
					    case '/': tri = '\\'; break;
					    case ')': tri = ']'; break;
					    case '\'':tri = '^'; break;
					    case '<': tri = '{'; break;
					    case '!': tri = '|'; break;
					    case '>': tri = '}'; break;
					    case '-': tri = '~'; break;
					    case '"': UNGETC(c); 
						/* fall through */
					    default:  tri = '\0'; break;
					    }
					if (hflag && !ansilint && (tri!='\0'))
						/* trigraph sequence '??... */
						WERROR( MESSAGE(237), c, tri );
				    }
				else UNGETC(c);
#endif /* LINT */
				if( is_initializer ) {
#ifndef LINT
					if( ct==0 || i<ct ) {
#ifndef IRIF
					     putdatum( val, 0 );
#else /* IRIF */
					     irif_string_buf[i] = val;
#endif /* IRIF */
					}
					/* "excess values in string initializer ignored" */
					else if( i == ct ) WERROR( MESSAGE( 175 ) );
#else  /* LINT */
					if( i==ct && ct!=0 ) WERROR( MESSAGE( 175 ) );
					/* "excess values in string initializer ignored" */
					else if( i==(ct-1) ) no_null_in_init=1;
#endif  /* LINT */
					}
			     else {
#ifdef IRIF
				  if( i >= SZ_IR_MAX_BUFLEN ) {
				       uerror( "max IRIF string initializer size (%d) exceeded", 
					      SZ_IR_MAX_BUFLEN );
				       return( 0 );
				  }
				  irif_string_buf[ i ] = val;
#else /* not IRIF */
				  bycode( val & 0377, i, 0 , 0 );
#endif /* not IRIF */
			     }
			}
			++i;

			/*
			 * HP-15 support:
			 * check if c is the first byte of 16 bit character 
			 */
#ifndef HAIL
#ifndef OSF
			if (nls && lxmatch == '"')
			{
			  if (!second && FIRSTof2(c&0377))
			  {
				  c = (GETCHAR) & 0377;
				  if (c==EOF)
				  {
					  /* "unexpected EOF" */
					  UERROR( MESSAGE( 113 ) );
					  break;
				  }
				  if (iscntrl(c)|| isspace(c))
				       /* "(NLS) illegal second byte 
					   in 16-bit character" */
				       UERROR( MESSAGE( 134 ));
				  second = 1;
				  val = c;
				  goto mkcc;
			  }
			  second = 0;
			}
#endif /* OSF */
#endif /* HAIL */
			continue;
		      } /* end switch */
		break;
	      }  /* end while */

	/* DO statement condition - handles concatenation of string literals */
        } /* DO - WHILE */
#if defined SA && defined ANSI
	while( ( lxmatch == '"' ) && (  c = get_non_WS(),
                                       ((c == lxmatch)||
                                       ( (lxcp[(c)+1]->lxact) == A_CTRL ))
                                        ? 1
                                        : 0  ) ); /* DTS#FSDdt-8973 */
#else /*defined SA && defined ANSI*/
	while( ( lxmatch == '"' ) && ( ( c = get_non_WS() ) == lxmatch   ) );
#endif /* if SA && ANSI */

	/* end of string or  char constant */

	if( lxmatch == '"' ) { 
	     (void)UNGETC( c );      /* return character retrieved at DO exit  */
#ifdef IRIF
	     irif_string_buf[ i ] = '\0';
	     dimtab[curdim] = i+1;	/* for later sizeof */
#else /* not IRIF */
	     lxEndStr( i, ct, 0 )
#endif /* not IRIF */
	     }
	else { /* end the character constant */
	       if( i== 0 ) {
		     display_line++;
		     /* "empty character constant" */
		     UERROR( MESSAGE( 36 ) );
		}
		if (i>1)
		    if( i>(SZINT/SZCHAR) || pflag || hflag ) {
			 display_line++;
			 /* "too many characters in character constant" */
			 UERROR( MESSAGE( 107 ) );
		    }
		    else {
			 display_line++;
			 /* "too many characters in character constant" */
			 WERROR( MESSAGE( 107 ) );		      
		    }
	      }
	return( i );
      }

LOCAL lxcom(){
	register c;
#ifndef __lint
	register int gettemp;		/* quicker than the static global */
#endif
	/* saw a /*: process a comment */

	/*
	 * modified for NLS : HP-15 processing
	 */

#ifdef APEX
	/* Require the APEX keyword to appear first to avoid all of the lxapex()
	 * parsing errors if "APEX" just happens to be used in a real comment.
	 */
	c = GETCHAR;
	while (c==' ' || c=='\t' || c=='\n') {
	    if (c=='\n')
		lineno++;
	    c = GETCHAR;
	}
	if( c == 'A' ) {
	    lxget( c, LEXLET );
	    if( apex_flag && (strcmp( yytext, "APEX" ) == 0) ) {
		lxapex();
	    } else if( !strcmp( yytext, "ARGSUSED" ) ) {
		argflag = 1;
		vflag = 0;
	    }
	}
	UNGETC(c);
#endif

	for(;;){

		switch( c = GETCHAR ){

		case EOF:
			/* "unexpected EOF" */
			UERROR( MESSAGE( 113 ) );
			return;

		case '\n':
			++lineno;
			if (echoflag) echo();
			echopt = & echoline[2];

		default:
			/*
			 * HP-15 support:
			 * check if c is the first byte of 16 bit character 
			 */
#ifndef HAIL
#ifndef OSF
			if (nls && FIRSTof2(c&0377))
			{
				c = (GETCHAR) & 0377;
				if (c==EOF) 
				{
				/* "unexpected EOF" */
				UERROR( MESSAGE( 113 ) );
				return;
				}
				if (iscntrl(c) || isspace(c))
				     /* "(NLS) illegal second byte 
					 in 16-bit character" */
				     UERROR( MESSAGE( 134 ));
			}
#endif /* OSF */
#endif /* HAIL */
			continue;

		case '*':
			if( (c = GETCHAR) == '/' ) return;
			else (void)UNGETC( c );
			continue;

# ifdef LINT
		case 'V':
			lxget( c, LEXLET|LEXDIG );
			{
				int i;
				i = yytext[7]?yytext[7]-'0':0;
				yytext[7] = '\0';
				if( strcmp( yytext, "VARARGS" ) ) continue;
				vaflag = i;
				continue;
				}
		case 'L':
			lxget( c, LEXLET );
			if( !strcmp(yytext,"LINTLIBRARY") ){
			    libflag = 1;
			    vflag = 0;
			} 
#ifdef LINT_TRY
			  else if ( !strcmp(yytext,"LINTSTDLIB") ){
			    libflag = 1;
			    stdlibflag = 1;
			    vflag = 0;
			    fcomment(0,1);
			    notusedflag = 1;
			}
#endif
			continue;

		case 'A':
			lxget( c, LEXLET );
			if( !strcmp( yytext, "ARGSUSED" ) ) 
			{
				argflag = 1;
				vflag = 0;
				}
			continue;

		case 'N':
			lxget( c, LEXLET );
			if( !strcmp( yytext, "NOTREACHED" ) ){
				reached = 0;
# ifdef LINT_TRY
				reachflg = 1;
# endif
			} 
#ifdef LINT_TRY
 			  else if (!strcmp( yytext, "NOTUSED" )){
				fcomment(0,1);
				notusedflag = 1;
#ifdef OSF
				/* patch for OSF lint libraries which
				   don't defined LINTLIBRARY flag
				   turn it on anyway if we can tell */
				if (notdefflag) libflag = 1;
#endif
			} else if (!strcmp( yytext, "NOTDEFINED" )){
			        fcomment(1,1);
				notdefflag = 1;
#ifdef OSF
				/* patch for OSF lint libraries which
				   don't defined LINTLIBRARY flag
				   turn it on anyway if we can tell */
				if (notusedflag) libflag = 1;
#endif
			}
#endif	/* LINT_TRY */
			continue;
# endif
			}
		}
	}



#ifdef APEX
apex_bad(s)
  char *s;
{
	if (libflag)
	    fprintf(stderr, "(%d) warning: invalid APEX directive (ignored): %s\n",
		lineno, s);
}


#define MAX_HINT_LEN	4096

lxapex() {
    long origin, stds[NUM_STD_WORDS], first[NUM_STD_WORDS];
    long dummy[NUM_STD_WORDS];
    int hint;
    int detail_min, detail_max;
    register int c;
    int origin_search;

	hint = FALSE;
	origin = 0;
	std_zero(stds);
	detail_min = 1;
	detail_max = 255;
	c = get_non_WS();
	lxget( c, LEXLET );
	if( strcmp( yytext, "HINT") == 0 ) 
	    hint = TRUE;
	else if( strcmp( yytext, "STD" ) == 0 ) 
	    /* nothing special - APEX STD detected */;
	else {
	    apex_bad("expected HINT|STD");
	    UNGETC(c);
	    return;
	}

	c = get_non_WS();
	if( c != '[' ) {
	    apex_bad("expected \'[\'");
	    (void)UNGETC(c);
	    return;
	}
	c = get_non_WS();
	lxget( c, LEXLET|LEXDIG|LEXDOT );	
	if( c == '*' ) {
	    set_bit(first, ALL_STD_BITS);
	    /* make sure this wasn't the beginning of a comment terminator */
	    c = GETCHAR;
	    if (c=='/') {
		apex_bad("incomplete directive");
		UNGETC('/');
		UNGETC('*');
		return;
	    }
	    (void)UNGETC(c);
	} else if (c != ']' && c != '\n' && c != EOF) {
		origin_search = get_bit(dummy, yytext, origin_list);
		origin = dummy[0];
		if(get_std_num(first, yytext, target_list)==-1 && 
				origin_search==-1)
			apex_bad("bad list");
	}
	c = get_non_WS();
	if (c == '-') {
	    c = GETCHAR;
	    if (c == '>') {
		c = get_non_WS();
		lxget( c, LEXLET|LEXDIG|LEXDOT );
	    } else {
		apex_bad("expected \'>\'");  
		UNGETC(c);
		return;
	    }
	} else {
	    origin = 0;
	    std_cpy(stds, first);
	}
	while (c != ']' && c != '\n' && c != EOF) {
	    if( c == ',' ) {
		c = get_non_WS();
		lxget( c, LEXLET|LEXDIG|LEXDOT );
		continue;
	    } else if( c == '*' ) {
		set_bit(dummy, ALL_STD_BITS);
		/* check for comment terminator */
		c = GETCHAR;
		if (c=='/') {
		    apex_bad("expected \']\'");
		    UNGETC('/');
		    UNGETC('*');
		    return;
		}
		(void)UNGETC(c);
	    } else if( get_std_num( dummy, yytext, target_list) == -1 ) {
		apex_bad("unrecognized standard name");
		return;
	    }
	    std_or(stds, dummy, stds);
	    c = get_non_WS();

	}

	if (c != ']') {
	    apex_bad("expected \']\'");
	    UNGETC(c);
	    return;
	}

	c = get_non_WS();
	if (c == '[') {
	    c = get_non_WS();
	    detail_min = c - '0';
	    if (detail_min < 0 || detail_min > 9) {
		apex_bad("invalid min detail level");
		UNGETC(c);
		return;
	    }
	    c = get_non_WS();
	    if (c == '-') {
		c = get_non_WS();
		detail_max = c - '0';
		if (detail_max < 0 || detail_max > 9 || detail_max < detail_min)
		{
		    UNGETC(c);
		    apex_bad("invalid max detail level");
		    return;
		}
		c = get_non_WS();
	    }
	    if (c != ']') {
		UNGETC(c);
		apex_bad("expected \']\'");
		return;
	    }
	} else UNGETC(c);

	if( hint ) {
	    char hint_text[MAX_HINT_LEN];
	    int hp = 0;

	        c = get_non_WS();

		while ( TRUE ) {
		    while ( c != '*' && c != EOF) {
			if (hp < MAX_HINT_LEN)
			    hint_text[hp++] = c;
			if (c == '\n')
			    lineno++;
			c = GETCHAR;
		    }
		    c = GETCHAR;
		    if( c == '/' ) {
			/* strip white space from end of hint, so that
			 * moving the close-comment to the next line in
			 * the directive doesn't produce inconsistent
			 * output formatting
			 */
			hp--;		/* back up to last filled location */
			while (isspace(hint_text[hp]) )
			    hp--;
			hint_text[hp+1] = '\0';
			outstd(REC_HINT, origin, stds, detail_min, detail_max, hint_text);
		/* unstack the end-of-comment, so lxcom will terminate */
			UNGETC('/');
			UNGETC('*');
			return;
		    } else {
			hint_text[hp++] = '*';
		    }
		}

	} else  {
	    outstd(REC_STD, origin, stds, detail_min, detail_max, "");
	}
}
#endif

/* determine type for integral constants - int || unsigned */
#ifdef ANSI
#define lxhexoctype                                              \
	(   !( lastcon & ~MSKINT )           ? INT               \
	 :                                     UNSIGNED )
#else
#define lxhexoctype	INT
#endif
 
#define lxdectype lxhexoctype

LOCAL lxintsfx( type ) register type; {  /*  check for suffixes on integer
					     constants & reconcile with first-
					     fit type  */
      register c;
      
      switch ( c = GETCHAR ) {

           case 'u':
	   case 'U':
		   if( ( c = GETCHAR ) != 'l' && c != 'L' )
		        (void)UNGETC( c );  
		   /* L suffix ignored */
		   return( UNSIGNED );

   
           case 'l':
           case 'L':
	           if( ( c = GETCHAR ) == 'u' || c == 'U' )
		        return( UNSIGNED ); 
		   /* L suffix ignored */
		   break;

           default: 
#ifdef SILENT_ANSI_CHANGE
		   if (silent_changes && (lastcon & ~MSKINT))
		       werror("unsuffixed integer constant has type 'long' in compatibility mode and 'unsigned' in ANSI mode");
#endif
		   break;
		 }
      (void)UNGETC( c );
      return( type );
    }

#ifdef DOMAIN
yylex() { int silent=0;	/* Domain yacc doesn't support -Xp./yaccpar */
#else
yylex(silent) int silent; {
#endif
        /* silent, an additional parameter added to yylex to turn off lex
	 * errors when the parser is resyncronizing.  */
	for(;;){

		register lxchar;
		register struct lxdope *p;
		register struct symtab *sp;
#ifndef __lint
		register int gettemp;		/* quicker than the static global */
#endif

		switch( (p=lxcp[(lxchar=GETCHAR)+1])->lxact ){

onechar:
			(void)UNGETC( lxchar );

		case A_1C:
			/* eat up a single character, and return an opcode */

			yylval.intval = p->lxval;
			return( p->lxtok );

		case A_DOL: /* $ not allowed as first char of id */
		case A_ERR:
incorrect_character:
			if (silent) continue;
			if( isprint( lxchar ) ) {
			     /* "incorrect character"" */
			     display_line++;
			     UERROR( MESSAGE( 103 ));
			}
			else {
			     /* "incorrect character: 0x%x" */
			     UERROR( MESSAGE( 51 ), lxchar );
			}
			continue;

		case A_LET:
			/* collect an identifier, check for reserved word and
			   wide character/string, and return */
			if (dollarflag)
			  lxget( lxchar, LEXLET|LEXDIG|LEXDOL );
			else
			  lxget( lxchar, LEXLET|LEXDIG );
			if( (lxchar=lxResWide()) > 0 ) return( lxchar ); /* reserved word */

			switch( lxchar ) {

			case 0:   continue;
#ifdef ANSI
			case -2:  return( lxWideChr() );
			case -3:  return( WIDESTRING );
#else /* not ANSI */
			case -2:
			  /* "wide characters supported in ANSI mode only "*/
			  if (!silent) 
			       UERROR( MESSAGE( 202 ));
			  goto char_const;
			case -3:
			  /* "wide characters supported in ANSI mode only "*/
			  if (!silent) 
			       UERROR( MESSAGE( 202 ));
			  goto string_const;
#endif /* not ANSI */

			default:

			if( stwart == FUNNYNAME ) {
			     /* structure/union member reference -
			      * postpone lookup until building reference
			      */
			     stwart = 0;
			     yylval.intval = (int) yytext;
#ifdef CXREF
			     id_lineno = lineno;
#endif
			     return( NAMESTRING );
			   }

			sp = lookup( yytext,
				/* tag name for struct/union/enum */
				(stwart&TAGNAME)? STAG:
				/* member name for struct/union */
				(stwart&(INSTRUCT|INUNION))? SMOS : 0);
#ifdef ANSI
			if( ( sp->sclass == TYPEDEF )) {
			     /* redundant test for structures */
			     if( !( typeseen || typedefseen ))
#else
			if( ( sp->sclass == TYPEDEF && !stwart )) {
#endif
			          {
			        
# ifdef CXREF
			          ref(sp, lineno);
# endif
#ifdef HPCDB
				  if (in_comp_unit)
				     sp->sflags |= SDBGREQD;
# ifdef SA
				  if (saflag)
				     add_xt_info(sp, USE);
# endif
#endif				  
				  stwart = instruct;
				  idname = sp;
                                  yylval.nodep = MKTYPDF( sp->stype, 
						      sp->dimoff, sp->sizoff );
                                  (yylval.nodep)->in.tattrib  = sp->sattrib;
#if defined(HPCDB) || defined(APEX)
				  (yylval.nodep)->in.nameunused = (int) sp;
#endif
				  return( TYPE_DEF );
				  }
#ifdef ANSI
			     else {
			          if( blevel > sp->slevel )
				       /* "local redeclaration of 
					  typedef as declarator: %s" */
				       if (!silent) WERROR( MESSAGE( 169 ), sp->sname );
				}
#endif
			   }
			stwart = (stwart&SEENAME) ? instruct : 0;
			yylval.symp = sp;
#ifdef CXREF
			id_lineno = lineno;
#endif
			return( NAME );
			
			}  /*end switch( lxchar ) */
		case A_DIG:
			/* collect a digit string, then look at last one... */
			lastcon = 0;
			overflow = 0;
			lxget( lxchar, LEXDIG );
			switch( lxchar=GETCHAR ){

			case 'x':
			case 'X': {
			        int size = 0;

				lxmore( lxchar, LEXHEX );  /* flush, then check syntax */
				if( yytext[0] != '0'  
				       || ( yytext[1] != 'x' && yytext[1] != 'X')
#ifdef ANSI
				       || yytext[2] == 0     /* just '0x' illegal */
#endif
				    ) {
				    if (!silent) UERROR( MESSAGE( 59 ) ); /* "incorrect hex constant" */
				    yylval.intval = INT;     /* supress spurious error messages */
				}
				else {
				  /* convert the value */
				     register char *cp;

				     for( cp = yytext+2; *cp; ++cp ){
				       lastcon <<= 4;
				       if( isdigit( *cp ) ) lastcon += *cp-'0';
				       else if( isupper( *cp ) ) lastcon += *cp - 'A'+ 10;
				       else lastcon += *cp - 'a'+ 10;
#ifdef LINT_TRY
				       size++;
#else
				       if (size || lastcon) size++;
#endif
					 }
				     if( size > (SZLONG/4) ) {
					  display_line++;
				          /* "%s constant too large to represent" */
				          if (!silent) WERROR( MESSAGE( 138 ), "hex");
				     }
				  /* determine type */
				     yylval.intval = lxintsfx( lxhexoctype );
				     }
				return( ICON );
			      }
			case '.':
				lxmore( lxchar, LEXDIG );

			getfp:
				if( (lxchar=GETCHAR) == 'e' || lxchar == 'E' ){ /* exponent */

			case 'e':
			case 'E':
					*lxgcp++ = 'e';
					if( (lxchar=GETCHAR) == '+' || lxchar == '-' ) {
						*lxgcp++ = lxchar;
					        lxchar = GETCHAR;
					      }
					if( lxmask[lxchar+1] & LEXDIG ) 
					     lxmore( lxchar, LEXDIG );
					else {
					     /* "syntax error in floating 
						 point constant" */
					     if (!silent) {
						  display_line++;
						  UERROR( MESSAGE( 187 ));
					     }
					     (void)UNGETC(lxchar);
					     *lxgcp++ = NULL;

					/* now have the whole thing... */
					}
				   }
				else {  /* no exponent */
					(void)UNGETC( lxchar );
					}

				/* check for suffix & determine type */

				switch( ( lxchar = GETCHAR )) {

				case 'f':
				case 'F':
				  yylval.intval = FLOAT; 
				  break;

			        case 'l':
			        case 'L':
#ifndef ANSI
				  /* "long double constants only supported for ANSI C" */
				  WERROR( MESSAGE( 208 ) );
				  yylval.intval = DOUBLE;
#else				  
				  yylval.intval = LONGDOUBLE;
#endif
				  break;

			        default:
				  (void)UNGETC( lxchar );
				  yylval.intval = DOUBLE;
				}

				return( isitfloat( yytext , (TWORD)yylval.intval ));

			default:  
				(void)UNGETC( lxchar );
				if( yytext[0] == '0' ){
				   /* convert in octal */
					register char *cp;
					for( cp = yytext+1; *cp; ++cp ){
						if (*cp > '7') {
						     /* "bad octal digit" */
						     if (!silent) WERROR( MESSAGE( 124 ), *cp );
						}
				                if( ( lastcon & ( 07 << ( SZLONG - 3 ))))
				                     overflow = 1;  /* overflow imminent */
						lastcon <<= 3;
						lastcon += *cp - '0';
					      }
				        if( overflow ){
					     display_line++;
				             /* "%s  constant too large to represent" */
				             if (!silent) WERROR( MESSAGE( 138 ), "octal");
					}
				   /* determine type */
					yylval.intval = lxintsfx( lxhexoctype );
					
					}
				else {
				   /* convert in decimal */
					register char *cp;
					register unsigned CONSZ previous;
                                        register unsigned CONSZ lasticon;

					previous = lasticon = lastcon;
					for( cp = yytext; *cp; ++cp ){
						lasticon = lasticon * 10 + *cp - '0';
                                            /*bad test - doesn't roll to 0*/
					    /*  if( previous > (unsigned CONSZ) lastcon )*/
                                                if (lasticon/10 != previous)
						     overflow = 1;  /* whoops */
						previous = lasticon;
					 }
                                         lastcon = lasticon;
				         if( overflow ){
					     display_line++;
				             /* "%s constant too large to represent" */
				             if (!silent) WERROR( MESSAGE( 138 ), "integer" );
					}
				   /* determine type */
					yylval.intval = lxintsfx( lxdectype );

				      }

				return( ICON );
			      }

		case A_DOT:
			/* look for a dot: if followed by a digit, floating point */
			lxchar = GETCHAR;
			if( lxmask[lxchar+1] & LEXDIG ){
				(void)UNGETC(lxchar);
				lxget( '.', LEXDIG );
				goto getfp;
				}
#ifdef ANSI
			else if (lxmask[lxchar+1] & LEXDOT ) {
				/* look for ellipsis ... */
				lxchar = GETCHAR;
				if ( lxmask[lxchar+1] & LEXDOT ) {
					return(ELLIPSIS);
					}
				else {  /* error */
					(void)UNGETC(lxchar);
					lxchar = '.';
					goto incorrect_character;
					}
				}
#endif
			stwart = FUNNYNAME;
			goto onechar;

		case A_STR:
			/* string constant */
	string_const:
			lxmatch = '"';
			return( STRING );

		case A_CC:
	char_const:
			/* character constant */
			lxmatch = '\'';
			lastcon = 0;
			(void)lxstr(0);
			yylval.intval = 0;
			return( ICON );

		case A_SL:
			  /* / */
			  switch( lxchar = GETCHAR ) {

			     case '*':
			       lxcom();
			       continue;

			     case '=':
			       yylval.intval = ASG DIV;
			       return( ASGNOP );

			     default:
#ifndef ANSI
			       LOOKFOR_ASGNOP( DIV );
#endif /* not ANSI */
			       goto onechar;

			  }

		case A_WS:
			continue;

		case A_NL:
			++lineno;
			if (echoflag)echo();
			echopt = & echoline[2];
			lxnewline();
			continue;

		case A_NOT:
			/* ! */
			if( (lxchar=GETCHAR) != '=' ) goto onechar;
			yylval.intval = NE;
			return( EQUOP );

		case A_MI:
			  /* - */
			  switch( lxchar = GETCHAR ) {

			     case '-':
			       yylval.intval = DECR;
			       return( INCOP );

			     case '=':
			       yylval.intval = ASG MINUS;
			       return( ASGNOP );
			       
			     case '>':
			       stwart = FUNNYNAME;
			       yylval.intval=STREF;
			       return( STROP );

			     default:
#ifndef ANSI
			       LOOKFOR_ASGNOP( MINUS );
#endif /* not ANSI */
			       goto onechar;

			  }


		case A_PL:
			/* + */
			  switch( lxchar = GETCHAR ) {

			     case '+':
			       yylval.intval = INCR;
			       return( INCOP );

			     case '=':
			       yylval.intval = ASG PLUS;
			       return( ASGNOP );

			     default:
#ifndef ANSI
			       LOOKFOR_ASGNOP( PLUS );
#endif /* not ANSI */
			       goto onechar;

			  }

		case A_AND:
			/* & */
			  switch( lxchar = GETCHAR ) {

			     case '&':
			       return( yylval.intval = ANDAND );

			     case '=':
			       yylval.intval = ASG AND;
			       return( ASGNOP );

			     default:
#ifndef ANSI
			       LOOKFOR_ASGNOP( AND );
#endif /* not ANSI */
			       goto onechar;

			  }


		case A_OR:
			  /* | */
			  switch( lxchar = GETCHAR ) {

			     case '|':
			       return( yylval.intval = OROR );

			     case '=':
			       yylval.intval = ASG OR;
			       return( ASGNOP );

			     default:
#ifndef ANSI
			       LOOKFOR_ASGNOP( OR );
#endif /* not ANSI */
			       goto onechar;

			  }


		case A_LT:
			/* < */
			switch( lxchar = GETCHAR ) {

			case '<':
#ifdef ANSI
			     lxchar = GETCHAR;
#else
			     lxchar = get_non_WS();
#endif
			     if( lxchar == '=' ) {
				  yylval.intval = ASG LS;
				  return ( ASGNOP );
			     }
			     else {
				  (void)UNGETC( lxchar );
				  yylval.intval = LS;
				  return( SHIFTOP );
			     }

			case '=':
				 yylval.intval = LE;
				 return( RELOP );

         	        default:
				 goto onechar;

			       }

		case A_GT:
			/* > */
			switch( lxchar = GETCHAR ) {

			case '>':
#ifdef ANSI
			     lxchar = GETCHAR;
#else
			     lxchar = get_non_WS();
#endif
			     if( lxchar == '=' ) {
				  yylval.intval = ASG RS;
				  return ( ASGNOP );
			     }
			     else {
				  (void)UNGETC( lxchar );
				  yylval.intval = RS;
				  return( SHIFTOP );
			     }

			case '=':
				 yylval.intval = GE;
				 return( RELOP );

         	        default:
				 goto onechar;

			       }

		case A_EQ:
			/* = */
			switch( lxchar = GETCHAR ){

			case '=':
				yylval.intval = EQ;
				return( EQUOP );

			default:
				goto onechar;

				}

		case A_ASCK:
			  /* assignment op check for MUL, MOD, ER */
#ifdef ANSI
			  lxchar = GETCHAR;
#else
			  lxchar = get_non_WS();
#endif
			  if( lxchar != '=')  goto onechar;
			  yylval.intval = ASG p->lxval;
			  return( ASGNOP );
#if defined SA && defined ANSI
	  	case A_CTRL:
		          /* a control character from cpp.ansi */
			{
		        char *linep,*ctl_beg;
			flag count_on;
			int col;

			if (!saflag) goto incorrect_character;
			col = (int) (source_line - source_line_start) + 1;
			UNGETC(lxchar);
			linep = source_line + 1;
			while(*linep)
			  {
			  switch (*linep)
			    {
			    case CTRL_A:
				strcpy(linep,linep+1);
				count_on = FALSE;
				break;
			    case CTRL_B:
				ctl_beg = linep;
				while ((*++linep!=CTRL_C) && (*linep!='\0')) ;
				if (*linep != CTRL_C)
					cerror("bad macro marking");
				*linep++ = '\0';
				do_Scanner_macro_use(ctl_beg,cdbfile_vtindex,lineno,col);
				col += strlen(ctl_beg);
				strcpy(ctl_beg,linep);
				linep = ctl_beg;
				count_on = TRUE;
				break;
			    case  CTRL_C:
				ctl_beg = linep;
				while ((*++linep!=CTRL_C) && (*linep!='\0')) ;
				if (*linep != CTRL_C)
					cerror("bad macro marking");
				*linep++ = '\0';
				do_Scanner_macro_define(ctl_beg+1,cdbfile_vtindex,lineno,col);
				col += strlen(ctl_beg);
				strcpy(ctl_beg,linep);
				linep = ctl_beg;
				count_on = TRUE;
				break;

			    default:
				++linep;
				if (count_on) col++;
			    }
			  }
                        if (count_on) { /* go through the # line etc. */
                           count_on = FALSE;
                           lxnewline();
                        }                   /* DTS#FSDdt08978 */
			continue;
			}
#endif /* if SA && ANSI */			  
			  
		default:
			cerror( "yylex error, character 0x%x", lxchar );

			}

		/* ordinarily, repeat here... */
		cerror( "out of switch in yylex" );

		}

	}

struct lxrdope {
	/* dope for reserved, in alphabetical order */

	char *lxrch;	/* name of reserved word */
	short lxract;	/* reserved word action */
	short lxrval;	/* value to be returned */
	} lxrdope[] = {

	"asm",		AR_RW,	ASM,
	"auto",		AR_CL,	AUTO,
	"break",	AR_RW,	BREAK,
	"char",		AR_TY,	CHAR,
	"case",		AR_RW,	CASE,
	"continue",	AR_RW,	CONTINUE,
	"const",	AR_TQ,	CONST,
	"double",	AR_TY,	DOUBLE,
	"default",	AR_RW,	DEFAULT,
	"do",		AR_RW,	DO,
	"extern",	AR_CL,	EXTERN,
	"else",		AR_RW,	ELSE,
	"enum",		AR_E,	ENUM,
	"for",		AR_RW,	FOR,
	"float",	AR_TY,	FLOAT,
# if 0
	"fortran",	AR_CL,	FORTRAN,
# endif /* 0 */
	"goto",		AR_RW,	GOTO,
	"if",		AR_RW,	IF,
	"int",		AR_TY,	INT,
	"long",		AR_TY,	LONG,
	"return",	AR_RW,	RETURN,
	"register",	AR_CL,	REGISTER,
	"switch",	AR_RW,	SWITCH,
	"struct",	AR_S,	0,
	"signed",	AR_TY,	SIGNED,
	"sizeof",	AR_RW,	SIZEOF,
	"short",	AR_TY,	SHORT,
	"static",	AR_CL,	STATIC,
	"typedef",	AR_CL,	TYPEDEF,
	"unsigned",	AR_TY,	UNSIGNED,
	"union",	AR_U,	0,
	"void",		AR_TY,	VOID,
	"volatile",	AR_TQ,	VOLATILE,
	"while",	AR_RW,	WHILE,
	"",		0,	0,	/* to stop the search */
	};

LOCAL int lxResWide() {
	/* check to see of yytext is reserved or initiates wide string; if so,
	   do the appropriate action and return:
	        the encoding of the reserved word (>0)
		wide character constant (-2)
		wide string constant (-3)
		otherwise (-1) */

	register ch;
#ifndef __lint
	register int gettemp;
#endif
	register struct lxrdope *p;
	int c;

	ch = yytext[0];

	if( !islower(ch) && ch != 'L' 
#ifdef APEX
                        && ch != '_'	/* for __attribute and __options */
#endif
		    ) return( -1 );

	switch( ch ){

#ifdef APEX
        case '_':
                c=0; break;
#endif
	case 'a':
		c=0; break;
	case 'b':
		c=2; break;
	case 'c':
		c=3; break;
	case 'd':
		c=7; break;
	case 'e':
		c=10; break;
	case 'f':
		c=13; break;
	case 'g':
		c=15; break;
	case 'i':
		c=16; break;
	case 'l':
		c=18; break;
#ifdef APEX
        case 'o':
                c=0;  break;
#endif
	case 'r':
		c=19; break;
	case 's':
		c=21; break;
	case 't':
		c=27; break;
	case 'u':
		c=28; break;
	case 'v':
		c=30; break;
	case 'w':
		c=32; break;

	case 'L':  /* possible wide character/string */

		if( yytext[1] == 0 ) {
		     if( ( c = GETCHAR ) == '\'' ) return( -2 );
		     if( c == '\"') return( -3 );
		     (void)UNGETC( c );
		}
		return( -1 );

	default:
		return( -1 );
		}

	for( p= lxrdope+c; p->lxrch[0] == ch; ++p ){
		if( !strcmp( yytext, p->lxrch ) ){ /* match */
			switch( p->lxract ){

			case AR_TY:
				/* type word */
				stwart = instruct;
				yylval.nodep = MKTY( (TWORD)p->lxrval, 0, p->lxrval );
				return( TYPE );

			case AR_TQ:
				/* type qualifier */
				yylval.nodep = MKTY( (TWORD)p->lxrval, 0, p->lxrval );
				return( QUAL );

			case AR_RW:
				/* ordinary reserved word */
				return( yylval.intval = p->lxrval );

			case AR_CL:
				/* class word */
				yylval.nodep = MKCL((TWORD) p->lxrval);
				return( CLASS );

			case AR_S:
				/* struct */
				stwart = INSTRUCT|SEENAME|TAGNAME;
				yylval.intval = INSTRUCT;
				return( STRUCT );

			case AR_U:
				/* union */
				stwart = INUNION|SEENAME|TAGNAME;
				yylval.intval = INUNION;
				return( STRUCT );

			case AR_E:
				/* enums */
				stwart = SEENAME|TAGNAME;
				return( yylval.intval = ENUM );

			default:
				cerror( "bad AR_?? action" );
				}
			}
		}
#ifdef APEX
        if (domain_extensions) {
            if (!strcmp(yytext, "__attribute"))
                return ATTRIBUTE;
            else if (!strcmp(yytext, "__options"))
                return OPTIONS;

            /* Have detected a '#', make these reserved now */
            if (opt_attr_flag) {
                if (!strcmp(yytext, "attribute"))
                    return ATTRIBUTE;
                else if (!strcmp(yytext, "options"))
                    return OPTIONS;
            }

        opt_attr_flag = 0;
        }
#endif
	return( -1 );
	}

LOCAL void lxnewline() {
  /* called after a newline; check for "#" directives. */
  register c;
  for(;;) { /* might be several such lines in a row */
      	lxget (' ',LEXWS);
	c = GETCHAR;
	/* The following was added to correct defect SWFfc00722. This is a */
	/* kludge. It insures that GETCHAR returns only non 0 characters.  */

	while (c=='\0') c = GETCHAR;

	/*******************************************************************/

	if (c == '#')
		{ /* line sync, or a pragma */
		lxget (' ',LEXWS);
		lxget (' ',LEXLET);
		if ( strcmp(yytext, " pragma") == 0 )
			lxdirective();
		else if ((strcmp(yytext," line")==0) || (strcmp(yytext," ")==0))
			lxtitle();
#ifdef DOMAIN
                else if (strcmp(yytext, " ident")==0)
                        while (c!='\n' && c!=EOF)
                                c = GETCHAR;
#endif
		else
			{
			/* "unknown directive '#%s'" */
			UERROR( MESSAGE( 213 ), yytext );
			while ( (c = GETCHAR) != '\n' );
			lineno++;
			}

		if (echoflag) echo();
		echopt = &echoline[2];
		}
	else
		{ /* just a new line */
		if (c != EOF) (void)UNGETC(c);
		break;
		}
	}
#ifdef IRIF
	ir_newline(lineno);
#endif
}

LOCAL void lxdirective() {

register char c;

  lxget(' ', LEXWS);
  lxget(' ', LEXLET);
  if ( strcmp(yytext," ALIAS") == 0 )
    {
#ifdef LIPINSKI
	char internal[256];
	char external[256];
	lxget(' ',LEXWS);
	lxget(' ',LEXLET|LEXDIG);
	(void)strcpy(internal,yytext);
	lxget(' ',LEXWS);
	lxget(' ',LEXLET|LEXDIG);
	(void)strcpy(external,yytext);
	if ((strcmp(internal," ")== 0) ||
	    (strcmp(external," ")== 0))
		werror("bad ALIAS directive");
	else if (naliasinsert(&internal[1],&external[1]) == 0)
		werror("duplicate ALIAS");
#endif
    }
  else if ( strcmp(yytext, " FORCED_ALIGNMENT") == 0)
    {
    int istruct = 0;
    struct symtab *sp;
    char buffer[1024];
    int align = 0;
    lxget(' ', LEXWS);
    if ( GETCHAR != '(' ) goto badfa;
    lxget(' ', LEXWS);
    lxget(' ', LEXDIG|LEXLET);
    if (!strcmp(yytext," struct") || !strcmp(yytext," union")){
	istruct = 1;
	lxget(' ',LEXWS);
        lxget(' ',LEXDIG|LEXLET);
    }
    strcpy(buffer,&yytext[1]);
    lxget(' ', LEXWS);
    do {
	c = GETCHAR;
	if (isdigit(c)) align = 10*align + c - '0';
    } while (isdigit(c));
    UNGETC(c);
    lxget(' ', LEXWS);
    if ( GETCHAR != ')') goto badfa;
    if (align < 1 || align > 64 || (align&(align-1))){
	uerror("bad FORCED_ALIGNMENT directive, alignment must be power of two up to 64");
	return;
    }
    sp = lookup(buffer,istruct?STAG:0);
    if (sp->stype == UNDEF){
       uerror("bad FORCED_ALIGNMENT directive, %s must be declared",buffer);
       return;
    }
    dimtab[sp->sizoff+2] = align*SZCHAR;
    return;
badfa:
    uerror("bad FORCED_ALIGNMENT directive");
    }
  else if ( strcmp(yytext, " HP_ALIGN") == 0 )
    {
    struct align_stack_element {flag apollo_align; short align_like;};
    static struct align_stack_element align_stack[256];
    static int top_align_stack = 0;
    int save_align;
    flag save_apollo_align;
    flag check_for_push  = FALSE;
    flag do_push = FALSE;
    flag align_error = FALSE;

    save_apollo_align = apollo_align;
    save_align = align_like;
    lxget(' ', LEXWS);
    lxget(' ', LEXLET|LEXDIG);
    if (strcmp(yytext, " POP") == 0)
        {
	if (top_align_stack == 0)
	    {
	    werror("HP_ALIGN stack is empty; POP ignored");
	    align_error = TRUE;
	    }
	else
	    {
	    --top_align_stack;
	    align_like = align_stack[top_align_stack].align_like;
            apollo_align = align_stack[top_align_stack].apollo_align;
	    }
	check_for_push  = FALSE;
	}
    else if (strcmp(yytext, " HPUX_WORD") == 0)
        {
        align_like = ALIGN300;
        apollo_align = FALSE;
	check_for_push  = TRUE;
	}
    else if (strcmp(yytext, " HPUX_NATURAL_S500") == 0)
        {
        align_like = ALIGN500;
        apollo_align = FALSE;
	check_for_push  = TRUE;
	}
    else if (strcmp(yytext, " HPUX_NATURAL") == 0)
        {
        align_like = ALIGN800;
        apollo_align = FALSE;
	check_for_push  = TRUE;
	}
    else if (strcmp(yytext, " NATURAL") == 0)
        {
        align_like = ALIGNCOMMON;
        apollo_align = FALSE;
	check_for_push  = TRUE;
	}
    else if (strcmp(yytext, " DOMAIN_WORD") == 0)
        {
        align_like = ALIGN300;
        apollo_align = TRUE;
	check_for_push  = TRUE;
        }
    else if (strcmp(yytext, " DOMAIN_NATURAL") == 0)
        {
        align_like = ALIGNCOMMON;
        apollo_align = TRUE;
	check_for_push  = TRUE;
        }
    else if (strcmp(yytext, " NOPADDING") == 0)
        {
        align_like = ALIGNNOPAD;
        apollo_align = TRUE;
	check_for_push  = TRUE;
        }
    else
	{
	werror("invalid HP_ALIGN option; pragma ignored");
	align_error = TRUE;
	check_for_push  = FALSE;
	}
    lxget(' ', LEXWS);
    lxget(' ', LEXLET|LEXDIG);
    if (check_for_push && (strlen(yytext) > 1))
	{
	if (strcmp(yytext, " PUSH") == 0)
	    do_push = TRUE;
	else
	    {
	    werror("invalid HP_ALIGN option; pragma ignored");
	    align_error = TRUE;
	    do_push = FALSE;
	    }
	}
#if 0
    /* Alignment pragmas are now allowed within a struct/union.
     * This changes the state for the alignment of the following members.
     * Alignment for the overall structure is still the max alignment for
     * the members */
    if (instruct & (INSTRUCT|INUNION))
	{
	werror("HP_ALIGN pragma not valid within structure or union; pragma ignored");
	align_error = TRUE;
	}
#endif
    if (do_push && (top_align_stack >= 256))
	{
	werror("HP_ALIGN stack overflow; pragma ignored");
	align_error = TRUE;
	do_push = FALSE;
	}
    if (align_error)
	{
	apollo_align = save_apollo_align;
	align_like = save_align;
	do_push = FALSE;
	}
    if (do_push && ! align_error)
	{
	align_stack[top_align_stack].align_like = save_align;
        align_stack[top_align_stack].apollo_align = save_apollo_align;
	++top_align_stack;
	}
#ifdef IRIF
    if( ! align_error ) {
	 ir_set_alignment( align_like );
    }
#endif /* IRIF */
    }
#ifdef HAIL
  else if ( strcmp(yytext, " HP_MODULE") == 0 )
    {
	lxget(' ',LEXWS);
	lxget('_',LEXLET|LEXDIG);
	if (strlen(yytext) <= 1)
		uerror("invalid HP_MODULE");
	hail_module_name(&yytext[1]);
    }
  else if ( strcmp(yytext, " HP_SECTION") == 0 )
    {
	lxget(' ',LEXWS);
	lxget('_',LEXLET|LEXDIG);
	if (strlen(yytext) <= 1)
		uerror("invalid HP_SECTION");
	hail_section_name(&yytext[1]);
    }
#endif /* HAIL */
  else if ( strcmp(yytext, " OPTIMIZE") == 0 )
    {
    if (blevel > 1)
       /* "pragma cannot appear inside a function or block" */
       WERROR( MESSAGE( 218 ));
    lxget(' ', LEXWS);
    lxget(' ', LEXLET);
    if (strcmp(yytext, " ON") == 0) {
	optlevel = ccoptlevel;	/* revert to the cc specified level */
#ifdef IRIF
	ir_reset_optlevel( optlevel );
#endif /* IRIF */
   }

    else if (strcmp(yytext, " OFF") == 0) {
	 if( ccoptlevel >= 1 ) {
	      optlevel = 1;	/* level 1 == c2 only */
#ifdef IRIF
	      ir_reset_optlevel( optlevel );
#endif /* IRIF */
	 }
   }

    else 
	 /* "ON or OFF must follow #pragma OPTIMIZE" */
	 WERROR( MESSAGE( 216 ));
    }
  else if ( strcmp(yytext, " OPT_LEVEL") == 0 )
    {
    if (blevel > 1)
       /* "pragma cannot appear inside a function or block" */
       WERROR( MESSAGE( 218 ));
    lxget(' ', LEXWS);
    c = GETCHAR;
#ifndef IRIF
    if (!isdigit(c) || (optlevel = c - '0') > 3 || isdigit(c=GETCHAR))
#else /* IRIF */
    if (!isdigit(c) || (optlevel = c - '0') > 2 || isdigit(c=GETCHAR))
#endif /* IRIF */
  {
      /* invalid optimization level for 'OPT_LEVEL' pragma; level %d assumed */
      WERROR( MESSAGE( 217 ), optlevel = ccoptlevel);
      }
    (void)UNGETC(c);	/* put back lookahead for the newline scanner below */
#ifndef LINT
    if (optlevel > ccoptlevel)
      {
      /* cannot select a level higher than specified by cc */
      WERROR( MESSAGE( 217 ), optlevel = ccoptlevel);
      }
#endif
#ifdef IRIF
    ir_reset_optlevel( optlevel );
#endif /* IRIF */
    }

#ifndef IRIF
  else if ( strcmp(yytext, " NO_SIDE_EFFECTS") == 0 )
    {
#ifdef C1_C
	do
	    {
		lxget(' ', LEXWS);
		if ( (c=GETCHAR)=='\n' || c==',') break;
		lxget(c, LEXLET|LEXDIG);
		p2triple(NOEFFECTS, 0, 0);
		p2name(exname(yytext));
		lxget(' ', LEXWS);
		c=GETCHAR;
	    }
	while (c == ',');
	(void)UNGETC(c);
#endif /* C1_C */
    }

  else if (strcmp(yytext, " _HP_SECONDARY_DEF") == 0)
    {
	int saveloc;
	char original[256], alias[256], buffer[1024];

	lxget(' ', LEXWS);
	lxget('_', LEXLET|LEXDIG);
	if (strlen(yytext) <= 1)
	     /* "invalid _HP_SECONDARY_DEF" */
	     UERROR( MESSAGE( 214 ));
	(void)strcpy(original,yytext);
	lxget(' ', LEXWS);
	lxget('_', LEXLET|LEXDIG);
	if (strlen(yytext) <= 1)
	     /* "invalid _HP_SECONDARY_DEF" */
	     UERROR( MESSAGE( 214 ));
	(void)strcpy(alias,yytext);

	/* surround sglobal/set with data/text so c2 won't move the
	 * set before a defining data label */
#ifdef NEWC0SUPPORT
#ifndef ONEPASS
	if (c0flag)
	    (void)sprntf(buffer,"#c0 z\n\tdata\n\tsglobal %s\n\tset %s,%s\n\ttext\n",
	               alias,alias,original);
	else
#endif /* ONEPASS */
#endif /* NEWC0SUPPORT */
	    (void)sprntf(buffer,"\tdata\n\tsglobal %s\n\tset %s,%s\n\ttext\n",
	               alias,alias,original);
	saveloc = locctr(ADATA);
#ifdef ONEPASS
	(void)fprntf(outfile,buffer);
#else  /* ONEPASS */
#   ifdef C1_C
	p2pass(buffer);
#   endif /* C1_C */
#endif  /* ONEPASS */
	(void) locctr(saveloc);
    }
else if ( strcmp(yytext, " HP_SHLIB_VERSION") == 0 ) {
	char buffer[30];
	int month,year;
	flag daterr = FALSE;
	lxget(' ', LEXWS);
	if ( (c = GETCHAR) != '"' ) { UNGETC(c); daterr = TRUE; }
	lxget(' ', LEXDIG);
	if ((strlen(yytext) == 0)||((month=atol(yytext))<1)||(month>12))
		daterr = TRUE;
	lxget(' ', LEXWS);
	if ( (c = GETCHAR) != '/' ) { UNGETC(c); daterr = TRUE; }
	lxget(' ', LEXDIG);
	if ( (c = GETCHAR) != '"' ) { UNGETC(c); daterr = TRUE; }
	if ((strlen(yytext) == 0)||((year=atol(yytext))<0)||
	    (year>9999)||((year>99)&&(year<1900)))
		{
		daterr = TRUE;
		}
	else    {
		if (year<100)
		  { /* do the work here to get right year a la CLL */
		  int currentyear,lower,upper;
		  long clock;
		  char *timestr;
		  
		  clock = time(0);
		  timestr = ctime(&clock);
		  currentyear = atoi(timestr+20); /* get current year in int */
		  lower = currentyear - 49;
		  upper = currentyear + 50;
		  year =  ((currentyear/100) * 100) + year;
		  if(year<lower)
		    year += 100; /* go to next century */
		  if(year>upper)
		    year -= 100; /* go to previous century */
		  if(year<1990)
		    daterr = TRUE;
		  }
		if(daterr)
		  uerror("bad HP_SHLIB_VERSION date");
		(void)sprntf(buffer,"\tshlib_version\t%d,%d\n",month,year);
#ifndef LINT
#   ifdef C1_C
		p2pass(buffer);
#   else
		(void)fprntf(outfile,buffer);
#   endif
#endif /* LINT */
		}
    }

#ifdef NEWC0SUPPORT
else if (strcmp(yytext, " HP_INLINE_LINES") == 0)
	{
	int lines;
	char buffer[40];
	lxget(' ', LEXWS);
	lxget(' ', LEXDIG);
	if ((strlen(yytext) == 0) || ((lines=atol(yytext)) < 0))
		werror("bad HP_INLINE_LINES directive");
	else
		{
#ifndef LINT
#ifdef C1_C
		sprntf(buffer,"#c0 l%d\n",lines);
		p2pass(buffer);
#endif /* C1_C */
#endif /* LINT */
		}
	}
else if ((strcmp(yytext, " HP_INLINE_DEFAULT") == 0) ||
	 (strcmp(yytext, " HP_INLINE_FORCE") == 0) ||
	 (strcmp(yytext, " HP_INLINE_OMIT") == 0) ||
	 (strcmp(yytext, " HP_INLINE_NOCODE") == 0))
	{
	char buffer[1024], pragmabuf[20];
	struct builtin bi,*bptr;
	char *name;
	flag strcpy_monad_flag;
	(void)strcpy(pragmabuf,yytext);
	(void)strcpy(buffer,"#c0 ");
	if (strcmp(pragmabuf," HP_INLINE_DEFAULT") == 0)
		{c = 'D'; strcpy_monad_flag = TRUE;}
	else if (strcmp(pragmabuf," HP_INLINE_FORCE") == 0)
		{c = 'f'; strcpy_monad_flag = TRUE;}
	else if (strcmp(pragmabuf," HP_INLINE_OMIT") == 0)
		{c = 'o'; strcpy_monad_flag = FALSE;}
	else /* if (strcmp(pragmabuf," HP_INLINE_NOCODE") == 0) */
		{c = 'n'; strcpy_monad_flag = TRUE;}

	lxget(' ', LEXWS);
	lxget(c, LEXLET|LEXDIG);
	if ((strlen(yytext) <= 1) &&
	    (strcmp(" HP_INLINE_DEFAULT",pragmabuf) != 0))
	  {
	  (void)sprntf(buffer,"bad%s directive",pragmabuf);
	  werror(buffer);
	  }
	else
	  {
	  if (strlen(yytext) > 1)
	    {
	    name = ((char *) yytext) + 1;
	    while (1)
		{
		(void)strcat(buffer,yytext);
		if (strcmp(name,"strcpy") == 0)
		  strcpy_inline_enabled = strcpy_monad_flag;
#ifndef LINT
		bi.name = name;
		bptr = (struct builtin *)
			 bsearch((char *)&bi, (char *)builtins,
			(unsigned)(builtins_table_size/sizeof(struct builtin)),
			sizeof(struct builtin),builtin_compare);
		if (bptr != NULL)
		  bptr->enabled = strcpy_monad_flag;
#endif
		lxget(' ', LEXWS);
		c=GETCHAR;
		if (c=='\n')
		  {
		  (void)UNGETC(c);
		  break;
		  }
		if (c!=',')
		  {
		  (void)sprntf(buffer, "bad%s directive",pragmabuf);
		  werror(buffer);
		  (void)strcpy(buffer,"");
		  break;
		  }
		lxget(' ', LEXWS);
		lxget(',', LEXLET|LEXDIG);
		if (strlen(yytext) <= 1)
		  {
		  (void)sprntf(buffer, "bad%s directive",pragmabuf);
		  werror(buffer);
		  (void)strcpy(buffer,"");
		  break;
		  }
		name = ((char *) yytext) + 1;
		} /* while (1) */
	    }
#ifndef LINT
#ifdef C1_C
	  p2pass(buffer);
#endif /* C1_C */
#endif /* LINT */
	  }
	}
#endif /* NEWC0SUPPORT */

else if ( strcmp(yytext, " FAST_ARRAY_ON") == 0 ||
	    strcmp(yytext, " FAST_ARRAY_OFF") == 0 ||
	    strcmp(yytext, " ALLOCS_NEW_MEMORY") == 0 ||
	    strcmp(yytext, " COPYRIGHT") == 0 ||
	    strcmp(yytext, " COPYRIGHT_DATE") == 0 ||
	    strcmp(yytext, " INTRINSIC") == 0 ||
	    strcmp(yytext, " INTRINSIC_FILE") == 0 ||
	    strcmp(yytext, " LOCALITY") == 0 ||
	    strcmp(yytext, " VERSIONID") == 0 ||
	    strcmp(yytext, " LINES") == 0 ||
	    strcmp(yytext, " WIDTH") == 0 ||
	    strcmp(yytext, " TITLE") == 0 ||
	    strcmp(yytext, " SUBTITLE") == 0 ||
	    strcmp(yytext, " PAGE") == 0 ||
	    strcmp(yytext, " LIST") == 0 ||
#ifdef SA
	    strcmp(yytext, " CURRENT_DATE") == 0 ||
#endif	       
	    strncmp(yytext, " BBA_", 4) == 0 ||	   /* BBA_IGNORE, etc */
	    strcmp(yytext, " AUTOPAGE") == 0 )
	    /* silently ignore valid S500/S800 directives */ ;
#if (defined (SA)) && (! defined (ANSI))
    else if ( strcmp(yytext, " macrofile") == 0 )
       /* Need to handle the pragma whether or not
	  saflag is on */
      { 
	register c;
        lxget(' ', LEXWS);
        c = GETCHAR;
	/*        if (c == '"')
	  printf("have a string\n");*/
	lxget('"',LEXSLASH|LEXDIG|LEXLET|LEXDOT);
	/* open now so if problem user gets
	   correct line number.  Only call if
	   saflag is set. */
	if (saflag)
	   sa_macro_file_open(&yytext[1]);
      }
#endif /* SA && !ANSI */ 
#endif /* not IRIF */
  else {
       /* "unrecognized compiler directive ignored: %s" */
       WERROR( MESSAGE( 215 ), yytext+1 );
  }

  while ((GETCHAR) != '\n');
  lineno++;
}

LOCAL void lxtitle(){
	/* called after a newline; set linenumber and file name */

	register c;
	register unsigned val;
	register char *cp;
#ifndef __lint
	register int gettemp;		/* quicker than the static global */
#endif

	lxget( ' ', LEXWS );
	val = 0;
	for( c=GETCHAR; isdigit(c); c=GETCHAR ){
		val = val*10+ c - '0';
		}
	(void)UNGETC( c );
	lineno = (int) val;
	lxget( ' ', LEXWS );
	if( (c=GETCHAR) != '\n' ){
#if defined SA	&& defined ANSI
	    for( cp=ftitle; c!='\n' && c!=','; c=GETCHAR)
	    {
	       if ( cp < &ftitle[FTITLESZ-2] )
		  *cp++ = c;
	    }
	    if (saflag)
	       for( ; c!='\n'; c=GETCHAR)
		  ;
#else		
		for( cp=ftitle; c!='\n'; c=GETCHAR)
			{
			if ( cp < &ftitle[FTITLESZ-2] )
				*cp++ = c;
			}
#endif		
		*cp = '\0';
#ifndef IRIF
# ifdef C1_C
		p2triple(FEXPR, (cp-ftitle-1)/4+1, lineno);
		p2str(ftitle);
# endif
#endif /* not IRIF */
# ifdef CXREF
		cxftitle(ftitle);
# endif

# ifdef HPCDB
		if (cdbflag) {
		    char savec, *fp;
		    int len;

		    if (strcmp(ftitle, cdbfile)) {
			/* new file name, get vt index */
			(void)strcpy(cdbfile, ftitle);
			/* skip "" and leading ./ */
			fp = cdbfile + 1;
			if ((fp[0] == '.')&&(fp[1] == '/')) fp +=2;
			len = strlen(fp);
			savec = fp[len - 1];
			fp[len - 1] = '\0';
			cdbfile_vtindex = add_to_vt(fp,TRUE,TRUE);
			fp[len - 1] = savec;
			cdb_srcfile();
		   }
		  
		  /* Do the work for keeping track of whether in
		     the compilation unit. 
		     This is done for -g or -y.  (-y implies cdbflag.)
		     */
		  if (!comp_unit_title[0]) {
		     /* First time, must be the .c being compiled. */
		     (void)strcpy(comp_unit_title, ftitle);
		     comp_unit_vtindex = cdbfile_vtindex;
		     in_comp_unit = 1;
		  }
		  else
		     if (strcmp(comp_unit_title, ftitle))
		        in_comp_unit = 0;
		     else
			in_comp_unit = 1;
		}
# endif /* HPCDB */
#ifdef IRIF 
		ir_linesync(lineno,ftitle);
#endif
		}
		else /* just a line number */
		{
#ifdef IRIF 
		ir_linesync(lineno,NULL);
#endif
		}
      }

#ifdef ANSI
/*******************************************************************
 * macros and globals related to multibyte characters
 *******************************************************************/

#define IS_OCTAL(c)   ( lxmask[c+1] & LEXOCT )
#define W_GETCHAR     ( ( *w_get_ptr == '\0' ) ? GETCHAR : *w_get_ptr++ )
#define W_UNGETC(c)   ( *(--w_get_ptr) = c ) 
#define INIT_GET_WIDE_CHAR  ( terminator_found = 0 )

LOCAL wchar_t wcc;
LOCAL char w_look_aside[3] = { 'h','i','\0'};
LOCAL char *w_get_ptr = &w_look_aside[2];
	/* mbtext holds wide character being processed (and perhaps more)
	 * its size needs to be at least max( SZWIDE/SZCHAR, MB_LEN_MAX ) + 1
	 */
LOCAL char mbtext [16]; 
LOCAL
flag hex_octal_seq;       /* informs caller of get_wide_char that the
			     characters returned formed a hex or octal
			     sequence, i.e., the entire sequence must be
			     a valid wide character [not just part of one or
			     more than one] */
LOCAL
flag terminator_found;    /* flag for get_wide_char which must be
			     initialized to false before each string/
			     character constant is processed */
/***************************************************************************
 *  multibyte character functions
 ***************************************************************************/

LOCAL lxWideChr() {   /* wide character */
  
     int  wcsize = 0;
     int status;

     if( !wide_seen ) {  /* first wide character/string constant */
	  wide_seen = 1;
	  get_tongue();  
     }

     yylval.intval = WIDECHAR;

     INIT_GET_WIDE_CHAR ;

     /* repeated get a wide character code until ' terminator */
     for( ; ( status = get_wc_code( '\'' )) != 0 ; ) {
	  if( status == -1 ) {
	       /* "invalid multibyte character detected starting at 
		   '%c' (hex value 0x%x)" */
	       UERROR( MESSAGE( 70 ), *mbtext, (unsigned char)*mbtext );
	  }
	  wcsize++;
     }
     if( wcsize == 0 ) {
	  display_line++;
	  /* "empty character constant" */
	  UERROR( MESSAGE( 36 ) );
     }
     else {
	  if( wcsize > 1 ) {
	       display_line++;
	       /* "too many bytes in multibyte character constant */
	       WERROR( MESSAGE( 186 ));
	  }
	  lastcon = ( CONSZ ) wcc;
     }
     return( ICON );
}

lxWideStr( limit ) {   /* wide string - limit is max number of initializers
			                (relevant only if is_initializer) */

	int shipped = 0;
	int status;

	if( !wide_seen ) {  /* first wide character/string constant */
	     wide_seen = 1;
	     get_tongue();
	}

	INIT_GET_WIDE_CHAR ;

	while( ( status = get_wc_code( '"' ) ) != 0 ) {

	     if( status == -1 ) {
		  /* "invalid multibyte character detected starting at 
		      '%c' (hex value 0x%x)" */
		  UERROR( MESSAGE( 70 ), *mbtext, (unsigned char)*mbtext );
	     }
             else {
	     /* ship it out */
	     if( is_initializer ) {
		  if( limit == 0 || shipped < limit ) 
		       putdatum( (int)wcc, WIDECHAR );
		  else if( shipped == limit ) WERROR( MESSAGE( 175 ) );
	     }
	     else bycode( (int)wcc, shipped, WIDECHAR, 0 );
	     shipped++;

	     }
	}
	lxEndStr( shipped, limit, WIDECHAR );
	return( shipped );
   }

LOCAL get_wc_code( terminator ) char terminator; {

     /* return wide character code in wcc 
      *
      * return codes:
      *  normal:
      *     0  :  empty character constant
      *     1  :  wcc contains code
      *  abnormal:
      *    -1  :  invalid mb character detected
      */     

     int  wcsize;
     int  mbytes;
     int  i;
     
     if( ( wcsize = get_wide_char( terminator )) == 0 )
	  return( 0 ); /* empty mb char constant */

     if( hex_octal_seq ) { 
	  /*
	   * hex/octal sequences are taken at face value
	   * - no verification is done, e.g., no mbtowc()
	   */
	  char *cp;
	  wcc = 0;
	  for( cp = mbtext; *cp ; cp++ )
	       wcc = ( wcc << SZCHAR ) | (wchar_t)((unsigned char) *cp );
     }
     else {
	  /* not hex_octal_seq  
	   * check validity
	   */
	  if(   (( mbytes = mbtowc( &wcc, mbtext, (size_t) MB_CUR_MAX )) == -1 )
	     /* rule out null */
	     && !( ( wcc == NULL ) && ( wcsize == 1 ))) {

	       return( -1 );   /* invalid mb character */
	  }
	  /* unget unused portion */
	  for( i = mbytes; i < wcsize; i++ ) {
	       W_UNGETC( mbtext[ i ] );
	  }
     }
     return( 1 );
}
LOCAL get_wide_char( terminator ) char terminator; { 
	  /* retrieves up to MB_CUR_MAX characters;
	   *
	   * puts characters in mbtext
	   * returns the number of bytes from beginning of mbtext
	   *     (which includes NULL character)
	   */

        register int c;
	char d1, d2;
#ifndef __lint
	register int gettemp;
#endif
	char *cp = mbtext;
	flag complete = 0;

	if( terminator_found ) return( 0 ); /* no more work here */

	hex_octal_seq = 0;

	do
	while(   ( cp - mbtext < MB_CUR_MAX ) && !complete 
	      && (( c = W_GETCHAR ) != terminator )) {

	     if( c == EOF ) {
			/* "unexpected EOF" */
			UERROR( MESSAGE( 113 ) );
			terminator_found = 1;
			break; }

	     if( c == '\n' ) {
		  /* "newline in string or char constant" */
		  UERROR( MESSAGE( 78 ) );
		  ++lineno;
		  continue; }

	     if( c == '\\' ) {
			if( (( c = W_GETCHAR ) == 'x' ) || IS_OCTAL( c )) {
			     complete = 1; /* hex/octal sequence specifies
					      entire character */
			     if( cp > mbtext ) {
				  /* hex/octal sequence will start next char */
				  W_UNGETC( c );
				  W_UNGETC( '\\' );
			     } 
			     else {
	                          cp = lxescape( c, SZWIDE/SZCHAR, cp );
				  hex_octal_seq++; 
			     }
			} else if ( c == terminator) {
                                      /* This is added to process the chars */
                                      /* "\"", '\"', "\'", '\''.            */ 
                                      /* DTS#FSDdt08969   09/23/92          */ 
                          *cp++ = c;            
                            c = 0;
                        }
			else {
	                     cp = lxescape( c, 1, cp );
			}
		   }
	     else  {
			*cp++ = c;
		   }

	   } /* end while */

	/* do condition: handles concatenation */
	while(   ( c == terminator ) && ( terminator == '"' )
	      && ( d1 = get_non_WS(), d2 = GETCHAR, 
		        ( ( d1 == 'L') && (d2 == terminator) )));
	/* end do */

	*cp = '\0';
	if( c == terminator ) {
	     terminator_found++;
	     if( terminator == '"' ) {
		  (void)UNGETC( d2 );   /* multiple UNGETCs possible, thanks to */
		  (void)UNGETC( d1 );   /* line oriented input && cpp.ansi (\<nl>) */
	     }
	}
	return( cp - mbtext );
}
/****************************************************************************
 *  end multibyte character macros/globals/functions
 ****************************************************************************/
#endif /* ANSI */

LOCAL char get_non_WS()
  {  /* advances input stream to next non whitespace or newline */

  char c;

  while( (c = GETCHAR, lxmask[c+1] & LEXWS) ||
         ((c == '\n')
          ? (++lineno,(echoflag?echo():0),echopt=(&echoline[2]),lxnewline(),1)
          : 0)
       )
          ;
  return( c );
  }
			          
extern char *getenv();	/* in libc */

get_tongue() {  /* determine language for nls/wide character processing */
#ifndef PAXDEV
        char *lang;
	lang = (char *)getenv("LANG");

	if (!lang || (*lang=='\0')) 
	     /* "LANG environment variable not set" */
	     WERROR( MESSAGE ( 162 ) );
#ifndef IRIF
#ifndef _LOCALE_INCLUDED
	if(nl_init(lang)) 
	     /* "Language %s not available ... " */
	     WERROR( MESSAGE ( 136 ), lang );
#else /* _LOCALE_INCLUDED */
	if (!setlocale(LC_ALL, "")) {
#ifdef OSF
		WERROR( MESSAGE ( 136 ), lang);
#else
		werror(_errlocale(""));
#endif /* OSF */
	        }
        /* Disables language localization for numeric data only. */
        /* This corrects defect:  CLL4600056.                    */
        setlocale(LC_NUMERIC, "C"); 

#endif /* _LOCAL_INCLUDED */
#else /* IRIF */
# ifndef HAIL
	if(nl_init(lang)) 
	     /* "Language %s not available ... " */
	     WERROR( MESSAGE ( 136 ), lang );
# endif /* HAIL */
#endif /* IRIF */
#endif /* PAXDEV */
}

void
read_asm_body() {

#ifndef IRIF

	/* called after ASM reserved word seen.  Read the body of the asm,
	 * and copy the characters to the output stream.
	 */
	 register int c;
#ifndef __lint
	 register int gettemp;		/* quicker than the static global */
#endif
# ifdef C1_C
	 static int already_warned = 0;
# define ASMFLAG 0x80
# define ASMLENGTH 512
	 /* a non-expandable buffer is used here because of the fixed limit
	  *  on the length of an FTEXT record (255 long words).  ANSI C 
	  *  requires 509 char/line.
	  */
	 char asmbuf[ASMLENGTH];
	 int asmptr = 0;		/* next position to be filled */
	 /* "asm code may be wrong with -O, try +O1 or #pragma OPT_LEVEL 1" */
	 if ( (optlevel>1) && !already_warned) 
	     {
	     WERROR(MESSAGE(135));
	     already_warned = 1;
	     }
	asm_esc = ASMFLAG; /* warn the world! */
# endif  /* C1_C */

	lxget( ' ', LEXWS );
	if( GETCHAR != '(' ) goto badasm;
	lxget( ' ', LEXWS );
	if( GETCHAR != '"' ) goto badasm;

# ifndef LINT
#   ifndef C1_C
	(void)prntf("#2 asm_begin\n");
#   else
	p2pass("#2 asm_begin");
#   endif /* C1_C */
# endif	/* LINT */

	while( (c=GETCHAR) != '"' ){
		if( c==EOF ) goto badasm;
		if( c=='\n' ) {
			++lineno;
# ifndef LINT
#   ifndef C1_C 	/* echoflag is disabled in the 2-pass version */
			if (echoflag) echo();
			echopt = &echoline[2];
#   else  /* C1_C */
			asmbuf[asmptr] = '\0';
			p2pass(asmbuf);
			asmptr = 0;
#   endif /* C1_C */
# endif
			}
# ifndef LINT
#   ifndef C1_C
		(void)putchar(c);
#   else  /* C1_C */
		else if (asmptr >= ASMLENGTH) 
		    {
			/* asm statement too long */
			/* "bad asm construction" */
			UERROR( MESSAGE( 16 ) );
			while( (c=GETCHAR) != EOF && c != '"' ) /* discard */;
		    }
		else asmbuf[asmptr++] = c;

#   endif /* C1_C */
# endif
		}
	lxget( ' ', LEXWS );
	if( GETCHAR != ')' ) goto badasm;
# ifndef LINT
#   ifndef C1_C
	(void)putchar('\n');
	(void)prntf("#3 asm_end\n");
#   else
	/* print last line */
	asmbuf[asmptr] = '\0';
	p2pass(asmbuf);
	p2pass("#3 asm_end");
#   endif /* C1_C */
# endif
	return;

badasm:
	/* "bad asm construction" */
	UERROR( MESSAGE( 16 ) );
	return;

#else /* IRIF */
	 cerror( "IRIF: support for 'asm' not implemented" );
#endif /* IRIF */
}

LOCAL echo()
{
	*--echopt = '\n';
	*++echopt = '\0';
	(void)prntf("%s", echoline);
}


/* These pass1 table routines have been moved here from common */

char *lastac;
char *asciz;
extern unsigned int maxascizsz;
extern char *taz;
extern unsigned int maxtascizsz;                /*  = TASCIZSZ in common */

/* addasciz takes a ptr to a char string containing a name, finds the next
   available spot in the asciz table, copies the string into the asciz array,
   and returns the address of the first char. Names are assumed null terminated.
   If the remaining space is insufficient, it makes an effort to rewrite the
   asciz table; if unsuccessful, it reports a cerror.
*/
char	*addasciz(cp)	char	*cp;
{
	register i;
	register char	*lcp = cp;

	for (i=1; *lcp++; i++) /* NULL */ ;	/* count the chars */
	if (i > maxascizsz - (lastac-asciz)) newasciz(1);
	lcp = lastac;				/* save it */
	lastac += i;
	return ( strcpy(lcp, cp) );
}



LOCAL flag	ascizrecursed;


/* newasciz makes a new asciz table and inits it. The idea behind re-
   allocating a new table is that as entries are removed, the table
   becomes increasingly sparse. By reallocating a new table and
   recopying the symbol table, it is made dense once again.
*/

newasciz(replacing) 	flag replacing;
{
	if (ascizrecursed++) cerror("logic error in newasciz-recursion");

	if (replacing) maxascizsz += 2000;
	if ( (lastac = asciz = malloc(maxascizsz) ) == NULL)
		many("Asciz", 'a');
	ascizrecursed--;
}


/* inittabs initializes internal pass 1 storage tables */
inittabs()
{
	psavbc = asavbc = ckalloc (maxbcsz * sizeof(int) );
	dimtab = ckalloc( maxdimtabsz * sizeof(int) );
	swtab  = (struct sw *) ckalloc( maxswitsz * sizeof (struct sw) );
	paramstk = ckalloc( maxparamsz * sizeof(int) );
#ifdef HPCDB
	if (cdbflag)
		{
		atype_dntt= (unsigned long *) ckalloc ( maxparamsz * sizeof(long) );
		}
#endif /* HPCDB */
	/* others will be done later */
}

#if defined(ANSI) && !defined(LINT)
LOCAL turn_off_asms() {
	/* change the scanner table so that it returns "-1" when the asm
	 * keyword is seen. This is the same value returned when no match
	 * is possible */
	lxrdope[0].lxrval = -1;
}
#endif

LOCAL turn_off_ansi_keywords(){
	/* change the scanner table so that it returns "-1" when
	 * const,volatile or signed are seen */
	char *p;
	struct lxrdope *lx = lxrdope;
	while (strlen(p=lx->lxrch)){
		if (!strcmp(p,"const")){
			lx->lxract = AR_RW;
			lx->lxrval = -1;
		} else if (!strcmp(p,"volatile")){
			lx->lxract = AR_RW;
			lx->lxrval = -1;
		} else if (!strcmp(p,"signed")){
			lx->lxract = AR_RW;
			lx->lxrval = -1;
		}
		lx++;
	}
}
 
/*  parser error handling */
yyerror( s ) char *s;
{
	display_line++;
	uerror(s);
}
/***********************************************************************
 *  error message displayed with source text
 ***********************************************************************/

void display_error () {
     int lead_ws;
     int i;
     int spot;

     display_line = 0;		
     if( source_line_start == NULL ) {
	  (void)stderr_fprntf( " at <end-of-file>\n");
	  return;
     }
     spot = source_line-source_line_start;
     /* strip leading white space */
     for( lead_ws=0; lead_ws<spot ; lead_ws++ )
	  if( !(lxmask[ source_line_start[lead_ws]+1 ] & LEXWS )) break;
     
     /* Ansi mod static will affect which lines meet 
	the following criterion, because all of the
	control characters are in the source buffer.
	Trying to make the error messages equivalent
	in all cases would slow down all compiles,
	because it seems that the underlying
	bufferring mechanism would have to change. */
     
     if( spot < DISPLAY_SIZE+lead_ws ) { 
	  /* get line ready for display */
	       
	  (void)stderr_fprntf( ":\n" );
#if defined SA && defined ANSI	       
	  /* If static and ansi, then the source_buffer might
	     have control characters in it for macros */
	  if (saflag) {
	       char sa_tmp;
	       flag copy_flag = TRUE;
	       int s;
	       for(s=i=0; (i<DISPLAY_SIZE) && ((s+lead_ws)<buffersize); s++) {
		    /* test for CTRL_B,CTRL_A; othw copy display_text */
		    switch(sa_tmp = source_line_start[s+lead_ws]) {

		       case CTRL_A:
			 copy_flag = TRUE;
			 break;
		       case CTRL_B:
			 copy_flag = FALSE;
			 break;
		       case CTRL_C:
			 copy_flag = TRUE;
			 break;
		       default:
			 if(copy_flag)
			      display_text[i++] = sa_tmp;
		    } 
	       }
	  }
	  else {
	       for( i=0; i<DISPLAY_SIZE ; i++ )
		    display_text[i] = source_line_start[i+lead_ws];
	  }
#else	       
	  for( i=0; i<DISPLAY_SIZE ; i++ ) {
	       display_text[i] = source_line_start[i+lead_ws];
	  }
#endif	       
	  display_text[i] = '\n';
	  display_text[i+1] = '\0';
	  (void)stderr_fprntf( "\t%s", display_text );
	  for( i=0; i<spot-lead_ws ; i++ )
	       display_text[i] = ' ';
	  display_text[i] = '^';
	  display_text[i+1] = '\0';
	  (void)stderr_fprntf( "\t%s\n", display_text );
     }
     else {
	  /* line too long for display */
	  (void)stderr_fprntf( "\n" );
     }
}

