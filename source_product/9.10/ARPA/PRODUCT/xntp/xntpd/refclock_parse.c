# if defined(REFCLOCK) && defined(PARSE)
/*
 * $Header: refclock_parse.c,v 1.2.109.3 94/11/09 10:59:55 mike Exp $
 *
 * generic reference clock driver for receivers
 * driven by a streams module on top of the tty (SunOS4.x)
 * if the compile time option STREAM is not defined the module
 * will default to standard tty usage. Due to SunOS 4.x STREAMS
 * implementation a jitter of up to 20ms may be observered.
 *
 * Copyright (c) 1989,1990,1991,1992,1993
 * Frank Kardel Friedrich-Alexander Universitaet Erlangen-Nuernberg
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * refclock_parse.c,v
 * Revision 3.5  1993/07/09  23:36:59  kardel
 * SYSV_TTYS used to produce errors 8-( - BSD driver support still lacking
 *
 * Revision 3.4  1993/07/09  12:42:29  kardel
 * RAW DCF now officially released
 *
 * Revision 3.3  1993/07/09  11:50:37  kardel
 * running GPS also on 960 to be able to switch GPS/DCF77
 *
 * Revision 3.2  1993/07/09  11:37:34  kardel
 * Initial restructured version + GPS support
 *
 * Revision 3.1  1993/07/06  10:01:07  kardel
 * DCF77 driver goes generic...
 *
 */

# 	include <stdio.h>
# 	include <ctype.h>
# 	include <sys/types.h>
# 	include <string.h>
# 	include <sys/time.h>
# 	include <sys/file.h>
# 	include <fcntl.h>

# 	ifdef STREAM
# 		include <sys/stream.h>
# 		include <sys/stropts.h>
# 	endif	/* STREAM */

# 	include <sys/errno.h>

# 	include "ntpd.h"
# 	include "ntp_refclock.h"
# 	include "ntp_unixtime.h"

# 	if defined(SYSV_TTYS)
# 		include <sys/termios.h>
# 	else				/* SYSV_TTYS */

/*
 * currently no BSD tty driver support - might change when
 * line discipline is available
 */

CURRENTLY   NO BSD TTY DRIVER SUPPORT - SORRY

# 	endif	/* defined (SYSV_TTYS) */

# 	include "parse.h"

# 	if !defined(NO_SCCSID) && !defined(lint) && !defined(__GNUC__)
static char     rcsid[] = "refclock_parse.c,v 3.5 1993/07/09 23:36:59 kardel Exp";
# 	endif	/* !defined (NO_SCCSID) && !defined(lint) && !defined(__GNUC__) */

static void     parse_init P ((void));
static int  parse_start P ((u_int, struct peer *));
static void     parse_shutdown P ((int));
static void     parse_poll P ((int , struct peer   *));
static void     parse_control P ((u_int, struct refclockstat   *, struct refclockstat  *));

# 	define	parse_buginfo	noentry

struct refclock     refclock_parse =
{
    parse_start,
	parse_shutdown,
	parse_poll,
	parse_control,
	parse_init,
	parse_buginfo,
	NOFLAGS
};

/*
 * This driver currently provides the support for
 *   - Meinberg DCF77 receiver DCF77 PZF 535 (TCXO version)
 *   - Meinberg DCF77 receiver DCF77 PZF 535 (OCXO version)
 *   - Meinberg DCF77 receiver U/A 31
 *   - ELV DCF7000
 *   - Schmid clock
 *   - Conrad DCF77 receiver module
 *   - FAU DCF77 NTP receiver (TimeBrick)
 *   - Meinberg GPS166
 *
 * Meinberg receivers are connected via a 9600 baud serial line
 *
 * Some receivers do NOT support:
 *          - announcement of alternate sender usage
 *          - leap second indication
 *
 * so...
 *          - for PZF535 please ask for revision PZFUERL4.6 or higher
 *
 * Meinberg receiver setup:
 *	output time code every second
 *	Baud rate 9600 7E2S
 */

/*
 * the unit field selects for one the prot to be used (lower 4 bits)
 * and for the other the clock type in case of different but similar
 * receivers (bits 4-6)
 * the most significat bit encodes PPS support
 * when the most significant bit is set the pps telegrams will be used
 * for controlling the local clock (ntp_loopfilter.c)
 * receiver specific configration data is kept in the clockinfo field.
 */

/*
 * Definitions
 */
# 	define	MAXUNITS	4	/* maximum number of
				   "PARSE" units permitted 
				*/
# 	define PARSEDEVICE	"/dev/refclock-%d"
				/* device to open %d is
				   unit number */

struct parseunit
{
    /* 
     * XNTP management
     */
    struct peer    *peer;	/* backlink to peer
				   structure - refclock
				   inactive if 0  */
    int     fd;			/* device file descriptor 
				*/
    u_char  unit;		/* encoded unit/type/PPS */
    struct refclockio   io;	/* io system structure
				   (used in PPS mode) */
# 	ifndef STREAM
                        parse_t parseio;
				/* io handling structure */
# 	endif	/* not STREAM */

    /* 
     * type specific parameters
     */
    struct clockinfo   *parse_type; /* link to clock
				   description */

    /* 
     * clock specific configuration
     */
    l_fp    basedelay;		/* clock local phase offset
				   */

    /* 
     * clock state handling/reporting
     */
    u_char  flags;		/* flags (leap_control) */
    u_char  status;		/* current status */
    u_char  lastevent;		/* last not NORMAL status 
				*/
    U_LONG  lastchange;		/* time (xntp) when last
				   state change accured */
    U_LONG  statetime[CEVNT_MAX + 1];
				/* accumulated time of
				   clock states */
    struct event    stattimer;	/* statistics timer */
    U_LONG  polls;		/* polls from NTP protocol
				   machine */
    U_LONG  noresponse;		/* number of expected but
				   not seen datagrams */
    U_LONG  badformat;		/* bad format (failed
				   format conversions) */
    U_LONG  baddata;		/* usually bad receive
				   length, bad format */

    u_char  pollonly;		/* 1 for polling only (no
				   PPS mode) */
    u_char  pollneeddata;	/* 1 for receive sample
				   expected in PPS mode */
    U_LONG  laststatus;		/* last packet status
				   (error indication) */
    u_short     lastformat;	/* last format used */
    U_LONG  lastsync;		/* time (xntp) when clock
				   was last seen fully
				   synchronized */
    U_LONG  timestarted;	/* time (xntp) when peer
				   clock was instantiated 
				*/
    U_LONG  nosynctime;		/* time (xntp) when last
				   nosync message was
				   posted */
    U_LONG  lastmissed;		/* time (xntp) when poll
				   didn't get data (powerup
				   heuristic) */
    parsetime_t     time;	/* last (parse module) data
				   */
    void   *localdata;		/* optional local data */
};

# 	define NO_POLL		(void (*)())0
# 	define NO_INIT		(int  (*)())0
# 	define NO_END		(void (*)())0
# 	define NO_DATA		(void *)0
# 	define NO_FORMAT	""

# 	define DCF_ID		"DCF"	/* generic DCF */
# 	define DCF_A_ID	"DCFa"	/* AM demodulation */
# 	define DCF_P_ID	"DCFp"	/* psuedo random phase
				   shift */
# 	define GPS_ID		"GPS"	/* GPS receiver */

# 	define NOCLOCK_PRECISION	(-1)
# 	define	NOCLOCK_ROOTDELAY	0x00000000
# 	define	NOCLOCK_BASEDELAY	0x00000000
# 	define	NOCLOCK_DESCRIPTION	((char *)0)
# 	define NOCLOCK_MAXUNSYNC       0
# 	define NOCLOCK_CFLAG           0
# 	define NOCLOCK_IFLAG           0
# 	define NOCLOCK_OFLAG           0
# 	define NOCLOCK_LFLAG           0
# 	define NOCLOCK_ID		"TILT"
# 	define NOCLOCK_POLL		NO_POLL
# 	define NOCLOCK_INIT		NO_INIT
# 	define NOCLOCK_END		NO_END
# 	define NOCLOCK_DATA		NO_DATA
# 	define NOCLOCK_FORMAT		NO_FORMAT

/*
 * receiver specific constants
 */
# 	define MBG_CFLAG19200		(B19200|CS7|PARENB|CREAD|HUPCL)
# 	define MBG_CFLAG		(B9600|CS7|PARENB|CREAD|HUPCL)
# 	define MBG_IFLAG		(IGNBRK|IGNPAR|ISTRIP)
# 	define MBG_OFLAG		0
# 	define MBG_LFLAG		0
/*
 * Meinberg DCF U/A 31 (AM) receiver
 */
# 	define	DCFUA31_PRECISION	(-8)
				/* accounts for receiver
				   errors */
# 	define	DCFUA31_ROOTDELAY	0x00000D00  /* 50.78125ms */
# 	define	DCFUA31_BASEDELAY	0x02C00000
				/* 10.7421875ms: 10 ms (+/-
				   3 ms) */
# 	define	DCFUA31_DESCRIPTION	"Meinberg DCF U/A 31"
# 	define DCFUA31_MAXUNSYNC       60*30
				/* only trust clock for 1/2
				   hour */
# 	define DCFUA31_CFLAG           MBG_CFLAG
# 	define DCFUA31_IFLAG           MBG_IFLAG
# 	define DCFUA31_OFLAG           MBG_OFLAG
# 	define DCFUA31_LFLAG           MBG_LFLAG

/*
 * Meinberg DCF PZF535/TCXO (FM/PZF) receiver
 */
# 	define	DCFPZF535_PRECISION	(-18)	/* 355kHz clock
				   (f(DCF)*4(PLL)) */
# 	define	DCFPZF535_ROOTDELAY	0x00000034  /* 800us */
# 	define	DCFPZF535_BASEDELAY	0x00800000
				/* 1.968ms +- 104us
				   (oscilloscope) -
				   relative to start (end
				   of STX) */
# 	define	DCFPZF535_DESCRIPTION	"Meinberg DCF PZF 535/TCXO"
# 	define DCFPZF535_MAXUNSYNC     60*60*12
				/* only trust clock for 12
				   hours * @ 5e-8df/f we
				   have accumulated * at
				   most 2.16 ms (thus we
				   move to * NTP
				   synchronisation */
# 	define DCFPZF535_CFLAG         MBG_CFLAG
# 	define DCFPZF535_IFLAG         MBG_IFLAG
# 	define DCFPZF535_OFLAG         MBG_OFLAG
# 	define DCFPZF535_LFLAG         MBG_LFLAG


/*
 * Meinberg DCF PZF535/OCXO receiver
 */
# 	define	DCFPZF535OCXO_PRECISION	(-18)	/* 355kHz clock
				   (f(DCF)*4(PLL)) */
# 	define	DCFPZF535OCXO_ROOTDELAY	0x00000034
				/* 800us (max error * 10) 
				*/
# 	define	DCFPZF535OCXO_BASEDELAY	0x00800000
				/* 1.968ms +- 104us
				   (oscilloscope) -
				   relative to start (end
				   of STX) */
# 	define	DCFPZF535OCXO_DESCRIPTION "Meinberg DCF PZF 535/OCXO"
# 	define DCFPZF535OCXO_MAXUNSYNC     60*60*96
				/* only trust clock for 4
				   days * @ 5e-9df/f we
				   have accumulated * at
				   most an error of 1.73 ms *
				   (thus we move to NTP
				   synchronisation) */
# 	define DCFPZF535OCXO_CFLAG         MBG_CFLAG
# 	define DCFPZF535OCXO_IFLAG         MBG_IFLAG
# 	define DCFPZF535OCXO_OFLAG         MBG_OFLAG
# 	define DCFPZF535OCXO_LFLAG         MBG_LFLAG

/*
 * Meinberg GPS166 receiver
 */
# 	define	GPS166_PRECISION	(-23)	/* 100 ns */
# 	define	GPS166_ROOTDELAY	0x00000000
				/* nothing here */
# 	define	GPS166_BASEDELAY	0x00800000
				/* XXX to be fixed !
				   1.968ms +- 104us
				   (oscilloscope) -
				   relative to start (end
				   of STX) */
# 	define	GPS166_DESCRIPTION      "Meinberg GPS166 receiver"
# 	define GPS166_MAXUNSYNC        60*60*96
				/* only trust clock for 4
				   days * @ 5e-9df/f we
				   have accumulated * at
				   most an error of 1.73 ms *
				   (thus we move to NTP
				   synchronisation) */
# 	define GPS166_CFLAG            MBG_CFLAG
# 	define GPS166_IFLAG            MBG_IFLAG
# 	define GPS166_OFLAG            MBG_OFLAG
# 	define GPS166_LFLAG            MBG_LFLAG
# 	define GPS166_POLL		NO_POLL
# 	define GPS166_INIT		NO_INIT
# 	define GPS166_END		NO_END
# 	define GPS166_DATA		NO_DATA
# 	define GPS166_ID		GPS_ID
# 	define GPS166_FORMAT		NO_FORMAT

/*
 * ELV DCF7000 Wallclock-Receiver/Switching Clock (Kit)
 *
 * This is really not the hottest clock - but before you have nothing ...
 */
# 	define DCF7000_PRECISION (-8)	/* 
  * I don't hav much faith in this - but I haven't
  * evaluated it yet
  */
# 	define DCF7000_ROOTDELAY	0x00000364  /* 13 ms */
# 	define DCF7000_BASEDELAY	0x67AE0000
				/* 405 ms - slow blow */
# 	define DCF7000_DESCRIPTION	"ELV DCF7000"
# 	define DCF7000_MAXUNSYNC	(60*5)
				/* sorry - but it just was
				   not build as a clock */
# 	define DCF7000_CFLAG           (B9600|CS8|CREAD|PARENB|PARODD|CLOCAL|HUPCL)
# 	define DCF7000_IFLAG		(IGNBRK)
# 	define DCF7000_OFLAG		0
# 	define DCF7000_LFLAG		0

/*
 * Schmid DCF Receiver Kit
 *
 * When the WSDCF clock is operating optimally we want the primary clock
 * distance to come out at 300 ms.  Thus, peer.distance in the WSDCF peer
 * structure is set to 290 ms and we compute delays which are at least
 * 10 ms long.  The following are 290 ms and 10 ms expressed in u_fp format
 */
static void     wsparse_dpoll P ((struct parseunit *));
static void     wsparse_poll P ((struct parseunit  *));
static int  wsparse_init P ((struct parseunit  *));
static void     wsparse_end P ((struct parseunit   *));

# 	define WSDCF_INIT		wsparse_init
# 	define WSDCF_POLL		wsparse_dpoll
# 	define WSDCF_END		wsparse_end
# 	define	WSDCF_PRECISION		(-9)	/* what the heck */
# 	define	WSDCF_ROOTDELAY		0X00004A3D  /*  ~ 290ms */
# 	define	WSDCF_BASEDELAY	 	0x028F5C29  /*  ~  10ms */
# 	define WSDCF_DESCRIPTION	"WS/DCF Receiver"
# 	define WSDCF_FORMAT		"Schmid"
# 	define WSDCF_MAXUNSYNC		(60*60)
				/* assume this beast hold
				   at 1 h better than 2 ms
				   XXX-must verify */
# 	define WSDCF_CFLAG		(B1200|CS8|CREAD|CLOCAL)
# 	define WSDCF_IFLAG		0
# 	define WSDCF_OFLAG		0
# 	define WSDCF_LFLAG		0

/*
 * RAW DCF77 - input of DCF marks via RS232 - many variants
 */
static int  rawdcf_init ();

# 	define RAWDCF_PRECISION (-9)	/* expect only  ms
				   accurancy */
# 	define RAWDCF_ROOTDELAY	0x00000364  /* 13 ms */
# 	define RAWDCF_FORMAT		"RAW DCF77 Timecode"
# 	define RAWDCF_MAXUNSYNC	(0) /* sorry - its a true
				   receiver - no signal -
				   no time */
# 	define RAWDCF_CFLAG            (B50|CS8|CREAD|CLOCAL)
# 	define RAWDCF_IFLAG		0
# 	define RAWDCF_OFLAG		0
# 	define RAWDCF_LFLAG		0

/*
 * RAW DCF variants
 */
/*
 * Conrad receiver
 *
 * simplest (cheapest) DCF clock - e. g. DCF77 receiver by Conrad
 * (~40DM - roughly $30 ) followed by a level converter for RS232
 */
# 	define CONRAD_BASEDELAY	0x420C49B0
				/* ~258 ms - Conrad
				   receiver @ 50 Baud on a
				   Sun */
# 	define CONRAD_DESCRIPTION	"RAW DCF77 CODE (Conrad DCF77 receiver module)"

/*
 * TimeBrick receiver
 */
# 	define TIMEBRICK_BASEDELAY	0x35C29000
				/* ~210 ms - TimeBrick @ 50
				   Baud on a Sun */
# 	define TIMEBRICK_DESCRIPTION	"RAW DCF77 CODE (TimeBrick)"

static struct clockinfo
{
    void ( *cl_poll) ();	/* active poll routine */
    int (  *cl_init) ();	/* active poll init routine
				   */
    void ( *cl_end) ();		/* active poll end routine 
				*/
    void   *cl_data;		/* local data area for
				   "poll" mechanism */
    u_fp    cl_rootdelay;	/* rootdelay */
    U_LONG  cl_basedelay;	/* current offset -
				   unsigned l_fp fractional
				   part */
    s_char  cl_precision;	/* device precision */
    char   *cl_id;		/* ID code (usually "DCF") 
				*/
    char   *cl_description;	/* device name */
    char   *cl_format;		/* fixed format */
    U_LONG  cl_maxunsync;	/* time to trust oscillator
				   after loosing synch */
    U_LONG  cl_cflag;		/* terminal io flags */
    U_LONG  cl_iflag;		/* terminal io flags */
    U_LONG  cl_oflag;		/* terminal io flags */
    U_LONG  cl_lflag;		/* terminal io flags */
}       clockinfo[] =
{				/*   0.  0.0.128 - base
				   offset for PPS support 
				*/
        {			/* 127.127.8.<device> */
	NO_POLL,
	    NO_INIT,
	    NO_END,
	    NO_DATA,
	    DCFPZF535_ROOTDELAY,
	    DCFPZF535_BASEDELAY,
	    DCFPZF535_PRECISION,
	    DCF_P_ID,
	    DCFPZF535_DESCRIPTION,
	    NO_FORMAT,
	    DCFPZF535_MAXUNSYNC,
	    DCFPZF535_CFLAG,
	    DCFPZF535_IFLAG,
	    DCFPZF535_OFLAG,
	    DCFPZF535_LFLAG
        },
        {			/* 127.127.8.4+<device> */
	NO_POLL,
	    NO_INIT,
	    NO_END,
	    NO_DATA,
	    DCFPZF535OCXO_ROOTDELAY,
	    DCFPZF535OCXO_BASEDELAY,
	    DCFPZF535OCXO_PRECISION,
	    DCF_P_ID,
	    DCFPZF535OCXO_DESCRIPTION,
	    NO_FORMAT,
	    DCFPZF535OCXO_MAXUNSYNC,
	    DCFPZF535OCXO_CFLAG,
	    DCFPZF535OCXO_IFLAG,
	    DCFPZF535OCXO_OFLAG,
	    DCFPZF535OCXO_LFLAG
        },
        {			/* 127.127.8.8+<device> */
	NO_POLL,
	    NO_INIT,
	    NO_END,
	    NO_DATA,
	    DCFUA31_ROOTDELAY,
	    DCFUA31_BASEDELAY,
	    DCFUA31_PRECISION,
	    DCF_A_ID,
	    DCFUA31_DESCRIPTION,
	    NO_FORMAT,
	    DCFUA31_MAXUNSYNC,
	    DCFUA31_CFLAG,
	    DCFUA31_IFLAG,
	    DCFUA31_OFLAG,
	    DCFUA31_LFLAG
        },
        {			/* 127.127.8.12+<device> */
	NO_POLL,
	    NO_INIT,
	    NO_END,
	    NO_DATA,
	    DCF7000_ROOTDELAY,
	    DCF7000_BASEDELAY,
	    DCF7000_PRECISION,
	    DCF_A_ID,
	    DCF7000_DESCRIPTION,
	    NO_FORMAT,
	    DCF7000_MAXUNSYNC,
	    DCF7000_CFLAG,
	    DCF7000_IFLAG,
	    DCF7000_OFLAG,
	    DCF7000_LFLAG
        },
        {			/* 127.127.8.16+<device> */
	WSDCF_POLL,
	    WSDCF_INIT,
	    WSDCF_END,
	    NO_DATA,
	    WSDCF_ROOTDELAY,
	    WSDCF_BASEDELAY,
	    WSDCF_PRECISION,
	    DCF_A_ID,
	    WSDCF_DESCRIPTION,
	    WSDCF_FORMAT,
	    WSDCF_MAXUNSYNC,
	    WSDCF_CFLAG,
	    WSDCF_IFLAG,
	    WSDCF_OFLAG,
	    WSDCF_LFLAG
        },
        {			/* 127.127.8.20+<device> */
	NO_POLL,
	    rawdcf_init,
	    NO_END,
	    NO_DATA,
	    RAWDCF_ROOTDELAY,
	    CONRAD_BASEDELAY,
	    RAWDCF_PRECISION,
	    DCF_A_ID,
	    CONRAD_DESCRIPTION,
	    RAWDCF_FORMAT,
	    RAWDCF_MAXUNSYNC,
	    RAWDCF_CFLAG,
	    RAWDCF_IFLAG,
	    RAWDCF_OFLAG,
	    RAWDCF_LFLAG
        },
        {			/* 127.127.8.24+<device> */
	NO_POLL,
	    rawdcf_init,
	    NO_END,
	    NO_DATA,
	    RAWDCF_ROOTDELAY,
	    TIMEBRICK_BASEDELAY,
	    RAWDCF_PRECISION,
	    DCF_A_ID,
	    TIMEBRICK_DESCRIPTION,
	    RAWDCF_FORMAT,
	    RAWDCF_MAXUNSYNC,
	    RAWDCF_CFLAG,
	    RAWDCF_IFLAG,
	    RAWDCF_OFLAG,
	    RAWDCF_LFLAG
        },
        {			/* 127.127.8.28+<device> */
	GPS166_POLL,
	    GPS166_INIT,
	    GPS166_END,
	    GPS166_DATA,
	    GPS166_ROOTDELAY,
	    GPS166_BASEDELAY,
	    GPS166_PRECISION,
	    GPS166_ID,
	    GPS166_DESCRIPTION,
	    GPS166_FORMAT,
	    GPS166_MAXUNSYNC,
	    GPS166_CFLAG,
	    GPS166_IFLAG,
	    GPS166_OFLAG,
	    GPS166_LFLAG
        }
};

static int  ncltypes = sizeof (clockinfo)/sizeof (struct clockinfo);

# 	define CL_REALTYPE(x) (((x) >> 2) & 0x1F)
# 	define CL_TYPE(x)  ((CL_REALTYPE(x) >= ncltypes) ? ~0 : CL_REALTYPE(x))
# 	define CL_PPS(x)   ((x) & 0x80)
# 	define CL_UNIT(x)  ((x) & 0x3)

/*
 * Other constant stuff
 */
# 	define	PARSEHSREFID	0x7f7f08ff
				/* 127.127.8.255 refid for
				   hi strata */

# 	define PARSENOSYNCREPEAT (10*60)   /* mention uninitialized
				   clocks all 10 minutes */
# 	define PARSESTATISTICS   (60*60)
				/* output state statistics
				   every hour */

static struct parseunit    *parseunits[MAXUNITS];

extern U_LONG   current_time;
extern struct event     timerqueue[];

# 	define PARSE_STATETIME(parse, i) ((parse->status == i) ? parse->statetime[i] + current_time - parse->lastchange : parse->statetime[i])

static void     parse_process P ((struct parseunit *, parsetime_t *));

/*--------------------------------------------------
 * convert a flag field to a string
 */
static char    *
parsestate (state, buffer)
unsigned    LONG state;
char   *buffer;
{
    static struct bits
        {
	unsigned    LONG bit;
	char   *name;
        }   flagstrings[] =
        {
	    {
	    PARSEB_ANNOUNCE, "DST SWITCH WARNING"
	    },
	    {
	    PARSEB_POWERUP, "NOT SYNCHRONIZED"
	    },
	    {
	    PARSEB_NOSYNC, "TIME CODE NOT CONFIRMED"
	    },
	    {
	    PARSEB_DST, "DST"
	    },
	    {
	    PARSEB_UTC, "UTC DISPLAY"
	    },
	    {
	    PARSEB_LEAP, "LEAP WARNING"
	    },
	    {
	    PARSEB_LEAPSECOND, "LEAP SECOND"
	    },
	    {
	    PARSEB_ALTERNATE, "ALTERNATE ANTENNA"
	    },
	    {
	    PARSEB_TIMECODE, "TIME CODE"
	    },
	    {
	    PARSEB_PPS, "PPS"
	    },
	    {
	    PARSEB_POSITION, "POSITION"
	    },
	    {
	    0
	    }
        };

    static struct sbits
        {
	unsigned    LONG bit;
	char   *name;
        }   sflagstrings[] =
        {
	    {
	    PARSEB_S_LEAP, "LEAP INDICATION"
	    },
	    {
	    PARSEB_S_PPS, "PPS SIGNAL"
	    },
	    {
	    PARSEB_S_ANTENNA, "ANTENNA"
	    },
	    {
	    PARSEB_S_POSITION, "POSITION"
	    },
	    {
	    0
	    }
        };
    int     i;

    *buffer = '\0';

    i = 0;
    while (flagstrings[i].bit)
        {
	if (flagstrings[i].bit & state)
	    {
	    if (buffer[0])
		strcat (buffer, "; ");
	    strcat (buffer, flagstrings[i].name);
	    }
	i++;
        }

    if (state & (PARSEB_S_LEAP | PARSEB_S_ANTENNA | PARSEB_S_PPS | PARSEB_S_POSITION))
        {
	register char  *s,
	               *t;

	if (buffer[0])
	    strcat (buffer, "; ");

	strcat (buffer, "(");

	t = s = buffer + strlen (buffer);

	i = 0;
	while (sflagstrings[i].bit)
	    {
	    if (sflagstrings[i].bit & state)
	        {
		if (t != s)
		    {
		    strcpy (t, "; ");
		    t += 2;
		    }

		strcpy (t, sflagstrings[i].name);
		t += strlen (t);
	        }
	    i++;
	    }
	strcpy (t, ")");
        }
    return buffer;
}

/*--------------------------------------------------
 * convert a status flag field to a string
 */
static char    *
parsestatus (state, buffer)
unsigned    LONG state;
char   *buffer;
{
    static struct bits
        {
	unsigned    LONG bit;
	char   *name;
        }   flagstrings[] =
        {
	    {
	    CVT_OK, "CONVERSION SUCCESSFUL"
	    },
	    {
	    CVT_NONE, "NO CONVERSION"
	    },
	    {
	    CVT_FAIL, "CONVERSION FAILED"
	    },
	    {
	    CVT_BADFMT, "ILLEGAL FORMAT"
	    },
	    {
	    CVT_BADDATE, "DATE ILLEGAL"
	    },
	    {
	    CVT_BADTIME, "TIME ILLEGAL"
	    },
	    {
	    0
	    }
        };
    int     i;

    *buffer = '\0';

    i = 0;
    while (flagstrings[i].bit)
        {
	if (flagstrings[i].bit & state)
	    {
	    if (buffer[0])
		strcat (buffer, "; ");
	    strcat (buffer, flagstrings[i].name);
	    }
	i++;
        }

    return buffer;
}

/*--------------------------------------------------
 * convert a clock status flag field to a string
 */
static char    *
clockstatus (state)
unsigned    LONG state;
{
    static char     buffer[20];
    static struct status
        {
	unsigned    LONG value;
	char   *name;
        }   flagstrings[] =
        {
	    {
	    CEVNT_NOMINAL, "NOMINAL"
	    },
	    {
	    CEVNT_TIMEOUT, "NO RESPONSE"
	    },
	    {
	    CEVNT_BADREPLY, "BAD FORMAT"
	    },
	    {
	    CEVNT_FAULT, "FAULT"
	    },
	    {
	    CEVNT_PROP, "PROPAGATION DELAY"
	    },
	    {
	    CEVNT_BADDATE, "ILLEGAL DATE"
	    },
	    {
	    CEVNT_BADTIME, "ILLEGAL TIME"
	    },
	    {
	    ~0
	    }
        };
    int     i;

    i = 0;
    while (flagstrings[i].value != ~0)
        {
	if (flagstrings[i].value == state)
	    {
	    return flagstrings[i].name;
	    }
	i++;
        }

    sprintf (buffer, "unknown #%d", state);

    return buffer;
}

/*--------------------------------------------------
 * mkascii - make a printable ascii string
 * assumes (unless defined better) 7-bit ASCII
 */
# 	ifndef isprint
# 		define isprint(_X_) (((_X_) > 0x1F) && ((_X_) < 0x7F))
# 	endif	/* not isprint */

static char    *
mkascii (buffer, blen, src, srclen)
register char  *buffer;
register LONG   blen;
register char  *src;
register LONG   srclen;
{
    register char  *b = buffer;
    register char  *endb = (char   *)0;

    if (blen < 4)
	return (char   *)0;	/* don't bother with mini
				   buffers */

    endb = buffer + blen - 4;

    blen--;			/* account for '\0' */

    while (blen && srclen--)
        {
	if ((*src != '\\') && isprint (*src))
	    {			/* printables are easy... 
				*/
	    *buffer++ = *src++;
	    blen--;
	    }
	else
	    {
	    if (blen < 4)
	        {
		while (blen--)
		    {
		    *buffer++ = '.';
		    }
		*buffer = '\0';
		return b;
	        }
	    else
	        {
		if (*src == '\\')
		    {
		    strcpy (buffer, "\\\\");
		    buffer += 2;
		    blen -= 2;
		    }
		else
		    {
		    sprintf (buffer, "\\x%02x", *src++);
		    blen -= 4;
		    buffer += 4;
		    }
	        }
	    }
	if (srclen && !blen && endb)	/* overflow - set last
				   chars to ... */
	    strcpy (endb, "...");
        }

    *buffer = '\0';
    return b;
}


/*--------------------------------------------------
 * l_mktime - make representation of a relative time
 */
static char    *
l_mktime (delta)
unsigned    LONG delta;
{
    unsigned    LONG tmp,
                m,
                s;
    static char     buffer[40];

    buffer[0] = '\0';

    if ((tmp = delta / (60 * 60 * 24)) != 0)
        {
	sprintf (buffer, "%dd+", tmp);
	delta -= tmp * 60 * 60 * 24;
        }

    s = delta % 60;
    delta /= 60;
    m = delta % 60;
    delta /= 60;

    sprintf (buffer + strlen (buffer), "%02d:%02d:%02d",
	    delta, m, s);

    return buffer;
}


/*--------------------------------------------------
 * parse_init - initialize internal parse driver data
 */
static void
parse_init ()
{
    bzero ((caddr_t)parseunits, sizeof parseunits);
}

/*--------------------------------------------------
 * parse_statistics - list summary of clock states
 */
static void
parse_statistics (parse)
register struct parseunit  *parse;
{
    register int    i;

    syslog (LOG_INFO, "PARSE receiver #%d: running time: %s",
	    CL_UNIT (parse -> unit),
	    l_mktime (current_time - parse -> timestarted));

    syslog (LOG_INFO, "PARSE receiver #%d: current status: %s",
	    CL_UNIT (parse -> unit),
	    clockstatus (parse -> status));

    for (i = 0; i <= CEVNT_MAX; i++)
        {
	register unsigned   LONG stime;
	register unsigned   LONG percent,
	                    div = current_time - parse -> timestarted;

	percent = stime = PARSE_STATETIME (parse, i);

	while (((unsigned   LONG) (~0) / 10000) < percent)
	    {
	    percent /= 10;
	    div /= 10;
	    }

	if (div)
	    percent = (percent * 10000) / div;
	else
	    percent = 10000;

	if (stime)
	    syslog (LOG_INFO, "PARSE receiver #%d: state %18s: %13s (%3d.%02d%%)",
		    CL_UNIT (parse -> unit),
		    clockstatus (i),
		    l_mktime (stime),
		    percent / 100, percent % 100);
        }
}

/*--------------------------------------------------
 * cparse_statistics - wrapper for statistics call
 */
static void
cparse_statistics (peer)
register struct peer   *peer;
{
    register struct parseunit  *parse = (struct parseunit  *)peer;

    parse_statistics (parse);
    parse -> stattimer.event_time = current_time + PARSESTATISTICS;
    TIMER_ENQUEUE (timerqueue, &parse -> stattimer);
}

/*--------------------------------------------------
 * parse_shutdown - shut down a PARSE clock
 */
static void
parse_shutdown (unit)
int     unit;
{
    register struct parseunit  *parse;

    unit = CL_UNIT (unit);

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR,
		"PARSE receiver #%d: parse_shutdown: INTERNAL ERROR, unit invalid (max %d)",
		unit, MAXUNITS);
	return;
        }

    parse = parseunits[unit];

    if (parse && !parse -> peer)
        {
	syslog (LOG_ERR,
		"PARSE receiver #%d: parse_shutdown: INTERNAL ERROR, unit not in use", unit);
	return;
        }

    /* 
     * print statistics a last time and
     * stop statistics machine
     */
    parse_statistics (parse);
    TIMER_DEQUEUE (&parse -> stattimer);

    /* 
     * Tell the I/O module to turn us off.  We're history.
     */
    if (!parse -> pollonly)
	io_closeclock (&parse -> io);
    else
	(void)close (parse -> fd);

# 	ifndef STREAM
    parse_ioend (&parse -> parseio);
# 	endif	/* not STREAM */
    if (parse -> parse_type -> cl_end)
        {
	parse -> parse_type -> cl_end (parse);
        }

    syslog (LOG_INFO, "PARSE receiver #%d: reference clock \"%s\" removed",
	    CL_UNIT (parse -> unit), parse -> parse_type -> cl_description);

    parse -> peer = (struct peer   *)0;	/* unused now */
}


/*--------------------------------------------------
 * parse_start - open the PARSE devices and initialize data for processing
 */
static int
parse_start (sysunit, peer)
u_int   sysunit;
struct peer    *peer;
{
    u_int   unit;
    int     fd232,
            i;
    struct termios  tm;		/* NEEDED FOR A LONG TIME !
				   */
# 	ifdef STREAM
    struct strioctl     strioc;
# 	endif	/* STREAM */
    struct parseunit   *parse;
    parsectl_t  tmp_ctl;
    char    parsedev[sizeof (PARSEDEVICE)+20];
    u_int   type;
    static void     parse_receive P ((struct recvbuf   *rbufp));

    type = CL_TYPE (sysunit);
    unit = CL_UNIT (sysunit);

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: unit number invalid (max %d)",
		unit, MAXUNITS - 1);
	return 0;
        }

    if ((type == ~0) || (clockinfo[type].cl_description == (char   *)0))
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: unsupported clock type %d (max %d)",
		unit, CL_REALTYPE (sysunit), ncltypes - 1);
	return 0;
        }

    if (parseunits[unit] && parseunits[unit] -> peer)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: unit in use", unit);
	return 0;
        }

    /* 
     * Unit okay, attempt to open the device.
     */
    (void)sprintf (parsedev, PARSEDEVICE, unit);

# 	ifndef O_NOCTTY
# 		define O_NOCTTY 0
# 	endif	/* not O_NOCTTY */

    fd232 = open (parsedev, O_RDWR | O_NOCTTY, 0777);
    if (fd232 == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: open of %s failed: %m", unit, parsedev);
	return 0;
        }

    /* 
     * configure terminal line
     */
# 	if defined(SYSV_TTYS)
# 		ifndef HPUX
    if (ioctl (fd232, TCGETS, (caddr_t)&tm) == -1)
# 		else	/* ! not HPUX */
	if (tcgetattr (fd232, &tm) == -1)
# 		endif	/* not HPUX */
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: parse_start: ioctl(%d, TCGETS, &tm): %m", unit, fd232);
	    close (fd232);
	    return 0;
	    }
	else
	    {
	    bzero (tm.c_cc, sizeof (tm.c_cc));
				/* no specials - we'll do
				   all ourself */

	    tm.c_cc[VMIN] = 1;	/* get any character */

	    tm.c_cflag = clockinfo[type].cl_cflag;
	    tm.c_iflag = clockinfo[type].cl_iflag;
	    tm.c_oflag = clockinfo[type].cl_oflag;
	    tm.c_lflag = clockinfo[type].cl_lflag;

# 		ifndef HPUX
	    if (ioctl (fd232, TCSETS, (caddr_t)&tm) == -1)
# 		else	/* ! not HPUX */
		if (tcsetattr (fd232, TCSANOW, &tm) == -1)
# 		endif	/* not HPUX */
		    {
		    syslog (LOG_ERR, "PARSE receiver #%d: parse_start: ioctl(%d, TCSETS, &tm): %m", unit, fd232);
		    close (fd232);
		    return 0;
		    }
	    }

# 		ifdef I_POP
    /* 
     * pop all possibly active streams moduls from the tty
     */
    while (ioctl (fd232, I_POP, (caddr_t)0) == 0)
	 /* empty loop !! */ ;
# 		endif	/* I_POP */

# 		ifdef STREAM
    /* 
     * now push the parse streams module
     * it will ensure exclusive access to the device
     */
    if (ioctl (fd232, I_PUSH, (caddr_t)"parse") == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: ioctl(%d, I_PUSH, \"parse\"): %m",
		unit, fd232);
	close (fd232);
	return 0;
        }
# 		endif	/* STREAM */
# 	endif				/* SYSV_TTYS */

    /* 
     * Looks like this might succeed.  Find memory for the structure.
     * Look to see if there are any unused ones, if not we malloc()
     * one.
     */
    if (parseunits[unit])
        {
	parse = parseunits[unit];
				/* The one we want is okay
				   - and free */
        }
    else
        {
	for (i = 0; i < MAXUNITS; i++)
	    {
	    if (parseunits[i] && !parseunits[i] -> peer)
		break;
	    }
	if (i < MAXUNITS)
	    {
	    /* 
	     * Reclaim this one
	     */
	    parse = parseunits[i];
	    parseunits[i] = (struct parseunit  *)0;
	    }
	else
	    {
	    parse = (struct parseunit  *)
		emalloc (sizeof (struct parseunit));
	    }
        }

    bzero ((char   *)parse, sizeof (struct parseunit));
    parseunits[unit] = parse;

    /* 
     * Set up the structures
     */
    parse -> unit = (u_char)sysunit;
    parse -> timestarted = current_time;
    parse -> lastchange = current_time;
    /* 
     * we want to filter input for the sake of
     * getting an impression on dispersion
     * also we like to average the median range
     */
    parse -> flags = PARSE_STAT_FILTER | PARSE_STAT_AVG;
    parse -> pollneeddata = 0;
    parse -> pollonly = 1;	/* go for default polling
				   mode */
    parse -> lastformat = ~0;	/* assume no format known 
				*/
    parse -> status = CEVNT_TIMEOUT;	/* expect the worst */
    parse -> laststatus = ~0;	/* be sure to mark initial
				   status change */
    parse -> nosynctime = 0;	/* assume clock reasonable 
				*/
    parse -> lastmissed = 0;	/* assume got everything */
    parse -> localdata = (void *)0;
    parse -> parse_type = &clockinfo[type];

    parse -> basedelay.l_ui = 0;/* we can only
				   pre-configure delays
				   less than 1 second */
    parse -> basedelay.l_uf = parse -> parse_type -> cl_basedelay;

    /* We do not want to get signals on any packet from the
       streams module * It will keep one old packet and
       after this is read the next read * will return an
       accurate new packet to be processed. */
    parse -> fd = fd232;

# 	ifndef STREAM
    if (!parse_ioinit (&parse -> parseio))
        {
	(void)close (fd232);
	return 0;
        }
# 	endif	/* not STREAM */

    /* 
     * as we always(?) get 8 bit chars we want to be
     * sure, that the upper bits are zero for less
     * than 8 bit I/O - so we pass that information on.
     * note that there can be only one bit count format
     * per file descriptor
     */

    switch (tm.c_cflag & CSIZE)
        {
	case CS5:
	    tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS5;
	    break;

	case CS6:
	    tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS6;
	    break;

	case CS7:
	    tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS7;
	    break;

	case CS8:
	    tmp_ctl.parsesetcs.parse_cs = PARSE_IO_CS8;
	    break;
        }

# 	ifdef STREAM
    strioc.ic_cmd = PARSEIOC_SETCS;
    strioc.ic_timout = 0;
    strioc.ic_dp = (char   *)&tmp_ctl;
    strioc.ic_len = sizeof (tmp_ctl);

    if (ioctl (fd232, I_STR, (caddr_t)&strioc) == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: ioctl(%d, I_STR, PARSEIOC_SETCS): %m", unit, fd232);
	(void)close (fd232);
	return 0;
        }
# 	else	/* ! STREAM */
    if (!parse_setcs (&tmp_ctl, &parse -> parseio))
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: parse_setcs() FAILED.", unit);
	parse_ioend (&parse -> parseio);
	(void)close (fd232);
	return 0;
        }
# 	endif				/* STREAM */

    strcpy (tmp_ctl.parseformat.parse_buffer, parse -> parse_type -> cl_format);
    tmp_ctl.parseformat.parse_count = strlen (tmp_ctl.parseformat.parse_buffer);

# 	ifdef STREAM
    strioc.ic_cmd = PARSEIOC_SETFMT;
    strioc.ic_timout = 0;
    strioc.ic_dp = (char   *)&tmp_ctl;
    strioc.ic_len = sizeof (tmp_ctl);

    if (ioctl (fd232, I_STR, (caddr_t)&strioc) == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: ioctl(%d, I_STR, PARSEIOC_SETFMT): %m", unit, fd232);
	(void)close (fd232);
	return 0;
        }
# 	else	/* ! STREAM */
    if (!parse_setfmt (&tmp_ctl, &parse -> parseio))
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: parse_setfmt() FAILED.", unit);
	parse_ioend (&parse -> parseio);
	(void)close (fd232);
	return 0;
        }
# 	endif				/* STREAM */

    /* 
     * get rid of all IO accumulated so far
     */
        {
	int     flshcmd = TCIOFLUSH;

	(void)ioctl (fd232, TCFLSH, (caddr_t)&flshcmd);
        }

    tmp_ctl.parsestatus.flags = parse -> flags & PARSE_STAT_FLAGS;

# 	ifdef STREAM
    strioc.ic_cmd = PARSEIOC_SETSTAT;
    strioc.ic_timout = 0;
    strioc.ic_dp = (char   *)&tmp_ctl;
    strioc.ic_len = sizeof (tmp_ctl);

    if (ioctl (fd232, I_STR, (caddr_t)&strioc) == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: ioctl(%d, I_STR, PARSEIOC_SETSTAT): %m", unit, fd232);
	(void)close (fd232);
	return 0;
        }
# 	else	/* ! STREAM */
    if (!parse_setstat (&tmp_ctl, &parse -> parseio))
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_start: parse_setstat() FAILED.", unit);
	parse_ioend (&parse -> parseio);
	(void)close (fd232);
	return 0;
        }
# 	 endif /*   */

    /* 
     * All done.  Initialize a few random peer variables, then
     * return success.
     */
    peer -> rootdelay = parse -> parse_type -> cl_rootdelay;
    peer -> precision = parse -> parse_type -> cl_precision;
    peer -> stratum = STRATUM_REFCLOCK;
    if (peer -> stratum <= 1)
	bcopy (parse -> parse_type -> cl_id, (char *)&peer -> refid, 4);
    else
	peer -> refid = htonl (PARSEHSREFID);

    parse -> peer = peer;	/* marks it also as busy */

    /* 
     * try to do any special initializations
     */
    if (parse -> parse_type -> cl_init)
        {
	if (parse -> parse_type -> cl_init (parse))
	    {
	    parse_shutdown (parse -> unit);
				/* let our cleaning staff
				   do the work */
	    return 0;		/* well, ok - special
				   initialisation broke */
	    }
        }

    /* 
     * hot setup section
     * we need the asynch IO on non STREAMS operation and/or
     * when PPS is configured
     */
# 	if (defined(PPS) && defined(PARSEPPS)) || !defined(STREAM)
# 		ifdef STREAM
    if (CL_PPS (parse -> unit))
# 		endif				/* STREAM */
        {
	/* 
	 * Insert in device list.
	 */
	parse -> io.clock_recv = parse_receive;
	parse -> io.srcclock = (caddr_t)parse;
	parse -> io.datalen = 0;
	parse -> io.fd = parse -> fd;
				/* replicated, but what the
				   heck */
	if (!io_addclock (&parse -> io))
	    {
# 		ifdef STREAM
	    syslog (LOG_ERR,
		    "PARSE receiver #%d: parse_start: addclock %s fails (switching to polling mode)", CL_UNIT (parse -> unit), parsedev);
# 		else	/* ! STREAM */
	    syslog (LOG_ERR,
		    "PARSE receiver #%d: parse_start: addclock %s fails (ABORT)", CL_UNIT (parse -> unit), parsedev);
	    parse_shutdown (parse -> unit);
				/* let our cleaning staff
				   do the work */
	    return 0;
# 		endif				/* STREAM */
	    }
	else
	    {
	    parse -> pollonly = 0;  /* 
				 * update at receipt of time_stamp - also
				 * supports PPS processing
				 */
	    }
        }
# 	endif				/* (defined(PPS) &&
				   defined(PARSEPPS)) ||
				   !defined(STREAM) */

    /* 
     * wind up statistics timer
     */
    parse -> stattimer.peer = (struct peer *)parse;
				/* we know better, but what
				   the heck */
    parse -> stattimer.event_handler = cparse_statistics;
    parse -> stattimer.event_time = current_time + PARSESTATISTICS;
    TIMER_ENQUEUE (timerqueue, &parse -> stattimer);

    /* 
     * get out Copyright information
     */
    syslog (LOG_INFO, "PARSE receiver #%d: NTP PARSE support: Copyright (c) 1989-1993, Frank Kardel", CL_UNIT (parse -> unit));

    /* 
     * print out configuration
     */
    syslog (LOG_INFO, "PARSE receiver #%d: reference clock \"%s\" (device %s) added",
	    CL_UNIT (parse -> unit),
	    parse -> parse_type -> cl_description, parsedev);

    syslog (LOG_INFO, "PARSE receiver #%d:  Stratum %d, %sPPS support, trust time %s, precision %d",
	    CL_UNIT (parse -> unit),
	    parse -> peer -> stratum, parse -> pollonly ? "no " : "",
	    l_mktime (parse -> parse_type -> cl_maxunsync), parse -> peer -> precision);

    syslog (LOG_INFO, "PARSE receiver #%d:  rootdelay %s s, phaseadjust %s s, %s IO handling",
	    CL_UNIT (parse -> unit),
	    ufptoa (parse -> parse_type -> cl_rootdelay, 6),
	    lfptoa (&parse -> basedelay, 8),
# 	ifdef STREAM
	    "STREAM"
# 	else	/* ! STREAM */
	    "normal"
# 	endif	/* STREAM */
	);

    syslog (LOG_INFO, "PARSE receiver #%d:  Format recognition: %s", CL_UNIT (parse -> unit),
	    !(*parse -> parse_type -> cl_format) ? "<AUTOMATIC>" : parse -> parse_type -> cl_format);

    return 1;
}

/*--------------------------------------------------
 * event handling - note that nominal events will also be posted
 */

static void
parse_event (parse, event)
struct parseunit   *parse;
int     event;
{
    if (parse -> status != (u_char)event)
        {
	parse -> statetime[parse -> status] += current_time - parse -> lastchange;
	parse -> lastchange = current_time;

	parse -> status = (u_char)event;
	if (event != CEVNT_NOMINAL)
	    parse -> lastevent = parse -> status;

	report_event (EVNT_PEERCLOCK, parse -> peer);
        }
}

/*--------------------------------------------------
 * parse_receive - called by io handler
 *
 * this routine is called each time a time packed is
 * deliverd by the STREAMS module
 * if PPS processing is requested, pps_sample is called
 */
# 	if (defined(PPS) && defined(PARSEPPS)) || !defined(STREAM)
static void
parse_receive (rbufp)
struct recvbuf *rbufp;
{
    struct parseunit   *parse = (struct parseunit  *)rbufp -> recv_srcclock;
    parsetime_t     parsetime;
# 		ifndef STREAM
    register int    count;
    register char  *s;
# 		endif	/* not STREAM */

# 		ifdef STREAM
    if (rbufp -> recv_length != sizeof (parsetime_t))
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_receive: bad size (got %d expected %d)",
		CL_UNIT (parse -> unit), rbufp -> recv_length, sizeof (parsetime_t));
	parse -> baddata++;
	parse_event (parse, CEVNT_BADREPLY);
	return;
        }
    bcopy ((caddr_t)&rbufp -> recv_space, (caddr_t)&parsetime, sizeof (parsetime_t));

    parse_process (parse, &parsetime);
# 		else	/* ! STREAM */
    /* 
     * eat all characters, parsing then and feeding complete samples
     */
    count = rbufp -> recv_length;
    s = rbufp -> recv_buffer;

    while (count--)
        {
	int     cvtrtc;

	if (parse_ioread (&parse -> parseio, *s++, &rbufp -> recv_time))
	    {
	    /* 
	     * got something good to eat
	     */
	    parse_process (parse, &parse -> parseio.parse_dtime);
	    parse_iodone (&parse -> parseio);
	    }
        }
# 		endif				/* STREAM */
}
# 	endif				/* (defined(PPS) &&
				   defined(PARSEPPS)) ||
				   !defined(STREAM) */

/*--------------------------------------------------
 * parse_poll - called by the transmit procedure
 */
static void
parse_poll (unit, peer)
int     unit;
struct peer    *peer;
{
    extern int  errno;

    register int    fd,
                    i,
                    rtc;
    fd_set  fdmask;
    struct timeval  timeout,
                    starttime,
                    curtime,
                    selecttime;
    register struct parseunit  *parse;
    parsetime_t     parsetime;

    unit = CL_UNIT (unit);

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: poll: INTERNAL: unit invalid",
		unit);
	return;
        }

    parse = parseunits[unit];

    if (!parse -> peer)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: poll: INTERNAL: unit unused",
		unit);
	return;
        }

    if (peer != parse -> peer)
        {
	syslog (LOG_ERR,
		"PARSE receiver #%d: poll: INTERNAL: peer incorrect",
		unit);
	return;
        }

    /* 
     * Update clock stat counters
     */
    parse -> polls++;

# 	if (defined(PPS) && defined(PARSEPPS)) || !defined(STREAM)
    /* 
     * in PPS mode we just mark that we want the next sample
     * for the clock filter
     */
# 		ifdef STREAM
    if (!parse -> pollonly)
# 		endif	/* STREAM */
        {
	if (parse -> pollneeddata)
	    {
	    /* 
	     * bad news - didn't get a response last time
	     */
	    parse -> noresponse++;
	    parse -> lastmissed = current_time;
	    parse_event (parse, CEVNT_TIMEOUT);

	    syslog (LOG_WARNING, "PARSE receiver #%d: no data from device within poll interval", CL_UNIT (parse -> unit));
	    }
	parse -> pollneeddata = 1;
	if (parse -> parse_type -> cl_poll)
	    {
	    parse -> parse_type -> cl_poll (parse);
	    }
	return;
        }
# 	endif	/* defined (PPS) && defined(PARSEPPS)) || !defined(STREAM) */

# 	ifdef STREAM
    /* 
     * now we do the following:
     *    - read the first packet from the parse module  (OLD !!!)
     *    - read the second packet from the parse module (fresh)
     *    - compute values for xntp
     */

    FD_ZERO (&fdmask);
    fd = parse -> fd;
    FD_SET (fd, &fdmask);
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;	/* 0.5 sec */

    if (parse -> parse_type -> cl_poll)
        {
	parse -> parse_type -> cl_poll (parse);
        }

    if (gettimeofday (&starttime, 0L) == -1)
        {
	syslog (LOG_ERR, "gettimeofday failed: %m");
	exit (1);
        }

    selecttime = timeout;

    while ((rtc = select (fd + 1, &fdmask, 0, 0, &selecttime)) != 1)
        {
	/* no data from the radio clock */

	if (rtc == -1)
	    {
	    if (errno == EINTR)
	        {
		if (gettimeofday (&curtime, 0L) == -1)
		    {
		    syslog (LOG_ERR, "gettimeofday failed: %m");
		    exit (1);
		    }
		selecttime.tv_sec = curtime.tv_sec - starttime.tv_sec;
		if (curtime.tv_usec < starttime.tv_usec)
		    {
		    selecttime.tv_sec -= 1;
		    selecttime.tv_usec = 1000000 + curtime.tv_usec - starttime.tv_usec;
		    }
		else
		    {
		    selecttime.tv_usec = curtime.tv_usec - starttime.tv_usec;
		    }


		if (timercmp (&selecttime, &timeout, >))
		    {
		    /* 
		     * elapsed real time passed timeout value - consider it timed out
		     */
		    break;
		    }

		/* 
		 * calculate residual timeout value
		 */
		selecttime.tv_sec = timeout.tv_sec - selecttime.tv_sec;

		if (selecttime.tv_usec > timeout.tv_usec)
		    {
		    selecttime.tv_sec -= 1;
		    selecttime.tv_usec = 1000000 + timeout.tv_usec - selecttime.tv_usec;
		    }
		else
		    {
		    selecttime.tv_usec = timeout.tv_usec - selecttime.tv_usec;
		    }

		FD_SET (fd, &fdmask);
		continue;
	        }
	    else
	        {
		syslog (LOG_WARNING, "PARSE receiver #%d: no data[old] from device (select() error: %m)", unit);
	        }
	    }
	else
	    {
	    syslog (LOG_WARNING, "PARSE receiver #%d: no data[old] from device", unit);
	    }
	parse -> noresponse++;
	parse -> lastmissed = current_time;
	parse_event (parse, CEVNT_TIMEOUT);

	return;
        }

    while (((i = read (fd, &parsetime, sizeof (parsetime))) < sizeof (parsetime)))
        {
	/* bad packet */
	if (i == -1)
	    {
	    if (errno == EINTR)
	        {
		continue;
	        }
	    else
	        {
		syslog (LOG_WARNING, "PARSE receiver #%d: bad read[old] from streams module (read() error: %m)", unit, i, sizeof (parsetime));
	        }
	    }
	else
	    {
	    syslog (LOG_WARNING, "PARSE receiver #%d: bad read[old] from streams module (got %d bytes - expected %d bytes)", unit, i, sizeof (parsetime));
	    }
	parse -> baddata++;
	parse_event (parse, CEVNT_BADREPLY);

	return;
        }

    if (parse -> parse_type -> cl_poll)
        {
	parse -> parse_type -> cl_poll (parse);
        }

    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;	/* 1.500 sec */
    FD_ZERO (&fdmask);
    FD_SET (fd, &fdmask);

    if (gettimeofday (&starttime, 0L) == -1)
        {
	syslog (LOG_ERR, "gettimeofday failed: %m");
	exit (1);
        }

    selecttime = timeout;

    while ((rtc = select (fd + 1, &fdmask, 0, 0, &selecttime)) != 1)
        {
	/* no data from the radio clock */

	if (rtc == -1)
	    {
	    if (errno == EINTR)
	        {
		if (gettimeofday (&curtime, 0L) == -1)
		    {
		    syslog (LOG_ERR, "gettimeofday failed: %m");
		    exit (1);
		    }
		selecttime.tv_sec = curtime.tv_sec - starttime.tv_sec;
		if (curtime.tv_usec < starttime.tv_usec)
		    {
		    selecttime.tv_sec -= 1;
		    selecttime.tv_usec = 1000000 + curtime.tv_usec - starttime.tv_usec;
		    }
		else
		    {
		    selecttime.tv_usec = curtime.tv_usec - starttime.tv_usec;
		    }


		if (timercmp (&selecttime, &timeout, >))
		    {
		    /* 
		     * elapsed real time passed timeout value - consider it timed out
		     */
		    break;
		    }

		/* 
		 * calculate residual timeout value
		 */
		selecttime.tv_sec = timeout.tv_sec - selecttime.tv_sec;

		if (selecttime.tv_usec > timeout.tv_usec)
		    {
		    selecttime.tv_sec -= 1;
		    selecttime.tv_usec = 1000000 + timeout.tv_usec - selecttime.tv_usec;
		    }
		else
		    {
		    selecttime.tv_usec = timeout.tv_usec - selecttime.tv_usec;
		    }

		FD_SET (fd, &fdmask);
		continue;
	        }
	    else
	        {
		syslog (LOG_WARNING, "PARSE receiver #%d: no data[new] from device (select() error: %m)", unit);
	        }
	    }
	else
	    {
	    syslog (LOG_WARNING, "PARSE receiver #%d: no data[new] from device", unit);
	    }

	/* 
	 * we will return here iff we got a good old sample as this would
	 * be misinterpreted. bad samples are passed on to be logged into the
	 * state statistics
	 */
	if ((parsetime.parse_status & CVT_MASK) == CVT_OK)
	    {
	    parse -> noresponse++;
	    parse -> lastmissed = current_time;
	    parse_event (parse, CEVNT_TIMEOUT);
	    return;
	    }
        }

    /* 
     * we get here either by a possible read() (rtc == 1 - while assertion)
     * or by a timeout or a system call error. when a read() is possible we
     * get the new data, otherwise we stick with the old
     */
    if ((rtc == 1) && ((i = read (fd, &parsetime, sizeof (parsetime))) < sizeof (parsetime)))
        {
	/* bad packet */
	if (i == -1)
	    {
	    syslog (LOG_WARNING, "PARSE receiver #%d: bad read[new] from streams module (read() error: %m)", unit, i, sizeof (parsetime));
	    }
	else
	    {
	    syslog (LOG_WARNING, "PARSE receiver #%d: bad read[new] from streams module (got %d bytes - expected %d bytes)", unit, i, sizeof (parsetime));
	    }
	parse -> baddata++;
	parse_event (parse, CEVNT_BADREPLY);

	return;
        }

    /* 
     * process what we got
     */
    parse_process (parse, &parsetime);
# 	endif	/* STREAM */
}

/*--------------------------------------------------
 * process a PARSE time sample
 */
static void
parse_process (parse, parsetime)
struct parseunit   *parse;
parsetime_t    *parsetime;
{
    unsigned char   leap;
    struct timeval  usecdisp;
    l_fp    off,
            rectime,
            reftime,
            dispersion;

    /* 
     * check for changes in conversion status
     * (only one for each new status !)
     */
    if (parse -> laststatus != parsetime -> parse_status)
        {
	char    buffer[200];

	syslog (LOG_WARNING, "PARSE receiver #%d: conversion status \"%s\"",
		CL_UNIT (parse -> unit), parsestatus (parsetime -> parse_status, buffer));

	if ((parsetime -> parse_status & CVT_MASK) == CVT_FAIL)
	    {
	    /* 
	     * tell more about the story - list time code
	     * there is a slight change for a race condition and
	     * the time code might be overwritten by the next packet
	     */
	    parsectl_t  tmpctl;
# 	ifdef STREAM
	    struct strioctl     strioc;

	    strioc.ic_cmd = PARSEIOC_TIMECODE;
	    strioc.ic_timout = 0;
	    strioc.ic_dp = (char   *)&tmpctl;
	    strioc.ic_len = sizeof (tmpctl);

	    if (ioctl (parse -> fd, I_STR, (caddr_t)&strioc) == -1)
	        {
		syslog (LOG_ERR, "PARSE receiver #%d: parse_process: ioctl(%d, I_STR, PARSEIOC_TIMECODE): %m", CL_UNIT (parse -> unit), parse -> fd);
	        }
# 	else	/* ! STREAM */
	    if (!parse_timecode (&tmpctl, &parse -> parseio))
	        {
		syslog (LOG_ERR, "PARSE receiver #%d: parse_process: parse_timecode() FAILED", CL_UNIT (parse -> unit));
	        }
# 	endif		/* STREAM */
	    else
	        {
		syslog (LOG_WARNING, "PARSE receiver #%d: FAILED TIMECODE: \"%s\"",
			CL_UNIT (parse -> unit), mkascii (buffer, sizeof buffer, tmpctl.parsegettc.parse_buffer, tmpctl.parsegettc.parse_count - 1));
		parse -> badformat += tmpctl.parsegettc.parse_badformat;
	        }
	    }

	parse -> laststatus = parsetime -> parse_status;
        }

    /* 
     * examine status and post appropriate events
     */
    if ((parsetime -> parse_status & CVT_MASK) != CVT_OK)
        {
	/* 
	 * got bad data - tell the rest of the system
	 */
	switch (parsetime -> parse_status & CVT_MASK)
	    {
	    case CVT_NONE:
		break;		/* well, still waiting -
				   timeout is handled at
				   higher levels */

	    case CVT_FAIL:
		parse -> badformat++;
		if (parsetime -> parse_status & CVT_BADFMT)
		    {
		    parse_event (parse, CEVNT_BADREPLY);
		    }
		else
		if (parsetime -> parse_status & CVT_BADDATE)
		    {
		    parse_event (parse, CEVNT_BADDATE);
		    }
		else
		if (parsetime -> parse_status & CVT_BADTIME)
		    {
		    parse_event (parse, CEVNT_BADTIME);
		    }
		else
		    {
		    parse_event (parse, CEVNT_BADREPLY);
				/* for the lack of
				   something better */
		    }
	    }
	return;			/* skip the rest - useless 
				*/
        }

    /* 
     * check for format changes
     * (in case somebody has swapped clocks 8-)
     */
    if (parse -> lastformat != parsetime -> parse_format)
        {
	parsectl_t  tmpctl;
# 	ifdef STREAM
	struct strioctl     strioc;

	strioc.ic_cmd = PARSEIOC_GETFMT;
	strioc.ic_timout = 0;
	strioc.ic_dp = (char   *)&tmpctl;
	strioc.ic_len = sizeof (tmpctl);
# 	endif	/* STREAM */

	tmpctl.parseformat.parse_format = parsetime -> parse_format;

# 	ifdef STREAM
	if (ioctl (parse -> fd, I_STR, (caddr_t)&strioc) == -1)
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: ioctl(%d, I_STR, PARSEIOC_GETFMT): %m", CL_UNIT (parse -> unit), parse -> fd);
	    }
# 	else	/* ! STREAM */
	if (!parse_getfmt (&tmpctl, &parse -> parseio))
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: parse_getfmt() FAILED", CL_UNIT (parse -> unit));
	    }
# 	endif	/* STREAM */
	else
	    {
	    syslog (LOG_INFO, "PARSE receiver #%d: new packet format \"%s\"",
		    CL_UNIT (parse -> unit), tmpctl.parseformat.parse_buffer);
	    }
	parse -> lastformat = parsetime -> parse_format;
        }

    /* 
     * now, any changes ?
     */
    if (parse -> time.parse_state != parsetime -> parse_state)
        {
	char    tmp1[200];
	char    tmp2[200];
	/* 
	 * something happend
	 */

	(void)parsestate (parsetime -> parse_state, tmp1);
	(void)parsestate (parse -> time.parse_state, tmp2);

	syslog (LOG_INFO, "PARSE receiver #%d: STATE CHANGE: %s -> %s",
		CL_UNIT (parse -> unit), tmp2, tmp1);
        }

    /* 
     * remember for future
     */
    parse -> time = *parsetime;

    /* 
     * check to see, whether the clock did a complete powerup or lost PZF signal
     * and post correct events for current condition
     */
    if (PARSE_POWERUP (parsetime -> parse_state))
        {
	/* 
	 * this is bad, as we have completely lost synchronisation
	 * well this is a problem with the receiver here
	 * for PARSE U/A 31 the lost synchronisation ist true
	 * as it is the powerup state and the time is taken
	 * from a crude real time clock chip
	 * for the PZF series this is only partly true, as
	 * PARSE_POWERUP only means that the pseudo random
	 * phase shift sequence cannot be found. this is only
	 * bad, if we have never seen the clock in the SYNC
	 * state, where the PHASE and EPOCH are correct.
	 * for reporting events the above business does not
	 * really matter, but we can use the time code
	 * even in the POWERUP state after having seen
	 * the clock in the synchronized state (PZF class
	 * receivers) unless we have had a telegram disruption
	 * after having seen the clock in the SYNC state. we
	 * thus require having seen the clock in SYNC state
	 * *after* having missed telegrams (noresponse) from
	 * the clock. one problem remains: we might use erroneously
	 * POWERUP data if the disruption is shorter than 1 polling
	 * interval. fortunately powerdowns last usually longer than 64
	 * seconds and the receiver is at least 2 minutes in the
	 * POWERUP or NOSYNC state before switching to SYNC
	 */
	parse_event (parse, CEVNT_FAULT);
	if (parse -> nosynctime)
	    {
	    /* 
	     * repeated POWERUP/NOSYNC state - look whether
	     * the message should be repeated
	     */
	    if (current_time - parse -> nosynctime > PARSENOSYNCREPEAT)
	        {
		syslog (LOG_ERR, "PARSE receiver #%d: *STILL* NOT SYNCHRONIZED (POWERUP or no PZF signal)",
			CL_UNIT (parse -> unit));
		parse -> nosynctime = current_time;
	        }
	    }
	else
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: NOT SYNCHRONIZED (POWERUP or no PZF signal)",
		    CL_UNIT (parse -> unit));
	    parse -> nosynctime = current_time;
	    }
        }
    else
        {
	/* 
	 * we have two states left
	 *
	 * SYNC:
	 *  this state means that the EPOCH (timecode) and PHASE
	 *  information has be read correctly (at least two
	 *  successive PARSE timecodes were received correctly)
	 *  this is the best possible state - full trust
	 *
	 * NOSYNC:
	 *  The clock should be on phase with respect to the second
	 *  signal, but the timecode has not been received correctly within
	 *  at least the last two minutes. this is a sort of half baked state
	 *  for PARSE U/A 31 this is bad news (clock running without timecode
	 *  confirmation)
	 *  PZF 535 has also no time confirmation, but the phase should be
	 *  very precise as the PZF signal can be decoded
	 */
	parse -> nosynctime = 0;/* current state is better
				   than worst state */

	if (PARSE_SYNC (parsetime -> parse_state))
	    {
	    /* 
	     * currently completely synchronized - best possible state
	     */
	    parse -> lastsync = current_time;
	    /* 
	     * log OK status
	     */
	    parse_event (parse, CEVNT_NOMINAL);
	    }
	else
	    {
	    /* 
	     * we have had some problems receiving the time code
	     */
	    parse_event (parse, CEVNT_PROP);
	    }
        }

    if (PARSE_PPS (parsetime -> parse_state) && CL_PPS (parse -> unit))
        {
	l_fp    offset;

	/* 
	 * we have a PPS signal - much better than the RS232 stuff
	 */
	if (!buftvtots ((char  *)&parsetime -> parse_ptime, &offset))
	    return;

	if (PARSE_TIMECODE (parsetime -> parse_state))
	    {
	    if (buftvtots ((char   *)&parsetime -> parse_time, &off))
	        {
		/* 
		 * WARNING: assumes on TIMECODE == PULSE (timecode after pulse)
		 */
		rectime = reftime = off;
				/* take reference time -
				   fake rectime */
		L_SUB (&off, &offset);	/* true offset */
	        }
	    else
	        {
		/* 
		 * assumes on second pulse
		 */
		reftime = offset;   /* we don't have anything
				   better */
		rectime = offset;
				/* to pass sanity checks */
		off = offset;
		off.l_ui = 0;
		M_NEG (off.l_ui, off.l_f);
				/* implied on second offset
				   */
	        }
	    }
	else
	    {
	    /* 
	     * assumes on second pulse
	     */
	    reftime = off = offset;
	    rectime = offset;
	    off.l_ui = 0;
	    M_NEG (off.l_ui, off.l_f);
				/* implied on second offset
				   */
	    }
        }
    else
    if (PARSE_TIMECODE (parsetime -> parse_state))
        {
	l_fp    ts;
	l_fp    offset;

	/* 
	 * calculate time offset including systematic delays
	 * off = PARSE-timestamp + propagation delay - kernel time stamp
	 */
	offset = parse -> basedelay;

	if (!buftvtots ((char  *)&parsetime -> parse_time, &off))
	    return;

    reftime = off;

	L_ADD (&off, &offset);
	rectime = off;		/* this makes org time and
				   xmt time somewhat
				   artificial */

	if (parse -> flags & PARSE_STAT_FILTER)
	    {
	    struct timeval  usecerror;
	    /* 
	     * offset is already calculated
	     */
	    usecerror.tv_sec = parsetime -> parse_usecerror / 1000000;
	    usecerror.tv_usec = parsetime -> parse_usecerror % 1000000;

	    sTVTOTS (&usecerror, &off);
	    L_ADD (&off, &offset);
	    }
	else
	    {
# 	ifdef STREAM
	    if (!buftvtots ((char  *)&parsetime -> parse_stime, &ts))
		return;
	L_SUB (&off, &ts);
# 	else	/* ! STREAM */
	    L_SUB (&off, &parsetime -> parse_stime);
# 	endif	/* STREAM */
	    }
        }
    else
        {
	/* 
	 * Well, no PPS, no TIMECODE, no work ...
	 */
	return;
        }


# 	if defined(PPS) && defined(PARSEPPS)
    if (CL_PPS (parse -> unit) && !parse -> pollonly && PARSE_SYNC (parsetime -> parse_state))
        {
	/* 
	 * only provide PPS information when clock
	 * is in sync
	 * thus PHASE and EPOCH are correct
	 */
	(void)pps_sample (&off);
        }
# 	endif	/* defined (PPS) && defined(PARSEPPS) */

    /* 
     * ready, unless the machine wants a sample
     */
    if (!parse -> pollonly && !parse -> pollneeddata)
	return;

    parse -> pollneeddata = 0;

    if (PARSE_PPS (parsetime -> parse_state))
        {
	L_CLR (&dispersion);
        }
    else
        {
	/* 
	 * convert usec dispersion into NTP TS world
	 */

	usecdisp.tv_sec = parsetime -> parse_usecdisp / 1000000;
	usecdisp.tv_usec = parsetime -> parse_usecdisp % 1000000;

	TVTOTS (&usecdisp, &dispersion);
        }

    /* 
     * and now stick it into the clock machine
     * samples are only valid iff lastsync is not too old and
     * we have seen the clock in sync at least once
     * after the last time we didn't see an expected data telegram
     * see the clock states section above for more reasoning
     */
    if (((current_time - parse -> lastsync) > parse -> parse_type -> cl_maxunsync) ||
	    (parse -> lastsync <= parse -> lastmissed))
        {
	leap = LEAP_NOTINSYNC;
        }
    else
        {
	if (PARSE_LEAP (parsetime -> parse_state))
	    {
	    leap = (parse -> flags & PARSE_LEAP_DELETE) ? LEAP_DELSECOND : LEAP_ADDSECOND;
	    }
	else
	    {
	    leap = LEAP_NOWARNING;
	    }
        }

    refclock_receive (parse -> peer, &off, 0, LFPTOFP (&dispersion), &reftime, &rectime, leap);

}

/*--------------------------------------------------
 * parse_leap - called when a leap second occurs
 */

static void
parse_leap ()
{
    /* 
     * PARSE does encode a leap warning... we are aware but not afraid of that
     * as long as we get a little help for the direction from the operator until
     * PARSE encodes the LEAP correction direction.
     */
}


/*--------------------------------------------------
 * parse_control - set fudge factors, return statistics
 */
static void
parse_control (unit, in, out)
u_int   unit;
struct refclockstat    *in;
struct refclockstat    *out;
{
    register struct parseunit  *parse;
    parsectl_t  tmpctl;
# 	ifdef STREAM
    struct strioctl     strioc;
# 	endif	/* STREAM */
    unsigned    LONG type;
    static char     outstatus[400];
				/* status output buffer */

    type = CL_TYPE (unit);
    unit = CL_UNIT (unit);

    if (out)
        {
	out -> lencode = 0;
	out -> lastcode = 0;
	out -> polls = out -> noresponse = 0;
	out -> badformat = out -> baddata = 0;
	out -> timereset = 0;
	out -> currentstatus = out -> lastevent = CEVNT_NOMINAL;
        }

    if (unit >= MAXUNITS)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_control: unit invalid (max %d)",
		unit, MAXUNITS - 1);
	return;
        }

    parse = parseunits[unit];

    if (!parse || !parse -> peer)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: parse_control: unit invalid (UNIT INACTIVE)",
		unit);
	return;
        }

    if (in)
        {
	if (in -> haveflags & CLK_HAVETIME1)
	    parse -> basedelay = in -> fudgetime1;

	if (in -> haveflags & CLK_HAVETIME2)
	    {
	    /* not USED */
	    }

	if (in -> haveflags & CLK_HAVEVAL1)
	    {
	    parse -> peer -> stratum = (u_char)(in -> fudgeval1 & 0xf);
	    if (parse -> peer -> stratum <= 1)
		bcopy (parse -> parse_type -> cl_id, (char *)&parse -> peer -> refid, 4);
	    else
		parse -> peer -> refid = htonl (PARSEHSREFID);
	    }

	/* 
	 * NOT USED - yet
	 *
	 if (in->haveflags & CLK_HAVEVAL2)
	 {
	 }
	 */
	if (in -> haveflags & (CLK_HAVEFLAG1 | CLK_HAVEFLAG2 | CLK_HAVEFLAG3 | CLK_HAVEFLAG4))
	    {
	    parse -> flags = in -> flags & (CLK_FLAG1 | CLK_FLAG2 | CLK_FLAG3 | CLK_FLAG4);
	    }

	if (in -> haveflags & (CLK_HAVEVAL2 | CLK_HAVETIME2 | CLK_HAVEFLAG1 | CLK_HAVEFLAG2 | CLK_HAVEFLAG3 | CLK_HAVEFLAG4))
	    {
	    parsectl_t  tmpctl;
	    tmpctl.parsestatus.flags = parse -> flags & PARSE_STAT_FLAGS;

# 	ifdef STREAM
	    /* 
	     * now configure the streams module
	     */
	    strioc.ic_cmd = PARSEIOC_SETSTAT;
	    strioc.ic_timout = 0;
	    strioc.ic_dp = (char   *)&tmpctl;
	    strioc.ic_len = sizeof tmpctl;

	    if (ioctl (parseunits[unit] -> fd, I_STR, (caddr_t)&strioc) == -1)
	        {
		syslog (LOG_ERR, "PARSE receiver #%d: parse_control: ioctl(%d, I_STR, PARSEIOC_SETSTAT): %m", unit, parseunits[unit] -> fd);
	        }
# 	else	/* ! STREAM */
	    if (!parse_setstat (&tmpctl, &parse -> parseio))
	        {
		syslog (LOG_ERR, "PARSE receiver #%d: parse_control: parse_setstat() FAILED", unit);
	        }
# 	endif	/* STREAM */
	    }
        }

    if (out)
        {
	register unsigned   LONG sum = 0;
	register char  *t;
	register struct tm     *tm;
	register short  utcoff;
	register char   sign;
	register int    i;

	out -> haveflags = CLK_HAVETIME1 | CLK_HAVEVAL1 | CLK_HAVEFLAG1 | CLK_HAVEFLAG2 | CLK_HAVEFLAG3;
	out -> clockdesc = parse -> parse_type -> cl_description;

	out -> fudgetime1 = parse -> basedelay;

	out -> fudgeval1 = (LONG)parse -> peer -> stratum;

	out -> flags = parse -> flags;

	out -> type = REFCLK_PARSE;

	/* 
	 * all this for just finding out the +-xxxx part (there are always
	 * new and changing fields in the standards 8-().
	 *
	 * but we do it for the human user...
	 */
	tm = gmtime (&parse -> time.parse_time.tv_sec);
	utcoff = tm -> tm_hour * 60 + tm -> tm_min;
	tm = localtime (&parse -> time.parse_time.tv_sec);
	utcoff = tm -> tm_hour * 60 + tm -> tm_min - utcoff + 12 * 60;
	utcoff += 24 * 60;
	utcoff %= 24 * 60;
	utcoff -= 12 * 60;
	if (utcoff < 0)
	    {
	    utcoff = -utcoff;
	    sign = '-';
	    }
	else
	    {
	    sign = '+';
	    }

	strcpy (outstatus, ctime (&parse -> time.parse_time.tv_sec));
	t = strrchr (outstatus, '\n');
	if (!t)
	    {
	    t = outstatus + strlen (outstatus);
	    }
	else
	    {
	    sprintf (t, " %c%02d%02d", sign, utcoff / 60, utcoff % 60);
	    t += strlen (t);
	    }

# 	ifdef STREAM
	strioc.ic_cmd = PARSEIOC_TIMECODE;
	strioc.ic_timout = 0;
	strioc.ic_dp = (char   *)&tmpctl;
	strioc.ic_len = sizeof (tmpctl);

	if (ioctl (parseunits[unit] -> fd, I_STR, (caddr_t)&strioc) == -1)
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: parse_control: ioctl(%d, I_STR, PARSEIOC_TIMECODE): %m", unit, parseunits[unit] -> fd);
	    }
# 	else	/* ! STREAM */
	if (!parse_timecode (&tmpctl, &parse -> parseio))
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: parse_control: parse_timecode() FAILED", unit);
	    }
# 	endif		/* STREAM */
	else
	    {
	    if (t)
	        {
		*t = ' ';
		(void)parsestate (tmpctl.parsegettc.parse_state, t + 1);
	        }
	    else
	        {
		strcat (outstatus, " ");
		(void)parsestate (tmpctl.parsegettc.parse_state, outstatus + strlen (outstatus));
	        }
	    strcat (outstatus, " <");
	    if (tmpctl.parsegettc.parse_count)
		mkascii (outstatus + strlen (outstatus), sizeof (outstatus)-strlen (outstatus) - 1,
			tmpctl.parsegettc.parse_buffer, tmpctl.parsegettc.parse_count - 1);
	    strcat (outstatus, ">");
	    parse -> badformat += tmpctl.parsegettc.parse_badformat;
	    }

	tmpctl.parseformat.parse_format = tmpctl.parsegettc.parse_format;

# 	ifdef STREAM
	strioc.ic_cmd = PARSEIOC_GETFMT;

	if (ioctl (parseunits[unit] -> fd, I_STR, (caddr_t)&strioc) == -1)
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: parse_control: ioctl(%d, I_STR, PARSEIOC_GETFMT): %m", unit, parseunits[unit] -> fd);
	    }
# 	else	/* ! STREAM */
	if (!parse_getfmt (&tmpctl, &parse -> parseio))
	    {
	    syslog (LOG_ERR, "PARSE receiver #%d: parse_control: parse_getfmt() FAILED", unit);
	    }
# 	endif	/* STREAM */
	else
	    {
	    strcat (outstatus, " (");
	    strncat (outstatus, tmpctl.parseformat.parse_buffer, tmpctl.parseformat.parse_count);
	    strcat (outstatus, ")");
	    }

	/* 
	 * gather state statistics
	 */

	t = outstatus + strlen (outstatus);

	for (i = 0; i <= CEVNT_MAX; i++)
	    {
	    register unsigned   LONG stime;
	    register unsigned   LONG div = current_time - parse -> timestarted;
	    register unsigned   LONG percent;

	    percent = stime = PARSE_STATETIME (parse, i);

	    while (((unsigned   LONG) (~0) / 10000) < percent)
	        {
		percent /= 10;
		div /= 10;
	        }

	    if (div)
		percent = (percent * 10000) / div;
	    else
		percent = 10000;

	    if (stime)
	        {
		sprintf (t, "%s%s%s: %s (%d.%02d%%)",
			sum ? "; " : " [",
			(parse -> status == i) ? "*" : "",
			clockstatus (i),
			l_mktime (stime),
			percent / 100, percent % 100);
		sum += stime;
		t += strlen (t);
	        }
	    }

	sprintf (t, "; running time: %s]", l_mktime (sum));

	out -> lencode = strlen (outstatus);
	out -> lastcode = outstatus;
	out -> timereset = parse -> timestarted;
	out -> polls = parse -> polls;
	out -> noresponse = parse -> noresponse;
	out -> badformat = parse -> badformat;
	out -> baddata = parse -> baddata;
	out -> lastevent = parse -> lastevent;
	out -> currentstatus = parse -> status;
        }
}

/*-------------------- special support --------------------*/

/*
 * Schmid clock polling support
 */

# 	define WS_POLLRATE	1	/* every second - watch
				   interdependency with
				   poll routine */

struct ws_poll
{
    struct event    timer;	/* we'd like to poll a a
				   higher rate than 1/64s 
				*/
};

typedef struct ws_poll  ws_poll_t;

static char     ws_cmd[] =
{
    0x73
};

/*--------------------------------------------------
 * direct poll routine (output a 0x73 on a poll event)
 */
static void
wsparse_dpoll (parse)
struct parseunit   *parse;
{
    register int    rtc;

    rtc = write (parse -> fd, ws_cmd, sizeof ws_cmd);
    if (rtc < 0)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: wsparse_poll: failed to send cmd to clock: %m", CL_UNIT (parse -> unit));
        }
    else
    if (rtc != sizeof ws_cmd)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: wsparse_poll: failed to send cmd incomplete (%d of %d bytes sent)", CL_UNIT (parse -> unit), rtc, sizeof ws_cmd);
        }
}

/*--------------------------------------------------
 * poll routine (output a 0x73 on a poll event - and reload timer)
 */
static void
wsparse_poll (parse)
struct parseunit   *parse;
{
    register ws_poll_t *wsp = (ws_poll_t *)parse -> localdata;

    wsparse_dpoll (parse);

    if (wsp != (ws_poll_t *)0)
        {
	wsp -> timer.event_time = current_time + WS_POLLRATE;
	TIMER_ENQUEUE (timerqueue, &wsp -> timer);
        }
}

/*--------------------------------------------------
 * init routine - setup timer
 */
static int
wsparse_init (parse)
struct parseunit   *parse;
{
    register ws_poll_t *wsp;

    parse -> localdata = (void *)malloc (sizeof (ws_poll_t));
    bzero ((char   *)parse -> localdata, sizeof (ws_poll_t));

    wsp = (ws_poll_t *)parse -> localdata;

    wsp -> timer.peer = (struct peer   *)parse;
				/* well, only we know what
				   it is */
    wsp -> timer.event_handler = wsparse_poll;
    wsparse_poll (parse);

    return 0;
}

/*--------------------------------------------------
 * end routine - clean up timer
 */
static void
wsparse_end (parse)
struct parseunit   *parse;
{
    if (parse -> localdata != (void    *)0)
        {
	TIMER_DEQUEUE (&((ws_poll_t *)parse -> localdata) -> timer);
	free ((char    *)parse -> localdata);
	parse -> localdata = (void *)0;
        }
}

/*
 * RAW DCF support
 */
static int  rawdcf_init (parse)
struct parseunit   *parse;
{
# 	if defined(TIOCMBIS) && defined(TIOCMBIC) && defined(TIOCM_RTS) && defined(TIOCM_DTR)
    int     c;

    c = TIOCM_RTS;
    if (ioctl (parse -> fd, TIOCMBIC, &c) == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: rawdcf_init: could not clear RTS (%m) - no power for receiver", CL_UNIT (parse -> unit));
        }
    c = TIOCM_DTR;
    if (ioctl (parse -> fd, TIOCMBIS, &c) == -1)
        {
	syslog (LOG_ERR, "PARSE receiver #%d: rawdcf_init: could not set DTR (%m) - no power for receiver", CL_UNIT (parse -> unit));
        }
# 	endif	/* defined (TIOCMBIS) && defined(TIOCMBIC) && defined(TIOCM_RTS) && defined(TIOCM_DTR) */
    /* 
     * we assume we do not have to supply power, as somebody can use just a battery
     */
    syslog (LOG_INFO, "PARSE receiver #%d: NTP DCF77 True Time Code support: Copyright (c) 1993, Frank Kardel", CL_UNIT (parse -> unit));
    return 0;
}
# endif				/* defined(REFCLOCK) &&
				   defined(PARSE) */
