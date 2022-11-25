#define NOPARSER	1 /*"cannot find parser %s"*/
#define TOOBIG  	4 /*"item too big"*/
#define SUM1		5 /*"\n%d/%d terminals, %d/%d nonterminals\n"*/
#define SUM2 		6 /*"%d/%d grammar rules, %d/%d states\n"*/
#define SUM3 		7 /*"%d shift/reduce, %d reduce/reduce conflicts reported\n"*/
#define SUM4 		8 /*"%d/%d working sets used\n"*/
#define SUM5 		9 /*"memory: states,etc. %d/%d, parser %d/%d\n"*/
#define SUM6 		10 /*"%d/%d distinct lookahead sets\n"*/
#define SUM7 		11 /*"%d extra closures\n"*/
#define SUM8 		12 /*"%d shift entries, %d exceptions\n"*/
#define SUM9 		13 /*"%d goto entries\n"*/
#define SUM10 		14 /*"%d entries saved by goto default\n"*/
#define SUM11 		15 /*"\nconflicts: "*/
#define SUM12 		16 /*"%d shift/reduce"*/
#define SUM13		17 /*", "*/
#define SUM14 		18 /*"%d reduce/reduce"*/
#define NEWLINE		19 /*"\n"*/
#define FATAL 		20 /*" fatal error: "*/
#define ERRORCOL	21 /*" error: "*/
#define SRCANDLINE	22 /*"\"%s\", line %d:"*/
#define NULL_S		23 /*"\tNULL"*/
#define OBRACE 		24 /*" { "*/
#define PCTS		25 /*"%s "*/
#define CBRACE		26 /*"}"*/
#define UNDEFNTERM	27 /*"nonterminal %s not defined!"*/
#define PYIELD 		28 /*"internal Yacc error: pyield %d"*/
#define NLNPCTS		29 /*"\n%s: "*/
#define PCTDNLN 	30 /*" %d\n"*/
#define STATEERR	31 /*"yacc state/nolook error"*/
#define MANYSTATES 	32 /*"too many states, try -Ns option"*/
#define PUTITEM		33 /*"putitem(%s), state %d\n"*/
#define NOSPACE 	34 /*"out of state space, try -Nm option"*/
#define NOSTRING	35 /*"nonterminal %s never derives any token string"*/
#define PCTDCOL		36 /*"%d: "*/
#define PCTSPCTD	37 /*"%s %d, "*/
#define WSOVER		39 /*"working set overflow, try -Nw option"*/
#define STNLOOK		40 /*"\nState %d, nolook = %d\n"*/
#define FLAGSET		41 /*"flag set!\n"*/
#define TABPCTS		42 /*"\t%s"*/
#define MANYLKAHD	44 /*"too many lookahead sets, try -Nl option"*/
#define CANTOPEN	45 /*"cannot open %s"*/
#define OISDEFAULT	47 /*"`o' flag now default in yacc\n"*/
#define NORATFOR	48 /*"Ratfor Yacc is dead: sorry...\n"*/
#define BADNOPT 	49 /*"illegal -N option"*/
#define SMALLTBL	50 /*"Table size specified too small: %s"*/
#define BADXOPT 	51 /*"illegal -X option"*/
#define PSRRST 		53 /*"parser is reset to %s\n"*/
#define BADOPT 		54 /*"illegal option: %c"*/
#define CANTOPNTMP	57 /*"cannot open temp files"*/
#define CANTOPNINP	58 /*"cannot open input file"*/
#define BADPCTSTRT	59 /*"bad %%start construction"*/
#define BADPCTTYPE	60 /*"bad syntax in %%type"*/
#define TKNREDECL	61 /*"type redeclaration of token %s"*/
#define NTERMREDECL	62 /*"type redeclaration of nonterminal %s"*/
#define PRECREDECL	63 /*"redeclaration of precedence of %s"*/
#define MANYPREC	64 /*"too many precedence levels"*/
#define TYPEREDECL 	65 /*"redeclaration of type of %s"*/
#define LATEPCTS	66 /*"please define type number of %s earlier"*/
#define SYNTAX		67 /*"syntax error"*/
#define BADEOF		68 /*"unexpected EOF before %%"*/
#define BADSYNTAX1	69 /*"bad syntax on first rule"*/
#define LHSILL		70 /*"token illegal on LHS of grammar rule"*/
#define NOSEMI		71 /*"illegal rule: missing semicolon or | ?"*/
#define BADPCTPREC	72 /*"illegal %%prec syntax"*/
#define BADTERMPREC	73 /*"nonterminal %s illegal after %%prec"*/
#define MANYPRODS	74 /*"too many production rules, try -Np option"*/
#define NOVALUE		75 /*"must return a value, since LHS has a type"*/
#define TYPECLASH	76 /*"default action in the preceding production causes potential type clash"*/
#define MANYNTERMS	78 /*"too many nonterminals, try -Nn option"*/
#define MANYTERMS	79 /*"too many terminals, limit %d"*/
#define BADESCAPE	80 /*"invalid escape"*/
#define BADNNN		81 /*"illegal \\nnn construction"*/
#define NONULL		82 /*"'\\000' is illegal"*/
#define MANYCHARS	83 /*"too many characters in id's and literals, try -Nc option"*/
#define BADCLAUSE	84 /*"unterminated < ... > clause"*/
#define MANYTYPES	85 /*"too many types"*/
#define NOQUOTE		86 /*"illegal or missing ' or \""*/
#define BADRSRVDWRD	87 /*"invalid escape, or illegal reserved word: %s"*/
#define MISSNGTYPE	88 /*"must specify type for %s"*/
#define LATEDEFIN	89 /*"%s should have been defined earlier"*/
#define EOFINUNION	90 /*"EOF encountered while processing %%union"*/
#define EOFINCODE	91 /*"eof before %%}"*/
#define BADCMNT		92 /*"illegal comment"*/
#define EOFINCMNT	93 /*"EOF inside comment"*/
#define IDENTSYNTAX	94 /*"bad syntax on $<ident> clause"*/
#define BADDOLLAR	95 /*"Illegal use of $%d"*/
#define NOTYPE		96 /*"must specify type of $%d"*/
#define BADSTRING	98 /*"newline in string or char. const."*/
#define EOFINSTRING	99 /*"EOF in string or character constant"*/
#define NOTERMINATE	100 /*"action does not terminate"*/
#define REDREDMSG	105 /*"\n%d: reduce/reduce conflict (red'ns %d and %d ) on %s"*/
#define LARGEACTION	106 /*"action table overflow"*/
#define TAB		107 /*"\t"*/
#define PCTD		108 /*"%d "*/
#define NOSPACEACT	110 /*"no space in action table"*/
#define GOTOSMSG	111 /*"%s: gotos on "*/
#define SHFTRED		114 /*"\n%d: shift/reduce conflict (shift %d, red'n %d) on %s"*/
#define STATEMSG	115 /*"\nstate %d\n"*/
#define TABPCTSNL	116 /*"\t%s\n"*/
#define NLTABPCTS	118 /*"\n\t%s  "*/
#define ACCEPT		119 /*"accept"*/
#define ERROR		120 /*"error"*/
#define SHIFT		121 /*"shift %d"*/
#define REDUCE		122 /*"reduce %d"*/
#define REDUCENL	123 /*"\n\t.  reduce %d\n\n"*/
#define ERRORNL		124 /*"\n\t.  error\n\n"*/
#define GOTO		125 /*"\t%s  goto %d\n"*/
#define NOTREDUCED	126 /*"Rule not reduced:   %s\n"*/
#define NEVERREDUCED	127 /*"%d rules never reduced\n"*/
#define BADTMPFILE	129 /*"bad tempfile"*/
#define NULLSTATE	131 /*"State %d: null\n"*/
#define ARRYOVRFLW	132 /*"a array overflow, try -Na option"*/
#define OPTOVRFLW	137 /*"out of space in optimizer a array, try -Na option"*/
#define ARRYCLOBER	138 /*"clobber of a array, pos'n %d, by %d,\n\t\tcheck for multiple definition of token # %d"*/
#define OPTSPACE	141 /*"Optimizer space used: input %d/%d, output %d/%d\n"*/
#define TBLENTRIES	142 /*"%d table entries, %d zero\n"*/
#define MAXSPREAD	143 /*"maximum spread: %d, maximum offset: %d\n"*/
#define NOSPACENM	144 /*"out of space, try -Nm option"*/
#define NOINTERNMEM	145 /*"out of space - unable to allocate memory for internal use"*/
#define NONT		146 /*"-Nt not implemented, ignored"*/
#define BADTBLSPEC	147 /*"unrecognized table specifier -N%c (ignored)"*/
#define BADPNAME	148 /*"missing external-name-prefix in -p argument"*/
#define NOGRAMMAR	149 /*"no grammar filename argument"*/
#define BADARGS		150 /*"extra arguments after filename or improper argument format"*/
#define WARNING		151 /*" warning: "*/
#define ILLCHAR		152 /*"Illegal character in token name '%s'"*/
#define BADBNAME	153 /*"missing file-prefix-name in -b argument"*/
