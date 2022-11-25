/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: menu_bar.c,v 70.1 92/03/09 16:16:02 ssa Exp $ */
/*
 * menu_bar.c
 *
 * functions to implement the menu bar type in the Facet Menu.
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved
 *
 */

/* #include	"mycurses.h" */

/* #include	<term.h> */
#include	"uiterm.h"
#include	"fmenu.h"

#define MENU_BAR_TOP_ROW			0
#define MENU_BAR_LEFT_COL			0
#define MENU_BAR_FIRST_ITEM_ROW_NO_TITLE	0
#define MENU_BAR_FIRST_ITEM_ROW_WITH_TITLE	1
#define MENU_BAR_FIRST_ITEM_COL			3
#define MENU_BAR_ITEM_SEPARATION		3

pos_menu_bar( menu )
struct menu_level *menu;
{
	if ( menu->loc_row != -1 && menu->loc_col != -1 )
		return;
	if ( menu->prev_menu == 0 )
	{
		menu->loc_row = MENU_BAR_TOP_ROW;
		menu->loc_col = MENU_BAR_LEFT_COL;
	}
	else
	{
		menu->loc_row = menu->prev_menu->loc_row +
				menu->prev_menu->size_rows;
		menu->loc_col = MENU_BAR_LEFT_COL;
	}
}

pos_menu_bar_item( menu, itemndx )
struct menu_level *menu;
int itemndx;
{
	struct menu_item *item, *prev_item;
	char instantiated_string[ 81 ];

	if ( menu->nbr_items <= itemndx || itemndx < 0 )
		return;
	item = menu->items[itemndx];
	if ( itemndx == 0 )
	{
		item->col = MENU_BAR_FIRST_ITEM_COL;
	}
	else
	{
		prev_item = menu->items[itemndx - 1];
		instantiate_string( prev_item->name, instantiated_string );
		item->col = prev_item->col + strlen( instantiated_string ) +
			    MENU_BAR_ITEM_SEPARATION;
	}
	if ( menu->title[ 0 ] == 0 )
		item->row = menu->loc_row + MENU_BAR_FIRST_ITEM_ROW_NO_TITLE;
	else
		item->row = menu->loc_row + MENU_BAR_FIRST_ITEM_ROW_WITH_TITLE;
}

calc_menu_bar_size( menu )
struct menu_level *menu;
{
	if ( menu->title[ 0 ] == 0 )
	{
		menu->size_rows = 3;
		menu->size_cols = 80;
	}
	else
	{
		menu->size_rows = 4;
		menu->size_cols = 80;
	}
}

draw_menu_bar( menu )
struct menu_level *menu;
{
	char instantiated_string[81];
	int nblanks1, nblanks2;
	int attr;
	int i;
	struct menu_item *item;
	int has_title;

				/* determine the size of the menu and be
				   sure that it hasn't been positioned
				   off the edge of the screen */
	calc_menu_bar_size( menu );
	check_menu_position( menu );
	position_items( menu );

				/* If magic cookie terminal, turn off attributes
				   at the right edge of the menu before
				   starting.
				   
				   If an HP terminal clear the line so that it
				   won't be necessary to put down an attribute
				   with every stinking character written */

	if ( Fm_magic_cookie || Fm_ceol_standout_glitch )
	{
		for ( i = 0; i < menu->size_rows; i++ )
		{
			if ( Fm_ceol_standout_glitch )
			{
				term_cursor_address( menu->loc_row + i,
						     menu->loc_col );
				term_clear_line();
			}
			else
			{
				term_cursor_address( menu->loc_row + i,
				  menu->loc_col + (int) menu->size_cols - 1 );
				term_write( "%a%n", FM_ATTR_NONE );
			}
		}
	}
				/* draw the title line */

	has_title = menu->title[ 0 ];
	if ( has_title )
	{
				/* expand any intrinsic variables that
				   might be in the title */

		instantiate_string( menu->title, instantiated_string );

		term_cursor_address( menu->loc_row, menu->loc_col );
		nblanks1 = ( ( menu->size_cols -
			       strlen( instantiated_string ) ) / 2 ) - 4;
		nblanks2 = menu->size_cols - nblanks1 -
			   strlen( instantiated_string ) - 6;
		if ( Fm_magic_cookie )
		{
			term_write( "%a%g1%c%r%g0 %s %g1%r%c%g0%a%n",
				    FM_ATTR_BOX, UPPER_LEFT, HORIZONTAL,
				    nblanks1, instantiated_string, HORIZONTAL,
				    nblanks2, UPPER_RIGHT, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%g1%c%r%g0%a %s %a%g1%r%c%g0%a %n",
				    FM_ATTR_SHADOW, FM_ATTR_BOX, UPPER_LEFT,
				    HORIZONTAL, nblanks1, FM_ATTR_TITLE,
				    instantiated_string, FM_ATTR_BOX,
				    HORIZONTAL, nblanks2, UPPER_RIGHT,
				    FM_ATTR_SHADOW );
		}
	}

				/* draw the choices line */

	if ( has_title )
	{
		term_cursor_address( menu->loc_row +
				     MENU_BAR_FIRST_ITEM_ROW_WITH_TITLE,
				     menu->loc_col );
		nblanks1 = 80;
		if ( Fm_magic_cookie )
		{
			nblanks1 -= term_write( "%a%g1%c%g0%a%n", FM_ATTR_BOX,
						VERTICAL,
						FM_ATTR_NONE );
		}
		else
		{
			nblanks1 -= term_write( "%a %a%g1%c%g0%a %n",
						FM_ATTR_SHADOW, FM_ATTR_BOX,
						VERTICAL, FM_ATTR_ITEM );
		}
	}
	else
	{
		term_cursor_address( menu->loc_row +
				     MENU_BAR_FIRST_ITEM_ROW_NO_TITLE,
				     menu->loc_col );
		nblanks1 = 80;
		if ( Fm_magic_cookie )
		{
			nblanks1 -= term_write( "%a  %n", FM_ATTR_NONE );
		}
		else
		{
			nblanks1 -= term_write( "%a %a  %n",
						FM_ATTR_SHADOW, FM_ATTR_ITEM );
		}
	}
	for ( i = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];

				/* expand any intrinsic variables that
				   might be in the item name */

		instantiate_string( item->name, instantiated_string );

		if ( i == menu->cur_item )
			attr = FM_ATTR_HIGHLIGHT;
		else
			attr = FM_ATTR_ITEM;
		if ( Fm_magic_cookie )
		{
			nblanks1 -= term_write( "%a%s%a%b%n", attr,
						instantiated_string,
						FM_ATTR_ITEM,
						MENU_BAR_ITEM_SEPARATION - 2 );
		}
		else
		{
			nblanks1 -= term_write( " %a%s%a %b%n", attr,
						instantiated_string,
						FM_ATTR_ITEM,
						MENU_BAR_ITEM_SEPARATION - 2 );
		}
	}
	if ( has_title )
	{
		if ( Fm_magic_cookie )
		{
			term_write( "%b%a%g1%c%g0%a%n", nblanks1 - 3,
				    FM_ATTR_BOX, VERTICAL, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%b%a %a%g1%c%g0%a %n", nblanks1 - 3,
				    FM_ATTR_ITEM, FM_ATTR_BOX, VERTICAL,
				    FM_ATTR_SHADOW );
		}
	}
	else
	{
		if ( Fm_magic_cookie )
		{
			term_write( "%b%a%n", nblanks1 - 1, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%b%a %n", nblanks1 - 1, FM_ATTR_SHADOW );
		}
	}

					/* draw the bottom of the menu bar */

	if ( has_title )
	{
		term_cursor_address( menu->loc_row + 2, menu->loc_col );
		if ( Fm_magic_cookie )
		{
			term_write( "%a%g1%c%r%c%g0%a%n", FM_ATTR_BOX,
				    LOWER_LEFT, HORIZONTAL, 76, LOWER_RIGHT,
				    FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%g1%c%r%c%g0%a %n",
				    FM_ATTR_SHADOW, FM_ATTR_BOX,
				    LOWER_LEFT, HORIZONTAL, 76, LOWER_RIGHT,
				    FM_ATTR_SHADOW );
		}
	}
	else
	{
		term_cursor_address( menu->loc_row + 1, menu->loc_col );
		if ( Fm_magic_cookie )
		{
			term_write( "%a%g1%r%g0%a%n", FM_ATTR_BOX,
				    HORIZONTAL, 78, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%g1%r%g0%a %n",
				    FM_ATTR_SHADOW, FM_ATTR_BOX,
				    HORIZONTAL, 78, FM_ATTR_SHADOW );
		}
	}

				/* shadow the line below the menu bar */

	term_cursor_address( menu->loc_row + (int) menu->size_rows - 1,
			     menu->loc_col );
	if ( Fm_magic_cookie )
	{
		term_write( "%a%b%a", FM_ATTR_NONE, menu->size_cols - 2,
			    FM_ATTR_NONE );
	}
	else if ( Fm_ceol_standout_glitch )
	{
		; /* nothing to do - line is already cleared */
	}
	else
	{
		term_write( "%a%b%a", FM_ATTR_SHADOW, menu->size_cols,
			    FM_ATTR_NONE );
	}
}


/*
 * menu_bar_interp_key
 *
 * interprets keys for a menu bar
 *
 * returns 0 if staying in the menu, non-zero if destroying the menu
 * and going back to the previous menu level.
 *
 */

menu_bar_interp_key( menu, c )
struct menu_level *menu;
int c;
{
	int i;
	struct menu_item *item;

	for( i = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];
		if ( tolower( c ) == tolower( item->selection_char ) )
		{
			if ( menu->cur_item != i )
			{
				highlight_cur_item( menu, FM_ATTR_ITEM );
				menu->cur_item = i;
				highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
			}
			execute_cur_item( menu );
			return( 0 );
		}
	}
	switch ( c )
	{
		case '\033':
		case ' ':
			return( 1 );

		case FTKEY_LEFT:
		case FTKEY_BACKSPACE:
		case '\b':
			highlight_cur_item( menu, FM_ATTR_ITEM );
			dec_cur_item( menu );
			highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
			return( 0 );

		case FTKEY_RIGHT:
			highlight_cur_item( menu, FM_ATTR_ITEM );
			inc_cur_item( menu );
			highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
			return( 0 );

		case FTKEY_UP:
			if ( menu->prev_menu == 0 )
				return( 0 );
			else
				return( 1 );

		case '\r':
		case FTKEY_RETURN:
			execute_cur_item( menu );
			return( 0 );

		case FTKEY_DOWN:
		case '\n':
			item = menu->items[menu->cur_item];
			if ( item->type == MENU ||
			     ( item->type == INTRINSIC &&
			       strcmp( item->action[ 0 ], "win_menu" ) == 0 ) )
			{
				execute_cur_item( menu );
			}
			return( 0 );

		default:
			return( 0 );
	}
}
