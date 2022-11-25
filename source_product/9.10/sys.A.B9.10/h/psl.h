/* @(#) $Revision: 1.9.83.5 $ */      
/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/psl.h,v $
 * $Revision: 1.9.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 11:49:14 $
 */
#ifndef _SYS_PSL_INCLUDED
#define _SYS_PSL_INCLUDED

#ifdef __hp9000s300
#include <machine/psl.h>
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#ifdef _KERNEL_BUILD
#include "../machine/psl.h"
#else  /* ! _KERNEL_BUILD */
#include <machine/psl.h>
#endif /* _KERNEL_BUILD */
#endif /* __hp9000s800 */

#endif /* _SYS_PSL_INCLUDED */
