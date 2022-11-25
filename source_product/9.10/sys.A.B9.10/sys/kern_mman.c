/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_mman.c,v $
 * $Revision: 1.32.83.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/08 16:53:29 $
 */


/*	kern_mman.c	6.1	83/07/29	*/

#include "../h/types.h"
#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../h/vfd.h"

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/trace.h"
#include "../h/mman.h"
#include "../h/conf.h"
#include "../h/vas.h"
#include "../h/debug.h"
#include "../h/kern_sem.h"

/* BEGIN DEFUNCT */
obreak()
{
	struct a {
		char	*nsiz;
	};
	register int n, d;
	register preg_t *prp;
	register reg_t *rp;

	/*
	 * set n to new data size
	 * set d to new-old
	 */

	vmemp_lock();		/* lock the vm empire */
	if ((prp = findpregtype(u.u_procp->p_vas, PT_DATA)) == NULL)
		panic("obreak: no data region");

	n = btorp(((struct a *)u.u_ap)->nsiz - prp->p_vaddr);
	if (n < 0)
		n = 0;
	d = n - prp->p_count;
	if(d == 0)
		vmemp_return();
	if (ptob(prp->p_count+d) > u.u_rlimit[RLIMIT_DATA].rlim_cur) {
		u.u_error = ENOMEM;
		vmemp_return();
	}

	/* If data region is memory locked then:
	 *	- don't release any memory if we're shrinking, and
	 *	- abort if we're growing beyond the maximum system
	 *	  limit of lockable memory.
	 */
	if (PREGMLOCKED(prp)) {
		if (d < 0)
			d = 0;
		if (!chkmaxlockmem(d)) {
			u.u_error = ENOMEM;
			vmemp_return();
		}
	}

	rp = prp->p_reg;
	reglock(rp);
	if (growpreg(prp, d, 0, DBD_DZERO, ADJ_REG) < 0) {
		VASSERT(u.u_error);
		regrele(rp);
		vmemp_return();
	}

	regrele(rp);
	vmemp_unlock();
}

/* END DEFUNCT */

#ifdef __hp9000s800
/*
 * grow the stack to include the SP
 * true return if successful.
 */
grow(sp)
	unsigned sp;
{
	register int si;
	register preg_t *prp;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	if ((prp = findpregtype(u.u_procp->p_vas, PT_STACK)) == NULL)
		panic("grow: no stack region");

	/* Although in for other reasons, needed for PROCESSLOCK also */
	if (sp < (unsigned)(prp->p_vaddr+ptob(prp->p_count)))
		vmemp_returnx(0);
	si = btorp(sp-(u_int)prp->p_vaddr) - prp->p_count + SINCR;

	if (ptob(prp->p_count+si) > u.u_rlimit[RLIMIT_STACK].rlim_cur)
		vmemp_returnx(0);

	/* If stack region is memory locked and we're growing beyond
	 * the maximum system limit of lockable memory then abort.
	 */
	if (PREGMLOCKED(prp) && !chkmaxlockmem(si))
		vmemp_returnx(0);

	reglock(prp->p_reg);

	if (growpreg(prp, si, 1, DBD_DZERO, ADJ_REG) < 0) {
		regrele(prp->p_reg);
		vmemp_returnx(0);
	}

	regrele(prp->p_reg);
	vmemp_unlockx();	/* free up VM empire */
	return(1);
}
#endif /* hp9000s800 */
