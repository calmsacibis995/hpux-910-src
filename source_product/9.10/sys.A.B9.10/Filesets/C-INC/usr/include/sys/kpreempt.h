/*
 * @(#)kpreempt.h: $Revision: 1.28.83.4 $ $Date: 93/09/17 18:28:09 $
 * $Locker:  $
 * 
 */

#ifndef _SYS_KPREEMPT_INCLUDED
#define _SYS_KPREEMPT_INCLUDED

/* Use Series 300 defines to NOP the preemption points */

#define KPREEMPTPOINT()
#define IFKPREEMPTPOINT()       0
#define preemptkernel()		0
#define CHECKKPREEMPT()		0

#endif /* not _SYS_KPREEMPT_INCLUDED */
