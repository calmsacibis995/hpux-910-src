/* @(#) $Revision: 64.2 $ */
#ifndef _ELOG_INCLUDED
#define _ELOG_INCLUDED

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifdef __hp9000s800
/*
 * "True" major device numbers. These correspond
 * to standard positions in the configuration
 * table, but are used for error logging
 * purposes only.
 */

#define CNTL	1
#define SYS	2
#define CAC	3
#define PF	4

/*
 * IO statistics are kept for each physical unit of each
 * block device (within the driver). Primary purpose is
 * to establish a guesstimate of error rates during
 * error logging.
 */

struct d0stat {
	long	io_ops;		/* number of read/writes */
	long	io_misc;	/* number of "other" operations */
	long	io_qcnt;	/* number of jobs assigned to drive */
	ushort io_unlog;	/* number of unlogged errors */
};

/*
 * structure for system accounting
 */
struct d0time {
	struct d0stat d0s;
	long	io_bcnt;	/* total blocks transferred */
	time_t	io_resp;	/* total block response time */
	time_t	io_act;		/* total drive active time (cumulative utilization) */
};
#define	io_cnt	d0s.io_ops
#define io_qc d0s.io_qcnt
/* drive utilization times can be calculated by system software as follows */

/* Average drive utilization = (io_cact/io_elapt) */
/* Average drive utilization for last interval = (io_liact/io_intv) */
#endif /* __hp9000s800 */

#endif /* _ELOG_INCLUDED */
