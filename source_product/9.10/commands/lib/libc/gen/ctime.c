/* @(#) $Revision: 66.3 $ */    
/*LINTLIBRARY*/
/*
 * This routine converts time as follows.
 * The epoch is 0000 Jan 1 1970 GMT.
 * The argument time is in seconds since then.
 * The localtime(t) entry returns a pointer to an array
 * containing
 *  seconds (0-59)
 *  minutes (0-59)
 *  hours (0-23)
 *  day of month (1-31)
 *  month (0-11)
 *  year-1970
 *  weekday (0-6, Sun is 0)
 *  day of the year
 *  daylight savings flag
 *
 * The routine corrects for daylight saving
 * time and will work in any time zone provided
 * "timezone" is adjusted to the difference between
 * Coordinated Universal Time and local standard time (measured in seconds).
 * In places like Michigan "daylight" must
 * be initialized to 0 to prevent the conversion
 * to daylight time.
 * There is a table which accounts for the peculiarities
 * undergone by daylight time in 1974-1975.
 *
 * The routine does not work
 * in Saudi Arabia which runs on Solar time.
 *
 * asctime(tvec)
 * where tvec is produced by localtime
 * returns a ptr to a character string
 * that has the ascii time in the form
 *	Thu Jan 01 00:00:00 1970n0\\
 *	01234567890123456789012345
 *	0	  1	    2
 *
 * ctime(t) just calls localtime, then asctime.
 *
 * tzset() looks for an environment variable named
 * TZ. It should be in the form:
 *      [:]STDoffset[DST[offset][,date[/time],date[/time]]]
 *
 * If the variable is present, it will set the external
 * variables "timezone", "daylight", and "tzname"
 * appropriately. It is called by localtime, and
 * may also be called explicitly by the user.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define asctime _asctime
#define ctime _ctime
#define gmtime _gmtime
#define localtime _localtime
#define tzset _tzset
#define daylight __daylight
#define timezone __timezone
#define tzname __tzname
#define getenv _getenv
#define bsearch _bsearch
#define strcmp _strcmp
#define strcpy _strcpy
#define strncmp _strncmp
#define strncpy _strncpy
#define strlen _strlen
#define close _close
#define open _open
#define read _read
#endif

#define	dysize(A) (((A)%4)? 365: 366)
#define skip_space() while(*ptr == ' ' || *ptr == '\t') ptr++;
#define TZRULECNT 140	/* the maximum number of rules: 2 yearly for 1970 to 2038 plus 2 */
#define TZBUFSIZE 128
#define INULL -1
#define LNULL -86402L	/* more seconds than in any day (leap second or not) */
#define MSECS 60L	/* number of seconds in a minute */
#define HSECS 3600L	/* number of seconds in an hour */
#define DSECS 86400L	/* number of seconds in a day */


#define MIN_HOURS          0
#define MAX_HOURS          24
#define MIN_MINUTES        0
#define MAX_MINUTES        59
#define MIN_SECONDS        0
#define MAX_SECONDS        59

#define MIN_JULIAN_DAY     1
#define MAX_JULIAN_DAY     365
#define MIN_LEAP_DAY       0
#define MAX_LEAP_DAY       365
#define MIN_M_DAY          0
#define MAX_M_DAY          6
#define MIN_M_WEEK         1
#define MAX_M_WEEK         5
#define MIN_M_MONTH        1
#define MAX_M_MONTH        12

#define STD_NAME_INDEX     0
#define DST_NAME_INDEX     1

#define EST_OFFSET         5*60*60
#define DEFAULT_DST_OFFSET 1*60*60
#define DST_CHANGE_TIME    2*60*60  /* Default time for DST to start or end */

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <values.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#  define TZLEN 	(TZNAME_MAX + 1)

#include <values.h>
#define MIN_TIME_T      ((time_t)1 << BITS(time_t) - 1)                 /* min time_t representation    */
#define MAX_TIME_T      (~MIN_TIME_T)                                   /* max time_t representation    */

/* pragma directives to enable use of underscored names for Name Space
   Pollution cleanup w/o breaking old code 
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef timezone
#pragma _HP_SECONDARY_DEF __timezone timezone
#define timezone __timezone
#endif
time_t	timezone = EST_OFFSET;

#ifdef _NAMESPACE_CLEAN
#undef daylight
#pragma _HP_SECONDARY_DEF __daylight daylight
#define daylight __daylight
#endif
int	daylight = 1;


#ifdef _NAMESPACE_CLEAN
#undef tzname
#pragma _HP_SECONDARY_DEF __tzname tzname
#define tzname __tzname
#endif
char	*tzname[2] = {"EST\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 
                      "EDT\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",};
static unsigned char	*ptr;

static int defaults_used;  /* global boolean to determine if tzset used defaults */

time_t	__sdt_dst;			/* used to pass time difference between SDT & DST to mktime() */
static time_t	local_offset = 0;	/* used to pass local time zone offset from localtime to gmtime */

static time_t tz_dst_offset;
static time_t dst_start_date;
static time_t dst_start_time;
static time_t dst_end_date;
static time_t dst_end_time;



struct tm *gmtime(), *localtime();
char	*ctime(), *asctime();
char	*getenv();
char	*bsearch();
static int	next_int();
static time_t	next_offset();
static time_t	*gmtime2();
static unsigned char	*getstrtx();
static void    shellsort();
static int	tzcmp();

void	tzset();


static char cbuf[26];
static short dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static short dmleap[12]={31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* 13 elements, so 0th is not used and others correspond to month numbers */
static short month_days[13]={0,0,31,59,90,120,151,181,212,243,273,304,334};

/* Day months start, in non leap year if Jan starts on Sunday (0) */
static short month_starts[13]={0,0,3,3,6,1,4,6,2,5,0,3,5};


/*
 * The following table is used for 1974 and 1975 and
 * gives the day number of the first day after the Sunday of the
 * change.
 */
static struct {
	int	daylb;
	int	dayle;
} daytab[] = {
	5,	333,	/* 1974: Jan 6 - last Sun. in Nov */
	58,	303,	/* 1975: Last Sun. in Feb - last Sun in Oct */
};

/*
** The tztab file  contains a  collection  of time zone  adjustment  rules.
** Each rule  specifies the first minute in which the  adjustment  applies,
** the  number of hours to adjust  GMT to local  time, and a string of the
** standard abbreviation of the time zone.
**
** Since in general  this is a yearly  process,  the year  field may have a
** range.
**
** The rule may be specified  either as a particular  day of the month (and
** the day of the week is a range of all values) or as a particular  day of
** the week (and the day of the month is a range of exactly 7 values).
** 
** In a  typical  case,  the  tztab  file  would  contain  two  rules.  One
** describes  the set of times at which summer time zone  adjustment  (read
** Daylight Savings Time) starts and another  specifies the set of times at
** which standard time resumes.  Historical data is allowed however.
*/

struct tzrange {
	int loyear;	/* First year of the range */
	int hiyear;	/* Last year in the range */
	int mon;	/* Month in which the rule first applies */
	int lomday;	/* Earliest monthday on which rule first applies */
	int himday;	/* Latest monthday on which rule applies */
	int hour;	/* Hour in which the rule first applies */
	int min;	/* Minute in which the rule first applies */
	int lowday;	/* Earliest weekday on which rule first applies */
	int hiwday;	/* Latest monthday on which rule applies */
	time_t offset;	/* Number of seconds to adjust GMT */
	char tztok[TZLEN];	/* Time zone abbreviation */
};

struct tzstart {
	time_t start;	/* First second in the UN*X epoch in which rule applies */
	time_t stop;	/* First second in the UN*X epoch in which rule does not apply */
	time_t offset;	/* Number of seconds to adjust GMT */
	char tztok[TZLEN];	/* Time zone abbreviation */
};


extern int errno;


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ctime
#pragma _HP_SECONDARY_DEF _ctime ctime
#define ctime _ctime
#endif

char *
ctime(t)
time_t	*t;
{
	return(asctime(localtime(t)));
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef localtime
#pragma _HP_SECONDARY_DEF _localtime localtime
#define localtime _localtime
#endif

struct tm *
localtime(tim)
time_t	*tim;
{
	register int dayno, daylbegin, daylend;
	register struct tm *ct;
	time_t sdt_offset, dst_offset;
	int dst;

	struct tzrange t1;
	struct tm t2;
	static struct tzstart t3[TZRULECNT];
	struct tzstart t4;
	static int t3size;
	struct tzstart *t3ptr;
	int t3index;

	int fd;
	static int tztab_ok = 0;

	time_t *tim2; /* temporary variable */
	int state; /* temporary variable */
	int year; /* temporary variable */
	unsigned char buf[TZBUFSIZE];
	static char tzbuf[TZBUFSIZE] = "";
	char *tzptr;
	register char *ptr2;
	register int n;
        int in_dst;
	
        int dst_offset_provided = 0;   /* Boolean for existence of dst offset in TZ */
	
	int errno_save;

	errno_save = errno;
	tzset();

        if (defaults_used)
	    goto notztab;
	else
	{
	    dst_offset_provided = get_dst_offset();

            if (!daylight)
	    {
                local_offset = timezone; /* timezone set to std offset in tzset */
                __sdt_dst = 0;
                ct = gmtime(tim);
		return(ct);
	    }
	    else   /* daylight time exists */
	    {
                if (*ptr != '\0')  /* More of TZ variable, Rule may follow */
		{
                    if (*ptr == ',')   /* start of rule syntax */
		    {
                       ptr++;			
                       if (!get_dst_rule(tim)) /* errors found in rule */
                           goto notztab;
                       else  /* rule had no errors (but possible defaults are needed) */
	               {

                           /* Test to see if current time is later that start of daylight   *
                            * start time, and if so is it also less than daylight end       *
                            * time plus the difference between std and dst since if in dst, *
                            * this offset must be used to correctly calculate when the      *
                            * end of dst takes place.                                       */

                           if (((dst_start_date+dst_start_time+timezone) <= *tim) && 
                               (*tim < ((dst_end_date+dst_end_time+tz_dst_offset))))
                           {    /* In daylight time */

                               in_dst = 1;
                               local_offset = tz_dst_offset;
			   }
			   else
			   {
			       in_dst = 0;
                               local_offset = timezone; /* timezone set to std offset in tzset */
			   }
			   
		           __sdt_dst = timezone - tz_dst_offset;

                           ct = gmtime(tim);
                           if (in_dst)
                               ct->tm_isdst++; 

		           return(ct);
                       }
		    }
                    else /* error in rule format (may want to ignore white space) */
                          goto notztab; 
                }
                /*
	        else no rule provided, so fall through to rest of localtime code.
                */
	    }
	}    
		



/*
** Tztab_ok  will take on three  values.  If it is 0, then no  attempt  has
** been  made to  access  the  /usr/lib/tztab  file.  If it is -1, then the
** attempt failed and no further attempt should be made.  (This failure may
** either be no file or a file which is  defective.  If it is 1, then tztab
** has already been parsed and the data should be used.
*/

	if (tztab_ok == -1)
		goto notztab;

/*
** If the TZ environment variable has changed, the /usr/lib/tztab file must
** be reparsed even though it was already parsed for a different  TZ value.
*/

	if (!(tzptr = getenv("TZ")) || !(*tzptr)) 
		goto notztab;

	if (strcmp(tzbuf, tzptr) != 0) {
		tztab_ok = 0;
		(void)strncpy(tzbuf, tzptr, TZBUFSIZE);
	}

/*
** The previous time zone  adjustment  information was retained.  If it has
** been  initialized  and is  still  valid,  then it is  used  rather  than
** reparsing the entire tztab table.
*/

	if (tztab_ok == 1) {

		t4.start = *tim;
		t3ptr = (struct tzstart *)bsearch((char *)(&t4), (char *)t3, (unsigned)t3size, sizeof(struct tzstart), tzcmp);

		local_offset = t3ptr->offset;
		ct = gmtime(tim);
		if (strcmp(tzname[STD_NAME_INDEX], t3ptr->tztok) != 0)
			ct->tm_isdst++;
		errno = errno_save;
		return(ct);
	}

/*
** The tztab table must be reparsed.
*/

	if (((fd = open("/usr/lib/tztab", O_RDONLY)) < 0)) {
		tztab_ok = -1;
		goto notztab;
	}

/*
** Initialize  the data  structures  which  will be  filled as the table is
** parsed.  This includes filling in the earliest time zone adjustment.
*/

	state = 0;
	t3ptr = t3;
	dst_offset = t3ptr->start = MIN_TIME_T;
	sdt_offset = t3ptr->offset = timezone;
	(void)strncpy(t3ptr->tztok, tzname[STD_NAME_INDEX], TZLEN);
	t3ptr++;
	t3index = 1;

/*
** Parse each line of the tztab table.
*/

	while (1) {
		if (getstrtx(buf, TZBUFSIZE, fd) == NULL)
			ptr = NULL;
		else 
		{
			ptr = buf;
                        if (*ptr == '#')
			    continue;
		}
		
/*
** If EOF is encountered  and the applicable  rule has not been found, then
** the standard American pattern is used.
*/

		if (state == 0 && ptr == NULL)
			goto badtztab;

/*
** The end of the rules  pertaining  to the time zone of interest  has been
** sensed.  This may either be caused by an EOF or by a line which begins a
** new set of time zone adjustment rules.  The correct time zone adjustment
** is applied to the time of interest.
*/

		else if (state == 1 && (ptr == NULL || valid_name_char())) {
			tztab_ok = 1;
			(void)close(fd);

			/* pass time difference between SDT & DST to mktime() */
			if (dst_offset > MIN_TIME_T)
				__sdt_dst = sdt_offset - dst_offset;
			else
				__sdt_dst = 0L;

			t3ptr->start = MAX_TIME_T;
			t3ptr->stop = MAX_TIME_T;
			t3ptr->offset = timezone;
			(void)strncpy(t3ptr->tztok, tzname[STD_NAME_INDEX], TZLEN);
			t3size = t3index + 1;

			/* Do simple shell sort */

			shellsort(t3,t3size);

			for (t3index = 1; t3index < t3size; t3index++)
				t3[t3index - 1].stop = t3[t3index].start - 1;

			t4.start = *tim;
			t3ptr = (struct tzstart *)bsearch((char *)(&t4), (char *)t3, (unsigned)t3size, sizeof(struct tzstart), tzcmp);

			local_offset = t3ptr->offset;
			ct = gmtime(tim);
			if (strcmp(tzname[STD_NAME_INDEX], t3ptr->tztok) != 0)
				ct->tm_isdst++;
			errno = errno_save;
			return(ct);
		}

/*
** This  line  of  the  table  contains  a  string  which  matches  the  TZ
** environment variable.
*/

		else if (state == 0 && valid_name_char()) {
			if (*(ptr + strlen ((char *)ptr) - 1) == '\n')
				*(ptr + strlen ((char *)ptr) - 1) = '\0';
			if (strcmp((char *)ptr, tzptr))
				continue;
			state = 1;
		}

/*
** This line of the table contains a time zone adjustment rule.
*/

		else if (state == 1 && isdigit(*ptr)) {
			if ((t1.min = next_int(0, 59)) == INULL)
				goto badtztab;
			if ((t1.hour = next_int(0, 23)) == INULL)
				goto badtztab;
			if ((t1.lomday = next_int(1, 31)) == INULL)
				goto badtztab;
			if (*ptr == '-') {
				ptr++;
				if ((t1.himday = next_int(1, 31)) == INULL)
					goto badtztab;
			}
			else
				t1.himday = t1.lomday;
			if ((t1.mon = next_int(1, 12)) == INULL)
				goto badtztab;
			if ((t1.loyear = next_int(1970, 2038)) == INULL)
				goto badtztab;
			if (*ptr == '-') {
				ptr++;
				if ((t1.hiyear = next_int(1970, 2038)) == INULL)
					goto badtztab;
			}
			else
				t1.hiyear = t1.loyear;
			if ((t1.lowday = next_int(0, 6)) == INULL)
				goto badtztab;
			if (*ptr == '-') {
				ptr++;
				if ((t1.hiwday = next_int(0, 6)) == INULL)
					goto badtztab;
			}
			else
				t1.hiwday = t1.lowday;

			skip_space();
			n = TZLEN;
			ptr2 = t1.tztok;
			while (--n > 0 && valid_name_char())
				*ptr2++ = *ptr++;
			*ptr2 = '\0';

                        if (dst_offset_provided) 
			{   /* want to use offset provided in TZ, not tztab */
			    t1.offset = tz_dst_offset;
			}
                        else  
			{
                           if ((t1.offset = next_offset(-DSECS, DSECS)) == LNULL)
				goto badtztab;
                        }
			

/*
** Insure  consistancy  between the TZ environment  variable  value and the
** rules within the tztab table.  The time zone abbreviation must match one
** of those parsed by tzset().
*/

			if ((dst = strcmp(tzname[STD_NAME_INDEX], t1.tztok)) &&
                            strcmp(tzname[DST_NAME_INDEX], t1.tztok))
				goto badtztab;

/*
** One and only one of the  weekday  and  monthday  specificatons  can be a
** range.
*/

			if (t1.hiwday != t1.lowday && t1.himday != t1.lomday)
				goto badtztab;

			if (t1.hiwday == t1.lowday && t1.himday == t1.lomday)
				goto badtztab;

/*
** The specification which is a range must span exactly 7 days.
*/

			if (t1.hiwday == t1.lowday && t1.himday - 6 != t1.lomday)
				goto badtztab;

			if (t1.himday == t1.lomday && t1.hiwday - 6 != t1.lowday)
				goto badtztab;

/*
** The current adjustment rule is free of inconsistancies and is now tested
** against the current moment of interest.
** 
** The current  adjustment rule will generate one or more adjustment  start
** times,  one for each  year in the year  range.  These  are  added to the
** table of start time.
*/

			if (dst)					/* Save so we can calculate	  */
				dst_offset = t1.offset;			/* difference between SDT and DST */

			for (year = t1.loyear - 1900; year <= t1.hiyear - 1900; year++) {

				t2.tm_year = year;
				t2.tm_mon = t1.mon - 1;
				t2.tm_mday = t1.lomday;
				t2.tm_hour = t1.hour;
				t2.tm_min = t1.min;
				t2.tm_sec = 0;

				tim2 = gmtime2(&t2);

				if (*tim2 == MIN_TIME_T)
					goto badtztab;

				*tim2 += t1.offset;

/*
** Compute the actual time if the weekday is specified  and the monthday is
** given as a range.
*/

				if (t1.lowday == t1.hiwday) {
					if (t1.lowday < t2.tm_wday)
						t2.tm_mday += 7 + t1.lowday - t2.tm_wday;
					else
						t2.tm_mday += t1.lowday - t2.tm_wday;
					tim2 = gmtime2(&t2);

					if (*tim2 == MIN_TIME_T)
						goto badtztab;

					*tim2 += t1.offset;
				}

/*
** Fill in the start times table.  See that the table size is not exceeded.
*/

				t3ptr->start = *tim2;
				t3ptr->offset = t1.offset;
				(void)strncpy(t3ptr->tztok, t1.tztok, TZLEN);
				t3ptr++;
				if (t3index++ > TZRULECNT - 2)
					goto badtztab;

			}
		}
	}

badtztab:(void)close(fd);
	tztab_ok = -1;

notztab:
	errno = errno_save;

/*
** The tztab table fails to yield the time zone  adjustment  either because
** of some problem with the information in the table or because the desired
** entry is not  present.  The  standard  American  Daylight  Savings  Time
** pattern is then used.
*/
	local_offset = timezone;
	ct = gmtime(tim);
	dayno = ct->tm_yday;
	daylbegin = 119;	/* last Sun in Apr (converted to 1st below) */
	daylend = 303;		/* Last Sun in Oct */
	if(ct->tm_year == 74 || ct->tm_year == 75) {
		daylbegin = sunday(ct, daytab[ct->tm_year-74].daylb);
		daylend = daytab[ct->tm_year-74].dayle;
	}
	else {
                daylbegin = sunday(ct, daylbegin);
                /* convert from last Sunday in April to first Sunday in April */
                if (daylbegin >= 118)  /* If April 29th or 30th */
		    daylbegin -= 28;   /* Subtract 4 weeks */
		else                   /* April 24 - 28 */
		    daylbegin -= 21;   /* Subtract 3 weeks */
        }
	daylend = sunday(ct, daylend);
	if(daylight &&
	    (dayno>daylbegin || (dayno==daylbegin && ct->tm_hour>=2)) &&
	    (dayno<daylend || (dayno==daylend && ct->tm_hour<1))) {
		local_offset = timezone - 1*60*60;
		ct = gmtime(tim);
		ct->tm_isdst++;
	} 
        __sdt_dst = (daylight) ? DEFAULT_DST_OFFSET : 0;
             
	return(ct);
}

/*
 * The argument is a 0-origin day number.
 * The value is the day number of the last
 * Sunday on or before the day.
 */

static int
sunday(t, d)
register struct tm *t;
register int d;
{
	if(d > 58)
		d += dysize(t->tm_year) - 365;
	return(d - (d - t->tm_yday + t->tm_wday + 700) % 7);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef gmtime
#pragma _HP_SECONDARY_DEF _gmtime gmtime
#define gmtime _gmtime
#endif

struct tm __tm_xtime;	/* this variable is referenced in mktime.c */

struct tm *
gmtime(tim)
time_t	*tim;
{
	register int d0, d1;
	time_t hms, day;

	/*
	 * break initial number into days
	 */
	hms = *tim % 86400L;
	day = *tim / 86400L;

	/*
	 *  Subtract any time zone or Daylight Savings Time offset
	 *  as specified by localtime().  This is done in this manner
	 *  (as opposed to just subtracting the offset from *tim)
	 *  because *tim may be at the limits of its range and we
	 *  don't want to have any wrap around.
	 */
	hms -= local_offset;
	local_offset = 0;

	if (hms >= 86400L) {
		hms -= 86400L;
		day++;
	}

	while (hms < 0) {
		hms += 86400L;
		day -= 1;
	}
	/*
	 * generate hours:minutes:seconds
	 */
	__tm_xtime.tm_sec = hms % 60;
	d1 = hms / 60;
	__tm_xtime.tm_min = d1 % 60;
	d1 /= 60;
	__tm_xtime.tm_hour = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	__tm_xtime.tm_wday = (day + 7340036L) % 7;

	/*
	 * year number
	 */
	if(day >= 0)
		for(d1=70; day >= dysize(d1); d1++)
			day -= dysize(d1);
	else
		for(d1=70; day < 0; d1--)
			day += dysize(d1-1);
	__tm_xtime.tm_year = d1;
	__tm_xtime.tm_yday = d0 = day;

	/*
	 * generate month
	 */

	if(dysize(d1) == 366)
		for(d1=0; d0 >= dmleap[d1]; d1++)
			d0 -= dmleap[d1];
	else
		for(d1=0; d0 >= dmsize[d1]; d1++)
			d0 -= dmsize[d1];
	__tm_xtime.tm_mday = d0+1;
	__tm_xtime.tm_mon = d1;
	__tm_xtime.tm_isdst = 0;
	return(&__tm_xtime);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef asctime
#pragma _HP_SECONDARY_DEF _asctime asctime
#define asctime _asctime
#endif

char *
asctime(t)
struct tm *t;
{
	register char *cp, *ncp;
	register int *tp;
	char	*ct_numb();

	cp = cbuf;
	for(ncp = "Day Mon 00 00:00:00 1900\n"; *cp++ = *ncp++; );
	ncp = &"SunMonTueWedThuFriSat"[3*t->tm_wday];
	cp = cbuf;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	cp++;
	tp = &t->tm_mon;
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[(*tp)*3];
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	cp = ct_numb(cp, *--tp);
	cp = ct_numb(cp, *--tp+100);
	cp = ct_numb(cp, *--tp+100);
	cp = ct_numb(cp, *--tp+100);
	if(t->tm_year >= 100) {
		cp[1] = '2';
		cp[2] = '0';
	}
	cp += 2;
	cp = ct_numb(cp, t->tm_year+100);
	return(cbuf);
}

static char *
ct_numb(cp, n)
register char *cp;
int	n;
{
	cp++;
	if(n >= 10)
		*cp++ = (n/10)%10 + '0';
	else
		*cp++ = ' ';
	*cp++ = n%10 + '0';
	return(cp);
}


/*
** next_int returns an integer between lower and upper.
*/

static int
next_int(lower, upper)
int lower,upper;
{
	int num = 0;
	skip_space();
	if (!isdigit(*ptr))
		return(INULL);
	do { 
		num = num*10 + (*ptr-'0'); 
	} while (isdigit(*++ptr));
	if ((num < lower) || (num > upper)) 
		return(INULL); 

	return(num);
}


/*
** next_offset  returns  an time  zone  offset  between  lower  and  upper.
** Fractional hours are parsed as HH:MM.
*/

static time_t
next_offset(lower, upper)
time_t lower,upper;
{

	register int n;
	int sign = 0;
	time_t offset;
	time_t num1 = 0L;
	time_t num2 = 0L;

	skip_space();

	if (*ptr == '+')
		ptr++;
	if (*ptr == '-') {
		sign++;
		ptr++;
	}

	if (!isdigit(*ptr))
		return(0L); 

	do { 
		num1 = num1 * 10L + (*ptr-'0'); 
	} while (isdigit(*++ptr));

	if (*ptr == ':') {
		ptr++;

		n = 2;
		while (n-- > 0) {
			if (!isdigit(*ptr))
				return(LNULL); 
			num2 = num2 * 10L + (*ptr++ - '0'); 
		}

		offset = (HSECS * num1 + HSECS * num2 / MSECS);
	}
	else
		offset = HSECS * num1;

	if (sign)
		offset = -offset;

	if ((offset < lower) || (offset > upper)) 
		return(LNULL); 

	return(offset);
}


/*
** Perform  essentially the inverse of the gmtime() operation.  Take a time
** as stored in the  struct tm and  convert it into the  number of  seconds
** since the UN*X meridian.  The tm_wday,  tm_yday, and tm_isdst fields are
** ignored for input.  The first two are recomputed  from the values of the
** other fields, the last is simply set to zero as it is in gmtime().
** 
** If one of the fields of the tm  structure  is out of range, the earliest
** possible value is returned (MIN_TIME_T).
*/

static time_t *
gmtime2(xtime)
struct tm	*xtime;
{
	register int d0, d1;
	time_t hms, day;
	static time_t tim;

/*
** Verify that the month has the correct number of days.
*/

	tim = 0;

	if (dysize(xtime->tm_year) == 366) {
		if (xtime->tm_mday > dmleap[xtime->tm_mon])
			tim = MIN_TIME_T;
	}
	else {
		if (xtime->tm_mday > dmsize[xtime->tm_mon])
			tim = MIN_TIME_T;
	}

	if (xtime->tm_hour > 23)
		tim = MIN_TIME_T;

	if (xtime->tm_min > 59)
		tim = MIN_TIME_T;

	if (xtime->tm_sec > 59)
		tim = MIN_TIME_T;

	if (tim == MIN_TIME_T)
		return(&tim);

/*
** Number of seconds into the given day.
*/

	hms = ((xtime->tm_hour) * 60 + xtime->tm_min) * 60 + xtime->tm_sec;

/*
** Number of days since or until the UN*X meridian.
*/

	day = xtime->tm_mday - 1;

	if (dysize(xtime->tm_year) == 366)
		for (d0 = 0; d0 < xtime->tm_mon; d0++)
			day += dmleap[d0];
	else
		for (d0 = 0; d0 < xtime->tm_mon; d0++)
			day += dmsize[d0];

	xtime->tm_yday = day;

	if (xtime->tm_year >= 70)
		for (d1 = 70; d1 < xtime->tm_year; d1++)
			day += dysize(d1);
	else
		for (d1 = 70; d1 > xtime->tm_year; d1--)
			day -= dysize(d1 - 1);

	xtime->tm_wday = (day + 7340036L) % 7;
	xtime->tm_isdst = 0;

	tim = day * 86400L + hms;
	return(&tim);
}


static unsigned char *
getstrtx(s, n, fd)
unsigned char *s;
int n;
int fd;
{
	register i = 0;

	static int current = 0;
	static int count = 0;
	static unsigned char txbuf[TZBUFSIZE];

	for(;;) {
		if (current >= count) {
			current = 0;
			if ((count = read(fd, (char *)txbuf, TZBUFSIZE)) <= 0) {
				s[i] = '\0';
				if (i == 0) 
					return(NULL); /* read error or EOF */
				else 
					return(s);
			}
		}
		if ((s[i] = txbuf[current++]) == '\n') {
			s[i] = '\0';
			return(s);
		}
		if (i < n) i++;
	}
}

static void
shellsort(tzruleptr,nel)
	struct tzstart *tzruleptr;
	int nel;
{
	register int gap, i, j;
	register struct tzstart *tr1, *tr2;
	struct tzstart tzruleswap;

	for (gap = nel / 2; gap > 0; gap /= 2) {
		for (i = gap; i < nel; i++) {
			for (j = i - gap; j >= 0; j -= gap) {

				tr1 = tzruleptr + j;
				tr2 = tr1 + gap;
				if (tr1->start <= tr2->start)
					break;

				/* swap elements (using  */
				/* structure assignment) */

				tzruleswap = *tr1;
				*tr1 = *tr2;
				*tr2 = tzruleswap;
			}
		}
	}
}

static int
tzcmp(tz1, tz2)
struct tzstart *tz1, *tz2;

{
	if (tz1->start < tz2->start)
		return (-1);
	if (tz1->start > tz2->stop)
		return (1);
	return (0);
}



/* Valid_name_char tests for the characters that are allowed as         *
 * part of a valid TZ name.                                             */

static int
valid_name_char()
{
    if ((*ptr == ',') || (*ptr == '+') || (*ptr == '-') || (*ptr == '\0') ||
        isdigit(*ptr))
        return(0);
    else
        return(1);
}




/* get_name parses the string pointed to by ptr to get the longest       *
 * valid TZ name.  It fills in the proper tzname entry and returns 1     *
 * if valid, else 0 if invalid.                                          */

static int
get_name(index)
int index;

{
    int size = 0;
    char *tmp_ptr;
    
    tmp_ptr = tzname[index];
    
    while  (valid_name_char()) 
    {
	     size++;
             if (size <= TZNAME_MAX)
                *tmp_ptr++ = *ptr++;
             else
                ptr++;   
    }
    *tmp_ptr = '\0';    

    if (size < _POSIX_TZNAME_MAX)
        return(0);
    else
        return(1);
}


	

/*  Get_num returns the longest integer possible starting at ptr.  It is   *
 *  assumed that isdigit(*ptr) is true.                                    */

static int 
get_num()
{
    time_t num = 0L;

    do 
    {
       num = num * 10L + (*ptr-'0');
    } while (isdigit(*++ptr));
    return(num);
}


/* Parse_time calculates the number of seconds and sets the value in the   *
 * variable pointed to by the argument passed in.  If an hour value is not *
 * even provided, or a value is out of range, zero is returned, else       *
 * non-zero is returned.                                                   */

static int
parse_time(time_ptr)
time_t *time_ptr;
{
    int hours, minutes, seconds;
    
    if (isdigit(*ptr))
    {
       hours = get_num();

       if ((hours < MIN_HOURS) || (hours > MAX_HOURS))
            return(0);
    
       if (*ptr == ':') 
       {
	   ptr++;
         
           if (isdigit(*ptr)) 
           {
              minutes = get_num();

              if ((minutes < MIN_MINUTES) || (minutes > MAX_MINUTES))
                   return(0);
    
              if (*ptr == ':')
              {
		  ptr++;
		  
                  if (isdigit(*ptr))
		  {
                      seconds = get_num();

                      if ((seconds < MIN_SECONDS) || (seconds > MAX_SECONDS))
                           return(0);
                  }
                  else
                     seconds = 0;
              }
              else
                 seconds = 0;
           }
           else
           {
              minutes = seconds = 0;
           }
       }
       else
       {
           minutes = seconds = 0;
       }
       *time_ptr = seconds + (60 * minutes) + (3600 * hours);
       return(1);
   }
   else
      return(0);
}




/* get_std_offset fills in std_offset and returns non-zero if valid,    *
 * and returns zero if invalid.                                         */

static int 
get_std_offset() 
{
    time_t num = 1L;

    if (*ptr == '\0')   /* This was added to allow SVVD tests to pass in the */
    {                   /* case of a STD but no offset provided.  It will    */
                        /* be set to 0.   PKS.                               */
         timezone = 0;
         return(1);
    }

    if (*ptr == '+')
         ptr++;
    else if (*ptr == '-') 
    {
	 num *= -1;
         ptr++;
    }
    
    if (parse_time(&timezone))   /* At least hour field was supplied */
    {
        timezone *= num;
        return(1);
    }
    else
        return(0);
}



/* get_dst_offset fills in tz_dst_offset and returns non-zero if valid, or  *
 * sets tz_dst_offset to timezone plus 1-hour and returns zero if invalid   */

static int
get_dst_offset()
{
    time_t num = 1L;
    
    if (*ptr == '+')
         ptr++;
    else if (*ptr == '-') 
    {
         num *= -1;
         ptr++;
    }
    
    if (parse_time(&tz_dst_offset)) /* At least hour field was supplied */
    {
        tz_dst_offset *= num;
        return(1);
    }
    else
    {
        tz_dst_offset = timezone - DEFAULT_DST_OFFSET;
        return(0); 
    }
    
}



/* Get_dst_date determines the type of date format used and returns the      *
 * number of seconds since the epoch if all data is valid, else returns -1   */


static int
get_dst_date(tim, date_ptr)
time_t *tim;  /* passed in to localtime */
time_t *date_ptr;

{
    int day;
    int week;
    int month;
    int year;
    int year_days = 0;
    struct tm *ct;
    int leap_day = 0; /* set to 1 if on or past leap day in a leap year */
    int jan_1;   /* day of week that Jan 1st falls on */
    int num_days; /* tmp holder of number of days up to the date given */
    int last_day; /* day of week that the last day of the month falls on */
    int day_month_starts;  /* day of week for first day of month */

    local_offset = timezone;
    ct = gmtime(tim);    

    for (year=ct->tm_year; year>70; year--)
         year_days += dysize(year-1);
    
    switch (*ptr) {	    

    case 'J': 
	ptr++;
	if (isdigit(*ptr)) 
	{
           day = get_num();
           if ((day < MIN_JULIAN_DAY) || (day > MAX_JULIAN_DAY)) 
	      return(0);
	   else 
	   {
	      if ((dysize(ct->tm_year) == 365) || (ct->tm_yday < 59)) 
	      {    /* not a leap year or else it                          *
                    * is before Feb 29 (59th day (Jan 1st is 0th)) anyway */

                  *date_ptr = (((day-1) + year_days) * DSECS);
                  return(1); 
	      }
	      else
	          *date_ptr = ((day + year_days) * DSECS);
	      return(1); 
	   }
	   
        }
	else
           return(0); /* improper date format */
	/*NOTREACHED*/
	break;
    
    case 'M':
	ptr++;
	if (isdigit(*ptr)) 
	{
            month = get_num();
            if ((month < MIN_M_MONTH) || (month > MAX_M_MONTH)) 
	       return(0);
        }
	else
	   return(0); /* improper date format */

	if (*ptr == '.')
        {
            ptr++;
            if (isdigit(*ptr)) 
	    {
                week = get_num();
                
                if ((week < MIN_M_WEEK) || (week > MAX_M_WEEK)) 
	           return(0);
            }
            else
                return(0); /* improper date format */

            if (*ptr == '.')
            {
                ptr++;
                if (isdigit(*ptr)) 
		{
                    day = get_num();

                    if ((day < MIN_M_DAY) || (day > MAX_M_DAY)) 
	               return(0);
	            else
		    {

                       jan_1 = -(sunday(ct,0));  /* day of week Jan 1 fell on */
                    
                       if ((dysize(ct->tm_year) == 366) &&
                           ((month > 2) || ((month == 2) && (week == 5) &&
                                            (day == ((jan_1 + 3) % 7)))))
                           leap_day = 1;
                      
                       day_month_starts = (month_starts[month] + jan_1 + leap_day) % 7;

                       if (week == 5) 
                       {
                           last_day = (day_month_starts + dmsize[month-1] -1) % 7;
                    
                           if (last_day >= day)
                               num_days = month_days[month] + (dmsize[month-1] - (last_day - day) - 1) + leap_day;
                           else
		               num_days = month_days[month] + (dmsize[month-1] - (7 - (day - last_day) + 1)) + leap_day; 
		       }
                       else 
                       {
                           if (day >= day_month_starts)
                               num_days = month_days[month] + 7*(week-1) + day - day_month_starts + leap_day;
                           else
                               num_days = month_days[month] + 7*week - day_month_starts + day + leap_day;
		       }
                       *date_ptr = ((num_days + year_days) * DSECS);
                       return(1);
		   }
		}
                else
                    return(0); /* improper date format */
            }
            else
                return(0); /* improper date format */
        }
        else 
            return(0); /* improper date format */
	/*NOTREACHED*/
        break;

    default:
	if (isdigit(*ptr))
        {
            day = get_num();

            if ((day < MIN_LEAP_DAY) || (day > MAX_LEAP_DAY)) 
                return(0);
	    else 
	    {
                *date_ptr = ((day + year_days) * DSECS);
                return(1); 
	    }
        }
        else
            return(0); /* improper date format */
    }
    return(1);
}


/* get_dst_rule calls routines to fill in the dst_(start/end)_(date/time)   *
 * and returns non-zero if valid, else returns 0.                           */

static int
get_dst_rule(tim)         /*  returns 0 if valid,  non-zero if invalid  */
time_t *tim;  /* passed in to localtime */
{
   int tmp_return=1;
    
   if (get_dst_date(tim,&dst_start_date))  /* Data is ok, so it will fill in proper fields */
   {
      if (*ptr == '/')  /* Start time is being supplied */
      {
         ptr++;
         tmp_return = parse_time(&dst_start_time); 
                      /* fills in & returns non_zero if valid */
      }
      else
      {
         dst_start_time = DST_CHANGE_TIME;
      }
            
      if ((*ptr == ',') && tmp_return)
      {
	  ptr++;
	  if (get_dst_date(tim,&dst_end_date)); /* returns 0 if invalid */
          {
             if (*ptr == '/')  /* End time is being supplied */
             {
                ptr++;
                tmp_return = parse_time(&dst_end_time);
                     /* fills in & returns non-zero if ok */
             }
	     else
             {
                dst_end_time = DST_CHANGE_TIME;
             }
	  
             return(tmp_return);  /* Non-zero if valid */
	  }
      }
  }
  /* Haven't returned yet, so bad startdate or no comma preceeding end date */
  return(0);
}    
      


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef tzset
#pragma _HP_SECONDARY_DEF _tzset tzset
#define tzset _tzset
#endif


void
tzset()
{
    defaults_used = 0;
    
    if (ptr = (unsigned char *)getenv("TZ")) {
        /* TZ is set to something, possibly empty string */
	if (*ptr == ':')  /* Implementation defined TZ format, ignore ':' */ 
	    ptr++;

	if (get_name(STD_NAME_INDEX) && get_std_offset()) {
            /* Both are required and both are valid */
	    if (!(daylight = get_name(DST_NAME_INDEX))) 
                *tzname[DST_NAME_INDEX] = '\0';
	    
	}
        else
        {
            (void)strcpy(tzname[STD_NAME_INDEX],"EST");
	    (void)strcpy(tzname[DST_NAME_INDEX],"EDT");
	    daylight = 1;
	    timezone = EST_OFFSET;
	    
            defaults_used = 1;    
        }
    }
    else
    {
        (void)strcpy(tzname[STD_NAME_INDEX],"EST");
        (void)strcpy(tzname[DST_NAME_INDEX],"EDT");
        daylight = 1;
        timezone = EST_OFFSET;

        defaults_used = 1;    
    }    
}
 
