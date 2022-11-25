/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fmenu.h,v 70.1 92/03/09 16:15:45 ssa Exp $ */
/*
 * fmenu.h
 *
 * Header file for Facet Menu user interface
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved
 *
 */

			/* what type of Facet (if any) is running */

#define NO_FACET	0
#define	FACET_TERM	1
#define FACET_PC	2

extern int Facet_type;

			/* which window is the menu running in */

extern int Menu_window;

			/* menu hot key - default defined in fmenu.c as
			   ^F (for Facet) */

extern char Menu_hot_key[];

			/* Facet/Term hot key */

extern char Ft_hot_key[];

			/* definable constants for menus.
			   Note that most of these are in
			   unsigned char's and therefore
			   cannot be defined greater than
			   256. Others are constrained by
			   screen size. */

#define MAX_MENU_ITEM_NAME	80
#define MAX_MENU_ITEM_ACTION	160
#define MAX_MENU_NBR_ACTIONS	2
#define MAX_MENU_ITEM_TOKENS	10
#define MAX_MENU_LEVEL_NAME	14	/* can't be more than file name size */
#define MAX_MENU_LEVEL_TITLE	80
#define MAX_MENU_LEVEL_ITEMS	20	/* must be at least MAX_FACET_WINDOWS */
#define MAX_MENU_LEVELS		20
#define MAX_MENU_LINE_LENGTH	160
#define MAX_CURRENT_WINDOW_TITLE 40
#define MAX_DIALOG_BOX_ITEMS	10
#define MAX_DIALOG_BOX_PROMPT	80
#define MAX_VARIABLES		10
#define MAX_VARIABLE_LENGTH	80
#define MAX_FACET_WINDOWS	10

			/* dialog box item structure */
struct dialog_item
{
	char prompt[ MAX_DIALOG_BOX_PROMPT + 1 ];
	char *variable;
	unsigned char max_var_len;
	unsigned char being_entered;
	unsigned char action;
	unsigned char loc_row;
	unsigned char prompt_col;
	unsigned char var_col;
};

			/* dialog box item action values */

#define ENTER_STRING		1
#define OK_TO_LEAVE		2
#define CANCEL			3

			/* dialog box structure */

struct dialog_box
{
	int nbr_items;
	int cur_item;
	int loc_row;			/* loc_row and loc_col of -1 means */
	int loc_col;			/* default placement */
	unsigned char size_rows;
	unsigned char size_cols;
	struct dialog_item *items[ MAX_DIALOG_BOX_ITEMS ];
	unsigned char completed;
};

			/* menu item structure */
struct menu_item
{
	char name[ MAX_MENU_ITEM_NAME + 1 ];	/* item name in menu */
	unsigned char row;			/* item row on screen */
	unsigned char col;			/* item column on screen */
	unsigned char type;			/* item types defined below */
	char selection_char;			/* char to select & exec */
						/* what to do when chosen */
	char action[ MAX_MENU_NBR_ACTIONS ][ MAX_MENU_ITEM_ACTION+1 ];
						/* title for new window */
	char new_window_title[ MAX_MENU_ITEM_NAME + 1 ];
	int window_nbr;				/* window to take action in */
	struct dialog_box *dialog_box;		/* pointer to dialog box */
};
			/* menu item types */

#define MENU			1
#define PROGRAM			2
#define INTRINSIC		3
#define FT_COMMAND		4
#define FT_CMD_TO_USER		5
#define MENU_PROGRAM		6

			/* menu level structure */
struct menu_level
{
	unsigned char type;			/* menu types defined below */
	char name[MAX_MENU_LEVEL_NAME+1];	/* name of menu file */
	char title[MAX_MENU_LEVEL_TITLE+1];	/* menu title to display */
	struct menu_level *prev_menu;		/* parent menu if any */
	int loc_row;				/* upper row of menu */
	int loc_col;				/* left col of menu */
	unsigned char size_rows;		/* number of rows occupied */
	unsigned char size_cols;		/* number of cols occupied */
	long options;				/* menu options defined below */
	int nbr_items;				/* number of items in menu */
	int cur_item;				/* pointer to current item */
	int auto_select_item;			/* item to automatically sel */
	struct menu_item *items[MAX_MENU_LEVEL_ITEMS]; /* item pointers */
};

			/* menu types */

#define MAX_MENU_TYPES		3

#define MENU_BAR		0
#define PULL_DOWN_MENU		1
#define WINDOW_SELECTION_MENU	2

			/* menu level options flags */


struct menu_funcs
{
	int (*position_menu)();
	int (*position_item)();
	int (*draw_menu)();
	int (*interp_key)();
};

extern pos_menu_bar(), pos_menu_bar_item(), calc_menu_bar_size(),
       draw_menu_bar(), menu_bar_interp_key();
extern pos_pull_down(), pos_pull_down_item(), calc_pull_down_size(),
       draw_pull_down(), pull_down_interp_key();
extern win_sel_interp_key();

			/* Special character definitions for use with
			   the acsc terminfo string which uses vt100
			   special characters as the default */

#define UPPER_LEFT	'l'
#define LOWER_LEFT	'm'
#define UPPER_RIGHT	'k'
#define LOWER_RIGHT	'j'
#define HORIZONTAL	'q'
#define VERTICAL	'x'
#define LEFT_TEE	't'
#define RIGHT_TEE	'u'
#define BOTTOM_TEE	'v'
#define TOP_TEE		'w'

			/* Attribute values understood by term_write */

#define FM_NBR_ATTRS		7

#define FM_ATTR_NONE		0
#define FM_ATTR_SHADOW		1
#define FM_ATTR_BOX		2
#define FM_ATTR_TITLE		3
#define FM_ATTR_ITEM		4
#define FM_ATTR_HIGHLIGHT	5
#define FM_ATTR_HIGHLIGHT_BLINK	6


			/* Globals */

extern struct menu_level *Menu_levels[MAX_MENU_LEVELS];
extern int Cur_menu_level;
extern int Auto_drop_down;
extern int Go_back_to_top_level;
extern int Current_window;
extern char Current_window_title[MAX_CURRENT_WINDOW_TITLE+1];
extern struct menu_funcs Menu_switch[];
extern char String_variables[ MAX_VARIABLES ][ MAX_VARIABLE_LENGTH ];
extern unsigned char Intr_char;
