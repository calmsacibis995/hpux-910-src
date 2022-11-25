/*
 *  @(#) $Revision: 72.2 $
 *
 *  Routine name: getdate
 *
 *  getdate is used to convert user-definable date and/or time
 *  format specifications into a tm structure (see <sys/time.h>).
 *  See also: getdate(3c) manual entry.
 */
/*LINTLIBRARY*/

/*
 *  Local functions
 */

static int  is_valid_tm();
#ifdef DEBUG
static void getdate_print_tm();
#endif /* DEBUG */
static int  process_fmt();
static int  strntoi();
static void tm_resolve();

/*
 *  Lines added to clean up ANSI/POSIX namespace
 */

#ifdef _NAMESPACE_CLEAN
#  ifdef DEBUG
#    define daylight __daylight	/* from tzset() */
#    define printf __printf
#  endif
#  ifdef __lint
#    define ferror _ferror
#  endif
#  define fgets _fgets
#  define fopen _fopen
#  define fclose _fclose
#  define getdate _getdate
#  define getenv _getenv
#  ifdef __lint
#    define isdigit _isdigit
#    define isspace _isspace
#  endif
#  define localtime _localtime
#  ifdef _ANSIC_CLEAN
#    define malloc _malloc
#    define free _free
#  endif /* _ANSIC_CLEAN */
#  define mktime _mktime
#  define nl_langinfo _nl_langinfo
#  define stat _stat
#  define strcpy _strcpy
#  define strlen _strlen
#  define strncasecmp _strncasecmp
#  define strncmp _strncmp
#  define sysconf _sysconf
#  define time _time
#  ifdef DEBUG
#    define timezone __timezone	/* from tzset() */
#  endif
#  define tzname __tzname	/* from tzset() */
#endif /* _NAMESPACE_CLEAN */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <langinfo.h>
#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 *  Miscellaneous defines
 */

#if defined(NLS) || defined(NLS16)
#  include <nl_ctype.h>
#else
#  define CHARAT(p)  (*p)
#  define ADVANCE(p) (++p)
#endif

#ifndef NULL
#  define NULL	0
#endif

#ifndef TRUE
#  define TRUE	1
#endif

#ifndef FALSE
#  define FALSE	0
#endif

#ifndef LINE_MAX
#  define _MY_LINE_MAX	2048
#else
#  define _MY_LINE_MAX	LINE_MAX
#endif

#ifndef _POSIX_TZNAME_MAX
#  define _MY_TZNAME_MAX  3
#else
#  define _MY_TZNAME_MAX _POSIX_TZNAME_MAX
#endif

#ifndef __STDC__
#  define const
#endif

/*
 *  Global variables  BEGIN
 */

#ifdef _NAMESPACE_CLEAN
#undef getdate_err
#pragma _HP_SECONDARY_DEF _getdate_err getdate_err
#define getdate_err _getdate_err
#endif

/*
 *  This is getdate()'s errno.  See "getdate_err values" below.  This is
 *  initialized here because the support for secondary definitions is
 *  only good on initialized data and will not work on BSS data items.
 */

int getdate_err = 0;

/*
 *  return_tm is the tm structure whose address will actually be
 *  returned.  It is declared global so callers of getdate can access
 *  the info after the function returns.
 */

static struct tm return_tm;

/*
 *  current_tm is the tm structure that contains the current date
 *  Made it global so that tm_year information would be available
 *  in process_fmt during the processing of the %y variable.
 */

static struct tm *current_tm;

/*
 *  is_AM, is_PM, and need_AM_or_PM are indicators used to tell whether
 *  an AM/PM specification was supplied (is_*), and whether an AM/PM
 *  spec should have been supplied.  It is considered an invalid input
 *  specification if %I is used without %p.  These variables are set in
 *  process_fmt() and referenced in is_valid_tm().
 */

static int is_AM;
static int is_PM;
static int need_AM_or_PM;

/*
 *  Global variables  END
 */

/*
 *  getdate_err values
 */

#define _GETDATE_DATEMSK_NULL_OR_UNDEF	1
#define _GETDATE_DATEMSK_CANT_OPEN	2
#define _GETDATE_DATEMSK_CANT_STAT	3
#define _GETDATE_DATEMSK_NOT_REG_FILE	4
#define _GETDATE_DATEMSK_READ_ERR	5
#define _GETDATE_MALLOC_FAILED		6
#define _GETDATE_NO_MATCHING_TEMPLATE	7
#define _GETDATE_INVALID_INPUT_SPEC	8

/*
 *  These macros come from ctime.c and are used for indexing into the
 *  tzname[] array.  See tzset description in ctime(3c) for more
 *  information about tzname[].
 */

#define _STD_NAME_INDEX     0
#define _DST_NAME_INDEX     1

/*
 *  Lines added for general namespace clean up
 */

#ifdef _NAMESPACE_CLEAN
#undef getdate
#pragma _HP_SECONDARY_DEF _getdate getdate
#define getdate _getdate
#endif


struct tm *
getdate(input_string)
const char *input_string;
{
	/*
	 *  Parameters:
	 *	input_string - points to string containing the input
	 *		       date and/or time specification.
	 *
	 *  If input_string is a NULL pointer, take care of it in the
	 *  process_fmt() routine below.   The V.4 system still seems to
	 *  go through all the intervening work because, for example, if
	 *  called with an NULL pointer input_string and a non-existent
	 *  DATEMSK file, it will return '2' (the template file could
	 *  not be open for reading).
	 *
	 *  If called with a NULL pointer input_string, the V.4 behavior
	 *  is to return '7' (there is no line in the template that
	 *  matches the input).
	 */

	/*
	 *  Local variables
	 */

	FILE *file_ptr = (FILE *) NULL;

	char *datemsk = (char *) NULL;
	char *fmt_buf = (char *) NULL;
	char *format  = (char *) NULL;
	char *string  = (char *) NULL;
	char *string_orig  = (char *) NULL;	/* Save ptr for free() later */

	int buffer_size	= 0;
	int match	= FALSE;
	
	time_t time_val = time((time_t *) NULL);

	struct stat stat_struct;

	/*
	 *  Initialization
	 */

	errno = 0;
	getdate_err = 0;
	is_AM = FALSE;
	is_PM = FALSE;
	need_AM_or_PM = FALSE;
	current_tm = localtime(&time_val);
	return_tm.tm_sec   = -1;
	return_tm.tm_min   = -1;
	return_tm.tm_hour  = -1;
	return_tm.tm_mday  = -1;
	return_tm.tm_mon   = -1;
	return_tm.tm_year  = -1;
	return_tm.tm_wday  = -1;
	return_tm.tm_yday  = -1;
	return_tm.tm_isdst = -1;

#ifdef DEBUG
	getdate_print_tm(return_tm, "Initialized return_tm contains:");
	getdate_print_tm(*current_tm, "Local time is (localtime()):");
	(void) printf("tzname[_STD_NAME_INDEX] is: \"%s\"\n",
		      tzname[_STD_NAME_INDEX]);
	(void) printf("tzname[_DST_NAME_INDEX] is: \"%s\"\n",
		      tzname[_DST_NAME_INDEX]);
	(void) printf("timezone  is: \"%d\"\n", timezone);
	(void) printf("daylight  is: \"%d\"\n", daylight);
#endif /* DEBUG */

	/*
	 *  Is DATEMSK in the environment?  If so, is it non-null?
	 */

	if (((datemsk = getenv("DATEMSK")) == (char *) NULL) ||
	    (CHARAT(datemsk) == '\0'))
	{
	    /*
	     *  No, set getdate_err and return.
	     */

	    getdate_err = _GETDATE_DATEMSK_NULL_OR_UNDEF; /* value: 1 */
	    return (struct tm *) NULL;
	}
	else if ((file_ptr = fopen(datemsk, "r")) == (FILE *) NULL)
	{
	    /*
	     *  Cannot open DATEMSK for reading, set
	     *  getdate_err and return.
	     */

	    getdate_err = _GETDATE_DATEMSK_CANT_OPEN;	/* value: 2 */
	    return (struct tm *) NULL;
	}

	/*
	 *  NOTE: From this point on, we must close the DATEMSK file before
	 *        returning.
	 */

	else if (stat(datemsk, &stat_struct) != 0)
	{
	    /*
	     *  Stat failed, set getdate_err and return.
	     */

	    getdate_err = _GETDATE_DATEMSK_CANT_STAT;	/* value: 3 */
	    (void)fclose(file_ptr);
	    return (struct tm *) NULL;
	}
	else if (S_ISREG(stat_struct.st_mode) != 1)
	{
	    /*
	     *  DATEMSK is not a regular file, set
	     *  getdate_err and return.
	     */

	    getdate_err = _GETDATE_DATEMSK_NOT_REG_FILE; /* value: 4 */
	    (void)fclose(file_ptr);
	    return (struct tm *) NULL;
	}

	/*
	 *  Get text file maximum line length.  It is used to determine
	 *  how big a buffer to use for both the format strings and the
	 *  input string.  If sysconf() doesn't work, substitute another
	 *  value.
	 */

	buffer_size = (int) sysconf(_SC_LINE_MAX);
	if (buffer_size == -1)
	    buffer_size = _MY_LINE_MAX;

	/*
	 *  Allocate space for the format and input string buffers
	 */

	fmt_buf = (char *) malloc((size_t) buffer_size);
	string_orig = string = (char *) malloc((size_t) buffer_size);

	if ((fmt_buf == (char *) NULL) || (string == (char *) NULL))
	{
	    /*
	     *  Malloc failed, set getdate_err and return.
	     */

	    getdate_err = _GETDATE_MALLOC_FAILED;	/* value: 6 */
	    (void)fclose(file_ptr);
	    if (fmt_buf != (char *) NULL)
		free(fmt_buf);
	    if (string_orig != (char *) NULL)
		free(string_orig);
	    return (struct tm *) NULL;
	}

	/*
	 *  At this point, the file is opened for reading and
	 *  buffer space is allocated -- time to get to work.
	 *
	 *  NOTE: From this point on, we must free `fmt_buf' and `string_orig'
	 *        before returning.
	 */

	while ((match == FALSE) &&
	       ((format = fgets(fmt_buf, buffer_size, file_ptr)) != 
		  (char *) NULL))
	{

	    /*
	     *  fgets returns the \n, too.
	     */

	    *(format + strlen(format) - 1) = '\0';

	    (void) strcpy(string, input_string);

#ifdef DEBUG
	    (void) printf("Looking at format: \"%s\"\n", format);
#endif
	    match = process_fmt(format, &string,
				&return_tm, (int) FALSE);
	}

	/*
	 *  Check:
	 *	- whether a read error happened (fgets returns
	 *	  a NULL pointer both when EOF is reached and
	 *	  when a read error occurs, so use ferror);
	 *	- whether EOF has been reached and a matching
	 *	  format specification not found.
	 */

	if ((format == (char *) NULL) && (ferror(file_ptr) != 0))
	{
	    /*
	     *  An error occurred while reading the file.
	     *  Set getdate_err and return.
	     */

	    getdate_err = _GETDATE_DATEMSK_READ_ERR;	/* value: 5 */
	    (void)fclose(file_ptr);
	    if (fmt_buf != (char *) NULL)
		free(fmt_buf);
	    if (string_orig != (char *) NULL)
		free(string_orig);
	    return (struct tm *) NULL;
	}
	else if (match == FALSE)
	{
	    /*
	     *  A matching format specification was not found in
	     *  the DATEMSK file.  Set getdate_err and return.
	     */

	    getdate_err = _GETDATE_NO_MATCHING_TEMPLATE;  /* value: 7 */
	    (void)fclose(file_ptr);
	    if (fmt_buf != (char *) NULL)
		free(fmt_buf);
	    if (string_orig != (char *) NULL)
		free(string_orig);
	    return (struct tm *) NULL;
	}
	else
	{

#ifdef DEBUG
	    getdate_print_tm(return_tm,
		     "After process_fmt(), before tm_resolve():");
#endif /* DEBUG */

	    /*
	     * Here when we think we have a valid matching format
	     * specification.  First, call tm_resolve to fill in any
	     * remaining tm structure fields.
	     */

	    tm_resolve(&return_tm, current_tm);

#ifdef DEBUG
	    getdate_print_tm(return_tm,
		     "After tm_resolve(), before is_valid_tm():");
#endif /* DEBUG */

	    /*
	     *  Now, call is_valid_tm() to verify that the tm
	     *  structure contains valid information.
	     */

	    if (is_valid_tm(&return_tm) != TRUE)
	    {
		getdate_err = _GETDATE_INVALID_INPUT_SPEC;  /* value: 8 */
	        (void)fclose(file_ptr);
	        if (fmt_buf != (char *) NULL)
		    free(fmt_buf);
	        if (string_orig != (char *) NULL)
		    free(string_orig);
		return (struct tm *) NULL;
	    }
	}

        (void)fclose(file_ptr);
	if (fmt_buf != (char *) NULL)
	    free(fmt_buf);
	if (string_orig != (char *) NULL)
	    free(string_orig);
	return &return_tm;

} /* function getdate */

/*
 *  This function is responsible for processing the format and input
 *  strings by walking through each a character at a time and attempting
 *  to match field descriptors from the format string with text from the
 *  input string.  For such matches, the appropriate fields in the
 *  provided tm structure are filled in.  This function also has a
 *  special way of handling whitespace in the strings (see comments
 *  below).  process_fmt() may be recursively called in the cases where
 *  a single descriptor can expand into a string of several descriptors
 *  (%c, %D, %r, %R, %T, %x, %X).  The routine has a different behavior
 *  upon return in such cases, hence the "recurse" control variable.
 *  The recursion is supposed to be (at most) only one level
 *  because field descriptor expansions are not supposed to result in
 *  descriptors that again need expansion.
 */

static int
process_fmt(fmt, str, tm_s, recurse)
char *fmt;
char **str;
struct tm *tm_s;
int recurse;
{
	/*
	 *  Parameters:
	 *
	 *	fmt     - points to the format string read from the
	 *		  DATEMSK file
	 *	str     - points to the input string supplied by the
	 *		  user
	 *	tm_s    - points to the tm structure used to pass the
	 *		  time back to the user
	 *	recurse - control variable used to indicate whether this
	 *		  is a recursive call (TRUE) or not (FALSE)
	 */

	/*
	 *  Local variables
	 */

	char name_AM[32];
	char name_PM[32];
	char *c_p  = (char *) NULL;
	char *name = (char *) NULL;
	char *str_ptr = (char *) NULL;
	char *sub_fmt = (char *) NULL;
	int i = 0;
	int match_so_far = TRUE;
	int name_len = 0;
	int name_len_AM = 0;
	int name_len_PM = 0;
	int number = 0;

	/*
	 *  These two variables are used to implement a one-character
	 *  lookahead in the format string.
	 */

	char *fmt_tmp = (char *) NULL;
	int next_fmt_char = 0;

	/*
	 *  fmt should never be NULL (because we check the return
	 *  value of fgets()), but str certainly could point to a NULL
	 *  string.
	 */

	if ((fmt == (char *) NULL) || (*str == (char *) NULL))
	    return FALSE;

	while ((match_so_far == TRUE) &&
	       ((CHARAT(fmt) != '\0') && (CHARAT(*str) != '\0')))
	{
	    /*
	     *  Here's what it seems like V.4 getdate() does w.r.t.
	     *  whitespace and the %t and %n specifications:
	     *
	     * 		1) If a %t or %n is found in the format string,
	     *		   a \t or \n (respectively) must occur in
	     *		   the input string, otherwise the format string
	     *		   will be rejected.  For example:
	     *		
	     *		   	"%A%t%B %d"
	     *
	     *		   matches ("[ \t]*" means zero or more blanks
	     *		   or tabs):
	     *
	     *			"[ \t]*Friday\t[ \t]*June[ \t]*X[ \t]*"
	     *				      ^
	     *				      |--- this tab must occur
	     *
	     *		   (For the purpose of these examples, let X be
	     *		    a valid Friday date in the "next" month of
	     *		    June.)
	     *
	     *		2) A \t or \n in the input string does not
	     *		   necessarily require the presence of a %t or
	     *		   %n (respectively) in the format line.  For
	     *		   example:
	     *
	     *			"%A%B%d"
	     *
	     *		   matches:
	     *
	     *			"[ \t]*Friday[ \t]*June[ \t]*X[ \t]*"
	     *
	     *		3) A space character (octal 040) in the format
	     *		   string does not require a matching space
	     *		   character in the input string (and
	     *		   vice-versa).  For example:
	     *
	     *			"%A %B %d"
	     *
	     *		   matches:
	     *
	     *			"FridayJuneX"
	     *
	     *		   and:
	     *
	     *			"just plain text" (in DATEMSK file)
	     *
	     *		   matches:
	     *
	     *			"justplaintext"   (user input)
	     *
	     *  First, skip any whitespace in the format string.
	     */

	    while (isspace(*fmt))
		ADVANCE(fmt);

	    /*
	     *  If a format specifier of '%t' or '%n' is immediately
	     *  ahead in the format string, don't do anything with
	     *  input string whitespace - we need to handle it in a
	     *  special way below (see "case 'n'" and "case 't'"
	     *  below).  Otherwise, skip any whitespace that occurs
	     *  at this point in the input string.
	     *
	     *  Wanted to do:
	     *
	     *      if (CHARAT(fmt) != '%' || 
	     *          (CHARAT(fmt+1) != 't' && CHARAT(fmt+1) != 'n'))
	     *
	     *  but was not sure if that was acceptable CHARAT() usage.
	     */

	    fmt_tmp = fmt;

	    if (CHARAT(fmt_tmp) != '\0')
		ADVANCE(fmt_tmp);

	    next_fmt_char = CHARAT(fmt_tmp);

	    if ((CHARAT(fmt) != '%') ||
		((next_fmt_char != 't') && (next_fmt_char != 'n')))
	    {
		while (isspace(**str))
		    ADVANCE(*str);
	    }

	    /*
	     *  At this point, we should either:
	     *      CASE 1) be sitting at a '%' in the format string; or
	     *      CASE 2) be at matching chars in both strings; or
	     *      CASE 3) know we don't match.
	     *
	     *  match_so_far was initialized to TRUE to get into this
	     *  while loop for the first time.  Now, set it to FALSE -
	     *  it's up to the rest of the code to show otherwise.
	     */

	    match_so_far = FALSE;

	    if (CHARAT(fmt) == '%')
	    {
		/*
		 *  CASE 1 applies.
		 *
		 *  Look at the next character.
		 */

		ADVANCE(fmt);

		switch (CHARAT(fmt))
		{

		    case '%' :

			/*
			 *  Match a '%' character.
			 */

			if (CHARAT(*str) == '%')
			{
			    /*
			     *  Consume the matched characters from both
			     *  the input and format strings.
			     */

			    ADVANCE(fmt);
			    ADVANCE(*str);

			    match_so_far = TRUE;
			}

			break;

		    case 'a' :

		    	/*
		    	 *  Match abbreviated weekday name.
			 *
			 *  (Taking match_so_far out of the "for" test
			 *   and adding a break statement after setting
			 *   match_so_far to TRUE might be faster, but
			 *   I'll go with the structured approach.
			 *
			 *   It would also be faster (in some cases) to
			 *   populate char *abday[], *abmon[], *day[],
			 *   *mon[] arrays to avoid needless repetitive
			 *   nl_langinfo calls every time a %a, %b, %A,
			 *   or %B (respectively) descriptor was
			 *   encountered.)
		    	 */

			for (i = ABDAY_1;
			     (i <= ABDAY_7) && (match_so_far == FALSE);
			     i++)
			{
			    name = nl_langinfo(i);
			    name_len = strlen(name);

			    /*
			     *  nl_langinfo() may return a pointer to an
			     *  empty string.  If that's the case, a
			     *  match will happen here since a string
			     *  comparison of 0 bytes yields equality.
			     *  However it's a pretty safe bet that full
			     *  and abbreviated day (%A, %a) and month
			     *  (%B, %b) names will exist (for all 7
			     *  days and 12 months) for a locale.
			     */

			    if (strncasecmp(*str, name,
					    (size_t) name_len) == 0)
			    {
				tm_s->tm_wday = i - ABDAY_1;

		    		/*
				 *  Consume the abbreviated weekday
				 *  name.
				 */

				for (i = 0; i < name_len; i++)
				    ADVANCE(*str);

				ADVANCE(fmt);

				match_so_far = TRUE;
			    }
			} /* for */

		    	break;

		    case 'A' :

		    	/*
		    	 *  Match full weekday name.
		    	 */

			for (i = DAY_1;
			     (i <= DAY_7) && (match_so_far == FALSE);
			     i++)
			{
			    name = nl_langinfo(i);
			    name_len = strlen(name);

			    if (strncasecmp(*str, name,
					    (size_t) name_len) == 0)
			    {
				tm_s->tm_wday = i - DAY_1;

				/*
				 *  Consume the weekday name.
				 */

				for (i = 0; i < name_len; i++)
				    ADVANCE(*str);

				ADVANCE(fmt);

				match_so_far = TRUE;
			    }

			} /* for */

		    	break;

		    case 'b' :

		    	/* Fall through */

		    case 'h' :

		    	/*
		    	 *  Match abbreviated month name.
		    	 */

			for (i = ABMON_1;
			     (i <= ABMON_12) && (match_so_far == FALSE);
			     i++)
			{
			    name = nl_langinfo(i);
			    name_len = strlen(name);

			    if (strncasecmp(*str, name,
					    (size_t) name_len) == 0)
			    {
				tm_s->tm_mon = i - ABMON_1;

				/*
				 *  Consume the abbreviated month
				 *  name.
				 */

				for (i = 0; i < name_len; i++)
				    ADVANCE(*str);

				ADVANCE(fmt);

				match_so_far = TRUE;
			    }

			} /* for */

			break;

		    case 'B' :

			/*
			 *  Match full month name.
			 */

			for (i = MON_1;
			     (i <= MON_12) && (match_so_far == FALSE);
			     i++)
			{
			    name = nl_langinfo(i);
			    name_len = strlen(name);

			    if (strncasecmp(*str, name,
					    (size_t) name_len) == 0)
			    {
				tm_s->tm_mon = i - MON_1;

				/*
				 *  Consume the full month name.
				 */

				for (i = 0; i < name_len; i++)
				    ADVANCE(*str);

				ADVANCE(fmt);

				match_so_far = TRUE;
			    }

			} /* for */

		    	break;

		    case 'c' :

		    	/*
		    	 *  Match locale's appropriate date and time
			 *  representation.
		    	 *  For example: "%a %b %d %H:%M:%S %Y"
		    	 */

			sub_fmt = nl_langinfo(D_T_FMT);

			if (strlen(sub_fmt) > 0)
			    match_so_far = process_fmt(sub_fmt, str,
						       tm_s,
						       (int) TRUE);

			if (match_so_far == TRUE)
			{
			    /*
			     *  Consume the format specifier from the
			     *  original format string.
			     */

			    ADVANCE(fmt);
			}

			break;
/*
 *		    case 'E' :
 *
 *			 *  Match locale's combined Emperor/Era name
 *			 *  and year.  Need also %N and %o.
 *			sub_fmt = nl_langinfo(ERA_FMT);
 *
 *			if (strlen(sub_fmt) > 0)
 *			    match_so_far = process_fmt(sub_fmt, str,
 *						       tm_s,
 *						       (int) TRUE);
 *
 *			if (match_so_far == TRUE)
 *			    ADVANCE(fmt);
 *
 *			break;
 */
		    case 'e' :

		    	/* Fall through */

		    case 'd' :

			/*
			 *  Match day of month as number (01 - 31).
			 */

			number = strntoi(*str, &str_ptr, 2);

			/*
			 *  If str and str_ptr point different places,
			 *  there must have been a number.
			 */

			if ((str_ptr != *str) &&
			    (1 <= number) && (number <= 31))
			{
				tm_s->tm_mday = number;

				/*
				 *  Consume the format specifier from
				 *  the original format string.  Also,
				 *  update the string pointer to what
				 *  strntoi() returned in its second
				 *  argument.
				 */

				ADVANCE(fmt);
				*str = str_ptr;

				match_so_far = TRUE;
			}

			break;

		    case 'D' :

			/*
			 *  Match date specified as "%m/%d/%y".
			 *
			 *  Lang "C" instance of %x.
			 */

			sub_fmt = "%m/%d/%y";
			match_so_far = process_fmt(sub_fmt, str,
						   tm_s, (int) TRUE);

			if (match_so_far == TRUE)
			    /*
			     *  Consume the format specifier from the
			     *  original format string.
			     */

			    ADVANCE(fmt);

			break;

		    case 'H' :

			/*
			 *  Match hour of day as number (00 - 23).
			 */

			number = strntoi(*str, &str_ptr, 2);
			
			if ((str_ptr != *str) &&
			    (0 <= number) && (number <= 23))
			{
			    tm_s->tm_hour = number;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 'I' :

			/*
			 *  Match hour of day as number (01 - 12).
			 *
			 *  This field may have to be adjusted later
			 *  if it is determined that the time is PM.
			 *  (See case for "%p".)
			 */

			number = strntoi(*str, &str_ptr, 2);

			if ((str_ptr != *str) &&
			    (1 <= number) && (number <= 12))
			{
			    tm_s->tm_hour = number;
			    need_AM_or_PM = TRUE;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 'm' :

			/*
			 *  Match month of year as number (01 - 12).
			 */

			number = strntoi(*str, &str_ptr, 2);
			
			if ((str_ptr != *str) &&
			    (1 <= number) && (number <= 12))
			{
			    /*
			     *  Must adjust the number to account for
			     *  fact that our range for months is 1-12,
			     *  while the tm_mon field has a range of
			     *  0-11.
			     */

			    tm_s->tm_mon = number - 1;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 'M' :

			/*
			 *  Match minute of hour as number (00 - 59).
			 */

			number = strntoi(*str, &str_ptr, 2);

			if ((str_ptr != *str) &&
			    (0 <= number) && (number <= 59))
			{
			    tm_s->tm_min = number;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 'n' :

			/*
			 *  Match a '\n' (newline) character.
			 *
			 *  Need to handle possible whitespace in the
			 *  input string - skip any whitespace as long
			 *  as it's not a '\n'.
			 */

			while (isspace(**str) && (CHARAT(*str) != '\n'))
			    ADVANCE(*str);

			if (CHARAT(*str) == '\n')
			{
			    /*
			     *  Consume the '\n'.
			     */

			    ADVANCE(fmt);
			    ADVANCE(*str);

			    match_so_far = TRUE;
			}

			break;

		    case 'p' :

			/*
			 *  Match locale's equivalent of either AM or
			 *  PM.
			 *
			 *  The following code deals with %p without
			 *  first making sure that the tm_hour field
			 *  adhered to the normal range for 12-hour time
			 *  (1-12).  When the tm_hour field is
			 *  out-of-range with respect to the 12-hour
			 *  clock, "_GETDATE_INVALID_INPUT_SPEC" should
			 *  be assigned to getdate_err.  I wanted to
			 *  maintain one place in the code where that
			 *  would happen, so chose not to return it
			 *  here.
			 */

			is_AM = FALSE;
			is_PM = FALSE;
			name_len = 0;

			name = nl_langinfo(AM_STR);
			(void) strcpy(name_AM, name);
			name_len_AM = strlen(name_AM);

			name = nl_langinfo(PM_STR);
			(void) strcpy(name_PM, name);
			name_len_PM = strlen(name_PM);

			if ((name_len_AM > 0) &&
			    (strncasecmp(*str, name_AM,
					(size_t) name_len_AM) == 0))
			{
			    name_len = name_len_AM;
			    is_AM = TRUE;
			}
			else if ((name_len_PM > 0) &&
				 (strncasecmp(*str, name_PM,
					     (size_t) name_len_PM) == 0))
			{
			    name_len = name_len_PM;
			    is_PM = TRUE;
			}
			
			/*
			 *  If at this point the tm_hour field has not
			 *  been set, set it to a value that will
			 *  guarantee that it is flagged as an invalid
			 *  input specification (there's a test in
			 *  is_valid_tm() that checks for (is_AM ||
			 *  is_PM) && tm_hour > 12).  You cannot use %p
			 *  without supplying the hour.  If an hour
			 *  specification appears later in the input
			 *  string, it will overwrite this value anyway.
			 */

			if ((tm_s->tm_hour == -1) &&
			    ((is_AM == TRUE) || (is_PM == TRUE)))
			{
			    tm_s->tm_hour = 13;
			}
			else if ((1 <= tm_s->tm_hour) &&
				 (tm_s->tm_hour <= 23))
			{
			    if (is_AM == TRUE)
			    {
				if (tm_s->tm_hour == 12)
				{
				    tm_s->tm_hour -= 12;
				}
			    }
			    else if (is_PM == TRUE)
			    {
				if (tm_s->tm_hour != 12)
				{
				    /*
				     *  This assignment could very well
				     *  cause "overflow" (for those
				     *  values between 13 and 23), but
				     *  that's the intent for when the
				     *  user mixes 24-hour time with a
				     *  "PM" designation, the result
				     */

				    tm_s->tm_hour += 12;
				}
			    }
			}

			/*
			 *  If a valid AM_STR or PM_STR was specified,
			 *  or if there is not the notion of AM/PM in
			 *  this locale, there's a match (in the latter
			 *  case, a match of no characters since
			 *  name_len is still 0).
			 */

			if (((is_AM == TRUE) || (is_PM == TRUE)) ||
			    ((name_len_AM == 0) && (name_len_PM == 0)))
			{
			    for (i = 0; i < name_len; i++)
				ADVANCE(*str);

			    ADVANCE(fmt);

			    match_so_far = TRUE;
			}

			break;

		    case 'r' :

			/*
			 *  Match time as "%I:%M:%S %p".
			 *
			 *  Lang "C" instance of "%X".
			 */

			sub_fmt = "%I:%M:%S %p";
			match_so_far = process_fmt(sub_fmt, str,
						   tm_s, (int) TRUE);

			if (match_so_far == TRUE)
			    ADVANCE(fmt);

			break;

		    case 'R' :

			/*
			 *  Match time as "%H:%M".
			 */

			sub_fmt = "%H:%M";
			match_so_far = process_fmt(sub_fmt, str,
						   tm_s, (int) TRUE);

			if (match_so_far == TRUE)
			    ADVANCE(fmt);

			break;

		    case 'S' :

			/*
			 *  Match seconds as number (00 - 59).
			 */

			number = strntoi(*str, &str_ptr, 2);

			if ((str_ptr != *str) &&
			    (0 <= number) && (number <= 59))
			{
			    tm_s->tm_sec = number;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 't' :

			/*
			 *  Match a '\t' (tab) character.
			 *
			 *  Need to handle possible whitespace in the
			 *  input string - skip any whitespace as long
			 *  as it's not a '\t'.
			 */

			while (isspace(**str) && (CHARAT(*str) != '\t'))
			    ADVANCE(*str);

			if (CHARAT(*str) == '\t')
			{
			    /*
			     *  Consume the '\t'.
			     */
			    ADVANCE(fmt);
			    ADVANCE(*str);

			    match_so_far = TRUE;
			}

		    	break;

		    case 'T' :

			/*
			 *  Match time as "%H:%M:%S".
			 */

			sub_fmt = "%H:%M:%S";
			match_so_far = process_fmt(sub_fmt, str,
						   tm_s, (int) TRUE);

			if (match_so_far == TRUE)
			    ADVANCE(fmt);

			break;

		    case 'w' :

			/*
			 *  Match weekday as number (0 - 6).
			 */

			number = strntoi(*str, &str_ptr, 2);
			
			if ((str_ptr != *str) &&
			    (0 <= number) && (number <= 6))
			{
			    /*
			     *  Will let the mktime function do
			     *  consistency checks on the actual
			     *  weekday number later.
			     */

			    tm_s->tm_wday = number;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 'x' :

			/*
			 *  Match locale's appropriate date
			 *  representation.
			 *  For example: "%m/%d/%y"
			 */

			sub_fmt = nl_langinfo(D_FMT);

			if (strlen(sub_fmt) > 0)
			    match_so_far = process_fmt(sub_fmt, str,
						       tm_s,
						       (int) TRUE);

			if (match_so_far == TRUE)
			    ADVANCE(fmt);

			break;

		    case 'X' :

			/*
			 *  Match locale's appropriate time
			 *  representation.
			 *  For example: "%H:%M:%S"
			 */

			sub_fmt = nl_langinfo(T_FMT);

			if (strlen(sub_fmt) > 0)
			    match_so_far = process_fmt(sub_fmt, str,
						       tm_s,
						       (int) TRUE);

			if (match_so_far == TRUE)
			    ADVANCE(fmt);

			break;

		    case 'y' :

			/*
			 *  Match year (without century) as number.
			 */

			number = strntoi(*str, &str_ptr, 2);

			/*
			 *  SVID3 getdate(3c):
			 *    "Dates before 1970 and after 2037 are
			 *     illegal."			
			 *
			 *  Use current_tm->tm_year to figure out which
			 *  century it is (as if this routine will still
			 *  be used in 2000! :).
			 */

			if ((str_ptr != *str) &&
			    (((70 <= number) && (number <= 99) &&
			      (current_tm->tm_year <= 99)) ||
			     ((0  <= number) && (number <= 37) &&
			      (current_tm->tm_year >  99))))
			{
			    tm_s->tm_year = number;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

			break;

		    case 'Y' :

			/*
			 *  Match year (with century) as number
			 *  (e.g. 1990).
			 */

			number = strntoi(*str, &str_ptr, 4);

			/*
			 *  SVID3 getdate(3c):
			 *    "Dates before 1970 and after 2037 are
			 *     illegal."
			 */

			if ((str_ptr != *str) &&
			    (1970 <= number) && (number <= 2037))
			{
			    /*
			     *  The tm_year field represents years since
			     *  1900 (see ctime(3c)).  (When this
			     *  goes negative, the function mktime
			     *  will fail - I checked!)
			     */

			    tm_s->tm_year = number - 1900;

			    ADVANCE(fmt);
			    *str = str_ptr;

			    match_so_far = TRUE;
			}

		    	break;

		    case 'Z' :

			/*
			 *  Match a time zone name.  If no time
			 *  zone exists, match no characters.
			 *  (On a V.4 system, "no time zone" behavior
			 *   seems to include when TZ="".)
			 */

			if (((c_p = getenv("TZ")) != (char *) NULL) &&
			    (CHARAT(c_p) != '\0'))
			{
			    /*
			     *  A time zone exists.  Now use tzname[]
			     *  (see ctime(3C)) to compare against the
			     *  time zone name from the input string.
			     *  (Only look at the first three [or
			     *   _POSIX_TZNAME_MAX] characters as it
			     *   appears that's what V.4 getdate does.)
			     *
			     *  Set the tm_isdst field according to
			     *  whether a STD (0) or a DST (1) time
			     *  is supplied.  This is important later
			     *  on when we call mktime - it behaves
			     *  different depending on how tm_isdst is
			     *  set.
			     */ 
			    if (strncasecmp(*str,
					    tzname[_STD_NAME_INDEX],
					    (size_t) _MY_TZNAME_MAX) == 0)
				tm_s->tm_isdst = 0;
			    else
				if (strncasecmp(*str,
						tzname[_DST_NAME_INDEX],
					        (size_t) _MY_TZNAME_MAX) == 0)
				    tm_s->tm_isdst = 1;

			    if ((tm_s->tm_isdst == 0) ||
				(tm_s->tm_isdst == 1))
			    {
				for (i = 0; i < _MY_TZNAME_MAX; i++)
				    ADVANCE(*str);

				ADVANCE(fmt);

				match_so_far = TRUE;
			    }
			}
			else
			{
			    /*
			     *  Go ahead and skip the format specifier
			     *  as we still have a chance to match no
			     *  characters.
			     */

			    ADVANCE(fmt);
			}

			break;

		    default :

			/*
			 *  At this point, we have found an invalid
			 *  field descriptor.  Reject this line.
			 */

			match_so_far = FALSE; /* just to be sure */
			break;

		} /* switch */

	    }
	    else if (CHARAT(fmt) == CHARAT(*str))
	    {
		/*
		 *  CASE 2 applies.  Be careful, could be sitting at
		 *  end-of-string (EOS) for both strings.  Check one of
		 *  the strings to be sure.
		 */ 

		if (CHARAT(fmt) != '\0')
		{
		    ADVANCE(fmt);
		    ADVANCE(*str);
		}

		match_so_far = TRUE;
	    }

	    /*
	     *  Here when CASE 3 applies (did not find a format
	     *  specifier key-letter (%) and did not find
	     *  matching regular characters in both strings.
	     *  Since match_so_far has not been set to TRUE,
	     *  we should fall out of this while loop, clear the
	     *  tm structure, and return FALSE.
	     */

	} /* while */

	/*
	 *  Skip any trailing whitespace in either string.
	 */

	while (isspace(*fmt))
	    ADVANCE(fmt);

	while (isspace(**str))
	    ADVANCE(*str);

	if ((match_so_far == TRUE) &&
	    (CHARAT(fmt) == '\0') && (CHARAT(*str) == '\0'))
	{
	    /*
	     *	Here when both strings have matched throughout the
	     *	processing and EOS has been reached for both strings.
	     *	NOTE:  this case may be satisfied for a recursive call
	     *	if the format specification causing the recursive call
	     *	was the last one to be processed.
	     */

	    return TRUE;
	}
	else if ((recurse == TRUE) && (CHARAT(fmt) == '\0'))
	{
	    /*
	     *  Here when, during a recursive call of this routine,
	     *  the expanded format descriptors have been completely
	     *  matched.  EOS for the input string may not have been
	     *  reached, but the rest of that string will be handled
	     *  (along with a possible future mismatch) upon return
	     *  from this recursive call.
	     */

	    return TRUE;
	}
	else
	{
	    /*
	     *  Here when, recursive call or not, we know there is a
	     *  mismatch between the input and format strings.
	     */

#ifdef DEBUG
	    (void) printf("%s%s%s\n%s%s%s\n",
			  "Bailing with remaining fmt of: \"",
			  fmt,
			  "\"",
			  "	 and remaining *str of: \"",
			  *str,
			  "\"");
#endif /* DEBUG */

	    /*
	     *  Make sure that any information so far built up in
	     *  the tm structure is cleared.
	     */

	    tm_s->tm_sec   = -1;
	    tm_s->tm_min   = -1;
	    tm_s->tm_hour  = -1;
	    tm_s->tm_mday  = -1;
	    tm_s->tm_mon   = -1;
	    tm_s->tm_year  = -1;
	    tm_s->tm_wday  = -1;
	    tm_s->tm_yday  = -1;
	    tm_s->tm_isdst = -1;

	    return FALSE;
	}

} /* function process_fmt */


/*
 *  The tm_resolve() function's job is to completely fill in the
 *  contents of the supplied tm structure.  In other words, if not all
 *  the fields in the supplied tm structure contain meaningful values,
 *  this function will attempt to initialize such fields.
 *
 *  From the SVID3 manpage for getdate(), the following rules apply for
 *  converting the input specification into the internal format:
 *
 *  Rule #1     If only the weekday is given, today is assumed if the
 *		given day is equal to the current day and next week if
 *		it is less.
 *
 *  Rule #2     If only the month is given, the current month is assumed
 *		if the given month is equal to the current month and
 *		next year if it is less and no year is given.  (The
 *		first day of the month is assumed if no day is given).
 *
 *  Rule #3     If no hour, minute and second are given, the current
 *		hour, minute and second are assumed.
 *
 *  Rule #4     If no date is given, today is assumed if the given hour
 *		is greater than the current hour, and tomorrow is
 *		assumed if it is less.
 *
 *		(I assumed that "date" is fully specified by the
 *		 tm_mday, tm_mon, and tm_year fields.)
 *
 *	While using these rules to default to a date, I don't check the
 *	tm_wday and tm_yday fields (except for Rule #1).  For instance,
 *	in Rule #2, I don't check to be sure that a year or week day
 *	have not been specified when determining that only the month
 *	part has been supplied.
 *
 *	I depend on the function mktime (see ctime(3c)) to give me the
 *	correct tm_wday, tm_yday, and tm_isdst values.  (And then cross
 *	check against possibly supplied values.)
 *
 */

static void
tm_resolve(given_tm, curr_tm)
struct tm *given_tm;
struct tm *curr_tm;
{
	/*
	 *  Parameters:
	 *
	 *	given_tm - pointer to the tm structure containing the
	 *		   date constructed (so far) using the input
	 *		   time specification
	 *	curr_tm  - pointer to tm structure containing the
	 *		   current date/time information
	 *
	 *  Local variables:
	 *
	 *  The adjust_days variable is used when an adjustment
	 *  is required to the tm_mday field.  This will happen
	 *  when the specified week day has passed for the week
	 *	1) used to hold value by which to adjust the tm_mday
	 *	   component of the given tm structure
	 *	2) a non-zero value means that a possible "carry"
	 *	   operation is needed from the tm_mday field to the
	 *	   tm_mon and tm_year fields (operation to be carried
	 *	   out by the function mktime)
	 */

	int adjust_days = 0;
	struct tm temp_tm;

	/*
	 *  First, check the time fields.
	 */

	if ((given_tm->tm_sec  == -1) &&
	    (given_tm->tm_min  == -1) &&
	    (given_tm->tm_hour == -1))
	{
	    /*
	     *  Here when hour, minute, and second are not specified.
	     *  Rule #3 applies.
	     */

	    given_tm->tm_sec  = curr_tm->tm_sec;
	    given_tm->tm_min  = curr_tm->tm_min;
	    given_tm->tm_hour = curr_tm->tm_hour;
	}
	else
	{
	    /*
	     *  Time partially specified, default missing time info
	     *  to 0.
	     */

	    if (given_tm->tm_hour == -1)
		given_tm->tm_hour = 0;

	    if (given_tm->tm_min  == -1)
		given_tm->tm_min  = 0;

	    if (given_tm->tm_sec  == -1)
		given_tm->tm_sec  = 0;
	}

	/*
	 *  Now, check the date-related fields.
	 */

	if ((given_tm->tm_year == -1) &&
	    (given_tm->tm_mon  == -1) &&
	    (given_tm->tm_mday == -1))
	{
	    /*
	     *  Here when day, month, and year are not specified.
	     *  If the weekday is given, Rule #1 applies.
	     *  If the weekday is not given, Rule #4 applies.
	     */

	    given_tm->tm_year = curr_tm->tm_year;
	    given_tm->tm_mon  = curr_tm->tm_mon;
	    given_tm->tm_mday = curr_tm->tm_mday;

	    if (given_tm->tm_wday != -1)
	    {
		/*
		 *  Rule #1 applies.
		 *  If the given day has already passed for this week,
		 *  go with next week; otherwise, stay with this week.
		 *  In either case, let the function mktime normalize
		 *  the rest of the structure in case of needing to
		 *  "carry" to next month and/or next year.
		 */

		adjust_days = curr_tm->tm_wday - given_tm->tm_wday;

		if (adjust_days >= 0)
		    given_tm->tm_mday += (7 - adjust_days);
		else
		    given_tm->tm_mday += ( - adjust_days);

	    }
	    else
	    {
		/*
		 *  Rule #4 applies.
		 *
		 *  Rule #4 states:
		 *	"..., today is assumed if the given hour is
		 *	 greater than the current hour and tomorrow
		 *	 is assumed if it is less."
		 *  
		 *  The interpretation I made was that the complete
		 *  time is significant, not just the tm_hour part.
		 */

		if ((curr_tm->tm_hour > given_tm->tm_hour)      ||

		    ((curr_tm->tm_hour == given_tm->tm_hour) &&
		     (curr_tm->tm_min  >  given_tm->tm_min))    ||

		    ((curr_tm->tm_hour == given_tm->tm_hour) &&
		     (curr_tm->tm_min  == given_tm->tm_min)  &&
		     (curr_tm->tm_sec  >  given_tm->tm_sec)))
		{
		    given_tm->tm_mday += 1;

		    /*
		     *  Since the decision has been made to go with
		     *  tomorrow, set the adjust_days variable 
		     *  accordingly.
		     */

		    adjust_days = 1;
		}
	    }
	}

	/*
	 *  See Rule #2 above.
	 *
	 *  (The last part of Rule #2 is implemented below because
	 *   I assumed that it applies in all cases, not just when
	 *   only the month is given.)
	 */

	else if ((given_tm->tm_mon != -1) && (given_tm->tm_year == -1))
	{
	    given_tm->tm_year = curr_tm->tm_year;
	    if (given_tm->tm_mon < curr_tm->tm_mon)
		given_tm->tm_year++;
	}

	/*
	 *  At this point, we know the tm_sec, tm_min, and tm_hour
	 *  fields are filled in.  Also, we'll let the function
	 *  mktime fill in the tm_yday, tm_wday, and tm_isdst fields.
	 *  So, if they've not already been treated by any of the
	 *  supplied rules, take care of the tm_mday, tm_mon, and
	 *  tm_year fields.
	 */

	if (given_tm->tm_mday == -1)
	{
	    /*
	     *  Here when the month day field remains unresolved.  How
	     *  to resolve it depends on whether a tm_wday was given
	     *  (because that would affect whether or not we could just
	     *   default to the first day of the month).
	     */

	    if (given_tm->tm_wday == -1)
		given_tm->tm_mday = 1;
	    else
	    {
		/*
		 *  Use the function mktime to determine the weekday of
		 *  the first day in the month, and then figure the
		 *  month day based on that.
		 */

		temp_tm.tm_sec   = 1;  /* one second into ... */
		temp_tm.tm_min   = 0;
		temp_tm.tm_hour  = 0;
		temp_tm.tm_mday  = 1;  /* ... first day of month */
		temp_tm.tm_mon   = given_tm->tm_mon;
		temp_tm.tm_year  = given_tm->tm_year;
		temp_tm.tm_wday  = -1; /* mktime will furnish these */
		temp_tm.tm_yday  = -1;
		temp_tm.tm_isdst = -1;

		errno = 0;

		/*
		 *  If mktime fails, just default and go on (the
		 *  mktime call in is_valid_tm() should also fail).
		 *  (From ctime(3c): if the time cannot be represented,
		 *   mktime() returns -1 and sets errno to ERANGE.)
		 */

		if ((mktime(&temp_tm) == (time_t) -1) &&
		    (errno == ERANGE))
		{
		    given_tm->tm_mday = 1;
		}
		else
		{
		    /*
		     *  temp_tm.tm_wday contains the weekday number for
		     *  the first day of the given month.  Use that
		     *  number to figure out the corresponding month day
		     *  number for the given weekday number.
		     *
		     *  adjust_days will be less than 0 if the given
		     *  week day has passed already this week - in that
		     *  case, add 7 to get to the month day number for
		     *  the corresponding week day in the next week.
		     */
		    adjust_days = given_tm->tm_wday - temp_tm.tm_wday;

		    given_tm->tm_mday = 1 + adjust_days;
		    if (adjust_days < 0)
			given_tm->tm_mday += 7;

		    /*
		     *  Reset adjust_days as this particular adjustment
		     *  should never precipitate a "carry" to the month
		     *  and year fields (because it is based from the
		     *  first day of the month).
		     */

		    adjust_days = 0;
		}
	    }
	} /* if (given_tm->tm_mday == -1) */

	/*
	 *  For tm_mon, default to current month.
	 */

	if (given_tm->tm_mon == -1)
	    given_tm->tm_mon = curr_tm->tm_mon;

	/*
	 *  For tm_year, refer to tm_mon to determine whether to
	 *  default to this year or go with next year.  Go with next
	 *  year if the given month has already passed for this year.
	 */

	if (given_tm->tm_year == -1)
	{
	    given_tm->tm_year = curr_tm->tm_year;
	    if (given_tm->tm_mon < curr_tm->tm_mon)
		given_tm->tm_year++;
	}

	/*
	 *  The final step to completely resolve the given tm structure
	 *  is to accomplish a "carry" made necessary by a month day
	 *  adjustment.  (It's easier to let mktime() do this rather
	 *  than add the code to do it here.)  Allow only the month day,
	 *  month, and year fields to be adjusted.  Do not adjust any
	 *  other fields so that a subsequent mktime() call can be used
	 *  to perform a check on all elements of the structure.
	 */

	if (adjust_days != 0)
	{
	    /*
	     *  Make a copy of the tm structure.
	     */
	    temp_tm = *given_tm;

	    /*
	     *  Reset the time fields so they cannot possibly enter into
	     *  the calculation.  This is done because we're really not
	     *  interested in anything more granular than tm_mday (and
	     *  also because we may have changed the time as a result
	     *  of the "case 'p'" part of the switch in process_fmt()).
	     */

	    temp_tm.tm_sec  = 1;
	    temp_tm.tm_min  = 0;
	    temp_tm.tm_hour = 0;

	    errno = 0;

	    /*
	     *  From ctime(3c): the value (time_t)-1 also corresponds
	     *  to the time 31 Dec 1969 23:59:59.  Therefore,
	     *  have to also check errno to reliably detect an
	     *  error condition.
	     */
	    if ((mktime(&temp_tm) != (time_t) -1) || (errno != ERANGE))
	    {
		given_tm->tm_mday = temp_tm.tm_mday;
		given_tm->tm_mon  = temp_tm.tm_mon;
		given_tm->tm_year = temp_tm.tm_year;
	    }

	    /*
	     *  If the function mktime fails, just ignore the results
	     *  (the next call to mktime [in is_valid_tm()] should
	     *   also fail).
	     */
	}
} /* function tm_resolve */


/*
 *  The is_valid_tm() function's job is to make sure that the
 *  supplied tm structure contains a meaningful date.  For
 *  instance, an input specification of "February 31" should be
 *  rejected.
 */

static int
is_valid_tm(reference_tm)
struct tm *reference_tm;
{
	/*
	 *  Parameters:
	 *	reference_tm - points to the tm structure containing the
	 *		       date constructed using the input
	 *		       date/time specification
	 */

	/*
	 *  Local variables
	 */

	int invalid = FALSE;
	struct tm test_tm;

	test_tm = *reference_tm;

	/*
	 *  The first three if statments are necessary to check
	 *  consistency between %I (or %H) and %p.
	 *
	 *    First, check that if an hour input matched a %I format, an
	 *	     AM or PM string matched a %p format.  (%I => %p)
	 *
	 *    Second, check that if an AM or PM string matched a %p
	 *	      format, that an hour input matched a %I format.
	 *	      (%p => %I)
	 *
	 *    Third, if it turns out that a 12-hour clock was intended
	 *	     (an AM or PM string matched a %p format), check to
	 *	     see whether the hour is in the acceptable range.
	 */

	if ((need_AM_or_PM == TRUE) &&
	    ((is_AM == FALSE) && (is_PM == FALSE)))
	{
	    invalid = TRUE;
	}
	else if ((need_AM_or_PM == FALSE) &&
		 ((is_AM == TRUE) || (is_PM == TRUE)))
	{
	    invalid = TRUE;
	}
	else if (((is_AM == TRUE) && (test_tm.tm_hour > 12))  ||
		 ((is_PM == TRUE) && (test_tm.tm_hour > 23)))
	{
	    invalid = TRUE;
	}
	else
	{
	    /*
	     *  Use the function mktime to help do other checking.
	     *  It will return -1 and set errno to ERANGE when the
	     *  supplied calendar time cannot be represented.
	     */

	    errno = 0;

	    if ((mktime(&test_tm) == (time_t) -1) && (errno == ERANGE))
		invalid = TRUE;
	    else
	    {
		/*
		 *  The calendar time made it through mktime, but there
		 *  are still "by hand" checks that need to be done to
		 *  the returned structure.  For instance, if a week day
		 *  was specified, it may not "jibe" with the given
		 *  date.  But, if mktime is fed an invalid month/day
		 *  combination (e.g.  31 February) and succeeds, it
		 *  will bump the tm_mon and tm_mday fields (in the 31
		 *  February case, to numbers representing 3 March).
		 *  Therefore, to catch this and similar problems,
		 *  compare the after-mktime structure with the
		 *  before-mktime structure.
		 *
		 *  mktime fills in the tm_wday, tm_yday, and tm_isdst
		 *  fields, so treat them special.
		 *
		 *  Here's a twist:  do not cross-check the rest of the
		 *  structure elements if the tm_isdst value has
		 *  switched from one valid value (e.g.  from "1"
		 *  indicating DST in effect) to the other valid value
		 *  (e.g.  to "0" indicating STD in effect).
		 *  This is to be able to mimic the following
		 *  behavior.
		 *
		 *  Given an input string argument of:
		 *  
		 *  	"01/01/92 00:59:59 MDT"
		 *  
		 *  The V.4 version of getdate returns a tm structure
		 *  representing:
		 *  
		 *  	"12/31/91 23:59:59 MST"
		 */

		if ((reference_tm->tm_isdst == -1) ||
		    (reference_tm->tm_isdst == test_tm.tm_isdst))
		{
		    if ((test_tm.tm_sec  != reference_tm->tm_sec)    ||
			(test_tm.tm_min  != reference_tm->tm_min)    ||
			(test_tm.tm_hour != reference_tm->tm_hour)   ||
			(test_tm.tm_mday != reference_tm->tm_mday)   ||
			(test_tm.tm_mon  != reference_tm->tm_mon)    ||
			(test_tm.tm_year != reference_tm->tm_year)   ||
			((reference_tm->tm_wday != -1) &&
			 (test_tm.tm_wday != reference_tm->tm_wday)) ||
			((reference_tm->tm_yday != -1) &&
			 (test_tm.tm_yday != reference_tm->tm_yday)))
		    {
#ifdef DEBUG
			getdate_print_tm(*reference_tm,
					 "is_valid_tm *before* mktime:");
			getdate_print_tm(test_tm,
					 "is_valid_tm *after* mktime:");
#endif
			invalid = TRUE;
		    }
		}
	    }
	}

	if (invalid == TRUE)
	    return FALSE;
	else
	{
	    *reference_tm = test_tm;
	    return TRUE;
	}

} /* function is_valid_tm */

/*
 *  Convert up to count characters from input string into an integer.
 *  Set string_ptr to point to character terminating the search.
 */

static int
strntoi(string, string_ptr, count)
char *string;
char **string_ptr;
const int count;
{
	/*
	 *  Parameters:
	 *
	 *	string	   - points into the user-supplied input string
	 *	string_ptr - points to a location containing a pointer
	 *		     that indicates where in the input string
	 *		     the string-to-interger conversion left off
	 *	count	   - number of digits significant to the
	 *		     conversion
	 */

	int i   = 0;
	int num = 0;

	*string_ptr = string;

#ifdef DEBUG
	(void) printf("%s \n\t %s \"%s\", \n\t%s \"%s\", \n\t%s \"%d\"\n",
		      "Entering strntoi():", 
		      "string: ", string,
		      "*string_ptr: ", *string_ptr,
		      "count: ", count);
#endif /* DEBUG */

	for (i = 0; i < count; i++)
	{
	    if (isdigit(**string_ptr))
	    {
		/*
		 *  I was not sure whether numbers would ever need two
		 *  characters.  In the grand scheme of this routine,
		 *  an unnecessary CHARAT is in the wash anyway. (?)
		 */

		num = num * 10 + (CHARAT(*string_ptr) - '0');

		ADVANCE(*string_ptr);
	    }
	    else
		break;
	}

#ifdef DEBUG
	(void) printf("%s \n\t %s \"%s\", \n\t%s \"%s\", \n\t%s \"%d\"\n",
		      "Exiting strntoi():", 
		      "string: ", string,
		      "*string_ptr: ", *string_ptr,
		      "num: ", num);
#endif /* DEBUG */

	return num;
}

#ifdef DEBUG
static void
getdate_print_tm(tm_p, text)
struct tm tm_p;
char *text;
{
	(void) printf("\n**********\n");
	(void) printf("%s\n", text);
	(void) printf("**********\n");

	(void) printf("tm_sec   is: %d\n", tm_p.tm_sec);
	(void) printf("tm_min   is: %d\n", tm_p.tm_min);
	(void) printf("tm_hour  is: %d\n", tm_p.tm_hour);
	(void) printf("tm_mday  is: %d\n", tm_p.tm_mday);
	(void) printf("tm_mon   is: %d\n", tm_p.tm_mon);
	(void) printf("tm_year  is: %d\n", tm_p.tm_year);
	(void) printf("tm_wday  is: %d\n", tm_p.tm_wday);
	(void) printf("tm_yday  is: %d\n", tm_p.tm_yday);
	(void) printf("tm_isdst is: %d\n", tm_p.tm_isdst);

} /* function getdate_print_tm */
#endif /* DEBUG */
