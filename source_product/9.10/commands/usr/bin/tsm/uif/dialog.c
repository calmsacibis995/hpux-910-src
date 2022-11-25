/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: dialog.c,v 70.1 92/03/09 16:15:20 ssa Exp $ */
/*
 * dialog.c
 *
 * functions to implement dialog boxes in the Facet Menu.
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved
 *
 */

/* #include	"mycurses.h" */

/* #include	<term.h> */
#include	"uiterm.h"
#include	"fmenu.h"


calc_dialog_box_size( menu, dialog_box )
struct menu_level *menu;
struct dialog_box *dialog_box;
{
	int i, len, maxlen;
	struct dialog_item *dialog_item;
	char promptbuff[ 81 ];

	if ( menu->type == MENU_BAR && Fm_ceol_standout_glitch )
		dialog_box->size_rows = dialog_box->nbr_items + 2;
	else
		dialog_box->size_rows = dialog_box->nbr_items + 3;
	for( i = 0, maxlen = 0; i < dialog_box->nbr_items; i++ )
	{
		dialog_item = dialog_box->items[i];
		instantiate_string( dialog_item->prompt, promptbuff );
		len = strlen( promptbuff );
		if ( len > 0 )
		{
			if ( (int) ( dialog_item->max_var_len ) > 0 )
				len += dialog_item->max_var_len + 2;
			else
				len += 1;
		}
		else
		{
			if ( (int) ( dialog_item->max_var_len ) > 0 )
				len = dialog_item->max_var_len + 1;
			else
				len = 0;
		}
		if ( len > maxlen )
			maxlen = len;
	}
	dialog_box->size_cols = maxlen + 7;
}

pos_dialog_box( menu, dialog_box )
struct menu_level *menu;
struct dialog_box *dialog_box;
{
	struct menu_item *prev_item;

	prev_item = menu->items[menu->cur_item];
	if ( menu->type == MENU_BAR )
	{
		if ( Fm_ceol_standout_glitch )
			dialog_box->loc_row = menu->loc_row +
					      menu->size_rows - 1;
		else
			dialog_box->loc_row = menu->loc_row +
					      menu->size_rows - 2;
		dialog_box->loc_col = prev_item->col;
	}
	else if ( menu->type == PULL_DOWN_MENU )
	{
		dialog_box->loc_row = prev_item->row;
		dialog_box->loc_col = menu->loc_col + menu->size_cols - 4;
	}
}


check_dialog_position( menu, dialog_box )
struct menu_level *menu;
struct dialog_box *dialog_box;
{
	if ( dialog_box->loc_col + dialog_box->size_cols > Columns )
	{
		dialog_box->loc_col -=
		      ( dialog_box->loc_col + dialog_box->size_cols ) - Columns;
		if ( dialog_box->loc_col < 0 )
			dialog_box->loc_col = 0;
		if ( menu->type == PULL_DOWN_MENU )
			dialog_box->loc_row++;
	}
	if ( dialog_box->loc_row + dialog_box->size_rows > Lines )
	{
		dialog_box->loc_row -=
			( dialog_box->loc_row + dialog_box->size_rows ) - Lines;
		if ( dialog_box->loc_row < 0 )
			dialog_box->loc_row = 0;
	}
}

pos_dialog_box_items( menu, dialog_box )
struct menu_level *menu;
struct dialog_box *dialog_box;
{
	int i;
	int len;
	struct dialog_item *dialog_item;
	char promptbuff[ 81 ];

	for ( i = 0; i < dialog_box->nbr_items; i++ )
	{
		dialog_item = dialog_box->items[ i ];
		if ( menu->type == MENU_BAR && Fm_ceol_standout_glitch )
			dialog_item->loc_row = dialog_box->loc_row + i;
		else
			dialog_item->loc_row = dialog_box->loc_row + i + 1;
		if ( dialog_item->action == OK_TO_LEAVE ||
		     dialog_item->action == CANCEL )
		{
			dialog_item->prompt_col = dialog_box->loc_col +
				( ( dialog_box->size_cols -
				    strlen( dialog_item->prompt ) )
				/ 2 );
			dialog_item->var_col = dialog_item->prompt_col;
			continue;
		}
		dialog_item->prompt_col = dialog_box->loc_col + 3;
		instantiate_string( dialog_item->prompt, promptbuff );
		len = strlen( promptbuff );
		if ( len )
			dialog_item->var_col = dialog_item->prompt_col + len +1;
		else
			dialog_item->var_col = dialog_item->prompt_col;
	}
}

extern struct dialog_item *create_dialog_item();

draw_dialog_box( menu, dialog_box )
struct menu_level *menu;
struct dialog_box *dialog_box;
{
	int i, nblanks1, nblanks2, nblanks3;
	int attr;
	struct dialog_item *dialog_item;
	char promptbuff[ 81 ];

				/* add the OK and CANCEL items to the bottom
				   of the dialog box */

	if ( dialog_box->nbr_items == 0 ||
	     dialog_box->items[ dialog_box->nbr_items - 1 ]->action !=
	     CANCEL )
	{
		dialog_item = create_dialog_item( dialog_box );
		strcpy( dialog_item->prompt, "OK" );
		dialog_item->action = OK_TO_LEAVE;
		dialog_item = create_dialog_item( dialog_box );
		strcpy( dialog_item->prompt, "CANCEL" );
		dialog_item->action = CANCEL;
	}

				/* calculate the size of the dialog box */

	calc_dialog_box_size( menu, dialog_box );

				/* position the dialog box if the file control
				   menu hasn't specified a location */

	if ( dialog_box->loc_row == -1 && dialog_box->loc_col == -1 )
		pos_dialog_box( menu, dialog_box );

				/* make sure it doesn't run off the screen */

	check_dialog_position( menu, dialog_box );

				/* position all the items in the dialog box */

	pos_dialog_box_items( menu, dialog_box );

				/* If magic cookie terminal, turn off attributes
				   at the right edge of the box before
				   starting.
				   
				   If an HP terminal clear the line so that it
				   won't be necessary to put down an attribute
				   with every stinking character written */

	if ( Fm_magic_cookie || Fm_ceol_standout_glitch )
	{
		for ( i = 0; i < dialog_box->size_rows; i++ )
		{
			if ( Fm_ceol_standout_glitch )
			{
				term_cursor_address( dialog_box->loc_row + i,
						     dialog_box->loc_col );
				term_clear_line();
			}
			else
			{
				term_cursor_address( dialog_box->loc_row + i,
						     dialog_box->loc_col +
					      (int) dialog_box->size_cols - 1 );
				term_write( "%a%n", FM_ATTR_NONE );
			}
		}
	}

				/* draw the top of the dialog box */

	if ( menu->type != MENU_BAR || !Fm_ceol_standout_glitch )
	{
		term_cursor_address( dialog_box->loc_row, dialog_box->loc_col );
		if ( Fm_magic_cookie )
		{
			term_write( "%a%g1%c%r%c%g0%a%n", FM_ATTR_BOX,
				    UPPER_LEFT, HORIZONTAL,
				    dialog_box->size_cols - 4, UPPER_RIGHT,
				    FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%g1%c%r%c%g0%a %n", FM_ATTR_SHADOW,
				    FM_ATTR_BOX, UPPER_LEFT, HORIZONTAL,
				    dialog_box->size_cols - 4,
				    UPPER_RIGHT, FM_ATTR_SHADOW );
		}
	}

				/* start with the first item in the box */

	dialog_box->cur_item = 0;
	if ( dialog_box->items[ 0 ]->action == 0 )
		inc_cur_dialog_item( dialog_box );

				/* write the item lines */

	for( i = 0; i < dialog_box->nbr_items; i++ )
	{
		dialog_item = dialog_box->items[ i ];
		dialog_item->being_entered = 0;
		term_cursor_address( dialog_item->loc_row,
				     dialog_box->loc_col );
		if ( dialog_box->cur_item == i )
			attr = FM_ATTR_HIGHLIGHT;
		else
			attr = FM_ATTR_ITEM;
		instantiate_string( dialog_item->prompt, promptbuff );
		if ( dialog_item->variable != 0 )
		{
			*( dialog_item->variable ) = '\0';
			nblanks1 = dialog_item->prompt_col -
				   dialog_box->loc_col - 2;
			nblanks2 = dialog_item->max_var_len;
			nblanks3 = dialog_box->size_cols - 3;
			if ( Fm_magic_cookie )
			{
				nblanks3 -= term_write(
					    "%a%g1%c%g0%a%b%s%a%b%a%n",
					    FM_ATTR_BOX, VERTICAL,
					    FM_ATTR_ITEM, nblanks1,
					    promptbuff, attr,
					    nblanks2, FM_ATTR_ITEM );
			}
			else
			{
				nblanks3 -= term_write(
					    "%a %a%g1%c%g0%a %b%s %a%b%a %n",
					    FM_ATTR_SHADOW, FM_ATTR_BOX,
					    VERTICAL, FM_ATTR_ITEM,
					    nblanks1, promptbuff, attr,
					    nblanks2, FM_ATTR_ITEM );
			}
		}
		else
		{
			nblanks1 = dialog_item->prompt_col -
				   dialog_box->loc_col - 3;
			nblanks3 = dialog_box->size_cols - 3;
			if ( Fm_magic_cookie )
			{
				nblanks3 -= term_write(
					    "%a%g1%c%g0%a%b%a%s%a%n",
					    FM_ATTR_BOX, VERTICAL, FM_ATTR_ITEM,
					    nblanks1, attr, promptbuff,
					    FM_ATTR_ITEM );
			}
			else
			{
				nblanks3 -= term_write(
					    "%a %a%g1%c%g0%a %b %a%s%a %n",
					    FM_ATTR_SHADOW, FM_ATTR_BOX,
					    VERTICAL, FM_ATTR_ITEM,
					    nblanks1, attr, promptbuff,
					    FM_ATTR_ITEM );
			}
		}
		if ( Fm_magic_cookie )
		{
			term_write( "%b%a%g1%c%g0%a%n", nblanks3, FM_ATTR_BOX,
				    VERTICAL, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%b %a%g1%c%g0%a %n", nblanks3, FM_ATTR_BOX,
				    VERTICAL, FM_ATTR_SHADOW );
		}
	}

				/* draw the bottom of the dialog box */

	if ( menu->type == MENU_BAR && Fm_ceol_standout_glitch )
	{
		term_cursor_address( dialog_box->loc_row +
				     dialog_box->nbr_items,
				     dialog_box->loc_col );
	}
	else
	{
		term_cursor_address( dialog_box->loc_row +
				     dialog_box->nbr_items + 1,
				     dialog_box->loc_col );
	}
	if ( Fm_magic_cookie )
	{
		term_write( "%a%g1%c%r%c%g0%a%n", FM_ATTR_BOX, LOWER_LEFT,
			    HORIZONTAL, dialog_box->size_cols - 4, LOWER_RIGHT,
			    FM_ATTR_NONE );
	}
	else
	{
		term_write( "%a %a%g1%c%r%c%g0%a %n", FM_ATTR_SHADOW, FM_ATTR_BOX,
			    LOWER_LEFT, HORIZONTAL, dialog_box->size_cols - 4,
			    LOWER_RIGHT, FM_ATTR_SHADOW );
	}

				/* shadow the line below the box */

	term_cursor_address( dialog_box->loc_row + dialog_box->nbr_items + 2,
			    dialog_box->loc_col );
	if ( Fm_magic_cookie )
	{
		term_write( "%a%b%a", FM_ATTR_NONE, dialog_box->size_cols - 2,
			    FM_ATTR_NONE );
	}
	else if ( Fm_ceol_standout_glitch )
	{
		; /* nothing to do - line is already cleared */
	}
	else
	{
		term_write( "%a%b%a", FM_ATTR_SHADOW, dialog_box->size_cols,
			    FM_ATTR_NONE );
	}

	dialog_item = dialog_box->items[ dialog_box->cur_item ];
	term_cursor_address( dialog_item->loc_row, 
		       (int) dialog_item->var_col + 1 );
}


/*
 * dialog_box_interp_key
 *
 * interprets keys for a menu bar
 *
 * returns 0 if staying in the menu, non-zero if destroying the menu
 * and going back to the previous menu level.
 *
 */

dialog_box_interp_key( dialog_box, c )
struct dialog_box *dialog_box;
int c;
{
	struct dialog_item *dialog_item;
	int var_len;

	dialog_item = dialog_box->items[ dialog_box->cur_item ];
	switch ( c )
	{
		case '\033':
		case ' ':
			if ( c == ' ' && dialog_item->action == ENTER_STRING &&
			     dialog_item->being_entered &&
			     strlen( dialog_item->variable ) > 0 )
				goto enter_text_chars;
			return( 1 );

		case FTKEY_UP:
			highlight_cur_dialog_item( dialog_box, FM_ATTR_ITEM );
			dec_cur_dialog_item( dialog_box );
			highlight_cur_dialog_item( dialog_box,
						   FM_ATTR_HIGHLIGHT );
			dialog_item = dialog_box->items[ dialog_box->cur_item ];
			term_cursor_address( dialog_item->loc_row,
					     (int) dialog_item->var_col + 1 );
			return( 0 );

		case FTKEY_DOWN:
		case '\n':
			highlight_cur_dialog_item( dialog_box, FM_ATTR_ITEM );
			inc_cur_dialog_item( dialog_box );
			highlight_cur_dialog_item( dialog_box,
						   FM_ATTR_HIGHLIGHT );
			dialog_item = dialog_box->items[ dialog_box->cur_item ];
			term_cursor_address( dialog_item->loc_row,
					     (int) dialog_item->var_col + 1 );
			return( 0 );

		case '\b':	/* backspace */
		case FTKEY_LEFT:
		case FTKEY_BACKSPACE:
			if ( dialog_item->being_entered &&
			     strlen( dialog_item->variable ) > 0 )
			{
				var_len = strlen( dialog_item->variable );
				if ( var_len <= 0 )
					return( 0 );
				term_write( "\010 \010" );
				dialog_item->variable[ var_len - 1 ] = 0;
			}
			return( 0 );

		case '\r':
		case FTKEY_RETURN:
			if ( dialog_item->action == OK_TO_LEAVE )
			{
				dialog_box->completed = 1;
			}
			else if ( dialog_item->action == CANCEL )
			{
				return( 1 );
			}
			else
			{
				highlight_cur_dialog_item( dialog_box,
							   FM_ATTR_ITEM );
				inc_cur_dialog_item( dialog_box );
				highlight_cur_dialog_item( dialog_box,
							   FM_ATTR_HIGHLIGHT );
			}
			return( 0 );

		default:
enter_text_chars:
			if ( c < ' ' || c > '~' || dialog_item->variable == 0 )
				return( 0 );
			if ( !dialog_item->being_entered )
			{
				dialog_item->being_entered = 1;
				if ( strlen( dialog_item->variable ) != 0 )
				{
					term_cursor_address(
						dialog_item->loc_row,
						dialog_item->var_col );
					if ( Fm_magic_cookie )
					{
						term_write( "%a%b%a",
						  FM_ATTR_HIGHLIGHT,
						  dialog_item->max_var_len,
						  FM_ATTR_ITEM );
					}
					else
					{
						term_write( "%a %a%b%a ",
						  FM_ATTR_ITEM,
						  FM_ATTR_HIGHLIGHT,
						  dialog_item->max_var_len,
						  FM_ATTR_ITEM );
					}
				}
				term_cursor_address( dialog_item->loc_row,
						     dialog_item->var_col );
				if ( Fm_magic_cookie )
					term_write( "%a", FM_ATTR_HIGHLIGHT );
				else
					term_write( "%a %a", FM_ATTR_ITEM,
						    FM_ATTR_HIGHLIGHT );
				*( dialog_item->variable ) = 0;
			}
			var_len = strlen( dialog_item->variable );
			if ( var_len >= dialog_item->max_var_len )
				return( 0 );
			term_putchar( c );
			term_outgo();
			dialog_item->variable[ var_len++ ] = c;
			dialog_item->variable[ var_len ] = 0;
			return( 0 );
	}
}


highlight_cur_dialog_item( dialog_box, attr )
struct dialog_box *dialog_box;
int attr;
{
	struct dialog_item *dialog_item;
	int nblanks;
	char promptbuff[ 81 ];

	dialog_item = dialog_box->items[ dialog_box->cur_item ];
	if ( dialog_item->variable != 0 )
	{
		nblanks = dialog_item->max_var_len -
			  strlen( dialog_item->variable );
		term_cursor_address( dialog_item->loc_row,
				     dialog_item->var_col );
		if ( Fm_magic_cookie )
		{
			term_write( "%a%s%b%a", attr, dialog_item->variable,
				    nblanks, FM_ATTR_ITEM );
		}
		else
		{
			term_write( "%a %a%s%b%a %a", FM_ATTR_ITEM, attr,
				    dialog_item->variable, nblanks,
				    FM_ATTR_ITEM, FM_ATTR_NONE );
		}
	}
	else
	{
		instantiate_string( dialog_item->prompt, promptbuff );
		term_cursor_address( dialog_item->loc_row,
				     dialog_item->prompt_col );
		if ( Fm_magic_cookie )
		{
			term_write( "%a%s%a", attr, promptbuff, FM_ATTR_NONE );
		}
		else
		{
			term_write( "%a %a%s%a %a", FM_ATTR_ITEM, attr,
				    promptbuff, FM_ATTR_ITEM, FM_ATTR_NONE );
		}
	}
	term_cursor_address( dialog_item->loc_row,
			     (int) dialog_item->var_col + 1 );
}


dec_cur_dialog_item( dialog_box )
struct dialog_box *dialog_box;
{
	int original_item;

	original_item = dialog_box->cur_item;
	dialog_box->items[ dialog_box->cur_item ]->being_entered = 0;
	do
	{
		if ( dialog_box->cur_item == 0 )
			dialog_box->cur_item = dialog_box->nbr_items - 1;
		else
			dialog_box->cur_item--;
	}
	while ( dialog_box->items[ dialog_box->cur_item ]->action == 0 &&
		dialog_box->cur_item != original_item );
	dialog_box->items[ dialog_box->cur_item ]->being_entered = 0;
}


inc_cur_dialog_item( dialog_box )
struct dialog_box *dialog_box;
{
	int original_item;

	original_item = dialog_box->cur_item;
	dialog_box->items[ dialog_box->cur_item ]->being_entered = 0;
	do
	{
		if ( dialog_box->cur_item == dialog_box->nbr_items - 1 )
			dialog_box->cur_item = 0;
		else
			dialog_box->cur_item++;
	}
	while ( dialog_box->items[ dialog_box->cur_item ]->action == 0 &&
		dialog_box->cur_item != original_item );
	dialog_box->items[ dialog_box->cur_item ]->being_entered = 0;
}
