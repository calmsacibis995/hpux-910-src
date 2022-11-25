/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/magic.h,v $
 * $Revision: 1.13.83.4 $       $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 18:29:00 $
 */
#ifndef _SYS_MAGIC_INCLUDED
#define _SYS_MAGIC_INCLUDED

/*
 * sys/magic.h: info about HP-UX "magic numbers"
 */

/*
 * Pick up CPU versions of system_id from unistd.h 
 */

#ifdef _KERNEL_BUILD
#include "../h/unistd.h"
#else  /* ! _KERNEL_BUILD */
#include <unistd.h>
#endif /* _KERNEL_BUILD */

/*
 * where to find the magic number in a file and what it looks like:
 */
#define MAGIC_OFFSET	0L

struct magic {
    unsigned short int system_id;
    unsigned short int file_type;
};

typedef struct magic MAGIC;

/*
 * predefined (required) file types:
 */
#define RELOC_MAGIC	0x106		/* relocatable only */
#define EXEC_MAGIC	0x107		/* normal executable */
#define SHARE_MAGIC	0x108		/* shared executable */

#define AR_MAGIC	0xFF65

/*
 * optional (implementation-dependent) file types:
 */
#define DEMAND_MAGIC	0x10B		/* demand-load executable */
#define DL_MAGIC	0x10D		/* dynamic load library */
#define SHL_MAGIC	0x10E		/* shared library	*/

/*
 * predefined HP-UX target system machine IDs
 */

/* Old HP-UX machines */
#define HP9000_ID	0x208	/* HP 9000 Series 500 machine */
#define HP98x6_ID	0x20A	/* HP 9000 Series 200 2.x or Integral PC */

/* Current HP-UX systems; the CPU_* constants are from unistd.h */
#define HP9000S200_ID	CPU_HP_MC68020
#define HP9000S800_ID	CPU_PA_RISC1_0

#define _PA_RISC1_0_ID	CPU_PA_RISC1_0
#define _PA_RISC1_1_ID	CPU_PA_RISC1_1
#define _PA_RISC1_2_ID	CPU_PA_RISC1_2
#define _PA_RISC2_0_ID	CPU_PA_RISC2_0

/* Macro for detecting system_id values for any PA-RISC machine */

/* The CPU_* constants are not monotonic. Checks should be made on the
   basis of equality and not monotonicity */
#define _PA_RISC_MAXID	0x2FF

#define _PA_RISC_ID(__m_num)		\
    (((__m_num) == _PA_RISC1_0_ID) ||	\
     ((__m_num) == _PA_RISC1_1_ID) ||   \
     ((__m_num) == _PA_RISC1_2_ID) ||   \
     ((__m_num) == _PA_RISC2_0_ID ))

#endif /* _SYS_MAGIC_INCLUDED */
