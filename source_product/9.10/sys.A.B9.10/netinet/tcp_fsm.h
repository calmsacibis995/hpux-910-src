/* $Header: tcp_fsm.h,v 1.11.83.4 93/09/17 19:05:29 kcs Exp $ */

#ifndef	_SYS_TCP_FSM_INCLUDED
#define	_SYS_TCP_FSM_INCLUDED
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
 *	@(#)tcp_fsm.h	7.3 (Berkeley) 6/29/88
 */

/*
 * TCP FSM state definitions.
 * Per RFC793, September, 1981.
 */

#define	TCP_NSTATES	12

#define	TCPS_CLOSED		0	/* closed */
#define	TCPS_LISTEN		1	/* listening for connection */
#define	TCPS_SYN_SENT		2	/* active, have sent syn */
#define	TCPS_SYN_HOLD		3	/* have received syn (from listen) */
#define	TCPS_SYN_RECEIVED	4	/* have send and received syn */
/* states < TCPS_ESTABLISHED are those where connections not established */
#define	TCPS_ESTABLISHED	5	/* established */
#define	TCPS_CLOSE_WAIT		6	/* rcvd fin, waiting for close */
/* states > TCPS_CLOSE_WAIT are those where user has closed */
#define	TCPS_FIN_WAIT_1		7	/* have closed, sent fin */
#define	TCPS_CLOSING		8	/* closed xchd FIN; await FIN ACK */
#define	TCPS_LAST_ACK		9	/* had fin and close; await FIN ACK */
/* states > TCPS_CLOSE_WAIT && < TCPS_FIN_WAIT_2 await ACK of FIN */
#define	TCPS_FIN_WAIT_2		10	/* have closed, fin is acked */
#define	TCPS_TIME_WAIT		11	/* in 2*msl quiet wait after close */


#define	TCPS_HAVEHELDSYN(s)	((s) >= TCPS_SYN_HOLD)
#define	TCPS_HAVERCVDSYN(s)	((s) >= TCPS_SYN_RECEIVED)
#define	TCPS_HAVERCVDFIN(s)	((s) >= TCPS_TIME_WAIT)

#ifdef	TCPOUTFLAGS
/*
 * Flags used when sending segments in tcp_output.
 * Basic flags (TH_RST,TH_ACK,TH_SYN,TH_FIN) are totally
 * determined by state, with the proviso that TH_FIN is sent only
 * if all data queued for output is included in the segment.
 */
u_char	tcp_outflags[TCP_NSTATES] = {
    TH_RST|TH_ACK, 0, TH_SYN, TH_RST|TH_ACK, TH_SYN|TH_ACK,
    TH_ACK, TH_ACK,
    TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_FIN|TH_ACK, TH_ACK, TH_ACK,
};
#endif

#ifdef KPROF
int	tcp_acounts[TCP_NSTATES][PRU_NREQ];
#endif

#ifdef	TCPSTATES
char *tcpstates[] = {
	"CLOSED",	"LISTEN",	"SYN_SENT",	"SYN_HOLD",
	"SYN_RCVD",
	"ESTABLISHED",	"CLOSE_WAIT",	"FIN_WAIT_1",	"CLOSING",
	"LAST_ACK",	"FIN_WAIT_2",	"TIME_WAIT",
};
#endif
#endif	/* _SYS_TCP_FSM_INCLUDED */
