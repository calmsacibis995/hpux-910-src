/*
 * @(#)sleep.h: $Revision: 1.4.83.4 $ $Date: 93/09/17 18:34:47 $
 * $Locker:  $
 */

#ifndef	_SLEEP_INCLUDED
#define	_SLEEP_INCLUDED

/************************
 * Sleep.h - Parameter and sleep hash queue
 *           definitions for use in kern_synch.c
 *           and sys_gen.c
 ************************/

extern int SQSIZE;
extern int SQMASK;

#ifdef MP
extern sema_t slpsem[];
#else
extern struct proc *slpque[];
#ifdef __hp9000s800
extern struct proc *slptl[];	/* For FIFO sleep queues */
#endif
#endif 

#define HASH(x)	(( (int) x >> 5) & SQMASK)

#endif	/* ! _SLEEP_INCLUDED */
