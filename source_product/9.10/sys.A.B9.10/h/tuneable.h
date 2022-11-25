/*
 * @@(#)tuneable.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 18:37:19 $
 * $Locker:  $
 */

#ifndef _SYS_TUNEABLE_INCLUDED /* allows multiple inclusion */
#define _SYS_TUNEABLE_INCLUDED

#ifdef _KERNEL_BUILD
#include "../machine/param.h"
#else  /* ! _KERNEL_BUILD */
#include <machine/param.h>
#endif /* _KERNEL_BUILD */

#define MAXPREP         8
#define MAXPAGEPHYS (64 * 1024)
#define MAXPG (MAXPAGEPHYS/NBPG)
#define NETMAXPHYS (8 * 1024)
#define NETMAXPG (NETMAXPHYS/NBPG)

#endif /* _SYS_TUNEABLE_INCLUDED */
