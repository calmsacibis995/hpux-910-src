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
*******************************************************************************
*
*			inc/err.h
*
*******************************************************************************/


/****************
* Error handling functions.
****************/
extern void            err_handler();
extern void            err_add_string_parm();
extern void            err_add_num_parm();


/**************
* Exit codes. The following parties depend on these codes:
*	o   the install program
*	o   SAM
*	o   the /etc/bcheckrc script
**************/
#define EXIT_OK_NO_REBOOT	0
	/* The configuration has not changed. No reboot is necessary.
	   Alternatively, we have added a new board and initialized it without
	   needing a reboot. */

#define EXIT_OK_REBOOT			1
	/* The configuration has been changed without any problems.
	   The system must be rebooted. */

#define EXIT_CHECKCFG_PROBLEM		2
	/* An error was detected while checking a CFG file for correctness. */

#define EXIT_RUNSTRING_ERR		3
	/* An invalid runstring option was specified. */

#define EXIT_INIT_ERR			4
	/* An initialization error was detected in interactive mode. */

#define EXIT_RESOURCE_ERR		5
	/* Could not get enough memory. */

#define EXIT_EEPROM_BUSY		6
	/* The /dev/eeprom driver is already in use by another process. */

#define EXIT_EEPROM_ERROR		7
	/* There is something very wrong with the EEPROM.
	   This is most likely a hardware problem. */

#define EXIT_EXISTING_BOARD_PROBLEM 	8
	/* There is a problem in adding an existing board back into the
	   configuration. The EEPROM has not been changed. */

#define EXIT_PROBLEMS_NO_REBOOT 	9
	/* Automatic mode ran into a problem while trying to add a new board.
	   The EEPROM has not been changed. */

#define EXIT_PARTIAL_REBOOT 		10
	/* Automatic mode ran into a problem while trying to add a new baord.
	   However, at least one other board was successfully added.
	   The EEPROM has been changed. The system must be rebooted. */

#define EXIT_PARTIAL_REBOOT_HALT 	11
	/* Automatic mode ran into a problem while trying to add a new board.
	   However, at least one other board was successfully added.
	   At least one of the added boards needs a switch change.
	   The EEPROM has been changed. The system must be halted. */

#define EXIT_OK_REBOOT_HALT 		12
	/* One or more boards was successfully added.
	   At least one of the added boards needs a switch change.
	   The EEPROM has been changed. The system must be halted. */

#define EXIT_NO_EEPROM			13
	/* This machine does not have an EISA backplane or the EISA bus adapter
	   is very broken. */

#define EXIT_NO_EEPROM_DRIVER		14
	/* This machine has EISA hardware, but the eeprom driver is not part of
	   the kernel. EISA will be unusable. */

#define EXIT_PARTIAL			15
	/* There was some problem, but at least one new board has been
	   successfully added to the EEPROM.
	   No reboot is required (we were able to re-initialize without it). */

#ifdef DEBUG
#define EXIT_DBG_RESOURCE_ERR	20
#endif

/**************
* Error codes for the parser  - 0 to 99
**************/
#define PARSE0_ERRCODE    		0
#define PARSE1_ERRCODE    		1
#define PARSE2_ERRCODE    		2
#define PARSE3_ERRCODE    		3
#define PARSE4_ERRCODE    		4
#define PARSE5_ERRCODE    		5
#define PARSE6_ERRCODE    		6
#define PARSE7_ERRCODE    		7
#define PARSE8_ERRCODE    		8
#define PARSE9_ERRCODE    		9

#define PARSE10_ERRCODE   		10
#define PARSE11_ERRCODE   		11 
#define PARSE13_ERRCODE   		13
#define PARSE14_ERRCODE   		14
#define PARSE16_ERRCODE   		16
#define PARSE17_ERRCODE   		17
#define PARSE18_ERRCODE   		18
#define PARSE19_ERRCODE   		19

#define PARSE20_ERRCODE   		20
#define PARSE21_ERRCODE   		21
#define PARSE22_ERRCODE   		22
#define PARSE23_ERRCODE   		23
#define PARSE24_ERRCODE   		24
#define PARSE25_ERRCODE   		25
#define PARSE26_ERRCODE   		26
#define PARSE27_ERRCODE   		27
#define PARSE28_ERRCODE   		28
#define PARSE29_ERRCODE   		29

#define PARSE30_ERRCODE   		30
#define PARSE31_ERRCODE   		31
#define PARSE32_ERRCODE   		32
#define PARSE34_ERRCODE   		34
#define PARSE35_ERRCODE   		35
#define PARSE36_ERRCODE   		36
#define PARSE37_ERRCODE   		37
#define PARSE38_ERRCODE   		38
#define PARSE39_ERRCODE   		39

#define PARSE40_ERRCODE   		40
#define PARSE41_ERRCODE   		41
#define PARSE42_ERRCODE   		42
#define PARSE43_ERRCODE   		43
#define PARSE44_ERRCODE   		44
#define PARSE46_ERRCODE   		46
#define PARSE47_ERRCODE   		47
#define PARSE48_ERRCODE   		48

#define PARSE51_ERRCODE   		51
#define PARSE52_ERRCODE   		52
#define PARSE53_ERRCODE   		53
#define PARSE54_ERRCODE   		54
#define PARSE55_ERRCODE   		55
#define PARSE56_ERRCODE   		56
#define PARSE57_ERRCODE   		57
#define PARSE58_ERRCODE   		58
#define PARSE59_ERRCODE   		59

#define PARSE60_ERRCODE   		60
#define PARSE61_ERRCODE   		61
#define PARSE63_ERRCODE   		63
#define PARSE64_ERRCODE   		64
#define PARSE65_ERRCODE   		65
#define PARSE66_ERRCODE   		66
#define PARSE67_ERRCODE   		67
#define PARSE68_ERRCODE   		68
#define PARSE69_ERRCODE   		69

#define PARSE70_ERRCODE   		70
#define PARSE71_ERRCODE   		71
#define PARSE72_ERRCODE   		72
#define PARSE73_ERRCODE   		73
#define PARSE74_ERRCODE   		74
#define PARSE75_ERRCODE   		75
#define PARSE76_ERRCODE			76
#define PARSE77_ERRCODE			77
#define PARSE78_ERRCODE			78



/**************
* CFG handling error codes -- 100 to 119
**************/
#define NO_CFG_FILE_ERRCODE			100
#define ILLEGAL_INCLUDE_ERRCODE			101
#define CANNOT_OPEN_CFG_DIR_ERRCODE		102
#define NO_CFG_FILES_ERRCODE			103
#define NO_CFG_FILES_OF_THIS_TYPE_ERRCODE  	104



/**************
* EEPROM/SCI error codes -- 120 to 149
**************/
#define NO_SCI_FILE_ERRCODE			120
#define BAD_SCI_FILE_ERRCODE			121
#define INVALID_SYSBRD_ERRCODE			123
#define CANT_WRITE_EEPROM_ERRCODE		124
#define CANT_WRITE_SCI_ERRCODE			125
#define CANT_WRITE_EEPROM_OR_SCI_ERRCODE 	126
#define EEPROM_IN_USE_ERRCODE			127
#define INVALID_EEPROM_ERRCODE			129
#define EEPROM_AND_SCI_UPDATE_OK_ERRCODE 	130
#define SCI_UPDATE_OK_ERRCODE			131
#define EEPROM_UPDATE_OK_ERRCODE		132
#define EEPROM_AND_CFG_MISMATCH_ERRCODE		133
#define TOO_MANY_IRQS_ERRCODE			134
#define TOO_MANY_DMAS_ERRCODE			135
#define TOO_MANY_PORTS_ERRCODE			136
#define TOO_MANY_MEMORYS_ERRCODE		137
#define SYSBRD_ID_CHANGED_ERRCODE		138
#define NO_EISA_HW_PRESENT_ERRCODE		139
#define NO_EEPROM_DRIVER_ERRCODE		140
#define SYSBRD_CFG_FILE_DIFF_ERRCODE		141
#define INVALID_EEPROM_READ_ERRCODE		142
#define TOO_MANY_INITS_ERRCODE			143



/**************
* Header and command handling error codes -- 150 to 179
**************/
#define INTERACTIVE_HEADER1_ERRCODE		150
#define INTERACTIVE_INVALID_CMD_ERRCODE 	151
#define INTERACTIVE_BASIC_HELP_ERRCODE  	152
#define INVALID_COMMENT_CMD_ERRCODE		153
#define INTERACTIVE_PROMPT_ERRCODE		154
#define RUNSTRING_INV_CMD_ERRCODE		155
#define CHECK_CFG_HDR_ERRCODE			156
#define INVALID_SHOW_CMD_ERRCODE		157
#define NO_OTHER_OPTIONS_WITH_CHECK_ERRCODE  	158
#define INVALID_HELP_CMD_ERRCODE		159
#define BLANK_LINE_ERRCODE			160
#define INVALID_ADD_ERRCODE			161
#define INVALID_REMOVE_ERRCODE			162
#define INVALID_MOVE_ERRCODE			163
#define INVALID_SAVE_ERRCODE			164
#define INVALID_INIT_ERRCODE			165
#define INVALID_EXIT_ERRCODE			166
#define INVALID_CFGCMD_ERRCODE			167
#define INTERACTIVE_HEADER2_ERRCODE		168
#define INVALID_CHG_ERRCODE			169
#define NO_OTHER_OPTIONS_WITH_AUTO_ERRCODE  	170



/**************
* Slot selection error codes -- 180 to 199
**************/
#define NO_AVAIL_SLOTS_ERRCODE			180
#define SLOT_INCOMPATIBLE_ERRCODE       	181
#define BOARD_NEEDS_TAG_ERRCODE			182
#define BOARD_NEEDS_BUSMASTER_ERRCODE   	183
#ifdef VIRT_BOARD
#define INV_EMB_BOARD_ERRCODE			184
#define EMB_SLOT_OCCUPIED_ERRCODE		185
#endif
#define SLOT_OCCUPIED_ERRCODE			186
#define EMPTY_SLOT_ERRCODE			187
#define NONEXISTENT_SLOT_ERRCODE		188



/**************
* Memory error codes -- 200 to 219
**************/
#define NO_MORE_MEMORY_ERRCODE			200



/**************
* Conflict detection error codes -- 220 to 239
**************/
#define AMP_OVERLOAD_DETECTED_ERRCODE		220
#define AMP_OVERLOAD_FIXED_ERRCODE		221
#define NOT_CONFIGURED_ERRCODE			222



/**************
* Switch error codes -- 240 to 259
**************/
#define NO_SWITCH_CHANGE_IN_SLOT_ERRCODE  	240
#define NO_SWITCH_CHANGE_ERRCODE		241
#define NO_SWITCHES_IN_SLOT_ERRCODE		242
#define NO_SWITCHES_ERRCODE			243
#define AUTO_SWITCH_ERRCODE			244
#define AUTO_SWITCH_PROMPT_ERRCODE		245
#define AUTO_SWITCH_HALT_ERRCODE		246



/**************
* Move and Remove error codes -- 260 to 279
**************/
#define REMOVE_EMBEDDED_BRD_ERRCODE		260
#define REMOVED_BOARD_ERRCODE			261
#define MOVE_EMBEDDED_BRD_ERRCODE		262
#ifdef VIRT_BOARD
#define MOVE_VIRTUAL_BRD_ERRCODE		263
#endif
#define BAD_MOVE_SLOT_ERRCODE			264
#define MOVE_WORKED_ERRCODE			265
#define VALID_SLOT_LIST_ERRCODE			266


/**************
* Exit error codes -- 280 to 299
**************/
#define EXIT_NO_CHANGES_EVER_ERRCODE		280
#define EXIT_NO_NEW_CHANGES_ERRCODE		281
#define EXIT_TODO_AFTER_EXIT_ERRCODE    	282
#define EXIT_NEW_CHANGES_ERRCODE		283
#define EXIT_WITHOUT_SAVING_ERRCODE		284
#define EXIT_ABORT_EXIT_ERRCODE			285
#define EXIT_LEAVING_ERRCODE			286


/**************
* Init error codes -- 300 to 319
**************/
#define INIT_WILL_DESTROY_CHANGES_ERRCODE 	300
#define INIT_WORKED_ERRCODE			301
#define INIT_ABORTING_ERRCODE			302
#define INIT_FAILED_ERRCODE			303


/**************
* Help error codes -- 320 to 339
**************/
#define HELP_ADD_ERRCODE			321
#define HELP_REMOVE_ERRCODE			322
#define HELP_MOVE_ERRCODE			323
#define HELP_SAVE_ERRCODE			324
#define HELP_INIT_ERRCODE			325
#define HELP_EXIT_ERRCODE			326
#define HELP_SHOW_ERRCODE			327
#define HELP_COMMENT_ERRCODE			328
#define HELP_CFG_ERRCODE			329
#define HELP_HELP_ERRCODE			330
#define HELP_CHG_ERRCODE			331


/**************
* Change error codes -- 340 to 359
**************/
#define FUNCTION_INVALID1_ERRCODE		340
#define FUNCTION_INVALID2_ERRCODE		341
#define NO_VALID_FUNCTIONS_ERRCODE		342
#define CHOICE_INVALID_ERRCODE			343
#define CHOICE_THE_SAME_ERRCODE			344
#define CHANGE_WORKED_ERRCODE			345
#define CHANGE_FAILED_ERRCODE			346


/**************
* IODC write errorr codes -- 360 to 379
**************/
#define IODC_BAD_OPTION_ERRCODE			360
#define IODC_SUCCESSFUL_ERRCODE			361
#define IODC_BAD_SLOT_ERRCODE			362
#define IODC_WRITE_PROBLEM_ERRCODE		363


/**************
* Add a board error codes -- 380 to 399
**************/
#define BOARD_NOT_ADDED_ERRCODE			380
#define EXPLICIT_ADD_WORKED_ERRCODE		381



/**************
* Automatic mode error codes -- 400 to 419
**************/
#define AUTO_MODE_HEADER_ERRCODE		400
#define AUTO_MODE_TRAILER_ERRCODE		401
#define AUTO_BOARDS_REMOVED_ERRCODE		402
#define AUTO_BOARD_REMOVED_SLOT_ERRCODE		403
#define AUTO_CANT_ADD_EXISTING_BOARD_ERRCODE	404
#define AUTO_RESOURCE_CONFLICT_ERRCODE 		405
#define AUTO_CANT_ADD_NEW_BOARD_ERRCODE		406
#define AUTO_ADD_OF_NEW_BOARD_ERRCODE		407
#define AUTO_RESTORE_FROM_SCI_ERRCODE		408
#define AUTO_TRYING_REINIT_ERRCODE		409
#define AUTO_MUST_REBOOT_ERRCODE		410
#define AUTO_BOOT_BOARD_NOT_ADDED_1_ERRCODE	411
#define AUTO_BOOT_BOARD_NOT_ADDED_2_ERRCODE	412
#define AUTO_FIRST_EISA_ERRCODE			413
#define AUTO_FROM_SCRATCH_ERRCODE		414




/**************
* General debug error codes  --  500 to 599
**************/
#ifdef DEBUG
#define DEBUG_FREE_FAILS_ERRCODE		500
#define DEBUG_ALLOC_FAILS_ERRCODE		501
#endif
