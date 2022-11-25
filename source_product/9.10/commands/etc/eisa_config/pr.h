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
*                         inc/pr.h
*
*      These are some defines used for modules which use pr.c functions.
*
*******************************************************************************/


/************
* Function externs for pr.c entry points
************/
extern void             pr_break_and_buffer_line();
extern void             pr_put_text();
extern void             pr_put_text_fancy();
extern void             pr_clear_image();
extern void             pr_put_string();
extern void             pr_print_image();
extern void             pr_print_image_nolf();
extern void		pr_init_more();
extern void		pr_end_more();
extern void		pr_init_file_print();
extern void		pr_end_file_print();


#define MAXCOL			78
#define L_MARGIN                1
#define R_MARGIN                2
typedef char 			image_buf[MAXCOL+2];


/* interactive commands (for show and comments) */
#define CMD_KEY_CHANGED         "changed"
#define CMD_KEY_BOARD           "board"
#define CMD_KEY_BOARDS          "boards"
#define CMD_KEY_RESOURCE        "resource"
#define CMD_KEY_RESOURCES       "resources"
#define CMD_KEY_FUNCTION        "function"
#define CMD_KEY_FUNCTIONS       "functions"
#define CMD_KEY_CHOICE          "choice"
#define CMD_KEY_CHOICES         "choices"
#define CMD_KEY_SWITCH          "switch"
#define CMD_KEY_SWITCHES1       "switches"
#define CMD_KEY_SWITCHES2       "switchs"
#define CMD_KEY_SLOT       	"slot"
#define CMD_KEY_SLOTS      	"slots"


/* modes used to call show_cfg() */
#define	CFG_TYPES		1
#define CFG_ALL_FILES		2
#define CFG_SOME_FILES		3
