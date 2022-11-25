/* $Header: strptime.c,v 72.8 92/11/30 14:44:07 ssa Exp $ */

/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define strptime	_strptime
#define isspace		_isspace
#define strncasecmp	_strncasecmp
#define strlen		_strlen
#define strcoll		_strcoll
#define nl_langinfo	_nl_langinfo
#define mktime		_mktime
/* Note that the malloc family should not have
   an secondary definition.
   #define malloc	_malloc
   #define free		_free
*/
#endif /* _NAMESPACE_CLEAN */

#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <langinfo.h>
#include <stdlib.h>
#include <setlocale.h>

#if defined(NLS) || defined(NLS16)
#  include <nl_ctype.h>
#else
#  define CHARAT(p)  (*(p))
#  define ADVANCE(p) (++(p))
#  define CHARADV(p) (*(p)++)
#endif /* defined(NLS) || defined(NLS16) */

#define FAILURE (char *)0	/* return value in case of failure */
#define UNKNOWN    -1
#define BASEYEAR   1900		/* the base year in the tm_year field */
#define DEFCENTURY 19		/* default century */
#define COMPFAIL   -1		/* failed in compare() */
#define COMPIGN    0		/* nothing to compare in compare() */

/* bit masks used in flagword */
#define FHOUR12  0x1	/* a 12-hour clock time is known */
#define FYEAR    0x2	/* the year is specified */
#define FMON     0x4	/* the month is specified */
#define FMDAY    0x8	/* the day of month is specified */
#define FWDAY    0x10	/* the week day is specified */
#define FSUNWK   0x20	/* Sunday-first week number is given, ie. 'U' flag */
#define FMONWK   0x40 	/* Monday-first week number is given, ie. 'W' flag */
#define FHOUR    0x80	/* the hour of day is specified */

extern int errno;
extern size_t strlen();

#ifdef _NAMESPACE_CLEAN
#    undef  strptime
#    pragma _HP_SECONDARY_DEF _strptime strptime
#    define strptime _strptime
#endif /* _NAMESPACE_CLEAN */

/*****************************************************************************
 *                                                                           *
 *  DATE AND TIME CONVERSION                                                 *
 *                                                                           *
 *  INPUTS:      buffer        - the input string                            *
 *                                                                           *
 *               format        - the format string                           *
 *                                                                           *
 *               tm            - the output time representation              *
 *                                                                           *
 *  OUTPUTS:     a character pointer to the character following the last     *
 *               character parsed. Otherwise, a null pointer is returned.    *
 *                                                                           *
 *  AUTHOR:      Francis Au-Yeung                                            *
 *                                                                           *
 *****************************************************************************/


/* ------------------------------
 * CHECK_SCAN_SPACE()
 * Check, if necessary, to see if at least one white-space character 
 * exists in the buffer. If so, skip over the rest of spaces. Otherwise,
 * program returns as an error.
 * ------------------------------
 */
#define CHECK_SCAN_SPACE()					\
{								\
	/* hasspace is set by the white-space directives, 	\
	 * %n and/or %t directives.				\
	 */							\
	if (hasspace) { 					\
		if (!isspace((int)CHARADV(bufptr)))		\
			/* expected spaces but no show */	\
			return(FAILURE);			\
								\
		/* skip over rest of spaces if needed */	\
		while (isspace((int)CHARAT(bufptr)))		\
			ADVANCE(bufptr);			\
		hasspace = 0; 	/* clear future checking */	\
	}							\
}


/* ------------------------------
 * CENT_TO_YEAR()
 * figure out the year if both century and the year within century
 * (cenyear) are known.
 * ------------------------------
 */
#define CENT_TO_YEAR()						\
{								\
	if (century != UNKNOWN && cenyear != UNKNOWN) {		\
		tm->tm_year = century * 100 + cenyear - BASEYEAR; \
		*flagptr |= FYEAR;				\
	}							\
}


/* ------------------------------
 * HOUR_12_TO_24()
 * convert a 12-hour time to a 24-hour time.
 * ------------------------------
 */
#define HOUR_12_TO_24()						\
{								\
	if (am_pm == AM_STR) {					\
		/* 12 a.m is 0 hr; rest same */			\
		if (tm->tm_hour == 12)				\
			tm->tm_hour = 0;			\
	} else {						\
		/* n p.m is n+12 hr, except 12 p.m 		\
		 * is also 12 */				\
		if (tm->tm_hour != 12)				\
			tm->tm_hour += 12;			\
	}							\
}


#define MAXARAY 50
/* ------------------------------
 * nl_strncasecmp():
 * This routine performs the same functions as strncasecmp(3C), 
 * except that it also honors locale in the comparison. 
 * ------------------------------
 */
static int
nl_strncasecmp(s1, s2, n) 
	char *s1, *s2;		/* pointers to strings to be compared */
	size_t n;		/* number of bytes to compare */
{
	char *ls1, *ls2;
	char space1[MAXARAY], space2[MAXARAY];
	int  i;

	/* Short cut, if we are processing "C" locale, just call the
	 * faster strncasecmp(), and skip the extra processing.
	 */
	if (!_nl_collate_on)
		return(strncasecmp(s1, s2, n));

	/* else NLS processing... */

	/* short cut, we already know the string length of s2 is n */
	if (strlen(s1) < n)
		return(-1);

	/* for performance reasons, try to avoid using malloc if we have
	 * enough spaces from the fix size arrays.
	 */
	if (n < MAXARAY) {	/* spaces provided are big enough */
		ls1 = space1;
		ls2 = space2;
	} else {
		/* instead of using two malloc calls for ls1 and ls2, 
		 * call malloc once and split the result.
		 */
		if (ls1 = malloc((size_t)n+n+2)) { 
			/* malloc success */
			ls2 = ls1 + n + 1;
		} else {
			/* out of space, return no match */
			errno = ENOSPC;
			return(-1);
		}
	}

	for (i=0; i<n; i++) {
		*(ls1+i) = _tolower(*(s1+i));
		*(ls2+i) = _tolower(*(s2+i));
	}
	*(ls1+n) = *(ls2+n) = '\0';	/* add terminators */

	i = strcoll(ls1, ls2);

	if (n >= MAXARAY)
		free((void *)ls1);

	return(i);
}
	

/* ------------------------------
 * compare():
 * compare the string pointed to by string pointer, tarptr, with the locale 
 * string returned from nl_langinfo, given the index from low to high
 * one at a time. 
 *
 * RETURN VALUES:
 *   [low, high]: if there is a match, returns the matching index value,
 *                and update the tarptr pointer to the end of string;
 *   COMPFAIL:    if there is non match at all;
 *   COMPIGN:     if there is no value defined between the low and high 
 *                index for that locale (i.e. comparsion is ignored).
 * ------------------------------
 */
static int
compare(tarptr, low, high)
	char **tarptr;		/* pointer to string pointer to be compared */
	int  low;		/* lower index to the locale database */
	int  high;		/* higher index to the locale database */
{
        register int    i;
	register size_t nlen;
        register char   *name;
	register comp = 0;	/* performed comparsion? */

        for (i=low; i<=high; i++) {
                name = nl_langinfo(i);
                if ((nlen = strlen(name)) == 0) 
			continue;	/* no need to compare */

		comp = 1;
                if (!nl_strncasecmp(*tarptr, name, nlen)) {
                        /* skip over the matched string */
                        *tarptr += nlen;
                        return(i);
                }
        }
	/* either no matched or comparison ignored */
        return(comp ? COMPFAIL : COMPIGN); 
}


/* ------------------------------
 * str2int()
 * read from the string pointed by the string pointer, ptr, and convert
 * it into an integer value not larger than the max value. If there is
 * no digit string being pointed or if the value converted is less than
 * the min value, -1 is returned. 
 * ------------------------------
 */
static int
str2int (ptr, min, max)
	char **ptr;		/* pointer to string pointer to be converted */
	int  min, max;		/* minimum and maximum converted range */
{
	register int  i, val;
	register unsigned int c;

	/* must start with a digit */
	/* note the extra parenthesis to (*ptr) is preferred, as CHARADV()
	 * is a macro. 
	 */
	if (!isdigit(c=CHARADV((*ptr))))
		return(-1);

	val = c - '0';
	while (isdigit(c=CHARAT((*ptr))) && ((i = val * 10 + c - '0') <= max)) {
		/* valid value */
		val = i;
		ADVANCE((*ptr));
	}
	return(val >= min && val <= max ? val : -1);
}


/* ------------------------------
 * parsetime()
 * ------------------------------
 */
static char *
parsetime(buffer, format, tm, flagptr, wknptr)
	char      *buffer;	/* pointer to the time string */
	char      *format;	/* pointer to the format string */
	struct tm *tm;		/* pointer to the resulting time value */
	int       *flagptr;	/* pointer to the flag status word */
	int       *wknptr;	/* pointer to the week number */
{
	register char *formptr; /* pointer to format string */
	unsigned int  formchar;	/* a format character */
	         char *bufptr;	/* next char to be scanned in input string */
	register int  hasspace;	/* white space(s) expected in input? */
	register int  match, i;	
	register int  century;	/* century as specified by %C */
	register int  cenyear;	/* year within a century */
	register int  am_pm;	/* stores AM_STR or PM_STR */

	/* ----------------------
	 * MAIN BODY
	 * ----------------------
	 */
	formptr  = format;
	bufptr   = buffer;
	hasspace = 0;
	am_pm = cenyear = UNKNOWN;
	century  = DEFCENTURY;

	/* the main loop, each iteration process a format directive */
	for (;;) {
		formchar = CHARADV(formptr);

		/* 
		 *  End of format string
		 *  --------------------
		 */
		if (!formchar) {	/* format parsing done */
			CHECK_SCAN_SPACE();
			break;
		}

		/*
		 *  White-space directive
		 *  ---------------------
		 */
		if (isspace((int)formchar)) {
			hasspace = 1; 	/* expect to have space in buffer */
			/* skip format ptr to the end of the blank string */
			while (isspace((int)CHARAT(formptr))) 
				ADVANCE(formptr);

			continue;
		}
				
		/*
		 *  Conversion directive
		 *  --------------------
		 */
		if (formchar == '%') {
			/* see what follows the coversion directive */
			formchar = CHARADV(formptr);

			if (formchar == 'n' || formchar == 't') {
				/* for now, mark spaces are expected 
			 	 * in buffer, deal with them later. 
				 */
				hasspace = 1; 
				continue;
			}

			/* check for leading space(s) as specified earlier */
			CHECK_SCAN_SPACE();

			if (formchar == 'E' || formchar == 'O') {
				/*
				 * these supposed to call the modified 
				 * directives. However, they are currently 
				 * not supported in HP-UX.
				 * So, simply ignore them and move on.
				 */
				formchar = CHARADV(formptr);
			}

			switch(formchar) {
			case '%': 	/* expect a '%' */
				if (CHARADV(bufptr) != '%') 
					return(FAILURE);
				break;

			case 'a': 	/* day of week, full or abbreviated */
			case 'A':
				if ((match=compare(&bufptr, DAY_1, ABDAY_7)) == COMPFAIL)
					return(FAILURE);
				else if (match != COMPIGN) {
					tm->tm_wday = (match - DAY_1) % 7;
					*flagptr |= FWDAY;
				}
				break;

			case 'b':	/* month, full or abbreviated */
			case 'B':
			case 'h':
				if ((match = compare(&bufptr, MON_1, ABMON_12)) == COMPFAIL)
					return(FAILURE);
				else if (match != COMPIGN) {
					tm->tm_mon = (match - MON_1) % 12;
					*flagptr |= FMON;
				}
				break;

			case 'c':	/* locale's date and time format */
				if (!(bufptr = parsetime(bufptr, nl_langinfo(D_T_FMT), tm, flagptr, wknptr)))
					return(FAILURE);
				break;

			case 'C':	/* century number, [0,99] */
				if ((century = str2int(&bufptr, 0, 99)) < 0)
					return(FAILURE);

				CENT_TO_YEAR();
				break;

			case 'd':	/* day of month, [1,31] */
			case 'e':
				if ((tm->tm_mday = str2int(&bufptr, 1, 31)) < 0)
					return(FAILURE);
				*flagptr |= FMDAY;
				break;

			case 'D':	/* date as %m/%d/%y */
				if (!(bufptr=parsetime(bufptr, "%m/%d/%y", tm, flagptr, wknptr)))
					return(FAILURE);
				break;

			case 'H':	/* hour (24-hour clock), [0,23] */
				if ((tm->tm_hour = str2int(&bufptr, 0, 23)) < 0)
					return(FAILURE);
				*flagptr |= FHOUR;
				break;

			case 'I':	/* hour (12-hour clock), [1,12] */
				if ((tm->tm_hour = str2int(&bufptr, 1, 12)) < 0)
					return(FAILURE);
				*flagptr |= FHOUR;

				/* if we know it's am or pm, convert it now.
				 * Else, mark we get the time in 12-hour 
				 * form, and hope to convert it later.
				 */
				if (am_pm != UNKNOWN) {
					HOUR_12_TO_24();
					/* reset value to avoid future double
					 * conversion 
					 */
					am_pm = UNKNOWN;
				} else
					*flagptr |= FHOUR12;
				break;

			case 'j':	/* day number of the year, [1,366] */
				if ((i = str2int(&bufptr, 1, 366)) < 0)
					return(FAILURE);
				tm->tm_yday = --i;  /* internally as [0,365] */
				break;

			case 'm':	/* month number, [1,12] */
				if ((i = str2int(&bufptr, 1, 12)) < 0)
					return(FAILURE);
				tm->tm_mon = --i;  /* internally as [0,11] */
				*flagptr |= FMON;
				break;

			case 'M':	/* minute, [0,59] */
				if ((tm->tm_min = str2int(&bufptr, 0, 59)) < 0)
					return(FAILURE);
				break;

			case 'p':	/* locale's equivalent of a.m or p.m */
				if ((match = compare(&bufptr, AM_STR, PM_STR)) == COMPFAIL)
					return(FAILURE);
				else if (match != COMPIGN) {
					am_pm = match;
					/* if we have the 12-hour time earlier, 
				 	 * convert it.
				 	 */
					if (*flagptr & FHOUR12) {
						HOUR_12_TO_24();
						/* reset flag to avoid future 
					 	 * double conversion 
					 	 */
						*flagptr &= ~FHOUR12;
					}
				}
				break;

			case 'r':	/* time as %I:%M:%S %p */
				if (!(bufptr=parsetime(bufptr, "%I:%M:%S %p", tm, flagptr, wknptr)))
					return(FAILURE);
				break;

			case 'R':	/* time as %H:%M */
				if (!(bufptr = parsetime(bufptr, "%H:%M", tm, flagptr, wknptr)))
					return(FAILURE);
				break;

			case 'S':	/* seconds, [0,61] */
				if ((tm->tm_sec = str2int(&bufptr, 0, 61)) < 0)
					return(FAILURE);
				break;

			case 'T':	/* time as %H:%M:%S */
				if (!(bufptr=parsetime(bufptr, "%H:%M:%S", tm, flagptr, wknptr)))
					return(FAILURE);
				break;

			case 'U':	/* (Sunday-first) week number, [0,53] */
				if ((*wknptr = str2int(&bufptr, 0, 53)) < 0)
					return(FAILURE);
				*flagptr |= FSUNWK;
				break;

			case 'w':	/* weekday, [0,6] */
				if ((tm->tm_wday = str2int(&bufptr, 0, 6)) < 0)
					return(FAILURE);
				*flagptr |= FWDAY;
				break;

			case 'W':	/* (Monday-first) week number, [0,53] */
				if ((*wknptr = str2int(&bufptr, 0, 53)) < 0)
					return(FAILURE);
				*flagptr |= FMONWK;
				break;

			case 'x':	/* locale's date format */
				if (!(bufptr = parsetime(bufptr, nl_langinfo(D_FMT), tm, flagptr, wknptr)))
					return(FAILURE);
				break;
	
			case 'X':	/* locale's time format */
				if (!(bufptr = parsetime(bufptr, nl_langinfo(T_FMT), tm, flagptr, wknptr)))
					return(FAILURE);
				break;

			case 'y':	/* year within century, [0,99] */
				if ((cenyear = str2int(&bufptr, 0, 99)) < 0)
					return(FAILURE);
		
				CENT_TO_YEAR();
				break;

			case 'Y':	/* year including century, [0,9999] */
				if ((i = str2int(&bufptr, 0, 9999)) < 0)
					return(FAILURE);
				tm->tm_year = i - BASEYEAR;
				*flagptr |= FYEAR;
				break;

			default:	/* entered invalid conversion spec */
				return(FAILURE);
				break;

			} /* end switch */				

			continue; 	/* examine next format character */
		} /* end if formchar = % */

		/*
		 *  Ordinary character directive
		 *  ----------------------------
		 */
		/* check and scan leading space(s) as specified earlier */
		CHECK_SCAN_SPACE();

		if (CHARADV(bufptr) != formchar)
			/* must have a matching char in buffer, else fail. */
			return(FAILURE);
				
	} /* end for loop */

	return(bufptr);		/* return current string pointer */

} /* end parsetime */


/* --------------------------
 * strptime()
 * --------------------------
 */
char *
strptime(buffer, format, tm)
	const char *buffer;	/* pointer to the input time string */
	const char *format;	/* pointer to the format string */
	struct tm *tm;		/* pointer to the resulting time value */
{
	register char *retptr;	/* return value from call to parsetime */
	int flagword = 0;	/* keep track what options specified */
	struct tm t;		/* temporary varariable */
	int wknum = 0;		/* week number of the year, either */
				/* Sunday or Monday as the first day */

	retptr = parsetime(buffer, format, tm, &flagword, &wknum);

	if (!retptr) 		/* return now if error detected */
		return(FAILURE);

	/* try to resolve the week number if specified. The week number can
	 * be resolved to the tm_yday field only if the tm_year and tm_wday
	 * fields are both known. 
	 */
	if ((flagword & FSUNWK || flagword & FMONWK) && (flagword & FYEAR) &&
	    (flagword & FWDAY)) {
		int    offset, displacement;

		/* the tm_yday is calculated by first finding out what week day
		 * the first day of the year (1st of January) is. Then use it 
		 * to figure out the number of days (offset) from the first 
		 * Sunday (or Monday), and the number of days (displacement)
		 * from the tm->wday away from Sunday (or Monday).
		 * Finally, tm_yday is wknum - 1 times 7 (7 days in a week)
		 * plus offset and displacement.
		 */
		t.tm_sec   = 0;	/* set it to the first day of the year */
		t.tm_min   = 0;
		t.tm_hour  = 0;
		t.tm_mday  = 1;
		t.tm_mon   = 0;
		t.tm_year  = tm->tm_year;
		t.tm_wday  = -1;
		t.tm_yday  = -1;
		t.tm_isdst = -1;

		/* use mktime(3C) to figure out the week day on 1/1 */
		if ((time_t)-1 != mktime(&t) || errno != ERANGE) {
			/* mktime sucess */
			offset = (flagword & FSUNWK) ? (7 - t.tm_wday) % 7 :
			         (8 - t.tm_wday) % 7;

			displacement = (flagword & FSUNWK) ? tm->tm_wday :
			               (tm->tm_wday + 6) % 7;

			/* a zero wknum stands for the days in a new year
			 * preceding the first Sunday (or Monday)
			 */
			tm->tm_yday = 7 * (wknum - 1) + offset + displacement;
		}
	} /* end resolve week number */
			
	/* If the user has given the tm_mday, tm_mon and tm_year
	 * fields, we'll figure out, using mktime(3C), the corresponding 
	 * tm_wday and tm_yday fields. For the isdst field, we also need
	 * the tm_hour info, since DST can be changed in the middle of
	 * a day (eg. 1am). 
	 */
	if ((flagword & FMDAY) && (flagword & FMON) && (flagword & FYEAR)) {
		t.tm_sec   = 0;   /* min and sec are not important here */
		t.tm_min   = 0;
		t.tm_hour  = (flagword & FHOUR) ? tm->tm_hour : 0;
		t.tm_mday  = tm->tm_mday;
		t.tm_mon   = tm->tm_mon;
		t.tm_year  = tm->tm_year;
		t.tm_wday  = -1;
		t.tm_yday  = -1;
		t.tm_isdst = -1;  /* request mktime() to return a value */

		if (((time_t)-1 != mktime(&t)) || errno != ERANGE) {
			/* mktime() success */
			tm->tm_wday = t.tm_wday;
			tm->tm_yday = t.tm_yday;
			if (flagword & FHOUR) {
				/* we also need a valid hour to figure isdst 
				 * because the value can be changed in the
				 * middle of a day. Eg. 1am. 
				 */
				tm->tm_isdst = t.tm_isdst;
			}
		}
	}
	return(retptr);
} /* end strptime */
