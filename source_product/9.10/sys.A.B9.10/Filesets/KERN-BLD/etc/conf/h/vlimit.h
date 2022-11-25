/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/vlimit.h,v $
 * $Revision: 1.8.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:38:46 $
 */
/* HPUX_ID: @(#)vlimit.h	52.2		88/04/28 */
#ifndef _SYS_VLIMIT_INCLUDED /* allows multiple inclusion */
#define _SYS_VLIMIT_INCLUDED

/*
 * Limits for u.u_limit[i], per process, inherited.
 */
#define	LIM_NORAISE	0	/* if <> 0, can't raise limits */
#define	LIM_CPU		1	/* max secs cpu time */
#define	LIM_FSIZE	2	/* max size of file created */
#define	LIM_DATA	3	/* max growth of data space */
#define	LIM_STACK	4	/* max growth of stack */
#define	LIM_CORE	5	/* max size of ``core'' file */
#define	LIM_MAXRSS	6	/* max desired data+stack core usage */

#define	NLIMITS		6

#define	INFINITY	0x7fffffff
#endif /* _SYS_VLIMIT_INCLUDED */
