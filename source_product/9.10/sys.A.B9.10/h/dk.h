/* @(#) $Revision: 1.26.83.5 $ */
#ifndef _SYS_DK_INCLUDED /* allows multiple inclusion */
#define _SYS_DK_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#endif /* _KERNEL_BUILD */

#ifdef __hp9000s800
#  ifdef MP
#ifdef _KERNEL_BUILD
#      include  "../machine/mp.h"
#else  /* ! _KERNEL_BUILD */
#      include <machine/mp.h>
#endif /* _KERNEL_BUILD */
#  endif /* MP */
#endif	/* __hp9000s800 */

/*
 * Instrumentation
 */
#ifdef  FSD_KI
#  define CPUSTATES		9
#else   /* FSD_KI */
#  ifdef __hp9000s300
#    define CPUSTATES		4
#  endif /* __ hp9000s300 */
#  ifdef __hp9000s800
#    ifdef MP
#      define CPUSTATES		7
#    else
#      define CPUSTATES		5
#    endif /* MP */
#  endif /* __ hp9000s800 */
#endif  /* FSD_KI */

#define CP_USER         0	/* user mode of USER process */
#define CP_NICE         1	/* user mode of USER process at nice priority */
#define CP_SYS          2	/* kernel mode of USER process */
#define CP_IDLE         3	/* IDLE mode */

#ifdef __hp9000s800
#  define CP_WAIT       4       
#  ifdef MP
#    define CP_BLOCK	5	/* time blocked on a spinlock */
#    define CP_SWAIT	6	/* time blocked on the kernel semaphore */
#  endif /* MP */
#endif /* __hp9000s800 */

#ifdef  FSD_KI
#  define CP_INTR	7	/* INTERRUPT mode */
#  define CP_SSYS	8	/* kernel mode of KERNEL process */
#endif  /* FSD_KI */

#ifdef __hp9000s300
#  define DK_NDRIVE     8

#  ifdef _KERNEL
long    cp_time[CPUSTATES];
dev_t   dk_devt[DK_NDRIVE];
int     dk_busy;
long    dk_time[DK_NDRIVE];
long    dk_seek[DK_NDRIVE];
long    dk_xfer[DK_NDRIVE];
long    dk_wds[DK_NDRIVE];
float   dk_mspw[DK_NDRIVE];

long    tk_nin;
long    tk_nout;
#  endif  /* _KERNEL */
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#  ifdef _KERNEL
extern  long    cp_time[];
#    ifdef MP
extern  long    mcp_time[MAX_PROCS][CPUSTATES];
#    endif /* MP */

#    ifdef _WSIO
     /* Note: space for these should be declared in only ONE place! */
#      define DK_NDRIVE       8
dev_t   dk_devt[DK_NDRIVE];
int     dk_busy;
long    dk_time[DK_NDRIVE];
long    dk_seek[DK_NDRIVE];
long    dk_xfer[DK_NDRIVE];
long    dk_wds[DK_NDRIVE];
float   dk_mspw[DK_NDRIVE];
#    else /* !_WSIO */

     /*
      * Disc0 data structures;  obsolete, but necessary until commands
      * that nlist() the kernel for them are updated
      */
extern  int	dk_ndrive;
extern	char	dk_busy[];
extern	long	dk_time[];
extern  long    dk_time_snap[];
extern  long    dk_seek[];
extern  long    dk_xfer[];
extern  long    dk_wds[];
extern  float   dk_mspw[];

		/* disc1 */		/* disc2 */		/* disc3 */
extern int      dk_ndrive1,		dk_ndrive2,		dk_ndrive3;
extern int      dk_busy1[],		dk_busy2[],		dk_busy3[];
extern long     dk_time1[],		dk_time2[],		dk_time3[];
extern long     dk_time_snap1[],	dk_time_snap2[],	dk_time_snap3[];
extern long     dk_seek1[],		dk_seek2[],		dk_seek3[];
extern long     dk_xfer1[],		dk_xfer2[],		dk_xfer3[];
extern long     dk_wds1[],		dk_wds2[],		dk_wds3[];
extern float    dk_mspw1[],		dk_mspw2[],		dk_mspw3[];

#        define DK_CYL_SIZE     100
extern  int     dk_cyl_index1[],	dk_cyl_index2[],	dk_cyl_index3[];
extern  int     *dk_cyl1[],		*dk_cyl2[],		*dk_cyl3[];

#    endif /* !_WSIO */

extern	long	tk_nin;
extern	long	tk_nout;
#  endif /* _KERNEL */
#endif /* __hp9000s800 */

#endif /* _SYS_DK_INCLUDED */
