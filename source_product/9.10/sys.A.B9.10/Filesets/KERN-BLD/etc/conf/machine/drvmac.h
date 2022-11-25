/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/drvmac.h,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:13:09 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 

/*---------------------------------------------------------------------------+
 |									     |
 |   DDDD    RRRR    V	 V	     M	 M    AAA     CCC	     H	 H   |
 |    D	 D   R	 R   V	 V	     MM MM   A	 A   C	 C	     H	 H   |
 |    D	 D   R	 R   V	 V	     M M M   A	 A   C		     H	 H   |
 |    D	 D   RRRR     V V	     M M M   AAAAA   C		     HHHHH   |
 |    D	 D   R R      V V	     M	 M   A	 A   C		     H	 H   |
 |    D	 D   R	R      V	     M	 M   A	 A   C	 C     ..    H	 H   |
 |   DDDD    R	 R     V	     M	 M   A	 A    CCC      ..    H	 H   |
 |									     |
 +---------------------------------------------------------------------------*/


#ifndef _MACHINE_DRVMAC_INCLUDED
#define _MACHINE_DRVMAC_INCLUDED

/************************** drvmac.h ***********************************
 *	
 *	AUTHOR:		  Jeff Wu, Bill Hayes
 *	
 *	DATE BEGUN:	  7 June 1984
 *
 *	MODULE:		  etherh
 *	PROJECT:	  hacksaw
 *	REVISION:	  3.2
 *	SCCS NAME:	  /users/fh/hacksaw/sccs/etherh/header/s.drvmac.h
 *	LAST DELTA DATE:  3/26/86
 *
 *	DESCRIPTION:
 *		This file contains common shorthand notation and 
 *		macros for the LAN driver.
 *	DEPENDENCIES:
 *		ntypes.h
 *		narchdef.h 
 *		narchvar.h
 *
 ************************************************************************/

/*-----------------------------------------------------------------------
 *	Short hand notations
 *----------------------------------------------------------------------*/

/* 
 *  Shorthand notation for accessing the protocol globals record
 */

#define Myrtn	( protocol_globals[LAN_ID] )
  
/* 
 *  Mystate returns a pointer to the LAN connection state information 
 */ 

#define Mystate ( (lan_info_ptr)csrp->protos[mylevel].info.lan_infop )
  
  
/* 
 *  Myhw returns a pointer to hwvars
 *  depends on the existance of myglobal variables 
 */

#define Myhw	(&(myglobal->hwvars))
  
/* 
 *  Iheader and Eheader enables a element of the respective
 *  header structure to be reference Eheader.type_field
 */

#define Iheader	    (mystate->info.iheader)
#define Eheader	    (mystate->info.eheader)

/*-----------------------------------------------------------------------
 *	Macros
 *----------------------------------------------------------------------*/

/*
 *  Copy an address.  The parameters 'from' and 'to' should be of type unsbyte *. 
 */

/*-=- #define COPY_ADDRESS( from, to ) bcopy( from, to, 6) -=-*/
#define COPY_ADDRESS(from,to)  LAN_COPY_ADDRESS(from,to)
#define LAN_COPY_ADDRESS(from,to) { \
	struct linkaddr { \
		link_address dummy; \
	}; \
	*(struct linkaddr *)(to) = *(struct linkaddr *)(from); \
}

/*
 *  Test if the packet is ETHERNET by checking the type field.
 *  Returns a true condition when the type_field is a valid ETHERNET
 *  type field.
 */

#define IS_ETHERNET( type_field )	((type_field) >= ETH_START_ADR )

/*
 *  Test if the 'address' is individual.  'address' should be of type unsbyte *.
 */

#define INDIVIDUAL( address )	(( (address)[0] & 0x01 ) == 0 )

/*
 *  Test if the 'address' is broadcast.	 'address' should be of type unsbyte *.
 */

#define BROADCAST(address) (((unsbyte)(address)[0]==0xFF)&&((unsbyte)(address)[1]==0xFF)&&((unsbyte)(address)[2]==0xFF)&&((unsbyte)(address)[3]==0xFF)&&((unsbyte)(address)[4]==0xFF)&&((unsbyte)(address)[5]==0xFF))

/*
 *  Test if the addresses are the same, 'addr1 and addr2' should be of the
 *  same type.
 */

#define SAME_ADDRESS(addr1,addr2)	((addr1)[0]==(addr2)[0]&&(addr1)[1]==(addr2)[1]&&(addr1)[2]==(addr2)[2]&&(addr1)[3]==(addr2)[3]&&(addr1)[4]==(addr2)[4]&&(addr1)[5]==(addr2)[5])

/*
 *  Read a word from the received packet (starting at addr).  This macro 
 *  will assemble the word bytewise, thus this macro can read over 
 *  different alignments as well as work for the machines which word data 
 *  is byte-swapped from the network representation.
 */

#define READ_WORD(addr) ( (*((unsbyte *) (addr)) << 8) + *((unsbyte *) (addr) + 1) )

/*
 *  Write a word (info) to the packet (starting at addr).  This macro will 
 *  write the word bytewise, thus this macro can read over different 
 *  alignments as well as work for the machines which word data is 
 *  byte-swapped from the network representation.
 */

#define WRITE_WORD(addr,info)	{ *((unsbyte *) (addr)) = ((info) >> 8); *((unsbyte *) (addr)+1) = (info) & 0x00FF; }

#define DRV_ASSERT( cond, num ) WARNING( !(cond), num, NIL, __LINE__, 0 )

/************************** end of "drv_mac.h" **************************/

#endif /* not _MACHINE_DRVMAC_INCLUDED */
