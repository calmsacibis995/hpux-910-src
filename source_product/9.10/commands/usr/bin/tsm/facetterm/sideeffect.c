/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: sideeffect.c,v 70.1 92/03/09 16:42:48 ssa Exp $ */

#include "ftwindow.h"
extern int	errno;

#define SE_auto_wrap_off		1
#define SE_clear_screen			2
#define SE_cursor_on			3
#define SE_exit_insert_mode		4
#define SE_exit_appl_keypad_mode	5
#define SE_keypad_local			6
#define SE_scroll_region		7
#define SE_character_set		8
#define SE_attributes_off		9
#define SE_save_cursor			10
#define SE_mode				11
#define SE_perwindow_clear		12
#define SE_no_status			13
#define SE_switch_page_number_0		14
#define SE_clears_pages			15
#define SE_rows_change			16

win_se( side_effects )
	long	side_effects[];
{
	int	i;
	long	e;

	for ( i = 0; (e = side_effects[ i ]) > 0; i++ )
	{
		if (      e == SE_auto_wrap_off )
			win_se_auto_wrap_off();
		else if ( e == SE_clear_screen )
		{
			Outwin->xenl = 0;
			Outwin->real_xenl = 0;
			d_blankwin();
			Outwin->row = 0;
			Outwin->col = 0;
			b_cursor_home();		/* pan buffer to top */
		}
		else if ( e == SE_cursor_on )
			win_se_cursor( 1 );
		else if ( e == SE_exit_insert_mode )
			win_se_insert_mode( 0 );
		else if ( e == SE_exit_appl_keypad_mode )
			win_se_appl_keypad_mode( 0 );
		else if ( e == SE_keypad_local )
			win_se_keypad_xmit( 0 );
		else if ( e == SE_scroll_region )
			win_se_scroll_region_normal();
		else if ( e == SE_character_set )
			win_se_character_set_normal();
		else if ( e == SE_attributes_off )
		{
			FTCHAR	attributes_off;

			i++;
			attributes_off = side_effects[ i ];
			win_se_attributes_off( attributes_off );
		}
		else if ( e == SE_save_cursor )
			init_save_cursor();
		else if ( e == SE_mode )
		{
			long	mode_on;
			long	mode_off;

			i++;
			mode_on = side_effects[ i ];
			i++;
			mode_off = side_effects[ i ];

			win_se_mode( mode_on, mode_off );
		}
		else if ( e == SE_perwindow_clear )
		{
			char	clears_perwindow[ 2 ];

			i++;
			clears_perwindow[ 0 ] = side_effects[ i ];
			clears_perwindow[ 1 ] = '\0';
			win_se_perwindow_clear_string( clears_perwindow,
						       Outwin->number );
		}
		else if ( e == SE_no_status )
			win_se_no_status();
		else if ( e == SE_switch_page_number_0 )
		{
			/* term only */
		}
		else if ( e == SE_clears_pages )
		{
			/* term only */
		}
		else if ( e == SE_rows_change )
		{
			int	rows_changeno;

			i++;
			rows_changeno = side_effects[ i ];
			fct_rows_change( rows_changeno );
		}
		else
		{
			term_beep();
		}
	}
}
term_se( side_effects )
	long	side_effects[];
{
	int	i;
	long	e;

	for ( i = 0; (e = side_effects[ i ]) > 0; i++ )
	{
		if (      e == SE_auto_wrap_off )
			term_se_auto_wrap_off();
		else if ( e == SE_clear_screen )
		{
			/* win only */
		}
		else if ( e == SE_cursor_on )
			term_se_cursor( 1 );
		else if ( e == SE_exit_insert_mode )
			term_se_insert_mode( 0 );
		else if ( e == SE_exit_appl_keypad_mode )
			term_se_appl_keypad_mode( 0 );
		else if ( e == SE_keypad_local )
			term_se_keypad_xmit( 0 );
		else if ( e == SE_scroll_region )
			term_se_scroll_region_normal();
		else if ( e == SE_character_set )
			term_se_character_set_normal();
		else if ( e == SE_attributes_off )
		{
			FTCHAR	attributes_off;

			i++;
			attributes_off = side_effects[ i ];
			term_se_attribute_off( attributes_off );
		}
		else if ( e == SE_save_cursor )
		{
			/* save_cursor is software - not terminal */
		}
		else if ( e == SE_mode )
		{
			long	mode_on;
			long	mode_off;

			i++;
			mode_on = side_effects[ i ];
			i++;
			mode_off = side_effects[ i ];

			term_se_mode( mode_on, mode_off );
		}
		else if ( e == SE_perwindow_clear )
		{
			char	clears_perwindow[ 2 ];

			i++;
			clears_perwindow[ 0 ] = side_effects[ i ];
			clears_perwindow[ 1 ] = '\0';
			term_se_perwindow_default_string( clears_perwindow );
		}
		else if ( e == SE_no_status )
			term_se_no_status();
		else if ( e == SE_switch_page_number_0 )
			term_se_switch_page_number_0();
		else if ( e == SE_clears_pages )
			forget_all_pages();
		else if ( e == SE_rows_change )
		{
			int	rows_changeno;

			i++;
			rows_changeno = side_effects[ i ];
			term_rows_change( rows_changeno );
		}
		else
		{
			term_beep();
		}
	}
}
encode_se( new_side_effects, side_effects, max )
	char	*new_side_effects;
	long	side_effects[];
	int	max;
{
	int	i;
	char	*s;
	char	*p;
	char	*f;
	long	attribute_encode();
	long	mode_encode();

	for ( i = 0; side_effects[ i ] > 0; i++ )
	{
		if ( i >= max - 2 )
		{
			printf( "Too many side effects\r\n" );
			return( -1 );
		}
	}
	s = new_side_effects;
	for( s = new_side_effects; *s != '\0'; s = p )
	{
		if ( i >= max - 2 )
		{
			printf( "Too many side effects\r\n" );
			return( -1 );
		}
		for ( p = s; ( *p != '\0' ) && ( *p != ',' ); p++ )
			;
		if ( *p == ',' )
			*p++ = '\0';
		if ( strcmp( s , "auto_wrap_off" ) == 0 )
		{
			side_effects[ i++ ] = SE_auto_wrap_off;
		}
		else if ( strcmp( s, "clear_screen" ) == 0 )
		{
			side_effects[ i++ ] = SE_clear_screen;
		}
		else if ( strcmp( s, "cursor_on" ) == 0 )
		{
			side_effects[ i++ ] = SE_cursor_on;
		}
		else if ( strcmp( s, "exit_insert_mode" ) == 0 )
		{
			side_effects[ i++ ] = SE_exit_insert_mode;
		}
		else if ( strcmp( s, "exit_appl_keypad_mode" ) == 0 )
		{
			side_effects[ i++ ] = SE_exit_appl_keypad_mode;
		}
		else if ( strcmp( s, "keypad_local" ) == 0 )
		{
			side_effects[ i++ ] = SE_keypad_local;
		}
		else if ( strcmp( s, "scroll_region" ) == 0 )
		{
			side_effects[ i++ ] = SE_scroll_region;
		}
		else if ( strcmp( s, "character_set" ) == 0 )
		{
			side_effects[ i++ ] = SE_character_set;
		}
		else if ( strcmp( s, "attributes_off" ) == 0 )
		{
			if ( *p == '\0' )
			{
				printf( "side effect mode w/o modes\r\n" );
				return( -1 );
			}
			s = p;
			for ( ; ( *p != '\0' ) && ( *p != ',' ); p++ )
				;
			if ( *p == ',' )
				*p++ = '\0';
			side_effects[ i++ ] = SE_attributes_off;
			side_effects[ i++ ] = attribute_encode( s );
		}
		else if ( strcmp( s, "save_cursor" ) == 0 )
		{
			side_effects[ i++ ] = SE_save_cursor;
		}
		else if ( strcmp( s, "mode" ) == 0 )
		{
			char	*f;

			if ( *p == '\0' )
			{
				printf( "side effect mode w/o modes\r\n" );
				return( -1 );
			}
			f = p;
			for ( ; ( *p != '\0' ) && ( *p != ',' ); p++ )
				;
			if ( *p == ',' )
				*p++ = '\0';
			if ( *p == '\0' )
			{
				printf( "side effect mode w/o modes\r\n" );
				return( -1 );
			}
			s = p;
			for ( ; ( *p != '\0' ) && ( *p != ',' ); p++ )
				;
			if ( *p == ',' )
				*p++ = '\0';
			side_effects[ i++ ] = SE_mode;
			side_effects[ i++ ] = mode_encode( f );
			side_effects[ i++ ] = mode_encode( s );
		}
		else if ( strcmp( s, "perwindow_clear" ) == 0 )
		{
			if ( *p == '\0' )
			{
				printf(
				"side effect perwindow_clear w/o letter\r\n" );
				return( -1 );
			}
			side_effects[ i++ ] = SE_perwindow_clear;
			side_effects[ i++ ] = *p++;
			for ( ; ( *p != '\0' ) && ( *p != ',' ); p++ )
				;
			if ( *p == ',' )
				*p++ = '\0';
		}
		else if ( strcmp( s, "no_status" ) == 0 )
		{
			side_effects[ i++ ] = SE_no_status;
		}
		else if ( strcmp( s, "switch_page_number_0" ) == 0 )
		{
			side_effects[ i++ ] = SE_switch_page_number_0;
		}
		else if ( strcmp( s, "clears_pages" ) == 0 )
		{
			side_effects[ i++ ] = SE_clears_pages;
		}
		else if ( strcmp( s, "rows_change" ) == 0 )
		{
			if ( *p == '\0' )
			{
				printf(
				"side effect rows_change w/o number\r\n" );
				return( -1 );
			}
			side_effects[ i++ ] = SE_rows_change;
			side_effects[ i++ ] = atoi( p );
			for ( ; ( *p != '\0' ) && ( *p != ',' ); p++ )
				;
			if ( *p == ',' )
				*p++ = '\0';
		}
		else
		{
			printf( "side effect unknown '%s'\r\n", s );
		}
		side_effects[ i ] = 0;
	}
	return( i );
}
#define MAX_ALLOCATE_SE 50
long *
allocate_se( new_side_effects )
	char	*new_side_effects;
{
	long	side_effects[ MAX_ALLOCATE_SE ];/* temporary hold side effects*/
	long    *malloc_run();
	long	*storage;			/* malloc to hold side effects*/
	int	num;				/* number of side effects */
	int	need;				/* bytes of storage */
	int	i;

	side_effects[ 0 ] = 0;
	num = encode_se( new_side_effects, side_effects, MAX_ALLOCATE_SE );
	if ( num <= 0 )
		return( (long *) 0 );
	need = ( num + 1 ) * sizeof( long );
	storage = (long *) malloc_run( need, "allocate_se" );
	if ( storage == (long *) 0 )
	{
		printf( "ERROR: side_effect allocate failed - %d\r\n",
				errno );
		term_outgo();
		return( (long *) 0 );
	}
	for ( i = 0; i < ( num + 1 ); i++ )
		storage[ i ] = side_effects[ i ];
	return( storage );
}
