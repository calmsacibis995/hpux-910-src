/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ti9914.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:19:11 $
 */
/* @(#) $Revision: 1.4.84.3 $ */      
#ifndef _SYS_TI9914_INCLUDED /* allows multiple inclusion */
#define _SYS_TI9914_INCLUDED
#ifdef __hp9000s300
/*
**  Structure defining the registers on the 98624 HPIB card
**  with the TI9914 GPIB controller chip
*/

struct TI9914 {

        /* EXTERNAL REGISTERS (CARD REGISTERS) */
        
	unsigned char pad1;        /* CARD ID REGISTER (READ ONLY) --         */
	unsigned char card_id;     /* NOT IMPLEMENTED ON THE INTERNAL HPIB    */

	unsigned char pad2;
	unsigned char iostatus;    /* I/O STATUS (READ) - I/O CONTROL (WRITE) */

	unsigned char pad3;
	unsigned char extstatus;   /* EXTERNAL STATUS REGISTER (READ ONLY)    */

	unsigned char pad5;
	unsigned char ppoll_int;   /* bobcat internal only                    */

	unsigned char pad7;
	unsigned char ppoll_mask;  /* bobcat internal only                    */

	unsigned short pad8a;      /* REGISTERS 7 - 15 ARE NOT IMPLEMENTED    */
	unsigned short pad8b;
	unsigned short pad8c;
        
        /* TI9914 REGISTERS */
        
	unsigned char pad9;        /* INTERRUPT STATUS 0 (READ) --            */
	unsigned char intstat0;    /* INTERRUPT MASK 0 (WRITE)                */
        
	unsigned char pad10;       /* INTERRUPT STATUS 1 (READ) --            */
	unsigned char intstat1;    /* INTERRUPT MASK 1 (WRITE)                */
        
	unsigned char pad11;
	unsigned char addrstate;   /* ADDRESS STATE (READ ONLY)               */
        
	unsigned char pad12;       /* BUS STATUS (READ) --                    */
	unsigned char busstat;     /* AUXILIARY COMMAND REGISTER (WRITE)      */
        
	unsigned char pad13;
	unsigned char address_reg; /* ADDRESS REGISTER (WRITE ONLY)           */
        
	unsigned char pad14;
	unsigned char spoll;       /* SERIAL POLL REGISTER (WRITE ONLY)       */
        
	unsigned char pad15;       /* COMMAND PASS THRU (READ) --             */
	unsigned char cmdpass;     /* PARALLEL POLL REGISTER (WRITE)          */
        
	unsigned char pad16;
	unsigned char datain;      /* DATA IN (READ) -- DATA OUT (WRITE)      */
};
#define iocontrol	iostatus
#define intmask0	intstat0
#define intmask1	intstat1
#define auxcmd		busstat
#define ppoll		cmdpass
#define dataout		datain

/* TI9914 AUXILIARY COMMANDS */

#define H_SWRST1        0x80           /* enable software reset */
#define H_SWRST0        0x00           /* disable software reset */

#define H_DACR1           0x01         /* release data accepted holdoff (DAC) */
#define H_DACR0           0x81         /* set data accepted holdoff (DAC) */

#define H_RHDF          0x02           /* release ready for data (RFD) holdoff */

#define H_HDFA1         0x83            /* set hold off on all data */
#define H_HDFA0         0x03            /* release hold off on all data */

#define H_HDFE1         0x84            /* set hold off on EOI only */
#define H_HDFE0         0x04            /* release hold off on EOI only */

#define	H_NBAF		0x05		/* set new byte available */

#define	H_FGET1		0x86		/* set force group execute trigger */
#define	H_FGET0		0x06		/* release group execute trigger */

#define	H_RTL1		0x87		/* set return to local */
#define	H_RTL0		0x07		/* release return to local */

#define	H_FEOI		0x08		/* pulse EOI */

#define	H_LON1		0x89		/* set listen only */
#define	H_LON0		0x09		/* clear listen only */

#define	H_TON1		0x8A		/* set talk only */
#define	H_TON0		0x0A		/* clear talk only */

#define	H_GTS		0x0B		/* clear atn line */
#define	H_TCA		0x0C		/* take control async */
#define H_TCS		0x0D		/* take control sync */

#define	H_RPP1		0x8E		/* set ppoll */
#define	H_RPP0		0x0E		/* clear ppoll */

#define	H_SIC1		0x8F		/* set IFC true */
#define	H_SIC0		0x0F		/* set IFC false */

#define	H_SRE1		0x90		/* set REN true */
#define	H_SRE0		0x10		/* set REN false */

#define	H_RQC		0x11		/* set request control */
#define	H_RLC		0x12		/* set release control */

#define H_DAI1		0x93		/* disable all interrupts */
#define H_DAI0		0x13		/* enable all interrupts */

#define	H_PTS		0x14		/* set pass through next secondary */

#define	H_STDL1		0x95		/* set T1 delay to 1200ns */
#define	H_STDL0		0x15		/* release T1 delay */

#define	H_SHDW1		0x96		/* set shadow handshake */
#define	H_SHDW0		0x16		/* clear shadow handshake */

#define	H_VSTDL1	0x97		/* set T1 delay to 600ns (9914A) */
#define H_VSTDL0	0x17		/* clear T1 delay */


/* TI9914 register masks */

/* interrupt mask register */
#define	TI_M_GET	0x8000
#define	TI_M_ERR	0x4000
#define	TI_M_UCG	0x2000		/* enable Unrecoginized Command */
#define	TI_M_APT	0x1000
#define	TI_M_DCAS	0x0800		/* enable on Device Clear Active State*/
#define	TI_M_MA		0x0400		/* enable on My Address */
#define	TI_M_SRQ	0x0200		/* enable SRQ */
#define	TI_M_IFC	0x0100		/* enable Interface Clear */

#define TI_M_INT0	0x0080
#define TI_M_INT1	0x0040
#define TI_M_BI		0x0020		/* enable on byte in */
#define TI_M_BO		0x0010		/* enable on byte out */
#define TI_M_END	0x0008		/* enable on EOI */
#define TI_M_SPAS	0x0004		/* enable on request service */
#define TI_M_REN       	0x0002		/* enable on remote/local change */
#define TI_M_MAC       	0x0001		/* enable on my address change */

/* interrupt status register for TI9914_ruptmask */
#define	TI_S_INT0	0x8000		/* interrupt in status 0 */
#define	TI_S_INT1	0x4000		/* interrupt in status 1 */
#define	TI_S_BI		0x2000		/* byte in interrupt */
#define	TI_S_BO		0x1000		/* byte out interrupt */
#define	TI_S_END	0x0800		/* EOI interrupt */
#define	TI_S_SPAS	0x0400
#define	TI_S_RLC	0x0200
#define	TI_S_MAC	0x0100

#define	TI_S_GET	0x0080
#define	TI_S_ERR	0x0040
#define	TI_S_UCG	0x0020		/* UCG interrupt */
#define	TI_S_APT	0x0010
#define	TI_S_DCAS	0x0008		/* DCAS interrupt */
#define	TI_S_MA		0x0004		/* MA interrupt */
#define	TI_S_SRQ	0x0002		/* SRQ interrupt */
#define	TI_S_IFC	0x0001		/* IFC interrupt */



/* address status register */
#define	TI_A_REN	0x80
#define	TI_A_LLO	0x40
#define	TI_A_ATN	0x20		/* ATN is set */
#define	TI_A_LPAS	0x10
#define	TI_A_TPAS	0x08
#define	TI_A_LADS	0x04		/* addressed to listen */
#define	TI_A_TADS	0x02		/* addressed to talk */
#define	TI_A_ULPA	0x01

/* bus status register */
#define	TI_B_ATN	0x80
#define	TI_B_DAV	0x40
#define	TI_B_NDAC	0x20		/* NDAC asserted */
#define	TI_B_NRFD	0x10
#define	TI_B_EOI	0x08
#define	TI_B_SRQ	0x04		/* SRQ asserted */
#define	TI_B_IFC	0x02
#define	TI_B_REN	0x01		/* REN asserted */

/* external status register */
#define	TI_E_SYS	0x80		/* system controller */

/* card enable register */
#define	TI_DMA_0	0x01		/* enable with dma chan 0 */
#define	TI_DMA_1	0x02		/* enable with dma chan 1 */
#define	TI_ENI		0x80		/* enable interrupts */

#define TI9914_IMASK    (TI_S_UCG)	/* always enable UCG */

#define HOLDOFF		0x10		/* Holdoff on all */

#define	TOTAL_TI9914	5		/* number of ti9914's allowed */

#define	LINEFEED	0x0a		/* termination character */
#define INT_ON		1		/* turn interrupt on */
#define INT_OFF		0		/* turn it off */
#endif /* __hp9000s300 */
#endif /* _SYS_TI9914_INCLUDED */
