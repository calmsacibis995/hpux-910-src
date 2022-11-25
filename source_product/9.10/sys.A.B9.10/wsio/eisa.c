/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/eisa.c,v $
 * $Revision: 1.7.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/20 12:21:36 $
 */

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/mman.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/malloc.h"
#include "../wsio/eisa.h"
#include "../wsio/eeprom.h"
#include "../h/io.h"
#include "../h/buf.h"
#include "../wsio/intrpt.h"
#if defined (__hp9000s800) && defined (_WSIO)
#include "../machine/pa_cache.h"
#endif

extern vas_t kernvas;
extern struct eeprom_per_slot_info     *eeprom_slot_info[];

/* Modules from eeprom.c */
extern int eeprom_exists();
extern int valid_eeprom_boot_area();

extern int (*eisa_eoi_routine)();
extern int (*eisa_init_routine)();
extern int (*eisa_nmi_routine)();
extern caddr_t map_mem_to_host();
extern caddr_t host_map_mem();
extern struct eisa_sysbrd_parms *get_eisa_sysbrd_parms();

#if defined (__hp9000s800) && defined (_WSIO)
extern int eisa_int();
#endif

int eisa_last_attach();

#ifdef __hp9000s300
extern caddr_t vtop();
#else
extern caddr_t ltor();
#endif

int eisa_eoi();
int eisa_nmi();

char eeprom_found = 0;
int eisa_exists = 0;
#if defined (__hp9000s800) && defined (_WSIO)
int eisa_boot_slot = -1;
#endif

struct bus_info_type *eisa[NUM_EISA_BUSES];

extern ushort eisa_int_enable;

#ifdef EISA_TESTS
unsigned char chaining_status[50];
unsigned char ch_stat_i=0;

#define STATE1       1
#define STATE2       2
#define STATE3       3
#define STATE4       4
#define STATE5       5
#define STATE6       6
#define ETEST_ISR 0x10
#define DMA_COMP  0x11

#endif

/* need an ident string for backpatch identification */
static char eisa_revision[] = "$Header: eisa.c,v 1.7.83.4 93/10/20 12:21:36 rpc Exp $";

/* Set catch all routine at end of chain */
int (*eisa_attach)() = eisa_last_attach; 

#ifdef EISA_TESTS
/*----------------------------------------------------------------------*/
ch_stat(value) /* DEBUG */
/*----------------------------------------------------------------------*/

unsigned char value;
{
	char i;

	if (value != 0xff) {
		if (ch_stat_i < 50)
			chaining_status[ch_stat_i++] = value;
	} else {
		for (i=0; i< 50; i++)
			chaining_status[i]=0;
		ch_stat_i = 0;
	}

}

/*----------------------------------------------------------------------*/
get_ch_stat(arg) /* DEBUG */
/*----------------------------------------------------------------------*/

unsigned char *arg;
{
	char i;
	for (i=0; i< 50; i++) {
		arg[i] = chaining_status[i];
		chaining_status[i] = 0;
	}
	ch_stat_i = 0;

}
#endif


/*----------------------------------------------------------------------*/
cvt_eisa_id_to_ascii(id, str)
/*----------------------------------------------------------------------*/
/* This routine allows displaying Product ID's in a friendly manner.
 * str should be the address of a memory area capabale of holding 8 chars
 */

int id;
char *str;
{
	int tid;

	str[0] = ((id & 0x7c000000) >> 26) + 0x40;
	str[1] = ((id & 0x03e00000) >> 21) + 0x40;
	str[2] = ((id & 0x001f0000) >> 16) + 0x40;

	tid = (id & 0x0000f000) >> 12;
	str[3] = (0 <= tid && tid <= 9) ? tid + '0' : (tid - 10) + 'A';

	tid = (id & 0x00000f00) >> 8;
	str[4] = (0 <= tid && tid <= 9) ? tid + '0' : (tid - 10) + 'A';

	tid = (id & 0x000000f0) >> 4;
	str[5] = (0 <= tid && tid <= 9) ? tid + '0' : (tid - 10) + 'A';

	tid = (id & 0x0000000f);
	str[6] = (0 <= tid && tid <= 9) ? tid + '0' : (tid - 10) + 'A';

	str[7] = 0;
}


/*----------------------------------------------------------------------*/
int cvt_ascii_to_eisa_id(str)
/*----------------------------------------------------------------------*/
/* This routine allows referencing Product ID's in a friendly manner.
 * str should be the address of a memory area holding 8 chars
 */

char *str;
{
	struct compressed_id {
		int res0        :1;     /* reserved (0)                       */
		int mcode1      :5;     /* first char of compressed mfg code  */
		int mcode2      :5;     /* second char of compressed mfg code */
		int mcode3      :5;     /* third char of compressed mfg code  */
		int pcode1      :4;     /* first char of product code         */
		int pcode2      :4;     /* second char of product code        */
		int pcode3      :4;     /* third char of product code         */
		int rcode1      :4;     /* first char of revision code        */
	};

	union work {
		struct compressed_id inval;
		int outval;
	} build;

        build.inval.mcode1 = str[0] - 0x40;
        build.inval.mcode2 = str[1] - 0x40;
        build.inval.mcode3 = str[2] - 0x40;
        build.inval.pcode1=(str[3] >= 'A' ? str[3] - 55 : str[3]-'0');
        build.inval.pcode2=(str[4] >= 'A' ? str[4] - 55 : str[4]-'0');
        build.inval.pcode3=(str[5] >= 'A' ? str[5] - 55 : str[5]-'0');
        build.inval.rcode1=(str[6] >= 'A' ? str[6] - 55 : str[6]-'0');
	return(build.outval);
}

#ifdef __hp9000s300
/*----------------------------------------------------------------------*/
eisa_isr_default(interrupt)
/*----------------------------------------------------------------------*/

struct interrupt *interrupt;
{
	struct eisa_system_board *esb = 
		((struct eisa_bus_info *)(eisa[EISA1]->spec_bus_info))->esb;

	short irr_mask, isr_mask;
	int i;

	esb->int1_ocw3 = OCW3 | READ_IRR;
	irr_mask = esb->int1_irr>>8;
	esb->int2_ocw3 = OCW3 | READ_IRR;
	irr_mask |= esb->int2_irr;
	esb->int1_ocw3 = OCW3 | READ_ISR;
	isr_mask = esb->int1_isr>>8;
	esb->int2_ocw3 = OCW3 | READ_ISR;
	isr_mask |= esb->int2_isr;
	for (i=0; i<= NUM_IRQ_PER_BUS; i++) {
		if ((i >= 0) && (i <= 7)) {
			if ((esb->int1_int_mask_reg & (1>>i)) == 0)
				if (((1>>i) & irr_mask) || ((1>>i) & isr_mask))
					panic("EISA: Autovectored interrupt from EISA, bad adapter hardware");
		} else {
			if ((esb->int2_int_mask_reg & (1>>i)) == 0)
				if (((1>>i) & irr_mask) || ((1>>i) & isr_mask))
					panic("EISA: Autovectored interrupt from EISA, bad adapter hardware");
		}
	}
	interrupt->chainflag = 1;
}
#endif

/*----------------------------------------------------------------------*/
eisa_interrupt_init(esb)
/*----------------------------------------------------------------------*/
/* Set up interrupt controller of EISA converter                        
 */

register struct eisa_system_board *esb;
{
	/* 
	 * init ICW's of master interupt controller 
	 */

	/* multiple interrupt controllers on this EISA bus, need to init ICW4 */
	esb->int1_icw1 = ICW4_USED | ICW1; 
	esb->int1_icw2 = EISA1_MASTER_VECTOR; 	/* EISA master interrupt vector */
	esb->int1_icw3 = MASTER;		/* I have a slave controller */

	/* no auto-EOI, special fully nested mode, other bits as required for EISA */
	esb->int1_icw4 = SFNM | uPM; 	        
	esb->int1_int_mask_reg = 0xff;	        /* OCW1 disable all interrupts */

	/* 
	 * init ICW's of slave controller 
	 */

	/* multiple interrupt controllers on this EISA bus, need to init ICW4 */
	esb->int2_icw1 = ICW4_USED | ICW1; 

	esb->int2_icw2 = EISA1_SLAVE_VECTOR; 	/* EISA slave interrupt vector */
	esb->int2_icw3 = SLAVE;			/* slave ID = mster IRQ line for cascading */

	/* no auto-EOI, special fully nested mode, other bits as required for EISA */
	esb->int2_icw4 = SFNM | uPM; 	        
	esb->int2_int_mask_reg = 0xff;	        /* disable all interrupts */

	/* zero edge/level triggered registers */
	esb->int1_edge_level = 0;               /* ensure set to edge sensitive */
	esb->int2_edge_level = 0;

	/* nmi initialization - clear all nmi sources before enabling any */
	esb->nmi_status = PARITY_DISABLED | IOCHK_DISABLED;  /* disable to clear any nmi's */
	esb->nmi_status = PARITY_DISABLED | !IOCHK_DISABLED; /* enable iochk nmi sources */

#ifdef __hp9000s300
	/* add EISA autovector routine to DIO interrupt chain to catch bad
         * hardware, before enabling interrupts.  We know the parity nmi
	 * is always disabled, so we use that bit to get us into the isr (XXX) 
	 */
	isrlink(eisa_isr_default, 5, esb->nmi_status, PARITY_DISABLED, 0, 0, 0);

	esb->nmi_enable = 0;	             /* all extended sources cleared and disabled */
#else /* snakes */
	/* nmi was not enabled on 400 due to INTEL bug */
        esb->nmi_enable = 0;                 /* all extended sources cleared and disabled */
        esb->nmi_enable = BUS_TO_ENABLED;    /* enable bus timeout nmi source */

        asp_isrlink(eisa_int, EISA_INT_LINE, 0, 0);
        asp_isrlink(eisa_nmi, EISA_NMI_INT_LINE, 0, 0);
#endif /* snakes */
}


/*----------------------------------------------------------------------*/
eisa_dma_init(esb)
/*----------------------------------------------------------------------*/
/* Set up DMA controllers                                               
 */

register struct eisa_system_board *esb;
{
	/* dma controller 0-3 initialization */
	esb->dma0_master_clr = 0;
	esb->dma1_master_clr = 0;

	esb->dma0_command = 0;		/* channels enabled, fixed priority, DERQ high, DAK low */

	/* record values in software copies where needed */
	esb->dma0_mode = CHANNEL_0_SELECT;	/* autoinit off, address increment, demand mode */
	esb->dma0_mode = CHANNEL_1_SELECT;	/* autoinit off, address increment, demand mode */
	esb->dma0_mode = CHANNEL_2_SELECT;	/* autoinit off, address increment, demand mode */
	esb->dma0_mode = CHANNEL_3_SELECT;	/* autoinit off, address increment, demand mode */

	/* stop registers disabled */
	esb->dma0_extended_mode = CHANNEL_0_SELECT | STOP_DISABLED;
	esb->dma0_extended_mode = CHANNEL_1_SELECT | STOP_DISABLED;
	esb->dma0_extended_mode = CHANNEL_2_SELECT | STOP_DISABLED;
	esb->dma0_extended_mode = CHANNEL_3_SELECT | STOP_DISABLED;

	esb->dma0_mask_all = 0xff;	/* mask all dma channels */
	esb->dma0_chaining_mode = CHANNEL_0_SELECT;	/* chaining off */
	esb->dma0_chaining_mode = CHANNEL_1_SELECT;	/* chaining off */
	esb->dma0_chaining_mode = CHANNEL_2_SELECT;	/* chaining off */
	esb->dma0_chaining_mode = CHANNEL_3_SELECT;	/* chaining off */

	/* dma controller 4-7 initialization */
	esb->dma1_command = 0;		/* channels enabled, fixed priority, DERQ high, DAK low */

	/* record values in software copies where needed */
	esb->dma1_mode = CHANNEL_4_SELECT | CASCADE_MODE; /* autoinit off, address increment, 
							     cascade mode */
	esb->dma1_mode = CHANNEL_5_SELECT;	/* autoinit off, address increment, demand mode */
	esb->dma1_mode = CHANNEL_6_SELECT;	/* autoinit off, address increment, demand mode */
	esb->dma1_mode = CHANNEL_7_SELECT;	/* autoinit off, address increment, demand mode */

	/* stop registers disabled */
	esb->dma1_extended_mode = CHANNEL_4_SELECT | STOP_DISABLED;
	esb->dma1_extended_mode = CHANNEL_5_SELECT | STOP_DISABLED;
	esb->dma1_extended_mode = CHANNEL_6_SELECT | STOP_DISABLED;
	esb->dma1_extended_mode = CHANNEL_7_SELECT | STOP_DISABLED;

	esb->dma1_mask_all &= ~SET_CHANNEL4_MASK; /* ch 4 must be unmasked to use 0-3 */
	esb->dma1_chaining_mode = CHANNEL_4_SELECT;	/* chaining off */
	esb->dma1_chaining_mode = CHANNEL_5_SELECT;	/* chaining off */
	esb->dma1_chaining_mode = CHANNEL_6_SELECT;	/* chaining off */
	esb->dma1_chaining_mode = CHANNEL_7_SELECT;	/* chaining off */
}

#ifdef __hp9000s300
/*----------------------------------------------------------------------*/
eisa_expander_exists(esb)
/*----------------------------------------------------------------------*/
/* Test for existence of EISA expander in box                           
 */

register struct eisa_system_board *esb;
{
	/* test for existence of EISA expander in box */
	if (testr(&esb->dma0_ch0_count, 1)) {
		esb->dma0_clr_byte_ptr = 1;
		esb->dma0_ch0_count = 0xaa;
		esb->dma0_ch0_count = 0x55;
		esb->dma0_clr_byte_ptr = 1;
		if (esb->dma0_ch0_count != 0xaa) 
			return 0;
		if (esb->dma0_ch0_count != 0x55) 
			return 0;
		if (testr(&esb->dma0_ch0_high_count, 1)) {
			esb->dma0_ch0_high_count = 0xaa;
			if (esb->dma0_ch0_high_count != 0xaa) 
				return 0;
			else return 1;
		}
		else return 0;	/* must be ISA, not EISA */
	} else return 0;
}
#endif


/*---------------------------------------------------------------------------*/
initialize_slot(slot)
/*---------------------------------------------------------------------------*/
/* This  routine  is called  for each slot  (slots 1 -4 only)  requiring
 * initialization.  In the case of eisa  boot,  only for the boot  slot,
 * otherwise  this  routine  is called for each slot in the  system.  It
 * does error checking:  eeprom data match card,  initializes  each card
 * with eeprom  data.  It then calls the attach  chain and fills out the
 * isc entrys on sucessful initialization.
 */

int slot;
{
	struct eeprom_slot_data slot_data;
	int slot_is_isa;
	int i, id, val, return_value;
	struct isc_table_type *isc;
	struct eeprom_function_data func_data;
	char str1[8], str2[8];
	register struct bus_info_type *eisa1;
	struct isc_table_type *esb_isc; 
	caddr_t addr;

	eisa1 = eisa[EISA1];
	esb_isc = eisa1->isc;

	/* read eeprom for this slot, return info in slot_data */
	if ((val = read_eisa_slot_data(slot, &slot_data)) < 0) {
		/* Slot is empty */ 
		if (val == -2) {	
			if ((addr = map_mem_to_host(esb_isc, 0x1000*slot, 1)) == NULL)  
				return(0);

			if ((id = read_eisa_cardid(addr)) != 0) {
				cvt_eisa_id_to_ascii(id, str1);
				printf("No EEPROM Data -> EISA Card ID: %s ",str1);
			}

			unmap_mem_from_host(esb_isc, addr, 1);
		}
		return(0);
	}

	/* get space for isc information */
	isc = (struct isc_table_type *)kmem_alloc(sizeof(struct isc_table_type));
	/* init per slot data structure fields in isc structure */
	bzero(isc, sizeof(struct isc_table_type));
		
	/* check to see if this is an ISA card */
 	if (eeprom_slot_info[slot - 1]->minor_cfg_ext_rev & SLOT_IS_ISA)
		  slot_is_isa = 1;
	else slot_is_isa = 0;

	/* if not isa, map in registers and check ids */
	if (!slot_is_isa) {
		if ((isc->if_reg_ptr = map_mem_to_host(esb_isc, 0x1000*slot, 1)) == NULL) {
			kmem_free(isc, sizeof(struct isc_table_type));
			return(0);
		}

		if ((slot_data.slot_info & NON_READABLE_PID) == 0) {
			if ((id = read_eisa_cardid(isc->if_reg_ptr)) == 0) {
				printf("Cannot read EISA card ID");
				unmap_mem_from_host(esb_isc, isc->if_reg_ptr, 1);
				kmem_free(isc, sizeof(struct isc_table_type));
				return(0);
			}
			if (id != slot_data.slot_id) {
				cvt_eisa_id_to_ascii(id, str1);
				cvt_eisa_id_to_ascii(slot_data.slot_id, str2);
				printf("Board ID: %s inconsistent with NVM ID: %s",str1,str2);
				unmap_mem_from_host(esb_isc, isc->if_reg_ptr, 1);
				kmem_free(isc, sizeof(struct isc_table_type));
				return(0);
			}
		} else id = 0;
	} else id = 0; 

	/* init per slot data structure fields */
	isc->if_id = id;
	isc->bus_type = EISA_BUS;
	isc->bus_info = eisa1;
	isc->if_info  = (caddr_t)kmem_alloc(sizeof(struct eisa_if_info));
	bzero(((struct eisa_if_info *)(isc->if_info)), sizeof(struct eisa_if_info));
	isc->gfsw = (struct gfsw *)kmem_alloc(sizeof(struct gfsw));
	bzero(isc->gfsw, sizeof(struct gfsw));
	isc->next_ftn = NULL;
	isc->ftn_no = -1;
	isc->my_isc = EISA1_MIN + slot;
	((struct eisa_if_info *)(isc->if_info))->slot = slot;

	return_value = 0;
	/* initialize this card based on eeprom information */
	for (i = 0; i < slot_data.number_of_functions; i++) {
		return_value = read_eisa_function_data(slot, i, &func_data);
		if (return_value == -1) {
			cvt_eisa_id_to_ascii(id, str1);
			printf("Bad eeprom data for board %s", str1);
			unmap_mem_from_host(esb_isc, isc->if_reg_ptr, 1);
			kmem_free(isc->if_info, sizeof(struct eisa_if_info));
			kmem_free(isc->gfsw, sizeof(struct gfsw));
			kmem_free(isc, sizeof(struct isc_table_type));
			return(0);
		}
		if (func_data.slot_features & EISA_IOCHKERR)
			((struct eisa_if_info *)(isc->if_info))->flags = HAS_IOCHKERR;
		if (slot_is_isa)
			((struct eisa_if_info *)(isc->if_info))->flags |= IS_ISA_CARD;
		/* initialize this function */
		return_value = eisa_card_init(isc, &func_data);
		if (return_value == -1) {
			cvt_eisa_id_to_ascii(id, str1);
			printf("Error initializing board %s", str1);
			unmap_mem_from_host(esb_isc, isc->if_reg_ptr, 1);
			kmem_free(isc->if_info, sizeof(struct eisa_if_info));
			kmem_free(isc->gfsw, sizeof(struct gfsw));
			kmem_free(isc, sizeof(struct isc_table_type));
			return(0);
		}
	} /* for number of functions */

	/* call attach routines */
	return_value = (*eisa_attach)(id, isc);

	/* check attach routine return values */
	switch (return_value) {
		case ATTACH_OK:
			/* attach routine returned successful, set isc_table pointer */
			isc_table[EISA1_MIN + slot] = isc;
			/* if EISA, hit card's enable bit */
			if (!(((struct eisa_if_info *)(isc->if_info))->flags & IS_ISA_CARD))
				*(isc->if_reg_ptr+CARD_CONTROL_OFFSET) = EISA_CARD_ENABLE;
			break;

		case ATTACH_NO_DRIVER:
			cvt_eisa_id_to_ascii(slot_data.slot_id, str1);
		  	if (id == 0) {
				printf("EISA Board ID: %s ignored\n", str1);
				printf("\tBoard not present or driver not configured into kernel.");
			} else {
			 	printf("EISA Board ID: %s ignored, driver not configured into kernel", str1);
			}
			if (!slot_is_isa && isc->if_reg_ptr) {
				unmap_mem_from_host(esb_isc, isc->if_reg_ptr, 1);
			}
			kmem_free(isc->if_info, sizeof(struct eisa_if_info));
			kmem_free(isc->gfsw, sizeof(struct gfsw));
			kmem_free(isc, sizeof(struct isc_table_type));
			return(0); 
		
		case ATTACH_INIT_ERROR:
			cvt_eisa_id_to_ascii(id, str1);
			printf("EISA Board ID: %s ignored, error initializing board", str1);
			if (!slot_is_isa && isc->if_reg_ptr) {
				unmap_mem_from_host(esb_isc, isc->if_reg_ptr, 1);
			}
			kmem_free(isc->if_info, sizeof(struct eisa_if_info));
			kmem_free(isc->gfsw, sizeof(struct gfsw));
			kmem_free(isc, sizeof(struct isc_table_type));
			return(0);

		default:  panic("EISA: eisa_last_attach not called, bad driver in kernel");
	}

	/* return successful */
	return(1);
}


/*----------------------------------------------------------------------*/
eisa_init()
/*----------------------------------------------------------------------*/
/* Initialize  EISA  system  board  etc.  This  routine  is called  from
 * kernel_initialize (300) and vsc_mod_table_traverse (700).
 */

{
	int i, eisa_id, fd, iterations;
	register int slot;
	struct isc_table_type *isc;
	struct isc_table_type *esb_isc;
	struct eisa_system_board *esb; 
	struct eisa_bus_info *eisa1;
	struct iomap_data *iomap_temp;
	caddr_t reg_ptr;
	char str[8];
#if defined (__hp9000s800) && defined (_WSIO)
	u_char dummy;
#endif

	eisa_exists = 0;

	/*
	 * allocate and bzero bus info structures 
	 */
	eisa[EISA1] = (struct bus_info_type *)kmem_alloc(sizeof(struct bus_info_type));
	bzero(eisa[EISA1], sizeof(struct bus_info_type));

	eisa[EISA1]->spec_bus_info=(char *)kmem_alloc(sizeof(struct eisa_bus_info));
	bzero(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));

	/*
	 * begin initializing generic bus_info structure
	 */
	eisa[EISA1]->bus_type = EISA_BUS;
	eisa[EISA1]->isc = (struct isc_table_type *)kmem_alloc(sizeof(struct isc_table_type));

	esb_isc = eisa[EISA1]->isc;
	esb_isc->bus_type = EISA_BUS;
	esb_isc->bus_info = eisa[EISA1]; 

	/* 
	 * begin initializing fields of eisa_bus_info structure 
	 */
	eisa1 = (struct eisa_bus_info *)eisa[EISA1]->spec_bus_info;
	eisa1->eisa_base_addr = (caddr_t)EISA1_BASE_ADDR;
	eisa1->dma0_status_sw = 0;
	eisa1->dma1_status_sw = 0;

	/* 
	 * map in EISA1 system board space 
	 */
	if ((eisa1->esb = (struct eisa_system_board *)
	    host_map_mem((caddr_t)EISA1_BASE_ADDR, NBPG)) == NULL) {
		kmem_free(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));
		kmem_free(eisa[EISA1], sizeof(struct bus_info_type));
		printf("EISA WARNING: mapping in system board failed\n");
		return(1);
	}

	/* pointer to hardware registers */
	esb = eisa1->esb;

	/*
	 * determine if the eisa expander exists
	 */
#ifdef __hp9000s300
	if (eisa_expander_exists(esb)) {
		eisa_exists = 1;
	} else {
		unmap_host_mem(eisa1->esb, 1); 
		kmem_free(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));
		kmem_free(eisa[EISA1], sizeof(struct bus_info_type));
		return(1);
	}
#else
	/* if eisa_init got called on the 700, PDC reported Mongoose */
	eisa_exists = 1;
#endif

	/*
	 * Need to get  parameters  for  the  system  board.  We  keep a
	 * separate  copy  of the  number  of  slots  since  it  will be
	 * overwritten  by the  eeprom's  stored  value if the eeprom is
	 * neither corrupt nor new.
	 */
	eisa1->sysbrd_parms = get_eisa_sysbrd_parms(read_eisa_cardid(esb));
	eisa1->number_of_slots = eisa1->sysbrd_parms->num_slots;

#if defined (__hp9000s800) && defined (_WSIO)
	/*
	 * map in mongoose specific hardware registers
	 */

        /* map in EISA1 iack register space on mongoose */
	if ((eisa1->iack_reg = host_map_mem((caddr_t)EISA1_IACK_REG, NBPG)) == NULL) {
		kmem_free(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));
		kmem_free(eisa[EISA1], sizeof(struct bus_info_type));
                printf("EISA WARNING: mapping in system board failed\n");
                return(1);
	}

        /* map in EISA1 mongoose register space */
	if ((eisa1->mongoose_reg = host_map_mem((caddr_t)EISA1_MG_REG_BASE, NBPG)) == NULL) {
		kmem_free(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));
		kmem_free(eisa[EISA1], sizeof(struct bus_info_type));
                printf("EISA WARNING: mapping in system board failed\n");
                return(1);
	}

        eisa1->mongoose_reg->liowait = 1;       /* no liowait delays */
        eisa1->mongoose_reg->bus_lock = 1;      /* unlock bus */
#endif /* snakes */

	/* INTEL ERRATA */
	/* reset EISA bus -- also fixes Intel chip bug with powering up interrupting */
	esb->nmi_extended_status  =  RESET_SYSTEM_BUS;
	/* 
	 * SPEC says wait long enough for devices to reset. Okay, we didn't wait before 
	 * and things appeared to work, so 5 millisecs should be even better...Right
	 */
	snooze(5000); 
	esb->nmi_extended_status &= ~RESET_SYSTEM_BUS;

	/*
	 * see if the eeprom exists. We are in trouble if it isn't there
	 */
	if (eeprom_exists() == 0) 
		panic("EISA: Invalid Hardware, EEPROM Missing");
	else 
		eeprom_found = 1;

	/* 
	 * map in and initialize iomap table 
	 */
	if ((eisa[EISA1]->host_iomap_base = (int *)host_map_mem((caddr_t)EISA1_IOMAP_ADDR, 
		eisa1->sysbrd_parms->size_iomap * NBPG)) == NULL) {
		kmem_free(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));
		kmem_free(eisa[EISA1], sizeof(struct bus_info_type));
		printf("EISA WARNING: mapping in I/O map entries failed\n");
		return(1);
	}

	/*
	 * get the EISA address of the iomap hardware
	 */
	eisa[EISA1]->bus_iomap_base = (int *)(EISA1_IOMAP_ADDR - EISA1_BASE_ADDR);

	/*
	 * set up the iomap resource map
	 */
	eisa[EISA1]->iomap_rsrcmap = (struct rsrc_node *)kmem_alloc(sizeof(struct rsrc_node));
	if (eisa[EISA1]->iomap_rsrcmap == NULL) {
		kmem_free(eisa[EISA1]->spec_bus_info, sizeof(struct eisa_bus_info));
		kmem_free(eisa[EISA1], sizeof(struct bus_info_type));
		printf("eisa_init: memory allocation failure\n");
		return(1);
	}

	/*
	 * initialize header node
	 */
	bzero(eisa[EISA1]->iomap_rsrcmap, sizeof(struct rsrc_node));
	eisa[EISA1]->iomap_rsrcmap->next = eisa[EISA1]->iomap_rsrcmap;
	eisa[EISA1]->iomap_rsrcmap->prev = eisa[EISA1]->iomap_rsrcmap;
	

	if ((i = free_rsrc_entries(eisa[EISA1]->iomap_rsrcmap, 0, 
		eisa1->sysbrd_parms->size_iomap)) < 0) {

		printf("eisa_init: iomap rsrc allocation failure: %d\n", i);
		return(1);
	}

	/*
	 * begin system board hardware initialization
	 */


	/* 
	 * init interrupt controllers on system board to power on state 
	 */
	eisa_interrupt_init(esb);

#if defined (__hp9000s800) && defined (_WSIO)
        /* read iack reg until we get three 7's */
        for (i=0, iterations = 0; i != 3;) {
                dummy = *eisa1->iack_reg;
		(*eisa_eoi_routine)(dummy);
		if (dummy == 7) i++;
		if (iterations++ >= 100)
			panic("EISA: Bad EISA Interrupt Hardware");
	}
#endif /* snakes */	

	/* 
	 * init refresh and interval timers 
	 */
	esb->timer1_control = COUNTER_1_SELECT | RW_LEAST;  /* refresh disabled */
	esb->timer1_control = COUNTER_0_SELECT | RW_LEAST;  /* disable timer1, counter0 */	
	esb->timer2_control = COUNTER_0_SELECT | RW_LEAST;  /* disable timer2, counter0 */
	esb->timer2_control = COUNTER_2_SELECT | RW_LEAST;  /* disable timer2, counter2 */

	/* must 0 this register, or some boards may have problems */
	esb->refresh_low_page = 0;

	/* 
	 * init DMA controllers on system board to power on state 
	 */
	eisa_dma_init(esb);

	/* 
	 * open the eeprom driver 
	 */
	if (fd = eeprom_open(makedev(EEPROM_MAJ, 0), 0)) {
		panic("EISA: Data in configuration EEPROM corrupt"); 
	}

	/* 
	 * if we get here, we have initialized  everything  correctly and
	 * we have the number of eeprom slots from eeprom.
	 */
	cvt_eisa_id_to_ascii(eisa1->sysbrd_parms->id, str);
	printf("\n    %d Slot EISA Expander Initialized: %s\n", eisa1->number_of_slots, str);

#if defined (__hp9000s800) && defined (_WSIO)
	/* 
	 * begin attempting to identify and initialize the cards that are found. 
	 */

	/* did we boot from an eisa device */
	if ((slot = valid_eeprom_boot_area()) != SLOT_IS_EMPTY) {
		if (slot < 1 || slot > eisa1->number_of_slots)
			panic("EISA: Corrupt EISA Boot Information");

		printf("    Slot %d: ", slot);
		if (initialize_slot(slot)) 
			printf("    Successfully Initialized EISA Boot Device\n");
		else
			panic("EISA: Cannot initialize EISA Boot Device");
		eisa_boot_slot = slot;
	} else { 
#endif
		/* 
		 * Normal card search power up sequence 
		 */
		for (slot = 1; slot <= eisa1->number_of_slots; slot++) {
			printf("    Slot %d: ", slot);
			if (!initialize_slot(slot))  
				printf(" -- Skipping\n");
		}
#if defined (__hp9000s800) && defined (_WSIO)
	}  /* No new boot area information found */
#endif


	/*
	 * loop through drivers calling init routines.  A multi-function
	 * card may have an isc entry  associated  with  each  function,
	 * each of which may have an init routine to call.
	 */
	for (slot = 1; slot <= eisa1->number_of_slots; slot++) {
		isc = isc_table[EISA1_MIN + slot];
		while (isc != NULL) {
	     		if (isc->gfsw != NULL && isc->gfsw->init != NULL)
				(*isc->gfsw->init)(isc);
			isc = isc->next_ftn;
		}
	}

	/*
	 * enable interrupts
	 */
	esb->int1_int_mask_reg = eisa_int_enable & 0xff;
	esb->int2_int_mask_reg = eisa_int_enable >> 8;

	/* close since mutually exclusive access */
	eeprom_close(fd);
}


/*------------------------------------------------------------------------*/
read_eisa_slot_id(slot)
/*------------------------------------------------------------------------*/
/* this routine is called from an eeprom ioctl. 
 */

int	slot;
{
	int	slot_id;
	caddr_t	reg_ptr;
	struct isc_table_type *esb_isc;

	esb_isc = eisa[EISA1]->isc;

	if (slot < 0 || slot > ((struct eisa_bus_info *)
		(eisa[EISA1]->spec_bus_info))->number_of_slots)
		return(0);

	if ((reg_ptr = map_mem_to_host(esb_isc, 0x1000*slot, 1)) == NULL)
		return(0);

	slot_id = read_eisa_cardid(reg_ptr);
	unmap_mem_from_host(esb_isc, reg_ptr, 1);

	return(slot_id);
}


/*------------------------------------------------------------------------*/
read_eisa_cardid(address)
/*------------------------------------------------------------------------*/
/* This routine is written per the EISA spec, the  algorithm is dictated
 * by the spec page 390.
 *
 * The  routine  returns 0 if it cannot  read the id.  A non-zero  value
 * indicates a readable id.
 */

register caddr_t address;
{
	register unsigned char first_byte;
	register unsigned int id;

	/* prime the bus line capacitors */
	*(address+CARD_ID_OFFSET) = 0xff;

	/* attempt to read the id */
	first_byte = *(address+CARD_ID_OFFSET);
	
	/* see if they responded with a not-ready */
	if ((first_byte & 0xf0) == 0x70) { 
		/* must wait 100 milliseconds for card to respond, snooze in usecs */
		snooze(100000);

		/* times up, attempt to read the id */
		first_byte = *(address+CARD_ID_OFFSET);

		/* if still not ready, bail out */
		if ((first_byte & 0xf0) == 0x70)  
			return(0);
	}

	/* bail out if we read bus line values or the always = 0 MSB is 1 */
	if (first_byte == 0xff || first_byte & 0x80)
		return(0);

	/* piece together the id */
	id = first_byte << 24;
	id += *((unsigned char *)(address+CARD_ID_OFFSET+1)) << 16;
	id += *((unsigned char *)(address+CARD_ID_OFFSET+2)) << 8;
	id += *((unsigned char *)(address+CARD_ID_OFFSET+3));

	return(id);
}
	

/*------------------------------------------------------------------------*/
eisa_card_init(isc, func_data)
/*------------------------------------------------------------------------*/
/* This routine  performs powerup  initialization  of cards whose eeprom
 * data matches  card ids.  It does this by looking at the init data and 
 * writing to the indicated registers, the indicated values.
 */

struct isc_table_type *isc;
register struct eeprom_function_data *func_data;
{
	struct eisa_bus_info *eisa_bus = 
		(struct eisa_bus_info *)(isc->bus_info->spec_bus_info);

	struct eisa_system_board *esb = eisa_bus->esb;
	register caddr_t card_address = isc->if_reg_ptr;	/* address of card */
	u_char int_config;
	u_char *dma_config;
	unsigned short this_int, word_port_value, word_port_mask, word_read;
	caddr_t port_address; 
	u_char this_channel, init_type, byte_port_value, byte_port_mask, byte_read;
	unsigned long_port_value, long_port_mask, long_read, i;
	
	/* if function info is freeform data, exit, driver will have to init */
	if (func_data->function_info & CFG_FREEFORM)
		return(0);

	if (func_data->function_info & HAS_IRQ_ENTRY) {  /* we have interrupt entries */
		for (i = 0; i < MAX_INT_ENTRIES; i++) {
			int_config = func_data->intr_cfg[i][0];
			this_int = int_config & INT_IRQ_MASK;
			if ((this_int < 3) || (this_int == 8) || (this_int == 13))
				return(-1);
			eisa_int_enable &= ~(1 << this_int);

			/* set edge/level indication for this interrupt level */
			if (int_config & INT_LEVEL_TRIG) 
				if ((this_int >= 0) && (this_int <= 7)) 
					esb->int1_edge_level |= (1<<this_int);
				else
					esb->int2_edge_level |= (1<<(this_int-8));

			if ((int_config & INT_MORE_ENTRIES) == 0)
				break;
		}
	}

	if (func_data->function_info & HAS_DMA_ENTRY) {  /* we have dma entries */
		for (i = 0; i < MAX_DMA_ENTRIES; i++) {
			dma_config = func_data->dma_cfg[i];
			this_channel = dma_config[0] & DMA_CHANNEL_MASK;
			if (this_channel == 4)
				return(-1);

			/* set timing & size bits as in eeprom for channel */
			if ((this_channel >= 0) && (this_channel <= 3)) {
				eisa_bus->dma_channels[this_channel].dma_extended_mode_sw =
					(eisa_bus->dma_channels[this_channel].dma_extended_mode_sw
							& (~ADDR_MODE_MASK))
					| (dma_config[1] & DMA_SIZE_MASK);
				eisa_bus->dma_channels[this_channel].dma_extended_mode_sw =
					(eisa_bus->dma_channels[this_channel].dma_extended_mode_sw
							& (~CYCLE_TIMING_MASK))
					| (dma_config[1] & DMA_TIMING_MASK);
				esb->dma0_extended_mode = this_channel | 
					eisa_bus->dma_channels[this_channel].dma_extended_mode_sw;
			} else {
				eisa_bus->dma_channels[this_channel].dma_extended_mode_sw =
					(eisa_bus->dma_channels[this_channel].dma_extended_mode_sw
							& (~ADDR_MODE_MASK))
							| (dma_config[1] & DMA_SIZE_MASK);
				eisa_bus->dma_channels[this_channel].dma_extended_mode_sw =
					(eisa_bus->dma_channels[this_channel].dma_extended_mode_sw
							& (~CYCLE_TIMING_MASK))
							| (dma_config[1] & DMA_TIMING_MASK);
				esb->dma1_extended_mode = (this_channel - 4) | 
					eisa_bus->dma_channels[this_channel].dma_extended_mode_sw;
			}

			if ((dma_config[0] & DMA_MORE_ENTRIES) == 0)
				break;
		}
	}

	/* 
	 * the layout of the port init entries is defined by the first byte of
	 * each entry, so read and interpret format and process each entry 
	 */

        if (((((struct eisa_if_info *)(isc->if_info))->flags & IS_ISA_CARD) == 0)
			 && ((func_data->slot_info & NON_READABLE_PID) == 0)) {

	if (func_data->function_info & HAS_PORT_INIT) {
		i = 0;
		for (;;) {
			init_type = func_data->init_data[i++];
			port_address = (caddr_t)(func_data->init_data[i++]);
			port_address = (caddr_t)((int)(port_address) + (func_data->init_data[i++] << 8));
			port_address = ((caddr_t)(card_address + (EISA_REG_PAGE & (int)port_address)));
			if (init_type & INIT_MASK) {
				if (init_type & INIT_ACCESS_DWORD) {
					long_port_value = func_data->init_data[i++]; 
					long_port_value += (func_data->init_data[i++] << 8);
					long_port_value += (func_data->init_data[i++] << 16);
					long_port_value += (func_data->init_data[i++] << 24);
					long_port_mask = func_data->init_data[i++];
					long_port_mask += (func_data->init_data[i++] << 8);
					long_port_mask += (func_data->init_data[i++] << 16);
					long_port_mask += (func_data->init_data[i++] << 24);
					long_read = inl(port_address);
					outl((u_int *)port_address, ((long_read & long_port_mask) | long_port_value));
				} else if (init_type & INIT_ACCESS_WORD) {
					word_port_value = (func_data->init_data[i++]);
					word_port_value += (func_data->init_data[i++] << 8);
					word_port_mask = func_data->init_data[i++];
					word_port_mask += (func_data->init_data[i++] << 8);
					word_read = ins(port_address);
					outs((u_short *)port_address, ((word_read & word_port_mask) | word_port_value));
				} else { /* INIT_ACCESS_BYTE */
					byte_port_value = func_data->init_data[i++];
					byte_port_mask = func_data->init_data[i++];
					byte_read = inb(port_address);
					/* see spec, rev 3.1, p 381 */
					outb((u_char *)port_address, ((byte_read & byte_port_mask) | byte_port_value));
				}
			} else { /* no mask */
				if (init_type & INIT_ACCESS_DWORD) {
					long_port_value = func_data->init_data[i++];
					long_port_value += (func_data->init_data[i++] << 8);
					long_port_value += (func_data->init_data[i++] << 16);
					long_port_value += (func_data->init_data[i++] << 24);
					outl((u_int *)port_address, long_port_value);
				} else if (init_type & INIT_ACCESS_WORD) {
					word_port_value = func_data->init_data[i++];
					word_port_value += (func_data->init_data[i++] << 8);
					outs((u_short *)port_address, word_port_value);
				} else 
					outb((u_char *)port_address, func_data->init_data[i++]);
			}

			if ((init_type & INIT_MORE_ENTRIES) == 0)
				break;
		}
		}
	}
}


/*------------------------------------------------------------------------*/
eisa_last_attach(id, isc)
/*------------------------------------------------------------------------*/
/* This routine marks the end of the eisa attach chain. It MUST be 
 * called.
 */

register int id;
register struct isc_table_type *isc;
{
	/* causes powerup code to ignore card, at driver's request */
	if (((struct eisa_if_info *)(isc->if_info))->flags & INIT_ERROR)
		return(ATTACH_INIT_ERROR);

	/* card has been accepted by a driver */
	if (((struct eisa_if_info *)(isc->if_info))->flags & INITIALIZED)
		return(ATTACH_OK);

	/* 
	 * no driver configured in for card - return of 1 causes powerup
	 * code to print no driver in kernel message for this card
	 */
	return(ATTACH_NO_DRIVER);	
} 


/*------------------------------------------------------------------------*/
caddr_t map_isa_address(isc, isa_address)
/*------------------------------------------------------------------------*/
/* Map an 8 byte segment of ISA registers into host virtual space.        
 */

struct isc_table_type *isc;
register unsigned int isa_address;
{
	register unsigned int host_address;
	
	/* 
	 * this incredibly  complex looking algorithm  ensures that each
	 * grouping of 8 ISA IO addresses  sits in a different  4K page.
	 * This   allows  vm  page   protection   to  protect  IO  space
	 * allocations  much  the  same  way it  protects  EISA  address
	 * ranges.  For  details  see  Mongoose  ERS or  Trailways  EISA
	 * adapter ERS.
	 */
	host_address = (isa_address & 0x03f8) << 9;
	host_address += (isa_address & 0xfc00) >> 6;
	host_address += isa_address & 0x0007;

	/* now that we have  generated an ISA  address, get a virtual to
	 * physical translation.
	 */
	return (map_mem_to_host(isc, host_address, 1));
}


/*------------------------------------------------------------------------*/
eisa_refresh_on(isc)
/*------------------------------------------------------------------------*/
/* Turn on refresh cycles for ISA memory.                                 
 */

struct isc_table_type *isc;
{
	struct eisa_system_board *esb = 
		((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;

	esb->timer1_control = COUNTER_1_SELECT | RW_LEAST | MODE_2;
	esb->timer1_refresh_rqst = 0x12;
}


#if defined(__hp9000s800) && defined(_WSIO)
/*------------------------------------------------------------------------*/
set_liowait(isc)
/*------------------------------------------------------------------------*/
/* provides ability to set liowait condition for duration.                
 */
 
struct isc_table_type *isc;
{
	register struct eisa_bus_info *eisa1 = 
		((struct eisa_bus_info *)(isc->bus_info->spec_bus_info));

        eisa1->mongoose_reg->liowait = 0;       /* set liowait delays */
}


/*------------------------------------------------------------------------*/
int read_eisa_bus_lock(isc)
/*------------------------------------------------------------------------*/
/* provides ability to do successive locked bus cycles to eisa space.     
 */

struct isc_table_type *isc;
{
	register struct eisa_bus_info *eisa1 = 
		((struct eisa_bus_info *)(isc->bus_info->spec_bus_info));

        return(eisa1->mongoose_reg->bus_lock);
}


/*------------------------------------------------------------------------*/
lock_eisa_bus(isc)
/*------------------------------------------------------------------------*/
/* provides ability to do successive locked bus cycles to eisa space.     
 */

struct isc_table_type *isc;
{
	register struct eisa_bus_info *eisa1 = 
		((struct eisa_bus_info *)(isc->bus_info->spec_bus_info));

        eisa1->mongoose_reg->bus_lock = 0;      /* lock bus */
}


/*------------------------------------------------------------------------*/
unlock_eisa_bus(isc)
/*------------------------------------------------------------------------*/
/* turns off locked eisa bus cycles. see lock_eisa_bus.                   
 */

struct isc_table_type *isc;
{
	register struct eisa_bus_info *eisa1 = 
		((struct eisa_bus_info *)(isc->bus_info->spec_bus_info));

        eisa1->mongoose_reg->bus_lock = 1;      /* unlock bus */
}
#endif


/*------------------------------------------------------------------------*/
eeprom_link()
/*------------------------------------------------------------------------*/
/* Set up eisa  routines.  This  routine  is called in  kernel_initialize
 * (300) and vsc_ioconf (700)
 */
{
	eisa_init_routine = eisa_init;

	eisa_eoi_routine = eisa_eoi;
	eisa_nmi_routine = eisa_nmi;
}


/*------------------------ EISA Interrupt Code ---------------------------*/

/*------------------------------------------------------------------------*/
eisa_enable_irq(isc, irq)
/*------------------------------------------------------------------------*/

struct isc_table_type *isc;
int irq;
{
	struct eisa_system_board *esb;

	esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;
	if ((irq >= 3) && (irq <= 7))
		esb->int1_int_mask_reg &= ~(1 << irq);
	else
		esb->int2_int_mask_reg &= ~(1 << (irq - 8));
}


/*------------------------------------------------------------------------*/
eisa_eoi(irq)
/*------------------------------------------------------------------------*/
/* This routine now issues specific eoi's. This was changed to protect
 * the innocent.
 */

int irq;
{
	struct eisa_system_board *esb = ((struct eisa_bus_info *)
		eisa[EISA1]->spec_bus_info)->esb;

	/* INTEL ERRATA */
	/* the nop must be  issued  in  accordance  with the ISP  errata
	 * dated  Jun  1990 to make  the  following  specific  eoi  work
	 * correctly (ie not clear other ISR bits).
	 */

	if ((irq >= 8) && (irq <= 15)) {   
		/* slave interrupted */
		/* send specific EOI to slave */
		esb->int2_ocw2 = SP_NOP | ((irq - 8) & 0x7);   
		esb->int2_ocw2 = SP_EOI | ((irq - 8) & 0x7);   

		/* if there's another interrupt pending on slave, don't send EOI to master */
		esb->int2_ocw3 = OCW3 | READ_ISR;

		/* no other slave interrupt */
		if (esb->int2_isr == 0)	{ 
			/* send non-specific EOI to master */
			esb->int1_ocw2 = SP_NOP | SLAVE;  
			esb->int1_ocw2 = SP_EOI | SLAVE;  
		}

	} else {
		/* send non-specific EOI to master */
		esb->int1_ocw2 = SP_NOP | (irq & 0x7);   
		esb->int1_ocw2 = SP_EOI | (irq & 0x7);   
	}	

#ifdef EISA_TESTS
	ch_stat(0x20 | irq);
#endif
} 


/*------------------------------------------------------------------------*/
eisa_nmi()
/*------------------------------------------------------------------------*/
{
	int i, j;
	struct eisa_system_board *esb; 
	u_char nmi_status, nmi_extended_status;
	struct isc_table_type *isc;

	if (eisa_exists) {
		for (j = 0; j < NUM_EISA_BUSES; j++) {
			esb = ((struct eisa_bus_info *)eisa[j]->spec_bus_info)->esb;
			nmi_status = esb->nmi_status;
			nmi_extended_status = esb->nmi_extended_status;
			/* read NMI status to see if EISA NMI occurred */
			if (!(nmi_status & (IOCHK_NMI | PARITY_NMI)) && 
				!(nmi_extended_status & (SW_NMI | BUS_TO_NMI | FAILSAFE_NMI)))
				continue;   /* value?  No EISA NMI occurred */

			/* to reset NMI, disable it, then re-enable it; 
			 * we'll re-enable at end 
			 */

			/* disable all NMI sources */
			esb->nmi_status = PARITY_DISABLED | IOCHK_DISABLED;
			esb->nmi_extended_status = 0; /* !SW_ENABLED & !FAILSAFE_ENABLED & !BUS_TO_ENABLED */
		
			if (nmi_status & PARITY_NMI) 
				/* 
				 * parity error should never  occur.  If
				 * it does, it  indicates a bad ISP.  We
				 * panic,  becuase  a  root/swap  device
				 * chould be out here
				 */
				panic("EISA: eisa_nmi: Unexpected NMI, Possible Bad EISA Hardware");
	
			if (nmi_status & IOCHK_NMI) {
				/* 
				 * This indicates a fatal error from one
				 * of the EISA cards.  We panic, since a
				 * root/swap  device  could  be  on  the
				 * card.  Would  sure be nice to be able
				 * to know for sure before we panic'ed.
				 */

				/* Locate offending card and print it out before panic'ing */

				for (i = 1; i<= ((struct eisa_bus_info *)
					eisa[j]->spec_bus_info)->number_of_slots; i++) {
					isc = isc_table[EISA1_MIN + i];
					if (isc != NULL) {
						if (((struct eisa_if_info *)(isc->if_info))->flags & HAS_IOCHKERR) {
							if ((int)(*(isc->if_reg_ptr + CARD_CONTROL_OFFSET)) & IOCHKERR) {
								printf("EISA card id %d in slot %d  had fatal error\n", 
								isc->if_id,
								((struct eisa_if_info *)(isc->if_info))->slot);
								panic("EISA: eisa_nmi: Fatal EISA card error");
							}
						}
					} else continue;
				}
				panic("EISA: eisa_nmi: fatal EISA card error, unknown card id");
			}
	
			if (nmi_extended_status & FAILSAFE_NMI) 
				/* 
				 * The failsafe timer is disabled on our
				 * system, so it should never occur.  If
				 * it does, it  indicates a bad ISP.  We
				 * panic,  becuase  a  root/swap  device
				 * chould be out here 
				 */

				panic("EISA: eisa_nmi: unexpected failsafe NMI, possible bad EISA hardware");
	
			if (nmi_extended_status & BUS_TO_NMI) {
#ifdef __hp9000s300
			        /* 
				 * Due to an  Intel  chip  bug, we  will
				 * leave this NMI  source  disabled  for
				 * now.  If the bug gets  fixed,  we may
				 * want to  enable  this  source of NMI.
				 * Since it's  disabled, it should never
				 * occur.  If it does,  it  indicates  a
				 * bad   ISP.  We   panic,   becuase   a
				 * root/swap device could be out here
				 */
				panic("EISA: eisa_nmi: unexpected bus timeout NMI, possible bad EISA hardware");
#else /* snakes */
                                /* 
				 * how to  determine  who  did it?  last
				 * bus master  register?  What does that
				 * tell us, slot #, ID,....?
				 */

                                panic("EISA: eisa_nmi: bus timeout NMI, suspected EISA card in slot x");
#endif
			}
	
			if (nmi_extended_status & SW_NMI) 

				/* 
				 * The software  NMI is never  generated
				 * on our  system,  so it  should  never
				 * occur.  If it does,  it  indicates  a
				 * bad   ISP.  We   panic,   becuase   a
				 * root/swap device chould be out here
				 */

				panic("EISA: eisa_nmi: unexpected software NMI, possible bad EISA hardware");
	
			/* enable NMI sources that we are interested in, currently IOCHK */
			esb->nmi_status = (!IOCHK_DISABLED);
#if defined (__hp9000s800) && defined (_WSIO)
                        esb->nmi_extended_status = BUS_TO_ENABLED;
#endif
		}
	}
}


/*---------------------- EISA DMA (and I/O) code ---------------------------*/

/*------------------------------------------------------------------------*/
eisa_get_dma(isc, channel)
/*------------------------------------------------------------------------*/

register struct isc_table_type *isc;
register int channel;
{
	int spl;
	struct dma_channel_data *channel_data;

	channel_data = &((struct eisa_bus_info *)
		(isc->bus_info->spec_bus_info))->dma_channels[channel];

	spl = spl6();
	if (channel_data->locked) {
		splx(spl);
		return(-1);
	}
	channel_data->locked = isc;
	splx(spl);
	return(0);
}


/*------------------------------------------------------------------------*/
eisa_free_dma(isc, channel)
/*------------------------------------------------------------------------*/

register struct isc_table_type *isc;
register int channel;
{
	int spl;
	register struct dma_channel_data *channel_data; 
	register struct driver_to_call *this_entry;
	register struct driver_to_call *previous;

	channel_data = &((struct eisa_bus_info *)
		(isc->bus_info->spec_bus_info))->dma_channels[channel];

	spl = spl6();
	if (channel_data->locked == NULL) {
		splx(spl);
		return(-1);	/* no one owns channel, error, or OK? */
	}

	if (channel_data->locked != isc) {
		previous = NULL;
		this_entry = channel_data->driver_wait_list;
		while (this_entry != NULL) {
			if (this_entry->isc == isc)
				break;
			previous = this_entry;
			this_entry = this_entry->next;
		}
		if (this_entry != NULL) {
			if (previous == NULL)
				channel_data->driver_wait_list = this_entry->next;
			else 
				previous->next = this_entry->next;
			FREE(this_entry, M_IOSYS);
		}
		splx(spl);
		return(0);
	}
			
	/* else channel is locked by this guy */
	channel_data->locked = 0;
	if (channel_data->driver_wait_list != NULL) {
		this_entry = channel_data->driver_wait_list;
		(*this_entry->drv_routine)(this_entry->drv_arg);
		channel_data->driver_wait_list = this_entry->next;
		FREE(this_entry, M_IOSYS);
	}
	splx(spl);
	return(0);
}
	

/*------------------------------------------------------------------------*/
eisa_dma_build_chain(isc, dma_parms)
/*------------------------------------------------------------------------*/

struct isc_table_type *isc;
struct dma_parms *dma_parms;
{
	caddr_t addr;
	int count;
	int num_pages, this_iomap, bytes_this_page, x;
	unsigned phys_addr;
	struct driver_to_call *iomap_list;
	int *iomap_start;
        int chain_count;
	int err;
        struct addr_chain_type *chain_ptr;
	int mid_buf = 0; 
#if defined (__hp9000s800) && defined (_WSIO)
	int beg_buflet=0, end_buflet=0, beg_pagelet = 0, end_pagelet = 0; 
#endif

	/* get iomap host address */
	iomap_start = isc->bus_info->host_iomap_base;

	num_pages = (dma_parms->count/NBPG) + 2;

	chain_count = 0;

#if defined (__hp9000s800) && defined (_WSIO)
	/* 
	 * Only need buflets if we are doing a read into memory.  Buflets deal with
      	 * the scenerio  where a cacheline may be shared  between two processes via
	 * shared  memory, or where the  cacheline  is shared  between  two  kernel
	 * buffers.  The  problem  is that an access to that  cacheline  after  the
	 * flush/purge  prior to dma will bring the  cacheline  back into cache and
	 * result in data corruption (dma went to memory, but cacheline in cache is
	 * valid.)
	 *
	 * The current physio() implementation will remove the user mapping of user
	 * buffers and map them into kernel space.  Thus user buffers processed via
	 * physio() need niether  buflets or pagelet  protection.  The only problem
	 * will be an unhappily aligned kernel buffer (no remap is done).
	 */

	if ((dma_parms->dma_options & DMA_READ) && !(dma_parms->flags & USER_PHYS)) {
		/*
		 * BUS MASTERS can be protected by using buflets only. Slaves need 
		 * pagelets.
		 */
		if (dma_parms->channel == BUS_MASTER_DMA) {
			/* CACHE size byte alignment check */
			beg_buflet = CPU_IOLINE - (unsigned int)dma_parms->addr & (CPU_IOLINE-1);
			if (beg_buflet == CPU_IOLINE) beg_buflet = 0;

			if (beg_buflet >= dma_parms->count) {
				/* 
				 * if the  first  buflet  can hold  more  bytes  than the
				 * count, we do not need a middle or an end element.
				 */
				beg_buflet = dma_parms->count;
			} else {
				/* we now need to decide if we need an end buflet */
				end_buflet = ((unsigned int)dma_parms->addr + dma_parms->count) & 
					(CPU_IOLINE-1);  

				if ((beg_buflet + end_buflet) != dma_parms->count) {
					/* only now do we need a middle element. */
					/* this is however, the norm             */
					mid_buf = dma_parms->count - beg_buflet - end_buflet;
				}
			}

			/* bump request for iomap entries for buflets */
			if (beg_buflet) {
				num_pages++; 
				chain_count++;
			}

			if (end_buflet) {
				num_pages++; 
				chain_count++;
			}

			if (mid_buf) {
				chain_count++;
			}

		} else { /* slave DMA, lets look for pagelets */
			/* 
			 * CACHE size byte alignment check. if the start address is not cache 
			 * line aligned, then we have a beginning pagelet.
			 */
			if ((unsigned int)dma_parms->addr & (CPU_IOLINE-1))
				/*
				 * specifically we take the offset into the page and subtract
				 * that from how many bytes on a page, and this tells us how
				 * large of a transfer we can handle.
				 */
				beg_pagelet = NBPG - ((unsigned int)dma_parms->addr & (NBPG-1));

			/* 
			 * if all of the transfer will fit on the rest of the page, we only
			 * need a beginning pagelet and nothing else.
			 */
			if (beg_pagelet >= dma_parms->count) {
				/* 
				 * if the first pagelet can hold more bytes than the count, we 
				 * do not need a middle or an end element.
				 */
				beg_pagelet = dma_parms->count;
			} else {

				/*
				 * we have determined  that either all of the bytes will not fit
				 * on a beginning  pagelet, or the start  address was cache line
				 * aligned.  We  must  now  determine  if  the  ending  address,
				 * calculated by adding count to start is cache line aligned.
				 */

				if (((unsigned int)dma_parms->addr+dma_parms->count)&(CPU_IOLINE-1))
					/* 
					 * the number of bytes which can
					 * and must be fit onto  the end
					 * pagelet is now calculated.
					 */
					end_pagelet = ((unsigned int)dma_parms->addr + 
						dma_parms->count) & (NBPG-1);
				/* 
				 * if the  number  of bytes  the end  pagelet  will  contain  is
				 * greater than the requested transfer, then there was not a beg
				 * pagelet and there is no need for a middle buffer.
				 */

				if (end_pagelet > dma_parms->count) {
					end_pagelet = dma_parms->count;
				} else {
					/* 
					 * we  will  need a  middle  buffer  only  if the  bytes
					 * contained  on the beg and end  pagelets are less than
					 * the bytes requested to transfer.
					 */

					if ((beg_pagelet + end_pagelet) < dma_parms->count) {
						/* only now do we need a middle element. */
						mid_buf=dma_parms->count-beg_pagelet-end_pagelet;
					}
				}
			}
			chain_count = 1;
		}
		
	} else { 
#endif
		/* 
		 * this code handles the much  simplified  case of not having to
		 * deal with  either the  buflet or pagelet  scenerio.  There is
		 * only a middle buffer and it contains the full count.
		 */
		mid_buf = dma_parms->count;
		chain_count = 1; /* got to have at least one element for main xfer */

#if defined (__hp9000s800) && defined (_WSIO)
	}
#endif

	dma_parms->chain_count = chain_count;

	/* 
	 * attempt to allocate iomap entries 
	 */
	x = spl6();
	if (!(dma_parms->dma_options & DMA_SKIP_IOMAP) && lock_iomap_entries(isc, num_pages, 
	    dma_parms)) {
		if (dma_parms->drv_routine) {
			if ((err = put_on_wait_list(isc, dma_parms->drv_routine, 
			    dma_parms->drv_arg)) < 0) {
				splx(x);
				return(err);
			}
		}
		splx(x);
		return(RESOURCE_UNAVAILABLE);
	}
	splx(x);


        /* 
	 * allocate space for the DMA chain (this is the default action) 
	 */
        if (!(dma_parms->flags & NO_ALLOC_CHAIN)) {
                MALLOC(dma_parms->chain_ptr, struct addr_chain_type *,
                    chain_count * sizeof(struct addr_chain_type), M_DMA, M_NOWAIT);
                if (dma_parms->chain_ptr == NULL) {
                        free_iomap_entries(isc, dma_parms);
                        return(BUFLET_ALLOC_FAILED);
                }

        }

#if defined (__hp9000s800) && defined (_WSIO)
	/* 
	 * lets do some major memory allocation and/or cleanup 
	 */
	if (beg_buflet || beg_pagelet || end_buflet || end_pagelet) {
		/* need to allocate a buflet_info structure */
               	MALLOC(dma_parms->buflet_key, struct buflet_info *,
                    sizeof(struct buflet_info), M_DMA, M_NOWAIT);
               	if (dma_parms->buflet_key == NULL) {
			/* cleanup prior allocations */
			FREE(dma_parms->chain_ptr, M_DMA);
                        free_iomap_entries(isc, dma_parms);
                        return(BUFLET_ALLOC_FAILED);
		}

		if (beg_buflet) {
			MALLOC(dma_parms->buflet_key->buflet_beg, caddr_t, CPU_IOLINE, 
		 	    M_DMA, (M_ALIGN | M_NOWAIT));

			if (dma_parms->buflet_key->buflet_beg == NULL) {
				/* cleanup prior allocations */
				FREE(dma_parms->buflet_key, M_DMA);
				FREE(dma_parms->chain_ptr, M_DMA);
                        	free_iomap_entries(isc, dma_parms);
				return(BUFLET_ALLOC_FAILED);
			}
#ifdef EISA_TESTS
			/* this is here to help verify the buflet testing */
			bzero(dma_parms->buflet_key->buflet_beg, CPU_IOLINE); 
#endif
		} else if (beg_pagelet) {
			MALLOC(dma_parms->buflet_key->buflet_beg, caddr_t, NBPG, M_IOSYS, M_NOWAIT);

			if (dma_parms->buflet_key->buflet_beg == NULL) {
				/* cleanup prior allocations */
				FREE(dma_parms->buflet_key, M_DMA);
				FREE(dma_parms->chain_ptr, M_DMA);
                        	free_iomap_entries(isc, dma_parms);
				return(BUFLET_ALLOC_FAILED);
			}
#ifdef EISA_TESTS
			/* this is here to help verify the buflet testing */
			bzero(dma_parms->buflet_key->buflet_beg, NBPG); 
#endif
		} else dma_parms->buflet_key->buflet_beg = NULL;
			

		if (end_buflet) {
			MALLOC(dma_parms->buflet_key->buflet_end, caddr_t, CPU_IOLINE, 
			    M_DMA, (M_ALIGN | M_NOWAIT));

			if (dma_parms->buflet_key->buflet_end == NULL) {
				/* cleanup prior allocations */
				if (beg_buflet) 
					FREE(dma_parms->buflet_key->buflet_beg, M_DMA);
				FREE(dma_parms->buflet_key, M_DMA);
				FREE(dma_parms->chain_ptr, M_DMA);
                        	free_iomap_entries(isc, dma_parms);
				return(BUFLET_ALLOC_FAILED);
			}		
#ifdef EISA_TESTS
			/* this is here to help verify the buflet testing */
			bzero(dma_parms->buflet_key->buflet_end, CPU_IOLINE); 
#endif
		} else if (end_pagelet) {
			MALLOC(dma_parms->buflet_key->buflet_end, caddr_t, NBPG, M_IOSYS, M_NOWAIT);

			if (dma_parms->buflet_key->buflet_end == NULL) {
				/* cleanup prior allocations */
				if (beg_buflet) 
					FREE(dma_parms->buflet_key->buflet_beg, M_IOSYS);
				FREE(dma_parms->buflet_key, M_DMA);
				FREE(dma_parms->chain_ptr, M_DMA);
                        	free_iomap_entries(isc, dma_parms);
				return(BUFLET_ALLOC_FAILED);
			}		
#ifdef EISA_TESTS
			/* this is here to help verify the buflet testing */
			bzero(dma_parms->buflet_key->buflet_end, NBPG); 
#endif
		} else dma_parms->buflet_key->buflet_end = NULL; 

		dma_parms->buflet_key->space = dma_parms->spaddr;
	} else {
		/* since this is a flag for copies later, we need it cleared */
		dma_parms->buflet_key = NULL;
	}
#endif

	chain_ptr  = dma_parms->chain_ptr;
	this_iomap = dma_parms->key;
	addr  = dma_parms->addr;

#if defined (__hp9000s800) && defined (_WSIO)
	/* 
	 * the following code will deal with pagelets (on reads only)
	 * setting up iomap entries and dma_parms buflet structures.
	 */
	if (beg_pagelet || end_pagelet) {
		dma_parms->flags |= PAGELETS;
		chain_ptr->count = dma_parms->count;

		/* store starting DMA element data into DMA chain */
		chain_ptr->phys_addr = ((caddr_t)(
			/* calculate page number */
			(unsigned int)isc->bus_info->bus_iomap_base + ((this_iomap) * 0x1000) 
			/* calculate offset into page */
			+ ((int)(addr) & 0xfff)));

		/* if we have a beginning pagelet */
		if (beg_pagelet) {
#ifdef EISA_TESTS
			ch_stat(0xd5);
#endif
			/* set up buflet_info structure */
			dma_parms->buflet_key->buffer_beg = addr;
			dma_parms->buflet_key->cnt_beg = beg_pagelet;

#ifdef EISA_TESTS
			fdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_beg,
				NBPG, c_always);
#else
			pdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_beg,
				NBPG, c_always);
#endif
			SYNC();
			
			/* note, the original page offset and this differ, but is lost when used */
			phys_addr = (unsigned int)ltor(KERNELSPACE, dma_parms->buflet_key->buflet_beg);

			/* load the iomap entry */
			(*(iomap_start + ((this_iomap*0x1000)/4))) = phys_addr>>PGSHIFT;

			addr += beg_pagelet;  /* should now be a page boundary */
			this_iomap++;  /* next iomap entry */
		}

		/* if we have a middle buffer */
		if (mid_buf) {
#ifdef EISA_TESTS
			ch_stat(0xd6);
#endif
			count = mid_buf;

			while (count) {
				phys_addr = (unsigned int)ltor(dma_parms->spaddr, addr);

				bytes_this_page = NBPG - ((unsigned int)addr & PGOFSET);
				if (bytes_this_page > count)
					bytes_this_page = count;

				addr  += bytes_this_page;
				count -= bytes_this_page;
			
				(*(iomap_start + ((this_iomap*0x1000/4)))) = phys_addr>>PGSHIFT;
				this_iomap++; /* next iomap entry */
			}
		}

		/* if we have an end pagelet */
		if (end_pagelet) {
#ifdef EISA_TESTS
			ch_stat(0xd7);
#endif

			/* set up buflet_info structure */
			dma_parms->buflet_key->buffer_end = addr;
			dma_parms->buflet_key->cnt_end = end_pagelet;
			
#ifdef EISA_TESTS
			fdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_end,
				NBPG, c_always);
#else
			pdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_end,
				NBPG, c_always);
#endif
			SYNC();
			
			/* note, the original page offset and this differ, but is lost when used */
			phys_addr=(unsigned int)ltor(KERNELSPACE,dma_parms->buflet_key->buflet_end);

			(*(iomap_start + ((this_iomap*0x1000)/4))) = phys_addr>>PGSHIFT;

		}

		return (0);
	} /* end of pagelet handling code */


	/* 
	 * BUFLET  HANDLING CODE - the following code will deal with all
	 * writes,  and buflets on reads  setting up dma  chains,  iomap
	 * entries, and dma_parms buflet structures
	 */

	if (beg_buflet) {
#ifdef EISA_TESTS
		ch_stat(0xd1);
#endif
        	chain_ptr->count = beg_buflet;    /* size of first */

		/* set up buflet_info structure */
		dma_parms->buflet_key->buffer_beg = addr;
		dma_parms->buflet_key->cnt_beg = beg_buflet;

#ifdef EISA_TESTS
		fdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_beg,
			CPU_IOLINE, c_always);
#else
		pdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_beg,
			CPU_IOLINE, c_always);
#endif
		SYNC();
		
		/* not concerned with original offset here since not used */
		phys_addr = (unsigned int)ltor(KERNELSPACE, dma_parms->buflet_key->buflet_beg);

		/* load the iomap entry */
        	(*(iomap_start + ((this_iomap*0x1000)/4))) = phys_addr>>PGSHIFT;

        	/* 
		 * need to set up the physical address for a dma controller.  we want to
		 * preserve the actual  offset into a cache line since some  controllers
		 * make multi-byte decisions based on lower address bits.
		 *
		 * First take the  iomap_base  address  and add the iomap  entry  number
		 * times 4K.  This gets you to the correct iomap entry.  The page number
		 * has  already  been stored in the iomap, but we now we need to tack on
		 * the correct  offset.  This is the buflets  page  offset (a cache line
		 * boundary) + the original buffers cache line offset.  Isn't this fun!!
		 */

        	chain_ptr->phys_addr = ((caddr_t)(
			/* calculate the page number */
			(unsigned int)isc->bus_info->bus_iomap_base + (this_iomap * 0x1000) 
			/* calculate the offset into the page */
			+ (((unsigned int)(dma_parms->buflet_key->buflet_beg) & 0xfff) +
			((unsigned)addr & (CPU_IOLINE-1)))));


        	addr += chain_ptr->count;  /* should now be a 32 byte boundary */

        	this_iomap++;  /* next iomap entry */
        	chain_ptr++;   /* next element in chain */
	}


	if (mid_buf) {
#ifdef EISA_TESTS
		ch_stat(0xd2);
#endif
#endif
		/* 
		 * set up the dma  address  for the chunk of  contiguous  iomap  entries
		 * comprising  the middle  buffer.  This code is all that is required to
		 * handle  all  non-buflet/pagelet  cases,  and will  handle the  middle
		 * buffer case for buflets.
		 */

       		chain_ptr->phys_addr = ((caddr_t)(
	  	    /* calculate page number */
		    (unsigned int)isc->bus_info->bus_iomap_base + (this_iomap * 0x1000) 
		    /* calculate offset into the page */
		    + ((unsigned int)(addr) & PGOFSET)));

		count = chain_ptr->count = mid_buf;

		/* loop pulling out succesive max 4K chunks, and setting up the iomap */
		while (count) {
#ifdef __hp9000s300
              		phys_addr = (unsigned int)vtop(addr);
#else
              		phys_addr = (unsigned int)ltor(dma_parms->spaddr, addr);
#endif

			bytes_this_page = NBPG - ((unsigned int)addr & PGOFSET);
			if (bytes_this_page > count)
				bytes_this_page = count;

			addr  += bytes_this_page;
			count -= bytes_this_page;
		
        		(*(iomap_start + ((this_iomap*0x1000)/4))) = phys_addr>>PGSHIFT;
			this_iomap++; /* next iomap entry */
		}
#if defined (__hp9000s800) && defined (_WSIO)
        	chain_ptr++;  /* next chain element */
	}


	if (end_buflet) {
#ifdef EISA_TESTS
		ch_stat(0xd3);
#endif

		/* size of last chain element */
        	chain_ptr->count = end_buflet;

		/* set up the buflet_info structure */
		dma_parms->buflet_key->buffer_end = addr;
		dma_parms->buflet_key->cnt_end = end_buflet;
		
#ifdef EISA_TESTS
		fdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_end,
			CPU_IOLINE, c_always);
#else
		pdcache_conditionally(KERNELSPACE, dma_parms->buflet_key->buflet_end,
			CPU_IOLINE, c_always);
#endif
		SYNC();
		
		phys_addr = (unsigned int)ltor(KERNELSPACE, dma_parms->buflet_key->buflet_end);

		(*(iomap_start + ((this_iomap*0x1000)/4))) = phys_addr>>PGSHIFT;

        	/* 
		 * need to set up the physical address for a dma controller.  we want to
		 * preserve the actual  offset into a cache line since some  controllers
		 * make multi-byte decisions based on lower address bits.
		 *
		 * First take the  iomap_base  address  and add the iomap  entry  number
		 * times 4K.  This gets you to the correct iomap entry.  The page number
		 * has  already  been stored in the iomap, but we now we need to tack on
		 * the correct  offset.  This is the buflets  page  offset (a cache line
		 * boundary).  Note,  we do not need to  include  the  original  buffers
		 * cache line offset since it is "always" 0.  No, this isn't this fun!!
		 */

        	chain_ptr->phys_addr = ((caddr_t)(
			/* calculate page number */
			(unsigned int)isc->bus_info->bus_iomap_base + (this_iomap * 0x1000) 
			/* calculate offset into the page */
			+ (((unsigned int)(dma_parms->buflet_key->buflet_end) & 0xfff))));

		}
#endif

	return(0);
}


/*------------------------------------------------------------------------*/
eisa_load_addr_count(isc, dma_parms)
/*------------------------------------------------------------------------*/
/* Load an address and count pair into DMA channel from DMA chain 
 */

register struct isc_table_type *isc;
register struct dma_parms *dma_parms;
{
        register struct eisa_system_board *esb;
        register struct dma_channel_data *channel_data;
        register int channel = dma_parms->channel, x;
        unsigned dma_addr_reg_value;
        register int count, controller;

        /* Get next address and count from DMA chain */
        channel_data = &(((struct eisa_bus_info *)
		(isc->bus_info->spec_bus_info))->dma_channels[channel]);
        dma_addr_reg_value = (int)(channel_data->next_cp->phys_addr);
        count = (int)(channel_data->next_cp->count);

        /* Advance chain pointer and decrement pend_chain_count */
        channel_data->next_cp++;

        /* Setup some other variables */
        esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;
        if ((channel >= 0) && (channel <= 3)) 
                controller = 0;
        else controller = 1;

        /* Process 16-word mode DMA requests */
        if (dma_parms->transfer_size == DMA_16WORD) {
                count = count/2;
                dma_addr_reg_value = (dma_addr_reg_value & 0xfffe0000) | 
                                        ((dma_addr_reg_value >> 1) & 0xffff);
        }

        /* Program count and address for DMA channels 5-7 */
        if (controller) {
		x = spl6();
                esb->dma1_clr_byte_ptr = 1;
		count--; /* transfers one more than count */
                if (channel == 5) {
                        esb->dma1_ch5_count = (count & 0xff);
                        esb->dma1_ch5_count = (count & 0xff00) >> 8;
                        esb->dma1_ch5_high_count = (count & 0xff0000) >> 16;
                } else if (channel == 6) {
                        esb->dma1_ch6_count = (count & 0xff);
                        esb->dma1_ch6_count = (count & 0xff00) >> 8;
                        esb->dma1_ch6_high_count = (count & 0xff0000) >> 16;
                } else if (channel == 7) {
                        esb->dma1_ch7_count = (count & 0xff);
                        esb->dma1_ch7_count = (count & 0xff00) >> 8;
                        esb->dma1_ch7_high_count = (count & 0xff0000) >> 16;
                }
                esb->dma1_clr_byte_ptr = 1;
                if (channel == 5) {
                        esb->dma1_ch5_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma1_ch5_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma1_ch5_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma1_ch5_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                } else  if (channel == 6) {
                        esb->dma1_ch6_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma1_ch6_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma1_ch6_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma1_ch6_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                } else  if (channel == 7) {
                        esb->dma1_ch7_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma1_ch7_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma1_ch7_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma1_ch7_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                }
		splx(x);

        /* Program count and address for DMA channels 0-3 */
        } else {
		x = spl6();
                esb->dma0_clr_byte_ptr = 1;
		count--; /* transfers one more than count */
                if (channel == 0) {
                        esb->dma0_ch0_count = (count & 0xff);
                        esb->dma0_ch0_count = (count & 0xff00) >> 8;
                        esb->dma0_ch0_high_count = (count & 0xff0000) >> 16;
                } else if (channel == 1) {
                        esb->dma0_ch1_count = (count & 0xff);
                        esb->dma0_ch1_count = (count & 0xff00) >> 8;
                        esb->dma0_ch1_high_count = (count & 0xff0000) >> 16;
                } else if (channel == 2) {
                        esb->dma0_ch2_count = (count & 0xff);
                        esb->dma0_ch2_count = (count & 0xff00) >> 8;
                        esb->dma0_ch2_high_count = (count & 0xff0000) >> 16;
                } else if (channel == 3) {
                        esb->dma0_ch3_count = (count & 0xff);
                        esb->dma0_ch3_count = (count & 0xff00) >> 8;
                        esb->dma0_ch3_high_count = (count & 0xff0000) >> 16;
                }
                esb->dma0_clr_byte_ptr = 1;
                if (channel == 0) {
                        esb->dma0_ch0_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma0_ch0_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma0_ch0_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma0_ch0_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                } else  if (channel == 1) {
                        esb->dma0_ch1_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma0_ch1_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma0_ch1_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma0_ch1_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                } else  if (channel == 2) {
                        esb->dma0_ch2_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma0_ch2_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma0_ch2_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma0_ch2_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                } else  if (channel == 3) {
                        esb->dma0_ch3_base_addr = dma_addr_reg_value & 0xff;
                        esb->dma0_ch3_base_addr = (dma_addr_reg_value & 0xff00) >> 8;
                        esb->dma0_ch3_low_page = (dma_addr_reg_value & 0xff0000) >> 16;
                        esb->dma0_ch3_high_page = (dma_addr_reg_value & 0xff000000) >> 24;
                }
		splx(x);
        }
} /* End eisa_load_addr_count() */


/*------------------------------------------------------------------------*/
eisa_dma_setup(isc, dma_parms)
/*------------------------------------------------------------------------*/

register struct isc_table_type *isc;
register struct dma_parms *dma_parms;
{
	register caddr_t addr = dma_parms->addr;
	register int count = dma_parms->count;
	register int channel = dma_parms->channel;
	int bus_master = channel == BUS_MASTER_DMA ? 1 : 0;
	int options = dma_parms->dma_options;
	register struct eisa_system_board *esb;
	register struct dma_channel_data *channel_data;
	struct driver_to_call *this_entry;
	int return_value, x;
	register controller;
	unsigned dma_addr_reg_value;
	int isa_master = 0;
        struct addr_chain_type *chain_ptr;
        int chain_count;

	/* 
	 * if this is a 16bit ISA master  specifying  a DMA channel  for
	 * cascading,  store the  channel  he wants in  channel  and set
	 * isa_master flag
	 */
	if ((bus_master) && (options & DMA_CASCADE)) {
		channel = options & ISA_CHANNEL_MASK;
		isa_master = 1;
	}

	/* see if we already have iomap entries */
	if (!(dma_parms->dma_options & DMA_SKIP_IOMAP))
		dma_parms->key = -1; 

	/* perform error checking, if requested */
	if (!(dma_parms->flags & NO_CHECK)) {	
		if ((count > EISA_MAXPHYS) || (count <=0))
			return(TRANSFER_SIZE);
		if ((!bus_master) || (isa_master)) {
			if (channel > 7 || channel < 0 || channel == 4)
				return(ILLEGAL_CHANNEL);
		}
		if (!(options & (DMA_8BYTE | DMA_16BYTE | DMA_16WORD | DMA_32BYTE)))
			return(INVALID_OPTIONS_FLAGS);
		if (((options & (DMA_8BYTE | DMA_16BYTE | DMA_16WORD | DMA_32BYTE)) == DMA_16WORD)
			&& (((int)addr%2) || (count%2))) 
				return(BUF_ALIGN_ERROR);

		if (dma_parms->flags & NO_ALLOC_CHAIN)
			if (dma_parms->chain_ptr == NULL)
				return(NULL_CHAINPTR);

		if (!(dma_parms->flags & EISA_BYTE_ENABLE)) { /* card does not support BE's */
			if (((int)addr%2) || (count%2)) { 
				/* odd transfer, can only do 8bit */
				if (!(options & DMA_8BYTE))
					return(BUF_ALIGN_ERROR);
				else dma_parms->transfer_size = DMA_8BYTE;
			} else if (((int)addr%4) || (count%4)) { 
				/* not long aligned, but even, can't do 32bit */
				if (options & DMA_16WORD)
					dma_parms->transfer_size = DMA_16WORD;
				else if (options & DMA_16BYTE)
					dma_parms->transfer_size = DMA_16BYTE;
				else if (options & DMA_8BYTE)
					dma_parms->transfer_size = DMA_8BYTE;
				else return(BUF_ALIGN_ERROR);
			} else {  /* long aligned, do longest they specified */
				if (options & DMA_32BYTE) 
					dma_parms->transfer_size = DMA_32BYTE;
				else if (options & DMA_16WORD)
					dma_parms->transfer_size = DMA_16WORD;
				else if (options & DMA_16BYTE)
					dma_parms->transfer_size = DMA_16BYTE;
				else if (options & DMA_8BYTE)
					dma_parms->transfer_size = DMA_8BYTE;
			}

		} else { /* card supports byte enables */
#ifdef __hp9000s300
			/* if would require 3byte transfer */
			if ((((int)addr&3) == 1) || ((((int)addr + count)&3) == 3)) {
				if (options & DMA_16BYTE)
					dma_parms->transfer_size = DMA_16BYTE;
				else if (options & DMA_8BYTE)
					dma_parms->transfer_size = DMA_8BYTE;
				else return(BUF_ALIGN_ERROR);	/* 16word or 32byte */
			} else { /* can do largest specified, 16word/odd caught above */
#endif
				if (options & DMA_32BYTE) 
					dma_parms->transfer_size = DMA_32BYTE;
				else if (options & DMA_16WORD)
					dma_parms->transfer_size = DMA_16WORD;
				else if (options & DMA_16BYTE)
					dma_parms->transfer_size = DMA_16BYTE;
				else dma_parms->transfer_size = DMA_8BYTE;
#ifdef __hp9000s300
			}
#endif
		}

		/* burst and cascade are an illegal combination */
		if ((!(isa_master)) && (options & DMA_CASCADE))
			return(INVALID_OPTIONS_FLAGS);
		/* burst and single are an illegal combination */
		if ((options & DMA_BURST) && (options & DMA_SINGLE))
			return(INVALID_OPTIONS_FLAGS);
		/* 
		 * isa timing and demand mode are a discouraged  combination, no
		 * error is returned for them .  isa timing and block mode are a
		 * discouraged combination, no error is returned for them
		 */
	} else {	
		/* if we're not error checking, just init  transfer_size
		 * to the largest they've specified
		 */
		if (options & DMA_32BYTE) 
			dma_parms->transfer_size = DMA_32BYTE;
		else if (options & DMA_16WORD)
			dma_parms->transfer_size = DMA_16WORD;
		else if (options & DMA_16BYTE)
			dma_parms->transfer_size = DMA_16BYTE;
		else dma_parms->transfer_size = DMA_8BYTE;
	}

	/* allocate the dma channel, if using EISA's or if a 16bit ISA master needs one */
	if ((!bus_master) || (isa_master)) {
		channel_data = &(((struct eisa_bus_info *)
			(isc->bus_info->spec_bus_info))->dma_channels[channel]);

		x = spl6();
		if (eisa_get_dma(isc, channel)) {
			if (dma_parms->drv_routine) {
				if (channel_data->driver_wait_list == NULL) {
					MALLOC(channel_data->driver_wait_list, 
					   struct driver_to_call *, sizeof(struct driver_to_call),
					   M_IOSYS, M_NOWAIT);

					if (channel_data->driver_wait_list == NULL) {
						splx(x);
						return(MEMORY_ALLOC_FAILED);
					}

					channel_data->driver_wait_list->drv_routine = dma_parms->drv_routine;
					channel_data->driver_wait_list->drv_arg = dma_parms->drv_arg;
					channel_data->driver_wait_list->isc = isc;
					channel_data->driver_wait_list->next = NULL;
				} else {
					this_entry = channel_data->driver_wait_list;
					while (this_entry->next != NULL)
						this_entry = this_entry->next;

					MALLOC(this_entry->next, struct driver_to_call *,
						sizeof(struct driver_to_call), M_IOSYS, M_NOWAIT);
					this_entry = this_entry->next;

					if (this_entry == NULL) {
						splx(x);
						return(MEMORY_ALLOC_FAILED);
					}
					this_entry->drv_routine = dma_parms->drv_routine;
					this_entry->drv_arg = dma_parms->drv_arg;
					this_entry->isc = isc;
					this_entry->next = NULL;
				}
			}
			splx(x);
			return(RESOURCE_UNAVAILABLE);
		}
		splx(x);
	}

	/* Build the transfer chain and setup the iomap */
#ifdef __hp9000s300
	if ((((int)(vtop(addr)) & 0xff000000) == ((int)((struct eisa_bus_info *)
		(isc->bus_info->spec_bus_info))->eisa_base_addr))) {
#else
	if ((((int)(ltor(dma_parms->spaddr, addr)) & 0xff000000) == 
		((int)((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->eisa_base_addr))) {
#endif
		/* 
		 * eisa-to-eisa transfer does not need iomap entries 
		 */
		if (!(dma_parms->flags & NO_ALLOC_CHAIN)) {
			MALLOC(dma_parms->chain_ptr, struct addr_chain_type *,
				sizeof(struct addr_chain_type)*(count/NBPG + 3), M_DMA, M_NOWAIT);
			if (dma_parms->chain_ptr == NULL) {
				if ((!bus_master) || (isa_master)) 
					eisa_free_dma(isc, channel);
				return(MEMORY_ALLOC_FAILED);
			}
		}	
                dma_parms->chain_count = io_dma_build_chain(dma_parms->spaddr, addr, count, dma_parms->chain_ptr);
		
	} else {
		/* 
		 * either eisa-to-host or host-to-eisa transfer 
		 */
		if (return_value = eisa_dma_build_chain(isc, dma_parms)) {
			if ((!bus_master) || (isa_master)) 
				eisa_free_dma(isc, channel);
			return(return_value);
		}
	}

	/* initialize the DMA channel, if EISA's */
	if (!bus_master) {
		esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;
		if ((channel >= 0) && (channel <= 3))
			controller = 0;
		else 
			controller = 1;
		
		switch (options & (DMA_DEMAND | DMA_BLOCK | DMA_SINGLE | DMA_CASCADE)) {
			case DMA_DEMAND:
				channel_data->dma_mode_sw = DEMAND_MODE; break;
			case DMA_BLOCK:
				channel_data->dma_mode_sw = BLOCK_MODE; break;
			case DMA_SINGLE:
				channel_data->dma_mode_sw = SINGLE_MODE; break;
			case DMA_CASCADE:
				channel_data->dma_mode_sw = CASCADE_MODE; break;
			default:
				if (!(dma_parms->flags & NO_CHECK)) {
					eisa_free_dma(isc, channel);
					free_iomap_entries(isc, dma_parms);

#if defined(__hp9000s800) && defined(_WSIO)
					if (dma_parms->buflet_key->buflet_beg)
						FREE(dma_parms->buflet_key->buflet_beg, M_DMA);
					if (dma_parms->buflet_key->buflet_end)
						FREE(dma_parms->buflet_key->buflet_end, M_DMA);
					if (dma_parms->buflet_key)
						FREE(dma_parms->buflet_key, M_DMA);
#endif
					if (!(dma_parms->flags & NO_ALLOC_CHAIN))
						FREE(dma_parms->chain_ptr, M_DMA);
					return(INVALID_OPTIONS_FLAGS);
				}
		}
				
		switch (options & (DMA_READ | DMA_WRITE)) { 
			case DMA_READ:
				channel_data->dma_mode_sw |= READ_TRANSFER; break;
			case DMA_WRITE:
				channel_data->dma_mode_sw |= WRITE_TRANSFER; break;
			default:
				if (!(dma_parms->flags & NO_CHECK)) {
					eisa_free_dma(isc, channel);
					free_iomap_entries(isc, dma_parms);

#if defined(__hp9000s800) && defined(_WSIO)
					if (dma_parms->buflet_key->buflet_beg)
					FREE(dma_parms->buflet_key->buflet_beg, M_DMA);
					if (dma_parms->buflet_key->buflet_end)
						FREE(dma_parms->buflet_key->buflet_end, M_DMA);
					if (dma_parms->buflet_key)
						FREE(dma_parms->buflet_key, M_DMA);
#endif
					if (!(dma_parms->flags & NO_ALLOC_CHAIN))
						FREE(dma_parms->chain_ptr, M_DMA);
					return(INVALID_OPTIONS_FLAGS);
				}
		}

		if (controller)
			esb->dma1_mode = channel_data->dma_mode_sw | (channel - 4);
		else 
			esb->dma0_mode = channel_data->dma_mode_sw | channel;

		channel_data->dma_extended_mode_sw = STOP_DISABLED;
		
		switch (options & (DMA_ISA | DMA_TYPEA | DMA_TYPEB | DMA_BURST)) { 
			case DMA_ISA:
				channel_data->dma_extended_mode_sw |= ISA_TIMING; break;
			case DMA_TYPEA:
				channel_data->dma_extended_mode_sw |= TYPEA_TIMING; break;
			case DMA_TYPEB:
				channel_data->dma_extended_mode_sw |= TYPEB_TIMING; break;
			case DMA_BURST:
				channel_data->dma_extended_mode_sw |= BURST; break;
			default:
				if (!(dma_parms->flags & NO_CHECK)) {
					eisa_free_dma(isc, channel);
					free_iomap_entries(isc, dma_parms);

#if defined(__hp9000s800) && defined(_WSIO)
					if (dma_parms->buflet_key->buflet_beg)
						FREE(dma_parms->buflet_key->buflet_beg, M_DMA);
					if (dma_parms->buflet_key->buflet_end)
						FREE(dma_parms->buflet_key->buflet_end, M_DMA);
					if (dma_parms->buflet_key)
						FREE(dma_parms->buflet_key, M_DMA);
#endif
					if (!(dma_parms->flags & NO_ALLOC_CHAIN))
						FREE(dma_parms->chain_ptr, M_DMA);
					return(INVALID_OPTIONS_FLAGS);
				}
		}

		if (dma_parms->transfer_size == DMA_32BYTE) 
			channel_data->dma_extended_mode_sw |= BYTE_32BIT;
		else if (dma_parms->transfer_size == DMA_16WORD)
			channel_data->dma_extended_mode_sw |= WORD_16BIT;
		else if (dma_parms->transfer_size == DMA_16BYTE)
			channel_data->dma_extended_mode_sw |= BYTE_16BIT;
		else channel_data->dma_extended_mode_sw |= BYTE_8BIT;

                /* point to start of DMA chain and get initial count */
                chain_ptr = dma_parms->chain_ptr;
                chain_count = dma_parms->chain_count;

                /* initialize dma_channel_data with DMA chain state info */
                channel_data->dma_parms = dma_parms;
                channel_data->next_cp = chain_ptr;

		/* Program TC as an output */
		channel_data->dma_extended_mode_sw &= ~TC_INPUT;
		if (controller)
			esb->dma1_extended_mode = channel_data->dma_extended_mode_sw | 
			    ((channel) - 4);
		else
			esb->dma0_extended_mode = channel_data->dma_extended_mode_sw | 
			    (channel);

		/* Load address and count pair */
		eisa_load_addr_count(isc, dma_parms);

#ifdef EISA_TESTS
	ch_stat(0xe1); /* indicate next byte is ext mode byte */
	ch_stat((unsigned char)(channel_data->dma_extended_mode_sw | channel));
#endif
                
		if (0 <= channel && channel <= 3)
			esb->dma0_single_mask = (unsigned char) channel;
		else
			esb->dma1_single_mask = (unsigned char) (channel - 4);

	} /* if not bus_master */

	/* 
	 * if this is a 16bit ISA master  specifying  a DMA channel  for
	 * cascading,  initialize the channel he wants
	 */
	if (isa_master) {
		esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;
		channel_data->dma_mode_sw = CASCADE_MODE;
		if ((channel >= 0) && (channel <= 3))
			esb->dma0_mode = channel_data->dma_mode_sw | channel;
		else esb->dma1_mode = channel_data->dma_mode_sw | (channel - 4);

		if (0 <= channel && channel <= 3)
			esb->dma0_single_mask = (unsigned char) channel;
		else
			esb->dma1_single_mask = (unsigned char) (channel - 4);
		}

#ifdef __hp9000s300
	purge_dcache_physical();
#else
        if (options & DMA_READ) {
#ifdef EISA_TESTS    /* Flush cache to physical mem for positive read tests */
                fdcache_conditionally(dma_parms->spaddr, dma_parms->addr, dma_parms->count, 
		    c_always);
#else
                pdcache_conditionally(dma_parms->spaddr, dma_parms->addr, dma_parms->count, 
		    c_accessed); 
#endif /* EISA_TESTS */
                SYNC();
        } else {
                fdcache_conditionally(dma_parms->spaddr, dma_parms->addr, dma_parms->count, 
		    c_modified); 
                SYNC();
        }
#endif
	return(0);
}


/*------------------------------------------------------------------------*/
eisa_dma_cleanup(isc, dma_parms)
/*------------------------------------------------------------------------*/
/* eisa  masters  must  calculate  their  own dma and so we just free up
 * memory  allocations  for them.  isa masters "own" a dma channel which
 * also  needs to be  freed.  we do need to do all the  above as well as
 * calculate resid for slaves.
 */

struct isc_table_type *isc;
struct dma_parms *dma_parms;
{
	int channel;
	int controller, tc, x, i;
	int resid; 
	int reg_value;
	struct eisa_system_board *esb;
	int return_value = 0;
	struct dma_channel_data *channel_data;
	int isa_master = 0;
	struct addr_chain_type *ch_ptr;

	esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;
	channel = dma_parms->channel;

	/* make a determination between EISA and ISA masters */
	if ((channel == BUS_MASTER_DMA) && (dma_parms->dma_options & DMA_CASCADE)) {
		isa_master = 1;
		channel = dma_parms->dma_options & ISA_CHANNEL_MASK;
	}

	/* if not EISA masters */
	if (channel != BUS_MASTER_DMA || isa_master) {
		/*slaves and isa_masters need extra attention */

		if (channel > 7 || channel < 0 || channel == 4)
			return(ILLEGAL_CHANNEL);

		channel_data = &((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->dma_channels[channel];
		/* see if channel is locked with this isc */
		if (isc == channel_data->locked) {
			if ((channel >=0) && (channel <=3)) 
				controller = 0;
			else 
				controller = 1;

			/* disable the channel */
			if (controller) 
				esb->dma1_single_mask = (unsigned char)(0x4 | (channel - 4));
			else 
				esb->dma0_single_mask = (unsigned char)(0x4 | channel);

			/* dma slaves only */
			if (!isa_master) { 
				/* lets do resid calcs for slaves */
				resid = 0;

				/* registers read are based on which controller */
				if (controller) {
					x = spl6();
					esb->dma1_clr_byte_ptr = 1;
					if (channel == 5) {	
						resid = esb->dma1_ch5_count;
						resid += esb->dma1_ch5_count<<8;
						resid += esb->dma1_ch5_high_count<<16;
					} else if (channel == 6) {
						resid = esb->dma1_ch6_count;
						resid += esb->dma1_ch6_count<<8;
						resid += esb->dma1_ch6_high_count<<16;
					} else if (channel == 7) {
						resid = esb->dma1_ch7_count;
						resid += esb->dma1_ch7_count<<8;
						resid += esb->dma1_ch7_high_count<<16;
					}
					((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->
					    dma1_status_sw |= esb->dma1_status;
					tc = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info)
					    )->dma1_status_sw & (1 << (channel - 4));
					((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->
					    dma1_status_sw &= ~(1 << (channel - 4));
					splx(x);
				} else {
					x = spl6();
					esb->dma0_clr_byte_ptr = 1;
					if (channel == 0) {	
						resid = esb->dma0_ch0_count;
						resid += esb->dma0_ch0_count<<8;
						resid += esb->dma0_ch0_high_count<<16;
					} else if (channel == 1) {
						resid = esb->dma0_ch1_count;
						resid += esb->dma0_ch1_count<<8;
						resid += esb->dma0_ch1_high_count<<16;
					} else if (channel == 2) {
						resid = esb->dma0_ch2_count;
						resid += esb->dma0_ch2_count<<8;
						resid += esb->dma0_ch2_high_count<<16;
					} else if (channel == 3) {
						resid = esb->dma0_ch3_count;
						resid += esb->dma0_ch3_count<<8;
						resid += esb->dma0_ch3_high_count<<16;
					}
					((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->
					    dma0_status_sw |= esb->dma0_status;
					tc = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info)
					    )->dma0_status_sw & (1 << channel);
					((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->
					    dma0_status_sw &= ~(1 << channel);
					splx(x);
				}

				/* lets see if it "rolled" over */
				if (resid != 0xffffff) {
					/* we must therefore have some count left */
					if (tc) 
						panic("EISA: Bad Converter Hardware suspected-TC");
					else
						resid++;
				} else {
					/* must therefore have completed */
					if (tc) 
						resid = 0;
					else
						return_value = -1;
				}


				if (return_value != -1) {
					if (dma_parms->dma_options & DMA_16WORD)
						return_value = resid * 2;
					else 
						return_value = resid;
				} /* return value == -1 */
			} /* slave mode dma cleanup */
		} /* isc == channel->locked */

		/* both slaves and isa masters need to give up dma channel */
		eisa_free_dma(isc, channel);
	}
	
#if defined(__hp9000s800) && defined(_WSIO)
	/* 
	 * if a buflet_info  structure exists, we need to copy the info from the
	 * pagelet or buflet into the  original  buffer, then we need to free up
	 * the buflets and buflet_info structure if it exists.  this is done for
	 * both eisa and isa masters, and slaves.
	 */

	if (dma_parms->buflet_key != NULL) {
		if (dma_parms->flags & PAGELETS) { /* we need to deal with pagelets here */
			if (dma_parms->buflet_key->buflet_beg) {
				privlbcopy(KERNELSPACE,
					(caddr_t)((unsigned)dma_parms->buflet_key->buflet_beg +
					((unsigned)dma_parms->buflet_key->buffer_beg & 0xfff)),
					dma_parms->buflet_key->space, 
					dma_parms->buflet_key->buffer_beg,
					dma_parms->buflet_key->cnt_beg); 
				FREE(dma_parms->buflet_key->buflet_beg, M_IOSYS); 
			}		

			if (dma_parms->buflet_key->buflet_end) {
				privlbcopy(KERNELSPACE, 
					(caddr_t)((unsigned)dma_parms->buflet_key->buflet_end +
					((unsigned)dma_parms->buflet_key->buffer_end & 0xfff)),
					dma_parms->buflet_key->space, 
					dma_parms->buflet_key->buffer_end,
					dma_parms->buflet_key->cnt_end); 
				FREE(dma_parms->buflet_key->buflet_end, M_IOSYS); 
			}		

		} else { /* this is the buflet case */
			if (dma_parms->buflet_key->buflet_beg) {
				privlbcopy(KERNELSPACE,
					(caddr_t)((unsigned)dma_parms->buflet_key->buflet_beg +
					((unsigned)dma_parms->buflet_key->buffer_beg & 
					(CPU_IOLINE-1))),
					dma_parms->buflet_key->space, 
					dma_parms->buflet_key->buffer_beg,
					dma_parms->buflet_key->cnt_beg); 
				FREE(dma_parms->buflet_key->buflet_beg, M_DMA); 
			}		

			/* 
			 * note: in the end_buflet case, we do not need to add the original
			 * buffers offset since it is "always" 0.
			 */
			if (dma_parms->buflet_key->buflet_end) {
				privlbcopy(KERNELSPACE, dma_parms->buflet_key->buflet_end,
					dma_parms->buflet_key->space, 
					dma_parms->buflet_key->buffer_end,
					dma_parms->buflet_key->cnt_end); 
				FREE(dma_parms->buflet_key->buflet_end, M_DMA); 
			}		
		}

		FREE(dma_parms->buflet_key, M_DMA);
	}
#endif
	
	if (!(dma_parms->dma_options & DMA_SKIP_IOMAP) && dma_parms->key != -1)
		free_iomap_entries(isc, dma_parms);

	/* free the chain if services allocated it */
	if (!(dma_parms->flags & NO_ALLOC_CHAIN))
		FREE(dma_parms->chain_ptr, M_DMA);

	return(return_value);
}


/*------------------------------------------------------------------------*/
lock_isa_channel(isc, dma_parms)
/*------------------------------------------------------------------------*/

register struct isc_table_type *isc;
register struct dma_parms *dma_parms;
{
	register int channel = dma_parms->channel;
	register struct eisa_system_board *esb;
	register struct dma_channel_data *channel_data;
	struct driver_to_call *this_entry;
	int x;
	
	esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;

	if (!(dma_parms->flags & NO_CHECK)) {	
		if (channel > 7 || channel < 0 || channel == 4)
			return(ILLEGAL_CHANNEL);
	}

	channel_data = &(((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->dma_channels[channel]);
	if (eisa_get_dma(isc, channel)) {
		x = spl6();
		if (dma_parms->drv_routine) {
			if (channel_data->driver_wait_list == NULL) {
				MALLOC(channel_data->driver_wait_list, 
				   struct driver_to_call *, sizeof(struct driver_to_call),
				   M_IOSYS, M_NOWAIT);
				if (channel_data->driver_wait_list == NULL) {
					splx(x);
					return(MEMORY_ALLOC_FAILED);
				}
				channel_data->driver_wait_list->drv_routine = dma_parms->drv_routine;
				channel_data->driver_wait_list->drv_arg = dma_parms->drv_arg;
				channel_data->driver_wait_list->isc = isc;
				channel_data->driver_wait_list->next = NULL;
			} else {
				this_entry = channel_data->driver_wait_list;
				while (this_entry->next != NULL)
					this_entry = this_entry->next;
				MALLOC(this_entry->next, struct driver_to_call *,
					sizeof(struct driver_to_call), M_IOSYS, M_NOWAIT);
				this_entry = this_entry->next;
				if (this_entry == NULL) {
					splx(x);
					return(MEMORY_ALLOC_FAILED);
				}
				this_entry->drv_routine = dma_parms->drv_routine;
				this_entry->drv_arg = dma_parms->drv_arg;
				this_entry->isc = isc;
				this_entry->next = NULL;
			}
		}
		splx(x);
		return(RESOURCE_UNAVAILABLE);
	}

	/* set cascade mode */
	channel_data->dma_mode_sw = CASCADE_MODE;

	if ((channel >= 0) && (channel <= 3))
		esb->dma0_mode = channel_data->dma_mode_sw | channel;
	else esb->dma1_mode = channel_data->dma_mode_sw | (channel - 4);

	/* enable channel */
	if( 0 <= channel && channel <= 3)
		esb->dma0_single_mask = (unsigned char)channel;
	else
		esb->dma1_single_mask = (unsigned char)(channel - 4);

	return(0);
}


/*------------------------------------------------------------------------*/
unlock_isa_channel(isc, dma_parms)
/*------------------------------------------------------------------------*/

register struct isc_table_type *isc;
register struct dma_parms *dma_parms;
{
	register int channel;
	int controller, x;
	register struct eisa_system_board *esb;
	register struct dma_channel_data *channel_data;

	esb = ((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->esb;
	channel = dma_parms->channel;
	channel_data = &(((struct eisa_bus_info *)(isc->bus_info->spec_bus_info))->dma_channels[channel]);

	/* if owns channel, disable it */
	if (isc == channel_data->locked) {
		if( 0 <= channel && channel <= 3)
			esb->dma0_single_mask = (unsigned char)(0x4 | channel);
		else 
			esb->dma1_single_mask = (unsigned char)(0x4 | (channel - 4));
	}

	/* give back channel, or get him off list, if he doesn't own it */
	eisa_free_dma(isc, channel);

	return(0);
}
