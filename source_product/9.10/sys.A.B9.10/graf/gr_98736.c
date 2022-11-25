/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/gr_98736.c,v $
 * $Revision: 1.9.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:57:30 $
 */

#ifdef GENESIS
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../s200/pte.h"
#include "../h/proc.h"
#include "../graf.300/graphics.h"
#include "../graf.300/gr_98736.h"
#include "../wsio/intrpt.h"
#include "../h/debug.h"
#include "../h/swap.h"
#include "../h/vdma.h"
#include "../h/vmmac.h"
#include "../machine/vmparam.h"

static int vdma_fault_pending = 0;
static struct proc *vdmad_proc_ptr;

/* Forward Declarations */

unsigned long gen_get_physaddr();
void free_work_buffers(), write_genesis_registers(), gen_exit();
void genesis_fault(), genesis_access_fault();
void gen_stop_dma();
void gen_cache_write_through();
int gen_idle(), genesis_page_fault();

int
gen_idle(p)
    struct graphics_descriptor *p;
{
    register char *base_address;
    register unsigned long *reg_ptr;

    base_address = p->primary;


    /* Make sure dma processor is completely idle, i.e. it    */
    /* is not busy, suspended or waiting for fault resolution */

    reg_ptr = (unsigned long *)(base_address + GEN_IOSTAT);
    if ((*reg_ptr & (IOST_CMDRDY|IOST_DMABUSY|IOST_DMASUSP|
		     IOST_DMA_PG_FLT|IOST_DMA_AC_FLT|
		     IOST_DMA_INT) ) != IOST_CMDRDY) {

	return(0);
    }

    return(1);
}

dma_in_progress(p)
    struct graphics_descriptor *p;
{
    register char *base_address;
    register unsigned long *reg_ptr;

    base_address = p->primary;

    /* If blackfoot, first check with dma processor */

    if (is_blackfoot(p) && !gen_idle(p))
	return(1);

    /* Check Bus Control Register to make sure that dma state machine is idle */

    reg_ptr  = (unsigned long *)(base_address + GEN_BUSCTL);
    if ((*reg_ptr & BC_START) != 0)
	return(1);

    return(0);
}

void
write_genesis_registers(p, slot)
    struct graphics_descriptor *p;
    int slot;
{
    register struct gdev_info *gdev_ptr;
    register char *base_address;
    register unsigned long *reg_ptr;
    register unsigned long physaddr;

    gdev_ptr     = p->g_info_array[slot]->gdev;
    base_address = p->primary;

    /* Write Work Buffer 0 and 1 physical addresses. It   */
    /* is not an error if these have not been allocated   */
    /* by GCDMA_BUFFER_ALLOC. Instead we will just point  */
    /* the registers at the card (A safe known place that */
    /* won't be a security issue).                        */

    reg_ptr  = (unsigned long *)(base_address + GEN_WBUF0);
    if (gdev_ptr->gen_work_buf_0_ptr == 0)
	*reg_ptr = (unsigned long)base_address;
    else
	*reg_ptr = gdev_ptr->gen_work_buf_0_ptr;

    reg_ptr  = (unsigned long *)(base_address + GEN_WBUF1);
    if (gdev_ptr->gen_work_buf_1_ptr == 0)
	*reg_ptr = (unsigned long)base_address;
    else
	*reg_ptr = gdev_ptr->gen_work_buf_1_ptr;

    if (is_blackfoot(p)) {

	/* Check to see if Shadow Segment Table has been allocated */

	if (gdev_ptr->gen_root_ptr == 0) {

	    /* Allocate Shadow Segment Table */

	    gdev_ptr->gen_segment_table = (struct ssegtable *)kdalloc(1);
	    physaddr = gen_get_physaddr((struct proc *)0,&kernvas,(space_t)&kernvas,(caddr_t)gdev_ptr->gen_segment_table,(int *)0);
	    if (physaddr == 0)
		panic("Could not get physical address of shadow segment table");

	    if (processor == M68040)
		gen_cache_write_through(&kernvas,(caddr_t)gdev_ptr->gen_segment_table);

	    gdev_ptr->gen_root_ptr = physaddr;
	}

	/* Write Update Root Pointer Command (this routine is */
	/* only called after verifying that the dma processor */
	/* is idle, so we don't need to check for command     */
	/* ready.                                             */

	reg_ptr  = (unsigned long *)(base_address + GEN_DATA1);
	*reg_ptr = gdev_ptr->gen_root_ptr;
	reg_ptr  = (unsigned long *)(base_address + GEN_CMD);
	*reg_ptr = GEN_UPDATE_ROOT_POINTER;
    }
    return;
}

void
gen_exit(p,slot)
    struct graphics_descriptor *p;
    register int slot;
{
    register int i;
    register int j;
    register struct gdev_info *gdev_ptr;
    register struct ssegtable *seg_table;
    struct graphics_info *gi_ptr;

    gi_ptr   = p->g_info_array[slot];
    gdev_ptr = gi_ptr->gdev;


    if (p->g_last_dma_lock_slot == (slot + 1) && dma_in_progress(p)) {
	gen_stop_dma(p);

	if (gi_ptr->vdmad_fault_flag) {

	    /* vdmad is currently handling a fault on  */
	    /* behalf of this process. Wait for it to  */
	    /* complete before we finish exiting and   */
	    /* freeing up the process's vm. We also    */
	    /* clear the flag so that when the fault   */
	    /* handler returns it can short circuit    */
	    /* and not attempt to send a RESTART_FAULT */
	    /* to the genesis interface.               */

	    gi_ptr->vdmad_fault_flag = 0;
	    sleep(&(gi_ptr->vdmad_fault_flag),PSWP);
	}
    }

    free_work_buffers(gi_ptr);

    /* free up shadow page tables */

    if ((seg_table = gdev_ptr->gen_segment_table) != (struct ssegtable *)0) {
	for (i = 0; i < NSSTE_PER_PAGE; i++) {
	    if (seg_table->sste[i] != 0) {

		/* Free up page table */

		kdfree(gdev_ptr->gen_spt_block[(i/NSPT_PER_SVASSEG)]->shpagetables[(i%NSPT_PER_SVASSEG)],1);

	    }

	    if ((i % NSPT_PER_SVASSEG) == 0) {

		j = i / NSPT_PER_SVASSEG;
		if (gdev_ptr->gen_spt_block[j] != (struct spt_block *)0) {

		    /* Free up spt block */

		    kmem_free(gdev_ptr->gen_spt_block[j], 
			      sizeof(struct spt_block));
		}
	    }
	}

	/* Free up segment table */

	kdfree(seg_table,1);
    }
    return;
}

void
gen_stop_dma(p)
    struct graphics_descriptor *p;
{
    register char *base_address;
    register unsigned long *reg_ptr;

    base_address = p->primary;

    if (is_blackfoot(p)) {

	/* Reset the DMA processor */

	reg_ptr  = (unsigned long *)(base_address + GEN_IOCR);
	*reg_ptr = IOCR_RESET_DMA;

	/* Wait for DMA processor to be reset */

	snooze(50000);

	/* Wait for Command Ready */

	reg_ptr = (unsigned long *)(base_address + GEN_IOSTAT);
	while((*reg_ptr & IOST_CMDRDY) == 0)
	    ;

	/* Clear any pending fault for this card */

	p->save_io_status = 0;
    }

    /* Stop state machine dma and wait for ack */

    reg_ptr  = (unsigned long *)(base_address + GEN_BUSCTL);
    *reg_ptr = 0;

    while ((*reg_ptr & BC_START) != 0)
	;

    return;
}

alloc_work_buffers(gi_ptr)
    struct graphics_info *gi_ptr;
{
    struct gdev_info *gdev_ptr;
    unsigned long physaddr;
    reg_t *rp;
    preg_t *prp;

    gdev_ptr = gi_ptr->gdev;

    /* It is assumed that the caller of this procedure has already checked */
    /* to make sure that work buffers have not already been allocated.     */

    /* allocate a shared region for kernel use */

    if ((rp = allocreg(NULL, swapdev_vp, RT_SHARED)) == NULL ) {
	    return(ENOMEM);
    }

    if (growreg(rp, 1, DBD_DZERO) < 0) {
	    regrele(rp);
	    return(ENOMEM);
    }

    if ((prp = attachreg(gi_ptr->gi_vas, rp,
			   PROT_URWX, PT_GRAFLOCKPG, 0, 0, 0, 1)) == NULL) {
	    regrele(rp);
	    return(ENOMEM);
    }

    hdl_procattach(&u, gi_ptr->gi_vas, prp);

    /* The buffer should be page aligned, but just in case we check */
    /* (This will need to change for Series 800).                   */

    if (((unsigned long)prp->p_vaddr & 0xfff) != 0)
	panic("alloc_work_buffers: dma buffer page not aligned.\n");

    /* Lock the pregion */

    if (!mlockpreg( prp))
	    panic("alloc_work_buffers: mlockpreg failed");

    regrele(rp);

    /* Get physical address */

    physaddr = gen_get_physaddr(gi_ptr->gi_proc,gi_ptr->gi_vas,gi_ptr->gi_space,prp->p_vaddr,(int *)0);

    if (physaddr == 0)
	panic("alloc_work_buffers: Could not get physical address of locked buffer.\n");

    if (processor == M68040)
	gen_cache_write_through(gi_ptr->gi_vas,prp->p_vaddr);

    /* Store new pointers */

    gdev_ptr->gen_work_buf_0_ptr   = physaddr;
    gdev_ptr->gen_work_buf_1_ptr   = physaddr + 2048;
    gdev_ptr->gen_work_buf_0_uvptr = prp->p_vaddr;
    gdev_ptr->gen_work_buf_1_uvptr = prp->p_vaddr + 2048;

    return(0);
}

void
free_work_buffers(gi_ptr)
    register struct graphics_info *gi_ptr;
{
    register struct gdev_info *gdev_ptr;
    preg_t *prp;

    gdev_ptr = gi_ptr->gdev;

    if (gdev_ptr->gen_work_buf_0_ptr == 0)
	return;

    if (prp = findprp(gi_ptr->gi_vas,
		      gi_ptr->gi_space,
		      gdev_ptr->gen_work_buf_0_uvptr)) {

	if (prp->p_reg) {
	    hdl_procdetach(&u, gi_ptr->gi_vas, prp);
	    detachreg(gi_ptr->gi_vas, prp);
	}
    }

    gdev_ptr->gen_work_buf_0_ptr = 0;
    gdev_ptr->gen_work_buf_1_ptr = 0;
    return;
}

vdma_page_info(vn_ptr)
    struct vnotify *vn_ptr;
{
    register int slot;
    register int numb_slots;
    register space_t space;
    register vas_t *vas;
    register unsigned long vaddr;
    register int type;
    register struct graphics_descriptor *p;
    register struct graphics_info **gi_ptr;
    register struct spagetable *spt_ptr;
    register struct spt_block *spt_block_ptr;
    int retval;
    int page_count;
    unsigned long *reg_ptr;
    char *base_address;
    unsigned long tmp1;
    unsigned long tmp2;
    int i;
    int save_spl;
    extern struct graphics_descriptor *graphics_devices;

    /* Find the graphics info pointer(s) for this mapping and */
    /* then find the associated shadow page table entry(s).   */

    space = vn_ptr->v_space;
    vas   = vn_ptr->v_vas;
    type  = vn_ptr->v_type;

    p = graphics_devices;
    retval = 0;
    while (p != (struct graphics_descriptor *)0) {

	if (is_genesis(p)) {

	    /* Set vaddr and page_count each time since they */
	    /* will be modified below if page_count > 1      */

	    vaddr = (unsigned long)vn_ptr->v_vaddr;
	    page_count = vn_ptr->v_cnt;

	    gi_ptr = p->g_info_array;
	    numb_slots = p->g_numb_slots;
	    for (slot = 0; slot < numb_slots; slot++, gi_ptr++) {
		if (   *gi_ptr != (struct graphics_info *)0
		    && (*gi_ptr)->gi_space == space
		    && (*gi_ptr)->gi_vas   == vas) {

		    /* Loop for each page */

		    while (page_count > 0) {

			/* Get pointer to page table if it exists */

			spt_block_ptr = (*gi_ptr)->gdev->gen_spt_block[SPT_BLPTR_INDEX(vaddr)];
			if (spt_block_ptr != (struct spt_block *)0) {
			    spt_ptr = spt_block_ptr->shpagetables[SPT_BLOCK_INDEX(vaddr)];
			    if (   spt_ptr != (struct spagetable *)0
				&& spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] != 0) {

				switch (type) {

				case VDMA_GETBITS:

				    /* PURGE cache so that we are sure to get the */
				    /* modify bits written by the dma processor   */

				    purge_dcache_physical();
				    purge_dcache_s();

				    tmp1 = spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)];

				    /* Convert Shadow Page table modify and reference */
				    /* bits to Series 300 page table modify and ref-  */
				    /* ence bits.                                     */

				    if (tmp1 & GEN_SPTE_REF)
					retval |= PG_REF;

				    if (tmp1 & GEN_SPTE_DIRTY)
					retval |= PG_M;

				    break;

				case VDMA_UNSETBITS:

				    /* Clear bits indicated in v_flags field of notify */
				    /* structure.                                      */

				    tmp1 = vn_ptr->v_flags;
				    tmp2 = 0;

				    /* Currently the VM system never cleans pages, it just */
				    /* pages them out. We will have a unclosable critical  */
				    /* section here if this were to change in the future   */
				    /* without some kind of handshaking with the dma       */
				    /* processor. The problem is that the dma processor    */
				    /* can dirty the page again after the VM system has    */
				    /* seen the modify bit set but before the VM system    */
				    /* has gotten around to clearing it.                   */

				    if (tmp1 & PG_M)
					panic("Attempt to clear modify bit in shadow pte in vdma_page_info()\n");


				    if (tmp1 & PG_REF)
					tmp2 |= GEN_SPTE_REF;

				    /* no cache flushing is necessary since shadow page */
				    /* tables are write-through.                        */

				    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] &= ~tmp2;
				    break;

				case VDMA_USERUNPROTECT:

				    /* Set Valid bit again */

				    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] |=  GEN_SPTE_VALID;
				    break;


				case VDMA_DELETETRANS:
				case VDMA_USERPROTECT:
				case VDMA_PROCDETACH:
				case VDMA_UNVIRTUALIZE:
				case VDMA_READONLY:

				    /* Does this change affect the current dma user? */

				    if (p->g_last_dma_lock_slot != (slot + 1)) {

					/* No, just zero or clear valid or write enable bit */

					switch (type) {

					case VDMA_USERPROTECT:
					    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] &= ~GEN_SPTE_VALID;
					    break;

					case VDMA_READONLY:
					    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] &= ~GEN_SPTE_WRITE;
					    break;

					default:
					    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] = 0;
					    break;
					}
				    }
				    else {

					/* Block interrupts so we won't handle      */
					/* page faults while attempting to page out */

					save_spl = spl6();

					/* Yes, suspend the dma processor if it is currently */
					/* doing dma.                                        */

					base_address = p->primary;

					if ((*gi_ptr)->vdmad_fault_flag == 0
					    && vdma_fault_pending == 0) {

					    reg_ptr = (unsigned long *)(base_address + GEN_IOSTAT);
					    if ((*reg_ptr & (IOST_DMABUSY|IOST_DMASUSP|
							     IOST_DMA_PG_FLT|IOST_DMA_AC_FLT) )
							  == IOST_DMABUSY) {

						/* Wait for Command Ready */

						while((*reg_ptr & IOST_CMDRDY) != IOST_CMDRDY)
						    ;

						reg_ptr  = (unsigned long *)(base_address + GEN_CMD);
						*reg_ptr = GEN_SUSPEND_DMA;

						/* Wait for Done, Faulted or Suspended */

						reg_ptr = (unsigned long *)(base_address + GEN_IOSTAT);
						while ((*reg_ptr & (IOST_DMABUSY|IOST_DMASUSP|
								 IOST_DMA_PG_FLT|IOST_DMA_AC_FLT) )
							      == IOST_DMABUSY)
						    ;

					    }
					}

					/* zero or clear valid or write enable bit */

					switch (type) {

					case VDMA_USERPROTECT:
					    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] &= ~GEN_SPTE_VALID;
					    break;

					case VDMA_READONLY:
					    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] &= ~GEN_SPTE_WRITE;
					    break;

					default:
					    spt_ptr->spte[SPAGE_TBL_INDEX(vaddr)] = 0;
					    break;
					}

					/* Check to see if dma processor has this mapping in its TLB */

					reg_ptr  = (unsigned long *)(base_address + GEN_TLB_V1);
					for (i = 1; i <= 4; i++) {

					    if (*reg_ptr == vaddr) {

						/* Purge this tlb entry */

						/* Wait for Command Ready */

						reg_ptr = (unsigned long *)(base_address + GEN_IOSTAT);
						while((*reg_ptr & IOST_CMDRDY) != IOST_CMDRDY)
						    ;

						reg_ptr  = (unsigned long *)(base_address + GEN_CMD);
						*reg_ptr = (GEN_PURGE_TLB_ENTRY | i);

						/* point to next virtual tlb entry */

						reg_ptr  = (unsigned long *)(base_address + GEN_TLB_V1);
						reg_ptr  += (2 * i);

					    }
					    else
						reg_ptr += 2; /* skip to next virtual tlb entry */
					}

					/* DMA state machine should have finished a long time ago, */
					/* but just in case, we wait for it to complete.           */

					reg_ptr  = (unsigned long *)(base_address + GEN_BUSCTL);
					while ((*reg_ptr & BC_START) != 0)
					    ;

					/* Resume dma processor if it is suspended */

					reg_ptr = (unsigned long *)(base_address + GEN_IOSTAT);
					if ((*reg_ptr & IOST_DMASUSP) == IOST_DMASUSP) {

					    /* Wait for Command Ready */

					    while((*reg_ptr & IOST_CMDRDY) != IOST_CMDRDY)
						;

					    reg_ptr  = (unsigned long *)(base_address + GEN_CMD);
					    *reg_ptr = GEN_RESUME_DMA;
					}

					splx(save_spl);
				    }

				    break;

				default:
				    panic("vdma_page_info: unknown type.\n");
				}
			    }
			}

			page_count--;
			vaddr += ptob(1);
		    }

		    /* skip to next genesis device */

		    break;
		}
	    }
	}
	p = p->next;
    }

    return(retval);
}

vdmad()
{
    extern int vdma_present;
    register int s;
    register struct proc *p;

    /* Initialize command name for pstat */

    p = u.u_procp;
    vdmad_proc_ptr = p;
    pstat_cmd(p, "vdmad", 1, "vdmad");

    for (;;) {
	s = spl6();
	if (!vdma_fault_pending) {

	    /* put vdmad back to regular priority if it became realtime */
	    /* on behalf of a realtime vdma process.                    */

	    if (p->p_pri != PSWP) {
		p->p_pri = PSWP;
		p->p_flag &= ~SRTPROC;
	    }

	    sleep(&vdma_present,PSWP);
	}
	vdma_fault_pending = 0;
	splx(s);
	genesis_fault();
    }
}

vdma_isr(inf)
    struct interrupt *inf;
{
    register struct graphics_descriptor *p = (struct graphics_descriptor *)inf->temp;
    register unsigned long *io_status_register = (unsigned long *)(p->primary + GEN_IOSTAT);
    register unsigned long io_status;
    register struct graphics_info *gi_ptr;
    register struct proc *proc_ptr;

    /* Get the contents of the io status register */

    io_status = *io_status_register;

    /* Clear interrupt bits in io status register */

    *io_status_register = io_status & ~(IOST_DMA_PG_FLT | IOST_DMA_AC_FLT | IOST_DMA_INT);

    /* Get graphics info pointer for user causing fault */

    gi_ptr = p->g_info_array[p->g_last_dma_lock_slot - 1];
    if (gi_ptr == (struct graphics_info *)0)
	panic("Vdma Fault after user exit");

    /* Check to see if we can handle this now instead of scheduling vdmad */

    if (io_status & IOST_DMA_PG_FLT) {

	/* If genesis_page_fault returns a non_zero value then it handled */
	/* the fault so we don't need to schedule vdmad.                  */

	if (genesis_page_fault(p,gi_ptr,1) != 0)
	    return;
    }

    /* Save in graphics descriptor */

    p->save_io_status = io_status;

    /* Make vdmad a realtime process if it needs to do work on behalf */
    /* of a realtime process.                                         */

    proc_ptr = gi_ptr->gi_proc;
    if( (proc_ptr->p_flag & SRTPROC) &&
	(proc_ptr->p_pri < vdmad_proc_ptr->p_pri)) {

	/* We make vdmad priority 1 higher so that it can */
	/* do its work without having to compete with the */
	/* vdma real time process (which may be compute   */
	/* bound, checking on the status register).       */

	if (proc_ptr->p_rtpri == 0)
	    (void)changepri(vdmad_proc_ptr,0);
	else
	    (void)changepri(vdmad_proc_ptr,proc_ptr->p_rtpri - 1);
    }

    /* Wakeup vdmad */

    wakeup(&vdma_present);
    vdma_fault_pending = 1; /* in case vdmad() is not currently sleeping */

    return;
}

void
genesis_fault()
{
    register struct graphics_descriptor *p;
    register struct graphics_info *gi_ptr;
    register unsigned long io_status;
    extern struct graphics_descriptor *graphics_devices;

    /* Find any blackfoot genesis interfaces that are currently in a */
    /* fault state.                                                  */

    for (p = graphics_devices; p; p = p->next) {

	io_status = p->save_io_status;
	p->save_io_status = 0;
	if (io_status & (IOST_DMA_PG_FLT | IOST_DMA_AC_FLT)) {

	    /* Get graphics info pointer for user causing fault */

	    gi_ptr = p->g_info_array[p->g_last_dma_lock_slot - 1];
	    if (gi_ptr == (struct graphics_info *)0)
		panic("Vdma Fault after user exit");

	    gi_ptr->vdmad_fault_flag = 1;
	    if (io_status & IOST_DMA_PG_FLT)
		(void) genesis_page_fault(p,gi_ptr,0);
	    else
		genesis_access_fault(p,gi_ptr);

	    /* See if process is hanging in gen_exit(). If it is  */
	    /* then it will have cleared the vdmad_fault_flag. If */
	    /* that has happened then we need to do a wakeup here */
	    /* so the exit can complete.                          */

	    if (gi_ptr->vdmad_fault_flag == 0)
		wakeup(&(gi_ptr->vdmad_fault_flag));

	    gi_ptr->vdmad_fault_flag = 0;
	}
    }

    return;
}

/* genesis_page_fault() returns 0 if it could not handle the fault, 1 if it    */
/* could (or its not worth trying again). It should never return 0 if int_flag */
/* (int_flag is 1 if genesis_page_fault() is called under interrupt) is 0.     */

int
genesis_page_fault(p,gi_ptr,int_flag)
    register struct graphics_descriptor *p;
    register struct graphics_info *gi_ptr;
    int int_flag;
{
    register unsigned long fault_address;
    register int fault_count;
    register int count;
    unsigned long save_fault_address;
    unsigned long *spte_ptr;
    unsigned long *sste_ptr;
    unsigned long *save_spte_ptr;
    unsigned long *save_sste_ptr;
    unsigned long *reg_ptr;
    int retval;

    /* Get Fault Address */

    fault_address = *((unsigned long *)(p->primary + GEN_FAULT_ADDR));
    save_fault_address = fault_address;

    /* Get Fault Count -- The Fault Count Register should always    */
    /* be >= 1. The only true fault is the first one. The Fault     */
    /* Count Register gives an indication of what addresses will    */
    /* be accessed in the future. It can be safely ignored (It is   */
    /* currently ignored for access faults). For better performance */
    /* we can use it to fault in pages in advance. If we are doing  */
    /* this under interrupt we stop processing faults when we       */
    /* find an address that we need to call vfault() for or we have */
    /* processed VDMA_MAX_ISR_FAULT pages.                          */

    fault_count = *((unsigned long *)(p->primary + GEN_FAULT_CNT));
    if (fault_count == 0)
	panic("genesis_page_fault: Zero value in fault count register.\n");

    if (int_flag && fault_count > VDMA_MAX_ISR_FAULT) {
	if (fault_count >= VDMA_FAULT_THRESH)
	    return(0); /* schedule vdmad */

	fault_count = VDMA_MAX_ISR_FAULT;
    }

    for (count = 0; count < fault_count; count++) {

	retval = genesis_get_page(gi_ptr,fault_address,int_flag,
				  &sste_ptr,&spte_ptr);

	if (count == 0) {

	    /* Save the sste and spte pointers for the fault restart */
	    /* after the loop terminates.                            */

	    save_sste_ptr = sste_ptr;
	    save_spte_ptr = spte_ptr;
	}

	if (int_flag == 0) {

	    /* Make sure we should continue. We shouldn't if        */
	    /* gi_ptr->vdmad_fault_flag == 0 (i.e. it was cleared   */
	    /* due to the process exiting during a sleep in vfault. */

	    if (gi_ptr->vdmad_fault_flag == 0)
		return(1); /* We don't really care */

	    /* If fault_address was bad we terminate the loop   */
	    /* immediately. The spte will have been zero'ed by  */
	    /* genesis_get_page(). If this is the first fault   */
	    /* of the loop, the dma processor will indicate an  */
	    /* error to the vdma user process when we issue the */
	    /* fault restart.                                   */

	    if (retval == -1)
		break;
	}
	else {
	    if (retval == 0) {
		if (count == 0)
		    return(0); /* schedule vdmad */
		else
		    break; /* send restart fault */
	    }
	}

	fault_address += ptob(1);

    } /* End for loop, process next fault if fault_count > 1 */

    /* It is possible that if we serviced more than one fault    */
    /* that the page brought in by the first fault of the above  */
    /* loop could have been paged out to make room for pages     */
    /* brought in later in the loop. This should not normally    */
    /* happen since it would defeat the performance enhancement. */
    /* If it did happen then we need to process the first fault  */
    /* again. We don't check for a valid spte if count == 0      */
    /* since the only way the above loop can terminate with      */
    /* count == 0 is if we broke out of the loop due to a bad    */
    /* fault address. In this case we send the restart fault     */
    /* with an invalid spte intentionally so that the dma        */
    /* processor will indicate an error to the user process.     */

    if (count > 0 && (*save_spte_ptr & GEN_SPTE_VALID) == 0) {

	/* We should never get here if int_flag != 0 */

	if (int_flag != 0)
	    panic("Bad vdma pte under interrupt.\n");

	/* No need to check the return value, because if it */
	/* failed, we still need to restart the fault.      */

	(void) genesis_get_page(gi_ptr,save_fault_address,0,
				&save_sste_ptr,&save_spte_ptr);
    }

    /* Send Restart Fault - We send the saved sste and spte */
    /* since the genesis dma processor is interested in the */
    /* mapping for the first page.                          */

    reg_ptr  = (unsigned long *)(p->primary + GEN_DATA1);
    *reg_ptr = *save_sste_ptr;
    reg_ptr  = (unsigned long *)(p->primary + GEN_DATA2);
    *reg_ptr = *save_spte_ptr;
    reg_ptr  = (unsigned long *)(p->primary + GEN_CMD);
    *reg_ptr = GEN_RESTART_FAULT;

    return(1);
}

/* genesis_get_page() returns 0 if it could not handle the fault under */
/* interrupt (i.e. 0 can only be returned if int_flag is 1), 1 if it   */
/* could. -1 will be returned if the address was illegal (and the spte */
/* will be zero'ed).                                                   */

int
genesis_get_page(gi_ptr,fault_address,int_flag,sste_ptr_ptr,spte_ptr_ptr)
    register struct graphics_info *gi_ptr;
    register unsigned long fault_address;
    int int_flag;
    unsigned long **sste_ptr_ptr;
    unsigned long **spte_ptr_ptr;
{
    register struct spagetable *spt_ptr;
    register struct spt_block *spt_block_ptr;
    register struct pte *pte_ptr;
    register int index;
    register unsigned long physaddr;
    register unsigned long spte;
    int prot;

    /* Check to see if there is an spt_block allocated for this address */

    index = SPT_BLPTR_INDEX(fault_address);
    spt_block_ptr = gi_ptr->gdev->gen_spt_block[index];
    if (spt_block_ptr == (struct spt_block *)0) {

	if (int_flag)
	    return(0);

	/* Allocate a spt_block */

	spt_block_ptr = (struct spt_block *)kmem_alloc(sizeof(struct spt_block));
	bzero((caddr_t)spt_block_ptr,sizeof(struct spt_block));
	gi_ptr->gdev->gen_spt_block[index] = spt_block_ptr;
    }

    /* Check to see if there is an shadow page table allocated for this address */

    index = SPT_BLOCK_INDEX(fault_address);
    spt_ptr = spt_block_ptr->shpagetables[index];
    if (spt_ptr == (struct spagetable *)0) {

	if (int_flag)
	    return(0);

	/* Allocate a shadow page table */

	spt_ptr = (struct spagetable *)kdalloc(1);
	physaddr = gen_get_physaddr((struct proc *)0,&kernvas,(space_t)&kernvas,(caddr_t)spt_ptr,(int *)0);
	if (physaddr == 0)
	    panic("Could not get physical address of shadow page table");

	if (processor == M68040)
	    gen_cache_write_through(&kernvas,(caddr_t)spt_ptr);

	spt_block_ptr->shpagetables[index] = spt_ptr;

	/* Enter into segment table */

	index = SSEG_TBL_INDEX(fault_address);

	/* The following is the equivalent of :                           */
	/*                                                                */
	/*     gi_ptr->gdev->gen_segment_table->sste[index] = <ste>       */
	/*                                                                */
	/* however, it also stores the address of the segment table entry */
	/* so that it can be returned to the calling process.             */

	*(*sste_ptr_ptr = &(gi_ptr->gdev->gen_segment_table->sste[index])) =
	    (physaddr & GEN_SSTE_PTPTR) | GEN_SSTE_VALID;
    }
    else {

	/* Get pointer to Shadow segment table entry so that it */
	/* can be returned to the calling process.              */

	index = SSEG_TBL_INDEX(fault_address);
	*sste_ptr_ptr = &(gi_ptr->gdev->gen_segment_table->sste[index]);
    }

    /* Get physical address map for fault address */

    if (int_flag == 0) {

	physaddr = gen_get_physaddr(gi_ptr->gi_proc,gi_ptr->gi_vas,gi_ptr->gi_space,(caddr_t)fault_address,&prot);

	/* return -1 if gen_get_physaddr failed. get_physaddr should only */
	/* fail if the fault address is not a legal address in the users  */
	/* address space. Zero the shadow pte to guarantee a failure.     */

	if (physaddr == 0) {
	    index = SPAGE_TBL_INDEX(fault_address);

	    /* The following is the equivalent of :                        */
	    /*                                                             */
	    /*     spt_ptr->spte[index] = 0                                */
	    /*                                                             */
	    /* however, it also stores the address of the page table entry */
	    /* so that it can be returned to the calling process.          */

	    *(*spte_ptr_ptr = &(spt_ptr->spte[index])) = 0;
	    return(-1);
	}
    }
    else {
	pte_ptr = vastopte(gi_ptr->gi_vas, (caddr_t)fault_address);
	if (pte_ptr == (struct pte *)0 || !pte_ptr->pg_v)
	    return(0);

	/* Construct physical address */

	physaddr  = (pte_ptr->pg_pfnum << PGSHIFT);
	physaddr |= (fault_address & ~PG_FRAME);
	prot = pte_ptr->pg_prot;
    }

    /* Enter into page table. We set the reference bit here since  */
    /* currently the blackfoot processor does not. (They believe   */
    /* that the performance hit for updating reference bits is     */
    /* greater than the potential page thrashing that might happen */
    /* if they don't set reference bits. If we set it here, it     */
    /* will slow down any thrashing.                               */

    if (prot == 0)
	spte = (physaddr & GEN_SPTE_PFRAME) | GEN_SPTE_WRITE | GEN_SPTE_REF | GEN_SPTE_VALID;
    else
	spte = (physaddr & GEN_SPTE_PFRAME) | GEN_SPTE_REF | GEN_SPTE_VALID;

    index = SPAGE_TBL_INDEX(fault_address);

    /* The following is the equivalent of :                        */
    /*                                                             */
    /*     spt_ptr->spte[index] = spte                             */
    /*                                                             */
    /* however, it also stores the address of the page table entry */
    /* so that it can be returned to the calling process.          */

    *(*spte_ptr_ptr = &(spt_ptr->spte[index])) = spte;

    /* Tell VM System that we are interested in this translation  */
    /* make sure notify is not already set  (this can happen with */
    /* if another vdma device has already set it).                */

    if (!vdma_cset(gi_ptr->gi_vas,gi_ptr->gi_space, (caddr_t)fault_address)) {
	if (vdma_set(gi_ptr->gi_vas,gi_ptr->gi_space, (caddr_t)fault_address) != 1)
	    panic("genesis_page_fault: vdma_set failure.\n");
    }

    if (processor == M68040)
	gen_cache_write_through(gi_ptr->gi_vas,(caddr_t)fault_address);

    return(1);
}

void
genesis_access_fault(p,gi_ptr)
    register struct graphics_descriptor *p;
    register struct graphics_info *gi_ptr;
{
    register struct spagetable *spt_ptr;
    register struct spt_block *spt_block_ptr;
    register int index;
    register unsigned long fault_address;
    register unsigned long physaddr;
    register unsigned long spte;
    register unsigned long sste;
    unsigned long *reg_ptr;
    int tmp;
    int prot;

    /* Get Fault Address */

    fault_address = *((unsigned long *)(p->primary + GEN_FAULT_ADDR));

    /* Check to see if there is an spt_block allocated for this address */

    index = SPT_BLPTR_INDEX(fault_address);
    spt_block_ptr = gi_ptr->gdev->gen_spt_block[index];
    if (spt_block_ptr == (struct spt_block *)0)
	return;

    /* Check to see if there is an shadow page table allocated for this address */

    index = SPT_BLOCK_INDEX(fault_address);
    spt_ptr = spt_block_ptr->shpagetables[index];
    if (spt_ptr == (struct spagetable *)0)
	return;

    /* Get Shadow segment table entry */

    index = SSEG_TBL_INDEX(fault_address);
    sste = gi_ptr->gdev->gen_segment_table->sste[index];

    /* Make sure process is swapped in (pfault() does */
    /* not work if process is swapped out).           */

    if (gi_ptr->gi_proc->p_flag & SSWAPPED)
	swapin(gi_ptr->gi_proc);

    /* Make sure page is still read only. We are not notified of write  */
    /* promotions, so it is possible that the shadow pte does not agree */
    /* with the real pte. In this case we just update the shadow pte    */
    /* and avoid calling pfault().                                      */

    physaddr = gen_get_physaddr(gi_ptr->gi_proc,gi_ptr->gi_vas,gi_ptr->gi_space,(caddr_t)fault_address,&prot);
    if (physaddr != 0 && prot == 0)
	spte = (physaddr & GEN_SPTE_PFRAME) | GEN_SPTE_WRITE | GEN_SPTE_REF | GEN_SPTE_VALID;
    else {

	/* Call pfault to attempt resolution of fault */
	/* We don't check for failure, since we want  */
	/* to call restart fault anyway. Also, it is  */
	/* possible that the page has been stolen, in */
	/* which case the vfault() call in gen_get_   */
	/* physaddr(0 may fix things up for us.       */

	gi_ptr->gi_proc->p_flag |= SKEEP;
	(void)pfault(gi_ptr->gi_vas, 1, gi_ptr->gi_space, (caddr_t)fault_address);
	gi_ptr->gi_proc->p_flag &= ~SKEEP;

	/* Make sure we should continue. We shouldn't if        */
	/* gi_ptr->vdmad_fault_flag == 0 (i.e. it was cleared   */
	/* due to the process exiting during a sleep in pfault. */

	if (gi_ptr->vdmad_fault_flag == 0)
	    return;

	/* Get physical address map for fault address with new protection bit */

	physaddr = gen_get_physaddr(gi_ptr->gi_proc,gi_ptr->gi_vas,
				    gi_ptr->gi_space,(caddr_t)fault_address,
				    &prot);
	if (physaddr == 0)
	    spte = 0;
	else {
	    if (prot == 0)
		spte = (physaddr & GEN_SPTE_PFRAME) | GEN_SPTE_WRITE | GEN_SPTE_REF | GEN_SPTE_VALID;
	    else
		spte = (physaddr & GEN_SPTE_PFRAME) | GEN_SPTE_REF | GEN_SPTE_VALID;
	}
    }

    index = SPAGE_TBL_INDEX(fault_address);
    spt_ptr->spte[index] = spte;

    /* We don't need to set the vdma notify bit here */
    /* since we will only get access faults after an */
    /* initial page fault.                           */

    /* Send Restart Fault */

    reg_ptr  = (unsigned long *)(p->primary + GEN_DATA1);
    *reg_ptr = sste;
    reg_ptr  = (unsigned long *)(p->primary + GEN_DATA2);
    *reg_ptr = spte;
    reg_ptr  = (unsigned long *)(p->primary + GEN_CMD);
    *reg_ptr = GEN_RESTART_FAULT;

    return;
}

unsigned long
gen_get_physaddr(p,vas,space,vaddr,prot_ret)
    struct proc *p;
    vas_t *vas;
    space_t space;
    caddr_t vaddr;
    int *prot_ret;
{
    register struct pte *pte_ptr;
    register unsigned long tmp;

    /* Get pte for address */

    pte_ptr = vastopte(vas, vaddr);

    /* Make sure entry is valid */

    if (pte_ptr == (struct pte *)0 || !pte_ptr->pg_v) {

	/* Make sure process is swapped in (vfault() does */
	/* not work if process is swapped out).           */

	if ((p->p_flag & SSWAPPED) == 0)
	    swapin(p);

	/* Try to fault it in. Set SKEEP flag    */
	/* so that process doesn't get swapped   */
	/* during vfault (Yes, this can happen!) */

	p->p_flag |= SKEEP;
	tmp = vfault(vas, 0, space, vaddr);
	p->p_flag &= ~SKEEP;
	if (tmp != 0)
	    return(0);


	pte_ptr = vastopte(vas, vaddr);
	if (pte_ptr == (struct pte *)0 || !pte_ptr->pg_v)
	    return(0);
    }

    /* Construct physical address */

    tmp = (pte_ptr->pg_pfnum << PGSHIFT);
    tmp |= ((unsigned long) vaddr & ~PG_FRAME);

    /* return protection */

    if (prot_ret != (int *)0)
	*prot_ret = pte_ptr->pg_prot;

    return(tmp);
}

void
gen_cache_write_through(vas,vaddr)
    vas_t *vas;
    caddr_t vaddr;
{
    register struct pte *pte_ptr;
    register unsigned long pte_bits;

    pte_ptr = vastopte(vas, vaddr);

    /* This should never happen since vaddr has already  */
    /* been verified before gen_cache_write_through has  */
    /* been called.                                      */

    if (pte_ptr == (struct pte *)0)
	panic("Cache_write_through: bad address\n");

    /* check to make sure page is currently copy back */

    pte_bits = *(unsigned long *)pte_ptr;

    if ((pte_bits & (PG_CI|PG_CB)) == PG_CB) {

	/* Turn off Copy Back caching */

	pte_bits &= ~PG_CB;
	*(unsigned long *)pte_ptr = pte_bits;

	/* Flush appropriate tlb */

	if (vas == &kernvas)
	    purge_tlb_select_super(vaddr);
	else
	    purge_tlb_select_user(vaddr);

	/* Push cache */

	push_cached_page(pte_bits & PG_FRAME);
    }

    return;
}

#ifdef GENESIS_DEBUG

gen_validate(pid)
    int pid;
{
    register int i;
    register int j;
    register int k;
    register unsigned long vaddr;
    register unsigned long pte_bits;
    register unsigned long spte_bits;
    register struct graphics_descriptor *p;
    register struct graphics_info **gi_ptr;
    register struct spagetable *spt_ptr;
    register struct spt_block *spt_block_ptr;
    struct proc *proc;
    struct pte *pte_ptr;
    extern struct graphics_descriptor *graphics_devices;
    extern int (*kdb_printf)();

    proc = pfind(pid);
    if (proc == (struct proc *)0) {
	(*kdb_printf)("Could not find proc pointer for pid in gen_validate\n");
	return;
    }

    /* Find Graphics descriptor */

    p = graphics_devices;

    while (p != (struct graphics_descriptor *)0) {
	if (is_blackfoot(p))
	    break;

	p = p->next;
    }

    if (p == (struct graphics_descriptor *)0) {
	(*kdb_printf)("Could not find genesis graphics descriptor\n");
	return;
    }

    /* Find slot for proc of interest */

    gi_ptr = p->g_info_array;
    j = p->g_numb_slots;
    for (i = 0; i < j; i++, gi_ptr++) {
	if (   *gi_ptr != (struct graphics_info *)0
	    && (*gi_ptr)->gi_vas   == proc->p_vas) {

	    break;
	}
    }

    if (i >= j) {
	(*kdb_printf)("Could not find slot for proc of interest\n");
	return;
    }

    /* Look at all shadow page table entries and report caching mode */
    /* for each one.                                                 */

    vaddr = 0;
    for (i = 0; i < NSVASSEG; i++) {
	spt_block_ptr = (*gi_ptr)->gdev->gen_spt_block[i];
	if (spt_block_ptr != (struct spt_block *)0) {
	    for (j = 0; j < NSPT_PER_SVASSEG; j++) {
		spt_ptr = spt_block_ptr->shpagetables[j];
		if (spt_ptr != (struct spagetable *)0) {
		    for (k = 0; k < NSPTE_PER_PAGE; k++) {
			spte_bits = spt_ptr->spte[k];
			if (spte_bits != 0) {
			    if ((spte_bits & GEN_SPTE_VALID) == 0)
				(*kdb_printf)("Spte for vaddr %08x not valid\n");
			    else {
				pte_ptr = vastopte(proc->p_vas, vaddr);
				if (pte_ptr == (struct pte *)0)
				    (*kdb_printf)("Pte for vaddr %08x does not exist!\n");
				else {
				    pte_bits = *(unsigned long *)pte_ptr;
				    (*kdb_printf)("vaddr %08x Pte bits %08x Spte bits %08x\n",
						vaddr,pte_bits,spte_bits);
				}
			    }
			}
			vaddr += 4096;
		    }
		}
		else
		    vaddr += 0x400000;
	    }
	}
	else
	    vaddr += 0x8000000;
    }
    return;
}
#endif /* GENESIS_DEBUG */
#endif /* GENESIS */
