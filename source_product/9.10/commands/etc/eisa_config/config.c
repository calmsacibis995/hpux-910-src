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
*                             src/config.c
*
*   The file contains the source code for conflict resolution and
*   detection.
*
*	config()	--	cf_util.c
*
**++***************************************************************************/


#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"


#define BARRIERS    3


/***************
* The following globals point to the current entry of each type. In many cases,
* the global is used to just avoid re-declaring a local temporary. In others,
* inter-function state is being kept here. (The trick, of course, is trying
* to tell the difference.)
***************/
static struct system 		*sysptr;
static struct board 		*board;
static struct subfunction 	*subf;
static struct choice 		*choice;
static struct subchoice 	*subch;
static struct resource_group 	*resgrp;

						  
/***************
* This is an array of space pointers. The first element points to
* sysptr->irq. Subsequent entries point to sysptr->dma, sysptr->port,
* ans sysptr->memory. They are used when opertaing on all of the space
* lists to free up memory.
***************/
static struct space 		**space_ptr;
						   
/***************
* This is used to communicate between allocate_space() and check_memory().
***************/
static struct space 		*glb_space;

/***************
* This is the maximum of the sizing values for the system and all of the
* boards.
***************/
static unsigned long		sizing;

/***************
* This is the number of system entries we have (or will have) created.
***************/
static int  			entries;

/***************
* This is the current index into the entries array (the one we are currently
* dealing with.
***************/
static int  			current;

/***************
* This is the mode that config() was called with. The possibilities are    
* DETECTION and RESOLUTION.
***************/
static int  			detection_mode;

/***************
* Does memory have to be contiguous? 
***************/
static int  			noncontiguous_mode;


static union {
    struct resource 	*res;
    struct irq 		*irq;
    struct dma 		*dma;
    struct port 	*port;
    struct memory 	*mem;
} rp;


/**************
* We have one entry for each record off of sysptr. We do this because they
* are much easier to deal with in this format.
* The first entry is always pointed to by entry. ep is the current entry.
**************/
static struct entry {
    /* The key union is used to sort the entries. The bits side are only
       used by create_entry() to build a unique key. The sort side
       is used when the qsort is done */
    union {
	unsigned     		sort;
	struct {
	    unsigned		type_status: 10;
	    unsigned		slot_number: 6;
	    unsigned		sequence:    16;
	} bits;
    } key;
    union {
	struct barrier 		*barrier;
	struct subfunction 	*subfunction;
	struct choice 		*choice;
	struct resource_group 	*link;
	struct resource 	*resource;
    } ptr;
    enum { et_barrier, et_subfunction, et_choice, et_link, et_resource } type;
    unsigned    		active: 1;
    unsigned    		totalmem: 1;
    int 			conflict;
} *entry, *ep;


/**************
* ......
* The first barrier is always pointed to by barrier. bp is the current barrier.
**************/
static struct barrier {
    union {
	unsigned    	word;
	struct {
	    unsigned	share: 1;
	    unsigned	unused1: 1;               /* this field not used */
	    unsigned	disable: 1;
	    unsigned	done: 1;
	} bits;
    } pending, permit;
} *barrier, *bp;


/*********************
* Externally available functions declared here.
*********************/
void			config();


/*********************
* These are the functions used within this file only.
*********************/
static int  		sort_compare();
static int  		compute_entries();
static int  		init_system();
static int  		sysmem_ok();
static int  		check_regions();
static void 		set_nonlink_indices(); 
static void 		set_subchoice();
static void 		init_subfunction(); 
static void 		init_subchoice(); 
static void 		create_entry();
static void 		cross_reference();
static void  		process();
static void 		process_barrier();
static void 		process_subfunction(); 
static void 		process_choice();
static void 		process_subchoice();
static void 		process_link();
static void 		process_resource(); 
static void 		process_conflict();
static void 		post_process();
static void 		bump();

static struct index 	*bump_subfunction();
static struct index 	*bump_choice();
static struct index 	*bump_link();
static struct index 	*bump_resource();
static struct index 	*bump_irq();
static struct index 	*bump_dma();
static struct index 	*bump_port();
static struct index 	*bump_memory();
static struct index 	*bump_address();

static void 		backtrack();
static void 		backtrack_prep();
static void 		subchoice_inactive();
static void 		current_subfunction();
static void 		first_subfunction();
static void 		next_subfunction();
static void 		current_choice();
static void 		first_choice();
static void 		next_choice();
static void 		current_resource_group();
static void 		first_resource_group();
static void 		next_resource_group();
static void 		current_resource();
static void 		first_resource();
static void 		next_resource();
static void 		current_irq();
static void 		first_irq();
static void 		next_irq();
static void 		current_dma();
static void 		first_dma();
static void 		next_dma();
static void 		current_port();
static void 		first_port();
static void 		next_port();
static void 		current_memory();
static void 		first_memory();
static void 		next_memory();
static void 		current_address();
static void 		first_address();
static void 		next_address();

static int  		allocate();
static int  		allocate_port();
static int  		allocate_memory();
static int  		check_totalmem();
static void 		totalmem_conflict();
static int  		check_memory();
static int  		allocate_space();
static int  		space_conflict();
#ifdef MANUAL_VERIFY
static void 		set_detection_conflict();
#endif
static void 		release_spaces();
static struct space 	*create_space();
static struct space 	*free_space();
static void 		free_all();


/*****************
* These are the outside functions which are used.
*****************/
extern void 		mn_trapfree();
extern void 		*mn_trapcalloc();


/****+++***********************************************************************
*
* Function:     config()
*
* Parameters:   system_ptr		ptr to a system struct
*		detection		action to take:
*					   DETECTION
*					   RESOLUTION
*
* Used:		external only
*
* Returns:	Nothing
*
* Description:
*
*    main configure function -- called only by cf_configure() and
*    cf_retry_boards() -- both in cf_util.c
*
****+++***********************************************************************/

void config(system_ptr, detection)
    struct system 	*system_ptr;
    int 		detection;
{
    int			err;
    int  		sysmem_error;


    /*******************
    * Initialize globals.
    *******************/
    sysptr = system_ptr;
    detection_mode = detection;
    space_ptr = &sysptr->irq;
    entries = compute_entries() + BARRIERS;
    sysptr->configured = 0;
    noncontiguous_mode = 0;

    /*******************
    * Try to generate a configuration (or see if one can be built). If there
    * is an error, exit this function immediately. We may go through the loop
    * a second time if we are doing a resolution and we run into a system memory
    * error while running in contiguous mode. If we hit this case, change the
    * mode and try again.
    *******************/
    while (1) {

	/*******************
	* Initialize some variables and make sure that memory from the last
	* pass (if any) has been released.
	*******************/
	sysmem_error = 0;
	free_all();

	/******************
	* Allocate space for the entry and barrier structures.
	******************/
	entry = mn_trapcalloc((unsigned)entries, sizeof(*entry));
	barrier = mn_trapcalloc(BARRIERS, sizeof(*barrier));

	/******************
	* Initialize the system's data structures. Set all of the various
	* levels to their current values (config = current). Also create
	* all of the entries we will need.
	******************/
	err = init_system();
	if (err != 0) {
	    free_all();
	    return;
	}

	/*****************
	* Sort all of those newly created entries into ascending order based
	* on the keys which create_entry() built. Also walk through all of the
	* entries and stuff their entry index into the entry_num field of
	* the appropriate data structure.
	*****************/
	qsort((void *)entry, (size_t)entries, sizeof(*entry), sort_compare);
	cross_reference();
	process(&sysmem_error);

	if ( (current <= entries) && (detection_mode == RESOLUTION) )  {
	    if (sysmem_error && !noncontiguous_mode) {
		noncontiguous_mode = 1;
		continue;
	    }
	    free_all();
	    return;
	}

	break;
    }

    /*******************
    * If we are doing resolution, set the configured flag and .........
    *******************/
#ifdef MANUAL_VERIFY
    if (detection_mode == DETECTION)
	sysptr->configured = 0;
    else {
#endif
	sysptr->configured = 1;
	set_nonlink_indices();
#ifdef MANUAL_VERIFY
    }
#endif

    /******************
    * Release memory we've allocated and get outta here.
    ******************/
    mn_trapfree((void *)barrier);
    barrier = NULL;
    mn_trapfree((void *)entry);
    entry = NULL;

}



/****+++***********************************************************************
*
* Function:     sort_compare()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    perform compare for qsort (ascending order based on 32-bit key)
*
****+++***********************************************************************/

static int  sort_compare(entry1, entry2)
    struct entry	*entry1;
    struct entry	*entry2;
{
    return(entry1->key.sort < entry2->key.sort ? -1 : 1);
}






/****+++***********************************************************************
*
* Function:     compute_entries()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   Compute the number of entries for the system (add up all boards' entries).
*   Only include boards that are active.
*
****+++***********************************************************************/

static int compute_entries()
{
    int total_entries = 0;


    for (board=sysptr->boards ; board!=NULL ; board=board->next)
	total_entries += board->entries;

    return(total_entries);
}







/****+++***********************************************************************
*
* Function:     set_nonlink_indices()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set non-link resource group indices
*
****+++***********************************************************************/

static void set_nonlink_indices()
{
    struct group 		*group;
    struct function 		*func;


    for (board = sysptr->boards; board != NULL; board = board->next)
	for (group = board->groups; group != NULL; group = group->next)
	    for (func = group->functions; func != NULL; func = func->next)
		for (subf=func->subfunctions ; subf!=NULL ; subf=subf->next) {
		    choice = subf->config;
		    set_subchoice(choice->primary);
		    set_subchoice(choice->config);
		}

}



/****+++***********************************************************************
*
* Function:     set_subchoice()
*
* Parameters:   subchoice         
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set non-link resource group indices in subchoice
*
****+++***********************************************************************/

static void set_subchoice(subchoice)
    struct subchoice	*subchoice;
{
    int 		value;


    if (subchoice == NULL)
	return;

    for (resgrp=subchoice->resource_groups ; resgrp ; resgrp=resgrp->next) {

	if (resgrp->type == rg_link)
	    continue;

        resgrp->index.config = 0; 

	for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next) {

	    switch (rp.res->type) {

		case rt_irq:
		    value = rp.irq->index.config * rp.irq->index_value;
		    break;

		case rt_dma:
		    value = rp.dma->index.config * rp.dma->index_value;
		    break;

		case rt_port:
		    value = rp.port->index.config * rp.port->index_value;
		    break;

		case rt_memory:
		    value = rp.mem->memory.index.config *
			    rp.mem->memory.index_value;
		    if (rp.mem->address != NULL)
			value += rp.mem->address->index.config *
			         rp.mem->address->index_value;
		    break;

	    }

	    resgrp->index.config += value;

	}

    }
}


/****+++***********************************************************************
*
* Function:     init_system()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      -1    			# of entries created is not correct
*		0			ok
*
* Description:
*
*    Initialize system. Get the sysptr data structure ready to go for the
*    rest of the config() rtn.
*
****+++***********************************************************************/

static int init_system()
{
    struct group 		*group;
    struct function 		*func;


    /*********************
    * Initialize some globals. Create an entry for each of the barriers.
    *********************/
    ep = entry;
    current = 0;
    bp = barrier;
    while (current < BARRIERS)
	create_entry(et_barrier);

    /***********************
    * Start sizing at the default size for the system.
    ***********************/
    sizing = sysptr->default_sizing;

    /************************
    * Walk through each of the boards and do the initializations.
    ************************/
    for (board = sysptr->boards; board != NULL; board = board->next) {

	/******************
	* Mark board as no-conflict.
	******************/
	board->conflict = 0;

	/*******************
	* Initialize each of the board's subfunctions.
	*******************/
	for (group = board->groups; group != NULL; group = group->next)
	    for (func = group->functions; func != NULL; func = func->next)
		for (subf = func->subfunctions; subf != NULL; subf = subf->next)
		    init_subfunction();

    }

    /*******************
    * If we didn't do all of the entries that we should have, exit with an
    * error. Otherwise, just return success.
    *******************/
    if (current != entries)
	return(-1);
    return(0);
}


/****+++***********************************************************************
*
* Function:     init_subfunction()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    initialize subfunction
*
****+++***********************************************************************/

static void init_subfunction()
{

    /*****************
    * Set up the current subfunction and add an entry for it.
    *****************/
    current_subfunction();
    create_entry(et_subfunction);

    /******************
    * Initialize each of the choices for this subfunction.
    ******************/
    for (choice=subf->choices ; choice!=NULL ; choice=choice->next)  {

	/*******************
	* Set up the current choice and add an entry for it.
	*******************/
	current_choice();
	create_entry(et_choice);

	/*******************
	* If there is a primary subchoice, initialize it.
	*******************/
	subch = choice->primary;
	if (subch != NULL)
	    init_subchoice(subch);

	/********************
	* Initialize all of the subchoices.
	********************/
	for (subch = choice->subchoices; subch != NULL; subch = subch->next)
	    init_subchoice(subch);

    }
}





/****+++***********************************************************************
*
* Function:     init_subchoice()
*
* Parameters:   subchoi           subchoice to initialize
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    initialize subchoice
*
****+++***********************************************************************/

static void init_subchoice(subchoi)
    struct subchoice	*subchoi;

{

    /*******************
    * Initialize all resource groups associated with this subchoice.
    *******************/
    for (resgrp=subchoi->resource_groups ; resgrp!=NULL ; resgrp=resgrp->next) {

	/****************
	* Set up the current resource group (and all resources in it).
	****************/
	current_resource_group();

	/********************
	* Create entries for the link or resource. Mark all of the resources
	* as not in conflict.
	********************/
	if (resgrp->type == rg_link) {
	    for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next)
		rp.res->conflict = 0;
	    create_entry(et_link);
	}
	else  {
	    for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next){
		rp.res->conflict = 0;
		create_entry(et_resource);
	    }
	}

    }

}


/****+++***********************************************************************
*
* Function:     create_entry()
*
* Parameters:   type              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Create an entry of the appropriate type. Construct a key for each
*    entry to allow a qsort of the entries. Key characteristics:
*                 !!!!!!hole!!!!!!
*
****+++***********************************************************************/

static void create_entry(type)
    int 	type;
{
    int 	status[4];


    /*********************
    * If we have already created as many entries as we should have, there is
    * a problem.
    **********************/
    if (current >= entries)
	return;

    switch (ep->type = type) {

	/***********************
	* Create a barrier record. Connect the entry record with a barrier
	* record. Advance to the next barrier record.
	***********************/
	case et_barrier:
	    /*
		    Current =	1    |	2   |  3
		     Status = Locked | User | eeprom
		*/
	    status[0] = current + 1;
	    status[1] = 0;
	    status[2] = 0;
	    status[3] = 0;
	    ep->ptr.barrier = bp++;
	    ep->active = 1;
#ifdef MANUAL_VERIFY
	    if (detection_mode == DETECTION)
		ep->ptr.barrier->permit.word = 0xffff;
#endif
	    ep->ptr.barrier->permit.bits.disable = 1;
	    break;

	case et_subfunction:
	    ep->ptr.subfunction = subf;
	    status[0] = subf->status;
	    status[1] = 0;
	    status[2] = 0;
	    status[3] = 1;
	    ep->active = 1;
	    ep->key.bits.slot_number = board->slot_number;
	    break;

	case et_choice:
	    ep->ptr.choice = choice;
	    status[0] = choice->status;
	    status[1] = subf->status;
	    status[2] = 0;
	    status[3] = 2;
	    ep->key.bits.slot_number = board->slot_number;
	    break;

	case et_link:
	    ep->ptr.link = resgrp;
	    status[0] = resgrp->status;
	    status[1] = choice->status;
	    status[2] = subf->status;
	    status[3] = 3;
	    ep->key.bits.slot_number = board->slot_number;
	    break;

	case et_resource:
	    ep->ptr.resource = rp.res;
	    status[0] = resgrp->status;
	    status[1] = choice->status;
	    status[2] = subf->status;
	    status[3] = 3;
	    ep->key.bits.slot_number = board->slot_number;
    }

    /************************
    * Construct a status word and keep track of which entry this is.
    * Bump the current global.
    *   For barriers, status =
    *                  100  for the first barrier record
    *                  200  for the second barrier record
    *                  300  for the third barrier record
    ************************/
    ep->key.bits.type_status = ( (100 * status[0]) +
				 (25  * status[3]) +
				 (5   * status[1]) +
				 (      status[2]) );
    ep->key.bits.sequence = current++;
    (ep++)->conflict = -1;
}


/****+++***********************************************************************
*
* Function:     cross_reference()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function walks through each of the entries we have just created
*    and writes their index number into the entry_num field of the appropriate
*    structure (for example, subfunction). Barriers are skipped since they
*    have no associated entry_num field.
*
****+++***********************************************************************/

static void cross_reference()
{
    for (ep=entry, current=0  ;  current<entries  ;  current++, ep++)

	switch (ep->type) {

	    case et_barrier:
		break;

	    case et_subfunction:
		ep->ptr.subfunction->entry_num = current;
		break;

	    case et_choice:
		ep->ptr.choice->entry_num = current;
		break;

	    case et_link:
		ep->ptr.link->entry_num = current;
		break;

	    case et_resource:
		ep->ptr.resource->entry_num = current;

	}
}


/****+++***********************************************************************
*
* Function:     process()
*
* Parameters:   sysmem_error      set on exit if there was a memory problem
*
* Used:		internal only
*
* Returns:	Nothing
*
* Description:
*
*    process entry array
*
****+++***********************************************************************/

static void process(sysmem_error)
    int		*sysmem_error;
{

    /******************
    * Prepare to walk through the entries.
    ******************/
    ep = entry;
    current = 0;

    /******************
    * Walk through all of the entries.
    ******************/
    while ((unsigned)current <= (unsigned)entries)  {

	/****************
	* Skip through the non-active ones at the front. 
	* ??????? What does this mean ????????
	****************/
	while ( (current < entries) && (!ep->active) ) {
	    current++;
	    ep++;
	}

	/*********************
	* If we still have entries to process, call the appropriate function
	* to process that type.
	*********************/
	if (current < entries)
	    switch (ep->type) {
		case et_barrier:
		    process_barrier();
		    break;
		case et_subfunction:
		    process_subfunction();
		    break;
		case et_choice:
		    process_choice();
		    break;
		case et_link:
		    process_link();
		    break;
		case et_resource:
		    process_resource();
	    }

	/************************
	* Otherwise, we're done, so do the post processing. The sysmem_error
	* parm will be set if there is a memory problem.
	************************/
	else 
	    post_process(sysmem_error);

    }

}






/****+++***********************************************************************
*
* Function:     process_barrier()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Process a barrier entry. Set pending to 0 and bump current and ep.
*
****+++***********************************************************************/

static void process_barrier()
{
    bp = ep->ptr.barrier;
    bp->pending.word = 0;
    current++;
    ep++;
}


/****+++***********************************************************************
*
* Function:     process_subfunction()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Process a subfunction entry. Mark the current choice as active and 
*    process the primary subchoice.
*
****+++***********************************************************************/

static void process_subfunction()
{

    subf = ep->ptr.subfunction;
    choice = subf->config;
    entry[choice->entry_num].active = 1;
    subch = choice->primary;
    process_subchoice();
    current++;
    ep++;
}






/****+++***********************************************************************
*
* Function:     process_choice()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	process choice entry
*
****+++***********************************************************************/

static void process_choice()
{

    choice = ep->ptr.choice;
    subch = choice->config;
    process_subchoice();
    current++;
    ep++;
}


/****+++***********************************************************************
*
* Function:     process_subchoice()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Walk through each of the resource groups for this subchoice and mark
*    all entries associated with them (either links or resources) as active
*    and with totalmem set.
*
****+++***********************************************************************/

static void process_subchoice()
{

    if (subch == NULL)
	return;

    for (resgrp=subch->resource_groups ; resgrp!=NULL ; resgrp=resgrp->next)  {

	if (resgrp->type == rg_link) {
	    entry[resgrp->entry_num].active = 1;
	    entry[resgrp->entry_num].totalmem = 1;
	}

	else 
	    for (rp.res=resgrp->resources; rp.res!=NULL ; rp.res=rp.res->next) {
		entry[rp.res->entry_num].active = 1;
		entry[rp.res->entry_num].totalmem = 1;
	    }

    }
}






/****+++***********************************************************************
*
* Function:     process_link()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    process link group entry -- handle all entries in this resource group
*
****+++***********************************************************************/

static void process_link()
{
    unsigned		n;
    int 		conflict;


    resgrp = ep->ptr.link;
    conflict = -1;

    for (rp.res = resgrp->resources; rp.res != NULL; rp.res = rp.res->next) {
	n = allocate();
	if (n < (unsigned)conflict)
	    conflict = n;
    }

    process_conflict(conflict);

}


/****+++***********************************************************************
*
* Function:     process_resource()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    process a single resource entry
*
****+++***********************************************************************/

static void process_resource()
{
    int 	conflict;


    rp.res = ep->ptr.resource;
    resgrp = rp.res->parent;
    conflict = allocate();
    process_conflict(conflict);

}






/****+++***********************************************************************
*
* Function:     process_conflict()
*
* Parameters:   conflict          -1:     no conflict detected
*				  other:  entry ndx of the one we conflicted
*					  with
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    process conflict for link group or resource
*
****+++***********************************************************************/

static void process_conflict(conflict)
    int		conflict;
{


    /****************
    * If there was no conflict, move on.
    ****************/
    if (conflict < 0) {
	current++;
	ep++;
    }

    else  {
	if (entry[conflict].conflict <= current) {
	    entry[conflict].totalmem = ep->totalmem;
	    entry[conflict].conflict = current;
	}
	bump();
    }

}





/****+++***********************************************************************
*
* Function:     post_process()
*
* Parameters:   sysmem_error       -- set here if there is a memory problem
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    perform post process functions
*
****+++***********************************************************************/

static void post_process(sysmem_error)
    int		*sysmem_error;
{


    if (sysmem_ok())
	current++;

    else {
	*sysmem_error = 1;
	backtrack();
	bump();
    }

}



/****+++***********************************************************************
*
* Function:     sysmem_ok()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      1		-- always if in noncontiguous mode
*				-- memory allocation ok (contiguous mode)
*		0		-- memory allocation error
*
* Description:
*
*    check system memory contiguity and sizing gaps
*
****+++***********************************************************************/

static int sysmem_ok()
{
    int 		err;
    struct space	*space;


    /**************
    * If the memory spaces don't have to be contiguous, everything is
    * guaranteed to be ok.
    **************/
    if (noncontiguous_mode)
	return(1);

    /**************
    * Check the two regions. If they are ok, exit with good status. If
    * err comes back as 1, it means .......
    **************/
    err = check_regions();
    if (err == 0)
	return(1);
    if (err == 1)
	return(0);

    /**************
    * We had a problem, so ...
    **************/
    for (space = sysptr->memory; space != NULL; space = space->next) {

	rp.res = space->resource;
	if (rp.mem->memtype != mt_sys)
	    continue;

	entry[space->owner].totalmem = 0;
	if (rp.res->parent->status > is_eeprom)  /* only is_free is > */
	    entry[space->owner].conflict = current;
	else
	    entry[space->owner].conflict = -1;

    }

    return(0);
}


/****+++***********************************************************************
*
* Function:     check_regions()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      0		-- successful
*		1		--
*		2		--
*
* Description:
*
*    check memory regions
*
*    ??? what are memory regions? do we have them?
*
****+++***********************************************************************/

static int check_regions()
{
    int 		n;
    struct space 	*prev;
    struct space 	*space;
    unsigned long   	min;
    unsigned long   	max;
    unsigned long   	gap;
#define REGIONS	    2
    static struct {
	unsigned long   min, max;
    } region[REGIONS] = {
	0L,		0x9ffffL,
	0x100000L,	0xffffffL};


    for (n=0  ;  n<REGIONS  ;  n++) {

	min = region[n].min;
	max = region[n].max;
	if (sizing == 0)
	    gap = min;
	else
	    gap = (min + sizing * 2 - 1) / sizing * sizing;

	for (space = sysptr->memory; 
	     (space != NULL) && (space->sp_min <= max);
	     space = space->next) {

	    /***************
	    * If this entry is not part of this region, move on.
	    ***************/
	    if (space->sp_max < min)
		continue;

	    rp.res = space->resource;

	    if (rp.mem->memtype != mt_sys) {

		if ( (space->sp_min >= gap) || (!rp.mem->writable) )
		    continue;

		prev = space->prev;
		if (prev != NULL) {
		    entry[prev->owner].totalmem = 0;
		    entry[prev->owner].conflict = current;
		}

		entry[space->owner].totalmem = 0;
		entry[space->owner].conflict = current;
		return(1);

	    }

	    if (space->sp_min != min)
		return(2);

	    min = space->sp_max + 1;
	    if (sizing == 0)
		gap = min;
	    else
		gap = (min + sizing * 2 - 1) / sizing * sizing;
	}
    }

    return(0);
}


/****+++***********************************************************************
*
* Function:     bump()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    bump current entry
*
****+++***********************************************************************/

static void bump()
{
    struct index 	*index;


    while (current >= 0) {

	switch (ep->type) {

	    case et_barrier:
		release_spaces();
		return;

	    case et_subfunction:
		index = bump_subfunction();
		break;

	    case et_choice:
		index = bump_choice();
		break;

	    case et_link:
		index = bump_link();
		break;

	    case et_resource:
		index = bump_resource();
		break;

	}

	if (index == NULL)
	    return;

	if (index->config < index->total)
	    break;

	backtrack();
	if (current < 0)
	    return;
    }

    release_spaces();

}


/****+++***********************************************************************
*
* Function:     bump_subfunction()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    bump subfunction
*
****+++***********************************************************************/

static struct index *bump_subfunction()
{

    subf = ep->ptr.subfunction;
    if ( (subf->status == is_actual)  || (subf->status == is_locked) ) {
	current = -1;
	return(NULL);
    }

    choice = subf->config;
    entry[choice->entry_num].active = 0;
    subchoice_inactive(choice->primary);

    while (1)  {

	if (subf->index.config == subf->index.current)
	    first_subfunction();
	else 
	    next_subfunction();

	if (subf->index.config == subf->index.current)
	    next_subfunction();

	if ( (choice == NULL) || !choice->disable || bp->permit.bits.disable)
	    break;

	bp->pending.bits.disable = 1;

    }

    return(&subf->index);
}






/****+++***********************************************************************
*
* Function:     bump_choice()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump choice
*
****+++***********************************************************************/

static struct index *bump_choice()
{


    choice = ep->ptr.choice;
    if (choice->status == is_locked) {
	current = -1;
	return(NULL);
    }

    subch = choice->config;
    subchoice_inactive(subch);

    if (choice->index.config == choice->index.current)
	first_choice();
    else 
	next_choice();

    if (choice->index.config == choice->index.current)
	next_choice();

    return(&choice->index);
}



/****+++***********************************************************************
*
* Function:     bump_link()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump link group
*
****+++***********************************************************************/

static struct index *bump_link()
{

    resgrp = ep->ptr.link;

    if (resgrp->status == is_locked) {
	current = -1;
	return(NULL);
    }

    if ( (resgrp->resources->type == rt_memory)       &&
	 (resgrp->parent->parent->totalmem != NULL)   &&
	 (resgrp->status > is_user) )
	next_resource_group();

    else {

	if (resgrp->index.config == resgrp->index.current)
	    first_resource_group();
	else 
	    next_resource_group();

	if (resgrp->index.config == resgrp->index.current)
	    next_resource_group();

    }

    return(&resgrp->index);
}





/****+++***********************************************************************
*
* Function:     bump_resource()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump resource
*
****+++***********************************************************************/

static struct index *bump_resource()
{

    rp.res = ep->ptr.resource;
    resgrp = rp.res->parent;

    if (resgrp->status == is_locked) {
	current = -1;
	return(NULL);
    }

    if (rp.res->type == rt_irq)
	return(bump_irq());
    if (rp.res->type == rt_dma)
	return(bump_dma());
    if (rp.res->type == rt_port)
	return(bump_port());
    return(bump_memory());

}


/****+++***********************************************************************
*
* Function:     bump_irq()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump irq
*
****+++***********************************************************************/

static struct index *bump_irq()
{


    if (rp.irq->index.config == rp.irq->index.current)
	first_irq();
    else 
	next_irq();

    if (rp.irq->index.config == rp.irq->index.current)
	next_irq();

    return(&rp.irq->index);
}






/****+++***********************************************************************
*
* Function:     bump_dma()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump dma
*
****+++***********************************************************************/

static struct index *bump_dma()
{

    if (rp.dma->index.config == rp.dma->index.current)
	first_dma();
    else 
	next_dma();

    if (rp.dma->index.config == rp.dma->index.current)
	next_dma();

    return(&rp.dma->index);
}






/****+++***********************************************************************
*
* Function:     bump_port()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump port
*
****+++***********************************************************************/

static struct index *bump_port()
{

    if (rp.port->index.config == rp.port->index.current)
	first_port();
    else 
	next_port();

    if (rp.port->index.config == rp.port->index.current)
	next_port();

    return(&rp.port->index);
}


/****+++***********************************************************************
*
* Function:     bump_memory()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      
*
* Description:
*
*    bump memory
*
****+++***********************************************************************/

static struct index *bump_memory()
{
    struct index 	*index;


    if (rp.mem->address != NULL) {
	if (!ep->totalmem) {
	    index = bump_address();
	    if (index->config < index->total)
		return(index);
	}
	current_address();
    }

    if ( (rp.res->parent->parent->parent->totalmem == NULL)  ||
	 (rp.res->parent->status <= is_user) )  {

	if (rp.mem->memory.index.config == rp.mem->memory.index.current)
	    first_memory();
	else 
	    next_memory();

	if (rp.mem->memory.index.config == rp.mem->memory.index.current)
	    next_memory();

    }

    else
	next_memory();

    return(&rp.mem->memory.index);
}





/****+++***********************************************************************
*
* Function:     bump_address()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:
*
* Description:
*
*    bump address
*
****+++***********************************************************************/

static struct index *bump_address()
{

    if ( (rp.res->parent->parent->parent->totalmem == NULL)  ||
	 (rp.res->parent->status <= is_user) )  {

	if (rp.mem->address->index.config==rp.mem->address->index.current)
	    first_address();
	else 
	    next_address();

	if (rp.mem->address->index.config==rp.mem->address->index.current)
	    next_address();
    }

    else
	next_address();

    return(&rp.mem->address->index);
}


/****+++***********************************************************************
*
* Function:     backtrack()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    backtrack through entry array
*
****+++***********************************************************************/

static void backtrack()
{
    int 	target;
    int 	conflict;


    target = current;
    backtrack_prep();

    while (1) {

	while ( (current >= 0) && (!ep->active) )   {
	    current--;
	    ep--;
	}

	if (current < 0)
	    return;

	conflict = ep->conflict;
	ep->conflict = -1;
	if (conflict >= target)
	    return;

	switch (ep->type) {

	    case et_barrier:
		/***************************
		***** this is a very strange loop -- what the hell is going
		***** on here??
		***************************/
		do {
		    bp->permit.word <<= 1;
		    bp->permit.word |= 1;
		    if ((bp->permit.word & bp->pending.word) != 0)
			return;
		} while (bp->permit.bits.done == 0);
		ep->active = 0;
		bp--;
		break;

	    case et_subfunction:
		subf = ep->ptr.subfunction;
		choice = subf->config;
		if (choice != NULL) {
		    entry[choice->entry_num].active = 0;
		    subchoice_inactive(choice->primary);
		}
		current_subfunction();
		break;

	    case et_choice:
		choice = ep->ptr.choice;
		subchoice_inactive(choice->config);
		current_choice();
		break;

	    case et_link:
		resgrp = ep->ptr.link;
		current_resource_group();
		ep->totalmem = 1;
		break;

	    case et_resource:
		rp.res = ep->ptr.resource;
		resgrp = rp.res->parent;
		current_resource();
		ep->totalmem = 1;
	}

	current--;
	ep--;
    }
}


/****+++***********************************************************************
*
* Function:     backtrack_prep()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    prepare to backtrack
*
****+++***********************************************************************/

static void backtrack_prep()
{
    int 	parent;


    if (current == entries) {
	current--;
	ep--;
	return;
    }

    ep->totalmem = 1;
    ep->conflict = -1;

    switch (ep->type) {

	case et_barrier:
	case et_subfunction:
	    return;

	case et_choice:
	    parent = ep->ptr.choice->parent->entry_num;
	    break;

	case et_link:
	    subch = ep->ptr.link->parent;
	    if (subch->explicit)
		parent = subch->parent->entry_num;
	    else
		parent = subch->parent->parent->entry_num;
	    break;

	case et_resource:
	    subch = ep->ptr.resource->parent->parent;
	    if (subch->explicit)
		parent = subch->parent->entry_num;
	    else
		parent = subch->parent->parent->entry_num;
	    break;
    }

    if (entry[parent].conflict < current)
	entry[parent].conflict = current;

}


/****+++***********************************************************************
*
* Function:     subchoice_inactive()
*
* Parameters:   subchoice         
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    mark subchoice link group and resource entries inactive
*
****+++***********************************************************************/

static void subchoice_inactive(subchoice)
    struct subchoice	*subchoice;
{


    if (subchoice == NULL)
	return;

    for (resgrp=subchoice->resource_groups ; resgrp!=NULL ; resgrp=resgrp->next)

	if (resgrp->type == rg_link)
	    entry[resgrp->entry_num].active = 0;

	else 
	    for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next)
		entry[rp.res->entry_num].active = 0;
}


/****+++***********************************************************************
*
* Function:     current_subfunction()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set subfunction index = current
*
****+++***********************************************************************/

static void current_subfunction()
{

    subf->conflict = 0;
    subf->index.config = subf->index.current;
    choice = subf->current;
    subf->config = subf->current;
}





/****+++***********************************************************************
*
* Function:     first_subfunction()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set subfunction index = first
*
****+++***********************************************************************/

static void first_subfunction()
{

    subf->index.config = 0;
    choice = subf->choices;
    subf->config = subf->choices;
}







/****+++***********************************************************************
*
* Function:     next_subfunction()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set subfunction index = next
*
****+++***********************************************************************/

static void next_subfunction()
{

    subf->index.config++;
    subf->config = choice->next;
    choice = choice->next;
}


/****+++***********************************************************************
*
* Function:     current_choice()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set choice index = current
*
****+++***********************************************************************/

static void current_choice()
{

    choice->index.config = choice->index.current;
    subch = choice->current;
    choice->config = choice->current;
}





/****+++***********************************************************************
*
* Function:     first_choice()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set choice index = first
*
****+++***********************************************************************/

static void first_choice()
{

    choice->index.config = 0;
    subch = choice->subchoices;
    choice->config = choice->subchoices;
}





/****+++***********************************************************************
*
* Function:     next_choice()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set choice index = next
*
****+++***********************************************************************/

static void next_choice()
{

    choice->index.config++;
    choice->config = subch->next;
    subch = subch->next;
}


/****+++***********************************************************************
*
* Function:     current_resource_group()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set resource group index = current
*
****+++***********************************************************************/

static void current_resource_group()
{

    if ( (resgrp->resources->type == rt_memory)  &&
	 (resgrp->parent->parent->totalmem != NULL) &&
	 (resgrp->status > is_user) )
	first_resource_group();

    else {
	resgrp->index.config = resgrp->index.current;
	for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next)
	    current_resource();
    }
}




/****+++***********************************************************************
*
* Function:     first_resource_group()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set resource group index = first
*
****+++***********************************************************************/

static void first_resource_group()
{

    for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next)
	first_resource();

    rp.res = resgrp->resources;

    if (rp.res->type == rt_memory)
	resgrp->index.config = rp.mem->memory.index.config;
    else
	resgrp->index.config = 0;
}




/****+++***********************************************************************
*
* Function:     next_resource_group()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set resource group index = next
*
****+++***********************************************************************/

static void next_resource_group()
{

    for (rp.res=resgrp->resources ; rp.res!=NULL ; rp.res=rp.res->next)
	next_resource();

    rp.res = resgrp->resources;

    if (rp.res->type == rt_memory)
	resgrp->index.config = rp.mem->memory.index.config;
    else
	resgrp->index.config++;
}


/****+++***********************************************************************
*
* Function:     current_resource()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set resource index = current
*
****+++***********************************************************************/

static void current_resource()
{

    rp.res->conflict = 0;

    switch (rp.res->type) {

	case rt_irq:
	    current_irq();
	    break;

	case rt_dma:
	    current_dma();
	    break;

	case rt_port:
	    current_port();
	    break;

	case rt_memory:

	    if ( (rp.res->parent->parent->parent->totalmem != NULL)  &&
		 (rp.res->parent->status > is_user) )
		first_resource();
	    else
		current_memory();

	    if (rp.mem->address != NULL)
		current_address();

    }
}



/****+++***********************************************************************
*
* Function:     first_resource()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set resource index = first
*
****+++***********************************************************************/

static void first_resource()
{

    switch (rp.res->type) {
	case rt_irq:
	    first_irq();
	    break;
	case rt_dma:
	    first_dma();
	    break;
	case rt_port:
	    first_port();
	    break;
	case rt_memory:
	    first_memory();
	    if (rp.mem->address != NULL)
		first_address();
    }

}


/****+++***********************************************************************
*
* Function:     next_resource()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set resource index = next
*
****+++***********************************************************************/

static void next_resource()
{

    switch (rp.res->type) {
	case rt_irq:
	    next_irq();
	    break;
	case rt_dma:
	    next_dma();
	    break;
	case rt_port:
	    next_port();
	    break;
	case rt_memory:
	    next_memory();
	    if (rp.mem->address != NULL)
		next_address();
    }

}


/****+++***********************************************************************
*
* Function:     current_irq()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set irq index = current
*
****+++***********************************************************************/

static void current_irq()
{
    rp.irq->index.config = rp.irq->index.current;
    rp.irq->config = rp.irq->current;
}





/****+++***********************************************************************
*
* Function:     first_irq()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set irq index = first
*
****+++***********************************************************************/

static void first_irq()
{
    rp.irq->index.config = 0;
    rp.irq->config = rp.irq->values;
}





/****+++***********************************************************************
*
* Function:     next_irq()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set irq index = next
*
****+++***********************************************************************/

static void next_irq()
{
    rp.irq->index.config++;
    rp.irq->config = rp.irq->config->next;
}


/****+++***********************************************************************
*
* Function:     current_dma()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set dma index = current
*
****+++***********************************************************************/

static void current_dma()
{
    rp.dma->index.config = rp.dma->index.current;
    rp.dma->config = rp.dma->current;
}





/****+++***********************************************************************
*
* Function:     first_dma()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set dma index = first
*
****+++***********************************************************************/

static void first_dma()
{
    rp.dma->index.config = 0;
    rp.dma->config = rp.dma->values;
}





/****+++***********************************************************************
*
* Function:     next_dma()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set dma index = next
*
****+++***********************************************************************/

static void next_dma()
{
    rp.dma->index.config++;
    rp.dma->config = rp.dma->config->next;
}


/****+++***********************************************************************
*
* Function:     current_port()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set port index = current
*
****+++***********************************************************************/

static void current_port()
{

    rp.port->index.config = rp.port->index.current;

    if (rp.port->step == 0)
	rp.port->u.list.config = rp.port->u.list.current;
    else 
	rp.port->u.range.config = rp.port->u.range.current;
}




/****+++***********************************************************************
*
* Function:     first_port()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set port index = first
*
****+++***********************************************************************/

static void first_port()
{

    rp.port->index.config = 0;

    if (rp.port->step == 0)
	rp.port->u.list.config = rp.port->u.list.values;
    else 
	rp.port->u.range.config = rp.port->u.range.min;
}





/****+++***********************************************************************
*
* Function:     next_port()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set port index = next
*
****+++***********************************************************************/

static void next_port()
{

    rp.port->index.config++;

    if (rp.port->step == 0)
	rp.port->u.list.config = rp.port->u.list.config->next;
    else 
	rp.port->u.range.config += rp.port->step;
}


/****+++***********************************************************************
*
* Function:     current_memory()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set memory index = current
*
****+++***********************************************************************/

static void current_memory()
{

    rp.mem->memory.index.config = rp.mem->memory.index.current;

    if (rp.mem->memory.step == 0)
	rp.mem->memory.u.list.config = rp.mem->memory.u.list.current;
    else 
	rp.mem->memory.u.range.config = rp.mem->memory.u.range.current;
}





/****+++***********************************************************************
*
* Function:     first_memory()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set memory index = first
*
****+++***********************************************************************/

static void first_memory()
{
    struct value 	*value;
    int 		i;
    int 		totalmem_flag;


    if (rp.res->parent->parent->parent->totalmem != NULL)
	totalmem_flag = 1;
    else
	totalmem_flag = 0;

    if (rp.mem->memory.step == 0)

	if (totalmem_flag) {
	    for (value = rp.mem->memory.u.list.values, i = 0; 
		 value->init_index != 1; 
		 value = value->next, i++);
	    rp.mem->memory.u.list.config = value;
	    rp.mem->memory.index.config = i;
	}

	else {
	    rp.mem->memory.u.list.config = rp.mem->memory.u.list.values;
	    rp.mem->memory.index.config = 0;
	}

    else if (totalmem_flag) {
	rp.mem->memory.u.range.config = rp.mem->memory.u.range.max;
	rp.mem->memory.index.config = rp.mem->memory.index.total - 1;
    }

    else {
	rp.mem->memory.u.range.config = rp.mem->memory.u.range.min;
	rp.mem->memory.index.config = 0;
    }

}


/****+++***********************************************************************
*
* Function:     next_memory()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set memory index = next
*
****+++***********************************************************************/

static void next_memory()
{
    struct value 	*value;
    int 		i;
    int 		totalmem_flag;
    int 		last;


    if (rp.res->parent->parent->parent->totalmem != NULL)
	totalmem_flag = 1;
    else
	totalmem_flag = 0;

    if (rp.mem->memory.step == 0)

	if (totalmem_flag) {
	    last = rp.mem->memory.u.list.config->init_index;
	    for (value = rp.mem->memory.u.list.values, i = 0; 
		 (value != NULL) && (value->init_index != last + 1); 
		 value = value->next, i++) ;
	    rp.mem->memory.u.list.config = value;
	    rp.mem->memory.index.config = i;
	}

	else {
	    rp.mem->memory.u.list.config = rp.mem->memory.u.list.config->next;
	    rp.mem->memory.index.config++;
	}

    else if (totalmem_flag) {
	if (rp.mem->memory.index.config == 0)
	    rp.mem->memory.index.config = rp.mem->memory.index.total;
	else {
	    rp.mem->memory.u.range.config -= rp.mem->memory.step;
	    rp.mem->memory.index.config--;
	}
    }

    else {
	rp.mem->memory.u.range.config += rp.mem->memory.step;
	rp.mem->memory.index.config++;
    }
}


/****+++***********************************************************************
*
* Function:     current_address()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set address index = current
*
****+++***********************************************************************/

static void current_address()
{

    rp.mem->address->index.config = rp.mem->address->index.current;
    if (rp.mem->address->step == 0)
	rp.mem->address->u.list.config = rp.mem->address->u.list.current;
    else 
	rp.mem->address->u.range.config = rp.mem->address->u.range.current;
}





/****+++***********************************************************************
*
* Function:     first_address()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set address index = first
*
****+++***********************************************************************/

static void first_address()
{
    struct value *value;
    int 	 i;


    if (rp.res->parent->type == rg_link) {

	rp.mem->address->index.config = rp.mem->memory.index.config;

	if (rp.mem->address->step == 0) {
	    for (i = rp.mem->address->index.config,
		 value = rp.mem->address->u.list.values; 
		 i > 0; 
		 value = value->next, i--) ;
	    rp.mem->address->u.list.config = value;
	}

	else if (rp.mem->address->index.config == 0)
	    rp.mem->address->u.range.config = rp.mem->address->u.range.min;
	else
	    rp.mem->address->u.range.config = rp.mem->address->u.range.max;

    }

    else {
	rp.mem->address->index.config = 0;
	if (rp.mem->address->step == 0)
	    rp.mem->address->u.list.config = rp.mem->address->u.list.values;
	else 
	    rp.mem->address->u.range.config = rp.mem->address->u.range.min;
    }

}


/****+++***********************************************************************
*
* Function:     next_address()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    set address index = next
*
****+++***********************************************************************/

static void next_address()
{
    struct value 	*value;
    unsigned long   	memvalue;
    int 		i;


    if (rp.res->parent->type == rg_link) {
	rp.mem->address->index.config = rp.mem->memory.index.config;
	if (rp.mem->address->step == 0) {
	    for (i = rp.mem->address->index.config,
		    value = rp.mem->address->u.list.values; 
		 i > 0;
		 value = value->next, i--);
	    rp.mem->address->u.list.config = value;
	}
	else {
	    for (i = rp.mem->address->index.config,
		    memvalue = rp.mem->address->u.range.min; 
		 i > 0;
		 memvalue += rp.mem->address->step, i--);
	    rp.mem->address->u.range.config = memvalue;
	}
    }

    else {
	rp.mem->address->index.config++;
	if (rp.mem->address->step == 0)
	    rp.mem->address->u.list.config = rp.mem->address->u.list.config->next;
	else 
	    rp.mem->address->u.range.config += rp.mem->address->step;
    }
}


/****+++***********************************************************************
*
* Function:     allocate()
*
* Parameters:   None
*
* Used:		internal only
*
* Returns:      -1		always if we are in detection mode
*		-1		if there was no info to check (null_value)
*				    -- there is no conflict
*               -1		a new space was allocated -- no conflict
*		other		there was a conflict, this is the index of
*				the entry we conflicted with
*
* Description:
*
*    Get a "space" for the appropriate type of resource. Check for conflicts.
*
****+++***********************************************************************/

static int allocate()
{
    int		ret;


    switch (rp.res->type) {

	case rt_irq:
	    if (rp.irq->config->null_value == 0)  {
		ret = allocate_space(&sysptr->irq, rp.irq->config->min,
				     rp.irq->config->max);
		return(ret);
	    }
	    break;

	case rt_dma:
	    if (rp.dma->config->null_value == 0)  {
		ret = allocate_space(&sysptr->dma, rp.dma->config->min,
				     rp.dma->config->max);
		return(ret);
	    }
	    break;

	case rt_port:
	    ret = allocate_port();
	    return(ret);

	case rt_memory:
	    ret = allocate_memory();
	    return(ret);

    }

    return(-1);

}






/****+++***********************************************************************
*
* Function:     allocate_port()
*
* Parameters:   None
*
* Used:		internal only
*
* Returns:      -1		always if we are in detection mode
*		-1		no info to check (null_value)
*               -1		a new space was allocated -- no conflict
*		other		there was a conflict, this is the index of
*				the entry we conflicted with
*
* Description:
*
*    Attempt to allocate a space for the current port resource.
*
****+++***********************************************************************/

static int allocate_port()
{
    unsigned long   min;
    unsigned long   max;


    if (rp.port->step == 0) {
	if (rp.port->u.list.config->null_value)
	    return(-1);
	min = rp.port->u.list.config->min;
	max = rp.port->u.list.config->max;
    }

    else {
	min = (unsigned long) (rp.port->u.range.config);
	max = (unsigned long) (min + rp.port->u.range.count - 1);
    }

    return(allocate_space(&sysptr->port, min, max));
}


/****+++***********************************************************************
*
* Function:     allocate_memory()
*
* Parameters:   None
*
* Used:		internal only
*
* Returns:      -1		always if we are in detection mode
*		-1		no info to check (null_vlue)
*               -1		a new space was allocated -- no conflict
*		other		there was a conflict, this is the index of
*				the entry we conflicted with
                ***incomplete***
*
* Description:
*
*    Attempt to allocate a space for the current memory resource.
*
****+++***********************************************************************/

static int allocate_memory()
{
    int 		conflict;
    unsigned long   	min;
    unsigned long   	max;
    unsigned long	actual;
    struct totalmem 	*totmem;


    /********************
    *
    *********************/
    if ( (ep->totalmem) && (detection_mode == RESOLUTION) ) {
	choice = resgrp->parent->parent;
	totmem = choice->totalmem;
	if (totmem != NULL) {
	    actual = totmem->actual;
	    if ( (check_totalmem(choice->primary, &actual)) ||
	         (check_totalmem(choice->config, &actual)) ) {
		totalmem_conflict(choice->primary);
		totalmem_conflict(choice->config);
		ep->totalmem = 1;
		ep->conflict = current;
		return(ep->conflict);
	    }
	}
	ep->totalmem = 0;
    }

    /********************
    *
    *********************/
    if (rp.mem->address == NULL)
	return(-1);

#ifdef MANUAL_VERIFY
    /********************
    *
    *********************/
    if (detection_mode == DETECTION)
	if ((rp.mem->memory.step != 0) && (rp.mem->memory.u.range.current == 0))
	    return(-1);
        else if ( (rp.mem->memory.step == 0) &&
	          (rp.mem->memory.u.list.current->min == 0) )
	    return(-1);
#endif

    /**************
    * Establish min and max based on what type of memory record this is.
    * If the range is empty (min == max), exit immediately.
    **************/
    if (rp.mem->address->step == 0)
	min = rp.mem->address->u.list.config->min;
    else
	min = rp.mem->address->u.range.config;
    if (rp.mem->memory.step == 0)
	max = min + rp.mem->memory.u.list.config->min;
    else
	max = min + rp.mem->memory.u.range.config;
    if (min == max)
	return(-1);

    /**************
    * Try to allocate a space struct for this memory resource. If no
    * conflict was detected, make sure that the memory decoding will be ok. 
    **************/
    conflict = allocate_space(&sysptr->memory, min, (unsigned long)(max-1));
    if (conflict < 0)
	conflict = check_memory();

    return(conflict);
}


/****+++***********************************************************************
*
* Function:     check_totalmem()
*
* Parameters:   subchoice         
*		actual		total amount of memory (we may change here)
*
* Used:		internal only
*
* Returns:      0		no problem (may be null subchoice)
*		1		memsize more than what it should be
*
* Description:
*
*    check for totalmem conflict
*
****+++***********************************************************************/

static int check_totalmem(subchoice, actual)
    struct subchoice		*subchoice;
    unsigned long		*actual;
{
    struct resource_group 	*rgrp;
    union {
	struct resource 	*res;
	struct memory 		*mem;
    } r;
    unsigned long   		value;


    /********************
    * Leave immediately with good status if there's nothing to check.
    *********************/
    if (subchoice == NULL)
	return(0);

    /********************
    * Walk through all of the resources used for this subchoice. We will
    * process all memory resources (except virtual) with an entry num
    * less than or equal to the current resource being processed. For
    * each of these resources, make sure that the space they use does
    * not make the total amount of memory bigger than the total that was
    * passed in. If so, exit with an error.
    *********************/
    for (rgrp = subchoice->resource_groups; rgrp != NULL; rgrp = rgrp->next) {

	if (rgrp->entry_num > current)
	    continue;

	for (r.res=rgrp->resources  ;  r.res!=NULL  ;  r.res=r.res->next) {

	    if ( (r.res->type != rt_memory)   ||
		 (r.res->entry_num > current) ||
		 (r.mem->memtype == mt_vir) )
		continue;

	    if (r.mem->memory.step == 0)
		value = r.mem->memory.u.list.config->min;
	    else
		value = r.mem->memory.u.range.config;

	    if (value > *actual)
		return(1);

	    *actual -= value;
	}

    }

    return(0);
}


/****+++***********************************************************************
*
* Function:     totalmem_conflict()
*
* Parameters:   subchoice         
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    record totalmem conflict
*
****+++***********************************************************************/

static void totalmem_conflict(subchoice)
    struct subchoice		*subchoice;
{
    struct resource_group 	*rgrp;
    union {
	struct resource 	*res;
	struct memory 		*mem;
    } r;
    int 			conflict;


    if (subchoice == NULL)
	return;

    for (rgrp = subchoice->resource_groups; rgrp != NULL; rgrp = rgrp->next) {

	if (rgrp->entry_num >= current)
	    continue;

	for (r.res=rgrp->resources  ;  r.res!=NULL  ;  r.res=r.res->next)
	    if (r.res->type == rt_memory) {
		conflict = r.res->entry_num;
		if ( ( (rgrp->type == rg_link) || (conflict < current) )  &&
		     (entry[conflict].conflict < current) ) {
		    entry[conflict].totalmem = 1;
		    entry[conflict].conflict = current;
		}
	    }

    }
}


/****+++***********************************************************************
*
* Function:     check_memory()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Check memory allocation for m16 decode conflicts.
*
****+++***********************************************************************/

static int check_memory()
{
    struct space 		*sp;
    int 			n;
    int 			conflict;
    int 			m16;		/* resource has data_size = 16
						   and decodes 24 bits        */
    int 			byte;		/* resource has data_size = 8 */
    unsigned long   		min;
    unsigned long   		max;
    unsigned long   		maxmax;		/* biggest addr for this memory
						   (on 128k -1 byte boundary) */
    union {
	struct resource 	*res;
	struct memory 		*mem;
    } r;

#define CONST_16M   0x1000000L		/* 24 bit decode boundary -- top end */
#define CONST_128K  0x20000L


    /********************
    * Set min to be the next lower multiple of 128k that the memory space
    * will respond to. Set maxmax to be the next higher multiple of 128k
    * (minus 1) for the high range of the memory space.
    *********************/
    min = glb_space->sp_min & -CONST_128K;
    maxmax = (glb_space->sp_max & -CONST_128K) + (CONST_128K - 1);

    /********************
    * If the low range of the memory is bigger than 16M, it is obviously doing
    * 32 bit address decode, so forget about it (it cannot conflict since
    * it is out of the 20 and 24 bit decode range).
    ********************/
    if (min >= CONST_16M)
	return(-1);

    /********************
    * If the high range of memory is bigger than 16MB, make it 16MB (minus 1).
    * (It is only necessary to check up to the top of the 24 bit decode
    * range, as explained above.)
    *********************/
    if (maxmax >= CONST_16M)
	maxmax = CONST_16M - 1;

    /********************
    * Walk through each 128k from min to maxmax.
    *********************/
    conflict = -1;
    for (max = min + (CONST_128K - 1); 
	 max <= maxmax;
	 min += CONST_128K, max += CONST_128K) {

	/********************
	*
	*********************/
	n = -1;
	m16 = 0;
	byte = 0;
	for (sp=sysptr->memory ; (sp!=NULL)&&(sp->sp_min<=max) ; sp=sp->next) {

	    /********************
	    * If we are not far enough on the chain yet, advance.
	    *********************/
	    if (sp->sp_max < min)
		continue;

	    /********************
	    * Set the data size flags for this entry. It is either the one
	    * that we just added (I think so) or the next one.
	    *********************/
	    r.res = sp->resource;
	    switch (r.mem->data_size) {
		case ds_byte:
		    byte = 1;
		    break;
		case ds_word:
		    if (r.mem->decode == md_24)
			m16 = 1;
		    break;
		default:
		    continue;
	    }

	    /********************
	    *
	    *********************/
	    if ((unsigned)sp->owner < (unsigned)n)
		n = sp->owner;
	}

	/********************
	*
	*********************/
	if ( (m16) && (byte) && ((unsigned)n < (unsigned)conflict) )
	    conflict = n;

    }

    return(conflict);
}


/****+++***********************************************************************
*
* Function:     allocate_space()
*
* Parameters:   head		system resource head ptr
*		min		minimum value for this resource
*		max		maximum value for this resource
*
* Used:		internal only
*
* Returns:      -1		always if we are in detection mode
*               -1		a new space was allocated -- no conflict
*		other		there was a conflict, this is the index of
*				the entry we conflicted with
*
* Description:
*
*    Attempt to allocate space for a given resource type. If there is
*    no conflict (or we're just doing detection) add an entry into the
*    appropriate list. Otherwise, return the index of the lowest entry
*    we conflict with.
*
****+++***********************************************************************/

static int allocate_space(head, min, max)
    struct space 	**head;
    unsigned long 	min;
    unsigned long 	max;
{
    unsigned		n;
    int 		conflict;
    struct space 	*prev;


    /**********************
    * Walk through the existing list (if any) until we find which slot
    * the new record should fit into. When the slot is found, figure out
    * if we have a conflict with the neighboring records. Note that we may
    * have several conflicts since one new record may overlap several
    * existing records. We will only keep track of the lowest numbered
    * conflict (the entry with the lowest index).
    * Note that the list is maintained in ascending order.
    **********************/
    conflict = -1;
    prev = NULL;
    for (glb_space = *head; 
	 (glb_space != NULL) && (glb_space->sp_min <= max);
	 glb_space = glb_space->next) {

	/**************
	* Advance the prev ptr if the one we're looking for is bigger than
	* the one we're looking at.
	**************/
	if (glb_space->sp_min <= min)
	    prev = glb_space;

	/**************
	* If we haven't looked far enough yet, just advance to the next one.
	**************/
	if (glb_space->sp_max < min)
	    continue;

	/**************
	* We potentially have an overlap, so call space_conflict() to figure
	* it out. If a -1 is returned, there is no conflict. Otherwise, we
	* have a conflict and the ret value is the index of the entry we
	* conflict with. Save it away if it's the smallest one we've seen
	* so far. Note that n is unsigned, so -1 is a *big* number.
	**************/
	n = space_conflict(glb_space);
	if (n < (unsigned)conflict)
	    conflict = n;

#ifdef MANUAL_VERIFY
	/**************
	* If we're just doing detection and we have a conflict, set the
	* conflict flags in the appropriate structures for this
	* resource's predecessors.
	**************/
	if ( (detection_mode == DETECTION) && ((int)n >= 0) )
	    set_detection_conflict(glb_space);
#endif

    }

    /*****************
    * If we're trying to do a resolution and we detected a conflict,
    * return the index of the entry we conflicted with.
    *****************/
    if ( (conflict >= 0) && (detection_mode == RESOLUTION) )
	return(conflict);

    /******************
    * Resolution mode: No conflict was detected, so create a new entry.
    * Detection mode:  A conflict may have been detected, but create a new
    *                  entry anyway. If we did have a conflict, set the
    *                  conflict flags in the appropriate structures for this
    *                  resource's predecessors.
    ******************/
    glb_space = create_space(head, prev);
    if (glb_space != NULL) {
	glb_space->sp_min = min;
	glb_space->sp_max = max;
	glb_space->owner = current;
	glb_space->resource = rp.res;
#ifdef MANUAL_VERIFY
	if ( (detection_mode == DETECTION) && (conflict >= 0) )
	    set_detection_conflict(glb_space);
#endif
    }

    return(-1);
}


/****+++***********************************************************************
*
* Function:     space_conflict()
*
* Parameters:   space		ptr to space struct which is being compared
*
* Used:		internal only
*
* Returns:      -1		these two can share this resource
*		other		space->owner -- this means that this resource
*				cannot be shared by these two
*
* Description:
*
*    This function is responsible for checking whether two resource needs
*    conflict with one another. Sharing is handled here. The different resource
*    entries compared are:
*         res    -- the resource for the space passed in
*         rp.res -- the resource we are currently examining (global)
*
****+++***********************************************************************/

static int space_conflict(space)
    struct space	*space;
{
    struct resource 	*res;
    int 		user_share;


    /**************
    * Tuck away the resource for the one that already has a space.
    **************/
    res = space->resource;

    /**************
    * If one or both of them is incapable of sharing, exit.
    **************/
    if ( (res->share == 0) || (rp.res->share == 0) )
	return(space->owner);

    /***************
    * If both of them have share tags and they match each other, exit with
    * a "sharing possible" code. Alternatively, if only one has a tag or they
    * both have tags but do not match, exit with "sharing impossible".
    ***************/
    if ( (res->share_tag != NULL) || (rp.res->share_tag != NULL) )
	if ( (res->share_tag != NULL)  &&
	     (rp.res->share_tag != NULL)  &&
	     (strcmp(res->share_tag, rp.res->share_tag) == 0) )
	    return(-1);
	else
	    return(space->owner);

    /***************
    * If the current resource is an irq and the trigger types do not match,
    * they cannot share.
    * ????? Shouldn't we make sure that res is also an rt_irq????
    ***************/
    if ( (rp.res->type == rt_irq) &&
	 (rp.irq->trigger != ((struct irq *)res)->trigger) )
	return(space->owner);

    /**************
    * If the current barrier says that we are permitted to share, exit.
    **************/
    if (bp->permit.bits.share)
	return(-1);

    /**************
    * If the resource group that the current resource belongs to has
    * status of is_actual, is_locked, or is_user, enter this conditional.
    * For the appropriate resource type, if the one we are currently working on
    * is the one the user chose, exit with "sharing permitted" status.
    **************/
    if (rp.res->parent->status < is_eeprom) {

	user_share = 0;

	switch (rp.res->type) {

	    case rt_irq:
		if (rp.irq->index.config == rp.irq->index.current)
		    user_share = 1;
		break;

	    case rt_dma:
		if (rp.dma->index.config == rp.dma->index.current)
		    user_share = 1;
		break;

	    case rt_port:
		if (rp.port->index.config == rp.port->index.current)
		    user_share = 1;
		break;

	    case rt_memory:
		if ( (rp.mem->memory.index.config == rp.mem->memory.index.current)   &&
		     (rp.mem->address != NULL)  &&
		     (rp.mem->address->index.config == rp.mem->address->index.current) )
		    user_share = 1;
		break;

	}

	if (user_share)
	    return(-1);

    }

    /*****************
    * If we get here, we can't share. Set the pending share bit and get
    * outta here.
    *****************/
    bp->pending.bits.share = 1;
    return(space->owner);
}


#ifdef MANUAL_VERIFY
/****+++***********************************************************************
*
* Function:     set_detection_conflict()
*
* Parameters:   space             resource block with a conflict
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Set conflict detection bit in resource, subfunction, and board.
*
****+++***********************************************************************/

static void set_detection_conflict(space)
    struct space	*space;
{

    /********************
    * Set the conflict bits in the appropriate resource, subfunction, and
    * board structures.
    ********************/
    space->resource->conflict = 1;
    space->resource->parent->parent->parent->parent->conflict = 1;
    space->resource->parent->parent->parent->parent->parent->parent->
	parent->conflict = 1;
}
#endif


/****+++***********************************************************************
*
* Function:     release_spaces()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    release all space entries where owner >= current
*
****+++***********************************************************************/

static void release_spaces()
{
    int 		n;
    struct space 	*space;


    for (n = 0; n < 4; n++)
	for (space = space_ptr[n];
	     space != NULL; 
	     space = space->owner < current ? space->next :
	     				      free_space(space_ptr + n, space))
	    ;
}






/****+++***********************************************************************
*
* Function:     create_space()
*
* Parameters:   head		head of the list this space goes on
*		prev		space preceeding the new space
*
* Used:		internal only
*
* Returns:	a pointer to the new space struct
*
* Description:
*
*    Allocate memory for a new space entry and pop him into the doubly-linked
*    specified by the input parameters.
*
****+++***********************************************************************/

static struct space *create_space(head, prev)
    struct space 	**head;
    struct space 	*prev;
{
    struct space 	*space;
    struct space 	*next;


    space = mn_trapcalloc(1, sizeof(*space));

    space->prev = prev;
    if (space->prev == NULL) {
	next = *head;
	*head = space;
    }

    else {
	next = prev->next;
	prev->next = space;
    }

    space->next = next;
    if (space->next != NULL)
	next->prev = space;

    return(space);
}





/****+++***********************************************************************
*
* Function:     free_space()
*
* Parameters:   head		head of the list this space is on
*		space		entry to be removed
*
* Used:		internal only
*
* Returns:	a ptr to the next element on the list (after the one we are
*		removing)
*
* Description:
*
*    Remove a space struct from the specified list. Free up his memory.
*
****+++***********************************************************************/

static struct space *free_space(head, space)
    struct space 	**head;
    struct space 	*space;
{
    struct space 	*next;
    struct space 	*prev;


    next = space->next;
    prev = space->prev;

    if (prev == NULL)
	*head = next;
    else 
	prev->next = next;

    if (next != NULL)
	next->prev = prev;

    mn_trapfree((void *)space);
    return(next);
}


/****+++***********************************************************************
*
* Function:     free_all()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Release all memory blocks and set all ptrs to null. We will release all of
*    the space blocks, all of the entries, and the barriers.
*
****+++***********************************************************************/

static void free_all()
{
    int 		n;
    struct space 	*space;
    struct space 	*next;


    for (n=0 ; n<4 ; space_ptr[n++]=NULL)
	for (space=space_ptr[n] ; space!=NULL ; space=next) {
	    next = space->next;
	    mn_trapfree((void *)space);
	}

    if (barrier != NULL) {
	mn_trapfree((void *)barrier);
	barrier = NULL;
    }

    if (entry != NULL) {
	mn_trapfree((void *)entry);
	entry = NULL;
    }

}
