/* @(#) $Revision: 66.1 $ */       
#ifndef _TRYREC_INCLUDED /* allows multiple inclusion */
#define _TRYREC_INCLUDED
#ifdef __hp9000s300

struct C__trystuff {
	long	pctr;
	long	link;
	long	regs[12];
	};

#define try if (C__tst()){struct C__trystuff T__R; C__try(&T__R);
#define recover C__rec(&T__R);} else 

#define	escapecode u.u_pcb.pcb_escapecode
#endif /* __hp9000s300 */
#endif /* _TRYREC_INCLUDED */
