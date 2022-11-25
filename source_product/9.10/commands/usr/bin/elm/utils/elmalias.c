/**			elmalias.c			**/

/*
 *  @(#) $Revision: 70.1 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This program will install a new set of aliases  or check
 *  or list aliases for invoking user, useable by the Elm mailer. 
 *
 *	If invoked with no arguments, it assumes that
 *  it is working with an individual users alias tables, and
 *  generates the aliases.hash and aliases.data files in their
 *  home directory.
 *	If, however, it is invoked with "-q", then
 *  it assumes that the user is updating the system alias
 *  file and uses the defaults for everything.
 *
 *  The format for the input file is;
 *    alias1, alias2, ... = username = address
 *or  alias1, alias2, ... = groupname= member, member, member, ...
 *                                     member, member, member, ...
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "defs.h"


#ifdef NLS
# include <nl_types.h>
# define NL_SETN   1
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#define group(string)		( strpbrk(string,", ") != NULL )

#define ERROR	-1

struct alias_rec
                shash_table[MAX_SALIASES];	/* the actual hash table     */

struct alias_rec
                uhash_table[MAX_UALIASES];	/* the actual hash table     */

int             hash_table_loaded = 0,		/* is system table actually loaded? */
                buff_loaded,			/* for file input overlap... */
                error = 0,			/* if errors, don't save!    */
                is_system = 0,			/* system file updating?     */
                count = 0,			/* how many aliases so far?  */
                groups_encountered = 0;		/* how many of 'em are groups? */
long            offset = 0L;			/* data file line offset!    */
char            home[SLEN];			/* the users home directory  */

extern int      errno;


#ifdef NLS
nl_catd		nl_fd;			/* message catalogue file    */
char		lang[NLEN];			/* language name for NLS     */
#endif NLS


char		*strcpy(),
		*fgets();

uid_t		geteuid();

main( argc, argv )

	int             argc;
	char           *argv[];

{
	/*
	 *  This is main routine of elmalias. Check the command line
	 *  options and set the corresponding flags. Then call
	 *  some functions for the operation.
	 */

	char            aliaslist[SLEN], 
			regular_exp[SLEN], 
			buffer[SLEN];
	int		quiet = 0,
			index;
	register int	c;
	extern int	opt_index;		/*argnum+1 when we leave*/
	extern char	*optional_arg;		/*optional arg as we go*/


#ifdef NLS
	/*
	 * initalize for native language messages.... now set ready 
	 */

	strcpy( lang, getenv( "LANG" ) );
	nl_init( lang );
	nl_fd = catopen( "elmalias", 0 );
#endif NLS


	while( ( c = get_options( argc, argv, "c:lq" )) > 0 ){
		switch ( c ){
			case 'c':
				strcpy( aliaslist, optional_arg );
				index = 3;

				while ( argc > index ) {
					strcat( aliaslist, " " );
					strcat( aliaslist, argv[ index ] );
					index++;
				}	

				checkalias( aliaslist );
				exit( 0 );

			case 'l':
				if ( argc == 3 ){
					strcpy( regular_exp, argv[2] );
				} else if ( argc > 3 ){
						c = ERROR;
						goto help;
				}

				listalias( argc, regular_exp );
				exit( 0 );

			case 'q':
				if ( argc >2 ){
					c = ERROR;
					goto help;
				}
				quiet++;
				break;
		}
	}

  help:	if ( c == ERROR ){
		printf( catgets(nl_fd,NL_SETN,1,"\n\nUsage: %s    [ -c alias-list | -q ] \n   or  %s -l [ regular-expression ]\n\n"), argv[0], argv[0] );

		/* display help brief */

		printf( catgets(nl_fd,NL_SETN,2,"\t -c alias-list\t checkalias - check to see if an alias is defined.\n") );
		printf( catgets(nl_fd,NL_SETN,3,"\t -l expression\t listalias - list user and system alias.\n") );
		printf( catgets(nl_fd,NL_SETN,4,"\t -q\t\t systemalias - install system-aliases.\n\n\n") );
		exit( 1 );
	}

	/*
	 * Now we go without any argument.
	 * Let's install our own aliases, shall we ?!
	 */

	if ( !quiet && argc > 1 ){
		c = ERROR;
		goto help;
	}

	newaliases( quiet, argv );


#ifdef NLS
	catclose( nl_fd );
#endif NLS


	exit( 0 );

}


int
newaliases( quiet, argv )

	int             quiet;
	char 		*argv[];

{
	/*
	 *  Install aliases. If 'quiet' is TRUE and uid is root,
	 *  then I'm going to add system aliases. In other case,
	 *  install user aliases.
	 */


	FILE            *in, 
			*data;
	char            inputname[SLEN], 
			hashname[SLEN], 
			dataname[SLEN],
	                buffer[VERY_LONG_STRING];
	int             hash, 
			fault_cnt = 0,
			owner; 


	owner = (int)geteuid();

	if ( owner == 0 && quiet ) {			/* being run by root! */
		printf( catgets(nl_fd,NL_SETN,5,"Would you like to update the system aliases? (y/n)") );

		do {	
			gets( buffer );
		} while( buffer[0] != 'y' && buffer[0] != 'Y'
		      && buffer[0] != 'n' && buffer[0] != 'N' );

		if ( buffer[0] == 'y' || buffer[0] == 'Y' ) {
			printf( catgets(nl_fd,NL_SETN,6,"Updating the system alias file...\n") );
			strcpy( inputname, system_text_file );
			strcpy( hashname, system_hash_file );
			strcpy( dataname, system_data_file );
			is_system++;
			init_table( shash_table, MAX_SALIASES );
		} else
			printf( catgets(nl_fd,NL_SETN,7,"Updating your personal alias file...\n") );
	}

	if ( !is_system ) {
		if ( strcpy(home, getenv("HOME")) == NULL )
			exit( printf( catgets(nl_fd,NL_SETN,8,"I'm confused - no HOME variable in environment!\n")) );

		sprintf( inputname, "%s/%s", home, ALIAS_TEXT );
		sprintf( hashname, "%s/%s", home, ALIAS_HASH );
		sprintf( dataname, "%s/%s", home, ALIAS_DATA );

		init_table( uhash_table, MAX_UALIASES );

		read_in_system( shash_table, sizeof(shash_table) );
	}

	if ( (in = fopen(inputname, "r")) == NULL ) {

		/*
		 *  let's see if they have the files in the old place... 
		 */

		sprintf( buffer, "%s/.alias_text", home );

		if ( access(buffer, ACCESS_EXISTS) != -1 ) {
			update_alias_file_locations();
			in = fopen( inputname, "r" );
		} else {
			printf( catgets(nl_fd,NL_SETN,9,"You don't appear to have an alias source text file!\n") );
			printf( catgets(nl_fd,NL_SETN,10,"(I looked for \"%s\" and the old \"%s\" and got error #%d)\n"),
			       inputname, buffer, errno );
			exit( 1 );
		}
	}

	if ( (hash = open(hashname, O_WRONLY | O_CREAT, 0644)) == -1 )
		exit( printf( catgets(nl_fd,NL_SETN,11,"I couldn't open file %s for output!\n"),
					hashname) );

	if ( (data = fopen(dataname, "w")) == NULL )
		exit( printf( catgets(nl_fd,NL_SETN,11,"I couldn't open file %s for output!\n"),
					dataname) );

	buff_loaded = 0;			/* file buffer empty right now! */

	while ( get_alias(in, buffer) != -1 ) {

		if ( fault_cnt ){
			printf( catgets(nl_fd,NL_SETN,13,"\n\nAlias database is already full. \n" ) );
			printf( catgets(nl_fd,NL_SETN,14,"Following aliases are not installed (alias = fullname = address)\n\n"));
			printf( "%s\n", buffer );

		} else {
			if ( is_system )
				(void) put_alias( data, buffer, shash_table,
						MAX_SALIASES, argv );
			else
				(void) put_alias( data, buffer, uhash_table,
						MAX_UALIASES, argv );

			if ( count > (is_system ? MAX_SALIASES-1 : MAX_UALIASES-1)) {
				fault_cnt += 1;
				printf( catgets(nl_fd,NL_SETN,13,"\n\nAlias database is already full. \n" ) );
				printf( catgets(nl_fd,NL_SETN,14,"Following aliases are not installed (alias = fullname = address)\n\n"));
				printf( "%s\n", buffer );
			}
		}

		if ( buff_loaded == 0 )
			break;

	}

	if ( error ) {
		printf( catgets(nl_fd,NL_SETN,15,"\nNot saving tables.  Please fix your aliases and re-run %s!\n"),
		       argv[0] );
		exit( 1 );
	} else {

	/*
	 *   add an internal marker that indicates what size the table
      	 *   was built as -- we'll use ``=size='' as the alias, and the
      	 *   value will be "MAX_SALIASES" if we're building the system
      	 *   aliases, and "MAX_UALIASES" otherwise.
      	 */

		if ( is_system )
			put_end_token( data, shash_table, MAX_SALIASES );
		else
			put_end_token( data, uhash_table, MAX_UALIASES );

		if ( is_system )
			write( hash, shash_table, sizeof shash_table );
		else
			write( hash, uhash_table, sizeof uhash_table );

		close( hash );
		fclose( data );
		close( in );

		printf( catgets(nl_fd,NL_SETN,16,"Processed %d aliases (%d individual and %d group)\n"),
			count, count - groups_encountered, groups_encountered );
		exit( 0 );
	}
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
		if ( get_line(file, mybuffer, first_read) == -1 )
			return ( -1 );

		first_read = 0;

		if ( !is_comment(mybuffer))         
			strcpy( buffer, mybuffer );
			   
	} while ( strlen(buffer) == 0 );

	/*
	 *  now read in the rest (if there is any!) 
	 */

	do {
		if ( get_line(file, mybuffer, first_read) == -1 ) {
			buff_loaded = 0;	/* force a read next pass! */
			return ( 0 );		/* okay. let's just hand 'buffer'
					 	 * back! */
		}

		done = ( !whitespace(mybuffer[0]) || is_comment(&mybuffer) );

		if ( !done )
			if ( strlen(buffer)+strlen(mybuffer) < VERY_LONG_STRING )
				strcat( buffer, mybuffer );
			else {
				printf( catgets(nl_fd,NL_SETN,17,"\nAddress list is too long. \n" ) );
				printf( catgets(nl_fd,NL_SETN,18,"\nNot saving tables.  Please fix your aliases and try again!\n"));
				exit(1);
			}

                /* not done reading in buffer if this line is a comment */
		/* also not done if blank line, read next line          */

		done = ( done && !is_comment(&mybuffer) ) ;
	} while ( !done );

	return ( 0 );	
}


int
put_alias( data, buffer, table, size, argv )

	FILE		*data;
	char		*argv[],
			*buffer;
	struct alias_rec table[];
	int             size;

{
	/*
	 *  break buffer down into three pieces: aliases, comment, and address.
	 *  Make the appropriate entries in the table (size) 
	 *
	 *  Modified to know that lines with ':' as a separator are 
	 *  from the previous version of Elm -- we'll let users choose
	 *  if they'd like to modify the file or not...
	 */


	char            aliases[VERY_LONG_STRING], 
			address[VERY_LONG_STRING],
	                comment[VERY_LONG_STRING];
	int             first, 
			last, 
			has_colon,
			has_equal,
			num_equal,
			i = 0, 
			j = 0;


	has_colon = chloc(buffer, ':');
	has_equal = chloc(buffer, '=');

	 if ( has_equal == -1 && has_colon == -1 )
		return( 0 );			/* Ignore ! */
	else if ( has_equal == -1 && has_colon )

		/* has only colon delimiter!         */
		/* means we've hit an obsolete file! */

		(void) correct_obsolete_file( argv ); 

	(void) remove_all( ' ', TAB, buffer ); 

	has_equal = chloc(buffer, '=');

	for ( i = 0; buffer[i] != '=' && i < VERY_LONG_STRING && buffer[i] != '\0'; i++ )
		aliases[i] = buffer[i];

	aliases[i] = '\0';

	for ( i = strlen(buffer) - 1; buffer[i] != '=' && i > 0
					&& j < VERY_LONG_STRING ; i-- )
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

	(void) add_to_table( data, aliases, comment, address, table, size );

}


int
put_end_token( data, table, size )

	FILE           *data;
	struct alias_rec table[];
	int             size;

{
	/*
	 *  add an internal marker that indicates what size the table
	 *  was built as -- we'll use ``=size='' as the alias, and the
	 *  value will be "MAX_SALIASES" if we're building the system
	 *  aliases, and "MAX_UALIASES" otherwise.
	 */


	char            buffer[SLEN];


	sprintf( buffer, "%d", size );

	add_to_table( data, SIZE_INDICATOR, SIZE_COMMENT, buffer, table, size );
}


int
get_line( file, buffer, first_line )

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
		return ( 0 );
	}

	buff_loaded = 1;		/* we're going to get SOMETHING in the buffer */

	stat = ( fgets( buffer, VERY_LONG_STRING, file ) == NULL ? -1 : 0 );

	if ( stat != -1 )
		no_ret( buffer );

	return ( stat );
}


int
reverse( string )

	char           *string;

{
	/*
	 *  reverse the order of the characters in string... 
	 *  uses a bubble-sort type of algorithm!                 
	 */

	register int    f, 
			l;
	char            c;

	f = 0;
	l = strlen( string ) - 1;

	while ( f < l ) {
		c = string[f];
		string[f] = string[l];
		string[l] = c;
		f++;
		l--;
	}
}


int
add_to_table( data, aliases, comment, address, table, size )

	FILE            *data;
	char            *aliases, 
			*comment, 
			*address;
	struct alias_rec table[];
  	int             size;

{
	/*
	 *  add address + comment to datafile, incrementing offset count 
	 *  (bytes), then for each alias in the aliases string, add to the
	 *  hash table, with the associated pointer value! 
	 */


	static char     buf[VERY_LONG_STRING], 
			*word;
	long            additive = 1L;


	word = buf;				/* use the allocated space! */

	if ( group(address) ) {


#ifndef NOCHECK_VALIDNAME
		check_group( address, aliases );
#endif


		if ( error )
			return;			/* don't do work if we aren't to save it! */

		fprintf( data, "!%s\n", address );
		additive = 2L;
		groups_encountered++;
	} else {
		if ( error )
			return;			/* don't do work if we aren't to save it! */

		if ( strlen(comment) > 0 ) {
			fprintf( data, "%s (%s)\n", address, comment );

			additive = (long) ( strlen(comment) + 4 );
		} else
			fprintf( data, "%s\n", address, comment );
	}

	while ( (word = (char *) strtok(aliases, ", ")) != NULL ) {

		add_to_hash_table( word, offset, table, size ); 

		aliases = NULL;			/* let's get ALL entries via 'strtok' */
		if ( !equal(word, SIZE_INDICATOR) )
			count++;
	}

	if ( is_system ? count > MAX_SALIASES - 1 : count > MAX_UALIASES - 1 ) {
		printf( catgets(nl_fd,NL_SETN,19,"Sorry, but you have too many aliases in your file!\n") );
		printf( catgets(nl_fd,NL_SETN,20,"(the current limit for %s aliases is %d entries)\n"),
		       is_system ? catgets(nl_fd,NL_SETN,21,"system") : catgets(nl_fd,NL_SETN,22,"individual user"),
		       is_system ? MAX_SALIASES - 1 : MAX_UALIASES - 1 );
		error++;
	}

	offset = ( offset + (long) strlen(address) + additive );

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
	 * equals fields.  This assumes the left component of the first equal
	 * as alias part and the right component of the last equal as the
	 * address part.  The component between the last and the first equal
	 * (it may contain some equals), is assumed as comments.  That's OK if there is 
	 * no comment part(case of one equal).  Remove the leading and tailing
	 * ',' in the alias and address part.
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

	for ( last_equals = strlen(string) - 1; string[last_equals] != '=';
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

	return( 0 );
}


int
add_to_hash_table( word, offset, table, size )

	char           *word;
	long            offset;
	struct alias_rec table[];
	int             size;

{
	/*
	 *  add word and offset to current hash table. 
	 */
	

	register int    loc;


	if ( strlen(word) > 20 ) {
		printf( catgets(nl_fd,NL_SETN,23,"Sorry, but alias '%s' is too long.\n"), word );
		printf( catgets(nl_fd,NL_SETN,24,"(the current maximum length for aliases is %d characters)\n"),
		       20 );				/* hard coded  */
		exit( 1 );
	}

	loc = hash_it( word, size );

	while ( table[loc].name[0] != '\0' && !equal(table[loc].name, word) ) {
		loc = loc + 1 % size;

		if ( loc >= size )	/* over shoot */
			loc = 0;
	}

	if ( table[loc].name[0] == '\0' ) {
		strcpy( table[loc].name, word );
		table[loc].byte = offset;
	} else
		printf( catgets(nl_fd,NL_SETN,25,"Warning: duplicate alias '%s' found - multiples ignored.\n"),
		       word );

}


int
hash_it( string, table_size )

	char           *string;
	int		table_size;

{
	/*
	 *  compute the hash function of the string, returning
	 *  it (mod table_size) 
	 */


	register int    i, 
			sum = 0;


	for ( i = 0; string[i] != '\0'; i++ )
		sum += (int) string[i];

	return ( sum % table_size );
}


int
init_table( table, size )

	struct alias_rec 	table[];
	int             	size;

{
	/*
	 *  initialize hash table! 
	 */


	register int    i;


	for ( i = 0; i < size; i++ )
		table[i].name[0] = '\0';
}


int
read_in_system( table, size )

	struct alias_rec 	table[];
	int             	size;

{
	/*
	 *  read in the system hash table...to check for group aliases
	 *  from the user alias file (to ensure that there are no names
	 *  in the user group files that are not purely contained within
	 *  either alias table) 
	 */


	int             fd;
	char            fname[SLEN];


	sprintf( fname, "%s/%s", mailhome, ALIAS_HASH );

	if ( (fd = open(fname, O_RDONLY)) == -1 )
		return;		/* flag 'hash_table_loaded' not set! */

	(void) read( fd, table, size );

	close( fd );
	hash_table_loaded++;
}


#ifndef NOCHECK_VALIDNAME

int
check_group( names, groupname )

	char            *names, 
			*groupname;

{
	/*
	 *  one by one make sure each name in the group is defined
	 *  in either the system alias file or the user alias file.
	 *  This search is linearly dependent, so all group aliases
	 *  in the source file should appear LAST, after all the user
	 *  aliases! 
	 */


	char            *word, 
			*bufptr, 
			buffer[LONG_STRING];


	strcpy( buffer, names );
	bufptr = (char *) buffer;

	while ( (word = (char *) strtok(bufptr, ", ")) != NULL ) {
		if ( !can_find(word) )
			if ( !valid_name(word) ) {
				error++;
				printf( catgets(nl_fd,NL_SETN,26,"Sorry, but alias '%s' in the group %s is bad!\n"),
				       word, groupname );
			}

		bufptr = NULL;
	}
}

#endif


int
can_find( name )

	char           *name;

{
	/*
	 *  find name in either hash table...use 'is_system' variable to
	 *  determine if we should look in both or just system....    
	 */


	register int    loc;


	if ( strlen(name) > 20 ) {
		error++;
		printf( catgets(nl_fd,NL_SETN,27,"Sorry, but alias '%s' is too long.\n"), name );
		printf( catgets(nl_fd,NL_SETN,28,"(the current maximum length for aliases is %d characters)\n"),
		       20 );			/* hard coded, yick! */
		return ( 1 );			/* fake out: don't want 2 error messages! */
	}

	/*
	 *  system alias table... 
	 */

	if ( hash_table_loaded || is_system ) {
		loc = hash_it( name, MAX_SALIASES );

		while ( !equal(name, shash_table[loc].name) &&
		       shash_table[loc].name[0] != '\0' )
			loc = (loc + 1) % MAX_SALIASES;

		if ( equal(name, shash_table[loc].name) )
			return ( 1 );	/* found it! */
	}

	if ( !is_system ) {		/* okay! Let's check the user alias file! */
		loc = hash_it( name, MAX_UALIASES );

		while ( !equal(name, uhash_table[loc].name) &&
		       uhash_table[loc].name[0] != '\0' )
			loc = (loc + 1) % MAX_UALIASES;

		if ( equal(name, uhash_table[loc].name) )
			return ( 1 );	/* found it! */
	}
	return ( 0 );
}


int 
is_comment ( buffer )
        char 		*buffer;
{
	/* check to see if this s a comment line.    */
	/* i.e. buffer either starts with a "#" or   */
	/*      starts with a whitespace then a "#"  */
	/* A white space is a ' ' or '/t'.           */

	int		i = 0;

        if (buffer[0] == '#') return 1;
	if (strlen(buffer) == 0) return 0;

	while ( whitespace(buffer[i]) )
		i++;

	if (i <= strlen(buffer)) 
	{
            if (buffer[i] == '#') return 1;
        }
	return 0;

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

		while ( first <= last && loc < LONG_STRING-1 )
			comment[loc++] = buffer[first++];

		comment[loc] = '\0';
	}
}


int
update_alias_file_locations()

{
	/*
	 *  a short-term routine to ensure that the data files are
	 *  moved into the correct directory... and translated as
	 *  needed... 
	 */


	char            source[SLEN], 
			dest[SLEN], 
			command[SLEN];


	/*
	 *  first let's create the directory if it ain't there... 
	 */

	sprintf( source, "%s/.elm", home );
	(void) mkdir( source, 0700 );

	/*
	 *  now *link* the files... 
	 */

	sprintf( source, "%s/.alias_text", home );
	sprintf( dest, "%s/%s", home, ALIAS_TEXT );
	sprintf( command, "cat %s | sed 's/:/=/g' > %s", source, dest );

	(void) system( command );

	printf( catgets(nl_fd,NL_SETN,29,"    (translating alias file -- new format uses '=' instead of ':')\n") );

	sprintf( source, "%s/.alias_hash", home );
	sprintf( dest, "%s/%s", home, ALIAS_HASH );
	link( source, dest );

	sprintf( source, "%s/.alias_data", home );
	sprintf( dest, "%s/%s", home, ALIAS_DATA );
	link( source, dest );

	printf( catgets(nl_fd,NL_SETN,30,"\n*** Moved all data files into %s/.elm directory ***\n\n"),
	       home );
}


int
correct_obsolete_file( argv )

	char           *argv[];

{
	/*
	 *  this is called if the file is in the right place, but uses
	 *  ':' characters instead of '=' characters.  Odd, but we'll
	 *  deal with it in a mildly intelligent fashion at least...
	 */


	char            answer[2], 
			source[SLEN], 
			dest[SLEN], 
			command[SLEN];


	printf( catgets(nl_fd,NL_SETN,31,"\n*** Your Elm Alias file is in the old format, with ':' being used as ***\n") );
	printf( catgets(nl_fd,NL_SETN,32,"*** a field separator rather than the new '=' character.  Shall I go ***\n") );
	printf( catgets(nl_fd,NL_SETN,33,"*** ahead and translate the file for you (y/n) ? ") );
	fflush( stdout );


	do {	
		gets( answer );
	} while( answer[0] != 'y' && answer[0] != 'Y'
	      && answer[0] != 'n' && answer[0] != 'N' );

	if ( answer[0] != 'y' && answer[0] != 'Y' ) {
		printf( catgets(nl_fd,NL_SETN,34,"\n*** Since you don't want me to translate, you should do it yourself. ***\n") );
		printf( catgets(nl_fd,NL_SETN,35,"*** Using your favorite editor, change all ':' characters to '=',    ***\n") );
		printf( catgets(nl_fd,NL_SETN,36,"*** then rerun the ``elmalias'' command.                             ***\n\n") );
		exit( 1 );
	}

	/*
	 *  if we're here, then we want to translate the file.  The easy
	 *  way to do this will be to move the existing file to a new
	 *  location, then use 'sed' to translate it back into the old
	 *  location.  When we're done we'll "exec" this program again
	 *  and vanish...
	 */

	sprintf( source, "%s/%s", home, ALIAS_TEXT );
	sprintf( dest, "%s/%s.old", home, ALIAS_TEXT );

	link( source, dest );
	unlink( source );

	sprintf( command, "cat %s | sed 's/:/=/g' > %s", dest, source );

	(void) system( command );

	/*
	 * and we're outta here... 
	 */

	printf( catgets(nl_fd,NL_SETN,37,"\n(restarting the ``elmalias'' command)..\n\n") );

	execvp( argv[0], argv );

	printf( catgets(nl_fd,NL_SETN,38,"\nCouldn't re-exec the program for some reason.  Please do so by hand\n"));
	printf( catgets(nl_fd,NL_SETN,39,"by typing ``%s'' at the command line.\n\n"), argv[0] );
}
