/* @(#) $Revision: 64.1 $ */       
#ifndef _EXECARGS_INCLUDED /* allows multiple inclusion */
#define _EXECARGS_INCLUDED

/* To include this file, user has to explicitly include sys/param.h first */
#ifdef __hp9000s300
#define  USRSTART  0x2000
char	**execargs = (char**)(USRSTACK - 4);
#endif /* __hp9000s300 */

#ifdef __hp9000s800
char	**execargs = (char**)(USRSTACK);
#endif /* __hp9000s800 */

#endif /* _EXECARGS_INCLUDED */
