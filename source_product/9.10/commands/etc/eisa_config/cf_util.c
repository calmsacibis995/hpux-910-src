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
*                            src/cf_util.c
*
*   This file contains the source code for the CF utilities.
*
*   Most of the functions in this file are used locally only. The public
*   functions are:
*
*	cf_configure()		-- add.c, init.c, mn_cf.c,
*				   mrbrd.c, open_save.c, vbeditfn.c
*	
*	cf_process_system_indexes() -- init.c, mn_cf.c
*
*	cf_lock_reset_restore()	-- cfgload.c, init.c, mn_cf.c
*
**++***************************************************************************/


#include <string.h>
#include "config.h"
#include "cf_util.h"
#include "err.h"


/*******************
* Functions declared here and also used elsewhere.
*******************/
int		cf_process_system_indexes();
int		cf_lock_reset_restore();
int		cf_configure();


/*******************
* These are the functions declared in this file that are only used locally.
*******************/
static void 	cf_process_subfunction();
static void 	cf_process_choice();
static void 	cf_process_subchoice();
static void 	cf_process_resource_group();
static void	cf_update_memory_resource();
static void 	cf_update_irq_resource();
static void 	cf_update_dma_resource();
static void 	cf_update_port_resource();
static int 	cf_get_index();
static void 	cf_set_index();
static void	cf_operate_on_subfunction();
static void	cf_operate_on_choice();
static void	cf_operate_on_resource_group();
static void	cf_operate_on_resource();
static void	cf_reset_noncurrent_paths();
static void	cf_check_amp_load();


/**************
* Functions declared elsewhere and used here.
**************/
extern void		config();
extern void  		*mn_trapcalloc();
extern void		mn_trapfree();
extern void		set_share_and_cache();
extern void		set_port_address();
extern void		del_release_board();


/* Data structure filled in by cf_process_system_indexes().	*/
struct	cf_path_template {
    struct system		*system;
    struct board		*board;
    struct group		*group;
    struct function		*function;
    struct subfunction		*subfunction;
    struct choice		*choice;
    struct subchoice		*subchoice;
    struct resource_group	*resource_group;
};


/**********************
* cf_process_system_indexes() local globals
**********************/
static int	    		glb_src;    	    /* source and destination */
static int	    		glb_dest;   	    /*   index types          */
static int	    		glb_mode;   	    /* user specified mode    */
static int	    		glb_end_of_path;    /* next subfunction time  */
static int	    		glb_primary_checked;/* for primary subchoice  */
static struct cf_path_template	glb_path;	    /* current search path    */
static struct cf_path_template	*glb_users;	    /* user's return path     */


/***********************
* Local defines
***********************/
#define FAILURE			-1


/****+++***********************************************************************
*
* Function:     cf_process_system_indexes()
*
* Parameters:   system_ptr 	pointer to the system data structure
*		src 		the source index type: CF_EEPROM_NDX, 
*				    CF_CONFIG_NDX
*		dest 		the destination index type: CF_CURRENT_NDX
*		mode 		the mode: CF_COPY_MODE
*		address_ptr 	pointer to the return system struct (DIFF modes
*				only)  -- unused
*
* Used:		internal and external
*
* Returns:
*	CF_COPY_MODE:
*	    The return value is always CF_NO_DIFF.
*	CF_*_DIFF:
*	    The type of the first difference detected is returned:
*	        CF_PRIMARY_RESOURCE_DIFF
*		CF_SUBCHOICE_RESOURCE_DIFF
*		CF_SUBCHOICE_DIFF
*		CF_CHOICE_DIFF
*		CF_NO_DIFF
*
* Description:
*
*	This routine performs 1 of 2 functions: it either compares the
*	index data struct values in a system data struct for differences
*	or it copies the index values from one index type to another.
*
****+++***********************************************************************/

cf_process_system_indexes(system_ptr, src, dest, mode, address_ptr)
    struct system		*system_ptr;
    int				src;
    int				dest;
    int				mode;
    void			*address_ptr;
{
    struct cf_path_template    	users_path;


    /********************
    * Some of the original parameters are saved in global space.
    *********************/
    glb_src = src;
    glb_dest = dest;
    glb_mode = mode;
    glb_users = (struct cf_path_template *)address_ptr;

    /*******************
    * If the user provided a null
    * pointer, use a temporary one on the stack just to simplify
    * the rest of the code in the modules.
    *********************/
    if (glb_users == NULL)
	glb_users = &users_path;

    /*******************
    * If this is the first time thru then load the system ptr
    * and initialize the rest of the static pointers.
    *********************/
    if ( (glb_mode == CF_FIRST_DIFF) || (glb_mode == CF_COPY_MODE) ) {
	glb_path.system = system_ptr;
	glb_path.board = glb_path.system->boards;
	glb_path.group = glb_path.board->groups;
	glb_path.function = glb_path.group->functions;
	glb_path.subfunction = glb_path.function->subfunctions;
	glb_path.choice = NULL;
	glb_path.resource_group = NULL;
	glb_primary_checked = FALSE;
    }

    /*******************
    * Walk through each board in the system and either do the copies or
    * look for differences. There are no indexes until
    * the subfunction level is reached.
    *********************/
    glb_users->system = glb_path.system;
    while (glb_path.board != NULL) {

	glb_users->board = glb_path.board;

	while (glb_path.group != NULL) {
	    glb_users->group = glb_path.group;

	    while (glb_path.function != NULL) {
		glb_users->function = glb_path.function;

		while (glb_path.subfunction != NULL) {

		    glb_end_of_path = FALSE;
		    cf_process_subfunction(glb_path.subfunction);

		    /******************
		    * Set up the pointer to the next subfunction.
		    ******************/
		    glb_path.subfunction = glb_path.subfunction->next;
		    glb_primary_checked = FALSE;

		}

		/*******************
		* Reset the subfunction pointer to the beginning
		* for the next function.
		* The check for a null pointer cannot be moved up because
		* we have to support CF_NEXT_DIFF mode.
		*********************/
		glb_path.function = glb_path.function->next;
		if (glb_path.function != NULL)
		    glb_path.subfunction = glb_path.function->subfunctions;
	    }

	    /******************
	    * Reset the pointers upon changing the next group.
	    * The checks for a null pointer cannot be moved up because
	    * we have to support CF_NEXT_DIFF mode.
	    ******************/
	    glb_path.group = glb_path.group->next;
	    if (glb_path.group != NULL) {
		glb_path.function = glb_path.group->functions;
		if (glb_path.function != NULL)
		    glb_path.subfunction = glb_path.function->subfunctions;
	    }
	}

	/******************
	* Reset the pointers upon changing to the next board.
	* The checks for a null pointer cannot be moved up because
	* we have to support CF_NEXT_DIFF mode.
	******************/
	glb_path.board = glb_path.board->next;
	if (glb_path.board != NULL) {
	    glb_path.group = glb_path.board->groups;
	    if (glb_path.group != NULL) {
		glb_path.function = glb_path.group->functions;
		if (glb_path.function != NULL)
		    glb_path.subfunction = glb_path.function->subfunctions;
	    }
	}
    }

    /*******************
    * If this is reached then all of the diffs have been found.
    * This is also COPY_MODE's exit code.
    *********************/
    return(CF_NO_DIFF);
}


/****+++***********************************************************************
*
* Function:     cf_process_subfunction()
*
* Parameters:   subfunction	ptr to a subfunction struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine processes the subfunction data struct.  It checks
*	the index values in this data struct for diffs or a copy.
*
****+++***********************************************************************/

static void cf_process_subfunction(subfunction)
    struct subfunction 	*subfunction;
{
    int	    		index;
    int	    		i;
    struct choice  	*choice;


    /*******************
    * The users path is updated.
    *********************/
    glb_users->subfunction = subfunction;

    /*******************
    * If copy mode then copy indexes over.
    *********************/
    if (glb_mode == CF_COPY_MODE) {
	index = cf_get_index(&(subfunction->index), glb_src);
	if (index != -1)
	    cf_set_index(&(subfunction->index), glb_dest, index);
    }

    /*******************
    * The correct choice pointer is now loaded based on the src
    * index count.  If the index is not set then this path is done.
    *********************/
    index = cf_get_index(&(subfunction->index), glb_src);
    if (index == -1)
	return;
    choice = subfunction->choices;
    if (choice == NULL)
	return;
    for (i = 0; i < index; i++)
	choice = choice->next;

    /*******************
    * If this is a copy mode and the destination is config or
    * current then update the pointer.
    *********************/
    if (glb_mode == CF_COPY_MODE) {
	if (glb_dest == CF_CURRENT_NDX)
	    subfunction->current = choice;
    }

    /*******************
    * The choice data struct is processed for this path.
    *********************/
    cf_process_choice(choice);
    return;
}


/****+++***********************************************************************
*
* Function:     cf_process_choice()
*
* Parameters:   choice		ptr to the choice struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine processes the choice data struct.	It checks
*	the index values in this data struct for diffs or a copy.
*
****+++***********************************************************************/

static void cf_process_choice(choice)
    struct choice	*choice;
{
    int	    		index;
    int	    		i;
    struct subchoice	*subchoice;


    /*******************
    * The paths are updated.
    *********************/
    glb_users->choice = choice;
    glb_path.choice = choice;

    /*******************
    * The primary subchoice is checked to see if it has been
    * loaded yet. The primary must be processed until done.
    *********************/
    if ( (!glb_primary_checked) && (choice->primary != NULL) ) {
	cf_process_subchoice(choice->primary);
	glb_end_of_path = FALSE;
	glb_primary_checked = TRUE;
    }

    /*******************
    * If copy mode then copy indexes over.
    *********************/
    if (glb_mode == CF_COPY_MODE) {

	index = cf_get_index(&(choice->index), glb_src);
	cf_set_index(&(choice->index), glb_dest, index);

	/******************
	* If this is a copy from config to current then update
	* the total memory actual amount too.
	******************/
	if ( (glb_dest == CF_CURRENT_NDX) &&
	     (glb_src == CF_EEPROM_NDX) ) {
	    if (choice->totalmem != NULL)
		choice->totalmem->actual = choice->totalmem->eeprom;
	}

    }

    /*******************
    * The correct subchoice pointer is now loaded based on the src
    * index count. If the index is not set then this path is done.
    *********************/
    index = cf_get_index(&(choice->index), glb_src);
    if (index == -1)
	return;
    subchoice = choice->subchoices;
    if (subchoice == NULL)
	return;
    for (i = 0; i < index; i++)
	subchoice = subchoice->next;

    /*******************
    * If this is a copy mode and the destination is config or
    * current then update the pointer.
    *********************/
    if (glb_mode == CF_COPY_MODE) {
	if (glb_dest == CF_CURRENT_NDX)
	    choice->current = subchoice;
    }

    /*******************
    * The subchoice data struct is processed for this path.
    ********************/
    cf_process_subchoice(subchoice);
    return;
}


/****+++***********************************************************************
*
* Function:     cf_process_subchoice()
*
* Parameters:   subchoice	pointer to the subchoice data struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine processes the subchoice data struct. There are no
*	indexes here so it just handles the resource groups.
*
****+++***********************************************************************/

static void cf_process_subchoice(subchoice)
    struct subchoice   *subchoice;
{


    /*******************
    * The paths are updated.
    *******************/
    glb_users->subchoice = subchoice;
    glb_path.subchoice = subchoice;

    /*******************
    * If the resource group pointer is not valid then reset it
    * to the first entry for this subchoice.
    *******************/
    if (glb_path.resource_group == NULL)
	glb_path.resource_group = subchoice->resource_groups;

    /*******************
    * For each resource group, it is processed.
    *******************/
    while (glb_path.resource_group != NULL) {

	cf_process_resource_group(glb_path.resource_group);
	glb_path.resource_group = glb_path.resource_group->next;

	/*******************
	* If this is the last resource group then set a flag so
	* that the main routine knows to continue with the next
	* subfunction.
	*******************/
	if (glb_path.resource_group == NULL)
	    glb_end_of_path = TRUE;

    }

    /*******************
    * If this is reached then all of the diffs have been found
    * so continue processing. The resource group pointer is
    * reset so that the beginning of the routine knows to start
    * over with a new pointer the next time its called.
    *******************/
    glb_path.resource_group = NULL;
    return;
}


/****+++***********************************************************************
*
* Function:     cf_process_resource_group()
*
* Parameters:   resource_group  	pointer to the resource group
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine processes the resource group struct.  It checks
*	the index values in this data struct for diffs or a copy.
*
****+++***********************************************************************/

static void cf_process_resource_group(resource_group)
    struct resource_group	*resource_group;
{
    int	    			index;
    struct resource		*resource;
    struct memory		*memory;


    /*******************
    * The users path is updated.
    *******************/
    glb_users->resource_group = resource_group;

    /*******************
    * The resource group index is copied over.
    *******************/
    index = cf_get_index(&(resource_group->index), glb_src);
    cf_set_index(&(resource_group->index), glb_dest, index);

    /*******************
    * The resources must be updated.
    *********************/
    resource = resource_group->resources;
    while (resource != NULL) {
	switch(resource->type) {
	    case rt_memory:
		memory = (struct memory *)resource;
		cf_update_memory_resource(&(memory->memory));
		if (memory->address != NULL)
		    cf_update_memory_resource(memory->address);
		break;
	    case rt_irq:
		cf_update_irq_resource((struct irq *)resource);
		break;
	    case rt_dma:
		cf_update_dma_resource((struct dma *)resource);
		break;
	    case rt_port:
		cf_update_port_resource((struct port *)resource);
		break;
	}
	resource = resource->next;
    }

    return;
}


/****+++***********************************************************************
*
* Function:     cf_update_memory_resource()
*
* Parameters:   data_struct	ptr to memory struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine updates the memory pointer parameter in a memory
*	data struct.
*
****+++***********************************************************************/

static void cf_update_memory_resource(data_struct)
    struct memory_address	*data_struct;
{
    int				i;
    int				index;
    unsigned long		*value_ptr;
    struct value		**struct_ptr;
    struct value		*src_struct_ptr;


    /*******************
    * The index values are updated.
    *********************/
    index = cf_get_index(&(data_struct->index), glb_src);
    cf_set_index(&(data_struct->index), glb_dest, index);

    /*******************
    * If the step value is set then use the range values 
    * part of the union.
    *********************/
    if (data_struct->step) {

	/*******************
	* Get the address of the destination.
	*********************/
	value_ptr = &(data_struct->u.range.current);

	/*******************
	* The correct memory size is determined and copied.
	*********************/
	*value_ptr = data_struct->u.range.min + data_struct->step * index;
    }

    /*******************
    * If no step value then use the linked lists.
    *********************/
    else {

	/******************
	* Get the address of the destination.
	******************/
	struct_ptr = &(data_struct->u.list.current);

	/******************
	* The linked list is followed and the value copied.
	******************/
	src_struct_ptr = data_struct->u.list.values;
	for (i = 0; i < index; i++)
	    src_struct_ptr = src_struct_ptr->next;
	*struct_ptr = src_struct_ptr;
    }

}

/****+++***********************************************************************
*
* Function:     cf_update_irq_resource()
*
* Parameters:   irq		ptr to an irq struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Updates the irq data struct.
*
****+++***********************************************************************/

static void cf_update_irq_resource(irq)
    struct irq 		*irq;
{
    int	    		i;
    int	    		index;
    struct value   	*range;
    struct value   	**range_ptr;


    /*******************
    * The index values are always updated.
    *********************/
    index = cf_get_index(&(irq->index), glb_src);
    cf_set_index(&(irq->index), glb_dest, index);

    /*******************
    * The address of the config or current pointer is loaded first.
    *********************/
    range_ptr = &(irq->current);

    /*******************
    * The range list is stepped thru until the correct one is found.
    *********************/
    range = irq->values;
    index = cf_get_index(&(irq->index), glb_src);
    for (i = 0; i < index; i++)
	range = range->next;

    /*******************
    * The pointer is updated.
    *********************/
    *range_ptr = range;
}


/****+++***********************************************************************
*
* Function:     cf_update_dma_resource()
*
* Parameters:   dma		ptr to a dma struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Updates the dma struct.
*
****+++***********************************************************************/

static void cf_update_dma_resource(dma)
    struct dma 		*dma;
{
    int	    		i;
    int	    		index;
    struct value   	*range;
    struct value   	**range_ptr;


    /*******************
    * The index values are always updated.
    *********************/
    index = cf_get_index(&(dma->index), glb_src);
    cf_set_index(&(dma->index), glb_dest, index);

    /*******************
    * The address of the config or current pointer is loaded first.
    *********************/
    range_ptr = &(dma->current);

    /*******************
    * The range list is stepped thru until the correct one is found.
    *********************/
    range = dma->values;
    index = cf_get_index(&(dma->index), glb_src);
    for (i = 0; i < index; i++)
	range = range->next;

    /*******************
    * The pointer is updated.
    *********************/
    *range_ptr = range;
}


/****+++***********************************************************************
*
* Function:     cf_update_port_resource()
*
* Parameters:   port		ptr to a port struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Updates the port data struct.
*
****+++***********************************************************************/

static void cf_update_port_resource(port)
    struct port		*port;
{
    int			i;
    int			index;
    unsigned int	*range_value;
    struct value	*range;
    struct value	**range_ptr;


    /*******************
    * The index values are always updated.
    *********************/
    index = cf_get_index(&(port->index), glb_src);
    cf_set_index(&(port->index), glb_dest, index);

    /*******************
    * If there is a step then use the range data.
    *********************/
    if (port->step) {
	range_value = &(port->u.range.current);
	*range_value = port->u.range.min + port->step * index;
    }

    /*******************
    * If no step then use the range list linked lists.
    *********************/
    else {

	/******************
	* The address of the pointer is loaded first.
	******************/
	range_ptr = &(port->u.list.current);

	/******************
	* The range list is stepped thru.
	******************/
	range = port->u.list.values;
	for (i = 0; i < index; i++)
	    range = range->next;

	/******************
	* The pointer is updated.
	******************/
	*range_ptr = range;
    }
}


/****+++***********************************************************************
*
* Function:     cf_get_index
*
* Parameters:   index_ptr 	pointer to the index struct to use
*		index_type 	type of index to fetch
*
* Used:		internal only
*
* Returns:      the data struct type
*
* Description:
*
*	This routine fetches the index value from the data struct.
*
****+++***********************************************************************/

static int cf_get_index(index_ptr, index_type)
    struct index	*index_ptr;
    int			index_type;
{

    if (index_type == CF_EEPROM_NDX)
	return(index_ptr->eeprom);
    return(index_ptr->config);

}






/****+++***********************************************************************
*
* Function:     cf_set_index()
*
* Parameters:   index_ptr 	pointer to the index struct to use
*		index_type 	type of index to set
*		new_value 	new value to assign to the index
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine sets the index value for the index type to the
*	new value.
*
****+++***********************************************************************/

static void cf_set_index(index_ptr, index_type, new_value)
    struct index	*index_ptr;
    int			index_type;
    int			new_value;
{

    if (index_type == CF_CURRENT_NDX)
	index_ptr->current = new_value;
}


/****+++***********************************************************************
*
* Function:     cf_check_amp_load()
*
* Parameters:   system		ptr to the system struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine determines the total amount of amps being used by
*	the boards in the system and compares it against the amount the
*	system board can supply. If the amount is excessive then a
*	warning message is printed. Only print the warning the first time
*	we detect a problem. If the problem has been fixed, let the user
*	know it is ok now.
*
****+++***********************************************************************/

static void cf_check_amp_load(system)
    struct system	*system;
{
    unsigned long	amps;
    struct  board	*board;
    struct  group	*group;
    struct  function	*function;
    struct  subfunction *subfunction;


    /*******************
    * If the system amperage was not specified, don't bother.
    *******************/
    if (system->amperage == 0)
	return;

    /*******************
    * For all the boards and choices, add up the total amperage.
    * Note: only the current choice's amperage needs are included.
    *********************/
    amps = 0;
    board = system->boards;
    while (board != NULL) {

	amps += board->amperage;
	group = board->groups;

	while (group != NULL) {
	    function = group->functions;
	    while (function != NULL) {
		subfunction = function->subfunctions;
		while (subfunction != NULL) {
		    amps += subfunction->current->amperage;
		    subfunction = subfunction->next;
		}
		function = function->next;
	    }
	    group = group->next;
	}

	board = board->next;
    }

    /*******************
    * If the total used is greater then the system amount then display a
    * warning message (if we haven't already displayed one). Keep an indicator
    * of whether a warning has been displayed.
    *********************/
    if (amps > system->amperage) {
	if (!system->amp_overload) {
	    err_add_num_parm(1, (unsigned)system->amperage);
	    err_add_num_parm(2, (unsigned)amps);
	    err_handler(AMP_OVERLOAD_DETECTED_ERRCODE);
	    system->amp_overload = 1;
	}
    }

    /****************
    * We used to have an overload problem (and we displayed a warning for it
    * when it was detected). The overload no longer exists, so let the user
    * know the problem has been corrected.
    *****************/
    else if (system->amp_overload) {
	err_handler(AMP_OVERLOAD_FIXED_ERRCODE);
	system->amp_overload = 0;
    }

}


/****+++***********************************************************************
*
* Function:     cf_lock_reset_restore()
*
* Parameters:   op   		type of operation
*				    CF_LOCK_MODE
*				    CF_LOCK_CHOICE_MODE
*				    CF_UNLOCK_MODE
*				    CF_EEPROM_UNLOCK
*				    CF_RESET_MODE    -- reset to man defaults
*		type   		type of data struct:
*		   		   CF_SYSTEM_LEVEL
*		   		   CF_BOARD_LEVEL
*		sys		ptr to the sys structure (CF_SYSTEM_LEVEL)
*		brd		ptr to the board structure (CF_BOARD_LEVEL)
*
* Used:		external only
*
* Returns:      0		successful
*		-1		attempt to reset/restore a board which
*				was locked -- if this was done with a
*				SYSTEM_LEVEL call, all unlocked boards
*				in the system are reset or restored as
*				requested
*
* Description:
*
*	This routine will either lock, reset or restore a portion of
*	the config data structure. The operation starts at the type
*	provided and globally encompasses all sublevels. The only error
*	is if a user tries to reset/restore a board which is locked. In
*	this case, -1 is returned and the other boards (which are not locked)
*	are reset or restored as requested.
*
****+++***********************************************************************/

int cf_lock_reset_restore(op, type, sys, brd)
    int			op;
    int			type;
    struct system	*sys;
    struct board	*brd;
{
    struct  board	*board;
    struct  group	*group;
    struct  function	*function;
    struct  subfunction	*subf;


    /*************
    * Set up the data pointer values for board.
    *************/
    if (type == CF_SYSTEM_LEVEL)
	board = sys->boards;
    else
	board = brd;

    /*************
    * Walk through each of the boards in the system (or this board only
    * if this is board mode).
    *************/
    while (board != NULL) {

	/*************
	* Set up the board locked status.
	*************/
	if ( (op == CF_LOCK_MODE) || (op == CF_LOCK_CHOICE_MODE) )
	    board->locked = 1;
	else if ((op == CF_UNLOCK_MODE) || (op == CF_EEPROM_UNLOCK))
	    board->locked = 0;

	/************
	* Walk through and handle each of the subfunctions for this
	* board.
	************/
	group = board->groups;
	while (group != NULL) {
	    function = group->functions;
	    while (function != NULL) {
		subf = function->subfunctions;
		while (subf != NULL) {
		    cf_operate_on_subfunction(op, subf);
		    subf = subf->next;
		}
		function = function->next;
	    }
	    group = group->next;
	}

	/*************
	* If this is board mode, exit. Otherwise, go to the next 
	* board.
	*************/
	if (type == CF_BOARD_LEVEL)
	    break;
	board = board->next;

    }

    return(0);

}


/****+++***********************************************************************
*
* Function:     cf_operate_on_subfunction()
*
* Parameters:   op		lock, unlock, reset, or restore
*		subfunction	ptr to subfunction struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine will either lock, unlock, reset or restore the config data
*	structure starting at the subfunction level.
*
****+++***********************************************************************/

static void cf_operate_on_subfunction(op, subfunction)
    int			op;
    struct subfunction 	*subfunction;
{

    if ( (op == CF_LOCK_MODE) || (op == CF_LOCK_CHOICE_MODE) )
	subfunction->status = is_locked;

    else if (op == CF_EEPROM_UNLOCK)
	subfunction->status = is_eeprom;

    else if (op == CF_UNLOCK_MODE)
	subfunction->status = is_user;	

    /***************
    * Handle reset mode. Also handle restore mode if there is nothing
    * to restore to (no valid eeprom data).
    ***************/
    else if (subfunction->index.eeprom == FAILURE) {
	subfunction->status = is_free;
	if (subfunction->index.current != FAILURE) {
	    subfunction->index.current = 0;
	    subfunction->current = subfunction->choices;
	}
    }

    cf_operate_on_choice(op, subfunction->current);
}


/****+++***********************************************************************
*
* Function:     cf_operate_on_choice()
*
* Parameters:   op		lock, unlock, reset, or restore
*		choice     	ptr to choice struct
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*	This routine will either lock, unlock, reset or restore the config data
*	structure starting at the choice level.
*
****+++***********************************************************************/

static void cf_operate_on_choice(op, choice)
    int				op;
    struct choice		*choice;
{
    struct resource_group  	*resource_group;


    if (op == CF_LOCK_MODE)
	choice->status = is_locked;

    else if (op == CF_LOCK_CHOICE_MODE)  {
	choice->status = is_locked;
	return;
    }

    else if (op == CF_EEPROM_UNLOCK)
	choice->status = is_eeprom;

    else if (op == CF_UNLOCK_MODE)
	choice->status = is_user;	

    /***************
    * Handle restore mode if there is nothing
    * to restore to (no valid eeprom data).
    ***************/
    else if (choice->index.eeprom == FAILURE) {

	choice->status = is_free;

	if (choice->index.current != FAILURE) {
	    choice->index.current = 0;
	    choice->current = choice->subchoices;
	}

	if (choice->totalmem != NULL)
	    if (choice->totalmem->step == 0)
		choice->totalmem->actual = choice->totalmem->u.values->min;
	    else
		choice->totalmem->actual = choice->totalmem->u.range.min;
    }

    if (choice->primary != NULL) {
	resource_group = choice->primary->resource_groups;
	while (resource_group != NULL) {
	    cf_operate_on_resource_group(op, resource_group);
	    resource_group = resource_group->next;
	}
    }

    if (choice->subchoices != NULL) {
	resource_group = choice->current->resource_groups;
	while (resource_group != NULL) {
	    cf_operate_on_resource_group(op, resource_group);
	    resource_group = resource_group->next;
	}
    }
}


/****+++***********************************************************************
*
* Function:     cf_operate_on_resource_group()
*
* Parameters:   op			lock, unlock, reset, or restore
*		resource_group     	ptr to resource_group struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine will either lock, unlock, reset or restore the config data
*	structure starting at the resource_group level.
*
****+++***********************************************************************/

static void cf_operate_on_resource_group(op, resource_group)
    int				op;
    struct resource_group	*resource_group;
{
    struct resource		*resource;


    if (op == CF_LOCK_MODE)
	resource_group->status = is_locked;

    else if (op == CF_EEPROM_UNLOCK)
	resource_group->status = is_eeprom;

    else if (op == CF_UNLOCK_MODE)
	resource_group->status = is_user;	

    /***************
    * Handle restore mode if there is nothing
    * to restore to (no valid eeprom data).
    ***************/
    else if (op == CF_RESET_MODE || resource_group->index.eeprom == FAILURE) {
	resource_group->status = is_free;
	if (resource_group->index.current != FAILURE)
	    resource_group->index.current = 0;
    }

    resource = resource_group->resources;
    while (resource != NULL) {
	cf_operate_on_resource(op, resource);
	resource = resource->next;
    }
}


/****+++***********************************************************************
*
* Function:     cf_operate_on_resource()
*
* Parameters:   op			lock, unlock, reset, or restore
*		resource     		ptr to resource struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine will either lock, unlock, reset or restore the config data
*	structure starting at the resource level.
*
****+++***********************************************************************/

static void cf_operate_on_resource(op, resource)
    int			op;
    struct resource    	*resource;
{
    int	    		i;
    struct  irq	    	*irq;
    struct  dma	    	*dma;
    struct  port    	*port;
    struct  memory  	*memory;
    struct  value   	*value;


    if ((op==CF_LOCK_MODE) || (op==CF_UNLOCK_MODE) || (op==CF_EEPROM_UNLOCK))
	return;

    /***************
    * Handle reset mode. Also handle restore mode if there is nothing
    * to restore to (no valid eeprom data).
    ***************/
    if ( (op == CF_RESET_MODE) || (resource->parent->index.eeprom == FAILURE) ) {

	switch (resource->type) {

	    case rt_irq:
		irq = (struct irq *)resource;
		irq->index.current = 0;
		irq->current = irq->values;
		break;

	    case rt_dma:
		dma = (struct dma *)resource;
		dma->index.current = 0;
		dma->current = dma->values;
		break;

	    case rt_port:
		port = (struct port *)resource;
		port->index.current = 0;
		if (port->step)
		    port->u.range.current = port->u.range.min;
		else
		    port->u.list.current = port->u.list.values;
		break;

	    case rt_memory:

		memory = (struct memory *)resource;
		if (resource->parent->parent->parent->totalmem != NULL)
		    if (memory->memory.step == 0) {
			for (value = memory->memory.u.list.values, i = 0; 
				value->init_index != 1;
				value = value->next, i++)
				;
			memory->memory.u.list.current = value;
			memory->memory.index.current = i;
		    }
		    else {
			memory->memory.u.range.current = 
			    memory->memory.u.range.max;
			memory->memory.index.current = 
			    memory->memory.index.total - 1;
		    }

		else {
		    memory->memory.index.current = 0;
		    if (memory->memory.step)
			memory->memory.u.range.current =
			    memory->memory.u.range.min;
		    else
			memory->memory.u.list.current =
			    memory->memory.u.list.values;
		}

		resource->parent->index.current = 
		    memory->memory.index.current * memory->memory.index_value;

		if (memory->address != NULL)

		    if (resource->parent->type == rg_link) {

			memory->address->index.current = 
				    memory->memory.index.current;

			if (memory->address->step == NULL) {
			    for (i = 0, value = memory->address->u.list.values;
				    i != memory->address->index.current;
				    value = value->next, i++)
				    ;
			    memory->address->u.list.current = value;
			}
			else
			    memory->address->u.range.current = 
				    memory->address->u.range.max;
		    }

		    else {
			memory->address->index.current = 0;
			if (memory->address->step)
			    memory->address->u.range.current =
				memory->address->u.range.min;
			else
			    memory->address->u.list.current =
				memory->address->u.list.values;
		    }

		break;
	}
    }

}


/****+++***********************************************************************
*
* Function:     cf_configure()
*
* Parameters:   system 		pointer to the system data structure
*		op 		type of operation being done when called:
*		    CF_ADD_OP -- adding a board
*		    CF_DELETE_OP -- removing a board
*		    CF_MOVE_OP -- moving a board
*		    CF_EDIT_OP -- changing functions/resources
*
*		    CF_SAVE_OP -- exiting, save the configuration (unused now)
*		    CF_VERIFY_OP -- manual verify or fast config (unused now)
*		    CF_MANUAL_AUTO_OP -- chg to auto verify (unused now)
*		    CF_RESET_OP -- from revert, reset, and unlock (unused now)
*
* Used:		external only
*
* Returns:      0			Successful -- system is configured
*		-1			Could not get system configured
*		-2			System configured ok, board that was
*					trying to add not included
*
* Description:
*
*	This routine controls how conflict resolution is handled based
*	on who is calling it and the program mode.
*
*	Note that, currently, program_mode.auto_configure is always 1.
*
****+++***********************************************************************/

cf_configure(system, op)
    struct system	*system;
    int			op;
{
    struct board   	*board;
#ifdef MANUAL_VERIFY
    extern struct pmode	program_mode;
#endif


    /*******************
    * Set all of the "current" indices to 0 -- start config() from scratch.
    *******************/
    cf_reset_noncurrent_paths(system);

    /*******************
    * Fixup the slot specific port addresses for any operations which may have
    * changed slot assignments.
    *********************/
    if ( (op == CF_MOVE_OP)  ||  (op == CF_ADD_OP) )
	set_port_address(system);

#ifdef MANUAL_VERIFY
    /*******************
    * If manual mode and we aren't explicitly doing a verify, just run a
    * detection.
    *********************/
    if ( (!program_mode.auto_verify) && 
         (op != CF_VERIFY_OP)  &&
         (op != CF_SAVE_OP)  &&
         (op != CF_MANUAL_AUTO_OP) )  {
	config(system, DETECTION);
	return(0);
    }
#endif

#ifdef MANUAL_VERIFY
    /*******************
    * Try to resolve a good configuration unless this is an auto-verify save
    * (should already be done in this case).
    *********************/
    if (!( (program_mode.auto_verify) && (op == CF_SAVE_OP) ) )
	config(system, RESOLUTION);
#else
    /*******************
    * Try to resolve a good configuration.
    *******************/
    config(system, RESOLUTION);
#endif

    /*******************
    * If there is a valid configuration ...
    *********************/
    if (system->configured) {

	/*************
	* If we are saving the configuration, make sure we haven't
	* used too much current. If there is a problem, cf_check_amp_load()
	* will display a warning -- we will go on regardless.
	*************/
#ifdef MANUAL_VERIFY
	if (op == CF_SAVE_OP)
	    system->amp_overload = 0;
#endif
	cf_check_amp_load(system);

	/**************
	* Everything is cool, so copy from config to current.
	**************/
	(void)cf_process_system_indexes(system, CF_CONFIG_NDX, CF_CURRENT_NDX,
					CF_COPY_MODE, (void *)NULL);
	set_share_and_cache(system);

	return(0);
    }

#ifdef MANUAL_VERIFY
    /***************
    * System was not configured successfully and we are doing some sort
    * of manual operation. Detect what went wrong.
    *
    * Note: should not hit this case yet either
    ***************/
    else if ( (op == CF_VERIFY_OP)       ||
	      (op == CF_MANUAL_AUTO_OP)  || 
	      (op == CF_SAVE_OP) )  {
	config(system, DETECTION);
	return(-1);
    }
#endif

    /*******************
    * If we didn't get a valid configuration and we were adding a board,
    * release the last board that was added.
    *********************/
    else if (op == CF_ADD_OP)  {
	board = system->boards;
	while (board->next != NULL)
	    board = board->next;
	system->entries -= board->entries;
	del_release_board(system, board);
	config(system, RESOLUTION);
	(void)cf_process_system_indexes(system, CF_CONFIG_NDX, CF_CURRENT_NDX,
					CF_COPY_MODE, (void *)NULL);
	set_share_and_cache(system);
	return(-2);
    }

    /*******************
    * We did not get a valid configuration and we were trying one of these
    * operations:
    *     delete, move, edit, or reset
    * We can't do anything but exit.
    *******************/
    else
	return(-1);

}


/****+++***********************************************************************
*
* Function:     cf_reset_noncurrent_paths()
*
* Parameters:   system		ptr to a system data structure
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This routine resets the indexes and current pointers to choice
*	0 for all paths in the system data struct that are NOT the
*	current path. This insures that config() will start over from
*	scratch when running.
*
****+++***********************************************************************/

static void cf_reset_noncurrent_paths(system)
    struct  system		*system;
{
    struct  board   		*board;
    struct  group   		*group;
    struct  function		*function;
    struct  subfunction 	*subf;
    struct  choice		*choice;
    struct  subchoice		*subch;
    struct  resource_group  	*resgrp;


    board = system->boards;
    while (board != NULL) {
	group = board->groups;
	while (group != NULL) {
	    function = group->functions;
	    while (function != NULL) {
		subf = function->subfunctions;
		while (subf != NULL) {
		    choice = subf->choices;
		    while (choice != NULL) {
			if (choice != subf->current) {
			    choice->status = is_free;
			    if (choice->primary != NULL) {
				resgrp = choice->primary->resource_groups;
				while (resgrp != NULL) {
				    cf_operate_on_resource_group(CF_RESET_MODE,
								 resgrp);
				    resgrp = resgrp->next;
				}
			    }
			}
			subch = choice->subchoices;
			while (subch != NULL) {
			    if ( (choice != subf->current) ||
				 (subch != choice->current) ) {
				resgrp = subch->resource_groups;
				while (resgrp != NULL) {
				    cf_operate_on_resource_group(CF_RESET_MODE,
							         resgrp);
				    resgrp = resgrp->next;
				}
			    }
			    subch = subch->next;
			}
			choice = choice->next;
		    }
		    subf = subf->next;
		}
		function = function->next;
	    }
	    group = group->next;
	}
	board = board->next;
    }
}
