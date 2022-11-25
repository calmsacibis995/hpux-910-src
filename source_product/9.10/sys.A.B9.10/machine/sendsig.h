/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/sendsig.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:08:04 $
 */
/* @(#) $Revision: 1.4.84.3 $ */       
#ifndef _MACHINE_SENDSIG_INCLUDED /* allow multiple inclusions */
#define _MACHINE_SENDSIG_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/signal.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <signal.h>
#endif /* _KERNEL_BUILD */

/*
** General purpose registers d0-d7,a0-a6 are saved in fs_regs array
** Float card registers fstatus, ferrbit, fpr0-fpr7 are saved in fs_regs array
** MC68881 saves internal state (184 bytes), 3 control registers (12 bytes),
**         and 8 96 bit floating point registers (96 bytes) in fs_regs array
** See also reg.h
*/
#define	GPR_START		0
#define	FLOAT_START		15	
#define	MC68881_START		25	

#define	GPR_REGS		15
#define	FLOAT_CARD_REGS		10
#define	MC68881_REGS		81
#define DRAGON_REGS		34
#define DRAGON_START	(MC68881_START+MC68881_REGS)

/* Note - registers must be last field in sigframe, as the rest of the frame */
/*        is copied out to the user stack.                                   */
/* Note further that there are now errno's greater than sizeof(u_char),
 * when one is encountered, the errno is held temporarily in fs_rval1,
 * and fs_error is set to 255.
 */

struct	full_sigcontext {
	struct	sigcontext	fs_context;
	char			fs_eosys;
	u_char			fs_error;
	int			fs_rval1;
	int			fs_rval2;
	int			fs_regs[GPR_REGS+FLOAT_CARD_REGS+MC68881_REGS
					+DRAGON_REGS];
};

struct sigframe {
	void                    (*sf_handler)();
	int			sf_signum;
	int			sf_code;
	struct	sigcontext	*sf_scp;
	struct	full_sigcontext	sf_full;
};
#endif /* _MACHINE_SENDSIG_INCLUDED */
