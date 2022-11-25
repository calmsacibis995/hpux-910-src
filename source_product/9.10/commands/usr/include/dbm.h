/* @(#) $Revision: 66.2 $ */

#ifndef _DBM_INCLUDED /* allow multiple inclusions */
#define _DBM_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_HPUX_SOURCE

/*
 * We no longer use NULL (and never should have unconditionally defined
 * it), but, this whole file is for backwards compatability - someone
 * may rely on this.
 */
#ifndef NULL
#define	NULL	((char *) 0)
#endif

#include <ndbm.h>

#if defined(__STDC__) || defined(__cplusplus)
   extern datum fetch(datum);
   extern datum firstkey(void);
   extern datum nextkey(datum);
#else /* not __STDC__ || __cplusplus */
   extern datum fetch();
   extern datum firstkey();
   extern datum nextkey();
#  if 0
     extern datum makdatum();
     extern datum firsthash();
     extern long calchash();
     extern long hashinc();
#  endif
#endif /* else not __STDC__ || __cplusplus */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _DBM_INCLUDED */
