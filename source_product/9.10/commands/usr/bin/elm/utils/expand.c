/**			expand.c			**/

/*
 *  @(#) $Revision: 66.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *
 *  This is a library routine for the various utilities that allows
 *  users to have the standard 'Elm' folder directory nomenclature
 *  for all filenames (e.g. '+', '=' or '%').  It should be compiled
 *  and then linked in as needed.
 */


#include <stdio.h>

#include "defs.h"


#ifdef NLS
# include <nl_types.h>
# define NL_SETN   2
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


char           *expand_define();


#ifdef NLS
extern nl_catd		nl_fd;			/* message catalogue file    */
#endif NLS


int
expand( filename )

	char           *filename;

{
	/*
	 *  Expand the filename since the first character is a meta-
	 *  character that should expand to the "maildir" variable
	 *  in the users "elmrc" file...
 	 *
	 *  Note: this is a brute force way of getting the entry out 
	 *  of the elmrc file, and isn't recommended for the faint 
	 *  of heart!
	 */


	FILE            *rcfile;
	char            buffer[SLEN], 
			username[SLEN],
			*expanded_dir, 
			*home, 
			*getenv(), 
			*bufptr;
	int             foundit = 0;


	bufptr = (char *) buffer;		/* same address */

	if ( equal( filename, "!" ) ){
		strcpy( username, getenv("LOGNAME") );
		if ( strlen(username) == 0 )
			strcpy( username, getlogin() );
		if ( strlen(username) == 0 )
			strcpy( username, cuserid(NULL) );
		sprintf( filename, "%s%s", mailhome, username );
		return( 1 );
	}

	if ( (home = getenv("HOME")) == NULL ) {
		fprintf( stderr, catgets(nl_fd,NL_SETN,1,"Can't expand environment variable $HOME to find elmrc file!\n") );
		exit( 1 );
	}

	sprintf( buffer, "%s/%s", home, elmrcfile );

	if ( (rcfile = fopen(buffer, "r")) == NULL ) {
		sprintf( buffer, "%s/%s", home, "Mail" );
		expanded_dir = expand_define( bufptr );
		sprintf( buffer, "%s%s%s", expanded_dir,
			(expanded_dir[strlen(expanded_dir) - 1] == '/' ||
		 	filename[1] == '/') ? "" : "/", (char *) filename + 1 );
		strcpy( filename, buffer );
		return( 1 );
	}

	while(!foundit && (fgets(buffer, SLEN, rcfile) != NULL)) {
		if ( first_word(buffer, "maildir") || first_word(buffer, "folders") ) {

			while ( *bufptr != '=' && *bufptr )
				bufptr++;

			bufptr++;		/* skip the equals sign */

			while ( whitespace(*bufptr) && *bufptr )
				bufptr++;

			home = bufptr;		/* remember this address */

			while ( !whitespace(*bufptr) && *bufptr != '\n' )
				bufptr++;

			*bufptr = '\0';		/* remove trailing space */
			foundit++;
		}
	}

	fclose( rcfile );			/* be nice... */

	if ( !foundit ) {
		sprintf( buffer, "%s/%s", home, "Mail" );
		expanded_dir = expand_define( bufptr );
		sprintf( buffer, "%s%s%s", expanded_dir,
			(expanded_dir[strlen(expanded_dir) - 1] == '/' ||
		 	filename[1] == '/') ? "" : "/", (char *) filename + 1 );
		strcpy( filename, buffer );
		return( 1 );
	}

	/*
	 *  Home now points to the string containing your maildir, with
	 *  no leading or trailing white space...
	 */

	expanded_dir = expand_define( home );

	sprintf( buffer, "%s%s%s", expanded_dir,
		(expanded_dir[strlen(expanded_dir) - 1] == '/' ||
		 filename[1] == '/') ? "" : "/", (char *) filename + 1 );

	strcpy( filename, buffer );
}


char  
*expand_define( maildir )

	char           *maildir;

{
	/*
	 *  This routine expands any occurances of "~" or "$var" in
	 *  the users definition of their maildir directory out of
	 *  their elmrc file.
	 *
	 *  Again, another routine not for the weak of heart or staunch
	 *  of will!
	 */

	
	static char     buffer[SLEN];	/* static buffer AIEE!! */
	char            name[SLEN],	/* dynamic buffer!! (?) */
	                *nameptr,	/* pointer to name??     */
	                *value;		/* char pointer for munging */


	if ( *maildir == '~' )
		sprintf( buffer, "%s%s", getenv("HOME"), ++maildir );
	else if ( *maildir == '$' ) {			/* shell variable */

		/*
		 *  break it into a single word - the variable name 
		 */

		strcpy( name, (char *) maildir + 1 );	/* hurl the '$' */
		nameptr = (char *) name;

		while ( *nameptr != '/' && *nameptr )
			nameptr++;

		*nameptr = '\0';			/* null terminate */

		/*
		 *  got word "name" for expansion 
		 */

		if ( (value = getenv(name)) == NULL ) {
			fprintf( stderr, catgets(nl_fd,NL_SETN,2,"Couldn't expand shell variable $%s in elmrc!\n"), name );
			exit( 1 );
		}

		sprintf( buffer, "%s%s", value, maildir + strlen(name) + 1 );
	} else
		strcpy( buffer, maildir );

	return ( (char *) buffer );
}
