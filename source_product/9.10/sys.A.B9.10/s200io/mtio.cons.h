/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/mtio.cons.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:15:00 $
 */
/* HPUX_ID: @(#)mtio.cons.h.	52.1		88/04/19 */

#ifndef _MACHINE_MTIO.CONS_INCLUDED
#define _MACHINE_MTIO.CONS_INCLUDED

/* magtape internal flags */
#define BELL_STYLE	0x00
#define UCB_STYLE	0x01
#define AUTO_REWIND	0x02

#define STATUS_SIZE 3

struct tp_status {
	daddr_t	tp_blkno;		/* for b mode access */
	daddr_t tp_nextrec;
	long	tp_resid;
	char	tp_sbytes[STATUS_SIZE];	/* status result */
	char	tp_data[STATUS_SIZE];	/* scratch area */
	struct	buf tp_buf;
};

#endif /* not _MACHINE_MTIO.CONS_INCLUDED */
