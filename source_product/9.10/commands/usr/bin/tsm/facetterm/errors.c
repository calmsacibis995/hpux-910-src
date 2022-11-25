/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: errors.c,v 70.1 92/03/09 15:41:52 ssa Exp $ */
#include <stdio.h>
#include "terminal.h"
#include <fcntl.h>
#include "facetpath.h"
char Ft_error_file[ 256 ] = "";
/**************************************************************************
* fsend_init_errors
*	Build the name of the error file for later use.
**************************************************************************/
fsend_init_errors()
{
	sprintf( Ft_error_file,"%s/errors/%s", Facettermpath, Facetterm );
}
/**************************************************************************
* error_record_char
*	The character "c" was not recognized by the terminal description
*	and is not believed to be a printable character.  Record it in
*	the error file.
**************************************************************************/
error_record_char( c )
	int	c;
{
	unsigned char	buff[ 2 ];

	buff[ 0 ] = c;
	buff[ 1 ] = '\0';
	error_record( buff, 1 );
}
/**************************************************************************
* error_record_msg
*	Record the string "buff" in the error file.
**************************************************************************/
error_record_msg( buff )
	char	*buff;
{
	error_record( (unsigned char *) buff, strlen( buff ) );
}
/**************************************************************************
* error_record_msg_raw
*	Record the string "buff" in the error file w/o translation.
**************************************************************************/
error_record_msg_raw( buff )
	char	*buff;
{
	int	fd;

	fd = open( Ft_error_file, O_WRONLY | O_APPEND | O_CREAT, 0666 );
	if ( fd < 0 )
		return;
	write( fd, buff, strlen( buff ) );
	chmod( Ft_error_file, 0666 );
	chown( Ft_error_file, 0, 1 );
	close( fd );
}
/**************************************************************************
* error_record
*	Record the string "buff" which has length "buffno" in the error
*	file.  Recording is done by append with possible create to a file
*	named $FACETTERM or $TERM in /usr/facetterm/errors.  If the file
*	cannot be created ( e.g. no such directory ), it is assumed that
*	the user does not want an errors file.
*	Characeters are recorded in a 'visual' format - ^X for Control-X
*	etc.  Newline is added at the end and every 256 characters to allow
*	editing.
**************************************************************************/
error_record( buff, buffno )
	unsigned char	*buff;
	int		buffno;
{
	char	out[ 512 ];
	char	*o;
	int	c;
	int	fd;
	int	i;
	int	len;

	fd = open( Ft_error_file, O_WRONLY | O_APPEND | O_CREAT, 0666 );
	if ( fd < 0 )
		return;
	chmod( Ft_error_file, 0666 );
	chown( Ft_error_file, 0, 1 );
	o = out;
	for ( i = 0; i < buffno; i++ )
	{
		c = buff[ i ];
		if ( c >= 0x80 )
		{
			if ( c <= 0x9F )
			{
				*o++ = '\\';
				c -= 0x40;
			}
			else
			{
				*o++ = '~';
				c &= 0x7F;
			}
		}
		if ( c == 0x7F )
		{
			*o++ = '^';
			*o++ = '?';
		}
		else if ( c < 0x20 )
		{
			if ( c == 0x1B )
			{
				*o++ = '\\';
				*o++ = 'E';
			}
			else
			{
				*o++ = '^';
				*o++ = ( c + '@' );
			}
		}
		else if ( c == '\\' )
		{
			*o++ = '\\';
			*o++ = '\\';
		}
		else if ( c == ' ' )
		{
			*o++ = '\\';
			*o++ = 's';
		}
		else
			*o++ = c;
		len = o - &out[ 0 ];
		if ( len > 256 )
		{
			*o++ = '\n';
			len = o - &out[ 0 ];
			write( fd, out, len );
			o = out;
		}
	}
	*o++ = '\n';
	len = o - &out[ 0 ];
	write( fd, out, len );
	close( fd );
}
