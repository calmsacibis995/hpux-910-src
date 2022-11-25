/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/sys_proc.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:21:47 $
 */

/* HPUX_ID: @(#)sys_proc.c	55.1		88/12/23 */

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


#include "../machine/reg.h"
#include "../machine/psl.h"

#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/acct.h"
#include "../h/ptrace.h"
#include "../s200/trap.h"

#include "../h/vfs.h"
#include "../netinet/in.h"
#include "../nfs/nfs_clnt.h"
#include "../h/debug.h"
#include "../h/pregion.h"

extern preg_t *get_text();

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		pid_t	pid;
		int	*addr;
		int	data;
	} *uap;
	register int req;
        preg_t *prp;
        struct vnode *vp;

	uap = (struct a *)u.u_ap;
	req = uap->req;

	if (req < PT_SETTRC || (req > PT_SINGLE && req < PT_ATTACH) ||
	    (req > PT_DETACH && req < PT_RFPREGS) || req > PT_WFPREGS) {
		u.u_error = EIO;
		return;
	}

	if (req == PT_SETTRC) {
		/*
		 * Because NFS code sometimes sleeps at interruptible
		 * priorities, it is not acceptable to access NFS files
		 * from procxmt().  Thus we must be sure to pre-load
		 * any demand-loadable pages from a traced process.
		 */
		if ((prp = findpregtype(u.u_procp->p_vas, PT_TEXT)) != 0) {
		    vp = prp->p_reg->r_fstore;
		    if (vp->v_fstype == VNFS) {
			    if (pre_demand_load(u.u_procp->p_vas)) {
				    u.u_error = EIO;
				    return;
			    }
		    }
		}
		u.u_procp->p_flag |= STRC;
		return;
	}
	p = pfind(uap->pid);

	if (req == PT_ATTACH) {
		if (p == 0) {
			u.u_error = ESRCH;	/* can't find the process */
			return;
		}
		if ((prp = findpregtype(p->p_vas, PT_TEXT)) != 0) {

		    /* tracing processes across interruptable NFS */
		    /* mount-points is not allowed */

		    vp = prp->p_reg->r_fstore;
		    if (vtomi(vp)->mi_int) {
			    u.u_error = EACCES;
			    return;
		    }
		}

		if ((p == u.u_procp) ||			/* ourself */
		    (p->p_flag & STRC) ||		/* already traced */
		    ((u.u_uid != p->p_uid || u.u_uid != p->p_suid)
		     && !suser()))            /* real and saved uid check */
			u.u_error = EPERM;
		else {
			p->p_dptr = u.u_procp;	/* point process to debugger */
			p->p_flag |= STRC;	/* now being traced */
			u.u_procp->p_flag2 |= SADOPTIVE;/* performance hack */
			psignal(p, SIGTRAP);	/* stop process/wake debugger */
			u.u_r.r_val1 = 0;
		}
		return;
	}


	if (p == 0 || p->p_stat != SSTOP ||
	    (p->p_ppid != u.u_procp->p_pid && u.u_procp != p->p_dptr) ||
	    !(p->p_flag & STRC)) {
		u.u_error = ESRCH;
		return;
	}

	if (req == PT_RFPREGS) {
		dragon_read_ureg(p, uap->addr);
		return;
	}

	if (req == PT_WFPREGS) {
		dragon_write_ureg(p, uap->addr);
		return;
	}

	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, IPCPRI);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = req;
	p->p_flag &= ~SWTED;
	while (ipc.ip_req > 0) {
		if (p->p_stat==SSTOP)
			setrun(p);
		sleep((caddr_t)&ipc, IPCPRI);
	}
	u.u_r.r_val1 = ipc.ip_data;
	if (ipc.ip_req < 0)
		u.u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}


#define	PHYSOFF(p, o) ((caddr_t)(p)+(o))

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register int i;
	register *p;
	register struct text *xp;
	preg_t *prp;
	vas_t *vas;
	short tmp_prot;
	register struct exception_stack *regs;
	regs = (struct exception_stack *)u.u_ar0;
	if (ipc.ip_lock != u.u_procp->p_pid)
		return (0);

	u.u_procp->p_slptime = 0;
	if (u.u_pcb.pcb_flags & MULTIPLE_MAP_MASK) {
		if ((((int) ipc.ip_addr) & 0xf0000000) != 0xf0000000)
			ipc.ip_addr = (int *) (((int)ipc.ip_addr) & 0x0fffffff);
	}
	i = ipc.ip_req;
	ipc.ip_req = 0;
	switch (i) {

	/* read user I */
	case PT_RIUSER:
		/* fall through, I&D spaces are the same */

	/* read user D */
	case PT_RDUSER:
		/* Use copyin rather than fuword to check return value.  */
		/* Even usracc won't catch odd-address problem on 68010. */
		if (copyin((caddr_t)ipc.ip_addr, &ipc.ip_data, 4))
			goto error;
		break;

	/* read u */
	case PT_RUAREA:

		/* Note: Since the kernel stack is now below the
		 *       U-area, ipc.ip_addr can now be negative.
		 *       We should not support reading the kernel
		 *       stack, instead we should support the
		 *       Series 800 PT_RUREGS request.
		 */

		i = (int)ipc.ip_addr;
		/* must be a legal offset for a longword transfer */
		/* must be in the U-area or kernel stack */
		if (   i < -((int)ptob(KSTACK_PAGES))
		    || i > (int)(ptob(UPAGES)-sizeof(int))
		    || (i & 1) )
			goto error;
		ipc.ip_data = *(int *)PHYSOFF(&u, i);
		break;

	/* write user I */
	/* Must set up to allow writing */
	case PT_WIUSER:
		vas = u.u_procp->p_vas;
		if ((prp = get_text(vas)) == NULL)
			goto error;
		/*
		 * All the tests related to shared text have been moved
		 * from here to ptrace() (in the parent).  With NFS, all
		 * such tests have the possibility of sleeping at an
		 * interruptible priority.  If such a sleep is interrupted
		 * by a signal, it causes recursion into this code, and
		 * thus various problems.
		 */
		i = -1;
		if ((i = suiword((caddr_t)ipc.ip_addr, ipc.ip_data)) < 0) {
			/*
			 * Only bypass if the address is within the
			 *  text pregion.
			 */
			if (!prpcontains(prp, prp->p_space,
					(caddr_t)ipc.ip_addr))
				goto error;

			/*
			 * Copy it out, ignoring protections on the pregion.
			 *  Tell vfault about the write, first.  This
			 *  will break copy-on-write associations.
			 */
			vfault(vas, 1, prp->p_space, ipc.ip_addr);
			if (vascopy(KERNVAS, KERNELSPACE, &ipc.ip_data,
				vas, prp->p_space, ipc.ip_addr,
				sizeof(ipc.ip_data), 1))
					goto error;
			i = 0;
		}
		break;

	/* write user D */
	case PT_WDUSER:
		if (suword((caddr_t)ipc.ip_addr, 0) < 0)
			goto error;
		(void) suword((caddr_t)ipc.ip_addr, ipc.ip_data);
		purge_dcache();
		break;

	/* write u */
	case PT_WUAREA:

		/* Note: Since the kernel stack is now below the
		 *       U-area, ipc.ip_addr can now be negative.
		 *       We should not support writing the kernel
		 *       stack, instead we should support the
		 *       Series 800 PT_WUREGS request.
		 */

		i = (int)ipc.ip_addr;
		p = (int *)PHYSOFF(&u, i);
		/* there are only 16 regs in this processor */
		for (i = 0; i < 16; i++)
			if (p == &regs->e_regs[i])
				goto ok;
		if (p == &regs->e_PC) 
			goto ok;
		if (p == (int *)&regs->e_PS) {
			ipc.ip_data |= PSL_USERSET;
			ipc.ip_data &=  ~PSL_USERCLR;
			if (ipc.ip_data & PSL_T)
				u.u_pcb.pcb_flags |= USER_TRACE_MASK;
			else
				u.u_pcb.pcb_flags &= ~USER_TRACE_MASK;
			/* and stuff into the PS reg */
			regs->e_PS = (u_short)ipc.ip_data;
			break;
		}
		if ((int)p & 1)
			goto error;
		if ((p >= &u.u_pcb.pcb_float[FERRBIT]) &&
		    (p <= &u.u_pcb.pcb_float[FR7]))
			goto ok;
		if ((p >= &u.u_pcb.pcb_mc68881[FMC68881_C]) && 
		    (p <= &u.u_pcb.pcb_mc68881[FMC68881_R7]))
			goto ok;

		goto error;

	ok:
		*p = ipc.ip_data;
		break;

	/* detach from debugger */
	case PT_DETACH:
		if ((int)ipc.ip_addr != 1)
			regs->e_PC = (int)ipc.ip_addr;
#ifdef  BSDJOBCTL
		if ((unsigned)ipc.ip_data >= NUSERSIG)
#else   ! BSDJOBCTL
		if ((unsigned)ipc.ip_data >= NUSERSIG || IS_JOBCTL_SIG(ipc.ip_data))
#endif  ! BSDJOBCTL
			goto error;
		u.u_procp->p_cursig = ipc.ip_data;	/* see issig */
		u.u_procp->p_dptr = 0;
		u.u_procp->p_flag &= ~STRC;
		wakeup((caddr_t) &ipc);
		return DODETACH;/* issig() handles delivering the signal */

	/* set signal and continue */
	/* one version causes a trace-trap */
	case PT_CONTIN:
	case PT_SINGLE:
		if ((int)ipc.ip_addr != 1) {

			/* Make sure address is not odd */

			if ((int)ipc.ip_addr & 0x1)
			    goto error;

			regs->e_PC = (int)ipc.ip_addr;
		}
#ifdef  BSDJOBCTL
		if ((unsigned)ipc.ip_data >= NUSERSIG)
#else   ! BSDJOBCTL
		if ((unsigned)ipc.ip_data >= NUSERSIG || IS_JOBCTL_SIG(ipc.ip_data))
#endif  ! BSDJOBCTL
			goto error;
		u.u_procp->p_cursig = ipc.ip_data;	/* see issig */
		if (i == PT_SINGLE) {
			regs->e_PS |= PSL_T;
			u.u_pcb.pcb_flags |= USER_TRACE_MASK;
		}
		wakeup((caddr_t)&ipc);
		return (1);

	/* force exit */
	case PT_EXIT:
		ipc.ip_req = PT_EXIT;	/* put it back */
		return DOEXIT;          /* do not wakeup parent,
					   do not exit,
					   but let caller know! */


	default:
	error:
		ipc.ip_req = -1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
