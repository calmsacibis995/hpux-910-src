/* SCCS messages.c   REV(64.2);       DATE(92/04/03        14:22:12) */
/* KLEENIX_ID @(#)messages.c	64.2 91/11/30 */
/* file messages.c */
#include	"messages.h"

/* msgtext is an array of pointers to printf format strings used in calls to
 * error message printing functions
 * the index into msgtext of each string is "hardwired" hence when adding new
 * messages they should be appended to the end of msgtext and when deleting
 * messages, they should be replaced by empty strings
 *
 * NUMMSGS is defined to be the number of entries is msgtext and it too is a
 * hardwired constant, defined in messages.h
 */

char	*msgtext[ NUMMSGS ] = {
/* [0] */	"%s evaluation order undefined",	/* werror::name */
/* [1] */	"'%s' may be used before set",	/* werror::name */
/* [2] */	"'%s' redefinition hides earlier one",	/* uerror::name */
/* [3] */	"'%s' set but not used in function '%s'", /* werror::name,name */
/* [4] */	"'%s' undefined",	/* uerror::name */
/* [5] */	"bad structure offset",	/* uerror */
/* [6] */	"'%s' unused in function '%s'",	/* werror::name,name */
/* [7] */	"address operator '&' before an array or function is ignored",	/* werror */
/* [8] */	"=<%c incorrect",	/* uerror::character */
/* [9] */	"=>%c incorrect",	/* uerror::character */
/* [10] */	"source file is empty",
/* [11] */	"function is declared as an argument",	/* werror */
/* [12] */	"ambiguous assignment: simple assign, unary op assumed",	/* werror */
/* [13] */	"argument '%s' unused in function '%s'",	/* werror::name,name */
/* [14] */	"'array of functions' is an incorrect type",	/* uerror */
/* [15] */	"assignment of different structures",	/* uerror */
/* [16] */	"incorrect use of 'asm'",	/* uerror */
/* [17] */	"error in floating point constant to %sinteger constant conversion", /* uerror */
/* [18] */	"should not take '&' of %s",	/* uerror::name */
/* [19] */	"identifiers with %s should not be initialized ",	/* uerror */
/* [20] */	"'case' label should be inside a 'switch' statement",	/* uerror */
/* [21] */	"comparison of unsigned with negative constant",	/* werror */
/* [22] */	"constant argument to NOT",	/* werror */
/* [23] */	"integral constant expression expected",	/* uerror */
/* [24] */	"constant in conditional context",	/* werror */
/* [25] */      "incorrect initialization or too many initializers", /* uerror */
/* [26] */	"conversion from longer type may lose accuracy",	/* werror */
/* [27] */	"conversion to longer type may sign-extend incorrectly",	/* werror */
/* [28] */	"declared argument '%s' does not appear in argument list",	/* uerror::name */
/* [29] */	"'default' label should be inside a 'switch' statement",	/* uerror */
/* [30] */	"degenerate unsigned comparison",	/* werror */
/* [31] */	"division by '0' detected",	/* uerror */
/* [32] */	"division by '0.0' detected",	/* uerror */
/* [33] */	"'case %d' should appear at most once in 'switch' statement",	/* uerror::number */
/* [34] */	"'default' should appear at most once in 'switch' statement",	/* uerror */
/* [35] */	"array declaration%s%s%s should not be empty",	/* uerror */
/* [36] */	"character constants should be nonempty",	/* uerror */
/* [37] */	"enumeration type clash for operator '%s'",	/* werror::operator */
/* [38] */	"bitfield '%s' should be within a struct/union",	/* uerror */
/* [39] */	"bitfield too big",	/* uerror */
/* [40] */	"struct/union containing const-qualified members should not be assigned to",	/* uerror */
/* [41] */	"function designators or pointers to functions should not be operands of '%s'",	/* werror,uerror */
/* [42] */	"block scope identifiers with 'extern' storage class should not be initialized ",	/* uerror */
/* [43] */	"function '%s' has both 'return(expression)' and 'return'",	/* werror::name */
/* [44] */	"'%s' declared as function returning %s", /* uerror */
/* [45] */	"function storage class incorrect; 'extern' assumed",	/* werror */
/* [46] */	"function declaration/definition incorrect in struct/union",	/* uerror */
/* [47] */	"incorrect type detected, 'function returning %s'",	/* uerror */
/* [48] */	"left operand of '?' should have scalar type",	/* uerror */
/* [49] */	"function parameter types in call and definition are incompatible (pointer and %s)", /* werror, uerror */
/* [50] */	"'break' should appear only within an iteration or 'switch' statement",	/* uerror */
/* [51] */	"incorrect input character detected, hex value 0x%x",	/* uerror */
/* [52] */	"incorrect storage class",	/* uerror */
/* [53] */	"incorrect combination of pointer and integer for operator '%s'",	/* werror::operator */
/* [54] */	"incorrect comparison of enums",	/* uerror */
/* [55] */	"'continue' should appear only within an iteration statement",	/* uerror */
/* [56] */	"bitfield size of '%d' is out of range",	/* uerror */
/* [57] */	"incorrect bitfield type",	/* uerror */
/* [58] */	"function designator/pointer expected%s%s%s",	/* uerror */
/* [59] */	"incorrect hex constant",	/* uerror */
/* [60] */	"operand for the indirection operator '*' should have non-void pointer type",	/* uerror */
/* [61] */	"incorrect initialization",	/* uerror */
/* [62] */	"left-hand side of '%s' should be an lvalue",	/* uerror */
/* [63] */	"incorrect member use: '%s'",	/* uerror::name */
/* [64] */	"bitfield size of '%d' for '%s' is out of range",	/* uerror */
/* [65] */	"incorrect member use: perhaps '%s.%s'",	/* werror::name,name */
/* [66] */	"incorrect pointer combination",	/* werror */
/* [67] */	"pointers used with '-' should point to objects of equal size",	/* uerror */
/* [68] */	"'register' storage class should not be used at file scope",	/* uerror */
/* [69] */	"function parameter %s types in call and definition are incompatible", /* werror */
/* [70] */	"invalid multibyte character detected starting at '%c' (hex value 0x%x)", /* uerror */
/* [71] */      "pointer to object of unknown size used in context where size must be known", /* uerror */
/* [72] */	"type too complex", /* uerror */
/* [73] */	"no storage class, type specifier or type qualifier specified - 'int' assumed", /* werror */
/* [74] */	"'{' unexpected",	/* uerror */
/* [75] */	"loop not entered at top",	/* werror */
/* [76] */	"unnamed bitfields should not be initialized", /* werror */
/* [77] */	"function designator/pointer used in an incorrect context",	/* uerror */
/* [78] */	"unterminated string or character constant",	/* uerror */
/* [79] */	"aggregate automatic identifiers should not be initialized",	/* uerror */
/* [80] */	"'case' expression should be integral constant",	/* uerror */
/* [81] */	"non-null byte ignored in string initializer",	/* werror */
/* [82] */	"nonportable character comparison",	/* werror */
/* [83] */	"nonportable bitfield type",	/* uerror */
/* [84] */	"nonunique name demands struct/union or struct/union pointer",	/* uerror */
/* [85] */	"null dimension",	/* uerror */
/* [86] */	"null effect",	/* werror */
/* [87] */	"old-fashioned assignment operator",	/* werror */
/* [88] */	"old-fashioned initialization: use '='",	/* werror */
/* [89] */	"operands of '%s' have incompatible or incorrect types",	/* uerror::operator */
/* [90] */	"side effects incurred by operand of 'sizeof' are ignored", /* werror */
/* [91] */	"possible pointer alignment problem",	/* werror */
/* [92] */	"precedence confusion possible: parenthesize!",	/* werror */
/* [93] */	"precision lost in assignment to (sign-extended?) bitfield",	/* werror */
/* [94] */	"precision lost in bitfield assignment",	/* werror */
/* [95] */	"questionable conversion of function pointer",	/* werror */
/* [96] */	"redeclaration of '%s'",	/* uerror::name */
/* [97] */	"redeclaration of formal parameter '%s'", /* uerror, werror */
/* [98] */  	"pointer casts may be troublesome",	/* werror */
/* [99] */	"'sizeof' returns a zero value", /* werror */
/* [100] */	"statement not reached",	/* werror */
/* [101] */	"static identifier '%s' defined but never used",	/* werror::name */
/* [102] */	"struct/union '%s' never defined",	/* werror::name */
/* [103] */	"incorrect input character detected", /* uerror */
/* [104] */	"structure '%s' never defined",	/* werror::name */
/* [105] */	"structure reference must be addressable",	/* uerror */
/* [106] */	"structure typed union member should be named",	/* werror */
/* [107] */	"too many characters in character constant",	/* uerror */
/* [108] */	"number of initializers exceeds array size%s%s%s",	/* uerror */
/* [109] */	"linkage conflict with prior declaration for '%s'; %s linkage assumed", /* werror */
/* [110] */	"address operator '&' should not be applied to this operand",	/* uerror */
/* [111] */	"undeclared initializer name '%s'",	/* werror::name */
/* [112] */	"address/value of right-hand side of assignment should be known at compile time", /* uerror */
/* [113] */	"unexpected <end-of-file>",	/* uerror */
/* [114] */	"unknown size",	/* uerror */
/* [115] */	"unsigned comparison with 0?",	/* werror */
/* [116] */	"void function '%s' should not return a value",	/* uerror::name */
/* [117] */	"void type incorrect for identifier '%s'",	/* uerror */
/* [118] */	"void type incorrect in expression",	/* uerror */
/* [119] */	"array subscript should be nonnegative integral constant",	/* werror */
/* [120] */	"zero size bitfield",	/* uerror */
/* [121] */	"zero sized structure",	/* uerror */
/* [122] */	"'}' expected",	/* uerror */
/* [123] */	"'long' in 'case' label or 'switch' statement may be truncated",	/* werror */
/* [124] */	"bad octal digit '%c'",	/* werror */
/* [125] */	"floating point constant folding causes exception",	/* uerror */
/* [126] */	"invalid null dimension used in declaration of '%s'", /* uerror */
/* [127] */	"'main' returns random value to invocation environment",	/* werror */
/* [128] */	"'%s' may be indistinguishable from '%s' due to internal name truncation",	/* werror::name,name */
/* [129] */	"%s should not be arguments to 'sizeof'",	/* uerror */
/* [130] */	"void pointer inappropriate", /* uerror */
/* [131] */     "const-qualified object should not be incremented, decremented or assigned to", /* uerror */
/* [132] */     "",
/* [133] */     "storage classes should not be used in struct/union member declarations",
/* [134] */	"illegal second byte in 16-bit character", /* uerror */
/* [135] */     "'asm' code may be wrong with '-O'; try '+O1' or '#pragma OPT_LEVEL 1'",
/* [136] */     "language '%s' not available - processing continues using language 'n-computer'", /* werror */
/* [137] */     "empty declaration", /* werror */
/* [138] */     "%s constant too large to represent; high order bytes will be lost", /*werror */
/* [139] */     "empty hex escape sequence", /* uerror */
/* [140] */     "floating point constant '%s' too %s for represention; information will be lost", /* werror */
/* [141] */     "enum constant overflow: '%s' given value INT_MIN", /* werror */
/* [142] */     "NULL pointer constant, rather than void pointer, should be used with '%s' and pointer to function or function designator", /* werror */
/* [143] */     "void pointer should not be used as the left-hand side of '%s' when the right-hand side is pointer to function or function designator", /* werror */
/* [144] */     "NULL pointer constant, rather than void pointer, should be used as the right-hand side of '%s' if the left-hand side is pointer to function", /* werror */
/* [145] */     "qualifiers are not assignment-compatible", /* werror */
/* [146] */     "types of objects pointed to by operands of '%s' should be qualified or unqualified versions of compatible types", /* werror */
/* [147] */     "",
/* [148] */     "additional type specifiers should not be used with 'typedef' ",
/* [149] */     "function declarations for '%s' have incompatible return types",
/* [150] */  	"function declarations for '%s' should have the same number of parameters",
/* [151] */	"function declarations for '%s' have incompatible uses of '...'",
/* [152] */	"function declarations for '%s' have incompatible parameters",
/* [153] */	"function prototype and non-ANSI definition for '%s' should have the same number of parameters",
/* [154] */	"function prototype for '%s' should not use '...' when used with non-ANSI declaration",
/* [155] */	"function prototype and non-ANSI definition for '%s' have incompatible parameters",
/* [156] */	"function prototype for '%s' should not use '...' when used with an empty declaration",
/* [157] */	"function prototype for '%s' should contain parameters compatible with default argument promotions when used with an empty declaration",
/* [158] */	"incompatible array declarations for '%s'",
/* [159] */	"incompatible pointer declarations for '%s'",
/* [160] */	"incompatible qualifier declarations for '%s'",
/* [161] */	"incompatible declaration for '%s'",
/* [162] */     "'LANG' environment variable is not set", /* werror */
/* [163] */	"operands of '%s' have incompatible types (pointer and %s)", /* werror */
/* [164] */	"operands of '%s' should have %s type", /* uerror */
/* [165] */	"%s keyword should be followed by identifier or declaration list", /* uerror */
/* [166] */	"type-name of cast should be scalar", /* uerror */
/* [167] */	"%sstruct/union does not contain member '%s'", /* uerror::name */
/* [168] */	"controlling expression of '%s' should have %s type", /* werror, uerror */
/* [169] */	"local redeclaration of typedef '%s'", /* werror */
/* [170] */	"type error in array expression", /* uerror */
/* [171] */	"operands of '%s' have incompatible %s types", /* werror, uerror */
/* [172] */	"multiple %s in declaration", /* uerror */
/* [173] */	"should not subscript an array which has no lvalue", /* werror */
/* [174] */	"",
/* [175] */	"extra values in initializer ignored", /* werror */
/* [176] */	"double to float conversion exception", /* uerror */
/* [177] */	"types of cast operands should be scalar unless first operand has void type", /* uerror */
/* [178] */	"static function identifier '%s' used but not defined; external reference generated",   /* werror::name */
/* [179] */	"declaration list of %s should be nonempty",	/* uerror */
/* [180] */	"struct/union %s required before '%s'", /* uerror */
/* [181] */	"';' assumed at end of declaration list", /*werror */
/* [182] */	"',' ignored at end of declaration list", /* werror */
/* [183] */	"unknown size for '%s'", /* uerror */
/* [184] */	"zero sized bitfield '%s'", /* uerror */
/* [185] */	"zero sized struct/union", /* uerror */
/* [186] */	"too many multibyte characters in wide character constant; leading multibyte characters ignored",	/* werror */
/* [187] */	"syntax error in floating point constant", /* uerror */
/* [188] */	"prototypes and non-ANSI parameter declarations mixed", /* uerror */
/* [189] */	"incorrect mix of names and parameter types in parameter list", /* uerror */
/* [190] */	"parameter name list should occur only in function definition", /* uerror */
/* [191] */	"unexpected ';'", /* uerror */
/* [192] */	"'...' should not be used with non-ANSI parameter name list", /* uerror */
/* [193] */	"incorrect use of 'void' in parameter list", /* uerror */
/* [194] */	"parameter name omitted in function definition", /*  */
/* [195] */	"too many arguments for '%s'", /* uerror */
/* [196] */	"integer overflow in constant expression", /* werror */
/* [197] */     "not enough room for null terminator in string initializer", /* lint */
/* [198] */	"too few arguments for '%s'", /* uerror */
/* [199] */	"at least one fixed parameter is required", /* uerror */
/* [200] */     "'%s' has already been initialized", /* uerror */
/* [201] */	"",
/* [202] */	"wide characters supported in ANSI mode only", /* uerror */
/* [203] */     "use of an incomplete type '%s'", /* werror */
/* [204] */     "",
/* [205] */     "undefined escape character: '\\%c' treated as '%s'", /* werror */
/* [206] */     "undefined escape character: '\\%c' treated as '%c'", /* werror */
/* [207] */     "hex escape sequence contains more than two digits", /* werror */
/* [208] */     "long double constants supported in ANSI mode only", /* werror */
/* [209] */     "extraneous ';' ignored", /* werror */
/* [210] */     "'long float' type supported in compatibility (non-ANSI) mode only", /* werror, uerror */
/* [211] */     "'long double' type supported in ANSI mode only", /* werror */
/* [212] */     "unnamed bitfield should be within a struct/union", /* uerror */
/* [213] */     "unknown directive '#%s'", /* uerror */
/* [214] */     "invalid '_HP_SECONDARY_DEF'", /* uerror */
/* [215] */     "unrecognized compiler directive '%s' ignored", /* werror */
/* [216] */     "'ON' or 'OFF' should follow '#pragma OPTIMIZE'", /* werroe */
/* [217] */     "invalid optimization level for 'OPT_LEVEL' pragma; level %d assumed", /* werror */
/* [218] */     "pragma should not appear inside a function or block", /* werror */
/* [219] */     "small table size of '%d' specified for '%c' ignored", /* werror */
/* [220] */     "unrecognized option '%s%s' ignored", /* werror */
/* [221] */     "error in floating point constant", /* uerror */
/* [222] */     "redeclaration of '%s' hides formal parameter",
/* [223] */     "questionably signed expression - ANSI/non-ANSI promotion rules differ for 'unsigned char' and 'unsigned short'",
/* [224] */     "declare the VARARGS arguments you want checked!", /*lint*/
/* [225] */     "constant too big for field",
/* [226] */     "types in call and definition of '%s' are incompatible (pointer and %s) for parameter '%s'",
/* [227] */     "types in call and definition of '%s' have incompatible %s types for parameter '%s'",
/* [228] */     "types in call and definition of '%s' should be qualified or unqualified versions of compatible types for parameter '%s'",
/* [229] */     "types in call and definition of '%s' are incompatible for parameter '%s'",
/* [230] */	"enumeration constant '%s' is outside range representable by bitfield '%s'", /* werror */
/* [231] */	"%s '%s' declared in function parameter list will have scope limited to this function declaration or definition", /* werror */
/* [232] */	"untagged %s declared in function parameter list will have scope limited to this function declaration or definition", /* werror */
/* [233] */	"null statement following 'if'",
/* [234] */	"null statement following 'else'",
/* [235] */	"%sstruct/union '%s' does not contain member '%s'", /* uerror::name */
/* [236] */	"function prototype not visible at point of call",
/* [237] */	"trigraph sequence '??%c' will be converted to '%c' in ANSI mode",
/* [238] */	"token length exceeds 255 characters, truncated",
/* [239] */	"alignment of struct '%s' may not be portable",
/* [240] */	"trailing padding of struct/union '%s' may not be portable",
/* [241] */	"number of arguments in call does not agree with function definition",
/* [242] */	"unsupported __attributes or __options",
/* [243] */	"reference parameters are not supported",
/* [244] */	"old-style function declaration for '%s' in LINTSTDLIB",
};


