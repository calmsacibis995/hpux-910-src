/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: lineattr.c,v 70.1 92/03/09 15:45:05 ssa Exp $ */
/**************************************************************************
* lineattr.c
*	Capability to handle line attributes such as the vt220 where a
*	sequence can cause the characters to be double wide.
*	Double wide is assumed if a line attribute is on.
**************************************************************************/
#include <stdio.h>
#include "person.h"
#include "lineattr_p.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"
#include "ftchar.h"

char	*sT_line_attribute_off = { NULL };

#define MAX_LINE_ATTRIBUTE	10
int	sT_line_attribute_no = { 0 };		/* 0 is not valid */
char	*sT_line_attribute[ MAX_LINE_ATTRIBUTE ]={NULL};

/**************************************************************************
* dec_line_attribute_off
*	DECODE module for 'line_attribute_off'.
*	Turn line attributes off.
**************************************************************************/
dec_line_attribute_off()
{
	fct_line_attribute_off();
}
/**************************************************************************
* inst_line_attribute_off
*	INSTALL module for 'line_attribute_off'.
**************************************************************************/
inst_line_attribute_off( str )
	char	*str;
{
	dec_install( "line_attribute_off", (UNCHAR *) str,
					dec_line_attribute_off, 0, 0,
					(char *) 0 );
}
/**************************************************************************
* fct_line_attribute_off
*	ACTION module for 'line_attribute_off'.
**************************************************************************/
fct_line_attribute_off()
{
	set_row_changed_1( Outwin, Outwin->row );
	set_col_changed_all( Outwin );
	Outwin->line_attribute[ Outwin->row ] = 0;
	d_sync_line_attribute_current();
	if ( Outwin->onscreen )
		term_line_attribute_off();
}
/**************************************************************************
* term_line_attribute_off
*	TERMINAL OUTPUT module for 'line_attribute_off'.
**************************************************************************/
term_line_attribute_off()
{
	if ( mT_line_attribute_off != NULL )
		term_tputs( mT_line_attribute_off );
}
/**************************************************************************
* clear_line_attribute
*	Turn any possible line attributes off on screen rows
*	"top_row" to row "bot_row".
**************************************************************************/
clear_line_attribute( top_row, bot_row )
	int	top_row;
	int	bot_row;
{
	int 	i;

	if ( mT_line_attribute_off == NULL )
		return;
	if ( bot_row < top_row )
		return;
	term_pos( top_row, 0 );
	term_line_attribute_off();
	for ( i = top_row + 1; i <= bot_row; i++ )
	{
		term_linefeed();
		term_line_attribute_off();
	}
}
/**************************************************************************
* dec_line_attribute
*	DECODE module for 'line_attribute'.
*	Turn line attribute number "line_attribute_no" on.
**************************************************************************/
/*ARGSUSED*/
dec_line_attribute( line_attribute_no, parm_ptr )
	int	line_attribute_no;
	char	*parm_ptr;
{
	fct_line_attribute( line_attribute_no );
}
/**************************************************************************
* inst_line_attribute
*	INSTALL module for 'line_attribute'.
*	"line_attribute_no" is the line attribute number.
**************************************************************************/
inst_line_attribute( str, line_attribute_no )
	char	*str;
	int	line_attribute_no;
{
	dec_install( "line_attribute", (UNCHAR *) str, dec_line_attribute, 
		line_attribute_no, 0,
		(char *) 0 );
}
/**************************************************************************
* fct_line_attribute
*	ACTION module for 'line_attribute'.
*	"line_attribute_no" is the line attribute number.
**************************************************************************/
fct_line_attribute( line_attribute_no )
	int	line_attribute_no;
{
	set_row_changed_1( Outwin, Outwin->row );
	set_col_changed_all( Outwin );
	Outwin->line_attribute[ Outwin->row ] = line_attribute_no;
	d_sync_line_attribute_current();
	d_clr_eol( Outwin->row, (int) ( Outwin->col_right_line + 1 ),
				(FTCHAR) 0, (FTCHAR) ' ' );
	if ( Outwin->onscreen )
		term_line_attribute( line_attribute_no );
}
/**************************************************************************
* term_line_attribute
*	TERMINAL OUTPUT module for 'line_attribute'.
*	"line_attribute_no" is the line attribute number.
**************************************************************************/
term_line_attribute( line_attribute_no )
	int line_attribute_no;
{
	char	*line_attribute_str;

	if ( line_attribute_no < mT_line_attribute_no )
	{
		line_attribute_str = mT_line_attribute[ line_attribute_no ];
		if ( line_attribute_str != NULL )
			term_tputs( line_attribute_str );
	}
}

/****************************************************************************/
char *dec_encode();
/**************************************************************************
* extra_line_attribute
*	TERMINAL DESCRIPTION PARSER module for 'line_attribute_off' and
*					       'line_attribute'.
**************************************************************************/
/*ARGSUSED*/
extra_line_attribute( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;		/* not used */
	char	*attr_off_string;		/* not used */
{
	if ( strcmp( buff, "line_attribute_off" ) == 0 )
	{
		xT_line_attribute_off = dec_encode( string );
		xT_line_attribute[ 0 ] = xT_line_attribute_off;
		inst_line_attribute_off( xT_line_attribute_off );
	}
	else if ( strcmp( buff, "line_attribute" ) == 0 )
	{
		if ( xT_line_attribute_no < MAX_LINE_ATTRIBUTE )
		{
			if ( xT_line_attribute_no < 1 )
			{
				xT_line_attribute_no = 1;
			}
			xT_line_attribute[ xT_line_attribute_no ] =
				dec_encode( string );
			inst_line_attribute(
				xT_line_attribute[ xT_line_attribute_no ],
				xT_line_attribute_no );
			xT_line_attribute_no++;
		}
		else
		{
			printf( "Too many line_attribute\n" );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
