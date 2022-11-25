/* @(#) $Revision: 64.1 $ */    
#ifndef _KERN_PROF_INCLUDED /* allows multiple inclusions */
#define _KERN_PROF_INCLUDED

#ifdef __hp9000s300
#include <sys/gprof.h>

#define START_PROFILING	    if (kern_prof(GPROF_INIT) || kern_prof(GPROF_ON))\
			    {\
				 perror ("cannot start profiling");\
				 exit (1);\
			    }

#define	STOP_PROFILING	    if (kern_prof(GPROF_OFF))\
			    {\
				 perror ("cannot start profiling");\
				 exit (1);\
			    }

/* function takes pathname to write data as argument */
/* returns 1 for success, 0 for failure              */
extern  int	write_profile_data ();
#endif /* __hp9000s300 */

#endif /* _KERN_PROF_INCLUDED */
