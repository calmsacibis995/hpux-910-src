/* @(#) $Revision: 64.1 $ */    
#ifndef _NAN_INCLUDED /* allow multiple inclusions */
#define _NAN_INCLUDED

#ifdef __hp9000s800
/* Handling of Not_a_Number's (only in IEEE floating-point standard) */

#define KILLFPE()	(void) kill(getpid(), 8)
#if u3b || u3b5
#define NaN(X)	(((union { double d; struct { unsigned :1, e:11; } s; } \
			*)&X)->s.e == 0x7ff)
#define KILLNaN(X)	if (NaN(X)) KILLFPE()
#else
#define Nan(X)	0
#define KILLNaN(X)
#endif

#endif /* __hp9000s800 */
#endif /* _NAN_INCLUDED */
