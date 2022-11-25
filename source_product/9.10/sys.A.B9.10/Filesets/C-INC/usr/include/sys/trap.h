/* @(#) $Revision: 1.9.83.5 $ */     
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/trap.h,v $
 * $Revision: 1.9.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 12:40:46 $
 */
#ifndef _SYS_TRAP_INCLUDED
#define _SYS_TRAP_INCLUDED

#ifdef __hp9000s300
#include <machine/trap.h>
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#ifdef _KERNEL_BUILD
#include "../machine/trap.h"
#else  /* ! _KERNEL_BUILD */
#include <machine/trap.h>
#endif /* _KERNEL_BUILD */
#endif /* __hp9000s800 */

#endif /* _SYS_TRAP_INCLUDED */
