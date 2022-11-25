/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: box.c,v 70.1 92/03/09 15:40:33 ssa Exp $ */
#include <stdio.h>
#include "ftchar.h"
#include "ftproc.h"
#include "ftwindow.h"
#include "features.h"
#include "decode.h"

#include "ftterm.h"
#define MAX_DRAW_BOX 5
int	T_draw_box_no = 0;
int	Draw_box_width_height[ MAX_DRAW_BOX ] = { 0 };
char	*T_draw_box[ MAX_DRAW_BOX ] = { NULL };
FTCHAR	Draw_box_chars[ MAX_DRAW_BOX ][ 6 ] = { 0 };

/**************************************************************************
* dec_draw_box
*	DECODE module for 'draw_box'.
**************************************************************************/
/*ARGSUSED*/
dec_draw_box( draw_box_no, parm_ptr, parms_valid, parm )
	int	draw_box_no;
	char	*parm_ptr;
	int	parms_valid;
	int	parm[];
{
	int     row;
	int     col;

	if ( parms_valid & 1 )
		row = parm[ 0 ];
	else
		row = 0;
	if ( parms_valid & 2 )
		col = parm[ 1 ];
	else
		col = 0;
	fct_draw_box( draw_box_no, row, col );
}
/**************************************************************************
* inst_draw_box
*	INSTALL module for 'draw_box'.
**************************************************************************/
inst_draw_box( str, draw_box_no )
	char	*str;
	int	draw_box_no;
{
	dec_install( "draw_box", (UNCHAR *) str, 
			dec_draw_box, draw_box_no, CURSOR_OPTION,
			(char *) 0 );
}
/**************************************************************************
* fct_draw_box
*	ACTION module for 'draw_box'.
**************************************************************************/
fct_draw_box( draw_box_no, in_row, in_col )
	int	draw_box_no;
	int	in_row;
	int	in_col;
{
	int	top_row;
	int	bot_row;
	int	left_col;
	int	right_col;
	int	row;
	int	col;

	if ( Draw_box_width_height[ draw_box_no ] )
	{
		top_row = Outwin->row;
		left_col = Outwin->col;
		bot_row = Outwin->row + in_row;
		right_col = Outwin->col + in_col;
	}
	else
	{
		if ( in_row <= Outwin->row )
		{
			top_row = in_row;
			bot_row = Outwin->row;
		}
		else
		{
			top_row = Outwin->row;
			bot_row = in_row;
		}
		if ( in_col <= Outwin->col )
		{
			left_col = in_col;
			right_col = Outwin->col;
		}
		else
		{
			left_col = Outwin->col;
			right_col = in_col;
		}
	}
	if ( top_row < 0 )
		top_row = 0;
	if ( left_col < 0 )
		left_col = 0;
	for ( row = top_row + 1; row < bot_row; row++ )
	{
		if ( row <= Outwin->display_row_bottom )
		{
			Outwin->ftchars[ row ][ left_col ] = 
					Draw_box_chars[ draw_box_no][ 3 ];
			if ( right_col <= Outwin->col_right )
			{
				Outwin->ftchars[ row ][ right_col ] = 
					Draw_box_chars[ draw_box_no][ 3 ];
			}
		}
	}
	for ( col = left_col + 1; col < right_col; col++ )
	{
		if ( col <= Outwin->col_right )
		{
			Outwin->ftchars[ top_row ][ col ] = 
					Draw_box_chars[ draw_box_no][ 1 ];
			if ( bot_row <= Outwin->display_row_bottom )
			{
				Outwin->ftchars[ bot_row ][ col ] = 
					Draw_box_chars[ draw_box_no][ 1 ];
			}
		}
	}
	if ( left_col < right_col )
	{
		Outwin->ftchars[ top_row ][ left_col ] = 
					Draw_box_chars[ draw_box_no][ 0 ];
		if ( right_col <= Outwin->col_right )
		{
			Outwin->ftchars[ top_row ][ right_col ] = 
					Draw_box_chars[ draw_box_no][ 2 ];
			if ( bot_row <= Outwin->display_row_bottom )
			{
				Outwin->ftchars[ bot_row ][ right_col ] = 
					Draw_box_chars[ draw_box_no][ 4 ];
			}
		}
		if ( bot_row <= Outwin->display_row_bottom )
		{
			Outwin->ftchars[ bot_row ][ left_col ] = 
					Draw_box_chars[ draw_box_no][ 5 ];
		}
	}
	else
	{
		if ( right_col <= Outwin->col_right )
		{
			Outwin->ftchars[ top_row ][ right_col ] = 
					Draw_box_chars[ draw_box_no][ 3 ];
			if ( bot_row <= Outwin->display_row_bottom )
			{
				Outwin->ftchars[ bot_row ][ right_col ] = 
					Draw_box_chars[ draw_box_no][ 3 ];
			}
		}
	}
	if ( top_row == bot_row ) 
	{
		if ( bot_row <= Outwin->display_row_bottom )
		{
			Outwin->ftchars[ bot_row ][ left_col ] = 
					Draw_box_chars[ draw_box_no][ 1 ];
			if ( right_col <= Outwin->col_right )
			{
				Outwin->ftchars[ bot_row ][ right_col ] = 
					Draw_box_chars[ draw_box_no][ 1 ];
			}
		}
	}
	if ( Outwin->onscreen )
		term_draw_box( draw_box_no, in_row, in_col );
}
/**************************************************************************
* term_draw_box
*	TERMINAL OUTPUT module for 'draw_box'.
**************************************************************************/
term_draw_box( draw_box_no, row, col )
	int	draw_box_no;
	int	row;
	int	col;
{
	int	affcnt;			/* needs to be passed in ??? */
	char	*draw_box_str;
	int	parm[ 2 ];
	char	*string_parm[ 1 ];		/* not used */
	char	*my_tparm();
	char	*p;

	affcnt = Rows;
	if ( draw_box_no < T_draw_box_no )
	{
		draw_box_str = T_draw_box[ draw_box_no ];
		if ( draw_box_str != NULL )
		{
			parm[ 0 ] = row;
			parm[ 1 ] = col;
			p = my_tparm( draw_box_str, parm, string_parm, -1 );
			my_tputs( p, affcnt );
		}
	}
}
/**************************************************************************
* extra_draw_box
*	TERMINAL DESCRIPTION PARSER module for 'draw_box'.
**************************************************************************/
char *dec_encode();
extra_draw_box( buff, string, attr_on_string, attr_off_string ) 
	char	*buff;
	char	*string;
	char	*attr_on_string;
	char	*attr_off_string;
{
	char	*encoded;

	if (  ( strcmp( buff, "draw_box" ) == 0 )
	   || ( strcmp( buff, "draw_box_width_height" ) == 0 ) )
	{
		if ( T_draw_box_no < MAX_DRAW_BOX )
		{
			if ( strcmp( buff, "draw_box_width_height" ) == 0 )
				Draw_box_width_height[ T_draw_box_no ] = 1;
			else
				Draw_box_width_height[ T_draw_box_no ] = 0;
			if ( attr_off_string != NULL )
			{
				encoded = dec_encode( string );
				T_draw_box[ T_draw_box_no ] = encoded;
				inst_draw_box( encoded, T_draw_box_no );
				draw_box_encode( T_draw_box_no, 
							    attr_off_string );
				T_draw_box_no++;
			}
			else
				printf( "Missing attributes on draw_box.\n" );
		}
		else
		{
			printf( "Too many draw_box. Max = %d\n", 
				MAX_DRAW_BOX );
		}
	}
	else
	{
		return( 0 );		/* no match */
	}
	return( 1 );
}
/**************************************************************************
* draw_box_encode
*	Encode the character string "string" which contains 6 dash-separated
*	pairs of character-attribute for the 6 parts of the box starting
*	in the upper-right and going clockwise.
*	Store in the array "Draw_box_chars".
**************************************************************************/
draw_box_encode( draw_box_no, string )
	int	draw_box_no;
	char	*string;
{
	char	*p;
	char	*next;
	char	*break_dash_string();
	FTCHAR	c;
	FTCHAR	attribute_encode();
	int	i;
	char	*dec_encode();
	char	*encoded;

	if ( string != NULL )
	{
		p = string;
		for ( i = 0; i < 6; i++ )
		{
			next = break_dash_string( p );
			encoded = dec_encode( p );
			c = *encoded & 0x00FF;
			p = next;
			if ( p == (char *) 0 )
				break;
			next = break_dash_string( p );
			Draw_box_chars[ draw_box_no ][ i ] = 
						attribute_encode( p ) | c;
			p = next;
			if ( p == (char *) 0 )
				break;
		}
	}
}
