/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: pull_down.c,v 70.1 92/03/09 16:16:08 ssa Exp $ */
/*
 * pull_down.c
 *
 * functions to implement the pull down menu type in the Facet Menu.
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved
 *
 */

/* #include	"mycurses.h" */

/* #include	<term.h> */
#include	"uiterm.h"
#include	"fmenu.h"

#define PULL_DOWN_TOP_ROW			0
#define PULL_DOWN_LEFT_COL			0
#define PULL_DOWN_FIRST_ITEM_ROW_NO_TITLE	1
#define PULL_DOWN_FIRST_ITEM_ROW_WITH_TITLE	3
#define PULL_DOWN_FIRST_ITEM_COL		3


pos_pull_down( menu )
struct menu_level *menu;
{
	struct menu_level *prev_menu;
	struct menu_item *prev_item;

	if ( menu->loc_row != -1 && menu->loc_col != -1 )
		return;
	prev_menu = menu->prev_menu;
	if ( prev_menu == 0 )
	{
		menu->loc_row = PULL_DOWN_TOP_ROW;
		menu->loc_col = PULL_DOWN_LEFT_COL;
	}
	else
	{
		prev_item = prev_menu->items[prev_menu->cur_item];
		if ( prev_menu->type == MENU_BAR )
		{
			if ( Fm_ceol_standout_glitch )
				menu->loc_row = prev_menu->loc_row +
						prev_menu->size_rows -1;
			else
				menu->loc_row = prev_menu->loc_row +
						prev_menu->size_rows -2;
			menu->loc_col = prev_item->col;
		}
		else if ( prev_menu->type == PULL_DOWN_MENU )
		{
			menu->loc_row = prev_item->row;
			menu->loc_col = prev_item->col +
					strlen( prev_item->name ) + 3;
		}
	}
}

pos_pull_down_item( menu, itemndx )
struct menu_level *menu;
int itemndx;
{
	struct menu_item *item, *prev_item;
	struct menu_level *prev_menu;

	prev_menu = menu->prev_menu;
	if ( menu->nbr_items <= itemndx || itemndx < 0 )
		return;
	item = menu->items[itemndx];
	if ( itemndx == 0 )
	{
		if ( menu->title[ 0 ] == 0 )
		{
			if ( prev_menu->type == MENU_BAR &&
			     Fm_ceol_standout_glitch )
			{
				item->row = menu->loc_row +
					  PULL_DOWN_FIRST_ITEM_ROW_NO_TITLE - 1;
			}
			else
			{
				item->row = menu->loc_row +
					    PULL_DOWN_FIRST_ITEM_ROW_NO_TITLE;
			}
		}
		else
			item->row = menu->loc_row +
				    PULL_DOWN_FIRST_ITEM_ROW_WITH_TITLE;
		item->col = menu->loc_col + PULL_DOWN_FIRST_ITEM_COL;
	}
	else
	{
		prev_item = menu->items[itemndx - 1];
		item->row = prev_item->row + 1;
		item->col = menu->loc_col + PULL_DOWN_FIRST_ITEM_COL;
	}
}

calc_pull_down_size( menu, instantiated_title )
struct menu_level *menu;
char *instantiated_title;
{
	int i, len, maxlen;
	struct menu_item *item;
	struct menu_level *prev_menu;
	char instantiated_string[ 81 ];

	prev_menu = menu->prev_menu;
	if ( menu->title[ 0 ] == 0 )
	{
		if ( prev_menu->type == MENU_BAR && Fm_ceol_standout_glitch )
			menu->size_rows = menu->nbr_items + 2;
		else
			menu->size_rows = menu->nbr_items + 3;
	}
	else
		menu->size_rows = menu->nbr_items + 5;
	for( i = 0, maxlen = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];
		instantiate_string( item->name, instantiated_string );
		if ( ( len = strlen( instantiated_string ) ) > maxlen )
			maxlen = len;
	}
	menu->size_cols = maxlen + 8;
	i = strlen( instantiated_title );
	if ( i + 4 > menu->size_cols )
		menu->size_cols = i + 4;
}


draw_pull_down( menu )
struct menu_level *menu;
{
	int i;
	int nblanks1, nblanks2;
	struct menu_item *item;
	int attr;
	struct menu_level *prev_menu;
	char instantiated_string[81];
	int last_item_line;

				/* expand any intrinsic variables that
				   might be in the title */

	instantiate_string( menu->title, instantiated_string );

				/* calculate the size of the menu and be
				   sure that automatic positioning of the
				   menu has not placed it off the screen */

	calc_pull_down_size( menu, instantiated_string );
	check_menu_position( menu );
	position_items( menu );

	prev_menu = menu->prev_menu;

				/* If magic cookie terminal, turn off attributes
				   at the right edge of the menu before
				   starting.

				   If an HP terminal clear the line so that it
				   won't be necessary to put down an attribute
				   with every stinking character written */

	if ( Fm_magic_cookie || Fm_ceol_standout_glitch )
	{
		if ( prev_menu->type == MENU_BAR && ! Fm_ceol_standout_glitch )
			i = 1;
		else
			i = 0;
		for ( ; i < menu->size_rows; i++ )
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

				/* draw top of box */
				/* (but only if its NOT a pull down without
				   a title from a menu bar on an hp terminal) */

	if ( !( menu->title[ 0 ] == 0 && prev_menu->type == MENU_BAR &&
	     Fm_ceol_standout_glitch ) )
	{
		term_cursor_address( menu->loc_row , menu->loc_col );
		if ( Fm_magic_cookie )
		{
			if ( prev_menu->type == MENU_BAR )
				attr = FM_ATTR_BOX;
			else
				attr = FM_ATTR_NONE;
			term_write( "%a%g1%c%r%c%g0%a%n", FM_ATTR_BOX,
				    UPPER_LEFT, HORIZONTAL,
				    menu->size_cols - 4, UPPER_RIGHT, attr);
		}
		else
		{
			term_write( "%a %a%g1%c%r%c%g0%a %n", FM_ATTR_SHADOW,
				    FM_ATTR_BOX, UPPER_LEFT, HORIZONTAL,
				    menu->size_cols - 4, UPPER_RIGHT,
				    FM_ATTR_SHADOW);
		}
	}

				/* draw the title line */

	if ( menu->title[ 0 ] != 0 )
	{
		term_cursor_address( menu->loc_row + 1, menu->loc_col );
		nblanks1 = ( ( menu->size_cols -
			       strlen( instantiated_string ) ) / 2 ) - 2;
		if ( nblanks1 < 0 )
			nblanks1 = 0;
		nblanks2 = menu->size_cols - nblanks1 -
			   strlen( instantiated_string ) - 4;
		if ( nblanks2 < 0 )
			nblanks2 = 0;
		if ( Fm_magic_cookie )
		{
			term_write( "%a%g1%c%g0%b%s%b%g1%c%g0%a%n",
				    FM_ATTR_BOX, VERTICAL,
				    nblanks1, instantiated_string,
				    nblanks2, VERTICAL, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%g1%c%g0%a%b%s%b%a%g1%c%g0%a %n",
				    FM_ATTR_SHADOW, FM_ATTR_BOX, VERTICAL,
				    FM_ATTR_TITLE, nblanks1, instantiated_string,
				    nblanks2, FM_ATTR_BOX, VERTICAL,
				    FM_ATTR_SHADOW );
		}

				/* draw a line under the title */

		term_cursor_address( menu->loc_row + 2, menu->loc_col );
		if ( Fm_magic_cookie )
		{
			term_write( "%a%g1%c%r%c%g0%a%n", FM_ATTR_BOX, LEFT_TEE,
				    HORIZONTAL, menu->size_cols - 4, RIGHT_TEE,
				    FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%g1%c%r%c%g0%a %n", FM_ATTR_SHADOW,
				    FM_ATTR_BOX, LEFT_TEE, HORIZONTAL,
				    menu->size_cols - 4, RIGHT_TEE,
				    FM_ATTR_SHADOW );
		}
		last_item_line = menu->loc_row + menu->nbr_items + 2;
	}
	else
	{
		if ( prev_menu->type == MENU_BAR && Fm_ceol_standout_glitch )
			last_item_line = menu->loc_row + menu->nbr_items - 1;
		else
			last_item_line = menu->loc_row + menu->nbr_items;
	}

				/* draw the choices lines */

	for ( i = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];

				/* expand any intrinsic variables that
				   might be in the item name */

		instantiate_string( item->name, instantiated_string );

		term_cursor_address( item->row, menu->loc_col );
		nblanks1 = menu->size_cols - 3;
		if ( i == menu->cur_item )
			attr = FM_ATTR_HIGHLIGHT;
		else
			attr = FM_ATTR_ITEM;
		if ( Fm_magic_cookie )
		{
			nblanks1 -= term_write( "%a%g1%c%g0%a%a%s%a%n",
						FM_ATTR_BOX, VERTICAL,
						FM_ATTR_ITEM, attr,
						instantiated_string,
						FM_ATTR_ITEM );
		}
		else
		{
			nblanks1 -= term_write( "%a %a%g1%c%g0%a  %a%s%a %n",
						FM_ATTR_SHADOW, FM_ATTR_BOX,
						VERTICAL, FM_ATTR_ITEM, attr,
						instantiated_string,
						FM_ATTR_ITEM );
		}
		if ( Fm_magic_cookie )
		{
			term_write( "%b%a%g1%c%g0%a%n", nblanks1, FM_ATTR_BOX,
				    VERTICAL, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%b %a%g1%c%g0%a %n", nblanks1, FM_ATTR_BOX,
				    VERTICAL, FM_ATTR_SHADOW );
		}
	}

				/* draw the bottom of the box */

	term_cursor_address( last_item_line + 1, menu->loc_col );
	if ( Fm_magic_cookie )
	{
		term_write( "%a%g1%c%r%c%g0%a%n", FM_ATTR_BOX, LOWER_LEFT,
			    HORIZONTAL, menu->size_cols - 4, LOWER_RIGHT,
			    FM_ATTR_NONE );
	}
	else
	{
		term_write( "%a %a%g1%c%r%c%g0%a %n", FM_ATTR_SHADOW, FM_ATTR_BOX,
			    LOWER_LEFT, HORIZONTAL, menu->size_cols - 4,
			    LOWER_RIGHT, FM_ATTR_SHADOW );
	}

				/* shadow the line below the box */

	term_cursor_address( last_item_line + 2, menu->loc_col );
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
 * pull_down_interp_key
 *
 * interprets keys for a menu bar
 *
 * returns 0 if staying in the menu, non-zero if destroying the menu
 * and going back to the previous menu level.
 *
 */

pull_down_interp_key( menu, c )
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
			Auto_drop_down = 0;
			return( 1 );

		case FTKEY_UP:
			highlight_cur_item( menu, FM_ATTR_ITEM );
			dec_cur_item( menu );
			highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
			return( 0 );

		case FTKEY_DOWN:
		case '\n':
			highlight_cur_item( menu, FM_ATTR_ITEM );
			inc_cur_item( menu );
			highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
			return( 0 );

		case FTKEY_LEFT:
		case FTKEY_BACKSPACE:
		case '\b':
			if ( menu->prev_menu->type == MENU_BAR )
			{
				Auto_drop_down = 1;
				push_key( FTKEY_LEFT );
				return( 1 );
			}
			return( 1 );
			/************************
			or, to cause pop of both pull downs and a drop
			down on next menu bar item left:
			if ( prev is MENU_BAR )
				Auto_drop_down = 1;
			push_key( FTKEY_LEFT );
			return( 1 );
			*************************/

		case FTKEY_RIGHT:
			if ( menu->prev_menu->type == MENU_BAR )
			{
				Auto_drop_down = 1;
				push_key( FTKEY_RIGHT );
				return( 1 );
			}
			return( 1 );

		case '\r':
		case FTKEY_RETURN:
			execute_cur_item( menu );
			return( 0 );

		default:
			return( 0 );
	}
}
