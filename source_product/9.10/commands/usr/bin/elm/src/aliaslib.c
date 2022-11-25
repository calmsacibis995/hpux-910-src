/**			aliaslib.c			**/

/*
 *  @(#) $Revision: 66.5 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  Library of functions dealing with the alias system...
 */


#include "headers.h"


#ifdef NLS
# define NL_SETN   4
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char            *expand_group(),
		*get_alias_address(), 
		*expand_system(),
                *get_token(),
		*strpbrk();
long            lseek();


char
*get_alias_address( name, mailing, depth )

	char            *name;
	int             mailing, 
			depth;

{
	/*
	 *  return the line from either datafile that corresponds 
	 *  to the specified name.  If 'mailing' specified, then
	 *  fully expand group names.  Depth specifies the nesting
	 *  depth - the routine should always initially be called
	 *  with this equal 0.  Returns NULL if not found  
	 */


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
			get_line( user_data, alias_address );

			if ( alias_address[0] == '!' && mailing )
				return ( expand_group(alias_address, depth) );
			else if ( strpbrk(alias_address, "!@:") != NULL )	/* has a hostname */
		

#ifdef DONT_TOUCH_ADDRESSES 
				return( (char *) alias_address );
#else
			  	return( expand_system(alias_address, TRUE) );
#endif 


			else if (mailing) {
				/* if alias maps to itself, then we are done.
				   if not, try to expand it further. */
				i = 0;
				while(alias_address[i] != ' ' &&
				      alias_address[i] != '\0') {
					stripped[i] = alias_address[i];
					i++;
				}
				stripped[i] = '\0';
				if (strcmp(stripped, name) == 0)
					return((char *)alias_address);
				else
					return(expand_group(alias_address, depth));
			} else
				return ( (char *) alias_address );
		}

	if ( system_files )
		if ( (loc = find(name, system_hash_table, MAX_SALIASES)) >= 0 ) {
			lseek( system_data, system_hash_table[loc].byte, 0L );
			get_line( system_data, alias_address );

			if ( alias_address[0] == '!' && mailing )
				return ( expand_group(alias_address, depth) );
			else if ( strpbrk(alias_address, "!@:") != NULL )	/* has a hostname */


#ifdef DONT_TOUCH_ADDRESSES 
				return( (char *) alias_address );
#else
				return( expand_system(alias_address, TRUE) );
#endif 


			else if (mailing) {
				/* if alias maps to itself, then we are done.
				   if not, try to expand it further. */
				i = 0;
				while(alias_address[i] != ' ' &&
				      alias_address[i] != '\0') {
					stripped[i] = alias_address[i];
					i++;
				}
				stripped[i] = '\0';
				if (strcmp(stripped, name) == 0)
					return((char *)alias_address);
				else
					return(expand_group(alias_address, depth));
			} else
				return ( (char *) alias_address );
		}

	return ( (char *) NULL );
}


char 
*expand_system( buffer, show_errors )

	char           *buffer;
	int             show_errors;

{
	/*
	 *  This routine will check the first machine name in the given path 
	 *  (if any) and expand it out if it is an alias...if not, it will 
	 *  return what it was given.  If show_errors is false, it won't 
	 *  display errors encountered...
	 */


	dprint( 6, (debugfile, "expand_system(%s, show-errors=%s)\n", buffer,
		   onoff(show_errors)) );
	findnode( buffer, show_errors );

	return ( (char *) buffer );

}


char 
*expand_group( members, depth )

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
	 

	char            buf[VERY_LONG_STRING], *word, *address, *bufptr;
	char		comment[VERY_LONG_STRING];
	char           *strcpy();


	strcpy( buf, members );			/* parameter safety! */

	if ( depth == 0 )
		expanded_group[0] = '\0';		/* nothing in yet!   */

	if (buf[0] == '!') {
		/* advance beyond the '!' character used as group delimiter */
		bufptr = (char *)buf + 1;
		*comment = '\0';
	} else {

		/* this alias is not a group.  we must strip off the comment
		   field so we don't try to expand it.  we will put it back
		   on later */

		for(bufptr = buf; *bufptr != ' ' && *bufptr != '\0'; bufptr++);
		strcpy(comment, bufptr);
		*bufptr = '\0';
		bufptr = buf;
	}

	depth++;				/* one deeper!       */

	while ( (word = get_token(bufptr, ", ", depth, TRUE)) != NULL ) {
		if ( (address = get_alias_address(word, 1, depth)) == NULL ) {


#ifdef ENABLE_NAMESERVER
			if ( check_nameserver(word) ) {
				if ( !equal(expanded_group, word) )
					if ( strlen(expanded_group)+strlen(word)+strlen(comment)+2 
						< VERY_LONG_STRING )
						sprintf( expanded_group, "%s%s%s%s",
							 expanded_group, 
							 (strlen(expanded_group) > 0) 
							 ? ", " : "", word, comment );
					else
						error("too many member in group alias");
			} else
#endif


#ifndef NOCHECK_VALIDNAME
			if ( !valid_name(word) ) {
				dprint( 3, (debugfile, 
					   "Encountered illegal address %s in %s\n",
					   word, "expand_group") );
				error1( catgets(nl_fd,NL_SETN,1,"%s is an illegal address!"), word );
				return ( (char *) NULL );
			} else
#endif


			if ( !equal(expanded_group, word) )
				if ( strlen(expanded_group)+strlen(word)+strlen(comment)+2 
						< VERY_LONG_STRING )
					sprintf( expanded_group, "%s%s%s%s", expanded_group,
				        (strlen(expanded_group) > 0) ? ", " : "", word, comment);
				else
					error("too many member in group alias");

		} else if ( !equal(expanded_group, address) )
			if ( strlen(expanded_group)+strlen(address)+2 
						< VERY_LONG_STRING )
				sprintf( expanded_group, "%s%s%s", expanded_group,
				(strlen(expanded_group) > 0) ? ", " : "", address );
			else
				error("too many member in group alias");

		bufptr = NULL;
	}

	return ( (char *) expanded_group );
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

	if ( strlen(word) > 20 ) {
		dprint( 3, (debugfile,
			   "Overly long alias name entered -- assuming address for\n> %s\n",
			   word) );
		return ( -1 );
	}

	loc = hash_it( word, size );

	while ( !equal(word, table[loc].name) ) {
		if ( table[loc].name[0] == '\0' || cnt >= size )
			return ( -1 );

		cnt+=1;
		loc+=1;

		if ( loc >= size )
			loc = 0;
	}

	return ( loc );
}


int
hash_it( string, table_size )

	char           *string;
	int             table_size;

{
	/*
	 *  compute the hash function of the string, returning
	 *  it (mod table_size) 
	 */


	register int    i, sum = 0;


	for ( i = 0; string[i] != '\0'; i++ )
		sum += (int) string[i];

	return ( sum % table_size );
}


int 
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


	while ( read(fd, &ch, 1) > 0 )
		if ( ch == '\n' || ch == '\r'
				|| i == VERY_LONG_STRING ) {
			buffer[i] = 0;
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


	if (system_aliases) {
		if ( (value = get_alias_address(SIZE_INDICATOR, 0, 0)) == NULL ){
			fprintf( stderr,
				 catgets(nl_fd,NL_SETN,2,"\n** You appear to have a system alias file built for a different version of the\n") );
			fprintf( stderr,
				 catgets(nl_fd,NL_SETN,3,"** Elm system.  You'll need to have your administrator rebuild it as root\n") );
			fprintf( stderr,
				 catgets(nl_fd,NL_SETN,4,"** using the ``elmalias'' command (no modifications to the file are needed\n") );
			fprintf( stderr,
			catgets(nl_fd,NL_SETN,5,"** --> just invoke `elmalias -q' as user root).\n\n") );
			dprint( 1, (debugfile,
				   "\n** wrong size hash table for system alias file...user must fix!\n\n") );
			leave( 1 );
		}

	} else {					/* user aliases */
		if ( (value = get_alias_address(SIZE_INDICATOR, 0, 0)) == NULL ){
			fprintf( stderr,
				 catgets(nl_fd,NL_SETN,6,"\n** You appear to have an alias file built for a different version of the\n") );
			fprintf( stderr,
				 catgets(nl_fd,NL_SETN,7,"** Elm system.  You'll need to rebuild it using the ``elmalias'' command,\n") );
			fprintf( stderr,
				 catgets(nl_fd,NL_SETN,8,"** or, if you'd like, I can do it directly.  Shall I (y/n) ? ") );
			fflush( stdout );

			while ( 1 ) {
				answer[0] = (char)ReadCh();

				if ( answer[0] != 'y' && answer[0] != 'Y'
				  && answer[0] != 'n' && answer[0] != 'N' )
					putchar( (char) 007 );	/* BEEP ! */
				else
					break;
			}

			if ( answer[0] != 'y' && answer[0] != 'Y' ) {
				fprintf( stderr, "No" );
				fprintf( stderr,
					 catgets(nl_fd,NL_SETN,9,"\n\n** I won't do it, but you will need to by hand instead: just type the\n") );
				fprintf( stderr,
					 catgets(nl_fd,NL_SETN,10,"** command ``elmalias'' at the next prompt -- no changes to the actual\n") );
				fprintf( stderr,
				         catgets(nl_fd,NL_SETN,11,"** alias text file are needed).\n\n") );
				dprint( 1, (debugfile,
					   "\n** wrong size hash table for user alias file...user wants to fix by hand!\n\n") );
				leave( 1 );
			} else
				fprintf( stderr, 
					 catgets(nl_fd,NL_SETN,12,"Yes\n\n** rebuilding alias database...\n\n") );

			/*
			 *  fixing the users alias file... 
			 *  some stuff we need to do to get this program to be happy 
			 */

			Raw( OFF );
			setgid( groupid );
			setuid( userid );

			if ( (ret = system(newalias_cmd)) != 0 ) {
				dprint( 1, (debugfile,
					   "\n** wrong size hash table for user alias file.  We tried to rebuild it\n") );
				dprint( 1, (debugfile,
					   "** by invoking \"%s\" and got a return code of %d, so we're\n",
					   newalias_cmd, ret) );
				dprint( 1, (debugfile,
					   "** going to let the user fix it by hand.\n\n") );
				fprintf( stderr,
					 catgets(nl_fd,NL_SETN,13,"\n** problem rebuilding aliases (%d ; please fix by hand.\n\n"),
					ret );

			} else {
				dprint( 1, (debugfile,
					   "\n** wrong size hash table for user alias file.  We rebuilt it without\n") );
				dprint( 1, (debugfile,
					   "** any problems by invoking \"%s\".  Now the user has to restart\n",
					   newalias_cmd) );
				dprint( 1, (debugfile, "** the program.  \n\n") );
				fprintf( stderr,
					 catgets(nl_fd,NL_SETN,14,"\n** aliases rebuilt, please reinvoke the Elm program.\n\n") );
			}

			leave( 1 );
		}
	}
}
