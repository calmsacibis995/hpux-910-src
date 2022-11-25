/* HPUX_ID: @(#) $Revision: 66.1 $ */
#ifdef HP_DEBUG
/*
 *  This stuff is for the debug options that can be turned
 *  on/off with set -D or set +D option.  See also sh/args.c.
 *
 *  At the place you want to put debugging stuff, make something
 *  like:
 *       #ifdef HP_DEBUG
 *	     if(hp_debug & DEBUG_OPTION)
 *	     {
 *		  do debugging stuff
 *	     }
 *	 #endif
 *
 *  Make sure DEBUG_OPTION is defined here and is in the
 *  debug_options SYSTAB.  Then at runtime, when you want 
 *  turn on DEBUG_OPTION, do a "set -D debug_option" and
 *  it will be turned on.  To turn it off do a "set +D debug_option".
 */

/*  These are the flags that are or'ed into hp_debug */

#define ALL_DEBUG	0x7777	/*  all the debug options  */
#define	FORK_DEBUG	0x0001	/*  xec.c in the fork code */

#define Z1_DEBUG	0x1000	/*  User defined 1 */
#define Z2_DEBUG	0x2000	/*  User defined 2 */
#define Z3_DEBUG	0x4000	/*  User defined 3 */

#endif /* HP_DEBUG */
