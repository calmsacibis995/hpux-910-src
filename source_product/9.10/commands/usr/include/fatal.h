/* @(#) $Revision: 66.1 $ */       
#ifndef _FATAL_INCLUDED /* allow multiple inclusions */
#define _FATAL_INCLUDED

#ifndef _JBLEN
#include <setjmp.h>
#endif /* _JBLEN */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

extern	int	Fflags;
extern	char	*Ffile;
extern	int	Fvalue;

#if defined(__STDC__) || defined(__cplusplus)
   extern	int	(*Ffunc)(const char *);
#else
   extern	int	(*Ffunc)();
#endif

extern	jmp_buf	Fjmp;

# define FTLMSG		0100000
# define FTLCLN		 040000
# define FTLFUNC	 020000
# define FTLACT		    077
# define FTLJMP		     02
# define FTLEXIT	     01
# define FTLRET		      0

# define FSAVE(val)	SAVE(Fflags,old_Fflags); Fflags = val;
# define FRSTR()	RSTR(Fflags,old_Fflags);

#ifdef __cplusplus
}
#endif

#endif /* _FATAL_INCLUDED */
