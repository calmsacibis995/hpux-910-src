/* @(#) $Revision: 1.14.83.4 $ */       
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/vmparam.h,v $
 * $Revision: 1.14.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 18:39:09 $
 */
#ifndef _SYS_VMPARAM_INCLUDED /* allows multiple inclusion */
#define _SYS_VMPARAM_INCLUDED
/*
 * Machine dependent constants
 */
#ifdef _KERNEL_BUILD
#include "../machine/vmparam.h"
#else  /* ! _KERNEL_BUILD */
#include <machine/vmparam.h>
#endif /* _KERNEL_BUILD */

#endif /* _SYS_VMPARAM_INCLUDED */
