/* $Header: termio.h,v 1.27.83.4 93/09/17 18:36:05 kcs Exp $ */
#ifndef _SYS_TERMIO_INCLUDED
#define _SYS_TERMIO_INCLUDED

#ifdef	_KERNEL_BUILD
#include "../h/stdsyms.h"
#define _INCLUDE_TERMIO
#else /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif	/* _KERNEL_BUILD */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifndef NCCS
#    define NCCS 16
#  endif /* NCCS */

   typedef unsigned int tcflag_t;
   typedef unsigned char cc_t;
   typedef unsigned int speed_t;

#  ifdef _TERMIOS_INCLUDED
     struct termios {
	tcflag_t	c_iflag;	/* Input modes */
	tcflag_t	c_oflag;	/* Output modes */
	tcflag_t	c_cflag;	/* Control modes */
	tcflag_t	c_lflag; 	/* Local modes */
	tcflag_t	c_reserved;	/* Reserved for future use */
	cc_t		c_cc[NCCS];		/* Control characters */
     };
#  else  /* not _TERMIOS_INCLUDED */
#  define _INCLUDE_TERMIO
#  endif /* _TERMIOS_INCLUDED */

/* Input Modes */
#  define IGNBRK	0000001		/* Ignore break condition */
#  define BRKINT	0000002		/* Signal interrupt on break */
#  define IGNPAR	0000004		/*Ignore characters with parity errors*/
#  define PARMRK	0000010		/* Mark parity errors */
#  define INPCK		0000020		/* Enable input parity check */
#  define ISTRIP	0000040		/* Strip character */
#  define INLCR		0000100		/* Map NL to CR on input */
#  define IGNCR		0000200		/* Ignore CR */
#  define ICRNL		0000400		/* Map CR to NL on input */
#  define _IUCLC	0001000		/* Map upper case to lower case 
					   input added to ansic */	
#  define IXON		0002000		/* Enable start/stop output control */
#  define _IXANY   	0004000		/* enable any character to restart 
					   output */	
#  define IXOFF		0010000		/* Enable start/stop input control */
#  define _IENQAK	0020000

/* Output Modes */
#  define OPOST		0000001		/* Perform output processing */
#  define _OLCUC	0000002
#  define _ONLCR	0000004
#  define _OCRNL	0000010
#  define _ONOCR	0000020
#  define _ONLRET	0000040
#  define _OFILL	0000100
#  define _OFDEL	0000200
#  define _NLDLY	0000400
#  define _NL0		0
#  define _NL1		0000400
#  define _CRDLY	0003000
#  define _CR0		0
#  define _CR1		0001000
#  define _CR2		0002000
#  define _CR3		0003000
#  define _TABDLY	0014000
#  define _TAB0		0
#  define _TAB1		0004000
#  define _TAB2		0010000
#  define _TAB3		0014000
#  define _BSDLY	0020000
#  define _BS0		0
#  define _BS1		0020000
#  define _VTDLY	0040000
#  define _VT0		0
#  define _VT1		0040000
#  define _FFDLY	0100000
#  define _FF0		0
#  define _FF1		0100000

/* Control Modes */
#  define _EXTA		0000036
#  define _EXTB		0000037
#  define CLOCAL	0010000		/* Ignore modem status lines */
#  define CREAD		0000400		/* Enable receiver */
#  define CSIZE		0000140		/* Number of bits per byte */
#  define CS5		0		/* 5 bits */
#  define CS6		0000040		/* 6 bits */
#  define CS7		0000100		/* 7 bits */
#  define CS8		0000140		/* 8 bits */
#  define CSTOPB	0000200		/* Send two stop bits, else one */
#  define HUPCL		0004000		/* Hang up on last close */
#  define PARENB	0001000		/* Parity enable */
#  define PARODD	0002000		/* Odd parity, else even */
#  define _LOBLK	0020000

/* Local Modes */
#  define ISIG		0000001		/* Enable signals */
#  define ICANON	0000002		/* Canonical input (erase and kill
					   or suspend */
#  define _XCASE	0000004		/* Canonical upper/lower presentation */
#  define ECHO		0000010		/* Enable echo */
#  define ECHOE		0000020		/* Echo ERASE as an error-correcting
					   backspace */
#  define ECHOK		0000040		/* Echo KILL */
#  define ECHONL	0000100		/* Echo '\n' */
#  define NOFLSH	0000200		/* Disable flush after interrupt, quit,
				   	   processing */
/*
#  ifdef __hp9000s800
#    ifdef TEPE
#      define PREAD	0000400
#    endif
#  endif
 */

#  define TOSTOP	010000000000	/* Send SIGTTOU for background output */
#  define IEXTEN	020000000000	/* Enable extended functions */

/* Special Control Characters */
#  define VINTR		0		/* INTR character */
#  define VQUIT		1		/* QUIT character */
#  define VERASE	2		/* ERASE character */
#  define VKILL		3		/* KILL character */
#  define VEOF 		4		/* EOF character */
#  define VEOL		5		/* EOL character */
#  define _VEOL2	6		/* Backward compatability to termio */

#  ifdef _TERMIOS_INCLUDED
#    define VMIN	11		/* MIN value */
#    define VTIME	12		/* TIME value */
#    define _V2_VMIN	VEOF		/* TCSETA VMIN */
#    define _V2_VTIME	VEOL		/* TCSETA VTIME */
#    define VSUSP	13		/* SUSP character */
#    define VSTART	14		/* START character */
#    define VSTOP	15		/* STOP character */
#  else /* not _TERMIOS_INCLUDED */
#    define VMIN	VEOF		/* MIN value */
#    define VTIME	VEOL		/* TIME value */
#  endif /* else not _TERMIOS_INCLUDED */
  
/* Baud Rate Values */
#  define _CBAUD	0000037
#  define B0		0 		/* Hang up */
#  define B50		0000001		/* 50 baud */
#  define B75		0000002		/* 75 baud */
#  define B110		0000003		/* 110 baud */
#  define B134		0000004		/* 134 baud */
#  define B150		0000005		/* 150 baud */
#  define B200		0000006		/* 200 baud */
#  define B300		0000007		/* 300 baud */
#  define B600		0000010		/* 600 baud */
#  define _B900		0000011		/* 900 baud */
#  define B1200		0000012		/* 1200 baud */
#  define B1800		0000013		/* 1800 baud */
#  define B2400		0000014		/* 2400 baud */
#  define _B3600	0000015		/* 3600 baud */
#  define B4800		0000016		/* 4800 baud */
#  define _B7200	0000017		/* 7200 baud */
#  define B9600		0000020		/* 9600 baud */
#  define B19200	0000021		/* 19200 baud */
#  define B38400	0000022		/* 38400 baud */
#  define _B57600	0000023		/* 57600 baud */
#  define _B115200	0000024		/* 115200 baud */
#  define _B230400	0000025		/* 230400 baud */
#  define _B460800	0000026		/* 460800 baud */

#  ifdef _TERMIOS_INCLUDED
#    ifndef _KERNEL
      /* tcsetattr() constants */
#      define TCSANOW	0
#      define TCSADRAIN	1
#      define TCSAFLUSH	2

      /* tcflush() constants */
#      define TCIFLUSH	0
#      define TCOFLUSH	1
#      define TCIOFLUSH	2

      /* tcflow() constants */
#      define TCOOFF	0
#      define TCOON	1
#      define TCIOFF	2
#      define TCION	3

#      ifdef __cplusplus
         extern "C" {
#      endif /* __cplusplus */
#      ifdef _PROTOTYPES
         extern speed_t cfgetispeed(const struct termios *);
         extern speed_t cfgetospeed(const struct termios *);
         extern int cfsetispeed(struct termios *, speed_t);
         extern int cfsetospeed(struct termios *, speed_t);
         extern int tcgetattr(int, struct termios *);
         extern int tcsetattr(int, int, const struct termios *);
         extern int tcsendbreak(int, int);
         extern int tcdrain(int);
         extern int tcflush(int, int);
         extern int tcflow(int, int);
#      else /* not _PROTOTYPES */
         extern speed_t cfgetispeed();
         extern speed_t cfgetospeed();
         extern int cfsetispeed();
         extern int cfsetospeed();
         extern int tcgetattr();
         extern int tcsetattr();
         extern int tcsendbreak();
         extern int tcdrain();
         extern int tcflush();
         extern int tcflow();
#      endif /* _PROTOTYPES */
#      ifdef __cplusplus
         }
#      endif /* __cplusplus */
#    endif /* not _KERNEL */
#  endif /* _TERMIOS_INCLUDED */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_XOPEN_SOURCE
/* Input Modes */
#  define	IUCLC	_IUCLC		/* Map upper case to lower case 
					   input added to ansic */	
#  define	IXANY   _IXANY		/* enable any character to restart 
					   output */	

/* output modes */		/* original termio.h definition */
#  define	OLCUC	_OLCUC
#  define	ONLCR	_ONLCR
#  define	OCRNL	_OCRNL
#  define	ONOCR	_ONOCR
#  define	ONLRET	_ONLRET
#  define	OFILL	_OFILL	
#  define	OFDEL	_OFDEL
#  define	NLDLY	_NLDLY
#  define	NL0	_NL0
#  define	NL1	_NL1
#  define	CRDLY	_CRDLY
#  define	CR0	_CR0
#  define	CR1	_CR1
#  define	CR2	_CR2
#  define	CR3	_CR3
#  define	TABDLY	_TABDLY
#  define	TAB0	_TAB0
#  define	TAB1	_TAB1
#  define	TAB2	_TAB2
#  define	TAB3	_TAB3
#  define	BSDLY	_BSDLY
#  define	BS0	_BS0
#  define	BS1	_BS1
#  define	VTDLY	_VTDLY
#  define	VT0	_VT0
#  define	VT1	_VT1
#  define	FFDLY	_FFDLY
#  define	FF0	_FF0
#  define	FF1	_FF1

/* Baud Rate Values */
#  define	CBAUD	_CBAUD

/* control modes */
#  define 	EXTA	_EXTA
#  define 	EXTB	_EXTB
#  define	LOBLK	_LOBLK

/* Local Modes */		  /* ansic */
#  define XCASE   _XCASE          /*  Canonical upper/lower presentation */ 

#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#ifdef _KERNEL_BUILD
#    include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

#  ifndef NCC
#    define	NCC	8
#  endif /* NCC */

/* control characters */
#  define	VSWTCH	7

#  define	CNUL	0
#  define	CDEL	0377

/* default control chars */
#  define	CESC	'\\'
#  define	CINTR	0177	/* DEL */
#  define	CQUIT	034	/* FS, cntl | */
#  define	CERASE	'#'
#  define	CKILL	'@'
#  define	CEOF	04	/* cntl d */
#  define	CSTART	021	/* cntl q */
#  define	CSTOP	023	/* cntl s */
#  define	CSWTCH	032	/* cntl z */
#  define	CNSWTCH	0

/* input modes */
#  define	IENQAK	_IENQAK

/* Baud Rate Values and Control Modes */
#  define B900		_B900		/* 900 baud    */
#  define B3600		_B3600		/* 3600 baud   */
#  define B7200		_B7200		/* 7200 baud   */
#  define B57600	_B57600		/* 57600 baud  */
#  define B115200	_B115200	/* 115200 baud */
#  define B230400	_B230400	/* 230400 baud */
#  define B460800	_B460800	/* 460800 baud */
#  define LOBLK		_LOBLK

#  ifdef __hp9000s800
#    ifdef TEPE
#      define PREAD	0000400
#    endif /* TEPE */
#  endif /* __hp9000s800 */

/* Special Control Characters */
#  define VEOL2		_VEOL2		/* Backward compatability to termio */

/* line discipline 0 modes */
#  define	SSPEED	7	/* default speed: 300 baud */

/* Some implementations may not support all baud rates */
/* Input baud rates are shifted by the kernel */
#  define COUTBAUD	CBAUD		/* output baud rate */
#  define CINBAUD	0x1f000000	/* input baud rate */
#  define COUTBAUDSHIFT	0		/* output baud rate shift factor */
#  define CINBAUDSHIFT	24		/* input baud rate shift factor */

#  ifdef _TERMIOS_INCLUDED
/*
 * P1003.1 ioctl interface 
 */
#    define TCGETATTR  _IOR('T', 16, struct termios)  /* get parameters */
#    define TCSETATTR  _IOW('T', 17, struct termios)  /* set parameters */
#    define TCSETATTRD _IOW('T', 18, struct termios)  /* set parameters after
						     output has drained */
#    define TCSETATTRF _IOW('T', 19, struct termios)  /* set parameters after
						     flushing output and 
						     discard unread input */
#  endif /* _TERMIOS_INCLUDED */

#  ifdef _INCLUDE_TERMIO
/*
 * Ioctl control packet
 */
     struct termio {
	unsigned short	c_iflag;	/* input modes 		*/
	unsigned short	c_oflag;	/* output modes 	*/
	unsigned short	c_cflag;	/* control modes 	*/
	unsigned short	c_lflag;	/* line discipline modes*/
	char	        c_line;		/* line discipline 	*/
	unsigned char	c_cc[NCC];	/* control chars 	*/
     };

#    define	TCGETA	_IOR('T', 1, struct termio) /* get parameters */
#    define	TCSETA	_IOW('T', 2, struct termio) /* put parameters */
#    define	TCSETAW	_IOW('T', 3, struct termio)
#    define	TCSETAF	_IOW('T', 4, struct termio)
#  endif /* _INCLUDE_TERMIO */
#    define	TCSBRK	_IO('T', 5)
#    define	TCXONC	_IO('T', 6)
#    define	TCFLSH	_IO('T', 7)
#  ifdef _KERNEL
#    undef _INCLUDE_TERMIO
#  endif /* _KERNEL */
#      define	IOCTYPE	0177400
#      define	TIOC	('T'<<8)

#  define  TIOCSCTTY _IO('T', 33)  /* Used only with streams */
#  define  TIOCCONS _IO('t', 104)  /* get console I/O (fd becomes the console ) */

/* Interface to get and set terminal size. */
struct	winsize {
        unsigned short	ws_row;		/* Rows, in characters		*/
        unsigned short	ws_col;		/* Columns, in characters	*/
        unsigned short	ws_xpixel;	/* Horizontal size, pixels	*/
        unsigned short	ws_ypixel;	/* Vertical size, pixels	*/
};

#  define TIOCGWINSZ      _IOR('t', 107, struct winsize)  /* get window size */
#  define TIOCSWINSZ      _IOW('t', 106, struct winsize)  /* set window size */

/* The following definitions are for source importability of Sun */
/* ioctls for terminal size.                                     */
#  ifndef _KERNEL
#    define ttysize	winsize
#    define ts_lines	ws_row
#    define ts_cols	ws_col
#    define TIOCSSIZE	TIOCSWINSZ
#    define TIOCGSIZE	TIOCGWINSZ
#  endif /* not _KERNEL */

#  ifdef __hp9000s300
#  define	LDIOC	_IO('D', 0)
#  define	LDOPEN	_IO('D', 0)
#  define	LDCLOSE	_IO('D', 1)
#  define	LDCHG	_IO('D', 2)

#  define  TCDSET   _IOW('T', 32, int)
/* NOTE:  The following ioctl command values are not currently */
/*        supported by HP-UX. */

#  endif /* __hp9000s300 */
#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_TERMIO_INCLUDED */
