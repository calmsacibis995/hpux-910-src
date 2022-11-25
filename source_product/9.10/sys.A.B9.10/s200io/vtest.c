/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/vtest.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:20:10 $
 */

/* HPUX_ID: @(#)vtest.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../wsio/timeout.h"  /* for hpibio.h */
#include "../wsio/hpibio.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../s200io/vme2.h"
#include "../machine/pte.h"
#include "../s200io/vtest.h"

extern int vtest_isr();
int vtest_dma_isr();
extern int unmap_mem_from_host();
extern caddr_t dio2_map_mem();
extern caddr_t kmem_alloc();
extern struct isc_table_type *vme_init_isc();
extern caddr_t map_mem_to_host();
extern struct bus_info_type *vme;

extern int current_io_base;

struct buf vtest_buf;
struct buf *vtest_bp = &vtest_buf;
struct active_vme_sc *vtest_sc = NULL;

int vtest_wakeup();
int vtest_fhs();

struct vme_dma_chain *vtest_dma_next;
struct addr_chain_type *vtest_iomapdma_next;


/*-------------------------------------------------------------------------*/
caddr_t vt_ram_locate(isc, ram_sc, cp, phys_cp)
/*-------------------------------------------------------------------------*/
/* Decode the ram address field of the minor number and locate the ram     */
/* accordingly.                                                            */

struct isc_table_type *isc;
int ram_sc;
register struct vmetestregs *cp;
int phys_cp;
{
	caddr_t log_ram_addr;
	register ram_address; 

	cp->low_ram_bit = 0;	/* powerup state undefined, clear register */
	if (!ram_sc) {		/* not specified, use 2nd half of card sc */
		cp->ram_addr = phys_cp >> 16;
		cp->low_ram_bit |= LOW_RAM_ADDR;
		cp->ram_AM = A24_AM;
		log_ram_addr = (caddr_t)cp + 32768;  
	} else {
		if (ram_sc > 255 || (ram_sc > 63 && ram_sc <132))
			return((caddr_t)0);	/* bad ram address */

		if (ram_sc < 48) {  /* 48 = bottom of DIO RAM space */
			ram_address = (0x600000 + 0x10000*ram_sc);  
			cp->ram_addr = ram_address >> 16;
			if (((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_STEALTH) 
				/* convert physical address to logical */
				log_ram_addr = (caddr_t)(current_io_base + ram_address);
			 else /* is MTV */
				/* convert physical address to logical */
				log_ram_addr = map_mem_to_host(isc, ram_address, RAM_SIZE);
			if ((ram_sc == ((int)(phys_cp - 0x600000)/ 0x10000) + 1)
				&& (((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_STEALTH)) 
				cp->ram_AM = A16_AM; /* sc is 2nd 1/2 of stealth, A16 */
			else cp->ram_AM = A24_AM;
		} else if (ram_sc >= 48 && ram_sc < 64)  /* 9M - 10M */ {
			ram_address = (0x600000 + 0x10000*ram_sc);
			cp->ram_addr = ram_address >> 16;
			/* convert physical address to logical */
			log_ram_addr = map_mem_to_host(isc, ram_address, RAM_SIZE);
			cp->ram_AM = A24_AM;
		} else {		/* in dio2 sc space 133-255 */
			ram_address = (0x1000000 + 0x400000*(ram_sc - 132));
			cp->ram_addr = ram_address >> 16;
			/* convert physical address to logical */
			log_ram_addr = map_mem_to_host(isc, ram_address, RAM_SIZE);
			cp->ram_AM = A32_AM;
		}
	}
	return(log_ram_addr);
}


/*-------------------------------------------------------------------------*/
vt_card_init(cp)
/*-------------------------------------------------------------------------*/
/* Initialize card registers to power-up defaults. 		           */
/* This routine is called by vtest_open on first open. 			   */

register struct vmetestregs *cp;
{
	/* initialize card registers */
	cp->assert_intr |= CLEAR_INT0 | CLEAR_INT1 | CLEAR_INT2; /* clear interrupts */
	cp->ram_AM |= RAM_ENABLE;		/* enable RAM */
	cp->dest_addr = 0;			/* clear destination address */
	cp->control = 0;
	cp->dest_AM = A32_AM | DMA_DISABLE;	/* A32 DMA, clear data mode bits */

	cp->int0 = INT_LVL;			/* set int lvl, 0 other bits */
	cp->int1 = INT_LVL; 			/* set int lvl, 0 other bits */
	cp->int2 = INT_LVL; 			/* set int lvl, 0 other bits */
	cp->int3 = INT_LVL; 			/* set int lvl, 0 other bits */
}


/*-------------------------------------------------------------------------*/
vtest_open(dev, flag)
/*-------------------------------------------------------------------------*/
/* Open test card.  If first open, do card initialization. 		   */

dev_t dev;
int flag;
{
	int card_sc, ram_sc;
	register struct vmetestregs *cp;      /* pointer to card registers */
	int phys_cp;
	register struct isc_table_type *isc;
	caddr_t log_ram_addr;
	register struct active_vme_sc *my_ptr;

	card_sc = m_selcode(dev);             /* get select code from dev */

	if (card_sc < 1 || card_sc > 63)
		return(ENXIO);		      /* invalid select code */

	if (card_sc == 40 || card_sc == 51 || card_sc == 54 || card_sc == 60)
		return(ENXIO);		      /* invalid select codes */

	/* see if this card has been initialized,ie is this first open? */
	if (isc_table[card_sc] == NULL) { /* first open */ 

		/* initialize it */
		isc_table[card_sc] = vme_init_isc(0);
		isc = isc_table[card_sc];

		/* locate it */
		if (card_sc >= 48) { /* whatever the card need map_mem_host */
			phys_cp = (0x600000 + 0x10000*card_sc); /* get physical address */
			/* map in sc's worth of space into kernel virtual memory */
			cp = (struct vmetestregs *)map_mem_to_host(isc, phys_cp, 65536);  

			if (cp == (struct vmetestregs *)0)
				panic("vtest: can't map in select code space");
		} else {
			if (((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_STEALTH) {
				phys_cp = (0x600000 + 0x10000*card_sc); /* get physical address */
				/* DIO space is already mapped into kernel vm space */
				cp = (struct vmetestregs *)(current_io_base + phys_cp);
			} else { 
				/* expander is MTV */
				phys_cp = (0x600000 + 0x10000*card_sc);
			        /* map in sc's worth of space into kernel virtual memory */
				cp = (struct vmetestregs *)map_mem_to_host(isc, phys_cp, 65536);
				if (cp == (struct vmetestregs *)0)
					panic("vtest: can't map in select code space");
			}
		}

		if (!(testr((char *)cp, 1) &&	/* test if a card exists at location */
	    		testr((char *)cp->tfr_count,1) &&  /* if so, is this our card? */
	    		!testr((char *)((int)cp+33),1) &&  /*this one should buserror */
	    		testr((char *)cp->int0, 1))) {

			/* card is not here or this is not our card */
			return(ENXIO);
		}

		/* reset card to power on state */
		/* interupts will be disabled */
		cp->reset = 1;                      

		/* convert ram select code into ram address and set ram location registers */
		ram_sc = m_busaddr(dev);
		if ((log_ram_addr = vt_ram_locate(isc, ram_sc, cp, phys_cp)) == (caddr_t)0)
			return(ENXIO);
			
		/* initialize card registers */
		vt_card_init(cp);

		/* get some interrupt vectors */
		if ((isc->vec_num1 = allocate_vector(0, vtest_isr) ) == -1)
			panic("VME: out of interrupt vectors");
		cp->intvec0 = isc->vec_num1;
		cp->intvec1 = isc->vec_num1;

		if ((isc->vec_num2 = allocate_vector(0, vtest_dma_isr) ) == -1)
			panic("VME: out of interrupt vectors");
		cp->intvec2 = isc->vec_num2;
		cp->intvec3 = isc->vec_num2;

		/* set up isc entry */
		isc->my_isc = card_sc;
		isc->card_ptr = (int *)cp;
		isc->int_lvl = INT_LVL;
		isc->vt_ram = (int)log_ram_addr;
		isc->transfer = DMA_TFR;
		isc->tfr_control = VTEST_USE_BUILDCH;
		isc->use_iomap = VTEST_NO_IOMAP;
		isc->dma_data_mode = BYTE_DMA;		/* default = D8 DMA ?? */
		(struct vme_dma_chain *)isc->dma_chan = (struct vme_dma_chain *)vme_dma_init();

		/* buffer for DMA, locked down */
 		if ((isc->buffer = kmem_alloc(RAM_SIZE)) == NULL) {
 			isc_table[card_sc] = NULL;
 			return(ENOSPC);  
 		}
	
		/* add sc to list of valid vtest sc's */
		my_ptr = vtest_sc;
		if (my_ptr != NULL)
		{
			while (my_ptr->next != NULL)
				my_ptr = my_ptr->next;
			my_ptr->next = (struct active_vme_sc *)calloc(sizeof(struct active_vme_sc));
			my_ptr = my_ptr->next;
		}
		else {
			vtest_sc = (struct active_vme_sc *)calloc(sizeof(struct active_vme_sc));
			my_ptr = vtest_sc;
		}
		my_ptr->sc = card_sc;
		my_ptr->card_ptr = cp;
		my_ptr->next = NULL;
	} /* card initialization check */

	/* open card */

	/* enable interrupts */
	isc = isc_table[m_selcode(dev)];
	cp = (struct vmetestregs *)isc->card_ptr;
	cp->int0 |= INT_ENABLE;
	cp->int1 |= INT_ENABLE;
	cp->int2 |= INT_ENABLE;
	/* int3 is for DMA completion and is not enabled until DMA is started */
	return(0);
}


/*-------------------------------------------------------------------------*/
vtest_close(dev, flag)
/*-------------------------------------------------------------------------*/

dev_t dev;
int flag;
{
	struct isc_table_type *isc = isc_table[m_selcode(dev)];
	register struct vmetestregs *cp = (struct vmetestregs *)isc->card_ptr;

	/* disable interrupts */
	cp->int0 &= ~INT_ENABLE;
	cp->int1 &= ~INT_ENABLE;
	cp->int2 &= ~INT_ENABLE;
	return(0);
}


int vtest_strategy (); /* forward declaration */


/*-------------------------------------------------------------------------*/
vtest_read(dev, uio)
/*-------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	struct isc_table_type *isc = isc_table[m_selcode(dev)];
	struct vmetestregs *cp = (struct vmetestregs *)isc->card_ptr;

	if (cp->status & RMW)
		return(EIO);	/* can't do RMW cycle on a read */
	return(physio(vtest_strategy, vtest_bp, dev, B_READ, minphys, uio));
}


/*-------------------------------------------------------------------------*/
vtest_write(dev,uio)
/*-------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	return(physio(vtest_strategy, vtest_bp, dev, B_WRITE, minphys, uio));
}


/*-------------------------------------------------------------------------*/
vtest_strategy(bp)
/*-------------------------------------------------------------------------*/

register struct buf *bp;
{
	register struct isc_table_type *isc = isc_table[m_selcode(bp->b_dev)];
	struct vmetestregs *cp = (struct vmetestregs *)isc->card_ptr;
	struct dma_parms *dma_parms;
	int pri, ret;

	/* ensure only one process is transfering at a time (sc management)*/
	pri = spl6(); 		/* determine appropriate level */

	if (isc->owner != NULL)
		sleep(isc, PRIBIO); /* someone already has it */

	isc->owner = bp;            /* take it */

	switch(isc->transfer) {
		case DMA_TFR:
			if (isc->tfr_control == VTEST_USE_BUILDCH) {
				if ((cp->ram_AM & RAM_AM_MASK) == A32_AM) {  
					/* if A32, do DMA */
					if (bp->b_flags & B_READ) {	 /* if read */
						cp->control &= ~DMA_WRITE;
					} else {  	/*ifdef this for benchmarks */
					/* This dance copies the buffer to transfer into the isc 
					   allocated buffer.  This is done because of the way the 
				 	   DMA address determines the offset into the test card 
					   RAM for the transfer.  By using a locked down location, 
					   the offset will always be the same
					*/
						bcopy(bp->b_un.b_addr, isc->buffer, bp->b_bcount);
						cp->control |= DMA_WRITE;
					}

					vme_dma_build_chain(isc->buffer, bp->b_bcount, 
						(struct vme_dma_chain *) isc->dma_chan);
					vtest_dma_next = (struct vme_dma_chain *)isc->dma_chan;
					vtest_dma_setup(isc);
					vtest_dma_start(isc);
					break;
				}
				/* else, fall through and do FHS */
			} else {	/* use vme_dma_setup */
				bp->b_s2 = (int) kmem_alloc(sizeof(struct dma_parms));
				dma_parms = (struct dma_parms *)bp->b_s2;

				if (bp->b_flags & B_READ) {	 /* if read */
					cp->control &= ~DMA_WRITE;
				} else {  	/*ifdef this for benchmarks */
					bcopy(bp->b_un.b_addr, isc->buffer, bp->b_bcount);
					cp->control |= DMA_WRITE;
				}
				dma_parms->addr = isc->buffer;
				dma_parms->count = bp->b_bcount;
				dma_parms->flags = 0;
				dma_parms->drv_routine = vtest_wakeup;
				dma_parms->drv_arg = (int)isc;
				dma_parms->dma_options = 0;

				if ((cp->ram_AM & RAM_AM_MASK) == A32_AM) {  
					/* if A32, do DMA */
					dma_parms->dma_options |= VME_A32_DMA;
					if (isc->use_iomap == VTEST_USE_IOMAP)
						dma_parms->dma_options |= VME_USE_IOMAP;
				} else if ((cp->ram_AM & RAM_AM_MASK) == A24_AM) {
					/* if A24, do DMA */
					dma_parms->dma_options |= VME_A24_DMA;
					if (isc->use_iomap == VTEST_USE_IOMAP)
						dma_parms->dma_options |= VME_USE_IOMAP;
					else {
						kmem_free(bp->b_s2, sizeof(struct dma_parms));
						bp->b_error = EIO;
						bp->b_flags |= B_ERROR;		
						iodone(bp);
						isc->owner = NULL;
						break;
					}
				} else { /* A16 */
					kmem_free(bp->b_s2, sizeof(struct dma_parms));
					bp->b_error = EIO;
					bp->b_flags |= B_ERROR;		
					iodone(bp);
					isc->owner = NULL;
					break;
				}

				while (ret = vme_dma_setup(isc, dma_parms)) {
					if (ret == RESOURCE_UNAVAILABLE)
						sleep(bp->b_s2, PZERO+2);
					else if (ret < 0) {
						kmem_free(bp->b_s2, sizeof(struct dma_parms));
						bp->b_error = EINVAL;
						bp->b_flags |= B_ERROR;		
						iodone(bp);
						isc->owner = NULL;
						break;
					} else break;
				}

				if (isc->use_iomap == VTEST_USE_IOMAP) {
					cp->dest_addr = (u_int) dma_parms->chain_ptr->phys_addr;
					cp->tfr_count = dma_parms->chain_ptr->count;
					cp->control |= DMA_ENABLE;
					cp->int3 |= INT_ENABLE;
					vtest_dma_start(isc);
					break;
				} else {	/* A32, not using I/O map */
					vtest_iomapdma_next = dma_parms->chain_ptr;
					vtest_iomapdma_setup(isc);
					vtest_dma_start(isc);
					break;
				}
			}
		case FHS_TFR:
			sw_trigger(&isc->intloc, vtest_fhs, isc, 0, 1);
			break;
		default:
			panic("vtest: unknown transfer type");
		}
	splx(pri);
	return(0);  /* any error condition returns ??? */
}


/*-------------------------------------------------------------------------*/
vtest_dma_setup(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	register struct vmetestregs *cp = (struct vmetestregs *) isc->card_ptr;

	cp->dest_addr = (long) vtest_dma_next->phys_addr;
	cp->tfr_count = (long) vtest_dma_next->count;
	vtest_dma_next++;

	cp->control |= DMA_ENABLE;
	cp->int3 |= INT_ENABLE;
}


/*-------------------------------------------------------------------------*/
vtest_iomapdma_setup(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	register struct vmetestregs *cp = (struct vmetestregs *) isc->card_ptr;

	cp->dest_addr = (long) vtest_iomapdma_next->phys_addr;
	cp->tfr_count = (long) vtest_iomapdma_next->count;
	vtest_iomapdma_next++;

	cp->control |= DMA_ENABLE;
	cp->int3 |= INT_ENABLE;
}


/*-------------------------------------------------------------------------*/
vtest_dma_start(isc)
/*-------------------------------------------------------------------------*/

register struct isc_table_type *isc;
{
	register struct vmetestregs *cp = (struct vmetestregs *)isc->card_ptr;

	cp->dest_AM |= isc->dma_data_mode << 6;
}


/*-------------------------------------------------------------------------*/
vtest_dma_isr()
/*-------------------------------------------------------------------------*/
{

	register struct vmetestregs *cp;
	register struct isc_table_type *isc;
	register struct active_vme_sc *my_ptr;
	int found, pri;

/***	BEGIN_ISR;  Obsolete ***/
	found = 0;

	pri = spl6();
	if (vtest_sc == NULL)
		panic("vtest: interrupt; no cards initialized");

	my_ptr = vtest_sc;
	while ((my_ptr) && (!found))
		if (my_ptr->card_ptr->dest_AM & DMA_DATA_MASK)
			found = 1;
		else my_ptr = my_ptr->next;
	if (!found)
		panic("vtest_dma_isr:  nobody interrupted");	/* or ignore?? */
	cp = my_ptr->card_ptr;
	splx(pri);

	/* disable interrupt */
	cp->int3 &= ~INT_ENABLE;
	cp->control &= ~DMA_ENABLE;
	cp->dest_AM &= ~DMA_DATA_MASK;

	isc = isc_table[my_ptr->sc];

	if (isc->owner->b_flags & B_READ)
		purge_dcache();

	if (cp->status & BERR) {
		cp->control &= ~BERR;
		isc->owner->b_resid += (vtest_dma_resid(isc) + 1);

		if (isc->tfr_control == VTEST_USE_SETUP) 
			vme_dma_cleanup(isc, isc->owner->b_s2);

		cp->control &= ~RMW;
	
		if (isc->owner->b_flags & B_READ)
			bcopy(isc->buffer, isc->owner->b_un.b_addr, 
				isc->owner->b_bcount - isc->owner->b_resid);
		iodone(isc->owner);
		isc->owner = NULL;
		wakeup(isc);
	} else if (vtest_dma_done(isc) ) {
		cp->control &= ~RMW;
		
		if (isc->owner->b_flags & B_READ)
			bcopy(isc->buffer, isc->owner->b_un.b_addr, isc->owner->b_bcount);
		iodone(isc->owner);
		isc->owner = NULL;
		wakeup(isc);
	}
/***	END_ISR;  Obsolete ***/
}


/*-------------------------------------------------------------------------*/
vtest_dma_done(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	struct dma_parms *dma_parms;

	if (isc->tfr_control == VTEST_USE_BUILDCH) {
		if (!vtest_dma_next->phys_addr) {
			isc->owner->b_resid += vtest_dma_resid(isc);
			return(1);
		} else {
			isc->owner->b_resid += vtest_dma_resid(isc);
			vtest_dma_setup(isc);
			vtest_dma_start(isc);
			return(0);
		}
	} else {	/* using dma_setup */
		dma_parms = (struct dma_parms *)isc->owner->b_s2;
		if (dma_parms->dma_options & VME_USE_IOMAP) {
			isc->owner->b_resid += vtest_dma_resid(isc);
			vme_dma_cleanup(isc, dma_parms);
			kmem_free(isc->owner->b_s2, sizeof(struct dma_parms));
			return(1);
		} else {	/* not using I/O map */
			if (!vtest_iomapdma_next->phys_addr) {
				isc->owner->b_resid += vtest_dma_resid(isc);
				vme_dma_cleanup(isc, dma_parms);
				kmem_free(isc->owner->b_s2, sizeof(struct dma_parms));
				return(1);
			} else {
				isc->owner->b_resid += vtest_dma_resid(isc);
				vtest_dma_setup(isc);
				vtest_dma_start(isc);
				return(0);
			}
		}
	}
}


/*-------------------------------------------------------------------------*/
int vtest_fhs(isc)
/*-------------------------------------------------------------------------*/

register struct isc_table_type *isc;
{
	struct buf *bp = isc->owner;
	register char *buffer = (char *)bp->b_un.b_addr;
	register int count = bp->b_bcount;
	struct vmetestregs *cp = (struct vmetestregs *)isc->card_ptr;
	register char *ram_addr = (char *) isc->vt_ram;
	int pri;

	while (count--) {
		if (bp->b_flags & B_READ)
			*buffer++ = *ram_addr++;
		else	/* write */
			*ram_addr++ = *buffer++;
	}

	cp->control &= ~RMW;
	pri = spl6();
	isc->owner->b_resid = 0;
	isc->owner = NULL;
	wakeup(isc);
	splx(pri);
	iodone(bp);
}


/*-------------------------------------------------------------------------*/
vtest_ioctl(dev, cmd, arg_ptr)
/*-------------------------------------------------------------------------*/

dev_t dev;
int cmd;
int *arg_ptr;
{

	caddr_t log_ram_addr;
	unsigned int ram_address; 

	struct isc_table_type *isc = isc_table[m_selcode(dev)];
	register struct vmetestregs *cp = (struct vmetestregs *)isc->card_ptr;
	int return_value = 0;
	int old_ram_address;
	int phys_cp = (0x600000 + 0x10000*m_selcode(dev));
	int pri;
	struct VME *vme_cp = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;
	int ram_sc;

	pri = spl6();
	if (isc->owner)
		sleep(isc, PRIBIO);
	(struct isc_table_type *)isc->owner = isc;
	splx(pri);

	switch (cmd) {
		case SET_RAM_ADDR:
			ram_address = (unsigned int)*arg_ptr;

			if ((ram_address < 0x600000) || (ram_address >= 0x900000 &&
							ram_address < 0x1000000) ) { 
				return_value = EINVAL; 
				break; 
			}
 
			old_ram_address = (cp->ram_addr << 16) + ((cp->low_ram_bit &
						LOW_RAM_ADDR) ? 0x8000 : 0);

			/* resetting to same place as ram currently is */
			if (old_ram_address == ram_address)  {
				break;
			}

			/* kludge to keep old CE.utils working with MTV -- XXX */
			if (((ram_address >> 16) == (int)(0x790000) >> 16) &&
					(((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_MTV)) 
				ram_address = 0;

			cp->ram_AM &= ~RAM_ENABLE;

			if (ram_address == phys_cp + 0x8000) { /* if setting to 2nd 1/2 of card */
				if ((old_ram_address >= 0x900000) || 
				((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_MTV)  
					unmap_mem_from_host(isc, isc->vt_ram, RAM_SIZE);

				cp->ram_addr = phys_cp >> 16;
				cp->low_ram_bit |= LOW_RAM_ADDR;
				cp->ram_AM = A24_AM;
				log_ram_addr = (caddr_t)cp + 32768;  
				isc->vt_ram = (int)log_ram_addr;
				cp->ram_AM |= RAM_ENABLE;
				break;
			}

			if (ram_address >= 0x900000) {
				if ((log_ram_addr = map_mem_to_host(isc, ram_address, RAM_SIZE)) 
				  == (caddr_t)0) { 
					cp->ram_AM |= RAM_ENABLE;  /* re-enable RAM */
					return_value = ENOSPC;
					break; 
				}
			}
			else if (((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_STEALTH) 
				log_ram_addr = (caddr_t)(current_io_base + ram_address);
			else { /* is MTV */
				if ((log_ram_addr = map_mem_to_host(isc, ram_address, RAM_SIZE)) 
				  == (caddr_t)0) { 
					cp->ram_AM |= RAM_ENABLE;  /* re-enable RAM */
					return_value = ENOSPC;
					break; 
				}
			}	

			/* if previous RAM location was mapped in with 
			   map_mem_to_host, unmap it to free up the space.
			   Done here after all possible error returns have
			   completed so we don't give up the RAM's location
			   before we know we can assign it the new one
			*/
			if ((old_ram_address >= 0x900000) || 
			((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_MTV)  {
				if (old_ram_address != phys_cp +0x8000) /* if 2nd 1/2 card, don't umap */
					unmap_mem_from_host(isc, isc->vt_ram, RAM_SIZE);
			}
		
			cp->ram_addr = ram_address >> 16;
			/* error check for invalid address ranges??*/
			if ((ram_address & 0x8000) == 0x8000)
				cp->low_ram_bit |= LOW_RAM_ADDR;
			else cp->low_ram_bit &= ~LOW_RAM_ADDR;

			/* reset address modifier */
			if (((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_STEALTH)  {
				if ((ram_address >> 16) == (int)(vme_cp + 0x10000) >> 16) {
					cp->ram_AM = A16_AM;
					cp->ram_addr = 0;
				} else if (ram_address > 0xff8000)
					cp->ram_AM = A32_AM;
				else cp->ram_AM = A24_AM;
			} else {
				if (ram_address <= 0xffff)
					cp->ram_AM = A16_AM;
				else if (ram_address > 0xff8000)
					cp->ram_AM = A32_AM;
				else cp->ram_AM = A24_AM;
			}

			isc->vt_ram = (int)log_ram_addr;
			cp->ram_AM |= RAM_ENABLE;
			break;
		case SET_RAM_AM:
			/* error check?? - if not in A16 space, can't set to A16? */
			cp->ram_AM = (cp->ram_AM & ~RAM_AM_MASK) | *arg_ptr;
			break;
		case SET_DATA_WIDTH:
			if ((*arg_ptr != BYTE_DMA) && (*arg_ptr != WORD_DMA) && (*arg_ptr != LONG_DMA))
				{ return_value = EINVAL;
				  break;  }
			isc->dma_data_mode = *arg_ptr;
			break;
		case SET_BR_LINE:
			if ((*arg_ptr != SET_BR0) && (*arg_ptr != SET_BR1) && (*arg_ptr != SET_BR2)
				&& (*arg_ptr != SET_BR3))
				{ return_value = EINVAL; break;  }
			cp->control = (cp->control & ~BR_MASK) | *arg_ptr;
			break;
		case SET_TFR_TYPE:
			switch (*arg_ptr)
			{
				case DMA_TRANSFER:
					isc->transfer = DMA_TFR;
					break;
				case FHS_TRANSFER:
					isc->transfer = FHS_TFR;
					break;
				default:
					return_value = EINVAL;
			}
			break;
		case SET_USE_IOMAP:
			if (*arg_ptr)
				isc->use_iomap = VTEST_USE_IOMAP;
			else isc->use_iomap = VTEST_NO_IOMAP;
			break;
		case SET_USE_COMPAT:
			if (*arg_ptr)
				isc->tfr_control = VTEST_USE_BUILDCH;
			else isc->tfr_control = VTEST_USE_SETUP;
			break;
		case SET_ROR:
			cp->control |= ROR;
			break;
		case SET_RWD:
			cp->control &= ~ROR;
			break;
		case SET_RMW:
			cp->control |= RMW;
			break;
		case INTERRUPT:
			pri = spl6();
			cp->assert_intr &= ~CLEAR_INT0;
			sleep(&(cp->assert_intr), PRIBIO);
			splx(pri);
			break;
		case SET_INT_LVL:
			if (*arg_ptr < 1 || *arg_ptr > 6)
			{
				return_value = EINVAL;
				break;
			}
			cp->int0 = (cp->int0 & ~INT_LVL_MASK) | *arg_ptr;
			cp->int1 = (cp->int1 & ~INT_LVL_MASK) | *arg_ptr;
			cp->int2 = (cp->int2 & ~INT_LVL_MASK) | *arg_ptr;
			cp->int3 = (cp->int3 & ~INT_LVL_MASK) | *arg_ptr;
			break;
		case VTEST_RESET:
			/* check to see if need to cleanup any previous map_mem_to_host()s */
			/* get old address */
			old_ram_address = (cp->ram_addr << 16) + ((cp->low_ram_bit &
						LOW_RAM_ADDR) ? 0x8000 : 0);

			if ((old_ram_address >= 0x900000) || 
			((struct vme_bus_info *)(isc->bus_info->spec_bus_info))->vme_expander == IS_MTV)  {
				if (old_ram_address != phys_cp +0x8000) /* if 2nd 1/2 card, don't umap */
					unmap_mem_from_host(isc, isc->vt_ram, RAM_SIZE);
			}

			/* clean up done, reset it */
			cp->reset = 1;
			/* convert ram selectcode into ram address and set ram location registers*/
			ram_sc = m_busaddr(dev);
			if ((log_ram_addr = vt_ram_locate(isc, ram_sc, cp, phys_cp)) == (caddr_t)0)
				return(ENXIO);

			/* initialize card registers */
			vt_card_init(cp);

			/* get some interrupt vectors */
			cp->intvec0 = isc->vec_num1;
			cp->intvec1 = isc->vec_num1;

			cp->intvec2 = isc->vec_num2;
			cp->intvec3 = isc->vec_num2;

		        /*set up isc entry */
			isc->int_lvl = INT_LVL;
			isc->vt_ram = (int)log_ram_addr;
			isc->transfer = DMA_TFR;
			isc->tfr_control = VTEST_USE_BUILDCH;
			isc->use_iomap = VTEST_NO_IOMAP;
			isc->dma_data_mode = BYTE_DMA;          /* default = D8 DMA ?? */

			cp->int0 |= INT_ENABLE;
			cp->int1 |= INT_ENABLE;
			cp->int2 |= INT_ENABLE;
			/* int3 is for DMA completion and is not enabled until DMA is started */
			break;
		default:
			/* unrecognized ioctl */
			return_value = EINVAL;
		}	/* end switch */

	isc->owner = NULL;
	wakeup(isc);
	return(return_value);
}


/*-------------------------------------------------------------------------*/
vtest_dma_resid(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	struct vmetestregs *cp = (struct vmetestregs *) isc->card_ptr;

	return cp->tfr_count;
}


/*-------------------------------------------------------------------------*/
vtest_wakeup(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	wakeup(isc->owner->b_s2);
}
