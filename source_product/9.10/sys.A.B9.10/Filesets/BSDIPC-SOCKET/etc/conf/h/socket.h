/* $Header: socket.h,v 1.28.83.4 93/09/17 18:34:53 kcs Exp $ */

#ifndef _SYS_SOCKET_INCLUDED    /* allow multiple includes of this file */
#define _SYS_SOCKET_INCLUDED    /* without causing compilation errors */

/*
 * Copyright (c) 1982, 1985, 1986 Regents of the University of California.
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
 *	@(#)socket.h	7.3 (Berkeley) 6/27/88
 */

/*
 * Definitions related to sockets: types, address families, options.
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/*
 * Types
 */

#ifndef _CADDR_T
#  define _CADDR_T
   typedef char *caddr_t;
#endif /* _CADDR_T */

/*
 * Types of sockets
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#define	SOCK_RDM	4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER	0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	0x1001		/* send buffer size */
#define SO_RCVBUF	0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */
#define	SO_SND_COPYAVOID	0x1009		/* avoid copy on send*/
#define	SO_RCV_COPYAVOID	0x100a		/* avoid copy on rcv */

/*
 * Structure used for manipulating linger option.
 *
 * if l_onoff == 0:
 *    close(2) returns immediately; any buffered data is sent later
 *    (default)
 * 
 * if l_onoff != 0:
 *    if l_linger == 0, close(2) returns after discarding any unsent data
 *    if l_linger != 0, close(2) does not return until buffered data is sent
 */

struct	linger {
	int	l_onoff;		/* 0 = do not wait to send data */
					/* non-0 = see l_linger         */
	int	l_linger;		/* 0 = discard unsent data      */
					/* non-0 = wait to send data    */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#define	AF_UNIX		1		/* local to host (pipes, portals) */
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NS		6		/* XEROX NS protocols */
#define	AF_NBS		7		/* nbs protocols */
#define	AF_ECMA		8		/* european computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define	AF_APPLETALK	16		/* Apple Talk */
#define AF_OTS		17		/* Used for OSI in the ifnets */
#define	AF_NIT		18		/* NIT */

#define	AF_MAX		19

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	unsigned short sa_family;	/* address family */
	char           sa_data[14];	/* up to 14 bytes of direct address */
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	unsigned short	sp_family;	/* address family */
	unsigned short	sp_protocol;	/* protocol */
};

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_UNIX		AF_UNIX
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NS		AF_NS
#define	PF_NBS		AF_NBS
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK

#define	PF_MAX		AF_MAX

/*
 * Maximum queue length specifiable by listen.
 */
#define	SOMAXCONN	20

/*
 * Message header for recvmsg and sendmsg calls.
 */
struct msghdr {
	caddr_t	msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};

#define	MSG_OOB		0x1		/* process out-of-band data */
#define	MSG_PEEK	0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */

#ifdef _KERNEL
#define MSG_EOR         0x8             /* data completes record */
#define MSG_TRUNC       0x10            /* data discarded before delivery */
#define MSG_CTRUNC      0x20            /* control data lost before delivery */
#define MSG_WAITALL     0x40            /* wait for full request or error */

/* Following used within kernel */
#define MSG_MBUF        0x1000          /* data in kernel, skip uiomove */
#define MSG_NONBLOCK    0x4000          /* nonblocking request */
#define MSG_COMPAT      0x8000          /* 4.3-format sockaddr */
#endif  /* _KERNEL */

#define	MSG_MAXIOVLEN	16


/* BSDIPC system calls */

#ifndef _KERNEL
#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
     extern int accept(int, void *, int *);
     extern int bind(int, const void *, int);
     extern int connect(int, const void *, int);
     extern int getpeername(int, void *, int *);
     extern int getsockname(int, void *, int *);
     extern int getsockopt(int, int, int, void *, int *);
     extern int listen(int, int);
     extern int rcmd(char **, unsigned short, const char *,
				     const char *, const char *, int *);
     extern int recv(int, void *, int, int);
     extern int recvfrom(int, void *, int, int, void *, int *);
     extern int recvmsg(int, struct msghdr msg[], int);
     extern int rexec(char **, int, const char *,
				     const char *, const char *, int *);
     extern int rresvport(int *);
     extern int ruserok(const char *, int, const char *, const char *);
     extern int send(int, const void *, int, int);
     extern int sendto(int, const void *, int, int, const void *, int);
     extern int sendmsg(int, const struct msghdr msg[], int);
     extern int setsockopt(int, int, int, const void *, int);
     extern int shutdown(int, int);
     extern int socket(int, int, int);
     extern int socketpair(int, int, int, int[2]);
#else /* not _PROTOTYPES */
     extern int accept();
     extern int bind();
     extern int connect();
     extern int getpeername();
     extern int getsockname();
     extern int getsockopt();
     extern int listen();
     extern int rcmd();		/* belongs here or elsewhere? */
     extern int recv();
     extern int recvfrom();
     extern int recvmsg();
     extern int rexec();	/* belongs here or elsewhere? */
     extern int rresvport();	/* belongs here or elsewhere? */
     extern int ruserok();	/* belongs here or elsewhere? */
     extern int send();
     extern int sendto();
     extern int sendmsg();
     extern int setsockopt();
     extern int shutdown();
     extern int socket();
     extern int socketpair();
#endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#endif /* not _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif	/* not _SYS_SOCKET_INCLUDED */
