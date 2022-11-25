/*
 *  $Header: ext.c,v 1.1.109.5 92/02/28 14:02:11 ash Exp $
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


#include "include.h"
#include "egp.h"


/*
 * AS specific variables
 */
as_t my_system;				/* My autonomous system */

/*
 * Miscellaneous variables.
 */
char *my_name;				/* name we were invoked as */
char *version_kernel;			/* OS version of the kernel */
char *my_hostname;			/* Hostname of this system */
int my_pid;				/* my process ID */
int my_mpid;				/* process ID of main process */
int install = TRUE;			/* if TRUE install route in kernel */
struct gtime gated_time;		/* Current time of day */

int test_flag;				/* Just testing configuration */

#ifndef	vax11c
const char *Gated_Configuration_File = INITFILE;	/* the configuration file */

#endif				/* vax11c */

/*
 * HELLO protocol, default route specification.
 */
struct sockaddr_in default_net;

/*
 *	Names for the various bits
 */


/*
 *	I/O structures
 */

struct ip recv_ip;			/* Received IP packet */
sockaddr_un recv_addr;			/* Source address of this packet */
struct iovec recv_iovec[RECV_IOVEC_SIZE] =
{
    {(caddr_t) & recv_ip, sizeof(recv_ip)},	/* IP address for RAW protocols */
    {NULL, 0}				/* Pointer to receive buffer */
};
struct msghdr recv_msghdr =
{
    (caddr_t) & recv_addr, sizeof(recv_addr),	/* Address and length of received address */
    recv_iovec, RECV_IOVEC_SIZE,	/* Address and length of buffer - changed at runtime */
    NULL, 0				/* Address and length of access rights */
};
