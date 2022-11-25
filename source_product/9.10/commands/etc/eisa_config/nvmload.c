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
*                        src/nvmload.c
*
*   This module contains the high level routines for setting board and
*   function configuration from EISA eeprom or an sci file.
*
* 	nvm_load_board()	-- add.c
*
**++***************************************************************************/


#include <stdio.h>
#include "config.h"
#include "nvm.h"


/* Functions declared in this file */
int			nvm_load_board();
static void		nvm_load_function();
static void		nvm_load_subchoice();
static void		nvm_load_link();
static void		nvm_load_combine();
static void		nvm_next_index();
static int		get_index_addr();


/* Functions used here but declared elsewhere */
extern  int             nvm_read_slot();
extern  int             nvm_read_function();


/* Variables global to this file only */
static nvm_funcinfo     nvm_function_info;
static unsigned int 	nvm_index_offset;


/****+++***********************************************************************
*
* Function:     nvm_load_board
*
* Parameters:   board		board data structure
*		source		NVM_SCI/NVM_EEPROM/NVM_CURRENT
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_INVALID_EEPROM	Invalid eeprom or sci information
*               NVM_INVALID_FUNCTION    Invalid function number
*
* Description:
*
*   The nvm_load_board function sets the selections for a given board
*   according to eeprom or sci configuration. Note that this function
*   is only called by add_to_slot() when a board is being automatically
*   added. Error handling is done by add_to_slot().
*
****+++***********************************************************************/

nvm_load_board(board, source)
    struct board 	*board;
    unsigned int	source;
{
    unsigned int    	function_number;
    unsigned int    	error;
    struct group 	*group;
    struct function 	*function;
    nvm_slotinfo 	nvm_slot_info;


    /********************
    * Read the slot information. If the read failed or the checksums don't
    * match, get outta here.
    ********************/
    if (nvm_read_slot((unsigned)board->eisa_slot, &nvm_slot_info, source))
	return(NVM_INVALID_EEPROM);
    if (nvm_slot_info.checksum != board->checksum)
	return(NVM_INVALID_EEPROM);

    /*******************
    * For each function specified in the board data structure, make sure that
    * the function is valid and readable from eeprom or sci. Note that the
    * function data read here is not saved as we go -- it keeps getting
    * overwritten.
    *******************/
    function_number = 0;
    for (group=board->groups ; group ; group=group->next)

	for (function=group->functions ; function ; function=function->next) {

	    error = nvm_read_function((unsigned)board->eisa_slot, function_number,
				      &nvm_function_info, source);
	    if (error != NVM_SUCCESSFUL)
		return(error);

	    nvm_load_function(function);

	    function_number++;

	}

    return(NVM_SUCCESSFUL);
}


/****+++***********************************************************************
*
* Function:     nvm_load_function
*
* Parameters:   function		function data structure
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_load_function function sets the current selections for the
*   function specified according to eeprom or sci (nvm_next_index() uses
*   data for the current function which was read from either eeprom or sci).
*
****+++***********************************************************************/

static void nvm_load_function(function)
    struct function 	*function;
{
    unsigned int    	selection;
    struct choice 	*choice;
    struct subchoice 	*subchoice;
    struct subfunction  *subf;


    nvm_index_offset = 0;

    for (subf=function->subfunctions ; subf ; subf=subf->next) {

	nvm_next_index(&selection, 1);

	subf->index.current = selection;
	subf->index.eeprom = selection;

	for (choice=subf->choices ; choice&&selection ; choice=choice->next)
	    selection--;

	subf->current = choice;

	if (choice->primary)
	    nvm_load_subchoice(choice->primary);

	subchoice = choice->subchoices;

	if (subchoice != 0) {
	    nvm_next_index(&selection, 1);
	    choice->index.current = selection;
	    choice->index.eeprom = selection;
	    for (; subchoice&&selection ; subchoice=subchoice->next)
		selection--;
	    choice->current = subchoice;
	    nvm_load_subchoice(subchoice);
	}

    }

}


/****+++***********************************************************************
*
* Function:     nvm_load_subchoice
*
* Parameters:   subchoice		subchoice tree
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_load_subchoice function sets the current selections for a
*   subchoice according to eeprom or sci.
*
****+++***********************************************************************/

static void nvm_load_subchoice(subchoice)
    struct subchoice 		*subchoice;
{
    unsigned int    		selection;
    int    			ndxsz;
    struct resource_group 	*resgrp;
    struct resource 		*res;


    for (resgrp=subchoice->resource_groups ; resgrp ; resgrp=resgrp->next) {

	for (ndxsz=1, res=resgrp->resources ; res ; res=res->next)
	    if (res->type == rt_memory) {
		ndxsz = 2;
		break;
	    }

	nvm_next_index(&selection, ndxsz);

	resgrp->index.current = selection;
	resgrp->index.eeprom = selection;

	switch (resgrp->type) {
	    case rg_link:
		nvm_load_link(resgrp, selection);
		break;
	    case rg_free:
	    case rg_dlink:
	    case rg_combine:
		nvm_load_combine(resgrp, selection);
		break;
	}

    }

}


/****+++***********************************************************************
*
* Function:     get_index_addr()
*
* Parameters:   res 
*		index_ptr
*
* Used:		internal only
*
* Returns:	index address
*
* Description:
*
*    xx
*
****+++***********************************************************************/

static int get_index_addr(res, index_ptr)
    struct resource 	*res;
    struct index 	**index_ptr;
{
    int 		index_value;


    switch (res->type) {
	case rt_irq:
	    *index_ptr = &((struct irq *)res)->index;
	    index_value = ((struct irq *)res)->index_value;
	    break;
	case rt_dma:
	    *index_ptr = &((struct dma *)res)->index;
	    index_value = ((struct dma *)res)->index_value;
	    break;
	case rt_port:
	    *index_ptr = &((struct port *)res)->index;
	    index_value = ((struct port *)res)->index_value;
	    break;
    }

    return(index_value);
}





/****+++***********************************************************************
*
* Function:     nvm_next_index
*
* Parameters:   index		receiving variable
*		size		bytes in selection (1 or 2)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_next_index function retrieves the next selection item from
*   the selection table in nvm_function_info.  The offset of the current
*   index is in nvm_index_offset.
*
****+++***********************************************************************/

static void nvm_next_index(index, size)
    unsigned int	*index;
    int 		size;
{
    int 		value;


    value = nvm_function_info.selects[nvm_index_offset];

    /***********
    * If this was a 2 byte index, the first is MSB and the second is LSB.
    ***********/
    if (size == 2)
	value = (value << 8)+nvm_function_info.selects[nvm_index_offset+1];

    *index = value;
    nvm_index_offset += size;

}


/****+++***********************************************************************
*
* Function:     nvm_load_link()
*
* Parameters:   resgrp 
*		selection
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

static void nvm_load_link(resgrp, selection)
    struct resource_group 	*resgrp;
    unsigned int		selection;
{
    struct resource 		*res;
    struct memory 		*mem;
    struct index 		*index_ptr;


    for (res=resgrp->resources ; res ; res=res->next) {

	if (res->type == rt_memory) {
	    mem = (struct memory *)res;
	    mem->memory.index.current = selection;
	    mem->memory.index.eeprom = selection;
	    if (mem->address) {
		mem->address->index.current = selection;
		mem->address->index.eeprom = selection;
	    }
	}

	else {
	    (void)get_index_addr(res, &index_ptr);
	    index_ptr->current = selection;
	    index_ptr->eeprom = selection;
	}

    }

}




/****+++***********************************************************************
*
* Function:     nvm_load_combine()
*
* Parameters:   resgrp
*		selection
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

static void nvm_load_combine(resgrp, sel)
    struct resource_group 	*resgrp;
    unsigned int		sel;
{
    struct resource 		*res;
    struct memory 		*mem;
    struct index 		*index_ptr;
    int 			index_value;


    for (res=resgrp->resources ; res ; res=res->next) {

	if (res->type == rt_memory) {
	    mem = (struct memory *)res;
	    mem->memory.index.current = sel / mem->memory.index_value;
	    mem->memory.index.eeprom = sel / mem->memory.index_value;
	    if (mem->address) {
		mem->address->index.current = sel % mem->memory.index_value;
		mem->address->index.eeprom = sel % mem->memory.index_value;
	    }
	}

	else {
	    index_value = get_index_addr(res, &index_ptr);
	    index_ptr->current = sel / index_value;
	    index_ptr->eeprom = sel / index_value;
	    sel = sel % index_value;
	}

    }

}
