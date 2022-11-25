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
*                          src/add.c
*
*	This module contains the high level routines for adding a board to
*	the system.
*
*	add_to_slot()		-- init.c, main.c, open_save.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "config.h"
#include "err.h"
#include "add.h"
#include "nvm.h"
#include "cf_util.h"
#include "compat.h"


/* Functions declared in this file */
int			add_to_slot();
static int		assign_slot();
static int 		check_busmaster();
static int 		check_slottags();
static int 		find_slot();


/* Functions used here but declared elsewhere */
extern  int             nvm_load_board();
extern  struct board    *cfg_load();
extern  FILE		*open_cfg_file();
extern  void  		del_release_board();
extern  void  		del_release_system();
extern  int		slot_ok();
extern  int		slot_type_ok();
extern  void		sci_add_cfg_file();


/* Globals used here and declared in globals.c */
extern struct system	glb_system;
#ifdef VIRT_BOARD
extern unsigned int	glb_virtual;
extern unsigned int	glb_logical;
extern unsigned int	glb_embedded;
#endif


/****+++***********************************************************************
*
* Function:   	add_to_slot  
*                                                   
* Parameters:	cfg_name        file name to add 
*               slot_number     requested slot number   [what is possible here?]
*		brd		address of the board ptr to fill in
*               add_options                             [what is possible here?]
*                                                 
* Used:		internal and external
*
* Returns:      0		Successful
*		-1		Not successful -- brd parm is not valid
*		-2		Not successful -- brd parm is valid
*		-3		Load error on slot 0 (changed cfg file)
*                                                
* Description:                                  
*                                              
*    The add_to_slot function will attempt to add the specified board to
*    the system and assign it to the requested slot number.            
*
*    Lots of problems are possible:
*	o  We may not be able to open the cfg file.
*	o  The cfg_load() may fail
*	o  We may not be able to assign a slot
*	      --> the slot_number should always be known coming in, the only
*		  error should be a board/slot capability mismatch
*                                                                     
****+++***********************************************************************/

int add_to_slot(cfg_name, slot_number, brd, add_options)
    char			*cfg_name;
    unsigned int		slot_number;
    struct board		**brd;
    unsigned int		add_options;
{
    FILE			*cfg_handle;
    int				status;
    int				err;
    char			full_filename[MAXPATHLEN];
    struct board		*board;


    /**********************
    * Open the cfg file that was passed in. If we cannot, get outta here.
    **********************/
    cfg_handle = open_cfg_file(cfg_name, full_filename);
    if (cfg_handle == NULL) {
	err_add_string_parm(1, cfg_name);
	err_handler(NO_CFG_FILE_ERRCODE);
	return(-1);
    }

    /*********************
    * Go ahead and load up the cfg file for this guy.
    * If the load failed, exit right away (the error
    * message has already been displayed).
    *********************/
    *brd = cfg_load(full_filename, cfg_handle, add_options);
    if (*brd == NULL) {
	return(-1);
    }
    board = *brd;

    /************************
    * If this board is embedded and we have not yet verified the hardware
    * in a given slot, allow the EMB(x) statement to change the slot number.
    ************************/
    if ( ( (add_options & HW_CHECKED) == 0) &&
	 (board->eisa_slot != UNKNOWN_SLOT) &&
	 (board->eisa_slot < MAX_NUM_SLOTS) )
	slot_number = board->eisa_slot;

    /**********************
    * Get a slot number for this guy (assign_slot() displays error messages).
    **********************/
    status = assign_slot(board, slot_number, add_options);
    if (status != 0) {
	del_release_board(&glb_system, board);
	if (status == -1)
	    return(-2);
	return(-1);
    }

    /***********************
    * If we are starting from a known configuration, bring in the rest
    * of the data from eeprom or sci.
    ***********************/
    if (add_options & AUTO_RESTORE) {
	err = nvm_load_board(board, NVM_CURRENT);
	if (err != NVM_SUCCESSFUL) {
	    if (slot_number == SYSTEM_SLOT) {
		del_release_system(&glb_system);
		return(-3);
	    }
	    err_add_string_parm(1, cfg_name);
	    err_add_num_parm(1, (unsigned)board->eisa_slot);
	    err_handler(EEPROM_AND_CFG_MISMATCH_ERRCODE);
	    del_release_board(&glb_system, board);
	    return(-2);
	}
    }

    /*****************
    * Keep track of the cfg file we used for this guy.
    *****************/
    sci_add_cfg_file((int)slot_number, full_filename);

    return(0);

}


/****+++***********************************************************************
*
* Function:     assign_slot
*                         
* Parameters:   board           board to assign slot to
*               slot_number     slot number to assign  [what is possible here?]
*               add_options                            [what is possible here?]
*                                                  
* Used:		internal only
*
* Returns:      0               assignment completed
*               -1              assignment not completed  (error displayed here)
*		-2		assignment not completed  (tag problem)
*                                                     
* Description:                                       
*                                                   
*   The assign_slot function will attempt to assign a slot number to the
*   specified board.
*                                                                   
*   ?? This code has the potential for assigning the same EISA slot to    
*   ?? multiple boards that request the same EISA slot.              
*                                                                
****+++***********************************************************************/

static assign_slot(board, slot_number, add_options)
    struct board	*board;
    unsigned int	slot_number;
    unsigned int	add_options;
{
    register int	slot;


    /**********************
    * If the hardware has verified the presence of this board in the
    * specified slot then assign the slot. The only thing that can blow
    * us out here is if the slot tags or busmaster attributes don't match.
    **********************/
    if (add_options & HW_CHECKED) {

	slot = find_slot(slot_number);
	board->eisa_slot = slot_number;

#ifdef VIRT_BOARD
	if (slot == UNKNOWN_SLOT) {
	    if (board->slot == bs_vir)
		board->slot_number = glb_virtual++;
	    else
		board->slot_number = glb_embedded++;
	}
	else {
#endif
	    board->slot_number = slot;
	    glb_system.slot[slot].occupied = 1;
#ifdef VIRT_BOARD
	}
#endif

	if (slot_number) {
	    if (check_busmaster(board) != 0)
		return(-1);
	    if (check_slottags(board) != 0)
		return(-2);
	}

	return(0);
    }

    /****************
    * If the board has not yet been verified by a hardware look, work on it
    * based on the slot type.
    ****************/
    switch (board->slot) {

#ifdef VIRT_BOARD
	/********************
	* If the virtual board is being added, it must be because we know it was
	* there before (eeprom or sci).
	********************/
	case bs_vir:
	    board->eisa_slot = glb_logical++;
	    board->slot_number = glb_virtual++;
	    return(0);
#endif


	/*******************
	* Handle an embedded slot.
	*******************/
	case bs_emb:

#ifdef VIRT_BOARD
	    /**********************
	    * If there is no slot specifier, we have a problem.
	    **********************/
	    if (slot_number == UNKNOWN_SLOT) {
		err_handler(INV_EMB_BOARD_ERRCODE);
		return(-1);
	    }
#endif

	    /********************
	    * Get a slot. Make sure that the slot we got is not currently
	    * occupied.
	    ********************/
	    slot = find_slot(slot_number);
#ifdef VIRT_BOARD
	    if (slot == UNKNOWN_SLOT)
		board->slot_number = glb_embedded++;
	    else {
		if (glb_system.slot[slot].occupied) {
		    err_add_num_parm(1, (unsigned)slot);
		    err_handler(EMB_SLOT_OCCUPIED_ERRCODE);
		    board->slot_number = UNKNOWN_SLOT;
		    return(-1);
		}
	    }
#endif
	    glb_system.slot[slot].occupied = 1;
	    board->slot_number = slot;

	    /********************
	    * The slot is available. Make sure that it is ok for this
	    * card (tags and busmaster match).
	    ********************/
	    board->eisa_slot = slot_number;
#ifdef VIRT_BOARD
	    if (slot_number != 0) {
		if (check_busmaster(board) != 0)
		    return(-1);
		if (check_slottags(board) != 0)
		    return(-2);
	    }
#endif
	    return(0);


	/******************
	* Handle a "normal" slot.
	******************/
	default:

	    /*****************
	    * Grab the slot. If the slot is out of range, exit.
	    * If the slot is already occupied or not there, exit.
	    *****************/
	    slot = find_slot(slot_number);
	    if ( (slot_number >= MAX_NUM_SLOTS) 	   ||
	    	 (slot == UNKNOWN_SLOT) 	   ||
	         (!glb_system.slot[slot].present) )  {
		err_add_num_parm(1, (unsigned)slot_number);
		err_handler(NONEXISTENT_SLOT_ERRCODE);
		return(-1);
	    }
	    else if (glb_system.slot[slot].occupied) {
		err_add_num_parm(1, (unsigned)slot_number);
		err_handler(SLOT_OCCUPIED_ERRCODE);
		return(-1);
	    }
	    
	    /********************
	    * The slot is available. Make sure that it is ok for this
	    * card (tags and busmaster match).
	    ********************/
	    board->slot_number = slot;
	    if (board->eisa_slot = slot_number) {
		if (check_busmaster(board) != 0)
		    return(-1);
		if (check_slottags(board) != 0)
		    return(-2);
	    }
	    if ( (slot_ok(board, &(glb_system.slot[slot])) )  &&
		 (slot_type_ok(board->slot, &(glb_system.slot[slot])) ) )  {
		glb_system.slot[slot].occupied = 1;
		return(0);
	    }
	    return(-1);

    }

}


/****+++***********************************************************************
*
* Function:     find_slot
*
* Parameters:   eisa_slot         
*
* Used:		internal only
*
* Returns:      index of the matching entry
*               -1 	if no matching slot
*
* Description:
*
*    The find_slot routine will scan the glb_system.slot array for an
*    entry with a matching eisa_slot number to the passed parameter.
*
****+++***********************************************************************/

static int find_slot(eisa_slot)
    register unsigned int	eisa_slot;
{
    register int		slot;


    if (eisa_slot >= MAX_NUM_SLOTS)
	return(-1);

    for (slot = MAX_NUM_SLOTS - 1; slot >= 0; slot--)
	if (glb_system.slot[slot].present &&
	    eisa_slot == glb_system.slot[slot].eisa_slot) {
	    break;
	}

    return(slot);
}





/****+++***********************************************************************
*                                                                
* Function:     check_busmaster                                   
*                                                                  
* Parameters:   board                                               
*                                                                    
* Used:		internal only
*
* Returns:      0		card matches slot
*		-1		card doesn't match slot (error printed here)
*                                                                      
* Description:                                                          
*                                                                 
*    The check_busmaster routine will verify that the slot occupied is
*    is capable of handling a busmaster if the board is a busmaster board.
*    If a busmaster board is placed in a non-busmaster slot, an error
*    message is displayed.
*                                                                        
****+++***********************************************************************/

static int check_busmaster(board)
    struct board	*board;
{


#ifdef VIRT_BOARD
    if ( (board->busmaster)  &&
         ( (board->slot_number >= MAX_NUM_SLOTS) ||
	   (!glb_system.slot[board->slot_number].busmaster) ) ) {
#else
    if ( (board->busmaster)  &&
	 (!glb_system.slot[board->slot_number].busmaster) )  {
#endif
	err_add_num_parm(1, (unsigned)board->slot_number);
	err_handler(BOARD_NEEDS_BUSMASTER_ERRCODE);
	return(-1);
    }
    return(0);

}


/****+++***********************************************************************
*
* Function:     check_slottags
*
* Parameters:   board
*
* Used:		internal only
*
* Returns:      0		card matches slot
*		-1		card doesn't match slot (error printed here)
*
* Description:
*
*    The check_slottags routine will verify that the slot occupied is
*    capable of handling the board.  If a board with a tag is placed
*    in a slot without a matching tag then display an error message which
*    includes slots which do have the matching tag (if any).
*
****+++***********************************************************************/

static int check_slottags(board)
    struct board	*board;
{

    char		item[42];
    char		buffer[320];
    unsigned int	slot;
    struct tag		*tag;


    /*************
    * If the board does not have a tag, there can't possibly be a problem.
    *************/
    if (board->slot_tag == 0)
	return(0);

    /*************
    * Grab the list of tags for this slot (if any). Walk through each of the
    * slot's tags and compare them to the tag for the board. If there was
    * a match, exit with good status.
    *************/
    slot = board->slot_number;
    tag = glb_system.slot[slot].tags;
    while (tag)
	if (strcmpi(tag->string, board->slot_tag) == 0)
	    break;
	else
	    tag = tag->next;
    if ((int)tag != 0)
	return(0);

    /*************
    * The board tag did not match any of the tags associated with the slot.
    * Walk through all of the slots and build a list of slots that *do*
    * have a matching tag (if any).
    * Display an error message which includes the list of possible slots.
    *************/
    buffer[0] = '\0';
    for (slot=0 ; slot<MAX_NUM_SLOTS ; slot++) {
	tag = glb_system.slot[slot].tags;
	while (tag) {
	    if (strcmpi(tag->string, board->slot_tag) == 0) {
		(void)sprintf(item, "%d ", slot); 
		(void)strcat(buffer, item);
		break;
	    }
	    tag = tag->next;
	}
    }
    err_add_num_parm(1, (unsigned)board->slot_number);
    err_add_string_parm(1, buffer);
    err_handler(BOARD_NEEDS_TAG_ERRCODE);
    return(-1);

}
