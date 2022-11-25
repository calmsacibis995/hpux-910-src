/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/umem.h,v $
 * $Revision: 1.2.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/09/22 14:50:39 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

#ifdef UMEM_DRIVER

struct addr_range {
	unsigned int start_addr;
	unsigned int end_addr;
	unsigned short flags;
	unsigned short count;
};

/* address range flags */
#define MM_MONITOR_READ		0x0002
#define MM_MONITOR_WRITE	0x0004
#define MM_MONITOR_RW		0x0008
#define MM_TRAP_ON_ACCESS	0x0080
#define MM_WAS_RO		0x0040

#define	MAXRANGES	60
#define	MM_IGNORE	1

struct umem_data {
	pid_t mpid;		  /* pid of process being monitored */
	struct proc *mp;	  /* proc ptr of process being monitored */
	pid_t tpid;		  /* pid of monitoring process */
	struct proc *tp;	  /* proc ptr of monitoring process */
	unsigned int sig;	  /* signal to send monitoring process */
	unsigned int fault_addr;  /* accessed address that caused fault */
	unsigned int pc;	  /* users pc at the time of the fault */
	unsigned int faults;	  /* how many page faults have we caused?  */
	struct addr_range ranges[MAXRANGES]; /* address ranges being monitored */
};

#define UMEMD_SIZE sizeof(struct umem_data)

/*
 *      ioctls
 */

/* which PID to access  */
#define MM_SET_PID	_IOW('M', 1, int)
#define MM_SET_SIG	_IOW('M', 2, int)

/* Address ranges to monitor */ 
#define MM_SET_RANGE	_IOW('M', 3, struct addr_range)
#define MM_DELETE_RANGE	_IOW('M', 4, struct addr_range)

/* restart/stop monitored process */
#define MM_RESUME	_IOW('M', 5, int)
#define MM_SUSPEND	_IOW('M', 6, int)

/*  read the structure for this process  */
#define MM_GET_STRUCT	_IOR('M', 7, struct umem_data)

/*  sleep until one of the protected ranges is hit  */
#define MM_SELECT	_IOR('M', 8, int)

/*  read the structure for this process  */
#define MM_GET_ADDR	_IOR('M', 9, int)

/*  read Dragon FP registers  */
#define MM_RFPREGS	_IOR('M', 10, int)

/*  write Dragon FP registers  */
#define MM_WFPREGS	_IOW('M', 11, int)

/*  make the target process exit  */
#define MM_EXIT		_IOR('M', 12, int)

/*  make the target process single step  */
#define MM_SINGLE_STEP	_IOW('M', 13, int)

/*  make the target process continue  */
#define MM_CONTIN	_IOW('M', 14, int)

/*  make the target process stop  */
#define MM_STOP		_IOR('M', 15, int)

#define MM_WAKEUP	_IOW('M', 16, int)

extern int mm_trace_trap();

#endif /* UMEM_DRIVER*/

