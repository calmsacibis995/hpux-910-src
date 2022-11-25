/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/simon.h,v $
 * $Revision: 1.4.84.4 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 14:06:06 $
 */
/* @(#) $Revision: 1.4.84.4 $ */    
#ifndef _SYS_SIMON_INCLUDED /* allows multiple inclusion */
#define _SYS_SIMON_INCLUDED
#ifdef __hp9000s300

/* SIMON (98625) disc interface card */
/* PHI/ABI chip registers are 'P_', Simon only are 'S_' */

#define MA		30

/* high order access bits via the status register in 8-bit mode */
#define P_HIGH_0        0x80
#define P_HIGH_1        0x40

/* interrupt register and mask */
#define P_INT_PEND      P_HIGH_0	/* interrupting condition usage */
#define P_INT_ENAB      P_HIGH_0	/* interrupt mask usage */
#define P_PRTY_ERR      P_HIGH_1
#define P_STAT_CHNG     0x80
#define P_PROC_ABRT     0x40
#define P_POLL_RESP     0x20
#define P_SERV_RQST     0x10
#define P_FIFO_ROOM     0x08
#define P_FIFO_BYTE     0x04
#define P_FIFO_IDLE     0x02
#define P_DEV_CLR       0x01

/* outbound fifo */
#define P_FIFO_EOI      P_HIGH_0
#define P_FIFO_ATN      P_HIGH_1
#define P_FIFO_LF_INH   P_HIGH_0
#define P_FIFO_UCT_XFR  (P_HIGH_0|P_HIGH_1)

/* status */
#define P_REM           0x20
#define P_HPIB_CTRL     0x10
#define P_SYST_CTRL     0x08
#define P_TLK_IDF       0x04
#define P_LTN           0x02
#define P_DATA_FRZ      0x01

/* control */
#define P_POLL_HDLF     P_HIGH_0	/* ABI only */
#define P_DHSK_DLY      P_HIGH_1	/* ABI only */
#define P_8BIT_PROC     0x80
#define P_PRTY_FRZ      0x40
#define P_REN           0x20
#define P_IFC           0x10
#define P_RSPD_POLL     0x08
#define P_RQST_SRVC     0x04
#define P_FIFO_SEL      0x02
#define P_INIT_FIFO     0x01

/* HP-IB control */
#define P_CRCE          P_HIGH_0	/* ABI only */
#define P_EPAR          P_HIGH_1	/* ABI only */
#define P_ONL           0x80
#define P_TA            0x40
#define P_LA            0x20
#define P_HPIB_ADR_MASK 0x1f

/* simon control bits */
#define S_ENAB		0x80
#define S_PEND		0x40
#define S_LEVMSK	0x30
#define S_LEVSHF	4
#define S_WRIT		0x08
#define S_WORD		0x04
#define S_DMA_1		0x02
#define S_DMA_0		0x01

/* simple simon control 2 bits */
#define S2_DISABLE_DONE	0x01

/* simple simon status bit */
#define S_HALFWORD	0x01

/* simon card structure */

struct simon {
	unsigned char
		sf0,	sim_reset,
		sf2,	sim_stat,
		sf4,	sim_ctrl2,
		sf6,	sim_latch,
		sf8,	sf9,
		sf10,	sf11,
		sf12,	sf13,
		sf14,	sf15,
		sf16,	phi_intr,
		sf18,	phi_imsk,
		sf20,	phi_fifo,
		sf22,	phi_status,
		sf24,	phi_ctrl,
		sf26,	phi_address,
		sf28,	phi_ppmsk,
		sf30,	phi_ppsns;
};

#define sim_id sim_reset
#define sim_ctrl sim_stat
#define phi_fstid phi_ppmsk
#define phi_secid phi_ppsns

/* simon defines for "Hidden Mode" registers */

#define sim_switches sim_id

#define S_INT_LEVEL             0x30
#define S_SYSTEM_CTLR           0x08
#define S_SPEED                 0x04
#define S_ENABLE_SIMONC         0x02
#define S_ENABLE_DMA32          0x01

#endif /* __hp9000s300 */
#endif /* _SYS_SIMON_INCLUDED */
