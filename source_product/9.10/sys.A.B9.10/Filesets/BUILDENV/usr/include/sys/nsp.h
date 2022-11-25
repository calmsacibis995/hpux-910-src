/*
 * @(#)nsp.h: $Revision: 1.6.83.3 $ $Date: 93/09/17 16:44:51 $
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

#ifndef _SYS_NSP_INCLUDED /* allows multiple inclusion */
#define _SYS_NSP_INCLUDED
/*
 * This file contains the includes for the cluster server
 * process (CSPs).
 * Within the implementation they are also known as network
 * server process (NSPs), and the terms are used interchangeably.
 */

/* values for commands to nsp() system call */
#define	NSP_CMD_ABS	0
#define	NSP_CMD_DELTA	1
#define NSP_INFO_READ	2
#define NSP_INFO_RESET	3

/*
 * information on each network server process.
 * Entry 0 is the limited NSP
 */
struct nsp
{
	u_char nsp_flags;		/*see below*/
	u_char nsp_timeout_type;	/*for statistics - see below*/
	struct proc *nsp_proc;		/*the process of this nsp*/
	site_t nsp_site;		/*site requesting operation*/
	short nsp_pid;			/*process requesting operation*/
	long nsp_rid;			/*mesg ID of current message*/
	struct ucred *nsp_cred;		/*saved ucred area*/
};

/* nsp flags */
#define NSP_BUSY	0x1
#define NSP_VALID	0x4	/*must be cleared if NSP dies*/
#define NSP_TIMED_OUT	0x8	/*NSP should commit suicide*/
#define NSP_LIMITED	0x10	/*limited NSP*/

/*other constants*/
#define PRINSP (37+PTIMESHARE)	/*dont ask me why 37*/

/* KNSP timeouts are a function of the number of KNSPs available */
#define NSPTIMEOUT_SHRT hz		/* 1 second  */
#define NSPTIMEOUT_MED  (2 * hz)	/* 2 seconds */
#define NSPTIMEOUT_LONG (4 * hz)	/* 4 seconds */

/* Defines for calling invoke_nsp */
#define LIMITED_OK	-1
#define LIMITED_NOT_OK	0

/* CSP statistics stuff */

#define	CSPSTAT_SHRT	0	/*short timeout*/
#define	CSPSTAT_MED	1	/*medium timeout*/
#define	CSPSTAT_LONG	2	/*long timeout*/
#define	CSPSTAT_LIMITED	3	/*limited CSP - no timeout*/

#define CSPSTAT_TIMEOUT_TYPES	3
#define CSPSTAT_COUNT_TYPES	4

struct csp_stats {
	int	limitedq_curlen;
	int	generalq_curlen;
	int	limitedq_maxlen;
	int	generalq_maxlen;
	int	min_gen_free;
	int	max_lim_time;		/* longest request in ticks */
	int	requests[CSPSTAT_COUNT_TYPES];
	int	timeouts[CSPSTAT_TIMEOUT_TYPES];
};

/*
 * Statistics structure returned by NSP_INFO_READ.  This is
 * similar to csp_stats, but has additional information at the
 * end of the structure so that too may be returned.
 */
struct csp_info {
	/* First part is identical to (struct csp_stats) */
	int	limitedq_curlen;
	int	generalq_curlen;
	int	limitedq_maxlen;
	int	generalq_maxlen;
	int	min_gen_free;
	int	max_lim_time;	/* longest request in ticks */
	int	requests[CSPSTAT_COUNT_TYPES];
	int	timeouts[CSPSTAT_TIMEOUT_TYPES];
	/* extra information */
	int	active_nsps;	/* number of nsps alive */
	int	free_nsps;	/* number of nsps free */
	int	ngcsp;		/* max number of csps possible */
	int	started;	/* number of csps started by user */
};

/*
 * macros for re-initializing appropriate fields - all but
 * current q lengths
 */
#define	CSPSTAT_FIRST_CLEAR	limitedq_maxlen
#define CSPSTAT_CLEAR_SIZE	(sizeof(struct csp_stats) - (2*sizeof(int)))

#ifdef _KERNEL
#ifdef VOLATILE_TUNE
extern volatile struct nsp nsp[];
extern volatile int max_nsp;	/* highest slot currently in use */
#else
extern struct nsp nsp[];
extern int max_nsp;		/* highest slot currently in use */
#endif /* VOLATILE_TUNE */
extern struct nsp *nspNCSP;	/* pointer past end of array */
extern int ncsp;		/* size of nsp array */
extern int ngcsp;		/* max number of general CSPS == ncsp - 1 */
struct csp_stats csp_stats;
#endif	/* _KERNEL */
#endif /* _SYS_NSP_INCLUDED */
