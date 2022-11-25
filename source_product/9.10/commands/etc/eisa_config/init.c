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
*                          src/init.c
*
*   This module contains the high level routines for initializing the
*   configuration utility.  Interrogating the EISA eeprom/sci file for
*   current cofiguration, and loading any configured boards.  It also
*   auto-configure EISA boards that have readable IDs.
*
*	init()			-- main.c
*	init2()			-- open_save.c
*	init_auto()		-- main.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <sys/eeprom.h>
#include "config.h"
#include "nvm.h"
#include "sci.h"
#include "err.h"
#include "add.h"
#include "cf_util.h"
#include "sw.h"


/*************
* This is the dupid structure used here when creating a new cfg file
* name. Note that it is implicitly initialized to 0s which means that
* there is not a duplicate.
*************/
static nvm_dupid	null_dupid;


/****************
* These are the globals used in this file which are declared in globals.c.
****************/
extern struct pmode	program_mode;
extern struct system	glb_system;
#ifdef VIRT_BOARD
extern unsigned int	glb_virtual;
extern unsigned int	glb_logical;
extern unsigned int	glb_embedded;
#endif
extern char		sci_name[];
extern int  		eeprom_fd;
extern int              eeprom_opened;


/************************
* Functions in this file
************************/
int			init();
int			init2();
void			init_auto();
static unsigned int	hw_read_slot_id();
static int		auto_add();
static void		setup_totalmem();
static unsigned long	select_totalmem();
static unsigned long	add_totalmems();
static void		get_backplane_ids();
static int		get_nvmsci_ids();
static int		check_match();
static int		check_match_one_board();
static void		change_orig_boards();
static void		auto_exit();
static void 		auto_save_and_exit();


/**********************
* Functions used here but declared elsewhere
**********************/
extern  int             nvm_read_slot();
extern  int		nvm_initialize();
extern  void		nvm_reinitialize();
extern  int             nvm_clear();
extern  int             nvm_chk_checksum();
extern  int             nvm_board_inited();
extern  int		add_to_slot();
extern  int  		cf_configure();
extern  int  		cf_process_system_indexes();
extern  int             cf_lock_reset_restore();
extern  int		save();
extern  int		sw_status();
extern  void		sw_disp();
extern  void            mn_trapfree();
extern  void		make_cfg_file_name();
extern  void  		compile_inits();
extern  void            del_release_system();
extern  void            exiter();
extern  void		pr_init_more();
extern  void		pr_end_more();


/************
* Modes used for check_match()
************/
#define EXACT		1
#define IGNORE_REMOVES  2
#define DISPLAY_REMOVES 3
#define IGNORE_EMPTIES  4


/****+++***********************************************************************
*
* Function:     init()
*
* Parameters:   None              
*
* Used:		external only
*
* Returns:      0		ok
*		-1		problem	  (error reported here)
*				   - couldn't read eeprom
*				   - couldn't read sysbrd id from hw
*				   - revision mismatch
*
* Description:
*
*   The init function will establish a system configuration either by
*   restoring a saved configuration or rebuilding a new default one from
*   currently connected hardware. Init gurantees a minimum system by the
*   time it returns to the caller (minimum system is a configuration with
*   only a system board).
*
*   To paraphrase the first paragraph, there are two valid cases when we
*   enter this function:
*
*	(1) The eeprom has been set up before. The hardware present may or
*	    may not match the contents of the eeprom.
*
*	(2) The eeprom is empty. This is our first shot at setting up the
*	    eeprom.
*
*   Note that this function should *never* be called when we are in non-target
*   mode.
*
****+++***********************************************************************/

init()
{
    char    	    	cfg_name[MAXPATHLEN];
    unsigned char   	boardid[4];
    unsigned int    	add_options;
    int		    	status;
    nvm_slotinfo    	slot_info;


    /*****************
    * Set up some globals.
    *****************/
    nvm_current = NVM_NONE;
    del_release_system(&glb_system);
    (void)strcpy(sci_name, SCI_DEFAULT_NAME);
#ifdef VIRT_BOARD
    glb_virtual = MIN_VIRTUAL_SLOT;
    glb_logical = MIN_LOGICAL_SLOT;
    glb_embedded = MIN_EMBEDDED_SLOT;
#endif

    /******************
    * Make sure that we can read from eeprom. Errors are fatal.
    ******************/
    if (eeprom_opened == 0) {
	status = nvm_initialize(NVM_EEPROM);
	if (status != NVM_SUCCESSFUL)  {
	    switch (status)  {
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
	    return(-1);
	}
    }

    /******************
    * Get the name of the system board. Errors are fatal.
    ******************/
    status = hw_read_slot_id(SYSTEM_SLOT, boardid);
    if (status != NVM_SUCCESSFUL) {
	err_handler(INVALID_SYSBRD_ERRCODE);
	return(-1);
    }

    /*****************
    * Set the add options depending on whether we have a valid nvm.
    * There are four cases:
    *    o  The NVM does not pass its own internal checksum check. We will
    *	    issue a warning and ignore the NVM.
    *    o  The slot 0 information is garbage or not there. We either
    *       have a virgin eeprom or a bad eeprom.
    *    o  The slot 0 stuff is there, but the boardid is not the same
    *       as what is there now (they changed the sysbrd).
    *    o  The slot 0 stuff is there and matches reality.
    *
    * The first three cases are treated the same:
    *    o  put out a warning
    *    o  ignore NVM and try to auto_add based on the hardware that is
    *       present
    *
    * The last case is easy, just auto_add based on the NVM.
    *****************/
    add_options = HW_CHECKED | HW_ADDED;
    status = nvm_chk_checksum();
    if (status == 0)
	err_handler(INVALID_EEPROM_READ_ERRCODE);
    else {
	status = nvm_read_slot(SYSTEM_SLOT, &slot_info, NVM_EEPROM);
	if ( (status != NVM_SUCCESSFUL) ||
	     ( (status == NVM_SUCCESSFUL) &&
	       (memcmp((void *)slot_info.boardid, (void *)boardid, 4) != 0)) )
	    err_handler(SYSBRD_ID_CHANGED_ERRCODE);
	else
	    add_options |= AUTO_RESTORE;
    }

    /*****************
    * We read valid system board info, so construct a cfg file name for the
    * system board. Set up to call init2() and do so. init2() will bring
    * in the system board cfg file and read the data for the other (existing)
    * boards to build a configuration.
    *****************/
    nvm_current = NVM_EEPROM;
    make_cfg_file_name(cfg_name, boardid, &null_dupid);
    status = init2(cfg_name, add_options);

    /***************
    * Set up all of the initial values for the inits.
    ***************/
    if (status == 0)
	compile_inits(&glb_system, 2);

    return(status);

}


/****+++***********************************************************************
*
* Function:     init2()
*
* Parameters:   cfg_name
*		add_options
*
* Used:		internal and external
*
* Returns:	0		peachy -- system board added, maybe other
*				boards in nvm/sci as well
*		-1		system board not added, error message
*				already displayed
*
* Description:
*
*   This is the second stage of the initialization.  Init2 is entered
*   after a system board file name has been determined (either a valid
*   file name or NULL is acceptable).  Init2 will continue the
*   initialization process by establishing a minimum configuration and
*   then attempt to configure the rest of the system either from current
*   nvm or sci.
*
****+++***********************************************************************/

init2 (cfg_name, add_options)
    char		*cfg_name;
    unsigned int	add_options;
{
    nvm_slotinfo    	slot_info;
    unsigned int    	slotnum;
    unsigned int    	err;
    char		brd_cfg_name[80];


    /***************
    * Try to add the system board to the configuration. On entry to init2(),
    * we have a cfg_name for a system board gleaned from either nvm or
    * from a user-specified sci file.
    *
    * If we get an error back from auto_add which indicates that we had a
    * cfg mismatch problem, we'll tell the user that the system board CFG file
    * has changed and we will use the new CFG file. This means we can't trust
    * any of the NVM anymore, so we will ignore it.
    ***************/
    err = auto_add(cfg_name, SYSTEM_SLOT, add_options);
    if (err == -3)  {
	err_add_string_parm(1, cfg_name);
	err_handler(SYSBRD_CFG_FILE_DIFF_ERRCODE);
	add_options &= ~AUTO_RESTORE;
	err = auto_add(cfg_name, SYSTEM_SLOT, add_options);
    }
    if (err != 0)  {
	err_handler(INVALID_SYSBRD_ERRCODE);
	return(-1);
    }

    /**************
    * At this point, we have successfully added the system board into the
    * configuration. If we are restoring from existing nvm or sci, add
    * the rest of the boards into the configuration.
    **************/
    if (add_options & AUTO_RESTORE)  {

#ifdef VIRT_BOARD
	for (slotnum = 1; slotnum <= MAX_VIRTUAL_SLOT; slotnum++) {
#else
	for (slotnum = 1; slotnum < MAX_NUM_SLOTS; slotnum++) {
#endif

	    err = nvm_read_slot(slotnum, &slot_info, NVM_CURRENT);

	    switch (err) {

		/****************
		* We encountered some error other than empty slot. If this
		* is a non-physical slot, forget about the error; otherwise,
		* drop through and deal with it.
		****************/
		default:
#ifdef VIRT_BOARD
		    if (slotnum >= MIN_VIRTUAL_SLOT)
			break;
#endif
		case NVM_EMPTY_SLOT:
		    break;

		/****************
		* The nvm/sci thinks that there is a board in this slot.
		* Autoadd the board saved in nvm/sci to the configuration.
		****************/
		case NVM_SUCCESSFUL:
		    make_cfg_file_name(brd_cfg_name, slot_info.boardid,
		    		       &slot_info.dupid);
		    (void)auto_add(brd_cfg_name, slotnum, add_options);
		    break;

	    }
	
	}

    }

    /*****************
    * Now unlock all of the current boards so that users can change
    * choices if they want to.
    *****************/
    (void)cf_lock_reset_restore(CF_EEPROM_UNLOCK, CF_SYSTEM_LEVEL,
				&glb_system, (struct board *)NULL);

    return(0);

}


/****+++***********************************************************************
*
* Function:     auto_add()
*
* Parameters:   cfg_name	cfg file name (not validated on entry)
*		slot_number	what are the rules for this
*		options		only the AUTO_RESTORE bit is used here
*
* Used:		internal only
*
* Returns:	0		added successfully
*		-1		not added successfully (add_to_slot failed)
*		-2		cf_configure failed (resource problem)
*		-3		add_to_slot found a checksum mismatch on the
*				system board
*
* Description:
*
*   The auto_add function will add a board to the current system. The
*   slot where the board appears is assumed known.
*
****+++***********************************************************************/

static int auto_add(cfg_name, slot_number, options)
    char		*cfg_name;
    unsigned int	slot_number;
    unsigned int	options;
{
    struct board 	*board;
    int			rc;


    /************
    * Add the board to the slot which was passed in. If there was an error in
    * loading the sysbrd cfg file, the message will be displayed by the caller
    * of auto_add. All other error types have already had messages displayed
    * by add_to_slot or its descendents. If we are not running in automatic
    * mode, we will add a "board not added" message here on non-resource
    * errors.
    ************/
    rc = add_to_slot(cfg_name, slot_number, &board, options);
    if (rc == -3)
	return(-3);
    else if (rc != 0) {
	if (program_mode.automatic == 0) {
	    err_add_num_parm(1, slot_number);
	    err_handler(BOARD_NOT_ADDED_ERRCODE);
	}
	return(-1);
    }

    /**************
    * Copy the system indexes from eeprom to current. Build the totalmem
    * structures.
    **************/
    (void)cf_process_system_indexes(&glb_system, CF_EEPROM_NDX, CF_CURRENT_NDX,
			    	    CF_COPY_MODE, (void * )NULL);
    setup_totalmem(board, options & AUTO_RESTORE);

    /**************
    * If this board was part of the configuration before, lock it down
    * so that no choices (or resources) will be changed.
    **************/
    if (options & AUTO_RESTORE)
	(void)cf_lock_reset_restore(CF_LOCK_MODE, CF_BOARD_LEVEL,
				    (struct system *)NULL, board);

    /******************
    * Re-configure the system with the new board. If there is some sort of
    * resource conflict, the board will be removed by cf_configure(). Return
    * a different error code so that the caller can distinguish this case.
    ******************/
    if (cf_configure(&glb_system, CF_ADD_OP) != 0)
	return(-2);

    return(0);

}


/****+++***********************************************************************
*
* Function:     hw_read_slot_id()
*
* Parameters:   slot		the slot number to get the id for
*		id		the compressed board id of the board currently
*				in the slot
*
* Used:		internal only
*
* Returns:	NVM_SUCCESSFUL		successfully read
*		NVM_INVALID_SLOT 	bad slot number coming in
*		NVM_EMPTY_SLOT		slot currently empty -or- occupied
*					by a card without a readable id (ISA)
*
* Description:
*
*    This function is responsible for reading the board id register for the
*    board which currently occupies the slot in question. This function
*    assumes that the eeprom driver has already been opened. Note that we
*    cannot tell the difference between an empty slot and a slot that contains
*    an ISA card. Also note that we are getting the id by going through the
*    eeprom driver. It has scanned the backplane and recorded ids at boot time.
*
****+++***********************************************************************/

static unsigned int hw_read_slot_id(slot, id)
    unsigned int	slot;
    unsigned char	*id;
{
    unsigned int	slot_id;
    int			rc;


    /* do the ioctl */
    slot_id = slot;
    rc = ioctl(eeprom_fd, READ_SLOT_ID, &slot_id);

    /* handle the return */
    if (rc == 0) {
	if (slot_id == 0)
	    return(NVM_EMPTY_SLOT);
	id[0] = slot_id >> 24;
	id[1] = (slot_id >> 16) & 0xff;
	id[2] = (slot_id >> 8) & 0xff;
	id[3] = slot_id & 0xff;
	return(NVM_SUCCESSFUL);
    }
    return(NVM_INVALID_SLOT);

}


/****+++***********************************************************************
*
* Function:     setup_totalmem
*
* Parameters:   board 		board to setup totalmem for
*   		auto_add_flag 	if not, set to default
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   The setup_totalmem function sets the totalmem structures for the
*   board.  Both totalmem->eeprom and totalmem->actual is set by this
*   function.
*
****+++***********************************************************************/

static void setup_totalmem(board, auto_add_flag)
    struct board 	*board;
    unsigned int 	auto_add_flag;
{
    struct group 	*group;
    struct function 	*func;
    struct subfunction 	*subf;
    struct choice 	*ch;
    unsigned long   	actual;


    for (group=board->groups ; group ; group=group->next)

	for (func=group->functions ; func ; func=func->next)

	    for (subf=func->subfunctions ; subf ; subf=subf->next) {

		ch = subf->current;

		if (ch->totalmem)

		    if (auto_add_flag) {
			actual = add_totalmems(ch->primary) + 
				 add_totalmems(ch->current);
			ch->totalmem->eeprom = 
				    select_totalmem(actual, ch->totalmem);
			if (ch->totalmem->eeprom != -1)
			    ch->totalmem->actual = ch->totalmem->eeprom;
		    }

		    else {
			if (ch->totalmem->step == NULL)
			    ch->totalmem->eeprom = ch->totalmem->u.values->min;
			else
			    ch->totalmem->eeprom = ch->totalmem->u.range.min;
			ch->totalmem->actual = ch->totalmem->eeprom;
		    }

	    }
}


/****+++***********************************************************************
*
* Function:     select_totalmem()
*
* Parameters:   actual
*		totalmem
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

static unsigned long select_totalmem(actual, totalmem)
    unsigned long	actual;
    struct totalmem 	*totalmem;
{
    unsigned long   	best = -1;
    unsigned long   	size;
    struct value 	*value;


    if (totalmem->step) {

	for (size = totalmem->u.range.min; 
	    size <= totalmem->u.range.max; 
	    size += totalmem->step) {
	    if ( (size >= actual) && (size <= best) )
		best = size;
	}

	if (best == -1)
	    best = totalmem->u.range.min;

    }

    else {

	for (value=totalmem->u.values ; value ; value=value->next) {
	    size = value->min;
	    if ( (size >= actual) && (size <= best) )
		best = size;
	}

	if (best == -1)
	    best = totalmem->u.values->min;

    }

    return(best);
}





/****+++***********************************************************************
*
* Function:     add_totalmems()
*
* Parameters:   subchoice         
*
* Used:		internal only
*
* Returns:	total memory used
*
* Description:
*
*    This function adds together all of the memory requirements for the
*    subchoice which is passed in.
*
****+++***********************************************************************/

static unsigned long add_totalmems(subchoice)
    struct subchoice 		*subchoice;
{
    unsigned long   		totalmem;
    struct resource 		*res;
    struct resource_group 	*resgrp;
    struct memory 		*memory;


    totalmem = 0;

    if (subchoice)

	for (resgrp=subchoice->resource_groups ; resgrp ; resgrp=resgrp->next)

	    for (res=resgrp->resources ; res ; res=res->next)

		if (res->type == rt_memory) {
		    memory = (struct memory *)res;
		    if (memory->memtype != mt_vir)
			if (memory->memory.step)
			    totalmem += memory->memory.u.range.current;
			else
			    totalmem += memory->memory.u.list.current->min;
		}

    return(totalmem);
}


/****+++***********************************************************************
*
* Function:     init_auto()
*
* Parameters:   None
*
* Used:         external only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to automatically configure the EISA backplane.
*	User intervention is only required when hardware switches or jumpers
*	are present.
*
****+++***********************************************************************/

void init_auto()
{
    int                 status;
    unsigned int	hw_slot_ids[MAX_NUM_SLOTS];
    unsigned int	nvm_slot_ids[MAX_NUM_SLOTS];
    unsigned char	readable_ids[MAX_NUM_SLOTS];
    nvm_dupid     	dupids[MAX_NUM_SLOTS];
    unsigned char	orig_boards[MAX_NUM_SLOTS];
    unsigned char	boot_boards[MAX_NUM_SLOTS];
    unsigned int	first_invalid_slot;
    int			bad_nvm;
    unsigned int	i;
    int			added_board;
    int			some_error;
    int			any_inited = 0;
    int			any_boot_board = 0;
    int			must_reboot = 0;
    int			ignore_current = 0;
    char                cfg_name[MAXPATHLEN];
    int			old_board_resources_changed = 0;
    int			res_fail_on_this_board;



    /******************
    * Get the ids for all of the boards currently in the backplane. If we
    * run into an error here, we will not return (exit done in
    * get_backplane_ids()).
    *****************/
    get_backplane_ids(hw_slot_ids, &first_invalid_slot);

    /************
    * Read the NVM for all of the slots which are present on the system.
    ************/
    bad_nvm = get_nvmsci_ids(first_invalid_slot, nvm_slot_ids, readable_ids,
			     dupids, NVM_EEPROM);

    /*************
    * Check for a match -- if everything is the same, exit immediately.
    * If boards have been removed, but everything else is the same, exit
    * after telling the user which board(s) have been removed.
    * If the NVM is garbage, don't bother looking for a match.
    *************/
    if (bad_nvm == 0) {
	if (check_match(hw_slot_ids, nvm_slot_ids, readable_ids, 
	                first_invalid_slot, EXACT) == 1)
	    exiter(EXIT_OK_NO_REBOOT);
	else if (check_match(hw_slot_ids, nvm_slot_ids, readable_ids, 
			     first_invalid_slot, IGNORE_REMOVES) == 1) {
	    (void)check_match(hw_slot_ids, nvm_slot_ids, readable_ids, 
			      first_invalid_slot, DISPLAY_REMOVES);
	    exiter(EXIT_OK_NO_REBOOT);
	}
    }

    /*****************
    * Figure out which boards were initialized by the kernel. This lets us
    * figure out if we have a boot board that was initialized, but shows up
    * as a non-matching board now -- boot from an unitialized board case.
    *****************/
    for (i=0 ; i<first_invalid_slot ; i++)  {
	boot_boards[i] = 0;
	orig_boards[i] = 0;
	if (nvm_board_inited((int)i) == 1)  {
	    any_inited = 1;
	    if (nvm_slot_ids[i] != hw_slot_ids[i])  {
		boot_boards[i] = 1;
		any_boot_board = 1;
	    }
	}
    }

    /************
    * We did not find an exact match with the NVM, so try the default sci file.
    * Read the SCI for all of the slots which are present on the system. If the
    * read was successful, check for a match with the boards that are present.
    * If everything is the same, we will set up to build the configuration
    * from the sci file. If we have a successful match and any of the boards
    * have already been initialized (NVM was a partial match), set up to
    * reboot at the end -- a re-init would not be safe.
    ************/
    nvm_current = NVM_EEPROM;
    (void)strcpy(sci_name, SCI_DEFAULT_NAME);
    status = nvm_initialize(NVM_SCI);
    if (status == NVM_SUCCESSFUL)  {
	unsigned int	sci_slot_ids[MAX_NUM_SLOTS];
	unsigned char	sci_readable_ids[MAX_NUM_SLOTS];
	nvm_dupid     	sci_dupids[MAX_NUM_SLOTS];
	status = get_nvmsci_ids(first_invalid_slot, sci_slot_ids,
				sci_readable_ids, sci_dupids, NVM_SCI);
	if ( (status == 0) &&
	     (check_match(hw_slot_ids, sci_slot_ids, sci_readable_ids, 
			  first_invalid_slot, EXACT) == 1) ) {
	    nvm_current = NVM_SCI;
	    err_handler(AUTO_RESTORE_FROM_SCI_ERRCODE);
	    if (any_inited)
		must_reboot = 1;
	    for (i=0 ; i<first_invalid_slot ; i++) {
		readable_ids[i] = sci_readable_ids[i];
		nvm_slot_ids[i] = sci_slot_ids[i];
		dupids[i] = sci_dupids[i];
	    }
	}
    }

    /**************
    * If we detected a bad checksum on the NVM before and the SCI file does
    * not match, finish our warning here.
    **************/
    if ( (bad_nvm == 2) && (nvm_current == NVM_EEPROM) )
	err_handler(AUTO_FROM_SCRATCH_ERRCODE);

    /**************
    * At this point, we have one of three possible cases:
    *   (1) the default sci file matches the configuration exactly
    *   (2) not #1, and we detected some sort of NVM error
    *   (3) not #1 or #2, (partial match of NVM -- possibly empty set)
    * We will ignore this clause in case #2. If the NVM is trash, we will
    * try to autoadd all boards based solely on what is in the backplane
    * (the NVM will be ignored).
    *
    * We will attempt to add the system board in. There are a few
    * interesting cases:
    *
    *     o  The system board id in the NVM does not match the backplane.
    *	     This can happen when the system board actually changes -- 
    *        maybe a 1-slot EISA backplane was replaced by a 4-slot backplane.
    *        If this happens, tell the user and ignore the rest of the NVM.
    *
    *	  o  The system board matches, but the CFG file has been changed.
    *        In this case, we will tell the user and ignore the NVM.
    *
    *	  o  The system board's cfg file was not found. This is a fatal error.
    *
    *************/
    if ( (bad_nvm == 0)  ||  (nvm_current == NVM_SCI) )
	if (check_match_one_board(0, hw_slot_ids[0], nvm_slot_ids[0],
				  readable_ids[0], IGNORE_EMPTIES) )  {
	    make_cfg_file_name(cfg_name, (unsigned char *)&nvm_slot_ids[0],
			       &dupids[0]);
	    status = auto_add(cfg_name, 0, HW_ADDED|HW_CHECKED|AUTO_RESTORE);

	    /***********
	    * We got a sysbrd CFG file mismatch -- ignore the NVM and
	    * try to add from the backplane alone.
	    ***********/
	    if (status == -3) {
		ignore_current = 1;
		err_add_string_parm(1, cfg_name);
		err_handler(SYSBRD_CFG_FILE_DIFF_ERRCODE);
		status = auto_add(cfg_name, 0, HW_ADDED|HW_CHECKED);
	    }

	    /***********
	    * If we have any auto_add problem, hang it up.
	    ***********/
	    if (status != 0) {
		err_add_num_parm(1, 0);
		err_handler(AUTO_CANT_ADD_EXISTING_BOARD_ERRCODE);
		exiter(EXIT_EXISTING_BOARD_PROBLEM);
	    }
	    orig_boards[0] = 1;
	}
	else  {
	    /**************
	    * If the backplane is totally empty, just exit. This avoids the  
	    * problem of rebooting the system so that we can write an NVM
	    * with just a system board entry.
	    **************/
	    if (nvm_slot_ids[0] == SLOT_IS_EMPTY)  {
		for (i=1 ; i<first_invalid_slot ; i++)
		    if (hw_slot_ids[i] != SLOT_IS_EMPTY)
			break;
		if (i == first_invalid_slot)
		    exiter(EXIT_OK_NO_REBOOT);
		else {
		    err_handler(AUTO_FIRST_EISA_ERRCODE);
		    ignore_current = 1;
		}
	    }

	    else {
		ignore_current = 1;
		err_handler(SYSBRD_ID_CHANGED_ERRCODE);
	    }
	}

    /**************
    * We will attempt to add in any boards that match either the
    * sci file (case 1) or the nvm (case 3). Note that case 2 is explicitly
    * excluded from this clause. Also, if we did not successfully add a
    * system board from NVM data in the last clause, we will skip this
    * clause.
    *
    * Errors encountered here are fatal. If we detect an error, we will notify
    * the user of what the problem was and will exit. The configuration will
    * not be re-written. This possible errors from an auto_add are:
    *	  o  the board's cfg file was not found
    *	  o  the board's cfg file does not parse cleanly
    *	  o  the cfg file and nvm/sci contents do not match (cfg file changed)
    *     o  the board will not fit in this slot because of a tag conflict
    *     o  the board will not fit in this slot because of a busmaster conflict
    *	  o  the board could not be added without generating a resource conflict
    *
    * The tag, busmaster, and resource errors cannot occur in this context (the
    * boards are already part of the configuration in the same slots as now).
    * The cfg errors are possible, but unlikely (since we keep backup copies
    * of the cfg files in the default sci file). Therefore, an error in this
    * clause is extremely unlikely; the safest course is to tell the user what
    * has happened and exit with the previous contents of NVM intact.
    *
    * Note that the auto_add() calls here will lock down the existing boards
    * so that they cannot be changed (when we try to add in the new boards).
    * Everything is locked -- down to the resource level.
    *
    * If we have a failure adding one of the existing boards and we had a
    * first-time boot board out there, we will issue a special message so that
    * the user does not get too confused.
    *************/
    if ( (ignore_current == 0) && ((bad_nvm == 0) || (nvm_current == NVM_SCI)) )
	for (i=1 ; i<first_invalid_slot ; i++)
	    if (check_match_one_board(i, hw_slot_ids[i], nvm_slot_ids[i],
				      readable_ids[i], IGNORE_EMPTIES) )  {
		make_cfg_file_name(cfg_name, (unsigned char *)&nvm_slot_ids[i],
				   &dupids[i]);
		status = auto_add(cfg_name,i,HW_ADDED|HW_CHECKED|AUTO_RESTORE);
		if (status != 0) {

		    err_add_num_parm(1, i);
		    err_handler(AUTO_CANT_ADD_EXISTING_BOARD_ERRCODE);

		    /***************
		    * If we had a first-time boot board here, tell the user
		    * to make sure they get it added interactively.
		    ***************/
		    if (any_boot_board == 1) {
			char      item[5];
			char      buffer[60];
			status = 0;
			*buffer = 0;
			for (i=1 ; i<first_invalid_slot ; i++)
			    if (boot_boards[i] == 1) {
				(void)sprintf(item, "%u ", i);
				(void)strcat(buffer, item);
			    }
			err_add_string_parm(1, buffer);
			err_handler(AUTO_BOOT_BOARD_NOT_ADDED_1_ERRCODE);
		    }

		    exiter(EXIT_EXISTING_BOARD_PROBLEM);

		}
		orig_boards[i] = 1;
	    }

    /************
    * Now try to add any remaining boards (those that were not included in the
    * NVM). Note that we are explicitly excluding case 1 here. If the default
    * sci file is used as the basis for the configuration, it must match the
    * configuration exactly -- this implies that this clause is null for that
    * case.
    *
    * The basic strategy here is to add any new boards that can be added. If
    * an error is detected while adding a board, we will go on and try the
    * next one. The user will be notified of problems. If there were boards
    * to add, but we couldn't add any of them, we will exit without changing
    * the NVM.
    *
    * Note that first-time boot boards will be added here (they do not match
    * the NVM).
    ***********/
    added_board = 0;
    some_error = 0;
    if (nvm_current == NVM_EEPROM) {
	for (i=0 ; i<first_invalid_slot ; i++)
	    if ( (hw_slot_ids[i] != SLOT_IS_EMPTY) && (orig_boards[i] == 0) )  {
		res_fail_on_this_board = 0;
		make_cfg_file_name(cfg_name, (unsigned char *)&hw_slot_ids[i],
				   &null_dupid);
autoadd:
		status = auto_add(cfg_name, i, HW_ADDED|HW_CHECKED);

		/****************
		* If we were trying to auto_add the system board, and we had a
		* failure, exit immediately.
		****************/
		if ( (status != 0) && (i == 0) )  {
		    err_add_num_parm(1, 0);
		    err_handler(AUTO_CANT_ADD_EXISTING_BOARD_ERRCODE);
		    exiter(EXIT_EXISTING_BOARD_PROBLEM);
		}

		/**************
		* The auto_add succeeded. If this is our second attempt on this
		* board (the first failed on a resource conflict), we must note
		* that we have changed one or more resources on one of the
		* original boards. If this board is a first-time boot board,
		* set a flag so that we will reboot later (not re-init).
		**************/
		if (status == 0) {
		    if (res_fail_on_this_board == 1)
			old_board_resources_changed = 1;
		    added_board = 1;
		    err_add_num_parm(1, i);
		    err_handler(AUTO_ADD_OF_NEW_BOARD_ERRCODE);
		    if (boot_boards[i] == 1)
			must_reboot = 1;
		    /****************
		    * !!!!!!!!!!!!!!!
		    * If we ever have to make sure a reboot occurs when a
		    * given board has automatically been added, do it here
		    * by setting the must_reboot flag.
		    * !!!!!!!!!!!!!!!
		    **************/
		}

		/**************
		* We have detected a resource conflict on trying to add this
		* board.
		**************/
		else if (status == -2) {

		    /*************
		    * If this is our second attempt on this board (the first
		    * failed on a resource conflict) or if another board has
		    * already caused the original boards to have their
		    * resources unlocked, we are dead.
		    * This board cannot be added in automatic mode.
		    *
		    * If none of the original boards' resources had been changed
		    * before we started trying to auto_add this board, we must
		    * re-lock all of the original boards' resources.
		    *************/
		    if ( (res_fail_on_this_board == 1) ||
			 (old_board_resources_changed == 1) )  {
			 err_add_num_parm(1, i);
			 err_handler(AUTO_RESOURCE_CONFLICT_ERRCODE);
			 some_error = 1;
			 if (old_board_resources_changed == 0)
			     change_orig_boards(orig_boards, CF_LOCK_MODE);
		     }

		     /**************
		     * This is our first resource conflict on this board and
		     * the original boards are still using their previous
		     * resources. Set state to indicate we will retry, then
		     * change all of the existing boards so that they are
		     * locked only to the choice level -- allow resource
		     * changes.
		     **************/
		     else {
			 res_fail_on_this_board = 1;
			 change_orig_boards(orig_boards, CF_UNLOCK_MODE);
			 change_orig_boards(orig_boards, CF_LOCK_CHOICE_MODE);
			 goto autoadd;
		     }

		}

		/***************
		* We had some non-resource error on the auto_add(). The
		* specific error has already been printed, so just add
		* a trailing message and move to the next board.
		***************/
		else {
		    some_error = 1;
		    err_add_num_parm(1, i);
		    if (boot_boards[i] == 1)
			err_handler(AUTO_BOOT_BOARD_NOT_ADDED_2_ERRCODE);
		    else
			err_handler(AUTO_CANT_ADD_NEW_BOARD_ERRCODE);
		}
	    }

	/**************
	* If no new boards were successfully added, we can exit immediately.
	**************/
	if (added_board == 0)
	    exiter(EXIT_PROBLEMS_NO_REBOOT);

    }

    /************
    * Finally, save the results to eeprom and exit. Note that resource changes
    * on existing boards will force a reboot. Also, a changed system board
    * will force a reboot.
    ************/
    if ( (old_board_resources_changed == 1) || (ignore_current == 1) )
	must_reboot = 1;
    auto_save_and_exit(must_reboot, some_error);

}


/****+++***********************************************************************
*
* Function:     get_backplane_ids()
*
* Parameters:   ids			array of board ids to fill in
*		first_invalid_slot	first invalid slot to fill in
*
* Used:         internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function reads ids off of the boards currently in the backplane.
*	It fills in the ids array with the board id (for each slot). If the
*	slot is empty, the ids entry is set to SLOT_IS_EMPTY. This function
* 	also finds the first invalid slot (that is, it finds how many eisa
*	slots are in the backplane). The first slot whose status comes back
*	NVM_INVALID_SLOT is the first invalid slot.
*
*	Errors here are fatal. There are three possibilities:
*	    o   Open fails with NVM_EEPROM_IN_USE (eeprom driver already open)
*		This should be impossible when run from /etc/rc.
*	    o   Open fails with some other error. The three cases here are
*		no eeprom/bad eeprom/no eeprom driver in kernel.
*	    o	The system board slot looks empty or there are no valid
*		slots except for the system board slot.
*
****+++***********************************************************************/

static void get_backplane_ids(ids, first_invalid_slot)
    unsigned int	ids[];
    unsigned int	*first_invalid_slot;
{
    int                 status;
    unsigned int        i;


    /******************
    * Make sure that we can read from eeprom. Errors are fatal.
    ******************/
    status = nvm_initialize(NVM_EEPROM);
    if (status == NVM_EEPROM_IN_USE)  {
	err_handler(EEPROM_IN_USE_ERRCODE);
	exiter(EXIT_EEPROM_BUSY);
    }
    if (status == NVM_NO_EISA_HW)  {
	/* don't print a message here -- just means this box does
	   not have any eisa hardware (not an error).  */
	exiter(EXIT_NO_EEPROM);
    }
    if (status == NVM_NO_EEPROM_DRIVER)  {
	err_handler(NO_EEPROM_DRIVER_ERRCODE);
	exiter(EXIT_NO_EEPROM_DRIVER);
    }
    if (status == NVM_INVALID_EEPROM)  {
	err_handler(INVALID_EEPROM_ERRCODE);
	exiter(EXIT_EEPROM_ERROR);
    }

    /*************
    * Read what is in each slot and figure out what the last slot is.
    * Note that ids is only filled in for valid slots.
    *************/
    *first_invalid_slot = MAX_NUM_SLOTS;
    for (i=0 ; i<MAX_NUM_SLOTS ; i++)  {
	status = hw_read_slot_id(i, (unsigned char *)&ids[i]);
	if (status == NVM_INVALID_SLOT) {
	    *first_invalid_slot = i;
	    break;
	}
	else if (status == NVM_EMPTY_SLOT)
	    ids[i] = SLOT_IS_EMPTY;
    }

    /*************
    * Now make sure that we got a system board id and that we have at least
    * one additional slot. Errors here are fatal.
    *************/
    if ( (*first_invalid_slot < 2)  ||  (ids[0] == SLOT_IS_EMPTY) )  {
	err_handler(INVALID_EEPROM_ERRCODE);
	exiter(EXIT_EEPROM_ERROR);
    }

}


/****+++***********************************************************************
*
* Function:     get_nvmsci_ids()
*
* Parameters:   first_invalid_slot	first slot not to do
*		nvmsci_ids		board id from nvm or sci (fill in here)
*		readable_ids		does board have a readable id register?
*					   (fill in here)
*		dupids			is a duplicate cfg file name used?
*		target			NVM_EEPROM or NVM_SCI
*
* Used:         internal only
*
* Returns:      0			successful
*		1			unsuccessful (one or more slots' info
*					is garbage -- don't trust anything
*					this source)
*		2			like 1, except we told the user
*
* Description:
*
*	This function pulls per slot information out of either the NVM or
*	the default SCI file. For each slot, it grabs the board id and
*	stuffs it into the corresponding nvmsci_ids entry. If the slot is
*	empty, the entry is stuffed with SLOT_IS_EMPTY. If a board is found
*	in a slot, its readable id register status is stuffed into
*	readable_ids.
*
*	If we are grabbing stuff from the NVM, make sure that the NVM
*	checksum is consistent. If it is not, warn the user and exit
*	immediately with bad status. The NVM will not be trusted.
*
****+++***********************************************************************/

static int get_nvmsci_ids(first_invalid_slot, nvmsci_ids, readable_ids, dupids,
			  target)
    unsigned int	first_invalid_slot;
    unsigned int	nvmsci_ids[];
    unsigned char	readable_ids[];
    nvm_dupid		dupids[];
    unsigned int	target;
{
    int                 status;
    unsigned int        i;
    nvm_slotinfo        slot_info;


    /*************
    * If this is an eeprom read, make sure the eeprom is ok. If not, exit
    * after warning the user that we will not use NVM.
    *************/
    if (target == NVM_EEPROM) {
	status = nvm_chk_checksum();
	if (status == 0) {
	    err_handler(INVALID_EEPROM_READ_ERRCODE);
	    return(2);
	}
    }

    /***************
    * Walk through each of the valid slots and fill in the boardid and
    * readable id fields.
    ***************/
    for (i=0 ; i<first_invalid_slot ; i++) {
	status = nvm_read_slot(i, &slot_info, target);
	readable_ids[i] = 0;
	if (status == NVM_SUCCESSFUL) {
	    (void)memcpy((void *)&nvmsci_ids[i], (void *)slot_info.boardid, 4);
	    if (slot_info.dupid.noreadid == 0)
		readable_ids[i] = 1;
	    dupids[i] = slot_info.dupid;
	}
	else if (status == NVM_EMPTY_SLOT)
	    nvmsci_ids[i] = SLOT_IS_EMPTY;
	else
	    return(1);
    }

    return(0);

}


/****+++***********************************************************************
*
* Function:     check_match()
*
* Parameters:   hw_slot_ids		board ids for boards that are there
*		nvm_slot_ids		board ids form nvm/sci
*		readable_ids		do boards have readable ids?
*		first_invalid_slot	first slot that is not there
*		mode			
*		   EXACT		return 1 if there is an exact match
*		   IGNORE_REMOVES	return 1 if exact match or if board(s)
*					have been removed from backplane
*		   DISPLAY_REMOVES	tell which boards aren't there now
*
* Used:         internal only
*
* Returns:      1			boards match nvm/sci
*		0			boards don't match nvm/sci
*
* Description:
*
*	This function determines whether the boards in the backplane match
*       what was previously saved in the nvm or sci. If all boards match,
*       1 is returned. Otherwise, a 0 is returned.
*
*	When the mode is DISPLAY_REMOVES, the behavior is slightly different.
*	No checks are made. Instead, a message is displayed saying that one
*	or more boards has been removed. The boards that were removed are then
*	listed. This function should only be called with this mode when it is
*	already known that some board(s) were removed.
*
****+++***********************************************************************/

static int check_match(hw_slot_ids, nvm_slot_ids, readable_ids,
		       first_invalid_slot, mode)
    unsigned int	hw_slot_ids[];
    unsigned int	nvm_slot_ids[];
    unsigned char	readable_ids[];
    unsigned int	first_invalid_slot;
    unsigned int	mode;
{
    unsigned int	i;


    if (mode == DISPLAY_REMOVES) 
	err_handler(AUTO_BOARDS_REMOVED_ERRCODE);

    for (i=0 ; i<first_invalid_slot ; i++)
	if (check_match_one_board(i, hw_slot_ids[i], nvm_slot_ids[i],
				  readable_ids[i], mode) == 0)
	    return(0);

    return(1);
}


/****+++***********************************************************************
*
* Function:     check_match_one_board()
*
* Parameters:   slot_num		slot number of this board
*               slot_id 		board id for board that is there
*		nvm_slot_id 		board id from nvm/sci
*		readable_id		did board have readable id?
*		mode			
*		   EXACT		return 1 if there is an exact match
*		   IGNORE_REMOVES	return 1 if exact match or if board
*					has been removed from backplane
*		   DISPLAY_REMOVES	display a message if the board is gone
*		   IGNORE_EMPTIES	if both slots are empty, do not match
*
* Used:         internal only
*
* Returns:      1			board matches nvm/sci
*		0			board doesn't match nvm/sci
*
* Description:
*
*	This function determines whether a given board in the backplane matches
*       what was previously saved in the nvm or sci. If the board matches,
*       1 is returned. Otherwise, a 0 is returned.
*
*       There are three ways that a board can match the nvm/sci:
*	   o   if the board ids match
*	   o   if the backplane says empty and the nvm/sci said we had a
*	       board in this slot that did not have a readable id register
*          o  if the backplane says empty and the mode is IGNORE_REMOVES.
*
*	If the mode is DISPLAY_REMOVES and the board was removed, a line is
*	displayed for that board.
*
****+++***********************************************************************/

static int check_match_one_board(slot_num, slot_id, nvm_slot_id, readable_id, mode)
    unsigned int	slot_num;
    unsigned int	slot_id;
    unsigned int	nvm_slot_id;
    unsigned char	readable_id;
    unsigned int	mode;
{

    /*************
    * If both slots are empty and we are ignoring empty slots, return no
    * match.
    *************/
    if ( (mode == IGNORE_EMPTIES)  &&  (slot_id == SLOT_IS_EMPTY)  &&
	 (nvm_slot_id == SLOT_IS_EMPTY) )
	return(0);

    /*************
    * If the ids match, it is the same board.
    *************/
    if (slot_id == nvm_slot_id)
	return(1);

    /*************
    * Otherwise, if the slot looks empty and we previously had a board in
    * that slot which was not readable, assume the same board is still
    * there.
    *************/
    if ( (slot_id == SLOT_IS_EMPTY) && (readable_id == 0) )
	return(1);

    /************
    * If we had a readable board before but detect nothing now  --and--
    * we can ignore removed boards, call it a match.
    ************/
    if ( (slot_id == SLOT_IS_EMPTY) && (mode == IGNORE_REMOVES) )
	return(1);

    /************
    * If we had a readable board before but detect nothing now  --and--
    * we are displaying removed boards, do so now.
    ************/
    if ( (slot_id == SLOT_IS_EMPTY) && (mode == DISPLAY_REMOVES) )  {
	err_add_num_parm(1, slot_num);
	err_handler(AUTO_BOARD_REMOVED_SLOT_ERRCODE);
	return(1);
    }

    /************
    * If none of those conditions above were hit, we do not have a match.
    ************/
    return(0);

}


/****+++***********************************************************************
*
* Function:     change_orig_boards()
*
* Parameters:   orig			list of original boards
*               op			what operation to do
*		    CF_LOCK_MODE	  lock all original boards
*		    CF_LOCK_CHOICE_ MODE  lock all original boards (to choice
*						level
*		    CF_UNLOCK_MODE	   unlock all original boards
*
* Used:         internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function does the specified operation on each of the original
*	boards in the system. It is used in automatic mode to lock and
*	unlock the boards which were previously in the NVM.
*
****+++***********************************************************************/

static void change_orig_boards(orig, op)
    unsigned char	orig[];
    int			op;
{
    struct board	*brd;
    int			slotnum;
	

    /*************
    * Get a pointer to the first board to check.
    *************/
    brd = glb_system.boards;

    /************
    * Walk through each of the boards in the system. If the board was
    * part of the "original" system, perform the operation specified
    * on it.
    ************/
    while (brd != NULL)  {
	slotnum = brd->eisa_slot;
	if (orig[slotnum] == 1)
	    (void)cf_lock_reset_restore(op, CF_BOARD_LEVEL,
					(struct system *)NULL, brd);
	brd = brd->next;
    }

}


/****+++***********************************************************************
*
* Function:     auto_save_and_exit()
*
* Parameters:   must_reboot		do we *have* to do a reboot?
*		partial_only		not all boards could be added
*
* Used:         internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to save the config and then exit.
*
****+++***********************************************************************/

static void auto_save_and_exit(must_reboot, partial_only)
    int			must_reboot;
    int			partial_only;
{
    int                 status;
    int         	c;
    char        	buffer[256];


    /************
    * Save the results to eeprom.
    * The possible errors are:
    *	o  clear of NVM fails
    *   o  write to NVM fails
    *   o  write to default SCI fails
    *   o  resource overload detected -- this needs to be detected at add time
    *	   instead!!
    * These errors are all reported inside of save().
    *
    * If the nvm save worked but the sci save failed go on anyway (with a
    * warning). If nvm save fails, exit immediately.
    ************/
    status = save((char *)NULL);
    if (status == CANT_WRITE_EEPROM_ERRCODE)
	exiter(EXIT_PROBLEMS_NO_REBOOT);

    /************
    * Now figure out if we have to display any switches. If so, display them
    * and ask the user if the switches are ok. If they are, we can just reboot.
    * Otherwise, we will have to halt.
    ************/
    status = sw_status(&glb_system);

    if (status == 0)
	auto_exit(partial_only, must_reboot);

    pr_init_more();
    err_handler(AUTO_SWITCH_ERRCODE);
    sw_disp(SWITCH_HARDWARE, (struct board *)NULL);
    pr_end_more();
    err_handler(AUTO_SWITCH_PROMPT_ERRCODE);

    status = 0;
    while ((c = getc(stdin)) != '\n')
	buffer[status++] = c;
    err_handler(BLANK_LINE_ERRCODE);

    /**************
    * If the switches are ok, just need to reboot. Otherwise, we will
    * need to halt.
    **************/
    if ( (buffer[0] == 'y')  ||  (buffer[0] == 'Y') )
	auto_exit(partial_only, must_reboot);
    err_handler(AUTO_SWITCH_HALT_ERRCODE);
    if (partial_only)
	exiter(EXIT_PARTIAL_REBOOT_HALT);
    exiter(EXIT_OK_REBOOT_HALT);

}


/****+++***********************************************************************
*
* Function:     auto_exit()
*
* Parameters:   error			was there some kind of error?
*               should_reboot		must we reboot?
*
* Used:         internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called when we have made some changes to eeprom and
*	it is time to exit from automatic mode. If we did not have to change
* 	any existing boards, we can just re-init.
*
*	The correct exit is done.
*
****+++***********************************************************************/

static void auto_exit(error, should_reboot)
    int			error;
    int			should_reboot;
{
	

    /*************
    * If we did not have to change any of the existing boards, we can just
    * do a re-init here, which will just init the drivers for newly added
    * boards.
    *************/
    if (should_reboot == 0) {
	err_handler(AUTO_TRYING_REINIT_ERRCODE);
	nvm_reinitialize();
	if (error)
	    exiter(EXIT_PARTIAL);
	exiter(EXIT_OK_NO_REBOOT);
    }

    /**************
    * Now handle the reboot cases.
    **************/
    err_handler(AUTO_MUST_REBOOT_ERRCODE);
    if (error)
	exiter(EXIT_PARTIAL_REBOOT);
    exiter(EXIT_OK_REBOOT);

}
