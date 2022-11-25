/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/tryrec.h,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:35:03 $
 */
/* @(#) $Revision: 1.4.83.3 $ */       

#ifndef _MACHINE_TRYREC_INCLUDED /* allows multiple inclusion */
#define _MACHINE_TRYREC_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _WSIO
#ifdef __hp9000s300
struct C__trystuff {
	long	pctr;
	long	link;
	long	regs[12];
	};
#endif /* __hp9000s300 */
#ifdef __hp9000s800
struct C__trystuff {
	long	rp;
	long	sp;
	long	dp;
	long	ss;
	long	link;
	long	regs[16];
	double  fpregs[4];
	};
#endif /* __hp9000s800 */

#define try if (C__tst()){struct C__trystuff T__R; C__try(&T__R);
#define recover C__rec(&T__R);} else 

#define	escapecode u.u_pcb.pcb_escapecode
#endif /* _WSIO */
#endif /* ! _MACHINE_TRYREC_INCLUDED */
