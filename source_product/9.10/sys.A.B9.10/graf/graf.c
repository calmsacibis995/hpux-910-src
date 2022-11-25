/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/graf/RCS/graf.c,v $ */
/* $Revision: 1.11.84.6 $      $Author: rpc $ */
/* $State: Exp $        $Locker:  $ */
/* $Date: 94/10/11 07:43:11 $ */

#include "../h/param.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/ipc.h"
#include "../s200/pte.h"
#include "../h/shm.h"
#include "../h/proc.h"
#include "../graf.300/graphics.h"
#include "../s200io/bootrom.h"
#include "../wsio/hpibio.h"
#include "../graf.300/gr_98736.h"
#include "../h/debug.h"
#include "../h/swap.h"
#include "../machine/vmparam.h"
#include "../h/vmmac.h"
#include "../h/mman.h"
#include "../mach.300/cpu.h"

#ifndef true
#define true 1
#define false 0
#endif

extern preg_t *io_map(), *io_map_c(), *io_unmap(), *io_findmap(), *attachreg();
extern reg_t *allocreg();
extern int lockable_mem, growreg(), hdl_procattach(), hdl_procdetach();
extern void freereg(), detachreg();
extern caddr_t kmem_alloc();

#if ((MAX_GRAPH_PROCS+1) > NBPG) || (MAX_GRAPH_PROCS > 255)
	panic more than 255 slot numbers will not fit into byte storage
#endif

#define DIOII_SC(sc) ((sc) >= MIN_DIOII_ISC)

#define BPB	8			     	/* bits per byte */

struct graphics_descriptor *graphics_devices;
int ite_select_code = -1; 			/* impossible select code */
int ite_type = -1;				/* impossible type */
struct graphics_descriptor *graphics_ite_dev = NULL;
struct graphics_descriptor *find_graphics_dev();    /* forward declaration */

int (*ite_bitmap_call)() = NULL;
void graphics_exit(),        (*graphics_exitptr)() = graphics_exit;
void graphics_fork(),        (*graphics_forkptr)() = graphics_fork;
int  graphics_get_sigmask(), (*graphics_sigptr)()  = graphics_get_sigmask;
int ite_memory_used = 0;
int ite_memory_trashed = 0;
int first_sgc_slot, last_sgc_slot;

extern void free_work_buffers(), write_genesis_registers(), gen_exit();

#define sti_call(p,routine,a,b,c) \
     (*(p)->routine)((a),(b),(c), &(p)->glob_config)

typedef int (*function)();
function copy_function();

struct sti_info {
	char    *crt_region[CRT_MAX_REGIONS];
};


/* Forward Declarations */

char *get_frame_addr();
void graphics_unmap_frame(), graph_on(), graph_off();
void set_sizes(), set_topcat_name(), dav_bits_used();
static struct graphics_info *ginfo_alloc();
static struct gdev_info *gdev_alloc();
static void ginfo_free(), gdev_free();

extern vas_t kernvas;

int
graphics_open(dev, mode)
    dev_t dev;
    int mode;
{
    register struct graphics_descriptor *device;
    reg_t *rp;
    preg_t *kprp;

    device = find_graphics_dev(dev);

    /* check if graphics device found */
    if (!device)
        return(ENXIO);

    /* bump the device open count */
    device->count++;

    if (device->g_rp_slot == NULL) {

	/* Allocate a shared region for kernel use. */
	if ((rp = allocreg(NULL, swapdev_vp, RT_SHARED)) == NULL) {
	    msg_printf("graphics_open: allocreg().\n");
	    u.u_error = ENOMEM;
	    return(u.u_error);
	}

	if (growreg(rp, 1, DBD_DZERO) < 0) {
	    msg_printf("graphics_open: growreg().\n");
	    freereg(rp);
	    u.u_error = ENOMEM;
	    return(u.u_error);
	}

	/*
	 *  Attach this region to the kernel vas so we can find it when
	 *  other processes want to attach to it.
	 */
	if ((kprp = attachreg(&kernvas, rp, PROT_URWX,
			      PT_GRAFLOCKPG, 0, 0, 0, 1)) == NULL) {
	    msg_printf("graphics_open: attachreg().\n");
	    freereg(rp);
	    u.u_error = ENOMEM;
	    return(u.u_error);
	}

	/* Now lock the page into memory. */
	if (!mlockpreg(kprp)) {
	    msg_printf("graphics_open: mlockpreg().\n");
	    detachreg(&kernvas, kprp);
	    u.u_error = ENOMEM;
	    return(u.u_error);
	}
	regrele(rp);

	/* Save the region ptr in the graphics descriptor. */
	device->g_rp_slot = rp;
	device->g_lock_page_addr = (unsigned char *) kprp->p_vaddr;

	bzero(device->g_lock_page_addr, NBPG);
    }

    return(0);
}


/*
 *  This routine is called on each close.
 *
 *  WARNING, because the ALL_CLOSES flag is set, the close routine cannot
 *  clear process owned resources (because the owning process may not call
 *  the close routine).
 */
int
graphics_close(dev)
    dev_t dev;
{
    register struct graphics_descriptor *p;
    int s;
    register struct graphics_info *gi_ptr = NULL;


    p = find_graphics_dev(dev);
    if (p == NULL)
	panic("opened graphics device not found on close");

    /* give up the lock */
    if (p->g_locking_proc == u.u_procp) 
       graphics_unlock(p);

    /*
     *  Decrement open count.  If it goes to zero then
     *  detach the graphics I/O regions.
     */

    if (p->count <= 0) 
        panic("graphics_close called more often than graphics_open");

    if (--p->count == 0) {

	/* Notify the ite that we are not using frame memory  */

	if (p == graphics_ite_dev) 
	    ite_memory_used = 0;

	if ((s = graf_slot_get(p, u.u_procp->p_pid, FALSE)) >= 0)
	    gi_ptr = p->g_info_array[s];

	/* 
	 *  Here we remove the mapping of the frame buffer regions from the
	 *  address space of the running process.  We do this here because
	 *  prior to 8.0, on this last close, shared memory allocated to the
	 *  DISPLAY was released.  This had the side effect of detaching this
	 *  region from the address space of this LAST process.  We do this
	 *  here to retain these semantics.  If the mapping has already been
	 *  released, we do nothing.
	 */
	if (gi_ptr && gi_ptr->crt_control_base) {
			
	    /* unmap all the regions associated with the fb */
	    graphics_unmap_frame(p, gi_ptr, dev);
	    if (u.u_error)
	        return(u.u_error);
	}

    }
}


int static_colormap = 0;	/* Initially, the colormap is variable */
int map_sti_sys_only_regions = 0;	/* for debugging */

graphics_ioctl(dev, cmd, arg, mode)
    dev_t dev;
    int cmd;
    register int *arg;
    int mode;
{
    register struct graphics_descriptor *p;
    register preg_t *prp;
    int frame_size;
    caddr_t phys_addr;
    register my_slot, size;
    short prot;

    p = find_graphics_dev(dev);
    if (p == NULL)
	panic("opened graphics device not found on close");

    if ((my_slot = graf_slot_get(p, u.u_procp->p_pid, TRUE)) < 0)
	return(ENXIO);

    switch (cmd) {

    case GCID:
	*arg = p->desc.crt_id;
	break;

    case GCDESCRIBE:
	/* Get current configuration */
	if (p->type==SGC_TYPE) {
	    
		struct sti_info *si = (struct sti_info *)
						p->g_info_array[my_slot]->gdev;
		get_sti_configuration(p);
		bcopy(si->crt_region, p->desc.crt_region,
			sizeof(si->crt_region));
	}

	/* Give them alpha/graphics on/off if an ITE */
	if (p == graphics_ite_dev)
	    p->desc.crt_attributes |= (CRT_ALPHA_ON_OFF|CRT_GRAPHICS_ON_OFF);

	/* get control base and frame base for this proc */
	p->desc.crt_control_base = p->g_info_array[my_slot]->crt_control_base;
	p->desc.crt_frame_base = p->g_info_array[my_slot]->crt_frame_base;

	*(crt_frame_buffer_t *)arg = p->desc;
	break;

    case GCSLOT:
	{
	    register struct gcslot_info *slot_info = (struct gcslot_info *)arg;
	    register struct graphics_info *gi_ptr = p->g_info_array[my_slot];

	    /* 
	     *  Check to see if GCSLOT has already been called for this 
	     *  process.
	     */
	    if (gi_ptr->slot_page_base == (unsigned char *)0) {
		/*
		 *  Get the region for slot table and attach it to the
		 *  current running process
		 */
		reglock(p->g_rp_slot);
		if ((prp = attachreg(gi_ptr->gi_vas, p->g_rp_slot,
				PROT_URW, PT_GRAFLOCKPG,
				(slot_info->myslot_address) ? PF_EXACT : 0,
				slot_info->myslot_address, 0, 1)) == NULL) {
		    msg_printf("graphics_ioctl (gcslot): attachreg().\n");
		    regrele(p->g_rp_slot);
		    u.u_error = EIO;
		    return(u.u_error);
		}

		hdl_procattach(&u, gi_ptr->gi_vas, prp);
		regrele(p->g_rp_slot);

		/* Save slot page base */
		gi_ptr->slot_page_base = (unsigned char *)prp->p_vaddr;

	    }

	    /* 
	     *  Return the slot page attach address and my slot number to 
	     *  process 
	     */
	    gi_ptr->gcslot_count++;
	    slot_info->myslot_address = gi_ptr->slot_page_base;
	    slot_info->myslot_number = my_slot + 1;

	    break;
	}

    case GCON:
	if (p == graphics_ite_dev)
	    graphics_on();
	else
	    graph_on(p);

	break;

    case GCOFF:
	if (p == graphics_ite_dev)
	    graphics_off();
	else
	    graph_off(p);
	break;

    case GCLOCK:
    case GCLOCK_MINIMUM:
    case GCDMA_TRY_LOCK:
	{
	    int got_dma_lock;

	    /* not considered legal to do double locks */
	    if (p->g_locking_proc == u.u_procp) {
		u.u_error = EBUSY;
		return(u.u_error);
	    }

	    /* wait for and ask locking process to give up lock */
	    size = CRIT();
	    while (cant_grab_the_lock(p)) {
		sleep(p, PZERO+1);
	    }

	    /* Check to see if this is a genesis interface card */
	    got_dma_lock = 0;
	    if (is_genesis(p)) {

		/*
		 *  Do not allow lock if dma is currently in progress and 
		 *  this is not GCDMA_TRY_LOCK.
		 */
		if (dma_in_progress(p) ) {
		    if (cmd != GCDMA_TRY_LOCK) {
			UNCRIT(size);
			u.u_error = EAGAIN;
			return(u.u_error);
		    }
		}
		else {
		    got_dma_lock = 1;
		    p->g_last_dma_lock_slot = my_slot + 1;
		}
	    }

	    /* acquire the lock for my process */
	    p->g_locking_proc = u.u_procp;
	    p->g_lock_slot = my_slot + 1;
	    p->g_lock_page_addr[0] = p->g_lock_slot;

	    /* set my inuse bit */
	    p->g_lock_page_addr[p->g_lock_slot] = 1;

	    UNCRIT(size);

	    if (cmd == GCLOCK_MINIMUM) {
		 p->flags &= ~G_GCLOCK;
	    }
	    else  {
		/*
		 *  Save away the signals which the user is currently
		 *  catching, and set a per-device flag telling whether
		 *  the device is currently locked via a GCLOCK or a
		 *  GCLOCK_MINIMUM.
		 */
		p->g_locking_caught_sigs = u.u_procp->p_sigcatch;
		p->flags |= G_GCLOCK;
	    }

	    if (is_genesis(p)) {

		if (cmd != GCDMA_TRY_LOCK || got_dma_lock != 0) {

		    /* Write new work buffer pointers */
		    write_genesis_registers(p, my_slot);
		}
	    }

	    if (p == graphics_ite_dev) {
		if (ite_bitmap_call != NULL)
			(*ite_bitmap_call)(TRUE);
	    }

	    if (cmd == GCDMA_TRY_LOCK)
		*arg = got_dma_lock;
	    break;

	}

    case GCUNLOCK:
    case GCUNLOCK_MINIMUM:

	/* give up the lock */
	if (p->g_locking_proc != u.u_procp)
	    return(u.u_error = EPERM);
	else
	    graphics_unlock(p);
	break;

    case GCAON:
	if ((p->flags & DEFAULT) && (p->flags & ALPHA))
	    alpha_on();
	break;

    case GCAOFF:
	if ((p->flags & DEFAULT) && (p->flags & ALPHA))
	    alpha_off();
	break;

    case GCSTATIC_CMAP:

	/* Remember who it was that did it. */
	static_colormap = (int) u.u_procp;
	break;

    case GCVARIABLE_CMAP:
	static_colormap = 0;
	break;

    /*
     *  GCMAP maps the control registers & frame buffer into user space.  
     *  It returns the virtual address in *arg.  Due to an amazing accident,
     *  it also returns the virtual address as a return value, i.e., in  
     *  u.u_r.r_val1.  I am confident that some user has stumbled across this,
     *  and is using it in a program for which the source is lost.  Hence, 
     *  don't change this undocumented, unsupported feature.
     */

    case GCMAP:
	{
	    register struct graphics_info *gi_ptr = p->g_info_array[my_slot];

	    /* Has GCMAP already been called? */
	    if (gi_ptr->crt_control_base != (char *)0) {

		/* Return the virtual address to the process */
		if (p->type==SGC_TYPE)
			*arg = (int)
			      ((struct sti_info *) gi_ptr->gdev)->crt_region[0];
		else
			*arg = (int)gi_ptr->crt_control_base;
	    }
	    else {

		/* Tell the ite that memory may be altered */
		if (p == graphics_ite_dev) {
		    ite_memory_trashed = 1;
		    ite_memory_used = 1;
		}

		/* Get the size in bytes of the frame buffer */
		if (p->flags & FRAME)
		    frame_size = p->psize + p->ssize;
		else
		    frame_size = p->psize;

		/* Get the physical address of the frame buffer */
		if (p->type==SGC_TYPE)
		    phys_addr = p->primary;
		else if (DIOII_SC(p->isc))
		    phys_addr = (caddr_t)(0x1000000 + (p->isc-132) * 0x400000);
		else if ((p->flags & GRAPHICS) || !(p->flags & MULTI))
		    phys_addr = (caddr_t)p->primary - LOG_IO_OFFSET;
		else
		    phys_addr = (caddr_t)p->secondary - LOG_IO_OFFSET;

		/*
		 *  Attach a region mapping the frame buffer to the
		 *  calling process
		 */
		prot = (mode & FWRITE) ? PROT_URW : PROT_USER|PROT_READ;

		if (p->type==SGC_TYPE) {
		    int i, zot, *rlist;
		    caddr_t v_addr;
		    struct sti_rom *sti;
		    region_desc *r;
		    struct sti_info *si = (struct sti_info *) gi_ptr->gdev;

		    for (i=0; i<CRT_MAX_REGIONS; i++)
			si->crt_region[i] = NULL;

		    v_addr = (caddr_t) *arg;	/* first virtual address or 0 */

		    sti = (struct sti_rom *) p->glob_config.region_ptrs[0];

		    save_sti(p);
		    rlist = (int *) ((int) sti + get_int(sti->dev_region_list)&~3);
		    for (i=0; ; i++, rlist+=4) {
			    zot = get_int(rlist);
			    r = (region_desc *) &zot;
			    if (!r->sys_only || map_sti_sys_only_regions) {
				prp = io_map_c(gi_ptr->gi_vas, v_addr,
					       phys_addr+r->offset*4096,
					       btorp(r->length*4096), prot,
					       r->cache ? PHDL_CACHE_WT
							: PHDL_CACHE_CI);
				if (prp==NULL || prp->p_vaddr==0)
					return (u.u_error = ENOMEM);

				v_addr = prp->p_vaddr;

				switch(i) {
				case 0:
					si->crt_region[0] = v_addr;
					*arg = (int) v_addr;
					break;
				case 1:
					gi_ptr->crt_frame_base  =  v_addr;
					break;
				case 2:
					gi_ptr->crt_control_base =  v_addr;
					break;
				default:
					si->crt_region[i-2] = v_addr;
					break;
				}
				v_addr += pageup(r->length*4096);
			    }
			    if (r->last)
				break;
		    }
		    restore_sti(p);
		}
		else if (is_genesis(p)) {
		    preg_t *ctrlgbus_prp;
		    caddr_t v_addr;

		    /*
		     *  For Genesis we need two pregions:
		     *  
		     *  1) Start of address space up to the beginning of
		     *     the privileged page.
		     *  2) The non-privileged page (The privileged page is
		     *     not mapped at all) through the end of the interface
		     *     card address space.
		     */
		    v_addr = (caddr_t) *arg;
		    prp = io_map(gi_ptr->gi_vas, v_addr, phys_addr,
				 btop(GEN_PRIV_PAGE), prot);

		    if ((prp == NULL) || (prp->p_vaddr == 0)) {
			u.u_error = ENOMEM;
			return(u.u_error);
		    }

		    v_addr = prp->p_vaddr;

		    /* If diagnostic bit set then map the privileged page */
		    if (GRAPH_DIAG(dev)) {
			ctrlgbus_prp = io_map(gi_ptr->gi_vas,
					      v_addr + GEN_PRIV_PAGE,
					      phys_addr + GEN_PRIV_PAGE,
					      btop(p->psize - 
						   GEN_PRIV_PAGE),
					      prot);
		    }
		    else {
			ctrlgbus_prp = io_map(gi_ptr->gi_vas,
					      v_addr + GEN_NON_PRIV_PAGE,
					      phys_addr + GEN_NON_PRIV_PAGE,
					      btop(p->psize - 
						   GEN_NON_PRIV_PAGE),
					      prot);
		    }

		    if ((ctrlgbus_prp == NULL) ||
			(ctrlgbus_prp->p_vaddr == 0)) {

			reglock(prp->p_reg);
			io_unmap(prp);
			u.u_error = ENOMEM;
			return(u.u_error);
		    }
		}
		else {

		    prp = io_map(gi_ptr->gi_vas, *arg, phys_addr,
				 (p->flags & FRAME) ?
				 btorp(p->psize) : btorp(frame_size), prot);

		    if ((prp == NULL) || (prp->p_vaddr == 0)) {
			u.u_error = ENOMEM;
			return(u.u_error);
		    }

		    if (p->flags & FRAME) {
			preg_t *frame_prp;

			frame_prp = io_map(gi_ptr->gi_vas,
					   prp->p_vaddr + p->psize,
					   p->secondary - LOG_IO_OFFSET,
					   btorp(p->ssize), prot);

			if ((frame_prp == NULL) || (frame_prp->p_vaddr == 0)) {
			    reglock(prp->p_reg);
			    io_unmap(prp);
			    u.u_error = ENOMEM;
			    return(u.u_error);
			}
		    }
		}

		if (p->type != SGC_TYPE) {
		    /* save the virtual address to the process and return it. */
		    gi_ptr->crt_control_base = (char *)prp->p_vaddr;
		    p->g_map_size = frame_size;

		    *arg = (int)gi_ptr->crt_control_base;
		    if (DIOII_SC(p->isc)) {
		        int offset;
		        unsigned char *z;

		        z = (unsigned char *) p->primary;
		        if (!testr(z, 1))
		    		return(u.u_error = ENXIO);
		        offset = z[0x5d]<<8 | z[0x5f];
		        offset = z[offset]<<16;
		        gi_ptr->crt_frame_base=gi_ptr->crt_control_base+offset;
		    }
		    else  /* DIO */
		        gi_ptr->crt_frame_base = gi_ptr->crt_control_base +
						 p->psize;
		}
	    }
	    gi_ptr->gcmap_count++;
	}
	break;

    case GCUNMAP:
	{
	    register struct graphics_info *gi_ptr = p->g_info_array[my_slot];

	    /*  
	     *  The GCUNMAP request is made for two different types of
	     *  arguments -- the framebuffer/control space, and the shared
	     *  lock page.  Correctly passed, these addresses will correspond
	     *  to those found in the crt_control_base and slot_page_base
	     *  fields of the correct graphics_info structure, respectively.
	     *  For the first case, the graphics_unmap_frame() routine is 
	     *  called to unmap all the regions that may be associated with
	     *  the framebuffer.
	     */
	    char *gcmap_val = 
			p->type==SGC_TYPE
				? ((struct sti_info *) gi_ptr->gdev)->crt_region[0]
				: gi_ptr->crt_control_base;

	    if (gcmap_val != (char *)0 && gcmap_val == (char *) *arg) {

		if (--gi_ptr->gcmap_count == 0) {

		    /* unmap the fb/ctrl space regions */
		    graphics_unmap_frame(p, gi_ptr, dev);
		    if (u.u_error)
		        return(u.u_error);
		}

	    } else if (gi_ptr->slot_page_base != (unsigned char *)0 &&
		       gi_ptr->slot_page_base == (unsigned char *) *arg) {

		if (--gi_ptr->gcslot_count == 0) {
		    
		    /* unmap the shared lock page region */		
		    register vas_t *gi_vas = gi_ptr->gi_vas;
		    caddr_t vaddr = (caddr_t) *arg;;

		    if (prp = findprp(gi_vas, gi_ptr->gi_space, vaddr)) {

			/* 
			 *  If the pregion points to a non-null region, and 
			 *  it's the region associated with this display's 
			 *  lock page, then go ahead and detach it from this 
			 *  user's virtual address space.
			 */
			if (prp->p_reg && prp->p_reg == p->g_rp_slot) {
			    hdl_procdetach(&u, gi_vas, prp);
			    detachreg(gi_vas, prp);
			    gi_ptr->slot_page_base = (unsigned char *) 0;
			} 
			else {
			    regrele(prp->p_reg);
			    u.u_error = EINVAL;
			}
		    }
		    else
		        u.u_error = EINVAL;
		}
	    }
	    else {
		u.u_error = EINVAL;
	    }
	}
	break;

    case GCDMA_BUFFER_ALLOC:
	{
	    register struct graphics_work_buffer *gw_ptr =
		    (struct graphics_work_buffer *)arg;
	    register struct gdev_info *gdev_ptr;

	    /* Are we a WAMPUM or a BLACKFOOT? */
	    if (is_genesis(p) == 0) {
		u.u_error = EIO;
		return(u.u_error);
	    }

	    if (p->g_locking_proc == u.u_procp) {

		/*
		 *  If we have the lock then we can't allow this operation
		 *  because we could deadlock.
		 */
		u.u_error = EBUSY;
		return(u.u_error);
	    }

	    /*
	     *  Check to see if buffers have already been allocated, if so
	     *  then skip the allocation and just return current addressses.
	     */
	    gdev_ptr = p->g_info_array[my_slot]->gdev;
	    if (gdev_ptr->gen_work_buf_0_ptr == 0) {
		u.u_error = alloc_work_buffers(p->g_info_array[my_slot]);
		if (u.u_error != 0)
		    return(u.u_error);
	    }

	    gw_ptr->work_buffer_0 = (unsigned char *)
	                                    gdev_ptr->gen_work_buf_0_uvptr;
	    gw_ptr->work_buffer_1 = (unsigned char *)
	                                    gdev_ptr->gen_work_buf_1_uvptr;
	    break;
	}

    case GCDMA_BUFFER_FREE:
	{
	    register struct graphics_work_buffer *gw_ptr =
		    (struct graphics_work_buffer *)arg;
	    register struct gdev_info *gdev_ptr;

	    /* Are we a WAMPUM or a BLACKFOOT? */
	    if (is_genesis(p) == 0) {
		u.u_error = EIO;
		return(u.u_error);
	    }

	    /* Can't free if process currently has the lock */
	    if (p->g_locking_proc == u.u_procp) {
		u.u_error = EBUSY;
		return(u.u_error);
	    }

	    /*
	     *  Check to see if this process could have started DMA and if so,
	     *  check if DMA is in progress.
	     */
	    if (p->g_last_dma_lock_slot == (my_slot + 1) ) {

		if (dma_in_progress(p)) {
		    u.u_error = EAGAIN;
		    return(u.u_error);
		}
	    }

	    /*
	     *  Check to make sure that the addresses passed in correspond
	     *  to the allocated buffer.
             */
	    gdev_ptr = p->g_info_array[my_slot]->gdev;
	    if ((gw_ptr->work_buffer_0 != 
		        (unsigned char *)gdev_ptr->gen_work_buf_0_uvptr) ||
		(gw_ptr->work_buffer_1 !=
		        (unsigned char *)gdev_ptr->gen_work_buf_1_uvptr)) {

		u.u_error = EINVAL;
		return(u.u_error);
	    }

	    free_work_buffers(p->g_info_array[my_slot]);

	    break;
	}

    /*
     *  The following ioctl() is an HP internal ioctl used by Sherlock 
     *  diagnostics to allow extended manipulatation of a specific graphics
     *  devices.
     */
    case GCDIAG_COMMAND:
	{
	    struct crt_diag_args *diag = (struct crt_diag_args *)arg;

	    if (GRAPH_DIAG(dev)) {
		switch (diag->request) {

		    case GRAPH_DIAG_RESET:
		        switch (p->desc.crt_id) {

			    case S9000_ID_98705:
			    case S9000_ID_98736:
			        if (!init_device(p->primary, p->isc, 1))
				    u.u_error = EIO;
				break;

			    default:
			        u.u_error = EINVAL;
				break;
			    }
		        break;

		    case GRAPH_DIAG_ATTRIBUTES:
			if (p == graphics_ite_dev)
			    diag->u_diag.diag_attributes |= GRAPH_DIAG_ITE;
			else
			    diag->u_diag.diag_attributes &= ~GRAPH_DIAG_ITE;
		        break;

		    default:
		        u.u_error = EINVAL;
			break;
		    }
	    }
	    else {
		u.u_error = EACCES;
	    }
	    break;
	}

#define	GCTERM _IOWR('G',20,int)
#define SOFT 2
    case GCTERM:
	switch (*arg) {
	case 0: if (p==graphics_ite_dev)
			term_reset(SOFT);
		break;
	case 1: *arg = graphics_ite_dev==p;
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
	break;
	
    default:
	u.u_error = EINVAL;
	break;
    }
    return(u.u_error);
}



void
graphics_unmap_frame(p, gi_ptr, dev)
    register struct graphics_descriptor *p;
    register struct graphics_info *gi_ptr;
    dev_t dev;
/* 
 *  This routine takes a (per graphics device) graphics_descriptor structure
 *  and a (per process) graphics_info structure and unmaps all the regions 
 *  associated with this process's access to this device's framebuffer.
 *  This includes both the framebuffer and the control space regions if they
 *  are in separate regions and in the case of Genesis, also includes the 
 *  control and gbus region.  This routine is called from both
 *  graphics_close() and graphics_ioctl(GCUNMAP).
 */
{
	register preg_t *prp;
	register vas_t *gi_vas = gi_ptr->gi_vas;
	caddr_t vaddr = (caddr_t)gi_ptr->crt_control_base;

	if (p->type==SGC_TYPE) {
		register int i;
		struct sti_info *si = (struct sti_info *) gi_ptr->gdev;
		
		graphics_unmap_vaddr(gi_ptr, &gi_ptr->crt_frame_base);
		graphics_unmap_vaddr(gi_ptr, &gi_ptr->crt_control_base);

		for (i=0; i<CRT_MAX_REGIONS; i++)
			graphics_unmap_vaddr(gi_ptr, &si->crt_region[i]);
	}

	prp = findprp(gi_vas, gi_ptr->gi_space, vaddr);
	if (prp == NULL)
		return;
	if (prp->p_type != PT_IO) {
		regrele(prp->p_reg);
		u.u_error = EINVAL;
		return;
	}

	vaddr = prp->p_vaddr;

	io_unmap(prp);
	gi_ptr->crt_control_base = (char *)0;
	gi_ptr->crt_frame_base   = (char *)0;

	if (p->flags & FRAME) {
		if (prp = findprp(gi_vas, gi_ptr->gi_space, vaddr + p->psize)) {
			if (prp->p_type == PT_IO)
				io_unmap(prp);
			else {
				regrele(prp->p_reg);
				u.u_error = EINVAL;
			}
		} else
			u.u_error = EINVAL;
	}
	else {
		if (is_genesis(p)) {

			/* Need to unmap control/gbus pregion */
			if (GRAPH_DIAG(dev)) {
				prp = findprp(gi_vas, gi_ptr->gi_space,
				    vaddr + GEN_PRIV_PAGE);
			}
			else {
				prp = findprp(gi_vas, gi_ptr->gi_space,
				    vaddr + GEN_NON_PRIV_PAGE);
			}

			if (prp) {
				if (prp->p_type == PT_IO) {
					io_unmap(prp);
				}
				else {
					regrele(prp->p_reg);
					u.u_error = EINVAL;
				}
			}
			else
				u.u_error = EINVAL;
		}
	}
}

graphics_unmap_vaddr(gi_ptr, vaddr)
register struct graphics_info *gi_ptr;
caddr_t *vaddr;
{
	register preg_t *prp;
	register vas_t *gi_vas = gi_ptr->gi_vas;

	if (*vaddr == NULL)
		return;
	prp = findprp(gi_vas, gi_ptr->gi_space, *vaddr);
	*vaddr = NULL;
	if (prp == NULL)
		return;
	if (prp->p_type != PT_IO) {
		regrele(prp->p_reg);
		u.u_error = EINVAL;
		return;
	}

	io_unmap(prp);
}


void
alpha_graphics(turn_it_on)
{
    if (graphics_ite_dev == NULL) 
        return;

    if (turn_it_on)
        graph_on(graphics_ite_dev);
    else
        graph_off(graphics_ite_dev);
}	



void
graph_on(p)
    register struct graphics_descriptor *p;
{
    register int slot;
    register vas_t *gi_vas;
    register preg_t *prp;
    register caddr_t pphys_addr;
    register caddr_t sphys_addr;
    register caddr_t virt_addr;

    if (p->type==SGC_TYPE)
	sgc_graphics_on_off(p, 1);
    else if (p->flags & MULTI)
        testr(p->primary+1,1);
    else if (p->gcntl)
        *p->gcntl = p->gon;

    p->flags |= GRAPHICS;

    if (p->flags & MULTI) {

	/*
	 *  We need to determine whether the device has been mapped into user
	 *  space.  We use the graf_slot_get() routine to try to get the device
	 *  dependent data.  If graf_slot_get() fails, there is no way it could
	 *  be mapped.  If it succeeds, then the crt_control_base field in the
	 *  the graphics_info structure will be non-zero if the device has been
	 *  mapped.  If it has not been mapped, we can simply return.
	 */
	if (((slot = graf_slot_get(p, u.u_procp->p_pid, FALSE)) < 0) ||
	    (p->g_info_array[slot]->crt_control_base == (char *)0))
	    return;

	gi_vas = p->g_info_array[slot]->gi_vas;

	pphys_addr = (caddr_t)p->primary - LOG_IO_OFFSET;
	sphys_addr = (caddr_t)p->secondary - LOG_IO_OFFSET;

	/* Search for primary or secondary find the current one. */
	if ((prp = io_findmap(gi_vas, pphys_addr)) != (preg_t *)NULL)
	    return;
	else if ((prp = io_findmap(gi_vas, sphys_addr)) == (preg_t *)NULL) {
	    msg_printf("graph_on: io_findmap() failed\n");
	    return;
	}

	virt_addr = prp->p_vaddr;
	reglock(prp->p_reg); 	/* io_unmap() requires the pregion be locked */
	io_unmap(prp);
	prp = io_map(gi_vas, virt_addr, pphys_addr, btorp(p->psize), PROT_URW);
	if ((prp == NULL) || (prp->p_vaddr == 0)) {
	    msg_printf("graph_on: io_map() failed\n");
	    return;
	}
    }
}



void
graph_off(p)
    register struct graphics_descriptor *p;
{
    register vas_t *gi_vas;
    register preg_t *prp;
    register int slot;
    register caddr_t pphys_addr;
    register caddr_t sphys_addr;
    register caddr_t virt_addr;

    if (p->type==SGC_TYPE)
	sgc_graphics_on_off(p, 0);
    else if (p->flags & MULTI)
        testr(p->secondary+1,1);
    else if (p->gcntl)
        *p->gcntl = p->gon;

    p->flags &= ~GRAPHICS;

    if (p->flags & MULTI) {

	/*
	 *  We need to determine whether the device has been mapped into user
	 *  space.  We use the graf_slot_get() routine to try to get the device
	 *  dependent data.  If graf_slot_get() fails, there is no way it could
	 *  be mapped.  If it succeeds, then the crt_control_base field in the
	 *  the graphics_info structure will be non-zero if the device has been
	 *  mapped.  If it has not been mapped, we can simply return.
	 */
	if (((slot = graf_slot_get(p, u.u_procp->p_pid, FALSE)) < 0) ||
	    (p->g_info_array[slot]->crt_control_base == (char *)0))
	    return;

	gi_vas = p->g_info_array[slot]->gi_vas;

	pphys_addr = (caddr_t)p->primary - LOG_IO_OFFSET;
	sphys_addr = (caddr_t)p->secondary - LOG_IO_OFFSET;

	/* Search for primary or secondary find the current one. */
	if ((prp = io_findmap(gi_vas, sphys_addr)) != (preg_t *)NULL)
	    return;
	else if ((prp = io_findmap(gi_vas, pphys_addr)) == (preg_t *)NULL) {
	    msg_printf("graph_off: io_findmap() failed\n");
	    return;
	}

	virt_addr = prp->p_vaddr;
	reglock(prp->p_reg); 	/* io_unmap() requires the pregion be locked */
	io_unmap(prp);
	prp = io_map(gi_vas, virt_addr, sphys_addr, btorp(p->ssize), PROT_URW);
	if ((prp == NULL) || (prp->p_vaddr == 0)) {
	    msg_printf("graph_off: io_map() failed\n");
	    return;
	}
    }
}

sgc_graphics_on_off(p, desired)
register struct graphics_descriptor *p;
{
	init_flags  i_flags;
	init_inptr  i_in;
	init_outptr i_out;

	save_sti(p);

	bzero(&i_flags, sizeof(i_flags));
	bzero(&i_in, sizeof(i_in));

	i_flags.wait = true;
	i_flags.no_chg_tx = true;	/* Don't change text settings. */
	i_flags.no_chg_bet = true;	/* Don't change berr timer settings. */
	i_flags.no_chg_bei = true;	/* Don't change berr int settings. */
	i_flags.nontext = desired;	/* Set to what the user wants. */
	sti_call(p, init_graph, &i_flags, &i_in, &i_out);

	restore_sti(p);
}

caddr_t sti_save_area;

save_sti(p)
register struct graphics_descriptor *p;
{
	state_flags s_flags;
	state_inptr s_in;
	state_outptr s_out;

	bzero(&s_flags, sizeof(s_flags));
	bzero(&s_in, sizeof(s_in));

	if (sti_save_area == 0) {
		struct sti_rom *sti =
			(struct sti_rom *) p->glob_config.region_ptrs[0];

		/* Save area size is in 4-byte quantities. */
		sti_save_area = kmem_alloc(get_int(sti->max_state_storage)*4);
		if (sti_save_area==NULL)
			panic("save_sti: can't allocate");
	}

	s_flags.wait = true;
	s_flags.save = true;
	s_in.save_addr = (int) sti_save_area;

	woody_fix_save();
	sti_call(p, state_mgmt, &s_flags, &s_in, &s_out);
}

restore_sti(p)
register struct graphics_descriptor *p;
{
	state_flags s_flags;
	state_inptr s_in;
	state_outptr s_out;

	bzero(&s_flags, sizeof(s_flags));
	bzero(&s_in, sizeof(s_in));

	s_flags.wait = true;
	s_in.save_addr = (int) sti_save_area;

	sti_call(p, state_mgmt, &s_flags, &s_in, &s_out);
	woody_fix_restore();
}


extern int (*make_entry)();
int (*graphics_saved_make_entry)();

/* Just guessing on the sizes for this unsupported display. */
crt_frame_buffer_t HP98204A_desc = {
	S9000_ID_98204A,
	1024*1024*2,	/* size in bytes of mapped frame buffer area */
	1280, 390,	/* width,height in pixels (displayed part) */
	512, 390,	/* width,height in pixels (total area) */
	512*1,		/* length of one row in bytes */
	8,		/* total number of fb bits per one screen pixel */
	1,		/* number of those bits that are used */
	1,		/* number of frame buffer planes */
	512*390*1,	/* in bytes of a frame buffer bank */
	"HP98204 Display",
	0,				/* no attributes! */
};

crt_frame_buffer_t HP98546A_desc = {	/* alias the series 200 98204B */
	S9000_ID_98204B,
	1024*1024*2,	/* size in bytes of mapped frame buffer area */
	512, 390,	/* width,height in pixels (displayed part) */
	512, 390,	/* width,height in pixels (total area) */
	512*1,		/* length of one row in bytes */
	8,		/* total number of fb bits per one screen pixel */
	1,		/* number of those bits that are used */
	1,		/* number of frame buffer planes */
	512*390*1,	/* in bytes of a frame buffer bank */
	"HP98546 Display",
	0,				/* no attributes! */
};

/* No idea on the sizes for this unsupported series 200 display. */
crt_frame_buffer_t HP98627A_desc = {
	S9000_ID_98627A,
	0*0*1,		/* size in bytes of frame buffer area */
	0, 0,		/* width,height in pixels (displayed part) */
	0, 0,		/* width,height in pixels (total area) */
	0*1,		/* length of one row in bytes */
	8,		/* total number of fb bits per one screen pixel */
	1,		/* number of those bits that are used */
	1,		/* number of frame buffer planes */
	0*0*1,		/* in bytes of a frame buffer bank */
	"HP98627 Display",
	0,		/* no attributes? */
};



    /* When we unifdef REGION we can expand the GD and GDADDR macros   */
    /* macros inline. This was done just so we didn't pollute the code */
    /* with lots of ifdefs for what is really a minor change.          */

#define GD (*p)
#define GDADDR p
#define setname(n) strcpy(p->desc.crt_name, n);

#define TIG_ZHERE_REG		0x7053
#define GEN_FBC_STATUS  	0x44000
#define GEN_48_PLANES(addr) 	((addr)[GEN_FBC_STATUS] & 0x00000400)

struct graphics_descriptor *add_sti_device();

int
graphics_make_entry(id, isc)
    int id;
    struct isc_table_type *isc;
{
    register struct sysflag *sflag = (struct sysflag *) &sysflags;
    register caddr_t addr = (caddr_t) isc->card_ptr;  /* interface card regs */
    register struct graphics_descriptor *p, *new;
    char *ptr;
    int passed_init, offset;
    extern int bus_master_count;
    extern int vdma_present;
    extern int vdma_isr();
    extern int vdma_page_info();
    static int vdma_installed = 0;

    /* Allocate a graphics descriptor and zero it */

    p = (struct graphics_descriptor *)
                kmem_alloc(sizeof (struct graphics_descriptor));
    bzero(p, sizeof(struct graphics_descriptor));

    GD.isc = isc->my_isc;
    GD.type = DIO_TYPE;			    /* as opposed to SGC's 3 */
    GD.desc.crt_bits_per_pixel = 8;         /* good default */
    GD.desc.crt_attributes = 0x0;           /* start fresh  */

    switch(isc->my_isc) {
    
    case EXTERNALISC+19:	/* internal low resolution */

	if (sflag->big_graph) {

	    /* 204B is the same as HP98546A */

	    GD.desc      = HP98546A_desc;
	    GD.psize     = 25*1024;
	    GD.ssize     = 25*1024;
	    GD.flags     = ALPHA | MULTI;
	    GD.primary   = (char *) (0x530000 + LOG_IO_OFFSET);
	    GD.secondary = (char *) (0x538000 + LOG_IO_OFFSET);
	}
	else {

	    /* 98204A is unsupported but should work */
	    GD.desc      = HP98204A_desc;
	    GD.psize     = 30*1024;
	    GD.ssize     = 30*1024;
	    GD.flags     = ALPHA | MULTI;
	    GD.primary   = (char *) (0x530000 + LOG_IO_OFFSET);
	    GD.secondary = (char *) (0x538000 + LOG_IO_OFFSET);
	}
	break;

    default:	
	GD.primary = addr; /* a good default (interface card regs.) */
	switch(id) {
	
	case 0x1c:   /* Moon Unit */
	    GD.desc  = HP98627A_desc;
	    GD.flags = 0;
	    GD.psize = 128*1024;
	    GD.gcntl = addr+1;
	    GD.gon   = 0x80;
	    GD.goff  = 0x00;
	    break;
	case 0x39:
	    /* Is there an STI rom piggybacked here? */
	    offset = (((addr[0x61] & 0xff)<<24)
		    + ((addr[0x63] & 0xff)<<16)
		    + ((addr[0x65] & 0xff)<<8)
		    + (addr[0x67] & 0xff));
	    if (offset!=0 && (new=add_sti_device(addr+offset, isc->my_isc))) {
		struct sti_rom *sti = 
		    (struct sti_rom *) new->glob_config.region_ptrs[0];
		int i = sti->global_rom_rev;	/* to split up */
		printf("    %s Bit-Mapped Display (revision %d.%02d/%d) at select code %d\n",
			new->desc.crt_name, 
			i>>4, i&15, sti->local_rom_rev,
			isc->my_isc);
		return 1;	/* success */
	    }
		
	    passed_init = init_device(isc->card_ptr, isc->my_isc, 0);
	    switch(addr[0x15]) {
	    
	    case 0x1:   /* Gatorbox or Gatoraide */
		GD.desc.crt_id = S9000_ID_98700;
		setname("HP98700 Bit Mapped Display");
		GD.desc.crt_attributes |= CRT_BLOCK_MOVER_PRESENT;
		GD.psize = 64*1024;     /* always DIO I */
		addr[0x6009] = 0; /* enable all planes */
		addr[3] = 4;	/* enable frame buffer */
		set_sizes(GDADDR);
		GD.desc.crt_bits_used = 
		        GD.desc.crt_planes = planes_deep(GDADDR);
		
		/* Disable frame buffer. The ITE will re-enable it as needed */
		addr[3] = 0;
		break;
	    
	    case 0x5:   /* low-cost color (LCC) Catseye */
		setname("HP98549 Bit Mapped Display");
		goto common_catseye;

	    case 0x6:   /* high-res color (HRC) Catseye */
		setname("HP98550 Bit Mapped Display");
		goto common_catseye;

	    case 0x7:   /* 1280x1024 mono Catseye */
		setname("HP98548 Bit Mapped Display");
		goto common_catseye;

	    case 0x9:   /* 640x480 4-plane color Catseye 98541 (319x) */
		setname("HP98451 Bit Mapped Display");
common_catseye:	GD.desc.crt_id = S9000_ID_S300;
		GD.desc.crt_attributes |= CRT_BLOCK_MOVER_PRESENT;
		GD.psize = 64*1024;    			/* assume DIO I      */
		*(short *)&addr[0x4090] = 0xff<<8;	/* enable all planes */
		set_sizes(GDADDR);
		GD.desc.crt_bits_used =
		        GD.desc.crt_planes = planes_deep(GDADDR);
		if ((*((short *) (addr+0x4800)) & 8)==0)
		    GD.desc.crt_attributes |= CRT_ADVANCED_HARDWARE;
		break;

	    case 0x2:   /* generic topcat */
		GD.desc.crt_id = S9000_ID_S300;
		GD.desc.crt_attributes |= CRT_BLOCK_MOVER_PRESENT;
		GD.psize = 64*1024;			/* always DIO I	     */
		*(short *)&addr[0x4090] = 0xff<<8;	/* enable all planes */
		set_sizes(GDADDR);
		GD.desc.crt_bits_used =
		        GD.desc.crt_planes = planes_deep(GDADDR);
		set_topcat_name(GDADDR);
		break;

	    case 0x4:   /* Renaissance (SRX) */
		GD.desc.crt_id = S9000_ID_98720;
		setname("HP98720 SRX Bit Mapped Display");
		/* Is there a transform engine? */
		if (testr(addr+0x8002, 1))
		    GD.desc.crt_attributes |= CRT_ADVANCED_HARDWARE;
		goto common_renais;

	    case 0xc:   /* Tigershark */
		GD.desc.crt_id = S9000_ID_98705;
		setname("HP98705 Bit Mapped Display");
		
		/* Always a transform engine */
		GD.desc.crt_attributes |= (CRT_ADVANCED_HARDWARE |
					   CRT_BLOCK_MOVER_PRESENT);
		set_sizes(GDADDR);
		GD.desc.crt_bits_used =	(addr[TIG_ZHERE_REG]&0x1 ? 8 : 24);
		GD.desc.crt_planes = 8;
		break;
 
	    case 0x8:   /* da Vinci(turbo SRX)*/
		GD.desc.crt_id = S9000_ID_98730;
		setname("HP98730 Bit Mapped Display");
		/* Transform engine? */
		if (testr(addr+0xf1fc, 1))
		GD.desc.crt_attributes |= CRT_ADVANCED_HARDWARE;

common_renais:	GD.desc.crt_attributes |= CRT_BLOCK_MOVER_PRESENT;
		GD.psize = 128*1024;    		/* assume DIO I      */
		set_sizes(GDADDR);
		dav_bits_used(p, &p->desc.crt_planes,
			      &p->desc.crt_bits_used);
		break;

	    case 0xb: /* Genesis WAMPUM interface */
		GD.desc.crt_id = S9000_ID_98736;
		setname("HP98736 PDMA Bit Mapped Display");
		GD.desc.crt_attributes |= CRT_DMA_HARDWARE;
		goto common_genesis;

	    case 0xd: /* Genesis BLACKFOOT interface */
		p->desc.crt_id = S9000_ID_98736;
		setname("HP98736 VDMA Bit Mapped Display");

		/* Initialize vdma interrupt handling */

		vdma_present = 1;			/* Used in main()    */

		isrlink(vdma_isr,VDMA_INT_LEVEL, addr + GEN_INT,
			VDMA_INT_REQUEST, VDMA_INT_REQUEST, 0x0, (int)p);

		addr[GEN_INT] = (VDMA_INT_ENABLE |
				 ((VDMA_INT_LEVEL - VDMA_INT_OFFSET) << 4));

		p->desc.crt_attributes |= (CRT_CAN_INTERRUPT |
					   CRT_VDMA_HARDWARE);

		/*
		 *  Tell the VM System that we are interested in notification
		 *  of change of status on selected mappings.  To do this we
		 *  hand vdma_install() a pointer to a procedure to call to 
		 *  tell us when a selected mapping changes. This procedure,
		 *  vdma_page_info(), is also called by the VM system when it
		 *  wants to look at the reference and dirty bits in the
		 *  shadow page tables.  We only call vdma_install() for the
		 *  first genesis device, because we don't need 
		 *  vdma_page_info() to be called more than once, and we
		 *  have no way of knowing which device it is called for.
		 */
		if (!vdma_installed) {
		    if (!vdma_install(vdma_page_info))
		        passed_init = 0;
		    vdma_installed = 1;
		}
		/* Always a transform engine */

common_genesis:	GD.desc.crt_attributes |= (CRT_ADVANCED_HARDWARE |
					   CRT_BLOCK_MOVER_PRESENT);
		GD.psize = 128*1024;
		set_sizes(GDADDR);

		GD.desc.crt_bits_used = 24;
		if (testr(addr+GEN_FBC_STATUS, 1))
		    GD.desc.crt_planes = (GEN_48_PLANES(addr) ? 24 : 48);
		else
		    passed_init = 0;

		/*
		 *  Since both Genesis interface cards can perform DMA, they
		 *  both can become bus masters.  The bus_master_count variable
		 *  reflects the current number of possible bus masters in the
		 *  system.
		 */
		bus_master_count++;

		break;

	    case HPHYPER_SECONDARY_ID:
		GD.desc.crt_id = S9000_ID_A1096A;
		setname("HPA1096A Monochrome Bit Mapped Display");
		set_sizes(GDADDR);
		GD.desc.crt_plane_size =
		        GD.desc.crt_map_size = (2048*1024)/BPB;
		GD.desc.crt_bits_per_pixel = 1;
		GD.desc.crt_bits_used      = 1;
		GD.desc.crt_planes         = 1;
		GD.desc.crt_x_pitch	   = 2048/BPB;
		GD.desc.crt_attributes     = 0;
		break;

	    default:
		goto reject;
	    }
	    break;

	default:
reject:	    return(*graphics_saved_make_entry)(id, isc);
	}
	break;
    }

    p->next = graphics_devices;
    graphics_devices = p;

    /* initialize generic fields */
    p->g_rp_map  = NULL;
    p->g_rp_slot  = NULL;


    /* save the descriptor pointer of the ite */
    if (isc->my_isc == ite_select_code)		/* if it's the ITE */
        graphics_ite_dev = p;


#ifndef QUIETBOOT
    if (isc->my_isc >= EXTERNALISC && isc->my_isc <= MAXEXTERNALISC)
        printf("    %s at 0x%x\n", p->desc.crt_name, addr-LOG_IO_OFFSET);   /* funny addr */
    else
        io_inform(p->desc.crt_name, isc, 0);      /* A real select code */
#endif /* ! QUIETBOOT */

    if (!passed_init)
        printf("        turn-on failed!\n");

    /*
     *  Replace the first blank in the name with a null so that the
     *  product name in GCDESCRIBE comes out neatly.
     */
    for (ptr=p->desc.crt_name; *ptr && *ptr!=' '; ptr++);
        *ptr = '\0';			/* replace a blank or the null */

    return(DIOII_SC(isc->my_isc));	/* Only claim a DIO II display */
}


int
init_device(card_ptr, my_isc, do_ite)
    int *card_ptr;
    unsigned char my_isc;
    int do_ite;
{
    char reset_value;

    /* If it is the ITE, it is already initialized */
    if (!do_ite && my_isc == ite_select_code)
        return 1;

    /* reset the card */
    reset_value = 0x39;
    bcopy_prot(&reset_value, ((char *)card_ptr)+1, 1);
    
    /* Let it settle */
    snooze(100000);

    return(idrom_init((unsigned char *)card_ptr));
}


void
set_sizes(p)
    register struct graphics_descriptor *p;
{
    register unsigned char *addr;
    register unsigned short h, w, color, total_h, total_w;

    if (DIOII_SC(p->isc)) {			/* if DIO II display */
	p->psize     = (p->primary[0x101]+1) * 1024 * 1024;
	p->flags     = ALPHA;			/* no separate frame buffer */
	p->secondary = NULL;			/* all one big happy chunk */
	p->ssize     = 0;			/* no secondary */
	p->desc.crt_map_size = 1024*1024*8;
	p->desc.crt_bits_per_pixel = 32;
    }
    else {
	p->flags     = ALPHA | FRAME;
	p->secondary = get_frame_addr(p->primary);
	p->ssize     = get_frame_size(p->primary);
	
	/* Must set psize elsewhere */
	p->desc.crt_map_size = 1024*1024*2;
    }

    addr = (unsigned char *) p->primary;	/* addr of ID rom */
    asm("	movp.w	0x11(%a4),%d7");	/* h=visible screen height */
    asm("	movp.w	0x0d(%a4),%d6");	/* w=visible screen width */
    asm("	movp.w	0x33(%a4),%d5");	/* color=colormap pointer */
    asm("	movp.w	0x9(%a4),%d4 ");	/* total_h=framebuf height */
    asm("	movp.w	0x5(%a4),%d3 ");	/* total_w=framebuf width */

    p->desc.crt_total_x = total_w;
    p->desc.crt_total_y = total_h;
    p->desc.crt_x = w;
    p->desc.crt_y = h;
    p->desc.crt_x_pitch = total_w * (p->desc.crt_bits_per_pixel / 8);
    p->desc.crt_plane_size = total_w * total_h;  
    /*
     *  crt_plane_size is the number of bytes in a minimun configuration
     *  framebuffer.  The total number of framebuffer pixels also equals
     *  the number of bytes in this framebuffer.  
     */
}


void
set_topcat_name(p)
    register struct graphics_descriptor *p;
{
    char *name;

    if (p->desc.crt_x==1024  && p->desc.crt_y==400) {
	p->desc.crt_attributes |= CRT_Y_EQUALS_2X;
	switch (p->desc.crt_bits_used) {
	    case 1: name="HP98542"; break;
	    case 4: name="HP98543"; break;
	    default: goto bad;
	}
    }
    else if (p->desc.crt_x==1024 && p->desc.crt_y==768) {
	switch (p->desc.crt_bits_used) {
	    case 1: name="HP98544"; break;
	    case 4: name="HP98545"; break;
	    case 6: name="HP98547"; break;
	    default: goto bad;
	}
    }
    else {
bad:	msg_printf("set_topcat_name: unknown %dx%d display planes=%d\n",
		   p->desc.crt_x, p->desc.crt_y, p->desc.crt_bits_used);
        name="";		/* punt */
    }
    strcpy(p->desc.crt_name, name);
    strcat(p->desc.crt_name, " Bit Mapped Display");
}


/* Find out how many planes deep a byte-per-pixel crt is. */
int
planes_deep(p)
    register struct graphics_descriptor *p;
{
    register unsigned char *addr;
    register int planes, save_bits, mask;

    planes = p->primary[0x5b];		/* number of planes or 0 */
    if (planes == 0) {			/* if 0, figure it out yourself */
	addr = (unsigned char *) get_frame_addr(p->primary);
	if (testr(addr, 1)) {
	    save_bits = *addr;  	/* save the pixel */
	    *addr = ~0;			/* set all bits */
	    mask = *addr;	    	/* how many here? */
	    *addr = save_bits;		/* restore the pixel */
	    while (mask & 01) {
		planes++;
		mask >>= 1;
	    }
	}
	else {
	    printf("planes_deep: can't read location 0x%x\n", addr);
	    planes=0;
	}
    }
    return planes;
}


/* Describe parts of the Renaissance/daVinci control space */
struct dav {
    char dummy1[0x0015];	      char id;
    char dummy4[0x4090-0x0015-1]; int fbwen;
    char dummy2[0x40b2-0x4090-4]; short fold;
    char dummy3[0x40be-0x40b2-2]; short drive;
    char dummy5[0x40d2-0x40be-2]; short zrr;
    char dummy6[0x40ee-0x40d2-2]; short rep_rule;
};


/* 
 *  Find out how many planes deep a Renaissance or daVinci is. 
 *  This routines cannot be used to determine the number of planes in a
 *  Tigershark.  The Tigershark bus lines are tied high instead of low as
 *  in daVinci and Renaissance.  Luckily, Tigershark provides registers
 *  which provide this information.
 */
void
dav_bits_used(p, planes_there, bits_used)
    register struct graphics_descriptor *p;
    int *planes_there, *bits_used;
{
    register struct dav *cs = (struct dav *) p->primary;
    register unsigned int *fb = (unsigned int *) get_frame_addr(p->primary);
    register int i, bits, planes;
    register unsigned tmp_fold, tmp_drive, tmp_fbwen, oneval, zeroval;

    /* Did we create the structure correctly? */
    if ((int) &(((struct dav *) 0)->rep_rule) != 0x40ee)
        panic("dav_bits_used: struct dav messed up");

    tmp_fold = cs->fold;		/* save fold register */
    cs->fold = 0;			/* don't fold 32 bits into a byte */
    tmp_drive = cs->drive;		/* save drive register */
    cs->drive = 0xf;			/* all four frame buffer boards */
    tmp_fbwen = cs->fbwen;		/* save frame buffer enable */
    cs->fbwen = ~0;			/* enable everything */
    if (cs->id == 8)			/* is this a daVinci? */
        cs->zrr = 0x33;			/* Z buffer replacement rule = MOVE */
    cs->rep_rule = 0x33;		/* replacement rule = MOVE */

    *fb = 0;  zeroval = *fb;
    *fb = ~0; oneval  = *fb;

    for (planes=0; (oneval ^ zeroval) & (1<<planes) && planes<24; planes++)
        ;
    for (bits=0, i=0; i<32; i++)
        if ((oneval ^ zeroval) & (1<<i)) bits++;

    cs->drive = tmp_drive;		/* restore drive */
    cs->fbwen = tmp_fbwen;		/* restore frame buffer enable */
    cs->fold  = tmp_fold;		/* restore fold register */

    *bits_used = bits;
    *planes_there = planes;
}


char *
get_frame_addr(addr)
    register unsigned char *addr;
{
    char *retval; 
    int offset;

    offset = addr[0x5d]<<8 | addr[0x5f];
    offset = addr[offset]<<16;

    if (((unsigned int) addr < LOGICAL_IO_BASE) ||
	((unsigned int) addr > LOGICAL_IO_BASE + 0x600000)) 
        retval = (char *) addr+offset;			/* is relative */
    else {
	offset += LOG_IO_OFFSET;			/* is absolute */
	retval = (char *) offset;			/* is absolute */
	}
    return retval;
}

int
get_frame_size(addr)
    register unsigned char *addr;
{
    int size;

    size = *(addr+0x5)<<8 | *(addr+0x7);
    size *= *(addr+0x9)<<8 | *(addr+0xb);
    switch(*(addr+0x19)) {
        case 0:  size = (size+7)/8;	break;	/* bit per pixel */
	case 1:				break;	/* byte per pixel */
	case 2:  size *=2;		break;	/* word per pixel */
	case 3:  size *=4;		break;	/* long per pixel */
	default:			break;	/* unknown; should error */
    }
    return(size);
}

void
graphics_link()
{
    graphics_saved_make_entry = make_entry;
    make_entry = graphics_make_entry;
}


#define MASK(sig) (1<<((sig)-1))
#define BLOCKMASK (MASK(SIGTSTP) | MASK(SIGTTIN) | MASK(SIGTTOU))

/*
 * PASSMASK contains signals that are always passed through during a
 * graphics ioctl(GCLOCK).
 */
#define PASSMASK  ~(MASK(SIGFPE) | MASK(SIGILL) | MASK(SIGSEGV) | MASK(SIGBUS))

int
graphics_get_sigmask(procp)
    register struct proc *procp;
/*
 *  This routine returns one of two signal masks.  If the 'procp' process
 *  does not currently hold any graphics locks, the routine simply returns 
 *  the value of the signal mask present in the proc structure.  If procp 
 *  does hold a GCLOCK (as opposed to a GCLOCK_MINIMUM) graphics lock, the 
 *  routine returns a modified version of the signal mask in the proc 
 *  structure.  This modified version causes the caught signals AT THE TIME 
 *  OF THE GCLOCK (see the graphics(7) man page) to be blocked.  Several 
 *  other signals are also blocked.  This routines is called from issig() in
 *  sys/kern_sig.c and is necessary because if any of these signals get
 *  through to a process holding a graphics lock, deadlock may result.
 *
 *  The signals SIGBUS, SIGFPE, SIGILL, and SIGSEGV will not be blocked.
 *  This is because the restarted instruction will cause the same
 *  exception again. We must let the signal handler run, and rely on it
 *  to avoid activities that cause deadlocks.
 */

{
    register struct graphics_descriptor *gd;
    register int sigtemp;

    /* Look thru graphics devices and check if process has device locked */
    for (gd = graphics_devices; gd; gd = gd->next) {

	/* Does this proc have this graphics device locked ? */
	if (gd->g_locking_proc == procp &&
	    gd->g_lock_page_addr[gd->g_lock_slot] == 1)
			
	if (gd->flags & G_GCLOCK) {
	    sigtemp = (procp->p_sigmask |
                       (gd->g_locking_caught_sigs & PASSMASK) |
                       BLOCKMASK);

	    /*
	     *  If the process is traced, mask keyboard generated signals
	     *  while the graphics lock is owned.
	     */
	    if (procp->p_flag & STRC)
	        sigtemp |= (MASK(SIGINT) | MASK(SIGQUIT));
	    return (sigtemp);
	}		
    }

    /* if no graphics lock is being held, exit with real signal mask */
    return(procp->p_sigmask);
}


void
graphics_exit()
/*
 *  This routine is called from the exit() and exec() paths to cleanup
 *  the graphics lock and the per-process data structure space.
 */
{
    register struct graphics_descriptor *p;

    if (static_colormap == (int) u.u_procp)	/* Did we make it static? */
        static_colormap=0;     			/* Make it un-static */

    for (p = graphics_devices; p; p = p->next) {

	/* give up the lock */
	if (p->g_locking_proc == u.u_procp) 
	    graphics_unlock(p);

	/* remove this proc from slot table */
	if (p->g_numb_slots > 0) 
	    graf_slot_rem(p);
    }
    u.u_procp->p_flag2 &= ~SGRAPHICS;
}


void
graphics_fork(pptr, cptr)
    register struct proc *pptr;
    register struct proc *cptr;
/*
 *  This routine is called from the fork path to allocate a per-process
 *  graphics info structre for the child.  The parent's graphics memory map
 *  information from the parent's graphics_info structures is transfered to
 *  the child's newly created graphics_info structures.  The routine is
 *  called from procdup() in sys.300/pm_procdup.c.
 */
{
    register struct graphics_descriptor *p;
    int pslot, cslot;
    struct graphics_info *gi_pptr;
    struct graphics_info *gi_cptr;

    for (p = graphics_devices; p; p = p->next) {
	if ((pslot = graf_slot_get(p, pptr->p_pid, FALSE)) < 0)
	    continue;
	if ((cslot = graf_slot_get(p, cptr->p_pid, TRUE)) < 0) {
	    u.u_error = ENOMEM;
	    return;
	}
	gi_pptr = p->g_info_array[pslot];
	gi_cptr = p->g_info_array[cslot];
	gi_cptr->gi_vas = cptr->p_vas;
	gi_cptr->gi_space = (space_t)gi_cptr->gi_vas;
	gi_cptr->gi_proc = cptr;
	gi_cptr->slot_page_base   = gi_pptr->slot_page_base;
	gi_cptr->crt_control_base = gi_pptr->crt_control_base;
	gi_cptr->crt_frame_base   = gi_pptr->crt_frame_base;
	gi_cptr->gcmap_count      = gi_pptr->gcmap_count;
	gi_cptr->gcslot_count     = gi_pptr->gcslot_count;
	cptr->p_flag2 |= SGRAPHICS;
    }
}


/* unlock the graphics device */
graphics_unlock(p)
    register struct graphics_descriptor *p;
{
    int s;

    s = CRIT();
    p->g_locking_proc = NULL;

    /* 
     *  We do not really have to do this since the driver uses 
     *  g_locking_proc to valididate g_lock_page_addr[0],
     *  and g_lock_slot, but we do just to be nice.
     */
    p->g_lock_page_addr[p->g_lock_slot] = 0;
    p->g_lock_page_addr[0] = 0;
    p->g_lock_slot = 0;

    UNCRIT(s);

    /* wakeup processess waiting for the lock on this device */
    wakeup(p); 
    
    if ((p == graphics_ite_dev) && (ite_bitmap_call != NULL))
        (*ite_bitmap_call)(FALSE);
}


int graf_really_locked(p)
    register struct graphics_descriptor *p;
/* 
 *  This routine checks to see if the graphics device described by *gd
 *  is "completely" locked (ie. is there a locking process, and does it have
 *  the inuse bit set).  The routine returns 1 if the device is locked, 
 *  zero otherwise.  This routine is called from check_screen_access().
 */
{
    if (p == NULL || p->g_locking_proc == NULL || 
	p->g_lock_page_addr[p->g_lock_slot] == 0)

        return(0);
    else 
        return(1);
}


int cant_grab_the_lock(gd)
    register struct graphics_descriptor *gd;
/* 
 *  It is the job of this routine to tell the caller whether it can currently
 *  grab the graphics lock, and if not, ask the locking process to give it up.
 *  The routine returns 1 if the device CANNOT currently be grabbed, zero
 *  otherwise.
 */
{
    if (gd == NULL || gd->g_locking_proc == NULL)
        return(0);
    
    /* 
     * If g_locking_proc is non-zero, this means that some process has 
     * done a GCLOCK and has not done a GCUNLOCK, BUT it does NOT mean
     * that the process has not cleared its bit in the lock page. 
     *
     * The g_locking_proc field is effectively being used as a 
     * "g_lock_page_addr[0], g_locking_proc, and g_lock_slot" valid
     * flag.
     */

    if (gd->g_lock_page_addr[0] != 0) {

	/* 
	 * If lock_page[0] is nonzero at this point, no other process
	 * is waiting for the lock except us.  If the inuse bit is 
	 * clear, we can grab the lock.  If not, we ask the locking 
	 * process to give up the lock by writing a 0 to lock_page[0].
	 */

	if (gd->g_lock_page_addr[gd->g_lock_slot] == 0)
	    return(0);
	else 	
	    gd->g_lock_page_addr[0] = 0;
    }

    /* 
     * At this point, either the locking process has the inuse bit set,
     * or lock_page[0] is already zero (ie., there are already processes
     * waiting for the lock).  In either case, we cannot immediately take
     * the lock, so we return 1. 
     */
    return(1);
}


/*
 *  Find the slot number of the current process or allocate one.
 */

int
graf_slot_get(p, pid, allocflag)
    register struct graphics_descriptor *p;
    register pid_t pid;
    int allocflag;
{
    register slot;

    register new_slot;
    register numb_slots = p->g_numb_slots;
    register struct graphics_info **gi_ptr = p->g_info_array;
    static int slot_cache = 0;

    /* Same proc as last time? */
    if (p->g_info_array[slot_cache] != (struct graphics_info *)0 &&
	p->g_info_array[slot_cache]->pid == pid) {

	return(slot_cache);
    }

    new_slot = -1;
    for (slot = 0; slot < numb_slots; slot++, gi_ptr++) {
		
	/* Check if graphics info structure allocated for this slot. */

	if (*gi_ptr == (struct graphics_info *)0) {

	    /*
	     *  Have we found an empty slot yet? If not, store
	     *  this one.
	     */
	    if (new_slot == -1)
	        new_slot = slot;
	}
	else {

	    /* check if already in table */
	    if ((*gi_ptr)->pid == pid) {
		slot_cache = slot;
		return(slot);
	    }
	}
    }

    if (allocflag == FALSE)
        return(-1);

    /* not found in table, so allocate a new slot number */
    if (new_slot == -1) {

	/* Need to increase number of slots */
	if (numb_slots >= MAX_GRAPH_PROCS)
	    return(-1);

	new_slot = numb_slots;
	p->g_numb_slots++;
    }

    gi_ptr = &(p->g_info_array[new_slot]);

    /* Now allocate a new entry */
    *gi_ptr = ginfo_alloc();
    if (*gi_ptr == (struct graphics_info *)0)
        return(-1);

    bzero((caddr_t)(*gi_ptr),sizeof(struct graphics_info));

    (*gi_ptr)->pid = pid;
    (*gi_ptr)->gi_vas = u.u_procp->p_vas;
    (*gi_ptr)->gi_proc = u.u_procp;
    (*gi_ptr)->gi_space = (space_t) u.u_procp->p_vas;
    (*gi_ptr)->gdev = (struct gdev_info *)0;
    slot_cache = new_slot;

    /* Allocate gdev_info structure for genesis or STI */
    if (is_genesis(p) || p->type==SGC_TYPE) {

	(*gi_ptr)->gdev = gdev_alloc();
	if ((*gi_ptr)->gdev == (struct gdev_info *)0) {
	    ginfo_free(*gi_ptr);
	    return(-1);
	}

	bzero((caddr_t)(*gi_ptr)->gdev,sizeof(struct gdev_info));
    }
    u.u_procp->p_flag2 |= SGRAPHICS;
    return(new_slot);
}

/*
 *  Remove current process from slot table
 */
graf_slot_rem(p)
    register struct graphics_descriptor *p;
{
    register slot; 
    register struct graphics_info *gi_ptr;

    if ((slot = graf_slot_get(p, u.u_procp->p_pid, FALSE)) < 0)
        return;

    /* matched, remove from table */
    gi_ptr = p->g_info_array[slot];
    if (gi_ptr->gdev != (struct gdev_info *)0) {

	if (is_genesis(p)) {

	    /* Free work buffers, free up shadow page tables, etc. */
	    gen_exit(p,slot);
	}
	gdev_free(gi_ptr->gdev);
    }

    ginfo_free(gi_ptr);
    p->g_info_array[slot] = (struct graphics_info *)0;

    /* clear lock bit */	
    p->g_lock_page_addr[slot+1] = 0;

    /* See if g_numb_slots can be reduced */

    for (slot = p->g_numb_slots - 1; slot >= 0; slot--) {
	if (p->g_info_array[slot] != (struct graphics_info *)0)
	    break;
    }

    p->g_numb_slots = slot + 1;
    return;
}


static struct graphics_info *
ginfo_alloc()
{
    return((struct graphics_info *)kmem_alloc(sizeof(struct graphics_info)));
}

static struct gdev_info *
gdev_alloc()
{
    return((struct gdev_info *)kmem_alloc(sizeof(struct gdev_info)));
}

static void
ginfo_free(ginfo_ptr)
    struct graphics_info *ginfo_ptr;
{
    (void)kmem_free(ginfo_ptr, sizeof(struct graphics_info));
    return;
}

static void
gdev_free(gdev_ptr)
    struct gdev_info *gdev_ptr;
{
    (void)kmem_free(gdev_ptr, sizeof(struct gdev_info));
    return;
}

extern caddr_t dio2_map_page(), dio2_map_mem();

sgc_init(look_for_console)
{
	register int slot, i;
	register caddr_t physical_addr, logical_addr;
	register struct graphics_descriptor *p;
	register struct sti_rom *sti;

	/*
	 * Where is SGC space?
	 * For s400, sgc space is 2xxxxxxx.
	 * Oddly enough, Strider has one slot, at 2Cxxxxxx.
	 */
	switch (machine_model) {
	case MACH_MODEL_40S:		/* Trailways Matchless */
	case MACH_MODEL_42S:		/* Trailways 25 MHz 68040 */
	case MACH_MODEL_43S:		/* Trailways 33 MHz 68040 */
	case MACH_MODEL_WOODY25:	/* 25MHz woody */
	case MACH_MODEL_WOODY33:	/* 33MHz woody */
		first_sgc_slot=8; last_sgc_slot=11; break;
	case MACH_MODEL_40T:		/* Strider Matchless */
	case MACH_MODEL_42T:		/* Strider 25 MHz 68040 */
	case MACH_MODEL_43T:		/* Strider 33 MHz 68040 */
		first_sgc_slot=8; last_sgc_slot=11; break;
	default:
		first_sgc_slot=1; last_sgc_slot=0; break;	/* no slots */
	}

	/* ROM definition good? */
	if ((int) (((struct sti_rom *) 0) -> inq_conf) != 0x260)
		panic("sgc_init: struct sti_rom messed up");

	for (slot=0; slot<=last_sgc_slot-first_sgc_slot; slot++) {
		physical_addr = (caddr_t) ((first_sgc_slot+slot) << (32-6));
		logical_addr = dio2_map_page(physical_addr);
		if (logical_addr == NULL)
			panic("dio2_map_page failed");

		p = add_sti_device(logical_addr, slot);
		if (p==NULL)
			continue;
		if (look_for_console) {
			graphics_ite_dev = graphics_devices;
			return slot;
		}

		sti = (struct sti_rom *) p->glob_config.region_ptrs[0];
		i = sti->global_rom_rev;	/* to split into halves */
#ifndef QUIETBOOT
		printf("    %s Bit-Mapped Display (revision %d.%02d/%d) in SGC slot %d\n",
			p->desc.crt_name, i>>4, i&15, sti->local_rom_rev, slot);
#endif /* ! QUIETBOOT */
	}
	return -1;		/* didn't find a console */
}

struct graphics_descriptor *
add_sti_device(logical_addr, slot)
register caddr_t logical_addr;
{
	register caddr_t physical_addr;
	register struct graphics_descriptor *p;
	register struct sti_rom *sti;
	register region_desc *r;
	register struct pte *pte;
	register preg_t *prp;
	register int rom_size, i, *rlist;
	extern struct pte *tablewalk();
	init_flags  i_flags;
	init_inptr  i_in;
	init_outptr i_out;

	/* Do we have this one already? */
	for (p = graphics_devices; p; p = p->next)
		if (p->type == SGC_TYPE && p->isc == slot)
			return p;		/* pretend we installed it */

	sti = (struct sti_rom *) logical_addr;

	/*
	 * Due to a bug in the Trailway's SGC write pipe,
	 * do a read and ignore bus error.  This will clear the pipe.
	 */
	testr(logical_addr + 3, 1);

	/* Now see if there's an STI card there. */
	if (!testr(logical_addr + 3, 1) || sti->device_type!=1)
		return NULL;			/* failure */

	pte = tablewalk(Syssegtab, logical_addr);
	physical_addr = pte==NULL
			? logical_addr
			: (caddr_t) (pte->pg_pfnum<<PGSHIFT);
#ifdef STI_DEBUG
	printf("add_sti_device: physical_addr=%x\n", physical_addr);
#endif

	/*
	 * We need to get to the list of regions in the STI rom.
	 * Map in the entire rom to get to them.
	 */
	rom_size = get_int(sti->last_addr_of_rom)+1;
	logical_addr = dio2_map_mem(physical_addr, rom_size);
	if (logical_addr == NULL)
		panic("add_sti_device: can't allocate space");
	sti = (struct sti_rom *) logical_addr;

	if (!sti_crc_ok(sti)) {			/* How's the checksum? */
		printf("\tCRC checksum failed.\n");
		return NULL;		/* failure */
	}

	p = (struct graphics_descriptor *) calloc(sizeof(*p));
	i=0;
	rlist = (int *) ((int) sti + get_int(sti->dev_region_list)&~3);
	do {
		int zot;

		zot = get_int(rlist); rlist += 4;
		r = (region_desc *) &zot;
		/* If this is the frame buffer, remember the size */
		if (i==1)
			p->desc.crt_map_size = r->length*4096;
		prp = io_map_c((space_t) &kernvas, NULL,
			       physical_addr+r->offset*4096,
			       btorp(r->length*4096), PROT_URW,
			       r->cache
				     ? PHDL_CACHE_WT : PHDL_CACHE_CI);
		if (prp==NULL)
			panic("add_sti_device: can't map in STI region");
#ifdef STI_DEBUG
		printf("region %d: offset=%d len=%d cache=%d vaddr=%x\n",
			i, r->offset*4096, r->length*4096,
			r->cache, prp->p_vaddr);
#endif
		p->glob_config.region_ptrs[i++] = (int) prp->p_vaddr;
		
	} while (!r->last);
	dio2_unmap_mem(logical_addr, rom_size);		/* unmap the rom */
	sti = (struct sti_rom *) p->glob_config.region_ptrs[0];

	/* Copy in the graphics code */
	p->init_graph = copy_function(sti, sti->init_graph);
	p->state_mgmt = copy_function(sti, sti->state_mgmt);
	p->font_unpmv = copy_function(sti, sti->font_unpmv);
	p->block_move = copy_function(sti, sti->block_move);
	p->inq_conf   = copy_function(sti, sti->inq_conf);

	/* Initialize the card */
	p->glob_config.reent_lvl = 0;

	bzero(&i_flags, sizeof(i_flags));
	bzero(&i_in, sizeof(i_in));

	i_flags.wait = true;
	i_flags.reset = true;
	i_flags.text = true;
	i_flags.clear = true;
	i_flags.cmap_blk = true;
	i_flags.enable_be_timer = true;
	i_flags.init_cmap_tx = true;

	i_in.text_planes = 3;

	i = sti_call(p, init_graph, &i_flags, &i_in, &i_out);
	if (i==-1) {
		printf("\tDisplay FAILED, error=%d.\n", i_out.errno);
		return NULL;		/* failure */
	}
	get_sti_configuration(p);

	p->type = SGC_TYPE;		/* not DIO */
	p->isc = slot;			/* use select code for slot */
	p->g_rp_map  = NULL;		/* no map */
	p->g_rp_slot  = NULL;		/* no GCLOCK slot */
	p->desc.crt_id = get_int(sti->graphics_id);
	p->flags = ALPHA;		/* no separate frame buffer */
	p->primary = physical_addr;	/* physical address of STI region */
	p->secondary = NULL;		/* all one big happy chunk */

	/* Link us into the list of graphics devices. */
	p->next = graphics_devices;
	graphics_devices = p;
#ifdef STI_DEBUG
	printf("onscreen: %dx%d total: %dx%d\n",
		p->desc.crt_x, p->desc.crt_y,
		p->desc.crt_total_x, p->desc.crt_total_y);
	printf("bits_per_pixel: %d bits_used: %d planes: %d\n",
		p->desc.crt_bits_per_pixel, p->desc.crt_bits_used,
		p->desc.crt_planes);
	printf("dev_name: \"%s\"\n", p->desc.crt_name);
	printf("attributes: %d\n", p->desc.crt_attributes);
#endif
	return p;
}


sti_crc_ok(sti)
register struct sti_rom *sti;
{
	register unsigned char *rom, *romend;
	register short code, poly, accum;
	register int i;

	/* get size of rom from last_addr entry */
	rom = ((unsigned char *) sti) + 3;
	i = get_int(sti->last_addr_of_rom);
	romend = rom + i;

	/* calculate CRC of ROM */
	code = 0;
	poly = 0x8408;
	while (rom<romend) {
		accum = (rom[0]<<8) | rom[4];
		rom += 8;
		accum ^= code;
		for (i=16; --i>=0; ) {
			/* do a left rotate */
			if (accum<0) {
				accum <<=1;
				accum++;
				accum ^= poly;
			}
			else
				accum <<= 1;
		}
		code = accum;
	}
	return (code==0);
}



get_sti_configuration(p)
register struct graphics_descriptor *p;
{
	conf_flags  c_flags;
	conf_inptr  c_in;
	conf_outptr c_out;

	bzero(&c_flags, sizeof(c_flags));
	bzero(&c_in, sizeof(c_in));

	save_sti(p);

	c_flags.wait = true;
	sti_call(p, inq_conf, &c_flags, &c_in, &c_out);

	restore_sti(p);

	p->desc.crt_x = c_out.onscreen_x;
	p->desc.crt_y = c_out.onscreen_y;
	p->desc.crt_total_x = c_out.total_x;
	p->desc.crt_total_y = c_out.total_y;
	p->desc.crt_x_pitch = (c_out.bits_per_pixel >> 3) * c_out.total_x;
	p->desc.crt_bits_per_pixel = c_out.bits_per_pixel;
	p->desc.crt_bits_used = c_out.bits_used;
	p->desc.crt_planes = c_out.planes;
	p->desc.crt_plane_size = (c_out.bits_per_pixel >> 3)
				 * c_out.total_x * c_out.total_y;
	p->desc.crt_attributes = c_out.attributes;
	strcpy(p->desc.crt_name, c_out.dev_name);
}

function
copy_function(sti, addr)
struct sti_rom *sti;
int *addr;
{
	register int i, size, *start_addr;
	register caddr_t code;

	start_addr = (int *) ((get_int(addr) & ~03) + (char *) sti);
	size = (get_int(addr+4) - get_int(addr))/4 ;
	code = kmem_alloc(size);
	if (code==NULL)
		panic("copy_function: can't alloc space for SGC device");
	for (i=0; i<size; i++)
		code[i] = *start_addr++;
	purge_icache();			/* Purge Instruction cache */
	purge_dcache_s();		/* Purge Supervisor data cache */
	return (function) code;		/* Pointer to the new code */
}

get_int(addr)
register int *addr;
{
	register int a,b,c,d;
	a = *addr++ & 0xff;
	b = *addr++ & 0xff;
	c = *addr++ & 0xff;
	d = *addr++ & 0xff;

	return (a<<24) + (b<<16) + (c<<8) + d;
}

/*
 *  By convention there can only be a maximum of two graphics
 *  devices in internal io space.  graphics_link (see below)
 *  enforces this convention.
 *
 *  This routine assumes that if there are two graphics 
 *  devices in internal io space then:
 *
 *	1) High resolution devices (9837H, GATORBOX)
 *	   must be at io address 0x560000 (select code EXTERNALISC+22)
 *	2) Low resolution devices (9836[AC], 9826A, Marbox) are
 *	   at io address 0x530000 (select code EXTERNALISC+19)
 *
 *  The internal graphics devices (two max) are auto-configured by 
 *  the open routine.  The low resolution graphics device is configured
 *  to minor number type 0 (if present).  The high resolution device
 *  is configured to minor number type 1 if there is an internal low
 *  resolution device, and to minor number type 0 if there isnt.  
 */
struct graphics_descriptor *
find_graphics_dev(dev)
dev_t dev;
{
	struct graphics_descriptor *p, *device = NULL;
	register struct graphics_descriptor *low 	= NULL;
	register struct graphics_descriptor *high 	= NULL;

	switch (GRAPH_TYPE(dev)) {
	case LOW_TYPE:
	case HIGH_TYPE:
		/*
		** Find the internal graphics devices (max of two, one high
		** resolution and one low resolution)
		*/
		for (p = graphics_devices; p; p = p->next) {
			if (p->isc == EXTERNALISC+19)
				low = p;
			else if (p->isc == EXTERNALISC+22)
				high = p;
		}
		switch (GRAPH_TYPE(dev)) {
		case LOW_TYPE:
			if (low) {
				low->flags |= DEFAULT;
				device = low;
				break;
			}
			if (high) {
				high->flags |= DEFAULT;
				device = high;
				break;
			}
			break;
		case HIGH_TYPE:
			if (low && high) {
				device = high;
				break;
			}
			break;
		}
		break;

	case DIO_TYPE:	/* DIO I & II */
	case SGC_TYPE: /* SGC */
		for (p = graphics_devices; p; p = p->next) {

			/* exclude internal 98546A/98204B type device */
			if (p->isc == EXTERNALISC+19) continue;

			/* exclude internal ITE device */
			if (p->isc == EXTERNALISC+22) continue;

			if (p->isc == GRAPH_SC(dev) &&
			    p->type == GRAPH_TYPE(dev)) {
				device = p;
				break;
			}
		}
		break;

	default:
		device = NULL;
	}
	return device;
}
