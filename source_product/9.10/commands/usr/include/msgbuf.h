/* @(#) $Revision: 66.3 $ */    

/*
 *  WARNING!  Use of this header file is unsupported.  It may not be
 *  provided on future releases of HP-UX.  It is recommended that
 *  applications define their own nl_catd identifier and use the
 *  supported funtions catgets(3C) and catopen(3C) in place of the
 *  macros defined by this file.
 */
#ifndef _MSGBUF_INCLUDED /* allow multiple inclusions */
#define _MSGBUF_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#include <nl_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NL_SETN 
#define NL_SETN 1
#endif

extern	nl_catd	_nl_fn;

#if defined(__STDC__) || defined(__cplusplus)
   extern char *catgets(nl_catd, int, int, const char *);
   extern nl_catd catopen(const char *, int);
   extern int catclose(nl_catd);
#else /* not __STDC__ || __cplusplus */
   extern char *catgets();
   extern nl_catd catopen();
   extern int catclose();
#endif /* else not __STDC__ || __cplusplus */

#define nl_msg(i, s)		catgets(_nl_fn, NL_SETN, i, s)
#define nl_catopen(cmdn)	(_nl_fn = catopen(cmdn, 0))

#ifdef __cplusplus
}
#endif

#endif /* _MSGBUF_INCLUDED */
