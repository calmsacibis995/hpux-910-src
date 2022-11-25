/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/drvhw.h,v $
 * $Revision: 1.7.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:12:57 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 

/************************** drvhw.h *************************************
 *	
 *	AUTHOR:	 Jeff Wu
 *	
 *	DATE BEGUN:  9 April 1984
 *
 *	MODULE:		  etherh
 *	PROJECT:	  hacksaw
 *	REVISION:	  3.1
 *	SCCS NAME:	  /users/fh/hacksaw/sccs/etherh/header/s.drvhw.h
 *	LAST DELTA DATE:  3/4/86
 *
 *	DESCRIPTION:
 *		This is the include file for the hardware model
 *		implementation of the driver for the series 200 Unix.
 *		The include information should not be of concern
 *		to anyone but the driver.  It needs to be included
 *		by users of the driver since drv.h depends on this
 *		file.  This file contains definitions of types and
 *		functions which are implemented as macros.
 *	DEPENDENCIES:
 *		ntypes.h,
 *		narchdefs.h -- for network architecture definitions.
 *		timeout.h  -- a series 200 definition file.  It is included 
 *			      to access the definition of the structure 
 *			      'sw_intloc'.
 *			      Note: timeout.h depends on types.h
 *
 *	NOTE:
 *		all references to processes in the comments refer to
 *		the the three network architecture processes:
 *		    user-invoked, receive-initiated, timer-initiated.
 *
 ************************************************************************/

/*
 *  Driver compile time flags:
 *
 *	NEED_TIMER -- this is the transmit timer code implemented for
 *		      LANCE Rev B' defect.  In Rev C, STOP-RESET
 *		      workaround does not work.	 Since there is no
 *		      known reason for it, it is pulled out.  If
 *		      the transmit timer is needed again, then the
 *		      xmit_timer should re-initialize the LANCE.
 *		      Re-initializing affects many state variables,
 *		      thus making the functions: hw_send, hw_isr, and
 *		      isr_proc (possibly more) critical sections.
 *		      A method which does not involve disabling
 *		      system interrupts is to have xmit_timer to
 *		      check for card enabled/disabled.	If disabled,
 *		      just set the timer again (maybe shorter period).
 *	DRVLOOPBACK -- This flag controls if the full-duplex action is
 *		      to be used.  In full-duplex, all packets addressed
 *		      to my own node or to an acceptable broadcast or
 *		      multicast frame are put in the receive queue for
 *		      processing.
 */
#undef NEED_TIMER
#define DRVLOOPBACK	1


/*-------------------------- statistics_type ----------------------------
 *
 *	PURPOSE:  contains the enumeration of the different statistics. 
 *
 *---------------------------------------------------------------------*/

typedef byte statistics_type;

#define ALL_RECEIVE	0
#define ALL_TRANSMIT	1
#define UNDELIVERABLE	2		
#define UNSENDABLE	3
#define FCS_ERRORS	4
#define TX_COLLISIONS	5

#define INT_DEFERRED	6
#define INT_ONE		7
#define INT_MORE	8
#define INT_RETRY	9
#define INT_LATE	10
#define LOST_CARRIER	11
#define NO_HEART_BEAT	12
#define INT_FRAMMING	13
#define INT_MISS	14
#define UNKNOWN_802	15
#define NO_CSR_FOUND	16	/* Last Real Statistic (see below) */

/*
 *  Start of the non-released statistics
 */

#define NO_BUF_COUNT	17
#define IND_RECEIVE	18

#define BCAST_TAKEN	19
#define MCAST_TAKEN	20
#define INT_BCAST_DROP	21
#define INT_MCAST_DROP	22
#define INDIVIDUAL_DROP 23

#define TX_NO_RINGS	24
#define TX_NO_ROOM_A	25
#define TX_NO_ROOM_B	26
#define TX_NEED_ALIGN	27
#define NO_ACK		28
#define ACK		29
#define RX_QUEUE_NIL	30
#define RX_QUEUE_NOT_NIL 31
#define TRIGGER_RCV	32
#define TRIGGER_ISR	33
#define RESET		34
#define NO_RESET	35
#define NO_ENP		36
#define LENGTH_ERROR	37
#define LANCE_FILLING	38
#define INT_RXBUFFER	39
#define BABBLE_COUNT	40
#define HW_XMIT_TIMER	41
#define SW_XMIT_TIMER	42
#define SKIP_BAD_COUNT	43
#define INT_OVERFLOW	44
#define INT_UNDERFLOW	45
#define LEN_ARRAY	46	/* 12 elements */
#define TX_QUEUED	58
#define TX_SENT		59
#define QUEUED_ARRAY	60	/* 6 elements */

#define START_STAT_VAL	0 


#ifdef NPERFORM

#define END_STAT_VAL	(QUEUED_ARRAY+5)
#define HW_QA_INCR_STAT( hwvars, stat )	  (hwvars)->statistics[stat]++
#define HW_COND_INCR_STAT( cond, h, s )	  { if (cond) (h)->statistics[s]++; }

#else

#define END_STAT_VAL	NO_CSR_FOUND
#define HW_QA_INCR_STAT( hwvars, stat )
#define HW_COND_INCR_STAT( cond, h, s ) 

#endif

#define MAX_STATISTICS	(END_STAT_VAL+1)

#define INCR_NOCSR( hwvars, stat )		hw_incr_stat( hwvars, stat )
#define INCR_UNKNOWN802( hwvars, stat )		hw_incr_stat( hwvars, stat )
#define INCR_NOBUF				HW_COND_INCR_STAT

/*-----------------------------------------------------------------------
 *	Note:	the prefix INT_ used to denote internal (QA only) statistics
 *		but we decided to have the following statistics user
 *		accessible.
 *
 *	INT_ONE:
 *		Number of transmits with ONE collision.
 *	INT_MORE:
 *		Number of transmits with MORE than ONE collision.
 *	INT_MISS:
 *		Counts the number of packets that the card missed.  This 
 *		number	will always be less than or equal to the "actual" 
 *		number of packets lost.	 It includes all possible hardware
 *		MISSing a frame--MISS bit, OVERFLOW bit, RXBUFFER bit.
 *	INT_FRAMMING:
 *		Counts framing errors.	This error only occur when there 
 *		is a CRC error.
 *	INT_DEFERRED:
 *		Counts how many times the LANCE needed to defer before
 *		transmitting.
 *	INT_LATE:
 *		Counts the number of late collisions: collisions which
 *		occur after the slot time.
 *	INT_RETRY:
 *		Counts the number of times transmission failed because of
 *		excessive collisions.
 *
 *	Most of the non-released statistics are flags to indicate
 *	paths taken.  These probably should be removed since the
 *	behavior of the driver is fairly well known.
 *----------------------------------------------------------------------*/


/*--------------------------- hw_token ----------------------------------
 *	This token "type" is used by hw_protect and hw_unprotect to
 *	store away the card interrupt state.  
 *----------------------------------------------------------------------*/

typedef byte	hw_token;


/*--------------------------- card_state_type ---------------------------
 *	The various states of the driver.  Most of the enumerated type
 *	has been defined in drv.h.  This file adds series 200 specific
 *	failure states.
 *----------------------------------------------------------------------*/

typedef word	card_state_type;

#define HARDWARE_UP		 0	/* "up" state	  */

#define NO_CARD_STATE		-2
#define CARD_INIT		-1

#define UNKNOWN_ERROR		 1	/* failure states */
#define RESET_ERROR		 2
#define READ_ADDRESS_ERROR	 3
#define MEMORY_ERROR		 4
#define TXBUFFER_ERROR		 8
#define ACK_ERROR		 9

/* --- the numbers 10+ are reserved for self-test error codes --- */

/*-----------------------------------------------------------------------
 *	
 *	NO_CARD_STATE:
 *		There is no LAN card.  This error is visible to the user
 *		because if the network is powered up without a card, the
 *		user still can use bounce-back.
 *	INITIALIZING:
 *		This state is entered at drv_init.  Since the 
 *		initialization is polled and uninterruptable, this will 
 *		never be returned to the user.
 *	MEMORY_ERROR:
 *		The AMD card had a bus error trying to access the shared 
 *		memory.	 This means there is something wrong with the 
 *		shared memory control circuitry.
 *	TXBUFFER_ERROR:
 *		This implies the transmit buffer was rejected by the
 *		LANCE; which either means a driver bug or a chip failure.
 *
 *	The above numbering scheme is sparse because some conditions which
 *	were thought to be unreachable could be reached due to new
 *	information on the LANCE.
 *
 *----------------------------------------------------------------------*/


/*-------------------------- hw_globals ---------------------------------
 *
 *	PURPOSE:  all the global data used by the hardware model
 *		  implementation.
 *
 *	ALLOCATED BY:  the architecture at initialization as a part
 *		       of the <lan_global> structure.
 *
 *	DEALLOCATED BY:	 <never>
 *
 *	EXAMINED BY:  driver's hardware model routines
 *
 *	MODIFIED BY:  driver's hardware model routines
 *
 *----------------------------------------------------------------------*/
#define NORMAL_FRAME	0x03
#define LAN_PACKET_SIZE 1514
#define LAN_MIN_LEN	60
  
/*
 * Common for both IEEE802 and ETHERNET protocols
 */

#define LAN_DA_OFF	0		/* destination address */
#define LAN_TYPE_OFF	12		/* type field or length field */

/*
 * IEEE802 specific offsets
 */

#define IEEE_LEN_OFF		12		/* frame length */
#define IEEE_DSAP_OFF		14
#define IEEE_CONTROL_OFF	16
#define IEEE_EXP_OFF		20

/*
 * The following is the number of bytes in the frame that is not
 * specified in the contents of IEEE_LEN_OFF.
 */

#define IEEE_LEN_ADD	14

/*
 * Header sizes
 */

#define IEEE_SIZE	17
#define IEEE_EXP_SIZE	24

	
typedef struct {	/* typedef of hw_globals and hw_gloptr */ 
	card_state_type card_state;
	unsbyte *	card_base;
	unsbyte *	init_block;

	unsdword	statistics[ MAX_STATISTICS ];

	unsword		last_tdr;

	unsbyte *	rxring;
	unsbyte *	rxring_start;
	unsbyte *	rxring_end;

	unsbyte *	txring_fill;
	unsbyte *	txring_empty;
	unsbyte *	txring_start;
	unsbyte *	txring_end;

	unsbyte *	tx_fill;
	unsbyte *	tx_empty;
	unsbyte *	tx_start;
	unsbyte *	tx_end;

	word		xmit_queued;

	anyptr		drvglob;

	struct 
	sw_intloc	intloc, intloc2;

#ifdef NEED_TIMER
	boolean		timer_set;

	struct
	timeout		timeout_loc;
#endif /* NEED_TIMER */
	word		select_code;
	boolean		reset_iffup;

} hw_globals, *hw_gloptr; 

/*-----------------------------------------------------------------------
 *	Note:  The description below of which variables are modified 
 *	by which processes does not include the initialization of that 
 *	variable (done in either <hw_reset_card> or <hw_startup>.  This 
 *	initialization will be atomic (they are <protect>ed from the 
 *	other processes via both <protect> and disabling card interrupts).
 *	The processes named which can modify or access the variable are 
 *	those which possibly can modify or access that variable; the 
 *	actual usage will depend on how the driver intrinsics are used 
 *	by other protocols.
 *	
 *	card_state:
 *	     This variable contains the current state of the hardware.	
 *	     It can be modified by any of the processes.
 *	     MODIFIED BY:  any process
 *	     ACCESSED BY:  any process
 *	     PROTECTION :  none needed because setting this variable is 
 *			   an atomic operation.
 *	card_base:
 *	     This variable points to the base address of the card.  
 *	     It is used to access all cards data structures except 
 *	     the shared memory.	 The card base will always be even.  
 *	     This variable will be initialized once at <hw_reset_card> 
 *	     and will not be modified thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  any process
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	init_block:
 *	     This variable points to the start of the initialization 
 *	     block.  It also happens that this points to the start of 
 *	     memory.  This variable will be initialized once at 
 *	     <hw_reset_card> and will not be modified thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  any process
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	statistics:
 *	     This is an array of internal and user accessible statistics.
 *	     MODIFIED BY:  any process
 *	     ACCESSED BY:  receive process, user process
 *	     PROTECTION :  none used because any modification to any 
 *			   statistic value will be atomic.
 *			   A problem exists when two processes try to 
 *			   increment the counter when it is at FFFFFFFE.  
 *			   The comparison and increment is not atomic.	
 *			   This is not a problem currently because most 
 *			   of the statistics are incremented by <isr_proc> 
 *			   or <hw_isr>, both of which cannot be 
 *			   interrupted by themselves.  <drv_rcv> 
 *			   increments NO_CSR_FOUND but it also cannot be 
 *			   interrupted by itself.  <drv_send> increments 
 *			   IMMED_SEND_RETURN but it also cannot be 
 *			   interrupted by itself because of <protect>s.
 *	last_tdr:
 *	     This keeps the last value of Time Domain Reflectometry.  
 *	     The LANCE produces this value only when there is a 
 *	     collision retry.  This value is currently used only as a 
 *	     QA aid.
 *	     MODIFIED BY:  ISR process.
 *	     ACCESSED BY:  nothing right now.
 *	     PROTECTION :  none needed.
 *	
 *	     The three circular queues mentioned below (tx, txring, and 
 *	rxring) will have the following properties:
 *	
 *	     _start, _end:  mark the queue boundaries
 *			    these are essentially constants, they are 
 *			    used as variables to support the G-board.
 *	     _fill	 :  where the producer puts data
 *	     _empty	 :  where the consumer takes data
 *	
 *	     rxring_	 :  receive ring pointers
 *	     txring_	 :  transmit ring pointers
 *	     tx_	 :  transmit area pointers
 *
 *	     Note that the transmit ring and area will always have at
 *	least one unused ring or byte.	This is because only the fill 
 *	and empty pointers are used to determine the "full" and "empty" 
 *	conditions.  An ambiguity is created between "full" and "empty"
 *	if all the ring elements (or all of the transmit area) are used.
 *
 *	     Both fill and empty pointers will be written atomically, 
 *	and they will always be valid to avoid any critical section 
 *	problems.  
 *	
 *	     The queue descriptor <rxring> should be really named 
 *	rxring_empty, since it is where the driver consumes data.  It 
 *	is name as such since there is no rxring_fill (this is kept by 
 *	the lance card and communicated by the <own> bit).  The asymmetry 
 *	of the rxring and the txring occurs because the producer of the 
 *	queue needs to reclaim space, and the consumer does not.  This 
 *	asymmetry also means that the txring and tx pointers needs to be 
 *	always valid so that the fill does not go past empty when empty 
 *	is invalid, or vice versa.
 *	
 *	rxring:
 *	     This points to the next ring where the data will be put by 
 *	     the LANCE.	 The own bit will be checked before the data is 
 *	     assumed to be valid.
 *	     MODIFIED BY:  ISR process
 *	     ACCESSED BY:  ISR process
 *	     PROTECTION :  none needed since ISR process cannot interrupt 
 *			   itself
 *	rxring_start:
 *	     The address where the receive ring starts.	 This will be 
 *	     initialized at <hw_reset_card> and will not be changed 
 *	     thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  ISR process
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	rxring_end:
 *	     Points to the byte after where the receive ring ends.
 *	     (The logical place where the next ring element would occur).
 *	     This will be initialized at <hw_reset_card> and will not 
 *	     be changed thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  ISR process
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	
 *	txring_fill:
 *	     This points to the ring which the driver should use next 
 *	     to transmit a frame.  The driver will check the own bit 
 *	     before using this ring in case the ring buffer is full.  
 *	     This pointer will point to a valid transmit ring at all 
 *	     times (it will never point to txring_end).
 *	     MODIFIED BY:  user process, timer process, receive process
 *	     ACCESSED BY:  user process, timer process, receive process
 *			   ISR process
 *	     PROTECTION :  The send protect token will protect the 
 *			   accesses from the user process, timer process, 
 *			   and receive process.	 The ISR process needs to 
 *			   read this pointer so that empty does not go 
 *			   past fill.  All writes to this pointer will be 
 *			   atomic therefore no protection is needed 
 *			   against the ISR processes.  Note that the
 *			   txring_empty will not ever go past txring_fill 
 *			   and vice versa because of the atomic nature of 
 *			   writing to both txring_empty and txring_fill.  
 *	txring_empty:
 *	     This points to the ring which the lance will return next 
 *	     after transmission.  This will be used to return the buffers 
 *	     to the transmit area, and to update transmit statistics.  
 *	     The driver will check the own bit before assuming control of 
 *	     this ring.	 This pointer will point to a valid transmit ring 
 *	     at all times (it will never point to txring_end).
 *	     MODIFIED BY:  ISR process
 *	     ACCESSED BY:  ISR process, user process, timer process,
 *			   receive processes.
 *	     PROTECTION :  The non-ISR processes needs to read this 
 *			   pointer so that fill does not go past empty.	 
 *			   All writes to this pointer will be atomic 
 *			   therefore no protection is needed (analogous 
 *			   to the case above).
 *	txring_start:
 *	     The address where the transmit ring starts.  This will be 
 *	     initialized at <hw_reset_card> and will not be changed 
 *	     thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  ISR process, user process, timer process,
 *			   receive processes.
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	txring_end:
 *	     Points to the byte after where the transmit ring ends.
 *	     (The logical place where the next ring element would occur).
 *	     This will be initialized at <hw_reset_card> and will not 
 *	     be changed thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  ISR process, user process, timer process,
 *			   receive processes.
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	     
 *	tx_fill:
 *	     This points to where the driver should fill the next 
 *	     transmit frame.  The space available is between tx_fill 
 *	     and tx_empty (circularly).	 This pointer is updated whenever 
 *	     a frame is queued for transmission.  This pointer will be 
 *	     valid at all times (it will never point to tx_end).
 *	     MODIFIED BY:  user process, timer process, receive process
 *	     ACCESSED BY:  user process, timer process, receive process
 *	     PROTECTION :  The send protect token will protect the 
 *			   accesses from the user process, timer process, 
 *			   and receive process.	 Note that the ISR 
 *			   process does <not> need to read this pointer 
 *			   (unlike the txring_fill).
 *	tx_empty:
 *	     This points to where the start of the occupied (unusable) 
 *	     transmit area.  This pointer is updated whenever a transmit 
 *	     interrupt indicates that the transmit has finished.  This 
 *	     pointer will be valid at all times (it will never point to 
 *	     tx_end).
 *	     MODIFIED BY:  ISR process
 *	     ACCESSED BY:  ISR process, user process, timer process,
 *			   receive processes.
 *	     PROTECTION :  The non-ISR processes needs to read this 
 *			   pointer so that fill does not go past empty.	 
 *			   All writes to this pointer will be atomic 
 *			   therefore no protection is needed (analogous 
 *			   to the case above).
 *	tx_start:
 *	     The address where the transmit area starts.  This will be 
 *	     initialized at <hw_reset_card> and will not be changed 
 *	     thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  ISR process, user process, timer process,
 *			   receive processes.
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	tx_end:
 *	     Points to the byte after where the transmit area ends.
 *	     This will be initialized at <hw_reset_card> and will not 
 *	     be changed thereafter.
 *	     MODIFIED BY:  None
 *	     ACCESSED BY:  ISR process, user process, timer process,
 *			   receive processes.
 *	     PROTECTION :  none needed since this is basically a 
 *			   constant.
 *	
 *	xmit_queued:
 *	     The number of transmit frames queued for LANCE to transmit.
 *	     MODIFIED BY:  isr_proc, and hw_send
 *	     ACCESSED BY:  isr_proc, and hw_send
 *	     PROTECTION:   increments and decrements are assumed to be
 *			   atomic.  Total interrupt disabling is used
 *			   however, to keep xmit_queued in sync with
 *			   timer_set and the timer schedule (timeout) calls.
 *	     
 *	drvglob:
 *	     Back pointer to the shared driver globals, needed when 
 *	     accessing info such as the multicast list.
 *	     MODIFIED BY:  hw_reset_card
 *	     ACCESSED BY:  hw_loopback
 *	     PROTECTION :  none needed since it is written only once.
 *
 *	timer_set:
 *	     This describes if a transmit timer is active in the system.
 *	     MODIFIED BY:  isr_proc, and hw_send
 *	     ACCESSED BY:  isr_proc, and hw_send
 *	     PROTECTION:   Total interrupt disabling is used to keep 
 *			   timer_set in sync with xmit_queued and the 
 *			   timer schedule (timeout) calls.
 *	     
 *	intloc, intloc2:
 *	     This is the structure used by sw_trigger.	
 *	     This structure will only be passed as a "token" by the
 *	     driver (except for initialization).
 *		 intloc is used to run the receive_task
 *		 intloc2 is used to run the isr_proc
 *	     MODIFIED BY:  sw_trigger, and the interrupt system.
 *	     ACCESSED BY:  sw_trigger, and the interrupt system
 *			   passed by hw_isr.
 *	     PROTECTION:   by the interrupt system and sw_trigger.
 *
 *	timeout_loc
 *	     This is the structure used by timeout and clear_timeout.
 *	     This structure will only be passed as a "token" by the
 *	     driver (except for initialization).
 *	     MODIFIED BY:  timeout and clear_timeout
 *	     ACCESSED BY:  timeout and clear_timeout
 *	     PROTECTION:   by the timeout system
 *	    
 *----------------------------------------------------------------------*/
/********************** end of drvhw.h *********************************/
