/*
 * $Header: defs.h,v 1.1.109.5 92/02/28 14:01:34 ash Exp $
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


/* defs.h
 *
 * Compiler switches and miscellaneous definitions.
 */

#ifndef	__STDC__
#define	const
#endif				/* __STDC__ */

/* Common types */

typedef u_short as_t;
typedef int pref_t;
typedef u_int flag_t;
typedef u_int proto_t;
typedef u_long hash_t;
typedef u_short mtu_t;
typedef int metric_t;

typedef struct _rt_head rt_head;
typedef struct _rt_entry rt_entry;
typedef struct _task task;
typedef struct _timer timer;

typedef union {
    struct sockaddr a;
    struct sockaddr_in in;
#ifdef	ISOPROTO_RAW
    struct sockaddr_iso iso;
#endif				/* ISOPROTO_RAW */
#ifdef	AF_LINK
    struct sockaddr_dl dl;
#endif				/* AF_LINK */
} sockaddr_un;


/* Gated uses it's own version of *printf */
#define	fprintf	gd_fprintf
#define	sprintf	gd_sprintf
#define	vsprintf	gd_vsprintf

#ifndef	LOG_FACILITY
#define	LOG_FACILITY LOG_DAEMON		/* Insure that syslog facility is defined. */
#endif				/* LOG_FACILITY */

/* For compatibility BSD 4.4 and later */
#ifdef	RTM_ADD
#define	rtentry	ortentry
#define	socksize(x)	(int) (((struct sockaddr *)(x))->sa_len)
#else				/* RTM_ADD */
#define	socksize(x)	sizeof (*(x))
#endif				/* RTM_ADD */
#define	sockcopy(x, y)	memcpy((caddr_t) (y), (caddr_t) (x), socksize(x))


/* Definitions for insque() and remque() */
struct qelem {
    struct qelem *q_forw;
    struct qelem *q_back;
    char data[1];
};


extern const char *Gated_Configuration_File;

#define EGPINITFILE	Gated_Configuration_File

extern const char *version;
extern const char *build_date;
extern char *version_kernel;
extern char *my_hostname;

/* general definitions for GATED user process */

#ifndef	TRUE
#define TRUE	 1
#define FALSE	 0
#endif				/* TRUE */

#define ERROR	-1
#define NOERROR -2

#ifndef NULL
#define NULL	 0
#endif

#ifndef	INADDR_LOOPBACK
#define	INADDR_LOOPBACK	0x7f000001
#endif				/* INADDR_LOOPBACK */

#define MAXHOSTNAMELENGTH 64		/*used in init_egpngh & rt_dumb_init*/

#undef  MAXPACKETSIZE


/* Definitions for putting data into and getting data out of packets */
/* in a machine dependent manner. */
#define	PickUp(s, d)	memcpy((caddr_t)&d, (caddr_t) s, sizeof(d));	s += sizeof(d);
#define	PutDown(s, d)	memcpy((caddr_t) s, (caddr_t)&d, sizeof(d));	s += sizeof(d);
#define	PickUpStr(s, d, l)	memcpy((caddr_t) d, (caddr_t) s, l);	s += l;
#define	PutDownStr(s, d, l)	memcpy((caddr_t) s, (caddr_t) d, l);	s += l;


/* Current time definition */

struct gtime {
    time_t gt_sec;
    char gt_str[16];
    char gt_ctime[26];
};

extern struct gtime gated_time;

#define	time_sec gated_time.gt_sec
#define time_string gated_time.gt_str
#define	time_full gated_time.gt_ctime

/* external definitions */

extern char *my_name;			/* name we were invoked as */
extern int my_pid;			/* my process ID */
extern int my_mpid;			/* process ID of main process */
extern int install;			/* if TRUE install route in kernel */
extern as_t my_system;			/* My autonomous system */
extern int test_flag;			/* Just testing configuration */

extern struct sockaddr_in default_net;

/* Gated functions */

#ifdef	USE_PROTOTYPES
extern char *gd_lower(char *string);
extern void quit(int errno);
extern void getod(void);
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, int n);

#ifdef	STDARG
extern int fprintf(FILE * stream, const char *format,...);
extern int vsprintf(char *dest, const char *format, va_list * argp);
extern int sprintf(char *s, const char *format,...);

#else				/* STDARG */
extern int fprintf();
extern int vsprintf();
extern int sprintf();

#endif				/* STDARG */
extern void krt_init(void);		/* Read kernel routing tables and other useful info */
extern int krt_add(rt_entry * rt);	/* Add a route to the kernel */
extern int krt_change(rt_entry * old_rt, rt_entry * new_rt);	/* Change a kernel route */
extern int krt_delete(rt_entry * rt);	/* Delete a kernel route */
extern int
krt_delete_dst(task * tp,
	       sockaddr_un * dest,
	       sockaddr_un * mask,
	       sockaddr_un * gate,
	       flag_t flags);		/* Delete a kernel route */

#else				/* USE_PROTOTYPES */
extern char *gd_lower();
extern void quit();
extern void getod();
extern int strcasecmp(), strncasecmp();
extern int fprintf();
extern int vsprintf();
extern int sprintf();
extern void krt_init();			/* Read kernel routing tables and other useful info */
extern int krt_add();			/* Add a route to the kernel */
extern int krt_change();		/* Change a kernel route */
extern int krt_delete();		/* Delete a kernel route */
extern int krt_delete_dst();		/* Delete a kernel route given with a kernel route*/

#endif				/* USE_PROTOTYPES */

/* Error message defines */

extern int errno;

/*
 *	Definitions of descriptions of bits
 */

typedef struct {
    u_int t_bits;
    const char *t_name;
} bits;


/*
 *	I/O structures
 */

extern struct ip recv_ip;		/* Received IP packet */
extern sockaddr_un recv_addr;		/* Source address of received packet */
extern struct iovec recv_iovec[];	/* Pointer to I/O buffers */
extern struct msghdr recv_msghdr;	/* Default msghdr for receive */

#define	RECV_IOVEC_IP	0		/* Member of iovec pointing to IP header */
#define	RECV_IOVEC_DATA	1		/* Member of iovec pointing to packet */
#define	RECV_IOVEC_SIZE	2		/* Size of IOVEC */
