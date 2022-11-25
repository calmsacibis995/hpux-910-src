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
*+*+*+*************************************************************************
*             
*                         src/err.c
*
*   This file contains the code to handle generic errors.
*
*	err_handler()
*	err_add_string_parm()
*	err_add_num_parm()
*
**++***************************************************************************/


#include <stdio.h>
#include <string.h>
#include <time.h>
#include "err.h"
#include "config.h"
#include "pr.h"


/**************
* Globals used here but declared in globals.c
**************/
FILE			*print_err_file_fd;
extern struct pmode     program_mode;


/**************
* Functions declared in this file
**************/
void		err_handler();
void		err_add_string_parm();
void		err_add_num_parm();


/**************
* These are the parameters that will be stuffed into the message on the next
* err_handler() call.
**************/
#define NUM_STRING_PARMS	5
#define NUM_NUM_PARMS		5
static char			str_parm1[80];
static char			str_parm2[600];  /* max size of comments/help */
static char			str_parm3[80];
static char			str_parm4[80];
static char			str_parm5[80];
static unsigned 		num_parm1;
static unsigned 		num_parm2;
static unsigned 		num_parm3;
static unsigned 		num_parm4;
static unsigned 		num_parm5;


/**************
* These are the messages which we will display on errors.
**************/
#define COULD_NOT_OPEN_CFG_ERRSTR	"\
Could not find the %s CFG file.\n\
Ensure that this file is present in the %s directory."

#define COULD_NOT_OPEN_SCI_ERRSTR	"\
Could not find the %s SCI file.\n"

#define BOARD_NOT_ADDED_ERRSTR		"\
Board not added to slot %u."

#define NO_AVAIL_SLOTS_ERRSTR 		"\
No slots are available for this board."

#define VALID_SLOT_LIST_ERRSTR		"\
Valid slots for this board: %s"

#define INTERACTIVE_HEADER1_ERRSTR 	"\n\
HP-UX E/ISA CONFIGURATION UTILITY"

#define INTERACTIVE_HEADER2_ERRSTR 	"\n\
    Type q or quit to exit eisa_config.\n\
    Type ? or help for help on eisa_config commands.\n\n"

#define INTERACTIVE_INVALID_CMD_ERRSTR  "\
Not a valid command.\n"

#define TYPE_HELP_ERRSTR		"\
Type \"?\" to obtain a list of valid commands.\n"

#define AUTO_MODE_HEADER_ERRSTR		"\n\
**************\n\
**************\n\
eisa_config running in automatic mode to fix a configuration mismatch\n\
    Date: %s\n"

#define AUTO_MODE_TRAILER_ERRSTR	"\n\
eisa_config finished\n\
**************\n\
**************\n"

#define AUTO_RESTORE_FROM_SCI_ERRSTR	"\
Restoring configuration from default sci file (/etc/eisa/system.sci).\n"

#define AUTO_TRYING_REINIT_ERRSTR	"\
One or more boards have been successfully added to the configuration.\n\
Since none of the existing boards have been re-configured, there will be\n\
no need to reboot the system. The drivers for the new boards will now be\n\
initialized.\n\
\n\
If the required drivers are not present in the kernel, they\n\
must be added before the boards will be usable. Refer to the\n\
\"E/ISA Configuration Documentation\".\n"

#define AUTO_MUST_REBOOT_ERRSTR		"\
The system will be rebooted now."

#define BAD_SCI_FILE_ERRSTR		"\
The specified SCI file (%s) is invalid or corrupt.\n\
Try another SCI file."

#define INTERACTIVE_PROMPT_ERRSTR	"\n\
EISA: "

#define AUTO_SYSBRD_ID_CHANGED_ERRSTR	"\
The system board ID has changed since the last time the configuration was\n\
saved to NVM. The previous NVM contents will be disregarded. eisa_config\n\
will attempt to build a configuration based on the boards that are\n\
detected in the backplane.\n\
\n\
If you have ISA boards, you must add them using interactive eisa_config\n\
after the system has booted. Refer to the \"E/ISA Configuration\n\
Documentation\".\n"

#define SYSBRD_ID_CHANGED_ERRSTR	"\n\
The system board ID has changed since the last time the configuration was\n\
saved to NVM. The previous NVM contents (if any) will be disregarded. If\n\
you want any E/ISA boards in your configuration, you must add them using\n\
interactive eisa_config. Refer to the \"E/ISA Configuration Documentation\"."

#define AUTO_SYSBRD_CFG_FILE_DIFF_ERRSTR	"\
The system board cfg file (%s) has changed since the\n\
configuration was changed to NVM. The previous NVM contents (if any) will\n\
be disregarded. eisa_config will attempt to build a configuration based\n\
on the boards that are detected in the backplane.\n\
\n\
If you have ISA boards, you must add them using interactive eisa_config\n\
after the system has booted. Refer to the \"E/ISA Configuration\n\
Documentation\".\n"

#define SYSBRD_CFG_FILE_DIFF_ERRSTR	"\
The system board cfg file (%s) has changed since the\n\
configuration was changed to NVM. The previous NVM contents (if any) will\n\
be disregarded. If you want any E/ISA boards in your configuration, you\n\
must add them using interactive eisa_config. Refer to the \"E/ISA\n\
Configuration Documentation\"."

#define AUTO_FIRST_EISA_ERRSTR		"\
This is the first time you have booted with EISA boards in your backplane.\n\
eisa_config will attempt to build a configuration automatically based\n\
on the boards that are detected in the backplane.\n\
\n\
If you have ISA boards, you must add them using interactive eisa_config\n\
after the system has booted. Refer to the \"E/ISA Configuration\n\
Documentation\".\n"

#define AUTO_INVALID_EEPROM_READ_ERRSTR	"\
The NVM is not internally consistent. The previous NVM contents (if any)\n\
will be disregarded. This error may indicate a hardware problem with\n\
your EISA NVM.\n"

#define AUTO_FROM_SCRATCH_ERRSTR	"\
eisa_config will attempt to build a configuration based on the boards\n\
that are detected in the backplane.\n\
\n\
If you have ISA boards, you must add them using interactive eisa_config\n\
after the system has booted. Refer to the \"E/ISA Configuration\n\
Documentation\".\n"

#define INVALID_EEPROM_READ_ERRSTR	"\
The NVM is not internally consistent. The previous NVM contents (if any)\n\
will be disregarded. If you want any E/ISA boards in your configuration, you\n\
must add them using interactive eisa_config. Refer to the \"E/ISA\n\
Configuration Documentation\". This error may indicate a hardware problem\n\
with your EISA NVM."

#define INVALID_SYSBRD_ERRSTR 		"\
Cannot run eisa_config without the system board CFG file.\n\
Ensure that this file is present in the /etc/eisa directory."

#define CANT_WRITE_EEPROM_ERRSTR  	"\
Could not save configuration in NVM.\n\
Replace the NVM and rebuild this configuration."

#define CANT_WRITE_SCI_ERRSTR  	        "\
Could not save configuration in %s.\n\
The directory may not exist or you may lack write permission."

#define EEPROM_UPDATE_OK_ERRSTR		"\
Successfully saved configuration in NVM."

#define EEPROM_AND_CFG_MISMATCH_ERRSTR  "\
The board that was previously configured in slot %u cannot be automatically\n\
added to the configuration. The CFG file %s does not\n\
match the data saved in NVM for this board. There are two possible causes:\n\
    (1) The CFG file has changed since the NVM was last written.\n\
    (2) The NVM data is corrupt.\n\
To use this board, you must add it using interactive eisa_config. Refer\n\
to the \"E/ISA Configuration Documentation\".\n"

#define AUTO_EEPROM_AND_CFG_MISMATCH_ERRSTR  "\
The CFG file %s does not match the data saved in NVM\n\
for the board in slot %u. There are two possible causes:\n\
    (1) The CFG file has changed since the NVM was last written.\n\
    (2) The NVM data is corrupt.\n\
To use this board, you must add it using interactive eisa_config. Refer\n\
to the \"E/ISA Configuration Documentation\".\n"

#define SCI_UPDATE_OK_ERRSTR		"\
Successfully saved configuration in %s."

#define EEPROM_AND_SCI_UPDATE_OK_ERRSTR	"\
Successfully saved configuration in NVM and in %s.\n"

#define ILLEGAL_INCLUDE_ERRSTR 		"\
An include statement has been detected in the CFG file %s.\n\
Include statements are not supported."

#define EEPROM_IN_USE_ERRSTR 		"\
The eisa driver (/dev/eeprom) is already being used by someone else.\n\
Ensure that you are not running more than one session of eisa_config."

#define AUTO_EEPROM_IN_USE_ERRSTR 	"\
The eisa driver (/dev/eeprom) is already being used by someone\n\
else. The configuration cannot be checked for consistency. If you have\n\
changed your configuration since the last reboot, run eisa_config\n\
interactively once the system has completed the boot process. Refer\n\
to the \"E/ISA Configuration Documentation\"."

#define AUTO_RESOURCE_CONFLICT_ERRSTR	"\
The board in slot %u cannot be added to the configuration automatically.\n\
It requires one or more of the resources that another board is using.\n\
\n\
After the system boots, try running eisa_config interactively to\n\
add the new board. Refer to the \"E/ISA Configuration Documentation\".\n"

#define AUTO_CANT_ADD_NEW_BOARD_ERRSTR		"\
The board in slot %u will not be added to the configuration\n\
automatically. It will not be accessible until the problem is fixed.\n\
Once the system boots, fix the problem and then reboot the system.\n"

#define AUTO_CANT_ADD_EXISTING_BOARD_ERRSTR	"\
The existing board in slot %u cannot be processed. eisa_config cannot add\n\
any new boards until this problem is fixed.\n\
\n\
Once the system boots, fix the problem and then reboot the system. Refer\n\
to the \"E/ISA Configuration Documentation\".\n"

#define AUTO_BOOT_BOARD_NOT_ADDED_1_ERRSTR	"\
Until you fix the problem explained above, the only EISA board(s) that\n\
will be usable are:\n\
	 Slot(s) %s\n"

#define AUTO_BOOT_BOARD_NOT_ADDED_2_ERRSTR	"\
The board in slot %u will not be added to the configuration\n\
automatically. Once the system boots, fix the problem and then reboot\n\
your system.\n"

#define AUTO_CANT_ADD_NEW_BOARD_ERRSTR		"\
The board in slot %u will not be added to the configuration\n\
automatically. It will not be accessible until the problem is fixed.\n\
Once the system boots, fix the problem and then reboot the system.\n"


#define INVALID_EEPROM_ERRSTR 		"\
The EISA NVM is corrupt or broken. Contact your HP service\n\
representative."

#define NO_EISA_HW_PRESENT_ERRSTR	"\
This machine does not appear to have an EISA backplane. If an EISA\n\
backplane is present, it cannot be identified.\n"

#define NO_EEPROM_DRIVER_ERRSTR		"\
The eisa (/dev/eeprom) is not in the kernel.\n\
You must add the eisa driver to your kernel. Refer to the\n\
\"E/ISA Configuration Documentation\".\n"

#define AUTO_NO_EEPROM_DRIVER_ERRSTR	"\
The eisa driver (/dev/eeprom) is not in the kernel.\n\
If you do not have any E/ISA boards present, you can safely ignore\n\
this message.\n\
\n\
If you have an E/ISA board present, you must first add the eisa driver\n\
to your kernel. Refer to the \"E/ISA Configuration Documentation\".\n"

#define AUTO_BOARDS_REMOVED_ERRSTR	"\
One or more of the EISA boards that are listed in the configuration\n\
are no longer responding. There are two possible causes:\n\
     (1)  The board has been removed.\n\
     (2)  The board is no longer responding because of a hardware problem.\n\
The board(s) listed below are not responding:"

#define AUTO_BOARD_REMOVED_SLOT_ERRSTR	"\
    Slot %u"

#define NO_MORE_MEMORY_ERRSTR 		"\
The eisa_config program cannot allocate the memory that it needs -- exiting!\n"

#define CHECK_CFG_HDR_ERRSTR		"\n\
Checking this CFG File for correctness: %s\n"

#define BOARD_NEEDS_TAG_ERRSTR  		"\
This board cannot use slot %u because the tags do not match."

#define MATCHING_TAGS_ERRSTR	 	"\
The slot(s) with a matching tag are: %s"

#define BOARD_NEEDS_BUSMASTER_ERRSTR    "\
This board cannot use slot %u because this slot does not\n\
support bus master boards."

#ifdef VIRT_BOARD
#define INV_EMB_BOARD_ERRSTR 		"\
Invalid embedded board description -- needs a slot number."

#define EMB_SLOT_OCCUPIED_ERRSTR	"\
Embedded slot %u is already occupied by another board."
#endif

#define SLOT_OCCUPIED_ERRSTR		"\
Slot %u is already occupied by another board."

#define AMP_OVERLOAD_FIXED_ERRSTR  	"\
The boards in your computer are no longer drawing too much power.\n\
You can ignore the previous warning message.\n"

#define AMP_OVERLOAD_DETECTED_ERRSTR	"\
Your computer has a maximum power capacity of %u milliamps.\n\
The added boards will draw a total of %u milliamps.\n\n\
This is a greater power draw than HP recommends, and\n\
could damage your computer.\n"

#define NOT_CONFIGURED_ERRSTR 		"\
A conflict-free configuration cannot be generated if this board is\n\
added to the system. Hence, the board was not added."

#define INVALID_SHOW_CMD_ERRSTR		"\
Incorrect parameter in a show command. The valid show commands are:"

#define EMPTY_SLOT_ERRSTR		"\
Slot %u is empty.\n"

#define NONEXISTENT_SLOT_ERRSTR		"\
Slot %u does not exist.\n\
Type \"show\" to see a list of valid slot numbers.\n"

#define AUTO_SWITCH_ERRSTR		"\
One or more the boards in the backplane need hardware switches or\n\
jumpers set to certain values. The required switch and jumper settings\n\
are listed below.\n"

#define AUTO_SWITCH_PROMPT_ERRSTR	"\n\
The switches and jumpers listed above must match your boards. If they\n\
do not, the system will act unpredictably. If your switches and jumpers\n\
do not match the required settings, write down the required settings now.\n\
Choose one of the following options: \n\
     n  --  no, switches and jumpers do not currently match\n\
     y  --  yes, switches and jumpers already match\n\
     \n\
Please enter your choice followed by a carriage return: "

#define AUTO_SWITCH_HALT_ERRSTR		"\
The system will be halted so that you can change the switches\n\
and jumpers to match what eisa_config is expecting. After the system is\n\
halted, turn the machine off and change the switches/jumpers. Then,\n\
turn your machine back on."

#define NO_SWITCH_CHANGE_ERRSTR		"\
No switches or jumpers have changed."

#define NO_SWITCH_CHANGE_IN_SLOT_ERRSTR	"\
No switches or jumpers have changed on the board in this slot."

#define NO_SWITCHES_ERRSTR		"\
None of the boards in the current configuration have hardware switches\n\
or jumpers."

#define NO_SWITCHES_IN_SLOT_ERRSTR	"\
This board does not have any hardware switches or jumpers."

#define INVALID_COMMENT_CMD_ERRSTR	"\
Incorrect parameter in a comment command. The valid comment commands are:"

#define INVALID_HELP_CMD_ERRSTR		"\
Incorrect parameter in a help command. The valid help commands are:"

#define REMOVE_EMBEDDED_BRD_ERRSTR 	"\
Cannot remove embedded board:  %s\n\
    Slot:  %d"

#define REMOVED_BOARD_ERRSTR		"\
Removed board:  %s\n"

#define MOVE_EMBEDDED_BRD_ERRSTR	"\
Cannot move embedded board:  %s\n\
    Slot:  %d"

#ifdef VIRT_BOARD
#define MOVE_VIRTUAL_BRD_ERRSTR 		"\
Cannot move virtual board:  %s\n\
    Slot:  %d"
#endif

#define BAD_MOVE_SLOT_ERRSTR		"\
Cannot move board:  %s"

#define SLOT_INCOMPATIBLE_ERRSTR	"\
Slot %d is not compatible with this board."

#define MOVE_WORKED_ERRSTR		"\
Moved board:       %s\n\
\n\
You may need to make new device files for devices connected to this\n\
board. For more detail, refer to the \"E/ISA Configuration Documentation\".\n"

#define EXPLICIT_ADD_WORKED_ERRSTR	"\
Added board:  %s"

#define AUTO_ADD_OF_NEW_BOARD_ERRSTR	"\
Automatically added a new board in slot %u.\n"

#define COMMENTS_ERRSTR			"\
 Comments:     "

#define NO_COMMENTS_ERRSTR		"\
None"

#define CANNOT_OPEN_CFG_DIR_ERRSTR 	"\
Could not open the CFG file directory %s.\n\
The directory may not exist or you may lack read permission."

#define NO_CFG_FILES_ERRSTR		"\
Could not find any valid CFG files in %s."

#define NO_CFG_FILES_OF_THIS_TYPE_ERRSTR "\
Could not find any valid CFG files of type %s in %s."

#define LEAVING_ERRSTR			"\
Exiting eisa_config.\n"

#define EXIT_ABORT_EXIT_ERRSTR		"\
Aborting quit."

#define EXIT_NO_CHANGES_EVER_ERRSTR	"\
The configuration was not changed."

#define EXIT_NO_NEW_CHANGES_ERRSTR	"\
The configuration was changed and has already been saved to NVM."

#define EXIT_WITHOUT_SAVING_ERRSTR	"\
The configuration was not saved."

#define EXIT_TODO_AFTER_EXIT_ERRSTR 	"\n\
A description of the configuration was saved in /etc/eisa/config.log.\n\
\n\
If eisa_config was run per the instructions of a specific product\n\
installation manual, refer to that manual for specifics on device file\n\
creation and I/O drivers. Step 4 may apply if other cards were affected.\n\
\n\
Otherwise, the following is a list of generally required steps:\n\
\n\
    (1)  Make any necessary device files. If you have moved a board you\n\
	 may also need to make new device files.\n\
    (2)  Ensure that all appropriate software I/O drivers are present\n\
	 in the kernel.\n\
    (3)  Shut down the system with the \"/etc/shutdown -h\" command.\n\
    (4)	 Once the system is shut down, turn the power off. Then set any\n\
         physical switches and jumpers correctly. The switches and jumpers\n\
         that have changed since you invoked eisa_config are listed below.\n\
         Also refer to /etc/eisa/config.log for a summary of the new\n\
         configuration, including required settings.\n\
    (5)	 Physically add, move, or remove boards as needed.\n\
    (6)  Turn the power on and boot the system.\n\
\n\
Refer to the \"E/ISA Configuration Documentation\" for specific instructions."

#define EXIT_NEW_CHANGES_ERRSTR 		"\
The configuration has changed since the last save to NVM.\n\
Choose one of the following options: \n\
     s  --  save the configuration to NVM and then exit\n\
     e  --  exit without saving the configuration\n\
     a  --  abort exit (return to eisa_config prompt)\n\
\n\
\n\
Please enter your choice followed by a carriage return: "

#define INIT_ABORTING_ERRSTR		"\
Aborting init command."

#define INIT_WORKED_ERRSTR		"\
The initialization was successful.\n"

#define INIT_FAILED_ERRSTR 		"\
The initialization failed."

#define INIT_WILL_DESTROY_CHANGES_ERRSTR "\
The configuration has changed since you invoked eisa_config.\n\
If you continue with this init command, the current configuration\n\
will be lost.\n\
Choose one of the following options: \n\
     a  --  abort init (return to eisa_config prompt)\n\
     c  --  continue init (start with a new configuration)\n\
\n\
\n\
Please enter your choice followed by a carriage return: "

#define NO_OTHER_OPTIONS_WITH_CHECK_ERRSTR	"\n\
When the -c option is used, no other options are allowed."

#define NO_OTHER_OPTIONS_WITH_AUTO_ERRSTR	"\n\
When the -a option is used, no other options are allowed."

#define FUNCTION_INVALID1_ERRSTR		"\
This function number is out of range.\n\
Valid functions for the board in slot %u are F1 to F%u.\n\
To see the functions and choices for this board, use the\n\
command \"show board %u\". \n"

#define FUNCTION_INVALID2_ERRSTR		"\
This function number is out of range.\n\
The only valid function for the board in slot %u is F1.\n\
To see the functions and choices for this board, use the\n\
command \"show board %u\". \n"

#define NO_VALID_FUNCTIONS_ERRSTR		"\
No functions can be changed on the board in slot %u.\n"

#define CHOICE_INVALID_ERRSTR			"\
This choice number is out of range.\n\
Valid choices for function F%u for the board in slot %u are\n\
CH1 to CH%u. To see the functions and choices for this board, use the\n\
command \"show board %u\". \n"

#define CHOICE_THE_SAME_ERRSTR			"\
Function F%u on the board in slot %u is already using choice CH%u.\n\
To see the functions and choices for this board, use the\n\
command \"show board %u\". \n"

#define CHANGE_FAILED_ERRSTR			"\
Change function failed. One of the resources used by the specified choice\n\
is used elsewhere.\n"

#define CHANGE_WORKED_ERRSTR			"\
Change function succeeded.\n\
Function F%u on the board in slot %u is now using choice CH%u.\n"

#define IODC_BAD_OPTION_ERRSTR		"\n\
To specify an IODC-style save of slot data, you must use all of these\n\
three options:\n\
    -n  sci_filename  (input sci file)\n\
    -S  slot_num      (slot number from sci_filename to save away)\n\
    -I  filename      (target filename)\n"

#define IODC_SUCCESSFUL_ERRSTR		"\n\
IODC data was successfully written.\n"

#define IODC_BAD_SLOT_ERRSTR		"\n\
The slot specified is either empty in the sci file or non-existent.\n"

#define IODC_WRITE_PROBLEM_ERRSTR	"\n\
Could not write the output file specified.\n"

#define TOO_MANY_IRQS_ERRSTR		"\
Too many interrupt lines are used by the board in slot %u."

#define TOO_MANY_DMAS_ERRSTR		"\
Too many dma channels are used by the board in slot %u."

#define TOO_MANY_PORTS_ERRSTR		"\
Too many ports are used by the board in slot %u."

#define TOO_MANY_INITS_ERRSTR		"\
Too many port initializations are used by the board in slot %u."

#define TOO_MANY_MEMORYS_ERRSTR		"\
Too many memory resources are used by the board in slot %u."

#define BOARD_NOT_SAVED_ERRSTR		"\n\
This board will not be saved. Will try to save remaining boards\n\
(if any).\n"


#ifdef DEBUG

#define DEBUG_FREE_FAILS_ERRSTR 		"\
Tried to free a buffer that was never allocated"

#define DEBUG_ALLOC_FAILS_ERRSTR 	"\
Too many mallocs have been done"

#endif


/*************
* Runstring help strings
*************/

#define RUNSTRING_HELP_ERRSTR		"\n\
The eisa_config command options are:\n\
\n\
    -a               automatic mode\n\
    -c cfgfile       check CFG file mode\n\
    -n sci_filename  non-target mode\n"


/************
* Interactive help strings
************/

#define HELP_ADD_ERRSTR		"\
   add <cfgfile> <slotnum>\n\
       Adds a board to the current configuration. You must specify which\n\
       CFG file corresponds to the board and which slot the board will be\n\
       put into. To see the CFG file name that corresponds to each\n\
       board, use the \"cfgfiles\" command. \n"

#define HELP_REMOVE_ERRSTR		"\
   remove <slotnum>\n\
       Removes a board from the current configuration. You must specify\n\
       which slot the board is currently in.\n"

#define HELP_MOVE_ERRSTR		"\
   move <curslotnum> <newslotnum>\n\
       Moves a board that is currently configured in one slot to a\n\
       different slot. You must specify the two slot numbers.\n"

#define HELP_CHG_ERRSTR			"\
   change <slotnum> <functionname> <choicename>\n\
       Changes the choice used for a given function. You must specify\n\
       the slot number, the function name, and the choice name.\n\
       To see function and choice names, use the show board <slotnum>\n\
       command. Function names have the format F<num> and choice names\n\
       have the format CH<num>. Note that a board must already be part\n\
       of the configuration before you can use the change command on it.\n"

#define HELP_SAVE_ERRSTR		"\
   save [ <filename> ]\n\
       Saves the current configuration. If the current configuration is not\n\
       conflict-free, you will be notified and the configuration will not\n\
       be saved. If you specify a file name, the configuration will be\n\
       saved to that file. Otherwise, the configuration will be saved to\n\
       the NVM and the /etc/eisa/system.sci file.\n\n\
       Note that the \"quit\" command also allows you to save the\n\
       configuration to the NVM and the /etc/eisa/system.sci file.\n\
       Saving or quitting also writes a summary of the new configuration\n\
       in the file /etc/eisa/config.log.\n"

#define HELP_INIT_ERRSTR		"\
   init [ <filename> ]\n\
       Initializes the configuration. If you specify an SCI file name,\n\
       the initial configuration is retrieved from that file. Otherwise, it\n\
       is retrieved from the NVM. Note that an implicit init will be\n\
       done when eisa_config is first started. \n\n\
       You should use this command only when the current configuration\n\
       is incorrect. For example, if you make some changes and decide you\n\
       do not want to keep them, you can use this command to start over.\n"

#define HELP_QUIT_ERRSTR		"\
   quit (or q)\n\
       Exits eisa_config. If the configuration is conflict-free and has\n\
       been changed, you will be asked if you want to save the\n\
       configuration (to NVM). If any switches or jumpers must be\n\
       changed as a result of this new configuration, you will be\n\
       notified of the required changes. The file /etc/eisa/config.log\n\
       also contains a summary of the new configuration. You must ensure\n\
       that all switches and jumpers match what eisa_config has specified\n\
       before you reboot the system.\n"


#define HELP_SHOW_ERRSTR		"\
   show\n\
       Shows a list of all the slots and whether they are empty or occupied\n\
       by a particular board.\n"

#define HELP_SHOW_SLOT_ERRSTR		"\
   show slots <cfgfile>\n\
       Shows a list of all the slots that can accept the board\n\
       corresponding to the specified CFG file.\n"

#define HELP_SHOW_BOARD_ERRSTR		"\
   show board [ <cfgfile> | <slotnum> ]\n\
       Shows a list of basic attributes for the selected board(s),\n\
       including all functions and choices. If the board is part of\n\
       the working configuration, this command also indicates\n\
       the currently selected choice for each function.\n\
       If you do not specify either a CFG file name or a slot\n\
       number, information is displayed for all of the boards in the system.\n"

#define HELP_SHOW_SWITCH_ERRSTR		"\
   show switch [ changed ] [ <slotnum> ]\n\
       Shows any switch and jumper settings (both default and required) for\n\
       the boards in the configuration. If you use the keyword \"changed\",\n\
       only those switches and jumpers that have changed since eisa_config\n\
       was invoked are displayed. If you specify a slot number,\n\
       only switches and jumpers on the board in that slot are displayed.\n\
       Note that you can use all combinations of \"changed\" and <slotnum>\n"


#define HELP_CFG_ERRSTR			"\
   cfgtypes\n\
       Lists the types of boards that have CFG files in the /etc/eisa\n\
       directory. For each board type, also lists the number of\n\
       associated CFG files in /etc/eisa.\n\n\
   cfgfiles [ <type> ]\n\
       Lists the CFG files that are currently available for use in the\n\
       /etc/eisa directory. If you specify a board type such as NET,\n\
       only CFG files of that type will be displayed.\n"

#define HELP_COMMENT_ERRSTR		"\
   comment <level>  [ <cfgfile> | <slotnum> ]\n\
       Displays comments or help provided by the board manufacturer in the\n\
       CFG file. If you do not specify a CFG file or slot number, comments\n\
       for all boards in the configuration will be displayed. You must\n\
       specify one of the following levels:\n\
	  board\n\
	  switch\n\
	  function\n\
	  choice\n"

#define HELP_HELP_ERRSTR		"\
   help [ <command> ]\n\
       Lists and explains eisa_config commands. If you specify a command,\n\
       help is displayed for that command only. If you do not specify a\n\
       command, help is displayed for all eisa_config interactive commands.\n"

/**************
* These are the messages that the parser can spit out.
**************/

#define PARSE0_ERRSTR    "CFG file [%s] is illegal. Error at line number:  %u"
#define PARSE1_ERRSTR    "      ADDRESS statement is not allowed with expanded memory"
#define PARSE2_ERRSTR    "      LINK group - INIT statement mismatch"
#define PARSE3_ERRSTR    "      INITVAL statement / IOPORT size mismatch"
#define PARSE4_ERRSTR    "      Board category not specified"
#define PARSE5_ERRSTR    "      Board ID not specified"
#define PARSE6_ERRSTR    "      Board manufacturer not specified"
#define PARSE7_ERRSTR    "      Board name not specified"
#define PARSE8_ERRSTR    "      Embedded characters removed from string"
#define PARSE9_ERRSTR    "      DECODE statement can only use 20 | 24 | 32"
#define PARSE10_ERRSTR   "      Duplicate expansion slot definition"

#define PARSE11_ERRSTR   "      Duplicate index"
#define PARSE13_ERRSTR   "      Embedded slot number out of range"
#define PARSE14_ERRSTR   "      SYS board category for system boards only"
#define PARSE16_ERRSTR   "      INIT statement between MEMORY and ADDRESS only allowed in LINK group"
#define PARSE17_ERRSTR   "      All EISA slot numbers must be specified"
#define PARSE18_ERRSTR   "      Invalid board category, 3 characters required"
#define PARSE19_ERRSTR   "      Invalid board ID"
#define PARSE20_ERRSTR   "      Invalid slot number"

#define PARSE21_ERRSTR   "      Invalid INIT statement range"
#define PARSE22_ERRSTR   "      Invalid IOPORT / SWITCH / JUMPER / SOFTWARE index"
#define PARSE23_ERRSTR   "      Invalid jumper pair"
#define PARSE24_ERRSTR   "      Invalid LOC list numbering"
#define PARSE25_ERRSTR   "      Invalid numerical argument:  %s"
#define PARSE26_ERRSTR   "      Invalid PORTVAR index"
#define PARSE27_ERRSTR   "      Invalid I/O port address"
#define PARSE28_ERRSTR   "      LOC list / value list mismatch"
#define PARSE29_ERRSTR   "      LOC list out of range"
#define PARSE30_ERRSTR   "      Maximum amperage is 99999 decimal"

#define PARSE31_ERRSTR   "      Maximum board length is 999 decimal"
#define PARSE32_ERRSTR   "      Maximum Busmaster is 999 decimal"
#define PARSE34_ERRSTR   "      Maximum memory allowable is 64 Megabytes"
#define PARSE35_ERRSTR   "      Maximum Slot length is 999 decimal"
#define PARSE36_ERRSTR   "      Maximum System board nonvolatile is 64K, or 65536"
#define PARSE37_ERRSTR   "      MEMORY statement must be alone in a resource group"
#define PARSE38_ERRSTR   "      Missing ADDRESS statement"
#define PARSE39_ERRSTR   "      Missing CHOICE statement"
#define PARSE40_ERRSTR   "      Missing FUNCTION statement"

#define PARSE41_ERRSTR   "      Missing quotation mark"
#define PARSE42_ERRSTR   "      N or n only used with tripole jumpers"
#define PARSE43_ERRSTR   "      COMBINE group - INIT statement mismatch"
#define PARSE44_ERRSTR   "      Number of switches / jumpers must be 1 - 16"
#define PARSE46_ERRSTR   "      Internal error, file too complex"
#define PARSE47_ERRSTR   "      Slide and rotary switches must be set to a single position"
#define PARSE48_ERRSTR   "      String too long, truncated"

#define PARSE51_ERRSTR   "    Successful syntax verification\n"
#define PARSE52_ERRSTR   "      Switch / jumper name not specified"
#define PARSE53_ERRSTR   "      Switch / jumper type not specified"
#define PARSE54_ERRSTR   "      Syntax error"
#define PARSE55_ERRSTR   "      Syntax error:  %s"
#define PARSE56_ERRSTR   "      SYSTEM statement only valid for system board"
#define PARSE57_ERRSTR   "      SYSTEM statement required for system board"
#define PARSE58_ERRSTR   "      INIT statement / IOPORT size mismatch"
#define PARSE59_ERRSTR   "      X can not be used in an INIT or FACTORY statement"
#define PARSE60_ERRSTR   "      Each list must have the same number of values"

#define PARSE61_ERRSTR   "      ANDed values allowed in LINK groups only"
#define PARSE63_ERRSTR   "      Invalid DMA channel"
#define PARSE64_ERRSTR   "      Invalid IRQ level"
#define PARSE65_ERRSTR   "      Illegal reversed init statement range"
#define PARSE66_ERRSTR   "      Range must be low - high"
#define PARSE67_ERRSTR   "      INIT statement redefinition of reserved bits"
#define PARSE68_ERRSTR   "      Freeform data value out of range"
#define PARSE69_ERRSTR   "      Exceeded maximum number of freeform data values"
#define PARSE70_ERRSTR   "      Freeform data value count mismatch"

#define PARSE71_ERRSTR   "      Resource groups not allowed with freeform data"
#define PARSE72_ERRSTR   "      Invalid PORT statement STEP or COUNT value"
#define PARSE73_ERRSTR   "      Invalid STEP value"
#define PARSE74_ERRSTR   "      Memory values must be multiples of 1K (400h)."
#define PARSE75_ERRSTR   "      Address values must be multiples of 256 (100h)."
#define PARSE76_ERRSTR   "    Error at line number %u:"
#define PARSE77_ERRSTR   "    Could not open this CFG file\n"
#define PARSE78_ERRSTR   "CFG file [%s] is illegal in this context.\n\
Already have a system board specified."




/****+++***********************************************************************
*
* Function:     err_add_string_parm()
*
* Parameters:   parm_num	which global string to set up
*		parm		the string to copy in
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    This function is used to set up the global string parameters. These
*    parameters will be stuffed into a message when err_handler() is next
*    called.
*
****+++***********************************************************************/

void err_add_string_parm(parm_num, parm)
    int		parm_num;
    char	*parm;
{

    if ( (parm_num > 0)   &&   (parm_num <= NUM_STRING_PARMS) )  {

	switch (parm_num)  {
	    case  1:	(void)strcpy(str_parm1, parm);
	    case  2:	(void)strcpy(str_parm2, parm);
	    case  3:	(void)strcpy(str_parm3, parm);
	    case  4:	(void)strcpy(str_parm4, parm);
	    case  5:	(void)strcpy(str_parm5, parm);
	}

    }

}




/****+++***********************************************************************
*
* Function:     err_add_num_parm()
*
* Parameters:   parm_num	which global num to set up
*		parm		the number to copy in
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    This function is used to set up the global num parameters. These
*    parameters will be stuffed into a message when err_handler() is next
*    called.
*
****+++***********************************************************************/

void err_add_num_parm(parm_num, parm)
    int		parm_num;
    unsigned	parm;
{

    if ( (parm_num > 0)   &&   (parm_num <= NUM_STRING_PARMS) )  {

	switch (parm_num)  {
	    case  1:	num_parm1 = parm;
	    case  2:	num_parm2 = parm;
	    case  3:	num_parm3 = parm;
	    case  4:	num_parm4 = parm;
	    case  5:	num_parm5 = parm;
	}

    }

}


/****+++***********************************************************************
*
* Function:     err_handler()
*
* Parameters:   err_code		which error is this?
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    This function handles errors. It may take different approaches depending on
*    what mode we are in. For example, in interactive mode, it will display
*    a particular message (or messages) for a given err_code. In command-line
*    mode, it can do something else. This gets all of the policy for handling
*    errors in one place (here) and lets the rest of the code be cleaner.
*
****+++***********************************************************************/

void err_handler(err_code)
    int		err_code;
{
    char	buffer[601];
    
    /***************
    * If we are doing stuff for SAM (-Y or -Z), don't bother putting
    * out any error messages.
    ***************/
    if ( (program_mode.slotnum) || (program_mode.id) )
	return;

    switch (err_code) {

	case PARSE0_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, PARSE0_ERRSTR, str_parm1, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case PARSE1_ERRCODE:
	    pr_put_text(PARSE1_ERRSTR);
	    break;

	case PARSE2_ERRCODE:
	    pr_put_text(PARSE2_ERRSTR);
	    break;

	case PARSE3_ERRCODE:
	    pr_put_text(PARSE3_ERRSTR);
	    break;

	case PARSE4_ERRCODE:
	    pr_put_text(PARSE4_ERRSTR);
	    break;

	case PARSE5_ERRCODE:
	    pr_put_text(PARSE5_ERRSTR);
	    break;

	case PARSE6_ERRCODE:
	    pr_put_text(PARSE6_ERRSTR);
	    break;

	case PARSE7_ERRCODE:
	    pr_put_text(PARSE7_ERRSTR);
	    break;

	case PARSE8_ERRCODE:
	    pr_put_text(PARSE8_ERRSTR);
	    break;

	case PARSE9_ERRCODE:
	    pr_put_text(PARSE9_ERRSTR);
	    break;

	case PARSE10_ERRCODE:
	    pr_put_text(PARSE10_ERRSTR);
	    break;

	case PARSE11_ERRCODE:
	    pr_put_text(PARSE11_ERRSTR);
	    break;

	case PARSE13_ERRCODE:
	    pr_put_text(PARSE13_ERRSTR);
	    break;

	case PARSE14_ERRCODE:
	    pr_put_text(PARSE14_ERRSTR);
	    break;

	case PARSE16_ERRCODE:
	    pr_put_text(PARSE16_ERRSTR);
	    break;

	case PARSE17_ERRCODE:
	    pr_put_text(PARSE17_ERRSTR);
	    break;

	case PARSE18_ERRCODE:
	    pr_put_text(PARSE18_ERRSTR);
	    break;

	case PARSE19_ERRCODE:
	    pr_put_text(PARSE19_ERRSTR);
	    break;

	case PARSE20_ERRCODE:
	    pr_put_text(PARSE20_ERRSTR);
	    break;

	case PARSE21_ERRCODE:
	    pr_put_text(PARSE21_ERRSTR);
	    break;

	case PARSE22_ERRCODE:
	    pr_put_text(PARSE22_ERRSTR);
	    break;

	case PARSE23_ERRCODE:
	    pr_put_text(PARSE23_ERRSTR);
	    break;

	case PARSE24_ERRCODE:
	    pr_put_text(PARSE24_ERRSTR);
	    break;

	case PARSE25_ERRCODE:
	    (void)sprintf(buffer, PARSE25_ERRSTR, str_parm2);
	    pr_put_text(buffer);
	    break;

	case PARSE26_ERRCODE:
	    pr_put_text(PARSE26_ERRSTR);
	    break;

	case PARSE27_ERRCODE:
	    pr_put_text(PARSE27_ERRSTR);
	    break;

	case PARSE28_ERRCODE:
	    pr_put_text(PARSE28_ERRSTR);
	    break;

	case PARSE29_ERRCODE:
	    pr_put_text(PARSE29_ERRSTR);
	    break;

	case PARSE30_ERRCODE:
	    pr_put_text(PARSE30_ERRSTR);
	    break;

	case PARSE31_ERRCODE:
	    pr_put_text(PARSE31_ERRSTR);
	    break;

	case PARSE32_ERRCODE:
	    pr_put_text(PARSE32_ERRSTR);
	    break;

	case PARSE34_ERRCODE:
	    pr_put_text(PARSE34_ERRSTR);
	    break;

	case PARSE35_ERRCODE:
	    pr_put_text(PARSE35_ERRSTR);
	    break;

	case PARSE36_ERRCODE:
	    pr_put_text(PARSE36_ERRSTR);
	    break;

	case PARSE37_ERRCODE:
	    pr_put_text(PARSE37_ERRSTR);
	    break;

	case PARSE38_ERRCODE:
	    pr_put_text(PARSE38_ERRSTR);
	    break;

	case PARSE39_ERRCODE:
	    pr_put_text(PARSE39_ERRSTR);
	    break;

	case PARSE40_ERRCODE:
	    pr_put_text(PARSE40_ERRSTR);
	    break;

	case PARSE41_ERRCODE:
	    pr_put_text(PARSE41_ERRSTR);
	    break;

	case PARSE42_ERRCODE:
	    pr_put_text(PARSE42_ERRSTR);
	    break;

	case PARSE43_ERRCODE:
	    pr_put_text(PARSE43_ERRSTR);
	    break;

	case PARSE44_ERRCODE:
	    pr_put_text(PARSE44_ERRSTR);
	    break;

	case PARSE46_ERRCODE:
	    pr_put_text(PARSE46_ERRSTR);
	    break;

	case PARSE47_ERRCODE:
	    pr_put_text(PARSE47_ERRSTR);
	    break;

	case PARSE48_ERRCODE:
	    pr_put_text(PARSE48_ERRSTR);
	    break;

	case PARSE51_ERRCODE:
	    pr_put_text(PARSE51_ERRSTR);
	    break;

	case PARSE52_ERRCODE:
	    pr_put_text(PARSE52_ERRSTR);
	    break;

	case PARSE53_ERRCODE:
	    pr_put_text(PARSE53_ERRSTR);
	    break;

	case PARSE54_ERRCODE:
	    pr_put_text(PARSE54_ERRSTR);
	    break;

	case PARSE55_ERRCODE:
	    (void)sprintf(buffer, PARSE55_ERRSTR, str_parm2);
	    pr_put_text(buffer);
	    break;

	case PARSE56_ERRCODE:
	    pr_put_text(PARSE56_ERRSTR);
	    break;

	case PARSE57_ERRCODE:
	    pr_put_text(PARSE57_ERRSTR);
	    break;

	case PARSE58_ERRCODE:
	    pr_put_text(PARSE58_ERRSTR);
	    break;

	case PARSE59_ERRCODE:
	    pr_put_text(PARSE59_ERRSTR);
	    break;

	case PARSE60_ERRCODE:
	    pr_put_text(PARSE60_ERRSTR);
	    break;

	case PARSE61_ERRCODE:
	    pr_put_text(PARSE61_ERRSTR);
	    break;

	case PARSE63_ERRCODE:
	    pr_put_text(PARSE63_ERRSTR);
	    break;

	case PARSE64_ERRCODE:
	    pr_put_text(PARSE64_ERRSTR);
	    break;

	case PARSE65_ERRCODE:
	    pr_put_text(PARSE65_ERRSTR);
	    break;

	case PARSE66_ERRCODE:
	    pr_put_text(PARSE66_ERRSTR);
	    break;

	case PARSE67_ERRCODE:
	    pr_put_text(PARSE67_ERRSTR);
	    break;

	case PARSE68_ERRCODE:
	    pr_put_text(PARSE68_ERRSTR);
	    break;

	case PARSE69_ERRCODE:
	    pr_put_text(PARSE69_ERRSTR);
	    break;

	case PARSE70_ERRCODE:
	    pr_put_text(PARSE70_ERRSTR);
	    break;

	case PARSE71_ERRCODE:
	    pr_put_text(PARSE71_ERRSTR);
	    break;

	case PARSE72_ERRCODE:
	    pr_put_text(PARSE72_ERRSTR);
	    break;

	case PARSE73_ERRCODE:
	    pr_put_text(PARSE73_ERRSTR);
	    break;

	case PARSE74_ERRCODE:
	    pr_put_text(PARSE74_ERRSTR);
	    break;

	case PARSE75_ERRCODE:
	    pr_put_text(PARSE75_ERRSTR);
	    break;

	case PARSE76_ERRCODE:
	    (void)sprintf(buffer, PARSE76_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;

	case PARSE77_ERRCODE:
	    pr_put_text(PARSE77_ERRSTR);
	    break;

	case PARSE78_ERRCODE:
	    (void)sprintf(buffer, PARSE78_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case NO_CFG_FILE_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, COULD_NOT_OPEN_CFG_ERRSTR, str_parm1,
			  CFG_FILES_DIR);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case ILLEGAL_INCLUDE_ERRCODE:
	    (void)sprintf(buffer, ILLEGAL_INCLUDE_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case CANNOT_OPEN_CFG_DIR_ERRCODE:
	    (void)sprintf(buffer, CANNOT_OPEN_CFG_DIR_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case NO_CFG_FILES_ERRCODE:
	    (void)sprintf(buffer, NO_CFG_FILES_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case NO_CFG_FILES_OF_THIS_TYPE_ERRCODE:
	    (void)sprintf(buffer, NO_CFG_FILES_OF_THIS_TYPE_ERRSTR, str_parm1,
			  str_parm2);
	    pr_put_text(buffer);
	    break;

	case NO_SCI_FILE_ERRCODE:
	    (void)sprintf(buffer, COULD_NOT_OPEN_SCI_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case BAD_SCI_FILE_ERRCODE:
	    (void)sprintf(buffer, BAD_SCI_FILE_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case INVALID_SYSBRD_ERRCODE:
	    pr_put_text(INVALID_SYSBRD_ERRSTR);
	    break;

	case CANT_WRITE_EEPROM_ERRCODE:
	    pr_init_more();
	    pr_put_text(CANT_WRITE_EEPROM_ERRSTR);
	    pr_end_more();
	    break;

	case CANT_WRITE_SCI_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, CANT_WRITE_SCI_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case EEPROM_IN_USE_ERRCODE:
	    if (program_mode.automatic == 1) {
		pr_init_more();
		pr_put_text(AUTO_EEPROM_IN_USE_ERRSTR);
		pr_end_more();
	    }
	    else
		pr_put_text(EEPROM_IN_USE_ERRSTR);
	    break;

	case INVALID_EEPROM_ERRCODE:
	    pr_init_more();
	    pr_put_text(INVALID_EEPROM_ERRSTR);
	    pr_end_more();
	    break;

        case EEPROM_AND_SCI_UPDATE_OK_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, EEPROM_AND_SCI_UPDATE_OK_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

        case SCI_UPDATE_OK_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, SCI_UPDATE_OK_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

        case EEPROM_UPDATE_OK_ERRCODE:
	    pr_init_more();
	    pr_put_text(EEPROM_UPDATE_OK_ERRSTR);
	    pr_end_more();
	    break;

        case EEPROM_AND_CFG_MISMATCH_ERRCODE:
	    pr_init_more();
	    if (program_mode.automatic == 1)
		(void)sprintf(buffer, AUTO_EEPROM_AND_CFG_MISMATCH_ERRSTR,
			      str_parm1, num_parm1);
	    else
		(void)sprintf(buffer, EEPROM_AND_CFG_MISMATCH_ERRSTR, num_parm1,
			      str_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case TOO_MANY_IRQS_ERRCODE:
	    (void)sprintf(buffer, TOO_MANY_IRQS_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_put_text(BOARD_NOT_SAVED_ERRSTR);
	    break;

	case TOO_MANY_DMAS_ERRCODE:
	    (void)sprintf(buffer, TOO_MANY_DMAS_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_put_text(BOARD_NOT_SAVED_ERRSTR);
	    break;

	case TOO_MANY_PORTS_ERRCODE:
	    (void)sprintf(buffer, TOO_MANY_PORTS_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_put_text(BOARD_NOT_SAVED_ERRSTR);
	    break;

	case TOO_MANY_MEMORYS_ERRCODE:
	    (void)sprintf(buffer, TOO_MANY_MEMORYS_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_put_text(BOARD_NOT_SAVED_ERRSTR);
	    break;

	case SYSBRD_ID_CHANGED_ERRCODE:
	    pr_init_more();
	    if (program_mode.automatic == 1)
		pr_put_text(AUTO_SYSBRD_ID_CHANGED_ERRSTR);
	    else
		pr_put_text(SYSBRD_ID_CHANGED_ERRSTR);
	    pr_end_more();
	    break;

	case NO_EISA_HW_PRESENT_ERRCODE:
	    pr_init_more();
	    pr_put_text(NO_EISA_HW_PRESENT_ERRSTR);
	    pr_end_more();
	    break;

	case NO_EEPROM_DRIVER_ERRCODE:
	    pr_init_more();
	    if (program_mode.automatic == 1)
		pr_put_text(AUTO_NO_EEPROM_DRIVER_ERRSTR);
	    else
		pr_put_text(NO_EEPROM_DRIVER_ERRSTR);
	    pr_end_more();
	    break;

	case SYSBRD_CFG_FILE_DIFF_ERRCODE:
	    pr_init_more();
	    if (program_mode.automatic == 1)
		(void)sprintf(buffer, AUTO_SYSBRD_CFG_FILE_DIFF_ERRSTR, str_parm1);
	    else
		(void)sprintf(buffer, SYSBRD_CFG_FILE_DIFF_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case INVALID_EEPROM_READ_ERRCODE:
	    pr_init_more();
	    if (program_mode.automatic == 1)
		pr_put_text(AUTO_INVALID_EEPROM_READ_ERRSTR);
	    else
		pr_put_text(INVALID_EEPROM_READ_ERRSTR);
	    pr_end_more();
	    break;

	case TOO_MANY_INITS_ERRCODE:
	    (void)sprintf(buffer, TOO_MANY_INITS_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_put_text(BOARD_NOT_SAVED_ERRSTR);
	    break;

	case INTERACTIVE_HEADER1_ERRCODE:
	    pr_put_text(INTERACTIVE_HEADER1_ERRSTR);
	    break;

	case INTERACTIVE_INVALID_CMD_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
	    pr_put_text(TYPE_HELP_ERRSTR);
	    pr_end_more();
	    break;

	case INTERACTIVE_BASIC_HELP_ERRCODE:
	    pr_init_more();
	    pr_put_text(HELP_COMMENT_ERRSTR);
	    pr_put_text(HELP_HELP_ERRSTR);
            pr_put_text(HELP_ADD_ERRSTR); 
            pr_put_text(HELP_REMOVE_ERRSTR); 
            pr_put_text(HELP_MOVE_ERRSTR); 
            pr_put_text(HELP_CHG_ERRSTR); 
            pr_put_text(HELP_SAVE_ERRSTR); 
            pr_put_text(HELP_INIT_ERRSTR); 
            pr_put_text(HELP_QUIT_ERRSTR); 
	    pr_put_text(HELP_SHOW_ERRSTR);
	    pr_put_text(HELP_SHOW_SLOT_ERRSTR);
	    pr_put_text(HELP_SHOW_BOARD_ERRSTR);
	    pr_put_text(HELP_SHOW_SWITCH_ERRSTR);
	    pr_put_text(HELP_CFG_ERRSTR);
	    pr_end_more();
	    break;

	case INVALID_COMMENT_CMD_ERRCODE:
	    pr_init_more();
	    pr_put_text(INVALID_COMMENT_CMD_ERRSTR);
	    pr_put_text(HELP_COMMENT_ERRSTR);
	    pr_end_more();
	    break;

	case INTERACTIVE_PROMPT_ERRCODE:
	    (void)fprintf(stderr, INTERACTIVE_PROMPT_ERRSTR);
	    break;

	case RUNSTRING_INV_CMD_ERRCODE:
	    pr_put_text(RUNSTRING_HELP_ERRSTR);
	    break;

	case CHECK_CFG_HDR_ERRCODE:
	    (void)sprintf(buffer, CHECK_CFG_HDR_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case INVALID_SHOW_CMD_ERRCODE:
	    pr_init_more();
	    pr_put_text(INVALID_SHOW_CMD_ERRSTR);
	    pr_put_text(HELP_SHOW_ERRSTR);
	    pr_put_text(HELP_SHOW_SLOT_ERRSTR);
	    pr_put_text(HELP_SHOW_BOARD_ERRSTR);
	    pr_put_text(HELP_SHOW_SWITCH_ERRSTR);
	    pr_end_more();
	    break;

	case NO_OTHER_OPTIONS_WITH_CHECK_ERRCODE:
	    pr_put_text(NO_OTHER_OPTIONS_WITH_CHECK_ERRSTR);
	    break;

	case INVALID_HELP_CMD_ERRCODE:
	    pr_init_more();
	    pr_put_text(INVALID_HELP_CMD_ERRSTR);
	    pr_put_text(HELP_HELP_ERRSTR);
	    pr_end_more();
	    break;

	case BLANK_LINE_ERRCODE:
	    pr_put_text("");
	    break;

	case INVALID_ADD_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_ADD_ERRSTR); 
	    pr_end_more();
	    break;

	case INVALID_REMOVE_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_REMOVE_ERRSTR); 
	    pr_end_more();
	    break;

	case INVALID_MOVE_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_MOVE_ERRSTR); 
	    pr_end_more();
	    break;

	case INVALID_SAVE_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_SAVE_ERRSTR); 
	    pr_end_more();
	    break;

	case INVALID_INIT_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_INIT_ERRSTR); 
	    pr_end_more();
	    break;

	case INVALID_EXIT_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_QUIT_ERRSTR); 
	    pr_end_more();
	    break;

	case INVALID_CFGCMD_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_CFG_ERRSTR); 
	    pr_end_more();
	    break;

	case INTERACTIVE_HEADER2_ERRCODE:
	    pr_put_text(INTERACTIVE_HEADER2_ERRSTR);
	    break;

	case INVALID_CHG_ERRCODE:
	    pr_init_more();
	    pr_put_text(INTERACTIVE_INVALID_CMD_ERRSTR);
            pr_put_text(HELP_CHG_ERRSTR); 
	    pr_end_more();
	    break;

	case NO_OTHER_OPTIONS_WITH_AUTO_ERRCODE:
	    pr_put_text(NO_OTHER_OPTIONS_WITH_AUTO_ERRSTR);
	    break;

	case NO_AVAIL_SLOTS_ERRCODE:
	    pr_put_text(NO_AVAIL_SLOTS_ERRSTR);
	    break;

	case SLOT_INCOMPATIBLE_ERRCODE:
	    (void)sprintf(buffer, SLOT_INCOMPATIBLE_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;

	case BOARD_NEEDS_TAG_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, BOARD_NEEDS_TAG_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    if (*str_parm1 != 0) {
		(void)sprintf(buffer, MATCHING_TAGS_ERRSTR, str_parm1);
		pr_put_text(buffer);
	    }
	    pr_end_more();
	    break;

	case BOARD_NEEDS_BUSMASTER_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, BOARD_NEEDS_BUSMASTER_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

#ifdef VIRT_BOARD
	case INV_EMB_BOARD_ERRCODE:
	    pr_put_text(INV_EMB_BOARD_ERRSTR);
	    break;

	case EMB_SLOT_OCCUPIED_ERRCODE:
	    (void)sprintf(buffer, EMB_SLOT_OCCUPIED_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;
#endif

	case SLOT_OCCUPIED_ERRCODE:
	    (void)sprintf(buffer, SLOT_OCCUPIED_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;

	case EMPTY_SLOT_ERRCODE:
	    (void)sprintf(buffer, EMPTY_SLOT_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;

	case NONEXISTENT_SLOT_ERRCODE:
	    (void)sprintf(buffer, NONEXISTENT_SLOT_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;

	case NO_MORE_MEMORY_ERRCODE:
	    pr_put_text(NO_MORE_MEMORY_ERRSTR);
	    break;

	case AMP_OVERLOAD_DETECTED_ERRCODE:
	    (void)sprintf(buffer, AMP_OVERLOAD_DETECTED_ERRSTR, num_parm1,
			  num_parm2);
	    pr_put_text(buffer);
	    break;

	case AMP_OVERLOAD_FIXED_ERRCODE:
	    pr_put_text(AMP_OVERLOAD_FIXED_ERRSTR);
	    break;

	case NOT_CONFIGURED_ERRCODE:
	    pr_put_text(NOT_CONFIGURED_ERRSTR);
	    break;

	case NO_SWITCH_CHANGE_IN_SLOT_ERRCODE:
	    pr_put_text(NO_SWITCH_CHANGE_IN_SLOT_ERRSTR);
	    break;

	case NO_SWITCH_CHANGE_ERRCODE:
	    pr_put_text(NO_SWITCH_CHANGE_ERRSTR);
	    break;

	case NO_SWITCHES_IN_SLOT_ERRCODE:
	    pr_put_text(NO_SWITCHES_IN_SLOT_ERRSTR);
	    break;

	case NO_SWITCHES_ERRCODE:
	    pr_put_text(NO_SWITCHES_ERRSTR);
	    break;

	case AUTO_SWITCH_ERRCODE:
	    pr_put_text(AUTO_SWITCH_ERRSTR);
	    break;

	case AUTO_SWITCH_PROMPT_ERRCODE:
	    (void)fprintf(stderr, AUTO_SWITCH_PROMPT_ERRSTR);
	    break;

	case AUTO_SWITCH_HALT_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_SWITCH_HALT_ERRSTR);
	    pr_end_more();
	    break;

	case REMOVE_EMBEDDED_BRD_ERRCODE:
	    (void)sprintf(buffer, REMOVE_EMBEDDED_BRD_ERRSTR, str_parm1,
			  num_parm1);
	    pr_put_text(buffer);
	    break;

	case REMOVED_BOARD_ERRCODE:
	    (void)sprintf(buffer, REMOVED_BOARD_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case MOVE_EMBEDDED_BRD_ERRCODE:
	    (void)sprintf(buffer, MOVE_EMBEDDED_BRD_ERRSTR, str_parm1, num_parm1);
	    pr_put_text(buffer);
	    break;

#ifdef VIRT_BOARD
	case MOVE_VIRTUAL_BRD_ERRCODE:
	    (void)sprintf(buffer, MOVE_VIRTUAL_BRD_ERRSTR, str_parm1, num_parm1);
	    pr_put_text(buffer);
	    break;
#endif

	case BAD_MOVE_SLOT_ERRCODE:
	    (void)sprintf(buffer, BAD_MOVE_SLOT_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case MOVE_WORKED_ERRCODE:
	    (void)sprintf(buffer, MOVE_WORKED_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case VALID_SLOT_LIST_ERRCODE:
	    (void)sprintf(buffer, VALID_SLOT_LIST_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    break;

	case EXIT_NO_CHANGES_EVER_ERRCODE:
	    pr_put_text(EXIT_NO_CHANGES_EVER_ERRSTR);
	    pr_put_text(LEAVING_ERRSTR);
	    break;

	case EXIT_NO_NEW_CHANGES_ERRCODE:
	    (void)sprintf(buffer, EXIT_NO_NEW_CHANGES_ERRSTR);
	    pr_put_text(buffer);
	    break;

	case EXIT_TODO_AFTER_EXIT_ERRCODE:
	    pr_put_text(EXIT_TODO_AFTER_EXIT_ERRSTR);
	    break;

	case EXIT_NEW_CHANGES_ERRCODE:
	    (void)fprintf(stderr, EXIT_NEW_CHANGES_ERRSTR);
	    break;

	case EXIT_WITHOUT_SAVING_ERRCODE:
	    pr_put_text(EXIT_WITHOUT_SAVING_ERRSTR);
	    pr_put_text(LEAVING_ERRSTR);
	    break;

	case EXIT_ABORT_EXIT_ERRCODE:
	    pr_put_text(EXIT_ABORT_EXIT_ERRSTR);
	    break;

	case EXIT_LEAVING_ERRCODE:
	    (void)sprintf(buffer, LEAVING_ERRSTR);
	    pr_put_text(buffer);
	    break;

	case INIT_WILL_DESTROY_CHANGES_ERRCODE:
	    (void)fprintf(stderr, INIT_WILL_DESTROY_CHANGES_ERRSTR);
	    break;

	case INIT_WORKED_ERRCODE:
	    pr_put_text(INIT_WORKED_ERRSTR);
	    break;

	case INIT_ABORTING_ERRCODE:
	    pr_put_text(INIT_ABORTING_ERRSTR);
	    break;

	case INIT_FAILED_ERRCODE:
	    pr_put_text(INIT_FAILED_ERRSTR);
	    pr_put_text(LEAVING_ERRSTR);
	    break;

	case HELP_ADD_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_ADD_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_REMOVE_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_REMOVE_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_MOVE_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_MOVE_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_SAVE_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_SAVE_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_INIT_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_INIT_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_EXIT_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_QUIT_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_SHOW_ERRCODE:
	    pr_init_more();
	    pr_put_text(HELP_SHOW_ERRSTR);
	    pr_put_text(HELP_SHOW_SLOT_ERRSTR);
	    pr_put_text(HELP_SHOW_BOARD_ERRSTR);
	    pr_put_text(HELP_SHOW_SWITCH_ERRSTR);
	    pr_end_more();
	    break;

	case HELP_COMMENT_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_COMMENT_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_CFG_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_CFG_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_HELP_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_HELP_ERRSTR); 
	    pr_end_more();
	    break;

	case HELP_CHG_ERRCODE:
	    pr_init_more();
            pr_put_text(HELP_CHG_ERRSTR); 
	    pr_end_more();
	    break;

	case FUNCTION_INVALID1_ERRCODE:
	    (void)sprintf(buffer, FUNCTION_INVALID1_ERRSTR, num_parm1,
			  num_parm4, num_parm1);
	    pr_put_text(buffer);
	    break;

	case FUNCTION_INVALID2_ERRCODE:
	    (void)sprintf(buffer, FUNCTION_INVALID2_ERRSTR, num_parm1,
			  num_parm1);
	    pr_put_text(buffer);
	    break;

	case NO_VALID_FUNCTIONS_ERRCODE:
	    (void)sprintf(buffer, NO_VALID_FUNCTIONS_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    break;

	case CHOICE_INVALID_ERRCODE:
	    (void)sprintf(buffer, CHOICE_INVALID_ERRSTR, num_parm2, num_parm1,
			  num_parm4, num_parm1);
	    pr_put_text(buffer);
	    break;

	case CHOICE_THE_SAME_ERRCODE:
	    (void)sprintf(buffer, CHOICE_THE_SAME_ERRSTR, num_parm2, num_parm1,
			  num_parm3, num_parm1);
	    pr_put_text(buffer);
	    break;

	case CHANGE_WORKED_ERRCODE:
	    (void)sprintf(buffer, CHANGE_WORKED_ERRSTR, num_parm2, 
			  num_parm1, num_parm3);
	    pr_put_text(buffer);
	    break;

	case CHANGE_FAILED_ERRCODE:
	    pr_put_text(CHANGE_FAILED_ERRSTR);
	    break;

	case IODC_BAD_OPTION_ERRCODE:
	    pr_put_text(IODC_BAD_OPTION_ERRSTR);
	    break;

	case IODC_SUCCESSFUL_ERRCODE:
	    pr_put_text(IODC_SUCCESSFUL_ERRSTR);
	    break;

	case IODC_BAD_SLOT_ERRCODE:
	    pr_put_text(IODC_BAD_SLOT_ERRSTR);
	    break;

	case IODC_WRITE_PROBLEM_ERRCODE:
	    pr_put_text(IODC_WRITE_PROBLEM_ERRSTR);
	    break;

	case BOARD_NOT_ADDED_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, BOARD_NOT_ADDED_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case EXPLICIT_ADD_WORKED_ERRCODE:
	    (void)sprintf(buffer, EXPLICIT_ADD_WORKED_ERRSTR, str_parm1);
	    pr_put_text(buffer);
	    if (str_parm2[0] != 0) {
		pr_print_image_nolf(COMMENTS_ERRSTR);
		pr_put_text_fancy(15, str_parm2, 1000, 
			    (int)(MAXCOL-strlen(COMMENTS_ERRSTR)), 1);
	    }
	    pr_print_image("");
	    pr_print_image("");
	    break;

	case AUTO_MODE_HEADER_ERRCODE:
	    {
	    time_t	t;
	    t = time((time_t *)NULL);
	    (void)strftime(str_parm5, 50, "%x    %H:%M:%S", localtime(&t));
	    }
	    (void)sprintf(buffer, AUTO_MODE_HEADER_ERRSTR, str_parm5);
	    pr_put_text(buffer);
	    break;

	case AUTO_MODE_TRAILER_ERRCODE:
	    pr_put_text(AUTO_MODE_TRAILER_ERRSTR);
	    (void)fprintf(print_err_file_fd, AUTO_MODE_TRAILER_ERRSTR);
	    break;

	case AUTO_BOARDS_REMOVED_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_BOARDS_REMOVED_ERRSTR);
	    pr_end_more();
	    break;

	case AUTO_BOARD_REMOVED_SLOT_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, AUTO_BOARD_REMOVED_SLOT_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_CANT_ADD_EXISTING_BOARD_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer,AUTO_CANT_ADD_EXISTING_BOARD_ERRSTR,num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_RESOURCE_CONFLICT_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, AUTO_RESOURCE_CONFLICT_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_CANT_ADD_NEW_BOARD_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, AUTO_CANT_ADD_NEW_BOARD_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_ADD_OF_NEW_BOARD_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, AUTO_ADD_OF_NEW_BOARD_ERRSTR, num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_RESTORE_FROM_SCI_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_RESTORE_FROM_SCI_ERRSTR);
	    pr_end_more();
	    break;

	case AUTO_TRYING_REINIT_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_TRYING_REINIT_ERRSTR);
	    pr_end_more();
	    break;

	case AUTO_MUST_REBOOT_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_MUST_REBOOT_ERRSTR);
	    pr_end_more();
	    break;

	case AUTO_BOOT_BOARD_NOT_ADDED_1_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, AUTO_BOOT_BOARD_NOT_ADDED_1_ERRSTR,str_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_BOOT_BOARD_NOT_ADDED_2_ERRCODE:
	    pr_init_more();
	    (void)sprintf(buffer, AUTO_BOOT_BOARD_NOT_ADDED_2_ERRSTR,num_parm1);
	    pr_put_text(buffer);
	    pr_end_more();
	    break;

	case AUTO_FIRST_EISA_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_FIRST_EISA_ERRSTR);
	    pr_end_more();
	    break;

	case AUTO_FROM_SCRATCH_ERRCODE:
	    pr_init_more();
	    pr_put_text(AUTO_FROM_SCRATCH_ERRSTR);
	    pr_end_more();
	    break;


#ifdef DEBUG
	case DEBUG_FREE_FAILS_ERRCODE:
	    pr_put_text(DEBUG_FREE_FAILS_ERRSTR);
	    break;

	case DEBUG_ALLOC_FAILS_ERRCODE:
	    pr_put_text(DEBUG_ALLOC_FAILS_ERRSTR);
	    break;
#endif

    }

}
