/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/drvhw.c,v $
 * $Revision: 1.11.84.6 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/06 14:06:48 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 
#if  defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) drvhw.c $Revision: 1.11.84.6 $ $Date: 93/12/06 14:06:48 $";
#endif


/*---------------------------------------------------------------------*
 |								       |
 |	  d		       h				       |
 |	  d		       h				       |
 |     dddd   r rr    v	  v    h hh    w   w	       ccc             |
 |    d	  d   rr  r   v	  v    hh  h   w w w	      c	               |
 |    d	  d   r	      v	  v    h   h   w w w	      c	               |
 |    d	  d   r	       v v     h   h   w w w	..    c	               |
 |     dddd   r		v      h   h    w w	..     ccc             |
 |							               |
 |								       |
 *---------------------------------------------------------------------*/

/************************** drvhw.c *************************************
 *	
 *	AUTHOR:		  Jeff Wu (Leafnode code)
 *      Modified for Peking: Prabha Chadayammuri, 
 *                           Peter Notess, Dave Gutierrez.	
 *
 *	MODULE:		  s200io 
 *	PROJECT:	  peking 
 *	REVISION:	  6.0
 *	SCCS NAME:	  /source/sys/sccs/s200io/s.drvhw.c
 *	LAST DELTA DATE:  
 *
 *	DESCRIPTION:
 *		This is the code file for the hardware model
 *		implementation of the driver for the series 200 Unix.
 *
 ************************************************************************/
#ifndef TEMP_4.3_KLUDGE /* temprarily putting this stuff in --
                                notice if_n_def !  (skumar 7/27)*/
#define      NS_ASSERT(cond, string) if(!(cond)) panic((string))

#endif  TEMP_4.3_KLUDGE

/* #ifdef DUX */
#include "../h/param.h"  /* needed by timeout.h		      */
#include "../h/buf.h"
/* #else */
/* #include "../h/types.h" */ /* needed by timeout.h		      */
/* #endif DUX */
#include "../h/time.h"
#include "../h/kernel.h"
#include "../wsio/timeout.h"	/* for sw_intloc needed by drv_hw.h   */
#include "../wsio/intrpt.h"	/* has definition of struct interrupt */

#include "../h/socket.h"
#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_ieee.h"
#include "../net/route.h"
#include "../net/raw_cb.h"

/* For tracing in 8.0 */
#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

#include "../s200io/lnatypes.h" /* leaf node decls for driver...prabha */
#include "../h/mbuf.h"
#include "../s200io/time_stamp.h" /* time stamp defs. */
#include "../s200io/drvhw.h" /* added for 4.2.....prabha */
#include "../sio/lanc.h" /* added for 4.2....prabha */
#include "../s200io/drvhw_ift.h"
#include "../h/errno.h"
#include "../s200io/drvmac.h"
#include "../wsio/hpibio.h"	/* definition of isc_table */
				/* needs to be included after narchdef.h */

/* #ifdef DUX */
#include "../dux/dm.h" 		/*DUX's message interface definitions*/
#include "../dux/duxlaninit.h"
#include "../dux/dux_hooks.h"	/* Needed for DUX/LAN configurability */
extern int dux_nop();		/* Needed for DUX/LAN configurability */
/* #endif DUX */

#define copy_network_data(from,to,len)   iobcopy(from, to, (unsigned)len);


/*---------------------- HARDWARE CONSTANTS -----------------------------
 *	
 *	The following are hardware related constants.
 *	Most of the constants fall into three categories:
 *	    1.	Where to find the data.	 This is usually an offset from
 *		some base address.  These constants have the convention
 *		that they are suffixed by _REG, or _OFF.  The _REG is
 *		used in data structures which are really registers.
 *		The _OFF is used to address structures which are memory
 *		based.	(Registers differ from memory in that some
 *		registers have side effects).
 *	    2.	The field within the address.  The common suffixes here
 *		are _BIT or _FIELD.  _BIT is used to describe one bit
 *		fields.	 _FIELD is used to describe the rest.
 *	    3.	The constants which to write to these addresses and 
 *		fields.	 These do NOT have common suffixes.
 *	
 *	The indentation scheme below have the constants in categories 
 *	(2), (3) immediately following and indented from the constants 
 *	in category (1).
 *
 *---------------------------------------------------------------------*/

/*
 *	Card registers offset from the base address of the card
 */

#define ID_REG		0x0001
#define		ID_FIELD	0x7F
#define		LAN_CARD_ID	0x15		/* 21 decimal */

#define RESET_REG	0x0001

#define INTR_REG	0x0003
#define		INTR_BIT	0x80
#define		INTREQ_BIT	0x40
#define		ACK_BIT		0x04
#define		BABBLE_BIT	0x02
#define		INTR_MASK	INTR_BIT | INTREQ_BIT	/* used for isrlink */
#define		INTR_VALUE	INTR_BIT | INTREQ_BIT	/* used for isrlink */
#define		INTR_ENABLE	INTR_BIT
#define		INTR_DISABLE	0

/*
 *	LANCE registers offset from the base address of the card
 */

#define LANCERDP_REG	0x4000

#define LANCERAP_REG	0x4002
#define		ERROR_BIT	0x8000		/* CSR0 bits */
#define		BABL_BIT	0x4000
#define		CERR_BIT	0x2000
#define		MISS_BIT	0x1000
#define		MERR_BIT	0x0800
#define		RINT_BIT	0x0400
#define		TINT_BIT	0x0200
#define		IDON_BIT	0x0100
#define		INEA_BIT	0x0040
#define		RXON_BIT	0x0020
#define		TXON_BIT	0x0010
#define		TDMD_BIT	0x0008
#define		STOP_BIT	0x0004
#define		STRT_BIT	0x0002
#define		INIT_BIT	0x0001
#define		CSR0_INIT		0x7F43	/* constants made up from    */
#define		CSR0_INTR_ACK_BITS	0x7F40	/* the bit definitions above */
#define		CSR0_START		0x7F42	/* re-start constant */
#define		CSR0_IDON_ACK		0x0140
#define		CSR3_INIT		0x0004

/*
 *	Receive ring descriptors
 */

#define RING_SIZE	8			/* 4 words long */

#define RMD1_OFF	2			/* byte offset from RMD0 */
#define		OWN_BIT		0x8000		/* RMD1 bits (in word) */
#define		ERR_BIT		0x4000
#define		FRAM_BIT	0x2000
#define		OFLO_BIT	0x1000
#define		CRC_BIT		0x0800
#define		RXBUFF_BIT	0x0400
#define		STP_BIT		0x0200
#define		ENP_BIT		0x0100
#define		STAT_FIELD	0xFF00		/* only the statistics	 */

#define		MY_OWN		0		/* owned by driver	 */

#define		RMD1_RETURN	0x80		/* stat to write when	 */
						/* returning ownership to*/ 
						/* the LANCE. THIS IS A	 */
						/* BYTE CONSTANT	 */

#define RMD2_OFF	4			/* byte offset from RMD0 */

#define RMD3_OFF	6			/* byte offset from RMD0 */
#define		MCOUNT_FIELD	0x0FFF		/* message byte count	 */
#define		RMD3_RETURN	0		/* clears count		 */

/*
 *	Transmit ring descriptors
 */

#define TMD1_OFF	2			/* byte offset from TMD0 */
#define		MORE_BIT	0x1000		/* TMD1 bits		 */
#define		ONE_BIT		0x0800
#define		DEF_BIT		0x0400
	/*
	 *	OWN_BIT				these are defined
	 *	ERR_BIT				in RMD1 above
	 *	STP_BIT
	 *	ENP_BIT
	 */

#define		TMD1_RETURN	0x83		/* stat to write when	   */
						/* returning ownership to  */
						/* the LANCE (when	   */
						/* transmitting)	   */
						/* THIS IS A BYTE CONSTANT */

#define		TMD1_INIT	0x00		/* set to my owner. This   */
						/* field will be set	   */
						/* properly when sending.  */
						/* This is a BYTE CONSTANT */

#define TMD2_OFF	4			/* byte offset from TMD0   */

#define TMD3_OFF	6			/* byte offset from TMD0   */
#define		TXBUFF_BIT	0x8000		/* TMD3 bits		   */
#define		UFLO_BIT	0x4000
#define		LCOL_BIT	0x1000
#define		LCAR_BIT	0x0800
#define		RTRY_BIT	0x0400
#define		TDR_FIELD	0x03FF

#define		TMD3_RETURN	0		/* value to write to TMD3  */
						/* when returning ownership*/
						/* to the LANCE (when	   */
						/* transmitting		   */

/*
 *  Init block structure
 */

#define MODE_OFF	0
#define		MODE_INIT	0
#define		PROM_MODE_INIT	0x8000

#define PADR_OFF	2

#define LADRF0_OFF	8
#define		LADRF0_INIT	0xFFFF

#define LADRF1_OFF	10
#define		LADRF1_INIT	0xFFFF

#define LADRF2_OFF	12
#define		LADRF2_INIT	0xFFFF

#define LADRF3_OFF	14
#define		LADRF3_INIT	0xFFFF

#define RDRA_OFF	16
#define RLEN_OFF	18
#define		RLEN_INIT	5 << 5		/* related to RXRING_NUM*/
						/* see LANCE specs for	*/
						/* more information	*/

#define TDRA_OFF	20
#define TLEN_OFF	22
#define		TLEN_INIT	4 << 5		/* related to TXRING_NUM*/
						/* see LANCE specs for	*/
						/* more information	*/

/*
 *  Non-volatile Memory related constants
 */

#define NOVRAM_OFF	   0xC001		/* byte address		*/
#define		BANK_BIT     0x01
#define HIGH_BANK_OFF	       64		/* offset from NOVRAM	*/
						/* starting addres	*/
#define		NOVBYTE_SIZE	4		/* number to increment	*/
						/* after reading two	*/
						/* nibbles		*/
#define		ADDRESS_SIZE	6		/* link level address	*/
						/* size			*/

/*
 *  The following defines the memory map from constants
 */

#define MEMORY_BASE	0x8000			/* offset from card addr*/
#define MEMORY_SIZE	0x4000			/* total memory size	*/

#define RXBUFFER_SIZE	304
#define RXRING_NUM	32			/* related to RLEN_OFF	*/
#define TXRING_NUM	16			/* related to TLEN_OFF	*/

/*
 *  The following _BASE are offset from the MEMORY_BASE
 */

#define INIT_SIZE	24

#define RXRING_BASE	INIT_SIZE
#define RXRING_SIZE	(RXRING_NUM*RING_SIZE)

#define TXRING_BASE	(RXRING_BASE+RXRING_SIZE)
#define TXRING_SIZE	(TXRING_NUM*RING_SIZE)

#define RX_BASE		(TXRING_BASE+TXRING_SIZE)
#define RX_SIZE		(RXRING_NUM*RXBUFFER_SIZE)

#define TX_BASE		(RX_BASE+RX_SIZE)
#define TX_SIZE		(MEMORY_SIZE-TX_BASE)

/*
 *  CRC_SIZE: CRC is appended to every packet received, need to subtract
 *	      this constant
 */

#define CRC_SIZE	4

/*
 *  interrupt levels for hw_isr
 */

#define LAN_INTR_LEVEL		5		/* hardware interrupt level */

/*
 *  timeout counts
 */

#define ACK_TIMEOUT_COUNT	1000		/* this is just a count */
#define INIT_TIMEOUT		30		/* this is in millisecs */
#define XMIT_TIMEOUT		300		/* this is in 1/60sec	*/


/*---------------------- MACRO DEFINITIONS ------------------------------
 *	
 *	The following are macros used by the code.
 *	
 *---------------------------------------------------------------------*/

/*
 *  Access the unsigned byte at 'addr.'	 This can be used to read or write.
 */

#define CARDBYTE( addr )	*( (unsbyte *) (addr) )

/*
 *  Access the unsigned word at 'addr.'	 This can be used to read or write.
 *  This macro assumes that 'addr' is at a word boundary.
 */

#define CARDWORD( addr )	*( (unsword *) (addr) )

/*
 *  Access the SIGNED word at 'addr.'  This can be used to read or write.
 *  This macro assumes that 'addr' is at a word boundary.
 */

#define CARDSWORD( addr )	*( (word *) (addr) )

/*
 *  Read the buffer address from a ring descriptor pointed to by 'addr'
 *  The 24 bit address is in the AMD LANCE format:
 *
 *		     even	    odd
 *		+-------------+-------------+
 *	addr	| middle byte |	 low byte   |  addr+1
 *		+-------------+-------------+
 *	addr+2	|	      |	 high byte  |  addr+3
 *		+-------------+-------------+
 *
 *  How it works:
 *	1.  read the higher order byte, and shift it.
 *	2.  read the lower order word.
 *	3.  add it and then type cast it.
 */

#define READADDR( addr )        ( (unsbyte *)                       \
                                ( (*((unsbyte *) (addr) + 3) << 16) \
                                + *((unsword *) (addr))             \
                                + LOGICAL_IO_BASE))

/*
 *  Write the buffer address 'info' to the ring descriptor pointed to by
 *  'addr.'  (See above READADDR for description).
 *  Note that 'info' should not have any side effects.
 *
 */

#define WRITEADDR( addr, info ) { *((unsword *) (addr)) = \
				      (unsdword) (info) & 0x0000FFFF; \
				  *((unsbyte *) (addr) + 3) = \
				      ((unsdword) (info) >> 16) & 0x00FF; }

/*
 *  Copy an address.  The parameters 'from' and 'to' should be of type unsbyte *. 
 *  'src' and 'dest' are used because 'from' and 'to' might be expressions.
 */

#define COPY_SW_ADDRESS( from, to ) {register unsbyte * src = (unsbyte *) (from);\
				     register unsbyte * dest = (unsbyte *) (to); \
				     dest[0] = src[1]; dest[1] = src[0]; \
				     dest[2] = src[3]; dest[3] = src[2]; \
				     dest[4] = src[5]; dest[5] = src[4]; }

/*
 *  Read a byte from the non-volatile memory.  
 *  It reads two consequetive nibbles starting at address 'addr'
 *  Note:  The nibbles are on odd boundaries so they are 2 bytes apart.
 */

#define NOVBYTE( addr ) ((( CARDBYTE( addr ) & 0x0F ) << 4) |\
			  ( CARDBYTE( (byteptr) (addr) + 2 ) & 0x0F ))

/*
 *  Protect/unprotect accesses by any 'processes'
 */

#if NEED_TIMER		/* disable all interrupts */

typedef int all_token;

#define ALL_PROTECT( hwvars, token )	*(token) = spl6()
#define ALL_UNPROTECT( hwvars, token )	splx( token )

#else			/* disable card interrupts */

typedef hw_token all_token;

#define ALL_PROTECT( hwvars, token )	{ \
    *(token) = CARDBYTE( (hwvars)->card_base + INTR_REG );	\
    CARDBYTE( (hwvars)->card_base + INTR_REG ) = INTR_DISABLE;	\
}
#define ALL_UNPROTECT( hwvars, token )	\
    CARDBYTE( (hwvars)->card_base + INTR_REG ) = (token) & INTR_BIT;

#endif /* NEED_TIMER */

int hw_find_mcaddr();

/*---------------------- EXTERNAL FUNCTIONS -----------------------------
 *	
 *	The following are functions defined in the OS.
 *	
 *---------------------------------------------------------------------*/

extern drv_save_stats();

extern isrlink();
extern lan0();
extern sw_trigger();
extern get_buffer();

#if NEED_TIMER

extern spl6();
extern splx();
extern timeout();
extern clear_timeout();

#endif /* NEED_TIMER */

#ifdef LAN_MS
/* measurement system variables */
extern  int     ms_lan_read_queue;
extern  int     ms_lan_proto_return;
extern  int     ms_lan_netisr_recv;
extern  int     ms_lan_write_queue;
extern  int     ms_lan_write_complete;
#endif /* LAN_MS */

/*
 *  DTRACE bits -- hardware dependent driver can use 161-170.
 *		   (These have been moved to ndtrace.h)
 *
 *  (167) is used by logstats
 */

#define HW_ADDR1	168		/* set local address to constant */

/*
 *  IFBITOFF -- a macro that probably should go in ndtrace.h
 *		opposite meaning of TESTBIT
 */

/* $IRS$ get_network_time */
/* ***************************************************************************
 *
 *                       global get_network_time
 *
 *    Overview:
 *          This routine retrieves the network time, and is very implementation
 *          dependent.  It either retrieves the time from the system clock or
 *          from a <timer_isr>-maintained global.
 *
 *          The returned time_stamp (<time>) must be in milliseconds!
 *          Wraparound of 2**32 msecs occurs in 49 days.
 *
 *     VARIABLES EXAMINED:
 *                 LOCAL:                          GLOBAL:
 *                                                 system_clock
 *     VARIABLES MODIFIED:
 *                 LOCAL:                          GLOBAL:
 *
 *     THIS FUNCTION CALLS:
 *                 INTERNAL:                       EXTERNAL:
 *                                                 OS dependent systime call
 *     FUNCTION CALLED BY:
 *                 wake_me_in, timer_handler, any other network module
 * **************************************************************************/

void
get_network_time ( net_time )

time_stamp   *net_time;

{
        int     saved_pri;
        struct  timeval temptime;

        saved_pri = spl6();
        temptime.tv_usec = time.tv_usec;
        temptime.tv_sec  = time.tv_sec;
        splx( saved_pri );
        net_time->time = temptime.tv_usec / 1000;
        net_time->time += temptime.tv_sec * 1000;
#ifdef DEBUG    
        printf( "get_network_time. %u\n", net_time->time );
#endif /* DEBUG */
}


/***************** FUNCTION : hw_incr_stat ******************************
 *
 * [OVERVIEW]	 
 *	Sticky increment the specified statistic (stat).
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	stat   -- the statistic to increment 
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	statistics
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	statistics
 *
 ************************************************************************/

void
hw_incr_stat( hwvars, stat )
hw_gloptr	hwvars;
statistics_type stat;

{

/*----------------------------- ALGORITHM -------------------------------
 *	if STATISTICS [ stat ] <> "FFFFFFFF" then
 *	|   statistics[ stat ]++;
 *	end;
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> hw_incr_stat <<<<\n");
#endif /* DEBUG */
    /* Assume that 'stat' is in range */
    if ((stat>=START_STAT_VAL && stat<MAX_STATISTICS))
      if (hwvars->statistics[stat] != 0xFFFFFFFF) 
	  hwvars->statistics[stat]++;

#ifdef DEBUG    
    else {
         printf("hw_incr_stat: E_HW_WRONG_STAT\n");
         return;
    }
#endif /* DEBUG */    
}

/***************** FUNCTION : hw_reset_stat *****************************
 *
 * [OVERVIEW]	 
 *	Reset the specified statistics (stat).
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	stat   -- the statistic to clear.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	statistics
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	<None>.
 *
 ************************************************************************/

error_number
hw_reset_stat (lanc_ift_ptr, stat)
lan_ift         *lanc_ift_ptr;
statistics_type stat;

{

hw_gloptr  hw_vars = &(((landrv_ift *)lanc_ift_ptr)->lan_vars.hwvars);

/*----------------------------- ALGORITHM -------------------------------
 *	if (stat < START_STAT_VAL) or (stat >= MAX_STATISTICS) then
 *	|   return( e_no_such_stat );
 *	end;
 *	STATISTICS[ stat ] := 0;
 *	return( e_no_error );
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> hw_reset_stat <<<<\n");
#endif /* DEBUG */   

    if (stat<START_STAT_VAL || stat >= MAX_STATISTICS) {
	return( E_LLP_NO_SUCH_STAT );
    }

    drv_save_stats (lanc_ift_ptr);

    hw_vars->statistics[stat] = 0;

    return( E_NO_ERROR );
}

/***************** FUNCTION : read_rdp **********************************
 *
 * [OVERVIEW]	 
 *	Read LANCE register data port (and put it in data).
 *	This function returns TRUE if the read is successful.
 *	It returns FALSE and sets card_state if the read is unsuccessful.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *
 * [OUTPUT]
 *	data   -- contains the read register contents
 *	
 * [GLOBAL VARIABLES MODIFIED]
 *	card_state
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_base
 *
 ************************************************************************/

boolean
read_rdp( hwvars, data )
register hw_gloptr hwvars;
	 word	   *data;
{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	save_int -- save current interrupt level
 *	tcount	 -- timeout counter for polled ACK
 *---------------------------------------------------------------------*/

		all_token	save_int;
    register	word		tcount;

/*-------------------------- ALGORITHM ----------------------------------
 *	repeat
 *	|   *data = lance data port
 *	|   if ack bit then
 *	|   |	return(TRUE);
 *	|   end
 *	until timed out
 *	set card to down state
 *	return(FALSE);
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> read_rdp <<<<\n");
#endif /* DEBUG */    

    tcount = ACK_TIMEOUT_COUNT;
    do {


	ALL_PROTECT( hwvars, &save_int );
	    *data = CARDWORD( hwvars->card_base+LANCERDP_REG );
	    if ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) != 0 ) {
		ALL_UNPROTECT( hwvars, save_int );
		HW_QA_INCR_STAT( hwvars, ACK );
		return(TRUE);
	    }
	ALL_UNPROTECT( hwvars, save_int );
	HW_QA_INCR_STAT( hwvars, NO_ACK );
    } while (--tcount > 0);

/*
 *  timed out -- put card in down state
 */

    hwvars->card_state = ACK_ERROR;
    CARDBYTE( hwvars->card_base+RESET_REG ) = 1;
    CARDBYTE( hwvars->card_base+INTR_REG ) = INTR_DISABLE;
    return(FALSE);
}

/***************** FUNCTION : write_rdp *********************************
 *
 * [OVERVIEW]	 
 *	Write LANCE register data port (with data).
 *	This function returns TRUE if the read is successful.
 *	It returns FALSE and sets card_state if the read is unsuccessful.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	data   -- data to be written into the LANCE register
 *	
 * [GLOBAL VARIABLES MODIFIED]
 *	card_state
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_base
 *
 ************************************************************************/

boolean
write_rdp( hwvars, data )
register hw_gloptr hwvars;
	 word	   data;
{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	save_int -- save current interrupt level
 *	tcount	 -- timeout counter for polled ACK
 *---------------------------------------------------------------------*/

		all_token	save_int;
    register	word		tcount;

/*-------------------------- ALGORITHM ----------------------------------
 *	repeat
 *	|   lance data port = data;
 *	|   if ack bit then
 *	|   |	return(TRUE);
 *	|   end
 *	until timed out
 *	set card to down state
 *	return(FALSE);
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> write_rdp <<<<\n");
#endif /* DEBUG */   

    tcount = ACK_TIMEOUT_COUNT;
    do {


	ALL_PROTECT( hwvars, &save_int );
	    CARDWORD( hwvars->card_base+LANCERDP_REG ) = data;
	    if ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) != 0 ) {
		ALL_UNPROTECT( hwvars, save_int );
		HW_QA_INCR_STAT( hwvars, ACK );
		return(TRUE);
	    }
	ALL_UNPROTECT( hwvars, save_int );
	HW_QA_INCR_STAT( hwvars, NO_ACK );
    } while (--tcount > 0);

/*
 *  timed out -- put card in down state
 */

    hwvars->card_state = ACK_ERROR;
    CARDBYTE( hwvars->card_base+RESET_REG ) = 1;
    CARDBYTE( hwvars->card_base+INTR_REG ) = INTR_DISABLE;
    return(FALSE);
}

/***************** FUNCTION : hw_softreset ******************************
 *
 * [OVERVIEW]	 
 *	STOP the lance and RESTART it.	This effectively clears
 *	the card so that it will come out of a "hang" condition.
 *	But the LANCE will keep the current receive and transmit
 *	rings (they don't need to be reinitialized.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	
 * [GLOBAL VARIABLES MODIFIED]
 *	card_state
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_base
 *
 ************************************************************************/

boolean
hw_softreset( hwvars )
register hw_gloptr hwvars;
{
/*-------------------------- LOCAL VARIABLES ----------------------------
 *	save_int -- save current interrupt level
 *---------------------------------------------------------------------*/

		all_token	save_int;

/*-------------------------- ALGORITHM ----------------------------------
 *	STOP the card
 *	Setup CSR3
 *	START the card
 *---------------------------------------------------------------------*/

/*
 *  STOP the card 
 */

    ALL_PROTECT( hwvars, &save_int );
    if (!write_rdp(hwvars, STOP_BIT)) {
	ALL_UNPROTECT( hwvars, save_int );
	return(FALSE);
    }

    CARDWORD( hwvars->card_base + LANCERAP_REG ) = 3;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	     "E_HW_NO_ACK");
    CARDWORD( hwvars->card_base + LANCERDP_REG ) = CSR3_INIT;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	     "E_HW_NO_ACK" );

    CARDWORD( hwvars->card_base + LANCERAP_REG ) = 0;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	     "E_HW_NO_ACK" );
    CARDWORD( hwvars->card_base + LANCERDP_REG ) = CSR0_START;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	     "E_HW_NO_ACK" );

    ALL_UNPROTECT( hwvars, save_int );
    return(TRUE);
}

#if NEED_TIMER
/***************** FUNCTION : xmit_timer ********************************
 *
 * [OVERVIEW]	 
 *	This is the timer routine that is executed if the transmit does
 *	not interrupt within the specified time.  This function will
 *	soft-reset the card and restart the timer.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	<None>.
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_state
 *
 ************************************************************************/

void
xmit_timer( hwvars )
register  hw_gloptr  hwvars;
{

/*-------------------------- ALGORITHM ----------------------------------
 *	do nothing if the card is dead
 *	stop the card
 *	start the card
 *	restart the timer
 *---------------------------------------------------------------------*/

/*
 *  Do nothing if the card is dead
 */

#ifdef DEBUG    
    printf(">>>> xmit_timer <<<<\n");
#endif /* DEBUG */    
    HW_QA_INCR_STAT( hwvars, SW_XMIT_TIMER );

    if (hwvars->card_state != HARDWARE_UP) return;

    /*
     *	Need to reset the card here, this is NOT a STOP-RESET, but
     *	a STOP-INIT.  If the timer is defined, then everything
     *	becomes a critical section.
     */

    timeout( xmit_timer, hwvars, XMIT_TIMEOUT, &hwvars->timeout_loc );

}
#endif	/* NEED_TIMER */


/******************FUNCTION copy_in_data ******************************
 * 
 * [OVERVIEW]
 *     This function copies a 'size' bytes of data from  'src' to the
 *     next free byte location in the mbuf chain pointed to by *m.
 *     It is assumed that all of the mbufs/clusters on the chain have
 *     their m_off fields pointing to the beginning of the associated
 *     data area.  m_len will be adjusted to reflect the
 *     transfer at the end of the copy operation. If all of 'size' bytes
 *     cannot be filled into the first mbuf/cluster, this routine will 
 *     follow (*m)->m_next to the next mbuf in the chain and use
 *     it.
 *
 * [INPUT PARAMETERS]
 *
 *     src       - the source address for the copy operation. The
 *                 source space is assumed to be logically contiguous.
 *
 *     m	 - pointer to the mbuf chain to
 *                 where the data is to be transferred. Changed
 *                 in this procedure.
 *
 *     size      - the size of data to be transferred from source.
 *                 size must be a +ve integer.
 * 
 *  [OUTPUT PARAMETERS]
 *      
 *     no output arguments. The function returns error if the mbuf chain
 *     runs out of space.
 *
 ***************************************************************************/
 
 error_number 
 copy_in_data(src, m, size)
 register anyptr src;
 register struct mbuf **m;
 register unsword size;

 {
   register anyptr    dest;
   register unsword   size1;
   register unsword   avail;


#ifdef DEBUG    
       printf(">>>>>> IN COPY_IN_DATA <<<<<<<<<<\n");
#endif /* DEBUG */    

       while (size > 0)
       { 
	 if ((int)*m == 0)
		return(E_OUT_OF_MEMORY);

	 /* note that the following code assumes that the data starts at
	    the front of the mbuf/cluster */
         avail = ((M_HASCL(*m)) ? (*m)->m_clsize: MLEN) - (*m)->m_len; 
         size1 = (size > avail) ? avail : size; 

         if (size1 > 0) {
           
           dest = mtod(*m,anyptr) + (*m)->m_len;
           copy_network_data(src,dest,size1);
           (*m)->m_len += size1; 
  
            /* if there's more left then fill next buffer until done */

           src  += size1;
           size -= size1;
	 }
         if (size > 0) 
            *m = (*m)->m_next;
        }
        return (E_NO_ERROR); 
 }


/******************FUNCTION cp_mbufs_to_mbufs ******************************
 * 
 * [OVERVIEW]
 *     This function copies 'size' bytes of data from  'src' mbuf_chain
 *     to the 'dst' mbuf chain. The mbufs chains are not assumed to be
 *     contiguous, or to be of the same size.  However, it is assumed that
 *     the m_off fields of all mbufs in the dst chain all point to the
 *     begining of their data areas.
 *
 * [INPUT PARAMETERS]
 *
 *     src       - pointer to the source mbuf chain.
 *
 *     dst       - pointer to the destination mbuf_chain. 
 *     
 *     size      - the size of data to be transferred from source.
 *                 size must be a +ve integer. This parameter is added
 *                 so we can do partial copies if need be.
 * 
 *  [OUTPUT PARAMETERS]
 *      
 *     no output arguments. The function returns error if the destination
 *     mbuf chain runs out of space.
 *
 ***************************************************************************/
 
 error_number 
 cp_mbufs_to_mbufs(src, dst, size)
 register struct mbuf *src;
 struct mbuf *dst;
 register unsword size;

 {
         register struct mbuf *tmp_ptr;
         unsword   size1;
         error_number err = E_NO_ERROR; 
         anyptr src_ptr;


#ifdef DEBUG    
         printf(">>>>> IN CP_MBUFS_TO_MBUFS <<<<<<<<\n");
#endif /* DEBUG */    

         /* set all m_len fields to be zero.  assume that m_off fields
	    all point to start of each data area.*/
	 /* set m_len field of first mbuf to be offset by 4, so there
	    will be room later for addition if ifnet pointer */
         tmp_ptr = dst->m_next;
	 dst->m_len = 4;
         while ((int)tmp_ptr)
         {
            tmp_ptr->m_len = 0;
            tmp_ptr        = tmp_ptr->m_next;
         }
         
         tmp_ptr   = src; 

         while (size > 0) 
         {
           src_ptr   = mtod(tmp_ptr,anyptr);
           size1     = tmp_ptr->m_len;
           if ((err = copy_in_data(src_ptr, &dst, size1)) != E_NO_ERROR)
	       break;
           size     -= size1;
           if (size > 0)
	      {
	       tmp_ptr = tmp_ptr->m_next;
	      }
	  }

	 return (err); 
 }


/******************FUNCTION copy_out_data ******************************
 * 
 * [OVERVIEW]
 *     This function copies 'size' bytes of data from the mbufs chain
 *     pointed  to by the src_ptr to 'dest'; The mbuf chain is followed
 *     until the required data is obtained or it runs out of data. If the
 *     latter happens, an error is returned.
 *
 * [INPUT PARAMETERS]
 *
 *     src_ptr   - pointer to the mbuf chain
 *                 where the data is to be transferred from.
 *
 *     dst       - the destination address for the copy operation.
 *                 This area is assumed to be logically contiguous.
 *
 *     size      - the size of data to be transferred from source.
 *                 size must be a +ve integer (i.e, > 0).
 * 
 *  [OUTPUT PARAMETERS]
 *      
 *     no output arguments. The function returns error if the mbufs pointed
 *     to by the src_ptr run out of data before enough data is gathered.
 *     In this case, all the data in the mbuf chain is still copied before
 *     the error is given.
 *
 ***************************************************************************/
 
 error_number 
 copy_out_data(src_ptr, dst, size)
 register struct mbuf *src_ptr;
 register anyptr dst;
 register unsword size;

 {
   register unsword   size1;


#ifdef DEBUG    
       printf(">>>>>> IN COPY_OUT_DATA <<<<<<<<<<\n");
#endif /* DEBUG */    

       while (size)  
        {
          if (src_ptr == NIL)  return (E_OUT_OF_MEMORY);

          /* the following is the MIN function */
          size1 = (size > src_ptr->m_len) ? src_ptr->m_len : size; 

          copy_network_data(mtod(src_ptr,anyptr),dst,size1);

          /* if there's more left then fill next buffer; loop until done */

          size -= size1;
          if (size > 0) 
           {
             dst  += size1;
             src_ptr = src_ptr->m_next;
           }
         }
        return (0); 
 }


/***************** FUNCTION : hw_loopback *******************************
 *
 * [OVERVIEW]	 
 *      This function puts the frame described by "frame_ptr" and 
 *	"frame_len" and puts it in the inbound queue if it is
 *	destined for this node.  It provides a full-duplex functionality.
 * [INPUT PARAMETERS]
 *	frameptr     -- pointer to the frame 
 *	framelen     -- length of the frame 
 * [RETURNS]
 *	TRUE  -- if the packet is to be sent
 *	FALSE -- the packet is for this node only, do not send it over
 *		 the network (for efficiency's sake).
 ************************************************************************/

boolean
hw_loopback( hwvars, frameptr)
register hw_gloptr	hwvars;
register struct mbuf *frameptr;


	 
{
/*-------------------------- LOCAL VARIABLES ----------------------------
 *	myglobal   -- points to the driver global variables
 *	offset	   -- where the frame should start within the buffer
 *	tmpbufptr -- points to destination buffer (via get_buffer)
 *	s	   -- save the interrupt level
 *	send_also  -- flag holding the result of function (to send or not
 *		      to send)
 *---------------------------------------------------------------------*/
	register lan_gloptr	myglobal;
	register packet_offset frame_len;
		 struct mbuf *tmpbufptr;
		 boolean	send_also;
                 lan_ift        *is;
			  int	   s;
		 anyptr    src = mtod(frameptr,anyptr);

#ifdef LAN_MS
/* measurement instrumentation */
lan_rheader      *lan; 
extern struct timeval   ms_gettimeofday();
static struct {
                struct timeval tv;
                int     count;
                int     unit;
                int     protocol;
                int     addr;
              } ms_io_info;
#endif /* LAN_MS */

mib_ifEntry *mib_ptr = NULL;

/*-------------------------- ALGORITHM ----------------------------------
 *	IF need to loop back the frame THEN
 *          calculate type of frame and align it
 *	    copy the frame to an inbound buffer
 *	    queue the inbound buffer
 *	    kick off the receive_task
 *---------------------------------------------------------------------*/


	myglobal = (lan_gloptr) hwvars->drvglob;
        is = (lan_ift *) myglobal->lan_ift_ptr;
	send_also = TRUE;

	mib_ptr = &(is->mib_xstats_ptr->mib_stats);

	{
	struct mbuf * m2;
	int len = 0;
	for (m2 = frameptr; m2; m2 = m2->m_next)
		len += m2->m_len;
        frame_len = len;
	}

	if ( INDIVIDUAL((unsbyte *)src ) )
        {
	    if ( !SAME_ADDRESS((unsbyte *)src, myglobal->local_link_address)) 
            {
		/*HW_QA_INCR_STAT( hwvars, INDIVIDUAL_DROP );*/
		return( TRUE );
	    }
	    HW_QA_INCR_STAT( hwvars, IND_RECEIVE );
	    send_also = FALSE;
	} 
        else if ( BROADCAST((unsbyte *)src) ) 
        {
	    if ( myglobal->broadcast_state == FALSE ) 
               {
		HW_QA_INCR_STAT( hwvars, INT_BCAST_DROP );
		return( TRUE );
	       }
	    HW_QA_INCR_STAT( hwvars, BCAST_TAKEN );
/*          send_also = TRUE;   at this point      */
	} 
        else /* must be multicast */ 
        {
            if(!hw_find_mcaddr(is,(unsigned char *)src)) {
		HW_QA_INCR_STAT( hwvars, INT_MCAST_DROP );
		return( TRUE );
	    } 
	    HW_QA_INCR_STAT( hwvars, MCAST_TAKEN );
/*          send_also = TRUE;    at this point   */
	}

/*
 *	determine odd/even boundary 
 */

	if (!IS_ETHERNET(READ_WORD( (unsbyte *) src  + LAN_TYPE_OFF )) ) {

/*
 *	    The actual size of the frame for IEEE802 is its length field.
 *	    No check is done on the validity of this field since I 
 *	    presume that the format is correct before sending.
 */
	    frame_len = READ_WORD((unsbyte *)src +IEEE_LEN_OFF ) + IEEE_LEN_ADD;
	}

/*
 *	get the architecture buffer: returns ptr to mbuf chain or NIL
 */
	
	/* Since lan0 runs at a higher priority then we probably are,
	   we need to disable interrupts before setting up the trigger.   --pcn */
	s=splimp();
	if (get_buffer(is, frame_len + 4,  &tmpbufptr) != E_NO_ERROR)
           {
	    splx(s);
/* packet is NOT dropped; it is enqueued for sending over the network */
/*	    hw_incr_stat( hwvars, UNDELIVERABLE );         */
	    return(TRUE);
	   }
	splx(s);
	
/* loopback'ed packet; do not add 4 bytes for CRC   */
	mib_ptr->ifInOctets += frame_len;
/* remember:  this is a LOOPBACK packet !           */
	mib_ptr->ifOutOctets += frame_len;
	
	/* this used to be in the old lanc_output() */
        if(((lan_rheader *)src)->destination[0] & 0x01) {
		mib_ptr->ifOutNUcastPkts++;
	} else {
		mib_ptr->ifOutUcastPkts++;
	}

	cp_mbufs_to_mbufs( frameptr, tmpbufptr, frame_len);
	tmpbufptr->m_off += 4;
	tmpbufptr->m_len -= 4;

/******************************************************************* 
 * *** pad inbound mbufs to account for the standard requirement for
 * *** packet size
 ********************************************************************/
	if (tmpbufptr->m_len < 60)
	   {
	    tmpbufptr->m_len =60;
	   }
	s=splimp();
	
#ifdef LAN_MS 
      /* measurement instrumentation */
      if (ms_turned_on(ms_lan_read_queue))
        {
        lan = (lan_rheader *) (mtod(tmpbufptr, struct mbuf *));
        ms_io_info.tv = ms_gettimeofday();
        if (lan->type < MIN_ETHER_TYPE)
          {
          ms_io_info.protocol = 1; /*IEEE packet*/
          ms_io_info.count = lan->type;
          ms_io_info.addr = ((struct ieee8023_hdr *)lan)->dsap;
          }
        else
          {
          ms_io_info.protocol = 0; /*Ethernet packet*/
          ms_io_info.addr = lan->type;
          ms_io_info.count = frame_len;
          }
        ms_io_info.unit = is->is_if.if_unit;
        ms_data(ms_lan_read_queue,&ms_io_info,sizeof(ms_io_info));
        }
#endif /* LAN_MS */

	ns_trace_link (is, TR_LINK_OUTBOUND, tmpbufptr, NS_LS_LAN0);

	hw_rint_ics(is,tmpbufptr);
	splx(s);
	return(send_also);
}


/* #ifdef DUX */

error_number
hw_send_dux( hwvars, frame_pointer, frame_len )
register hw_gloptr	hwvars;
charptr	frame_pointer;
register packet_offset	frame_len;
{
	register unsbyte *	nextringfill;
	register unsbyte *	fill;
	register unsbyte *	empty;
	register packet_offset	alloc_len;

	int s;
	struct dm_header *dmp; 
	lan_gloptr	 myglobal;
	register struct proto_header *ph;
	int dm_length; 			/*length of dm_header */
	int  data_length;		/*length of data part in the packet */
	register int byte_sent;


	/*
	** DUX uses mbufs quite differently than CND. Our dux_m_dat[] area
	** of the dux_mbuf contains data that is to be transmitted as well
	** as data that is NOT to be transmitted. As a result, it is VERY
	** difficult to simply pass hw_send() a pointer to an mbuf, or even
	** a pseudo-mbuf, since the data to be transmitted is not contiguous
	** within the dux_m_dat[] area. AND to make matters even worse, the leaf
	** node copy of hw_send for DUX, also modified a DUX structure that is
	** used later on when the isr_proc routine is invoked as a result of
	** a transmit interrupt. There were only two things that we
	** could do to make this work with the new CND S300 LAN driver. Either
	** DUX could change all of its dm interface and protocol code to 
	** conform to the CND/IND version of how mbufs should be layed out
	** -OR- put this special purpose code in the LAN driver. This approach
	** was much, much simpler -especially when considering our schedule:
	**
	**  struct dux_mbuf{
	**	struct dux_mbuf *m_next;
	**	u_long m_off;
	**	short m_len;
	**	short m_type;
	**
	**	u_char dux_m_dat[128];
	**	struct dux_mbuf *m_act;
	**	}
	**
	**
 	** The DATA portion (dux_m_dat[128] of the dux_mbuf contains four parts:
	**
	**	struct dm_message
	**	{
	**	   struct dm_header 
	**	   {	
	**		 * This part is transmitted *
	**		struct proto_header dm_ph;
	**
	**		 * This part is NOT transmitted *
	**		struct dm_no_transmit dm_no_transmit;
	**
	**		 * This part is transmitted *
	**		struct dm_transmit dm_transmit;
	**
	**		char dm_params[2];
	**	    }
	**	}
	*/


     mib_xEntry   *mib_xptr;
     mib_ifEntry  *mib_ptr ; 

/* declare the lanift related structs. */
     lan_gloptr lg_ptr;
     lan_ift *is;
     int l_u;
     lg_ptr = (lan_gloptr) (hwvars->drvglob);
     is = (lan_ift *) lg_ptr->lan_ift_ptr;
     l_u = is->lu; /* logical unit corresponding to this card */

     mib_xptr = is->mib_xstats_ptr;
     mib_ptr  = &(mib_xptr->mib_stats);

#ifdef DEBUG    
    printf(">>>> hw_send_dux <<<<\n");
#endif /* DEBUG */    
   /* if hardware has failed, drop pkt & quit */ 
    if (hwvars->card_state != HARDWARE_UP) 
    {
#ifdef DEBUG    
    printf("\n HARDWARE DOWN, pkt DROPPED: card %d, select_code = %d\n", l_u+1, hwvars->select_code); 
#endif /* DEBUG */    
     /* incr the output packets  in err */
     is->is_ac.ac_if.if_oerrors++;
     mib_xptr->if_DOutErrors++;
     return(E_HARDWARE_FAILURE);
    }


/*
** Need to serialize DUX accesses: 
** IMPORTANT NOTE: hw_send() and hw_send_dux() must both
**                 allow access to the card at the same
**                 level. Hw_send() uses splimp() for
**                 protection, which is actually spl5() or
**                 the level CRIT5(). If splimp() ever
** 		   changes to a different priority level,
**                 then the DUX code must be changed to
**                 correspond.
*/
    s = CRIT5();

    /* Assume that the frame_len is in range */
    NS_ASSERT((frame_len>=60 && frame_len<=1514), "E_HW_FRAME_RANGE" );

/*
 *  precalculate the ring element to use and check for ring full
 */

    nextringfill = hwvars->txring_fill + RING_SIZE;

    if (nextringfill >= hwvars->txring_end) 
	nextringfill = hwvars->txring_start;

    if (nextringfill == hwvars->txring_empty)
    {
	 /* no rings available */
	HW_QA_INCR_STAT( hwvars, TX_NO_RINGS );
	UNCRIT(s);
	return(E_LLP_NO_BUFFER_SEND);
    }

    alloc_len = frame_len + 1;

/*
 *  It is important to make empty be a temporary variable since tx_empty
 *  could be modified by the isr_proc.	Putting it in a temporary variable
 *  avoids the critical section (since we are only reading empty).
 */

    fill = hwvars->tx_fill;
    empty = hwvars->tx_empty;	
    if ( fill >= empty )
    {
	if (hwvars->tx_end - fill <= alloc_len)
        {
	    /* cannot allocate in (1) */
	    if (empty - hwvars->tx_start > alloc_len)
            {
		/* can allocate in (2) */
		fill = hwvars->tx_start;
	    }
            else
            {
		/* no room to allocate */
		HW_QA_INCR_STAT( hwvars, TX_NO_ROOM_A );
	        UNCRIT(s);
		return( E_LLP_NO_BUFFER_SEND );
	    }
	}
    }
    else
    {
	if (empty - fill <= alloc_len)
        {
	    /* no room to allocate */
	    HW_QA_INCR_STAT( hwvars, TX_NO_ROOM_B );
	    UNCRIT(s);
	    return( E_LLP_NO_BUFFER_SEND );
	}
    }

/*
** This is probably not needed - daveg
*/
if( ((unsword) frame_pointer & 1) != ((unsword) fill & 1) )
{
	HW_QA_INCR_STAT( hwvars, TX_NEED_ALIGN );
	fill++;
}


/*
 *  Copy the frame to "fill" (on the card)
 */

/*
**	Copy the DUX link address (source address) to the protocol header.
**
*/
       myglobal = Myglob_dux_lan_gloptr;

/*
** protocol header
*/
       dmp = (struct dm_header *) frame_pointer;
       ph = &(dmp->dm_ph);

       LAN_COPY_ADDRESS(myglobal->local_link_address, 
        	    ph->iheader.sourceaddr);

       byte_sent = ph->p_byte_no;

       mib_ptr->ifOutOctets += frame_len;  /* includes everything */

	/* this used to be in the old lanc_output() */
        if(ph->iheader.destaddr[0] & 0x01) {
		mib_ptr->ifOutNUcastPkts++;
	} else {
		mib_ptr->ifOutUcastPkts++;
	}

/*
** 1st copy the PROTO_LENGTH == sizeof(proto_header)
*/
       bcopy( dmp, fill, PROTO_LENGTH );

	 /*
	 ** DUX messages whose types are struct dm_message
	 */

          /* copy dmmsg if any */
          dm_length = MINIMUM2(ph->p_dmmsg_length - byte_sent,
			 frame_len - PROTO_LENGTH );

          if ( dm_length <= 0)
	  {
	     dm_length = 0;
          }
          else
          {
	     /* 
	     ** There is dmmsg to be sent in this packet
	     */
	     bcopy((u_char *) dmp + XMIT_OFFSET + byte_sent,
	  		    fill + PROTO_LENGTH, dm_length);

	     byte_sent += dm_length;
	  }

          /* 
	  ** Copy data if any
	  */
          data_length=MINIMUM2(ph->p_data_length + ph->p_dmmsg_length - 
		byte_sent, frame_len - PROTO_LENGTH - dm_length);

          if ( data_length > 0 ) 
	  {
	      bcopy(BUF_ADDR(dmp)+(byte_sent - ph->p_dmmsg_length),
			    fill + PROTO_LENGTH + dm_length, data_length);
	  }

          ph = (struct proto_header *) fill; /* ph is changed */
          ph->p_data_offset = dm_length;

/*
 *  Get the transmit ring ready
 */

    CARDSWORD( hwvars->txring_fill + TMD2_OFF ) = - frame_len;
		/* negate since TMD2 is defined to be negative */
    CARDWORD( hwvars->txring_fill + TMD3_OFF ) = TMD3_RETURN;
    WRITEADDR( hwvars->txring_fill, fill );

/*
 *  The order of the following two lines is important!
 *  It is because the transmit interrupt can happen in between
 *  the two statements.	 There probably should be a "protect"
 *  here, but it works correctly because of the corresponding
 *  isr handling.
 */

    hwvars->xmit_queued++;
    CARDBYTE( hwvars->txring_fill + TMD1_OFF ) = TMD1_RETURN;

/*
 *  Notify the LANCE that there is something to send.
 */

    if ( !write_rdp( hwvars, TDMD_BIT | INEA_BIT ) )
    {
        UNCRIT(s);
	return( E_HARDWARE_FAILURE );
     }

	/*
	** In the Series 300 HP/Apollo merged products the processor board
	** leds will be user visable on the front panel.  Four of these
	** lights have been defined to have special meaning.  To be 
	** compatible with Apollo products we will blink the lights in
	** the manner that they have defined.
	**
	** Toggle the lan driver transmit led so the front 
	** panel led will indicate lan transmit activity.
	*/
	toggle_led(LAN_XMIT_LED);

/*
 *  Update ring pointer / buffer pointer
 */

    hwvars->txring_fill = nextringfill;

    fill += frame_len;

/*
 *  Tricky stuff here:	note that because of the allocation above will
 *  always leave at least one byte at the end of the buffer, therefore
 *  you don't need to check if "dest" is at the end of buffer!
 */
    NS_ASSERT( fill < hwvars->tx_end, "E_HW_TX_PAST_END1" );
    hwvars->tx_fill = fill;
    
    UNCRIT(s);
    return( E_NO_ERROR );
}
/* #endif DUX */

/***************** FUNCTION : hw_send ***********************************
 *
 * [OVERVIEW]	 
 *	This function copies the frame onto the card and initiates the
 *	LANCE to transmit the frame.
 *	The frame is described by frame_pointer which points to the
 *	start of the buffer containing the frame, and frame_len
 *	which is the size of the frame.
 *	For efficiency, this routine will guarantee copying from odd 
 *	address to odd address or even address to even address.
 *
 * [INPUT PARAMETERS]
 *	hwvars	      -- points to global variables for the hardware
 *	frame_pointer -- pointer to the frame to be transmitted
 *	frame_len     -- length of the frame to be transmitted
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	tx_fill,	txring_fill,	xmit_queued
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	txring_empty,	txring_start,	txring_end
 *	tx_empty,	tx_start,	tx_end
 *
 ************************************************************************/

void
hw_send(is)
lan_ift * is;


{

 lan_rheader *lanhdr;
 register hw_gloptr	hwvars = &((landrv_ift *)is)->lan_vars.hwvars;
 struct mbuf *frameptr;
 register unsbyte *	nextringfill;
 register unsbyte *	fill;
 register unsbyte *	empty;
 register packet_offset	alloc_len;
 register packet_offset	frame_len;
 struct ifqueue  * ifq_snd_ptr;

 mib_xEntry  *mib_xptr = is->mib_xstats_ptr;
 mib_ifEntry *mib_ptr = &(is->mib_xstats_ptr->mib_stats);

#if NEED_TIMER
 all_token	save_int;
#endif /* NEED_TIMER */
 

/*----------------------------- ALGORITHM -------------------------------
 *	-- allocate ring element
 *	nextringfill := TXRING_FILL + ring size;
 *	if nextringfill >= TXRING_END then nextringfill := TXRING_START;
 *	if nextringfill == TXRING_EMPTY then return( e_no_buffer_send );
 *
 *	-- allocate 1 extra byte in case need to align for faster copy
 *	alloc_len := frame_len + 1;
 *
 *	if TX_FILL >= TX_EMPTY then
 *	|   -- assume can allocate to end of physical buffer 
 *	|   if TX_END-TX_FILL <= alloc_len then
 *	|   |	if TX_EMPTY-TX_START > alloc_len then
 *	|   |	|   -- allocate at beginning of physical buffer
 *	|   |	else
 *	|   |	|   -- no room to allocate
 *	|   |	end
 *	|   end
 *	else
 *	|   -- assume can allocate in the middle of physical buffer
 *	|   if TX_EMPTY-TX_FILL <= alloc_len then
 *	|   |	return( e_no_buffer_send );
 *	|   end
 *	end
 *
 *	if dest and frame_pointer are not both even or both odd then
 *	|   dest := dest + 1;
 *	end;
 *	copy( frame_pointer, dest, frame_len );
 *
 *	-- get ring ready for transmission and send packet
 *	TXRING_FILL->buffer_count := size;
 *	TXRING_FILL->buffer_pointer := dest;
 *
 *	XMIT_QUEUE++;
 *	TXRING_FILL->stat := start send stat constant;
 *	lance control word<transmit demand> := 1;
 *
 *	-- update ring pointer
 *	TXRING_FILL := nextringfill;
 *
 *	-- update buffer pointer
 *	dest := dest + size;
 *	if dest >= TX_END then 
 *	|   TX_FILL := TX_START;
 *	else
 *	|   TX_FILL := dest;
 *	end
 *
 *	-- setup transmitter timeout
 *	return( e_no_error );
 *---------------------------------------------------------------------*/
/* declare the lanift related structs. */
 int l_u;
 l_u = is->lu; /* logical unit corresponding to this card */

 /* if hardware has failed, drop pkt & quit */ 
 if (hwvars->card_state != HARDWARE_UP) 
    {
#ifdef DRVR_TEST
     printf("\n HARDWARE DOWN, pkt DROPPED: card %d, select_code = %d\n", l_u+1, hwvars->select_code); 
#endif /* DRVR_TEST */
     /* incr the output packets  in err */
     is->is_ac.ac_if.if_oerrors++;

     mib_xptr->if_DOutErrors++;

     /* SK: should log disaster here and return */
     return;
    }

 ifq_snd_ptr = &is->is_ac.ac_if.if_snd;
 while (ifq_snd_ptr->ifq_len > 0){
    frameptr = ifq_snd_ptr->ifq_head;/*SK: take a peek at the ifnet queue */ 
/* ***********************************************************************
 * *** check the length of the frame and pad as necessary.  This is done
 * *** by merely increasing the length field of the last mbuf in the frame.
 * ************************************************************************/
  {
  int len = 0;
  struct mbuf *m2;
  for (m2 = frameptr; m2; m2 = m2->m_next)
    len += m2->m_len;
  frame_len = len;
  }
 if (frame_len < 60)
    {
     frame_len = 60;
    }

 NS_ASSERT( frame_len>=60 && frame_len<=1514, "E_HW_FRAME_RANGE");

/* SK: hw_loopback() which used to be here is now called _before_ hw_send().

#ifdef NPERFORM
 {
  int x;
  x = hwvars->xmit_queued;
  if (x > 4) 
     {
      HW_QA_INCR_STAT( hwvars, QUEUED_ARRAY+5 );
     } 
  else 
     {
      HW_QA_INCR_STAT( hwvars, QUEUED_ARRAY+x );
     }
 }
 HW_QA_INCR_STAT( hwvars, (LEN_ARRAY+frame_len/128) );
#endif /* NPERFORM */

/*
 *  precalculate the ring element to use and check for ring full
 */

 nextringfill = hwvars->txring_fill + RING_SIZE;
 if (nextringfill >= hwvars->txring_end) 
    nextringfill = hwvars->txring_start;
 if (nextringfill == hwvars->txring_empty) 
    { /* no rings available */
     ((landrv_ift *)is)->lan_vars.flags |= LAN_NO_TX_BUF;
     return;
    }
 
 alloc_len = frame_len + 1;

 fill = hwvars->tx_fill;
 empty = hwvars->tx_empty;	
 if ( fill >= empty ) 
    {

/*
 *	   (2)			   (1)
 *	|	 |-------------|	 |
 *	       empty	      fill
 *	    ^			    ^
 *	    |			    |
 *	try to allocate above (in either "blank" areas)
 */

     if (hwvars->tx_end - fill <= alloc_len) 
	{
	 /* cannot allocate in (1) */
	 if (empty - hwvars->tx_start > alloc_len) 
	    {
	     /* can allocate in (2) */
	     fill = hwvars->tx_start;
	    } 
	 else 
	    {
		/* no room to allocate */
             ((landrv_ift *)is)->lan_vars.flags |= LAN_NO_TX_BUF;
	     return;
	    }
	}
    } 
 else 
    {

/*
 *	|--------|	       |---------|
 *		fill	     empty
 *			^
 *			|
 *	       try to allocate above
 */

     if (empty - fill <= alloc_len) 
	{
	    /* no room to allocate */
	 ((landrv_ift *)is)->lan_vars.flags |= LAN_NO_TX_BUF;
	 return;
	}
    }


    IF_DEQUEUE(ifq_snd_ptr, frameptr); /* now that we are sure we have 
                                          buffer space on the card, actually 
                                          dequeue the packet*/

  ns_trace_link (&(is->is_if), TR_LINK_OUTBOUND, frameptr, NS_LS_LAN0);

/*
 *  Copy the frame from mbufs pointed to by vquad chain to "fill" (on the card)
 */

 copy_out_data( frameptr, fill, frame_len );

 mib_ptr->ifOutOctets += frame_len + 4;

	/* this used to be in the old lanc_output() */
	lanhdr=mtod(frameptr,lan_rheader *);
        if(lanhdr->destination[0] & 0x01) {
		mib_ptr->ifOutNUcastPkts++;
	} else {
		mib_ptr->ifOutUcastPkts++;
	}

 /* free mbufs */
 m_freem(frameptr);

/*
 *  Get the transmit ring ready
 */

 CARDSWORD( hwvars->txring_fill + TMD2_OFF ) =  -frame_len;
		/* negate since TMD2 is defined to be negative */
 CARDWORD( hwvars->txring_fill + TMD3_OFF ) = TMD3_RETURN;
 WRITEADDR( hwvars->txring_fill, fill );

 hwvars->xmit_queued++;
 CARDBYTE( hwvars->txring_fill + TMD1_OFF ) = TMD1_RETURN;

/*
 *  Notify the LANCE that there is something to send.
 */

 if ( !write_rdp( hwvars, TDMD_BIT | INEA_BIT ) )
    {
    /* incr the out error pkt count */
     is->is_ac.ac_if.if_oerrors++;
     return; /* used to return E_HARDWARE_FAILURE */
    }

	/*
	** In the Series 300 HP/Apollo merged products the processor board
	** leds will be user visable on the front panel.  Four of these
	** lights have been defined to have special meaning.  To be 
	** compatible with Apollo products we will blink the lights in
	** the manner that they have defined.
	**
	** Toggle the lan driver transmit led so the front 
	** panel led will indicate lan transmit activity.
	*/
	toggle_led(LAN_XMIT_LED);

/*
 *  Update ring pointer / buffer pointer
 */

 hwvars->txring_fill = nextringfill;
 
 fill += frame_len;

/*
 *  Tricky stuff here:	note that because of the allocation above will
 *  always leave at least one byte at the end of the buffer, therefore
 *  you don't need to check if "dest" is at the end of buffer!
 */
 NS_ASSERT( fill < hwvars->tx_end,"E_HW_TX_PAST_END1");
 hwvars->tx_fill = fill;
    
#if NEED_TIMER

/*
 *  Set up watchdog timer
 */

 ALL_PROTECT( hwvars, &save_int );
 if ( ! hwvars->timer_set && hwvars->xmit_queued > 0 ) 
    {
     timeout( xmit_timer, hwvars, XMIT_TIMEOUT, 
	     &hwvars->timeout_loc );
     hwvars->timer_set = TRUE;
     HW_QA_INCR_STAT( hwvars, TX_QUEUED );
    } 
 else 
    {
     HW_QA_INCR_STAT( hwvars, TX_SENT );
    }
 ALL_UNPROTECT( hwvars, save_int );

#endif /* NEED_TIMER */


/* incr the out pkt count; if pkt is dropped, we don't count them in _oerrors */
 is->is_ac.ac_if.if_opackets++;
  
} /* end of while */
 /* if we get here then buffers (possibly) available */
 ((landrv_ift *)is)->lan_vars.flags &= ~LAN_NO_TX_BUF;

 return;
}

/***************** FUNCTION : copyin_frame ******************************
 *
 * [OVERVIEW]	 
 *	This function copies a frame from the card (the one pointed to 
 *	by RXRING) to the mbuf chain pointed to by dest.
 *	This function assumes that RXRING<start> is true,
 *	and that all rings up to <end> are owned by the driver.
 *
 * [INPUT PARAMETERS]
 *	hwvars	  -- points to the globals area for the card.
 *	dest	  -- points to the destination of the copy.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	rxring
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	rxring_start,	rxring_end
 *
 ************************************************************************/

void
copyin_frame( hwvars, dest )
register hw_gloptr	hwvars;
struct mbuf *		dest;

{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	accum_size -- keeps track of how many bytes copied
 *	src_addr   -- address of the buffer on card to copy
 *	size	   -- size of the buffer on card to copy
 *---------------------------------------------------------------------*/

	register unsword	accum_size;
	register unsword	size;
	register unsbyte *	src_addr;

/*----------------------------- ALGORITHM -------------------------------
 *	accum_size := 0;
 *	while not RXRING<end> and not RXRING<err> do
 *	|   src_addr := RXRING->buffer_pointer;
 *	|   size := RXRING->buffer_count;
 *	|   copy_network_data(src_addr, dest, size);
 *	|   dest := dest + size;
 *	|   accum_size := accum_size + size;
 *	|   -- return this ring for future use
 *	|   RXRING->stat := constant stat for returning receive rings;
 *	|   RXRING->message_count := 0;
 *	|   RXRING := RXRING + ring size;
 *	|   if RXRING >= RXRING_END then RXRING := RXRING_START;
 *	end;
 *	src_addr := RXRING->buffer_pointer;
 *	size := RXRING->message_count - accum_size;
 *	copy_network_data(src_addr, dest, size);
 *	accum_size := accum_size + size;
 *	-- return this ring for future use
 *	RXRING->stat := constant stat for returning receive rings;
 *	RXRING->message_count := 0;
 *	RXRING := RXRING + ring size;
 *	if RXRING >= RXRING_END then RXRING := RXRING_START;
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> copyin_frame <<<<\n");
#endif /* DEBUG */   

    /* assume rxring points to the start of the packet */
    NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & STP_BIT) == STP_BIT, 
	   "E_HW_NOT_STP1");

    accum_size = 0;
    while ( (CARDWORD( hwvars->rxring+RMD1_OFF ) & (ENP_BIT|ERR_BIT)) == 0 ) 
     {
	NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & OWN_BIT) == MY_OWN,
	"E_HW_NOT_OWN1");
	src_addr = READADDR( hwvars->rxring );
	size = - CARDSWORD( hwvars->rxring + RMD2_OFF );
			/* negate since RMD2 is defined to be negative */
	copy_in_data( src_addr, &dest, size);
	accum_size += size;
/*
 *	return the ring to the LANCE for future use
 */
	CARDWORD( hwvars->rxring+RMD3_OFF ) = RMD3_RETURN;
	CARDBYTE( hwvars->rxring+RMD1_OFF ) = RMD1_RETURN;
	hwvars->rxring += RING_SIZE;
	if ( hwvars->rxring >= hwvars->rxring_end )
	    hwvars->rxring = hwvars->rxring_start;
    }
    NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & OWN_BIT) == MY_OWN,
	   "E_HW_NOT_OWN2");

    src_addr = READADDR( hwvars->rxring );
    size = (CARDWORD( hwvars->rxring+RMD3_OFF ) & MCOUNT_FIELD) - accum_size;
    copy_in_data( src_addr, &dest, size );

	/*
	** In the Series 300 HP/Apollo merged products the processor board
	** leds will be user visable on the front panel.  Four of these
	** lights have been defined to have special meaning.  To be 
	** compatible with Apollo products we will blink the lights in
	** the manner that they have defined.
	**
	** Toggle the lan driver receive led so the front 
	** panel led will indicate lan receive activity.
	*/
	toggle_led(LAN_RCV_LED);

/*
 *  return the ring to the LANCE for future use
 */

    CARDWORD( hwvars->rxring+RMD3_OFF ) = RMD3_RETURN;
    CARDBYTE( hwvars->rxring+RMD1_OFF ) = RMD1_RETURN;
    hwvars->rxring += RING_SIZE;
    if ( hwvars->rxring >= hwvars->rxring_end )
	hwvars->rxring = hwvars->rxring_start;
    
}

/*#ifdef DUX */
/***************** FUNCTION : dux_copyin_frame **************************
 *
 * [OVERVIEW]	 
 *	This function copies a frame from the card (the one pointed to 
 *	by RXRING) to the buffers pointed to by dest1 and dest2.
 *	Data will be copied into the buffer pointed to by dest1
 *	until len1 bytes have been copied.  Then, the rest of the
 *	data will be copied into dest2.  If all of the data is to
 *	be copied into dest1, then both len1 and dest2 should be zero.
 *	The DUX protocol header will not be copied.
 *	This function assumes that RXRING<start> is true,
 *	and that all rings up to <end> are owned by the driver.
 *	This function also assumes that it will not be called for
 *	non-DUX packets.
 *
 * [INPUT PARAMETERS]
 *	hwvars	  -- points to the globals area for the card.
 *	dest1	  -- points to the first destination of the copy.
 *	len1	  -- number of bytes to go into dest1.
 *	dest2	  -- points to the second destination of the copy.
 *
 * [OUTPUT PARAMETER]
 *	return value -- number of bytes copied.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	rxring
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	rxring_start,	rxring_end
 *
 ************************************************************************/

unsword
dux_copyin_frame( hwvars, dest1, len1, dest2 )
register hw_gloptr	hwvars;
charptr		dest1,dest2;
int		len1;
{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	accum_size -- keeps track of how many bytes copied
 *	src_addr   -- address of the buffer on card to copy
 *	size	   -- size of the buffer on card to copy
 *---------------------------------------------------------------------*/

	register unsword	accum_size;
	register unsword	size;
	register unsbyte *	src_addr;
	unsbyte 		*old_rxring;
	register int 		byte_left;
	register charptr	dest = dest1;
	

#ifdef DEBUG    
    printf(">>>> dux_copyin_frame <<<<\n");
#endif /* DEBUG */   

    /* assume rxring points to the start of the packet */
    NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & STP_BIT) == STP_BIT, 
	   "E_HW_NOT_STP2");

/*
 *  get the packet size from the last rxring
 */
        old_rxring = hwvars->rxring;
        while (( CARDWORD (hwvars->rxring + RMD1_OFF ) & ENP_BIT) == 0 ){
	    hwvars->rxring += RING_SIZE;
	    if ( hwvars->rxring >=hwvars->rxring_end )
                hwvars->rxring = hwvars->rxring_start;
        }
        byte_left = (CARDWORD(hwvars->rxring+RMD3_OFF) & MCOUNT_FIELD) -
		    CRC_SIZE;
	hwvars->rxring = old_rxring;

    if (!len1) len1=byte_left;  /* Check for case of single destination. */
    accum_size = 0;
    while ( (CARDWORD( hwvars->rxring+RMD1_OFF ) & (ENP_BIT|ERR_BIT)) == 0 ) {
	NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & OWN_BIT) == MY_OWN,
	"E_HW_NOT_OWN1");
	src_addr = READADDR( hwvars->rxring );
	size = - CARDSWORD( hwvars->rxring + RMD2_OFF );
			/* negate since RMD2 is defined to be negative */
	    /* skip the DUX protocol header in each packet */
	    size = (byte_left < size ) ? byte_left : size;
	    if (accum_size == 0) {
	        src_addr +=PROTO_LENGTH;
		size  -=PROTO_LENGTH;
		byte_left -= PROTO_LENGTH;
		accum_size += PROTO_LENGTH;
	    }
	if (size > len1) {
		bcopy( src_addr, dest, len1);
		accum_size += len1;
		byte_left -= len1;
		size -= len1;
		src_addr += len1;
		len1 = byte_left + 1;
		dest = dest2;
	}
	bcopy( src_addr, dest, size );
	dest += size;
	accum_size += size;
	byte_left -= size;
	len1 -= size;
	if (!len1) {
		len1 = byte_left + 1;
		dest = dest2;
	}

/*
 *	return the ring to the LANCE for future use
 */
	CARDWORD( hwvars->rxring+RMD3_OFF ) = RMD3_RETURN;
	CARDBYTE( hwvars->rxring+RMD1_OFF ) = RMD1_RETURN;
	hwvars->rxring += RING_SIZE;
	if ( hwvars->rxring >= hwvars->rxring_end )
	    hwvars->rxring = hwvars->rxring_start;
    }
    NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & OWN_BIT) == MY_OWN,
	   "E_HW_NOT_OWN2");

    src_addr = READADDR( hwvars->rxring );
    size = (CARDWORD( hwvars->rxring+RMD3_OFF ) & MCOUNT_FIELD) - accum_size;

        /* skip the DUX protocol header in each packet */
        size = (byte_left < size ) ? byte_left : size;
        if (accum_size == 0) {
            src_addr +=PROTO_LENGTH;
            size  -=PROTO_LENGTH;
            byte_left -= PROTO_LENGTH;
            accum_size += PROTO_LENGTH;
        }

	if (size > len1) {
		bcopy( src_addr, dest, len1);
		accum_size += len1;
		size -= len1;
		src_addr += len1;
		dest = dest2;
	}
    bcopy( src_addr, dest, size );
    accum_size += size;

/*
 *  return the ring to the LANCE for future use
 */

    CARDWORD( hwvars->rxring+RMD3_OFF ) = RMD3_RETURN;
    CARDBYTE( hwvars->rxring+RMD1_OFF ) = RMD1_RETURN;
    hwvars->rxring += RING_SIZE;
    if ( hwvars->rxring >= hwvars->rxring_end )
	hwvars->rxring = hwvars->rxring_start;
    
	/*
	** In the Series 300 HP/Apollo merged products the processor board
	** leds will be user visable on the front panel.  Four of these
	** lights have been defined to have special meaning.  To be 
	** compatible with Apollo products we will blink the lights in
	** the manner that they have defined.
	**
	** Toggle the lan driver receive led so the front 
	** panel led will indicate lan receive activity.
	*/
	toggle_led(LAN_RCV_LED);

    return(accum_size - PROTO_LENGTH);
}


/* #endif DUX */

/***************** FUNCTION : skip_incomplete_frame **************************
 *
 * [OVERVIEW]	 
 *	This function throws away the current frame pointed to by RXRING.
 *	This frame is assumed to be an incomplete frame which does not
 *	have an ENP (nor ERR), but starts again with a STP.
 *	This function assumes that all ring elements up to the second
 *	STP is owned by the driver.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to the globals area for the card.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	rxring
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	rxring_end,	rxring_start
 *
 ************************************************************************/

void
skip_incomplete_frame( hwvars )
register hw_gloptr	hwvars;

{

/*----------------------------- ALGORITHM -------------------------------
 *	repeat
 *	|   RXRING->stat := constant stat for returning receive rings;
 *	|   RXRING := RXRING + ring size;
 *	|   if RXRING >= RXRING_END then RXRING := RXRING_START;
 *	until RXRING<stp>
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> skip_incomplete_frame <<<<\n");
#endif /* DEBUG */    
    HW_QA_INCR_STAT( hwvars, SKIP_BAD_COUNT );

    /* assume rxring points to the start of the packet */
    NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & STP_BIT) != 0, 
	   "E_HW_NOT_STP3");

    do {
/*
	WARNING( (CARDWORD(hwvars->rxring+RMD1_OFF) & (OWN_BIT|ENP_BIT|ERR_BIT))
		 != 0, E_HW_NOT_OWN3, 0, CARDWORD(hwvars->rxring+RMD1_OFF), 0 );
*/
	CARDWORD( hwvars->rxring+RMD3_OFF ) = RMD3_RETURN;
	CARDBYTE( hwvars->rxring+RMD1_OFF ) = RMD1_RETURN;
	hwvars->rxring += RING_SIZE;
	if ( hwvars->rxring >= hwvars->rxring_end )
	    hwvars->rxring = hwvars->rxring_start;
    } while ( (CARDWORD( hwvars->rxring+RMD1_OFF ) & STP_BIT) == 0 );
} /* skip_incomplete_frame */

/***************** FUNCTION : skip_frame ********************************
 *
 * [OVERVIEW]	 
 *	This function throws away the current frame pointed to by RXRING.
 *	This function assumes that RXRING<start> is true.
 *	and that all rings up to <end> are owned by the driver.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to the globals area for the card.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	rxring
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	rxring_end,	rxring_start
 *
 ************************************************************************/

void
skip_frame( hwvars )
register hw_gloptr	hwvars;

{

/*----------------------------- ALGORITHM -------------------------------
 *	while not RXRING<end> and next frame starts
 *	|   RXRING->stat := constant stat for returning receive rings;
 *	|   RXRING := RXRING + ring size;
 *	|   if RXRING >= RXRING_END then RXRING := RXRING_START;
 *	end;
 *---------------------------------------------------------------------*/
    int end_of_frame = 0;

#ifdef DEBUG    
    printf(">>>> skip_frame <<<<\n");
#endif /* DEBUG */   

    /* assume rxring points to the start of the packet */
    NS_ASSERT( (CARDWORD( hwvars->rxring+RMD1_OFF ) & STP_BIT) != 0, 
	   "E_HW_NOT_STP4");

    while (!end_of_frame) {
    	if ( CARDWORD( hwvars->rxring+RMD1_OFF ) & ENP_BIT ) 
		end_of_frame = 1;

	CARDWORD( hwvars->rxring+RMD3_OFF ) = RMD3_RETURN;
	CARDBYTE( hwvars->rxring+RMD1_OFF ) = RMD1_RETURN;
	hwvars->rxring += RING_SIZE;
	if ( hwvars->rxring >= hwvars->rxring_end )
	    hwvars->rxring = hwvars->rxring_start;

	if ( !end_of_frame  &&
	     ( (CARDWORD( hwvars->rxring+RMD1_OFF ) & OWN_BIT) ||
	       (CARDWORD( hwvars->rxring+RMD1_OFF ) & STP_BIT ) ) )
		end_of_frame = 1;
    } /* while */ 
} /* skip_frame */

/***************** FUNCTION : scan_frame ********************************
 *
 * [OVERVIEW]	 
 *	This function returns scans down the receive rings starting
 *	with <ring>, if a valid frame exists, this function will
 *	return the size of the frame and rstat will be the accumulated 
 *	statistic.  Else this function will return a negative number:
 *	NO_FRAME	 -- the driver owns no frames to process (yet)
 *	INCOMPLETE_FRAME -- the frame in the queue is truncated.
 *
 * [INPUT PARAMETERS]
 *	hwvars	  -- points to the globals area for the card.
 *	ring	  -- points to the start of the ring.
 *
 * [OUTPUT PARAMETERS]
 *	rstat	  -- contains the "or-ed" status of all RMD1 in the
 *		     frame (if the function returns true, else it is
 *		     undefined).
 *	size	  -- contains the frame size (including CRC) of the
 *		     frame only if the function returns true and
 *		     that the frame is complete (not condition 1 above).
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	<None>
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	rxring_end,	rxring_start
 *
 ************************************************************************/

#define NO_FRAME		-1
#define INCOMPLETE_FRAME	-2

word
scan_frame( hwvars, ring, rstat )
register hw_gloptr	hwvars;
register unsbyte *	ring;
	 unsword *	rstat;

{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	ored_rstat -- accumulates the status up to (but not including)
 *		      the current ring.	 Technically, this is not needed,
 *		      but it is put there as a safety feature.
 *	stat	   -- current status
 *---------------------------------------------------------------------*/
	
	register unsword	ored_rstat;
	register unsword	stat;

/*----------------------------- ALGORITHM -------------------------------
 *	if ring<own by me> && !ring<enp> && !ring<err> then begin
 *	|   ored_rstat := ring->stat;
 *	|   next ring;
 *	|   while ring<own by me> && !ring<enp> && !ring<stp> && !ring<err> do
 *	|   |	ored_rstat := ored_rstat | ring->stat;
 *	|   |	next ring;
 *	|   end;
 *	|   if ring<own by me> then
 *	|   |	if ring<stp> then
 *	|   |	|   return(incomplete_frame);
 *	|   |	else
 *	|   |	|   *rstat = ored_rstat | ring->stat;
 *	|   |	|   return(ring->message_count);
 *	|   |	end
 *	|   else
 *	|   |	return(no_frame);
 *	|   end
 *	else
 *	|   if ring<own by me> then
 *	|   |	*rstat = ring->stat;
 *	|   |	return(ring->message_count);
 *	|   else
 *	|   |	return(no_frame);
 *	|   end
 *	end;
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> scan_frame <<<<\n");
#endif /* DEBUG */    

    stat = CARDWORD(ring+RMD1_OFF);


    if ( (stat & (OWN_BIT|ENP_BIT|ERR_BIT) ) == 0 ) {
	/* assume rxring points to the start of the packet */
	NS_ASSERT( (stat & STP_BIT) != 0,"E_HW_NOT_STP5");

	ored_rstat = stat;
	ring += RING_SIZE;
	if (ring >= hwvars->rxring_end) ring = hwvars->rxring_start;
	stat = CARDWORD(ring+RMD1_OFF);
	while( (stat & (OWN_BIT|ENP_BIT|ERR_BIT|STP_BIT)) == 0 ) {
	    ored_rstat |= stat;
	    ring += RING_SIZE;
	    if (ring >= hwvars->rxring_end) ring = hwvars->rxring_start;
	    stat = CARDWORD(ring+RMD1_OFF);
	}
	if ( (stat & OWN_BIT) == 0 ) {
	    if ((stat & STP_BIT) != 0) {
		return(INCOMPLETE_FRAME);
	    } else {
		/* assume rxring points to the end of the packet */
		NS_ASSERT( (stat & (ENP_BIT|ERR_BIT)) != 0,"E_HW_NOT_STP6");

		*rstat = ored_rstat | stat;
		return( CARDWORD( ring+RMD3_OFF ) & MCOUNT_FIELD );
	    }
	} else {
	    HW_QA_INCR_STAT( hwvars, LANCE_FILLING );
	    return(NO_FRAME);
	}
    } else {
	if ( (stat & OWN_BIT) == 0 ) {
	    /* assume rxring points to the start of the packet */
	    NS_ASSERT( stat & STP_BIT,"E_HW_NOT_STP7");

	    *rstat = stat;
	    return( CARDWORD( ring+RMD3_OFF ) & MCOUNT_FIELD );
	} else {
	    return(NO_FRAME);
	}
    }
}

/***************** FUNCTION : isr_proc **********************************
 *
 * [OVERVIEW]	 
 *	This is the ISR process.  Its responsibilities are to process
 *	the inbound data, and return resources if outbound data has
 *	just finished.	Afterwards, it calls receive_task if there are 
 *	any inbound or outbound data to be processed by rcv_proc.
 *
 * [INPUT PARAMETERS]
 *	myglobal -- points to the globals area for the card.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	statistics,	input_buf_head, input_buf_tail
 *	txring_empty,	tx_empty,	xmit_queued
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	output_buf_head,	rxring,		
 *	txring_end,		txring_start,
 *	tx_end,			tx_start
 *
 ************************************************************************/

void
isr_proc( myglobal )
register lan_gloptr	myglobal;

{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	hwvars	   -- points to the hardware dependent variables
 *	xmitted	   -- true if recovered some transmit memory
 *	dest_addr  -- holds destination address of incoming frame
 *	newptr	   -- used to increment TX_EMPTY and TXRING_EMPTY
 *	srcbufptr  -- points to first buffer of incoming message on card
 *	tmpbufptr -- points to destination buffer (via get_buffer)
 *	res	   -- result from scan_frame (also the frame size)
 *	rstat	   -- status bits of receive frame
 *	tmpstat	   -- same as rstat except it is NOT register
 *	tstat1	   -- transmit status word (word 1 or ring pointer)
 *	tstat2	   -- transmit status word (word 3 or ring pointer)
 *	header_size-- size of the header
 *	offset	   -- where the frame should start within the buffer
 *	i	   -- index for counting
 *	length     -- length of the incoming frame (to pass up)
 *---------------------------------------------------------------------*/

 register hw_gloptr	hwvars;
 boolean	xmitted;
 link_address	dest_addr;
 register unsbyte *	newptr;
 register unsbyte *	srcbufptr;
 register unsbyte *	dux_srcbufptr;
 struct mbuf *  tmpbufptr;
 struct mbuf *  m;
 register word		res;
 register unsword	rstat;
 unsword	tmpstat;
 register unsword	tstat1;
 register unsword	tstat2;
 unsword	header_size;
 register word		length;
 int tmplength;
 word		i;
 lan_ift        * is;
 int		pad_size;
 int  s;

 mib_xEntry     *mib_xptr;
 mib_ifEntry    *mib_ptr;

#ifdef LAN_MS
 /* measurement instrumentation */
 lan_rheader         *lan; 
 extern struct timeval   ms_gettimeofday();
 static struct {
                struct timeval tv;
                int     count;
                int     unit;
                int     protocol;
                int     addr;
               } ms_io_info;
#endif /* LAN_MS */	


/*-------------------------CONDENSED ALGORITHM---------------------------
 *	for all frames received
 *	|   check for errors in packet and update statistics
 *	|   if no errors and don't filter address and got a buffer then
 *	|   |	copy in frame
 *	|   |	put frame on list
 *	|   else
 *	|   |	discard this frame
 *	|   end
 *	end
 *
 *	for all frames transmitted
 *	|   update the statistics
 *	|   return the ring used
 *	|   return buffer area used
 *	end
 *
 *	if transmitted
 *	|   reset timer
 *	|   restart timer if frames are still on card
 *	end
 *
 *	if received any frame or output queue not empty then
 *	|   receive_task
 *	end
 *---------------------------------------------------------------------*/

/*----------------------------- ALGORITHM -------------------------------
 *	while ( SCAN_FRAME(&rstat) != no_frame ) do
 *	|   if (incomplete_frame)
 *	|   |	SKIP_BAD_FRAME;
 *	|   |	HW_INCR_STAT(NO_ENP);
 *	|   |	continue;  -- with the while
 *	|   end;
 *	|   HW_INCR_STAT(ALL_RECEIVE);
 *	|   -- check for errors in packet
 *	|   if rstat<error summary> then 
 *	|   |	if rstat<framming error> then HW_INCR_STAT(INT_FRAMMING);
 *	|   |	if rstat<crc> then HW_INCR_STAT(FCS_ERRORS);
 *	|   |	if rstat<buffer error or overflow> then HW_INCR_STAT(INT_MISS);
 *	|   |	SKIP_FRAME;
 *	|   |	continue;  -- with the while
 *	|   end;
 *	|   if (frame too long or too short) then
 *	|   |	HW_INCR_STAT(UNDELIVERABLE);
 *	|   |	SKIP_FRAME;
 *	|   |	continue;  -- with the while
 *	|   end;
 *	|   -- filter broadcast packets
 *	|   srcbufptr := RXRING->buffer_pointer;
 *	|   dest_addr := copy dest address from( srcbufptr );
 *	|   if dest_addr == individual address then
 *	|   |	HW_INCR_STAT(IND_RECEIVE);
 *	|   else if dest_addr==broadcast then
 *	|   |	if BROADCAST_STATE==FALSE then
 *	|   |	|   HW_INCR_STAT(INT_BCAST_DROP);
 *	|   |	|   SKIP_FRAME;
 *	|   |	|   continue;  -- with the while 
 *	|   |	end; -- empty else clause
 *	|   else if FIND_ADDRESS(ml_active, dest_addr)->next == NIL then
 *	|   |	HW_INCR_STAT(INT_MCAST_DROP);
 *	|   |	SKIP_FRAME;
 *	|   |	continue;  -- with the while 
 *	|   |	end;
 *	|   end;
 *	|   -- copy the frame
 *	|   -- determine even/odd header length
 *	|   if type_field is Ethernet then
 *	|   |	offset := driver overhead size;
 *	|   |	length := res - CRC_SIZE;
 *	|   else  -- it is ieee802
 *	|   |	if dsap is HP expansion addressing then
 *	|   |	|   offset := driver overhead size;
 *	|   |	else
 *	|   |	|   offset := driver overhead size + 1;
 *	|   |	end;
 *	|   |	length := read length field from (srcbufptr) + constant;
 *	|   |	if (bad length) then
 *	|   |	|   HW_INCR_STAT(UNDELIVERABLE);
 *	|   |	|   SKIP_FRAME;
 *	|   |	|   continue;  -- with the while
 *	|   |	end;
 *	|   -- copy the frame
 *	|   end;
 *	|   -- get buffer
 *	|   destbufptr := GET_BUFFER;
 *	|   if destbufptr == nil then
 *	|   |	HW_INCR_STAT(UNDELIVERABLE);
 *	|   |	SKIP_FRAME;
 *	|   |	continue;  -- with the while 
 *	|   end; 
 *	|   COPYIN_FRAME( destbufptr+offset );
 *	|   -- put frame on inbound queue
 *	|   destbufptr->frame_offset := offset;
 *	|   destbufptr->frame_size := res;
 *	|   destbufptr->next := nil;
 *	|   if INPUT_BUF_HEAD == nil then
 *	|   |	INPUT_BUF_HEAD := destbufptr;
 *	|   |	INPUT_BUF_TAIL := destbufptr;
 *	|   else
 *	|   |	INPUT_BUF_TAIL->next := destbufptr;
 *	|   |	INPUT_BUF_TAIL := destbufptr;
 *	|   end;
 *	end;
 *
 *	xmitted := FALSE;
 *	while TXRING_EMPTY<own> == my owner and XMIT_QUEUED > 0 do
 *	|   -- update transmit statistics
 *	|   tstat1 := TXRING_EMPTY->word1;
 *	|   tstat2 := TXRING_EMPTY->word3;
 *	|   if tstat1<error summary> then
 *	|   |	HW_INCR_STAT(UNSENDABLE);
 *	|   |	if tstat2<underflow> then
 *	|   |	|  stop and restart the card
 *	|   |	|  reset the timer
 *	|   |	end;
 *	|   |	if tstat2<late collision> then HW_INCR_STAT(INT_LATE);
 *	|   |	if tstat2<loss carrier> then HW_INCR_STAT(LOST_CARRIER);
 *	|   |	if tstat2<retry error> then 
 *	|   |	|   HW_INCR_STAT(INT_RETRY);
 *	|   |	|   HW_INCR_STAT(TX_COLLISIONS) sixteen times;
 *	|   |	|   LAST_TDR := tstat2<tdr bits>;
 *	|   |	end;
 *	|   else
 *	|   |	HW_INCR_STAT(ALL_TRANSMIT);
 *	|   end;
 *	|   if tstat2<buffer error> then
 *	|   |	CARD_STATE = txbuffer_error;
 *	|   |	reset card;
 *	|   |	disable_interrupts;
 *	|   |	return;
 *	|   end;
 *	|   if not tstat2<retry error> then
 *	|   |	if tstat1<more> then HW_INCR_STAT(TX_COLLISIONS) twice;
 *	|   |	if tstat1<one> then HW_INCR_STAT(TX_COLLISIONS);
 *	|   end;
 *	|   if tstat1<deferred> then HW_INCR_STAT(INT_DEFERRED);
 *	|
 *	|   -- return the transmit buffer to transmit area
 *	|   newptr := TXRING_EMPTY->buffer_pointer + 
 *	|		TXRING_EMPTY->byte_count;   
 *	|   if newptr >= TX_END then 
 *	|   |	TX_EMPTY := TX_START;
 *	|   else
 *	|   |	TX_EMPTY := newptr;
 *	|   end;
 *	|
 *	|   -- return the transmit ring 
 *	|   newptr := TXRING_EMPTY + ring size;
 *	|   if newptr == TXRING_END then 
 *	|   |	TXRING_EMPTY := TXRING_START;
 *	|   else
 *	|   |	TXRING_EMPTY := newptr;
 *	|   end;
 *	|   XMIT_QUEUED--;
 *	|   xmitted := TRUE;
 *	end;
 *
 *	if xmitted && TIMER_SET then
 *	|   UNTIMEOUT;
 *	|   if XMIT_QUEUED > 0 then
 *	|   |	TIMEOUT( XMIT_TIMER );
 *	|   else
 *	|   |	TIMER_SET = FALSE;
 *	|   end;
 *	end;
 *
 *	if INPUT_BUF_HEAD != NIL or (xmitted and OUTPUT_BUF_HEAD != NIL) then 
 *	|   RECEIVE_TASK;
 *	end;
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
 printf(">>>> isr_proc <<<<\n");
#endif /* DEBUG */   
	
 hwvars = Myhw;
 is = (lan_ift *) myglobal->lan_ift_ptr;	

 mib_xptr = is->mib_xstats_ptr;
 mib_ptr  = &(is->mib_xstats_ptr->mib_stats);

 while ((res = scan_frame(hwvars,hwvars->rxring,&tmpstat)) != NO_FRAME ) 
    {

/*
 *	throw away incompletely received frames
 */

     if ( res == INCOMPLETE_FRAME )
	{
	 hw_incr_stat( hwvars, INT_MISS );
	 HW_QA_INCR_STAT( hwvars, NO_ENP );
	 skip_incomplete_frame( hwvars );
	 is->is_ac.ac_if.if_ierrors++;
	 mib_xptr->if_DInErrors++;
	 continue; /* skip to the end of the while loop */
	}
	    

     hw_incr_stat( hwvars, ALL_RECEIVE ); /* why keep this? pgc */
     /* if a pkt came in thru the filter incr input pkt count */
     ((is->is_ac).ac_if).if_ipackets++;

/*
 *	check for errors in receive
 */

     rstat = tmpstat;

     if ( rstat & ERR_BIT )
	{
	 if ( rstat & CRC_BIT  ) hw_incr_stat( hwvars, FCS_ERRORS );
	 if ( rstat & FRAM_BIT ) hw_incr_stat( hwvars, INT_FRAMMING );
	 if ( rstat & (RXBUFF_BIT | OFLO_BIT))
	    {
	     hw_incr_stat( hwvars, INT_MISS );
	     HW_COND_INCR_STAT( rstat & RXBUFF_BIT, hwvars, INT_RXBUFFER );
	     HW_COND_INCR_STAT( rstat & OFLO_BIT, hwvars, INT_OVERFLOW );
	    }
         HW_QA_INCR_STAT( hwvars, SKIP_BAD_COUNT );
	 skip_frame( hwvars );
	 is->is_ac.ac_if.if_ierrors++;
	 mib_xptr->if_DInErrors++;
	 continue; /* skip to the end of the while loop */
	}

/*
 *	throw away frames that are too long or too short
 *	(the LANCE will automatically throw away most frames that are
 *	too short, but there is a few corner cases where it will allow
 *	a runt frame to come in)
 */
	
     if ( res > LAN_PACKET_SIZE+CRC_SIZE || res < LAN_MIN_LEN+CRC_SIZE)
	{
	 HW_QA_INCR_STAT( hwvars, LENGTH_ERROR );
	 skip_frame( hwvars );
	 is->is_ac.ac_if.if_ierrors++;
	 mib_xptr->if_DInErrors++;
	 continue; /* skip to the end of the while loop */
	}

/*
 *	filter unwanted multicast and broadcast frames
 */

     srcbufptr = READADDR( hwvars->rxring );

     /* Assume that the buffer is on an even boundary */
     NS_ASSERT( ((unsdword) srcbufptr & 1) == 0,"E_HW_SRC_NOT_EVEN");

     LAN_COPY_ADDRESS( srcbufptr + LAN_DA_OFF, dest_addr );
     if ( INDIVIDUAL(dest_addr) )
	{
	 if ( !SAME_ADDRESS(dest_addr, myglobal->local_link_address) )
	    {
	     HW_QA_INCR_STAT( hwvars, INDIVIDUAL_DROP );
	     skip_frame( hwvars );
	     is->is_ac.ac_if.if_ierrors++;
	     mib_xptr->if_DInErrors++;
	     continue; /* skip to the end of the while loop */
	    }
	 HW_QA_INCR_STAT( hwvars, IND_RECEIVE );
	}
     else
	if ( BROADCAST(dest_addr) )
	   {
	    if ( myglobal->broadcast_state == FALSE )
	       {
		HW_QA_INCR_STAT( hwvars, INT_BCAST_DROP );
		skip_frame( hwvars );
		is->is_ac.ac_if.if_ipackets--; /*  shouldn't have incremented
                                                   ipackets count earlier*/
	        mib_xptr->if_DInErrors++;
		continue; /* skip to the end of the while loop */
	       }
	    HW_QA_INCR_STAT( hwvars, BCAST_TAKEN );
	   }
	else /* must be multicast */ 
	   {
            if(!hw_find_mcaddr(is,(unsigned char *)dest_addr)) {
		HW_QA_INCR_STAT( hwvars, INT_MCAST_DROP );
		skip_frame( hwvars );
		is->is_ac.ac_if.if_ipackets--; /* shouldn't have incremented 
                                                  ipackets count earlier*/
	        mib_xptr->if_DInErrors++;
		continue; /* skip to the end of the while loop */
	       } 
	    HW_QA_INCR_STAT( hwvars, MCAST_TAKEN );
	   }

/*
 *	determine odd/even boundary 
 */

     if ( IS_ETHERNET(CARDWORD( srcbufptr + LAN_TYPE_OFF )) )
	{
	 length = res - CRC_SIZE;
	 pad_size = 6;
	} 
     else /* must be IEEE802 */ {
      if ((CARDBYTE( srcbufptr + IEEE_DSAP_OFF ) == IEEESAP_HP) ||
	  (CARDBYTE( srcbufptr + IEEE_DSAP_OFF ) == IEEESAP_NM))
	 {
	  header_size = IEEE_EXP_SIZE;
	  pad_size = 4;
	 } 
      else
	 {
	  header_size = IEEE_SIZE;
	  pad_size = 7;
	 }

/*
 *	    The actual size of the frame for IEEE802 is its length field.
 *	    We should check if this length field is valid.
 */
      length = CARDWORD( srcbufptr+IEEE_LEN_OFF ) + IEEE_LEN_ADD;
      if ( length > res || length < header_size )
	 {
	  HW_QA_INCR_STAT( hwvars, LENGTH_ERROR );
	  skip_frame( hwvars );
          mib_xptr->if_DInErrors++;
	  continue; /* skip to the end of the while loop */
	 }
     }


/* #ifdef DUX */
/*
 * If a DUX packet, call DUX protocol's recv routines
 * This is an inline, trimmed is_dux_packet().
 */
     
     dux_srcbufptr = READADDR( hwvars->rxring );
     if ( (CARDWORD(dux_srcbufptr + LAN_TYPE_OFF)) < ETH_START_ADR )
    	{ 
	 
	 if(((CARDBYTE(dux_srcbufptr + IEEE_DSAP_OFF)) == (unsbyte)IEEE802_EXP_ADR) &&
	    ((CARDBYTE(dux_srcbufptr + IEEE_CONTROL_OFF)) == (unsbyte)NORMAL_FRAME) &&  
	    ((*( (u_short *) (dux_srcbufptr + IEEE_EXP_OFF))) == (u_short)DUX_PROTOCOL))
	    {
	     if( duxproc[DUX_RECV_ROUTINES] != dux_nop )
		{
		 DUXCALL(DUX_RECV_ROUTINES)(hwvars);
		 length = CARDWORD( srcbufptr+IEEE_LEN_OFF ) + IEEE_LEN_ADD;
		 mib_ptr->ifInOctets += length; /*includes everything*/ 
		}
	     else
		{
/************** obsolete; use lanc_ift->UNKNOWN_PROTO instead
		 hw_incr_stat(hwvars,NO_CSR_FOUND);
**************/
is->UNKNOWN_PROTO++;
/*************/
		 skip_frame(hwvars);
		 is->is_ac.ac_if.if_ierrors++;
	         mib_xptr->if_DInErrors++;
		}
	     continue; /*exit while loop*/
	     
	    }
     	}

/*
 *	get the architecture buffer, the size needed is res
 *	    Note that res is used instead of length since everything
 *	    is copied in including any padding and the CRC.
 */
     s=splimp();
     
     if ( get_buffer(is, res+pad_size, &tmpbufptr) !=E_NO_ERROR )
	{
	 splx(s);
	 hw_incr_stat( hwvars, UNDELIVERABLE );
	 skip_frame( hwvars ); /* s/w err; dont incr if_ierrors for this */
         mib_xptr->if_DInDiscards++;
	 continue;
	}
     splx(s);
     /* add byte alignments here if need be .........prabha */
     m = tmpbufptr;
     
     tmpbufptr->m_len = pad_size;
     copyin_frame( hwvars, tmpbufptr);
     tmpbufptr->m_off += pad_size;
     tmpbufptr->m_len -= pad_size;

     mib_ptr->ifInOctets += length; /* CRC included in length */

     tmplength = length;
	/* Strip off the CRC here and padding here.  It's easier. */
     while (length > 0)
	{
	 length -= tmpbufptr->m_len;
	 if (length <= 0)
	    {
	     if (tmpbufptr->m_next != 0)
		{

		 m_freem(tmpbufptr->m_next);
		 tmpbufptr->m_next = 0;
		}
	     tmpbufptr->m_len += length;
	     break;
	    }
	 if ((tmpbufptr = tmpbufptr->m_next) == 0)
	    break;  /* should never happen */
	}
     s=splimp();

#ifdef LAN_MS 
      /* measurement instrumentation */
      if (ms_turned_on(ms_lan_read_queue))
        {
        lan = (lan_rheader *) (mtod(m, struct mbuf *));
        ms_io_info.tv = ms_gettimeofday();
        if (lan->type < MIN_ETHER_TYPE)
          {
          ms_io_info.protocol = 1; /*IEEE packet*/
          ms_io_info.count = lan->type;
          ms_io_info.addr = ((struct ieee8023_hdr *)lan)->dsap;
          }
        else
          {
          ms_io_info.protocol = 0; /*Ethernet packet*/
          ms_io_info.addr = lan->type;
          ms_io_info.count = tmplength;
          }
        ms_io_info.unit = is->is_if.if_unit;
        ms_data(ms_lan_read_queue,&ms_io_info,sizeof(ms_io_info));
        }
#endif /* LAN_MS */

#ifdef	NET_QA
    if (arp_ptr) {
        ns_trace_link(is, TR_LINK_INBOUND, m, NS_LS_LAN0);
        hw_rint_ics(is, arp_ptr);
	arp_ptr = NULL;
    }
#endif /* NET_QA */

     ns_trace_link(is, TR_LINK_INBOUND, m, NS_LS_LAN0);
     hw_rint_ics(is,m);
     splx(s);
    } /* end of while */


    xmitted = FALSE;
    while (((CARDWORD(hwvars->txring_empty+TMD1_OFF) & OWN_BIT) == MY_OWN )
	  && hwvars->xmit_queued > 0 ) {

/*
 *	update transmit statistics
 */

	tstat1 = CARDWORD( hwvars->txring_empty+TMD1_OFF );


	/* the ENP and STP bits should both be set */
	NS_ASSERT( tstat1 & ENP_BIT,"E_HW_NOT_ENP");
	NS_ASSERT( tstat1 & STP_BIT,"E_HW_NOT_STP8");

	tstat2 = CARDWORD( hwvars->txring_empty+TMD3_OFF );

	if (tstat1 & ERR_BIT) {
	    hw_incr_stat( hwvars, UNSENDABLE );
	    if (tstat2 & UFLO_BIT) {
		HW_QA_INCR_STAT( hwvars, INT_UNDERFLOW );
		if ( !hw_softreset( hwvars ) ) {
		    return;
		}
	    }
	    if (tstat2 & LCOL_BIT) hw_incr_stat( hwvars, INT_LATE );
	    if (tstat2 & LCAR_BIT) hw_incr_stat( hwvars, LOST_CARRIER );
	    if (tstat2 & RTRY_BIT) {
		hw_incr_stat( hwvars, INT_RETRY );
                is->is_ac.ac_if.if_collisions++;
		for (i=0; i<16; i++) {
                     hw_incr_stat( hwvars, TX_COLLISIONS );
                }
		hwvars->last_tdr = tstat2 & TDR_FIELD;
	    }
	} else {
	    hw_incr_stat( hwvars, ALL_TRANSMIT );
	}
	if ( tstat2 & TXBUFF_BIT ) {
	    hwvars->card_state = TXBUFFER_ERROR;
	    CARDBYTE( hwvars->card_base + RESET_REG ) = 1;
	    CARDBYTE( hwvars->card_base + INTR_REG ) = INTR_DISABLE;
	    return;
	}
	if ( (tstat2 & RTRY_BIT) == 0 ) {
	    if (tstat1 & MORE_BIT) {
                is->is_ac.ac_if.if_collisions++;
		hw_incr_stat( hwvars, TX_COLLISIONS );
		hw_incr_stat( hwvars, INT_MORE );
	    }
	    if (tstat1 & ONE_BIT) {
		hw_incr_stat( hwvars, TX_COLLISIONS );
                is->is_ac.ac_if.if_collisions++;
		hw_incr_stat( hwvars, INT_ONE );
	    }
	}
	if (tstat1 & DEF_BIT) hw_incr_stat( hwvars, INT_DEFERRED );

/*
 *	return transmit buffer to the transmit area
 */
	
	newptr = READADDR( hwvars->txring_empty ) - 
		 CARDSWORD( hwvars->txring_empty + TMD2_OFF );
		 /* subtract since TMD2 is defined to be negative */
/*
 *	Tricky stuff here:  note that because of the allocation (in hw_send) 
 *	will always leave at least one byte at the end of the buffer, therefore 
 *	you don't need to check if "newptr" is at the end of buffer!
 */
	NS_ASSERT( newptr < hwvars->tx_end,"E_HW_TX_PAST_END2");
	hwvars->tx_empty = newptr;

/*
 *	return transmit ring
 */
	
	newptr = hwvars->txring_empty + RING_SIZE;
	if (newptr >= hwvars->txring_end)
	    hwvars->txring_empty = hwvars->txring_start;
	else
	    hwvars->txring_empty = newptr;

	hwvars->xmit_queued--;
	xmitted = TRUE;
    }

#if NEED_TIMER

    if ( xmitted ) {
	ALL_PROTECT( hwvars, &save_int );
	    if ( hwvars->timer_set ) {
		clear_timeout( &hwvars->timeout_loc );
		if ( hwvars->xmit_queued > 0 ) {
		    timeout( xmit_timer, hwvars, XMIT_TIMEOUT, 
			     &hwvars->timeout_loc );
		} else {
		    hwvars->timer_set = FALSE;
		}
	    }
	ALL_UNPROTECT( hwvars, save_int );
    }

#endif /* NEED_TIMER */
    if (xmitted)
    {
     int temp_s;
        temp_s = splimp();
        hw_send(is); /* SK: make sure this is ok for DUX! */
        splx(temp_s);
        if( !(((landrv_ift *)is)->lan_vars.flags & LAN_NO_TX_BUF)){
            DUXCALL(DUX_HW_SEND)();
        }
    }
}

/***************** FUNCTION : hw_isr ************************************
 *
 * [OVERVIEW]	 
 *	This is the hardware interrupt process.	 Its responsibilities
 *	are to update the statistics and to trigger the ISR process.
 *
 * [INPUT PARAMETERS]
 *	intrec	--  system interrupt structure.	 the misc field is used
 *		    to point to the global data area for the driver.
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	card_state,	statistics
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_base
 *
 ************************************************************************/

void
hw_isr( intrec )
struct interrupt *intrec;

{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	myglobal  -- points to the global variables
 *	hwvars	  -- points to the hardware dependent variables
 *	stat	  -- keeps the LANCE status word
 *	tmpstat	  -- same as stat except NOT register
 *	base_addr -- points to the base address of the card
 *	save_int  -- save interrupt status (for ALL_PROTECT)
 *---------------------------------------------------------------------*/
	register lan_gloptr	myglobal;
	register hw_gloptr	hwvars;
	register unsword	stat;
		 unsword	tmpstat;
	register unsbyte *	base_addr;

#if NEED_TIMER
		 all_token	save_int;
#endif /* NEED_TIMER */



/*----------------------------- ALGORITHM -------------------------------
 *	-- assume system interrupt level will be at card interrupt level
 *	
 *	myglobal := find card's globals area
 *	if on card babble interrupt then
 *	|   reset card
 *	|   restart timeout
 *	end;
 *
 *	stat := lance status word;
 *	lance status word := lance interrupt acknowledge word;
 *	
 *	if stat<error summary> then
 *	|   if stat<miss packet> then HW_INCR_STAT(INT_MISS);
 *	|   if stat<collision error> then HW_INCR_STAT(NO_HEART_BEAT);
 *	|   if stat<memory error> then
 *	|   |	CARD_STATE := memory_error;
 *	|   |	reset card;
 *	|   |	disable interrupts;
 *	|   |	return;
 *	|   end;
 *	end;
 *	
 *	SW_TRIGGER( ISR_PROC, myglobal );
 *	
 *	-- assume system interrupts will be turned on
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> hw_isr <<<<\n");
#endif /* DEBUG */    

    myglobal = (lan_gloptr) intrec->temp;
    hwvars = Myhw;
    base_addr = hwvars->card_base;

/*
 *  check the babble timer interrupt
 */
    
    if ( CARDBYTE(base_addr+INTR_REG) & BABBLE_BIT ) {
	HW_QA_INCR_STAT( hwvars, HW_XMIT_TIMER );
	
	if ( !hw_softreset(hwvars) ) {
	    return;
	}

#if NEED_TIMER

	ALL_PROTECT( hwvars, &save_int );
	    if ( hwvars->timer_set ) {
		clear_timeout( &hwvars->timeout_loc );
		timeout( xmit_timer, hwvars, XMIT_TIMEOUT,
			 &hwvars->timeout_loc );
	    }
	ALL_UNPROTECT( hwvars, save_int );

#endif /* NEED_TIMER */

    }

/*
 *  read status byte (with timeout poll of ACK_BIT)
 */

    if ( ! read_rdp( hwvars, &tmpstat ) ) {
	return;
    }

    stat = tmpstat;

    /* Assume that IDON=0, RXON=1, TXON=1 */
    NS_ASSERT( (stat & IDON_BIT) == 0,"E_HW_IDON_SET");

#ifdef NOTDEF /* don't panic the system just because these are not set... */
    NS_ASSERT( stat & RXON_BIT,"E_HW_NOT_RXON");
    NS_ASSERT( stat & TXON_BIT,"E_HW_NOT_TXON");
#else
    if(!(stat & RXON_BIT) || !(stat & TXON_BIT)) {
        hw_softreset(hwvars);
        read_rdp(hwvars, &tmpstat);
        if(!(tmpstat & RXON_BIT) || !(tmpstat & TXON_BIT)) {
                drv_restart(myglobal,0);
                return;
        }
    }
#endif



/*
 *  write status byte to acknowledge interrupts (with timeout poll of ACK_BIT)
*/

    HW_QA_INCR_STAT( hwvars, ACK );
    if ( !write_rdp(hwvars, stat | CSR0_INTR_ACK_BITS ) ) {
	return;
    }

    if ( stat & ERROR_BIT ) {
	if ( stat & MISS_BIT ) hw_incr_stat( hwvars, INT_MISS );
	if ( stat & CERR_BIT ) hw_incr_stat( hwvars, NO_HEART_BEAT );
	if ( stat & MERR_BIT ) {
	    hwvars->card_state = MEMORY_ERROR;
	    CARDBYTE( base_addr + RESET_REG ) = 1;
	    CARDBYTE( base_addr + INTR_REG ) = INTR_DISABLE;
	    return;
	}
	HW_COND_INCR_STAT( stat & BABL_BIT, hwvars, BABBLE_COUNT );
    }

	HW_QA_INCR_STAT( hwvars, TRIGGER_ISR );
	
	sw_trigger( &hwvars->intloc2, isr_proc, myglobal,
		   LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL+1 );
}

/***************** FUNCTION : hw_reset_card *****************************
 *
 * [OVERVIEW]	 
 *	This function will be called once at network powerup.  It will 
 *	initialize all data which need to be initialized only once.
 *	Interrupts are assumed to be disabled, and timer process
 *	cannot access the card.
 *	This function will link the interrupt service routine into the 
 *	host operating system.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	card_location -- the select code of the card
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	card_base,	init_block,
 *	rxring_start,	rxring_end,
 *	txring_start,	txring_end,
 *	tx_start,	tx_end,
 *	card_state,	intloc,
 *	STATISTICS,	last_tdr
 *	
 * [GLOBAL VARIABLES ACCESSED]
 *	isc_table[] (kernel)
 *
 ************************************************************************/

error_number
hw_reset_card( myglobal, card_location )
lan_gloptr		myglobal;
register word		card_location;

{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	hwvars -- points to the hardware globals
 *	s   -- index in statistic array
 *	isc -- points to the OS isc table entry
 *---------------------------------------------------------------------*/

	register hw_gloptr	hwvars;
	register word	s;
	register struct isc_table_type *isc;
	lan_ift *lanc_ift_ptr;



/*----------------------------- ALGORITHM -------------------------------
 *	-- and interrupts are turned off
 *
 *	if (card id register != lan card id) and 
 *	   (wrong interrupt level) then
 *	|   CARD_STATE := no_card_state;
 *	|   return( e_wrong_card_type );
 *	end;
 *	CARD_BASE := read from OS structure;
 *	isrlink( hw_isr );
 *	initialize INTLOC;
 *	initialize TIMEOUT_LOC;
 *	initialize CHIP_VERSION;
 *
 *	XMIT_QUEUED = 0;
 *	TIMER_SET = FALSE;
 *	STATISTICS := 0;
 *	LAST_TDR := 0;
 *	INIT_BLOCK := some function of (card_location);
 *	RXRING_START := INIT_BLOCK + some offset;
 *	RXRING_END := RXRING_START + some offset;
 *	TXRING_START := INIT_BLOCK + some offset;
 *	TXRING_END := TXRING_START + some offset;
 *	TX_START := INIT_BLOCK + some offset;
 *	TX_END := TX_START + some offset;
 *	CARD_STATE := initializing;
 *	return(e_no_error);
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> hw_reset_card <<<<\n");
#endif /* DEBUG */   

    lanc_ift_ptr = (lan_ift *)myglobal->lan_ift_ptr;

    hwvars = Myhw;
    hwvars->drvglob = (anyptr) myglobal;

    if ( card_location < MINISC || card_location >= EXTERNALISC ) {
	hwvars->card_state = NO_CARD_STATE;
	return( E_WRONG_CARD_TYPE );
    }

    isc = isc_table[ card_location ];
    if ( isc == NIL || isc->card_type != HP98643 || 
	 isc->int_lvl != LAN_INTR_LEVEL ) {
	hwvars->card_state = NO_CARD_STATE;
	return( E_WRONG_CARD_TYPE );
    }
    hwvars->card_base = (unsbyte *) isc->card_ptr;
    hwvars->select_code = card_location;
    lanc_ift_ptr->hdw_path[0] = card_location;

    /* assume that the OS has setup everything correctly */
    NS_ASSERT( (CARDBYTE( hwvars->card_base+ID_REG ) & ID_FIELD) == LAN_CARD_ID,
	   "E_HW_WRONG_ADDR");
    NS_ASSERT( ((CARDBYTE( hwvars->card_base+INTR_REG ) >> 4) & 0x3) + 3 
	    == LAN_INTR_LEVEL,"E_HW_WRONG_LEVEL");
    
/*
 *  The following is an OS call.  The misc parameter (char 0) is not used.
 *  The temp parameter is used to point to the global area.  hw_isr will
 *  look into the interrupt structure to extract this pointer.
 */

    isrlink( hw_isr, LAN_INTR_LEVEL, hwvars->card_base+INTR_REG, 
	     INTR_MASK, INTR_VALUE, (char) 0, (int) myglobal);

    hwvars->intloc.proc = NIL;
    hwvars->intloc2.proc = NIL;

#if NEED_TIMER

    hwvars->timeout_loc.b_link = NIL;
    hwvars->timer_set = FALSE;

#endif /* NEED_TIMER */

    hwvars->xmit_queued = 0;

    drv_save_stats (lanc_ift_ptr);

    for (s = START_STAT_VAL; s < MAX_STATISTICS; s++) {
	hw_reset_stat(lanc_ift_ptr,s);
    }
    hwvars->last_tdr = 0;
    hwvars->init_block = hwvars->card_base + MEMORY_BASE;
    hwvars->rxring_start = hwvars->init_block + RXRING_BASE;
    hwvars->rxring_end = hwvars->rxring_start + RXRING_SIZE;
    hwvars->txring_start = hwvars->init_block + TXRING_BASE;
    hwvars->txring_end = hwvars->txring_start + TXRING_SIZE;
    hwvars->tx_start = hwvars->init_block + TX_BASE;
    hwvars->tx_end = hwvars->tx_start + TX_SIZE;
    hwvars->card_state = CARD_INIT;
	
    return( E_NO_ERROR );
}


/***************** FUNCTION : hw_startup ********************************
 *
 * [OVERVIEW]	 
 *	This function will attempt to bring the hardware into the
 *	normal active state.  It will initialize all data which
 *	need to be initialized at every restart; this includes all of
 *	card's shared memory since self-test can wipe it out.
 *	This function will assume that interrupts are disabled upon
 *	entry to the routine, and will enable interrupts if the reset
 *	is successful.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	addr   -- local (individual) link address
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	txring_empty,	txring_fill,
 *	tx_empty,	tx_fill,
 *	rxring
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	rxring_start,	txring_start,	tx_start
 *	init_block
 *
 ************************************************************************/

error_number
hw_startup( hwvars, addr )
register hw_gloptr	hwvars;
	 link_address	addr;

{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	ring	--  points to tx and rx rings for initialization
 *	buf	--  points to the rx buffers associated with each rx ring
 *	ringnum --  the number of the ring initializing
 *	stat	--  lance status word to check initialization done
 *	timerec --  record used to get the time
 *	startime--  initial timeout value
 *---------------------------------------------------------------------*/

	register unsbyte *	ring;
	register unsbyte *	buf;
	register word		ringnum;
/*               int            saved_priority; */
		 unsword	stat;
		 unsdword	startime;
		 time_stamp	timerec;


/*----------------------------- ALGORITHM -------------------------------
 *	-- reset (and stop) the card
 *	reset card;
 *	-- initialize the data
 *	lance register 1 and 2 := INIT_BLOCK;
 *	lance register 3 := constant;
 *	INIT_BLOCK->local_address := addr;
 *	INIT_BLOCK->(rest of block) := regular init block constant;
 *	-- initialize the receive rings
 *	for ringnum := 0 to number of receive rings-1 do
 *	|   ring := calculate address from (ringnum, RXRING_START);
 *	|   ring->stat := constant for returning receive ring;
 *	|   ring->buffer_size := constant buffer size;
 *	|   ring->message_size := 0;
 *	|   ring->buffer_pointer := calculate address from ringnum;
 *	end;
 *	RXRING := RXRING_START;
 *	-- initialize the transmit rings
 *	for ringnum := 0 to number of transmit rings-1 do
 *	|   ring := calculate address from (ringnum, TXRING_START);
 *	|   ring<own> := owned by me;
 *	end;
 *	TXRING_EMPTY := TXRING_START;
 *	TXRING_FILL := TXRING_START;
 *	-- initialize the transmit area;
 *	TX_FILL := TX_START;
 *	TX_EMPTY := TX_START;
 *	-- startup the card
 *	lance register 0 := lance startup constant;
 *
 *	wait (with timeout) for initialization done interrupt
 *	if timeout then
 *	|   CARD_STATE := reset_error;
 *	|   reset card;
 *	|   return(e_hardware_failure);
 *	end;
 *	lance status word := lance acknowledge initialize (only)
 *	enable interrupts;
 *	return(e_no_error);
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> hw_startup <<<<\n");
#endif /* DEBUG */   

/*
 *  reset the card
 */

    CARDBYTE( hwvars->card_base+RESET_REG ) = 1;

#if NEED_TIMER

/*
 *  Cancel timers.
 */

    if (hwvars->timer_set) {
	clear_timeout( &hwvars->timeout_loc );
    }
    hwvars->timer_set = FALSE;

#endif /* NEED_TIMER */

    hwvars->xmit_queued = 0;

/*
 *  tell LANCE the location of the initialization block
 *  (set up LANCE registers 1 and 2)
 */

    CARDWORD( hwvars->card_base + LANCERAP_REG ) = 1;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");
    CARDWORD( hwvars->card_base + LANCERDP_REG ) = 
	(unsdword) hwvars->init_block & 0x0000FFFF;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");
	
    CARDWORD( hwvars->card_base + LANCERAP_REG ) = 2;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");
    CARDWORD( hwvars->card_base + LANCERDP_REG ) = 
	((unsdword) hwvars->init_block >> 16) & 0x00FF;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");

/*
 *  initialize LANCE register 3
 */

    CARDWORD( hwvars->card_base + LANCERAP_REG ) = 3;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");
    CARDWORD( hwvars->card_base + LANCERDP_REG ) = CSR3_INIT;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");

/*
 *  initialize INIT BLOCK
 */

    /* assume that the ring address are on a quadword boundary	*/
    /* and that the address passed in is individual		*/
    NS_ASSERT( ((unsdword) hwvars->rxring_start & 0x7) == 0,"E_HW_RXQUADWORD");
    NS_ASSERT( ((unsdword) hwvars->txring_start & 0x7) == 0,"E_HW_TXQUADWORD");
    NS_ASSERT( INDIVIDUAL( addr ),"E_HW_NOT_INDIVIDUAL");

    CARDWORD( hwvars->init_block+MODE_OFF ) = MODE_INIT;
/* the following temporarily commented out until HW_PROMISCUOUS is removed from
   ../h/ndtrace.h. HW_PROMISCUOUS MUST BE A cc line OPTION ..prabha*/

/* #ifdef HW_PROMISCUOUS */
    /* CARDWORD( hwvars->init_block+MODE_OFF ) = PROM_MODE_INIT; */
/* #endif */ /* HW_PROMISCUOUS */

    COPY_SW_ADDRESS( addr, hwvars->init_block+PADR_OFF );
    CARDWORD( hwvars->init_block+LADRF0_OFF ) = LADRF0_INIT;
    CARDWORD( hwvars->init_block+LADRF1_OFF ) = LADRF1_INIT;
    CARDWORD( hwvars->init_block+LADRF2_OFF ) = LADRF2_INIT;
    CARDWORD( hwvars->init_block+LADRF3_OFF ) = LADRF3_INIT;
    WRITEADDR( hwvars->init_block+RDRA_OFF, hwvars->rxring_start );
    CARDBYTE( hwvars->init_block+RLEN_OFF ) = RLEN_INIT;
    WRITEADDR( hwvars->init_block+TDRA_OFF, hwvars->txring_start );
    CARDBYTE( hwvars->init_block+TLEN_OFF ) = TLEN_INIT;

/*
 *  initialize the receive rings
 */
    
    for (ringnum = 0; ringnum < RXRING_NUM; ringnum++ ) {
	ring = hwvars->rxring_start + ringnum*RING_SIZE;
	buf  = (unsbyte *) (hwvars->init_block + RX_BASE + RXBUFFER_SIZE*ringnum);
	WRITEADDR( ring, buf );
	CARDBYTE( ring+RMD1_OFF ) = RMD1_RETURN;
	CARDSWORD( ring+RMD2_OFF ) = -RXBUFFER_SIZE;
		/* negate since RMD2 is defined to be negative */
	CARDWORD( ring+RMD3_OFF ) = RMD3_RETURN;
    }
    hwvars->rxring = hwvars->rxring_start;

/*
 *  initialize the transmit rings
 */
    
    for (ringnum = 0; ringnum < TXRING_NUM; ringnum++ ) {
	ring = hwvars->txring_start + ringnum*RING_SIZE;
	CARDBYTE( ring+TMD1_OFF ) = TMD1_INIT;
    }
    hwvars->txring_fill = hwvars->txring_start;
    hwvars->txring_empty = hwvars->txring_start;

/*
 *  initialize the transmit area
 */
    
    hwvars->tx_fill = hwvars->tx_start;
    hwvars->tx_empty = hwvars->tx_start;

/*
 *  start up the card (set LANCE's RAP to zero!)
 */
	
    CARDWORD( hwvars->card_base + LANCERAP_REG ) = 0;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");
    CARDWORD( hwvars->card_base + LANCERDP_REG ) = CSR0_INIT;
    NS_ASSERT ( (CARDBYTE(hwvars->card_base+INTR_REG) & ACK_BIT) == ACK_BIT, 
	    "E_HW_NO_ACK");

/*
 *  Poll for the initialization done interrupt
 */

/* read time into a temp rec (later) to improve performance; the following
 * division & multiplication under intpts disabled is not too good --prabha */

    get_network_time(&timerec);

    startime  = timerec.time;

    while (TRUE) {
/*
 *	time will be checked later if the reset did not complete
 */
	if ((CARDBYTE(hwvars->card_base+INTR_REG) & INTREQ_BIT) != 0) {
/*
 *	    read status (with polled ACK)
 */
	    if ( !read_rdp( hwvars, &stat ) )
		return( E_HARDWARE_FAILURE );

	    HW_QA_INCR_STAT( hwvars, ACK );
	    if (stat & IDON_BIT) {
/*
 *		acknowledge IDON interrupt only, bring card active
 */
		/* Check to see if INIT, STRT, TXON, RXON, INEA are on */
		NS_ASSERT( (stat & 0x0073) == 0x0073,"E_HW_NOT_UP");

		if ( !write_rdp( hwvars, CSR0_IDON_ACK ) )
		    return( E_HARDWARE_FAILURE );

		HW_QA_INCR_STAT( hwvars, RESET );

		hwvars->card_state = HARDWARE_UP;
		CARDBYTE( hwvars->card_base+INTR_REG ) = INTR_ENABLE;

		return(E_NO_ERROR);
	    }
	}
	HW_QA_INCR_STAT( hwvars, NO_RESET );

        /* reset the timer now; get system clock & compare with start time */
        /* assume that we won't run into a wrap around */

        get_network_time(&timerec);

	if ( timerec.time - startime > INIT_TIMEOUT ) 
         {
	    hwvars->card_state = RESET_ERROR;
	    CARDBYTE( hwvars->card_base+RESET_REG ) = 1;
	    return(E_HARDWARE_FAILURE);
	 }
    }
}

/***************** FUNCTION : hw_read_local_address *********************
 *
 * [OVERVIEW]	 
 *	Read the local address that is stored on the card's 
 *	non-volatile RAM.
 *
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *	
 * [OUTPUT]
 *	addr   -- the link address stored on the card
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	<None>.
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_base
 *
 ************************************************************************/

error_number
hw_read_local_address( hwvars, addr )
hw_gloptr	hwvars;
link_address	addr;
 
{

/*-------------------------- LOCAL VARIABLES ----------------------------
 *	checksum  --	computes the checksum
 *	addrptr	  --	points to nibbles of the NOVRAM
 *	i	  --	counter
 *---------------------------------------------------------------------*/

	register unsword checksum;
	register unsbyte *addrptr;
	register word	 i;

/*----------------------------- ALGORITHM -------------------------------
 *	figure out which bank to use
 *	checksum := 0;
 *	skip over unused fields, but calculate checksum
 *	for i := 1 to 6 do
 *	|   addr[i] := read two nibbles from the card;
 *	|   checksum := checksum + addr[i];
 *	end;
 *	skip over more unused fields, but calculate checksum
 *	if checksum != checksum on card then
 *	|   CARD_STATE := read_address_error;
 *	|   return( e_hardware_failure );
 *	else
 *	|   return( e_no_error );
 *	end;
 *---------------------------------------------------------------------*/

#ifdef DEBUG    
    printf(">>>> hw_read_local_address <<<<\n");
#endif /* DEBUG */   


    addrptr = hwvars->card_base + NOVRAM_OFF;
    if ((NOVBYTE( addrptr ) & BANK_BIT) != 0) {
	addrptr += HIGH_BANK_OFF;
    }
    checksum = 0;

/*
 *	skip the flags and SAP fields of the NOVRAM 
 *	(not used for the series 200, but need to calculate checksum)
 */

    for ( i = 0; i < 2; i++ ) {
	checksum += NOVBYTE( addrptr );
	addrptr += NOVBYTE_SIZE;
    }

/*
 *	read the address
 */

    for ( i = 0; i < ADDRESS_SIZE; i++ ) {
	addr[i] = NOVBYTE( addrptr );
	checksum += (unsword) addr[i];
	addrptr += NOVBYTE_SIZE;
    }

/*
 *	skip server address section
 */

    for ( i = 0; i < 7; i++ ) {
	checksum += NOVBYTE( addrptr );
	addrptr += NOVBYTE_SIZE;
    }

/*
 *  check checksum
 */

    if ((checksum & 0xFF) == NOVBYTE(addrptr)) 
	return( E_NO_ERROR );
    else {
	hwvars->card_state = READ_ADDRESS_ERROR;
	CARDBYTE( hwvars->card_base + RESET_REG ) = 1;
	CARDBYTE( hwvars->card_base + INTR_REG ) = INTR_DISABLE;
	return( E_HARDWARE_FAILURE );
    }
}


/***************** FUNCTION : hw_self_test ******************************
 *
 * [OVERVIEW]	 
 *	Perform self-test on the card.	Most of the work is performed
 *	by two assembly language routines.
 *	
 * [INPUT PARAMETERS]
 *	hwvars -- points to global variables for the hardware
 *
 * [GLOBAL VARIABLES MODIFIED]
 *	card_state
 *
 * [GLOBAL VARIABLES ACCESSED]
 *	card_base
 *
 ************************************************************************/

error_number
hw_self_test( hwvars )
hw_gloptr hwvars;
{
#ifdef DRVR_TEST
     lan_gloptr lg_ptr;
     lan_ift *is;
     int l_u;
     lg_ptr = (lan_gloptr) (hwvars->drvglob);
     is = (lan_ift *) lg_ptr->lan_ift_ptr;
     l_u = is->lu;
#endif /* DRVR_TEST */
#ifdef DEBUG    
    printf(">>>> hw_self_test <<<<\n");
#endif /* DEBUG */   
/* declare the card as being initialised to prevent a send operation during
   the self test, so hw_send will not send during initialisation...prabha*/
    hwvars->card_state = CARD_INIT;
    if ( ( hwvars->card_state = hw_int_test(hwvars->card_base) ) != 0 ) {
#ifdef DRVR_TEST
    printf("\nInternal Self Test Failed on lancard %d; sel code %d\n", l_u+1, hwvars->select_code); 
#endif /* DRVR_TEST */
	return( E_HARDWARE_FAILURE );
    }
#ifdef DEBUG    
    printf(">>>> hw_ext_test <<<<\n");
#endif /* DEBUG */    
    if ( ( hwvars->card_state = hw_ext_test(hwvars->card_base) ) != 0 ) {
#ifdef DRVR_TEST
    printf("\nExternal Self Test Failed on lancard %d; sel code %d\n", l_u, hwvars->select_code); 
#endif /* DRVR_TEST */
	return( E_HARDWARE_FAILURE );
    }
    return( E_NO_ERROR );
}

#ifdef NPERFORM

/*
 *  These routines are called directly from DA to stop and
 *  restart the card
 */

hw_stop( myglobal )
lan_gloptr	myglobal;
{
    write_rdp( &(myglobal->hwvars), STOP_BIT );
}
    
hw_start( myglobal )
lan_gloptr	myglobal;
{
    hw_softreset( &(myglobal->hwvars) );
}

hw_disable( myglobal )
lan_gloptr	myglobal;
{
    CARDBYTE( myglobal->hwvars.card_base + INTR_REG ) = INTR_DISABLE;
}

hw_enable( myglobal )
lan_gloptr	myglobal;
{
    CARDBYTE( myglobal->hwvars.card_base + INTR_REG ) = INTR_BIT;
}

int
hw_csr0( myglobal )
lan_gloptr	myglobal;
{
    word data;

    read_rdp( &(myglobal->hwvars), &data );
    return( (int) data );
}

#endif /* NPERFORM */

int
hw_find_mcaddr(ift,addr)
   lan_ift *ift;
   unsigned char *addr;
{
   int i;

   for(i=0;i<ift->num_multicast_addr;i++) {
      if(!bcmp((caddr_t)addr,(caddr_t)&(ift->mcast[PHYSADDR_SIZE*i]),
            PHYSADDR_SIZE)) {
         return(1);
      }
   }
   return(0);
}

hw_rint_ics(ift,mbuf)
   struct lan_ift *ift;
   struct mbuf *mbuf;
{
   lan_rheader *lan;
   struct mbuf *tmbuf;
   struct ieee8023_hdr *ie_hdr;
   int totlen,lenadj;

   for(totlen=0,tmbuf=mbuf ; tmbuf ; totlen+=tmbuf->m_len,tmbuf=tmbuf->m_next);

   lan=mtod(mbuf,lan_rheader *);
   if(lan->type < MIN_ETHER_TYPE) {
      /* IEEE 802.3 packet */
      ie_hdr=mtod(mbuf,struct ieee8023_hdr *);
      if((lenadj = totlen - (ie_hdr->length + ETHER_HLEN)) > 0) {
         m_adj(mbuf,-lenadj);
      }
      lanc_802_2_ics(ift,mbuf,IEEE_RAW8023_HLEN,IEEE8023_0);
   } else {
      /* Ethernet packet */
      lanc_ether_ics(ift,mbuf);
   }
}

