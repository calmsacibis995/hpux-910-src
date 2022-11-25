/* $Header: in_pcb.h,v 1.14.83.4 93/09/17 19:02:55 kcs Exp $ */

#ifndef	_SYS_IN_PCB_INCLUDED
#define	_SYS_IN_PCB_INCLUDED
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)in_pcb.h	7.3 (Berkeley) 6/29/88 plus MULTICAST 1.0
 */

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
struct inpcb {
	struct	inpcb *inp_next,*inp_prev;
					/* pointers to other pcb's */
	struct	inpcb *inp_nexth,*inp_prevh;
					/* pointers to hashed queue */
	struct	inpcb *inp_head;	/* pointer back to chain of inpcb's
					   for this protocol */
	struct	in_addr inp_faddr;	/* foreign host table entry */
	struct	in_addr inp_laddr;	/* local host table entry */
	u_short	inp_fport;		/* foreign port */
	u_short	inp_lport;		/* local port */
	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	struct	route inp_route;	/* placeholder for routing entry */
	struct	mbuf *inp_options;	/* IP options */
	struct	mbuf *inp_moptions;	/* IP multicast options */
	u_char	inp_ttl;		/* default ttl for sending */
};

#define	INPLOOKUP_WILDCARD	1
#define	INPLOOKUP_SETLOCAL	2

#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)

#define	inp_socket	inp_route.ro_socket

#ifdef _KERNEL
struct	inpcb *in_pcblookup();

#define PCB_NB	32		/* # of buckets.  Must be power of 2.  */

#define PCB_HASH(faddr,fport,laddr,lport,hash) { 			\
	(hash) = (lport) ^ (fport) ^ (laddr).s_addr ^ (faddr).s_addr;	\
	(hash) = (hash) & (PCB_NB -1);					\
}

#define PCB_LOOK(table,faddr,fport,laddr,lport,hash,inp) {		\
	PCB_HASH ((faddr), (fport), (laddr), (lport), (hash));		\
	(inp) = (table)[(hash)].inp_nexth; 				\
	for (;(inp)!=(struct inpcb *)&(table)[(hash)]; 			\
			(inp)=(inp)->inp_nexth) {			\
		if ((inp)->inp_lport != (lport)) 			\
			continue; 					\
		if ((inp)->inp_fport != (fport)) 			\
			continue; 					\
		if ((inp)->inp_faddr.s_addr != (faddr).s_addr) 		\
			continue; 					\
		if ((inp)->inp_laddr.s_addr != (laddr).s_addr) 		\
			continue; 					\
		break;							\
	} 								\
	if ((inp) == (struct inpcb *) &(table)[(hash)]) 		\
		(inp) = 0;						\
}
#endif	/* _KERNEL */
#endif	/* _SYS_IN_PCB_INCLUDED */
