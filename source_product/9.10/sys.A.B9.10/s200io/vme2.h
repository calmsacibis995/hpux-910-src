/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/vme2.h,v $
 * $Revision: 1.5.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/03 15:58:56 $
 */
/* @(#) $Revision: 1.5.84.4 $ */    
#ifndef _MACHINE_VME2_INCLUDED /* allows multiple inclusion */
#define _MACHINE_VME2_INCLUDED
#ifdef __hp9000s300

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
********VME interface 98577A definitions
*/

#ifdef _KERNEL_BUILD
#include "../h/ioctl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif /* _KERNEL_BUILD */

/* VME card ids */

#define STEALTH_ID	49
#define MTV_ID		113

/* flags for users to set bus arbitration scheme */

#define VME_PRI		0	/* set arbitration scheme to priority 	*/
#define VME_RR		1	/* set arbitration scheme to round robin */

/* VME registers declaration */

struct VME {
		unsigned char pad0;		/* non-existent */
		unsigned char card_id;		/* R = card id; W = reset card */
		unsigned char pad2;		/* non-existent */
		unsigned char status;		/* R = status; W = control */
		unsigned char pad4;		/* non-existent */
		unsigned char int_enable;	/* enable/disable interrupt levels */
		unsigned char pad6;		/* non-existent */
		unsigned char scratch;		/* extra, scratch register */
		unsigned char pad8;		/* non-existent */
		unsigned char addr_mod;		/* MTV only, address modifier register */
	    };

/* aliases for write registers */

#define reset card_id
#define control status

/* status/control register masks -- register 3 */

#define VME_INT_TEST		0x01	/* bit to enable vme interrupt self-test */
#define VME_ARB_TEST 		0x02	/* bit to enable vme arbitration self-test */
#define VME_TEST_NOT_DONE	0x20	/* vme arbitration self-test is in progress */
#define VME_ARB_RR		0x04	/* set vme arbitration scheme to round robin */
#define MTV_SNOOP		0x08	/* MTV only bit, enables 68040 snooping */
#define VME_ARB_PRI		0x00	/* set vme arbitration scheme to priority */
#define VME_INTERRUPT		0x40	/* vme card is interrupting (self-test only) */

/* self-test failure flag */

#define VME_INT_FAILED		0x01	/* interrupt test failed, use bit 1 in scratch */
					/* register (register 7) for this value	       */

/* card/interrupt level enables register masks -- register 5 */

#define VME_ENABLE_ALL	0xFF	/* enable all interrupt levels and vme interrupts */
#define VME_ENABLE_ALCM	0x80	/* enable interface card to pass interrupts from vme to dio */
#define VME_ENABLE_7	0x40	/* enable level 7 interrupts from vme		*/
#define VME_ENABLE_6	0x20	/* enable level 6 interrupts from vme		*/
#define VME_ENABLE_5	0x10	/* enable level 5 interrupts from vme		*/
#define VME_ENABLE_4	0x08	/* enable level 4 interrupts from vme		*/
#define VME_ENABLE_3	0x04	/* enable level 3 interrupts from vme		*/
#define VME_ENABLE_2	0x02	/* enable level 2 interrupts from vme		*/
#define VME_ENABLE_1	0x01	/* enable level 1 interrupts from vme		*/

/* address modifier bits -- register 9, MTV only */
#define A16_AM_MASK		0x1
#define A24_AM_MASK		0x6
#define A32_AM_MASK		0x18
#define VME_BLOCK_MODE		0x20
#define IOMAP_LOCATION		0xc0

/* structure used by vme_ioctl to pass arguments in */

struct vme_ioctl_arg
	{
	 int arg;
	};

/* vme ioctl commands */

/* execute vme arbitration self-test	*/
#define VMEARBTEST	_IO('V', 1)

/* execute vme interrupt self-test for level passed in by arg	*/
#define VMEINTTEST	_IOW('V', 2, struct vme_ioctl_arg)

/* set vme arbitration mode to value passed in by arg (VME_RR or VME_PRI) */
#define VMEARBMODE	_IOW('V', 3, struct vme_ioctl_arg)

/* enable vme interrupts on level passed in by arg */
#define VMEINTENABLE	_IOW('V', 4, struct vme_ioctl_arg)

/* disable vme interrupts on level passed in by arg */
#define VMEINTDISABLE	_IOW('V', 5, struct vme_ioctl_arg)

/* reset vme interface card to its power-up state -- all interrupts disabled */
#define VMERESET	_IO('V', 6)

/* set address modifer, valid on MTV only */
#define VMESET_AM	_IOW('V', 7, struct vme_ioctl_arg)

/* address modifier bits used for VMESET_AM ioctl */
#define A16_SUP		0x2d
#define A16		0x29
#define A24_DATA	0x39
#define A24_PRGRM	0x3a
#define A24_SUP_DATA	0x3d
#define A24_SUP_PRGRM	0x3e
#define A32_DATA	0x09
#define A32_PRGRM	0x0a
#define A32_SUP_DATA	0x0d
#define A32_SUP_PRGRM	0x0e

/* These macros are not necessary.  They no longer do anything */
/* #define BEGIN_ISR 	*/
/* #define END_ISR	*/


/* This data structure is used by customer written VME drivers that use DMA */

struct vme_dma_chain
	{
	 caddr_t phys_addr;
 	 unsigned short count;
	};

struct vme_bus_info {
	int vme_expander;
	struct VME *vme_cp;
	caddr_t vme_base_addr;
	struct iomap_data *iomap_data;
	int iomap_size;
	int *iomap;
	struct driver_to_call *iomap_wait_list;
	int vme_iomap_base;
	int number_of_slots;
};


/* vme_expander bits */
#define IS_STEALTH	0x1
#define IS_MTV	0x2

#define IS_300VME1	IS_STEALTH
#define IS_400VME1	IS_MTV

struct vme_hardware_type {
	int iomap_size;
	int vme_expander;
};

/* vme_bus_info constants for MTV */
#define MTV_BASE_ADDR	0x40000000
#define MTV_IOMAP_SIZE	1024
#define MTV_IOMAP_ADDR	0x40c00000
#define MTV_IOMAP_BASE	0xc00000
#define MTV_NUM_SLOTS	8

/* vme_bus_info constants for Stealth, most invalid */
#define STEALTH_BASE_ADDR	0x00000000
#define STEALTH_IOMAP_SIZE	-1
#define STEALTH_IOMAP_ADDR	-1
#define STEALTH_IOMAP_BASE	-1
#define STEALTH_NUM_SLOTS	4

/* bit definitions for flags field - bits cleared are defaults */
#define NO_CHECK	0x2	/* don't perform error checking */
#define NO_ALLOC_CHAIN	0x4	/* allocate chain for addr/count's */

/* dma_options bits for vme_dma_setup call */
#define VME_A32_DMA	0x1
#define VME_A24_DMA	0x2	
#define VME_USE_IOMAP	0x4	/* 0 = default for A32 and Stealth, 1 = default for A24 */

/* errors returned by dma_setup */
#define UNSUPPORTED_FLAG	-1
#define RESOURCE_UNAVAILABLE	-2
#define BUF_ALIGN_ERROR		-3
#define MEMORY_ALLOC_FAILED	-4
#define TRANSFER_SIZE		-5
#define INVALID_OPTIONS_FLAGS	-6
#endif /* __hp9000s300 */

/* valid isc table slot range for VME cards */
#define VME_MIN_ISC	0x50
#define VME_MAX_ISC	0x60

/* MTV address macros */
#define A16_ADDR(addr)  (0 <= (addr) && (addr) <= 0xffff)
#define A24_ADDR(addr)  (0x20000 <= (addr) && (addr) <= 0xffffff)
#define A32_ADDR(addr)  (0x41000000 <= (addr) && (addr) <= 0x7fffffff)


struct vme_isr_table_type {
	unsigned int status_id;		/* status_id generated by the card */
	int interupter_type;		/* type = IRQ_D32, IRQ_D16, IRQ_D8 */
	int (*isr_addr)();		/* address of interrupt service routine */
	int arg;			/* optional argumant to pass in */
	int sw_trig_lvl;		/* level at which to run isr */
	struct isc_table_type *isc;	/* pointer to isc */
	struct vme_isr_table_type *next;/* linked list */
};

struct vme_info {
	int interupter_type;	
};

struct vme_if_info {
        int addr_select;
        int interrupter_type;
};
#endif /* _MACHINE_VME2_INCLUDED */
