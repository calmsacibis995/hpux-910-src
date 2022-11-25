/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/******************************************************************************
*   (C) Copyright COMPAQ Computer Corporation 1985, 1989
*******************************************************************************
*
*                         inc/sw.h
*
*      These are some defines used to communicate with sw_disp().
*
*******************************************************************************/


#define SWITCH_SOFTWARE		0x01    /* show software parameters           */
#define SWITCH_HARDWARE		0x02    /* show hardware switches and jumpers */
#define SWITCH_ONESLOT		0x04    /* show for one slot only	      */
#define SWITCH_CHANGED		0x08    /* show stuff that has changed only   */
