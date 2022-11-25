/* 	@(#)bsdtermio.c 	$Revision: 1.6.109.1 $	$Date: 91/11/19 14:15:16 $  */

/*
 * This file contains routines for emulating bsd termio calls on HP systems.
 * Actually, this is designed for use with Sun termio calls, so it may be
 * slightly different from bsd systems.  Translations are done as best as
 * possible, but cannot be perfect.
 */

#include <termio.h>
#include <sys/errno.h>
#include <sys/signal.h>
/*
 * Several constants are defined in both termio.h and sgtty.h with 
 * conflicting values. In each case the value we want is in sgtty.h
 * so we use bsdundef.h to undefine the definitions from termio.h
 */
#include "bsdundef.h"
#include <sgtty.h>
#include <bsdterm.h>

/*
 * These are the functions that will be used to emulate the BSD ioctl calls.
 */
static int tiocgetp();
static int tiocsetp();
static int tiocgetc();
static int tiocsetc();
static int tiocgltc();
static int tiocsltc();
static int tioclget();
static int tioclset();
static int tiocgetd();
static int tiocsetd();
static int tioclbisc();

/*
 * Table mapping ioctl requests into a function that will perform them.
 */

struct lookup {
	int op;
	int (*op_function)();
};

static struct lookup optab[] = {
	{ TIOCGETP,	tiocgetp },
	{ TIOCSETP,	tiocsetp },
	{ TIOCSETN,	tiocsetp }, /* No way to do tiocsetn, emulate with p*/
	{ TIOCGETC,	tiocgetc },
	{ TIOCSETC,	tiocsetc },
	{ TIOCGLTC,	tiocgltc },
	{ TIOCSLTC,	tiocsltc },
	{ TIOCLGET,	tioclget },
	{ TIOCLSET,	tioclset },
	{ TIOCGETD,	tiocgetd },
	{ TIOCSETD,	tiocsetd },
	{ TIOCLBIS,	tioclbisc },
	{ TIOCLBIC,	tioclbisc },
};

#define OPTAB_SIZE sizeof(optab) / sizeof( struct lookup)

/*
 * bsd_gtty() and bsd_stty() are essentially the same as our stty() and
 * gtty().  However, there are a few differences, so we allow these 
 * routines to use the routines defined here.
 */

int
bsd_gtty(fd, argp)
int fd;
struct sgttyb *argp;
{
	return( bsd_tty_ioctl( fd, TIOCGETP, argp) );
}

int bsd_stty(fd, argp)
int fd;
struct sgttyb *argp;
{
	return( bsd_tty_ioctl( fd, TIOCSETP, argp) );
}

/*
 * Main routine.  The idea here is that the user should just be able to
 * replace certain ioctl() calls in his code with bsd_tty_ioctl() calls.
 * If emulation is simple, it is done in line, otherwise go to a special
 * routine that handles mapping the values.
 *
 * Note that the arg parameter may be either a pointer to a structure or an
 * integer depending on the operation.  We assume that the size of these two
 * are the same and that it's ok to just pass it around as an integer.
 */

bsd_tty_ioctl(fd, op, arg)
int fd, op;
int arg;
{
	int ret;	/* return value */
	extern int errno;
	int i;

	for (i = 0 ; i < OPTAB_SIZE ; i++ )
		if ( optab[i].op == op ) 
			return ((*optab[i].op_function)(fd, op, arg));

	/*
	 * This function is not supported by the bsd_tty_ioctl call.
	 */
	errno = EINVAL;
	return (-1);
}

/*
 * tiocgetp -- get the sgttyb structure
 *
 * While the HP structure is the same as BSD/Sun, some of the flags are
 * different.  Have to map these values as best as possible.  In particular,
 * we also have to look at the termio structure to emulate CBREAK.  The
 * erase and kill characters should be fine.  Speeds are mapped in a 
 * separate routine for clarity.
 */

static int
tiocgetp(fd, op, arg)
int fd;
struct sgttyb *arg;
{
	int ret;
	void hp_to_bsd_speed();
	struct termio term;

	if ( (ret = ioctl(fd, TCGETA, &term)) < 0 )
		return (ret);

	if ( (ret = ioctl(fd, TIOCGETP, arg)) < 0 )
		return (ret);

	hp_to_bsd_speed( &arg->sg_ispeed);
	arg->sg_ospeed = arg->sg_ispeed;

	/*
	 * First, no such thing as HUPCL on BSD, so turn it off.
	 * XTABS translates into different spot on BSD.  Since it occupies the
	 * space CBREAK does in BSD, we have to handle it first.
	 * Any flag not explicitly handled here SHOULD be fine.
	 */
	arg->sg_flags &= ~HUPCL;
	if ( (arg->sg_flags & XTABS) == XTABS ) {
		arg->sg_flags |= BSD_XTABS;
		arg->sg_flags &= ~XTABS;
	}
	/*
	 * The closest thing we have to CBREAK on HP is if input processing
	 * is turned off, i.e. ICANON is NOT set.
	 */
	if ( !(term.c_lflag & ICANON) ) {
		arg->sg_flags |= BSD_CBREAK;
	}

	return (0);
}

/*
 * tiocsetp() -- set the sgttyb structure, mapping values from BSD into HP
 * meanings.  Most flags in the sgttyb structure are OK.  The flags we have
 * to be careful of are TANDEM, CBREAK, and the tabs values.  We also have
 * to map the speeds into the appropriate HP values.
 */

static int
tiocsetp(fd, op, arg)
int fd, op;
struct sgttyb *arg;
{
	int ret;
	void bsd_to_hp_speed();
	struct termio term;

	/*
	 * Get the current modes for working with for special cases.
	 */
	if ( (ret = ioctl(fd, TCGETA, &term)) < 0 )
		return (ret);

	/*
	 * First adjust the speed to the appropriate HP value
	 */
	bsd_to_hp_speed( &arg->sg_ispeed);
	arg->sg_ospeed = arg->sg_ispeed;

	/*
	 * Now, fix those values that aren't the same between BSD and what
	 * HP does.  First, clear the TANDEM flag, since HP doesn't have that
	 * Then, look at the BSD CBREAK flag, and try to set HP values
	 * accordingly.  Finally, copy the XTABS value into its correct spot.
	 */
	arg->sg_flags &= ~BSD_TANDEM;
	if ( arg->sg_flags & BSD_CBREAK ) {
		/* 
		 * If RAW is set, it will override CBREAK, so just turn it off.
		 * Otherwise, if RAW is not set, first turn off input processing
		 */
		if ( !(arg->sg_flags & RAW) ) {	
			term.c_lflag &= ~ICANON;
			term.c_oflag |= OPOST;
		}
		arg->sg_flags &= ~BSD_CBREAK;
	}

	if ( (arg->sg_flags & BSD_XTABS) == BSD_XTABS ) {
		arg->sg_flags |= XTABS;
		arg->sg_flags &= ~BSD_XTABS;
	}

	/*
	 * Now set the new values. The ioctl call with TCSETAW must be 
	 * made before the ioctl call with TIOCSETP in order to work
	 * correctly.
	 */

	if ( (ret = ioctl(fd, TCSETAW, &term) ) < 0 )
		return (ret);

	ret = ioctl(fd, TIOCSETP, arg);
	return (ret);

}

/*
 * Functions to get and set the special chars on BSD.  This translates into
 * simply copying some of them out of the termio structure, and setting
 * the start/stop characters to ^Q/^S if flow control is on, or to -1(0377) if
 * flow control is off.
 */

static int
tiocgetc( fd, op, arg )
int fd, op;
struct tchars *arg;
{
	struct termio term;
	int ret;

	if ( (ret = ioctl(fd, TCGETA, &term)) < 0 )
		return (ret);

	arg->t_intrc = term.c_cc[VINTR];
	arg->t_quitc = term.c_cc[VQUIT];
	arg->t_eofc  = term.c_cc[VEOF];
	arg->t_brkc  = term.c_cc[VEOL];
	if ( term.c_iflag & IXON ) {
		arg->t_startc = CSTART;
		arg->t_stopc  = CSTOP;
	}
	else {
		arg->t_startc = 0377;
		arg->t_stopc  = 0377;
	}
	return (0);
}

static int
tiocsetc(fd, op, arg)
int fd, op;
struct tchars *arg;
{
	struct termio term;
	int ret;

	if ( (ret = ioctl(fd, TCGETA, &term)) < 0 )
		return (ret);
	
	term.c_cc[VINTR] = arg->t_intrc;
	term.c_cc[VQUIT] = arg->t_quitc;
	term.c_cc[VEOF]  = arg->t_eofc;
	term.c_cc[VEOL]  = arg->t_brkc;
	/*
	 * Choice to make here.  How do we interpret values for start/stop
	 * that are different from ^Q/^S?  Clearly if given -1 for both, we
	 * should disable flow control, but what about other values?  For
	 * now, let's play it safe and if they didn't give us ^Q/^S, turn
	 * off flow control.
	 */
	if ( (arg->t_startc != CSTART) || (arg->t_stopc != CSTOP) ) {
		term.c_iflag &= ~IXON;
	}
	else {
		term.c_iflag |= IXON;
	}

	return ( ioctl(fd, TCSETA, &term) );
		
}

/*
 * The following routines deal with the second structure of special characters.
 * This set is different than the first.  With the first set, we characters
 * with the same meanings, we just had to map them.  With this set of 
 * characters, we actually support the structure, but only allow certain
 * fields to be set, and then only on systems with job control.  Therefore
 * we ifdef based on SIGTSTP, which is the usual method for determining if
 * the system we are on supports job control.  On systems without job control,
 * we return all chars as -1 (0377) to indicate they are not set, and ignore
 * any attempt to set them.  On systems with job control, we set the non
 * supported characters to -1 to avoid getting an error on the ioctl call.
 */

static int
tiocgltc(fd, op, arg)
int fd, op;
struct ltchars *arg;
{
#ifdef SIGTSTP
	return( ioctl(fd, op, arg) );
#else no SIGTSTP
	arg->t_suspc = 0377;
	arg->t_dsuspc = 0377;
	arg->t_rprntc = 0377;
	arg->t_flushc = 0377;
	arg->t_werasc = 0377;
	arg->t_lnextc = 0377;
	return (0);
#endif SIGTSTP
}

static int
tiocsltc(fd, op, arg)
int fd, op;
struct ltchars *arg;
{
#ifdef SIGTSTP
	arg->t_rprntc = 0377;
	arg->t_flushc = 0377;
	arg->t_werasc = 0377;
	arg->t_lnextc = 0377;
	return( ioctl(fd, op, arg) );
#else SIGTSTP
	return( 0 );
#endif SIGTSTP
}

/*
 * Routine for mapping the local mode word.  This is a weird one.  We support
 * the call on the systems with job control, but only with the LTOSTOP bit
 * honored.  Other bits map onto values in the HP-UX termio stuff, or other
 * special calls.  Finally, there are calls both to explicitly set the flag
 * word, and calls to modify the word by certain bits.
 * Bottom line is that this is somewhat funky.
 */

static int
tioclget( fd, op, arg )
int fd, op;
int *arg;
{
	int ret;
	struct termio term;

	if ( (ret = ioctl( fd, TCGETA, &term)) < 0 )
		return (ret);

	/*
	 * Get our version of the local mode word on systems with Job Control.
	 * On systems without job control, just zero everything.
	 */
#ifdef SIGTSTP
	if ( (ret = ioctl( fd, TIOCLGET, arg)) < 0 )
		return(ret);

	/*
	 * Map LTOSTOP to the BSD placement.
	 */
	if ( (*arg) & LTOSTOP ) {
		*arg |= BSD_LTOSTOP;
		*arg &= ~LTOSTOP;
	}
#else
	*arg = 0;
#endif SIGTSTP

	/*
	 * If IXANY is not set, then turn on LDECCTQ since we then have to
	 * have a ^Q to restart after ^S (like DEC).
	 */
	if ( (term.c_iflag & IXANY) == 0 ) {
		*arg |= BSD_LDECCTQ;
	}

	/*
	 * Check the status of how we echo the erase character.
	 */
	if ( term.c_lflag & ECHOE ) {
	  /* need to set both BSD_LCRTBS and  BSD_LCRTERA  *mjk 11/10/88* */
	   *arg |= BSD_LCRTBS;
	   *arg |= BSD_LCRTERA;
	}

	/*
	 * If OPOST is not on, then no output processing.
	 */
	if ( (term.c_oflag & OPOST) == 0 ) {
		*arg |= BSD_LLITOUT;
	}

	/*
	 * Finally, map the no flush flag.
	 */
	if ( (term.c_lflag & NOFLSH ) ) {
		*arg |= BSD_LNOFLSH;
	}

	return ( 0 );
}

static int
tioclset( fd, op, arg )
int fd, op;
int *arg;
{
	int ret;
	struct termio term;

	/*
	 * Get the term structure to work with
	 */
	if ( (ret = ioctl( fd, TCGETA, &term)) < 0 )
		return (ret);

	/*
	 * Check the status of how we echo the erase character.
	 * Both BSD_LCRTERA and BSD_LCRTBS must be set to echo *mjk 11/10/88 *
	 */
	if ( ((*arg) & BSD_LCRTERA)  && ((*arg) & BSD_LCRTBS)  )
		term.c_lflag |= ECHOE;
	else
		term.c_lflag &= ~ECHOE;

	/*
	 * If LDECCTQ is set, turn off IXANY so that you must use ^S/^Q
	 * If not set, then we don't necessarily turn IXANY on, since we
	 * don't really know what we're doing.
	 */
	if ( (*arg) & BSD_LDECCTQ )
		term.c_iflag &= ~IXANY;

	/*
	 * If LLITOUT is on, then no output processing. else due output 
	 *            processing.  *changed by mjk 11/10/88*
	 */
	if ( (*arg) & BSD_LLITOUT )
	 	term.c_oflag &= ~OPOST;
	else
		term.c_oflag |= OPOST;

	/*
	 * Finally, map the no flush flag.
	 */
	if ( (*arg) & BSD_LNOFLSH )
		term.c_lflag |= NOFLSH;
	else
		term.c_lflag &= ~NOFLSH;

#ifdef SIGTSTP
	/*
	 * Map LTOSTOP to the HP placement.
	 */
	if ( (*arg) & BSD_LTOSTOP ) {
		*arg |= LTOSTOP;
		*arg &= ~BSD_LTOSTOP;
	}
	/*
	 * Now turn off all flags not supported by HP by anding with
	 * all flags that we do support (currently only LTOSTOP).
	 */
	*arg &= LTOSTOP;

	/*
	 * Now set the new values. The ioctl call with TCSETAW must be 
	 * made before the ioctl call with TIOCLSET in order to work
	 * correctly.
	 */
	if ( (ret = ioctl( fd, TCSETAW, &term)) < 0 )
	        return(ret);

	ret = ioctl( fd, TIOCLSET, arg);

	return (ret);
#endif SIGTSTP

}

/*
 * Do the masking ones by brute force...
 */

static int
tioclbisc(fd, op, arg)
int fd, op;
int *arg;
{
	int curval;
	int ret;

	if ( (ret = tioclget(fd, TIOCLGET, &curval)) < 0 )
		return (ret);

	if ( op == TIOCLBIS )
	    curval |= (*arg);
	else
	    curval &= ~(*arg);

	return ( tioclset(fd, TIOCLSET, &curval) );
}

/*
 * Line discipline, new, old or network.  We really don't have anything
 * analogous to this, so we always act as if we support the new discipline,
 * and return EINVAL errors if you try to set it to something else.
 */

static int
tiocgetd( fd, op, arg)
int fd, op;
int *arg;
{
	*arg = BSD_NTTYDISC;
	return (0);
}

static int
tiocsetd( fd, op, arg)
int fd, op;
int *arg;
{
	if ( *arg == BSD_NTTYDISC )
		return ( 0 );

	errno = EINVAL;
	return ( -1 );

}

/*
 * Table for mapping baud rates back and forth.  The only interesting
 * thing here is that HP supports some baud rates that aren't available
 * on BSD systems, so the emulation can't be perfect.
 */

#define SPEED_TABLE_SIZE 21

static char speed_table[SPEED_TABLE_SIZE][2] = {
	B0,	BSD_B0,
	B50,	BSD_B50,
	B75,	BSD_B75,
	B110,	BSD_B110,
	B134,	BSD_B134,
	B150,	BSD_B150,
	B200,	BSD_B200,
	B300,	BSD_B300,
	B600,	BSD_B600,
	B900,	BSD_B600,
	B1200,  BSD_B1200,
	B1800,  BSD_B1800,
	B2400,  BSD_B2400,
	B3600,  BSD_B2400,
	B4800,  BSD_B4800,
	B7200,  BSD_B4800,
	B9600,  BSD_B9600,
	B19200, BSD_B19200,
	B38400, BSD_B38400,
	EXTA,	BSD_EXTA,
	EXTB,	BSD_EXTB,
};

/*
 * A set of routines to do the mapping.
 */

static void
hp_to_bsd_speed( speed )
char *speed;
{
	int i;

	for ( i = 0; i < SPEED_TABLE_SIZE; i++ )
		if ( speed_table[i][0] == *speed ) {
			*speed = speed_table[i][1];
			break;
		}

	return;
}

static void
bsd_to_hp_speed( speed )
char *speed;
{
	int i; 

	for ( i=0 ; i < SPEED_TABLE_SIZE; i++ )
		if ( speed_table[i][1] == *speed ) {
			*speed = speed_table[i][0];
			break;
		}

	return;
}
