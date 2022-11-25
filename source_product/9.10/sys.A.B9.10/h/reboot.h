/* $Header: reboot.h,v 1.12.83.5 93/12/08 18:24:10 marshall Exp $ */       

#ifndef _SYS_REBOOT_INCLUDED
#define _SYS_REBOOT_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
      int reboot(int, ...);	/* see reboot(2) for other arguments */
#  else /* not _PROTOTYPES */
      int reboot();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */

   /*
    * Arguments to reboot system call.
    * These are passed to boot program in r11,
    * and on to init.
    */
#    define RB_AUTOBOOT	   0x0000  /* flags for system auto-booting itself */

#    define RB_ASKNAME	   0x0001  /* ask for file name to reboot from */
#    define RB_SINGLE	   0x0002  /* reboot to single user only */
#    define RB_NOSYNC	   0x0004  /* don't sync before reboot */
#    define RB_HALT	   0x0008  /* don't reboot, just halt */
#    define RB_INITNAME	   0x0010  /* name given for /etc/init */

#    ifdef __hp9000s300
#      define RB_NEWDEVICE 0x0020  /* name given for device file */
#      define RB_NEWFILE   0x0040  /* name given for file to be booted */

#      define RB_PANIC	   0x0080  /* reboot due to panic */
#      define RB_BOOT	   0x0100  /* reboot due to boot() */
#      define RB_NEWSERVER 0x0200  /* addr given for new server to boot from */

#      ifdef _KERNEL
#      ifdef CLEANUP_DEBUG
#          define RB_CRASH  0x8000  /* don't sync or kill or broadcast,
				       just reboot.  Used for Diskless
				       crash recovery testing */
#      endif /* CLEANUP_DEBUG */
#      endif /* _KERNEL */
#    endif /* __hp9000s300 */

#    ifdef	__hp9000s800
#    define	RB_PANIC	0	/* reboot due to panic */
#    define	RB_BOOT		1	/* reboot due to boot() */
#    endif /* __hp9000s800 */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_REBOOT_INCLUDED */
