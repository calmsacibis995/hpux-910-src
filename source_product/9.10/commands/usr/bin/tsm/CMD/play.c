/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: play.c,v 70.1 92/03/09 15:38:45 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/******************************************************************
* NAME
*	Play - Output a FACET/TERM capture file to a terminal,
*              pausing before each escape sequence for a keystoke.
* SYNOPSIS
*	Play [ -v [ -s ] ] file
* DESCRIPTION
*	-v indicates that the file is in "visible" format ( as
*	   output by the program "visible" ).
*	-s indicates that the "visible" file should be simultaneously
*          output to stderr.
*	file is the capture file to be played.
******************************************************************/
#include <sys/types.h>
#include <stdio.h>
#include <termio.h>
#include <fcntl.h>
#include <signal.h>

int Input_is_visible = 0;
int Visible_to_stderr = 0;

extern int		quit();

main( argc, argv )
	int	argc;
	char	**argv;
{
	unsigned char	c;
	int		fd;
	unsigned char	cc;

	if ( argc > 1 )
	{
		if ( strcmp( argv[ 1 ], "-v" ) == 0 )
		{
			Input_is_visible = 1;
			argc--;
			argv++;
		}
	}
	if ( Input_is_visible && ( argc > 1 ) )
	{
		if ( strcmp( argv[ 1 ], "-s" ) == 0 )
		{
			Visible_to_stderr = 1;
			argc--;
			argv++;
		}
	}
	if ( argc <= 1 )
	{
		printf( "Usage: %s [-v [-s]] file\n", argv[ 0 ] );
		exit( 1 );
	}
	if ( ( fd = open( argv[ 1 ], O_RDONLY )) < 1 )
	{
		perror( argv[ 1 ] );
		exit( 1 );
	}
	printf( "Press q to quit\r\n" );
	if ( get_cooked() < 0 )
		exit( 1 );
	if ( get_raw() < 0 )
		exit( 1 );
	signal( SIGINT, quit );
	signal( SIGTERM, quit );
	signal( SIGHUP, quit );
	if ( set_raw() < 0 )
		exit( 1 );
	while( 1 )
	{
		if ( Input_is_visible )
		{
			if ( read_visible_char( fd, &c ) < 1 )
				break;
		}
		else
		{
			if ( read( fd, &c, 1 ) < 1 )
				break;
		}
		if ( c == 0x1b )
		{
			if ( read( 0, &c, 1 ) < 1 )
				break;
			if ( c == 'Q' )
				break;
			if ( c == 'q' )
				break;
			write( 1, "\033", 1 );
		}
		else
			write( 1, &c, 1 );
	}
	set_cooked();
	exit( 0 );
}
quit()
{
	set_cooked();
	exit( 0 );
}

struct termio T_cooked;
struct termio T_raw;

get_cooked()
{
	if ( ioctl( 0, TCGETA, &T_cooked ) < 0 )
	{
		perror( "get_cooked");
		return( -1 );
	}
	return( 0 );
}
get_raw()
{
	if ( ioctl( 0, TCGETA, &T_raw ) < 0 )
	{
		perror( "get_raw");
		return( -1 );
	}
	T_raw = T_cooked;
	T_raw.c_iflag |=  ( 0 );
	T_raw.c_iflag &= ~( IGNBRK | BRKINT | PARMRK |
				INLCR | IGNCR | ICRNL | IUCLC | IXANY | IXOFF );
	T_raw.c_oflag |=  ( 0 );
	T_raw.c_oflag &= ~( OPOST | OLCUC | ONLCR | OCRNL | ONOCR | ONLRET
			     | OFILL | OFDEL | TAB3 );
	T_raw.c_lflag |=  ( 0 );
	T_raw.c_lflag &= ~( ICANON | XCASE | ECHO | ECHOE | ECHOK );
	T_raw.c_cc[ VMIN ] = 1;
	T_raw.c_cc[ VTIME ] = 1;
	return( 0 );
}
set_cooked()
{
	if ( ioctl( 0, TCSETA, &T_cooked ) < 0 )
	{
		perror( "set_cooked");
		return( -1 );
	}
	return( 0 );
}
set_raw()
{
	if ( ioctl( 0, TCSETA, &T_raw ) < 0 )
	{
		perror( "set_raw");
		return( -1 );
	}
	return( 0 );
}
read_visible_char( fd, p_c )
	int	fd;
	char	*p_c;
{
	unsigned char c;

	while( 1 )
	{
		if ( read( fd, &c, 1 ) < 1 )
			return( 0 );
		if ( Visible_to_stderr )
			write( 2, &c, 1 );
		if ( c != '\n' )
			break;
	}
	if ( ( c < ' ' ) || ( c >= 0x7F ) )
	{
		fprintf( stderr, "Invalid char 0x%x\r\n", c );
		return( 0 );
	}
	else if ( c == '^' )
	{
		if ( read( fd, &c, 1 ) < 1 )
		{
			fprintf( stderr, "Unexpected EOF on ^\r\n" );
			return( 0 );
		}
		if ( Visible_to_stderr )
			write( 2, &c, 1 );
		if ( c == '?' )
		{
			*p_c = 0x7F;
			return( 1 );
		}
		else if ( ( c >= '@' ) && ( c <= '_' ) )
		{
			*p_c = c - '@';
			return( 1 );
		}
		else
		{
			fprintf( stderr, "Invalid char ^ 0x%x\r\n", c );
			return( 0 );
		}
	}
	else if ( c == '\\' )
	{
		if ( read( fd, &c, 1 ) < 1 )
		{
			fprintf( stderr, "Unexpected EOF on \\ \r\n" );
			return( 0 );
		}
		if ( Visible_to_stderr )
			write( 2, &c, 1 );
		if ( c == '\\' )
		{
			*p_c = '\\';
			return( 1 );
		}
		else if ( c == '^' )
		{
			*p_c = '^';
			return( 1 );
		}
		else if ( c == 'E' )
		{
			*p_c = 0x1B;
			return( 1 );
		}
		else if ( c == 'b' )
		{
			*p_c = '\b';
			return( 1 );
			
		}
		else if ( c == 'r' )
		{
			*p_c = '\r';
			return( 1 );
		}
		else if ( c == 'n' )
		{
			*p_c = '\n';
			return( 1 );
		}
		else if ( c == 't' )
		{
			*p_c = '\t';
			return( 1 );
		}
		else if ( c == 's' )
		{
			*p_c = ' ';
			return( 1 );
		}
		else if ( ( c >= '0' ) && ( c <= '9' ) )
		{
			int	value;

			value = ( c - '0' ) << 6;
			if ( read( fd, &c, 1 ) < 1 )
			{
				fprintf( stderr,
					"Unexpected EOF on \\nn \r\n" );
				return( 0 );
			}
			if ( Visible_to_stderr )
				write( 2, &c, 1 );
			if ( ( c < '0' ) || ( c > '9' ) )
			{
				fprintf( stderr,
					"Invalid char \\nn 0x%x \r\n", c );
				return( 0 );
			}
			value += ( c - '0' ) << 3;
			if ( read( fd, &c, 1 ) < 1 )
			{
				fprintf( stderr, 
					"Unexpected EOF on \\nnn \r\n" );
				return( 0 );
			}
			if ( Visible_to_stderr )
				write( 2, &c, 1 );
			if ( ( c < '0' ) || ( c > '9' ) )
			{
				fprintf( stderr,
					"Invalid char \\nnn 0x%x\r\n",
					c );
				return( 0 );
			}
			value += ( c - '0' );
			*p_c = value;
			return( 1 );
		}
		else
		{
			fprintf( stderr, "Invalid char \\ 0x%x \r\n", c );
			return( 0 );
		}
	}
	else
	{
		*p_c = c;
		return( 1 );
	}
}
