/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/debrec/RCS/initcore.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:22:59 $
 */

/*
 * Original version based on: 
 * Revision 63.1  88/05/26  13:35:48  13:35:48  markm
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * Corefile initialization.
 *
 * init.c and initcore.c and initsym.c used to be together, but the
 * compiler's symbol table overflowed.
 */


#ifdef FOCUS
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/param.h>
#define SYS_UINT        1
#define SYS_USHORT      1
#include "macdefs.h"
#else

#ifdef SYSVHDRS
#include <sys/types.h>
#include <sys/signal.h>
#endif

#include <sys/param.h>

#ifdef SPECTRUM
#include <sys/types.h>
#include <sys/sysmacros.h>
#endif


#ifdef S200

#ifdef S200BSD
#include <sys/vmparam.h>
#define USRSTART 0x0
#define PAGESIZE ctob(1)
#define SYS_UINT 1
#else	/* not S200BSD */
#include <sys/mknod.h>
#include <sys/page.h>
#endif	/* not S200BSD */

#include <machine/reg.h>
#include <sys/user.h>
#define AOUTINC 1
#define SYS_USHORT 1
#include "cdb.h"
#define cbStorageMax	((int) (USRSTACK))
#define cbFileMax	((long) (1L << 24))

#else  /* not S200 */
#ifdef SPECTRUM 
#include <sys/signal.h>
#include <machine/vmparam.h>
#include <sys/user.h>
#define AOUTINC 1
#else
#include <sys/user.h>
#endif
#endif

#include <sys/stat.h>
#ifndef SPECTRUM
#include <core.h>
#endif

#endif /* not FOCUS */

#ifdef SYSIIIOS

#ifdef FOCUS
struct errors {
	long	errno;
	long	errinfo;
	long	trapno;
};
typedef	struct	{
	long    u_pid;			/* Process id */
	long	u_ppid;			/* Parent process id */
	ushort	u_ruid;			/* Real user id */
	ushort  u_euid;			/* Effective user id */
	ushort  u_suid;			/* Saved user id */
	ushort	u_rgid;			/* Real group id */
	ushort  u_egid;			/* Effective group id */
	ushort  u_sgid;			/* Saved group id */
	long	u_pgrp;			/* Process group leader pid */
	char	u_ppg_args[80];		/* Name and arguments of program */
	long	u_sig;			/* Signal that caused core dump */
	long	u_signal[NSIG-1];	/* Signal handler table */
	long	u_flags;		/* Flags register */
	long	u_umask;		/* Umask value */
	long	u_nice;			/* Nice value */
	struct errors	u_errors;	/* Errno, errinfo, trapno values */
	long	u_starting_time;	/* Starting time */
	long	u_sys_cpu_time;		/* System CPU time */
	long	u_user_cpu_time;	/* User CPU time */
	long	u_child_sys_cpu_time;	/* Children system CPU time */
	long	u_child_user_cpu_time;	/* Children user CPU time */
	struct itimerval u_itimer[3];	/* Interval timers */
	long	u_open_files[NOFILE];	/* Open files table */
	long	u_control_tty;		/* Controlling tty */
	struct	{ 			/* Magic number */
	ushort	sys_id, ld_type;
	}	u_magic_num;		
	long	u_lstab_off;		/* Link symbol table offset */
	long	u_dnttab_off;		/* Debug name-type table offset */
	long	u_vtab_off;		/* Value table offset */
	long	u_sltab_off;		/* Source line table offset */
	long	u_stack_off;		/* Offset to stack segment */
	long	u_stack_size;		/* Size of stack segment */
	long	u_trap_stack_marker;	/* Offset of capping stack marker */
	long	u_global_data_off;	/* Offset to global data segment */
	long	u_global_data_size;	/* Size of global data segment */
	long	u_heap_off;		/* Offset to heap segment */
	long	u_heap_size;		/* Size of heap segment */
	long	u_max_heap_size;	/* Maximum heap size */
	long	u_num_code_segs;	/* Number entries - 1 in code seg map */
	long	u_fix_map_off;		/* Offset to code segment fixup map */
	long	u_fix_map_size;		/* Size of code segment fixup map */
} COREHDRR, *pCOREHDRR ;
#define cbUSER sizeof(COREHDRR)
#else
#ifdef SPECTRUM
extern struct som_exec_auxhdr vauxhdr; /* declared in initsym.c */
#define cbUSER ctob(USIZE)+ctob(STACKSIZE)
#else
#ifdef S200
#define cbUSER (UPAGES * PAGESIZE)
#else
#define cbUSER (ctob (USIZE))
#endif  /* not S200 */
#endif  /* not SPECTRUM */
#endif  /* not FOCUS */
#endif  /* SYSIIIOS */

#ifdef SPECTRUM
/*
 * We DON'T want it for SPECTRUM because we did NOT include param.h
 * or types.h so we DO need ushort in basic.h.  Got that?  What a mess!!
 */
#define SYS_USHORT	1
#define SYS_UINT	1
#endif

extern int	errno;		/* system call error number */

#ifndef S200
#include "cdb.h"
#define cbStorageMax	(0x7ffff000L)
#define cbFileMax	((long) (1L << 24))
#endif

MAPR	vmapCore;

#ifdef S200
long	vUarea;
#endif

#ifdef FOCUS
FLAGT    vfCoreAttrValid;
ADRT     vpcCore;
ulong    vfpCore;
ulong    vspCore;
ulong    vOffStkCore;
ulong    vOffGdsCore;
ulong    vOffHeapCore;
ulong    vHeapCoreMax;
ulong    viStkCoreMac;
ulong    viGdsCoreMac;
ulong    viHeapCoreMac;
ulong    viCoreUSegMac;
ulong    *vrgUSegCore;
ulong    viCoreLSegMac;
ulong    *vrgLSegCore;
#endif


/***********************************************************************
 * I N I T   C O R E F I L E
 */

void InitCorefile (sbCore)
    char	*sbCore;	/* name of corefile */
{

    FLAGT	fSpecified = (sbCore != sbNil);
#ifndef S200
    int		sig;
#endif
    register long cbText;
    register long cbData;
    long	cbStack;
#ifdef S200BSD
    long        PGSHIFT;
    long        UPAGES;
    caddr_t     USRSTACK;
    char        auser[UPAGES_WOPR * NBPG_WOPR];  /* larger than UMM user area */
#else
    char        auser[cbUSER];          /* the user struct of core file */
#endif
#ifdef FOCUS
    struct STKMRKR {
        int     indexReg;
        int     relpc;
        int     statReg;
        int     deltaQ;
    } stkMrk;
#define cbSTKMRKR       sizeof(struct STKMRKR)
#define privBit         0x20000         /* status register privilege bit */
        FLAGT   fNoUserStk;
        long    cbUSeg;
    register pCOREHDRR userCore = (pCOREHDRR) auser;
#else
    register struct user *userCore = (struct user *) auser;
#endif

#ifdef INSTR
    vfStatProc[145]++;
#endif

#ifdef HPE
    if (!fSpecified || (vfnCore = open (sbCore, 0)) < 0)		/* open it read-only */
#else /* HP-UX */
    if (!fSpecified)
	sbCore = "core";

    if ((vfnCore = open (sbCore, 0)) < 0)		/* open it read-only */
#endif /* HP-UX */
    {
#ifndef HPE
	printf ((nl_msg(133, "No core file\n")));
#endif
	vfnCore = fnNil;
	vsbCorefile = vsbnocore;
	return;
    }

#ifdef NOTDEF
    if (vmtimeSym > AgeFFn(vfnCore))
    {
	printf ((nl_msg(134, "WARNING:  \"%s\" is older than \"%s\"; ignoring \"%s\".\n")),
	    sbCore, vsbSymfile, sbCore);
	close (vfnCore);
	vfnCore	    = fnNil;
	vsbCorefile = vsbnocore;
	return;
    }
#endif

    vsbCorefile = sbCore;

/*
 * DO HAVE A COREFILE:
 */

#ifdef S200BSD
    if (vfUMM)
    {
        UPAGES = UPAGES_UMM;
        USRSTACK = USRSTACK_UMM;
        PGSHIFT = PGSHIFT_UMM;
    }
    else        /* WOPR */
    {
        UPAGES = UPAGES_WOPR;
        PGSHIFT = PGSHIFT_WOPR;
        if (vfW310)                     /* 310 with 24-bits of address */
            USRSTACK = USRSTACK_M310;
        else                            /* 320 with 32-bits of address */
            USRSTACK = USRSTACK_M320;
    }
#endif
    if (read (vfnCore, userCore, cbUSER) != cbUSER)
    {
	printf ((nl_msg(135, "Can't read \"%s\"; ignoring it\n")), vsbCorefile);
	close (vfnCore);
	vfnCore	    = fnNil;
	vsbCorefile = vsbnocore;
	return;
    }


    vmapCore.fn = vfnCore;


    return;

} /* InitCorefile */
