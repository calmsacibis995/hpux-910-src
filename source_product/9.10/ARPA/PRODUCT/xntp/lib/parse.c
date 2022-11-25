# if defined(REFCLOCK) && defined(PARSE)
/*
 * $Header: parse.c,v 1.2.109.2 94/10/31 10:33:14 mike Exp $
 *  
 * parse.c,v 3.2 1993/07/09 11:37:11 kardel Exp
 *
 * Parser module for reference clock
 *
 * STREAM define switches between two personalities of the module
 * if STREAM is defined this module can be used with dcf77sync.c as
 * a STREAMS kernel module. In this case the time stamps will be
 * a struct timeval.
 * when STREAM is not defined NTP time stamps will be used.
 *
 * Copyright (c) 1992,1993
 * Frank Kardel Friedrich-Alexander Universitaet Erlangen-Nuernberg
 *                                    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * parse.c,v
 * Revision 3.2  1993/07/09  11:37:11  kardel
 * Initial restructured version + GPS support
 *
 * Revision 3.1  1993/07/06  10:00:08  kardel
 * DCF77 driver goes generic...
 *
 *
 */

# 	if	!(defined(lint) || defined(__GNUC__))
static char     rcsid[] = "parse.c,v 3.2 1993/07/09 11:37:11 kardel Exp";
# 	endif			/* defined (lint) ||
				   defined(__GNUC__)) */

# 	include "sys/types.h"
# 	include "sys/time.h"
# 	include "sys/errno.h"

# 	include "ntp_fp.h"
# 	include "ntp_unixtime.h"
# 	include "ntp_calendar.h"

# 	include "parse.h"

# 	ifdef STREAM
# 		include "sys/parsestreams.h"
# 	endif			/* STREAM */

extern clockformat_t   *clockformats[];
extern int  nformats;

static unsigned     LONG timepacket ();

/*
 * strings support usually not in kernel - duplicated, but what the heck
 */
int
Strlen (s)
register char  *s;
{
    register int    c;

    c = 0;
    if (s)
        {
	while (*s++)
	    {
	    c++;
	    }
        }
    return c;
}

int
Strcmp (s, t)
register char  *s;
register char  *t;
{
    register int    c = 0;

    if (!s || !t || (s == t))
        {
	return 0;
        }

    while (!(c = *s++ - *t++) && *s && *t)
	 /* empty loop */ ;

    return c;
}

static int
timedout (parseio, ctime)
register parse_t   *parseio;
register TIMESTAMP *ctime;
{
    struct timeval  delta;

# 	ifdef STREAM
    delta.tv_sec = ctime -> tv_sec - parseio -> parse_lastchar.tv_sec;
    delta.tv_usec = ctime -> tv_usec - parseio -> parse_lastchar.tv_usec;
    if (delta.tv_usec < 0)
        {
	delta.tv_sec -= 1;
	delta.tv_usec += 1000000;
        }
# 	else			/* ! STREAM */
    extern LONG     tstouslo[];
    extern LONG     tstousmid[];
    extern LONG     tstoushi[];

    l_fp    delt;

    delt = *ctime;
    L_SUB (&delt, &parseio -> parse_lastchar);
    TSTOTV (&delt, &delta);
# 	endif			/* STREAM */

    if (timercmp (&delta, &parseio -> parse_timeout, >))
        {
	parseprintf (DD_PARSE, ("parse: timedout: TRUE\n"));
	return 1;
        }
    else
        {
	parseprintf (DD_PARSE, ("parse: timedout: FALSE\n"));
	return 0;
        }
}

/*
 * setup_bitmaps
 */
static int
setup_bitmaps (parseio, low, high)
register parse_t   *parseio;
register int    low;
register int    high;
{
    register int    i;
    register int    f = 0;
    register clockformat_t *fmt;

    if ((low >= high) ||
	    (low < 0) ||
	    (high > nformats))
        {
	parseprintf (DD_PARSE, ("setup_bitmaps: failed: bounds error (low=%d, high=%d, nformats=%d)\n", low, high, nformats));
	return 0;
        }

    bzero (parseio -> parse_startsym, sizeof (parseio -> parse_startsym));
    bzero (parseio -> parse_endsym, sizeof (parseio -> parse_endsym));
    bzero (parseio -> parse_syncsym, sizeof (parseio -> parse_syncsym));
    parseio -> parse_syncflags = 0;
    parseio -> parse_timeout.tv_sec = 0;
    parseio -> parse_timeout.tv_usec = 0;

    /* 
     * gather bitmaps of possible start and end values
     */
    for (i = low; i < high; i++)
        {
	fmt = clockformats[i];

	if (fmt -> flags & F_START)
	    {
	    if (parseio -> parse_endsym[fmt -> startsym / 8] & (1 << (fmt -> startsym % 8)))
	        {
		printf ("parse: setup_bitmaps: failed: START symbol collides with END symbol (format %d)\n", i);
		return 0;
	        }
	    else
	        {
		parseio -> parse_startsym[fmt -> startsym / 8] |= (1 << (fmt -> startsym % 8));
		f = 1;
	        }
	    }

	if (fmt -> flags & F_END)
	    {
	    if (parseio -> parse_startsym[fmt -> endsym / 8] & (1 << (fmt -> endsym % 8)))
	        {
		printf ("parse: setup_bitmaps: failed: END symbol collides with START symbol (format %d)\n", i);
		return 0;
	        }
	    else
	        {
		parseio -> parse_endsym[fmt -> endsym / 8] |= (1 << (fmt -> endsym % 8));
		f = 1;
	        }
	    }
	if (fmt -> flags & SYNC_CHAR)
	    {
	    parseio -> parse_syncsym[fmt -> syncsym / 8] |= (1 << (fmt -> syncsym % 8));
	    }
	parseio -> parse_syncflags |= fmt -> flags & (SYNC_START | SYNC_END | SYNC_CHAR | SYNC_ONE | SYNC_ZERO | SYNC_TIMEOUT | SYNC_SYNTHESIZE);

	if ((fmt -> flags & SYNC_TIMEOUT) &&
		((parseio -> parse_timeout.tv_sec || parseio -> parse_timeout.tv_usec) ? timercmp (&parseio -> parse_timeout, &fmt -> timeout, >) : 1))
	    {
	    parseio -> parse_timeout = fmt -> timeout;
	    }

	if (parseio -> parse_dsize < fmt -> length)
	    parseio -> parse_dsize = fmt -> length;
        }

    if (!f && (high - low > 1))
        {
	/* 
	 * need at least one start or end symbol
	 */
	printf ("parse: setup_bitmaps: failed: neither START nor END symbol defined\n");
	return 0;
        }

    return 1;
}

/*ARGSUSED*/
int
parse_ioinit (parseio)
register parse_t   *parseio;
{
    parseprintf (DD_PARSE, ("parse_iostart\n"));

    if (!setup_bitmaps (parseio, 0, nformats))
	return 0;

    parseio -> parse_data = MALLOC (parseio -> parse_dsize * 2 + 2);
    if (!parseio -> parse_data)
        {
	parseprintf (DD_PARSE, ("init failed: malloc for data area failed\n"));
	return 0;
        }

    /* 
     * leave room for '\0'
     */
    parseio -> parse_ldata = parseio -> parse_data + parseio -> parse_dsize + 1;
    parseio -> parse_lformat = 0;
    parseio -> parse_badformat = 0;
    parseio -> parse_ioflags = PARSE_IO_CS7;
    /* usual unix default */
    parseio -> parse_flags = 0;	/* true samples */
    parseio -> parse_index = 0;
    parseio -> parse_ldsize = 0;

    return 1;
}

/*ARGSUSED*/
void
parse_ioend (parseio)
register parse_t   *parseio;
{
    parseprintf (DD_PARSE, ("parse_ioend\n"));
    if (parseio -> parse_data)
	FREE (parseio -> parse_data, parseio -> parse_dsize * 2 + 2);
}

/*ARGSUSED*/
int
parse_ioread (parseio, ch, ctime)
register parse_t   *parseio;
register unsigned char  ch;
register TIMESTAMP *ctime;
{
    register unsigned   updated = CVT_NONE;
    register int    low,
                    high;

    parseprintf (DD_PARSE, ("parse_ioread(0x%x, char=0x%x, ..., ...)\n", (unsigned int)parseio, ch & 0xFF));

    if (parseio -> parse_flags & PARSE_FIXED_FMT)
        {
	if (!clockformats[parseio -> parse_lformat] -> convert)
	    {
	    parseprintf (DD_PARSE, ("parse_ioread: input dropped.\n"));
	    return CVT_NONE;
	    }
	low = parseio -> parse_lformat;
	high = low + 1;
        }
    else
        {
	low = 0;
	high = nformats;
        }

    /* 
     * within STREAMS CSx (x < 8) chars still have the upper bits set
     * so we normalize the characters by masking unecessary bits off.
     */
    switch (parseio -> parse_ioflags & PARSE_IO_CSIZE)
        {
	case PARSE_IO_CS5:
	    ch &= 0x1F;
	    break;

	case PARSE_IO_CS6:
	    ch &= 0x3F;
	    break;

	case PARSE_IO_CS7:
	    ch &= 0x7F;
	    break;

	case PARSE_IO_CS8:
	    break;
        }

    if ((parseio -> parse_syncflags & SYNC_CHAR) &&
	    (parseio -> parse_syncsym[ch / 8] & (1 << (ch % 8))))
        {
	register clockformat_t *fmt;
	register int    i;
	/* 
	 * got a sync event - call sync routine
	 */

	for (i = low; i < high; i++)
	    {
	    fmt = clockformats[i];

	    if ((fmt -> flags & SYNC_CHAR) &&
		    (fmt -> syncsym == ch))
	        {
		parseprintf (DD_PARSE, ("parse_ioread: SYNC_CHAR event\n"));
		if (fmt -> syncevt)
		    fmt -> syncevt (parseio, ctime, fmt -> data, SYNC_CHAR);
	        }
	    }
        }

    if (((parseio -> parse_syncflags & SYNC_START) &&
		(parseio -> parse_startsym[ch / 8] & (1 << (ch % 8))) ||
		(parseio -> parse_index == 0)) ||
	    ((parseio -> parse_syncflags & SYNC_TIMEOUT) &&
		timedout (parseio, ctime)))
        {
	register int    i;
	/* 
	 * packet start - re-fill buffer
	 */
	if (parseio -> parse_index)
	    {
	    /* 
	     * filled buffer - thus not end character found
	     * do processing now
	     */
	    parseio -> parse_data[parseio -> parse_index] = '\0';

	    updated = timepacket (parseio);
	    bcopy (parseio -> parse_data, parseio -> parse_ldata, parseio -> parse_index + 1);
	    parseio -> parse_ldsize = parseio -> parse_index + 1;
	    if (parseio -> parse_syncflags & SYNC_TIMEOUT)
		parseio -> parse_dtime.parse_stime = *ctime;
	    }

	/* 
	 * could be a sync event - call sync routine if needed
	 */
	if (parseio -> parse_syncflags & SYNC_START)
	    for (i = low; i < high; i++)
	        {
		register clockformat_t *fmt = clockformats[i];

		if ((parseio -> parse_index == 0) ||
			((fmt -> flags & SYNC_START) && (fmt -> startsym == ch)))
		    {
		    parseprintf (DD_PARSE, ("parse_ioread: SYNC_START event\n"));
		    if (fmt -> syncevt)
			fmt -> syncevt (parseio, ctime, fmt -> data, SYNC_START);
		    }
	        }
	parseio -> parse_index = 1;
	parseio -> parse_data[0] = ch;
	parseprintf (DD_PARSE, ("parse: parse_ioread: buffer start\n"));
        }
    else
        {
	register int    i;

	if (parseio -> parse_index < parseio -> parse_dsize)
	    {
	    /* 
	     * collect into buffer
	     */
	    parseprintf (DD_PARSE, ("parse: parse_ioread: buffer[%d] = 0x%x\n", parseio -> parse_index, ch));
	    parseio -> parse_data[parseio -> parse_index++] = ch;
	    }

	if ((parseio -> parse_endsym[ch / 8] & (1 << (ch % 8))) ||
		(parseio -> parse_index >= parseio -> parse_dsize))
	    {
	    /* 
	     * packet end - process buffer
	     */
	    if (parseio -> parse_syncflags & SYNC_END)
		for (i = low; i < high; i++)
		    {
		    register clockformat_t *fmt = clockformats[i];

		    if ((fmt -> flags & SYNC_END) && (fmt -> endsym == ch))
		        {
			parseprintf (DD_PARSE, ("parse_ioread: SYNC_END event\n"));
			if (fmt -> syncevt)
			    fmt -> syncevt (parseio, ctime, fmt -> data, SYNC_END);
		        }
		    }
	    parseio -> parse_data[parseio -> parse_index] = '\0';
	    updated = timepacket (parseio);
	    bcopy (parseio -> parse_data, parseio -> parse_ldata, parseio -> parse_index + 1);
	    parseio -> parse_ldsize = parseio -> parse_index + 1;
	    parseio -> parse_index = 0;
	    parseprintf (DD_PARSE, ("parse: parse_ioread: buffer end\n"));
	    }
        }

    if ((updated == CVT_NONE) &&
	    (parseio -> parse_flags & PARSE_FIXED_FMT) &&
	    (parseio -> parse_syncflags & SYNC_SYNTHESIZE) &&
	    ((parseio -> parse_dtime.parse_status & CVT_MASK) == CVT_OK) &&
	    clockformats[parseio -> parse_lformat] -> synth)
        {
	updated = clockformats[parseio -> parse_lformat] -> synth (parseio, ctime);
        }

    /* 
     * remember last character time
     */
    parseio -> parse_lastchar = *ctime;

# 	ifdef DEBUG
    if ((updated & CVT_MASK) != CVT_NONE)
	parseprintf (DD_PARSE, ("parse_ioread: time sample accumulated (status=0x%x)\n", updated));
# 	endif			/* DEBUG */

    parseio -> parse_dtime.parse_status = updated;

    return (updated & CVT_MASK) != CVT_NONE;
}

/*
 * parse_iopps
 *
 * take status line indication and derive synchronisation information
 * from it.
 * It can also be used to decode a serial serial data format (such as the
 * ONE, ZERO, MINUTE sync data stream from DCF77)
 */
/*ARGSUSED*/
int
parse_iopps (parseio, status, ptime)
register parse_t   *parseio;
register int    status;
register struct timeval    *ptime;
{
    register unsigned   updated = CVT_NONE;

    /* 
     * PPS pulse information will only be delivered to ONE clock format
     * this is either the last successful conversion module with a ppssync
     * routine, or a fixed format with a ppssync routine
     */
    parseprintf (DD_PARSE, ("parse_iopps: STATUS %s\n", (status == SYNC_ONE) ? "ONE" : "ZERO"));

    if (((parseio -> parse_flags & PARSE_FIXED_FMT) ||
		((parseio -> parse_dtime.parse_status & CVT_MASK) == CVT_OK)) &&
	    clockformats[parseio -> parse_lformat] -> syncpps &&
	    (status & clockformats[parseio -> parse_lformat] -> flags))
        {
	updated = clockformats[parseio -> parse_lformat] -> syncpps (parseio, status == SYNC_ONE, ptime);
	parseprintf (DD_PARSE, ("parse_iopps: updated = 0x%x\n", updated));
        }
    else
        {
	parseprintf (DD_PARSE, ("parse_iopps: STATUS dropped\n"));
        }

    return (updated & CVT_MASK) != CVT_NONE;
}

/*
 * parse_iodone
 *
 * clean up internal status for new round
 */
/*ARGSUSED*/
void
parse_iodone (parseio)
register parse_t   *parseio;
{
    /* 
     * we need to clean up certain flags for the next round
     */
    parseio -> parse_dtime.parse_state = 0;
    /* no problems with ISRs */
}

/*---------- conversion implementation --------------------*/

/*
 * convert a struct clock to UTC since Jan, 1st 1970 0:00 (the UNIX EPOCH)
 */
# 	define dysize(x)	((x) % 4 ? 365 : ((x % 400) ? 365 :366))

time_t
parse_to_unixtime (clock, cvtrtc)
register clocktime_t   *clock;
register unsigned   LONG *cvtrtc;
{
# 	define SETRTC(_X_)	{ if (cvtrtc) *cvtrtc = (_X_); }
    static int  days_of_month[] =
        {
	0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
    register int    i;
    time_t  t;

    if (clock -> year < 100)
	clock -> year += 1900;

    if (clock -> year < 1970)
	clock -> year += 100;	/* XXX this will do it till
				   <2070 */

    if (clock -> year < 0)
        {
	SETRTC (CVT_FAIL | CVT_BADDATE);
	return -1;
        }

    /* 
     * sorry, slow section here - but it's not time critical anyway
     */
    t = (clock -> year - 1970) * 365;
    t += (clock -> year >> 2) - (1970 >> 2);
    t -= clock -> year / 400 - 1970 / 400;

    /* month */
    if (clock -> month <= 0 || clock -> month > 12)
        {
	SETRTC (CVT_FAIL | CVT_BADDATE);
	return -1;		/* bad month */
        }
    /* adjust leap year */
    if (clock -> month >= 3 && dysize (clock -> year) == 366)
	t++;

    for (i = 1; i < clock -> month; i++)
        {
	t += days_of_month[i];
        }
    /* day */
    if (clock -> day < 1 || ((clock -> month == 2 && dysize (clock -> year) == 366) ?
		clock -> day > 29 : clock -> day > days_of_month[clock -> month]))
        {
	SETRTC (CVT_FAIL | CVT_BADDATE);
	return -1;		/* bad day */
        }

    t += clock -> day - 1;
    /* hour */
    if (clock -> hour < 0 || clock -> hour >= 24)
        {
	SETRTC (CVT_FAIL | CVT_BADTIME);
	return -1;		/* bad hour */
        }

    t = TIMES24 (t) + clock -> hour;

    /* min */
    if (clock -> minute < 0 || clock -> minute > 59)
        {
	SETRTC (CVT_FAIL | CVT_BADTIME);
	return -1;		/* bad min */
        }

    t = TIMES60 (t) + clock -> minute;
    /* sec */

    t += clock -> utcoffset;	/* warp to UTC */

    if (clock -> second < 0 || clock -> second > 60)
	/* allow for LEAPs */
        {
	SETRTC (CVT_FAIL | CVT_BADTIME);
	return -1;		/* bad sec */
        }

    t = TIMES60 (t) + clock -> second;
    /* done */
    return t;
}

/*--------------- format conversion -----------------------------------*/

int
Stoi (s, zp, cnt)
char   *s;
LONG   *zp;
int     cnt;
{
    char   *b = s;
    int     f,
            z,
            v;
    char    c;

    f = z = v = 0;

    while (*s == ' ')
	s++;

    if (*s == '-')
        {
	s++;
	v = 1;
        }
    else
    if (*s == '+')
	s++;

    for (;;)
        {
	c = *s++;
	if (c == '\0' || c < '0' || c > '9' || (cnt && ((s - b) > cnt)))
	    {
	    if (f == 0)
	        {
		return (-1);
	        }
	    if (v)
		z = -z;
	    *zp = z;
	    return (0);
	    }
	z = (z << 3) + (z << 1) + (c - '0');
	f = 1;
        }
}


int
Strok (s, m)
char   *s;
char   *m;
{
    if (!s || !m)
	return 0;

    while (*s && *m)
        {
	if ((*m == ' ') ? 1 : (*s == *m))
	    {
	    s++;
	    m++;
	    }
	else
	    {
	    return 0;
	    }
        }
    return !*m;
}

unsigned    LONG
updatetimeinfo (parseio, t, usec, flags)
register parse_t   *parseio;
register time_t     t;
register unsigned   LONG usec;
register unsigned   LONG flags;
{
    register LONG   usecoff;
    register LONG   mean;
    LONG    delta[PARSE_DELTA];

# 	ifdef STREAM
    usecoff = (t - parseio -> parse_dtime.parse_stime.tv_sec) * 1000000
	- parseio -> parse_dtime.parse_stime.tv_usec + usec;
# 	else			/* ! STREAM */
    extern LONG     tstouslo[];
    extern LONG     tstousmid[];
    extern LONG     tstoushi[];

    TSFTOTVU (parseio -> parse_dtime.parse_stime.l_uf, usecoff);
    usecoff = -usecoff;
    usecoff += (t - parseio -> parse_dtime.parse_stime.l_ui + JAN_1970) * 1000000
	+ usec;
# 	endif			/* STREAM */

    /* 
     * filtering (median) if requested
     */
    if (parseio -> parse_flags & PARSE_STAT_FILTER)
        {
	register int    n,
	                i,
	                s,
	                k;

	parseio -> parse_delta[parseio -> parse_dindex] = usecoff;

	parseio -> parse_dindex = (parseio -> parse_dindex + 1) % PARSE_DELTA;

	/* 
	 * sort always - thus every sample gets its data
	 */
	bcopy ((caddr_t)parseio -> parse_delta, (caddr_t)delta, sizeof (delta));

	for (s = 0; s < PARSE_DELTA; s++)
	    for (k = s + 1; k < PARSE_DELTA; k++)
	        {		/* Yes - it's slow sort */
		if (delta[s] > delta[k])
		    {
		    register LONG   tmp;

		    tmp = delta[k];
		    delta[k] = delta[s];
		    delta[s] = tmp;
		    }
	        }

	i = 0;
	n = PARSE_DELTA;

	/* 
	 * you know this median loop if you have read the other code
	 */
	while ((n - i) > 8)
	    {
	    register LONG   top = delta[n - 1];
	    register LONG   mid = delta[(n + i) >> 1];
	    register LONG   low = delta[i];

	    if ((top - mid) > (mid - low))
	        {
		/* 
		 * cut off high end
		 */
		n--;
	        }
	    else
	        {
		/* 
		 * cut off low end
		 */
		i++;
	        }
	    }

	parseio -> parse_dtime.parse_usecdisp = delta[n - 1] - delta[i];

	if (parseio -> parse_flags & PARSE_STAT_AVG)
	    {
	    /* 
	     * take the average of the median samples as this clock
	     * is a little bumpy
	     */
	    mean = 0;

	    while (i < n)
	        {
		mean += delta[i++];
	        }

	    mean >>= 3;
	    }
	else
	    {
	    mean = delta[(n + i) >> 1];
	    }

	parseio -> parse_dtime.parse_usecerror = mean;
        }
    else
        {
	parseio -> parse_dtime.parse_usecerror = usecoff;
	parseio -> parse_dtime.parse_usecdisp = 0;
        }


    parseprintf (DD_PARSE, ("parse: updatetimeinfo: T=%x+%d usec, useccoff=%d, usecerror=%d, usecdisp=%d\n",
		t, usec, usecoff, parseio -> parse_dtime.parse_usecerror, parseio -> parse_dtime.parse_usecdisp));


# 	ifdef STREAM
        {
	int     s = splhigh ();
# 	endif			/* STREAM */

	parseio -> parse_lstate = parseio -> parse_dtime.parse_state | flags | PARSEB_TIMECODE;

	parseio -> parse_dtime.parse_state = parseio -> parse_lstate;

# 	ifdef STREAM
	(void)splx (s);
        }
# 	endif			/* STREAM */

    return CVT_OK;		/* everything fine and
				   dandy... */
}


/*
 * syn_simple
 *
 * handle a sync time stamp
 */
/*ARGSUSED*/
void
syn_simple (parseio, ts, format, why)
register parse_t   *parseio;
register TIMESTAMP *ts;
register struct format *format;
register unsigned   LONG why;
{
    parseio -> parse_dtime.parse_stime = *ts;
}

/*
 * pps_simple
 *
 * handle a pps time stamp
 */
/*ARGSUSED*/
unsigned    LONG
pps_simple (parseio, status, ptime)
register parse_t   *parseio;
register int    status;
register struct timeval    *ptime;
{
    parseio -> parse_dtime.parse_ptime = *ptime;
    parseio -> parse_dtime.parse_state |= PARSEB_PPS | PARSEB_S_PPS;

    return CVT_NONE;
}

/*
 * timepacket
 *
 * process a data packet
 */
static unsigned     LONG
timepacket (parseio)
register parse_t   *parseio;
{
    register int    k;
    register unsigned short     format;
    register time_t     t;
    register unsigned   LONG cvtsum = 0;
    /* accumulated CVT_FAIL errors */
    unsigned    LONG cvtrtc;	/* current conversion
				   result */
    clocktime_t     clock;

    format = parseio -> parse_lformat;

    k = 0;

    if (parseio -> parse_flags & PARSE_FIXED_FMT)
        {
	switch ((cvtrtc = clockformats[format] -> convert ? clockformats[format] -> convert (parseio -> parse_data, parseio -> parse_index, clockformats[format] -> data, &clock) : CVT_NONE) & CVT_MASK)
	    {
	    case CVT_FAIL:
		parseio -> parse_badformat++;
		cvtsum = cvtrtc & ~CVT_MASK;

		/* 
		 * may be too often ... but is nice to know when it happens
		 */
# 	ifdef STREAM
		printf ("parse: \"%s\" failed to convert\n", clockformats[format] -> name);
# 	else			/* ! STREAM */
		syslog (LOG_WARNING, "parse: \"%s\" failed to convert\n", clockformats[format] -> name);
# 	endif			/* STREAM */
		break;

	    case CVT_NONE:
		/* 
		 * too bad - pretend bad format
		 */
		parseio -> parse_badformat++;
		cvtsum = CVT_BADFMT;

		break;

	    case CVT_OK:
		k = 1;
		break;

	    default:
		/* shouldn't happen */
# 	ifdef STREAM
		printf ("parse: INTERNAL error: bad return code of convert routine \"%s\"\n", clockformats[format] -> name);
# 	else			/* ! STREAM */
		syslog (LOG_WARNING, "parse: INTERNAL error: bad return code of convert routine \"%s\"\n", clockformats[format] -> name);
# 	endif			/*   */
		return CVT_FAIL | cvtrtc;
	    }
        }
    else
        {
	/* 
	 * find correct conversion routine
	 * and convert time packet
	 * RR search starting at last successful conversion routine
	 */

	if (nformats)		/* very careful ... */
	    {
	    do
	        {
		switch ((cvtrtc = (clockformats[format] -> convert && !(clockformats[format] -> flags & CVT_FIXEDONLY)) ?
			    clockformats[format] -> convert (parseio -> parse_data, parseio -> parse_index, clockformats[format] -> data, &clock) :
			    CVT_NONE) & CVT_MASK)
		    {
		    case CVT_FAIL:
			parseio -> parse_badformat++;
			cvtsum |= cvtrtc & ~CVT_MASK;

			/* 
			 * may be too often ... but is nice to know when it happens
			 */
# 	ifdef STREAM
			printf ("parse: \"%s\" failed to convert\n", clockformats[format] -> name);
# 	else			/* !   */
			syslog (LOG_WARNING, "parse: \"%s\" failed to convert\n", clockformats[format] -> name);
# 	endif			/* STREAM */
			/* FALLTHROUGH */
		    case CVT_NONE:
			format++;
			break;

		    case CVT_OK:
			k = 1;
			break;

		    default:
			/* shouldn't happen */
# 	ifdef STREAM
			printf ("parse: INTERNAL error: bad return code of convert routine \"%s\"\n", clockformats[format] -> name);
# 	else			/* ! STREAM */
			syslog (LOG_WARNING, "parse: INTERNAL error: bad return code of convert routine \"%s\"\n", clockformats[format] -> name);
# 	endif			/* STREAM */
			return CVT_BADFMT;
		    }
		if (format >= nformats)
		    format = 0;
	        }
	    while (!k && (format != parseio -> parse_lformat));
	    }

        }

    if (!k)
        {
# 	ifdef STREAM
	printf ("parse: time format \"%s\" not convertable\n", parseio -> parse_data);
# 	else			/* ! STREAM */
	syslog (LOG_WARNING, "parse: time format \"%s\" not convertable\n", parseio -> parse_data);
# 	endif			/* STREAM */
	return CVT_FAIL | cvtsum;
        }

    if ((t = parse_to_unixtime (&clock, &cvtrtc)) == -1)
        {
# 	ifdef STREAM
	printf ("parse: bad time format \"%s\"\n", parseio -> parse_data);
# 	else			/* ! STREAM */
	syslog (LOG_WARNING, "parse: bad time format \"%s\"\n", parseio -> parse_data);
# 	endif			/* STREAM */
	return CVT_FAIL | cvtrtc;
        }

    parseio -> parse_lformat = format;

    /* 
     * time stamp
     */
    parseio -> parse_dtime.parse_time.tv_sec = t;
    parseio -> parse_dtime.parse_time.tv_usec = clock.usecond;
    parseio -> parse_dtime.parse_format = format;

    return updatetimeinfo (parseio, t, clock.usecond, clock.flags);
}


/*
 * control operations
 */
/*ARGSUSED*/
int
parse_getstat (dct, parse)
parsectl_t *dct;
parse_t    *parse;
{
    dct -> parsestatus.flags = parse -> parse_flags & PARSE_STAT_FLAGS;
    return 1;
}


/*ARGSUSED*/
int
parse_setstat (dct, parse)
parsectl_t *dct;
parse_t    *parse;
{
    parse -> parse_flags = (parse -> parse_flags & ~PARSE_STAT_FLAGS) | dct -> parsestatus.flags;
    return 1;
}


/*ARGSUSED*/
int
parse_timecode (dct, parse)
parsectl_t *dct;
parse_t    *parse;
{
    dct -> parsegettc.parse_state = parse -> parse_lstate;
    dct -> parsegettc.parse_format = parse -> parse_lformat;
    /* 
     * move out current bad packet count
     * user program is expected to sum these up
     * this is not a problem, as "parse" module are
     * exclusive open only
     */
    dct -> parsegettc.parse_badformat = parse -> parse_badformat;
    parse -> parse_badformat = 0;

    if (parse -> parse_ldsize <= PARSE_TCMAX)
        {
	dct -> parsegettc.parse_count = parse -> parse_ldsize;
	bcopy (parse -> parse_ldata, dct -> parsegettc.parse_buffer, dct -> parsegettc.parse_count);
	return 1;
        }
    else
        {
	return 0;
        }
}


/*ARGSUSED*/
int
parse_setfmt (dct, parse)
parsectl_t *dct;
parse_t    *parse;
{
    if (dct -> parseformat.parse_count <= PARSE_TCMAX)
        {
	if (dct -> parseformat.parse_count)
	    {
	    register int    i;

	    for (i = 0; i < nformats; i++)
	        {
		if (!Strcmp (dct -> parseformat.parse_buffer, clockformats[i] -> name))
		    {
		    parse -> parse_lformat = i;
		    parse -> parse_flags |= PARSE_FIXED_FMT;
		    /* set fixed format indication */
		    return setup_bitmaps (parse, i, i + 1);
		    }
	        }

	    return 0;
	    }
	else
	    {
	    parse -> parse_flags &= ~PARSE_FIXED_FMT;
	    /* clear fixed format indication */
	    return setup_bitmaps (parse, 0, nformats);
	    }
        }
    else
        {
	return 0;
        }
}

/*ARGSUSED*/
int
parse_getfmt (dct, parse)
parsectl_t *dct;
parse_t    *parse;
{
    if (dct -> parseformat.parse_format < nformats &&
	    Strlen (clockformats[dct -> parseformat.parse_format] -> name) <= PARSE_TCMAX)
        {
	dct -> parseformat.parse_count = Strlen (clockformats[dct -> parseformat.parse_format] -> name) + 1;
	bcopy (clockformats[dct -> parseformat.parse_format] -> name, dct -> parseformat.parse_buffer, dct -> parseformat.parse_count);
	return 1;
        }
    else
        {
	return 0;
        }
}

/*ARGSUSED*/
int
parse_setcs (dct, parse)
parsectl_t *dct;
parse_t    *parse;
{
    parse -> parse_ioflags &= ~PARSE_IO_CSIZE;
    parse -> parse_ioflags |= dct -> parsesetcs.parse_cs & PARSE_IO_CSIZE;
    return 1;
}

# endif				/* defined(REFCLOCK) &&
				   defined(PARSE) */
