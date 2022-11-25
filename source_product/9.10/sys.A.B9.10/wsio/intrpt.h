/*
 * @(#)intrpt.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 20:30:01 $
 * $Locker:  $
 */

/* @(#) $Revision: 1.2.83.3 $ */       
#ifndef _MACHINE_INTRPT_INCLUDED /* allows multiple inclusion */
#define _MACHINE_INTRPT_INCLUDED

/*
**  interrupt information structure
**	WARNING: there are assembly language dependencies in locore.s
**		 that are NOT handled by the genassym.c mechanism!!!
*/
struct interrupt {
	char *regaddr;
	char mask;
	char value;
	struct interrupt *next;
	int (*isr)();
	char chainflag;
	char misc;
	int temp;
};


struct interrupt *rupttable[8];

#endif /* _MACHINE_INTRPT_INCLUDED */
