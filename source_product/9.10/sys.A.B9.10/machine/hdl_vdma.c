/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_vdma.c,v $
 * $Revision: 1.3.84.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/20 11:08:47 $
 */

/*
 * Hardware dependent portions of VDMA support for the 300.
 *
 * Notification -
 *   int vdma_set(vas, space, vaddr)
 *
 *   int vdma_unset(vas, space, vaddr)
 *
 *   int vdma_cset(vas, space, vaddr)
 *
 */


#include "../h/debug.h"
#include "../h/types.h"
#include "../h/vas.h"

#include "../machine/pte.h"

#ifndef NULL
#define NULL 0
#endif

/*ARGSUSED*/
vdma_set(vas, space, vaddr)
vas_t	*vas;
space_t	space;
caddr_t	vaddr;
{
	register struct pte *pt;

	pt = vastopte(vas, vaddr);
	if (pt == NULL)	{	/* no pte here.... */
		return(0);
	}
	if (pt->pg_notify) {	/* no idempotency. */
		return(0);
	}
	if (!pt->pg_v) {	/* not a valid xlation ? */
		return(0);
	}
	pt->pg_notify = 1;
	return(1);
}

#ifdef NEVER_CALLED
/*ARGSUSED*/
vdma_unset(vas, space, vaddr)
vas_t	*vas;
space_t	space;
caddr_t	vaddr;
{
	register struct pte *pt;

	pt = vastopte(vas, vaddr);
	if (pt == NULL) { 	/* no pte here.... */
		return(0);
	}
	if ((pt->pg_notify) == 0) {	/* was not previously set */
		return(0);
	}
	pt->pg_notify = 0;
	return(1);
}
#endif /* NEVER_CALLED */


/*
 * vdma_cset - HDL routine that a HIL caller can call
 * to determine if a particular HIL has notification requested.
 */

/*ARGSUSED*/
vdma_cset(vas, space, vaddr)
vas_t	*vas;
space_t	space;
caddr_t	vaddr;
{
	register struct pte *pt;

	pt = vastopte(vas, vaddr);
	if (pt == NULL) { 	/* no pte here.... */
		return(0);
	}
	return(pt->pg_notify);
}
