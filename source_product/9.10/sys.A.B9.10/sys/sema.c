/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sema.c,v $
 * $Revision: 1.6.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:10:31 $
 */

#ifndef	MP

#include "../h/debug.h"
#include  "../h/types.h"
#include "../h/sema.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/mp.h"

/*
 * Fake semaphore routines to allow code using semaphores to run.
 */

int spinlockcnt = 0;	/* number of vm_spinlocks held by current process */
			/* must be zero before calling swtch()		*/

#ifdef OSDEBUG

vm_spinlock_c(smp)
register vm_sema_t *smp;
{
	register u_int reg_temp;

	smp->spl = spl7();
	VASSERT(vm_valusema(smp) > 0);
	smp->lock |= SEM_LOCKED;
	smp->pid = GETNOPROC(reg_temp) ? -1 : u.u_procp->p_pid;
	spinlockcnt++;
}

vm_spinunlock_c(smp)
register vm_sema_t *smp;
{
	register u_int reg_temp;

	VASSERT(vm_valusema(smp) <= 0);
	VASSERT(GETNOPROC(reg_temp) || smp->pid == u.u_procp->p_pid);
	smp->lock &= ~SEM_LOCKED;
	spinlockcnt--;
	splx(smp->spl);
}

vm_psema(smp, pri)
register vm_sema_t *smp;
register int pri;
{
	register u_int reg_temp;
	int x;

#ifdef hp9000s800
	extern int ISTACKPTR;	/* zero if running on interrupt stack */
	extern int mainentered;	/* zero if main() not yet reached */
#endif hp9000s800

#ifdef hp9000s800
	/* cannot be on interrupt stack 
	 */
	VASSERT(ISTACKPTR || !mainentered);  
#endif hp9000s800

	x = spl7();
	while (smp->lock & SEM_LOCKED) {
		if (GETNOPROC(reg_temp) == 0) {
			if (smp->pid == u.u_procp->p_pid)
				panic("vm_psema: circular deadlock");
		}
		smp->lock |= SEM_WANT;
		sleep(smp, pri);
	}
	smp->lock |= SEM_LOCKED;
	smp->pid = GETNOPROC(reg_temp) ? -1 : u.u_procp->p_pid;
	splx(x);
}


/*ARGSUSED*/
vm_vsema(smp, null)
register vm_sema_t *smp;
int null;
{
	int x;
	
	x = spl7();
	if (!(smp->lock & SEM_LOCKED))
		panic("vm_vsema: semaphore not locked");

	smp->lock &= ~SEM_LOCKED;
	if (smp->lock & SEM_WANT) {
		smp->lock &= ~SEM_WANT;
		wakeup(smp);
	} 
	splx(x);
}

#endif  OSDEBUG

vm_cpsema(smp)
register vm_sema_t *smp;
{
	register u_int reg_temp;
	int x;
	
	x = spl7();
	if (smp->lock & SEM_LOCKED) {
		splx(x);
		return(0);
	}

	smp->lock |= SEM_LOCKED;
	smp->pid = GETNOPROC(reg_temp) ? -1 : u.u_procp->p_pid;
	splx(x);
	return(1);
}

vm_cvsema(smp)
register vm_sema_t *smp;
{

	printf("warning: vm_cvsema called!\n");
	if (!(smp->lock & SEM_LOCKED)) {
		return(0);
	}
	smp->lock &= ~SEM_LOCKED;
	if (smp->lock & SEM_WANT) {
		smp->lock &= ~SEM_WANT;
		wakeup(smp);
	}
	return(1);
}

vm_disownsema(smp)
register vm_sema_t *smp;
{
	VASSERT(vm_valusema(smp) <= 0);
	VASSERT(((smp->pid == -1) || (smp->pid == u.u_procp->p_pid)));
	smp->pid = -1;
}

/* vm_psyncx() and vm_wsyncx() are analogous to psync() and wsync() but
 * not as fully functional.
 *
 * Processes which block via vm_psyncx() should only be unblocked via
 * vm_wsyncx(), NOT via wakeup().  Note that vm_psyncx() does not recheck
 * after resuming to ensure that the sleep condition has changed;
 * it assumes that, if it resumes, that it (and it alone) has the
 * resource it was waiting for.
 */

vm_psyncx(smp, pri)
register vm_sema_t *smp;
register int pri;
{
	extern int ISTACKPTR;		/* zero if running on interrupt stack */
	extern int mainentered;		/* zero if main() not yet reached */

	VASSERT(ISTACKPTR || !mainentered);  /* cannot be on interrupt stack */

	sleep(smp, pri);
}

/* vm_wsyncx(): see comments preceeding vm_psyncx() above. */

vm_wsyncx(smp, p)
register vm_sema_t *smp;
struct proc *p;
{
	VASSERT(p->p_wchan == (caddr_t)smp);
	/* do the equivalent of a wakeup() on just the specified process */
	unselect(p);
}


vm_valusema(smp)
register vm_sema_t *smp;
{
	if (smp->lock & SEM_LOCKED)
		return(0);
	else
		return(1);
}

vm_initsema_c(smp, val)
register vm_sema_t *smp;
register int val;
{
	if (val <= 0)
		smp->lock = SEM_LOCKED;
	else
		smp->lock = 0;
}

vm_termsema()
{}
/* Initalize a spinlock.  Spinlocks are implemented as semaphores here. */

vm_initlock_c(smp)
register vm_sema_t *smp;
{
	vm_initsema(smp, 1, 0);
}

#endif	/* ! MP */

