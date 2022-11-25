/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/pcb.h,v $
 * $Revision: 1.4.84.7 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/09/22 14:50:42 $
 */
/* @(#) $Revision: 1.4.84.7 $ */       
#ifndef _MACHINE_PCB_INCLUDED /* allow multiple inclusions */
#define _MACHINE_PCB_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
 * 68K software process control block
 */

struct pcb
{
  int	pcb_ksp; 	/* kernel stack pointer */
  int	pcb_esp; 	/* exec stack pointer */
#define u_probe_addr	u_pcb.pcb_ksp  /* temp redefine of unused area in 300 */
	int	pcb_ssp; 	/* supervisor stack pointer */
	int	pcb_usp; 	/* user stack pointer */
	struct  pte *pcb_p0br; 	/* seg 0 base register */
	int	pcb_p0lr; 	/* seg 0 length register and astlevel */
	struct  pte *pcb_p1br; 	/* seg 1 base register */
	int	pcb_p1lr; 	/* seg 1 length register and pme */
/*
 * Software pcb (extension)
 */
	int	pcb_szpt; 	/* number of pages of user page table */
	int	pcb_scratch;	/* scratch entry (place holder) */
	int	pcb_cmap2;
	int	*pcb_sswap;
	int	pcb_escapecode;
	struct	C__trystuff *pcb_trychain;
	u_char	pcb_flags;	/* machine-dependent flags defined below */
				/* note - assembly language code knows   */
				/*        this field is one byte long.   */

	/* Fields for accessing proper sysent table.  There is one table */
	/* for native object code and a second for compatibility with    */
	/* s200 2.x releases and the Integral PC (aka Pisces).           */
	struct	sysent *pcb_sysent_ptr;
	int	pcb_nsysent;

	int	pcb_float[10];
	int	pcb_mc68881[81];
	short	pcb_dragon_bank;	/* -1 means not using dragon */
	int	pcb_dragon_regs[32];
	int 	pcb_dragon_sr;
	int	pcb_dragon_cr;
};

/* bits in pcb_flags */
#define	POP_STACK_BIT	0	/* clean up exception stack after signal */
#define PROBE_BIT	1	/* probe operation in progress */
#define	USER_TRACE_BIT	2	/* trace trap causes user signal */
#define	POP_STACK_MASK	(1 << POP_STACK_BIT)
#define	PROBE_MASK	(1 << PROBE_BIT)
#define	USER_TRACE_MASK	(1 << USER_TRACE_BIT)
#define	MULTIPLE_MAP_BIT 3	/* multiple map 68020 address space in 256meg regions */
#define	MULTIPLE_MAP_MASK	(1 << MULTIPLE_MAP_BIT)
#define M68040_WB_BIT	4
#define M68040_WB_MASK	(1 << M68040_WB_BIT)
#define UMEM_TRACE_BIT	5
#define UMEM_TRACE_MASK (1 <<  UMEM_TRACE_BIT)
#define UMEM_PTRACE_BIT	6
#define UMEM_PTRACE_MASK (1 <<  UMEM_PTRACE_BIT)

#define pcb_fault_addr pcb_float[0]
#define pcb_signal pcb_float[1]
#define pcb_locregs pcb_float[2]
#define pcb_fpiar pcb_float[3]

#define	aston() runrun++	/* the best we can do */

#endif /* _MACHINE_PCB_INCLUDED */
