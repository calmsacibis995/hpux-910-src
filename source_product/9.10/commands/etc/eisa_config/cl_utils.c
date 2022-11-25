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
*                           src/cl_utils.c
*
*   Routine to traverse data structure and compile the
*   selected values of the selected init statements into single values
*   stored in the ioport, jumper, switch, and software blocks.
*
*	compile_inits()		-- init.c, open_save.c, show.c
*	set_share_and_cache()	-- cf_util.c
*	set_port_address()	-- cf_util.c
*	set_subchoice()		-- config.c
*
**++***************************************************************************/


#include <stdio.h>
#include <string.h>
#include "config.h"

void 		compile_inits();
void 	    	set_share_and_cache();
void 	    	set_port_address();
void 	    	set_subchoice(); 
static void 	do_subchoice();
static void 	do_init();
#ifdef SYS_CACHE
static int  	is_cache();
static int  	granularity_conflict(); 
#endif

extern void 	*mn_trapcalloc();
extern void 	*mn_traprealloc();
extern void 	mn_trapfree();


/****+++***********************************************************************
*
* Function:     compile_inits()
*
* Parameters:   sys			system struct
*		flag         		0 --> do ports, switches, jumpers
*		             		1 --> do software stmts
*					2 --> do both + set up "initial"s
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	Routine to traverse data structure and compile the
*	selected values of the selected init statements into single values
*	stored in the ioport, jumper, switch, and software blocks.
*
****+++***********************************************************************/

void compile_inits(sys, flag)
    struct system 	*sys;
    int 		flag;
{
    struct board 	*board;
    struct ioport 	*ioport;
    struct jumper 	*jumper;
    struct bswitch 	*bswitch;
    struct software 	*software;
    struct group 	*group;
    struct function 	*func;
    struct subfunction 	*subf;
    struct choice 	*choice;
    struct portvar 	*portvar;


    /* traverse all iib's in all boards, setting previous to current */
    for (board=sys->boards ; board!=NULL ; board=board->next) {

	if ( (flag == 0) || (flag == 2) ) {

	    for (ioport=board->ioports ; ioport!=NULL ; ioport=ioport->next) {
		ioport->config_bits.previous = ioport->config_bits.current;
		ioport->config_bits.current = 0;
		ioport->referenced = 0;
		ioport->valid_address = (ioport->portvar_index == 0);
	    }

	    for (jumper=board->jumpers ; jumper!=NULL ; jumper=jumper->next) {
		jumper->config_bits.previous = jumper->config_bits.current;
		jumper->config_bits.current = 0;
		jumper->tristate_bits.previous = jumper->tristate_bits.current;
		jumper->tristate_bits.current = 0;
	    }

	    for (bswitch=board->switches; bswitch!=NULL; bswitch=bswitch->next){
		bswitch->config_bits.previous = bswitch->config_bits.current;
		bswitch->config_bits.current = 0;
	    }

	}

	if ( (flag == 1) || (flag == 2) ) {
	    for (software=board->softwares; software!=NULL; software=software->next) {
		if (software->previous != NULL)
		    mn_trapfree((void *)software->previous);
		software->previous = software->current;
		software->current = NULL;
	    }
	}

    }

    /* traverse all init statements in all boards, compiling values in iib's */
    for (board = sys->boards; board != NULL; board = board->next) {

	for (group = board->groups; group != NULL; group = group->next)

	    for (func = group->functions; func != NULL; func = func->next)

		for (subf = func->subfunctions; subf!=NULL; subf = subf->next) {

		    choice = subf->current;

		    if (choice->portvars != NULL)
			for (portvar = choice->portvars; portvar != NULL; portvar = portvar->next)
			    for (ioport = board->ioports; ioport != NULL; ioport = ioport->next)
				if (ioport->portvar_index == portvar->index) {
				    ioport->address = portvar->address;
				    ioport->slot_specific = portvar->slot_specific;
				    ioport->valid_address = 1;
				}

		    do_subchoice (choice->primary, flag);
		    do_subchoice (choice->current, flag);
		}

	for (ioport = board->ioports; ioport != NULL; ioport = ioport->next)
	    ioport->config_bits.current |= 
		(ioport->initval.reserved_mask & ioport->initval.forced_bits);

	for (bswitch = board->switches; bswitch != NULL; bswitch = bswitch->next)  {
	    bswitch->config_bits.current |= 
		(bswitch->initval.reserved_mask & bswitch->initval.forced_bits);
	    if (flag == 2)
		bswitch->config_bits.initial = bswitch->config_bits.current;
	}

	for (jumper = board->jumpers; jumper != NULL; jumper = jumper->next) {
	    jumper->config_bits.current |= 
		(jumper->initval.reserved_mask & jumper->initval.forced_bits);
	    jumper->tristate_bits.current |= 
		(jumper->initval.reserved_mask & jumper->initval.tristate_bits);
	    if (flag == 2) {
		jumper->config_bits.initial = jumper->config_bits.current;
		jumper->tristate_bits.initial = jumper->tristate_bits.current;
	    }
	}

    }

}


/****+++***********************************************************************
*
* Function:     do_subchoice()
*
* Parameters:   subchoice
*		flag
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

static void do_subchoice (subchoice, flag)
    struct subchoice 		*subchoice;
    int 			flag;
{
    struct resource_group 	*resource_group;
    struct init 		*init;
    void 			do_init ();


    if (subchoice == NULL)
	return;

    for (resource_group = subchoice->resource_groups; 
	 resource_group != NULL; 
	 resource_group = resource_group->next)
	for (init = resource_group->inits; init != NULL; init = init->next)
	    if (resource_group->type == rg_dlink) {
		if (init->memory)
		    do_init (init,
		             ((struct memory *)resource_group->resources)->memory.index.current,
			     flag);
		else
		    do_init (init, ((struct memory *)resource_group->resources)->address->index.current,
							flag);
	    }
	    else
		do_init (init, resource_group->index.current, flag);
}


/****+++***********************************************************************
*
* Function:     do_init()
*
* Parameters:   init
*		index
*		flag
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This is only called by do_subchoice().
*
****+++***********************************************************************/

static void do_init (init, index, flag)
    struct init 	*init;
    int 		index;
    int 		flag;
{
    unsigned long   	value;
    unsigned long   	n;
    int 		i;


    if ( ( (flag == 1) && (init->type != it_software) ) ||
	 ( (flag == 0) && (init->type == it_software) ) )
	return;

    /* use list of values */
    if (init->init_values != NULL) {
	for (i = 0, init->current = init->init_values; i < index; i++)
	    init->current = init->current->next;
	value = init->current->u.value;
    }

    /* if it is a rotary or slide switch, shift the value by the index
	    instead of adding or subtracting the index	*/
    else if ( (init->type == it_bswitch) &&
	      ( (init->ptr.bswitch->type == st_rotary) ||
	        (init->ptr.bswitch->type == st_slide) ) )
	if (init->range.min <= init->range.max)
	    value = init->range.current = init->range.min << index;
	else
	    value = init->range.current = init->range.min >> index;

    else {
	value = index;
	for (n = 1; !(n & init->mask); n <<= 1)
	    value <<= 1;
	if (init->range.min <= init->range.max)
	    value = init->range.current = init->range.min + value;
	else
	    value = init->range.current = init->range.min - value;
    }

    switch (init->data_type) {
	case dt_string:

	    if (init->ptr.software->current == NULL) {
		init->ptr.software->current = 
		    mn_trapcalloc(1, (unsigned)(strlen(init->current->u.parameter)+2));
		(void)strcpy (init->ptr.software->current,init->current->u.parameter);
		(void)strcat (init->ptr.software->current, "\n");
	    }

	    else {
		init->ptr.software->current = mn_traprealloc((void *)init->ptr.software->current,
				    (unsigned)(strlen (init->ptr.software->current) +
				    strlen (init->current->u.parameter) + 2));
		(void)strcat (init->ptr.software->current,init->current->u.parameter);
		(void)strcat (init->ptr.software->current, "\n");
	    }
	    break;

	case dt_tripole:
	    init->ptr.jumper->config_bits.current 
		 |= (init->mask & init->current->u.tripole.data_bits);
	    init->ptr.jumper->tristate_bits.current 
		 |= (init->mask & init->current->u.tripole.tristate_bits);
	    break;

	case dt_value:
	    switch (init->type) {
		case it_ioport:
		    init->ptr.ioport->config_bits.current |= (init->mask&value);
		    init->ptr.ioport->referenced = 1;
		    break;
		case it_jumper:
		    init->ptr.jumper->config_bits.current |= (init->mask&value);
		    break;
		case it_bswitch:
		    /* if it is a rotary/slide switch, don't or in the value */
		    if ((init->ptr.bswitch->type == st_rotary)
			 || (init->ptr.bswitch->type == st_slide))
			init->ptr.bswitch->config_bits.current = (init->mask & value);
		    else
			init->ptr.bswitch->config_bits.current |= (init->mask & value);
		    break;
	    }
	    break;
    }
}


/****+++***********************************************************************
*
* Function:     set_share_and_cache();
*
* Parameters:   sys
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	Traverse the space allocation structures and
*	determine which resources are sharing and or caching.  Set the 
*	corresponding bits in the data structure.
*
****+++***********************************************************************/

void set_share_and_cache (sys)
    struct system 	*sys;
{
    struct space 	**spaces;
    struct space 	*space;
    int 		n;
#ifdef SYS_CACHE
    struct space 	*nextspace;
    int 		not_done;
    union {
	struct resource *resource;
	struct memory   *memory;
    } ptr, nextptr;
#endif



    /* set sharing bits */
    for (spaces = &sys->irq, n = 0; n < 4; n++)
	for (space = spaces[n]; space != NULL; space = space->next)
	    if (space->next != NULL)
		if (space->sp_max >= space->next->sp_min) {
		    space->resource->sharing = 1;
		    space->next->resource->sharing = 1;
		}

#ifdef SYS_CACHE
    /* if no cache map, return */
    if (sys->cache_map == NULL)
	return;

    /* set caching bits */
    for (space = sys->memory; space != NULL; space = space->next) {
	ptr.resource = space->resource;
	ptr.memory->caching = ((ptr.memory->cache)
	     && (is_cache (sys->cache_map, space->sp_min, space->sp_max)));
    }

    /* resolve cache granularity conflicts */
    for (space = sys->memory; space != NULL && space->next != NULL; space = space->next) {
	ptr.resource = space->resource;
	nextspace = space->next;
	not_done = 1;
	while (not_done && nextspace != NULL) {
	    nextptr.resource = nextspace->resource;
	    if (not_done = granularity_conflict(sys->cache_map, space->sp_max, nextspace->sp_min))
		if (ptr.memory->caching != nextptr.memory->caching)
		    ptr.memory->caching = nextptr.memory->caching = 0;
	    nextspace = nextspace->next;
	}
    }
#endif

}


#ifdef SYS_CACHE
/****+++***********************************************************************
*
* Function:     is_cache()
*
* Parameters:   cache_map
*		min
*		max
*
* Used:		internal only
*
* Returns:
*
* Description:
*
*    xx
*
****+++***********************************************************************/

static int  is_cache (cache_map, min, max)
    struct cache_map 	*cache_map;
    unsigned long	min;
    unsigned long	max;
{
    int 		caching;
    int 		found;


    for (caching = 1, found = 0; cache_map != NULL; cache_map = cache_map->next)
	if ((min <= cache_map->address + cache_map->memory - 1) && (cache_map->address <= max)) {
	    found = 1;
	    caching = caching && cache_map->cache;
	}

    return (caching && found);
}
#endif






#ifdef SYS_CACHE
/****+++***********************************************************************
*
* Function:     granularity_conflict()
*
* Parameters:   cache_map
*		max
*		min
*
* Used:		internal only
*
* Returns:
*
* Description:
*
*    xx
*
****+++***********************************************************************/

static int  granularity_conflict (cache_map, max, min)
    struct cache_map 	*cache_map;
    unsigned long	max;
    unsigned long	min;
{
    unsigned long   	cache_map_max;


    while (max < cache_map->address && cache_map != NULL)
	cache_map = cache_map->next;

    cache_map_max = cache_map->address + cache_map->memory - 1;

    if (cache_map == NULL || max > cache_map_max || min > cache_map_max)
	return(0);

    return((max & -cache_map->step) == (min & -cache_map->step));
}
#endif


/****+++***********************************************************************
*
* Function:     set_port_address()
*
* Parameters:   system            
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    Compute the final address for all slot-specific port statements.
*
****+++***********************************************************************/

void set_port_address (system)
    struct system 	*system;
{
    struct board 	*board;
    struct group 	*group;
    struct function 	*function;
    struct subfunction 	*subfunction;
    struct choice 	*choice;
    struct subchoice 	*subchoice;


    for (board = system->boards; board != NULL; board = board->next)
	for (group = board->groups; group != NULL; group = group->next)
	    for (function = group->functions; 
		function != NULL; 
		function = function->next)
		for (subfunction = function->subfunctions; 
		    subfunction != NULL; 
		    subfunction = subfunction->next)
		    for (choice = subfunction->choices; 
			choice != NULL; 
			choice = choice->next) {
			set_subchoice (board->eisa_slot, choice->primary);
			for (subchoice = choice->subchoices; 
			    subchoice != NULL; 
			    subchoice = subchoice->next)
			    set_subchoice (board->eisa_slot, subchoice);
		    }
}


/****+++***********************************************************************
*
* Function:     set_subchoice()
*
* Parameters:   eisa_slot
*		subchoice
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

void set_subchoice (eisa_slot, subchoice)
    int 			eisa_slot;
    struct subchoice 		*subchoice;
{
    struct resource_group 	*resource_group;
    union {
	struct resource 	*resource;
	struct port 		*port;
    } ptr;

    struct value 		*value;
    unsigned			slot;


    if (subchoice == NULL)
	return;

    for (resource_group = subchoice->resource_groups; 
	resource_group != NULL; 
	resource_group = resource_group->next)
	for (ptr.resource = resource_group->resources; 
	    ptr.resource != NULL; 
	    ptr.resource = ptr.resource->next)
	    if (ptr.resource->type == rt_port && ptr.port->slot_specific) {
		slot = eisa_slot << 12;

		if (ptr.port->step) {
		    ptr.port->u.range.min &= 0xFFF;
		    ptr.port->u.range.max &= 0xFFF;
		    ptr.port->u.range.min |= slot;
		    ptr.port->u.range.max |= slot;
		    ptr.port->u.range.current |= slot;
		}

		else
		    for (value = ptr.port->u.list.values; 
			value != NULL; 
			value = value->next) {
			value->min &= 0xFFF;
			value->max &= 0xFFF;
			value->min |= slot;
			value->max |= slot;
		    }
	    }
}
