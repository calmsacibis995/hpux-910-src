/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_init.c,v $
 * $Revision: 1.5.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:20 $
 */

/*
 * This file contains the hardware dependent data structure initialization
 *  routines.
 */
#include "../h/debug.h"
#include "../h/vmparam.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/region.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/vfd.h"
#include "../h/pfdat.h"
#include "../h/vmmac.h"
#include "../h/map.h"
#include "../h/vdma.h"
#include "../h/malloc.h"
#include "../machine/param.h"
#include "../machine/pte.h"
#include "../machine/atl.h"
#include "../machine/vmparam.h"

unsigned int hil_poffset;	/* Offset from HIL to physical page #'s */
unsigned int physpfdat;		/* physical address of the pfdat array */
#ifdef OSDEBUG
int check_iptes = 0;
#endif /* OSDEBUG */

/* These create the concept of KERNELSPACE */
reg_t kernel_dummy_reg;
preg_t kernel_text_preg, *kern_text_preg = &kernel_text_preg;
preg_t kernel_data_preg, *kern_data_preg = &kernel_data_preg;
preg_t kernel_stack_preg, *kern_stack_preg = &kernel_stack_preg;
preg_t kernel_internl_io_preg, *kern_internl_io_preg = &kernel_internl_io_preg;
preg_t kernel_debug_preg, *kern_debug_preg = &kernel_debug_preg;
preg_t kernel_tt_preg, *kern_tt_preg = &kernel_tt_preg;
preg_t kernel_valloc_preg, *kern_valloc_preg = &kernel_valloc_preg;
preg_t kernelpreg, *kernpreg = &kernelpreg;
vas_t kernvas;
vas_t *kernelspace = &kernvas;

/*
 *initialize a static dummy region for the kernel to point to
 */

kernel_dummy_reg_init()
{
	kernel_dummy_reg.r_type = RT_SHARED;
	kernel_dummy_reg.r_flags = RF_ALLOC;
	kernel_dummy_reg.r_fstore = (struct vnode *)NULL;
	kernel_dummy_reg.r_bstore = (struct vnode *)NULL;
	vm_initsema(&kernel_dummy_reg.r_lock, 1, REG_R_LOCK_ORDER, "reg sema");
	vm_initsema(&kernel_dummy_reg.r_mlock, 1, REG_R_MLOCK_ORDER,
		    "reg mlock sema");
}

kpreg_init(prp, vaddr, num_pages, pregtype, prot)
	register preg_t *prp;
	register caddr_t vaddr;
	int num_pages;
	short pregtype;
	int prot;
{
	/*
	 * Manually initialize the pregion and vas
	 */

	prp->p_vas = (vas_t *)KERNELSPACE;
	prp->p_space = KERNELSPACE;
	prp->p_reg = &kernel_dummy_reg;
	prp->p_vaddr = vaddr;
	prp->p_type = pregtype;
	prp->p_count = num_pages;
	prp->p_next = prp->p_prev = prp;
	prp->p_flags = PF_ALLOC;
	prp->p_prot = prot;
	prp->p_prpnext = (preg_t *)NULL;
	prp->p_prpprev = (preg_t *)NULL;
	prp->p_hdl.p_hdlflags = 0;
	prp->p_hdl.p_atl = NULL;
	
	/*
	 * If the protection are read-only,
	 * make the hdl fields reflect this.
	 */
	if (!(prot&PROT_WRITE))
		prp->p_hdl.p_hdlflags |= PHDL_RO;
		
	if (chkattach(&kernvas, prp, 0)) 
		panic("kpreg_init: bad chkattach");

	/*
	 * Link this pregion into the list of pregions associated
	 * with its region.
	 */
	add_pregion_to_region(prp);

	insertpreg(&kernvas, prp);
}

hdl_initlocks()
{
	/* Fix lock order sfk! XXX */
}

/*
 * Hardware dependent memory initialization routine.
 *
 * The HDL meminit routine is responsible for:
 *	- initializing any HDL data structures.
 *	- initialize sysmap so that it contains the virtual
 *	  addresses for kernel memory.
 *
 * 300 HDL meminit routine.
 *
 * For 300 we need to:
 *	- Create a "kernel pregion" so that KERNELSPACE has orthogonal
 *		meaning to the general region routines.
 */
/*ARGSUSED*/
hdl_meminit(first, last, vpage)
	int first, last, vpage;
{
	register pfd_t *pfd;
	register int x;
	register int i;
	register vas_t *v;
	struct ste *st;
	caddr_t kernvaddr;
	extern int indirect_ptes;

	v = &kernvas;
	v->va_hdl.va_seg = Syssegtab;
	v->va_hdl.va_vsegs = NULL;
	v->va_hdl.va_attach_hint = NULL;
        VA_KILL_CACHE(v);
        v->va_next = v->va_prev = (preg_t *)v;
        vm_initsema(&v->va_lock, 1, VAS_VA_LOCK_ORDER, "vas sema");

        /* Initialize a kernel dummy region */
        kernel_dummy_reg_init();

	/* setup the kernel's text pregion */
	kernvaddr = (caddr_t)0;
	kpreg_init(kern_text_preg, kernvaddr, btorp(&etext), PT_TEXT,
		   PROT_KERNEL|PROT_READ|PROT_EXECUTE);
	kernvaddr += ptob(btorp(&etext));

	/* setup the kernel's initialized data/bss pregion */
	kpreg_init(kern_data_preg, kernvaddr,btorp(&end)-btorp(&etext),
		   PT_DATA, PROT_KRW);

	/* setup the kernel's stack and U-area region. Note u is not */
	/* mapped yet, however kpreg_init does not dereference the   */
	/* vaddr, so it is ok to initialize the pregion now.         */

	kpreg_init(kern_stack_preg, (caddr_t)&u - ptob(KSTACK_PAGES),
		   KSTACK_PAGES + UPAGES, PT_DATA, PROT_KRW);

	/* setup the kernel's valloc mapping area pregion */
	kpreg_init(kern_valloc_preg, GENMAPSPACE, 
                   btop(highest_kvaddr) - btop(GENMAPSPACE - 1), PT_DATA,
		   PROT_KRW);

	/* setup the kernel's sysmap mapping area pregion */
	kpreg_init(kernpreg, 0x40000000, 0x40000, PT_DATA, PROT_KRW);
#ifdef PFDAT32
	kernpreg->p_hdl.p_hdlflags |= PHDL_DPTES;
#endif	

	/* setup the kernel's internal I/O pregion */
	kpreg_init(kern_internl_io_preg, LOGICAL_IO_BASE, INTIOPAGES, PT_IO,
		   PROT_KRW);

	/* 
	 * setup the kernel's transparent translation pregion
	 * XXX what type should this be?  call it pt_data 
	 * -1 of count is really a bug in chkattach.  Chkattach should
	 * look at btop(count) -1 byte. 
	 */
	kpreg_init(kern_tt_preg, tt_region_addr, 
		   btop(0xffffffff) - btop(tt_region_addr - 1) - 1, PT_IO,
		   PROT_KRW);

	/* Precalculate offset from HIL page numbers to physical ones */
	hil_poffset = physmembase+hil_pbase;

	if (indirect_ptes) {
		/* initialize the ptes that indirect ptes will point at */
		pfd = &pfdat[first];
		for (i = first; i <= last; pfd++,i++) {
			pfd->pf_hdl.pf_ro_pte.pg_pfnum = hiltopfn(i);
			pfd->pf_hdl.pf_ro_pte.pg_prot = 1;
			pfd->pf_hdl.pf_ro_pte.pg_ropte = 1;
			pfd->pf_hdl.pf_rw_pte.pg_pfnum = hiltopfn(i);
#ifdef PFDAT32			
			pfd->pf_hdl.pf_bits &= PFHDL_ZERO_BITS;
#else
			pfd->pf_hdl.pf_bits = 0;			
#endif			
		}
	}
	physpfdat = (unsigned int)vtop(&pfdat[0]);

	atl_init(); /* initialize attachlists */

#ifdef OSDEBUG
	if (check_iptes)
		vl_insert((vas_t *)KERNELSPACE);
#endif /* OSDEBUG */
}

/*
 * Hardware dependent region initialization routine
 */
/*ARGSUSED*/
void
hdl_allocreg(rp)
	reg_t *rp;
{
	/* r_flags field exists, but is unused currently */
	rp->r_hdl.r_flags = 0;
}
/*ARGSUSED*/
void
hdl_freereg(rp)
	reg_t *rp;
{
}


/*
 * Hardware dependent pregion initialization routine
 */
void
hdl_allocpreg(prp)
	preg_t *prp;
{
	prp->p_hdl.p_hdlflags = 0;
	prp->p_hdl.p_physpfn = 0;
	prp->p_hdl.p_ntran = 0;
	prp->p_hdl.p_atl = NULL; /* clear the attach list */
	prp->p_hdl.p_spreg = (hdl_subpreg_t *)0;
}

/*ARGSUSED*/
void
hdl_freepreg(prp)
	preg_t *prp;
{
	VASSERT((prp->p_hdl.p_hdlflags & PHDL_ATTACH) == 0);
	VASSERT((prp->p_hdl.p_hdlflags & PHDL_PROCATTACH) == 0);

	if (prp->p_hdl.p_spreg != (hdl_subpreg_t *)0) {
	    FREE(prp->p_hdl.p_spreg, M_PREG);
	}
}

/*
 * Hardware dependent vas initialization routine
 */
void
hdl_allocvas(vas)
	register vas_t *vas;
{
	register int x;

	/*
	 * Set up initial data structures
	 */
	vas->va_hdl.va_seg = NULL;
	vas->va_hdl.va_vsegs = NULL;
	vas->va_hdl.va_attach_hint = NULL;
#ifdef OSDEBUG
	if (check_iptes)
		vl_insert(vas);
#endif /* OSDEBUG */

}

void
hdl_freevas(vas)
	register vas_t *vas;
{
	VASSERT(vas->va_hdl.va_seg == NULL);
	VASSERT(vas->va_hdl.va_vsegs == NULL);
#ifdef OSDEBUG
	if (check_iptes)
		vl_remove(vas);
#endif /* OSDEBUG */
}


hdl_initkvm(prp)
preg_t *prp;
{
#ifdef PFDAT32
	prp->p_hdl.p_hdlflags |= PHDL_DPTES;
#else	
	if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0)
		atl_initkvm(prp);
#endif
}

#ifdef OSDEBUG

struct vaslist {
    vas_t *vl_vas;
    struct vaslist *vl_next;
    struct vaslist *vl_prev;
};

struct vaslist *vaslist;

vl_insert(vas)
vas_t *vas;
{
    struct vaslist *vl;
    int x = spl6();

    VASSERT(vas);

    vl = (struct vaslist *) kmem_alloc(sizeof(struct vaslist));

    vl->vl_vas = vas;
    vl->vl_next = NULL;
    vl->vl_prev = NULL;

    if (vaslist == NULL)
        vaslist = vl;
    else {
        vaslist->vl_prev = vl;
        vl->vl_next = vaslist;
        vaslist = vl;
    }
    splx(x);
}

vl_remove(vas)
vas_t *vas;
{
    struct vaslist *vl;
    int x = spl6();

    VASSERT(vas);
    VASSERT(vaslist);

    for (vl = vaslist; vl != NULL; vl = vl->vl_next) {
        if (vl->vl_vas == vas) {
            if (vl->vl_prev)
                vl->vl_prev->vl_next = vl->vl_next;
	    else
		vaslist = vl->vl_next;
            if (vl->vl_next)
                vl->vl_next->vl_prev = vl->vl_prev;
            break;
        }
    }
    VASSERT(vl && (vl->vl_vas == vas));
    kmem_free(vl, sizeof(struct vaslist));
    splx(x);
}    

vl_find_iptes(pfn, crash)
int pfn;
{
    struct vaslist *vl;
    register int i, j;
    struct vste *vste;
    unsigned int hdlpfn;
    unsigned int segtab;
    int x = spl6();

    VASSERT(three_level_tables);

    hdlpfn = hiltopfn(pfn);

    for (vl = vaslist; vl != NULL; vl = vl->vl_next) {
        vste = vl->vl_vas->va_hdl.va_vsegs;
        segtab = (unsigned int)vl->vl_vas->va_hdl.va_seg;

        while (vste != NULL) {
	    unsigned int ste;
	    unsigned int bte;
	    unsigned int ipte;

	    VASSERT(segtab);
            ste = segtab + vste->vste_offset;
            VASSERT(*(int *)ste);

	    bte = stetobt(ste);
	    for (i = 0; i < NBTEBT; i++, bte+=4) {
		if (*(int *)bte) {
                    ipte = btetop(bte);
                    for (j = 0; j < NPTEPT; j++, ipte+=4) {
                        if (*(int *)ipte & PG_IV) {
                            struct pte *pte;
                            pte = (struct pte *)((*(int *)ipte) & 0xfffffffc);
                            if (pte->pg_pfnum == hdlpfn) {
				printf("BAD INDIRECT PTE: vas = 0x%x pfn = 0x%x hdlpfn = 0x%x ste = 0x%x bte = 0x%x ipte = 0x%x pte = 0x%x\n", vl->vl_vas, pfn, hdlpfn, ste, bte, ipte, pte);
				if (crash)
				    panic("vl_find_iptes");
                            }
                        }
                        if (*(int *)ipte & PG_V) {
                            struct pte *pte;
                            pte = (struct pte *)ipte;
                            if (pte->pg_pfnum == hdlpfn) {
				printf("BAD DIRECT PTE: vas = 0x%x pfn = 0x%x hdlpfn = 0x%x ste = 0x%x bte = 0x%x ipte = 0x%x pte = 0x%x\n", vl->vl_vas, pfn, hdlpfn, ste, bte, ipte, pte);
				if (crash)
				    panic("vl_find_iptes");
                            }
                        }
                    }
                }
	    }
            vste = vste->vste_next;
	}
    }
    splx(x);
}
                
vl_fix_iptes(pfn)
int pfn;
{
    struct vaslist *vl;
    register int i, j;
    struct vste *vste;
    unsigned int hdlpfn;
    unsigned int segtab;
    int x = spl6();

    VASSERT(three_level_tables);

    hdlpfn = hiltopfn(pfn);

    for (vl = vaslist; vl != NULL; vl = vl->vl_next) {
        vste = vl->vl_vas->va_hdl.va_vsegs;
        segtab = (unsigned int)vl->vl_vas->va_hdl.va_seg;

        while (vste != NULL) {
	    unsigned int ste;
	    unsigned int bte;
	    unsigned int ipte;

	    VASSERT(segtab);
            ste = segtab + vste->vste_offset;
            VASSERT(*(int *)ste);

	    bte = stetobt(ste);
	    for (i = 0; i < NBTEBT; i++, bte+=4) {
		if (*(int *)bte) {
                    ipte = btetop(bte);
                    for (j = 0; j < NPTEPT; j++, ipte+=4) {
                        if (*(int *)ipte & PG_IV) {
                            struct pte *pte;
                            pte = (struct pte *)((*(int *)ipte) & 0xfffffffc);
                            if (pte->pg_pfnum == hdlpfn) {
				*(int *)ipte = 0;
				purge_tlb();
                            }
                        }
                        if (*(int *)ipte & PG_V) {
                            struct pte *pte;
                            pte = (struct pte *)ipte;
                            if (pte->pg_pfnum == hdlpfn) {
				*(int *)ipte = 0;
				purge_tlb();
                            }
                        }
                    }
                }
	    }
            vste = vste->vste_next;
	}
    }
    splx(x);
}
#endif /* OSDEBUG */
