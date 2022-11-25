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
*                         src/main.c
*
*   This file contains the main function for the eisa_config program.
*   needed functions.
*
*	main()		-- main for eisa_config
*	exiter()	-- init.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/param.h>
#include "config.h"
#include "err.h"
#include "cf_util.h"
#include "add.h"
#include "sw.h"
#include "pr.h"
#include "compat.h"


extern struct pmode	program_mode;
extern struct system	glb_system;
extern char		sci_name[];
extern char		iodc_name[];
extern int		changed_since_save;
extern int		changed_since_start;
extern char		more_file_name[];
extern int       	auto_mode_messages;
extern FILE		*print_err_file_fd;


static char		cfg_filename[MAXPATHLEN];
static FILE		*cfgfile;


#define  MN_MANUAL_VERIFY 	0
#define  MN_AUTO_VERIFY   	1

#define  MN_LOCK     		1
#define  MN_RESET    		2
#define  MN_RESTORE  		3
#define  MN_UNLOCK   		4


extern int  		open_sci();
extern  int		add_to_slot();
extern  int  		cf_configure();
extern  void            *mn_trapcalloc();
extern  void            mn_trapfree();
extern  FILE		*open_cfg_file();
extern  void		del_release_board();
extern void		show();
extern void		show_cfg();
extern void		comment();
extern struct board	*get_this_board();
extern void		dump_iodc_slot();
extern int		init();
extern int		last_physical_slot();
extern void		sci_init_cfg_file();
extern void		sci_remove_cfg_file();
extern void		sci_move_cfg_file();
extern void		init_auto();
#ifdef DEBUG
extern  void		display_system();
#endif


/* Define internal functions */
static void		mn_interactive_cf();
static void		mn_automatic_cf();
static int 		mn_parse_args();
static int 		mn_ntarget();
static int 		mn_checkcfg();
static void		mn_add_board();
static void		mn_save_config();
static void		mn_init_config();
static void		mn_exit_config();
static void		mn_remove_board();
static void		mn_move_board();
void			exiter();
int			mn_read_char();
static void		mn_change_function();


/* interactive commands -- top-level */
#define CMD_ADD_BOARD		"add"
#define CMD_REMOVE_BOARD	"remove"
#define CMD_MOVE_BOARD		"move"
#define CMD_CHANGE_FUNCTION	"change"
#define CMD_SAVE		"save"
#define CMD_INIT		"init"
#define CMD_EXIT1		"exit"
#define CMD_EXIT2		"quit"
#define CMD_EXIT3		"bye"
#define CMD_EXIT4		"q"
#define CMD_SHOW		"show"
#define CMD_CFG_TYPES		"cfgtypes"
#define CMD_CFG_TYPE		"cfgtype"
#define CMD_CFG_FILES		"cfgfiles"
#define CMD_CFG_FILE		"cfgfile"
#define CMD_HELP1		"help"
#define CMD_HELP2		"?"
#define CMD_COMMENT1		"comment"
#define CMD_COMMENT2		"comments"


/* debugging global */
#ifdef DEBUG
static int		display = 0;
#endif


/****+++***********************************************************************
*
* Function:     main()
*
* Parameters:   argc
*		argv		as defined in mn_parse_args()
*
* Used:		external only
*
* Exit Values:  EXIT_OK_NO_REBOOT	successful (no changes)
*               EXIT_OK_REBOOT		successful (changes made)
*		EXIT_CHECKCFG_PROBLEM	the cfg file check failed
*		EXIT_RUNSTRING_ERR	bad runstring parameters
*		EXIT_INIT_ERR		the configuration initialization failed
*
* Description:
*
*    Main for eisa_config.
*
****+++***********************************************************************/

main(argc, argv)
    int 		argc;
    char		*argv[];
{
    int			interactive;
    int			err;



    /***************
    * Initialize the program mode flags.
    ***************/
    program_mode.checkcfg = 0;
    program_mode.non_target = 0;
    program_mode.nvm_present = 0;
    program_mode.id = 0;
#ifdef MANUAL_VERIFY
    program_mode.auto_verify = MN_AUTO_VERIFY;
#endif
    program_mode.iodc_slot = 0;
    program_mode.slotnum = 0;
    program_mode.automatic = 0;
    program_mode.writefast = 0;

    /***************
    * Parse the command line -- mn_parse_args() handles any errors.
    ***************/
    if (mn_parse_args(argc, argv, &interactive) != 0)
	exiter(EXIT_RUNSTRING_ERR);

    /***************
    * If this is checkcfg mode, handle it right here.
    ***************/
    if (program_mode.checkcfg) {
	if (mn_checkcfg() == 0)
	    exiter(EXIT_OK_NO_REBOOT);
	exiter(EXIT_CHECKCFG_PROBLEM);
    }

    /***************
    * Display a header containing the utility name, copyright, & version number.
    ***************/
    if (interactive)
	err_handler(INTERACTIVE_HEADER1_ERRCODE);

    /***************
    * Set up the temporary file needed for printing with more.
    * Also set up the current set of cfg files used to null (for the sci
    * save to come).
    ***************/
    sci_init_cfg_file();
    (void)strcpy(more_file_name, tempnam(CFG_FILES_DIR, "ECONF"));

    /***************
    * Set up the system. This means getting the configuration from either
    * an sci file (non-target mode) or eeprom or the hardware itself (normal
    * mode). At the end of this call, the global data structures should be set
    * up with the configuration that was saved the last time eisa_config ran.
    * We exit if we get an error (the error messages have already been
    * displayed).
    *
    * Note that we never return from init_auto().
    ***************/
    if (program_mode.scratch)
	err = 0;
    else if (program_mode.non_target)
	err = mn_ntarget();
    else if (program_mode.automatic) {
#ifdef LINT
	err = 0;
#endif
	init_auto();
    }
    else
	err = init();
    if (err != 0) {
	if (program_mode.slotnum)
	    (void)printf("0\n");
	exiter(EXIT_INIT_ERR);
    }

    /***************
    * Display a header containing basic command usage.
    ***************/
    if (interactive)
	err_handler(INTERACTIVE_HEADER2_ERRCODE);

    /***************
    * Get the commands or process the runstring options.
    ***************/
    if (interactive)
	mn_interactive_cf();
    else
	mn_automatic_cf();

    exiter(EXIT_OK_NO_REBOOT);

#ifdef LINT
    return(0);
#endif
}


/****+++***********************************************************************
*
* Function:     mn_parse_args()
*
* Parameters:   argc  			number of arguments
*		argv  			arguments
*		interactive		set on return if mode is interactive
*
* Used:		internal only
*
* Returns:      0			ok
*		-1			error (error handling done here)
*
* Description:
*
*       This routine parses the command line arguments and sets flags for each 
*       command.
*
****+++***********************************************************************/

static int mn_parse_args(argc, argv, interactive)
    int 	argc;
    char	*argv[];
    int		*interactive;
{
    int		c;
    int		argct = 0;
    char	*error;
    extern char *optarg;
#ifndef LINT
    extern int  optind, opterr;
#endif


    *interactive = 1;
    *iodc_name = 0;
#ifndef LINT
    opterr = 0;
#endif

    /*************
    * Walk through the arguments saving stuff away until we run out of
    * arguments or we hit an invalid one. The small letters are ones that
    * real customers will use. The large letters are SAM or other HP internal
    * options.
    *************/
    while ( (c = getopt(argc, argv, "n:c:S:I:asdWYZ")) != EOF) {

	switch (c)  {

	    /* automatic mode */
	    case 'a':
		program_mode.automatic = 1;
		*interactive = 0;
		argct++;
		break;

	    /* check cfg file mode */
	    case 'c':
		program_mode.checkcfg = 1;
		(void)strcpy(cfg_filename, optarg);
		*interactive = 0;
		argct++;
		break;

#ifdef DEBUG
	    /* display mode -- debug only */
	    case 'd':
		display = 1;
		break;
#endif

	    /* run in non-target mode */
	    case 'n':
		program_mode.non_target = 1;
		(void)strcpy(sci_name, optarg);
		argct++;
		break;

	    /* start from scratch mode */
	    case 's':
		program_mode.scratch = 1;
		program_mode.non_target = 1;
		argct++;
		break;

	    /* specify a file to dump IODC data to */
	    case 'I':
		(void)strcpy(iodc_name, optarg);
		argct++;
		break;

	    /* specify a slot number */
	    case 'S':
		program_mode.iodc_slot = (int)strtol(optarg, &error, 0);
		if (*error != NULL) {
		    err_handler(IODC_BAD_OPTION_ERRCODE);
		    return(-1);
		}
		*interactive = 0;
		argct++;
		break;

	    /* set up to write NVM directly fron sci file */
	    case 'W':
		program_mode.writefast = 1;
		*interactive = 0;
		argct++;
		break;

	    /* give the highest slot number on this system */
	    case 'Y':
		program_mode.slotnum = 1;
		*interactive = 0;
		argct++;
		break;

	    /* give a basic identification of what is configured */
	    case 'Z':
		program_mode.id = 1;
		*interactive = 0;
		argct++;
		break;

	    /* invalid command */
	    default:
		err_handler(RUNSTRING_INV_CMD_ERRCODE);
		return(-1);

	}

    }

    /*************
    * Handle conflicts here.
    *************/
    if ( (program_mode.checkcfg) && (argct != 1) ) {
	err_handler(NO_OTHER_OPTIONS_WITH_CHECK_ERRCODE);
	err_handler(RUNSTRING_INV_CMD_ERRCODE);
	return(-1);
    }
    if ( (program_mode.automatic) && (argct != 1) ) {
	err_handler(NO_OTHER_OPTIONS_WITH_AUTO_ERRCODE);
	err_handler(RUNSTRING_INV_CMD_ERRCODE);
	return(-1);
    }

    /*************
    * If this is an IODC setup, make sure they have specified:
    *    o   -n sci_file
    *    o   -S slot_number
    *    o   -I output_file
    * If either -I or -S is there, but one of the others is not,
    * this is an error.
    *************/
    if ( (program_mode.iodc_slot != 0) || (*iodc_name != 0) )  {
	if ( (program_mode.iodc_slot == 0) ||
	     (*iodc_name == 0)             ||
	     (program_mode.non_target == 0) ) {
	    err_handler(IODC_BAD_OPTION_ERRCODE);
	    return(-1);
	}
    }

    return(0);

}


/****+++***********************************************************************
*
* Function:     mn_interactive_cf()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function is called to run an interactive session. It prompts the
*    user for a command, parses the command, and invokes the correct function
*    to perform the requested action. It will continue to go through this 
*    cycle until the user explicitly exits.
*
****+++***********************************************************************/

static void mn_interactive_cf()
{
    char	buffer[256];
    char    	*command;
    char    	*str_parm[5+1];
    int		int_parm[5+1];
    int		num_parms;
    char	*error;
    int		i;
    int		c;


    /*************
    * Show what the configuration looks like (show slots).
    *************/
    show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);

    /*************
    * Get commands until the user says quit.
    *************/
    while (1)   {

	/***************
	* Display the prompt and wait for a response. Stuff all characters
	* of the response into buffer. Stop on a carriage return. If we see 
	* a Ctrl-d, do an exit.
	***************/
	err_handler(INTERACTIVE_PROMPT_ERRCODE);
	i = 0;
	while ((c = getc(stdin)) != '\n') {
	    if (c == -1)  {
		mn_exit_config();
		buffer[0] = 0;
		break;
	    }
	    buffer[i++] = c;
	}
	buffer[i] = 0;

	/****************
	* Grab the first token from the string (the command).
	* If no command was typed, go back to the top of the loop and
	* give them a new prompt.
	****************/
	command = strtok(buffer, " ");
	if (command == NULL)
	    continue;

	/****************
	* Try to get a few more tokens as well -- some commands
	* need parameters. Go ahead and convert the parameters to ints
	* for the commands that need them that way (if the parm is not
	* a valid integer, mark it as such). Exit the loop when all of 
	* the parms have been read or the maximum number has been
	* exceeded. Print a blank line.
	****************/
	num_parms = 0;
	while ((str_parm[num_parms] = strtok((char *)NULL, " ")) != NULL) {
	    int_parm[num_parms] = (int)strtol(str_parm[num_parms], &error, 0);
	    if (*error != NULL)
		int_parm[num_parms] = NOT_AN_INT;
	    num_parms++;
	    if (num_parms > 5)
		break;
	}
	err_handler(BLANK_LINE_ERRCODE);

	/****************
	* Add a board    -- add cfgfile slotnum
	****************/
	if (strcmp(command, CMD_ADD_BOARD) == 0)  {
	    if ( (num_parms == 2) && (int_parm[1] != NOT_AN_INT) )
		mn_add_board(str_parm[0], int_parm[1]);
	    else
		err_handler(INVALID_ADD_ERRCODE);
	}

	/****************
	* Remove a board   -- remove slotnum
	****************/
	else if (strcmp(command, CMD_REMOVE_BOARD) == 0) {
	    if ( (num_parms == 1) && (int_parm[0] != NOT_AN_INT) )
		mn_remove_board(int_parm[0]);
	    else
		err_handler(INVALID_REMOVE_ERRCODE);
	}

	/****************
	* Move a board   -- move curslotnum newslotnum
	****************/
	else if (strcmp(command, CMD_MOVE_BOARD) == 0) {
	    if ( (num_parms == 2) && (int_parm[0] != NOT_AN_INT) &&
	                             (int_parm[1] != NOT_AN_INT) )
		mn_move_board(int_parm[0], int_parm[1]);
	    else
		err_handler(INVALID_MOVE_ERRCODE);
	}

	/****************
	* Change a function   -- change slotnum function_number choice_number
	****************/
	else if (strcmp(command, CMD_CHANGE_FUNCTION) == 0) {
	    if ( (num_parms == 3) &&
		 (int_parm[0] != NOT_AN_INT) )
		mn_change_function(int_parm[0], str_parm[1], str_parm[2]);
	    else
		err_handler(INVALID_CHG_ERRCODE);
	}

	/****************
	* Save configuration   -- save [filename]
	****************/
	else if (strcmp(command, CMD_SAVE) == 0) {
	    if (num_parms < 2)
		mn_save_config(num_parms, str_parm[0]);
	    else
		err_handler(INVALID_SAVE_ERRCODE);
	}

	/****************
	* Initialize configuration   -- init [filename]
	****************/
	else if (strcmp(command, CMD_INIT) == 0) {
	    if (num_parms < 2)
		mn_init_config(num_parms, str_parm[0]);
	    else
		err_handler(INVALID_INIT_ERRCODE);
	}

	/****************
	* Exit configuration   -- exit (also accept "bye", "quit", and "q")
	****************/
	else if ( (strcmp(command, CMD_EXIT1) == 0) ||
	          (strcmp(command, CMD_EXIT2) == 0) ||
	          (strcmp(command, CMD_EXIT3) == 0) ||
	          (strcmp(command, CMD_EXIT4) == 0) )  {
	    if (num_parms == 0)
		mn_exit_config();
	    else
		err_handler(INVALID_EXIT_ERRCODE);
	}

	/****************
	* Show configuration   -- show -- lots of variations (handled in
	*  show())
	****************/
	else if (strcmp(command, CMD_SHOW) == 0) {
	    show(num_parms, str_parm[0], str_parm[1], str_parm[2],
	         int_parm[1], int_parm[2], 1);
	}

	/****************
	* Get CFG Types   --   cfgtypes
	****************/
	else if ( (strcmp(command, CMD_CFG_TYPES) == 0) ||
	          (strcmp(command, CMD_CFG_TYPE) == 0) )  {
	    if (num_parms == 0)
		show_cfg(CFG_TYPES, (char *)0);
	    else
		err_handler(INVALID_CFGCMD_ERRCODE);
	}

	/****************
	* Get CFG Files   --  cfgfiles [type]
	****************/
	else if ( (strcmp(command, CMD_CFG_FILES) == 0) ||
	          (strcmp(command, CMD_CFG_FILE) == 0) )  {
	    if (num_parms == 0)
		show_cfg(CFG_ALL_FILES, (char *)0);
	    else if (num_parms == 1)
		show_cfg(CFG_SOME_FILES, str_parm[0]);
	    else
		err_handler(INVALID_CFGCMD_ERRCODE);
	}

	/***************
	* Comments (from cfg file)    --  comments (lots of variations)
	***************/
	else if ( (strcmp(command, CMD_COMMENT1) == 0) ||
	          (strcmp(command, CMD_COMMENT2) == 0) )   {
	    if ( (num_parms == 0) || (num_parms > 2) )
		err_handler(INVALID_COMMENT_CMD_ERRCODE);
	    else
		comment(num_parms, str_parm[0], str_parm[1], int_parm[1]);
	}

	/***************
	* Utility Help    --  help
	***************/
	else if ( (strcmp(command, CMD_HELP1) == 0) ||
	          (strcmp(command, CMD_HELP2) == 0) )  {
	    if (num_parms > 1)
		err_handler(INVALID_HELP_CMD_ERRCODE);
	    else if (num_parms == 0)
		err_handler(INTERACTIVE_BASIC_HELP_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_ADD_BOARD) == 0)
		err_handler(HELP_ADD_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_REMOVE_BOARD) == 0)
		err_handler(HELP_REMOVE_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_MOVE_BOARD) == 0)
		err_handler(HELP_MOVE_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_CHANGE_FUNCTION) == 0)
		err_handler(HELP_CHG_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_SAVE) == 0)
		err_handler(HELP_SAVE_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_INIT) == 0)
		err_handler(HELP_INIT_ERRCODE);
	    else if ( (strcmp(str_parm[0], CMD_EXIT1) == 0) ||
	              (strcmp(str_parm[0], CMD_EXIT2) == 0) ||
	              (strcmp(str_parm[0], CMD_EXIT3) == 0) ||
	              (strcmp(str_parm[0], CMD_EXIT4) == 0) )
		err_handler(HELP_EXIT_ERRCODE);
	    else if (strcmp(str_parm[0], CMD_SHOW) == 0)
		err_handler(HELP_SHOW_ERRCODE);
	    else if ( (strcmp(str_parm[0], CMD_COMMENT1) == 0)  ||
	              (strcmp(str_parm[0], CMD_COMMENT2) == 0) )
		err_handler(HELP_COMMENT_ERRCODE);
	    else if ( (strcmp(str_parm[0], CMD_HELP1) == 0)  ||
	              (strcmp(str_parm[0], CMD_HELP2) == 0) )
		err_handler(HELP_HELP_ERRCODE);
	    else if ( (strcmp(str_parm[0], CMD_CFG_TYPES) == 0) ||
	              (strcmp(str_parm[0], CMD_CFG_TYPE) == 0)  ||
	              (strcmp(str_parm[0], CMD_CFG_FILES) == 0) ||
		      (strcmp(str_parm[0], CMD_CFG_FILE) == 0) )
		err_handler(HELP_CFG_ERRCODE);
	    else
		err_handler(INVALID_HELP_CMD_ERRCODE);
	}

	/**************
	* Invalid command
	**************/
	else
	    err_handler(INTERACTIVE_INVALID_CMD_ERRCODE);

    }
}


/****+++***********************************************************************
*
* Function:     mn_automatic_cf()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function is called to run an automatic session.
*
****+++***********************************************************************/

static void mn_automatic_cf()
{

    /*************
    * Handle the id option --show what boards are out there.
    *************/
    if (program_mode.id)
	show(0, (char *)0, (char *)0, (char *)0, 0, 0, 0);

    /*************
    * Handle the iodc dump stuff here.
    *************/
    else if (program_mode.iodc_slot != 0)
	dump_iodc_slot(program_mode.iodc_slot);

    /*************
    * Handle the max slot number case here.
    *************/
    else if (program_mode.slotnum != 0)
	(void)printf("%d\n", last_physical_slot());

}






/****+++***********************************************************************
*
* Function:     mn_ntarget()
*
* Parameters:   None
*
* Used:		internal only
*
* Returns:      0			ok
*		other			error
*
* Description:
*
*   This function is called in two different ways (only one of which is
*   supported today). Today, this function is called when a user selects
*   non-target mode and wants to start from an sci file. In the future, we
*   should also allow a user to start over; that is, pick a new system
*   to start with. This would use open_sys() instead of open_sci().
*
*   The sci file name is assumed to be contained in the global sci_name.
*
****+++***********************************************************************/

static int mn_ntarget()
{
    int 	status;


    status = open_sci();

    /****************
    * If the user wanted a fast write to NVM, do it here. Will never return.
    ****************/
    if (program_mode.writefast == 1)  {
	mn_save_config(0, (char *)NULL);
	exiter(EXIT_OK_NO_REBOOT);
    }

    return(status);
}


/****+++***********************************************************************
*
* Function:     mn_checkcfg()
*
* Parameters:   None
*
* Used:		internal only
*
* Returns:      0			ok
*		1			error
*
* Description:
*
*     This function is called when the user just wants to check a particular
*     cfg file for correctness (no configuration will be done). The raw
*     cfg file name is in the file global cfg_filename.
*
*     If the file is not a valid cfg file (bad use of grammar), appropriate
*     errors are displayed.
*
****+++***********************************************************************/

static int mn_checkcfg()
{

    struct system 	*sysptr;
    int 		result;
    char		full_filename[MAXPATHLEN];

    
    /**************
    * Open the cfg file (if possible).
    * Print the check cfg header message (contains file name).
    * If we couldn't open the file (or derivatives), print an error
    * message and exit.
    **************/
    cfgfile = open_cfg_file(cfg_filename, full_filename);
    if (cfgfile == NULL) {
	err_add_string_parm(1, cfg_filename);
	err_handler(CHECK_CFG_HDR_ERRCODE);
	err_handler(PARSE77_ERRCODE);
	return(1);
    }
    else {
	err_add_string_parm(1, full_filename);
	err_handler(CHECK_CFG_HDR_ERRCODE);
    }

    /*************
    * Call the parser.
    *************/
    sysptr = (struct system *) mn_trapcalloc(1, sizeof(struct system));
    result = parser(sysptr, full_filename, mn_read_char);

#ifdef DEBUG
    if (display)
	display_system(sysptr);
#endif

    if (result == 0)
	return(1);
    return(0);
}




/****+++***********************************************************************
*
* Function:     mn_read_char()
*
* Parameters:   None              
*
* Used:		internal and external
*
* Returns:      the next char from the cfg file
*
* Description:
*
*    Get the next char from the cfg file. This is a support function for
*    the checkcfg option.
*
****+++***********************************************************************/

int mn_read_char()
{
    int ch;

    ch = getc(cfgfile);
    return(feof(cfgfile) ? -1 : ch);
}


/****+++***********************************************************************
*
* Function:     mn_add_board()
*
* Parameters:   cfg_file		cfg file name of new board
*		slot			slot num of new board
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function is called to add a board to the configuration.
*    The user has supplied both the cfg file name and the slot number.
*
*    This function is responsible for verifying the parameters
*    and adding the new board to the configuration.
*
****+++***********************************************************************/

static void mn_add_board(cfg_file, slot)
    char		*cfg_file;
    int			slot;
{
    struct board	*brd;
    int			num_avail_slots;
    int			avail_slots[MAX_NUM_SLOTS];
    int			i;
    int			status;
    char		buffer[80];
    char		tbuf[10];


    /*************
    * Load the cfg file specified and parse it. Error messages (if any)
    * will be displayed by add_to_slot().
    * See if there are any slots that would match this board. If so,
    * give the user a list of them. Note that add_to_slot() will display
    * an error message if the slot is occupied or out of range.
    *************/
    status = add_to_slot(cfg_file, (unsigned int)slot, &brd, 0);
    err_add_num_parm(1, (unsigned)slot);
    if (status == -2) {
	num_avail_slots = find_avail_slots(&glb_system, brd, avail_slots);
	if ( (slot < MAX_NUM_SLOTS) &&
	     (slot >= 0) &&
	     (glb_system.slot[slot].present) &&
	     (!glb_system.slot[slot].occupied) )
	    err_handler(SLOT_INCOMPATIBLE_ERRCODE);
	if (num_avail_slots == 0) {
	    err_handler(NO_AVAIL_SLOTS_ERRCODE);
	    err_handler(BOARD_NOT_ADDED_ERRCODE);
	}
	else {
	    *buffer = 0;
	    for (i=0 ; i<num_avail_slots ; i++) {
		(void)sprintf(tbuf, "%d  ", avail_slots[i]);
		(void)strcat(buffer, tbuf);
	    }
	    err_add_string_parm(1, buffer);
	    err_handler(VALID_SLOT_LIST_ERRCODE);
	    err_handler(BOARD_NOT_ADDED_ERRCODE);
	}
	return;
    }
    else if (status == -1) {
	err_handler(BOARD_NOT_ADDED_ERRCODE);
	return;
    }

    /****************
    * Try to configure the system.
    ****************/
    if (cf_configure(&glb_system, CF_ADD_OP) == -2) {
	err_handler(NOT_CONFIGURED_ERRCODE);
	return;
    }

    /**********
    * Put up a message telling the user that the board was successfully
    * added.
    **********/
    changed_since_save = 1;
    changed_since_start = 1;
    err_add_string_parm(1, brd->name);
    if (brd->comments)
	err_add_string_parm(2, brd->comments);
    else
	err_add_string_parm(2, (char *)0);
    err_handler(EXPLICIT_ADD_WORKED_ERRCODE);
    show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);

}


/****+++***********************************************************************
*
* Function:	mn_save_config()
*
* Parameters:   num_parms	        number of valid parms (0 or 1)
*		fname			sci file to save to (if num_parms = 1)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to save the current configuration to the
*	eeprom and default sci file (if num_parms == 0) or to a specified
*	sci file (if num_parms == 1).
*
****+++***********************************************************************/

static void mn_save_config(num_parms, fname)
    int		num_parms;
    char	*fname;
{

    if (num_parms == 0)
	(void)save((char *)NULL);
    else
	(void)save(fname);

}


/****+++***********************************************************************
*
* Function:	mn_init_config()
*
* Parameters:   num_parms	        number of valid parms (0 or 1)
*		fname			sci file to init from (if num_parms = 1)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function reinitializes the configuration from either the eeprom
*       or default sci file (if num_parms = 0) or a specified sci file (if
*	num_parms = 1).
*
****+++***********************************************************************/

static void mn_init_config(num_parms, fname)
    int		num_parms;
    char	*fname;
{
    int		err;
    int         c;
    int         i;
    char        buffer[256];


    /*************
    * Check to see if the configuration has changed since we started. If so,
    * ask the user if they really want to do this.
    *************/
    if (changed_since_start == 1) {
	err_handler(INIT_WILL_DESTROY_CHANGES_ERRCODE);
	i = 0;
	while ((c = getc(stdin)) != '\n') {
	    buffer[i++] = c;
	}
	err_handler(BLANK_LINE_ERRCODE);
	if ( (buffer[0] != 'c') && (buffer[0] != 'C') ) {
	    err_handler(INIT_ABORTING_ERRCODE);
	    return;
	}
    }

    /************
    * Re-initialize what we have to.
    ************/
    changed_since_save = 1;
    changed_since_start = 1;
    program_mode.non_target = 0;
    sci_init_cfg_file();

    /***********
    * Now start over, with either an sci file or nvm.
    ***********/
    if (num_parms == 1)  {
	program_mode.non_target = 1;
	(void)strcpy(sci_name, fname);
	err = mn_ntarget();
    }
    else
	err = init();

    /***********
    * If we had an error, blow this guy off. Otherwise, tell him that it worked
    * and do a show slots.
    ***********/
    if (err != 0) {
	err_handler(INIT_FAILED_ERRCODE);
	exiter(EXIT_INIT_ERR);
    }
    err_handler(INIT_WORKED_ERRCODE);
    show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);

}


/****+++***********************************************************************
*
* Function:	mn_exit_config()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to exit the eisa_config utility. There are
*	only two ways we return from here:
*		o   if the user changes her mind when prompted
*		o   if we try a save and it fails
*
****+++***********************************************************************/

static void mn_exit_config()
{
    int		c;
    int		i;
    int		status;
    char	buffer[256];


    /*************
    * If there have been no changes since we started, print a message to that
    * effect and get outta here.
    *************/
    if (changed_since_start == 0) {
	err_handler(EXIT_NO_CHANGES_EVER_ERRCODE);
	exiter(EXIT_OK_NO_REBOOT);
    }

    /*************
    * If there have been changes since we started, but no further changes 
    * since the last save to eeprom, print a message to that
    * effect and get outta here.
    *************/
    if (changed_since_save == 0) {
	pr_init_more();
	err_handler(EXIT_NO_NEW_CHANGES_ERRCODE);
	err_handler(EXIT_TODO_AFTER_EXIT_ERRCODE);
	show(2, "switch", "changed", (char *)0, NOT_AN_INT, 0, 1);
	err_handler(EXIT_LEAVING_ERRCODE);
	pr_end_more();
	exiter(EXIT_OK_REBOOT);
    }

    /*************
    * There have been changes since the last save to eeprom. Ask the user
    * what they want to do.
    *************/
    err_handler(EXIT_NEW_CHANGES_ERRCODE);
    i = 0;
    while ((c = getc(stdin)) != '\n') {
	buffer[i++] = c;
    }
    err_handler(BLANK_LINE_ERRCODE);

    /**************
    * The user wants to do the save, so try it. If the eeprom save failed,
    * just return to the user (message already displayed).
    **************/
    if ( (buffer[0] == 's') || (buffer[0] == 'S') ) {
	status = save((char *)0);
	if ( (status == CANT_WRITE_EEPROM_OR_SCI_ERRCODE) ||
	     (status == CANT_WRITE_EEPROM_ERRCODE) )  {
	    err_handler(EXIT_ABORT_EXIT_ERRCODE);
	    return;
	}
	pr_init_more();
	err_handler(EXIT_TODO_AFTER_EXIT_ERRCODE);
	show(2, "switch", "changed", (char *)0, NOT_AN_INT, 0, 1);
	err_handler(EXIT_LEAVING_ERRCODE);
	pr_end_more();
	exiter(EXIT_OK_REBOOT);
    }

    /**************
    * The user wants to do the exit without saving.
    **************/
    else if ( (buffer[0] == 'e') || (buffer[0] == 'E') ) {
	err_handler(EXIT_WITHOUT_SAVING_ERRCODE);
	exiter(EXIT_OK_NO_REBOOT);
    }

    /**************
    * The user wants to return to eisa_config (no exit).
    **************/
    else {
	err_handler(EXIT_ABORT_EXIT_ERRCODE);
    }



}


/****+++***********************************************************************
*
* Function:	mn_remove_board()
*
* Parameters:   slotnum			where board to be removed resides
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to remove a board from the configuration.
*
****+++***********************************************************************/

static void mn_remove_board(slotnum)
    int			slotnum;
{
    struct board 	*brd;


    /***************
    * Set up to print a message.
    ***************/
    err_add_num_parm(1, (unsigned)slotnum);

    /***************
    * Get a pointer to the board to be removed. If there is no such
    * board (bad slot given), handle it here.
    ***************/
    brd = get_this_board(slotnum);
    if (brd == NULL)
	return;
    err_add_string_parm(1, brd->name);

    /************
    * If this board is embedded, don't allow it to be removed.
    ************/
    if (brd->slot == bs_emb)  {
	err_handler(REMOVE_EMBEDDED_BRD_ERRCODE);
	return;
    }

    /************
    * Remove the board and reconfigure the system. Tell the user it worked.
    ************/
    glb_system.entries -= brd->entries;
    del_release_board(&glb_system, brd);
    (void)cf_configure(&glb_system, CF_DELETE_OP);
    err_handler(REMOVED_BOARD_ERRCODE);
    show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);
    changed_since_save = 1;
    changed_since_start = 1;
    sci_remove_cfg_file(slotnum);

}


/****+++***********************************************************************
*
* Function:	mn_move_board()
*
* Parameters:   curslotnum		where board is now
*		newslotnum		where board should be moved to
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function is called to move a board from one slot to another.
*
****+++***********************************************************************/

static void mn_move_board(curslotnum, newslotnum)
    int			curslotnum;
    int			newslotnum;
{
    struct board 	*brd;
    int			num_avail_slots;
    int			avail_slots[MAX_NUM_SLOTS];
    int			i;
    int			status;
    int			found = 0;
    char		buffer[80];
    char		tbuf[10];


    /***************
    * Get a pointer to the board to be moved. If there is no such
    * board (bad slot given), handle it here.
    ***************/
    brd = get_this_board(curslotnum);
    if (brd == NULL)
	return;
    err_add_string_parm(1, brd->name);

    /************
    * If this board is embedded or virtual, don't allow it to be moved.
    ************/
    if ( (brd->slot == bs_emb) || (brd->eisa_slot == 0) )  {
	err_add_num_parm(1, (unsigned)curslotnum);
	err_handler(MOVE_EMBEDDED_BRD_ERRCODE);
	return;
    }
#ifdef VIRT_BOARD
    else if (brd->slot == bs_vir)  {
	err_add_num_parm(1, (unsigned)curslotnum);
	err_handler(MOVE_VIRTUAL_BRD_ERRCODE);
	return;
    }
#endif

    /************
    * Get the list of currently available slots which this board could be
    * moved to. If the newslotnum is not on the list, handle it here.
    ************/
    err_add_num_parm(1, (unsigned)newslotnum);
    num_avail_slots = find_avail_slots(&glb_system, brd, avail_slots);
    if (num_avail_slots == 0) {
	err_handler(BAD_MOVE_SLOT_ERRCODE);
	if ( (newslotnum >= MAX_NUM_SLOTS) || (newslotnum < 0) || 
	     (glb_system.slot[newslotnum].present == 0) )
	    err_handler(NONEXISTENT_SLOT_ERRCODE);
	else if (glb_system.slot[newslotnum].occupied)
	    err_handler(SLOT_OCCUPIED_ERRCODE);
	else
	     err_handler(SLOT_INCOMPATIBLE_ERRCODE);
	err_handler(NO_AVAIL_SLOTS_ERRCODE);
	return;
    }
    else {
	*buffer = 0;
	for (i=0 ; i<num_avail_slots ; i++) {
	    if (avail_slots[i] == newslotnum) {
		found = 1;
		break;
	    }
	    (void)sprintf(tbuf, "%d  ", avail_slots[i]);
	    (void)strcat(buffer, tbuf);
	}
	if (!found) {
	    err_handler(BAD_MOVE_SLOT_ERRCODE);
	    if ( (newslotnum >= MAX_NUM_SLOTS) || (newslotnum < 0) || 
		 (glb_system.slot[newslotnum].present == 0) )
		err_handler(NONEXISTENT_SLOT_ERRCODE);
	    else if (glb_system.slot[newslotnum].occupied)
		err_handler(SLOT_OCCUPIED_ERRCODE);
	    else
		 err_handler(SLOT_INCOMPATIBLE_ERRCODE);
	    err_add_string_parm(1, buffer);
	    err_handler(VALID_SLOT_LIST_ERRCODE);
	    return;
	}
    }

    /************
    * Both slot numbers are valid, so make the switch.
    ************/
    brd->slot_number = newslotnum;
    brd->eisa_slot = glb_system.slot[newslotnum].eisa_slot;
    glb_system.slot[newslotnum].occupied = 1;
    glb_system.slot[curslotnum].occupied = 0;

    /************
    * Build the configuration and handle any errors.
    ************/
    status = cf_configure(&glb_system, CF_MOVE_OP);
    if (status != 0) {
	; /*do something!!!!!! */
    }
    else {
	err_handler(MOVE_WORKED_ERRCODE);
	show(0, (char *)0, (char *)0, (char *)0, 0, 0, 1);
	changed_since_save = 1;
	changed_since_start = 1;
	sci_move_cfg_file(curslotnum, newslotnum);
    }

}


/****+++***********************************************************************
*
* Function:	mn_change_function()
*
* Parameters:   slotnum			slot of board to change
*		name1			either a function name or a choice
*					name (Fnum or CHnum)
*		name2			like name1
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*	This function changes a given function to a specified choice.
*
****+++***********************************************************************/

static void mn_change_function(slotnum, name1, name2)
    int			slotnum;
    char		*name1;
    char		*name2;
{
    struct board	*brd;
    struct function	*func;
    struct subfunction	*subf;
    struct group	*grp;
    struct choice	*choi;
    int			counter=0;
    int			found_it;
    int			err;
    struct choice       *old_current;
    int			old_index;
    enum item_status	old_status;
    int			funcnum;
    int			choicenum;
    int			num;
    char 		*temp;
    char		*error;
    char		name[80];



    /*************
    * Get the function number and the choice number.
    *************/
    funcnum = -1;
    choicenum = -1;
    (void)strcpy(name, name1);
start:
    if (strncmpi(name, "F", 1) == 0)
	temp = &name[1];
    else if (strncmpi(name, "CH", 2) == 0)
	temp = &name[2];
    else {
	err_handler(INVALID_CHG_ERRCODE);
	return;
    }

    num = (int)strtol(temp, &error, 0);
    if (*error != NULL) {
	err_handler(INVALID_CHG_ERRCODE);
	return;
    }

    if (strncmpi(name, "F", 1) == 0)
	funcnum = num;
    else
	choicenum = num;

    if (counter == 0) {
	counter = 1;
	(void)strcpy(name, name2);
	goto start;
    }

    if ( (funcnum == -1) || (choicenum == -1) ) {
	err_handler(INVALID_CHG_ERRCODE);
	return;
    }
   
    /*************
    * Get the brd and (incidentally) verify the slot number.
    *************/
    brd = get_this_board(slotnum);
    if (brd == NULL)
	return;

    /**************
    * Set up some basic error parms.
    **************/
    err_add_num_parm(1, (unsigned)slotnum);
    err_add_num_parm(2, (unsigned)funcnum);
    err_add_num_parm(3, (unsigned)choicenum);

    /***************
    * Verify the function number and get a pointer to the correct subf
    * structure for the chosen function (if verified). We do this by walking
    * through all of the functions (and subfunctions, if explicit) for
    * this board. Functions without explicit subfunctions are counted as
    * one. Explicit subfunctions are also each counted as one. Note that
    * we ignore functions which are not displayed -- the user does not
    * know about them!
    ***************/
    counter = 1;
    found_it = 0;
    for (grp = brd->groups; grp != NULL; grp = grp->next)  {
	for (func = grp->functions; func != NULL; func = func->next)  {
	    if (!func->display)
		continue;
	    subf = func->subfunctions;
	    if (subf->explicit)  {
		for ( ; subf != NULL; subf = subf->next) {
		    if (counter == funcnum) {
			found_it = 1;
			goto out_of_loop;
		    }
		    counter++;
		}
	    }
	    else {
		if (counter == funcnum) {
		    found_it = 1;
		    goto out_of_loop;
		}
		counter++;
	    }
	}
    }

out_of_loop:
    if (found_it == 0) {
	if (counter == 1)
	    err_handler(NO_VALID_FUNCTIONS_ERRCODE);
	else if (counter == 2)
	    err_handler(FUNCTION_INVALID2_ERRCODE);
	else {
	    err_add_num_parm(4, (unsigned)(counter-1));
	    err_handler(FUNCTION_INVALID1_ERRCODE);
	}
	return;
    }

    /***************
    * Verify the choice number.
    ***************/
    if ((choicenum-1) == subf->index.current) {
	err_handler(CHOICE_THE_SAME_ERRCODE);
	return;
    }
    counter = 1;
    found_it = 0;
    for (choi=subf->choices ; choi != NULL ; choi = choi->next) {
	if (counter == choicenum) {
	    found_it = 1;
	    break;
	}
	counter++;
    }
    if (found_it == 0) {
	err_add_num_parm(4, (unsigned)(counter-1));
	err_handler(CHOICE_INVALID_ERRCODE);
	return;
    }

    /***************
    * Everything is valid, so try the edit.
    ***************/
    old_current = subf->current;
    old_index = subf->index.current;
    old_status = subf->status;
    subf->current = choi;
    subf->index.current = choicenum - 1;
    subf->status = is_locked;
    err = cf_configure(&glb_system, CF_EDIT_OP);

    /***************
    * If the edit configuration failed, set stuff the way they were before.
    * Tell the user it failed.
    ***************/
    if (err == 0) {
	changed_since_save = 1;
	changed_since_start = 1;
	err_handler(CHANGE_WORKED_ERRCODE);
    }
    else {
	subf->current = old_current;
	subf->index.current = old_index;
	subf->status = old_status;
	err = cf_configure(&glb_system, CF_EDIT_OP);
	err_handler(CHANGE_FAILED_ERRCODE);
    }

}


/****+++***********************************************************************
*
* Function:	exiter()
*
* Parameters:   exit_code		exit code
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*	This function does some clean up:
*	    o  removes the temporary print file (if any)
*	    o  if we have printed auto mode messages, it prints the trailer
*	       message and closes the file
*	    o  exits with the given code
*
****+++***********************************************************************/

void exiter(exit_code)
    int		exit_code;
{

    if (*more_file_name != 0)
	(void)unlink(more_file_name);

    if (auto_mode_messages == 1)  {
	err_handler(AUTO_MODE_TRAILER_ERRCODE);
	(void)fclose(print_err_file_fd);
    }

    exit(exit_code);

}
