/*
 * @(#)unsp.h: $Revision: 1.4.83.4 $ $Date: 93/12/09 11:48:09 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */

/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/

#ifndef _SYS_UNSP_INCLUDED /* allows multiple inclusion */
#define _SYS_UNSP_INCLUDED

/*
 * This file contains definitions and declarations related to user
 * cluster server process (UCSPs or user CSPs).  Within the
 * implementation they are also known as user network server process
 * (UNSPs or user NSPs), and the terms are used interchangeably.
 */

#define	UNSP_SIG	SIGVTALRM	/* request to spawn a user NSP */


/* structures for recording user NSP Info */

#define MAX_UNSP	16	/* maximum concurrent user NPSs */

#ifdef _KERNEL

/* kernel structure representing user NSP */

struct unsp
{
	int un_flags;		/* see below */
	dm_message un_request;	/* a pointer to the request */
	dm_message un_reply;	/* a pointer to the reply */
	int un_timeouts;	/* number of timeouts */
	site_t un_site;		/* site requiring unsp */
	struct proc *un_proc;	/* proc ptr (for sending signals) */
	union			/* a unique identifier */
	{
		u_int unu_id;	/* treated either as an integer */
		struct		/* or as two fields, consisting of: */
		{
			u_int unu_uniq:16;	/*and an incrementing number*/
			u_int unu_slot:16;	/*the slot number*/
		} unu_fields;
	}unu;
#define un_id		unu.unu_id
#define un_slot		unu.unu_fields.unu_slot
#define un_unique	unu.unu_fields.unu_uniq
} unsps[MAX_UNSP];

#define MAX_UNSP_TIMEOUT 6	/*maximum retries on forking unsp*/

#endif /* _KERNEL */

/*ioctl commands*/

#define UNSP_GETOP	_IO('U',0)
#define UNSP_GETSITE	_IO('U',1)
#define UNSP_SETFL	_IO('U',2)
#define UNSP_GETFL	_IO('U',3)	/* NOT IMPLEMENTED */
#define UNSP_REPLY	_IO('U',4)
#define UNSP_CHECK_REPLY _IO('U',5)
#define UNSP_ERROR	_IO('U',6)
#define UNSP_READ	_IOR('U',7,unsp_msg)
#define UNSP_WRITE	_IOW('U',8,unsp_msg)
#define UNSP_REPLY_CNTL	_IO('U',9)



/*flags settable by user through ioctl*/
#define UNSP_RREAD	0x01	/* send reply when read done */
#define UNSP_RWRITE	0x02	/* send reply when write done */
#define UNSP_RIREAD	0x04	/* send reply when ioctl read */
#define UNSP_RIWRITE	0x08	/* send reply when ioctl write */

/*flags used in reply control command*/
#define UNSP_EOSYS_RESTART	0x01	/*set u.u_eosys to restartsys*/
#define UNSP_LONGJMP	0x02	/*set DM_LONGJMP flag*/

#ifdef _KERNEL
/*flags used internally by kernel*/
#define UNSP_UFLAGS	(UNSP_RREAD|UNSP_RWRITE|UNSP_RIREAD|UNSP_RIWRITE)
				/* kernel mask for setting user flags */

#define UNSP_IN_USE	0x00100	/* This request structure is in use */
#define UNSP_HAS_PROCESS 0x00200 /* A process has picked up this request */
#define UNSP_DEAD	0x00400	/* The requesting site has died, ... */
#define UNSP_UWAITING	0x00800	/* User NSP waiting for the request */
#define UNSP_WAKE_REPLY	0x01000	/* use wakeup instead of sending response */
#define UNSP_SIGNAL	0x02000	/* A signal came in before the process
				 * could be created.  Send a signal as
				 * soon as possible */

/*
 * WARNING - the constant UNSP_PARM_SIZE is dependent on the sizes of
 *	     struct dux_mbuf in dux_mbuf.h and struct dm_header in dm.h.
 *	     It should be less than or equal to the diffeence between
 *	     those sizes, and must thus track any changes in the
 *	     declarations of those structures or any components thereof.
 */
#endif	/* _KERNEL */

/*array for transferring info to and from mbuf header*/

#define UNSP_PARM_SIZE 44	/* Maximum size of transferred data */

typedef char unsp_msg[UNSP_PARM_SIZE];

#ifdef _KERNEL
struct unsp_message		/*DUX MESSAGE STRUCTURE*/
{
	u_int um_id;		/*id number*/
	unsp_msg um_msg;	/*user data*/
};
#endif /* _KERNEL */

#endif /* _SYS_UNSP_INCLUDED */
