/*
 * ntpdate.h - declarations for the ntpdate program
 * $Header: ntpdate.h,v 1.2.109.4 94/11/08 16:25:18 mike Exp $
 */

/*
 * The server structure is a much simplified version of the
 * peer structure, for ntpdate's use.  Since we always send
 * in client mode and expect to receive in server mode, this
 * leaves only a very limited number of things we need to
 * remember about the server.
 */
struct server
{
    struct sockaddr_in  srcadr;	/* address of remote host 
				*/
    u_char  leap;		/* leap indicator */
    u_char  stratum;		/* stratum of remote server
				   */
    s_char  precision;		/* server's clock precision
				   */
    u_char  trust;		/* trustability of the
				   filtered data */
    u_fp    rootdelay;		/* distance from primary
				   clock */
    u_fp    rootdispersion;	/* peer clock dispersion */
    U_LONG  refid;		/* peer reference ID */
    l_fp    reftime;		/* time of peer's last
				   update */
    U_LONG  event_time;		/* time for next timeout */
    u_short     xmtcnt;		/* number of packets
				   transmitted */
    u_short     filter_nextpt;	/* index into filter shift
				   register */
    s_fp    filter_delay[NTP_SHIFT];	/* delay part of shift
				   register */
    l_fp    filter_offset[NTP_SHIFT];
				/* offset part of shift
				   register */
    s_fp    filter_soffset[NTP_SHIFT];
				/* offset in s_fp format,
				   for disp */
    u_fp    filter_error[NTP_SHIFT];	/* error part of shift
				   register */
    l_fp    org;		/* peer's originate time
				   stamp */
    l_fp    xmt;		/* transmit time stamp */
    u_fp    delay;		/* filter estimated delay 
				*/
    u_fp    dispersion;		/* filter estimated
				   dispersion */
    l_fp    offset;		/* filter estimated clock
				   offset */
    s_fp    soffset;		/* fp version of above */
};


/*
 * ntpdate runs everything on a simple, short timeout.  It sends a
 * packet and sets the timeout (by default, to a small value suitable
 * for a LAN).  If it receives a response it sends another request.
 * If it times out it shifts zeroes into the filter and sends another
 * request.
 *
 * The timer routine is run often (once every 1/5 second currently)
 * so that time outs are done with reasonable precision.
 */
# define TIMER_HZ	(5)	/* 5 per second */

/*
 * ntpdate will make a LONG adjustment using adjtime() if the times
 * are close, or step the time if the times are farther apart.  The
 * following defines what is "close".
 */
# if defined(SOLARIS)
# 	define	NTPDATE_THRESHOLD	(FP_SECOND*2)
				/* 2 second */
# else	/* ! defined (SOLARIS) */
# 	define	NTPDATE_THRESHOLD	(FP_SECOND >> 1)
				/* 1/2 second */
# endif	/* defined (SOLARIS) */

/*
 * When doing adjustments, ntpdate actually overadjusts (currently
 * by 50%, though this may change).  While this will make it take longer
 * to reach a steady state condition, it will typically result in
 * the clock keeping more accurate time, on average.  The amount of
 * overshoot is limited.
 */
# ifdef	NOTNOW
# 	define	ADJ_OVERSHOOT	1/2	/* this is hard coded */
# endif				/* NOTNOW */
# define	ADJ_MAXOVERSHOOT	0x10000000
				/* 50 ms as a ts fraction 
				*/

/*
 * Since ntpdate isn't aware of some of the things that normally get
 * put in an NTP packet, we fix some values.
 */
# define	NTPDATE_PRECISION	(-6)
				/* use this precision */
# define	NTPDATE_DISTANCE	FP_SECOND
				/* distance is 1 sec */
# define	NTPDATE_DISP		FP_SECOND
				/* so is the dispersion */
# define	NTPDATE_REFID		(0)
				/* reference ID to use */


/*
 * Some defaults
 */
# define	DEFTIMEOUT	5	/* 5 timer increments */
# define	DEFSAMPLES	4	/* get 4 samples per server
				   */
# define	DEFPRECISION	(-5)	/* the precision we claim 
				*/
