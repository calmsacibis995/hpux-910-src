/* file opcodes.h */
/* SCCS opcodes.h    REV(64.2);       DATE(92/04/03        14:22:15) */
/* KLEENIX_ID @(#)opcodes.h	64.2 91/11/30 */

/* CAUTION: This file must be in sync with pccdefs (Fortran) and cgram.y (C). */

/* incoming defs:
 *	FORT		for all FORTRAN-related pieces (everything?)
 */


# define PLUS 1     /* 2,3 reserved for ASG and UNARY +    */
# define ICON 4     /* pccdefs refers to "4" directly for ICON */
# define FCON 5
# define NAME 6
# define STRING 7
# define MINUS 8    /* 9,10 reserved for ASG and UNARY -   */
# define MUL 11     /* 12,13 reserved for ASG and UNARY *  */
# define AND 14     /* 15,16 reserved for ASG and UNARY &  */
# define OR 17      /* 18 reserved for ASG |               */
# define ER 19      /* 20 reserved for ASG ^               */
# define QUEST 21
		/* pccdefs refers to "22" directly for COLON */
# define COLON 22
# define ANDAND 23
# define OROR 24

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
# define ASGNOP 25
     /* C1 internal use only -- Neither input nor output */
#    define FOREG ASGNOP

# define RELOP 26
# define EQUOP 27
# define DIVOP 28
# define SHIFTOP 29
# define INCOP 30
	/* C1 internal use only -- neither input nor output *
	/* a placeholder NODE with no code implied */
#	define NOCODEOP INCOP
# define UNOP 31
# define STROP 32

/*	reserved words, etc */
# define TYPE 33
     /* C1 internal use only -- Neither input nor output */
#   define SEMICOLONOP 33
#   define DBRA 34
# define CLASS 34
# define STRUCT 35
/* C1 -- UNARY SEMICOLONOP == 35 */
# define RETURN 36
# define SIZEOF 37
# define IF 38
# define ELSE 39
# define SWITCH 40
# define BREAK 41
# define CONTINUE 42
# define WHILE 43
# define DO 44
# define FOR 45
# define DEFAULT 46
#ifndef IRIF
#        define CBRANCH 47
#endif /* not IRIF */
# define GOTO 48 /* pccdefs refers to "48" directly for GOTO */
# define FREE 49


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

# define LC 50
# define RC 51
# define LB 52
# define RB 53
# define LP 54
# define RP 55
# ifdef FORT
#	define FCMGO RC
#		define FCOMPGOTO FCMGO
# endif /* FORT */
     /* C1 internal use only -- Neither input nor output */
#	define UCM RB
#	define EXITBRANCH LC

# define CM 56
# define SM 57
     /* C1 internal use only -- Neither input nor output */
#   define LGOTO SM

# define ASSIGN 58

#ifdef APEX
/* Domain #attribute and #option extensions */
# define ATTRIBUTE 59
# define OPTIONS 60
# define SHARP 61
#endif

/*	END OF YACC */

/*	left over tree building operators */
# define COMOP 59
# define DIV 60     /* 61 reserved for ASG /               */
# define MOD 62     /* 63 reserved for ASG %               */
# define LS 64      /* 65 reserved for ASG <<              */
# undef RS          /* S800 syscall.h defines RS */
# define RS 66      /* 67 reserved for ASG >>              */
# define DOT 68
# define STREF 69
# define CALL 70    /* 72 reserved for UNARY CALL          */
# define FENTRY 73
# define NOT 76
# define COMPL 77
# define INCR 78
# define DECR 79
# define EQ 80
# define NE 81
# define LE 82
# define LT 83
		/* pccdefs refers to "84" directly for GE */
# define GE 84
# define GT 85
# define ULE 86
# define ULT 87
# define UGE 88
# define UGT 89

# define SETBIT 90
# define TESTBIT 91
# define RESETBIT 92
# define ARS 93
		/* pccdefs refers to "94" directly for REG */
# define REG 94
# define OREG 95
# define CCODES 96
# define ENUM 97

#ifndef IRIF
#       define STASG 98
#       define STARG 99
#       define STCALL 100   /* 102 reserved for UNARY STCALL          */
#else /* IRIF */
#       define INDEX 98
#       define VA_START 99
#       define ASGOPCONV 100
#endif /* IRIF */

/*	some conversion operators */
# define FLD 103
# define SCONV 104
# define PCONV 105
# define PMCONV 106
# define PVCONV 107

# define FORCE 108
# define CASE 109
# define INIT 110
# define CAST 111

/* special opcodes for 68881 support. Seen only in pass 2 */
#       define FEQ	112
#       define FNEQ	113
#       define FGT	114
#       define FNGT	115
#       define FGE	116
#       define FNGE	117
#       define FLT	118
#       define FNLT	119
#       define FLE	120
#       define FNLE	121

# define FMONADIC 122
# define DIVP2	123	/* integer divide by power of 2; 
			 * 124 reserved for ASG DIVP2 */

/* used only for DOUBLE/FLOAT compares in local2.c */
/* C1 uses 125 exclusively for UNARY SEMICOLONOP's */
# define BICCODES 125

# define PAREN 126 /* () node to indicate grouping */
# define ELLIPSIS 127
# define WIDESTRING 128
# define QUAL 129
# define TYPE_DEF 130
# define NAMESTRING 131
# define PHEAD 132
# define ARGLIST 133
# define ARGASG 134
# define EQV 135  /* FORTRAN .EQV. operator */
# define NEQV 136  /* FORTRAN .NEQV. operator */

/* 
 *   code generator and C1 "manifest" expect ASM as the last interesting OP
 */

#ifdef IRIF 
 
/* even number opcodes below (138-148) reserved for 'ASG op' */

#       define ADDICON  137   /* addition between integer variable & constant */
#       define SUBICON  139   /* subtraction between integer variable & constant */
#       define PADDICON 141   /* addition between pointer & integer constant */
#       define PSUBICON 143   /* subtraction between pointer & integer constant */
#       define PADDI    145   /* addition between pointer & integer variable */
#       define PSUBI    147   /* subtraction between pointer & integer variable */
#       define PSUBP    149   /* subtraction between two pointers */

# define ASM 150 /* constant for lex/yacc interface for "asm" statements */

/* opcodes 151-153 are internal c0 opcodes -- never seen outside c0 */
# define FC0CALL 151
# define FC0CALLC 152
# define FC0OREG 153

#else /* NOT IRIF */

# define ASM 137 /* constant for lex/yacc interface for "asm" statements */

/* opcodes 138-140 are internal c0 opcodes -- never seen outside c0 */
# define FC0CALL 138
# define FC0CALLC 139
# define FC0OREG 140

#endif /* not IRIF */

/* FC0OREG is expected to be the last interesting tree opcode for 'C0' */

/* SEE NOTE BELOW BEFORE ADDING ANY NEW OPCODES */

/*	new opcode definitions */

#	define FORTOPS 200
#	define FTEXT 200
#	define FEXPR 201
#	define FSWITCH 202
#	define FLBRAC 203
#	define FRBRAC 204
#	define FEOF 205
#	define FARIF 206
#	define LABEL 207
#	define SETREGS 208

#	define ARRAYREF 209

#	define FMAXLAB 211


#	define VAREXPRFMTDEF 212
#	define VAREXPRFMTEND 213
#	define VAREXPRFMTREF 214
#	define FICONLAB 215
#	define C1SYMTAB 216
#	define C1OPTIONS 217
#	define C1OREG 218
#	define C1NAME 219
#	define STRUCTREF 220
#	define C1HIDDENVARS 221
#	define C1HVOREG	222
#	define C1HVNAME 223
#	define SWTCH 224
#	define C1881CODEGEN 225
#	define NOEFFECTS 226
#	define C0TEMPASG 227
/* The following opcode is only used in the     */
/* generation of Apollo IL from pcc. (eval_pcc) */
#ifdef HAIL
#	define HAILCTEMP 228
#endif

/* 	***************************************************
 *		IMPORTANT NOTE for ADDING OPCODES
 * 	***************************************************
 *	Be sure that DSIZE is always set to the highest value
 *	opcode that can appear in an expression tree. This is
 *	important insofar as the 'dope', 'opst', and '*opptr'
 *	array sizes depend on this, and defect causing memory 
 *	writes can "spill" off the end of these arrays otherwise.
 *	ALSO, note that newly added tree nodes also require an
 *	entry in the dope array, to ensure proper 'optype()'
 *	recognition of LTYPE, BITYPE, or UTYPE (e.g. 'fwalk()')
 *	See files 'common' and 'match.c' for the mentioned objects.
 */


/* FC0OREG is expected to be the last interesting tree opcode for 'C0' */

# define ASG 1+
# define UNARY 2+
# define NOASG (-1)+
# define NOUNARY (-2)+

	/* DSIZE is the size of the dope array */

# ifdef C0
# 	define DSIZE (FC0OREG + 1)
# else /* C0 */
# ifdef IRIF
#       define DSIZE (PSUBP + 1 )
# else /* not IRIF */
# 	define DSIZE (ASM + 1)
# endif /* not IRIF */
# endif /* C0 */


