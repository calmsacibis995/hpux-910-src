/**			strings.c			**/

/*
 *  @(#) $Revision: 70.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This file contains all the string oriented functions for the
 *  ELM Mailer, and lots of other generally useful string functions! 
 *
 *  For BSD systems, this file also includes the function "tolower"
 *  to translate the given character from upper case to lower case.
 *
 */


#include "headers.h"


#ifdef NLS
# define NL_SETN   43
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char		*get_last();

int
printable_chars( string )

 	unsigned char *string;

{
	/*
	 *  Returns the number of "printable" (ie non-control) characters
	 *  in the given string... Modified 4/86 to know about TAB
	 *  characters being every eight characters... 
	 */


	register int    count = 0;
	unsigned char   c, 
			*pend, 
			*p;


	for ( pend = (unsigned char *)string + strlen((char *)string), 
		p = (unsigned char *)string; p < pend; ) {
		if ((c = *p++) == (unsigned char)'\t')
			count = (count | 7) + 1;
		else if (c >= (unsigned char)' ')
			count++;
	}

	return ( count );
}


int
copy_sans_escape( dest, source, len )

	unsigned char   *dest, 
		 	*source;
	int             len;

{
	/*
	 *  this performs the same function that strncpy() does, but
	 *  also will translate any escape character to a printable
	 *  format (e.g. ^(char value + 32))
	 */


	register int    i = 0, j = 0;


	while ((j < len - 2) && (source[i] != '\0')) {
		if ( source[i] < (unsigned char)' ' 
		 && source[i] != (unsigned char)'\t' ) {
			dest[j++] = '^';
			dest[j++] = source[i++] + 'A' - 1;
		} else
			dest[j++] = source[i++];
	}

	dest[j] = '\0';
}


int
tail_of( from, buffer, header_line )

	char            *from, 
			*buffer;
	int             header_line;

{
	/*
	 *  Return last two words of 'from'.  This is to allow
	 *  painless display of long return addresses as simply the
	 *  machine!username.  Alternatively, if the first three
	 *  characters of the 'from' address are 'To:' and 'header_line'
	 *  is TRUE, then return the buffer value prepended with 'To '. 
	 *
	 *  Mangled to know about the PREFER_UUCP hack.  6/86
	 *
	 *  Also modified to know about X.400 addresses (sigh) and
	 *  that when we ask for the tail of an address similar to
	 *  a%b@c we want to get back a@b ...
	 */

	/*
	 *  Note: '!' delimits Usenet nodes, '@' delimits ARPA nodes,
	 *        ':' delimits CSNet & Bitnet nodes, '%' delimits multi-
	 *	  stage ARPA hops, and '/' delimits X.400 addresses...
	 *        (it is fortunate that the ASCII character set only has
	 * 	  so many metacharacters, as I think we're probably using
	 *	  them all!!) 
	 */


	register int    loc, 
			i = 0, 
			cnt = 0;
	char            tempbuffer[VERY_LONG_STRING];


#ifdef PREFER_UUCP

	/*
	 *  let's see if we have an address appropriate for hacking: 
	 *  what this actually does is remove the spuriously added
	 *  local BOGUS_INTERNET header if we have one and the message
	 *  has some sort of UUCP component too...
	 */

	if ( chloc(from, '!') != -1 && in_string(from, BOGUS_INTERNET) )
		from[strlen(from) - strlen(BOGUS_INTERNET)] = '\0';

#endif


	for ( loc = strlen(from) - 1; loc >= 0 && cnt < 2; loc-- ) {
		if ( from[loc] == BANG || from[loc] == AT_SIGN ||
		     from[loc] == COLON )
			cnt++;

		if ( cnt < 2 )
			buffer[i++] = from[loc];
	}

	buffer[i] = '\0';

	reverse( buffer );

	if ( first_word(from, "To:") && header_line ) {
		if ( first_word(buffer, "To:") )
			sprintf( tempbuffer, "To %s", (char *) buffer + 3 );
		else
			sprintf( tempbuffer, "To %s", buffer );

		strcpy( buffer, tempbuffer );

	} else if ( first_word(buffer, "To:") ) {
		for ( i = 3; i < strlen(buffer); i++ )
			tempbuffer[i - 3] = buffer[i];

		tempbuffer[i - 3] = '\0';
		strcpy( buffer, tempbuffer );

	} else {		/* user%host@host? */

	  /*  The logic here is that we're going to use 'loc' as a handy
      	   *  flag to indicate if we've hit a '%' or not.  If we have,
      	   *  we'll rewrite it as an '@' sign and then when we hit the
      	   *  REAL at sign (we must have one) we'll simply replace it
      	   *  with a NULL character, thereby ending the string there.
      	   */

		loc = 0;

		for ( i = 0; buffer[i] != '\0'; i++ )
			if ( buffer[i] == '%' ) {
				buffer[i] = AT_SIGN;
				loc++;
			} else if ( buffer[i] == AT_SIGN && loc )
				buffer[i] = '\0';
	}
}


char  *
format_long( inbuff, init_len )

	char           *inbuff;
	int             init_len;

{
	/*
	 *  Return buffer with \n\t sequences added at each point where it 
	 *  would be more than 80 chars long.  It only allows the breaks at 
	 *  legal points (ie commas followed by white spaces).  init-len is 
	 *  the characters already on the first line...  Changed so that if 
         *  this is called while mailing without the overhead of "elm", it'll 
         *  include "\r\n\t" instead.
	 *  Changed to use ',' as a separator and to REPLACE it after it's
	 *  found in the output stream...
	 */


	register int    indx = 0, 
			in_parens = 0,
			current_length = 0, 
			depth = 15, i;
	char            buffer[VERY_LONG_STRING],
			temp_word[VERY_LONG_STRING],
	                *word, 
			*bufptr;
 

	strcpy( buffer, inbuff );

	bufptr = (char *) buffer;

	current_length = init_len + 2;	
	temp_word[0] = '\0';

	while ( (word = get_token(bufptr, ",", depth, FALSE)) != NULL ) {

		/*
		 * Don't touch comment part.
		 */

		bufptr = NULL;

		for ( i = 0; i < strlen(word); i++ )
			if ( *(word + i) == '(' )
				in_parens++;
			else if ( *(word + i) == ')' )
				in_parens--;

		sprintf( temp_word, "%s%s%s", temp_word,
			 strlen(temp_word) ? ",":"", word );

		if ( in_parens )
			continue;

		/*
		 * first, decide what sort of separator we need, if any... 
		 */

		if ( strlen(temp_word) + current_length > 79 ) {
			if ( indx > 0 ) {
				formatted_buf[indx++] = ',';	
				formatted_buf[indx++] = '\n';

				if ( mail_only )
					formatted_buf[indx++] = '\r';

				formatted_buf[indx++] = '\t';
			}

			/*
			 * now add this pup! 
			 */

			for ( i = (temp_word[0] == ' ' ? 1 : 0); i < strlen(temp_word); i++ )
				formatted_buf[indx++] = temp_word[i];

			current_length = strlen( temp_word ) + 8;	/* 8 = TAB */

		} else {		/* just add this address to the list.. */

			if ( indx > 0 ) {
				formatted_buf[indx++] = ',';	/* comma added! */
				formatted_buf[indx++] = ' ';
				current_length += 2;
			}

			for ( i = (temp_word[0] == ' ' ? 1:0); i < strlen(temp_word); i++ )
				formatted_buf[indx++] = temp_word[i];

			current_length += strlen( temp_word );
		}

		temp_word[0] = '\0';

	}

	formatted_buf[indx] = '\0';

	return ( (char *) formatted_buf );
}

char  *
strip_commas( string )

	char           *string;

{
	/*
	 *  return string with all commas changed to spaces.  This IS
	 *  destructive and will permanently change the input string.. 
	 */


	register int    i;


	for ( i = 0; i < strlen(string); i++ )
		if ( string[i] == COMMA )
			string[i] = SPACE;

	return ( (char *) string );
}


char  *
strip_tabs( string )

	char           *string;

{
	/*
	 *  return string with all tabs changed to spaces.  This IS
	 *  destructive and will permanently change the input string.. 
	 */


	register int    i;


	for ( i = 0; i < strlen(string); i++ )
		if ( string[i] == TAB )
			string[i] = SPACE;

	return ( (char *) string );
}


char  *
strip_parens( string )

	char           *string;

{
	/*
	 *  Return string with all parenthesized information removed.
	 *  This is a non-destructive algorithm... 
	 */


	register int    i, 
			depth = 0, 
			stripped_buf_index = 0;


	for ( i = 0; i < strlen(string); i++ ) {
		if ( string[i] == '(' )
			depth++;
		else if ( string[i] == ')' )
			depth--;
		else if ( depth == 0 )
			stripped_buf[stripped_buf_index++] = string[i];
	}

	stripped_buf[stripped_buf_index] = '\0';

	return ( (char *) stripped_buf );
}


int
move_left( string, chars )

	char            string[];
	int             chars;

{
	/*
	 *  moves string chars characters to the left DESTRUCTIVELY 
	 */

	register int    i;

	/*
	 * index starting at zero! 
	 */

	for ( i = chars; string[i] != '\0' && string[i] != '\n'; i++ )
		string[i - chars] = string[i];

	string[i - chars] = '\0';
}


int
remove_first_word( string )

	char           *string;

{		
	/*
	 *  removes first word of string, ie up to first non-white space
	 *  following a white space! 
	 */


	register int    loc;


	for ( loc = 0; string[loc] != ' ' && string[loc] != '\0'; loc++ );

	while ( string[loc] == ' ' || string[loc] == '\t' )
		loc++;

	move_left( string, loc );
}


int
split_word( buffer, first, rest )

	char            *buffer, 
			*first, 
			*rest;

{
	/*
	 *  Rip the buffer into first word and rest of word, translating it
	 *  all to lower case as we go along..
	 */


	register int    i, 
			j = 0;


	/*
	 *  skip leading white space, just in case.. 
	 */

	for ( i = 0; whitespace(buffer[i]); i++ );

	/*
	 *  now copy into 'first' until we hit white space or EOLN 
	 */

	for ( j = 0; i < strlen(buffer) && !whitespace(buffer[i]); )
		first[j++] = (char)tolower( (int)buffer[i++] );

	first[j] = '\0';

	while ( whitespace(buffer[i]) )
		i++;

	for ( j = 0; i < strlen(buffer); i++ )
		rest[j++] = (char)tolower( (int)buffer[i] );

	rest[j] = '\0';

	return;
}


char 
*tail_of_string( string, maxchars )

	char            *string;
	int             maxchars;

{
	/*
	 *  Return a string that is the last 'maxchars' characters of the
	 *  given string.  This is only used if the first word of the string
	 *  is longer than maxchars, else it will return what is given to
	 *  it... 
	 */

	static char     tail_comp_buf[SLEN];
	register int    indx, 
			i;

	for ( indx = 0; !whitespace(string[indx]) && indx < strlen(string);
	     indx++ );

	if ( indx < maxchars ) {
		strncpy( tail_comp_buf, string, (size_t)maxchars - 2 );	/* word too short */
		tail_comp_buf[maxchars - 2] = '.';
		tail_comp_buf[maxchars - 1] = '.';
		tail_comp_buf[maxchars] = '.';
		tail_comp_buf[maxchars + 1] = '\0';
	} else {
		i = maxchars;
		tail_comp_buf[i--] = '\0';
		while ( i > 1 )
			tail_comp_buf[i--] = string[indx--];
		tail_comp_buf[2] = '.';
		tail_comp_buf[1] = '.';
		tail_comp_buf[0] = '.';
	}

	return ( (char *) tail_comp_buf );
}


int
reverse( string )

	char           *string;

{
	/*
	 *  reverse string... pretty trivial routine, actually! 
	 */


	char            buffer[VERY_LONG_STRING];
	register int    i, 
			j = 0;


	for ( i = strlen(string) - 1; i >= 0; i-- )
		buffer[j++] = string[i];

	buffer[j] = '\0';

	strcpy( string, buffer );
}


int
get_word( buffer, start, word )

	char		*buffer,
			*word;
	int		*start;			
	
{
	int		loc = 0,
			in_parens = 0;
	register int    i;


	i = *start;

	if ( buffer[i] != '(' )
		while ( buffer[i] != ' ' && buffer[i] != '\0' )
			word[loc++] = buffer[i++];
	else
		do {
			if ( buffer[i] == '(' )
				in_parens++;
			else if ( buffer[i] == ')' )
				in_parens--;
			
			word[loc++] = buffer[i++];
		} while ( in_parens > 0 && buffer[i] != '\0' );

	word[loc] = '\0';

	if ( buffer[i] == '\0' )
		*start = -1;
	else
		*start = ++i;

	if ( word[0] == '(' )
		return( TRUE );
	else 
		return ( FALSE );

}


int
search_delimiter( buffer, start, word )

	char            *buffer, 
			*word;
	int             start;

{
	/*
	 * 	return next word in buffer, starting at 'start'.
	 *	delimiter is space or end-of-line.  Returns the
	 *	location of the next word, or *	-1 indicates no
	 *      more token in buffer.
	 */

	register int    loc = 0,
			indx;

	indx = start;

	while ( buffer[indx] != ' ' && buffer[indx] != '\0'
	    && buffer[indx] != ',' && buffer[indx] != '\t'
	    && buffer[indx] != '(' && buffer[indx] != ')' ) 
		word[loc++] = buffer[indx++];

	word[loc] = '\0';

	if ( buffer[indx] == '\0' )
		return( -1 );
	else
		return ( indx );
}


char   *
shift_lower( string )

	char           *string;

{
	/*
	 *  return 'string' shifted to lower case.  Do NOT touch the
	 *  actual string handed to us! 
	 */


	register int    i;


	for ( i = 0; i < strlen(string); i++ )
		if ( isupper(string[i]) )
			lowered_buf[i] = (char)tolower( (int)string[i] );
		else
			lowered_buf[i] = string[i];

	lowered_buf[strlen( string )] = 0;

	return ( (char *) lowered_buf );
}


int
Centerline( line, string )

	int             line;
	char            *string;

{
	/*
	 *  Output 'string' on the given line, centered. 
	 */

	register int    length, 
			col;

	length = strlen( string );

	if ( length > COLUMNS )
		col = 0;
	else
		col = ( COLUMNS - length ) / 2;

	PutLine0( line, col, string );
}


char   *
get_token( source, keys, depth, aliasing )

	char            *source, 
			*keys;
	int             depth;
	int		aliasing;

{
	/*
	 *  This function is similar to strtok() (see "opt_utils")
	 *  but allows nesting of calls via pointers... 
	 */


	register int    last_ch;
	static char     *return_value;
	char		*sourceptr;


	if ( depth > MAX_RECURSION ) {
		if (aliasing)
			error1( catgets(nl_fd,NL_SETN,2,"possible alias loop.  an alias could not be resolved in %d expansions!"), MAX_RECURSION );
		else
			error1( catgets(nl_fd,NL_SETN,1,"get_token calls nested greater than %d deep!"), MAX_RECURSION );
		emergency_exit();
	}

	if ( source != NULL )
		token_buf[depth] = source;

	sourceptr = token_buf[depth];

	if ( *sourceptr == '\0' )
		return ( NULL );		/* we hit end-of-string last time!? */

	sourceptr += strspn( sourceptr, keys );	/* skip the bad.. */

	if ( *sourceptr == '\0' ) {
		token_buf[depth] = sourceptr;
		return ( NULL );		/* we've hit end-of-string   */
	}

	last_ch = strcspn( sourceptr, keys );	/* end of good stuff   */

	return_value = sourceptr;		/* and get the ret     */

	sourceptr += last_ch;			/* ...value            */

	if ( *sourceptr != '\0' )		/** don't forget if we're at end! **/
		sourceptr++;

	return_value[last_ch] = '\0';		/* ..ending right      */

	token_buf[depth] = sourceptr;		/* save this, mate!    */

	return ( (char *) return_value );	/* and we're outta here! */
}


char *
get_last( buffer )

	char 		*buffer;

{
	/*
	 *  This function takes the last component of the PATH.
	 *      ...../.. ../filename  ->  filename
	 */


	int		i,
			j;


	j = strlen( buffer ) - 1;

	for ( i = j; j-i < SLEN-2 && *(buffer+i) != '/'; i-- )
		last_cmp[j-i] = buffer[i];

	last_cmp[j-i] = '\0';
		
	reverse( last_cmp );

	return( (char *)last_cmp );
}
