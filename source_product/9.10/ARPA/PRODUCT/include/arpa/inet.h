/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)inet.h	5.2 (Berkeley) 6/27/88
 *	@(#)$Revision: 1.5.109.1 $
 */

/*
 * External definitions for
 * functions in inet(3N)
 */

#ifdef _KERNEL
#include "..h/stdsyms.h"
#else
#include <sys/stdsyms.h>
#endif

#ifdef _INCLUDE_HPUX_SOURCE

# ifndef _KERNEL
#  ifdef __cplusplus
    extern "C" {
#  endif

#  ifdef _PROTOTYPES
     extern unsigned long inet_addr(const char *);
     extern int inet_lnaof(struct in_addr);
     extern struct in_addr inet_makeaddr(int, int);
     extern int inet_netof(struct in_addr);
     extern unsigned long inet_network(const char *);
     extern char *inet_ntoa(struct in_addr);
#  else /* not _PROTOTYPES */
     extern unsigned long inet_addr();
     extern int inet_lnaof();
     extern struct in_addr inet_makeaddr();
     extern int inet_netof();
     extern unsigned long inet_network();
     extern char *inet_ntoa();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
    }
#  endif
# endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */
