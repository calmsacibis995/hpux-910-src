/*
 * @(#)core.h: $Revision: 1.6.83.4 $ $Date: 93/09/17 18:24:11 $
 * $Locker:  $
 */

#ifndef _SYS_CORE_INCLUDED
#define _SYS_CORE_INCLUDED
#ifdef  _KERNEL_BUILD
#include "../h/types.h"
#ifdef  __hp9000s300
#include  "../machine/a.out.h"
#endif /* __hp9000s300 */
#ifdef __hp9000s800
#include  "../machine/save_state.h"
#endif /* __hp9000s800 */
#else  /* ! _KERNEL_BUILD */
#include  <a.out.h>
#include  <sys/types.h>
#ifdef  __hp9000s800
#include  <machine/save_state.h>
#endif /* __hp9000s800 */
#endif /* _KERNEL_BUILD */
	
/* These are p_coreflags values */
#define CORE_NONE	0x00000000      /* reserved for future use */
#define CORE_FORMAT	0x00000001	/* core version */
#define CORE_KERNEL     0x00000002      /* kernel version */
#define CORE_PROC       0x00000004      /* per process information */
#define CORE_TEXT	0x00000008	/* reserved for future use */
#define CORE_DATA	0x00000010      /* data of the process */
#define CORE_STACK      0x00000020      /* stack of the process */
#define CORE_SHM	0x00000040      /* reserved for future use */
#define CORE_MMF	0x00000080      /* reserved for future use */
#define CORE_EXEC       0x00000100      /* exec information */

#define DEFAULT_CORE_OPTS	\
(CORE_KERNEL | CORE_EXEC | CORE_PROC | CORE_DATA | CORE_STACK \
| CORE_SHM | CORE_MMF | CORE_FORMAT)

#define CORE_FORMAT_VERSION	1	/* inc this by 1 every time core
					   file format changes. */
#define MAXCOMLEN 	14

struct corehead {
	int 	type;
	space_t space;
	caddr_t addr;
	size_t 	len;
};

struct proc_exec {
#ifdef __hp9000s800
	   /* The magic number, auxillary SOM header and spares */
	   struct{
	   	int	  u_magic;
	   	struct som_exec_auxhdr som_aux;
	   } exdata;
#endif
#ifdef  __hp9000s300
	   struct exec	Ux_A;
#endif
	   char	  	cmd[MAXCOMLEN + 1];
};

#ifdef  __hp9000s300
struct proc_regs {
	int	d0;
	int	d1;
	int	d2;
	int	d3;
	int	d4;
	int	d5;
	int	d6;
	int	d7;

	int	a0;
	int	a1;
	int	a2;
	int	a3;
	int	a4;
	int	a5;
	int	a6;
	int	a7;

	unsigned short ps;
	int     pc;
	int	usp;
	int	p_float[10];
	int	mc68881[81];
	short	dragon_bank;
	int	dragon_regs[32];
	int	dragon_sr;
	int	dragon_cr;

};
#endif

struct proc_info {
	int 	sig;
	int	trap_type;
#ifdef  __hp9000s300
	struct proc_regs hw_regs;
#else
	struct  save_state hw_regs;
#endif
};
#endif /* _SYS_CORE_INCLUDED */




	


