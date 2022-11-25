/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftextra.c,v 70.1 92/03/09 15:43:12 ssa Exp $ */
#include <stdio.h>
#include "person.h"

#include "ftchar.h"
#include "modes.h"
int	M_auto_wrap_on = 1;
int	M_columns_wide_on = 0;
int	M_csr_top_row = 0;
int	M_csr_bot_row = 100;
int	M_memory_lock_row = -1;

#include "myattrs.h"

#include "facetpath.h"

#include "features.h"
int	F_xon_xoff = 0;
int	F_force_CS8 = 0;
int	F_no_clear_ISTRIP = 0;
int	F_eat_newline_glitch[ MAX_PERSONALITY ] = { 0 };
int	F_real_eat_newline_glitch = 0;
int	F_auto_left_margin = 0;
int	sF_insdel_line_move_col0 = { 0 };
int	sF_scroll_reverse_move_col0 = { 0 };
int	sF_scroll_could_be_cursor_only = { 0 };
int	sF_use_csr = { 0 };
int	F_no_split_screen = 0;
FTCHAR	F_character_set_attributes = 0;
int	F_columns_wide_clears_screen = 0;
int	F_columns_wide_clears_onstatus = 0;
int	F_columns_wide_switch_resets_scroll_region = 0;
int	F_columns_wide_switch_reload_scroll_region = 0;
int	F_cursor_up_at_home_wraps_ll = 0;
int	sF_insert_line_needs_clear_glitch = { 0 };
int	sF_insert_line_sets_attributes = { 0 };
int	F_columns_wide_mode_on_default = 0;
int	sF_has_scroll_memory = { 0 };
int	F_tilde_glitch = 0;
int	F_allow_tabs = 0;
int	F_auto_wrap_on_default = 1;

#include "extra.h"
char	*T_carriage_return = "\r";
char	*T_cursor_address[ MAX_PERSONALITY ] = { NULL };
char	*T_cursor_address_wide[ MAX_PERSONALITY ] = { NULL };
char	*sT_row_address = { NULL };
char	*sT_column_address = { NULL };
char	*sT_column_address_parm_up_cursor = { NULL };
char	*sT_column_address_parm_down_cursor = { NULL };
char	*sT_new_line = { NULL };
char	*sT_cursor_home = { NULL };
char	*sT_cursor_up = { NULL };
char	*sT_parm_up_cursor = { NULL };
char	*sT_cursor_down = { NULL };
char	*sT_parm_down_cursor = { NULL };
char	*sT_cursor_right = { NULL };
char	*sT_parm_right_cursor = { NULL };
char	*sT_cursor_left = { NULL };
char	*sT_parm_left_cursor = { NULL };
char	*sT_cursor_to_ll= { NULL };
char	*sT_insert_character = { NULL };
char	*sT_parm_ich = { NULL };
char	*sT_enter_insert_mode = { NULL };
char	*sT_exit_insert_mode = { NULL };
char	*sT_delete_character = { NULL };
char	*sT_parm_delete_character = { NULL };
char	*sT_insert_line = { NULL };
char	*sT_out_insert_line = { NULL };
char	*sT_parm_insert_line = { NULL };
char	*sT_out_parm_insert_line = { NULL };
char	*sT_delete_line = { NULL };
char	*sT_out_delete_line = { NULL };
char	*sT_parm_delete_line = { NULL };
char	*sT_out_parm_delete_line = { NULL };
char	*sT_cursor_home_down = { NULL };
char	*sT_scroll_forward = { "\n" };
char	*sT_parm_index = { NULL };
char	*sT_scroll_reverse = { NULL };
char	*sT_parm_rindex = { NULL };
char	*T_flash_screen = { NULL };
char	*sT_change_scroll_region = { NULL };
char	*sT_memory_lock = { NULL };
char	*sT_memory_unlock = { NULL };
char	*sT_save_cursor = { NULL };
char	*sT_restore_cursor = { NULL };

T_CA_MODE	*T_enter_ca_mode_ptr = (T_CA_MODE *) 0;


T_CA_MODE	*T_enter_ca_mode_2_ptr = (T_CA_MODE *) 0;

T_CA_MODE	*T_exit_ca_mode_ptr = (T_CA_MODE *) 0;


T_CA_MODE	*T_exit_ca_mode_2_ptr = (T_CA_MODE *) 0;

char	*sT_insert_padding = { NULL };
char	*T_nomagic = NULL;
char	*T_magic[ MAX_MAGIC ]={NULL};
int	T_magicno=1;
char	*sT_auto_wrap_on = { NULL };
char	*sT_auto_wrap_off = { NULL };
char	*T_columns_wide_on = NULL;
char	*T_columns_wide_off = NULL;

#include "ftterm.h"
#include "cursor.h"

char	*Text_cannot_open_terminal_desc = 
	"Cannot open %s\n";

/**************************************************************************
* open_facetinfo
*	Look for and open a facetterm information file "infoname" in 
*	the directories:
*		current directory
*		$FACETINFO directory
*		$HOME directory
*		/usr/facetterm/localterm directory
*		/usr/facetterm/term directory
*		/usr/facetterm directory
*	if not found try an alias	
**************************************************************************/
#define MAX_ALIAS	40
FILE	*
open_facetinfo( infoname )
	char	*infoname;
{
	char	filename[ 256 ];
	FILE	*fd;
	char	*getenv();
	char	*directory;

	fd = fopen( infoname, "r" );
	if ( fd == NULL )
	{
		directory = getenv( "FACETINFO" );
		if ( directory == NULL )
			directory = getenv( "TSMINFO" );
		if ( directory != NULL )
		{
			strcpy( filename, directory );
			strcat( filename, "/" );
			strcat( filename, infoname );
			fd = fopen( filename, "r" );
		}
	}
	if ( fd == NULL )
	{
		directory = getenv( "HOME" );
		if ( directory != NULL )
		{
			strcpy( filename, directory );
			strcat( filename, "/" );
			strcat( filename, infoname );
			fd = fopen( filename, "r" );
		}
	}
	if ( fd == NULL )
	{
		sprintf( filename, "%s/localterm/%s", Facettermpath, infoname );
		fd = fopen( filename, "r" );
	}
	if ( fd == NULL )
	{
		sprintf( filename, "%s/term/%s", Facettermpath, infoname );
		fd = fopen( filename, "r" );
	}
	if ( fd == NULL )
	{
		strcpy( filename, Facettermpath );
		strcat( filename, "/" );
		strcat( filename, infoname );
		fd = fopen( filename, "r" );
	}
	if ( fd == NULL )
	{
		char	alias[ MAX_ALIAS + 1 ];
		int	status;

		status = get_facetinfo_alias( infoname, alias, MAX_ALIAS );
		if ( status == 0 )
		{
			fd = open_facetinfo( alias );
			if ( fd == NULL )
			{
					/* "Cannot open %s\n" */
				printf( Text_cannot_open_terminal_desc, alias );
			}
		}
	}
	return( fd );
}
/**************************************************************************
* get_facetinfo_alias
*	Try to find the filename "infoname" in a alias file.
*	Return 0 and the "alias" of maximum length "max_alias" if found.
*	Return -1 otherwise.
*	Look in files:
*		.facetaliasM
*		.facetalias
*		$FACETINFO/.facetaliasM
*		$FACETINFO/.facetalias
*		$HOME/.facetaliasM
*		$HOME/.facetalias
*		/usr/facetterm/localterm/.facetaliasM
*		/usr/facetterm/localterm/.facetalias
*		/usr/facetterm/term/.facetaliasM
*		/usr/facetterm/term/.facetalias
*		/usr/facetterm/.facetaliasM
*		/usr/facetterm/.facetalias
**************************************************************************/
get_facetinfo_alias( infoname, alias, max_alias )
	char	*infoname;
	char	*alias;
	int	max_alias;
{
	char	*getenv();
	char	*directory;
	char	alias_filename[ 256 ];
	char	alias_pathname[ 256 ];
	int	status;

	sprintf( alias_filename, ".%saliasM", Facetname );
	status = search_facetinfo_alias( alias_filename, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	sprintf( alias_filename, ".%salias", Facetname );
	status = search_facetinfo_alias( alias_filename, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	directory = getenv( "FACETINFO" );
	if ( directory == NULL )
		directory = getenv( "TSMINFO" );
	if ( directory != NULL )
	{
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		strcat( alias_pathname, "M" );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
	}
	directory = getenv( "HOME" );
	if ( directory != NULL )
	{
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		strcat( alias_pathname, "M" );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
	}
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/localterm/" );
	strcat( alias_pathname, alias_filename );
	strcat( alias_pathname, "M" );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/localterm/" );
	strcat( alias_pathname, alias_filename );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/term/" );
	strcat( alias_pathname, alias_filename );
	strcat( alias_pathname, "M" );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/term/" );
	strcat( alias_pathname, alias_filename );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/" );
	strcat( alias_pathname, alias_filename );
	strcat( alias_pathname, "M" );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/" );
	strcat( alias_pathname, alias_filename );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );

	return( -1 );
}
/**************************************************************************
* search_facetinfo_alias
*	Look for "infoname" in "alias_pathname",
*	If "alias_pathname does not exist or infoname not there, return -1.
*	If found, store alias in "alias" with maximum length "max_alias"
*	and return 0.
**************************************************************************/
search_facetinfo_alias( alias_pathname, infoname, alias, max_alias )
	char	*alias_pathname;
	char	*infoname;
	char	*alias;
	int	max_alias;
{
	FILE	*fd;
	char	buffer[ 1024 + 1 ];
	char	*strpbrk();
	char	*p;
	char	*x;

	fd = fopen( alias_pathname, "r" );
	if ( fd == NULL )
		return( -1 );
	while( read_dotfacet( buffer, 1024, fd ) >= 0 )
	{
		/**********************************************************
		* Truncate at first tab or space which must be present.
		**********************************************************/
		p = strpbrk( buffer, "\t " );
		if ( p == ( char * ) 0 )
			continue;
		*p++ = '\0';
		/**********************************************************
		* Skip any remaining tab or space.
		**********************************************************/
		p += strspn( p, "\t " );
		/**********************************************************
		* alias must have non-zero length.
		**********************************************************/
		if ( *p == '\0' )
			continue;
		/**********************************************************
		* Truncate at optional tab or space.
		**********************************************************/
		x = strpbrk( p, "\t " );
		if ( x != ( char * ) 0 )
			*x = '\0';
		if ( strcmp( buffer, infoname ) == 0 )
		{
			strncpy( alias, p, max_alias );
			alias[ max_alias ] = '\0';
			close( fd );
			return( 0 );			/* found */
		}
	}
	close( fd );
	return( -1 );					/* not found */
}
int	Facet_echo_fi = 0;
/**************************************************************************
* get_extra
*	Main module for reading terminal description files
**************************************************************************/
get_extra( terminal )
	char	*terminal;
{
	FILE	*fd;
	int	status;
	char	infoname[ 80 ];
	char	*getenv();

	strcpy( infoname, terminal );
	strcat( infoname, ".fi" );
	fd = open_facetinfo( infoname );
	if ( fd == NULL )
	{
			/* "Cannot open %s\n" */
		printf( Text_cannot_open_terminal_desc, infoname );
		return( 1 );
	}
	if ( getenv( "FACETECHOFI" ) != (char *) 0 )
		Facet_echo_fi = 1;
	for (	X_pe = 0; X_pe < MAX_PERSONALITY; X_pe++ )
	{
		inst_linefeed( "\n" );			/* ??? */
		inst_carriage_return( "\r" );
		inst_tab( "\t", 0 );			/* ??? */
		inst_backspace( "\b" );			/* ??? */
		inst_beep( "\007" );			/* ??? */
	}
	X_pe = 0;
	status = get_extra_strings( fd );
	close( fd );
	return( status );
}
#include "options.h"
#include "termdesc.h"
#include "baud.h"
#include "linklast.h"
#include "scroll.h"
char	*T_scroll_refresh_start = NULL;
char	*T_scroll_refresh_end = NULL;
int	F_scroll_refresh_no_scroll_last = 0;
char	*T_out_scroll_memory_clear = NULL;

#include "screensave.h"

/**************************************************************************
* get_extra_strings
*	Main module for parsing terminal description files.
**************************************************************************/
get_extra_strings( fd )
	FILE	*fd;
{
	char	buff[ TERM_DESC_BUFF_LEN + 1 ];
	char	*buffptr;
	char	*p;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
	char	*dec_encode();
	char	*dec_pull_pad();
	char	*encoded;
	int	len;
	int	no_error_msg;
	char	*pad;
	int	status;

	while( fgets( buff, TERM_DESC_BUFF_LEN, fd ) != NULL )
	{
				/*
				printf( "%s", buff );
				*/
		/**********************************************************
		* Comments begin with a #
		* echo those with # in column 1 but not column 2.
		**********************************************************/
		if ( Facet_echo_fi )
			printf( "%s", buff );
		if ( buff[ 0 ] == '#' )		/* echo comment */
		{
			if ( ( buff[ 1 ] != '#' ) && ( Opt_nonstop == 0 ) )
				printf( "%s", buff );
			continue;
		}
		/**********************************************************
		* Lines that start with * do not provoke complaint if not
		* recognized.
		**********************************************************/
		no_error_msg = 0;
		buffptr = buff;
		if ( *buffptr == '*' )
		{
			no_error_msg = 1;
			buffptr++;
		}
		/**********************************************************
		* Parse the line
		*	name-on-off=sequence
		* buffptr points at name
		* attr_on_string points at "on" or is NULL if no first -
		* attr_off_string points at "off" or is NULL if no second -
		* so that "string" points at "sequence" or is NULL if no =
		**********************************************************/
		len = strlen( buffptr );
		if ( len <= 1 ) 
			continue;
		p = &(buffptr[ len - 1 ]);
		if ( *p == '\n' )		/* kill trailing newline */
			*p-- = '\0';
		string = NULL;
		for ( p = buffptr; *p != '\0'; p++ )/* split @ first = if any */
		{
			if ( *p == '=' )	/* point string after = */
			{
				*p++ = '\0';
				string = p;
				break;
			}
		}
		attr_on_string = NULL;
		attr_off_string = NULL;
		for ( p = buffptr; *p != '\0'; p++ )/* split @ first - if any*/
		{
			if ( *p == '-' )	/* point after - */
			{
				*p++ = '\0';
				attr_on_string = p;
				for ( ; *p != '\0'; p++ )/* split next dash */
				{
					if ( *p == '-' )/* point after - */
					{
						*p++ = '\0';
						attr_off_string = p;
						break;
					}
				}
				break;
			}
		}
		if ( status = extra_sub_1( 
			   buffptr, string, attr_on_string, attr_off_string ) )
		{
			if ( status < 0 )
				return( status );
		}
		else if ( extra_sub_2( 
			   buffptr, string, attr_on_string, attr_off_string ) )
		{
		}
		else if ( extra_sub_3( 
			   buffptr, string, attr_on_string, attr_off_string ) )
		{
		}
		else if ( extra_other( 
			   buffptr, string, attr_on_string, attr_off_string ) )
		{
		}
		else if ( strcmp( buffptr, "ignore" ) == 0 )
		{
			encoded = dec_encode( string );
			inst_ignore( encoded, 0, 0 );
		}
		else if ( strcmp( buffptr, "parm_ignore" ) == 0 )
		{
			encoded = dec_encode( string );
			inst_ignore( encoded, 0, 1 );
		}
		else if ( strcmp( buffptr, "not_imp" ) == 0 )
		{
			encoded = dec_encode( string );
			inst_not_imp( encoded, 0, 1 );
		}
		else if ( strcmp( buffptr, "pass" ) == 0 )
		{
				encoded = dec_encode( string );
				pad = dec_pull_pad( string );
				inst_pass( encoded, pad );
		}
		else if ( strcmp( buffptr, "pass_current" ) == 0 )
		{
				encoded = dec_encode( string );
				pad = dec_pull_pad( string );
				inst_pass_current( encoded, pad );
		}
		else if ( strcmp( buffptr, "pass_same_personality" ) == 0 )
		{
				encoded = dec_encode( string );
				pad = dec_pull_pad( string );
				inst_pass_same_personality( encoded, pad );
		}
		else if ( no_error_msg == 0 )
		{
			printf( "Do not recognize '%s'\n", buffptr );
		}
	}
	fclose( fd );
	return( 0 );
}
/**************************************************************************
* extra_sub_1
*	Break up get extra for compilers.
**************************************************************************/
extra_sub_1( buffptr, string, attr_on_string, attr_off_string )
	char	*buffptr;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;
	char	*dec_encode();
	FILE	*new_fd;
	int	status;
	char	*getenv();

	if ( strcmp( buffptr, "use" ) == 0 )
	{
		new_fd = open_facetinfo( string );
		if ( new_fd == NULL )
		{
				/* "Cannot open %s\n" */
			printf( Text_cannot_open_terminal_desc, string );
			return( -1 );
		}
		status = get_extra_strings( new_fd );
		close( new_fd );
		if ( status )
			return( -1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "xon_xoff" ) == 0 )
	{
		F_xon_xoff =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "force_CS8" ) == 0 )
	{
		F_force_CS8 =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "no_clear_ISTRIP" ) == 0 )
	{
		F_no_clear_ISTRIP =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( extra_attribute( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "lines" ) == 0 )
	{
		Rows = atoi( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "use_LINES_if_set" ) == 0 )
	{
		char	*env_lines;

		env_lines = getenv( "LINES" );
		if ( env_lines != NULL )
			Rows = atoi( env_lines );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns" ) == 0 )
	{
		Cols = atoi( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "use_COLS_if_set" ) == 0 )
	{
		char	*env_cols;

		env_cols = getenv( "COLS" );
		if ( env_cols != NULL )
			Cols = atoi( env_cols );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "use_TIOCWINSZ_if_set" ) == 0 )
	{			/* kernel window structure showing rows/cols */
		int	rows;
		int	cols;

		if ( get_tiocwinsz( &rows, &cols ) == 0 )
		{
			if ( rows > 0 )
				Rows = rows;
			if ( cols > 0 )
				Cols = cols;
		}
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide" ) == 0 )
	{
		Cols_wide = atoi( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "lines_scroll_memory" ) == 0 )
	{
		do_lines_scroll_memory( attr_on_string, 
					attr_off_string, string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "scroll_refresh_start" ) == 0 )
	{
		T_scroll_refresh_start = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "scroll_refresh_end" ) == 0 )
	{
		T_scroll_refresh_end = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( 
	    strcmp( buffptr, "scroll_refresh_no_scroll_last" ) == 0 )
	{
		F_scroll_refresh_no_scroll_last =
				get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "out_scroll_memory_clear" ) == 0 )
	{
		T_out_scroll_memory_clear = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr,
			  "columns_wide_mode_on_default" ) == 0 )
	{
		F_columns_wide_mode_on_default =
						get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "carriage_return" ) == 0 )
	{
		T_carriage_return = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_address" ) == 0 )
	{
		encoded = dec_encode( string );
		T_cursor_address[ X_pe ] = encoded;
		inst_cursor_address( encoded );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_address_wide" ) == 0 )
	{
		encoded = dec_encode( string );
		T_cursor_address_wide[ X_pe ] = encoded;
		inst_cursor_address_wide( encoded );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_address_wide_starts" ) == 0 )
	{
		Cursor_address_wide_starts[ X_pe ] = atoi( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "row_address" ) == 0 )
	{
		xT_row_address = dec_encode( string );
		inst_row_address( xT_row_address );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "row_address_addsvp" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_row_address_addsvp( encoded );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "column_address" ) == 0 )
	{
		xT_column_address = dec_encode( string );
		inst_column_address( xT_column_address );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "column_address_addsvp" ) == 0 )
	{
		encoded = dec_encode( string );
		inst_column_address_addsvp( encoded );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "column_address_parm_up_cursor" ) 
								== 0 )
	{
		xT_column_address_parm_up_cursor = 
						dec_encode( string );
		inst_column_address_parm_up_cursor( 
				xT_column_address_parm_up_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "column_address_parm_down_cursor" ) 
								== 0 )
	{
		xT_column_address_parm_down_cursor = 
						dec_encode( string );
		inst_column_address_parm_down_cursor( 
				xT_column_address_parm_down_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "new_line" ) == 0 )
	{
		xT_new_line = dec_encode( string );
		inst_new_line( xT_new_line );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "home" ) == 0 )
	{
		xT_cursor_home = dec_encode( string );
		inst_cursor_home( xT_cursor_home );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_home" ) == 0 )
	{
		xT_cursor_home = dec_encode( string );
		inst_cursor_home( xT_cursor_home );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_up" ) == 0 )
	{
		xT_cursor_up = dec_encode( string );
		inst_cursor_up( xT_cursor_up );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_up_at_home_wraps_ll" ) == 0 )
	{
		F_cursor_up_at_home_wraps_ll =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_up_cursor" ) == 0 )
	{
		xT_parm_up_cursor = dec_encode( string );
		inst_parm_up_cursor( xT_parm_up_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_down" ) == 0 )
	{
		xT_cursor_down = dec_encode( string );
		inst_cursor_down( xT_cursor_down );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_down_cursor" ) == 0 )
	{
		xT_parm_down_cursor = dec_encode( string );
		inst_parm_down_cursor( xT_parm_down_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_right" ) == 0 )
	{
		xT_cursor_right = dec_encode( string );
		inst_cursor_right( xT_cursor_right );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_right_cursor" ) == 0 )
	{
		xT_parm_right_cursor = dec_encode( string );
		inst_parm_right_cursor( xT_parm_right_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_left" ) == 0 )
	{
		xT_cursor_left = dec_encode( string );
		inst_cursor_left( xT_cursor_left );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_left_cursor" ) == 0 )
	{
		xT_parm_left_cursor = dec_encode( string );
		inst_parm_left_cursor( xT_parm_left_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "cursor_to_ll" ) == 0 )
	{
		encoded = dec_encode( string );
		xT_cursor_to_ll = encoded;
		inst_cursor_to_ll( encoded );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "insert_character" ) == 0 )
	{
		xT_insert_character = dec_encode( string );
		inst_insert_character( xT_insert_character );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_ich" ) == 0 )
	{
		xT_parm_ich = dec_encode( string );
		inst_parm_ich( xT_parm_ich );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "enter_insert_mode" ) == 0 )
	{
		xT_enter_insert_mode = dec_encode( string );
		inst_enter_insert_mode( xT_enter_insert_mode );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "exit_insert_mode" ) == 0 )
	{
		xT_exit_insert_mode = dec_encode( string );
		inst_exit_insert_mode( xT_exit_insert_mode );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "delete_character" ) == 0 )
	{
		xT_delete_character = dec_encode( string );
		inst_delete_character( xT_delete_character );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_delete_character" ) == 0 )
	{
		xT_parm_delete_character = dec_encode( string );
		inst_parm_delete_character( xT_parm_delete_character );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "insert_line" ) == 0 )
	{
		xT_insert_line = dec_encode( string );
		inst_insert_line( xT_insert_line );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "out_insert_line" ) == 0 )
	{
		xT_out_insert_line = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_insert_line" ) == 0 )
	{
		xT_parm_insert_line = dec_encode( string );
		inst_parm_insert_line( xT_parm_insert_line );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "out_parm_insert_line" ) == 0 )
	{
		xT_out_parm_insert_line = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr,
			  "insert_line_needs_clear_glitch") == 0 )
	{
		xF_insert_line_needs_clear_glitch =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "insert_line_sets_attributes") == 0 )
	{
		xF_insert_line_sets_attributes =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "delete_line" ) == 0 )
	{
		xT_delete_line = dec_encode( string );
		inst_delete_line( xT_delete_line );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "out_delete_line" ) == 0 )
	{
		xT_out_delete_line = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_delete_line" ) == 0 )
	{
		xT_parm_delete_line = dec_encode( string );
		inst_parm_delete_line( xT_parm_delete_line );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "out_parm_delete_line" ) == 0 )
	{
		xT_out_parm_delete_line = dec_encode( string );
		return( 1 );				/* found */
	}
	return( 0 );			/* not found */
}
/**************************************************************************
* extra_sub_2
*	Break up get extra for compilers.
**************************************************************************/
extra_sub_2( buffptr, string, attr_on_string, attr_off_string )
	char	*buffptr;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;
	char	*dec_encode();
	long	*mymalloc();
	T_CA_MODE	*t_enter_ca_mode_ptr;
	T_CA_MODE	*t_enter_ca_mode_2_ptr;
	T_CA_MODE	*t_exit_ca_mode_ptr;
	T_CA_MODE	*t_exit_ca_mode_2_ptr;

	if ( strcmp( buffptr, "cursor_home_down" ) == 0 )
	{
		xT_cursor_home_down = dec_encode( string );
		inst_cursor_home_down( xT_cursor_home_down );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "scroll_forward" ) == 0 )
	{
		xT_scroll_forward = dec_encode( string );
		inst_scroll_forward( xT_scroll_forward );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_index" ) == 0 )
	{
		xT_parm_index = dec_encode( string );
		inst_parm_index( xT_parm_index );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "scroll_reverse" ) == 0 )
	{
		xT_scroll_reverse = dec_encode( string );
		inst_scroll_reverse( xT_scroll_reverse );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "parm_rindex" ) == 0 )
	{
		xT_parm_rindex = dec_encode( string );
		inst_parm_rindex( xT_parm_rindex );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "scroll_reverse_move_col0" ) == 0 )
	{
		xF_scroll_reverse_move_col0 =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "flash_screen" ) == 0 )
	{
		T_flash_screen = dec_encode( string );
		inst_flash_screen( T_flash_screen );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "change_scroll_region" ) == 0 )
	{
		xT_change_scroll_region = dec_encode( string );
		inst_change_scroll_region( xT_change_scroll_region );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "memory_lock" ) == 0 )
	{
		xT_memory_lock = dec_encode( string );
		inst_memory_lock( xT_memory_lock );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "memory_unlock" ) == 0 )
	{
		xT_memory_unlock = dec_encode( string );
		inst_memory_unlock( xT_memory_unlock );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "save_cursor" ) == 0 )
	{
		xT_save_cursor = dec_encode( string );
		inst_save_cursor( xT_save_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "restore_cursor" ) == 0 )
	{
		xT_restore_cursor = dec_encode( string );
		inst_restore_cursor( xT_restore_cursor );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "enter_ca_mode" ) == 0 )
	{
		t_enter_ca_mode_ptr = 
			(T_CA_MODE *) mymalloc( sizeof( T_CA_MODE ), 
				  "enter_ca_mode" );
		t_enter_ca_mode_ptr->next = (T_CA_MODE *) 0;
		link_last( (T_STRUCT *)  t_enter_ca_mode_ptr,
			   (T_STRUCT *) &T_enter_ca_mode_ptr );
		t_enter_ca_mode_ptr->t_ca_mode = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "enter_ca_mode_pc" ) == 0 )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "enter_ca_mode_pc_2" ) == 0 )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "enter_ca_mode_2" ) == 0 )
	{
		t_enter_ca_mode_2_ptr = 
			(T_CA_MODE *) mymalloc( sizeof( T_CA_MODE ), 
				  "enter_ca_mode_2" );
		t_enter_ca_mode_2_ptr->next = (T_CA_MODE *) 0;
		link_last( (T_STRUCT *)  t_enter_ca_mode_2_ptr, 
			   (T_STRUCT *) &T_enter_ca_mode_2_ptr );
		t_enter_ca_mode_2_ptr->t_ca_mode = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "exit_ca_mode" ) == 0 )
	{
		t_exit_ca_mode_ptr = 
			(T_CA_MODE *) mymalloc( sizeof( T_CA_MODE ), 
				  "exit_ca_mode" );
		t_exit_ca_mode_ptr->next = (T_CA_MODE *) 0;
		link_last( (T_STRUCT *)  t_exit_ca_mode_ptr,
			   (T_STRUCT *) &T_exit_ca_mode_ptr );
		t_exit_ca_mode_ptr->t_ca_mode = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "exit_ca_mode_pc" ) == 0 )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "exit_ca_mode_pc_2" ) == 0 )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "exit_ca_mode_2" ) == 0 )
	{
		t_exit_ca_mode_2_ptr = 
			(T_CA_MODE *) mymalloc( sizeof( T_CA_MODE ), 
				  "exit_ca_mode_2" );
		t_exit_ca_mode_2_ptr->next = (T_CA_MODE *) 0;
		link_last( (T_STRUCT *)  t_exit_ca_mode_2_ptr, 
			   (T_STRUCT *) &T_exit_ca_mode_2_ptr );
		t_exit_ca_mode_2_ptr->t_ca_mode = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "insert_padding" ) == 0 )
	{
		xT_insert_padding = dec_encode( string );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "nomagic" ) == 0 )
	{
		T_nomagic = dec_encode( string );
		inst_nomagic( T_nomagic );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "magic" ) == 0 )
	{
		if ( T_magicno < MAX_MAGIC )
		{
			T_magic[ T_magicno ] = dec_encode( string );
			inst_magic( T_magic[ T_magicno ], T_magicno );
			T_magicno++;
		}
		else
		{
			printf( "Too many magic\n" );
		}
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "out_magic" ) == 0 )
	{
					/* override the output for the
					   last magic string */
		if ( T_magicno > 1  )
		{
			T_magic[ T_magicno - 1 ] = dec_encode( string );
		}
		else
		{
			printf( "Out_magic precedes magic\n" );
		}
		return( 1 );				/* found */
	}
	return( 0 );			/* not found */
}
/**************************************************************************
* extra_sub_3
*	Break up get extra for compilers.
**************************************************************************/
extra_sub_3( buffptr, string, attr_on_string, attr_off_string )
	char	*buffptr;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*dec_encode();

	if ( strcmp( buffptr, "auto_wrap_on" ) == 0 )
	{
		xT_auto_wrap_on = dec_encode( string );
		inst_auto_wrap_on( xT_auto_wrap_on );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "auto_wrap_off" ) == 0 )
	{
		xT_auto_wrap_off = dec_encode( string );
		inst_auto_wrap_off( xT_auto_wrap_off );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "auto_wrap_off_default" ) == 0 )
	{
		F_auto_wrap_on_default = 0;
		M_auto_wrap_on = 0;
		return( 1 );				/* found */
	}
	if ( extra_auto_scroll( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_write_protect( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide_on" ) == 0 )
	{
		T_columns_wide_on = dec_encode( string );
		inst_columns_wide_on( T_columns_wide_on );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide_off" ) == 0 )
	{
		T_columns_wide_off = dec_encode( string );
		inst_columns_wide_off( T_columns_wide_off );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide_clears_screen" ) == 0 )
	{
		F_columns_wide_clears_screen =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide_clears_onstatus" ) 
		  == 0 )
	{
		F_columns_wide_clears_onstatus =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide_switch_resets_scroll_region" )
									== 0 )
	{
		F_columns_wide_switch_resets_scroll_region =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "columns_wide_switch_reload_scroll_region" )
									== 0 )
	{
		F_columns_wide_switch_reload_scroll_region =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( extra_clear( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_character( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_perwindow( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_mode( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_ibm_control( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "eat_newline_glitch" ) == 0 )
	{
		F_eat_newline_glitch[ X_pe ] =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "real_eat_newline_glitch" ) == 0 )
	{
		F_real_eat_newline_glitch = 
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "tilde_glitch" ) == 0 )
	{
		F_tilde_glitch = 
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "allow_tabs" ) == 0 )
	{
		F_allow_tabs = 
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "auto_left_margin" ) == 0 )
	{
		F_auto_left_margin =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "insdel_line_move_col0" ) == 0 )
	{
		xF_insdel_line_move_col0 =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr,
			  "scroll_could_be_cursor_only" ) == 0 )
	{
		xF_scroll_could_be_cursor_only =
			get_optional_value( string, 1 );
		return( 1 );				/* found */
	}
	if ( strcmp( buffptr, "screen_saver_timer" ) == 0 )
	{
		if ( Screen_saver_timer_in_dotfacet == 0 )
		{
			Screen_saver_timer = atoi( string );
			if ( Screen_saver_timer <= 0 )
				Screen_saver_timer = DEFAULT_SCREEN_SAVER_TIMER;
		}
		return( 1 );				/* found */
	}
	return( 0 );			/* not found */
}
/**************************************************************************
* extra_other
*	Break up get extra for compilers.
**************************************************************************/
extra_other( buffptr, string, attr_on_string, attr_off_string )
	char	*buffptr;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	if ( extra_keys( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_status_line( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_line_attribute( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_pages( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_pkey_key( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_tabs( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_pc_mode( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_terminal_mode( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_transparent_print( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_margins( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_paste_eol( buffptr, string,
				attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_graph_mode( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_decode_type( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_split( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_set( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_onstatus( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_rows_change( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_hp_attribute( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_personality( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_keyboard( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_meta_roll( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_answer( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_fct_info_answer( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_draw_box( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_substitute( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	if ( extra_graph_screen( 
		   buffptr, string, attr_on_string, attr_off_string ) )
	{
		return( 1 );				/* found */
	}
	return( 0 );            /* no match */
}
/**************************************************************************
* do_lines_scroll_memory
*	Validate and store:
*		lines_scroll_memory-default_rows-default_baud=max_rows
**************************************************************************/
do_lines_scroll_memory( attr_on_string, attr_off_string, string )
	char	*attr_on_string;
	char	*attr_off_string;
	char	*string;
{
	int	max_rows;
	int	default_rows;
	int	baud;
	int	trans_baud;

	max_rows = atoi( string );
	if (  ( attr_on_string != NULL ) && ( *attr_on_string != '\0' ) )
	{
		default_rows = atoi( attr_on_string );
		if (    ( default_rows < 0 ) 
		   || ( ( default_rows == 0 ) && ( *string != '0' ) ) )
		{
			printf(
"lines_scroll_memory - invalid default = '%s'\n", attr_on_string );
			default_rows = 0;
		}
	}
	else
	{
		printf( "lines_scroll_memory - missing default\n" );
		default_rows = 0;
	}
	if (  ( attr_off_string != NULL ) && ( *attr_off_string != '\0' ) )
	{
		baud = atoi( attr_off_string );
		trans_baud = translate_baud( baud );
		if ( trans_baud == INVALID_BAUD_RATE )
		{
					printf(
"lines_scroll_memory - Unknown baud rate %d\n", baud );
		}
	}
	else
	{
		trans_baud = INVALID_BAUD_RATE;
	}
	if ( Opt_min_scroll_memory_baud != INVALID_BAUD_RATE )
		trans_baud = Opt_min_scroll_memory_baud;
	if (  ( trans_baud == INVALID_BAUD_RATE ) || ( Ft_baud <= trans_baud ) )
	{
		Rows_scroll_memory = default_rows;
		if ( Opt_rows_scroll_memory >= 0 )
			Rows_scroll_memory = Opt_rows_scroll_memory;
		if ( Rows_scroll_memory > max_rows )
			Rows_scroll_memory = max_rows;
	}
	xF_has_scroll_memory = 1;
}
/**************************************************************************
* get_optional_value
*	If "string" is a decimal integer, return it.
*	Otherwise return "default_value".
**************************************************************************/
get_optional_value( string, default_value )
	char	*string;
	int	default_value;
{
	if ( string == NULL )
		return( default_value );
	return( atoi( string ) );
}

#define ENCODE_LEN_MAX 2000
char Dummy_string[] = "";
/**************************************************************************
* dec_encode
*	Encode a string from a terminal description file.  Malloc room
*	to store it, store, and return a pointer.
**************************************************************************/
char *
dec_encode( string )
	char	*string;
{
	char	*string_encode();
	char	extra_buff[ ENCODE_LEN_MAX + 1 ];
	char	*p;
	char	*new;
	int	len;
	long	*malloc_run();

	if ( strlen( string ) > ENCODE_LEN_MAX )
	{
		printf( "ERROR: encode string too long '%s'\n", string );
		return( Dummy_string );
	}
	p = extra_buff;
	string_encode( string, &p );
	len = p - extra_buff + 1;
	new = (char *) malloc_run( len, "dec_encode" );
	if ( new == NULL )
	{
		printf( "ERROR: malloc in dec_encode failed.\n" );
		return( Dummy_string );
	}
	memcpy( new, extra_buff, len );
	return( new );
}
/**************************************************************************
* dec_pull_pad
*	If the string "s" has a terminfo padding construct in it, then
*	extract it, copy it to malloc'ed storage, and return a pointer to
*	it.
*	Return NULL otherwise.
**************************************************************************/
char *
dec_pull_pad( s )
	char	*s;
{
	char	c;
	char	*p;
	char	extra_buff[ ENCODE_LEN_MAX + 1 ];
	int	len;
	char	*new;
	long	*malloc_run();

	if ( strlen( s ) > ENCODE_LEN_MAX )
	{
		printf( "ERROR: encode string too long.'%s'\n", s );
		return( NULL );
	}
	p = extra_buff;
	while ( ( c = *s++ ) != '\0' )
	{
		if ( c == '$' )
		{
			*p++ = c;
			while ( ( c = *s++ ) != '\0' )
			{
				*p++ = c;
				if ( c == '>' )
					break;
			}
			*p++ = '\0';
			len = p - extra_buff + 1;
			new = (char *) malloc_run( len, "dec_pull_pad" );
			if ( new == NULL )
			{
				printf( 
				    "ERROR: malloc in dec_pull_pad failed.\n" );
				return( NULL );
			}
			memcpy( new, extra_buff, len );
			return( new );
		}
	}
	return( NULL );
}
/**************************************************************************
* temp_encode
*	Encode the string "string" into the buffer "storage" which is
*	guaranteed to be big enough to hold it.
**************************************************************************/
char *
temp_encode( string, storage )
	char	*string;
	char	*storage;
{
	char	*string_encode();
	char	*ptr;

	ptr = storage;
	string_encode( string, &ptr );
	return( storage );
}
/**************************************************************************
* string_encode
*	Encode the string "string" into the buffer whose pointer is pointed
*	to by "store".
*	Recognizes the ^X convention for Control-X
*	Recognizes common \ sequences such as \n for newline
*	Recognizes \h for facetterm hotkey and \m for menu hotkey.
*  NO_PROMPT ???
**************************************************************************/
char *
string_encode( string, store )
	char	*string;
	char	**store;
{
	char	c;
	char	*t;
	char	*s;
	char	out[ 10 ];
	int	value;
	int	digit;

	if ( string == NULL )
		s = "";
	else
		s = string;
	t = *store;
	while ( ( c = *s++ ) != '\0' )
	{
		if ( c == '\\' )
		{
			c = *s++;
			if ( ( c == 'E' ) || ( c == 'e' ) )
				*(*store)++ = 0x1b;
			else if ( c == '\\' )
				*(*store)++ = '\\';
			else if ( c == '^' )
				*(*store)++ = '^';
			else if ( c == 'r' )
				*(*store)++ = '\r';
			else if ( c == 'n' )
				*(*store)++ = '\n';
			else if ( c == 'b' )
				*(*store)++ = '\b';
			else if ( c == 's' )
				*(*store)++ = ' ';
			else if ( c == 'h' )
			{
				if ( Opt_hotkey > 0 )
					*(*store)++ = Opt_hotkey;
			}
			else if ( c == 'm' )
			{
				if ( Opt_menu_hotkey > 0 )
					*(*store)++ = Opt_menu_hotkey;
			}
			else if ( c == '[' )
				*(*store)++ = 0x40 + '[';	/* 8_bit */
			else if ( c == 'O' )
				*(*store)++ = 0x40 + 'O';	/* 8_bit */
			else if ( c >= '0' && c <= '9' )
			{
				value = c - '0';
				digit = 1;
				while ( *s >= '0' && *s <= '9' && digit < 3 )
				{
					c = *s++;
					value <<= 3;
					value += ( c - '0' );
					digit++;
				}
				if ( value == 0 )
					*(*store)++ = 0x80;
				else
					*(*store)++ = value;
			}
			else
			{
				out[ 0 ] = c;
				out[ 1 ] = '\0';
				printf( "Did not encode '\\%s'\n", out );
				printf( "%s\n", string );
			}
		}
		else if ( c == '^' )
		{
			c = *s++;
			if ( c == '?' )
				*(*store)++ = '\177';
			else if ( c == '@' )
				*(*store)++ = 0x80;
			else if ( c > 0x60 )
				*(*store)++ = c - 0x60;
			else if ( c > 0x40 )
				*(*store)++ = c - 0x40;
			else
			{
				out[ 0 ] = c;
				out[ 1 ] = '\0';
				printf( "Did not encode '^%s'\n", out );
				printf( "%s\n", string );
			}
		}
		else
		{
			*(*store)++ = c;
		}
	}
	*(*store)++ = '\0';
	return( t );
}
