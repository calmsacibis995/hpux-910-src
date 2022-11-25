/* @(#) $Revision: 66.1 $ */

#ifdef _NAMESPACE_CLEAN
#   define cfgetospeed _cfgetospeed
#   define cfsetospeed _cfsetospeed
#   define cfgetispeed _cfgetispeed
#   define cfsetispeed _cfsetispeed
#   define tcgetattr _tcgetattr
#   define tcsetattr _tcsetattr
#   define tcsendbreak _tcsendbreak
#   define tcdrain _tcdrain
#   define tcflush _tcflush
#   define tcflow _tcflow
#   define tcgetpgrp _tcgetpgrp
#   define tcsetpgrp _tcsetpgrp
#   define ioctl _ioctl
#endif

#include <errno.h>
#include <termios.h>
#include <sys/bsdtty.h>
#include <sys/types.h>

/*
 *  These are the functions which are defined by the POSIX 1003.1
 *  Draft 12.3 Section 7 General Terminal Interface
 */

#ifdef _NAMESPACE_CLEAN
#   undef cfgetospeed
#   pragma _HP_SECONDARY_DEF _cfgetospeed cfgetospeed
#   define cfgetospeed _cfgetospeed
#endif

speed_t
cfgetospeed (termios_p)
struct termios *termios_p;
{
    return (termios_p->c_cflag & COUTBAUD) >> COUTBAUDSHIFT;
}

#ifdef _NAMESPACE_CLEAN
#   undef cfsetospeed
#   pragma _HP_SECONDARY_DEF _cfsetospeed cfsetospeed
#   define cfsetospeed _cfsetospeed
#endif

int
cfsetospeed (termios_p, speed)
struct termios *termios_p;
register int speed;
{
    if ((speed & COUTBAUD) != speed) {
	errno = EINVAL;
	return -1;
    }
    termios_p->c_cflag &= ~COUTBAUD;
    termios_p->c_cflag |= (speed << COUTBAUDSHIFT);
    return 0;
}

#ifdef _NAMESPACE_CLEAN
#   undef cfgetispeed
#   pragma _HP_SECONDARY_DEF _cfgetispeed cfgetispeed
#   define cfgetispeed _cfgetispeed
#endif

speed_t
cfgetispeed (termios_p)
struct termios *termios_p;
{
    return (termios_p->c_cflag & CINBAUD) >> CINBAUDSHIFT;
}

#ifdef _NAMESPACE_CLEAN
#   undef cfsetispeed
#   pragma _HP_SECONDARY_DEF _cfsetispeed cfsetispeed
#   define cfsetispeed _cfsetispeed
#endif

int
cfsetispeed(termios_p, speed)
register struct termios *termios_p;
register int speed;
{
    if ((speed & COUTBAUD) != speed) {
	errno = EINVAL;
	return -1;
    }
    termios_p->c_cflag &= ~CINBAUD;

    /* if speed is 0 then set input baud to output baud */
    termios_p->c_cflag |= (speed ? (speed & COUTBAUD)
	    : (termios_p->c_cflag & COUTBAUD)) << CINBAUDSHIFT;
    return 0;
}

#ifdef _NAMESPACE_CLEAN
#   undef tcgetattr
#   pragma _HP_SECONDARY_DEF _tcgetattr tcgetattr
#   define tcgetattr _tcgetattr
#endif

int
tcgetattr(fd, termios_p)
int fd;
struct	termios *termios_p;
{
    register int ret_val;
    register int sav_err;

    sav_err = errno;
    errno = 0;

    ret_val = ioctl(fd, TCGETATTR, termios_p);

    if (errno != 0)
    {
	if (errno == EINVAL)
	    errno = ENOTTY;
    }
    else
	errno = sav_err;

    return ret_val;
}

#ifdef _NAMESPACE_CLEAN
#   undef tcsetattr
#   pragma _HP_SECONDARY_DEF _tcsetattr tcsetattr
#   define tcsetattr _tcsetattr
#endif

int
tcsetattr(fd, cmd, termios_p)
int fd;
struct termios *termios_p;
register int cmd;
{
    switch (cmd)
    {
    case TCSANOW:
	cmd = TCSETATTR;
	break;
    case TCSADRAIN:
	cmd = TCSETATTRD;
	break;
    case TCSAFLUSH:
	cmd = TCSETATTRF;
	break;
    default:
	errno = EINVAL;
	return -1;
    }
    return ioctl(fd, cmd, termios_p);
}

#ifdef _NAMESPACE_CLEAN
#   undef tcsendbreak
#   pragma _HP_SECONDARY_DEF _tcsendbreak tcsendbreak
#   define tcsendbreak _tcsendbreak
#endif

int
tcsendbreak(fd, duration)
int fd;
int duration;
{
    /*
     * Use AT&T TCSBRK, which guarantees a break for at
     * least .25 sec, which is compliant with the POSIX standard.
     * Note that no wait is done for output to drain.
     */
    return ioctl(fd, TCSBRK, 0);
}

#ifdef _NAMESPACE_CLEAN
#   undef tcdrain
#   pragma _HP_SECONDARY_DEF _tcdrain tcdrain
#   define tcdrain _tcdrain
#endif

int
tcdrain(fd)
int fd;
{
    /*
     * Use AT&T TCSBRK.  The driver does a ttywait(), then if the
     * duration is non-zero, the driver returns without sending
     * <BREAK>.
     */
    return ioctl(fd, TCSBRK, 1);
}

#ifdef _NAMESPACE_CLEAN
#   undef tcflush
#   pragma _HP_SECONDARY_DEF _tcflush tcflush
#   define tcflush _tcflush
#endif

int
tcflush(fd, queue)
int fd;
int queue;
{
    int retval;

    /*
     * Use AT&T TCFLSH.  TCIFLUSH, TCOFLUSH, and TCIOFLUSH are
     * defined as 0, 1, and 2, respectively in termios.h to match
     * the AT&T queue values.
     *
     * NOTE:  to meet POSIX intertpretations of the atomicity of
     * flushes, we are adding a drain after flush. Maybe in 8.0,
     * we will move this into the kernel
     */
    switch (queue)
    {
    case TCOFLUSH:
    case TCIOFLUSH:
	if (retval = ioctl(fd, TCFLSH, TCOFLUSH)) /* flush output Q */
	    return retval;
	if (retval= ioctl(fd, TCSBRK, 1))	  /* drain output Q */
	    return retval;

	if (queue == TCOFLUSH)
	    return retval; /* don't flush input queue */

	/* falls through */

    case TCIFLUSH:
	return  ioctl(fd, TCFLSH, TCIFLUSH);	/* flush input Q */

    default:
	errno = EINVAL;
        return -1;
    }
}

#ifdef _NAMESPACE_CLEAN
#   undef tcflow
#   pragma _HP_SECONDARY_DEF _tcflow tcflow
#   define tcflow _tcflow
#endif

int
tcflow(fd, action)
int fd;
int action;
{
    /*
     * Use AT&T TCXONC.  TCOOFF, TCOON, TCIOFF, and TCION are
     * defined as 0, 1, 2, and 3, respectively in termios.h to match
     * the AT&T queue values.
     */
    return ioctl(fd, TCXONC, action);
}

#ifdef _NAMESPACE_CLEAN
#   undef  tcgetpgrp
#   pragma _HP_SECONDARY_DEF _tcgetpgrp tcgetpgrp
#   define tcgetpgrp _tcgetpgrp
#endif

pid_t
tcgetpgrp(fd)
int fd;
{
    int arg;

    if (ioctl(fd, TIOCGPGRP, &arg) == -1)
	return -1;
    else
	return arg;
}

#ifdef _NAMESPACE_CLEAN
#   undef tcsetpgrp
#   pragma _HP_SECONDARY_DEF _tcsetpgrp tcsetpgrp
#   define tcsetpgrp _tcsetpgrp
#endif

int
tcsetpgrp(fd, pgrp_id)
int fd;
pid_t pgrp_id;
{
    return ioctl(fd, TIOCSPGRP, &pgrp_id);
}
