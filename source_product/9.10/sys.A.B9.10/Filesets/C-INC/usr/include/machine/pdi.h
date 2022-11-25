/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/pdi.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:16:35 $
 */
/* @(#) $Revision: 1.4.84.3 $ */      
#ifndef _MACHINE_PDI_INCLUDED /* allows multiple inclusion */
#define _MACHINE_PDI_INCLUDED
#ifdef __hp9000s300

#ifdef _KERNEL_BUILD
#include "../machine/timeout.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/timeout.h>
#endif /* _KERNEL_BUILD */

#define OUTLINE	0x80			/* set if priveleged line */

/* register definitions */

#define	INTR_REG		4
#define	INTR_COND_REG		121
#define	MODEM_INTCOND_REG	13
#define	MODEM_INTMASK_REG	15
#define	MODEM_REG_7		7
#define	MODEM_REG_8		8
#define	BREAK_REG		6
#define	SPACE_AVAIL_REG		11
#define	CBLOCK_MASK_REG		14 
#define	BAUD_REG		20
#define	HANDSHAKE_REG		22
#define	PROTOCOL_REG		24
#define	INBOUND_MASK_REG	28
#define	CHAR_SIZE_REG		34
#define	STOP_BIT_REG		35
#define	PARITY_REG		36 
#define	CLEAR_REG		101 

/* control blocks for break, parity and framing errors */
#define REPORT_ERRORS_BREAK	0x0c
#define RCV_ERROR	0x08
#define FRMERR		0x08
#define PARERR		0x04
#define BRKBIT		0x10
#define RXINTEN 	0x02
#define TXINTEN 	0x04
#define MODEM_INTEN	0x08
#define PENABLE 	0x08
#define EPAR		0x10
#define TWOSB		0x04
#define SETBRK		0x40

#define	TRANS_ERR	0x0300		/* for control block reporting */
#define	BREAK		0x0200
#define	REPORT_MASK	0x0300
#define	CHAR_SIZE_MASK	0x03
#define	SET_RTS		0x01
#define	SET_DTR		0x02
#define	PARITY_NONE	0x00
#define	PARITY_ODD	0x01
#define	PARITY_EVEN	0x02
#define	DCD		0x02
#define	DATA_AVAILABLE	0x02
#define	WRITE_AVAILABLE	0x04
#define	MODEM_STATUS	0x08
#define	ONE_STOP_BIT	0x00
#define	TWO_STOP_BITS	0x02
#define	D1D3		3

#define ON	1
#define OFF	0


struct pdi_desc_type {
	int card_address;
	short which_RXbuf;
	short term_and_mode;
	struct sw_intloc intlock;
};

/* 
 * 15 May 92	jlau
 * We needed to add a new field (i.e. open_count for successful open)
 * in the hardware structure which tp->utility points to.  However,
 * we chose not to add it to the hardware structure (i.e. struct 
 * pdi_desc_type above) directly because the pdis.s assembly code uses 
 * hardcoded offset to access the fields on pdi_desc_type structure.  
 * In order to avoid modifying assembly code, we decided to add the open
 * count field on a separte structure which contains a pointer tothe 
 * pdi_desc_type structure.  Hence, we need to modify the PASS_PDI to 
 * reflect the extra level of indirection.
 *
 * WARNING: the common code (i.e. ttycomn.c) shares by the the pdi.c
 *	    the pci.c, the apollo_pci.c and the mux.c drivers assumes
 *	    that the tp->utility points to the open_count. Therefore,
 *	    do NOT move the location of the open_count when new fields
 *	    need to be added to the HW data structure.
 */
struct pdi_hw_data {
	u_int				open_count;  /* this field must be */
						     /* the 1st one in struct */
	u_int				vhangup;
	struct pdi_desc_type	*	pdi_desc_ptr;	
};

/* Updated the PASS_PDI macro to reflect the above change */
#define PASS_PDI ((struct pdi_hw_data *)(tp->utility))->pdi_desc_ptr

#endif /* __hp9000s300 */
#endif /* _MACHINE_PDI_INCLUDED */
