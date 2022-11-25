/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: paste.c,v 66.6 90/10/18 09:25:37 kb Exp $ */
#include <stdio.h>
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "paste.h"

int	T_paste_eol_no = 0;
char	*T_paste_eol[ MAX_PASTE_EOL ]={NULL};
char	T_paste_eol_name[ MAX_PASTE_EOL ][ MAX_PASTE_EOL_NAME_LEN + 1 ] ={'\0'};

char *dec_encode();
/**************************************************************************
* extra_paste_eol
*	TERMINAL DESCRIPTION PARSER module for 'paste_eol'.
*	The terminal description file can contain lines of the form:
*		paste_eol-name=string;
*	By typing 
*		^W e name \r
*	the user can set the paste eol type to 'string'.
*	This modifies the behavior of the pastes that put a newline at the
*	end of the lines.
**************************************************************************/
/*ARGSUSED*/
extra_paste_eol( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "paste_eol" ) == 0 )
	{
		if ( T_paste_eol_no < MAX_PASTE_EOL )
		{
			strncpy( T_paste_eol_name[ T_paste_eol_no ],
				 attr_on_string, MAX_PASTE_EOL_NAME_LEN );
			T_paste_eol_name[ T_paste_eol_no]
					[ MAX_PASTE_EOL_NAME_LEN ] = '\0';

			T_paste_eol[ T_paste_eol_no ] =
				dec_encode( string );
			T_paste_eol_no++;
		}
		else
		{
			printf( "Too many paste_eol\n" );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
