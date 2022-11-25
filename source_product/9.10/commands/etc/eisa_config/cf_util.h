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
*                             inc/cf_util.h
*
*	This file contains the headers for the CF utilities.
*
*******************************************************************************/



/* Src and dest index types for cf_process_system_indexes() */
#define CF_EEPROM_NDX			0
#define CF_CURRENT_NDX			1
#define CF_CONFIG_NDX 			2
#define CF_PREV_NDX			3


/* Modes for cf_process_system_indexes() */
#define CF_FIRST_DIFF			0
#define CF_NEXT_DIFF			1
#define CF_COPY_MODE			2


/* Return codes for cf_process_system_indexes() */
#define CF_NO_DIFF			0
#define CF_CHOICE_DIFF			1
#define CF_SUBCHOICE_DIFF		2
#define CF_SUBCHOICE_RESOURCE_DIFF	3
#define CF_PRIMARY_RESOURCE_DIFF	4
#define CF_GO_ON			5


/* Used with cf_convert_memory_value() */
#define VALUE_TO_STRING 		0
#define STRING_TO_VALUE 		1


/* Used with cf_lock_reset_restore() */
#define CF_LOCK_MODE			0
#define CF_RESET_MODE			1
#define CF_UNLOCK_MODE			3
#define CF_EEPROM_UNLOCK		4
#define CF_LOCK_CHOICE_MODE		5	/* lock down to choice level */


/* Used with cf_lock_reset_restore() */
#define CF_SYSTEM_LEVEL 		0
#define CF_BOARD_LEVEL			1
#define CF_SUBFUNCTION_LEVEL		2


/* Operation parameter passed to cf_configure() */
#define CF_ADD_OP		1  /* adding a board			      */
#define CF_DELETE_OP		2  /* removing a board			      */
#define CF_EDIT_OP		3  /* changing functions/resources	      */
#define CF_VERIFY_OP		4  /* manual verify or fast config (unused)   */
#define CF_SAVE_OP		5  /* exiting, save the configuration	      */
#define CF_MANUAL_AUTO_OP	6  /* chg to auto verify (unused)	      */
#define CF_RESET_OP		7  /* from revert, reset, and unlock (unused) */
#define CF_MOVE_OP		8  /* moving a board			      */
