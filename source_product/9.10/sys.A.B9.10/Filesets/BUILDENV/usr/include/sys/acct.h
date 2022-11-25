/* $Header: acct.h,v 1.18.83.3 93/09/17 17:01:35 root Exp $ */

#ifndef _SYS_ACCT_INCLUDED
#define _SYS_ACCT_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE
# ifndef _KERNEL
# ifdef __cplusplus
    extern "C" {
# endif /* __cplusplus */

# ifdef _PROTOTYPES
     extern int acct(const char *);
# else /* not _PROTOTYPES */
     extern int acct();
# endif /* not _PROTOTYPES */

# ifdef __cplusplus
    }
# endif /* __cplusplus */
# endif /* not _KERNEL */



    /*
     * Accounting structures;
     * these use a comp_t type which is a 3 bits base 8
     * exponent, 13 bit fraction ``floating point'' number.
     */
    typedef unsigned short comp_t;

    struct	acct
    {
	   char	ac_flag;		/* Accounting flag */
	   char	ac_stat;		/* Exit status */
	   unsigned short ac_uid;	/* Accounting user ID */
	   unsigned short ac_gid;	/* Accounting group ID */
	   dev_t	ac_tty;		/* control typewriter */
	   time_t	ac_btime;	/* Beginning time */
	   comp_t	ac_utime;	/* acctng user time in clock ticks */
	   comp_t	ac_stime;	/* acctng system time in clock ticks */
	   comp_t	ac_etime;	/* acctng elapsed time in clock ticks */
	   comp_t	ac_mem;		/* memory usage */
	   comp_t	ac_io;		/* chars transferred */
	   comp_t	ac_rw;		/* blocks read or written */
	   char	ac_comm[8];		/* command name */
    };	

    extern	struct	acct	acctbuf;
    extern	struct	vnode	*acctp;

#   define AFORK   01            /* has executed fork, but no exec */
#   define ASU     02           /* used super-user privileges */
#   define ACCTF   0300		/* record type: 00 = acct */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* not _SYS_ACCT_INCLUDED */
