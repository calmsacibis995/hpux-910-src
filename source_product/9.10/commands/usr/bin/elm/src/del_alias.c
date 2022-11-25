/**			del_alias.c			**/

/*
 *  @(#) $Revision: 70.4 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Delete-alias function in alias mode...
 */

#include <stdio.h>

#include "headers.h"


#ifdef NLS
# define NL_SETN   9
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#define group(string)		(strpbrk(string,", ") != NULL)

char		*get_member();		/* extract member from group  		*/
int		deleting = 0;		/* switch comment in install_alias()	*/
static int	buff_loaded;		/* indicate continuing line   		*/
static char	source[VERY_LONG_STRING];/* buffer for get_member()		*/

extern int	cancelled;
extern int	num_char;


int
delete_alias()

{
	/*
	 * This function delete alias from $HOME/.elm/alias.text
	 * file. In alias-text file, following case is supposed.
	 *     1. 'alias' is the target alias name.
	 *     2. 'alias' is group-alias and it contain the target
	 *        name in the address list.
	 *     3. 'address' list contain only one alias address
	 *        and it's the target alias.
	 * In case of 1 and 3, this delete the line from alias text
	 * line. In case of 2, only the target alias is deleted from
	 * the address list.
	 * 
	 * This function return 1 if the alias is deleted successfully
	 * or return 0 if do nothing.
	 *
	 * The algorithm is:
	 *      1. get the alias name to delete.
	 *	2. check the existence and permission to delete
	 *	3. first, copy the original file to temporary file.
	 *	4. remove the original file.
	 *	5. read line by line from temporary file and
	 *	   delete the target alias from the line along the 
	 *	   case above, then write to the file with the original
	 *	   file name.
	 *	6. at last, remove the temporary file.
	 * After exit this routine the function 'install_aliases()'
	 * will run by the return value of this routine '1'.
	 */


	FILE    *in,			/* fdes for file to read    */
		*text;			/* fdes for file to write   */
	char	name[LONG_STRING],	/* entered name of target   */
		aliases[LONG_STRING],   /* alias part of the line   */
		comment[LONG_STRING],	/* comment part of the line */
		alias_text[SLEN],	/* buffer for file name     */
		temp_text[SLEN],     	/* buffer for file name     */
		buffer[VERY_LONG_STRING],/* buffer for a alias line  */
		member[VERY_LONG_STRING],/* buffer for modified addrs*/
		*address,		/* ptr for addrs list defined 
					   static in aliaslib.c */
		*addr,			/* temporary buffer	    */
		*ptr;			/* temporary pointer        */
	int	current_status;		/* stack for file existing  */


	PutLine0( LINES-2, 0, catgets(nl_fd,NL_SETN,1,"Delete alias name: ") );
	CleartoEOLN();

	if ( hp_terminal )
		define_softkeys( CANCEL );

	name[0] = '\0';

	num_char = strlen(catgets(nl_fd,NL_SETN,1,"Delete alias name: "));

	if ( optionally_enter( name, LINES-2, num_char, FALSE ) ){
		cancelled = TRUE;
		return ( 0 );		/* cancelled by softkey */
	}

	/*
	 * If we get nothing, then return without doing anything 
	 */

	if ( strlen( name ) == 0 ) {
		cancelled = TRUE;
		return( 0 );
	}

	if ( chloc(name, ' ') != -1 || chloc(name, TAB) != -1 
				|| chloc(name, ',') != -1 ) {
		error( catgets(nl_fd,NL_SETN,7,"Can't delete multiple aliases at once") );
		return( 0 );
	}

	/*
	 * Check the existance of the alias. If can't find,
	 * return without doing anything.
	 * First, search in user and system alias file, and then
	 * search in only system alias file. If the target is 
	 * found in only system alias file, then return without
	 * doing anything.
	 */

	if ( (address = get_alias_address( name, 0, 0 )) == NULL ){
		dprint( 3, (debugfile, "Attempt to delete a non-existing alias [%s] in delete_alias\n", name ));
		error1( catgets(nl_fd,NL_SETN,2,"no aliases for that name: %s"), name );
		return( 0 );
	}

	if( system_file != 0 ){			/* Have system alias file ? */
		current_status = system_files;
		system_files = 0;

		if (( address = get_alias_address( name, 0, 0 )) == NULL ){
			dprint( 3, (debugfile, 
				"Attempt to delete a system alias [%s] in delete_alias\n",
				name ) );
			error1( catgets(nl_fd,NL_SETN,3,"Can't delete system alias: %s"), name );
			return( 0 );
		}

		system_files = current_status;
	}

	/*
	 * Now we open temporary file and  copy the original to it 
	 */

	sprintf( alias_text, "%s/%s", getenv("HOME"), ALIAS_TEXT );
	
	if ( (in = user_fopen( alias_text, "a" )) == NULL ){
		dprint( 2, ( debugfile, 
			"Failure attempting to open alias text file within %s", 
			"delete_alias" ) );
		error1( catgets(nl_fd,NL_SETN,4,"Can't open alias text file: %s"), alias_text );
		(void) sleep( 3 );
		return( 0 );
	}

	user_fclose( in );

	sprintf( temp_text, "%s/%s%d", tmpdir, temp_alias, getpid() );

	if ( link( alias_text, temp_text ) != 0 ){
		if ( errno == EXDEV ) {	/** different file devices!  Use copy! **/

			if ( copy(alias_text, temp_text) != 0 ) {
				dprint( 1, (debugfile, "delete_alias: copy(%s, %s) failed;",
					    alias_text, temp_text) );
				dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
				  	    error_description(errno)) );
				error( catgets(nl_fd,NL_SETN,5,"couldn't modify alias text file!") );
				(void) sleep( 1 );
				unlink( temp_text );
				return( 0 );
			}

		} else {
			dprint( 1, (debugfile, "link(%s, %s) failed (leavembox)\n",
				    alias_text, temp_text) );
			dprint( 1, (debugfile, "** %s - %s **\n", error_name(errno),
			 	    error_description(errno)) );
			error2( catgets(nl_fd,NL_SETN,6,"link failed! %s - %s"), error_name(errno),
			        error_description(errno) );
			return( 0 );
		}

	}

	unlink( alias_text );

	/*
	 * Read from temporary file and transfer the valid line to
	 * the file with original name.
	 */

	if ( (in = fopen( temp_text, "r" )) == NULL ){
		dprint( 3, (debugfile, 
			"Failure attempting to edit text file [%s]", temp_text ) );
		emergency_exit();
	}

	if ( (text = user_fopen( alias_text, "a" )) == NULL ){
		dprint( 3, (debugfile, 
			"Failure attempting to edit text file [%s]", alias_text ) );
		emergency_exit();
	}

	buffer[0] = '\0';

	while( get_alias( in, buffer ) != -1 ){

		/*
		 * decompose the text line into each component 
		 */

		if ( !extract_alias(buffer, aliases, comment, address) )
			continue;

		/* 
		 * If alias field has alias list, then check
		 * whether it contains the target or not.
		 * If contain target, delete it.
		 * NOTE: if the list is " target,", then the 'aliases'
		 * will have nothing.
		 */

		if ( group( aliases ) ){		/* contain ',' ? */
			member[0] = '\0';
			addr=aliases;

			while( (ptr = get_member( addr, "," )) != NULL ){
				if ( strcmp( ptr, name ) ){
					strcat( member, ptr );
					strcat( member, "," );
				}
				addr = NULL;
			}

			member[ strlen(member)-1 ] = '\0';
			strcpy( aliases, member );
		}

		if ( !equal(aliases, name) && !equal(address, name)
					   && strlen(aliases) != 0 ){

			/*
			 * If it's group alias, then check in the address lists.
			 * If the target alias is contained in it, then
			 * delete only it from the list.
			 */

			if ( group( address ) ){		/* contain ',' ?*/
				member[0] = '\0';
				addr=address;

				while( (ptr = get_member( addr, "," )) != NULL ){
					if ( strcmp( ptr, name ) ){
						strcat( member, ptr );
						strcat( member, "," );
					}
					addr = NULL;
				}

				member[ strlen(member)-1 ] = '\0';
				strcpy( address, member );
			}
		
			if ( strlen(address) != 0 )		/* case of 'xxx,' */
				if ( fprintf( text, "%s = %s%s%s\n", 
					      aliases, 
					      strlen(comment) ? comment:"",
					      strlen(comment) ? " = ":""
					      , address ) == EOF ){
				        fclose( in );
					user_fclose( text );
					dprint( 3, (debugfile, 
						"Failure attempting to deleting alias [%s]",
						temp_text ) );
					emergency_exit();
				}	
		}
	}

	fclose( in );
	user_fclose( text );
	unlink( temp_text );

	deleting += 1;
	return( 1 );				/* we've done the work successfully !! */
}


int
extract_alias( buffer, aliases, comment, address )

	char	*buffer,	/* line to decomposed into component */
		*aliases,	/* return with alias part            */
		*comment,	/* return with comment part          */
		*address;       /* return with address part          */

{
	/*
	 * This function is almost same as 'put_alias()' in 
	 * ../utils/elmalias.c.  The differnce is this doesn't
	 * call 'add_to_table()'.
	 */


	int             first, 
			last, 
			has_equal,
			num_equal,
			i = 0, 
			j = 0;


	has_equal = chloc(buffer, '=');

	if ( has_equal == -1 )
		return( 0 );			/* Ignore ! */

	(void) remove_all( ' ', TAB, buffer );

	has_equal = chloc(buffer, '=');

	for ( i = 0; buffer[i] != '=' && i < LONG_STRING && buffer[i] != '\0'; i++ )
		aliases[i] = buffer[i];

	aliases[i] = '\0';

	for ( i = strlen(buffer) - 1; buffer[i] != '=' && i > 0
					&& j < LONG_STRING ; i-- )
		address[j++] = buffer[i];

	address[j] = '\0';

	comment[0] = '\0';		/* default to nothing at all..*/

	if ( (num_equal=occurances_of('=', buffer)) > 1 ){

		first = has_equal + 1;
		last  = has_equal;

		while( --num_equal > 0 )
			last = last + chloc( buffer+last+1, '=' ) + 1;

		last--;		/* set before the last = */

		(void) extract_comment( comment, buffer, first, last );

	}

	(void) reverse( address );

	return( 1 );

}


int
get_alias( file, buffer )

	FILE           *file;
	char           *buffer;

{
	/*
	 * load buffer with the next complete alias from the file. (this can
	 * include reading in multiple lines and appending them all
	 * together!)  Returns EOF after last entry in file. 
	 *
	 * Lines that start with '#' are assumed to be comments and are ignored. 
	 * White space as the first field of a line is taken to indicate that
	 * this line is a continuation of the previous. 
	 */


	static char     mybuffer[VERY_LONG_STRING];
	int             done = 0, 
			first_read = 1;


	/*
	 *  get the first line of the entry... 
	 */

	buffer[0] = '\0';			/* zero out line */

	do {
		if ( get_alias_line(file, mybuffer, first_read) == -1 ) {
			buff_loaded = 0;
			return ( -1 );
		}

		first_read = 0;

		if ( mybuffer[0] != '#' )
			strcpy( buffer, mybuffer );

	} while ( strlen(buffer) == 0 );

	/*
	 *  now read in the rest (if there is any!) 
	 */

	do {
		if ( get_alias_line(file, mybuffer, first_read) == -1 ) {
			buff_loaded = 0;	/* force a read next pass! */
			return ( 0 );		/* okay. let's just hand 'buffer'
						 * back! */
		}

		done = ( !whitespace(mybuffer[0]) );

		if ( !done && strlen(buffer)+strlen(mybuffer) < VERY_LONG_STRING )
			strcat( buffer, mybuffer );

		done = ( done && mybuffer[0] != '#' );

	} while ( !done );

	return ( 0 );	
}


int
get_alias_line( file, buffer, first_line )

	FILE           *file;
	char           *buffer;
	int             first_line;

{
	/*
	 *  read line from file.  If first_line and buff_loaded, 
	 *  then just return! 
	 */


	int             stat;


	if ( first_line && buff_loaded ) {
		buff_loaded = 1;
		return (0);
	}

	buff_loaded = 1;		/* we're going to get SOMETHING in the buffer */

	stat = fgets( buffer, VERY_LONG_STRING, file ) == NULL ? -1 : 0;

	if ( stat != -1 )
		no_ret( buffer );

	return ( stat );
}


int
remove_all( c1, c2, string )

	char            c1, 
			c2, 
			*string;

{
	/*
	 * Remove all occurances of character 'c1' or 'c2' from the string.
	 * Hacked (literally) to NOT remove ANY characters from within the
	 * equals fields.  This will only be used if the line contains TWO
	 * equalss (and comments with equalss in them are the kiss of death!) 
	 */


	char            buffer[VERY_LONG_STRING];
	register int    i = 0, 
			j = 0, 
			last_j,
			first_equals = -1, 
			last_equals = -1;


	for ( i = 0; string[i] != '='; i++ ) {

		if ( string[i] != c1 && string[i] != c2 && string[i] != ',' )
			buffer[j++] = string[i];
		else if ( j != 0 && buffer[j-1] != ',' )
			buffer[j++] = ',';

	}

	while( buffer[j-1] == ',' )
		j--;

	first_equals = i;

	for ( last_equals = strlen(string); string[last_equals] != '=';
			     last_equals-- );

	for ( i = first_equals; i <= last_equals; i++ )
		buffer[j++] = string[i];

	last_j = j;

	for ( i = last_equals + 1; string[i] != '\0'; i++ ) {

		if ( string[i] != c1 && string[i] != c2 && string[i] != ',' )
			buffer[j++] = string[i];
		else if ( j != last_j && buffer[j-1] != ',' )
			buffer[j++] = ',';

	}

	while( buffer[j-1] == ',' )
		j--;

	buffer[j] = '\0';
	strcpy( string, buffer );
}


int
extract_comment( comment, buffer, first, last )

	char            *comment, 
			*buffer;
	int             first, 
			last;

{
	/*
	 *  Buffer contains a comment, located between the first and last
	 *  values.  Copy that into 'comment', but remove leading and
	 *  trailing white space.  Note also that it doesn't copy past
	 *  a comma, so `unpublishable' comments can be of the form;
	 *	dave: Dave Taylor, HP Labs : taylor@hplabs
	 *  and the output will be "taylor@hplabs (Dave Taylor)".
	 */


	register int    loc = 0;


	/*
	 *  first off, skip the LEADING white space... 
	 */

	while ( whitespace(buffer[first]) )
		first++;

	/*
	 *  now let's backup the 'last' value until we hit a non-whitespace 
	 */

	while ( whitespace(buffer[last]) )
		last--;

	/*
	 *  now a final check to make sure we're still talking about a 
	 *  reasonable string (rather than a "joe :: joe@dec" type string) 
	 */

	if ( first <= last ) {

		/*
		 * one more check - let's find the comma, if present... 
		 */

		for ( loc = first; loc <= last; loc++ )
			if ( buffer[loc] == ',' ) {
				last = loc - 1;
				break;
			}

		loc = 0;

		while ( first <= last && loc < LONG_STRING )
			comment[loc++] = buffer[first++];

		comment[loc] = '\0';
	}
}


char
*get_member( members, key )

	char	*members,
		*key;

{
	/*
	 * This function is added by UXL in HP and almost same as
	 * 'strtok()'.
	 * 'strtok()' doesn't return correct string for last token
	 * as I know. I mean for line:
	 *       < token1 >key< token2 >key< token3 >
	 * I get < token3 > correctly and at next entry to this
	 * routine, I get NULL.
	 * At first time of calling this routine, set members to
	 * correct pointer. It doesn't need to set next pointer
	 * to next string any more. This searches from the next
	 * with 'members = NULL'.
	 * This returns the pointer to token, or zero if no token
	 * remained.
	 *
	 * Usage:
	 *	char	*ptr, *member, *key, *array[];
	 *	int	i;
	 *
	 *	while(( ptr = get_member( members, key )) != NULL ){
	 *		member = NULL;
	 *		strcpy( array[i++], member );
	 *	}
	 */


	register int 	length;
	static int	indx;
	char		*return_value;


	if ( members != NULL ){
		indx = 0;
		strcpy( source, members );
	}
	
	if ( source[indx] == '\0' )
		return( NULL );

	indx += strspn( &source[indx], key );

	if ( source[indx] == '\0')
		return( NULL );

	/*
	 * We are just at top of token 
	 */

	return_value = &source[indx];

	length = strcspn( return_value, key );
	indx = indx + length;

	if ( source[indx] != '\0' ){
		source[indx] = '\0';
		indx += strlen( key );		/* for next entry */
	}

	return( (char *) return_value );
}
