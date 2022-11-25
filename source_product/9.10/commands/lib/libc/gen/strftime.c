/* @(#) $Revision: 72.2 $ */
/* LINTLIBRARY */
/*
 *	strftime()
 */

/*
 * alternate digits support
 * Note: (dt: 2/27/92)
 * ALT_DIGITS for %O conversion modifier.
 * ALT_DIGITS is not supported for 9_0 release (POSIX.2). So, this is not implemented here.
 * Thus %O modifiers are not implemented as such, except for parsing the format string!
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define nl_langinfo _nl_langinfo
#define tzname __tzname
#define tzset _tzset
#define strftime _strftime
#endif

#include	<time.h>
#include	<langinfo.h>
#include	<nl_ctype.h>
#include	<setlocale.h>
#ifdef _THREAD_SAFE
#include "rec_mutex.h"

/* use same mutex as ctime(3C) routines because we may reference tzname */
extern REC_MUTEX _ctime_rmutex;
#endif


#define TRUE		1
#define FALSE		0
#define LEFT		1					/* field should be flush on the left	*/
#define RIGHT		2					/* field should be flush on the right	*/
#define NOVAL		-1					/* no value specified for width/precision */
#define NUMERIC		1					/* field type is "numeric"		*/
#define STRING		2					/* field type is "string"		*/
#define RANGE(a,b,c)	(a >= b && a <= c)			/* is b <= a <= c ?			*/
#define E_BUF_SIZE	1					/* error code for exceeding buffer size	*/
#define E_BAD_SPEC	2					/* error code for bad format spec	*/
#define E_BAD_NUMBER	3					/* error code for number out of range	*/

static struct fld {						/* field width/precision details	*/
    int type;							/*    numeric or string			*/
    int leadzero;						/*    leading zeros instead of spaces	*/
    int flush;							/*    align field on left or right	*/
    int width;							/*    min field width			*/
    int precision;						/*    min digits or max string len	*/
};

static struct tm	*timest;				/* global ptr to time being formatted	*/
static unsigned char	*buf;					/* global ptr to output buffer		*/
static unsigned char	*bufend;				/* global ptr to end of output buffer	*/
static struct _era_data	*last_era;				/* global ptr to era matching timest	*/
static int		alt_digits;				/* true if locale has alt digits	*/

char			__nl_derror;				/* error flag/character for date.c	*/

static unsigned char	*fld_spec();				/* functions used and defined below	*/
static void		bufright();


/*
 * strftime() returns the a formatted date/time string based upon the time
 * values supplied with the timeptr argument and the format specification
 * of the format argument.  No more than maxsize bytes are written to the user
 * supplied buffer `s'.  The number of bytes written, not including the null
 * terminator, is returned unless there was an error condition in which case
 * zero is returned.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef strftime
#pragma _HP_SECONDARY_DEF _strftime strftime
#define strftime _strftime
#endif

size_t strftime(s, maxsize, format, timeptr)
unsigned char		*s;
size_t			maxsize;
const unsigned char	*format;
const struct tm		*timeptr;
{
#ifdef _THREAD_SAFE
    int len;

    _rec_mutex_lock(&_ctime_rmutex);
#endif

    __nl_derror = '\0';						/* error flag -- used only by "date(1)"	*/

    /*
     * POSIX.1 requires tzset to be called regardless of the actual
     * formatting requested.
     */
    (void) tzset();

    bufend  = (buf = s) + maxsize - 1;				/* init buf/buflen for other routines	*/
    timest = timeptr;						/* make timeptr visible /wo passing it	*/
    last_era = (struct _era_data *)0;				/* indicate no era matched as of yet	*/
    alt_digits = *_nl_dgt_alt != '\0';				/* any alternative digits?		*/

    if (do_time(format) == E_BUF_SIZE) {			/* do the formatting			*/
#ifdef _THREAD_SAFE
	_rec_mutex_unlock(&_ctime_rmutex);
#endif
	return (0);						/* return 0 if buffer too small		*/
    }
    else {
	*buf = '\0';						/* close off the output			*/
#ifdef _THREAD_SAFE
	len = buf - s;						/* return its size			*/
	_rec_mutex_unlock(&_ctime_rmutex);
	return (len);
#else
	return (buf - s);					/* return its size			*/
#endif
    }
}



/*

    do_time() chugs through the format string, making calls to
    do_fmt() each time a format specification is encountered,
    otherwise just echoing format characters into the date
    buffer.  do_time() also handles any field-width specifications
    that immediately follow a "%" specifier, aligning whatever
    do_fmt() puts in the date buffer.  This function is re-entrant.

*/

static int	do_time(fmt)
register unsigned char	*fmt;					/* format to use */
{
    unsigned char		*base;
    register unsigned char	*start_spec;
    register unsigned int	c;
    unsigned int		i;
    struct fld			fld;
    int				modifier;			/* for %E and %O conversion modifiers */

    while (c = CHARADV(fmt)) {

	if (c != '%') {						/* regular char or start of specification? */

	    if ((buf + (c > 255)) >= bufend) {			/* room left to copy regular char? */
		if (!__nl_derror)
			__nl_derror = c;
		return (E_BUF_SIZE);
	    } else
		WCHARADV(c, buf);				/* copy regular char to output buffer */

	} else {

	    start_spec = fmt;					/* save start of specification */
	    base = buf;						/* and current output buffer position */

	    if (!(fmt = fld_spec(fmt, &fld))) {			/* invalid field specification? */
		fmt = start_spec;				/* yep, go back to its start */
		if (buf == bufend)
		    return (E_BUF_SIZE);			/* no room, return error */
		else
		    WCHARADV('%', buf);				/* else begin copy of bad spec to output */
	    }

	    modifier = check_modifier(fmt);			/* Draft 11 XPG4 for %E and %O */
	    if(modifier) CHARADV(fmt);
	    switch (do_fmt(*fmt, &fld, modifier)) {		/* process the specification */

	    case E_BUF_SIZE:					/* no room to hold the output */
		if (!__nl_derror)
		    __nl_derror = CHARAT(fmt);
		return (E_BUF_SIZE);
	    
	    case E_BAD_SPEC:					/* bad specification */
		if (*fmt)
		    ADVANCE(fmt);				/* advance, but not past end of format */
		if (buf + (fmt - start_spec) >= bufend)		/* if there's room ... */
		    return (E_BUF_SIZE);
		else
		    for (i=fmt-start_spec; i; i--)		/* copy the bad spec to the output */
			*buf++ = *start_spec++;
		break;
	    
	    default:						/* continue on with regular processing */
		if (fld_adjust(base, &fld))			/* adjust width and precision per request */
		    fmt++;					/* done (no kanji could be at this position */
		else {
		    if (!__nl_derror)				/* only error possible is not enough room */
			__nl_derror = CHARAT(fmt);
		    return (E_BUF_SIZE);
		}
	    
	    }	/* end switch */
	}
    }	/* end while */

    return (0);							/* return success */
}



/*
    do_fmt() handles individual format specifications, translating
    fields of the "tm" structure into the date buffer or calling
    nl_langinfo for local date strings.  Some strings that nl_langinfo
    returns are format strings themselves, so do_fmt() calls do_time()
    recursively to handle them.  Its return value is the status of the
    formatting--0 if all went well, an error code otherwise.
*/

static int     do_fmt(fmt, fld, modf)
register unsigned char	fmt;					/* format specification character	*/
struct fld *fld;						/* field width details			*/
int modf;							/* modifier present? */
{
    register int i, hour;
    char *rc;
    int  fweek, ifsun;

    fld->type = STRING;						/* default value			*/

    switch(fmt) {
	/*
	 *   %% - the percent symbol
	 */
	case '%':
	    *buf++ = '%';
	    break;

	/*
	 *   %n - the newline character
	 */
	case 'n':
	    *buf++ = '\n';
	    break;

	/*
	 *   %t - the tab character
	 */
	case 't':
	    *buf++ = '\t';
	    break;

	/*
	 *   %C - century number as a decimal number [00,99]
	 */
	case 'C':
	    if(modf) {	/* name of the base year in the alternative representation */
	       era_fmt(fld, 'N');
	       if(last_era != (struct _era_data *)0) return(0);
	       /* else continue processing as if modifier did not exist */
	    }
	    fld->type  = NUMERIC;
	    return (_itoa((timest->tm_year+1900) / 100, 2, fld));

	/*
	 *   %y - year without century as a decimal number [00,99]
	 */
	case 'y':
	    fld->type  = NUMERIC;
	    if(modf) {	/* Offset from %EC in the alternate representation */
	       era_fmt(fld, 'X');			/* just get last_era pointer */
	       if(last_era != (struct _era_data *)0) {
		  i = (timest->tm_year + 1900 - last_era->origin_year) * last_era->signflag
			  + last_era->offset;
	          return(_itoa(i, 4, fld));
	       }
	       /* else continue processing as if modifier did not exist */
	    }
	    return (_itoa((timest->tm_year+1900) % 100, 2, fld));

	/*
	 *   %Y - year with century as a decimal number [0000,9999]
	 */
	case 'Y':
	    fld->type  = NUMERIC;
	    if(modf) {	/* full alternative year representation */
		era_fmt(fld, 'X');			/* get last_era pointer */
		if(last_era && *last_era->format)
		   return(do_time(last_era->format));
		/* else, just ignore the modifier */
	    }
	    return (_itoa(timest->tm_year+1900, 4, fld));

	/*
	 *   %m - month as a decimal number [01,12]
	 */
	case 'm':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_mon+1, 2, fld));

	/*
	 *   %e - day of the month as a decimal number [1,31]
	 *   in a 2-digit field with a leading space fill.
	 */
	case 'e':
	    fld->type  = NUMERIC;
	    if(fld->width == NOVAL) fld->width = 2;	/* default */
	    fld->leadzero = 0;				/* leading spaces */
	    return (_itoa(timest->tm_mday, 2, fld));

	/*
	 *   %d - day of the month as a decimal number [01,31]
	 */
	case 'd':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_mday, 2, fld));

	/*
	 *   %D - date in usual US format (MM/DD/YY)
	 */
	case 'D':
	    return (do_time((unsigned char *) "%m/%d/%y"));

	/*
	 *   %H - hour (24-hour clock) as a decimal number [00,23]
	 */
	case 'H':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_hour, 2, fld));

	/*
	 *   %M - minute as a decimal number [00,59]
	 */
	case 'M':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_min, 2, fld));

	/*
	 *   %S - second as a decimal number [00,59]
	 */
	case 'S':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_sec, 2, fld));

	/*
	 *   %T - time in 24-hour US format (HH:MM:SS)
	 */
	case 'T':
	    return (do_time((unsigned char *) "%H:%M:%S"));

	/*
	 *   %j - day of the year as a decimal number [001,365]
	 */
	case 'j':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_yday+1, 3, fld));

	/*
	 *   %w - weekday as a decimal number [0(Sunday),6]
	 */
	case 'w':
	    fld->type  = NUMERIC;
	    return (_itoa(timest->tm_wday, 1, fld));

	/*
	 *   %u - weekday as a decimal number [1(Monday),7]
	 */
	case 'u':
	    fld->type  = NUMERIC;
	    if((i = timest->tm_wday) == 0) i = 7;   /* mon=1, tue=2,...sun=7 */
	    return (_itoa(i, 1, fld));

	/*
	 *   %R - time in 24-hour notation (%H:%M)
	 */
	case 'R':
	    return (do_time((unsigned char *) "%H:%M"));

	/*
	 *   %r - time in 12-hour US format (HH:MM:SS [AM|PM])
	 */
	case 'r':
	    return (do_time((unsigned char *) "%I:%M:%S %p"));

	/*
	 *   %I - hour (12-hour clock) as a decimal number [01,12]
	 */
	case 'I':
	    fld->type = NUMERIC;
	    hour =  timest->tm_hour;
	    if (hour == 0)
		hour = 12;
	    else if (hour > 12)
		hour -= 12;
	    return (_itoa(hour, 2, fld));

	/*
	 *   %p - locale's equivalent of AM or PM
	 */
	case 'p':
	    if (timest->tm_hour < 12)
		return (x_strcpy((unsigned char *) nl_langinfo((nl_item)AM_STR)));
	    else
		return (x_strcpy((unsigned char *) nl_langinfo((nl_item)PM_STR)));

	/*
	 *   %h - locale's abbreviated month name
	 *        (%h is for backward compatibility, %b is the standard format spec)
	 */
	case 'h':

	/*
	 *   %b - locale's abbreviated month name
	 */
	case 'b':
	    if (RANGE(timest->tm_mon, 0, 11))
		return (x_strcpy((unsigned char *) nl_langinfo((nl_item)timest->tm_mon+ABMON_1)));
	    return 0;

	/*
	 *   %a - locale's abbreviated weekday name
	 */
	case 'a':
	    if (RANGE(timest->tm_wday, 0, 6))
		return (x_strcpy((unsigned char *) nl_langinfo((nl_item)timest->tm_wday+ABDAY_1)));
	    return 0;

	/*
	 *   %z - time zone name
	 *        (%z is for backward compatibility, %Z is the standard format spec)
	 */
	case 'z':

	/*
	 *   %Z - time zone name
	 */
	case 'Z':
	    if (timest->tm_isdst)
		return (x_strcpy((unsigned char *) tzname[1]));
	    else
		return (x_strcpy((unsigned char *) tzname[0]));

	/*
	 *   %A - locale's full weekday name
	 */
	case 'A':
	    if (RANGE(timest->tm_wday, 0, 6))
		return (x_strcpy((unsigned char *) nl_langinfo((nl_item)timest->tm_wday+DAY_1)));
	    return 0;

	/*
	 *   %F - locale's full month name
	 *        (%F is for backward compatibility, %B is the standard format spec)
	 */
	case 'F':

	/*
	 *   %B - locale's full month name
	 */
	case 'B':
	    if (RANGE(timest->tm_mon, 0, 11))
		return (x_strcpy((unsigned char *) nl_langinfo((nl_item)timest->tm_mon+MON_1)));
	    return 0;

	/*
	 *   %o - locale's Emperor/Era year
	 */
	case 'o':
	    fld->type = NUMERIC;

	/*
	 *   %N - locale's Emperor/Era name
	 */
	case 'N':

	/*
	 *   %E - locale's combined Emperor/Era name and year
	 */
	case 'E':
	    return (era_fmt(fld, fmt));

	/*
	 *   %c - locale's appropriate date and time representation
	 */
	case 'c':
	    if(modf) {	/* alternate date and time representation */
	    /* $$ TO BE DONE LATER after the format is added to the locale
	     * for XPG4 */
	    }
	    return (do_time((unsigned char *) nl_langinfo((nl_item)D_T_FMT)));

	/*
	 *   %x - locale's appropriate date representation
	 */
	case 'x':
	    if(modf) {	/* locale's alternative date representation */
	       rc = nl_langinfo((nl_item)ERA_D_FMT);
	       if(*rc)
	          return (do_time((unsigned char *)rc));
	       /* else get appropriate date representation */
	    }
	    return (do_time((unsigned char *) nl_langinfo((nl_item)D_FMT)));

	/*
	 *   %X - locale's appropriate time representation
	 */
	case 'X':
	    return (do_time((unsigned char *) nl_langinfo((nl_item)T_FMT)));

	/*
	 *   %U - week number of the year (Sunday 1st day of week)
	 */
	case 'U':
	    fld->type  = NUMERIC;
	    return (_itoa((timest->tm_yday + 7 - timest->tm_wday) / 7, 2, fld));

	/*
	 *   %W - week number of the year (Monday 1st day of week) [00-53]
	 */
	case 'W':
	    fld->type  = NUMERIC;
	    return (_itoa((timest->tm_yday + 7 - (timest->tm_wday ? (timest->tm_wday - 1) : 6)) / 7, 2, fld));

	/*
	 *   %V - week number of the year (Monday 1st day of week) [01-53]
	 *   if 1st week has >=4 days in the new year, it is week one, else
	 *   it is week 53 of previous year.
	 */
	case 'V':
	    fld->type  = NUMERIC;
				/* first normalize to monday of current week */
	    i = (timest->tm_yday - (timest->tm_wday - 1));
	    fweek = (i%7 > 0) ? 1 : 0;	/* fweek = 0 if the year starts on
					 * a monday */
	    ifsun = (timest->tm_wday == 0) ? 1 : 0;  /* if day is a sunday */

	    fweek = (i%7 >= 4) ? 1 : 0; /* fweek = 1, if week one has 4 or more
					 * days in the new year */
	    if(timest->tm_yday < 4 && !fweek) return(_itoa(53, 2, fld));
	    return (_itoa((i/7 + 1) + fweek - ifsun, 2, fld));

	default:						/* invalid fmt character */
	    if (!__nl_derror)
		__nl_derror = fmt;
	    return (E_BAD_SPEC);
    }

    if (buf >= bufend) {					/* out of room? */
	buf--;
	if (!__nl_derror)
	    __nl_derror = fmt;
	return (E_BUF_SIZE);
    } else
	return 0;
}

/********************* GENERAL PURPOSE FUNCTIONS ***********************/


static	_atoi(str, integer)
register unsigned char	*str;
register int		*integer;
{
    register unsigned char *base = str;

    *integer = 0;
    while (CHARAT(str) >= '0' && CHARAT(str) <= '9') {
	*integer *= 10;
	*integer += *str++ - '0';
    }
    return(str-base);
}

/* Note: ALT_DIGIT support: This is HP-UX specific for Arabic only. If ALT_DIGIT exists, it is assumed
 * to be for arabic (8 bit characters). If ALT_DIGIT exists, that is the default digit set for Arabic
 * and we want to print that! (Talk to Eric Wendelboe at NSG)
 * the "alt_digits" used here refers to this!!
 */

static	_itoa(anint, digits, fld)
register int		anint;
register int		digits;					/* default fld width.prec = digits.digits */
register struct fld	*fld;
{
    register int	div;

    if (anint < 0) {						/* if a negative number			*/
	if (buf >= bufend)
	    return (E_BUF_SIZE);
	*buf++ = alt_digits ? _nl_dgt_alt[12]			/* copy out appropriate minus sign	*/
			    : (unsigned char)'-';
	anint = -anint;						/* convert to positive and continue	*/
    }

    if (anint < 0 || anint > 1000000)				/* quit if number out of range		*/
	return (E_BAD_NUMBER);

    if (fld->width == NOVAL && fld->precision == NOVAL)		/* if no field spec given,		*/
	for (div=1; --digits; div*=10);				/* use default field width.precision	*/
    else
	div = 1;						/* otherwise, start with small spec	*/

    for (; anint/(div*10); div *= 10);				/* grow to size of full number		*/

    while (div) {
	if (buf >= bufend)
	    return (E_BUF_SIZE);
	*buf++ = alt_digits ? _nl_dgt_alt[anint / div]
			    : (unsigned char) (anint / div) + '0';
	anint %= div;
	div /= 10;
    }
    return (0);
}


static	x_strcpy(from)
register unsigned char	*from;
{
    if (!from)							/* nothing to copy? */
	return (0);

    while (*from) {
	if (buf >= bufend)
	    return (E_BUF_SIZE);
	*buf++ = *from++;
    }

    return (0);
}


/******************* PRINTF-LIKE FIELD WIDTH/PRECISION *******************

    The following code will duplicate printf's handling of field
    width/precision specifications, including full support of the
    ".", "-", and numeric format specifications.

    In conjunction with the "fld" structure definition and defines at
    the beginning of this program, this section can be extracted and
    used where necessary.  The user should:

	1)  Detect a "%" or equivalent in a format string.

	2)  Call "fld_spec()" with a pointer just past the "%".

	3)  When "fld_spec()" returns a pointer "bumped" past
	    any field specification, the caller should determine
	    whether the rest of the specifier is a number
	    (%d) or a string (%s), setting fld->type accordingly.

	4)  Once the caller has read/transformed the parameterized
	    date, he should call "fld_adjust()" to implement the
	    field specifications detected by "fld_spec()".

*/


/*

    fld_spec() determines if the next characters in a format string are
    field-width specification characters.  It returns a pointer to the
    first character beyond the field specification, or NULL if invalid
    syntax was detected.

    fld_spec() expects a "fmt" pointer aimed at the beginning of a field
    specification, and knows nothing about "%" or and other format
    characters except ".", "-" and digits.

    SYNTAX:

	unsigned char *fld_spec(fmt, fld)
	unsigned char *fmt;		-> format string's field specification
	struct fld *fld;		-> field details

	return(#)			-> past the format field specification, if any
	return(NULL)			:  invalid field specification syntax

*/

static unsigned char *fld_spec(fmt, fld)
register unsigned char	*fmt;
register struct fld	*fld;
{
    register unsigned int	c;
    register int		dot = FALSE;

    fld->type = fld->leadzero = fld->flush = 0;
    fld->width = fld->precision = NOVAL;

    while (c = CHARAT(fmt)) {
	if (c == '-') {
	    if (!fld->flush && !dot)
		fld->flush = LEFT;
	    else
		goto punt;
	}
	else if (c == '.') {
	    if (!dot)
		dot = TRUE;
	    else
		goto punt;
	}
	else if (c>= '0' && c<='9') {
	    if (dot) {
		if (fld->precision == NOVAL)
		    fmt += _atoi(fmt, &fld->precision) - 1;
		else
		    goto punt;
	    }
	    else if (fld->width == NOVAL) {
		if (c == '0')
		    fld->leadzero = TRUE;
		fmt += _atoi(fmt, &fld->width) - 1;
	    }
	    else
		goto punt;
	}
	else
	    break;
	ADVANCE(fmt);
    }
    return(fmt);						/* valid return */

punt:								/* error return */
    if (!__nl_derror)
	__nl_derror = CHARAT(fmt);
    return(NULL);
}



/*
    fld_adjust() will move a buffer left or right, with spaces or
    zeros depending on the caller's "fld" structure.

    SYNTAX:

	unsigned char *fld_adjust(base, fld)
	unsigned char *base;		-> start of field to be shifted
	struct fld *fld;		-> field width/precision details

	return(#)			-> success or fail

*/

static int fld_adjust(base, fld)
unsigned char	*base;
struct fld	*fld;
{
    if (fld->type == STRING)
	return(str_adjust(base, fld));
    else
	return(num_adjust(base, fld));
}



/*
    str_adjust() will left/right adjust a field containing text
    characters -- a string, although no '\0' delimiter is presumed.
    Text characters may be dropped if the precision value in
    "fld" is too small for "buf - base" text bytes, but only
    characters will be truncated.  A 16 bit character will not
    be split in half.

    SYNTAX:

	unsigned char *str_adjust(base, fld)
	unsigned char *base;		-> start of field to be shifted
	struct fld *fld;		-> field width/precision details

	return(#)			-> success or fail
*/

static int str_adjust(base, fld)
register unsigned char	*base;
register struct fld	*fld;
{
    register int		bufsize = buf - base;
    register unsigned char	*endbuf = buf - 1;
    register unsigned char	*tmpstart, *tmpend;
    register int		fill_count, j;
    register unsigned char	fill  = ' ';
    register int		chars, width;

    if (fld->width == NOVAL && fld->precision == NOVAL)
	return (1);

    /*	if a precision was specified, it will determine the actual
	number of bytes which get printed
    */
    if (fld->precision != NOVAL) {
	if (fld->precision == 0) {	/* printf-like behavior */
	    buf = base;
	    return (1);
	}
	chars = fld->precision;
	if (chars > bufsize)
	    chars = bufsize;
    }
    else
	chars = bufsize;

    /*  16 bit languages need special handling to insure that
	we don't truncate part of a 16 bit character; as
	with printf, we'll reduce the precision count by one
	rather than chop up a two byte character.
	this should be in effect whether or not fld->precision != NOVAL
     */

    if (__nl_char_size > 1)	{
	tmpstart = base;
	tmpend   = &base[chars];
	for (;;) {
		if (tmpstart < tmpend)
		    CHARADV(tmpstart);
		else {
		    if (tmpstart > tmpend)
			--chars;
		    break;
		}
	}
    }

    /*	if a width was specified, it will determine the total width
	into which the field's characters must fit
    */
    if (fld->width != NOVAL) {

	if (chars > fld->width)
	    width = chars;
	else
	    width = fld->width;
    }
    else
	width = chars;

    if (width > chars)
	fill_count = width - chars;				/* amount of fill necessary */
    else {
	buf = base + chars;					/* no fill necessary; truncate to precision */
	return (1);						/* and return */
    }
    
    if (base + width >= bufend)					/* won't fit in the output buffer */
	return (0);

    if (fld->leadzero && fld->flush != LEFT)
	fill = alt_digits ? _nl_dgt_alt[0] : '0';

    if (fld->flush == LEFT) {
	endbuf = &base[chars];
	while (fill_count--)
	    *endbuf++ = (unsigned char) fill;
    }
    else {
	bufright(base, chars, fill_count);
	for (j=0; j<fill_count; j++)
	    base[j] = (unsigned char) fill;
	endbuf = base + fill_count + chars;
    }
    buf = endbuf;
    return (1);
}


static int num_adjust(base, fld)
register unsigned char	*base;
register struct fld	*fld;
{
    register int space		= ' ';
    register int spaces		= 0;				/* number of spaces to add for full width */
    register int zeros		= 0;				/* number of leading zeros to add */
    register int bufsize	= buf - base;

    if (fld->precision != NOVAL) {
	zeros = fld->precision - bufsize;
	if (zeros < 0)
	    zeros = 0;
    }

    if (fld->width != NOVAL) {
	spaces = fld->width - bufsize - zeros;
	if (spaces < 0)
	    spaces = 0;
	if (fld->leadzero && fld->flush != LEFT)
	    space = alt_digits ? _nl_dgt_alt[0] : '0';
    }

    if (base + bufsize + zeros + spaces > bufend)		/* won't fit in output buffer */
								/* Fixes SR 5003-136721 */
	return (0);

    if (fld->flush == LEFT) {
	bufright(base, bufsize, zeros);
	while (zeros-- > 0)
	    *base++ = alt_digits ? _nl_dgt_alt[0] : '0';
	base += bufsize;
	while (spaces-- > 0)
	    *base++ = (unsigned char) space;
    }
    else {
	bufright(base, bufsize, zeros + spaces);
	while (spaces-- > 0)
	    *base++ = (unsigned char) space;
	while (zeros-- > 0)
	    *base++ = alt_digits ? _nl_dgt_alt[0] : '0';
	base += bufsize;
   }
   buf = base;
   return (1);
}

static void bufright(base, buflen, count)
register unsigned char	*base;
register int		buflen;
register int		count;
{
    register unsigned char *from = &base[buflen] - 1;
    register unsigned char *to	= from + count;

    while (buflen--)
	*to-- = *from--;
}


/********************* Era/Emperor Time Support ***********************/

/*
    This function examines successive era data structures until it finds
    one which fits the date specified in the caller's "tm" structure, or
    the _nl_era[] list of era data structures is exhausted.
*/

static int era_fmt(fld, fmt)
struct fld	*fld;
unsigned char	fmt;						/* type of format wanted (%E, %N, %o)	*/
{
    register struct _era_data	**era;
    register int		year = timest->tm_year + 1900;
    register int		month = timest->tm_mon + 1;

    /* if we don't already know the era from a previous formatting request */
    if (!last_era) {

	/* try to match the time against every era in the list */
	for (era=_nl_era; *era; era++) {
	    register int	start_year = (*era)->start_year;
	    register int	end_year = (*era)->end_year;

	    if ((start_year != SHRT_MIN && year < start_year)	/* year < lower limit (if one exists)	*/
		|| (end_year != SHRT_MAX && year > end_year))	/* year > upper limit (if one exists)	*/
		continue;
	    
	    if (start_year != SHRT_MIN && year == start_year &&	/* in the first year of the era		*/
		    (	(month < (*era)->start_month)		/*    but before the first month	*/
		    ||	(month == (*era)->start_month		/*    or in the first month		*/
			&& timest->tm_mday < (*era)->start_day)	/*       but before the first day	*/
		    )
		)
		continue;
	    
	    if (end_year != SHRT_MAX && year == end_year &&	/* in the last year of the era		*/
		    (	(month > (*era)->end_month)		/*    but after the last month		*/
		    ||	(month == (*era)->end_month		/*    or in the last month		*/
			&& timest->tm_mday > (*era)->end_day)	/*       but after the last day		*/
		    )
		)
		continue;

	    break;						/* date is within era			*/
	}

	if (!*era)						/* time given not part of any era	*/
	    return 0;						/* return no errors (but /wo any output) */
    
	last_era = *era;					/* save for multiple %E/%N/%o fmt calls	*/
    }

    switch (fmt) {						/* handle each format separately	*/

    case 'E':							/* %E - combined name and year		*/
	if (last_era->format && *last_era->format)		/*      if it exists,			*/
	    return (do_time(last_era->format));			/*      use special format for this era	*/
	else							/*      otherwise, use default		*/
	    return (do_time((unsigned char *) nl_langinfo((nl_item)ERA_FMT)));

    case 'N':							/* %N - name				*/
	return x_strcpy(last_era->name);

    case 'o':							/* %o - year				*/
	if (fld->width == NOVAL && fld->precision == NOVAL)	/*      default to a simple number	*/
		fld->width = fld->precision = 1;

	/*
	 * era year relative to origin, adjusted pos/neg per
	 * era specification, and plus offset for first year
	 */
	year = (year - last_era->origin_year) * last_era->signflag + last_era->offset;

	return (_itoa(year, 4, fld));

    default:							/* when era_fmt is called with 'X' */
	return(0);

    }
    /*NOTREACHED*/
}

/* Modifiers are allowed for these conversion specifiers only */
static char *Estr = "cCxyY";
static char *Ostr = "deHImMSUwWy";

static int check_modifier(fmt)
unsigned char *fmt;
{
    switch(*fmt++) {
       case 'E':
	   return(valid_str(*fmt, Estr));
       case 'O':
	   return(valid_str(*fmt, Ostr));
    }
    return(0);			/* no modifiers present */
}

static int valid_str(ch, str)
unsigned char ch, *str;
{
    if(ch == '\0') return(0);
    while(*str) 
       if(ch == *str++) return(1);
    return(0);
}
