/* @(#) $Revision: 66.2 $ */
#ifndef _UNCTRL_INCLUDED /* allow multiple inclusions */
#define _UNCTRL_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

/*
 * unctrl.h
 *
 */

#ifdef _INCLUDE_XOPEN_SOURCE
   extern char	*_unctrl[];
#  ifndef __lint
#    define	unctrl(ch)	(_unctrl[(unsigned int) ch])
#  endif /* __lint */
#endif /* _INCLUDE_XOPEN_SOURCE */

#endif /* _UNCTRL_INCLUDED */
