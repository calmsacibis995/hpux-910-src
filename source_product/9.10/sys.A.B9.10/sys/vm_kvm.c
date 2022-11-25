/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_kvm.c,v $
 * $Revision: 1.7.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:17:20 $
 */

/*
 * vm_kvm.c--routines to support kernel virtual memory
 */

#ifdef __hp9000s300  /* s300 only arch to support kvm at this time */
#include "../h/debug.h"
#include "../h/pregion.h"
#include "../h/region.h"
#include "../h/map.h"
#include "../h/pfdat.h"
#include "../h/vas.h"
#include "../machine/param.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../h/vmmac.h"
#include "../h/swap.h"

#include "../h/user.h"			/* regrele() refers to u_procp */

#define KVM_MAP_RATIO 4

extern long rmalloc();
extern vas_t kernvas;

preg_t *kvm_preg;	/* pregion in kernel vas w. memory under it */
struct map *kvm_rmap;	/* Resource map of vaddrs available in pregion*/
int kvm_wanted;		/* Flag for out of virtual addresses */

static caddr_t kvm_base;	/* Configuration values for kvm */
static int kvm_length;
/*
 * Get the configuration values for kernel virtual memory, store them
 *  away.  Should be called from machine-dependent code to be told
 *  what size of virtual array to use.
 */
void
config_kvm(base, length)
    caddr_t base;
    int length;
{
    kvm_base = base;
    kvm_length = length;
}

/*
 * init_kvm()--called from late in init_main, once things like swapdev_vp
 *  are set up.  At this point it will actually set up the region.
 */
void
init_kvm()
{
    register preg_t *prp;
    register reg_t *rp;
    int mapsize;

    /*
     * Create a private data region, add it to the kernel vas.
     */
    if ((rp = allocreg(NULL, swapdev_vp, RT_PRIVATE)) == NULL)
	panic("kvm allocreg");

    /* Grow it non-filled to the requested length */
    if (growreg(rp, kvm_length, DBD_DFILL) < 0)
	panic("kvm growreg");

    /* Now attach a view of this length */
    if ((prp = attachreg(KERNVAS, rp, PROT_URW, PT_DATA,
		    PF_EXACT, kvm_base, 0, kvm_length)) == NULL)
	panic("kvm attachreg");
    kvm_preg = prp;

    hdl_initkvm(prp);
    
    regrele(rp);
 
    /* Set up a resource map to manage this virtual space */
    mapsize = (kvm_length/KVM_MAP_RATIO);
    kvm_rmap = (struct map *)kmem_alloc(sizeof(struct map) * mapsize);
    if (!kvm_rmap)
	panic("kvm kmem_alloc");
    rminit(kvm_rmap, kvm_length, btop(kvm_base), "kvm map", mapsize);
}

/*
 * Allocate some virtual pages, sleep until they're available
 */
caddr_t
kvalloc(pages)
    int pages;
{
    long pg;
    vm_sema_state;		/* semaphore save state */

    vmemp_lockx();		/* lock down VM empire */
    VASSERT(kvm_preg != NULL);

    /* Get some memory */
    while ((pg = rmalloc(kvm_rmap, pages)) == 0) {
	++kvm_wanted;
	sleep(&kvm_wanted, PMEM);
    }

    vmemp_unlockx();	/* free up VM empire */
    /* Return it as a virtual address */
    return ((caddr_t)ptob(pg));
}

/*
 * Free some virtual memory
 */
void
kvfree(vaddr, pages)
    caddr_t vaddr;
    int pages;
{
    /* Release the virtual memory */
    rmfree(kvm_rmap, pages, btop(vaddr));

    /* If people were waiting, tell them */
    if (kvm_wanted) {
	kvm_wanted = 0;
	wakeup(&kvm_wanted);
    }
}

#else /* __hp9000s300, s700, s800 do not support kvm */
#include "../h/types.h"

/*
 * For architectures/configurations which choose not to support kernel
 *  virtual memory, we provide a compatibility interface to kdalloc/kdfree
 */
void
init_kvm()
{
}
caddr_t
kvalloc(pages)
    int pages;
{
    extern caddr_t kdalloc();

    return (kdalloc((unsigned long)pages));
}
void
kvfree(vaddr, pages)
    caddr_t vaddr;
    int pages;
{
    kdfree(vaddr, (unsigned long)pages);
}
#endif /* __hp9000s300 */
