/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_hooks.h,v $
 * $Revision: 1.8.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:41:07 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986 Hewlett-Packard Company.
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

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


/*
** This header is used solely for configuring out portions
** of the DUX code. The macro DUXCALL(XX)() is used in several
** places in the permanently resident portion of the kernel.
** The entire duxproc[] array of pointers to functions is 
** initialized to nop functions (dux_nop(){}), and is then
** filled in at boot time as appropriate. The idea is that
** all undefined externals are able to be resolved in this 
** manner. iI the kernel being configured is  standalone
** (i.e. no LAN -or- DUX) then those DUX specific functions
** like the dm, protocol, nsps, unsp, clocksync, etc. shouldn't
** be called and it is a significant RAM savings to configure
** them out as nop's.
*/


extern int(*duxproc[])();
/*
** usage: DUXCALL(function)(parm1, parm2, ...) 
*/
#define DUXCALL(function) (*duxproc[function])


/*
** The following macro is used to populate the DUX funcentry table
** at boot time. This table holds all of the remote procedure call
** (dm) serving side functions specific to DUX. The entire funcentry
** table is compiled with nop's as the function to call. If DUX is
** configured in, the table is filled in as appropriate at boot time.
*/
extern struct dm_funcentry dm_functions[];
#define dm_funcentry_assign(x, y, z)\
{\
	dm_functions[x].dm_how = y;\
	dm_functions[x].dm_callfunc = z;\
};
	


/*
** The following definitions are use as indices 
** into the duxproc table.
*/
#define	DM_ALLOC		0
#define	DM_RELEASE		1
#define	DM_SEND			2
#define	DM_REPLY		3
#define	DM_QUICK_REPLY		4
#define	DM_WAIT			5
#define	DM_RECV_REPLY		6
#define	DM_RECV_REQUEST 	7
#define	SENDDMSIG		8


#define FIND_ROOT		9
#define BROADCAST_FAILURE	10
#define WS_CLUSTER		11
#define REQ_SETTIMEOFDAY	12
                                      /* 13 was SCHED_CHUNK_RELEASE */
#define LIMITED_NSP		14
#define RETRY_SYNC_REQ		17
#define DUX_STRATEGY		18
#define BRELSE_TO_NET		19
#define DUX_LAN_ID_CHECK	20
#define ROOT_CLUSTER		21
#define KILL_LIMITED_NSP	22
#ifdef AUDIT
#define SETEVENT		23
#define AUDCTL			24
#define AUDOFF			25
#define GETAUDSTUFF		26
#define SWAUDFILE		27
#define CL_SWAUDFILE		28
#endif
#define	DUX_CLOSE_SEND		29
					/* 30: not used */
#define DUX_FPATHCONF		31
#define DUX_GETATTR		32
#define DUX_PATHSEND		33

/* moved here from what was lanfunc.h */
#define DUX_LANIFT_INIT         34
#define DUX_RECV_ROUTINES       35
#define DUX_HW_SEND             36
					/* 37: unused (was DUX_BCOPY) */
					/* 38: unused (was SKIP_FRAME) */
#define HW_SEND_DUX             39
#define DUX_COPYIN_FRAME        40
					/* 41: unused (was DRV_ADD_MULTICAST) */
#define LLA_OPEN                42
