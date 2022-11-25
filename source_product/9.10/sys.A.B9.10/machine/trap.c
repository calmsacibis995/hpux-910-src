/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/trap.c,v $
 * $Revision: 1.11.84.13 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/09/22 14:50:47 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include "../h/debug.h"
#include "../s200/psl.h"
#include "../s200/reg.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../s200/trap.h"
#include "../h/vm.h"
#include "../h/acct.h"
#include "../h/kernel.h"
#include "../h/ipc.h"
#include "../h/syscall.h"
#include "../h/rtprio.h"
#include "../h/ptrace.h"
#include "../s200io/dragon.h"
#ifdef AUDIT
#include "../h/audit.h"		/* auditing support */
#include "../h/acl.h"		/* auditing aupport */
#endif
#include "../h/pregion.h"
#ifdef STRACE
#include "../s200io/strace.h"
#endif
#ifdef UMEM_DRIVER
#include "../machine/umem.h"
#endif 

/* move this to a header file after hell freezes over XXX */
#define	iptetopte(P)	((struct pte *) (*(int *)((int)(P) & ~3)))

#ifdef BNR_HPSOS
/*
 * A pointer to a function to call at certain 'strategic locations' in
 * do_buserr.  By default this points to a NOP function; customers may bind
 * in their own implemenation.
 */
int default_fault_hook(es, sig)
struct exception_stack  *es;
int sig;
{
	return 1;	/* Always succeed */
}

int     (*fault_hook)() = default_fault_hook;

#endif /* BNR_HPSOS */

extern vas_t kernvas;

int m68040_fp_emulation_error = 0;

/* return values for do_buserr() */
#define IGNORE_WRITEBACKS	0
#define DO_WRITEBACKS		1
#define REPEAT_WRITEBACKS	2

#define	SUPERVISOR	0x0000		/* supervisor-mode flag added to type */
#define	USER		0x1000		/* user-mode flag added to type       */

char	*trap_type[] = {
	"",	/* Reset Initial SSP */
	"",	/* Reset Initial PC  */
	"Bus error",
	"Address error",
	"Illegal instruction",
	"Zero divide",
	"CHK instruction",
	"TRAPV instruction",
	"Privilege violation",
	"Trace",
	"Line 1010 emulator",
	"Line 1111 emulator",
};

char 	*err_format = "trap type %x, pc = %x, ps = %x\n";
char	*buserr_format = "buserr: pc = %x, ps = %x\n";

#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

#ifdef	SYSCALLTRACE
int	trap2_trace = 0;		/*  ???  */
#endif	SYSCALLTRACE

#ifdef STRACE
int (*log_syscall)() = NULL;   /*  will be set by strace_link if configed in  */
#endif

extern	int printf();
int	(*test_printf)() = printf; /* used only for test code                 */
				   /* do not #ifdef since used outside trap.c */


/*
 * Called from the trap handler when a processor trap occurs.
 */

	/* Central trap handler */
trap(locregs)
struct exception_stack locregs;
{
	register int i;	/* i is known to be in register d7 */
	register struct proc *p;
	time_t syst;
	short delay_signal = 0;
	int temp_for_issig;
	int type = locregs.e_offset & 0xfff;
	extern int fpsr_save;

	cnt.v_trap++;
	syst = u.u_procp->p_sticks;

	if (USERMODE(locregs.e_PS)) { 
		u.u_syscall = SYS_NOTSYSCALL;
		u.u_eosys = EOSYS_NOTSYSCALL;
		type |= USER;
		u.u_ar0 = (int *) &locregs;
	} else 
		type |= SUPERVISOR;

	p = u.u_procp;

	switch (type) {

	default:
		/* Check for probe simulation */
		/* Necessary for address error on 68010 */
		if (u.u_pcb.pcb_flags & PROBE_MASK)
 			panic ("PROBE_MASK");
 		if (u.u_probe_addr != 0) {
 			locregs.e_PC = u.u_probe_addr;
 			u.u_pcb.pcb_flags |= POP_STACK_MASK;
 			return;
 		}

		printf(err_format, type, locregs.e_PC, locregs.e_PS);
		type &= ~USER;
		type >>= 2;
		if ((unsigned)type < TRAP_TYPES)
			panic(trap_type[type]);
		panic("trap");
		break;

	case T_UNIMP_DATA:
		/* MC68040 unimplemented floating point data type error */
		if (processor == M68040)
			panic("T_UNIMP_DATA in supervisory state");
		else
			panic("T_UNIMP_DATA in supervisory state on non-68040");
		break;
	case T_UNIMP_DATA+USER:
		/* MC68040 unimplemented floating point data type error */
		if (processor == M68040) {
			i = m68040_fp_emulation_error;
			u.u_code = 0;
		} else
			panic("T_UNIMP_DATA+USER on non-68040");
		break;
	case T_CPROTO:
	case T_FORMAT:
		psignal(p, SIGFPE);
		longjmp(&u.u_psave);
		break;
	case T_TRAP2+USER:
		/* The 2nd instruction of each procedure is a trap #2, */
		/* followed  by a 16-bit integer indicating the local  */
		/* stack size, to allow non-MMU PISCES systems to grow */
		/* user stacks.  Change trap #2 instruction & 16-bit   */
		/* integer to two no-op instructions (opcode 0x4e71)   */
		/* to avoid repeats.  Since PISCES/2.x object is never */
		/* run as shared text, this should always work.        */
		/* If trap 2's do occur in shared text (possible at    */
		/* least in some pre-release versions of code), it is  */
		/* tricky to avoid race conditions while doing the     */
		/* change, and thus safer to advance the PC over the   */
		/* 16-bit integer and continue.                        */

			/* assume shared text and bump PC */
			locregs.e_PC += 2;
		goto out;

	case T_ILLFLT+USER:
	case T_FORMAT+USER:
		u.u_code = 0;
		i = SIGILL;
		break;

	case T_CHKFLT+USER:
	case T_PRIVFLT+USER:
	case T_TRAPV+USER:
	case T_TRAP3+USER:
	case T_TRAP4+USER:
	case T_TRAP5+USER:
	case T_TRAP6+USER:
	case T_TRAP7+USER:
	case T_TRAP9+USER:
	case T_TRAP10+USER:
	case T_TRAP11+USER:
	case T_TRAP12+USER:
	case T_TRAP13+USER:
	case T_TRAP14+USER:
	case T_TRAP15+USER:
		u.u_code = (type &~ USER) >> 2;
		i = SIGILL;
		break;

	case T_LINEA+USER:
		u.u_code = 0;
		i = SIGIOT;
		break;

	case T_LINEF+USER:
		u.u_code = 0;
		/* MC68040 unimplemented floating point instruction? */
		if (processor == M68040 && m68040_fp_emulation_error) {
			i = m68040_fp_emulation_error;
			m68040_fp_emulation_error = 0;
			break;
		}
		i = SIGEMT;
		break;
	/* fp coprocessor errors in supervisory space */
	case T_BSUN:
	case T_INEX:
	case T_ZDIV:
	case T_UNDRFLW:
	case T_OPERR:
	case T_OVRFLW:
	case T_SNAN:
		u.u_code = (fpsr_save & 0x7fffffff); /* high bit means dragon */

		if (u.u_probe_addr == 0)
			panic ("fp error in supervisory state: probe not set");
		else {
			printf("longjumping on fp error");
			locregs.e_PC = u.u_probe_addr;
			u.u_pcb.pcb_flags |= POP_STACK_MASK;
			return;
		}
		break;
	/* fp coprocessor errors in user space */
	case T_BSUN+USER:
	case T_INEX+USER:
	case T_ZDIV+USER:
	case T_UNDRFLW+USER:
	case T_OPERR+USER:
	case T_OVRFLW+USER:
	case T_SNAN+USER:
		/* save the faulting address */
		u.u_pcb.pcb_fpiar = get_fpiar();
		if (processor != M68040) {
			/* The 68882 can generate mid-instruction exceptions */
			if ((locregs.e_offset & 0xf000) == 0x9000)
				delay_signal = 1;
		}
		u.u_code = (fpsr_save & 0x7fffffff); /* high bit means dragon */
		i = SIGFPE;
		break;

	case T_CPROTO+USER:
		u.u_code = 0;
		i = SIGFPE;
		break;

	case T_ADDRFLT+USER:
		i = SIGBUS;
#ifdef BNR_HPSOS
		(*fault_hook)(&locregs, i);
#endif /* BNR_HPSOS */
		break;

	case T_ARITHTRAP+USER:
		u.u_code = (type & ~USER) >> 2;
		i = SIGFPE;
		break;

	case T_TRAP8+USER:	/* error mechanism for float libraries */
		u.u_code = 0;
		i = SIGFPE;
		break;

	case T_TRCTRAP+USER:	/* trace trap */
		if (!(u.u_pcb.pcb_flags & USER_TRACE_MASK)) {
			/* trace bit set previously in buserror() to catch signal */
			locregs.e_PS &= ~PSL_T;
			goto out;
		} /* else fall through */
	case T_BPTFLT+USER:	/* bpt instruction fault */
		u.u_pcb.pcb_flags &= ~USER_TRACE_MASK;
		locregs.e_PS &= ~PSL_T;
#ifdef UMEM_DRIVER
		if (mm_trace_trap() == MM_IGNORE)
			goto out;
#endif 
		i = SIGTRAP;
		break;

	case T_TRCTRAP:		/* trace trap and supervisor - must be  */
				/* tracing during trap instruction.     */
		if (u.u_pcb.pcb_flags & USER_TRACE_MASK) {
#ifdef UMEM_DRIVER
			if (mm_trace_trap() == MM_IGNORE) {
				u.u_pcb.pcb_flags &= ~USER_TRACE_MASK;
				locregs.e_PS &= ~PSL_T;
				return;
			}
#endif 
			psignal(p, SIGTRAP);
			u.u_pcb.pcb_flags &= ~USER_TRACE_MASK;
			locregs.e_PS &= ~PSL_T;
		}
		return;
	
	case T_SPURIOUS:	/* spurious interrupt */
	case T_SPURIOUS+USER:	/* spurious interrupt */
		printf("Spurious interrupt\n");
		return;

	case T_RESCHED:		/* parity errors come here to resched cpu */
		return;

	case T_RESCHED+USER:
		if ((p->p_flag & SOWEUPC) && u.u_prof.pr_scale) {
			addupc(locregs.e_PC, &u.u_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		if ((locregs.e_offset & 0xf000) == 0x9000)
			delay_signal = 1;
		goto out;
	}

	psignal(p, i);
out:
	if (p->p_cursig || ISSIG_EXIT(p, temp_for_issig)) {
		if (delay_signal && (p->p_sigcatch & (1 << (p->p_cursig-1)))) {
			/* We need to continue an interrupted instruction */
			/* and can't invoke a signal handler now without  */
			/* disturbing the exception vector on the stack.  */
			/* Pretend we haven't seen the signal yet, and    */
			/* make sure we get a trace trap soon to get the  */
			/* signal (no problem if tracing is already on).  */
			psignal (p, p->p_cursig);
			p->p_cursig = 0;
			locregs.e_PS |= PSL_T;
		} else
			psig();
	}

	p->p_pri = p->p_usrpri;
	p->p_flag |= SSIGABL;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		i = spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		splx(i);
#ifndef RTPRIO_DEBUG
	}
#else RTPRIO_DEBUG
	}else{
		/*
		 * just testing to see if everything is set up
		 * with the right priorities
		 */
		rqtest(p->p_pri,1);
	 }
#endif RTPRIO_DEBUG
	if (u.u_prof.pr_scale) {
		syst = u.u_procp->p_sticks - syst;
		if (syst && USERMODE(locregs.e_PS))
			addupc(locregs.e_PC, &u.u_prof, syst);
	}
	curpri = p->p_pri;
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{
	register struct proc *p = u.u_procp;

	if (u.u_signal[SIGSYS-1] == SIG_IGN || p->p_sigmask & (1 << (SIGSYS-1)))
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}

/*
 * nonexistent NFS system call.  Don't want to dump core.  Will return
 * a reasonable errno, which can be checked by caller.  This default will
 * be replaced by NFS function if NFS is configured in (in nfsc_link).
 */

nfs_nosys()
{
	u.u_error = ENOPROTOOPT;
}

/*
 * Called from the trap handler when a system call occurs
 */

/* syscall
 * 
 * RFA Requirements and Modifications
 * ----------------------------------
 *
 *	Before each system call, clear the flag (u.u_rfa.ur_wasremote)
 *	which indicates that a system call failed because it found a 
 *	remote file.  
 *
 *	After each system call, if u.u_error != 0, check u.u_rfa.ur_wasremote.
 *	if it is non-zero, call rfa_syscall with the call number
 *	(index into sysent table) as a parameter.
 *
 *	Note - the RFA code only knows about the standard sysent table, not
 *	       the compatibility one.  This is okay as long as there are no
 *	       conflicts in the calls which RFA knows about.  This is the
 *	       case now (13 March, 1985)
 */

syscall(locregs)
struct exception_stack locregs;
{
	register struct user *uap_a5 = &u; 
	register struct proc *procp_a4;
	register struct sysent *callp;
	register int *params;
	time_t syst;
	int temp_for_issig;

	cnt.v_syscall++;
	syst = uap_a5->u_procp->p_sticks;
#ifdef SYSCALLTRACE
	if (!USERMODE(locregs.e_PS))
		panic("syscall");
#endif SYSCALLTRACE
	uap_a5->u_ar0 = (int *) &locregs;
	uap_a5->u_error = 0;
	{
		register unsigned int code;
		/* get d0 passed from user call */
		code = locregs.e_regs[R0];

		/* get parameter pointer from users stack */
		params = (((int *)locregs.e_regs[SP]) + 1);
	
		/* Test for (code == 0) rather than (callp == sysent), */
		/* since SYS_NOSYS might be zero.                      */
		if (code == 0) code = fuword(params++);
	
		/* check if code is within the table range */
		if (code >= uap_a5->u_pcb.pcb_nsysent) code = SYS_NOSYS;

		callp = &uap_a5->u_pcb.pcb_sysent_ptr[code];

		/* if not zero parameters, copy users stack into u_area */
		if (callp->sy_narg) 
			copyin4(params, (caddr_t)uap_a5->u_arg, callp->sy_narg);
	
		uap_a5->u_syscall = code;
	/* done with register code, (see special setjmp used below) */
	} 
	uap_a5->u_ap = uap_a5->u_arg;
	uap_a5->u_r.r_val1 = 0;
/*	uap_a5->u_r.r_val2 = locregs.e_regs[R1]; */

	/* PC, A6, SP are only regs that need to be saved */

/*    		setjmp_min(&uap_a5->u_qsave)    EMULATION       	*/
/*									*/
/*  	if (setjmp_min(&uap_a5->u_qsave))      				*/
/*  		goto longjmp_it;	       				*/
/*									*/
/*  label_t = PC, D2, D3, D4, D5, D6, D7, A2, A3, A4, A5, A6, A7 	*/
/*             0   4   8  12  16  20  24  28  32  36  40  44  48 	*/
/*              U_QSAVE_PC = u_qsave +  0                               */
/*              U_QSAVE_SP = u_qsave + 44                               */
/*              U_QSAVE_PC = u_qsave + 48                               */

asm("   mov.l   %a6,U_QSAVE_A6(%a5)             # save     A6 (label_t) ");
asm("	lea	-8(%sp),%a0			# adj SP for _longjmp  	");
asm("   mov.l   %a0,U_QSAVE_SP(%a5)             # save     SP (label_t) ");
asm("   mov.l   &longjmp_rtn,U_QSAVE_PC(%a5)    # save     PC (label_t) ");

	uap_a5->u_eosys = EOSYS_NORMAL;
#ifdef STRACE
	if (log_syscall && (uap_a5->u_procp->p_flag & SSCT))
		(*log_syscall)(uap_a5->u_procp->p_pid, TL_ENTRY);
#endif		
#ifdef SYSCALLTRACE
	if (SCT_ON (SCT_ENTRY, uap_a5->u_procp, uap_a5->u_syscall)) {
		register int i;
		char *cp;

		if (uap_a5->u_pcb.pcb_sysent_ptr != sysent)
			(*test_printf) ("compat ");
		if (uap_a5->u_syscall >= nsysent)
			(*test_printf)("0x%x", uap_a5->u_syscall);
		else
			(*test_printf)("%s", sysent[uap_a5->u_syscall].sy_name);
		cp = "(";
		for (i= 0; i < callp->sy_narg; i++) {
			(*test_printf)("%s%x", cp, uap_a5->u_arg[i]);
			cp = ", ";
		}
		if (i)
			(*test_printf)(")");
		(*test_printf)(" - \"%s\" - process %d\n",
			uap_a5->u_comm, uap_a5->u_procp->p_pid);
	}
#endif			

	(*(callp->sy_call))(uap_a5->u_ap);

return_from_lnjmp:
		/* make sure this is done AFTER longjmp */
		procp_a4 = uap_a5->u_procp;

	if (uap_a5->u_error == EOPCOMPLETE)
		uap_a5->u_error = 0;
#ifdef AUDIT
	if (AUDITEVERON())
		save_aud_data();			/* auditing support */
#endif
#ifdef STRACE
	if (log_syscall && (uap_a5->u_procp->p_flag & SSCT))
		if (uap_a5->u_error) 
			(*log_syscall)(uap_a5->u_procp->p_pid, TL_ERR);
		else		    /*  if uap_a5->u_eosys == EOSYS_NORMAL?  */
			(*log_syscall)(uap_a5->u_procp->p_pid, TL_EXIT);
#endif			
	if (procp_a4->p_cursig || ISSIG_EXIT(procp_a4, temp_for_issig))
			psig();
	
	if (uap_a5->u_error) {
		locregs.e_regs[R0] = uap_a5->u_error;
		locregs.e_PS |= PSL_C;	/* carry bit */

#ifdef SYSCALLTRACE
		if (SCT_ON (SCT_ERROR_EXIT, procp_a4, uap_a5->u_syscall) &&
		    SCT_BIT (sct_errno_mask, uap_a5->u_error)) {
			register int i;
			char *cp;

			if (uap_a5->u_pcb.pcb_sysent_ptr != sysent)
				(*test_printf) ("compat ");
			if (uap_a5->u_syscall >= nsysent)
				(*test_printf)("0x%x", uap_a5->u_syscall);
			else
				(*test_printf)("%s",
					sysent[uap_a5->u_syscall].sy_name);
			if (uap_a5->u_syscall != SYS_SIGCLEANUP) {
				cp = "(";
				for (i= 0; i < callp->sy_narg; i++) {
					(*test_printf)("%s%x",
							cp, uap_a5->u_arg[i]);
					cp = ", ";
				}
				if (i)
					(*test_printf)(")");
			}
			(*test_printf)(" - \"%s\" - process %d - errno %d",
				uap_a5->u_comm, procp_a4->p_pid, uap_a5->u_error);
			(*test_printf) ("\n");
		}
#endif SYSCALLTRACE

	} else if (uap_a5->u_eosys == EOSYS_NORMAL) {
		locregs.e_regs[R0] = uap_a5->u_r.r_val1;
		locregs.e_regs[R1] = uap_a5->u_r.r_val2;
		locregs.e_PS &= ~PSL_C;

#ifdef SYSCALLTRACE
		if (SCT_ON (SCT_NORMAL_EXIT, procp_a4, uap_a5->u_syscall)) {
			register int i;
			char *cp;

			if (uap_a5->u_pcb.pcb_sysent_ptr != sysent)
				(*test_printf) ("compat ");
			if (uap_a5->u_syscall >= nsysent)
				(*test_printf)("0x%x", uap_a5->u_syscall);
			else
				(*test_printf)("%s",
					sysent[uap_a5->u_syscall].sy_name);
			if (uap_a5->u_syscall != SYS_SIGCLEANUP) {
				cp = "(";
				for (i= 0; i < callp->sy_narg; i++) {
					(*test_printf)("%s%x",
							cp, uap_a5->u_arg[i]);
					cp = ", ";
				}
				if (i)
					(*test_printf)(")");
			}
			(*test_printf)
				(" - \"%s\" - process %d - returning 0x%x",
				uap_a5->u_comm, procp_a4->p_pid,
				uap_a5->u_r.r_val1);
			if (uap_a5->u_r.r_val2 != 0)
				(*test_printf) (", 0x%x", uap_a5->u_r.r_val2);
			(*test_printf) ("\n");
		}
#endif SYSCALLTRACE

	} else if (uap_a5->u_eosys == EOSYS_RESTART) {
		locregs.e_PC -= 2;	/* back up over TRAP #0 instruction */
	}
	/* else (EOSYS_INTERRUPTED or EOSYS_NOTSYSCALL) -      */
	/* registers OK as set up by sendsig() or sigcleanup() */

/*	uap_a5->u_eosys = EOSYS_NOTSYSCALL;  done on other entrys */
/*	uap_a5->u_syscall = SYS_NOTSYSCALL;  done on other entrys */

	procp_a4->p_pri = procp_a4->p_usrpri;
/*	procp->p_flag |= SSIGABL;  sleep takes care of this */
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()''ed, we might not be on the queue indicated by
		 * our priority.
		 */
		register int i;
		i = spl6();
		setrq(procp_a4);
		uap_a5->u_ru.ru_nivcsw++;
		swtch();
		splx(i);
	}
#ifdef RTPRIO_DEBUG
	else{
		/*
		 * just testing to see if everything is set up
		 * with the right priorities
		 */
		rqtest(p->p_pri,2);
	 }
#endif RTPRIO_DEBUG
	if (uap_a5->u_prof.pr_scale) {
		syst = uap_a5->u_procp->p_sticks - syst;
		if (syst && USERMODE(locregs.e_PS)) {
			if (    syst < 0
			    &&  (  uap_a5->u_syscall == SYS_FORK
				|| uap_a5->u_syscall == SYS_VFORK ) ) {

			    syst = uap_a5->u_procp->p_sticks;
			}
			addupc(locregs.e_PC, &uap_a5->u_prof, syst);
		}
	}
	curpri = procp_a4->p_pri;
	return;

asm("longjmp_rtn:						");
longjmp_rtn:
		/* restore a5 because we did not save */
		uap_a5 = &u; 
		if (uap_a5->u_error == 0 && uap_a5->u_eosys == EOSYS_NORMAL)
			uap_a5->u_error = EINTR;
#ifdef	SYSCALLTRACE
		/* restore callp since it was not saved and it   */
		/* may be used by SYSCALLTRACE after the longjmp */
		callp = (uap_a5->u_syscall >= uap_a5->u_pcb.pcb_nsysent) ?
		 	&uap_a5->u_pcb.pcb_sysent_ptr[SYS_NOSYS] :
		 	&uap_a5->u_pcb.pcb_sysent_ptr[uap_a5->u_syscall];
#endif	SYSCALLTRACE
		goto return_from_lnjmp;
}

/*
 * Called from the trap handler when a buserror occurs
 */

extern char panic_msg[];
extern int panic_msg_len;

buserr_panic(locregs)
struct exception_stack *locregs;
{
	char *trap_msg;
	int int_lvl;
	

	int_lvl = (locregs->e_PS>>8)&0x0f;
	if (int_lvl < 1 || int_lvl > 5)
		trap_msg = "in syscall or kernel process";
	else
		trap_msg = "handling device interrupt";
	sprintf(panic_msg, panic_msg_len,
	        "bus error while %s (IL=%d, type=0x%x, PC=0x%x)",
	    	trap_msg, int_lvl, locregs->e_offset & 0xf000, locregs->e_PC);
	panic(panic_msg);
}

#define	SG_OFFSET(x)	(((x) & SG_IMASK) >> SG_ISHIFT)

/*
 * Decode stack frame & tell what status word and fault address are
 */
get_fault(locregs, vaddr, psw, rfc)
    register struct exception_stack *locregs;
    caddr_t *vaddr;
    u_short *psw;
    int *rfc;
{
    register u_int fault_address;
    register u_short ssw;
    register int fc = 0;	/* Default to user access */

	/* Decode stack frame */
    switch(locregs->e_offset & 0xf000) {
    case 0x8000:
	    panic("68010 trap");
    case 0xa000:
	    /* must or in FB/FC if RB/RC set -- see page 6-17 of manual */

	    ssw = locregs->e_union.e_68020_short.e_ssw;
	    if (ssw & DF) {
		fc = ssw & AS;
		fault_address = locregs->e_union.e_68020_short.e_address;
	    } else if (ssw & FB)
		fault_address = locregs->e_PC + 4;
	    else if (ssw & FC)
		fault_address = locregs->e_PC + 2;
	    else {
		buserr_panic(locregs);
	    }
	    if (ssw & RB)
		    locregs->e_union.e_68020_short.e_ssw |= FB;
	    if (ssw & RC)
		    locregs->e_union.e_68020_short.e_ssw |= FC;
	    break;

    case 0xb000:
	    /* must or in FB/FC if RB/RC set -- see page 6-17 of manual */

	    ssw = locregs->e_union.e_68020_long.e_ssw;
	    if (ssw & DF) {
		fc = ssw & AS;
		fault_address = locregs->e_union.e_68020_long.e_address;
	    } else if (ssw & FB)
		fault_address = locregs->e_union.e_68020_long.e_stage_b;
	    else if (ssw & FC)
		fault_address = locregs->e_union.e_68020_long.e_stage_b - 2;
	    else {
		buserr_panic(locregs);
	    }
	    if (ssw & RB)
		    locregs->e_union.e_68020_long.e_ssw |= FB;
	    if (ssw & RC)
		    locregs->e_union.e_68020_long.e_ssw |= FC;
	    break;

    default:
	    buserr_panic(locregs);
	    break;
    }
    *vaddr = (caddr_t)fault_address;
    *psw = ssw;
    *rfc = fc;
}

int get_writeback_status(vas, vaddr)
vas_t *vas;
caddr_t vaddr;
{
	struct pte *pte;
	extern int wb_error, wbl_err;

	/* 
	 * Only the 68040 cares about writebacks.
	 */
	if (processor != M68040)
		return 0;

        /*
         * Always do writebacks for the kernel vas.
         */
        if (vas == KERNVAS)
                return DO_WRITEBACKS;

	/* 
	 * The fault handler is claiming that it has fixed the fault but under
	 * some conditions vfault makes this claim even when it hasn't.  If it 
	 * hasn't fixed the fault then vfaut is trying to tell us to retry the
	 * access.  For 68040 writebacks this can lead to infinite recursion
	 * crashing the system.  Here we must detect this case and let the 
	 * 68040 bus error handler know.
	 */
	if ((u.u_probe_addr == (int)&wb_error) ||
	    (u.u_probe_addr == (int)&wbl_err)) {
		/* 
		 * This is a writeback that faulted, see
		 * if the fault handler really fixed the fault.
		 */
		pte = vastopte(vas, vaddr);
		if (!pte || !pte->pg_v || pte->pg_prot) {
			/* 
			 * The fault didn't get fixed.  Tell the 68040 handler
			 * it needs to reserve memory prior to the next try.
			 */
			return REPEAT_WRITEBACKS;
		}
	}

	/*
	 * The fault is fixed and the subsequent writeback will succceed.
	 */
	return DO_WRITEBACKS;
}
/*
 * Common code for all MMUs.  pmmubuserror() and buserror() have
 *  decoded their respective MMU, and call us with the parameters
 *  for resolving the error.  The return value is used by the 
 *  68040 code to know whether or not to do writebacks.  1 means
 *  skip valid writebacks and 0 means do them.
 */
do_buserr(locregs, addr, write, wfault, bus, invalid, fc)
    register struct exception_stack *locregs;
    caddr_t addr;	/* Addr of fault */
    int write,		/* Addr was written to */
	wfault,		/* Write fault resulted */
	bus,		/* Bus error resulted */
	invalid,	/* PTE/STE was not valid */
	fc;		/* Function code from status word */
{
	struct proc *p = u.u_procp;
	time_t syst;
	register int i;
	register int state, tmp_uerr, delay_signal = 0;
	int temp_for_issig;
	vas_t *vas;
	extern int dragon_present;
	extern int pmmu_exist;
	struct pte *pt;

	/* Record when we came in */
	syst = u.u_procp->p_sticks;

	/* save the faulting address */
	u.u_pcb.pcb_fault_addr = (u_int)addr;

	/* Flag for coming from user-land */
	if (USERMODE(locregs->e_PS)) {
		u.u_syscall = SYS_NOTSYSCALL;
		u.u_eosys = EOSYS_NOTSYSCALL;
		u.u_ar0 = (int *) locregs;
		state = USER;
	} else
		state = SUPERVISOR;

	/*
	 * Function code 0 means that it wasn't a data reference (and thus
	 *  a function code isn't provided in the SSW).  Since we're talking
	 *  text reference, we will turn off the write bit if the translation
	 *  wasn't valid at all.  This keeps the upper layers from doing
	 *  copy-on-write actions on pure text pages.
	 */
	if ((fc == 0) && invalid)
		write = 0;

	/* Reasons for being here from kernel */
	if (state == SUPERVISOR) {

		/*
		 * Kernel references & page faults to user
		 *  addresses are fine so long as we set up for it up front.
		 */
		if (fc == 1) {
			if (!u.u_probe_addr) {
	printf("Bad kernel reference to user address 0x%x\n", addr);
				panic("user addr");
			}
			vas = u.u_procp->p_vas;
		} else {
			vas = &kernvas;
		}
	} else {
		vas = p->p_vas;
	}

#ifdef UMEM_DRIVER
	if (mm_buserror(locregs, vas, addr) == MM_IGNORE)
		goto out;	/*		return DO_WRITEBACKS;	  */
#endif 

	/* see if it was a fault to an unmapped dragon card */

	if ((fc == 1) && (addr >= DRAGON_AREA) &&
	   ((addr <= (DRAGON_AREA + DRAGON_PAGES*NBPG)))) {

		/* 
		** Check for dragon card.
		*/
		if (dragon_present && (u.u_pcb.pcb_dragon_bank == -1)) {
			/*
			** If the process gets a signal while sleeping waiting
			** for a dragon bank, just return.  If the signal is
			** going to kill the process we don't care.  If the
			** process is going to catch the signal, trap will
			** deliver it when we return.  When the process returns
			** from its signal handler the instruction that caused
			** this fault will be restarted.
			*/
			if (!setjmp(&u.u_qsave))
				dragon_buserror();
			return DO_WRITEBACKS;
		}
	}

	/* Process stuff which bounced us here from user-land */
	tmp_uerr = u.u_error;		/* Save because of copyin/out */

	/*
	 * Translation fault.  vfault() will resolve it, or
	 *  return a signal for the user to get.
	 */
	if (invalid && !wfault) {
		if ((i = vfault( vas, write, vas, addr)) == 0) {
			if (state == SUPERVISOR) {
				return get_writeback_status(vas, addr);
			}
#ifdef BNR_HPSOS
			if ((processor == M68040) &&
			    (!(*fault_hook)(locregs, SIGBUS))) {
                                i = SIGBUS;
                                goto sig;
                        }
#endif /* BNR_HPSOS */
			delay_signal = 1;
                        goto out;
                }
#ifdef BNR_HPSOS
		/* Notify hook of a bus error */
		(*fault_hook)(locregs, i);
#endif /* BNR_HPSOS */
                goto sig;
	}

	/*
	 * If pmmu, wfault uniquely identifies a protection
	 * fault so, just call pfault.
	 *
	 * If hpmmu (WOPR), we must get the pte to see
	 * what kind of fault this is (protection or
	 * validity) because the hardware mmu status register
	 * does not provide the information.
	 */
	if (!wfault) {
		/* Bus errors through valid bits means "real" bus faults */
		i = SIGBUS;
#ifdef BNR_HPSOS
		/* Notify hook of a bus error (not a write violation). */
		(*fault_hook)(locregs, SIGSEGV);
#endif /* BNR_HPSOS */
	}
	else if (pmmu_exist || (processor == M68040) ||
		 ((pt = vastopte(vas, addr)) && pt->pg_v)) {
		/*
		 * Write protect fault.  pfault() will resolve it, or
		 * return a signal for the user to get.
		 */
#ifdef BNR_HPSOS
		if (! (*fault_hook)(locregs, SIGBUS)) {
			i = SIGBUS;
			goto sig;
		}
#endif /* BNR_HPSOS */
		i = pfault(vas, write, vas, addr);
	} else {
		i = vfault(vas, write, vas, addr);
#ifdef BNR_HPSOS
		if (i != 0) {
			/* Notify hook of a bus error */
			(void) (*fault_hook)(locregs, i);
		}
#endif /* BNR_HPSOS */
	}

	if (i==0) {
		if (state == SUPERVISOR) {
			return get_writeback_status(vas, addr);
		}
		delay_signal = 1;
		goto out;
	}


sig:
	u.u_pcb.pcb_signal = i;

	/*
	 * Unsatisfied references to user-land from supervisor when probe
	 *  is set means a fubyte/fuword-type operation failed.  Arrange
	 *  for it to flag errors for itself.  If it wasn't a probe, panic.
	 */
	if (state == SUPERVISOR) {
		if (u.u_probe_addr) {
			locregs->e_PC = u.u_probe_addr;
			u.u_pcb.pcb_flags |= POP_STACK_MASK;
			return IGNORE_WRITEBACKS;
		}
		if (vas == &kernvas)
			printf("Kernel bus error on address 0x%x\n", addr);
		else
			printf("Kernel reference to bad user addr 0x%x\n", addr);

		buserr_panic(locregs);
	}

	/* Signal a user process for their sins */
	psignal(p, i);

out:
#ifdef UMEM_DRIVER
	if (p->p_flag & SUMEM)
		umem_bus_return(locregs, vas, addr);
#endif		
	u.u_error = tmp_uerr;
	if (u.u_error == EOPCOMPLETE) 
		u.u_error = 0;
	if (p->p_cursig || ISSIG_EXIT(p, temp_for_issig)) {
		if (delay_signal && (p->p_sigcatch & (1 << (p->p_cursig-1)))) {
			/* We need to continue an interrupted instruction */
			/* and can't invoke a signal handler now without  */
			/* disturbing the exception vector on the stack.  */
			/* Pretend we haven't seen the signal yet, and    */
			/* make sure we get a trace trap soon to get the  */
			/* signal (no problem if tracing is already on).  */
			psignal (p, p->p_cursig);
			p->p_cursig = 0;
			locregs->e_PS |= PSL_T;
		} else {
			if (processor == M68040) {
				u.u_pcb.pcb_flags |= M68040_WB_MASK;
				bcopy(locregs, u.u_pcb.pcb_locregs, 
					sizeof(struct mc68040_exception_stack));
			}
			psig();
		}
	}
	p->p_pri = p->p_usrpri;
	p->p_flag |= SSIGABL;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		i = spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		splx(i);
	}
	if (u.u_prof.pr_scale) {
		syst = u.u_procp->p_sticks - syst;
		if (syst && USERMODE(locregs.e_PS))
			addupc(locregs->e_PC, &u.u_prof, syst);
	}
	curpri = p->p_pri;
	return DO_WRITEBACKS;
}

/*
 * Handle PMMU bus error
 *
 *     The following 2 variables are only referenced in
 *     pmmubuserror(), however they need to be file global
 *     so that we can use them in asm statements.
 */

static u_int p_fault_fc; /* fault function code   */
static u_int p_fault_addr; /* fault address   */
static u_int pmmu_sr;      /* pmmu status reg */

pmmubuserror(locregs)
struct exception_stack locregs;
{
    int fc;
    u_int fault_address;
    u_short ssw;
    register u_int mmu_sr;
    int wfault = 0;

    /* Find out where it happened */

    get_fault(&locregs, &fault_address, &ssw, &fc);

    p_fault_addr = fault_address;
    p_fault_fc = fc;

    /*
     * Get status register
     */

    asm("       mov.l   %a0,-(%sp) ");          /* save registers */
    asm("       mov.l   %a1,-(%sp) ");
    asm("       mov.l   %d0,-(%sp) ");
    asm("	mov.l	_p_fault_addr,%a0 ");    
    asm("       mov.l   &_pmmu_sr,%a1 ");
    asm("       mov.l   _p_fault_fc,%d0 ");
    asm("       long    0xf0109e08 ");          /* ptest %d0,(a0),#7 */
    asm("       long    0xf0116200 ");          /* pmove psr,(a1)   */
    asm("       mov.l   (%sp)+,%d0 ");          /* restore registers */
    asm("       mov.l   (%sp)+,%a1 ");
    asm("       mov.l   (%sp)+,%a0 ");
    mmu_sr = pmmu_sr;

    /*
     * When the PMMU doesn't have a translation in the ATC during a read-
     *  modify-write sequence, it is unable to do the table walk to get it
     *  because the '020 won't relinquish the bus (it's trying to get an
     *  atomic bus sequence, I guess).  So the PMMU bus errors the '020,
     *  aborting the sequence entirely. We need to differentiate a real
     *  fault from an ATC miss (which can only happen on a 68020/68851).
     *  If mmu_sr does not indicate any real fault condition then it must
     *  have been an ATC miss. Do a pload to fix the problem and then just
     *  return.
     */

    if(    processor == M68020 && (ssw & DF) && (ssw & RM)
       && (mmu_sr & (PMMU_B|PMMU_W|PMMU_I)) == 0) {

	asm("   mov.l   %a0,-(%sp) ");          /* save registers */
        asm("   mov.l   %d0,-(%sp) ");
	asm("   mov.l   _p_fault_addr,%a0 ");
        asm("   mov.l   _p_fault_fc,%d0 ");
	asm("   long    0xf0102008");           /* pload %d0,(a0) */
        asm("   mov.l   (%sp)+,%d0 ");          /* restore registers */
	asm("   mov.l   (%sp)+,%a0 ");

	return;
    }

    /* if it was a write and the pte was vaild and it was write protected */
    if (((ssw&(RM|RRW)) != RRW) && ((mmu_sr&PMMU_I) == 0) && (mmu_sr&PMMU_W))
	wfault = 1;

    /* Call central routine to handle bus error */

    (void) do_buserr( &locregs, fault_address, (ssw & (RM|RRW)) != RRW, wfault,
					mmu_sr & PMMU_B, mmu_sr & PMMU_I, fc);
}

/*
 * Bus error handler for HP's MMU
 */
buserror(locregs)
struct exception_stack locregs;
{
    register u_short mmu_sr;
    caddr_t fault_address;
    u_short ssw;
    int fc;

	/* Get MMU status, clear it */
    mmu_sr = *((u_short *) (MMU_BASE + 0x0e));
    *((u_short *) (MMU_BASE+0x0e)) = mmu_sr& ~(MMU_PTF|MMU_PF|MMU_WPF|MMU_BERR);

	/* Find out status & location of fault */
    get_fault(&locregs, &fault_address, &ssw, &fc);

	/* Call central routine to handle */
    (void) do_buserr( &locregs, fault_address, (ssw&RRW) == 0, mmu_sr & MMU_WPF,
			mmu_sr & MMU_BERR, mmu_sr & (MMU_PTF|MMU_PF), fc );
}

#ifdef OSDEBUG

writeback_check(fc, stage)
int fc, stage;
{
#ifdef WB_DEBUG
	switch (fc) {
	case 1: /* User data space */
		/* 
		** We will only notice the unusual ones.  This is a
		** plain old normal boaring one so we will ignore it.
		*/
		break;

	case 2: /* User program space */
		printf("writeback to user text in stage %d\n", stage);
		break;

	case 5: /* Supervisory data space */
		printf("writeback to supervisory data in stage %d\n", stage);
		break;

	case 6: /* Supervisory program space */
		printf("writeback to supervisory text in stage %d\n", stage);
		printf("I think we're in deep do-do now\n");
		break;

	case 7: /* CPU space */
		printf("writeback to CPU space in stage %d\n", stage);
		panic("I'm going to sprout wings and fly to the moon");
		break;

	case 0: /* undefined */
	case 3: 
	case 4:
	default:
		printf("Weird fc in writeback stage %d, fc = %d\n", stage, fc);
		panic("Dumping core, dumping toxic waste, dumping ...");
	}
#endif /* WB_DEBUG */
}
#endif /* OSDEBUG */

/*
 * Handle MC68040 writebacks
 */
writeback_handler(rp)
struct mc68040_exception_stack *rp;
{
	register u_short ssw = rp->e_ssw;
	int ssw_push = 0;

	/* Complain if this isn't an MC68040 access error exception */
	VASSERT((rp->e_offset & FORMAT_BITS) == MC68040_ACCESS_EXCEPTION);

	/* complete any pending writebacks */
	if (WB_VALID(rp->e_wb1s)) {
#ifdef OSDEBUG
		writeback_check(WB_TM(rp->e_wb1s), 1);
#endif /* OSDEBUG */

		if (WB_TT(rp->e_wb1s) == 1) {
			/*
			** This was a move16 physical bus access error.  Write
			** PD0-PD3, using translated WB1A, clearing bits A3:0.
			*/
#ifdef OSDEBUG
			printf("move16 physical bus access error\n");
#endif /* OSDEBUG */
			if (writeback_line(rp->e_wb1a&0xfffffff0, &rp->e_pd0, 
						WB_TM(rp->e_wb1s))) {
				/* writeback failed, signal the process */
				writeback_signal(rp);
			}
				
		} else {
			/*
			** This was a normal physical bus error.
			*/
			int size;
			int alignment;
			unsigned int data;
	
			size = WB_SIZE(rp->e_wb1s);
			alignment = rp->e_wb1a & 3;

			switch (size) {
			case 0: /* byte */
				data = rp->e_wb1d >> ((3 - alignment) * 8);
				break;
			case 1: /* word */
				if (alignment == 3)
					data = (rp->e_wb1d << 8) | (rp->e_wb1d >> 24);
				else	
					data = rp->e_wb1d >> ((2-alignment)*8);

				break;
			case 2: /* long */
				if (alignment)
					data = (rp->e_wb1d << (alignment*8)) | 
                                           (rp->e_wb1d >> (32 - (alignment*8)));
				else
					data = rp->e_wb1d;
				break;
			case 3: /* line */
				data = rp->e_wb1d;
				break;
			default: /* bummer */
				panic("wbs1: unexpected size");
			}

			if (writeback(rp->e_wb1a, data, WB_SIZE(rp->e_wb1s),
                                      WB_TM(rp->e_wb1s))) {
				/* writeback failed, signal the process */
				writeback_signal(rp);
			}
		} 
	}
	
	/* Check for physical push access errors */
	if ((WB_TT(ssw) == 0) && (WB_TM(ssw) == 0)) {

		if (WB_VALID(rp->e_wb1s))
			panic("physical push access error with wbs1 valid");

		/* Remember that we did this for the WB2 stage. */
		ssw_push = 1;

		/* 
		** We had a physical push access error.  Write PD0-PD3 using
		** untranslated FA, clearing bits A3:0.
		*/
#ifdef OSDEBUG
		printf("physical push access error\n");
#endif /* OSDEBUG */

		/* The kernel has a transparent translation mapping for this */
		if (writeback_line(rp->e_address & 0xfffffff0, &rp->e_pd0, 5)) {
			/* writeback failed, signal the process */
			writeback_signal(rp);
		}
	}

	if (WB_VALID(rp->e_wb2s) && (WB_TT(rp->e_wb2s) != 1)) {
#ifdef OSDEBUG
		writeback_check(WB_TM(rp->e_wb2s), 2);
#endif /* OSDEBUG */

		/*
		** Writeback completion for WB2 stage.
		*/
		if (writeback(rp->e_wb2a, rp->e_wb2d, WB_SIZE(rp->e_wb2s), 
			      WB_TM(rp->e_wb2s))) {
			writeback_signal(rp);
		}
	}

	if (WB_VALID(rp->e_wb2s) && WB_TT(rp->e_wb2s) == 1 && 
				   !WB_VALID(rp->e_wb1s) && !ssw_push) {
		if (writeback_line(rp->e_wb2a & 0xfffffff0, &rp->e_pd0,
			           WB_TM(rp->e_wb2s))) {
			writeback_signal(rp);
		}
	}

	if (WB_VALID(rp->e_wb3s)) {
#ifdef OSDEBUG
		writeback_check(WB_TM(rp->e_wb3s), 3);
#endif /* OSDEBUG */

		/*
		** Writeback completion for WB3 stage.
		*/
		if (writeback(rp->e_wb3a, rp->e_wb3d, WB_SIZE(rp->e_wb3s), 
			      WB_TM(rp->e_wb3s))) {
			writeback_signal(rp);
		}
	}
}

writeback_signal(locregs)
register struct exception_stack *locregs;
{
	struct proc *p = u.u_procp;
	time_t syst;
	int i;
	int temp_for_issig;
	int signal = u.u_pcb.pcb_signal;

	/* 
	 * if we have already gone through sendsig
	 * then let that signal get delivered.
	 */
	if (locregs->e_PC == (int)((struct user *)user_area)->u_sigcode)
		return;

	/* Record when we came in */
	syst = u.u_procp->p_sticks;

	u.u_syscall = SYS_NOTSYSCALL;
	u.u_eosys = EOSYS_NOTSYSCALL;
	u.u_ar0 = (int *) locregs;

	/* Signal a user process for their sins */
	psignal(p, signal);

	if (p->p_cursig || ISSIG_EXIT(p, temp_for_issig)) {
		u.u_pcb.pcb_flags |= M68040_WB_MASK;
		bcopy(locregs, u.u_pcb.pcb_locregs, 
					sizeof(struct mc68040_exception_stack));
		psig();
	}

	p->p_pri = p->p_usrpri;
	p->p_flag |= SSIGABL;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		i = spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
		splx(i);
	}
	if (u.u_prof.pr_scale) {
		syst = u.u_procp->p_sticks - syst;
		if (syst && USERMODE(locregs.e_PS))
			addupc(locregs->e_PC, &u.u_prof, syst);
	}
	curpri = p->p_pri;
}

/*
 * Handle MC68040 access error exceptions
 */
mc68040_access_error(locregs)
struct mc68040_exception_stack locregs;
{
	register struct mc68040_exception_stack *rp = &locregs;
	register u_short ssw = rp->e_ssw;
	u_int fault_addr;
	register u_int mmu_sr;
	int wprot_fault = 0;
	int fault_status = 0;

	/* Complain if this isn't an MC68040 access error exception */
	VASSERT((rp->e_offset & FORMAT_BITS) == MC68040_ACCESS_EXCEPTION);

	/* If this is a locked access, then pretend it was a write access */

	if (ssw & LK)
	    ssw &= ~RW;

	/* get the fault address */
	if ((ssw & ATC) && ssw & MALIGN) 
		fault_addr = pageup(rp->e_address);
	else
		fault_addr = rp->e_address;

	/* if we had an atc fault get the mmu status for the faulted address */
	if (ssw & ATC)
		mmu_sr = mc68040_ptest(fault_addr, ssw & TM, ssw & RW);
	else
		mmu_sr = MC68040_RESIDENT | MC68040_BUS_ERROR;

	
	/* was it a write protection violation? */
	if ( (mmu_sr & MC68040_RESIDENT) &&
	     (mmu_sr & MC68040_WRITE_PROTECTED) &&
	     (ssw & RW) == 0 )
		wprot_fault = 1;

	/* Call central routine to handle bus error */
	if ((fault_status = do_buserr(&locregs, fault_addr, !(ssw & RW), 
				      wprot_fault, mmu_sr & MC68040_BUS_ERROR,
				      !(mmu_sr & MC68040_RESIDENT),
				      ssw & TM)) == IGNORE_WRITEBACKS) {
		return;
	}

	/*
	 * If we are repeating a writeback then reserve enough memory, sleeping
	 * if necessary, to guarantee that our next attempt to do the writeback
	 * will not fail.  We must do this or we will recurse into oblivian.
	 * We reserve 10 pages, 1 for the page being faulted in and 3 for user
	 * translation table data structures 3 for supervisory tables 1 for 
	 * possible attach lists 2 for possible HIL MALLOCS and 1 for padding.
	 * Most likely all of these will not be needed but we must reserve for 
	 * the worst case because we know memory is tight or we wouldn't be 
	 * here.
	 */
#define SLEEP_OK	1
#define MAXFAULTPAGES	10
	if ((fault_status == REPEAT_WRITEBACKS) && !has_procmemreserved())
		procmemreserve(MAXFAULTPAGES, SLEEP_OK, (reg_t *)0);

	/* complete any pending writebacks */
	writeback_handler(&locregs);

	/*
	 * Give back any unused pages after retrying a writeback.
	 */
	if ((fault_status == REPEAT_WRITEBACKS) && has_procmemreserved())
		procmemunreserve();

}
