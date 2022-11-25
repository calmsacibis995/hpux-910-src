/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/io_map.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:05:59 $
 */

/*
 * io_map.c--routines to provide physical I/O memory mapping to the
 *  kernel and user processes.
 *
 * This module is written in terms of regions and pregions, and should
 *  be reasonably convenient for providing access to physical I/O space.
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmparam.h"
#include "../h/vmmac.h"
#include "../h/region.h"
#include "../h/pregion.h"
#include "../h/vfd.h"
#include "../h/vas.h"
#include "../h/pfdat.h"
#include "../h/user.h"
#include "../h/swap.h"

preg_t *io_map_c();

/*
 * io_map()--establish a mapping between the physical pages starting at
 *  phys_pfn to the space/vaddr provided.  If vaddr is NULL, this routine
 *  will choose a vaddr in the space for you.
 * On return, io_map() will return a pregion view of the physical pages,
 *  or NULL if the mapping failed.  The mapping will be cache inhibited.
 */
preg_t *
io_map(space, vaddr, phys_addr, count, prot)
	space_t space;
	caddr_t vaddr, phys_addr;
	u_int count;
        short prot;
{
	io_map_c(space, vaddr, phys_addr, count, prot, PHDL_CACHE_CI);
}

/*
 * io_map_c()--establish a mapping between the physical pages starting at
 *  phys_pfn to the space/vaddr provided.  If vaddr is NULL, this routine
 *  will choose a vaddr in the space for you.
 * On return, io_map() will return a pregion view of the physical pages,
 *  or NULL if the mapping failed.  The caller supplies the cache mode.
 */
preg_t *
io_map_c(space, vaddr, phys_addr, count, prot, cachemode)
	space_t space;
	caddr_t vaddr, phys_addr;
	u_int count;
        short prot;
	int cachemode;
{
	register preg_t *prp;
	register reg_t *rp;
	register u_int off;
	register int x, pfn;
	VASSERT(count > 0);

	/*
	 * Create a region, attach a pregion
	 */
	if ((rp = allocreg(NULL, NULL, RT_SHARED)) == NULL) {
		return(NULL);
	}
	if ((prp = attachreg((vas_t *)space, rp, prot, PT_IO,
			     vaddr ? PF_EXACT : 0, vaddr, 0, count)) == NULL) {
		freereg(rp);
		return(NULL);
	}
	prp->p_hdl.p_physpfn = btop(phys_addr);
	prp->p_hdl.p_hdlflags |= (cachemode & PHDL_CACHE_MASK);

	/*
	 * Do hdl-level process attach
	 */
	hdl_procattach(&u, space, prp);
	regrele(rp);
	return(prp);
}

/*
 * io_unmap()--break the translation of the physical pages viewed through
 *  the pregion, free any resources used this way.  The pregion must have
 *  been previously locked, probably through a findprp() call.
 */
void
io_unmap(prp)
    register preg_t *prp;
{
    VASSERT(prp->p_off == 0);
    VASSERT(prp->p_type == PT_IO);
    VASSERT(vm_valusema(&prp->p_reg->r_lock) <= 0);

    hdl_procdetach(&u, prp->p_vas, prp);
    detachreg((vas_t *)prp->p_space, prp);
}


preg_t *
io_findmap(vas, phys_addr)
    vas_t *vas;
    caddr_t phys_addr;
{
    register preg_t *prp;

    VASSERT(vas);
    VASSERT(phys_addr);

    vaslock(vas);
    if (vas->va_next == (preg_t *)vas)
        goto notfound;
    
    for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
	if (prp->p_type != PT_IO)
	    continue;
	if (btop(phys_addr) != prp->p_hdl.p_physpfn)
	    continue;
	goto found;
    }

notfound:
    vasunlock(vas);
    return((preg_t *)NULL);

found:
    vasunlock(vas);
    return(prp);
}
