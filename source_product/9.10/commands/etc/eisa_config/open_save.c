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
*+*+*+*************************************************************************
*
*                          src/open_save.c
*
*   This module contains the high level routines for starting a
*   configuration from an sci file.
*   This module also contains the high level routines for saving current
*   configuration to EISA eeprom and an sci file.
*
*	open_sci()		-- main.c
*	save()			-- main.c, init.c
*	last_physical_slot()    -- main.c
*	find_board()		-- init.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "config.h"
#include "nvm.h"
#include "sci.h"
#include "err.h"
#include "add.h"
#include "cf_util.h"
#include "compat.h"


/***************
* Globals used in this file -- declared in globals.c.
***************/
#ifdef VIRT_BOARD
extern unsigned int	glb_virtual;
#endif
extern struct system	glb_system;
extern char		sci_name[];
extern int		changed_since_save;
extern int		changed_since_start;
extern int		eeprom_opened;


/***************
* Functions declared in this file.
***************/
int			open_sci();
int			save();
int			last_physical_slot();
struct board		*find_board();
static void 		check_dupids();


/**************
* Functions declared elsewhere and used here.
**************/
extern  int           	nvm_save_board();
extern  int             nvm_clear();
extern  int             nvm_read_slot();
extern  int             nvm_initialize();
extern  int  		cf_configure();
extern  void  		del_release_system();
extern  void            compile_inits();
extern  void            make_cfg_file_name();
extern void             pr_init_file_print();
extern void             pr_end_file_print();
extern void		show();


/****+++***********************************************************************
*
* Function:     open_sci()
*
* Parameters:	None
*
* Used:		external only
*
* Returns:      0			peachy
*		-1			problem -- error already reported
*
* Description:
*
*   The open_sci function will start the configuration with a new .sci
*   file. The user-specified sci file name is assumed to be in the global
*   sci_name[]. This function makes sure the file exists and is valid.
*
*   Note: Currently, this function assumes we are in non-target mode.
*         This should be relaxed for later releases when it would be nice
*         to restore from an sci file if eeprom is bad or blank.
*
****+++***********************************************************************/

int open_sci()
{
    char    		cfg_name[MAXPATHLEN];
    nvm_slotinfo    	slot_info;
    int 		err;
    int 		add_options;


    /********************
    * Do the sci initialization. If it fails, display an error message
    * and exit.
    ********************/
    err = nvm_initialize(NVM_SCI);
    if (err != NVM_SUCCESSFUL)  {
	switch (err) {
	    case NVM_SCI_CANNOT_OPEN:
		err_add_string_parm(1, sci_name);
		err_handler(NO_SCI_FILE_ERRCODE);
		break;
	    case NVM_INVALID_SCI:
		err_add_string_parm(1, sci_name);
		err_handler(BAD_SCI_FILE_ERRCODE);
		break;
	}
	return(-1);
    }

    /*******************
    * Otherwise, the initialization worked, so free up the old system resources
    * and set up some more globals.
    *******************/
    else {
	del_release_system(&glb_system);
#ifdef VIRT_BOARD
	glb_virtual = MIN_VIRTUAL_SLOT;
#endif
	nvm_current = NVM_SCI;
    }

    /*******************
    * Try to get the nvm information for the system board from the sci file.
    *******************/
    err = nvm_read_slot(SYSTEM_SLOT, &slot_info, NVM_SCI);
    if (err != NVM_SUCCESSFUL) {
	err_add_string_parm(1, sci_name);
	err_handler(BAD_SCI_FILE_ERRCODE);
	return(-1);
    }

    /*********************
    * Make a cfg file name for the system board and call init2().
    *********************/
    make_cfg_file_name(cfg_name, slot_info.boardid, &slot_info.dupid);
    add_options = AUTO_RESTORE;
    err = init2(cfg_name, (unsigned)add_options);

    /***************
    * Set up all of the initial values for the inits.
    ***************/
    if (err == 0)
	compile_inits(&glb_system, 2);

    return(err);
}


/****+++***********************************************************************
*
* Function:     save()
*
* Parameters:   fname		0	save to eeprom and default sci file
*				other   name of file to store to (checked here)
*
* Used:		external only
*
* Returns:      0					Successful
*		CANT_WRITE_EEPROM_OR_SCI_ERRCODE	nothing saved
*		CANT_WRITE_EEPROM_ERRCODE		eeprom not saved, but
*							sci saved
*		CANT_WRITE_SCI_ERRCODE			eeprom saved if
*							requested, sci not saved
*
* Description:
*
*   This function operates in two different modes:
*
*	(1) If fname is 0, write the configuration out to eeprom and then
*	    write it out to the default sci file. Note that if the eeprom
*	    write fails, we still write to the default sci file.
*
*	(2) If fname is not 0, it is the name of a file to write the
*	    configuration to. The eeprom will not be updated.
*
*   Errors are handled in the following manner:
*
*	o  err_handler will be called from this function for any errors which
*	   are detected
*
*	o  if the eeprom write fails, an sci write will still be attemped
*
*	o  if the eeprom write fails, the eeprom is cleared
*
****+++***********************************************************************/

save(fname)
    char		*fname;
{
    register int	slot;
    unsigned int	err;
    int			eeprom_err=0;
    int			sci_err=0;
    struct board	*board;
    int			lastslot;
    int			ignore_too_many_resources;


    /****************
    * Scan the system looking for duplicate ids and record that data
    * in the per board structure (board->duplicate = 1).
    ****************/
    check_dupids();

    /****************
    * Build the system-level linked lists of resources (inits).
    * Calculate what the last slot is.
    ****************/
    compile_inits(&glb_system, 0);
    lastslot = last_physical_slot();

    /****************
    * If we want to write to eeprom, go ahead and do so.
    ****************/
    if (*fname == 0)  {

	/******************
	* Initialize the eeprom. If there is an error, forget about the eeprom
	* write; however, still try to write to the sci file.
	******************/
	if (eeprom_opened == 0) {
	    err = nvm_initialize(NVM_EEPROM);
	    if (err != NVM_SUCCESSFUL) {
		eeprom_err = CANT_WRITE_EEPROM_ERRCODE;
		switch (err)  {
		    case NVM_EEPROM_IN_USE:
			err_handler(EEPROM_IN_USE_ERRCODE);
			break;
		    case NVM_NO_EISA_HW:
			err_handler(NO_EISA_HW_PRESENT_ERRCODE);
			break;
		    case NVM_NO_EEPROM_DRIVER:
			err_handler(NO_EEPROM_DRIVER_ERRCODE);
			break;
		    case NVM_INVALID_EEPROM:
			err_handler(INVALID_EEPROM_ERRCODE);
			break;
		}
	        goto sci_stuff;
	    }
	}
	err = nvm_clear(NVM_EEPROM, UTIL_REVISION_2, (unsigned char *)0);
	if (err != NVM_SUCCESSFUL) {
	    eeprom_err = CANT_WRITE_EEPROM_ERRCODE;
	    err_handler(eeprom_err);
	}

	/********************
	* The clear worked, so walk through each of the boards and save
	* away their configuration in eeprom. If the board is currently
	* turned off, write a blank record. If we encounter a write error,
	* exit this loop and go ahead and try the sci writes.
	********************/
	else {

	    for (slot=SYSTEM_SLOT ; slot<=lastslot ; slot++) {

		board = find_board((unsigned)slot);

		err = nvm_save_board(board, (unsigned)slot, NVM_EEPROM, 0);
		if (err != NVM_SUCCESSFUL) {
		    (void)nvm_clear(NVM_EEPROM, UTIL_REVISION_2,
		    		    (unsigned char *)0);
		    if (err == NVM_WRITE_ERROR) {
			eeprom_err = CANT_WRITE_EEPROM_ERRCODE;
			err_handler(eeprom_err);
			break;
		    }
		}

	    }
	}
    }

sci_stuff:
    /*******************
    * Set up the correct sci file name in preparation for the
    * write to the sci file. If we have already wriiten to eeprom, set up
    * the option for nvm_save_board() so that resource overruns are not
    * reported again in the /etc/eisa/system.sci case.
    *******************/
    if (*fname == 0) {
	(void)strcpy(sci_name, SCI_DEFAULT_NAME);
	ignore_too_many_resources = 1;
    }
    else {
	(void)strcpy(sci_name, fname);
	ignore_too_many_resources = 0;
    }
    err_add_string_parm(1, sci_name);

    /*******************
    * Set up the sci buffer. If there's a problem, tell the user and forget
    * the sci write.
    *******************/
    board = find_board(SYSTEM_SLOT);
    err = nvm_clear(NVM_SCI, UTIL_REVISION_2, (unsigned char *)board->name);
    if (err != NVM_SUCCESSFUL) {
	sci_err = CANT_WRITE_SCI_ERRCODE;
	err_handler(sci_err);
    }

    /******************
    * Walk through each of the boards we have data for and write the
    * configuration data to the sci file. Save stuff in slot order.
    ******************/
    else

	for (slot = SYSTEM_SLOT; slot <= lastslot; slot++) {
	    board = find_board((unsigned)slot);
	    err = nvm_save_board(board, (unsigned)slot, NVM_SCI,
	    			 ignore_too_many_resources);
	    if (err != NVM_SUCCESSFUL) {
		sci_err = CANT_WRITE_SCI_ERRCODE;
		err_handler(sci_err);
		break;
	    }
	}

    /******************
    * Set up the current location to be eeprom if we have that capability,
    * otherwise, sci.
    ******************/
    if (*fname == 0)
	nvm_current = NVM_EEPROM;
    else
	nvm_current = NVM_SCI;

    /******************
    * Flush the sci internal buffer to the sci file.
    ******************/
    if (sci_err == 0) {
	err = sci_flush();
	if (err != NVM_SUCCESSFUL) {
	    sci_err = CANT_WRITE_SCI_ERRCODE;
	    err_handler(sci_err);
	}
    }

    /****************
    * Return the correct error code. Note that err_handler has already
    * been called if there were any problems.
    ****************/
    if ( (eeprom_err != 0) && (sci_err != 0) )
	return(CANT_WRITE_EEPROM_OR_SCI_ERRCODE);
    if (eeprom_err != 0)
	return(CANT_WRITE_EEPROM_ERRCODE);
    if (sci_err != 0) {
	if (*fname == 0) {
	    err_handler(EEPROM_UPDATE_OK_ERRCODE);
	    changed_since_save = 0;
	    changed_since_start = 1;
	    pr_init_file_print();
	    show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);
	    show(1, "board", (char *)0, (char *)0, NOT_AN_INT, 0, 1);
	    show(1, "switch", (char *)0, (char *)0, NOT_AN_INT, 0, 1);
	    pr_end_file_print();
	}
	return(CANT_WRITE_SCI_ERRCODE);
    }
    if (*fname == 0) {
	err_handler(EEPROM_AND_SCI_UPDATE_OK_ERRCODE);
	changed_since_save = 0;
	changed_since_start = 1;
	pr_init_file_print();
	show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);
	show(1, "board", (char *)0, (char *)0, NOT_AN_INT, 0, 1);
	show(1, "switch", (char *)0, (char *)0, NOT_AN_INT, 0, 1);
	pr_end_file_print();
    }
    else
	err_handler(SCI_UPDATE_OK_ERRCODE);
    return(0);

}


/****+++***********************************************************************
*
* Function:     find_board()
*
* Parameters:   slot_number		slot to find board for
*
* Used:		internal and external
*
* Returns:      NULL			board cannot be found
*		other			ptr to board structure
*
* Description:
*
*   The find_board function will search the glb_system structure for a
*   board that is occupying the specified slot number.
*
****+++***********************************************************************/

struct board *find_board(slot_number)
    unsigned int 	slot_number;
{
    struct board	*board;


    for (board=glb_system.boards ; board ; board=board->next)
	if (board->eisa_slot == slot_number)
	    break;

    return(board);
}





/****+++***********************************************************************
*
* Function:     last_physical_slot()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      the highest slot number actually present
*
* Description:
*
* The last_physical_slot function will determine the highest slot
* number present in the current system.
*
****+++***********************************************************************/

int last_physical_slot()
{
    int			last;
    int			i;


    last = 0;
    for (i=0 ; i<MAX_NUM_SLOTS ; i++)
	if ( (glb_system.slot[i].present) &&
	     (glb_system.slot[i].eisa_slot > last) )
	    last = glb_system.slot[i].eisa_slot;

    return(last);
}






/****+++***********************************************************************
*
* Function:     check_dupids
*
* Parameters:   None                
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    The check_dupids routine will scan the entire system for duplicate
*    IDs and set the appropriate bits in the board structures of the
*    boards with duplicate IDs in the system.
*
****+++***********************************************************************/

static void check_dupids()
{
    struct board	*anchor;
    struct board	*board;


    for (board = glb_system.boards; board; board = board->next)
	board->duplicate = 0;

    for (anchor = glb_system.boards; anchor; anchor = anchor->next)
	for (board = anchor->next; board; board = board->next)
	    if (!strcmpi(anchor->id, board->id)) {
		anchor->duplicate = 1;
		board->duplicate = 1;
	    }
}
