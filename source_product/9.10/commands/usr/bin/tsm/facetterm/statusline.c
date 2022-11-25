/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: statusline.c,v 70.1 92/03/09 15:46:59 ssa Exp $ */
/**************************************************************************
* statusline.c
*	Support terminals with a single-sequence status line command.
*	Also supports labels and attributes on that same status line.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"

char	M_status_line[ MAX_STATUS_LINE_CHARS + 1 ] = "";
char	*T_status_line = NULL;

int	M_status_on = 0;
char	*T_status_on = NULL;
char	*T_status_off = NULL;

int	M_status_type = 0;
#define MAX_STATUS_TYPE	20
int	T_status_typeno = 0;
char	*T_status_type[ MAX_STATUS_TYPE ]={NULL};

char	M_status_line_label[ MAX_STATUS_LINE_LABEL ]
			   [ MAX_STATUS_LINE_LABEL_CHARS + 1 ] =
{ "\377", "\377", "\377", "\377", "\377", "\377", "\377", "\377" };
int	T_status_line_labelno = 0;
char	*T_status_line_label[ MAX_STATUS_LINE_LABEL ]={NULL};

int	F_status_off_clears_status_type = 0;
int	F_status_off_clears_status_line = 0;
int	F_status_type_turns_status_on = 0;
int	F_status_type_ruins_status_line = 0;
int	F_status_line_turns_status_on = 0;
int	F_status_line_clears_status_line_labels = 0;
int	F_status_off_clears_status_labels = 0;

/**************************************************************************
* init_status_line
*	Startup for window 'win' - all clear.
**************************************************************************/
init_status_line( win )
	FT_WIN	*win;
{
	int	i;

	win->status_on = 0;
	win->status_type = 0;
	win->status_line[ 0 ] = '\0';
	win->status_type_after_status_line = 0;
	win->status_line_labels_after_status_line = 0;
	for ( i = 0; i < T_status_line_labelno; i++ )
	{
		win->status_line_label[ i ][ 0 ] = 0xFF;
		win->status_line_label[ i ][ 1 ] = '\0';
		win->status_line_label[ i ][ MAX_STATUS_LINE_LABEL_CHARS ] =
									'\0';
	}
}
/**************************************************************************
* win_se_no_status
*	An event has occurred that had the side effect of setting the
*	status line of the terminal to all clear.
**************************************************************************/
win_se_no_status()
{
	init_status_line( Outwin );
}
/**************************************************************************
* dec_status_line
*	DECODE module for 'status_line'.
*	Set the entire status line to '*string_parm'.
**************************************************************************/
/*ARGSUSED*/
dec_status_line( func_parm, parm_ptr, parms_valid, parm, string_parm )
	int	func_parm;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	fct_status_line( string_parm[ 0 ] );
}
/**************************************************************************
* inst_status_line
*	INSTALL module for 'status_line'.
**************************************************************************/
inst_status_line( str )
	char	*str;
{
	dec_install( "status_line", (UNCHAR *) str, dec_status_line, 
		0, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_status_line
*	ACTION module for 'status_line'.
**************************************************************************/
fct_status_line( string )
	char	*string;
{
	int	i;

	strncpy( (char *) Outwin->status_line, string, MAX_STATUS_LINE_CHARS );
	Outwin->status_line[ MAX_STATUS_LINE_CHARS ] = '\0';
	Outwin->status_type_after_status_line = 0;
	if ( F_status_line_turns_status_on )
		Outwin->status_on = 1;
	if ( F_status_line_clears_status_line_labels )
	{
		for ( i = 0; i < T_status_line_labelno; i++ )
		{
			Outwin->status_line_label[ i ][ 0 ] = 0xFF;
			Outwin->status_line_label[ i ][ 1 ] = '\0';
		}
	}
	if ( outwin_is_curwin() )
		term_status_line( string );
}
extern	char	*my_tparm();
/**************************************************************************
* term_status_line
*	TERMINAL OUTPUT module for 'status_line'.
**************************************************************************/
term_status_line( string )
	char	*string;
{
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];

	if ( T_status_line != NULL )
	{
		string_parm[ 0 ] = string;
		p = my_tparm( T_status_line, parm, string_parm, -1 );
		term_tputs( p );
		if ( F_status_line_turns_status_on )	/* ??? conditional */
			M_status_on = 1;
		if ( F_status_line_clears_status_line_labels )
			term_se_clear_status_line_labels();
	}
	strcpy( M_status_line, string );
}
/**************************************************************************
* term_se_no_status
*	An event has occurred that had the side effect of setting the
*	status line of the terminal to off.
**************************************************************************/
term_se_no_status()
{
	M_status_on = 0;
	M_status_line[ 0 ] = '\0';
	M_status_type = 0;
	term_se_clear_status_line_labels();
}
/**************************************************************************
* term_se_clear_status_line_labels
*	An event has occurred that had the side effect of clearing the
*	status line labels of the terminal.
**************************************************************************/
term_se_clear_status_line_labels()
{
	int	i;

	for ( i = 0; i < T_status_line_labelno; i++ )
	{
		M_status_line_label[ i ][ 0 ] = 0xFF;
		M_status_line_label[ i ][ 1 ] = '\0';
	}
}
/**************************************************************************
* dec_status_on
*	DECODE module for 'status_on'.
*	Turn the status line on.
**************************************************************************/
dec_status_on()
{
	fct_status_on();
}
/**************************************************************************
* inst_status_on
*	INSTALL module for 'status_on'.
**************************************************************************/
inst_status_on( str )
	char	*str;
{
	dec_install( "status_on", (UNCHAR *) str, dec_status_on, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* fct_status_on
*	ACTION module for 'status_on'.
**************************************************************************/
fct_status_on()
{
	Outwin->status_on = 1;
	if ( outwin_is_curwin() )
		term_status_on();
}
/**************************************************************************
* term_status_on
*	TERMINAL OUTPUT module for 'status_on'.
**************************************************************************/
term_status_on()
{
	if ( T_status_on != NULL )
	{
		term_tputs( T_status_on );
		M_status_on = 1;
	}
}
/**************************************************************************
* dec_status_off
*	DECODE module for 'status_off'.
*	Turn the status line off.
**************************************************************************/
dec_status_off()
{
	fct_status_off();
}
/**************************************************************************
* inst_status_off
*	INSTALL module for 'status_off'.
**************************************************************************/
inst_status_off( str )
	char	*str;
{
	dec_install( "status_off", (UNCHAR *) str, dec_status_off, 0, 0,
	(char *) 0 );
}
/**************************************************************************
* fct_status_off
*	ACTION module for 'status_off'.
**************************************************************************/
fct_status_off()
{
	int	i;

	Outwin->status_on = 0;
	if ( F_status_off_clears_status_line )
	{
		Outwin->status_line[ 0 ] = '\0';
		Outwin->status_type_after_status_line = 0;
	}
	if ( F_status_off_clears_status_type )
	{
		Outwin->status_type = 0;
		Outwin->status_type_after_status_line = 0;
	}
	if ( F_status_off_clears_status_labels )
	{
		for ( i = 0; i < T_status_line_labelno; i++ )
		{
			Outwin->status_line_label[ i ][ 0 ] = 0xFF;
			Outwin->status_line_label[ i ][ 1 ] = '\0';
		}
	}
	if ( outwin_is_curwin() )
		term_status_off();
}
/**************************************************************************
* term_status_off
*	TERMINAL OUTPUT module for 'status_off'.
**************************************************************************/
term_status_off()
{
	int	i;

	if ( T_status_off != NULL )
	{
		term_tputs( T_status_off );
		M_status_on = 0;
		if ( F_status_off_clears_status_type )
			M_status_type = -1;
		if ( F_status_off_clears_status_line )
			M_status_line[ 0 ] = '\0';
		if ( F_status_off_clears_status_labels )
		{
			for ( i = 0; i < T_status_line_labelno; i++ )
			{
				M_status_line_label[ i ][ 0 ] = 0xFF;
				M_status_line_label[ i ][ 1 ] = '\0';
			}
		}
	}
}
/**************************************************************************
* dec_status_type
*	DECODE module for 'status_type'.
*	Set status line attribute.
**************************************************************************/
/*ARGSUSED*/
dec_status_type( typeno, parm_ptr )
	int	typeno;
	char	*parm_ptr;
{
	fct_status_type( typeno );
}
/**************************************************************************
* inst_status_type
*	INSTALL module for 'status_type'.
**************************************************************************/
inst_status_type( str, typeno )
	char	*str;
	int	typeno;
{
	dec_install( "status_type", (UNCHAR *) str, dec_status_type, 
		typeno, 0,
		(char *) 0 );
}
/**************************************************************************
* fct_status_type
*	ACTION module for 'status_type'.
**************************************************************************/
fct_status_type( typeno )
	int	typeno;
{
	Outwin->status_type = typeno;
	Outwin->status_type_after_status_line = 1;
	if ( F_status_type_turns_status_on )
		Outwin->status_on = 1;
	if ( outwin_is_curwin() )
		term_status_type( typeno );
}
/**************************************************************************
* term_status_type
*	TERMINAL OUTPUT module for 'status_type'.
**************************************************************************/
term_status_type( typeno )
	int typeno;
{
	char	*statusstr;

	if ( typeno < T_status_typeno )
	{
		statusstr = T_status_type[ typeno ];
		if ( statusstr != NULL )
		{
			term_tputs( statusstr );
			M_status_type = typeno;
			if ( F_status_type_turns_status_on )
				M_status_on = 1;
		}
	}
}
/**************************************************************************
* dec_status_line_label
*	DECODE module for 'status_line_label'.
*	Set status line label number "labelno" to '*string_parm'.
**************************************************************************/
/*ARGSUSED*/
dec_status_line_label( labelno, parm_ptr, parms_valid, parm, string_parm )
	int	labelno;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
	char	*string_parm[];
{
	fct_status_line_label( labelno, string_parm[ 0 ] );
}
/**************************************************************************
* inst_status_line_label
*	INSTALL module for 'status_line_label'.
*	"labelno" is the label number.
**************************************************************************/
inst_status_line_label( str, labelno )
	char	*str;
	int	labelno;
{
	dec_install( "status_line_label", (UNCHAR *) str, dec_status_line_label, 
		labelno, CURSOR_OPTION,
		(char *) 0 );
}
/**************************************************************************
* fct_status_line_label
*	ACTION module for 'status_line_label'.
*	"labelno" is the label number.
*	"string" is the label contents.
**************************************************************************/
fct_status_line_label( labelno, string )
	int	labelno;
	char	*string;
{
	strncpy( (char *) Outwin->status_line_label[ labelno ], 
		 string, MAX_STATUS_LINE_LABEL_CHARS );
	Outwin->status_line_label[ labelno ][ MAX_STATUS_LINE_LABEL_CHARS ] = 
									'\0';
	if ( outwin_is_curwin() )
		term_status_line_label( labelno, string );
}
/**************************************************************************
* term_status_line_label
*	TERMINAL OUTPUT module for 'status_line_label'.
*	"labelno" is the label number.
*	"string" is the label contents.
**************************************************************************/
term_status_line_label( labelno, string )
	int	labelno;
	char	*string;
{
	char	*status_line_labelstr;
	char	*p;
	int	parm[ 2 ];			/* 1 used - must be >= 2 - %i */
	char	*string_parm[ 1 ];

	if ( labelno < T_status_line_labelno )
	{
		status_line_labelstr = T_status_line_label[ labelno ];
		if ( status_line_labelstr != NULL )
		{
			string_parm[ 0 ] = string;
			p = my_tparm( status_line_labelstr, parm, string_parm,
									-1 );
			term_tputs( p );
		}
	}
	strncpy( M_status_line_label[ labelno ], 
		 string, MAX_STATUS_LINE_LABEL_CHARS );
	M_status_line_label[ labelno ][ MAX_STATUS_LINE_LABEL_CHARS ] = '\0';
}

/**************************************************************************
* t_sync_status_line
*	Syncronize the terminal, if necessary, for the window "win".
**************************************************************************/
t_sync_status_line( win )
	FT_WIN	*win;
{
	int	status_on;
	int	status_type;
	int	i;

	status_on = win->status_on;
	status_type = win->status_type;
	if ( status_on )
	{
		if ( M_status_on == 0 )
			term_status_on();
	}
	else
	{
		if ( M_status_on )
			term_status_off();
	}
	if ( status_on || M_status_on )
	{
		if (  ( status_type != M_status_type )
		   || ( strcmp( M_status_line, (char *) win->status_line ) 
			!= 0 ) )
		{
			if ( win->status_type_after_status_line )
			{
				term_status_line( (char *) win->status_line );
				term_status_type( status_type );
			}
			else
			{
				term_status_type( status_type );
				term_status_line( (char *) win->status_line );
			}
		}
	}
	for ( i = 0; i < T_status_line_labelno; i++ )
	{
		if ( strcmp( win->status_line_label[ i ],
			     M_status_line_label[ i ] ) != 0 )
		{
			term_status_line_label( i,
						win->status_line_label[ i ] );
		}
	}
}
/**************************************************************************
* t_sync_status_line_modes_off
*	Syncronize the terminal, if necessary, to the have the status
*	line turned off.
**************************************************************************/
t_sync_status_line_modes_off()
{
	int	i;

	if ( M_status_on )
		term_status_off();
	if ( M_status_on )
	{
		if (  ( M_status_type )
		   || ( strcmp( M_status_line, "" ) != 0 ) )
		{
			term_status_type( 0 );
			term_status_line( "" );
		}
	}
	for ( i = 0; i < T_status_line_labelno; i++ )
	{
		if ( strcmp( "\377", M_status_line_label[ i ] ) != 0 )
		{
			term_status_line_label( i, "" );
			M_status_line_label[ i ][ 0 ] = '\377';
			M_status_line_label[ i ][ 1 ] = '\0';
		}
	}
}
/**************************************************************************
* status_line_label_default
*	Set the status line labels to their default values for window
*	"Outwin".
**************************************************************************/
status_line_label_default()
{
	int	i;

	for ( i = 0; i < T_status_line_labelno; i++ )
	{
		Outwin->status_line_label[ i ][ 0 ] = 0xFF;
		Outwin->status_line_label[ i ][ 1 ] = '\0';
	}
}
/**************************************************************************
* status_line_label_load
*	Load the status line label number "labelno" with the string
*	"string" on windows "start_winno" to "end_winno".
**************************************************************************/
status_line_label_load( start_winno, end_winno, labelno, string )
	int	start_winno;
	int	end_winno;
	int	labelno;
	char	*string;
{
	int	winno;
	FT_WIN	*win;

	if ( labelno >= 0 && labelno < T_status_line_labelno )
	{
		for ( winno = start_winno; winno <= end_winno; winno++ )
		{
			win = Wininfo[ winno ];
			strncpy( win->status_line_label[ labelno ], 
				 string, MAX_STATUS_LINE_LABEL_CHARS );
			win->status_line_label[ labelno ]
					      [ MAX_STATUS_LINE_LABEL_CHARS ] = 
					      '\0';
		}
	}
}
/**************************************************************************
* status_line_load
*	Load the status line with the string "string" on windows
*	number "start_winno" to "end_winno".
*	Return 0 if the terminal has a status line.
*	Return -1 if not.
**************************************************************************/
status_line_load( start_winno, end_winno, string )
	int	start_winno;
	int	end_winno;
	char	*string;
{
	int	winno;
	FT_WIN	*win;
	int	i;
	char	buff[ MAX_STATUS_LINE_CHARS + 1 ];
	char	buff2[ MAX_STATUS_LINE_CHARS + 30 ];

	if ( T_status_line != NULL )
	{
		for ( winno = start_winno; winno <= end_winno; winno++ )
		{
			win = Wininfo[ winno ];
			strncpy( buff, string, MAX_STATUS_LINE_CHARS );
			buff[ MAX_STATUS_LINE_CHARS ] = '\0';
			sprintf( buff2, buff, winno + 1 );
			strncpy( win->status_line, buff2,
				 MAX_STATUS_LINE_CHARS );
			win->status_line[ MAX_STATUS_LINE_CHARS ] = '\0';
			win->status_type_after_status_line = 0;
			if ( F_status_line_turns_status_on )
				win->status_on = 1;
			if ( F_status_line_clears_status_line_labels )
			{
				for ( i = 0; i < T_status_line_labelno; i++ )
				{
					win->status_line_label[ i ][ 0 ] = 0xFF;
					win->status_line_label[ i ][ 1 ] = '\0';
				}
			}
		}
		return( 0 );
	}
	return( -1 );
}
/****************************************************************************/
char *dec_encode();
/**************************************************************************
* extra_status_line
*	TERMINAL DESCRIPTION PARSER module for 'status_line'.
**************************************************************************/
/*ARGSUSED*/
extra_status_line( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "status_line" ) == 0 )
	{
		T_status_line = dec_encode( string );
		inst_status_line( T_status_line );
	}
	else if ( strcmp( buff, "status_on" ) == 0 )
	{
		T_status_on = dec_encode( string );
		inst_status_on( T_status_on );
	}
	else if ( strcmp( buff, "status_off" ) == 0 )
	{
		T_status_off = dec_encode( string );
		inst_status_off( T_status_off );
	}
	else if ( strcmp( buff, "status_type" ) == 0 )
	{
		if ( T_status_typeno < MAX_STATUS_TYPE )
		{
			T_status_type[ T_status_typeno ] =
				dec_encode( string );
			inst_status_type(
				T_status_type[ T_status_typeno ],
				T_status_typeno );
			T_status_typeno++;
		}
		else
		{
			printf( "Too many status_type\n" );
		}
	}
	else if ( strcmp( buff, "status_off_clears_status_type" ) == 0 )
	{
		F_status_off_clears_status_type =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_off_clears_status_line" ) == 0 )
	{
		F_status_off_clears_status_line =
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_type_turns_status_on" ) == 0 )
	{
		F_status_type_turns_status_on = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_type_ruins_status_line" ) == 0 )
	{
		F_status_type_ruins_status_line = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_line_turns_status_on" ) == 0 )
	{
		F_status_line_turns_status_on = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_line_clears_status_line_labels" ) == 0 )
	{
		F_status_line_clears_status_line_labels = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_off_clears_status_labels" ) == 0 )
	{
		F_status_off_clears_status_labels = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_off_clears_status_line_labels" ) == 0 )
	{
		F_status_off_clears_status_labels = 
				get_optional_value( string, 1 );
	}
	else if ( strcmp( buff, "status_line_label" ) == 0 )
	{
		if ( T_status_line_labelno < MAX_STATUS_LINE_LABEL )
		{
			T_status_line_label[ T_status_line_labelno ] =
				dec_encode( string );
			inst_status_line_label(
				T_status_line_label[ T_status_line_labelno ],
				T_status_line_labelno );
			T_status_line_labelno++;
		}
		else
		{
			printf( "Too many status_line_label\n" );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
