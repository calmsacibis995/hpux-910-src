/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/etest.c,v $
 * $Revision: 1.7.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:28:48 $
 */

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/buf.h"
#include "../wsio/eisa.h"
#include "../wsio/eeprom.h"
#include "../h/hpibio.h"
#include "../h/param.h"
#include "../h/malloc.h"
#include "../wsio/etest.h"

extern caddr_t kmem_alloc();
extern int (*eisa_attach)();
extern caddr_t map_isa_address();
extern caddr_t map_mem_to_host();
extern struct isc_table_type *get_new_isc();
extern int eisa_reinit;
#if defined (__hp9000s700) && defined (_WSIO)
extern int iomap_estimate;
extern unsigned int eisa_io_estimate;
extern unsigned int vme_io_estimate;
#endif

int (*etest_saved_attach)();
int etest_attach();
int etest_init();
int etest_strategy();
int etest_wakeup();
int etest_notify();
int etest_isr();

/* need an ident string for backpatch identification */
static char etest_revision[] = "$Header: etest.c,v 1.7.83.3 93/09/17 20:28:48 kcs Exp $";

static int etest_mailbox = 0;
static char *buffer;
int etest_eisa_reinit_test = 0; /* gross, but adb needs to find this for the test */


/*----------------------------------------------------------------------------*/
etest_link() 
/*----------------------------------------------------------------------------*/
/* Link in attach routine. This routine called by kernel_initialize.          
 */
{
	etest_saved_attach = eisa_attach;
	eisa_attach = etest_attach;
}


/*----------------------------------------------------------------------------*/
etest_init(isc) 
/*----------------------------------------------------------------------------*/
/* A test init routine. 
 */

struct isc_table_type *isc;
{
	struct etest_card_info *card_info;

	card_info = (struct etest_card_info *)isc->if_drv_data;

	if (card_info->card_initialized++)
		printf("ETEST: Test card has been initialized %d times\n",
			card_info->card_initialized);

	if (eisa_reinit)
		printf("ETEST: Init Ran: eisa_reinit is true.\n");

#if defined (__hp9000s700) && defined (_WSIO)
	printf("iomap_estimate  : %d pages\n", iomap_estimate);
	printf("eisa_io_estimate: %d pages\n", eisa_io_estimate);
	printf("vme_io_estimate : %d pages\n", vme_io_estimate);
#endif
}


/*----------------------------------------------------------------------------*/
initialize_test_card(isc)
/*----------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	struct etest_reg *etest_reg;
	struct etest_card_info *card_info;
	struct dma_parms *dma_parms;
	struct bmic_reg *bmic_reg;

	etest_reg = (struct etest_reg *)isc->if_reg_ptr;
	card_info = (struct etest_card_info *)isc->if_drv_data;
	dma_parms = (struct dma_parms *)isc->my_dma_parms;

	/* 
	 * init all card registers to powerup state 
	 */
	outb(&etest_reg->isa_waitstates, E_ISA_16BIT);	/* no waits, 16bit isa tfrs */
	outb(&etest_reg->eisa_waitstates, 0xc0);        /* no waits, 32bit eisa burst tfrs */
	outb(&etest_reg->dma_control, 0);               /* init'ed at DMA time */
	outb(&etest_reg->dma_ch_enable, 0);             /* init'ed at DMA time */
	outb(&etest_reg->more_dma_control, 0);          /* signal defaults, not ISA master */
	outb(&etest_reg->isa_dma_cycles, 0);
	outb(&etest_reg->isa_dma_odd, 0);
	outb(&etest_reg->dma_tc, 0);
	outb(&etest_reg->dma_done, 0xef);

	card_info->max_block_xfer_len = 64;
	outb(&etest_reg->dma_burst_length, card_info->max_block_xfer_len);

	/* 0x89 - address of RAM in ISA 20-bit space - 0-31 */
	outb(&etest_reg->isa20_select,0);	       

	/* 0x8a - address of RAM in ISA 24-bit space - 16-127 */ 
	outb(&etest_reg->isa24_select,0);

	/*
	 * we need to map in the BMIC register and ISA high byte address for use
	 */
	if (card_info->virt_bmic_ptr != NULL) {
		etest_reg->io_select &= ~E_IOSEL_ENABLE;
		unmap_mem_from_host(isc, card_info->virt_bmic_ptr, NBPG);
		card_info->virt_bmic_ptr = NULL;
	}

	outb(&etest_reg->io_select,((int)(card_info->etd->port_loc)/E_ISA_IO_CHUNK)|E_IOSEL_ENABLE);
	card_info->phys_bmic_ptr = (caddr_t)card_info->etd->port_loc;
	if ((card_info->virt_bmic_ptr = map_isa_address(isc, card_info->etd->port_loc)) == NULL) {
		printf("\n    ETEST WARNING: Unable to map BMIC registers. Card Ignored");
		return(-1);
	}

	/*
	 * EISA memory  address  space must be mapped in so that dma may
	 * be "setup"  by  performing  reads from an  address.  All test
	 * cards will be mapped in on top of each  other so that 4 cards
	 * will fit in 16M of  space.  When the  actual  read is to take
	 * place, priority is raised to ensure autonomous execution, the
	 * cards  memory  space is enabled,  the address  read, the card
	 * disabled and the priority  dropped.  Note:  I am only mapping
	 * in 2 pages, 8K since that is all that is needed to access all
	 * of memory.
	 */
	if (card_info->virt_eisa_ptr != NULL) {
		etest_reg->eisa_select &= ~E_EMEMSEL_ENABLE;
		unmap_mem_from_host(isc, card_info->virt_eisa_ptr, 2*NBPG);
	}

	outb(&etest_reg->eisa_select, ((int)(card_info->etd->mem_loc) / E_EISA_CHUNK));
	etest_reg->eisa_select &= ~E_EMEMSEL_ENABLE;
	card_info->phys_eisa_ptr = (caddr_t)card_info->etd->mem_loc;
	if ((card_info->virt_eisa_ptr=map_mem_to_host(isc,card_info->etd->mem_loc,2*NBPG)) == NULL){
		printf("\n    ETEST WARNING: Unable to map in EISA memory. Card Ignored");
		return(-1);
	}

	/* 
	 * initialize the DMA default defaults
	 */
	dma_parms->dma_options = DMA_BURST | DMA_32BYTE | DMA_DEMAND;

	/* 
	 * bmic powerup init 
	 */
	outb(&etest_reg->eisa_control, ETEST_ENABLE);
	bmic_reg = (struct bmic_reg *)card_info->virt_bmic_ptr;
	bmic_reg->local_index = global_config;
	bmic_reg->local_data = 2;

	bmic_reg->local_index = ch0_status;
	bmic_reg->local_data = E_TFR_COMPLETE;

	/* 
	 * init misc card_info fields 
	 */
	card_info->flags = E_EISA_SLAVE;

	return(0);
}


/*----------------------------------------------------------------------------*/
etest_attach(id, isc)
/*----------------------------------------------------------------------------*/

int id;
struct isc_table_type *isc;
{
	struct etest_reg *etest_reg;
	struct etest_card_info *card_info;
	struct dma_parms *dma_parms;
	struct bmic_reg *bmic_reg;
	int slot, eid;
	char int_config;
	int i, pwr_up_fnd;
	struct eeprom_function_data func_data;
	struct eeprom_slot_data slot_data;
	int func_num;

#ifdef __hp9000s300
	/* 
	 * this is an eisa_config testing kludge. It is a quick way to 
	 * write a driver 
	 */
	if (id == cvt_ascii_to_eisa_id("HWP0C90")) {
		printf("Pseudo VIRGA Driver is Installed\n");
		((struct eisa_if_info *)(isc->if_info))->flags |= INITIALIZED;
		return((*etest_saved_attach)(id, isc)); 
	}
#endif
	/* strip revision from id */
	eid = id & 0xfffffff0; 

	/*
	 * see if it is in fact one or another variations of test card
	 */
	if (eid == ETEST_ID || eid == ETEST_ID_OLD) {
		/* eisa_config testing with reinit stuff */
		if (etest_eisa_reinit_test) {
			if (eisa_reinit) {
				((struct eisa_if_info *)(isc->if_info))->flags |= INIT_ERROR;
				return((*etest_saved_attach)(id, isc)); 
			}
		}

		etest_reg = (struct etest_reg *)isc->if_reg_ptr;
		slot = ((struct eisa_if_info *)(isc->if_info))->slot;

#ifdef EISA_TESTS
		/* This check verifies that port init data has been entered correctly */
		if (inl((int *)((int)etest_reg + 0x88L)) != 0x5a5b1c5d)
			printf("\n    ETEST slot %d Port Init Data Error", slot);
		else
			printf("\n    ETEST slot %d Port Init Data PASSED", slot);
#endif

		/*
		 * this code is for  verifying  that early  powerup code
		 * correctly  knows  that eisa  cards  are  present.  In
		 * other words, if we get here via either power up init,
		 * or  eisa_config  automatic mode reinit, and this flag
		 * is not set, there is a problem.
		 */ 

		/* 
		 * get slot data from eeprom 
		 */
		if (read_eisa_slot_data(slot, &slot_data) < 0) {
			((struct eisa_if_info *)(isc->if_info))->flags |= INIT_ERROR;
			return((*etest_saved_attach)(id, isc)); 
		}

		/* 
		 * allocate needed data structures 
		 */
#ifdef OLD_WAY
		isc->if_drv_data = kmem_alloc(sizeof(struct etest_card_info));
#else
		isc->if_drv_data = io_malloc(sizeof(struct etest_card_info), IOM_WAITOK);
#endif
		card_info = (struct etest_card_info *)isc->if_drv_data;
		bzero(card_info, sizeof(struct etest_card_info));

#ifdef OLD_WAY
		card_info->etd= (struct etest_func_data*)kmem_alloc(sizeof(struct etest_func_data));
#else
		card_info->etd= (struct etest_func_data*)
		    io_malloc(sizeof(struct etest_func_data), IOM_WAITOK);
#endif
		bzero(card_info->etd, sizeof(struct etest_func_data));

#ifdef OLD_WAY
		isc->my_dma_parms = (int *)kmem_alloc(sizeof(struct dma_parms));
#else
		isc->my_dma_parms = (int *)io_malloc(sizeof(struct dma_parms), IOM_WAITOK);
#endif
		dma_parms = (struct dma_parms *)isc->my_dma_parms;


		/* 
		 * get function data from the eeprom, store it and later use it. 
		 */
		for (func_num = 0; func_num < slot_data.number_of_functions; func_num++) {
			if (read_eisa_function_data(slot, func_num, &func_data)) {
				printf("\n    ETEST: Cannot read EISA test card function data.");
			} else {

				/*
				 * the test card should use all available interrupts
				 */
				if (func_data.function_info & HAS_IRQ_ENTRY) {  
					for (i = 0; i < MAX_INT_ENTRIES; i++) {
						int_config = func_data.intr_cfg[i][0];
						/* set level indication for this interrupt level */
						if (int_config & INT_LEVEL_TRIG) 
							card_info->etd->lvl_ints |= 
								(1 << (int_config & INT_IRQ_MASK));
						else
							card_info->etd->edge_ints |= 
								(1 << (int_config & INT_IRQ_MASK));

						if ((int_config & INT_MORE_ENTRIES) == 0)
							break;
					}
				}

				/*
				 * the test card should use all available dma channels
				 */
				if (func_data.function_info & HAS_DMA_ENTRY) { 
					for (i=0; i < MAX_DMA_ENTRIES; i++) {
						card_info->etd->dma_chans |= 
						(1<<(func_data.dma_cfg[i][0] & DMA_CHANNEL_MASK));

						if (!(func_data.dma_cfg[i][0] & DMA_MORE_ENTRIES)) 
							break;
					}
				}

				/* 
				 * the test card has only one memory location: EISA Memory 
				 */
				if (func_data.function_info & HAS_MEMORY_ENTRY) { 
					card_info->etd->mem_loc = ((func_data.mem_cfg[0][4] << 16)+
						(func_data.mem_cfg[0][3] << 8) +
						func_data.mem_cfg[0][2]) * 0x100;
				}

				/* 
				 * the test card has only one port entry: BMIC Registers 
				 */
				if (func_data.function_info & HAS_PORT_ENTRY) {
					card_info->etd->port_loc = (func_data.port_cfg[0][2] << 8)+
						func_data.port_cfg[0][1];
					}


			} /* function data read succeed */
		} /* interate over number of functions present */

		/* 
		 * see if we have the information necessary to continue. If not
		 * give a meaningful message and blow chow.
		 */
		if ((card_info->etd->lvl_ints == 0 && card_info->etd->edge_ints == 0) ||
			card_info->etd->dma_chans == 0 ||
			card_info->etd->mem_loc   == 0 ||
			card_info->etd->port_loc  == 0) {

			printf("\n    ETEST: EEPROM Information Incorrect");
			printf("\n           Interrupts: lvl %0x edge %0x", 
				card_info->etd->lvl_ints, card_info->etd->edge_ints);
			printf("\n           DMA Chans : %0x",card_info->etd->dma_chans);
			printf("\n           Memory Loc: %0x",card_info->etd->mem_loc);
			printf("\n           Port Loc  : %0x\n",card_info->etd->port_loc);

#ifdef OLD_WAY
			kmem_free(card_info->etd, sizeof(struct etest_func_data));
			kmem_free(isc->if_drv_data,sizeof(struct etest_card_info));
			kmem_free(isc->my_dma_parms,sizeof(struct dma_parms));
#else
			io_free(card_info->etd, sizeof(struct etest_func_data));
			io_free(isc->if_drv_data,sizeof(struct etest_card_info));
			io_free(isc->my_dma_parms,sizeof(struct dma_parms));
#endif

			((struct eisa_if_info *)(isc->if_info))->flags |= INIT_ERROR;
			return((*etest_saved_attach)(id, isc)); 
		}


		/* 
		 * initialize interrupts and DMA 
		 */

		/* search for interrrupts */
		for (i=0; i < NUM_IRQ_PER_BUS; i++) {
			if ((card_info->etd->lvl_ints & (1<<i)) && (i>=3) && (i!=8) && (i!=13)) {
				if (card_info->etest_irq == 0) {
#ifdef EISA_TESTS
					printf("\n    ETEST: Interrupt Set: level, %d",i);
#endif
					card_info->etest_irq = i;
					outs(&etest_reg->level_irqs, 1 << i);
				}
				eisa_isrlink(isc, etest_isr, i, 0);
				
			} 
			if ((card_info->etd->edge_ints & (1<<i)) && (i>=3) && (i!=8) && (i!=13)) {
				if (card_info->etest_irq == 0) {
#ifdef EISA_TESTS
					printf("\n    ETEST: Interrupt Set: edge, %d",i);
#endif
					card_info->etest_irq = i;
					outs(&etest_reg->edge_irqs, 1 << i);
				}	
				eisa_isrlink(isc, etest_isr, i, 0);
			}
		} /* interrupt search loop */

		/* set powerup dma */
		for (i=0; i < NUM_DMA_CH_PER_BUS; i++) {
			if ((card_info->etd->dma_chans & (1<<i)) && (i!=4)) {
#ifdef EISA_TESTS
				printf("\n    ETEST: DMA Channel Set: %d\n",i);
#endif
				dma_parms->channel = i;
				break;
			}
		} /* dma search loop */

		/* do the generic initialization */
		if (initialize_test_card(isc) < 0) {
#ifdef OLD_WAY
			kmem_free(card_info->etd, sizeof(struct etest_func_data));
			kmem_free(isc->if_drv_data,sizeof(struct etest_card_info));
			kmem_free(isc->my_dma_parms,sizeof(struct dma_parms));
#else
			io_free(card_info->etd, sizeof(struct etest_func_data));
			io_free(isc->if_drv_data,sizeof(struct etest_card_info));
			io_free(isc->my_dma_parms,sizeof(struct dma_parms));
#endif

			((struct eisa_if_info *)(isc->if_info))->flags |= INIT_ERROR;
			return((*etest_saved_attach)(id, isc)); 
		}

		/* 
		 * set up init routine.
		 */
#ifdef OLD_WAY
		 isc->gfsw = (struct gfsw *)kmem_alloc(sizeof(struct gfsw));
#else
		 isc->gfsw = (struct gfsw *)io_malloc(sizeof(struct gfsw), IOM_WAITOK);
#endif
		 bzero(isc->gfsw, sizeof(struct gfsw));
		 isc->gfsw->init = etest_init;


		/* 
		 * enable card 
		 */	
		outb(&etest_reg->eisa_control, ETEST_ENABLE);

		((struct eisa_if_info *)(isc->if_info))->flags |= INITIALIZED;

		if (eid == ETEST_ID_OLD) 
			printf("EISA Test Card (OLD ID) Initialized\n");
		else
			printf("EISA Test Card Initialized\n");
	}

	return((*etest_saved_attach)(id, isc)); 
}


/*----------------------------------------------------------------------------*/
etest_open(dev, flags)
/*----------------------------------------------------------------------------*/
/* provides a mutually exclusive open. */

dev_t dev;
int flags;
{
	register struct isc_table_type *isc;
	register struct etest_card_info *card_info;

	get_isc(dev, isc);
	if (isc == NULL)
		return(ENODEV);

	card_info = (struct etest_card_info *)isc->if_drv_data;

	if (!card_info->card_opened) {
		card_info->card_opened = 1;
		return(0);
	} else
		return(EBUSY);
}


/*----------------------------------------------------------------------------*/
etest_close(dev)
/*----------------------------------------------------------------------------*/

dev_t dev;
{
	register struct isc_table_type *isc;
	register struct etest_card_info *card_info;

	get_isc(dev, isc);
	if (isc == NULL)
		return(ENODEV);

	card_info = (struct etest_card_info *)isc->if_drv_data;

	card_info->card_opened = 0;
	return(0);
}


/*----------------------------------------------------------------------------*/
etest_write(dev, uio)
/*----------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	return(physio(etest_strategy, NULL, dev, B_WRITE, minphys, uio));
}


/*----------------------------------------------------------------------------*/
etest_read(dev, uio)
/*----------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	return(physio(etest_strategy, NULL, dev, B_READ, minphys, uio));
}


/*----------------------------------------------------------------------------*/
et_slave_setup(isc)
/*----------------------------------------------------------------------------*/
/* this routine prepares for dma.  It uses eisa_dma_setup to build a dma
 * chain,  setup the iomap,  flush/purge  the cache,  and set up the dma
 * controller.  It then preps the test card for the  coming  action  and
 * fires it off.
 */

struct isc_table_type *isc;
{
	register struct dma_parms *isc_dma_parms;
	register struct dma_parms *dma_parms;
	register struct buf *bp;
	register struct etest_reg *etest_reg;
	register dma_options;
	int slot, dummy, ret, x;
	struct etest_card_info *card_info;

	card_info = (struct etest_card_info *)isc->if_drv_data;
	bp = isc->owner;
	isc_dma_parms = (struct dma_parms *)isc->my_dma_parms;
	etest_reg = (struct etest_reg *)isc->if_reg_ptr;
	dma_options = isc_dma_parms->dma_options;
	slot = ((struct eisa_if_info *)(isc->if_info))->slot;

#ifdef OLD_WAY
	if ((bp->b_s2 = (int)kmem_alloc(sizeof(struct dma_parms))) == NULL) {
#else
	if ((bp->b_s2 = (int)io_malloc(sizeof(struct dma_parms), IOM_WAITOK)) == NULL) {
#endif
		printf("ETEST WARNING: allocating memory for dma_parms structure failed\n");
		return -1;
	}
	dma_parms = (struct dma_parms *)bp->b_s2;
	dma_parms->dma_options = isc_dma_parms->dma_options;
	dma_parms->channel = isc_dma_parms->channel;

	/*
	 * eisa_dma_setup needs to know this information for handling buflets,
	 * flushes vs purges, and in the case of slave dma, setting up the dma 
	 * controller.
	 */
	if (bp->b_flags & B_READ) {
		dma_parms->dma_options |= DMA_READ;
		dma_parms->dma_options &= ~DMA_WRITE;
	} else {
		dma_parms->dma_options |= DMA_WRITE;
		dma_parms->dma_options &= ~DMA_READ;
	}
	
	dma_parms->flags = EISA_BYTE_ENABLE;
	dma_parms->drv_routine = etest_wakeup;
	dma_parms->drv_arg = (int)dma_parms;
	dma_parms->spaddr = bp->b_spaddr; 
	dma_parms->addr = bp->b_un.b_addr;
	dma_parms->count = bp->b_bcount;

	/*
	 * call eisa services dma setup
	 */
	x=spl6();
	while (ret = eisa_dma_setup(isc, dma_parms)) {
		if (ret == RESOURCE_UNAVAILABLE)  {
			isc->owner = NULL;
#ifdef EISA_TESTS
			printf("ETEST: eisa_dma_setup: RESOURCE_UNAVAILABLE, SLAVE sleeping: %d\n", slot);
			ch_stat(0x50 | (0xf & slot));
#endif
			sleep(bp->b_s2, PZERO+2);
			isc->owner = bp;
		} else if (ret < 0) {
			splx(x);
#ifdef OLD_WAY
			kmem_free(dma_parms, sizeof(struct dma_parms));
#else
			io_free(dma_parms, sizeof(struct dma_parms));
#endif
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			isc->owner = NULL;
			msg_printf("ETEST: eisa_dma_setup returned error %d\n", ret);
			return(-1);
		} else break;
	}
	splx(x);

	/* 
	 * initialize the test card based on dma_options values 
	 */
	outb(&etest_reg->dma_burst_length, 64);
	outb(&etest_reg->dma_burst_length, card_info->max_block_xfer_len);
	outb(&etest_reg->dma_control, 0);
	outb(&etest_reg->dma_done, 0xef);	/* clear DMA done bits for all channels (not 4) */
	if (dma_options & DMA_BURST) {
		etest_reg->eisa_waitstates |= E_EISA_BURST;
		etest_reg->dma_control |= E_DMA_BURST;
	} else {
		etest_reg->eisa_waitstates &= ~E_EISA_BURST;
		etest_reg->dma_control &= ~E_DMA_BURST;
	}
	etest_reg->dma_control &= ~E_DMA_SIZE_MASK;
	if (dma_options & DMA_8BYTE) {
		etest_reg->dma_control |= E_DMA_SIZE_BYTE;
#ifdef EISA_TESTS
		if (card_info->force_test_card_count) /* used to test resid calculations */
			outs(&etest_reg->dma_count, card_info->force_test_card_count);
		else
#endif
			outs(&etest_reg->dma_count, bp->b_bcount);
	} else if (dma_options & DMA_32BYTE) {
		etest_reg->dma_control |= E_DMA_SIZE_LONG;
		outs(&etest_reg->dma_count, (bp->b_bcount/4)+2);
	} else { /* one of the two 16 byte sizes */ 
		etest_reg->dma_control |= E_DMA_SIZE_WORD;
		outs(&etest_reg->dma_count, (bp->b_bcount/2)+2);
	}
	if (dma_options & DMA_BLOCK)
		etest_reg->dma_control &= ~E_DREQ_ON;
	else
		etest_reg->dma_control |= E_DREQ_ON;
	if (bp->b_flags & B_READ)
		etest_reg->dma_control &= ~E_DMA_WRITE;
	else
		etest_reg->dma_control |= E_DMA_WRITE;
	etest_reg->more_dma_control = 0;
		
	/* 
	 * read from test card RAM to initialize RAM address pointers.  the test
	 * card dmas into/from its ram based upon an address which increments by
	 * long words on each  transfer.  It is important  that if you want back
	 * what you put in, you set the pointer to point to the beginning again.
	 */
	x = spl6();
	etest_reg->eisa_select |= 0x80;
/*	snooze(5); */
	dummy = (*card_info->virt_eisa_ptr);
/*	snooze(5); */
	etest_reg->eisa_select &= 0x7f;
	splx(x);

	/* 
	 * write to card to start DMA transfer 
	 */
	etest_reg->dma_ch_enable |= 1 << dma_parms->channel;


	return(0);
}


/*----------------------------------------------------------------------------*/
load_isa_dma(isc)
/*----------------------------------------------------------------------------*/
/* this  routine  prepares  the test  card for an ISA dma  transfer.  It
 * handles  the  multi-elements   chains  returned  by   eisa_dma_setup.
 * bp->b_s6 contains the number of the chain element in processing.
 */

struct isc_table_type *isc;
{

	struct addr_chain_type *ch_ptr;
	struct buf *bp;
	struct etest_reg *etest_reg;
	struct dma_parms *dma_parms;
	int i;

	etest_reg = (struct etest_reg *)isc->if_reg_ptr;
	bp = isc->owner;

	dma_parms = (struct dma_parms *)bp->b_s2;

	/* one less element to do later */
	dma_parms->chain_count--;

	/* point to the correct element in chain */
	ch_ptr = dma_parms->chain_ptr;

	for (i=0; i < bp->b_s6; i++) 
		ch_ptr++;

	bp->b_s6++; 

	outs(&etest_reg->isa_dma_addr, (u_short)((int)(ch_ptr->phys_addr) & 0xffff));
	outb(&etest_reg->isa_dma_high_byte, (u_char)(((int)(ch_ptr->phys_addr) & 0xff0000) >> 16));

	/* need to get accurate transfer count for ISA master */
	if (((int)(ch_ptr->phys_addr) % 2) || (ch_ptr->count % 2))
		outs(&etest_reg->dma_count, (ch_ptr->count/2) + 1);
	else
		outs(&etest_reg->dma_count, ch_ptr->count/2);

	outb(&etest_reg->isa_dma_cycles, E_ISA_DMA_MEMORY);
	if (bp->b_flags & B_READ)
		outb(&etest_reg->dma_control, E_DMA_SIZE_WORD | E_DREQ_ON);
	else
		outb(&etest_reg->dma_control, E_DMA_SIZE_WORD | E_DREQ_ON | E_DMA_WRITE);

	outb(&etest_reg->more_dma_control, E_DO_ISA_MASTER);
	outb(&etest_reg->dma_tc, E_ISA_DMA_TC_OUT);
}


/*----------------------------------------------------------------------------*/
et_isa_master_dma(isc)
/*----------------------------------------------------------------------------*/
/* this routine sets up for an isa master dma transfer.
 */

struct isc_table_type *isc;
{
	struct dma_parms *isc_dma_parms;
	register struct dma_parms *dma_parms;
	register struct buf *bp;
	register struct etest_reg *etest_reg;
	register struct etest_card_info *card_info;
	int x, channel, dummy, ret, slot, options;

	card_info = (struct etest_card_info *)isc->if_drv_data;
	isc_dma_parms = (struct dma_parms *)isc->my_dma_parms;
	etest_reg = (struct etest_reg *)isc->if_reg_ptr;
	slot = ((struct eisa_if_info *)(isc->if_info))->slot;
	bp = isc->owner;

#ifdef OLD_WAY
	if ((bp->b_s2 = (int)kmem_alloc(sizeof(struct dma_parms))) == NULL) {
#else
	if ((bp->b_s2 = (int)io_malloc(sizeof(struct dma_parms), IOM_WAITOK)) == NULL) {
#endif
		printf("ETEST WARNING: allocating memory for dma_parms structure failed\n");
		return -1;
	}
	dma_parms = (struct dma_parms *)bp->b_s2;

	dma_parms->dma_options = DMA_CASCADE | isc_dma_parms->channel | DMA_16BYTE;
	dma_parms->flags = EISA_BYTE_ENABLE;
	dma_parms->drv_routine = etest_wakeup;
	dma_parms->drv_arg = (int)dma_parms;
	dma_parms->spaddr = bp->b_spaddr;
	dma_parms->addr = bp->b_un.b_addr;
	dma_parms->count = bp->b_bcount;

	/* 
	 * this is the indication to eisa_dma_setup that this is an ISA 
	 * master transfer.
	 */
	dma_parms->channel = -1;

	if (bp->b_flags & B_READ) {
		dma_parms->dma_options |= DMA_READ;
		dma_parms->dma_options &= ~DMA_WRITE;
	} else {
		dma_parms->dma_options |= DMA_WRITE;
		dma_parms->dma_options &= ~DMA_READ;
	}

	x = spl6();
	while (ret = eisa_dma_setup(isc, dma_parms)) {
		if (ret == RESOURCE_UNAVAILABLE) {
#ifdef EISA_TESTS
			printf("ETEST: eisa_dma_setup: RESOURCE_UNAVAILABLE, ISA sleeping: %d\n",slot);
#endif
			isc->owner = NULL;
			sleep(bp->b_s2, PZERO+2);
			isc->owner = bp;
		} else if (ret < 0) {
			splx(x);
#ifdef OLD_WAY
			kmem_free(dma_parms, sizeof(struct dma_parms));
#else
			io_free(dma_parms, sizeof(struct dma_parms));
#endif
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			isc->owner = NULL;
			printf("eisa_dma_setup returned error %d\n", ret);
			return(-1);
		} else break;
	}
	splx(x);

	/* set chain pointer index for use in dma chains */
	bp->b_s6 = 0; 

	/* initialize test card for DMA */
	load_isa_dma(isc);

	/* read from test card RAM to initialize RAM address pointers */
	x = spl6();
	etest_reg->eisa_select |= 0x80;
	snooze(5);
	dummy = (*card_info->virt_eisa_ptr);
	snooze(5);
	etest_reg->eisa_select &= 0x7f;
	splx(x);

	/* write to card to start DMA transfer */
	etest_reg->dma_ch_enable |= 1 << isc_dma_parms->channel;

	return(0);
}


/*----------------------------------------------------------------------------*/
load_eisa_master_dma(isc)
/*----------------------------------------------------------------------------*/
/* this routine  prepares the test card for an EISA master dma transfer.
 * It handles the  multi-elements  chains  returned  by  eisa_dma_setup.
 * bp->b_s6 contains the number of the chain element in processing.
 */

struct isc_table_type *isc;
{
	struct addr_chain_type *ch_ptr;
	struct dma_parms *dma_parms;
	struct buf *bp;
	struct etest_card_info *card_info;
	struct bmic_reg *bmic_reg;
	struct dma_parms *isc_dma_parms;
	int i, dma_address;
	int count = 0;

	card_info = (struct etest_card_info *)isc->if_drv_data;
	isc_dma_parms = (struct dma_parms *)isc->my_dma_parms;
	bmic_reg = (struct bmic_reg *)card_info->virt_bmic_ptr;
	bp = isc->owner;

	dma_parms = (struct dma_parms *)bp->b_s2;

	/* one less element to do later */
	dma_parms->chain_count--;

	/* point to the correct element in chain */
	ch_ptr = dma_parms->chain_ptr;

	/* 
	 * count is used to update the BMIC's ram pointer
	 */
	for (i=0; i < bp->b_s6; i++) {
		count += ch_ptr->count;
		ch_ptr++;
	}

	bp->b_s6++; 

#ifdef EISA_TESTS
	ch_stat(0x40);
#endif

	bmic_reg->local_index = global_config;
	bmic_reg->local_data = 0x2;	

	bmic_reg->local_index = ch0_status;
	bmic_reg->local_data = E_TFR_COMPLETE;	/* clear TFR_COMPLETE bit for channel */

	dma_address = (int)ch_ptr->phys_addr;

	/* init low byte of dma address */
	bmic_reg->local_index = ch0_base_addr_0;
	bmic_reg->local_data = dma_address & 0xff;

	/* init 2nd byte of dma address */
	bmic_reg->local_index = ch0_base_addr_1;
	bmic_reg->local_data = (dma_address & 0xff00) >> 8;

	/* init 3rd byte of dma address */
	bmic_reg->local_index = ch0_base_addr_2;
	bmic_reg->local_data = (dma_address & 0xff0000) >> 16;

	/* init 4th byte of dma address */
	bmic_reg->local_index = ch0_base_addr_3;
	bmic_reg->local_data = (dma_address & 0xff000000) >> 24;

	/* init low byte of dma count */
	bmic_reg->local_index = ch0_count_0;
	bmic_reg->local_data = ch_ptr->count & 0xff;

	/* init 2nd byte of dma count */
	bmic_reg->local_index = ch0_count_1;
	bmic_reg->local_data = (ch_ptr->count & 0xff00) >> 8;

	/* init 3rd byte of dma count */
	bmic_reg->local_index = ch0_count_2;

	if (bp->b_flags & B_READ)
		bmic_reg->local_data = ((ch_ptr->count & 0xff0000) >> 16) | E_START_DMA 
								| E_BMIC_READ;
	else
		bmic_reg->local_data = ((ch_ptr->count & 0xff0000) >> 16) | E_START_DMA;

	/* init dma config register */
	bmic_reg->local_index = ch0_config;
	bmic_reg->local_data = E_BMIC_BURST | E_TBI_ENABLE | E_RELEASE_CH; 

	/* 
	 * initialize RAM address pointer start registers. 
	 * These guys point to 16 bit quantities.
	 */

	count /=2;
	bmic_reg->local_index = ch0_tbi_addr_0;
	bmic_reg->local_data = count & 0x00ff;
	bmic_reg->local_index = ch0_tbi_addr_1;
	bmic_reg->local_data = (count & 0xff00) >> 8;

}


/*----------------------------------------------------------------------------*/
et_eisa_master_dma(isc)
/*----------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	register struct dma_parms *dma_parms;
	register struct buf *bp;
	register struct etest_card_info *card_info;
	register struct bmic_reg *bmic_reg;
	struct dma_parms *isc_dma_parms;
	int channel, dma_address, ret;
	int slot, done, timeout_count;
	struct addr_chain_type *ch_ptr;
	int x;

	card_info = (struct etest_card_info *)isc->if_drv_data;
	isc_dma_parms = (struct dma_parms *)isc->my_dma_parms;
	bmic_reg = (struct bmic_reg *)card_info->virt_bmic_ptr;
	slot = ((struct eisa_if_info *)(isc->if_info))->slot;
	bp = isc->owner;

#ifdef OLD_WAY
	if ((bp->b_s2 = (int)kmem_alloc(sizeof(struct dma_parms))) == NULL) {
#else
	if ((bp->b_s2 = (int)io_malloc(sizeof(struct dma_parms), IOM_WAITOK)) == NULL) {
#endif
		printf("ETEST WARNING: allocating memory for dma_parms structure failed\n");
		return(-1);
	}
	dma_parms = (struct dma_parms *)bp->b_s2;

	dma_parms->dma_options = isc_dma_parms->dma_options;
	dma_parms->flags = EISA_BYTE_ENABLE;
	dma_parms->drv_routine = etest_wakeup;
	dma_parms->drv_arg = (int)dma_parms;
	dma_parms->spaddr = bp->b_spaddr;
	dma_parms->addr = bp->b_un.b_addr;
	dma_parms->count = bp->b_bcount;
	dma_parms->channel = -1;

	/*
	 * read and  write  dma_option  parameters  should be set up for
	 * eisa  masters  so  that  eisa_dma_setup  can  handle  buflets
	 * correctly.
	 */
	if (bp->b_flags & B_READ) {
		dma_parms->dma_options |= DMA_READ;
		dma_parms->dma_options &= ~DMA_WRITE;
	} else {
		dma_parms->dma_options |= DMA_WRITE;
		dma_parms->dma_options &= ~DMA_READ;
	}

	x = spl6();
	while (ret = eisa_dma_setup(isc, dma_parms)) {
		if (ret == RESOURCE_UNAVAILABLE) {
#ifdef EISA_TESTS
			printf("ETEST: eisa_dma_setup: RESOURCE_UNAVAILABLE, EISA sleeping: %d\n",slot);
#endif
			isc->owner = NULL;
			sleep(bp->b_s2, PZERO+2);
			isc->owner = bp;
		} else if (ret < 0) {
			splx(x);
#ifdef OLD_WAY
			kmem_free(dma_parms, sizeof(struct dma_parms));
#else
			io_free(dma_parms, sizeof(struct dma_parms));
#endif
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			isc->owner = NULL;
			printf("eisa_dma_setup returned error %d\n", ret);
			return(-1);
		} else  break;
	}
	splx(x);

	/* 
	 * init bmic based on  dma_options  values, we'll always use ch0
	 * for now check to see if channel 0 is busy, wait if so
	 */

	bmic_reg->local_index = ch0_status;

	/* shouldn't be possible with isc->owner impl? */
	while (bmic_reg->local_data & (E_TFR_GOING | E_TFR_ENABLED))
		sleep(bmic_reg, PZERO+2);	/* wake this guy up in dma_cleanup */
	
	bp->b_s6 = 0; 
	load_eisa_master_dma(isc);

	/* write to card to start DMA transfer */
	bmic_reg->local_index = ch0_strobe;
	bmic_reg->local_data = 1;	/* write to reg, data doesn't matter */

	/*
	 * Since the old  version  of the test card does not allow the bmic chip
	 * to generate an  interrupt  upon dma  completion,  we were stuck using
	 * polling to determine  completion.  On the other hand, it is desirable
	 * for this driver to be an  acceptable  reference  for other dma master
	 * drivers  and as such should have the look and feel of a real  driver.
	 * etest_isr()  is written to share  interrupt  lines and as such can be
	 * called for cases in which it has nothing to do.  the current  timeout
	 * value is 3 minutes.  A timeout will result in an  incomplete  dma and
	 * cause the tests to fail.
	 */
	done = 2;
	while (done == 2) {
		done = timeout_count = 0;
		while ((!done) && (timeout_count < 2000000)) {
			done = etest_isr(isc);
			snooze(100);  timeout_count++;
		}
	}

	if (done == 0)
		printf("eisa master timed out\n");

	return(0);
}


/*----------------------------------------------------------------------------*/
etest_strategy(bp)
/*----------------------------------------------------------------------------*/

struct buf *bp;
{
	struct isc_table_type *isc;
	struct etest_reg * etest_reg;
	struct etest_card_info *card_info;
	int ret, pri; 

	get_isc(bp->b_dev, isc);
	if (isc == NULL) return(ENODEV);

	pri = spl6();
	if (isc->owner) sleep(isc, PZERO+2);
	isc->owner = bp;
	splx(pri);

	bp->b_s2 = (int)isc->my_dma_parms;
	card_info = (struct etest_card_info *)isc->if_drv_data;

	if (card_info->flags & E_EISA_SLAVE)
		return(et_slave_setup(isc));
	else if (card_info->flags & E_ISA_MASTER)
		return(et_isa_master_dma(isc));
	else if (card_info->flags & E_EISA_MASTER)
		return(et_eisa_master_dma(isc));
	else {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		isc->owner = NULL;
		return(-1);
	}
}


#ifdef SOMEDAY
/*----------------------------------------------------------------------------*/
test_map_mem_to_bus(isc)
/*----------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	unsigned char buffer[NBPG];
	struct io_parms *iop;
	struct isc_table_type *isc1;

#ifdef OLD_WAY
	iop = (struct io_parms *)kmem_alloc(sizeof(struct io_parms));
#else
	iop = (struct io_parms *)io_malloc(sizeof(struct io_parms), IOM_WAITOK);
#endif

	iop->host_addr = buffer;
	iop->size = sizeof(buffer);
	iop->drv_routine = etest_notify;
	iop->drv_arg = (1<<slot);

	isc1 = get_new_isc(isc);
}
#endif


/*----------------------------------------------------------------------------*/
etest_ioctl(dev, cmd, arg_ptr, flag)
/*----------------------------------------------------------------------------*/

dev_t dev;
int cmd;
int *arg_ptr;
int flag;
{
	struct isc_table_type *isc;
	struct etest_reg *etest_reg;
	int new_address, current_address;
	caddr_t new_ptr;
	struct etest_card_info *card_info;
	struct dma_parms *dma_parms, dp;
	struct bmic_reg *bmic_reg;
	char *ch_ptr;
	int spl, slot, i, j, *int_ptr; 
	int return_value = 0;
	int addr, *ptr, cnt;
	struct resource_manip *rsrc;
	int size, num;
	struct iomap_data *table; 
	struct addr_chain_type *chn_ptr;
	struct io_parms *iop;
	int test_size;
	struct isc_table_type *isc1, *isc2;
	struct mov_pair *mvp;


	get_isc(dev, isc);
	if (isc == NULL)
		return(ENODEV);

	spl = spl6();
	if (isc->owner)
		sleep(isc, PZERO+2);
	isc->owner = (struct buf *)isc;
	splx(spl);

	card_info = (struct etest_card_info *)isc->if_drv_data;
	dma_parms = (struct dma_parms *)isc->my_dma_parms;
	etest_reg = (struct etest_reg *)isc->if_reg_ptr;
	bmic_reg = (struct bmic_reg *)card_info->virt_bmic_ptr;
	slot = ((struct eisa_if_info *)(isc->if_info))->slot;

	switch (cmd) {
		case E_SET_BMIC_ADDR:
			/* 
			 * allows the BMIC  address to be  changed.  This  routine  does
			 * rely  upon  EEPROM  data and  might  be  dangerous  to use in
			 * multicard systems.
			 */

			new_address = (unsigned int)*arg_ptr;
			/* valid range check */
			if (((new_address/E_ISA_IO_CHUNK) < E_ISA_IO_MIN) || 
					((new_address/E_ISA_IO_CHUNK) > E_ISA_IO_MAX) ) { 
				return_value = EINVAL; 
				break; 
			}
			current_address = (int)card_info->phys_bmic_ptr;
			if (current_address == new_address)  {
				break;
			}
			etest_reg->io_select &= ~E_IOSEL_ENABLE;

			if ((new_ptr = map_isa_address(isc, new_address)) == NULL) {
				return_value = ENOMEM;
				break;
			}

			/* if everything else succeeded, unmap old location of bmic */
			unmap_mem_from_host(isc, card_info->virt_bmic_ptr, NBPG);
			
			card_info->virt_bmic_ptr = new_ptr;
			card_info->phys_bmic_ptr = (caddr_t)new_address;
			outb(&etest_reg->io_select, new_address/E_ISA_IO_CHUNK | E_IOSEL_ENABLE);

			break;

		case E_SET_ISA20_RAM:
			/*
			 * allows  the ISA 20 bit  address  space  to be  changed.  This
			 * routine does rely upon EEPROM data and might be dangerous  to
			 * use in multicard systems.
			 */

			new_address = (unsigned int)*arg_ptr;
			if (((new_address/E_ISA20_CHUNK) < E_ISA20_MIN) || 
					((new_address/E_ISA20_CHUNK) > E_ISA20_MAX) ) { 
				return_value = EINVAL; 
				break; 
			}
			current_address = (int)card_info->phys_isa20_ptr;
			if (current_address == new_address)  {
				break;
			}
			etest_reg->isa20_select &= ~E_SMEMSEL_ENABLE;

			if ((new_ptr = map_mem_to_host(isc, new_address, E_ISA20_CHUNK)) == NULL) {
				return_value = ENOMEM;
				break;
			}

			/* if everything else succeeded, unmap old location of isa20 */
			unmap_mem_from_host(isc, card_info->virt_isa20_ptr, E_ISA20_CHUNK);

			card_info->virt_isa20_ptr = new_ptr;
			card_info->phys_isa20_ptr = (caddr_t)new_address;
			outb(&etest_reg->isa20_select,new_address/E_ISA20_CHUNK | E_SMEMSEL_ENABLE);

			break;

		case E_SET_ISA24_RAM:  
			/*
			 * allows  the ISA 24 bit  address  space  to be  changed.  This
			 * routine does rely upon EEPROM data and might be dangerous  to
			 * use in multicard systems.
			 */

			new_address = (unsigned int)*arg_ptr;
			if (((new_address/E_ISA24_CHUNK) < E_ISA24_MIN) || 
					((new_address/E_ISA24_CHUNK) > E_ISA24_MAX) ) { 
				return_value = EINVAL; 
				break; 
			}
			current_address = (int)card_info->phys_isa24_ptr;
			if (current_address == new_address)  {
				break;
			}
			etest_reg->isa24_select &= ~E_IMEMSEL_ENABLE;

			if ((new_ptr = map_mem_to_host(isc, new_address, E_ISA24_CHUNK)) == NULL) {
				return_value = ENOMEM;
				break;
			}

			/* if everything else succeeded, unmap old location of isa24 */
			unmap_mem_from_host(isc, card_info->virt_isa24_ptr, E_ISA24_CHUNK);

			card_info->virt_isa24_ptr = new_ptr;
			card_info->phys_isa24_ptr = (caddr_t)new_address;
			outb(&etest_reg->isa24_select,new_address/E_ISA24_CHUNK | E_IMEMSEL_ENABLE);

			break;

		case E_SET_EISA_RAM:
			/* 
			 * will  take an eisa  physical  address  and  setup  a  virtual
			 * address  mapping  for it.  It will  also  free up a  previous
			 * address  mapping if it existed.  32 bit eisa memory  takes up
			 * 16M (it is  multiply  mapped).  This  routine  does rely upon
			 * EEPROM  data  and  might  be  dangerous  to use in  multicard
			 * systems.
			 */

			new_address = (unsigned int)*arg_ptr;
			if (((new_address/E_EISA_CHUNK) < E_EISA_MIN) || 
					((new_address/E_EISA_CHUNK) > E_EISA_MAX) ) { 
				return_value = EINVAL; 
				break; 
			}

			current_address = (int)card_info->phys_eisa_ptr;
			if (current_address == new_address)  {
				/* no work to do */
				break;
			}
			/* do not change it while enabled */
			etest_reg->eisa_select &= ~E_EMEMSEL_ENABLE;

			/* lets do the mapping */
			if ((new_ptr = map_mem_to_host(isc, new_address, E_EISA_CHUNK)) == NULL) {
				/* cannot do it */
				return_value = ENOMEM;
				break;
			}

			/* if everything else succeeded, unmap old location of eisa memory */
			unmap_mem_from_host(isc, card_info->virt_eisa_ptr, E_EISA_CHUNK);

			/* store the results in some globals */
			card_info->virt_eisa_ptr = new_ptr;
			card_info->phys_eisa_ptr = (caddr_t)new_address;

			/* lets tell the card */
			outb(&etest_reg->eisa_select, new_address/E_EISA_CHUNK | E_EMEMSEL_ENABLE);

			break;

		case E_SET_FLAGS:
			/* this sets the type of DMA transfers to be performed */
			if ((*arg_ptr != E_ISA_MASTER) && (*arg_ptr != E_EISA_MASTER) && 
					(*arg_ptr != E_EISA_SLAVE)) { 
				return_value = EINVAL;
				break;  
			}

			/* 
			 * right now, all flags bits are mutually exclusive, so OK to clear 
			 * other bits 
			 */
			card_info->flags = *arg_ptr;
			break;

		case E_SET_DATA_SIZE:
			/* this sets the size of the DMA transfers to be performed */
			if ((*arg_ptr != DMA_8BYTE) && (*arg_ptr != DMA_16BYTE) && 
					(*arg_ptr != DMA_16WORD) && (*arg_ptr != DMA_32BYTE)) { 
				return_value = EINVAL;
				break;  
			}

			dma_parms->dma_options &= ~(DMA_8BYTE|DMA_16BYTE | DMA_16WORD | DMA_32BYTE);
			dma_parms->dma_options |= *arg_ptr;
			break;

		case E_SET_DMA_TYPE:
			/* this sets the bus cycle type of DMA transfers to be performed */
			if ((*arg_ptr != DMA_ISA) && (*arg_ptr != DMA_TYPEA) && 
					(*arg_ptr != DMA_TYPEB) && (*arg_ptr != DMA_BURST)) { 
				return_value = EINVAL;
				break;  
			}

			dma_parms->dma_options &= ~(DMA_ISA | DMA_TYPEA | DMA_TYPEB | DMA_BURST);
			dma_parms->dma_options |= *arg_ptr;
			break;

		case E_SET_DMA_MODE:
			/* this sets the mode of DMA transfers to be performed */
			if ((*arg_ptr != DMA_DEMAND) && (*arg_ptr != DMA_BLOCK) && 
					(*arg_ptr != DMA_SINGLE)) { 
				return_value = EINVAL;
				break;  
			}

			dma_parms->dma_options &= ~(DMA_DEMAND | DMA_BLOCK | DMA_SINGLE);
			dma_parms->dma_options |= *arg_ptr;
			break;

		case E_INTERRUPT:
			/*
			 * this will force an interrupt, always an interesting first test.
			 */
			spl = spl6();
			etest_reg->dma_done |= E_FORCE_INTERRUPT;
			sleep(&(etest_reg->dma_done), PRIBIO);
			splx(spl);
			break;

		case E_SET_IRQ:
			/* 
			 * this  allows the current  interrupt  to be  changed.  It uses
			 * EEPROM  data  obtained  by  attach  and is safe in  multicard
			 * systems.
			 */

			if (*arg_ptr < 3 || *arg_ptr == 8 || *arg_ptr == 13 || *arg_ptr > 15) {
				return_value = EINVAL;
				break;
			}

			/* 
			 * use  information  set from function data obtained at powerup.
			 * if the  request  is not in the list, do not  change it.  this
			 * will allow the test card (assuming  friendly  eeprom data) to
			 * remain friendly with other cards in the system.
			 */

			if (card_info->etd->lvl_ints & (1<<*arg_ptr)) {
				outs(&etest_reg->level_irqs, 1 << *arg_ptr);
				card_info->etest_irq = *arg_ptr;
			} else if (card_info->etd->edge_ints & (1<<*arg_ptr)) {
				outs(&etest_reg->edge_irqs, 1 << *arg_ptr);
				card_info->etest_irq = *arg_ptr;
			} else return_value = EINVAL;

			break;

		case E_DMA_CHANNEL:
			if (*arg_ptr < 0 || *arg_ptr == 4 || *arg_ptr > 7) {
				return_value = EINVAL;
				break;
			}

			if (card_info->etd->dma_chans & (1<<*arg_ptr)) 
				dma_parms->channel = *arg_ptr;
			else 
				return_value = EINVAL;
			break;

		case E_SET_BLK_XFER_LEN:
			if (1 <= *arg_ptr && *arg_ptr <= 64) 
				card_info->max_block_xfer_len = *arg_ptr;
			else 
				return_value = EINVAL;

			break;

		case E_ASSERT_IOCHECK:
			etest_reg->assert_iochk |= E_ASSERT_IOCHK;
			break;

		case E_FORCE_XFER_COUNT:
			/*
			 * this allows the test card to be programmed  for premature dma
			 * termination.  This can be used for resid calculations.
			 */

			card_info->force_test_card_count = *arg_ptr;
			break;

		case E_CHECK_EISA_ADDR: {
			/*
			 * this ioctl performs a test which will map in all conceivable
			 * ranges of ISA and EISA addresses, and check that reads and writes 
			 * can be performed to it.
			 *
			 * NOTE: This should be the only card in the system when this test
			 *       is run.
			 */

#ifdef NOT_NOW
			int off;
			int physaddr;
			caddr_t vaddr;

			etest_reg = (struct etest_reg *)isc->if_reg_ptr;
			card_info = (struct etest_card_info *)isc->if_drv_data;

			/* if mapped prior, disable and unmap */
			if (card_info->virt_bmic_ptr != NULL) {
				etest_reg->io_select &= ~E_IOSEL_ENABLE;
				unmap_mem_from_host(isc, card_info->virt_bmic_ptr, NBPG);
				card_info->virt_bmic_ptr = NULL;
			}

			/* show no errors */
			*arg_ptr = 0;

			/* check address range 0x8000 to 0x100000(1M) */
			for (off = 16; off <= 31; off++) {
				physaddr = 32768 * off;	
				if ((vaddr = map_mem_to_host(isc, physaddr, 32*1024)) == NULL) {
					*arg_ptr |= (1<<0);
					break;
				}

				outb(&etest_reg->io_select,
				    ((int)(card_info->etd->port_loc)/E_ISA_IO_CHUNK)|E_IOSEL_ENABLE);
				for (i=0; i<4096; i++) {
					*((char *)((int)vaddr + 2 + (i * 8))) = 0x5a;
					if (*((char *)((int)vaddr + 2 + (i * 8))) != 0x5a) 
						*arg_ptr |= (1<<1);

					*((char *)((int)vaddr + 2 + (i * 8))) = 0xa5;
					if (*((char *)((int)vaddr + 2 + (i * 8))) != 0xa5) 
						*arg_ptr |= (1<<2);
				}

				unmap_mem_from_host(isc, vaddr, 32*1024);
			}
						

			/* check address range 0x500000(5M) to 0x800000(8M) */
			for (off = 80; off <= 127; off++) {
				physaddr = 65536 * off;	
				if ((vaddr = map_mem_to_host(isc, physaddr, 64*1024)) == NULL) {
					*arg_ptr |= (1<<3);
					break;
				}

				for (i=0; i<8192; i++) {
					*((char *)((int)vaddr + 2 + (i * 8))) = 0x5a;
					if (*((char *)((int)vaddr + 2 + (i * 8))) != 0x5a) 
						*arg_ptr |= (1<<4);

					*((char *)((int)vaddr + 2 + (i * 8))) = 0xa5;
					if (*((char *)((int)vaddr + 2 + (i * 8))) != 0xa5) 
						*arg_ptr |= (1<<5);
				}

				unmap_mem_from_host(isc, vaddr, 64*1024);
			}
			break;
#else
			int physaddr = *arg_ptr;
			caddr_t vaddr;

			etest_reg = (struct etest_reg *)isc->if_reg_ptr;
			card_info = (struct etest_card_info *)isc->if_drv_data;

			*arg_ptr = 0;

			/* reset card */
			etest_reg->eisa_control &= ~ETEST_ENABLE; 
			if (initialize_test_card(isc) < 0) {
				*arg_ptr |= (1<<5);  break;
			}
			outb(&etest_reg->eisa_control, ETEST_ENABLE); /* Turn it all back on */

			/* if mapped prior, disable and unmap */
			if (card_info->virt_bmic_ptr != NULL) {
				etest_reg->io_select &= ~E_IOSEL_ENABLE;
				unmap_mem_from_host(isc, card_info->virt_bmic_ptr, NBPG);
				card_info->virt_bmic_ptr = NULL;
			}


			/* 
			 * avoid iomap hardware, and EISA/ISA IO space 
			 */
			if (0x100 <= physaddr && physaddr <= 0x3ff) {
	outb(&etest_reg->io_select,(physaddr/E_ISA_IO_CHUNK)|E_IOSEL_ENABLE);
				/* etest_reg->io_select = 0x80 | (physaddr / 8); */

				if ((vaddr = map_isa_address(isc, physaddr)) == NULL) { 
					*arg_ptr |= (1<<1);  break;
				}
			} else if (0x80000 <= physaddr && physaddr <= 0x100000) { 
				etest_reg->isa20_select =  0x80 | (physaddr / 0x8000); 

				if ((vaddr = map_mem_to_host(isc, physaddr, 4 /* bytes */)) == NULL) { 
					*arg_ptr |= (1<<1);  break;
				}
			} else if (0x500000 <= physaddr && physaddr <= 0x800000) {
				etest_reg->isa24_select =  0x80 | (physaddr / 0x10000); 

				if ((vaddr = map_mem_to_host(isc, physaddr, 4 /* bytes */)) == NULL) { 
					*arg_ptr |= (1<<1);  break;
				}
			} else {
				*arg_ptr |= (1<<0);
				break;
			}

			bmic_reg = (struct bmic_reg *)vaddr;
			bmic_reg->local_index = global_config;

			if (bmic_reg->local_index != global_config)
				*arg_ptr |= (1<<2);

			bmic_reg->local_index = 0;

			if (bmic_reg->local_index != 0)
				*arg_ptr |= (1<<3);

			etest_reg->io_select &= ~0x80;
			etest_reg->isa20_select &= ~0x80; 
			etest_reg->isa24_select &= ~0x80; 
				
			unmap_mem_from_host(isc, vaddr, 4);
#endif
		}

#ifdef IO_TEST
		case E_GRAB_RESOURCE:
			rsrc = (struct resource_manip *)arg_ptr;
			if (rsrc->resource_type == IOMAP_ENTRIES) {
				if (rsrc->action == AQUIRE) {
					set_io_test(1,0);
				} else if (rsrc->action == RELEASE) {
					set_io_test(2,0);
					/* 
					 * we need to call free to release any  processes on the
					 * waiting  list.  To be very safe we will  setup  first
					 * with a lock entries  call.  We will request  only one
					 * entry  which in light of how many were  "given"  back
					 * above should "always" be successful
					 */
					
					if (lock_iomap_entries(isc, 1, &dp)) {
						rsrc->action *= -1;
					} else
						free_iomap_entries(isc, &dp);

					splx(spl);
				} else if (rsrc->action == STATUS) {
				} else rsrc->action = -1 * ACT_ERROR;
			} else if (rsrc->resource_type == DMA_CHANNELS) {
				if (rsrc->action == AQUIRE) {
					if (eisa_get_dma(isc, rsrc->num))
						rsrc->action *= -1;
				} else if (rsrc->action == RELEASE) {
					if (eisa_free_dma(isc, rsrc->num))
						rsrc->action *= -1;

				} else if (rsrc->action == STATUS) {
					/* update fields to reflect status */
				} else rsrc->action = -1 * ACT_ERROR;
			} else rsrc->action = -1 * RESRC_ERROR;
			break;
#endif

		case E_SET_BUF:
			/* 
			 * this writes a known 4 byte  pattern  into EISA memory  space.
			 * It is not safe for multiple cards mapped to the same space.
			 */
			int_ptr = (int *)card_info->virt_eisa_ptr;

			etest_reg->eisa_select |= 0x80; /* enable the cards memory */
			for (i=0; i<2*1024; i++) 
				*(int_ptr++) = *arg_ptr;
			etest_reg->eisa_select &= 0x7f; /* disable the cards memory */
			break;

		case E_CMP_BUF:
			int_ptr = (int *)card_info->virt_eisa_ptr;

			etest_reg->eisa_select |= 0x80; /* enable the cards memory */
			for (i=0; i<2*1024; i++) {
			 	if (*arg_ptr != *(int_ptr++)) {
					*arg_ptr = 1;
					break;
				}
			}
			*arg_ptr = 0;
			etest_reg->eisa_select &= 0x7f; /* disable the cards memory */
			break;


		/*
		case MAP_MEM_TO_HOST_TEST:
			test_ptr1 = map_mem_to_host(isc, 
			break;
		*/

		case E_UIOMOVE_IN:
			break;

		case E_UIOMOVE_OUT:
			break;

		case E_COPYIN:
			mvp = (struct mov_pair *) arg_ptr;
#ifdef OLD_WAY
			buffer = (char *)kmem_alloc(mvp->size);
#else
			buffer = (char *)io_malloc(mvp->size, IOM_WAITOK);
#endif
			if (copyin(mvp->addr, buffer, mvp->size))
				return_value = -1;
			break;

		case E_COPYOUT:
			mvp = (struct mov_pair *) arg_ptr;
			if (copyout(buffer, mvp->addr, mvp->size)) 
				return_value = -1;
#ifdef OLD_WAY
			kmem_free(buffer, mvp->size);
#else
			io_free(buffer, mvp->size);
#endif
			break;


#ifdef IO_TEST
		case E_NRML_TST_ROUTINES:
			/*
			 * this is where I can test various normal services routines
			 */
			
			*arg_ptr = 0;

			spl = spl6();	
			etest_reg->eisa_select |= 0x80; /* enable the cards memory */

		/*---- test outl() and inl() */
			outl((int *)card_info->virt_eisa_ptr, 0x12345678);
			if (*((int *)card_info->virt_eisa_ptr) != 0x78563412)
				*arg_ptr |= (1<<0);

			if (inl((int *)card_info->virt_eisa_ptr) != 0x12345678)
				*arg_ptr |= (1<<0);

		/*---- test outs() and ins() */
			outs((u_short *)card_info->virt_eisa_ptr, 0x9876);
			if (ins((u_short *)card_info->virt_eisa_ptr) != 0x9876)
				*arg_ptr |= (1<<1);

#ifdef __hp9000s700
		/*---- test lock and unlock eisa bus */
			cnt = 0;
			lock_eisa_bus(isc);
			while (cnt++ < 4 && (read_eisa_bus_lock(isc) != 0))
				lock_eisa_bus(isc);

			if (cnt == 4)
				*arg_ptr |= (1<<2);

			unlock_eisa_bus(isc);
			if (read_eisa_bus_lock(isc) != 1)
				*arg_ptr |= (1<<3);
#endif

			etest_reg->eisa_select &= 0x7f; /* disable the cards memory */
			splx(spl);

		/* test map_mem_to_host and unmap_mem_from_host */
			/* create an address not already mapped in and add an offset */
			addr = card_info->etd->mem_loc + (3*NBPG) + 0x7f0;
			if ((ptr = (int *)map_mem_to_host(isc, addr, 0x100)) == NULL) {
				*arg_ptr |= (1<<4);
			} else {
				spl = spl6();	
				etest_reg->eisa_select |= 0x80; /* enable the cards memory */

				*ptr = 0x23456789;
				if (*ptr != 0x23456789)
					*arg_ptr |= (1<<5);

				etest_reg->eisa_select &= 0x7f; /* disable the cards memory */
				splx(spl);

				unmap_mem_from_host(isc, ptr, 0x100);
			}

			/* create an address not already mapped in and add an offset */
			addr = card_info->etd->mem_loc + (3*NBPG) + 0x6f0;
			if ((ptr = (int *)map_mem_to_host(isc, addr, 0x100+2*NBPG)) == NULL) {
				*arg_ptr |= (1<<6);
			} else {
				spl = spl6();	
				etest_reg->eisa_select |= 0x80; /* enable the cards memory */

				*ptr = 0x34567890;
				if (*ptr != 0x34567890)
					*arg_ptr |= (1<<7);

				*((int *)((int)(ptr)+2*NBPG)) = 0x98765432;
				if (*((int *)((int)(ptr)+2*NBPG)) != 0x98765432)
					*arg_ptr |= (1<<8);

				etest_reg->eisa_select &= 0x7f; /* disable the cards memory */
				splx(spl);

				unmap_mem_from_host(isc, ptr, 0x100+2*NBPG);
			}

		/* test io_dma_build_chain */
			/* create an address not already mapped in and add an offset */
			addr = card_info->etd->mem_loc + (6*NBPG) + 0x555;

			/* an ltor call in io_dma_build_chain requires a v-to-p mapping */
			if ((ptr = (int *)map_mem_to_host(isc, addr, 12288)) == NULL) {
				*arg_ptr |= (1<<9);
			} else {
				dp.spaddr = 0;

#ifdef OLD_WAY
				dp.chain_ptr=(struct addr_chain_type *)kmem_alloc(sizeof(struct addr_chain_type)*(12288/NBPG + 3));
#else
				dp.chain_ptr=(struct addr_chain_type *)io_malloc(sizeof(struct addr_chain_type)*(12288/NBPG + 3), IOM_WAITOK);
#endif
                		dp.chain_count = io_dma_build_chain(dp.spaddr,ptr,12288,dp.chain_ptr);
				chn_ptr = dp.chain_ptr;

				/* 
				 * This  test  assumes  logical  =  physical.  This is not a bad
				 * assumption  since  for  current  workstations  this is  true.
				 * Ideally this test would do some dma or something later.
				 */
				for (j=1, i=0; i < 12288/NBPG + 3; i++) {
					addr = (int)chn_ptr->phys_addr + chn_ptr->count;
					chn_ptr++;
					if (chn_ptr->phys_addr != 0) {
						j++;
						if (addr != (int)chn_ptr->phys_addr) 
							*arg_ptr |= (1<<10);
					} else break;
					
				}

				if (j != dp.chain_count)
					*arg_ptr |= (1<<11);

				unmap_mem_from_host(isc, ptr, 12288);
#ifdef OLD_WAY
				kmem_free(dp.chain_ptr, sizeof(struct addr_chain_type)); 
#else
				io_free(dp.chain_ptr, sizeof(struct addr_chain_type)); 
#endif
			}

		/* test map_mem_to_bus and unmap_mem_from_bus */
			for (test_size=1; test_size <4; test_size++) {
#ifdef OLD_WAY
				buffer = kmem_alloc(NBPG*test_size);
				iop = (struct io_parms *)kmem_alloc(sizeof(struct io_parms));
#else
				buffer = io_malloc(NBPG*test_size, IOM_WAITOK);
				iop = (struct io_parms *)
				    io_malloc(sizeof(struct io_parms), IOM_WAITOK);
#endif

				iop->host_addr = buffer;
				iop->size = sizeof(buffer);
				iop->drv_routine = etest_notify;
				iop->drv_arg = (1<<slot);

				/* lets try a simple test first */
				spl = spl6();
				etest_mailbox &= ~(1<<slot);
				splx(spl);

				if ((addr = map_mem_to_bus(isc, iop)) <= 0) {
					if (addr != RESOURCE_UNAVAILABLE) {
						*arg_ptr |= (1<<12);
#ifdef OLD_WAY
						kmem_free(buffer, NBPG);
						kmem_free(iop, sizeof(struct io_parms));
#else
						io_free(buffer, NBPG);
						io_free(iop, sizeof(struct io_parms));
#endif
						break;
					} else {
						/* 
						 * it is not going to be simple so lets wait and
						 * see if our notify  routine gets called.  I do
						 * not want to  sleep  here  since I would  like
						 * more  indication of failure  other than never
						 * coming back.
						 */
						for (i=0;i<5000&& !(etest_mailbox & (1<<slot)); i++)
							snooze(50);

						if (i >= 5000) {
							*arg_ptr |= (1<<13);
#ifdef OLD_WAY
							kmem_free(buffer, NBPG);
							kmem_free(iop, sizeof(struct io_parms));
#else
							io_free(buffer, NBPG);
							io_free(iop, sizeof(struct io_parms));
#endif
							break;
						}
					}

				}

				/* did we or did we not get our earlier request */
				if (addr > 0) {
#ifdef __hp9000s300
					/* this needs fixed */
					if (*((int *)(EISA1_BASE_ADDR + (int)addr)) != (vtop(buffer)>>PGSHIFT))
						*arg_ptr |= 17;
#else
					if (*((int *)(EISA1_BASE_ADDR + (int)addr)) != (ltor(0,buffer)>>PGSHIFT))
						*arg_ptr |= 17;
#endif

					/* now we can give it back */
					unmap_mem_from_bus(isc, iop);
				}

				/* steal all of the iomap entries */
				set_io_test(1,0);

				spl = spl6();

				etest_mailbox &= ~(1<<slot);

				/* this is to actually test the free up list code */
				isc1 = get_new_isc(isc);
				isc2 = get_new_isc(isc);
				
				addr = map_mem_to_bus(isc1, iop);

				if ((addr = map_mem_to_bus(isc, iop)) != RESOURCE_UNAVAILABLE) {
					/*
					 * it is an error not to get this  return  value
					 * when there are no iomap entries.
					 */

					*arg_ptr |= (1<<14);

					set_io_test(2,0);
					splx(spl);

#ifdef OLD_WAY
					kmem_free(buffer, NBPG);
					kmem_free(iop, sizeof(struct io_parms));
#else
					kmem_free(buffer, NBPG);
					kmem_free(iop, sizeof(struct io_parms));
#endif
					break;
				}

				iop->drv_arg = (1<<(3*slot));
				addr = map_mem_to_bus(isc2, iop);

				/* 
				 * this  should  simply  remove  the guy
				 * from the  waiting  list this is being
				 * done soley for test coverage
				 */
				unmap_mem_from_bus(isc, iop);
				unmap_mem_from_bus(isc2, iop);
				unmap_mem_from_bus(isc1, iop);

#ifdef OLD_WAY
				kmem_free(isc2, sizeof(struct isc_table_type));
#else
				io_free(isc2, sizeof(struct isc_table_type));
#endif

				iop->drv_arg = (1<<(slot+4));
				addr = map_mem_to_bus(isc1, iop);
				iop->drv_arg = (1<<slot);

				if ((addr = map_mem_to_bus(isc, iop)) != RESOURCE_UNAVAILABLE) {
					/* it is an error not to get this return value when there
					 * are no iomap entries.
					 */
					*arg_ptr |= (1<<15);

					set_io_test(2,0);
					splx(spl);

#ifdef OLD_WAY
					kmem_free(buffer, NBPG);
					kmem_free(iop, sizeof(struct io_parms));
					kmem_free(isc1, sizeof(struct isc_table_type));
#else
					io_free(buffer, NBPG);
					io_free(iop, sizeof(struct io_parms));
					io_free(isc1, sizeof(struct isc_table_type));
#endif
					break;
				}

				/* unlock the iomap */
				set_io_test(2,0);

				/* 
				 * we need to call free to  release  any
				 * processes on the waiting list.  To be
				 * very safe we will setup  first with a
				 * lock entries  call.  We will  request
				 * only one entry  which in light of how
				 * many were  "given" back above  should
				 * "always" be successful
				 */
				lock_iomap_entries(isc, 1, &dp);
				free_iomap_entries(isc, &dp);
				splx(spl);

				for (i=0; i<5000; i++) {
					if (etest_mailbox&(1<<slot) && etest_mailbox&(1<<(slot+4)))
						break;
					snooze(50);
				}		

				if (i >= 5000) {
					*arg_ptr |= (1<<16);
				}

#ifdef OLD_WAY
				kmem_free(buffer, NBPG);
				kmem_free(iop, sizeof(struct io_parms));
				kmem_free(isc1, sizeof(struct isc_table_type));
#else
				io_free(buffer, NBPG);
				io_free(iop, sizeof(struct io_parms));
				io_free(isc1, sizeof(struct isc_table_type));
#endif
			} /* loop on test_size */
			break;
#endif

		case E_BAD_TST_ROUTINES:
#ifdef __hp9000s700
			set_liowait(isc);
#endif
			eisa_refresh_on(isc);
			break;

#ifdef NOT_YET
		case E_GET_NEW_ISC:
			card_info->old_isc = isc;
			isc = get_new_isc(isc);
			/* do everything */
			break;
#endif

		case E_RESET:      
			/* 
			 * initialize card registers and globals to powerup state 
			 */

			/* disable card */
			etest_reg->eisa_control &= ~ETEST_ENABLE; 
	
			/* do the generic initialization */
			if (initialize_test_card(isc) < 0)
				return_value = EIO; 

			/* enable card */	
			outb(&etest_reg->eisa_control, ETEST_ENABLE); /* Turn it all back on */

			*arg_ptr = 0;
			break;

#ifdef EISA_TESTS
			/* Clear global holding results of dma chaining tests */
			ch_stat(0xff);  /* clear */
#endif
	
			break;
#ifdef EISA_TESTS 
		case READ_CHAIN_STAT:
			/* 
                         * give the test program the results of chaining instrumentation 
			 * note: reading the global also clears it.
			 */

			get_ch_stat((unsigned char *)arg_ptr);
			break;
#endif
		default:
			/* unrecognized ioctl */
			return_value = EINVAL;
		}	/* end switch */

	isc->owner = NULL;
	wakeup(isc);
	return(return_value);
}


/*----------------------------------------------------------------------------*/
etest_wakeup(dma_parms)
/*----------------------------------------------------------------------------*/

struct dma_parms *dma_parms;
{
	wakeup(dma_parms);
}


/*----------------------------------------------------------------------------*/
etest_notify(arg)
/*----------------------------------------------------------------------------*/

int arg;
{
	etest_mailbox |= arg;
}


/*----------------------------------------------------------------------------*/
etest_isr(isc)
/*----------------------------------------------------------------------------*/

struct isc_table_type *isc;
{
	struct etest_reg *etest_reg;
	struct etest_card_info *card_info;
	struct dma_parms *dma_parms;
	struct dma_parms *isc_dma_parms;
	struct bmic_reg *bmic_reg;
	struct buf *bp;
	int resid;
	int channel, options;
	int slot;
	u_short value;

	/* 
	 * the following instructions must be safe regardless of whose interrupt 
	 * this is 
	 */
	etest_reg = (struct etest_reg *)isc->if_reg_ptr;
	card_info = (struct etest_card_info *)isc->if_drv_data;
	bmic_reg  = (struct bmic_reg *)card_info->virt_bmic_ptr;


	/* lets share interrupts, was this ours or somebody elses? */
	if (!(inb(&etest_reg->dma_done))) { 
		/* no interrupt for EISA slave, ISA master, or forced int */
		if (bmic_reg == NULL) {
			printf("WHOA. Not supposed to happen. bmic == NULL in isr\n");
			return(0);
		}

		bmic_reg->local_index = ch0_status;
		if ((bmic_reg->local_data & E_TFR_COMPLETE) == 0) 
			/* let 1st level int hndlr know it is not us */
			return(0); 
	}

	/* 
	 * now we know we are here for us 
	 */
	bp = isc->owner;
	slot = ((struct eisa_if_info *)(isc->if_info))->slot;
	if (bp == NULL) {
		printf("Card in slot %d, bp is NULL\n", slot);
		panic("etest_isr: bp is NULL");
	}

	dma_parms = (struct dma_parms *)bp->b_s2;
	isc_dma_parms = (struct dma_parms *)isc->card_ptr;

	/* was it a forced interrupt test */
	if (inb(&etest_reg->dma_done) & E_FORCE_INTERRUPT) {
		etest_reg->dma_done &= ~E_FORCE_INTERRUPT;
		wakeup(&(etest_reg->dma_done));
		return(1);
	}

#ifdef EISA_TESTS
	/* tell instrumentation that we are here */
	ch_stat(0x10);
	value = ins(&etest_reg->dma_count);
	ch_stat(0xe2);
	ch_stat(value & 0xff);
	ch_stat((value>>8) & 0xff);
#endif

	/* are we handling multiple chains for ISA master */
	if ((card_info->flags & E_ISA_MASTER) && (dma_parms->chain_count != 0)) {

#ifdef EISA_TESTS
		/* tell instrumentation that we are here */
		ch_stat(0x11);
#endif

		/* write to card to disable channel */
		etest_reg->dma_ch_enable &= ~(1 << isc_dma_parms->channel);

		/* clear interrupting condition */
		outb(&etest_reg->dma_done, inb(&etest_reg->dma_done));

		/* load isa with next element in chain */
		load_isa_dma(isc);

		/* write to card to start DMA transfer */
		etest_reg->dma_ch_enable |= 1 << isc_dma_parms->channel;

		/* let the eisa handler know the interrupt has been claimed */
		return(1);
	}


	if ((card_info->flags & E_EISA_MASTER) && (dma_parms->chain_count > 0)) {

		/* clear interrupting condition */
		bmic_reg->local_index = ch0_status;
		bmic_reg->local_data = E_TFR_COMPLETE;

		load_eisa_master_dma(isc);

		/* write to card to start DMA transfer */
		bmic_reg->local_index = ch0_strobe;
		bmic_reg->local_data = 1;	/* write to reg, data doesn't matter */

		return(2);

	}

	/* disable interrupts */
		/* what about edge trig ints ???? */
	value = ins(&etest_reg->level_irqs) & ~(1 << card_info->etest_irq);
	outs(&etest_reg->level_irqs, value);

	if ((card_info->flags & E_EISA_SLAVE) || (card_info->flags & E_ISA_MASTER)) {
		/* get interrupting channel from card */
		channel = inb(&etest_reg->dma_done);

		if (channel != (1 << isc_dma_parms->channel)) /* ?? */
			panic("ETEST: bad DMA channel found in isr");

		/* clear interrupting condition */
		etest_reg->dma_done |= channel;	

		/* what about edge trig ints */
		value = ins(&etest_reg->level_irqs) & ~(1 << card_info->etest_irq);
		outs(&etest_reg->level_irqs, value);

		/* disable channel */
		etest_reg->dma_ch_enable &= ~(1 << isc_dma_parms->channel);

		/* calc resid, cleanup allocated buflet space, free dma_parms  */
		resid = eisa_dma_cleanup(isc, dma_parms);

		/* now calculate resid, for ISA master */
		if (card_info->flags & E_ISA_MASTER) {
			/* 0 on complete transfer? */
			if (inb(&etest_reg->dma_count) != 0)
				isc->owner->b_resid = inb(&etest_reg->dma_count)*2;	
			outb(&etest_reg->dma_tc, 0);
		} else { /* E_EISA_SLAVE */
			if (resid > 0)
				isc->owner->b_resid = resid;
		}

	} else {	/* E_EISA_MASTER */
		/* cleanup allocated buflet space, free dma_parms  */
		eisa_dma_cleanup(isc, dma_parms);

		/* clear interrupting condition */
		bmic_reg->local_index = ch0_status;
		bmic_reg->local_data = E_TFR_COMPLETE;

		/* calculate resid, if any */
		bmic_reg->local_index = ch0_curr_count_0;
		resid = (u_int)bmic_reg->local_data;
		bmic_reg->local_index = ch0_curr_count_1;
		resid += (u_int)(bmic_reg->local_data << 8);
		bmic_reg->local_index = ch0_curr_count_2;
		resid += (u_int)((bmic_reg->local_data & ~(E_START_DMA | E_BMIC_READ)) << 16);

		/* this handles BMIC errata */
		if (resid > 4)
			isc->owner->b_resid = resid;
		else isc->owner->b_resid = 0;

		/* wakeup anybody sleeping for eisa master use, this shouldn't be needed? */
		wakeup(bmic_reg);
	}

	iodone(isc->owner);

	/* free select code, wakeup anybody waiting */
	isc->owner = NULL;
	wakeup(isc);

	io_free(dma_parms, sizeof(struct dma_parms));

	/* re-enable the interrupt */
	value = ins(&etest_reg->level_irqs) | (1 << card_info->etest_irq);
	outs(&etest_reg->level_irqs, value);

	/* let the eisa handler know the interrupt has been claimed */
	return(1);
}
