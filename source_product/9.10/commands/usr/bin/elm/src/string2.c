/**			string2.c			**/

/*
 *  @(#) $Revision: 66.3 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains string functions that are shared throughout the
 *  various ELM utilities...
 *
 */


#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define whitespace(c)		( c == ' ' || c == '\t' )


int
in_string( buffer, pattern )

	register char   *buffer, 
			*pattern;

{
	/*
	 *  Returns TRUE iff pattern occurs IN ITS ENTIRETY in buffer. 
	 */


	register int j, p;


	for (; *buffer; buffer++) {
		for (j=0; (p=pattern[j]) && buffer[j] == p; j++ );
		if ( p == '\0' )
			return ( TRUE );
	}
	return ( FALSE );
}


int
in_string_indx( buffer, pattern )

	char            *buffer, 
			*pattern;

{
	/*
	 *  Returns the index of the first character of the pattern in
	 *  the buffer iff there is a full match, otherwise it returns 
	 *  -1 to indicate a failure.
	 */


	register int    i = 0, 
			j = 0;


	while ( buffer[i] != '\0' ) {
		while ( buffer[i++] == pattern[j++] )
			if ( pattern[j] == '\0' )
				return ( i - j );
		i = i - j + 1;
		j = 0;
	}
	return ( -1 );
}


int
chloc( string, ch )

	char            *string, 
			ch;

{
	/*
	 *  returns the index of ch in string, or -1 if not in string 
	 */


	register int    i;


	for ( i = 0; string[i]; i++ )
		if ( string[i] == ch )
			return ( i );
	return ( -1 );
}


int
occurances_of( ch, string )

	char            ch, 
			*string;

{
	/*
	 *  returns the number of occurances of 'ch' in string 'string' 
	 */

	register int    count = 0, 
			i;

	for ( i = 0; i < strlen(string); i++ )
		if ( string[i] == ch )
			count++;

	return ( count );
}


int
remove_possible_trailing_spaces( string )

	char           *string;

{
	/*
	 *  an incredibly simple routine that will read backwards through
	 *  a string and remove all trailing whitespace.
	 */


	register int    i;


	for ( i = strlen(string) - 1; whitespace(string[i]); i-- )

		/*
		 *  spin backwards 
		 */

		string[i + 1] = '\0';	/* note that even in the worst case
					 * when there are no trailing spaces
					 * at all, we'll simply end up
					 * replacing the existing '\0' with
					 * another one! 
					 */
}


int
first_word(string, word)
register char *string, *word;
{
	/* Does string start with word? */

	while (*word)
		if (*string++ != *word++)
			return 0;
	return 1;
}
