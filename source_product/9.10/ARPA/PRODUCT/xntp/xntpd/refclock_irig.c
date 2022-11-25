/*
 * $Header: refclock_irig.c,v 1.2.109.3 94/11/09 10:55:14 mike Exp $
 * refclock_irig - clock driver for the IRIG audio decoder
 */
# if defined(REFCLOCK) && defined(IRIG)

# 	include <stdio.h>
# 	include <ctype.h>
# 	include <sys/types.h>
# 	include <sys/time.h>
# 	include <sys/file.h>
# 	include <sys/fcntl.h>
# 	include <sys/ioctl.h>
# 	include <sys/stropts.h>

# 	include "ntpd.h"
# 	include "ntp_refclock.h"
# 	include "ntp_unixtime.h"
# 	include "bsd_audioio.h"


/*
 * This driver supports the IRIG audio decoder. This clever gadget uses
 * a modified BSD audio driver for the Sun SPARCstation which provides
 * a timestamp and timecode on every read. The format is:
 *
 * ttttttttbbbbbbbbbbddd hh:mm:ss
 *
 * where tttttttt represents a timestamp at the zero crossing of the
 * index marker at the second's epoch and bbbbbbbbbb is a 10-octet
 * binary coded string representing code elements 1 through 78 in the
 * IRIG-B code format, left justified and zero padded. In the decoded
 * time, ddd is the day of year and hh:mm:ss is the time of day. The
 * timestamp is in unix timeval format, consisting of two 32-bit long
 * words, the first of which is the seconds since 1970 and the second is
 * the fraction of the second in microseconds.
 *
 * The control function elements begin at element 50, which is control-
 * field element 1, and extend to element 78, which is control-field
 * element 27. The control functions have different interpretations in
 * various devices.
 *
 * Spectracom Netclock/2 WWVB Synchronized Clock
 * 6		time sync status
 * 10-13	bcd year units
 * 15-18	bcd year tens
 * 
 * Austron 2201A GPS Receiver (speculative)
 */

/*
 * Definitions
 */
# 	define	MAXUNITS	1	/* max number of irig units
				   */
# 	define	IRIGFD		"/dev/irig"
				/* name of driver device */

/*
 * IRIG interface parameters.
 */
# 	define	IRIGPRECISION	(-20)	/* precision assumed (1 us)
				   */
# 	define	IRIGREFID	"IRIG"	/* reference id */
# 	define	IRIGDESCRIPTION	"IRIG audio decoder"
				/* who we are */
# 	define	IRIGHSREFID	0x7f7f0c0a  /* 127.127.6.10 refid hi
				   strata */
# 	define GMT		0	/* hour offset from
				   Greenwich */
# 	define IRIG_FORMAT	1	/* IRIG timestamp format */
# 	define FMTIRIG		10	/* raw binary code length
				   (octets) */
# 	define FMTLEN		12	/* decoded timecode length 
				*/
# 	define BMAX		40	/* timecode buffer length 
				*/


/*
 * Hack to avoid excercising the multiplier.  I have no pride.
 */
# 	define	MULBY10(x)	(((x)<<3) + ((x)<<1))

/*
 * Imported from ntp_timer module
 */
extern U_LONG   current_time;	/* current time (s) */

/*
 * Imported from ntpd module
 */
extern int  debug;		/* global debug flag */

/*
 * irig unit control structure.
 */
struct irigunit
{
    struct peer    *peer;	/* associated peer
				   structure */
    struct refclockio   io;	/* given to the I/O handler
				   */
    l_fp    lastrec;		/* last local time */
    l_fp    lastref;		/* last timecode time */
    char    lastcode[BMAX];	/* decoded timecode */
    u_char  bincode[FMTIRIG];	/* raw binary code */
    int     lencode;		/* lengthof last timecode 
				*/
    U_LONG  lasttime;		/* last time clock heard
				   from */
    u_char  unit;		/* unit number for this guy
				   */
    u_char  status;		/* clock status */
    u_char  lastevent;		/* last clock event */
    u_char  year;		/* year of eternity */
    u_short     day;		/* day of year */
    u_char  hour;		/* hour of day */
    u_char  minute;		/* minute of hour */
    u_char  second;		/* seconds of minute */
    U_LONG  yearstart;		/* start of current year */
    u_char  leap;		/* leap indicators */
    /* 
     * Status tallies
     */
    U_LONG  polls;		/* polls sent */
    U_LONG  coderecv;		/* timecodes received */
    U_LONG  badformat;		/* bad format */
    U_LONG  baddata;		/* bad data */
    U_LONG  timestarted;	/* time we started this */
};

/*
 * Data space for the unit structures.  Note that we allocate these on
 * the fly, but never give them back.
 */
static struct irigunit *irigunits[MAXUNITS];
static u_char   unitinuse[MAXUNITS];

/*
 * Keep the fudge factors separately so they can be set even
 * when no clock is configured.
 */
static l_fp     fudgefactor[MAXUNITS];
static u_char   stratumtouse[MAXUNITS];
static u_char   sloppyclockflag[MAXUNITS];

/*
 * Function prototypes
 */
static void     irig_init P (());
static int  irig_start P ((u_int, struct peer  *));
static void     irig_shutdown P ((int));
static void     irig_report_event P ((struct irigunit  *, int));
static void     irig_poll P ((int  , struct peer   *));
static void     irig_control P ((u_int, struct refclockstat    *, struct refclockstat  *));
static void     irig_buginfo P ((int   , struct refclockbug    *));

/*
 * Transfer vector
 */
struct refclock     refclock_irig =
{
    irig_start, irig_shutdown, irig_poll,
	irig_control, irig_init, irig_buginfo, NOFLAGS
};

/*
 * irig_init - initialize internal irig driver data
 */
static void
irig_init ()
{
    register int    i;
    /* 
     * Just zero the data arrays
     */
    bzero ((char   *)irigunits, sizeof irigunits);
    bzero ((char   *)unitinuse, sizeof unitinuse);

    /* 
     * Initialize fudge factors to default.
     */
    for (i = 0; i < MAXUNITS; i++)
        {
	fudgefactor[i].l_ui = 0;
	fudgefactor[i].l_uf = 0;
	stratumtouse[i] = 0;
	sloppyclockflag[i] = 0;
        }
}


/*
 * irig_start - open the irig device and initialize data for processing
 */
static int
irig_start (unit, peer)
u_int   unit;
struct peer    *peer;
{
    register struct irigunit   *irig;
    register int    i;
    int     fd_irig;
    int     format = IRIG_FORMAT;

    /* 
     * Check configuration info.
     */
    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "irig_start: unit %d invalid", unit);
	return (0);
        }
    if (unitinuse[unit])
        {
	syslog (LOG_ERR, "irig_start: unit %d in use", unit);
	return (0);
        }

    /* 
     * Open IRIG device and set format
     */
    fd_irig = open (IRIGFD, O_RDONLY | O_NDELAY, 0777);
    if (fd_irig == -1)
        {
	syslog (LOG_ERR, "irig_start: open of %s: %m", IRIGFD);
	return (0);
        }
    if (ioctl (fd_irig, AUDIO_IRIG_OPEN, 0) < 0)
        {
	syslog (LOG_ERR,
		"irig_start: ioctl(%s, AUDIO_IRIG_OPEN): %m", IRIGFD);
	close (fd_irig);
	return (0);
        }
    if (ioctl (fd_irig, AUDIO_IRIG_SETFORMAT, &format) < 0)
        {
	syslog (LOG_ERR,
		"irig_start: ioctl(%s, AUDIO_IRIG_SETFORMAT): %m", IRIGFD);
	close (fd_irig);
	return (0);
        }
    if (ioctl (fd_irig, AUDIO_FLUSH, 0) < 0)
        {
	syslog (LOG_ERR,
		"irig_start: ioctl(%s, AUDIO_FLUSH): %m", IRIGFD);
	close (fd_irig);
	return (0);
        }

    /* 
     * Allocate unit structure
     */
    if (irigunits[unit] != 0)
        {
	irig = irigunits[unit];	/* The one we want is okay 
				*/
        }
    else
        {
	for (i = 0; i < MAXUNITS; i++)
	    {
	    if (!unitinuse[i] && irigunits[i] != 0)
		break;
	    }
	if (i < MAXUNITS)
	    {
	    /* 
	     * Reclaim this one
	     */
	    irig = irigunits[i];
	    irigunits[i] = 0;
	    }
	else
	    {
	    irig = (struct irigunit    *)
		emalloc (sizeof (struct irigunit));
	    }
        }
    bzero ((char   *)irig, sizeof (struct irigunit));
    irigunits[unit] = irig;

    /* 
     * Set up the structures
     */
    irig -> peer = peer;
    irig -> unit = (u_char)unit;
    irig -> timestarted = current_time;

    irig -> io.clock_recv = noentry;
    irig -> io.srcclock = (caddr_t)irig;
    irig -> io.datalen = 0;
    irig -> io.fd = fd_irig;
    if (!io_addclock_simple (&irig -> io))
        {
	(void)close (fd_irig);
	return (0);
        }

    /* 
     * All done.  Initialize a few random peer variables, then
     * return success. Note that root delay and root dispersion are
     * always zero for this clock.
     */
    peer -> precision = IRIGPRECISION;
    peer -> rootdelay = 0;
    peer -> rootdispersion = 0;
    peer -> stratum = stratumtouse[unit];
    if (stratumtouse[unit] <= 1)
	bcopy (IRIGREFID, (char    *)&peer -> refid, 4);
    else
	peer -> refid = htonl (IRIGHSREFID);
    unitinuse[unit] = 1;
    return (1);
}

/*
 * irig_shutdown - shut down a irig clock
 */
static void
irig_shutdown (unit)
int     unit;
{
    register struct irigunit   *irig;

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "irig_shutdown: unit %d invalid", unit);
	return;
        }
    if (!unitinuse[unit])
        {
	syslog (LOG_ERR, "irig_shutdown: unit %d not in use", unit);
	return;
        }

    /* 
     * Tell the I/O module to turn us off.  We're history.
     */
    irig = irigunits[unit];
    io_closeclock (&irig -> io);
    unitinuse[unit] = 0;
}


/*
 * irig_report_event - note the occurance of an event
 *
 * This routine presently just remembers the report and logs it, but
 * does nothing heroic for the trap handler.
 */
static void
irig_report_event (irig, code)
struct irigunit    *irig;
int     code;
{
    struct peer    *peer;

    peer = irig -> peer;
    if (irig -> status != (u_char)code)
        {
	irig -> status = (u_char)code;
	if (code != CEVNT_NOMINAL)
	    irig -> lastevent = (u_char)code;
	syslog (LOG_INFO,
		"clock %s event %x", ntoa (&peer -> srcadr), code);
        }
}

/*
 * irig_poll - called by the transmit procedure
 */
static void
irig_poll (unit, peer)
int     unit;
struct peer    *peer;
{

    struct irigunit    *irig;
    register u_char    *dpt;
    register char  *cp,
                   *dp;
    int     dpend;
    l_fp    tstmp;
    int     i;
    u_char  buf[40];

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "irig_poll: unit %d invalid", unit);
	return;
        }
    if (!unitinuse[unit])
        {
	syslog (LOG_ERR, "irig_poll: unit %d not in use", unit);
	return;
        }
    irig = irigunits[unit];
    dpend = read (irig -> io.fd, buf, BMAX);
    if (dpend < 0)
        {
	syslog (LOG_ERR, "irig_poll: unit %d: %m", irig -> unit);
	irig_report_event (irig, CEVNT_FAULT);
	return;
        }
    if (dpend < FMTIRIG + 8 || !buftvtots (buf, &irig -> lastrec))
        {
# 	ifdef DEBUG
	if (debug)
	    printf ("irig_poll: invalid length or timestamp");
# 	endif	/* DEBUG */
	return;
        }
    dpt = buf;
    dp = irig -> bincode;
    for (i = 0; i < FMTIRIG; i++)
	*dp++ = *dpt++;
    dpend -= FMTIRIG + 8;
    cp = dp = irig -> lastcode;
    for (i = 0; i < dpend; i++)
	if ((*dp = 0x7f & *dpt++) >= ' ')
	    dp++;
    *dp = '\0';
    irig -> lencode = dp - cp;

# 	if DEBUG
    if (debug)
	printf ("irig: timecode %d %s\n",
		irig -> lencode, irig -> lastcode);
# 	endif	/* DEBUG */
    irig -> lasttime = current_time;

    /* 
     * Get irig time and convert to timestamp format.
     */
    if (irig -> lencode < 12 || !isdigit (cp[0]) || !isdigit (cp[1]) ||
	    !isdigit (cp[2]) ||
	    cp[3] != ' ' || !isdigit (cp[4]) || !isdigit (cp[5]) ||
	    cp[6] != ':' || !isdigit (cp[7]) || !isdigit (cp[8]) ||
	    cp[9] != ':' || !isdigit (cp[10]) || !isdigit (cp[11]))
        {
	irig -> badformat++;
	irig_report_event (irig, CEVNT_BADREPLY);
	return;
        }
    record_clock_stats (&(irig -> peer -> srcadr), irig -> lastcode);
    irig -> day = cp[0] - '0';
    irig -> day = MULBY10 (irig -> day) + cp[1] - '0';
    irig -> day = MULBY10 (irig -> day) + cp[2] - '0';
    irig -> hour = MULBY10 (cp[4] - '0') + cp[5] - '0';
    irig -> minute = MULBY10 (cp[7] - '0') + cp[8] - '0';
    irig -> second = MULBY10 (cp[10] - '0') + cp[11] - '0';

    /* 
     * Now, compute the reference time value. Use the heavy
     * machinery for the seconds and the millisecond field for the
     * fraction when present. If an error in conversion to internal
     * format is found, the program declares bad data and exits.
     * Note that this code does not yet know how to do the years and
     * relies on the clock-calendar chip for sanity.
     */
    if (!clocktime (irig -> day, irig -> hour, irig -> minute,
		irig -> second, GMT, irig -> lastrec.l_ui,
		&irig -> yearstart, &irig -> lastref.l_ui))
        {
	irig -> baddata++;
	irig_report_event (irig, CEVNT_BADTIME);
	return;
        }
    tstmp = irig -> lastref;
    L_SUB (&tstmp, &irig -> lastrec);
    irig -> coderecv++;
    L_ADD (&tstmp, &(fudgefactor[irig -> unit]));
    refclock_receive (irig -> peer, &tstmp, GMT, 0,
	    &irig -> lastrec, &irig -> lastrec, irig -> leap);
}

/*
 * irig_control - set fudge factors, return statistics
 */
static void
irig_control (unit, in, out)
u_int   unit;
struct refclockstat    *in;
struct refclockstat    *out;
{
    register struct irigunit   *irig;

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "irig_control: unit %d invalid", unit);
	return;
        }

    if (in != 0)
        {
	if (in -> haveflags & CLK_HAVETIME1)
	    fudgefactor[unit] = in -> fudgetime1;
	if (in -> haveflags & CLK_HAVEVAL1)
	    {
	    stratumtouse[unit] = (u_char)(in -> fudgeval1 & 0xf);
	    if (unitinuse[unit])
	        {
		struct peer    *peer;

		/* 
		 * Should actually reselect clock, but
		 * will wait for the next timecode
		 */
		irig = irigunits[unit];
		peer = irig -> peer;
		peer -> stratum = stratumtouse[unit];
		if (stratumtouse[unit] <= 1)
		    bcopy (IRIGREFID, (char    *)&peer -> refid,
			    4);
		else
		    peer -> refid = htonl (IRIGHSREFID);
	        }
	    }
	if (in -> haveflags & CLK_HAVEFLAG1)
	    {
	    sloppyclockflag[unit] = in -> flags & CLK_FLAG1;
	    }
        }

    if (out != 0)
        {
	out -> type = REFCLK_IRIG_AUDIO;
	out -> haveflags
	    = CLK_HAVETIME1 | CLK_HAVEVAL1 | CLK_HAVEVAL2 | CLK_HAVEFLAG1;
	out -> clockdesc = IRIGDESCRIPTION;
	out -> fudgetime1 = fudgefactor[unit];
	out -> fudgetime2.l_ui = 0;
	out -> fudgetime2.l_uf = 0;
	out -> fudgeval1 = (LONG)stratumtouse[unit];
	out -> fudgeval2 = 0;
	out -> flags = sloppyclockflag[unit];
	if (unitinuse[unit])
	    {
	    irig = irigunits[unit];
	    out -> timereset = current_time - irig -> timestarted;
	    out -> polls = irig -> polls;
	    out -> baddata = irig -> baddata;
	    out -> lastevent = irig -> lastevent;
	    out -> currentstatus = irig -> status;
	    }
	else
	    {
	    out -> polls = out -> noresponse = 0;
	    out -> timereset = 0;
	    out -> currentstatus = out -> lastevent = CEVNT_NOMINAL;
	    }
        }
}

/*
 * irig_buginfo - return clock dependent debugging info
 */
static void
irig_buginfo (unit, bug)
int     unit;
register struct refclockbug    *bug;
{
    register struct irigunit   *irig;

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "irig_buginfo: unit %d invalid", unit);
	return;
        }

    if (!unitinuse[unit])
	return;
    irig = irigunits[unit];

    bug -> nvalues = 8;
    bug -> ntimes = 2;
    if (irig -> lasttime != 0)
	bug -> values[0] = current_time - irig -> lasttime;
    else
	bug -> values[0] = 0;
    bug -> values[2] = (U_LONG)irig -> year;
    bug -> values[3] = (U_LONG)irig -> day;
    bug -> values[4] = (U_LONG)irig -> hour;
    bug -> values[5] = (U_LONG)irig -> minute;
    bug -> values[6] = (U_LONG)irig -> second;
    bug -> values[7] = irig -> yearstart;
    bug -> stimes = 0x1c;
    bug -> times[0] = irig -> lastref;
    bug -> times[1] = irig -> lastrec;
}
# endif	/* defined (REFCLOCK) && defined(IRIG) */
