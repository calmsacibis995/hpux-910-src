/* 	@(#)bsdterm.h 	$Revision: 1.6.109.1 $	$Date: 91/11/19 14:42:15 $  */

/*
 * BSD Speeds for the sgtty.sg_ispeed and sg_ospeed fields
 */
#define BSD_B0		0
#define BSD_B50		1
#define BSD_B75		2
#define BSD_B110	3
#define BSD_B134	4
#define BSD_B150	5
#define BSD_B200	6
#define BSD_B300	7
#define BSD_B600	8
#define BSD_B1200	9
#define	BSD_B1800	10
#define BSD_B2400	11
#define BSD_B4800	12
#define BSD_B9600	13
#define BSD_B19200	14
#define BSD_B38400	15
#define BSD_EXTA	14
#define BSD_EXTB	15

/*
 * BSD flag values for the sgtty.sg_flags field.
 */
#define BSD_TANDEM	0000001		/* Automatic flow control (ignored) */
#define BSD_CBREAK	0000002		/* NO input processing (half-raw) */
#define BSD_LCASE	0000004		/* Map upper case to lower case */
#define BSD_ECHO	0000010		/* Full Duplex mode */
#define BSD_CRMOD	0000020		/* Map CR to LF; output LF as CR-LF */
#define BSD_RAW		0000040		/* NO input or output processing */
#define BSD_ODDP	0000100		/* Even or Odd parity, or none at all*/
#define BSD_EVENP	0000200
#define BSD_NLDELAY	0001400 	/* New line delays */
#define BSD_NL0		0000000
#define BSD_NL1		0000400
#define BSD_NL2		0001000
#define BSD_NL3		0001400
#define BSD_TBDELAY	0006000		/* Tab delays */
#define BSD_TAB0	0000000
#define BSD_TAB1	0002000
#define BSD_TAB2	0004000
#define BSD_XTABS	0006000
#define BSD_CRDELAY	0030000		/* Carriage return delays */
#define BSD_CR0		0000000
#define BSD_CR1		0010000
#define BSD_CR2		0020000
#define BSD_CR3		0030000
#define BSD_VTDELAY	0040000		/* Line feed, vertical tab delay */
#define BSD_FF0		0000000
#define BSD_FF1		0040000
#define BSD_BSDELAY	0100000		/* Back space delays */
#define BSD_BS0		0000000
#define BSD_BS1		0100000
#define BSD_ALLDELAY	0177400		/* Delay algorithm selection */

/*
 * Flags associated with the local mode word on BSD
 */
#define	BSD_LCRTBS	000001
#define	BSD_LPRTERA	000002
#define	BSD_LCRTERA	000004
#define	BSD_LTILDE	000010
#define	BSD_LMDMBUF	000020
#define	BSD_LLITOUT	000040
#define	BSD_LTOSTOP	000100
#define	BSD_LFLUSHO	000200
#define	BSD_LNOHANG	000400
#define	BSD_LCRTKIL	002000
#define	BSD_LPASS8	004000
#define	BSD_LCTLECH	010000
#define	BSD_LPENDIN	020000
#define	BSD_LDECCTQ	040000
#define	BSD_LNOFLSH	0100000

/*
 * Possible line disciplines drivers.  
 */
#define BSD_OTTYDISC	0	/* Old style, unsupported */
#define BSD_NETLDISC	1	/* line discip for berkeley networking */
#define BSD_NTTYDISC	2	/* New style */

/*
 * Define the tty chars that are not defined on HP/Sys V systems.
 *
 * NOTE: On systems which support Job Control, ltchars will already be 
 * defined, but some fields MUST be set to -1 (reserved or not in use).
 */

struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};

#ifndef SIGTSTP
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};
#else
#include <sys/bsdtty.h>
#endif SIGTSTP


/*
 * Define the ioctl calls that are not available on this system.
 * Need to be somewhat careful about conflicting with current ioctl defines.
 * Therefore, use 'B' as the character differentiator, since nothing else
 * currently uses that.  Note that some ioctl's are actually already
 * defined for HP systems.  Namely, TIOC[GS]ETP for all systems, and the
 * TIO[GS]LTC for those supporting job control (e.g. the 800).  TIOCSETN
 * is essentially TIOCSETP, with no flushing of the buffers.  Since this
 * is not possible to do from user space, emulate it with TIOCSETP.
 */

#define TIOCSETN TIOCSETP
#define TIOCGETC _IOR('B', 1, struct tchars)	/* Get special chars */
#define TIOCSETC _IOW('B', 2, struct tchars)
#ifndef SIGTSTP
#define TIOCGLTC _IOR('B', 3, struct ltchars)	/* Get local special chars */
#define TIOCSLTC _IOW('B', 4, struct ltchars)
#define TIOCLGET _IOR('B', 5, int)		/* Get local mode word */
#define TIOCLSET _IOW('B', 6, int)
#define TIOCLBIS _IOW('B', 7, int)
#define TIOCLBIC _IOW('B', 8, int)
#endif SIGTSTP
#define TIOCGETD _IOR('B', 9, int)	/* Get line discipline */
#define TIOCSETD _IOR('B', 10, int)
