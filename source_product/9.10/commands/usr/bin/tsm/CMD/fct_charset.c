/*****************************************************************************
** Copyright (c) 1989 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fct_charset.c,v 70.1 92/03/09 15:37:39 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif
/*
 * fct_charset.c
 *
 * Simple program to run on Facet/PC to display all the character
 * sets. Requires that the window be in VT200-7 or VT200-8 mode.
 */

main()
{
	print_charset( "US ASCII CHARACTER SET", 'B' );
	print_charset( "SUPPLEMENTAL CHARACTER SET", '<' );
	print_charset( "GRAPHICS CHARACTER SET", '0' );
	print_charset( "BRITISH CHARACTER SET", 'A' );
	print_charset( "DUTCH CHARACTER SET", '4' );
	print_charset( "FINNISH CHARACTER SET", '5' );
	print_charset( "FRENCH CHARACTER SET", 'R' );
	print_charset( "CANADIAN CHARACTER SET", 'Q' );
	print_charset( "GERMAN CHARACTER SET", 'K' );
	print_charset( "ITALIAN CHARACTER SET", 'Y' );
	print_charset( "NORWEGIAN CHARACTER SET", '6' );
	print_charset( "SPANISH CHARACTER SET", 'Z' );
	print_charset( "SWEDISH CHARACTER SET", '7' );
	print_charset( "SWISS CHARACTER SET", '=' );
	print_charset( "PC C0 CHARACTER SET", 'T' );
	print_charset( "PC C1 CHARACTER SET", 'U' );
	print_charset( "PC 8 BIT CHARACTER SET", 'V' );
	write( 1, "\033)<", 3 );
}

print_charset( charset_name, charset_id )
char *charset_name;
char charset_id;
{
	int i;

	write( 1, "\033[H\033[2J", 7 );
	printf( "%s\n\n\033)%c", charset_name, charset_id );
	printf( "\n	2	3	4	5	6	7\n");
	printf( "  +----------------------------------------------\n");
	for ( i = 0; i < 16; i++ )
	{
		printf(
		"%2d|	\016%c\017	\016%c\017	\016%c\017	\016%c\017	\016%c\017	\016%c\017\n",
		i, 0x20+i, 0x30+i, 0x40+i, 0x50+i, 0x60+i, 0x70 + i );
	}
	printf( "\nPress RETURN to continue (Interrupt to Quit)\n" );
	read( 0, &i, 1 );
}
