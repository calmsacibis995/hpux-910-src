/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftoptions.c,v 70.1 92/03/09 15:43:38 ssa Exp $ */
/**************************************************************************
* ftoptions.c
*	Gather user-configurable options.
**************************************************************************/
#include "stdio.h"
#define CONTROL ( -0x40 )
#include "baud.h"
#include "options.h"
int	Opt_decode = 1;
int	Opt_function_keys = 1;
int	Opt_nonstop = 0;
int	Opt_hotkey = CONTROL + 'W';
int	Opt_no_prompt_hotkey = -1;
int	Opt_quiet_switch_when_prompting_off = 0;

int	Opt_menu_hotkey = CONTROL + 'T';
int	Opt_menu_hotkey_set = 0;

int	Opt_capture_no_buffer = 0;
int	Opt_error_ignore = 0;
int	Opt_error_pass = 0;
int	Opt_error_control_ignore = 0;
int	Opt_error_control_pass = 0;
int	Opt_rows_scroll_memory = -1;
int	Opt_min_scroll_memory_baud = INVALID_BAUD_RATE;
int	Opt_malloc_record = 0;

int	Opt_use_PARMRK_for_break = 1;
int	Opt_send_break_to_window = 1;

#include "wins.h"
#include "ft_notbusy.h"
int	This_is_ft_notbusy = 0;

char	Ft_arg_facetterm[ 40 ] = "FACETTERM=";
char	Ft_arg_tsmterm[ 40 ] = "TSMTERM=";

#include "facetpath.h"

/**************************************************************************
* set_options
*	Extract options from command line, environment, and .facet file.
**************************************************************************/
set_options( argc, argv )
	int	argc;
	char	**argv;
{
	char	*p;
	char	*getenv();
	int	i;
	int	numwins;

	set_options_facetpath();
	set_options_dotfacet();
	p = getenv( "DECODE" );
	if ( p != NULL && p[0] == 'N' )
		Opt_decode = 0;

	p = getenv( "FUNCTION_KEYS" );
	if ( p != NULL && p[0] == 'N' )
		Opt_function_keys = 0;

	p = getenv( "FACETHOTKEY" );
	if ( p != NULL )
		set_hotkey( p );

	p = getenv( "FACETNOPROMPTHOTKEY" );
	if ( p != NULL )
		set_no_prompt_hotkey( p );

	p = getenv( "FACETCAPTUREDIR" );
	if ( p != NULL )
		set_capture_directory( p );

	p = getenv( "FACETERROR" );
	if ( p != NULL )
	{
		if ( p[0] == 'P' )
			Opt_error_pass = 1;
		else
			Opt_error_ignore = 1;
	}

	for ( i = 1; i < argc; i++ )
	{
		numwins = atoi( argv[ i ] );
		if ( ( numwins > 0 ) && ( numwins <= 10 ) )
		{
			Wins = numwins;
		}
		else if ( strcmp( argv[ i ], "nonstop" ) == 0 )
		{
			Opt_nonstop = 1;
		}
		else if ( strncmp( argv[ i ], "hotkey=", 7 ) == 0 )
		{
			p = argv[ i ];
			p += 7;
			set_hotkey( p );
		}
		else if ( strncmp( argv[ i ], "no_prompt_hotkey=", 17 ) == 0 )
		{
			p = argv[ i ];
			p += 17;
			set_no_prompt_hotkey( p );
		}
		else if ( strcmp( argv[ i ], "quiet_switch_when_prompting_off" )
			  == 0 )
		{
			Opt_quiet_switch_when_prompting_off = 1;
		}
		else if ( strcmp( argv[ i ], "error_ignore" ) == 0 )
		{
			Opt_error_ignore = 1;
		}
		else if ( strcmp( argv[ i ], "error_pass" ) == 0 )
		{
			Opt_error_pass = 1;
		}
		else if ( strcmp( argv[ i ], "error_control_ignore" ) == 0 )
		{
			Opt_error_control_ignore = 1;
		}
		else if ( strcmp( argv[ i ], "error_control_pass" ) == 0 )
		{
			Opt_error_control_pass = 1;
		}
		else if ( strncmp( argv[ i ], "scroll_memory_lines=", 20 ) == 0)
		{
			p = argv[ i ];
			p += 20;
			set_scroll_memory_lines( p );
			set_min_scroll_memory_baud( "300" );
		}
		else if ( strcmp( argv[ i ], "malloc_record" ) == 0 )
		{
			Opt_malloc_record = 1;
		}
		else if ( strlen( argv[ i ] ) < 20 )
		{
			strcat( Ft_arg_facetterm, argv[ i ] );
			putenv( Ft_arg_facetterm );
			strcat( Ft_arg_tsmterm, argv[ i ] );
			putenv( Ft_arg_tsmterm );
		}
	}
	if ( strcmp( argv[ 0 ], "ft_notbusy" ) == 0 )
		This_is_ft_notbusy = 1;
}
#define FACETFILEMAX 256
#include "func.h"
#include "printtimer.h"

#include "screensave.h"
int	Screen_saver_timer = 0;
int	Screen_saver_timer_in_dotfacet = 0;

#include "printonly.h"
int Opt_print_only_hp_personality = 0;

#include "cut.h"

/**************************************************************************
* set_options_dotfacet
*	Read options from the .facet file.
**************************************************************************/
set_options_dotfacet()
{
	FILE	*open_dotfacet();
	FILE	*facetfile;
	char	buffer[ FACETFILEMAX + 1 ];
	char	dotfacetname[ 20 ];
	char	*getenv();
	int	facet_echo_dotfacet;
	int	numwins;


	sprintf( dotfacetname, ".%s", Facetname );
	facetfile = open_dotfacet( dotfacetname );
	if ( facetfile == NULL )
		return;
	facet_echo_dotfacet = 0;
	if ( getenv( "FACETECHODOTFACET" ) != ( (char *) 0 ) )
		facet_echo_dotfacet = 1;
	while ( read_dotfacet( buffer, FACETFILEMAX, facetfile ) >= 0 )
	{
		if ( facet_echo_dotfacet )
		{
			printf( ".facet: %s\r\n", buffer );
		}
		if ( strncmp( buffer, "hotkey=", 7 ) == 0 )
		{
			set_hotkey( &buffer[ 7 ] );
		}
		else if ( strncmp( buffer, "no_prompt_hotkey=", 17 ) == 0 )
		{
			set_no_prompt_hotkey( &buffer[ 17 ] );
		}
		else if ( strcmp( buffer, "quiet_switch_when_prompting_off" )
			  == 0 )
		{
			Opt_quiet_switch_when_prompting_off = 1;
		}
		else if ( strncmp( buffer, "menu_hotkey=", 12 ) == 0 )
		{
			set_menu_hotkey( &buffer[ 12 ] );
		}
		else if ( strncmp( buffer, "windows=", 8 ) == 0 )
		{
			numwins = atoi( &buffer[ 8 ] );
			if ( ( numwins > 0 ) && ( numwins <= 10 ) )
			{
				Wins = numwins;
			}
		}
		else if ( strncmp( buffer, "capture_directory=", 18 ) == 0 )
		{
			set_capture_directory( &buffer[ 18 ] );
		}
		else if ( strcmp( buffer, "capture_no_buffer" ) == 0 )
		{
			Opt_capture_no_buffer = 1;
		}
		else if ( strcmp( buffer, "no_assume_default_function_keys" ) 
			  == 0 )
		{
			F_assume_default_function_keys = 0;
		}
		else if ( strcmp( buffer, "error_ignore" ) == 0 )
		{
			Opt_error_ignore = 1;
		}
		else if ( strcmp( buffer, "error_pass" ) == 0 )
		{
			Opt_error_pass = 1;
		}
		else if ( strcmp( buffer, "error_control_ignore" ) == 0 )
		{
			Opt_error_control_ignore = 1;
		}
		else if ( strcmp( buffer, "error_control_pass" ) == 0 )
		{
			Opt_error_control_pass = 1;
		}
		else if ( strncmp( buffer, "scroll_memory_lines=", 20 ) == 0)
		{
			set_scroll_memory_lines( &buffer[ 20 ] );
		}
		else if ( strncmp( buffer, "min_scroll_memory_baud=", 23 ) == 0)
		{
			set_min_scroll_memory_baud( &buffer[ 23 ] );
		}
		else if ( strcmp( buffer, "send_break_to_window" ) == 0 )
		{
			Opt_send_break_to_window = 1;
		}
		else if ( strcmp( buffer, "send_break_to_facetterm" ) == 0 )
		{
			Opt_send_break_to_window = 0;
		}
		else if ( strcmp( buffer, "send_break_to_tsm" ) == 0 )
		{
			Opt_send_break_to_window = 0;
		}
		else if ( strcmp( buffer, "use_PARMRK_for_break" ) == 0 )
		{
			Opt_use_PARMRK_for_break = 1;
		}
		else if ( strcmp( buffer, "use_NULL_for_break" ) == 0 )
		{
			Opt_use_PARMRK_for_break = 0;
		}
		else if ( strncmp( buffer, "transparent_print_buffer_size=",
								30 ) == 0 )
		{
			set_transparent_print_buffer_size( &buffer[ 30 ] );
		}
		else if ( strncmp( buffer, "transparent_print_window=",
								25 ) == 0 )
		{
			set_transparent_print_window( &buffer[ 25 ] );
		}
		else if ( strncmp( buffer, "transparent_print_quiet_timer=", 
								30 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 30 ] ) ) > 0 )
			{
				Transparent_print_quiet_timer = x;
			}
		}
		else if ( strncmp( buffer, "transparent_print_idle_timer=", 
								29 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 29 ] ) ) > 0 )
			{
				Transparent_print_idle_timer = x;
				Transparent_print_short_timer = x;
			}
		}
		else if ( strncmp( buffer, "transparent_print_idle_timer_max=", 
								33 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 33 ] ) ) > 0 )
			{
				Transparent_print_idle_timer_max = x;
			}
		}
		else if ( strncmp( buffer, "transparent_print_full_timer=", 
								29 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 29 ] ) ) > 0 )
			{
				Transparent_print_full_timer = x;
			}
		}
		else if ( strncmp( buffer, "transparent_print_delay_count=", 
								30 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 30 ] ) ) > 0 )
			{
				Transparent_print_delay_count = x;
			}
		}
		else if ( strcmp( buffer, "transparent_print_timers_disable" )
									== 0 )
		{
			Transparent_print_timers_disable = 1;
		}
		else if ( strncmp( buffer, "screen_saver_timer=", 19 ) == 0 )
		{
			int	x;

			Screen_saver_timer_in_dotfacet = 1;
			if ( ( x = atoi( &buffer[ 19 ] ) ) >= 0 )
			{
				Screen_saver_timer = x;
				if ( Screen_saver_timer == 0 )
				{
					Screen_saver_timer =
						DEFAULT_SCREEN_SAVER_TIMER;
				}
			}
		}
		else if ( strncmp( buffer, "screen_saver_interval=", 22 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 22 ] ) ) >= 0 )
			{
				Screen_saver_interval = x;
			}
		}
		else if ( strncmp( buffer, "screen_saver_text=", 18 ) == 0 )
		{
			int	len;
			char	*text;
			char	*store;
			long	*mymalloc();

			text = &buffer[ 18 ];
			len = strlen( text ) + 1;
			store = (char *) mymalloc( len, "screen_saver_text" );
			strcpy( store, text );
			User_text_screen_saver = store;
		}
		else if ( strncmp( buffer, "screen_lock_text=", 18 ) == 0 )
		{
			int	len;
			char	*text;
			char	*store;
			long	*mymalloc();

			text = &buffer[ 18 ];
			len = strlen( text ) + 1;
			store = (char *) mymalloc( len, "screen_lock_text" );
			strcpy( store, text );
			User_text_screen_lock = store;
		}
		else if ( strcmp( buffer, "kill_processes_on_shutdown" ) == 0 )
		{
			Opt_kill_processes_on_shutdown = 1;
		}
		else if ( strcmp( buffer, "no_kill_processes_on_shutdown" ) ==0)
		{
			Opt_kill_processes_on_shutdown = 0;
		}
		else if ( strcmp( buffer, "print_only_hp_personality" ) == 0 )
		{
			Opt_print_only_hp_personality = 1;
		}
		else if ( strcmp( buffer, "stream_cut" ) == 0 )
		{
			Cut_type = STREAM_CUT_TYPE;
		}
		else if ( strcmp( buffer, "wrap_cut" ) == 0 )
		{
			Cut_type = WRAP_CUT_TYPE;
		}
		else if ( strcmp( buffer, "block_cut" ) == 0 )
		{
			Cut_type = BLOCK_CUT_TYPE;
		}
		else if ( strncmp( buffer, "cut_and_paste_timer=", 20 ) == 0 )
		{
			int	x;

			if ( ( x = atoi( &buffer[ 20 ] ) ) >= 0 )
			{
				Cut_and_paste_timer = x;
			}
		}
		else
		{
			if ( facet_echo_dotfacet )
			{
				printf( "^^^^^ NOT THIS PASS ^^^^^\r\n" );
			}
		}
	}
	fclose( facetfile );
}
#define HOTKEY_MAX 10
/**************************************************************************
* set_hotkey
*	Set the hot-key that may have been specified in the .facet file
*	or the environment.
*	If it is null, the hot-key is disabled.
*	Otherwise it is set to the first character of the string "p".
**************************************************************************/
set_hotkey( p )
	char	*p;
{
	char	*temp_encode();
	char	temp_encode_store[ HOTKEY_MAX + 1 ];
	char	hotkey_store[ HOTKEY_MAX + 1 ];

	if ( *p == '\0' )
	{
		Opt_hotkey = -1;
	}
	else
	{
		strncpy( hotkey_store, p, HOTKEY_MAX );
		hotkey_store[ HOTKEY_MAX ] = '\0';
		temp_encode( hotkey_store, temp_encode_store );
		Opt_hotkey = *temp_encode_store;
	}
}
/**************************************************************************
* set_no_prompt_hotkey
*	Set the noprompt hot-key that may have been specified in the .facet file
*	or the environment.
*	If it is null, the hot-key is disabled.
*	Otherwise it is set to the first character of the string "p".
**************************************************************************/
set_no_prompt_hotkey( p )
	char	*p;
{
	char	*temp_encode();
	char	temp_encode_store[ HOTKEY_MAX + 1 ];
	char	hotkey_store[ HOTKEY_MAX + 1 ];

	if ( *p == '\0' )
	{
		Opt_no_prompt_hotkey = -1;
	}
	else
	{
		strncpy( hotkey_store, p, HOTKEY_MAX );
		hotkey_store[ HOTKEY_MAX ] = '\0';
		temp_encode( hotkey_store, temp_encode_store );
		Opt_no_prompt_hotkey = ( *temp_encode_store ) & 0x00FF;
	}
}
/**************************************************************************
* set_menu_hotkey
*	Set the menu hot-key that may have been specified in the .facet file
*	or the environment.
*	If it is null, the menu hot-key is disabled.
*	Otherwise it is set to the first character of the string "p".
*	A record or whether it is set is remembered to know whether to
*	output a prompt to the user on startup.
**************************************************************************/
set_menu_hotkey( p )
	char	*p;
{
	char	*temp_encode();
	char	temp_encode_store[ HOTKEY_MAX + 1 ];
	char	hotkey_store[ HOTKEY_MAX + 1 ];

	if ( *p == '\0' )
	{
		Opt_menu_hotkey = -1;
		Opt_menu_hotkey_set = 0;
	}
	else
	{
		strncpy( hotkey_store, p, HOTKEY_MAX );
		hotkey_store[ HOTKEY_MAX ] = '\0';
		temp_encode( hotkey_store, temp_encode_store );
		Opt_menu_hotkey = *temp_encode_store;
		Opt_menu_hotkey_set = 1;
	}
}
/**************************************************************************
* set_scroll_memory
*	The character string "string" contains a number of rows.
*	Set the number of rows of scroll memory to that number of rows.
**************************************************************************/
set_scroll_memory_lines( string )
	char	*string;
{
	int	rows;

	rows = atoi( string );
	if ( ( rows > 0 ) || ( ( rows == 0 ) && ( *string == '0' ) ) )
		Opt_rows_scroll_memory = rows;
	else
		printf( "invalid scroll_memory_lines =  '%s'\r\n", string );
}
/**************************************************************************
* set_min_scroll_memory_baud
*	The character string "string" contains a baud rate.
*	Set the mininum baud rate at which scroll memory is used to that
*	baud rate.
**************************************************************************/
set_min_scroll_memory_baud( string )
	char	*string;
{
	int	trans_baud;

	trans_baud = translate_baud( atoi( string ) );
	if ( trans_baud != INVALID_BAUD_RATE )
		Opt_min_scroll_memory_baud = trans_baud;
	else
		printf( "invalid min_scroll_memory_baud =  '%s'\r\n", string );
}
/**************************************************************************
* set_transparent_print_buffer_size
*	The character string "string" contains a print buffer size.
*	Set the maximum number of characters to be read from a window
*	in transparent print mode to this print buffer size.
**************************************************************************/
#include "readwindow.h"
set_transparent_print_buffer_size( string )
	char	*string;
{
	int	buffer;

	buffer = atoi( string );
	if ( ( buffer > 0 ) && ( buffer < READ_WINDOW_MAX ) )
		Transparent_print_read_window_max = buffer;
	else
		printf( "invalid transparent_print_buffer_size =  '%s'\r\n", 
								string );
}
/**************************************************************************
* set_transparent_print_window
*	The character string "string" contains a window number ( 1-10 or 0 ).
*	Mark the corresponding window to be in print mode.
**************************************************************************/
#include "printer.h"
set_transparent_print_window( string )
	char	*string;
{
	int	winno;

	winno = atoi( string );
	if ( ( winno == 0 ) && ( strcmp( string, "0" ) == 0 ) )
		winno = 9;
	else if ( ( winno >= 1 ) && ( winno <= 10 ) )
	{
		winno--;
	}
	else
	{
		printf( "invalid transparent_print_window =  '%s'\r\n", 
								string );
		return;
	}
	Window_printer[ winno ] = 1;
	Read_window_max[ winno ] = Transparent_print_read_window_max;
	set_window_printer_mask();
}
