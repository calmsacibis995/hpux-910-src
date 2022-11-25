/* @(#) $Revision: 70.2 $ */      
# include	"messages.h"
# include	"lerror.h"

/* msginfo keeps auxiliary lint information about messages.
 * This data needs to be kept consistent with messages.c
 * msginfo is an array of struct msginfo containing:
 *	- buffering information (0=not buffered, otherwise which section)
 *	- argument information
 *	- message class information
 */
struct msginfo msginfo[ NUMMSGS ] = {
	/* [0] "%.8s evaluation order undefined" */
	{ 0,STRINGTY,WEORDER },

	/* [1] "%.8s may be used before set" */
	{ 0,STRINGTY,WHEURISTIC },

	/* [2] "%.8s redefinition hides earlier one" */
	{ 0,STRINGTY,WHEURISTIC },

	/* [3] "%.8s set but not used in function %.8s" */
	{ 0,DBLSTRTY,WUSAGE },

	/* [4] "%.8s undefined" */
	{ 0,STRINGTY,WALWAYS },

	/* [5] "bad structure offset", */
	{ 0,PLAINTY,WALWAYS },

	/* [6] "%.8s unused in function %.8s" */
	{ 0,DBLSTRTY,WUDECLARE },

	/* [7] "& before array or function: ignored" */
	{ 0,PLAINTY,WALWAYS },

	/* [8] "=<%c illegal" */
	{ 0,CHARTY,WALWAYS },

	/* [9] "=>%c illegal" */
	{ 0,CHARTY,WALWAYS },

	/* [10] "source file is empty" */
	{ 0,PLAINTY,WANSI },

	/* [11] "a function is declared as an argument" */
	{ 0,PLAINTY,WALWAYS },

	/* [12] "ambiguous assignment: assignment op taken" */
	{ 0,PLAINTY,WPORTABLE },

	/* [13] "argument %.8s unused in function %.8s" */
	{ 1,DBLSTRTY,WUDECLARE },

	/* [14] "array of functions is illegal" */
	{ 0,PLAINTY,WALWAYS },

	/* [15] "assignment of different structures" */
	{ 12,PLAINTY,WALWAYS },

	/* [16] "bad asm construction" */
	{ 0,PLAINTY,WALWAYS },

	/* [17] "error in floating point constant to %sinteger constant convers" */
	{ 0,STRINGTY,WALWAYS },

	/* [18] "should not take '&' of %s" */
	{ 18,STRINGTY|SIMPL,WALWAYS },

	/* [19] "identifiers with %s should not be initialized" */
	{ 0,STRINGTY,WALWAYS },

	/* [20] "case not in switch" */
	{ 0,PLAINTY,WALWAYS },

	/* [21] "comparison of unsigned with negative constant" */
	{ 0,PLAINTY,WALWAYS },

	/* [22] "constant argument to NOT" */
	{ 0,PLAINTY,WCONSTANT|WHEURISTIC },

	/* [23] "constant expected" */
	{ 0,PLAINTY,WALWAYS },

	/* [24] "constant in conditional context" */
	{ 0,PLAINTY,WCONSTANT|WHEURISTIC },

	/* [25] "incorrect initialization or too many initializers" */
	{ 0,PLAINTY,WALWAYS },

	/* [26] "conversion from 'long' may lose accuracy" */
	{ 2,PLAINTY,WLONGASSIGN },

	/* [27] "conversion to 'long' may sign-extend incorrectly" */
	{ 3,PLAINTY,WLONGASSIGN },

	/* [28] "declared argument %.8s is missing" */
	{ 0,STRINGTY,WALWAYS },

	/* [29] "default not inside switch" */
	{ 0,PLAINTY,WALWAYS },

	/* [30] "degenerate unsigned comparison" */
	{ 0,PLAINTY,WUCOMPARE },

	/* [31] "division by 0" */
	{ 0,PLAINTY,WALWAYS },

	/* [32] "division by 0." */
	{ 0,PLAINTY,WALWAYS },

	/* [33] "duplicate case in switch, %d" */
	{ 0,NUMTY,WALWAYS },

	/* [34] "duplicate default in switch" */
	{ 0,PLAINTY,WALWAYS },

	/* [35] "array declaration%s%s%s should not be empty" */
	{ 0,TRIPLESTR,WALWAYS },

	/* [36] "empty character constant" */
	{ 0,PLAINTY,WALWAYS },

	/* [37] "enumeration type clash, operator %s" */
	{ 5,STRINGTY,WHEURISTIC|WDECLARE },

	/* [38] "field outside of structure" */
	{ 0,STRINGTY,WALWAYS },

	/* [39] "field too big" */
	{ 0,PLAINTY,WSTORAGE },

	/* [40] "struct/union containing const-qualified members should not" */
	{ 0,PLAINTY,WALWAYS },

	/* [41] "function designators or pointers to functions should not be operands of '%s'" */
	{ 0,STRINGTY,WALWAYS },

	/* [42] "block scope identifiers with 'extern' storage class should not be initialized" */
	{ 0,PLAINTY,WALWAYS },

	/* [43] "function %.8s has return(e); and return;" */
	{ 0,STRINGTY,WRETURN },

	/* [44] "'%s' declared as function returning %s" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [45] "function has illegal storage class" */
	{ 0,PLAINTY,WALWAYS },

	/* [46] "function illegal in structure or union" */
	{ 0,PLAINTY,WALWAYS },

	/* [47] "function returns illegal type" */
	{ 0,STRINGTY,WALWAYS },

	/* [48] "left operand of '?' should have scalar type" */
	{ 0,PLAINTY,WALWAYS },

	/* [49] "function parameter types in call and definition are incomp" */
	{ 0,STRINGTY,WDECLARE },

	/* [50] "illegal break" */
	{ 0,PLAINTY,WALWAYS },

	/* [51] "incorrect input character detected, hex value 0x%x" */
	{ 0,HEXTY,WALWAYS },

	/* [52] "illegal class" */
	{ 0,PLAINTY,WALWAYS },

	/* [53] "illegal combination of pointer and integer, op %s" */
	{ 8,STRINGTY,WALWAYS },

	/* [54] "illegal comparison of enums" */
	{ 6,PLAINTY,WALWAYS },

	/* [55] "illegal continue" */
	{ 0,PLAINTY,WALWAYS },

	/* [56] "illegal field size" */
	{ 0,NUMTY,WALWAYS },

	/* [57] "illegal field type" */
	{ 0,PLAINTY,WALWAYS },

	/* [58] "function designator/pointer expected%s%s%s" */
	{ 0,TRIPLESTR,WALWAYS },

	/* [59] "illegal hex constant" */
	{ 0,PLAINTY,WALWAYS },

	/* [60] "illegal indirection" */
	{ 0,PLAINTY,WALWAYS },

	/* [61] "illegal initialization" */
	{ 0,PLAINTY,WALWAYS },

	/* [62] "illegal lhs of assignment operator" */
	{ 0,STRINGTY,WALWAYS },

	/* [63] "illegal member use: %.8s" */
	{ 14,STRINGTY|SIMPL,WDECLARE|WHEURISTIC },

	/* [64] "bitfield size of '%d' for '%s' is out of range" */
	{ 0,NUMTY|STR2TY,WALWAYS },

	/* [65] "illegal member use: perhaps %.8s.%.8s" */
	{ 16,DBLSTRTY,WDECLARE|WHEURISTIC },

	/* [66] "illegal pointer combination" */
	{ 7,PLAINTY,WALWAYS },

	/* [67] "illegal pointer subtraction" */
	{ 0,PLAINTY,WALWAYS },

	/* [68] "illegal register declaration" */
	{ 0,PLAINTY,WALWAYS },

	/* [69] "function parameter %s types in call and definition are incompatible" */
	{ 0,STRINGTY,WALWAYS },

	/* [70] "invalid multibyte character detected starting at '%c' (hex value 0x%x)" */
	{ 0,CHARTY|HEX2TY,WALWAYS },

	/* [71] "pointer to object of unknown size used in context where size must be known" */
	{ 0,PLAINTY,WALWAYS },

	/* [72] "type too complex" */
	{ 0,PLAINTY,WALWAYS },

	/* [73] "no storage class, type specifier or type qualifier specified - 'int' assumed" */
	{ 0,PLAINTY,WALWAYS },

	/* [74] "illegal {" */
	{ 0,PLAINTY,WALWAYS },

	/* [75] "loop not entered at top" */
	{ 0,PLAINTY,WALWAYS },

	/* [76] "unnamed bitfields should not be initialized" */
	{ 17,PLAINTY,WALWAYS },

	/* [77] "function designator/pointer used in an incorrect context" */
	{ 0,PLAINTY,WALWAYS },

	/* [78] "newline in string or char constant" */
	{ 0,PLAINTY,WALWAYS },

	/* [79] "no automatic aggregate initialization" */
	{ 0,PLAINTY,WKNR },

	/* [80] "non-constant case expression" */
	{ 0,PLAINTY,WALWAYS },

	/* [81] "non-null byte ignored in string initializer" */
	{ 0,PLAINTY,WALWAYS },

	/* [82] "nonportable character comparison" */
	{ 0,PLAINTY,WPORTABLE },

	/* [83] "the only portable field type is unsigned int" */
	{ 0,PLAINTY,WPORTABLE },

	/* [84] "nonunique name demands struct/union or struct/union pointer" */
	{ 13,PLAINTY,WALWAYS },

	/* [85] "null dimension" */
	{ 0,PLAINTY,WALWAYS },

	/* [86] "null effect" */
	{ 20,PLAINTY,WNULLEFF },

	/* [87] "old-fashioned assignment operator" */
	{ 0,PLAINTY,WOBSOLETE },

	/* [88] "old-fashioned initialization: use =" */
	{ 0,PLAINTY,WOBSOLETE },

	/* [89] "operands of %s have incompatible types" */
	{ 0,STRINGTY,WALWAYS },

	/* [90] "side effects incurred by operand of 'sizeof' are ignored" */
	{ 0,PLAINTY,WALWAYS },

	/* [91] "possible pointer alignment problem" */
	{ 9,PLAINTY,WHEURISTIC|WPORTABLE|WALIGN },

	/* [92] "precedence confusion possible: parenthesize!" */
	{ 0,PLAINTY,WEORDER|WHEURISTIC },

	/* [93] "precision lost in assignment to (possibly sign-extended) field" */
	{ 0,PLAINTY,WALWAYS },

	/* [94] "precision lost in field assignment" */
	{ 0,PLAINTY,WALWAYS },

	/* [95] "questionable conversion of function pointer" */
	{ 0,PLAINTY,WHEURISTIC },

	/* [96] "redeclaration of %.8s" */
	{ 0,STRINGTY,WDECLARE },

	/* [97] "redeclaration of formal parameter, %.8s" */
	{ 0,STRINGTY,WDECLARE },

	/* [98] "pointer casts may be troublesome" */
	{ 22,PLAINTY,WPORTABLE },

	/* [99] "sizeof returns 0" */
	{ 0,PLAINTY,WALWAYS },

	/* [100] "statement not reached" */
	{ 21,PLAINTY,WREACHED },

	/* [101] "static variable %.8s unused" */
	{ 0,STRINGTY,WUDECLARE },

	/* [102] "struct/union %.8s never defined" */
	{ 0,STRINGTY,WDECLARE|WHEURISTIC },

	/* [103] "incorrect input character detected" */
	{ 0,PLAINTY,WALWAYS },

	/* [104] "structure %.8s never defined" */
	{ 0,STRINGTY,WDECLARE|WHEURISTIC },

	/* [105] "structure reference must be addressable" */
	{ 19,PLAINTY,WALWAYS },

	/* [106] "structure typed union member must be named" */
	{ 0,PLAINTY,WALWAYS },

	/* [107] "too many characters in character constant" */
	{ 0,PLAINTY,WSTORAGE },

	/* [108] "too many initializers" */
	{ 0,TRIPLESTR,WALWAYS },

	/* [109] "linkage conflict with prior declaration for '%s'; %s linkage assumed" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [110] "unacceptable operand of &" */
	{ 0,PLAINTY,WALWAYS },

	/* [111] "undeclared initializer name %.8s" */
	{ 0,STRINGTY,WALWAYS },

	/* [112] "address/value of right-hand side of assignment should be known at compile time" */
	{ 0,PLAINTY,WALWAYS },

	/* [113] "unexpected EOF" */
	{ 0,PLAINTY,WALWAYS },

	/* [114] "unknown size" */
	{ 0,PLAINTY,WALWAYS },

	/* [115] "unsigned comparison with 0?" */
	{ 0,PLAINTY,WALWAYS },

	/* [116] "void function %.8s cannot return value" */
	{ 0,STRINGTY,WRETURN },

	/* [117] "void type for %.8s" */
	{ 0,STRINGTY,WALWAYS },

	/* [118] "void type illegal in expression" */
	{ 0,PLAINTY,WALWAYS },

	/* [119] "zero or negative subscript" */
	{ 0,PLAINTY,WALWAYS },

	/* [120] "zero size field" */
	{ 0,PLAINTY,WALWAYS },

	/* [121] "zero sized structure" */
	{ 0,PLAINTY,WALWAYS },

	/* [122] "} expected" */
	{ 0,PLAINTY,WALWAYS },

	/* [123] "long in case or switch statement may be truncated" */
	{ 23,PLAINTY,WPORTABLE },

	/* [124] "bad octal digit %c" */
	{ 0,CHARTY,WALWAYS },

	/* [125] "floating point constant folding causes exception" */
	{ 0,PLAINTY,WALWAYS },

	/* [126] "invalid null dimension used in declaration of '%s'" */
	{ 0,STRINGTY,WALWAYS },

	/* [127] "main() returns random value to invocation environment" */
	{ 0,PLAINTY,WRETURN },

	/* [128] "`%s' may be indistinguishable from `%s' due to internal name truncation" */
	{ 0,DBLSTRTY,WPORTABLE },

	/* [129] "%s should not be arguments to sizeof()" */
	{ 0,STRINGTY,WALWAYS },

	/* [130] "pointer to VOID inappropriate" */
	{ 0,PLAINTY,WALWAYS },

	/* [131] "const object, may not be incremented, decremented or assigned" */
	{ 0,PLAINTY,WALWAYS },

	/* [132] "" */
	{ 0,NUMTY,WALWAYS },

	/* [133] "storage class not allowed in struct/union" */
	{ 0,PLAINTY,WALWAYS },

	/* [134] "(NLS) illegal second byte in 16-bit character" */
	{ 0,PLAINTY,WALWAYS },

	/* [135] "asm code may be wrong with -O, try +O1 or #pragma OPT_LEVEL 1" */
	{ 0,PLAINTY,WALWAYS },

	/* [136] "language '%s' not available - processing continues using language 'n-computer'" */
	{ 0,STRINGTY,WALWAYS },

	/* [137] "empty declaration" */
	{ 0,PLAINTY,WANSI },

	/* [138] "%s constant too large to represent; high order bytes will be lost" */
	{ 0,STRINGTY,WSTORAGE },

	/* [139] "empty hex escape sequence" */
	{ 0,PLAINTY,WALWAYS },

	/* [140] "floating point constant %s too %s for represention; information will be lost" */
	{ 0,PLAINTY,WSTORAGE },

	/* [141] "enum constant overflow: %s given value INT_MIN" */
	{ 0,STRINGTY,WSTORAGE },

	/* [142] "void pointer should be NULL pointer constant when used with %s and pointer to function/function designator" */
	{ 0,STRINGTY,WALWAYS },

	/* [143] "void pointer should not be on lhs of %s when rhs is pointer to function/function designator" */
	{ 0,STRINGTY,WALWAYS },

	/* [144] "void pointer should be NULL pointer constant when used on the rhs with %s and pointer to function" */
	{ 0,STRINGTY,WALWAYS },

	/* [145] "qualifiers are not assignment-compatible" */
	{ 0,PLAINTY,WALWAYS },

	/* [146] "types pointed to by %s operands must be qualified or unqualified versions of compatible types" */
	{ 0,STRINGTY,WALWAYS },

	/* [147] "" */
	{ 0,PLAINTY,WALWAYS },

	/* [148] "use of typedef precludes use of additional type specifiers " */
	{ 0,PLAINTY,WALWAYS },

	/* [149] "function declarations for %s have incompatible return types" */
	{ 0,STRINGTY,WDECLARE },

	/* [150] "function declarations for %s have incompatible number of parameters" */
	{ 0,STRINGTY,WDECLARE },

	/* [151] "function declarations for %s have incompatible use of ELLIPSIS" */
	{ 0,STRINGTY,WDECLARE },

	/* [152] "function declarations for %s have incompatible parameters" */
	{ 0,STRINGTY,WDECLARE },

	/* [153] "function prototype for %s and old style definition have incompatible number of parameters" */
	{ 0,STRINGTY,WDECLARE|WANSI },

	/* [154] "function prototype for %s may not use ELLIPSIS when used with old style declaration" */
	{ 0,STRINGTY,WDECLARE|WANSI },

	/* [155] "function prototype for %s and old style definition have incompatible parameters" */
	{ 0,STRINGTY,WDECLARE|WANSI },

	/* [156] "function prototype for %s may not use ELLIPSIS when used with an empty declaration" */
	{ 0,STRINGTY,WDECLARE|WANSI },

	/* [157] "function prototype for %s must contain parameters compatible with default argument promotions, when used with an empty declaration" */
	{ 0,STRINGTY,WDECLARE|WANSI },

	/* [158] "incompatible array declarations for %s" */
	{ 0,STRINGTY,WDECLARE },

	/* [159] "incompatible pointer declarations for %s" */
	{ 0,STRINGTY,WDECLARE },

	/* [160] "incompatible qualifier declarations for %s" */
	{ 0,STRINGTY,WDECLARE },

	/* [161] "incompatible declaration for %s" */
	{ 0,STRINGTY,WDECLARE },

	/* [162] "LANG environment variable is not set" */
	{ 0,PLAINTY,WALWAYS },

	/* [163] "pointer and %s are type incompatible, op %s" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [164] "operands of %s should have %s type" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [165] "%s keyword should be followed by identifier or declaration list" */
	{ 0,STRINGTY,WALWAYS },

	/* [166] "type-name of CAST should be scalar" */
	{ 0,PLAINTY,WALWAYS },

	/* [167] "%sstruct/union does not contain member: %s" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [168] "controlling expression of %s should have %s type" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [169] "local redeclaration of typedef as declarator: %s" */
	{ 0,STRINGTY,WDECLARE },

	/* [170] "array/array subscript type error" */
	{ 0,PLAINTY,WALWAYS },

	/* [171] "%s types incompatible, op %s" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [172] "multiple %s in declaration" */
	{ 0,STRINGTY,WALWAYS },

	/* [173] "should not subscript an array which has no lvalue" */
	{ 0,PLAINTY,WALWAYS },

	/* [174] "" */
	{ 0,PLAINTY,WALWAYS },

	/* [175] "excess values in initializer ignored" */
	{ 0,PLAINTY,WALWAYS },

	/* [176] "double to float conversion exception" */
	{ 0,PLAINTY,WALWAYS },

	/* [177] "types of CAST operands must be scalar unless first operand is VOID" */
	{ 0,PLAINTY,WALWAYS },

	/* [178] "static identifier used but not defined: %s" */
	{ 0,STRINGTY,WUSAGE },

	/* [179] "declaration list of %s should be nonempty" */
	{ 0,STRINGTY,WALWAYS },

	/* [180] "struct/union %s required before %s" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [181] "missing \";\" assumed to indicate end of declaration list" */
	{ 0,PLAINTY,WANSI },

	/* [182] "\",\" at end of declaration list ignored" */
	{ 0,PLAINTY,WANSI },

	/* [183] "unknown size: %s" */
	{ 0,STRINGTY,WALWAYS },

	/* [184] "zero sized field: %s" */
	{ 0,STRINGTY,WALWAYS },

	/* [185] "zero sized struct/union" */
	{ 0,PLAINTY,WALWAYS },

	/* [186] "too many bytes in multibyte character constant: limit is %d" */
	{ 0,PLAINTY,WSTORAGE },

	/* [187] "syntax error in floating point constant, after %s" */
	{ 0,PLAINTY,WALWAYS },

	/* [188] "prototypes and old style parameter declarations mixed" */
	{ 0,PLAINTY,WANSI|WKNR },

	/* [189] "incorrect mix of names and parameter types in parameter list" */
	{ 0,PLAINTY,WANSI },

	/* [190] "(old-style) parameter name list only legal in function definition" */
	{ 0,PLAINTY,WANSI },

	/* [191] "unexpected ';'" */
	{ 0,PLAINTY,WANSI },

	/* [192] "ELLIPSIS used with old style parameter name list" */
	{ 0,PLAINTY,WANSI },

	/* [193] "incorrect use of 'void' in parameter list" */
	{ 0,PLAINTY,WANSI },

	/* [194] "abstract declarator incorrect in function definition" */
	{ 0,PLAINTY,WANSI },

	/* [195] "too many arguments for \"%s\"" */
	{ 0,STRINGTY,WALWAYS },

	/* [196] "integer overflow in constant expression" */
	{ 0,PLAINTY,WSTORAGE },

	/* [197] "not enough room for null terminator in string initializer" */
	{ 0,PLAINTY,WSTORAGE },

	/* [198] "too few arguments for \"%s\"" */
	{ 0,STRINGTY,WALWAYS },

	/* [199] least one fixed parameter is required" */
	{ 0,PLAINTY,WALWAYS },

	/* [200] "reinitialization of %s" */
	{ 0,STRINGTY,WALWAYS },

	/* [201] "" */
	{ 0,PLAINTY,WSTORAGE },

	/* [202] "wide characters supported in ANSI mode only" */
	{ 0,PLAINTY,WALWAYS },

	/* [203] "use of an incomplete type: %s" */
	{ 0,STRINGTY,WUSAGE },

	/* [204] "" */
	{ 0,PLAINTY,WALWAYS },

	/* [205] "undefined escape character: \\%c treated as %s" */
	{ 0,CHARTY|STR2TY,WALWAYS },

	/* [206] "undefined escape character: \\%c treated as %c" */
	{ 0,CHARTY|CHAR2TY,WALWAYS },

	/* [207] "hex escape sequence contains more than two digits" */
	{ 0,PLAINTY,WALWAYS },

	/* [208] "long double constants only supported for ANSI C" */
	{ 0,PLAINTY,WANSI },

	/* [209] "empty external declaration, \";\" ignored" */
	{ 0,PLAINTY,WANSI },

	/* [210] "long float type is not supported by ANSI C" */
	{ 0,PLAINTY,WKNR },

	/* [211] "long double type only supported by ANSI C" */
	{ 0,PLAINTY,WANSI },

	/* [212] "unnamed field should be within a struct/union" */
	{ 0,PLAINTY,WALWAYS },

	/* [213] "unknown directive '#%s'" */
	{ 0,STRINGTY,WALWAYS },

	/* [214] "invalid _HP_SECONDARY_DEF" */
	{ 0,PLAINTY,WALWAYS },

	/* [215] "unrecognized compiler directive ignored: %s" */
	{ 0,STRINGTY,WALWAYS },

	/* [216] "ON or OFF must follow #pragma OPTIMIZE" */
	{ 0,PLAINTY,WALWAYS },

	/* [217] "invalid OPT_LEVEL '%c' - level 1 assumed" */
	{ 0,PLAINTY,WALWAYS },

	/* [218] "pragma cannot appear inside a function or block" */
	{ 0,PLAINTY,WALWAYS },

	/* [219] "small table size of '%d' specified for '%c' ignored" */
	{ 0,NUMTY|CHAR2TY,WALWAYS },

	/* [220] "unrecognized option '%s%s' ignored" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [221] "error in floating point constant" */
	{ 0,PLAINTY,WALWAYS },

	/* [222] "redeclaration of '%s' hides formal parameter" */
	{ 0,STRINGTY,WDECLARE|WHEURISTIC },

	/* [223] "questionably signed expression - ANSI/non-ANSI promotion rules\n\tdiffer for 'unsigned char' and 'unsigned short'" */
	{ 0,PLAINTY,WHEURISTIC|WANSI },

	/* [224] "declare the VARARGS arguments you want checked!" */
	{ 0,PLAINTY,WALWAYS },

	/* [225] "constant too large for field" */
	{ 0,PLAINTY,WSTORAGE },

	/* [226] "types in call and definition of '%s' are incompatible (pointer and %s) for parameter '%s'" */
	{ 0,TRIPLESTR,WALWAYS },

	/* [227]  "types in call and definition of '%s' have incompatible %s types for parameter '%s'" */
	{ 0,TRIPLESTR,WALWAYS },

	/* [228]  "types in call and definition of '%s' should be qualified or unqualified versions of compatible types for parameter '%s'" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [229]  "types in call and definition of '%s' are incompatible for parameter '%s'" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [230]  "enumeration constant '%s' is outside range representable by bitfield '%s'" */
	{ 0,DBLSTRTY,WSTORAGE },

	/* [231]  "%s '%s' declared in function parameter list will have scope limited to this function declaration or definition" */
	{ 0,DBLSTRTY,WDECLARE },

	/* [232]  "untagged %s declared in function parameter list will have scope limited to this function declaration or definition" */
	{ 0,STRINGTY,WDECLARE },

	/* [233] "null statement following 'if'" */
	{ 0,PLAINTY,WNULLEFF },

	/* [234] "null statement following 'else'" */
	{ 0,PLAINTY,WNULLEFF },

	/* [235] "%sstruct/union '%s' does not contain member '%s'" */
	{ 0,DBLSTRTY,WALWAYS },

	/* [236] "function prototype not visible at point of call" */
	{ 10,STRINGTY,WPROTO|WKNR },

	/* [237] "trigraph sequence '??%c' will be converted to '%c' in ANSI mode" */
	{ 0,CHARTY|CHAR2TY,WANSI },

	/* [238] "token length exceeds 255 characters, truncated" */
	{ 0,PLAINTY,WALWAYS },

	/* [239] "alignment of struct '%s' may not be portable" */
	{ 0,STRINGTY,WALIGN|WHEURISTIC|WPORTABLE },

	/* [240] "trailing padding of struct/union '%s' may not be portable" */
	{ 0,STRINGTY,WALIGN|WHEURISTIC|WPORTABLE },

	/* [241] "number of arguments in call does not agree with function definition" */
	{ 0,PLAINTY,WHEURISTIC},

	/* [242] "unsupported __attributes or __options" */
	{ 0,PLAINTY,WALWAYS},

	/* [243] "reference parameters are not supported" */
	{ 0,PLAINTY,WALWAYS},

	/* [244] "old-style function declaration for '%s' in LINTSTDLIB" */
	{ 0,STRINGTY,WHEURISTIC}

};


char		*outmsg[ NUMBUF ] = {
/* [0] */	"",
/* [1] */	"argument unused in function:",
/* [2] */	"conversion from 'long' may lose accuracy",
/* [3] */	"conversion to 'long' may sign-extend incorrectly",
/* [4] */	"illegal array size combination",
/* [5] */	"enumeration type clash:",
/* [6] */	"incorrect comparison of enums",
/* [7] */	"incorrect pointer combination",
/* [8] */	"incorrect combination of pointer and integer:",
/* [9] */	"possible pointer alignment problem",
/* [10] */	"function prototype not visible at point of call:",
/* [11] */	"",
/* [12] */	"assignment of different structures",
/* [13] */	"nonunique name demands struct/union or struct/union pointer",
/* [14] */	"incorrect member use:",
/* [15] */	"",
/* [16] */	"incorrect member use:",
/* [17] */	"unnamed bitfields should not be initialized",
/* [18] */	"should not take '&' of:",
/* [19] */	"structure reference must be addressable",
/* [20] */	"null effect",
/* [21] */	"statement not reached",
/* [22] */	"pointer casts may be troublesome",
/* [23] */	"'long' in 'case' label or 'switch' statement may be truncated",
};

	char		*outformat[ NUMBUF ] = {
	/* [0] */	"",
	/* [1] */	"'%s' in '%s'",
	/* [2] */	"",
	/* [3] */	"",
	/* [4] */	"",
	/* [5] */	"operator '%s'",
	/* [6] */	"",
	/* [7] */	"",
	/* [8] */	"operator '%s'",
	/* [9] */	"",
	/* [10] */	"%s",
	/* [11] */	"",
	/* [12] */	"",
	/* [13] */	"",
	/* [14] */	"%s",
	/* [15] */	"%s",
	/* [16] */	"perhaps %s.%s",
	/* [17] */	"",
	/* [18] */	"%s",
	/* [19] */	"",
	/* [20] */	"",
	/* [21] */	"",
	/* [22] */	"",
	/* [23] */	"",
	};

int warnmask = WALLMSGS;
