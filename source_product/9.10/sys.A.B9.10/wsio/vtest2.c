/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/vtest2.c,v $
 * $Revision: 1.2.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:35:57 $
 */

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/io.h"
#include "../wsio/vme2.h"
#include "../wsio/vtest2.h"

#ifdef THIS_IS_A_PLACE_HOLDER_FILE

/*
 * externals
 */
extern struct isc_table_type *vme_init_isc();
extern caddr_t map_mem_to_host();


/* #define DEBUG */

/* 
 * internals
 */
#define DEV_CP(dev) ((struct vtest2regs *)isc_table[m_selcode(dev)]->card_ptr)
#define ISC_CP(isc) ((struct vtest2regs *)isc->card_ptr)
#define ISC_C_INFO(isc) ((struct vtest2_card_info *)isc->if_drv_data)

int vtest2_isr();
int vtest2_dma_isr();
int vtest2_wakeup();

#ifdef DEBUG

unsigned char dbuffer[32*1024];

/*-------------------------------------------------------------------------*/
get_card_memory(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	int i;
	struct vtest2_card_info *c_info = ISC_C_INFO(isc);

	for (i=0; i < 32*1024; i++)
		dbuffer[i] = *((unsigned char *)((int)c_info->ram_virtual + i));
}
#endif

/*-------------------------------------------------------------------------*/
vt2_card_init(cp)
/*-------------------------------------------------------------------------*/
/* Initialize card registers to power-up defaults. This routine is called 
 * by vtest2_init. 			   
 */

struct vtest2regs *cp;
{
	/* initialize card registers */
	cp->assert_intr |= CLEAR_INT0 | CLEAR_INT1 | CLEAR_INT2; /* clear interrupts */
	cp->ram_AM |= RAM_ENABLE;		/* enable RAM */

	cp->dest_addr = 0;			/* clear destination address */
	cp->control = 0;
	cp->dest_AM = A32_AM | VDMA_DISABLE;	/* A32 DMA, clear data mode bits */

	cp->int0 = INT_LVL;			/* set int lvl, 0 other bits */
	cp->int1 = INT_LVL; 			/* set int lvl, 0 other bits */
	cp->int2 = INT_LVL; 			/* set int lvl, 0 other bits */
	cp->int3 = INT_LVL; 			/* set int lvl, 0 other bits */
}


/*-------------------------------------------------------------------------*/
vtest2_init(dev)
/*-------------------------------------------------------------------------*/
/*
 * dev is of form 0xMMSCabde 
 *     where MM is major number
 *     where SC is index into isc_table and S=5, C={0-f}
 *     where abde is rest of minor number.
 *
 * test card switches are set like this:
 *         SW1               SW2              SW3
 *  +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+
 *  |8l7|6|5|4|3|2|1| |8|7|6|5|4|3|2|1| |8|7|6|5|4|3|2|1| 
 *  +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+ +-+-+-+-+-+-+-+-+
 *   3 3 2 2 2 2 2 2   2 2 2 2 1 1 1 1   1 1 1 1 1 1 0 0   0 0 0 0 0 0 0 0
 *   1 0 9 8 7 6 5 4   3 2 1 0 9 8 7 6   5 4 3 2 1 0 9 8   7 6 5 4 3 2 1 0
 *      a       b         C       d         e       0         0       0 
 *                   |     A24 Page Number      |   offset into a page    |
 *  |              A32 Page Number              |   offset into a page    |
 *
 */

dev_t dev;
{

	struct vtest2regs *cp;
	struct isc_table_type *isc;
	struct vtest2_card_info *c_info;
	int vme_addr;
#ifdef DEBUG
	int vme_addr1;
#endif


	/*
	 * get the required isc entry
	 */
	if ((isc = vme_init_isc(m_selcode(dev))) == NULL)
		return(ENXIO);

	/*
	 * generate the VME address
	 */

	/*              ab                      C                    de        */
	vme_addr = ((dev & 0xff00)<<16) | ((dev & 0xf0000)<<4) | ((dev & 0xff)<<12);


	/*
	 * get the corresponding PA virtual address
	 */
	if ((cp = (struct vtest2regs *)map_mem_to_host(isc, vme_addr, 256)) == NULL) {
		free_isc(isc);
		return(ENOMEM);
	}

#ifdef DEBUG
	vme_addr1 = kvtophys((caddr_t)cp);
#endif

	/*
	 * probe for the card, start out passive and then go active
	 */
	if (io_testr(isc,(caddr_t)&cp->status, BYTE_WIDE) <= 0 && 
	    io_testr(isc,(caddr_t)cp, LONG_WIDE) <= 0 &&
	    io_testr(isc,(caddr_t)&cp->int0, BYTE_WIDE) <= 0 && 
	    io_testr(isc,(caddr_t)&cp->intvec3,BYTE_WIDE) <= 0 && 
	    io_testr(isc,(caddr_t)&cp->reset,BYTE_WIDE) <= 0 && 

	    io_testw(isc,(caddr_t)&cp->reset,BYTE_WIDE) <= 0 &&  /* this will reset card */
	    io_testw(isc,(caddr_t)&cp->bus_error, BYTE_WIDE) == 0 &&
	    io_testw(isc,(caddr_t)cp, LONG_WIDE) <= 0) {
		/* return mapping if not vtest2 card */
		unmap_mem_from_host(isc, cp, 256);
		free_isc(isc);
		return(ENXIO);
	}

#ifdef NOT_NOW
	if (cp->status != 0 || cp->int0 != 0 || cp->intvec3 != 0x0f) {
		/* return mapping if not vtest2 card */
		unmap_mem_from_host(isc, cp, 256);
		free_isc(isc);
		return(ENXIO);
	}
#endif


	/* 
	 * reset card to power on state, interupts will be disabled 
	 */
	cp->reset = 1;                      

	/* 
	 * setup an area for each cards parameters
	 */
	c_info=(struct vtest2_card_info *)io_malloc(sizeof(struct vtest2_card_info), IOM_WAITOK);
	if ((isc->if_drv_data = (caddr_t)c_info) == NULL) {
		unmap_mem_from_host(isc, cp, 256);
		free_isc(isc);
		return(ENOMEM);
	}
		
	/*
	 * generate the ram physical address, ram has 32K resolution for mapping
	 */
	c_info->ram_physical = (caddr_t)((int)vme_addr + 32*1024);
		

	/*
	 * map in the cards memory, this can be changed later by ioctl
	 */
	if ((c_info->ram_virtual = map_mem_to_host(isc, c_info->ram_physical, 32*1024)) == NULL) {
		unmap_mem_from_host(isc, cp, 256);
		io_free((caddr_t)c_info);
		free_isc(isc);
		return(ENXIO);
	}


	/*
	 * set the card memory up to recognize this address
	 */
	cp->ram_addr    = (((int)c_info->ram_physical & 0xffff0000) >> 16);
	cp->low_ram_bit = (((int)c_info->ram_physical & 0x00008000) >>  8);

#ifdef DEBUG
	printf("Phys Addr  : %08x\n", c_info->ram_physical);
	printf("Ram Addr   : %04x\n", cp->ram_addr);
	printf("Ram Low Bit: %02x\n", cp->low_ram_bit);
#endif

	if (A16_ADDR((int)c_info->ram_physical))       cp->ram_AM = A16_AM;
	else if (A24_ADDR((int)c_info->ram_physical))  cp->ram_AM = A24_AM; 
	else if (A32_ADDR((int)c_info->ram_physical))  cp->ram_AM = A32_AM;
	else {
		cp->reset = 1;                      
		unmap_mem_from_host(isc, c_info->ram_virtual, 32*1024);
		unmap_mem_from_host(isc, cp, 256);
		io_free((caddr_t)c_info);
		free_isc(isc);
		return(EINVAL);
	}
	
	/* 
	 * initialize card registers 
	 */
	vt2_card_init(cp);


	/*
	 * defensive programming
	 */
	if (io_testr(isc, (caddr_t)&c_info->ram_virtual, LONG_WIDE) <= 0 &&
	    io_testr(isc, (caddr_t)((int)&c_info->ram_virtual + 32*1024 -1), LONG_WIDE) <= 0) {
		cp->reset = 1;                      
		unmap_mem_from_host(isc, c_info->ram_virtual, 32*1024);
		unmap_mem_from_host(isc, cp, 256);
		io_free((caddr_t)c_info);
		free_isc(isc);
		return(EINVAL);
	}


	/*
	 * associate the isr's with available vectors.
	 *    - take any available vector
	 *    - argument to ISR will be status/ID
	 *    - execute at VME level handler priority
	 */
	if ((c_info->vec_num1 = io_isrlink(isc, vtest2_isr, 0, 0, 0)) < 0 ||
	    (c_info->vec_num2 = io_isrlink(isc, vtest2_dma_isr, 0, 0, 0)) < 0) {
		cp->reset = 1;                      
		unmap_mem_from_host(isc, c_info->ram_virtual, 32*1024);
		unmap_mem_from_host(isc, cp, 256);
		io_free((caddr_t)c_info);
		free_isc(isc);
		return(ENXIO);
	}


	/*
	 * program card with obtained status/ID's
	 */
	cp->intvec0 = c_info->vec_num1;
	cp->intvec1 = c_info->vec_num1;
	cp->intvec2 = c_info->vec_num2;
	cp->intvec3 = c_info->vec_num2;


	/*
	 * save the pointer to the card
	 */
	isc->card_ptr = (int *)cp;


	/*
	 * setup some default DMA characteristics
	 */
	c_info->transfer = VDMA_TFR;
	c_info->tfr_control = VTEST_USE_BUILDCH;
	c_info->use_iomap = VTEST_NO_IOMAP;
	c_info->dma_data_mode = BYTE_DMA;

	return(0);
}
	


/*-------------------------------------------------------------------------*/
vtest2_open(dev, flag)
/*-------------------------------------------------------------------------*/
/* Open test card. 
 *
 */

dev_t dev;
int flag;
{
	struct vtest2regs *cp;
	int ret;

	/*
	 * see if card was previously initialized, if not do it
	 */
	if (isc_table[m_selcode(dev)] == NULL) 
		if (ret = vtest2_init(dev))
			return(ret);

	/* 
	 * enable interrupts 
	 */
	cp = DEV_CP(dev);
	cp->int0 |= INT_ENABLE;
	cp->int1 |= INT_ENABLE;
	cp->int2 |= INT_ENABLE;

	return(0);
}



/*-------------------------------------------------------------------------*/
vtest2_close(dev, flag)
/*-------------------------------------------------------------------------*/

dev_t dev;
int flag;
{
	struct vtest2regs *cp = DEV_CP(dev);

	/* 
	 * disable interrupts 
	 */
	cp->int0 &= ~INT_ENABLE;
	cp->int1 &= ~INT_ENABLE;
	cp->int2 &= ~INT_ENABLE;

	return(0);
}



/*-------------------------------------------------------------------------*/
vtest2_dma_setup(isc)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	struct vtest2regs *cp = ISC_CP(isc);
	struct dma_parms *dma_parms = (struct dma_parms *)isc->dma_parms;
	int idx = dma_parms->chain_index;

	cp->dest_addr = (long)dma_parms->chain_ptr[idx].phys_addr;
	cp->tfr_count = (long)dma_parms->chain_ptr[idx].count;
	dma_parms->chain_index++;

	cp->control |= VDMA_ENABLE;
	cp->int3 |= INT_ENABLE;
}



/*-------------------------------------------------------------------------*/
vtest2_dma_start(isc)
/*-------------------------------------------------------------------------*/

register struct isc_table_type *isc;
{
	struct vtest2regs *cp = ISC_CP(isc);
	struct vtest2_card_info *c_info = ISC_C_INFO(isc);

	cp->dest_AM |= c_info->dma_data_mode << 6;
}



/*-------------------------------------------------------------------------*/
vtest2_strategy(bp)
/*-------------------------------------------------------------------------*/

register struct buf *bp;
{
	struct isc_table_type *isc = isc_table[m_selcode(bp->b_dev)];
	struct vtest2_card_info *c_info = ISC_C_INFO(isc);
	struct vtest2regs *cp = DEV_CP(bp->b_dev);
	struct dma_parms *dma_parms;
	int ret;
	int spl;
#ifdef DEBUG
	caddr_t addr1;
#endif


	/*
	 * sanity check, only one call at a time allowed
	 */
	if (isc->owner) {
		iodone(bp);
		return(EIO);
	}
	isc->owner = bp;


	/*
	 * get and set up a dma_parms structure
	 */
	dma_parms = (struct dma_parms *)io_malloc(sizeof(struct dma_parms), IOM_WAITOK);
	if (!dma_parms) 
		return(ENOMEM);

	bzero(dma_parms, sizeof(struct dma_parms));
		

	/*
	 * set up the options required for the transfer
	 */
	if (bp->b_flags & B_READ) {
		dma_parms->dma_options |= VDMA_READ;  dma_parms->dma_options &= ~VDMA_WRITE;
		cp->control &= ~VDMA_WRITE;
	} else {
		dma_parms->dma_options |= VDMA_WRITE;  dma_parms->dma_options &= ~VDMA_READ;
		cp->control |= VDMA_WRITE;
	}
	

	/*
	 * set up the rest of the dma transfer parameters
	 */
	dma_parms->addr = bp->b_un.b_addr;
	dma_parms->count = bp->b_bcount;


	dma_parms->drv_routine = vtest2_wakeup;
	dma_parms->drv_arg = (int)dma_parms;

	if (c_info->use_iomap != VTEST_NO_IOMAP) 
		dma_parms->dma_options |= VME_USE_IOMAP;

	isc->dma_parms = dma_parms;


	/*
	 * call VME services to set up the DMA
	 */
	spl=spl6();
	while (ret = vme_dma_setup(isc, dma_parms)) {
		if (ret == RESOURCE_UNAVAILABLE)
			sleep(isc, PZERO+2);
		else if (ret < 0) {
			/*
			 * there was an error
			 */
			splx(spl);
			io_free((caddr_t)dma_parms);

			bp->b_error = EINVAL;  bp->b_flags |= B_ERROR;		
			iodone(bp);
			isc->owner = NULL;

			return(EIO);
		} else break;
	}
	splx(spl);

#ifdef DEBUG
	addr1 = kvtophys((caddr_t)dma_parms->addr);
#endif

	/*
	 * start the dma transfer
	 */
	vtest2_dma_setup(isc);
	vtest2_dma_start(isc);

	return(0);
}



/*-------------------------------------------------------------------------*/
vtest2_read(dev, uio)
/*-------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	struct vtest2regs *cp = DEV_CP(dev);

	/* 
	 * card will not do RMW cycle on a read 
	 */
	if (cp->status & RMW) return(EIO);	

	return(physio(vtest2_strategy, NULL, dev, B_READ, minphys, uio));
}



/*-------------------------------------------------------------------------*/
vtest2_write(dev,uio)
/*-------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	return(physio(vtest2_strategy, NULL, dev, B_WRITE, minphys, uio));
}



/*-------------------------------------------------------------------------*/
vtest2_wakeup(dma_parms)
/*-------------------------------------------------------------------------*/

struct dma_parms *dma_parms;
{
	wakeup(dma_parms);
}



/*-------------------------------------------------------------------------*/
vtest2_isr(isc, arg)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
int arg;
{
	struct vtest2regs *cp = ISC_CP(isc);

	cp->assert_intr |= (CLEAR_INT0 | CLEAR_INT1 | CLEAR_INT2);
	wakeup(&(cp->assert_intr));
}



/*-------------------------------------------------------------------------*/
vtest2_dma_isr(isc, arg)
/*-------------------------------------------------------------------------*/

struct isc_table_type *isc;
int arg;
{

	struct vtest2regs *cp = ISC_CP(isc);
	struct buf *bp = isc->owner;
	struct dma_parms *dp = isc->dma_parms;

	/* 
	 * disable interrupt 
	 */
	cp->int3 &= ~INT_ENABLE;
	cp->control &= ~VDMA_ENABLE;
	cp->dest_AM &= ~VDMA_DATA_MASK;

	/*
	 * Was there a bus error
	 */
	if (cp->status & BERR) {
		cp->control &= ~BERR;
		bp->b_error |= EIO;
	} else {

		/*
		 * determine if DMA completed
		 */
		if (cp->tfr_count)	{

			/*
			 * DMA did not complete, calculate resid
			 */
			int idx;

			bp->b_resid += cp->tfr_count;

			for (idx = dp->chain_index+1; idx < dp->chain_count; idx++)
				bp->b_resid += dp->chain_ptr[idx].count;

		} else if (dp->chain_index != dp->chain_count) {
#ifdef DEBUG
			printf("Doing Chain: %d\n", dp->chain_index);
			get_card_memory(isc);
#endif

			/*
			 * more elements to do
			 */
			 vtest2_dma_setup(isc);
			 vtest2_dma_start(isc);

			 return;
		}
	}


#ifdef DEBUG
	get_card_memory(isc);
#endif

	/*
	 * must always call cleanup for a setup
	 */
	vme_dma_cleanup(isc, dp);
	
	/*
	 * give back allocated memory
	 */
	io_free(dp);

	isc->owner = NULL;

	/*
	 * wakeup the process: the io_wait(bp) was called by physio()
	 */
	iodone(bp);

}


/*-------------------------------------------------------------------------*/
vtest2_ioctl(dev, cmd, arg)
/*-------------------------------------------------------------------------*/

dev_t dev;
int cmd;
int *arg;
{
	struct isc_table_type *isc = isc_table[m_selcode(dev)];	
	struct vtest2_card_info *c_info = ISC_C_INFO(isc);
	struct vtest2regs *cp = ISC_CP(isc);
	int ret = 0;
	int idx;

	switch (cmd) {
		case READ_TEST:
			for (idx=0; idx < 32*1024; idx++)
				*((unsigned char *)((int)c_info->ram_virtual + idx)) = 
				    *(unsigned char *)arg;
			ret = 0;
			break;

		default:
			ret = 0;
	}

#ifdef DEBUG
	get_card_memory(isc);
#endif
	return(ret);
}
#endif /* PLACE HOLDER */
