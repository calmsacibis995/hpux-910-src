/* $Header: ntp_loopfilter.c,v 1.2.109.3 94/11/09 10:38:33 mike Exp $
 * ntp_loopfilter.c - implements the NTP loop filter algorithm
 */

# include <stdio.h>
# include <ctype.h>
# include <sys/types.h>
# include <sys/time.h>
# include <fcntl.h>

# include "ntpd.h"
# include "timex.h"
# include "ntp_unixtime.h"

# if defined(PPS) || defined(PPSDEV) || defined(PPSCD)
# 	include <sys/file.h>
# 	include <sys/stat.h>
# 	include "ntp_refclock.h"
# endif				/* PPS || PPSDEV || PPSCD 
				*/

# if defined(PPSDEV) || defined(PPSCD)
# 	if defined(BSD_TTYS)
# 		include <sgtty.h>
# 	endif				/* BSD_TTYS */
# 	if defined(SYSV_TTYS)
# 		include <termio.h>
# 	endif				/* SYSV_TTYS */
# 	if defined(STREAM)
# 		include <termios.h>
# 		include <stropts.h>
# 		if defined(PPSDEV)
# 			include <sys/clkdefs.h>
# 		endif				/* PPSDEV */
# 	endif				/* STREAM */
# endif				/* PPSDEV || PPSCD */

# if defined(PPSCD)
# 	include <sys/ppsclock.h>
# endif				/* PPSCD */

/*
 * The loop filter is implemented in slavish adherence to the
 * specification (Section 5), except that for consistency we
 * mostly carry the quantities in the same units as appendix G.
 *
 * Note that the long values below are the fractional portion of
 * a long fixed-point value.  This limits these values to +-0.5
 * seconds.  When adjustments are capped inside this range (see
 * CLOCK_MAX_{I,F}) both the clock_adjust and the compliance
 * registers should be fine. (When the compliance is above 16, it
 * will at most accumulate 2**CLOCK_MULT times the maximum offset,
 * which means it fits in a s_fp.)
 *
 * The skew compensation is a special case. In version 2, it was
 * kept in ms/4s (i.e., CLOCK_FREQ was 10). In version 3 (Section 5)
 * it seems to be 2**-16ms/4s in a s_fp for a maximum of +-125ppm
 * (stated maximum 100ppm). Since this seems about to change to a
 * larger range, it will be kept in units of 2**-20 (CLOCK_DSCALE)
 * in an s_fp (mainly because that's nearly the same as parts per
 * million). Note that this is ``seconds per second'', whereas a
 * clock adjustment is a 32-bit fraction of a second to be applied
 * every 2**CLOCK_ADJ seconds; to find it, shift the drift right by
 * (CLOCK_DSCALE-16-CLOCK_ADJ). When updating the drift, on the other
 * hand, the CLOCK_FREQ factor from the spec assumes the value to be
 * in ``seconds per 4 seconds''; to get our units, CLOCK_ADJ must be
 * added to the shift.
 */

/*
 * Macro to compute log2(). We don't want to to this very often, but
 * needs what must.
 */
# define LOG2(r, t)		\
do\
{\
    LONG    x = t;\
    r = 0;\
    while (x >> 1)\
	r++;\
}

# define RSH_DRIFT_TO_FRAC (CLOCK_DSCALE - 16)
# define RSH_DRIFT_TO_ADJ (RSH_DRIFT_TO_FRAC - CLOCK_ADJ)
# define RSH_FRAC_TO_FREQ (CLOCK_FREQ + CLOCK_ADJ - RSH_DRIFT_TO_FRAC)

/*
 * Imported from the ntp_proto module
 */
extern u_char   sys_stratum;
extern s_fp     sys_rootdelay;
extern u_fp     sys_rootdispersion;
extern s_char   sys_precision;

l_fp    last_offset;		/* last adjustment done */
static LONG     clock_adjust;	/* clock adjust register
				   (fraction only) */

s_fp    drift_comp;		/* drift compensation
				   register */
static s_fp     max_comp;	/* drift limit imposed by
				   max host clock slew */

int     time_constant;		/* log2 of time constant (0
				   .. 4) */
static U_LONG   tcadj_time;	/* last time-constant
				   adjust time */

U_LONG  watchdog_timer;		/* watchdog timer, in
				   seconds */
static int  first_adjustment;	/* set to 1 if waiting for
				   first adjustment */
static int  tc_counter;		/* time-constant hold
				   counter */

int     pll_control;		/* set nonzero if pll
				   implemented in kernel */
int     pps_control;		/* set nonzero if pps
				   signal valid */
static l_fp     pps_delay;	/* pps tuning offset */
U_LONG  pps_update;		/* last pps update time */
int     fdpps = 0;		/* pps file descriptor */

# if defined(PPS) || defined(PPSDEV) || defined(PPSCD)
/*
 * This module has support for a 1-pps signal to fine-tune the local
 * clock. The signal is optional; when present and operating within
 * given tolerances in frequency and jitter, it is used to discipline
 * the local clock. In order for this to work, the local clock must be
 * set to within +-500 ms by another means, such as a radio clock or
 * NTP itself. The 1-pps signal is connected via a serial port and
 * gadget box consisting of a one-shot and EIA level-converter. When
 * operated at 38.4 kbps with a SPARCstation IPC, this arrangement has a
 * worst-case jitter less than 26 us. The pps delay configuration
 * declaration can be used to compensate for miscellaneous uart and
 * os delays. Allow about 247 us for uart delays at 38400 bps and
 * -1 ms for SunOS streams nonsense.
 */

# 	ifndef B19200
# 		define B19200	EXTA
# 	endif	/* not B19200 */
# 	ifndef B38400
# 		define B38400	EXTB
# 	endif	/* not B38400 */

/*
 * A really messy way to map an integer baud rate to the system baud rate.
 * There should be a better way.
 */
# 	define SYS_BAUD(i) \
(i == 38400 ? B38400 :\
    (i == 19200 ? B19200 :\
	(i == 9600 ? B9600 :\
	    (i == 4800 ? B4800 :\
		(i == 2400 ? B2400 :\
		    (i == 1200 ? B1200 :\
			(i == 600 ? B600 :\
			    (i == 300 ? B300 : 0))))))))

# 	define PPS_BAUD B38400		/* default serial port
				   speed */
# 	define PPS_MAXAGE 120		/* seconds after which we
				   disbelieve pps */
# 	define PPS_MAXUPDATE 600	/* seconds after which we
				   disbelieve timecode */
# 	define	PPS_DEV	"/dev/pps"	/* pps port */
# 	define PPS_FAC 3		/* pps shift (log2 trimmed
				   samples) */
# 	define	NPPS 12			/* pps filter size
				   (1<<PPS_FAC+2*PPS_TRIM) 
				*/
# 	define PPS_TRIM 2		/* samples trimmed from
				   median filter */
# 	define PPS_XCPT "\377"		/* intercept character */

# 	if defined(PPSDEV)
static struct refclockio    io;	/* given to the I/O handler
				   */
static int  pps_baud;		/* baud rate of PPS line */
# 	endif				/* PPSDEV */
static l_fp     pps_offset;	/* filtered pps offset */
static U_LONG   pps_time;	/* last pps sample time */
static U_LONG   nsamples;	/* number of pps samples
				   collected */
static LONG     samples[NPPS];	/* median filter for pps
				   samples */
/*
 * Imported from ntp_io.c
 */
extern struct interface    *loopback_interface;

# endif				/* PPS || PPSDEV || PPSCD 
				*/

/*
 * Imported from ntpd module
 */
extern int  debug;		/* global debug flag */

/*
 * Imported from timer module
 */
extern U_LONG   current_time;	/* like it says, in seconds
				   */

/*
 * sys_poll and sys_refskew are set here
 */
extern u_char   sys_poll;	/* log2 of system poll
				   interval */
extern l_fp     sys_refskew;	/* accumulated skew since
				   last update */
extern u_fp     sys_maxd;	/* max dispersion of
				   survivor list */

/*
 * Function prototypes
 */
# if defined(PPSDEV)
static void     pps_receive P ((struct recvbuf *));
# endif				/* PPSDEV */

/*
 * init_loopfilter - initialize loop filter data
 */
void
init_loopfilter ()
{
    extern U_LONG   tsf_maxslew;
    U_LONG  tsf_limit;
# if defined(PPSDEV)
    int     fd232;
# endif				/* PPSDEV */
    clock_adjust = 0;
    drift_comp = 0;
    time_constant = 0;
    tcadj_time = 0;
    watchdog_timer = 0;
    tc_counter = 0;
    last_offset.l_i = 0;
    last_offset.l_f = 0;
    first_adjustment = 1;

/*
 * Limit for drift_comp, minimum of two values. The first is to avoid
 * signed overflow, the second to keep within 75% of the maximum
 * adjustment possible in adj_systime().
 */
    max_comp = 0x7fff0000;
    tsf_limit = ((tsf_maxslew >> 1) + (tsf_maxslew >> 2));
    if ((max_comp >> RSH_DRIFT_TO_ADJ) > tsf_limit)
	max_comp = tsf_limit << RSH_DRIFT_TO_ADJ;

    pps_control = 0;
# if defined(PPS) || defined(PPSDEV) || defined(PPSCD)
# 	if defined(PPSDEV)
    pps_baud = PPS_BAUD;
# 	endif				/* PPSDEV */
    pps_delay.l_i = 0;
    pps_delay.l_f = 0;
    pps_time = pps_update = 0;
    nsamples = 0;
# 	if defined(PPSDEV)

    /* 
     * Open pps serial port. We don't care if the serial port comes
     * up; if not, we  just use the timecode. Therefore, if anything
     * goes wrong, just reclaim the resources and continue.
     */
    fd232 = open (PPS_DEV, O_RDONLY);
    if (fd232 == -1)
        {
	syslog (LOG_ERR, "loopfilter: open of %s: %m", PPS_DEV);
	return;
        }

# 		if !defined(HPUXGADGET)	/* dedicated to Ken Stone 
				*/
# 			if defined(SYSV_TTYS)
    /* 
     * System V serial line parameters (termio interface)
     *
     * The PPSDEV support uses a 1-pps signal and level converter
     * (gadget box). It requires the tty_clk line discipline and
     * SysV v7 (?) or later.
     */
        {
	struct termio   ttyb;
	if (ioctl (fd232, TCGETA, (char    *)&ttyb) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TCGETA): %m", PPS_DEV);
	    goto screwed;
	    }
    ttyb.c_iflag = IGNBRK | IGNPAR | ICRNL;
	ttyb.c_oflag = 0;
	ttyb.c_cflag = PPS_BAUD | CS8 | CLOCAL | CREAD;
	ttyb.c_lflag = ICANON;
	ttyb.c_line = CLKLDISC;
	ttyb.c_cc[VERASE] = ttyb.c_cc[VKILL] = '\0';
	if (ioctl (fd232, TCSETA, (char    *)&ttyb) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TCSETA): %m", PPS_DEV);
	    goto screwed;
	    }
    }
# 			endif				/* SYSV_TTYS */
# 			if defined(STREAM)
    /* 
     * System V STREAM serial line parameters (termios interface)
     *
     * The PPSDEV support uses a 1-pps signal and level converter
     * (gadget box). It requires the tt_clk streams module and
     * SunOS 4.1.1 or later.
     */
        {
	struct termios  ttyb;
	if (ioctl (fd232, TCGETS, (char    *)&ttyb) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TCGETS): %m", PPS_DEV);
	    goto screwed;
	    }
    ttyb.c_iflag = IGNBRK | IGNPAR | ICRNL;
	ttyb.c_oflag = 0;
	ttyb.c_cflag = PPS_BAUD | CS8 | CLOCAL | CREAD;
	ttyb.c_lflag = ICANON;
	ttyb.c_cc[VERASE] = ttyb.c_cc[VKILL] = '\0';
	if (ioctl (fd232, TCSETS, (char    *)&ttyb) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TCSETS): %m", PPS_DEV);
	    goto screwed;
	    }
    while (ioctl (fd232, I_POP, 0) >= 0);
	if (ioctl (fd232, I_PUSH, "clk") < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, I_PUSH, clk): %m", PPS_DEV);
	    goto screwed;
	    }
	if (ioctl (fd232, CLK_SETSTR, PPS_XCPT) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, CLK_SETSTR, PPS_XCPT): %m", PPS_DEV);
	    goto screwed;
	    }
        }
# 			endif				/* STREAM */
# 			if defined(BSD_TTYS)
    /* 
     * 4.3bsd serial line parameters (sgttyb interface)
     *
     * The PPSDEV support uses a 1-pps signal and level converter
     * (gadget box). It requires the tty_clk line discipline and
     * 4.3bsd or later.
     */
        {
	struct sgttyb   ttyb;

	if (ioctl (fd232, TIOCGETP, (char  *)&ttyb) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TIOCGETP): %m", PPS_DEV);
	    goto screwed;
	    }
    ttyb.sg_ispeed = ttyb.sg_ospeed = PPS_BAUD;
	ttyb.sg_erase = ttyb.sg_kill = '\r';
	ttyb.sg_flags = EVENP | ODDP | RAW | CRMOD;
	if (ioctl (fd232, TIOCSETP, (char  *)&ttyb) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TIOCSETP): %m", PPS_DEV);
	    goto screwed;
	    }
	if (ioctl (fd232, TIOCSETD, CLKLDISC) < 0)
	    {
	    syslog (LOG_ERR,
		    "loopfilter: ioctl(%s, TIOCSETD): %m", PPS_DEV);
	    goto screwed;
	    }
        }
# 			endif				/* BSD_TTYS */
    fdpps = fd232;
# 		endif				/* HPUXGADGET */

    /* 
     * Insert in device list.
     */
    io.clock_recv = pps_receive;
    io.srcclock = (caddr_t)NULL;
    io.datalen = 0;
    io.fd = fdpps;
# 		if defined(HPUXGADGET)
    if (!io_addclock_simple (&io))
# 		else	/* ! HPUXGADGET) */
	if (!io_addclock (&io))
# 		endif				/* HPUXGADGET */
	    goto screwed;
    return;

    /* 
     * Something broke. Reclaim resources.
     */
screwed:
    (void)close (fdpps);
    return;
# 	endif				/* PPSDEV */
# endif				/* PPS || PPSDEV || PPSCD 
				*/
}

/*
 * local_clock - the NTP logical clock loop filter.  Returns 1 if the
 *		 clock was stepped, 0 if it was slewed and -1 if it is
 *		 hopeless.
 */
int
local_clock (fp_offset, peer)
l_fp   *fp_offset;		/* best offset estimate */
struct peer    *peer;		/* from peer - for messages
				   */
{
    register LONG   offset;
    register U_LONG     tmp_ui;
    register U_LONG     tmp_uf;
    register LONG   tmp;
    int     isneg;
    struct timeval  atv,
                    otv;

# ifdef DEBUG
    if (debug > 1)
	printf ("local_clock(%s, %s)\n", lfptoa (fp_offset, 9),
		ntoa (&peer -> srcadr));
# endif	/* DEBUG */

    /* 
     * Take the absolute value of the offset
     */
    tmp_ui = fp_offset -> l_ui;
    tmp_uf = fp_offset -> l_uf;
    if (M_ISNEG (tmp_ui, tmp_uf))
        {
	M_NEG (tmp_ui, tmp_uf);
	isneg = 1;
        }
    else
	isneg = 0;

    /* 
     * If the clock is way off, don't tempt fate by correcting it.
     */
    if (tmp_ui >= CLOCK_WAYTOOBIG)
        {
	syslog (LOG_ERR,
		"Clock appears to be %u seconds %s, something may be wrong",
		tmp_ui, isneg > 0 ? "fast" : "slow");
# ifndef BIGTIMESTEP
	return (-1);
# endif /* BIGTIMESTEP */
        }

    /* 
     * Save this offset for later perusal
     */
    last_offset = *fp_offset;

    /* 
     * If the magnitude of the offset is greater than CLOCK.MAX, step
     * the time and reset the registers.
     */
    if (tmp_ui > CLOCK_MAX_I || (tmp_ui == CLOCK_MAX_I
		&& (U_LONG)tmp_uf >= (U_LONG)CLOCK_MAX_F))
        {
	if (watchdog_timer < CLOCK_MINSTEP)
	    {
	    /* Mustn't step yet, pretend we adjusted. */
	    syslog (LOG_INFO,
		    "adjust: STEP dropped (%s offset %s)\n",
		    ntoa (&peer -> srcadr), lfptoa (fp_offset, 9));
	    return (0);
	    }
	syslog (LOG_INFO, "adjust: STEP %s offset %s\n",
		ntoa (&peer -> srcadr), lfptoa (fp_offset, 9));
	step_systime (fp_offset);

	clock_adjust = 0;
	watchdog_timer = 0;
	first_adjustment = 1;
	pps_update = 0;
	return (1);
        }

    /* 
     * Here we've got an offset small enough to slew.  Note that
     * since the offset is small we don't have to carry the damned
     * high order longword in our calculations.
     *
     * The time constant and sample interval are approximated with
     * shifts, as in Section 5 of the v3 spec. The spec procedure
     * looks strange, as an interval of 64 to 127 seconds seems to
     * cause multiplication with 128 (and so on). This code lowers
     * the multiplier by one bit.
     *
     * The time constant update goes after adjust and skew updates,
     * as in appendix G.
     */
# if defined(PPS) || defined(PPSDEV) || defined(PPSCD)
    /* 
     * If pps samples are valid, update offset, root delay and
     * root dispersion. This may be a dramatic surprise to high-
     * stratum clients, since all of a sudden this server looks
     * like a stratum-1 clock.
     */
    if (pps_control)
        {
	last_offset = pps_offset;
	sys_stratum = 1;
	sys_rootdelay = 0;
	sys_rootdispersion = sys_maxd;
        }
# endif				/* PPS || PPSDEV || PPSCD 
				*/

    /* 
     * The pll_control is active when the phase-lock code is
     * implemented in the kernel, which at present is only in the
     * (modified) Ultrix 4.2a and SunOS 4.1.1 kernels. In the case
     * of the DECstation 5000/240, additional kernal modifications
     * provide a true microsecond clock. Note the nonstandard
     * version of adjtime(), in which the second argument is used as
     * a sneak path to communicate the kernel variables for time
     * constant, frequency and precision. We know the scaling of our
     * frequency variable is << 16, while the scaling of the kernel
     * frequency variable is << 24.
     *
     * In the case of stock kernels the phase-lock loop is
     * implemented the hard way and the clock_adjust and drift_comp
     * computed as required.
     */
    offset = last_offset.l_f;
    if (pll_control)
        {
	atv.tv_sec = 0;
	if (offset >= 0)
	    {
	    TSFTOTVU (offset, atv.tv_usec);
	    }
	else
	    {
	    TSFTOTVU (-offset, atv.tv_usec);
	    atv.tv_usec = -atv.tv_usec;
	    }
	otv.tv_sec = time_constant;
	otv.tv_usec = 0;
	adjtime (&atv, &otv);
	if (otv.tv_usec < 0)
	    drift_comp = -(-otv.tv_usec >> (FREQ_SCALE - 16));
	else
	    drift_comp = otv.tv_usec >> (FREQ_SCALE - 16);
        }
    else
        {
	if (offset < 0)
	    {
	    clock_adjust = -((-offset) >> time_constant);
	    }
	else
	    {
	    clock_adjust = offset >> time_constant;
	    }

	/* 
	 * Calculate the new frequency error. The factor given in the
	 * spec gives the adjustment per 2**CLOCK_ADJ seconds, but we
	 * want it as a (scaled) pure ratio, so we include that factor
	 * now and remove it later.
	 */
	if (first_adjustment)
	    {
	    first_adjustment = 0;
	    }
	else
	    {
	    /* 
	     * Clamp the integration interval to maxpoll.
	     * The bitcounting in Section 5 gives (n+1)-6 for 2**n,
	     * but has a factor 2**6 missing from CLOCK_FREQ.
	     * We make 2**n give n instead. If watchdog_timer is
	     * zero, pretend it's one.
	     */
	    tmp = peer -> maxpoll;
	    tmp_uf = watchdog_timer;
	    if (tmp_uf == 0)
		tmp_uf = 1;
	    while (tmp_uf < (1 << peer -> maxpoll))
	        {
		tmp--;
		tmp_uf <<= 1;
	        }

	    /* 
	     * We apply the frequency scaling at the same time as
	     * the sample interval; this ensures a safe right-shift.
	     * (as long as it keeps below 31 bits, which current
	     *  parameters should ensure.
	     */
	    tmp = (RSH_FRAC_TO_FREQ - tmp) +
		time_constant + time_constant;
	    if (offset < 0)
		tmp = -((-offset) >> tmp);
	    else
		tmp = offset >> tmp;
	    drift_comp += tmp;
	    if (drift_comp > max_comp)
		drift_comp = max_comp;
	    else if (drift_comp < -max_comp)
		drift_comp = -max_comp;
	    }
        }
    watchdog_timer = 0;

    /* 
     * Determine when to adjust the time constant and poll interval.
     */
    if (current_time - tcadj_time >= (1 << sys_poll))
        {
	tcadj_time = current_time;
	tmp = offset;
	if (tmp < 0)
	    tmp = -tmp;
	tmp = tmp >> (16 + CLOCK_WEIGHTTC - time_constant);
	if (tmp > sys_maxd)
	    {
	    tc_counter = 0;
	    time_constant--;
	    }
	else
	    {
	    tc_counter++;
	    if (tc_counter > CLOCK_HOLDTC)
	        {
		tc_counter = 0;
		time_constant++;
	        }
	    }
	if (time_constant < peer -> minpoll - NTP_MINPOLL)
	    time_constant = peer -> minpoll - NTP_MINPOLL;
	if (time_constant > peer -> maxpoll - NTP_MINPOLL)
	    time_constant = peer -> maxpoll - NTP_MINPOLL;
        }
    sys_poll = (u_char)(NTP_MINPOLL + time_constant);

# ifdef DEBUG
    if (debug > 1)
	printf ("adj %s, drft %s, tau %3i\n",
		mfptoa ((clock_adjust < 0 ? -1 : 0), clock_adjust, 9),
		fptoa (drift_comp, 9), time_constant);
# endif				/* DEBUG */

    (void)record_loop_stats (&last_offset, &drift_comp, time_constant);

    /* 
     * Whew.  I've had enough.
     */
    return (0);
}


/*
 * adj_host_clock - Called every 2**CLOCK_ADJ seconds to update host clock
 */
void
adj_host_clock ()
{
    register LONG   adjustment;
# if defined(PPSCD)
    struct ppsclockev   ev;
    l_fp    ts;
# endif				/* PPSCD */

    watchdog_timer += (1 << CLOCK_ADJ);
    if (watchdog_timer >= NTP_MAXAGE)
        {
	first_adjustment = 1;	/* don't use next offset
				   for freq */
        }
    if (sys_refskew.l_i >= NTP_MAXSKEW)
	sys_refskew.l_f = 0;	/* clamp it */
    else
	L_ADDUF (&sys_refskew, NTP_SKEWINC);

# if defined(PPS) || defined(PPSDEV) || defined(PPSCD)
# 	if defined(PPSCD)
    /* 
     * Note that nothing ugly happens even if the CIOGETEV ioctl is
     * not configured. Correct for signal delays (!) for ultimate
     * finick.
     */
    if (ioctl (fdpps, CIOGETEV, (caddr_t)&ev) >= 0)
        {
	ts.l_ui = ev.tv.tv_sec + (U_LONG)JAN_1970;
	TVUTOTSF (ev.tv.tv_usec, ts.l_uf);
	L_SUB (&ts, &pps_delay);
	L_NEG (&ts);
	(void)pps_sample (&ts);
        }
# 	endif				/* PPSCD */
    if (pps_time != 0 && current_time - pps_time > PPS_MAXAGE)
	pps_time = 0;
    if (pps_update != 0 && current_time - pps_update > PPS_MAXUPDATE)
	pps_update = 0;
    if (pps_time != 0 && pps_update != 0)
        {
	if (!pps_control)
	    syslog (LOG_INFO, "pps synch");
	pps_control = 1;
        }
    else
        {
	if (pps_control)
	    syslog (LOG_INFO, "pps synch lost");
	pps_control = 0;
        }
# endif				/* PPS || PPSDEV || PPSCD 
				*/

    /* 
     * Resist the following code if the phase-lock loop has been
     * implemented in the kernel.
     */
    if (pll_control)
	return;
    adjustment = clock_adjust;
    if (adjustment < 0)
	adjustment = -((-adjustment) >> CLOCK_PHASE);
    else
	adjustment >>= CLOCK_PHASE;

    clock_adjust -= adjustment;
    if (drift_comp < 0)
	adjustment -= ((-drift_comp) >> RSH_DRIFT_TO_ADJ);
    else
	adjustment += drift_comp >> RSH_DRIFT_TO_ADJ;

        {
	l_fp    offset;
	offset.l_i = 0;
	offset.l_f = adjustment;
	adj_systime (&offset);
        }
}


/*
 * loop_config - configure the loop filter
 */
void
loop_config (item, lfp_value, int_value)
int     item;
l_fp   *lfp_value;
int     int_value;
{
    s_fp    tmp;
    struct timeval  atv,
                    otv;

    switch (item)
        {
	case LOOP_DRIFTCOMP:
	    tmp = LFPTOFP (lfp_value);
	    if (tmp >= max_comp || tmp <= -max_comp)
	        {
		syslog (LOG_ERR,
			"loop_config: skew compensation %s too large",
			fptoa (tmp, 5));
	        }
	    else
	        {

		/* 
		 * If the phase-lock code is implemented in the kernel,
		 * give the time_constant and saved frequency offset
		 * to the kernel. If not, no harm is done.
		 */
		drift_comp = tmp;
		atv.tv_sec = atv.tv_usec = 0;
		otv.tv_sec = time_constant;
		otv.tv_usec = drift_comp << (FREQ_SCALE - 16);
		pll_control = adjtime (&atv, &otv);
		if (pll_control)
		    syslog (LOG_INFO,
			    "using kernel phase-lock loop");
	        }
	    break;

	case LOOP_PPSDELAY:
	    pps_delay = *lfp_value;
	    break;

# if defined(PPSDEV)
	case LOOP_PPSBAUD:
# 	if defined(SYSV_TTYS)
	    /* 
	     * System V serial line parameters (termio interface)
	     */
	        {
		struct termio   ttyb;

		if (ioctl (fdpps, TCGETA, (char    *)&ttyb) < 0)
		    return;
	    ttyb.c_cflag = CS8 | CLOCAL | CREAD | int_value;
		if (ioctl (fdpps, TCSETA, (char    *)&ttyb) < 0)
		    return;
	        }
# 	endif				/* SYSV_TTYS */
# 	if defined(STREAM)
	    /* 
	     * System V STREAM serial line parameters
	     * (termios interface)
	     */
	        {
		struct termios  ttyb;

		if (ioctl (fdpps, TCGETS, (char    *)&ttyb) < 0)
		    return;
	    ttyb.c_cflag = CS8 | CLOCAL | CREAD | int_value;
		if (ioctl (fdpps, TCSETS, (char    *)&ttyb) < 0)
		    return;
	        }
# 	endif				/* STREAM */
# 	if defined(BSD_TTYS)

	    /* 
	     * 4.3bsd serial line parameters (sgttyb interface)
	     */
	        {
		struct sgttyb   ttyb;

		if (ioctl (fdpps, TIOCGETP, (char  *)&ttyb) < 0)
		    return;
	    ttyb.sg_ispeed = ttyb.sg_ospeed = SYS_BAUD (int_value);
		if (ioctl (fdpps, TIOCSETP, (char  *)&ttyb) < 0)
		    return;
	        }
# 	endif				/* BSD_TTYS */
	    pps_baud = int_value;
	    break;
# endif				/* PPSDEV */

	default:
	    /* sigh */
	    break;
        }
}

# if defined(PPSDEV)

/*
 * pps_receive - compute and store 1-pps signal offset
 * 
 * This routine is called once per second when the 1-pps signal is 
 * present. It calculates the offset of the local clock relative to the
 * 1-pps signal and saves in a circular buffer for later use. If the
 * clock line discipline is active, its timestamp is used; otherwise,
 * the buffer timestamp is used.
 */
static void
pps_receive (rbufp)
struct recvbuf *rbufp;
{
    u_char *dpt;		/* buffer pointer */
    l_fp    ts;			/* l_fp temps */
    int     dpend;		/* buffer length */

    /* 
     * Set up pointers, check the buffer length, discard intercept
     * character and convert unix timeval to timestamp format.
     */
    dpt = (u_char *)&rbufp -> recv_space;
    dpend = rbufp -> recv_length;
# 	if !defined(HPUXGADGET)
    dpt++;
    dpend--;
# 	endif				/* HPUXGADGET */
    if (dpend != sizeof (struct timeval)||!buftvtots (dpt, &ts))
	ts = rbufp -> recv_time;

    /* 
     * Correct for uart and os delay and process sample offset.
     */
    L_SUB (&ts, &pps_delay);
    L_NEG (&ts);
    (void)pps_sample (&ts);
}
# endif				/* PPSDEV */

# if defined(PPS) || defined(PPSDEV) || defined(PPSCD)
/*
 * pps_sample - process pps sample offset
 */
int     pps_sample (tsr)
l_fp   *tsr;
{
    int     i,
            j;			/* temp ints */
    LONG    sort[NPPS];		/* temp array for sorting 
				*/
    l_fp    lftemp,
            ts;			/* l_fp temps */
    LONG    ltemp;		/* long temp */

    /* 
     * Note the seconds offset is already in the low-order timestamp
     * doubleword, so all we have to do is sign-extend and invert it.
     * The resulting offset is believed only if within CLOCK_MAX.
     */
    ts = *tsr;
    lftemp.l_i = lftemp.l_f = 0;
    M_ADDF (lftemp.l_i, lftemp.l_f, ts.l_f);
    if (ts.l_f <= -CLOCK_MAX_F || ts.l_f >= CLOCK_MAX_F)
        {
	pps_time = 0;
	return (-1);
        }

    /* 
     * Save the sample in a circular buffer for later processing.
     */
    nsamples++;
    i = ((int)(nsamples)) % NPPS;
    samples[i] = lftemp.l_f;
    if (i != NPPS - 1)
	return (0);

    /* 
     * When the buffer fills up, construct an array of sorted
     * samples.
     */
    pps_time = current_time;
    for (i = 0; i < NPPS; i++)
        {
	sort[i] = samples[i];
	for (j = 0; j < i; j++)
	    {
	    if (sort[j] > sort[i])
	        {
		ltemp = sort[j];
		sort[j] = sort[i];
		sort[i] = ltemp;
	        }
	    }
        }

    /* 
     * Compute offset as the average of all samples in the filter
     * less PPS_TRIM samples trimmed from the beginning and end,
     * dispersion as the difference between max and min of samples
     * retained. The system stratum, root delay and root dispersion
     * are also set here.
     */
    pps_offset.l_i = pps_offset.l_f = 0;
    for (i = PPS_TRIM; i < NPPS - PPS_TRIM; i++)
	M_ADDF (pps_offset.l_i, pps_offset.l_f, sort[i]);
    if (L_ISNEG (&pps_offset))
        {
	L_NEG (&pps_offset);
	for (i = 0; i < PPS_FAC; i++)
	    L_RSHIFT (&pps_offset);
	L_NEG (&pps_offset);
        }
    else
        {
	for (i = 0; i < PPS_FAC; i++)
	    L_RSHIFT (&pps_offset);
        }
    lftemp.l_i = 0;
    lftemp.l_f = sort[NPPS - 1 - PPS_TRIM] - sort[PPS_TRIM];
    sys_maxd = LFPTOFP (&lftemp);
# 	ifdef DEBUG
    if (debug)
	printf ("pps_filter: %s %s %s\n", lfptoa (&pps_delay, 6),
		lfptoa (&pps_offset, 6), lfptoa (&lftemp, 5));
# 	endif				/* DEBUG */
    record_peer_stats (&loopback_interface -> sin, ctlsysstatus (), &pps_offset,
	    sys_rootdelay, sys_rootdispersion);
    return (0);
}
# endif				/* PPS || PPSDEV || PPSCD 
				*/
