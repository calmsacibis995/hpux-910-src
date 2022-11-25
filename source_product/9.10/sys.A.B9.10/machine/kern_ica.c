/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/kern_ica.c,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:06:06 $
 */

#ifdef	ICA_300

/*
 *	The function of the ICA code is to implement an Instruction
 *	Coverage Analysis scheme.  The purpose of it is to identify
 *	each instruction in the kernel as executed or not executed.
 *	This is done by setting breakpoints in the code under test,
 *	then noting which instructions have been executed, and replacing
 *	the breakpoint with the original instructions.  Note that
 *	some routines absolutely cannot be covered this way ( for
 *	example, these routines) and some may be difficult for timing
 *	or other reasons.
 *
 *	In this file is only the break point handling routine(s).
 *	The driver code is found in ica.c
 */

#include "../s200/ica.h"
#include "../s200/trap.h"
#include "../h/param.h"
#include "../h/vas.h"
#include "../h/debug.h"
#include "../machine/pte.h"

/* Global variables */

int    ica_initflag=ICA_INITOFF,
				/* ICA_INITOFF => Do not do ica initialization
				 * ICA_INITON  => Do ica initialization 
				 * ICA_ISON => Initialization done,and ica on.
				 * ICA_ISOFF => Initialization done,but not on.
				 * ICA_INITFAIL => Ica initialization failed
				 */
	*ica_hitmap,		/* Pointer to the hitmap bitmap. 	
				 * The i'th bit represents the i'th 
				 * instruction in the kernel.	 
				 * A 1 indicates that a breakpoint has 	
				 * been hit, and a zero that the location
				 * has not been hit (it might or might	
				 * not have a breakpoint).		
				 */
	*ica_initmap,		/* Pointer to the initmap bitmap.	
				 * The i'th bit represents the i'th 
				 * instruction in the kernel.	
				 * A 1 indicates that this location is 
				 * to receive a breakpoint upon initiaization
				 * A 0 means it should be left as is	
				 * Note that since initmap is used to
				 * detect errors in the break handler,
				 * it should not be altered unless ica
				 * is turned off.
				 */
	*ica_breakmap,		/* Pointer to the breakmap bitmap. The i'th
				 * bit represents the i'th instruciton in
				 * the kernel.  A 1 indicates that this
				 * location has a breakpoint. 
				 */
	*ica_jtblmap,		/* Pointer to the jump table bitmap.  The i'th
				 * bit will be set if the i'th word in the
				 * kernel is part of a switch statement jump
				 * table rather than part of a machine
				 * instruction.
				 */
	ica_maplength, 		/* Length of both the ica_hitmap and the
				 * ica_initmap. Measured in bytes. 
			 	 */
	ica_textlength; 	/* Length of kernel text. Measured in words */

short	*ica_textcopy,		/* Pointer to a copy of kernel text.	
				 * Used when breakpoints need replacing	
				 */
	*ica_text;		/* Pointer to beginning of kernel text  */


/*
 * The ica_handler is called from the trap handler when an ica
 * breakpoint is encountered during kernel execution.
 * It marks the hitmap for the given location, replaces the
 * trap instruction with the original instruction, and
 * returns.
 */

ica_handler(locregs)
struct exception_stack locregs;
{
	int		s;
	unsigned int	i_addr;		/* address of trap instruction that
					   executed.  note that this address
					   is in units of 16-bit words, not
					   bytes. */
	int		i_lngth;	/* length in 16-bit words of the actual
					   instruction corresponding to the trap
					   that fired. */
	int		i;

	struct pte *kpte;
	int         ntp;

	s = CRIT();
	if (ica_initflag != ICA_ISON)
		panic("ica_handler called but ica not on");

	/* get address and length of instruction corresponding to trap
	   that fired */

	i_addr = (locregs.e_PC - 2)/2;

	/* Make sure that ICA put a break here. */
	if (!wisset(ica_breakmap, i_addr)) {
		panic("Break where we are not expecting one");
	}

	/* Make sure i_addr is within range */
	if (i_addr > ica_textlength) {
		panic("i_addr beyond text length");
	}
	i_lngth = inst_size(&ica_textcopy[i_addr])/2;

	/* adjust return address on stack so replaced instruction will be
	   executed on return from trap */

	locregs.e_PC -= 2;

	/* set bit(s) in hitmap indicating that instruction executed and
	   clear bit in breakmap since break will be replaced by actual
	   instruction */

	for(i = 0; i < i_lngth; i++)
		wsetbit(ica_hitmap, i_addr + i);
	wclrbit(ica_breakmap, i_addr);

	/* Turn off the write protection for kernel, in case kdb has reset it */
	kpte = vastopte(&kernvas, locregs.e_PC);
	*(int *)kpte &= ~PG_PROT;

	/* Replace break with the original instruction. */
	ica_text[i_addr] = ica_textcopy[i_addr];

	/* For the 68040, make sure the caches get flushed.  I'm not sure
	   that it we need this, but I haven't got it to work with the
	   68040 yet.  CWB */
	purge_icache();
	purge_dcache_physical();

	UNCRIT(s);
}

#endif	ICA_300
