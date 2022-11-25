/*
 * $Header: include.h,v 1.1.109.6 92/02/28 15:55:52 ash Exp $
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


/* include.h
 *
 * System and EGP header files to be included.
 */

#ifdef	vax11c
#include "[.vms]gated_named.h"
#endif	/* vax11c */

#if	defined(_IBMR2) && !defined(_BSD)
#define	_BSD
#endif

#include <sys/param.h>			/* Was types */
#ifdef	_IBMR2
#include <sys/types.h>
#endif	/* _IBMR2 */
#ifdef SYSV
#include <sys/types.h>
#include <sys/bsdtypes.h>
#include <sys/stream.h>
#include <sys/sioctl.h>
#endif
#ifdef	vax11c
#include <sys/ttychars.h>
#include <sys/ttydev.h>
#endif				/* vax11c */
#include <sys/uio.h>

#include <sys/socket.h>
#ifdef	SYSV
#undef	SO_RCVBUF
#endif	/* SYSV */

#ifdef	AF_LINK
#include <net/if_dl.h>
#endif				/* AF_LINK */

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#ifndef	HPUX7_X
#include <arpa/inet.h>
#endif	/* HPUX7_X */

#if	BSD > 43
#include <netiso/iso.h>
#endif

#include <stdio.h>
#include <netdb.h>
#include <sys/errno.h>
#ifdef	SYSV
#undef ENAMETOOLONG
#undef ENOTEMPTY
#include <net/errno.h>
#include <string.h>
#else				/* SYSV */
#ifdef hpux
#include <string.h>
#else
#include <strings.h>
#endif 		/* hpux */
#endif				/* SYSV */
#include <memory.h>

#ifdef vax11c
#define DONT_INCLUDE_IF_ARP
#endif				/* vax11c */
#include <net/if.h>
#ifdef	ROUTE_KERNEL
#define	KERNEL
#endif				/* ROUTE_KERNEL */
#include <net/route.h>
#undef	KERNEL

#include "config.h"

#if	defined(AIX)
#include <sys/syslog.h>
#else				/* defined(AIX) */
#include <syslog.h>
#endif				/* defined(AIX) */

#ifdef	STDARG
#include <stdarg.h>
#else				/* STDARG */
#include <varargs.h>
#endif				/* STDARG */

#include "defs.h"
#include "inet.h"
#include "rt_control.h"
#include "if.h"
#include "task.h"
#include "rt_table.h"
#include "trace.h"
#ifdef	notdef
#include "unix.h"
#endif				/* notdef */
