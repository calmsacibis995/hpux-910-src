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
*			src/show.c
*
*	This file contains routines to handle the show commands (display
*       parts of the configuration).
*
*		show()		-- main.c
*		show_cfg()	-- main.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "config.h"
#include "pr.h"
#include "err.h"
#include "sw.h"
#include "nvm.h"
#include "compat.h"


/***********
* Titles
***********/
#define  CFGFILE_NAME		"CFG file: "
#define  FUNCTION_HDR 		"Function names and possible choices:"
#define  CURRENT_CHOICE		" [** current **]"
#define	 SLOTS_HDR		"Slot      CFG File      Contents"
#define  EMPTY_INDICATOR	"** EMPTY **"
#define  CFGFILE_HDR		"Filename    Board Type   Board Name"
#define  CFGTYPE_HDR		"Board Type                             Number of CFG Files"


/***********
* Generic labels
***********/
#define  SUPPORTED		"Supported"
#define  NOT_SUPPORTED		"Not supported"
#define  VALID			"Valid"
#define  INVALID		"Invalid"
#define  YES  			"Yes"
#define  NO  			"No"
#define  MILLIAMPS        	"milliamps"
#define  MILLIMETERS      	"millimeters"
#define  MICROSECONDS     	"microseconds"


/**********
* Board identification labels
**********/
#define  IDENT_ID            	"ID ..........................."
#define  IDENT_MFR           	"Manufacturer ................."
#define  IDENT_CATEGORY      	"Board type ..................."
#define  IDENT_SLOT_TYPE     	"Board slot type .............."
#define  IDENT_AMPERAGE      	"Current Required ............."
#define  IDENT_LENGTH        	"Length ......................."
#define  IDENT_BUS_MASTER    	"Busmaster ...................."
#define  IDENT_SKIRT         	"Skirt ........................"
#define  IDENT_READABLE_ID	"Readable ID .................."
#define  IDENT_IO_CHECK      	"I/O check ...................."
#define  IDENT_DISABLE       	"Disable ......................"


/***********
* Slot types
***********/
#define  SLOTTYPE_EMBEDDED	"Embedded"
#define  SLOTTYPE_VIRTUAL    	"Virtual"
#define  SLOTTYPE_8BIT       	"ISA 8 Bit"
#define  SLOTTYPE_8OR16BIT   	"ISA 8 or 16 Bit"
#define  SLOTTYPE_16BIT      	"ISA 16 Bit"
#define  SLOTTYPE_32BIT      	"EISA"
#define  SLOTTYPE_OTHER      	"Other"


/***************
* Modes for print_board() and print_brd_body()
***************/
#define  NOT_IN_CONFIG		1
#define  SHOW_ALL		2


/***************
* Functions declared in this file
***************/
void             	show();
void             	show_cfg();
static void             show_switch();
static void 		print_board();
static void 		print_all_boards();
static void 		print_brd_body();
static void		print_choice();
static void 		print_ident_fields();	   
static void 		print_brd_hdr();	   
static void		print_all_slots();
static void 		decode_category();
static void		show_resources();
static void		show_irq();
static void		show_dma();
static void		show_port();
static void		show_memory();


/*************
* Functions used in this file but declared elsewhere
*************/
extern void 		*mn_trapcalloc();
extern void 		*mn_traprealloc();
extern void 		mn_trapfree();
extern void             sw_disp();
extern void		compile_inits();
extern struct system    *cfg_load_temp();
extern struct board 	*get_this_board();
extern FILE		*open_cfg_file();
extern void		get_cfg_file_base_name();
extern void		del_release_system();
extern void   		make_cfg_file_name();
extern void		make_board_id();


/*************
* Globals declared elsewhere
*************/
extern struct system 	glb_system;
extern int		parse_err_no_print;


/*************
* Various defines 
*************/
#define IDENT_HDR_LENGTH       	30   /* size of each IDENT_* string */

#define GROUPWIDTH      	70
#define FUNC40          	40
#define SUBFUNC40       	40
#define FUNC70          	70

#define SLOTS_CFGFILE_OFFSET    10
#define SLOTS_CONTENTS_OFFSET   24
#define SLOTS_BRD_SIZE		(MAXCOL - L_MARGIN - SLOTS_CONTENTS_OFFSET - 2)

#define CFGFILE_CAT_OFFSET	14
#define CFGFILE_BRD_OFFSET	25
#define CFGFILE_FILE_SIZE	12
#define CFGFILE_CAT_SIZE	 9
#define CFGFILE_BRD_SIZE	50

#define CFGTYPE_COUNT_OFFSET    42

#define TOTAL_SLOTS		64


/****+++***********************************************************************
*
* Function:	show()
*
* Parameters:   num_parms	number of parameters user typed
*               type		what kind of "show" is this?:
*				    board
*				    resource
*				    switch
*				    slots
*				    (nothing)
*		str1		depends on type above:
*				    board      ==>  cfgfile
*				    switch     ==>  "changed"
*				    slots      ==>  cfgfile
*		str2		depends on type above:
*				    switch     ==>  "changed"
*		slotnum		for board and switch only
*		int1		depends on type above:
*				    switch     ==>  slotnum
*		interactive	is this show for interactive use?
*		
*				
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	This function handles all of the interactive "show" commands. Note that
*	there are lots of possibilities:
*		show board [ cfgfile | slotnum]
*		show resource
*		show switch [ "changed" ] [ slotnum ]
*		show slots cfgfile
*		show
*
*	We will accept the plural forms of each the types (e.g. board and
*       boards). Note that "changed" is a keyword. Note also that the parameters
*	can be in any order in the switch case; in all others, the
*	order is important.
*
****+++***********************************************************************/

void show(num_parms, type, str1, str2, slotnum, int1, interactive)
    int			num_parms;
    char		*type;
    char		*str1;
    char		*str2;
    int			slotnum;
    int			int1;
    int			interactive;
{
    int			parm_error=0;
    struct board 	*brd;
    struct system	*sys;
    unsigned 		i;
    int                 num_avail_slots;
    int                 avail_slots[MAX_NUM_SLOTS];
    char                buffer[80];
    char                tbuf[10];


    /***************
    * If no parameters were given, default it to a show slots.
    ***************/
    if (num_parms == 0) {
	print_all_slots(&glb_system, interactive);
    }

    /***************
    * Handle the board case.
    ***************/
    else if ( (strcmpi(type, CMD_KEY_BOARD) == 0)   ||
	      (strcmpi(type, CMD_KEY_BOARDS) == 0) )   {
	if (num_parms == 1) {
	    print_all_boards(&glb_system);
	}
	else if (num_parms == 2) {
	    if (slotnum != NOT_AN_INT) {
		brd = get_this_board(slotnum);
		if (brd == NULL) {
		    /* error message has already been displayed */
		    return;
		}
		pr_init_more();
		print_board(brd, SHOW_ALL);
		pr_end_more();
	    }
	    else {
		sys = cfg_load_temp(str1);
		if (sys == NULL)
		    return;
		brd = sys->boards;
		pr_init_more();
		print_board(brd, NOT_IN_CONFIG);
		pr_end_more();
		del_release_system(sys);
	    }
	}
	else
	    parm_error = 1;
    }

    /***************
    * Handle the switch case. Note that we have five basic cases here:
    *     num_parms = 1:
    *	       switch
    *     num_parms = 2:
    *	       switch  changed
    *          switch  slotnum
    *	  num_parms = 3:
    *          switch  changed  slotnum
    *          switch  slotnum  changed
    ***************/
    else if ( (strcmpi(type, CMD_KEY_SWITCH) == 0)      ||
	      (strcmpi(type, CMD_KEY_SWITCHES1) == 0)   ||
	      (strcmpi(type, CMD_KEY_SWITCHES2) == 0) )   {
	if (num_parms == 1) {
	    show_switch(0, 0);
	}
	else if (num_parms == 2) {
	    if (slotnum != NOT_AN_INT)
		show_switch(SWITCH_ONESLOT, slotnum);
	    else if (strcmpi(str1, CMD_KEY_CHANGED) == 0)
		show_switch(SWITCH_CHANGED, 0);
	    else
		err_handler(INVALID_SHOW_CMD_ERRCODE);
	}
	else if (num_parms == 3) {
	    if ( (slotnum != NOT_AN_INT) &&
	         (strcmpi(str2, CMD_KEY_CHANGED) == 0) )
		show_switch(SWITCH_ONESLOT | SWITCH_CHANGED, slotnum);
	    else if ( (int1 != NOT_AN_INT) &&
	         (strcmpi(str1, CMD_KEY_CHANGED) == 0) )
		show_switch(SWITCH_ONESLOT | SWITCH_CHANGED, int1);
	    else
		err_handler(INVALID_SHOW_CMD_ERRCODE);
	}
	else
	    parm_error = 1;
    }

    /***************
    * Handle the available slots case.
    ***************/
    else if ( (strcmpi(type, CMD_KEY_SLOT) == 0)   ||
	      (strcmpi(type, CMD_KEY_SLOTS) == 0) )   {

	/*************
	* Show all slots (if any) which are currently empty and are valid
	* for the specified cfg file (in str1).
	*************/
	if (num_parms == 2) {
	    sys = cfg_load_temp(str1);
	    if (sys == NULL)
		return;
	    brd = sys->boards;
	    num_avail_slots = find_avail_slots(&glb_system, brd, avail_slots);
#ifdef VIRT_BOARD
	    /************
	    * Must handle num_avail_slots = -1 | -2 here for virtual or
	    * embedded boards.
	    ************/
#endif
	    if (num_avail_slots == 0)
		err_handler(NO_AVAIL_SLOTS_ERRCODE);
	    else {
		*buffer = 0;
		for (i=0 ; i<num_avail_slots ; i++) {
		    (void)sprintf(tbuf, "%d  ", avail_slots[i]);
		    (void)strcat(buffer, tbuf);
		}
		err_add_string_parm(1, buffer);
		err_handler(VALID_SLOT_LIST_ERRCODE);
	    }
	    del_release_system(sys);
	}

	else
	    parm_error = 1;
    }

    /***************
    * Handle the resources
    ***************/
    else if (strcmpi(type, "irq") == 0)
	show_resources(glb_system.irq);
    else if (strcmpi(type, "dma") == 0)
	show_resources(glb_system.dma);
    else if (strcmpi(type, "port") == 0)
	show_resources(glb_system.port);
    else if (strcmpi(type, "memory") == 0)
	show_resources(glb_system.memory);

    /***************
    * Handle the invalid type case.
    ***************/
    else {
	err_handler(INVALID_SHOW_CMD_ERRCODE);
	return;
    }

    /***************
    * If the command entered had the wrong number of parameters, tell the
    * user about it here.
    ***************/
    if (parm_error == 1)
	err_handler(INVALID_SHOW_CMD_ERRCODE);

}


/****+++***********************************************************************
*
* Function:     show_switch()
*
* Parameters:   flag		which switches should be displayed:
*				    SWITCH_ONESLOT  bit
*				    SWITCH_CHANGED  bit
*		slotnum		valid only if SWITCH_ONESLOT set (above)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to display some switches and jumpers.
*	It can display only switches that need to be changed or all switches.
*	It can display switches for a single board or for all boards.
*
*	Note that slotnum has not been validated on entry. If the slotnum
* 	is invalid (out of range or empty), it will be detected here.
*	The user will be notified here.
*
****+++***********************************************************************/

static void show_switch(flag, slotnum)
    unsigned		flag;
    int			slotnum;
{
    struct board 	*board;


    /*****************
    * Set up the currently selected values for all switches and jumpers --
    * software parms as well.
    *****************/
    compile_inits(&glb_system, 1);
    compile_inits(&glb_system, 0);

    /*****************
    * If we need to display switches for a given board only, use the specified
    * slot number to find the board struct in the current configuration.
    * If a board cannot be found, handle the error here and exit.
    *****************/
    if (flag & SWITCH_ONESLOT) {
	board = get_this_board(slotnum);
	if (board == NULL) {
	    /* error message already printed */
	    return;
	}
    }

    /*****************
    * Finally, call sw_disp() to display the switches that were selected.
    *****************/
    pr_init_more();
    flag |= SWITCH_HARDWARE | SWITCH_SOFTWARE;
    if (flag & SWITCH_ONESLOT)
	sw_disp(flag, board);
    else
	sw_disp(flag, (struct board *)NULL);
    pr_end_more();

}

/****+++***********************************************************************
*
* Function:     print_board()
*
* Parameters:   brd             board to display
*		mode		how much function info should be displayed?
*				  NOT_IN_CONFIG  (show info on all functions)
*				  SHOW_ALL (all choices -- mark current)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display information for a single board.
*
****+++***********************************************************************/

static void print_board(brd, mode)
    struct board 	*brd;
    int			mode;
{

    /************
    * Print 2 blank lines at the beginning.
    ************/
    pr_print_image("");
    pr_print_image("");

    /**********
    * Print the basic info for this board.
    **********/
    print_ident_fields(brd);

    /**********
    * Print the functions and choices for this board.
    **********/
    print_brd_body(brd, mode);

}






/****+++***********************************************************************
*
* Function:     print_all_boards()
*
* Parameters:   sys		system to display
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display information for all boards in the configuration.
*
****+++***********************************************************************/

static void print_all_boards(sys)
    struct system 	*sys;
{
    struct board 	*brd;
    unsigned 		slotnum;


    /************
    * Initialize the global line count to 0.
    ************/
    pr_init_more();

    /************
    * Walk through all of the slots and display information for the board
    * in that slot (if any).
    ************/
    for (slotnum = 0; (slotnum < TOTAL_SLOTS) ; slotnum++)  {
        for (brd = sys->boards;
	     (brd->slot_number != slotnum) && brd != NULL;
	     brd = brd->next);
	if (brd)
	    print_board(brd, SHOW_ALL);
    }
    pr_end_more();

}


/****+++***********************************************************************
*
* Function:     print_brd_body()
*
* Parameters:   brd             board to display info for
*		mode		how much function info should be displayed?
*				  NOT_IN_CONFIG  (show info on all functions)
*				  SHOW_ALL (all choices -- mark current)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the groups, functions, subfunctions, and current choices for
*    a given board.
*
*    The format is like this:
*
*        Function names and current choices:
*          groupname (if explicit)
*             funcname ..................... current choice
*             funcname
*               subfuncname ................ current choice
*               subfuncname ................ current choice
*
****+++***********************************************************************/

static void print_brd_body(brd, mode)
    struct board 	*brd;
    int			mode;
{
    struct group 	*grp;
    struct function	*func;
    struct subfunction	*subf;
    struct choice	*choi;
    char 		*groupbuf;
    char 		*funcbuf;
    int 		funcwidth;
    image_buf 		image;
    unsigned  		indent;
    int			first_time=1;
    int			func_ct=1;
    int			choice_ct;


    pr_print_image(" ");

    /* Loop while there are groups of functions to print. */
    for (grp = brd->groups; grp != NULL; grp = grp->next)  {

        /* Reset the horizontal indentation */
        indent = 4 + L_MARGIN;

        pr_clear_image(image);

        /* If the group is explicit, print the board's current group name */
        if (grp->explicit)  {

	    /* print the hdr if this is the first line we want to print */
	    if (first_time) {
		first_time = 0;
		pr_put_string(image, indent - 2, FUNCTION_HDR);
		pr_print_image(image);
		pr_clear_image(image);
	    }

	    groupbuf = (char *) mn_trapcalloc(GROUPWIDTH+1,1);
	    (void)strncpy(groupbuf,grp->name,GROUPWIDTH);
	    groupbuf[GROUPWIDTH+1] = NULL;
	    pr_put_string(image, indent, groupbuf);
	    mn_trapfree((void *)groupbuf);
	    pr_print_image(image);
	    pr_clear_image(image);

	    /* Increase horizontal indentation due to the explicit GROUP. */
	    indent += 2;

        }

        /* Loop while there are functions to print in the current group. */
        for (func = grp->functions; func != NULL; func = func->next)  {

	    /*********
	    * Move on if we're not supposed to display this one.
	    * Do not count him as a function (want the user to see the
	    * function numbers without any holes).
	    *********/
	    if (!func->display)
	        continue;

	    /* print the hdr if this is the first line we want to print */
	    if (first_time) {
		first_time = 0;
		pr_put_string(image, indent - 2, FUNCTION_HDR);
		pr_print_image(image);
		pr_clear_image(image);
	    }

	    /*********
	    * Allocate space for the function name and stuff it into image.
	    * Size the function name field bigger if subfunctions are explicit.
	    *********/
	    if (!func->subfunctions->explicit)
		funcwidth = FUNC40;
	    else
	 	funcwidth = FUNC70;
	    funcbuf = (char *) mn_trapcalloc((unsigned)(funcwidth+1), 1);
	    if (!func->subfunctions->explicit) {
		(void)sprintf(funcbuf, "F%d: ", func_ct);
		(void)strncat(funcbuf, func->name, (unsigned)(funcwidth-strlen(funcbuf)));
	    }
	    else
		(void)strncpy(funcbuf, func->name, (unsigned)funcwidth);
	    funcbuf[funcwidth+1] = NULL;
	    pr_put_string(image, indent, funcbuf);
	    pr_print_image(image);
	    pr_clear_image(image);

	    /* Create a pointer to the first subfunction of the current	 */
	    /* function. */
	    subf = func->subfunctions;

	    /************************************************************/
	    /* If the subfunction is implicit, print the current choice */
	    /* and the function name (above) on the same output line.   */
	    /* It is assumed that an implicit subfunction is the ONLY   */
	    /* subfunction of the parent function. 		     */
	    /************************************************************/
	    /* Note: There may be multiple "choices" associated with    */
	    /* the one subfunction, one of which will be the current.   */
	    /* Also Note: If the first subfunction is EXPLICIT, all the */
	    /* subfunctions which follow must also be explicit.	    */
	    /************************************************************/
	    if (!subf->explicit)  {

		choice_ct = 1;
		for (choi=subf->choices ; choi != NULL ; choi = choi->next) {
		    if (mode == NOT_IN_CONFIG)
			print_choice(choi, indent+4, (struct choice *)0, choice_ct);
		    else
			print_choice(choi, indent+4, subf->current, choice_ct);
		    choice_ct++;
		}
		
	        mn_trapfree((void *)funcbuf);

		func_ct++;

	    }

	    /* explicit subfunctions */
	    else  {

		/* Increase the indent. */
		indent += 2;

		/* Loop while there are subfunctions to print */
		for ( ; subf != NULL; subf = subf->next) {

		    funcbuf = (char *) mn_trapcalloc(SUBFUNC40+1, 1);
		    (void)sprintf(funcbuf, "F%d: ", func_ct);
		    (void)strncat(funcbuf, subf->name, (unsigned)(SUBFUNC40-strlen(funcbuf)));
		    funcbuf[SUBFUNC40+1] = NULL;
		    pr_put_string(image, indent, funcbuf);
		    pr_print_image(image);
		    pr_clear_image(image);

		    choice_ct = 1;
		    for (choi=subf->choices ; choi != NULL ; choi = choi->next) {
			if (mode == NOT_IN_CONFIG)
			    print_choice(choi, indent+4, (struct choice *)0, choice_ct);
			else
			    print_choice(choi, indent+4, subf->current, choice_ct);
			choice_ct++;
		    }
		    
		    mn_trapfree((void *)funcbuf);

		    func_ct++;

		}

		/* Decrease horiz indentation due to explicit SUBFUNCTIONS */
		indent -= 2;
		if (func->next != NULL)
		    pr_print_image(" ");

	    }
	    
	}

        /* Decrease horizontal indentation due to an explicit GROUP */
        if (grp->explicit)
	    indent -= 2;

	/* Print a blank line if another group follows this one */
	if (grp->next != NULL)
	    pr_print_image(" ");

    }

    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:     print_choice()
*
* Parameters:   choi		choice to be displayed
*    		indent		number of chars to indent
*    		cur_choi	current choice (or 0 if marker not wanted)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function prints a line with a single choice on it.
*	If a current choice is given and the
*	choice to be printed matches it, a current indicator is printed after
*	the choice name.
*
****+++***********************************************************************/

static void print_choice(choi, indent, cur_choi, count)
    struct choice	*choi;
    unsigned  		indent;
    struct choice	*cur_choi;
    int			count;
{
    image_buf 		image;
    char		buffer[MAXCOL+1];
    int			calc_position;
    int			last_possible_position;


    (void)sprintf(buffer, "CH%d: ", count);
    (void)strncat(buffer, choi->name, MAXCOL-indent-strlen(buffer));
    buffer[MAXCOL-indent] = 0;
    pr_clear_image(image);
    pr_put_string(image, indent, buffer);

    if (choi == cur_choi) {
	calc_position = indent + strlen(buffer);
	last_possible_position = MAXCOL - strlen(CURRENT_CHOICE);
	if (calc_position >= last_possible_position)
	    calc_position = last_possible_position;
	pr_put_string(image, (unsigned)calc_position, CURRENT_CHOICE);
    }

    pr_print_image(image);

}


/****+++***********************************************************************
*
* Function:     print_ident_fields()
*
* Parameters:   brd             	board to display
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the basics for a single board.
*
*    The output looks like this (* are optional lines):
*
*  |---------------------------------------------------------------------|
*  | boardname                                                           |
*  | slot #x                                                             |
*  |                                                                     |
*  |   comments *                                                        |
*  |   comments *                                                        |
*  |   comments *                                                        |
*  |                                                                     |
*  |   Manufacturer ...................... mfr                           |
*  |   ID ................................ id                            |
*  |   Board type ........................ category                      |
*  |   Board Slot Type ................... slot type, tag (if any)       |
*  |   Readable Id ....................... yes or no *                   |
*  |   Skirt ............................. yes or no *                   |
*  |   Length ............................ num millimeters *             |
*  |   Current Required .................. num milliamps *               |
*  |   Busmaster ......................... num microseconds *            |
*  |   I/O Check ......................... valid or invalid *            |
*  |   Disable ........................... supported or unsupported *    |
*  |---------------------------------------------------------------------|
*
****+++***********************************************************************/

static void print_ident_fields(brd)
    struct board 	*brd;
{
    image_buf 		image;
    char 		work_str[100];
    unsigned		indent;
    unsigned 		ident_shift;


    indent = L_MARGIN + 2;
    ident_shift = indent + IDENT_HDR_LENGTH + 1;

    /* Board name and slot number */
    print_brd_hdr(brd);

    /* Comments and a blank line */
    if (strlen(brd->comments))  {
        pr_put_text_fancy((int)indent,brd->comments,100, (int)(80-indent-R_MARGIN),0);
	pr_print_image(" ");

    }

    /* Manufacturer */
    pr_clear_image(image);
    pr_put_string(image, indent, IDENT_MFR);
    pr_put_string(image, ident_shift, brd->mfr);
    pr_print_image(image);

    /* ID */
    pr_clear_image(image);
    pr_put_string(image, indent, IDENT_ID);
    pr_put_string(image, ident_shift, brd->id);
    pr_print_image(image);

    /* Board type  */
    pr_clear_image(image);
    pr_put_string(image, indent, IDENT_CATEGORY);
    pr_put_string(image, ident_shift, brd->category);
    decode_category(brd->category, work_str);
    pr_put_string(image, ident_shift+5, work_str);
    pr_print_image(image);

    /* Slot Type and Tag */
    pr_clear_image(image);
    pr_put_string(image, indent, IDENT_SLOT_TYPE);
    switch (brd->slot)  {
        case bs_isa8:
	    (void)strcpy(work_str, SLOTTYPE_8BIT);
	    break;
        case bs_isa16:
	    (void)strcpy(work_str, SLOTTYPE_16BIT);
	    break;
        case bs_isa8or16:
	    (void)strcpy(work_str, SLOTTYPE_8OR16BIT);
	    break;
        case bs_eisa:
	    (void)strcpy(work_str, SLOTTYPE_32BIT);
	    break;
        case bs_vir:
	    (void)strcpy(work_str, SLOTTYPE_VIRTUAL);
	    break;
        case bs_emb:
	    (void)strcpy(work_str, SLOTTYPE_EMBEDDED);
	    break;
#ifdef LINT
        default:
#endif
        case bs_oth:
	    (void)strcpy(work_str, SLOTTYPE_OTHER);
	    break;
    }
    if ( (brd->slot_tag != NULL) && (strlen(brd->slot_tag) != 0) )  {
        (void)strcat(work_str, ", ");
        (void)strcat(work_str, brd->slot_tag);
    }
    pr_put_string(image, ident_shift, work_str);
    pr_print_image(image);

    /* Readable ID */
    if (brd->slot != bs_vir)  {
	pr_clear_image(image);
	pr_put_string(image, indent, IDENT_READABLE_ID);
   	pr_put_string(image,ident_shift,(brd->readid ? YES:NO));
	pr_print_image(image);
    }

    /* Skirt */
    if(brd->slot_number && (brd->slot != bs_vir) && (brd->slot != bs_emb)){
	pr_clear_image(image);
        pr_put_string(image, indent, IDENT_SKIRT);
        pr_put_string(image,ident_shift,(brd->skirt ? YES:NO));
	pr_print_image(image);
    }

    /* Board Length */
    if (brd->length && brd->slot_number && (brd->slot != bs_vir)
			&& (brd->slot != bs_emb))  {
	pr_clear_image(image);
	pr_put_string(image, indent, IDENT_LENGTH);
        (void)sprintf(work_str, "%-*d", 5, brd->length);
        pr_put_string(image, ident_shift, work_str);
        pr_put_string(image,ident_shift+strlen(work_str)+1,MILLIMETERS);
	pr_print_image(image);
    }

    /* Amperage */
    if (brd->amperage)  {
	pr_clear_image(image);
	pr_put_string(image, indent, IDENT_AMPERAGE);
        (void)sprintf(work_str, "%-*d", 5, brd->amperage);
        pr_put_string(image, ident_shift, work_str);
        pr_put_string(image, ident_shift+strlen(work_str)+1, MILLIAMPS);
	pr_print_image(image);
    }

    /* Busmaster Latency */
    if (brd->busmaster)  {
	pr_clear_image(image);
	pr_put_string(image, indent, IDENT_BUS_MASTER);
        (void)sprintf(work_str, "%-*d", 5, brd->busmaster);
        pr_put_string(image, ident_shift, work_str);
        pr_put_string(image,ident_shift+strlen(work_str)+1,MICROSECONDS);
	pr_print_image(image);
    }

#ifdef VIRT_BOARD
    /* I/O Check and Board Disable if this is a system board */
    if (strcmpi(brd->category, "sys") == 0)  {

	switch (brd->slot)  {

	    /* if embedded check for slot 0 - SYS Board */
	    case bs_emb:
		/* If board 0 (i.e. the board is a system board) break out. */
		/* If not board 0, fall through to I/O Check */
		if (brd->slot_number == 0)
		    break;

	    case bs_eisa:	  /*****************/
	    case bs_oth:	  /* if EISA board */
	    case bs_vir:	  /*****************/

		/* I/O Check */
		pr_clear_image(image);
		pr_put_string(image, indent, IDENT_IO_CHECK);
		pr_put_string(image, ident_shift,
			  (brd->iocheck ? VALID : INVALID));
		pr_print_image(image);

		/* Board Disable */
		pr_clear_image(image);
		pr_put_string(image, indent, IDENT_DISABLE);
		pr_put_string(image, ident_shift,
			  (brd->disable ? SUPPORTED : NOT_SUPPORTED));
		pr_print_image(image);

		break;

	}

    }
#endif

}


/****+++***********************************************************************
*
* Function:     print_brd_hdr()
*
* Parameters:   brd             	board to display
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the header for a single board.
*
*    The output looks like this:
*
*  |---------------------------------------------------------------------|
*  | boardname                                                           |
*  | CFG file: yyyyyyyyyyyy						 |
*  | slot #x                                                             |
*  |                                                                     |
*  |---------------------------------------------------------------------|
*
****+++***********************************************************************/

static void print_brd_hdr(brd)
    struct board 	*brd;
{
    image_buf 		image;
    FILE		*fd;
    char		filename1[64];
    char		filename2[64];
    nvm_dupid           dupid;
    unsigned int	compressed_id;


    /* Board Name */
    pr_clear_image(image);
    pr_put_string(image, L_MARGIN, brd->name);
    pr_print_image(image);

    /* CFG file name */
    (void)make_board_id(brd->id, &compressed_id);
    dupid.dup_id = brd->duplicate_id;
    make_cfg_file_name(filename2, (unsigned char *)&compressed_id, &dupid); 
    fd = open_cfg_file(filename2, filename1);
    if (fd != NULL) {
	pr_clear_image(image);
	pr_put_string(image, L_MARGIN, CFGFILE_NAME);
	get_cfg_file_base_name(filename1, filename2);
	pr_put_string(image, L_MARGIN + strlen(CFGFILE_NAME), filename2);
	pr_print_image(image);
	(void)fclose(fd);
    }

    /* Slot Number */
    pr_clear_image(image);
#ifdef VIRT_BOARD
    if (brd->slot_number >= MAX_NUM_SLOTS * 2 - 1)
	pr_put_string(image, L_MARGIN, SW_VIRTUAL);
    else if (brd->slot_number >= MAX_NUM_SLOTS-1)
	pr_put_string(image, L_MARGIN, SW_EMBEDDED);
    else if (brd->slot_number >= 0)
#else
    if (brd->slot_number >= 0)
#endif
	pr_put_string(image, L_MARGIN, glb_system.slot[brd->slot_number].label);
    pr_print_image(image);

    /* Blank Line */
    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:     print_all_slots()
*
* Parameters:   sys		system to display
*		interactive	is this for interactive use?
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display information for all slots in the configuration.
*
****+++***********************************************************************/

static void print_all_slots(sys, interactive)
    struct system 	*sys;
    int			interactive;
{
    struct board 	*brd;
    unsigned 		slot;
    image_buf		image;
    char		buf[3];
    char		filename[64];
    char		basename[64];
    FILE		*fd;
    nvm_dupid           dupid;
    unsigned int	compressed_id;


    /************
    * Display the header line and initialize the line count for scrolling.
    ************/
    pr_init_more();
    pr_clear_image(image);
    if (interactive) {
	pr_put_string(image, L_MARGIN, SLOTS_HDR);
	pr_print_image(image);
	pr_print_image(" ");
    }

    /************
    * Walk through all of the slots and display information for each of the
    * slots.
    ************/
    for (slot=0 ; slot<TOTAL_SLOTS ; slot++)  {

	/***********
	* Walk through all of the boards until we find the one that is in
	* this slot (if there is one in this slot).
	***********/
        for (brd = sys->boards;
	     (brd->slot_number != slot) && (brd != NULL);
	     brd = brd->next);

	/***********
	* If we found a board in this slot, print a line for it.
	***********/
	if (brd != NULL) {
	    pr_clear_image(image);
	    (void)sprintf(buf, "%d", slot);
	    pr_put_string(image, L_MARGIN, buf);
	    (void)make_board_id(brd->id, &compressed_id);
	    dupid.dup_id = brd->duplicate_id;
	    make_cfg_file_name(basename,(unsigned char *)&compressed_id,&dupid);
	    fd = open_cfg_file(basename, filename);
	    if (fd != NULL) {
		get_cfg_file_base_name(filename, basename);
		pr_put_string(image, L_MARGIN+SLOTS_CFGFILE_OFFSET, basename);
		(void)fclose(fd);
	    }
	    (void)strncpy(filename, brd->name, SLOTS_BRD_SIZE);
	    filename[SLOTS_BRD_SIZE] = 0;
	    pr_put_string(image, L_MARGIN+SLOTS_CONTENTS_OFFSET, filename);
	    pr_print_image(image);
	}

	/***********
	* Otherwise, if the slot is actually there, print an EMPTY 
	* indication.
	***********/
	else if ( (slot < MAX_NUM_SLOTS) && (sys->slot[slot].present) && (interactive) )  {
	    pr_clear_image(image);
	    (void)sprintf(buf, "%d", slot);
	    pr_put_string(image, L_MARGIN, buf);
	    pr_put_string(image, L_MARGIN+SLOTS_CONTENTS_OFFSET, EMPTY_INDICATOR);
	    pr_print_image(image);
	}

    }

    pr_end_more();

}


/****+++***********************************************************************
*
* Function:	show_cfg()
*
* Parameters:   mode		type of operation to do:
*				    CFG_TYPES
*				    CFG_ALL_FILES
*				    CFG_SOME_FILES
*		category	what type of boards to display (only valid if
*				   type = CFG_SOME_FILES)
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	Thsi function is responsible for handling the cfgfiles and cfgtypes
*	commands. Both commands require a scan through all of the files in
*	the cfg directory.
*
****+++***********************************************************************/

void show_cfg(mode, category)
    int			mode;
    char		*category;
{
    struct system	*sys;
    struct board	*brd;
    int			found_one = 0;
    DIR 		*dirp;
    struct dirent 	*dp;
    image_buf		image;
    int			type_count = 0;
    int 		found;
    int			i;
    char		buffer[CFGFILE_BRD_SIZE+2];
    struct category  {
	char	*name;
	int	count;
    };
    struct category	*cat;
    int			num_cat_elements;
    char		filename[CFGFILE_FILE_SIZE+1];
    char		tfilename[CFGFILE_FILE_SIZE+1];


    /************
    * Try to open the cfg file directory. If we cannot, blow this guy off
    * immediately.
    ************/
    dirp = opendir(CFG_FILES_DIR);
    if (dirp == NULL) {
	err_add_string_parm(1, CFG_FILES_DIR);
	err_handler(CANNOT_OPEN_CFG_DIR_ERRCODE);
	return;
    }

    /************
    * Allocate some initial space if we are looking for cfg file types.
    ************/
    if (mode == CFG_TYPES) {
	cat = (struct category *)mn_trapcalloc(sizeof(struct category), 5);
	num_cat_elements = 5;
    }

    /************
    * Set the global flag to indicate parser errors should not be displayed.
    ************/
    pr_init_more();
    parse_err_no_print = 1;

    /***********
    * Walk through each of the files in /etc/eisa. For each file, try to
    * parse it as a cfg file. If not successful, just go to the next file.
    * If we are successful, take action depending on the mode:
    *    CFG_TYPES:        if the board category is one we have not seen, add
    *		           it to our list of categories
    *    CFG_ALL_FILES:    display a line for this board
    *    CFG_SOME_FILES:   if the category matches the board category, display
    *			   a line for this board
    ***********/
    while ((dp = readdir(dirp)) != NULL) {

	/*************
	* Parse the file to see if it is a cfg file. Move to the next file
	* if it does not parse cleanly.
	**************/
	sys = cfg_load_temp(dp->d_name);
	if (sys == NULL)
	    continue;
	brd = sys->boards;

	/************
	* If the file name had a .CFG or .cfg suffix, strip it off here.
	************/
	(void)strncpy(tfilename, dp->d_name, CFGFILE_FILE_SIZE);
	tfilename[CFGFILE_FILE_SIZE] = 0;
	get_cfg_file_base_name(tfilename, filename);

	/************
	* If this file has a new category, add it to the list. Otherwise, just
	* bump the count for this category.
	*************/
	if (mode == CFG_TYPES) {

	    found = 0;
	    for (i=0 ; i<type_count ; i++)
		if (strcmpi(cat[i].name, brd->category) == 0) {
		    cat[i].count++;
		    found = 1;
		    break;
		}

	    if (found == 0) {
		found_one++;
		if (type_count+1 == num_cat_elements) {
		    num_cat_elements += 5;
		    cat = (struct category *)mn_traprealloc((void *)cat,
				    sizeof(struct category)*num_cat_elements);
		}
		cat[type_count].name = mn_trapcalloc(strlen(brd->category), 1);
		(void)strcpy(cat[type_count].name, brd->category);
		cat[type_count].count = 1;
		type_count++;
	    }
	    
	}
	
	/*************
	* If the user asked for all cfg files, format a single line and
	* display it.
	*************/
	else if (mode == CFG_ALL_FILES) {
	    if (found_one == 0) {
		pr_clear_image(image);
		pr_put_string(image, L_MARGIN, CFGFILE_HDR);
		pr_print_image(image);
		pr_print_image(" ");
		found_one++;
	    }
	    pr_clear_image(image);
	    pr_put_string(image, L_MARGIN, filename);
	    (void)strncpy(buffer, brd->category, CFGFILE_CAT_SIZE);
	    buffer[CFGFILE_CAT_SIZE] = 0;
	    pr_put_string(image, L_MARGIN+CFGFILE_CAT_OFFSET, buffer);
	    (void)strncpy(buffer, brd->name, CFGFILE_BRD_SIZE);
	    buffer[CFGFILE_BRD_SIZE] = 0;
	    pr_put_string(image, L_MARGIN+CFGFILE_BRD_OFFSET, buffer);
	    pr_print_image(image);
	}

	/*************
	* If the user asked for a single category only and this file matches
	* it, format a line and display it.
	*************/
	else {
	    if (strcmpi(category, brd->category) == 0) {
		if (found_one == 0) {
		    pr_clear_image(image);
		    pr_put_string(image, L_MARGIN, CFGFILE_HDR);
		    pr_print_image(image);
		    pr_print_image(" ");
		    found_one++;
		}
		pr_clear_image(image);
		pr_put_string(image, L_MARGIN, filename);
		(void)strncpy(buffer, brd->category, CFGFILE_CAT_SIZE);
		buffer[CFGFILE_CAT_SIZE] = 0;
		pr_put_string(image, L_MARGIN+CFGFILE_CAT_OFFSET, buffer);
		(void)strncpy(buffer, brd->name, CFGFILE_BRD_SIZE);
		buffer[CFGFILE_BRD_SIZE] = 0;
		pr_put_string(image, L_MARGIN+CFGFILE_BRD_OFFSET, buffer);
		pr_print_image(image);
	    }
	}

    }

    /************
    * Close the directory.
    ************/
    (void)closedir(dirp);

    /************
    * Set the global flag back to indicate parser errors should be displayed
    * again.
    ************/
    parse_err_no_print = 0;

    /***********
    * If we aren't going to display any entries, tell the user about it.
    ***********/
    if (found_one == 0) {
	if ( (mode == CFG_TYPES) || (mode == CFG_ALL_FILES) ) {
	    err_add_string_parm(1, CFG_FILES_DIR);
	    err_handler(NO_CFG_FILES_ERRCODE);
	}
	else {
	    err_add_string_parm(1, category);
	    err_add_string_parm(2, CFG_FILES_DIR);
	    err_handler(NO_CFG_FILES_OF_THIS_TYPE_ERRCODE);
	}
    }

    /**********
    * Otherwise, if we were looking for types, display them now.
    **********/
    else if (mode == CFG_TYPES)  {

	pr_clear_image(image);
	pr_put_string(image, L_MARGIN, CFGTYPE_HDR);
	pr_print_image(image);
	pr_print_image(" ");

	for (i=0 ; i<type_count ; i++) {

	    pr_clear_image(image);
	    (void)strncpy(buffer, cat[i].name, 4);
	    buffer[4] = 0;
	    pr_put_string(image, L_MARGIN, buffer);

	    decode_category(cat[i].name, buffer);
	    pr_put_string(image, L_MARGIN+4, buffer);

	    (void)sprintf(buffer, "%d", cat[i].count);
	    mn_trapfree((void *)cat[i].name);
	    pr_put_string(image, L_MARGIN+CFGTYPE_COUNT_OFFSET, buffer);
	    pr_print_image(image);

	}

	mn_trapfree((void *)cat);
    }

    pr_end_more();

}


/****+++***********************************************************************
*
* Function:	decode_category()
*
* Parameters:   category	3 character board category
*		expanded	what it means
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	This function turns a board category into something people can
*	understand.
*
****+++***********************************************************************/

static void decode_category(category, expanded)
    char	*category;
    char	*expanded;
{

    if (strcmpi(category, "COM") == 0)
	(void)strcpy(expanded, " (Communications Board)");
    else if (strcmpi(category, "CPU") == 0)
	(void)strcpy(expanded, " (Processor Board)");
    else if (strcmpi(category, "JOY") == 0)
	(void)strcpy(expanded, " (Joystick Board)");
    else if (strcmpi(category, "KEY") == 0)
	(void)strcpy(expanded, " (Keyboard)");
    else if (strcmpi(category, "MEM") == 0)
	(void)strcpy(expanded, " (Memory Board)");
    else if (strcmpi(category, "MFC") == 0)
	(void)strcpy(expanded, " (Multi-function Board)");
    else if (strcmpi(category, "MSD") == 0)
	(void)strcpy(expanded, " (Mass Storage Device)");
    else if (strcmpi(category, "NET") == 0)
	(void)strcpy(expanded, " (Network Board)");
    else if (strcmpi(category, "NPX") == 0)
	(void)strcpy(expanded, " (Numeric Coprocessor Board)");
    else if (strcmpi(category, "PAR") == 0)
	(void)strcpy(expanded, " (Parallel Port Board)");
    else if (strcmpi(category, "PTR") == 0)
	(void)strcpy(expanded, " (Pointing Device)");
    else if (strcmpi(category, "SYS") == 0)
	(void)strcpy(expanded, " (System Board)");
    else if (strcmpi(category, "VID") == 0)
	(void)strcpy(expanded, " (Video Adapter Board)");
    else if (strcmpi(category, "OTH") == 0)
	(void)strcpy(expanded, " (Other)");
    else if (strcmpi(category, "OSE") == 0)
	(void)strcpy(expanded, " (Operating System or Environment)");
    else
	(void)strcpy(expanded, "");

}


/****+++***********************************************************************
*
* Function:     show_resources()
*
* Parameters:   space_ptr	 pointer to the space in question, may be NULL
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the resource in question.
*
****+++***********************************************************************/

static void show_resources(space_ptr)
    struct space	*space_ptr;
{
    struct resource	*rptr;
    struct board	*brd;


    /******************
    * First, make sure this is something on the list.
    ******************/
    if (space_ptr == NULL) {
	(void)fprintf(stderr, "None of this resource in use.\n\n");
	return;
    }

    /******************
    * Walk through the space entries one by one and display what they are.
    ******************/
    while (space_ptr != NULL) {

	rptr = space_ptr->resource;

	switch (rptr->type) {
	    case rt_irq:
		show_irq((struct irq *)rptr, space_ptr->sp_min);
		break;
	    case rt_dma:
		show_dma((struct dma *)rptr, space_ptr->sp_min);
		break;
	    case rt_port:
		show_port((struct port *)rptr, space_ptr->sp_min, space_ptr->sp_max);
		break;
	    case rt_memory:
		show_memory((struct memory *)rptr, space_ptr->sp_min, space_ptr->sp_max);
		break;
	}

	brd = rptr->parent->parent->parent->parent->parent->parent->parent;
	(void)fprintf(stderr, "      slot %d\n", brd->eisa_slot);

	space_ptr = space_ptr->next;

    }
}


/****+++***********************************************************************
*
* Function:     show_irq()
*
* Parameters:   irq_ptr	 	pointer to an irq resource
*		min		irq number
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the irq resource in question.
*
****+++***********************************************************************/
static void show_irq(irq_ptr, min)
    struct irq		*irq_ptr;
    unsigned long	min;
{


    (void)fprintf(stderr, "  IRQ %d", min);

    if (irq_ptr->trigger == it_edge)
	(void)fprintf(stderr, "     edge");
    else
	(void)fprintf(stderr, "    level");

    if (irq_ptr->r.share)
	(void)fprintf(stderr, "      can share");
    else
	(void)fprintf(stderr, "    can't share");

}


/****+++***********************************************************************
*
* Function:     show_dma()
*
* Parameters:   dma_ptr	 	pointer to the resource in question
*		min		dma number
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the dma resource in question.
*
****+++***********************************************************************/
static void show_dma(dma_ptr, min)
    struct dma		*dma_ptr;
    unsigned long	min;
{


    (void)fprintf(stderr, "  DMA %d", min);

    switch (dma_ptr->data_size) {
	case ds_byte:
	    (void)fprintf(stderr, "     byte size");
	    break;
	case ds_word:
	    (void)fprintf(stderr, "     word size");
	    break;
	case ds_dword:
	    (void)fprintf(stderr, "    dword size");
	    break;
    }

    switch (dma_ptr->timing) {
	case dt_default:
	    (void)fprintf(stderr, "     default timing");
	    break;
	case dt_typea:
	    (void)fprintf(stderr, "       typea timing");
	    break;
	case dt_typeb:
	    (void)fprintf(stderr, "       typeb timing");
	    break;
	case dt_typec:
	    (void)fprintf(stderr, "       typec timing");
	    break;
    }

    if (dma_ptr->r.share)
	(void)fprintf(stderr, "      can share");
    else
	(void)fprintf(stderr, "    can't share");

}


/****+++***********************************************************************
*
* Function:     show_port()
*
* Parameters:   port_ptr	 pointer to the port resource 
*		min		 first address
*		max		 last address
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the port in question.
*
****+++***********************************************************************/
static void show_port(port_ptr, min, max)
    struct port		*port_ptr;
    unsigned long	min;
    unsigned long	max;
{

    (void)fprintf(stderr, "  PORT %x-%x", min, max);

    switch (port_ptr->data_size) {
	case ds_byte:
	    (void)fprintf(stderr, "     byte size");
	    break;
	case ds_word:
	    (void)fprintf(stderr, "     word size");
	    break;
	case ds_dword:
	    (void)fprintf(stderr, "    dword size");
	    break;
    }

    if (port_ptr->r.share)
	(void)fprintf(stderr, "      can share");
    else
	(void)fprintf(stderr, "    can't share");

}


/****+++***********************************************************************
*
* Function:     show_memory()
*
* Parameters:   memory_ptr	 pointer to the memory resource
*		min		 first address
*		max		 last address
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Display the resource in question.
*
****+++***********************************************************************/
static void show_memory(memory_ptr, min, max)
    struct memory	*memory_ptr;
    unsigned long	min;
    unsigned long	max;
{

    (void)fprintf(stderr, "  MEMORY %x-%x", min, max);

    switch (memory_ptr->data_size) {
	case ds_byte:
	    (void)fprintf(stderr, "     byte size");
	    break;
	case ds_word:
	    (void)fprintf(stderr, "     word size");
	    break;
	case ds_dword:
	    (void)fprintf(stderr, "    dword size");
	    break;
    }

    if (memory_ptr->r.share)
	(void)fprintf(stderr, "      can share");
    else
	(void)fprintf(stderr, "    can't share");

}
