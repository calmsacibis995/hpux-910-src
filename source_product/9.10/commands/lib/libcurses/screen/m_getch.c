/* @(#) $Revision: 70.2 $ */    
#include "curses.ext"
#include <signal.h>
#include <errno.h>

extern	chtype	_oldctrl;
extern	short	_oldy;
extern	short	_oldx;

static int sig_caught;

/*
 *	This routine reads in a character from the window.
 *
 *	wgetch MUST return an int, not a char, because it can return
 *	things like ERR, meta characters, and function keys > 256.
 */
int
m_getch()
{

	register int inp;
	register int i, j;
	char c;
	int arg;
	bool	weset = FALSE;
	FILE *inf;

	if (SP->fl_echoit && !stdscr->_scroll && (stdscr->_flags&_FULLWIN)
	    && stdscr->_curx == stdscr->_maxx && stdscr->_cury == stdscr->_maxy)
		return ERR;
#ifdef DEBUG
	if(outf) fprintf(outf, "WGETCH: SP->fl_echoit = %c, SP->fl_rawmode = %c\n", SP->fl_echoit ? 'T' : 'F', SP->fl_rawmode ? 'T' : 'F');
	if (outf) fprintf(outf, "_use_keypad %d, kp_state %d\n", stdscr->_use_keypad, SP->kp_state);
#endif
	if (SP->fl_echoit && !SP->fl_rawmode) {
		cbreak();
		weset++;
	}

#ifdef KEYPAD
	/* Make sure keypad on is in proper state */
	if (stdscr->_use_keypad != SP->kp_state) {
		_kpmode(stdscr->_use_keypad);
		fflush(stdout);
	}
#endif

	/* Make sure we are in proper nodelay state */
	if (stdscr->_nodelay != SP->fl_nodelay)
		_fixdelay(SP->fl_nodelay, stdscr->_nodelay);

	/* Check for pushed typeahead.  We make the assumption that
	 * if a function key is being typed, there is no pushed
	 * typeahead from the previous key waiting.
	 */
	if (SP->input_queue[0] >= 0) {
		inp = SP->input_queue[0];
		for (i=0; i<16; i++) {
			SP->input_queue[i] = SP->input_queue[i+1];
			if (SP->input_queue[i] < 0)
				break;
		}
		goto gotit;
	}

	inf = SP->input_file;
	if (inf == stdout)	/* so output can be teed somewhere */
		inf = stdin;
#ifdef FIONREAD
	if (stdscr->_nodelay) {
		ioctl(fileno(inf), FIONREAD, &arg);
#ifdef DEBUG
		if (outf) fprintf(outf, "FIONREAD returns %d\n", arg);
#endif
		if (arg < 1)
			return -1;
	}
#endif
	for (i = -1; i<0; ) {
		extern int errno;
		sig_caught = 0;
		i = read(fileno(inf), &c, 1);
		/*
		 * I hope the system won't retern infinite EINTRS - maybe
		 * there should be a hop count here.
		 */
		if (i < 0 && errno != EINTR && !sig_caught) {
			inp = ERR;
			goto gotit;
		}
	}
	if (i > 0) {
		inp = c;
#ifdef NONLS
		if (!stdscr->_use_meta)
			inp &= 0177;
		else
			inp &= 0377;
#else
		inp &= 0377;
#endif
	} else {
		inp = ERR;
		goto gotit;
	}
#ifdef DEBUG
	if(outf) fprintf(outf,"WGETCH got '%s'\n",unctrl(inp));
#endif

#ifdef KEYPAD
	/* Check for arrow and function keys */
	if (stdscr->_use_keypad) {
		SP->input_queue[0] = inp;
		SP->input_queue[1] = -1;
		for (i=0; SP->kp[i].keynum > 0; i++) {
			if (SP->kp[i].sends[0] == inp) {
				for (j=0; ; j++) {
					if (SP->kp[i].sends[j] <= 0)
						break;	/* found */
					if (SP->input_queue[j] == -1) {
						SP->input_queue[j] = _fpk(inf);
						SP->input_queue[j+1] = -1;
					}
					if (SP->kp[i].sends[j] != SP->input_queue[j])
						goto contouter; /* not this one */
				}
				/* It matched the function key. */
				inp = SP->kp[i].keynum;
				SP->input_queue[0] = -1;
				goto gotit;
			}
		contouter:;
		}
		/* Didn't match any function keys. */
		inp = SP->input_queue[0];
		for (i=0; i<16; i++) {
			SP->input_queue[i] = SP->input_queue[i+1];
			if (SP->input_queue[i] < 0)
				break;
		}
		goto gotit;
	}
#endif

	if (SP->fl_echoit) {

		chtype	ch;
		short	x;
		short	y;

		m_addch((chtype) inp);

		x = _oldx;
		y = _oldy;
		ch = _oldctrl;

		_oldctrl = 0;

		m_refresh();

		_oldx = x;
		_oldy = y;
		_oldctrl = ch;
	}
gotit:
	if (weset)
		nocbreak();
#ifdef DEBUG
	if(outf) fprintf(outf, "getch returns %o, pushed %o %o %o\n",
		inp, SP->input_queue[0], SP->input_queue[1], SP->input_queue[2]);
#endif
	return inp;
}
m_catch_alarm()
{
	sig_caught = 1;
}

/*
 * Fast peek key.  Like getchar but if the right flags are set, times out
 * quickly if there is nothing waiting, returning -1.
 * f is an output stdio descriptor, we read from the fileno.  win is the
 * window this is supposed to have something to do with.
 */
#ifndef FIONREAD
/*
 * Traditional implementation.  The best resolution we have is 1 second,
 * so we set a 1 second alarm and try to read.  If we fail for 1 second,
 * we assume there is no key waiting.  Problem here is that 1 second is
 * too long, people can type faster than this.
 */
static
_fpk(f)
FILE *f;
{
	char c;
	int rc;
	int (*oldsig)();
	int oldalarm;

	oldsig = signal(SIGALRM, m_catch_alarm);
	oldalarm = alarm(1);
	sig_caught = 0;
	rc = read(fileno(f), &c, 1);
	if (sig_caught) {
		sig_caught = 0;
		alarm(oldalarm);
		return -2;
	}
	signal(SIGALRM, oldsig);
	alarm(oldalarm);
	return rc == 1 ? c : -1;
}
#else /* FIONREAD */
/*
 * If we have the select system call, we can do much better.
 * We wait for long enough for a terminal to send another character
 * (at 15cps repeat rate, this is 67 ms, I'm using 100ms to allow
 * a bit of a fudge factor) and time out more quickly.  Even if we
 * don't have the real 4.2BSD select, we can emulate it with napms
 * and FIONREAD.  napms might be done with only 1 second resolution,
 * but this is no worse than what we have above.
 */

#include <time.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
/*
 * Just in case NBBY, NBTSPW, wsetbit aren't defined in "sys/param.h".
 */
#ifndef NBBY
#    define NBBY    8               /* number of bits in a byte */
#endif

#ifndef NBTSPW
#    define NBTSPW  (NBBY*NBPW)     /* number of bits in an integer */
#endif

#ifndef wsetbit
#    define wsetbit(a,i)    ((a)[(i)/NBTSPW] |= 1 << ((i) % NBTSPW))
#endif

/*
 * Number of ints needed to hold n bits
 */
#define BITS_TO_INTS(n)  (((n)+NBTSPW-1) / NBTSPW)

static
_fpk(f)
FILE *f;
{
    int rc;
    int i;
    int *infd;
    int fds;
    struct timeval timeval;
    char c;

    struct tms tms_buf;
    clock_t start_time, elapsed_time;
    long clock_ticks;  /* to hold number of clock ticks per second */

    extern int errno;
    extern struct timeval tune_timeout;

    fds = getnumfds();
    infd = (int *) malloc((sizeof(int)) * BITS_TO_INTS(fds));
    if (infd == (int *)0)
	return -1;  /* error */
    for (i = 0; i < BITS_TO_INTS(fds); i++)
	infd[i] = 0;
    wsetbit(infd, fileno(f));	/* set the bit for input */

	/* allow user to set timeout */
	/* otherwise set it to default */

    if (tune_timeout.tv_sec == 0 && tune_timeout.tv_usec == 0)
    {
    	timeval.tv_sec = 0;       /* 500 msec - default */
    	/*timeval.tv_usec = 500000;*/
    	timeval.tv_usec = 800000;
    }
    else
    {                             /* user has set this value */
    	timeval.tv_sec = tune_timeout.tv_sec;
    	timeval.tv_usec = tune_timeout.tv_usec;
    }

	/* retry when select returns because of interruption by other signals */
	/* retry till the normal timeout period elapses and check
	   if any character has arrived
	 */

    start_time = times(&tms_buf);
    elapsed_time = 0;
    clock_ticks = sysconf(_SC_CLK_TCK);   /* clock ticks per second */
    do
    {
    errno = 0;
    rc = select(fds, infd, (int *)0, (int *)0, &timeval);
    if (rc < 0 && errno == EINTR)
    {
	elapsed_time = times(&tms_buf) - start_time;
    }
    else
	break;
    } while ( 3*elapsed_time <= clock_ticks);

    /* 3*elapsed_time <= clock_ticks - time out for one third of a second */

    if (3*elapsed_time > clock_ticks)
    {
	int waitinginput = 0;

	ioctl (fileno(SP -> input_file), FIONREAD, &waitinginput);
	if (waitinginput == 0)
	   return -2; /* timeout */
	else
	   rc = waitinginput;
    }

    if (rc < 0)
	return -1;  /* error */
    if (rc == 0)
	return -2;  /* timeout */
    rc = read(fileno(f), &c, 1);
    return rc == 1 ? c : -1;
}
#endif /* FIONREAD */
