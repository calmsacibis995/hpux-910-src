/* @(#) $Revision: 66.2 $ */       
#ifndef _MODEL_INCLUDED /* allow for multiple inclusions */
#define _MODEL_INCLUDED

#include <sys/magic.h>

#define HP_S_500 HP9000_ID
#define HP_S_200 HP98x6_ID
#define HP_S_300 CPU_HP_MC68020
#define HP_S_800 CPU_PA_RISC1_0
#define HP_S_700 CPU_PA_RISC1_1

#ifdef __hp9000s300
#define MYSYS    HP_S_300
#endif /* __hp9000s300 */

#ifdef __hp9000s700
#  define MYSYS    HP_S_700
#else
#  ifdef __hp9000s800
#    define MYSYS    HP_S_800
#  endif /* __hp9000s800 */
#endif /* __hp9000s700 */

/* predefined types */
typedef	char	int8;
typedef	unsigned char	u_int8;
typedef	short	int16;
typedef	unsigned short	u_int16;
typedef	long	int32;
typedef	unsigned long	u_int32;
typedef	long	machptr;
typedef	unsigned long	u_machptr;

#endif /* _MODEL_INCLUDED */
