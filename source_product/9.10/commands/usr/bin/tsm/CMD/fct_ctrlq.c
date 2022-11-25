/*****************************************************************************
** Copyright (c) 1991 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_ctrlq.c,v 70.1 92/03/09 16:40:07 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1991 by SSSI";
#endif
/*****************************************************************************
** fct_ctrlq
**
** program:	Release a tty port that has been given a control-S and is
**              waiting on a control-Q.
**
** usage:	fct_ctrlq tty-pathname
*****************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <termio.h>
main( argc, argv )
	int	argc;
	char	**argv;
{
	int	fd;
	int	status;

#ifndef TCXONC
	fprintf( stderr, "Sorry: TCXONC not implemented on this machine.\n" );
	exit( 1 );
#else
	if ( argc < 2 )
	{
		fprintf( stderr, "Usage:   %s tty_pathname\n", argv[ 0 ] );
		fprintf( stderr, "Example: %s /dev/tty02\n", argv[ 0 ] );
		exit( 1 );
	}
	fd = open( argv[ 1 ], O_RDWR | O_NDELAY );
	if ( fd < 0 )
	{
		perror( argv[ 1 ] );
		fprintf( stderr, "Usage:   %s tty_pathname\n", argv[ 0 ] );
		fprintf( stderr, "Example: %s /dev/tty02\n", argv[ 0 ] );
		exit( 1 );
	}
	status = ioctl( fd, TCXONC, 1 );
	if ( status < 0 )
	{
		perror( "TCXONC" );
		fprintf( stderr, "Usage:   %s tty_pathname\n", argv[ 0 ] );
		fprintf( stderr, "Example: %s /dev/tty02\n", argv[ 0 ] );
		exit( 1 );
	}
	fprintf( stderr, "Successful\n" );
	exit( 0 );
#endif
}
