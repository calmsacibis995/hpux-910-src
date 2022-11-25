/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/vme.c,v $
 * $Revision: 1.8.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 14:06:41 $
 */
/* HPUX_ID: @(#)vme.c	55.1     88/12/23  */


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


/*
 * WORK_TO_BE_DONE
 * NOTE: This is to ensure that the 300 and 700 files are kept totally distinct
 * until such time as they can be merged in the post 9.0 timeframe.
 */
#ifdef __hp9000s300

#include "../h/types.h"
#include "../h/errno.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../wsio/timeout.h"		/* for hpibio.h */
#include "../h/io.h"
#include "../s200io/vme2.h"
#include "../wsio/tryrec.h"
#include "../wsio/iobuf.h"
#include "../s200io/dma.h"
#include "../s200/param.h"
#include "../s200/pte.h"
#include "../h/vmmac.h"
#include "../h/malloc.h"
#include "../h/vas.h"
#include "../machine/vmparam.h"
#include "../h/debug.h"

/* time to wait for arbitration self-test to complete */
#define TEST_TIME 5
#define MAXPHYS (64 * 1024)

extern int bus_master_count;
extern int (*make_entry)();
extern caddr_t kvtophys();
extern caddr_t host_map_mem();
extern caddr_t unmap_host_mem();
extern int (*vme_init_routine)();

int (*stealth_saved_make_entry)();
int stealth_init();
int vme_intr_enable();
int vme_intr_disable();
int stealth_make_entry();
int vme_init();

struct bus_info_type *vme;
static int vme_opened;		/* global to enforce exclusive open */

/*-----------------------------------------------------------------------*/
vme_nop()
/*-----------------------------------------------------------------------*/
{ 
	return(0); 
}

/* vme_iosw table: used to call stealth_init at powerup */
struct drv_table_type vme_iosw =
{
	stealth_init,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
	vme_nop,
};

static struct buf vme_buf;
static struct buf *bp = &vme_buf;
static struct iobuf vme_iobuf;
static struct iobuf *iob = &vme_iobuf;

/*-----------------------------------------------------------------------*/
vme_last_attach()
/*-----------------------------------------------------------------------*/
{
	return(0);
}

int (*vme_attach)() = vme_last_attach;


/*-----------------------------------------------------------------------*/
stealth_link()
/*-----------------------------------------------------------------------*/
/* This routine links stealth_make_entry into the make_entry list of all
 * drivers  at  powerup.  It also  will  set up VME init  pointer  space
 * reserved in space.h to point to vme_init for MTV.
 */

{
	stealth_saved_make_entry = make_entry;
	make_entry = stealth_make_entry;
	vme_init_routine = vme_init;
	vme = NULL;
}


/*-----------------------------------------------------------------------*/
stealth_make_entry(id, isc)
/*-----------------------------------------------------------------------*/
/* Gets called for every dio card found at  powerup.  If the card is the
 * VME interface card, complete its arbitration self-test; if it passed,
 * inform  the  user  of  the  presence  of the  card  and  arrange  for
 * stealth_init to be called later.  If it fails, print an error message
 * to that effect.  If the card is not the VME interface,  pass it on to
 * the next driver.
 */

int id;
struct isc_table_type *isc;
{
	struct VME *card = (struct VME *) isc->card_ptr;
	struct vme_bus_info *vme_s;

	/* 
	 * if card is not VME interface, return to next driver 
	 */
	if (id != STEALTH_ID)
		return (*stealth_saved_make_entry)(id, isc);

	/*
	 * setup generic bus info structure
	 */
	vme = (struct bus_info_type *)kmem_alloc(sizeof(struct bus_info_type));
	bzero(vme, sizeof(struct bus_info_type));
	isc->bus_info = vme;

	/*
	 * set up specific bus info structure
	 */
	vme->spec_bus_info = (char *)kmem_alloc(sizeof(struct vme_bus_info));
	bzero(vme->spec_bus_info, sizeof(struct vme_bus_info));
	vme_s = (struct vme_bus_info *)vme->spec_bus_info;

	vme_s->vme_cp = card;
	vme_s->vme_expander = IS_STEALTH;
	vme_s->number_of_slots = STEALTH_NUM_SLOTS;
	vme_s->vme_base_addr = (caddr_t)STEALTH_BASE_ADDR;
	bus_master_count++;
	card->scratch = 0;		/* intialize scratch reg to 0 at powerup, undefined */
	bp->b_queue = iob;

	/* 
	 * complete arbitration self-test and print error message if it fails 
	 */
	if (vme_arb_test())
		 return io_inform("98577A VME interface", isc, -2, " ignored: arbitration self-test failed");

	isc->iosw = &vme_iosw; 
	isc->card_type = HP98577; 

	/* 
	 * print VME acknowledgement to console 
	 */ 
	return io_inform("HP98577 VME interface", isc, isc->int_lvl); 
} 


/*-----------------------------------------------------------------------*/
vme_init() 
/*-----------------------------------------------------------------------*/
/* This routine is called from kernel_initialize and initializes the MTV
 * Space for a pointer to it is reserved in space.h and  initialized  by
 * stealth_link()
 */

{ 
	struct VME *vme_cp;
	struct iomap_data *iomap_temp;
	int err;
	int *iomap_start;
	struct vme_bus_info *vme_s;

	/* 
	 * init MTV, called from kernel_initialize 
	 */
	if (vme != NULL) {	 
		/* stealth has been found and accepted, */
		(*vme_attach)(); /* call attach list and return */
		return(1);
	}

	/*
	 * initialize generic bus_info
	 */
	vme = (struct bus_info_type *)kmem_alloc(sizeof(struct bus_info_type));
	bzero(vme, sizeof(struct bus_info_type));

	/*
	 * initialize specific bus info
	 */
	vme->spec_bus_info = (char *)kmem_alloc(sizeof(struct vme_bus_info));
	bzero(vme->spec_bus_info, sizeof(struct vme_bus_info));
	vme_s = (struct vme_bus_info *)vme->spec_bus_info;

	/* 
	 * map in MTV registers 
	 */
	if ((vme_cp = (struct VME *)host_map_mem((caddr_t)MTV_BASE_ADDR, 16*NBPG)) == (struct VME *)0) {
		printf("VME WARNING: mapping in MTV registers failed\n");
		return(1);
	}
	vme_s->vme_cp = vme_cp;

	/*
	 * attempt to verify bus adapters existence
	 */
	if (testr(&vme_cp->card_id, 1)) {
		if (!(vme_cp->card_id == MTV_ID)) {
			unmap_host_mem(vme_cp, 16*NBPG);
			kmem_free(vme, sizeof(struct vme_bus_info));
			return(1);
		}
	} else return(1);

	/* 
	 * bus_master_count is not needed for MTV -- right? 
	 */
	vme_cp->reset = 1;
	vme_cp->scratch = 0;
	vme_cp->int_enable = VME_ENABLE_ALL;
	vme_cp->addr_mod = IOMAP_LOCATION;	/* 0xc0 */

	/* 
	 * complete arbitration self-test and print error message if it fails 
	 */
	if (vme_arb_test()) {
		unmap_host_mem(vme_cp, 16*NBPG);
		kmem_free(vme->spec_bus_info, sizeof(struct vme_bus_info));
		kmem_free(vme, sizeof(struct bus_info_type));
		printf("A1467 VME Expander Ignored: Arbitration Self-Test Failed\n");
		return(-1); 
	}

	vme_s->vme_expander = IS_MTV;
	vme_s->number_of_slots = MTV_NUM_SLOTS;
	vme_s->vme_base_addr = (caddr_t)MTV_BASE_ADDR;

	vme->host_iomap_base = (int *)((int)(vme_cp) + 0x8000);
	vme_s->iomap_size = MTV_IOMAP_SIZE;
	vme->bus_iomap_base = (int *)MTV_IOMAP_ADDR;

        /*
         * set up the iomap resource map
         */
        vme->iomap_rsrcmap = (struct rsrc_node*)
		kmem_alloc(sizeof(struct rsrc_node));

        if (vme->iomap_rsrcmap == NULL) {
                printf("vme init: memory allocation failure\n");
                return(1);
        }

        bzero(vme->iomap_rsrcmap, sizeof(struct rsrc_node));
        vme->iomap_rsrcmap->next = vme->iomap_rsrcmap;
        vme->iomap_rsrcmap->prev = vme->iomap_rsrcmap;


        if ((err = free_rsrc_entries(vme->iomap_rsrcmap, 0, MTV_IOMAP_SIZE)) <0) {
                printf("vme init: iomap rsrc allocation failure: %d\n", err);
                return(1);
        }

	/*
	 * indicate successful initialization
	 */
	printf("\n    VME Expander Initialized\n");

	/*
	 * call attach routines
	 */
	(*vme_attach)();
}


/*-----------------------------------------------------------------------*/
get_vme_hardware(hw_type)
/*-----------------------------------------------------------------------*/

struct vme_hardware_type *hw_type;
{
	struct vme_bus_info *vme_s;

	vme_s = (struct vme_bus_info *)vme->spec_bus_info;

	if (vme_s->vme_expander == IS_MTV) {
		hw_type->iomap_size = MTV_IOMAP_SIZE;
		hw_type->vme_expander = IS_400VME1;
	} else {
		hw_type->iomap_size = 0;
		hw_type->vme_expander = IS_300VME1;
	}
}


/*-----------------------------------------------------------------------*/
struct isc_table_type *vme_init_isc(isc_slot)
/*-----------------------------------------------------------------------*/

int isc_slot;
{
	struct isc_table_type *isc;

	if (isc_slot < 0 || isc_slot > 255) /* Bad slot number */
		return((struct isc_table_type *) 0);

	isc = (struct isc_table_type *)kmem_alloc(sizeof(struct isc_table_type));
	bzero(isc, sizeof(struct isc_table_type));

	isc->bus_type = VME_BUS;
	isc->bus_info = vme;

	if (isc_slot != 0) {
		if (isc_table[isc_slot] != NULL) {
			kmem_free(isc, sizeof(struct isc_table_type));
			return(NULL);
		} else {
			isc_table[isc_slot] = isc;
			isc->my_isc = isc_slot;
		}
	} else {
		isc_slot = VME_MIN_ISC;
		while (isc_table[isc_slot] != NULL) {
			isc_slot++;
			if (isc_slot > VME_MAX_ISC) {
				kmem_free(isc, sizeof(struct isc_table_type));
				return(NULL);
			}
		}
		isc_table[isc_slot] = isc;
		isc->my_isc = isc_slot;
	}
	
	/* if_info from vme_init, if we have one */
	
	isc->gfsw = (struct gfsw *)kmem_alloc(sizeof(struct isc_table_type));
	bzero(isc->gfsw, sizeof(struct gfsw));
	
	isc->next_ftn = NULL;
	/* isc->my_isc, passed in, if they want to use one? */ 

	return(isc);
}


/*-----------------------------------------------------------------------*/
int vme_arb_test()
/*-----------------------------------------------------------------------*/
/* This  routine  executes the  arbitration  self test.  It is called by
 * vme_init(),  stealth_make_entry(),  and as an  ioctl.  Returns:  0 if
 * passed, EIO if failed
 */

{
	struct VME *card;
	card = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;


	/* start hardware arbitration self test */
	card->control = card->control | VME_ARB_TEST;

 	/* snooze for 5 usec waiting for test to complete */
	snooze(TEST_TIME);

 	/* check for test completion */
	if (card->status & VME_TEST_NOT_DONE) {
 		/* if not complete, disable test, return fail (EIO). */               
		return(EIO);
	} else {
 		/* if complete, disable test, return pass (0) */
		card->control &= ~VME_ARB_TEST;
		return(0);
	}
}


/*-----------------------------------------------------------------------*/
vme_isr()
/*-----------------------------------------------------------------------*/
/* Interrupt  service  routine.  Called when  vme_int_test  is executed.
 * The VME interface can interrupt ONLY during the interrupt  self test.
 * If the  hardware  test passes, we will get an  interrupt.  To process
 * it, disable the  interrupt  test and * set the test passed bit in the
 * VME interface scratch register.
 */

{
	struct VME *vme_cp;

	vme_cp = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;
	vme_cp->control &= ~VME_INT_TEST;	/* disable interrupt test */
	vme_cp->scratch &= ~VME_INT_FAILED;	/* report that test passed */
}


/*-----------------------------------------------------------------------*/
vme_int_test(level)
/*-----------------------------------------------------------------------*/
/* execute the  hardware  interrupt  self test for the  interrupt  level
 * passed in "level".  Note that the VME bus should be quiet to run this
 * test.  The  sequence  is:  enable  "level"  interrupts  to pass  thru
 * STEALTH; if not already there:
 *
 *	link vme_isr onto the rupttable list for level;
 *	start the self test;
 *	wait for test to complete by snoozing;
 *	if test fails, disable test; 
 *	disable "level" interrupts from VME box;
 *	return EIO for failure, 0 for success.
 */

int level;
{
	int status = 0;
	struct VME *card;
	card = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;

	/* 
	 * can only test interrupt levels 1 - 6 
	 */
	if ( (level < 1) || (level > 6) )
		return(EINVAL);

	/* 
	 * enable "level" interrupts from vme 
	 */
	vme_intr_enable(level);	                
	if ((card->scratch & (1 << level)) == 0) {
		isrlink(vme_isr, level, &card->control, VME_INTERRUPT, VME_INTERRUPT, 0, 0);
		card->scratch |= 1 << level;
	} 

	/* initialize to failed, vme_isr will clear on success */
	card->scratch |= VME_INT_FAILED; 

	/* start test */
	card->control |= VME_INT_TEST;		

	/* wait for test to complete */
	snooze(10);				

	if (card->scratch & VME_INT_FAILED ) {
		card->control &= ~VME_INT_TEST;
		status = EIO;
	}

	/* disable "level" interrupts from vme */
	vme_intr_disable(level);		
	return(status);
}


/*-----------------------------------------------------------------------*/
stealth_init(isc)
/*-----------------------------------------------------------------------*/
/* Initialize  stealth  interface  card.  Called from  kernel_initialize
 * thru iosw table after make_entry  routines have been called.  Enables
 * signals  from the VME bus to pass thru to  processor  and enables all
 * interrupt levels.
 *
 * This is done after the backplane  has been  searched for dio cards so
 * that the search  cannot find any VME cards.  If a VME card lives at a
 * select code  boundary and the kernel tries to treat it as a dio card,
 * who knows  what will  happen -- so the VME box is not turned on until
 * this search has completed.
 */

register struct isc_table_type *isc;
{
	struct VME *card = (struct VME *) isc->card_ptr;

	card->reset = (unsigned char) 0;
	card->int_enable = VME_ENABLE_ALCM | VME_ENABLE_ALL;
	return(0);
}


/*-----------------------------------------------------------------------*/
vme_arb_mode(new_mode)
/*-----------------------------------------------------------------------*/
/* Select the mode for  arbitration for the VME bus.  This can be called
 * from the  kernel  by a VME  driver  or thru the  ioctl  to allow  the
 * selecting  of one of the  arbitration  modes on VME:  round  robin or
 * priority.  Priority is default.
 */

int new_mode;
{

	struct VME *card;
	card = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;

	switch (new_mode) {
		case VME_RR: card->control = card->status | VME_ARB_RR; return(0);
		case VME_PRI: card->control = card->status & ~VME_ARB_RR; return(0);
		default: /*bad argument*/ return(EINVAL);
	}
}


/*-----------------------------------------------------------------------*/
vme_intr_enable(int_lvl)
/*-----------------------------------------------------------------------*/
/* Enable  VME   interrupts   on  int_lvl   level.  This  is  called  by
 * vme_int_test and thru ioctl.
 */

int int_lvl;
{

	struct VME *card;
	card = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;

	switch (int_lvl) {
		case 7: card->int_enable |= VME_ENABLE_7; return(0);
		case 6: card->int_enable |= VME_ENABLE_6; return(0);
		case 5: card->int_enable |= VME_ENABLE_5; return(0);
		case 4: card->int_enable |= VME_ENABLE_4; return(0);
		case 3: card->int_enable |= VME_ENABLE_3; return(0);
		case 2: card->int_enable |= VME_ENABLE_2; return(0);
		case 1: card->int_enable |= VME_ENABLE_1; return(0);
		case 0: card->int_enable |= VME_ENABLE_ALL; return(0);
		default: /*bad argument*/ return(EINVAL);
	}
}


/*-----------------------------------------------------------------------*/
vme_intr_disable(int_lvl)
/*-----------------------------------------------------------------------*/
/* Disable  VME   interrupts  on  int_lvl  level.  This  is  called  by
 * vme_int_test and thru ioctl.
 */

int int_lvl;
{

	struct VME *card;
	card = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;

	switch (int_lvl) {
		case 7: card->int_enable &= ~VME_ENABLE_7; return(0);
		case 6: card->int_enable &= ~VME_ENABLE_6; return(0);
		case 5: card->int_enable &= ~VME_ENABLE_5; return(0);
		case 4: card->int_enable &= ~VME_ENABLE_4; return(0);
		case 3: card->int_enable &= ~VME_ENABLE_3; return(0);
		case 2: card->int_enable &= ~VME_ENABLE_2; return(0);
		case 1: card->int_enable &= ~VME_ENABLE_1; return(0);
		case 0: card->int_enable &= ~VME_ENABLE_ALL; return(0);
		default: /*bad argument*/ return(EINVAL);
	}
}


/*-----------------------------------------------------------------------*/
stealth_open(dev, flag, uio)
/*-----------------------------------------------------------------------*/
/* Open the  STEALTH  device  file.  Note:  the only reason to open this
 * device  is to  perform  the  ioctl's.  The  open  is  implemented  as
 * exclusive.
 */

dev_t dev;
int flag;
struct uio *uio;
{
	if (vme_opened)
		return(EACCES);
	vme_opened++;
	return(0);
}


/*-----------------------------------------------------------------------*/
stealth_close(dev)
/*-----------------------------------------------------------------------*/
/* Close the STEALTH device file. Implements exclusive open.            
 */

dev_t dev;
{
	vme_opened = 0;
	return(0);
}


/*-----------------------------------------------------------------------*/
stealth_ioctl(dev, cmd, arg)
/*-----------------------------------------------------------------------*/
/*  Perform ioctl's to the VME interface card.
 *
 * Possible ioctl's are:
 * 	VMEARBTEST: 	execute arbitration self test on card.
 * 		    	VME bus should be quiet. No arguments.
 * 
 *	VMEINTTEST: 	execute interrupt self test on level 
 *	            	passed by arg.  VME bus should be quiet.
 *		    	Arg = interrupt level to test.
 * 
 * 	VMEARBMODE: 	set arbitration mode for VME bus arbiter.
 *			Arg = VME_PRI or VME_RR.
 * 
 * 	VMEINTENABLE:	enable interrupt level to pass from VME
 * 			to DIO.  Arg = interrupt level to enable
 * 			or VME_ENABLE_ALL to enable all levels.
 * 			(see vme_intr_enable)
 * 
 * 	VMEINTDISABLE:	disable interrupt level to pass from VME
 * 			to DIO.  Arg = interrupt level to disable
 * 			or VME_DISABLE_ALL to disable all levels.
 * 			(see vme_intr_disable)
 * 
 * 	VMERESET:	reset VMEinterface card to its powerup state.
 * 			Signals from VME disabled, all interrupt levels
 * 			from VME disabled, arbitration mode = priority.
 * 			VME bus should be quiet.
 *
 *	VMESET_AM:      (MTV Only) set the address modifier mode.
 *			Possible settings:
 *
 *			A16_SUP: 
 *			A16: 
 *			A24_DATA: 
 *			A24_PRGRM: 
 *			A24_SUP_DATA:
 *			A24_SUP_PRGRM:
 *			A32_DATA:
 *			A32_PRGRM:
 *			A32_SUP_DATA:
 *			A32_SUP_PRGRM:
 */
	
dev_t dev;
int cmd;
struct vme_ioctl_arg *arg;
{

	struct VME *vme_cp;
	vme_cp = ((struct vme_bus_info *)vme->spec_bus_info)->vme_cp;

	switch(cmd) {
		case VMEARBTEST:    return vme_arb_test();
		case VMEINTTEST:    return vme_int_test(arg->arg);
		case VMEARBMODE:    return vme_arb_mode(arg->arg);
		case VMEINTENABLE:  return vme_intr_enable(arg->arg);
		case VMEINTDISABLE: return vme_intr_disable(arg->arg);
		case VMERESET:      vme_cp->reset = 0; return(0);
		case VMESET_AM:
			switch(arg->arg) {
				case A16_SUP: 
					vme_cp->addr_mod |= A16_AM_MASK; 
					return(0);
				case A16: 
					vme_cp->addr_mod &= ~A16_AM_MASK; 
					return(0);
				case A24_DATA: 
					vme_cp->addr_mod &= ~A24_AM_MASK; 
					return(0);
				case A24_PRGRM: 
					vme_cp->addr_mod &= ~A24_AM_MASK;
					vme_cp->addr_mod |= 0x2;
					return(0);
				case A24_SUP_DATA:
					vme_cp->addr_mod &= ~A24_AM_MASK;
					vme_cp->addr_mod |= 0x4;
					return(0);
				case A24_SUP_PRGRM:
					vme_cp->addr_mod &= ~A24_AM_MASK;
					vme_cp->addr_mod |= 0x6;
					return(0);
				case A32_DATA:
					vme_cp->addr_mod &= ~A32_AM_MASK;
					return(0);
				case A32_PRGRM:
					vme_cp->addr_mod &= ~A32_AM_MASK;
					vme_cp->addr_mod |= 0x8;
					return(0);
				case A32_SUP_DATA:
					vme_cp->addr_mod &= ~A32_AM_MASK;
					vme_cp->addr_mod |= 0x10;
					return(0);
				case A32_SUP_PRGRM:
					vme_cp->addr_mod &= ~A32_AM_MASK;
					vme_cp->addr_mod |= 0x18;
					return(0);
				default: return(EINVAL);
			}
		default:
			return(EINVAL);
	}  /* end switch */
}


/*-----------------------------------------------------------------------*/
int allocate_vector(vect_numb, isr_addr)
/*-----------------------------------------------------------------------*/
/* This routine sets up an  interrupt  vector for user card  interrupts.
 * It selects a free  exception  vector  and sets its value to  isr_addr
 * returning the vector number it chose.
 */

register vect_numb;
caddr_t	isr_addr;
{
	extern	caddr_t intr_vect_table[];

	/* allocate next available */
	if (vect_numb == 0) {
		for (vect_numb = 64; vect_numb <= 255; vect_numb++) {
			if (intr_vect_table[vect_numb] == NULL) {
				intr_vect_table[vect_numb] = isr_addr;
				return(vect_numb);
			}
		}
	} else {
		/* allocate requested vector number */

		/* check if in range */
		if (vect_numb >= 64 && vect_numb <= 255) 
		{
			/* give him the vector if available */
			if (intr_vect_table[vect_numb] == NULL) 
			{
				/* put caller isr in vector table */
				intr_vect_table[vect_numb] = isr_addr;
				return(vect_numb);
			}
		}
	}
	/* oops -- not available */
	return(-1);
}


/*-----------------------------------------------------------------------*/
caddr_t vme_dma_init()
/*-----------------------------------------------------------------------*/
/* Allocate a  vme_dma_chain  array to hold up to 64K worth of  transfer
 * information.
 */

{
	return((caddr_t) calloc((sizeof(struct vme_dma_chain)) * 
	    (MAXPHYS/NBPG + 1)));
}


/*-----------------------------------------------------------------------*/
vme_dma_build_chain(log_addr, count, chain_ptr)
/*-----------------------------------------------------------------------*/
/* This routine  constructs  an array of physical  DMA  transfers  to be
 * completed.
 */ 

caddr_t log_addr;
unsigned short count;
struct vme_dma_chain * chain_ptr;	
{
	int p_offset;

	while (count > 0) {
		chain_ptr->phys_addr = kvtophys(log_addr);
		p_offset = (unsigned) log_addr & PGOFSET;
		chain_ptr->count = NBPG - p_offset;    /* NBPG is 4K */

		if (chain_ptr->count > count)		
			chain_ptr->count = count;

		log_addr += chain_ptr->count;
		count -= chain_ptr->count;
		chain_ptr++;
	}

	chain_ptr->phys_addr = 0;	/*indicates end of chain*/
	return(0);
}


/*-----------------------------------------------------------------------*/
vme_dma_setup(isc, dma_parms)
/*-----------------------------------------------------------------------*/

register struct isc_table_type *isc;
register struct dma_parms *dma_parms;
{
	register int count = dma_parms->count;
	register caddr_t addr = dma_parms->addr;
	struct vme_bus_info *bus_info = ((struct vme_bus_info *)(isc->bus_info->spec_bus_info));
	int num_pages, temp_count, this_iomap, bytes_this_page, vme_base;
	register unsigned phys_addr;
	caddr_t temp_addr;
	int *iomap_start;
	int err, spl;

	/*
	 * do a preliminary check if requested
	 */
	if (!(dma_parms->flags & NO_CHECK)) {
		if(count > MAXPHYS)
			return(TRANSFER_SIZE);
		if((dma_parms->dma_options & VME_A24_DMA) && 
		    (dma_parms->dma_options & VME_USE_IOMAP == 0))
			return(INVALID_OPTIONS_FLAGS);
		if((dma_parms->dma_options & VME_USE_IOMAP) && 
		    (bus_info->vme_expander == IS_STEALTH))
			return(INVALID_OPTIONS_FLAGS);
	}

	/*
	 * determine whether iomap hardware will be used
	 */
	if (dma_parms->dma_options & VME_USE_IOMAP) {
		num_pages = (count/NBPG) + 2;

		spl = spl6();
		if(lock_iomap_entries(isc, num_pages, dma_parms)) {
			if (dma_parms->drv_routine) {
				if ((err = put_on_wait_list(isc, dma_parms->drv_routine, 
			 	    dma_parms->drv_arg)) < 0) {
					splx(spl);
					return(err);
				}
			}
			splx(spl);
			return(RESOURCE_UNAVAILABLE);
		}
		splx(spl);

		iomap_start = isc->bus_info->host_iomap_base;

		this_iomap = dma_parms->key;
		temp_count = count;
		vme_base = (int)(isc->bus_info->bus_iomap_base);

		while (temp_count) {
			phys_addr = (unsigned)kvtophys(addr);
			bytes_this_page = NBPG - ((unsigned)addr & PGOFSET);
			if (bytes_this_page > temp_count)
				bytes_this_page = temp_count;
			addr += bytes_this_page;
			temp_count -= bytes_this_page;

			if ((phys_addr & 0xff000000) == vme_base)
				phys_addr &= ~vme_base;
			(*(iomap_start + this_iomap)) = phys_addr >> PGSHIFT;
			this_iomap++;
		}

		if (!(dma_parms->flags & NO_ALLOC_CHAIN)) {
			MALLOC(dma_parms->chain_ptr, struct addr_chain_type *, 
				sizeof(struct addr_chain_type), M_IOSYS, M_NOWAIT);
			if (dma_parms->chain_ptr == NULL) {
				free_iomap_entries(isc, dma_parms);
				return(MEMORY_ALLOC_FAILED);
			}
		}
		dma_parms->chain_count = 1;
	
		dma_parms->chain_ptr->phys_addr = (caddr_t)((unsigned)isc->bus_info->bus_iomap_base
		    + (dma_parms->key * 0x1000) + ((unsigned)addr & 0xfff));

		dma_parms->chain_ptr->count = count;
	} else { 	
		/* 
		 * not using I/O map entries 
		 */
		if (!(dma_parms->flags & NO_ALLOC_CHAIN)) {
			MALLOC(dma_parms->chain_ptr, struct addr_chain_type *, 
				sizeof(struct addr_chain_type)*(count/NBPG + 3), M_IOSYS, M_NOWAIT);
			if (dma_parms->chain_ptr == NULL) {
				return(MEMORY_ALLOC_FAILED);
			}
		}

		dma_parms->chain_count = io_dma_build_chain(dma_parms->spaddr, addr, count, 
		    dma_parms->chain_ptr);
	}

	/*
	 * purge data cache
	 */
	purge_dcache_physical();
}


/*-----------------------------------------------------------------------*/
vme_dma_cleanup(isc, dma_parms)
/*-----------------------------------------------------------------------*/

register struct isc_table_type *isc;
register struct dma_parms *dma_parms;
{

	if (dma_parms->dma_options & VME_USE_IOMAP)
		free_iomap_entries(isc, dma_parms);

	if (!(dma_parms->flags & NO_ALLOC_CHAIN))
		FREE(dma_parms->chain_ptr, M_IOSYS);

	return(0);
}




/*
 * the following routines are added to give some semblance of a 700 VME look
 * to 400 drivers.
 */

/* VME_ISR_TABLE_SIZE must be a power of 2 */
#define VME_ISR_TABLE_SIZE 256
#define VME_ISR_MASK VME_ISR_TABLE_SIZE-1

struct vme_isr_table_type *vme_isr_table[VME_ISR_TABLE_SIZE];

/*----------------------------------------------------------------------*/
struct vme_isr_table_type *search_vme_isr_table(irq_size, vector)
/*----------------------------------------------------------------------*/
/* this routine either returns NULL or a pointer to the correct struct
 */

int irq_size;
unsigned int vector;
{
	struct vme_isr_table_type *ptr = vme_isr_table[vector & VME_ISR_MASK];

#ifdef NEEDS_THOUGHT
	while (ptr != NULL && ((ptr->status_id & ptr->interupter_type) != 
	    (ptr->interupter_type & vector)))
		ptr = ptr->next;
#endif

	return(ptr);
}



/*----------------------------------------------------------------------*/
int vme_isrlink(isc, isr_addr, level_vector, arg, sw_trig_lvl)
/*----------------------------------------------------------------------*/

struct isc_table_type *isc;
int (*isr_addr)();
int level_vector;
int arg;
int sw_trig_lvl;
{
	extern caddr_t intr_vect_table[];
	extern int vme_IRQ();
	struct vme_isr_table_type *ptr;

	if (isc->bus_type != VME_BUS) return(-1);

	/*
	 * see if it is already taken
	 */
	if (level_vector) {
		if (search_vme_isr_table(((struct vme_info*)isc->if_info)->interupter_type, level_vector)) 
			return(-2);
	} else {
		int i;

		/*
		 * choose to keep number of linked list searches down
		 */
#ifdef __hp9000s300
		for (i=64; i < VME_ISR_TABLE_SIZE && vme_isr_table[i] != NULL; i++);
#else
		for (i=0; i < VME_ISR_TABLE_SIZE && vme_isr_table[i] != NULL; i++);
#endif

		if (i == VME_ISR_TABLE_SIZE) {
			return(-2);
		}

		level_vector = i;
	}

	IOASSERT(vme_isr_table[level_vector] == NULL);

	/* we can have it */
	MALLOC(ptr, struct vme_isr_table_type *, sizeof(struct vme_isr_table_type),
	    M_IOSYS, M_WAITOK);
	if (!ptr) return(-2);

	ptr->status_id = level_vector;
	ptr->isc = isc;
	ptr->interupter_type =  ((struct vme_info*)isc->if_info)->interupter_type;
	ptr->isr_addr = isr_addr;
	ptr->arg = arg;
	ptr->sw_trig_lvl = sw_trig_lvl;
	ptr->next = vme_isr_table[level_vector & VME_ISR_MASK];

	vme_isr_table[level_vector & VME_ISR_MASK] = ptr;

#ifdef __hp9000s300
	intr_vect_table[level_vector] = (caddr_t)vme_IRQ;	
	return(level_vector);
#else
	return(-1);
#endif
}


/*--------------------------------------------------------------------*/
vme_IRQ(vector)
/*--------------------------------------------------------------------*/

int vector;
{
	struct vme_isr_table_type *ptr;

	if (ptr = search_vme_isr_table(0, vector)) {
			(*ptr->isr_addr)(ptr->isc, vector, ptr->arg);
	} else {
		panic("vme_IRQ: bad vector victor");
	}
}

#endif
