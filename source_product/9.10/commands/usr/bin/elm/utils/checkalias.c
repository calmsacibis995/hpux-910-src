/**			checkalias.c			**/

/*  
 *  @(#) $Revision: 66.4 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  These routines are for reading alias data base of elm.
 *  The essence is same as routines in 'alias.c', 'aliasdb.c'
 *  or 'aliaslib.c' in '../src' directory. But they're more
 *  compact than those.
 */


#include <stdio.h>
#include <fcntl.h>

#include "defs.h"


#ifdef NLS
# include <nl_types.h>
# define NL_SETN  2
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


int		read_in_aliases = 0;

int system_files = 0;		/* do we have system aliases? */
int user_files = 0;		/* do we have user aliases?   */

int system_data;		/* fileno of system data file */
int user_data;			/* fileno of user data file   */

int userid;			/* uid for current user	      */
int groupid;			/* groupid for current user   */

char		home[SLEN];

struct alias_rec user_hash_table[ MAX_UALIASES ];
struct alias_rec system_hash_table[ MAX_SALIASES ];


char            *expand_group(),
		*get_alias_address(), 
		*expand_system(),
		*getenv(),
                *get_token(),
		*strcpy(),
		*strpbrk();
long            lseek();
static int	get_line();


#ifdef NLS
extern nl_catd		nl_fd;	/* message catalogue file    */
#endif NLS


int
checkalias( aliaslist )

	char		*aliaslist;

{
	/*
	 * checkalias checks whether the alias is defined or not.
	 * This searches and solves the alias only in the alias
	 * data base of elm( aliases.data ).
	 */


	int	i;
	char	*ptr,
	    	word[SLEN];


	i = get_word( aliaslist, 0, word );

	while( i > 0 ){

		if ( ( ptr = get_alias_address( word, 1, 0 )) != NULL )
			printf( "%s\t%s\n", word, ptr );
		else
			printf( catgets(nl_fd,NL_SETN,1,"%s\tnot defined\n"), word );

		i = get_word( aliaslist, i, word );
	}
}


char
*get_alias_address( name, mailing, depth )

	char            *name;
	int             mailing, 
			depth;

{
	/*
	 *  return the line from datafile that corresponds 
	 *  to the specified name.  If 'mailing' specified, then
	 *  fully expand group names.  Depth specifies the nesting
	 *  depth - the routine should always initially be called
	 *  with this equal 0.  Returns NULL if not found.
	 *  this searches the aliasname only in the elm alias database.
	 */


	static char     buffer[VERY_LONG_STRING];
	int             loc;
	int		i;
	char		stripped[VERY_LONG_STRING];


	if ( strlen(name) == 0 )
		return ( (char *) NULL );

	if ( !read_in_aliases )
		read_alias_files();

	if ( user_files )
		if ( (loc = find(name, user_hash_table, MAX_UALIASES)) >= 0 ) {
			lseek( user_data, user_hash_table[loc].byte, 0L );
			get_line( user_data, buffer );
			if ( buffer[0] == '!' && mailing )
				return ( expand_group(buffer, depth) );
			else if (mailing) {
				/* if alias maps to itself, then we are done.
				   if not, try to expand it further. */
				i = 0;
				while(buffer[i] != ' ' && buffer[i] != '\0') {
					stripped[i] = buffer[i];
					i++;
				}
				stripped[i] = '\0';
				if (strcmp(stripped, name) == 0)
					return((char *)buffer);
				else
					return(expand_group(buffer, depth));
			} else 
				return ( (char *) buffer );
		}

	if ( system_files )
		if ( (loc = find(name, system_hash_table, MAX_SALIASES)) >= 0 ) {
			lseek( system_data, system_hash_table[loc].byte, 0L );
			get_line( system_data, buffer );
			if ( buffer[0] == '!' && mailing )
				return ( expand_group(buffer, depth) );
			else if (mailing) {
				/* if alias maps to itself, then we are done.
				   if not, try to expand it further. */
				i = 0;
				while(buffer[i] != ' ' && buffer[i] != '\0') {
					stripped[i] = buffer[i];
					i++;
				}
				stripped[i] = '\0';
				if (strcmp(stripped, name) == 0)
					return((char *)buffer);
				else
					return(expand_group(buffer, depth));
			} else  
				return ( (char *) buffer );
		}

	return ( (char *) NULL );
}


char *
expand_group( members, depth )

	char           *members;
	int             depth;

{
	/*
	 *  Given a group of names separated by commas, this routine
	 *  will return a string that is the full addresses of each
	 *  member separated by spaces. Depth is an internal counter
	 *  that keeps track of the depth of nesting that the routine
	 *  is in...it's for the get_token routine!  
	 */


	static char     buffer[VERY_LONG_STRING];
	char            buf[VERY_LONG_STRING], *word, *address, *bufptr;
	char		comment[VERY_LONG_STRING];
	char           *strcpy();


	strcpy( buf, members );		/* parameter safety! */

	if ( depth == 0 )
		buffer[0] = '\0';	/* nothing in yet!   */

	if (buf[0] == '!') {
		/* advance beyond the '!' character used as group delimiter */
		bufptr = (char *)buf + 1;
		*comment = '\0';
	} else {

		/* this alias is not a group.  we must strip off the comment
		   field so we don't try to expand it.  we will put it back
		   on later. */

		for(bufptr = buf; *bufptr != ' ' && *bufptr != '\0'; bufptr++);
		strcpy(comment, bufptr);
		*bufptr = '\0';
		bufptr = buf;
	}

	depth++;			/* one deeper!       */

	while ( (word = get_token(bufptr, ", ", depth)) != NULL ) {

		if ( (address = get_alias_address(word, 1, depth)) == NULL ) {
			if (!equal(buffer, word)) {
				if (strlen(buffer)+strlen(word)+strlen(comment)+2<VERY_LONG_STRING)
					sprintf( buffer, "%s%s%s%s", buffer,
				    		(strlen(buffer) > 0) ? ", " : "", word, comment );
				else {
					printf(catgets(nl_fd,NL_SETN,9,"too many members in group alias"));
					break;
				}
			}
		} else if (!equal(buffer, address)) {
			if (strlen(buffer)+strlen(address)+2 < VERY_LONG_STRING)
				sprintf( buffer, "%s%s%s", buffer,
					(strlen(buffer) > 0) ? ", " : "", address );
			else {
				printf(catgets(nl_fd,NL_SETN,9,"too many members in group alias"));
				break;
			}
		}
		bufptr = NULL;
	}

	return ( (char *) buffer );
}


int
find( word, table, size )

	char	           *word;
	struct alias_rec   table[];
	int	           size;

{
	/*
	 *  find word and return loc, or -1 
	 */


	register int    loc,
			cnt;

	cnt = 0;

	if ( strlen(word) > 20 )
		return ( -1 );
	 
	loc = hash_it( word, size );

	while ( !equal(word, table[loc].name) ) {
		if ( table[loc].name[0] == '\0' || cnt >= size )
			return ( -1 );

		cnt+=1;
		loc+=1;

		if ( loc >= size )
			loc = 0;	/* overshoot !! */
	}

	return ( loc );
}


static int 
get_line( fd, buffer )

	int             fd;
	char           *buffer;

{
	/*
	 * Read from file fd.  End read upon reading either EOF or '\n'
	 * character (this is where it differs from a straight 'read'
	 * command!) 
	 */


	register int    i = 0;
	char            ch;


	while ( read(fd, &ch, 1) > 0)
		if (ch == '\n' || ch == '\r' 
			       || i >= VERY_LONG_STRING-1 ){
			buffer[i] = '\0';
			return;
		} else
			buffer[i++] = ch;
}


int 
check_for_size_marker( system_aliases )

	int             system_aliases;

{
	/*
	 *  this routine will check the alias file for the entry
	 *  ``=size='' (e.g. SIZE_INDICATOR) and then check it
	 *  against the current compiled size.  If they're different,
	 *  or it can't find it (which would mean they're different,
	 *  right?) then the program will suggest that the user go
	 *  ahead and recompile the aliases -- indeed it'll offer to
	 *  do that on the fly...
	 */


	char           *value, 
			answer[2];
	int             ret;


	if ( system_aliases ) {
		if ( (value = get_alias_address(SIZE_INDICATOR, 0, 0)) == NULL ){
			fprintf( stderr, catgets(nl_fd,NL_SETN,2,"\n** You appear to have a system alias file built for a different version of the\n") );
			fprintf( stderr, catgets(nl_fd,NL_SETN,3,"** Elm system.  You'll need to have your administrator rebuild it as root\n") );
			fprintf( stderr, catgets(nl_fd,NL_SETN,4,"** using the ``elmalias'' command (no modifications to the file are needed\n") );
			fprintf( stderr, catgets(nl_fd,NL_SETN,5,"** --> just invoke `elmalias -q' as user root).\n\n") );
			return;
		}

	} else {		/* user aliases */
		if ( (value = get_alias_address(SIZE_INDICATOR, 0, 0)) == NULL ){
			fprintf( stderr, catgets(nl_fd,NL_SETN,6,"\n** You appear to have an alias file built for a different version of the\n") );
			fprintf( stderr, catgets(nl_fd,NL_SETN,7,"** Elm system.  You'll need to rebuild it invoking just ``elmalias''\n") );

			return;
		}
	}
}


int 
read_alias_files()
{
	/*
	 *  read the system and user alias files, if present.
	 *  Set the flags 'systemfiles' and 'userfiles' accordingly.
	 */


	char		*cp,
	                fname[SLEN];
	int             hash;


	strcpy( home, ((cp = getenv("HOME")) == NULL)? "" : cp );

	if ( (hash = open(system_hash_file, O_RDONLY)) == -1 ) 
		goto user;

	read( hash, (char *) system_hash_table, sizeof system_hash_table );
	close( hash );

	/*
	 * and data file opened.. 
	 */

	if ( (system_data = open(system_data_file, O_RDONLY)) == -1 ) 
		goto user;

	system_files++;			/* got the system files! */

	/*
	 *  as a last final check... 
	 */

	read_in_aliases = TRUE;		/* mark it along the way.. */

	check_for_size_marker( 1 );

user:	sprintf( fname, "%s/%s", home, ALIAS_HASH );

	if ( (hash = open(fname, O_RDONLY)) == -1 ) 
		return;

	read( hash, (char *) user_hash_table, sizeof user_hash_table );
	close( hash );

	sprintf( fname, "%s/%s", home, ALIAS_DATA );

	if ( (user_data = open(fname, O_RDONLY)) == -1 ) 
		return;

	user_files++;			/* got user files too! */

	/*
	 *  and, just before we leave... 
	 */

	read_in_aliases = TRUE;

	check_for_size_marker( 0 );
}


int
get_word( buffer, start, word )

	char            *buffer, 
			*word;
	int             start;

{
	/*
	 * 	return next word in buffer, starting at 'start'.
	 *	delimiter is space or end-of-line.  Returns the
	 *	location of the next word, or -1 if returning
	 *	the last word in the buffer.  -2 indicates empty
	 *	buffer!  
	 */

	register int    loc = 0;

	while ( (buffer[start] == ' ' || buffer[start] == ',' 
	     ||	buffer[start] == '\t') && buffer[start] != '\0' )
		start++;

	if ( buffer[start] == '\0' )
		return ( -2 );	/* nothing IN buffer! */

	while ( buffer[start] != ' ' && buffer[start] != '\0'
	     && buffer[start] != ',' && buffer[start] != '\t'
	     && loc < SLEN )
		word[loc++] = buffer[start++];

	word[loc] = '\0';
	return ( start );
}



#define MAX_RECURSION		20	/* up to 20 deep recursion */


char 
*get_token( source, keys, depth )

	char            *source, 
			*keys;
	int             depth;

{
	/*
	 *  This function is similar to strtok() (see "../src/opt_utils")
	 *  but allows nesting of calls via pointers... 
	 */


	register int    last_ch;
	static char     *buffers[MAX_RECURSION];
	char            *return_value, 
			*sourceptr;


	if ( depth > MAX_RECURSION ) {
		printf( catgets(nl_fd,NL_SETN,8,"possible alias loop.  an alias could not be resolved in %d expansions!\n"), MAX_RECURSION );
		exit(1);
	}

	if ( source != NULL )
		buffers[depth] = source;

	sourceptr = buffers[depth];

	if ( *sourceptr == '\0' )
		return ( NULL );		/* we hit end-of-string last time!? */

	sourceptr += strspn( sourceptr, keys );	/* skip the bad.. */

	if ( *sourceptr == '\0' ) {
		buffers[depth] = sourceptr;
		return (NULL);			/* we've hit end-of-string   */
	}

	last_ch = strcspn( sourceptr, keys );	/* end of good stuff   */

	return_value = sourceptr;		/* and get the ret     */

	sourceptr += last_ch;			/* ...value            */

	if ( *sourceptr != '\0' )		/* don't forget if we're at end! */
		sourceptr++;

	return_value[last_ch] = '\0';		/* ..ending right      */

	buffers[depth] = sourceptr;		/* save this, mate!    */

	return ( (char *) return_value );	/* and we're outta here! */
}
