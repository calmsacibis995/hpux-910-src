/*
 * @(#)drvhw_ift.h: $Revision: 1.2.84.3 $ $Date: 93/09/17 21:13:03 $
 * $Locker:  $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 

/*****************************************************************************
 ../sio/lanc.h must be included before this file to get def of lan_ift 
 ../s200io/drvhw.h must be included before this file to get def of hw_globals
*****************************************************************************/

#define LAN0_PROTOCOL_IEEE 0
#define LAN0_PROTOCOL_ETHER 1

#define PHYSADDR_SIZE 6

#define LAN_NO_TX_BUF 0x01

#define MAX_LAN_CARDS   5       /*  Max # of Lan Cards for S300 system */

#define LAN_MTU 1514    /* maximum length of packet (includes header */



typedef struct {
   boolean        broadcast_state;
   int            flags;
   u_char         *local_link_address;
   anyptr         lan_ift_ptr;
   hw_globals     hwvars;
} lan_global,*lan_gloptr;


typedef struct {
   lan_ift        lanc_ift;
   lan_global     lan_vars;
} landrv_ift;

