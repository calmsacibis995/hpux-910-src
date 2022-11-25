/**			mailfrom.c			**/

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
 *  print out whom each message is from in the pending folder or specified 
 *  one, including a subject line if available.. 
 *
 */


#include <stdio.h>

#include "defs.h"




#ifdef NLS
# include <nl_types.h>
# define NL_SETN   1
#else NLS
# define catgets(i,sn,mn,s) (s)
#endif NLS


#define LINEFEED	(char) 10

#define metachar(c)	(c == '!' || c == '=' || c == '+' || c == '%')


FILE            *mailfile;

char            *expand_define(),
		*strcpy();
int             number = 0,	/* should we number the messages?? */
                verbose = 0;	/* and should we prepend a header? */


#ifdef NLS
nl_catd		nl_fd;			/* message catalogue file    */
char		lang[NLEN];			/* language name for NLS     */
#endif NLS


main( argc, argv )

	int             argc;
	char           *argv[];

{
	char            infile[LONG_SLEN], 
			username[SLEN], 
			c;
	int             multiple_files = 0, 
			output_files = 0;

	extern int      opt_index;





#ifdef NLS
	/*
	 * initalize for native language messages.... now set ready 
	 */

	strcpy( lang, getenv( "LANG" ) );
	nl_init( lang );
	nl_fd = catopen( "mailfrom", 0 );
#endif NLS


	while ( (c = get_options(argc, argv, "nv")) > 0 )
		switch ( c ) {
		case 'n':
			number++;
			break;

		case 'v':
			verbose++;
			break;
		}

	if ( c == -1 ) {
		printf( catgets(nl_fd,NL_SETN,1,"Usage: %s [ -n ] [ filename ]\n"), argv[0] );
		exit( 1 );
	}

	if ( opt_index == argc ) {
		strcpy( username, getenv("LOGNAME") );

		if ( strlen(username) == 0 )
			strcpy( username, getlogin() );

		if ( strlen(username) == 0 )
			cuserid( username );

		sprintf( infile, "%s%s", mailhome, username );
		opt_index -= 1;			/* ensure one pass through loop */
	}

	multiple_files = ( argc - opt_index > 1 );

	while ( opt_index < argc ) {

		if ( multiple_files ) {
			strcpy( infile, argv[opt_index] );
			printf( "%s%s: \n", output_files++ > 0 ? "\n" : "", infile );
		} else if ( infile[0] == '\0' )
			strcpy( infile, argv[opt_index] );

		if ( metachar(infile[0]) ) {
			if ( expand(infile) == 0 ) {
				fprintf( stderr, "%s: couldn't expand filename %s!\n",
					argv[0], infile );
				exit( 1 );
			}
		}

		if ( (mailfile = fopen(infile, "r")) == NULL ) {
			if ( opt_index + 1 == argc )
				printf( catgets(nl_fd,NL_SETN,2,"No mail\n") );
			else {
				if ( infile[0] == '/' )
					printf( catgets(nl_fd,NL_SETN,3,"Couldn't open folder \"%s\".\n"), infile );
				else {
					sprintf( infile, "%s%s", mailhome, argv[opt_index] );
					if ( (mailfile = fopen(infile, "r")) == NULL )
						printf( catgets(nl_fd,NL_SETN,4,"Couldn't open folders \"%s\" or \"%s\".\n"),
						   argv[opt_index], infile );
					else if ( read_headers(0) == 0 )
						printf( catgets(nl_fd,NL_SETN,5,"No messages in that folder!\n") );
				}
			}

		} else if ( read_headers(opt_index + 1 == argc) == 0 )
			if ( opt_index + 1 == argc )
				printf( catgets(nl_fd,NL_SETN,2,"No mail\n") );
			else
				printf( catgets(nl_fd,NL_SETN,5,"No messages in that folder!\n") );

		opt_index++;
	}



#ifdef NLS
	catclose( nl_fd );
#endif NLS


	exit( 0 );
}


int
read_headers( user_mailbox )

	int             user_mailbox;

{
	/*
	 *  Read the headers, output as found.  User-Mailbox is to guarantee
	 *  that we get a reasonably sensible message from the '-v' option
	 */


	char            buffer[LONG_SLEN], 
			from_whom[SLEN], 
			subject[SLEN];
	register int    subj = 0, 
			in_header = 1, 
			count = 0;


	while ( fgets(buffer, LONG_SLEN, mailfile) != NULL ) {
		if ( real_from(buffer, from_whom) ) {
			subj = 0;
			in_header = 1;
		} else if ( in_header ) {
			if ( first_word(buffer, ">From") )
				forwarded( buffer, from_whom );	/* return address */

			else if ( first_word(buffer, "Subject:") ||
				 first_word(buffer, "Re:") ) {
				if ( !subj++ ) {
					remove_first_word( buffer );
					strcpy( subject, buffer );
				}

			} else if ( first_word(buffer, "From:") )
				parse_arpa_from( buffer, from_whom );

			else if ( buffer[0] == LINEFEED ) {
				if ( verbose && count == 0 )
					printf( catgets(nl_fd,NL_SETN,6,"%s contains the following messages:\n\n"),
					       user_mailbox ? "Your mailbox" : "Folder" );

				in_header = 0;			/* in body of message! */
				show_header( count + 1, from_whom, subject );
				from_whom[0] = 0;
				subject[0] = 0;
				count++;
			}
		}
	}
	return ( count );
}


int
real_from( buffer, who )

	char   		        *buffer,
				*who;

{
	/*
	 *     Returns true iff 's' has the seven 'from' fields, (or
	 *     8 - some machines include the TIME ZONE!!!)
	 */


	char            buff6th[STRING],
			buff7th[STRING],
			buff8th[STRING],
			buff9th[STRING];


	if ( !first_word(buffer, "From ") )
		return ( FALSE );

	buff6th[0] = buff7th[0] = buff8th[0] = buff9th[0] = '\0';
        
	/*
	 * From <user> <day> <month> <date> <hr:min:sec> [<tz name>] <year> 
	 */

	sscanf( buffer, "%*s %s %*s %*s %*s %s %s %s %s",
		who, buff6th, buff7th, buff8th ,buff9th );

	if ( strlen(buff6th) != 0 && strlen(buff7th) != 0 &&
	     strlen(buff8th) != 0 && strlen(buff9th) != 0    )
		return ( FALSE );

	if ( strlen(buff7th) == 0 && strlen(buff8th) == 0 &&
	                             strlen(buff9th) == 0    )
		return ( FALSE );

	/* Check time field */
	if ( strlen(buff6th) < 3 )
		return ( FALSE );

	if ( buff6th[1] != ':' && buff6th[2] != ':' )
		return ( FALSE );

	return ( TRUE );

}


int
forwarded( buffer, who )

	char            *buffer, 
			*who;

{
	/*
	 *  change 'from' and date fields to reflect the ORIGINATOR of 
	 *  the message by iteratively parsing the >From fields... 
	 */


	char            machine[80], 
			buff[80];


	machine[0] = '\0';

	sscanf( buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %*s %s",
	        who, machine ); 	/* has TZ field ? */

	if ( machine[0] == '\0')	/* has no TZ      */
		sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %s",
		       who, machine );

	if ( machine[0] == '\0')	/* try for srm address */
		sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %s",
		       who, machine );

	if ( machine[0] == '\0' )
		sprintf( buff, "anonymous" );
	else
		sprintf( buff, "%s!%s", machine, who );

	strncpy( who, buff, 80 );
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
move_left( string, chars )

	char            string[];
	int             chars;

{
	/*
	 *  moves string chars characters to the left DESTRUCTIVELY 
	 */


	register int    i;


	chars--;			/* index starting at zero! */

	for ( i = chars; string[i] != '\0' && string[i] != '\n'; i++ )
		string[i - chars] = string[i];

	string[i - chars] = '\0';
}


int
show_header( count, from, subject )

	int             count;
	char            *from, 
			*subject;

{
	/*
	 *  output header in clean format, including abbreviation
	 *  of return address if more than one machine name is
	 *  contained within it! 
	 */


	char            buffer[SLEN];
	int             loc, i = 0, 
			exc = 0;


#ifdef PREFER_UUCP
	char           *p;

	if ( chloc(from, '!') != -1 && chloc(from, '@') > 0 ) {
		for ( p = from; *p != '@'; p++ );
		*p = '\0';
	}
#endif


	loc = strlen( from );

	while ( exc < 2 && loc > 0 )
		if ( from[--loc] == '!' )
			exc++;

	if ( exc == 2 ) {		/* lots of machine names!  Get last one */
		loc++;
		while ( loc < strlen(from) && loc < SLEN )
			buffer[i++] = from[loc++];
		buffer[i] = '\0';
		if ( number )
			printf( "%3d: %-20s  %s\n", count, buffer, subject );
		else
			printf( "%-20s  %s\n", buffer, subject );

	} else if ( number )
		printf( "%3d: %-20s  %s\n", count, from, subject );
	else
		printf( "%-20s  %s\n", from, subject );
}


int
parse_arpa_from( buffer, newfrom )

	char            *buffer, 
			*newfrom;

{
	/*
	 *  try to parse the 'From:' line given... It can be in one of
	 *  two formats:
	 *	From: Dave Taylor <hpcnou!dat>
	 *  or  From: hpcnou!dat (Dave Taylor)
	 *  Change 'newfrom' ONLY if sucessfully parsed this entry and
	 *  the resulting name is non-null! 
	 */


	char            temp_buffer[SLEN], 
			*temp;
	register int    i, 
			j = 0;


	temp = (char *) temp_buffer;
	temp[0] = '\0';

	no_ret( buffer );			/* blow away '\n' char! */

	if ( lastch(buffer) == '>' ) {
		for ( i = strlen("From: "); buffer[i] != '\0' && buffer[i] != '<'
		      					      && buffer[i] != '('; i++ )
			temp[j++] = buffer[i];

		temp[j] = '\0';

	} else if ( lastch(buffer) == ')' ) {
		for ( i = strlen(buffer) - 2; buffer[i] != '\0' && buffer[i] != '(' 
		     					        && buffer[i] != '<'; i-- )
			temp[j++] = buffer[i];
		temp[j] = '\0';
		reverse( temp );
	}

	if ( strlen(temp) > 0 ) {		/* mess with buffer... */

		/*
		 * remove leading spaces... 
		 */

		while ( whitespace(temp[0]) )
			temp = (char *) ( temp + 1 );	/* increment address! */

		/*
		 * remove trailing spaces... 
		 */

		i = strlen( temp ) - 1;

		while ( whitespace(temp[i]) )
			temp[i--] = '\0';

		/*
		 * if anything is left, let's change 'from' value! 
		 */

		if ( strlen(temp) > 0 )
			strcpy( newfrom, temp );
	}
}


int
reverse( string )

	char           *string;

{
	/*
	 *  reverse string... pretty trivial routine, actually! 
	 */


	char            buffer[SLEN];
	register int    i, 
			j = 0;


	for ( i = strlen(string) - 1; i >= 0; i-- )
		buffer[j++] = string[i];

	buffer[j] = '\0';

	strcpy( string, buffer );
}
