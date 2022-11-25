/*
 * @(#)beeper.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 20:25:37 $
 * $Locker:  $
 */

#ifndef _SYS_BEEPER_INCLUDED
#define _SYS_BEEPER_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

struct beep_info {
    unsigned int frequency;  /* Frequency in Hertz       */
    unsigned int duration;   /* Duration in milliseconds */
    unsigned int volume;     /* 0-100, 0=Off,100=loudest */
    unsigned int spare[4];
};

#define DOBEEP       _IOW('B',1,struct beep_info)

#endif /* _SYS_BEEPER_INCLUDED */
