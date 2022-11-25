/*
 * $Header: config.h,v 1.1.109.6 92/03/05 13:38:21 ash Exp $
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


/*	Protocols to include - theoreticaly you can undef on of these and	*/
/*	that protocol will not be included.  This has not yet been tested.	*/

#define	PROTO_BGP			/* Border Gateway Protocol */
#define	PROTO_EGP			/* Exterior Gateway Protocol */
#define	PROTO_RIP			/* Routing Information Protocol */
#define	PROTO_HELLO			/* DCN HELLO Protocol */
#define	PROTO_ICMP			/* ICMP protocol support */
#undef	PROTO_OSPF			/* Open SPF */
#undef	PROTO_IGRP			/* Wishful thinking */

/* initialization file */
#ifndef vax11c
#define INITFILE	"/etc/gated.conf"
#define PIDFILE		"/etc/gated.pid"
#define	VERSIONFILE	"/etc/gated.version"
#ifndef	DUMPFILE
#define DUMPFILE	"/usr/tmp/gated_dump"	/* XXX - should be defined in Makefile */
#endif	/* DUMPFILE */
#else				/* vax11c */
#define DUMPFILE	"MULTINET:GATEWAY-DAEMON.DUMP"
#endif				/* vax11c */

#define	DUMPDIR		"/usr/tmp"

/* compiler switches */

/* How about a standard AIX define? */
#if	(defined(_AIX) || defined(__AIX)) && !defined(AIX)
#define	AIX
#endif

/* Systems that include the IP header with ICMP packets */
#if	defined(BSD4_3) || defined(SUNOS) || defined(ULTRIX3_X) || defined(ULTRIX4_X) || defined(AIX) || defined(hpux)
#define	ICMP_IP_HEADER
#endif


/* Systems that have a parameter for SO_LINGER */
#if	defined(BSD4_3) || defined(SUNOS) || defined(ULTRIX3_X) || defined(ULTRIX4_X) || defined(AIX) || defined(hpux)
#define	LINGER_PARAM
#endif


/* Systems where function prototypes work */
#if	defined(__STDC__) || defined(_IBMR2)
#define	USE_PROTOTYPES
#endif

/* Systems that define a signal as void instead of int */
#if	defined(SUNOS) || defined(ULTRIX3_X) || defined(ULTRIX4_X) || defined(SYSV) || (BSD > 43) || defined(_IBMR2) || defined(hpux)
#define	SIGTYPE	void
#define	SIGRETURN return
#endif

/* Systems that support ANSI varargs */
#if	defined(__STDC__) && !defined(__HIGHC__) && !defined(ibm032) && !(BSD > 43)
#define	STDARG
#endif

/* AIX defines __SIGVOID */
#if	!defined(SIGTYPE) && defined(__SIGVOID)
#define	SIGTYPE	__SIGVOID
#if	__SIGVOID == void
#define	SIGRETURN return
#else
#define	SIGRETURN return(0)
#endif
#endif				/* !defined(SIGTYPE) && defined(__SIGVOID) */

#if	!defined(SIGTYPE)
#define	SIGTYPE	int
#define	SIGRETURN return(0)
#endif				/* !defined(SIGTYPE) */

/* Some systems do not define sigmask (Ultrix) */
#if	!defined(sigmask)  && !defined(hpux)
#define sigmask(m)      (1 << ((m)-1))
#endif				/* !defined(sigmask) */

/* Set the correct name for /vmunix for namelist */
#ifdef	hpux
#define	UNIX_NAME "/hp-ux"
#endif				/* hp-ux */
#if	defined(AIX) || defined(SYSV)
#define	UNIX_NAME "/unix"
#endif				/* AIX */
#ifndef	UNIX_NAME
#define	UNIX_NAME "/vmunix"
#endif				/* UNIX_NAME */

/* Do kernel symbols begin with an _? */
#if	defined(hp9000s800) || defined(sun386) || (defined(AIX) && defined(i386)) || defined(_IBMR2)
#define	NLIST_NOUNDER
#endif

/* For systems that do not support fork(), process dump requests from the main process */
#if	defined(vax11c) || defined(SYSV)
#define	NO_FORK
#endif	/* defined(vax11c) || defined(SYSV) */

/* POSIX compatible waitpid() */
#if	(BSD > 43) || defined(SUNOS) || defined(hpux) || defined(ULTRIX4_X)
#define	WAIT_T	int
#else	/* (BSD > 43) || defined(SUNOS) */
#define	WAIT_T	union wait
#define	waitpid(pid, statusp, options)	wait3(statusp, options, NULL)
#ifndef	WEXITSTATUS		
#define	WEXITSTATUS(statusp)	(statusp.w_retcode)
#define	WTERMSIG(statusp)	(statusp.w_termsig)
#define	WSTOPSIG(statusp)	(statusp.w_stopsig)
#endif	/* WEXITSTATUS */
#endif	/* (BSD > 43) || defined(SUNOS) */


/* Systems that use setpgrp() instead of TIOCNOTTY */
#if	defined(SYSV) || defined(hpux)
#define	USE_SETPGRP
#endif


/* Systems that do not support recvmsg() */
#if	defined(HPUX7_X)
#define	NEED_RECVMSG
#endif


/* For HP/UX */
#ifdef HPUX7_X
/* this is ifdef'ed KERNEL in socket.h for some reason */
struct msghdr {
    caddr_t msg_name;			/* optional address */
    int msg_namelen;			/* size of address */
    struct iovec *msg_iov;		/* scatter/gather array */
    int msg_iovlen;			/* # elements in msg_iov */
    caddr_t msg_accrights;		/* access rights sent/received */
    int msg_accrightslen;
};

#endif

/* Macros for System V compatability */
#if	defined(SYSV) || defined(hpux)
#define	srandom	srand
#define	random	rand
#define	setlinebuf(x)	setvbuf(x, NULL, _IOLBF, BUFSIZ)
#define	getwd(path)	getcwd(path, sizeof (path))
#endif

#ifdef	SYSV
#define	index	strchr
#define	rindex	strrchr
#define	MAXPATHLEN	MAXNAMLEN
#endif				/* SYSV */

/* For systems that do not have FD_SET macros */

#ifndef	FD_SET
#ifndef	NBBY
#define	NBBY	8			/* number of bits in a byte */
#endif				/* NBBY */
typedef long fd_mask;

#define	NFDBITS	(sizeof(fd_mask) * NBBY)/* bits per mask */

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define	FD_ZERO(p)	memset((char *)(p), (char) 0, sizeof(*(p)))
#endif				/* FD_SET */
