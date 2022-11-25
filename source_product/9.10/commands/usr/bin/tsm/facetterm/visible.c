/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: visible.c,v 66.6 90/10/18 09:28:07 kb Exp $ */
char	*Visible_ptr;
char	Visible_string[ 100 ];
int	Visible_count = 0;

/**************************************************************************
* visible
*	Turn the string "s" of length "len" into a 'visible' format
*	( I.E. all displayable characters ) inserting a newline every
*	70 to 80 characters.
*	If "double_377" is true, output two \377 if one is seen in "string".
*	Return a pointer to a static storage area containing the result.
**************************************************************************/
char *
visible( s, len, double_377 )
	char	*s;
	int	len;
	int	double_377;		/* if 0377 in output, send 2 */
{
	int	c;
	int	i;

	Visible_ptr = Visible_string;
	for ( i = 0; i < len; i++ )
	{
		c = ( *s++ ) & 0x00FF;
		if ( c == ' ' )
		{
			visible_out( '\\' );
			visible_out( 's' );
		}
		else if ( c < ' ' )
		{
			if ( c == 0x1B )
			{
				visible_out( '\\' );
				visible_out( 'E' );
			}
			else if ( c == '\b' )
			{
				visible_out( '\\' );
				visible_out( 'b' );
				
			}
			else if ( c == '\r' )
			{
				visible_out( '\\' );
				visible_out( 'r' );
			}
			else if ( c == '\n' )
			{
				visible_out( '\\' );
				visible_out( 'n' );
			}
			else if ( c == '\t' )
			{
				visible_out( '\\' );
				visible_out( 't' );
			}
			else
			{
				visible_out( '^' );
				visible_out( '@' + c );
			}
		}
		else if ( c < 0x7F )
		{
			if ( c == '\\' )
			{
				visible_out( '\\' );
				visible_out( c );
			}
			else if ( c == '^' )
			{
				visible_out( '\\' );
				visible_out( c );
			}
			else
			{
				visible_out( c );
			}
		}
		else if ( c == 0x7F )
		{
			visible_out( '^' );
			visible_out( '?' );
		}
		else if ( ( c == 0x00FF ) && double_377 )
		{
			visible_out( '\\' );
			visible_out( '3' );
			visible_out( '7' );
			visible_out( '7' );
			visible_out( '\\' );
			visible_out( '3' );
			visible_out( '7' );
			visible_out( '7' );
		}
		else
		{
			visible_out( '\\' );
			visible_out( '0' + ( ( c >> 6 ) & 0x3 ) );
			visible_out( '0' + ( ( c >> 3 ) & 0x7 ) );
			visible_out( '0' + (   c        & 0x7 ) );
		}
		visible_check();
	}
	*Visible_ptr++ = '\0';
	return( Visible_string );
}
/**************************************************************************
* visible
*	Turn the string "s" of length "len" into a 'visible' format
*	( I.E. all displayable characters ) inserting a newline every
*	70 to 80 characters.
*	Use an all octal representation for non displayable characters.
*	If "double_377" is true, output two \377 if one is seen in "string".
*	Return a pointer to a static storage area containing the result.
**************************************************************************/
char *
visible_octal( s, len, double_377 )
	char	*s;
	int	len;
	int	double_377;		/* if 0377 in output, send 2 */
{
	int	c;
	int	i;

	Visible_ptr = Visible_string;
	for ( i = 0; i < len; i++ )
	{
		c = ( *s++ ) & 0x00FF;
		if ( ( c == 0x00FF ) && double_377 )
		{
			visible_out( '\\' );
			visible_out( '3' );
			visible_out( '7' );
			visible_out( '7' );
			visible_out( '\\' );
			visible_out( '3' );
			visible_out( '7' );
			visible_out( '7' );
		}
		else
		{
			visible_out( '\\' );
			visible_out( '0' + ( ( c >> 6 ) & 0x3 ) );
			visible_out( '0' + ( ( c >> 3 ) & 0x7 ) );
			visible_out( '0' + (   c        & 0x7 ) );
		}
		visible_check();
	}
	*Visible_ptr++ = '\0';
	return( Visible_string );
}
/**************************************************************************
* visible_out
*	Store the character "c" in the 'visible' storage.
**************************************************************************/
visible_out( c )
	char	 c;
{
	*Visible_ptr++ = c;
	Visible_count++;
}
/**************************************************************************
* visible_check
*	If more than 70 characters have been added to the storage since
*	the last newline, add one.
*	This allows easy editing.
**************************************************************************/
visible_check()
{
	if ( Visible_count > 70 )
	{
		*Visible_ptr++ = '\n';
		Visible_count = 0;
	}
}
/**************************************************************************
* visible_end
*	Reset the newline count.
**************************************************************************/
visible_end()
{
	Visible_count = 0;
}
