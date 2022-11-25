/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: func.c,v 70.1 92/03/09 15:44:27 ssa Exp $ */
/**************************************************************************
* func.c
*	Function key programming.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "wins.h"
#include "options.h"

#include "func.h"
int	F_assume_default_function_keys = 1;

char	*T_pkey_key = NULL;
char	*T_out_pkey_key = NULL;
char	*T_pkey_key_clear = NULL;
char	*T_out_pkey_key_clear = NULL;
char	*T_function_key_clear = NULL;
char	*T_function_key_default = NULL;
int	T_function_key_clear_mode_yes = -1;
int	T_function_key_clear_mode_no = -1;
int	T_function_key_clear_mode_default = -1;
int	T_function_key_includes_label = 0;
int	T_function_key_label_max_len = 0;
int	T_function_key_len_0_label_only = 0;
int	T_function_key_len_0_func_only = 0;

int	T_function_key_string_is_hex = 0;
int	T_function_key_string_is_ansi = 0;
int	T_function_key_string_is_escaped = 0;
UNCHAR	T_func_key_escaped_char = 'X';
UNCHAR	*T_func_key_escaped_list = NULL;

int	T_function_key_no = 0;
int	T_shift_function_key_no = 0;

/* typedef struct t_function_key_struct T_FUNCTION_KEY; */
struct t_function_key_struct
{
	T_FUNCTION_KEY		*next;
	short			function_key_id;
							     /*	term	win */
	UNCHAR			function_key_delimiter;	     /*	def	cur */
	UNCHAR			*function_key_value;	     /* def	cur */
	UNCHAR			*function_key_label;	     /* def	cur */
	UNCHAR			function_key_type;	     /* def	cur */
	UNCHAR			*last_function_key_value;    /* cur	^Wk */
	UNCHAR			*last_function_key_label;    /* cur	^Wk */
	UNCHAR			last_function_key_type;     /* cur	^Wk */
};
T_FUNCTION_KEY	*T_function_key_ptr = (T_FUNCTION_KEY *) 0;

char	*T_function_key_file = (char *) 0;

UNCHAR	T_function_key_type_default = '0';
UNCHAR	T_function_key_type_set = '0';

#include "linklast.h"
/**************************************************************************
* init_function_key
*	Allocate and set default function keys values for window "win".
**************************************************************************/
init_function_key( win )
	FT_WIN	*win;
{
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;
	long		*mymalloc();

	win->function_key_ptr = (T_FUNCTION_KEY *) 0;
	for ( t = T_function_key_ptr; t != (T_FUNCTION_KEY *) 0; t = t->next )
	{
		w = (T_FUNCTION_KEY *) mymalloc( 
						sizeof( T_FUNCTION_KEY ),
						"window_function_key" );
		w->next = (T_FUNCTION_KEY *) 0;
		link_last( (T_STRUCT *) w,
			   (T_STRUCT *) &(win->function_key_ptr) );
		w->function_key_id =	t->function_key_id;
		w->function_key_delimiter =
					t->function_key_delimiter;
		w->function_key_value =
		w->last_function_key_value =
					t->function_key_value;
		w->function_key_label =
		w->last_function_key_label =
					t->function_key_label;
		w->function_key_type =
		w->last_function_key_type =
					t->function_key_type;
	}
}
/**************************************************************************
* init_default_function_key
*	Initialize the function keys if necessary to their assumed defaults.
*	Called during sender initialization.
**************************************************************************/
init_default_function_key()
{
	if ( F_assume_default_function_keys )
	{
		term_se_defaults_function_keys();
	}
	else
	{
		if ( T_function_key_default != NULL )
			term_function_key_default();
	}
	t_sync_function_key();
}
/**************************************************************************
* term_se_defaults_function_keys
*	An event has occurred that had the side effect of setting the
*	function keys of the terminal to their default values.
**************************************************************************/
term_se_defaults_function_keys()
{
	T_FUNCTION_KEY	*t;

	for ( t = T_function_key_ptr; t != (T_FUNCTION_KEY *) 0; t = t->next )
	{
		t->last_function_key_value = t->function_key_value;
		t->last_function_key_label = t->function_key_label;
		t->last_function_key_type  = t->function_key_type;
	}
}
#define LINEMAX 160

#include "ftkey.h"
#define	NOT_PROCESSING_FTKEY 0
#define PROCESSING_FTKEY 1

#include "perwindow.h"
#include "facetpath.h"
#include "terminal.h"

#define TITLED_MAX 400
/**************************************************************************
* load_all_function_keys
*	Load function key file "filename_in" into the window number
*	"load_winno".
*	If "load_winno" is < 0 the load onto all windows.
**************************************************************************/
load_all_function_keys( filename_in, load_winno )	/* 0=OK 1=DIDNT */
	char	*filename_in;
	int	load_winno;
{
	int	winno;
	int	i;
	UNCHAR	*remember_function_key_hex();
	UNCHAR	*remember_function_key_ansi();
	UNCHAR	*remember_function_key_escaped();
	UNCHAR	*function_key_encode();
	UNCHAR	*p;
	char	buff[ LINEMAX + 1 ];
	char	*ptr;
	FILE	*open_facetinfo();
	FILE	*file;
	int	len;
	int	start_winno;
	int	end_winno;
	int	base;
	int	is_label;
	char	name[ 80 ];
	int	shifted;
	char	*filename;
	char	default_filename[ 100 ];
	char	titled[ TITLED_MAX + 10 ];
	FT_WIN	*win;

	filename = filename_in;
	if ( (load_winno >= 0) && (load_winno < Wins) )
	{
		start_winno = load_winno;
		end_winno = load_winno;
	}
	else if ( load_winno < 0 )
	{
		start_winno = 0;
		end_winno = Wins - 1;
	}
	else
	{
		term_beep();
		return( 1 );
	}
	if ( strcmp( filename, "DEFAULT" ) == 0 )
	{
		set_function_key_value_default( start_winno, end_winno );
		if ( T_function_key_includes_label == 0)
			status_line_label_default();
		for ( winno = start_winno; winno <= end_winno; winno++ )
		{
			perwindow_init( SYNC_MODE_FUNCTION, winno, 
					PROCESSING_FTKEY );
		}
		for ( winno = start_winno; winno <= end_winno; winno++ )
		{
			win = Wininfo[ winno ];
			init_status_line( win );
		}
	}
	else if ( strcmp( filename, "+" ) == 0 )
	{
		switch_function_key_value_last( start_winno, end_winno );
		for ( winno = start_winno; winno <= end_winno; winno++ )
		{
			perwindow_ftkey_last( SYNC_MODE_FUNCTION, winno );
		}
	}
	else if ( strcmp( filename, "-" ) == 0 )
	{
		switch_function_key_value_default( start_winno, end_winno );
		for ( winno = start_winno; winno <= end_winno; winno++ )
		{
			perwindow_init( SYNC_MODE_FUNCTION, winno, 
					NOT_PROCESSING_FTKEY );
		}
	}
	else
	{
		if (  ( filename[ 0 ] == '\0' ) 
		   || ( strcmp(filename, "WINDOWS") == 0 ) )
		{
			filename = default_filename;
							/* .facetkeys */
			sprintf( filename, ".%skeys", Facetname );
			file = open_facetinfo( filename );
			if ( file == NULL )
			{				/* vt220.fk */
				sprintf( filename, "%s.fk", Facetterm );
				file = open_facetinfo( filename );
			}
			if (  ( file == NULL )
			   && ( T_function_key_file != (char *) 0 ) )
			{
				filename = T_function_key_file;
				file = open_facetinfo( filename );
			}
			if ( file == NULL )
			{				/* WINDOWS.ftkey */
				filename = "WINDOWS.ftkey";
				file = open_facetinfo( filename );
			}
		}
		else
			file = open_facetinfo( filename );
		if ( file != NULL )
		{
			Processing_ftkey = 1;
			while ( fgets( buff, LINEMAX, file ) != NULL )
			{
				len = strlen( buff );
				if ( buff[ len-1 ] == '\n' )
				{
					buff[ len-1 ] = '\0';
					len--;
				}
				if ( strncmp( buff,
					    "max_label_from_title=", 21 ) == 0 )
				{
					int	x;

					if ( ( x = atoi( &buff[ 21 ] ) ) > 0 )
						set_max_label_from_title( x );
					continue;
				}
				if ( strncmp( buff,
					    "min_label_from_title=", 21 ) == 0 )
				{
					int	x;

					if ( ( x = atoi( &buff[ 21 ] ) ) > 0 )
						set_min_label_from_title( x );
					continue;
				}
							/* see if this matched
							   a "set" name.
							*/
				if ( check_store_set( start_winno, end_winno, 
						      buff ) == 0 )
					continue;
				base = 0;
				shifted = 0;
				ptr = buff;
				if ( *ptr == 'W' )
				{
					ptr++;
					if ( (*ptr < '0') && ( *ptr > '9') )
						continue;
					winno = atoi( ptr ) - 1;
					if ( winno == -1 )
						winno = 9;
					if ( (winno >= 0) && (winno < Wins) )
					{
						start_winno = winno;
						end_winno = winno;
					}
					continue;
				}
				if ( *ptr == 'S' )
				{
					base = T_shift_function_key_no;
					shifted = 1;
					ptr++;
					len--;
				}
				if ( *ptr == 'L' )
				{
					is_label = 1;
					ptr++;
					len--;
				}
				else if ( *ptr == '*' )
				{			/* status line */
					ptr++;
					len--;
					if ( len < 1 )
						continue;
					encode_label_titles( ptr, 
							titled, TITLED_MAX );
					if ( shifted == 0 )
					{
						if ( store_set( 
							start_winno, 
							end_winno, 
							"status_line", 
							titled ) == 0 )
						    continue;
					}
					else
					{
						if ( store_set( 
							start_winno, 
							end_winno, 
							"shift_status_line", 
							titled ) == 0 )
						    continue;
					}
					if ( status_line_load(
						start_winno, end_winno, titled )
						== 0 )
					{
						continue;
					}
					if ( onstatus_load(
						start_winno, end_winno, titled )
						== 0 )
					{
						continue;
					}
					continue;
				}
				else
					is_label = 0;
				if ( len < 3 )
					continue;
				if ( (*ptr < '0') && ( *ptr > '9') )
					continue;
				i = atoi( ptr ) - 1;
				if ( i >= 0 )
					i += base;
				if ( ( i >= 0 ) && ( i < T_function_key_no ) )
				{
					for ( ptr++; *ptr != '\0'; ptr++ )
					{
						if (  ( *ptr == ' ' )
						   || ( *ptr == '\t' ) )
						{
							ptr++;
							break;
						}
					}
					if (  ( is_label )
					   && ( T_function_key_includes_label
						== 0 ) )
					{
						encode_label_titles( ptr, 
							titled, TITLED_MAX );
						if ( shifted == 0 )
							sprintf( name,
								 "label_%d",
								 i + 1 );
						else
							sprintf( name,
							"shift_label_%d",
								 i + 1 - base );
						if ( store_set( start_winno, 
								end_winno, 
								name,
								titled ) == 0 )
							continue;
						status_line_label_load(
							start_winno,
							end_winno,
							i, titled );
						continue;
					}
					if ( is_label )
					{
						encode_label_titles( ptr, 
							titled, TITLED_MAX );
						if (T_function_key_label_max_len
						    > 0 )
						{
						  titled[
						   T_function_key_label_max_len]
						   = '\0';
						}
						p = function_key_encode(
								titled );
						set_function_key_label(
							start_winno,
							end_winno,
							i,
							p );
						continue;
					}
					p = function_key_encode( ptr );
					if ( T_function_key_string_is_hex )
					{
					    p = remember_function_key_hex( p );
					}
					else if ( T_function_key_string_is_ansi)
					{
					    p = remember_function_key_ansi( p );
					}
					else if (
					      T_function_key_string_is_escaped)
					{
					    p = 
					    remember_function_key_escaped( p );
					}
					set_function_key_value(
						start_winno,
						end_winno,
						i,		/* key # */
						p,		/* value */
						T_function_key_type_set );
				}
			}
			fclose( file );
			Processing_ftkey = 0;
		}
		else
		{
			term_beep();
			return( 1 );
		}
	}
	t_sync_function_key();
	t_sync_status_line( Outwin );
	onstatus_refresh();
	t_sync_perwindow_all( SYNC_MODE_FUNCTION );
	t_cursor();
	return( 0 );
}
/**************************************************************************
* set_function_key_value_default
*	Set functio key values to their default on windows "start_winno"
*	to "end_winno".
**************************************************************************/
set_function_key_value_default( start_winno, end_winno )
	int	start_winno;
	int	end_winno;
{
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;
	int	winno;
	FT_WIN	*win;

	for ( winno = start_winno; winno <= end_winno; winno++ )
	{

		win = Wininfo[ winno ];
		for (   t = T_function_key_ptr,	w = win->function_key_ptr;
			   ( t != (T_FUNCTION_KEY *) 0 ) 
			&& ( w != (T_FUNCTION_KEY *) 0 );
			t = t->next,		w = w->next )
		{
			w->function_key_delimiter =
					t->function_key_delimiter;
			w->function_key_value =
			w->last_function_key_value =
					t->function_key_value;
			w->function_key_label =
			w->last_function_key_label =
					t->function_key_label;
			w->function_key_type =
			w->last_function_key_type =
					t->function_key_type;
		}
	}
}
/**************************************************************************
* switch_function_key_value_default
*	^W k -  sets the function key values to default without disturbing
*	the values remembered as the last loaded from function key files
*	on windows number "start_winno" to "end_winno".
**************************************************************************/
switch_function_key_value_default( start_winno, end_winno )
	int	start_winno;
	int	end_winno;
{
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;
	int	winno;
	FT_WIN	*win;

	for ( winno = start_winno; winno <= end_winno; winno++ )
	{
		win = Wininfo[ winno ];
		for (   t = T_function_key_ptr,	w = win->function_key_ptr;
			   ( t != (T_FUNCTION_KEY *) 0 ) 
			&& ( w != (T_FUNCTION_KEY *) 0 );
			t = t->next,		w = w->next )
		{
			w->function_key_delimiter =
					t->function_key_delimiter;
			w->function_key_value =
					t->function_key_value;
			w->function_key_label =
					t->function_key_label;
			w->function_key_type =
					t->function_key_type;
		}
	}
}
/**************************************************************************
* switch_function_key_value_last
*	^W k + sets the function key values to the values
*	remembered as the last loaded from function key files
*	for windows numbered "start_winno" to "end_winno".
**************************************************************************/
switch_function_key_value_last( start_winno, end_winno )
	int	start_winno;
	int	end_winno;
{
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;
	int	winno;
	FT_WIN	*win;

	for ( winno = start_winno; winno <= end_winno; winno++ )
	{

		win = Wininfo[ winno ];
		for (   t = T_function_key_ptr,	w = win->function_key_ptr;
			   ( t != (T_FUNCTION_KEY *) 0 ) 
			&& ( w != (T_FUNCTION_KEY *) 0 );
			t = t->next,		w = w->next )
		{
			w->function_key_delimiter =
					t->function_key_delimiter;
			w->function_key_value =
					w->last_function_key_value;
			w->function_key_label =
					w->last_function_key_label;
			w->function_key_type =
					w->last_function_key_type;
		}
	}
}
/**************************************************************************
* set_function_key_label
*	Set the function key label for key number "keyno" to the
*	label value "value" for windows numbered "start_winno" to
*	"end_winno".
**************************************************************************/
set_function_key_label( start_winno, end_winno, keyno, value )
	int	start_winno;
	int	end_winno;
	int	keyno;
	UNCHAR	*value;
{
	int		winno;
	FT_WIN		*win;
	T_FUNCTION_KEY	*w;
	T_FUNCTION_KEY	*find_function_key_keyno();

	for ( winno = start_winno; winno <= end_winno; winno++ )
	{
		win = Wininfo[ winno ];
		w = find_function_key_keyno( win->function_key_ptr, keyno );
		if ( w != (T_FUNCTION_KEY *) 0 )
		{
			w->function_key_label =
			w->last_function_key_label = value;
		}
	}
}
/**************************************************************************
* set_function_key_value
*	Set the function key value for key number "keyno" to
*	"value" for windows numbered "start_winno" to "end_winno".
**************************************************************************/
set_function_key_value( start_winno, end_winno, keyno, value,
							function_key_type )
	int	start_winno;
	int	end_winno;
	int	keyno;
	UNCHAR	*value;
	UNCHAR	function_key_type;
{
	int		winno;
	FT_WIN		*win;
	T_FUNCTION_KEY	*w;
	T_FUNCTION_KEY	*find_function_key_keyno();

	for ( winno = start_winno; winno <= end_winno; winno++ )
	{
		win = Wininfo[ winno ];
		w = find_function_key_keyno( win->function_key_ptr, keyno );
		if ( w != (T_FUNCTION_KEY *) 0 )
		{
			w->function_key_value =
			w->last_function_key_value = value;
			w->function_key_type =
			w->last_function_key_type = function_key_type;
		}
	}
}
/**************************************************************************
* find_function_key_keyno
*	Return the pointer to the "keyno"th function key structure
*	in the list pointed to by "base".
**************************************************************************/
T_FUNCTION_KEY	*
find_function_key_keyno( base, keyno )
	T_FUNCTION_KEY	*base;
	int		keyno;
{
	int		i;
	T_FUNCTION_KEY	*t;;

	t = base;
	for ( i = 0; i < keyno; i++ )
	{
		if ( t == (T_FUNCTION_KEY *) 0 )
			break;
		t = t->next;
	}
	return( t );
}
/**************************************************************************
* find_function_key_id
*	Return the pointer to the function key structure with id 
*	"function_key_id" in the list pointed to by "base".
**************************************************************************/
T_FUNCTION_KEY *
find_function_key_id( base, function_key_id )
	T_FUNCTION_KEY	*base;
	int		function_key_id;
{
	T_FUNCTION_KEY	*t;

	for ( t = base; t != (T_FUNCTION_KEY *) 0; t = t->next )
	{
		if ( t->function_key_id == function_key_id )
			break;
	}
	return( t );
}
#define CHECK_STORE_SET_MAX 80
/**************************************************************************
* check_store_set
*	See if "buff" contains a valid "store set".
*	If so then load it into windows "first_winno" to "last_winno",
*		and return 0;
*	Otherwise return -1;
*	A "store set" is a line where the first word matches a keyword
*	on a "set" line in the terminal description file.
*	The rest of the line is used as a parameter to the sequence
*	specified in the "set" capability.
**************************************************************************/
check_store_set( first_winno, last_winno, buff )
	int	first_winno;
	int	last_winno;
	char	*buff;
{
	char	name[ CHECK_STORE_SET_MAX + 1 ];
	int	len;
	int	len2;
	int	status;

	len = strcspn( buff, "\t " );
	if ( len > CHECK_STORE_SET_MAX )
		return( -1 );
	strncpy( name, buff, len );
	name[ len ] = '\0';
	len2 = strspn( &buff[ len ], "\t " );

	status =
	store_set( first_winno, last_winno, name, &buff[ len + len2 ] );
	return( status );
}
/*
	parm1		id
	string1		value
	parm2		id
	string2		value
	parm11		mode;
	parm12		 delimiter
*/

#define POS_FUNCTION_KEY_TYPE	10

#define POS_FUNCTION_KEY_MODE	10
#define POS_FUNCTION_KEY_LABEL	11
/**************************************************************************
* dec_pkey_key
*	ACTION module for 'pkey_key'.
*	The pkey_key sequence number "pkey_key_type" has been decoded and
*	contained the integer parms "parms"
*	with the valid entries indicated by bits in "parms_valid"
*	and the string parms "string_parm"
*	with the valid entries indicated by "string_parms_valid".
**************************************************************************/
/*ARGSUSED*/
dec_pkey_key( pkey_key_type, parm_ptr, parms_valid, parm, 
					string_parm, string_parms_valid )
	int	pkey_key_type;
	char	*parm_ptr;		/* not used */
	int	parms_valid;
	int	parm[];
	UNCHAR	*string_parm[];
	int	string_parms_valid;
{
	if ( T_function_key_string_is_ansi )
	{
		fct_pkey_key_ansi( pkey_key_type, 
		      parm, parms_valid & 0x3FF,
		      string_parm, string_parms_valid & 0x3FF,
		      parm[ POS_FUNCTION_KEY_MODE ],
		      parms_valid & (1 << POS_FUNCTION_KEY_MODE),
		      (UNCHAR) parm[ POS_FUNCTION_KEY_DELIMITER ],
		      parms_valid & (1 << POS_FUNCTION_KEY_DELIMITER),
		      string_parms_valid & ( 1 << POS_FUNCTION_KEY_LABEL ),
		      string_parms_valid & ( 1 << POS_FUNCTION_KEY_TYPE ) );
	}
	else
	{
		fct_pkey_key( pkey_key_type, 
		      parm, parms_valid & 0x3FF,
		      string_parm, string_parms_valid & 0x3FF,
		      parm[ POS_FUNCTION_KEY_MODE ],
		      parms_valid & (1 << POS_FUNCTION_KEY_MODE),
		      (UNCHAR) parm[ POS_FUNCTION_KEY_DELIMITER ],
		      parms_valid & (1 << POS_FUNCTION_KEY_DELIMITER),
		      string_parms_valid & ( 1 << POS_FUNCTION_KEY_LABEL ),
		      string_parms_valid & ( 1 << POS_FUNCTION_KEY_TYPE ) );
	}
}
#define PKEY_KEY_CLEAR 0x01
/**************************************************************************
* inst_pkey_key
*	INSTALL module for 'pkey_key'.
**************************************************************************/
inst_pkey_key( str, pkey_key_type )
	char	*str;
	int	pkey_key_type;
{
	dec_install( "pkey_key", (UNCHAR *) str,
				dec_pkey_key, pkey_key_type, CURSOR_OPTION,
				(char *) 0 );
}
/**************************************************************************
* inst_parm_pkey_key
*	INSTALL module for 'parm_pkey_key'.
**************************************************************************/
inst_parm_pkey_key( str, pkey_key_type )
	char	*str;
	int	pkey_key_type;
{
	dec_install( "parm_pkey_key", (UNCHAR *) str,
				dec_pkey_key, pkey_key_type, CURSOR_OPTION,
				(char *) 0 );
}
/**************************************************************************
* fct_pkey_key
*	ACTION module for 'pkey_key'.
*	The decoder has recognized the pkey_key sequence number
*	"pkey_key_type".
*	It decoded the function key ids "function_key_id" whose
*	valid entries are indicated by bits in "function_key_id valid";
*	the function key values "function_key_value" whose
*	valie entries are indicated by bits in "function_key_value_valid";
*	the function key mode "function_key_mode"  which
*	is valid if "function_key_mode_valid" is set;
*	the function key delimiter "function_key_delimiter"  which
*	is valid if "function_key_delimiter_valid" is set;
*	and function key label valid flag "function_key_label_valid".
**************************************************************************/
/*ARGSUSED*/
fct_pkey_key( pkey_key_type,
	      function_key_id,	function_key_id_valid,
	      function_key_value, function_key_value_valid,
	      function_key_mode, function_key_mode_valid,
	      function_key_delimiter, function_key_delimiter_valid,
	      function_key_label_valid, function_key_type_valid )

	int	pkey_key_type;
	int	function_key_id[];	/* id */
	int	function_key_id_valid;
	UNCHAR	*function_key_value[];
	int	function_key_value_valid;
	int	function_key_mode;	/* mode */
	int	function_key_mode_valid;
	UNCHAR	function_key_delimiter;	/* delimiter */
	int	function_key_delimiter_valid;		/* not used */
	int	function_key_label_valid;
	int	function_key_type_valid;
{
	UNCHAR		*remember_function_key();
	int		j;
	T_FUNCTION_KEY	*find_function_key_id();
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;
	UNCHAR		function_key_type;

	if (  function_key_mode_valid 
	   && ( function_key_mode == T_function_key_clear_mode_yes ) )
	{
		fct_function_key_clear();
	}
	if (  function_key_mode_valid 
	   && ( function_key_mode == T_function_key_clear_mode_default ) )
	{
		fct_function_key_default();
	}
	function_key_type = T_function_key_type_default;
	if ( function_key_type_valid )
	{
		UNCHAR	*pp;

		pp = function_key_value[ POS_FUNCTION_KEY_TYPE ];
		if ( pp != (UNCHAR *) 0 )
		{
			function_key_type = *pp;
		}
	}
	for ( j = 0; j < POS_FUNCTION_KEY_MODE; j++ )
	{
		if ( ( function_key_id_valid & ( 1 << j ) ) == 0 )
			break;
		t = find_function_key_id( T_function_key_ptr,
					  function_key_id[ j ] );
		if ( t != (T_FUNCTION_KEY *) 0 )
		{					/* know that one */
		    w = find_function_key_id( Outwin->function_key_ptr,
					      function_key_id[ j ] );
		    if ( w != (T_FUNCTION_KEY *) 0 )
		    {

						/* if zero length function key
						   values are ok or function key
						   value has a length - store.
						*/
			if ( pkey_key_type & PKEY_KEY_CLEAR )
			{
				w->function_key_value =
					remember_function_key( (UNCHAR *) "" );
				w->function_key_delimiter =
					function_key_delimiter;
			}
			else if 
			   (  ( T_function_key_len_0_label_only == 0         )
			   || (  ( function_key_value_valid & ( 1 << j ) )
			      && ( function_key_value[ j ] != NULL         )
			      && ( strlen( (char *) function_key_value[ j ] ) 
				   >  0  )
			   )  )
			{
				w->function_key_value =
						remember_function_key(
						function_key_value[ j ] );
				w->function_key_delimiter =
						function_key_delimiter;
			}
			if ( function_key_label_valid )
			{
				UNCHAR	*pp;

				pp = function_key_value[ 
						POS_FUNCTION_KEY_LABEL ];
				if (  ( T_function_key_label_max_len > 0 )
				   && ( pp != NULL )
				   && ( strlen( pp ) 
					> T_function_key_label_max_len )
				   )
				{
					pp[ T_function_key_label_max_len ] =
									'\0';
				}
				w->function_key_label =
				remember_function_key( pp );
			}
			else if ( T_function_key_len_0_func_only )
			{			/* label does not change */
			}
			else
				w->function_key_label = NULL;
			w->function_key_type = function_key_type;
			if ( outwin_is_curwin() )
				term_pkey_key(	
					t,
					function_key_id[ j ],
					w->function_key_value,
					function_key_delimiter,
					w->function_key_label,
					w->function_key_type );
		    }
		}
		else
		{			/* not recognized - beep if trying */
			if ( Opt_function_keys )
			{
				char	errbuff[ 80 ];

				term_beep();
				sprintf( errbuff,
					 "Unknown function key id '%c'",
					 function_key_id[ j ] );
				error_record_msg( errbuff );
				term_beep();
			}
			if ( outwin_is_curwin() )
			{
				if ( pkey_key_type & PKEY_KEY_CLEAR )
					term_pkey_key(	
						(T_FUNCTION_KEY *) 0,
						function_key_id[ j ],
						(UNCHAR *) "",
						function_key_delimiter,
						function_key_value[
						   POS_FUNCTION_KEY_LABEL ],
						function_key_type );
				else
					term_pkey_key(	
						(T_FUNCTION_KEY *) 0,
						function_key_id[ j ],
						function_key_value[ j ],
						function_key_delimiter,
						function_key_value[
						   POS_FUNCTION_KEY_LABEL ],
						function_key_type );
			}
		}
	}
}
#include "max_buff.h"
/**************************************************************************
* fct_pkey_key_ansi
*	The decoder has recognized the pkey_key sequence number
*	"pkey_key_type" on a terminal that has ansi function keys.
*	It decoded the function key ids "function_key_id" whose
*	valid entries are indicated by bits in "function_key_id valid";
*	the function key values "function_key_value" whose
*	valie entries are indicated by bits in "function_key_value_valid";
*	the function key mode "function_key_mode"  which
*	is valid if "function_key_mode_valid" is set;
*	the function key delimiter "function_key_delimiter"  which
*	is valid if "function_key_delimiter_valid" is set;
*	and function key label valid flag "function_key_label_valid".
**************************************************************************/
/*ARGSUSED*/
fct_pkey_key_ansi( pkey_key_type,
	      function_key_id,	function_key_id_valid, 
	      function_key_strings, function_key_strings_valid,
	      function_key_mode, function_key_mode_valid,
	      function_key_delimiter, function_key_delimiter_valid,
	      function_key_label_valid, function_key_type_valid )

	int	pkey_key_type;			/* not used */
	int	function_key_id[];	/* id */
	int	function_key_id_valid;
	UNCHAR	*function_key_strings[];
	int	function_key_strings_valid;
	int	function_key_mode;	/* mode */
	int	function_key_mode_valid;
	UNCHAR	function_key_delimiter;	/* delimiter */
	int	function_key_delimiter_valid;	/* not used */
	int	function_key_label_valid;
	int	function_key_type_valid;
{
	UNCHAR		*remember_function_key();
	int		k;
	char		number[ 15 ];
	char		value[ MAX_BUFF ];
	T_FUNCTION_KEY	*find_function_key_id();
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;
	UNCHAR		function_key_type;

	if (  function_key_mode_valid 
	   && ( function_key_mode == T_function_key_clear_mode_yes ) )
	{
		fct_function_key_clear();
	}
	if (  function_key_mode_valid 
	   && ( function_key_mode == T_function_key_clear_mode_default ) )
	{
		fct_function_key_default();
	}
	function_key_type = T_function_key_type_default;
	if ( function_key_type_valid )
	{
		UNCHAR	*pp;

		pp = function_key_strings[ POS_FUNCTION_KEY_TYPE ];
		if ( pp != (UNCHAR *) 0 )
		{
			function_key_type = *pp;
		}
	}
	value[ 0 ] = '\0';
	for ( k = 1; k < POS_FUNCTION_KEY_MODE; k++ )
	{
		if ( function_key_id_valid & ( 1 << k )  )
		{
			strcat( value, ";" );
			sprintf( number, "%d", function_key_id[ k ] );
			strcat( value, number );
		}
		else if ( function_key_strings_valid & ( 1 << k ) )
		{
			strcat( value, ";" );
			strcat( value, "\"" );
			strcat( value, (char *) function_key_strings[ k ] );
			strcat( value, "\"" );
		}
		else
			break;
	}
	t = find_function_key_id( T_function_key_ptr, function_key_id[ 0 ] );
	if ( t != (T_FUNCTION_KEY *) 0 )
	{					/* know that one */
	    w = find_function_key_id( Outwin->function_key_ptr,
				      function_key_id[ 0 ] );
	    if ( w != (T_FUNCTION_KEY *) 0 )
	    {
		if (  ( T_function_key_len_0_label_only == 0 )
		   || ( strlen( value ) >  0                 ) )
		{
			w->function_key_value =
				remember_function_key( (UNCHAR *) value );
			w->function_key_delimiter =
				function_key_delimiter;
		}
		if ( function_key_label_valid )
		{
			w->function_key_label =
				remember_function_key( 
				function_key_strings[ POS_FUNCTION_KEY_LABEL ]);
		}
		else if ( T_function_key_len_0_func_only )
		{
		}
		else
			w->function_key_label = NULL;
		w->function_key_type = function_key_type;
		if ( outwin_is_curwin() )
			term_pkey_key(	
				t,
				function_key_id[ 0 ],
				w->function_key_value,
				function_key_delimiter,
				w->function_key_label,
				w->function_key_type );
	    }
	}
	else
	{			/* not recognized - beep if trying */
		if ( Opt_function_keys )
		{
				char	errbuff[ 80 ];

				term_beep();
				sprintf( errbuff,
					 "Unknown function key id '%c'",
					 function_key_id[ 0 ] );
				error_record_msg( errbuff );
				term_beep();
		}
		if ( outwin_is_curwin() )
			term_pkey_key(	
				(T_FUNCTION_KEY *) 0,
				function_key_id[ 0 ],
				(UNCHAR *) value,
				function_key_delimiter,
				function_key_strings[
						POS_FUNCTION_KEY_LABEL ],
				function_key_type );
	}
}
char	*my_tparm();
/**************************************************************************
* term_pkey_key
*	Output the function key "t" and/or "function_key_id" to the
*	terminal with the value "function_key_value",
*	and the label "function_key_label",
*	using the delimiter "function_key_delimiter" if applicable.
*	If "t" is null, then this in not a known function key.
*	In this case, output but do not record.
**************************************************************************/
term_pkey_key(	t,
		function_key_id, function_key_value, 
		function_key_delimiter, function_key_label,
		function_key_type )
	T_FUNCTION_KEY	*t;		/* NULL = not in my list */
	int	function_key_id;
	UNCHAR	*function_key_value;
	UNCHAR	function_key_delimiter;
	UNCHAR	*function_key_label;
	UNCHAR	function_key_type;
{
	char	*p;
	int	parm[ POS_FUNCTION_KEY_DELIMITER + 1 ];
	char	*string_parm[ POS_FUNCTION_KEY_LABEL + 1 ];
	char	*out_pkey_key;
	int	using_clear;
	char	function_key_type_string[ 2 ];

	using_clear = 0;
	if ( T_out_pkey_key != NULL )
		out_pkey_key = T_out_pkey_key;
	if (  (  ( function_key_value == NULL ) 
	      || ( *function_key_value == '\0' ) )
	   && (  T_out_pkey_key_clear != NULL ) )
	{
		out_pkey_key = T_out_pkey_key_clear;
		using_clear = 1;
	}
	if ( out_pkey_key != NULL )
	{
		parm[ 0 ] = function_key_id;
		parm[ POS_FUNCTION_KEY_MODE ] = T_function_key_clear_mode_no;
		parm[ POS_FUNCTION_KEY_DELIMITER ] = function_key_delimiter;
		if ( function_key_value != NULL )
			string_parm[ 0 ] = (char *) function_key_value;
		else
			string_parm[ 0 ] = "";
		if ( function_key_label != NULL )
			string_parm[ POS_FUNCTION_KEY_LABEL ] =
						(char *) function_key_label;
		else
			string_parm[ POS_FUNCTION_KEY_LABEL ] = "";
		function_key_type_string[ 0 ] = function_key_type;
		function_key_type_string[ 1 ] = '\0';
		string_parm[ POS_FUNCTION_KEY_TYPE ] = function_key_type_string;
		p = my_tparm( out_pkey_key, parm, string_parm, -1 );
		term_tputs( p );
		if ( t != (T_FUNCTION_KEY *) 0)
		{
			if ( using_clear )
				t->last_function_key_value = NULL;
			else if (  (  T_function_key_len_0_label_only == 0   )
			        || (  ( function_key_value != NULL         )
			           && ( strlen( (char *) function_key_value )
					>  0 )
				)  )
			{
				t->last_function_key_value = function_key_value;
			}
			if (  ( T_function_key_len_0_func_only == 0   )
			   || (  ( function_key_label != NULL       )
			      && ( strlen( (char *) function_key_label ) > 0 ) )
			   )
			{
				t->last_function_key_label = function_key_label;
			}
			t->last_function_key_type = function_key_type;
		}
	}
}
/**************************************************************************
* t_sync_function_key
*	Syncronize the terminal, if necessary, to the function key
*	values for "Outwin".
**************************************************************************/
t_sync_function_key()
{
	t_sync_function_key_common( Outwin->function_key_ptr );
}
/**************************************************************************
* t_sync_function_key_default
*	Syncronize the terminal, if necessary, to the default function key
*	values.
**************************************************************************/
t_sync_function_key_default()
{
	t_sync_function_key_common( T_function_key_ptr );
}
/**************************************************************************
* t_sync_function_key_common
*	Syncronize the terminal, if necessary, to the function key
*	values specified by the function key list "to".
**************************************************************************/
t_sync_function_key_common( to )
	T_FUNCTION_KEY	*to;
{
	T_FUNCTION_KEY	*w;
	UNCHAR	*want;
	T_FUNCTION_KEY	*t;

	for (   t = T_function_key_ptr,          w = to;
	      ( t != (T_FUNCTION_KEY *) 0 ) && ( w != (T_FUNCTION_KEY *) 0 );
	        t = t->next,			 w = w->next )
	{
		want = w->function_key_value;
		if ( want != t->last_function_key_value )
			break;
		if ( T_function_key_includes_label )
		{
			if ( w->function_key_label !=
						t->last_function_key_label )
				break;
		}
		if ( w->function_key_type != t->last_function_key_type )
			break;
	}
	if ( ( t == (T_FUNCTION_KEY *) 0 ) || ( w == (T_FUNCTION_KEY *) 0 ) )
		return;
	if ( T_function_key_default != (char *) 0 )
		term_function_key_default();
	else
		term_function_key_clear();
	for (   t = T_function_key_ptr,          w = to;
	      ( t != (T_FUNCTION_KEY *) 0 ) && ( w != (T_FUNCTION_KEY *) 0 );
	        t = t->next,			 w = w->next )
	{
		want = w->function_key_value;
		if (  ( want != t->last_function_key_value )
		   || (  T_function_key_includes_label
		      && ( w->function_key_label != t->last_function_key_label )
		      )
		   || ( w->function_key_type != t->last_function_key_type )
		   )
		{
			term_pkey_key(	t, 
					w->function_key_id, 
					want,
					w->function_key_delimiter,
					w->function_key_label,
					w->function_key_type );
		}
	}
}
/**************************************************************************
* dec_function_key_clear
*	DECODE module for 'function_key_clear'.
**************************************************************************/
dec_function_key_clear()
{
	fct_function_key_clear();
}
/**************************************************************************
* inst_function_key_clear
*	INSTALL module for 'function_key_clear'.
**************************************************************************/
inst_function_key_clear( str )
	char	*str;
{
	dec_install( "function_key_clear", (UNCHAR *) str,
					dec_function_key_clear, 0, NO_OPTION,
					(char *) 0 );
}
/**************************************************************************
* fct_function_key_clear
*	ACTION module for 'function_key_clear'.
**************************************************************************/
fct_function_key_clear()
{
	T_FUNCTION_KEY	*w;

	for (   w = Outwin->function_key_ptr; 
		w != (T_FUNCTION_KEY *) 0; 
		w = w->next )
	{
		w->function_key_value = NULL;
		if ( T_function_key_includes_label )
			w->function_key_label = NULL;
		w->function_key_type = T_function_key_type_default;
	}
	if ( outwin_is_curwin() )
		term_function_key_clear();
}
/**************************************************************************
* term_function_key_clear
*	TERMINAL OUTPUT module for 'function_key_clear'.
**************************************************************************/
term_function_key_clear()
{
	T_FUNCTION_KEY	*t;

	if ( T_function_key_clear != NULL )
	{
		term_tputs( T_function_key_clear );
		for (   t = T_function_key_ptr; 
			t != (T_FUNCTION_KEY *) 0; 
			t = t->next )
		{
			t->last_function_key_value = NULL;
			t->last_function_key_label = NULL;
			t->last_function_key_type = T_function_key_type_default;
		}
	}
}
/**************************************************************************
* dec_function_key_default
*	DECODE module for 'function_key_default'.
**************************************************************************/
dec_function_key_default()
{
	fct_function_key_default();
}
/**************************************************************************
* inst_function_key_default
*	INSTALL module for 'function_key_default'.
**************************************************************************/
inst_function_key_default( str )
	char	*str;
{
	dec_install( "function_key_default", (UNCHAR *) str,
					dec_function_key_default, 0, NO_OPTION,
					(char *) 0 );
}
/**************************************************************************
* fct_function_key_default
*	ACTION module for 'function_key_default'.
**************************************************************************/
fct_function_key_default()
{
	T_FUNCTION_KEY	*t;
	T_FUNCTION_KEY	*w;

	for (   t = T_function_key_ptr,	w = Outwin->function_key_ptr;
		   ( t != (T_FUNCTION_KEY *) 0 ) 
		&& ( w != (T_FUNCTION_KEY *) 0 );
		t = t->next,		w = w->next )
	{
		w->function_key_value =       	t->function_key_value;
		w->function_key_label =		t->function_key_label;
		w->function_key_type =		t->function_key_type;
		w->function_key_delimiter =	t->function_key_delimiter;
	}
	win_se_perwindow_clear( 'n', Outwin->number );
	if ( outwin_is_curwin() )
		term_function_key_default();
}
/**************************************************************************
* term_function_key_default
*	TERMINAL OUTPUT module for 'function_key_default'.
**************************************************************************/
term_function_key_default()
{
	T_FUNCTION_KEY	*t;

	if ( T_function_key_default != NULL )
	{
		term_tputs( T_function_key_default );
		for (   t = T_function_key_ptr; 
			t != (T_FUNCTION_KEY *) 0; 
			t = t->next )
		{
			t->last_function_key_value =	t->function_key_value;
			t->last_function_key_label =	t->function_key_label;
			t->last_function_key_type =	t->function_key_type;
		}
	}
	term_se_perwindow_default( 'n' );
}

/****************************************************************************/
char	*dec_encode();
#include "termdesc.h"
/**************************************************************************
* extra_pkey_key
*	TERMINAL DESCRIPTION PARSER module for function keys.
**************************************************************************/
extra_pkey_key( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	UNCHAR	*function_key_encode();
	UNCHAR	*enc;
	char	tempbuff[ TERM_DESC_BUFF_LEN + 1 ];
	char	*temp_encode();
	long	*mymalloc();
	char	*strdup();
	UNCHAR	*remember_function_key_hex();
	UNCHAR	*remember_function_key_ansi();
	UNCHAR	*remember_function_key_escaped();

	if ( strcmp( buff, "pkey_key" ) == 0 )
	{
		T_pkey_key = dec_encode( string );
		inst_pkey_key( T_pkey_key, 0 );
		T_out_pkey_key = T_pkey_key;
	}
	else if ( strcmp( buff, "out_pkey_key" ) == 0 )
	{
		T_out_pkey_key = dec_encode( string );
	}
	else if ( strcmp( buff, "pkey_key_clear" ) == 0 )
	{
		T_pkey_key_clear = dec_encode( string );
		inst_pkey_key( T_pkey_key_clear, PKEY_KEY_CLEAR );
		T_out_pkey_key_clear = T_pkey_key_clear;
	}
	else if ( strcmp( buff, "out_pkey_key_clear" ) == 0 )
	{
		T_out_pkey_key_clear = dec_encode( string );
	}
	else if ( strcmp( buff, "parm_pkey_key" ) == 0 )
	{
		T_pkey_key = dec_encode( string );
		inst_parm_pkey_key( T_pkey_key, 0 );
		T_out_pkey_key = T_pkey_key;
	}
	else if ( strcmp( buff, "function_key_default" ) == 0 )
	{
		T_function_key_default = dec_encode( string );
		inst_function_key_default( T_function_key_default );
	}
	else if ( strcmp( buff, "function_key_clear" ) == 0 )
	{
		T_function_key_clear = dec_encode( string );
		inst_function_key_clear( T_function_key_clear );
	}
	else if ( strcmp( buff, "function_key_clear_mode_yes" ) == 0 )
	{
		T_function_key_clear_mode_yes = atoi( string );
	}
	else if ( strcmp( buff, "function_key_clear_mode_no" ) == 0 )
	{
		T_function_key_clear_mode_no = atoi( string );
	}
	else if ( strcmp( buff, "function_key_clear_mode_default" ) == 0 )
	{
		T_function_key_clear_mode_default = atoi( string );
	}
	else if ( strcmp( buff, "function_key_includes_label" ) == 0 )
	{
		T_function_key_includes_label = 1;
	}
	else if ( strcmp( buff, "function_key_label_max_len" ) == 0 )
	{
		T_function_key_label_max_len = atoi( string );
	}
	else if ( strcmp( buff, "function_key_len_0_label_only" ) == 0 )
	{
		T_function_key_len_0_label_only = 1;
	}
	else if ( strcmp( buff, "function_key_len_0_func_only" ) == 0 )
	{
		T_function_key_len_0_func_only = 1;
	}
	else if ( strcmp( buff, "function_key" ) == 0 )
	{
		if ( Opt_function_keys == 0 )
		{
			/* do not read */
		}
		else if ( attr_on_string == NULL )
		{
			printf ( "function_key id is NULL '%s'\n",
				 string );
		}
		else
		{
			UNCHAR d;
			T_FUNCTION_KEY	*t;

			t = (T_FUNCTION_KEY *) mymalloc( 
						sizeof( T_FUNCTION_KEY ),
						"function_key" );
			t->next = (T_FUNCTION_KEY *) 0;
			link_last( (T_STRUCT *) t,
				   (T_STRUCT *) &T_function_key_ptr );
			t->function_key_id = 
				function_key_id_encode( attr_on_string );
			if ( attr_off_string == NULL )
				d = '"';
			else
				d = attr_off_string[ 0 ];
			t->function_key_delimiter = d;
			enc = function_key_encode( string );
			if ( T_function_key_string_is_hex )
				enc = remember_function_key_hex( enc );
			else if ( T_function_key_string_is_ansi)
				enc = remember_function_key_ansi( enc );
			else if ( T_function_key_string_is_escaped)
				enc = remember_function_key_escaped( enc );
			t->function_key_value = enc;
			if ( T_function_key_includes_label )
			{
				t->function_key_label =
				 function_key_encode( attr_off_string );
			}
			else
				t->function_key_label = NULL;
			t->function_key_type = T_function_key_type_default;
			t->last_function_key_value = NULL;
			t->last_function_key_label = NULL;
			t->last_function_key_type =
						T_function_key_type_default;
			T_function_key_no++;
		}
	}
	else if ( strcmp( buff, "shift_function_key" ) == 0 )
	{
		T_shift_function_key_no = T_function_key_no;
	}
	else if ( strcmp( buff, "function_key_string_is_hex" ) == 0 )
	{
		T_function_key_string_is_hex = 1;
	}
	else if ( strcmp( buff, "function_key_string_is_ansi" ) == 0 )
	{
		T_function_key_string_is_ansi = 1;
	}
	else if ( strcmp( buff, "function_key_string_is_escaped" ) == 0 )
	{
		if ( ( attr_on_string == NULL ) || ( *attr_on_string == '\0' ) )
		{
			printf ( 
			"function_key_string_is_escaped list is missing\n" );
		}
		else
		{
			T_function_key_string_is_escaped = 1;
			T_func_key_escaped_char =
				  *( temp_encode( string, tempbuff ) );
			T_func_key_escaped_list = 
					(UNCHAR *) dec_encode( attr_on_string );
		}
	}
	else if ( strcmp( buff, "function_key_file" ) == 0 )
	{
		T_function_key_file = strdup( string );
	}
	else if ( strcmp( buff, "function_key_type_default" ) == 0 )
	{
		if ( string == NULL )
		{
			printf ( "function_key_type_default value is NULL\n" );
		}
		else
		{
			T_function_key_type_default =
				  *( temp_encode( string, tempbuff ) );
			T_function_key_type_set = T_function_key_type_default;
		}
	}
	else if ( strcmp( buff, "function_key_type_set" ) == 0 )
	{
		if ( string == NULL )
		{
			printf ( "function_key_type_set value is NULL\n" );
		}
		else
		{
			T_function_key_type_set =
				  *( temp_encode( string, tempbuff ) );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* function_key_id_encode
*	Encode the function key id "s".
*	If it is a single character, then that character is the id.
*	Otherwise it is treated as a decimal number whose value is returned.
**************************************************************************/
function_key_id_encode( s )
	char	*s;
{
	int	len;

	len = strlen( s );
	if ( len < 1 )
		return( -1 );
	if ( len == 1 )
		return( s[ 0 ] );
	return( atoi( s ) );
}
#define FUNCTION_KEY_STRINGS 400
UNCHAR	*Function_key_ptrs[ FUNCTION_KEY_STRINGS ];
int	Function_key_ptrs_no = 0;

/**************************************************************************
* function_key_encode
*	Encode a function key value "string".
*	Remember it and return a pointer to the remembered string.
**************************************************************************/
UNCHAR *
function_key_encode( string )
	char	*string;
{
	UNCHAR	buff[ TERM_DESC_BUFF_LEN + 1 ];
	char	*temp_encode();
	UNCHAR	*remember_function_key();

	if ( string == NULL )
		return( NULL );
	if ( string[ 0 ] == '\0' )
		return( NULL );
	if ( strlen( string ) >= TERM_DESC_BUFF_LEN )
	{
		printf( "\r\nERROR: function key string too long\r\n" );
		term_outgo();
		return( NULL );
	}
	temp_encode( string, (char *) buff );
	return( remember_function_key( buff ) );
}
/**************************************************************************
* remember_function_key_hex
*	Encode a function key value "string" on a terminal that uses
*	hex function keys.
*	Remember it and return a pointer to the remembered string.
**************************************************************************/
UNCHAR *
remember_function_key_hex( string )
	UNCHAR	*string;
{
	UNCHAR	c;
	int	cc;
	UNCHAR	buff[ TERM_DESC_BUFF_LEN + 1 ];
	UNCHAR	*p;
	UNCHAR	*s;
	UNCHAR	*remember_function_key();

	if ( string == NULL )
		return( NULL );
	s = string;
	p = buff;
	while ( (c = *s++) != '\0' )
	{
		cc = ( c >> 4 ) & 0x0F;
		if ( cc > 9 )
			*p++ = 'A' + cc - 10;
		else
			*p++ = '0' + cc;
		cc = c & 0x0F;
		if ( cc > 9 )
			*p++ = 'A' + cc - 10;
		else
			*p++ = '0' + cc;
	}
	*p++ = '\0';
	return( remember_function_key( buff ) );
}
/**************************************************************************
* remember_function_key_ansi
*	Encode a function key value "string" on a terminal that uses
*	ansi valued function keys.
*	Remember it and return a pointer to the remembered string.
**************************************************************************/
UNCHAR *
remember_function_key_ansi( string )
	UNCHAR	*string;
{
	UNCHAR	c;
	UNCHAR	buff[ TERM_DESC_BUFF_LEN + 1 ];
	UNCHAR	*p;
	UNCHAR	*s;
	UNCHAR	*remember_function_key();
	char	number[ 15 ];
	int	in_string;
	char	*n;

	if ( string == NULL )
		return( NULL );
	s = string;
	p = buff;
	in_string = 0;
	while ( (c = *s++) != '\0' )
	{
		if ( ( c < ' ' ) || ( c > '~' ) || ( c == '"' ) )
		{
			if ( in_string )
			{
				*p++ = '"';
				in_string = 0;
			}
			*p++ = ';';
			sprintf( number, "%d", (int) c );
			n = number;
			while ( (c = *n++) != '\0' )
				*p++ = c;
		}
		else
		{
			if ( in_string == 0 )
			{
				*p++ = ';';
				*p++ = '"';
				in_string = 1;
			}
			*p++ = c;
		}
	}
	if ( in_string )
		*p++ = '"';
	*p++ = '\0';
	return( remember_function_key( buff ) );
}
/**************************************************************************
* remember_function_key_escaped
*	Encode a function key value "string" on a terminal that uses
*	escaped valued function keys.
*	Remember it and return a pointer to the remembered string.
**************************************************************************/
UNCHAR *
remember_function_key_escaped( string )
	UNCHAR	*string;
{
	UNCHAR	c;
	UNCHAR	buff[ TERM_DESC_BUFF_LEN + 1 ];
	UNCHAR	*p;
	UNCHAR	*s;
	UNCHAR	*remember_function_key();
	UNCHAR	*l;
	UNCHAR	cc;

	if ( string == NULL )
		return( NULL );
	s = string;
	p = buff;
	while ( (c = *s++) != '\0' )
	{
		l = T_func_key_escaped_list;
		while( (cc = *l++) != '\0' )
		{
			if ( c == cc )
			{
				*p++ = T_func_key_escaped_char;
				break;
			}
		}
		*p++ = c;
	}
	*p++ = '\0';
	return( remember_function_key( buff ) );
}
/**************************************************************************
* remember_function_key
*	Find a matching old or store a new function key value "string" and
*	return a pointer to it.
**************************************************************************/
UNCHAR *
remember_function_key( string )
	UNCHAR	*string;
{
	UNCHAR	*t;
	UNCHAR	*find_function_key();
	UNCHAR	*store_function_key();

	if ( string == NULL )
		return( NULL );
	t = find_function_key( string );
	if ( t == NULL )
		t = store_function_key( string );
	return( t );
}
/**************************************************************************
* find_function_key
*	Try to find a previously stored function key value that matches
*	"string".
*	If so return a pointer to it, otherwise return NULL.
**************************************************************************/
UNCHAR *
find_function_key( string )
	UNCHAR	*string;
{
	int	i;

	for ( i = 0; i < Function_key_ptrs_no; i++ )
	{
		if ( strcmp( (char *) string, (char *) Function_key_ptrs[ i ] ) 
		     == 0 )
			return( Function_key_ptrs[ i ] );
	}
	return( NULL );
}
/**************************************************************************
* store_function_key
*	Store a new function key value "string" and return a pointer to
*	it.
**************************************************************************/
UNCHAR *
store_function_key( string )
	UNCHAR	*string;
{
	UNCHAR	*begin;
	UNCHAR	*s;
	int	len;
	long	*malloc_run();

	if ( string == NULL )
		return( NULL );
	if ( string[ 0 ] == '\0' )
		return( NULL );
	if ( Function_key_ptrs_no >= FUNCTION_KEY_STRINGS - 1 )
	{
		printf( "\r\nERROR: function key ptrs exhausted\r\n" );
		term_outgo();
		return( NULL );
	}
	s = string;
	len = strlen( (char *) s );
	begin = ( UNCHAR * ) malloc_run( len + 1, "store_function_key" );
	if ( begin == NULL )
	{
		printf( "\r\nERROR: Function Key malloc failed\r\n" );
		term_outgo();
		return( NULL );
	}
	strcpy( (char *) begin, (char *) s );
	Function_key_ptrs[ Function_key_ptrs_no++ ] = begin;
	return( begin );
}
int	Max_label_from_title = 80;
int	Min_label_from_title = 1;
/**************************************************************************
* set_max_label_from_title
*	Store a value to limit the number of characters that will be 
*	accepted when storing a window title in a label.
**************************************************************************/
set_max_label_from_title( len )
	int	len;
{
	Max_label_from_title = len;
}
/**************************************************************************
* set_min_label_from_title
*	Store a value to be the lower limit on the number of characters 
*	that will be accepted when storing a window title in a label.
*	This is done to prevent no title from not programming the label.
**************************************************************************/
set_min_label_from_title( len )
	int	len;
{
	Min_label_from_title = len;
}
/**************************************************************************
* encode_label_titles
*	Replace the construct "\p[0-9]" with the appropriate window title.
*	"in" is the input string, "out" is the output string which can
*	store a maximum of "max" characters.
**************************************************************************/
encode_label_titles( in, out, max )
	char	*in;
	char	*out;
	int	max;
{
	int	winno;
	char	*ptr;
	char	*get_exec_list_ptr();
	char	*s;
	char	*d;
	int	left;
	char	c;
	int	limit;
	int	need_pad;

	s = in;
	d = out;
	left = max - 5;
	while ( (c = *s++ ) != '\0' )
	{
		if ( c == '\\' )
		{
			c = *s++;
			if ( c == 'p' )
			{
				c = *s++;
				if ( ( c >= '0' ) && ( c <= '9' ) )
				{
					if ( c == '0' )
						winno = 9;
					else
						winno = c - '1';
					ptr = get_exec_list_ptr( winno );
					limit = Max_label_from_title;
					need_pad = Min_label_from_title;
					while (  ( *ptr != '\0' )
					      && ( limit-- > 0  ) )
					{
						*d++ = *ptr++;
						need_pad--;
						if ( --left <= 0 )
							break;
					}
					while ( need_pad > 0 )
					{
						*d++ = ' ';
						need_pad--;
						if ( --left <= 0 )
							break;
					}
				}
				else
				{
					*d++ = '\\';
					*d++ = 'p';
					*d++ = c;
					left--;
					left--;
					left--;
					if ( c == '\0' )
						break;
				}
			}
			else
			{
				*d++ = '\\';
				*d++ = c;
				left--;
				left--;
			}
		}
		else
		{
				*d++ = c;
				left--;
		}
		if( left <= 0 )
			break;
	}
	*d++ = '\0';
}
