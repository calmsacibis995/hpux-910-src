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
*                         src/help.c
*
*   This file contains the functions to handle the interactive help
*   commands.
*
*	comment()	-- main.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "err.h"
#include "pr.h"
#include "sw.h"
#include "compat.h"


/***********
* Functions declared in this file
***********/
void			comment();
static void		comment_board();
static void		comment_switch();
static void		comment_function();
static void		comment_choice();
static void 		comment_board_header();
static void		comment_one_something();


/***********
* Functions used here but declared elsewhere
***********/
extern struct board 	*get_this_board();
extern struct system 	*cfg_load_temp();
extern void		del_release_system();


/*************
* Strings used in this file only
*************/
#define  HELP_HDR		"Help:"
#define  COMMENTS_HDR		"Comments:"
#define  NO_HELP_OR_COMMENTS    "No help or comments were supplied."
#define  NO_HELP                "No help was supplied."
#define  NO_COMMENTS    	"No comments were supplied."
#define  NO_SWITCHES_OR_JUMPERS "This board does not have switches or jumpers."
#define  SWITCH_HDR		"Switch: "
#define  JUMPER_HDR		"Jumper: "
#define  FUNCTION_HDR		"Function: "
#define  SUBFUNC_HDR		"Subfunction: "
#define  GROUP_HDR		"Group: "
#define  CHOICE_HDR		"Choice: "


/*************
* Type of structure being passed to comment_one_something()
*************/
#define  HELP_BOARD_STRUCT		1
#define  HELP_SWITCH_STRUCT		2
#define  HELP_JUMPER_STRUCT		3
#define  HELP_FUNCTION_STRUCT		4
#define  HELP_SUBFUNCTION_STRUCT	5
#define  HELP_CHOICE_STRUCT		6


/************
* What fields should be displayed in comment_one_something()
************/
#define  HELP_SHOW_HELP		0x01
#define  HELP_SHOW_COMMENTS	0x02


/************
* Globals declared elsewhere
************/
extern struct system	glb_system;


/****+++***********************************************************************
*
* Function:	comment()
*
* Parameters:   num_parms		number of parms user specified
*		type			what kind of help:
*					    board
*					    function
*					    choice
*					    switch
*		cfgfile			file to get info from
*	 	slotnum			slot number of board to get info for
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays the cfg help and comments for a given level:
*		comment board [ cfgfile | slotnum ]
*		comment function [ cfgfile | slotnum ]
*		comment choice [ cfgfile | slotnum ]
*		comment switch [ cfgfile | slotnum ]
*
****+++***********************************************************************/

void comment(num_parms, type, cfgfile, slotnum)
    int			num_parms;
    char		*type;
    char		*cfgfile;
    int			slotnum;

{
    struct board 	*brd;
    struct system	*sys;
    int			htype;


    /*************
    * Handle help board case.
    *************/
    if ( (strcmpi(type, CMD_KEY_BOARD) == 0)   ||
         (strcmpi(type, CMD_KEY_BOARDS) == 0) )
	htype = HELP_BOARD_STRUCT;

    /*************
    * Handle help function case.
    *************/
    else if ( (strcmpi(type, CMD_KEY_FUNCTION) == 0)   ||
              (strcmpi(type, CMD_KEY_FUNCTIONS) == 0) )
	htype = HELP_FUNCTION_STRUCT;

    /*************
    * Handle help choice case.
    *************/
    else if ( (strcmpi(type, CMD_KEY_CHOICE) == 0)   ||
              (strcmpi(type, CMD_KEY_CHOICES) == 0) )
	htype = HELP_CHOICE_STRUCT;

    /*************
    * Handle help switch case.
    *************/
    else if ( (strcmpi(type, CMD_KEY_SWITCH) == 0)     ||
              (strcmpi(type, CMD_KEY_SWITCHES1) == 0)  ||
              (strcmpi(type, CMD_KEY_SWITCHES2) == 0) )
	htype = HELP_SWITCH_STRUCT;

    /************
    * The user has specified an invalid help sub-command. Tell them
    * about it.
    ************/
    else {
	err_handler(INVALID_COMMENT_CMD_ERRCODE);
	return;
    }

    /************
    * Handle the case where they want help for all boards in the system.
    ************/
    if (num_parms == 1) {
	brd = glb_system.boards;
	pr_init_more();
	while (brd != NULL) {
	    switch (htype)  {
		case HELP_BOARD_STRUCT:
		    comment_board(brd, 1);
		    break;
		case HELP_SWITCH_STRUCT:
		    comment_switch(brd, 1);
		    break;
		case HELP_FUNCTION_STRUCT:
		    comment_function(brd, 1);
		    break;
		case HELP_CHOICE_STRUCT:
		    comment_choice(brd, 1);
		    break;
	    }
	    brd = brd->next;
	}
	pr_end_more();
    }

    /***********
    * Handle the case where they want help for one board currently in the
    * system.
    ***********/
    else if (slotnum != NOT_AN_INT) {
	brd = get_this_board(slotnum);
	if (brd == NULL) {
	    /* error already displayed */
	    return;
	}
	pr_init_more();
	switch (htype)  {
	    case HELP_BOARD_STRUCT:
		comment_board(brd, 1);
		break;
	    case HELP_SWITCH_STRUCT:
		comment_switch(brd, 1);
		break;
	    case HELP_FUNCTION_STRUCT:
		comment_function(brd, 1);
		break;
	    case HELP_CHOICE_STRUCT:
		comment_choice(brd, 1);
		break;
	}
	pr_end_more();
    }

    /***********
    * Handle the case where they want help for one board currently not in the
    * system.
    ***********/
    else {
	sys = cfg_load_temp(cfgfile);
	if (sys == NULL)
	    return;
	brd = sys->boards;
	pr_init_more();
	switch (htype)  {
	    case HELP_BOARD_STRUCT:
		comment_board(brd, 0);
		break;
	    case HELP_SWITCH_STRUCT:
		comment_switch(brd, 0);
		break;
	    case HELP_FUNCTION_STRUCT:
		comment_function(brd, 0);
		break;
	    case HELP_CHOICE_STRUCT:
		comment_choice(brd, 0);
		break;
	}
	pr_end_more();
	del_release_system(sys);
    }

}


/****+++***********************************************************************
*
* Function:	comment_board()
*
* Parameters:   brd			board to display help for
*		in_config		0 -- board not currently in system
*					1 -- board is in configuration
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays the help and comments information for a
*	single board. If no help or comments was provided, a message will
*	be displayed.
*
*       The format looks like this:
*
*      |-------------------------------------------------------------------|
*      |                                                                   |
*      | <board name>                                                      |
*      | <slot label>  (if in the configuration)                           |
*      |                                                                   |
*      |   Comments:                                                       |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |                                                                   |
*      |   Help:                                                           |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |                                                                   |
*      |-------------------------------------------------------------------|
*
****+++***********************************************************************/

static void comment_board(brd, in_config)
    struct board 	*brd;
    int			in_config;
{
    unsigned		indent;


    /************
    * Set up the starting indentation.
    ************/
    indent = L_MARGIN;

    /***********
    * Display the board header.
    ***********/
    comment_board_header(brd, in_config, indent);

    /***********
    * Display the comments and help (if any).
    ***********/
    comment_one_something((void *)brd, HELP_BOARD_STRUCT,
                       HELP_SHOW_HELP | HELP_SHOW_COMMENTS, indent+2);

    /***********
    * Add a blank line to the end.
    ***********/
    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:	comment_switch()
*
* Parameters:   brd			board to display help for
*		in_config		0 -- board not currently in system
*					1 -- board is in configuration
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays the comments information for all
*	switches and jumpers on a single board.
*
****+++***********************************************************************/

static void comment_switch(brd, in_config)
    struct board 	*brd;
    int			in_config;
{
    unsigned		indent;
    int			displayed=0;
    struct bswitch	*sw;
    struct jumper	*jump;
    image_buf		image;
    char		header[80];


    /************
    * Set up the starting indentation.
    ************/
    indent = L_MARGIN;

    /***********
    * Display the board header.
    ***********/
    comment_board_header(brd, in_config, indent);

    /***********
    * Display the comments and help for all switches (if any).
    ***********/
    sw = brd->switches;
    while (sw)  {
	displayed = 1;
	pr_clear_image(image);
	(void)strcpy(header, SWITCH_HDR);
	(void)strcat(header, sw->name);
	pr_put_string(image, indent+2, header);
	pr_print_image(image);
	comment_one_something((void *)sw, HELP_SWITCH_STRUCT,
			   HELP_SHOW_COMMENTS, indent+4);
	sw = sw->next;
	pr_print_image(" ");
    }

    /***********
    * Display the comments and help for all jumpers (if any).
    ***********/
    jump = brd->jumpers;
    while (jump)  {
	displayed = 1;
	pr_clear_image(image);
	(void)strcpy(header, JUMPER_HDR);
	(void)strcat(header, jump->name);
	pr_put_string(image, indent+2, header);
	pr_print_image(image);
	comment_one_something((void *)jump, HELP_JUMPER_STRUCT,
			   HELP_SHOW_COMMENTS, indent+4);
	jump = jump->next;
	pr_print_image(" ");
    }

    /************
    * If nothing was displayed, display a message telling the user.
    ************/
    if (displayed == 0) {
	pr_clear_image(image);
	pr_put_string(image, indent+2, NO_SWITCHES_OR_JUMPERS);
	pr_print_image(image);
    }

    /***********
    * Add a blank line to the end.
    ***********/
    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:	comment_function()
*
* Parameters:   brd			board to display help for
*		in_config		0 -- board not currently in system
*					1 -- board is in configuration
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays the help and comments information for all
*	functions on a single board.
*
****+++***********************************************************************/

static void comment_function(brd, in_config)
    struct board 	*brd;
    int			in_config;
{
    unsigned		indent;
    image_buf		image;
    char		header[80];
    struct function	*fun;
    struct subfunction	*subf;
    struct group	*grp;


    /************
    * Set up the starting indentation.
    ************/
    indent = L_MARGIN;

    /***********
    * Display the board header.
    ***********/
    comment_board_header(brd, in_config, indent);

    /***********
    * Display help and comments for each function and subfunction. 
    ***********/
    indent += 2;
    for (grp=brd->groups ; grp!=NULL ; grp=grp->next) {

	/**************
	* Print the group name if it was explicit.
	**************/
	if (grp->explicit) {
	    pr_clear_image(image);
	    (void)strcpy(header, GROUP_HDR);
	    (void)strcat(header, grp->name);
	    pr_put_string(image, indent, header);
	    pr_print_image(image);
	    pr_print_image(" ");
	    indent += 2;
	}

	/**************
	* Handle each one of the functions in this group.
	**************/
	for (fun=grp->functions ; fun!=NULL ; fun=fun->next) {

	    if (!fun->display)
		continue;

	    pr_clear_image(image);
	    (void)strcpy(header, FUNCTION_HDR);
	    (void)strcat(header, fun->name);
	    pr_put_string(image, indent, header);
	    pr_print_image(image);
	    comment_one_something((void *)fun, HELP_FUNCTION_STRUCT,
			       HELP_SHOW_HELP | HELP_SHOW_COMMENTS, indent+2);

	    if (fun->subfunctions->explicit) {
		indent += 2;
		for (subf=fun->subfunctions ; subf!=NULL ; subf=subf->next) {
		    pr_print_image(" ");
		    pr_clear_image(image);
		    (void)strcpy(header, SUBFUNC_HDR);
		    (void)strcat(header, subf->name);
		    pr_put_string(image, indent, header);
		    pr_print_image(image);
		    comment_one_something((void *)subf, HELP_SUBFUNCTION_STRUCT,
			       HELP_SHOW_HELP | HELP_SHOW_COMMENTS, indent+2);
		}
		indent -= 2;
	    }

	}

	/*************
	* Move the indentation back since we did an explicit group.
	*************/
	if (grp->explicit)
	    indent -= 2;

    }

    /***********
    * Add a blank line to the end.
    ***********/
    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:	comment_choice()
*
* Parameters:   brd			board to display help for
*		in_config		0 -- board not currently in system
*					1 -- board is in configuration
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays the help and comments information for all
*	choices on a single board.
*
****+++***********************************************************************/

static void comment_choice(brd, in_config)
    struct board 	*brd;
    int			in_config;
{
    unsigned		indent;
    image_buf		image;
    char		header[80];
    struct function	*fun;
    struct subfunction	*subf;
    struct group	*grp;
    struct choice	*choi;


    /************
    * Set up the starting indentation.
    ************/
    indent = L_MARGIN;

    /***********
    * Display the board header.
    ***********/
    comment_board_header(brd, in_config, indent);

    /***********
    * Display help and comments for each choice.
    ***********/
    indent += 2;
    for (grp=brd->groups ; grp!=NULL ; grp=grp->next) {

	/**************
	* Print the group name if it was explicit.
	**************/
	if (grp->explicit) {
	    pr_clear_image(image);
	    (void)strcpy(header, GROUP_HDR);
	    (void)strcat(header, grp->name);
	    pr_put_string(image, indent, header);
	    pr_print_image(image);
	    indent += 2;
	}

	/**************
	* Handle each one of the functions in this group.
	**************/
	for (fun=grp->functions ; fun!=NULL ; fun=fun->next) {

	    /*************
	    * If this function is not supposed to be displayed, move on.
	    *************/
	    if (!fun->display)
		continue;

	    /*************
	    * Display the function name.
	    *************/
	    pr_print_image(" ");
	    pr_clear_image(image);
	    (void)strcpy(header, FUNCTION_HDR);
	    (void)strcat(header, fun->name);
	    pr_put_string(image, indent, header);
	    pr_print_image(image);

	    /*************
	    * Process each of the subfunctions.
	    *************/
	    for (subf=fun->subfunctions ; subf!=NULL ; subf=subf->next) {

		/*************
		* If the subfunctions were explicit, display their names too.
		*************/
		if (fun->subfunctions->explicit) {
		    indent += 2;
		    pr_print_image(" ");
		    pr_clear_image(image);
		    (void)strcpy(header, SUBFUNC_HDR);
		    (void)strcat(header, subf->name);
		    pr_put_string(image, indent, header);
		    pr_print_image(image);
		}

		/************
		* Now display each choice, with it's help information.
		************/
		indent += 2;
		for (choi=subf->choices ; choi!=NULL ; choi=choi->next) {
		    pr_print_image(" ");
		    pr_clear_image(image);
		    (void)strcpy(header, CHOICE_HDR);
		    (void)strcat(header, choi->name);
		    pr_put_string(image, indent, header);
		    pr_print_image(image);
		    comment_one_something((void *)choi, HELP_CHOICE_STRUCT,
				       HELP_SHOW_HELP, indent+2);
		}
		indent -= 2;

		/*************
		* If the subfunctions were explicit, reset the ident.
		*************/
		if (fun->subfunctions->explicit)
		    indent -= 2;

	    }

	}

	/*************
	* Move the indentation back since we did an explicit group.
	*************/
	if (grp->explicit)
	    indent -= 2;

    }

    /***********
    * Add a blank line to the end.
    ***********/
    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:	comment_board_header()
*
* Parameters:   brd			board to display hdr for
*		in_config		0 -- board not currently in system
*					1 -- board is in configuration
*		indent			where to start the header
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays a header with the board name and slot number.
*
*       The format looks like this:
*
*      |-------------------------------------------------------------------|
*      |                                                                   |
*      | <board name>                                                      |
*      | <slot label>  (if in the configuration)                           |
*      |                                                                   |
*      |-------------------------------------------------------------------|
*
****+++***********************************************************************/

static void comment_board_header(brd, in_config, indent)
    struct board 	*brd;
    int			in_config;
    unsigned		indent;
{
    image_buf 		image;


    /***********
    * Display a blank line and the board name.
    ***********/
    pr_print_image(" ");
    pr_clear_image(image);
    pr_put_string(image, indent, brd->name);
    pr_print_image(image);

    /************
    * Display the slot number (or label) if the board is currently part of the
    * configuration.
    ************/
    if (in_config) {
	pr_clear_image(image);
#ifdef VIRT_BOARD
	if (brd->slot_number >= MAX_NUM_SLOTS * 2 - 1)
	    pr_put_string(image, indent, SW_VIRTUAL);
	else if (brd->slot_number >= MAX_NUM_SLOTS-1)
	    pr_put_string(image, indent, SW_EMBEDDED);
	else
#endif
	    pr_put_string(image,indent,glb_system.slot[brd->slot_number].label);
	pr_print_image(image);
    }

    /*************
    * Display a blank line.
    *************/
    pr_print_image(" ");

}


/****+++***********************************************************************
*
* Function:	comment_one_something()
*
* Parameters:   ptr			ptr to some kind of struct:
*					    struct board *
*					    struct bswitch *
*					    struct jumper *
*					    struct function *
*					    struct choice *
*		type			what type of struct is this?
*					    HELP_BOARD_STRUCT
*					    HELP_SWITCH_STRUCT
*					    HELP_JUMPER_STRUCT
*					    HELP_FUNCTION_STRUCT
*					    HELP_SUBFUNCTION_STRUCT
*					    HELP_CHOICE_STRUCT
*		helpcomm		which fields should be displayed?
*					    HELP_SHOW_HELP
*					    HELP_SHOW_COMMENTS
*		indent			where to start the header
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function displays the help and comments information for a
*	given structure. If no help or comments was provided, a message will
*	be displayed.
*
*       The format looks like this:
*
*      |-------------------------------------------------------------------|
*      |                                                                   |
*      |   Comments:                                                       |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |                                                                   |
*      |   Help:                                                           |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    |
*      |                                                                   |
*      |-------------------------------------------------------------------|
*
****+++***********************************************************************/

static void comment_one_something(ptr, type, helpcomm, indent)
    void		*ptr;
    int			type;
    int			helpcomm;
    unsigned		indent;
{
    struct board 	*brd;
    struct bswitch	*swi;
    struct jumper	*jump;
    struct function	*fun;
    struct subfunction	*sub;
    struct choice	*choi;
    int			displayed = 0;
    char		*comments;
    char		*help;
    image_buf 		image;


    /*************
    * Get the comments and help from whatever struct was passed in.
    *************/
    switch (type) {
	case HELP_BOARD_STRUCT:
	    brd = (struct board *)ptr;
	    comments = brd->comments;
	    help = brd->help;
	    break;
	case HELP_SWITCH_STRUCT:
	    swi = (struct bswitch *)ptr;
	    comments = swi->comments;
	    break;
	case HELP_JUMPER_STRUCT:
	    jump = (struct jumper *)ptr;
	    comments = jump->comments;
	    break;
	case HELP_FUNCTION_STRUCT:
	    fun = (struct function *)ptr;
	    comments = fun->comments;
	    help = fun->help;
	    break;
	case HELP_SUBFUNCTION_STRUCT:
	    sub = (struct subfunction *)ptr;
	    comments = sub->comments;
	    help = sub->help;
	    break;
	case HELP_CHOICE_STRUCT:
	    choi = (struct choice *)ptr;
	    help = choi->help;
	    break;
    }

    /**************
    * Print a blank line.
    **************/
    pr_print_image(" ");

    /*************
    * If comments were requested and there are comments, print a header line
    * and the comments that were supplied.
    *************/
    if ( (helpcomm & HELP_SHOW_COMMENTS)  &&  (strlen(comments) != 0) )  {
	displayed = 1;
	pr_clear_image(image);
	pr_put_string(image, indent, COMMENTS_HDR);
	pr_print_image(image);
	indent += 2;
        pr_put_text_fancy((int)indent, comments, 100, (int)(80-indent-R_MARGIN), 0);
	pr_print_image(" ");
	indent -= 2;
    }

    /*************
    * If help was requested and there is help, print a header line
    * and the help that was supplied.
    *************/
    if ( (helpcomm & HELP_SHOW_HELP)  &&  (strlen(help) != 0) )  {
	displayed = 1;
	pr_clear_image(image);
	pr_put_string(image, indent, HELP_HDR);
	pr_print_image(image);
	indent += 2;
        pr_put_text_fancy((int)indent, help, 100, (int)(80-indent-R_MARGIN), 0);
	pr_print_image(" ");
	indent -= 2;
    }

    /************
    * If nothing was displayed, display a message telling the user.
    ************/
    if (displayed == 0) {
	pr_clear_image(image);
	if ( (helpcomm & HELP_SHOW_HELP) && (helpcomm & HELP_SHOW_COMMENTS) )
	    pr_put_string(image, indent, NO_HELP_OR_COMMENTS);
	else if (helpcomm & HELP_SHOW_HELP)
	    pr_put_string(image, indent, NO_HELP);
	else if (helpcomm & HELP_SHOW_COMMENTS)
	    pr_put_string(image, indent, NO_COMMENTS);
	pr_print_image(image);
	pr_print_image(" ");
    }

}
