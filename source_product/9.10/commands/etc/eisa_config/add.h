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
*                              inc/add.h
*
*******************************************************************************/


/************************
* These defines are bits in a flag word. They keep track of how a given
* board was added to the system and what state that add is currently in.
************************/

#define AUTO_RESTORE		0x1     	/* set whenever we are adding
						   a board from eeprom or sci --
						   we should know all about 
						   the board already         */

#define HW_ADDED		0x8      	/* found via hw check */

#define HW_CHECKED		0x20     	/* hw is there and matches id */
