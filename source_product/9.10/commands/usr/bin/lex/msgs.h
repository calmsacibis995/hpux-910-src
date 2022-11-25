
/*
**  FILE: msgs.h
**
**  This file contains values for all error messages and corresponding
**  default text (in english) for each error.  The default text is only
**  used if, at runtime, the message catalog for lex could not be opened.
**
**  To Add A New Message:
**  (1)  Add a new #define value for the message.  The value should be one
**       greater than the last error number #define.
**
**  (2)  Add the message to the file lex.cat.in.  Preceed the error message
**       with the integer value used for the #define in step (1).
**
**       NOTE: Do Not reuse old message numbers.  This may result in incorrect
**             messages being displayed on systems that have mismatched 
**             executables and message catalogs.
**             
*/

#define MF_LEX "lex"

/* Set Numbers */

#define Const_Set        1
#define Msg_Set          2

/* Default Words Used in each message.  These are set 1 messages. */
/*       NOTE: Do Not reuse old message numbers.  This may result in incorrect
**             messages being displayed on systems that have mismatched 
**             executables and message catalogs.
*/

#define Message_String	0
#define Error_String    1
#define Warning_String  2
#define Line_String     3
#define COLON           4


/* Message String indexes.  These are for set 2 messages. */
/*       NOTE: Do Not reuse old message numbers.  This may result in incorrect
**             messages being displayed on systems that have mismatched 
**             executables and message catalogs.
*/

#define Empty		0
#define BADOPTION 	1
#define BADSTATE 	2
#define BIGPAR 		3
#define BIGPAR2 	4
#define CALLOCFAILED 	5
#define CHARPUSH 	6
#define CHARRANGE 	7
#define CHTAB1 		8
#define DCHAR0 		9
#define DCHARC 		10
#define EMAXPOS	 	11
#define EMAXPOS1 	12
#define ENDLESSTR 	13
#define ENTRANS 	14
#define ENTRANS1 	15
#define EOFCOM 		16
#define EOFSTR 		17
#define EPOS 		18
#define EREVERSE 	19
#define ERTEXTS 	20
#define CCLOVRFLW	21
#define EXSTATES 	22
#define INVALREQ 	23
#define LANG2LATE 	24
#define LINE 		25
#define LONGDEFS 	26
#define LONGSTART 	27
#define MANYCLASSES 	28
#define MANYDEFS 	29
#define MANYSTATES 	30
#define MANYSTATES2 	31
#define NEGINTR 	32
#define NOCORE1 	33
#define NOCORE2 	34
#define NOCORE3 	35
#define NOCORE4 	36
#define NODEF 		37
#define NOINPUT 	38
#define NOSTDIN 	39
#define NOLDIGITS 	40
#define NOLEXDRIV 	41
#define NONTERM 	42
#define NOPEN 		43
#define NOPEN2 		44
#define NOPORTCHAR 	45
#define NORATFOR 	46
#define NORULESEND 	47
#define NOSTART 	48
#define NOTRANS 	49
#define OVERFL 		50
#define PEOF 		51
#define SHIFT 		52
#define STRLONG 	53
#define MANYPCLASSES 	54
#define TSTART 		55
#define TSTART1 	56

#define XTRASLASH 	58
#define STATISTIC 	59
#define STATISTIC2 	60
#define STATISTIC3 	61
#define STATISTIC4 	62
#define BADRANGE1     	63
#define BADRANGE2     	64
#define BADRANGE3     	65
#define UNEQUALRNG    	66
#define BADCOLLSYM    	67
#define BADEQVCLS     	68
#define BADCHARCLS    	69
#define BADXOPT		70
#define BADTBLSIZE	71
#define BADTBLSPC	72
#define BADTRANS	73
#define EMPTYCOLLSYM	74
#define LOCALE2LATE	75
#define BADLOCALE	76
#define NOCARROT	77
#define NOMIXING	78
#define SYNTAXBKUP	79
#define STACKOVR	80
#define SYNTAX  	81
#define NOSPACE 	82

extern char msgs[][256];
extern nl_catd catd;

/* Below macros and declarationsare for converting numbers to strings. */
extern char b_[],  b_1[], b_2[], b_3[], 
            b_4[], b_5[], b_6[], b_7[];

