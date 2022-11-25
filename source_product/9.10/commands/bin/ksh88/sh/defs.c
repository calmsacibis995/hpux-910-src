/* HPUX_ID: @(#) $Revision: 72.2 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/*
 * Ksh - AT&T Bell Laboratories
 * Written by David Korn
 * This file defines all the  read/write shell global variables
 */

#include	"defs.h"
#include	"jobs.h"
#include	"sym.h"
#include	"history.h"
#include	"edit.h"
#include	"timeout.h"


struct sh_scoped	st;
struct sh_static	sh;

#ifdef VSH
    struct	edit	editb;
#else
#   ifdef ESH
	struct	edit	editb;
#   endif /* ESH */
#endif	/* VSH */

struct history	*hist_ptr;
struct jobs	job;
int		jobtty=JOBTTY;		/* current tty is using: 0, 1, or 2 */
int             winchanged=0;
int		sh_lastbase = 10; 
time_t		sh_mailchk = 600;
#ifdef TIMEOUT
    long		sh_timeout = TIMEOUT;
#else
    long		sh_timeout = 0;
#endif /* TIMEOUT */
char		io_tmpname[] = "/tmp/shxxxxxx.aaa";

#ifdef 	NOBUF
    char	_sibuf[IOBSIZE+1];
    char	_sobuf[IOBSIZE+1];
#endif	/* NOBUF */

struct fileblk io_stdin = { _sibuf, _sibuf, _sibuf, 0, IOREAD, 0, F_ISFILE};
struct fileblk io_stdout = { _sobuf, _sobuf, _sobuf+IOBSIZE, 0, IOWRT,2};
struct fileblk *io_ftable[NFILE] = { 0, &io_stdout, &io_stdout};

#ifdef MULTIBYTE
/*
 * These are default values.  They can be changed with CSWIDTH
 */

char int_charsize[] =
{
	1, CCS1_IN_SIZE, CCS2_IN_SIZE, CCS3_IN_SIZE,	/* input sizes */
	1, CCS1_OUT_SIZE, CCS2_OUT_SIZE, CCS3_OUT_SIZE	/* output widths */
};
#else
char int_charsize[] =
{
	1, 0, 0, 0,	/* input sizes */
	1, 0, 0, 0	/* output widths */
};
#endif /* MULTIBYTE */

/*
 *  Since job_init gets done *after* the $ENV file, and it turns
 *  on the monitor option, we use env_monitor to tell us when
 *  a set +m (turn off job control) has been done in $ENV so that we
 *  don't immediately turn it on again in job_init.
 *  Values used by env_monitor are defined in defs.h
 */

int env_monitor = 0;

/*
 *  This is just a collection of flags that
 *  we have added for various reasons.  See the
 *  defines in defs.h for more details.
 */

unsigned long hp_flags1 = 0;


#ifdef HP_DEBUG
int hp_debug=0;
#endif /* HP_DEBUG */

#ifdef HP_BUILTIN
int hp_builtin=0;
#endif /* HP_BUILTIN */
