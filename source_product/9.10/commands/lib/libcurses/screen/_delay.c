/* @(#) $Revision: 66.2 $ */   
/* Copyright (c) 1979 Regents of the University of California */
#include <ctype.h>
#include "curses.h"
#include <termio.h>
#include <term.h>
#ifdef NONSTANDARD
# include "ns_curses.h"
#endif

/*
 * The following array gives the number of tens of milliseconds per
 * character for each speed as returned by ioctl.  Thus since 300
 * baud returns a 7, there are 33.3 milliseconds per char at 300 baud.
 */
static
short	tmspc10[] = {
	0,	/* 0 baud */
	2000,	/* 50 baud */
	1333,	/* 75 baud */
	909,	/* 110 baud */
	743,	/* 134.5 baud */
	666,	/* 150 baud */
	500,	/* 200 baud */
	333,	/* 300 baud */
	166,	/* 600 baud */
	111,	/* 900 baud */
	83,	/* 1200 baud */
	55,	/* 1800 baud */
	41,	/* 2400 baud */
	28,	/* 3600 baud */
	20,	/* 4800 baud */
	14,	/* 7200 baud */
	10,	/* 9600 baud */
	5,	/* 19200 baud */
	2	/* 38400 baud */
};

/*
 * Insert a delay into the output stream for "delay/10" milliseconds.
 * Round up by a half a character frame, and then do the delay.
 * Too bad there are no user program accessible programmed delays.
 * Transmitting pad characters slows many terminals down and also
 * loads the system.
 */
_delay(delay, outc)
register int delay;
int (*outc)();
{
	register int mspc10;
	register int pc;
	register int outspeed;

#ifndef 	NONSTANDARD
# ifdef USG
	outspeed = cur_term->Nttyb.c_cflag&CBAUD;
# else
	outspeed = cur_term->Nttyb.sg_ospeed;
# endif
#else		NONSTANDARD
	outspeed = outputspeed(cur_term);
#endif		NONSTANDARD
	if (outspeed <= 0 || outspeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return ERR;

	mspc10 = tmspc10[outspeed];
	delay += mspc10 / 2;
	if (pad_char)
		pc = *pad_char;
	else
		pc = 0;
	for (delay /= mspc10; delay > 0; delay--)
		(*outc)(pc);
	return OK;
}
