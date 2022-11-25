/* @(#) $Revision: 1.13.83.4 $ */    
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/modem.h,v $
 * $Revision: 1.13.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:30:04 $
 */
#ifndef _SYS_MODEM_INCLUDED /* allows multiple inclusion */
#define _SYS_MODEM_INCLUDED
/***********************************************************************
 *
 * file:    modem.h
 *
 *********************************************************************/

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* Define the timer ioctl structure */
#define   NMTIMER   6

struct  mtimer {
   unsigned   short   m_timers[NMTIMER];
};

typedef unsigned long mflag;

/* Define the bits within the mflag field */
#define MDRS        00000000004   /* Data Rate Select */
#define MDTR        00000000040   /* Data Terminal Ready */
#define MRTS        00010000000   /* Request To Send */
#define MDSR        00002000000   /* Data Set Ready */
#define MDCD        00000400000   /* Data Carrier Detect */
#define MRI         00000000010   /* Ring Indicator */
#define MCTS        00004000000   /* Clear To Send */

/* Define the timer fields */
#define MTCONNECT        0
#define MTCARRIER        1
#define MTNOACTIVITY     2
#define MTHANGUP         3

/* Define the modem ioctl commands */
#ifdef __hp9000s300
#define MCGETA      _IOR('M', 1, long)
#define MCSETA      _IOW('M', 2, long)
#define MCSETAW     _IOW('M', 3, long)
#define MCSETAF     _IOW('M', 4, long)
#define MCGETT      _IOR('M', 5, struct mtimer)
#define MCSETT      _IOW('M', 6, struct mtimer)
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#define MCGETA      _IOR('M', 0, mflag)
#define MCSETA      _IOW('M', 1, mflag)
#define MCSETAW     _IOW('M', 2, mflag)
#define MCSETAF     _IOW('M', 3, mflag)
#define MCGETT      _IOR('M', 4, struct mtimer)
#define MCSETT      _IOW('M', 5, struct mtimer)
#endif /* __hp9000s800 */
#endif /* _SYS_MODEM_INCLUDED */
