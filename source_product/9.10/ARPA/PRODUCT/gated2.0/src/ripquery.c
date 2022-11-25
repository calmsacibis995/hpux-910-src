/*
 *  $Header: ripquery.c,v 1.1.109.5 92/02/28 16:01:24 ash Exp $
 */

/*%Copyright%*/
/************************************************************************
*									*
*	GateD, Release 2						*
*									*
*	Copyright (c) 1990,1991,1992 by Cornell University		*
*	    All rights reserved.					*
*									*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY		*
*	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT		*
*	LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY		*
*	AND FITNESS FOR A PARTICULAR PURPOSE.				*
*									*
*	Royalty-free licenses to redistribute GateD Release		*
*	2 in whole or in part may be obtained by writing to:		*
*									*
*	    GateDaemon Project						*
*	    Information Technologies/Network Resources			*
*	    143 Caldwell Hall						*
*	    Cornell University						*
*	    Ithaca, NY 14853-2602					*
*									*
*	GateD is based on Kirton's EGP, UC Berkeley's routing		*
*	daemon	 (routed), and DCN's HELLO routing Protocol.		*
*	Development of Release 2 has been supported by the		*
*	National Science Foundation.					*
*									*
*	Please forward bug fixes, enhancements and questions to the	*
*	gated mailing list: gated-people@gated.cornell.edu.		*
*									*
*	Authors:							*
*									*
*		Jeffrey C Honig <jch@gated.cornell.edu>			*
*		Scott W Brim <swb@gated.cornell.edu>			*
*									*
*************************************************************************
*									*
*      Portions of this software may fall under the following		*
*      copyrights:							*
*									*
*	Copyright (c) 1988 Regents of the University of California.	*
*	All rights reserved.						*
*									*
*	Redistribution and use in source and binary forms are		*
*	permitted provided that the above copyright notice and		*
*	this paragraph are duplicated in all such forms and that	*
*	any documentation, advertising materials, and other		*
*	materials related to such distribution and use			*
*	acknowledge that the software was developed by the		*
*	University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote		*
*	products derived from this software without specific		*
*	prior written permission.  THIS SOFTWARE IS PROVIDED		*
*	``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,	*
*	INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF	*
*	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.		*
*									*
************************************************************************/


#ifndef	__STDC__
#define	const
#endif				/* __STDC__ */

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#ifndef	HPUX7_X
#include <arpa/inet.h>
#endif				/* HPUX7_X */
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#define	RIPCMDS
#include "routed.h"

/* macros to select internet address given pointer to a struct sockaddr */

/* result is u_long */
#define sock_inaddr(x) (((struct sockaddr_in *)(x))->sin_addr)

/* result is struct in_addr */
#define in_addr_ofs(x) (((struct sockaddr_in *)(x))->sin_addr)

/* Calculate the natural netmask for a given network */
#define	inet_netmask(net) (IN_CLASSA(net) ? IN_CLASSA_NET :\
			 (IN_CLASSB(net) ? IN_CLASSB_NET :\
			  (IN_CLASSC(net) ? IN_CLASSC_NET : 0)))

#ifdef vax11c
#define perror(s) vmsperror(s)
#endif				/* vax11c */

#define WTIME	5			/* Time to wait for responses */
#define STIME	1			/* Time to wait between packets */

int s;
char packet[MAXPACKETSIZE];
int cmds_request[] =
{RIPCMD_REQUEST, RIPCMD_POLL, 0};
int cmds_poll[] =
{RIPCMD_POLL, RIPCMD_REQUEST, 0};
int *cmds = cmds_poll;
int nflag;
int dflag;
const char *version = "$Revision: 1.1.109.5 $";

extern int errno;
extern char *optarg;
extern int optind, opterr;
extern char *inet_ntoa();


/*
 * Return the possible subnetwork number from an internet address.
 * SHOULD FIND OUT WHETHER THIS IS A LOCAL NETWORK BEFORE LOOKING
 * INSIDE OF THE HOST PART.  We can only believe this if we have other
 * information (e.g., we can find a name for this number).
 */
long
inet_subnetof(in)
struct in_addr in;
{
    register long i = ntohl(in.s_addr);

    if (IN_CLASSA(i))
	return (i & IN_CLASSB_NET);
    else if (IN_CLASSB(i))
	return (i & IN_CLASSC_NET);
    else
	return (i & 0xffffffc0);
}


/*
 *	Trace RIP packets
 */
void
rip_trace(dir, who, cp, size)
struct sockaddr_in *who;		/* should be sockaddr */
char *dir;
caddr_t cp;
register int size;
{
    register struct rip *rpmsg = (struct rip *) cp;
    register struct netinfo *n;
    register const char *cmd = "Invalid";

    if (rpmsg->rip_cmd && rpmsg->rip_cmd < RIPCMD_MAX) {
	cmd = ripcmds[rpmsg->rip_cmd];
    }
    (void) fprintf(stderr, "RIP %s %s.%d vers %d, cmd %s, length %d",
		   dir,
		   inet_ntoa(who->sin_addr),
		   ntohs(who->sin_port),
		   rpmsg->rip_vers,
		   cmd, size);
    switch (rpmsg->rip_cmd) {
#ifdef	RIPCMD_POLL
	case RIPCMD_POLL:
#endif				/* RIPCMD_POLL */
	case RIPCMD_REQUEST:
	case RIPCMD_RESPONSE:
	    (void) fprintf(stderr, "\n");
	    size -= 4 * sizeof(char);
	    n = rpmsg->rip_nets;
	    for (; size > 0; n++, size -= sizeof(struct netinfo)) {
		if (size < sizeof(struct netinfo)) {
		    break;
		}
		(void) fprintf(stderr, "\tnet %-15s  metric %2d  size %d\n", inet_ntoa(sock_inaddr(&n->rip_dst)),
			       ntohl((u_long) n->rip_metric), size);
	    }
	    (void) fprintf(stderr, "RIP %s end of packet", dir);
	    break;
	case RIPCMD_TRACEON:
	    (void) fprintf(stderr, ", file %*s", size, rpmsg->rip_tracefile);
	    break;
#ifdef	RIPCMD_POLLENTRY
	case RIPCMD_POLLENTRY:
	    n = rpmsg->rip_nets;
	    (void) fprintf(stderr, ", net %s", inet_ntoa(sock_inaddr(&n->rip_dst)));
	    break;
#endif				/* RIPCMD_POLLENTRY */
	default:
	    (void) fprintf(stderr, "\n");
	    break;
    }
    (void) fprintf(stderr, "\n");
}


void
query(host, cmd)
char *host;
int cmd;
{
    struct sockaddr_in router;
    register struct rip *msg = (struct rip *) packet;
    struct hostent *hp;
    struct servent *sp;

    bzero((char *) &router, sizeof(router));
    hp = gethostbyname(host);
    if (hp == 0) {
	router.sin_addr.s_addr = inet_addr(host);
	if (router.sin_addr.s_addr == (u_long) - 1) {
	    (void) printf("%s: unknown\n", host);
	    exit(1);
	}
    } else {
	(void) memcpy((char *) &router.sin_addr, hp->h_addr, hp->h_length);
    }
    sp = getservbyname("router", "udp");
    if (sp == NULL) {
	(void) fprintf(stderr, "No service for router available\n");
	exit(1);
    }
    router.sin_family = AF_INET;
    router.sin_port = sp->s_port;
    msg->rip_cmd = cmd;
    msg->rip_vers = RIPVERSION;
    msg->rip_nets[0].rip_dst.rip_family = htons(AF_UNSPEC);
    msg->rip_nets[0].rip_metric = htonl((u_long) HOPCNT_INFINITY);

    if (dflag) {
	rip_trace("SEND", &router, (caddr_t) msg, sizeof(struct rip));
    }
    if (sendto(s, packet, sizeof(struct rip), 0, (struct sockaddr *) & router, sizeof(router)) < 0)
	perror(host);
}


/*
 * Handle an incoming routing packet.
 */
void
rip_input(from, size)
struct sockaddr_in *from;
int size;
{
    register struct rip *msg = (struct rip *) packet;
    struct netinfo *n;
    const char *name;
    u_long lna, net, subnet;
    struct hostent *hp;
    struct netent *np;

    if (dflag) {
	rip_trace("RECV", from, (caddr_t) msg, size);
    }
    if (msg->rip_cmd != RIPCMD_RESPONSE)
	return;
    hp = gethostbyaddr((char *) &from->sin_addr, sizeof(struct in_addr), AF_INET);
    name = hp == 0 ? "???" : hp->h_name;
    (void) printf("%d bytes from %s(%s):\n", size, name, inet_ntoa(from->sin_addr));
    size -= sizeof(int);
    n = msg->rip_nets;
    while (size > 0) {
	register struct sockaddr_in *sin;
	int i;

	if (size < sizeof(struct netinfo))
	    break;
	if (msg->rip_vers) {
	    n->rip_dst.rip_family = ntohs(n->rip_dst.rip_family);
	    n->rip_metric = ntohl((u_long) n->rip_metric);
	}
	sin = (struct sockaddr_in *) & n->rip_dst;
	if (sin->sin_port) {
	    (void) printf("**Non-zero port (%d) **",
			  sin->sin_port & 0xFFFF);
	}
	for (i = 0; i < 8; i++)
	    if (n->rip_dst.rip_zero2[i]) {
		(void) printf("sockaddr = ");
		for (i = 0; i < 8; i++)
		    (void) printf("%d ", n->rip_dst.rip_zero2[i] & 0xFF);
		break;
	    }
	net = inet_netmask(ntohl(sin->sin_addr.s_addr)) & ntohl(sin->sin_addr.s_addr);
	subnet = inet_subnetof(sin->sin_addr);
	lna = inet_lnaof(sin->sin_addr);
	name = "???";
	if (!nflag) {
	    if (net == 0) {
		name = "default";
	    } else if (lna == INADDR_ANY) {
		net = IN_CLASSA(net) ? net >> IN_CLASSA_NSHIFT :
		    IN_CLASSB(net) ? net >> IN_CLASSB_NSHIFT :
		    net >> IN_CLASSC_NSHIFT;
		np = getnetbyaddr(net, AF_INET);
		if (np)
		    name = np->n_name;
	    } else if (sin->sin_addr.s_addr == htonl(subnet)) {
		subnet = IN_CLASSA(subnet) ? subnet >> IN_CLASSB_NSHIFT :
		    IN_CLASSB(subnet) ? subnet >> IN_CLASSC_NSHIFT :
		    subnet >> 4;
		np = getnetbyaddr(subnet, AF_INET);
		if (np)
		    name = np->n_name;
	    } else {
		hp = gethostbyaddr((char *) &sin->sin_addr, sizeof(struct in_addr), AF_INET);
		if (hp)
		    name = hp->h_name;
	    }
	    (void) printf("\t%s(%s), metric %d\n", name,
			  inet_ntoa(sin->sin_addr), n->rip_metric);
	} else {
	    (void) printf("\t%s, metric %d\n",
			  inet_ntoa(sin->sin_addr), n->rip_metric);
	}
	size -= sizeof(struct netinfo), n++;
    }
}


int
main(argc, argv)
int argc;
char *argv[];
{
    int c, cc, bits, errflg = 0, *cmd = cmds;
    struct sockaddr from;
    int fromlen = sizeof(from);
    static struct timeval *wait_time, long_time =
    {WTIME, 0}, short_time =
    {STIME, 0};

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	perror("socket");
#ifdef vax11c
	exit(0x10000002);
#else				/* vax11c */
	exit(2);
#endif				/* vax11c */
    }
    while ((c = getopt(argc, argv, "dnprvw:")) != EOF) {
	switch (c) {
	    case 'd':
		dflag++;
		break;
	    case 'n':
		nflag++;
		break;
	    case 'p':
		cmd = cmds_poll;
		break;
	    case 'r':
		cmd = cmds_request;
		break;
	    case 'v':
		(void) fprintf(stderr, "RIPQuery %s\n", version);
		break;
	    case 'w':
		long_time.tv_sec = atoi(optarg);
		(void) fprintf(stderr, "Wait time set to %d\n", long_time.tv_sec);
		break;
	    case '?':
		errflg++;
		break;
	}
    }

    if (errflg || (optind >= argc)) {
	(void) printf("usage: ripquery [ -d ] [ -n ] [ -p ] [ -r ] [ -v ] [ -w time] hosts...\n");
	exit(1);
    }
    setnetent(1);

    for (; optind < argc; optind++) {
      retry:
	query(argv[optind], *cmd);
	bits = 1 << s;
	wait_time = &long_time;
	for (;;) {
#ifndef vax11c
	    cc = select(s + 1, (struct fd_set *) & bits, (struct fd_set *) 0, (struct fd_set *) 0, wait_time);
#else				/* vax11c */
	    cc = Socket_Ready(s, wait_time->tv_sec);
#endif				/* vax11c */
	    if (cc == 0) {
		if (wait_time == &short_time) {
		    break;
		}
		if (*(++cmd)) {
		    goto retry;
		} else {
		    break;
		}
	    } else if (cc < 0) {
		perror("select");
		(void) close(s);
		exit(1);
	    } else {
		wait_time = &short_time;
		cc = recvfrom(s, packet, sizeof(packet), 0, &from, &fromlen);
		if (cc <= 0) {
		    if (cc < 0) {
			if (errno == EINTR) {
			    continue;
			}
			perror("recvfrom");
			(void) close(s);
			exit(1);
		    }
		    continue;
		}
		rip_input((struct sockaddr_in *) & from, cc);
	    }
	}
    }

    endnetent();
    exit(0);
    return (0);
}



#ifdef	vax11c
/*
 *	See if a socket is ready for reading (waiting "n" seconds)
 */
static int
Socket_Ready(Socket, Wait_Time)
{
#include <vms/iodef.h>
#define EFN_1 20
#define EFN_2 21
    int Status;
    int Timeout_Delta[2];
    int Dummy;
    unsigned short int IOSB[4];

    /*
     *	Check for data (using MSG_PEEK)
     */
    Status = SYS$QIO(EFN_1,
		     Socket,
		     IO$_READVBLK,
		     IOSB,
		     0,
		     0,
		     &Dummy,
		     sizeof(Dummy),
		     MSG_PEEK,
		     0,
		     0,
		     0);
    /*
     *	Check for completion
     */
    if (IOSB[0] != 0) {
	return (1);
    }
    /*
     *	Setup timer
     */
    if (Wait_Time) {
	Timeout_Delta[0] = -(Wait_Time * 10000000);
	Timeout_Delta[1] = -1;
	SYS$SETIMR(EFN_2, Timeout_Delta, 0, Socket_Ready);
	SYS$WFLOR(EFN_1, (1 << EFN_1) | (1 << EFN_2));
	if (IOSB[0] != 0) {
	    SYS$CANTIM(Socket_Ready, 0);
	    return (1);
	}
    }
    /*
     *	Last chance
     */
    if (IOSB[0] == 0) {
	/*
	 *	Lose:
	 */
	SYS$CANCEL(Socket);
	return (0);
    }
    return (1);
}

#endif				/* vax11c */
