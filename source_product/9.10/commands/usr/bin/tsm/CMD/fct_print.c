/*****************************************************************************
** Copyright (c) 1989 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_print.c,v 70.1 92/03/09 15:37:53 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

main(argc,argv)
int argc;
char *argv[];
{
	int i;
	char command[80];

	if ( argc < 2 )
	{
		printf("usage: pcprint filename [ filename ... ]\n");
	}
	write( 1, "\033[5i", 4 );
	for ( i = 1; i < argc; i++ )
	{
		sprintf( command, "cat %s\n", argv[i] );
		system( command );
		write( 1, "\014", 1 );
	}
	write( 1, "\033[4i", 4 );
}
