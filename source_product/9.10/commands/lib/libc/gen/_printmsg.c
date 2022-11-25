/* @(#) $Revision: 70.4 $ */    

/*
    9/15/87:

	1)  This module was largely rewritten.  Previously, it
	    made repeated "sprintf" calls, one for each format
	    specification.  See the "_printmsg" header in this
	    module for notes on the new mechanism.

	2)  16 bit support added.

	3)  For backward compatability, "associative" width/
	    precision is supported.  Width and precision specifi-
	    cations may be parameterized without a digit$
	    specifier, as in: %2$*.*d

	    This has some unfortunate limitations, and should
	    be phased out over time or as X/OPEN decides on a
	    more reasonable mechanism.

	4)  A general syntax check is performed on the caller's
	    format string.  Invalid syntax is rewarded with
	    a 0 (FALSE) return.  

	    While an embedded syntax check may not be in the true 
	    spirit of printf and its affinity for core dumps, the 
	    nl_printf variants have to chug through the format string
	    anyway to get all the %digit$ specifications.  

	    This is nearly a syntax check, so we toss one in at
	    the cost of a little extra overhead.  And since 0 
	    is already returned for an improper mixing of %digit$
	    and non-%digit$ fields, a precedent exists for
	    refusing to accept invalid syntax.  

	    Which is what printf should have done all along, on 
	    detecting bad format syntax...

    10/03/88:

	Rel. 7.0 features: XPG 3 and ANSI/X3J11 C

	1)  Numbered width/precision specifiers (%3$*6$.*7$d, etc) (XPG).
	    You can still have "associative" but can not mix the two
	    in the same specification.

	2)  Multiple references to numbered arguments (XPG).

	3)  "0" flag (ANSI).

	4)  "L" flag (ANSI).

	5)  "n" conversion character (ANSI).

	6)  "p" conversion character (ANSI).

    01/20/89:

	Rel. 7.0 features:

	We now have some long double <--> string library routines.
	The L type specifier is changed to mean long double and not
	double precision.  Long doubles implemented as a 4 unsigned
	integer structure.  There is a machine dependency.  For S800
	machines, the long double structure is passed by reference.
	For S300 machines, the long double structure is passed by value.

    03/16/92:
	
	Added support for two new conversion types:
		'C': wchar_t	wide character
		'S': wchar_t *	pointer to wide character string.

    06/19/92:
	Added support for ' (grouping) flag.

*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define fprintf _fprintf
#  ifdef __lint
#  define isascii _isascii
#  define isdigit _isdigit
#  endif /* __lint */
#endif

#include <ctype.h>
#include <stdio.h>
#include <varargs.h>
#include <msgcat.h>
#include <limits.h>		
#include <stdlib.h>
#ifdef _THREAD_SAFE
#include "rec_mutex.h"

extern REC_MUTEX _printmsg_rmutex;
#endif

#if defined NLS || defined NLS16
#include <nl_ctype.h>
#else
#define ADVANCE(p)	(++p)
#define CHARAT(p)	(*p)
#define CHARADV(p)	(*p++)
#define PCHAR(c,p)	(*p = c)
#define PCHARADV(c,p)	(*p++ = c)
#endif

#define FALSE		0
#define MAX_ARGLEN	10
#define TRUE		1
#define UCHAR		unsigned char
#define UINT		unsigned int

#define RANGE(x,min,max) ((x)>=(min) && (x)<=(max) ? TRUE : FALSE)

#define NOTYPE		0
#define	CHAR		1
#define	DOUBLE		2
#define	LONG_DOUBLE	3
#define	FLOAT_TYPE	4
#define	INT		5
#define LONG		6
#define SHORT		7
#define	STRING		8
#define	PTR_INT		9
#define	PTR_VOID	10
#define	WCHAR_T		11
#define	PTR_WCHAR_T	12

typedef union {				/* possible argtbl arguments */
	char		cval;
	double		dval;
#if defined (__hp9000s800)
	long_double *	ldval;
#else
	long_double	ldval;
#endif
	float		fval;
	int		ival;
	long int	lval;
	short int	sval;
	char *		cpval;
	int *		ipval;
	void *		vpval;
	wchar_t		wcval;
	wchar_t *	wcpval;
} arg_val;

struct table {
	int	type;		/* argument type 			*/
	int	numstar;	/* Number of non-digit '*'s -- 0, 1, or 2   */
	int	star1;		/* field width "*" argument		*/
	int	star2;		/* precision "*" argument		*/
	int	star1digit;	/* field width "*digit$" digit		*/
	int	star2digit;	/* precision "*digit$" digit		*/
	arg_val argval;		/* local storage for argument		*/
};
typedef	struct table	TABLE;

/*  argtbl[] will contain details about arguments from the caller's
    initial argument list, including type, width/precision specifiers, 
    and actual value.  Ordering of argtbl[] elements is the same as in 
    the argument list, i.e. argtbl[0] will contain details about the 
    first argument in the list.
*/

static TABLE argtbl[NL_ARGMAX];

/*  fmtorder[] will contain the order in which arguments were
    specified by the format string.  For instance, if the format is 
    "%3$d %2$d %1$d", then fmtorder[0] = 3, fmtorder[1] = 2, and 
    fmtorder[2] = 1;
*/

static UINT fmtorder[NL_SPECMAX];

static UINT numspecs,	/* number of conversion specs in `fmt`		*/
	    numargs,	/* number of arguments on the argument list	*/
	    long_flag,	/* long argument flag 				*/
	    short_flag,	/* short argument flag				*/
	    long_double_flag,	/* long double argument flag		*/
	    star1digit, /* field width "*digit$" digit			*/
	    star2digit, /* precision "*digit$" digit			*/
	    argdigit,	/* argument "%digit$" digit			*/
	    numstar,    /* number of non-digit "*" specifiers 		*/
	    fmtclass;	/* storage classes -- INT, DOUBLE, etc		*/

/*
    _printmsg() will read the caller's format string, rewriting it
    into the caller's storage in a form acceptable to "_doprnt".  
    All fields must be in the "%digits$" format, or an error will
    result.

    A re-ordered argument list is created in the caller's own storage
    area, based on the caller's arguement list and format string.  The
    rewritten format string and re-ordered argument list can be used in 
    conjunction with a subsequent call to _doprnt.

    No presumption is made that the caller's initial argument list is
    on the stack.  However, macros from "varargs.h" are used to access
    the arguments, so this function is compatible with stack arguments.

    The use of caller storage for re-ordered arguments is consistent
    with this design.  This approach also seems preferrable over 
    directly rewriting the stack because no formal syntax check is 
    made of the caller's format string here.  

    Such a check would typically be redundant since _doprnt() makes
    makes a similar check.  But without such a check, it is conceivable
    that an out-of-whack format string might cause the stack to be 
    corrupted if arguments were rewritten directly to the stack.

    Sufficient space for the rewritten format string and the re-ordered
    arguments is presumed.

    SYNTAX:

	int _printmsg(str, fmt, args, newargs)
	char *fmt;		-> format string
	char *str;		-> storage for rewritten format
	va_list args;		-> initial argument list
	va_list newargs;	-> storage for re-ordered argument list

	return(1)		:  success
	return(0)		:  invalid format

*/

int
_printmsg( fmt, str, args, newargs)
char *fmt;
char *str;
va_list args;
va_list newargs;
{
	register int i; 
#ifdef _THREAD_SAFE
	int retval;
#endif

	/* Initializations */

#ifdef _THREAD_SAFE
	_rec_mutex_lock(&_printmsg_rmutex);
#endif
	numspecs =  numargs = 0;
	for (i=0; i<NL_SPECMAX ; i++)
		fmtorder[i]=0;
	for (i=0; i<NL_ARGMAX ; i++)
		argtbl[i].type = NOTYPE;

	/* Parse the `fmt' string, set up argtbl information table */

	if (!parse((UCHAR *)str, fmt)) {
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_printmsg_rmutex);
#endif
		return(FALSE);
	}

	/*  copy arguments from caller's arguement list into the 
	    appropriate slot in the argtbl information table
	*/

	if (!saveargs(args)) {
#ifdef _THREAD_SAFE
		_rec_mutex_unlock(&_printmsg_rmutex);
#endif
		return(FALSE);
	}

	/*  rewrite the saved arguments into the caller's local
	    argument storage, in the order determined earlier
	    from the caller's format string
	*/

#ifdef _THREAD_SAFE
	retval = arg_rewrite(newargs);
	_rec_mutex_unlock(&_printmsg_rmutex);
	return(retval);
#else
	return(arg_rewrite(newargs));
#endif
}

/*
    arg_rewrite() will rewrite the values in argtbl[] into the caller's 
    argument list storage, ordering them based on the format order 
    indicated in fmtorder[].  
   
    NOTE:  "associative" width/precision specifiers are supported
	   here for backward compatability.  This feature may need
	   to be removed later.

    SYNTAX: 

	arg_rewrite(newargs)
	va_list newargs;	= address of caller's storage area

	return(1)		: success
	return(0)		: invalid format string; should have been
				  caught by earlier functions, but it's
				  an easy test...
*/

static int
arg_rewrite(newargs)
register va_list newargs;
{
    register UINT i, j;
    register TABLE *tbl;

    for (i=0; i<NL_SPECMAX; i++) {

	/* be sure conversion spec has an argument number */
	if (!fmtorder[i])
	    break;

	/* get argument table entry associated with spec */
	j = fmtorder[i] - 1;
	tbl = &argtbl[j];

	/* "associative" width/precision specifiers (%digit$*.*d, etc)
	   are supported here for backward compatability.
	*/
#ifdef __hp9000s800
	if (tbl->numstar) {
		va_arg(newargs, int);
		*((int *)newargs) = tbl->star1;
	}
	if (tbl->numstar > 1) {
		va_arg(newargs, int);
		*((int *)newargs) = tbl->star2;
	}

	/* "numbered" width/precision specifiers (%digit$*digit$.*digit$d, etc).
	   Can not mix "associative" and "numbered".
	*/
	if (tbl->star1digit) {
		va_arg(newargs, int);
		*((int *)newargs) = argtbl[(tbl->star1digit)-1].argval.ival;
	}
	if (tbl->star2digit) {
		va_arg(newargs, int);
		*((int *)newargs) = argtbl[(tbl->star2digit)-1].argval.ival;
	}
#endif /* __hp9000s800 */

#ifdef __hp9000s300
	if (tbl->numstar) {
		*((int *)newargs) = tbl->star1;
		va_arg(newargs, int);
	}
	if (tbl->numstar > 1) {
		*((int *)newargs) = tbl->star2;
		va_arg(newargs, int);
	}

	/* "numbered" width/precision specifiers (%digit$*digit$.*digit$d, etc).
	   Can not mix "associative" and "numbered".
	*/
	if (tbl->star1digit) {
		*((int *)newargs) = argtbl[(tbl->star1digit)-1].argval.ival;
		va_arg(newargs, int);
	}
	if (tbl->star2digit) {
		*((int *)newargs) = argtbl[(tbl->star2digit)-1].argval.ival;
		va_arg(newargs, int);
	}
#endif /* __hp9000s300 */

	/* place argument value associated with spec on new arg list */
	switch (tbl->type ) {
	    case CHAR :
#ifdef __hp9000s800
		va_arg(newargs, int);
		*((int *)newargs) = tbl->argval.cval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((int *)newargs) = tbl->argval.cval;
		va_arg(newargs, int);
#endif /* __hp9000s300 */
		break;
	    case STRING :
#ifdef __hp9000s800
		va_arg(newargs, char *);
		*((char **)newargs) = tbl->argval.cpval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((char **)newargs) = tbl->argval.cpval;
		va_arg(newargs, char *);
#endif /* __hp9000s300 */
		break;
	    case INT :
#ifdef __hp9000s800
		va_arg(newargs, int);
		*((int *)newargs) = tbl->argval.ival;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((int *)newargs) = tbl->argval.ival;
		va_arg(newargs, int);
#endif /* __hp9000s300 */
		break;
	    case SHORT :
#ifdef __hp9000s800
		va_arg(newargs, int);
		*((int *)newargs) = tbl->argval.sval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((int *)newargs) = tbl->argval.sval;
		va_arg(newargs, int);
#endif /* __hp9000s300 */
		break;
	    case LONG :
#ifdef __hp9000s800
		va_arg(newargs, int);
		*((int *)newargs) = tbl->argval.lval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((int *)newargs) = tbl->argval.lval;
		va_arg(newargs, int);
#endif /* __hp9000s300 */
		break;
	    case FLOAT_TYPE :
#ifdef __hp9000s800
		va_arg(newargs, double);
		*((double *)newargs) = tbl->argval.fval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((double *)newargs) = tbl->argval.fval;
		va_arg(newargs, double);
#endif /* __hp9000s300 */
		break;
	    case DOUBLE :
#ifdef __hp9000s800
		va_arg(newargs, double);
		*((double *)newargs) = tbl->argval.dval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((double *)newargs) = tbl->argval.dval;
		va_arg(newargs, double);
#endif /* __hp9000s300 */
		break;
	    case LONG_DOUBLE :
#if defined (__hp9000s800)
		va_arg(newargs, long_double*);
		*((long_double **)newargs) = tbl->argval.ldval;
#else
		*((long_double *)newargs) = tbl->argval.ldval;
		va_arg(newargs, long_double);
#endif
		break;
	    case PTR_INT :
#ifdef __hp9000s800
		va_arg(newargs, int *);
		*((int **)newargs) = tbl->argval.ipval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((int **)newargs) = tbl->argval.ipval;
		va_arg(newargs, int *);
#endif /* __hp9000s300 */
		break;
	    case PTR_VOID :
#ifdef __hp9000s800
		va_arg(newargs, void *);
		*((void **)newargs) = tbl->argval.vpval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((void **)newargs) = tbl->argval.vpval;
		va_arg(newargs, void *);
#endif /* __hp9000s300 */
		break;
	    case WCHAR_T :
#ifdef __hp9000s800
		va_arg(newargs, wchar_t);
		*((wchar_t *)newargs) = tbl->argval.wcval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((wchar_t *)newargs) = tbl->argval.wcval;
		va_arg(newargs, wchar_t);
#endif /* __hp9000s300 */
		break;
	    case PTR_WCHAR_T :
#ifdef __hp9000s800
		va_arg(newargs, wchar_t *);
		*((wchar_t **)newargs) = tbl->argval.wcpval;
#endif /* __hp9000s800 */
#ifdef __hp9000s300
		*((wchar_t **)newargs) = tbl->argval.wcpval;
		va_arg(newargs, wchar_t *);
#endif /* __hp9000s300 */
		break;
	    default :
		return(FALSE);
	}
    }
    return(TRUE);
}

/*
    store_argtbl(): store type and width/precision info in argument table.
    Can have mulitple references to numbered arguments.  But if you do,
    the type and width/precision information must agree with what was
    there before.
*/

static int
store_argtbl( num)
register UINT num;			/* position of argument in arg list */
{
	register TABLE *argtblptr;	/* point to arg table entry */

	if (! RANGE(num, 1, NL_ARGMAX)) {   /* valid arg #? */
		return FALSE;
	}

	argtblptr = &argtbl[num-1];

	if (argtblptr->type == NOTYPE) {
		/* new "digit$" */
		argtblptr->numstar = numstar;
		argtblptr->star1digit = star1digit;
		argtblptr->star2digit = star2digit;
		argtblptr->type	= fmtclass;
		numargs++;
	}
	else {	/* duplicate "digit$" */
		if ( (argtblptr->numstar != numstar) ||
		     (argtblptr->star1digit != star1digit) ||
		     (argtblptr->star2digit != star2digit) ||
		     (argtblptr->type != fmtclass) ) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
    parse() reads a format string which contains #$ field specifications.
    The format string is rewritten into another storage area with these
    specifications stripped out.  Information about each field speci-
    fication is stored in "argtbl[]" for subsequent use.  The
    "fmtorder[]" table is also set in this call.

    NOTE:  "associative" width/precision specifiers ($digit$*.*d, etc)
	   are supported here for backward compatability.  This feature 
	   may need removal later.

    NOTE:  10/3/88 jk
	   Can now have "numbered" width/precision specifiers 
	   (%digit$*digit$.*digit$d, etc).
	   You can still have "associative" but can not mix the two
	   in the same spec.

    SYNTAX:

	int parse(str, fmt)
	UCHAR *str;	-> new rewritten format string
	UCHAR *fmt;	-> format to be parsed

	return(1)	: success
	return(0)	: invalid format

*/

static int
parse( str, fmt)
UCHAR *str;
UCHAR *fmt;
{
	register TABLE	*argtblptr;
	register UINT c;
	int tmp1, tmp2;

	while (c = CHARADV(fmt)) {

		PCHARADV(c, str);
		if (c != '%')		/* not a fmt cmd */
			continue;

		c = CHARAT(fmt);
		if (c == '%') {		/* "%%" is not a fmt cmd */
			ADVANCE(fmt);
			PCHARADV(c, str);
			continue;
		}

		/* get a conversion spec and copy without digit$ */
		if (! parse_field(&str,&fmt)) {
			return(FALSE);
		}

		/* store conversion spec info in argument table */
		if (! store_argtbl( argdigit)) {
			return FALSE;
		}

		tmp1 = star1digit;
		tmp2 = star2digit;
		star1digit = star2digit = 0;
		if (tmp1) {	/* field width "*digit$" ? */
			/* store field width info in argument table */
			fmtclass = INT;
			if (! store_argtbl( tmp1)) {
				return(FALSE);
			}
		}

		if (tmp2) {	/* precision "*digit$" ? */
			/* store precision info in argument table */
			fmtclass = INT;
			if (! store_argtbl( tmp2)) {
				return(FALSE);
			}
		}

		/* keep track of the number and order of conversion specs */
		numspecs++;
		if (! RANGE(numspecs, 1, NL_SPECMAX)) {
			return FALSE;
		}
		fmtorder[numspecs-1] = argdigit;
	}
	*str = '\0';
	return(TRUE);
}

/*
    saveargs() will copy arguments from an argument list into the 
    appropriate fields in argtbl[].  "approriateness" is determined
    by the values in fmtorder[].  Typically, this function is used
    with stack arguments, but it will scan any argument list.

    NOTE:  "associative" width/precision specifiers are supported
	   here for backward compatability.  This feature may need
	   removal later.

    NOTE:  10/3/88 jk
	   Can now have "numbered" width/precision specifiers 
	   (%digit$*digit$.*digit$d, etc).
	   You can still have "associative" but can not mix the two
	   in the same spec.
	   "Associative" width/precision specifiers stored in
	   "star1" and "star2" in the same iteration that
	   handles the format conversion character.
	   "Numbered" width/precision specifiers are handled in a seperate
	   iteration and are NOT stored in "star1" and "star2".

    SYNTAX:

	int saveargs(args)
	va_list args;	-> argument list to scan

	return(TRUE)	:  no problems
	return(FALSE)	:  invalid format specified
*/

static int
saveargs( args)
register va_list args;
{
	register UINT i;

#ifdef TRACE
	fprintf(stderr, "numargs = %d\n", numargs);
#endif

	for (i=0 ; i < numargs ; i++) {
#ifdef TRACE
		fprintf(stderr, "argdigit = %d\n", i+1);
		fprintf(stderr, "type   = %d\n", argtbl[i].type);
#endif

		/* get "associative" width/precision arguments */

		if (argtbl[i].numstar) {
			argtbl[i].star1 = va_arg(args, int);
		}

		if ((argtbl[i].numstar) > 1) {
			argtbl[i].star2 = va_arg(args, int);
		}

		/* store the argtbl values temporarily in the "argtbl"
		   array; the fmtorder of "cases" in this "switch" is
		   loosely based on most common order of useage
		*/

		switch(argtbl[i].type) {
		case CHAR :
			argtbl[i].argval.cval = va_arg(args, int);
			break;
		case STRING :
			argtbl[i].argval.cpval = va_arg(args, char *);
			break;
		case INT :
			argtbl[i].argval.ival = va_arg(args, int);
			break;
		case LONG :
			argtbl[i].argval.lval = va_arg(args, int);
			break;
		case SHORT :
			argtbl[i].argval.sval = va_arg(args, int);
			break;
		case FLOAT_TYPE :
			argtbl[i].argval.fval = va_arg(args, double);
			break;
		case DOUBLE :
			argtbl[i].argval.dval = va_arg(args, double);
			break;
		case LONG_DOUBLE :
#if defined (__hp9000s800)
			argtbl[i].argval.ldval = va_arg(args, long_double *);
#else
			argtbl[i].argval.ldval = va_arg(args, long_double);
#endif
			break;
		case PTR_INT :
			argtbl[i].argval.ipval = va_arg(args, int *);
			break;
		case PTR_VOID :
			argtbl[i].argval.vpval = va_arg(args, void *);
			break;
		case WCHAR_T :
			argtbl[i].argval.wcval = va_arg(args, wchar_t);
			break;
		case PTR_WCHAR_T :
			argtbl[i].argval.wcpval = va_arg(args, wchar_t *);
			break;
		default :
			/* have a hole in the %n$ specs */
			return(FALSE);
		}
	}
	return(TRUE);
}

/*
    digit_dollar(): If you have "digit$" sequence return the value of "digit".
    Otherwise, return 0.

    Note:  The caller's format string pointer will be incremented
*/

static int
digit_dollar( fmt)
register UCHAR **fmt;		/* ptr to format string ptr */
{
    register UINT c;		/* character in format string */
    register UINT num;		/* atoi of "digit" */
    register UCHAR *savfmt;	/* don't go one past */

    num = 0;
    savfmt = *fmt;

    while ((c = CHARADV( *fmt)) && isascii(c) && isdigit(c)) {
	num = num * 10 + c - '0';
        savfmt = *fmt;
    }
    *fmt = savfmt;

    if (num && c == '$') {
	return num;
    }
    else {
    	return 0;
    }
}

/*
    is_fmtchar() will determine if a character is one of the
    valid "printf" field specification types.
   
    SYNTAX:
 
	int is_fmtchar(c)
	UINT c;		-> character to be tested
   
	return(0)	: not a valid printf type
	return(#)	: indicates which class, i.e INT, SHORT, etc.
*/
 
static int 
is_fmtchar( c)
register UINT c;
{
	switch (c) {
		case 'c':
			return(INT);
		case 's':
			return(STRING);
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x': case 'X':
			if (long_flag)
				return(INT);
			else if (short_flag)
				return(INT);
			else
				return(INT);
		case 'f':
		case 'e':case 'E':
		case 'g':case 'G':
			if (long_double_flag)
				return(LONG_DOUBLE);
			else
				return(DOUBLE);
		case 'n':
			return(PTR_INT);
		case 'p': case 'P':
			return(PTR_VOID);
		case 'C':
			return(WCHAR_T);
		case 'S':
			return(PTR_WCHAR_T);
		default:
			return(FALSE);
	}
}

/*
    parse_field() will determine if a "printf" field is formatted
    reasonably.  It assumes the pointer it receives is aimed just
    past the initial "%" of a field specification, and that the
    first characters in the field are digit$, for parameter
    re-ordering.

    The notion of "reasonable" is pretty bizarre in "printf-land".
    For instance, the following produces "reasonable" results:

	printf("%+8.--#-6.#4+s", "abcde");

    The following global values are set here:

    	fmtclass long_flag, short_flag, long_double_flag,
	numstar, star1digit, star2digit

    SYNTAX:

	parse_field(fmt)
	UCHAR **str;	-> new format
	UCHAR **fmt;	-> old format

	return(0)	:  invalid field specification
	return(#)	:  valid field; indicates type of
			   field, i.e., "d" for %+2.3d, etc.
*/


static int
parse_field( str,fmt)
register UCHAR **str;
register UCHAR **fmt;
{
    register UINT c;
    UCHAR *fmtstar;

    /* check for valid %digits$ specification */
    if (! (argdigit = digit_dollar(fmt)))
	return(FALSE);

    ADVANCE(*fmt);
    numstar = 0;
    star1digit = star2digit = 0;
    long_flag  = short_flag = long_double_flag = FALSE;

    while (c = CHARAT(*fmt)) {	/* a very loose syntax check... */
	switch (c) {
	    case '0' :
	    case '1' :
	    case '2' :
	    case '3' :
	    case '4' :
	    case '5' :
	    case '6' :
	    case '7' :
	    case '8' :
	    case '9' :
            case '+' :
	    case '-' :
	    case '.' :
	    case '#' :
	    case ' ' :
	    case '\'' :
		break;
	    case 'h' :
		short_flag = TRUE;
		break;
	    case 'l' :
		long_flag = TRUE;
		break;
	    case 'L' :
		long_double_flag = TRUE;
		break;

	    case '*' :		
		if (++numstar == 1) {
		    fmtstar = *fmt;
		    ADVANCE(fmtstar);
		    if (star1digit = digit_dollar(&fmtstar)) {
			*fmt = fmtstar;
		    }
		}
		else if (numstar == 2) {
		    fmtstar = *fmt;
		    ADVANCE(fmtstar);
		    if (star2digit = digit_dollar(&fmtstar)) {
		        *fmt = fmtstar;
		    }
		}
		else {
		    return(FALSE);
		}
		break;

	    default :
		goto end;
	}
	PCHARADV(c,*str);
	ADVANCE(*fmt);
    }

end:
    /* Can not mix "associative" and "numbered" width/precision specifiers */
    if ((numstar == 2) && 
	((star1digit && !star2digit) || (star2digit && !star1digit))) {
	return(FALSE);
    }
    if (star1digit || star2digit) {
	numstar = 0;
    }

    /* find type of conversion spec */
    if (!c)
	return(FALSE);
    if (fmtclass = is_fmtchar(c)) {
	PCHARADV(c,*str);
	ADVANCE(*fmt);
	return(c);
    }
    else
        return(FALSE);
}
