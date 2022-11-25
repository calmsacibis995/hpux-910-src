/* @(#) $Revision: 27.2 $ */   
/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

static short    baud_convert[] =
{
	0,
	50,
	75,
	110,
	135,
	150,
	200,
	300,
	600,
	900,
	1200,
	1800,
	2400,
	3600,
	4800,
	7200,
	9600,
	19200,
	38400,
	-1,		/* EXTA */
	-1		/* EXTB */
};

/*
 * Force output to be buffered.
 * Also figures out the baud rate.
 * Grouped here because they are machine dependent.
 */
_setbuffered(fd)
FILE *fd;
{
	char *sobuf;
	char *calloc();
	SGTTY   sg;

	sobuf = calloc(1, BUFSIZ);
	setbuf(fd, sobuf);

# ifdef USG
	ioctl (fileno (fd), TCGETA, &sg);
	SP->baud = sg.c_cflag&CBAUD ? baud_convert[sg.c_cflag&CBAUD] : 1200;
# else
	ioctl (fileno (fd), TIOCGETP, &sg);
	SP->baud = sg.sg_ospeed ? baud_convert[sg.sg_ospeed] : 1200;
# endif
}
