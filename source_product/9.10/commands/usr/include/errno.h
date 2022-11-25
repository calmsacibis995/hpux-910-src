/* @(#) $Revision: 66.2 $ */
#ifndef _ERRNO_INCLUDED /* allow multiple inclusions */
#define _ERRNO_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

extern int errno;
#include <sys/errno.h>

#  ifdef __cplusplus

extern "C" {
    extern void perror(const char*);
    extern int sys_nerr;
    extern char *sys_errlist[];
    extern char *strerror (int);
}

#  endif


#endif /* _ERRNO_INCLUDED */
