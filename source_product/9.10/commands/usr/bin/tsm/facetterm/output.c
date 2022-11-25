/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: output.c,v 70.1 92/03/09 15:45:55 ssa Exp $ */
#include <stdio.h>
#include "baud.h"
#include "decode.h"
#include "max_buff.h"
#include "modes.h"
typedef unsigned char UNCHAR;
UNCHAR Mytparm_out[ MAX_BUFF + 300 ];
/**************************************************************************
* my_tparm
*	Instantiate the string "string" with the decimal parameters
*	in "parm" and the string parameters in "string_parm".
**************************************************************************/
char *
my_tparm( string, parm, string_parm, winno )
	char	*string;	/* terminfo string to be instantiated */
	int	parm[];
	char	*string_parm[];
	int	winno;
{
	REG	UNCHAR	*pstring;
	REG	UNCHAR	c;

	UNCHAR	*out;
	UNCHAR	*start;

	int	stack[ 10 ];
	int	stackno;

	int	lead;
	int	width;
	int	width_columns_wide_mode;
	int	val1;
	int	val2;
	int	value;

	UNCHAR	cc;
	UNCHAR	*p;
	int	parmno;

	int	full;			/* %8s\r has 8 char string -ignore \r */

	UNCHAR	cout;

	int	final_parm;

	pstring = (UNCHAR *) string;
	out = Mytparm_out;
	stackno = 0;
	while ( 1 )
	{
		start = pstring;
		c = *pstring++;
		if (c == '\0')
		{
			*out = '\0';
			return( (char *) Mytparm_out );
		}
		else if (c == '%')
		{
			c = *pstring++;
			if ( c == '%' )
			{				/* %% */
				*out++ = '%';
				continue;
			}
			else if ( c == 'i' )
			{	 			/* incr row & col */
				parm[ 0 ]++;
				parm[ 1 ]++;
				continue;
			}
			else if ( c == 'p' )
			{
				c = *pstring++;
				switch( c )
				{
				case '1': stack[ stackno++ ] = parm[ 0 ]; 
								continue;
				case '2': stack[ stackno++ ] = parm[ 1 ]; 
								continue;
				case '3': stack[ stackno++ ] = parm[ 2 ]; 
								continue;
				case '4': stack[ stackno++ ] = parm[ 3 ]; 
								continue;
				case '5': stack[ stackno++ ] = parm[ 4 ]; 
								continue;
				case '6': stack[ stackno++ ] = parm[ 5 ]; 
								continue;
				case '7': stack[ stackno++ ] = parm[ 6 ]; 
								continue;
				case '8': stack[ stackno++ ] = parm[ 7 ]; 
								continue;
				case '9': stack[ stackno++ ] = parm[ 8 ]; 
								continue;
				case 'A': stack[ stackno++ ] = parm[ 9 ]; 
								continue;
				case 'B': stack[ stackno++ ] = parm[ 10 ]; 
								continue;
				case 'C': stack[ stackno++ ] = parm[ 11 ]; 
								continue;
				default: break;
				}
			}
			else if ( c == '\'' )
			{
				cc = *pstring++;
				if ( cc != '\0' )
				{
					c = *pstring++;
					if ( c == '\'' )
					{
						stack[ stackno++ ] = cc;
						continue;
					}
				}
			}
			else if ( c == '{' )	/* } makes showmatch happy */
			{
				value = 0;
				while (  ( (c = *pstring++) >= '0' )
				      && (  c               <= '9' ) )
				{
					value *= 10;
					value += ( c - '0' );
				}
						/* { makes showmatch happy */
				if ( c == '}' )
				{
					stack[ stackno++ ] = value;
					continue;
				}
			}
			else if ( c == '+' )
			{
				if ( stackno >= 2 )
				{
					val1 = stack[ --stackno ];
					val2 = stack[ --stackno ];
					stack[ stackno++ ] = val1 + val2;
					continue;
				}
				else
					stackno = 0;
			}
			else if ( c == '-' )
			{
				if ( stackno >= 2 )
				{
					val1 = stack[ --stackno ];
					val2 = stack[ --stackno ];
					stack[ stackno++ ] = val1 - val2;
					continue;
				}
				else
					stackno = 0;
			}
			else if ( c == 'c' )
			{
				if ( stackno >= 1 )
				{
					cout = stack[ --stackno ];
					if ( cout == 0 )
						*out++ = 0x80;
					else
						*out++ = cout;
					continue;
				}
			}
			else if (  c == 'd'
			        || c == 'X'
			        || c == 's'
			        || c == 'u'
				|| c == 'H'
				|| c == 'b' || c == 'B'
				|| c == 'l'
				|| c == 'h'
				|| c == 'L'
			        || c == 'S'
			        || c == 'f'
				|| ( (c >= '0') && (c <= '9') ) )
			{
				width = 0;
				lead = 0;
				if ( c == '0' )
				{
					lead = 1;
					c = *pstring++;
				}
				while ( (c >= '0') && (c <= '9') )
				{
					width = (width * 10 ) + (c - '0');
					c = *pstring++;
				}
				width_columns_wide_mode = width;
				if ( c == '-' )
				{
					c = *pstring++;
					width_columns_wide_mode = 0;
					while ( (c >= '0') && (c <= '9') )
					{
					 width_columns_wide_mode *= 10;
					 width_columns_wide_mode += ( c - '0' );
					 c = *pstring++;
					}
				}
				if ( ( c == 'd' ) && ( stackno >= 1 ) )
				{
					value = stack[ --stackno ];
					outdec( &out, value, width, lead );
					continue;
				}
				else if ( ( c == 'X' ) && ( stackno >= 1 ) )
				{
					value = stack[ --stackno ];
					outhex( &out, value, width, lead );
					continue;
				}
				else if ( c == 'l' )
				{
					c = *pstring++;
					parmno = -1;
					if ( c >= '1' && c <= '9' )
						parmno = c - '1';
					else if ( c >= 'A' && c <= 'C' )
						parmno = 9 + c - 'A';
					if ( parmno >= 0 )
					{
						value = strlen( 
							string_parm[ parmno ] );
						outdec( &out, value, width,
							lead );
						continue;
					}
				}
				else if ( c == 'h' )
				{
					c = *pstring++;
					parmno = -1;
					if ( c >= '1' && c <= '9' )
						parmno = c - '1';
					else if ( c >= 'A' && c <= 'C' )
						parmno = 9 + c - 'A';
					if ( parmno >= 0 )
					{
						value = strlen( 
							string_parm[ parmno ] );
						outhex( &out, value, width,
							lead );
						continue;
					}
				}
				else if ( c == 'L' )
				{
					c = *pstring++;
					switch( c )
					{
					case '1': outstr( &out, 
					    string_parm[ 0 ], width ); continue;
					case '2': outstr( &out, 
					    string_parm[ 1 ], width ); continue;
					case '3': outstr( &out, 
					    string_parm[ 2 ], width ); continue;
					case '4': outstr( &out, 
					    string_parm[ 3 ], width ); continue;
					case '5': outstr( &out, 
					    string_parm[ 4 ], width ); continue;
					case '6': outstr( &out, 
					    string_parm[ 5 ], width ); continue;
					case '7': outstr( &out, 
					    string_parm[ 6 ], width ); continue;
					case '8': outstr( &out, 
					    string_parm[ 7 ], width ); continue;
					case '9': outstr( &out, 
					    string_parm[ 8 ], width ); continue;
					case 'A': outstr( &out, 
					    string_parm[ 9 ], width ); continue;
					case 'B': outstr( &out, 
					   string_parm[ 10 ], width ); continue;
					case 'C': outstr( &out, 
					   string_parm[ 11 ], width ); continue;
					}
				}
				else if ( c == 's' )
				{
					int	w;

					if ( winno < 0 )
					{
					    if ( M_columns_wide_on )
						w = width_columns_wide_mode;
					    else
						w = width;
					}
					else
					{
					    if ( is_columns_wide_mode_on_winno(
									winno ))
						w = width_columns_wide_mode;
					    else
						w = width;
					}
					c = *pstring++;
					switch( c )
					{
					case '1': full = outstr( &out, 
						    string_parm[ 0 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '2': full = outstr( &out, 
						    string_parm[ 1 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '3': full = outstr( &out, 
						    string_parm[ 2 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '4': full = outstr( &out, 
						    string_parm[ 3 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '5': full = outstr( &out, 
						    string_parm[ 4 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '6': full = outstr( &out, 
						    string_parm[ 5 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '7': full = outstr( &out, 
						    string_parm[ 6 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '8': full = outstr( &out, 
						    string_parm[ 7 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case '9': full = outstr( &out, 
						    string_parm[ 8 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case 'A': full = outstr( &out, 
						    string_parm[ 9 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case 'B': full = outstr( &out, 
						    string_parm[ 10 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					case 'C': full = outstr( &out, 
						    string_parm[ 11 ], w ); 
						if ( full )
							c = *pstring++;
						continue;
					}
				}
				else if ( c == 'u' )
				{
					c = *pstring++;
					/**********************************
					* Skip the until length.
					**********************************/
					pstring++;
					switch( c )
					{
					case '1': outstr( &out, 
						    string_parm[ 0 ], 0 ); 
						continue;
					case '2': outstr( &out, 
						    string_parm[ 1 ], 0 ); 
						continue;
					case '3': outstr( &out, 
						    string_parm[ 2 ], 0 ); 
						continue;
					case '4': outstr( &out, 
						    string_parm[ 3 ], 0 ); 
						continue;
					case '5': outstr( &out, 
						    string_parm[ 4 ], 0 ); 
						continue;
					case '6': outstr( &out, 
						    string_parm[ 5 ], 0 ); 
						continue;
					case '7': outstr( &out, 
						    string_parm[ 6 ], 0 ); 
						continue;
					case '8': outstr( &out, 
						    string_parm[ 7 ], 0 ); 
						continue;
					case '9': outstr( &out, 
						    string_parm[ 8 ], 0 ); 
						continue;
					case 'A': outstr( &out, 
						    string_parm[ 9 ], 0 ); 
						continue;
					case 'B': outstr( &out, 
						    string_parm[ 10 ], 0 ); 
						continue;
					case 'C': outstr( &out, 
						    string_parm[ 11 ], 0 ); 
						continue;
					}
				}
				else if ( c == 'H' )
				{
					c = *pstring++;
					switch( c )
					{
					case '1': outstr( &out, 
						    string_parm[ 0 ], 0 ); 
						continue;
					case '2': outstr( &out, 
						    string_parm[ 1 ], 0 ); 
						continue;
					case '3': outstr( &out, 
						    string_parm[ 2 ], 0 ); 
						continue;
					case '4': outstr( &out, 
						    string_parm[ 3 ], 0 ); 
						continue;
					case '5': outstr( &out, 
						    string_parm[ 4 ], 0 ); 
						continue;
					case '6': outstr( &out, 
						    string_parm[ 5 ], 0 ); 
						continue;
					case '7': outstr( &out, 
						    string_parm[ 6 ], 0 ); 
						continue;
					case '8': outstr( &out, 
						    string_parm[ 7 ], 0 ); 
						continue;
					case '9': outstr( &out, 
						    string_parm[ 8 ], 0 ); 
						continue;
					case 'A': outstr( &out, 
						    string_parm[ 9 ], 0 ); 
						continue;
					case 'B': outstr( &out, 
						    string_parm[ 10 ], 0 ); 
						continue;
					case 'C': outstr( &out, 
						    string_parm[ 11 ], 0 ); 
						continue;
					}
				}
				else if ( ( c == 'b' ) || ( c == 'B' ) )
				{
					if ( c == 'B' )
						final_parm = 1;
					else
						final_parm = 0;
					c = *pstring++;
					switch( c )
					{
					case '1': outibmdec( &out, 
						    parm[ 0 ], final_parm ); 
						continue;
					case '2': outibmdec( &out, 
						    parm[ 1 ], final_parm ); 
						continue;
					case '3': outibmdec( &out, 
						    parm[ 2 ], final_parm ); 
						continue;
					case '4': outibmdec( &out, 
						    parm[ 3 ], final_parm ); 
						continue;
					case '5': outibmdec( &out, 
						    parm[ 4 ], final_parm ); 
						continue;
					case '6': outibmdec( &out, 
						    parm[ 5 ], final_parm ); 
						continue;
					case '7': outibmdec( &out, 
						    parm[ 6 ], final_parm ); 
						continue;
					case '8': outibmdec( &out, 
						    parm[ 7 ], final_parm ); 
						continue;
					case '9': outibmdec( &out, 
						    parm[ 8 ], final_parm ); 
						continue;
					case 'A': outibmdec( &out, 
						    parm[ 9 ], final_parm ); 
						continue;
					case 'B': outibmdec( &out, 
						    parm[ 10 ], final_parm ); 
						continue;
					case 'C': outibmdec( &out, 
						    parm[ 11 ], final_parm ); 
						continue;
					}
				}
				else if ( c == 'I' )
				{
					c = *pstring++;
					switch( c )
					{
					case '1': outibmstr( &out, 
						    string_parm[ 0 ], width ); 
						continue;
					case '2': outibmstr( &out, 
						    string_parm[ 1 ], width ); 
						continue;
					case '3': outibmstr( &out, 
						    string_parm[ 2 ], width ); 
						continue;
					case '4': outibmstr( &out, 
						    string_parm[ 3 ], width ); 
						continue;
					case '5': outibmstr( &out, 
						    string_parm[ 4 ], width ); 
						continue;
					case '6': outibmstr( &out, 
						    string_parm[ 5 ], width ); 
						continue;
					case '7': outibmstr( &out, 
						    string_parm[ 6 ], width ); 
						continue;
					case '8': outibmstr( &out, 
						    string_parm[ 7 ], width ); 
						continue;
					case '9': outibmstr( &out, 
						    string_parm[ 8 ], width ); 
						continue;
					case 'A': outibmstr( &out, 
						    string_parm[ 9 ], width ); 
						continue;
					case 'B': outibmstr( &out, 
						    string_parm[ 10 ], width ); 
						continue;
					case 'C': outibmstr( &out, 
						    string_parm[ 11 ], width ); 
						continue;
					}
				}
				else if ( c == 'f' )
				{
					c = *pstring++;
					parmno = -1;
					if ( c >= '1' && c <= '9' )
						parmno = c - '1';
					else if ( c >= 'A' && c <= 'C' )
						parmno = 9 + c - 'A';
					if ( parmno >= 0 )
					{
						outstrfixed( &out,
							string_parm[ parmno ],
							width );
						continue;
					}
				}
				else if ( c == 'S' )
				{
					c = *pstring++;
					parmno = -1;
					if ( c >= '1' && c <= '9' )
						parmno = c - '1';
					else if ( c >= 'A' && c <= 'C' )
						parmno = 9 + c - 'A';
					if ( parmno >= 0 )
					{
						*out++ = parm[
						   POS_FUNCTION_KEY_DELIMITER ];
						outstr( &out,
							string_parm[ parmno ],
							width );
						*out++ = parm[
						   POS_FUNCTION_KEY_DELIMITER ];
						continue;
					}
				}
			}
		}
		for ( p = start; p < pstring; p++ )
		{
			if ( *p == '\0' )
			{
				*out = '\0';
				return( (char *) Mytparm_out );
			}
			*out++ = *p;
		}
	}
}
/**************************************************************************
* nopad_strncpy
*	Copy the string "string" to the string "outstring" removing any
*	padding specifications that may be present.
*	NULL pad outstring to the number of characters "max".
**************************************************************************/
nopad_strncpy( outstring, string, max )
	char	*outstring;
	char	*string;
	int	max;
{
	UNCHAR	*pstring;
	UNCHAR	*out;
	UNCHAR	*try;
	UNCHAR	c;
	UNCHAR	cc;
	int	i;

	pstring = (UNCHAR *) string;
	out = (UNCHAR *) outstring;
	for( i = 0; i < max; i++ )
	{
		c = *pstring++;
		if ( c == '\0' )
			break;
		if ( c == '$' ) 
		{					/* ignore padding */
			try = pstring;
			cc = *try++;
			if ( cc == '<' )
			{
				while(  ((cc = *try++) >= '0' )
				     && ( cc               <= '9' ) )
				{
				}
				if ( cc == '.' )
				{
					cc = *try++;
					if(  ( cc >= '0' ) && ( cc <= '9' ) )
						cc = *try++;
				}
				if ( cc == '*' )
					cc = *try++;
				if ( cc == '/' )
					cc = *try++;
				if ( cc == '>' )
				{
					pstring = try;
					if ( *pstring == '\0' )
						break;
					continue;
				}
			}
		}
		*out++ = c;
	}
	if ( i < max )
		*out++ = '\0';
	outstring[ max - 1 ] = '\0';
}
/**************************************************************************
* my_tputs
*	Output the string "str" turning 0x80 to NULLS and calculating padding
*	based on the number of affected units "affcnt".
**************************************************************************/
my_tputs( str, affcnt )
	char	*str;
	int	affcnt;
{
	REG	UNCHAR	*pstring;
	REG	UNCHAR	c;
	REG	UNCHAR	*start;

	UNCHAR	*p;
	int	pad;
	int	padtenths;
	int	perline;
	int	mandatory;

	pstring = (UNCHAR *) str;
	while ( 1 )
	{
		start = pstring;
		c = *pstring++;
		if ( c == '\0' )
			break;
		else if ( c == '$' ) 
		{					/* padding */
			pad = 0;
			padtenths = 0;
			perline = 0;
			mandatory = 0;
			c = *pstring++;
			if ( c == '<' )
			{
				while(  ((c = *pstring++) >= '0' )
				     && ( c               <= '9' ) )
				{
					pad = (pad * 10) + (c - '0');
				}
				if ( c == '.' )
				{
					c = *pstring++;
					if(  ( c >= '0' )
					  && ( c <= '9' ) )
					{
						padtenths = c - '0';
						c = *pstring++;
					}
				}
				if ( c == '*' )
				{
					perline = 1;
					c = *pstring++;
				}
				if ( c == '/' )
				{
					mandatory = 1;
					c = *pstring++;
				}
				if ( c == '>' )
				{
					outpad( affcnt, pad, padtenths,
						perline, mandatory );
					continue;
				}
			}
		}
		else if ( c == 0x80 )
		{
			putchar( '\0' );
			continue;
		}
		else
		{
			putchar( c );
			continue;
		}
		for ( p = start; p < pstring; p++ )
		{
			if ( *p == '\0' )
				return;
			putchar( *p );
		}
	}
}
/**************************************************************************
* outpad
*	Output padding of "pad" milliseconds and "padtenths" tenths of
*	milliseconds.
*	The number of affected lines is "affcnt".
*	"perline" indicates the padding is per "affcnt".
*	"mandatory" indicates the padding should be output even in xon xoff
*	is being used.
**************************************************************************/
outpad( affcnt, pad, padtenths, perline, mandatory )
	int	affcnt;
	int	pad;
	int	padtenths;
	int	perline;
	int	mandatory;
{
	int	padchars;
	int	i;
	int	shifted_back;

	if ( pad || padtenths )
	{
		if ( mandatory || ( Ft_xon_xoff == 0 ) )
		{
			if ( perline && ( affcnt > 0 ) )
				pad *= affcnt;
			if ( padtenths )
			{
				if ( perline && ( affcnt > 0 ) )
				{
					padtenths *= affcnt;
					pad += ( padtenths / 10 );
				}
				pad++;
			}
			if ( Ft_baud > 0 )
			{
				padchars = pad >> Ft_baud;
				shifted_back = padchars << Ft_baud;
			}
			else if ( Ft_baud < 0 )
			{
				padchars = pad << ( - Ft_baud );
				shifted_back = padchars >> ( - Ft_baud );
			}
			else
			{
				padchars = pad;
				shifted_back = padchars;
			}
			if ( shifted_back != pad )
				padchars++;
			for ( i = 0; i < padchars; i++ )
				putchar( '\0' );
		}
	}
}
/**************************************************************************
* outdec
*	Store a decimal representation of "value" in the buffer.
*	"pout" is a pointer to a pointer to the proper place in the buffer.
*	Store the characters and update the pointer as well.
*	Posible modifiers are:
*	"lead" for leading zeros
*	and "width" for minimum width.
**************************************************************************/
outdec( pout, value, width, lead )
	UNCHAR	**pout;
	int	value;
	int	width;
	int	lead;
{
	REG	char	c;
	REG	char	*p;

	char	buff[ 100 ];

	if ( lead )
	{
		sprintf( buff, "%0*d", width, value );
	}
	else if ( width )
	{
		sprintf( buff, "%*d", width, value );
	}
	else
	{
		sprintf( buff, "%d", value );
	}
	p = buff;
	while( (c = *p++) != '\0' )
	{
		*(*pout)++ = c;
	}
}
/**************************************************************************
* outhex
*	Store a hex representation of "value" in the buffer.
*	"pout" is a pointer to a pointer to the proper place in the buffer.
*	Store the characters and update the pointer as well.
*	Posible modifiers are:
*	"lead" for leading zeros
*	and "width" for minimum width.
**************************************************************************/
outhex( pout, value, width, lead )
	UNCHAR	**pout;
	int	value;
	int	width;
	int	lead;
{
	REG	char	c;
	REG	char	*p;

	char	buff[ 100 ];

	if ( lead )
	{
		sprintf( buff, "%0*X", width, value );
	}
	else if ( width )
	{
		sprintf( buff, "%*X", width, value );
	}
	else
	{
		sprintf( buff, "%X", value );
	}
	p = buff;
	while( (c = *p++) != '\0' )
	{
		*(*pout)++ = c;
	}
}
/**************************************************************************
* outstr
*	Store the string "string" in the buffer.
*	"pout" is a pointer to a pointer to the proper place in the buffer.
*	Store the characters and update the pointer as well.
*	Posible modifiers are:
*	"width" which if > 0 indicates a maximum width.
*	Return 1 if the maximum was hit.
*	Return 0 otherwise
**************************************************************************/
outstr( pout, string, width )
	UNCHAR	**pout;
	char	*string;
	int	width;
{
	REG	UNCHAR	c;
	REG	int	i;
	UNCHAR	*s;

	s = (UNCHAR *) string;
	if ( width == 0 )
		width = MAX_BUFF;
	i = 0;
	while ( (c = *s++) != '\0' )
	{
		*(*pout)++ = c;
		if ( ++i >= width )
			return( 1 );
	}
	return( 0 );
}
/**************************************************************************
* outstrfixed
*	Store the string "string" in the buffer.
*	"pout" is a pointer to a pointer to the proper place in the buffer.
*	Store the characters and update the pointer as well.
*	Limit or pad the string to exactly "width" characters.
**************************************************************************/
outstrfixed( pout, string, width )
	UNCHAR	**pout;
	char	*string;
	int	width;
{
	REG	UNCHAR	c;
	REG	int	i;
	UNCHAR	*s;

	s = (UNCHAR *) string;
	i = 0;
	while ( (c = *s++) != '\0' )
	{
		*(*pout)++ = c;
		i++;
		if ( i >= width )
			return;
	}
	for( ; i < width; i++ )
		*(*pout)++ = ' ';
}
/**************************************************************************
* outibmdec
*	Output a two byte ibm representation of "value" in the buffer.
*	5 bits in the bottom of each byte.
*	"pout" is a pointer to a pointer to the proper place in the buffer.
*	Store the characters and update the pointer as well.
*	Posible modifiers are:
*	"final_parm" - output 0x40 on end byte.
**************************************************************************/
outibmdec( pout, value, final_parm )
	UNCHAR	**pout;
	int	value;
	int	final_parm;
{
	int	c;

	c = ( ( value >> 5 ) & 0x1F ) | 0x20;
		*(*pout)++ = c;
	if ( final_parm )
		c = ( value & 0x1F ) | 0x40;
	else
		c = ( value & 0x1F ) | 0x20;
	*(*pout)++ = c;
}
/**************************************************************************
* outibmstr
*	Store the string "string" in the buffer.
*	"pout" is a pointer to a pointer to the proper place in the buffer.
*	Store the characters and update the pointer as well.
*	Posible modifiers are:
*	"width" which if > 0 indicates a maximum width.
*	Return 1 if the maximum was hit.
*	Return 0 otherwise
**************************************************************************/
outibmstr( pout, string, width )
	UNCHAR	**pout;
	char	*string;
	int	width;
{
	REG	UNCHAR	c;
	REG	int	i;
	UNCHAR	*s;

	s = (UNCHAR *) string;
	if ( width == 0 )
		width = MAX_BUFF;
	i = 0;
	while ( (c = *s) != '\0' )
	{
		s++;
		/* 0x60 = 0x60 is 3151 operation specifier*/
		if ( *s == '\0' )
		{
			if ( ( c & 0x60 ) != 0x60 )
				*(*pout)++ = ( c & 0x1F ) | 0x40;
			else
				*(*pout)++ = ( c & 0x7F );
			return( 0 );
		}
		if ( ++i >= width )
		{
			if ( ( c & 0x60 ) != 0x60 )
				*(*pout)++ = ( c & 0x1F ) | 0x40;
			else
				*(*pout)++ = ( c & 0x7F );
			return( 1 );
		}
		*(*pout)++ = ( c & 0x1F ) | 0x20;
	}
	return( 0 );
}
