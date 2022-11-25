/*****************************************************************************
** Copyright (c) 1991 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fmenu.c,v 70.1 92/03/09 16:15:38 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ 2.2.0 Copyright (c) 1991 by SSSI";
#endif

/*
 * fmenu.c
 *
 * Facet Menu
 *
 * Main module
 *
 */

#include	<stdio.h>
#include	<signal.h>
#include	<malloc.h>

/* #include	"mycurses.h" */

/* #include	<term.h> */
#include	"uiterm.h"
#include	"termio.h"
#include	"facetterm.h"
#include	"facetpath.h"
#include	"fmenu.h"
#include	"facetpc.h"

				/* Globals for the Facet Menu */
int Facet_type;
int Menu_window;

char Menu_hot_key[3] = "^T";
char Menu_hot_key_char = '\024';
char Ft_hot_key[3] = "^W";
char Windows_window_char_stop = WINDOWS_WINDOW_CHAR_STOP;
char Notify_when_current_char = NOTIFY_WHEN_CURRENT_CHAR;
char Window_mode_to_user_char = WINDOW_MODE_TO_USER_CHAR;

struct menu_level *Menu_levels[MAX_MENU_LEVELS];
int Cur_menu_level = -1;
int Pop_up_level = -1;
int Auto_drop_down = 0;
int First_key = 0;
int Go_back_to_top_level = 0;
int Current_window;
char Current_window_title[MAX_CURRENT_WINDOW_TITLE+1];
char Remembered_name[ 10 ][ MAX_MENU_ITEM_NAME + 1 ] = { "" };
struct menu_funcs Menu_switch[ MAX_MENU_TYPES ] =
{
	{ pos_menu_bar, pos_menu_bar_item, draw_menu_bar,
	  menu_bar_interp_key },
	{ pos_pull_down, pos_pull_down_item, draw_pull_down,
	  pull_down_interp_key },
	{ pos_pull_down, pos_pull_down_item, draw_pull_down,
	  win_sel_interp_key }
};
char String_variables[ MAX_VARIABLES ][ MAX_VARIABLE_LENGTH ];
char Blanks[] = "                                                                                ";
char Rowenv[ 40 ];
char Colenv[ 40 ];

extern signal_quit();

int Opt_quit_on_all_windows_idle = 0;

/* text strings used in printf's which are user definable */

char *Text_must_run_on_window =
"Menu must run on a TSM window\n";

char *Text_first_menu_not_found =
"First menu file not found ... exiting\r\n";

char *Text_menu_window_title =
"'TSM Menu\r";

char *Text_window_selection_menu_title =
"Window Selection";

char *Text_all_windows_except_menu_idle =
"All sessions except menu are idle";


main( argc, argv )
int argc;
char *argv[];
{
	char menu_name[ MAX_MENU_LEVEL_NAME + 1 ];
	char buff[ 80 ];
	char		filename[ 256 ];

/*open_debug();*/

	set_options_facetpath();
	sprintf( filename, "%suitext", Facetname );
	load_foreign( filename );

				/* Determine what environment the menu
				   is running in, and if on a window,
				   which one */

	Facet_type = what_facet_type();

				/* if not running on Facet/Term or Facet/PC
				   then quit */

	if ( Facet_type != FACET_TERM
	     && Facet_type != FACET_PC
	   )
	{
		printf( Text_must_run_on_window );
		exit( 1 );
	}

	if ( Facet_type == FACET_PC )
	{
		printf( "FacetPC not yet supported\n" );
		exit( 1 );
	}

				/* Initialize the terminal */
	term_init();

				/* Initialize the interface to FACET/TERM or
				   FACET/PC */

				/* initialize keyboard processing */
	init_keys();

				/* get configuration information from
				   .facet file */
	process_dotfacet();

	if ( Facet_type == FACET_TERM )
		ft_init( Menu_window );
	else if ( Facet_type == FACET_PC )
		fpc_init( Menu_hot_key_char, Notify_when_current_char );

				/* catch signals to quit properly */

	signal( SIGINT, signal_quit );	/* actually this is ineffective in
					   raw mode */
	signal( SIGTERM, signal_quit );
	signal( SIGHUP, signal_quit );

				/* set up the top level menu */

	if ( argc < 2 )
	{
		if ( Facet_type == FACET_TERM )
		{
			strcpy( menu_name, "tsm_menu" );
		}
		else
			strcpy( menu_name, "fpc_menu" );
	}
	else
		strncpy( menu_name, argv[1], MAX_MENU_LEVEL_NAME );
	menu_name[ MAX_MENU_LEVEL_NAME ] = 0;
	create_menu_level( menu_name );

				/* read in the first level menu */

	if ( read_menu_file( Menu_levels[0] ) == -1 )
	{
		printf( Text_first_menu_not_found );
		quit( 0 );
	}
	if ( Facet_type == FACET_TERM )
	{
		ft_send_command( Text_menu_window_title );
		sprintf( buff, ":h\\%s\\%s\\r", Menu_hot_key, Menu_hot_key );
		ft_send_command( buff );
	}
	else
	{
		;/* FACETPC HOT KEY */
	}
	go();
	/*NOTREACHED*/
}


what_facet_type()
{
	char buffer[ FIOC_BUFFER_SIZE ];
	int type;

	strcpy( buffer, "facet_type" );
	if ( fct_ioctl( 0, FIOC_GET_INFORMATION, buffer ) == -1 )
		return( NO_FACET );
	if ( strcmp( buffer, "FACET/PC" ) == 0 )
		type = FACET_PC;
	else if ( strcmp( buffer, "FACET/TERM" ) == 0 )
		type = FACET_TERM;
	else
		return( NO_FACET );
	Menu_window = ft_get_window_number();
	return( type );
}


create_menu_level( name )
char *name;
{
	struct menu_level *menu;

	menu = (struct menu_level *)malloc( sizeof( struct menu_level ) );
	if ( Cur_menu_level == -1 )
		menu->prev_menu = 0;
	else
		menu->prev_menu = Menu_levels[ Cur_menu_level ];
	Cur_menu_level++;
	Menu_levels[ Cur_menu_level ] = menu;
	strcpy( menu->name, name );
	menu->loc_row = -1;
	menu->loc_col = -1;
	menu->size_rows = 0;
	menu->size_cols = 0;
	menu->options = 0;
	menu->nbr_items = 0;
	menu->cur_item = 0;
	menu->auto_select_item = -1;
}


destroy_menu_level()
{
	struct menu_level *menu;
	struct menu_item *item;
	int i;

	menu = Menu_levels[ Cur_menu_level-- ];
	for( i = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];
		if ( item->dialog_box )
			destroy_dialog_box( item->dialog_box );
		free( item );
	}
	free( menu );
}


struct menu_item *create_menu_item( menu )
struct menu_level *menu;
{
	int itemndx;
	struct menu_item *item;

	itemndx = menu->nbr_items++;
	menu->items[itemndx] =
		( struct menu_item * )malloc( sizeof( struct menu_item ) );
	item = menu->items[itemndx];
	item->name[0] = 0;
	item->selection_char = 0;
	item->type = MENU;
	item->action[0][0] = 0;
	item->action[1][0] = 0;
	item->new_window_title[0] = 0;
	item->window_nbr = -1;
	item->dialog_box = 0;
	return( item );
}


check_menu_position( menu )
struct menu_level *menu;
{
	if ( menu->loc_col + menu->size_cols > Columns )
	{
		menu->loc_col -= ( menu->loc_col + menu->size_cols ) - Columns;
		if ( menu->loc_col < 0 )
			menu->loc_col = 0;
		if ( menu->type == PULL_DOWN_MENU )
		{
			if ( menu->prev_menu != 0 &&
			     menu->prev_menu->type != MENU_BAR )
			{
				menu->loc_row++;
			}
		}
	}
	if ( menu->loc_row + menu->size_rows > Lines )
	{
		menu->loc_row -= ( menu->loc_row + menu->size_rows ) - Lines;
		if ( menu->loc_row < 0 )
			menu->loc_row = 0;
	}
}

position_items( menu )
struct menu_level *menu;
{
	int i;

	for ( i = 0; i < menu->nbr_items; i++ )
		(*Menu_switch[menu->type].position_item)( menu, i );
}


struct dialog_box *create_dialog_box()
{
	struct dialog_box *dialog_box;
	int i;

	dialog_box =
		( struct dialog_box * )malloc( sizeof( struct dialog_box ) );
	dialog_box->nbr_items = 0;
	dialog_box->cur_item = -1;
	dialog_box->loc_row = -1;
	dialog_box->loc_col = -1;
	for( i = 0; i < MAX_DIALOG_BOX_ITEMS; i++ )
		dialog_box->items[ i ] = 0;
	return( dialog_box );
}

destroy_dialog_box( dialog_box )
struct dialog_box *dialog_box;
{
	int i;

	for ( i = 0; i < MAX_DIALOG_BOX_ITEMS; i++ )
		if ( dialog_box->items[ i ] != 0 )
			free( dialog_box->items[ i ] );
	free( dialog_box );
}

struct dialog_item *create_dialog_item( dialog_box )
struct dialog_box *dialog_box;
{
	int itemndx;
	struct dialog_item *dialog_item;

	itemndx = dialog_box->nbr_items++;
	dialog_box->items[ itemndx ] =
		( struct dialog_item * )malloc( sizeof( struct dialog_item ) );
	dialog_item = dialog_box->items[itemndx];
	dialog_item->prompt[0] = 0;
	dialog_item->variable = 0;
	dialog_item->max_var_len = 0;
	dialog_item->action = 0;
	return( dialog_item );
}


quit( clear_flag )
int clear_flag;
{
	if ( Facet_type == FACET_TERM )
	{
		fct_ioctl_close();
		ft_quit( Menu_window );
	}
	else
		fpc_quit();
	term_quit( clear_flag );
	exit( 0 );
}

signal_quit( ignore )
	int	ignore;
{
	quit( 1 );
}


extern char *getenv();

FILE *open_menu_file( menu_name )
char *menu_name;
{
	FILE *file;
	FILE *open_text();

	file = open_text( menu_name );
	return( file );
}


read_menu_file( menu )
struct menu_level *menu;
{
	int i;
	FILE *menu_file;
	char linebuff[ MAX_MENU_LINE_LENGTH + 1 ];
	char tokbuff[ MAX_MENU_LINE_LENGTH + MAX_MENU_ITEM_TOKENS + 1 ];
	char *token[ MAX_MENU_ITEM_TOKENS ];
	char *toksinline[ MAX_MENU_ITEM_TOKENS ];
	char c;
	int nbrtokens;
	int curtoken;
	int row, col;
	struct menu_item *item;
	struct dialog_item *dialog_item;

				/* try to open the menu file */

	if ( strlen( menu->name ) == 0 )
		return( -1 );
	if ( ( menu_file = open_menu_file( menu->name ) ) == NULL )
		return( -1 );

				/* set up default menu values */

	menu->type = PULL_DOWN_MENU;
	(*Menu_switch[ PULL_DOWN_MENU].position_menu)( menu );
	menu->title[0] = 0;
	menu->options = 0;
	menu->nbr_items = 0;
	menu->cur_item = 0;

				/* read values from menu file */
	while( 1 )
	{
		if ( fgets( linebuff, MAX_MENU_LINE_LENGTH, menu_file ) == NULL)
			break;
		while( 1 )
		{
			i = strlen( linebuff );
			if ( linebuff[i-1] == '\r' || linebuff[i-1] == '\n' )
				linebuff[i-1] = 0;
			else
				break;
		}
		if ( linebuff[0] == '#' || strlen( linebuff ) == 0 )
			continue;
		nbrtokens = tokenize( linebuff, " \t=", tokbuff, token,
				      toksinline, MAX_MENU_ITEM_TOKENS );
		curtoken = 0;
		if ( strcmp( token[curtoken], "menu_bar" ) == 0 )
		{
			row = -1;
			col = -1;
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				row = atoi( token[ curtoken ] );
				if ( next_token( &curtoken, nbrtokens, token ) )
				{
					col = atoi( token[ curtoken ] );
				}
			}
			if ( row >= 0 && col >= 0 )
			{
				menu->loc_row = row;
				menu->loc_col = col;
			}
			menu->type = MENU_BAR;
			(*Menu_switch[menu->type].position_menu)( menu );
		}
		else if ( strcmp( token[curtoken], "pull_down_menu" ) == 0 )
		{
			row = -1;
			col = -1;
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				row = atoi( token[ curtoken ] );
				if ( next_token( &curtoken, nbrtokens, token ) )
				{
					col = atoi( token[ curtoken ] );
				}
			}
			if ( row >= 0 && col >= 0 )
			{
				menu->loc_row = row;
				menu->loc_col = col;
			}
			menu->type = PULL_DOWN_MENU;
			(*Menu_switch[menu->type].position_menu)( menu );
		}
		else if ( strcmp( token[curtoken], "menu_title" ) == 0 )
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				strncpy( menu->title, toksinline[curtoken],
					 MAX_MENU_LEVEL_TITLE );
				menu->title[ MAX_MENU_LEVEL_TITLE ] = 0;
			}
		}
		else if ( strcmp( token[curtoken], "item_name" ) == 0 )
		{
			if ( menu->nbr_items < MAX_MENU_LEVEL_ITEMS )
			{
				item = create_menu_item( menu );
				if ( next_token( &curtoken, nbrtokens, token ) )
				{
					strncpy( item->name,
						toksinline[curtoken],
						MAX_MENU_ITEM_NAME );
					item->name[MAX_MENU_ITEM_NAME] = 0;
				}
			}
		}
		else if ( strcmp( token[curtoken], "item_selection" ) == 0 &&
			  item != 0 )
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				item->selection_char = *token[ curtoken ];
			}
		}
		else if ( strcmp( token[curtoken], "item_type" ) == 0 &&
			  item != 0 )
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				if ( strcmp( token[curtoken], "program" ) == 0 )
					item->type = PROGRAM;
				else if ( strcmp( token[curtoken], "menu" )
					  == 0 )
					item->type = MENU;
				else if ( strcmp( token[curtoken], "intrinsic" )
					  == 0 )
					item->type = INTRINSIC;
				else if ( strcmp( token[curtoken],
						  "ft_command" ) == 0 ||
					  strcmp( token[curtoken],
						  "tsm_command" ) == 0 )
					item->type = FT_COMMAND;
				else if ( strcmp( token[curtoken],
						  "ft_cmd_to_user" ) == 0 ||
					  strcmp( token[curtoken],
						  "tsm_cmd_to_user" ) == 0 )
					item->type = FT_CMD_TO_USER;
				else if ( strcmp( token[curtoken],
						  "menu_program" ) == 0 )
					item->type = MENU_PROGRAM;
			}
		}
		else if ( strcmp( token[curtoken], "item_action" ) == 0 &&
			  item != 0)
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				strncpy( item->action[0], toksinline[curtoken],
					MAX_MENU_ITEM_ACTION );
				item->action[0][MAX_MENU_ITEM_ACTION] = 0;
			}
		}
		else if ( strcmp( token[curtoken], "item_action_2" ) == 0 &&
			  item != 0)
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				strncpy( item->action[1], toksinline[curtoken],
					MAX_MENU_ITEM_ACTION );
				item->action[1][MAX_MENU_ITEM_ACTION] = 0;
			}
		}
		else if ( strcmp( token[curtoken], "new_window_title" ) == 0 &&
			  item != 0)
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				strncpy( item->new_window_title,
					 toksinline[curtoken],
					 MAX_MENU_ITEM_NAME );
				item->new_window_title[MAX_MENU_ITEM_NAME] = 0;
			}
		}
		else if ( strcmp( token[curtoken], "item_auto_select" ) == 0 &&
			  item != 0)
		{
			if ( menu->auto_select_item == -1 )
			{
				menu->auto_select_item = menu->nbr_items - 1;
			}
		}
		else if ( strcmp( token[curtoken], "dialog_box" ) == 0 &&
			  item != 0 )
		{
			row = -1;
			col = -1;
			item->dialog_box = create_dialog_box();
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				row = atoi( token[ curtoken ] );
				if ( next_token( &curtoken, nbrtokens, token ) )
				{
					col = atoi( token[ curtoken ] );
				}
			}
			if ( row >= 0 && col >= 0 )
			{
				item->dialog_box->loc_row = row;
				item->dialog_box->loc_col = col;
			}
		}
		else if ( strcmp( token[curtoken], "dialog_item_prompt") == 0 &&
			  item != 0 && item->dialog_box != 0 )
		{
			if ( next_token( &curtoken, nbrtokens, token ) )
			{
				dialog_item =
					create_dialog_item( item->dialog_box );
				strncpy( dialog_item->prompt,
					 toksinline[curtoken],
					 MAX_DIALOG_BOX_PROMPT );
				dialog_item->prompt[MAX_DIALOG_BOX_PROMPT] = 0;
			}
		}
		else if ( strcmp( token[curtoken], "dialog_item_var" ) == 0 &&
			  item != 0 && item->dialog_box != 0 )
		{
			if ( !next_token( &curtoken, nbrtokens, token ) )
				continue;
			if ( item->dialog_box->nbr_items <= 0 )
				continue;
			c = *( token[curtoken] + 1 );
			if ( c < '0' || c > '9' ||
			     strlen( token[ curtoken ] ) != 2 )
			{
				continue;
			}
			dialog_item->variable = String_variables[ c - '0' ];
			if ( !next_token( &curtoken, nbrtokens, token ) )
				continue;
			dialog_item->max_var_len = atoi( token[ curtoken ] );
			dialog_item->action = ENTER_STRING;
		}
		else
		{
			/*printf("Unknown menu entry %s\r\n",linebuff);*/
		}
	}
	fclose( menu_file );
	return( 0 );
}

/*
 * determines if the next token exists, and skips over a token of "="
 * as second token on line
 */

next_token( curtoken, nbrtokens, tokens )
int *curtoken;
int nbrtokens;
char *tokens[];
{
	( *curtoken )++;
	if ( nbrtokens <= *curtoken )
		return( 0 );
	if ( ( *curtoken == 1 ) && ( strcmp( tokens[*curtoken], "=" ) == 0 ) )
	{
		( *curtoken )++;
		if ( nbrtokens <= *curtoken )
			return( 0 );
	}
	return( 1 );
}

go()
{
	int c;

	while( 1 )
	{
		term_clear_screen();

			/* wait for a key */
		do
		{
			c = get_key();
			if ( c == Intr_char )
			{
				quit( 1 );
				break; /* NOTREACHED */
			}
		} while( c != Menu_hot_key_char &&
			 c != Notify_when_current_char );
		First_key = 2;

			/* get info needed for intrinsic variables */

		Current_window = ft_get_current_window();
		ask_window_title( Current_window, Current_window_title,
				  MAX_CURRENT_WINDOW_TITLE );

			/* put the top level menu on item 1 */

		Menu_levels[0]->cur_item = 0;

			/* reset the pop-up level for the first level menu */

		Pop_up_level = -1;

		do_menu();
	}
}


do_menu()
{
	struct menu_level *menu;
	unsigned int c;
	struct menu_item *item;

	menu = Menu_levels[ Cur_menu_level ];

	mark_screen();
	(*Menu_switch[menu->type].draw_menu)( menu );
	if ( menu->type == MENU_BAR )
		Auto_drop_down = 0;
	Go_back_to_top_level = 0;

	if ( menu->auto_select_item != -1 )
	{
		push_key( menu->items[menu->auto_select_item]->selection_char );
	}

	while( 1 )
	{
		item = menu->items[ menu->cur_item ];
		term_cursor_address( item->row, (int) item->col + 1 );
		if ( menu->type == MENU_BAR && how_many_pushed_keys() == 0 &&
		     ( item->type == MENU || ( item->type == INTRINSIC &&
		       strcmp( item->action[0], "win_menu" ) == 0 ) ) &&
		     Auto_drop_down )
		{
			c = FTKEY_DOWN;
		}
		else
		{
			c = get_key();
		}
		if ( c == Menu_hot_key_char )
		{
			if ( First_key )
			{
				First_key = 0;
				if ( Current_window != Menu_window )
				{
					/*PASTE HOT KEY*/
					ft_paste_to_window( Current_window,
							    Menu_hot_key );
					ft_send_exit_window_mode();
				}
			}
		}
		else if ( c == Windows_window_char_stop )
		{
			Go_back_to_top_level = 1;
			break;
		}
		else if ( c == Intr_char && Cur_menu_level == 0 )
		{
			quit( 1 );
		}
		else
		{
			if ( ( *Menu_switch[ menu->type ].interp_key )( menu,
				c ) )
			{
				if ( Cur_menu_level == 0 &&
				     !Go_back_to_top_level )
				{
					if ( Facet_type == FACET_TERM )
						ft_send_exit_window_mode();
					else
						fpc_pop_down();
				}
				else
				{
					break;
				}
			}
		}
		if ( Go_back_to_top_level )
		{
			break;
		}
	}
	restore_screen();
}


dec_cur_item( menu )
struct menu_level *menu;
{
	if ( menu->nbr_items == 0 )
		return;
	if ( menu->cur_item == 0 )
		menu->cur_item = menu->nbr_items - 1;
	else
		menu->cur_item--;
}


inc_cur_item( menu )
struct menu_level *menu;
{
	if ( menu->nbr_items == 0 )
		return;
	if ( menu->cur_item == menu->nbr_items - 1 )
		menu->cur_item = 0;
	else
		menu->cur_item++;
}


/* Returns: 0 if staying on this select menu, menu, or pull down
 *	    non-zero if returning to previous level. ( Note that
 *					this is currently only used by
 *					the select menu with no items. )
 */
execute_cur_item( menu )
struct menu_level *menu;
{
	struct menu_item *item;
	char buff[160];
	char buff2[160];
	char menu_name[MAX_MENU_LEVEL_NAME+1];
	int winno;
	int child_pid;
	int active[11];

	if ( menu->nbr_items == 0 )
		return( 0 );
	item = menu->items[menu->cur_item];
	if ( item->dialog_box )
	{
		if ( do_dialog_box( menu, item->dialog_box ) )
			return( 0 );
	}
	instantiate_string( item->action[0], buff );
	if ( item->type == MENU )
	{
		strncpy( menu_name, buff, MAX_MENU_LEVEL_NAME );
		menu_name[ MAX_MENU_LEVEL_NAME ] = 0;
		create_menu_level( menu_name );
		if ( read_menu_file( Menu_levels[ Cur_menu_level ] ) == 0 )
			do_menu();
		else
			Auto_drop_down = 0;
		destroy_menu_level();
	}
	else if ( item->type == PROGRAM )
	{
		highlight_cur_item( menu, FM_ATTR_HIGHLIGHT_BLINK );
		instantiate_string( item->new_window_title, buff2 );
		get_active_windows( active );
		winno = Current_window;
		if ( active[ winno ] )
			winno = -1;
		if ( buff2[0] == 0 )
			winno = run_program( item->name, buff, winno );
		else
			winno = run_program( buff2, buff, winno );
		if ( winno <= 0 )
		{
		}
		else
		{
			/*sleep( 1 );*/
			if ( Facet_type == FACET_TERM )
				ft_send_select_window( winno );
			else
				fpc_send_select_window( winno );
		}
		highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
	}
	else if ( item->type == INTRINSIC )
	{
		if ( strcmp( buff, "win_menu" ) == 0 )
		{
			int	numopen;

			create_menu_level( "win_menu" );
			numopen = build_window_selection_menu( "" );
			if ( Opt_quit_on_all_windows_idle && ( numopen == 0 ) )
			{
				ft_send_exit_window_mode();
				quit( 1 );
			}
			do_menu();
			destroy_menu_level();
		}
		if ( strcmp( buff, "win_menu_with_title" ) == 0 )
		{
			int	numopen;

			create_menu_level( "win_menu" );
			numopen = build_window_selection_menu(
					Text_window_selection_menu_title );
			if ( Opt_quit_on_all_windows_idle && ( numopen == 0 ) )
			{
				ft_send_exit_window_mode();
				quit( 1 );
			}
			do_menu();
			destroy_menu_level();
		}
		else if ( strcmp( buff, "win_sel" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				ft_send_select_window( item->window_nbr );
			else
				fpc_send_select_window( item->window_nbr );
		}
		else if ( strcmp( buff, "start_cut" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				ft_send_command_then_user( "c" );
		}
		else if ( strcmp( buff, "do_paste" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				ft_send_command( "p" );
		}
		else if ( strcmp( buff, "print_window" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				ft_send_command( "o" );
		}
		else if ( strcmp( buff, "start_shell" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				ft_send_command( "s" );
		}
		else if ( strcmp( buff, "quit_ft" ) == 0 ||
			  strcmp( buff, "quit_tsm" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				ft_send_command( "qy" );
		}
		else if ( strcmp( buff, "hangup" ) == 0 )
		{
			if ( Facet_type == FACET_TERM )
				kill_processes();
		}
		else if ( strcmp( buff, "options" ) == 0 )
		{
		}
		else if ( strcmp( buff, "no_action" ) == 0 )
		{
		}
		else if ( strcmp( buff, "previous_level" ) == 0 )
		{
			Auto_drop_down = 0;
			return( 1 );
		}
	}
	else if ( item->type == FT_COMMAND )
	{
		ft_send_command( buff );
		if ( item->action[1][0] )
		{
			instantiate_string( item->action[1], buff );
			ft_send_command( buff );
		}
	}
	else if ( item->type == FT_CMD_TO_USER )
	{
		ft_send_command_then_user( buff );
	}
	else if ( item->type == MENU_PROGRAM )
	{
		child_pid = fork();
		if ( child_pid == 0 )		/* child */
		{
			mark_screen();
			create_menu_level( "MENU_PROG" );
			( *Menu_switch[ PULL_DOWN_MENU ].position_menu )
				( Menu_levels[ Cur_menu_level ] );
			sprintf( Rowenv, "MENU_ROW=%d",
				 Menu_levels[ Cur_menu_level ]->loc_row );
			putenv( Rowenv );
			sprintf( Colenv, "MENU_COL=%d",
				 Menu_levels[ Cur_menu_level ]->loc_col );
			putenv( Colenv );
			system( buff );
			destroy_menu_level();
			restore_screen();
			exit( 0 );
		}
		else if ( child_pid != -1 )	/* parent */
		{
			while( wait( 0 ) != child_pid );
		}
	}
	return( 0 );
}


do_dialog_box( menu, dialog_box )
struct menu_level *menu;
struct dialog_box *dialog_box;
{
	int return_value;
	int c;

	mark_screen();
	draw_dialog_box( menu, dialog_box );
	Go_back_to_top_level = 0;
	dialog_box->completed = 0;

	while( 1 )
	{
		c = get_key();
		if ( c == Menu_hot_key_char )
		{
			if ( First_key )
			{
				First_key = 0;
				if ( Current_window != Menu_window )
				{
					/*PASTE HOT KEY*/
					ft_paste_to_window( Current_window,
							    Menu_hot_key );
					ft_send_exit_window_mode();
				}
			}
		}
		else if ( c == Windows_window_char_stop )
		{
			Go_back_to_top_level = 1;
			return_value = 1;
			break;
		}
		else
		{
			if ( dialog_box_interp_key( dialog_box, c ) )
			{
				return_value = 1;
				break;
			}
		}
		if ( dialog_box->completed )
		{
			return_value = 0;
			break;
		}
	}
	restore_screen();
	return( return_value );
}


instantiate_string( argstring, instantiated_string )
char *argstring, *instantiated_string;
{
	int len, expanded_len;
	int i, j;
	char c;
	char buff[81];

	len = strlen( argstring );
	expanded_len = 0;
	for( i = 0; i < len; i++ )
	{
		c = argstring[i];
		if ( c == '$' )
		{
			if ( strncmp( &argstring[i+1], "current_window_number",
				      21 ) == 0 )
			{
				i += 21;
				j = itos( (unsigned) Current_window, buff, 10 );
				strcpy( &instantiated_string[ expanded_len ],
					buff );
				expanded_len += j;
			}
			else if ( strncmp( &argstring[i+1],
					   "current_window_title", 20 ) == 0 )
			{
				i += 20;
				strcpy( &instantiated_string[ expanded_len ],
					Current_window_title );
				expanded_len += strlen( Current_window_title );
			}
			else if ( strncmp( &argstring[i+1],
					   "ft_hot_key", 10 ) == 0 )
			{
				i += 10;
				strcpy( &instantiated_string[ expanded_len ],
					Ft_hot_key );
				expanded_len += strlen( Ft_hot_key );
			}
			else if ( strncmp( &argstring[i+1],
					   "tsm_hot_key", 11 ) == 0 )
			{
				i += 11;
				strcpy( &instantiated_string[ expanded_len ],
					Ft_hot_key );
				expanded_len += strlen( Ft_hot_key );
			}
			else if ( argstring[i+1] >= '0' &&
				  argstring[i+1] <= '9' )
			{
				j = argstring[ i + 1 ] - '0';
				i++;
				strcpy( &instantiated_string[ expanded_len ],
					String_variables[ j ] );
				expanded_len += strlen( 
					String_variables[ j ] );
			}
			else
				instantiated_string[ expanded_len++ ] = c;
		}
		else if ( c == '\\' )
		{
			i++;
			c = argstring[ i ];
			if ( c == 'r' )
				instantiated_string[ expanded_len++ ] = '\r';
			else if ( c == 'n' )
				instantiated_string[ expanded_len++ ] = '\n';
			else if ( c == 'b' )
				instantiated_string[ expanded_len++ ] = '\b';
			else
				instantiated_string[ expanded_len++ ] = c;
		}
		else
			instantiated_string[ expanded_len++ ] = c;
	}
	instantiated_string[ expanded_len ] = 0;
}

/* Key reading functions */

#define MAX_PUSHED_KEYS		40
unsigned int pushed_keys[ MAX_PUSHED_KEYS ];
int nbr_pushed_keys = 0;

#define MAX_BUFFERED_CHARS	20
unsigned char already_read_chars[ MAX_BUFFERED_CHARS ];
int nbr_already_read = 0;
int max_string_key = 1;

#define NBR_SPECIAL_KEYS	100
int Nbr_special_keys = 0;
struct special_keys
{
	char *key_string;
	int key_value;
};
struct special_keys special_keys[ NBR_SPECIAL_KEYS ];

struct key_list
{
	char	*key_name;
	int	key_value;
};

typedef struct key_list KEY_LIST;

KEY_LIST Key_list[] = 
{
	{ "key_up",		FTKEY_UP	},
	{ "key_down",		FTKEY_DOWN	},
	{ "key_right",		FTKEY_RIGHT	},
	{ "key_left",		FTKEY_LEFT	},
	{ "key_home",		FTKEY_HOME	},
	{ "key_backspace",	FTKEY_BACKSPACE	},
	{ "key_return",		FTKEY_RETURN	},
	{ "key_ignore",		FTKEY_IGNORE	},
	{ "",			0		}
};
/**************************************************************************
* extra_keys
*	TERMINAL DESCRIPTION PARSER module for "key_...".
**************************************************************************/
/*ARGSUSED*/
extra_keys( key_name, key_string )
	char	*key_name;
	char	*key_string;
{
	KEY_LIST	*k;
	char		*dec_encode();
	char		*encoded;

	for ( k = &Key_list[ 0 ]; k->key_name[0] != '\0'; k++ )
	{
		if ( strcmp( key_name, k->key_name ) == 0 )
		{
		    if ( *key_string != '\0' )
		    {
			if ( Nbr_special_keys < NBR_SPECIAL_KEYS )
			{
				special_keys[ Nbr_special_keys ].key_string =
						dec_encode( key_string );
				special_keys[ Nbr_special_keys ].key_value = 
						k->key_value;
				Nbr_special_keys++;
			}
			else
			{
				printf( "ERROR: Too many keys '%s'\r\n",
							key_name );
			}
		    }
		    return( 1 );
		}
	}
	return( 0 );
}

init_keys()
{
	int i, x;

	for ( i = 0; i < Nbr_special_keys; i++ )
	{
		if ( special_keys[ i ].key_string != NULL )
		{
			if ( ( x = strlen( special_keys[ i ].key_string ) ) >
			      max_string_key )
				max_string_key = x;
		}
	}
	flush_keys();
}

flush_keys()
{
	nbr_pushed_keys = 0;
	nbr_already_read = 0;
	ioctl( 0, TCFLSH, 0 );
}

get_key()
{
	int c;
#ifdef REDUCE_COAST
	int i;
	int c2;
#endif

	if ( nbr_pushed_keys > 0 )
	{
		c = pushed_keys[ nbr_pushed_keys - 1 ];
		nbr_pushed_keys--;
	}
	else
	{
		while ( ( c = read_key( 1 ) ) == 0 );
#ifdef REDUCE_COAST
		for ( i = 0; i < 10; i++ )
			if ( ( c2 = read_key( 0 ) ) != c )
				break;
		if ( c2 != c && c2 != 0 )
			push_key( c2 );
#endif
	}
	return( c );
}


read_key( wait_time )
int wait_time;
{
	int c;
	int nbr_read, nbr_used;
	int i;
	int vmin, vtime;

	nbr_read = 0;
	if ( nbr_already_read == 0 )
	{
		if ( wait_time )
		{
			vmin = max_string_key;
			vtime = 1;
		}
		else
		{
			vmin = 0;
			vtime = 0;
		}
		if ( set_vmin_vtime( 0, vmin, vtime ) )
			quit( 1 );
		while ( nbr_read == 0 )
		{
			nbr_read = read( 0,
				(char *)
				&already_read_chars[ nbr_already_read ],
				10 );
			if ( wait_time == 0 && nbr_read == 0 )
				return( 0 );
		}
		if ( nbr_read < 0 )
			quit( 1 );
		if ( First_key )
			First_key--;
	}
	nbr_already_read += nbr_read;
	already_read_chars[ nbr_already_read ] = '\0';
	if ( ( nbr_used = match_string_key( &c ) ) == 0 )
	{
		c = already_read_chars[0];
		nbr_used = 1;
	}
	for( i = nbr_used; i < nbr_already_read; i++ )
		already_read_chars[i-nbr_used] = already_read_chars[i];
	nbr_already_read -= nbr_used;
	already_read_chars[nbr_already_read] = '\0';
	return( c );
}

match_string_key( c )
int *c;
{
	int i;
	int string_len;
	int nbr_read;
	int second_read_timed_out;

	*c = already_read_chars[ 0 ] ;
	if ( *c == Menu_hot_key_char ||
	     *c == Windows_window_char_stop ||
	     *c == Notify_when_current_char )
	{
		return( 1 );
	}
	second_read_timed_out = 0;
	for ( i = 0; i < Nbr_special_keys; i++ )
	{
		if ( special_keys[ i ].key_string != NULL )
		{
			string_len = strlen( special_keys[i].key_string );
			while ( nbr_already_read < string_len &&
			        strncmp( (char *) already_read_chars,
				      special_keys[i].key_string,
				      nbr_already_read ) == 0 &&
			        second_read_timed_out == 0 )
			{
				if ( set_vmin_vtime( 0, 0, 10 ) )
				{
					quit( 1 );
				}
				nbr_read = read( 0,
					(char *)
					&already_read_chars[ nbr_already_read ],
					string_len - nbr_already_read );
				if ( nbr_read > 0 )
				{
					nbr_already_read += nbr_read;
					already_read_chars[ nbr_already_read ] =
									'\0';
				}
				else
				{
					second_read_timed_out = 1;
					break;
				}
			}
			if ( nbr_already_read >= string_len &&
			     strncmp( (char *) already_read_chars,
			      	special_keys[i].key_string,
				      string_len ) == 0 )
			{
				*c = special_keys[i].key_value;
				return( string_len );
			}
		}
	}
	return( 0 );
}


push_key( c )
int c;
{
	if ( nbr_pushed_keys >= MAX_PUSHED_KEYS )
		return;
	pushed_keys[ nbr_pushed_keys++ ] = c;
}

how_many_pushed_keys()
{
	return( nbr_pushed_keys );
}

/* Screen management functions */


highlight_cur_item( menu, attr )
struct menu_level *menu;
int attr;
{
	struct menu_item *item;
	char instantiated_string[ 81 ];

	if ( menu->nbr_items == 0 )
		return;
	item = menu->items[menu->cur_item];
	instantiate_string( item->name, instantiated_string );
	term_cursor_address( item->row, item->col );
	if ( Fm_magic_cookie )
	{
		term_write( "%a%s%a", attr, instantiated_string, FM_ATTR_NONE );
	}
	else
	{
		term_write( "%a %a%s%a %a", FM_ATTR_ITEM, attr,
			    instantiated_string,
			    FM_ATTR_ITEM, FM_ATTR_NONE );
	}
}


mark_screen()
{
	Pop_up_level++;
	if ( Facet_type == FACET_TERM )
	{
		if ( Pop_up_level == 0 )
			ft_send_popup_start( Menu_window );
		else
			ft_send_push_popup();
	}
	else
	{
		;/* FACETPC POP-UP */
	}
}

restore_screen()
{
	if ( Facet_type == FACET_TERM )
	{
		if ( Pop_up_level > 0 )
			ft_send_pop_popup();
	}
	else
	{
		;/* FACETPC POP-DOWN */
	}
	Pop_up_level--;
}


/* Intrinsic functions */

build_window_selection_menu( title )
char *title;
{
	struct menu_level *menu;
	struct menu_item *item;
	int active[11];
	int winno;
	int i;
	int len, maxlen;
	char buff[80];

	menu = Menu_levels[ Cur_menu_level ];
	menu->type = WINDOW_SELECTION_MENU;
	(*Menu_switch[ WINDOW_SELECTION_MENU].position_menu)( menu );
	strcpy( menu->title, title );
	menu->options = 0;

	get_active_windows( active );
	set_active_windows( active );
	menu->nbr_items = 0;
	menu->cur_item = 0;
	maxlen = 0;
	for( i = 1; i <= MAX_FACET_WINDOWS; i++ )
	{
		if ( active[i] && i != Menu_window )
		{
			item = create_menu_item( menu );
			if ( i < 10 )
				winno = i;
			else
				winno = 0;
			item->selection_char = winno + '0';
			itos( (unsigned) winno, item->name, 10 );
			strcat( item->name, " " );
			strcat( item->name, Remembered_name[i -1] );
			if ( maxlen < ( len = strlen( item->name ) ) )
				maxlen = len;
			item->type = INTRINSIC;
			strcpy( item->action[0], "win_sel" );
			item->window_nbr = i;
			if ( Current_window == i )
				menu->cur_item = menu->nbr_items - 1;
		}
	}
	if ( menu->nbr_items == 0 )
	{
		item = create_menu_item( menu );
		item->selection_char = '\r';
		/* "All windows except menu are idle" */
		strcat( item->name, Text_all_windows_except_menu_idle );
		item->type = INTRINSIC;
		strcpy( item->action[0], "previous_level" );
		item->window_nbr = 1;
		menu->cur_item =  0;
		menu->nbr_items = 1;
		return( 0 );
	}
	for ( i = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];
		len = strlen( item->name );
		if ( len < maxlen )
		{
			strncat( item->name, Blanks, maxlen - len );
			item->name[ maxlen ] = '\0';
		}
		if ( Facet_type == FACET_TERM )
			strcat( item->name, "   $ft_hot_key" );
		else
			strcat( item->name, "   Alt-" );
		buff[0] = item->selection_char;
		buff[1] = 0;
		strcat( item->name, buff );
	}
	return( menu->nbr_items );
}


win_sel_interp_key( menu, c )
struct menu_level *menu;
int c;
{
	int i;
	struct menu_item *item;
	int	pop_select_menu;

	for( i = 0; i < menu->nbr_items; i++ )
	{
		item = menu->items[i];
		if ( c == item->selection_char )
		{
			if ( menu->cur_item != i )
			{
				highlight_cur_item( menu, FM_ATTR_ITEM );
				menu->cur_item = i;
				highlight_cur_item( menu, FM_ATTR_HIGHLIGHT );
			}
			pop_select_menu = execute_cur_item( menu );
			if ( pop_select_menu )
			{
				Auto_drop_down = 0;
				return( 1 );
			}
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
		case '\b':
		case FTKEY_BACKSPACE:
			if ( menu->prev_menu->type == MENU_BAR )
				Auto_drop_down = 1;
			push_key( FTKEY_LEFT );
			return( 1 );

		case FTKEY_RIGHT:
			if ( menu->prev_menu->type == MENU_BAR )
				Auto_drop_down = 1;
			push_key( FTKEY_RIGHT );
			return( 1 );

		case '\r':
		case FTKEY_RETURN:
			pop_select_menu = execute_cur_item( menu );
			if ( pop_select_menu )
			{
				Auto_drop_down = 0;
				return( 1 );
			}
			return( 0 );

		default:
			return( 0 );
	}
}


set_active_windows( active )
int active[ 11 ];
{
	int i;

	for ( i = 1; i <= 10; i++ )
	{
		if ( active[ i ] )
		{
			if ( ask_window_title( i,
					Remembered_name[ i-1 ],
					MAX_MENU_ITEM_NAME ) >= 0 )
			{
				if ( Remembered_name[i-1][ 0 ] == '\0' )
				{
					strcpy( Remembered_name[ i-1 ],
						"(open)" );
				}
			}
			else
			{
				strcpy( Remembered_name[ i-1 ],
					"(open)" );
			}
		}
		else
		{
			Remembered_name[ i-1 ][ 0 ] = '\0';
		}
	}
}


kill_processes()
{
	int i;

	signal( SIGHUP, SIG_IGN );
	ft_send_command( "xhy" );
	for ( i = 0; i < 10; i++ )
	{
		if ( only_active( Menu_window ) )
			break;
		sleep( 1 );
	}
	signal( SIGHUP, signal_quit );
}


process_dotfacet()
{
	FILE    *open_dotfacet();
	FILE    *facetfile;
	char	buffer[ 101 ];
	char	tempbuff[ 256 ];

	sprintf( buffer, ".%s", Facetname );
	if ( ( facetfile = open_dotfacet( buffer ) ) == NULL )
		return;
	while ( read_dotfacet( buffer, 100, facetfile ) >= 0 )
	{
		if ( strncmp( buffer, "menu_hotkey=", 12 ) == 0 )
		{
			if ( buffer[ 12 ] == '^' &&
			     ( buffer[ 13 ] >= 'A' && buffer[ 13 ] <= '_' ) )
			{
				Menu_hot_key[ 0 ] = buffer[ 12 ];
				Menu_hot_key[ 1 ] = buffer[ 13 ];
				Menu_hot_key[ 2 ] = 0;
				Menu_hot_key_char = Menu_hot_key[ 1 ] - '@';
			}
			continue;
		}
		if ( strcmp( buffer, "quit_on_all_windows_idle" ) == 0 )
		{
			Opt_quit_on_all_windows_idle = 1;
			continue;
		}
		if ( strncmp( buffer, "notify_when_current_char=", 25 ) == 0 )
		{
			/**************************************************
			* The character to send to a window when it is
			* set to "notify_on_current" and it becomes current.
			**************************************************/
			string_encode( &buffer[ 25 ], tempbuff );
			Notify_when_current_char = *tempbuff;
			continue;
		}
		if ( strncmp( buffer, "windows_window_char_stop=", 25 ) == 0 )
		{
			/**************************************************
			* The character to send to a pop-up window when
			* the pop-up is cancelled.
			**************************************************/
			string_encode( &buffer[ 25 ], tempbuff );
			Windows_window_char_stop = *tempbuff;
			continue;
		}
		if ( strncmp( buffer, "window_mode_to_user_char=", 25 ) == 0 )
		{
			/**************************************************
			* The character to be used in a "WINDOW_MODE" ioctl
			* to indicate that the user must finish the command.
			* E.g. Put the program in cut mode and then user
			* must position and ok.
			**************************************************/
			string_encode( &buffer[ 25 ], tempbuff );
			Window_mode_to_user_char = *tempbuff;
			continue;
		}
	}
}
#ifdef lint
static int lint_alignment_warning_ok_4;
#endif
