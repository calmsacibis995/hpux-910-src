/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/lnatypes.h,v $
 * $Revision: 1.4.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:14:18 $
 */

#ifndef _MACHINE_LNATYPES_INCLUDED
#define _MACHINE_LNATYPES_INCLUDED

/* the following are required by software/hardware drivers..prabha */
typedef char                  byte;
typedef short                 word;
typedef long                 dword;
typedef unsigned   char    unsbyte;
typedef unsigned   short   unsword;
typedef unsigned long     unsdword;
typedef unsbyte    link_address[6];
typedef unsbyte	       sap_address;
typedef unsword	       hp_addr_exp;
typedef unsword	     ethernet_type;
typedef unsword       error_number;
typedef char              *charptr;
typedef unsigned long     *longptr;
typedef byte              *byteptr;   /* added for NFS project pvf 7/14/86 */
typedef word         packet_offset;
typedef char               *anyptr;
typedef byte               boolean;
#define NIL 0
/*#define NPERFORM        1*/
#define LAN_PROC_LEVEL          2
#define LAN_PROC_SUBLEVEL       5
#define TIMER_PROC_LEVEL        0
#define TIMER_PROC_SUBLEVEL     5
#define IEEE802_IP_ADR  006             /*  006 Octal.          */
#define IEEE802_EXP_ADR 252             /*  374 Octal.          */
#define ETH_IP_ADR      2048            /*  4000 Octal.         */
#define ETH_EXP_ADR     32773           /*  100005 Octal.       */

/*      Used to distiguish between an ethernet and ieee802 packet.  If the word
 *      in the ethernet type_field position is less than this value, assume it
 *      is IEEE802's frame length and that the packet is an ieee802 packet.
 */

#define ETH_START_ADR   0x600

#define IP_HEADER_SIZE  20
#define TCP_HEADER_SIZE 24
	

/*
 * These protocol identifiers are used in a connection to specify a protocol
 * handler, and can be used to map into the protocol_globals structure.
 */
 
#define NULL_ID         0
#define LAN_ID          1
#define ASYNC_ID        2
#define SWITCH_ID       3       /* For VAX process switching */
#define DIRECT_ID       4
#define PROBE_ID        5
#define IP_ID           6
#define TCP_ID          7
#define SI_ID           8
#define BB_ID           9

#ifdef QA
#define TH_ID           10      /* QA only.  Test Harness.              */
#define EG_ID           11      /* QA only.  Exception Generator.       */
#define TICL_ID         12      /* QA only.  TICL protocol.             */
#endif

#ifdef QA
#define NUM_PROTO_IDS   13
#else
#define NUM_PROTO_IDS   10
#endif
#define LLP_LEVEL       0



/* constants from nerror.h */

#define   E_NO_ERROR			0

/*
* * * * * * * * * * * * *
*                       *
*       General         *
*       Errors          *
*                       *
* * * * * * * * * * * * *
*/
#define   E_OUT_OF_MEMORY			103
#define   E_WRONG_CARD_TYPE			128

/* added by prabha on 02/04/87 for white box tests */
#define   E_FORCED_ERROR                        131
#define   E_HARDWARE_FAILURE			204
/*
* * * * * * * * * * *
*                   *
*       LLP         *
*                   *
* * * * * * * * * * *
*/

#define   E_LLP_NO_SUCH_STAT			1001
#define   E_LLP_NO_BUFFER_SEND			1002
#define   E_NOT_INDIVIDUAL			1003
#define   E_NOT_MULTICAST			1004
#define   E_TOO_MANY_MULTICAST			1005
#define   E_NOT_ON_MC_LIST			1006
#define   E_ALREADY_ON_LIST			1007


/*
* * * * * * * * * * * *
*                     *
*      HARDWARE       *
*                     *
* * * * * * * * * * * *
*/

#define   E_HW_NOT_STP1				1100
#define   E_HW_NOT_STP2				1101
#define   E_HW_NOT_STP3				1102
#define   E_HW_NOT_STP4				1103
#define   E_HW_RXQUADWORD			1104
#define   E_HW_TXQUADWORD			1105
#define   E_HW_WRONG_STAT			1106
#define   E_HW_DEST_NOT_EVEN			1107
#define   E_HW_SRC_NOT_EVEN			1108
#define   E_HW_FRAME_RANGE			1109
#define   E_HW_IDON_SET				1110
#define   E_HW_NOT_RXON				1111
#define   E_HW_NOT_TXON				1112
#define   E_HW_NOT_INDIVIDUAL			1113
#define   E_HW_NOT_UP				1115
#define   E_HW_TX_PAST_END1			1116
#define   E_HW_TX_PAST_END2			1117
#define   E_HW_NOT_OWN1				1118
#define   E_HW_NOT_OWN2				1119
#define   E_HW_NOT_OWN3				1120
#define   E_HW_NOT_OWN4				1121
#define   E_HW_NO_ACK				1122
#define   E_HW_WRONG_ADDR			1123
#define   E_HW_WRONG_LEVEL			1124
#define   E_HW_SKIP_BAD				1125
#define   E_HW_TIMER_CALLED			1126
#define   E_HW_NOT_ENP				1127

#endif /* not _MACHINE_LNATYPES_INCLUDED */
