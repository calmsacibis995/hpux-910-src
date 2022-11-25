/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/dma.h,v $
 * $Revision: 1.4.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 18:59:33 $
 */
/* @(#) $Revision: 1.4.84.4 $ */      
#ifndef _MACHINE_DMA_INCLUDED /* allows multiple inclusion */
#define _MACHINE_DMA_INCLUDED

#ifdef __hp9000s300
#ifdef _KERNEL_BUILD
#include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#endif /* _KERNEL_BUILD */

/*
** structure definitions for the dma channels
** both logical and physical
*/

/* physical addresses for dma channels 0 and 1 */

#define	DMA_CHANNEL0	(0x500000 + LOG_IO_OFFSET)
#define	DMA_CHANNEL1	(0x500008 + LOG_IO_OFFSET)
#define	DMA32_CONTROL	(0x500014 + LOG_IO_OFFSET)
#define	DMA32_CHANNEL0	(0x500100 + LOG_IO_OFFSET)
#define	DMA32_CHANNEL1	(0x500200 + LOG_IO_OFFSET)
#define PROGRAMABLE_DMA	(0x508000 + LOG_IO_OFFSET) /* to see if we have new dma */


/* physical dma channel definition */

struct dma {
	caddr_t dma_address;
	unsigned short dma_count;
	unsigned char pad1;
	unsigned char dma_control;
};

struct dma32 {
	caddr_t dma32_address;
	unsigned long dma32_count;
	unsigned short dma32_control;
	unsigned short dma32_status;
};

#define	dma_reset dma_address
#define dma_status dma_control

/* bit definitions for dma channels */

/* interrupt level settings */
#define	DMA_LVL3	0x0000
#define	DMA_LVL4	0x0010
#define	DMA_LVL5	0x0020
#define	DMA_LVL6	0x0030
#define	DMA_LVL7	0x0040

/* control register bits */
#define	DMA_PRI		0x0008
#define	DMA_IO		0x0004
#define	DMA_WB		0x0002
#define	DMA_ENABLE	0x0001

/* Additional defines for 98620C control register */
#define	DMA32_START	0x8000
#define	DMA32_LWORD	0x0100
/* Defines for 98620C General Control/Status registers */
#define	DMA32_RESET0	0x0010
#define	DMA32_RESET1	0x0020

/* status register bits */
#define	DMA_INT		0x0002
#define	DMA_ARM		0x0001

/*
** logical dma channel definition
*/
struct dma_channel {
	struct buf *b_actf;		/* for queue of requests */
	struct buf *b_actl;		/* for queue of requests */
	struct dma *card;		/* address of dma channel */
	int reserved;			/* bit map of reserving sc's */
	short locked;			/* locked by a single sc */
	struct dma_chain *base;		/* start of chain array */
	struct dma_chain *extent;	/* end of chain array */
	struct isc_table_type *sc;	/* interface owning channel */
	int (*isr)();			/* current dma isr address */
	int arg;			/* argument to isr */
	struct dma32 *card32;		/* alternate addr of dma C channel */
};


/* 
** Each entry in the chain has the following format:
**      bytes   0-3:	address of io card register
**	byte      4:	level on which interrupt is supposed to occur
**	byte      5:	io card byte (used to modify io card state)
**	bytes   6-9:	address of transfer buffer
**      bytes 10-11:	transfer count
**	bytes 12-13:	dma channel arm word
*/
struct dma_chain {
	caddr_t	       card_reg;
	unsigned char  level;
	unsigned char  card_byte;
	caddr_t	       buffer;
	unsigned short count;
	unsigned short arm;
};


/*
** Defines for the number of physical page dma transfers (max)
** and the number of bytes (max)
*/
#define	DMA_BYTES	65536
#define	DMA_PAGES	(DMA_BYTES / NBPG)
#define DMACHANSZ 	(sizeof(struct dma_chain) * (DMA_PAGES+3))

#ifdef _KERNEL
/*
** booleans for 32bit dma usage
*/
int dma32_chan0, dma32_chan1;

/*
** boolean for dma installed
*/
int dma_here;

/*
** dma allocation state info
*/
int free_dma_channels;	/* (channels)-(active sc's); can go negative */

/* pre allocated dma chain arrays -- one per channel */
/* hpibio.s has a dependency on this structure! */

struct dma_chain *dmachain[2];

struct dma_channel dmatab[2];

/* global count of all bus masters in the system */
int bus_master_count;
#endif /* KERNEL */

#define DMA_LVL_MASK		0x70
#define DMA_CHAN_MASK		(DMA_LVL_MASK | DMA_ENABLE)
#define dmachan_arm_level(arg)	(((arg-3)<<4) | DMA_ENABLE)

#endif /* __hp9000s300 */
#endif /* _MACHINE_DMA_INCLUDED */
