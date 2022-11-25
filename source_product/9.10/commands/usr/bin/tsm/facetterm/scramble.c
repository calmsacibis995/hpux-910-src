/*****************************************************************************
** Copyright (c) 1991        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
#include <stdio.h>
/* #define DEBUG */

#define dbgprts(loc,name,string) /* */
#define dbgprtn(loc,name,num) /* */

#define MAXLEN	240

/**************************************************************************
* scramb
*	Scramble "string" using the "method" number and "key".
*	return scrambled string or error string.	
**************************************************************************/
#define MAX_METHOD 1
char	*Scramble_error_msg = "Error, bad method.";
char *
scramb( string, method, key )
	char		*string;
	int		method;
	long		key;
{
	char		hold[ MAXLEN + 1 ];
	char		c;
	static char	output[ MAXLEN + 1 ];
	int		i;
	long		offset;
	long		rotor;

	i = 0;

	key %= 32767L;

/* Normalize the string and store in hold */
	while( (c = string[ i ]) && (i < MAXLEN) )
	{
		if ( (c < ' ') || ( (unsigned char) c >= 128) )
		{
			c = ' ';
		}
		hold[ i++ ] = c;
	}
	hold[ i ] = '\0';

	if ( method == 1 )
	{
		i = 0;
		rotor = 11 + key % 97;
		while( hold[ i ] && (i < MAXLEN) )
		{
			offset = ( key * rotor + 95 ) % 95;
dbgprtn(100, "offset", offset);
			c = hold[ i ] - ' ';
			if ( c % 2 )
				rotor = ( rotor + c + 97 ) % 97;
			else
				rotor = ( rotor - c + 97 ) % 97;
			output[ i++ ] = ' ' + ( c - offset + 95 ) % 95;
dbgprtn(100, "rotor", rotor);
		}
		output[ i ] = '\0';
		return( output );
	}
	else
		return( Scramble_error_msg );
}
extern	long	atol();
remove_scramble_string( in, p_method, p_key )
	char	*in;
	int	*p_method;
	long	*p_key;
{
	int	method;
	long	key;
	char	*p;
	char	*d;

	p = in;
	/* must have leading dash */
	if ( *p != '-' )
		return( 0 );
	p++;
	/* followed by method */
	if ( ( *p < '0' ) || ( *p > '9' ) )
		return( 0 );
	method = atoi( p );
	if ( method <= 0 )
		return( 0 );
	while ( ( *p != '\0' ) && ( *p != '-' ) )
		p++;
	/* followed by dash */
	if ( *p != '-' )
		return( 0 );
	p++;
	/* followed by key */
	if ( ( *p < '0' ) || ( *p > '9' ) )
		return( 0 );
	key = atol( p );
	while ( ( *p != '\0' ) && ( *p != '-' ) )
		p++;
	/* followed by dash */
	if ( *p != '-' )
		return( 0 );
	p++;
	/* followed by request */
	d = in;
	while ( *d++ = *p++ )
		;
	*p_method = method;
	*p_key = key;
	return( 1 );		/* yes */
}
