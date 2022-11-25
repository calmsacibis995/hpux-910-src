/* @(#) $Revision: 70.1 $    */

/************************  asgram.y ****************************************/
/* Yacc parser definition for 68020/68881/68010 assembler.
 * Plus supporting routines.
 *
 * Since yacc does not provide a "conditional" compiliation facility for
 * productions, the 68010 version is handled by conditional compilation
 * in the action code of productions that do not apply to a 68010.
 */
/* NOTES:
 *	850724  -- try using AND in place of LITOP for compatibility with
 *		   Motorola.  May need to add some precendence forcing onto
 *		   some of the productions??.
 */

%{
# include "stdio.h"

# include "symbols.h"
# include "adrmode.h"
# include "bitmask.h"
# include "align.h"
# include "icode.h"
# include "header.h"

/*  the 800 does not have cvtnum.h in /usr/include,  so we need to include
    it from our own directory   */

# include <cvtnum.h>

# include "ivalues.h"
# include "sdopt.h"

# define	TXTCHARS	200

# define defsym(x)	(((x).exptype==TSYM)&&(((x).symptr->stype&STYPE)!=SUNDEF))

node zero_node;
extern node	nodes[];
extern int	maxnode;
extern int	inode;
extern char	yytext[];

extern unsigned int	line;	/* current input file line number */

extern short	sdopt_flag;	/* sdi optimization flag */
extern short	p1subtract_flag;/* during span-dependent, do we calculate
				 * a subtraction in pass1.
				 */
extern short	nerrors_this_stmt;

extern FILE *	fdin;	/* input source */

extern FILE *fdmod;	/* module info */
extern int   modsiz;

extern char	*file;	/* assembly input file name */

extern symbol *	dot;	/* pc pointer symbol */

# ifdef LISTER1
extern short	listflag;
extern	dump_echobuf();
extern long	listdot;
# endif
# ifdef LISTER2
extern short	listflag;
extern long	listdot;
# endif

extern long	newdot;    /* up to date value of pc pointer symbol */
extern long	dotbss;    /* bss pointer symbol */
long		labdot;	   /* save value of dot for label definition */

long	data_type;

extern int fp_size, fp_cpid, fp_round;
extern symbol  * usersym;

rexpr	* operate(); /* procedure to perform expression operations */

extern upsymins	*lookup(); /* symbol table access routine */

static BYTE defseen = 0; /* flag set if .def expression found on this line */

operand_addr1  * operand_address[MAXOPERAND];
int noperand = 0;
symbol * labellist = (symbol *) NULL;
symbol * datalabel = (symbol *) NULL;
symbol * bsslabel  = (symbol *) NULL;
symbol * lastlabel = (symbol *) NULL;
unsigned long lastval;
unsigned long laststype;
symbol * idlist;
symbol * xref_sym = (symbol *) NULL;	/* For xt symbols from "xtpointer" */

int    stattype_is_id;			/* TRUE when stattype is label.    */

int    st_is_id1;			/* The stattype_is_id values for   */
int    st_is_id2;			/* the three stattype fields in    */
int    st_is_id3;			/* the DNTT_BLOCK entry.           */


extern long dotdat;

extern struct instruction instruction;
extern int fpsize;
int data_align = ALBYTE;

extern dragon_nowait;

#ifdef PIC
extern short shlib_flag;
#endif

%}

/* note: a future tuning issue would be to get this down to
 * using pointers to structures whose space is allocated elsewhere.
 */
%union	{
		expvalue	*unfloat;
		long	unlong;
		rexpr	*unrexpr;
		symbol	*unsymptr;
		instr	*uninsptr;
		addrmode1 *unaddr;
		reg	unreg;
		xreg	*unxreg;
		char	*unstrptr;
		operand_addr1  *unopaddr;
		register_pair *unregp;
	}

%token	<unlong>	ENDTOKEN

%token	<uninsptr>	OPCODE
%token	<uninsptr>	MOVEMOP
%token	<uninsptr>	CINVCPUSHOP CINVCPUSHOP_JC
%token	<uninsptr>	BITFONEOP
%token	<uninsptr>	BITFTWOOP1 BITFTWOOP2
%token	<uninsptr>	FPOP0 FPOP1 FPOP2 FPOP3 FPOP4 FPOP5 FPMOVEMOP
%token	<uninsptr>	PMUOP0 PMUOP1 PMUOP2 PMUOP3 PMUOP4 PMUOP5

%token	<uninsptr>	ISPACE IBYTE ISHORT ILONG
%token	<uninsptr>	IDATA ITEXT IBSS
%token	<uninsptr>	ISET
%token	<uninsptr>	ICOMM  ILCOMM
%token	<uninsptr>	IALIGN ILALIGN IEVEN
%token	<uninsptr>	IASCIZ
%token	<uninsptr>	IEQU
%token	<uninsptr>	IGLOBAL
%token	<uninsptr>	ISGLOBAL
%token	<uninsptr>	IORG
%token	<uninsptr>	IVERSION
%token	<uninsptr>	ISHLIBVERSION
%token	<uninsptr>	IINTERNAL

%token	<uninsptr>	IMODULE

%token	<uninsptr>	IP1SUBON IP1SUBOFF

/* 68881 Pseudo Ops */
%token	<uninsptr>	IFLOAT IDOUBLE
%token	<uninsptr>	IPACKED	IEXTEND
%token	<uninsptr>	IFPID
%token	<uninsptr>	IFPMODE
%token	<uninsptr>	IFSET

/* DRAGON */
%token	<uninsptr>	DRGOP

/* Symbolic Debugger Support Pseudo Operations */

%token	<uninsptr>	IGNTT		ILNTT		

%token	<uninsptr>	IDNTSRCFILE	IDNTMODULE	IDNTFUNCTION
%token	<uninsptr>	IDNTENTRY	IDNTBEGIN	IDNTEND
%token	<uninsptr>	IDNTIMPORT	IDNTLABEL	IDNTWITH
%token  <uninsptr>      IDNTCOMMON      IDNTFPARAM	IDNTBLOCKDATA
%token	<uninsptr>	IDNTSVAR	IDNTDVAR	IDNTCONST
%token	<uninsptr>	IDNTTYPEDEF	IDNTTAGDEF	IDNTPOINTER
%token	<uninsptr>	IDNTENUM	IDNTMEMENUM	IDNTSET
%token	<uninsptr>	IDNTSUBRANGE	IDNTARRAY	IDNTSTRUCT
%token	<uninsptr>	IDNTUNION	IDNTFIELD	IDNTVARIANT
%token	<uninsptr>	IDNTFILE	IDNTFUNCTYPE    IDNTCOBSTRUCT
%token	<uninsptr>	IDNTXREF        IDNTSA		IDNTMACRO
%token	<uninstpr>	IDNTGENFIELD	IDNTMEMACCESS	IDNTMODIFIER
%token	<uninstpr>	IDNTVFUNC	IDNTCLASSSCOPE	IDNTFRIENDCLASS
%token	<uninstpr>	IDNTFRIENDFUNC	IDNTCLASS	IDNTPTRMEM
%token	<uninstpr>	IDNTINHERITANCE	IDNTOBJECTID	IDNTBLOCK
%token	<uninstpr>	IDNTMEMFUNC	IDNTREFERENCE	IDNTINCLUDE
%token	<uninstpr>	IDNTFIXUP	IDNTENDINCLUDE

%token	<uninsptr>	ISLTNORMAL
%token	<uninsptr>	ISLTSPECIAL
%token	<uninsptr>	ISLTEXIT
%token	<uninsptr>	ISLTASST

%token	<uninsptr>	IVT IVTBYTES IVTFILE 

%token	<uninsptr>	IXT

%token	<uninsptr>	IXTINFO         IXTLINK          IXTNAME
%token	<uninsptr>	IXTINFO1        IXTINFO2A        IXTINFO2B
%token	<uninsptr>	IXTBLOCK



/* Lexical Tokens recognized by yylex */

%token	<unlong>	SP NL SH REGC AND SQ LP RP MUL PLUS COMMA 
%token	<unlong>	MINUS ALPH DIV DIG COLON SEMI LSH RSH LB RB XOR
%token	<unlong>	IOR TILDE DQ DOT MOD LCB RCB BANG
%token	<unreg>		NOREG AREG DREG PC CCREG SRREG CREG CACHEREG
%token	<unreg>		ZAREG ZDREG ZPC
%token	<unreg>		FPDREG FPCREG
%token	<unreg>		PMUREG
%token	<unreg>		DRGDREG DRGCREG
%token	<unlong>	SIZE_SUFFIX FP_SIZE_SUFFIX
%token	<unlong>	NOWAIT

/* define operator precedences so that yacc can make a clean parser */

%nonassoc	COLON
%left		IOR
%left		XOR
%left		AND
%left		LSH RSH
%left		PLUS MINUS
%left		MUL DIV MOD
%right		UNOP

%token	<unlong>	INTNUM
%token	<unfloat>	FLTNUM
%token	<unsymptr>	IDENT
%token	<unlong>	ERROR
%token	<unlong>	STRING

%type	<unrexpr>	iexpr expr
%type	<unrexpr>	iexpr_data fexpr_data

%type	<unaddr>	soperand
%type	<unaddr>	roperand
%type	<unaddr>	cregaddrmode
%type	<unopaddr>	dregoperand
%type	<unopaddr>	operand
%type	<unopaddr>	cache_list
%type	<unopaddr>	fpoperand
%type	<unopaddr>	movem_operand
%type	<unopaddr>	fpmovem_operand
%type	<unopaddr>	bitfield_operand
%type	<unopaddr>	simple_operand
%type	<unopaddr>	pmuoperand
%type	<unopaddr>	dragonoperand

%type	<unlong>	index_size index_scale opt_index_size opt_index_scale
%type	<unreg>		dreg areg pc pc_areg  dareg
%type	<unxreg>	xreg xreg2
%type	<unaddr>	mem_indirect  mem_indirect_index opt_xreg_disp
%type	<unrexpr>	opt_disp
%type	<unlong>	reglist reglist_component
%type	<unlong>	fpdreglist fpdreglist_component fpcreglist
%type	<unopaddr>	fppacked_specifier
%type	<unregp>	reg_pair
%type	<unregp>	fpdreg_pair
%type	<unopaddr>	bitfield_specifier
%type	<unaddr>	bit_offset bit_width
%type	<unaddr>	bit_offset_or_width
%type	<unlong>	opt_instr_size opt_fpinstr_size  fp_size_suffix
%type	<uninsptr>	fpop
%type	<uninsptr>	pmuop

%type	<unlong>	dnttpointer vtindex xtpointer
%type	<unlong>	stattype stattype1 stattype2 stattype3
%type	<unlong>	sltindex  dyntype boolean
%type	<unlong>	integer posinteger
%type	<unlong>	address
%type   <unlong>        xt_data vt_byte

/*%type	<unlong>	regtype */

%%
start:		file ENDTOKEN
			{ do_labels(1); /* in case there are labels but no
					 * instruction on last line. */
			  YYACCEPT;
			}
	;

file:		/* empty */ 
					{ new_stmt_setup(); }
					
	| 	file labinst NL 	{ 
#ifdef LISTER2
					  if (listflag) make_list_marker();
#endif
					  line++;
					  new_stmt_setup();
# ifdef LISTER1
					  if (listflag) dump_echobuf();
# endif
					}
	| 	file labinst SEMI 	{ 
#ifdef LISTER2
					  if (listflag) make_list_marker();
#endif
					  new_stmt_setup();
# ifdef LISTER1
					  if (listflag) dump_echobuf();
# endif
					}
/*	|	file error NL 		{ line++;
/*					  yyerrok;
/*					  new_stmt_setup();
/*					}
*/
		;

labinst:	labels instruction
			{ noperand = 0; }
		;

labels:		/* no label is okay */
			{ /* null */ }
	|	labels IDENT COLON
			{ if ($2->stag != USRNAME )
				uerror("illegal label symbol (%s)", $2->snamep);
			  if (($2->stype&STYPE) != SUNDEF)
				uerror("multiply defined label (%s)", $2->snamep);
			  /*$2->svalue = newdot;*/
			  /* ?set the type now (to catch multiple defines), but
			   * don't set the value until after instruction 
			   * parsed (in case of implicit alignment).
			   */
			  /*$2->stype |= dot->stype;*/
			  save_ident(&labellist,$2);
# ifdef SDOPT
			  if (sdopt_flag & ((dot->stype&STYPE)==STEXT)) /* sdi */
				$2->ssdiinfo = makesdi_labelnode($2);
# endif
					}
		;

instruction:	/* no instruction is okay */
		{ /* null */ }
	|	instruction1
		{ do_labels(0); }
	;

instruction1:

/* pseudo operations - assembler directives */

/* space */	ISPACE iexpr
			{ if (($2->exptype!=TYPL) ||
				($2->expval.lngval < 0))
			  uerror("illegal space size expression");
			  if (dot->stype != SBSS)
				fill($2->expval.lngval);
			  else
				newdot += $2->expval.lngval;
			}

/* byte */	| IBYTE setbyte iexpr_strg_datalist

/* short */	| ISHORT setshort iexpr_datalist

/* long */	| ILONG setlong iexpr_datalist

/* float */	| IFLOAT setfloat fexpr_datalist

/* double */	| IDOUBLE setdouble fexpr_datalist

/* extend */	| IEXTEND setextend fexpr_datalist

/* packed */	| IPACKED setpacked fexpr_datalist

/* asciz */	| IASCIZ setbyte  STRING
			{ /*ckalign(data_align,0);*/
			  generate_icode(I_STRING, yytext, $3+1);
			  /*newdot += nbytes;  do this only if doing some
						kind of bytes expected vs.
						bytes generated checks.*/
			}

/* text */	| ITEXT
			{ if ((labellist != NULL) && (dot->stype != STEXT))
				uerror("statement should not be labeled");
			  cgsect(STEXT);
			}

/* data */	| IDATA
			{ if ((labellist != NULL) && (dot->stype != SDATA))
				uerror("statement should not be labeled");
			cgsect(SDATA);
			}
/* bss */	| IBSS
			{ if ((labellist != NULL) && (dot->stype != SBSS))
				uerror("statement should not be labeled");
			  cgsect(SBSS);
			}


/* lcomm */	| ILCOMM IDENT COMMA iexpr COMMA iexpr  
			{ long mod;
			  long lalignval;
			  int errflag = 0;
			  if (($4->exptype != TYPL)||($4->expval.lngval < 0)){
				uerror("illegal bss size");
				errflag++;
				}
			  if (($6->exptype != TYPL)||
			      ((lalignval=$6->expval.lngval) <= 0)){
				uerror("illegal bss alignment");
				errflag++;
				}
			  if ((lalignval != 1) && (lalignval != 2) &&
			      (lalignval != 4) && (lalignval != 8) &&
			      (lalignval != 16)) {
				werror("nonstandard lalignment: may not be preserved by linker");
				}
			  if (($2->stype&STYPE) != SUNDEF) {
				uerror("multiply defined bss label (%s)", $2->snamep);
				errflag++;
				}
			  if (!errflag) {
				if (dot->stype==SBSS)
				   dotbss = newdot;
			  	if (mod = dotbss % $6->expval.lngval)
				   dotbss += $6->expval.lngval - mod;
			  	$2->svalue = dotbss;
			  	dotbss += $4->expval.lngval;
			  	$2->stype |= SBSS;
				if (dot->stype==SBSS)
				   newdot = dotbss;
# ifdef LISTER2
				if (listflag && dot->stype != SBSS)
				   make_list_marker_bss($2->svalue, dotbss);

# endif
				}
			}
		

/* global */	| IGLOBAL identifier_list
			{ do_global_ident(idlist, SEXTERN); }

/* sglobal */	| ISGLOBAL identifier_list
			{ do_global_ident(idlist, SEXTERN|SEXTERN2); }

/* internal */	| IINTERNAL identifier_list
			{ do_internal_ident(idlist); }

/* set */	| ISET IDENT COMMA iexpr
			{ do_set_pseudo($2, $4); }

/******* fset not implemented ("fset" not in mnemonic symbol table *********/
/* fset */	| IFSET  FP_SIZE_SUFFIX { fp_size = $2;} COMMA IDENT COMMA expr
			{ /*do_fset_pseudo($5, $7);*/ }

/* equ */	| IEQU iexpr
			{ if (labellist != NULL) {
				do_set_pseudo(labellist, $2);
				labellist = labellist->slink;
				if (labellist != NULL)
				  uerror("too many labels on equ");
				}
			  else
				uerror("no label for equ");
			}

/* allow_p1sub */
		| IP1SUBON
			{ p1subtract_flag = 1; }

/* end_p1sub */
		| IP1SUBOFF
			{ p1subtract_flag = 0; }

/* version */	| IVERSION iexpr
			/* support for version pseudo-op.
			 * The version number must be pass1 absolute.
			 * A -V<num> command line option overrides any
			 * version pseudo-op in the source.
			 */
			{ short new_version;
			  new_version = abs_value($2, 1, 0, 0xffff);
			  if (version_seen && (new_version!=version))
				werror("multiple version pseudo-op; previous value overwritten");
			  else if (vcmd_seen && (new_version!=version))
				werror("version pseudo-op ignored when -V option used");
			  else version = new_version;
			  version_seen = 1;
			}

/*sh version */	| ISHLIBVERSION iexpr COMMA iexpr
			/* support for the shlib_ version pseudo-op.
			 * The version number must be pass1 absolute.
			 */
			{ 
#ifdef PIC
			   ShlibVersion(
			     abs_value($2, 0, 0, 0xffff),
			     abs_value($4, 0, 0, 0xffff)
			   );
#else
			   aerror("'shlib_version' pseudo-op found in non PIC assembler");
#endif			   
			} 

/* module info*/  | IMODULE STRING
			{ 
			  module_info(yytext);
			}

/*  ** NOTE this action code for "comm" allow multiple commons with the
 *     same neme.  That seems to be consistent with the old assembler.
 */
/* comm */	| ICOMM IDENT COMMA iexpr
			{ if (($4->exptype!=TYPL)||($4->expval.lngval<0))
				uerror("illegal comm size expression");
			  else if (($2->stype&STYPE)!=SUNDEF)
				uerror("illegal attempt to redefine symbol (%s)",
				  $2->snamep);
			  else {
				$2->stype = (SEXTERN | SUNDEF);
				if ($2->svalue < $4->expval.lngval)
				    $2->svalue = $4->expval.lngval;
				};
			}

/* lalign */	| ILALIGN iexpr
			{ register int lalmod = $2->expval.lngval;
			  if (($2->exptype!=TYPL) || (lalmod < 1) ||
			     (lalmod > 8192)) {
				uerror("illegal lalign expression");
				goto lalign_exit;
				}
			  if ((lalmod != 1) && (lalmod != 2) &&
			      (lalmod != 4) && (lalmod != 8) &&
			      (lalmod != 16))
				werror("nonstandard lalignment: may not be preserved by linker");
# ifdef SDOPT
			  if (sdopt_flag && (dot->stype==STEXT)) {
				if (lalmod & 01) {
				   werror("noneven alignment not preserved during span-dependent optimization");
				   ckalign(lalmod, 0);
				   }
				else {
				   ckalign(2, 0);
				   if (lalmod > 2) {
				      generate_icode(I_LALIGN, lalmod);
				      fill(lalmod-2);
				      }
				   }
				}
			  else
# endif
				ckalign(lalmod ,0);
			lalign_exit: ;
			}

/* even */	| IEVEN
			{ ckalign(2,0); }

/* align */	| IALIGN  IDENT  COMMA  iexpr
			{ if ($2->stag != USRNAME)
				uerror("illegal alignment symbol (%s)",
				  $2->snamep);
			  if (($2->stype&STYPE) != SUNDEF)
				uerror("multiply defined label (%s)",
				  $2->snamep);
			  if (($4->exptype!=TYPL) || ($4->expval.lngval<=0) ||
			      ($4->expval.lngval>0x7fff))
				uerror("illegal alignment expression");
			  if (dot->stype == STEXT)
				werror("align in text segment; avoid branches across it");
 			  $2->stype |= dot->stype|SALIGN|SEXTERN;
			  $2->svalue = dot->svalue;  /* ?? or newdot ?? */
			  $2->salmod = $4->expval.lngval;
			  /* when the alignment occurs in text or data, 
			   * will need a relocation record, and I_ALIGN
			   * will force this in pass2.  For bss, there
			   * will only be the LST symbol.
			   */
			  if ((dot->stype&STYPE)!=SBSS)
				generate_icode(I_ALIGN, &$2, 4);
			}


/****** org not currently implemented ("org" not in mnemonic symbol table) ****/
/* org */	| IORG iexpr
			{ int incr;
			       if (($2->exptype!=TYPL)&&(!defsym(*$2)))
					uerror("illegal org expression");
			       else if (defsym(*$2)) {	
					if ($2->symptr->stype!=dot->stype)
						uerror("incompatible org expression type");
				        incr = $2->expval.lngval + $2->symptr->svalue -
						dot->svalue;
						}
			       else { /* absolute origins */
				        werror("usage of absolute origins");
					incr = $2->expval.lngval - dot->svalue;
				  	};
			       if (incr < 0)
					uerror("cannot decrement value of '.'");
			       else
					fill(incr);

			     }

/* fpmode */	| IFPMODE  iexpr 
			{ register int rnd;
			  if ($2->exptype != TYPL)
			     uerror("absolute expression required for fpmode");
			  else {
				rnd = $2->expval.lngval;
				if (rnd != C_NEAR && rnd != C_POS_INF &&
				    rnd != C_NEG_INF && rnd != C_TOZERO)
				    uerror("illegal value for fpmode");
				else
				    fp_round = rnd;
				}
			}

/* fpid */	| IFPID  iexpr 
			{ if ($2->exptype != TYPL)
			     uerror("absolute expression required for fpid");
			  else
			     fp_cpid = $2->expval.lngval;
			}

/* Symbolic Debugger support pseudo operations */

/* gntt */	| IGNTT
			{ if (labellist != NULL)
				uerror("'gntt' statement should not be labeled");
			  cgsect(SGNTT);
			}

/* lntt */	| ILNTT
			{ if (labellist != NULL)
				uerror("'lntt' statement should not be labeled");
			  cgsect(SLNTT);
			}

	|	IDNTSRCFILE  posinteger COMMA vtindex COMMA sltindex
			{ dntt_srcfile($2,$4,$6); }
			  

	|	IDNTMODULE vtindex COMMA vtindex COMMA sltindex
			{ dntt_module($2,$4,$6); }

	|	IDNTFUNCTION boolean COMMA posinteger COMMA posinteger COMMA
                        posinteger COMMA boolean COMMA posinteger COMMA
                        boolean COMMA vtindex COMMA vtindex COMMA
			dnttpointer COMMA sltindex COMMA address COMMA
			dnttpointer COMMA address COMMA address
			{ dntt_function($2,$4,$6,$8,$10,$12,$14,$16,$18,$20,$22,$24,$26,$28,$30); }

	|	IDNTENTRY boolean COMMA posinteger COMMA posinteger COMMA
                        posinteger COMMA boolean COMMA posinteger COMMA
                        boolean COMMA vtindex COMMA vtindex COMMA
			dnttpointer COMMA sltindex COMMA address COMMA
			dnttpointer 
			{  dntt_entry($2,$4,$6,$8,$10,$12,$14,$16,$18,$20,$22,$24,$26); }

	|	IDNTBLOCKDATA boolean COMMA posinteger COMMA posinteger COMMA
                        posinteger COMMA boolean COMMA posinteger COMMA
                        vtindex COMMA vtindex COMMA dnttpointer COMMA
			sltindex COMMA dnttpointer 
			{ dntt_blockdata($2,$4,$6,$8,$10,$12,$14,$16,$18,$20,$22); }

	|	IDNTBEGIN  boolean COMMA sltindex
			{ dntt_begin($2,$4); }

	|	IDNTEND posinteger COMMA boolean COMMA sltindex COMMA 
			dnttpointer
			{ dntt_end($2,$4,$6,$8); }

	|	IDNTIMPORT boolean COMMA vtindex COMMA vtindex
			{ dntt_import($2,$4,$6); }

	|	IDNTLABEL vtindex COMMA sltindex
			{ dntt_label($2,$4); }

	|	IDNTWITH posinteger COMMA boolean COMMA boolean COMMA posinteger COMMA
                        stattype COMMA sltindex COMMA dnttpointer COMMA vtindex COMMA
                        posinteger
			{ dntt_with($2,$4,$6,$8,$10,$12,$14,$16,$18, stattype_is_id); }

	|	IDNTCOMMON vtindex COMMA vtindex 
			{ dntt_common($2,$4); }

	|	IDNTFPARAM boolean COMMA boolean COMMA boolean COMMA 
			   boolean COMMA boolean COMMA vtindex COMMA 
			   dyntype COMMA dnttpointer COMMA dnttpointer COMMA
                           integer
			{ dntt_fparam($2,$4,$6,$8,$10,$12,$14,$16,$18,$20); }

	|	IDNTSVAR boolean COMMA boolean COMMA boolean COMMA 
			 boolean COMMA boolean COMMA vtindex COMMA 
			 stattype COMMA dnttpointer COMMA posinteger COMMA 
			 posinteger
			{ dntt_svar($2,$4,$6,$8,$10,$12,$14,$16,$18,$20,stattype_is_id); }

	|	IDNTDVAR boolean COMMA boolean COMMA boolean COMMA
			 boolean COMMA vtindex COMMA dyntype COMMA 
			 dnttpointer COMMA posinteger
			{ dntt_dvar($2,$4,$6,$8,$10,$12,$14,$16); }

	|	IDNTCONST boolean COMMA boolean COMMA posinteger COMMA
                          boolean COMMA vtindex COMMA stattype COMMA 
			  dnttpointer COMMA posinteger COMMA posinteger
                        { dntt_const($2,$4,$6,$8,$10,$12,$14,$16,$18,stattype_is_id); }

	|	IDNTTYPEDEF boolean COMMA vtindex COMMA dnttpointer
			{ dntt_typedef($2,$4,$6); }

	|	IDNTTAGDEF boolean COMMA boolean COMMA vtindex COMMA dnttpointer
			{ dntt_tagdef($2,$4,$6,$8); }

	|	IDNTPOINTER dnttpointer COMMA posinteger
			{ dntt_pointer($2,$4); }

	|	IDNTENUM dnttpointer COMMA posinteger
			{ dntt_enum($2,$4); }

	|	IDNTMEMENUM boolean COMMA vtindex COMMA integer COMMA 
			    dnttpointer
                          /* "value" is an unsigned long in symtab.h, but */
                          /* C allows negative enum ranges.               */
			{ dntt_memenum($2,$4,(unsigned long)$6,$8); }

	|	IDNTSET posinteger COMMA dnttpointer COMMA posinteger
			{ dntt_set($2,$4,$6); }

	|	IDNTSUBRANGE posinteger COMMA posinteger COMMA integer COMMA
                        integer COMMA dnttpointer COMMA posinteger
			{ dntt_subrange($2,$4,$6,$8,$10,$12); }

	|	IDNTARRAY posinteger COMMA posinteger COMMA posinteger COMMA
                          boolean COMMA boolean COMMA boolean COMMA boolean COMMA
                          posinteger COMMA dnttpointer COMMA dnttpointer COMMA posinteger
			{ dntt_array($2,$4,$6,$8,$10,$12,$14,$16,$18,$20,$22); }

	|	IDNTSTRUCT posinteger COMMA dnttpointer COMMA dnttpointer COMMA
			dnttpointer COMMA posinteger
			{ dntt_struct($2,$4,$6,$8,$10); }

	|	IDNTUNION dnttpointer COMMA posinteger
			{ dntt_union($2,$4); }

	|	IDNTFIELD boolean COMMA posinteger COMMA vtindex COMMA 
			  posinteger COMMA dnttpointer COMMA
			posinteger COMMA dnttpointer
			{ dntt_field($2,$4,$6,$8,$10,$12,$14); }

	|	IDNTVARIANT integer COMMA integer COMMA dnttpointer COMMA
                        posinteger COMMA dnttpointer
			{ dntt_variant($2,$4,$6,$8,$10); }

	|	IDNTFILE boolean COMMA posinteger COMMA posinteger COMMA dnttpointer
			{ dntt_file($2,$4,$6,$8); }

	|	IDNTFUNCTYPE boolean COMMA posinteger COMMA posinteger COMMA 
                        dnttpointer COMMA dnttpointer
			{ dntt_functype($2,$4,$6,$8,$10); }

	|	IDNTCOBSTRUCT boolean COMMA boolean COMMA dnttpointer COMMA 
                        dnttpointer COMMA dnttpointer COMMA dnttpointer COMMA
                        posinteger COMMA posinteger COMMA posinteger COMMA 
                        dnttpointer COMMA vtindex COMMA posinteger
			{ dntt_cobstruct($2,$4,$6,$8,$10,$12,$14,$16,$18,$20,$22,$24); }
	|	IDNTGENFIELD posinteger COMMA boolean COMMA 
			     dnttpointer COMMA dnttpointer
			{ dntt_genfield($2,$4,$6,$8); }


	|	IDNTMEMACCESS dnttpointer COMMA dnttpointer
			{ dntt_memaccess($2,$4); }
		

	|	IDNTMODIFIER boolean COMMA boolean COMMA boolean COMMA 
			     boolean COMMA boolean COMMA dnttpointer
			{ dntt_modifier($2,$4,$6,$8,$10,$12); }


	|	IDNTVFUNC boolean COMMA dnttpointer COMMA posinteger
			{ dntt_vfunc($2,$4,$6); }


	|	IDNTCLASSSCOPE sltindex COMMA dnttpointer
			{ dntt_classscope($2,$4); }


	|	IDNTFRIENDCLASS dnttpointer COMMA dnttpointer
			{ dntt_friendclass($2,$4); }


	|	IDNTFRIENDFUNC dnttpointer COMMA dnttpointer COMMA 
			       dnttpointer
			{ dntt_friendfunc($2,$4,$6); }


	|	IDNTCLASS boolean COMMA posinteger COMMA dnttpointer COMMA 
			  posinteger COMMA dnttpointer COMMA posinteger COMMA 
			  dnttpointer COMMA dnttpointer COMMA posinteger COMMA 
			  posinteger
			{ dntt_class($2,$4,$6,$8,$10,$12,$14,$16,$18,$20); }


	|	IDNTPTRMEM dnttpointer COMMA dnttpointer
			{ dntt_ptrmem($2,$4); }


	|	IDNTINHERITANCE boolean COMMA posinteger COMMA 
				dnttpointer COMMA posinteger COMMA 
				dnttpointer
			{ dntt_inheritance($2,$4,$6,$8,$10); }

	|	IDNTOBJECTID IDENT COMMA posinteger COMMA dnttpointer
			{ dntt_objectid($2,$4,$6); } 

	|	IDNTBLOCK integer COMMA integer COMMA integer  COMMA
			  stattype1 COMMA stattype2 COMMA stattype3
			{ dntt_block($2,$4,$6,$8,$10,$12,st_is_id1,st_is_id2,st_is_id3); } 

	|	IDNTMEMFUNC boolean COMMA posinteger COMMA boolean COMMA 
			posinteger COMMA boolean COMMA vtindex COMMA 
			vtindex COMMA dnttpointer COMMA dnttpointer 
			{ dntt_memfunc($2,$4,$6,$8,$10,$12,$14,$16,$18); }
			
	|	IDNTXREF posinteger COMMA xtpointer
			{ dntt_xref($2, $4, xref_sym); }

	|	IDNTSA posinteger COMMA vtindex
			{ dntt_sa($2,$4); }

	|	IDNTMACRO vtindex
			{ dntt_macro($2); }

	|	IDNTREFERENCE dnttpointer COMMA posinteger
			{ dntt_reference($2,$4); }

	| 	IDNTINCLUDE  STRING
			{
			  dntt_include(yytext);
			}

	| 	IDNTFIXUP  posinteger COMMA IDENT
			{
			  dntt_fixup(0,$2,$4);
			}

	| 	IDNTENDINCLUDE
			{
			  dntt_fixup(1,NULL,NULL);
			}

	|	ISLTNORMAL posinteger
			{ sltnormal($2); }

	|	ISLTSPECIAL integer COMMA posinteger COMMA dnttpointer
			{ sltspecial($2,$4,$6); }

	|	ISLTEXIT posinteger
			{ sltexit($2); }

	|	ISLTASST sltindex
			{ sltasst($2); }
/* vt */
 	| IVT   
			{ if (labellist != NULL)
				uerror("'vt' statement should not be labeled");
			  cgsect(SVT);
			}
/* vtbytes */
        | IVTBYTES  vt_bytelist
                        { if (labellist != NULL)
			      uerror("'vtbytes' statement should not be labeled");
		        }

/* vtfile */
	| IVTFILE  STRING
			{ if (labellist != NULL)
				uerror("'vtfile' statement should not be labeled");
			  open_vt_temp_file(yytext);
			}

/* xt */
 	| IXT   
			{ if (labellist != NULL)
				uerror("'xt' statement should not be labeled");
			  cgsect(SXT);
			}

        |       IXTINFO boolean COMMA boolean COMMA boolean COMMA boolean COMMA
                        boolean COMMA posinteger COMMA posinteger
                        { xt_info($2,$4,$6,$8,$10,$12,$14); }

        |       IXTLINK xtpointer
                        { xt_link($2, xref_sym); }

        |       IXTNAME posinteger
                        { xt_name($2); }

        |       IXTINFO1 boolean COMMA boolean COMMA boolean COMMA boolean COMMA
                        boolean COMMA posinteger COMMA posinteger
                        { if ($14 > 65535) 
			      uerror("xtinfo1 'line' must be less than 65525");
			  
			  xt_info1($2,$4,$6,$8,$10,$12,$14); }

        |       IXTINFO2A boolean COMMA boolean COMMA boolean COMMA boolean COMMA
                        boolean COMMA posinteger 
                        { xt_info2A($2,$4,$6,$8,$10,$12); }

        |       IXTINFO2B posinteger
                        { xt_info2B($2); }

        |       IXTBLOCK xt_datalist
                        { /* data added in "xt_data/xt_datalist" production */; }

/* Instructions */


	|	OPCODE  {ckalign(ALINSTRUCTION,1);} opt_instr_size operand_list
			{ match_and_verify($1,$3,operand_address, noperand);
			  generate_icode(I_INSTRUCTION, &instruction);
			}

	|	CINVCPUSHOP {ckalign(ALINSTRUCTION,1);} 
		  cache_list COMMA operand
			{	operand_address[0] = $3;
				operand_address[1] = $5;
				match_and_verify($1,SZNULL,operand_address, 2);
				generate_icode(I_INSTRUCTION, &instruction);
			}

	|	CINVCPUSHOP_JC {ckalign(ALINSTRUCTION,1);} cache_list
			{	operand_address[0] = $3;
				match_and_verify($1,SZNULL,operand_address, 1);
				generate_icode(I_INSTRUCTION, &instruction);
			}

	|	MOVEMOP {ckalign(ALINSTRUCTION,1);} 
		  opt_instr_size movem_operand COMMA movem_operand
			{ operand_address[0] = $4;
			  operand_address[1] = $6;
			  match_and_verify($1,$3,operand_address, 2);
			  generate_icode(I_INSTRUCTION, &instruction);
			}

	|	BITFONEOP {ckalign(ALINSTRUCTION,1);} bitfield_operand
			{ match_and_verify($1,SZNULL,operand_address, noperand);
			  generate_icode(I_INSTRUCTION, &instruction);
			}

	|	BITFTWOOP1 {ckalign(ALINSTRUCTION,1);} bitfield_operand COMMA
		  dregoperand
			{ match_and_verify($1,SZNULL,operand_address, noperand);
			  generate_icode(I_INSTRUCTION, &instruction);
			}
		

	|	BITFTWOOP2 {ckalign(ALINSTRUCTION,1);} dregoperand COMMA
		  bitfield_operand
			{ match_and_verify($1,SZNULL,operand_address, noperand);
			  generate_icode(I_INSTRUCTION, &instruction);
			}

	|	fpop {ckalign(ALINSTRUCTION,1);} opt_fpinstr_size {fp_size=$3;}
		  fpoperand_list
			/* Note that when opt_fpinstr_size is null, the 
			 * float constant actually gets read as a lookahead
			 * token before the {fp_size=$3;} action can be
			 * executed.  That is why the "new_stmt_setup"
			 * routine does "fp_size = SZNULL;" before every
			 * statement.
			 */
			{ match_and_verify($1,$3,operand_address, noperand);
			  instruction.cpid = fp_cpid;
			  generate_icode($1->iopclass+I_FPINSTRUCTION0-FPOP0, &instruction);
			}

	|	FPMOVEMOP {ckalign(ALINSTRUCTION,1);}  opt_fpinstr_size 
		  fpmovem_operand COMMA fpmovem_operand
			{ noperand = 2;
			  operand_address[0] = $4;
			  operand_address[1] = $6;
			  match_and_verify($1,$3,operand_address, noperand);
			  instruction.cpid = fp_cpid;
			  generate_icode(I_FPINSTRUCTION0, &instruction);
			}
	|	pmuop  {ckalign(ALINSTRUCTION,1);} opt_instr_size pmuoperand_list
			{ match_and_verify($1,$3,operand_address, noperand);
			  instruction.cpid = 0; /*PMUCPID;*/
			  generate_icode($1->iopclass+I_PMUINSTRUCTION0-PMUOP0, &instruction);
			}
	|	DRGOP {ckalign(ALINSTRUCTION,1);} opt_fpinstr_size {fp_size=$3;}
		dragonoperand_list
			{ 
# ifdef DRAGON
			  match_and_verify($1,$3,operand_address, noperand);
			  if (nerrors_this_stmt==0)
				translate_dragon(&instruction, operand_address);
# endif
			}
	;


setbyte:	 { data_align = ALBYTE;
		   data_type = I_BYTE;
		   ckalign(data_align, 1);
		  };

setlong:	 { data_align = ALLONG;
		   data_type = I_LONG;
		   ckalign(data_align, 1);
		  };

setshort:	 { data_align = ALSHORT;
		   data_type = I_SHORT;
		   ckalign(data_align, 1);
		  };

setfloat:	 { data_align = ALFLOAT;
		   data_type = I_FLOAT;
		   ckalign(data_align, 1);
		   fp_size = SZSINGLE;
		  };

setdouble:	 { data_align = ALDOUBLE;
		   data_type = I_DOUBLE;
		   ckalign(data_align, 1);
		   fp_size = SZDOUBLE;
		  };

setpacked:	 { data_align = ALPACKED;
		   data_type = I_PACKED;
		   ckalign(data_align, 1);
		   fp_size = SZPACKED;
		  };

setextend:	 { data_align = ALEXTEND;
		   data_type = I_EXTEND;
		   ckalign(data_align, 1);
		   fp_size = SZEXTEND;
		  };

identifier_list: IDENT
		{ idlist = NULL; 
		  save_ident(&idlist, $1);
		}
	|	identifier_list COMMA IDENT
		{ save_ident(&idlist, $3); }
	;

simple_operand:	soperand
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = $1; }
	|	roperand
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = $1; }
	;

operand:	reg_pair
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = (($1->rpreg1.regkind == DREG) &&
			   ($1->rpreg2.regkind == DREG))? A_DREGPAIR: A_REGPAIR;
			  $$->unoperand.regpair = $1; }
	|	simple_operand
			{ $$ = $1; }
	|	cregaddrmode
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = $1;
			}
	;

operand_list:	/* empty */
			{ noperand = 0; }
	|	operand_list1
			{ if (noperand > MAXOPERAND) {
				uerror("too many operands");
				noperand = MAXOPERAND;
				}
			}
	;

operand_list1:	/* one or more operands */
		operand
			{ operand_address[0] = $1;
			  noperand = 1;
			}
	|	operand_list1  COMMA operand
			{ if(noperand < MAXOPERAND)
				operand_address[noperand] = $3;
			   noperand++;
			}
	;

cache_list:	CACHEREG
		{	$$ = ALLOCATE_NODE(operand_addr1);
			$$->opadtype = A_CACHELIST;
			$$->unoperand.reglist = BITMASK($1.regno,1);
		}
	| CACHEREG minus_div CACHEREG
		{	int reglo, reghi;
			reglo = $1.regno;
			reghi = $3.regno;
			$$ = ALLOCATE_NODE(operand_addr1);
			$$->opadtype = A_CACHELIST;
			if (reglo > reghi)
				$$->unoperand.reglist = BITMASK(reglo,reglo-reghi+1);
			else
				$$->unoperand.reglist = BITMASK(reghi,reghi-reglo+1);
		}
	;

minus_div:	MINUS
	| DIV
	;

movem_operand:	soperand
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = $1; }
	|	reglist
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_REGLIST;
			  $$->unoperand.reglist = $1; }
	;

fpoperand:	simple_operand
			{ $$ = $1; }
	|	simple_operand fppacked_specifier
			{ operand_address[noperand++] = $2;
			  $$ = $1;
			}
	|	FPDREG
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_FPDREG;
			  $$->unoperand.soperand->adreg1 = $1;
			}
	|	FPCREG
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_FPCREG;
			  $$->unoperand.soperand->adreg1 = $1;
			}
	|	fpdreg_pair
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_FPDREGPAIR;
			  $$->unoperand.regpair = $1;
			}
	;

fpmovem_operand:  simple_operand
			{ $$ = $1; }
	|	fpdreglist
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_FPDREGLIST;
			  $$->unoperand.reglist = $1;
			}
	|	fpcreglist
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_FPCREGLIST;
			  $$->unoperand.reglist = $1;
			}
	;
			
fpoperand_list:	/* empty */
			{ noperand = 0; }
	|	fpoperand_list1
			{ if (noperand > MAXOPERAND) {
				uerror("too many operands");
				noperand = MAXOPERAND;
				}
			}
	;

fpoperand_list1:	/* one or more fpoperands */
		fpoperand
			{ operand_address[noperand++] = $1;
			}
	|	fpoperand_list1  COMMA fpoperand
			{ if(noperand < MAXOPERAND)
				operand_address[noperand] = $3;
			  noperand++;
			}
	;

fpdreglist:
		fpdreglist_component
			{ $$ = $1; }
	|	fpdreglist DIV fpdreglist_component
			{ $$ = $1 | $3; }
	;

fpdreglist_component:	FPDREG
			{ $$ = BITMASK(7-$1.regno,1); }
	|	FPDREG  MINUS  FPDREG
			{ int reglo, reghi;
			  reglo = $1.regno;
			  reghi = $3.regno;
			  if (reglo > reghi) {
				uerror("illegal register range");
				$$ = 0;
				}
			  else
			    $$ = BITMASK(7-reglo,reghi-reglo+1);
			}
	;

fpcreglist:	FPCREG
			{ $$ = $1.regno; }
	|	fpcreglist DIV FPCREG
			{ $$ = $1 | $3.regno; }
	;

fpdreg_pair:	FPDREG COLON FPDREG
			{ $$ = ALLOCATE_NODE(register_pair);
			  $$->rpreg1 = $1;
			  $$->rpreg2 = $3;
			}
	;

fppacked_specifier:	LCB  AND iexpr RCB
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_FPPKSPECIFIER;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_IMM;
			  $$->unoperand.soperand->adexptr1 = $3;
			}
	|	LCB  DREG RCB
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_FPPKSPECIFIER;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_DREG;
			  $$->unoperand.soperand->adreg1 = $2;
			}
	;

fpop:		FPOP0 { $$ = $1; }
	|	FPOP1 { $$ = $1; }
	|	FPOP2 { $$ = $1; }
	|	FPOP4 { $$ = $1; }
	|	FPOP5 { $$ = $1; }
	;

/* Really would like to just if-def out all this whole section of 
 * productions for DRAGON, but no clean way to do this within yacc.
 */
dragonoperand:	simple_operand
			{ $$ = $1; }
	|	DRGDREG
			{ 
# ifdef DRAGON
			  $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_DRGDREG;
			  $$->unoperand.soperand->adreg1 = $1;
# endif
			}
	|	DRGCREG
			{ 
# ifdef DRAGON
			  $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_DRGCREG;
			  $$->unoperand.soperand->adreg1 = $1;
# endif
			}
	|	reg_pair
			{ 
# ifdef DRAGON
			  $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = (($1->rpreg1.regkind == DREG) &&
			   ($1->rpreg2.regkind == DREG))? A_DREGPAIR: A_REGPAIR;
			  $$->unoperand.regpair = $1;
# endif
			}
	;


dragonoperand_list:	/* empty */
			{ noperand = 0; }
	|	dragonoperand_list1  opt_nowait
			{ 
# ifdef DRAGON
			  if (noperand > MAXOPERAND) {
				uerror("too many operands");
				noperand = MAXOPERAND;
				}
# endif
			}
	;

dragonoperand_list1:	/* one or more dragonoperands */
		dragonoperand
			{
# ifdef DRAGON
			  operand_address[noperand++] = $1;
# endif
			}
	|	dragonoperand_list1  COMMA dragonoperand
			{ 
# ifdef DRAGON
			  if(noperand < MAXOPERAND)
				operand_address[noperand] = $3;
			  noperand++;
# endif
			}
	;

opt_nowait:
		/* empty */
			{ 
# ifdef DRAGON
			  dragon_nowait = 0; 
# endif
			}
	|	COMMA	NOWAIT
			{ 
# ifdef DRAGON
			  dragon_nowait = 1; 
# endif
			}


pmuop:		PMUOP0 { $$ = $1; }
	|	PMUOP1 { $$ = $1; }
	|	PMUOP2 { $$ = $1; }
	|	PMUOP4 { $$ = $1; }
	|	PMUOP5 { $$ = $1; }
	;

pmuoperand:	simple_operand
			{ $$ = $1; }
	|	PMUREG
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_PMUREG;
			  $$->unoperand.soperand->adreg1 = $1;
			}
	|	CREG
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_CREG;
			  $$->unoperand.soperand->adreg1 = $1;
			}
	;

pmuoperand_list:	/* empty */
			{ noperand = 0; }
	|	pmuoperand_list1
			{ if (noperand > MAXOPERAND) {
				uerror("too many operands");
				noperand = MAXOPERAND;
				}
			}
	;

pmuoperand_list1:	/* one or more pmuoperands */
		pmuoperand
			{ operand_address[noperand++] = $1;
			}
	|	pmuoperand_list1  COMMA pmuoperand
			{ if(noperand < MAXOPERAND)
				operand_address[noperand] = $3;
			  noperand++;
			}
	;

dregoperand:	DREG
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->admode = A_DREG;
			  $$->unoperand.soperand->adreg1 = $1;
			  operand_address[noperand++] = $$;
			}


roperand:	/* %dn */
		/* data register direct */
		DREG
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_DREG;
			  $$->adreg1 = $1;
			}

	|	/* %an */
		/* address register direct */
		AREG
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_AREG;
			  $$->adreg1 = $1;
			}
	;

cregaddrmode:	/* %cc, %sr, or %creg */
		CCREG
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_CCREG;
			  $$->adreg1 = $1;
			}
	|	SRREG
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_SRREG;
			  $$->adreg1 = $1;
			}
	|	CREG
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_CREG;
			  $$->adreg1 = $1;
			}
	|	PMUREG
			{ /* need this rule since the 040 allows 2 pmu regs as 
				 operands to "movec".								*/
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_PMUREG;
			  $$->adreg1 = $1;
			}
	;

soperand:	/* (%an) */
		/* address register indirect */
		LP  AREG  RP
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_ABASE;
			  $$->adreg1 = $2;
			}

	|	/* (%an)+ */
		/* address register indirect, with post-increment */
		LP AREG RP PLUS
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_AINCR;
			  $$->adreg1 = $2;
			}

	|	/* -(%an) */
		/* address register indirect, with pre-decrement */
		MINUS LP AREG RP
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_ADECR;
			  $$->adreg1 = $3;
			}

	|	/* d(%an) */
		/* d(%pc) */
		/* Note Motorola always assumes a 16-bit displacement here.
		 * We do a general 16- or 32-bit, as needed.
		 */
		iexpr LP pc_areg RP
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($3.regkind==PC || $3.regkind==ZPC)?
				A_PCDISP : A_ADISP;
			  $$->adreg1 = $3;
			  $$->adexptr1 = $1;
			}


	|	/* d(%an,%xm) */
		/* d(%pc,%xm) */
		/* Note Motorola always assumes a 8-bit displacement here. */
		iexpr LP pc_areg COMMA xreg RP
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($3.regkind==PC  || $3.regkind==ZPC)? 
				A_PCINDX : A_AINDX;
			  $$->adexptr1 = $1;
			  $$->adreg1 = $3;
			  $$->adxreg = *$5;
			}

	|	/* (d, areg, xreg) */
		/* (d, pc, xreg ) */
		LP  iexpr COMMA pc_areg COMMA xreg RP
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($4.regkind==PC || $4.regkind==ZPC)?
				A_PCINDX : A_AINDX;
			  $$->adexptr1 = $2;
			  $$->adreg1 = $4;
			  $$->adxreg = *$6;
			}

	|	/* (d, areg) */
		/* (d, pc)   */
		LP iexpr COMMA pc_areg RP
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($4.regkind==PC || $4.regkind==ZPC)?
				A_PCDISP : A_ADISP;
			  $$->adexptr1 = $2;
			  $$->adreg1 = $4;
			}
	/***** All of these A_AINDX cases could be collaped to use a
	 *****  disp_areg_xreg
	 ***** production that could be shared with the memory-indirect
	 ***** pre-indexed case.
	 */
	|	/* (d, xreg2) */
		LP iexpr COMMA xreg2 RP
			{ 
# ifdef M68020
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_AINDX;
			  $$->adexptr1 = $2;
			  $$->adxreg = *$4;
			  $$->adreg1.regkind = NOREG;
# else
			  uerror("illegal addressing mode for 68010");
			  YYERROR;
# endif
			}

	|	/* (areg, xreg) */
		LP pc_areg COMMA xreg RP
			{ 
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($2.regkind==PC || $2.regkind==ZPC)?
				A_PCINDX : A_AINDX;
			  $$->adreg1 = $2;
			  $$->adxreg = *$4;
			  $$->adexptr1 = NULL;
			}

	|	/* (xreg2) */
		LP xreg2 RP
			{ 
# ifdef M68020
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_AINDX;
			  $$->adxreg = *$2;
			  $$->adreg1.regkind = NOREG;
			  $$->adexptr1 = NULL;
# else
			  uerror("illegal addressing mode for 68010");
			  YYERROR;
# endif
			}

	|	/* (%zareg) */
		LP ZAREG RP
			{ 
# ifdef M68020
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_AINDX;
			  $$->adreg1 = $2;
			  $$->adxreg.xrreg.regkind = NOREG;
			  $$->adexptr1 = NULL;
# else
			  uerror("illegal addressing mode for 68010");
			  YYERROR;
# endif
			}

	|	/* (pc) */
		LP pc RP
			{ /* ??? is this A_PCDISP with DISP==0 or
			     A_PCINDX with bd==0 and xreg suppressed */
			  
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_PCDISP;
			  $$->adreg1 = $2;
			  $$->adexptr1 = NULL;
			}

	|	/* ( [bd, areg], xreg, od ) */
		LP LB mem_indirect RB opt_xreg_disp RP
			{ 
# ifdef M68020
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($3->adreg1.regkind==PC ||
				$3->adreg1.regkind==ZPC)?A_PCMEM : A_AMEM ;
			  $$->adexptr1 = $3->adexptr1;
			  $$->adreg1 = $3->adreg1;
			  $$->adxreg = $5->adxreg;
			  $$->adexptr2 = $5->adexptr2;
# else
			  uerror("illegal addressing mode for 68010");
			  YYERROR;
# endif
			}

	|	/* ( [bd, areg, xreg], od ) */
		LP LB mem_indirect_index RB opt_disp RP
			{ 
# ifdef M68020
			  $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = ($3->adreg1.regkind==PC ||
				$3->adreg1.regkind==ZPC)?A_PCMEMX : A_AMEMX ;
			  $$->adexptr1 = $3->adexptr1;
			  $$->adreg1 = $3->adreg1;
			  $$->adxreg = $3->adxreg;
			  $$->adexptr2 = $5;
# else
			  uerror("illegal addressing mode for 68010");
			  YYERROR;
# endif
			}


	|	/* absolute */
		iexpr
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_ABS;
			  $$->adexptr1 = $1;
			  $$->adreg1.regkind = $$->adxreg.xrreg.regkind = NOREG;
			}

	|	iexpr SIZE_SUFFIX
			{ $$ = ALLOCATE_NODE(addrmode1);
			  if ($2==SZWORD)
				$$->admode = A_ABS16;
			  else if ($2==SZLONG)
				$$->admode = A_ABS32;
			  else {
				uerror("illegal size suffix for absolute address");
				$$->admode = A_ABS;
				}
			  $$->adexptr1 = $1;
			  $$->adreg1.regkind = $$->adxreg.xrreg.regkind = NOREG;
			}


	|	/* & expression */
		AND expr
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->admode = A_IMM;
			  $$->adreg1.regno = $$->adxreg.xrreg.regno = NOREG;
			  $$->adexptr1 = $2;
			}
	;

reg_pair:	/* %dx:%dy	for div and mul instructions.
		 * %rx:%ry	for cas2
		 */
		dareg  COLON  dareg
			{ 
# ifdef M68020
			  $$ = ALLOCATE_NODE(register_pair);
			  $$->rpreg1 = $1;
			  $$->rpreg2 = $3;
# else
			  uerror("illegal addressing mode for 68010");
			  YYERROR;
# endif
			}
	;

dareg:		AREG	{ $$ = $1; }
	|	DREG	{ $$ = $1; }
	;

bitfield_operand: 
		simple_operand  bitfield_specifier
			{ operand_address[noperand++] = $2;
			  operand_address[noperand++] = $1;
			  
			}
	;

bitfield_specifier: /*  {offset:width} */
		LCB  bit_offset  COLON  bit_width  RCB
			{ $$ = ALLOCATE_NODE(operand_addr1);
			  $$->unoperand.soperand = ALLOCATE_NODE(addrmode1);
			  $$->unoperand.soperand->adexptr1 = $2->adexptr1;
			  $$->unoperand.soperand->adreg1  = $2->adreg1;
			  $$->unoperand.soperand->adexptr2 = $4->adexptr1;
			  $$->unoperand.soperand->adxreg.xrreg  = $4->adreg1;
			  $$->opadtype = A_SOPERAND;
			  $$->unoperand.soperand->admode = A_BFSPECIFIER;
			}
	;

bit_offset:	bit_offset_or_width   { $$ = $1; }
	;

bit_width:	bit_offset_or_width   { $$ = $1; }
	;

bit_offset_or_width:
		DREG
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adreg1 = $1;
			  $$->adexptr1 = NULL;
			}
	|	AND iexpr
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adreg1.regkind = NOREG;
			  $$->adexptr1 = $2;
			}
	;

reglist:
		reglist_component
			{ $$ = $1; }
	|	reglist DIV reglist_component
			{ $$ = $1 | $3; }
	;

reglist_component:
		AREG
			{ $$ = BITMASK($1.regno + 8,1); }
	|	DREG
			{ $$ = BITMASK($1.regno,1); }
	|	AREG  MINUS  AREG
			{ int reglo, reghi;
			  reglo = $1.regno;
			  reghi = $3.regno;
			  if (reglo > reghi) {
				uerror("illegal register range");
				$$ = 0;
				}
			  else
			    $$ = BITMASK(reghi+8,reghi-reglo+1);
			}
	|	DREG  MINUS  DREG
			{ int reglo, reghi;
			  reglo = $1.regno;
			  reghi = $3.regno;
			  if (reglo > reghi) {
				uerror("illegal register range");
				$$ = 0;
				}
			  else
			    $$ = BITMASK(reghi,reghi-reglo+1);
			}
	;


xreg:		xreg2	{ $$ = $1; }
	|	areg
			{ $$ = ALLOCATE_NODE(xreg);
			  $$->xrreg = $1;
			  $$->xrsize = SZNULL;
			  $$->xrscale = XSF_NULL;
			}
	;

xreg2:		dreg opt_index_size opt_index_scale
			{ $$ = ALLOCATE_NODE(xreg);
			  $$->xrreg = $1;
			  $$->xrsize = $2;
			  $$->xrscale = $3;
			}
	|	areg index_size opt_index_scale
			{ $$ = ALLOCATE_NODE(xreg);
			  $$->xrreg = $1;
			  $$->xrsize = $2;
			  $$->xrscale = $3;
			}
	|	areg opt_index_size index_scale
			{ $$ = ALLOCATE_NODE(xreg);
			  $$->xrreg = $1;
			  $$->xrsize = $2;
			  $$->xrscale = $3;
			}
	;


areg:		AREG	{ $$ = $1; }
	|	ZAREG	{ $$ = $1; }
	;

dreg:		DREG	{ $$ = $1; }
	|	ZDREG	{ $$ = $1; }
	;

pc:		PC	{ $$ = $1; }
	|	ZPC	{ $$ = $1; }
	;

pc_areg:	pc
			{ $$ = $1; }
	|	areg	{ $$ = $1; }
	;

mem_indirect:  iexpr
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adexptr1 = $1;
			  $$->adreg1.regkind = NOREG;
			}
	|	iexpr COMMA pc_areg
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adexptr1 = $1;
			  $$->adreg1 = $3;
			}
	|	pc_areg
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adexptr1 = NULL;
			  $$->adreg1 = $1;
			}
	;

mem_indirect_index:
		xreg2
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adreg1.regkind = NOREG;
			  $$->adxreg = *$1;
			  $$->adexptr1 = NULL;
			}
	|	pc_areg COMMA xreg
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adreg1 = $1;
			  $$->adxreg = *$3;
			  $$->adexptr1 = NULL;
			}
	|	iexpr COMMA xreg2
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adreg1.regkind = NOREG;
			  $$->adxreg = *$3;
			  $$->adexptr1 = $1;
			}
	|	iexpr COMMA pc_areg COMMA xreg
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adreg1 = $3;
			  $$->adxreg = *$5;
			  $$->adexptr1 = $1;
			}
	;

opt_xreg_disp:	COMMA xreg opt_disp
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adxreg = *$2;
			  $$->adexptr2 = $3;
			}
	|	opt_disp
			{ $$ = ALLOCATE_NODE(addrmode1);
			  $$->adxreg.xrreg.regkind = NOREG;
			  $$->adexptr2 = $1;
			}
	;

opt_disp:	/* empty */
			{ $$ = ALLOCATE_NODE(rexpr);
			  $$->exptype = TYPNULL;
			}
	|	COMMA iexpr
			{ $$ = $2;
			}
	;



opt_index_size:	/* empty */
			{ $$ = SZNULL; }
	|	index_size
			{ $$ = $1; }
	;

index_size:	SIZE_SUFFIX
			{ /* verify that size suffix is valid */
			  if ($1 == SZWORD || $1 == SZLONG)
				$$ = $1;
			  else {
				uerror("illegal size for index register");
				$$ = SZNULL;
				}
			}
	;

opt_index_scale:	/* empty */
			{ $$ = XSF_NULL; }
	|	index_scale
			{ $$ = $1; }
	;

index_scale:	MUL INTNUM
			{ $$ = check_index_scale($2); }
	;

opt_instr_size:		/* empty */
			{ $$ = SZNULL; }
	|	SIZE_SUFFIX
			{ $$ = $1; }

opt_fpinstr_size:	/* empty */
			{ $$ = SZNULL; }
	|	fp_size_suffix
			{ $$ = $1; }
	;

fp_size_suffix:	SIZE_SUFFIX
			{ $$ = $1; }
	|	FP_SIZE_SUFFIX
			{ $$ = $1; }
	;

iexpr_datalist:	iexpr_data
	|	iexpr_datalist COMMA iexpr_data
	;

iexpr_strg_datalist: iexpr_strg_data
	|	iexpr_strg_datalist COMMA iexpr_strg_data
	;

iexpr_strg_data:	iexpr_data
	|	STRING
			{ /*ckalign(data_align,0);*/
			  generate_icode(I_STRING, yytext, $1);
			  /*newdot += nbytes;*/
			}
	;


fexpr_datalist:	fexpr_data
		|	fexpr_datalist COMMA fexpr_data
		;



fexpr_data:		expr = { if (($1->exptype&TFLT)==0)
				uerror("illegal floating point expression");
			  $$=$1;
			  /*ckalign(data_align,0);*/
			  generate_icode(data_type, $1, sizeof(*$1) );
		       }
		;


iexpr_data:		expr = { if (($1->exptype&TFLT)!=0)
				uerror("illegal integer expression");
			  $$=$1;
			  ckalign(data_align,0);
			  generate_icode(data_type, $1, sizeof(*$1) );
		       }

iexpr:		expr = { if (($1->exptype&TFLT)!=0)
				uerror("expression not of type integer");
			  $$=$1;
		       }


expr:		IDENT
			{ $$ = ALLOCATE_NODE(rexpr);
			  if (($1->stype&STYPE)!=SABS) {
				/* we don't currently allow the cdb-type
				 * labels in general expressions. */
				if ($1->stype&(SGNTT|SLNTT|SSLT|SVT|SXT))
				   uerror("illegal symbol type for general expression");
				$$->exptype = TSYM;
			  	$$->symptr = $1;
			  	$$->expval.lngval = 0L;
				}
			  else { /* an abs symbol is just like an intnum */
				$$->exptype = TYPL;
				$$->symptr = NULL;
				$$->expval.lngval = $1->svalue;
				};
			}

	|	INTNUM
			{ $$ = ALLOCATE_NODE(rexpr);
			  $$->exptype = TYPL;
			  $$->expval.lngval = $1;
			}

	|	FLTNUM
			{ $$ = ALLOCATE_NODE(rexpr);
			  $$->exptype = float_exptype(fp_size);
			  $$->expval = *$1;
			   /* ??? might consider storing singles as TYPD so
			    * can do arithmetic on them -- w/o adding TYPF
			    * to all the cases in the constant folder.
			    */
			}

	|	LP expr RP
			{ $$=$2; }

	|	expr PLUS expr
			{ $$ = operate(PLUS,$1,$3); }

	|	expr MINUS expr
			{ $$ = operate(MINUS,$1,$3); }

	|	expr IOR expr
			{ $$ = operate(IOR,$1,$3); }

	|	expr XOR expr
			{ $$ = operate(XOR,$1,$3); }

	|	expr AND expr
			{ $$ = operate(AND,$1,$3); }

	|	expr LSH expr
			{ $$ = operate(LSH,$1,$3); }

	|	expr RSH expr
			{ $$ = operate(RSH,$1,$3); }

	|	expr MUL expr
			{ $$ = operate(MUL,$1,$3); }

	|	expr DIV expr
			{ $$ = operate(DIV,$1,$3); }

	|	expr MOD expr
			{ $$ = operate(MOD,$1,$3); }

	|	MINUS expr %prec UNOP
			{ $$ = operate(-MINUS,$2,NULL); }

	|	PLUS expr %prec UNOP
			{ $$ = $2; }

	|	TILDE expr %prec UNOP
			{ $$ = operate(-TILDE,$2,NULL); }

	;

/* Productions to support cdb-pseduo-ops operand types */

vt_bytelist: vt_byte
             |	vt_bytelist COMMA vt_byte
             ;

vt_byte:     integer
             { $$=$1;
	       generate_vtbyte( (int) $1);
	     }
             ;

xt_datalist: xt_data
             |	xt_datalist COMMA xt_data
             ;

xt_data:     integer
             { $$=$1;
	       generate_xtdata($1);
	     }
             ;

xtpointer:	integer
			{ $$=$1;
                          xref_sym=0;
		        }
	|	IDENT	{ $$=0;
                          xref_sym=$1;
		        }
	;

dnttpointer:	integer
			{ $$=$1;
			  /* the high bit must be set to be a legal dnttpointer.
			   * Check this because if it's not, the dnttfixup
			   * routines will think this is for a symbol.
			   */
			  if ( !($$ & 0x80000000) )
				uerror("illegal value for dnttpointer(extension bit not set)");
			}
	|	IDENT	{ /* This is really kludgy an non-portable.
			   * In order to distinguish a symbol dntt from
			   * an immediate or indexed, we rely on the
			   * fact that our machine is aligning symbol
			   * structures on an even boundary.  So the
			   * low address bit is 0.  We shift it off here
			   * to be sure the high bit is zer0, then
			   * we'll shift it back in the dnttfixup.
			   */
			   if ((int)$1&01)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			     aerror("unexpected alignment for symbol entry");
			   $$ = ((unsigned int)$1>>1); }
	;

sltindex:	posinteger  { $$=$1; };

vtindex:	posinteger  { $$=$1; };

dyntype:	integer	{ $$=$1; };

stattype1:	stattype
			{ $$ = (int)$1;
		          st_is_id1 = stattype_is_id;
                        };

stattype2:	stattype
			{ $$ = (int)$1;
		          st_is_id2 = stattype_is_id;
                        };

stattype3:	stattype
			{ $$ = (int)$1;
		          st_is_id3 = stattype_is_id;
                        };

stattype:	IDENT
			{ $$ = (int)$1;
		          stattype_is_id = 1; 
                        }
	|	integer
			{ 
			  /* For SVARs, this must be "-1", but for CONST and */
			  /* WITH it does not have to be.  Check for these   */
			  /* restrictions in the "dnt_x" routines in debug.c */
			  /* ----------------------------------------------- */
			  $$ = (unsigned long)$1;
			  stattype_is_id = 0; 
			}
	;

address:	IDENT { $$ = (long) $1; }
	;

boolean:	INTNUM
			{ if (($1 != 0) && ($1 != 1)) {
				uerror("boolean (0/1) value required");
				$$ = $1;
				}
			  else $$=$1;
			}
	;

integer:	INTNUM		{ $$ = $1; }
	|	MINUS INTNUM 	{ $$ = - $2; }
	;

posinteger:	INTNUM
			{ if((long)$1 < 0) {
				uerror("illegal negative value");
				$$=0;
				}
			  else $$ = (long)$1; }

%%

rexpr bad_expr = { TYPNULL };

/* future tuning issue: just put the result in the left operand, and
 * return a pointer, not a structure.
 */

/***************************************************************************
 * operate
 *
 * perform constant folding
 */
rexpr * operate(op,leftptr,rightptr)

int op;
rexpr *leftptr, *rightptr;

	{ rexpr * resultptr;
	  rexpr left, right;

	  resultptr = ALLOCATE_NODE(rexpr);
	  
	  if (op < 0) { /* unary operator  - or ~ */
		  left = *leftptr;
		  op = - op;
		  switch (op) {

			case MINUS: switch (left.exptype) {

					case TYPL: resultptr->exptype = TYPL;
						   resultptr->symptr = NULL;
						   resultptr->expval.lngval =
						   - left.expval.lngval;
						   return(resultptr);

					case TYPD: resultptr->exptype = TYPD;
						   resultptr->symptr = NULL;
						   resultptr->expval.dblval =
						   - left.expval.dblval;
						   return(resultptr);

					default: uerror("illegal negation");
						 return(&bad_expr);
				    };

			case TILDE: switch (left.exptype) {

					case TYPL: resultptr->exptype = TYPL;
						   resultptr->symptr = NULL;
						   resultptr->expval.lngval =
						   ~ left.expval.lngval;
						   return(resultptr);

					default: uerror("illegal bit complement");
						 return(&bad_expr);
				    };

			default: uerror("illegal expression");
				 return(&bad_expr);
			
			};
	   	}
	  else  /* binary operator */  {
		left = *leftptr;
		right = *rightptr;
		switch (op)  {

			case DIV: if (left.exptype==TYPL&&right.exptype==TYPL) {
					if (right.expval.lngval==0) {
					   uerror("division by 0");
					   return(&bad_expr);
					   }
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval/right.expval.lngval;
					return(resultptr);

				} else

				if (left.exptype==TYPD&&right.exptype==TYPD) {
					resultptr->exptype = TYPD;
					resultptr->symptr = NULL;
					resultptr->expval.dblval =
					left.expval.dblval/right.expval.dblval;
					return(resultptr);

				} else

					uerror("illegal division");
					return(&bad_expr);
				break;

			case MOD: if (left.exptype==TYPL&&right.exptype==TYPL) {
					if (right.expval.lngval==0) {
					   uerror("division by 0");
					   return(&bad_expr);
					   }
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval%right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal mod operation");
					return(&bad_expr);
				break;

			case MUL: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval*right.expval.lngval;
					return(resultptr);

				} else

				if (left.exptype==TYPD&&right.exptype==TYPD) {
					resultptr->exptype = TYPD;
					resultptr->symptr = NULL;
					resultptr->expval.dblval =
					left.expval.dblval*right.expval.dblval;
					return(resultptr);

				} else

					uerror("illegal multiplication");
					return(&bad_expr);
				break;

			case RSH: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval>>right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal right shift");
					return(&bad_expr);
				break;

			case LSH: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval<<right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal left shift");
					return(&bad_expr);
				break;

			case AND: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval&right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal and");
					return(&bad_expr);
				break;

			case XOR: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval^right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal xor");
					return(&bad_expr);
				break;

			case IOR: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval|right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal ior");
					return(&bad_expr);
				break;

			case MINUS: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval-right.expval.lngval;
					return(resultptr);

				} else

				if (left.exptype==TYPD&&right.exptype==TYPD) {
					resultptr->exptype = TYPD;
					resultptr->symptr = NULL;
					resultptr->expval.dblval =
					left.expval.dblval-right.expval.dblval;
					return(resultptr);

				} else
				
				if ((left.exptype == TSYM) &&
				    (right.exptype == TSYM)) {
					/* we do the subtraction "if we can"
					 * else build a TDIFF.
					 */
					if (left.symptr == right.symptr) 
					{
					  resultptr->exptype = TYPL;
					  resultptr->expval.lngval = 
						left.expval.lngval -
						right.expval.lngval;
					    return(resultptr);
					}
					else if(defsym(left) &&
					  (left.symptr->stype&STYPE)==
					  (right.symptr->stype&STYPE)
					   && (!sdopt_flag 
					      ||(left.symptr->stype&STYPE)!=STEXT
					      || p1subtract_flag)
					   /* || #not in switch table# */  )
					  { resultptr->exptype = TYPL;
					    resultptr->expval.lngval = left.symptr->
						svalue + left.expval.lngval -
						(right.symptr->svalue + right.
						expval.lngval);
					    return(resultptr);
					  }
					else if (left.expval.lngval==0 &&
					  right.expval.lngval==0) {
					/* do following when implement
					/* span dependent.  Then need
					/* pseduo-op's to flag if
					/* we are inside a switch.
					 */
					   resultptr->exptype = (TSYM | TDIFF);
					   resultptr->symptr = left.symptr;
					   resultptr->expval.lngval=(long)right.symptr;
					   return(resultptr);
					   }
					else {
					    uerror("illegal subtraction");
					    return(&bad_expr);
					    }

				} else

				if ((left.exptype == TSYM) &&
				    (right.exptype == TYPL)) {
					resultptr->exptype = TSYM;
					resultptr->symptr = left.symptr;
					resultptr->expval.lngval = 
					left.expval.lngval-right.expval.lngval;
					return(resultptr);

				} else

					uerror("illegal subtraction");
					return(&bad_expr);
				break;

			case PLUS: if (left.exptype==TYPL&&right.exptype==TYPL) {
					resultptr->exptype = TYPL;
					resultptr->symptr = NULL;
					resultptr->expval.lngval =
					left.expval.lngval+right.expval.lngval;
					return(resultptr);

				} else

				if (left.exptype==TYPD&&right.exptype==TYPD) {
					resultptr->exptype = TYPD;
					resultptr->symptr = NULL;
					resultptr->expval.dblval =
					left.expval.dblval+right.expval.dblval;
					return(resultptr);

				} else

				if (((left.exptype==TYPL)&&(right.exptype==TSYM)) ||
				    ((left.exptype==TSYM)&&(right.exptype==TYPL))) {
					resultptr->exptype = TSYM;
					resultptr->expval.lngval = 
				        left.expval.lngval+right.expval.lngval;
					if (left.exptype==TSYM) 
						resultptr->symptr = left.symptr;
					else 
						resultptr->symptr = right.symptr;
					return(resultptr);

				} else

					uerror("illegal add");
					return(&bad_expr);
				break;

			      default: uerror("illegal expression");
					return(&bad_expr);

			      };
		  
	  	};

	}  /* operate */


/*****************************************************************************
 *  fill
 * 
 * Generate nbytes of filler into the current segment.
 */
fill(nbytes)
long nbytes;
{
	long fillval;

	fillval = (dot->stype == STEXT)	? TXTFILL : FILL;
	generate_icode(I_FILL,fillval, nbytes );
	/*newdot += nbytes;*/

} /* fill */


/*****************************************************************************
 * ckalign
 *
 * Increment the current section's location counter value (newdot) to
 * align it on an "almod" boundary.
 * If "update_flag" is non-zero, then also update the actual dot symbol
 * at this time, rather than just newdot.
 */
ckalign(almod,update_flag)
  long almod;
  int update_flag;
{
  long mod;
  long nbytes;
  if (almod <= 0) {
	uerror("illegal local alignment value");
	return;
	}
  if ((mod = newdot % almod) != 0) {
	nbytes = almod - mod;
	if (dot->stype != SBSS)
	   fill(nbytes);
	else
	   newdot += nbytes;
	if (update_flag) {
	   dot->svalue = newdot;
	   labdot = newdot;
# if defined(LISTER1) || defined(LISTER2)
	   listdot = newdot;
# endif
	   }
  }
} /* ckalign */


/*****************************************************************************
 * save_ident
 *
 * Link a symbol into a linked list via the "slink" field.  
 * A check is made that the symbol has not already been linked into
 * a list, to avoid circular list/infinite loop problems (when the
 * same label is multiply defined in a global list for example).
 */
save_ident(list,sym)
  symbol **list, *sym;
{ if (sym->slink != NULL || *list==sym ) {
	werror("identifier (%s) occurs more than once in idlist",
	  sym->snamep);
	return;
	}
  sym->slink = *list;
  *list = sym;
}	/* save_ident */


/***************************************************************************
 * do_labels
 *
 * traverse the list of labels for the current instruction and set the
 * value field.
 */
do_labels(lasttime)
int lasttime;
{
  register symbol *lab;
  register symbol *lab2;
  register symbol *next;
  unsigned long    val;
  unsigned long    stype;
  int              internal = 0;

  val = labdot;
  stype = dot->stype;
  internal = labellist && labellist->internal;

#ifdef PIC
  if (shlib_flag && (!internal || lasttime)) {
     if ((stype == laststype) && (val == lastval) && 
         lastlabel && (labellist != 0)) {
        lastlabel->endoflist = 0;
        lastlabel->size = (long) labellist;
        if (lasttime) {
	   if (lastlabel == datalabel) {
	      datalabel = NULL;
	   }
	   else if (lastlabel == bsslabel) {
	      bsslabel = NULL;
	   }
	   goto lastchance;
        }
     }
     else {
        lastchance:
        if (datalabel && (((stype == SDATA) && (labellist != 0)) || lasttime)) {
           if (stype == SDATA) {
              datalabel->size = val - datalabel->svalue;
           }
           else {
              datalabel->size = dotdat - datalabel->svalue;
           }
        }
        if (bsslabel && (((stype == SBSS) && (labellist != 0)) || lasttime)) {
           if (stype == SBSS) {
              bsslabel->size = val - bsslabel->svalue;
           }
           else {
              bsslabel->size = dotbss - bsslabel->svalue;
           }
        }
     }
  }
#endif
  for(lab = labellist; lab != 0; lab = next) {
     lab->svalue = val;
     lab->stype |= stype;
     next = lab->slink;
     lab->slink = NULL;
#ifdef PIC
     if (shlib_flag && !internal) {
        if (next) {
	   lab->endoflist = 0;
	   lab->size = (long) next;
        }
        else {
	   lab->endoflist = 1;
	   lastlabel = lab;
           laststype = stype;
           lastval = val;
	   if (stype == SDATA) {
	      datalabel = lab;
           }
	   else if (stype == SBSS) {
	      bsslabel = lab;
           }
        }
     }
     else {
	lab->endoflist = 1;
	lab->size = 0;
     }
#endif
  }
  labellist = (symbol * ) NULL;
}	/* do_labels */


/*****************************************************************************
 * module_info
 *
 * Adds new module information
 *
 */

module_info(s)
char *s;
{
   int l = strlen(s);
   fwrite(s,l,1,fdmod);
   modsiz += l;
}

/*****************************************************************************
 * do_set_pseudo
 *
 * implement the set pseudo-op
 *	set	<sym>,<expr>
 * set the value and type of <sym> according to <expr>
 * The expression can be absolute or relocatable, but its type must be
 * known at pass1, i.e., no forward references.
 */
do_set_pseudo(sym, expr)
  symbol * sym;
  rexpr * expr;
{
  if (sym==NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	uerror("unable to define symbol");
  else if ((sym->stype&STYPE) != SUNDEF)
	uerror("illegal attempt to redefine symbol (%s)", sym->snamep);
  else if (defsym(*expr)) {
	sym->svalue = expr->expval.lngval + expr->symptr->svalue;
	sym->stype &= ~STYPE;
	sym->stype |= expr->symptr->stype&STYPE;
	if (sym->stype & STYPE == SABS) {
		sym->got = 0; /* in case one or more of these was set
		sym->plt = 0;  * by "get_reloc". since the symbol is
		sym->pc  = 0;  * absolute, no fixups will be needed */
	}
#ifdef PIC
	if (shlib_flag && (expr->expval.lngval == 0) &&
	    !(*expr->symptr->snamep == '.')) {
           sym->size =  expr->symptr->size;
           sym->endoflist =  expr->symptr->endoflist;
           expr->symptr->size = (long) sym;
           expr->symptr->endoflist = 0;
	   if (datalabel == expr->symptr) {
	      datalabel = sym;
	   }
	   else if (bsslabel == expr->symptr) {
	      bsslabel = sym;
	   }
	}
	else {
           sym->size =  0;
           sym->endoflist =  1;
	}
#else
        sym->size =  0;
        sym->endoflist =  1;
#endif
  }
  else if (expr->exptype != TYPL)
	uerror("illegal expression in set");
  else {
	sym->svalue = expr->expval.lngval;
	sym->stype &= ~STYPE;
	sym->stype |= SABS;
	sym->got = 0; /* in case one or more of these was set
	sym->plt = 0;  * by "get_reloc". since the symbol is
	sym->pc  = 0;  * absolute, no fixups will be needed */
	}
}	/* do_set_pseudo */

# if FUTURE_FEATURE
 /* conditionally remove for now */
/*****************************************************************************
 * do_fset_pseudo
 *
 * entry point for a future "fset" feature.
 * Not currently implemented.
 * Currently, there is not enough space in the "value" field of a symbol to
 * allow for a general float expression.
 */
do_fset_pseudo(sym, expr)
  symbol * sym;
  rexpr * expr;
{ if ( !(expr->exptype & TFLT) )
	uerror("illegal expression in fset");
  else if (sym==NULL)
	uerror("unable to define symbol (%s)", sym->snamep);
  else {
	uerror("fset not implemented");
	}
}	/* do_fset_pseudo */
# endif

/*****************************************************************************
 * check_index_scale
 *
 * make sure the integer represents a valid scale factor for a 
 * index register and map it to the 3-bit representation. */
check_index_scale(x) 
  int x;
{
  switch(x) {
	default:
		uerror("invalid xreg scale factor");
		return(0);

	case 1: return(0);
# ifdef M68020
	case 2: return(1);
	case 4: return(2);
	case 8: return(3);
# endif
	}
}	/* check_index_scale */
	

/****************************************************************************
 * do_global_ident
 *
 * Process the list of identfiers for a "global" pseudo-op.  Turn on
 * the appropriate flag (SEXTERN or SEXTERN2) bit(s) for each identifier on the list.
 */
do_global_ident(idlist, flag) 
  symbol * idlist;
  int flag;
{ register symbol * sym, *next;
  for (sym=idlist; sym != NULL; sym=next) {
	sym->stype |= flag;
	next = sym->slink;
	sym->slink = NULL;
	}
}	/* do_global_ident */
	

/****************************************************************************
 * do_internal_ident
 *
 * Process the list of identfiers for a "internal" pseudo-op. 
 */
do_internal_ident(idlist) 
  symbol * idlist;
{ 
  register symbol * sym, *next;

  for (sym=idlist; sym != NULL; sym=next) {
	sym->internal = 1;
	next = sym->slink;
	sym->slink = NULL;
	}
}	/* do_global_ident */


/*****************************************************************************
 * new_stmt_setup
 *
 * reset any vars that need to be reset before each statment
 */
new_stmt_setup()
{
  dot->svalue = newdot;
  labdot = newdot;
#if defined(LISTER1) || defined(LISTER2)
  listdot = newdot;
#endif
  
  /* This initialization is needed because in productions that use
   * "opt_fpinstr_size" if there is no size suffix, the float constant
   * can be read as the look ahead token before the fp_size value
   * can be set from the value of "opt_fpinstr_size".
   */
  fp_size = SZNULL;

  inode = 0;
  nerrors_this_stmt = 0;
}	/* new_stmt_setup */


/******************************************************************************
 * float_exptype
 *
 * convert from a size suffix code to an expression type for a floating
 * point expression.
 */
			
int  float_exptype(fpsize)
  int fpsize;
{
  switch(fpsize)  {
    default:
	uerror("illegal size for float expression");
	return (TYPNULL);
	break;
    case SZSINGLE:
	return (TYPF);
	break;
    case SZDOUBLE:
	return (TYPD);
	break;
    case SZEXTEND:
	return (TYPX);
	break;
    case SZPACKED:
	return (TYPP);
	break;
   }
}	/* float_exptype */ 


/*******************************************************************************
 * yyerror
 *	Error printing routine for parser.
 *
 */
 yyerror(s)
  char *s;
{
  uerror(s);
}
