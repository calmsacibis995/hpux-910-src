/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/hpib.h,v $
 * $Revision: 1.3.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:29:17 $
 */
/* @(#) $Revision: 1.3.83.3 $ */     
#ifndef _SYS_HPIB_INCLUDED /* allows multiple inclusion */
#define _SYS_HPIB_INCLUDED
/*
**	hpib.h	Device I/O Library header for HPIB
*/

/*
**	HPIB_CONTROL commands
*/
#define	HPIB_EOI		1
#define	HPIB_SIGNAL_MASK	2
#define	HPIB_LOCK		3
#define	HPIB_ADDRESS		4
#define	HPIB_RESET		5
#define	HPIB_PPOLL_RESP		6
#define	HPIB_PPOLL_IST		7
#define	HPIB_REN		8
#define	HPIB_ATN		9
#define	HPIB_SRQ		10
#define	HPIB_PASS_CONTROL	11
#define	HPIB_GET_CONTROL	12

/*
**	HPIB_STATUS commands
*/
#define	HPIB_PPOLL		13
#define	HPIB_SPOLL		14
#define	HPIB_BUS_STATUS		15
#define	HPIB_WAIT_ON_PPOLL	16
#define	HPIB_WAIT_ON_STATUS	17
#define	HPIB_TERM_REASON	18

/*
**	GPIO_CONTROL commands
*/
#define	GPIO_EIR_CONTROL	 1

/* constants for parity and address control */
#define	HPIB_PARITY		19
#define	HPIB_SET_ADDR		20

/* constants for HPIB_BUS_STATUS */
#define	STATE_NDAC		0x01
#define	STATE_SRQ		0x02
#define	STATE_REN		0x04
#define	STATE_ACTIVE_CTLR	0x08
#define	STATE_SYSTEM_CTLR	0x10
#define	STATE_TALK		0x20
#define	STATE_LISTEN		0x40
#define	STATE_MY_ADDRESS	0x80
#define	STATUS_BITS		0xff

#define	STATUS_ADDR		8	/* shift this much to return address */

/* types of opens */
#define	HPIB_CHAN		1
#define	HPIB_DEVICE		2

/* reset functions */
#define	DEVICE_CLR		1
#define	BUS_CLR			2
#define	HW_CLR			3

/* HPIB_PPOLL info */
#define	CLEAR_PPOLL		0x10

/* hpibio structure */
/*
struct iodetail {
	char mode;
	char terminator;
	int count;
	char *buf;
};	*/
#define	HPIWRITE	0x00
#define	HPIBREAD	0x01
#define	HPIBATN		0x02
#define	HPIBEOI		0x04
#define	HPIBCHAR	0x08
#endif /* _SYS_HPIB_INCLUDED */
