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
*                       src/release.c
*
*   This module contains various routines for releasing memory space that
*   is occupied by a system data structure.
*
*     	del_release_system()	-- mn_util.c, open_save.c
*     	del_release_board()	-- add.c, cfgload.c, mrbrd.c
*
**++***************************************************************************/


#include <stdio.h>
#include <memory.h>
#include "config.h"


/* Functions declared in this file */
void  			del_release_system();
void  			del_release_board();
#ifdef VIRT_BOARD
static	void 		del_reorder();
#endif
static	void 		del_release_group();
static	void 		del_release_function();
static	void 		del_release_subfunction();
static	void 		del_release_choice();
static	void 		del_release_subchoice();
static	void 		del_release_resgrp();
static	void 		del_release_resource();
static	void 		del_release_init();
static	void 		del_release_jumper();
static	void 		del_release_bswitch();
static	void 		del_release_space();

extern void             mn_trapfree();

#ifdef VIRT_BOARD
extern unsigned int	glb_virtual;
extern unsigned int	glb_logical;
extern unsigned int	glb_embedded;
#endif

/****+++***********************************************************************
*
* Function:     del_release_system()
*
* Parameters:   system		ptr to system struct to release
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   The del_release_system function will release all boards from the
*   specified system board and clear the system board entry.
*
****+++***********************************************************************/

void del_release_system(system)
    struct system 	*system;
{
    struct board 	*board;
    struct board	*next_board;
#ifdef SYS_CACHE
    struct cache_map 	*cache_map;
    struct cache_map	*next_cache_map;
#endif


    for (board = system->boards; board; board = next_board) {
	next_board = board->next;
	del_release_board(system, board);
    }

#ifdef SYS_CACHE
    for (cache_map = system->cache_map; cache_map; cache_map = next_cache_map) {
	next_cache_map = cache_map->next;
	mn_trapfree((void *)cache_map);
    }
#endif

    del_release_space(system->irq);
    del_release_space(system->dma);
    del_release_space(system->port);
    del_release_space(system->memory);

    (void)memset((void *)system, 0, sizeof(*system));
}


/****+++***********************************************************************
*
* Function:     del_release_board()
*
* Parameters:   system
*		board
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/


void del_release_board(system, board)
    struct system 	*system;
    struct board 	*board;
{
    struct board 	*bp;
    struct group 	*group;
    struct group 	*next_group;
    struct ioport 	*ioport;
    struct ioport 	*next_ioport;
    struct jumper 	*jumper;
    struct jumper 	*next_jumper;
    struct bswitch 	*bswitch;
    struct bswitch 	*next_bswitch;
    struct software 	*software;
    struct software 	*next_software;


    if ((bp = system->boards) == board)
	system->boards = board->next;
    else
	for (; bp; bp = bp->next)
	    if (bp->next == board) {
		bp->next = board->next;
		break;
	    }

    if (board->slot_number >= 0 && board->slot_number < MAX_NUM_SLOTS)
	system->slot[board->slot_number].occupied = 0;

#ifdef VIRT_BOARD
    del_reorder(system, board->eisa_slot, board->slot_number);
#endif

    for (group = board->groups; group; group = next_group) {
	next_group = group->next;
	del_release_group(group);
    }

    for (ioport = board->ioports; ioport; ioport = next_ioport) {
	next_ioport = ioport->next;
	mn_trapfree((void *)ioport);
    }

    for (jumper = board->jumpers; jumper; jumper = next_jumper) {
	next_jumper = jumper->next;
	del_release_jumper(jumper);
    }

    for (bswitch = board->switches; bswitch; bswitch = next_bswitch) {
	next_bswitch = bswitch->next;
	del_release_bswitch(bswitch);
    }

    for (software = board->softwares; software; software = next_software) {
	next_software = software->next;
	mn_trapfree((void *)software->description);
	mn_trapfree((void *)software);
    }

    mn_trapfree((void *)board->id);
    mn_trapfree((void *)board->mfr);
    mn_trapfree((void *)board->name);
    mn_trapfree((void *)board->category);
    mn_trapfree((void *)board->comments);
    mn_trapfree((void *)board->help);
    mn_trapfree((void *)board->slot_tag);
    mn_trapfree((void *)board);
}


#ifdef VIRT_BOARD
/****+++***********************************************************************
*
* Function:     del_reorder()
*
* Parameters:   system
*		eisa_slot
*		slot_number
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

static void del_reorder(system, eisa_slot, slot_number)
    struct system 	*system;
    int 		eisa_slot;
    int 		slot_number;
{
    register int    	slot;
    struct board 	*board;


    if (slot_number >= MAX_NUM_SLOTS) {

	glb_virtual = MIN_VIRTUAL_SLOT;
	glb_embedded = MIN_EMBEDDED_SLOT;

	for (board = system->boards; board; board = board->next) {
	    if (board->slot_number > slot_number)
		board->slot_number--;

	    slot = board->slot_number;
	    if (slot > MIN_VIRTUAL_SLOT) {
		if (slot > glb_virtual)
		    glb_virtual = slot;
	    }
	    else if (slot > MIN_EMBEDDED_SLOT)
		if (slot > glb_embedded)
		    glb_embedded = slot;
	}
    }

    if (eisa_slot >= MAX_NUM_SLOTS) {
	glb_logical = MIN_LOGICAL_SLOT;

	for (board = system->boards; board; board = board->next) {
	    if (board->eisa_slot > eisa_slot)
		board->eisa_slot--;

	    if (board->eisa_slot > glb_logical)
		glb_logical = board->eisa_slot;
	}
    }
}
#endif


/****+++***********************************************************************
*
* Function:     del_release_group()
*
* Parameters:   group             
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

static void del_release_group(group)
    struct group 	*group;
{
    struct function 	*function;
    struct function	*next_function;


    for (function = group->functions; function; function = next_function) {
	next_function = function->next;
	del_release_function(function);
    }

    mn_trapfree((void *)group->name);
    mn_trapfree((void *)group->type);
    mn_trapfree((void *)group);
}




/****+++***********************************************************************
*
* Function:     del_release_function()
*
* Parameters:   function          
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

static void del_release_function(function)
    struct function 	*function;
{
    struct subfunction 	*subfunction;
    struct subfunction	*next_subfunction;


    for (subfunction = function->subfunctions; subfunction; subfunction = next_subfunction) {
	next_subfunction = subfunction->next;
	del_release_subfunction(subfunction);
    }

    mn_trapfree((void *)function->name);
    mn_trapfree((void *)function->type);
    mn_trapfree((void *)function->connection);
    mn_trapfree((void *)function->comments);
    mn_trapfree((void *)function->help);
    mn_trapfree((void *)function);
}


/****+++***********************************************************************
*
* Function:     del_release_subfunction()
*
* Parameters:   subfunction       
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

static void del_release_subfunction(subfunction)
    struct subfunction 	*subfunction;
{
    struct choice 	*choice;
    struct choice	*next_choice;


    for (choice = subfunction->choices; choice; choice = next_choice) {
	next_choice = choice->next;
	del_release_choice(choice);
    }

    mn_trapfree((void *)subfunction->name);
    mn_trapfree((void *)subfunction->type);
    mn_trapfree((void *)subfunction->connection);
    mn_trapfree((void *)subfunction->comments);
    mn_trapfree((void *)subfunction->help);
    mn_trapfree((void *)subfunction);
}




/****+++***********************************************************************
*
* Function:     del_release_choice()
*
* Parameters:   choice            
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

static void del_release_choice(choice)
    struct choice 	*choice;
{
    struct subchoice 	*subchoice;
    struct subchoice	*next_subchoice;
    struct value 	*value;
    struct value	*next_value;
    struct portvar 	*portvar;
    struct portvar	*next_portvar;


    for (portvar = choice->portvars; portvar; portvar = next_portvar) {
	next_portvar = portvar->next;
	mn_trapfree((void *)portvar);
    }

    mn_trapfree((void *)choice->freeform);

    if (choice->primary)
	del_release_subchoice(choice->primary);

    for (subchoice=choice->subchoices; subchoice; subchoice = next_subchoice) {
	next_subchoice = subchoice->next;
	del_release_subchoice(subchoice);
    }

    if (choice->totalmem) {
	if (!choice->totalmem->step)
	    for (value=choice->totalmem->u.values; value; value = next_value) {
		next_value = value->next;
		mn_trapfree((void *)value);
	    }
	mn_trapfree((void *)choice->totalmem);
    }

    mn_trapfree((void *)choice->help);
    mn_trapfree((void *)choice->name);
    mn_trapfree((void *)choice->subtype);
    mn_trapfree((void *)choice);
}


/****+++***********************************************************************
*
* Function:     del_release_subchoice()
*
* Parameters:   subchoice         
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

static void del_release_subchoice(subchoice)
    struct subchoice 		*subchoice;
{
    struct resource_group 	*resgrp;
    struct resource_group 	*next_resgrp;


    for (resgrp = subchoice->resource_groups; resgrp; resgrp = next_resgrp) {
	next_resgrp = resgrp->next;
	del_release_resgrp(resgrp);
    }

    mn_trapfree((void *)subchoice);
}





/****+++***********************************************************************
*
* Function:     del_release_resgrp()
*
* Parameters:   resgrp            
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

static void del_release_resgrp(resgrp)
    struct resource_group 	*resgrp;
{
    struct resource 		*resource;
    struct resource 		*next_resource;
    struct init 		*init;
    struct init 		*next_init;


    for (resource = resgrp->resources; resource; resource = next_resource) {
	next_resource = resource->next;
	del_release_resource(resource);
    }

    for (init = resgrp->inits; init; init = next_init) {
	next_init = init->next;
	del_release_init(init);
    }

    mn_trapfree((void *)resgrp);
}


/****+++***********************************************************************
*
* Function:     del_release_resource()
*
* Parameters:   resource          
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

static void del_release_resource(resource)
    struct resource 	*resource;
{
    struct irq 		*irq;
    struct dma 		*dma;
    struct port 	*port;
    struct memory 	*memory;
    struct value 	*value;
    struct value 	*next_value;


    switch (resource->type) {
	case rt_irq:
	    irq = (struct irq *)resource;
	    for (value = irq->values; value; value = next_value) {
		next_value = value->next;
		mn_trapfree((void *)value);
	    }
	    break;

	case rt_dma:
	    dma = (struct dma *)resource;
	    for (value = dma->values; value; value = next_value) {
		next_value = value->next;
		mn_trapfree((void *)value);
	    }
	    break;

	case rt_port:
	    port = (struct port *)resource;
	    if (!port->step)
		for (value = port->u.list.values; value; value = next_value) {
		    next_value = value->next;
		    mn_trapfree((void *)value);
		}
	    break;

	case rt_memory:
	    memory = (struct memory *)resource;
	    if (!memory->memory.step)
		for (value = memory->memory.u.list.values; value; value = next_value) {
		    next_value = value->next;
		    mn_trapfree((void *)value);
		}

	    if (memory->address) {
		if (!memory->address->step)
		    for (value = memory->address->u.list.values; value; value = next_value) {
			next_value = value->next;
			mn_trapfree((void *)value);
		    }
		mn_trapfree((void *)memory->address);
	    }
	    break;
    }

    mn_trapfree((void *)resource->share_tag);
    mn_trapfree((void *)resource);
}


/****+++***********************************************************************
*
* Function:     del_release_init()
*
* Parameters:   init              
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

static void del_release_init(init)
    struct init 	*init;
{
    struct init_value 	*init_value;
    struct init_value 	*next_init_value;


    for (init_value = init->init_values; init_value; init_value = next_init_value) {
	next_init_value = init_value->next;
	if (dt_string == init->data_type)
	    mn_trapfree((void *)init_value->u.parameter);
	mn_trapfree((void *)init_value);
    }

    mn_trapfree((void *)init);
}




/****+++***********************************************************************
*
* Function:     del_release_jumper()
*
* Parameters:   jumper           
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

static void del_release_jumper(jumper)
    struct jumper 	*jumper;
{
    struct label 	*label;
    struct label 	*next_label;


    for (label = jumper->labels; label; label = next_label) {
	next_label = label->next;
	mn_trapfree((void *)label->string);
	mn_trapfree((void *)label);
    }

    mn_trapfree((void *)jumper->name);
    mn_trapfree((void *)jumper->comments);
    mn_trapfree((void *)jumper->help);
    mn_trapfree((void *)jumper);
}


/****+++***********************************************************************
*
* Function:     del_release_bswitch()
*
* Parameters:   bswitch           
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

static void del_release_bswitch(bswitch)
    struct bswitch 	*bswitch;
{
    struct label 	*label;
    struct label 	*next_label;


    for (label = bswitch->labels; label; label = next_label) {
	next_label = label->next;
	mn_trapfree((void *)label->string);
	mn_trapfree((void *)label);
    }

    mn_trapfree((void *)bswitch->name);
    mn_trapfree((void *)bswitch->comments);
    mn_trapfree((void *)bswitch->help);
    mn_trapfree((void *)bswitch);
}




/****+++***********************************************************************
*
* Function:     del_rlease_space()
*
* Parameters:   space             
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

static void del_release_space(space)
    struct space 	*space;
{
    struct space 	*next_space;


    for (; space; space = next_space) {
	next_space = space->next;
	mn_trapfree((void *)space);
    }
}
