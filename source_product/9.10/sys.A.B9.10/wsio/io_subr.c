/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/io_subr.c,v $
 * $Revision: 1.7.83.5 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/22 10:40:01 $
 */

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/malloc.h"
#include "../wsio/eisa.h"
#include "../wsio/eeprom.h"
#ifdef __hp9000s300
#include "../s200io/vme2.h"
#else
#include "../wsio/vme2.h"
#endif
#include "../h/io.h"
#include "../h/buf.h"
#include "../h/dk.h"
#if defined (__hp9000s800) && defined (_WSIO)
#include "../machine/pa_cache.h"
#endif

extern preg_t *io_map();

#if defined(__hp9000s800) && defined(_WSIO)
extern caddr_t ltor();
extern caddr_t vme_map_mem_to_bus();
extern caddr_t vme_unmap_mem_from_bus();
#else
extern caddr_t vtop();
#endif
extern bcopy();

ushort eisa_int_enable = 0xfffb;
static struct rsrc_node *hold_entry = (struct rsrc_node *)0;

#ifdef IO_TEST
/* 
 * io testing code and data
 */

static int io_test_iomap_deprivation = 0;

/*------------------------------------------------------------------------*/
set_io_test(test, arg)
/*------------------------------------------------------------------------*/

int test;
int arg;
{
	switch (test) {
		case 1: /* set iomap deprivation to true */
			io_test_iomap_deprivation = 1;
			break;
		case 2: /* set iomap deprivation to false */
			io_test_iomap_deprivation = 0;
			break;
	}
}
#endif


/*------------------------------------------------------------------------*/
caddr_t io_malloc(size, flags)
/*------------------------------------------------------------------------*/
/* This subroutine performs the memory allocation function for the I/O
 * system. 
 *
 * Entry:
 *     size  - the amount of requested memory in bytes.
 *     flags - the following two flags are supported:
 *        IOM_WAITOK - the process will sleep until memory available
 *        IOM_NOWAIT - the process will fail if no memory available
 *
 * Returns:
 *	>0 - successful, return value is the address
 *       0 - failed.
 *
 * Call on Interrupt Stack: Yes, using IOM_NOWAIT.
 */

int size;
int flags;
{
	caddr_t ptr;

	if (flags & IOM_WAITOK) {
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		ptr = (caddr_t) kmalloc(size, M_IOSYS, M_WAITOK);
	} else if (flags & IOM_NOWAIT) {
		ptr = (caddr_t) kmalloc(size, M_IOSYS, M_NOWAIT);
	} else
		return(NULL);

	return(ptr);
}


/*------------------------------------------------------------------------*/
io_free(ptr, size)
/*------------------------------------------------------------------------*/
/* This subroutine frees memory previously allocated with io_malloc().
 *
 * Entry:
 *     ptr  - the pointer obtained by io_malloc().
 *     size - the size of the block requested during that call.
 *
 * Returns:
 *	>0 - always successful
 *
 * Call on Interrupt Stack: Yes
 */

caddr_t ptr;
int size;
{
	FREE(ptr, M_IOSYS);
	return(1);
}


/*------------------------------------------------------------------------*/
io_testr(isc, virt_addr, width)
/*------------------------------------------------------------------------*/
/* This subroutine tests for the presence of a device acknowledging a
 * width wide read access to virt_addr.
 *
 * Entry:
 *     isc       - info including the expansion bus the device is on.
 *     virt_addr - the virtual address obtained from map_mem_to_host().
 *     width     - the following widths are supported:
 *         BYTE_WIDE  - a byte access will be made.
 *         SHORT_WIDE - a short integer access will be made.
 *         LONG_WIDE  - a long access will be made.
 *
 * Returns:
 *      -1 - usage error. NOTE: address must be evenly divisible by width.
 *       0 - access resulted in an expansion bus error.
 *	>0 - access acknowledged by expansion bus device.
 *
 * Call on Interrupt Stack: Yes
 */

struct isc_table_type *isc;
caddr_t virt_addr;
int width;
{
	unsigned char byte;
	unsigned short sh;
	unsigned long l;
	int ret;

	/*
	 * this routine is supported for VME only at this time.
	 */
	if (isc->bus_type != VME_BUS) return(-1);
	if ((int)virt_addr % width) return(-1);
#ifdef __hp9000s300
	switch (width) {
		case BYTE_WIDE: ret = bcopy_prot(virt_addr, &byte, 1); break;
		case SHORT_WIDE: ret = scopy_prot(virt_addr, &sh, 1); break;
		case LONG_WIDE: ret = lcopy_prot(virt_addr, &l, 1); break;
		default: return(-1);
	}
	return(ret);	
#else
	return(vme_testr(isc, virt_addr, width));
#endif
}


/*------------------------------------------------------------------------*/
io_testw(isc, virt_addr, width)
/*------------------------------------------------------------------------*/
/* This subroutine tests for the presence of a device acknowledging a
 * width wide write access to virt_addr.
 *
 * Entry:
 *     isc       - info including the expansion bus the device is on.
 *     virt_addr - the virtual address obtained from map_mem_to_host().
 *     width     - the following widths are supported:
 *         BYTE_WIDE  - a byte access will be made.
 *         SHORT_WIDE - a short integer access will be made.
 *         LONG_WIDE  - a long access will be made.
 *
 * Returns:
 *      -1 - usage error. NOTE: address must be evenly divisible by width.
 *       0 - access resulted in an expansion bus error.
 *	>0 - access acknowledged by expansion bus device.
 *
 * Call on Interrupt Stack: Yes
 */

struct isc_table_type *isc;
caddr_t virt_addr;
int width;
{
	unsigned char byte = 0xff;
	unsigned short sh = 0xffff;
	unsigned long l = 0xffffffff;
	int ret;

	/*
	 * this routine is supported for VME only at this time.
	 */
	if (isc->bus_type != VME_BUS) return(-1);
	if ((int)virt_addr % width) return(-1);

#ifdef __hp9000s300
	switch (width) {
		case BYTE_WIDE: ret = bcopy_prot(&byte, virt_addr, 1); break;
		case SHORT_WIDE: ret = scopy_prot(&sh, virt_addr, 1); break;
		case LONG_WIDE: ret = lcopy_prot(&l, virt_addr, 1); break;
		default: return(-1);
	}
	return(ret);	
#else
	return(vme_testw(isc, virt_addr, width));	
#endif
}


/*------------------------------------------------------------------------*/
io_flushcache(virtual_addr, size)
/*------------------------------------------------------------------------*/
/* This subroutine will flush the cache lines associated with the block
 * described by virtual_addr and size.
 *
 * Entry:
 *     virtual_addr - the address of the block to flush.
 *     size  - the amount of memory to flush in bytes.
 *
 * Returns:
 *	>0 - always successful
 *
 * Call on Interrupt Stack: Yes, if virtual_addr is not allocated on
 *      the stack.
 */

caddr_t virtual_addr;
int size;
{
#if defined(__hp9000s800) && defined(_WSIO)
	fdcache_conditionally(KERNELSPACE, virtual_addr, size, c_always);
	SYNC();
#else
	purge_dcache_physical();
#endif
	return(1);
}


/*------------------------------------------------------------------------*/
io_purgecache(virtual_addr, size)
/*------------------------------------------------------------------------*/
/* This subroutine will purge the cache lines associated with the block
 * described by virtual_addr and size.
 *
 * Entry:
 *     virtual_addr - the address of the block to flush.
 *     size  - the amount of memory to flush in bytes.
 *
 * Returns:
 *	>0 - always successful
 *
 * Call on Interrupt Stack: Yes, if virtual_addr is not allocated on
 *      the stack.
 */

caddr_t virtual_addr;
int size;
{
#if defined(__hp9000s800) && defined(_WSIO)
	pdcache_conditionally(KERNELSPACE, virtual_addr, size, c_always);
	SYNC();
#else
	purge_dcache_physical();
#endif
	return(1);
}


/*------------------------------------------------------------------------*/
io_dma_setup(isc, dma_parms)
/*------------------------------------------------------------------------*/
/* This subroutine performs the required setup and returns the neccessary 
 * information for a DMA transfer. 
 *
 * Entry:
 *     isc       - generic information.
 *     dma_parms - general and bus specific DMA description
 *
 * Returns:
 *	dma_parms - contains address/count pairs.
 *	0  - successful, return value is the address
 *      <0 - failure, one of the following errors:
 *          UNSUPPORTED_FLAG 
 *          RESOURCE_UNAVAILABLE 
 *          BUF_ALIGN_ERROR     
 *          MEMORY_ALLOC_FAILED
 *          TRANSFER_SIZE     
 *          INVALID_OPTIONS_FLAGS 
 *          ILLEGAL_CHANNEL      
 *          NULL_CHAINPTR       
 *          BUFLET_ALLOC_FAILED
 *          -10                   - usage error.
 *
 * Call on Interrupt Stack: Yes, but why?
 */

struct isc_table_type *isc;
struct dma_parms *dma_parms;
{
	if (isc->bus_type == VME_BUS) 
		return(vme_dma_setup(isc, dma_parms));
#ifdef NOT_IN_9_0
	if (isc->bus_type == EISA_BUS)
		return(eisa_dma_setup(isc, dma_parms));
	if (isc->bus_type == CORE_BUS)
		return(core_dma_build_chain(dma_parms));
#endif
	return(-10);
}


/*------------------------------------------------------------------------*/
io_dma_cleanup(isc, dma_parms)
/*------------------------------------------------------------------------*/
/* This subroutine performs the required cleanup to recover from a DMA
 * transfer. 
 *
 * Entry:
 *     isc       - generic information.
 *     dma_parms - the same structure obtained from io_dma_setup()
 *
 * Returns:
 *     >0 - success, the return value is the residual count for slaves.
 *      0 - success for masters
 *     <0  - failure, one of the following errors:
 *          -1                    - resid error for slaves
 *          ILLEGAL_CHANNEL      
 *          -10                   - usage error.
 *
 * Call on Interrupt Stack: Yes, but only if DMA is not to an address 
 *     allocated on the stack (700).
 */

struct isc_table_type *isc;
struct dma_parms *dma_parms;
{
	if (isc->bus_type == VME_BUS) 
		return(vme_dma_cleanup(isc, dma_parms));
#ifdef NOT_IN_9_0
	if (isc->bus_type == EISA_BUS)
		return(eisa_dma_cleanup(isc, dma_parms));
	if (isc->bus_type == CORE_BUS)
		return(core_dma_cleanup(dma_parms));
#endif
	return(-10);
}


/*----------------------------------------------------------------------*/
int io_isrlink(isc, isr_addr, level_vector, arg, sw_trig_lvl)
/*----------------------------------------------------------------------*/
/* This subroutine will link an isr into the internal interrupt handlers
 * and enable the CPU side of interrupts to work for this level_vector.
 * It should be called with the cards interrupts disabled.
 *
 * Entry:
 *     isc - general info.
 *     isr_addr     - the address of the interrupt service routine.
 *     level_vector - the level or vector the isr is associated with.
 *     arg - an optional argument to be passed into the isr when invoked.
 *     sw_trig_lvl - the spl level this routine will be executed at.
 *
 * Returns:
 *      0 - success
 *     <0 - usage failure.
 *
 * Call on Interrupt Stack: No.
 */

struct isc_table_type *isc;
caddr_t isr_addr;
int level_vector;
int arg;
int sw_trig_lvl;
{
        if (isc->bus_type == VME_BUS)
                return(vme_isrlink(isc, isr_addr, level_vector, arg, sw_trig_lvl));
        return(-1);
}


/*------------------------------------------------------------------------*/
io_isrunlink(isc, level_vector)
/*------------------------------------------------------------------------*/
/* This subroutine will unlink an isr linked by io_isrlink() from the
 * internal interrupt structures. This routine should be called before
 * a driver is unloaded but after the cards interrupts are disabled.
 *
 * Entry:
 *     isc          - generic information.
 *     level_vector - the same level_vector used when the routine was linked.
 *
 * Returns:
 *      0 - success
 *     <0 - usage failure.
 *
 * Call on Interrupt Stack: No.
 */

struct isc_table_type *isc;
int level_vector;
{
#ifdef __hp9000s300
	return(0);
#else
        if (isc->bus_type == VME_BUS)
                return(vme_isrunlink(isc, level_vector));
        return(-1);
#endif

}


/*------------------------------------------------------------------------*/
eisa_isrlink(isc, isr, irq, arg)
/*------------------------------------------------------------------------*/
/* Link a driver's interrupt service routine into the interrrupt table.   */

struct isc_table_type *isc;
int (*isr)();
int irq;
caddr_t arg;
{
	register struct irq_isr **p, *q;
	
#if defined(__hp9000s800) && defined(_WSIO)
	if (isc->bus_type == PA_CORE)
		asp_isrlink(isr, irq, isc, (int)arg);
	else 
#endif
	if (isc->bus_type == EISA_BUS) {
		/* eisa ints are 0-15, 0,1,2, 8, and 13 are unusable by drivers */
		if (irq < 3 || irq > 15 || irq == 8 || irq == 13) return(-1);

		if (isr == NULL) return(-1);
	
#if defined(LEVEL) || defined(EISA_TESTS_SELFISH)
		if (irq >= 3 && irq <= 7) 
			((struct eisa_bus_info *)
				(isc->bus_info->spec_bus_info))->esb->int1_edge_level |= (1 << irq);
		else
			((struct eisa_bus_info *)
				(isc->bus_info->spec_bus_info))->esb->int2_edge_level 
				|= 1 << (irq - 8);
#endif

		/* add this isrlink entry to list of isr's for this IRQ */
		p = &eisa_rupttable[irq];
		while ((q = *p) != NULL)
			p = &q->next;
		
		q = (struct irq_isr *)kmem_alloc(sizeof(struct irq_isr));
		q->next = 0;  q->isc = isc;  q->arg = arg;  q->isr = isr;
		*p = q;
	
		/* add this interrupt to interrupts to enable, this catches ISA cards */
		eisa_int_enable &= ~(1 << irq);

		return(0);
	
	} else {
		/*  bus type not PACORE or not EISA is not accepted */
		return(-1);
	}

#if defined(__hp9000s800) && defined(_WSIO)
	return(0);
#endif
}


/*------------------------------------------------------------------------*/
struct isc_table_type *get_new_isc(old_isc)
/*------------------------------------------------------------------------*/
/* This subroutine will create a new isc entry and link it to the old
 * entry. Its use is intended for multifunction cards.
 * The following new isc fields are duplicated ptrs:
 *     if_reg_ptr
 *     ifsw
 *     if_drv_data
 *
 * If the gfsw field was non null, a new one was allocated and the old 
 *     data copied into it.
 *
 * ftn_no gets set to -1.
 *
 * Entry:
 *     isc - the previous function in the chain.
 *
 * Returns:
 *     >0 - success, the return value is the address of the new isc
 *      0 - memory allocation failure
 *
 * Call on Interrupt Stack: Yes.
 */

struct isc_table_type *old_isc;
{
	struct isc_table_type *new_isc;

	/* get some allocation */
	if ((new_isc = (struct isc_table_type *)
	    io_malloc(sizeof(struct isc_table_type), IOM_NOWAIT)) == NULL)
	    return(NULL);

	/* clean it up */
	bzero(new_isc, sizeof(struct isc_table_type));

	/* bus_type is an integer, so copy it */
	new_isc->bus_type = old_isc->bus_type;

	/* my_isc is an integer, so copy it */
	new_isc->my_isc = old_isc->my_isc;

	/* if_reg_ptr is a (type casted)caddr_t, just copy the ptr */
	new_isc->if_reg_ptr = old_isc->if_reg_ptr;

	/* if_id is an integer, so copy it */
	new_isc->if_id = old_isc->if_id;

	/* bus_info is a system structure and so should just copy ptr */
	new_isc->bus_info = old_isc->bus_info;

	/* caller should set this */
	new_isc->ftn_no = -1;

	/* 
	 * this routine is for building chains for multi function cards. 
	 * so I will do the link
	 */
	old_isc->next_ftn = new_isc;
	new_isc->next_ftn = NULL;

	/* make copy of if_info structure */
	if (old_isc->bus_type == EISA_BUS) {
		new_isc->if_info = io_malloc(sizeof(struct eisa_if_info), IOM_NOWAIT);
		bcopy(old_isc->if_info, new_isc->if_info, sizeof(struct eisa_if_info));
	} else if (old_isc->bus_type == VME_BUS) {
		new_isc->if_info = io_malloc(sizeof(struct vme_if_info), IOM_NOWAIT);
		bcopy(old_isc->if_info, new_isc->if_info, sizeof(struct vme_if_info));
	} else
		new_isc->if_info = NULL;

	/* if gfsw exists, make a copy */
	if (old_isc->gfsw) {
		new_isc->gfsw = (struct gfsw *)
		   io_malloc(sizeof(struct gfsw), IOM_NOWAIT);
		bcopy(old_isc->gfsw, new_isc->gfsw, sizeof(struct gfsw));
	}

	/* ifsw is a typecasted caddr_t, so just copy ptr */
	new_isc->ifsw = old_isc->ifsw;
	
	/* if_drv_data is a typecasted caddr_t, so just copy ptr */
	new_isc->if_drv_data = old_isc->if_drv_data;

	return(new_isc);
}


/*------------------------------------------------------------------------*/
free_isc(isc)
/*------------------------------------------------------------------------*/
/* This subroutine will free an isc entry obtained from core services,
 * eisa services, or from the routines get_new_isc() or vme_init_isc().
 * The isc should be freed on error, or before a driver is unloaded.
 *
 * Entry:
 *     isc - the entry to be freed.
 *
 * Returns:
 *      0 - success
 *     <0 - usage error.
 *
 * Call on Interrupt Stack: Yes.
 */

struct isc_table_type *isc;
{
	struct isc_table_type *ip, *op;

	/* first we remove this isc from a possible function chain */
	ip = op = isc_table[isc->my_isc];
	while (ip) {
		if (ip == isc) {
			/* found it */
			if (ip == op) {
				/* head of list */
				isc_table[isc->my_isc] = isc->next_ftn;			
				break;
			} else {
				/* middle or end */
				op->next_ftn = ip->next_ftn;
				break;
			}
		}
		op = ip;
		ip = ip->next_ftn;
	}

	if (ip == NULL) 
		return(-1);

	/* free the if_info structure */
	if (ip->if_info) {
		if (ip->bus_type == EISA_BUS) {
			    io_free((caddr_t)ip->if_info, sizeof(struct eisa_if_info));
		} else if (ip->bus_type == VME_BUS) {
			    io_free((caddr_t)ip->if_info, sizeof(struct vme_if_info));
		} else
			return(-1);
	}

	/* if gfsw exists, remove it */
	if (ip->gfsw) 
		   io_free((caddr_t)ip->gfsw, sizeof(struct gfsw));

	/* get rid of whole isc */
	io_free((caddr_t)isc, sizeof(struct isc_table_type));

	return(0);
}


/*-------------------------------------------------------------------------*/
caddr_t kvtophys(virtual_addr)
/*-------------------------------------------------------------------------*/
/* this subroutine translates a kernel virtual kernel address into a 
 * physical address.
 *
 * Entry:
 *     virtual_addr - the address.
 *
 * Returns:
 *     <0 - error
 *     >0 - physical address.
 *
 * Call on Interrupt Stack: Yes.
 */

caddr_t virtual_addr;
{
#if defined (__hp9000s800) && defined (_WSIO)
	return(ltor(KERNELSPACE, virtual_addr));
#else
	return(vtop(virtual_addr));
#endif
}


/*----------------------------------------------------------------------------*/
get_rsrc_entries(rsrc_hdr, dir, num)
/*----------------------------------------------------------------------------*/
/* This subroutine returns the first of a consecutive string of size num
 * of available  entries of the resource pointed to by rsrc_hdr.  
 * 
 * Return: 
 *      >=0 - number of first entry
 *      -1  - num consecutive entries cannot be reserved.
 *
 * Call on Interrupt Stack: Yes
 */

struct rsrc_node *rsrc_hdr; 
int dir;
int num;
{
	struct rsrc_node *p;
	int ent_num;
	int spl;

	IOASSERT(rsrc_hdr != NULL);
#ifdef IO_TEST
	if (io_test_iomap_deprivation)
		return(-1);
#endif

	spl = spl6();
	if (dir)
		p = rsrc_hdr->prev;
	else
		p = rsrc_hdr->next;

	while (p != rsrc_hdr) {
		if (num < p->size) {
			p->size -= num;
			if (dir) {
				ent_num = p->entry_number + p->size;
			} else {
				ent_num = p->entry_number;
				p->entry_number += num;
			}
			splx(spl);
			return(ent_num);
		}

		if (num == p->size) {
			ent_num = p->entry_number;
			p->prev->next = p->next;
			p->next->prev = p->prev;
			if (hold_entry) {
				FREE(p, M_DMA);
			} else {
				hold_entry = p;
			}
			splx(spl);
			return(ent_num);
		}

		if (dir)
			p = p->prev;
		else
			p = p->next;
	}
	splx(spl);
	return(-1);
}
 

/*----------------------------------------------------------------------------*/
free_rsrc_entries(rsrc_hdr, ent_num, num)
/*----------------------------------------------------------------------------*/
/* This routine will free num number of entries beginning at ent_num of the
 * resource pointed to by rsrc_hdr.
 *
 * Returns:
 *	0 - if resource map was successfully updated
 *	Guaranteed not to fail.
 *
 * Call on Interrupt Stack: Yes
 */

struct rsrc_node *rsrc_hdr; 
int ent_num;
int num;
{
	struct rsrc_node *p, *node;
	int spl;

	IOASSERT(rsrc_hdr != NULL);

	spl = spl6();
	p = rsrc_hdr;
	while (p->next != rsrc_hdr) {

		IOASSERT((p->next->entry_number > ent_num || 
			ent_num >= p->next->entry_number + p->next->size)); 

		if (p->next->entry_number + p->next->size == ent_num) {
			p->next->size += num;

			/* list coalesce */
			node = p->next->next;
			if (node != rsrc_hdr) {
				if (p->next->entry_number + p->next->size == 
					node->entry_number) {
					p->next->size += node->size;
					node->next->prev = p->next;
					p->next->next = node->next;
					if (hold_entry) {
						FREE(node, M_DMA);
					} else {
						hold_entry = node;
					}
				}
			}
			splx(spl);
			return(0);
		}

		IOASSERT((ent_num >= p->next->entry_number ||
			p->next->entry_number >= ent_num + num));

		if (ent_num + num == p->next->entry_number) {
			p->next->entry_number = ent_num;
			p->next->size += num;
			splx(spl);
			return(0);
		}

		if (ent_num < p->next->entry_number) break;
		p = p->next;
	}

	if (hold_entry) {
		node = hold_entry;
		hold_entry = (struct rsrc_node *)0;
	} else {
		MALLOC(node, struct rsrc_node *, sizeof(struct rsrc_node), 
			M_DMA, M_NOWAIT);

		if (node == NULL) { 
			panic("free_rsrc_entry: Cannot get memory for rsrc list");
		}
	}

	node->entry_number = ent_num;
	node->size = num;
	node->prev = p;
	node->next = p->next;

	p->next = node;
	node->next->prev = node;
	splx(spl);
	return(0);
}


/*----------------------------------------------------------------------------*/
put_on_wait_list(isc, drv_routine, drv_arg)
/*----------------------------------------------------------------------------*/
/* This  routine  will place the address of and a  parameter  for a call
 * back routine onto the (currently) VME iomap waiting list.
 *
 * Return:
 *	0 - successful
 *	MEMORY_ALLOC_FAILED - no memory for waiting list element
 *
 * Call on Interrupt Stack: Yes
 */

struct isc_table_type *isc;
int (*drv_routine)();
int drv_arg;
{
	struct driver_to_call *iomap_list;
	int spl;

	IOASSERT(isc->bus_info != NULL);

	spl = spl6(); /* linked list manipulation is a critical section */
	iomap_list = isc->bus_info->iomap_wait_list;

	MALLOC(isc->bus_info->iomap_wait_list, struct driver_to_call *, 
		sizeof(struct driver_to_call), M_IOSYS, M_NOWAIT);

	if (isc->bus_info->iomap_wait_list == NULL) {
		isc->bus_info->iomap_wait_list = iomap_list;
		splx(spl);
		return(MEMORY_ALLOC_FAILED);
	}

	isc->bus_info->iomap_wait_list->next = iomap_list;
	isc->bus_info->iomap_wait_list->drv_routine = drv_routine;
	isc->bus_info->iomap_wait_list->drv_arg = drv_arg;
	isc->bus_info->iomap_wait_list->isc = isc;
	splx(spl);
	return(0);
}


/*----------------------------------------------------------------------------*/
free_wait_list(isc)
/*----------------------------------------------------------------------------*/
/* This  routine will call all driver wait routines on the waiting list. 
 *
 * Return:
 * 	Always successful.
 *
 * Call on Interrupt Stack: Yes
 */

struct isc_table_type *isc;
{
	int spl;
	struct driver_to_call *this_entry, *previous;

	spl = spl6();
        /*
	 * since we are about to empty the  waiting  list,  set the main
	 * pointer to null
         */
	 this_entry = isc->bus_info->iomap_wait_list;
         isc->bus_info->iomap_wait_list = NULL;

        /*
	 * this will  empty  the  waiting  list.  In each  case a "user"
	 * supplied  routine will be called  (normally a wakeup) and the
	 * drivers are then free to re-request  their  desired  entries.
	 * This in turn may or may not end up putting  some of them back
	 * onto the waiting list.
         */

        while (this_entry != NULL) {
                (*this_entry->drv_routine)(this_entry->drv_arg);
                previous = this_entry;
                this_entry = this_entry->next;
                FREE(previous, M_IOSYS);
        }

	splx(spl);
}


/*----------------------------------------------------------------------------*/
remove_wait_list(isc)
/*----------------------------------------------------------------------------*/
/* This  routine  will remove a driver's wait routine from the current
 * waiting list.
 *
 * Return:
 *      0 - whether driver on it or not.
 *
 * Call on Interrupt Stack: Yes.
 */

struct isc_table_type *isc;
{
	int spl;
	struct driver_to_call *this_entry, *previous;

	spl = spl6();
	this_entry = isc->bus_info->iomap_wait_list;

	while (this_entry != NULL && this_entry->isc != isc) {
		previous = this_entry;
		this_entry = this_entry->next;
	}

	if (this_entry != NULL) { /* implies we found him */
		if (this_entry == isc->bus_info->iomap_wait_list) { 
			/* it was at head of list */
			isc->bus_info->iomap_wait_list = this_entry->next;
		} else { 
			/* remove it from the list */
			previous->next = this_entry->next;
		}

		/* give back the space it was using */
		FREE(this_entry, M_IOSYS);
	} 

	splx(spl);
	return(0);
}

#define SHORT_TERM_ENTRY 0
#define LONG_TERM_ENTRY  1

/*-----------------------------------------------------------------------*/
lock_iomap_entries(isc, num_entries, dma_parms)
/*-----------------------------------------------------------------------*/
/* This routine attempts to allocate num_entries worth of contiguous
 * iomap entries.
 *
 * Return:
 *     0 - satisfied request
 *    -1 - could not satisfy request.
 *
 * Call on Interrupt Stack: Yes.
 */

struct isc_table_type *isc;
int num_entries;
struct dma_parms *dma_parms;
{
	struct rsrc_node *iomap_rsrc; 

	/*
	 * if no iomap rsrc map, then return error
	 */
	if ((iomap_rsrc = isc->bus_info->iomap_rsrcmap) == NULL) 
		return(-1);

	/*
	 * attempt to get request. return -1 if failed
	 */
	if ((dma_parms->key = get_rsrc_entries(iomap_rsrc, SHORT_TERM_ENTRY, num_entries)) != -1) {
		dma_parms->num_entries = num_entries;
		return(0); /* we satisfied the request */
	} else  
		return(-1); /* we failed for some reason */
}


/*-----------------------------------------------------------------------*/
free_iomap_entries(isc, dma_parms)
/*-----------------------------------------------------------------------*/
/* This routine either free's up prevouusly  allocated  iomap entries or
 * removes a driver's  wait routine from the waiting list.  If prevously
 * allocated  entries  were  freed,  it will  call  all of the  wake  up
 * routines on the waiting list.
 *
 * Return:
 *      0 - successful
 *     -1 - memory could not be allocated for resource map manipulation.
 *
 * Call on Interrupt Stack: Yes.
 */

struct isc_table_type *isc;
struct dma_parms *dma_parms;

{

	int ret = 0;

	IOASSERT(isc->bus_info->iomap_rsrcmap != NULL); 

	/* 
	 * if  dma_parms->key  == -1 then the  caller  does  not own any
	 * entries  but is on the  waiting  list.  A free  in this  case
	 * indicates he wants to be removed from the waiting list.
	 */
	if (dma_parms->key == -1) { 
		return(remove_wait_list(isc));
	}
			
	/*
	 * give back the entries which were actually previously obtained.
	 */
	ret=free_rsrc_entries(isc->bus_info->iomap_rsrcmap,dma_parms->key,dma_parms->num_entries);

	/*
	 * call back any driver on wait list
	 */
	free_wait_list(isc);

	dma_parms->key = -1;
	return(ret);
}


/*-----------------------------------------------------------------------*/
caddr_t map_mem_to_bus(isc, io_parms)
/*-----------------------------------------------------------------------*/
/* this routine  attempts to grab iomap  entries, sets up the iomap with
 * host physical address and returns a pointer to the iomap.  Its intent
 * is for non-dma card  accesses  into host  memory.  An example is SCSI
 * controller   scripts.  This   routine  does  not  handle  host  cache
 * flushing.
 *
 * Return:
 *      >0 : Bus physical address of first acquired entry.
 *      =0 : No iomap_rsrcmap available.
 *      MEMORY_ALLOC_FAILED - memory alloc of waiting list elements failed
 *      RESOURCE_UNAVAILABLE - failed to attain requested iomap entries
 *
 * Call on Interrupt Stack: Yes
 */

struct isc_table_type *isc;
struct io_parms *io_parms;
{
	int num_pages, count, bytes_this_page, this_iomap, spl;
	unsigned int phys_addr;
	int *host_iomap_base = isc->bus_info->host_iomap_base;
	struct rsrc_node *iomap_rsrc; 
	unsigned int host_addr = (unsigned int)io_parms->host_addr;

#if defined (__hp9000s800) && defined (_WSIO)
	if (isc->bus_type == VME_BUS) {
		return(vme_map_mem_to_bus(isc, io_parms));
	} else 
#endif
	if (isc->bus_type != EISA_BUS) {
		return((caddr_t)0);
	}

	/*
	 * if no iomap rsrc map, then return error
	 */
	if ((iomap_rsrc = isc->bus_info->iomap_rsrcmap) == NULL) 
		return((caddr_t)0);

	/* calculate number of pages */
	num_pages = ((host_addr & PGOFSET) + io_parms->size + NBPG - 1) >> PGSHIFT;

	/*
	 * attempt to get request. 
	 */
	spl = spl6(); /* if get fails, must be on wakeup list before a free occurs */
	if ((io_parms->key = get_rsrc_entries(iomap_rsrc, LONG_TERM_ENTRY, num_pages)) < 0) {
		if (io_parms->drv_routine != NULL) {
			if (put_on_wait_list(isc, io_parms->drv_routine, io_parms->drv_arg) < 0) {
				splx(spl);
				return((caddr_t)MEMORY_ALLOC_FAILED);
			}
		}
		splx(spl);
		return((caddr_t)RESOURCE_UNAVAILABLE);
	} 
	splx(spl);

	io_parms->num_entries = num_pages;

	/* 
	 * load up the iomap entries with the actual physical addresses 
	 * corresponding to the requested host address
	 */
	this_iomap = io_parms->key;
	count = io_parms->size;
	while (count) {
		phys_addr = (int)kvtophys(io_parms->host_addr);

		bytes_this_page = NBPG - (host_addr & PGOFSET);
		if (bytes_this_page > count)
			bytes_this_page = count;
		host_addr += bytes_this_page;
		count -= bytes_this_page;
		
		if (isc->bus_type == EISA_BUS) {
			(*(unsigned int *)((unsigned int)host_iomap_base + (this_iomap*NBPG))) 
				= phys_addr>>PGSHIFT;
		} else if (isc->bus_type == VME_BUS) 
			(*(unsigned int *)((unsigned int)host_iomap_base + (this_iomap*4))) 
				= phys_addr>>PGSHIFT;

		/* we are going backwards based on LONG_TERM_ENTRY above */
		this_iomap--;
	}


	/* 
	 * return the specific bus address 
	 */

	return((caddr_t)
		/* calculate the page number */
		((int)isc->bus_info->bus_iomap_base + (io_parms->key*NBPG)
		/* calculate the offset into the page */
		+ ((int)io_parms->host_addr & PGOFSET)));
}


/*-----------------------------------------------------------------------*/
caddr_t unmap_mem_from_bus(isc, io_parms)
/*-----------------------------------------------------------------------*/
/* this routine frees up the entries obtained with map_mem_to_bus().  It
 * will ultimately invoke driver specific wait routines from the waiting
 * list if they were initially given.
 * 
 * Return:
 *	0 - successful
 *     -1 - memory could not be allocated for resource map manipulation.
 *
 * Call on Interrupt Stack: Yes.
 */

struct isc_table_type *isc;
struct io_parms *io_parms;
{
	struct dma_parms dma_parms;


#if defined (__hp9000s800) && defined (_WSIO)
	if (isc->bus_type == VME_BUS) {
		return(vme_unmap_mem_from_bus(isc, io_parms));
	} else 
#endif
	if (isc->bus_type != EISA_BUS)
		return((caddr_t) -1);

	dma_parms.key = io_parms->key;
	dma_parms.num_entries = io_parms->num_entries;
	dma_parms.drv_routine = io_parms->drv_routine;
	dma_parms.drv_arg = io_parms->drv_arg;
	return((caddr_t)free_iomap_entries(isc, &dma_parms));
}


/*-----------------------------------------------------------------------*/
caddr_t host_map_mem(phys_addr, size)
/*-----------------------------------------------------------------------*/
/* this routine provides a reasonable physical to virtual memory mapping
 * mechanism.  It hides all of the 3/400, 700 differences, p regions etc
 * and takes as  parameters,  those  things  only those  things  someone
 * should need to know about. It returns a host virtual address.
 */

caddr_t phys_addr;   /* host physical address */
int size;            /* number of bytes */
{
	preg_t *prp;
	caddr_t base_addr;

	/* 
	 * io_map() addresses must be page-aligned and in page increments 
	 */
	base_addr = (caddr_t)((unsigned long)phys_addr & ~(NBPG - 1));

	/*
	 * Series 3/400 must use PROT_URW even though this is kernel space
	 * (Drew told me this).  We avoid problems by having the kernel
	 * use different virtual addresses than user space.
	 *
	 * Normally, you would expect s700 to use PROT_KRW, because if you
	 * used PROT_URW, user processes could access this space.  However,
	 * we use protection ids to get around this.
	 */
#ifdef __hp9000s300
        if ((prp = io_map((space_t)KERNVAS, NULL, base_addr,
                          pagesinrange(phys_addr, size), PROT_URW)) == NULL)
#else
	if ((prp = io_map((space_t)KERNVAS, KERNELSPACE, NULL, base_addr,
			  pagesinrange(phys_addr, size), PROT_URW)) == NULL) 
#endif
		return((caddr_t)0);

	/* 
	 * offset into a page is the same for virtual or physical 
	 * so restore the offset by adding back to the virtual address
	 */
	return(prp->p_vaddr + ((int)phys_addr & (NBPG - 1)));
}


/*-----------------------------------------------------------------------*/
caddr_t map_mem_to_host(isc, phys_addr, size)
/*-----------------------------------------------------------------------*/
/* this routine provides a virtual to physical mapping of the requested
 * physical address range.
 */

struct isc_table_type *isc;
int phys_addr, size;
{
	int addr;

	/* 
	 * physical  addresses  are relative to the specific bus address
	 * space therefore they must be added onto the bus base address
	 */
	if (isc->bus_type == EISA_BUS) {
		addr = phys_addr + (int)((struct eisa_bus_info *)
		    (isc->bus_info->spec_bus_info))->eisa_base_addr;
	} else if (isc->bus_type == VME_BUS) {
#ifdef __hp9000s300
		if (phys_addr <= 0xffff)
			addr = phys_addr + 0x10000 + (int)((struct vme_bus_info *)
		  	    (isc->bus_info->spec_bus_info))->vme_base_addr;
		else
			addr = phys_addr | (int)((struct vme_bus_info *)
			    (isc->bus_info->spec_bus_info))->vme_base_addr;
#else
		if ((addr = vme_map_mem_to_host(isc, phys_addr, size)) == 0)
			return((caddr_t)0);
#endif
	
	} else return((caddr_t)0);

	return(host_map_mem((caddr_t)addr, size));
}


/*-----------------------------------------------------------------------*/
unmap_mem_from_host(isc, virt_addr, size)
/*-----------------------------------------------------------------------*/
/* isc free mechanism of unmapping a physical to virtual translation 
 */

struct isc_table_type *isc;
int virt_addr;
int size;
{
#if defined (__hp9000s800) && defined (_WSIO)
	/*
	 * if VME we need to give back master mapper entries first
	 * if error panic since major protected software error.
	 */
	if (isc->bus_type == VME_BUS) 
		if (!vme_unmap_mem_from_host(isc, virt_addr, size)) 
			panic("unmap_mem_from_host: parameters do not match request");
#endif

	return(unmap_host_mem((caddr_t)virt_addr, size));
}


/*-----------------------------------------------------------------------*/
unmap_host_mem(virt_addr, size)
/*-----------------------------------------------------------------------*/
/* this routine removes the virtual to physical mapping obtained by a
 * map_host_mem() call. size is ignored. it provides a more useable shell
 * around io_unmap.
 */

caddr_t virt_addr;
int size;
{
	preg_t *prp;
	caddr_t base_addr;

	/* findprp expects a page aligned address */
	base_addr = (caddr_t)((unsigned long)virt_addr & ~(NBPG - 1));

	/* findprp finds the pregion associated with a virtual address */
#ifdef __hp9000s300
	if ((prp = findprp(KERNVAS, (space_t)KERNVAS, base_addr)) == NULL) {
#else
	if ((prp = findprp(KERNVAS, KERNELSPACE, base_addr)) == NULL) {
#endif
                panic("unmap_host_mem: bad logical address");
        }  

	/* now unmap it */
	io_unmap(prp);
}


/*-----------------------------------------------------------------------*/
io_dma_build_chain(spaddr, log_addr, count, chain_ptr)
/*-----------------------------------------------------------------------*/
/* this routine will build a dma chain consisting of physical address and
 * size pairs. The chain end is indicated by an element with a NULL physical
 * address. It does not use iomap entries and thus is useable for 
 * eisa-to-eisa transfers.
 */

space_t spaddr;
caddr_t log_addr;
unsigned short count;
struct addr_chain_type * chain_ptr;

{
	int p_offset;
	int num_chain_entries = 0;

	while (count > 0) {
#ifdef __hp9000s300
		chain_ptr->phys_addr = (caddr_t)vtop(log_addr);
#else
		chain_ptr->phys_addr = (caddr_t)ltor(spaddr, log_addr);
#endif
		p_offset = (unsigned) log_addr & PGOFSET;
		chain_ptr->count = NBPG - p_offset;
		if (chain_ptr->count > count)
			chain_ptr->count = count;
		log_addr += chain_ptr->count;
		count -= chain_ptr->count;
		chain_ptr++;
		num_chain_entries++;
	}
	chain_ptr->phys_addr = 0;       /*indicates end of chain*/
	return(num_chain_entries);
}

#if defined (__hp9000s800) && defined (_WSIO)
/*-----------------------------------------------------------------------*/
inl(addr)
/*-----------------------------------------------------------------------*/
/* this routine inputs a long value from eisa space and performs the 
 * necessary byte swapping on it.
 */

caddr_t addr;
{
	char *wkb1, wkb2[4];
	int  wk, *ans;

	wk = *((int *)addr);
	wkb1 = (char *) &wk;
	wkb2[0] = wkb1[3];
	wkb2[1] = wkb1[2];
	wkb2[2] = wkb1[1];
	wkb2[3] = wkb1[0];
	ans = (int *)wkb2;
	return (*ans);
	
}


/*-----------------------------------------------------------------------*/
u_short ins(addr)
/*-----------------------------------------------------------------------*/
/* this routine inputs an unsigned short value from eisa space and performs 
 * the necessary byte swapping on it.
 */

caddr_t addr;
{
	char *wkb1, wkb2[2];
	u_short  wk, *ans;

	wk = *((u_short *)addr);
	wkb1 = (char *) &wk;
	wkb2[0] = wkb1[1];
	wkb2[1] = wkb1[0];
	ans = (u_short *)wkb2;
	return (*ans);
}
#endif


#if defined (__hp9000s800) && defined (_WSIO)
/*
 * The following routines are specific for the EISA LAN Card Driver and
 * are not documented for generic users. 
 */
struct iomapentry_info {
   struct  iomapentry_info *next;
   u_long  phys_page; 
   u_int   iomap_tbl_indx;
   u_int   ref_cnt;
};

struct iomap_data lan_iomap_table[1024];

#define NHASHBITS 8
#define NHASHMASK ((1 << NHASHBITS) - 1)

struct iomapentry_info *iomapentry_hash_ptr[1 << NHASHBITS];

/*
 * NOTE: we will no longer use iomap table's ref_count 
 * 
 *       How about statically malloc'ing the hash structures 
 */


/***********************************************************************
 *
 *       io_setup_iomap(isc, virt_addr, option, sid)
 *
 **********************************************************************/
io_setup_iomap(isc, virt_addr, option, sid)
   struct isc_table_type *isc;
   caddr_t               virt_addr;
   int                   option;
   int                   sid;
{
   int    *iomap_start;
   int    iomap_base;
   int    spl, i;
   u_int  indx;
   u_long phys_page, 
	  page_offset,
          phys_addr;
   struct iomapentry_info *info_ptr, *hash_ptr;

   static int iomap_indx = 0;

   iomap_start = isc->bus_info->host_iomap_base;
   iomap_base  = isc->bus_info->bus_iomap_base;

#ifdef __hp9000s300
   phys_addr = (unsigned int)vtop(virt_addr);
#endif

#ifdef __hp9000s800
   phys_addr = (unsigned int)ltor(sid, virt_addr);
#endif

   phys_page   = phys_addr >> PGSHIFT;
   page_offset = phys_addr & 0xfff;

   /* 
    * check to see if page has been mapped 
    * if IOMAP_REF_COUNT increment ref count
    * calculate  mapped address
    *
    * We _have_ to check if the page has already been mapped because
    * this routine maps a page (4K) at a time; and a page can hold
    * 2 clusters or 32 mbufs or anything in between !
    */
   for (info_ptr = iomapentry_hash_ptr[phys_page & NHASHMASK];
          info_ptr != NULL; info_ptr = info_ptr->next) {

	  /* contents of the array which holds the info ptr */

      if (info_ptr->phys_page == phys_page) {

#ifdef LAN_DEBUG
         if (iomap_table[info_ptr->iomap_tbl_indx].entry_value != phys_page) {
            panic("setup: lan2 iomap hash error");
         }
#endif /* LAN_DEBUG */

         if (option == IOMAP_REF_COUNT) {
	    /*
	     * we will not use ref count field in the io_map struct 
	     */
	    info_ptr->ref_cnt++;
         }
         return(iomap_base + (info_ptr->iomap_tbl_indx << 12) + page_offset);
      }
   }

   /* 
    * page has not been mapped
    * first mapping _must_ be done with option set
    */
   if (option == IOMAP_REF_COUNT) {
      /* 
       * malloc a hash entry 
       */
      MALLOC(info_ptr, struct iomapentry_info *, sizeof(struct
	       iomapentry_info), M_DYNAMIC, M_NOWAIT);

      if (info_ptr == NULL) {
	 return(-1);
      }

#ifdef OLD_WAY
      i = iomap_size;
      while (i > 0) {
	 /*
	  * iomap_indx is the previous successful search for an empty
	  * slot into the iomap table
	  */
         if (++iomap_indx == iomap_size)
            iomap_indx = 0;

         if (iomap_table[iomap_indx].locked == -1) {
	    /*
	     * Found a potentially empty entry in the io_map table
	     */
	    spl = spl6();
            if (iomap_table[iomap_indx].locked == -1) {
	       /* 
		* the entry is still available; update iomap entry 
		*/
               iomap_table[iomap_indx].locked = iomap_indx;
	       *(iomap_start + (iomap_indx << 10)) = 
	       iomap_table[iomap_indx].entry_value = phys_page;
	       splx(spl);
            }
	    else {
	    /*
	     * Oops !! Somebody grabbed the entry from behind us, SOB:-|
	     */
	       i--;
	       splx(spl);
	       continue;
            }
#else
         if((iomap_indx = get_rsrc_entries(isc->bus_info->iomap_rsrcmap, 
		LONG_TERM_ENTRY, 1)) > 0) {

	       *(iomap_start + (iomap_indx << 10)) = 
	       lan_iomap_table[iomap_indx].entry_value = phys_page;
#endif

	    /* 
	     * Update hash struct
	     */
            info_ptr->phys_page = phys_page;
            info_ptr->iomap_tbl_indx = iomap_indx;
            info_ptr->ref_cnt = 1;

            /*
	     * Add the new hash strcuture at the beginning of the linked
	     * list of hash strcutures
	     */
            hash_ptr = (struct iomapentry_info *)
                        &(iomapentry_hash_ptr[phys_page & NHASHMASK]);
            info_ptr->next = hash_ptr->next;
	    hash_ptr->next = info_ptr;

            /*
	     * return the mapped_to_eisa_space_address
	     */
            return(iomap_base + (iomap_indx << 12) + page_offset);
         }
#ifdef OLD_WAY
	 else {
            i--;
         }
      }
#endif

      FREE(info_ptr, M_DYNAMIC);

   }

   /* 
    * no free entries _OR_ 
    * page was not mapped AND called without option set
    */
   return(-1);
}


/***********************************************************************
 *
 *      io_release_iomap(isc, eisa_addr)
 *
 **********************************************************************/
io_release_iomap(isc, eisa_addr)
   struct isc_table_type *isc;
   u_long                 eisa_addr;
{

   int    *iomap_start;
   int    iomap_base, iomap_size;
   int    old_spl;
   u_int  phys_page;
   u_int  indx;    /* index into iomap tbl for phys_addr */
   struct iomapentry_info *prev_ptr, *info_ptr;

   iomap_base = isc->bus_info->bus_iomap_base;
   iomap_size = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->sysbrd_parms->size_iomap;
   iomap_start = isc->bus_info->host_iomap_base;

   indx = ((eisa_addr - iomap_base) & 0xfff000) >> 12;

   if (indx < 0 || indx >= iomap_size) {
      panic("io_release_iomap(): invalid eisa address");
   }

   phys_page = lan_iomap_table[indx].entry_value;

   /*
    * Since lan2 and lan5 process interrupts at spl5, and call this
    * routine at spl5, AND lan2 and lan5 are the only routines which 
    * know about the hash structures, it is unnecessary to protect the 
    * hash struct with spl6. 
    */

#ifdef LAN_DEBUG
   if (phys_page == 0) {
      panic("release: freeing unused/wrong entry in iomap");
   }
#endif /* LAN_DEBUG */

   /* 
    * find entry in hash table 
    */
   prev_ptr = (struct iomapentry_info *)
	         &(iomapentry_hash_ptr[phys_page & NHASHMASK]);

   for ( ; prev_ptr->next != NULL; prev_ptr = prev_ptr->next) {
      if (prev_ptr->next->phys_page == phys_page) {
	 goto FOUND_ENTRY;
      }
   }

   /* 
    * Didn't find it in hash table 
    */
   panic("release: lan driver iomap hash error");

FOUND_ENTRY:
   info_ptr = prev_ptr->next;

#ifdef LAN_DEBUG
   if (indx != info_ptr->iomap_tbl_indx) {
      panic("release: iomap tbl indx is NOT consistent");
   }
#endif /* LAN_DEBUG */

   /* 
    * delete entry in hash table 
    */
   if ((--(info_ptr->ref_cnt)) == 0) {
      prev_ptr->next = info_ptr->next;
      FREE(info_ptr, M_DYNAMIC);

#ifdef OLD_WAY
      /*
       * now update the iomap data structure
       * NOTE: this portion is protected with spl6
       */
      old_spl = spl6();
      /*
       * Unlock the iomap entry to indicate availability
       */
      iomap_table[indx].locked = -1;
      
      /*
       * Notify other drivers if they waiting to for this iomap entry 
       * to be unlocked
       */
      isc->bus_info->iomap_wait_list = NULL;

      while (this_entry != NULL) {
         (*this_entry->drv_routine)(this_entry->drv_arg);
         previous = this_entry;
         this_entry = this_entry->next;
         kmem_free(previous, sizeof(struct driver_to_call));
      }
      splx(old_spl);
#else
	free_rsrc_entries(isc->bus_info->iomap_rsrcmap, indx, 1);
	free_wait_list(isc);
#endif
   }
}
#endif


/***********************************************************************
 *
 *	LED management routines
 *
 **********************************************************************/

#ifdef __hp9000s300
/*
 * led_init() -- determine the kernel logical address for the LED
 *		 register.
 */

unsigned char *led_laddr;

led_init()
{
	extern vas_t kernvas;
	extern preg_t *io_map();
	extern int activity_led_enb;

	register preg_t *prp;

	/* allocate virtual address space, map in pages */
	if ( (prp = io_map((space_t)&kernvas,
			   NULL, 0x1ffff, 1, PROT_URW)) == NULL)
		panic("led_init: Can't map in leds");

	/* save the kernel logical address */
	led_laddr = (unsigned char *)prp->p_vaddr + 3;
	activity_led_enb = 1;
}

/*
 * BEGIN_DESC
 *
 * update_leds(led_value) [Series 300 version]
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	led_laddr	Kernel logical address to LED register
 *
 * Description:
 *	Set the LEDs to the values specified by the passed parameter.
 *	The LEDs and associated activity they indicate (heartbeat,
 *	disk, LAN receive, and LAN transmit) are defined in param.h.
 *
 * Algorithm:
 *	Simply write the requested value to the LED register.
 *
 * END_DESC
 */
update_leds(led_value)
u_char led_value;
{
	extern unsigned char *led_laddr;

	*led_laddr = ~led_value; /* s300 is inverted from s700 */
}
#endif /* __hp9000s300 */

#ifdef __hp9000s700
/*
 * BEGIN_DESC
 *
 * update_leds(led_value) [Series 700 version]
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	asp		Pointer to Asp chip which contains register
 *			for accessing the LEDs.
 *
 * Description:
 *	Set the LEDs to the values specified by the passed parameter.
 *	The LEDs and associated activity they indicate (heartbeat,
 *	disk, LAN receive, and LAN transmit) are defined in param.h.
 *
 * Algorithm:
 *	The value inverted before it is shifted and clocked out to the
 *	front panel LED display.
 *
 *      For older versions of ASP:
 *	  The led register referenced by asp->led_control is a two bit
 *	  register, the LSB is the data that is to be shifted out to the
 *	  LEDs, and the MSB is used to clock the shifting, the clocking
 *	  occurs when this bit transitions from a 0 to 1.
 *
 *      For newer versions of ASP (cutoff):
 *	  The led register referenced by asp->led_control is an 8 bit
 *	  register, and the led value is just written directly to it.
 *
 *	Note:	The updating of the LEDs could of been perform via
 *		PDC_CHASSIS, but in the interest of performance this
 *		is performed directly.
 * END_DESC
 */

int led_method = 0;

update_leds(led_value)
register u_long led_value;
{
	extern struct corehpa2	*asp;
	extern struct corehpa	*core_registers;

	register volatile unsigned char *asp_led;
	register unsigned long i;

	asp_led = &asp->led_control;
	led_value = ~led_value;

	if (led_method == 0) {
	    char asp_version = core_registers->core_asp_version & 0x0f;

	    switch (asp_version) {
	    case 0:
	    case 1:
	    case 2:
		/* old version of ASP, has weird 2 bit led register */
		led_method = 1;
		break;
	    default:
		msg_printf(
		    "Unknown asp version 0x%02x, assuming 8 bit led register\n",
		    asp_version);
		/* FALLS THROUGH */
	    case 3:
		/* new version of ASP (cutoff), has 8 bit led register */
		led_method = 2;
		break;
	    }
	}

	if (led_method == 1) {
	    for (i = 8; i; --i) {
		*asp_led = led_value & 0x01;
		*asp_led = led_value & 0x01 | 0x02;
		led_value >>= 1;
	    }
	}
	else {
	    register unsigned char x = (unsigned char)led_value;

	    /*
	     * The ordering of the bits is backwards from what we
	     * maintain.  This formula reverses the bits.
	     */
	    x = (x << 4 & 0xf0) | (x >> 4 & 0x0f);
	    x = (x << 2 & 0xcc) | (x >> 2 & 0x33);
	    x = (x << 1 & 0xaa) | (x >> 1 & 0x55);

	    *asp_led = (char)x;
	}
}
#endif /* __hp9000s700 */

/*
 * Update the front panel LEDs.	 We do this from hardclock(), to
 * avoid doing it so often.  Since updating the LEDs is relatively
 * expensive, and the human eye cannot perceive flicker rates more
 * than about 60hz, this is a good place to update LED values.	This
 * way we prevent unnecessary updates that nobody could notice anyway.
 *
 * The global activity_led_enb is a flag used to enable the updating
 * of the LEDs to show system activity.	 It starts out 0 (False).  At
 * the end of main() the flag is set to 1 (True) to enable the use of
 * the LEDs for displaying system activity.  The displaying of system
 * activity is disabled in three places when the system is being
 * brought down under user control or in crash situations.  The three
 * places where the flag is set to 0 (False) are: panic(), boot() and
 * crash_dump().  Code after these three points and code before the
 * end of main() use the display for other purposes.
 */

/*
 * Variables that must be visible to other parts of the kernel.
 */
int	activity_led_enb = 0;
int	load_led_enb = 1;
u_char	led_activity_bits = 0;

display_led_activity()
{
	static u_long inactivity_count;
	static u_long load_toggle_cntr = HZ; /* prevent div by 0 */
	static u_long load_toggle_off;
	static u_long load_toggle_off_cnt;
	static u_char load_toggle_bit;
	static u_char load_led_value;
	static u_char heartbeat_led;
	static u_char led_value;
	static int heartbeat_cnt;

	register u_long act_bits = led_activity_bits & 0xf0;
	register u_long leds	 = led_value;
	register u_long new_leds;

	/*
	 * Every second we update the LED load indicator value.
	 */
	if (load_led_enb && load_toggle_cntr-- == 0) {
		extern long cp_time[];

		static u_long old_busy_time;
		static u_long old_idle_time;

		register u_long busy_time;
		register u_long idle_time;
		register u_long pct_busy;
		u_long divisor;

		load_toggle_cntr = HZ/4; /* update every 1/4 second */

		/*
		 * Calculate the time that we were busy doing work...
		 */
		busy_time = cp_time[CP_USER] +	/* user mode process */
			    cp_time[CP_NICE] +	/* user mode niced process */
#ifdef CP_INTR
			    cp_time[CP_INTR] +	/* interrupt mode */
#endif
#ifdef CP_SSYS
			    cp_time[CP_SSYS] +	/* kernel mode of kernel proc */
#endif
			    cp_time[CP_SYS];	/* kernel mode of user proc */
		/*
		 * Calculate the time we were twiddling our thumbs...
		 */
		idle_time =
#ifdef CP_WAIT
			    cp_time[CP_WAIT] +	/* waiting for I/O */
#endif
#ifdef CP_BLOCK
			    cp_time[CP_BLOCK] +	/* waiting for a spinlock */
#endif
#ifdef CP_SWAIT
			    cp_time[CP_SWAIT] +	/* waiting for kernel sem */
#endif
			    cp_time[CP_IDLE];	/* totally idle */

		/*
		 * Guard against division by 0.  In certain instances
		 * we will have failed to accumulate any CPU ticks,
		 * which leaves us with a divisor of 0.  We fake it
		 * and say we are 100% busy in this case.
		 */
		divisor = (busy_time - old_busy_time +
			   idle_time - old_idle_time);
		if (divisor != 0)
		    pct_busy = ((busy_time - old_busy_time) * 100) / divisor;
		else
		    pct_busy = 100;

		old_busy_time = busy_time;
		old_idle_time = idle_time;

		load_led_value	= 0x0f >> (4 - (pct_busy+24)/25);
		load_toggle_bit = 0x08 >> (4 - (pct_busy+24)/25);
		load_toggle_off = 25 - (pct_busy % 25);

		if (load_toggle_off == 25)
			load_toggle_bit = 0;
	}

	/*
	 * Code to make the KERN_OK_LED blink in a "heartbeat"
	 * pattern.  We monitor the KERN_OK_LED in led_activity_bits
	 * bits.  When it is set, we reset our heartbeat counter.
	 * If our heartbeat counter goes past 100, we stop beating
	 * the heart until the KERN_OK_LED is set again.
	 *
	 * schedcpu() is supposed to set the KERN_OK_LED bit in
	 * led_activity_bits (by calling toggle_led()) once per
	 * second.  If it does not do this, then the kernel is sick.
	 */
	if (heartbeat_cnt < HZ)
	{
	    switch (heartbeat_cnt++)
	    {
	    case ( 0 * HZ) / 100:
	    case (20 * HZ) / 100:
		heartbeat_led = 1;
		break;
	    case ( 5 * HZ) / 100:
	    case (27 * HZ) / 100:
		heartbeat_led = 0;
		break;
	    }
	}
	else
	    heartbeat_led = 0;

	if (act_bits & KERN_OK_LED)
	    heartbeat_cnt = 0;

	/*
	 * The local static variable "led_value" holds the current
	 * state of all 8 LEDs.
	 *
	 * The global activity_bits contains a mask of the
	 * bits LAN_XMIT_LED, LAN_RCV_LED, DISK_DRV_LED and
	 * KERN_OK_LED.	 For the first three, we toggle the
	 * lights if there was activity, or force the light
	 * off if there was no activity.  See above for how we
	 * handle KERN_OK_LED.
	 *
	 * The low 4 bits are used as a relative system load indicator.
	 * As "busy" time increases, so does the number of LEDS that
	 * are illuminated.  The brightness of the last illuminated
	 * load LED is adjusted in relation to the percent load.  Each
	 * LED represents 25% of "busy" time.
	 *
	 * For example, if the system was 70% busy, we would have 2
	 * LEDs completely lit (70 / 25).  The third LED would be
	 * toggled so that it was on 80% of the time [i.e. we use the
	 * formula (70 % 25) / 25, which yields 0.80].
	 */
	if (load_toggle_off_cnt-- == 0) {
		load_led_value |= load_toggle_bit;
		load_toggle_off_cnt = load_toggle_off;
	}
	else {
		load_led_value &= ~load_toggle_bit;
	}

	led_activity_bits = 0;
	new_leds = ((leds ^ act_bits) & 0xf0) | (load_led_value & 0x0f);

	if (heartbeat_led)
	    new_leds |= KERN_OK_LED;
	else
	    new_leds &= ~KERN_OK_LED;

	if (inactivity_count-- == 0) {
		/*
		 * Turn off all bits except for the heartbeat and the
		 * load indicators.
		 */
		new_leds &= (act_bits | KERN_OK_LED | 0x0f);
		inactivity_count = HZ/20;

		/*
		 * If someone disables the load leds, we want to
		 * ensure that we turn them off.  We do this here
		 * (rather in the mainstream code) to minimze the
		 * overhead of this check.
		 */
		if (load_led_enb == 0) {
			load_led_value = load_toggle_bit = 0;
			new_leds &= 0xf0;
		}
	}

	if (new_leds != leds) {
		update_leds(led_value = new_leds);
	}
}
