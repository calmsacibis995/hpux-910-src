/* DOND_ID @(#)cgram.y          70.7 93/11/10 */
/* SCCS cgram.y    REV(64.4);       DATE(92/04/03        14:21:53) */
/* KLEENIX_ID @(#)cgram.y	64.4 92/01/17 */
/* file cgram.y */

%term PLUS 1
%term ICON  4
%term FCON  5
%term NAME  6
%term STRING 7
%term MINUS   8
%term MUL   11
%term AND   14
%term OR   17
%term ER   19
%term QUEST  21
%term COLON  22
%term ANDAND  23
%term OROR  24

/*	special interfaces for yacc alone */
/*	These serve as abbreviations of 2 or more ops:

	ASGNOP  +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
	RELOP	LE,LT,GE,GT
	EQUOP	EQ,NE
	DIVOP	DIV,MOD
	SHIFTOP	LS,RS
	ICOP	ICR,DECR
	UNOP	NOT,COMPL
	STROP	DOT,STREF

	*/

%term ASGNOP 25
%term RELOP  26
%term EQUOP  27
%term DIVOP  28
%term SHIFTOP  29
%term INCOP  30
%term UNOP  31
%term STROP  32

/*	reserved words, etc */
%term TYPE  33
%term CLASS  34
%term STRUCT  35
%term RETURN  36
%term SIZEOF  37
%term IF  38
%term ELSE  39
%term SWITCH  40
%term BREAK  41
%term CONTINUE  42
%term WHILE  43
%term DO  44
%term FOR  45
%term DEFAULT  46
%term CASE  109
%term GOTO  48
%term ENUM 97


/*	little symbols, etc. */
/*	namely,

	LP	(
	RP	)

	LC	{
	RC	}

	LB	[
	RB	]

	CM	,
	SM	;

	*/

%term LC  50
%term RC  51
%term LB  52
%term RB  53
%term LP  54
%term RP  55
%term CM  56
%term SM  57
%term ASSIGN  58
#ifdef APEX
%term ATTRIBUTE 59
%term OPTIONS 60
%term SHARP  61
#endif

/* token for parsing an "asm" reserved word */
#ifdef IRIF
%term ASM  150
#else /* not IRIF */
%term ASM  137
#endif /* IRIF */

%term ELLIPSIS 127
%term WIDESTRING 128
%term QUAL 129
%term TYPE_DEF 130
%term NAMESTRING 131


%left CM
%right ASSIGN ASGNOP
%right QUEST COLON
%left OROR
%left ANDAND
%left OR
%left ER
%left AND
%left EQUOP
%left RELOP
%left SHIFTOP
%left PLUS MINUS
%left MUL DIVOP
%right UNOP
%right INCOP SIZEOF
%left LB LP STROP
%{
# include "mfile1"
# include "messages.h"
# include <malloc.h>
# ifdef SA
# include "sa.h"
# endif
# ifdef LINT_TRY
# include "lmanifest"
# endif
# ifdef APEX
extern int opt_attr_flag;       /* # seen: treat "options" and "attribute" as
				 * reserved words in scan.c */
# endif

#ifdef IRIF
       int looplab;
# define SCONVCK( p )    ( ( p->in.op == SCONV &&                            \
			    ir_t_bytesz( p ) == ir_t_bytesz( p->in.left )) ? \
			  p->in.left : p )
#endif /* IRIF */

# define CHECKTERMPTR(p) if (!p) niltermptr()

/* WILL_BE_SCALAR will be scalar after implicit type conversion in buildtree() */
# define WILL_BE_SCALAR(t) ( ISSCALAR(t) || ISARY(t) || ISFTN(t) )

%}

	/* define types */
%start translation_unit

%type <intval> con_e ifelprefix ifprefix doprefix enum_head str_head
%type <nodep> e .e term typcl typcl_list attributes oattributes enum_dcl
		struct_dcl cast_type null_decl funct_idn declarator fdeclarator
		nfdeclarator elist tattrib switchpart
#ifdef IRIF
%type <nodep> forprefix whprefix
#else /* not IRIF */
%type <intval> forprefix whprefix
#endif /* not IRIF */

%type <symp> name_lp
#ifdef ANSI
/*%type <nodep> parameter_list*/
%type <nodep> parameter_type_list  oparameter_type_list  
%type <nodep> abstract_declarator
#endif

%token <intval> STRUCT RELOP CM DIVOP PLUS MINUS SHIFTOP MUL AND OR ER ANDAND OROR
		ASSIGN ASGNOP STROP INCOP UNOP ICON FCON NAMESTRING
%token <nodep> TYPE TYPE_DEF QUAL CLASS
%token <symp>  NAME

%%

%{
#ifdef IRIF
     static void genswitch();
#endif /* IRIF */

extern void read_asm_body();

extern flag display_line;    /* in scan.c: error msg includes source line */

extern short nerrors;        /* in common: number of errors */
short nerrors_old;

flag icons_error;
static no_attributes;

# ifdef SA
struct symtab unknown_sym = {NULL, NULL,
                              "__Unknown_function",
			      FTN, 0, /*stype,sattrib*/
			      EXTERN, 0, /*sclass,slevel*/
			       SDBGREQD, /* sflags */
			       0, /* offset */
			       0, 0, 0, /*dimoff,sizoff,suse*/
			       NULL_XT_INDEX /*xt_table_index*/
			       };  /* other fields, don't care*/
# endif			       
			       

# ifndef ANSI
	static int fake = 0;
	static char fakename[NCHNAM+1];
	static char paramflag = 0;
# endif

# ifdef HPCDB
	static int dcl_list_empty;
	static int LC_lineno;
# endif
# ifdef ANSI
extern void do_default_param_dcls();
	char fdecl_style = FDECL_NULL;	/* keep track of old/new function style */
	char curftn_style = FDECL_NULL;	/* keep track of old/new function style */
	NODE * curftn_node;	/* Node for the FTN declarator */
	int curftn_class;	/* Like curclass, but for the current FTN
				 * declarator. It's used to pass the correct
				 * class info to the defid call for the
				 * function id.
				 */
# endif

# ifdef ANSI
/* The CHK_LEV_HEADS macro calls are inserted to do cleanup in case
 * parsing errors have occurred.  Unrecoverable syntax errors while
 * parsing function declarators can leave garbage entries linked to
 * the symbol table chains.  'reset_lev_heads()' does cleanup to
 * help allow parsing to continue if possible.
 */
extern void reset_lev_heads();
# define CHK_LEV_HEADS(lev) if (blevel>lev) reset_lev_heads(lev, blevel);\
	inproto=0;
# else
# define CHK_LEV_HEADS(lev)
# endif 

	static NODE * qidecl;	/* tymerge result for init_declarator */
%}

translation_unit:  ext_def_list            /* shift/reduce with ext_def_list */
                |  /* empty */
                   ={
#ifdef ANSI
		     /* "empty source file" */
		     WERROR( MESSAGE( 10 ));
#endif
		   }

ext_def_list:	   ext_def_list external_def
		|  external_def
			=ftnend();
		;
external_def:	   data_def
			={ curclass = SNULL; CHK_LEV_HEADS(0); blevel = 0; }
		| ASM { (void)read_asm_body(); } semicolon
			={ curclass = SNULL; CHK_LEV_HEADS(0); blevel = 0; }
		|  error
			={ curclass = SNULL;  CHK_LEV_HEADS(0); blevel = 0; }
		;
data_def:          semicolon
                        ={ 
#ifdef ANSI
			     /* "extraneous ';' ignored" */
			     display_line++;
			     WERROR( MESSAGE( 209 ));
#endif /* ANSI */
			}
                |  attributes  semicolon
			={ $1->in.op = FREE; 
#ifdef ANSI
			   if( !( $1->in.type == STRTY ) && 
			       !( $1->in.type == ENUMTY ) &&
			       !( $1->in.type == UNIONTY ) )
			       /* "empty declaration" */
			       WERROR( MESSAGE( 137 ) );
#endif
			 }
		|  oattributes init_dcl_list  semicolon
			={  $1->in.op = FREE;
			     if( no_attributes && ( nerrors == nerrors_old )) {
				  /* "storage class, type specifier or type
				      qualifier expected; 'int' assumed" */
				  WERROR( MESSAGE( 73 ));
			     }
		       }
		|  oattributes fdeclarator {
		                if( curclass != SNULL && curclass != STATIC &&
			                  curclass != EXTERN ) {
				     /* "function storage class incorrect; 
					 'extern' assumed" */
				     WERROR( MESSAGE( 45 ));
				     curclass = EXTERN;
				}
# ifndef ANSI
				defid( tymerge($1,$2), curclass==STATIC?STATIC:EXTDEF );
# ifdef SA
                                if (saflag)
				  add_xt_info($2->nn.sym, DEFINITION | DECLARATION);
# endif				
				paramflag = 0;
# else /* ANSI */
				{
				NODE * q; 
				q = tymerge($1,$2);
				check_function_header(q);
				/* In ANSI mode, defid call deferred until
				 * after the args have been defined.  This
				 * is because defid needs to have the parameter
				 * types to be able to do compatibility
				 * checks on function prototypes.
				 * defid( q, curclass==STATIC?STATIC:EXTDEF );*/
				curftn_style = fdecl_style;
				fdecl_style = FDECL_NULL;
				curftn_node = q;
				curftn = q->nn.sym;  /* ??? will any place 
						      * need to use this? it
						      * isn't really properly
						      * set up yet. */
				/* The parameter/name list was unscoped at
				 * the close of the list.  Now that we
				 * know that it is a real function definition
				 * the names need to be made accessible
				 * again.
				 */
				rescope_parameter_list((NODE *)dimtab[q->fn.cdim], 1);
			        }
# endif  /* ANSI */
# ifdef CXREF
				/* moved 'bbcode' call to 'name_lp' actions
				 * so that parameter names are properly
				 * blocked.
				 */
				/*bbcode();*/
# endif
				}  function_body
			={  
			    if( blevel ) cerror( "function level error" );
			    if( reached ) retstat |= NRETVAL; 
			    $1->in.op = FREE;
			    ftnend();
#ifdef ANSI
			    curftn_node = NIL;
			    curftn_style = FDECL_NULL;
			    fdecl_style = FDECL_NULL;
#endif /* ANSI */
			    }
		;

function_body:	   arg_dcl_list 
# ifdef ANSI
			{ /* defid of the function id is now delayed until
			   * after processing the arglist.
			   * ??? will the curclass be valid at this point???
			   */
			   do_default_param_dcls();
			   blevel = 0;	/* while defid the function name */
			   defid(curftn_node, curftn_class==STATIC?STATIC:EXTDEF);
# ifdef SA
                           if (saflag)
                             add_xt_info(curftn_node->nn.sym, DEFINITION | DECLARATION);
# endif				
			   blevel = 1; /* reset */
			   }
# endif /* ANSI */
			  compoundstmt
		;
arg_dcl_list:	   arg_dcl_list attributes declarator_list semicolon
#ifndef ANSI
			={ curclass = SNULL;  $2->in.op = FREE; }
#else /* ANSI */
			={ 
			    if (curftn_style!=FDECL_NEW && curftn_style!=FDECL_OLD)
				cerror("curftn_style panic");
			    if (curftn_style==FDECL_NEW)
				/* "prototypes and old-style parameter declarations mixed" */
				UERROR( MESSAGE( 188 ) );
			    curclass = SNULL;  $2->in.op = FREE;
			    }
#endif /* ANSI */
		| 	={  blevel = 1; }
		;

stmt_list:	   stmt_list statement
		|  /* empty */
			={  
#ifndef IRIF
			     bccode();
			     (void)locctr(PROG);
#endif
			    }
		;


/* check_cdb_scope :  this production added to aid generation of K_BEGIN for
 *	compound statments that contain at least one declaration when generating
 *	cdb symbol tables.
 *	When this rule is reduced, we have a declaration within a compound 
 *	statement. If this is the first, force out a K_BEGIN.
 * 	The purpose here is to generate a K_BEGIN/SLT_BEGIN before any sltnormal's
 * 	generated by declaration initialization code. BUT to only generate a 
 * 	K_BEGIN if the compound statement does include declarations. (Thus
 * 	we don't just generate K_BEGIN/SLT_BEGIN every time a '{' opening a
 * 	compound statement is encountered.
 */

check_cdb_scope  : /* empty */
			={

# ifdef HPCDB

			if (cdbflag && dcl_list_empty) {
			   dcl_list_empty=0;
			   cdb_startscope(1,LC_lineno);
			   }
# endif
			}

		;

/* added new 'check_cdb_scope' to support cdb.  The placement before
 * 'attributes' is necessary because 'init_dcl_list' rule assumes that
 * it is always just preceeded by 'attributes'.
 */
dcl_stat_list	:  dcl_stat_list check_cdb_scope attributes semicolon
			={ $3->in.op = FREE; 
#ifdef ANSI
			if(($3->in.type == STRTY)||($3->in.type == UNIONTY)) {
			    if( $3->nn.sym && ($3->nn.sym->slevel < blevel) )
			    { /* An empty declaration causes a new incomplete
				 declaration */
				NODE *q = block(FREE,NIL,NIL,$3->in.type,
					$3->fn.cdim,$3->fn.csiz);
				q->nn.sym = $3->nn.sym;
				defid(q, ($3->in.type==STRTY)? STNAME: UNAME);
			    }
			} else if ($3->in.type == ENUMTY) {
			} else WERROR( MESSAGE( 137 ) );
#endif
			 }
		|  dcl_stat_list check_cdb_scope  attributes init_dcl_list semicolon
			={  $3->in.op = FREE; }
		|  /* empty */
		;

oattributes:	  attributes
                        ={
			    no_attributes = 0;
		       } 
		|  /* VOID */
			={  
			     $$ = MKTY(INT,0,INT);  
			     curclass = SNULL;
			     no_attributes = 1;
			     nerrors_old = nerrors;
		       }
		;
attributes	:  typcl_list 
			={ if (!was_class_set) curclass = SNULL; }
		;


typcl_list	:  typcl
  			{ 
		          $$ = types($1,NIL,NIL,NIL,NIL,NIL); 
#ifdef ANSI
			  typeseen = 0; typedefseen = 0;
#endif
			}
		|  typcl typcl
			{
#ifdef ANSI
  			  if( ( typeseen + typedefseen ) > 1 ) 
			       /* "use of typedef precludes use of additional
				   type specifiers "*/
			       UERROR( MESSAGE( 148 ));
#endif
			  $$ = types($1,$2,NIL,NIL,NIL,NIL); 
#ifdef ANSI
			  typeseen = 0; typedefseen = 0;
#endif
			}
		|  typcl typcl typcl
  			{
#ifdef ANSI
			  if( ( typeseen + typedefseen ) > 1 ) 
			       /* "use of typedef precludes use of additional
				   type specifiers "*/
			       UERROR( MESSAGE( 148 ));
#endif
			  $$ = types($1,$2,$3,NIL,NIL,NIL); 
#ifdef ANSI
			  typeseen = 0; typedefseen = 0;
#endif
			}
		|  typcl typcl typcl typcl
  			{ 
#ifdef ANSI
			   if( ( typeseen + typedefseen ) > 1 )
			       /* "use of typedef precludes use of additional
				   type specifiers "*/
			       UERROR( MESSAGE( 148 ));
#endif
			  $$ = types($1,$2,$3,$4,NIL,NIL); 
#ifdef ANSI
			  typeseen = 0; typedefseen = 0;
#endif
			}
		|  typcl typcl typcl typcl typcl
  			{ 
#ifdef ANSI
			   if( ( typeseen + typedefseen ) > 1 )
			       /* "use of typedef precludes use of additional
				   type specifiers "*/
			       UERROR( MESSAGE( 148 ));
#endif
			  $$ = types($1,$2,$3,$4,$5, NIL); 
#ifdef ANSI
			  typeseen = 0; typedefseen = 0;
#endif
			}
		|  typcl typcl typcl typcl typcl typcl
  			{ 
#ifdef ANSI
			   if( ( typeseen + typedefseen ) > 1 ) 
			       /* "use of typedef precludes use of additional
				   type specifiers "*/
			       UERROR( MESSAGE( 148 ));
#endif
			  $$ = types($1,$2,$3,$4,$5,$6); 
#ifdef ANSI
			  typeseen = 0; typedefseen = 0;
#endif
			}
		;

typcl		:  CLASS
			{ if( ( curclass != MOS ) && ( curclass != MOU ))
			       curclass = (int) $1->in.left; 
			  $$ = $1; }
                |  QUAL
		|  TYPE       
#ifdef ANSI
			      { typeseen = 1; }
#endif
                |  TYPE_DEF   
#ifdef ANSI
			      { typedefseen++;}
#endif
		|  struct_dcl 
#ifdef ANSI
			      { typeseen = 1; }
#endif
		|  enum_dcl   
#ifdef ANSI
			      { typeseen = 1; }
#endif
		;

tattrib		:  QUAL
			{ $$ = ntattrib($1,(NODE *)NULL); }
		|  QUAL QUAL
			{ $$ = ntattrib($1,$2); }
		;

enum_dcl:	   enum_head LC enum_dcl_suffix
         		={ $$ = dclstruct($1); }
		|  ENUM NAME
			={  $$ = rstruct($2,0);  stwart = instruct;
# ifdef CXREF
			    ref($2, id_lineno);
# endif
# ifdef HPCDB
                            if (in_comp_unit)
			       $2->sflags |= SDBGREQD;
# endif

			    }
                |  ENUM     /* shift/reduce conflict */
                          {  
			    /* "%s keyword should be followed by identifier or declaration list" */
			    UERROR( MESSAGE( 165 ), "ENUM" );
			    $$ = MKTY(ENUMTY,0,INT);  
			    curclass = SNULL; 
			  }
		;

enum_head:	   ENUM
			={  $$ = bstruct((struct symtab *)NULL,0); stwart = SEENAME; }
		|  ENUM NAME
			={  $$ = bstruct((struct symtab *)$2,0); stwart = SEENAME;
# ifdef CXREF
			    def($2, id_lineno);
# endif
			    }
		;

enum_dcl_suffix:   moe_list enum_optcomma RC	
                |  enum_optcomma RC 
                        ={  
			  /* "declaration list of %s should be nonempty" */
			  UERROR( MESSAGE( 179 ), "enumeration" );
			}
                ;
moe_list:	   moe
		|  moe_list CM moe
		;

moe:		   NAME
			={  moedef( $1 );
# ifdef CXREF
			    def($1, id_lineno);
# endif
			    }
		|  NAME ASSIGN con_e
#ifndef ANSI
			={  strucoff = $3;  moedef( $1 );
#else 
			={  last_enum_const = strucoff = $3;  moedef( $1 );
#endif
# ifdef CXREF
			    def($1, id_lineno);
# endif
			    }
		;

struct_dcl:	   str_head LC struct_dcl_suffix
                        ={ $$ = dclstruct($1);  }
		|  STRUCT NAME
			={  $$ = rstruct($2,$1);
# ifdef CXREF
			    ref($2, id_lineno);
# endif
# ifdef HPCDB
                            if (in_comp_unit)
			       $2->sflags |= SDBGREQD;
# endif

			    }
                |  STRUCT   /* shift/reduce conflict */
                         ={  
			    /* "%s keyword should be followed by identifier 
			        or declaration list" */
			    UERROR( MESSAGE( 165 ), "STRUCT/UNION" );
			    $$ = MKTY(STRTY,0,INT);  
			    curclass = SNULL;
			  }
		;

str_head:	   STRUCT
			={  $$ = bstruct((struct symtab *)NULL,$1);  stwart=0; }
		|  STRUCT NAME
			={  $$ = bstruct((struct symtab *)$2,$1);  stwart=0; 
# ifdef CXREF
			    def($2, id_lineno);
# endif
			    }
		;

struct_dcl_suffix: type_dcl_list optsemi RC
                |  optsemi RC
                |  error RC
                ;

#ifdef APEX
attribute_spec: NAME
                | NAME LP NAME RP
                | NAME LP ICON RP
                | NAME LP NAME CM NAME RP
                | QUAL
                | QUAL LP NAME RP
                | LP attribute_spec RP
                ;
sharp_prefix:   SHARP
                        ={ opt_attr_flag = 1; }
domain_ext:     sharp_prefix ATTRIBUTE LB attribute_spec RB
                        ={  warn_domain(ATTRIBUTE); }
                |  ATTRIBUTE LP attribute_spec RP
                        ={  warn_domain(ATTRIBUTE); }
                |  sharp_prefix OPTIONS LB attribute_spec RB
                        ={  warn_domain(OPTIONS); }
                |  OPTIONS LP attribute_spec RP
                        ={  warn_domain(OPTIONS); }
                |  sharp_prefix OPTIONS LP attribute_spec RP
                        ={  warn_domain(OPTIONS); }
                ;
#endif

type_dcl_list:	   type_declaration
		|  type_dcl_list semicolon type_declaration 
		;

type_declaration:  typcl_list declarator_list
			={
			   if (was_class_set) UERROR( MESSAGE(133));
			   curclass = SNULL;
			   stwart = 0;
			   $1->in.op = FREE;
			}
		|  typcl_list
#ifndef ANSI
			={  if( curclass != MOU ){
				if (was_class_set) UERROR( MESSAGE(133));
				curclass = SNULL;
				}
			    else {
				(void)sprntf( fakename, "$%dFAKE", fake++ );
				defid( tymerge($1, bdty(NAME,NIL,lookup( fakename, SMOS ))), curclass );
				/* "structure typed union member must be named" */
				WERROR( MESSAGE( 106 ) );
				}
			    stwart = 0;
			    $1->in.op = FREE;
			    }
#else /*ANSI*/
			={
			   if (was_class_set) UERROR( MESSAGE(133));
			   curclass = SNULL;
			   stwart = 0;
			   $1->in.op = FREE;
         		   /* "empty declaration" */
			   WERROR( MESSAGE( 137 ) );
			 }
#endif /*ANSI*/
		;

declarator_list:   declarator
			={ defid( tymerge($<nodep>0,$1), curclass);  stwart = instruct; }
		|  declarator_list  CM {$<nodep>$=$<nodep>0;}  declarator
			={ defid( tymerge($<nodep>0,$4), curclass);  stwart = instruct; }
		;
declarator:	   nfdeclarator
#ifdef APEX
                |  nfdeclarator domain_ext
#endif
		|  nfdeclarator COLON con_e
			%prec CM
			/* "field '%s' should be within a struct/union" */
			={ if( !( instruct & ( INSTRUCT|INUNION) )) {
				UERROR( MESSAGE( 38 ), 
				       ($1->nn.sym->sname != NULL ?
					$1->nn.sym->sname : ""));
				$$ = $1;
			   }
			   else {
			   if( $3<0 || $3 >= FIELD ){

				if( $1->nn.sym->sname == NULL )
				     /* "bitfield size of '%d' 
					 is out of range" */
				     UERROR( MESSAGE( 56 ), $3 );
				else
				     /* "bitfield size of '%d' for '%s' 
					 is out of range" */
				     UERROR( MESSAGE( 64 ), $3, 
					                  $1->nn.sym->sname );

				$3 = 1;
			   }
			   /* If bitfields are not signed and the "signed"
			      keyword is not seen then make the bitfield
			      unsigned */
#ifdef SILENT_ANSI_CHANGE
			   if (silent_changes && (!type_signed && !ISUNSIGNED(($<nodep>0)->in.type)))
			       werror("bitfield is signed in ANSI mode and unsigned in compatibility mode");
#endif
			   if (!(compatibility_mode_bitfields) &&
				 (!signed_bitfields && !type_signed))
			       mkunsigned($<nodep>0);

			    defid( tymerge($<nodep>0,$1), FIELD|$3);
			    $$ = NIL;
		            }
			    }
		|  COLON con_e
			%prec CM
			={  
			     if( !(instruct&(INSTRUCT|INUNION) )) {
				  /* "unnamed field should be within a struct/union" */
				  UERROR( MESSAGE( 212 ) );
			     }
			     else {
				  (void)falloc( (struct symtab *)NULL, $2, -1, 
					       $<nodep>0 );  /* alignment or hole */
			     }
			     $$ = NIL;
			}
		|  error
			={  $$ = NIL; }
		;

		/* int (a)();   is not a function --- sorry! */
nfdeclarator:	   MUL nfdeclarator		
			={  umul:
				$$ = bdty( UNARY MUL, $2, (struct symtab *)0 ); }
		|  MUL tattrib nfdeclarator
			={	$$ = bdty( UNARY MUL, $3, (struct symtab *)0 ); 
			        $$->in.tattrib = $2->in.tattrib;
				$2->in.op = FREE;
			      }
#ifdef LINT
#ifdef xcomp300_800
		  /* recognize the long pointer syntax, for parsing
		   * only -- doesn't properly treat it special */
		|  ER nfdeclarator		
			={  goto umul; }
		|  ER tattrib nfdeclarator
			={	$$ = bdty( UNARY MUL, $3, (struct symtab *)0 ); 
			        $$->in.tattrib = $2->in.tattrib;
				$2->in.op = FREE;
			      }
#endif
#ifdef APEX
                |  AND nfdeclarator
                        ={   warn_ref_parm();	$$ = $2; }
                |  AND tattrib nfdeclarator
                        ={   warn_ref_parm();	$$ = $3; }
#endif
#endif	/* LINT */
#ifndef ANSI
		|  nfdeclarator  LP   RP
			={  uftn:
				$$ = bdty( UNARY CALL, $1, (struct symtab *)0 );  }
#else /* ANSI */
		|  nfdeclarator  proto_LP oparameter_type_list proto_RP		
			{  
				$$ = bdty( CALL, $1, (struct symtab *)$3 ); 
				}
#endif /* ANSI */
		|  nfdeclarator LB RB		
			={  uary:
				$$ = bdty( LB, $1, (struct symtab *)0 );  }
		|  nfdeclarator LB con_e RB	
			={  bary:
			        /* "array subscript should be nonnegative integral constant" */
				if( (int)$3 <= 0 ) UERROR( MESSAGE( 119 ) );
				$$ = bdty( LB, $1, (struct symtab *)$3 );  }
		|  NAME  		
			={  $$ = bdty( NAME, NIL, $1 ); 
# ifdef CXREF
			    def($1, id_lineno);
# endif
			    }
#ifdef ANSI
		|   TYPE_DEF
			{ $$ = bdty( NAME, NIL, idname );
#ifdef CXREF
			  id_lineno = lineno;
			  def(idname, id_lineno);
#endif
			}
#endif
		|   LP  nfdeclarator  RP 		
			={ $$=$2; }
		;


fdeclarator:	   MUL fdeclarator
			={  goto umul; }
#ifdef LINT
#ifdef xcomp300_800
		/* recognize long pointer syntax, but don't handle the
		 * semnatics. 
		 */
		|  ER fdeclarator
			={ goto umul; }
#endif
#ifdef APEX
                |  AND fdeclarator
                        ={   warn_ref_parm();	$$ = $2; }
                |  fdeclarator proto_LP oparameter_type_list proto_RP domain_ext
                        ={  $$ = bdty( CALL, $1, (struct symtab *)$3 );
                        }
		|  name_lp proto_RP domain_ext
			{
			if (fdecl_style==FDECL_NULL) fdecl_style = FDECL_OLD;
			$$ = bdty( CALL, bdty(NAME,NIL,$1), 
			(struct symtab *)make_proto_header() );
			}
                |  name_lp  name_or_paramtype_list  proto_RP domain_ext
                        { register NODE * ph;
                           ph = proto_head[blevel+1];
                           if (ph==NIL) cerror("bad prototype");

                           if (fdecl_style==FDECL_NULL)
                                fdecl_style = (ph->ph.flags&SPARAM) ? FDECL_NEW:
                                        FDECL_OLD;

                           if( ph->ph.flags&SFARGID && blevel!=0 )
                                /* "parameter name list should occur only
                                    in function definition" */
                                UERROR( MESSAGE( 190 ) );
#ifdef IRIF
                           if( ph->ph.flags & SELLIPSIS )
                                $1->sflags |= SELLIPSIS;
#endif /* IRIF */
                           $$ = bdty( CALL, bdty(NAME,NIL,$1), (struct symtab *)
ph );
                           proto_head[blevel+1] = NIL;
                           }
#endif
#endif
#ifndef ANSI
		|  fdeclarator  LP   RP
			={  $$ = bdty( UNARY CALL, $1, (struct symtab *)0 ); }
#else /* ANSI */
		|  fdeclarator proto_LP oparameter_type_list proto_RP
			={  $$ = bdty( CALL, $1, (struct symtab *)$3 );
			}
#endif /* ANSI */
		|  fdeclarator LB RB
			={  goto uary; }
		|  fdeclarator LB con_e RB
			={  goto bary; }
		|   LP  fdeclarator  RP
			={ $$ = $2; }
# ifndef ANSI
                |  name_lp  name_list  RP
			={
				if( blevel!=0 ) 
				     /* "parameter name list should occur 
					 only in function definition" */
				     UERROR( MESSAGE( 190 ) );
				else paramflag = 1;

       				$$ = bdty( UNARY CALL, bdty(NAME,NIL,$1), (struct symtab *)0 );
				stwart = 0;
				}
		|  name_lp RP
			={
				paramflag = 0;
				$$ = bdty( UNARY CALL, bdty(NAME,NIL,$1), (struct symtab *)0 );
				stwart = 0;
				}
# else
		|  name_lp  name_or_paramtype_list  proto_RP
			 { register NODE * ph;
			   ph = proto_head[blevel+1];
			   if (ph==NIL) cerror("bad prototype");

			   if (fdecl_style==FDECL_NULL) 
				fdecl_style = (ph->ph.flags&SPARAM) ? FDECL_NEW:
					FDECL_OLD;

			   if( ph->ph.flags&SFARGID && blevel!=0 ) 
				/* "parameter name list should occur only 
				    in function definition" */
				UERROR( MESSAGE( 190 ) );
#ifdef IRIF
			   if( ph->ph.flags & SELLIPSIS )
				$1->sflags |= SELLIPSIS;
#endif /* IRIF */
			   $$ = bdty( CALL, bdty(NAME,NIL,$1), (struct symtab *)ph );
			   proto_head[blevel+1] = NIL;
#ifndef APEX	/* apex does this in proto_RP reduction */
			   stwart = 0;
#endif
			   }

		|  name_lp proto_RP
			{
				if (fdecl_style==FDECL_NULL) fdecl_style = FDECL_OLD;
				$$ = bdty( CALL, bdty(NAME,NIL,$1), 
					(struct symtab *)make_proto_header() );
#ifndef APEX	/* apex does this in proto_RP reduction */
				stwart = 0;
#endif
				}
# endif  /* ANSI */
		;

# ifdef ANSI
	/* Productions for describing prototypes. */

	/* proto_LP and proto_RP : match the LP and RP tokens in the
	 * context of a function declaration, and drive the save/restore
	 * of global variables that describe the current scan/parse
	 * context.
	 */
proto_LP:  LP
		{ save_parse_context(); }
	   ;

proto_RP:  RP
		{ restore_parse_context(); 
#ifdef APEX
		  stwart = 0;
#endif
		}
	   ;

	
	/* Abstract declarators are basically the same things allowed
	 * for cast types.  So these productions are derived from those
	 * for casts, except that they need to be grouned by a dummy
	 * 'SNONAME' for defid purposes.
	 *
	 * Note that the
	 *      LP RP
	 * case really is
	 *      proto_LP proto_RP
	 * but is left as LP RP because using the proto-LP, ... causes a
	 * reduce/reduce conflict with the "empty" case, and the save/restore
	 * of parse context is not needed since there is no parameter list.
	 */
abstract_declarator:	
		   /* empty */
			={ $$ = bdty( NAME, NIL, make_snoname() ); }
		|  LP RP
			={ $$ = bdty( CALL, bdty(NAME,NIL,make_snoname()),
				(struct symtab *)make_proto_header() ); }
		|  proto_LP parameter_type_list proto_RP
			={ $$ = bdty( CALL, bdty(NAME,NIL,make_snoname()), 
							(struct symtab *)$2 ); }
		|  LP abstract_declarator RP proto_LP oparameter_type_list
			proto_RP
			={  $$ = bdty( CALL, $2,  (struct symtab *)$5 ); }
		|  MUL abstract_declarator
			={  goto umul; }
		|  MUL tattrib abstract_declarator
			={	$$ = bdty( UNARY MUL, $3, (struct symtab *)0 ); 
			        $$->in.tattrib = $2->in.tattrib;
				$2->in.op = FREE;
			      }
#ifdef LINT
#ifdef xcomp300_800
		   /* accept long pointer syntax;  but doesn't handle
		    * semantics -- just treat like a pointer.
		    */
		|  ER abstract_declarator
			={  goto umul; }
		|  ER tattrib abstract_declarator
			={	$$ = bdty( UNARY MUL, $3, (struct symtab *)0 ); 
			        $$->in.tattrib = $2->in.tattrib;
				$2->in.op = FREE;
			      }
#endif
#ifdef APEX
                |  AND abstract_declarator
                        ={   warn_ref_parm();	$$ = $2; }
                |  AND tattrib abstract_declarator
                        ={   warn_ref_parm();	$$ = $3; }
#endif
#endif /* LINT */
		|  abstract_declarator LB RB
			={  goto uary; }
		|  abstract_declarator LB con_e RB
			={  goto bary;  }
		|  LP abstract_declarator RP
			={ $$ = $2; }
		;


	/* optional parameter list */
oparameter_type_list:
		   /* empty */
			{ $$ = make_proto_header(); } 
		|  parameter_type_list
			{ $$ = $1; }
		;

	/* non-empty parameter list */
parameter_type_list:
		   { open_proto_scope();
		     proto_head[blevel]->ph.flags |= SPARAM;
		     }  parameter_type_list1
			{ $$ = proto_head[blevel];
			  close_proto_scope();
			  }
		;

parameter_type_list1:
		   parameter_list
		|  parameter_list CM ELLIPSIS
			{ proto_head[blevel]->ph.flags |= SELLIPSIS;
			}
		;

parameter_list:    parameter_declaration
		   	{ register NODE * ph;
			  ph = proto_head[blevel];
			  ph->ph.flags |= SPARAM;
			  ph->ph.nparam++;
			  ph->ph.phead = slevel_head(blevel);
			  }

		|  parameter_list CM parameter_declaration
		   	{ register NODE * ph;
			  ph = proto_head[blevel];
			  ph->ph.nparam++;
			  ph->ph.flags |= SPARAM;
			  ph->ph.phead = slevel_head(blevel);
			  }
		| error
		;

parameter_declaration:
		   attributes declarator
		   { defid_fparam($1, $2, curclass);
		     $1->in.op = FREE;
		     curclass = SNULL;
		     stwart = 0;
		     }
		|  attributes abstract_declarator
			{ defid_fparam($1, $2, curclass);
			  $1->in.op = FREE;
			  curclass = SNULL;
			  stwart = 0;
			}
		;

	/* List of names and/or parameter types */
name_or_paramtype_list:
			{ open_proto_scope(); }    name_or_paramtype_list0  
			 { close_proto_scope(); }
		;
name_or_paramtype_list0:
		   name_or_paramtype_list1
		|  name_or_paramtype_list1 CM ELLIPSIS
			{ proto_head[blevel]->ph.flags |= SELLIPSIS;
			}
		;


name_or_paramtype_list1:
		   name_or_paramtype 
		|  name_or_paramtype_list1  CM name_or_paramtype
		|  error
		;

name_or_paramtype:
		   NAME			
			={ register NODE * ph;
			   ftnarg( $1 );
			   ph = proto_head[blevel];
			   ph->ph.flags |= SFARGID;
			   ph->ph.nparam++;
			   ph->ph.phead = slevel_head(blevel);
# ifdef CXREF
			   def($1, id_lineno);
# endif
			   }
		|  parameter_declaration			
			={ register NODE * ph;
			   ph = proto_head[blevel];
			   ph->ph.flags |= SPARAM;
			   ph->ph.nparam++;
			   ph->ph.phead = slevel_head(blevel);
			   }
		;

# endif /* ANSI */


# ifndef ANSI
name_lp:	  NAME LP
# else
name_lp:	  NAME proto_LP
# endif
			={
# ifdef CXREF
				newf($1, id_lineno);
				def($1, id_lineno);
				bbcode();
# endif
				/* In compatibility mode, turn off typedefs
				 * for argument names */
# ifndef ANSI
				stwart = SEENAME;
# endif
# ifdef ANSI
				/* save curclass because defid of the
				 * function name now comes
				 * after all the arguments have been
				 * declared.  Need to access the value
				 * via curclass_save, because proto_LP
				 * already reset the global 'curclass'.
				 */
				curftn_class = curclass_save[blevel];
# endif
				if( $1->sclass == SNULL )
				    $1->stype = FTN;
				}
		;

# ifndef ANSI
	/* parameter id-list for non-ANSI function definitions. */
name_list:	   NAME			
			={ ftnarg( $1 );  stwart = SEENAME;
# ifdef CXREF
			   def($1, id_lineno);
# endif
			   }
		|  name_list  CM  NAME 
			={ ftnarg( $3 );  stwart = SEENAME;
# ifdef CXREF
			   def($3, id_lineno);
# endif
			   }
		| error
		;
# endif  /* not ANSI */

		/* always preceeded by attributes: thus the $<nodep>0's */
init_dcl_list:	   init_declarator
			%prec CM
		|  init_dcl_list  CM {$<nodep>$=$<nodep>0;}  init_declarator
		;
		/* always preceeded by attributes */
xnfdeclarator:	   nfdeclarator
			={
			 defid( $1 = tymerge($<nodep>0,$1), curclass);
#ifdef SA
                            /* Mark initializations */
			 if (saflag)
			   {
			   add_xt_info($1->nn.sym, MODIFICATION);
			   if (((BTYPE($1->nn.sym->stype)==STRTY) ||
				(BTYPE($1->nn.sym->stype)==UNIONTY)) &&
                                   /* AND there are members/fields */
				dimtab[$1->nn.sym->sizoff+1])
			     {
			     int index;
			     for (index=dimtab[$1->nn.sym->sizoff+1];
				     dimtab[index];index++)
				{
				/* Change the line number to that
				   of the variable or else you show
				   fields modified at their last use */
				((struct symtab *)dimtab[index])->suse
					= $1->nn.sym->suse;
				add_xt_info(dimtab[index], MODIFICATION);
				}
			     } 
			   }
#endif			    
			 beginit($1->nn.sym,TRUE);
			 }
#ifdef APEX
	    	|  nfdeclarator domain_ext
			={
			 defid( $1 = tymerge($<nodep>0,$1), curclass);
			 beginit($1->nn.sym,TRUE);
			 }
#endif
		|  error
		;
		/* always preceeded by attributes */
init_declarator:   nfdeclarator
			/*={  nidcl( tymerge($<nodep>0,$1) ); }*/
			{   /* kludge here.  Reduce the type, and
			     * then see if it is really a FTN, which
			     * can happen if a typedef was used. If
			     * it is then jump down into the fdeclarator
			     * case. 
			     */
			     
			    qidecl = tymerge($<nodep>0,$1);
			    if ( qidecl!=NIL && ISFTN(qidecl->in.type) )
				/* it's really an fdeclarator even though
				 * it doesn't look like it.
				 */
				goto fdecl;
			    nidcl( qidecl );
			    }
#ifdef APEX
                |  nfdeclarator domain_ext
                        ={
                            /* same as above */
                            qidecl = tymerge($<nodep>0,$1);
                            if ( qidecl!=NIL && ISFTN(qidecl->in.type) )
                                goto fdecl;
                            nidcl( qidecl );
                        }
#endif
		|  fdeclarator
# ifndef ANSI
			={ 
			/*defid( tymerge($<nodep>0,$1), uclass(curclass) );*/
# ifdef CXREF
			becode();
# endif
			qidecl = tymerge($<nodep>0,$1);
		      fdecl:
			  defid( qidecl, uclass(curclass) );
# ifdef SA
                          if (saflag)
		   	    add_xt_info($1->nn.sym, (curclass==TYPEDEF) ?
			            DECLARATION|DEFINITION : DECLARATION);
# endif				
			if ( paramflag ) {
			     /* "parameter name list should occur only
				 in function definition" */
			     UERROR( MESSAGE( 190 ));
			}
			paramflag = 0;
			}
# else  /* ANSI */
			{ NODE * ph;
# ifdef CXREF
			  becode();
# endif
			  qidecl = tymerge($<nodep>0,$1);
			fdecl:
			  ph = (NODE *) dimtab[qidecl->fn.cdim];
			  if (fdecl_style==FDECL_OLD && ph->ph.nparam>0 )  {
			       if( blevel == 0 ) {
				    /* "parameter name list should occur 
				        only in function definition" */
				    UERROR( MESSAGE( 190 ) );
			       }
				/* delete the parameter id symbols or leave
				 * them linked to the phead ???
				 * If going to leave them, then do we need
				 * to set a default type???
				 */
				ph->ph.nparam = 0;
				/*ph->ph.phead = NULL_SYMLINK;*/  /* ??? */
				}
			  defid( qidecl, uclass(curclass) );
# ifdef SA
                          if (saflag)
		   	    add_xt_info($1->nn.sym, (curclass==TYPEDEF) ?
			            DECLARATION|DEFINITION : DECLARATION);
# endif				
			  fdecl_style = FDECL_NULL;
			  }
# endif /* ANSI */
#ifdef ANSI
		|  xnfdeclarator ASSIGN initializer
			={  endinit(); }
#else
		|  xnfdeclarator ASSIGN e
			%prec CM
			={ doinit( $3 );
			   endinit(); }
		|  xnfdeclarator ASSIGN LC init_list optcomma RC
			={ endinit(); }
#endif		
		| error
		;

init_list:	   initializer
			%prec CM
		|  init_list  CM  initializer
		;
initializer:	   e
			%prec CM
			={  doinit( $1 ); }
		|  ibrace init_list optcomma RC
			={  irbrace(); }
		;

ibrace		: LC
                        ={  ilbrace(); }
		;

enum_optcomma	:	/* VOID */
		|  CM
                   ={
#ifdef ANSI
		     /* "\",\" at end of declaration list ignored" */
		     WERROR( MESSAGE( 182 ) );
#endif
		   }
		;

optcomma	:	/* VOID */
		|  CM
		;

optsemi		:	/* VOID */
                   ={
#ifdef ANSI
		     /* "missing \";\" assumed to indicate end of 
			 declaration list" */
		     WERROR( MESSAGE( 181 ) );
#endif
		   }
		| SM
		;

semicolon       : SM
                ;


/*	STATEMENTS	*/

compoundstmt:	   begin dcl_stat_list 
			{
# ifdef HPCDB
			  if (cdbflag && dcl_list_empty)
				cdb_startscope(0,LC_lineno);
# endif
			}
			stmt_list  RC
			={  
# ifdef ANSI
			    NODE * ph;
			    int flags;
# ifdef DEBUGGING
			    SYMLINK p;
# endif
# endif /* ANSI */
# ifdef CXREF
			    becode();
# endif
# ifdef HPCDB
			    if (cdbflag) {
			    	cdb_local_names(blevel);
				}
# endif
# ifndef LINT
			    clearstab( blevel );
# else
			    clearstab( blevel, 1 ); /* flag == call aocode() */
# endif /* LINT */
			    blevel--;
			    if ( blevel == 1) {
# ifdef HPCDB
				if (cdbflag) cdb_local_names(blevel);
# endif
#ifndef ANSI
				clearstab(blevel);
# else  /* ANSI */
				/* new names could have been added to level 1
				 * via an old-style arg decl list.
				 * ?? Do we need to save these names too??
				 */
				ph = (NODE *)dimtab[curftn->dimoff];
				flags = ph->ph.flags;
				if (flags&SPARAM && flags&SFARGID) {
				   /* there was a previous prototype,
				    * but now we are completing an old-
				    * style function definition.  The
				    * prototype should stay but does
				    * not need to be unscoped, and the
				    * arg dcl list should be deleted.
				    */
# ifdef DEBUGGING
				    p = next_param(slevel_head(blevel));
				    if ( p != NULL_SYMLINK && p == ph->ph.phead )
					cerror("internal error: bad parameter list at end of function");
# endif
# ifndef LINT
				    clearstab(blevel);
# else
				    clearstab(blevel, 1);
# endif /* LINT */
				    ph->ph.flags &= ~SFARGID;
				    }
				else if (flags&SFARGID) {
				   /* this was an old-style defn, with
				    * no previous prototype.  Save the
				    * info for future type checking.
				    */
# ifdef DEBUGGING
				   /* Debugging check:  even if new names were
				    * added, we should be able to find ph.phead
				    * as we traverse the slevel_head list, 
				    * and it should be the first "paramter-type"
				    * symbol we find */
				   p = next_param(slevel_head(blevel));
				   if ( p != ph->ph.phead ) 
				      cerror("internal error: bad parameter list at end of function");
# endif
				   ph->ph.phead = slevel_head(blevel);
				   unscope_parameter_list((NODE *)dimtab[curftn->dimoff],
					blevel);
				   }
				else if (flags&SPARAM) {
# ifdef DEBUGGING
				   p = next_param(slevel_head(blevel));
				   if ( p != ph->ph.phead )
					cerror("internal error: bad parameter list at end of function");
# endif
				   /* just unscope the parameter list */
				   unscope_parameter_list((NODE *)dimtab[curftn->dimoff],
					blevel);
				   }
				else {
				   /* should only be names to get rid of
				    * in an error case.
				    */
# ifndef LINT
				   clearstab(blevel);
# else
				   clearstab(blevel,0);  /*don't call aocode*/
# endif /* LINT */
				   }

# endif /* ANSI */
				blevel = 0;
			    }
#ifdef HPCDB
			    if (cdbflag) cdb_endscope();
#endif
			    checkst(blevel);
#ifdef IRIF
			    ir_block_end(blevel);
#else
			    restore_autooff();
#endif /* IRIF */
			    regvar = *--psavbc;
#ifndef IRIF
# ifdef C1_C
			    if (optlevel < 2) putsetregs(); /* release regs */
# endif /* C1_C */
#endif /* not IRIF */
			    fdregvar = *--psavbc;
# ifdef LINT_TRY
                            /* be quiet about 'if (a) {}' */
                            nullif = IFOK;
# endif
			    }
		;

begin:		  LC
			={
# ifdef HPCDB
			    if (cdbflag) {
				LC_lineno = lineno;
				dcl_list_empty = 1;
			    }
# endif
			    if( blevel == 1 ) dclargs();
# ifdef CXREF
			    else if (blevel > 1) bbcode();
# endif
			    ++blevel;
			    if (blevel >= NSYMLEV)
				cerror("too many lexical scopes");
			    if( psavbc > &asavbc[maxbcsz-3] ) newbctab();
			    *psavbc++ = fdregvar;
			    *psavbc++ = regvar;
#ifdef IRIF
			    ir_block_begin(blevel);
#else
			    *psavbc++ = autooff;
#endif /* IRIF */
			    }
		;

statement:	   e   semicolon
			={ ecomp( $1 ); }
		|  compoundstmt
		|  ifprefix statement
			={ deflab((unsigned)$1);
			   reached = 1;
# ifdef LINT_TRY
                           if (hflag && nullif) WERROR (MESSAGE(233) );
                           nullif = IFOK;
# endif
			   }
		|  ifelprefix statement
			={  if( $1 != NOLAB ){
				deflab( (unsigned)$1 );
				reached = 1;
# ifdef LINT_TRY
                                if (hflag && nullif) WERROR (MESSAGE(234) );
                                nullif = IFOK;
# endif
				}
			    }
		|  whprefix statement
			={ 
#if !defined(IRIF) || defined(HAIL)
			    branch( (unsigned)contlab );
#else /* IRIF */
			    /* For IRIF, code for
			     *
			     *     while( B ) S;
			     *
			     * is generated as if written
			     *
			     *     if( B ) {
			     *             do S;
			     *             while( B );
			     *     }
			     */

			    deflab( (unsigned)contlab );
			    ir_cond_jump( looplab, SCONVCK( $1 ), TRUE );
			    tfree( $1 );
#endif /* IRIF */
			    deflab( (unsigned)brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP)) reached = 1;
			    else reached = 0;
			    resetbc(0);
# ifdef LINT_TRY
                            nullif = IFOK;
# endif
			    }
		|  doprefix statement WHILE  LP  e  RP   semicolon
			={  
			    if( !WILL_BE_SCALAR( $5->in.type )) {
			         /* "controlling expression of '%s' should
				     have %s type" */
			         UERROR( MESSAGE( 168 ), "while", "scalar" );
				 $5->in.type = INT;  /* to get thru this */
			    }
			    deflab( (unsigned)contlab );
			    if( flostat & FCONT ) reached = 1;
#ifndef IRIF
			    ecomp( buildtree( CBRANCH, buildtree( NOT, $5, NIL ), bcon( (int)$1, INT ) ) );

#else /* IRIF */
			    $5 = do_optim( $5 );
#ifdef HPCDB
			    if( cdbflag ) (void)sltnormal();
#endif /* HPCDB */
			    ir_cond_jump( $1, SCONVCK( $5 ), TRUE ); 
			    tfree( $5 );
#endif /* IRIF */
			    deflab( (unsigned)brklab );
			    reached = 1;
			    resetbc(0);
# ifdef LINT_TRY
                            nullif = IFOK;
# endif
		       }

		|  forprefix .e RP statement
			={  deflab( (unsigned)contlab );
			    if( flostat&FCONT ) reached = 1;
			    if( $2 ) ecomp( $2 );

#if !defined(IRIF) || defined(HAIL)
			    branch( (unsigned)$1 );
#else /* IRIF and !HAIL */
			    /* For IRIF, code for
			     *
			     *     for( A;B;C ) S;
			     *
			     * is generated as if written
			     *
			     *     A;
			     *     if( B ) {
			     *             do S;
			     *             while( ( C,B ));
			     *     }
			     */

			    if( $1 ) {
				 ir_cond_jump( looplab, SCONVCK( $1 ), TRUE );
				 tfree( $1 );
			    }
			    else {
				 branch( looplab );
			    }
#endif /* IRIF and !HAIL */

			    deflab( (unsigned)brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP) ) reached = 1;
			    else reached = 0;
			    resetbc(0);
# ifdef LINT_TRY
                            nullif = IFOK;
# endif
			    }

		| switchpart statement
		     ={  if( reached ) branch( (unsigned)brklab );
#ifndef IRIF
			 if (!(flostat&FDEF) ) swdefend();
			 deflab( (unsigned)$1 );
#else /* IRIF */
			 deflab( (unsigned) swlab );
			 $1 = do_optim( $1 );
			 ir_switch_test( $1 );
			 tfree( $1 );
#endif /* IRIF */
			 swend();
			 deflab((unsigned)brklab);
			 if( (flostat&FBRK) || !(flostat&FDEF) ) reached = 1;
			 resetbc(FCONT);
# ifdef LINT_TRY
			 nullif = IFOK;
# endif
		    }
		|  BREAK  semicolon
			/* "illegal break" */
			={  if( brklab == NOLAB ) UERROR( MESSAGE( 50 ) );
			    else if(reached) {
# ifdef HPCDB
				if (cdbflag) (void)sltnormal();
# endif
				branch( (unsigned)brklab );
				}
			    flostat |= FBRK;
# ifdef LINT_TRY
                            nullif = IFOK;
# endif
			    if( brkflag ) goto rch;
			    reached = 0;
			    }
		|  CONTINUE  semicolon
			/* "illegal continue" */
			={  if( contlab == NOLAB ) UERROR( MESSAGE( 55 ) );
			    else {
# ifdef HPCDB
				if (cdbflag) (void)sltnormal();
# endif
				branch( (unsigned)contlab );
				}
			    flostat |= FCONT;
			    goto rch;
			    }
		|  RETURN  semicolon
			={  retstat |= NRETVAL;
# ifdef HPCDB
			    if (cdbflag) (void)sltnormal();
# endif

#ifdef IRIF
			    ir_return( retlab );
#else /* not IRIF */
#ifdef C1_C
			    if ((DECREF(curftn->stype) == LONGDOUBLE) ||
				(DECREF(curftn->stype) == STRTY) ||
				(DECREF(curftn->stype) == UNIONTY)) {
				put_addr_return();    /* cpass1 copies here */
				}
#endif /* C1_C */
			    branch( svfdefaultlab );
#endif /* not IRIF */
			rch:
			    /* "statement not reached" */
# ifdef LINT_TRY
			    if( !reached && !reachflg ) WERROR( MESSAGE( 100 ) );
			    reachflg = 0;
			    nullif = IFOK;
# else
			    if( !reached ) WERROR( MESSAGE( 100 ) );
# endif
			    reached = 0;
			    }
		|  RETURN e  semicolon
			={  register NODE *temp;
			    idname = curftn;
			    temp = buildtree( NAME, NIL, NIL );
			    if(temp->in.type == FVOID)
				/* "void function %s cannot return value" */
				UERROR( MESSAGE( 116 ),
					idname->sname);
			    temp->fn.cdim++;	/* past prototype pointer */
			    temp->in.type = DECREF( temp->in.type );
			    temp->in.tattrib = DECREF( temp->in.tattrib );
#ifdef SEQ68K
			    if (temp->in.type == FLOAT)
				temp->in.type = DOUBLE;
#endif
			    temp = buildtree( RETURN, temp, $2 );
			    /* now, we have the type of the RHS correct */
#ifdef IRIF
			    temp->in.right = do_optim( temp->in.right );
			    ir_return_value( temp->in.right, curftn );
			    tfree( $2 );
			    tfree( temp );
# ifdef HPCDB
			    if (cdbflag) (void)sltnormal();
# endif /* HPCDB */
			    ir_return( retlab );
# ifdef LINT_TRY
			    if( !reached && !reachflg)
# else
			    if( !reached )
# endif
				 {
				 /* "statement not reached" */
				 WERROR( MESSAGE( 100 ) );
				 reached = 1;
			    }
#else /* not IRIF */

#ifdef C1_C
			    putreturn(temp);    /* cpass1 copies here */
#else /* C1_C */
#ifdef ANSI
			    if (temp->in.type == LONGDOUBLE) {
				temp->in.op = ASSIGN;
				temp->in.left->in.op = UNARY MUL;
				temp->in.left->in.left = block(OREG,NIL,NIL,(TWORD)INCREF(temp->in.type),0,INT);
				temp->in.left->in.left->tn.lval = stroffset;
				temp->in.left->in.left->tn.rval = 14;  /* %a6 */
				ecomp(temp);
				temp = block(OREG,NIL,NIL,(TWORD)INCREF(LONGDOUBLE),0,INT);
				temp->tn.lval = stroffset;
				temp->tn.rval = 14;  /* %a6 */
				ecomp(buildtree(FORCE,temp,NIL));
			    }
			    else {
				tfree(temp->in.left);
				temp->in.op = FREE;
				ecomp( buildtree( FORCE,temp->in.right, NIL ));
			    }
#else
			    tfree(temp->in.left);
			    temp->in.op = FREE;
			    ecomp( buildtree( FORCE, temp->in.right, NIL ) );
#endif /* ANSI */
#endif /* C1_C */
			    branch( (unsigned)retlab );
#endif /* not IRIF */
			    retstat |= RETVAL;
			    reached = 0;
# ifdef LINT_TRY
			    reachflg = 0;
# endif
			    }
		|  GOTO NAME semicolon
			={  register NODE *q;
			    q = block( FREE, NIL, NIL, INT, 0, INT );
			    $2 = q->nn.sym = idname = lookup($2->sname,SLAB);
			    defid( q, ULABEL );
			    $2->sflags |= SNAMEREF;
# ifdef HPCDB
			    if (cdbflag) (void)sltnormal();
# endif
			    branch( (unsigned)idname->offset );
# ifdef CXREF
			    ref($2, id_lineno);
# endif
# ifdef SA
                            if (saflag) 
			      add_xt_info($2, USE); 
# endif
			    goto rch;
			    }
		|  semicolon
                        ={
# ifdef LINT_TRY
                                nullif = NULLSTMT;
# endif
                         }
		|  error  semicolon
		|  error RC
		|  label statement
		|  ASM
			{
				(void)read_asm_body();
# ifdef LINT_TRY
                                nullif = IFOK;
# endif
			}  semicolon
		;
label:		   NAME COLON
			={  register NODE *q;
			    q = block( FREE, NIL, NIL, INT, 0, CLABEL );
			    $1 = idname = q->nn.sym = lookup($1->sname,SLAB);
			    defid( q, CLABEL );
			    reached = 1;
# ifdef CXREF
			    def($1, id_lineno);
# endif
#ifdef SA
                            if (saflag) {
			      add_xt_info($1, MODIFICATION);
			      }
#endif			      
			    }
		|  CASE e COLON
			={  addcase($2);
			    reached = 1;
			    }
		|  DEFAULT COLON
			={  reached = 1;
			    adddef();
			    flostat |= FDEF;
			    }
		;
doprefix:	DO
			={  savebc();
			    /* "loop not entered at top" */
			    if( !reached ) WERROR( MESSAGE( 75 ) );
			    brklab = GETLAB();
			    contlab = GETLAB();
			    deflab( (unsigned)($$ = GETLAB()) );
			    reached = 1;
			    }
		;
ifprefix:	IF LP e RP
			={  
			    if( !WILL_BE_SCALAR( $3->in.type )) {
			         /* "controlling expression of '%s' should
				     have %s type" */
				 $3->in.type = INT;  /* to get thru this */
			         UERROR( MESSAGE( 168 ), "if", "scalar" );
			    }
#ifndef IRIF
			    ecomp( buildtree( CBRANCH, $3, bcon( (int)($$=GETLAB()), INT) ) ) ;

#else /* IRIF */
			    $3 = do_optim( $3 );
#ifdef HPCDB
			    if( cdbflag ) (void)sltnormal();
#endif /* HPCDB */
			    ir_cond_jump( $$=GETLAB(), SCONVCK( $3 ), FALSE );
			    tfree( $3 );
			    if( !reached
# ifdef LINT_TRY
			       && !reachflg 
#endif /* LINT_TRY */
			       ) {
				 /* "statement not reached" */
				 WERROR( MESSAGE( 100 ) );
			    }
#endif /* IRIF */
			    reached = 1;
		       }
		;
ifelprefix:	  ifprefix statement ELSE
			={  if( reached ) branch( (unsigned)($$ = GETLAB()) );
			    else $$ = NOLAB;
			    deflab( (unsigned)$1 );
			    reached = 1;
			    }
		;

whprefix:	  WHILE  LP  e  RP
			={  
			    if( !WILL_BE_SCALAR( $3->in.type )) {
			         /* "controlling expression of '%s' should
				     have %s type" */
			         UERROR( MESSAGE( 168 ), "while", "scalar" );
				 $3->in.type = INT;  /* to get thru this */
			    }
			    savebc();
			    /* "loop not entered at top" */
			    if( !reached ) WERROR( MESSAGE( 75 ) );
			    if( $3->in.op == ICON && $3->tn.lval != 0 ) flostat = FLOOP;
			    reached = 1;
			    contlab = GETLAB();
			    brklab = GETLAB();

#if !defined(IRIF) || defined(HAIL)
			    deflab( (unsigned)contlab );
			    if( flostat == FLOOP )
				{
				tfree( $3 );
# ifdef HPCDB
				/* since this is not going through ecomp,  */
				/* generate the slt entry here             */
				if (cdbflag)
					(void) sltnormal();
# endif /* HPCDB */
				}
			    else
# ifdef HAIL
				{
				$3 = do_optim( $3 );
				ir_cond_jump( brklab, SCONVCK( $3 ), FALSE ); 
				tfree( $3 );
				}
# else
				ecomp(buildtree(CBRANCH,$3,bcon(brklab,INT)));
# endif

#else /* IRIF */
			    /* For IRIF, code for
			     *
			     *     while( B ) S;
			     *
			     * is generated as if written
			     *
			     *     if( B ) {
			     *             do S;
			     *             while( B );
			     *     }
			     */

			    if( flostat == FLOOP )
				{
# ifdef HPCDB
				/* since this is not going through ecomp,  */
				/* generate the slt entry here             */
				if (cdbflag)
					(void) sltnormal();
# endif /* HPCDB */
				}
			    else {
				 $3 = do_optim( $3 );
#ifdef HPCDB
				 if( cdbflag ) (void)sltnormal();
#endif /* HPCDB */
				 ir_cond_jump( brklab, SCONVCK( $3 ), FALSE ); 
			    }
			    deflab( (unsigned)(looplab = GETLAB()) );
			    $$ = $3;
#endif /* IRIF */
		       }
		;

forprefix:	  FOR  LP  .e  semicolon .e semicolon
			={  if( $3 ) ecomp( $3 );
			    /* "loop not entered at top" */
			    else if( !reached ) WERROR( MESSAGE( 75 ) );
# ifdef HPCDB
			    /* if no initialzer, emit slt number to   */
			    /* allow breakpoints on the for statement */
			    if (!($3) &&  cdbflag)
				(void) sltnormal();
# endif
			    savebc();
			    contlab = GETLAB();
			    brklab = GETLAB();

#if !defined(IRIF) || defined(HAIL)
			    deflab( (unsigned)($$ = (NODE *)GETLAB()) );
			    reached = 1;

			    if( $5 ) {
			         if( !WILL_BE_SCALAR( $5->in.type )) {
			              /* "controlling expression of '%s' should
				          have %s type" */
			              UERROR( MESSAGE( 168 ), "for", "scalar" );
				      $5->in.type = INT;  /* to get thru this */
				 }
# ifdef HAIL
				 $5 = do_optim( $5 );
				 ir_cond_jump( brklab, SCONVCK( $5 ), FALSE ); 
				 tfree( $5 );
# else
				 ecomp( buildtree( CBRANCH, $5, bcon( brklab, INT) ) );
# endif
			    }
			    else flostat |= FLOOP;
#else /* IRIF and !HAIL */
			    /* For IRIF, code for
			     *
			     *     for( A;B;C ) S;
			     *
			     * is generated as if written
			     *
			     *     A;
			     *     if( B ) {
			     *             do S;
			     *             while( ( C,B ));
			     *     }
			     */
			    if( $5 ) {
			         if( !WILL_BE_SCALAR( $5->in.type )) {
			              /* "controlling expression of '%s' should
				          have %s type" */
			              UERROR( MESSAGE( 168 ), "for", "scalar" );
				      $5->in.type = INT;  /* to get thru this */
				 }
				 $5 = do_optim( $5 );
#ifdef HPCDB
				 if( cdbflag ) (void)sltnormal();
#endif /* HPCDB */
				 ir_cond_jump( brklab, SCONVCK( $5 ), FALSE );
			    }
			    else flostat |= FLOOP;
			    $$ = $5;
			    deflab( (unsigned)( looplab = GETLAB()) );
			    reached = 1;
#endif /* IRIF and !HAIL */
		       }
			    ;

switchpart:	   SWITCH  LP  e  RP
			={  register TWORD t = $3->in.type;
			    TWORD t2 = INT;
#ifndef ANSI
			    if( t & ~BTMASK ) {
			         /* "controlling expression of '%s' should
				     have %s type" */
			         WERROR( MESSAGE( 168 ), "switch", "integral" );
			       }
			    else 
#endif
				 {
			         switch( t ) {

			         case CHAR:
			         case UCHAR:
			         case SCHAR:
			         case SHORT:
			         case USHORT:
			              t2 = SHORT;
				      break;

			         case INT:
			         case UNSIGNED:
			         case LONG:
			         case ULONG:
				      break;
#ifndef ANSI				      
				 case FLOAT:
				 case DOUBLE:
				 case LONGDOUBLE:
			              /* "controlling expression of '%s' should
				          have %s type" */
			              WERROR( MESSAGE( 168 ), "switch", "integral" );
				      break;
#endif
				 default:
			              /* "controlling expression of '%s' should
				          have %s type" */
			              UERROR( MESSAGE( 168 ), "switch", "integral" );
				      $3->in.type = INT;  /* to get thru this */
				      break;
				    }
			    }


			    savebc();
			    brklab = GETLAB();
#ifndef IRIF
			    ecomp( buildtree( FORCE, 
					makety($3,t2,0,(int)t2,0), NIL ) );
			    branch( (unsigned)($$ = (NODE *)GETLAB()) );
#else /* IRIF */
			    branch( (unsigned)( swlab = GETLAB() ) );
			    $$ = $3;
			    if( !reached
# ifdef LINT_TRY
			       && !reachflg 
#endif /* LINT_TRY */
			       ) {
				 /* "statement not reached" */
				 WERROR( MESSAGE( 100 ) );
			    }
#endif /* IRIF */
   			    swstart(t);
			    reached = 0;
		       }
		;
/*	EXPRESSIONS	*/
con_e:		   { $<intval>$=instruct; stwart=instruct=0; } e
			%prec CM
			={  $$ = icons( $2, &icons_error );  instruct=$<intval>1; }
		;
.e:		   e
		|
			={ $$=0; }
		;

	/* elist builds the argument list for a function call.  For
	 * ANSI there is the extra overhead of building a list header
	 * to give an argument count.  This facilitates prototype checking
	 * and handling the ELLIPSIS given that the argument list and
	 * prototype list are in reverse order.
	 */
elist:		   e
			%prec CM
# ifdef ANSI
			{ 
#ifdef SA		  
                          if (saflag)   
			    sawalk_params($1);
#endif			  
			  $$ = block( ARGLIST, $1, (NODE *)1, INT, 0, INT );}
# else
			{ 
#ifdef SA			    
                          if (saflag)   
			    sawalk_params($1);
#endif			  
			  $$ = $1; }
# endif /* ANSI */
		|  elist  CM  e
#ifndef ANSI
			={  
#ifdef SA			    
                           if (saflag)   
  			     sawalk_params($3);
#endif			  
			   goto bop; }
#else
			{  
#ifdef SA			    
                            if (saflag)   
			      sawalk_params($3);
#endif			  
			    $1->in.left = buildtree($2, $1->in.left, $3 );
			    $1->tn.rval++;
			    $$ = $1;
			    }
# endif /* ANSI */
		;

e:		   e RELOP e
			={
			preconf:
			    if( yychar==RELOP||yychar==EQUOP||yychar==AND||yychar==OR||yychar==ER ){
			    precplaint:
#ifdef LINT
				/* "precedence confusion possible: parenthesize!" */
				if( hflag ) WERROR( MESSAGE( 92 ) );
#else
				;
#endif /* LINT */
				}
			bop:
			    $$ = buildtree( $2, $1, $3 );
			    }
		|  e CM e
			={  $2 = COMOP;
			    goto bop;
			    }
		|  e DIVOP e
			={  goto bop; }
		|  e PLUS e
			={  if(yychar==SHIFTOP) goto precplaint; else goto bop; }
		|  e MINUS e
			={  if(yychar==SHIFTOP ) goto precplaint; else goto bop; }
		|  e SHIFTOP e
			={  if(yychar==PLUS||yychar==MINUS) goto precplaint; else goto bop; }
		|  e MUL e
			={  goto bop; }
		|  e EQUOP  e
			={  goto preconf; }
		|  e AND e
			={  if( yychar==RELOP||yychar==EQUOP ) goto preconf;  else goto bop; }
		|  e OR e
			={  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; }
		|  e ER e
			={  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; }
		|  e ANDAND e
			={  goto bop; }
		|  e OROR e
			={  goto bop; }
		|  e ASGNOP e
		        ={
#ifdef SA		
                        if (saflag)
			  sawalk( $1 , $1->in.type, FALSE);
#endif			
		        goto bop; }
		|  e QUEST e COLON e
			={  $$=buildtree(QUEST, $1, buildtree( COLON, $3, $5 ) );
			    }
		|  e ASSIGN e
		        ={
#ifdef SA			
                        if (saflag)
			  sawalk( $1 , $1->in.type, FALSE);
#endif			
		        goto bop; }
		|  term
			={ $$ = fixtag($1); }
		;
term:		   term INCOP
			={  (void)fixtag($1);
#ifdef SA			
                            if (saflag)
			      sawalk( $1 , $1->in.type, FALSE);
#endif			
			    $$ = buildtree( $2, $1, bcon(1,0) );
			}
		|  MUL term
			={ 
#ifdef IRIF
			     if( ISARY( $2->in.type )) {
				  $2 = buildtree( INDEX, $2, bcon( 0, 0 ));
			     }
			     if( $2->in.op == INDEX ) {
				  $2->in.tattrib |= ATTR_NEXT_DIM;
			     }
#endif /* IRIF */
			   ubop:
			     /* fixtag unless an array name is being
			      * dereferenced, e.g. char c[5]; *c = 'a'; */
			     if (!ISARY($2->in.type)) (void)fixtag($2);
#ifdef ANSI
			     /* result of * is always an lvalue */
			     $2->in.not_lvalue = 0;
#endif
			     $$ = buildtree( UNARY $1, $2, NIL );
			}
		|  AND term
			={
				CHECKTERMPTR($2);
#ifndef ANSI
				if( ISFTN($2->in.type) || ISARY($2->in.type) ){
				/* "& before array or function: ignored" */
				WERROR( MESSAGE( 7 ) );
				$$ = $2;
				} else
#endif				     
			    if(  ( SCLASS($2->tn.tattrib) == ATTR_REG )
#ifdef ANSI                      
			       ||( $2->in.op == UNARY MUL  &&
			              (  $2->in.left->in.op == CALL
				       ||$2->in.left->in.op == UNARY CALL
#ifndef IRIF 
				       ||$2->in.left->in.op == STCALL 
				       ||$2->in.left->in.op == UNARY STCALL 
#endif /* not IRIF */
				       ))
#ifndef IRIF
/* check for plus ... icon wrapped around the structure call */
			       ||(($2->in.op == UNARY MUL
				 && $2->in.left->in.op == PLUS)  &&
				  ($2->in.left->in.left->in.op == STCALL ||
				   $2->in.left->in.left->in.op == UNARY STCALL))
#endif /* not IRIF */

			       ||( $2->in.op == QUEST )
#endif /*ANSI*/
			                                     ){
				      /* "unacceptable operand of &" */
				      UERROR( MESSAGE( 110 ) );
				      $$ = $2;
				 }
#ifdef ANSI
			    else if ($2->in.op == COMOP){
				       /* "left-hand side of '%s' should be an lvalue" */
				       WERROR( MESSAGE( 62 ),"&");
				       goto ubop;
				    }
#endif
			    else goto ubop;
			    }
		|  MINUS term
			={  (void)fixtag($2); 
			    $$ = buildtree( UNARY $1, $2, NIL ); }
#ifdef ANSI
                |  PLUS term
                        ={  (void)fixtag( $2 ); 
			    $$ = buildtree( UNARY $1, $2, NIL ); }
#endif
		|  UNOP term
			={
			    (void)fixtag($2);
			    $$ = buildtree( $1, $2, NIL );
			    }
		|  INCOP term
			={  (void)fixtag($2);
#ifdef SA			
                            if (saflag)
			      sawalk( $2 , $2->in.type, FALSE);
#endif			
			    $$ = buildtree( $1==INCR ? ASG PLUS : ASG MINUS,
						$2,
						bcon(1,0)  );
			    }
		|  SIZEOF term
			={
			CHECKTERMPTR($2);
			$$ = doszof( $2 );
			}
		|  LP cast_type RP term  %prec INCOP
			={
			    (void)fixtag($4);
			    $$ = buildtree( CAST, $2, $4 );
			    tfree($$->in.left);
			    $$->in.op = FREE;
			    $$ = $$->in.right;
#ifdef ANSI
			    $$->in.not_lvalue = 1;
#endif
			    }
		|  SIZEOF LP cast_type RP  %prec SIZEOF
			={  
			     if( ISFTN( $3->in.type )) {
				  /* "%s should not be arguments to sizeof()" */
				  UERROR( MESSAGE( 129 ), "function types" );
				  $$ = bcon( SZINT, UNSIGNED );
			     }
			     else if( $3->in.type == VOID ) {
				  /* "%s should not be arguments to sizeof()" */
				  UERROR( MESSAGE( 129 ), "void types" );
				  $$ = bcon( SZINT, UNSIGNED );
			     }
			     else $$ = doszof( $3 ); }
		|  term LB e RB
			={
#ifdef C1_C
#ifndef IRIF
                         /* correct premature tagging of (a)[i], bug don't
                         /* undo fixtag on pointer that's dereferenced
                          * as if it were an array, e.g., s.p[-1] */
                         if (ISARY($1->in.type)) $1->in.fixtag = 0;
#endif /* not IRIF */
#endif /* C1_C */
			    if( !(  ( ISINTEGRAL( $3->in.type ) &&
			            ( ISARY( $1->in.type ) || ISPTR( $1->in.type)))
			          ||( ISINTEGRAL( $1->in.type ) &&
			            ( ISARY( $3->in.type ) || ISPTR( $3->in.type )))))

			         /* "array/array subscript type error" */
			         UERROR( MESSAGE( 170 ));

#ifndef IRIF
			         $$ = buildtree( UNARY MUL, buildtree( PLUS, $1, $3 ), NIL ); 
#else /* IRIF */
			         $$ = buildtree( INDEX, $1, $3 );
			         $$->in.tattrib |= ATTR_NEXT_DIM;
			         $$ = buildtree( UNARY MUL, $$, NIL ); 
#endif /* IRIF */
			}
		|  funct_idn  RP
			={  $$=buildtree(UNARY CALL,$1,NIL); }
		|  funct_idn elist  RP
			={$$=buildtree(CALL,$1,$2); }
		|  term STROP NAMESTRING
			={  
			int attr_reg = 0;
			CHECKTERMPTR($1);
#ifndef IRIF
#ifdef C1_C
                        /* Correct premature tag fixing, e.g. (s).a,
                         * which were fixed because of the term->e
                         * production, but leave tags which were fixed
                         * due to being a pointer
                         */
                        if (!ISPTR($1->in.type))
                                $1->in.fixtag = 0;
#endif /* C1_C */
			if( $2 == DOT )
				{
				/* "structure reference must be addressable" */
				if ($1->in.op != REG) {
					attr_reg = (SCLASS($1->tn.tattrib)
							== ATTR_REG);
					$1 = buildtree( UNARY AND, $1, NIL );
					}
				}
			    if (unionflag && $1->in.op == REG && $2 == DOT && !ISPTR($1->in.type) )
				{
				/* possible iff unionflag */
				$$ = $1;
				idname = lookup( (char *)$3, SMOS );  /* I dunno ... */
				$$->in.type = idname->stype;
				$$->fn.csiz = idname->sizoff;
				if ( ISFTP($$->in.type) )
					uerror("floating pt register union field use");
				if ( !ISPTR($$->fn.type) 
					&& dimtab[$$->fn.csiz] != SZINT )
					uerror("sizeof(register union field) != 4");
				}
			    else /* the normal case */
#endif /* not IRIF */
				{
				register NODE *q;

				/* stash away name string pointer */
				q = block( NAMESTRING, NIL, NIL, INT, 0, INT );
				q->tn.lval = yylval.intval;
				q->tn.rval = $2; /* part of better msg campaign */
#ifndef IRIF
				if ($2 == STREF) (void)fixtag($1);
				$$ = buildtree( STREF, $1, q );
				if (attr_reg) $$->tn.tattrib |= (ATTR_REG<<ATTR_CSHIFT);
				if (ISPTR($$->in.type)) (void)fixtag($$);
#else /* IRIF */
			       $$ = buildtree( $2, $1, q );
			       if (attr_reg) $$->tn.tattrib |= (ATTR_REG<<ATTR_CSHIFT);
#endif /* IRIF*/
				}
# ifdef CXREF
			    refname($3, id_lineno);
# endif
			    }
		|  NAME
			={  idname = $1;
			    /* recognize identifiers in initializations */
			    if( blevel==0 && idname->stype == UNDEF ) {
				register NODE *q;
				/* "undeclared initializer name %s" */
				WERROR( MESSAGE( 111 ), idname->sname );
				q = block( FREE, NIL, NIL, INT, 0, INT );
				q->nn.sym = idname;
				defid( q, EXTERN );
				}
# ifdef ANSI
                            /* generate a warning if a block extern is   */
                            /* being promted to file scope. Done to pass */
                            /* Perennial testing 11/15/93.               */
                            else if(idname->sflags & SBEXTERN_SCOPE)
                               WERROR( MESSAGE( 4 ), idname->sname );
# endif /* ANSI */
			    $$=buildtree(NAME,NIL,NIL);
# ifdef LINT_TRY
			    if ( ($1->sclass==EXTDEF ||
				  $1->sclass==EXTERN ||
				  $1->sclass==USTATIC||
				  $1->sclass==STATIC)
				&& (!($1->sflags & SNAMEREF)) )	
				 outdef($1,LUM,0,lineno);
# endif /* LINT_TRY */
			    $1->sflags |= SNAMEREF; /*indicate name referenced*/
# ifdef CXREF
			    ref($1, id_lineno);
# endif
# ifdef SA
			    $1->sflags |= SDBGREQD; /*need dbg info*/
			    if (saflag)
			      add_xt_info($1, USE); 
# endif
			}
		|  ICON
			={  $$=bcon((int)lastcon,(TWORD)$1);
			    }
		|  FCON
			={  
			    short type = $1;
			    $$=buildtree(FCON,NIL,NIL);
#ifdef ANSI
#ifdef OSF
			    /* This is a hack, without it OSF lint
			       cannot handle floating point constants,
			       I do not understand why it is a problem.
			       sje */

			    $$->qfpn.qval.u[0] = qcon.u[0];
			    $$->qfpn.qval.u[1] = qcon.u[1];
			    $$->qfpn.qval.u[2] = qcon.u[2];
			    $$->qfpn.qval.u[3] = qcon.u[3];
#else
			    $$->qfpn.qval = qcon;
#endif /* OSF */
#else
			    $$->fpn.dval = dcon;
#endif /* ANSI */
			    $$->in.type = type;
			    $$->fn.csiz = type;
			    }
		|  STRING
#ifndef ANSI
			={  $$ = getstr(); /* get string contents */ }
#else /*ANSI*/
			={  $$ = getstr( STRING ); /* get string contents */ }
		|  WIDESTRING
			={  $$ = getstr( WIDESTRING ); /* get string contents */ }
#endif /*ANSI*/
		|   LP  e  RP
			{ 
#ifdef ANSI
			 if (ISFTP($2->in.type))
				$$=buildtree(PAREN,$2,NIL);
			 else
				$$=$2;
#else
			 $$=$2; 
#endif
			}
		;

cast_type:	  typcl_list null_decl
			={
			$$ = tymerge( $1, $2 );
			$$->in.op = NAME;
			$1->in.op = FREE;
			}
		;

null_decl:	   /* empty */
			={ $$ = bdty( NAME, NIL, (struct symtab *)NULL ); }
# ifndef ANSI
		|  LP RP
			={ $$ = bdty( UNARY CALL, bdty(NAME,NIL,(struct symtab *)NULL),(struct symtab *)0); }
		|  LP null_decl RP LP RP
			={  $$ = bdty( UNARY CALL, $2, (struct symtab *)0 ); }
#else  /* ANSI */
		|  LP RP
			 /* This case doesn't use proto-LP proto-RP because
			  * the context save/restore isn't necessary when
			  * there are no parameter declarations, and the
			  * proto forms would cause a reduce/reduce conflict
			  * with the "empty" production.
			  */
			={ $$ = bdty( UNARY CALL, bdty(NAME,NIL,(struct symtab *)NULL),
				(struct symtab *)make_proto_header()); }
		|  proto_LP parameter_type_list proto_RP
			={ $$ = bdty( CALL, bdty(NAME,NIL,(struct symtab *)NULL), (struct symtab *)$2 ); }
		|  LP null_decl RP proto_LP oparameter_type_list proto_RP
			={  $$ = bdty( CALL, $2,  (struct symtab *)$5 ); }
#endif /* ANSI */
		|  MUL null_decl
			={  goto umul; }
		|  MUL tattrib null_decl
			={	$$ = bdty( UNARY MUL, $3, (struct symtab *)0 ); 
			        $$->in.tattrib = $2->in.tattrib;
				$2->in.op = FREE;
			      }
#ifdef LINT
#ifdef xcomp300_800
		   /* accept long pointer syntax;  but doesn't handle
		    * semantics -- just treat like a pointer.
		    */
		|  ER null_decl
			={  goto umul; }
		|  ER tattrib null_decl
			={      $$ = bdty( UNARY MUL, $3, (struct symtab *)0 );
				$$->in.tattrib = $2->in.tattrib;
				$2->in.op = FREE;
				}
#endif
#ifdef APEX
                |  AND null_decl
                        ={   warn_ref_parm();	$$= $2; }
                |  AND tattrib null_decl
                        ={   warn_ref_parm();	$$= $3; }
#endif /* APEX */
#endif /* LINT */
		|  null_decl LB RB
			={  goto uary; }
		|  null_decl LB con_e RB
			={  goto bary;  }
		|  LP null_decl RP
			={ $$ = $2; }
		;

funct_idn:	   NAME  LP 
			={  if( $1->stype == UNDEF ){
				/* function called before any decl/defn */
				register NODE *q;
				q = block( FREE, NIL, NIL, FTN|INT, curdim, INT );
# ifdef ANSI
				/* need to create a proto header since
				 * the function name has never been declared.
				 */
				dstash((int)make_proto_header());  /* ?? or NIL */
# else
				dstash(0);
# endif
				q->nn.sym = $1;
#ifdef ANSI
				defid( q, WEXTERN ); /* 'wild card' EXTERN -
						        matches any storage 
							class */
#else 
				defid( q, EXTERN );
# endif
#ifdef LINT_TRY
				/* function prototype not visible at point 
				 * of call */
				if (ansilint&&hflag)
				    WERROR( MESSAGE(236), $1->sname );
#endif
				}
			    idname = $1;
			    $$=buildtree(NAME,NIL,NIL);
# ifdef CXREF
			    ref($1, id_lineno);
# endif
                            $1->sflags |= SNAMEREF; /* indicate name ref'ed */
# ifdef SA
                            $1->sflags |= SDBGREQD;
			    if (saflag)
			      add_xt_info($1, XT_CALL); 
# endif
			}
		|  term  LP 
		= {
#ifdef C1_C
#ifndef IRIF
/* in case the "name" of the function is a PTR FTN within a struct */
		    fixtag($1);
#endif /* not IRIF */
#endif /* C1_C */
# ifdef SA
                   if (saflag) {
		     unknown_sym.suse = lineno;
		     add_xt_info(&unknown_sym, XT_CALL);
		     }
# endif		     
                   }

		;
%%

# ifdef DEBUGGING
NODE *
mkty( t, d, s ) unsigned t; {
	return( block( TYPE, NIL, NIL, t, d, s ) );
	}
# endif /* DEBUGGING */

NODE *
bdty( op, p, v ) short op; NODE *p; struct symtab *v; {
	NODE *q;

	q = block( op, p, NIL, INT, 0, INT );

	switch( op ){

	case UNARY MUL:
#ifndef ANSI
	case UNARY CALL:
#else 
	/*case UNARY CALL: 	ANSI development only: move done to binary */
#endif 
	case PAREN:
		break;
#ifdef ANSI
	case CALL:
	case UNARY CALL:
		q->in.right = (NODE *) v;  /* really a (struct symtab *) */
		break;
#endif

	case LB:
		q->in.right = bcon((int)v, INT);
		break;

	case NAME:
		q->nn.sym = v;
		break;

	default:
		cerror( "bad bdty" );
		}

	return( q );
	}

#ifdef DEBUGGING
extern flag ddebug;
#endif

dstash( n ){ /* put n into the dimension table */
	if( curdim >= maxdimtabsz-1 ){
		newdimtab();
		}
# ifdef DEBUGGING
	if (ddebug>3) prntf("\tdstash[%d] = %d\n",curdim,n);
# endif
	dimtab[ curdim++ ] = n;
	}

savebc() {
	if( psavbc > & asavbc[maxbcsz-4 ] ){
	        newbctab();
		}
	*psavbc++ = brklab;
	*psavbc++ = contlab;
	*psavbc++ = flostat;
	*psavbc++ = swx;
#ifdef IRIF
	*psavbc++ = swlab;
	*psavbc++ = looplab;
#endif /* IRIF */
	flostat = 0;
	}

resetbc(mask){

#ifdef IRIF
        looplab = *--psavbc;
        swlab = *--psavbc;
#endif /* IRIF */
	swx = *--psavbc;
	flostat = *--psavbc | (flostat&mask);
	contlab = *--psavbc;
	brklab = *--psavbc;
	}

addcase(p) register NODE *p; { /* add case to switch */

#ifndef ANSI
	/* already done???, leave this in ifndef ansi just in case */
	p = optim( p );  /* change enum to ints */
#endif
#ifndef ANSI
	if( p->in.op != ICON || p->tn.type != BTYPE(p->tn.type) ){
		/* "non-constant case expression" */
		UERROR( MESSAGE( 80 ) );
		return;
		}
#else /*ANSI*/
	int val = icons(p, &icons_error);
	if( icons_error ) return;

#endif /*ANSI*/
	if( swp == swtab ){
		/* "case not in switch" */
		UERROR( MESSAGE( 20 ) );
		return;
		}
	if( swp >= &swtab[maxswitsz] ){
		newswtab();
		}
#ifndef ANSI
	swp->sval = p->tn.lval;
#else /*ANSI*/
	swp->sval = val;
#endif /*ANSI*/
	deflab( (unsigned)(swp->slab = GETLAB()) );
	/* Only increment swp pointer when case can be reached.  Otherwise
	 * incorrect sorted code may be generated in swend (since it only
	 * sorts the last 16 bits for short or ushort values */
	switch (swtab[swx].stype){
	    case CHAR:
	    case SCHAR:
		if ((swp->sval < -128)||(swp->sval > 127));
		else ++swp;
		break;
	    case UCHAR:
		if ((swp->sval < 0)||(swp->sval > 255));
		else ++swp;
		break;
	    case SHORT:
		if ((swp->sval < -32768)||(swp->sval > 32767));
		else ++swp;
		break;
	    case USHORT:
		if ((swp->sval < 0)||(swp->sval > 65535));
		else ++swp;
		break;
	    default: ++swp;
	    }
#ifndef ANSI
	tfree(p);
#endif /*ANSI*/
	}

adddef(){ /* add default case to switch */

	if( swp == swtab ){
		/* "default not inside switch" */
		UERROR( MESSAGE( 29 ) );
		return;
		}
	if( swtab[swx].slab >= 0 ){
		/* "duplicate default in switch" */
		UERROR( MESSAGE( 34 ) );
		return;
		}
	deflab( (unsigned)(swtab[swx].slab = GETLAB()) );
	}

swstart(t)	TWORD t;{
	/* begin a switch block */
	if( swp >= &swtab[maxswitsz] ){
		newswtab();
		}
	swx = swp - swtab;	/* swx is offset of switch base record. */
	swp->slab = -1;
	swp->stype = t;		/* only the top sw element has stype set.*/
	++swp;
	}

#ifndef IRIF
 swdefend()
 /* if no DEFAULT case has been seen, add one. Necessary for new assembler. */
 {
	reached = 1;
	adddef();
	flostat |= FDEF;
	branch( (unsigned)brklab );
	flostat |= FBRK;
	reached = 0;
 }
#endif /* not IRIF */

 /* Switchswap() is a simple structure swap routine. Note that it assumes
 the support of structure valued assignment.
 */
 struct sw *switchswap(a)	register struct sw *a;
 {
	register struct sw *b = a + 1;
	struct sw temp;

	temp = *b;
	*b = *a;
	*a = temp;
	return (b);
 }

swend(){ /* end a switch block */

	register struct sw *swbeg, *q, *r, *r1;
	/* CONSZ temp; */
	TWORD type = swtab[swx].stype;
	flag duplicate_found = 0;

	swbeg = &swtab[swx+1];

	/* sort */

	r1 = swbeg;
	r = swp-1;

	if ( !ISUNSIGNED(type) )
		while( swbeg < r )
			{
			/* bubble largest to end */
			for( q=swbeg; q<r; ++q )
				{
				if ((type == INT)||(type == LONG))
					{
					if( q->sval > (q+1)->sval )
						{
						/* swap */
						r1 = switchswap(q);
						}
					}
				else /* type == SHORT */
					{
					if ( (short)(q->sval) > (short)((q+1)->sval) )
						{
						/* swap */
						r1 = switchswap(q);
						}
					}
				}
			r = r1;
			r1 = swbeg;
			}
	else
		while( swbeg < r )
			{
			/* bubble largest to end */
			for( q=swbeg; q<r; ++q )
				{
				if ((type == UNSIGNED)||(type == ULONG))
					{
					if( (unsigned)(q->sval) > (unsigned)((q+1)->sval) )
						{
						/* swap */
						r1 = switchswap(q);
						}
					}
				else /* type == UNSIGNED SHORT */
					{
					if( (unsigned short)(q->sval) > (unsigned short)((q+1)->sval) )
						{
						/* swap */
						r1 = switchswap(q);
						}
					}
				}
			r = r1;
			r1 = swbeg;
			}

	/* it is now sorted */

	for( q = swbeg+1; q<swp; ++q ){
	     if( q->sval == (q-1)->sval ){
		  duplicate_found++;
		  /* "duplicate case in switch, %d" */
		  UERROR( MESSAGE( 33 ), q->sval );
	     }
	}
	if( duplicate_found ) return;

	q = swbeg - 1;
	genswitch( q, swp-swbeg, q->stype );
	swp = q;
	}

#ifdef IRIF
static void genswitch( p, n, type ) struct sw *p; {
	/*	p points to an array of structures, each consisting
		of a constant value and a label.
		p[0] contains default case.
		The entries p[1] to p[n] are the nontrivial cases
		*/
     ir_switch_info( n, type, ( (flostat&FDEF) ? p->slab : brklab ), 
		     p[1].sval, p[n].sval );
     for( ; n>0; n-- ) {
	  ir_switch_cases( (++p)->sval, p->slab );
     }
}
#endif /* IRIF */

/* when a STRING is used in the context of an external initializer, getstr()
 * returns a NIL pointer rather than a treenode.  Productions that build
 * on <term> will have a null pointer for the <term>.  In most productions 
 * involving <term> this is caught by tests that have been added to 
 * buildtree().  But several cases either don't call buildtree or try
 * to dereference the pointer first.  These have had checks added using the
 * macro : CHECKTERMPTR. This routine is called to print an error message
 * when a null pointer is found.
 * The cases currently known are:
 *	AND term
 *	SIZEOF term
 *	term STROP NAMESTRING
 */
niltermptr() {
	cerror("panic in niltermptr: ill-formed expression (null ptr)");
}

/* Change the type of a TYPE node from a signed to the corresponding 
 *  unsigned type.  This is used to make bit fields explicitly unsigned.
 *  68020 code uses unsigned bit field op codes, so make the node type
 *  explicitly unsigned to prevent the LCD version from doing ext.l
 *  when it extracts a field.
 */
mkunsigned(typ)
register NODE *typ;
{
	if (typ->in.type <= LONG && typ->in.type >= CHAR)
		typ->in.type = ENUNSIGN(typ->in.type);
}

/* Stop the propagation of the C1 arrayref up the tree by making it
 *  negative, because a term has just been reduced to an expression.
 */

NODE *fixtag(p)
NODE *p;
{
#ifdef C1_C
#ifndef IRIF
	if ((p) && (p->in.c1tag)) p->in.fixtag = 1;
#endif /* not IRIF */
#endif  /* C1_C */
return(p);
}

#ifndef IRIF

/* Selectively restore the pointer to stack offset for auto variables upon
 *  exit from a block.  In the ONEPASS version, any stack space used within
 *  a nested block is reclaimed.  In the 2-pass version, we want distinct
 *  addresses for variables in parallel nested blocks, so we discard the
 *  saved autooff.  autooff will be initialized to AUTOINIT in ftnend.
 */
restore_autooff()
{
#ifndef C1_C
    autooff = *--psavbc;
#else  /* C1_C */
    if (optlevel >= 2)
    	--psavbc;
    else
	autooff = *--psavbc;
#endif /* C1_C */
}


extern int stroffset; 	/* saved %a1 location - defined in code.c */

#ifdef C1_C

extern prtdcon();

/* For the return statement in a structure-valued function, the grammar builds
 *  a stref tree to force the correct pointer into d0, but then discards the
 *  tree (relying on efcode() to emit the proper copy code).  This copy code
 *  hides some branch targets from C1, so we emit the STASG tree explicitly
 *  in the 2-pass case.
 */
putreturn(p)
NODE *p;
{

register NODE *oreg;

        if (p->in.left->in.type == STRTY ||
            p->in.left->in.type == UNIONTY) {
		/* Convert the NAME node into U*-OREG */
		p->in.left->in.op = UNARY MUL;
		p->in.left->in.left = oreg =
			block(OREG, NIL, NIL, p->in.type, 0, INT);
		oreg->tn.lval = stroffset;
		oreg->tn.rval = 14;  /* %a6 */
		p = clocal(p);
		p = optim(p);
		walkf(p, prtdcon);	/* convert FCONs into NAMEs */
		(void)locctr(PROG);
		puttree(p, 0/*longassign*/, 1/*stasg*/);
		/* Now get original result address into ret val: %d0=OREG */
		tfree(p->in.right);
	        p->in.op = FREE;
	        p->in.left->in.op = FREE;
#ifdef NEWC0SUPPORT
		if (c0flag) c0forceflag = 1;
#endif /* NEWC0SUPPORT */
	        ecomp( buildtree( FORCE, p->in.left->in.left, NIL ) );
#ifdef NEWC0SUPPORT
		c0forceflag = 0;
#endif /* NEWC0SUPPORT */
		/* Don't free the tree because the FORCE in the grammar 
		   needs a portion of it */
		}
#ifdef ANSI
	else if (p->in.left->in.type == LONGDOUBLE) {
		/* Convert the NAME node into U*-OREG */
		p->in.op = ASSIGN;
		p->in.left->in.op = UNARY MUL;
		p->in.left->in.left = oreg =
			block(OREG, NIL, NIL, INCREF(p->in.type), 0, INT);
		oreg->tn.lval = stroffset;
		oreg->tn.rval = 14;  /* %a6 */
		/* p = clocal(p); */
		/* p = optim(p); */
		/* walkf(p, prtdcon);*/
		/* puttree(p); */
		/* Now get original result address into ret val: %d0=OREG */
		/* tfree(p->in.right); */
	        /* p->in.op = FREE; */
	        /* p->in.left->in.op = FREE; */
		ecomp(p);
		oreg = block(OREG, NIL, NIL, INCREF(LONGDOUBLE), 0, INT);
		oreg->tn.lval = stroffset;
		oreg->tn.rval = 14;  /* %a6 */
#ifdef NEWC0SUPPORT
		if (c0flag) c0forceflag = 1;
#endif /* NEWC0SUPPORT */
	        ecomp( buildtree( FORCE, oreg, NIL ) );
#ifdef NEWC0SUPPORT
		c0forceflag = 0;
#endif /* NEWC0SUPPORT */
		/* Don't free the tree because the FORCE in the grammar 
		   needs a portion of it */
		}
#endif /* ANSI */
	else
		{
		tfree(p->in.left);
		p->in.op = FREE;
#ifdef NEWC0SUPPORT
		if (c0flag) c0forceflag = 1;
#endif /* NEWC0SUPPORT */
		ecomp( buildtree( FORCE, p->in.right, NIL ) );
#ifdef NEWC0SUPPORT
		c0forceflag = 0;
#endif /* NEWC0SUPPORT */
		}

}

put_addr_return()
{
	register NODE *oreg;

	oreg = block(OREG, NIL, NIL, INCREF(LONGDOUBLE), 0, INT);
	oreg->tn.lval = stroffset;
	oreg->tn.rval = 14;  /* %a6 */
#ifdef NEWC0SUPPORT
	if (c0flag) c0forceflag = 1;
#endif /* NEWC0SUPPORT */
	ecomp(buildtree(FORCE,oreg,NIL));
#ifdef NEWC0SUPPORT
	c0forceflag = 0;
#endif /* NEWC0SUPPORT */
}

#endif /* C1_C */

#endif /* IRIF not defined */

# ifdef ANSI

/************************************************************************
 * Routines for managing prototype scopes.
 *
 * 	The routines to "open" or "close" a prototype are split into
 *	pairs.  For starting a scope: 
 *	  (a) save_parse_context() is called when the LP is seen;  
 *	      this is necessary to make sure the context (instruct,
 *	      curclass, stwart) is saved and reset before yylex
 *	      does a lookahead to the leading token of the parameter list.
 *	      A STRUCT keyword in the parameter type, for example, must
 *	      reset stwart *after* the save_parse_context. 
 *	  (b) open_proto_scope() which sets of the next symbol table level
 *	      is not called until the start of an actual parameter list. 
 *	      This decreases the number of errors which are non-recoverable
 *	      because the nesting of open/close proto scope gets corrupted.
 *
 * 	Note that all the auxilary arrays used here have to be
 *	indexed with 'blevel' not 'inproto'.  This is because inproto
 *	will get temporarily set back to zero if we process a nested
 *	structure declaration within the prototype scope. See routine
 *	'bstruct' in file pftn.c.  If an error causes us to miss a
 *	call to 'close_proto_scope', the blevel will be wrong and
 *	'restore_parse_context' will grab the wrong values.  However,
 *	this seems a don't care, for now, because the mixed up proto
 *	scoping is going to end up in a fatal error because the
 *	symbol table levels are out of synch.
 *
 */

/************************************************************************
 * save_parse_context : called when an LP is seen in a prototype
 *	context.  Save global variables that describe the current
 *	parsing context so they can be restored after the prototype
 *	and set them to "clean" values for the start of the prototype.
 *	This is important when structures and prototype lists are
 *	nested.
 */
save_parse_context() {
  /* Save the current values of the globals curclass and instruct.
   * Then reset them and stwart to "clean" values to start a parameter list.
   */
  curclass_save[blevel] = curclass;
  instruct_save[blevel] = instruct;
  curclass = SNULL;
  instruct = 0;
  stwart = 0;
}

/************************************************************************
 * open_proto_scope : called at the start of a function prototype
 *	parameter list to do initializations.
 *
 */

open_proto_scope() {

  blevel++;
  if (blevel >= NSYMLEV)
	cerror("too many lexical scopes");
  inproto++;
  if (slevel_head(blevel) != NULL_SYMLINK)
	cerror("panic at start of parameter/name list");
  proto_head[blevel] = make_proto_header();
}

/***********************************************************************
 * restore_parse_context : restore the parse context that was saved
 *	at the start of the prototype (in case we were inside a
 *	structure, for example).
 */
restore_parse_context() {
  curclass = (char)curclass_save[blevel];
  stwart = instruct = instruct_save[blevel];
}

/************************************************************************
 * close_proto_scope : called at the end of a function prototype
 *	parameter list to do cleanup, and restore the curclass, and
 *	do some initial semnatic checking of the parameter list.
 */
close_proto_scope() {
  NODE * ph;
  ph = proto_head[blevel];
  ph->ph.phead = slevel_head(blevel);
  unscope_parameter_list(ph, blevel);
  check_parameter_list(ph);
  blevel--;
  inproto--;
}

/**********************************************************************/

# endif /* ANSI */

/* These routines should be accesible to ccom, cpass1, lint, cxref */
/* newdimtab - grow the size of the dimension table */
newdimtab() {
        maxdimtabsz += DIMTABSZ;
        if ((dimtab = (int *)realloc((void *)dimtab,maxdimtabsz*sizeof(int))) 
		== NULL)
                cerror("out of memory in newdimtab");
        }


/* newparamstk - grow the size of the parameter stack */
newparamstk() {
	maxparamsz += PARAMSZ;
	if ((paramstk = (int *)realloc((void *)paramstk,maxparamsz*sizeof(int)))
		 == NULL) 
		cerror("out of memory in newparamstk");
	}

/* newswtab - grow the size of the switch table */
newswtab() {
	int offset = swp-swtab;
	maxswitsz += SWITSZ;
	if ((swtab = (struct sw *)realloc((void *)swtab,maxswitsz*sizeof(struct sw)))
	    	== NULL)
		cerror("out of memory in newswtab");
	swp = swtab + offset;
	}

/* newbctab - grow the size of the break/continue table */
newbctab() {
	int offset = psavbc-asavbc;
	maxbcsz += BCSZ;
	if ((asavbc = (int *) realloc((void *)asavbc,maxbcsz * sizeof(int))) 
		== NULL)
		cerror("out of memory in newbctab");
	psavbc = asavbc + offset;
	}
